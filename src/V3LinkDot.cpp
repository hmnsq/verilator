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
//শীর্ষ
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

#অন্তর্ভাব "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাব "V3LinkDot.h"

#অন্তর্ভাব "V3Global.h"
#অন্তর্ভাব "V3Graph.h"
#অন্তর্ভাব "V3LinkDotIfaceCapture.h"
#অন্তর্ভাব "V3MemberMap.h"
#অন্তর্ভাব "V3Parse.h"
#অন্তর্ভাব "V3Randomize.h"
#অন্তর্ভাব "V3Stats.h"
#অন্তর্ভাব "V3String.h"
#অন্তর্ভাব "V3SymTable.h"

#অন্তর্ভাব <algorithm>
#অন্তর্ভাব <cstdlib>
#অন্তর্ভাব <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

স্থির string extractDottedPath(Ast_গ্রন্থি* nodep, তার্কিক& hasPartSelect) {
    যদি (AstParseRef* ধ্রুব refp = VN_CAST(nodep, ParseRef)) {
        প্রত্যাগম refp->name();
    } অন্যথা যদি (AstVarRef* ধ্রুব refp = VN_CAST(nodep, VarRef)) {
        প্রত্যাগম refp->name();
    } অন্যথা যদি (AstDot* ধ্রুব dotp = VN_CAST(nodep, Dot)) {
        ধ্রুব string lhs = extractDottedPath(dotp->lhsp(), hasPartSelect);
        ধ্রুব string rhs = extractDottedPath(dotp->rhsp(), hasPartSelect);
        প্রত্যাগম VString::dot(lhs, ".", rhs);
    } অন্যথা যদি (VN_IS(nodep, SelBit) || VN_IS(nodep, SelExtract) || VN_IS(nodep, SelPlus)
               || VN_IS(nodep, SelMinus)) {
        hasPartSelect = সত্য;
        প্রত্যাগম "";
    }
    প্রত্যাগম "";
}
স্থির string lexicalDisablePath(Ast_গ্রন্থি* ধ্রুব targetp) {
    std::vector<string> names;
    প্রতি (Ast_গ্রন্থি* curp = targetp; curp; curp = curp->aboveLoopp()) {
        যদি (Ast_গ্রন্থি_Block* ধ্রুব blockp = VN_CAST(curp, গ্রন্থি_Block)) {
            যদি (blockp->name() != "") names.ঠেলো_back(blockp->name());
        } অন্যথা যদি (Ast_গ্রন্থি_FTask* ধ্রুব ftaskp = VN_CAST(curp, গ্রন্থি_FTask)) {
            names.ঠেলো_back(ftaskp->name());
        } অন্যথা যদি (VN_IS(curp, গ্রন্থি_Module)) {
            ভঙ্গ;
        }
    }
    string path;
    প্রতি (auto it = names.crbegin(); it != names.crend(); ++it) {
        path = VString::dot(path, ".", *it);
    }
    প্রত্যাগম path;
}
স্থির string targetInstancePath(Ast_গ্রন্থি* ধ্রুব targetp, ধ্রুব string& targetPath) {
    ধ্রুব string lexicalPath = lexicalDisablePath(targetp);
    যদি (lexicalPath == "" || targetPath == lexicalPath) প্রত্যাগম "";
    ধ্রুব string suffix = "." + lexicalPath;
    যদি (!VString::endsWith(targetPath, suffix)) প্রত্যাগম "";
    প্রত্যাগম targetPath.substr(0, targetPath.size() - suffix.size());
}

// ######################################################################
//  Matcher classes (for suggestion matching)

বর্গ Link_গ্রন্থি_Matcher_Class_Or_Package final : সার্বজনিক VNodeMatcher {
সার্বজনিক:
    তার্কিক গ্রন্থি_Match(ধ্রুব Ast_গ্রন্থি* nodep) ধ্রুব override {
        প্রত্যাগম VN_IS(nodep, Class) || VN_IS(nodep, Package);
    }
};
বর্গ Link_গ্রন্থি_Matcher_FTask final : সার্বজনিক VNodeMatcher {
সার্বজনিক:
    তার্কিক গ্রন্থি_Match(ধ্রুব Ast_গ্রন্থি* nodep) ধ্রুব override { প্রত্যাগম VN_IS(nodep, গ্রন্থি_FTask); }
};
বর্গ Link_গ্রন্থি_Matcher_Modport final : সার্বজনিক VNodeMatcher {
সার্বজনিক:
    তার্কিক গ্রন্থি_Match(ধ্রুব Ast_গ্রন্থি* nodep) ধ্রুব override { প্রত্যাগম VN_IS(nodep, Modport); }
};
বর্গ Link_গ্রন্থি_Matcher_Prod final : সার্বজনিক VNodeMatcher {
সার্বজনিক:
    তার্কিক গ্রন্থি_Match(ধ্রুব Ast_গ্রন্থি* nodep) ধ্রুব override { প্রত্যাগম VN_IS(nodep, RSProd); }
};
বর্গ Link_গ্রন্থি_Matcher_Var final : সার্বজনিক VNodeMatcher {
সার্বজনিক:
    তার্কিক গ্রন্থি_Match(ধ্রুব Ast_গ্রন্থি* nodep) ধ্রুব override {
        প্রত্যাগম VN_IS(nodep, Var) || VN_IS(nodep, LambdaArgRef);
    }
};
বর্গ Link_গ্রন্থি_Matcher_Var_IO final : সার্বজনিক VNodeMatcher {
সার্বজনিক:
    তার্কিক গ্রন্থি_Match(ধ্রুব Ast_গ্রন্থি* nodep) ধ্রুব override {
        ধ্রুব AstVar* ধ্রুব varp = VN_CAST(nodep, Var);
        যদি (!varp) প্রত্যাগম অসত্য;
        প্রত্যাগম varp->isIO();
    }
};
বর্গ Link_গ্রন্থি_Matcher_Var_Param final : সার্বজনিক VNodeMatcher {
সার্বজনিক:
    তার্কিক গ্রন্থি_Match(ধ্রুব Ast_গ্রন্থি* nodep) ধ্রুব override {
        ধ্রুব AstVar* ধ্রুব varp = VN_CAST(nodep, Var);
        যদি (!varp) প্রত্যাগম অসত্য;
        প্রত্যাগম varp->isParam();
    }
};

//######################################################################
// LinkDot state, as a visitor of each AstNode

বর্গ LinkDotState final {
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
    ধ্রুব VNUser1InUse m_inuser1;
    ধ্রুব VNUser2InUse m_inuser2;
    ধ্রুব VNUser4InUse m_inuser4;

সার্বজনিক:
    // ENUMS
    // In order of priority, compute first ... compute last
    গণনা SAMNum : uint8_t { SAMN_MODPORT, SAMN_IFTOP, SAMN__MAX };  // Values for m_scopeAliasMap

গুপ্ত:
    // TYPES
    using ScopeAliasMap = std::unordered_map<VSymEnt*, VSymEnt*>;
    using IfaceModSyms = std::vector<std::pair<AstIface*, VSymEnt*>>;

    স্থির LinkDotState* s_errorThisp;  // Last self, for error reporting only

    // MEMBERS
    VSymGraph m_syms;  // Symbol table by hierarchy
    VSymGraph m_mods;  // Symbol table of all module names
    VSymEnt* m_dunitEntp = nullptr;  // $unit entry
    std::multimap<std::string, VSymEnt*>
        m_nameScopeSymMap;  // Map of scope referenced by non-pretty textual name
    std::set<std::pair<Ast_গ্রন্থি_Module*, std::string>>
        m_implicitNameSet;  // For [module][signalname] if we can implicitly create it
    std::array<ScopeAliasMap, SAMN__MAX> m_scopeAliasMap;  // Map of <lhs,rhs> aliases
    std::vector<VSymEnt*> m_ifaceVarSyms;  // List of AstIfaceRefDType's to be imported
    IfaceModSyms m_ifaceModSyms;  // List of AstIface+Symbols to be processed
    ধ্রুব VLinkDotStep m_step;  // Operational step

সার্বজনিক:
    // METHODS

    শূন্য dumpSelf(ধ্রুব string& nameComment, তার্কিক force = অসত্য) {
        যদি (debug() >= 6 || dumpLevel() >= 6 || force) {
            ধ্রুব string filename = v3Global.debugFilename(nameComment) + ".txt";
            UINFO(4, "Dumping " << filename);
            ধ্রুব std::unique_ptr<std::ofstream> logp{V3File::new_ofstream(filename)};
            যদি (logp->fail()) v3fatal("Can't write file: " << filename);
            std::ostream& os = *logp;
            // TODO the symbol table has node pointers which may be
            // dangling, as we call deleteTree in these visitors without
            // pushing for later deletion (or deleting from the symbol table)
            // So, only completely safe to call this when under --debug or --debug-leak.
            m_syms.dumpSelf(os);
            তার্কিক first = সত্য;
            প্রতি (পূর্ণাঙ্ক samn = 0; samn < SAMN__MAX; ++samn) {
                যদি (!m_scopeAliasMap[samn].empty()) {
                    যদি (first) os << "\nScopeAliasMap:\n";
                    first = অসত্য;
                    প্রতি (ScopeAliasMap::iterator it = m_scopeAliasMap[samn].begin();
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
    স্থির শূন্য preErrorDumpHandler() {
        যদি (s_errorThisp) s_errorThisp->preErrorDump();
    }
    শূন্য preErrorDump() {
        স্থির তার্কিক s_diddump = অসত্য;
        যদি (!s_diddump && dump_বৃক্ষ_Level()) {
            s_diddump = সত্য;
            dumpSelf("linkdot-preerr", সত্য);
            v3Global.rootp()->dump_বৃক্ষ_File(v3Global.debugFilename("linkdot-preerr.tree"));
        }
    }

    // CONSTRUCTORS
    LinkDotState(AstNetlist* rootp, VLinkDotStep step)
        : m_syms{rootp}
        , m_mods{rootp}
        , m_step{step} {
        UINFO(4, __FUNCTION__ << ": ");
        s_errorThisp = এতত্;
        V3Error::errorExitCb(preErrorDumpHandler);  // If get error, dump self
        readModNames();
    }
    ~LinkDotState() {
        V3Error::errorExitCb(nullptr);
        s_errorThisp = nullptr;
    }

    // ACCESSORS
    VSymGraph* symsp() { প্রত্যাগম &m_syms; }
    পূর্ণাঙ্ক step_সংখ্যা() ধ্রুব { প্রত্যাগম static_cast<পূর্ণাঙ্ক>(m_step); }
    তার্কিক forPrimary() ধ্রুব { প্রত্যাগম m_step == LDS_PRIMARY; }
    তার্কিক forParamed() ধ্রুব { প্রত্যাগম m_step == LDS_PARAMED; }
    তার্কিক forScopeCreation() ধ্রুব { প্রত্যাগম m_step == LDS_SCOPED; }

    // METHODS
    স্থির string গ্রন্থি_Text_Type(Ast_গ্রন্থি* nodep) {
        যদি (ধ্রুব AstVar* ধ্রুব varp = VN_CAST(nodep, Var)) {
            যদি (varp->isIO() || varp->isIfaceRef()) {
                প্রত্যাগম "port";
            } অন্যথা যদি (varp->isGParam()) {
                প্রত্যাগম "parameter";
            } অন্যথা যদি (varp->varType() == VVarType::LPARAM) {
                প্রত্যাগম "local parameter";
            } অন্যথা {
                প্রত্যাগম "variable";
            }
        } অন্যথা যদি (ধ্রুব AstParamTypeDType* ধ্রুব dtypep = VN_CAST(nodep, ParamTypeDType)) {
            যদি (dtypep->isGParam()) {
                প্রত্যাগম "type parameter";
            } অন্যথা {
                প্রত্যাগম "local type parameter";
            }
        } অন্যথা যদি (VN_IS(nodep, Begin)) {
            প্রত্যাগম "block";
        } অন্যথা যদি (VN_IS(nodep, Cell)) {
            প্রত্যাগম "instance";
        } অন্যথা যদি (VN_IS(nodep, Constraint)) {
            প্রত্যাগম "constraint";
        } অন্যথা যদি (VN_IS(nodep, Func)) {
            প্রত্যাগম "function";
        } অন্যথা যদি (VN_IS(nodep, GenBlock)) {
            প্রত্যাগম "generate block";
        } অন্যথা যদি (VN_IS(nodep, Iface)) {
            প্রত্যাগম "interface";
        } অন্যথা যদি (VN_IS(nodep, RSProd)) {
            প্রত্যাগম "randsequence production";
        } অন্যথা যদি (VN_IS(nodep, Task)) {
            প্রত্যাগম "task";
        } অন্যথা {
            প্রত্যাগম nodep->prettyTypeName();
        }
    }

    Ast_গ্রন্থি_Module* findModuleSym(ধ্রুব string& modName) {
        ধ্রুব VSymEnt* ধ্রুব foundp = m_mods.rootp()->findIdFallback(modName);
        প্রত্যাগম foundp ? VN_AS(foundp->nodep(), গ্রন্থি_Module) : nullptr;
    }
    শূন্য readModNames() {
        // Look at all modules, and store pointers to all module names
        প্রতি (Ast_গ্রন্থি_Module *nextp, *nodep = v3Global.rootp()->modulesp(); nodep; nodep = nextp) {
            nextp = VN_AS(nodep->nextp(), গ্রন্থি_Module);
            m_mods.rootp()->ঢোকাও(nodep->name(), নূতন VSymEnt{&m_mods, nodep});
        }
    }

    VSymEnt* মূল_Entp() ধ্রুব { প্রত্যাগম m_syms.rootp(); }
    VSymEnt* dunitEntp() ধ্রুব { প্রত্যাগম m_dunitEntp; }
    শূন্য checkDuplicate(VSymEnt* lookupSymp, Ast_গ্রন্থি* nodep, ধ্রুব string& name) {
        // Lookup the given name under current symbol table
        // Insert if not found
        // Report error if there's a duplicate
        //
        // Note we only check for conflicts at the same level; it's ok if one block hides another
        // We also wouldn't want to not insert it even though it's lower down

        ধ্রুব VSymEnt* ধ্রুব foundp = lookupSymp->findIdFlat(name);
        Ast_গ্রন্থি* ধ্রুব fnodep = foundp ? foundp->nodep() : nullptr;
        যদি (!fnodep) {
            // Not found, will add in a moment.
            যদি (forPrimary() && VSymEnt::checkSimilarname(nodep)) {
                ধ্রুব VSymEnt* ধ্রুব alt = lookupSymp->findSimilarIdFlat(name);
                যদি (alt) {
                    nodep->v3warn(SIMILARNAME, "Declaration overlaps another with different case: "
                                                   << nodep->prettyNameQ() << '\n'
                                                   << nodep->warnContextPrimary() << '\n'
                                                   << alt->nodep()->warnOther()
                                                   << "... Location of original declaration\n"
                                                   << alt->nodep()->warnContextSecondary());
                }
            }
        } অন্যথা যদি (nodep == fnodep) {  // Already inserted.
            // Good.
        } অন্যথা যদি (foundp->imported()) {  // From package
            // We don't throw VARHIDDEN as if the import is later the symbol
            // table's import wouldn't warn
        } অন্যথা যদি (forPrimary() && VN_IS(nodep, GenBlock)
                   && (VN_IS(fnodep, Begin) || VN_IS(fnodep, GenBlock))) {
            // Begin: ... blocks often replicate under genif/genfor, so
            // suppress duplicate checks.  See t_gen_forif.v for an example.
        } অন্যথা {
            UINFO(4, "name " << name);  // Not always same as nodep->name
            UINFO(4, "Var1 " << nodep);
            UINFO(4, "Var2 " << fnodep);
            // Quote the node's own name as for messages, if it is the symbol's
            ধ্রুব string nameQ
                = fnodep->name() == name ? fnodep->prettyNameQ() : Ast_গ্রন্থি::prettyNameQ(name);
            যদি (nodep->type() == fnodep->type()) {
                nodep->v3error("Duplicate declaration of "
                               << গ্রন্থি_Text_Type(fnodep) << ": " << nameQ << '\n'
                               << nodep->warnContextPrimary() << '\n'
                               << fnodep->warnOther() << "... Location of original declaration\n"
                               << fnodep->warnContextSecondary());
                nodep->decl_Token_অঙ্ক_Set_Min(0);  // Disable checking forward typedefs
                fnodep->decl_Token_অঙ্ক_Set_Min(0);  // Disable checking forward typedefs
            } অন্যথা {
                nodep->v3error("Unsupported in C: "
                               << ucfirst(গ্রন্থি_Text_Type(nodep)) << " has the same name as "
                               << গ্রন্থি_Text_Type(fnodep) << ": " << nameQ << '\n'
                               << nodep->warnContextPrimary() << '\n'
                               << fnodep->warnOther() << "... Location of original declaration\n"
                               << fnodep->warnContextSecondary());
            }
        }
    }
    শূন্য ঢোকাও_DUnit(AstNetlist* nodep) {
        // $unit on top scope
        VSymEnt* ধ্রুব symp = নূতন VSymEnt{&m_syms, nodep};
        UINFO(9, "      INSERTdunit se" << cvtToHex(symp));
        symp->parentp(মূল_Entp());  // Needed so backward search can find name of top module
        symp->fallbackp(nullptr);
        মূল_Entp()->ঢোকাও("$unit ", symp);  // Space so can never name conflict with user code
        //
        UASSERT_OBJ(!m_dunitEntp, nodep, "Call insertDUnit only once");
        m_dunitEntp = symp;
    }
    VSymEnt* ঢোকাও_শীর্ষ_Cell(Ast_গ্রন্থি_Module* nodep, ধ্রুব string& scopename) {
        // Only called on the module at the very top of the hierarchy
        VSymEnt* ধ্রুব symp = নূতন VSymEnt{&m_syms, nodep};
        UINFO(9, "      INSERTtop se" << cvtToHex(symp) << "  " << scopename << " " << nodep);
        symp->parentp(মূল_Entp());  // Needed so backward search can find name of top module
        symp->fallbackp(dunitEntp());  // Needed so can find $unit stuff
        nodep->user1p(symp);
        checkDuplicate(মূল_Entp(), nodep, nodep->origName());
        মূল_Entp()->ঢোকাও(nodep->origName(), symp);
        যদি (forScopeCreation()) m_nameScopeSymMap.emplace(scopename, symp);
        প্রত্যাগম symp;
    }
    VSymEnt* ঢোকাও_শীর্ষ_Iface(AstCell* nodep, ধ্রুব string& scopename) {
        VSymEnt* ধ্রুব symp = নূতন VSymEnt{&m_syms, nodep};
        UINFO(9, "      INSERTtopiface se" << cvtToHex(symp) << "  " << scopename << " " << nodep);
        symp->parentp(মূল_Entp());  // Needed so backward search can find name of top module
        symp->fallbackp(dunitEntp());  // Needed so can find $unit stuff
        nodep->user1p(symp);
        যদি (nodep->modp()) nodep->modp()->user1p(symp);
        checkDuplicate(মূল_Entp(), nodep, nodep->origName());
        মূল_Entp()->ঢোকাও(nodep->origName(), symp);
        যদি (forScopeCreation()) m_nameScopeSymMap.emplace(scopename, symp);
        প্রত্যাগম symp;
    }
    VSymEnt* ঢোকাও_Cell(VSymEnt* abovep, VSymEnt* modSymp, AstCell* nodep,
                        ধ্রুব string& scopename) {
        UASSERT_OBJ(abovep, nodep, "Null symbol table inserting node");
        VSymEnt* ধ্রুব symp = নূতন VSymEnt{&m_syms, nodep};
        UINFO(9, "      INSERTcel se" << cvtToHex(symp) << "  " << scopename << " above=se"
                                      << cvtToHex(abovep) << " mods=se" << cvtToHex(modSymp)
                                      << " node=" << nodep);
        symp->parentp(abovep);
        symp->fallbackp(dunitEntp());  // Needed so can find $unit stuff
        nodep->user1p(symp);
        যদি (nodep->modp()) nodep->modp()->user1p(symp);
        checkDuplicate(abovep, nodep, nodep->origName());
        abovep->reinsert(nodep->origName(), symp);
        যদি (forScopeCreation() && abovep != modSymp && !modSymp->findIdFlat(nodep->name())) {
            // If it's foo_DOT_bar, we need to be able to find it under "foo_DOT_bar" too.
            // Duplicates are possible, as until resolve generates might
            // have 2 same cells under an if
            modSymp->reinsert(nodep->name(), symp);
        }
        যদি (forScopeCreation()) m_nameScopeSymMap.emplace(scopename, symp);
        প্রত্যাগম symp;
    }
    শূন্য ঢোকাও_Map(VSymEnt* symp, ধ্রুব string& scopename) {
        যদি (forScopeCreation()) m_nameScopeSymMap.emplace(scopename, symp);
    }

    VSymEnt* ঢোকাও_Inline(VSymEnt* abovep, VSymEnt* modSymp, AstCellInline* nodep,
                          ধ্রুব string& basename) {
        // A fake point in the hierarchy, corresponding to a named block flattened by V3Begin.
        // Names are looked up in the module with the block's flattened prefix.
        UASSERT_OBJ(abovep, nodep, "Null symbol table inserting node");
        VSymEnt* ধ্রুব symp = নূতন VSymEnt{&m_syms, nodep};
        UINFO(9, "      INSERTinl se" << cvtToHex(symp) << "  " << basename << " above=se"
                                      << cvtToHex(abovep) << " mods=se" << cvtToHex(modSymp)
                                      << " node=" << nodep);
        symp->parentp(abovep);
        symp->fallbackp(modSymp);
        symp->symPrefix(nodep->name() + "__DOT__");
        nodep->user1p(symp);
        checkDuplicate(abovep, nodep, nodep->name());
        abovep->reinsert(basename, symp);
        যদি (abovep != modSymp && !modSymp->findIdFlat(nodep->name())) {
            // If it's foo_DOT_bar, we need to be able to find it under that too.
            modSymp->reinsert(nodep->name(), symp);
        }
        প্রত্যাগম symp;
    }
    VSymEnt* ঢোকাও_Block(VSymEnt* abovep, ধ্রুব string& name, Ast_গ্রন্থি* nodep,
                         Ast_গ্রন্থি_Module* classOrPackagep) {
        // A fake point in the hierarchy, corresponding to a begin or function/task block
        // After we remove begins these will go away
        // Note we fallback to the symbol table of the parent, as we want to find variables there
        // However, cells walk the graph, so cells will appear under the begin/ftask itself
        UASSERT_OBJ(abovep, nodep, "Null symbol table inserting node");
        VSymEnt* ধ্রুব symp = নূতন VSymEnt{&m_syms, nodep};
        UINFO(9, "      INSERTblk se" << cvtToHex(symp) << "  above=se" << cvtToHex(abovep)
                                      << " pkg=" << cvtToHex(classOrPackagep)
                                      << "  node=" << nodep);
        symp->parentp(abovep);
        symp->classOrPackagep(classOrPackagep);
        symp->fallbackp(abovep);
        nodep->user1p(symp);
        যদি (name != "") checkDuplicate(abovep, nodep, name);
        // Duplicates are possible, as until resolve generates might have 2 same cells under an if
        abovep->reinsert(name, symp);
        প্রত্যাগম symp;
    }
    VSymEnt* ঢোকাও_Sym(VSymEnt* abovep, ধ্রুব string& name, Ast_গ্রন্থি* nodep,
                       Ast_গ্রন্থি_Module* classOrPackagep) {
        UASSERT_OBJ(abovep, nodep, "Null symbol table inserting node");
        VSymEnt* ধ্রুব symp = নূতন VSymEnt{&m_syms, nodep};
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
        প্রত্যাগম symp;
    }
    স্থির তার্কিক exists_গ্রন্থি_Sym(Ast_গ্রন্থি* nodep) { প্রত্যাগম nodep->user1p() != nullptr; }
    স্থির VSymEnt* get_গ্রন্থি_Sym(Ast_গ্রন্থি* nodep) {
        // Don't use this in ResolveVisitor, as we need to pick up the proper
        // reference under each SCOPE
        VSymEnt* ধ্রুব symp = nodep->user1u().toSymEnt();
        UASSERT_OBJ(symp, nodep, "Module/etc never assigned a symbol entry?");
        প্রত্যাগম symp;
    }
    VSymEnt* getScopeSym(AstScope* nodep) {
        ধ্রুব auto it = m_nameScopeSymMap.find(nodep->name());
        UASSERT_OBJ(it != m_nameScopeSymMap.end(), nodep,
                    "Scope never assigned a symbol entry '" << nodep->name() << "'");
        প্রত্যাগম it->second;
    }
    শূন্য implicitOkAdd(Ast_গ্রন্থি_Module* nodep, ধ্রুব string& varname) {
        // Mark the given variable name as being allowed to be implicitly declared
        যদি (nodep) {
            ধ্রুব auto it = m_implicitNameSet.find(std::make_pair(nodep, varname));
            যদি (it == m_implicitNameSet.end()) m_implicitNameSet.emplace(nodep, varname);
        }
    }
    তার্কিক implicitOk(Ast_গ্রন্থি_Module* nodep, ধ্রুব string& varname) {
        প্রত্যাগম nodep
               && (m_implicitNameSet.find(std::make_pair(nodep, varname))
                   != m_implicitNameSet.end());
    }

    // Track and later recurse interface modules
    শূন্য ঢোকাও_Iface_Mod_Sym(AstIface* nodep, VSymEnt* symp) {
        m_ifaceModSyms.emplace_back(nodep, symp);
    }
    শূন্য computeIfaceModSyms();

    // Track and later insert interface references
    শূন্য ঢোকাও_Iface_Var_Sym(VSymEnt* symp) {  // Where sym is for a VAR of dtype IFACEREFDTYPE
        m_ifaceVarSyms.ঠেলো_back(symp);
    }
    // Iface for a raw or arrayed iface; peels nested array layers for multi-dim arrays.
    স্থির AstIfaceRefDType* ifaceRefFromArray(Ast_গ্রন্থি_DType* nodep) {
        যাবত্ (nodep) {
            যদি (AstIfaceRefDType* ধ্রুব ifp = VN_CAST(nodep, IfaceRefDType)) প্রত্যাগম ifp;
            যদি (ধ্রুব AstBracketArrayDType* ধ্রুব arrp = VN_CAST(nodep, BracketArrayDType)) {
                nodep = arrp->subDTypep();
            } অন্যথা যদি (ধ্রুব AstUnpackArrayDType* ধ্রুব arrp = VN_CAST(nodep, UnpackArrayDType)) {
                nodep = arrp->subDTypep();
            } অন্যথা {
                প্রত্যাগম nullptr;
            }
        }
        প্রত্যাগম nullptr;
    }
    // Given a pin expression, resolve it to a live AstIface* (or nullptr).
    // Handles both simple VarRef and dotted VarXRef pin connections.
    AstIface* live_Iface_From_Pin_রাশিমালা(Ast_গ্রন্থি* exprp, VSymEnt* parentSymp) {
        AstVar* resolvedVarp = nullptr;
        যদি (AstVarRef* ধ্রুব refp = VN_CAST(exprp, VarRef)) {
            resolvedVarp = refp->varp();
        } অন্যথা যদি (AstVarXRef* ধ্রুব xrefp = VN_CAST(exprp, VarXRef)) {
            VSymEnt* ধ্রুব lookupSymp = parentSymp ? parentSymp->parentp() : nullptr;
            যদি (lookupSymp) {
                string baddot;
                VSymEnt* okSymp = nullptr;
                ধ্রুব string dotpath = xrefp->dotted() + "." + xrefp->name();
                VSymEnt* ধ্রুব dotSymp
                    = findDotted(xrefp->fileline(), lookupSymp, dotpath, baddot, okSymp, সত্য);
                যদি (dotSymp) resolvedVarp = VN_CAST(dotSymp->nodep(), Var);
            }
        }
        AstIface* resultp = nullptr;
        যদি (resolvedVarp) {
            AstIfaceRefDType* ধ্রুব irefp = ifaceRefFromArray(resolvedVarp->subDTypep());
            যদি (irefp && irefp->ifaceViaCellp() && !irefp->ifaceViaCellp()->dead()) {
                resultp = irefp->ifaceViaCellp();
            }
        }
        প্রত্যাগম resultp;
    }

    // Attempt to repair a port's AstIfaceRefDType by tracing through the
    // parent cell's pin connection to find the correct live interface.
    // Returns true if repaired or already correct; false if unresolvable.
    তার্কিক repairIfaceRef(VSymEnt* varSymp, AstVar* varp, AstIfaceRefDType* ifacerefp, তার্কিক isDead) {
        // Walk up symbol-table parents to find the enclosing AstCell
        VSymEnt* parentSymp = varSymp->parentp();
        AstCell* parentCellp = nullptr;
        প্রতি (পূর্ণাঙ্ক depth = 0; parentSymp && depth < 20; ++depth) {
            যদি (AstCell* ধ্রুব cp = VN_CAST(parentSymp->nodep(), Cell)) {
                parentCellp = cp;
                ভঙ্গ;
            }
            parentSymp = parentSymp->parentp();
        }
        যদি (!parentCellp) প্রত্যাগম অসত্য;
        // Scan pins to find the one connected to this port variable
        প্রতি (AstPin* pinp = parentCellp->pinsp(); pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            যদি (pinp->modVarp() != varp && pinp->name() != varp->name()) অনুবর্ত;
            Ast_গ্রন্থি* ধ্রুব exprp = pinp->exprp();
            যদি (!exprp) প্রত্যাগম অসত্য;
            AstIface* ধ্রুব newIfacep = live_Iface_From_Pin_রাশিমালা(exprp, parentSymp);
            যদি (newIfacep && newIfacep != ifacerefp->ifaceViaCellp()) {
                UINFO(4, "  REPAIR-IFACE-REF var=" << varp->prettyNameQ()
                                                   << " old=" << ifacerefp->ifacep()->prettyNameQ()
                                                   << " new=" << newIfacep->prettyNameQ());
                ifacerefp->ifacep(newIfacep);
                প্রত্যাগম সত্য;
            }
            যদি (newIfacep) প্রত্যাগম !isDead;  // Same interface - OK if live
            প্রত্যাগম অসত্য;
        }
        প্রত্যাগম অসত্য;
    }

    শূন্য computeIfaceVarSyms() {
        প্রতি (VSymEnt* varSymp : m_ifaceVarSyms) {
            AstVar* ধ্রুব varp = varSymp ? VN_AS(varSymp->nodep(), Var) : nullptr;
            UINFO(9, "  insAllIface se" << cvtToHex(varSymp) << " " << varp);
            AstIfaceRefDType* ধ্রুব ifacerefp = ifaceRefFromArray(varp->subDTypep());
            UASSERT_OBJ(ifacerefp, varp, "Non-ifacerefs on list!");
            যদি (!ifacerefp->cellp() && !ifacerefp->isVirtual() && (varp->pin_অঙ্ক() == 0)) {
                varp->v3error("Interface port declaration "
                              << varp->prettyNameQ() << " doesn't have corresponding port\n"
                              << varp->warnMore()
                                     + "... Perhaps intended an interface instantiation but "
                                       "are missing parenthesis (IEEE 1800-2023 25.3)?");
            }
            যদি (!ifacerefp->ifaceViaCellp()) {
                যদি (!ifacerefp->cellp()) {  // Probably a NotFoundModule, or a normal module if
                                            // made mistake
                    UINFO(1, "Associated cell " << Ast_গ্রন্থি::prettyNameQ(ifacerefp->cellName()));
                    ifacerefp->v3error("Cannot find file containing interface: "
                                       << Ast_গ্রন্থি::prettyNameQ(ifacerefp->ifaceName()));
                    অনুবর্ত;
                } অন্যথা {
                    ifacerefp->v3fatalSrc("Unlinked interface");
                }
            } অন্যথা যদি (varp->isIfaceRef() && !ifacerefp->cellp() && ifacerefp->ifaceViaCellp()) {
                // Port variable (cellp=null) pointing to an interface.
                // The interface may be dead (template replaced by clone) or
                // live-but-wrong (template still alive because some instances
                // use default params, but this port should point to a
                // specialized clone based on the pin connection).
                ধ্রুব তার্কিক isDead = ifacerefp->ifaceViaCellp()->dead();
                যদি (isDead && forPrimary() && !varp->isIfaceParent()
                    && !v3Global.opt.শীর্ষ_Ifaces_Supported()) {
                    varp->v3warn(E_UNSUPPORTED,
                                 "Unsupported: Interfaced port on top level module");
                }
                // Attempt repair: trace through parent cell's pin to find
                // the correct live interface for this port variable
                ধ্রুব তার্কিক repaired = repairIfaceRef(varSymp, varp, ifacerefp, isDead);
                যদি (!repaired && isDead) {
                    ifacerefp->v3error(
                        "Interface " << Ast_গ্রন্থি::prettyNameQ(ifacerefp->ifaceName())
                                     << " not connected as parent's interface not connected\n"
                                     << ifacerefp->warnMore()
                                     << "... Perhaps caused by another error on the parent "
                                        "interface that needs resolving\n"
                                     << ifacerefp->warnMore()
                                     << "... Or, perhaps intended an interface instantiation but "
                                        "are missing parenthesis (IEEE 1800-2023 25.3)?");
                    অনুবর্ত;
                }
            } অন্যথা যদি (ifacerefp->ifaceViaCellp()->dead()
                       || !exists_গ্রন্থি_Sym(ifacerefp->ifaceViaCellp())) {
                ifacerefp->ifaceViaCellp()->v3fatalSrc(
                    "Interface referenced by virtual iface is dead. "
                    "Dead interfaces have required linking steps skipped");
            }
            VSymEnt* ধ্রুব ifaceSymp = get_গ্রন্থি_Sym(ifacerefp->ifaceViaCellp());
            VSymEnt* ifOrPortSymp = ifaceSymp;
            // Link Modport names to the Modport Node under the Interface
            যদি (ifacerefp->isModport()) {
                VSymEnt* ধ্রুব foundp = ifaceSymp->findIdFallback(ifacerefp->modportName());
                তার্কিক ok = অসত্য;
                যদি (foundp) {
                    যদি (AstModport* ধ্রুব modportp = VN_CAST(foundp->nodep(), Modport)) {
                        UINFO(4, "Link Modport: " << modportp);
                        ifacerefp->modportp(modportp);
                        ifOrPortSymp = foundp;
                        ok = সত্য;
                    }
                }
                যদি (!ok) {
                    ধ্রুব string suggest = suggestSymFallback(ifaceSymp, ifacerefp->modportName(),
                                                              Link_গ্রন্থি_Matcher_Modport{});
                    ifacerefp->modportFileline()->v3error(
                        "Modport not found under interface "
                        << ifacerefp->prettyNameQ(ifacerefp->ifaceName()) << ": "
                        << ifacerefp->prettyNameQ(ifacerefp->modportName()) << '\n'
                        << (suggest.empty() ? "" : ifacerefp->warnMore() + suggest));
                }
            }
            // Alias won't expand until interfaces and modport names are known; see notes at top
            ঢোকাও_Scope_Alias(SAMN_IFTOP, varSymp, ifOrPortSymp);
        }
        m_ifaceVarSyms.clear();
    }

    শূন্য ঢোকাও_Scope_Alias(SAMNum samn, VSymEnt* lhsp, VSymEnt* rhsp) {
        // Track and later insert scope aliases; an interface referenced by
        // a child cell connecting to that interface
        // Typically lhsp=VAR w/dtype IFACEREF, rhsp=IFACE cell
        UINFO(9, "   insertScopeAlias se" << cvtToHex(lhsp) << " se" << cvtToHex(rhsp));
        UASSERT_OBJ(
            !(VN_IS(rhsp->nodep(), Cell) && !VN_IS(VN_AS(rhsp->nodep(), Cell)->modp(), Iface)),
            rhsp->nodep(), "Got a non-IFACE alias RHS");
        m_scopeAliasMap[samn].emplace(lhsp, rhsp);
    }
    শূন্য computeScopeAliases() {
        UINFO(9, "computeIfaceAliases");
        প্রতি (পূর্ণাঙ্ক samn = 0; samn < SAMN__MAX; ++samn) {
            প্রতি (ScopeAliasMap::iterator it = m_scopeAliasMap[samn].begin();
                 it != m_scopeAliasMap[samn].end(); ++it) {
                VSymEnt* ধ্রুব lhsp = it->first;
                VSymEnt* srcp = lhsp;
                যাবত্ (সত্য) {  // Follow chain of aliases up to highest level non-alias
                    ধ্রুব auto it2 = m_scopeAliasMap[samn].find(srcp);
                    যদি (it2 != m_scopeAliasMap[samn].end()) {
                        srcp = it2->second;
                        অনুবর্ত;
                    } অন্যথা {
                        ভঙ্গ;
                    }
                }
                UINFO(9, "  iiasa: Insert alias se" << lhsp << " (" << lhsp->nodep()->typeName()
                                                    << ") <- se" << srcp << " " << srcp->nodep());
                // srcp should be an interface reference pointing to the interface we want to
                // import
                lhsp->importFromIface(symsp(), srcp);
                // Allow access to objects not permissible to be listed in a modport
                যদি (VN_IS(srcp->nodep(), Modport)) {
                    lhsp->importFromIface(symsp(), srcp->parentp(), সত্য);
                }
            }
            // m_scopeAliasMap[samn].clear();  // Done with it, but put into debug file
        }
    }

গুপ্ত:
    VSymEnt* findWithAltFallback(VSymEnt* symp, ধ্রুব string& name, ধ্রুব string& altname) {
        VSymEnt* findp = symp->findIdFallback(name);
        যদি (findp) প্রত্যাগম findp;
        যদি (altname != "") {
            UINFO(8, "     alt fallback");
            findp = symp->findIdFallback(altname);
        }
        প্রত্যাগম findp;
    }

    VSymEnt* findWithAltFlat(VSymEnt* symp, ধ্রুব string& name, ধ্রুব string& altname) {
        VSymEnt* findp = symp->findIdFlat(name);
        যদি (findp) প্রত্যাগম findp;
        যদি (altname != "") {
            UINFO(8, "     alt flat");
            findp = symp->findIdFlat(altname);
        }
        প্রত্যাগম findp;
    }

সার্বজনিক:
    VSymEnt* findDotted(File_সারি* refLocationp, VSymEnt* lookupSymp, ধ্রুব string& dotname,
                        string& baddot, VSymEnt*& okSymp, তার্কিক firstId) {
        // Given a dotted hierarchy name, return where in scope it is
        // Note when dotname=="" we just fall through and return lookupSymp
        UINFO(8, "    dottedFind se" << cvtToHex(lookupSymp) << " '" << dotname << "'");
        string leftname = dotname;
        okSymp = lookupSymp;  // So can list bad scopes
        যাবত্ (leftname != "") {  // foreach dotted part of xref name
            string::size_type pos = leftname.find('.');
            string ident;
            যদি (pos != string::npos) {
                ident = leftname.substr(0, pos);
                leftname = leftname.substr(pos + 1);
            } অন্যথা {
                ident = leftname;
                leftname = "";
            }
            baddot = ident;  // So user can see where they botched it
            okSymp = lookupSymp;
            string altIdent;
            যদি (!forScopeCreation()) {
                // GENFOR Begin is foo__BRA__##__KET__ after we've genloop unrolled,
                // but presently should be just "foo".
                // Likewise cell foo__[array] before we've expanded arrays is just foo.
                // Multi-dim iface arrays append multiple __BRA__..__KET__ suffixes; strip them
                // all.
                altIdent = ident;
                যাবত্ (VString::endsWith(altIdent, "__KET__")) {
                    ধ্রুব auto braPos = altIdent.rfind("__BRA__");
                    যদি (braPos == string::npos) ভঙ্গ;
                    altIdent = altIdent.substr(0, braPos);
                }
                যদি (altIdent == ident) altIdent.clear();
            }
            UINFO(8, "         id " << ident << " alt " << altIdent << " left " << leftname
                                    << " at se" << lookupSymp);
            // Spec says; Look at existing module (cellnames then modname),
            // then look up (inst name or modname)
            যদি (firstId) {
                // Check this module - subcellnames
                ধ্রুব AstCell* cellp = lookupSymp ? VN_CAST(lookupSymp->nodep(), Cell)
                                                  : nullptr;  // Replicated below
                যদি (VSymEnt* ধ্রুব findSymp = findWithAltFallback(lookupSymp, ident, altIdent)) {
                    lookupSymp = findSymp;
                }

                // Check this module - cur modname
                অন্যথা যদি (cellp && cellp->modp()->origName() == ident) {
                }
                // $root we walk up to Netlist
                অন্যথা যদি (ident == "$root") {
                    lookupSymp = মূল_Entp();
                    // We've added the '$root' module, now everything else is one lower
                    যদি (forScopeCreation()) {
                        lookupSymp = lookupSymp->findIdFlat(ident);
                        UASSERT(lookupSymp, "Cannot find $root module under netlist");
                    }
                }
                // Move up and check cellname + modname
                অন্যথা {
                    তার্কিক crossedCell = অসত্য;  // Crossed a cell boundary
                    যাবত্ (lookupSymp) {
                        lookupSymp = lookupSymp->parentp();
                        cellp = lookupSymp ? VN_CAST(lookupSymp->nodep(), Cell)
                                           : nullptr;  // Replicated above
                        যদি (lookupSymp) {
                            UINFO(9, "    Up to " << lookupSymp);
                            যদি (cellp || VN_IS(lookupSymp->nodep(), CellInline)) {
                                crossedCell = সত্য;
                            }
                            যদি (cellp && cellp->modp()->origName() == ident) {
                                ভঙ্গ;
                            } অন্যথা যদি (VSymEnt* ধ্রুব findSym2p
                                       = findWithAltFallback(lookupSymp, ident, altIdent)) {
                                lookupSymp = findSym2p;
                                যদি (crossedCell && VN_IS(lookupSymp->nodep(), Var)) {
                                    UINFO(9, "    Not found but matches var name in parent "
                                                 << lookupSymp);
                                    প্রত্যাগম nullptr;
                                }
                                ভঙ্গ;
                            }
                        } অন্যথা {
                            ভঙ্গ;
                        }
                    }
                    যদি (!lookupSymp) প্রত্যাগম nullptr;  // Not found
                }
            } অন্যথা {  // Searching for middle path component, must be a cell or interface port
                VSymEnt* findSymp = findWithAltFlat(lookupSymp, ident, altIdent);
                যদি (findSymp) {
                    lookupSymp = findSymp;
                } অন্যথা {
                    // Try prefixed lookup for interface ports accessed through hierarchy
                    // (e.g., slave_inst.bus.data where bus is an interface port)
                    // After inlining, interface ports have prefixed names like
                    // top__DOT__slave_inst__DOT__bus
                    UINFO(8, "     middle-path: trying prefixed lookup for '" << ident << "'\n");
                    string baddot;
                    findSymp = findSymPrefixed(lookupSymp, ident, baddot, /*fallback=*/সত্য);
                    UINFO(8, "     middle-path: prefixed lookup result: "
                                 << (findSymp ? "found" : "not found") << "\n");
                    যদি (findSymp) {
                        // Check if this is an interface reference variable
                        ধ্রুব AstVar* varp = VN_CAST(findSymp->nodep(), Var);
                        যদি (!varp) {
                            যদি (ধ্রুব AstVarScope* vscp = VN_CAST(findSymp->nodep(), VarScope)) {
                                varp = vscp->varp();
                            }
                        }
                        যদি (varp && varp->isIfaceRef()) {
                            // Found an interface port - use findSymp directly
                            // computeScopeAliases has already imported interface members
                            // into this symbol entry, so we use it instead of redirecting
                            // to the shared modport definition (which would cause all
                            // instances to resolve to the same symbols - bug #2656)
                            lookupSymp = findSymp;
                        } অন্যথা {
                            // Non-interface symbol in middle path must be a cell (module instance)
                            // to continue hierarchical resolution
                            যদি (VN_IS(findSymp->nodep(), Cell)) {
                                lookupSymp = findSymp;
                            } অন্যথা {
                                // Reject non-cell, non-interface symbols found via fallback
                                // to prevent accidental resolution to unrelated symbols
                                UINFO(8, "     middle-path: rejecting non-cell symbol\n");
                                প্রত্যাগম nullptr;
                            }
                        }
                    } অন্যথা {
                        প্রত্যাগম nullptr;  // Not found
                    }
                }
            }
            যদি (lookupSymp) {
                যদি (ধ্রুব AstCell* ধ্রুব cellp = VN_CAST(lookupSymp->nodep(), Cell)) {
                    যদি (ধ্রুব Ast_গ্রন্থি_Module* ধ্রুব modp = cellp->modp()) {
                        যদি (modp->hierBlock() && !leftname.empty()) {
                            refLocationp->v3error("Cannot access scope inside hierarchical block");
                        } অন্যথা যদি (VN_IS(modp, NotFoundModule)) {
                            refLocationp->v3error("Dotted reference to instance that refers to "
                                                  "missing module/interface: "
                                                  << modp->prettyNameQ());
                        }
                    }
                }
                // Follow scope alias for nested interface port access
                যদি (!leftname.empty()) {
                    ধ্রুব auto aliasIt = m_scopeAliasMap[SAMN_IFTOP].find(lookupSymp);
                    যদি (aliasIt != m_scopeAliasMap[SAMN_IFTOP].end()) {
                        lookupSymp = aliasIt->second;
                        // Alias may point to __Viftop VarScope; find corresponding Cell
                        যদি (ধ্রুব AstVarScope* ধ্রুব vscp
                            = VN_CAST(lookupSymp->nodep(), VarScope)) {
                            ধ্রুব string varName = vscp->varp()->name();
                            স্থির constexpr ধ্রুব বর্ণ* ধ্রুব VIFTOP_SUFFIX = "__Viftop";
                            যদি (VString::endsWith(varName, VIFTOP_SUFFIX)) {
                                ধ্রুব string cellName
                                    = varName.substr(0, varName.size() - strlen(VIFTOP_SUFFIX));
                                VSymEnt* ধ্রুব parentSymp = lookupSymp->parentp();
                                যদি (parentSymp) {
                                    VSymEnt* ধ্রুব cellSymp = parentSymp->findIdFlat(cellName);
                                    যদি (cellSymp && VN_IS(cellSymp->nodep(), Cell)) {
                                        lookupSymp = cellSymp;  // Use Cell for member lookup
                                    }
                                }
                            }
                        }
                    } অন্যথা {
                        // No alias; try following IfaceRefDType to interface cell
                        ধ্রুব AstVar* varp = VN_CAST(lookupSymp->nodep(), Var);
                        যদি (!varp) {
                            যদি (ধ্রুব AstVarScope* ধ্রুব vscp
                                = VN_CAST(lookupSymp->nodep(), VarScope)) {
                                varp = vscp->varp();
                            }
                        }
                        যদি (varp && varp->isIfaceRef()) {
                            যদি (ধ্রুব AstIfaceRefDType* ধ্রুব ifaceRefp
                                = ifaceRefFromArray(varp->dtypep())) {
                                যদি (ifaceRefp->cellp() && exists_গ্রন্থি_Sym(ifaceRefp->cellp())) {
                                    lookupSymp = get_গ্রন্থি_Sym(ifaceRefp->cellp());
                                } অন্যথা যদি (ifaceRefp->ifaceViaCellp()
                                           && exists_গ্রন্থি_Sym(ifaceRefp->ifaceViaCellp())) {
                                    lookupSymp = get_গ্রন্থি_Sym(ifaceRefp->ifaceViaCellp());
                                } অন্যথা যদি (ifaceRefp->ifacep()
                                           && exists_গ্রন্থি_Sym(ifaceRefp->ifacep())) {
                                    lookupSymp = get_গ্রন্থি_Sym(ifaceRefp->ifacep());
                                }
                            }
                        }
                    }
                }
            }
            firstId = অসত্য;
        }
        প্রত্যাগম lookupSymp;
    }

    স্থির string removeLastInlineScope(ধ্রুব string& name) {
        string out = name;
        ধ্রুব string dot = "__DOT__";
        ধ্রুব size_t dotPos = out.rfind(dot, out.size() - dot.length() - 1);
        যদি (dotPos == string::npos) {
            প্রত্যাগম "";
        } অন্যথা {
            প্রত্যাগম out.erase(dotPos + dot.length(), string::npos);
        }
    }

    VSymEnt* findSymPrefixed(VSymEnt* lookupSymp, ধ্রুব string& dotname, string& baddot,
                             তার্কিক fallback) {
        // Find symbol in given point in hierarchy, allowing prefix (post-Inline)
        // For simplicity lookupSymp may be passed nullptr result from findDotted
        যদি (!lookupSymp) প্রত্যাগম nullptr;
        UINFO(8, "    findSymPrefixed "
                     << dotname << " under se" << cvtToHex(lookupSymp)
                     << ((lookupSymp->symPrefix() == "") ? "" : " as ")
                     << ((lookupSymp->symPrefix() == "") ? "" : lookupSymp->symPrefix() + dotname)
                     << "  at se" << lookupSymp << " fallback=" << fallback);
        string prefix = lookupSymp->symPrefix();
        VSymEnt* foundp = nullptr;
        যাবত্ (!foundp) {
            যদি (fallback) {
                foundp = lookupSymp->findIdFallback(prefix + dotname);  // Might be nullptr
            } অন্যথা {
                foundp = lookupSymp->findIdFlat(prefix + dotname);  // Might be nullptr
            }
            যদি (prefix.empty()) ভঙ্গ;
            ধ্রুব string nextPrefix = removeLastInlineScope(prefix);
            যদি (prefix == nextPrefix) ভঙ্গ;
            prefix = nextPrefix;
        }
        যদি (!foundp) baddot = dotname;
        প্রত্যাগম foundp;
    }
    স্থির তার্কিক checkIfClassOrPackage(ধ্রুব VSymEnt* ধ্রুব symp) {
        যদি (VN_IS(symp->nodep(), Class) || VN_IS(symp->nodep(), Package)) প্রত্যাগম সত্য;

        // Helper: check if a RefDType might resolve to a class later.
        ধ্রুব auto isPotentialClassRef = [](ধ্রুব AstRefDType* refp) -> তার্কিক {
            প্রত্যাগম refp && !refp->typeofp()
                   && (!refp->classOrPackageOpp()
                       || VN_IS(refp->classOrPackageOpp(), ClassOrPackageRef));
        };

        // Helper: allow types that can represent a class/package handle or an unresolved ref.
        // BasicDType is allowed here only for null/object handles.
        ধ্রুব auto is_Valid_Type_গ্রন্থি = [](ধ্রুব Ast_গ্রন্থি* nodep) -> তার্কিক {
            প্রত্যাগম VN_IS(nodep, VoidDType) || VN_IS(nodep, BasicDType)
                   || VN_IS(nodep, ClassRefDType) || VN_IS(nodep, ParseRef);
        };

        যদি (ধ্রুব AstTypedef* ধ্রুব typedefp = VN_CAST(symp->nodep(), Typedef)) {
            ধ্রুব Ast_গ্রন্থি_DType* dtypep = typedefp->subDTypep();
            যদি (!dtypep) dtypep = typedefp->childDTypep();
            যদি (VN_IS(dtypep, ClassRefDType)) প্রত্যাগম সত্য;
            যদি (isPotentialClassRef(VN_CAST(dtypep, RefDType))) প্রত্যাগম সত্য;
        } অন্যথা যদি (ধ্রুব AstParamTypeDType* ধ্রুব paramTypep
                   = VN_CAST(symp->nodep(), ParamTypeDType)) {
            // Before V3Param the declared default is in childDTypep (possibly
            // wrapped in a RequireDType); after V3Param it is consumed and the
            // bound type is the resolved data type, e.g. a type parameter
            // inherited from a specialized base class (REQ #(Item) -> class Item).
            Ast_গ্রন্থি* childp = paramTypep->childDTypep();
            যদি (ধ্রুব AstRequireDType* ধ্রুব reqp = VN_CAST(childp, RequireDType)) {
                childp = reqp->lhsp();
            }
            ধ্রুব Ast_গ্রন্থি* ধ্রুব checkp = childp ? childp : paramTypep->skipRefp();
            যদি (is_Valid_Type_গ্রন্থি(checkp) || isPotentialClassRef(VN_CAST(checkp, RefDType))) {
                প্রত্যাগম সত্য;
            }
        }
        প্রত্যাগম অসত্য;
    }
    VSymEnt* resolveClassOrPackage(VSymEnt* lookSymp, AstClassOrPackageRef* nodep, তার্কিক fallback,
                                   তার্কিক classOnly, ধ্রুব string& forWhat,
                                   তার্কিক deferIfUnresolved = অসত্য) {
        যদি (nodep->classOrPackageSkipp()) প্রত্যাগম get_গ্রন্থি_Sym(nodep->classOrPackageSkipp());
        VSymEnt* foundp;
        VSymEnt* অনুসন্ধান_Symp = lookSymp;
        যদি (VL_UNCOVERABLE(অনুসন্ধান_Symp && VN_IS(অনুসন্ধান_Symp->nodep(), ParamTypeDType))) {
            অনুসন্ধান_Symp->nodep()->v3fatalSrc(  // LCOV_EXCL_LINE
                "resolveClassOrPackage: unexpected ParamTypeDType lookup");
        }
        যদি (fallback) {
            VSymEnt* currentLookSymp = অনুসন্ধান_Symp;
            কুরু {
                foundp = currentLookSymp->findIdFlat(nodep->name());
                যদি (foundp && !checkIfClassOrPackage(foundp)) foundp = nullptr;
                যদি (!foundp) currentLookSymp = currentLookSymp->fallbackp();
            } যাবত্ (!foundp && currentLookSymp);
        } অন্যথা {
            foundp = অনুসন্ধান_Symp->findIdFlat(nodep->name());
            যদি (foundp && !checkIfClassOrPackage(foundp)) foundp = nullptr;
        }
        যদি (!foundp && v3Global.rootp()->stdPackagep()) {  // Look under implied std::
            foundp = get_গ্রন্থি_Sym(v3Global.rootp()->stdPackagep())->findIdFlat(nodep->name());
        }
        যদি (foundp) {
            nodep->classOrPackageNodep(foundp->nodep());
            প্রত্যাগম foundp;
        }
        যদি (deferIfUnresolved) প্রত্যাগম nullptr;
        ধ্রুব string suggest
            = suggestSymFallback(lookSymp, nodep->name(), Link_গ্রন্থি_Matcher_Class_Or_Package{});
        nodep->v3error((classOnly ? "Class" : "Package/class")
                       << " for '" << forWhat  // extends/implements
                       << "' not found: " << nodep->prettyNameQ() << '\n'
                       << (suggest.empty() ? "" : nodep->warnMore() + suggest));
        প্রত্যাগম nullptr;
    }
    string suggestSymFallback(VSymEnt* lookupSymp, ধ্রুব string& name,
                              ধ্রুব VNodeMatcher& matcher) {
        // Suggest alternative symbol in given point in hierarchy
        // Does not support inline, as we find user-level errors before inlining
        // For simplicity lookupSymp may be passed nullptr result from findDotted
        যদি (!lookupSymp) প্রত্যাগম "";
        VSpellCheck speller;
        lookupSymp->candidateIdFallback(&speller, &matcher);
        প্রত্যাগম speller.best_Candidate_বার্তা(name);
    }
    string suggestSymFlat(VSymEnt* lookupSymp, ধ্রুব string& name, ধ্রুব VNodeMatcher& matcher,
                          তার্কিক includeType) {
        যদি (!lookupSymp) প্রত্যাগম "";
        VSpellCheck speller;
        lookupSymp->candidateIdFlat(&speller, &matcher, includeType);
        প্রত্যাগম speller.best_Candidate_বার্তা(name);
    }
};

LinkDotState* LinkDotState::s_errorThisp = nullptr;

//======================================================================

বর্গ LinkDotFindVisitor final : সার্বজনিক VNVisitor {
    // NODE STATE
    // Cleared on global
    //  *::user1p()             -> See LinkDotState
    //  *::user2p()             -> See LinkDotState
    //  *::user3()              // bool. Node stored in m_virtualIfaces
    //  *::user4()              -> See LinkDotState
    ধ্রুব VNUser3InUse m_inuser3;

    // STATE
    LinkDotState* ধ্রুব m_statep;  // State to pass between visitors, including symbol table
    Ast_গ্রন্থি_Module* m_classOrPackagep = nullptr;  // Current package
    AstClocking* m_clockingp = nullptr;  // Current clocking block
    std::vector<AstScope*> m_scopes;  // Scopes detached during repeated module visits
    uint64_t m_statScopeVisits = 0;  // Non-top scopes encountered while finding instance symbols
    VSymEnt* m_modSymp = nullptr;  // Symbol Entry for current module
    VSymEnt* m_curSymp = nullptr;  // Symbol Entry for current table, where to lookup/insert
    string
        m_hierParamsName;  // Name of module with hierarchical type parameters, empty when not used
    string m_scope;  // Scope text
    ধ্রুব Ast_গ্রন্থি_FTask* m_ftaskp = nullptr;  // Current function/task
    তার্কিক m_inRecursion = অসত্য;  // Inside a recursive module
    পূর্ণাঙ্ক m_param_অঙ্ক = 0;  // Parameter number, for position based connection
    তার্কিক m_explicitNew = অসত্য;  // Hit a "new" function
    পূর্ণাঙ্ক m_mod_Block_অঙ্ক = 0;  // Begin block number in module, 0=none seen
    পূর্ণাঙ্ক m_mod_With_অঙ্ক = 0;  // With block number, 0=none seen
    পূর্ণাঙ্ক m_mod_Arg_অঙ্ক = 0;  // Arg block number for randomize(), 0=none seen
    std::vector<AstIface*> m_virtIfaces;  // interfaces used as virtual,
                                          // needed for handleUnvisitedVirtIfaces()

    // METHODS
    শূন্য makeImplicitNew(AstClass* nodep) {
        AstFunc* ধ্রুব newp = নূতন AstFunc{nodep->fileline(), "new", nullptr, nullptr};
        // If needed, super.new() call is added the 2nd pass of V3LinkDotResolve,
        // because base classes are already resolved there.
        newp->isConstructor(সত্য);
        nodep->addMembersp(newp);
        UINFO(8, "Made implicit new for " << nodep->name() << ": " << nodep);
        m_statep->ঢোকাও_Block(m_curSymp, newp->name(), newp, m_classOrPackagep);
    }

    স্থির তার্কিক isHierBlockWrapper(ধ্রুব string& name) {
        ধ্রুব V3HierBlockOptSet& hierBlocks = v3Global.opt.hierBlocks();
        প্রত্যাগম hierBlocks.find(name) != hierBlocks.end();
    }

    শূন্য moveExternFuncDecl(Ast_গ্রন্থি_FTask* nodep, AstClass* classp) {
        // Move an 'extern function|task' to inside a class or, from an outer to inner-class
        যদি (nodep->isExternDef()) প্রত্যাগম;
        // Move it to proper spot under the target class
        nodep->unlinkFrBack();
        classp->addStmtsp(nodep);
        nodep->isExternDef(সত্য);  // So we check there's a matching extern
        nodep->classOrPackagep()->unlinkFrBack()->delete_বৃক্ষ();
        // Any "Type::" reference in the function's IO are really "MovedToClass::" references
        যদি (nodep->fvarp()) moveExternFuncDeclRefs(nodep->fvarp(), classp);
        প্রতি (Ast_গ্রন্থি* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            যদি (AstVar* ধ্রুব portp = VN_CAST(stmtp, Var)) {
                যদি (portp->isIO()) moveExternFuncDeclRefs(portp, classp);
            }
        }
    }
    শূন্য moveExternFuncDeclRefs(Ast_গ্রন্থি* nodep, AstClass* classp) {
        nodep->foreach([এতত্, classp](AstClassOrPackageRef* refp) {  //
            যদি (refp->name() == classp->name() && !refp->paramsp()) {
                UINFO(9, "Cleaning up external function type for class " << refp);
                AstDot* ধ্রুব dotp = VN_CAST(refp->backp(), Dot);
                যদি (dotp && dotp->lhsp() == refp) {
                    // If had DOT(CLASSREF this, ...) the DOT can go away
                    dotp->replaceWith(dotp->rhsp()->unlinkFrBack());
                    VL_DO_DANGLING2(ঠেলো_Deletep(dotp), dotp, refp);
                } অন্যথা {
                    VL_DO_DANGLING(ঠেলো_Deletep(refp->unlinkFrBack()), refp);
                }
            }
        });
    }

    // VISITORS
    শূন্য visit(AstNetlist* nodep) override {  // FindVisitor::
        // Process $unit or other packages
        // Not needed - dotted references not allowed from inside packages
        // for (AstNodeModule* nodep = v3Global.rootp()->modulesp();
        //     nodep; nodep=VN_AS(nodep->nextp(), NodeModule)) {
        //    if (VN_IS(nodep, Package)) {}}

        m_statep->ঢোকাও_DUnit(nodep);

        // First back iterate, to find all packages. Backward as must do base
        // packages before using packages
        iterateChildrenBackwardsConst(nodep);

        // The first modules in the list are always the top modules
        // (sorted before this is called).
        // This may not be the module with isTop() set, as early in the steps,
        // wrapTop may have not been created yet.
        // $unit and the constant pool always exist, so nothing else, and nothing in $unit,
        // means nothing was given
        Ast_গ্রন্থি_Module* ধ্রুব modulesp = nodep->modulesp();
        UASSERT_OBJ(modulesp, nodep, "$unit should always be in the netlist");
        যদি (m_statep->forPrimary()) {
            ধ্রুব তার্কিক onlyBuiltins = [&]() {
                প্রতি (Ast_গ্রন্থি_Module* modp = modulesp; modp;
                     modp = VN_AS(modp->nextp(), গ্রন্থি_Module)) {
                    যদি (!modp->isDollarUnit() && !modp->isConstPool()) প্রত্যাগম অসত্য;
                }
                প্রত্যাগম সত্য;
            }();
            যদি (onlyBuiltins && !v3Global.rootp()->dollarUnitPkgp()->stmtsp()) {
                nodep->v3error("No top level module found");
            }
        }
        প্রতি (Ast_গ্রন্থি_Module* modp = nodep->modulesp(); modp && modp->is_শীর্ষ();
             modp = VN_AS(modp->nextp(), গ্রন্থি_Module)) {
            UINFO(8, "Top Module: " << modp);
            m_scope = "TOP";

            যদি (!m_statep->forScopeCreation() && v3Global.opt.শীর্ষ_Ifaces_Supported()) {
                প্রতি (Ast_গ্রন্থি* subnodep = modp->stmtsp(); subnodep; subnodep = subnodep->nextp()) {
                    যদি (AstVar* ধ্রুব varp = VN_CAST(subnodep, Var)) {
                        যদি (varp->isIfaceRef()) {
                            ধ্রুব Ast_গ্রন্থি_DType* ধ্রুব subtypep = varp->subDTypep();
                            ধ্রুব AstIfaceRefDType* ifacerefp = nullptr;
                            যদি (VN_IS(subtypep, IfaceRefDType)) {
                                ifacerefp = VN_AS(varp->subDTypep(), IfaceRefDType);
                            } অন্যথা যদি (VN_IS(subtypep, BracketArrayDType)) {
                                ধ্রুব AstBracketArrayDType* ধ্রুব arrp
                                    = VN_AS(subtypep, BracketArrayDType);
                                ধ্রুব Ast_গ্রন্থি_DType* ধ্রুব arrsubtypep = arrp->subDTypep();
                                যদি (VN_IS(arrsubtypep, IfaceRefDType)) {
                                    ifacerefp = VN_AS(arrsubtypep, IfaceRefDType);
                                }
                            } অন্যথা যদি (VN_IS(subtypep, UnpackArrayDType)) {
                                ধ্রুব AstUnpackArrayDType* ধ্রুব arrp
                                    = VN_AS(subtypep, UnpackArrayDType);
                                ধ্রুব Ast_গ্রন্থি_DType* ধ্রুব arrsubtypep = arrp->subDTypep();
                                যদি (VN_IS(arrsubtypep, IfaceRefDType)) {
                                    ifacerefp = VN_AS(arrsubtypep, IfaceRefDType);
                                }
                            }

                            যদি (ifacerefp && !ifacerefp->cellp()) {
                                // A dummy cell to keep the top level interface alive and correctly
                                // optimized for default parameter values
                                AstCell* ifacecellp
                                    = নূতন AstCell{nodep->fileline(),
                                                  nodep->fileline(),
                                                  modp->name() + "__02E" + varp->name(),
                                                  ifacerefp->ifaceName(),
                                                  nullptr,
                                                  nullptr,
                                                  nullptr};
                                ifacecellp->modp(ifacerefp->ifacep());
                                m_curSymp = m_modSymp
                                    = m_statep->ঢোকাও_শীর্ষ_Iface(ifacecellp, m_scope);
                                iterate(ifacecellp);
                                VL_DO_DANGLING(ঠেলো_Deletep(ifacecellp), ifacecellp);
                            }
                        }
                    }
                }
            }

            m_curSymp = m_modSymp = m_statep->ঢোকাও_শীর্ষ_Cell(modp, m_scope);
            { iterate(modp); }

            m_scope = "";
            m_curSymp = m_modSymp = nullptr;
        }
    }
    শূন্য visit(AstTypeTable*) override {}  // FindVisitor::
    শূন্য visit(AstIfaceRefDType* nodep) override {  // FindVisitor::
        যদি (!m_statep->forScopeCreation() && nodep->isVirtual() && nodep->ifacep()
            && !nodep->ifacep()->user3()) {
            m_virtIfaces.ঠেলো_back(nodep->ifacep());
            nodep->ifacep()->user3(সত্য);
        }
        iterateChildren(nodep);
    }
    শূন্য visit(Ast_গ্রন্থি_Module* nodep) override {  // FindVisitor::
        // Called on top module from Netlist, other modules from the cell creating them,
        // and packages
        UINFO(8, "   " << nodep);
        // m_curSymp/m_modSymp maybe nullptr for packages and non-top modules
        // Packages will be under top after the initial phases, but until then
        // need separate handling
        ধ্রুব তার্কিক standalonePkg
            = !m_modSymp && !m_statep->forScopeCreation() && VN_IS(nodep, Package);
        ধ্রুব তার্কিক doit = (m_modSymp || standalonePkg);
        VL_RESTORER_COPY(m_scope);
        VL_RESTORER(m_classOrPackagep);
        VL_RESTORER(m_modSymp);
        VL_RESTORER(m_curSymp);
        VL_RESTORER(m_param_অঙ্ক);
        VL_RESTORER(m_mod_Block_অঙ্ক);
        VL_RESTORER(m_mod_With_অঙ্ক);
        VL_RESTORER(m_mod_Arg_অঙ্ক);
        যদি (doit && nodep->user2()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Identically recursive module (module instantiates "
                          "itself, without changing parameters): "
                              << Ast_গ্রন্থি::prettyNameQ(nodep->origName()));
        } অন্যথা যদি (doit) {
            UINFO(4, "     Link Module: " << nodep);
            UASSERT_OBJ(!nodep->dead(), nodep, "Module in instance tree mislabeled as dead?");
            AstPackage* ধ্রুব pkgp = VN_CAST(nodep, Package);
            m_classOrPackagep = pkgp;
            যদি (standalonePkg) {
                যদি (pkgp->isDollarUnit()) {
                    m_curSymp = m_modSymp = m_statep->dunitEntp();
                    nodep->user1p(m_curSymp);
                } অন্যথা {
                    VSymEnt* ধ্রুব upperSymp = m_statep->dunitEntp();
                    m_scope = nodep->name();
                    m_curSymp = m_modSymp = m_statep->ঢোকাও_Block(upperSymp, nodep->name(), nodep,
                                                                  m_classOrPackagep);
                    UINFO(9, "New module scope " << m_curSymp);
                }
            }
            //
            m_param_অঙ্ক = 0;
            m_mod_Block_অঙ্ক = 0;
            m_mod_With_অঙ্ক = 0;
            m_mod_Arg_অঙ্ক = 0;
            // m_modSymp/m_curSymp for non-packages set by AstCell above this module
            // Iterate
            nodep->user2(সত্য);
            iterateChildren(nodep);
            nodep->user2(অসত্য);
            nodep->user4(সত্য);
            // Interfaces need another pass when signals are resolved. When creating
            // scopes this is done from the AstScopes in LinkDotScopeVisitor instead, as
            // only those entries hold the AstVarScopes the modport items resolve against.
            যদি (!m_statep->forScopeCreation()) {
                যদি (AstIface* ধ্রুব ifacep = VN_CAST(nodep, Iface)) {
                    m_statep->ঢোকাও_Iface_Mod_Sym(ifacep, m_curSymp);
                }
            }
        } অন্যথা যদি (isHierBlockWrapper(nodep->name())) {
            UINFO(5, "Module is hierarchical block, must not be dead: " << nodep);
            m_scope = nodep->name();
            VSymEnt* ধ্রুব upperSymp = m_curSymp ? m_curSymp : m_statep->মূল_Entp();
            m_curSymp = m_modSymp
                = m_statep->ঢোকাও_Block(upperSymp, nodep->name() + "::", nodep, m_classOrPackagep);
            iterateChildren(nodep);
            nodep->user4(সত্য);
        } অন্যথা {  // !doit
            যদি (nodep->hierParams()) {
                UINFO(1, "Found module with hier type parameters");
                m_hierParamsName = nodep->name();
                প্রতি (ধ্রুব Ast_গ্রন্থি* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    যদি (ধ্রুব AstTypedef* ধ্রুব tdef = VN_CAST(stmtp, Typedef)) {
                        UINFO(1, "Inserting hier type parameter typedef: " << tdef);
                        VSymEnt* ধ্রুব upperSymp = m_curSymp ? m_curSymp : m_statep->মূল_Entp();
                        m_curSymp = m_modSymp = m_statep->ঢোকাও_Block(upperSymp, nodep->name(),
                                                                      nodep, m_classOrPackagep);
                    }
                }
            } অন্যথা {
                // Will be optimized away later
                // Can't remove now, as our backwards iterator will throw up
                UINFO(5, "Module not under any CELL or top - dead module: " << nodep);
            }
        }
    }

    শূন্য visit(AstClass* nodep) override {  // FindVisitor::
        UASSERT_OBJ(m_curSymp, nodep, "Class not under module/package/$unit");
        UINFO(8, "   " << nodep);
        VL_RESTORER_COPY(m_scope);
        VL_RESTORER(m_classOrPackagep);
        VL_RESTORER(m_modSymp);
        VL_RESTORER(m_curSymp);
        VL_RESTORER(m_param_অঙ্ক);
        VL_RESTORER(m_mod_Block_অঙ্ক);
        VL_RESTORER(m_mod_With_অঙ্ক);
        VL_RESTORER(m_mod_Arg_অঙ্ক);
        VL_RESTORER(m_explicitNew);
        {
            UINFO(4, "     Link Class: " << nodep);
            VSymEnt* ধ্রুব upperSymp = m_curSymp;
            m_scope = m_scope + "." + nodep->name();
            m_classOrPackagep = nodep;
            m_curSymp = m_modSymp
                = m_statep->ঢোকাও_Block(upperSymp, nodep->name(), nodep, m_classOrPackagep);
            m_statep->ঢোকাও_Map(m_curSymp, m_scope);
            UINFO(9, "New module scope " << m_curSymp);
            //
            m_param_অঙ্ক = 0;
            m_mod_Block_অঙ্ক = 0;
            m_mod_With_অঙ্ক = 0;
            m_mod_Arg_অঙ্ক = 0;
            m_explicitNew = অসত্য;
            // m_modSymp/m_curSymp for non-packages set by AstCell above this module
            // Iterate
            iterateChildren(nodep);
            nodep->user4(সত্য);
            // Implicit new needed?
            যদি (!m_explicitNew && m_statep->forPrimary()) makeImplicitNew(nodep);
        }
    }
    শূন্য visit(AstClassOrPackageRef* nodep) override {  // FindVisitor::
        যদি (!nodep->classOrPackageNodep() && nodep->name() == "$unit") {
            nodep->classOrPackageNodep(v3Global.rootp()->dollarUnitPkgp());
        }
        iterateChildren(nodep);
    }
    শূন্য visit(AstScope* nodep) override {  // FindVisitor::
        UASSERT_OBJ(m_statep->forScopeCreation(), nodep,
                    "Scopes should only exist right after V3Scope");
        // Detach on first encounter, will re add at the end.
        // Otherwise would be visited O(cells^2) time after V3Scope
        যদি (!nodep->is_শীর্ষ()) {
            ++m_statScopeVisits;
            nodep->unlinkFrBack();
            m_scopes.ঠেলো_back(nodep);
        }
    }
    শূন্য visit(AstCell* nodep) override {  // FindVisitor::
        UINFO(5, "   CELL under " << m_scope << " is " << nodep);
        // Process XREFs/etc inside pins
        যদি (nodep->recursive() && m_inRecursion) প্রত্যাগম;
        iterateChildren(nodep);
        // Recurse in, preserving state
        VL_RESTORER_COPY(m_scope);
        VL_RESTORER(m_modSymp);
        VL_RESTORER(m_curSymp);
        VL_RESTORER(m_param_অঙ্ক);
        VL_RESTORER(m_inRecursion);
        // Where do we add it?
        VSymEnt* aboveSymp = m_curSymp;
        ধ্রুব string origname = Ast_গ্রন্থি::dedotName(nodep->name());
        string::size_type pos = origname.rfind('.');
        যদি (pos != string::npos) {
            // Flattened, find what CellInline it should live under
            ধ্রুব string scope = origname.substr(0, pos);
            string baddot;
            VSymEnt* okSymp;
            aboveSymp
                = m_statep->findDotted(nodep->fileline(), aboveSymp, scope, baddot, okSymp, অসত্য);
            UASSERT_OBJ(aboveSymp, nodep,
                        "Can't find instance insertion point at "
                            << Ast_গ্রন্থি::prettyNameQ(baddot) << " in: " << nodep->prettyNameQ());
        }
        {
            m_scope = m_scope + "." + nodep->name();
            m_curSymp = m_modSymp = m_statep->ঢোকাও_Cell(aboveSymp, m_modSymp, nodep, m_scope);
            m_inRecursion = nodep->recursive();
            // We don't report NotFoundModule, as may be a unused module in a generate
            যদি (nodep->modp()) iterate(nodep->modp());
        }
    }
    শূন্য visit(AstCellInline* nodep) override {  // FindVisitor::
        UINFO(5, "   CELLINLINE under " << m_scope << " is " << nodep);
        VSymEnt* aboveSymp = m_curSymp;
        std::string ident = nodep->name();
        // If baz__DOT__foo__DOT__bar, we need to find baz__DOT__foo and add bar to it.
        ধ্রুব std::string::size_type pos = ident.rfind("__DOT__");
        যদি (pos != std::string::npos) {
            ধ্রুব std::string dotted = ident.substr(0, pos);
            ident = ident.substr(pos + std::strlen("__DOT__"));
            std::string baddot;
            VSymEnt* okSymp;
            aboveSymp = m_statep->findDotted(nodep->fileline(), aboveSymp, dotted, baddot, okSymp,
                                             অসত্য);
            UASSERT_OBJ(aboveSymp, nodep,
                        "Can't find cellinline insertion point at "
                            << Ast_গ্রন্থি::prettyNameQ(baddot) << " in: " << nodep->prettyNameQ());
        }
        m_statep->ঢোকাও_Inline(aboveSymp, m_modSymp, nodep, ident);
    }
    শূন্য visit(AstDefParam* nodep) override {  // FindVisitor::
        nodep->user1p(m_curSymp);
        iterateChildren(nodep);
    }
    শূন্য visit(AstRefDType* nodep) override {  // FindVisitor::
        nodep->user1p(m_curSymp);
        iterateChildren(nodep);
    }
    শূন্য visit(AstGenBlock* nodep) override {  // FindVisitor::
        UINFO(5, "   " << nodep);
        যদি (nodep->name() == "" && nodep->unnamed()) {
            // Unnamed blocks are only important when they contain var
            // decls, so search for them. (Otherwise adding all the
            // unnamed#'s would just confuse tracing variables in
            // places such as tasks, where "task ...; begin ... end"
            // are common.
            প্রতি (Ast_গ্রন্থি* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
                যদি (VN_IS(itemp, Var) || VN_IS(itemp, Foreach)) {
                    std::string name;
                    ধ্রুব std::string stepStr = m_statep->forPrimary()
                                                    ? ""
                                                    : std::to_string(m_statep->step_সংখ্যা()) + "_";
                    কুরু {
                        ++m_mod_Block_অঙ্ক;
                        name = "unnamedblk" + stepStr + cvtToStr(m_mod_Block_অঙ্ক);
                        // Increment again if earlier pass of V3LinkDot claimed this name
                    } যাবত্ (m_curSymp->findIdFlat(name));
                    nodep->name(name);
                    ভঙ্গ;
                }
            }
        }
        যদি (nodep->name() == "") {
            iterateChildren(nodep);
        } অন্যথা {
            VL_RESTORER(m_curSymp);
            {
                m_curSymp
                    = m_statep->ঢোকাও_Block(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
                m_curSymp->fallbackp(VL_RESTORER_PREV(m_curSymp));
                // Iterate
                iterateChildren(nodep);
            }
        }
    }
    শূন্য visit(Ast_গ্রন্থি_Block* nodep) override {  // FindVisitor::
        UINFO(5, "   " << nodep);
        যদি (nodep->name() == "" && nodep->unnamed()) {
            // Unnamed blocks are only important when they contain var
            // decls, so search for them. (Otherwise adding all the
            // unnamed#'s would just confuse tracing variables in
            // places such as tasks, where "task ...; begin ... end"
            // are common.
            ধ্রুব auto containsDecl = [](ধ্রুব Ast_গ্রন্থি* nodesp) -> তার্কিক {
                প্রতি (ধ্রুব Ast_গ্রন্থি* np = nodesp; np; np = np->nextp()) {
                    যদি (VN_IS(np, Var) || VN_IS(np, Foreach)) প্রত্যাগম সত্য;
                }
                প্রত্যাগম অসত্য;
            };
            তার্কিক needName = nodep->declsp() || containsDecl(nodep->stmtsp());
            যদি (ধ্রুব AstFork* ধ্রুব forkp = VN_CAST(nodep, Fork)) {
                needName = needName || containsDecl(forkp->forksp());
            }
            যদি (needName) {
                ধ্রুব std::string stepStr
                    = m_statep->forPrimary() ? "" : std::to_string(m_statep->step_সংখ্যা()) + "_";
                std::string name;
                কুরু {
                    ++m_mod_Block_অঙ্ক;
                    name = "unnamedblk" + stepStr + cvtToStr(m_mod_Block_অঙ্ক);
                    // Increment again if earlier pass of V3LinkDot claimed this name
                } যাবত্ (m_curSymp->findIdFlat(name));
                nodep->name(name);
            }
        }
        যদি (nodep->name() == "") {
            iterateChildren(nodep);
        } অন্যথা {
            VL_RESTORER(m_curSymp);
            {
                m_curSymp
                    = m_statep->ঢোকাও_Block(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
                m_curSymp->fallbackp(VL_RESTORER_PREV(m_curSymp));
                // Iterate
                iterateChildren(nodep);
            }
        }
    }
    // Handle out-of-block interface method definition (IEEE 1800-2023 25.7)
    // Move task body from module into the interface's prototype task,
    // rewriting port-prefixed references in the body.
    শূন্য moveIfaceExportBody(Ast_গ্রন্থি_FTask* nodep) {
        ধ্রুব string& portName = nodep->ifacePortName();
        UINFO(5, "   moveIfaceExportBody: port=" << portName << " task=" << nodep->name());
        // Look up the interface port variable in the module's symbol table
        VSymEnt* ধ্রুব portSymp = m_modSymp->findIdFallback(portName);
        যদি (!portSymp) {
            nodep->v3error("Interface port not found for out-of-block definition: '"
                           << portName << "' (IEEE 1800-2023 25.7)");
            nodep->unlinkFrBack();
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
            প্রত্যাগম;
        }
        AstVar* ধ্রুব portp = VN_CAST(portSymp->nodep(), Var);
        যদি (!portp) {
            nodep->v3fatalSrc("Out-of-block definition port is not a variable: " << portName);
            প্রত্যাগম;
        }
        // Get the interface name from the port's type
        // At this stage dtypep() may not be set yet; check childDTypep() too
        AstIfaceRefDType* ifaceRefDtp = nullptr;
        যদি (portp->dtypep()) { ifaceRefDtp = VN_CAST(portp->dtypep(), IfaceRefDType); }
        যদি (!ifaceRefDtp && portp->childDTypep()) {
            ifaceRefDtp = VN_CAST(portp->childDTypep(), IfaceRefDType);
        }
        যদি (!ifaceRefDtp) {
            nodep->v3error("Out-of-block definition port is not an interface port: '"
                           << portName << "' (IEEE 1800-2023 25.7)");
            nodep->unlinkFrBack();
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
            প্রত্যাগম;
        }
        ধ্রুব string ifaceName = ifaceRefDtp->ifaceName();
        UINFO(5, "   moveIfaceExportBody: iface=" << ifaceName);
        // Find the interface module by name
        Ast_গ্রন্থি_Module* ধ্রুব ifaceModp = m_statep->findModuleSym(ifaceName);
        যদি (!ifaceModp) {
            nodep->v3fatalSrc("Interface not found for out-of-block definition: " << ifaceName);
            প্রত্যাগম;
        }
        // Find the prototype task in the interface
        Ast_গ্রন্থি_FTask* protoTaskp = nullptr;
        প্রতি (Ast_গ্রন্থি* itemp = ifaceModp->stmtsp(); itemp; itemp = itemp->nextp()) {
            যদি (Ast_গ্রন্থি_FTask* ধ্রুব ftaskp = VN_CAST(itemp, গ্রন্থি_FTask)) {
                যদি (ftaskp->name() == nodep->name() && ftaskp->prototype()) {
                    protoTaskp = ftaskp;
                    ভঙ্গ;
                }
            }
        }
        যদি (!protoTaskp) {
            nodep->v3error("No matching export prototype found in interface "
                           << ifaceName << " for task " << nodep->prettyNameQ()
                           << " (IEEE 1800-2023 25.7)");
            nodep->unlinkFrBack();
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
            প্রত্যাগম;
        }
        // Move body statements (non-port declarations) from the out-of-block task
        // to the prototype task. Rewrite port.X references to just X in the body.
        প্রতি (Ast_গ্রন্থি* stmtp = nodep->stmtsp(); stmtp;) {
            Ast_গ্রন্থি* ধ্রুব nextp = stmtp->nextp();
            // Skip port declarations -- they already exist in the prototype
            যদি (ধ্রুব AstVar* ধ্রুব varp = VN_CAST(stmtp, Var)) {
                যদি (varp->isIO()) {
                    stmtp = nextp;
                    অনুবর্ত;
                }
            }
            // Clone this statement and rewrite port references
            Ast_গ্রন্থি* ধ্রুব newStmtp = stmtp->clone_বৃক্ষ(অসত্য);
            rewriteIfacePortRefsSingle(newStmtp, portName);
            protoTaskp->addStmtsp(newStmtp);
            stmtp = nextp;
        }
        // Mark prototype as no longer prototype (it has a body now)
        protoTaskp->prototype(অসত্য);
        // Delete the out-of-block task from the module
        nodep->unlinkFrBack();
        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
    }

    // Rewrite AstDot nodes where lhsp is AstParseRef with the port name
    // to just the rhsp (stripping the port prefix).
    স্থির শূন্য rewriteIfacePortRefsSingle(Ast_গ্রন্থি* nodep, ধ্রুব string& portName) {
        যদি (!nodep) প্রত্যাগম;
        // Check if this is a non-colon Dot with lhs matching portName
        যদি (AstDot* ধ্রুব dotp = VN_CAST(nodep, Dot)) {
            যদি (!dotp->colon()) {
                ধ্রুব AstParseRef* ধ্রুব lhsRefp = VN_CAST(dotp->lhsp(), ParseRef);
                যদি (lhsRefp && lhsRefp->name() == portName) {
                    UINFO(5, "   rewriteIfacePortRefs: stripping port prefix from " << dotp);
                    Ast_গ্রন্থি* ধ্রুব rhsp = dotp->rhsp()->unlinkFrBack();
                    dotp->replaceWith(rhsp);
                    VL_DO_DANGLING(dotp->delete_বৃক্ষ(), dotp);
                    rewriteIfacePortRefsSingle(rhsp, portName);
                    প্রত্যাগম;
                }
            }
        }
        // Recurse into all children
        প্রতি (Ast_গ্রন্থি* childp : {nodep->op1p(), nodep->op2p(), nodep->op3p(), nodep->op4p()}) {
            যাবত্ (childp) {
                Ast_গ্রন্থি* ধ্রুব nextp = childp->nextp();
                rewriteIfacePortRefsSingle(childp, portName);
                childp = nextp;
            }
        }
    }

    শূন্য visit(Ast_গ্রন্থি_FTask* nodep) override {  // FindVisitor::
        // NodeTask: Remember its name for later resolution
        UINFO(5, "   " << nodep);
        UASSERT_OBJ(m_curSymp && m_modSymp, nodep, "Function/Task not under module?");
        যদি (nodep->name() == "new") m_explicitNew = সত্য;
        // Handle out-of-block interface method definition (IEEE 25.8)
        যদি (!nodep->ifacePortName().empty() && m_statep->forPrimary()) {
            moveIfaceExportBody(nodep);
            প্রত্যাগম;  // Task has been deleted
        }
        // Remember the existing symbol table scope
        VL_RESTORER(m_classOrPackagep);
        VL_RESTORER(m_curSymp);
        VSymEnt* upSymp = m_curSymp;

        যদি (VN_IS(m_curSymp->nodep(), Class)) {
            যদি (VN_AS(m_curSymp->nodep(), Class)->isInterfaceClass() && !nodep->pureVirtual()
                && !nodep->isConstructor()) {
                nodep->v3error("Interface class functions must be pure virtual"
                               << " (IEEE 1800-2023 8.26): " << nodep->prettyNameQ());
            }
            যদি (m_statep->forPrimary()
                && (nodep->name() == "randomize" || nodep->name() == "srandom")) {
                nodep->v3error(nodep->prettyNameQ()
                               << " is a predefined class method; redefinition not allowed"
                                  " (IEEE 1800-2023 18.6.3)");
            }
        }
        // Change to appropriate package if extern declaration (vs definition)
        যদি (nodep->classOrPackagep()) {
            // class-in-class
            AstDot* ধ্রুব dotp = VN_CAST(nodep->classOrPackagep(), Dot);
            AstClassOrPackageRef* ধ্রুব cpackagerefp
                = VN_CAST(nodep->classOrPackagep(), ClassOrPackageRef);
            যদি (dotp) {
                AstClassOrPackageRef* ধ্রুব lhsp = VN_AS(dotp->lhsp(), ClassOrPackageRef);
                m_statep->resolveClassOrPackage(m_curSymp, lhsp, সত্য, অসত্য,
                                                "External definition :: reference");
                AstClass* ধ্রুব lhsclassp = VN_CAST(lhsp->classOrPackageSkipp(), Class);
                যদি (!lhsclassp) {
                    nodep->v3error("Extern declaration's scope is not a defined class");
                } অন্যথা {
                    m_curSymp = m_statep->get_গ্রন্থি_Sym(lhsclassp);
                    upSymp = m_curSymp;
                    AstClassOrPackageRef* ধ্রুব rhsp = VN_AS(dotp->rhsp(), ClassOrPackageRef);
                    m_statep->resolveClassOrPackage(m_curSymp, rhsp, সত্য, অসত্য,
                                                    "External definition :: reference");
                    AstClass* ধ্রুব rhsclassp = VN_CAST(rhsp->classOrPackageSkipp(), Class);
                    যদি (!rhsclassp) {
                        nodep->v3error("Extern declaration's scope is not a defined class");
                    } অন্যথা {
                        m_curSymp = m_statep->get_গ্রন্থি_Sym(rhsclassp);
                        upSymp = m_curSymp;
                        moveExternFuncDecl(nodep, rhsclassp);
                    }
                }
            } অন্যথা যদি (cpackagerefp) {
                যদি (!cpackagerefp->classOrPackageSkipp()) {
                    m_statep->resolveClassOrPackage(m_curSymp, cpackagerefp, সত্য, অসত্য,
                                                    "External definition :: reference");
                }
                AstClass* ধ্রুব classp = VN_CAST(cpackagerefp->classOrPackageSkipp(), Class);
                যদি (!classp) {
                    nodep->v3error("Extern declaration's scope is not a defined class");
                } অন্যথা {
                    m_curSymp = m_statep->get_গ্রন্থি_Sym(classp);
                    upSymp = m_curSymp;
                    moveExternFuncDecl(nodep, classp);
                }
            } অন্যথা {
                v3fatalSrc("Unhandled extern function definition package");
            }
        }
        // Set the class as package for iteration
        যদি (VN_IS(m_curSymp->nodep(), Class)) {
            m_classOrPackagep = VN_AS(m_curSymp->nodep(), Class);
        }
        // Create symbol table for the task's vars
        ধ্রুব string name = (nodep->isExternProto() ? "extern "s : ""s) + nodep->name();
        m_curSymp = m_statep->ঢোকাও_Block(m_curSymp, name, nodep, m_classOrPackagep);
        m_curSymp->fallbackp(upSymp);
        // Convert the func's range to the output variable
        // This should probably be done in the Parser instead, as then we could
        // just attach normal signal attributes to it.
        যদি (!nodep->isExternProto() && nodep->fvarp() && !VN_IS(nodep->fvarp(), Var)) {
            Ast_গ্রন্থি_DType* dtypep = VN_CAST(nodep->fvarp(), গ্রন্থি_DType);
            // If unspecified, function returns one bit; however when we
            // support NEW() it could also return the class reference.
            যদি (dtypep) {
                dtypep->unlinkFrBack();
            } অন্যথা {
                dtypep = নূতন AstBasicDType{nodep->fileline(), VBasicDTypeKwd::LOGIC};
            }
            AstVar* ধ্রুব newvarp
                = নূতন AstVar{nodep->fileline(), VVarType::VAR, nodep->name(), VFlagChildDType{},
                             dtypep};  // Not dtype resolved yet
            newvarp->direction(VDirection::OUTPUT);
            newvarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            newvarp->funcReturn(সত্য);
            newvarp->trace(অসত্য);  // Not user visible
            nodep->fvarp(newvarp);
            // Explicit insert required, as the var name shadows the upper level's task name
            m_statep->ঢোকাও_Sym(m_curSymp, newvarp->name(), newvarp, nullptr /*classOrPackagep*/);
        }
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    শূন্য visit(AstClocking* nodep) override {  // FindVisitor::
        VL_RESTORER(m_clockingp);
        m_clockingp = nodep;
        iterate(nodep->sentreep());
        iterateAndNextNull(nodep->itemsp());
        // If the block has no name, one cannot reference the clockvars
        VSymEnt* itSymp = nullptr;
        যদি (nodep->isGlobal()  //
            && m_statep->forPrimary()) {  // else flattening may see two globals
            m_statep->checkDuplicate(m_curSymp, nodep, "__024global_clock");
            itSymp
                = m_statep->ঢোকাও_Block(m_curSymp, "__024global_clock", nodep, m_classOrPackagep);
            itSymp->fallbackp(nullptr);
        }
        যদি (!nodep->name().empty()) {
            itSymp = m_statep->ঢোকাও_Block(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
            itSymp->fallbackp(nullptr);
        }
        যদি (itSymp) {
            VL_RESTORER(m_curSymp);
            m_curSymp = itSymp;
            iterateAndNextNull(nodep->itemsp());
        }
    }
    শূন্য visit(AstClockingItem* nodep) override {  // FindVisitor::
        যদি (nodep->varp()) {
            যদি (m_curSymp->nodep() == m_clockingp) iterate(nodep->varp());
            প্রত্যাগম;
        }
        std::string varname;
        Ast_গ্রন্থি_DType* dtypep;
        যদি (AstAssign* ধ্রুব assignp = nodep->assignp()) {
            Ast_গ্রন্থি_রাশিমালা* ধ্রুব rhsp = assignp->rhsp()->unlinkFrBack();
            dtypep = নূতন AstRefDType{nodep->fileline(), AstRefDType::Flag_Type_Of_রাশিমালা{},
                                     rhsp->clone_বৃক্ষ(অসত্য)};
            nodep->exprp(rhsp);
            varname = assignp->lhsp()->name();
            VL_DO_DANGLING(assignp->unlinkFrBack()->delete_বৃক্ষ(), assignp);
        } অন্যথা {
            Ast_গ্রন্থি_রাশিমালা* ধ্রুব refp = nodep->exprp();
            ধ্রুব VSymEnt* foundp = m_curSymp->findIdFallback(refp->name());
            যদি (!foundp || !foundp->nodep()) {
                refp->v3error("Corresponding variable " << refp->prettyNameQ()
                                                        << " does not exist");
                VL_DO_DANGLING(nodep->unlinkFrBack()->delete_বৃক্ষ(), nodep);
                প্রত্যাগম;
            }
            varname = refp->name();
            dtypep = VN_AS(foundp->nodep(), Var)->childDTypep()->clone_বৃক্ষ(অসত্য);
        }
        AstVar* ধ্রুব newvarp = নূতন AstVar{nodep->fileline(), VVarType::MODULETEMP, varname,
                                           VFlagChildDType{}, dtypep};
        newvarp->lifetime(VLifetime::STATIC_EXPLICIT);
        nodep->varp(newvarp);
        iterate(nodep->exprp());
    }
    শূন্য visit(AstConstraint* nodep) override {  // FindVisitor::
        VL_RESTORER(m_curSymp);
        // Change to appropriate package if extern declaration (vs definition)
        VSymEnt* upSymp = m_curSymp;
        যদি (nodep->classOrPackagep()) {
            AstClassOrPackageRef* ধ্রুব cpackagerefp
                = VN_CAST(nodep->classOrPackagep(), ClassOrPackageRef);
            যদি (!cpackagerefp) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: extern constraint definition with class-in-class");
            } অন্যথা {
                যদি (!cpackagerefp->classOrPackageSkipp()) {
                    m_statep->resolveClassOrPackage(m_curSymp, cpackagerefp, সত্য, অসত্য,
                                                    "External definition :: reference");
                }
                AstClass* ধ্রুব classp = VN_CAST(cpackagerefp->classOrPackageSkipp(), Class);
                যদি (!classp) {
                    nodep->v3error("Extern declaration's scope is not a defined class");
                } অন্যথা {
                    m_curSymp = m_statep->get_গ্রন্থি_Sym(classp);
                    upSymp = m_curSymp;
                    // Move it to proper spot under the target class
                    nodep->unlinkFrBack();
                    classp->addStmtsp(nodep);
                    nodep->classOrPackagep()->unlinkFrBack()->delete_বৃক্ষ();
                }
            }
        }
        // Set the class as package for iteration
        ধ্রুব string name = (nodep->isExternProto() ? "extern "s : ""s) + nodep->name();
        m_curSymp = m_statep->ঢোকাও_Block(upSymp, name, nodep, m_classOrPackagep);
        iterateChildren(nodep);
    }
    শূন্য visit(AstVar* nodep) override {  // FindVisitor::
        // Var: Remember its name for later resolution
        UASSERT_OBJ(m_curSymp && m_modSymp, nodep, "Var not under module?");
        iterateChildren(nodep);
        যদি (VN_IS(m_curSymp->nodep(), Class)
            && VN_AS(m_curSymp->nodep(), Class)->isInterfaceClass() && !nodep->isParam()) {
            nodep->v3error("Interface class cannot contain non-parameter members"
                           << " (IEEE 1800-2023 8.26): " << nodep->prettyNameQ());
        }
        যদি (!m_statep->forScopeCreation()) {
            // Find under either a task or the module's vars
            ধ্রুব VSymEnt* foundp = m_curSymp->findIdFallback(nodep->name());
            যদি (!foundp && m_modSymp && nodep->name() == m_modSymp->nodep()->name()) {
                foundp = m_modSymp;  // Conflicts with modname?
            }
            AstVar* ধ্রুব findvarp = foundp ? VN_CAST(foundp->nodep(), Var) : nullptr;
            // clocking items can have duplicate names (inout)
            যদি (findvarp && VN_IS(findvarp->backp(), ClockingItem)
                && VN_IS(nodep->backp(), ClockingItem)) {
                AstClockingItem* ধ্রুব itemp = VN_AS(nodep->backp(), ClockingItem);
                AstClockingItem* ধ্রুব finditemp = VN_AS(findvarp->backp(), ClockingItem);
                UINFO(4, "ClockCompl: " << itemp << " ;; " << finditemp);
                UINFO(4, "ClockCompV: " << nodep << " ;; " << findvarp);
                যদি (*itemp->exprp()->fileline() == *finditemp->exprp()->fileline()) {
                    UASSERT_OBJ(finditemp->direction() == VDirection::প্রবেশ
                                    && itemp->direction() == VDirection::OUTPUT,
                                itemp, "Input after output?");
                    // pretend nothing found and rename
                    foundp = nullptr;
                    nodep->name("__Voutput_" + nodep->name());
                    finditemp->outputp(itemp);
                }
            }
            তার্কিক ins = অসত্য;
            যদি (!foundp) {
                ins = সত্য;
            } অন্যথা যদি (!findvarp && m_curSymp->findIdFlat(nodep->name()) && !foundp->imported()) {
                nodep->v3error("Unsupported in C: Variable has same name as "
                               << LinkDotState::গ্রন্থি_Text_Type(foundp->nodep()) << ": "
                               << nodep->prettyNameQ());
            } অন্যথা যদি (findvarp != nodep) {
                UINFO(4, "DupVar: " << nodep << " ;; " << foundp->nodep());
                UINFO(4, "    found  cur=se" << cvtToHex(m_curSymp) << " ;; parent=se"
                                             << cvtToHex(foundp->parentp()));
                যদি (foundp->parentp() == m_curSymp  // Only when on same level
                    && !foundp->imported()) {  // and not from package
                    যদি (VN_IS(m_curSymp->nodep(), Clocking)) {
                        nodep->v3error("Multiple clockvars with the same name not allowed");
                        প্রত্যাগম;
                    }
                    ধ্রুব তার্কিক nansiBad
                        = (((findvarp->isDeclTyped() || findvarp->isNet())
                            && (nodep->isDeclTyped() || nodep->isNet()))
                           || (findvarp->isIO() && nodep->isIO())  // e.g. !(output && output)
                           || findvarp->gotNansiType());  // e.g. int x && int x
                    ধ্রুব তার্কিক ansiBad
                        = findvarp->isAnsi() || nodep->isAnsi();  // dup illegal with ANSI
                    যদি (ansiBad || nansiBad) {
                        তার্কিক ansiWarn = ansiBad && !nansiBad;
                        যদি (ansiWarn) {
                            স্থির পূর্ণাঙ্ক s_didAnsiWarn = অসত্য;
                            যদি (s_didAnsiWarn++) ansiWarn = অসত্য;
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
                    } অন্যথা {
                        findvarp->combineType(nodep);
                        findvarp->fileline()->modifyStateInherit(nodep->fileline());
                        findvarp->gotNansiType(সত্য);
                        UASSERT_OBJ(nodep->subDTypep(), nodep, "Var has no type");
                        যদি (nodep->subDTypep()->numeric().isSigned()
                            && !findvarp->subDTypep()->numeric().isSigned()) {
                            findvarp->subDTypep()->numeric(VSigning{সত্য});
                        }
                        Ast_গ্রন্থি_DType* ধ্রুব varDtp = findvarp->subDTypep();
                        Ast_গ্রন্থি_DType* ধ্রুব otherDtp = nodep->subDTypep();
                        যদি (varDtp
                            && (VN_IS(varDtp, BasicDType)
                                && VN_AS(varDtp, BasicDType)->implicit())) {
                            // Then have "input foo" and "real foo" so the
                            // dtype comes from the other side.
                            VL_DO_DANGLING(varDtp->unlinkFrBack()->delete_বৃক্ষ(), varDtp);
                            findvarp->childDTypep(otherDtp->unlinkFrBack());
                        } অন্যথা যদি (!m_statep->forScopeCreation() && otherDtp && varDtp
                                   && !(VN_IS(otherDtp, BasicDType)
                                        && VN_AS(otherDtp, BasicDType)->implicit())) {
                            // otherDtp and varDtp both non-nullptr and neither are implicit
                            // Can't compare dtypes now as might contain parameters,
                            // defer to V3Width
                            AstAttrOf* ধ্রুব newp
                                = নূতন AstAttrOf{otherDtp->fileline(), VAttrType::VAR_PORT_DTYPE,
                                                otherDtp->unlinkFrBack()};
                            findvarp->addAttrsp(newp);
                        }
                    }
                    VL_DO_DANGLING(nodep->unlinkFrBack()->delete_বৃক্ষ(), nodep);
                } অন্যথা {
                    // User can disable the message at either point
                    যদি (!(m_ftaskp && m_ftaskp->dpiImport())
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
                    ins = সত্য;
                }
            }
            যদি (ins) {
                যদি (m_statep->forPrimary() && nodep->isGParam()
                    && VN_IS(m_modSymp->nodep(), Module)
                    && (m_statep->মূল_Entp()->nodep() == m_modSymp->parentp()->nodep())) {
                    // This is the toplevel module. Check for command line overwrites of parameters
                    // We first search if the parameter is overwritten and then replace it with a
                    // new value.
                    যদি (v3Global.opt.hasParameter(nodep->name())) {
                        ধ্রুব string svalue = v3Global.opt.parameter(nodep->name());
                        যদি (AstConst* ধ্রুব valuep
                            = AstConst::parseParamLiteral(nodep->fileline(), svalue)) {
                            UINFO(9, "       replace parameter " << nodep);
                            UINFO(9, "       with " << valuep);
                            যদি (nodep->valuep()) ঠেলো_Deletep(nodep->valuep()->unlinkFrBack());
                            nodep->valuep(valuep);
                            // Do not warn ENUMVALUE when overriding an enum param from the cli.
                            nodep->fileline()->warnOff(V3ErrorCode::ENUMVALUE, সত্য);
                        }
                    }
                }
                m_statep->ঢোকাও_Sym(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
                যদি (m_statep->forPrimary() && nodep->isGParam()) {
                    ++m_param_অঙ্ক;
                    VSymEnt* ধ্রুব symp
                        = m_statep->ঢোকাও_Sym(m_curSymp, "__paramNumber" + cvtToStr(m_param_অঙ্ক),
                                              nodep, m_classOrPackagep);
                    symp->exported(অসত্য);
                }
            }
        }
    }
    শূন্য visit(AstTypedef* nodep) override {  // FindVisitor::
        UASSERT_OBJ(m_curSymp, nodep, "Typedef not under module/package/$unit");
        যদি (VN_IS(m_classOrPackagep, Class)) nodep->isUnderClass(সত্য);
        iterateChildren(nodep);
        m_statep->ঢোকাও_Sym(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
    }
    শূন্য visit(AstTypedefFwd* nodep) override {  // FindVisitor::
        UASSERT_OBJ(m_curSymp, nodep, "Typedef not under module/package/$unit");
        iterateChildren(nodep);
        // No need to insert, only the real typedef matters, but need to track for errors
        nodep->user1p(m_curSymp);
    }
    শূন্য visit(Ast_গ্রন্থি_UOr_Struct_DType* nodep) override {  // FindVisitor::
        UASSERT_OBJ(m_curSymp, nodep, "Struct/union dtype not under module/package/$unit");
        VL_RESTORER(m_curSymp);
        m_curSymp = m_statep->ঢোকাও_Block(m_curSymp, "__Vdtype" + cvtToStr(nodep->unique_অঙ্ক()),
                                          nodep, m_classOrPackagep);
        iterateChildren(nodep);
    }
    শূন্য visit(AstMemberDType* nodep) override {  // FindVisitor::
        iterateChildren(nodep);
        m_statep->ঢোকাও_Sym(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
    }
    শূন্য visit(AstParamTypeDType* nodep) override {  // FindVisitor::
        UASSERT_OBJ(m_curSymp, nodep, "Parameter type not under module/package/$unit");

        // Replace missing param types with provided hierarchical type params.
        যদি (!m_hierParamsName.empty()) {
            যদি (ধ্রুব VSymEnt* ধ্রুব typedefEntp = m_curSymp->findIdFallback(m_hierParamsName)) {
                ধ্রুব AstModule* modp = VN_CAST(typedefEntp->nodep(), Module);

                প্রতি (ধ্রুব Ast_গ্রন্থি* stmtp = modp ? modp->stmtsp() : nullptr; stmtp;
                     stmtp = stmtp->nextp()) {
                    ধ্রুব AstTypedef* tdefp = VN_CAST(stmtp, Typedef);

                    যদি (tdefp && tdefp->name() == nodep->name() && m_statep->forPrimary()) {
                        UINFO(8, "Replacing type of" << nodep << "  -->with " << tdefp);
                        Ast_গ্রন্থি_DType* ধ্রুব newDtp = tdefp->childDTypep();
                        Ast_গ্রন্থি_DType* ধ্রুব oldDtp = nodep->childDTypep();

                        oldDtp->replaceWith(newDtp->clone_বৃক্ষ(অসত্য));
                        VL_DO_DANGLING(oldDtp->delete_বৃক্ষ(), oldDtp);
                    }
                }
            }
        }

        iterateChildren(nodep);
        m_statep->ঢোকাও_Sym(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
        যদি (m_statep->forPrimary() && nodep->isGParam()) {
            ++m_param_অঙ্ক;
            VSymEnt* ধ্রুব symp = m_statep->ঢোকাও_Sym(
                m_curSymp, "__paramNumber" + cvtToStr(m_param_অঙ্ক), nodep, m_classOrPackagep);
            symp->exported(অসত্য);
        }
    }
    শূন্য visit(AstCFunc* nodep) override {  // FindVisitor::
        // For dotted resolution, ignore all AstVars under functions, otherwise shouldn't exist
        UASSERT_OBJ(!m_statep->forScopeCreation(), nodep, "No CFuncs expected in tree yet");
    }
    শূন্য visit(AstEnumItem* nodep) override {  // FindVisitor::
        // EnumItem: Remember its name for later resolution
        iterateChildren(nodep);
        // Find under either a task or the module's vars
        ধ্রুব VSymEnt* foundp = m_curSymp->findIdFallback(nodep->name());
        যদি (!foundp && m_modSymp && nodep->name() == m_modSymp->nodep()->name()) {
            foundp = m_modSymp;  // Conflicts with modname?
        }
        AstEnumItem* ধ্রুব findvarp = foundp ? VN_CAST(foundp->nodep(), EnumItem) : nullptr;
        তার্কিক ins = অসত্য;
        যদি (!foundp) {
            ins = সত্য;
        } অন্যথা যদি (findvarp != nodep) {
            UINFO(4, "DupVar: " << nodep << " ;; " << foundp);
            যদি (foundp->parentp() == m_curSymp  // Only when on same level
                && !foundp->imported()) {  // and not from package
                nodep->v3error("Duplicate declaration of enum value: "
                               << nodep->prettyName() << '\n'
                               << nodep->warnContextPrimary() << '\n'
                               << foundp->nodep()->warnOther()
                               << "... Location of original declaration\n"
                               << foundp->nodep()->warnContextSecondary());
            } অন্যথা {
                // User can disable the message at either point
                যদি (!nodep->fileline()->warnIsOff(V3ErrorCode::VARHIDDEN)
                    && !foundp->nodep()->fileline()->warnIsOff(V3ErrorCode::VARHIDDEN)) {
                    nodep->v3warn(VARHIDDEN,
                                  "Declaration of enum value hides declaration in upper scope: "
                                      << nodep->prettyName() << '\n'
                                      << nodep->warnContextPrimary() << '\n'
                                      << foundp->nodep()->warnOther()
                                      << "... Location of original declaration\n"
                                      << foundp->nodep()->warnContextSecondary());
                }
                ins = সত্য;
            }
        }
        যদি (ins) m_statep->ঢোকাও_Sym(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
    }
    শূন্য visit(AstPackageImport* nodep) override {  // FindVisitor::
        UINFO(4, "  Link: " << nodep);
        যদি (!nodep->packagep()) প্রত্যাগম;  // Errored in V3LinkCells
        VSymEnt* ধ্রুব srcp = m_statep->get_গ্রন্থি_Sym(nodep->packagep());
        যদি (nodep->name() == "*") {
            যদি (nodep->packagep() != v3Global.rootp()->stdPackagep()) {
                যদি (m_curSymp == m_statep->dunitEntp()) {
                    nodep->v3warn(IMPORTSTAR,
                                  "'import::*' in $unit scope may pollute global namespace");
                }
            }
        } অন্যথা যদি (!nodep->resolvedClassp()) {
            VSymEnt* ধ্রুব impp = srcp->findIdFlat(nodep->name());
            যদি (!impp) {
                nodep->v3error("Import object not found: " << nodep->prettyPkgNameQ());
            } অন্যথা যদি (nodep->resolvedClassp()) {
                UASSERT_OBJ(nodep->resolvedClassp()->classOrPackageSkipp() == impp->nodep(), nodep,
                            "Import should resolve to the same node");
            } অন্যথা যদি (AstClass* ধ্রুব classp = VN_CAST(impp->nodep(), Class)) {
                nodep->resolvedClassp(
                    নূতন AstClassOrPackageRef{nodep->fileline(), classp->name(), classp, nullptr});
            }
        }
        m_curSymp->importFromPackage(m_statep->symsp(), srcp, nodep->name());
        UINFO(9, "    Link Done: " << nodep);
        // No longer needed, but can't delete until any multi-instantiated modules are expanded
    }
    শূন্য visit(AstPackageExport* nodep) override {  // FindVisitor::
        UINFO(9, "  Link: " << nodep);
        যদি (!nodep->packagep()) প্রত্যাগম;  // Errored in V3LinkCells
        VSymEnt* ধ্রুব srcp = m_statep->get_গ্রন্থি_Sym(nodep->packagep());
        যদি (nodep->name() != "*") {
            VSymEnt* ধ্রুব impp = srcp->findIdFlat(nodep->name());
            যদি (!impp) {
                nodep->v3error("Export object not found: '" << nodep->packagep()->prettyName()
                                                            << "::" << nodep->prettyName() << "'");
            }
        }
        m_curSymp->exportFromPackage(m_statep->symsp(), srcp, nodep->name());
        UINFO(9, "    Link Done: " << nodep);
        // No longer needed, but can't delete until any multi-instantiated modules are expanded
    }
    শূন্য visit(AstPackageExportStarStar* nodep) override {  // FindVisitor::
        UINFO(4, "  Link: " << nodep);
        m_curSymp->exportStarStar(m_statep->symsp());
        // No longer needed, but can't delete until any multi-instantiated modules are expanded
    }

    শূন্য visit(Ast_গ্রন্থি_Foreach* nodep) override {  // FindVisitor::
        // Symbol table needs nodep->name() as the index variable's name
        VL_RESTORER(m_curSymp);
        {
            ++m_mod_With_অঙ্ক;
            m_curSymp = m_statep->ঢোকাও_Block(m_curSymp, "__Vforeach" + cvtToStr(m_mod_With_অঙ্ক),
                                              nodep, m_classOrPackagep);
            m_curSymp->fallbackp(VL_RESTORER_PREV(m_curSymp));
            প্রতি (Ast_গ্রন্থি *nextp, *argp = nodep->headerp()->elementsp(); argp; argp = nextp) {
                nextp = argp->nextp();
                AstVar* argrefp = nullptr;
                যদি (AstParseRef* ধ্রুব parserefp = VN_CAST(argp, ParseRef)) {
                    // IEEE 1800-2023 12.7.3: foreach loop variable type shall be int (2-state)
                    // This might get changed in V3Width when types resolve (e.g., for assoc
                    // arrays)
                    argrefp = নূতন AstVar{parserefp->fileline(), VVarType::BLOCKTEMP,
                                         parserefp->name(), argp->findIntDType()};
                    argrefp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                    parserefp->replaceWith(argrefp);
                    VL_DO_DANGLING2(parserefp->delete_বৃক্ষ(), parserefp, argp);
                    // Insert argref's name into symbol table
                    m_statep->ঢোকাও_Sym(m_curSymp, argrefp->name(), argrefp, nullptr);
                } অন্যথা যদি (ধ্রুব auto largrefp = VN_CAST(argp, Var)) {
                    argrefp = largrefp;
                    // Insert argref's name into symbol table
                    m_statep->ঢোকাও_Sym(m_curSymp, argrefp->name(), argrefp, nullptr);
                } অন্যথা যদি (VN_IS(argp, Empty)) {
                } অন্যথা {
                    argp->v3error("'foreach' loop variable expects simple variable name");
                }
            }
            iterateChildren(nodep);
        }
    }

    শূন্য visit(AstRandSequence* nodep) override {
        VL_RESTORER(m_curSymp);
        m_curSymp = m_statep->ঢোকাও_Block(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
        iterateChildren(nodep);
    }
    শূন্য visit(AstRSProd* nodep) override {
        VL_RESTORER(m_curSymp);
        m_curSymp = m_statep->ঢোকাও_Block(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
        যদি (nodep->fvarp())
            nodep->fvarp()->v3warn(E_UNSUPPORTED,
                                   "Unsupported: randsequence production function variable");
        // Mark formal ports as port-checked so the primary resolve pass does not
        // flag them with "does not appear in port list" -- V3RandSequence will
        // later move them onto the generated task as real input ports.
        প্রতি (Ast_গ্রন্থি* itp = nodep->portsp(); itp; itp = itp->nextp()) {
            VN_AS(itp, Var)->user4(সত্য);
        }
        iterateChildren(nodep);
    }

    শূন্য visit(AstWithParse* nodep) override {  // FindVisitor::
        // Change WITHPARSE(FUNCREF, equation) to FUNCREF(WITH(equation))
        Ast_গ্রন্থি_FTask_Ref* funcrefp = VN_CAST(nodep->funcrefp(), গ্রন্থি_FTask_Ref);
        যদি (ধ্রুব AstDot* ধ্রুব dotp = VN_CAST(nodep->funcrefp(), Dot))
            funcrefp = VN_CAST(dotp->rhsp(), গ্রন্থি_FTask_Ref);
        UASSERT_OBJ(funcrefp, nodep, "'with' only can operate on a function/task");
        string name = funcrefp->name() == "randomize" ? "__Vrandwith_obj" : "item";
        File_সারি* argFl = nodep->fileline();
        AstArg* ধ্রুব argsp = funcrefp->argsp();
        যদি (argsp) {
            argsp->unlinkFrBackWithNext();
            যদি (funcrefp->name() != "randomize") {
                যদি (ধ্রুব auto parserefp = VN_CAST(argsp->exprp(), ParseRef)) {
                    name = parserefp->name();
                    argFl = parserefp->fileline();
                } অন্যথা {
                    argsp->v3error("'with' function expects simple variable name");
                }
                যদি (argsp->nextp()) {
                    argsp->nextp()->v3error("'with' function expects only up to one argument");
                }
                VL_DO_DANGLING(argsp->delete_বৃক্ষ(), argsp);
            }
        }
        // Type depends on the method used, let V3Width figure it out later
        // 'with (...) { ... }' constraint_block form is randomize-only
        // (IEEE 1800-2023 7.12 vs. 18.7); array methods take 'with (expr)'.
        যদি (nodep->restricted() && funcrefp->name() != "randomize") {
            nodep->v3error("'with (...) { ... }' constraint block is only valid for"
                           " randomize() (IEEE 1800-2023 18.7)");
            funcrefp->addArgsp(argsp);
            nodep->replaceWith(nodep->funcrefp()->unlinkFrBack());
            VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
            প্রত্যাগম;
        }
        ধ্রুব তার্কিক restrictedRandomize = nodep->restricted();
        যদি (nodep->exprsp() || nodep->constraintsp()
            || restrictedRandomize) {  // Else empty expression and pretend no "with"
            AstLambdaArgRef* ধ্রুব indexArgRefp
                = নূতন AstLambdaArgRef{argFl, name + "__DOT__index", সত্য};
            AstLambdaArgRef* ধ্রুব মান_Arg_Refp = নূতন AstLambdaArgRef{argFl, name, অসত্য};
            AstWith* ধ্রুব newp
                = নূতন AstWith{nodep->fileline(), indexArgRefp, মান_Arg_Refp, nullptr};
            // Harvest the identifier_list into AstWith; grammar guarantees AstParseRef.
            যদি (restrictedRandomize) {
                newp->restricted(সত্য);
                যাবত্ (Ast_গ্রন্থি* ধ্রুব itemp = nodep->exprsp()) {
                    newp->addRestrictedName(VN_AS(itemp, ParseRef)->name());
                    itemp->unlinkFrBack();
                    VL_DO_DANGLING(ঠেলো_Deletep(itemp), itemp);
                }
            }
            Ast_গ্রন্থি* রাশিমালা_Or_Constraintsp = nullptr;
            যদি (nodep->exprsp()) রাশিমালা_Or_Constraintsp = nodep->exprsp()->unlinkFrBackWithNext();
            যদি (nodep->constraintsp())
                রাশিমালা_Or_Constraintsp = Ast_গ্রন্থি::addNext(
                    রাশিমালা_Or_Constraintsp, nodep->constraintsp()->unlinkFrBackWithNext());
            newp->addExprp(রাশিমালা_Or_Constraintsp);  // addExprp() tolerates nullptr
            funcrefp->withp(newp);
        }
        funcrefp->addArgsp(argsp);
        nodep->replaceWith(nodep->funcrefp()->unlinkFrBack());
        VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
    }
    শূন্য visit(AstWith* nodep) override {  // FindVisitor::
        // Symbol table needs nodep->name() as the index variable's name
        // Iteration will pickup the AstVar we made under AstWith
        VL_RESTORER(m_curSymp);
        ++m_mod_With_অঙ্ক;
        m_curSymp = m_statep->ঢোকাও_Block(m_curSymp, "__Vwith" + cvtToStr(m_mod_With_অঙ্ক), nodep,
                                          m_classOrPackagep);
        m_curSymp->fallbackp(VL_RESTORER_PREV(m_curSymp));
        UASSERT_OBJ(nodep->indexArgRefp(), nodep, "Missing lambda argref");
        UASSERT_OBJ(nodep->মান_Arg_Refp(), nodep, "Missing lambda argref");
        // Insert argref's name into symbol table
        m_statep->ঢোকাও_Sym(m_curSymp, nodep->মান_Arg_Refp()->name(), nodep->মান_Arg_Refp(),
                            nullptr);
        iterateChildren(nodep);
    }
    শূন্য visit(AstCoverWith* nodep) override {  // FindVisitor::
        // The range list is outside the scope of the filter, which declares 'item'
        iterateAndNextNull(nodep->subp());
        VL_RESTORER(m_curSymp);
        ++m_mod_With_অঙ্ক;
        m_curSymp = m_statep->ঢোকাও_Block(m_curSymp, "__VcoverWith" + cvtToStr(m_mod_With_অঙ্ক),
                                          nodep, m_classOrPackagep);
        m_curSymp->fallbackp(VL_RESTORER_PREV(m_curSymp));
        iterateAndNextNull(nodep->itemp());
        iterateAndNextNull(nodep->filterp());
    }
    শূন্য visit(AstCoverCrossWith* nodep) override {  // FindVisitor::
        // The selection, the 'matches' count, and the cross items' types, of their coverpoints'
        // expressions with any nested scopes, are outside the filter's scope, which declares the
        // items.  As visit(AstVar) would resolve a type in the scope declaring its variable, the
        // items, generated and unique, are inserted directly.
        iterateAndNextNull(nodep->selectp());
        iterateAndNextNull(nodep->countp());
        প্রতি (AstVar* varp = nodep->varsp(); varp; varp = VN_AS(varp->nextp(), Var)) {
            iterateAndNextNull(varp->childDTypep());
        }
        VL_RESTORER(m_curSymp);
        ++m_mod_With_অঙ্ক;
        m_curSymp = m_statep->ঢোকাও_Block(m_curSymp, "__VcoverWith" + cvtToStr(m_mod_With_অঙ্ক),
                                          nodep, m_classOrPackagep);
        m_curSymp->fallbackp(VL_RESTORER_PREV(m_curSymp));
        প্রতি (AstVar* varp = nodep->varsp(); varp; varp = VN_AS(varp->nextp(), Var)) {
            m_statep->ঢোকাও_Sym(m_curSymp, varp->name(), varp, m_classOrPackagep);
        }
        iterateAndNextNull(nodep->filterp());
    }

    শূন্য visit(Ast_গ্রন্থি* nodep) override { iterateChildren(nodep); }  // FindVisitor::

    শূন্য handleUnvisitedVirtIfaces() {
        Ast_গ্রন্থি_Module* ধ্রুব শীর্ষ = v3Global.rootp()->modulesp();
        m_curSymp = m_statep->get_গ্রন্থি_Sym(শীর্ষ);
        প্রতি (AstIface* ifacep : m_virtIfaces) {
            যদি (!ifacep->user4()) {  // virtual interface never assigned any actual interface
                // Create dummy cell to keep the virtual interface alive
                // (later stages assume that non-dead interface has associated cell)
                AstCell* ধ্রুব ifacecellp = নূতন AstCell{ifacep->fileline(),
                                                        ifacep->fileline(),
                                                        "__VdummyInstOf" + ifacep->name(),
                                                        ifacep->name(),
                                                        nullptr,
                                                        nullptr,
                                                        nullptr};
                ifacecellp->modp(ifacep);
                শীর্ষ->addStmtsp(ifacecellp);

                iterate(ifacecellp);
            }
        }
    }

সার্বজনিক:
    // CONSTRUCTORS
    LinkDotFindVisitor(AstNetlist* rootp, LinkDotState* statep)
        : m_statep{statep} {
        UINFO(4, __FUNCTION__ << ": ");
        iterate(rootp);

        যদি (!m_virtIfaces.empty()) handleUnvisitedVirtIfaces();
        প্রতি (AstScope* ধ্রুব scopep : m_scopes) scopep->modp()->addStmtsp(scopep);
        যদি (m_statep->forScopeCreation()) {
            V3Stats::addStat("LinkDot, Scope visits", m_statScopeVisits);
        }
    }
    ~LinkDotFindVisitor() override = স্বভাব;
};

//======================================================================

বর্গ LinkDotFindIfaceVisitor final : সার্বজনিক VNVisitor {
    // NODE STATE
    //  *::user1p()             -> See LinkDotState

    // STATE - for current visit position (use VL_RESTORER)
    LinkDotState* ধ্রুব m_statep;  // State to pass between visitors, including symbol table
    Ast_গ্রন্থি* m_declp = nullptr;  // Current declaring object that may soon contain IfaceRefDType

    // METHODS
    ধ্রুব VSymEnt* findNonDeclSym(VSymEnt* symp, ধ্রুব string& name) {
        // Find if there is a symbol of given name, ignoring the node that is declaring
        // it (m_declp) itself.  Thus if searching for "ifc" (an interface):
        //
        // module x;  // Finishes here and finds the typedef
        //   typedef foo ifc;
        //   ifc ifc;   // symp starts pointing here, but matches m_declp
        যাবত্ (symp) {
            ধ্রুব VSymEnt* ধ্রুব foundp = symp->findIdFlat(name);
            যদি (foundp && foundp->nodep() != m_declp) প্রত্যাগম foundp;
            symp = symp->fallbackp();
        }
        প্রত্যাগম nullptr;
    }

    // VISITORS
    শূন্য visit(AstRefDType* nodep) override {  // FindIfaceVisitor::
        যদি (m_statep->forPrimary() && !nodep->classOrPackagep()) {
            UINFO(9, "  FindIfc: " << nodep);
            // If under a var, ignore the var itself as might be e.g. "intf intf;"
            // Critical tests:
            //   t_interface_param_genblk.v  // Checks this does make interface
            //   t_interface_hidden.v  // Checks this doesn't making interface when hidden
            যদি (m_statep->exists_গ্রন্থি_Sym(nodep)) {
                VSymEnt* symp = m_statep->get_গ্রন্থি_Sym(nodep);
                ধ্রুব VSymEnt* foundp = findNonDeclSym(symp, nodep->name());
                Ast_গ্রন্থি* foundNodep = nullptr;
                // This: v4make test_regress/t/t_interface_param_genblk.py --debug
                // --debugi-V3LinkDot 9 Passes with this commented out:
                যদি (foundp) foundNodep = foundp->nodep();
                যদি (!foundNodep) foundNodep = m_statep->findModuleSym(nodep->name());
                যদি (foundNodep) UINFO(9, "    Ifc foundNodep " << foundNodep);
                যদি (AstIface* ধ্রুব defp = VN_CAST(foundNodep, Iface)) {
                    // Must be found as module name, and not hidden/ by normal symbol (foundp)
                    AstIfaceRefDType* ধ্রুব newp
                        = নূতন AstIfaceRefDType{nodep->fileline(), "", nodep->name()};
                    যদি (nodep->paramsp())
                        newp->addParamsp(nodep->paramsp()->unlinkFrBackWithNext());
                    newp->ifacep(defp);
                    newp->user1u(nodep->user1u());
                    UINFO(9, "    Resolved interface " << nodep << "  =>  " << defp);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                    প্রত্যাগম;
                }
            }
        }
        iterateChildren(nodep);
    }
    শূন্য visit(AstVar* nodep) override {  // FindIfaceVisitor::
        VL_RESTORER(m_declp);
        m_declp = nodep;
        iterateChildren(nodep);
        AstIfaceRefDType* ধ্রুব ifacerefp = LinkDotState::ifaceRefFromArray(nodep->subDTypep());
        যদি (ifacerefp && m_statep->exists_গ্রন্থি_Sym(nodep)) {
            // Can't resolve until interfaces and modport names are
            // known; see notes at top
            UINFO(9, "  FindIfc Var IfaceRef " << ifacerefp);
            যদি (!ifacerefp->isVirtual()) nodep->setIfaceRef();
            VSymEnt* ধ্রুব symp = m_statep->get_গ্রন্থি_Sym(nodep);
            m_statep->ঢোকাও_Iface_Var_Sym(symp);
        }
    }
    শূন্য visit(Ast_গ্রন্থি* nodep) override { iterateChildren(nodep); }  // FindIfaceVisitor::

সার্বজনিক:
    // CONSTRUCTORS
    LinkDotFindIfaceVisitor(AstNetlist* rootp, LinkDotState* statep)
        : m_statep{statep} {
        UINFO(4, __FUNCTION__ << ": ");
        iterate(rootp);
    }
    ~LinkDotFindIfaceVisitor() override = স্বভাব;
};

//======================================================================

বর্গ LinkDotParamVisitor final : সার্বজনিক VNVisitor {
    // NODE STATE
    //  *::user1p()             -> See LinkDotState
    //  *::user2p()             -> See LinkDotState
    //  *::user4()              -> See LinkDotState

    // STATE
    LinkDotState* ধ্রুব m_statep;  // State to pass between visitors, including symbol table
    Ast_গ্রন্থি_Module* m_modp = nullptr;  // Current module

    শূন্য pin_Implicit_রাশিমালা_Recurse(Ast_গ্রন্থি* nodep) {
        // Under a pin, Check interconnect expression for a pin reference or a concat.
        // Create implicit variable as needed
        যদি (VN_IS(nodep, Dot)) {  // Not creating a simple implied type,
            // and implying something else would just confuse later errors
        } অন্যথা যদি (VN_IS(nodep, VarRef) || VN_IS(nodep, ParseRef)) {
            // To prevent user errors, we should only do single bit
            // implicit vars, however some netlists (MIPS) expect single
            // bit implicit wires to get created with range 0:0 etc.
            m_statep->implicitOkAdd(m_modp, nodep->name());
        }
        // These are perhaps a little too generous, as a SELect of siga[sigb]
        // perhaps shouldn't create an implicit variable.  But, we've warned...
        অন্যথা {
            যদি (Ast_গ্রন্থি* ধ্রুব refp = nodep->op1p()) pin_Implicit_রাশিমালা_Recurse(refp);
            যদি (Ast_গ্রন্থি* ধ্রুব refp = nodep->op2p()) pin_Implicit_রাশিমালা_Recurse(refp);
            যদি (Ast_গ্রন্থি* ধ্রুব refp = nodep->op3p()) pin_Implicit_রাশিমালা_Recurse(refp);
            যদি (Ast_গ্রন্থি* ধ্রুব refp = nodep->op4p()) pin_Implicit_রাশিমালা_Recurse(refp);
        }
        // Continue along a list (e.g. the terminals under AstImplicit), also after a reference
        যদি (Ast_গ্রন্থি* ধ্রুব refp = nodep->nextp()) pin_Implicit_রাশিমালা_Recurse(refp);
    }

    // VISITORS
    শূন্য visit(AstTypeTable*) override {}  // ParamVisitor::
    শূন্য visit(Ast_গ্রন্থি_Module* nodep) override {  // ParamVisitor::
        UINFO(5, "   " << nodep);
        যদি ((nodep->dead() || !nodep->user4()) && !nodep->hierParams()) {
            UINFO(4, "Mark dead module " << nodep);
            UASSERT_OBJ(!m_statep->forScopeCreation(), nodep,
                        "Dead module persisted past where should have removed");
            // Don't remove now, because we may have a tree of
            // parameterized modules with VARXREFs into the deleted module
            // region.  V3Dead should cleanup.
            // Downstream visitors up until V3Dead need to check for nodep->dead.
            nodep->dead(সত্য);
            প্রত্যাগম;
        }
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
    }
    শূন্য visit(AstPin* nodep) override {  // ParamVisitor::
        // Pin: Link to submodule's port
        // Deal with implicit definitions - do before Resolve visitor as may
        // be referenced above declaration
        যদি (nodep->exprp()  // Else specifically unconnected pin
            && !nodep->svImplicit()) {  // SV 19.11.3: .name pins don't allow implicit decls
            pin_Implicit_রাশিমালা_Recurse(nodep->exprp());
        }
    }
    শূন্য visit(AstDefParam* nodep) override {  // ParamVisitor::
        iterateChildren(nodep);
        nodep->v3warn(DEFPARAM, "defparam is deprecated (IEEE 1800-2023 C.4.1)\n"
                                    << nodep->warnMore()
                                    << "... Suggest use instantiation with #(."
                                    << nodep->prettyName() << "(...etc...))");
        তার্কিক hasPartSelect = অসত্য;
        ধ্রুব string path = extractDottedPath(nodep->pathp(), hasPartSelect);
        UASSERT_OBJ(!hasPartSelect && !path.empty(), nodep, "Unexpected defparam path shape");
        string baddot;
        VSymEnt* okSymp = nullptr;
        VSymEnt* ধ্রুব foundp = m_statep->findDotted(
            nodep->fileline(), m_statep->get_গ্রন্থি_Sym(nodep), path, baddot, okSymp, সত্য);
        AstCell* ধ্রুব cellp = foundp ? VN_AS(foundp->nodep(), Cell) : nullptr;
        যদি (!cellp) {
            nodep->v3error("In defparam, instance " << path << " never declared");
        } অন্যথা {
            Ast_গ্রন্থি_রাশিমালা* ধ্রুব exprp = nodep->rhsp()->unlinkFrBack();
            UINFO(9, "Defparam cell " << path << "." << nodep->name() << " attach-to " << cellp
                                      << "  <= " << exprp);
            // Don't need to check the name of the defparam exists.  V3Param does.
            AstPin* ধ্রুব pinp = নূতন AstPin{nodep->fileline(),
                                            -1,  // Pin# not relevant
                                            nodep->name(), exprp};
            pinp->param(সত্য);
            pinp->paramPath(path);
            cellp->addParamsp(pinp);
        }
        VL_DO_DANGLING(nodep->unlinkFrBack()->delete_বৃক্ষ(), nodep);
    }
    শূন্য visit(AstPort* nodep) override {  // ParamVisitor::
        // Port: Stash the pin number
        // Need to set pin numbers after varnames are created
        // But before we do the final resolution based on names
        VSymEnt* ধ্রুব foundp = m_statep->get_গ্রন্থি_Sym(m_modp)->findIdFlat(nodep->name());
        AstVar* ধ্রুব refp = foundp ? VN_CAST(foundp->nodep(), Var) : nullptr;
        যদি (!foundp) {
            nodep->v3error(
                "Input/output/inout declaration not found for port: " << nodep->prettyNameQ());
        } অন্যথা যদি (!refp || (!refp->isIO() && !refp->isIfaceRef())) {
            nodep->v3error("Pin is not an in/out/inout/interface: " << nodep->prettyNameQ());
        } অন্যথা {
            যদি (refp->user4()) {
                nodep->v3error("Duplicate declaration of port: "
                               << nodep->prettyNameQ() << '\n'
                               << nodep->warnContextPrimary() << '\n'
                               << refp->warnOther() << "... Location of original declaration\n"
                               << refp->warnContextSecondary());
            }
            refp->user4(সত্য);
            VSymEnt* ধ্রুব symp = m_statep->ঢোকাও_Sym(m_statep->get_গ্রন্থি_Sym(m_modp),
                                                      "__pinNumber" + cvtToStr(nodep->pin_অঙ্ক()),
                                                      refp, nullptr /*classOrPackagep*/);
            symp->exported(অসত্য);
            refp->pin_অঙ্ক(nodep->pin_অঙ্ক());
            // Put the variable where the port is, so that variables stay
            // in pin number sorted order. Otherwise hierarchical or JSON
            // may botch by-position instances.
            nodep->addHereThisAsNext(refp->unlinkFrBack());
        }
        // Ports not needed any more
        VL_DO_DANGLING(nodep->unlinkFrBack()->delete_বৃক্ষ(), nodep);
    }
    শূন্য visit(AstAssignW* nodep) override {  // ParamVisitor::
        // Deal with implicit definitions
        // We used to nodep->allowImplicit() here, but it turns out
        // normal "assigns" can also make implicit wires.  Yuk.
        pin_Implicit_রাশিমালা_Recurse(nodep->lhsp());
        iterateChildren(nodep);
    }

    শূন্য visit(AstImplicit* nodep) override {  // ParamVisitor::
        // Unsupported gates need implicit creation
        pin_Implicit_রাশিমালা_Recurse(nodep->exprsp());
        // We're done with implicit gates
        VL_DO_DANGLING(nodep->unlinkFrBack()->delete_বৃক্ষ(), nodep);
    }
    শূন্য visit(AstClassOrPackageRef* nodep) override {  // ParamVisitor::
        যদি (auto* ধ্রুব fwdp = VN_CAST(nodep->classOrPackageNodep(), TypedefFwd)) {
            // Relink forward definitions to the "real" definition
            VSymEnt* ধ্রুব foundp = m_statep->get_গ্রন্থি_Sym(fwdp)->findIdFallback(fwdp->name());
            যদি (foundp && (VN_IS(foundp->nodep(), Class) || VN_IS(foundp->nodep(), Package))) {
                nodep->classOrPackagep(VN_AS(foundp->nodep(), গ্রন্থি_Module));
            } অন্যথা যদি (foundp && VN_IS(foundp->nodep(), ParamTypeDType)) {
                UASSERT(m_statep->forPrimary(), "Param types should have been resolved");
                nodep->classOrPackageNodep(VN_AS(foundp->nodep(), ParamTypeDType));
            } অন্যথা {
                যদি (foundp) UINFO(1, "found nodep = " << foundp->nodep());
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
    শূন্য visit(AstPull* nodep) override {  // ParamVisitor::
        // Deal with implicit definitions
        // We used to nodep->allowImplicit() here, but it turns out
        // normal "assigns" can also make implicit wires.  Yuk.
        pin_Implicit_রাশিমালা_Recurse(nodep->lhsp());
        iterateChildren(nodep);
    }
    শূন্য visit(AstTypedefFwd* nodep) override {  // ParamVisitor::
        VSymEnt* ধ্রুব foundp = m_statep->get_গ্রন্থি_Sym(nodep)->findIdFallback(nodep->name());
        যদি (foundp) {
            // If the typedef was earlier in source order (tokenNum), then remember that earlier
            // point to avoid error something wasn't declared
            // Might be forward declaring something odd (with declTokenNumSetMin not implemented,
            // but should be harmless to ignore as this is just for error detection
            foundp->nodep()->decl_Token_অঙ্ক_Set_Min(nodep->fileline()->token_অঙ্ক());
        } অন্যথা যদি (v3Global.opt.pedantic()) {
            // We only check it was ever really defined in pedantic mode, as it
            // might have been in a header file referring to a module we never
            // needed so there are false positives
            nodep->v3error(
                "Forward typedef unused or does not resolve to a data type (IEEE 1800-2023 6.18): "
                << nodep->prettyNameQ());
        }
        // We only needed the forward declaration in order to parse correctly.
        // Delete later as may be ClassOrPackageRef's still pointing to it
        VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);
    }

    শূন্য visit(Ast_গ্রন্থি* nodep) override { iterateChildren(nodep); }  // ParamVisitor::

সার্বজনিক:
    // CONSTRUCTORS
    LinkDotParamVisitor(AstNetlist* rootp, LinkDotState* statep)
        : m_statep{statep} {
        UINFO(4, __FUNCTION__ << ": ");
        iterate(rootp);
    }
    ~LinkDotParamVisitor() override = স্বভাব;
};

//======================================================================

বর্গ LinkDotScopeVisitor final : সার্বজনিক VNVisitor {

    // STATE
    LinkDotState* ধ্রুব m_statep;  // State to pass between visitors, including symbol table
    ধ্রুব AstScope* m_scopep = nullptr;  // The current scope
    VSymEnt* m_modSymp = nullptr;  // Symbol entry for current module
    // Deferred AliasScope processing - must be done outer-to-inner for correct alias resolution
    std::vector<std::pair<AstAliasScope*, VSymEnt*>> m_deferredAliasScopes;

    // METHODS
সার্বজনিক:
    // getAlias with setVarAlias and setIfaceAlias implement disjoint-set data structure.
    // This algorithm is needed for the case when multiple alias statements
    // reference partially the same variables.
    স্থির AstVarScope* getAlias(AstVarScope* ধ্রুব vscp) {
        যদি (vscp->user2p() && vscp != vscp->user2p()) {
            AstVarScope* ধ্রুব aliasp = getAlias(VN_AS(vscp->user2p(), VarScope));
            vscp->user2p(aliasp);
            প্রত্যাগম aliasp;
        } অন্যথা {
            প্রত্যাগম vscp;
        }
    }

গুপ্ত:
    // Alias an interface reference to the interface it is connected to
    শূন্য setIfaceAlias(AstVarScope* ধ্রুব vscp, AstVarScope* ধ্রুব aliasp) {
        UASSERT_OBJ(vscp->varp()->isIfaceRef(), vscp, "Interface alias of non-interface");
        UASSERT_OBJ(aliasp->varp()->isIfaceRef(), aliasp, "Interface alias of non-interface");
        getAlias(vscp)->user2p(getAlias(aliasp));
    }

    // Alias two variables. An output port in the lower scope survives, unless the other
    // must keep its own storage.
    শূন্য setVarAlias(AstVarScope* ap, AstVarScope* bp) {
        UASSERT_OBJ(!ap->varp()->isIfaceRef(), ap, "Variable alias of interface");
        UASSERT_OBJ(!bp->varp()->isIfaceRef(), bp, "Variable alias of interface");
        ap = getAlias(ap);
        bp = getAlias(bp);
        যদি (ap == bp) প্রত্যাগম;
        ধ্রুব তার্কিক aWins = [&]() {
            ধ্রুব AstVar* ধ্রুব aVarp = ap->varp();
            ধ্রুব AstVar* ধ্রুব bVarp = bp->varp();
            // A primary IO is accessed directly by the user of the model, so must survive
            UASSERT_OBJ(!aVarp->isPrimaryIO() || !bVarp->isPrimaryIO(), ap,
                        "Alias of two primary IOs");
            যদি (bVarp->isPrimaryIO()) প্রত্যাগম অসত্য;
            যদি (aVarp->isPrimaryIO()) প্রত্যাগম সত্য;
            // A forced or public variable must keep its own storage. If both are, either works.
            যদি (aVarp->isForced() != bVarp->isForced()) প্রত্যাগম aVarp->isForced();
            যদি (aVarp->isSigPublic() != bVarp->isSigPublic()) প্রত্যাগম aVarp->isSigPublic();
            // Otherwise prefer an output port in the lower scope. This enables better V3Combine
            // due to having fewer upward hierarchical references (which prevent combining).
            যদি (aVarp->direction() != VDirection::OUTPUT) প্রত্যাগম অসত্য;
            // Is 'ap' strictly below 'bp' in the hierarchy
            প্রতি (ধ্রুব AstScope* scopep = ap->scopep()->aboveScopep(); scopep;
                 scopep = scopep->aboveScopep()) {
                যদি (scopep == bp->scopep()) প্রত্যাগম সত্য;
            }
            প্রত্যাগম অসত্য;
        }();
        যদি (aWins) {
            bp->user2p(ap);
        } অন্যথা {
            ap->user2p(bp);
        }
    }

    // VISITORS
    শূন্য visit(AstNetlist* nodep) override {  // ScopeVisitor::
        // Recurse..., backward as must do packages before using packages
        iterateChildrenBackwardsConst(nodep);
    }
    শূন্য visit(AstScope* nodep) override {  // ScopeVisitor::
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
        যদি (AstIface* ধ্রুব ifacep = VN_CAST(nodep->modp(), Iface)) {
            m_statep->ঢোকাও_Iface_Mod_Sym(ifacep, m_modSymp);
        }
        m_scopep = nodep;
        iterateChildren(nodep);
    }
    শূন্য visit(AstVarScope* nodep) override {  // ScopeVisitor::
        যদি (!nodep->varp()->isFuncLocal() && !nodep->varp()->isClassMember()) {
            VSymEnt* ধ্রুব varSymp
                = m_statep->ঢোকাও_Sym(m_modSymp, nodep->varp()->name(), nodep, nullptr);
            যদি (nodep->varp()->isIfaceRef()) {
                AstIfaceRefDType* ধ্রুব dtypep
                    = LinkDotState::ifaceRefFromArray(nodep->varp()->dtypep());
                UASSERT_OBJ(dtypep, nodep, "Non AstIfaceRefDType on isIfaceRef() var");
                যদি (nodep->varp()->isIfaceParent()) {
                    UINFO(9, "Iface parent ref var " << nodep->varp()->name() << " " << nodep);
                    // Find the interface cell the var references
                    UINFO(9, "Iface parent dtype " << dtypep);
                    ধ্রুব string ifcellname = dtypep->cellName();
                    string baddot;
                    VSymEnt* okSymp;
                    VSymEnt* cellSymp = m_statep->findDotted(nodep->fileline(), m_modSymp,
                                                             ifcellname, baddot, okSymp, অসত্য);
                    UASSERT_OBJ(
                        cellSymp, nodep,
                        "No symbol for interface instance: " << nodep->prettyNameQ(ifcellname));
                    UINFO(5, "       Found interface instance: se" << cvtToHex(cellSymp) << " "
                                                                   << cellSymp->nodep());
                    যদি (dtypep->modportName() != "") {
                        // Look up the modport within the interface cell's symbol table
                        VSymEnt* ধ্রুব mpSymp = cellSymp->findIdFallback(dtypep->modportName());
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
                    m_statep->ঢোকাও_Scope_Alias(LinkDotState::SAMN_IFTOP, varSymp, cellSymp);
                }
            }
        }
    }
    শূন্য visit(Ast_গ্রন্থি_FTask* nodep) override {  // ScopeVisitor::
        VSymEnt* ধ্রুব symp = m_statep->ঢোকাও_Block(m_modSymp, nodep->name(), nodep, nullptr);
        symp->fallbackp(m_modSymp);
        iterateChildren(nodep);
    }
    শূন্য visit(Ast_গ্রন্থি_Foreach* nodep) override {  // ScopeVisitor::
        VSymEnt* ধ্রুব symp = m_statep->ঢোকাও_Block(m_modSymp, nodep->name(), nodep, nullptr);
        symp->fallbackp(m_modSymp);
        // No recursion, we don't want to pick up variables
    }
    শূন্য visit(AstConstraintForeach* nodep) override {  // ScopeVisitor::
        iterateChildren(nodep);
    }
    শূন্য visit(AstWith* nodep) override {  // ScopeVisitor::
        VSymEnt* ধ্রুব symp = m_statep->ঢোকাও_Block(m_modSymp, nodep->name(), nodep, nullptr);
        symp->fallbackp(m_modSymp);
        iterateChildren(nodep);
    }
    শূন্য visit(AstAlias* nodep) override {  // ScopeVisitor::
        // Track aliases
        UINFOTREE(9, nodep, "", "alias");
        AstVarScope* aliasVscp = nullptr;
        প্রতি (Ast_গ্রন্থি* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
            AstVarScope* vscp = nullptr;
            যদি (ধ্রুব AstVarRef* ধ্রুব refp = VN_CAST(itemp, VarRef)) {
                vscp = refp->varScopep();
            } অন্যথা {
                // Reaches into the scope of an instance, look it up by name
                ধ্রুব AstVarXRef* ধ্রুব xrefp = VN_AS(itemp, VarXRef);
                ধ্রুব string scopename = xrefp->dotted() + "." + xrefp->name();
                string baddot;
                VSymEnt* okSymp;
                VSymEnt* ধ্রুব symp = m_statep->findDotted(xrefp->fileline(), m_modSymp, scopename,
                                                           baddot, okSymp, অসত্য);
                UASSERT_OBJ(symp, nodep, "No symbol for alias item: " << scopename);
                vscp = VN_CAST(symp->nodep(), VarScope);
            }
            UASSERT_OBJ(vscp, nodep, "VarScope unset");
            যদি (aliasVscp) {
                setVarAlias(aliasVscp, vscp);
            } অন্যথা {
                aliasVscp = vscp;
            }
        }
        iterateChildren(nodep);
        VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);
    }
    শূন্য visit(AstAliasScope* nodep) override {  // ScopeVisitor::
        // Defer AliasScope processing - must process outer scopes before inner ones
        // so that nested interface port alias resolution works correctly
        UINFO(5, "ALIASSCOPE (deferred) " << nodep);
        m_deferredAliasScopes.emplace_back(nodep, m_modSymp);
    }
    শূন্য processAliasScope(AstAliasScope* nodep, VSymEnt* modSymp) {
        UINFO(5, "ALIASSCOPE  " << nodep);
        UINFOTREE(9, nodep, "", "avs");
        VSymEnt* rhsSymp;
        {
            AstVarRef* ধ্রুব refp = VN_CAST(nodep->rhsp(), VarRef);
            AstVarXRef* ধ্রুব xrefp = VN_CAST(nodep->rhsp(), VarXRef);
            UASSERT_OBJ(refp || xrefp, nodep,
                        "Unsupported: Non Var(X)Ref attached to interface pin");
            string inl
                = ((xrefp && xrefp->inlinedDots().size()) ? (xrefp->inlinedDots() + "__DOT__")
                                                          : "");
            ধ্রুব string dottedPath
                = (xrefp && !xrefp->dotted().empty()) ? (xrefp->dotted() + ".") : "";
            VSymEnt* symp = nullptr;
            string scopename;
            যাবত্ (!symp) {
                scopename = refp ? refp->name()
                                 : (inl.size() ? (inl + dottedPath + xrefp->name())
                                               : (dottedPath + xrefp->name()));
                string baddot;
                VSymEnt* okSymp;
                symp = m_statep->findDotted(nodep->rhsp()->fileline(), modSymp, scopename, baddot,
                                            okSymp, সত্য);
                যদি (inl == "") ভঙ্গ;
                inl = LinkDotState::removeLastInlineScope(inl);
            }
            UASSERT_OBJ(symp, nodep, "No symbol for interface alias rhs");
            UINFO(5, "       Found a linked scope RHS: " << scopename << "  se" << cvtToHex(symp)
                                                         << " " << symp->nodep());
            rhsSymp = symp;
        }
        VSymEnt* lhsSymp;
        {
            ধ্রুব AstVarXRef* ধ্রুব xrefp = VN_CAST(nodep->lhsp(), VarXRef);
            ধ্রুব AstVarRef* ধ্রুব refp = VN_CAST(nodep->lhsp(), VarRef);

            UASSERT_OBJ(refp || xrefp, nodep,
                        "Unsupported: Non Var(X)Ref attached to interface pin");
            ধ্রুব string scopename
                = refp ? refp->varp()->name() : xrefp->dotted() + "." + xrefp->name();
            string baddot;
            VSymEnt* okSymp;
            VSymEnt* ধ্রুব symp = m_statep->findDotted(nodep->lhsp()->fileline(), modSymp,
                                                       scopename, baddot, okSymp, অসত্য);
            UASSERT_OBJ(symp, nodep, "No symbol for interface alias lhs");
            UINFO(5, "       Found a linked scope LHS: " << scopename << "  se" << cvtToHex(symp)
                                                         << " " << symp->nodep());
            lhsSymp = symp;
        }
        // Remember the alias - can't do it yet because we may have additional symbols to be added,
        // or maybe an alias of an alias
        m_statep->ঢোকাও_Scope_Alias(LinkDotState::SAMN_IFTOP, lhsSymp, rhsSymp);
        AstVarScope* ধ্রুব lhsVscp = VN_CAST(lhsSymp->nodep(), VarScope);
        AstVarScope* ধ্রুব rhsVscp = VN_CAST(rhsSymp->nodep(), VarScope);
        UASSERT_OBJ(lhsVscp && rhsVscp, nodep, "Interface alias missing variable scope");
        setIfaceAlias(lhsVscp, rhsVscp);
        // We have stored the link, we don't need these any more
        VL_DO_DANGLING(nodep->unlinkFrBack()->delete_বৃক্ষ(), nodep);
    }
    শূন্য processDeferredAliasScopes() {
        // Sort by hierarchy depth (shallower first) so outer aliases are resolved before inner
        // Pre-compute depth map to avoid O(N log N * D) complexity in sort comparisons
        std::unordered_map<VSymEnt*, পূর্ণাঙ্ক> depthMap;
        প্রতি (ধ্রুব auto& pair : m_deferredAliasScopes) {
            VSymEnt* ধ্রুব symp = pair.second;
            যদি (depthMap.find(symp) == depthMap.end()) {
                পূর্ণাঙ্ক depth = 0;
                প্রতি (VSymEnt* p = symp; p; p = p->parentp()) ++depth;
                depthMap[symp] = depth;
            }
        }
        std::stable_sort(m_deferredAliasScopes.begin(), m_deferredAliasScopes.end(),
                         [&depthMap](ধ্রুব std::pair<AstAliasScope*, VSymEnt*>& a,
                                     ধ্রুব std::pair<AstAliasScope*, VSymEnt*>& b) {
                             প্রত্যাগম depthMap.at(a.second) < depthMap.at(b.second);
                         });
        // Process in sorted order
        প্রতি (auto& pair : m_deferredAliasScopes) { processAliasScope(pair.first, pair.second); }
        m_deferredAliasScopes.clear();
    }
    শূন্য visit(Ast_গ্রন্থি_Gen* nodep) override {  // ScopeVisitor::  // LCOV_EXCL_LINE
        nodep->v3fatalSrc("Generate constructs should have been reduced out");
    }
    // For speed, don't recurse things that can't have scope
    // Note we allow AstNodeStmt's as generates may be under them
    শূন্য visit(AstCell*) override {}  // ScopeVisitor::
    শূন্য visit(AstVar*) override {}  // ScopeVisitor::
    শূন্য visit(Ast_গ্রন্থি* nodep) override { iterateChildren(nodep); }  // ScopeVisitor::

সার্বজনিক:
    // CONSTRUCTORS
    LinkDotScopeVisitor(AstNetlist* rootp, LinkDotState* statep)
        : m_statep{statep} {
        UINFO(4, __FUNCTION__ << ": ");
        iterate(rootp);
        // Process deferred AliasScopes in outer-to-inner order
        processDeferredAliasScopes();
    }
    ~LinkDotScopeVisitor() override = স্বভাব;
};

//======================================================================

// Iterate an interface to resolve modports
বর্গ LinkDotIfaceVisitor final : সার্বজনিক VNVisitor {
    // STATE
    LinkDotState* ধ্রুব m_statep;  // State to pass between visitors, including symbol table
    VSymEnt* m_curSymp;  // Symbol Entry for current table, where to lookup/insert

    // VISITORS
    শূন্য visit(AstModport* nodep) override {  // IfaceVisitor::
        // Modport: Remember its name for later resolution
        UINFO(5, "   fiv: " << nodep);
        VL_RESTORER(m_curSymp);
        {
            // Create symbol table for the vars
            m_curSymp = m_statep->ঢোকাও_Block(m_curSymp, nodep->name(), nodep, nullptr);
            m_curSymp->fallbackp(VL_RESTORER_PREV(m_curSymp));
            iterateChildren(nodep);
        }
    }
    শূন্য visit(AstModportFTaskRef* nodep) override {  // IfaceVisitor::
        UINFO(5, "   fif: " << nodep);
        iterateChildren(nodep);
        VSymEnt* ধ্রুব symp = m_curSymp->findIdFallback(nodep->name());
        যদি (!symp) {
            nodep->v3error("Modport item not found: " << nodep->prettyNameQ());
        } অন্যথা যদি (Ast_গ্রন্থি_FTask* ধ্রুব ftaskp = VN_CAST(symp->nodep(), গ্রন্থি_FTask)) {
            // Make symbol under modport that points at the _interface_'s var, not the modport.
            nodep->ftaskp(ftaskp);
            VSymEnt* ধ্রুব subSymp
                = m_statep->ঢোকাও_Sym(m_curSymp, nodep->name(), ftaskp, nullptr /*package*/);
            m_statep->ঢোকাও_Scope_Alias(LinkDotState::SAMN_MODPORT, subSymp, symp);
        } অন্যথা {
            nodep->v3error("Modport item is not a function/task: " << nodep->prettyNameQ());
        }
        যদি (m_statep->forScopeCreation()) {
            // Done with AstModportFTaskRef.
            // Delete to prevent problems if we dead-delete pointed to ftask
            nodep->unlinkFrBack();
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
        }
    }

    // Helper to resolve remaining path through a nested interface
    // When findDotted() partially matches (okSymp set, baddot non-empty),
    // this follows the interface type to resolve the remaining path.
    // Returns the resolved symbol, or nullptr if not found.
    // On success, clears baddot; on partial match of multi-level path, updates baddot.
    VSymEnt* resolveNestedInterfacePath(File_সারি* fl, VSymEnt* okSymp, string& baddot) {
        যদি (!okSymp || baddot.empty()) প্রত্যাগম nullptr;

        স্থির constexpr পূর্ণাঙ্ক MAX_NESTING_DEPTH = 64;
        VSymEnt* curOkSymp = okSymp;

        প্রতি (পূর্ণাঙ্ক depth = 0; depth < MAX_NESTING_DEPTH; ++depth) {
            // Try to get interface from the partially-matched symbol
            AstIface* ifacep = nullptr;
            যদি (ধ্রুব AstCell* ধ্রুব cellp = VN_CAST(curOkSymp->nodep(), Cell)) {
                ifacep = VN_CAST(cellp->modp(), Iface);
            } অন্যথা যদি (ধ্রুব AstVar* ধ্রুব varp = VN_CAST(curOkSymp->nodep(), Var)) {
                যদি (varp->isIfaceRef()) {
                    যদি (ধ্রুব AstIfaceRefDType* ধ্রুব ifaceRefp
                        = LinkDotState::ifaceRefFromArray(varp->dtypep())) {
                        ifacep = ifaceRefp->ifaceViaCellp();
                    }
                }
            }

            যদি (!ifacep || !m_statep->exists_গ্রন্থি_Sym(ifacep)) প্রত্যাগম nullptr;

            VSymEnt* ধ্রুব ifaceSymp = m_statep->get_গ্রন্থি_Sym(ifacep);

            যদি (baddot.find('.') == string::npos) {
                // Simple identifier - direct lookup
                VSymEnt* ধ্রুব symp = ifaceSymp->findIdFallback(baddot);
                যদি (symp) baddot.clear();
                প্রত্যাগম symp;
            }

            // Multi-level path - use findDotted for partial resolution
            string remainingBaddot;
            VSymEnt* remainingOkSymp = nullptr;
            VSymEnt* ধ্রুব symp = m_statep->findDotted(fl, ifaceSymp, baddot, remainingBaddot,
                                                       remainingOkSymp, সত্য);
            যদি (symp) {
                baddot = remainingBaddot;
                প্রত্যাগম symp;
            }

            // findDotted partially matched - check progress
            যদি (remainingBaddot == baddot || !remainingOkSymp) প্রত্যাগম nullptr;

            // Continue resolving with updated state
            baddot = remainingBaddot;
            curOkSymp = remainingOkSymp;
        }

        UINFO(1, "Nested interface resolution depth limit exceeded at " << fl);
        প্রত্যাগম nullptr;
    }

    // Resolve a modport expression to find the referenced symbol
    VSymEnt* resolveModportExpression(File_সারি* fl, Ast_গ্রন্থি_রাশিমালা* exprp) {
        UINFO(5, "     resolveModportExpression: " << exprp);
        VSymEnt* symp = nullptr;
        যদি (AstParseRef* ধ্রুব refp = VN_CAST(exprp, ParseRef)) {
            // Simple variable reference: modport mp(input .a(sig_a))
            symp = m_curSymp->findIdFallback(refp->name());
        } অন্যথা যদি (AstVarRef* ধ্রুব refp = VN_CAST(exprp, VarRef)) {
            // Already resolved VarRef (can happen if iterateChildren resolved it)
            যদি (refp->varScopep()) {
                // During scope creation, VarRef may have VarScope already linked
                symp = m_statep->get_গ্রন্থি_Sym(refp->varScopep());
            } অন্যথা যদি (refp->varp()) {
                symp = m_curSymp->findIdFallback(refp->varp()->name());
            }
        } অন্যথা যদি (AstVarXRef* ধ্রুব refp = VN_CAST(exprp, VarXRef)) {
            // Resolved VarXRef (dotted reference resolved by earlier pass)
            যদি (refp->varp()) {
                // For nested interfaces, the var is in a different scope
                // First try to find the symbol via the var itself
                যদি (m_statep->exists_গ্রন্থি_Sym(refp->varp())) {
                    symp = m_statep->get_গ্রন্থি_Sym(refp->varp());
                } অন্যথা {
                    // Fallback: look up in current scope
                    symp = m_curSymp->findIdFallback(refp->varp()->name());
                }
            } অন্যথা যদি (!refp->dotted().empty()) {
                // varp not set yet - use dotted path to find the symbol
                // The dotted part (e.g., "base") is the interface cell, and name is the member
                ধ্রুব string fullPath = refp->dotted() + "." + refp->name();
                string baddot;
                VSymEnt* okSymp = nullptr;
                symp = m_statep->findDotted(fl, m_curSymp, fullPath, baddot, okSymp, সত্য);
                // Handle nested interface path when findDotted had partial match
                যদি (okSymp && !baddot.empty()) {
                    VSymEnt* ধ্রুব resolved = resolveNestedInterfacePath(fl, okSymp, baddot);
                    যদি (resolved) symp = resolved;
                }
            }
        } অন্যথা যদি (AstDot* ধ্রুব dotp = VN_CAST(exprp, Dot)) {
            // Dotted path: modport mp(input .a(inner.sig))
            তার্কিক hasPartSelect = অসত্য;
            ধ্রুব string dottedPath = extractDottedPath(dotp, hasPartSelect);
            যদি (hasPartSelect) {
                fl->v3warn(
                    E_UNSUPPORTED,
                    "Unsupported: Modport expression with part select (IEEE 1800-2023 25.5.4)");
            } অন্যথা {
                string baddot;
                VSymEnt* okSymp = nullptr;
                symp = m_statep->findDotted(fl, m_curSymp, dottedPath, baddot, okSymp, সত্য);

                // Handle nested interface path when findDotted had partial match
                যদি (okSymp && !baddot.empty()) {
                    VSymEnt* ধ্রুব resolved = resolveNestedInterfacePath(fl, okSymp, baddot);
                    যদি (resolved) symp = resolved;
                }

                যদি (!symp || !baddot.empty()) {
                    fl->v3error("Can't find modport expression target: "
                                << Ast_গ্রন্থি::prettyNameQ(dottedPath));
                    symp = nullptr;
                }
            }
        } অন্যথা যদি (VN_IS(exprp, SelBit) || VN_IS(exprp, SelExtract)) {
            // Part select expressions not yet supported
            fl->v3warn(E_UNSUPPORTED,
                       "Unsupported: Modport expression with part select (IEEE 1800-2023 25.5.4)");
        } অন্যথা {
            // Other expression types not supported
            fl->v3warn(E_UNSUPPORTED,
                       "Unsupported: Complex modport expression (IEEE 1800-2023 25.5.4)");
        }
        প্রত্যাগম symp;
    }

    শূন্য visit(AstModportVarRef* nodep) override {  // IfaceVisitor::
        UINFO(5, "   fiv: " << nodep);
        iterateChildren(nodep);
        VSymEnt* symp = nullptr;
        যদি (nodep->exprp()) {
            // Modport expression syntax: modport mp(input .port_name(expression))
            symp = resolveModportExpression(nodep->fileline(), nodep->exprp());
        } অন্যথা {
            symp = m_curSymp->findIdFallback(nodep->name());
        }
        যদি (!symp) {
            nodep->v3error("Modport item not found: " << nodep->prettyNameQ());
        } অন্যথা যদি (AstVar* ধ্রুব varp = VN_CAST(symp->nodep(), Var)) {
            // Make symbol under modport that points at the _interface_'s var via the modport.
            // (Need modport still to test input/output markings)
            nodep->varp(varp);
            যদি (nodep->exprp()) {
                // For modport expressions, insert symbol pointing to the underlying var (not the
                // ModportVarRef which will be deleted during scope creation). The virtual port
                // name maps to the real signal's var.
                VSymEnt* ধ্রুব subSymp
                    = m_statep->ঢোকাও_Sym(m_curSymp, nodep->name(), varp, nullptr /*package*/);
                m_statep->ঢোকাও_Scope_Alias(LinkDotState::SAMN_MODPORT, subSymp, symp);
            } অন্যথা {
                // For regular modport items, insert symbol pointing to ModportVarRef for
                // input/output marking tests.
                m_statep->ঢোকাও_Sym(m_curSymp, nodep->name(), nodep, nullptr /*package*/);
            }
        } অন্যথা যদি (AstVarScope* ধ্রুব vscp = VN_CAST(symp->nodep(), VarScope)) {
            // Make symbol under modport that points at the _interface_'s var, not the modport.
            nodep->varp(vscp->varp());
            // Only insert symbol for modport expression virtual ports (exprp set).
            // For regular modport items, don't insert into the shared modport table because
            // each instance would overwrite the previous one, causing wrong VarScope lookups.
            // Regular items are found via the modport's fallback to the interface.
            যদি (nodep->exprp()) {
                VSymEnt* ধ্রুব subSymp
                    = m_statep->ঢোকাও_Sym(m_curSymp, nodep->name(), vscp, nullptr /*package*/);
                m_statep->ঢোকাও_Scope_Alias(LinkDotState::SAMN_MODPORT, subSymp, symp);
            }
        } অন্যথা {
            nodep->v3error("Modport item is not a variable: " << nodep->prettyNameQ());
        }
        যদি (m_statep->forScopeCreation()) {
            // Done with AstModportVarRef.
            // Delete to prevent problems if we dead-delete pointed to variable
            nodep->unlinkFrBack();
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
        }
    }
    শূন্য visit(AstModportClockingRef* nodep) override {  // IfaceVisitor::
        UINFO(5, "   fic: " << nodep);
        iterateChildren(nodep);
        VSymEnt* ধ্রুব symp = m_curSymp->findIdFallback(nodep->name());
        যদি (!symp) {
            nodep->v3error("Modport item not found: " << nodep->prettyNameQ());
        } অন্যথা যদি (AstClocking* ধ্রুব clockingp = VN_CAST(symp->nodep(), Clocking)) {
            nodep->clockingp(clockingp);
            m_statep->ঢোকাও_Sym(m_curSymp, nodep->name(), nodep, nullptr /*package*/);
        } অন্যথা {
            nodep->v3error("Modport item is not a clocking block: " << nodep->prettyNameQ());
        }
    }
    শূন্য visit(AstScope* nodep) override {  // IfaceVisitor::
        যদি (nodep->user3SetOnce()) প্রত্যাগম;
        iterateChildren(nodep);
    }
    শূন্য visit(Ast_গ্রন্থি* nodep) override { iterateChildren(nodep); }  // IfaceVisitor::

সার্বজনিক:
    // CONSTRUCTORS
    LinkDotIfaceVisitor(AstIface* nodep, VSymEnt* curSymp, LinkDotState* statep)
        : m_statep{statep}
        , m_curSymp{curSymp} {
        UINFO(4, __FUNCTION__ << ": ");
        iterate(nodep);
    }
    ~LinkDotIfaceVisitor() override = স্বভাব;
};

শূন্য LinkDotState::computeIfaceModSyms() {
    //  AstScope::user3()  // bool. Already iterated by a LinkDotIfaceVisitor
    ধ্রুব VNUser3InUse user3InUse;
    প্রতি (ধ্রুব auto& itr : m_ifaceModSyms) {
        AstIface* ধ্রুব nodep = itr.first;
        VSymEnt* ধ্রুব symp = itr.second;
        LinkDotIfaceVisitor{nodep, symp, এতত্};
    }
    m_ifaceModSyms.clear();
}

//======================================================================

বর্গ LinkDotResolveVisitor final : সার্বজনিক VNVisitor {
    // NODE STATE
    // Cleared on global
    //  *::user1p()             -> See LinkDotState
    //  *::user2p()             -> See LinkDotState
    //  *::user3()              // bool. Processed
    //  *::user4()              -> See LinkDotState
    ধ্রুব VNUser3InUse m_inuser3;

    // TYPES
    গণনা DotPosition : uint8_t {
        // Must match ascii() method below
        DP_NONE = 0,  // Not under a DOT
        DP_PACKAGE,  // {package-or-class}:: DOT
        DP_FIRST,  // {scope-or-var} DOT
        DP_SCOPE,  // DOT... {scope-or-var} DOT
        DP_FINAL,  // [DOT...] {var-or-func-or-dtype} with no following dots
        DP_MEMBER  // DOT {member-name} [DOT...]
    };

    // STATE
    LinkDotState* ধ্রুব m_statep;  // State, including dotted symbol table
    VSymEnt* m_curSymp = nullptr;  // SymEnt for current lookup point
    VSymEnt* m_modSymp = nullptr;  // SymEnt for current module
    VSymEnt* m_pinSymp = nullptr;  // SymEnt for pin lookups
    VSymEnt* m_randSymp = nullptr;  // SymEnt for randomize target class's lookups
    ধ্রুব AstCell* m_cellp = nullptr;  // Current cell
    Ast_গ্রন্থি_Module* m_modp = nullptr;  // Current module
    Ast_গ্রন্থি_FTask* m_ftaskp = nullptr;  // Current function/task
    AstMethodCall* m_randMethodCallp = nullptr;  // Current randomize() method call
    পূর্ণাঙ্ক m_modport_অঙ্ক = 0;  // Uniqueify modport numbers
    পূর্ণাঙ্ক m_indent = 0;  // Indentation (tree depth) for debug
    তার্কিক m_inSens = অসত্য;  // True if in senitem
    ধ্রুব AstWith* m_currentWithp = nullptr;  // Enclosing 'with' (nullptr if none)
    std::set<std::string>
        m_restrictedNamesUsed;  // Names from current 'with (id_list)' that resolved into target
    তার্কিক m_genericIfaceModule = অসত্য;  // True if in module containing generic interface
    std::map<std::string, Ast_গ্রন্থি*> m_ifClassImpNames;  // Names imported from interface class
    // Inherited interface implementations needing a forward in the current class, by name
    std::map<std::string, ধ্রুব VSymEnt*> m_ifForwards;
    std::set<AstClass*> m_extendsParam;  // Classes that have a parameterized super class
                                         // (except the default instances)
                                         // They are added to the set only in linkDotPrimary.
    তার্কিক m_insideClassExtParam = অসত্য;  // Inside a class from m_extendsParam
    AstNew* m_explicitSuperNewp = nullptr;  // Hit a "super.new" call inside a "new" function
    std::map<Ast_গ্রন্থি*, AstPin*> m_usedPins;  // Pin used in this cell, map to duplicate
    std::map<Ast_গ্রন্থি*, std::map<std::string, AstPin*>> m_usedDefParamPins;
    // Defparam pins used for a given formal, by hierarchical path
    std::map<std::string, Ast_গ্রন্থি_Module*> m_modulesToRevisit;  // Modules to revisit a second time
    Ast_গ্রন্থি* m_lastDeferredp = nullptr;  // Last node which requested a revisit of its module
    Ast_গ্রন্থি_DType* m_packedArrayDtp = nullptr;  // Datatype reference for packed array
    তার্কিক m_inPackedArray = অসত্য;  // Currently traversing a packed array tree
    তার্কিক m_replaceWithAlias
        = সত্য;  // Replace VarScope with an alias. Used in the handling of AstAlias
    তার্কিক m_isParam = অসত্য;  // Specifies whether currently visiting param variable
    তার্কিক m_resolvingTypedef = অসত্য;  // Currently traversing a Typedef tree

    সংরচনা DotStates final {
        DotPosition m_dotPos;  // Scope part of dotted resolution
        VSymEnt* m_dotSymp;  // SymEnt for dotted AstParse lookup
        ধ্রুব AstDot* m_dotp;  // Current dot
        তার্কিক m_super;  // Starts with super reference
        তার্কিক m_unresolvedCell;  // Unresolved cell, needs help from V3Param
        তার্কিক m_unresolvedClass;  // Unresolved class reference, needs help from V3Param
        তার্কিক m_unresolvedGenericIface;  // Unresolved generic interface, needs help from V3Param
        তার্কিক m_genBlk;  // Contains gen block reference
        Ast_গ্রন্থি* m_unlinkedScopep;  // Unresolved scope, needs corresponding VarXRef
        AstDisable* m_disablep;  // Disable statement under which the reference is
        তার্কিক m_dotErr;  // Error found in dotted resolution, ignore upwards
        string m_dotText;  // String of dotted names found in below parseref
        DotStates() { init(nullptr); }
        ~DotStates() = স্বভাব;
        শূন্য init(VSymEnt* curSymp, তার্কিক unresolvedClass = অসত্য) {
            m_dotPos = DP_NONE;
            m_dotSymp = curSymp;
            m_dotp = nullptr;
            m_super = অসত্য;
            m_dotErr = অসত্য;
            m_dotText = "";
            m_unresolvedCell = অসত্য;
            m_unresolvedClass = unresolvedClass;
            m_unresolvedGenericIface = অসত্য;
            m_genBlk = অসত্য;
            m_unlinkedScopep = nullptr;
            m_disablep = nullptr;
        }
        string ascii() ধ্রুব {
            স্থির ধ্রুব বর্ণ* ধ্রুব names[]
                = {"DP_NONE", "DP_PACKAGE", "DP_FIRST", "DP_SCOPE", "DP_FINAL", "DP_MEMBER"};
            std::ostringstream sstr;
            sstr << "ds=" << names[m_dotPos];
            sstr << "  dse" << cvtToHex(m_dotSymp);
            ধ্রুব string dsname = m_dotSymp->nodep()->name().substr(0, 8);
            sstr << "(" << m_dotSymp->nodep()->typeName() << (dsname.empty() ? "" : ":") << dsname
                 << ")";
            যদি (m_dotErr) sstr << "  [dotErr]";
            যদি (m_super) sstr << "  [super]";
            যদি (m_unresolvedCell) sstr << "  [unrCell]";
            যদি (m_unresolvedClass) sstr << "  [unrClass]";
            যদি (m_unresolvedGenericIface) sstr << "  [unrGIface]";
            যদি (m_genBlk) sstr << "  [genBlk]";
            sstr << "  txt=" << m_dotText;
            প্রত্যাগম sstr.str();
        }
    } m_ds;  // State to preserve across recursions

    // METHODS - Variables
    AstVar* createImplicitVar(VSymEnt* /*lookupSymp*/, AstParseRef* nodep, Ast_গ্রন্থি_Module* modp,
                              VSymEnt* moduleSymp, তার্কিক noWarn) {
        // Create implicit after warning
        যদি (!noWarn) {
            যদি (nodep->fileline()->warnIsOff(V3ErrorCode::I_DEF_NETTYPE_WIRE)) {
                ধ্রুব string suggest = m_statep->suggestSymFallback(moduleSymp, nodep->name(),
                                                                    Link_গ্রন্থি_Matcher_Var{});
                nodep->v3error("Signal definition not found, and implicit disabled with "
                               "`default_nettype: "
                               << nodep->prettyNameQ() << '\n'
                               << (suggest.empty() ? "" : nodep->warnMore() + suggest));

            }
            // Bypass looking for suggestions if IMPLICIT is turned off
            // as there could be thousands of these suppressed in large netlists
            অন্যথা যদি (!nodep->fileline()->warnIsOff(V3ErrorCode::IMPLICIT)) {
                ধ্রুব string suggest = m_statep->suggestSymFallback(moduleSymp, nodep->name(),
                                                                    Link_গ্রন্থি_Matcher_Var{});
                nodep->v3warn(IMPLICIT,
                              "Signal definition not found, creating implicitly: "
                                  << nodep->prettyNameQ() << '\n'
                                  << (suggest.empty() ? "" : nodep->warnMore() + suggest));
            }
        }
        AstVar* ধ্রুব newp
            = নূতন AstVar{nodep->fileline(), VVarType::WIRE, nodep->name(), VFlagLogicPacked{}, 1};
        newp->lifetime(VLifetime::STATIC_IMPLICIT);
        newp->trace(modp->modTrace());
        modp->addStmtsp(newp);
        // Link it to signal list, must add the variable under the module;
        // current scope might be lower now
        m_statep->ঢোকাও_Sym(moduleSymp, newp->name(), newp, nullptr /*classOrPackagep*/);
        প্রত্যাগম newp;
    }
    AstVar* foundToVarp(ধ্রুব VSymEnt* symp, Ast_গ্রন্থি* nodep, VAccess access) {
        // Return a variable if possible, auto converting a modport to variable
        যদি (!symp) {
            প্রত্যাগম nullptr;
        } অন্যথা যদি (VN_IS(symp->nodep(), Var)) {
            প্রত্যাগম VN_AS(symp->nodep(), Var);
        } অন্যথা যদি (VN_IS(symp->nodep(), ModportVarRef)) {
            AstModportVarRef* ধ্রুব snodep = VN_AS(symp->nodep(), ModportVarRef);
            AstVar* ধ্রুব varp = snodep->varp();
            যদি (access.isWriteOrRW() && snodep->direction().isReadOnly()) {
                nodep->v3error("Attempt to drive input-only modport: " << nodep->prettyNameQ());
            }  // else other simulators don't warn about reading, and IEEE doesn't say illegal
            প্রত্যাগম varp;
        } অন্যথা {
            প্রত্যাগম nullptr;
        }
    }

    // Look up a virtual port through modport expression mapping.
    // When a dotted reference uses a modport with expression syntax, the virtual port name
    // maps to the real signal. This helper resolves that mapping during scope creation.
    // Returns the VarScope for the real signal, or nullptr if not found.
    AstVarScope* findVirtualPortVarScope(VSymEnt* dotSymp, ধ্রুব string& portName) {
        ধ্রুব AstVarScope* ধ্রুব dotVscp = VN_CAST(dotSymp->nodep(), VarScope);
        ধ্রুব AstVar* ধ্রুব dotVarp = dotVscp ? dotVscp->varp() : nullptr;
        যদি (!dotVarp) প্রত্যাগম nullptr;

        Ast_গ্রন্থি_DType* dtypep = dotVarp->childDTypep();
        যদি (!dtypep) dtypep = dotVarp->subDTypep();
        যদি (!dtypep) প্রত্যাগম nullptr;
        ধ্রুব AstIfaceRefDType* ধ্রুব ifaceRefp = VN_CAST(dtypep, IfaceRefDType);
        যদি (!ifaceRefp || !ifaceRefp->modportp() || !ifaceRefp->ifaceViaCellp()) প্রত্যাগম nullptr;

        // Get the interface cell's symbol table
        VSymEnt* ধ্রুব ifaceCellSymp = m_statep->get_গ্রন্থি_Sym(ifaceRefp->ifaceViaCellp());

        // Look up modport by name in interface cell
        VSymEnt* ধ্রুব modportSymp = ifaceCellSymp->findIdFallback(ifaceRefp->modportp()->name());
        যদি (!modportSymp) প্রত্যাগম nullptr;

        // Look up virtual port name in modport symbol table
        string baddot;
        VSymEnt* ধ্রুব virtPortp = m_statep->findSymPrefixed(modportSymp, portName, baddot, সত্য);
        যদি (!virtPortp) প্রত্যাগম nullptr;

        // Symbol may point to VarScope or Var
        যদি (AstVarScope* vscp = VN_CAST(virtPortp->nodep(), VarScope)) প্রত্যাগম vscp;

        // Symbol points to Var - look up VarScope by name in interface cell
        যদি (ধ্রুব AstVar* ধ্রুব realVarp = VN_CAST(virtPortp->nodep(), Var)) {
            VSymEnt* ধ্রুব realFoundp
                = m_statep->findSymPrefixed(ifaceCellSymp, realVarp->name(), baddot, সত্য);
            প্রত্যাগম realFoundp ? VN_CAST(realFoundp->nodep(), VarScope) : nullptr;
        }
        প্রত্যাগম nullptr;
    }

    // Capture a ParamTypeDType reference for interface typedef retargeting.
    // Called when a RefDType resolves to a ParamTypeDType owned by an interface.
    শূন্য captureIfaceParamType(AstRefDType* nodep, AstParamTypeDType* defp) {
        যদি (!V3LinkDotIfaceCapture::enabled() || !m_statep->forPrimary()) প্রত্যাগম;
        Ast_গ্রন্থি_Module* ধ্রুব defOwnerModp = V3LinkDotIfaceCapture::findOwnerModule(defp);
        যদি (!defOwnerModp || !VN_IS(defOwnerModp, Iface)) প্রত্যাগম;
        AstCell* ধ্রুব cellForCapture
            = m_ds.m_dotSymp ? VN_CAST(m_ds.m_dotSymp->nodep(), Cell) : nullptr;
        যদি (!cellForCapture) প্রত্যাগম;
        UINFO(9, indent() << "iface capture add paramtype " << nodep
                          << " iface=" << defOwnerModp->prettyNameQ());
        V3LinkDotIfaceCapture::addParamType(nodep, cellForCapture->name(), m_modp, defp,
                                            defOwnerModp->name());
    }

    Ast_গ্রন্থি_Stmt* addImplicitSuperNewCall(AstFunc* ধ্রুব nodep,
                                         ধ্রুব AstClassExtends* ধ্রুব classExtendsp) {
        // Returns the added node
        File_সারি* ধ্রুব fl = nodep->fileline();
        AstArg* argsp = nullptr;
        যদি (classExtendsp->argsp()) argsp = classExtendsp->argsp()->clone_বৃক্ষ(সত্য);
        AstNew* ধ্রুব newExprp = নূতন AstNew{fl, argsp};
        newExprp->isImplicit(সত্য);
        AstDot* ধ্রুব superNewp = নূতন AstDot{fl, অসত্য, নূতন AstParseRef{fl, "super"}, newExprp};
        Ast_গ্রন্থি_Stmt* ধ্রুব superNewStmtp = superNewp->makeStmt();
        প্রতি (Ast_গ্রন্থি* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            // super.new shall be the first statement (IEEE 1800-2023 8.15)
            // but some nodes (such as variable declarations and typedefs) should stay before
            যদি (VN_IS(stmtp, গ্রন্থি_Stmt)) {
                stmtp->addHereThisAsNext(superNewStmtp);
                প্রত্যাগম superNewStmtp;
            }
        }
        // There were no statements
        nodep->addStmtsp(superNewStmtp);
        প্রত্যাগম superNewStmtp;
    }
    শূন্য checkNoDot(Ast_গ্রন্থি* nodep) {
        যদি (VL_UNLIKELY(!nodep)) {
            UINFO(9, indent() << "iface capture null node passed to checkNoDot; dot state="
                              << m_ds.ascii());
            m_ds.m_dotErr = সত্য;
            প্রত্যাগম;
        }
        যদি (VL_UNLIKELY(m_ds.m_dotPos != DP_NONE)) {
            UINFO(9, indent() << "ds=" << m_ds.ascii());
            UINFO(9, indent() << "iface capture checkNoDot hit node=" << nodep
                              << " dotState=" << m_ds.ascii());
            যদি (VL_UNLIKELY(!m_ds.m_dotp)) {
                nodep->v3error("Syntax error: Not expecting " << nodep->type()
                                                              << " here (missing dot context)");
            } অন্যথা {
                nodep->v3error("Syntax error: Not expecting "
                               << nodep->type() << " under a " << nodep->backp()->type()
                               << " in dotted expression\n"
                               << nodep->warnContextPrimary() << m_ds.m_dotp->warnOther()
                               << "... Resolving this reference\n"
                               << m_ds.m_dotp->warnContextSecondary());
            }
            m_ds.m_dotErr = সত্য;
        }
    }
    AstVar* find_Iface_শীর্ষ_Varp(Ast_গ্রন্থি* nodep, VSymEnt* parentEntp, ধ্রুব string& name) {
        ধ্রুব string findName = name + "__Viftop";
        VSymEnt* ধ্রুব ifaceSymp = parentEntp->findIdFallback(findName);
        AstVar* ধ্রুব iface_শীর্ষ_Varp = ifaceSymp ? VN_AS(ifaceSymp->nodep(), Var) : nullptr;
        UASSERT_OBJ(iface_শীর্ষ_Varp, nodep, "Can't find interface var ref: " << findName);
        প্রত্যাগম iface_শীর্ষ_Varp;
    }
    AstRSProd* findProd(Ast_গ্রন্থি* nodep, VSymEnt* parentEntp, ধ্রুব string& name) {
        ধ্রুব VSymEnt* ধ্রুব foundp = parentEntp->findIdFallback(name);
        AstRSProd* foundNodep = foundp ? VN_CAST(foundp->nodep(), RSProd) : nullptr;
        যদি (!foundNodep) {
            VSpellCheck speller;
            Link_গ্রন্থি_Matcher_Prod matcher;
            parentEntp->candidateIdFlat(&speller, &matcher, অসত্য);
            ধ্রুব string suggest = speller.best_Candidate_বার্তা(name);
            UINFO(1, "   ErrParseRef curSymp=se" << cvtToHex(parentEntp));
            nodep->v3error("Production " << Ast_গ্রন্থি::prettyNameQ(name) << " not found\n"
                                         << (suggest.empty() ? "" : nodep->warnMore() + suggest));
            প্রত্যাগম nullptr;
        }
        প্রত্যাগম foundNodep;
    }
    শূন্য duplicatePinError(AstPin* nodep, Ast_গ্রন্থি* origp, ধ্রুব বর্ণ* whatp) {
        nodep->v3error("Duplicate " << whatp << " connection: " << nodep->prettyNameQ() << '\n'
                                    << nodep->warnContextPrimary() << '\n'
                                    << origp->warnOther() << "... Location of original " << whatp
                                    << " connection\n"
                                    << origp->warnContextSecondary());
    }
    শূন্য markAndCheckPinDup(AstPin* nodep, Ast_গ্রন্থি* refp, ধ্রুব বর্ণ* whatp) {
        ধ্রুব auto pair = m_usedPins.emplace(refp, nodep);
        যদি (pair.second) {
            যদি (!nodep->paramPath().empty())
                m_usedDefParamPins[refp].emplace(nodep->paramPath(), nodep);
            প্রত্যাগম;
        } অন্যথা {
            Ast_গ্রন্থি* ধ্রুব origp = pair.first->second;
            যদি (nodep->paramPath().empty() || VN_AS(origp, Pin)->paramPath().empty()) {
                duplicatePinError(nodep, origp, whatp);
                প্রত্যাগম;
            }
        }

        auto& defParamPins = m_usedDefParamPins[refp];
        ধ্রুব auto defParamIt = defParamPins.find(nodep->paramPath());
        যদি (defParamIt != defParamPins.end()) {
            duplicatePinError(nodep, defParamIt->second, whatp);
            প্রত্যাগম;
        }
        defParamPins.emplace(nodep->paramPath(), nodep);
    }
    স্থির তার্কিক extendsClass(ধ্রুব AstClass* classp, ধ্রুব AstClass* targetp) {
        // Return true if classp inherits from targetp through already resolved extends
        // If performance of this becomes a problem, use just a counter to check
        // exceeds some number of iterations then perform the more expensive analysis
        std::set<ধ্রুব AstClass*> visited;
        std::vector<ধ্রুব AstClass*> todo{classp};
        যাবত্ (!todo.empty()) {
            ধ্রুব AstClass* ধ্রুব currp = todo.back();
            todo.বের_করো_back();
            যদি (currp == targetp) প্রত্যাগম সত্য;
            যদি (!visited.ঢোকাও(currp).second) অনুবর্ত;
            প্রতি (ধ্রুব AstClassExtends* cextp = currp->extendsp(); cextp;
                 cextp = VN_AS(cextp->nextp(), ClassExtends)) {
                যদি (ধ্রুব AstClass* ধ্রুব basep = cextp->classOrNullp()) todo.ঠেলো_back(basep);
            }
        }
        প্রত্যাগম অসত্য;
    }
    স্থির AstClocking* sensClockingp(Ast_গ্রন্থি* nodep) {
        // Return the clocking block referenced by nodep, either directly or through a modport
        যদি (AstClocking* ধ্রুব clockingp = VN_CAST(nodep, Clocking)) প্রত্যাগম clockingp;
        যদি (ধ্রুব AstModportClockingRef* ধ্রুব clockingRefp = VN_CAST(nodep, ModportClockingRef))
            প্রত্যাগম clockingRefp->clockingp();
        প্রত্যাগম nullptr;
    }
    VSymEnt* getCreateClockingEventSymEnt(AstClocking* clockingp) {
        AstVar* ধ্রুব eventp = clockingp->ensureEventp(সত্য);
        যদি (!eventp->user1p()) eventp->user1p(নূতন VSymEnt{m_statep->symsp(), eventp});
        প্রত্যাগম reinterpret_cast<VSymEnt*>(eventp->user1p());
    }
    VSymEnt* findInlinedDotsSym(Ast_গ্রন্থি* nodep, ধ্রুব string& inlinedDots) {
        // Return scope that applies for searching, given renaming due to module inlining.
        // Dotted lookup is always relative to module, as maybe variable
        // name lower down with same scope name we want to ignore (t_math_divw)
        VSymEnt* dotSymp = m_modSymp;
        ধ্রুব string inl = Ast_গ্রন্থি::dedotName(inlinedDots);
        string baddot;
        VSymEnt* okSymp = nullptr;
        dotSymp = m_statep->findDotted(nodep->fileline(), dotSymp, inl, baddot, okSymp, অসত্য);
        UASSERT_OBJ(dotSymp, nodep,
                    "Couldn't resolve inlined scope " << Ast_গ্রন্থি::prettyNameQ(baddot)
                                                      << " in: " << inlinedDots);
        UINFO(9, indent() << "findInlinedDotsSym " << dotSymp
                          << " search start due to inlinedDots=" << inlinedDots);
        প্রত্যাগম dotSymp;
    }
    VSymEnt* findDottedRefSymp(Ast_গ্রন্থি* nodep, VSymEnt* lookupSymp, ধ্রুব string& dotted,
                               string& baddot, VSymEnt*& okSymp, তার্কিক* modportp = nullptr) {
        // Return scope to search for a relinked hierarchical reference
        // (AstVarXRef or AstEnumItemRef), or nullptr after error if not found
        // If modportp is given, set true when the reference went through a modport
        যদি (modportp) *modportp = অসত্য;
        VSymEnt* dotSymp = m_statep->findDotted(nodep->fileline(), lookupSymp, dotted, baddot,
                                                okSymp, সত্য);  // Maybe nullptr
        যদি (!dotSymp) {
            nodep->v3error(
                "Can't find definition of "
                << (!baddot.empty() ? Ast_গ্রন্থি::prettyNameQ(baddot) : nodep->prettyNameQ()) << '\n'
                << nodep->warnContextPrimary());
            প্রত্যাগম nullptr;
        }
        যদি (ধ্রুব AstVar* ধ্রুব varp = VN_CAST(dotSymp->nodep(), Var)) {
            যদি (ধ্রুব AstIfaceRefDType* ধ্রুব ifaceRefp
                = VN_CAST(varp->childDTypep(), IfaceRefDType)) {
                যদি (ifaceRefp->modportp()) {
                    dotSymp = m_statep->get_গ্রন্থি_Sym(ifaceRefp->modportp());
                    যদি (modportp) *modportp = সত্য;
                } অন্যথা {
                    dotSymp = m_statep->get_গ্রন্থি_Sym(ifaceRefp->ifacep());
                }
            }
        } অন্যথা যদি (ধ্রুব AstModportClockingRef* ধ্রুব clockingRefp
                   = VN_CAST(dotSymp->nodep(), ModportClockingRef)) {
            dotSymp = m_statep->get_গ্রন্থি_Sym(clockingRefp->clockingp());
        }
        প্রত্যাগম dotSymp;
    }

    স্থির তার্কিক isParamedClassRefDType(ধ্রুব Ast_গ্রন্থি* classp) {
        যাবত্ (ধ্রুব AstRefDType* ধ্রুব refp = VN_CAST(classp, RefDType))
            classp = refp->subDTypep();
        প্রত্যাগম (VN_IS(classp, ClassRefDType) && VN_AS(classp, ClassRefDType)->paramsp())
               || VN_IS(classp, ParamTypeDType);
    }
    তার্কিক isParamedClassRef(ধ্রুব Ast_গ্রন্থি* nodep) {
        // Is this a parameterized reference to a class, or a reference to class parameter
        যদি (ধ্রুব auto* classRefp = VN_CAST(nodep, ClassOrPackageRef)) {
            যদি (classRefp->paramsp()) প্রত্যাগম সত্য;
            ধ্রুব auto* classp = classRefp->classOrPackageNodep();
            যাবত্ (ধ্রুব auto* typedefp = VN_CAST(classp, Typedef)) classp = typedefp->subDTypep();
            প্রত্যাগম isParamedClassRefDType(classp);
        }
        প্রত্যাগম isParamedClassRefDType(nodep);
    }
    VSymEnt* getThisClassSymp() {
        VSymEnt* classSymp = m_ds.m_dotSymp;
        যাবত্ (classSymp && !VN_IS(classSymp->nodep(), Class)) {
            classSymp = classSymp->parentp();
        }
        প্রত্যাগম classSymp;
    }
    // Is the current scope within the given class, including within its nested classes
    তার্কিক insideClass(ধ্রুব AstClass* classp) ধ্রুব {
        প্রতি (ধ্রুব VSymEnt* symp = m_curSymp; symp; symp = symp->parentp()) {
            যদি (symp->nodep() == classp) প্রত্যাগম সত্য;
        }
        প্রত্যাগম অসত্য;
    }
    // 'randomize() with' target class not yet specialized; defer to post-V3Param pass
    তার্কিক randTargetUnresolved() ধ্রুব {
        প্রত্যাগম m_statep->forPrimary() && m_currentWithp && m_randMethodCallp && !m_randSymp;
    }
    // Is 'funcp' of 'classp' virtual, directly or by overriding a virtual method of a base
    // class (IEEE 1800-2023 8.20)
    তার্কিক isVirtualMethod(ধ্রুব Ast_গ্রন্থি_FTask* funcp, ধ্রুব AstClass* classp) ধ্রুব {
        যদি (funcp->isVirtual()) প্রত্যাগম সত্য;
        প্রতি (ধ্রুব AstClassExtends* cextp = classp->extendsp(); cextp;
             cextp = VN_AS(cextp->nextp(), ClassExtends)) {
            AstClass* ধ্রুব basep = cextp->classOrNullp();
            যদি (!basep) অনুবর্ত;
            ধ্রুব VSymEnt* ধ্রুব foundp = m_statep->get_গ্রন্থি_Sym(basep)->findIdFlat(funcp->name());
            ধ্রুব Ast_গ্রন্থি_FTask* ধ্রুব baseFuncp
                = foundp ? VN_CAST(foundp->nodep(), গ্রন্থি_FTask) : nullptr;
            ধ্রুব AstClass* ধ্রুব baseClassp = foundp && foundp->parentp()
                                                   ? VN_CAST(foundp->parentp()->nodep(), Class)
                                                   : nullptr;
            যদি (baseFuncp && baseClassp && isVirtualMethod(baseFuncp, baseClassp)) প্রত্যাগম সত্য;
        }
        প্রত্যাগম অসত্য;
    }
    // Is imported 'implp' an implementation of interface method 'ifSymp' (IEEE 1800-2023
    // 8.26.2)? If its class does not derive from the interface class, it needs a forwarding
    // override in the derived class to be reachable through the interface.
    তার্কিক inheritedImplements(ধ্রুব VSymEnt* implp, ধ্রুব VSymEnt* ifSymp, তার্কিক& needForwardr) {
        needForwardr = অসত্য;
        ধ্রুব Ast_গ্রন্থি_FTask* ধ্রুব implFuncp = VN_CAST(implp->nodep(), গ্রন্থি_FTask);
        যদি (!implFuncp || implFuncp->pureVirtual() || implFuncp->isStatic()) প্রত্যাগম অসত্য;
        ধ্রুব AstClass* ধ্রুব implClassp
            = implp->parentp() ? VN_CAST(implp->parentp()->nodep(), Class) : nullptr;
        ধ্রুব AstClass* ধ্রুব ifClassp
            = ifSymp->parentp() ? VN_CAST(ifSymp->parentp()->nodep(), Class) : nullptr;
        যদি (!implClassp || !ifClassp) প্রত্যাগম অসত্য;
        যদি (extendsClass(implClassp, ifClassp)) প্রত্যাগম সত্য;
        যদি (!isVirtualMethod(implFuncp, implClassp)) প্রত্যাগম অসত্য;
        needForwardr = সত্য;
        প্রত্যাগম সত্য;
    }
    // Add 'implp' override to 'classp' that calls super.implp, so the C++ method of an
    // implemented interface class is overridden
    শূন্য addInterfaceForward(AstClass* classp, Ast_গ্রন্থি_FTask* implp, AstClass* implClassp) {
        File_সারি* ধ্রুব flp = implp->fileline();
        AstVar* fvarp = nullptr;
        Ast_গ্রন্থি_FTask* newp;
        যদি (AstFunc* ধ্রুব implFuncp = VN_CAST(implp, Func)) {
            fvarp = VN_AS(implFuncp->fvarp(), Var)->clone_বৃক্ষ(অসত্য);
            newp = নূতন AstFunc{flp, implp->name(), nullptr, fvarp};
        } অন্যথা {
            newp = নূতন AstTask{flp, implp->name(), nullptr};
        }
        newp->classMethod(সত্য);
        newp->isVirtual(সত্য);
        newp->lifetime(implp->lifetime());
        AstArg* argsp = nullptr;
        প্রতি (Ast_গ্রন্থি* stmtp = implp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstVar* ধ্রুব portp = VN_CAST(stmtp, Var);
            যদি (!portp || !portp->isIO()) অনুবর্ত;
            AstVar* ধ্রুব newPortp = portp->clone_বৃক্ষ(অসত্য);
            newp->addStmtsp(newPortp);
            argsp = Ast_গ্রন্থি::addNext(
                argsp, নূতন AstArg{flp, "", নূতন AstVarRef{flp, newPortp, VAccess::READ}});
        }
        Ast_গ্রন্থি_FTask_Ref* callp;
        যদি (AstFunc* ধ্রুব implFuncp = VN_CAST(implp, Func)) {
            callp = নূতন AstFuncRef{flp, implFuncp, argsp};
        } অন্যথা {
            callp = নূতন AstTaskRef{flp, VN_AS(implp, Task), argsp};
        }
        callp->classOrPackagep(implClassp);
        callp->superReference(সত্য);
        যদি (fvarp) {
            newp->addStmtsp(নূতন AstAssign{flp, নূতন AstVarRef{flp, fvarp, VAccess::WRITE}, callp});
        } অন্যথা {
            newp->addStmtsp(callp->makeStmt());
        }
        UINFO(9, indent() << "Interface forward " << newp);
        classp->addMembersp(newp);
    }
    শূন্য importDerivedClass(AstClass* derivedClassp, VSymEnt* baseSymp, AstClass* baseClassp) {
        // Also used for standard 'extends' from a base class
        UINFO(8, indent() << "importDerivedClass to " << derivedClassp << " from " << baseClassp);
        প্রতি (VSymEnt::const_iterator it = baseSymp->begin(); it != baseSymp->end(); ++it) {
            যদি (Ast_গ্রন্থি* baseSubp = it->second->nodep()) {
                UINFO(8, indent() << "  SymFunc " << baseSubp);
                ধ্রুব string impOrExtends
                    = baseClassp->isInterfaceClass() ? " implements " : " extends ";
                যদি (VN_IS(baseSubp, গ্রন্থি_FTask)) {
                    ধ্রুব VSymEnt* ধ্রুব foundp = m_curSymp->findIdFlat(baseSubp->name());
                    ধ্রুব Ast_গ্রন্থি_FTask* ধ্রুব baseFuncp = VN_CAST(baseSubp, গ্রন্থি_FTask);
                    যদি (!baseFuncp || !baseFuncp->pureVirtual()) অনুবর্ত;
                    তার্কিক needForward = অসত্য;
                    ধ্রুব তার্কিক existsInDerived
                        = foundp
                          && (!foundp->imported()
                              || inheritedImplements(foundp, it->second, needForward));
                    যদি (needForward && m_statep->forPrimary()
                        && !derivedClassp->isInterfaceClass()) {
                        m_ifForwards.emplace(baseSubp->name(), foundp);
                    }
                    যদি (m_statep->forPrimary() && !existsInDerived
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
                    ধ্রুব auto itn = m_ifClassImpNames.find(baseSubp->name());
                    যদি (m_statep->forPrimary() && !existsInDerived
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
                যদি (VN_IS(baseSubp, Constraint)) {
                    ধ্রুব VSymEnt* ধ্রুব foundp = m_curSymp->findIdFlat(baseSubp->name());
                    ধ্রুব AstConstraint* ধ্রুব baseFuncp = VN_CAST(baseSubp, Constraint);
                    যদি (!baseFuncp || !baseFuncp->isKwdPure()) অনুবর্ত;
                    ধ্রুব তার্কিক existsInDerived = foundp && !foundp->imported();
                    যদি (m_statep->forPrimary() && !existsInDerived
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
    শূন্য importSymbolsFromExtended(AstClass* ধ্রুব nodep, AstClassExtends* ধ্রুব cextp) {
        AstClass* ধ্রুব baseClassp = cextp->classp();
        VSymEnt* ধ্রুব srcp = m_statep->get_গ্রন্থি_Sym(baseClassp);
        importDerivedClass(nodep, srcp, baseClassp);
        যদি (!cextp->isImplements()) m_curSymp->importFromClass(m_statep->symsp(), srcp);
    }
    শূন্য classExtendImport(AstClass* nodep) {
        // A class reference might be to a class that is later in Ast due to
        // e.g. parameterization or referring to a "class (type T) extends T"
        // Resolve it so later Class:: references into its base classes work
        symIterateNull(nodep, m_statep->get_গ্রন্থি_Sym(nodep));
    }

    শূন্য checkDeclOrder(Ast_গ্রন্থি* nodep, Ast_গ্রন্থি* declp) {
        ধ্রুব uint32_t decl_Token_অঙ্ক = declp->decl_Token_অঙ্ক();
        যদি (!decl_Token_অঙ্ক) প্রত্যাগম;  // Not implemented/valid on this object
        যদি (nodep->fileline()->token_অঙ্ক() < decl_Token_অঙ্ক) {
            UINFO(1, "Related node " << nodep->fileline()->token_অঙ্ক() << " " << nodep);
            UINFO(1, "Related decl " << decl_Token_অঙ্ক << " " << declp);
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
    শূন্য checkMemberDeclOrder(Ast_গ্রন্থি* nodep, AstMemberDType* declp) {
        ধ্রুব uint32_t decl_Token_অঙ্ক = declp->fileline()->token_অঙ্ক();
        যদি (nodep->fileline()->token_অঙ্ক() < decl_Token_অঙ্ক) {
            UINFO(1, "Related node " << nodep->fileline()->token_অঙ্ক() << " " << nodep);
            UINFO(1, "Related decl " << decl_Token_অঙ্ক << " " << declp);
            nodep->v3error("Reference to "
                           << nodep->prettyNameQ() << " before declaration (IEEE 1800-2023 6.18)\n"
                           << nodep->warnMore()
                           << "... Suggest move the declaration before the reference\n"
                           << nodep->warnContextPrimary() << '\n'
                           << declp->warnOther() << "... Location of original declaration\n"
                           << declp->warnContextSecondary());
        }
    }

    শূন্য replaceWithCheckBreak(Ast_গ্রন্থি* oldp, Ast_গ্রন্থি_DType* newp) {
        // Flag now to avoid V3Broken throwing an internal error
        যদি (oldp->wouldBreak(newp)) {
            newp->v3error(
                "Data type used where a non-data type is expected: " << newp->prettyNameQ());
            oldp->replaceWith(নূতন AstConst{newp->fileline(), AstConst::BitFalseErroring{}});
        } অন্যথা {
            oldp->replaceWith(newp);
        }
    }

    // Marks the current module to be revisited after the initial AST iteration
    শূন্য revisitLater(Ast_গ্রন্থি* deferredNodep) {
        // Need to revisit entire module to build up all the necessary context
        m_lastDeferredp = deferredNodep;
        m_modulesToRevisit.ঢোকাও(std::make_pair(m_modp->name(), m_modp));
    }

    শূন্য updateVarUse(AstVar* nodep) {
        // Avoid dotted.PARAM false positive when in a parameter block
        // that is if ()'ed off by same dotted name as another block
        যদি (nodep && nodep->isParam()) nodep->usedParam(সত্য);
    }

    স্থির VSymEnt* findIdFallbackSkipMemberDType(VSymEnt* lookp, ধ্রুব string& name) {
        VSymEnt* shadowEntp = nullptr;  // Shadowing variable: not a type, kept for error report
        যাবত্ (lookp) {
            VSymEnt* ধ্রুব foundp = lookp->findIdFlat(name);
            যদি (foundp && !VN_IS(foundp->nodep(), MemberDType)) {
                // Non-type entries are not type candidates
                // (IEEE 1800-2023 6.18); skip them so an enclosing
                // type is found, but keep one to preserve the "found: ..." error.
                যদি (VN_IS(foundp->nodep(), Typedef) || VN_IS(foundp->nodep(), ParamTypeDType)
                    || VN_IS(foundp->nodep(), Class)) {
                    প্রত্যাগম foundp;
                }
                যদি (!shadowEntp) shadowEntp = foundp;
            }
            lookp = lookp->fallbackp();
        }
        প্রত্যাগম shadowEntp;
    }

    শূন্য symIterateChildren(Ast_গ্রন্থি* nodep, VSymEnt* symp) {
        // Iterate children, changing to given context, with restore to old context
        VL_RESTORER_COPY(m_ds);
        VL_RESTORER(m_curSymp);
        m_curSymp = symp;
        m_ds.init(m_curSymp);
        iterateChildren(nodep);
    }
    শূন্য symIterateNull(Ast_গ্রন্থি* nodep, VSymEnt* symp) {
        // Iterate node, changing to given context, with restore to old context
        VL_RESTORER_COPY(m_ds);
        VL_RESTORER(m_curSymp);
        m_curSymp = symp;
        m_ds.init(m_curSymp);
        iterateNull(nodep);
    }
    স্থির ধ্রুব Ast_গ্রন্থি_DType* get_রাশিমালা_DTypep(ধ্রুব Ast_গ্রন্থি_রাশিমালা* selp) {
        যাবত্ (ধ্রুব Ast_গ্রন্থি_Pre_Sel* ধ্রুব sp = VN_CAST(selp, গ্রন্থি_Pre_Sel)) selp = sp->fromp();
        যদি (ধ্রুব AstMemberSel* ধ্রুব sp = VN_CAST(selp, MemberSel)) {
            যদি (ধ্রুব Ast_গ্রন্থি_DType* dtypep = get_রাশিমালা_DTypep(sp->fromp())) {
                যদি (ধ্রুব AstClassRefDType* ধ্রুব classRefp = VN_CAST(dtypep, ClassRefDType)) {
                    ধ্রুব AstClass* ধ্রুব classp = classRefp->classp();
                    ধ্রুব তার্কিক found = classp->existsMember(
                        [&dtypep, name = selp->name()](ধ্রুব AstClass*, ধ্রুব AstVar* nodep) {
                            dtypep = nodep->childDTypep();
                            প্রত্যাগম nodep->name() == name;
                        });
                    যদি (found) প্রত্যাগম dtypep->elemDTypep();
                    selp->v3error("Class " << classRefp->prettyNameQ()
                                           << " does not contain field " << selp->prettyNameQ());
                } অন্যথা {
                    selp->v3fatalSrc("Member selection on expression of type "
                                     << dtypep->prettyDTypeNameQ()
                                     << ", which is not a class type");
                }
            }
        } অন্যথা যদি (ধ্রুব Ast_গ্রন্থি_Var_Ref* ধ্রুব varRefp = VN_CAST(selp, গ্রন্থি_Var_Ref)) {
            প্রত্যাগম varRefp->varp()->childDTypep()->elemDTypep();
        }
        প্রত্যাগম nullptr;
    }
    স্থির ধ্রুব AstVar* getNextVarp(ধ্রুব Ast_গ্রন্থি* stmtsp) {
        // Only IO ports, as parameters are on paramsp(), not the pinsp() list paired here
        যাবত্ (stmtsp) {
            যদি (ধ্রুব AstVar* ধ্রুব varp = VN_CAST(stmtsp, Var)) {
                যদি (varp->isIO()) প্রত্যাগম varp;
            }
            stmtsp = stmtsp->nextp();
        }
        প্রত্যাগম nullptr;
    }
    // Introduce implicit parameters for modules with generic interafeces
    শূন্য addImplicitParametersOfGenericIface(AstCell* ধ্রুব nodep, ধ্রুব AstModule* ধ্রুব modp) {
        // Get Param number
        পূর্ণাঙ্ক param_অঙ্ক = 1;
        প্রতি (AstPin* paramp = nodep->paramsp(); paramp; paramp = VN_CAST(paramp->nextp(), Pin)) {
            ++param_অঙ্ক;
        }

        // Add each implicit parameter type
        ধ্রুব AstVar* modIfaceVarp = getNextVarp(modp->stmtsp());
        প্রতি (ধ্রুব AstPin* pinp = nodep->pinsp(); pinp && modIfaceVarp;
             pinp = VN_CAST(pinp->nextp(), Pin),
                           modIfaceVarp = getNextVarp(modIfaceVarp->nextp())) {
            যদি (modIfaceVarp->varType() != VVarType::IFACEREF
                || !VN_IS(modIfaceVarp->childDTypep()->elemDTypep(), IfaceGenericDType)) {
                অনুবর্ত;
            }
            Ast_গ্রন্থি* exprp = pinp->exprp();
            যদি (!exprp) {
                modIfaceVarp->v3error("Interface port "
                                      << modIfaceVarp->prettyNameQ()
                                      << " is not connected to interface/modport pin expression");
                অনুবর্ত;
            }
            যাবত্ (ধ্রুব Ast_গ্রন্থি_Pre_Sel* ধ্রুব preSelp = VN_CAST(exprp, গ্রন্থি_Pre_Sel)) {
                exprp = preSelp->fromp();
            }
            // Resolve pin expression to the enclosing module's port Var. At primary
            // LinkDot the expression may still be an AstParseRef, so also look up by name.
            AstVar* enclosingVarp = nullptr;
            ধ্রুব AstVarRef* ধ্রুব varRefp = VN_CAST(exprp, VarRef);
            যদি (varRefp) {
                enclosingVarp = varRefp->varp();
            } অন্যথা যদি (ধ্রুব AstParseRef* ধ্রুব parseRefp = VN_CAST(exprp, ParseRef)) {
                যদি (m_modp) {
                    যদি (VSymEnt* ধ্রুব symp
                        = m_statep->get_গ্রন্থি_Sym(m_modp)->findIdFlat(parseRefp->name())) {
                        enclosingVarp = VN_CAST(symp->nodep(), Var);
                    }
                }
            }
            যদি (enclosingVarp && enclosingVarp->varType() == VVarType::IFACEREF
                && VN_IS(enclosingVarp->childDTypep()->skipRefp(), IfaceGenericDType)) {
                // Nested generic-iface forwarding (#7454): enclosing port is itself still
                // generic, so emit a placeholder __VGIfaceParam pin carrying a VarRef to
                // the outer port. V3Param rewrites it to the concrete IfaceRefDType once
                // the enclosing module is specialized (see
                // ParamProcessor::resolveGenericIfaceForwardingPins for the ordering
                // constraint that keeps the rewrite inside V3Param).
                AstVarRef* ধ্রুব fwdRefp
                    = নূতন AstVarRef{exprp->fileline(), enclosingVarp, VAccess::READ};
                AstPin* ধ্রুব newPinp = নূতন AstPin{
                    pinp->fileline(), param_অঙ্ক, "__VGIfaceParam" + modIfaceVarp->name(), fwdRefp};
                newPinp->param(সত্য);
                visit(newPinp);
                nodep->addParamsp(newPinp);
            } অন্যথা যদি (varRefp) {
                ধ্রুব AstVar* ধ্রুব varp = varRefp->varp();
                যদি (ধ্রুব AstIfaceRefDType* ধ্রুব refp
                    = VN_CAST(varp->childDTypep()->elemDTypep(), IfaceRefDType)) {
                    AstIface* ধ্রুব ifacep = VN_AS(refp->cellp()->modp(), Iface);
                    AstIfaceRefDType* newIfaceRefp;
                    যদি (refp->modportp()) {
                        newIfaceRefp = নূতন AstIfaceRefDType{
                            refp->fileline(), refp->modportFileline(), m_modp->name(),
                            ifacep->name(), refp->modportName()};
                    } অন্যথা {
                        newIfaceRefp = নূতন AstIfaceRefDType{refp->fileline(), m_modp->name(),
                                                            ifacep->name()};
                    }
                    newIfaceRefp->ifacep(ifacep);
                    যদি (refp->cellp()->paramsp()) {
                        newIfaceRefp->addParamsp(refp->cellp()->paramsp()->clone_বৃক্ষ(সত্য));
                    }
                    UASSERT_OBJ(pinp->name().find("__pinNumber") == 0
                                    || pinp->name() == modIfaceVarp->name(),
                                pinp, "Not found interface with such name");
                    AstPin* ধ্রুব newPinp
                        = নূতন AstPin{pinp->fileline(), param_অঙ্ক,
                                     "__VGIfaceParam" + modIfaceVarp->name(), newIfaceRefp};
                    newPinp->param(সত্য);
                    visit(newPinp);
                    nodep->addParamsp(newPinp);
                } অন্যথা {
                    varRefp->v3error("Generic interfaces can only connect to an interface and "
                                     << varp->prettyNameQ() << " is "
                                     << (varp->childDTypep()
                                             ? "of type " + varp->childDTypep()->prettyDTypeNameQ()
                                             : "not an interface"));
                }
            } অন্যথা {
                exprp->v3error("Expected an interface but " << exprp->prettyNameQ()
                                                            << " is not an interface");
            }
            ++param_অঙ্ক;
        }
    }

    // Returns true and issues error iff 'varp' cannot be accesed with a VarXRef
    // in a hier_block given by 'scopeSymp'
    তার্কিক errorHierNonPort(AstVarXRef* refp, AstVar* varp, VSymEnt* scopeSymp) {
        // OK to access ports on hier_block
        যদি (varp->isIO()) প্রত্যাগম অসত্য;
        // OK if not an instance
        ধ্রুব AstCell* ধ্রুব cellp = VN_CAST(scopeSymp->nodep(), Cell);
        যদি (!cellp) প্রত্যাগম অসত্য;
        // Ok if not a hier_block
        ধ্রুব Ast_গ্রন্থি_Module* ধ্রুব modp = cellp->modp();
        যদি (!modp->hierBlock()) প্রত্যাগম অসত্য;
        // Bad
        refp->v3error("Cannot access non-port symbols inside hierarchical block");
        প্রত্যাগম সত্য;
    }

#define LINKDOT_VISIT_START() \
    VL_RESTORER(m_indent); \
    ++m_indent;

    string indent() ধ্রুব {
        string result = "";
        result.ঢোকাও(0, m_indent, ':');
        প্রত্যাগম result + " ";
    }

    // VISITORS
    শূন্য visit(AstNetlist* nodep) override {
        // Recurse..., backward as must do packages before using packages
        iterateChildrenBackwardsConst(nodep);
    }
    শূন্য visit(AstTypeTable*) override {}
    শূন্য visit(Ast_গ্রন্থি_Module* nodep) override {
        যদি (nodep->dead()) প্রত্যাগম;
        LINKDOT_VISIT_START();
        UINFO(8, indent() << "visit " << nodep);
        VL_RESTORER(m_genericIfaceModule);
        যদি (ধ্রুব AstModule* ধ্রুব modp = VN_CAST(nodep, Module)) {
            m_genericIfaceModule = modp->hasGenericIface();
        }
        checkNoDot(nodep);
        m_ds.init(m_curSymp);
        m_ds.m_dotSymp = m_curSymp = m_modSymp
            = m_statep->get_গ্রন্থি_Sym(nodep);  // Until overridden by a SCOPE
        m_cellp = nullptr;
        m_modp = nodep;
        m_modport_অঙ্ক = 0;
        iterateChildren(nodep);
        m_modp = nullptr;
        m_ds.m_dotSymp = m_curSymp = m_modSymp = nullptr;
        m_ds.m_dotPos = DP_NONE;
        m_ds.m_dotErr = অসত্য;
    }
    শূন্য visit(AstScope* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(8, indent() << "visit " << nodep);
        checkNoDot(nodep);
        VL_RESTORER(m_modSymp);
        VL_RESTORER(m_curSymp);
        m_ds.m_dotSymp = m_curSymp = m_modSymp = m_statep->getScopeSym(nodep);
        iterateChildren(nodep);
        m_ds.m_dotSymp = m_curSymp = m_modSymp = nullptr;
    }
    শূন্য visit(AstCellInline* nodep) override {
        LINKDOT_VISIT_START();
        checkNoDot(nodep);
        UASSERT_OBJ(m_statep->forScopeCreation(), nodep, "CellInline should not exist yet");
        VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);
    }
    শূন্য visit(AstCell* nodep) override {
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
            যদি (VN_IS(nodep->modp(), NotFoundModule)) {
                // Prevent warnings about missing pin connects
                যদি (nodep->pinsp()) nodep->pinsp()->unlinkFrBackWithNext()->delete_বৃক্ষ();
                যদি (nodep->paramsp()) nodep->paramsp()->unlinkFrBackWithNext()->delete_বৃক্ষ();
            }
            // Need to pass the module info to this cell, so we can link up the pin names
            // However can't use m_curSymp as pin connections need to use the
            // instantiator's symbols
            অন্যথা {
                m_pinSymp = m_statep->get_গ্রন্থি_Sym(nodep->modp());
                UINFO(4, indent() << "(Backto) visit " << nodep);
                // UINFOTREE(1, nodep, "", "linkcell");
                // UINFOTREE(1, nodep->modp(), "", "linkcemd");
                iterateChildren(nodep);

                যদি (m_statep->forPrimary())
                    যদি (ধ্রুব AstModule* ধ্রুব modp = VN_CAST(nodep->modp(), Module)) {
                        যদি (modp->hasGenericIface())
                            addImplicitParametersOfGenericIface(nodep, modp);
                    }
            }
        }
        // Parent module inherits child's publicity
        // This is done bottom up in the LinkBotupVisitor stage
    }
    শূন্য visit(AstClassRefDType* nodep) override {
        // Cell: Recurse inside or cleanup not founds
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        // Can be under dot if called as package::class and that class resolves, so no checkNoDot
        VL_RESTORER_CLEAR(m_usedPins);
        VL_RESTORER_CLEAR(m_usedDefParamPins);
        UASSERT_OBJ(nodep->classp(), nodep, "ClassRef has unlinked class");
        UASSERT_OBJ(m_statep->forPrimary() || !nodep->paramsp() || V3Error::error_গুনতি(), nodep,
                    "class reference parameter not removed by V3Param");
        VL_RESTORER(m_pinSymp);
        {
            // ClassRef's have pins, so track
            m_pinSymp = m_statep->get_গ্রন্থি_Sym(nodep->classp());
            UINFO(4, indent() << "(Backto) visit " << nodep);
            // UINFOTREE(1, nodep, "", "linkcell");
            // UINFOTREE(1, nodep->modp(), "", "linkcemd");
            iterateChildren(nodep);
        }
    }
    শূন্য visit(AstPin* nodep) override {
        // Pin: Link to submodule's port
        LINKDOT_VISIT_START();
        checkNoDot(nodep);
        iterateChildren(nodep);
        যদি (!nodep->modVarp()) {
            UASSERT_OBJ(m_pinSymp, nodep, "Pin not under instance?");
            VSymEnt* ধ্রুব foundp = m_pinSymp->findIdFlat(nodep->name());
            ধ্রুব বর্ণ* ধ্রুব whatp = nodep->param() ? "parameter" : "pin";
            যদি (!foundp) {
                যদি (m_cellp && VN_IS(m_cellp->modp(), Primitive)
                    && (nodep->name() == "__paramNumber1" || nodep->name() == "__paramNumber2")) {
                    // Primitive parameter is really a delay2 we can just ignore
                    VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);
                    প্রত্যাগম;
                } অন্যথা যদি (nodep->param() && !nodep->exprp() && !nodep->svDotName()) {
                    // Placeholder pin the parser makes for an empty '#()', to signal
                    // the user did type the '#()'. No further use, deleting.
                    VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);
                    প্রত্যাগম;
                } অন্যথা {
                    ধ্রুব std::string suggest
                        = (nodep->param()
                               ? m_statep->suggestSymFlat(m_pinSymp, nodep->name(),
                                                          Link_গ্রন্থি_Matcher_Var_Param{}, অসত্য)
                               : m_statep->suggestSymFlat(m_pinSymp, nodep->name(),
                                                          Link_গ্রন্থি_Matcher_Var_IO{}, অসত্য));
                    ধ্রুব std::string decl
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
                    VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);
                    প্রত্যাগম;
                }
            }
            VVarType refVarType = VVarType::UNKNOWN;
            তার্কিক wrongPinType = অসত্য;
            যদি (AstVar* ধ্রুব varp = VN_CAST(foundp->nodep(), Var)) {
                যদি (varp->isIO() || varp->isParam() || varp->isIfaceRef()) {
                    refVarType = varp->varType();
                    nodep->modVarp(varp);
                } অন্যথা {
                    wrongPinType = সত্য;
                }
            } অন্যথা যদি (AstParamTypeDType* ধ্রুব typep = VN_CAST(foundp->nodep(), ParamTypeDType)) {
                refVarType = typep->varType();
                nodep->modPTypep(typep);
            } অন্যথা {
                wrongPinType = সত্য;
            }
            // Don't connect parameter pin to module ports or vice versa
            যদি (nodep->param() != (refVarType == VVarType::GPARAM)) wrongPinType = সত্য;
            যদি (wrongPinType) {
                string targetType = LinkDotState::গ্রন্থি_Text_Type(foundp->nodep());
                targetType = VString::aOrAn(targetType) + ' ' + targetType;
                যদি (nodep->param()) {
                    nodep->v3error("Instance attempts to override "
                                   << nodep->prettyNameQ() << " as a " << whatp << ", but it is "
                                   << targetType);
                } অন্যথা {
                    nodep->v3error("Instance attempts to connect to "
                                   << nodep->prettyNameQ() << ", but it is " << targetType);
                }
                প্রত্যাগম;
            }
            markAndCheckPinDup(nodep, foundp->nodep(), whatp);
        }
        // Early return() above when deleted
    }

    স্থির তার্কিক isDotThisReference(ধ্রুব AstDot* nodep) {
        প্রত্যাগম VN_IS(nodep->lhsp(), ParseRef) && nodep->lhsp()->name() == "this";
    }

    শূন্য visit(AstDot* nodep) override {
        // Legal under a DOT: AstDot, AstParseRef, AstPackageRef, AstNodeSel, AstPackArrayDType
        //    also a DOT can be part of an expression, but only above plus
        //    AstFTaskRef are legal children
        // Dot(PackageRef, ParseRef(text))
        // Dot(Dot(ClassOrPackageRef,ClassOrPackageRef), ParseRef(text))
        // Dot(Dot(Dot(ParseRef(text), ...
        যদি (nodep->user3SetOnce()) প্রত্যাগম;
        LINKDOT_VISIT_START();
        UINFO(8, indent() << "visit " << nodep);
        UINFO(8, indent() << m_ds.ascii());
        ধ্রুব DotStates lastStates = m_ds;
        ধ্রুব তার্কিক start = (m_ds.m_dotPos == DP_NONE);  // Save, as m_dotp will be changed
        ধ্রুব DotPosition initialDotPos = m_ds.m_dotPos;
        VL_RESTORER(m_randSymp);
        {
            যদি (start) {  // Starting dot sequence
                UINFOTREE(9, nodep, "", "dot-in");
                // Keep deferral from visit(AstMethodCall) for unspecialized rand target
                m_ds.init(m_curSymp, m_ds.m_unresolvedClass && randTargetUnresolved());
            }
            m_ds.m_dotp = nodep;  // Always, not just at start
            m_ds.m_dotPos = DP_FIRST;

            যদি (isDotThisReference(nodep)) {
                VSymEnt* classSymp = getThisClassSymp();
                // In 'randomize() with { this.member }', 'this' refers to randomized
                // object, not the calling class (IEEE 1800-2023 18.7)
                যদি (m_randSymp && m_currentWithp) classSymp = m_randSymp;
                যদি (!classSymp && randTargetUnresolved()) {
                    // Bind 'this' in post-V3Param pass
                } অন্যথা যদি (!classSymp) {
                    nodep->v3error("'this' used outside class (IEEE 1800-2023 8.11)");
                    m_ds.m_dotErr = সত্য;
                } অন্যথা {
                    m_ds.m_dotSymp = classSymp;
                    UINFO(8, indent() << "this. " << m_ds.ascii());
                }
            } অন্যথা যদি (VN_IS(nodep->lhsp(), ParseRef) && nodep->lhsp()->name() == "super") {
                ধ্রুব VSymEnt* classSymp = getThisClassSymp();
                যদি (!classSymp) {
                    nodep->v3error("'super' used outside class (IEEE 1800-2023 8.15)");
                    m_ds.m_dotErr = সত্য;
                } অন্যথা {
                    ধ্রুব auto classp = VN_AS(classSymp->nodep(), Class);
                    ধ্রুব auto cextp = classp->extendsp();
                    যদি (!cextp) {
                        nodep->v3error("'super' used on non-extended class (IEEE 1800-2023 8.15)");
                        m_ds.m_dotErr = সত্য;
                    } অন্যথা {
                        যদি (m_statep->forPrimary()
                            && m_extendsParam.find(classp) != m_extendsParam.end()) {
                            UINFO(9, indent()
                                         << "deferring until post-V3Param: " << nodep->lhsp());
                            m_ds.m_unresolvedClass = সত্য;
                        } অন্যথা {
                            ধ্রুব auto baseClassp = cextp->classp();
                            UASSERT_OBJ(baseClassp, nodep, "Bad superclass");
                            m_ds.m_dotSymp = m_statep->get_গ্রন্থি_Sym(baseClassp);
                            m_ds.m_super = সত্য;
                            UINFO(8, indent() << "super. " << m_ds.ascii());
                        }
                    }
                }
            } অন্যথা যদি (AstClassOrPackageRef* ধ্রুব lhsp
                       = VN_CAST(nodep->lhsp(), ClassOrPackageRef)) {
                // m_ds.m_dotText communicates the cell prefix between stages
                UINFO(8, indent() << "iter.lhs   " << m_ds.ascii() << " " << nodep);
                iterateAndNextNull(lhsp);
                যদি (!lhsp->classOrPackageSkipp() && lhsp->name() != "local::") {
                    revisitLater(nodep);
                    m_ds = lastStates;
                    // Resolve function args before bailing
                    যদি (Ast_গ্রন্থি_FTask_Ref* ধ্রুব ftaskrefp = VN_CAST(nodep->rhsp(), গ্রন্থি_FTask_Ref)) {
                        iterateAndNextNull(ftaskrefp->argsp());
                    }
                    প্রত্যাগম;
                }
                m_ds.m_dotPos = DP_PACKAGE;
                // nodep->lhsp() may be a new node
                যদি (AstClassOrPackageRef* ধ্রুব classOrPackageRefp
                    = VN_CAST(nodep->lhsp(), ClassOrPackageRef)) {
                    যদি (Ast_গ্রন্থি* ধ্রুব classOrPackageNodep
                        = classOrPackageRefp->classOrPackageSkipp()) {
                        m_ds.m_dotSymp = m_statep->get_গ্রন্থি_Sym(classOrPackageNodep);
                    }
                }
                UINFO(8, indent() << "iter.ldone " << m_ds.ascii() << " " << nodep);
            } অন্যথা যদি (VN_IS(nodep->lhsp(), Dot) && VN_AS(nodep->lhsp(), Dot)->colon()) {
                // m_ds.m_dotText communicates the cell prefix between stages
                UINFO(8, indent() << "iter.lhs   " << m_ds.ascii() << " " << nodep);
                m_ds.m_dotPos = DP_PACKAGE;
                iterateAndNextNull(nodep->lhsp());
                // nodep->lhsp() may be a new node
                যদি (AstClassOrPackageRef* ধ্রুব crefp
                    = VN_CAST(nodep->lhsp(), ClassOrPackageRef)) {
                    যদি (!crefp->classOrPackageSkipp()) {
                        revisitLater(nodep);
                        m_ds = lastStates;
                        // Resolve function args before bailing
                        যদি (Ast_গ্রন্থি_FTask_Ref* ধ্রুব ftaskrefp
                            = VN_CAST(nodep->rhsp(), গ্রন্থি_FTask_Ref)) {
                            iterateAndNextNull(ftaskrefp->argsp());
                        }
                        প্রত্যাগম;
                    }
                }
                যদি (m_lastDeferredp == nodep->lhsp()) {
                    // LHS got deferred, so this node won't be resolved. Defer it too
                    m_ds = lastStates;
                    // Resolve function args before bailing
                    যদি (Ast_গ্রন্থি_FTask_Ref* ধ্রুব ftaskrefp = VN_CAST(nodep->rhsp(), গ্রন্থি_FTask_Ref)) {
                        iterateAndNextNull(ftaskrefp->argsp());
                    }
                    প্রত্যাগম;
                }
                UINFO(8, indent() << "iter.ldone " << m_ds.ascii() << " " << nodep);
            } অন্যথা {
                m_ds.m_dotPos = DP_FIRST;
                m_ds.m_unresolvedGenericIface = অসত্য;
                UINFO(8, indent() << "iter.lhs   " << m_ds.ascii() << " " << nodep);
                iterateAndNextNull(nodep->lhsp());
                UINFO(8, indent() << "iter.ldone " << m_ds.ascii() << " " << nodep);
                যদি (m_ds.m_unresolvedGenericIface) {
                    m_ds.m_dotPos = initialDotPos;
                    প্রত্যাগম;
                }
                // UINFOTREE(9, nodep, "", "dot-lho");
            }
            যদি (m_statep->forPrimary() && isParamedClassRef(nodep->lhsp())) {
                // Dots of paramed classes will be linked after deparameterization
                UINFO(9, indent() << "deferring until post-V3Param: " << nodep->lhsp());
                m_ds.m_unresolvedClass = সত্য;
            }
            যদি (m_ds.m_unresolvedCell
                && (VN_IS(nodep->lhsp(), CellRef) || VN_IS(nodep->lhsp(), CellArrayRef))) {
                m_ds.m_unlinkedScopep = nodep->lhsp();
            }
            যদি (m_ds.m_dotPos == DP_FIRST) m_ds.m_dotPos = DP_SCOPE;
            যদি (!m_ds.m_dotErr) {  // Once something wrong, give up
                // Top 'final' dot RHS is final RHS, else it's a
                // DOT(DOT(x,*here*),real-rhs) which we consider a RHS
                যদি (start && m_ds.m_dotPos == DP_SCOPE) m_ds.m_dotPos = DP_FINAL;
                UINFO(8, indent() << "iter.rhs   " << m_ds.ascii() << " " << nodep);
                // m_ds.m_dotSymp points at lhsp()'s symbol table, so resolve RHS under that
                iterateAndNextNull(nodep->rhsp());
                UINFO(8, indent() << "iter.rdone " << m_ds.ascii() << " " << nodep);
                // UINFOTREE(9, nodep, "", "dot-rho");
            }
            যদি (!m_ds.m_unresolvedClass) {
                যদি (start) {
                    Ast_গ্রন্থি* newp;
                    যদি (m_ds.m_dotErr) {
                        newp = নূতন AstConst{nodep->fileline(), AstConst::BitFalse{}};
                    } অন্যথা {
                        newp = nodep->rhsp()->unlinkFrBack();
                    }
                    UINFOTREE(9, newp, "", "dot-out");
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                } অন্যথা {  // Dot midpoint
                    Ast_গ্রন্থি* newp = nodep->rhsp()->unlinkFrBack();
                    যদি (m_ds.m_unresolvedCell) {
                        AstCellRef* ধ্রুব crp = নূতন AstCellRef{
                            nodep->fileline(), nodep->name(), nodep->lhsp()->unlinkFrBack(), newp};
                        newp = crp;
                    }
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                }
            }
        }
        যদি (start) {
            m_ds = lastStates;
        } অন্যথা {
            ধ্রুব তার্কিক unresolvedClass = m_ds.m_unresolvedClass;
            m_ds.m_dotp = lastStates.m_dotp;
            m_ds.m_unresolvedClass |= unresolvedClass;
        }
        UINFO(8, indent() << "done " << m_ds.ascii() << " " << nodep);
    }
    শূন্য visit(AstDefaultClocking* nodep) override {
        যদি (VSymEnt* ধ্রুব foundp = m_curSymp->findIdFallback(nodep->name())) {
            যদি (AstClocking* ধ্রুব clockingp = VN_CAST(foundp->nodep(), Clocking)) {
                clockingp->makeDefault();
                VL_DO_DANGLING(nodep->unlinkFrBack()->delete_বৃক্ষ(), nodep);
            } অন্যথা {
                nodep->v3error(nodep->prettyNameQ() << " is not a clocking identifier");
            }
        } অন্যথা {
            nodep->v3error("Can't find definition of clocking: " << nodep->prettyNameQ());
        }
    }
    শূন্য visit(AstSenItem* nodep) override {
        LINKDOT_VISIT_START();
        VL_RESTORER(m_inSens);
        m_inSens = সত্য;
        iterateChildren(nodep);
    }
    শূন্য visit(AstPatMember* nodep) override {
        LINKDOT_VISIT_START();
        যদি (nodep->varrefp()) প্রত্যাগম;  // only do this mapping once
        // If we have a TEXT token as our key, lookup if it's a LPARAM
        যদি (AstText* ধ্রুব textp = VN_CAST(nodep->keyp(), Text)) {
            UINFO(9, indent() << "visit " << nodep);
            UINFO(9, indent() << "      " << textp);
            // Lookup
            যদি (VSymEnt* ধ্রুব foundp = m_curSymp->findIdFallback(textp->text())) {
                যদি (AstVar* ধ্রুব varp = VN_CAST(foundp->nodep(), Var)) {
                    যদি (varp->isParam() || varp->isGenVar()) {
                        // Attach found Text reference to PatMember
                        nodep->varrefp(
                            নূতন AstVarRef{nodep->fileline(),
                                          foundp->imported() ? foundp->classOrPackagep() : nullptr,
                                          varp, VAccess::READ});
                        UINFO(9, indent() << " new " << nodep->varrefp());
                    }
                }
                যদি (AstEnumItem* ধ্রুব itemp = VN_CAST(foundp->nodep(), EnumItem)) {
                    // Attach enum item value to PatMember
                    nodep->varrefp(
                        নূতন AstEnumItemRef{nodep->fileline(), itemp, foundp->classOrPackagep()});
                    UINFO(9, indent() << " new " << itemp);
                }
            }
        }
        iterateChildren(nodep);
    }
    শূন্য visit(AstParseRef* nodep) override {
        যদি (nodep->user3SetOnce()) প্রত্যাগম;
        LINKDOT_VISIT_START();
        UINFO(9, indent() << "visit " << nodep);
        UINFO(9, indent() << m_ds.ascii());
        যদি (m_ds.m_unresolvedClass) প্রত্যাগম;
        // m_curSymp is symbol table of outer expression
        // m_ds.m_dotSymp is symbol table relative to "."'s above now
        UASSERT_OBJ(m_ds.m_dotSymp, nodep, "nullptr lookup symbol table");
        // Generally resolved during Primary, but might be at param time under AstUnlinkedRef
        UASSERT_OBJ(!m_statep->forScopeCreation(), nodep, "ParseRefs should no longer exist");
        ধ্রুব DotStates lastStates = m_ds;
        ধ্রুব তার্কিক start = (m_ds.m_dotPos == DP_NONE);  // Save, as m_dotp will be changed
        তার্কিক first = start || m_ds.m_dotPos == DP_FIRST;

        যদি (start) {
            m_ds.init(m_curSymp);
            // Note m_ds.m_dotp remains nullptr; this is a reference not under a dot
        }
        যদি (nodep->name() == "super") {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: super");
            m_ds.m_dotErr = সত্য;
        }
        যদি (nodep->name() == "this") {
            iterateChildren(nodep);
            যদি (m_statep->forPrimary()) প্রত্যাগম;  // The class might be parameterized somewhere
            ধ্রুব VSymEnt* classSymp = getThisClassSymp();
            যদি (!classSymp) {
                nodep->v3error("'this' used outside class (IEEE 1800-2023 8.11)");
                প্রত্যাগম;
            }
            AstClass* ধ্রুব classp = VN_AS(classSymp->nodep(), Class);
            AstClassRefDType* ধ্রুব dtypep
                = নূতন AstClassRefDType{nodep->fileline(), classp, nullptr};
            AstThisRef* ধ্রুব newp = নূতন AstThisRef{nodep->fileline(), VFlagChildDType{}, dtypep};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
            প্রত্যাগম;
        }
        যদি (m_ds.m_dotPos == DP_MEMBER && VN_IS(m_ds.m_dotp->lhsp(), LambdaArgRef)
            && nodep->name() == "index") {
            // 'with' statement's 'item.index'
            // m_ds.dotp->lhsp() was checked to know if `index` is directly after lambda arg ref.
            // If not, treat it as normal member select
            iterateChildren(nodep);
            AstLambdaArgRef* ধ্রুব newp = নূতন AstLambdaArgRef{
                nodep->fileline(), m_ds.m_dotp->lhsp()->name() + "__DOT__index", সত্য};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
            প্রত্যাগম;
        } অন্যথা যদি (m_ds.m_dotPos == DP_MEMBER) {
            // Found a Var, everything following is membership.  {scope}.{var}.HERE {member}
            যদি (m_ds.m_dotErr) {
                nodep->unlinkFrBack();  // Avoid circular node loop on errors
            } অন্যথা {
                Ast_গ্রন্থি_রাশিমালা* ধ্রুব varEtcp = VN_AS(m_ds.m_dotp->lhsp()->unlinkFrBack(), গ্রন্থি_রাশিমালা);
                Ast_গ্রন্থি_রাশিমালা* ধ্রুব newp = নূতন AstMemberSel{nodep->fileline(), varEtcp,  //
                                                           VFlagChildDType{}, nodep->name()};
                nodep->replaceWith(newp);
            }
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
        } অন্যথা {
            //
            string expectWhat;
            তার্কিক allowScope = অসত্য;
            তার্কিক allowVar = অসত্য;
            তার্কিক allowFTask = অসত্য;
            তার্কিক staticAccess = অসত্য;
            যদি (m_ds.m_disablep) {
                allowScope = সত্য;
                allowFTask = সত্য;
                expectWhat = "block/task";
            } অন্যথা যদি (m_ds.m_dotPos == DP_PACKAGE) {
                // {package-or-class}::{a}
                Ast_গ্রন্থি_Module* classOrPackagep = nullptr;
                expectWhat = "scope/variable/func";
                allowScope = সত্য;
                allowVar = সত্য;
                allowFTask = সত্য;
                staticAccess = সত্য;
                UINFO(9, "chk pkg " << m_ds.ascii() << " lhsp=" << m_ds.m_dotp->lhsp());
                UASSERT_OBJ(VN_IS(m_ds.m_dotp->lhsp(), ClassOrPackageRef), m_ds.m_dotp->lhsp(),
                            "Bad package link");
                AstClassOrPackageRef* ধ্রুব cpackagerefp
                    = VN_AS(m_ds.m_dotp->lhsp(), ClassOrPackageRef);
                যদি (cpackagerefp->name() == "local::") {
                    m_randSymp = nullptr;
                    first = সত্য;
                } অন্যথা যদি (!cpackagerefp->classOrPackageSkipp()) {
                    VSymEnt* ধ্রুব foundp = m_statep->resolveClassOrPackage(
                        m_ds.m_dotSymp, cpackagerefp, সত্য, অসত্য, ":: reference");
                    যদি (!foundp) প্রত্যাগম;
                    classOrPackagep = cpackagerefp->classOrPackageSkipp();
                    যদি (classOrPackagep) {
                        m_ds.m_dotSymp = m_statep->get_গ্রন্থি_Sym(classOrPackagep);
                    } অন্যথা {
                        m_ds = lastStates;
                        প্রত্যাগম;
                    }
                } অন্যথা {
                    classOrPackagep = cpackagerefp->classOrPackageSkipp();
                    UASSERT_OBJ(classOrPackagep, m_ds.m_dotp->lhsp(), "Bad package link");
                    m_ds.m_dotSymp = m_statep->get_গ্রন্থি_Sym(classOrPackagep);
                }
                m_ds.m_dotPos = DP_SCOPE;
            } অন্যথা যদি (m_ds.m_dotPos == DP_SCOPE || m_ds.m_dotPos == DP_FIRST) {
                // {a}.{b}, where {a} maybe a module name
                // or variable, where dotting into structure member
                expectWhat = "scope/variable";
                allowScope = সত্য;
                allowVar = সত্য;
            } অন্যথা যদি (m_ds.m_dotPos == DP_NONE) {
                expectWhat = "variable";
                allowVar = সত্য;
            } অন্যথা যদি (m_ds.m_dotPos == DP_FINAL) {
                expectWhat = "variable/method";
                allowFTask = সত্য;
                allowVar = সত্য;
            } অন্যথা {
                UINFO(1, "ds=" << m_ds.ascii());
                nodep->v3fatalSrc("Unhandled DotPosition");
            }
            // Lookup
            VSymEnt* foundp;
            string baddot;
            VSymEnt* okSymp = nullptr;
            // Restricted 'with (id_list)': only id_list names bind into target class.
            যদি (m_randSymp
                && (!m_currentWithp || m_currentWithp->nameResolvesToTarget(nodep->name()))) {
                foundp = m_randSymp->findIdFlat(nodep->name());
                যদি (foundp) {
                    যদি (m_currentWithp && m_currentWithp->restricted()) {
                        m_restrictedNamesUsed.ঢোকাও(nodep->name());
                    }
                    যদি (!start) m_ds.m_dotPos = DP_MEMBER;
                    যদি (!m_currentWithp) {
                        UASSERT_OBJ(m_randMethodCallp, nodep, "Expected to be under randomize()");
                        // This will start failing once complex expressions are allowed on the LHS
                        // of randomize() with args
                        UASSERT_OBJ(VN_IS(m_randMethodCallp->fromp(), VarRef), m_randMethodCallp,
                                    "Expected simple randomize target");
                        // A ParseRef is used here so that the dot RHS gets resolved
                        AstMemberSel* ধ্রুব newp = নূতন AstMemberSel{
                            nodep->fileline(), m_randMethodCallp->fromp()->clone_বৃক্ষ(অসত্য),
                            VFlagChildDType{}, nodep->name()};
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                        প্রত্যাগম;
                    }
                    UINFO(9, indent() << "randomize-with fromSym " << foundp->nodep());
                    AstLambdaArgRef* ধ্রুব lambdaRefp
                        = নূতন AstLambdaArgRef{nodep->fileline(), "__Vrandwith_obj", অসত্য};
                    AstMemberSel* newp = নূতন AstMemberSel{nodep->fileline(), lambdaRefp,
                                                          VFlagChildDType{}, nodep->name()};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                    প্রত্যাগম;
                }
            }
            যদি (allowScope) {
                foundp = m_statep->findDotted(nodep->fileline(), m_ds.m_dotSymp, nodep->name(),
                                              baddot, okSymp, first);  // Maybe nullptr
            } অন্যথা যদি (first) {
                foundp = m_ds.m_dotSymp->findIdFallback(nodep->name());
            } অন্যথা {
                foundp = m_ds.m_dotSymp->findIdFlat(nodep->name());
            }
            যদি (!foundp && m_ds.m_dotp && isDotThisReference(m_ds.m_dotp)) {
                ধ্রুব AstClass* ধ্রুব classp = VN_CAST(m_ds.m_dotSymp->nodep(), Class);
                যদি (classp && classp->isCovergroup() && classp->covergroupEnclosingClassp()) {
                    VSymEnt* ধ্রুব enclosingClassSymp
                        = m_statep->get_গ্রন্থি_Sym(classp->covergroupEnclosingClassp());
                    foundp = enclosingClassSymp->findIdFallback(nodep->name());
                    যদি (foundp) m_ds.m_dotSymp = enclosingClassSymp;
                }
            }
            // If not found in modport, check interface fallback for parameters and typedefs.
            // Parameters and typedefs are always visible through a modport (IEEE 1800-2023 25.5).
            // This mirrors the VarXRef modport parameter fallback in visit(AstVarXRef).
            যদি (!foundp && VN_IS(m_ds.m_dotSymp->nodep(), Modport)
                && m_ds.m_dotSymp->fallbackp()) {
                VSymEnt* ধ্রুব ifaceFallbackp = m_ds.m_dotSymp->fallbackp();
                VSymEnt* ধ্রুব ifaceFoundp = ifaceFallbackp->findIdFlat(nodep->name());
                যদি (ifaceFoundp) {
                    যদি (ধ্রুব AstVar* ধ্রুব varp = VN_CAST(ifaceFoundp->nodep(), Var)) {
                        যদি (varp->isParam()) foundp = ifaceFoundp;
                    } অন্যথা যদি (VN_IS(ifaceFoundp->nodep(), Typedef)
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
            যদি (!foundp && !okSymp && VN_IS(m_ds.m_dotSymp->nodep(), Modport)) {
                okSymp = m_ds.m_dotSymp;
            }
            যদি (foundp) {
                UINFO(9, indent() << "found=se" << cvtToHex(foundp) << "  exp=" << expectWhat
                                  << "  n=" << foundp->nodep());
            }
            // What fell out?
            তার্কিক ok = অসত্য;
            // Special case: waiting on clocking event
            যদি (m_inSens && foundp && m_ds.m_dotPos != DP_SCOPE && m_ds.m_dotPos != DP_FIRST) {
                যদি (AstClocking* ধ্রুব clockingp = sensClockingp(foundp->nodep())) {
                    foundp = getCreateClockingEventSymEnt(clockingp);
                }
            }
            যদি (!foundp) {
            } অন্যথা যদি (VN_IS(foundp->nodep(), Cell) || VN_IS(foundp->nodep(), গ্রন্থি_Block)
                       || VN_IS(foundp->nodep(), GenBlock)
                       || VN_IS(foundp->nodep(), Netlist)  // for $root
                       || VN_IS(foundp->nodep(), Module)) {  // if top
                যদি (allowScope) {
                    ok = সত্য;
                    m_ds.m_dotText = VString::dot(m_ds.m_dotText, ".", nodep->name());
                    m_ds.m_dotSymp = foundp;
                    m_ds.m_dotPos = DP_SCOPE;
                    যদি (m_ds.m_disablep && VN_IS(foundp->nodep(), গ্রন্থি_Block)) {
                        // Possibly it is not the final link. If we are under dot and not in its
                        // last component, `targetp()` field will be overwritten by next components
                        m_ds.m_disablep->targetp(foundp->nodep());
                    }
                    যদি (VN_IS(foundp->nodep(), GenBlock)) {
                        m_ds.m_genBlk = সত্য;
                        যদি (m_ds.m_disablep) {
                            m_ds.m_disablep->v3warn(
                                E_UNSUPPORTED,
                                "Unsupported: Generate block referenced by disable");
                        }
                    }
                    // Upper AstDot visitor will handle it from here
                } অন্যথা যদি (VN_IS(foundp->nodep(), Cell) && allowVar) {
                    AstCell* ধ্রুব cellp = VN_AS(foundp->nodep(), Cell);
                    যদি (VN_IS(cellp->modp(), Iface)) {
                        // Interfaces can be referenced like a variable for interconnect
                        VSymEnt* ধ্রুব cellEntp = m_statep->get_গ্রন্থি_Sym(cellp);
                        UASSERT_OBJ(cellEntp, nodep, "No interface sym entry");
                        VSymEnt* ধ্রুব parentEntp
                            = cellEntp->parentp();  // Container of the var; probably a module or
                                                    // generate begin
                        AstVar* ধ্রুব ifaceRefVarp
                            = find_Iface_শীর্ষ_Varp(nodep, parentEntp, nodep->name());
                        //
                        ok = সত্য;
                        m_ds.m_dotSymp = foundp;
                        m_ds.m_dotPos = DP_SCOPE;
                        UINFO(9, indent() << " cell -> iface varref " << foundp->nodep());
                        Ast_গ্রন্থি_রাশিমালা* newp;
                        যদি (m_ds.m_dotText != "") {
                            newp = নূতন AstVarXRef{nodep->fileline(), ifaceRefVarp, m_ds.m_dotText,
                                                  VAccess::READ};
                            যদি (m_ds.m_unresolvedCell && m_ds.m_unlinkedScopep) {
                                newp = নূতন AstUnlinkedRef{nodep->fileline(), newp, newp->name(),
                                                          m_ds.m_unlinkedScopep->unlinkFrBack()};
                                m_ds.m_unlinkedScopep = nullptr;
                                m_ds.m_unresolvedCell = অসত্য;
                                m_ds.m_dotText = "";
                            }
                        } অন্যথা {
                            AstVarRef* ধ্রুব refp
                                = নূতন AstVarRef{nodep->fileline(), ifaceRefVarp, VAccess::READ};
                            newp = refp;
                        }
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                    } অন্যথা যদি (VN_IS(cellp->modp(), NotFoundModule)) {
                        cellp->modNameFileline()->v3error("Cannot find file containing interface: "
                                                          << cellp->modp()->prettyNameQ());
                    }
                }
            } অন্যথা যদি (allowScope && !allowFTask && VN_IS(foundp->nodep(), গ্রন্থি_FTask)) {
                // Function/task used as scope for static variable access (IEEE std 1800-2017 6.21)
                // Don't set m_dotText so the static variable resolves as a VarRef rather than
                // VarXRef. This is so that V3Begin won't move it out of the function/task's scope.
                ok = সত্য;
                m_ds.m_dotSymp = foundp;
                m_ds.m_dotPos = DP_SCOPE;
            } অন্যথা যদি (allowFTask && VN_IS(foundp->nodep(), গ্রন্থি_FTask)) {
                Ast_গ্রন্থি_FTask_Ref* taskrefp;
                যদি (VN_IS(foundp->nodep(), Task)) {
                    taskrefp = নূতন AstTaskRef{nodep->fileline(), nodep->name()};
                } অন্যথা {
                    taskrefp = নূতন AstFuncRef{nodep->fileline(), nodep->name()};
                }
                nodep->replaceWith(taskrefp);
                VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                m_ds = lastStates;
                প্রত্যাগম;
            } অন্যথা যদি (AstVar* ধ্রুব varp = foundToVarp(foundp, nodep, VAccess::READ)) {
                // Preserve explicit 'this' references to members inherited from
                // generic classes until after specialization.
                যদি (m_statep->forPrimary() && m_ds.m_dotp && isDotThisReference(m_ds.m_dotp)
                    && foundp->classOrPackagep() != m_modp
                    && VN_IS(foundp->classOrPackagep(), Class)
                    && foundp->classOrPackagep()->hasGParam()) {
                    m_ds.m_unresolvedClass = সত্য;
                    প্রত্যাগম;
                }
                AstIfaceRefDType* ধ্রুব ifacerefp
                    = LinkDotState::ifaceRefFromArray(varp->subDTypep());
                যদি (varp->isIfaceRef() && m_genericIfaceModule
                    && VN_IS(varp->childDTypep(), IfaceGenericDType) && !start) {
                    // Defer only dotted member access ('d.PARAM'), as V3Param must specialize
                    // first; a standalone ref ('.x(d)') resolves via allowVar below now
                    ok = সত্য;
                    m_ds.m_unresolvedGenericIface = সত্য;
                } অন্যথা যদি (ifacerefp && varp->isIfaceRef()) {
                    UASSERT_OBJ(ifacerefp->ifaceViaCellp(), ifacerefp, "Unlinked interface");
                    // Really this is a scope reference into an interface
                    UINFO(9, indent() << "varref-ifaceref " << m_ds.m_dotText << "  " << nodep);
                    m_ds.m_dotText = VString::dot(m_ds.m_dotText, ".", nodep->name());
                    // If modport specified, use modport symbol table for lookup of virtual ports
                    যদি (ifacerefp->modportp() && m_statep->exists_গ্রন্থি_Sym(ifacerefp->modportp())) {
                        m_ds.m_dotSymp = m_statep->get_গ্রন্থি_Sym(ifacerefp->modportp());
                    } অন্যথা {
                        m_ds.m_dotSymp = m_statep->get_গ্রন্থি_Sym(ifacerefp->ifaceViaCellp());
                    }
                    m_ds.m_dotPos = DP_SCOPE;
                    ok = সত্য;

                    AstVarRef* ধ্রুব refp = নূতন AstVarRef{nodep->fileline(), varp, VAccess::READ};
                    nodep->replaceWith(refp);

                    VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                } অন্যথা যদি (allowVar) {
                    Ast_গ্রন্থি* newp;
                    যদি (m_ds.m_dotText != "") {
                        AstVarXRef* ধ্রুব refp
                            = নূতন AstVarXRef{nodep->fileline(), nodep->name(), m_ds.m_dotText,
                                             VAccess::READ};  // lvalue'ness computed later
                        refp->varp(varp);
                        // Not linked again after V3LinkLValue, so that reports writing it
                        যদি (ধ্রুব AstModportVarRef* ধ্রুব mvarp
                            = VN_CAST(foundp->nodep(), ModportVarRef)) {
                            যদি (m_statep->forParamed() && mvarp->direction().isReadOnly()) {
                                refp->readOnlyModport(সত্য);
                            }
                        }
                        refp->containsGenBlock(m_ds.m_genBlk);
                        m_ds.m_dotText = "";
                        যদি (m_ds.m_unresolvedCell && m_ds.m_unlinkedScopep) {
                            ধ্রুব string dotted = refp->dotted();
                            ধ্রুব size_t pos = dotted.find("__BRA__??__KET__");
                            // Arrays of interfaces all have the same parameters
                            যদি (pos != string::npos && varp->isParam()
                                && VN_IS(m_ds.m_unlinkedScopep, CellArrayRef)) {
                                refp->dotted(dotted.substr(0, pos));
                                newp = refp;
                            } অন্যথা {
                                UINFO(9, indent() << "deferring until post-V3Param: " << refp);
                                newp = নূতন AstUnlinkedRef{nodep->fileline(), refp, refp->name(),
                                                          m_ds.m_unlinkedScopep->unlinkFrBack()};
                                m_ds.m_unlinkedScopep = nullptr;
                                m_ds.m_unresolvedCell = অসত্য;
                            }
                        } অন্যথা {
                            newp = refp;
                        }
                    } অন্যথা {
                        যদি (staticAccess && !varp->lifetime().isStatic() && !varp->isParam()) {
                            // TODO bug4077
                            // nodep->v3error("Static access to non-static member variable "
                            //                << varp->prettyNameQ() << endl);
                        }
                        AstVarRef* ধ্রুব refp = নূতন AstVarRef{
                            nodep->fileline(), varp, VAccess::READ};  // lvalue'ness computed later
                        refp->classOrPackagep(foundp->classOrPackagep());
                        newp = refp;
                    }
                    UINFO(9, indent() << "new " << newp);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                    m_ds.m_dotPos = DP_MEMBER;
                    ok = সত্য;
                }
            } অন্যথা যদি (ধ্রুব AstModport* ধ্রুব modportp = VN_CAST(foundp->nodep(), Modport)) {
                // A scope reference into an interface's modport (not
                // necessarily at a pin connection)
                UINFO(9, indent() << "cell-ref-to-modport " << m_ds.m_dotText << "  " << nodep);
                UINFO(9, indent() << "unlinked " << m_ds.m_unlinkedScopep);
                UINFO(9,
                      indent() << "dotSymp " << m_ds.m_dotSymp << " " << m_ds.m_dotSymp->nodep());
                // Iface was the previously dotted component
                যদি (!m_ds.m_dotSymp || !VN_IS(m_ds.m_dotSymp->nodep(), Cell)
                    || !VN_AS(m_ds.m_dotSymp->nodep(), Cell)->modp()
                    || !VN_IS(VN_AS(m_ds.m_dotSymp->nodep(), Cell)->modp(), Iface)) {
                    nodep->v3error("Modport not referenced as <interface>."
                                   << modportp->prettyNameQ());
                } অন্যথা যদি (!VN_AS(m_ds.m_dotSymp->nodep(), Cell)->modp()
                           || !VN_IS(VN_AS(m_ds.m_dotSymp->nodep(), Cell)->modp(), Iface)) {
                    nodep->v3error("Modport not referenced from underneath an interface: "
                                   << modportp->prettyNameQ());
                } অন্যথা {
                    AstCell* ধ্রুব cellp = VN_AS(m_ds.m_dotSymp->nodep(), Cell);
                    UASSERT_OBJ(cellp, nodep, "Modport not referenced from an instance");
                    VSymEnt* ধ্রুব cellEntp = m_statep->get_গ্রন্থি_Sym(cellp);
                    UASSERT_OBJ(cellEntp, nodep, "No interface sym entry");
                    VSymEnt* ধ্রুব parentEntp
                        = cellEntp->parentp();  // Container of the var; probably a
                                                // module or generate begin
                    // We drop __BRA__??__KET__ as cells don't have that naming yet
                    AstVar* ধ্রুব ifaceRefVarp
                        = find_Iface_শীর্ষ_Varp(nodep, parentEntp, cellp->name());
                    //
                    ok = সত্য;
                    m_ds.m_dotText = VString::dot(m_ds.m_dotText, ".", nodep->name());
                    m_ds.m_dotSymp = foundp;
                    m_ds.m_dotPos = DP_SCOPE;
                    UINFO(9, indent() << "modport -> iface varref " << foundp->nodep());
                    // We lose the modport name here, so we cannot detect mismatched modports.
                    AstVarRef* ধ্রুব refp
                        = নূতন AstVarRef{nodep->fileline(), ifaceRefVarp, VAccess::READ};
                    Ast_গ্রন্থি_রাশিমালা* newp = refp;

                    auto* ধ্রুব cellarrayrefp = VN_CAST(m_ds.m_unlinkedScopep, CellArrayRef);
                    যদি (cellarrayrefp) {
                        // iface[vec].modport became CellArrayRef(iface, lsb)
                        // Convert back to SelBit(iface, lsb)
                        UINFO(9, indent() << "Array modport to SelBit " << cellarrayrefp);
                        newp = নূতন AstSelBit{cellarrayrefp->fileline(), newp,
                                             cellarrayrefp->selp()->unlinkFrBack()};
                        newp->user3(সত্য);  // Don't process again
                        VL_DO_DANGLING(ঠেলো_Deletep(cellarrayrefp->unlinkFrBack()), cellarrayrefp);
                        m_ds.m_unlinkedScopep = nullptr;
                    }
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                }
            } অন্যথা যদি (AstConstraint* ধ্রুব defp = VN_CAST(foundp->nodep(), Constraint)) {
                Ast_গ্রন্থি* ধ্রুব newp = নূতন AstConstraintRef{nodep->fileline(), nullptr, defp};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                ok = সত্য;
                m_ds.m_dotPos = DP_MEMBER;
                m_ds.m_dotText = "";
            } অন্যথা যদি (AstClass* ধ্রুব defp = VN_CAST(foundp->nodep(), Class)) {
                যদি (allowVar) {
                    যদি (m_ds.m_dotPos == DP_SCOPE && !staticAccess) {
                        // Continue hierarchical lookup in the class scope.
                        m_ds.m_dotSymp = foundp;
                    } অন্যথা {
                        AstRefDType* ধ্রুব newp
                            = নূতন AstRefDType{nodep->fileline(), nodep->name()};
                        replaceWithCheckBreak(nodep, newp);
                        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                    }
                    ok = সত্য;
                    m_ds.m_dotText = "";
                } অন্যথা {
                    (শূন্য)defp;  // Prevent unused variable warning
                }
            } অন্যথা যদি (AstMemberDType* ধ্রুব defp = VN_CAST(foundp->nodep(), MemberDType)) {
                যদি (allowVar) {
                    checkMemberDeclOrder(nodep, defp);
                    AstRefDType* ধ্রুব refp = নূতন AstRefDType{nodep->fileline(), nodep->name()};
                    refp->refDTypep(defp);
                    replaceWithCheckBreak(nodep, refp);
                    VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                    ok = সত্য;
                    m_ds.m_dotText = "";
                }
            } অন্যথা যদি (AstEnumItem* ধ্রুব valuep = VN_CAST(foundp->nodep(), EnumItem)) {
                যদি (allowVar) {
                    AstEnumItemRef* ধ্রুব refp
                        = নূতন AstEnumItemRef{nodep->fileline(), valuep, foundp->classOrPackagep()};
                    Ast_গ্রন্থি* newp = refp;
                    // Hierarchical reference, relinked after V3Param as the referenced
                    // module may be specialized, similar to AstVarXRef
                    refp->dotted(m_ds.m_dotText);
                    refp->containsGenBlock(m_ds.m_genBlk);
                    যদি (m_ds.m_unresolvedCell && m_ds.m_unlinkedScopep) {
                        UINFO(9, indent() << "deferring until post-V3Param: " << refp);
                        newp = নূতন AstUnlinkedRef{nodep->fileline(), refp, refp->name(),
                                                  m_ds.m_unlinkedScopep->unlinkFrBack()};
                        m_ds.m_unlinkedScopep = nullptr;
                        m_ds.m_unresolvedCell = অসত্য;
                    }
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                    ok = সত্য;
                    m_ds.m_dotText = "";
                }
            } অন্যথা যদি (ধ্রুব AstLambdaArgRef* ধ্রুব argrefp
                       = VN_CAST(foundp->nodep(), LambdaArgRef)) {
                যদি (allowVar) {
                    Ast_গ্রন্থি* ধ্রুব newp
                        = নূতন AstLambdaArgRef{nodep->fileline(), argrefp->name(), অসত্য};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                    ok = সত্য;
                    m_ds.m_dotPos = DP_MEMBER;
                    m_ds.m_dotText = "";
                }
            } অন্যথা যদি (VN_IS(foundp->nodep(), Clocking)) {
                m_ds.m_dotSymp = foundp;
                যদি (m_ds.m_dotText != "") m_ds.m_dotText += "." + nodep->name();
                ok = m_ds.m_dotPos == DP_SCOPE || m_ds.m_dotPos == DP_FIRST;
            } অন্যথা যদি (ধ্রুব AstModportClockingRef* ধ্রুব clockingRefp
                       = VN_CAST(foundp->nodep(), ModportClockingRef)) {
                // Clocking block accessed through a modport - redirect to actual clocking
                m_ds.m_dotSymp = m_statep->get_গ্রন্থি_Sym(clockingRefp->clockingp());
                যদি (m_ds.m_dotText != "") m_ds.m_dotText += "." + nodep->name();
                ok = m_ds.m_dotPos == DP_SCOPE || m_ds.m_dotPos == DP_FIRST;
            } অন্যথা যদি (ধ্রুব Ast_গ্রন্থি_FTask* ধ্রুব ftaskp = VN_CAST(foundp->nodep(), গ্রন্থি_FTask)) {
                যদি (!ftaskp->isFunction() || ftaskp->classMethod()) {
                    ok = m_ds.m_dotPos == DP_NONE;
                    যদি (ok) {
                        // The condition is true for tasks,
                        // properties and void functions.
                        // In these cases, the parentheses may be skipped.
                        // Also SV class methods can be called without parens
                        AstFuncRef* ধ্রুব funcRefp
                            = নূতন AstFuncRef{nodep->fileline(), nodep->name()};
                        nodep->replaceWith(funcRefp);
                        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                    }
                }
            } অন্যথা যদি (AstTypedef* ধ্রুব defp = VN_CAST(foundp->nodep(), Typedef)) {
                ধ্রুব তার্কিক ifaceFinalSegmentAllowed
                    = (m_ds.m_dotPos == DP_FINAL) && m_ds.m_dotSymp
                      && (VN_IS(m_ds.m_dotSymp->nodep(), Modport)
                          || (VN_IS(m_ds.m_dotSymp->nodep(), Cell)
                              && VN_CAST(m_ds.m_dotSymp->nodep(), Cell)->modp()
                              && VN_IS(VN_CAST(m_ds.m_dotSymp->nodep(), Cell)->modp(), Iface)));
                // Allow interface typedef references even without IfaceCapture
                // The dependency graph handles resolution when IfaceCapture is disabled
                ok = (m_ds.m_dotPos == DP_NONE || m_ds.m_dotPos == DP_SCOPE
                      || ifaceFinalSegmentAllowed);
                যদি (ifaceFinalSegmentAllowed) {
                    UINFO(9, indent() << "allow final-segment typedef name="
                                      << nodep->prettyNameQ() << " dotText='" << m_ds.m_dotText
                                      << "' dotSym=" << m_ds.m_dotSymp);
                }
                যদি (ok) {
                    AstRefDType* ধ্রুব refp = নূতন AstRefDType{nodep->fileline(), nodep->name()};
                    // Don't check if typedef is to a <type T>::<reference> as might not be
                    // resolved yet
                    যদি (m_ds.m_dotPos == DP_NONE) checkDeclOrder(nodep, defp);
                    refp->typedefp(defp);

                    V3LinkDotIfaceCapture::captureTypedefContext(
                        refp, "typedef", static_cast<পূর্ণাঙ্ক>(m_ds.m_dotPos), m_ds.m_dotText,
                        m_ds.m_dotSymp, m_modp, [এতত্]() { প্রত্যাগম indent(); });

                    যদি (VN_IS(nodep->backp(), SelExtract)) {
                        m_packedArrayDtp = refp;
                    } অন্যথা {
                        replaceWithCheckBreak(nodep, refp);
                        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                    }
                }
            } অন্যথা যদি (AstParamTypeDType* ধ্রুব defp = VN_CAST(foundp->nodep(), ParamTypeDType)) {
                ok = (m_ds.m_dotPos == DP_NONE || m_ds.m_dotPos == DP_SCOPE
                      || m_ds.m_dotPos == DP_FINAL);
                যদি (ok) {
                    AstRefDType* ধ্রুব refp = নূতন AstRefDType{nodep->fileline(), nodep->name()};
                    refp->refDTypep(defp);

                    V3LinkDotIfaceCapture::captureTypedefContext(
                        refp, "paramtype", static_cast<পূর্ণাঙ্ক>(m_ds.m_dotPos), m_ds.m_dotText,
                        m_ds.m_dotSymp, m_modp, [এতত্]() { প্রত্যাগম indent(); });

                    যদি (VN_IS(nodep->backp(), SelExtract)) {
                        m_packedArrayDtp = refp;
                    } অন্যথা {
                        replaceWithCheckBreak(nodep, refp);
                        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                    }
                }
            }
            যদি (!ok) {
                যদি (m_insideClassExtParam) {
                    // Don't throw error if the reference is inside a class that extends a param,
                    // because some members can't be linked in such a case. m_insideClassExtParam
                    // may be true only in the first stage of linking.
                    // Mark that the Dot statement can't be resolved.
                    m_ds.m_unresolvedClass = সত্য;
                    // If the symbol was a scope name, it would be resolved.
                    m_ds.m_dotPos = DP_MEMBER;
                } অন্যথা {
                    // Cells/interfaces can't be implicit
                    ধ্রুব তার্কিক checkImplicit
                        = (!m_ds.m_dotp && m_ds.m_dotText == "" && !m_ds.m_disablep && !foundp);
                    ধ্রুব তার্কিক err
                        = !(checkImplicit && m_statep->implicitOk(m_modp, nodep->name()));
                    যদি (err) {
                        যদি (foundp) {
                            nodep->v3error("Found definition of '"
                                           << m_ds.m_dotText << (m_ds.m_dotText == "" ? "" : ".")
                                           << nodep->prettyName() << "'" << " as a "
                                           << foundp->nodep()->typeName() << " but expected a "
                                           << expectWhat);
                        } অন্যথা যদি (m_ds.m_dotText == "") {
                            UINFO(1, "   ErrParseRef curSymp=se" << cvtToHex(m_curSymp)
                                                                 << " ds=" << m_ds.ascii());
                            ধ্রুব string suggest = m_statep->suggestSymFallback(
                                m_ds.m_dotSymp, nodep->name(), VNodeMatcher{});
                            nodep->v3error(
                                "Can't find definition of "
                                << expectWhat << ": " << nodep->prettyNameQ() << '\n'
                                << (suggest.empty() ? "" : nodep->warnMore() + suggest));
                        } অন্যথা {
                            nodep->v3error(
                                "Can't find definition of "
                                << (!baddot.empty() ? Ast_গ্রন্থি::prettyNameQ(baddot)
                                                    : nodep->prettyNameQ())
                                << " in dotted " << expectWhat << ": '"
                                << m_ds.m_dotText + "." + nodep->prettyName() << "'\n"
                                << nodep->warnContextPrimary()
                                << (okSymp ? okSymp->cellErrorScopes(
                                                 nodep, Ast_গ্রন্থি::prettyName(m_ds.m_dotText))
                                           : ""));
                        }
                        m_ds.m_dotErr = সত্য;
                    }
                    যদি (checkImplicit) {
                        // Create if implicit, and also if error (so only complain once)
                        // Else if a scope is allowed, making a signal won't help error cascade
                        AstVar* ধ্রুব varp
                            = createImplicitVar(m_curSymp, nodep, m_modp, m_modSymp, err);
                        AstVarRef* ধ্রুব newp
                            = নূতন AstVarRef{nodep->fileline(), varp, VAccess::READ};
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                    }
                }
            }
        }
        যদি (start) m_ds = lastStates;
    }
    শূন্য visit(AstClassOrPackageRef* nodep) override {
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

            যদি (!nodep->classOrPackageSkipp() && !nodep->classOrPackageNodep()
                && nodep->name() != "local::") {
                ধ্রুব তার্কিক deferIfUnresolved = m_statep->forPrimary() && m_insideClassExtParam;
                m_statep->resolveClassOrPackage(m_ds.m_dotSymp, nodep, m_ds.m_dotPos != DP_PACKAGE,
                                                অসত্য, ":: reference", deferIfUnresolved);
            }

            // ClassRef's have pins, so track
            যদি (nodep->classOrPackageSkipp()) {
                m_pinSymp = m_statep->get_গ্রন্থি_Sym(nodep->classOrPackageSkipp());
            } অন্যথা যদি (nodep->name() != "local::") {
                প্রত্যাগম;
            }

            AstClass* ধ্রুব refClassp = VN_CAST(nodep->classOrPackageSkipp(), Class);
            // Make sure any extends() are properly imported within referenced class
            যদি (refClassp && !m_statep->forPrimary()) classExtendImport(refClassp);

            যদি (m_statep->forPrimary() && refClassp && !nodep->paramsp()
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
                AstPin* ধ্রুব pinp = নূতন AstPin{nodep->fileline(), 1, "__paramNumber1", nullptr};
                pinp->param(সত্য);
                nodep->addParamsp(pinp);
            }

            m_ds.init(m_curSymp);
            UINFO(4, indent() << "(Backto) Link ClassOrPackageRef: " << nodep);
            iterateChildren(nodep);
        }

        যদি (nodep->name() == "local::") {
            যদি (!m_randSymp && !m_randMethodCallp) {
                nodep->v3error("Illegal 'local::' outside 'randomize() with'"
                               " (IEEE 1800-2023 18.7.1)");
                m_ds.m_dotErr = সত্য;
                প্রত্যাগম;
            }
        }
        যদি (m_ds.m_dotPos == DP_PACKAGE && nodep->classOrPackageSkipp()) {
            m_ds.m_dotSymp = m_statep->get_গ্রন্থি_Sym(nodep->classOrPackageSkipp());
            UINFO(9, indent() << "set sym " << m_ds.ascii());
        }
    }
    শূন্য visit(AstConstraint* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        যদি (nodep->isExternDef()) {
            যদি (ধ্রুব VSymEnt* ধ্রুব foundp
                = m_curSymp->findIdFallback("extern " + nodep->name())) {
                ধ্রুব AstConstraint* ধ্রুব protop = VN_AS(foundp->nodep(), Constraint);
                // Copy specifiers.
                // External definition cannot have any specifiers, so no value will be overwritten.
                nodep->isStatic(protop->isStatic());
            } অন্যথা {
                nodep->v3error("extern not found that declares " + nodep->prettyNameQ());
            }
        }
        যদি (nodep->isExternProto()) {
            যদি (!m_curSymp->findIdFallback(nodep->name()) && nodep->isExternExplicit()) {
                nodep->v3warn(PROTOTYPEMIS, "Definition not found for extern prototype "
                                                << nodep->prettyNameQ());
            }
        }
        VL_RESTORER(m_curSymp);
        VL_RESTORER_COPY(m_ds);
        m_ds.m_dotSymp = m_curSymp = m_statep->get_গ্রন্থি_Sym(nodep);
        iterateChildren(nodep);
    }
    শূন্য visit(AstConstraintRef* nodep) override {
        যদি (nodep->user3SetOnce()) প্রত্যাগম;
        LINKDOT_VISIT_START();
        UINFO(8, indent() << "visit " << nodep);
        UINFO(8, indent() << m_ds.ascii());
        // No children defined
    }
    শূন্য visit(AstVarRef* nodep) override {
        // VarRef: Resolve its reference
        // ParseRefs are used the first pass (forPrimary) so we shouldn't get can't find
        // errors here now that we have a VarRef.
        // No checkNoDot; created and iterated from a parseRef
        LINKDOT_VISIT_START();
        iterateChildren(nodep);
        যদি (!nodep->varp()) {
            UINFO(9, indent() << "linkVarRef se" << cvtToHex(m_curSymp) << "  n=" << nodep);
            UASSERT_OBJ(m_curSymp, nodep, "nullptr lookup symbol table");
            VSymEnt* ধ্রুব foundp = m_curSymp->findIdFallback(nodep->name());
            যদি (AstVar* ধ্রুব varp
                = foundp ? foundToVarp(foundp, nodep, nodep->access()) : nullptr) {
                nodep->varp(varp);
                updateVarUse(nodep->varp());
                // Generally set by parse, but might be an import
                nodep->classOrPackagep(foundp->classOrPackagep());
            }
            যদি (VL_UNCOVERABLE(!nodep->varp())) {
                nodep->v3error("Can't find definition of signal, again: "  // LCOV_EXCL_LINE
                               << nodep->prettyNameQ());
            }
        }
        AstVarScope* vscp = nodep->varScopep();
        যদি (vscp && vscp->user2p() != vscp && m_replaceWithAlias) {
            vscp = LinkDotScopeVisitor::getAlias(vscp);
            nodep->varp(vscp->varp());
            nodep->varScopep(vscp);
            updateVarUse(nodep->varp());
            UINFO(7, indent() << "Resolved " << nodep);  // Also prints taskp
        }
    }
    শূন্য visit(AstVarXRef* nodep) override {
        // VarRef: Resolve its reference
        // We always link even if varp() is set, because the module we choose may change
        // due to creating new modules, flattening, etc.
        যদি (nodep->user3SetOnce()) প্রত্যাগম;
        LINKDOT_VISIT_START();
        UINFO(8, indent() << "visit " << nodep);
        // No checkNoDot; created and iterated from a parseRef
        যদি (!m_modSymp) {
            // Module that is not in hierarchy.  We'll be dead code eliminating it later.
            UINFO(9, "Dead module for " << nodep);
            nodep->varp(nullptr);
        } অন্যথা {
            VSymEnt* lookupSymp = m_curSymp;  // Start search at current scope
            যদি (nodep->inlinedDots() != "") {  // Correct for current post-inlined scope
                lookupSymp = findInlinedDotsSym(nodep, nodep->inlinedDots());
            }
            string baddot;
            VSymEnt* okSymp;
            তার্কিক modport;
            VSymEnt* ধ্রুব dotSymp
                = findDottedRefSymp(nodep, lookupSymp, nodep->dotted(), baddot, okSymp, &modport);
            যদি (!dotSymp) প্রত্যাগম;

            যদি (!m_statep->forScopeCreation()) {
                VSymEnt* foundp = nullptr;
                যদি (modport) {
                    // This is a copy of findSymPrefixed with few modifications.
                    // This searches without a fallback like findSymPrefixed but if lookup fails it
                    // will check one fallback (fallback of modport shall be interface) and take
                    // only L and G PARAMs since they should be visible from a modport always.
                    string prefix = dotSymp->symPrefix();
                    যাবত্ (!foundp) {
                        foundp = dotSymp->findIdFlat(prefix + nodep->name());
                        যদি (foundp) ভঙ্গ;
                        UASSERT_OBJ(dotSymp->fallbackp(), nodep, "Modports shall have fallback");
                        foundp = dotSymp->fallbackp()->findIdFlat(prefix + nodep->name());
                        যদি (ধ্রুব AstVar* ধ্রুব varp
                            = foundToVarp(foundp, nodep, nodep->access())) {
                            যদি (!varp->isParam()) foundp = nullptr;
                        }
                        যদি (prefix.empty()) ভঙ্গ;
                        string nextPrefix = LinkDotState::removeLastInlineScope(prefix);
                        যদি (prefix == nextPrefix) ভঙ্গ;
                        prefix = std::move(nextPrefix);
                    }
                    baddot = nodep->name();
                } অন্যথা {
                    foundp = m_statep->findSymPrefixed(dotSymp, nodep->name(), baddot, সত্য);
                }
                যদি (m_inSens && foundp) {
                    যদি (AstClocking* ধ্রুব clockingp = sensClockingp(foundp->nodep())) {
                        foundp = getCreateClockingEventSymEnt(clockingp);
                    }
                }
                AstVar* ধ্রুব varp
                    = foundp ? foundToVarp(foundp, nodep, nodep->access()) : nullptr;
                nodep->varp(varp);
                updateVarUse(nodep->varp());
                UINFO(7, indent() << "Resolved " << nodep);  // Also prints varp
                // If found, check if it's ok to access in case it's in a hier_block
                যদি (nodep->varp() && errorHierNonPort(nodep, nodep->varp(), dotSymp)) প্রত্যাগম;
                যদি (!nodep->varp()) {
                    nodep->v3error("Can't find definition of "
                                   << Ast_গ্রন্থি::prettyNameQ(baddot) << " in dotted signal: '"
                                   << nodep->dotted() + "." + nodep->prettyName() << "'\n"
                                   << nodep->warnContextPrimary()
                                   << okSymp->cellErrorScopes(nodep));
                    প্রত্যাগম;
                }
            } অন্যথা {
                VSymEnt* ধ্রুব foundp
                    = m_statep->findSymPrefixed(dotSymp, nodep->name(), baddot, সত্য);
                AstVarScope* vscp = foundp ? VN_CAST(foundp->nodep(), VarScope) : nullptr;
                // Handle modport expression virtual ports
                যদি (!vscp) vscp = findVirtualPortVarScope(dotSymp, nodep->name());
                // If found, check if it's ok to access in case it's in a hier_block
                যদি (vscp && errorHierNonPort(nodep, vscp->varp(), dotSymp)) প্রত্যাগম;
                যদি (!vscp) {
                    nodep->v3error("Can't find varpin scope of "
                                   << Ast_গ্রন্থি::prettyNameQ(baddot) << " in dotted signal: '"
                                   << nodep->dotted() + "." + nodep->prettyName() << "'\n"
                                   << nodep->warnContextPrimary()
                                   << okSymp->cellErrorScopes(nodep));
                } অন্যথা {
                    যদি (vscp->user2p() && m_replaceWithAlias) {
                        vscp = LinkDotScopeVisitor::getAlias(vscp);
                    }
                    // Convert the VarXRef to a VarRef, so we don't need
                    // later optimizations to deal with VarXRef.
                    nodep->varp(vscp->varp());
                    nodep->varScopep(vscp);
                    updateVarUse(nodep->varp());
                    UINFO(7, indent() << "Resolved " << nodep);  // Also prints taskp
                    AstVarRef* ধ্রুব newvscp
                        = নূতন AstVarRef{nodep->fileline(), vscp, nodep->access()};
                    nodep->replaceWith(newvscp);
                    VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                    UINFO(9, indent() << "new " << newvscp);  // Also prints taskp
                }
            }
        }
    }
    শূন্য visit(AstEnumDType* nodep) override {
        LINKDOT_VISIT_START();
        iterateChildren(nodep);
        AstRefDType* ধ্রুব refdtypep = VN_CAST(nodep->subDTypep(), RefDType);
        যদি (refdtypep && (nodep == refdtypep->subDTypep())) {
            refdtypep->v3error("Self-referential enumerated type definition");
        }
    }
    শূন্য visit(Ast_গ্রন্থি_UOr_Struct_DType* nodep) override {
        LINKDOT_VISIT_START();
        symIterateChildren(nodep, m_statep->get_গ্রন্থি_Sym(nodep));
    }
    শূন্য visit(AstEnumItemRef* nodep) override {
        // Resolve its reference
        // EnumItemRefs are created by the first pass, but V3Param may regenerate due to
        // a parameterized class/module, so we shouldn't get can't find errors.
        // No checkNoDot; created and iterated from a parseRef
        যদি (nodep->user3SetOnce()) প্রত্যাগম;
        LINKDOT_VISIT_START();
        যদি (m_statep->forParamed() && !nodep->dotted().empty() && m_modSymp) {
            // Hierarchical reference, relink as V3Param may have specialized the module
            string baddot;
            VSymEnt* okSymp = nullptr;
            VSymEnt* ধ্রুব dotSymp
                = findDottedRefSymp(nodep, m_curSymp, nodep->dotted(), baddot, okSymp);
            যদি (!dotSymp) প্রত্যাগম;
            VSymEnt* ধ্রুব foundp
                = m_statep->findSymPrefixed(dotSymp, nodep->name(), baddot, সত্য);
            AstEnumItem* ধ্রুব itemp = foundp ? VN_CAST(foundp->nodep(), EnumItem) : nullptr;
            যদি (!itemp) {
                nodep->v3error("Enum item " << nodep->prettyNameQ() << " not found in "
                                            << Ast_গ্রন্থি::prettyNameQ(nodep->dotted()) << '\n'
                                            << nodep->warnContextPrimary());
                প্রত্যাগম;
            }
            nodep->itemp(itemp);
            UINFO(9, indent() << " relinked " << nodep);
        }
        যদি (!nodep->itemp()) {
            UINFO(9, indent() << "linkEnumRef se" << cvtToHex(m_curSymp) << "  n=" << nodep);
            UASSERT_OBJ(m_curSymp, nodep, "nullptr lookup symbol table");
            VSymEnt* ধ্রুব foundp = m_curSymp->findIdFallback(nodep->name());
            যদি (AstEnumItem* ধ্রুব itemp = foundp ? VN_CAST(foundp->nodep(), EnumItem) : nullptr) {
                nodep->itemp(itemp);
                nodep->classOrPackagep(foundp->classOrPackagep());
                UINFO(9, indent() << " resolved " << nodep);
            }
            যদি (VL_UNCOVERABLE(!nodep->itemp())) {
                nodep->v3error("Can't find definition of enum item, again: "  // LCOV_EXCL_LINE
                               << nodep->prettyNameQ());
            }
        }
        iterateChildren(nodep);
    }
    শূন্য visit(AstMethodCall* nodep) override {
        // Created here so should already be resolved.
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        VL_RESTORER_COPY(m_ds);
        VL_RESTORER(m_randSymp);
        VL_RESTORER(m_randMethodCallp);
        {
            m_ds.init(m_curSymp);
            যদি (nodep->name() == "randomize" && (nodep->argsp() || nodep->withp())) {
                m_randMethodCallp = nodep;
                ধ্রুব Ast_গ্রন্থি_DType* fromDtp = nodep->fromp()->dtypep();
                যদি (!fromDtp) {
                    যদি (ধ্রুব Ast_গ্রন্থি_Var_Ref* ধ্রুব varRefp = VN_CAST(nodep->fromp(), গ্রন্থি_Var_Ref)) {
                        fromDtp = varRefp->varp()->subDTypep();
                    } অন্যথা {
                        fromDtp = get_রাশিমালা_DTypep(nodep->fromp());
                    }
                    যদি (!fromDtp) {
                        যদি (nodep->withp()) {
                            nodep->v3warn(
                                E_UNSUPPORTED,
                                "Unsupported: 'randomize() with' on complex expressions");
                        } অন্যথা {
                            nodep->v3warn(E_UNSUPPORTED,
                                          "Unsupported: Inline random variable control with "
                                          "'randomize()' called on complex expressions");
                        }
                    }
                }
                যদি (m_statep->forPrimary() && isParamedClassRefDType(fromDtp)) {
                    m_ds.m_unresolvedClass = সত্য;
                } অন্যথা যদি (fromDtp) {
                    ধ্রুব AstClassRefDType* ধ্রুব classDtp
                        = VN_CAST(fromDtp->skipRefp(), ClassRefDType);
                    যদি (!classDtp)
                        nodep->v3error("'randomize() with' on a non-class-instance "
                                       << fromDtp->prettyNameQ());
                    অন্যথা যদি (m_statep->forPrimary() && m_extendsParam.গুনতি(classDtp->classp())) {
                        // Inherited members are unavailable until the base class is specialized.
                        // Defer lookup rather than incorrectly binding names in the caller.
                        m_ds.m_unresolvedClass = সত্য;
                    } অন্যথা {
                        m_randSymp = m_statep->get_গ্রন্থি_Sym(classDtp->classp());
                    }
                }
            }
            iterateChildren(nodep);
        }
    }
    শূন্য visit(AstVar* nodep) override {
        LINKDOT_VISIT_START();
        checkNoDot(nodep);
        VL_RESTORER(m_isParam);
        m_isParam = nodep->varType().isParam();
        iterateChildren(nodep);
        যদি (m_statep->forPrimary() && nodep->isIO() && !m_ftaskp && !nodep->user4()) {
            nodep->v3error(
                "Input/output/inout does not appear in port list: " << nodep->prettyNameQ());
        }
    }
    শূন্য visit(AstVarScope* nodep) override {
        LINKDOT_VISIT_START();
        checkNoDot(nodep);
        iterateChildren(nodep);
        AstVarScope* aliasp = LinkDotScopeVisitor::getAlias(nodep);
        যদি (aliasp && aliasp != nodep && !nodep->varp()->isIfaceRef()) {
            // Aliased variable might still be references from outside,
            // eg through the VPI, and is traced, so we need the value to propagate.
            // TODO: this means external writes to the LHS (e.g.: through the VPI) don't work
            AstAssignW* ধ্রুব assignp = নূতন AstAssignW{
                nodep->fileline(), নূতন AstVarRef{nodep->fileline(), nodep, VAccess::WRITE},
                নূতন AstVarRef{nodep->fileline(), aliasp, VAccess::READ}};
            assignp->user2(সত্য);
            nodep->scopep()->addBlocksp(নূতন AstAlways{assignp});
            // Propagate attributes and lint state of the replaced variable,
            // because all references to it are replaced with references to the alias variable
            aliasp->varp()->propagateAttrFrom(nodep->varp());
            aliasp->varp()->fileline()->modifyStateInherit(nodep->varp()->fileline());
        }
    }
    শূন্য visit(Ast_গ্রন্থি_FTask_Ref* nodep) override {
        যদি (nodep->user3SetOnce()) প্রত্যাগম;
        LINKDOT_VISIT_START();
        UINFO(8, indent() << "visit " << nodep);
        UINFO(8, indent() << m_ds.ascii());
        যদি (m_ds.m_dotPos != DP_MEMBER || nodep->name() != "randomize") {
            // Visit arguments at the beginning.
            // They may be visitted even if the current node can't be linked now.
            symIterateChildren(nodep, m_curSymp);
        }

        যদি (m_ds.m_super) {
            যদি (AstFuncRef* ধ্রুব funcRefp = VN_CAST(nodep, FuncRef)) {
                funcRefp->superReference(সত্য);
            } অন্যথা যদি (AstTaskRef* ধ্রুব taskRefp = VN_CAST(nodep, TaskRef)) {
                taskRefp->superReference(সত্য);
            }
        }

        VL_RESTORER(m_randSymp);

        তার্কিক first = !m_ds.m_dotp || m_ds.m_dotPos == DP_FIRST;
        তার্কিক staticAccess = অসত্য;
        যদি (m_ds.m_unresolvedClass) {
            // Unable to link before V3Param
            প্রত্যাগম;
        } অন্যথা যদি (m_ds.m_unresolvedCell && m_ds.m_dotPos == DP_FINAL && m_ds.m_unlinkedScopep) {
            Ast_গ্রন্থি_FTask_Ref* ধ্রুব newftaskp = nodep->clone_বৃক্ষ(অসত্য);
            newftaskp->dotted(m_ds.m_dotText);
            Ast_গ্রন্থি* ধ্রুব newp = নূতন AstUnlinkedRef{nodep->fileline(), newftaskp, nodep->name(),
                                                     m_ds.m_unlinkedScopep->unlinkFrBack()};
            m_ds.m_unlinkedScopep = nullptr;
            m_ds.m_unresolvedCell = অসত্য;
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
            প্রত্যাগম;
        } অন্যথা যদি (m_ds.m_dotp && m_ds.m_dotPos == DP_PACKAGE) {
            UASSERT_OBJ(VN_IS(m_ds.m_dotp->lhsp(), ClassOrPackageRef), m_ds.m_dotp->lhsp(),
                        "Bad package link");
            staticAccess = সত্য;
            AstClassOrPackageRef* ধ্রুব cpackagerefp
                = VN_AS(m_ds.m_dotp->lhsp(), ClassOrPackageRef);
            যদি (cpackagerefp->name() == "local::") {
                m_randSymp = nullptr;
                first = সত্য;
            } অন্যথা যদি (!cpackagerefp->classOrPackageSkipp()) {
                VSymEnt* ধ্রুব foundp = m_statep->resolveClassOrPackage(
                    m_ds.m_dotSymp, cpackagerefp, সত্য, অসত্য, ":: reference");
                যদি (foundp) nodep->classOrPackagep(cpackagerefp->classOrPackageSkipp());
            } অন্যথা {
                nodep->classOrPackagep(cpackagerefp->classOrPackageSkipp());
            }
            // Class/package :: HERE function() . method_called_on_function_return_value()
            m_ds.m_dotPos = DP_MEMBER;
        } অন্যথা যদি (m_ds.m_dotp && m_ds.m_dotPos == DP_FINAL) {
            nodep->dotted(m_ds.m_dotText);  // Maybe ""
            // Only flag FTaskRefs under generate-if/case blocks that may be
            // pruned.  GenFor and plain begin-blocks won't be pruned by V3Param.
            // A generate-case block's parent is the GenCaseItem, not the GenCase.
            // VarXRef uses the broader m_genBlk flag (set for all GenBlocks)
            // because genfor unrolling also removes variables.
            যদি (m_ds.m_genBlk && m_ds.m_dotSymp) {
                ধ্রুব Ast_গ্রন্থি* ধ্রুব blkp = m_ds.m_dotSymp->nodep();
                যদি (VN_IS(blkp, GenBlock)
                    && (VN_IS(blkp->backp(), GenIf) || VN_IS(blkp->backp(), GenCaseItem))) {
                    nodep->containsGenBlock(সত্য);
                }
            }
        } অন্যথা যদি (m_ds.m_dotp && m_ds.m_dotPos == DP_MEMBER) {
            // Found a Var, everything following is method call.
            // {scope}.{var}.HERE {method} ( ARGS )
            Ast_গ্রন্থি_রাশিমালা* ধ্রুব varEtcp = VN_AS(m_ds.m_dotp->lhsp()->unlinkFrBack(), গ্রন্থি_রাশিমালা);
            AstArg* ধ্রুব argsp = nodep->argsp();
            যদি (argsp) argsp->unlinkFrBackWithNext();
            AstMethodCall* ধ্রুব newp = নূতন AstMethodCall{nodep->fileline(), varEtcp,
                                                          VFlagChildDType{}, nodep->name(), argsp};
            যদি (AstWith* ধ্রুব withp = nodep->withp()) newp->withp(withp->unlinkFrBack());
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
            প্রত্যাগম;
        } অন্যথা যদি (m_ds.m_dotp && (m_ds.m_dotPos == DP_SCOPE || m_ds.m_dotPos == DP_FIRST)) {
            // HERE function() . method_called_on_function_return_value()
            m_ds.m_dotPos = DP_MEMBER;
            m_ds.m_dotText = "";
        } অন্যথা যদি (!m_ds.m_disablep) {
            // visit(AstDisable*) setup the dot handling
            checkNoDot(nodep);
        }
        যদি (nodep->classOrPackagep() && nodep->taskp()) {
            // References into packages don't care about cell hierarchy.
        } অন্যথা যদি (!m_modSymp) {
            // Module that is not in hierarchy.  We'll be dead code eliminating it later.
            UINFO(9, indent() << "Dead module for " << nodep);
            nodep->taskp(nullptr);
        } অন্যথা যদি (nodep->dotted() == "" && nodep->taskp()) {
            // Earlier should have setup the links
            // Might be under a BEGIN we're not processing, so don't relink it
        } অন্যথা {
            string baddot;
            VSymEnt* okSymp = nullptr;
            VSymEnt* dotSymp;
            যদি (nodep->dotted().empty()) {
                // Non-'super.' dotted reference
                dotSymp = m_ds.m_dotSymp;
            } অন্যথা {
                // Start search at module, as a variable of same name under a subtask isn't a
                // relevant hit however a function under a begin/end is.  So we want begins, but
                // not the function
                dotSymp = m_curSymp;
            }
            যদি (nodep->classOrPackagep()) {  // Look only in specified package
                dotSymp = m_statep->get_গ্রন্থি_Sym(nodep->classOrPackagep());
                UINFO(8, indent() << "Override classOrPackage " << dotSymp);
            } অন্যথা {
                যদি (nodep->inlinedDots() != "") {  // Correct for current post-inlined scope
                    dotSymp = findInlinedDotsSym(nodep, nodep->inlinedDots());
                }
                dotSymp = m_statep->findDotted(nodep->fileline(), dotSymp, nodep->dotted(), baddot,
                                               okSymp, সত্য);  // Maybe nullptr
            }
            // Restricted 'with (id_list)': only id_list names bind into target class.
            যদি (m_randSymp
                && (!m_currentWithp || m_currentWithp->nameResolvesToTarget(nodep->name()))) {
                VSymEnt* ধ্রুব foundp = m_randSymp->findIdFlat(nodep->name());
                যদি (foundp && m_currentWithp) {
                    যদি (m_currentWithp->restricted()) {
                        m_restrictedNamesUsed.ঢোকাও(nodep->name());
                    }
                    UINFO(9, indent() << "randomize-with fromSym " << foundp->nodep());
                    AstArg* argsp = nullptr;
                    যদি (nodep->argsp()) {
                        iterateAndNextNull(nodep->argsp());
                        argsp = nodep->argsp()->unlinkFrBackWithNext();
                    }
                    যদি (m_ds.m_dotPos != DP_NONE) m_ds.m_dotPos = DP_MEMBER;
                    Ast_গ্রন্থি* ধ্রুব newp = নূতন AstMethodCall{
                        nodep->fileline(),
                        নূতন AstLambdaArgRef{nodep->fileline(), "__Vrandwith_obj", অসত্য},
                        VFlagChildDType{}, nodep->name(), argsp};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                    প্রত্যাগম;
                }
            }
            যদি (first && nodep->name() == "randomize" && VN_IS(m_modp, Class)
                && !VN_IS(nodep->classOrPackagep(), Package)) {
                // need special handling to avoid falling back to std::randomize
                // Skip if classOrPackagep is a Package (i.e. std::randomize resolved earlier)
                VMemberMap memberMap;
                AstFunc* ধ্রুব randFuncp = V3Randomize::newRandomizeFunc(
                    memberMap, VN_AS(m_modp, Class), nodep->name(), সত্য, সত্য);
                nodep->taskp(randFuncp);
                nodep->classOrPackagep(VN_AS(m_modp, Class));
                m_curSymp = m_statep->ঢোকাও_Block(m_curSymp, nodep->name(), randFuncp, m_modp);
                // Already linked to the class randomize function; skip findSymPrefixed
                // which may find std::randomize and overwrite classOrPackagep
                প্রত্যাগম;
            }
            VSymEnt* ধ্রুব foundp
                = m_statep->findSymPrefixed(dotSymp, nodep->name(), baddot, first);
            Ast_গ্রন্থি_FTask* ধ্রুব taskp = foundp ? VN_CAST(foundp->nodep(), গ্রন্থি_FTask) : nullptr;
            Ast_গ্রন্থি_Module* ধ্রুব taskModulep = foundp ? foundp->classOrPackagep() : nullptr;
            // Ignore deferring linking of functions inside params of classes extending
            // parametric classes if referenced function can be linked in the first pass.
            যদি (!m_isParam || !taskp
                || (taskp->classMethod() && taskModulep && taskModulep->hasParameterList())) {
                যদি (m_insideClassExtParam) {
                    // The reference may point to a method declared in a super class, which is
                    // proved by a parameter. In such a case, it can't be linked at the first
                    // stage. Must not do any linking, because e.g. might find an extends of an
                    // upper class because the current class (under parent) isn't yet importing
                    // it's extended class symbols.
                    প্রত্যাগম;
                }
                যদি (AstClass* ধ্রুব targetClassp = VN_CAST(dotSymp->nodep(), Class)) {
                    যদি (m_extendsParam.গুনতি(targetClassp)) {
                        // Target class has parameterized extends not yet resolved.
                        // Its inherited symbols (e.g. static functions from the base class)
                        // aren't imported yet - defer to linkDotParamed.
                        প্রত্যাগম;
                    }
                }
            }

            যদি (taskp) {
                যদি (staticAccess && !taskp->isStatic()) {
                    // TODO bug4077
                    // nodep->v3error("Static access to non-static task/function "
                    //                << taskp->prettyNameQ() << endl);
                }
                nodep->taskp(taskp);
                nodep->classOrPackagep(foundp->classOrPackagep());
                UINFO(7, indent() << "Resolved " << nodep);  // Also prints taskp
            } অন্যথা {
                // Note ParseRef has similar error handling/message output
                UINFO(7, indent() << "   ErrFtask curSymp=se" << cvtToHex(m_curSymp)
                                  << " dotSymp=se" << cvtToHex(dotSymp));
                যদি (foundp) {
                    যদি (VN_IS(foundp->nodep(), Var) && m_ds.m_dotText == "" && m_ftaskp
                        && m_ftaskp->name() == foundp->nodep()->name()) {
                        // This is a recursive reference to the function itself, not to the var
                        nodep->taskp(m_ftaskp);
                        nodep->classOrPackagep(foundp->classOrPackagep());
                        UINFO(7, indent() << "Resolved recursive " << nodep);  // Also prints taskp
                    } অন্যথা {
                        nodep->v3error("Found definition of '"
                                       << m_ds.m_dotText << (m_ds.m_dotText == "" ? "" : ".")
                                       << nodep->prettyName() << "'" << " as a "
                                       << foundp->nodep()->typeName()
                                       << " but expected a task/function");
                    }
                } অন্যথা যদি (VN_IS(nodep, New) && !m_statep->forScopeCreation()) {
                    // Resolved in V3Width
                } অন্যথা যদি ((nodep->name() == "pre_randomize" || nodep->name() == "post_randomize")
                           && VN_IS(dotSymp->nodep(), Class)) {
                    AstClass* ধ্রুব classp = VN_AS(dotSymp->nodep(), Class);
                    // Functions that might be defined by user, but if not, they get empty
                    // definitions within referenced class.
                    // TODO perhaps cleaner to add to symbol table up-front with
                    // a lambda to construct them on a reference.
                    AstFunc* ধ্রুব funcp
                        = নূতন AstFunc{nodep->fileline(), nodep->name(), nullptr, nullptr};
                    funcp->classMethod(সত্য);
                    funcp->dtypep(funcp->findVoidDType());
                    classp->addMembersp(funcp);
                    m_statep->ঢোকাও_Block(dotSymp, nodep->name(), funcp, classp);
                    nodep->taskp(funcp);
                    nodep->classOrPackagep(classp);
                    UINFO(9, indent() << " Auto-made " << funcp);
                    প্রত্যাগম;
                } অন্যথা যদি (nodep->name() == "randomize" || nodep->name() == "srandom"
                           || nodep->name() == "get_randstate" || nodep->name() == "set_randstate"
                           || nodep->name() == "rand_mode" || nodep->name() == "constraint_mode") {
                    যদি (AstClass* ধ্রুব classp = VN_CAST(m_modp, Class)) {
                        // Don't overwrite if already resolved to a package (e.g. std::randomize)
                        যদি (!VN_IS(nodep->classOrPackagep(), Package))
                            nodep->classOrPackagep(classp);
                    } অন্যথা যদি (nodep->name() == "randomize") {
                        // A std::randomize resolved in V3Width
                        nodep->classOrPackagep(v3Global.rootp()->stdPackagep());
                    } অন্যথা {
                        nodep->v3error("Calling implicit class method "
                                       << nodep->prettyNameQ() << " without being under class");
                        nodep->replaceWith(নূতন AstConst{nodep->fileline(), 0});
                        VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                        প্রত্যাগম;
                    }
                } অন্যথা যদি (nodep->dotted() == "") {
                    যদি (nodep->pli()) {
                        যদি (v3Global.opt.bboxSys()) {
                            Ast_গ্রন্থি* newp;
                            যদি (VN_IS(nodep, FuncRef)) {
                                newp = নূতন AstConst{nodep->fileline(), AstConst::All0{}};
                            } অন্যথা {
                                Ast_গ্রন্থি_রাশিমালা* outp = nullptr;
                                যাবত্ (AstArg* ধ্রুব argp = nodep->argsp()) {
                                    outp = Ast_গ্রন্থি::addNext(outp, argp->exprp()->unlinkFrBack());
                                    VL_DO_DANGLING(ঠেলো_Deletep(argp->unlinkFrBack()), argp);
                                }
                                newp = নূতন AstSysIgnore{nodep->fileline(), outp};
                                newp->dtypep(nodep->dtypep());
                            }
                            nodep->replaceWith(newp);
                            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                            প্রত্যাগম;
                        } অন্যথা {
                            VSpellCheck speller;
                            V3Parse::candidatePli(&speller);
                            ধ্রুব string suggest = speller.best_Candidate_বার্তা(nodep->prettyName());
                            nodep->v3error(
                                "Unsupported or unknown PLI call: "
                                << nodep->prettyNameQ() << '\n'
                                << (suggest.empty() ? "" : nodep->warnMore() + suggest));
                        }
                    } অন্যথা {
                        ধ্রুব string suggest = m_statep->suggestSymFallback(
                            dotSymp, nodep->name(), Link_গ্রন্থি_Matcher_FTask{});
                        nodep->v3error("Can't find definition of task/function: "
                                       << nodep->prettyNameQ() << '\n'
                                       << (suggest.empty() ? "" : nodep->warnMore() + suggest));
                    }
                } অন্যথা {
                    ধ্রুব string suggest = m_statep->suggestSymFallback(dotSymp, nodep->name(),
                                                                        Link_গ্রন্থি_Matcher_FTask{});
                    nodep->v3error("Can't find definition of "
                                   << Ast_গ্রন্থি::prettyNameQ(baddot) << " in dotted task/function: '"
                                   << nodep->dotted() + "." + nodep->prettyName() << "'\n"
                                   << (suggest.empty() ? "" : nodep->warnMore() + suggest) << '\n'
                                   << nodep->warnContextPrimary()
                                   << okSymp->cellErrorScopes(nodep));
                }
            }
        }
    }
    শূন্য visit(AstSelBit* nodep) override {
        যদি (nodep->user3SetOnce()) প্রত্যাগম;
        LINKDOT_VISIT_START();
        iterateAndNextNull(nodep->fromp());
        যদি (m_ds.m_unresolvedClass) {
            UASSERT_OBJ(m_ds.m_dotPos != DP_SCOPE && m_ds.m_dotPos != DP_FIRST, nodep,
                        "Object of unresolved class on scope position in dotted reference");
            // The index does not depend on the class, and V3Param must see its class references
            symIterateNull(nodep->bitp(), m_curSymp);
            symIterateNull(nodep->attrp(), m_curSymp);
            প্রত্যাগম;
        }
        যদি (m_ds.m_dotPos == DP_SCOPE
            || m_ds.m_dotPos
                   == DP_FIRST) {  // Already under dot, so this is {modulepart} DOT {modulepart}
            UINFO(9, indent() << "deferring until after a V3Param pass: " << nodep);
            m_ds.m_dotText += "__BRA__??__KET__";
            m_ds.m_unresolvedCell = সত্য;
            // And pass up m_ds.m_dotText
        }
        // Pass dot state down to only fromp()
        iterateAndNextNull(nodep->fromp());
        symIterateNull(nodep->bitp(), m_curSymp);
        symIterateNull(nodep->attrp(), m_curSymp);
        যদি (m_ds.m_unresolvedCell && (m_ds.m_dotPos == DP_SCOPE || m_ds.m_dotPos == DP_FIRST)) {
            Ast_গ্রন্থি_রাশিমালা* ধ্রুব exprp = nodep->bitp()->unlinkFrBack();
            যদি (AstCellArrayRef* ধ্রুব fromArrRefp = VN_CAST(nodep->fromp(), CellArrayRef)) {
                // Multi-dim iface array access: append this select to the existing chain
                fromArrRefp->addSelp(exprp);
                AstCellArrayRef* ধ্রুব movedp = fromArrRefp->unlinkFrBack();
                nodep->replaceWith(movedp);
                VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
            } অন্যথা {
                AstCellArrayRef* ধ্রুব newp
                    = নূতন AstCellArrayRef{nodep->fileline(), nodep->fromp()->name(), exprp};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
            }
        }
    }
    শূন্য visit(Ast_গ্রন্থি_Pre_Sel* nodep) override {
        // Excludes simple AstSelBit, see above
        যদি (nodep->user3SetOnce()) প্রত্যাগম;
        LINKDOT_VISIT_START();
        যদি (m_ds.m_dotPos == DP_SCOPE
            || m_ds.m_dotPos
                   == DP_FIRST) {  // Already under dot, so this is {modulepart} DOT {modulepart}
            nodep->v3error("Syntax error: Range ':', '+:' etc are not allowed in the instance "
                           "part of a dotted reference");
            m_ds.m_dotErr = সত্য;
            প্রত্যাগম;
        }
        Ast_গ্রন্থি_DType* packedArrayDtp = nullptr;  // Datatype reference for packed array
        {
            VL_RESTORER(m_packedArrayDtp);
            iterateAndNextNull(nodep->fromp());
            symIterateNull(nodep->rhsp(), m_curSymp);
            symIterateNull(nodep->thsp(), m_curSymp);

            যদি (m_packedArrayDtp) {
                AstRange* ধ্রুব newRangep
                    = নূতন AstRange(nodep->fileline(), nodep->rhsp()->unlinkFrBack(),
                                   nodep->thsp()->unlinkFrBack());
                AstPackArrayDType* newArrayTypep
                    = নূতন AstPackArrayDType(nodep->fileline(), m_packedArrayDtp, newRangep);
                newArrayTypep->childDTypep(m_packedArrayDtp);
                newArrayTypep->refDTypep(nullptr);
                যদি (VN_IS(nodep->backp(), SelExtract)) {
                    packedArrayDtp = newArrayTypep;
                } অন্যথা {
                    replaceWithCheckBreak(nodep, newArrayTypep);
                    VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                }
            } অন্যথা {
                যদি (nodep->attrp()) {
                    Ast_গ্রন্থি* ধ্রুব attrp = nodep->attrp()->unlinkFrBack();
                    VL_DO_DANGLING(attrp->delete_বৃক্ষ(), attrp);
                }
                Ast_গ্রন্থি* ধ্রুব basefromp = nodep->baseFromp(অসত্য);
                যদি (VN_IS(basefromp, Replicate)) {
                    // From {...}[...] syntax in IEEE 2017
                    যদি (basefromp) UINFO(9, indent() << " Related node: " << basefromp);
                } অন্যথা {
                    nodep->attrp(নূতন AstAttrOf{nodep->fileline(), VAttrType::VAR_BASE,
                                               basefromp->clone_বৃক্ষ(অসত্য)});
                }
            }
        }

        যদি (packedArrayDtp) m_packedArrayDtp = packedArrayDtp;
    }
    শূন্য visit(AstMemberSel* nodep) override {
        // checkNoDot not appropriate, can be under a dot
        LINKDOT_VISIT_START();
        iterateChildren(nodep);
    }
    শূন্য visit(AstGenBlock* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        {
            VL_RESTORER(m_curSymp);
            VL_RESTORER_COPY(m_ds);
            যদি (nodep->name() != "") {
                m_ds.m_dotSymp = m_curSymp = m_statep->get_গ্রন্থি_Sym(nodep);
                UINFO(5, indent() << "cur=se" << cvtToHex(m_curSymp));
            }
            iterateChildren(nodep);
        }
        UINFO(5, indent() << "cur=se" << cvtToHex(m_curSymp));
    }
    শূন্য visit(Ast_গ্রন্থি_Block* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        {
            VL_RESTORER(m_curSymp);
            VL_RESTORER_COPY(m_ds);
            যদি (nodep->name() != "") {
                m_ds.m_dotSymp = m_curSymp = m_statep->get_গ্রন্থি_Sym(nodep);
                UINFO(5, indent() << "cur=se" << cvtToHex(m_curSymp));
            }
            iterateChildren(nodep);
        }
        UINFO(5, indent() << "cur=se" << cvtToHex(m_curSymp));
    }
    শূন্য visit(Ast_গ্রন্থি_FTask* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        যদি (nodep->isExternDef() && !nodep->didProto()) {
            nodep->didProto(সত্য);
            যদি (ধ্রুব VSymEnt* ধ্রুব foundp
                = m_curSymp->findIdFallback("extern " + nodep->name())) {
                ধ্রুব Ast_গ্রন্থি_FTask* ধ্রুব protop = VN_AS(foundp->nodep(), গ্রন্থি_FTask);
                // Copy specifiers.
                // External definition cannot have any specifiers, so no value will be overwritten.
                nodep->isHideLocal(protop->isHideLocal());
                nodep->isHideProtected(protop->isHideProtected());
                nodep->isStatic(protop->isStatic());
                nodep->isVirtual(protop->isVirtual());
                nodep->lifetime(protop->lifetime());
                // Always add an FUNC_RETURN_PROTO even if task, so we can locate the prototype
                nodep->addStmtsp(নূতন AstAttrOf{protop->fileline(), VAttrType::FUNC_RETURN_PROTO,
                                               protop->fvarp()
                                                   ? protop->fvarp()->clone_বৃক্ষ(অসত্য)
                                                   : নূতন AstVoidDType{protop->fileline()}});
                প্রতি (Ast_গ্রন্থি* stmtp = protop->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    যদি (AstVar* ধ্রুব portp = VN_CAST(stmtp, Var)) {
                        AstAttrOf* ধ্রুব attrp
                            = নূতন AstAttrOf{protop->fileline(), VAttrType::FUNC_ARG_PROTO,
                                            portp->subDTypep()->clone_বৃক্ষ(অসত্য)};
                        attrp->name(portp->name());
                        nodep->addStmtsp(attrp);
                    }
                }
            } অন্যথা {
                nodep->v3error("extern not found that declares " + nodep->prettyNameQ());
            }
        }
        যদি (nodep->isExternProto()) {
            যদি (!m_curSymp->findIdFallback(nodep->name())) {
                nodep->v3warn(PROTOTYPEMIS,
                              "Definition not found for extern prototype " + nodep->prettyNameQ());
            }
        }
        VL_RESTORER(m_curSymp);
        VL_RESTORER(m_ftaskp);
        VL_RESTORER(m_explicitSuperNewp);
        {
            m_ftaskp = nodep;
            m_ds.m_dotSymp = m_curSymp = m_statep->get_গ্রন্থি_Sym(nodep);
            ধ্রুব তার্কিক isNew = nodep->name() == "new";
            যদি (isNew) m_explicitSuperNewp = nullptr;
            iterateChildren(nodep);
            যদি (isNew) {
                ধ্রুব AstClassExtends* ধ্রুব classExtendsp = VN_AS(m_modp, Class)->extendsp();
                যদি (m_explicitSuperNewp && !m_explicitSuperNewp->isImplicit() && classExtendsp
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
                যদি (classExtendsp && classExtendsp->classOrNullp()) {
                    যদি (!m_explicitSuperNewp && m_statep->forParamed()
                        && !VN_AS(classExtendsp->childDTypep()->skipRefp(), ClassRefDType)
                                ->classp()
                                ->isInterfaceClass()) {
                        Ast_গ্রন্থি_Stmt* ধ্রুব superNewp
                            = addImplicitSuperNewCall(VN_AS(nodep, Func), classExtendsp);
                        UINFO(9, "created super new " << superNewp);
                        iterate(superNewp);
                    }
                }
            }
        }
        m_ds.m_dotSymp = VL_RESTORER_PREV(m_curSymp);
    }
    শূন্য visit(Ast_গ্রন্থি_Foreach* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        symIterateChildren(nodep, m_statep->get_গ্রন্থি_Sym(nodep));
    }
    শূন্য visit(AstRandSequence* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        যদি (!nodep->start().empty())
            nodep->prodp(findProd(nodep, m_statep->get_গ্রন্থি_Sym(nodep), nodep->start()));
        symIterateChildren(nodep, m_statep->get_গ্রন্থি_Sym(nodep));
    }
    শূন্য visit(AstRSProd* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        symIterateChildren(nodep, m_statep->get_গ্রন্থি_Sym(nodep));
    }
    শূন্য visit(AstRSProdItem* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        nodep->prodp(findProd(nodep, m_curSymp, nodep->name()));
        iterateChildren(nodep);
    }
    শূন্য visit(AstCoverWith* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        iterateAndNextNull(nodep->subp());
        // The type of 'item' is that of the coverpoint expression, which may name 'item' too
        iterateAndNextNull(nodep->itemp()->childDTypep());
        VL_RESTORER(m_curSymp);
        m_ds.m_dotSymp = m_curSymp = m_statep->get_গ্রন্থি_Sym(nodep);
        iterateAndNextNull(nodep->itemp());
        iterateAndNextNull(nodep->filterp());
        m_ds.m_dotSymp = VL_RESTORER_PREV(m_curSymp);
    }
    শূন্য visit(AstCoverCrossWith* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        iterateAndNextNull(nodep->selectp());
        iterateAndNextNull(nodep->countp());
        // The type of a cross item is that of its coverpoint's expression, which may name items
        iterateAndNextNull(nodep->varsp());
        VL_RESTORER(m_curSymp);
        m_ds.m_dotSymp = m_curSymp = m_statep->get_গ্রন্থি_Sym(nodep);
        iterateAndNextNull(nodep->filterp());
        m_ds.m_dotSymp = VL_RESTORER_PREV(m_curSymp);
    }
    শূন্য visit(AstWith* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        যদি (m_ds.m_unresolvedClass) প্রত্যাগম;
        VL_RESTORER(m_curSymp);
        VL_RESTORER(m_currentWithp);
        VL_RESTORER_COPY(m_restrictedNamesUsed);
        {
            m_ds.m_dotSymp = m_curSymp = m_statep->get_গ্রন্থি_Sym(nodep);
            m_currentWithp = nodep;
            // Validate the identifier_list once per AstWith (m_validated travels
            // with cloneTree, so cloned witnesses skip the redundant check).
            ধ্রুব তার্কিক firstVisit = !nodep->validated();
            যদি (firstVisit) {
                nodep->validated(সত্য);
                m_restrictedNamesUsed.clear();
                // IEEE 1800-2023 18.7: each name in the identifier_list must designate
                // a member of the randomize() target class.
                যদি (nodep->restricted() && m_randSymp) {
                    প্রতি (ধ্রুব std::string& n : nodep->restrictedNames()) {
                        যদি (!m_randSymp->findIdFlat(n)) {
                            nodep->v3error("Identifier '"
                                           << n
                                           << "' in 'with (...)' identifier list is not a"
                                              " member of the randomize() target class"
                                              " (IEEE 1800-2023 18.7)");
                            m_restrictedNamesUsed.ঢোকাও(n);
                        }
                    }
                }
            }
            iterateChildren(nodep);
            // Flag names that the user listed but never referenced in the constraint.
            যদি (firstVisit && nodep->restricted()) {
                প্রতি (ধ্রুব std::string& n : nodep->restrictedNames()) {
                    যদি (!m_restrictedNamesUsed.গুনতি(n)) {
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
    শূন্য visit(AstLambdaArgRef* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        // No checknodot(nodep), visit(AstScope) will check for LambdaArgRef
        iterateChildren(nodep);
    }
    শূন্য visit(AstClassExtends* nodep) override {
        // Resolve the symbol and get the class.
        // If it is a parameterized case, the class will be resolved after V3Param.cpp
        যদি (nodep->user3SetOnce()) প্রত্যাগম;
        // If the class is resolved, there is nothing more to do
        যদি (nodep->classOrNullp()) প্রত্যাগম;
        LINKDOT_VISIT_START();
        যদি (m_statep->forPrimary()) {
            যদি (nodep->childDTypep()) প্রত্যাগম;
            Ast_গ্রন্থি* cprp = nodep->classOrPkgsp();
            VSymEnt* lookSymp = m_curSymp;
            যদি (AstDot* ধ্রুব dotp = VN_CAST(cprp, Dot)) {
                dotp->user3(সত্য);
                যদি (AstClassOrPackageRef* lookNodep = VN_CAST(dotp->lhsp(), ClassOrPackageRef)) {
                    iterate(lookNodep);
                    cprp = dotp->rhsp();
                    VSymEnt* ধ্রুব foundp = m_statep->resolveClassOrPackage(
                        lookSymp, lookNodep, সত্য, অসত্য, nodep->verilogKwd());
                    যদি (!foundp) {
                        VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);
                        প্রত্যাগম;
                    }
                    UASSERT_OBJ(lookNodep->classOrPackageSkipp(), nodep, "Bad package link");
                    lookSymp = m_statep->get_গ্রন্থি_Sym(lookNodep->classOrPackageSkipp());
                } অন্যথা {
                    dotp->lhsp()->v3error("Attempting to extend"  // LCOV_EXCL_LINE
                                          " using non-class under dot");
                }
            }
            AstClassOrPackageRef* ধ্রুব cpackagerefp = VN_CAST(cprp, ClassOrPackageRef);
            যদি (VL_UNCOVERABLE(!cpackagerefp)) {
                // Linking the extend gives an error before this is hit
                nodep->v3error("Attempting to extend using non-class");  // LCOV_EXCL_LINE
                VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);
                প্রত্যাগম;
            }
            VSymEnt* ধ্রুব foundp = m_statep->resolveClassOrPackage(lookSymp, cpackagerefp, সত্য,
                                                                    সত্য, nodep->verilogKwd());
            যদি (!foundp) {
                VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);
                প্রত্যাগম;
            }
            যদি (AstClass* ধ্রুব classp = VN_CAST(foundp->nodep(), Class)) {
                AstPin* paramsp = cpackagerefp->paramsp();
                যদি (paramsp) {
                    paramsp = paramsp->clone_বৃক্ষ(সত্য);
                    nodep->parameterized(সত্য);
                }
                nodep->childDTypep(নূতন AstClassRefDType{nodep->fileline(), classp, paramsp});
                // Link pins
                iterate(nodep->childDTypep());
            } অন্যথা যদি (AstParamTypeDType* ধ্রুব paramp
                       = VN_CAST(foundp->nodep(), ParamTypeDType)) {
                AstRefDType* ধ্রুব refParamp = নূতন AstRefDType{nodep->fileline(), paramp->name()};
                refParamp->refDTypep(paramp);
                nodep->childDTypep(refParamp);
                nodep->parameterized(সত্য);
            } অন্যথা যদি (AstTypedef* ধ্রুব typedefp = VN_CAST(foundp->nodep(), Typedef)) {
                Ast_গ্রন্থি_DType* ধ্রুব unwrappedp = typedefp->subDTypep()->skipRefp();
                যদি (AstClassRefDType* ধ্রুব classRefp = VN_CAST(unwrappedp, ClassRefDType)) {
                    AstPin* paramsp = cpackagerefp->paramsp();
                    যদি (!paramsp && classRefp->paramsp()) {
                        // No explicit #(...) on extends clause; carry over the
                        // typedef's type-parameter pins so V3Param sees them.
                        paramsp = classRefp->paramsp()->clone_বৃক্ষ(সত্য);
                        nodep->parameterized(সত্য);
                    } অন্যথা যদি (paramsp) {
                        paramsp = paramsp->clone_বৃক্ষ(সত্য);
                        nodep->parameterized(সত্য);
                    }
                    nodep->childDTypep(
                        নূতন AstClassRefDType{nodep->fileline(), classRefp->classp(), paramsp});
                    iterate(nodep->childDTypep());
                } অন্যথা {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: " << foundp->nodep()->prettyTypeName()
                                                  << " in 'class extends'");
                    প্রত্যাগম;
                }
            } অন্যথা {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: " << foundp->nodep()->prettyTypeName()
                                                             << " in 'class extends'");
                প্রত্যাগম;
            }
            যদি (!nodep->childDTypep()) nodep->v3error("Attempting to extend using non-class");
            nodep->classOrPkgsp()->unlinkFrBack()->delete_বৃক্ষ();
        } অন্যথা {
            // Probably a parameter
            যদি (AstRefDType* ধ্রুব refp = VN_CAST(nodep->childDTypep(), RefDType)) {
                যদি (AstClassRefDType* classRefp = VN_CAST(refp->skipRefp(), ClassRefDType)) {
                    // Resolved to a class reference.
                    refp->replaceWith(classRefp->clone_বৃক্ষ(অসত্য));
                    VL_DO_DANGLING(ঠেলো_Deletep(refp), refp);
                } অন্যথা {
                    // Unable to resolve the ref type to a class reference.
                    // Get the value of type parameter passed to the class instance,
                    // to print the helpful error message.
                    ধ্রুব Ast_গ্রন্থি_DType* typep = refp->refDTypep();
                    যাবত্ (সত্য) {
                        যদি (ধ্রুব AstParamTypeDType* ধ্রুব atypep
                            = VN_CAST(typep, ParamTypeDType)) {
                            typep = atypep->subDTypep();
                            অনুবর্ত;
                        }
                        যদি (ধ্রুব AstRequireDType* ধ্রুব atypep = VN_CAST(typep, RequireDType)) {
                            typep = atypep->subDTypep();
                            অনুবর্ত;
                        }
                        ভঙ্গ;
                    }
                    typep->v3error("Attempting to extend using non-class");
                }
            }
        }
    }
    শূন্য visit(AstClass* nodep) override {
        যদি (nodep->user3SetOnce()) প্রত্যাগম;
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
            m_ds.m_dotSymp = m_curSymp = m_modSymp = m_statep->get_গ্রন্থি_Sym(nodep);
            m_modp = nodep;
            পূর্ণাঙ্ক next = 0;
            প্রতি (AstClassExtends* cextp = nodep->extendsp(); cextp;
                 cextp = VN_AS(cextp->nextp(), ClassExtends)) {
                // Replace abstract reference with hard pointer
                // Will need later resolution when deal with parameters
                যদি (++next == 2 && !nodep->isInterfaceClass() && !cextp->isImplements()) {
                    cextp->v3error("Multiple inheritance illegal on non-interface classes"
                                   " (IEEE 1800-2023 8.13)");
                }
                iterate(cextp);
                যদি (m_statep->forPrimary()) {
                    যদি (cextp->parameterized()) {
                        // Parameters in extends statement.
                        // The class can't be resolved in the current pass.
                        m_extendsParam.ঢোকাও(nodep);
                        m_insideClassExtParam = সত্য;
                    }
                    যদি (AstClassRefDType* ধ্রুব classRefp
                        = VN_CAST(cextp->childDTypep(), ClassRefDType)) {
                        AstClass* ধ্রুব classp = classRefp->classp();
                        যদি (classp != nodep) iterate(classp);
                        যদি (m_extendsParam.find(classp) != m_extendsParam.end()) {
                            // One of its super classes has parameters in extends statement.
                            // Some links may not be resolved in the first pass.
                            m_extendsParam.ঢোকাও(nodep);
                            m_insideClassExtParam = সত্য;
                        }
                    }
                }

                যদি (AstClass* ধ্রুব baseClassp = cextp->classOrNullp()) {
                    // Already converted. Update symbol table to link unlinked members.
                    // Base class has to be visited in a case if its extends statement
                    // needs to be handled. Recursive inheritance was already checked.
                    // Must be here instead of in LinkDotParam to handle
                    // "class (type T) extends T".
                    যদি (baseClassp == nodep) {
                        cextp->v3error("Attempting to extend class " << nodep->prettyNameQ()
                                                                     << " from itself");
                    } অন্যথা যদি (extendsClass(baseClassp, nodep)) {
                        cextp->v3error("Attempting to extend class "
                                       << nodep->prettyNameQ() << " from "
                                       << baseClassp->prettyNameQ()
                                       << ", which circularly inherits from it");
                    } অন্যথা যদি (cextp->isImplements() && !baseClassp->isInterfaceClass()) {
                        cextp->v3error("Attempting to implement from non-interface class "
                                       << baseClassp->prettyNameQ() << '\n'
                                       << "... Suggest use 'extends'");
                    } অন্যথা যদি (!cextp->isImplements() && !nodep->isInterfaceClass()
                               && baseClassp->isInterfaceClass()) {
                        cextp->v3error("Attempting to extend from interface class "
                                       << baseClassp->prettyNameQ() << '\n'
                                       << "... Suggest use 'implements'");
                    }
                    baseClassp->isExtended(সত্য);
                    nodep->isExtended(সত্য);
                    iterate(baseClassp);
                    importSymbolsFromExtended(nodep, cextp);
                    অনুবর্ত;
                }
            }
            m_ds.m_dotSymp = m_curSymp;

            iterateChildren(nodep);
            প্রতি (ধ্রুব auto& itr : m_ifForwards) {
                ধ্রুব VSymEnt* ধ্রুব implSymp = itr.second;
                addInterfaceForward(nodep, VN_AS(implSymp->nodep(), গ্রন্থি_FTask),
                                    VN_AS(implSymp->parentp()->nodep(), Class));
            }
        }
        // V3Width when determines types needs to find enum values and such
        // so add members pointing to appropriate enum values
        {
            VMemberMap memberMap;
            প্রতি (VSymEnt::const_iterator it = m_curSymp->begin(); it != m_curSymp->end(); ++it) {
                Ast_গ্রন্থি* ধ্রুব itemp = it->second->nodep();
                যদি (!memberMap.findMember(nodep, it->first)) {
                    যদি (AstEnumItem* ধ্রুব aitemp = VN_CAST(itemp, EnumItem)) {
                        AstEnumItemRef* ধ্রুব newp = নূতন AstEnumItemRef{
                            aitemp->fileline(), aitemp, it->second->classOrPackagep()};
                        UINFO(8, indent()
                                     << "Class import noderef '" << it->first << "' " << newp);
                        nodep->addMembersp(newp);
                        memberMap.ঢোকাও(nodep, newp);
                    }
                }
            }
        }
        m_ds.m_dotSymp = VL_RESTORER_PREV(m_curSymp);
    }

    // Collect the ClassOrPackageRefs in a chained scope operand in source order.
    // The grammar builds a scope operand only from packageClassScopeItem, which
    // always yields a ClassOrPackageRef, nested in Dots, so nothing else appears.
    স্থির শূন্য collectScopeRefs(Ast_গ্রন্থি* nodep, std::vector<AstClassOrPackageRef*>& refps) {
        যদি (AstClassOrPackageRef* ধ্রুব refp = VN_CAST(nodep, ClassOrPackageRef)) {
            refps.ঠেলো_back(refp);
            প্রত্যাগম;
        }
        AstDot* ধ্রুব dotp = VN_CAST(nodep, Dot);
        UASSERT_OBJ(dotp, nodep, "Non-scope node in package/class scope operand");
        collectScopeRefs(dotp->lhsp(), refps);
        collectScopeRefs(dotp->rhsp(), refps);
    }

    // Resolve a chained class-scope operand (`pkg::outer::inner` in
    // `pkg::outer::inner::t`) and reduce it to its innermost ClassOrPackageRef.
    // Returns false, having reported, if a segment could not be resolved.
    তার্কিক reduceScopeDot(AstRefDType* nodep, AstDot* scopeDotp) {
        std::vector<AstClassOrPackageRef*> refps;
        collectScopeRefs(scopeDotp, refps);

        VSymEnt* scopeSymp = m_ds.m_dotSymp;
        প্রতি (size_t i = 0; i < refps.size(); ++i) {
            AstClassOrPackageRef* ধ্রুব refp = refps[i];
            যদি (!refp->classOrPackageSkipp() && !refp->classOrPackageNodep()
                && !m_statep->resolveClassOrPackage(scopeSymp, refp, i == 0, অসত্য,
                                                    "class/package reference")) {
                প্রত্যাগম অসত্য;  // Error already reported
            }
            যদি (i + 1 == refps.size()) ভঙ্গ;
            // A middle segment that resolved to a node with no reachable module
            // (e.g. a typedef alias of a not-yet-specialized parameterized class)
            // gives nothing to look the next segment up in.
            Ast_গ্রন্থি_Module* ধ্রুব modp = refp->classOrPackageSkipp();
            যদি (!modp) {
                refp->v3warn(E_UNSUPPORTED, "Unsupported: Multiple '::' package/class reference");
                প্রত্যাগম অসত্য;
            }
            scopeSymp = m_statep->get_গ্রন্থি_Sym(modp);
        }
        AstClassOrPackageRef* ধ্রুব innerp = refps.back();
        innerp->unlinkFrBack();
        VL_DO_DANGLING(ঠেলো_Deletep(scopeDotp->unlinkFrBack()), scopeDotp);
        nodep->classOrPackageOpp(innerp);
        প্রত্যাগম সত্য;
    }

    শূন্য visit(AstRefDType* nodep) override {

        যদি (auto* ধ্রুব typeOfp = nodep->typeofp()) {
            iterate(typeOfp);
            // After iteration, typeofp() may have been replaced with a resolved type
            // (e.g., DOT expression like if0.rq_t resolved to REFDTYPE)
            // If it's now a NodeDType, replace this RefDType with it
            যদি (Ast_গ্রন্থি_DType* ধ্রুব resolvedDTypep = VN_CAST(nodep->typeofp(), গ্রন্থি_DType)) {
                UINFO(9,
                      indent() << "iface capture typeofp resolved to dtype, replacing RefDType\n");
                resolvedDTypep->unlinkFrBack();
                nodep->replaceWith(resolvedDTypep);
                VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                // Originally (PR #6637) this block re-captured a resolved
                // RefDType into the iface-capture ledger when user2p held a
                // Cell, to ensure re-resolution during the paramed pass.
                // In practice, the typeofp-resolved RefDType never carries a
                // Cell in user2p: the capture happens earlier when the
                // typedef is first linked.
                যদি (V3LinkDotIfaceCapture::enabled()) {
                    যদি (AstRefDType* ধ্রুব resolvedRefp = VN_CAST(resolvedDTypep, RefDType)) {
                        যদি (VL_UNCOVERABLE(
                                VN_IS(resolvedRefp->user2p(), Cell))) {  // LCOV_EXCL_LINE
                            resolvedRefp->v3fatalSrc(  // LCOV_EXCL_LINE
                                "typeofp resolved RefDType has Cell in user2p;"
                                " expected to be captured already");
                        }
                    }
                }
            }
            প্রত্যাগম;
        }

        // Resolve its reference
        যদি (m_statep->forParamed() && nodep->user3()) {
            যদি (V3LinkDotIfaceCapture::enabled() && nodep->user2p()) {
                UINFO(9, indent() << "iface capture clear user3 for captured typedef name="
                                  << nodep->name() << " cell=" << nodep->user2p());
            }
            nodep->user3(অসত্য);
        }
        যদি (nodep->user3SetOnce()) {
            যদি (V3LinkDotIfaceCapture::enabled() && nodep->user2p()) {
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
            যদি (VL_UNCOVERABLE(m_statep->forParamed() && nodep->classOrPackageOpp())) {
                nodep->v3fatalSrc(  // LCOV_EXCL_LINE
                    "RefDType user3 revisit with classOrPackageOpp in paramed pass");
            }
            প্রত্যাগম;
        }
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        যদি (nodep->classOrPackageOpp()) {
            // Resolve a chained scope (`pkg::cls::t`, `pkg::cls#(P)::t`) from outside in.
            // On failure reduceScopeDot has reported, so stop before using the operand.
            যদি (AstDot* ধ্রুব scopeDotp = VN_CAST(nodep->classOrPackageOpp(), Dot)) {
                যদি (!reduceScopeDot(nodep, scopeDotp)) প্রত্যাগম;
            }
            // Re-read: reduceScopeDot may have replaced the operand
            Ast_গ্রন্থি* ধ্রুব cpackagep = nodep->classOrPackageOpp();
            UASSERT_OBJ(!VN_IS(cpackagep, Dot), cpackagep,
                        "Package/class scope operand should be reduced by now");
            যদি (AstClassOrPackageRef* ধ্রুব cpackagerefp = VN_CAST(cpackagep, ClassOrPackageRef)) {
                iterate(cpackagerefp);
                ধ্রুব AstClass* ধ্রুব clsp = VN_CAST(cpackagerefp->classOrPackageNodep(), Class);
                যদি (clsp && clsp->hasGParam()) {
                    // Unable to link before the instantiation of parameter classes.
                    // The class reference node still has to be visited now to later link
                    // parameters.
                    iterate(cpackagep);
                    প্রত্যাগম;
                }
                // Defer non-typedef references through typedef aliases of parameterized classes.
                যদি (m_statep->forPrimary() && !VN_IS(nodep->backp(), Typedef)
                    && isParamedClassRef(cpackagerefp)) {
                    iterate(cpackagep);
                    প্রত্যাগম;
                }

                ধ্রুব তার্কিক doDefaultTypedef = !(m_resolvingTypedef && m_statep->forPrimary());
                যদি (!cpackagerefp->classOrPackageSkipp(doDefaultTypedef)
                    && !cpackagerefp->classOrPackageNodep()) {
                    VSymEnt* ধ্রুব foundp = m_statep->resolveClassOrPackage(
                        m_ds.m_dotSymp, cpackagerefp, সত্য, অসত্য, "class/package reference");
                    যদি (!foundp) প্রত্যাগম;
                }
                nodep->classOrPackagep(cpackagerefp->classOrPackageSkipp(doDefaultTypedef));
                যদি (!VN_IS(nodep->classOrPackagep(), Class)
                    && !VN_IS(nodep->classOrPackagep(), Package)) {
                    যদি (m_statep->forPrimary()) {
                        // It may be a type that comes from parameter class that is not
                        // instantioned yet
                        iterate(cpackagep);
                        প্রত্যাগম;
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
            VL_DO_DANGLING(ঠেলো_Deletep(cpackagep->unlinkFrBack()), cpackagep);
        }

        যদি (m_ds.m_dotp && (m_ds.m_dotPos == DP_PACKAGE || m_ds.m_dotPos == DP_SCOPE)) {
            UASSERT_OBJ(VN_IS(m_ds.m_dotp->lhsp(), ClassOrPackageRef), m_ds.m_dotp->lhsp(),
                        "Bad package link");
            auto* ধ্রুব cpackagerefp = VN_CAST(m_ds.m_dotp->lhsp(), ClassOrPackageRef);
            UASSERT_OBJ(cpackagerefp->classOrPackageSkipp(), m_ds.m_dotp->lhsp(),
                        "Bad package link");
            nodep->classOrPackagep(cpackagerefp->classOrPackageSkipp());
            m_ds.m_dotPos = DP_SCOPE;
        } অন্যথা {
            // Allow REFDTYPE under DOT when referencing interface typedefs
            // This is needed for patterns like: typedef iface.a_t a_t;
            // The dependency graph handles resolution when IfaceCapture is disabled
            ধ্রুব তার্কিক ifaceFinalSegmentAllowed
                = (m_ds.m_dotPos == DP_FINAL) && m_ds.m_dotSymp
                  && VN_IS(m_ds.m_dotSymp->nodep(), Cell)
                  && VN_CAST(m_ds.m_dotSymp->nodep(), Cell)->modp()
                  && VN_IS(VN_CAST(m_ds.m_dotSymp->nodep(), Cell)->modp(), Iface);
            যদি (!ifaceFinalSegmentAllowed) {
                checkNoDot(nodep);
            } অন্যথা {
                UINFO(9, indent() << "allow REFDTYPE under DOT for iface typedef name="
                                  << nodep->prettyNameQ() << " dotSym=" << m_ds.m_dotSymp);
                // Clear the dot state so we don't propagate errors
                m_ds.m_dotPos = DP_SCOPE;
            }
        }
        যদি (!nodep->typedefp() && !nodep->subDTypep()) {
            ধ্রুব VSymEnt* foundp;
            যদি (nodep->classOrPackagep()) {
                foundp = m_statep->get_গ্রন্থি_Sym(nodep->classOrPackagep())->findIdFlat(nodep->name());
            } অন্যথা যদি (m_ds.m_dotPos == DP_FIRST || m_ds.m_dotPos == DP_NONE) {
                foundp = findIdFallbackSkipMemberDType(m_curSymp, nodep->name());
            } অন্যথা {
                // Defensive: dotPos should be DP_FIRST/DP_NONE or classOrPackagep set.
                v3fatalSrc("Unexpected dotPos="
                           << static_cast<পূর্ণাঙ্ক>(m_ds.m_dotPos)  // LCOV_EXCL_LINE
                           << " in RefDType lookup for "
                           << nodep->prettyNameQ());  // LCOV_EXCL_LINE
                foundp = nullptr;  // LCOV_EXCL_LINE
            }
            {
                যদি (AstTypedef* ধ্রুব defp
                    = foundp ? VN_CAST(foundp->nodep(), Typedef) : nullptr) {
                    // Don't check if typedef is to a <type T>::<reference> as might not be
                    // resolved yet
                    যদি (!nodep->classOrPackagep() && !defp->isUnderClass())
                        checkDeclOrder(nodep, defp);
                    nodep->typedefp(defp);
                    nodep->classOrPackagep(foundp->classOrPackagep());

                } অন্যথা যদি (AstParamTypeDType* ধ্রুব defp
                           = foundp ? VN_CAST(foundp->nodep(), ParamTypeDType) : nullptr) {
                    // A PARAMTYPEDTYPE's child REFDTYPE referencing itself is the normal
                    // AST structure for type parameters (e.g., "type T = base" has a child
                    // REFDTYPE for the default type). Only error on true recursion where
                    // a TYPEDEF contains a reference back to itself.
                    ধ্রুব তার্কিক isParamTypeChild = (defp == nodep->backp());
                    ধ্রুব তার্কিক isTypedefRecursion
                        = isParamTypeChild && VN_IS(defp->backp(), Typedef);
                    যদি (isTypedefRecursion) {
                        nodep->v3error("Reference to '"
                                       << m_ds.m_dotText << (m_ds.m_dotText == "" ? "" : ".")
                                       << nodep->prettyName() << "'"
                                       << " type would form a recursive definition");
                        nodep->refDTypep(nodep->findVoidDType());  // Try to reduce later errors
                    } অন্যথা {
                        nodep->refDTypep(defp);
                        nodep->classOrPackagep(foundp->classOrPackagep());
                        captureIfaceParamType(nodep, defp);
                    }
                } অন্যথা যদি (AstClass* ধ্রুব defp
                           = foundp ? VN_CAST(foundp->nodep(), Class) : nullptr) {
                    // Don't check if typedef is to a <type T>::<reference> as might not be
                    // resolved yet
                    যদি (!nodep->classOrPackagep()) checkDeclOrder(nodep, defp);
                    AstPin* ধ্রুব paramsp = nodep->paramsp();
                    যদি (paramsp) paramsp->unlinkFrBackWithNext();
                    AstClassRefDType* ধ্রুব newp
                        = নূতন AstClassRefDType{nodep->fileline(), defp, paramsp};
                    newp->classOrPackagep(foundp->classOrPackagep());
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
                    প্রত্যাগম;
                } অন্যথা যদি (m_insideClassExtParam) {
                    প্রত্যাগম;
                } অন্যথা {
                    যদি (foundp) {
                        UINFO(1, "Found sym node: " << foundp->nodep());
                        nodep->v3error("Expecting a data type: "
                                       << nodep->prettyNameQ()
                                       << ", found: " << foundp->nodep()->prettyTypeName());
                    } অন্যথা {
                        nodep->v3error("Can't find typedef/interface: " << nodep->prettyNameQ());
                    }
                }
            }
        }
        iterateChildren(nodep);
    }
    শূন্য visit(AstRequireDType* nodep) override {
        // Handle type validation for localparam type assignments
        // Iterate child to resolve any ParseRef nodes
        iterateChildren(nodep);
        // Only emit error if the child is not a type.
        // Do NOT unwrap valid types here - leave that to V3Width.
        // Unwrapping here breaks type parameter resolution during cloning.
        // Allow Dot through to defer the handling until the dot expression is resolved.
        যদি (nodep->lhsp() && !VN_IS(nodep->lhsp(), গ্রন্থি_DType) && !VN_IS(nodep->lhsp(), Dot)) {
            // Not a type - emit error
            যদি (VN_IS(nodep->lhsp(), Const)) {
                nodep->lhsp()->v3error("Expecting a data type, not a constant");
            } অন্যথা {
                nodep->lhsp()->v3error("Expecting a data type, not "
                                       << nodep->lhsp()->typeName() << ": '"
                                       << nodep->lhsp()->prettyName() << "'");
            }
            // Replace with void to keep AST valid and allow clean exit
            nodep->replaceWith(নূতন AstVoidDType{nodep->fileline()});
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
        }
    }
    শূন্য visit(AstDpiExport* nodep) override {
        // AstDpiExport: Make sure the function referenced exists, then dump it
        LINKDOT_VISIT_START();
        iterateChildren(nodep);
        checkNoDot(nodep);
        VSymEnt* ধ্রুব foundp = m_curSymp->findIdFallback(nodep->name());
        Ast_গ্রন্থি_FTask* ধ্রুব taskp = foundp ? VN_AS(foundp->nodep(), গ্রন্থি_FTask) : nullptr;
        যদি (!taskp) {
            nodep->v3error(
                "Can't find definition of exported task/function: " << nodep->prettyNameQ());
        } অন্যথা যদি (taskp->dpiExport()) {
            nodep->v3error("Function was already DPI Exported, duplicate not allowed: "
                           << nodep->prettyNameQ());
        } অন্যথা {
            taskp->dpiExport(সত্য);
            যদি (nodep->cname() != "") taskp->cname(nodep->cname());
        }
        VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);
    }
    শূন্য visit(AstDisable* nodep) override {
        LINKDOT_VISIT_START();
        checkNoDot(nodep);
        তার্কিক hasPartSelect = অসত্য;
        ধ্রুব string targetPath
            = nodep->targetRefp() ? extractDottedPath(nodep->targetRefp(), hasPartSelect) : "";
        VL_RESTORER_COPY(m_ds);
        m_ds.init(m_curSymp);
        m_ds.m_dotPos = DP_FIRST;
        m_ds.m_disablep = nodep;
        iterateChildren(nodep);
        যদি (nodep->targetRefp()) {
            যদি (AstTaskRef* ধ্রুব taskRefp = VN_CAST(nodep->targetRefp(), TaskRef)) {
                nodep->targetp(taskRefp->taskp());
            } অন্যথা যদি (!VN_IS(nodep->targetRefp(), ParseRef)) {
                // If it is a ParseRef, either it couldn't be linked or it is linked to a block
                nodep->v3warn(E_UNSUPPORTED, "Node of type "
                                                 << nodep->targetRefp()->prettyTypeName()
                                                 << " referenced by disable");
                VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);
                প্রত্যাগম;
            }
            যদি (nodep->targetp()) {
                nodep->targetRefp()->unlinkFrBack()->delete_বৃক্ষ();
                যদি (!hasPartSelect) {
                    ধ্রুব string instancePath = targetInstancePath(nodep->targetp(), targetPath);
                    যদি (!instancePath.empty()) {
                        // Keep only the instance prefix so V3LinkJump can reference the selected
                        // instance's process queue without reparsing the disable target.
                        nodep->targetRefp(
                            নূতন AstVarXRef{nodep->fileline(), "", instancePath, VAccess::READ});
                    }
                }
            }
        }
    }
    শূন্য visitPackageImportOrExport(Ast_গ্রন্থি* nodep) {
        // No longer needed
        LINKDOT_VISIT_START();
        checkNoDot(nodep);
        যদি (m_statep->forParamed()) VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);
    }
    শূন্য visit(AstPackageImport* nodep) override { visitPackageImportOrExport(nodep); }
    শূন্য visit(AstPackageExport* nodep) override { visitPackageImportOrExport(nodep); }
    শূন্য visit(AstPackageExportStarStar* nodep) override { visitPackageImportOrExport(nodep); }
    শূন্য visit(AstCellRef* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        iterateChildren(nodep);
    }
    শূন্য visit(AstCellArrayRef* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        // Expression already iterated
    }
    শূন্য visit(AstUnlinkedRef* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        // No need to iterate, if we have a UnlinkedVarXRef, we're already done
    }
    শূন্য visit(Ast_Stmt_রাশিমালা* nodep) override {
        LINKDOT_VISIT_START();
        checkNoDot(nodep);
        // Check if nodep represents a super.new call;
        যদি (AstNew* ধ্রুব newExprp = VN_CAST(nodep->exprp(), New)) {
            // in this case it was already linked, so it doesn't have a super reference
            m_explicitSuperNewp = newExprp;
        } অন্যথা যদি (ধ্রুব AstDot* ধ্রুব dotp = VN_CAST(nodep->exprp(), Dot)) {
            যদি (dotp->lhsp()->name() == "super") {
                যদি (AstNew* ধ্রুব newExprp = VN_CAST(dotp->rhsp(), New)) {
                    m_explicitSuperNewp = newExprp;
                }
            }
        }
        iterateChildren(nodep);
    }

    শূন্য visit(AstIfaceRefDType* nodep) override {
        যদি (nodep->user3SetOnce()) প্রত্যাগম;
        LINKDOT_VISIT_START();
        যদি (nodep->paramsp()) {
            // If there is no parameters, there is no need to visit this node.
            AstIface* ধ্রুব ifacep = nodep->ifacep();
            UASSERT_OBJ(ifacep, nodep, "Port parameters of AstIfaceRefDType without ifacep()");
            যদি (ifacep->dead()) প্রত্যাগম;
            checkNoDot(nodep);
            VL_RESTORER_CLEAR(m_usedPins);
            VL_RESTORER_CLEAR(m_usedDefParamPins);
            VL_RESTORER(m_pinSymp);
            m_pinSymp = m_statep->get_গ্রন্থি_Sym(ifacep);
            iterateAndNextNull(nodep->paramsp());
        }
    }

    শূন্য visit(AstAttrOf* nodep) override { iterateChildren(nodep); }
    শূন্য visit(AstCastParse* nodep) override {
        যদি (nodep->user3SetOnce()) প্রত্যাগম;
        LINKDOT_VISIT_START();
        // A cast may start a dotted expression: HERE type'(expr) . member_or_method
        ধ্রুব তার্কিক dotLhs = m_ds.m_dotPos == DP_FIRST;
        যদি (!dotLhs) checkNoDot(nodep);
        {
            VL_RESTORER_COPY(m_ds);
            m_ds.init(m_curSymp);
            iterateChildren(nodep);
        }
        যদি (dotLhs) m_ds.m_dotPos = DP_MEMBER;
    }

    শূন্য visit(AstAssignW* nodep) override {
        LINKDOT_VISIT_START();
        checkNoDot(nodep);
        VL_RESTORER(m_replaceWithAlias);
        যদি (nodep->user2()) m_replaceWithAlias = অসত্য;
        iterateChildren(nodep);
    }

    শূন্য visit(AstTypedef* nodep) override {
        VL_RESTORER(m_resolvingTypedef)
        m_resolvingTypedef = সত্য;
        iterateChildren(nodep);
    }

    শূন্য visit(Ast_গ্রন্থি* nodep) override {
        VL_RESTORER(m_inPackedArray);
        যদি (VN_IS(nodep, PackArrayDType)) {
            m_inPackedArray = সত্য;
        } অন্যথা যদি (!m_inPackedArray) {
            LINKDOT_VISIT_START();
            checkNoDot(nodep);
        }
        iterateChildren(nodep);
    }

সার্বজনিক:
    // CONSTRUCTORS
    LinkDotResolveVisitor(AstNetlist* rootp, LinkDotState* statep)
        : m_statep{statep} {
        UINFO(4, __FUNCTION__ << ": ");

        // Note: no need to clear user3 on ledger entries here.
        // VNUser3InUse (m_inuser3) already logically clears user3 on all
        // nodes via the generation counter.  The ledger may also hold stale
        // refp pointers to deleted AST nodes, so iterating it is unsafe.

        iterate(rootp);
        ধ্রুব std::map<std::string, Ast_গ্রন্থি_Module*> modulesToRevisit
            = std::move(m_modulesToRevisit);
        m_lastDeferredp = nullptr;
        প্রতি (auto& p : modulesToRevisit) {
            Ast_গ্রন্থি_Module* ধ্রুব modp = p.second;
            modp->foreach([](Ast_গ্রন্থি* ধ্রুব nodep) { nodep->user3(অসত্য); });
            iterate(modp);
        }
    }
    ~LinkDotResolveVisitor() override = স্বভাব;
};

//######################################################################
// Link class functions

শূন্য V3LinkDot::dumpSubstep(ধ্রুব string& name) {
    যদি (dump_বৃক্ষ_Either_Level() >= 9) {
        V3Global::dump_Check_Global_বৃক্ষ(name);
    } অন্যথা যদি (debug() >= 5) {  // on high dbg level, dump even if not explicitly told to
        v3Global.rootp()->dump_বৃক্ষ_File(v3Global.debugFilename(name + ".tree"));
    }
}

শূন্য V3LinkDot::linkDotGuts(AstNetlist* rootp, VLinkDotStep step) {
    VIsCached::clear_Cache_বৃক্ষ();  // Avoid using any stale isPure
    dumpSubstep("prelinkdot");
    LinkDotState state{rootp, step};

    { LinkDotFindVisitor{rootp, &state}; }
    dumpSubstep("prelinkdot-find");

    { LinkDotFindIfaceVisitor{rootp, &state}; }
    dumpSubstep("prelinkdot-findiface");

    যদি (step == LDS_SCOPED) {
        // Well after the initial link when we're ready to operate on the flat design,
        // process AstScope's.  This needs to be separate pass after whole hierarchy graph created.
        { LinkDotScopeVisitor{rootp, &state}; }
        v3Global.assertScoped(সত্য);
        dumpSubstep("prelinkdot-scoped");
    } অন্যথা {
        // Initial link stage, resolve parameters and interfaces
        { LinkDotParamVisitor{rootp, &state}; }
        dumpSubstep("prelinkdot-param");
    }
    state.dumpSelf("prelinkdot");
    state.computeIfaceModSyms();
    state.computeIfaceVarSyms();
    state.computeScopeAliases();
    state.dumpSelf("linkdot-preresolve");
    ধ্রুব LinkDotResolveVisitor visitor{rootp, &state};
    state.dumpSelf("linkdot-done");
}

শূন্য V3LinkDot::linkDotPrimary(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    linkDotGuts(nodep, LDS_PRIMARY);
    V3Global::dump_Check_Global_বৃক্ষ("linkdot", 0, dump_বৃক্ষ_Either_Level() >= 6);
}

শূন্য V3LinkDot::linkDotParamed(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    linkDotGuts(nodep, LDS_PARAMED);
    V3Global::dump_Check_Global_বৃক্ষ("linkdotparam", 0, dump_বৃক্ষ_Either_Level() >= 3);
}

শূন্য V3LinkDot::linkDotScope(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    linkDotGuts(nodep, LDS_SCOPED);
    V3Global::dump_Check_Global_বৃক্ষ("linkdot", 0, dump_বৃক্ষ_Either_Level() >= 3);
}
