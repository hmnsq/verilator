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
//മുകൾ
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

#അന്തര്ഭാവ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#അന്തര്ഭാവ "V3LinkDot.h"

#അന്തര്ഭാവ "V3Global.h"
#അന്തര്ഭാവ "V3Graph.h"
#അന്തര്ഭാവ "V3LinkDotIfaceCapture.h"
#അന്തര്ഭാവ "V3MemberMap.h"
#അന്തര്ഭാവ "V3Parse.h"
#അന്തര്ഭാവ "V3Randomize.h"
#അന്തര്ഭാവ "V3Stats.h"
#അന്തര്ഭാവ "V3String.h"
#അന്തര്ഭാവ "V3SymTable.h"

#അന്തര്ഭാവ <algorithm>
#അന്തര്ഭാവ <cstdlib>
#അന്തര്ഭാവ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

സ്ഥിതം string extractDottedPath(Ast_കണ്ണി* nodep, തർക്കം& hasPartSelect) {
    എങ്കിൽ (AstParseRef* സ്ഥിരം refp = VN_CAST(nodep, ParseRef)) {
        തിരികെ refp->name();
    } അല്ലെങ്കിൽ എങ്കിൽ (AstVarRef* സ്ഥിരം refp = VN_CAST(nodep, VarRef)) {
        തിരികെ refp->name();
    } അല്ലെങ്കിൽ എങ്കിൽ (AstDot* സ്ഥിരം dotp = VN_CAST(nodep, Dot)) {
        സ്ഥിരം string lhs = extractDottedPath(dotp->lhsp(), hasPartSelect);
        സ്ഥിരം string rhs = extractDottedPath(dotp->rhsp(), hasPartSelect);
        തിരികെ VString::dot(lhs, ".", rhs);
    } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(nodep, SelBit) || VN_IS(nodep, SelExtract) || VN_IS(nodep, SelPlus)
               || VN_IS(nodep, SelMinus)) {
        hasPartSelect = സത്യം;
        തിരികെ "";
    }
    തിരികെ "";
}
സ്ഥിതം string lexicalDisablePath(Ast_കണ്ണി* സ്ഥിരം targetp) {
    std::vector<string> names;
    വേണ്ടി (Ast_കണ്ണി* curp = targetp; curp; curp = curp->aboveLoopp()) {
        എങ്കിൽ (Ast_കണ്ണി_Block* സ്ഥിരം blockp = VN_CAST(curp, കണ്ണി_Block)) {
            എങ്കിൽ (blockp->name() != "") names.തള്ളുക_back(blockp->name());
        } അല്ലെങ്കിൽ എങ്കിൽ (Ast_കണ്ണി_FTask* സ്ഥിരം ftaskp = VN_CAST(curp, കണ്ണി_FTask)) {
            names.തള്ളുക_back(ftaskp->name());
        } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(curp, കണ്ണി_Module)) {
            നിർത്തുക;
        }
    }
    string path;
    വേണ്ടി (auto it = names.crbegin(); it != names.crend(); ++it) {
        path = VString::dot(path, ".", *it);
    }
    തിരികെ path;
}
സ്ഥിതം string targetInstancePath(Ast_കണ്ണി* സ്ഥിരം targetp, സ്ഥിരം string& targetPath) {
    സ്ഥിരം string lexicalPath = lexicalDisablePath(targetp);
    എങ്കിൽ (lexicalPath == "" || targetPath == lexicalPath) തിരികെ "";
    സ്ഥിരം string suffix = "." + lexicalPath;
    എങ്കിൽ (!VString::endsWith(targetPath, suffix)) തിരികെ "";
    തിരികെ targetPath.substr(0, targetPath.size() - suffix.size());
}

// ######################################################################
//  Matcher classes (for suggestion matching)

വർഗ്ഗം Link_കണ്ണി_Matcher_Class_Or_Package final : പൊതു VNodeMatcher {
പൊതു:
    തർക്കം കണ്ണി_Match(സ്ഥിരം Ast_കണ്ണി* nodep) സ്ഥിരം override {
        തിരികെ VN_IS(nodep, Class) || VN_IS(nodep, Package);
    }
};
വർഗ്ഗം Link_കണ്ണി_Matcher_FTask final : പൊതു VNodeMatcher {
പൊതു:
    തർക്കം കണ്ണി_Match(സ്ഥിരം Ast_കണ്ണി* nodep) സ്ഥിരം override { തിരികെ VN_IS(nodep, കണ്ണി_FTask); }
};
വർഗ്ഗം Link_കണ്ണി_Matcher_Modport final : പൊതു VNodeMatcher {
പൊതു:
    തർക്കം കണ്ണി_Match(സ്ഥിരം Ast_കണ്ണി* nodep) സ്ഥിരം override { തിരികെ VN_IS(nodep, Modport); }
};
വർഗ്ഗം Link_കണ്ണി_Matcher_Prod final : പൊതു VNodeMatcher {
പൊതു:
    തർക്കം കണ്ണി_Match(സ്ഥിരം Ast_കണ്ണി* nodep) സ്ഥിരം override { തിരികെ VN_IS(nodep, RSProd); }
};
വർഗ്ഗം Link_കണ്ണി_Matcher_Var final : പൊതു VNodeMatcher {
പൊതു:
    തർക്കം കണ്ണി_Match(സ്ഥിരം Ast_കണ്ണി* nodep) സ്ഥിരം override {
        തിരികെ VN_IS(nodep, Var) || VN_IS(nodep, LambdaArgRef);
    }
};
വർഗ്ഗം Link_കണ്ണി_Matcher_Var_IO final : പൊതു VNodeMatcher {
പൊതു:
    തർക്കം കണ്ണി_Match(സ്ഥിരം Ast_കണ്ണി* nodep) സ്ഥിരം override {
        സ്ഥിരം AstVar* സ്ഥിരം varp = VN_CAST(nodep, Var);
        എങ്കിൽ (!varp) തിരികെ അസത്യം;
        തിരികെ varp->isIO();
    }
};
വർഗ്ഗം Link_കണ്ണി_Matcher_Var_Param final : പൊതു VNodeMatcher {
പൊതു:
    തർക്കം കണ്ണി_Match(സ്ഥിരം Ast_കണ്ണി* nodep) സ്ഥിരം override {
        സ്ഥിരം AstVar* സ്ഥിരം varp = VN_CAST(nodep, Var);
        എങ്കിൽ (!varp) തിരികെ അസത്യം;
        തിരികെ varp->isParam();
    }
};

//######################################################################
// LinkDot state, as a visitor of each AstNode

വർഗ്ഗം LinkDotState final {
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
    സ്ഥിരം VNUser1InUse m_inuser1;
    സ്ഥിരം VNUser2InUse m_inuser2;
    സ്ഥിരം VNUser4InUse m_inuser4;

പൊതു:
    // ENUMS
    // In order of priority, compute first ... compute last
    എണ്ണൽ SAMNum : uint8_t { SAMN_MODPORT, SAMN_IFTOP, SAMN__MAX };  // Values for m_scopeAliasMap

സ്വകാര്യ:
    // TYPES
    using ScopeAliasMap = std::unordered_map<VSymEnt*, VSymEnt*>;
    using IfaceModSyms = std::vector<std::pair<AstIface*, VSymEnt*>>;

    സ്ഥിതം LinkDotState* s_errorThisp;  // Last self, for error reporting only

    // MEMBERS
    VSymGraph m_syms;  // Symbol table by hierarchy
    VSymGraph m_mods;  // Symbol table of all module names
    VSymEnt* m_dunitEntp = nullptr;  // $unit entry
    std::multimap<std::string, VSymEnt*>
        m_nameScopeSymMap;  // Map of scope referenced by non-pretty textual name
    std::set<std::pair<Ast_കണ്ണി_Module*, std::string>>
        m_implicitNameSet;  // For [module][signalname] if we can implicitly create it
    std::array<ScopeAliasMap, SAMN__MAX> m_scopeAliasMap;  // Map of <lhs,rhs> aliases
    std::vector<VSymEnt*> m_ifaceVarSyms;  // List of AstIfaceRefDType's to be imported
    IfaceModSyms m_ifaceModSyms;  // List of AstIface+Symbols to be processed
    സ്ഥിരം VLinkDotStep m_step;  // Operational step

പൊതു:
    // METHODS

    ശൂന്യം dumpSelf(സ്ഥിരം string& nameComment, തർക്കം force = അസത്യം) {
        എങ്കിൽ (debug() >= 6 || dumpLevel() >= 6 || force) {
            സ്ഥിരം string filename = v3Global.debugFilename(nameComment) + ".txt";
            UINFO(4, "Dumping " << filename);
            സ്ഥിരം std::unique_ptr<std::ofstream> logp{V3File::new_ofstream(filename)};
            എങ്കിൽ (logp->fail()) v3fatal("Can't write file: " << filename);
            std::ostream& os = *logp;
            // TODO the symbol table has node pointers which may be
            // dangling, as we call deleteTree in these visitors without
            // pushing for later deletion (or deleting from the symbol table)
            // So, only completely safe to call this when under --debug or --debug-leak.
            m_syms.dumpSelf(os);
            തർക്കം first = സത്യം;
            വേണ്ടി (പൂർണ്ണസംഖ്യ samn = 0; samn < SAMN__MAX; ++samn) {
                എങ്കിൽ (!m_scopeAliasMap[samn].empty()) {
                    എങ്കിൽ (first) os << "\nScopeAliasMap:\n";
                    first = അസത്യം;
                    വേണ്ടി (ScopeAliasMap::iterator it = m_scopeAliasMap[samn].begin();
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
    സ്ഥിതം ശൂന്യം preErrorDumpHandler() {
        എങ്കിൽ (s_errorThisp) s_errorThisp->preErrorDump();
    }
    ശൂന്യം preErrorDump() {
        സ്ഥിതം തർക്കം s_diddump = അസത്യം;
        എങ്കിൽ (!s_diddump && dump_വൃക്ഷം_Level()) {
            s_diddump = സത്യം;
            dumpSelf("linkdot-preerr", സത്യം);
            v3Global.rootp()->dump_വൃക്ഷം_File(v3Global.debugFilename("linkdot-preerr.tree"));
        }
    }

    // CONSTRUCTORS
    LinkDotState(AstNetlist* rootp, VLinkDotStep step)
        : m_syms{rootp}
        , m_mods{rootp}
        , m_step{step} {
        UINFO(4, __FUNCTION__ << ": ");
        s_errorThisp = ഇത്;
        V3Error::errorExitCb(preErrorDumpHandler);  // If get error, dump self
        readModNames();
    }
    ~LinkDotState() {
        V3Error::errorExitCb(nullptr);
        s_errorThisp = nullptr;
    }

    // ACCESSORS
    VSymGraph* symsp() { തിരികെ &m_syms; }
    പൂർണ്ണസംഖ്യ step_സംഖ്യ() സ്ഥിരം { തിരികെ static_cast<പൂർണ്ണസംഖ്യ>(m_step); }
    തർക്കം forPrimary() സ്ഥിരം { തിരികെ m_step == LDS_PRIMARY; }
    തർക്കം forParamed() സ്ഥിരം { തിരികെ m_step == LDS_PARAMED; }
    തർക്കം forScopeCreation() സ്ഥിരം { തിരികെ m_step == LDS_SCOPED; }

    // METHODS
    സ്ഥിതം string കണ്ണി_Text_Type(Ast_കണ്ണി* nodep) {
        എങ്കിൽ (സ്ഥിരം AstVar* സ്ഥിരം varp = VN_CAST(nodep, Var)) {
            എങ്കിൽ (varp->isIO() || varp->isIfaceRef()) {
                തിരികെ "port";
            } അല്ലെങ്കിൽ എങ്കിൽ (varp->isGParam()) {
                തിരികെ "parameter";
            } അല്ലെങ്കിൽ എങ്കിൽ (varp->varType() == VVarType::LPARAM) {
                തിരികെ "local parameter";
            } അല്ലെങ്കിൽ {
                തിരികെ "variable";
            }
        } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstParamTypeDType* സ്ഥിരം dtypep = VN_CAST(nodep, ParamTypeDType)) {
            എങ്കിൽ (dtypep->isGParam()) {
                തിരികെ "type parameter";
            } അല്ലെങ്കിൽ {
                തിരികെ "local type parameter";
            }
        } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(nodep, Begin)) {
            തിരികെ "block";
        } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(nodep, Cell)) {
            തിരികെ "instance";
        } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(nodep, Constraint)) {
            തിരികെ "constraint";
        } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(nodep, Func)) {
            തിരികെ "function";
        } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(nodep, GenBlock)) {
            തിരികെ "generate block";
        } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(nodep, Iface)) {
            തിരികെ "interface";
        } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(nodep, RSProd)) {
            തിരികെ "randsequence production";
        } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(nodep, Task)) {
            തിരികെ "task";
        } അല്ലെങ്കിൽ {
            തിരികെ nodep->prettyTypeName();
        }
    }

    Ast_കണ്ണി_Module* findModuleSym(സ്ഥിരം string& modName) {
        സ്ഥിരം VSymEnt* സ്ഥിരം foundp = m_mods.rootp()->findIdFallback(modName);
        തിരികെ foundp ? VN_AS(foundp->nodep(), കണ്ണി_Module) : nullptr;
    }
    ശൂന്യം readModNames() {
        // Look at all modules, and store pointers to all module names
        വേണ്ടി (Ast_കണ്ണി_Module *nextp, *nodep = v3Global.rootp()->modulesp(); nodep; nodep = nextp) {
            nextp = VN_AS(nodep->nextp(), കണ്ണി_Module);
            m_mods.rootp()->തിരുകുക(nodep->name(), പുതിയ VSymEnt{&m_mods, nodep});
        }
    }

    VSymEnt* വേര്_Entp() സ്ഥിരം { തിരികെ m_syms.rootp(); }
    VSymEnt* dunitEntp() സ്ഥിരം { തിരികെ m_dunitEntp; }
    ശൂന്യം checkDuplicate(VSymEnt* lookupSymp, Ast_കണ്ണി* nodep, സ്ഥിരം string& name) {
        // Lookup the given name under current symbol table
        // Insert if not found
        // Report error if there's a duplicate
        //
        // Note we only check for conflicts at the same level; it's ok if one block hides another
        // We also wouldn't want to not insert it even though it's lower down

        സ്ഥിരം VSymEnt* സ്ഥിരം foundp = lookupSymp->findIdFlat(name);
        Ast_കണ്ണി* സ്ഥിരം fnodep = foundp ? foundp->nodep() : nullptr;
        എങ്കിൽ (!fnodep) {
            // Not found, will add in a moment.
            എങ്കിൽ (forPrimary() && VSymEnt::checkSimilarname(nodep)) {
                സ്ഥിരം VSymEnt* സ്ഥിരം alt = lookupSymp->findSimilarIdFlat(name);
                എങ്കിൽ (alt) {
                    nodep->v3warn(SIMILARNAME, "Declaration overlaps another with different case: "
                                                   << nodep->prettyNameQ() << '\n'
                                                   << nodep->warnContextPrimary() << '\n'
                                                   << alt->nodep()->warnOther()
                                                   << "... Location of original declaration\n"
                                                   << alt->nodep()->warnContextSecondary());
                }
            }
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep == fnodep) {  // Already inserted.
            // Good.
        } അല്ലെങ്കിൽ എങ്കിൽ (foundp->imported()) {  // From package
            // We don't throw VARHIDDEN as if the import is later the symbol
            // table's import wouldn't warn
        } അല്ലെങ്കിൽ എങ്കിൽ (forPrimary() && VN_IS(nodep, GenBlock)
                   && (VN_IS(fnodep, Begin) || VN_IS(fnodep, GenBlock))) {
            // Begin: ... blocks often replicate under genif/genfor, so
            // suppress duplicate checks.  See t_gen_forif.v for an example.
        } അല്ലെങ്കിൽ {
            UINFO(4, "name " << name);  // Not always same as nodep->name
            UINFO(4, "Var1 " << nodep);
            UINFO(4, "Var2 " << fnodep);
            // Quote the node's own name as for messages, if it is the symbol's
            സ്ഥിരം string nameQ
                = fnodep->name() == name ? fnodep->prettyNameQ() : Ast_കണ്ണി::prettyNameQ(name);
            എങ്കിൽ (nodep->type() == fnodep->type()) {
                nodep->v3error("Duplicate declaration of "
                               << കണ്ണി_Text_Type(fnodep) << ": " << nameQ << '\n'
                               << nodep->warnContextPrimary() << '\n'
                               << fnodep->warnOther() << "... Location of original declaration\n"
                               << fnodep->warnContextSecondary());
                nodep->decl_Token_അക്കം_Set_Min(0);  // Disable checking forward typedefs
                fnodep->decl_Token_അക്കം_Set_Min(0);  // Disable checking forward typedefs
            } അല്ലെങ്കിൽ {
                nodep->v3error("Unsupported in C: "
                               << ucfirst(കണ്ണി_Text_Type(nodep)) << " has the same name as "
                               << കണ്ണി_Text_Type(fnodep) << ": " << nameQ << '\n'
                               << nodep->warnContextPrimary() << '\n'
                               << fnodep->warnOther() << "... Location of original declaration\n"
                               << fnodep->warnContextSecondary());
            }
        }
    }
    ശൂന്യം തിരുകുക_DUnit(AstNetlist* nodep) {
        // $unit on top scope
        VSymEnt* സ്ഥിരം symp = പുതിയ VSymEnt{&m_syms, nodep};
        UINFO(9, "      INSERTdunit se" << cvtToHex(symp));
        symp->parentp(വേര്_Entp());  // Needed so backward search can find name of top module
        symp->fallbackp(nullptr);
        വേര്_Entp()->തിരുകുക("$unit ", symp);  // Space so can never name conflict with user code
        //
        UASSERT_OBJ(!m_dunitEntp, nodep, "Call insertDUnit only once");
        m_dunitEntp = symp;
    }
    VSymEnt* തിരുകുക_മുകൾ_Cell(Ast_കണ്ണി_Module* nodep, സ്ഥിരം string& scopename) {
        // Only called on the module at the very top of the hierarchy
        VSymEnt* സ്ഥിരം symp = പുതിയ VSymEnt{&m_syms, nodep};
        UINFO(9, "      INSERTtop se" << cvtToHex(symp) << "  " << scopename << " " << nodep);
        symp->parentp(വേര്_Entp());  // Needed so backward search can find name of top module
        symp->fallbackp(dunitEntp());  // Needed so can find $unit stuff
        nodep->user1p(symp);
        checkDuplicate(വേര്_Entp(), nodep, nodep->origName());
        വേര്_Entp()->തിരുകുക(nodep->origName(), symp);
        എങ്കിൽ (forScopeCreation()) m_nameScopeSymMap.emplace(scopename, symp);
        തിരികെ symp;
    }
    VSymEnt* തിരുകുക_മുകൾ_Iface(AstCell* nodep, സ്ഥിരം string& scopename) {
        VSymEnt* സ്ഥിരം symp = പുതിയ VSymEnt{&m_syms, nodep};
        UINFO(9, "      INSERTtopiface se" << cvtToHex(symp) << "  " << scopename << " " << nodep);
        symp->parentp(വേര്_Entp());  // Needed so backward search can find name of top module
        symp->fallbackp(dunitEntp());  // Needed so can find $unit stuff
        nodep->user1p(symp);
        എങ്കിൽ (nodep->modp()) nodep->modp()->user1p(symp);
        checkDuplicate(വേര്_Entp(), nodep, nodep->origName());
        വേര്_Entp()->തിരുകുക(nodep->origName(), symp);
        എങ്കിൽ (forScopeCreation()) m_nameScopeSymMap.emplace(scopename, symp);
        തിരികെ symp;
    }
    VSymEnt* തിരുകുക_Cell(VSymEnt* abovep, VSymEnt* modSymp, AstCell* nodep,
                        സ്ഥിരം string& scopename) {
        UASSERT_OBJ(abovep, nodep, "Null symbol table inserting node");
        VSymEnt* സ്ഥിരം symp = പുതിയ VSymEnt{&m_syms, nodep};
        UINFO(9, "      INSERTcel se" << cvtToHex(symp) << "  " << scopename << " above=se"
                                      << cvtToHex(abovep) << " mods=se" << cvtToHex(modSymp)
                                      << " node=" << nodep);
        symp->parentp(abovep);
        symp->fallbackp(dunitEntp());  // Needed so can find $unit stuff
        nodep->user1p(symp);
        എങ്കിൽ (nodep->modp()) nodep->modp()->user1p(symp);
        checkDuplicate(abovep, nodep, nodep->origName());
        abovep->reinsert(nodep->origName(), symp);
        എങ്കിൽ (forScopeCreation() && abovep != modSymp && !modSymp->findIdFlat(nodep->name())) {
            // If it's foo_DOT_bar, we need to be able to find it under "foo_DOT_bar" too.
            // Duplicates are possible, as until resolve generates might
            // have 2 same cells under an if
            modSymp->reinsert(nodep->name(), symp);
        }
        എങ്കിൽ (forScopeCreation()) m_nameScopeSymMap.emplace(scopename, symp);
        തിരികെ symp;
    }
    ശൂന്യം തിരുകുക_Map(VSymEnt* symp, സ്ഥിരം string& scopename) {
        എങ്കിൽ (forScopeCreation()) m_nameScopeSymMap.emplace(scopename, symp);
    }

    VSymEnt* തിരുകുക_Inline(VSymEnt* abovep, VSymEnt* modSymp, AstCellInline* nodep,
                          സ്ഥിരം string& basename) {
        // A fake point in the hierarchy, corresponding to a named block flattened by V3Begin.
        // Names are looked up in the module with the block's flattened prefix.
        UASSERT_OBJ(abovep, nodep, "Null symbol table inserting node");
        VSymEnt* സ്ഥിരം symp = പുതിയ VSymEnt{&m_syms, nodep};
        UINFO(9, "      INSERTinl se" << cvtToHex(symp) << "  " << basename << " above=se"
                                      << cvtToHex(abovep) << " mods=se" << cvtToHex(modSymp)
                                      << " node=" << nodep);
        symp->parentp(abovep);
        symp->fallbackp(modSymp);
        symp->symPrefix(nodep->name() + "__DOT__");
        nodep->user1p(symp);
        checkDuplicate(abovep, nodep, nodep->name());
        abovep->reinsert(basename, symp);
        എങ്കിൽ (abovep != modSymp && !modSymp->findIdFlat(nodep->name())) {
            // If it's foo_DOT_bar, we need to be able to find it under that too.
            modSymp->reinsert(nodep->name(), symp);
        }
        തിരികെ symp;
    }
    VSymEnt* തിരുകുക_Block(VSymEnt* abovep, സ്ഥിരം string& name, Ast_കണ്ണി* nodep,
                         Ast_കണ്ണി_Module* classOrPackagep) {
        // A fake point in the hierarchy, corresponding to a begin or function/task block
        // After we remove begins these will go away
        // Note we fallback to the symbol table of the parent, as we want to find variables there
        // However, cells walk the graph, so cells will appear under the begin/ftask itself
        UASSERT_OBJ(abovep, nodep, "Null symbol table inserting node");
        VSymEnt* സ്ഥിരം symp = പുതിയ VSymEnt{&m_syms, nodep};
        UINFO(9, "      INSERTblk se" << cvtToHex(symp) << "  above=se" << cvtToHex(abovep)
                                      << " pkg=" << cvtToHex(classOrPackagep)
                                      << "  node=" << nodep);
        symp->parentp(abovep);
        symp->classOrPackagep(classOrPackagep);
        symp->fallbackp(abovep);
        nodep->user1p(symp);
        എങ്കിൽ (name != "") checkDuplicate(abovep, nodep, name);
        // Duplicates are possible, as until resolve generates might have 2 same cells under an if
        abovep->reinsert(name, symp);
        തിരികെ symp;
    }
    VSymEnt* തിരുകുക_Sym(VSymEnt* abovep, സ്ഥിരം string& name, Ast_കണ്ണി* nodep,
                       Ast_കണ്ണി_Module* classOrPackagep) {
        UASSERT_OBJ(abovep, nodep, "Null symbol table inserting node");
        VSymEnt* സ്ഥിരം symp = പുതിയ VSymEnt{&m_syms, nodep};
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
        തിരികെ symp;
    }
    സ്ഥിതം തർക്കം exists_കണ്ണി_Sym(Ast_കണ്ണി* nodep) { തിരികെ nodep->user1p() != nullptr; }
    സ്ഥിതം VSymEnt* get_കണ്ണി_Sym(Ast_കണ്ണി* nodep) {
        // Don't use this in ResolveVisitor, as we need to pick up the proper
        // reference under each SCOPE
        VSymEnt* സ്ഥിരം symp = nodep->user1u().toSymEnt();
        UASSERT_OBJ(symp, nodep, "Module/etc never assigned a symbol entry?");
        തിരികെ symp;
    }
    VSymEnt* getScopeSym(AstScope* nodep) {
        സ്ഥിരം auto it = m_nameScopeSymMap.find(nodep->name());
        UASSERT_OBJ(it != m_nameScopeSymMap.end(), nodep,
                    "Scope never assigned a symbol entry '" << nodep->name() << "'");
        തിരികെ it->second;
    }
    ശൂന്യം implicitOkAdd(Ast_കണ്ണി_Module* nodep, സ്ഥിരം string& varname) {
        // Mark the given variable name as being allowed to be implicitly declared
        എങ്കിൽ (nodep) {
            സ്ഥിരം auto it = m_implicitNameSet.find(std::make_pair(nodep, varname));
            എങ്കിൽ (it == m_implicitNameSet.end()) m_implicitNameSet.emplace(nodep, varname);
        }
    }
    തർക്കം implicitOk(Ast_കണ്ണി_Module* nodep, സ്ഥിരം string& varname) {
        തിരികെ nodep
               && (m_implicitNameSet.find(std::make_pair(nodep, varname))
                   != m_implicitNameSet.end());
    }

    // Track and later recurse interface modules
    ശൂന്യം തിരുകുക_Iface_Mod_Sym(AstIface* nodep, VSymEnt* symp) {
        m_ifaceModSyms.emplace_back(nodep, symp);
    }
    ശൂന്യം computeIfaceModSyms();

    // Track and later insert interface references
    ശൂന്യം തിരുകുക_Iface_Var_Sym(VSymEnt* symp) {  // Where sym is for a VAR of dtype IFACEREFDTYPE
        m_ifaceVarSyms.തള്ളുക_back(symp);
    }
    // Iface for a raw or arrayed iface; peels nested array layers for multi-dim arrays.
    സ്ഥിതം AstIfaceRefDType* ifaceRefFromArray(Ast_കണ്ണി_DType* nodep) {
        ആകുമ്പോൾ (nodep) {
            എങ്കിൽ (AstIfaceRefDType* സ്ഥിരം ifp = VN_CAST(nodep, IfaceRefDType)) തിരികെ ifp;
            എങ്കിൽ (സ്ഥിരം AstBracketArrayDType* സ്ഥിരം arrp = VN_CAST(nodep, BracketArrayDType)) {
                nodep = arrp->subDTypep();
            } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstUnpackArrayDType* സ്ഥിരം arrp = VN_CAST(nodep, UnpackArrayDType)) {
                nodep = arrp->subDTypep();
            } അല്ലെങ്കിൽ {
                തിരികെ nullptr;
            }
        }
        തിരികെ nullptr;
    }
    // Given a pin expression, resolve it to a live AstIface* (or nullptr).
    // Handles both simple VarRef and dotted VarXRef pin connections.
    AstIface* live_Iface_From_Pin_വ്യഞ്ജകം(Ast_കണ്ണി* exprp, VSymEnt* parentSymp) {
        AstVar* resolvedVarp = nullptr;
        എങ്കിൽ (AstVarRef* സ്ഥിരം refp = VN_CAST(exprp, VarRef)) {
            resolvedVarp = refp->varp();
        } അല്ലെങ്കിൽ എങ്കിൽ (AstVarXRef* സ്ഥിരം xrefp = VN_CAST(exprp, VarXRef)) {
            VSymEnt* സ്ഥിരം lookupSymp = parentSymp ? parentSymp->parentp() : nullptr;
            എങ്കിൽ (lookupSymp) {
                string baddot;
                VSymEnt* okSymp = nullptr;
                സ്ഥിരം string dotpath = xrefp->dotted() + "." + xrefp->name();
                VSymEnt* സ്ഥിരം dotSymp
                    = findDotted(xrefp->fileline(), lookupSymp, dotpath, baddot, okSymp, സത്യം);
                എങ്കിൽ (dotSymp) resolvedVarp = VN_CAST(dotSymp->nodep(), Var);
            }
        }
        AstIface* resultp = nullptr;
        എങ്കിൽ (resolvedVarp) {
            AstIfaceRefDType* സ്ഥിരം irefp = ifaceRefFromArray(resolvedVarp->subDTypep());
            എങ്കിൽ (irefp && irefp->ifaceViaCellp() && !irefp->ifaceViaCellp()->dead()) {
                resultp = irefp->ifaceViaCellp();
            }
        }
        തിരികെ resultp;
    }

    // Attempt to repair a port's AstIfaceRefDType by tracing through the
    // parent cell's pin connection to find the correct live interface.
    // Returns true if repaired or already correct; false if unresolvable.
    തർക്കം repairIfaceRef(VSymEnt* varSymp, AstVar* varp, AstIfaceRefDType* ifacerefp, തർക്കം isDead) {
        // Walk up symbol-table parents to find the enclosing AstCell
        VSymEnt* parentSymp = varSymp->parentp();
        AstCell* parentCellp = nullptr;
        വേണ്ടി (പൂർണ്ണസംഖ്യ depth = 0; parentSymp && depth < 20; ++depth) {
            എങ്കിൽ (AstCell* സ്ഥിരം cp = VN_CAST(parentSymp->nodep(), Cell)) {
                parentCellp = cp;
                നിർത്തുക;
            }
            parentSymp = parentSymp->parentp();
        }
        എങ്കിൽ (!parentCellp) തിരികെ അസത്യം;
        // Scan pins to find the one connected to this port variable
        വേണ്ടി (AstPin* pinp = parentCellp->pinsp(); pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            എങ്കിൽ (pinp->modVarp() != varp && pinp->name() != varp->name()) തുടരുക;
            Ast_കണ്ണി* സ്ഥിരം exprp = pinp->exprp();
            എങ്കിൽ (!exprp) തിരികെ അസത്യം;
            AstIface* സ്ഥിരം newIfacep = live_Iface_From_Pin_വ്യഞ്ജകം(exprp, parentSymp);
            എങ്കിൽ (newIfacep && newIfacep != ifacerefp->ifaceViaCellp()) {
                UINFO(4, "  REPAIR-IFACE-REF var=" << varp->prettyNameQ()
                                                   << " old=" << ifacerefp->ifacep()->prettyNameQ()
                                                   << " new=" << newIfacep->prettyNameQ());
                ifacerefp->ifacep(newIfacep);
                തിരികെ സത്യം;
            }
            എങ്കിൽ (newIfacep) തിരികെ !isDead;  // Same interface - OK if live
            തിരികെ അസത്യം;
        }
        തിരികെ അസത്യം;
    }

    ശൂന്യം computeIfaceVarSyms() {
        വേണ്ടി (VSymEnt* varSymp : m_ifaceVarSyms) {
            AstVar* സ്ഥിരം varp = varSymp ? VN_AS(varSymp->nodep(), Var) : nullptr;
            UINFO(9, "  insAllIface se" << cvtToHex(varSymp) << " " << varp);
            AstIfaceRefDType* സ്ഥിരം ifacerefp = ifaceRefFromArray(varp->subDTypep());
            UASSERT_OBJ(ifacerefp, varp, "Non-ifacerefs on list!");
            എങ്കിൽ (!ifacerefp->cellp() && !ifacerefp->isVirtual() && (varp->pin_അക്കം() == 0)) {
                varp->v3error("Interface port declaration "
                              << varp->prettyNameQ() << " doesn't have corresponding port\n"
                              << varp->warnMore()
                                     + "... Perhaps intended an interface instantiation but "
                                       "are missing parenthesis (IEEE 1800-2023 25.3)?");
            }
            എങ്കിൽ (!ifacerefp->ifaceViaCellp()) {
                എങ്കിൽ (!ifacerefp->cellp()) {  // Probably a NotFoundModule, or a normal module if
                                            // made mistake
                    UINFO(1, "Associated cell " << Ast_കണ്ണി::prettyNameQ(ifacerefp->cellName()));
                    ifacerefp->v3error("Cannot find file containing interface: "
                                       << Ast_കണ്ണി::prettyNameQ(ifacerefp->ifaceName()));
                    തുടരുക;
                } അല്ലെങ്കിൽ {
                    ifacerefp->v3fatalSrc("Unlinked interface");
                }
            } അല്ലെങ്കിൽ എങ്കിൽ (varp->isIfaceRef() && !ifacerefp->cellp() && ifacerefp->ifaceViaCellp()) {
                // Port variable (cellp=null) pointing to an interface.
                // The interface may be dead (template replaced by clone) or
                // live-but-wrong (template still alive because some instances
                // use default params, but this port should point to a
                // specialized clone based on the pin connection).
                സ്ഥിരം തർക്കം isDead = ifacerefp->ifaceViaCellp()->dead();
                എങ്കിൽ (isDead && forPrimary() && !varp->isIfaceParent()
                    && !v3Global.opt.മുകൾ_Ifaces_Supported()) {
                    varp->v3warn(E_UNSUPPORTED,
                                 "Unsupported: Interfaced port on top level module");
                }
                // Attempt repair: trace through parent cell's pin to find
                // the correct live interface for this port variable
                സ്ഥിരം തർക്കം repaired = repairIfaceRef(varSymp, varp, ifacerefp, isDead);
                എങ്കിൽ (!repaired && isDead) {
                    ifacerefp->v3error(
                        "Interface " << Ast_കണ്ണി::prettyNameQ(ifacerefp->ifaceName())
                                     << " not connected as parent's interface not connected\n"
                                     << ifacerefp->warnMore()
                                     << "... Perhaps caused by another error on the parent "
                                        "interface that needs resolving\n"
                                     << ifacerefp->warnMore()
                                     << "... Or, perhaps intended an interface instantiation but "
                                        "are missing parenthesis (IEEE 1800-2023 25.3)?");
                    തുടരുക;
                }
            } അല്ലെങ്കിൽ എങ്കിൽ (ifacerefp->ifaceViaCellp()->dead()
                       || !exists_കണ്ണി_Sym(ifacerefp->ifaceViaCellp())) {
                ifacerefp->ifaceViaCellp()->v3fatalSrc(
                    "Interface referenced by virtual iface is dead. "
                    "Dead interfaces have required linking steps skipped");
            }
            VSymEnt* സ്ഥിരം ifaceSymp = get_കണ്ണി_Sym(ifacerefp->ifaceViaCellp());
            VSymEnt* ifOrPortSymp = ifaceSymp;
            // Link Modport names to the Modport Node under the Interface
            എങ്കിൽ (ifacerefp->isModport()) {
                VSymEnt* സ്ഥിരം foundp = ifaceSymp->findIdFallback(ifacerefp->modportName());
                തർക്കം ok = അസത്യം;
                എങ്കിൽ (foundp) {
                    എങ്കിൽ (AstModport* സ്ഥിരം modportp = VN_CAST(foundp->nodep(), Modport)) {
                        UINFO(4, "Link Modport: " << modportp);
                        ifacerefp->modportp(modportp);
                        ifOrPortSymp = foundp;
                        ok = സത്യം;
                    }
                }
                എങ്കിൽ (!ok) {
                    സ്ഥിരം string suggest = suggestSymFallback(ifaceSymp, ifacerefp->modportName(),
                                                              Link_കണ്ണി_Matcher_Modport{});
                    ifacerefp->modportFileline()->v3error(
                        "Modport not found under interface "
                        << ifacerefp->prettyNameQ(ifacerefp->ifaceName()) << ": "
                        << ifacerefp->prettyNameQ(ifacerefp->modportName()) << '\n'
                        << (suggest.empty() ? "" : ifacerefp->warnMore() + suggest));
                }
            }
            // Alias won't expand until interfaces and modport names are known; see notes at top
            തിരുകുക_Scope_Alias(SAMN_IFTOP, varSymp, ifOrPortSymp);
        }
        m_ifaceVarSyms.clear();
    }

    ശൂന്യം തിരുകുക_Scope_Alias(SAMNum samn, VSymEnt* lhsp, VSymEnt* rhsp) {
        // Track and later insert scope aliases; an interface referenced by
        // a child cell connecting to that interface
        // Typically lhsp=VAR w/dtype IFACEREF, rhsp=IFACE cell
        UINFO(9, "   insertScopeAlias se" << cvtToHex(lhsp) << " se" << cvtToHex(rhsp));
        UASSERT_OBJ(
            !(VN_IS(rhsp->nodep(), Cell) && !VN_IS(VN_AS(rhsp->nodep(), Cell)->modp(), Iface)),
            rhsp->nodep(), "Got a non-IFACE alias RHS");
        m_scopeAliasMap[samn].emplace(lhsp, rhsp);
    }
    ശൂന്യം computeScopeAliases() {
        UINFO(9, "computeIfaceAliases");
        വേണ്ടി (പൂർണ്ണസംഖ്യ samn = 0; samn < SAMN__MAX; ++samn) {
            വേണ്ടി (ScopeAliasMap::iterator it = m_scopeAliasMap[samn].begin();
                 it != m_scopeAliasMap[samn].end(); ++it) {
                VSymEnt* സ്ഥിരം lhsp = it->first;
                VSymEnt* srcp = lhsp;
                ആകുമ്പോൾ (സത്യം) {  // Follow chain of aliases up to highest level non-alias
                    സ്ഥിരം auto it2 = m_scopeAliasMap[samn].find(srcp);
                    എങ്കിൽ (it2 != m_scopeAliasMap[samn].end()) {
                        srcp = it2->second;
                        തുടരുക;
                    } അല്ലെങ്കിൽ {
                        നിർത്തുക;
                    }
                }
                UINFO(9, "  iiasa: Insert alias se" << lhsp << " (" << lhsp->nodep()->typeName()
                                                    << ") <- se" << srcp << " " << srcp->nodep());
                // srcp should be an interface reference pointing to the interface we want to
                // import
                lhsp->importFromIface(symsp(), srcp);
                // Allow access to objects not permissible to be listed in a modport
                എങ്കിൽ (VN_IS(srcp->nodep(), Modport)) {
                    lhsp->importFromIface(symsp(), srcp->parentp(), സത്യം);
                }
            }
            // m_scopeAliasMap[samn].clear();  // Done with it, but put into debug file
        }
    }

സ്വകാര്യ:
    VSymEnt* findWithAltFallback(VSymEnt* symp, സ്ഥിരം string& name, സ്ഥിരം string& altname) {
        VSymEnt* findp = symp->findIdFallback(name);
        എങ്കിൽ (findp) തിരികെ findp;
        എങ്കിൽ (altname != "") {
            UINFO(8, "     alt fallback");
            findp = symp->findIdFallback(altname);
        }
        തിരികെ findp;
    }

    VSymEnt* findWithAltFlat(VSymEnt* symp, സ്ഥിരം string& name, സ്ഥിരം string& altname) {
        VSymEnt* findp = symp->findIdFlat(name);
        എങ്കിൽ (findp) തിരികെ findp;
        എങ്കിൽ (altname != "") {
            UINFO(8, "     alt flat");
            findp = symp->findIdFlat(altname);
        }
        തിരികെ findp;
    }

പൊതു:
    VSymEnt* findDotted(File_വരി* refLocationp, VSymEnt* lookupSymp, സ്ഥിരം string& dotname,
                        string& baddot, VSymEnt*& okSymp, തർക്കം firstId) {
        // Given a dotted hierarchy name, return where in scope it is
        // Note when dotname=="" we just fall through and return lookupSymp
        UINFO(8, "    dottedFind se" << cvtToHex(lookupSymp) << " '" << dotname << "'");
        string leftname = dotname;
        okSymp = lookupSymp;  // So can list bad scopes
        ആകുമ്പോൾ (leftname != "") {  // foreach dotted part of xref name
            string::size_type pos = leftname.find('.');
            string ident;
            എങ്കിൽ (pos != string::npos) {
                ident = leftname.substr(0, pos);
                leftname = leftname.substr(pos + 1);
            } അല്ലെങ്കിൽ {
                ident = leftname;
                leftname = "";
            }
            baddot = ident;  // So user can see where they botched it
            okSymp = lookupSymp;
            string altIdent;
            എങ്കിൽ (!forScopeCreation()) {
                // GENFOR Begin is foo__BRA__##__KET__ after we've genloop unrolled,
                // but presently should be just "foo".
                // Likewise cell foo__[array] before we've expanded arrays is just foo.
                // Multi-dim iface arrays append multiple __BRA__..__KET__ suffixes; strip them
                // all.
                altIdent = ident;
                ആകുമ്പോൾ (VString::endsWith(altIdent, "__KET__")) {
                    സ്ഥിരം auto braPos = altIdent.rfind("__BRA__");
                    എങ്കിൽ (braPos == string::npos) നിർത്തുക;
                    altIdent = altIdent.substr(0, braPos);
                }
                എങ്കിൽ (altIdent == ident) altIdent.clear();
            }
            UINFO(8, "         id " << ident << " alt " << altIdent << " left " << leftname
                                    << " at se" << lookupSymp);
            // Spec says; Look at existing module (cellnames then modname),
            // then look up (inst name or modname)
            എങ്കിൽ (firstId) {
                // Check this module - subcellnames
                സ്ഥിരം AstCell* cellp = lookupSymp ? VN_CAST(lookupSymp->nodep(), Cell)
                                                  : nullptr;  // Replicated below
                എങ്കിൽ (VSymEnt* സ്ഥിരം findSymp = findWithAltFallback(lookupSymp, ident, altIdent)) {
                    lookupSymp = findSymp;
                }

                // Check this module - cur modname
                അല്ലെങ്കിൽ എങ്കിൽ (cellp && cellp->modp()->origName() == ident) {
                }
                // $root we walk up to Netlist
                അല്ലെങ്കിൽ എങ്കിൽ (ident == "$root") {
                    lookupSymp = വേര്_Entp();
                    // We've added the '$root' module, now everything else is one lower
                    എങ്കിൽ (forScopeCreation()) {
                        lookupSymp = lookupSymp->findIdFlat(ident);
                        UASSERT(lookupSymp, "Cannot find $root module under netlist");
                    }
                }
                // Move up and check cellname + modname
                അല്ലെങ്കിൽ {
                    തർക്കം crossedCell = അസത്യം;  // Crossed a cell boundary
                    ആകുമ്പോൾ (lookupSymp) {
                        lookupSymp = lookupSymp->parentp();
                        cellp = lookupSymp ? VN_CAST(lookupSymp->nodep(), Cell)
                                           : nullptr;  // Replicated above
                        എങ്കിൽ (lookupSymp) {
                            UINFO(9, "    Up to " << lookupSymp);
                            എങ്കിൽ (cellp || VN_IS(lookupSymp->nodep(), CellInline)) {
                                crossedCell = സത്യം;
                            }
                            എങ്കിൽ (cellp && cellp->modp()->origName() == ident) {
                                നിർത്തുക;
                            } അല്ലെങ്കിൽ എങ്കിൽ (VSymEnt* സ്ഥിരം findSym2p
                                       = findWithAltFallback(lookupSymp, ident, altIdent)) {
                                lookupSymp = findSym2p;
                                എങ്കിൽ (crossedCell && VN_IS(lookupSymp->nodep(), Var)) {
                                    UINFO(9, "    Not found but matches var name in parent "
                                                 << lookupSymp);
                                    തിരികെ nullptr;
                                }
                                നിർത്തുക;
                            }
                        } അല്ലെങ്കിൽ {
                            നിർത്തുക;
                        }
                    }
                    എങ്കിൽ (!lookupSymp) തിരികെ nullptr;  // Not found
                }
            } അല്ലെങ്കിൽ {  // Searching for middle path component, must be a cell or interface port
                VSymEnt* findSymp = findWithAltFlat(lookupSymp, ident, altIdent);
                എങ്കിൽ (findSymp) {
                    lookupSymp = findSymp;
                } അല്ലെങ്കിൽ {
                    // Try prefixed lookup for interface ports accessed through hierarchy
                    // (e.g., slave_inst.bus.data where bus is an interface port)
                    // After inlining, interface ports have prefixed names like
                    // top__DOT__slave_inst__DOT__bus
                    UINFO(8, "     middle-path: trying prefixed lookup for '" << ident << "'\n");
                    string baddot;
                    findSymp = findSymPrefixed(lookupSymp, ident, baddot, /*fallback=*/സത്യം);
                    UINFO(8, "     middle-path: prefixed lookup result: "
                                 << (findSymp ? "found" : "not found") << "\n");
                    എങ്കിൽ (findSymp) {
                        // Check if this is an interface reference variable
                        സ്ഥിരം AstVar* varp = VN_CAST(findSymp->nodep(), Var);
                        എങ്കിൽ (!varp) {
                            എങ്കിൽ (സ്ഥിരം AstVarScope* vscp = VN_CAST(findSymp->nodep(), VarScope)) {
                                varp = vscp->varp();
                            }
                        }
                        എങ്കിൽ (varp && varp->isIfaceRef()) {
                            // Found an interface port - use findSymp directly
                            // computeScopeAliases has already imported interface members
                            // into this symbol entry, so we use it instead of redirecting
                            // to the shared modport definition (which would cause all
                            // instances to resolve to the same symbols - bug #2656)
                            lookupSymp = findSymp;
                        } അല്ലെങ്കിൽ {
                            // Non-interface symbol in middle path must be a cell (module instance)
                            // to continue hierarchical resolution
                            എങ്കിൽ (VN_IS(findSymp->nodep(), Cell)) {
                                lookupSymp = findSymp;
                            } അല്ലെങ്കിൽ {
                                // Reject non-cell, non-interface symbols found via fallback
                                // to prevent accidental resolution to unrelated symbols
                                UINFO(8, "     middle-path: rejecting non-cell symbol\n");
                                തിരികെ nullptr;
                            }
                        }
                    } അല്ലെങ്കിൽ {
                        തിരികെ nullptr;  // Not found
                    }
                }
            }
            എങ്കിൽ (lookupSymp) {
                എങ്കിൽ (സ്ഥിരം AstCell* സ്ഥിരം cellp = VN_CAST(lookupSymp->nodep(), Cell)) {
                    എങ്കിൽ (സ്ഥിരം Ast_കണ്ണി_Module* സ്ഥിരം modp = cellp->modp()) {
                        എങ്കിൽ (modp->hierBlock() && !leftname.empty()) {
                            refLocationp->v3error("Cannot access scope inside hierarchical block");
                        } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(modp, NotFoundModule)) {
                            refLocationp->v3error("Dotted reference to instance that refers to "
                                                  "missing module/interface: "
                                                  << modp->prettyNameQ());
                        }
                    }
                }
                // Follow scope alias for nested interface port access
                എങ്കിൽ (!leftname.empty()) {
                    സ്ഥിരം auto aliasIt = m_scopeAliasMap[SAMN_IFTOP].find(lookupSymp);
                    എങ്കിൽ (aliasIt != m_scopeAliasMap[SAMN_IFTOP].end()) {
                        lookupSymp = aliasIt->second;
                        // Alias may point to __Viftop VarScope; find corresponding Cell
                        എങ്കിൽ (സ്ഥിരം AstVarScope* സ്ഥിരം vscp
                            = VN_CAST(lookupSymp->nodep(), VarScope)) {
                            സ്ഥിരം string varName = vscp->varp()->name();
                            സ്ഥിതം constexpr സ്ഥിരം അക്ഷരം* സ്ഥിരം VIFTOP_SUFFIX = "__Viftop";
                            എങ്കിൽ (VString::endsWith(varName, VIFTOP_SUFFIX)) {
                                സ്ഥിരം string cellName
                                    = varName.substr(0, varName.size() - strlen(VIFTOP_SUFFIX));
                                VSymEnt* സ്ഥിരം parentSymp = lookupSymp->parentp();
                                എങ്കിൽ (parentSymp) {
                                    VSymEnt* സ്ഥിരം cellSymp = parentSymp->findIdFlat(cellName);
                                    എങ്കിൽ (cellSymp && VN_IS(cellSymp->nodep(), Cell)) {
                                        lookupSymp = cellSymp;  // Use Cell for member lookup
                                    }
                                }
                            }
                        }
                    } അല്ലെങ്കിൽ {
                        // No alias; try following IfaceRefDType to interface cell
                        സ്ഥിരം AstVar* varp = VN_CAST(lookupSymp->nodep(), Var);
                        എങ്കിൽ (!varp) {
                            എങ്കിൽ (സ്ഥിരം AstVarScope* സ്ഥിരം vscp
                                = VN_CAST(lookupSymp->nodep(), VarScope)) {
                                varp = vscp->varp();
                            }
                        }
                        എങ്കിൽ (varp && varp->isIfaceRef()) {
                            എങ്കിൽ (സ്ഥിരം AstIfaceRefDType* സ്ഥിരം ifaceRefp
                                = ifaceRefFromArray(varp->dtypep())) {
                                എങ്കിൽ (ifaceRefp->cellp() && exists_കണ്ണി_Sym(ifaceRefp->cellp())) {
                                    lookupSymp = get_കണ്ണി_Sym(ifaceRefp->cellp());
                                } അല്ലെങ്കിൽ എങ്കിൽ (ifaceRefp->ifaceViaCellp()
                                           && exists_കണ്ണി_Sym(ifaceRefp->ifaceViaCellp())) {
                                    lookupSymp = get_കണ്ണി_Sym(ifaceRefp->ifaceViaCellp());
                                } അല്ലെങ്കിൽ എങ്കിൽ (ifaceRefp->ifacep()
                                           && exists_കണ്ണി_Sym(ifaceRefp->ifacep())) {
                                    lookupSymp = get_കണ്ണി_Sym(ifaceRefp->ifacep());
                                }
                            }
                        }
                    }
                }
            }
            firstId = അസത്യം;
        }
        തിരികെ lookupSymp;
    }

    സ്ഥിതം string removeLastInlineScope(സ്ഥിരം string& name) {
        string out = name;
        സ്ഥിരം string dot = "__DOT__";
        സ്ഥിരം size_t dotPos = out.rfind(dot, out.size() - dot.length() - 1);
        എങ്കിൽ (dotPos == string::npos) {
            തിരികെ "";
        } അല്ലെങ്കിൽ {
            തിരികെ out.erase(dotPos + dot.length(), string::npos);
        }
    }

    VSymEnt* findSymPrefixed(VSymEnt* lookupSymp, സ്ഥിരം string& dotname, string& baddot,
                             തർക്കം fallback) {
        // Find symbol in given point in hierarchy, allowing prefix (post-Inline)
        // For simplicity lookupSymp may be passed nullptr result from findDotted
        എങ്കിൽ (!lookupSymp) തിരികെ nullptr;
        UINFO(8, "    findSymPrefixed "
                     << dotname << " under se" << cvtToHex(lookupSymp)
                     << ((lookupSymp->symPrefix() == "") ? "" : " as ")
                     << ((lookupSymp->symPrefix() == "") ? "" : lookupSymp->symPrefix() + dotname)
                     << "  at se" << lookupSymp << " fallback=" << fallback);
        string prefix = lookupSymp->symPrefix();
        VSymEnt* foundp = nullptr;
        ആകുമ്പോൾ (!foundp) {
            എങ്കിൽ (fallback) {
                foundp = lookupSymp->findIdFallback(prefix + dotname);  // Might be nullptr
            } അല്ലെങ്കിൽ {
                foundp = lookupSymp->findIdFlat(prefix + dotname);  // Might be nullptr
            }
            എങ്കിൽ (prefix.empty()) നിർത്തുക;
            സ്ഥിരം string nextPrefix = removeLastInlineScope(prefix);
            എങ്കിൽ (prefix == nextPrefix) നിർത്തുക;
            prefix = nextPrefix;
        }
        എങ്കിൽ (!foundp) baddot = dotname;
        തിരികെ foundp;
    }
    സ്ഥിതം തർക്കം checkIfClassOrPackage(സ്ഥിരം VSymEnt* സ്ഥിരം symp) {
        എങ്കിൽ (VN_IS(symp->nodep(), Class) || VN_IS(symp->nodep(), Package)) തിരികെ സത്യം;

        // Helper: check if a RefDType might resolve to a class later.
        സ്ഥിരം auto isPotentialClassRef = [](സ്ഥിരം AstRefDType* refp) -> തർക്കം {
            തിരികെ refp && !refp->typeofp()
                   && (!refp->classOrPackageOpp()
                       || VN_IS(refp->classOrPackageOpp(), ClassOrPackageRef));
        };

        // Helper: allow types that can represent a class/package handle or an unresolved ref.
        // BasicDType is allowed here only for null/object handles.
        സ്ഥിരം auto is_Valid_Type_കണ്ണി = [](സ്ഥിരം Ast_കണ്ണി* nodep) -> തർക്കം {
            തിരികെ VN_IS(nodep, VoidDType) || VN_IS(nodep, BasicDType)
                   || VN_IS(nodep, ClassRefDType) || VN_IS(nodep, ParseRef);
        };

        എങ്കിൽ (സ്ഥിരം AstTypedef* സ്ഥിരം typedefp = VN_CAST(symp->nodep(), Typedef)) {
            സ്ഥിരം Ast_കണ്ണി_DType* dtypep = typedefp->subDTypep();
            എങ്കിൽ (!dtypep) dtypep = typedefp->childDTypep();
            എങ്കിൽ (VN_IS(dtypep, ClassRefDType)) തിരികെ സത്യം;
            എങ്കിൽ (isPotentialClassRef(VN_CAST(dtypep, RefDType))) തിരികെ സത്യം;
        } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstParamTypeDType* സ്ഥിരം paramTypep
                   = VN_CAST(symp->nodep(), ParamTypeDType)) {
            // Before V3Param the declared default is in childDTypep (possibly
            // wrapped in a RequireDType); after V3Param it is consumed and the
            // bound type is the resolved data type, e.g. a type parameter
            // inherited from a specialized base class (REQ #(Item) -> class Item).
            Ast_കണ്ണി* childp = paramTypep->childDTypep();
            എങ്കിൽ (സ്ഥിരം AstRequireDType* സ്ഥിരം reqp = VN_CAST(childp, RequireDType)) {
                childp = reqp->lhsp();
            }
            സ്ഥിരം Ast_കണ്ണി* സ്ഥിരം checkp = childp ? childp : paramTypep->skipRefp();
            എങ്കിൽ (is_Valid_Type_കണ്ണി(checkp) || isPotentialClassRef(VN_CAST(checkp, RefDType))) {
                തിരികെ സത്യം;
            }
        }
        തിരികെ അസത്യം;
    }
    VSymEnt* resolveClassOrPackage(VSymEnt* lookSymp, AstClassOrPackageRef* nodep, തർക്കം fallback,
                                   തർക്കം classOnly, സ്ഥിരം string& forWhat,
                                   തർക്കം deferIfUnresolved = അസത്യം) {
        എങ്കിൽ (nodep->classOrPackageSkipp()) തിരികെ get_കണ്ണി_Sym(nodep->classOrPackageSkipp());
        VSymEnt* foundp;
        VSymEnt* തിരയൽ_Symp = lookSymp;
        എങ്കിൽ (VL_UNCOVERABLE(തിരയൽ_Symp && VN_IS(തിരയൽ_Symp->nodep(), ParamTypeDType))) {
            തിരയൽ_Symp->nodep()->v3fatalSrc(  // LCOV_EXCL_LINE
                "resolveClassOrPackage: unexpected ParamTypeDType lookup");
        }
        എങ്കിൽ (fallback) {
            VSymEnt* currentLookSymp = തിരയൽ_Symp;
            ചെയ്യുക {
                foundp = currentLookSymp->findIdFlat(nodep->name());
                എങ്കിൽ (foundp && !checkIfClassOrPackage(foundp)) foundp = nullptr;
                എങ്കിൽ (!foundp) currentLookSymp = currentLookSymp->fallbackp();
            } ആകുമ്പോൾ (!foundp && currentLookSymp);
        } അല്ലെങ്കിൽ {
            foundp = തിരയൽ_Symp->findIdFlat(nodep->name());
            എങ്കിൽ (foundp && !checkIfClassOrPackage(foundp)) foundp = nullptr;
        }
        എങ്കിൽ (!foundp && v3Global.rootp()->stdPackagep()) {  // Look under implied std::
            foundp = get_കണ്ണി_Sym(v3Global.rootp()->stdPackagep())->findIdFlat(nodep->name());
        }
        എങ്കിൽ (foundp) {
            nodep->classOrPackageNodep(foundp->nodep());
            തിരികെ foundp;
        }
        എങ്കിൽ (deferIfUnresolved) തിരികെ nullptr;
        സ്ഥിരം string suggest
            = suggestSymFallback(lookSymp, nodep->name(), Link_കണ്ണി_Matcher_Class_Or_Package{});
        nodep->v3error((classOnly ? "Class" : "Package/class")
                       << " for '" << forWhat  // extends/implements
                       << "' not found: " << nodep->prettyNameQ() << '\n'
                       << (suggest.empty() ? "" : nodep->warnMore() + suggest));
        തിരികെ nullptr;
    }
    string suggestSymFallback(VSymEnt* lookupSymp, സ്ഥിരം string& name,
                              സ്ഥിരം VNodeMatcher& matcher) {
        // Suggest alternative symbol in given point in hierarchy
        // Does not support inline, as we find user-level errors before inlining
        // For simplicity lookupSymp may be passed nullptr result from findDotted
        എങ്കിൽ (!lookupSymp) തിരികെ "";
        VSpellCheck speller;
        lookupSymp->candidateIdFallback(&speller, &matcher);
        തിരികെ speller.best_Candidate_സന്ദേശം(name);
    }
    string suggestSymFlat(VSymEnt* lookupSymp, സ്ഥിരം string& name, സ്ഥിരം VNodeMatcher& matcher,
                          തർക്കം includeType) {
        എങ്കിൽ (!lookupSymp) തിരികെ "";
        VSpellCheck speller;
        lookupSymp->candidateIdFlat(&speller, &matcher, includeType);
        തിരികെ speller.best_Candidate_സന്ദേശം(name);
    }
};

LinkDotState* LinkDotState::s_errorThisp = nullptr;

//======================================================================

വർഗ്ഗം LinkDotFindVisitor final : പൊതു VNVisitor {
    // NODE STATE
    // Cleared on global
    //  *::user1p()             -> See LinkDotState
    //  *::user2p()             -> See LinkDotState
    //  *::user3()              // bool. Node stored in m_virtualIfaces
    //  *::user4()              -> See LinkDotState
    സ്ഥിരം VNUser3InUse m_inuser3;

    // STATE
    LinkDotState* സ്ഥിരം m_statep;  // State to pass between visitors, including symbol table
    Ast_കണ്ണി_Module* m_classOrPackagep = nullptr;  // Current package
    AstClocking* m_clockingp = nullptr;  // Current clocking block
    std::vector<AstScope*> m_scopes;  // Scopes detached during repeated module visits
    uint64_t m_statScopeVisits = 0;  // Non-top scopes encountered while finding instance symbols
    VSymEnt* m_modSymp = nullptr;  // Symbol Entry for current module
    VSymEnt* m_curSymp = nullptr;  // Symbol Entry for current table, where to lookup/insert
    string
        m_hierParamsName;  // Name of module with hierarchical type parameters, empty when not used
    string m_scope;  // Scope text
    സ്ഥിരം Ast_കണ്ണി_FTask* m_ftaskp = nullptr;  // Current function/task
    തർക്കം m_inRecursion = അസത്യം;  // Inside a recursive module
    പൂർണ്ണസംഖ്യ m_param_അക്കം = 0;  // Parameter number, for position based connection
    തർക്കം m_explicitNew = അസത്യം;  // Hit a "new" function
    പൂർണ്ണസംഖ്യ m_mod_Block_അക്കം = 0;  // Begin block number in module, 0=none seen
    പൂർണ്ണസംഖ്യ m_mod_With_അക്കം = 0;  // With block number, 0=none seen
    പൂർണ്ണസംഖ്യ m_mod_Arg_അക്കം = 0;  // Arg block number for randomize(), 0=none seen
    std::vector<AstIface*> m_virtIfaces;  // interfaces used as virtual,
                                          // needed for handleUnvisitedVirtIfaces()

    // METHODS
    ശൂന്യം makeImplicitNew(AstClass* nodep) {
        AstFunc* സ്ഥിരം newp = പുതിയ AstFunc{nodep->fileline(), "new", nullptr, nullptr};
        // If needed, super.new() call is added the 2nd pass of V3LinkDotResolve,
        // because base classes are already resolved there.
        newp->isConstructor(സത്യം);
        nodep->addMembersp(newp);
        UINFO(8, "Made implicit new for " << nodep->name() << ": " << nodep);
        m_statep->തിരുകുക_Block(m_curSymp, newp->name(), newp, m_classOrPackagep);
    }

    സ്ഥിതം തർക്കം isHierBlockWrapper(സ്ഥിരം string& name) {
        സ്ഥിരം V3HierBlockOptSet& hierBlocks = v3Global.opt.hierBlocks();
        തിരികെ hierBlocks.find(name) != hierBlocks.end();
    }

    ശൂന്യം moveExternFuncDecl(Ast_കണ്ണി_FTask* nodep, AstClass* classp) {
        // Move an 'extern function|task' to inside a class or, from an outer to inner-class
        എങ്കിൽ (nodep->isExternDef()) തിരികെ;
        // Move it to proper spot under the target class
        nodep->unlinkFrBack();
        classp->addStmtsp(nodep);
        nodep->isExternDef(സത്യം);  // So we check there's a matching extern
        nodep->classOrPackagep()->unlinkFrBack()->delete_വൃക്ഷം();
        // Any "Type::" reference in the function's IO are really "MovedToClass::" references
        എങ്കിൽ (nodep->fvarp()) moveExternFuncDeclRefs(nodep->fvarp(), classp);
        വേണ്ടി (Ast_കണ്ണി* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            എങ്കിൽ (AstVar* സ്ഥിരം portp = VN_CAST(stmtp, Var)) {
                എങ്കിൽ (portp->isIO()) moveExternFuncDeclRefs(portp, classp);
            }
        }
    }
    ശൂന്യം moveExternFuncDeclRefs(Ast_കണ്ണി* nodep, AstClass* classp) {
        nodep->foreach([ഇത്, classp](AstClassOrPackageRef* refp) {  //
            എങ്കിൽ (refp->name() == classp->name() && !refp->paramsp()) {
                UINFO(9, "Cleaning up external function type for class " << refp);
                AstDot* സ്ഥിരം dotp = VN_CAST(refp->backp(), Dot);
                എങ്കിൽ (dotp && dotp->lhsp() == refp) {
                    // If had DOT(CLASSREF this, ...) the DOT can go away
                    dotp->replaceWith(dotp->rhsp()->unlinkFrBack());
                    VL_DO_DANGLING2(തള്ളുക_Deletep(dotp), dotp, refp);
                } അല്ലെങ്കിൽ {
                    VL_DO_DANGLING(തള്ളുക_Deletep(refp->unlinkFrBack()), refp);
                }
            }
        });
    }

    // VISITORS
    ശൂന്യം visit(AstNetlist* nodep) override {  // FindVisitor::
        // Process $unit or other packages
        // Not needed - dotted references not allowed from inside packages
        // for (AstNodeModule* nodep = v3Global.rootp()->modulesp();
        //     nodep; nodep=VN_AS(nodep->nextp(), NodeModule)) {
        //    if (VN_IS(nodep, Package)) {}}

        m_statep->തിരുകുക_DUnit(nodep);

        // First back iterate, to find all packages. Backward as must do base
        // packages before using packages
        iterateChildrenBackwardsConst(nodep);

        // The first modules in the list are always the top modules
        // (sorted before this is called).
        // This may not be the module with isTop() set, as early in the steps,
        // wrapTop may have not been created yet.
        // $unit and the constant pool always exist, so nothing else, and nothing in $unit,
        // means nothing was given
        Ast_കണ്ണി_Module* സ്ഥിരം modulesp = nodep->modulesp();
        UASSERT_OBJ(modulesp, nodep, "$unit should always be in the netlist");
        എങ്കിൽ (m_statep->forPrimary()) {
            സ്ഥിരം തർക്കം onlyBuiltins = [&]() {
                വേണ്ടി (Ast_കണ്ണി_Module* modp = modulesp; modp;
                     modp = VN_AS(modp->nextp(), കണ്ണി_Module)) {
                    എങ്കിൽ (!modp->isDollarUnit() && !modp->isConstPool()) തിരികെ അസത്യം;
                }
                തിരികെ സത്യം;
            }();
            എങ്കിൽ (onlyBuiltins && !v3Global.rootp()->dollarUnitPkgp()->stmtsp()) {
                nodep->v3error("No top level module found");
            }
        }
        വേണ്ടി (Ast_കണ്ണി_Module* modp = nodep->modulesp(); modp && modp->is_മുകൾ();
             modp = VN_AS(modp->nextp(), കണ്ണി_Module)) {
            UINFO(8, "Top Module: " << modp);
            m_scope = "TOP";

            എങ്കിൽ (!m_statep->forScopeCreation() && v3Global.opt.മുകൾ_Ifaces_Supported()) {
                വേണ്ടി (Ast_കണ്ണി* subnodep = modp->stmtsp(); subnodep; subnodep = subnodep->nextp()) {
                    എങ്കിൽ (AstVar* സ്ഥിരം varp = VN_CAST(subnodep, Var)) {
                        എങ്കിൽ (varp->isIfaceRef()) {
                            സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം subtypep = varp->subDTypep();
                            സ്ഥിരം AstIfaceRefDType* ifacerefp = nullptr;
                            എങ്കിൽ (VN_IS(subtypep, IfaceRefDType)) {
                                ifacerefp = VN_AS(varp->subDTypep(), IfaceRefDType);
                            } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(subtypep, BracketArrayDType)) {
                                സ്ഥിരം AstBracketArrayDType* സ്ഥിരം arrp
                                    = VN_AS(subtypep, BracketArrayDType);
                                സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം arrsubtypep = arrp->subDTypep();
                                എങ്കിൽ (VN_IS(arrsubtypep, IfaceRefDType)) {
                                    ifacerefp = VN_AS(arrsubtypep, IfaceRefDType);
                                }
                            } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(subtypep, UnpackArrayDType)) {
                                സ്ഥിരം AstUnpackArrayDType* സ്ഥിരം arrp
                                    = VN_AS(subtypep, UnpackArrayDType);
                                സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം arrsubtypep = arrp->subDTypep();
                                എങ്കിൽ (VN_IS(arrsubtypep, IfaceRefDType)) {
                                    ifacerefp = VN_AS(arrsubtypep, IfaceRefDType);
                                }
                            }

                            എങ്കിൽ (ifacerefp && !ifacerefp->cellp()) {
                                // A dummy cell to keep the top level interface alive and correctly
                                // optimized for default parameter values
                                AstCell* ifacecellp
                                    = പുതിയ AstCell{nodep->fileline(),
                                                  nodep->fileline(),
                                                  modp->name() + "__02E" + varp->name(),
                                                  ifacerefp->ifaceName(),
                                                  nullptr,
                                                  nullptr,
                                                  nullptr};
                                ifacecellp->modp(ifacerefp->ifacep());
                                m_curSymp = m_modSymp
                                    = m_statep->തിരുകുക_മുകൾ_Iface(ifacecellp, m_scope);
                                iterate(ifacecellp);
                                VL_DO_DANGLING(തള്ളുക_Deletep(ifacecellp), ifacecellp);
                            }
                        }
                    }
                }
            }

            m_curSymp = m_modSymp = m_statep->തിരുകുക_മുകൾ_Cell(modp, m_scope);
            { iterate(modp); }

            m_scope = "";
            m_curSymp = m_modSymp = nullptr;
        }
    }
    ശൂന്യം visit(AstTypeTable*) override {}  // FindVisitor::
    ശൂന്യം visit(AstIfaceRefDType* nodep) override {  // FindVisitor::
        എങ്കിൽ (!m_statep->forScopeCreation() && nodep->isVirtual() && nodep->ifacep()
            && !nodep->ifacep()->user3()) {
            m_virtIfaces.തള്ളുക_back(nodep->ifacep());
            nodep->ifacep()->user3(സത്യം);
        }
        iterateChildren(nodep);
    }
    ശൂന്യം visit(Ast_കണ്ണി_Module* nodep) override {  // FindVisitor::
        // Called on top module from Netlist, other modules from the cell creating them,
        // and packages
        UINFO(8, "   " << nodep);
        // m_curSymp/m_modSymp maybe nullptr for packages and non-top modules
        // Packages will be under top after the initial phases, but until then
        // need separate handling
        സ്ഥിരം തർക്കം standalonePkg
            = !m_modSymp && !m_statep->forScopeCreation() && VN_IS(nodep, Package);
        സ്ഥിരം തർക്കം doit = (m_modSymp || standalonePkg);
        VL_RESTORER_COPY(m_scope);
        VL_RESTORER(m_classOrPackagep);
        VL_RESTORER(m_modSymp);
        VL_RESTORER(m_curSymp);
        VL_RESTORER(m_param_അക്കം);
        VL_RESTORER(m_mod_Block_അക്കം);
        VL_RESTORER(m_mod_With_അക്കം);
        VL_RESTORER(m_mod_Arg_അക്കം);
        എങ്കിൽ (doit && nodep->user2()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Identically recursive module (module instantiates "
                          "itself, without changing parameters): "
                              << Ast_കണ്ണി::prettyNameQ(nodep->origName()));
        } അല്ലെങ്കിൽ എങ്കിൽ (doit) {
            UINFO(4, "     Link Module: " << nodep);
            UASSERT_OBJ(!nodep->dead(), nodep, "Module in instance tree mislabeled as dead?");
            AstPackage* സ്ഥിരം pkgp = VN_CAST(nodep, Package);
            m_classOrPackagep = pkgp;
            എങ്കിൽ (standalonePkg) {
                എങ്കിൽ (pkgp->isDollarUnit()) {
                    m_curSymp = m_modSymp = m_statep->dunitEntp();
                    nodep->user1p(m_curSymp);
                } അല്ലെങ്കിൽ {
                    VSymEnt* സ്ഥിരം upperSymp = m_statep->dunitEntp();
                    m_scope = nodep->name();
                    m_curSymp = m_modSymp = m_statep->തിരുകുക_Block(upperSymp, nodep->name(), nodep,
                                                                  m_classOrPackagep);
                    UINFO(9, "New module scope " << m_curSymp);
                }
            }
            //
            m_param_അക്കം = 0;
            m_mod_Block_അക്കം = 0;
            m_mod_With_അക്കം = 0;
            m_mod_Arg_അക്കം = 0;
            // m_modSymp/m_curSymp for non-packages set by AstCell above this module
            // Iterate
            nodep->user2(സത്യം);
            iterateChildren(nodep);
            nodep->user2(അസത്യം);
            nodep->user4(സത്യം);
            // Interfaces need another pass when signals are resolved. When creating
            // scopes this is done from the AstScopes in LinkDotScopeVisitor instead, as
            // only those entries hold the AstVarScopes the modport items resolve against.
            എങ്കിൽ (!m_statep->forScopeCreation()) {
                എങ്കിൽ (AstIface* സ്ഥിരം ifacep = VN_CAST(nodep, Iface)) {
                    m_statep->തിരുകുക_Iface_Mod_Sym(ifacep, m_curSymp);
                }
            }
        } അല്ലെങ്കിൽ എങ്കിൽ (isHierBlockWrapper(nodep->name())) {
            UINFO(5, "Module is hierarchical block, must not be dead: " << nodep);
            m_scope = nodep->name();
            VSymEnt* സ്ഥിരം upperSymp = m_curSymp ? m_curSymp : m_statep->വേര്_Entp();
            m_curSymp = m_modSymp
                = m_statep->തിരുകുക_Block(upperSymp, nodep->name() + "::", nodep, m_classOrPackagep);
            iterateChildren(nodep);
            nodep->user4(സത്യം);
        } അല്ലെങ്കിൽ {  // !doit
            എങ്കിൽ (nodep->hierParams()) {
                UINFO(1, "Found module with hier type parameters");
                m_hierParamsName = nodep->name();
                വേണ്ടി (സ്ഥിരം Ast_കണ്ണി* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    എങ്കിൽ (സ്ഥിരം AstTypedef* സ്ഥിരം tdef = VN_CAST(stmtp, Typedef)) {
                        UINFO(1, "Inserting hier type parameter typedef: " << tdef);
                        VSymEnt* സ്ഥിരം upperSymp = m_curSymp ? m_curSymp : m_statep->വേര്_Entp();
                        m_curSymp = m_modSymp = m_statep->തിരുകുക_Block(upperSymp, nodep->name(),
                                                                      nodep, m_classOrPackagep);
                    }
                }
            } അല്ലെങ്കിൽ {
                // Will be optimized away later
                // Can't remove now, as our backwards iterator will throw up
                UINFO(5, "Module not under any CELL or top - dead module: " << nodep);
            }
        }
    }

    ശൂന്യം visit(AstClass* nodep) override {  // FindVisitor::
        UASSERT_OBJ(m_curSymp, nodep, "Class not under module/package/$unit");
        UINFO(8, "   " << nodep);
        VL_RESTORER_COPY(m_scope);
        VL_RESTORER(m_classOrPackagep);
        VL_RESTORER(m_modSymp);
        VL_RESTORER(m_curSymp);
        VL_RESTORER(m_param_അക്കം);
        VL_RESTORER(m_mod_Block_അക്കം);
        VL_RESTORER(m_mod_With_അക്കം);
        VL_RESTORER(m_mod_Arg_അക്കം);
        VL_RESTORER(m_explicitNew);
        {
            UINFO(4, "     Link Class: " << nodep);
            VSymEnt* സ്ഥിരം upperSymp = m_curSymp;
            m_scope = m_scope + "." + nodep->name();
            m_classOrPackagep = nodep;
            m_curSymp = m_modSymp
                = m_statep->തിരുകുക_Block(upperSymp, nodep->name(), nodep, m_classOrPackagep);
            m_statep->തിരുകുക_Map(m_curSymp, m_scope);
            UINFO(9, "New module scope " << m_curSymp);
            //
            m_param_അക്കം = 0;
            m_mod_Block_അക്കം = 0;
            m_mod_With_അക്കം = 0;
            m_mod_Arg_അക്കം = 0;
            m_explicitNew = അസത്യം;
            // m_modSymp/m_curSymp for non-packages set by AstCell above this module
            // Iterate
            iterateChildren(nodep);
            nodep->user4(സത്യം);
            // Implicit new needed?
            എങ്കിൽ (!m_explicitNew && m_statep->forPrimary()) makeImplicitNew(nodep);
        }
    }
    ശൂന്യം visit(AstClassOrPackageRef* nodep) override {  // FindVisitor::
        എങ്കിൽ (!nodep->classOrPackageNodep() && nodep->name() == "$unit") {
            nodep->classOrPackageNodep(v3Global.rootp()->dollarUnitPkgp());
        }
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstScope* nodep) override {  // FindVisitor::
        UASSERT_OBJ(m_statep->forScopeCreation(), nodep,
                    "Scopes should only exist right after V3Scope");
        // Detach on first encounter, will re add at the end.
        // Otherwise would be visited O(cells^2) time after V3Scope
        എങ്കിൽ (!nodep->is_മുകൾ()) {
            ++m_statScopeVisits;
            nodep->unlinkFrBack();
            m_scopes.തള്ളുക_back(nodep);
        }
    }
    ശൂന്യം visit(AstCell* nodep) override {  // FindVisitor::
        UINFO(5, "   CELL under " << m_scope << " is " << nodep);
        // Process XREFs/etc inside pins
        എങ്കിൽ (nodep->recursive() && m_inRecursion) തിരികെ;
        iterateChildren(nodep);
        // Recurse in, preserving state
        VL_RESTORER_COPY(m_scope);
        VL_RESTORER(m_modSymp);
        VL_RESTORER(m_curSymp);
        VL_RESTORER(m_param_അക്കം);
        VL_RESTORER(m_inRecursion);
        // Where do we add it?
        VSymEnt* aboveSymp = m_curSymp;
        സ്ഥിരം string origname = Ast_കണ്ണി::dedotName(nodep->name());
        string::size_type pos = origname.rfind('.');
        എങ്കിൽ (pos != string::npos) {
            // Flattened, find what CellInline it should live under
            സ്ഥിരം string scope = origname.substr(0, pos);
            string baddot;
            VSymEnt* okSymp;
            aboveSymp
                = m_statep->findDotted(nodep->fileline(), aboveSymp, scope, baddot, okSymp, അസത്യം);
            UASSERT_OBJ(aboveSymp, nodep,
                        "Can't find instance insertion point at "
                            << Ast_കണ്ണി::prettyNameQ(baddot) << " in: " << nodep->prettyNameQ());
        }
        {
            m_scope = m_scope + "." + nodep->name();
            m_curSymp = m_modSymp = m_statep->തിരുകുക_Cell(aboveSymp, m_modSymp, nodep, m_scope);
            m_inRecursion = nodep->recursive();
            // We don't report NotFoundModule, as may be a unused module in a generate
            എങ്കിൽ (nodep->modp()) iterate(nodep->modp());
        }
    }
    ശൂന്യം visit(AstCellInline* nodep) override {  // FindVisitor::
        UINFO(5, "   CELLINLINE under " << m_scope << " is " << nodep);
        VSymEnt* aboveSymp = m_curSymp;
        std::string ident = nodep->name();
        // If baz__DOT__foo__DOT__bar, we need to find baz__DOT__foo and add bar to it.
        സ്ഥിരം std::string::size_type pos = ident.rfind("__DOT__");
        എങ്കിൽ (pos != std::string::npos) {
            സ്ഥിരം std::string dotted = ident.substr(0, pos);
            ident = ident.substr(pos + std::strlen("__DOT__"));
            std::string baddot;
            VSymEnt* okSymp;
            aboveSymp = m_statep->findDotted(nodep->fileline(), aboveSymp, dotted, baddot, okSymp,
                                             അസത്യം);
            UASSERT_OBJ(aboveSymp, nodep,
                        "Can't find cellinline insertion point at "
                            << Ast_കണ്ണി::prettyNameQ(baddot) << " in: " << nodep->prettyNameQ());
        }
        m_statep->തിരുകുക_Inline(aboveSymp, m_modSymp, nodep, ident);
    }
    ശൂന്യം visit(AstDefParam* nodep) override {  // FindVisitor::
        nodep->user1p(m_curSymp);
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstRefDType* nodep) override {  // FindVisitor::
        nodep->user1p(m_curSymp);
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstGenBlock* nodep) override {  // FindVisitor::
        UINFO(5, "   " << nodep);
        എങ്കിൽ (nodep->name() == "" && nodep->unnamed()) {
            // Unnamed blocks are only important when they contain var
            // decls, so search for them. (Otherwise adding all the
            // unnamed#'s would just confuse tracing variables in
            // places such as tasks, where "task ...; begin ... end"
            // are common.
            വേണ്ടി (Ast_കണ്ണി* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
                എങ്കിൽ (VN_IS(itemp, Var) || VN_IS(itemp, Foreach)) {
                    std::string name;
                    സ്ഥിരം std::string stepStr = m_statep->forPrimary()
                                                    ? ""
                                                    : std::to_string(m_statep->step_സംഖ്യ()) + "_";
                    ചെയ്യുക {
                        ++m_mod_Block_അക്കം;
                        name = "unnamedblk" + stepStr + cvtToStr(m_mod_Block_അക്കം);
                        // Increment again if earlier pass of V3LinkDot claimed this name
                    } ആകുമ്പോൾ (m_curSymp->findIdFlat(name));
                    nodep->name(name);
                    നിർത്തുക;
                }
            }
        }
        എങ്കിൽ (nodep->name() == "") {
            iterateChildren(nodep);
        } അല്ലെങ്കിൽ {
            VL_RESTORER(m_curSymp);
            {
                m_curSymp
                    = m_statep->തിരുകുക_Block(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
                m_curSymp->fallbackp(VL_RESTORER_PREV(m_curSymp));
                // Iterate
                iterateChildren(nodep);
            }
        }
    }
    ശൂന്യം visit(Ast_കണ്ണി_Block* nodep) override {  // FindVisitor::
        UINFO(5, "   " << nodep);
        എങ്കിൽ (nodep->name() == "" && nodep->unnamed()) {
            // Unnamed blocks are only important when they contain var
            // decls, so search for them. (Otherwise adding all the
            // unnamed#'s would just confuse tracing variables in
            // places such as tasks, where "task ...; begin ... end"
            // are common.
            സ്ഥിരം auto containsDecl = [](സ്ഥിരം Ast_കണ്ണി* nodesp) -> തർക്കം {
                വേണ്ടി (സ്ഥിരം Ast_കണ്ണി* np = nodesp; np; np = np->nextp()) {
                    എങ്കിൽ (VN_IS(np, Var) || VN_IS(np, Foreach)) തിരികെ സത്യം;
                }
                തിരികെ അസത്യം;
            };
            തർക്കം needName = nodep->declsp() || containsDecl(nodep->stmtsp());
            എങ്കിൽ (സ്ഥിരം AstFork* സ്ഥിരം forkp = VN_CAST(nodep, Fork)) {
                needName = needName || containsDecl(forkp->forksp());
            }
            എങ്കിൽ (needName) {
                സ്ഥിരം std::string stepStr
                    = m_statep->forPrimary() ? "" : std::to_string(m_statep->step_സംഖ്യ()) + "_";
                std::string name;
                ചെയ്യുക {
                    ++m_mod_Block_അക്കം;
                    name = "unnamedblk" + stepStr + cvtToStr(m_mod_Block_അക്കം);
                    // Increment again if earlier pass of V3LinkDot claimed this name
                } ആകുമ്പോൾ (m_curSymp->findIdFlat(name));
                nodep->name(name);
            }
        }
        എങ്കിൽ (nodep->name() == "") {
            iterateChildren(nodep);
        } അല്ലെങ്കിൽ {
            VL_RESTORER(m_curSymp);
            {
                m_curSymp
                    = m_statep->തിരുകുക_Block(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
                m_curSymp->fallbackp(VL_RESTORER_PREV(m_curSymp));
                // Iterate
                iterateChildren(nodep);
            }
        }
    }
    // Handle out-of-block interface method definition (IEEE 1800-2023 25.7)
    // Move task body from module into the interface's prototype task,
    // rewriting port-prefixed references in the body.
    ശൂന്യം moveIfaceExportBody(Ast_കണ്ണി_FTask* nodep) {
        സ്ഥിരം string& portName = nodep->ifacePortName();
        UINFO(5, "   moveIfaceExportBody: port=" << portName << " task=" << nodep->name());
        // Look up the interface port variable in the module's symbol table
        VSymEnt* സ്ഥിരം portSymp = m_modSymp->findIdFallback(portName);
        എങ്കിൽ (!portSymp) {
            nodep->v3error("Interface port not found for out-of-block definition: '"
                           << portName << "' (IEEE 1800-2023 25.7)");
            nodep->unlinkFrBack();
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
            തിരികെ;
        }
        AstVar* സ്ഥിരം portp = VN_CAST(portSymp->nodep(), Var);
        എങ്കിൽ (!portp) {
            nodep->v3fatalSrc("Out-of-block definition port is not a variable: " << portName);
            തിരികെ;
        }
        // Get the interface name from the port's type
        // At this stage dtypep() may not be set yet; check childDTypep() too
        AstIfaceRefDType* ifaceRefDtp = nullptr;
        എങ്കിൽ (portp->dtypep()) { ifaceRefDtp = VN_CAST(portp->dtypep(), IfaceRefDType); }
        എങ്കിൽ (!ifaceRefDtp && portp->childDTypep()) {
            ifaceRefDtp = VN_CAST(portp->childDTypep(), IfaceRefDType);
        }
        എങ്കിൽ (!ifaceRefDtp) {
            nodep->v3error("Out-of-block definition port is not an interface port: '"
                           << portName << "' (IEEE 1800-2023 25.7)");
            nodep->unlinkFrBack();
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
            തിരികെ;
        }
        സ്ഥിരം string ifaceName = ifaceRefDtp->ifaceName();
        UINFO(5, "   moveIfaceExportBody: iface=" << ifaceName);
        // Find the interface module by name
        Ast_കണ്ണി_Module* സ്ഥിരം ifaceModp = m_statep->findModuleSym(ifaceName);
        എങ്കിൽ (!ifaceModp) {
            nodep->v3fatalSrc("Interface not found for out-of-block definition: " << ifaceName);
            തിരികെ;
        }
        // Find the prototype task in the interface
        Ast_കണ്ണി_FTask* protoTaskp = nullptr;
        വേണ്ടി (Ast_കണ്ണി* itemp = ifaceModp->stmtsp(); itemp; itemp = itemp->nextp()) {
            എങ്കിൽ (Ast_കണ്ണി_FTask* സ്ഥിരം ftaskp = VN_CAST(itemp, കണ്ണി_FTask)) {
                എങ്കിൽ (ftaskp->name() == nodep->name() && ftaskp->prototype()) {
                    protoTaskp = ftaskp;
                    നിർത്തുക;
                }
            }
        }
        എങ്കിൽ (!protoTaskp) {
            nodep->v3error("No matching export prototype found in interface "
                           << ifaceName << " for task " << nodep->prettyNameQ()
                           << " (IEEE 1800-2023 25.7)");
            nodep->unlinkFrBack();
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
            തിരികെ;
        }
        // Move body statements (non-port declarations) from the out-of-block task
        // to the prototype task. Rewrite port.X references to just X in the body.
        വേണ്ടി (Ast_കണ്ണി* stmtp = nodep->stmtsp(); stmtp;) {
            Ast_കണ്ണി* സ്ഥിരം nextp = stmtp->nextp();
            // Skip port declarations -- they already exist in the prototype
            എങ്കിൽ (സ്ഥിരം AstVar* സ്ഥിരം varp = VN_CAST(stmtp, Var)) {
                എങ്കിൽ (varp->isIO()) {
                    stmtp = nextp;
                    തുടരുക;
                }
            }
            // Clone this statement and rewrite port references
            Ast_കണ്ണി* സ്ഥിരം newStmtp = stmtp->clone_വൃക്ഷം(അസത്യം);
            rewriteIfacePortRefsSingle(newStmtp, portName);
            protoTaskp->addStmtsp(newStmtp);
            stmtp = nextp;
        }
        // Mark prototype as no longer prototype (it has a body now)
        protoTaskp->prototype(അസത്യം);
        // Delete the out-of-block task from the module
        nodep->unlinkFrBack();
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
    }

    // Rewrite AstDot nodes where lhsp is AstParseRef with the port name
    // to just the rhsp (stripping the port prefix).
    സ്ഥിതം ശൂന്യം rewriteIfacePortRefsSingle(Ast_കണ്ണി* nodep, സ്ഥിരം string& portName) {
        എങ്കിൽ (!nodep) തിരികെ;
        // Check if this is a non-colon Dot with lhs matching portName
        എങ്കിൽ (AstDot* സ്ഥിരം dotp = VN_CAST(nodep, Dot)) {
            എങ്കിൽ (!dotp->colon()) {
                സ്ഥിരം AstParseRef* സ്ഥിരം lhsRefp = VN_CAST(dotp->lhsp(), ParseRef);
                എങ്കിൽ (lhsRefp && lhsRefp->name() == portName) {
                    UINFO(5, "   rewriteIfacePortRefs: stripping port prefix from " << dotp);
                    Ast_കണ്ണി* സ്ഥിരം rhsp = dotp->rhsp()->unlinkFrBack();
                    dotp->replaceWith(rhsp);
                    VL_DO_DANGLING(dotp->delete_വൃക്ഷം(), dotp);
                    rewriteIfacePortRefsSingle(rhsp, portName);
                    തിരികെ;
                }
            }
        }
        // Recurse into all children
        വേണ്ടി (Ast_കണ്ണി* childp : {nodep->op1p(), nodep->op2p(), nodep->op3p(), nodep->op4p()}) {
            ആകുമ്പോൾ (childp) {
                Ast_കണ്ണി* സ്ഥിരം nextp = childp->nextp();
                rewriteIfacePortRefsSingle(childp, portName);
                childp = nextp;
            }
        }
    }

    ശൂന്യം visit(Ast_കണ്ണി_FTask* nodep) override {  // FindVisitor::
        // NodeTask: Remember its name for later resolution
        UINFO(5, "   " << nodep);
        UASSERT_OBJ(m_curSymp && m_modSymp, nodep, "Function/Task not under module?");
        എങ്കിൽ (nodep->name() == "new") m_explicitNew = സത്യം;
        // Handle out-of-block interface method definition (IEEE 25.8)
        എങ്കിൽ (!nodep->ifacePortName().empty() && m_statep->forPrimary()) {
            moveIfaceExportBody(nodep);
            തിരികെ;  // Task has been deleted
        }
        // Remember the existing symbol table scope
        VL_RESTORER(m_classOrPackagep);
        VL_RESTORER(m_curSymp);
        VSymEnt* upSymp = m_curSymp;

        എങ്കിൽ (VN_IS(m_curSymp->nodep(), Class)) {
            എങ്കിൽ (VN_AS(m_curSymp->nodep(), Class)->isInterfaceClass() && !nodep->pureVirtual()
                && !nodep->isConstructor()) {
                nodep->v3error("Interface class functions must be pure virtual"
                               << " (IEEE 1800-2023 8.26): " << nodep->prettyNameQ());
            }
            എങ്കിൽ (m_statep->forPrimary()
                && (nodep->name() == "randomize" || nodep->name() == "srandom")) {
                nodep->v3error(nodep->prettyNameQ()
                               << " is a predefined class method; redefinition not allowed"
                                  " (IEEE 1800-2023 18.6.3)");
            }
        }
        // Change to appropriate package if extern declaration (vs definition)
        എങ്കിൽ (nodep->classOrPackagep()) {
            // class-in-class
            AstDot* സ്ഥിരം dotp = VN_CAST(nodep->classOrPackagep(), Dot);
            AstClassOrPackageRef* സ്ഥിരം cpackagerefp
                = VN_CAST(nodep->classOrPackagep(), ClassOrPackageRef);
            എങ്കിൽ (dotp) {
                AstClassOrPackageRef* സ്ഥിരം lhsp = VN_AS(dotp->lhsp(), ClassOrPackageRef);
                m_statep->resolveClassOrPackage(m_curSymp, lhsp, സത്യം, അസത്യം,
                                                "External definition :: reference");
                AstClass* സ്ഥിരം lhsclassp = VN_CAST(lhsp->classOrPackageSkipp(), Class);
                എങ്കിൽ (!lhsclassp) {
                    nodep->v3error("Extern declaration's scope is not a defined class");
                } അല്ലെങ്കിൽ {
                    m_curSymp = m_statep->get_കണ്ണി_Sym(lhsclassp);
                    upSymp = m_curSymp;
                    AstClassOrPackageRef* സ്ഥിരം rhsp = VN_AS(dotp->rhsp(), ClassOrPackageRef);
                    m_statep->resolveClassOrPackage(m_curSymp, rhsp, സത്യം, അസത്യം,
                                                    "External definition :: reference");
                    AstClass* സ്ഥിരം rhsclassp = VN_CAST(rhsp->classOrPackageSkipp(), Class);
                    എങ്കിൽ (!rhsclassp) {
                        nodep->v3error("Extern declaration's scope is not a defined class");
                    } അല്ലെങ്കിൽ {
                        m_curSymp = m_statep->get_കണ്ണി_Sym(rhsclassp);
                        upSymp = m_curSymp;
                        moveExternFuncDecl(nodep, rhsclassp);
                    }
                }
            } അല്ലെങ്കിൽ എങ്കിൽ (cpackagerefp) {
                എങ്കിൽ (!cpackagerefp->classOrPackageSkipp()) {
                    m_statep->resolveClassOrPackage(m_curSymp, cpackagerefp, സത്യം, അസത്യം,
                                                    "External definition :: reference");
                }
                AstClass* സ്ഥിരം classp = VN_CAST(cpackagerefp->classOrPackageSkipp(), Class);
                എങ്കിൽ (!classp) {
                    nodep->v3error("Extern declaration's scope is not a defined class");
                } അല്ലെങ്കിൽ {
                    m_curSymp = m_statep->get_കണ്ണി_Sym(classp);
                    upSymp = m_curSymp;
                    moveExternFuncDecl(nodep, classp);
                }
            } അല്ലെങ്കിൽ {
                v3fatalSrc("Unhandled extern function definition package");
            }
        }
        // Set the class as package for iteration
        എങ്കിൽ (VN_IS(m_curSymp->nodep(), Class)) {
            m_classOrPackagep = VN_AS(m_curSymp->nodep(), Class);
        }
        // Create symbol table for the task's vars
        സ്ഥിരം string name = (nodep->isExternProto() ? "extern "s : ""s) + nodep->name();
        m_curSymp = m_statep->തിരുകുക_Block(m_curSymp, name, nodep, m_classOrPackagep);
        m_curSymp->fallbackp(upSymp);
        // Convert the func's range to the output variable
        // This should probably be done in the Parser instead, as then we could
        // just attach normal signal attributes to it.
        എങ്കിൽ (!nodep->isExternProto() && nodep->fvarp() && !VN_IS(nodep->fvarp(), Var)) {
            Ast_കണ്ണി_DType* dtypep = VN_CAST(nodep->fvarp(), കണ്ണി_DType);
            // If unspecified, function returns one bit; however when we
            // support NEW() it could also return the class reference.
            എങ്കിൽ (dtypep) {
                dtypep->unlinkFrBack();
            } അല്ലെങ്കിൽ {
                dtypep = പുതിയ AstBasicDType{nodep->fileline(), VBasicDTypeKwd::LOGIC};
            }
            AstVar* സ്ഥിരം newvarp
                = പുതിയ AstVar{nodep->fileline(), VVarType::VAR, nodep->name(), VFlagChildDType{},
                             dtypep};  // Not dtype resolved yet
            newvarp->direction(VDirection::OUTPUT);
            newvarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            newvarp->funcReturn(സത്യം);
            newvarp->trace(അസത്യം);  // Not user visible
            nodep->fvarp(newvarp);
            // Explicit insert required, as the var name shadows the upper level's task name
            m_statep->തിരുകുക_Sym(m_curSymp, newvarp->name(), newvarp, nullptr /*classOrPackagep*/);
        }
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstClocking* nodep) override {  // FindVisitor::
        VL_RESTORER(m_clockingp);
        m_clockingp = nodep;
        iterate(nodep->sentreep());
        iterateAndNextNull(nodep->itemsp());
        // If the block has no name, one cannot reference the clockvars
        VSymEnt* itSymp = nullptr;
        എങ്കിൽ (nodep->isGlobal()  //
            && m_statep->forPrimary()) {  // else flattening may see two globals
            m_statep->checkDuplicate(m_curSymp, nodep, "__024global_clock");
            itSymp
                = m_statep->തിരുകുക_Block(m_curSymp, "__024global_clock", nodep, m_classOrPackagep);
            itSymp->fallbackp(nullptr);
        }
        എങ്കിൽ (!nodep->name().empty()) {
            itSymp = m_statep->തിരുകുക_Block(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
            itSymp->fallbackp(nullptr);
        }
        എങ്കിൽ (itSymp) {
            VL_RESTORER(m_curSymp);
            m_curSymp = itSymp;
            iterateAndNextNull(nodep->itemsp());
        }
    }
    ശൂന്യം visit(AstClockingItem* nodep) override {  // FindVisitor::
        എങ്കിൽ (nodep->varp()) {
            എങ്കിൽ (m_curSymp->nodep() == m_clockingp) iterate(nodep->varp());
            തിരികെ;
        }
        std::string varname;
        Ast_കണ്ണി_DType* dtypep;
        എങ്കിൽ (AstAssign* സ്ഥിരം assignp = nodep->assignp()) {
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rhsp = assignp->rhsp()->unlinkFrBack();
            dtypep = പുതിയ AstRefDType{nodep->fileline(), AstRefDType::Flag_Type_Of_വ്യഞ്ജകം{},
                                     rhsp->clone_വൃക്ഷം(അസത്യം)};
            nodep->exprp(rhsp);
            varname = assignp->lhsp()->name();
            VL_DO_DANGLING(assignp->unlinkFrBack()->delete_വൃക്ഷം(), assignp);
        } അല്ലെങ്കിൽ {
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം refp = nodep->exprp();
            സ്ഥിരം VSymEnt* foundp = m_curSymp->findIdFallback(refp->name());
            എങ്കിൽ (!foundp || !foundp->nodep()) {
                refp->v3error("Corresponding variable " << refp->prettyNameQ()
                                                        << " does not exist");
                VL_DO_DANGLING(nodep->unlinkFrBack()->delete_വൃക്ഷം(), nodep);
                തിരികെ;
            }
            varname = refp->name();
            dtypep = VN_AS(foundp->nodep(), Var)->childDTypep()->clone_വൃക്ഷം(അസത്യം);
        }
        AstVar* സ്ഥിരം newvarp = പുതിയ AstVar{nodep->fileline(), VVarType::MODULETEMP, varname,
                                           VFlagChildDType{}, dtypep};
        newvarp->lifetime(VLifetime::STATIC_EXPLICIT);
        nodep->varp(newvarp);
        iterate(nodep->exprp());
    }
    ശൂന്യം visit(AstConstraint* nodep) override {  // FindVisitor::
        VL_RESTORER(m_curSymp);
        // Change to appropriate package if extern declaration (vs definition)
        VSymEnt* upSymp = m_curSymp;
        എങ്കിൽ (nodep->classOrPackagep()) {
            AstClassOrPackageRef* സ്ഥിരം cpackagerefp
                = VN_CAST(nodep->classOrPackagep(), ClassOrPackageRef);
            എങ്കിൽ (!cpackagerefp) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: extern constraint definition with class-in-class");
            } അല്ലെങ്കിൽ {
                എങ്കിൽ (!cpackagerefp->classOrPackageSkipp()) {
                    m_statep->resolveClassOrPackage(m_curSymp, cpackagerefp, സത്യം, അസത്യം,
                                                    "External definition :: reference");
                }
                AstClass* സ്ഥിരം classp = VN_CAST(cpackagerefp->classOrPackageSkipp(), Class);
                എങ്കിൽ (!classp) {
                    nodep->v3error("Extern declaration's scope is not a defined class");
                } അല്ലെങ്കിൽ {
                    m_curSymp = m_statep->get_കണ്ണി_Sym(classp);
                    upSymp = m_curSymp;
                    // Move it to proper spot under the target class
                    nodep->unlinkFrBack();
                    classp->addStmtsp(nodep);
                    nodep->classOrPackagep()->unlinkFrBack()->delete_വൃക്ഷം();
                }
            }
        }
        // Set the class as package for iteration
        സ്ഥിരം string name = (nodep->isExternProto() ? "extern "s : ""s) + nodep->name();
        m_curSymp = m_statep->തിരുകുക_Block(upSymp, name, nodep, m_classOrPackagep);
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstVar* nodep) override {  // FindVisitor::
        // Var: Remember its name for later resolution
        UASSERT_OBJ(m_curSymp && m_modSymp, nodep, "Var not under module?");
        iterateChildren(nodep);
        എങ്കിൽ (VN_IS(m_curSymp->nodep(), Class)
            && VN_AS(m_curSymp->nodep(), Class)->isInterfaceClass() && !nodep->isParam()) {
            nodep->v3error("Interface class cannot contain non-parameter members"
                           << " (IEEE 1800-2023 8.26): " << nodep->prettyNameQ());
        }
        എങ്കിൽ (!m_statep->forScopeCreation()) {
            // Find under either a task or the module's vars
            സ്ഥിരം VSymEnt* foundp = m_curSymp->findIdFallback(nodep->name());
            എങ്കിൽ (!foundp && m_modSymp && nodep->name() == m_modSymp->nodep()->name()) {
                foundp = m_modSymp;  // Conflicts with modname?
            }
            AstVar* സ്ഥിരം findvarp = foundp ? VN_CAST(foundp->nodep(), Var) : nullptr;
            // clocking items can have duplicate names (inout)
            എങ്കിൽ (findvarp && VN_IS(findvarp->backp(), ClockingItem)
                && VN_IS(nodep->backp(), ClockingItem)) {
                AstClockingItem* സ്ഥിരം itemp = VN_AS(nodep->backp(), ClockingItem);
                AstClockingItem* സ്ഥിരം finditemp = VN_AS(findvarp->backp(), ClockingItem);
                UINFO(4, "ClockCompl: " << itemp << " ;; " << finditemp);
                UINFO(4, "ClockCompV: " << nodep << " ;; " << findvarp);
                എങ്കിൽ (*itemp->exprp()->fileline() == *finditemp->exprp()->fileline()) {
                    UASSERT_OBJ(finditemp->direction() == VDirection::നിവേശം
                                    && itemp->direction() == VDirection::OUTPUT,
                                itemp, "Input after output?");
                    // pretend nothing found and rename
                    foundp = nullptr;
                    nodep->name("__Voutput_" + nodep->name());
                    finditemp->outputp(itemp);
                }
            }
            തർക്കം ins = അസത്യം;
            എങ്കിൽ (!foundp) {
                ins = സത്യം;
            } അല്ലെങ്കിൽ എങ്കിൽ (!findvarp && m_curSymp->findIdFlat(nodep->name()) && !foundp->imported()) {
                nodep->v3error("Unsupported in C: Variable has same name as "
                               << LinkDotState::കണ്ണി_Text_Type(foundp->nodep()) << ": "
                               << nodep->prettyNameQ());
            } അല്ലെങ്കിൽ എങ്കിൽ (findvarp != nodep) {
                UINFO(4, "DupVar: " << nodep << " ;; " << foundp->nodep());
                UINFO(4, "    found  cur=se" << cvtToHex(m_curSymp) << " ;; parent=se"
                                             << cvtToHex(foundp->parentp()));
                എങ്കിൽ (foundp->parentp() == m_curSymp  // Only when on same level
                    && !foundp->imported()) {  // and not from package
                    എങ്കിൽ (VN_IS(m_curSymp->nodep(), Clocking)) {
                        nodep->v3error("Multiple clockvars with the same name not allowed");
                        തിരികെ;
                    }
                    സ്ഥിരം തർക്കം nansiBad
                        = (((findvarp->isDeclTyped() || findvarp->isNet())
                            && (nodep->isDeclTyped() || nodep->isNet()))
                           || (findvarp->isIO() && nodep->isIO())  // e.g. !(output && output)
                           || findvarp->gotNansiType());  // e.g. int x && int x
                    സ്ഥിരം തർക്കം ansiBad
                        = findvarp->isAnsi() || nodep->isAnsi();  // dup illegal with ANSI
                    എങ്കിൽ (ansiBad || nansiBad) {
                        തർക്കം ansiWarn = ansiBad && !nansiBad;
                        എങ്കിൽ (ansiWarn) {
                            സ്ഥിതം പൂർണ്ണസംഖ്യ s_didAnsiWarn = അസത്യം;
                            എങ്കിൽ (s_didAnsiWarn++) ansiWarn = അസത്യം;
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
                    } അല്ലെങ്കിൽ {
                        findvarp->combineType(nodep);
                        findvarp->fileline()->modifyStateInherit(nodep->fileline());
                        findvarp->gotNansiType(സത്യം);
                        UASSERT_OBJ(nodep->subDTypep(), nodep, "Var has no type");
                        എങ്കിൽ (nodep->subDTypep()->numeric().isSigned()
                            && !findvarp->subDTypep()->numeric().isSigned()) {
                            findvarp->subDTypep()->numeric(VSigning{സത്യം});
                        }
                        Ast_കണ്ണി_DType* സ്ഥിരം varDtp = findvarp->subDTypep();
                        Ast_കണ്ണി_DType* സ്ഥിരം otherDtp = nodep->subDTypep();
                        എങ്കിൽ (varDtp
                            && (VN_IS(varDtp, BasicDType)
                                && VN_AS(varDtp, BasicDType)->implicit())) {
                            // Then have "input foo" and "real foo" so the
                            // dtype comes from the other side.
                            VL_DO_DANGLING(varDtp->unlinkFrBack()->delete_വൃക്ഷം(), varDtp);
                            findvarp->childDTypep(otherDtp->unlinkFrBack());
                        } അല്ലെങ്കിൽ എങ്കിൽ (!m_statep->forScopeCreation() && otherDtp && varDtp
                                   && !(VN_IS(otherDtp, BasicDType)
                                        && VN_AS(otherDtp, BasicDType)->implicit())) {
                            // otherDtp and varDtp both non-nullptr and neither are implicit
                            // Can't compare dtypes now as might contain parameters,
                            // defer to V3Width
                            AstAttrOf* സ്ഥിരം newp
                                = പുതിയ AstAttrOf{otherDtp->fileline(), VAttrType::VAR_PORT_DTYPE,
                                                otherDtp->unlinkFrBack()};
                            findvarp->addAttrsp(newp);
                        }
                    }
                    VL_DO_DANGLING(nodep->unlinkFrBack()->delete_വൃക്ഷം(), nodep);
                } അല്ലെങ്കിൽ {
                    // User can disable the message at either point
                    എങ്കിൽ (!(m_ftaskp && m_ftaskp->dpiImport())
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
                    ins = സത്യം;
                }
            }
            എങ്കിൽ (ins) {
                എങ്കിൽ (m_statep->forPrimary() && nodep->isGParam()
                    && VN_IS(m_modSymp->nodep(), Module)
                    && (m_statep->വേര്_Entp()->nodep() == m_modSymp->parentp()->nodep())) {
                    // This is the toplevel module. Check for command line overwrites of parameters
                    // We first search if the parameter is overwritten and then replace it with a
                    // new value.
                    എങ്കിൽ (v3Global.opt.hasParameter(nodep->name())) {
                        സ്ഥിരം string svalue = v3Global.opt.parameter(nodep->name());
                        എങ്കിൽ (AstConst* സ്ഥിരം valuep
                            = AstConst::parseParamLiteral(nodep->fileline(), svalue)) {
                            UINFO(9, "       replace parameter " << nodep);
                            UINFO(9, "       with " << valuep);
                            എങ്കിൽ (nodep->valuep()) തള്ളുക_Deletep(nodep->valuep()->unlinkFrBack());
                            nodep->valuep(valuep);
                            // Do not warn ENUMVALUE when overriding an enum param from the cli.
                            nodep->fileline()->warnOff(V3ErrorCode::ENUMVALUE, സത്യം);
                        }
                    }
                }
                m_statep->തിരുകുക_Sym(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
                എങ്കിൽ (m_statep->forPrimary() && nodep->isGParam()) {
                    ++m_param_അക്കം;
                    VSymEnt* സ്ഥിരം symp
                        = m_statep->തിരുകുക_Sym(m_curSymp, "__paramNumber" + cvtToStr(m_param_അക്കം),
                                              nodep, m_classOrPackagep);
                    symp->exported(അസത്യം);
                }
            }
        }
    }
    ശൂന്യം visit(AstTypedef* nodep) override {  // FindVisitor::
        UASSERT_OBJ(m_curSymp, nodep, "Typedef not under module/package/$unit");
        എങ്കിൽ (VN_IS(m_classOrPackagep, Class)) nodep->isUnderClass(സത്യം);
        iterateChildren(nodep);
        m_statep->തിരുകുക_Sym(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
    }
    ശൂന്യം visit(AstTypedefFwd* nodep) override {  // FindVisitor::
        UASSERT_OBJ(m_curSymp, nodep, "Typedef not under module/package/$unit");
        iterateChildren(nodep);
        // No need to insert, only the real typedef matters, but need to track for errors
        nodep->user1p(m_curSymp);
    }
    ശൂന്യം visit(Ast_കണ്ണി_UOr_Struct_DType* nodep) override {  // FindVisitor::
        UASSERT_OBJ(m_curSymp, nodep, "Struct/union dtype not under module/package/$unit");
        VL_RESTORER(m_curSymp);
        m_curSymp = m_statep->തിരുകുക_Block(m_curSymp, "__Vdtype" + cvtToStr(nodep->unique_അക്കം()),
                                          nodep, m_classOrPackagep);
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstMemberDType* nodep) override {  // FindVisitor::
        iterateChildren(nodep);
        m_statep->തിരുകുക_Sym(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
    }
    ശൂന്യം visit(AstParamTypeDType* nodep) override {  // FindVisitor::
        UASSERT_OBJ(m_curSymp, nodep, "Parameter type not under module/package/$unit");

        // Replace missing param types with provided hierarchical type params.
        എങ്കിൽ (!m_hierParamsName.empty()) {
            എങ്കിൽ (സ്ഥിരം VSymEnt* സ്ഥിരം typedefEntp = m_curSymp->findIdFallback(m_hierParamsName)) {
                സ്ഥിരം AstModule* modp = VN_CAST(typedefEntp->nodep(), Module);

                വേണ്ടി (സ്ഥിരം Ast_കണ്ണി* stmtp = modp ? modp->stmtsp() : nullptr; stmtp;
                     stmtp = stmtp->nextp()) {
                    സ്ഥിരം AstTypedef* tdefp = VN_CAST(stmtp, Typedef);

                    എങ്കിൽ (tdefp && tdefp->name() == nodep->name() && m_statep->forPrimary()) {
                        UINFO(8, "Replacing type of" << nodep << "  -->with " << tdefp);
                        Ast_കണ്ണി_DType* സ്ഥിരം newDtp = tdefp->childDTypep();
                        Ast_കണ്ണി_DType* സ്ഥിരം oldDtp = nodep->childDTypep();

                        oldDtp->replaceWith(newDtp->clone_വൃക്ഷം(അസത്യം));
                        VL_DO_DANGLING(oldDtp->delete_വൃക്ഷം(), oldDtp);
                    }
                }
            }
        }

        iterateChildren(nodep);
        m_statep->തിരുകുക_Sym(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
        എങ്കിൽ (m_statep->forPrimary() && nodep->isGParam()) {
            ++m_param_അക്കം;
            VSymEnt* സ്ഥിരം symp = m_statep->തിരുകുക_Sym(
                m_curSymp, "__paramNumber" + cvtToStr(m_param_അക്കം), nodep, m_classOrPackagep);
            symp->exported(അസത്യം);
        }
    }
    ശൂന്യം visit(AstCFunc* nodep) override {  // FindVisitor::
        // For dotted resolution, ignore all AstVars under functions, otherwise shouldn't exist
        UASSERT_OBJ(!m_statep->forScopeCreation(), nodep, "No CFuncs expected in tree yet");
    }
    ശൂന്യം visit(AstEnumItem* nodep) override {  // FindVisitor::
        // EnumItem: Remember its name for later resolution
        iterateChildren(nodep);
        // Find under either a task or the module's vars
        സ്ഥിരം VSymEnt* foundp = m_curSymp->findIdFallback(nodep->name());
        എങ്കിൽ (!foundp && m_modSymp && nodep->name() == m_modSymp->nodep()->name()) {
            foundp = m_modSymp;  // Conflicts with modname?
        }
        AstEnumItem* സ്ഥിരം findvarp = foundp ? VN_CAST(foundp->nodep(), EnumItem) : nullptr;
        തർക്കം ins = അസത്യം;
        എങ്കിൽ (!foundp) {
            ins = സത്യം;
        } അല്ലെങ്കിൽ എങ്കിൽ (findvarp != nodep) {
            UINFO(4, "DupVar: " << nodep << " ;; " << foundp);
            എങ്കിൽ (foundp->parentp() == m_curSymp  // Only when on same level
                && !foundp->imported()) {  // and not from package
                nodep->v3error("Duplicate declaration of enum value: "
                               << nodep->prettyName() << '\n'
                               << nodep->warnContextPrimary() << '\n'
                               << foundp->nodep()->warnOther()
                               << "... Location of original declaration\n"
                               << foundp->nodep()->warnContextSecondary());
            } അല്ലെങ്കിൽ {
                // User can disable the message at either point
                എങ്കിൽ (!nodep->fileline()->warnIsOff(V3ErrorCode::VARHIDDEN)
                    && !foundp->nodep()->fileline()->warnIsOff(V3ErrorCode::VARHIDDEN)) {
                    nodep->v3warn(VARHIDDEN,
                                  "Declaration of enum value hides declaration in upper scope: "
                                      << nodep->prettyName() << '\n'
                                      << nodep->warnContextPrimary() << '\n'
                                      << foundp->nodep()->warnOther()
                                      << "... Location of original declaration\n"
                                      << foundp->nodep()->warnContextSecondary());
                }
                ins = സത്യം;
            }
        }
        എങ്കിൽ (ins) m_statep->തിരുകുക_Sym(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
    }
    ശൂന്യം visit(AstPackageImport* nodep) override {  // FindVisitor::
        UINFO(4, "  Link: " << nodep);
        എങ്കിൽ (!nodep->packagep()) തിരികെ;  // Errored in V3LinkCells
        VSymEnt* സ്ഥിരം srcp = m_statep->get_കണ്ണി_Sym(nodep->packagep());
        എങ്കിൽ (nodep->name() == "*") {
            എങ്കിൽ (nodep->packagep() != v3Global.rootp()->stdPackagep()) {
                എങ്കിൽ (m_curSymp == m_statep->dunitEntp()) {
                    nodep->v3warn(IMPORTSTAR,
                                  "'import::*' in $unit scope may pollute global namespace");
                }
            }
        } അല്ലെങ്കിൽ എങ്കിൽ (!nodep->resolvedClassp()) {
            VSymEnt* സ്ഥിരം impp = srcp->findIdFlat(nodep->name());
            എങ്കിൽ (!impp) {
                nodep->v3error("Import object not found: " << nodep->prettyPkgNameQ());
            } അല്ലെങ്കിൽ എങ്കിൽ (nodep->resolvedClassp()) {
                UASSERT_OBJ(nodep->resolvedClassp()->classOrPackageSkipp() == impp->nodep(), nodep,
                            "Import should resolve to the same node");
            } അല്ലെങ്കിൽ എങ്കിൽ (AstClass* സ്ഥിരം classp = VN_CAST(impp->nodep(), Class)) {
                nodep->resolvedClassp(
                    പുതിയ AstClassOrPackageRef{nodep->fileline(), classp->name(), classp, nullptr});
            }
        }
        m_curSymp->importFromPackage(m_statep->symsp(), srcp, nodep->name());
        UINFO(9, "    Link Done: " << nodep);
        // No longer needed, but can't delete until any multi-instantiated modules are expanded
    }
    ശൂന്യം visit(AstPackageExport* nodep) override {  // FindVisitor::
        UINFO(9, "  Link: " << nodep);
        എങ്കിൽ (!nodep->packagep()) തിരികെ;  // Errored in V3LinkCells
        VSymEnt* സ്ഥിരം srcp = m_statep->get_കണ്ണി_Sym(nodep->packagep());
        എങ്കിൽ (nodep->name() != "*") {
            VSymEnt* സ്ഥിരം impp = srcp->findIdFlat(nodep->name());
            എങ്കിൽ (!impp) {
                nodep->v3error("Export object not found: '" << nodep->packagep()->prettyName()
                                                            << "::" << nodep->prettyName() << "'");
            }
        }
        m_curSymp->exportFromPackage(m_statep->symsp(), srcp, nodep->name());
        UINFO(9, "    Link Done: " << nodep);
        // No longer needed, but can't delete until any multi-instantiated modules are expanded
    }
    ശൂന്യം visit(AstPackageExportStarStar* nodep) override {  // FindVisitor::
        UINFO(4, "  Link: " << nodep);
        m_curSymp->exportStarStar(m_statep->symsp());
        // No longer needed, but can't delete until any multi-instantiated modules are expanded
    }

    ശൂന്യം visit(Ast_കണ്ണി_Foreach* nodep) override {  // FindVisitor::
        // Symbol table needs nodep->name() as the index variable's name
        VL_RESTORER(m_curSymp);
        {
            ++m_mod_With_അക്കം;
            m_curSymp = m_statep->തിരുകുക_Block(m_curSymp, "__Vforeach" + cvtToStr(m_mod_With_അക്കം),
                                              nodep, m_classOrPackagep);
            m_curSymp->fallbackp(VL_RESTORER_PREV(m_curSymp));
            വേണ്ടി (Ast_കണ്ണി *nextp, *argp = nodep->headerp()->elementsp(); argp; argp = nextp) {
                nextp = argp->nextp();
                AstVar* argrefp = nullptr;
                എങ്കിൽ (AstParseRef* സ്ഥിരം parserefp = VN_CAST(argp, ParseRef)) {
                    // IEEE 1800-2023 12.7.3: foreach loop variable type shall be int (2-state)
                    // This might get changed in V3Width when types resolve (e.g., for assoc
                    // arrays)
                    argrefp = പുതിയ AstVar{parserefp->fileline(), VVarType::BLOCKTEMP,
                                         parserefp->name(), argp->findIntDType()};
                    argrefp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                    parserefp->replaceWith(argrefp);
                    VL_DO_DANGLING2(parserefp->delete_വൃക്ഷം(), parserefp, argp);
                    // Insert argref's name into symbol table
                    m_statep->തിരുകുക_Sym(m_curSymp, argrefp->name(), argrefp, nullptr);
                } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം auto largrefp = VN_CAST(argp, Var)) {
                    argrefp = largrefp;
                    // Insert argref's name into symbol table
                    m_statep->തിരുകുക_Sym(m_curSymp, argrefp->name(), argrefp, nullptr);
                } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(argp, Empty)) {
                } അല്ലെങ്കിൽ {
                    argp->v3error("'foreach' loop variable expects simple variable name");
                }
            }
            iterateChildren(nodep);
        }
    }

    ശൂന്യം visit(AstRandSequence* nodep) override {
        VL_RESTORER(m_curSymp);
        m_curSymp = m_statep->തിരുകുക_Block(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstRSProd* nodep) override {
        VL_RESTORER(m_curSymp);
        m_curSymp = m_statep->തിരുകുക_Block(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
        എങ്കിൽ (nodep->fvarp())
            nodep->fvarp()->v3warn(E_UNSUPPORTED,
                                   "Unsupported: randsequence production function variable");
        // Mark formal ports as port-checked so the primary resolve pass does not
        // flag them with "does not appear in port list" -- V3RandSequence will
        // later move them onto the generated task as real input ports.
        വേണ്ടി (Ast_കണ്ണി* itp = nodep->portsp(); itp; itp = itp->nextp()) {
            VN_AS(itp, Var)->user4(സത്യം);
        }
        iterateChildren(nodep);
    }

    ശൂന്യം visit(AstWithParse* nodep) override {  // FindVisitor::
        // Change WITHPARSE(FUNCREF, equation) to FUNCREF(WITH(equation))
        Ast_കണ്ണി_FTask_Ref* funcrefp = VN_CAST(nodep->funcrefp(), കണ്ണി_FTask_Ref);
        എങ്കിൽ (സ്ഥിരം AstDot* സ്ഥിരം dotp = VN_CAST(nodep->funcrefp(), Dot))
            funcrefp = VN_CAST(dotp->rhsp(), കണ്ണി_FTask_Ref);
        UASSERT_OBJ(funcrefp, nodep, "'with' only can operate on a function/task");
        string name = funcrefp->name() == "randomize" ? "__Vrandwith_obj" : "item";
        File_വരി* argFl = nodep->fileline();
        AstArg* സ്ഥിരം argsp = funcrefp->argsp();
        എങ്കിൽ (argsp) {
            argsp->unlinkFrBackWithNext();
            എങ്കിൽ (funcrefp->name() != "randomize") {
                എങ്കിൽ (സ്ഥിരം auto parserefp = VN_CAST(argsp->exprp(), ParseRef)) {
                    name = parserefp->name();
                    argFl = parserefp->fileline();
                } അല്ലെങ്കിൽ {
                    argsp->v3error("'with' function expects simple variable name");
                }
                എങ്കിൽ (argsp->nextp()) {
                    argsp->nextp()->v3error("'with' function expects only up to one argument");
                }
                VL_DO_DANGLING(argsp->delete_വൃക്ഷം(), argsp);
            }
        }
        // Type depends on the method used, let V3Width figure it out later
        // 'with (...) { ... }' constraint_block form is randomize-only
        // (IEEE 1800-2023 7.12 vs. 18.7); array methods take 'with (expr)'.
        എങ്കിൽ (nodep->restricted() && funcrefp->name() != "randomize") {
            nodep->v3error("'with (...) { ... }' constraint block is only valid for"
                           " randomize() (IEEE 1800-2023 18.7)");
            funcrefp->addArgsp(argsp);
            nodep->replaceWith(nodep->funcrefp()->unlinkFrBack());
            VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
            തിരികെ;
        }
        സ്ഥിരം തർക്കം restrictedRandomize = nodep->restricted();
        എങ്കിൽ (nodep->exprsp() || nodep->constraintsp()
            || restrictedRandomize) {  // Else empty expression and pretend no "with"
            AstLambdaArgRef* സ്ഥിരം indexArgRefp
                = പുതിയ AstLambdaArgRef{argFl, name + "__DOT__index", സത്യം};
            AstLambdaArgRef* സ്ഥിരം മൂല്യം_Arg_Refp = പുതിയ AstLambdaArgRef{argFl, name, അസത്യം};
            AstWith* സ്ഥിരം newp
                = പുതിയ AstWith{nodep->fileline(), indexArgRefp, മൂല്യം_Arg_Refp, nullptr};
            // Harvest the identifier_list into AstWith; grammar guarantees AstParseRef.
            എങ്കിൽ (restrictedRandomize) {
                newp->restricted(സത്യം);
                ആകുമ്പോൾ (Ast_കണ്ണി* സ്ഥിരം itemp = nodep->exprsp()) {
                    newp->addRestrictedName(VN_AS(itemp, ParseRef)->name());
                    itemp->unlinkFrBack();
                    VL_DO_DANGLING(തള്ളുക_Deletep(itemp), itemp);
                }
            }
            Ast_കണ്ണി* വ്യഞ്ജകം_Or_Constraintsp = nullptr;
            എങ്കിൽ (nodep->exprsp()) വ്യഞ്ജകം_Or_Constraintsp = nodep->exprsp()->unlinkFrBackWithNext();
            എങ്കിൽ (nodep->constraintsp())
                വ്യഞ്ജകം_Or_Constraintsp = Ast_കണ്ണി::addNext(
                    വ്യഞ്ജകം_Or_Constraintsp, nodep->constraintsp()->unlinkFrBackWithNext());
            newp->addExprp(വ്യഞ്ജകം_Or_Constraintsp);  // addExprp() tolerates nullptr
            funcrefp->withp(newp);
        }
        funcrefp->addArgsp(argsp);
        nodep->replaceWith(nodep->funcrefp()->unlinkFrBack());
        VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
    }
    ശൂന്യം visit(AstWith* nodep) override {  // FindVisitor::
        // Symbol table needs nodep->name() as the index variable's name
        // Iteration will pickup the AstVar we made under AstWith
        VL_RESTORER(m_curSymp);
        ++m_mod_With_അക്കം;
        m_curSymp = m_statep->തിരുകുക_Block(m_curSymp, "__Vwith" + cvtToStr(m_mod_With_അക്കം), nodep,
                                          m_classOrPackagep);
        m_curSymp->fallbackp(VL_RESTORER_PREV(m_curSymp));
        UASSERT_OBJ(nodep->indexArgRefp(), nodep, "Missing lambda argref");
        UASSERT_OBJ(nodep->മൂല്യം_Arg_Refp(), nodep, "Missing lambda argref");
        // Insert argref's name into symbol table
        m_statep->തിരുകുക_Sym(m_curSymp, nodep->മൂല്യം_Arg_Refp()->name(), nodep->മൂല്യം_Arg_Refp(),
                            nullptr);
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstCoverWith* nodep) override {  // FindVisitor::
        // The range list is outside the scope of the filter, which declares 'item'
        iterateAndNextNull(nodep->subp());
        VL_RESTORER(m_curSymp);
        ++m_mod_With_അക്കം;
        m_curSymp = m_statep->തിരുകുക_Block(m_curSymp, "__VcoverWith" + cvtToStr(m_mod_With_അക്കം),
                                          nodep, m_classOrPackagep);
        m_curSymp->fallbackp(VL_RESTORER_PREV(m_curSymp));
        iterateAndNextNull(nodep->itemp());
        iterateAndNextNull(nodep->filterp());
    }
    ശൂന്യം visit(AstCoverCrossWith* nodep) override {  // FindVisitor::
        // The selection, the 'matches' count, and the cross items' types, of their coverpoints'
        // expressions with any nested scopes, are outside the filter's scope, which declares the
        // items.  As visit(AstVar) would resolve a type in the scope declaring its variable, the
        // items, generated and unique, are inserted directly.
        iterateAndNextNull(nodep->selectp());
        iterateAndNextNull(nodep->countp());
        വേണ്ടി (AstVar* varp = nodep->varsp(); varp; varp = VN_AS(varp->nextp(), Var)) {
            iterateAndNextNull(varp->childDTypep());
        }
        VL_RESTORER(m_curSymp);
        ++m_mod_With_അക്കം;
        m_curSymp = m_statep->തിരുകുക_Block(m_curSymp, "__VcoverWith" + cvtToStr(m_mod_With_അക്കം),
                                          nodep, m_classOrPackagep);
        m_curSymp->fallbackp(VL_RESTORER_PREV(m_curSymp));
        വേണ്ടി (AstVar* varp = nodep->varsp(); varp; varp = VN_AS(varp->nextp(), Var)) {
            m_statep->തിരുകുക_Sym(m_curSymp, varp->name(), varp, m_classOrPackagep);
        }
        iterateAndNextNull(nodep->filterp());
    }

    ശൂന്യം visit(Ast_കണ്ണി* nodep) override { iterateChildren(nodep); }  // FindVisitor::

    ശൂന്യം handleUnvisitedVirtIfaces() {
        Ast_കണ്ണി_Module* സ്ഥിരം മുകൾ = v3Global.rootp()->modulesp();
        m_curSymp = m_statep->get_കണ്ണി_Sym(മുകൾ);
        വേണ്ടി (AstIface* ifacep : m_virtIfaces) {
            എങ്കിൽ (!ifacep->user4()) {  // virtual interface never assigned any actual interface
                // Create dummy cell to keep the virtual interface alive
                // (later stages assume that non-dead interface has associated cell)
                AstCell* സ്ഥിരം ifacecellp = പുതിയ AstCell{ifacep->fileline(),
                                                        ifacep->fileline(),
                                                        "__VdummyInstOf" + ifacep->name(),
                                                        ifacep->name(),
                                                        nullptr,
                                                        nullptr,
                                                        nullptr};
                ifacecellp->modp(ifacep);
                മുകൾ->addStmtsp(ifacecellp);

                iterate(ifacecellp);
            }
        }
    }

പൊതു:
    // CONSTRUCTORS
    LinkDotFindVisitor(AstNetlist* rootp, LinkDotState* statep)
        : m_statep{statep} {
        UINFO(4, __FUNCTION__ << ": ");
        iterate(rootp);

        എങ്കിൽ (!m_virtIfaces.empty()) handleUnvisitedVirtIfaces();
        വേണ്ടി (AstScope* സ്ഥിരം scopep : m_scopes) scopep->modp()->addStmtsp(scopep);
        എങ്കിൽ (m_statep->forScopeCreation()) {
            V3Stats::addStat("LinkDot, Scope visits", m_statScopeVisits);
        }
    }
    ~LinkDotFindVisitor() override = സ്ഥിരസ്ഥിതി;
};

//======================================================================

വർഗ്ഗം LinkDotFindIfaceVisitor final : പൊതു VNVisitor {
    // NODE STATE
    //  *::user1p()             -> See LinkDotState

    // STATE - for current visit position (use VL_RESTORER)
    LinkDotState* സ്ഥിരം m_statep;  // State to pass between visitors, including symbol table
    Ast_കണ്ണി* m_declp = nullptr;  // Current declaring object that may soon contain IfaceRefDType

    // METHODS
    സ്ഥിരം VSymEnt* findNonDeclSym(VSymEnt* symp, സ്ഥിരം string& name) {
        // Find if there is a symbol of given name, ignoring the node that is declaring
        // it (m_declp) itself.  Thus if searching for "ifc" (an interface):
        //
        // module x;  // Finishes here and finds the typedef
        //   typedef foo ifc;
        //   ifc ifc;   // symp starts pointing here, but matches m_declp
        ആകുമ്പോൾ (symp) {
            സ്ഥിരം VSymEnt* സ്ഥിരം foundp = symp->findIdFlat(name);
            എങ്കിൽ (foundp && foundp->nodep() != m_declp) തിരികെ foundp;
            symp = symp->fallbackp();
        }
        തിരികെ nullptr;
    }

    // VISITORS
    ശൂന്യം visit(AstRefDType* nodep) override {  // FindIfaceVisitor::
        എങ്കിൽ (m_statep->forPrimary() && !nodep->classOrPackagep()) {
            UINFO(9, "  FindIfc: " << nodep);
            // If under a var, ignore the var itself as might be e.g. "intf intf;"
            // Critical tests:
            //   t_interface_param_genblk.v  // Checks this does make interface
            //   t_interface_hidden.v  // Checks this doesn't making interface when hidden
            എങ്കിൽ (m_statep->exists_കണ്ണി_Sym(nodep)) {
                VSymEnt* symp = m_statep->get_കണ്ണി_Sym(nodep);
                സ്ഥിരം VSymEnt* foundp = findNonDeclSym(symp, nodep->name());
                Ast_കണ്ണി* foundNodep = nullptr;
                // This: v4make test_regress/t/t_interface_param_genblk.py --debug
                // --debugi-V3LinkDot 9 Passes with this commented out:
                എങ്കിൽ (foundp) foundNodep = foundp->nodep();
                എങ്കിൽ (!foundNodep) foundNodep = m_statep->findModuleSym(nodep->name());
                എങ്കിൽ (foundNodep) UINFO(9, "    Ifc foundNodep " << foundNodep);
                എങ്കിൽ (AstIface* സ്ഥിരം defp = VN_CAST(foundNodep, Iface)) {
                    // Must be found as module name, and not hidden/ by normal symbol (foundp)
                    AstIfaceRefDType* സ്ഥിരം newp
                        = പുതിയ AstIfaceRefDType{nodep->fileline(), "", nodep->name()};
                    എങ്കിൽ (nodep->paramsp())
                        newp->addParamsp(nodep->paramsp()->unlinkFrBackWithNext());
                    newp->ifacep(defp);
                    newp->user1u(nodep->user1u());
                    UINFO(9, "    Resolved interface " << nodep << "  =>  " << defp);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
                    തിരികെ;
                }
            }
        }
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstVar* nodep) override {  // FindIfaceVisitor::
        VL_RESTORER(m_declp);
        m_declp = nodep;
        iterateChildren(nodep);
        AstIfaceRefDType* സ്ഥിരം ifacerefp = LinkDotState::ifaceRefFromArray(nodep->subDTypep());
        എങ്കിൽ (ifacerefp && m_statep->exists_കണ്ണി_Sym(nodep)) {
            // Can't resolve until interfaces and modport names are
            // known; see notes at top
            UINFO(9, "  FindIfc Var IfaceRef " << ifacerefp);
            എങ്കിൽ (!ifacerefp->isVirtual()) nodep->setIfaceRef();
            VSymEnt* സ്ഥിരം symp = m_statep->get_കണ്ണി_Sym(nodep);
            m_statep->തിരുകുക_Iface_Var_Sym(symp);
        }
    }
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override { iterateChildren(nodep); }  // FindIfaceVisitor::

പൊതു:
    // CONSTRUCTORS
    LinkDotFindIfaceVisitor(AstNetlist* rootp, LinkDotState* statep)
        : m_statep{statep} {
        UINFO(4, __FUNCTION__ << ": ");
        iterate(rootp);
    }
    ~LinkDotFindIfaceVisitor() override = സ്ഥിരസ്ഥിതി;
};

//======================================================================

വർഗ്ഗം LinkDotParamVisitor final : പൊതു VNVisitor {
    // NODE STATE
    //  *::user1p()             -> See LinkDotState
    //  *::user2p()             -> See LinkDotState
    //  *::user4()              -> See LinkDotState

    // STATE
    LinkDotState* സ്ഥിരം m_statep;  // State to pass between visitors, including symbol table
    Ast_കണ്ണി_Module* m_modp = nullptr;  // Current module

    ശൂന്യം pin_Implicit_വ്യഞ്ജകം_Recurse(Ast_കണ്ണി* nodep) {
        // Under a pin, Check interconnect expression for a pin reference or a concat.
        // Create implicit variable as needed
        എങ്കിൽ (VN_IS(nodep, Dot)) {  // Not creating a simple implied type,
            // and implying something else would just confuse later errors
        } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(nodep, VarRef) || VN_IS(nodep, ParseRef)) {
            // To prevent user errors, we should only do single bit
            // implicit vars, however some netlists (MIPS) expect single
            // bit implicit wires to get created with range 0:0 etc.
            m_statep->implicitOkAdd(m_modp, nodep->name());
        }
        // These are perhaps a little too generous, as a SELect of siga[sigb]
        // perhaps shouldn't create an implicit variable.  But, we've warned...
        അല്ലെങ്കിൽ {
            എങ്കിൽ (Ast_കണ്ണി* സ്ഥിരം refp = nodep->op1p()) pin_Implicit_വ്യഞ്ജകം_Recurse(refp);
            എങ്കിൽ (Ast_കണ്ണി* സ്ഥിരം refp = nodep->op2p()) pin_Implicit_വ്യഞ്ജകം_Recurse(refp);
            എങ്കിൽ (Ast_കണ്ണി* സ്ഥിരം refp = nodep->op3p()) pin_Implicit_വ്യഞ്ജകം_Recurse(refp);
            എങ്കിൽ (Ast_കണ്ണി* സ്ഥിരം refp = nodep->op4p()) pin_Implicit_വ്യഞ്ജകം_Recurse(refp);
        }
        // Continue along a list (e.g. the terminals under AstImplicit), also after a reference
        എങ്കിൽ (Ast_കണ്ണി* സ്ഥിരം refp = nodep->nextp()) pin_Implicit_വ്യഞ്ജകം_Recurse(refp);
    }

    // VISITORS
    ശൂന്യം visit(AstTypeTable*) override {}  // ParamVisitor::
    ശൂന്യം visit(Ast_കണ്ണി_Module* nodep) override {  // ParamVisitor::
        UINFO(5, "   " << nodep);
        എങ്കിൽ ((nodep->dead() || !nodep->user4()) && !nodep->hierParams()) {
            UINFO(4, "Mark dead module " << nodep);
            UASSERT_OBJ(!m_statep->forScopeCreation(), nodep,
                        "Dead module persisted past where should have removed");
            // Don't remove now, because we may have a tree of
            // parameterized modules with VARXREFs into the deleted module
            // region.  V3Dead should cleanup.
            // Downstream visitors up until V3Dead need to check for nodep->dead.
            nodep->dead(സത്യം);
            തിരികെ;
        }
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstPin* nodep) override {  // ParamVisitor::
        // Pin: Link to submodule's port
        // Deal with implicit definitions - do before Resolve visitor as may
        // be referenced above declaration
        എങ്കിൽ (nodep->exprp()  // Else specifically unconnected pin
            && !nodep->svImplicit()) {  // SV 19.11.3: .name pins don't allow implicit decls
            pin_Implicit_വ്യഞ്ജകം_Recurse(nodep->exprp());
        }
    }
    ശൂന്യം visit(AstDefParam* nodep) override {  // ParamVisitor::
        iterateChildren(nodep);
        nodep->v3warn(DEFPARAM, "defparam is deprecated (IEEE 1800-2023 C.4.1)\n"
                                    << nodep->warnMore()
                                    << "... Suggest use instantiation with #(."
                                    << nodep->prettyName() << "(...etc...))");
        തർക്കം hasPartSelect = അസത്യം;
        സ്ഥിരം string path = extractDottedPath(nodep->pathp(), hasPartSelect);
        UASSERT_OBJ(!hasPartSelect && !path.empty(), nodep, "Unexpected defparam path shape");
        string baddot;
        VSymEnt* okSymp = nullptr;
        VSymEnt* സ്ഥിരം foundp = m_statep->findDotted(
            nodep->fileline(), m_statep->get_കണ്ണി_Sym(nodep), path, baddot, okSymp, സത്യം);
        AstCell* സ്ഥിരം cellp = foundp ? VN_AS(foundp->nodep(), Cell) : nullptr;
        എങ്കിൽ (!cellp) {
            nodep->v3error("In defparam, instance " << path << " never declared");
        } അല്ലെങ്കിൽ {
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം exprp = nodep->rhsp()->unlinkFrBack();
            UINFO(9, "Defparam cell " << path << "." << nodep->name() << " attach-to " << cellp
                                      << "  <= " << exprp);
            // Don't need to check the name of the defparam exists.  V3Param does.
            AstPin* സ്ഥിരം pinp = പുതിയ AstPin{nodep->fileline(),
                                            -1,  // Pin# not relevant
                                            nodep->name(), exprp};
            pinp->param(സത്യം);
            pinp->paramPath(path);
            cellp->addParamsp(pinp);
        }
        VL_DO_DANGLING(nodep->unlinkFrBack()->delete_വൃക്ഷം(), nodep);
    }
    ശൂന്യം visit(AstPort* nodep) override {  // ParamVisitor::
        // Port: Stash the pin number
        // Need to set pin numbers after varnames are created
        // But before we do the final resolution based on names
        VSymEnt* സ്ഥിരം foundp = m_statep->get_കണ്ണി_Sym(m_modp)->findIdFlat(nodep->name());
        AstVar* സ്ഥിരം refp = foundp ? VN_CAST(foundp->nodep(), Var) : nullptr;
        എങ്കിൽ (!foundp) {
            nodep->v3error(
                "Input/output/inout declaration not found for port: " << nodep->prettyNameQ());
        } അല്ലെങ്കിൽ എങ്കിൽ (!refp || (!refp->isIO() && !refp->isIfaceRef())) {
            nodep->v3error("Pin is not an in/out/inout/interface: " << nodep->prettyNameQ());
        } അല്ലെങ്കിൽ {
            എങ്കിൽ (refp->user4()) {
                nodep->v3error("Duplicate declaration of port: "
                               << nodep->prettyNameQ() << '\n'
                               << nodep->warnContextPrimary() << '\n'
                               << refp->warnOther() << "... Location of original declaration\n"
                               << refp->warnContextSecondary());
            }
            refp->user4(സത്യം);
            VSymEnt* സ്ഥിരം symp = m_statep->തിരുകുക_Sym(m_statep->get_കണ്ണി_Sym(m_modp),
                                                      "__pinNumber" + cvtToStr(nodep->pin_അക്കം()),
                                                      refp, nullptr /*classOrPackagep*/);
            symp->exported(അസത്യം);
            refp->pin_അക്കം(nodep->pin_അക്കം());
            // Put the variable where the port is, so that variables stay
            // in pin number sorted order. Otherwise hierarchical or JSON
            // may botch by-position instances.
            nodep->addHereThisAsNext(refp->unlinkFrBack());
        }
        // Ports not needed any more
        VL_DO_DANGLING(nodep->unlinkFrBack()->delete_വൃക്ഷം(), nodep);
    }
    ശൂന്യം visit(AstAssignW* nodep) override {  // ParamVisitor::
        // Deal with implicit definitions
        // We used to nodep->allowImplicit() here, but it turns out
        // normal "assigns" can also make implicit wires.  Yuk.
        pin_Implicit_വ്യഞ്ജകം_Recurse(nodep->lhsp());
        iterateChildren(nodep);
    }

    ശൂന്യം visit(AstImplicit* nodep) override {  // ParamVisitor::
        // Unsupported gates need implicit creation
        pin_Implicit_വ്യഞ്ജകം_Recurse(nodep->exprsp());
        // We're done with implicit gates
        VL_DO_DANGLING(nodep->unlinkFrBack()->delete_വൃക്ഷം(), nodep);
    }
    ശൂന്യം visit(AstClassOrPackageRef* nodep) override {  // ParamVisitor::
        എങ്കിൽ (auto* സ്ഥിരം fwdp = VN_CAST(nodep->classOrPackageNodep(), TypedefFwd)) {
            // Relink forward definitions to the "real" definition
            VSymEnt* സ്ഥിരം foundp = m_statep->get_കണ്ണി_Sym(fwdp)->findIdFallback(fwdp->name());
            എങ്കിൽ (foundp && (VN_IS(foundp->nodep(), Class) || VN_IS(foundp->nodep(), Package))) {
                nodep->classOrPackagep(VN_AS(foundp->nodep(), കണ്ണി_Module));
            } അല്ലെങ്കിൽ എങ്കിൽ (foundp && VN_IS(foundp->nodep(), ParamTypeDType)) {
                UASSERT(m_statep->forPrimary(), "Param types should have been resolved");
                nodep->classOrPackageNodep(VN_AS(foundp->nodep(), ParamTypeDType));
            } അല്ലെങ്കിൽ {
                എങ്കിൽ (foundp) UINFO(1, "found nodep = " << foundp->nodep());
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
    ശൂന്യം visit(AstPull* nodep) override {  // ParamVisitor::
        // Deal with implicit definitions
        // We used to nodep->allowImplicit() here, but it turns out
        // normal "assigns" can also make implicit wires.  Yuk.
        pin_Implicit_വ്യഞ്ജകം_Recurse(nodep->lhsp());
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstTypedefFwd* nodep) override {  // ParamVisitor::
        VSymEnt* സ്ഥിരം foundp = m_statep->get_കണ്ണി_Sym(nodep)->findIdFallback(nodep->name());
        എങ്കിൽ (foundp) {
            // If the typedef was earlier in source order (tokenNum), then remember that earlier
            // point to avoid error something wasn't declared
            // Might be forward declaring something odd (with declTokenNumSetMin not implemented,
            // but should be harmless to ignore as this is just for error detection
            foundp->nodep()->decl_Token_അക്കം_Set_Min(nodep->fileline()->token_അക്കം());
        } അല്ലെങ്കിൽ എങ്കിൽ (v3Global.opt.pedantic()) {
            // We only check it was ever really defined in pedantic mode, as it
            // might have been in a header file referring to a module we never
            // needed so there are false positives
            nodep->v3error(
                "Forward typedef unused or does not resolve to a data type (IEEE 1800-2023 6.18): "
                << nodep->prettyNameQ());
        }
        // We only needed the forward declaration in order to parse correctly.
        // Delete later as may be ClassOrPackageRef's still pointing to it
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep->unlinkFrBack()), nodep);
    }

    ശൂന്യം visit(Ast_കണ്ണി* nodep) override { iterateChildren(nodep); }  // ParamVisitor::

പൊതു:
    // CONSTRUCTORS
    LinkDotParamVisitor(AstNetlist* rootp, LinkDotState* statep)
        : m_statep{statep} {
        UINFO(4, __FUNCTION__ << ": ");
        iterate(rootp);
    }
    ~LinkDotParamVisitor() override = സ്ഥിരസ്ഥിതി;
};

//======================================================================

വർഗ്ഗം LinkDotScopeVisitor final : പൊതു VNVisitor {

    // STATE
    LinkDotState* സ്ഥിരം m_statep;  // State to pass between visitors, including symbol table
    സ്ഥിരം AstScope* m_scopep = nullptr;  // The current scope
    VSymEnt* m_modSymp = nullptr;  // Symbol entry for current module
    // Deferred AliasScope processing - must be done outer-to-inner for correct alias resolution
    std::vector<std::pair<AstAliasScope*, VSymEnt*>> m_deferredAliasScopes;

    // METHODS
പൊതു:
    // getAlias with setVarAlias and setIfaceAlias implement disjoint-set data structure.
    // This algorithm is needed for the case when multiple alias statements
    // reference partially the same variables.
    സ്ഥിതം AstVarScope* getAlias(AstVarScope* സ്ഥിരം vscp) {
        എങ്കിൽ (vscp->user2p() && vscp != vscp->user2p()) {
            AstVarScope* സ്ഥിരം aliasp = getAlias(VN_AS(vscp->user2p(), VarScope));
            vscp->user2p(aliasp);
            തിരികെ aliasp;
        } അല്ലെങ്കിൽ {
            തിരികെ vscp;
        }
    }

സ്വകാര്യ:
    // Alias an interface reference to the interface it is connected to
    ശൂന്യം setIfaceAlias(AstVarScope* സ്ഥിരം vscp, AstVarScope* സ്ഥിരം aliasp) {
        UASSERT_OBJ(vscp->varp()->isIfaceRef(), vscp, "Interface alias of non-interface");
        UASSERT_OBJ(aliasp->varp()->isIfaceRef(), aliasp, "Interface alias of non-interface");
        getAlias(vscp)->user2p(getAlias(aliasp));
    }

    // Alias two variables. An output port in the lower scope survives, unless the other
    // must keep its own storage.
    ശൂന്യം setVarAlias(AstVarScope* ap, AstVarScope* bp) {
        UASSERT_OBJ(!ap->varp()->isIfaceRef(), ap, "Variable alias of interface");
        UASSERT_OBJ(!bp->varp()->isIfaceRef(), bp, "Variable alias of interface");
        ap = getAlias(ap);
        bp = getAlias(bp);
        എങ്കിൽ (ap == bp) തിരികെ;
        സ്ഥിരം തർക്കം aWins = [&]() {
            സ്ഥിരം AstVar* സ്ഥിരം aVarp = ap->varp();
            സ്ഥിരം AstVar* സ്ഥിരം bVarp = bp->varp();
            // A primary IO is accessed directly by the user of the model, so must survive
            UASSERT_OBJ(!aVarp->isPrimaryIO() || !bVarp->isPrimaryIO(), ap,
                        "Alias of two primary IOs");
            എങ്കിൽ (bVarp->isPrimaryIO()) തിരികെ അസത്യം;
            എങ്കിൽ (aVarp->isPrimaryIO()) തിരികെ സത്യം;
            // A forced or public variable must keep its own storage. If both are, either works.
            എങ്കിൽ (aVarp->isForced() != bVarp->isForced()) തിരികെ aVarp->isForced();
            എങ്കിൽ (aVarp->isSigPublic() != bVarp->isSigPublic()) തിരികെ aVarp->isSigPublic();
            // Otherwise prefer an output port in the lower scope. This enables better V3Combine
            // due to having fewer upward hierarchical references (which prevent combining).
            എങ്കിൽ (aVarp->direction() != VDirection::OUTPUT) തിരികെ അസത്യം;
            // Is 'ap' strictly below 'bp' in the hierarchy
            വേണ്ടി (സ്ഥിരം AstScope* scopep = ap->scopep()->aboveScopep(); scopep;
                 scopep = scopep->aboveScopep()) {
                എങ്കിൽ (scopep == bp->scopep()) തിരികെ സത്യം;
            }
            തിരികെ അസത്യം;
        }();
        എങ്കിൽ (aWins) {
            bp->user2p(ap);
        } അല്ലെങ്കിൽ {
            ap->user2p(bp);
        }
    }

    // VISITORS
    ശൂന്യം visit(AstNetlist* nodep) override {  // ScopeVisitor::
        // Recurse..., backward as must do packages before using packages
        iterateChildrenBackwardsConst(nodep);
    }
    ശൂന്യം visit(AstScope* nodep) override {  // ScopeVisitor::
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
        എങ്കിൽ (AstIface* സ്ഥിരം ifacep = VN_CAST(nodep->modp(), Iface)) {
            m_statep->തിരുകുക_Iface_Mod_Sym(ifacep, m_modSymp);
        }
        m_scopep = nodep;
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstVarScope* nodep) override {  // ScopeVisitor::
        എങ്കിൽ (!nodep->varp()->isFuncLocal() && !nodep->varp()->isClassMember()) {
            VSymEnt* സ്ഥിരം varSymp
                = m_statep->തിരുകുക_Sym(m_modSymp, nodep->varp()->name(), nodep, nullptr);
            എങ്കിൽ (nodep->varp()->isIfaceRef()) {
                AstIfaceRefDType* സ്ഥിരം dtypep
                    = LinkDotState::ifaceRefFromArray(nodep->varp()->dtypep());
                UASSERT_OBJ(dtypep, nodep, "Non AstIfaceRefDType on isIfaceRef() var");
                എങ്കിൽ (nodep->varp()->isIfaceParent()) {
                    UINFO(9, "Iface parent ref var " << nodep->varp()->name() << " " << nodep);
                    // Find the interface cell the var references
                    UINFO(9, "Iface parent dtype " << dtypep);
                    സ്ഥിരം string ifcellname = dtypep->cellName();
                    string baddot;
                    VSymEnt* okSymp;
                    VSymEnt* cellSymp = m_statep->findDotted(nodep->fileline(), m_modSymp,
                                                             ifcellname, baddot, okSymp, അസത്യം);
                    UASSERT_OBJ(
                        cellSymp, nodep,
                        "No symbol for interface instance: " << nodep->prettyNameQ(ifcellname));
                    UINFO(5, "       Found interface instance: se" << cvtToHex(cellSymp) << " "
                                                                   << cellSymp->nodep());
                    എങ്കിൽ (dtypep->modportName() != "") {
                        // Look up the modport within the interface cell's symbol table
                        VSymEnt* സ്ഥിരം mpSymp = cellSymp->findIdFallback(dtypep->modportName());
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
                    m_statep->തിരുകുക_Scope_Alias(LinkDotState::SAMN_IFTOP, varSymp, cellSymp);
                }
            }
        }
    }
    ശൂന്യം visit(Ast_കണ്ണി_FTask* nodep) override {  // ScopeVisitor::
        VSymEnt* സ്ഥിരം symp = m_statep->തിരുകുക_Block(m_modSymp, nodep->name(), nodep, nullptr);
        symp->fallbackp(m_modSymp);
        iterateChildren(nodep);
    }
    ശൂന്യം visit(Ast_കണ്ണി_Foreach* nodep) override {  // ScopeVisitor::
        VSymEnt* സ്ഥിരം symp = m_statep->തിരുകുക_Block(m_modSymp, nodep->name(), nodep, nullptr);
        symp->fallbackp(m_modSymp);
        // No recursion, we don't want to pick up variables
    }
    ശൂന്യം visit(AstConstraintForeach* nodep) override {  // ScopeVisitor::
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstWith* nodep) override {  // ScopeVisitor::
        VSymEnt* സ്ഥിരം symp = m_statep->തിരുകുക_Block(m_modSymp, nodep->name(), nodep, nullptr);
        symp->fallbackp(m_modSymp);
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstAlias* nodep) override {  // ScopeVisitor::
        // Track aliases
        UINFOTREE(9, nodep, "", "alias");
        AstVarScope* aliasVscp = nullptr;
        വേണ്ടി (Ast_കണ്ണി* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
            AstVarScope* vscp = nullptr;
            എങ്കിൽ (സ്ഥിരം AstVarRef* സ്ഥിരം refp = VN_CAST(itemp, VarRef)) {
                vscp = refp->varScopep();
            } അല്ലെങ്കിൽ {
                // Reaches into the scope of an instance, look it up by name
                സ്ഥിരം AstVarXRef* സ്ഥിരം xrefp = VN_AS(itemp, VarXRef);
                സ്ഥിരം string scopename = xrefp->dotted() + "." + xrefp->name();
                string baddot;
                VSymEnt* okSymp;
                VSymEnt* സ്ഥിരം symp = m_statep->findDotted(xrefp->fileline(), m_modSymp, scopename,
                                                           baddot, okSymp, അസത്യം);
                UASSERT_OBJ(symp, nodep, "No symbol for alias item: " << scopename);
                vscp = VN_CAST(symp->nodep(), VarScope);
            }
            UASSERT_OBJ(vscp, nodep, "VarScope unset");
            എങ്കിൽ (aliasVscp) {
                setVarAlias(aliasVscp, vscp);
            } അല്ലെങ്കിൽ {
                aliasVscp = vscp;
            }
        }
        iterateChildren(nodep);
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep->unlinkFrBack()), nodep);
    }
    ശൂന്യം visit(AstAliasScope* nodep) override {  // ScopeVisitor::
        // Defer AliasScope processing - must process outer scopes before inner ones
        // so that nested interface port alias resolution works correctly
        UINFO(5, "ALIASSCOPE (deferred) " << nodep);
        m_deferredAliasScopes.emplace_back(nodep, m_modSymp);
    }
    ശൂന്യം processAliasScope(AstAliasScope* nodep, VSymEnt* modSymp) {
        UINFO(5, "ALIASSCOPE  " << nodep);
        UINFOTREE(9, nodep, "", "avs");
        VSymEnt* rhsSymp;
        {
            AstVarRef* സ്ഥിരം refp = VN_CAST(nodep->rhsp(), VarRef);
            AstVarXRef* സ്ഥിരം xrefp = VN_CAST(nodep->rhsp(), VarXRef);
            UASSERT_OBJ(refp || xrefp, nodep,
                        "Unsupported: Non Var(X)Ref attached to interface pin");
            string inl
                = ((xrefp && xrefp->inlinedDots().size()) ? (xrefp->inlinedDots() + "__DOT__")
                                                          : "");
            സ്ഥിരം string dottedPath
                = (xrefp && !xrefp->dotted().empty()) ? (xrefp->dotted() + ".") : "";
            VSymEnt* symp = nullptr;
            string scopename;
            ആകുമ്പോൾ (!symp) {
                scopename = refp ? refp->name()
                                 : (inl.size() ? (inl + dottedPath + xrefp->name())
                                               : (dottedPath + xrefp->name()));
                string baddot;
                VSymEnt* okSymp;
                symp = m_statep->findDotted(nodep->rhsp()->fileline(), modSymp, scopename, baddot,
                                            okSymp, സത്യം);
                എങ്കിൽ (inl == "") നിർത്തുക;
                inl = LinkDotState::removeLastInlineScope(inl);
            }
            UASSERT_OBJ(symp, nodep, "No symbol for interface alias rhs");
            UINFO(5, "       Found a linked scope RHS: " << scopename << "  se" << cvtToHex(symp)
                                                         << " " << symp->nodep());
            rhsSymp = symp;
        }
        VSymEnt* lhsSymp;
        {
            സ്ഥിരം AstVarXRef* സ്ഥിരം xrefp = VN_CAST(nodep->lhsp(), VarXRef);
            സ്ഥിരം AstVarRef* സ്ഥിരം refp = VN_CAST(nodep->lhsp(), VarRef);

            UASSERT_OBJ(refp || xrefp, nodep,
                        "Unsupported: Non Var(X)Ref attached to interface pin");
            സ്ഥിരം string scopename
                = refp ? refp->varp()->name() : xrefp->dotted() + "." + xrefp->name();
            string baddot;
            VSymEnt* okSymp;
            VSymEnt* സ്ഥിരം symp = m_statep->findDotted(nodep->lhsp()->fileline(), modSymp,
                                                       scopename, baddot, okSymp, അസത്യം);
            UASSERT_OBJ(symp, nodep, "No symbol for interface alias lhs");
            UINFO(5, "       Found a linked scope LHS: " << scopename << "  se" << cvtToHex(symp)
                                                         << " " << symp->nodep());
            lhsSymp = symp;
        }
        // Remember the alias - can't do it yet because we may have additional symbols to be added,
        // or maybe an alias of an alias
        m_statep->തിരുകുക_Scope_Alias(LinkDotState::SAMN_IFTOP, lhsSymp, rhsSymp);
        AstVarScope* സ്ഥിരം lhsVscp = VN_CAST(lhsSymp->nodep(), VarScope);
        AstVarScope* സ്ഥിരം rhsVscp = VN_CAST(rhsSymp->nodep(), VarScope);
        UASSERT_OBJ(lhsVscp && rhsVscp, nodep, "Interface alias missing variable scope");
        setIfaceAlias(lhsVscp, rhsVscp);
        // We have stored the link, we don't need these any more
        VL_DO_DANGLING(nodep->unlinkFrBack()->delete_വൃക്ഷം(), nodep);
    }
    ശൂന്യം processDeferredAliasScopes() {
        // Sort by hierarchy depth (shallower first) so outer aliases are resolved before inner
        // Pre-compute depth map to avoid O(N log N * D) complexity in sort comparisons
        std::unordered_map<VSymEnt*, പൂർണ്ണസംഖ്യ> depthMap;
        വേണ്ടി (സ്ഥിരം auto& pair : m_deferredAliasScopes) {
            VSymEnt* സ്ഥിരം symp = pair.second;
            എങ്കിൽ (depthMap.find(symp) == depthMap.end()) {
                പൂർണ്ണസംഖ്യ depth = 0;
                വേണ്ടി (VSymEnt* p = symp; p; p = p->parentp()) ++depth;
                depthMap[symp] = depth;
            }
        }
        std::stable_sort(m_deferredAliasScopes.begin(), m_deferredAliasScopes.end(),
                         [&depthMap](സ്ഥിരം std::pair<AstAliasScope*, VSymEnt*>& a,
                                     സ്ഥിരം std::pair<AstAliasScope*, VSymEnt*>& b) {
                             തിരികെ depthMap.at(a.second) < depthMap.at(b.second);
                         });
        // Process in sorted order
        വേണ്ടി (auto& pair : m_deferredAliasScopes) { processAliasScope(pair.first, pair.second); }
        m_deferredAliasScopes.clear();
    }
    ശൂന്യം visit(Ast_കണ്ണി_Gen* nodep) override {  // ScopeVisitor::  // LCOV_EXCL_LINE
        nodep->v3fatalSrc("Generate constructs should have been reduced out");
    }
    // For speed, don't recurse things that can't have scope
    // Note we allow AstNodeStmt's as generates may be under them
    ശൂന്യം visit(AstCell*) override {}  // ScopeVisitor::
    ശൂന്യം visit(AstVar*) override {}  // ScopeVisitor::
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override { iterateChildren(nodep); }  // ScopeVisitor::

പൊതു:
    // CONSTRUCTORS
    LinkDotScopeVisitor(AstNetlist* rootp, LinkDotState* statep)
        : m_statep{statep} {
        UINFO(4, __FUNCTION__ << ": ");
        iterate(rootp);
        // Process deferred AliasScopes in outer-to-inner order
        processDeferredAliasScopes();
    }
    ~LinkDotScopeVisitor() override = സ്ഥിരസ്ഥിതി;
};

//======================================================================

// Iterate an interface to resolve modports
വർഗ്ഗം LinkDotIfaceVisitor final : പൊതു VNVisitor {
    // STATE
    LinkDotState* സ്ഥിരം m_statep;  // State to pass between visitors, including symbol table
    VSymEnt* m_curSymp;  // Symbol Entry for current table, where to lookup/insert

    // VISITORS
    ശൂന്യം visit(AstModport* nodep) override {  // IfaceVisitor::
        // Modport: Remember its name for later resolution
        UINFO(5, "   fiv: " << nodep);
        VL_RESTORER(m_curSymp);
        {
            // Create symbol table for the vars
            m_curSymp = m_statep->തിരുകുക_Block(m_curSymp, nodep->name(), nodep, nullptr);
            m_curSymp->fallbackp(VL_RESTORER_PREV(m_curSymp));
            iterateChildren(nodep);
        }
    }
    ശൂന്യം visit(AstModportFTaskRef* nodep) override {  // IfaceVisitor::
        UINFO(5, "   fif: " << nodep);
        iterateChildren(nodep);
        VSymEnt* സ്ഥിരം symp = m_curSymp->findIdFallback(nodep->name());
        എങ്കിൽ (!symp) {
            nodep->v3error("Modport item not found: " << nodep->prettyNameQ());
        } അല്ലെങ്കിൽ എങ്കിൽ (Ast_കണ്ണി_FTask* സ്ഥിരം ftaskp = VN_CAST(symp->nodep(), കണ്ണി_FTask)) {
            // Make symbol under modport that points at the _interface_'s var, not the modport.
            nodep->ftaskp(ftaskp);
            VSymEnt* സ്ഥിരം subSymp
                = m_statep->തിരുകുക_Sym(m_curSymp, nodep->name(), ftaskp, nullptr /*package*/);
            m_statep->തിരുകുക_Scope_Alias(LinkDotState::SAMN_MODPORT, subSymp, symp);
        } അല്ലെങ്കിൽ {
            nodep->v3error("Modport item is not a function/task: " << nodep->prettyNameQ());
        }
        എങ്കിൽ (m_statep->forScopeCreation()) {
            // Done with AstModportFTaskRef.
            // Delete to prevent problems if we dead-delete pointed to ftask
            nodep->unlinkFrBack();
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
        }
    }

    // Helper to resolve remaining path through a nested interface
    // When findDotted() partially matches (okSymp set, baddot non-empty),
    // this follows the interface type to resolve the remaining path.
    // Returns the resolved symbol, or nullptr if not found.
    // On success, clears baddot; on partial match of multi-level path, updates baddot.
    VSymEnt* resolveNestedInterfacePath(File_വരി* fl, VSymEnt* okSymp, string& baddot) {
        എങ്കിൽ (!okSymp || baddot.empty()) തിരികെ nullptr;

        സ്ഥിതം constexpr പൂർണ്ണസംഖ്യ MAX_NESTING_DEPTH = 64;
        VSymEnt* curOkSymp = okSymp;

        വേണ്ടി (പൂർണ്ണസംഖ്യ depth = 0; depth < MAX_NESTING_DEPTH; ++depth) {
            // Try to get interface from the partially-matched symbol
            AstIface* ifacep = nullptr;
            എങ്കിൽ (സ്ഥിരം AstCell* സ്ഥിരം cellp = VN_CAST(curOkSymp->nodep(), Cell)) {
                ifacep = VN_CAST(cellp->modp(), Iface);
            } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstVar* സ്ഥിരം varp = VN_CAST(curOkSymp->nodep(), Var)) {
                എങ്കിൽ (varp->isIfaceRef()) {
                    എങ്കിൽ (സ്ഥിരം AstIfaceRefDType* സ്ഥിരം ifaceRefp
                        = LinkDotState::ifaceRefFromArray(varp->dtypep())) {
                        ifacep = ifaceRefp->ifaceViaCellp();
                    }
                }
            }

            എങ്കിൽ (!ifacep || !m_statep->exists_കണ്ണി_Sym(ifacep)) തിരികെ nullptr;

            VSymEnt* സ്ഥിരം ifaceSymp = m_statep->get_കണ്ണി_Sym(ifacep);

            എങ്കിൽ (baddot.find('.') == string::npos) {
                // Simple identifier - direct lookup
                VSymEnt* സ്ഥിരം symp = ifaceSymp->findIdFallback(baddot);
                എങ്കിൽ (symp) baddot.clear();
                തിരികെ symp;
            }

            // Multi-level path - use findDotted for partial resolution
            string remainingBaddot;
            VSymEnt* remainingOkSymp = nullptr;
            VSymEnt* സ്ഥിരം symp = m_statep->findDotted(fl, ifaceSymp, baddot, remainingBaddot,
                                                       remainingOkSymp, സത്യം);
            എങ്കിൽ (symp) {
                baddot = remainingBaddot;
                തിരികെ symp;
            }

            // findDotted partially matched - check progress
            എങ്കിൽ (remainingBaddot == baddot || !remainingOkSymp) തിരികെ nullptr;

            // Continue resolving with updated state
            baddot = remainingBaddot;
            curOkSymp = remainingOkSymp;
        }

        UINFO(1, "Nested interface resolution depth limit exceeded at " << fl);
        തിരികെ nullptr;
    }

    // Resolve a modport expression to find the referenced symbol
    VSymEnt* resolveModportExpression(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* exprp) {
        UINFO(5, "     resolveModportExpression: " << exprp);
        VSymEnt* symp = nullptr;
        എങ്കിൽ (AstParseRef* സ്ഥിരം refp = VN_CAST(exprp, ParseRef)) {
            // Simple variable reference: modport mp(input .a(sig_a))
            symp = m_curSymp->findIdFallback(refp->name());
        } അല്ലെങ്കിൽ എങ്കിൽ (AstVarRef* സ്ഥിരം refp = VN_CAST(exprp, VarRef)) {
            // Already resolved VarRef (can happen if iterateChildren resolved it)
            എങ്കിൽ (refp->varScopep()) {
                // During scope creation, VarRef may have VarScope already linked
                symp = m_statep->get_കണ്ണി_Sym(refp->varScopep());
            } അല്ലെങ്കിൽ എങ്കിൽ (refp->varp()) {
                symp = m_curSymp->findIdFallback(refp->varp()->name());
            }
        } അല്ലെങ്കിൽ എങ്കിൽ (AstVarXRef* സ്ഥിരം refp = VN_CAST(exprp, VarXRef)) {
            // Resolved VarXRef (dotted reference resolved by earlier pass)
            എങ്കിൽ (refp->varp()) {
                // For nested interfaces, the var is in a different scope
                // First try to find the symbol via the var itself
                എങ്കിൽ (m_statep->exists_കണ്ണി_Sym(refp->varp())) {
                    symp = m_statep->get_കണ്ണി_Sym(refp->varp());
                } അല്ലെങ്കിൽ {
                    // Fallback: look up in current scope
                    symp = m_curSymp->findIdFallback(refp->varp()->name());
                }
            } അല്ലെങ്കിൽ എങ്കിൽ (!refp->dotted().empty()) {
                // varp not set yet - use dotted path to find the symbol
                // The dotted part (e.g., "base") is the interface cell, and name is the member
                സ്ഥിരം string fullPath = refp->dotted() + "." + refp->name();
                string baddot;
                VSymEnt* okSymp = nullptr;
                symp = m_statep->findDotted(fl, m_curSymp, fullPath, baddot, okSymp, സത്യം);
                // Handle nested interface path when findDotted had partial match
                എങ്കിൽ (okSymp && !baddot.empty()) {
                    VSymEnt* സ്ഥിരം resolved = resolveNestedInterfacePath(fl, okSymp, baddot);
                    എങ്കിൽ (resolved) symp = resolved;
                }
            }
        } അല്ലെങ്കിൽ എങ്കിൽ (AstDot* സ്ഥിരം dotp = VN_CAST(exprp, Dot)) {
            // Dotted path: modport mp(input .a(inner.sig))
            തർക്കം hasPartSelect = അസത്യം;
            സ്ഥിരം string dottedPath = extractDottedPath(dotp, hasPartSelect);
            എങ്കിൽ (hasPartSelect) {
                fl->v3warn(
                    E_UNSUPPORTED,
                    "Unsupported: Modport expression with part select (IEEE 1800-2023 25.5.4)");
            } അല്ലെങ്കിൽ {
                string baddot;
                VSymEnt* okSymp = nullptr;
                symp = m_statep->findDotted(fl, m_curSymp, dottedPath, baddot, okSymp, സത്യം);

                // Handle nested interface path when findDotted had partial match
                എങ്കിൽ (okSymp && !baddot.empty()) {
                    VSymEnt* സ്ഥിരം resolved = resolveNestedInterfacePath(fl, okSymp, baddot);
                    എങ്കിൽ (resolved) symp = resolved;
                }

                എങ്കിൽ (!symp || !baddot.empty()) {
                    fl->v3error("Can't find modport expression target: "
                                << Ast_കണ്ണി::prettyNameQ(dottedPath));
                    symp = nullptr;
                }
            }
        } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(exprp, SelBit) || VN_IS(exprp, SelExtract)) {
            // Part select expressions not yet supported
            fl->v3warn(E_UNSUPPORTED,
                       "Unsupported: Modport expression with part select (IEEE 1800-2023 25.5.4)");
        } അല്ലെങ്കിൽ {
            // Other expression types not supported
            fl->v3warn(E_UNSUPPORTED,
                       "Unsupported: Complex modport expression (IEEE 1800-2023 25.5.4)");
        }
        തിരികെ symp;
    }

    ശൂന്യം visit(AstModportVarRef* nodep) override {  // IfaceVisitor::
        UINFO(5, "   fiv: " << nodep);
        iterateChildren(nodep);
        VSymEnt* symp = nullptr;
        എങ്കിൽ (nodep->exprp()) {
            // Modport expression syntax: modport mp(input .port_name(expression))
            symp = resolveModportExpression(nodep->fileline(), nodep->exprp());
        } അല്ലെങ്കിൽ {
            symp = m_curSymp->findIdFallback(nodep->name());
        }
        എങ്കിൽ (!symp) {
            nodep->v3error("Modport item not found: " << nodep->prettyNameQ());
        } അല്ലെങ്കിൽ എങ്കിൽ (AstVar* സ്ഥിരം varp = VN_CAST(symp->nodep(), Var)) {
            // Make symbol under modport that points at the _interface_'s var via the modport.
            // (Need modport still to test input/output markings)
            nodep->varp(varp);
            എങ്കിൽ (nodep->exprp()) {
                // For modport expressions, insert symbol pointing to the underlying var (not the
                // ModportVarRef which will be deleted during scope creation). The virtual port
                // name maps to the real signal's var.
                VSymEnt* സ്ഥിരം subSymp
                    = m_statep->തിരുകുക_Sym(m_curSymp, nodep->name(), varp, nullptr /*package*/);
                m_statep->തിരുകുക_Scope_Alias(LinkDotState::SAMN_MODPORT, subSymp, symp);
            } അല്ലെങ്കിൽ {
                // For regular modport items, insert symbol pointing to ModportVarRef for
                // input/output marking tests.
                m_statep->തിരുകുക_Sym(m_curSymp, nodep->name(), nodep, nullptr /*package*/);
            }
        } അല്ലെങ്കിൽ എങ്കിൽ (AstVarScope* സ്ഥിരം vscp = VN_CAST(symp->nodep(), VarScope)) {
            // Make symbol under modport that points at the _interface_'s var, not the modport.
            nodep->varp(vscp->varp());
            // Only insert symbol for modport expression virtual ports (exprp set).
            // For regular modport items, don't insert into the shared modport table because
            // each instance would overwrite the previous one, causing wrong VarScope lookups.
            // Regular items are found via the modport's fallback to the interface.
            എങ്കിൽ (nodep->exprp()) {
                VSymEnt* സ്ഥിരം subSymp
                    = m_statep->തിരുകുക_Sym(m_curSymp, nodep->name(), vscp, nullptr /*package*/);
                m_statep->തിരുകുക_Scope_Alias(LinkDotState::SAMN_MODPORT, subSymp, symp);
            }
        } അല്ലെങ്കിൽ {
            nodep->v3error("Modport item is not a variable: " << nodep->prettyNameQ());
        }
        എങ്കിൽ (m_statep->forScopeCreation()) {
            // Done with AstModportVarRef.
            // Delete to prevent problems if we dead-delete pointed to variable
            nodep->unlinkFrBack();
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
        }
    }
    ശൂന്യം visit(AstModportClockingRef* nodep) override {  // IfaceVisitor::
        UINFO(5, "   fic: " << nodep);
        iterateChildren(nodep);
        VSymEnt* സ്ഥിരം symp = m_curSymp->findIdFallback(nodep->name());
        എങ്കിൽ (!symp) {
            nodep->v3error("Modport item not found: " << nodep->prettyNameQ());
        } അല്ലെങ്കിൽ എങ്കിൽ (AstClocking* സ്ഥിരം clockingp = VN_CAST(symp->nodep(), Clocking)) {
            nodep->clockingp(clockingp);
            m_statep->തിരുകുക_Sym(m_curSymp, nodep->name(), nodep, nullptr /*package*/);
        } അല്ലെങ്കിൽ {
            nodep->v3error("Modport item is not a clocking block: " << nodep->prettyNameQ());
        }
    }
    ശൂന്യം visit(AstScope* nodep) override {  // IfaceVisitor::
        എങ്കിൽ (nodep->user3SetOnce()) തിരികെ;
        iterateChildren(nodep);
    }
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override { iterateChildren(nodep); }  // IfaceVisitor::

പൊതു:
    // CONSTRUCTORS
    LinkDotIfaceVisitor(AstIface* nodep, VSymEnt* curSymp, LinkDotState* statep)
        : m_statep{statep}
        , m_curSymp{curSymp} {
        UINFO(4, __FUNCTION__ << ": ");
        iterate(nodep);
    }
    ~LinkDotIfaceVisitor() override = സ്ഥിരസ്ഥിതി;
};

ശൂന്യം LinkDotState::computeIfaceModSyms() {
    //  AstScope::user3()  // bool. Already iterated by a LinkDotIfaceVisitor
    സ്ഥിരം VNUser3InUse user3InUse;
    വേണ്ടി (സ്ഥിരം auto& itr : m_ifaceModSyms) {
        AstIface* സ്ഥിരം nodep = itr.first;
        VSymEnt* സ്ഥിരം symp = itr.second;
        LinkDotIfaceVisitor{nodep, symp, ഇത്};
    }
    m_ifaceModSyms.clear();
}

//======================================================================

വർഗ്ഗം LinkDotResolveVisitor final : പൊതു VNVisitor {
    // NODE STATE
    // Cleared on global
    //  *::user1p()             -> See LinkDotState
    //  *::user2p()             -> See LinkDotState
    //  *::user3()              // bool. Processed
    //  *::user4()              -> See LinkDotState
    സ്ഥിരം VNUser3InUse m_inuser3;

    // TYPES
    എണ്ണൽ DotPosition : uint8_t {
        // Must match ascii() method below
        DP_NONE = 0,  // Not under a DOT
        DP_PACKAGE,  // {package-or-class}:: DOT
        DP_FIRST,  // {scope-or-var} DOT
        DP_SCOPE,  // DOT... {scope-or-var} DOT
        DP_FINAL,  // [DOT...] {var-or-func-or-dtype} with no following dots
        DP_MEMBER  // DOT {member-name} [DOT...]
    };

    // STATE
    LinkDotState* സ്ഥിരം m_statep;  // State, including dotted symbol table
    VSymEnt* m_curSymp = nullptr;  // SymEnt for current lookup point
    VSymEnt* m_modSymp = nullptr;  // SymEnt for current module
    VSymEnt* m_pinSymp = nullptr;  // SymEnt for pin lookups
    VSymEnt* m_randSymp = nullptr;  // SymEnt for randomize target class's lookups
    സ്ഥിരം AstCell* m_cellp = nullptr;  // Current cell
    Ast_കണ്ണി_Module* m_modp = nullptr;  // Current module
    Ast_കണ്ണി_FTask* m_ftaskp = nullptr;  // Current function/task
    AstMethodCall* m_randMethodCallp = nullptr;  // Current randomize() method call
    പൂർണ്ണസംഖ്യ m_modport_അക്കം = 0;  // Uniqueify modport numbers
    പൂർണ്ണസംഖ്യ m_indent = 0;  // Indentation (tree depth) for debug
    തർക്കം m_inSens = അസത്യം;  // True if in senitem
    സ്ഥിരം AstWith* m_currentWithp = nullptr;  // Enclosing 'with' (nullptr if none)
    std::set<std::string>
        m_restrictedNamesUsed;  // Names from current 'with (id_list)' that resolved into target
    തർക്കം m_genericIfaceModule = അസത്യം;  // True if in module containing generic interface
    std::map<std::string, Ast_കണ്ണി*> m_ifClassImpNames;  // Names imported from interface class
    // Inherited interface implementations needing a forward in the current class, by name
    std::map<std::string, സ്ഥിരം VSymEnt*> m_ifForwards;
    std::set<AstClass*> m_extendsParam;  // Classes that have a parameterized super class
                                         // (except the default instances)
                                         // They are added to the set only in linkDotPrimary.
    തർക്കം m_insideClassExtParam = അസത്യം;  // Inside a class from m_extendsParam
    AstNew* m_explicitSuperNewp = nullptr;  // Hit a "super.new" call inside a "new" function
    std::map<Ast_കണ്ണി*, AstPin*> m_usedPins;  // Pin used in this cell, map to duplicate
    std::map<Ast_കണ്ണി*, std::map<std::string, AstPin*>> m_usedDefParamPins;
    // Defparam pins used for a given formal, by hierarchical path
    std::map<std::string, Ast_കണ്ണി_Module*> m_modulesToRevisit;  // Modules to revisit a second time
    Ast_കണ്ണി* m_lastDeferredp = nullptr;  // Last node which requested a revisit of its module
    Ast_കണ്ണി_DType* m_packedArrayDtp = nullptr;  // Datatype reference for packed array
    തർക്കം m_inPackedArray = അസത്യം;  // Currently traversing a packed array tree
    തർക്കം m_replaceWithAlias
        = സത്യം;  // Replace VarScope with an alias. Used in the handling of AstAlias
    തർക്കം m_isParam = അസത്യം;  // Specifies whether currently visiting param variable
    തർക്കം m_resolvingTypedef = അസത്യം;  // Currently traversing a Typedef tree

    ഘടന DotStates final {
        DotPosition m_dotPos;  // Scope part of dotted resolution
        VSymEnt* m_dotSymp;  // SymEnt for dotted AstParse lookup
        സ്ഥിരം AstDot* m_dotp;  // Current dot
        തർക്കം m_super;  // Starts with super reference
        തർക്കം m_unresolvedCell;  // Unresolved cell, needs help from V3Param
        തർക്കം m_unresolvedClass;  // Unresolved class reference, needs help from V3Param
        തർക്കം m_unresolvedGenericIface;  // Unresolved generic interface, needs help from V3Param
        തർക്കം m_genBlk;  // Contains gen block reference
        Ast_കണ്ണി* m_unlinkedScopep;  // Unresolved scope, needs corresponding VarXRef
        AstDisable* m_disablep;  // Disable statement under which the reference is
        തർക്കം m_dotErr;  // Error found in dotted resolution, ignore upwards
        string m_dotText;  // String of dotted names found in below parseref
        DotStates() { init(nullptr); }
        ~DotStates() = സ്ഥിരസ്ഥിതി;
        ശൂന്യം init(VSymEnt* curSymp, തർക്കം unresolvedClass = അസത്യം) {
            m_dotPos = DP_NONE;
            m_dotSymp = curSymp;
            m_dotp = nullptr;
            m_super = അസത്യം;
            m_dotErr = അസത്യം;
            m_dotText = "";
            m_unresolvedCell = അസത്യം;
            m_unresolvedClass = unresolvedClass;
            m_unresolvedGenericIface = അസത്യം;
            m_genBlk = അസത്യം;
            m_unlinkedScopep = nullptr;
            m_disablep = nullptr;
        }
        string ascii() സ്ഥിരം {
            സ്ഥിതം സ്ഥിരം അക്ഷരം* സ്ഥിരം names[]
                = {"DP_NONE", "DP_PACKAGE", "DP_FIRST", "DP_SCOPE", "DP_FINAL", "DP_MEMBER"};
            std::ostringstream sstr;
            sstr << "ds=" << names[m_dotPos];
            sstr << "  dse" << cvtToHex(m_dotSymp);
            സ്ഥിരം string dsname = m_dotSymp->nodep()->name().substr(0, 8);
            sstr << "(" << m_dotSymp->nodep()->typeName() << (dsname.empty() ? "" : ":") << dsname
                 << ")";
            എങ്കിൽ (m_dotErr) sstr << "  [dotErr]";
            എങ്കിൽ (m_super) sstr << "  [super]";
            എങ്കിൽ (m_unresolvedCell) sstr << "  [unrCell]";
            എങ്കിൽ (m_unresolvedClass) sstr << "  [unrClass]";
            എങ്കിൽ (m_unresolvedGenericIface) sstr << "  [unrGIface]";
            എങ്കിൽ (m_genBlk) sstr << "  [genBlk]";
            sstr << "  txt=" << m_dotText;
            തിരികെ sstr.str();
        }
    } m_ds;  // State to preserve across recursions

    // METHODS - Variables
    AstVar* createImplicitVar(VSymEnt* /*lookupSymp*/, AstParseRef* nodep, Ast_കണ്ണി_Module* modp,
                              VSymEnt* moduleSymp, തർക്കം noWarn) {
        // Create implicit after warning
        എങ്കിൽ (!noWarn) {
            എങ്കിൽ (nodep->fileline()->warnIsOff(V3ErrorCode::I_DEF_NETTYPE_WIRE)) {
                സ്ഥിരം string suggest = m_statep->suggestSymFallback(moduleSymp, nodep->name(),
                                                                    Link_കണ്ണി_Matcher_Var{});
                nodep->v3error("Signal definition not found, and implicit disabled with "
                               "`default_nettype: "
                               << nodep->prettyNameQ() << '\n'
                               << (suggest.empty() ? "" : nodep->warnMore() + suggest));

            }
            // Bypass looking for suggestions if IMPLICIT is turned off
            // as there could be thousands of these suppressed in large netlists
            അല്ലെങ്കിൽ എങ്കിൽ (!nodep->fileline()->warnIsOff(V3ErrorCode::IMPLICIT)) {
                സ്ഥിരം string suggest = m_statep->suggestSymFallback(moduleSymp, nodep->name(),
                                                                    Link_കണ്ണി_Matcher_Var{});
                nodep->v3warn(IMPLICIT,
                              "Signal definition not found, creating implicitly: "
                                  << nodep->prettyNameQ() << '\n'
                                  << (suggest.empty() ? "" : nodep->warnMore() + suggest));
            }
        }
        AstVar* സ്ഥിരം newp
            = പുതിയ AstVar{nodep->fileline(), VVarType::WIRE, nodep->name(), VFlagLogicPacked{}, 1};
        newp->lifetime(VLifetime::STATIC_IMPLICIT);
        newp->trace(modp->modTrace());
        modp->addStmtsp(newp);
        // Link it to signal list, must add the variable under the module;
        // current scope might be lower now
        m_statep->തിരുകുക_Sym(moduleSymp, newp->name(), newp, nullptr /*classOrPackagep*/);
        തിരികെ newp;
    }
    AstVar* foundToVarp(സ്ഥിരം VSymEnt* symp, Ast_കണ്ണി* nodep, VAccess access) {
        // Return a variable if possible, auto converting a modport to variable
        എങ്കിൽ (!symp) {
            തിരികെ nullptr;
        } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(symp->nodep(), Var)) {
            തിരികെ VN_AS(symp->nodep(), Var);
        } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(symp->nodep(), ModportVarRef)) {
            AstModportVarRef* സ്ഥിരം snodep = VN_AS(symp->nodep(), ModportVarRef);
            AstVar* സ്ഥിരം varp = snodep->varp();
            എങ്കിൽ (access.isWriteOrRW() && snodep->direction().isReadOnly()) {
                nodep->v3error("Attempt to drive input-only modport: " << nodep->prettyNameQ());
            }  // else other simulators don't warn about reading, and IEEE doesn't say illegal
            തിരികെ varp;
        } അല്ലെങ്കിൽ {
            തിരികെ nullptr;
        }
    }

    // Look up a virtual port through modport expression mapping.
    // When a dotted reference uses a modport with expression syntax, the virtual port name
    // maps to the real signal. This helper resolves that mapping during scope creation.
    // Returns the VarScope for the real signal, or nullptr if not found.
    AstVarScope* findVirtualPortVarScope(VSymEnt* dotSymp, സ്ഥിരം string& portName) {
        സ്ഥിരം AstVarScope* സ്ഥിരം dotVscp = VN_CAST(dotSymp->nodep(), VarScope);
        സ്ഥിരം AstVar* സ്ഥിരം dotVarp = dotVscp ? dotVscp->varp() : nullptr;
        എങ്കിൽ (!dotVarp) തിരികെ nullptr;

        Ast_കണ്ണി_DType* dtypep = dotVarp->childDTypep();
        എങ്കിൽ (!dtypep) dtypep = dotVarp->subDTypep();
        എങ്കിൽ (!dtypep) തിരികെ nullptr;
        സ്ഥിരം AstIfaceRefDType* സ്ഥിരം ifaceRefp = VN_CAST(dtypep, IfaceRefDType);
        എങ്കിൽ (!ifaceRefp || !ifaceRefp->modportp() || !ifaceRefp->ifaceViaCellp()) തിരികെ nullptr;

        // Get the interface cell's symbol table
        VSymEnt* സ്ഥിരം ifaceCellSymp = m_statep->get_കണ്ണി_Sym(ifaceRefp->ifaceViaCellp());

        // Look up modport by name in interface cell
        VSymEnt* സ്ഥിരം modportSymp = ifaceCellSymp->findIdFallback(ifaceRefp->modportp()->name());
        എങ്കിൽ (!modportSymp) തിരികെ nullptr;

        // Look up virtual port name in modport symbol table
        string baddot;
        VSymEnt* സ്ഥിരം virtPortp = m_statep->findSymPrefixed(modportSymp, portName, baddot, സത്യം);
        എങ്കിൽ (!virtPortp) തിരികെ nullptr;

        // Symbol may point to VarScope or Var
        എങ്കിൽ (AstVarScope* vscp = VN_CAST(virtPortp->nodep(), VarScope)) തിരികെ vscp;

        // Symbol points to Var - look up VarScope by name in interface cell
        എങ്കിൽ (സ്ഥിരം AstVar* സ്ഥിരം realVarp = VN_CAST(virtPortp->nodep(), Var)) {
            VSymEnt* സ്ഥിരം realFoundp
                = m_statep->findSymPrefixed(ifaceCellSymp, realVarp->name(), baddot, സത്യം);
            തിരികെ realFoundp ? VN_CAST(realFoundp->nodep(), VarScope) : nullptr;
        }
        തിരികെ nullptr;
    }

    // Capture a ParamTypeDType reference for interface typedef retargeting.
    // Called when a RefDType resolves to a ParamTypeDType owned by an interface.
    ശൂന്യം captureIfaceParamType(AstRefDType* nodep, AstParamTypeDType* defp) {
        എങ്കിൽ (!V3LinkDotIfaceCapture::enabled() || !m_statep->forPrimary()) തിരികെ;
        Ast_കണ്ണി_Module* സ്ഥിരം defOwnerModp = V3LinkDotIfaceCapture::findOwnerModule(defp);
        എങ്കിൽ (!defOwnerModp || !VN_IS(defOwnerModp, Iface)) തിരികെ;
        AstCell* സ്ഥിരം cellForCapture
            = m_ds.m_dotSymp ? VN_CAST(m_ds.m_dotSymp->nodep(), Cell) : nullptr;
        എങ്കിൽ (!cellForCapture) തിരികെ;
        UINFO(9, indent() << "iface capture add paramtype " << nodep
                          << " iface=" << defOwnerModp->prettyNameQ());
        V3LinkDotIfaceCapture::addParamType(nodep, cellForCapture->name(), m_modp, defp,
                                            defOwnerModp->name());
    }

    Ast_കണ്ണി_Stmt* addImplicitSuperNewCall(AstFunc* സ്ഥിരം nodep,
                                         സ്ഥിരം AstClassExtends* സ്ഥിരം classExtendsp) {
        // Returns the added node
        File_വരി* സ്ഥിരം fl = nodep->fileline();
        AstArg* argsp = nullptr;
        എങ്കിൽ (classExtendsp->argsp()) argsp = classExtendsp->argsp()->clone_വൃക്ഷം(സത്യം);
        AstNew* സ്ഥിരം newExprp = പുതിയ AstNew{fl, argsp};
        newExprp->isImplicit(സത്യം);
        AstDot* സ്ഥിരം superNewp = പുതിയ AstDot{fl, അസത്യം, പുതിയ AstParseRef{fl, "super"}, newExprp};
        Ast_കണ്ണി_Stmt* സ്ഥിരം superNewStmtp = superNewp->makeStmt();
        വേണ്ടി (Ast_കണ്ണി* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            // super.new shall be the first statement (IEEE 1800-2023 8.15)
            // but some nodes (such as variable declarations and typedefs) should stay before
            എങ്കിൽ (VN_IS(stmtp, കണ്ണി_Stmt)) {
                stmtp->addHereThisAsNext(superNewStmtp);
                തിരികെ superNewStmtp;
            }
        }
        // There were no statements
        nodep->addStmtsp(superNewStmtp);
        തിരികെ superNewStmtp;
    }
    ശൂന്യം checkNoDot(Ast_കണ്ണി* nodep) {
        എങ്കിൽ (VL_UNLIKELY(!nodep)) {
            UINFO(9, indent() << "iface capture null node passed to checkNoDot; dot state="
                              << m_ds.ascii());
            m_ds.m_dotErr = സത്യം;
            തിരികെ;
        }
        എങ്കിൽ (VL_UNLIKELY(m_ds.m_dotPos != DP_NONE)) {
            UINFO(9, indent() << "ds=" << m_ds.ascii());
            UINFO(9, indent() << "iface capture checkNoDot hit node=" << nodep
                              << " dotState=" << m_ds.ascii());
            എങ്കിൽ (VL_UNLIKELY(!m_ds.m_dotp)) {
                nodep->v3error("Syntax error: Not expecting " << nodep->type()
                                                              << " here (missing dot context)");
            } അല്ലെങ്കിൽ {
                nodep->v3error("Syntax error: Not expecting "
                               << nodep->type() << " under a " << nodep->backp()->type()
                               << " in dotted expression\n"
                               << nodep->warnContextPrimary() << m_ds.m_dotp->warnOther()
                               << "... Resolving this reference\n"
                               << m_ds.m_dotp->warnContextSecondary());
            }
            m_ds.m_dotErr = സത്യം;
        }
    }
    AstVar* find_Iface_മുകൾ_Varp(Ast_കണ്ണി* nodep, VSymEnt* parentEntp, സ്ഥിരം string& name) {
        സ്ഥിരം string findName = name + "__Viftop";
        VSymEnt* സ്ഥിരം ifaceSymp = parentEntp->findIdFallback(findName);
        AstVar* സ്ഥിരം iface_മുകൾ_Varp = ifaceSymp ? VN_AS(ifaceSymp->nodep(), Var) : nullptr;
        UASSERT_OBJ(iface_മുകൾ_Varp, nodep, "Can't find interface var ref: " << findName);
        തിരികെ iface_മുകൾ_Varp;
    }
    AstRSProd* findProd(Ast_കണ്ണി* nodep, VSymEnt* parentEntp, സ്ഥിരം string& name) {
        സ്ഥിരം VSymEnt* സ്ഥിരം foundp = parentEntp->findIdFallback(name);
        AstRSProd* foundNodep = foundp ? VN_CAST(foundp->nodep(), RSProd) : nullptr;
        എങ്കിൽ (!foundNodep) {
            VSpellCheck speller;
            Link_കണ്ണി_Matcher_Prod matcher;
            parentEntp->candidateIdFlat(&speller, &matcher, അസത്യം);
            സ്ഥിരം string suggest = speller.best_Candidate_സന്ദേശം(name);
            UINFO(1, "   ErrParseRef curSymp=se" << cvtToHex(parentEntp));
            nodep->v3error("Production " << Ast_കണ്ണി::prettyNameQ(name) << " not found\n"
                                         << (suggest.empty() ? "" : nodep->warnMore() + suggest));
            തിരികെ nullptr;
        }
        തിരികെ foundNodep;
    }
    ശൂന്യം duplicatePinError(AstPin* nodep, Ast_കണ്ണി* origp, സ്ഥിരം അക്ഷരം* whatp) {
        nodep->v3error("Duplicate " << whatp << " connection: " << nodep->prettyNameQ() << '\n'
                                    << nodep->warnContextPrimary() << '\n'
                                    << origp->warnOther() << "... Location of original " << whatp
                                    << " connection\n"
                                    << origp->warnContextSecondary());
    }
    ശൂന്യം markAndCheckPinDup(AstPin* nodep, Ast_കണ്ണി* refp, സ്ഥിരം അക്ഷരം* whatp) {
        സ്ഥിരം auto pair = m_usedPins.emplace(refp, nodep);
        എങ്കിൽ (pair.second) {
            എങ്കിൽ (!nodep->paramPath().empty())
                m_usedDefParamPins[refp].emplace(nodep->paramPath(), nodep);
            തിരികെ;
        } അല്ലെങ്കിൽ {
            Ast_കണ്ണി* സ്ഥിരം origp = pair.first->second;
            എങ്കിൽ (nodep->paramPath().empty() || VN_AS(origp, Pin)->paramPath().empty()) {
                duplicatePinError(nodep, origp, whatp);
                തിരികെ;
            }
        }

        auto& defParamPins = m_usedDefParamPins[refp];
        സ്ഥിരം auto defParamIt = defParamPins.find(nodep->paramPath());
        എങ്കിൽ (defParamIt != defParamPins.end()) {
            duplicatePinError(nodep, defParamIt->second, whatp);
            തിരികെ;
        }
        defParamPins.emplace(nodep->paramPath(), nodep);
    }
    സ്ഥിതം തർക്കം extendsClass(സ്ഥിരം AstClass* classp, സ്ഥിരം AstClass* targetp) {
        // Return true if classp inherits from targetp through already resolved extends
        // If performance of this becomes a problem, use just a counter to check
        // exceeds some number of iterations then perform the more expensive analysis
        std::set<സ്ഥിരം AstClass*> visited;
        std::vector<സ്ഥിരം AstClass*> todo{classp};
        ആകുമ്പോൾ (!todo.empty()) {
            സ്ഥിരം AstClass* സ്ഥിരം currp = todo.back();
            todo.എടുക്കുക_back();
            എങ്കിൽ (currp == targetp) തിരികെ സത്യം;
            എങ്കിൽ (!visited.തിരുകുക(currp).second) തുടരുക;
            വേണ്ടി (സ്ഥിരം AstClassExtends* cextp = currp->extendsp(); cextp;
                 cextp = VN_AS(cextp->nextp(), ClassExtends)) {
                എങ്കിൽ (സ്ഥിരം AstClass* സ്ഥിരം basep = cextp->classOrNullp()) todo.തള്ളുക_back(basep);
            }
        }
        തിരികെ അസത്യം;
    }
    സ്ഥിതം AstClocking* sensClockingp(Ast_കണ്ണി* nodep) {
        // Return the clocking block referenced by nodep, either directly or through a modport
        എങ്കിൽ (AstClocking* സ്ഥിരം clockingp = VN_CAST(nodep, Clocking)) തിരികെ clockingp;
        എങ്കിൽ (സ്ഥിരം AstModportClockingRef* സ്ഥിരം clockingRefp = VN_CAST(nodep, ModportClockingRef))
            തിരികെ clockingRefp->clockingp();
        തിരികെ nullptr;
    }
    VSymEnt* getCreateClockingEventSymEnt(AstClocking* clockingp) {
        AstVar* സ്ഥിരം eventp = clockingp->ensureEventp(സത്യം);
        എങ്കിൽ (!eventp->user1p()) eventp->user1p(പുതിയ VSymEnt{m_statep->symsp(), eventp});
        തിരികെ reinterpret_cast<VSymEnt*>(eventp->user1p());
    }
    VSymEnt* findInlinedDotsSym(Ast_കണ്ണി* nodep, സ്ഥിരം string& inlinedDots) {
        // Return scope that applies for searching, given renaming due to module inlining.
        // Dotted lookup is always relative to module, as maybe variable
        // name lower down with same scope name we want to ignore (t_math_divw)
        VSymEnt* dotSymp = m_modSymp;
        സ്ഥിരം string inl = Ast_കണ്ണി::dedotName(inlinedDots);
        string baddot;
        VSymEnt* okSymp = nullptr;
        dotSymp = m_statep->findDotted(nodep->fileline(), dotSymp, inl, baddot, okSymp, അസത്യം);
        UASSERT_OBJ(dotSymp, nodep,
                    "Couldn't resolve inlined scope " << Ast_കണ്ണി::prettyNameQ(baddot)
                                                      << " in: " << inlinedDots);
        UINFO(9, indent() << "findInlinedDotsSym " << dotSymp
                          << " search start due to inlinedDots=" << inlinedDots);
        തിരികെ dotSymp;
    }
    VSymEnt* findDottedRefSymp(Ast_കണ്ണി* nodep, VSymEnt* lookupSymp, സ്ഥിരം string& dotted,
                               string& baddot, VSymEnt*& okSymp, തർക്കം* modportp = nullptr) {
        // Return scope to search for a relinked hierarchical reference
        // (AstVarXRef or AstEnumItemRef), or nullptr after error if not found
        // If modportp is given, set true when the reference went through a modport
        എങ്കിൽ (modportp) *modportp = അസത്യം;
        VSymEnt* dotSymp = m_statep->findDotted(nodep->fileline(), lookupSymp, dotted, baddot,
                                                okSymp, സത്യം);  // Maybe nullptr
        എങ്കിൽ (!dotSymp) {
            nodep->v3error(
                "Can't find definition of "
                << (!baddot.empty() ? Ast_കണ്ണി::prettyNameQ(baddot) : nodep->prettyNameQ()) << '\n'
                << nodep->warnContextPrimary());
            തിരികെ nullptr;
        }
        എങ്കിൽ (സ്ഥിരം AstVar* സ്ഥിരം varp = VN_CAST(dotSymp->nodep(), Var)) {
            എങ്കിൽ (സ്ഥിരം AstIfaceRefDType* സ്ഥിരം ifaceRefp
                = VN_CAST(varp->childDTypep(), IfaceRefDType)) {
                എങ്കിൽ (ifaceRefp->modportp()) {
                    dotSymp = m_statep->get_കണ്ണി_Sym(ifaceRefp->modportp());
                    എങ്കിൽ (modportp) *modportp = സത്യം;
                } അല്ലെങ്കിൽ {
                    dotSymp = m_statep->get_കണ്ണി_Sym(ifaceRefp->ifacep());
                }
            }
        } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstModportClockingRef* സ്ഥിരം clockingRefp
                   = VN_CAST(dotSymp->nodep(), ModportClockingRef)) {
            dotSymp = m_statep->get_കണ്ണി_Sym(clockingRefp->clockingp());
        }
        തിരികെ dotSymp;
    }

    സ്ഥിതം തർക്കം isParamedClassRefDType(സ്ഥിരം Ast_കണ്ണി* classp) {
        ആകുമ്പോൾ (സ്ഥിരം AstRefDType* സ്ഥിരം refp = VN_CAST(classp, RefDType))
            classp = refp->subDTypep();
        തിരികെ (VN_IS(classp, ClassRefDType) && VN_AS(classp, ClassRefDType)->paramsp())
               || VN_IS(classp, ParamTypeDType);
    }
    തർക്കം isParamedClassRef(സ്ഥിരം Ast_കണ്ണി* nodep) {
        // Is this a parameterized reference to a class, or a reference to class parameter
        എങ്കിൽ (സ്ഥിരം auto* classRefp = VN_CAST(nodep, ClassOrPackageRef)) {
            എങ്കിൽ (classRefp->paramsp()) തിരികെ സത്യം;
            സ്ഥിരം auto* classp = classRefp->classOrPackageNodep();
            ആകുമ്പോൾ (സ്ഥിരം auto* typedefp = VN_CAST(classp, Typedef)) classp = typedefp->subDTypep();
            തിരികെ isParamedClassRefDType(classp);
        }
        തിരികെ isParamedClassRefDType(nodep);
    }
    VSymEnt* getThisClassSymp() {
        VSymEnt* classSymp = m_ds.m_dotSymp;
        ആകുമ്പോൾ (classSymp && !VN_IS(classSymp->nodep(), Class)) {
            classSymp = classSymp->parentp();
        }
        തിരികെ classSymp;
    }
    // Is the current scope within the given class, including within its nested classes
    തർക്കം insideClass(സ്ഥിരം AstClass* classp) സ്ഥിരം {
        വേണ്ടി (സ്ഥിരം VSymEnt* symp = m_curSymp; symp; symp = symp->parentp()) {
            എങ്കിൽ (symp->nodep() == classp) തിരികെ സത്യം;
        }
        തിരികെ അസത്യം;
    }
    // 'randomize() with' target class not yet specialized; defer to post-V3Param pass
    തർക്കം randTargetUnresolved() സ്ഥിരം {
        തിരികെ m_statep->forPrimary() && m_currentWithp && m_randMethodCallp && !m_randSymp;
    }
    // Is 'funcp' of 'classp' virtual, directly or by overriding a virtual method of a base
    // class (IEEE 1800-2023 8.20)
    തർക്കം isVirtualMethod(സ്ഥിരം Ast_കണ്ണി_FTask* funcp, സ്ഥിരം AstClass* classp) സ്ഥിരം {
        എങ്കിൽ (funcp->isVirtual()) തിരികെ സത്യം;
        വേണ്ടി (സ്ഥിരം AstClassExtends* cextp = classp->extendsp(); cextp;
             cextp = VN_AS(cextp->nextp(), ClassExtends)) {
            AstClass* സ്ഥിരം basep = cextp->classOrNullp();
            എങ്കിൽ (!basep) തുടരുക;
            സ്ഥിരം VSymEnt* സ്ഥിരം foundp = m_statep->get_കണ്ണി_Sym(basep)->findIdFlat(funcp->name());
            സ്ഥിരം Ast_കണ്ണി_FTask* സ്ഥിരം baseFuncp
                = foundp ? VN_CAST(foundp->nodep(), കണ്ണി_FTask) : nullptr;
            സ്ഥിരം AstClass* സ്ഥിരം baseClassp = foundp && foundp->parentp()
                                                   ? VN_CAST(foundp->parentp()->nodep(), Class)
                                                   : nullptr;
            എങ്കിൽ (baseFuncp && baseClassp && isVirtualMethod(baseFuncp, baseClassp)) തിരികെ സത്യം;
        }
        തിരികെ അസത്യം;
    }
    // Is imported 'implp' an implementation of interface method 'ifSymp' (IEEE 1800-2023
    // 8.26.2)? If its class does not derive from the interface class, it needs a forwarding
    // override in the derived class to be reachable through the interface.
    തർക്കം inheritedImplements(സ്ഥിരം VSymEnt* implp, സ്ഥിരം VSymEnt* ifSymp, തർക്കം& needForwardr) {
        needForwardr = അസത്യം;
        സ്ഥിരം Ast_കണ്ണി_FTask* സ്ഥിരം implFuncp = VN_CAST(implp->nodep(), കണ്ണി_FTask);
        എങ്കിൽ (!implFuncp || implFuncp->pureVirtual() || implFuncp->isStatic()) തിരികെ അസത്യം;
        സ്ഥിരം AstClass* സ്ഥിരം implClassp
            = implp->parentp() ? VN_CAST(implp->parentp()->nodep(), Class) : nullptr;
        സ്ഥിരം AstClass* സ്ഥിരം ifClassp
            = ifSymp->parentp() ? VN_CAST(ifSymp->parentp()->nodep(), Class) : nullptr;
        എങ്കിൽ (!implClassp || !ifClassp) തിരികെ അസത്യം;
        എങ്കിൽ (extendsClass(implClassp, ifClassp)) തിരികെ സത്യം;
        എങ്കിൽ (!isVirtualMethod(implFuncp, implClassp)) തിരികെ അസത്യം;
        needForwardr = സത്യം;
        തിരികെ സത്യം;
    }
    // Add 'implp' override to 'classp' that calls super.implp, so the C++ method of an
    // implemented interface class is overridden
    ശൂന്യം addInterfaceForward(AstClass* classp, Ast_കണ്ണി_FTask* implp, AstClass* implClassp) {
        File_വരി* സ്ഥിരം flp = implp->fileline();
        AstVar* fvarp = nullptr;
        Ast_കണ്ണി_FTask* newp;
        എങ്കിൽ (AstFunc* സ്ഥിരം implFuncp = VN_CAST(implp, Func)) {
            fvarp = VN_AS(implFuncp->fvarp(), Var)->clone_വൃക്ഷം(അസത്യം);
            newp = പുതിയ AstFunc{flp, implp->name(), nullptr, fvarp};
        } അല്ലെങ്കിൽ {
            newp = പുതിയ AstTask{flp, implp->name(), nullptr};
        }
        newp->classMethod(സത്യം);
        newp->isVirtual(സത്യം);
        newp->lifetime(implp->lifetime());
        AstArg* argsp = nullptr;
        വേണ്ടി (Ast_കണ്ണി* stmtp = implp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstVar* സ്ഥിരം portp = VN_CAST(stmtp, Var);
            എങ്കിൽ (!portp || !portp->isIO()) തുടരുക;
            AstVar* സ്ഥിരം newPortp = portp->clone_വൃക്ഷം(അസത്യം);
            newp->addStmtsp(newPortp);
            argsp = Ast_കണ്ണി::addNext(
                argsp, പുതിയ AstArg{flp, "", പുതിയ AstVarRef{flp, newPortp, VAccess::READ}});
        }
        Ast_കണ്ണി_FTask_Ref* callp;
        എങ്കിൽ (AstFunc* സ്ഥിരം implFuncp = VN_CAST(implp, Func)) {
            callp = പുതിയ AstFuncRef{flp, implFuncp, argsp};
        } അല്ലെങ്കിൽ {
            callp = പുതിയ AstTaskRef{flp, VN_AS(implp, Task), argsp};
        }
        callp->classOrPackagep(implClassp);
        callp->superReference(സത്യം);
        എങ്കിൽ (fvarp) {
            newp->addStmtsp(പുതിയ AstAssign{flp, പുതിയ AstVarRef{flp, fvarp, VAccess::WRITE}, callp});
        } അല്ലെങ്കിൽ {
            newp->addStmtsp(callp->makeStmt());
        }
        UINFO(9, indent() << "Interface forward " << newp);
        classp->addMembersp(newp);
    }
    ശൂന്യം importDerivedClass(AstClass* derivedClassp, VSymEnt* baseSymp, AstClass* baseClassp) {
        // Also used for standard 'extends' from a base class
        UINFO(8, indent() << "importDerivedClass to " << derivedClassp << " from " << baseClassp);
        വേണ്ടി (VSymEnt::const_iterator it = baseSymp->begin(); it != baseSymp->end(); ++it) {
            എങ്കിൽ (Ast_കണ്ണി* baseSubp = it->second->nodep()) {
                UINFO(8, indent() << "  SymFunc " << baseSubp);
                സ്ഥിരം string impOrExtends
                    = baseClassp->isInterfaceClass() ? " implements " : " extends ";
                എങ്കിൽ (VN_IS(baseSubp, കണ്ണി_FTask)) {
                    സ്ഥിരം VSymEnt* സ്ഥിരം foundp = m_curSymp->findIdFlat(baseSubp->name());
                    സ്ഥിരം Ast_കണ്ണി_FTask* സ്ഥിരം baseFuncp = VN_CAST(baseSubp, കണ്ണി_FTask);
                    എങ്കിൽ (!baseFuncp || !baseFuncp->pureVirtual()) തുടരുക;
                    തർക്കം needForward = അസത്യം;
                    സ്ഥിരം തർക്കം existsInDerived
                        = foundp
                          && (!foundp->imported()
                              || inheritedImplements(foundp, it->second, needForward));
                    എങ്കിൽ (needForward && m_statep->forPrimary()
                        && !derivedClassp->isInterfaceClass()) {
                        m_ifForwards.emplace(baseSubp->name(), foundp);
                    }
                    എങ്കിൽ (m_statep->forPrimary() && !existsInDerived
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
                    സ്ഥിരം auto itn = m_ifClassImpNames.find(baseSubp->name());
                    എങ്കിൽ (m_statep->forPrimary() && !existsInDerived
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
                എങ്കിൽ (VN_IS(baseSubp, Constraint)) {
                    സ്ഥിരം VSymEnt* സ്ഥിരം foundp = m_curSymp->findIdFlat(baseSubp->name());
                    സ്ഥിരം AstConstraint* സ്ഥിരം baseFuncp = VN_CAST(baseSubp, Constraint);
                    എങ്കിൽ (!baseFuncp || !baseFuncp->isKwdPure()) തുടരുക;
                    സ്ഥിരം തർക്കം existsInDerived = foundp && !foundp->imported();
                    എങ്കിൽ (m_statep->forPrimary() && !existsInDerived
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
    ശൂന്യം importSymbolsFromExtended(AstClass* സ്ഥിരം nodep, AstClassExtends* സ്ഥിരം cextp) {
        AstClass* സ്ഥിരം baseClassp = cextp->classp();
        VSymEnt* സ്ഥിരം srcp = m_statep->get_കണ്ണി_Sym(baseClassp);
        importDerivedClass(nodep, srcp, baseClassp);
        എങ്കിൽ (!cextp->isImplements()) m_curSymp->importFromClass(m_statep->symsp(), srcp);
    }
    ശൂന്യം classExtendImport(AstClass* nodep) {
        // A class reference might be to a class that is later in Ast due to
        // e.g. parameterization or referring to a "class (type T) extends T"
        // Resolve it so later Class:: references into its base classes work
        symIterateNull(nodep, m_statep->get_കണ്ണി_Sym(nodep));
    }

    ശൂന്യം checkDeclOrder(Ast_കണ്ണി* nodep, Ast_കണ്ണി* declp) {
        സ്ഥിരം uint32_t decl_Token_അക്കം = declp->decl_Token_അക്കം();
        എങ്കിൽ (!decl_Token_അക്കം) തിരികെ;  // Not implemented/valid on this object
        എങ്കിൽ (nodep->fileline()->token_അക്കം() < decl_Token_അക്കം) {
            UINFO(1, "Related node " << nodep->fileline()->token_അക്കം() << " " << nodep);
            UINFO(1, "Related decl " << decl_Token_അക്കം << " " << declp);
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
    ശൂന്യം checkMemberDeclOrder(Ast_കണ്ണി* nodep, AstMemberDType* declp) {
        സ്ഥിരം uint32_t decl_Token_അക്കം = declp->fileline()->token_അക്കം();
        എങ്കിൽ (nodep->fileline()->token_അക്കം() < decl_Token_അക്കം) {
            UINFO(1, "Related node " << nodep->fileline()->token_അക്കം() << " " << nodep);
            UINFO(1, "Related decl " << decl_Token_അക്കം << " " << declp);
            nodep->v3error("Reference to "
                           << nodep->prettyNameQ() << " before declaration (IEEE 1800-2023 6.18)\n"
                           << nodep->warnMore()
                           << "... Suggest move the declaration before the reference\n"
                           << nodep->warnContextPrimary() << '\n'
                           << declp->warnOther() << "... Location of original declaration\n"
                           << declp->warnContextSecondary());
        }
    }

    ശൂന്യം replaceWithCheckBreak(Ast_കണ്ണി* oldp, Ast_കണ്ണി_DType* newp) {
        // Flag now to avoid V3Broken throwing an internal error
        എങ്കിൽ (oldp->wouldBreak(newp)) {
            newp->v3error(
                "Data type used where a non-data type is expected: " << newp->prettyNameQ());
            oldp->replaceWith(പുതിയ AstConst{newp->fileline(), AstConst::BitFalseErroring{}});
        } അല്ലെങ്കിൽ {
            oldp->replaceWith(newp);
        }
    }

    // Marks the current module to be revisited after the initial AST iteration
    ശൂന്യം revisitLater(Ast_കണ്ണി* deferredNodep) {
        // Need to revisit entire module to build up all the necessary context
        m_lastDeferredp = deferredNodep;
        m_modulesToRevisit.തിരുകുക(std::make_pair(m_modp->name(), m_modp));
    }

    ശൂന്യം updateVarUse(AstVar* nodep) {
        // Avoid dotted.PARAM false positive when in a parameter block
        // that is if ()'ed off by same dotted name as another block
        എങ്കിൽ (nodep && nodep->isParam()) nodep->usedParam(സത്യം);
    }

    സ്ഥിതം VSymEnt* findIdFallbackSkipMemberDType(VSymEnt* lookp, സ്ഥിരം string& name) {
        VSymEnt* shadowEntp = nullptr;  // Shadowing variable: not a type, kept for error report
        ആകുമ്പോൾ (lookp) {
            VSymEnt* സ്ഥിരം foundp = lookp->findIdFlat(name);
            എങ്കിൽ (foundp && !VN_IS(foundp->nodep(), MemberDType)) {
                // Non-type entries are not type candidates
                // (IEEE 1800-2023 6.18); skip them so an enclosing
                // type is found, but keep one to preserve the "found: ..." error.
                എങ്കിൽ (VN_IS(foundp->nodep(), Typedef) || VN_IS(foundp->nodep(), ParamTypeDType)
                    || VN_IS(foundp->nodep(), Class)) {
                    തിരികെ foundp;
                }
                എങ്കിൽ (!shadowEntp) shadowEntp = foundp;
            }
            lookp = lookp->fallbackp();
        }
        തിരികെ shadowEntp;
    }

    ശൂന്യം symIterateChildren(Ast_കണ്ണി* nodep, VSymEnt* symp) {
        // Iterate children, changing to given context, with restore to old context
        VL_RESTORER_COPY(m_ds);
        VL_RESTORER(m_curSymp);
        m_curSymp = symp;
        m_ds.init(m_curSymp);
        iterateChildren(nodep);
    }
    ശൂന്യം symIterateNull(Ast_കണ്ണി* nodep, VSymEnt* symp) {
        // Iterate node, changing to given context, with restore to old context
        VL_RESTORER_COPY(m_ds);
        VL_RESTORER(m_curSymp);
        m_curSymp = symp;
        m_ds.init(m_curSymp);
        iterateNull(nodep);
    }
    സ്ഥിതം സ്ഥിരം Ast_കണ്ണി_DType* get_വ്യഞ്ജകം_DTypep(സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* selp) {
        ആകുമ്പോൾ (സ്ഥിരം Ast_കണ്ണി_Pre_Sel* സ്ഥിരം sp = VN_CAST(selp, കണ്ണി_Pre_Sel)) selp = sp->fromp();
        എങ്കിൽ (സ്ഥിരം AstMemberSel* സ്ഥിരം sp = VN_CAST(selp, MemberSel)) {
            എങ്കിൽ (സ്ഥിരം Ast_കണ്ണി_DType* dtypep = get_വ്യഞ്ജകം_DTypep(sp->fromp())) {
                എങ്കിൽ (സ്ഥിരം AstClassRefDType* സ്ഥിരം classRefp = VN_CAST(dtypep, ClassRefDType)) {
                    സ്ഥിരം AstClass* സ്ഥിരം classp = classRefp->classp();
                    സ്ഥിരം തർക്കം found = classp->existsMember(
                        [&dtypep, name = selp->name()](സ്ഥിരം AstClass*, സ്ഥിരം AstVar* nodep) {
                            dtypep = nodep->childDTypep();
                            തിരികെ nodep->name() == name;
                        });
                    എങ്കിൽ (found) തിരികെ dtypep->elemDTypep();
                    selp->v3error("Class " << classRefp->prettyNameQ()
                                           << " does not contain field " << selp->prettyNameQ());
                } അല്ലെങ്കിൽ {
                    selp->v3fatalSrc("Member selection on expression of type "
                                     << dtypep->prettyDTypeNameQ()
                                     << ", which is not a class type");
                }
            }
        } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം Ast_കണ്ണി_Var_Ref* സ്ഥിരം varRefp = VN_CAST(selp, കണ്ണി_Var_Ref)) {
            തിരികെ varRefp->varp()->childDTypep()->elemDTypep();
        }
        തിരികെ nullptr;
    }
    സ്ഥിതം സ്ഥിരം AstVar* getNextVarp(സ്ഥിരം Ast_കണ്ണി* stmtsp) {
        // Only IO ports, as parameters are on paramsp(), not the pinsp() list paired here
        ആകുമ്പോൾ (stmtsp) {
            എങ്കിൽ (സ്ഥിരം AstVar* സ്ഥിരം varp = VN_CAST(stmtsp, Var)) {
                എങ്കിൽ (varp->isIO()) തിരികെ varp;
            }
            stmtsp = stmtsp->nextp();
        }
        തിരികെ nullptr;
    }
    // Introduce implicit parameters for modules with generic interafeces
    ശൂന്യം addImplicitParametersOfGenericIface(AstCell* സ്ഥിരം nodep, സ്ഥിരം AstModule* സ്ഥിരം modp) {
        // Get Param number
        പൂർണ്ണസംഖ്യ param_അക്കം = 1;
        വേണ്ടി (AstPin* paramp = nodep->paramsp(); paramp; paramp = VN_CAST(paramp->nextp(), Pin)) {
            ++param_അക്കം;
        }

        // Add each implicit parameter type
        സ്ഥിരം AstVar* modIfaceVarp = getNextVarp(modp->stmtsp());
        വേണ്ടി (സ്ഥിരം AstPin* pinp = nodep->pinsp(); pinp && modIfaceVarp;
             pinp = VN_CAST(pinp->nextp(), Pin),
                           modIfaceVarp = getNextVarp(modIfaceVarp->nextp())) {
            എങ്കിൽ (modIfaceVarp->varType() != VVarType::IFACEREF
                || !VN_IS(modIfaceVarp->childDTypep()->elemDTypep(), IfaceGenericDType)) {
                തുടരുക;
            }
            Ast_കണ്ണി* exprp = pinp->exprp();
            എങ്കിൽ (!exprp) {
                modIfaceVarp->v3error("Interface port "
                                      << modIfaceVarp->prettyNameQ()
                                      << " is not connected to interface/modport pin expression");
                തുടരുക;
            }
            ആകുമ്പോൾ (സ്ഥിരം Ast_കണ്ണി_Pre_Sel* സ്ഥിരം preSelp = VN_CAST(exprp, കണ്ണി_Pre_Sel)) {
                exprp = preSelp->fromp();
            }
            // Resolve pin expression to the enclosing module's port Var. At primary
            // LinkDot the expression may still be an AstParseRef, so also look up by name.
            AstVar* enclosingVarp = nullptr;
            സ്ഥിരം AstVarRef* സ്ഥിരം varRefp = VN_CAST(exprp, VarRef);
            എങ്കിൽ (varRefp) {
                enclosingVarp = varRefp->varp();
            } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstParseRef* സ്ഥിരം parseRefp = VN_CAST(exprp, ParseRef)) {
                എങ്കിൽ (m_modp) {
                    എങ്കിൽ (VSymEnt* സ്ഥിരം symp
                        = m_statep->get_കണ്ണി_Sym(m_modp)->findIdFlat(parseRefp->name())) {
                        enclosingVarp = VN_CAST(symp->nodep(), Var);
                    }
                }
            }
            എങ്കിൽ (enclosingVarp && enclosingVarp->varType() == VVarType::IFACEREF
                && VN_IS(enclosingVarp->childDTypep()->skipRefp(), IfaceGenericDType)) {
                // Nested generic-iface forwarding (#7454): enclosing port is itself still
                // generic, so emit a placeholder __VGIfaceParam pin carrying a VarRef to
                // the outer port. V3Param rewrites it to the concrete IfaceRefDType once
                // the enclosing module is specialized (see
                // ParamProcessor::resolveGenericIfaceForwardingPins for the ordering
                // constraint that keeps the rewrite inside V3Param).
                AstVarRef* സ്ഥിരം fwdRefp
                    = പുതിയ AstVarRef{exprp->fileline(), enclosingVarp, VAccess::READ};
                AstPin* സ്ഥിരം newPinp = പുതിയ AstPin{
                    pinp->fileline(), param_അക്കം, "__VGIfaceParam" + modIfaceVarp->name(), fwdRefp};
                newPinp->param(സത്യം);
                visit(newPinp);
                nodep->addParamsp(newPinp);
            } അല്ലെങ്കിൽ എങ്കിൽ (varRefp) {
                സ്ഥിരം AstVar* സ്ഥിരം varp = varRefp->varp();
                എങ്കിൽ (സ്ഥിരം AstIfaceRefDType* സ്ഥിരം refp
                    = VN_CAST(varp->childDTypep()->elemDTypep(), IfaceRefDType)) {
                    AstIface* സ്ഥിരം ifacep = VN_AS(refp->cellp()->modp(), Iface);
                    AstIfaceRefDType* newIfaceRefp;
                    എങ്കിൽ (refp->modportp()) {
                        newIfaceRefp = പുതിയ AstIfaceRefDType{
                            refp->fileline(), refp->modportFileline(), m_modp->name(),
                            ifacep->name(), refp->modportName()};
                    } അല്ലെങ്കിൽ {
                        newIfaceRefp = പുതിയ AstIfaceRefDType{refp->fileline(), m_modp->name(),
                                                            ifacep->name()};
                    }
                    newIfaceRefp->ifacep(ifacep);
                    എങ്കിൽ (refp->cellp()->paramsp()) {
                        newIfaceRefp->addParamsp(refp->cellp()->paramsp()->clone_വൃക്ഷം(സത്യം));
                    }
                    UASSERT_OBJ(pinp->name().find("__pinNumber") == 0
                                    || pinp->name() == modIfaceVarp->name(),
                                pinp, "Not found interface with such name");
                    AstPin* സ്ഥിരം newPinp
                        = പുതിയ AstPin{pinp->fileline(), param_അക്കം,
                                     "__VGIfaceParam" + modIfaceVarp->name(), newIfaceRefp};
                    newPinp->param(സത്യം);
                    visit(newPinp);
                    nodep->addParamsp(newPinp);
                } അല്ലെങ്കിൽ {
                    varRefp->v3error("Generic interfaces can only connect to an interface and "
                                     << varp->prettyNameQ() << " is "
                                     << (varp->childDTypep()
                                             ? "of type " + varp->childDTypep()->prettyDTypeNameQ()
                                             : "not an interface"));
                }
            } അല്ലെങ്കിൽ {
                exprp->v3error("Expected an interface but " << exprp->prettyNameQ()
                                                            << " is not an interface");
            }
            ++param_അക്കം;
        }
    }

    // Returns true and issues error iff 'varp' cannot be accesed with a VarXRef
    // in a hier_block given by 'scopeSymp'
    തർക്കം errorHierNonPort(AstVarXRef* refp, AstVar* varp, VSymEnt* scopeSymp) {
        // OK to access ports on hier_block
        എങ്കിൽ (varp->isIO()) തിരികെ അസത്യം;
        // OK if not an instance
        സ്ഥിരം AstCell* സ്ഥിരം cellp = VN_CAST(scopeSymp->nodep(), Cell);
        എങ്കിൽ (!cellp) തിരികെ അസത്യം;
        // Ok if not a hier_block
        സ്ഥിരം Ast_കണ്ണി_Module* സ്ഥിരം modp = cellp->modp();
        എങ്കിൽ (!modp->hierBlock()) തിരികെ അസത്യം;
        // Bad
        refp->v3error("Cannot access non-port symbols inside hierarchical block");
        തിരികെ സത്യം;
    }

#define LINKDOT_VISIT_START() \
    VL_RESTORER(m_indent); \
    ++m_indent;

    string indent() സ്ഥിരം {
        string result = "";
        result.തിരുകുക(0, m_indent, ':');
        തിരികെ result + " ";
    }

    // VISITORS
    ശൂന്യം visit(AstNetlist* nodep) override {
        // Recurse..., backward as must do packages before using packages
        iterateChildrenBackwardsConst(nodep);
    }
    ശൂന്യം visit(AstTypeTable*) override {}
    ശൂന്യം visit(Ast_കണ്ണി_Module* nodep) override {
        എങ്കിൽ (nodep->dead()) തിരികെ;
        LINKDOT_VISIT_START();
        UINFO(8, indent() << "visit " << nodep);
        VL_RESTORER(m_genericIfaceModule);
        എങ്കിൽ (സ്ഥിരം AstModule* സ്ഥിരം modp = VN_CAST(nodep, Module)) {
            m_genericIfaceModule = modp->hasGenericIface();
        }
        checkNoDot(nodep);
        m_ds.init(m_curSymp);
        m_ds.m_dotSymp = m_curSymp = m_modSymp
            = m_statep->get_കണ്ണി_Sym(nodep);  // Until overridden by a SCOPE
        m_cellp = nullptr;
        m_modp = nodep;
        m_modport_അക്കം = 0;
        iterateChildren(nodep);
        m_modp = nullptr;
        m_ds.m_dotSymp = m_curSymp = m_modSymp = nullptr;
        m_ds.m_dotPos = DP_NONE;
        m_ds.m_dotErr = അസത്യം;
    }
    ശൂന്യം visit(AstScope* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(8, indent() << "visit " << nodep);
        checkNoDot(nodep);
        VL_RESTORER(m_modSymp);
        VL_RESTORER(m_curSymp);
        m_ds.m_dotSymp = m_curSymp = m_modSymp = m_statep->getScopeSym(nodep);
        iterateChildren(nodep);
        m_ds.m_dotSymp = m_curSymp = m_modSymp = nullptr;
    }
    ശൂന്യം visit(AstCellInline* nodep) override {
        LINKDOT_VISIT_START();
        checkNoDot(nodep);
        UASSERT_OBJ(m_statep->forScopeCreation(), nodep, "CellInline should not exist yet");
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep->unlinkFrBack()), nodep);
    }
    ശൂന്യം visit(AstCell* nodep) override {
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
            എങ്കിൽ (VN_IS(nodep->modp(), NotFoundModule)) {
                // Prevent warnings about missing pin connects
                എങ്കിൽ (nodep->pinsp()) nodep->pinsp()->unlinkFrBackWithNext()->delete_വൃക്ഷം();
                എങ്കിൽ (nodep->paramsp()) nodep->paramsp()->unlinkFrBackWithNext()->delete_വൃക്ഷം();
            }
            // Need to pass the module info to this cell, so we can link up the pin names
            // However can't use m_curSymp as pin connections need to use the
            // instantiator's symbols
            അല്ലെങ്കിൽ {
                m_pinSymp = m_statep->get_കണ്ണി_Sym(nodep->modp());
                UINFO(4, indent() << "(Backto) visit " << nodep);
                // UINFOTREE(1, nodep, "", "linkcell");
                // UINFOTREE(1, nodep->modp(), "", "linkcemd");
                iterateChildren(nodep);

                എങ്കിൽ (m_statep->forPrimary())
                    എങ്കിൽ (സ്ഥിരം AstModule* സ്ഥിരം modp = VN_CAST(nodep->modp(), Module)) {
                        എങ്കിൽ (modp->hasGenericIface())
                            addImplicitParametersOfGenericIface(nodep, modp);
                    }
            }
        }
        // Parent module inherits child's publicity
        // This is done bottom up in the LinkBotupVisitor stage
    }
    ശൂന്യം visit(AstClassRefDType* nodep) override {
        // Cell: Recurse inside or cleanup not founds
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        // Can be under dot if called as package::class and that class resolves, so no checkNoDot
        VL_RESTORER_CLEAR(m_usedPins);
        VL_RESTORER_CLEAR(m_usedDefParamPins);
        UASSERT_OBJ(nodep->classp(), nodep, "ClassRef has unlinked class");
        UASSERT_OBJ(m_statep->forPrimary() || !nodep->paramsp() || V3Error::error_എണ്ണം(), nodep,
                    "class reference parameter not removed by V3Param");
        VL_RESTORER(m_pinSymp);
        {
            // ClassRef's have pins, so track
            m_pinSymp = m_statep->get_കണ്ണി_Sym(nodep->classp());
            UINFO(4, indent() << "(Backto) visit " << nodep);
            // UINFOTREE(1, nodep, "", "linkcell");
            // UINFOTREE(1, nodep->modp(), "", "linkcemd");
            iterateChildren(nodep);
        }
    }
    ശൂന്യം visit(AstPin* nodep) override {
        // Pin: Link to submodule's port
        LINKDOT_VISIT_START();
        checkNoDot(nodep);
        iterateChildren(nodep);
        എങ്കിൽ (!nodep->modVarp()) {
            UASSERT_OBJ(m_pinSymp, nodep, "Pin not under instance?");
            VSymEnt* സ്ഥിരം foundp = m_pinSymp->findIdFlat(nodep->name());
            സ്ഥിരം അക്ഷരം* സ്ഥിരം whatp = nodep->param() ? "parameter" : "pin";
            എങ്കിൽ (!foundp) {
                എങ്കിൽ (m_cellp && VN_IS(m_cellp->modp(), Primitive)
                    && (nodep->name() == "__paramNumber1" || nodep->name() == "__paramNumber2")) {
                    // Primitive parameter is really a delay2 we can just ignore
                    VL_DO_DANGLING(തള്ളുക_Deletep(nodep->unlinkFrBack()), nodep);
                    തിരികെ;
                } അല്ലെങ്കിൽ എങ്കിൽ (nodep->param() && !nodep->exprp() && !nodep->svDotName()) {
                    // Placeholder pin the parser makes for an empty '#()', to signal
                    // the user did type the '#()'. No further use, deleting.
                    VL_DO_DANGLING(തള്ളുക_Deletep(nodep->unlinkFrBack()), nodep);
                    തിരികെ;
                } അല്ലെങ്കിൽ {
                    സ്ഥിരം std::string suggest
                        = (nodep->param()
                               ? m_statep->suggestSymFlat(m_pinSymp, nodep->name(),
                                                          Link_കണ്ണി_Matcher_Var_Param{}, അസത്യം)
                               : m_statep->suggestSymFlat(m_pinSymp, nodep->name(),
                                                          Link_കണ്ണി_Matcher_Var_IO{}, അസത്യം));
                    സ്ഥിരം std::string decl
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
                    VL_DO_DANGLING(തള്ളുക_Deletep(nodep->unlinkFrBack()), nodep);
                    തിരികെ;
                }
            }
            VVarType refVarType = VVarType::UNKNOWN;
            തർക്കം wrongPinType = അസത്യം;
            എങ്കിൽ (AstVar* സ്ഥിരം varp = VN_CAST(foundp->nodep(), Var)) {
                എങ്കിൽ (varp->isIO() || varp->isParam() || varp->isIfaceRef()) {
                    refVarType = varp->varType();
                    nodep->modVarp(varp);
                } അല്ലെങ്കിൽ {
                    wrongPinType = സത്യം;
                }
            } അല്ലെങ്കിൽ എങ്കിൽ (AstParamTypeDType* സ്ഥിരം typep = VN_CAST(foundp->nodep(), ParamTypeDType)) {
                refVarType = typep->varType();
                nodep->modPTypep(typep);
            } അല്ലെങ്കിൽ {
                wrongPinType = സത്യം;
            }
            // Don't connect parameter pin to module ports or vice versa
            എങ്കിൽ (nodep->param() != (refVarType == VVarType::GPARAM)) wrongPinType = സത്യം;
            എങ്കിൽ (wrongPinType) {
                string targetType = LinkDotState::കണ്ണി_Text_Type(foundp->nodep());
                targetType = VString::aOrAn(targetType) + ' ' + targetType;
                എങ്കിൽ (nodep->param()) {
                    nodep->v3error("Instance attempts to override "
                                   << nodep->prettyNameQ() << " as a " << whatp << ", but it is "
                                   << targetType);
                } അല്ലെങ്കിൽ {
                    nodep->v3error("Instance attempts to connect to "
                                   << nodep->prettyNameQ() << ", but it is " << targetType);
                }
                തിരികെ;
            }
            markAndCheckPinDup(nodep, foundp->nodep(), whatp);
        }
        // Early return() above when deleted
    }

    സ്ഥിതം തർക്കം isDotThisReference(സ്ഥിരം AstDot* nodep) {
        തിരികെ VN_IS(nodep->lhsp(), ParseRef) && nodep->lhsp()->name() == "this";
    }

    ശൂന്യം visit(AstDot* nodep) override {
        // Legal under a DOT: AstDot, AstParseRef, AstPackageRef, AstNodeSel, AstPackArrayDType
        //    also a DOT can be part of an expression, but only above plus
        //    AstFTaskRef are legal children
        // Dot(PackageRef, ParseRef(text))
        // Dot(Dot(ClassOrPackageRef,ClassOrPackageRef), ParseRef(text))
        // Dot(Dot(Dot(ParseRef(text), ...
        എങ്കിൽ (nodep->user3SetOnce()) തിരികെ;
        LINKDOT_VISIT_START();
        UINFO(8, indent() << "visit " << nodep);
        UINFO(8, indent() << m_ds.ascii());
        സ്ഥിരം DotStates lastStates = m_ds;
        സ്ഥിരം തർക്കം start = (m_ds.m_dotPos == DP_NONE);  // Save, as m_dotp will be changed
        സ്ഥിരം DotPosition initialDotPos = m_ds.m_dotPos;
        VL_RESTORER(m_randSymp);
        {
            എങ്കിൽ (start) {  // Starting dot sequence
                UINFOTREE(9, nodep, "", "dot-in");
                // Keep deferral from visit(AstMethodCall) for unspecialized rand target
                m_ds.init(m_curSymp, m_ds.m_unresolvedClass && randTargetUnresolved());
            }
            m_ds.m_dotp = nodep;  // Always, not just at start
            m_ds.m_dotPos = DP_FIRST;

            എങ്കിൽ (isDotThisReference(nodep)) {
                VSymEnt* classSymp = getThisClassSymp();
                // In 'randomize() with { this.member }', 'this' refers to randomized
                // object, not the calling class (IEEE 1800-2023 18.7)
                എങ്കിൽ (m_randSymp && m_currentWithp) classSymp = m_randSymp;
                എങ്കിൽ (!classSymp && randTargetUnresolved()) {
                    // Bind 'this' in post-V3Param pass
                } അല്ലെങ്കിൽ എങ്കിൽ (!classSymp) {
                    nodep->v3error("'this' used outside class (IEEE 1800-2023 8.11)");
                    m_ds.m_dotErr = സത്യം;
                } അല്ലെങ്കിൽ {
                    m_ds.m_dotSymp = classSymp;
                    UINFO(8, indent() << "this. " << m_ds.ascii());
                }
            } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(nodep->lhsp(), ParseRef) && nodep->lhsp()->name() == "super") {
                സ്ഥിരം VSymEnt* classSymp = getThisClassSymp();
                എങ്കിൽ (!classSymp) {
                    nodep->v3error("'super' used outside class (IEEE 1800-2023 8.15)");
                    m_ds.m_dotErr = സത്യം;
                } അല്ലെങ്കിൽ {
                    സ്ഥിരം auto classp = VN_AS(classSymp->nodep(), Class);
                    സ്ഥിരം auto cextp = classp->extendsp();
                    എങ്കിൽ (!cextp) {
                        nodep->v3error("'super' used on non-extended class (IEEE 1800-2023 8.15)");
                        m_ds.m_dotErr = സത്യം;
                    } അല്ലെങ്കിൽ {
                        എങ്കിൽ (m_statep->forPrimary()
                            && m_extendsParam.find(classp) != m_extendsParam.end()) {
                            UINFO(9, indent()
                                         << "deferring until post-V3Param: " << nodep->lhsp());
                            m_ds.m_unresolvedClass = സത്യം;
                        } അല്ലെങ്കിൽ {
                            സ്ഥിരം auto baseClassp = cextp->classp();
                            UASSERT_OBJ(baseClassp, nodep, "Bad superclass");
                            m_ds.m_dotSymp = m_statep->get_കണ്ണി_Sym(baseClassp);
                            m_ds.m_super = സത്യം;
                            UINFO(8, indent() << "super. " << m_ds.ascii());
                        }
                    }
                }
            } അല്ലെങ്കിൽ എങ്കിൽ (AstClassOrPackageRef* സ്ഥിരം lhsp
                       = VN_CAST(nodep->lhsp(), ClassOrPackageRef)) {
                // m_ds.m_dotText communicates the cell prefix between stages
                UINFO(8, indent() << "iter.lhs   " << m_ds.ascii() << " " << nodep);
                iterateAndNextNull(lhsp);
                എങ്കിൽ (!lhsp->classOrPackageSkipp() && lhsp->name() != "local::") {
                    revisitLater(nodep);
                    m_ds = lastStates;
                    // Resolve function args before bailing
                    എങ്കിൽ (Ast_കണ്ണി_FTask_Ref* സ്ഥിരം ftaskrefp = VN_CAST(nodep->rhsp(), കണ്ണി_FTask_Ref)) {
                        iterateAndNextNull(ftaskrefp->argsp());
                    }
                    തിരികെ;
                }
                m_ds.m_dotPos = DP_PACKAGE;
                // nodep->lhsp() may be a new node
                എങ്കിൽ (AstClassOrPackageRef* സ്ഥിരം classOrPackageRefp
                    = VN_CAST(nodep->lhsp(), ClassOrPackageRef)) {
                    എങ്കിൽ (Ast_കണ്ണി* സ്ഥിരം classOrPackageNodep
                        = classOrPackageRefp->classOrPackageSkipp()) {
                        m_ds.m_dotSymp = m_statep->get_കണ്ണി_Sym(classOrPackageNodep);
                    }
                }
                UINFO(8, indent() << "iter.ldone " << m_ds.ascii() << " " << nodep);
            } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(nodep->lhsp(), Dot) && VN_AS(nodep->lhsp(), Dot)->colon()) {
                // m_ds.m_dotText communicates the cell prefix between stages
                UINFO(8, indent() << "iter.lhs   " << m_ds.ascii() << " " << nodep);
                m_ds.m_dotPos = DP_PACKAGE;
                iterateAndNextNull(nodep->lhsp());
                // nodep->lhsp() may be a new node
                എങ്കിൽ (AstClassOrPackageRef* സ്ഥിരം crefp
                    = VN_CAST(nodep->lhsp(), ClassOrPackageRef)) {
                    എങ്കിൽ (!crefp->classOrPackageSkipp()) {
                        revisitLater(nodep);
                        m_ds = lastStates;
                        // Resolve function args before bailing
                        എങ്കിൽ (Ast_കണ്ണി_FTask_Ref* സ്ഥിരം ftaskrefp
                            = VN_CAST(nodep->rhsp(), കണ്ണി_FTask_Ref)) {
                            iterateAndNextNull(ftaskrefp->argsp());
                        }
                        തിരികെ;
                    }
                }
                എങ്കിൽ (m_lastDeferredp == nodep->lhsp()) {
                    // LHS got deferred, so this node won't be resolved. Defer it too
                    m_ds = lastStates;
                    // Resolve function args before bailing
                    എങ്കിൽ (Ast_കണ്ണി_FTask_Ref* സ്ഥിരം ftaskrefp = VN_CAST(nodep->rhsp(), കണ്ണി_FTask_Ref)) {
                        iterateAndNextNull(ftaskrefp->argsp());
                    }
                    തിരികെ;
                }
                UINFO(8, indent() << "iter.ldone " << m_ds.ascii() << " " << nodep);
            } അല്ലെങ്കിൽ {
                m_ds.m_dotPos = DP_FIRST;
                m_ds.m_unresolvedGenericIface = അസത്യം;
                UINFO(8, indent() << "iter.lhs   " << m_ds.ascii() << " " << nodep);
                iterateAndNextNull(nodep->lhsp());
                UINFO(8, indent() << "iter.ldone " << m_ds.ascii() << " " << nodep);
                എങ്കിൽ (m_ds.m_unresolvedGenericIface) {
                    m_ds.m_dotPos = initialDotPos;
                    തിരികെ;
                }
                // UINFOTREE(9, nodep, "", "dot-lho");
            }
            എങ്കിൽ (m_statep->forPrimary() && isParamedClassRef(nodep->lhsp())) {
                // Dots of paramed classes will be linked after deparameterization
                UINFO(9, indent() << "deferring until post-V3Param: " << nodep->lhsp());
                m_ds.m_unresolvedClass = സത്യം;
            }
            എങ്കിൽ (m_ds.m_unresolvedCell
                && (VN_IS(nodep->lhsp(), CellRef) || VN_IS(nodep->lhsp(), CellArrayRef))) {
                m_ds.m_unlinkedScopep = nodep->lhsp();
            }
            എങ്കിൽ (m_ds.m_dotPos == DP_FIRST) m_ds.m_dotPos = DP_SCOPE;
            എങ്കിൽ (!m_ds.m_dotErr) {  // Once something wrong, give up
                // Top 'final' dot RHS is final RHS, else it's a
                // DOT(DOT(x,*here*),real-rhs) which we consider a RHS
                എങ്കിൽ (start && m_ds.m_dotPos == DP_SCOPE) m_ds.m_dotPos = DP_FINAL;
                UINFO(8, indent() << "iter.rhs   " << m_ds.ascii() << " " << nodep);
                // m_ds.m_dotSymp points at lhsp()'s symbol table, so resolve RHS under that
                iterateAndNextNull(nodep->rhsp());
                UINFO(8, indent() << "iter.rdone " << m_ds.ascii() << " " << nodep);
                // UINFOTREE(9, nodep, "", "dot-rho");
            }
            എങ്കിൽ (!m_ds.m_unresolvedClass) {
                എങ്കിൽ (start) {
                    Ast_കണ്ണി* newp;
                    എങ്കിൽ (m_ds.m_dotErr) {
                        newp = പുതിയ AstConst{nodep->fileline(), AstConst::BitFalse{}};
                    } അല്ലെങ്കിൽ {
                        newp = nodep->rhsp()->unlinkFrBack();
                    }
                    UINFOTREE(9, newp, "", "dot-out");
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
                } അല്ലെങ്കിൽ {  // Dot midpoint
                    Ast_കണ്ണി* newp = nodep->rhsp()->unlinkFrBack();
                    എങ്കിൽ (m_ds.m_unresolvedCell) {
                        AstCellRef* സ്ഥിരം crp = പുതിയ AstCellRef{
                            nodep->fileline(), nodep->name(), nodep->lhsp()->unlinkFrBack(), newp};
                        newp = crp;
                    }
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
                }
            }
        }
        എങ്കിൽ (start) {
            m_ds = lastStates;
        } അല്ലെങ്കിൽ {
            സ്ഥിരം തർക്കം unresolvedClass = m_ds.m_unresolvedClass;
            m_ds.m_dotp = lastStates.m_dotp;
            m_ds.m_unresolvedClass |= unresolvedClass;
        }
        UINFO(8, indent() << "done " << m_ds.ascii() << " " << nodep);
    }
    ശൂന്യം visit(AstDefaultClocking* nodep) override {
        എങ്കിൽ (VSymEnt* സ്ഥിരം foundp = m_curSymp->findIdFallback(nodep->name())) {
            എങ്കിൽ (AstClocking* സ്ഥിരം clockingp = VN_CAST(foundp->nodep(), Clocking)) {
                clockingp->makeDefault();
                VL_DO_DANGLING(nodep->unlinkFrBack()->delete_വൃക്ഷം(), nodep);
            } അല്ലെങ്കിൽ {
                nodep->v3error(nodep->prettyNameQ() << " is not a clocking identifier");
            }
        } അല്ലെങ്കിൽ {
            nodep->v3error("Can't find definition of clocking: " << nodep->prettyNameQ());
        }
    }
    ശൂന്യം visit(AstSenItem* nodep) override {
        LINKDOT_VISIT_START();
        VL_RESTORER(m_inSens);
        m_inSens = സത്യം;
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstPatMember* nodep) override {
        LINKDOT_VISIT_START();
        എങ്കിൽ (nodep->varrefp()) തിരികെ;  // only do this mapping once
        // If we have a TEXT token as our key, lookup if it's a LPARAM
        എങ്കിൽ (AstText* സ്ഥിരം textp = VN_CAST(nodep->keyp(), Text)) {
            UINFO(9, indent() << "visit " << nodep);
            UINFO(9, indent() << "      " << textp);
            // Lookup
            എങ്കിൽ (VSymEnt* സ്ഥിരം foundp = m_curSymp->findIdFallback(textp->text())) {
                എങ്കിൽ (AstVar* സ്ഥിരം varp = VN_CAST(foundp->nodep(), Var)) {
                    എങ്കിൽ (varp->isParam() || varp->isGenVar()) {
                        // Attach found Text reference to PatMember
                        nodep->varrefp(
                            പുതിയ AstVarRef{nodep->fileline(),
                                          foundp->imported() ? foundp->classOrPackagep() : nullptr,
                                          varp, VAccess::READ});
                        UINFO(9, indent() << " new " << nodep->varrefp());
                    }
                }
                എങ്കിൽ (AstEnumItem* സ്ഥിരം itemp = VN_CAST(foundp->nodep(), EnumItem)) {
                    // Attach enum item value to PatMember
                    nodep->varrefp(
                        പുതിയ AstEnumItemRef{nodep->fileline(), itemp, foundp->classOrPackagep()});
                    UINFO(9, indent() << " new " << itemp);
                }
            }
        }
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstParseRef* nodep) override {
        എങ്കിൽ (nodep->user3SetOnce()) തിരികെ;
        LINKDOT_VISIT_START();
        UINFO(9, indent() << "visit " << nodep);
        UINFO(9, indent() << m_ds.ascii());
        എങ്കിൽ (m_ds.m_unresolvedClass) തിരികെ;
        // m_curSymp is symbol table of outer expression
        // m_ds.m_dotSymp is symbol table relative to "."'s above now
        UASSERT_OBJ(m_ds.m_dotSymp, nodep, "nullptr lookup symbol table");
        // Generally resolved during Primary, but might be at param time under AstUnlinkedRef
        UASSERT_OBJ(!m_statep->forScopeCreation(), nodep, "ParseRefs should no longer exist");
        സ്ഥിരം DotStates lastStates = m_ds;
        സ്ഥിരം തർക്കം start = (m_ds.m_dotPos == DP_NONE);  // Save, as m_dotp will be changed
        തർക്കം first = start || m_ds.m_dotPos == DP_FIRST;

        എങ്കിൽ (start) {
            m_ds.init(m_curSymp);
            // Note m_ds.m_dotp remains nullptr; this is a reference not under a dot
        }
        എങ്കിൽ (nodep->name() == "super") {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: super");
            m_ds.m_dotErr = സത്യം;
        }
        എങ്കിൽ (nodep->name() == "this") {
            iterateChildren(nodep);
            എങ്കിൽ (m_statep->forPrimary()) തിരികെ;  // The class might be parameterized somewhere
            സ്ഥിരം VSymEnt* classSymp = getThisClassSymp();
            എങ്കിൽ (!classSymp) {
                nodep->v3error("'this' used outside class (IEEE 1800-2023 8.11)");
                തിരികെ;
            }
            AstClass* സ്ഥിരം classp = VN_AS(classSymp->nodep(), Class);
            AstClassRefDType* സ്ഥിരം dtypep
                = പുതിയ AstClassRefDType{nodep->fileline(), classp, nullptr};
            AstThisRef* സ്ഥിരം newp = പുതിയ AstThisRef{nodep->fileline(), VFlagChildDType{}, dtypep};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
            തിരികെ;
        }
        എങ്കിൽ (m_ds.m_dotPos == DP_MEMBER && VN_IS(m_ds.m_dotp->lhsp(), LambdaArgRef)
            && nodep->name() == "index") {
            // 'with' statement's 'item.index'
            // m_ds.dotp->lhsp() was checked to know if `index` is directly after lambda arg ref.
            // If not, treat it as normal member select
            iterateChildren(nodep);
            AstLambdaArgRef* സ്ഥിരം newp = പുതിയ AstLambdaArgRef{
                nodep->fileline(), m_ds.m_dotp->lhsp()->name() + "__DOT__index", സത്യം};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
            തിരികെ;
        } അല്ലെങ്കിൽ എങ്കിൽ (m_ds.m_dotPos == DP_MEMBER) {
            // Found a Var, everything following is membership.  {scope}.{var}.HERE {member}
            എങ്കിൽ (m_ds.m_dotErr) {
                nodep->unlinkFrBack();  // Avoid circular node loop on errors
            } അല്ലെങ്കിൽ {
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം varEtcp = VN_AS(m_ds.m_dotp->lhsp()->unlinkFrBack(), കണ്ണി_വ്യഞ്ജകം);
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം newp = പുതിയ AstMemberSel{nodep->fileline(), varEtcp,  //
                                                           VFlagChildDType{}, nodep->name()};
                nodep->replaceWith(newp);
            }
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
        } അല്ലെങ്കിൽ {
            //
            string expectWhat;
            തർക്കം allowScope = അസത്യം;
            തർക്കം allowVar = അസത്യം;
            തർക്കം allowFTask = അസത്യം;
            തർക്കം staticAccess = അസത്യം;
            എങ്കിൽ (m_ds.m_disablep) {
                allowScope = സത്യം;
                allowFTask = സത്യം;
                expectWhat = "block/task";
            } അല്ലെങ്കിൽ എങ്കിൽ (m_ds.m_dotPos == DP_PACKAGE) {
                // {package-or-class}::{a}
                Ast_കണ്ണി_Module* classOrPackagep = nullptr;
                expectWhat = "scope/variable/func";
                allowScope = സത്യം;
                allowVar = സത്യം;
                allowFTask = സത്യം;
                staticAccess = സത്യം;
                UINFO(9, "chk pkg " << m_ds.ascii() << " lhsp=" << m_ds.m_dotp->lhsp());
                UASSERT_OBJ(VN_IS(m_ds.m_dotp->lhsp(), ClassOrPackageRef), m_ds.m_dotp->lhsp(),
                            "Bad package link");
                AstClassOrPackageRef* സ്ഥിരം cpackagerefp
                    = VN_AS(m_ds.m_dotp->lhsp(), ClassOrPackageRef);
                എങ്കിൽ (cpackagerefp->name() == "local::") {
                    m_randSymp = nullptr;
                    first = സത്യം;
                } അല്ലെങ്കിൽ എങ്കിൽ (!cpackagerefp->classOrPackageSkipp()) {
                    VSymEnt* സ്ഥിരം foundp = m_statep->resolveClassOrPackage(
                        m_ds.m_dotSymp, cpackagerefp, സത്യം, അസത്യം, ":: reference");
                    എങ്കിൽ (!foundp) തിരികെ;
                    classOrPackagep = cpackagerefp->classOrPackageSkipp();
                    എങ്കിൽ (classOrPackagep) {
                        m_ds.m_dotSymp = m_statep->get_കണ്ണി_Sym(classOrPackagep);
                    } അല്ലെങ്കിൽ {
                        m_ds = lastStates;
                        തിരികെ;
                    }
                } അല്ലെങ്കിൽ {
                    classOrPackagep = cpackagerefp->classOrPackageSkipp();
                    UASSERT_OBJ(classOrPackagep, m_ds.m_dotp->lhsp(), "Bad package link");
                    m_ds.m_dotSymp = m_statep->get_കണ്ണി_Sym(classOrPackagep);
                }
                m_ds.m_dotPos = DP_SCOPE;
            } അല്ലെങ്കിൽ എങ്കിൽ (m_ds.m_dotPos == DP_SCOPE || m_ds.m_dotPos == DP_FIRST) {
                // {a}.{b}, where {a} maybe a module name
                // or variable, where dotting into structure member
                expectWhat = "scope/variable";
                allowScope = സത്യം;
                allowVar = സത്യം;
            } അല്ലെങ്കിൽ എങ്കിൽ (m_ds.m_dotPos == DP_NONE) {
                expectWhat = "variable";
                allowVar = സത്യം;
            } അല്ലെങ്കിൽ എങ്കിൽ (m_ds.m_dotPos == DP_FINAL) {
                expectWhat = "variable/method";
                allowFTask = സത്യം;
                allowVar = സത്യം;
            } അല്ലെങ്കിൽ {
                UINFO(1, "ds=" << m_ds.ascii());
                nodep->v3fatalSrc("Unhandled DotPosition");
            }
            // Lookup
            VSymEnt* foundp;
            string baddot;
            VSymEnt* okSymp = nullptr;
            // Restricted 'with (id_list)': only id_list names bind into target class.
            എങ്കിൽ (m_randSymp
                && (!m_currentWithp || m_currentWithp->nameResolvesToTarget(nodep->name()))) {
                foundp = m_randSymp->findIdFlat(nodep->name());
                എങ്കിൽ (foundp) {
                    എങ്കിൽ (m_currentWithp && m_currentWithp->restricted()) {
                        m_restrictedNamesUsed.തിരുകുക(nodep->name());
                    }
                    എങ്കിൽ (!start) m_ds.m_dotPos = DP_MEMBER;
                    എങ്കിൽ (!m_currentWithp) {
                        UASSERT_OBJ(m_randMethodCallp, nodep, "Expected to be under randomize()");
                        // This will start failing once complex expressions are allowed on the LHS
                        // of randomize() with args
                        UASSERT_OBJ(VN_IS(m_randMethodCallp->fromp(), VarRef), m_randMethodCallp,
                                    "Expected simple randomize target");
                        // A ParseRef is used here so that the dot RHS gets resolved
                        AstMemberSel* സ്ഥിരം newp = പുതിയ AstMemberSel{
                            nodep->fileline(), m_randMethodCallp->fromp()->clone_വൃക്ഷം(അസത്യം),
                            VFlagChildDType{}, nodep->name()};
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
                        തിരികെ;
                    }
                    UINFO(9, indent() << "randomize-with fromSym " << foundp->nodep());
                    AstLambdaArgRef* സ്ഥിരം lambdaRefp
                        = പുതിയ AstLambdaArgRef{nodep->fileline(), "__Vrandwith_obj", അസത്യം};
                    AstMemberSel* newp = പുതിയ AstMemberSel{nodep->fileline(), lambdaRefp,
                                                          VFlagChildDType{}, nodep->name()};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
                    തിരികെ;
                }
            }
            എങ്കിൽ (allowScope) {
                foundp = m_statep->findDotted(nodep->fileline(), m_ds.m_dotSymp, nodep->name(),
                                              baddot, okSymp, first);  // Maybe nullptr
            } അല്ലെങ്കിൽ എങ്കിൽ (first) {
                foundp = m_ds.m_dotSymp->findIdFallback(nodep->name());
            } അല്ലെങ്കിൽ {
                foundp = m_ds.m_dotSymp->findIdFlat(nodep->name());
            }
            എങ്കിൽ (!foundp && m_ds.m_dotp && isDotThisReference(m_ds.m_dotp)) {
                സ്ഥിരം AstClass* സ്ഥിരം classp = VN_CAST(m_ds.m_dotSymp->nodep(), Class);
                എങ്കിൽ (classp && classp->isCovergroup() && classp->covergroupEnclosingClassp()) {
                    VSymEnt* സ്ഥിരം enclosingClassSymp
                        = m_statep->get_കണ്ണി_Sym(classp->covergroupEnclosingClassp());
                    foundp = enclosingClassSymp->findIdFallback(nodep->name());
                    എങ്കിൽ (foundp) m_ds.m_dotSymp = enclosingClassSymp;
                }
            }
            // If not found in modport, check interface fallback for parameters and typedefs.
            // Parameters and typedefs are always visible through a modport (IEEE 1800-2023 25.5).
            // This mirrors the VarXRef modport parameter fallback in visit(AstVarXRef).
            എങ്കിൽ (!foundp && VN_IS(m_ds.m_dotSymp->nodep(), Modport)
                && m_ds.m_dotSymp->fallbackp()) {
                VSymEnt* സ്ഥിരം ifaceFallbackp = m_ds.m_dotSymp->fallbackp();
                VSymEnt* സ്ഥിരം ifaceFoundp = ifaceFallbackp->findIdFlat(nodep->name());
                എങ്കിൽ (ifaceFoundp) {
                    എങ്കിൽ (സ്ഥിരം AstVar* സ്ഥിരം varp = VN_CAST(ifaceFoundp->nodep(), Var)) {
                        എങ്കിൽ (varp->isParam()) foundp = ifaceFoundp;
                    } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(ifaceFoundp->nodep(), Typedef)
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
            എങ്കിൽ (!foundp && !okSymp && VN_IS(m_ds.m_dotSymp->nodep(), Modport)) {
                okSymp = m_ds.m_dotSymp;
            }
            എങ്കിൽ (foundp) {
                UINFO(9, indent() << "found=se" << cvtToHex(foundp) << "  exp=" << expectWhat
                                  << "  n=" << foundp->nodep());
            }
            // What fell out?
            തർക്കം ok = അസത്യം;
            // Special case: waiting on clocking event
            എങ്കിൽ (m_inSens && foundp && m_ds.m_dotPos != DP_SCOPE && m_ds.m_dotPos != DP_FIRST) {
                എങ്കിൽ (AstClocking* സ്ഥിരം clockingp = sensClockingp(foundp->nodep())) {
                    foundp = getCreateClockingEventSymEnt(clockingp);
                }
            }
            എങ്കിൽ (!foundp) {
            } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(foundp->nodep(), Cell) || VN_IS(foundp->nodep(), കണ്ണി_Block)
                       || VN_IS(foundp->nodep(), GenBlock)
                       || VN_IS(foundp->nodep(), Netlist)  // for $root
                       || VN_IS(foundp->nodep(), Module)) {  // if top
                എങ്കിൽ (allowScope) {
                    ok = സത്യം;
                    m_ds.m_dotText = VString::dot(m_ds.m_dotText, ".", nodep->name());
                    m_ds.m_dotSymp = foundp;
                    m_ds.m_dotPos = DP_SCOPE;
                    എങ്കിൽ (m_ds.m_disablep && VN_IS(foundp->nodep(), കണ്ണി_Block)) {
                        // Possibly it is not the final link. If we are under dot and not in its
                        // last component, `targetp()` field will be overwritten by next components
                        m_ds.m_disablep->targetp(foundp->nodep());
                    }
                    എങ്കിൽ (VN_IS(foundp->nodep(), GenBlock)) {
                        m_ds.m_genBlk = സത്യം;
                        എങ്കിൽ (m_ds.m_disablep) {
                            m_ds.m_disablep->v3warn(
                                E_UNSUPPORTED,
                                "Unsupported: Generate block referenced by disable");
                        }
                    }
                    // Upper AstDot visitor will handle it from here
                } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(foundp->nodep(), Cell) && allowVar) {
                    AstCell* സ്ഥിരം cellp = VN_AS(foundp->nodep(), Cell);
                    എങ്കിൽ (VN_IS(cellp->modp(), Iface)) {
                        // Interfaces can be referenced like a variable for interconnect
                        VSymEnt* സ്ഥിരം cellEntp = m_statep->get_കണ്ണി_Sym(cellp);
                        UASSERT_OBJ(cellEntp, nodep, "No interface sym entry");
                        VSymEnt* സ്ഥിരം parentEntp
                            = cellEntp->parentp();  // Container of the var; probably a module or
                                                    // generate begin
                        AstVar* സ്ഥിരം ifaceRefVarp
                            = find_Iface_മുകൾ_Varp(nodep, parentEntp, nodep->name());
                        //
                        ok = സത്യം;
                        m_ds.m_dotSymp = foundp;
                        m_ds.m_dotPos = DP_SCOPE;
                        UINFO(9, indent() << " cell -> iface varref " << foundp->nodep());
                        Ast_കണ്ണി_വ്യഞ്ജകം* newp;
                        എങ്കിൽ (m_ds.m_dotText != "") {
                            newp = പുതിയ AstVarXRef{nodep->fileline(), ifaceRefVarp, m_ds.m_dotText,
                                                  VAccess::READ};
                            എങ്കിൽ (m_ds.m_unresolvedCell && m_ds.m_unlinkedScopep) {
                                newp = പുതിയ AstUnlinkedRef{nodep->fileline(), newp, newp->name(),
                                                          m_ds.m_unlinkedScopep->unlinkFrBack()};
                                m_ds.m_unlinkedScopep = nullptr;
                                m_ds.m_unresolvedCell = അസത്യം;
                                m_ds.m_dotText = "";
                            }
                        } അല്ലെങ്കിൽ {
                            AstVarRef* സ്ഥിരം refp
                                = പുതിയ AstVarRef{nodep->fileline(), ifaceRefVarp, VAccess::READ};
                            newp = refp;
                        }
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
                    } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(cellp->modp(), NotFoundModule)) {
                        cellp->modNameFileline()->v3error("Cannot find file containing interface: "
                                                          << cellp->modp()->prettyNameQ());
                    }
                }
            } അല്ലെങ്കിൽ എങ്കിൽ (allowScope && !allowFTask && VN_IS(foundp->nodep(), കണ്ണി_FTask)) {
                // Function/task used as scope for static variable access (IEEE std 1800-2017 6.21)
                // Don't set m_dotText so the static variable resolves as a VarRef rather than
                // VarXRef. This is so that V3Begin won't move it out of the function/task's scope.
                ok = സത്യം;
                m_ds.m_dotSymp = foundp;
                m_ds.m_dotPos = DP_SCOPE;
            } അല്ലെങ്കിൽ എങ്കിൽ (allowFTask && VN_IS(foundp->nodep(), കണ്ണി_FTask)) {
                Ast_കണ്ണി_FTask_Ref* taskrefp;
                എങ്കിൽ (VN_IS(foundp->nodep(), Task)) {
                    taskrefp = പുതിയ AstTaskRef{nodep->fileline(), nodep->name()};
                } അല്ലെങ്കിൽ {
                    taskrefp = പുതിയ AstFuncRef{nodep->fileline(), nodep->name()};
                }
                nodep->replaceWith(taskrefp);
                VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
                m_ds = lastStates;
                തിരികെ;
            } അല്ലെങ്കിൽ എങ്കിൽ (AstVar* സ്ഥിരം varp = foundToVarp(foundp, nodep, VAccess::READ)) {
                // Preserve explicit 'this' references to members inherited from
                // generic classes until after specialization.
                എങ്കിൽ (m_statep->forPrimary() && m_ds.m_dotp && isDotThisReference(m_ds.m_dotp)
                    && foundp->classOrPackagep() != m_modp
                    && VN_IS(foundp->classOrPackagep(), Class)
                    && foundp->classOrPackagep()->hasGParam()) {
                    m_ds.m_unresolvedClass = സത്യം;
                    തിരികെ;
                }
                AstIfaceRefDType* സ്ഥിരം ifacerefp
                    = LinkDotState::ifaceRefFromArray(varp->subDTypep());
                എങ്കിൽ (varp->isIfaceRef() && m_genericIfaceModule
                    && VN_IS(varp->childDTypep(), IfaceGenericDType) && !start) {
                    // Defer only dotted member access ('d.PARAM'), as V3Param must specialize
                    // first; a standalone ref ('.x(d)') resolves via allowVar below now
                    ok = സത്യം;
                    m_ds.m_unresolvedGenericIface = സത്യം;
                } അല്ലെങ്കിൽ എങ്കിൽ (ifacerefp && varp->isIfaceRef()) {
                    UASSERT_OBJ(ifacerefp->ifaceViaCellp(), ifacerefp, "Unlinked interface");
                    // Really this is a scope reference into an interface
                    UINFO(9, indent() << "varref-ifaceref " << m_ds.m_dotText << "  " << nodep);
                    m_ds.m_dotText = VString::dot(m_ds.m_dotText, ".", nodep->name());
                    // If modport specified, use modport symbol table for lookup of virtual ports
                    എങ്കിൽ (ifacerefp->modportp() && m_statep->exists_കണ്ണി_Sym(ifacerefp->modportp())) {
                        m_ds.m_dotSymp = m_statep->get_കണ്ണി_Sym(ifacerefp->modportp());
                    } അല്ലെങ്കിൽ {
                        m_ds.m_dotSymp = m_statep->get_കണ്ണി_Sym(ifacerefp->ifaceViaCellp());
                    }
                    m_ds.m_dotPos = DP_SCOPE;
                    ok = സത്യം;

                    AstVarRef* സ്ഥിരം refp = പുതിയ AstVarRef{nodep->fileline(), varp, VAccess::READ};
                    nodep->replaceWith(refp);

                    VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
                } അല്ലെങ്കിൽ എങ്കിൽ (allowVar) {
                    Ast_കണ്ണി* newp;
                    എങ്കിൽ (m_ds.m_dotText != "") {
                        AstVarXRef* സ്ഥിരം refp
                            = പുതിയ AstVarXRef{nodep->fileline(), nodep->name(), m_ds.m_dotText,
                                             VAccess::READ};  // lvalue'ness computed later
                        refp->varp(varp);
                        // Not linked again after V3LinkLValue, so that reports writing it
                        എങ്കിൽ (സ്ഥിരം AstModportVarRef* സ്ഥിരം mvarp
                            = VN_CAST(foundp->nodep(), ModportVarRef)) {
                            എങ്കിൽ (m_statep->forParamed() && mvarp->direction().isReadOnly()) {
                                refp->readOnlyModport(സത്യം);
                            }
                        }
                        refp->containsGenBlock(m_ds.m_genBlk);
                        m_ds.m_dotText = "";
                        എങ്കിൽ (m_ds.m_unresolvedCell && m_ds.m_unlinkedScopep) {
                            സ്ഥിരം string dotted = refp->dotted();
                            സ്ഥിരം size_t pos = dotted.find("__BRA__??__KET__");
                            // Arrays of interfaces all have the same parameters
                            എങ്കിൽ (pos != string::npos && varp->isParam()
                                && VN_IS(m_ds.m_unlinkedScopep, CellArrayRef)) {
                                refp->dotted(dotted.substr(0, pos));
                                newp = refp;
                            } അല്ലെങ്കിൽ {
                                UINFO(9, indent() << "deferring until post-V3Param: " << refp);
                                newp = പുതിയ AstUnlinkedRef{nodep->fileline(), refp, refp->name(),
                                                          m_ds.m_unlinkedScopep->unlinkFrBack()};
                                m_ds.m_unlinkedScopep = nullptr;
                                m_ds.m_unresolvedCell = അസത്യം;
                            }
                        } അല്ലെങ്കിൽ {
                            newp = refp;
                        }
                    } അല്ലെങ്കിൽ {
                        എങ്കിൽ (staticAccess && !varp->lifetime().isStatic() && !varp->isParam()) {
                            // TODO bug4077
                            // nodep->v3error("Static access to non-static member variable "
                            //                << varp->prettyNameQ() << endl);
                        }
                        AstVarRef* സ്ഥിരം refp = പുതിയ AstVarRef{
                            nodep->fileline(), varp, VAccess::READ};  // lvalue'ness computed later
                        refp->classOrPackagep(foundp->classOrPackagep());
                        newp = refp;
                    }
                    UINFO(9, indent() << "new " << newp);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
                    m_ds.m_dotPos = DP_MEMBER;
                    ok = സത്യം;
                }
            } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstModport* സ്ഥിരം modportp = VN_CAST(foundp->nodep(), Modport)) {
                // A scope reference into an interface's modport (not
                // necessarily at a pin connection)
                UINFO(9, indent() << "cell-ref-to-modport " << m_ds.m_dotText << "  " << nodep);
                UINFO(9, indent() << "unlinked " << m_ds.m_unlinkedScopep);
                UINFO(9,
                      indent() << "dotSymp " << m_ds.m_dotSymp << " " << m_ds.m_dotSymp->nodep());
                // Iface was the previously dotted component
                എങ്കിൽ (!m_ds.m_dotSymp || !VN_IS(m_ds.m_dotSymp->nodep(), Cell)
                    || !VN_AS(m_ds.m_dotSymp->nodep(), Cell)->modp()
                    || !VN_IS(VN_AS(m_ds.m_dotSymp->nodep(), Cell)->modp(), Iface)) {
                    nodep->v3error("Modport not referenced as <interface>."
                                   << modportp->prettyNameQ());
                } അല്ലെങ്കിൽ എങ്കിൽ (!VN_AS(m_ds.m_dotSymp->nodep(), Cell)->modp()
                           || !VN_IS(VN_AS(m_ds.m_dotSymp->nodep(), Cell)->modp(), Iface)) {
                    nodep->v3error("Modport not referenced from underneath an interface: "
                                   << modportp->prettyNameQ());
                } അല്ലെങ്കിൽ {
                    AstCell* സ്ഥിരം cellp = VN_AS(m_ds.m_dotSymp->nodep(), Cell);
                    UASSERT_OBJ(cellp, nodep, "Modport not referenced from an instance");
                    VSymEnt* സ്ഥിരം cellEntp = m_statep->get_കണ്ണി_Sym(cellp);
                    UASSERT_OBJ(cellEntp, nodep, "No interface sym entry");
                    VSymEnt* സ്ഥിരം parentEntp
                        = cellEntp->parentp();  // Container of the var; probably a
                                                // module or generate begin
                    // We drop __BRA__??__KET__ as cells don't have that naming yet
                    AstVar* സ്ഥിരം ifaceRefVarp
                        = find_Iface_മുകൾ_Varp(nodep, parentEntp, cellp->name());
                    //
                    ok = സത്യം;
                    m_ds.m_dotText = VString::dot(m_ds.m_dotText, ".", nodep->name());
                    m_ds.m_dotSymp = foundp;
                    m_ds.m_dotPos = DP_SCOPE;
                    UINFO(9, indent() << "modport -> iface varref " << foundp->nodep());
                    // We lose the modport name here, so we cannot detect mismatched modports.
                    AstVarRef* സ്ഥിരം refp
                        = പുതിയ AstVarRef{nodep->fileline(), ifaceRefVarp, VAccess::READ};
                    Ast_കണ്ണി_വ്യഞ്ജകം* newp = refp;

                    auto* സ്ഥിരം cellarrayrefp = VN_CAST(m_ds.m_unlinkedScopep, CellArrayRef);
                    എങ്കിൽ (cellarrayrefp) {
                        // iface[vec].modport became CellArrayRef(iface, lsb)
                        // Convert back to SelBit(iface, lsb)
                        UINFO(9, indent() << "Array modport to SelBit " << cellarrayrefp);
                        newp = പുതിയ AstSelBit{cellarrayrefp->fileline(), newp,
                                             cellarrayrefp->selp()->unlinkFrBack()};
                        newp->user3(സത്യം);  // Don't process again
                        VL_DO_DANGLING(തള്ളുക_Deletep(cellarrayrefp->unlinkFrBack()), cellarrayrefp);
                        m_ds.m_unlinkedScopep = nullptr;
                    }
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
                }
            } അല്ലെങ്കിൽ എങ്കിൽ (AstConstraint* സ്ഥിരം defp = VN_CAST(foundp->nodep(), Constraint)) {
                Ast_കണ്ണി* സ്ഥിരം newp = പുതിയ AstConstraintRef{nodep->fileline(), nullptr, defp};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
                ok = സത്യം;
                m_ds.m_dotPos = DP_MEMBER;
                m_ds.m_dotText = "";
            } അല്ലെങ്കിൽ എങ്കിൽ (AstClass* സ്ഥിരം defp = VN_CAST(foundp->nodep(), Class)) {
                എങ്കിൽ (allowVar) {
                    എങ്കിൽ (m_ds.m_dotPos == DP_SCOPE && !staticAccess) {
                        // Continue hierarchical lookup in the class scope.
                        m_ds.m_dotSymp = foundp;
                    } അല്ലെങ്കിൽ {
                        AstRefDType* സ്ഥിരം newp
                            = പുതിയ AstRefDType{nodep->fileline(), nodep->name()};
                        replaceWithCheckBreak(nodep, newp);
                        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
                    }
                    ok = സത്യം;
                    m_ds.m_dotText = "";
                } അല്ലെങ്കിൽ {
                    (ശൂന്യം)defp;  // Prevent unused variable warning
                }
            } അല്ലെങ്കിൽ എങ്കിൽ (AstMemberDType* സ്ഥിരം defp = VN_CAST(foundp->nodep(), MemberDType)) {
                എങ്കിൽ (allowVar) {
                    checkMemberDeclOrder(nodep, defp);
                    AstRefDType* സ്ഥിരം refp = പുതിയ AstRefDType{nodep->fileline(), nodep->name()};
                    refp->refDTypep(defp);
                    replaceWithCheckBreak(nodep, refp);
                    VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
                    ok = സത്യം;
                    m_ds.m_dotText = "";
                }
            } അല്ലെങ്കിൽ എങ്കിൽ (AstEnumItem* സ്ഥിരം valuep = VN_CAST(foundp->nodep(), EnumItem)) {
                എങ്കിൽ (allowVar) {
                    AstEnumItemRef* സ്ഥിരം refp
                        = പുതിയ AstEnumItemRef{nodep->fileline(), valuep, foundp->classOrPackagep()};
                    Ast_കണ്ണി* newp = refp;
                    // Hierarchical reference, relinked after V3Param as the referenced
                    // module may be specialized, similar to AstVarXRef
                    refp->dotted(m_ds.m_dotText);
                    refp->containsGenBlock(m_ds.m_genBlk);
                    എങ്കിൽ (m_ds.m_unresolvedCell && m_ds.m_unlinkedScopep) {
                        UINFO(9, indent() << "deferring until post-V3Param: " << refp);
                        newp = പുതിയ AstUnlinkedRef{nodep->fileline(), refp, refp->name(),
                                                  m_ds.m_unlinkedScopep->unlinkFrBack()};
                        m_ds.m_unlinkedScopep = nullptr;
                        m_ds.m_unresolvedCell = അസത്യം;
                    }
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
                    ok = സത്യം;
                    m_ds.m_dotText = "";
                }
            } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstLambdaArgRef* സ്ഥിരം argrefp
                       = VN_CAST(foundp->nodep(), LambdaArgRef)) {
                എങ്കിൽ (allowVar) {
                    Ast_കണ്ണി* സ്ഥിരം newp
                        = പുതിയ AstLambdaArgRef{nodep->fileline(), argrefp->name(), അസത്യം};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
                    ok = സത്യം;
                    m_ds.m_dotPos = DP_MEMBER;
                    m_ds.m_dotText = "";
                }
            } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(foundp->nodep(), Clocking)) {
                m_ds.m_dotSymp = foundp;
                എങ്കിൽ (m_ds.m_dotText != "") m_ds.m_dotText += "." + nodep->name();
                ok = m_ds.m_dotPos == DP_SCOPE || m_ds.m_dotPos == DP_FIRST;
            } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstModportClockingRef* സ്ഥിരം clockingRefp
                       = VN_CAST(foundp->nodep(), ModportClockingRef)) {
                // Clocking block accessed through a modport - redirect to actual clocking
                m_ds.m_dotSymp = m_statep->get_കണ്ണി_Sym(clockingRefp->clockingp());
                എങ്കിൽ (m_ds.m_dotText != "") m_ds.m_dotText += "." + nodep->name();
                ok = m_ds.m_dotPos == DP_SCOPE || m_ds.m_dotPos == DP_FIRST;
            } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം Ast_കണ്ണി_FTask* സ്ഥിരം ftaskp = VN_CAST(foundp->nodep(), കണ്ണി_FTask)) {
                എങ്കിൽ (!ftaskp->isFunction() || ftaskp->classMethod()) {
                    ok = m_ds.m_dotPos == DP_NONE;
                    എങ്കിൽ (ok) {
                        // The condition is true for tasks,
                        // properties and void functions.
                        // In these cases, the parentheses may be skipped.
                        // Also SV class methods can be called without parens
                        AstFuncRef* സ്ഥിരം funcRefp
                            = പുതിയ AstFuncRef{nodep->fileline(), nodep->name()};
                        nodep->replaceWith(funcRefp);
                        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
                    }
                }
            } അല്ലെങ്കിൽ എങ്കിൽ (AstTypedef* സ്ഥിരം defp = VN_CAST(foundp->nodep(), Typedef)) {
                സ്ഥിരം തർക്കം ifaceFinalSegmentAllowed
                    = (m_ds.m_dotPos == DP_FINAL) && m_ds.m_dotSymp
                      && (VN_IS(m_ds.m_dotSymp->nodep(), Modport)
                          || (VN_IS(m_ds.m_dotSymp->nodep(), Cell)
                              && VN_CAST(m_ds.m_dotSymp->nodep(), Cell)->modp()
                              && VN_IS(VN_CAST(m_ds.m_dotSymp->nodep(), Cell)->modp(), Iface)));
                // Allow interface typedef references even without IfaceCapture
                // The dependency graph handles resolution when IfaceCapture is disabled
                ok = (m_ds.m_dotPos == DP_NONE || m_ds.m_dotPos == DP_SCOPE
                      || ifaceFinalSegmentAllowed);
                എങ്കിൽ (ifaceFinalSegmentAllowed) {
                    UINFO(9, indent() << "allow final-segment typedef name="
                                      << nodep->prettyNameQ() << " dotText='" << m_ds.m_dotText
                                      << "' dotSym=" << m_ds.m_dotSymp);
                }
                എങ്കിൽ (ok) {
                    AstRefDType* സ്ഥിരം refp = പുതിയ AstRefDType{nodep->fileline(), nodep->name()};
                    // Don't check if typedef is to a <type T>::<reference> as might not be
                    // resolved yet
                    എങ്കിൽ (m_ds.m_dotPos == DP_NONE) checkDeclOrder(nodep, defp);
                    refp->typedefp(defp);

                    V3LinkDotIfaceCapture::captureTypedefContext(
                        refp, "typedef", static_cast<പൂർണ്ണസംഖ്യ>(m_ds.m_dotPos), m_ds.m_dotText,
                        m_ds.m_dotSymp, m_modp, [ഇത്]() { തിരികെ indent(); });

                    എങ്കിൽ (VN_IS(nodep->backp(), SelExtract)) {
                        m_packedArrayDtp = refp;
                    } അല്ലെങ്കിൽ {
                        replaceWithCheckBreak(nodep, refp);
                        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
                    }
                }
            } അല്ലെങ്കിൽ എങ്കിൽ (AstParamTypeDType* സ്ഥിരം defp = VN_CAST(foundp->nodep(), ParamTypeDType)) {
                ok = (m_ds.m_dotPos == DP_NONE || m_ds.m_dotPos == DP_SCOPE
                      || m_ds.m_dotPos == DP_FINAL);
                എങ്കിൽ (ok) {
                    AstRefDType* സ്ഥിരം refp = പുതിയ AstRefDType{nodep->fileline(), nodep->name()};
                    refp->refDTypep(defp);

                    V3LinkDotIfaceCapture::captureTypedefContext(
                        refp, "paramtype", static_cast<പൂർണ്ണസംഖ്യ>(m_ds.m_dotPos), m_ds.m_dotText,
                        m_ds.m_dotSymp, m_modp, [ഇത്]() { തിരികെ indent(); });

                    എങ്കിൽ (VN_IS(nodep->backp(), SelExtract)) {
                        m_packedArrayDtp = refp;
                    } അല്ലെങ്കിൽ {
                        replaceWithCheckBreak(nodep, refp);
                        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
                    }
                }
            }
            എങ്കിൽ (!ok) {
                എങ്കിൽ (m_insideClassExtParam) {
                    // Don't throw error if the reference is inside a class that extends a param,
                    // because some members can't be linked in such a case. m_insideClassExtParam
                    // may be true only in the first stage of linking.
                    // Mark that the Dot statement can't be resolved.
                    m_ds.m_unresolvedClass = സത്യം;
                    // If the symbol was a scope name, it would be resolved.
                    m_ds.m_dotPos = DP_MEMBER;
                } അല്ലെങ്കിൽ {
                    // Cells/interfaces can't be implicit
                    സ്ഥിരം തർക്കം checkImplicit
                        = (!m_ds.m_dotp && m_ds.m_dotText == "" && !m_ds.m_disablep && !foundp);
                    സ്ഥിരം തർക്കം err
                        = !(checkImplicit && m_statep->implicitOk(m_modp, nodep->name()));
                    എങ്കിൽ (err) {
                        എങ്കിൽ (foundp) {
                            nodep->v3error("Found definition of '"
                                           << m_ds.m_dotText << (m_ds.m_dotText == "" ? "" : ".")
                                           << nodep->prettyName() << "'" << " as a "
                                           << foundp->nodep()->typeName() << " but expected a "
                                           << expectWhat);
                        } അല്ലെങ്കിൽ എങ്കിൽ (m_ds.m_dotText == "") {
                            UINFO(1, "   ErrParseRef curSymp=se" << cvtToHex(m_curSymp)
                                                                 << " ds=" << m_ds.ascii());
                            സ്ഥിരം string suggest = m_statep->suggestSymFallback(
                                m_ds.m_dotSymp, nodep->name(), VNodeMatcher{});
                            nodep->v3error(
                                "Can't find definition of "
                                << expectWhat << ": " << nodep->prettyNameQ() << '\n'
                                << (suggest.empty() ? "" : nodep->warnMore() + suggest));
                        } അല്ലെങ്കിൽ {
                            nodep->v3error(
                                "Can't find definition of "
                                << (!baddot.empty() ? Ast_കണ്ണി::prettyNameQ(baddot)
                                                    : nodep->prettyNameQ())
                                << " in dotted " << expectWhat << ": '"
                                << m_ds.m_dotText + "." + nodep->prettyName() << "'\n"
                                << nodep->warnContextPrimary()
                                << (okSymp ? okSymp->cellErrorScopes(
                                                 nodep, Ast_കണ്ണി::prettyName(m_ds.m_dotText))
                                           : ""));
                        }
                        m_ds.m_dotErr = സത്യം;
                    }
                    എങ്കിൽ (checkImplicit) {
                        // Create if implicit, and also if error (so only complain once)
                        // Else if a scope is allowed, making a signal won't help error cascade
                        AstVar* സ്ഥിരം varp
                            = createImplicitVar(m_curSymp, nodep, m_modp, m_modSymp, err);
                        AstVarRef* സ്ഥിരം newp
                            = പുതിയ AstVarRef{nodep->fileline(), varp, VAccess::READ};
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
                    }
                }
            }
        }
        എങ്കിൽ (start) m_ds = lastStates;
    }
    ശൂന്യം visit(AstClassOrPackageRef* nodep) override {
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

            എങ്കിൽ (!nodep->classOrPackageSkipp() && !nodep->classOrPackageNodep()
                && nodep->name() != "local::") {
                സ്ഥിരം തർക്കം deferIfUnresolved = m_statep->forPrimary() && m_insideClassExtParam;
                m_statep->resolveClassOrPackage(m_ds.m_dotSymp, nodep, m_ds.m_dotPos != DP_PACKAGE,
                                                അസത്യം, ":: reference", deferIfUnresolved);
            }

            // ClassRef's have pins, so track
            എങ്കിൽ (nodep->classOrPackageSkipp()) {
                m_pinSymp = m_statep->get_കണ്ണി_Sym(nodep->classOrPackageSkipp());
            } അല്ലെങ്കിൽ എങ്കിൽ (nodep->name() != "local::") {
                തിരികെ;
            }

            AstClass* സ്ഥിരം refClassp = VN_CAST(nodep->classOrPackageSkipp(), Class);
            // Make sure any extends() are properly imported within referenced class
            എങ്കിൽ (refClassp && !m_statep->forPrimary()) classExtendImport(refClassp);

            എങ്കിൽ (m_statep->forPrimary() && refClassp && !nodep->paramsp()
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
                AstPin* സ്ഥിരം pinp = പുതിയ AstPin{nodep->fileline(), 1, "__paramNumber1", nullptr};
                pinp->param(സത്യം);
                nodep->addParamsp(pinp);
            }

            m_ds.init(m_curSymp);
            UINFO(4, indent() << "(Backto) Link ClassOrPackageRef: " << nodep);
            iterateChildren(nodep);
        }

        എങ്കിൽ (nodep->name() == "local::") {
            എങ്കിൽ (!m_randSymp && !m_randMethodCallp) {
                nodep->v3error("Illegal 'local::' outside 'randomize() with'"
                               " (IEEE 1800-2023 18.7.1)");
                m_ds.m_dotErr = സത്യം;
                തിരികെ;
            }
        }
        എങ്കിൽ (m_ds.m_dotPos == DP_PACKAGE && nodep->classOrPackageSkipp()) {
            m_ds.m_dotSymp = m_statep->get_കണ്ണി_Sym(nodep->classOrPackageSkipp());
            UINFO(9, indent() << "set sym " << m_ds.ascii());
        }
    }
    ശൂന്യം visit(AstConstraint* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        എങ്കിൽ (nodep->isExternDef()) {
            എങ്കിൽ (സ്ഥിരം VSymEnt* സ്ഥിരം foundp
                = m_curSymp->findIdFallback("extern " + nodep->name())) {
                സ്ഥിരം AstConstraint* സ്ഥിരം protop = VN_AS(foundp->nodep(), Constraint);
                // Copy specifiers.
                // External definition cannot have any specifiers, so no value will be overwritten.
                nodep->isStatic(protop->isStatic());
            } അല്ലെങ്കിൽ {
                nodep->v3error("extern not found that declares " + nodep->prettyNameQ());
            }
        }
        എങ്കിൽ (nodep->isExternProto()) {
            എങ്കിൽ (!m_curSymp->findIdFallback(nodep->name()) && nodep->isExternExplicit()) {
                nodep->v3warn(PROTOTYPEMIS, "Definition not found for extern prototype "
                                                << nodep->prettyNameQ());
            }
        }
        VL_RESTORER(m_curSymp);
        VL_RESTORER_COPY(m_ds);
        m_ds.m_dotSymp = m_curSymp = m_statep->get_കണ്ണി_Sym(nodep);
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstConstraintRef* nodep) override {
        എങ്കിൽ (nodep->user3SetOnce()) തിരികെ;
        LINKDOT_VISIT_START();
        UINFO(8, indent() << "visit " << nodep);
        UINFO(8, indent() << m_ds.ascii());
        // No children defined
    }
    ശൂന്യം visit(AstVarRef* nodep) override {
        // VarRef: Resolve its reference
        // ParseRefs are used the first pass (forPrimary) so we shouldn't get can't find
        // errors here now that we have a VarRef.
        // No checkNoDot; created and iterated from a parseRef
        LINKDOT_VISIT_START();
        iterateChildren(nodep);
        എങ്കിൽ (!nodep->varp()) {
            UINFO(9, indent() << "linkVarRef se" << cvtToHex(m_curSymp) << "  n=" << nodep);
            UASSERT_OBJ(m_curSymp, nodep, "nullptr lookup symbol table");
            VSymEnt* സ്ഥിരം foundp = m_curSymp->findIdFallback(nodep->name());
            എങ്കിൽ (AstVar* സ്ഥിരം varp
                = foundp ? foundToVarp(foundp, nodep, nodep->access()) : nullptr) {
                nodep->varp(varp);
                updateVarUse(nodep->varp());
                // Generally set by parse, but might be an import
                nodep->classOrPackagep(foundp->classOrPackagep());
            }
            എങ്കിൽ (VL_UNCOVERABLE(!nodep->varp())) {
                nodep->v3error("Can't find definition of signal, again: "  // LCOV_EXCL_LINE
                               << nodep->prettyNameQ());
            }
        }
        AstVarScope* vscp = nodep->varScopep();
        എങ്കിൽ (vscp && vscp->user2p() != vscp && m_replaceWithAlias) {
            vscp = LinkDotScopeVisitor::getAlias(vscp);
            nodep->varp(vscp->varp());
            nodep->varScopep(vscp);
            updateVarUse(nodep->varp());
            UINFO(7, indent() << "Resolved " << nodep);  // Also prints taskp
        }
    }
    ശൂന്യം visit(AstVarXRef* nodep) override {
        // VarRef: Resolve its reference
        // We always link even if varp() is set, because the module we choose may change
        // due to creating new modules, flattening, etc.
        എങ്കിൽ (nodep->user3SetOnce()) തിരികെ;
        LINKDOT_VISIT_START();
        UINFO(8, indent() << "visit " << nodep);
        // No checkNoDot; created and iterated from a parseRef
        എങ്കിൽ (!m_modSymp) {
            // Module that is not in hierarchy.  We'll be dead code eliminating it later.
            UINFO(9, "Dead module for " << nodep);
            nodep->varp(nullptr);
        } അല്ലെങ്കിൽ {
            VSymEnt* lookupSymp = m_curSymp;  // Start search at current scope
            എങ്കിൽ (nodep->inlinedDots() != "") {  // Correct for current post-inlined scope
                lookupSymp = findInlinedDotsSym(nodep, nodep->inlinedDots());
            }
            string baddot;
            VSymEnt* okSymp;
            തർക്കം modport;
            VSymEnt* സ്ഥിരം dotSymp
                = findDottedRefSymp(nodep, lookupSymp, nodep->dotted(), baddot, okSymp, &modport);
            എങ്കിൽ (!dotSymp) തിരികെ;

            എങ്കിൽ (!m_statep->forScopeCreation()) {
                VSymEnt* foundp = nullptr;
                എങ്കിൽ (modport) {
                    // This is a copy of findSymPrefixed with few modifications.
                    // This searches without a fallback like findSymPrefixed but if lookup fails it
                    // will check one fallback (fallback of modport shall be interface) and take
                    // only L and G PARAMs since they should be visible from a modport always.
                    string prefix = dotSymp->symPrefix();
                    ആകുമ്പോൾ (!foundp) {
                        foundp = dotSymp->findIdFlat(prefix + nodep->name());
                        എങ്കിൽ (foundp) നിർത്തുക;
                        UASSERT_OBJ(dotSymp->fallbackp(), nodep, "Modports shall have fallback");
                        foundp = dotSymp->fallbackp()->findIdFlat(prefix + nodep->name());
                        എങ്കിൽ (സ്ഥിരം AstVar* സ്ഥിരം varp
                            = foundToVarp(foundp, nodep, nodep->access())) {
                            എങ്കിൽ (!varp->isParam()) foundp = nullptr;
                        }
                        എങ്കിൽ (prefix.empty()) നിർത്തുക;
                        string nextPrefix = LinkDotState::removeLastInlineScope(prefix);
                        എങ്കിൽ (prefix == nextPrefix) നിർത്തുക;
                        prefix = std::move(nextPrefix);
                    }
                    baddot = nodep->name();
                } അല്ലെങ്കിൽ {
                    foundp = m_statep->findSymPrefixed(dotSymp, nodep->name(), baddot, സത്യം);
                }
                എങ്കിൽ (m_inSens && foundp) {
                    എങ്കിൽ (AstClocking* സ്ഥിരം clockingp = sensClockingp(foundp->nodep())) {
                        foundp = getCreateClockingEventSymEnt(clockingp);
                    }
                }
                AstVar* സ്ഥിരം varp
                    = foundp ? foundToVarp(foundp, nodep, nodep->access()) : nullptr;
                nodep->varp(varp);
                updateVarUse(nodep->varp());
                UINFO(7, indent() << "Resolved " << nodep);  // Also prints varp
                // If found, check if it's ok to access in case it's in a hier_block
                എങ്കിൽ (nodep->varp() && errorHierNonPort(nodep, nodep->varp(), dotSymp)) തിരികെ;
                എങ്കിൽ (!nodep->varp()) {
                    nodep->v3error("Can't find definition of "
                                   << Ast_കണ്ണി::prettyNameQ(baddot) << " in dotted signal: '"
                                   << nodep->dotted() + "." + nodep->prettyName() << "'\n"
                                   << nodep->warnContextPrimary()
                                   << okSymp->cellErrorScopes(nodep));
                    തിരികെ;
                }
            } അല്ലെങ്കിൽ {
                VSymEnt* സ്ഥിരം foundp
                    = m_statep->findSymPrefixed(dotSymp, nodep->name(), baddot, സത്യം);
                AstVarScope* vscp = foundp ? VN_CAST(foundp->nodep(), VarScope) : nullptr;
                // Handle modport expression virtual ports
                എങ്കിൽ (!vscp) vscp = findVirtualPortVarScope(dotSymp, nodep->name());
                // If found, check if it's ok to access in case it's in a hier_block
                എങ്കിൽ (vscp && errorHierNonPort(nodep, vscp->varp(), dotSymp)) തിരികെ;
                എങ്കിൽ (!vscp) {
                    nodep->v3error("Can't find varpin scope of "
                                   << Ast_കണ്ണി::prettyNameQ(baddot) << " in dotted signal: '"
                                   << nodep->dotted() + "." + nodep->prettyName() << "'\n"
                                   << nodep->warnContextPrimary()
                                   << okSymp->cellErrorScopes(nodep));
                } അല്ലെങ്കിൽ {
                    എങ്കിൽ (vscp->user2p() && m_replaceWithAlias) {
                        vscp = LinkDotScopeVisitor::getAlias(vscp);
                    }
                    // Convert the VarXRef to a VarRef, so we don't need
                    // later optimizations to deal with VarXRef.
                    nodep->varp(vscp->varp());
                    nodep->varScopep(vscp);
                    updateVarUse(nodep->varp());
                    UINFO(7, indent() << "Resolved " << nodep);  // Also prints taskp
                    AstVarRef* സ്ഥിരം newvscp
                        = പുതിയ AstVarRef{nodep->fileline(), vscp, nodep->access()};
                    nodep->replaceWith(newvscp);
                    VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
                    UINFO(9, indent() << "new " << newvscp);  // Also prints taskp
                }
            }
        }
    }
    ശൂന്യം visit(AstEnumDType* nodep) override {
        LINKDOT_VISIT_START();
        iterateChildren(nodep);
        AstRefDType* സ്ഥിരം refdtypep = VN_CAST(nodep->subDTypep(), RefDType);
        എങ്കിൽ (refdtypep && (nodep == refdtypep->subDTypep())) {
            refdtypep->v3error("Self-referential enumerated type definition");
        }
    }
    ശൂന്യം visit(Ast_കണ്ണി_UOr_Struct_DType* nodep) override {
        LINKDOT_VISIT_START();
        symIterateChildren(nodep, m_statep->get_കണ്ണി_Sym(nodep));
    }
    ശൂന്യം visit(AstEnumItemRef* nodep) override {
        // Resolve its reference
        // EnumItemRefs are created by the first pass, but V3Param may regenerate due to
        // a parameterized class/module, so we shouldn't get can't find errors.
        // No checkNoDot; created and iterated from a parseRef
        എങ്കിൽ (nodep->user3SetOnce()) തിരികെ;
        LINKDOT_VISIT_START();
        എങ്കിൽ (m_statep->forParamed() && !nodep->dotted().empty() && m_modSymp) {
            // Hierarchical reference, relink as V3Param may have specialized the module
            string baddot;
            VSymEnt* okSymp = nullptr;
            VSymEnt* സ്ഥിരം dotSymp
                = findDottedRefSymp(nodep, m_curSymp, nodep->dotted(), baddot, okSymp);
            എങ്കിൽ (!dotSymp) തിരികെ;
            VSymEnt* സ്ഥിരം foundp
                = m_statep->findSymPrefixed(dotSymp, nodep->name(), baddot, സത്യം);
            AstEnumItem* സ്ഥിരം itemp = foundp ? VN_CAST(foundp->nodep(), EnumItem) : nullptr;
            എങ്കിൽ (!itemp) {
                nodep->v3error("Enum item " << nodep->prettyNameQ() << " not found in "
                                            << Ast_കണ്ണി::prettyNameQ(nodep->dotted()) << '\n'
                                            << nodep->warnContextPrimary());
                തിരികെ;
            }
            nodep->itemp(itemp);
            UINFO(9, indent() << " relinked " << nodep);
        }
        എങ്കിൽ (!nodep->itemp()) {
            UINFO(9, indent() << "linkEnumRef se" << cvtToHex(m_curSymp) << "  n=" << nodep);
            UASSERT_OBJ(m_curSymp, nodep, "nullptr lookup symbol table");
            VSymEnt* സ്ഥിരം foundp = m_curSymp->findIdFallback(nodep->name());
            എങ്കിൽ (AstEnumItem* സ്ഥിരം itemp = foundp ? VN_CAST(foundp->nodep(), EnumItem) : nullptr) {
                nodep->itemp(itemp);
                nodep->classOrPackagep(foundp->classOrPackagep());
                UINFO(9, indent() << " resolved " << nodep);
            }
            എങ്കിൽ (VL_UNCOVERABLE(!nodep->itemp())) {
                nodep->v3error("Can't find definition of enum item, again: "  // LCOV_EXCL_LINE
                               << nodep->prettyNameQ());
            }
        }
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstMethodCall* nodep) override {
        // Created here so should already be resolved.
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        VL_RESTORER_COPY(m_ds);
        VL_RESTORER(m_randSymp);
        VL_RESTORER(m_randMethodCallp);
        {
            m_ds.init(m_curSymp);
            എങ്കിൽ (nodep->name() == "randomize" && (nodep->argsp() || nodep->withp())) {
                m_randMethodCallp = nodep;
                സ്ഥിരം Ast_കണ്ണി_DType* fromDtp = nodep->fromp()->dtypep();
                എങ്കിൽ (!fromDtp) {
                    എങ്കിൽ (സ്ഥിരം Ast_കണ്ണി_Var_Ref* സ്ഥിരം varRefp = VN_CAST(nodep->fromp(), കണ്ണി_Var_Ref)) {
                        fromDtp = varRefp->varp()->subDTypep();
                    } അല്ലെങ്കിൽ {
                        fromDtp = get_വ്യഞ്ജകം_DTypep(nodep->fromp());
                    }
                    എങ്കിൽ (!fromDtp) {
                        എങ്കിൽ (nodep->withp()) {
                            nodep->v3warn(
                                E_UNSUPPORTED,
                                "Unsupported: 'randomize() with' on complex expressions");
                        } അല്ലെങ്കിൽ {
                            nodep->v3warn(E_UNSUPPORTED,
                                          "Unsupported: Inline random variable control with "
                                          "'randomize()' called on complex expressions");
                        }
                    }
                }
                എങ്കിൽ (m_statep->forPrimary() && isParamedClassRefDType(fromDtp)) {
                    m_ds.m_unresolvedClass = സത്യം;
                } അല്ലെങ്കിൽ എങ്കിൽ (fromDtp) {
                    സ്ഥിരം AstClassRefDType* സ്ഥിരം classDtp
                        = VN_CAST(fromDtp->skipRefp(), ClassRefDType);
                    എങ്കിൽ (!classDtp)
                        nodep->v3error("'randomize() with' on a non-class-instance "
                                       << fromDtp->prettyNameQ());
                    അല്ലെങ്കിൽ എങ്കിൽ (m_statep->forPrimary() && m_extendsParam.എണ്ണം(classDtp->classp())) {
                        // Inherited members are unavailable until the base class is specialized.
                        // Defer lookup rather than incorrectly binding names in the caller.
                        m_ds.m_unresolvedClass = സത്യം;
                    } അല്ലെങ്കിൽ {
                        m_randSymp = m_statep->get_കണ്ണി_Sym(classDtp->classp());
                    }
                }
            }
            iterateChildren(nodep);
        }
    }
    ശൂന്യം visit(AstVar* nodep) override {
        LINKDOT_VISIT_START();
        checkNoDot(nodep);
        VL_RESTORER(m_isParam);
        m_isParam = nodep->varType().isParam();
        iterateChildren(nodep);
        എങ്കിൽ (m_statep->forPrimary() && nodep->isIO() && !m_ftaskp && !nodep->user4()) {
            nodep->v3error(
                "Input/output/inout does not appear in port list: " << nodep->prettyNameQ());
        }
    }
    ശൂന്യം visit(AstVarScope* nodep) override {
        LINKDOT_VISIT_START();
        checkNoDot(nodep);
        iterateChildren(nodep);
        AstVarScope* aliasp = LinkDotScopeVisitor::getAlias(nodep);
        എങ്കിൽ (aliasp && aliasp != nodep && !nodep->varp()->isIfaceRef()) {
            // Aliased variable might still be references from outside,
            // eg through the VPI, and is traced, so we need the value to propagate.
            // TODO: this means external writes to the LHS (e.g.: through the VPI) don't work
            AstAssignW* സ്ഥിരം assignp = പുതിയ AstAssignW{
                nodep->fileline(), പുതിയ AstVarRef{nodep->fileline(), nodep, VAccess::WRITE},
                പുതിയ AstVarRef{nodep->fileline(), aliasp, VAccess::READ}};
            assignp->user2(സത്യം);
            nodep->scopep()->addBlocksp(പുതിയ AstAlways{assignp});
            // Propagate attributes and lint state of the replaced variable,
            // because all references to it are replaced with references to the alias variable
            aliasp->varp()->propagateAttrFrom(nodep->varp());
            aliasp->varp()->fileline()->modifyStateInherit(nodep->varp()->fileline());
        }
    }
    ശൂന്യം visit(Ast_കണ്ണി_FTask_Ref* nodep) override {
        എങ്കിൽ (nodep->user3SetOnce()) തിരികെ;
        LINKDOT_VISIT_START();
        UINFO(8, indent() << "visit " << nodep);
        UINFO(8, indent() << m_ds.ascii());
        എങ്കിൽ (m_ds.m_dotPos != DP_MEMBER || nodep->name() != "randomize") {
            // Visit arguments at the beginning.
            // They may be visitted even if the current node can't be linked now.
            symIterateChildren(nodep, m_curSymp);
        }

        എങ്കിൽ (m_ds.m_super) {
            എങ്കിൽ (AstFuncRef* സ്ഥിരം funcRefp = VN_CAST(nodep, FuncRef)) {
                funcRefp->superReference(സത്യം);
            } അല്ലെങ്കിൽ എങ്കിൽ (AstTaskRef* സ്ഥിരം taskRefp = VN_CAST(nodep, TaskRef)) {
                taskRefp->superReference(സത്യം);
            }
        }

        VL_RESTORER(m_randSymp);

        തർക്കം first = !m_ds.m_dotp || m_ds.m_dotPos == DP_FIRST;
        തർക്കം staticAccess = അസത്യം;
        എങ്കിൽ (m_ds.m_unresolvedClass) {
            // Unable to link before V3Param
            തിരികെ;
        } അല്ലെങ്കിൽ എങ്കിൽ (m_ds.m_unresolvedCell && m_ds.m_dotPos == DP_FINAL && m_ds.m_unlinkedScopep) {
            Ast_കണ്ണി_FTask_Ref* സ്ഥിരം newftaskp = nodep->clone_വൃക്ഷം(അസത്യം);
            newftaskp->dotted(m_ds.m_dotText);
            Ast_കണ്ണി* സ്ഥിരം newp = പുതിയ AstUnlinkedRef{nodep->fileline(), newftaskp, nodep->name(),
                                                     m_ds.m_unlinkedScopep->unlinkFrBack()};
            m_ds.m_unlinkedScopep = nullptr;
            m_ds.m_unresolvedCell = അസത്യം;
            nodep->replaceWith(newp);
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
            തിരികെ;
        } അല്ലെങ്കിൽ എങ്കിൽ (m_ds.m_dotp && m_ds.m_dotPos == DP_PACKAGE) {
            UASSERT_OBJ(VN_IS(m_ds.m_dotp->lhsp(), ClassOrPackageRef), m_ds.m_dotp->lhsp(),
                        "Bad package link");
            staticAccess = സത്യം;
            AstClassOrPackageRef* സ്ഥിരം cpackagerefp
                = VN_AS(m_ds.m_dotp->lhsp(), ClassOrPackageRef);
            എങ്കിൽ (cpackagerefp->name() == "local::") {
                m_randSymp = nullptr;
                first = സത്യം;
            } അല്ലെങ്കിൽ എങ്കിൽ (!cpackagerefp->classOrPackageSkipp()) {
                VSymEnt* സ്ഥിരം foundp = m_statep->resolveClassOrPackage(
                    m_ds.m_dotSymp, cpackagerefp, സത്യം, അസത്യം, ":: reference");
                എങ്കിൽ (foundp) nodep->classOrPackagep(cpackagerefp->classOrPackageSkipp());
            } അല്ലെങ്കിൽ {
                nodep->classOrPackagep(cpackagerefp->classOrPackageSkipp());
            }
            // Class/package :: HERE function() . method_called_on_function_return_value()
            m_ds.m_dotPos = DP_MEMBER;
        } അല്ലെങ്കിൽ എങ്കിൽ (m_ds.m_dotp && m_ds.m_dotPos == DP_FINAL) {
            nodep->dotted(m_ds.m_dotText);  // Maybe ""
            // Only flag FTaskRefs under generate-if/case blocks that may be
            // pruned.  GenFor and plain begin-blocks won't be pruned by V3Param.
            // A generate-case block's parent is the GenCaseItem, not the GenCase.
            // VarXRef uses the broader m_genBlk flag (set for all GenBlocks)
            // because genfor unrolling also removes variables.
            എങ്കിൽ (m_ds.m_genBlk && m_ds.m_dotSymp) {
                സ്ഥിരം Ast_കണ്ണി* സ്ഥിരം blkp = m_ds.m_dotSymp->nodep();
                എങ്കിൽ (VN_IS(blkp, GenBlock)
                    && (VN_IS(blkp->backp(), GenIf) || VN_IS(blkp->backp(), GenCaseItem))) {
                    nodep->containsGenBlock(സത്യം);
                }
            }
        } അല്ലെങ്കിൽ എങ്കിൽ (m_ds.m_dotp && m_ds.m_dotPos == DP_MEMBER) {
            // Found a Var, everything following is method call.
            // {scope}.{var}.HERE {method} ( ARGS )
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം varEtcp = VN_AS(m_ds.m_dotp->lhsp()->unlinkFrBack(), കണ്ണി_വ്യഞ്ജകം);
            AstArg* സ്ഥിരം argsp = nodep->argsp();
            എങ്കിൽ (argsp) argsp->unlinkFrBackWithNext();
            AstMethodCall* സ്ഥിരം newp = പുതിയ AstMethodCall{nodep->fileline(), varEtcp,
                                                          VFlagChildDType{}, nodep->name(), argsp};
            എങ്കിൽ (AstWith* സ്ഥിരം withp = nodep->withp()) newp->withp(withp->unlinkFrBack());
            nodep->replaceWith(newp);
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
            തിരികെ;
        } അല്ലെങ്കിൽ എങ്കിൽ (m_ds.m_dotp && (m_ds.m_dotPos == DP_SCOPE || m_ds.m_dotPos == DP_FIRST)) {
            // HERE function() . method_called_on_function_return_value()
            m_ds.m_dotPos = DP_MEMBER;
            m_ds.m_dotText = "";
        } അല്ലെങ്കിൽ എങ്കിൽ (!m_ds.m_disablep) {
            // visit(AstDisable*) setup the dot handling
            checkNoDot(nodep);
        }
        എങ്കിൽ (nodep->classOrPackagep() && nodep->taskp()) {
            // References into packages don't care about cell hierarchy.
        } അല്ലെങ്കിൽ എങ്കിൽ (!m_modSymp) {
            // Module that is not in hierarchy.  We'll be dead code eliminating it later.
            UINFO(9, indent() << "Dead module for " << nodep);
            nodep->taskp(nullptr);
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->dotted() == "" && nodep->taskp()) {
            // Earlier should have setup the links
            // Might be under a BEGIN we're not processing, so don't relink it
        } അല്ലെങ്കിൽ {
            string baddot;
            VSymEnt* okSymp = nullptr;
            VSymEnt* dotSymp;
            എങ്കിൽ (nodep->dotted().empty()) {
                // Non-'super.' dotted reference
                dotSymp = m_ds.m_dotSymp;
            } അല്ലെങ്കിൽ {
                // Start search at module, as a variable of same name under a subtask isn't a
                // relevant hit however a function under a begin/end is.  So we want begins, but
                // not the function
                dotSymp = m_curSymp;
            }
            എങ്കിൽ (nodep->classOrPackagep()) {  // Look only in specified package
                dotSymp = m_statep->get_കണ്ണി_Sym(nodep->classOrPackagep());
                UINFO(8, indent() << "Override classOrPackage " << dotSymp);
            } അല്ലെങ്കിൽ {
                എങ്കിൽ (nodep->inlinedDots() != "") {  // Correct for current post-inlined scope
                    dotSymp = findInlinedDotsSym(nodep, nodep->inlinedDots());
                }
                dotSymp = m_statep->findDotted(nodep->fileline(), dotSymp, nodep->dotted(), baddot,
                                               okSymp, സത്യം);  // Maybe nullptr
            }
            // Restricted 'with (id_list)': only id_list names bind into target class.
            എങ്കിൽ (m_randSymp
                && (!m_currentWithp || m_currentWithp->nameResolvesToTarget(nodep->name()))) {
                VSymEnt* സ്ഥിരം foundp = m_randSymp->findIdFlat(nodep->name());
                എങ്കിൽ (foundp && m_currentWithp) {
                    എങ്കിൽ (m_currentWithp->restricted()) {
                        m_restrictedNamesUsed.തിരുകുക(nodep->name());
                    }
                    UINFO(9, indent() << "randomize-with fromSym " << foundp->nodep());
                    AstArg* argsp = nullptr;
                    എങ്കിൽ (nodep->argsp()) {
                        iterateAndNextNull(nodep->argsp());
                        argsp = nodep->argsp()->unlinkFrBackWithNext();
                    }
                    എങ്കിൽ (m_ds.m_dotPos != DP_NONE) m_ds.m_dotPos = DP_MEMBER;
                    Ast_കണ്ണി* സ്ഥിരം newp = പുതിയ AstMethodCall{
                        nodep->fileline(),
                        പുതിയ AstLambdaArgRef{nodep->fileline(), "__Vrandwith_obj", അസത്യം},
                        VFlagChildDType{}, nodep->name(), argsp};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
                    തിരികെ;
                }
            }
            എങ്കിൽ (first && nodep->name() == "randomize" && VN_IS(m_modp, Class)
                && !VN_IS(nodep->classOrPackagep(), Package)) {
                // need special handling to avoid falling back to std::randomize
                // Skip if classOrPackagep is a Package (i.e. std::randomize resolved earlier)
                VMemberMap memberMap;
                AstFunc* സ്ഥിരം randFuncp = V3Randomize::newRandomizeFunc(
                    memberMap, VN_AS(m_modp, Class), nodep->name(), സത്യം, സത്യം);
                nodep->taskp(randFuncp);
                nodep->classOrPackagep(VN_AS(m_modp, Class));
                m_curSymp = m_statep->തിരുകുക_Block(m_curSymp, nodep->name(), randFuncp, m_modp);
                // Already linked to the class randomize function; skip findSymPrefixed
                // which may find std::randomize and overwrite classOrPackagep
                തിരികെ;
            }
            VSymEnt* സ്ഥിരം foundp
                = m_statep->findSymPrefixed(dotSymp, nodep->name(), baddot, first);
            Ast_കണ്ണി_FTask* സ്ഥിരം taskp = foundp ? VN_CAST(foundp->nodep(), കണ്ണി_FTask) : nullptr;
            Ast_കണ്ണി_Module* സ്ഥിരം taskModulep = foundp ? foundp->classOrPackagep() : nullptr;
            // Ignore deferring linking of functions inside params of classes extending
            // parametric classes if referenced function can be linked in the first pass.
            എങ്കിൽ (!m_isParam || !taskp
                || (taskp->classMethod() && taskModulep && taskModulep->hasParameterList())) {
                എങ്കിൽ (m_insideClassExtParam) {
                    // The reference may point to a method declared in a super class, which is
                    // proved by a parameter. In such a case, it can't be linked at the first
                    // stage. Must not do any linking, because e.g. might find an extends of an
                    // upper class because the current class (under parent) isn't yet importing
                    // it's extended class symbols.
                    തിരികെ;
                }
                എങ്കിൽ (AstClass* സ്ഥിരം targetClassp = VN_CAST(dotSymp->nodep(), Class)) {
                    എങ്കിൽ (m_extendsParam.എണ്ണം(targetClassp)) {
                        // Target class has parameterized extends not yet resolved.
                        // Its inherited symbols (e.g. static functions from the base class)
                        // aren't imported yet - defer to linkDotParamed.
                        തിരികെ;
                    }
                }
            }

            എങ്കിൽ (taskp) {
                എങ്കിൽ (staticAccess && !taskp->isStatic()) {
                    // TODO bug4077
                    // nodep->v3error("Static access to non-static task/function "
                    //                << taskp->prettyNameQ() << endl);
                }
                nodep->taskp(taskp);
                nodep->classOrPackagep(foundp->classOrPackagep());
                UINFO(7, indent() << "Resolved " << nodep);  // Also prints taskp
            } അല്ലെങ്കിൽ {
                // Note ParseRef has similar error handling/message output
                UINFO(7, indent() << "   ErrFtask curSymp=se" << cvtToHex(m_curSymp)
                                  << " dotSymp=se" << cvtToHex(dotSymp));
                എങ്കിൽ (foundp) {
                    എങ്കിൽ (VN_IS(foundp->nodep(), Var) && m_ds.m_dotText == "" && m_ftaskp
                        && m_ftaskp->name() == foundp->nodep()->name()) {
                        // This is a recursive reference to the function itself, not to the var
                        nodep->taskp(m_ftaskp);
                        nodep->classOrPackagep(foundp->classOrPackagep());
                        UINFO(7, indent() << "Resolved recursive " << nodep);  // Also prints taskp
                    } അല്ലെങ്കിൽ {
                        nodep->v3error("Found definition of '"
                                       << m_ds.m_dotText << (m_ds.m_dotText == "" ? "" : ".")
                                       << nodep->prettyName() << "'" << " as a "
                                       << foundp->nodep()->typeName()
                                       << " but expected a task/function");
                    }
                } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(nodep, New) && !m_statep->forScopeCreation()) {
                    // Resolved in V3Width
                } അല്ലെങ്കിൽ എങ്കിൽ ((nodep->name() == "pre_randomize" || nodep->name() == "post_randomize")
                           && VN_IS(dotSymp->nodep(), Class)) {
                    AstClass* സ്ഥിരം classp = VN_AS(dotSymp->nodep(), Class);
                    // Functions that might be defined by user, but if not, they get empty
                    // definitions within referenced class.
                    // TODO perhaps cleaner to add to symbol table up-front with
                    // a lambda to construct them on a reference.
                    AstFunc* സ്ഥിരം funcp
                        = പുതിയ AstFunc{nodep->fileline(), nodep->name(), nullptr, nullptr};
                    funcp->classMethod(സത്യം);
                    funcp->dtypep(funcp->findVoidDType());
                    classp->addMembersp(funcp);
                    m_statep->തിരുകുക_Block(dotSymp, nodep->name(), funcp, classp);
                    nodep->taskp(funcp);
                    nodep->classOrPackagep(classp);
                    UINFO(9, indent() << " Auto-made " << funcp);
                    തിരികെ;
                } അല്ലെങ്കിൽ എങ്കിൽ (nodep->name() == "randomize" || nodep->name() == "srandom"
                           || nodep->name() == "get_randstate" || nodep->name() == "set_randstate"
                           || nodep->name() == "rand_mode" || nodep->name() == "constraint_mode") {
                    എങ്കിൽ (AstClass* സ്ഥിരം classp = VN_CAST(m_modp, Class)) {
                        // Don't overwrite if already resolved to a package (e.g. std::randomize)
                        എങ്കിൽ (!VN_IS(nodep->classOrPackagep(), Package))
                            nodep->classOrPackagep(classp);
                    } അല്ലെങ്കിൽ എങ്കിൽ (nodep->name() == "randomize") {
                        // A std::randomize resolved in V3Width
                        nodep->classOrPackagep(v3Global.rootp()->stdPackagep());
                    } അല്ലെങ്കിൽ {
                        nodep->v3error("Calling implicit class method "
                                       << nodep->prettyNameQ() << " without being under class");
                        nodep->replaceWith(പുതിയ AstConst{nodep->fileline(), 0});
                        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
                        തിരികെ;
                    }
                } അല്ലെങ്കിൽ എങ്കിൽ (nodep->dotted() == "") {
                    എങ്കിൽ (nodep->pli()) {
                        എങ്കിൽ (v3Global.opt.bboxSys()) {
                            Ast_കണ്ണി* newp;
                            എങ്കിൽ (VN_IS(nodep, FuncRef)) {
                                newp = പുതിയ AstConst{nodep->fileline(), AstConst::All0{}};
                            } അല്ലെങ്കിൽ {
                                Ast_കണ്ണി_വ്യഞ്ജകം* outp = nullptr;
                                ആകുമ്പോൾ (AstArg* സ്ഥിരം argp = nodep->argsp()) {
                                    outp = Ast_കണ്ണി::addNext(outp, argp->exprp()->unlinkFrBack());
                                    VL_DO_DANGLING(തള്ളുക_Deletep(argp->unlinkFrBack()), argp);
                                }
                                newp = പുതിയ AstSysIgnore{nodep->fileline(), outp};
                                newp->dtypep(nodep->dtypep());
                            }
                            nodep->replaceWith(newp);
                            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
                            തിരികെ;
                        } അല്ലെങ്കിൽ {
                            VSpellCheck speller;
                            V3Parse::candidatePli(&speller);
                            സ്ഥിരം string suggest = speller.best_Candidate_സന്ദേശം(nodep->prettyName());
                            nodep->v3error(
                                "Unsupported or unknown PLI call: "
                                << nodep->prettyNameQ() << '\n'
                                << (suggest.empty() ? "" : nodep->warnMore() + suggest));
                        }
                    } അല്ലെങ്കിൽ {
                        സ്ഥിരം string suggest = m_statep->suggestSymFallback(
                            dotSymp, nodep->name(), Link_കണ്ണി_Matcher_FTask{});
                        nodep->v3error("Can't find definition of task/function: "
                                       << nodep->prettyNameQ() << '\n'
                                       << (suggest.empty() ? "" : nodep->warnMore() + suggest));
                    }
                } അല്ലെങ്കിൽ {
                    സ്ഥിരം string suggest = m_statep->suggestSymFallback(dotSymp, nodep->name(),
                                                                        Link_കണ്ണി_Matcher_FTask{});
                    nodep->v3error("Can't find definition of "
                                   << Ast_കണ്ണി::prettyNameQ(baddot) << " in dotted task/function: '"
                                   << nodep->dotted() + "." + nodep->prettyName() << "'\n"
                                   << (suggest.empty() ? "" : nodep->warnMore() + suggest) << '\n'
                                   << nodep->warnContextPrimary()
                                   << okSymp->cellErrorScopes(nodep));
                }
            }
        }
    }
    ശൂന്യം visit(AstSelBit* nodep) override {
        എങ്കിൽ (nodep->user3SetOnce()) തിരികെ;
        LINKDOT_VISIT_START();
        iterateAndNextNull(nodep->fromp());
        എങ്കിൽ (m_ds.m_unresolvedClass) {
            UASSERT_OBJ(m_ds.m_dotPos != DP_SCOPE && m_ds.m_dotPos != DP_FIRST, nodep,
                        "Object of unresolved class on scope position in dotted reference");
            // The index does not depend on the class, and V3Param must see its class references
            symIterateNull(nodep->bitp(), m_curSymp);
            symIterateNull(nodep->attrp(), m_curSymp);
            തിരികെ;
        }
        എങ്കിൽ (m_ds.m_dotPos == DP_SCOPE
            || m_ds.m_dotPos
                   == DP_FIRST) {  // Already under dot, so this is {modulepart} DOT {modulepart}
            UINFO(9, indent() << "deferring until after a V3Param pass: " << nodep);
            m_ds.m_dotText += "__BRA__??__KET__";
            m_ds.m_unresolvedCell = സത്യം;
            // And pass up m_ds.m_dotText
        }
        // Pass dot state down to only fromp()
        iterateAndNextNull(nodep->fromp());
        symIterateNull(nodep->bitp(), m_curSymp);
        symIterateNull(nodep->attrp(), m_curSymp);
        എങ്കിൽ (m_ds.m_unresolvedCell && (m_ds.m_dotPos == DP_SCOPE || m_ds.m_dotPos == DP_FIRST)) {
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം exprp = nodep->bitp()->unlinkFrBack();
            എങ്കിൽ (AstCellArrayRef* സ്ഥിരം fromArrRefp = VN_CAST(nodep->fromp(), CellArrayRef)) {
                // Multi-dim iface array access: append this select to the existing chain
                fromArrRefp->addSelp(exprp);
                AstCellArrayRef* സ്ഥിരം movedp = fromArrRefp->unlinkFrBack();
                nodep->replaceWith(movedp);
                VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
            } അല്ലെങ്കിൽ {
                AstCellArrayRef* സ്ഥിരം newp
                    = പുതിയ AstCellArrayRef{nodep->fileline(), nodep->fromp()->name(), exprp};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
            }
        }
    }
    ശൂന്യം visit(Ast_കണ്ണി_Pre_Sel* nodep) override {
        // Excludes simple AstSelBit, see above
        എങ്കിൽ (nodep->user3SetOnce()) തിരികെ;
        LINKDOT_VISIT_START();
        എങ്കിൽ (m_ds.m_dotPos == DP_SCOPE
            || m_ds.m_dotPos
                   == DP_FIRST) {  // Already under dot, so this is {modulepart} DOT {modulepart}
            nodep->v3error("Syntax error: Range ':', '+:' etc are not allowed in the instance "
                           "part of a dotted reference");
            m_ds.m_dotErr = സത്യം;
            തിരികെ;
        }
        Ast_കണ്ണി_DType* packedArrayDtp = nullptr;  // Datatype reference for packed array
        {
            VL_RESTORER(m_packedArrayDtp);
            iterateAndNextNull(nodep->fromp());
            symIterateNull(nodep->rhsp(), m_curSymp);
            symIterateNull(nodep->thsp(), m_curSymp);

            എങ്കിൽ (m_packedArrayDtp) {
                AstRange* സ്ഥിരം newRangep
                    = പുതിയ AstRange(nodep->fileline(), nodep->rhsp()->unlinkFrBack(),
                                   nodep->thsp()->unlinkFrBack());
                AstPackArrayDType* newArrayTypep
                    = പുതിയ AstPackArrayDType(nodep->fileline(), m_packedArrayDtp, newRangep);
                newArrayTypep->childDTypep(m_packedArrayDtp);
                newArrayTypep->refDTypep(nullptr);
                എങ്കിൽ (VN_IS(nodep->backp(), SelExtract)) {
                    packedArrayDtp = newArrayTypep;
                } അല്ലെങ്കിൽ {
                    replaceWithCheckBreak(nodep, newArrayTypep);
                    VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
                }
            } അല്ലെങ്കിൽ {
                എങ്കിൽ (nodep->attrp()) {
                    Ast_കണ്ണി* സ്ഥിരം attrp = nodep->attrp()->unlinkFrBack();
                    VL_DO_DANGLING(attrp->delete_വൃക്ഷം(), attrp);
                }
                Ast_കണ്ണി* സ്ഥിരം basefromp = nodep->baseFromp(അസത്യം);
                എങ്കിൽ (VN_IS(basefromp, Replicate)) {
                    // From {...}[...] syntax in IEEE 2017
                    എങ്കിൽ (basefromp) UINFO(9, indent() << " Related node: " << basefromp);
                } അല്ലെങ്കിൽ {
                    nodep->attrp(പുതിയ AstAttrOf{nodep->fileline(), VAttrType::VAR_BASE,
                                               basefromp->clone_വൃക്ഷം(അസത്യം)});
                }
            }
        }

        എങ്കിൽ (packedArrayDtp) m_packedArrayDtp = packedArrayDtp;
    }
    ശൂന്യം visit(AstMemberSel* nodep) override {
        // checkNoDot not appropriate, can be under a dot
        LINKDOT_VISIT_START();
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstGenBlock* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        {
            VL_RESTORER(m_curSymp);
            VL_RESTORER_COPY(m_ds);
            എങ്കിൽ (nodep->name() != "") {
                m_ds.m_dotSymp = m_curSymp = m_statep->get_കണ്ണി_Sym(nodep);
                UINFO(5, indent() << "cur=se" << cvtToHex(m_curSymp));
            }
            iterateChildren(nodep);
        }
        UINFO(5, indent() << "cur=se" << cvtToHex(m_curSymp));
    }
    ശൂന്യം visit(Ast_കണ്ണി_Block* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        {
            VL_RESTORER(m_curSymp);
            VL_RESTORER_COPY(m_ds);
            എങ്കിൽ (nodep->name() != "") {
                m_ds.m_dotSymp = m_curSymp = m_statep->get_കണ്ണി_Sym(nodep);
                UINFO(5, indent() << "cur=se" << cvtToHex(m_curSymp));
            }
            iterateChildren(nodep);
        }
        UINFO(5, indent() << "cur=se" << cvtToHex(m_curSymp));
    }
    ശൂന്യം visit(Ast_കണ്ണി_FTask* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        എങ്കിൽ (nodep->isExternDef() && !nodep->didProto()) {
            nodep->didProto(സത്യം);
            എങ്കിൽ (സ്ഥിരം VSymEnt* സ്ഥിരം foundp
                = m_curSymp->findIdFallback("extern " + nodep->name())) {
                സ്ഥിരം Ast_കണ്ണി_FTask* സ്ഥിരം protop = VN_AS(foundp->nodep(), കണ്ണി_FTask);
                // Copy specifiers.
                // External definition cannot have any specifiers, so no value will be overwritten.
                nodep->isHideLocal(protop->isHideLocal());
                nodep->isHideProtected(protop->isHideProtected());
                nodep->isStatic(protop->isStatic());
                nodep->isVirtual(protop->isVirtual());
                nodep->lifetime(protop->lifetime());
                // Always add an FUNC_RETURN_PROTO even if task, so we can locate the prototype
                nodep->addStmtsp(പുതിയ AstAttrOf{protop->fileline(), VAttrType::FUNC_RETURN_PROTO,
                                               protop->fvarp()
                                                   ? protop->fvarp()->clone_വൃക്ഷം(അസത്യം)
                                                   : പുതിയ AstVoidDType{protop->fileline()}});
                വേണ്ടി (Ast_കണ്ണി* stmtp = protop->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    എങ്കിൽ (AstVar* സ്ഥിരം portp = VN_CAST(stmtp, Var)) {
                        AstAttrOf* സ്ഥിരം attrp
                            = പുതിയ AstAttrOf{protop->fileline(), VAttrType::FUNC_ARG_PROTO,
                                            portp->subDTypep()->clone_വൃക്ഷം(അസത്യം)};
                        attrp->name(portp->name());
                        nodep->addStmtsp(attrp);
                    }
                }
            } അല്ലെങ്കിൽ {
                nodep->v3error("extern not found that declares " + nodep->prettyNameQ());
            }
        }
        എങ്കിൽ (nodep->isExternProto()) {
            എങ്കിൽ (!m_curSymp->findIdFallback(nodep->name())) {
                nodep->v3warn(PROTOTYPEMIS,
                              "Definition not found for extern prototype " + nodep->prettyNameQ());
            }
        }
        VL_RESTORER(m_curSymp);
        VL_RESTORER(m_ftaskp);
        VL_RESTORER(m_explicitSuperNewp);
        {
            m_ftaskp = nodep;
            m_ds.m_dotSymp = m_curSymp = m_statep->get_കണ്ണി_Sym(nodep);
            സ്ഥിരം തർക്കം isNew = nodep->name() == "new";
            എങ്കിൽ (isNew) m_explicitSuperNewp = nullptr;
            iterateChildren(nodep);
            എങ്കിൽ (isNew) {
                സ്ഥിരം AstClassExtends* സ്ഥിരം classExtendsp = VN_AS(m_modp, Class)->extendsp();
                എങ്കിൽ (m_explicitSuperNewp && !m_explicitSuperNewp->isImplicit() && classExtendsp
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
                എങ്കിൽ (classExtendsp && classExtendsp->classOrNullp()) {
                    എങ്കിൽ (!m_explicitSuperNewp && m_statep->forParamed()
                        && !VN_AS(classExtendsp->childDTypep()->skipRefp(), ClassRefDType)
                                ->classp()
                                ->isInterfaceClass()) {
                        Ast_കണ്ണി_Stmt* സ്ഥിരം superNewp
                            = addImplicitSuperNewCall(VN_AS(nodep, Func), classExtendsp);
                        UINFO(9, "created super new " << superNewp);
                        iterate(superNewp);
                    }
                }
            }
        }
        m_ds.m_dotSymp = VL_RESTORER_PREV(m_curSymp);
    }
    ശൂന്യം visit(Ast_കണ്ണി_Foreach* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        symIterateChildren(nodep, m_statep->get_കണ്ണി_Sym(nodep));
    }
    ശൂന്യം visit(AstRandSequence* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        എങ്കിൽ (!nodep->start().empty())
            nodep->prodp(findProd(nodep, m_statep->get_കണ്ണി_Sym(nodep), nodep->start()));
        symIterateChildren(nodep, m_statep->get_കണ്ണി_Sym(nodep));
    }
    ശൂന്യം visit(AstRSProd* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        symIterateChildren(nodep, m_statep->get_കണ്ണി_Sym(nodep));
    }
    ശൂന്യം visit(AstRSProdItem* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        nodep->prodp(findProd(nodep, m_curSymp, nodep->name()));
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstCoverWith* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        iterateAndNextNull(nodep->subp());
        // The type of 'item' is that of the coverpoint expression, which may name 'item' too
        iterateAndNextNull(nodep->itemp()->childDTypep());
        VL_RESTORER(m_curSymp);
        m_ds.m_dotSymp = m_curSymp = m_statep->get_കണ്ണി_Sym(nodep);
        iterateAndNextNull(nodep->itemp());
        iterateAndNextNull(nodep->filterp());
        m_ds.m_dotSymp = VL_RESTORER_PREV(m_curSymp);
    }
    ശൂന്യം visit(AstCoverCrossWith* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        iterateAndNextNull(nodep->selectp());
        iterateAndNextNull(nodep->countp());
        // The type of a cross item is that of its coverpoint's expression, which may name items
        iterateAndNextNull(nodep->varsp());
        VL_RESTORER(m_curSymp);
        m_ds.m_dotSymp = m_curSymp = m_statep->get_കണ്ണി_Sym(nodep);
        iterateAndNextNull(nodep->filterp());
        m_ds.m_dotSymp = VL_RESTORER_PREV(m_curSymp);
    }
    ശൂന്യം visit(AstWith* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        എങ്കിൽ (m_ds.m_unresolvedClass) തിരികെ;
        VL_RESTORER(m_curSymp);
        VL_RESTORER(m_currentWithp);
        VL_RESTORER_COPY(m_restrictedNamesUsed);
        {
            m_ds.m_dotSymp = m_curSymp = m_statep->get_കണ്ണി_Sym(nodep);
            m_currentWithp = nodep;
            // Validate the identifier_list once per AstWith (m_validated travels
            // with cloneTree, so cloned witnesses skip the redundant check).
            സ്ഥിരം തർക്കം firstVisit = !nodep->validated();
            എങ്കിൽ (firstVisit) {
                nodep->validated(സത്യം);
                m_restrictedNamesUsed.clear();
                // IEEE 1800-2023 18.7: each name in the identifier_list must designate
                // a member of the randomize() target class.
                എങ്കിൽ (nodep->restricted() && m_randSymp) {
                    വേണ്ടി (സ്ഥിരം std::string& n : nodep->restrictedNames()) {
                        എങ്കിൽ (!m_randSymp->findIdFlat(n)) {
                            nodep->v3error("Identifier '"
                                           << n
                                           << "' in 'with (...)' identifier list is not a"
                                              " member of the randomize() target class"
                                              " (IEEE 1800-2023 18.7)");
                            m_restrictedNamesUsed.തിരുകുക(n);
                        }
                    }
                }
            }
            iterateChildren(nodep);
            // Flag names that the user listed but never referenced in the constraint.
            എങ്കിൽ (firstVisit && nodep->restricted()) {
                വേണ്ടി (സ്ഥിരം std::string& n : nodep->restrictedNames()) {
                    എങ്കിൽ (!m_restrictedNamesUsed.എണ്ണം(n)) {
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
    ശൂന്യം visit(AstLambdaArgRef* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        // No checknodot(nodep), visit(AstScope) will check for LambdaArgRef
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstClassExtends* nodep) override {
        // Resolve the symbol and get the class.
        // If it is a parameterized case, the class will be resolved after V3Param.cpp
        എങ്കിൽ (nodep->user3SetOnce()) തിരികെ;
        // If the class is resolved, there is nothing more to do
        എങ്കിൽ (nodep->classOrNullp()) തിരികെ;
        LINKDOT_VISIT_START();
        എങ്കിൽ (m_statep->forPrimary()) {
            എങ്കിൽ (nodep->childDTypep()) തിരികെ;
            Ast_കണ്ണി* cprp = nodep->classOrPkgsp();
            VSymEnt* lookSymp = m_curSymp;
            എങ്കിൽ (AstDot* സ്ഥിരം dotp = VN_CAST(cprp, Dot)) {
                dotp->user3(സത്യം);
                എങ്കിൽ (AstClassOrPackageRef* lookNodep = VN_CAST(dotp->lhsp(), ClassOrPackageRef)) {
                    iterate(lookNodep);
                    cprp = dotp->rhsp();
                    VSymEnt* സ്ഥിരം foundp = m_statep->resolveClassOrPackage(
                        lookSymp, lookNodep, സത്യം, അസത്യം, nodep->verilogKwd());
                    എങ്കിൽ (!foundp) {
                        VL_DO_DANGLING(തള്ളുക_Deletep(nodep->unlinkFrBack()), nodep);
                        തിരികെ;
                    }
                    UASSERT_OBJ(lookNodep->classOrPackageSkipp(), nodep, "Bad package link");
                    lookSymp = m_statep->get_കണ്ണി_Sym(lookNodep->classOrPackageSkipp());
                } അല്ലെങ്കിൽ {
                    dotp->lhsp()->v3error("Attempting to extend"  // LCOV_EXCL_LINE
                                          " using non-class under dot");
                }
            }
            AstClassOrPackageRef* സ്ഥിരം cpackagerefp = VN_CAST(cprp, ClassOrPackageRef);
            എങ്കിൽ (VL_UNCOVERABLE(!cpackagerefp)) {
                // Linking the extend gives an error before this is hit
                nodep->v3error("Attempting to extend using non-class");  // LCOV_EXCL_LINE
                VL_DO_DANGLING(തള്ളുക_Deletep(nodep->unlinkFrBack()), nodep);
                തിരികെ;
            }
            VSymEnt* സ്ഥിരം foundp = m_statep->resolveClassOrPackage(lookSymp, cpackagerefp, സത്യം,
                                                                    സത്യം, nodep->verilogKwd());
            എങ്കിൽ (!foundp) {
                VL_DO_DANGLING(തള്ളുക_Deletep(nodep->unlinkFrBack()), nodep);
                തിരികെ;
            }
            എങ്കിൽ (AstClass* സ്ഥിരം classp = VN_CAST(foundp->nodep(), Class)) {
                AstPin* paramsp = cpackagerefp->paramsp();
                എങ്കിൽ (paramsp) {
                    paramsp = paramsp->clone_വൃക്ഷം(സത്യം);
                    nodep->parameterized(സത്യം);
                }
                nodep->childDTypep(പുതിയ AstClassRefDType{nodep->fileline(), classp, paramsp});
                // Link pins
                iterate(nodep->childDTypep());
            } അല്ലെങ്കിൽ എങ്കിൽ (AstParamTypeDType* സ്ഥിരം paramp
                       = VN_CAST(foundp->nodep(), ParamTypeDType)) {
                AstRefDType* സ്ഥിരം refParamp = പുതിയ AstRefDType{nodep->fileline(), paramp->name()};
                refParamp->refDTypep(paramp);
                nodep->childDTypep(refParamp);
                nodep->parameterized(സത്യം);
            } അല്ലെങ്കിൽ എങ്കിൽ (AstTypedef* സ്ഥിരം typedefp = VN_CAST(foundp->nodep(), Typedef)) {
                Ast_കണ്ണി_DType* സ്ഥിരം unwrappedp = typedefp->subDTypep()->skipRefp();
                എങ്കിൽ (AstClassRefDType* സ്ഥിരം classRefp = VN_CAST(unwrappedp, ClassRefDType)) {
                    AstPin* paramsp = cpackagerefp->paramsp();
                    എങ്കിൽ (!paramsp && classRefp->paramsp()) {
                        // No explicit #(...) on extends clause; carry over the
                        // typedef's type-parameter pins so V3Param sees them.
                        paramsp = classRefp->paramsp()->clone_വൃക്ഷം(സത്യം);
                        nodep->parameterized(സത്യം);
                    } അല്ലെങ്കിൽ എങ്കിൽ (paramsp) {
                        paramsp = paramsp->clone_വൃക്ഷം(സത്യം);
                        nodep->parameterized(സത്യം);
                    }
                    nodep->childDTypep(
                        പുതിയ AstClassRefDType{nodep->fileline(), classRefp->classp(), paramsp});
                    iterate(nodep->childDTypep());
                } അല്ലെങ്കിൽ {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: " << foundp->nodep()->prettyTypeName()
                                                  << " in 'class extends'");
                    തിരികെ;
                }
            } അല്ലെങ്കിൽ {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: " << foundp->nodep()->prettyTypeName()
                                                             << " in 'class extends'");
                തിരികെ;
            }
            എങ്കിൽ (!nodep->childDTypep()) nodep->v3error("Attempting to extend using non-class");
            nodep->classOrPkgsp()->unlinkFrBack()->delete_വൃക്ഷം();
        } അല്ലെങ്കിൽ {
            // Probably a parameter
            എങ്കിൽ (AstRefDType* സ്ഥിരം refp = VN_CAST(nodep->childDTypep(), RefDType)) {
                എങ്കിൽ (AstClassRefDType* classRefp = VN_CAST(refp->skipRefp(), ClassRefDType)) {
                    // Resolved to a class reference.
                    refp->replaceWith(classRefp->clone_വൃക്ഷം(അസത്യം));
                    VL_DO_DANGLING(തള്ളുക_Deletep(refp), refp);
                } അല്ലെങ്കിൽ {
                    // Unable to resolve the ref type to a class reference.
                    // Get the value of type parameter passed to the class instance,
                    // to print the helpful error message.
                    സ്ഥിരം Ast_കണ്ണി_DType* typep = refp->refDTypep();
                    ആകുമ്പോൾ (സത്യം) {
                        എങ്കിൽ (സ്ഥിരം AstParamTypeDType* സ്ഥിരം atypep
                            = VN_CAST(typep, ParamTypeDType)) {
                            typep = atypep->subDTypep();
                            തുടരുക;
                        }
                        എങ്കിൽ (സ്ഥിരം AstRequireDType* സ്ഥിരം atypep = VN_CAST(typep, RequireDType)) {
                            typep = atypep->subDTypep();
                            തുടരുക;
                        }
                        നിർത്തുക;
                    }
                    typep->v3error("Attempting to extend using non-class");
                }
            }
        }
    }
    ശൂന്യം visit(AstClass* nodep) override {
        എങ്കിൽ (nodep->user3SetOnce()) തിരികെ;
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
            m_ds.m_dotSymp = m_curSymp = m_modSymp = m_statep->get_കണ്ണി_Sym(nodep);
            m_modp = nodep;
            പൂർണ്ണസംഖ്യ next = 0;
            വേണ്ടി (AstClassExtends* cextp = nodep->extendsp(); cextp;
                 cextp = VN_AS(cextp->nextp(), ClassExtends)) {
                // Replace abstract reference with hard pointer
                // Will need later resolution when deal with parameters
                എങ്കിൽ (++next == 2 && !nodep->isInterfaceClass() && !cextp->isImplements()) {
                    cextp->v3error("Multiple inheritance illegal on non-interface classes"
                                   " (IEEE 1800-2023 8.13)");
                }
                iterate(cextp);
                എങ്കിൽ (m_statep->forPrimary()) {
                    എങ്കിൽ (cextp->parameterized()) {
                        // Parameters in extends statement.
                        // The class can't be resolved in the current pass.
                        m_extendsParam.തിരുകുക(nodep);
                        m_insideClassExtParam = സത്യം;
                    }
                    എങ്കിൽ (AstClassRefDType* സ്ഥിരം classRefp
                        = VN_CAST(cextp->childDTypep(), ClassRefDType)) {
                        AstClass* സ്ഥിരം classp = classRefp->classp();
                        എങ്കിൽ (classp != nodep) iterate(classp);
                        എങ്കിൽ (m_extendsParam.find(classp) != m_extendsParam.end()) {
                            // One of its super classes has parameters in extends statement.
                            // Some links may not be resolved in the first pass.
                            m_extendsParam.തിരുകുക(nodep);
                            m_insideClassExtParam = സത്യം;
                        }
                    }
                }

                എങ്കിൽ (AstClass* സ്ഥിരം baseClassp = cextp->classOrNullp()) {
                    // Already converted. Update symbol table to link unlinked members.
                    // Base class has to be visited in a case if its extends statement
                    // needs to be handled. Recursive inheritance was already checked.
                    // Must be here instead of in LinkDotParam to handle
                    // "class (type T) extends T".
                    എങ്കിൽ (baseClassp == nodep) {
                        cextp->v3error("Attempting to extend class " << nodep->prettyNameQ()
                                                                     << " from itself");
                    } അല്ലെങ്കിൽ എങ്കിൽ (extendsClass(baseClassp, nodep)) {
                        cextp->v3error("Attempting to extend class "
                                       << nodep->prettyNameQ() << " from "
                                       << baseClassp->prettyNameQ()
                                       << ", which circularly inherits from it");
                    } അല്ലെങ്കിൽ എങ്കിൽ (cextp->isImplements() && !baseClassp->isInterfaceClass()) {
                        cextp->v3error("Attempting to implement from non-interface class "
                                       << baseClassp->prettyNameQ() << '\n'
                                       << "... Suggest use 'extends'");
                    } അല്ലെങ്കിൽ എങ്കിൽ (!cextp->isImplements() && !nodep->isInterfaceClass()
                               && baseClassp->isInterfaceClass()) {
                        cextp->v3error("Attempting to extend from interface class "
                                       << baseClassp->prettyNameQ() << '\n'
                                       << "... Suggest use 'implements'");
                    }
                    baseClassp->isExtended(സത്യം);
                    nodep->isExtended(സത്യം);
                    iterate(baseClassp);
                    importSymbolsFromExtended(nodep, cextp);
                    തുടരുക;
                }
            }
            m_ds.m_dotSymp = m_curSymp;

            iterateChildren(nodep);
            വേണ്ടി (സ്ഥിരം auto& itr : m_ifForwards) {
                സ്ഥിരം VSymEnt* സ്ഥിരം implSymp = itr.second;
                addInterfaceForward(nodep, VN_AS(implSymp->nodep(), കണ്ണി_FTask),
                                    VN_AS(implSymp->parentp()->nodep(), Class));
            }
        }
        // V3Width when determines types needs to find enum values and such
        // so add members pointing to appropriate enum values
        {
            VMemberMap memberMap;
            വേണ്ടി (VSymEnt::const_iterator it = m_curSymp->begin(); it != m_curSymp->end(); ++it) {
                Ast_കണ്ണി* സ്ഥിരം itemp = it->second->nodep();
                എങ്കിൽ (!memberMap.findMember(nodep, it->first)) {
                    എങ്കിൽ (AstEnumItem* സ്ഥിരം aitemp = VN_CAST(itemp, EnumItem)) {
                        AstEnumItemRef* സ്ഥിരം newp = പുതിയ AstEnumItemRef{
                            aitemp->fileline(), aitemp, it->second->classOrPackagep()};
                        UINFO(8, indent()
                                     << "Class import noderef '" << it->first << "' " << newp);
                        nodep->addMembersp(newp);
                        memberMap.തിരുകുക(nodep, newp);
                    }
                }
            }
        }
        m_ds.m_dotSymp = VL_RESTORER_PREV(m_curSymp);
    }

    // Collect the ClassOrPackageRefs in a chained scope operand in source order.
    // The grammar builds a scope operand only from packageClassScopeItem, which
    // always yields a ClassOrPackageRef, nested in Dots, so nothing else appears.
    സ്ഥിതം ശൂന്യം collectScopeRefs(Ast_കണ്ണി* nodep, std::vector<AstClassOrPackageRef*>& refps) {
        എങ്കിൽ (AstClassOrPackageRef* സ്ഥിരം refp = VN_CAST(nodep, ClassOrPackageRef)) {
            refps.തള്ളുക_back(refp);
            തിരികെ;
        }
        AstDot* സ്ഥിരം dotp = VN_CAST(nodep, Dot);
        UASSERT_OBJ(dotp, nodep, "Non-scope node in package/class scope operand");
        collectScopeRefs(dotp->lhsp(), refps);
        collectScopeRefs(dotp->rhsp(), refps);
    }

    // Resolve a chained class-scope operand (`pkg::outer::inner` in
    // `pkg::outer::inner::t`) and reduce it to its innermost ClassOrPackageRef.
    // Returns false, having reported, if a segment could not be resolved.
    തർക്കം reduceScopeDot(AstRefDType* nodep, AstDot* scopeDotp) {
        std::vector<AstClassOrPackageRef*> refps;
        collectScopeRefs(scopeDotp, refps);

        VSymEnt* scopeSymp = m_ds.m_dotSymp;
        വേണ്ടി (size_t i = 0; i < refps.size(); ++i) {
            AstClassOrPackageRef* സ്ഥിരം refp = refps[i];
            എങ്കിൽ (!refp->classOrPackageSkipp() && !refp->classOrPackageNodep()
                && !m_statep->resolveClassOrPackage(scopeSymp, refp, i == 0, അസത്യം,
                                                    "class/package reference")) {
                തിരികെ അസത്യം;  // Error already reported
            }
            എങ്കിൽ (i + 1 == refps.size()) നിർത്തുക;
            // A middle segment that resolved to a node with no reachable module
            // (e.g. a typedef alias of a not-yet-specialized parameterized class)
            // gives nothing to look the next segment up in.
            Ast_കണ്ണി_Module* സ്ഥിരം modp = refp->classOrPackageSkipp();
            എങ്കിൽ (!modp) {
                refp->v3warn(E_UNSUPPORTED, "Unsupported: Multiple '::' package/class reference");
                തിരികെ അസത്യം;
            }
            scopeSymp = m_statep->get_കണ്ണി_Sym(modp);
        }
        AstClassOrPackageRef* സ്ഥിരം innerp = refps.back();
        innerp->unlinkFrBack();
        VL_DO_DANGLING(തള്ളുക_Deletep(scopeDotp->unlinkFrBack()), scopeDotp);
        nodep->classOrPackageOpp(innerp);
        തിരികെ സത്യം;
    }

    ശൂന്യം visit(AstRefDType* nodep) override {

        എങ്കിൽ (auto* സ്ഥിരം typeOfp = nodep->typeofp()) {
            iterate(typeOfp);
            // After iteration, typeofp() may have been replaced with a resolved type
            // (e.g., DOT expression like if0.rq_t resolved to REFDTYPE)
            // If it's now a NodeDType, replace this RefDType with it
            എങ്കിൽ (Ast_കണ്ണി_DType* സ്ഥിരം resolvedDTypep = VN_CAST(nodep->typeofp(), കണ്ണി_DType)) {
                UINFO(9,
                      indent() << "iface capture typeofp resolved to dtype, replacing RefDType\n");
                resolvedDTypep->unlinkFrBack();
                nodep->replaceWith(resolvedDTypep);
                VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
                // Originally (PR #6637) this block re-captured a resolved
                // RefDType into the iface-capture ledger when user2p held a
                // Cell, to ensure re-resolution during the paramed pass.
                // In practice, the typeofp-resolved RefDType never carries a
                // Cell in user2p: the capture happens earlier when the
                // typedef is first linked.
                എങ്കിൽ (V3LinkDotIfaceCapture::enabled()) {
                    എങ്കിൽ (AstRefDType* സ്ഥിരം resolvedRefp = VN_CAST(resolvedDTypep, RefDType)) {
                        എങ്കിൽ (VL_UNCOVERABLE(
                                VN_IS(resolvedRefp->user2p(), Cell))) {  // LCOV_EXCL_LINE
                            resolvedRefp->v3fatalSrc(  // LCOV_EXCL_LINE
                                "typeofp resolved RefDType has Cell in user2p;"
                                " expected to be captured already");
                        }
                    }
                }
            }
            തിരികെ;
        }

        // Resolve its reference
        എങ്കിൽ (m_statep->forParamed() && nodep->user3()) {
            എങ്കിൽ (V3LinkDotIfaceCapture::enabled() && nodep->user2p()) {
                UINFO(9, indent() << "iface capture clear user3 for captured typedef name="
                                  << nodep->name() << " cell=" << nodep->user2p());
            }
            nodep->user3(അസത്യം);
        }
        എങ്കിൽ (nodep->user3SetOnce()) {
            എങ്കിൽ (V3LinkDotIfaceCapture::enabled() && nodep->user2p()) {
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
            എങ്കിൽ (VL_UNCOVERABLE(m_statep->forParamed() && nodep->classOrPackageOpp())) {
                nodep->v3fatalSrc(  // LCOV_EXCL_LINE
                    "RefDType user3 revisit with classOrPackageOpp in paramed pass");
            }
            തിരികെ;
        }
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        എങ്കിൽ (nodep->classOrPackageOpp()) {
            // Resolve a chained scope (`pkg::cls::t`, `pkg::cls#(P)::t`) from outside in.
            // On failure reduceScopeDot has reported, so stop before using the operand.
            എങ്കിൽ (AstDot* സ്ഥിരം scopeDotp = VN_CAST(nodep->classOrPackageOpp(), Dot)) {
                എങ്കിൽ (!reduceScopeDot(nodep, scopeDotp)) തിരികെ;
            }
            // Re-read: reduceScopeDot may have replaced the operand
            Ast_കണ്ണി* സ്ഥിരം cpackagep = nodep->classOrPackageOpp();
            UASSERT_OBJ(!VN_IS(cpackagep, Dot), cpackagep,
                        "Package/class scope operand should be reduced by now");
            എങ്കിൽ (AstClassOrPackageRef* സ്ഥിരം cpackagerefp = VN_CAST(cpackagep, ClassOrPackageRef)) {
                iterate(cpackagerefp);
                സ്ഥിരം AstClass* സ്ഥിരം clsp = VN_CAST(cpackagerefp->classOrPackageNodep(), Class);
                എങ്കിൽ (clsp && clsp->hasGParam()) {
                    // Unable to link before the instantiation of parameter classes.
                    // The class reference node still has to be visited now to later link
                    // parameters.
                    iterate(cpackagep);
                    തിരികെ;
                }
                // Defer non-typedef references through typedef aliases of parameterized classes.
                എങ്കിൽ (m_statep->forPrimary() && !VN_IS(nodep->backp(), Typedef)
                    && isParamedClassRef(cpackagerefp)) {
                    iterate(cpackagep);
                    തിരികെ;
                }

                സ്ഥിരം തർക്കം doDefaultTypedef = !(m_resolvingTypedef && m_statep->forPrimary());
                എങ്കിൽ (!cpackagerefp->classOrPackageSkipp(doDefaultTypedef)
                    && !cpackagerefp->classOrPackageNodep()) {
                    VSymEnt* സ്ഥിരം foundp = m_statep->resolveClassOrPackage(
                        m_ds.m_dotSymp, cpackagerefp, സത്യം, അസത്യം, "class/package reference");
                    എങ്കിൽ (!foundp) തിരികെ;
                }
                nodep->classOrPackagep(cpackagerefp->classOrPackageSkipp(doDefaultTypedef));
                എങ്കിൽ (!VN_IS(nodep->classOrPackagep(), Class)
                    && !VN_IS(nodep->classOrPackagep(), Package)) {
                    എങ്കിൽ (m_statep->forPrimary()) {
                        // It may be a type that comes from parameter class that is not
                        // instantioned yet
                        iterate(cpackagep);
                        തിരികെ;
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
            VL_DO_DANGLING(തള്ളുക_Deletep(cpackagep->unlinkFrBack()), cpackagep);
        }

        എങ്കിൽ (m_ds.m_dotp && (m_ds.m_dotPos == DP_PACKAGE || m_ds.m_dotPos == DP_SCOPE)) {
            UASSERT_OBJ(VN_IS(m_ds.m_dotp->lhsp(), ClassOrPackageRef), m_ds.m_dotp->lhsp(),
                        "Bad package link");
            auto* സ്ഥിരം cpackagerefp = VN_CAST(m_ds.m_dotp->lhsp(), ClassOrPackageRef);
            UASSERT_OBJ(cpackagerefp->classOrPackageSkipp(), m_ds.m_dotp->lhsp(),
                        "Bad package link");
            nodep->classOrPackagep(cpackagerefp->classOrPackageSkipp());
            m_ds.m_dotPos = DP_SCOPE;
        } അല്ലെങ്കിൽ {
            // Allow REFDTYPE under DOT when referencing interface typedefs
            // This is needed for patterns like: typedef iface.a_t a_t;
            // The dependency graph handles resolution when IfaceCapture is disabled
            സ്ഥിരം തർക്കം ifaceFinalSegmentAllowed
                = (m_ds.m_dotPos == DP_FINAL) && m_ds.m_dotSymp
                  && VN_IS(m_ds.m_dotSymp->nodep(), Cell)
                  && VN_CAST(m_ds.m_dotSymp->nodep(), Cell)->modp()
                  && VN_IS(VN_CAST(m_ds.m_dotSymp->nodep(), Cell)->modp(), Iface);
            എങ്കിൽ (!ifaceFinalSegmentAllowed) {
                checkNoDot(nodep);
            } അല്ലെങ്കിൽ {
                UINFO(9, indent() << "allow REFDTYPE under DOT for iface typedef name="
                                  << nodep->prettyNameQ() << " dotSym=" << m_ds.m_dotSymp);
                // Clear the dot state so we don't propagate errors
                m_ds.m_dotPos = DP_SCOPE;
            }
        }
        എങ്കിൽ (!nodep->typedefp() && !nodep->subDTypep()) {
            സ്ഥിരം VSymEnt* foundp;
            എങ്കിൽ (nodep->classOrPackagep()) {
                foundp = m_statep->get_കണ്ണി_Sym(nodep->classOrPackagep())->findIdFlat(nodep->name());
            } അല്ലെങ്കിൽ എങ്കിൽ (m_ds.m_dotPos == DP_FIRST || m_ds.m_dotPos == DP_NONE) {
                foundp = findIdFallbackSkipMemberDType(m_curSymp, nodep->name());
            } അല്ലെങ്കിൽ {
                // Defensive: dotPos should be DP_FIRST/DP_NONE or classOrPackagep set.
                v3fatalSrc("Unexpected dotPos="
                           << static_cast<പൂർണ്ണസംഖ്യ>(m_ds.m_dotPos)  // LCOV_EXCL_LINE
                           << " in RefDType lookup for "
                           << nodep->prettyNameQ());  // LCOV_EXCL_LINE
                foundp = nullptr;  // LCOV_EXCL_LINE
            }
            {
                എങ്കിൽ (AstTypedef* സ്ഥിരം defp
                    = foundp ? VN_CAST(foundp->nodep(), Typedef) : nullptr) {
                    // Don't check if typedef is to a <type T>::<reference> as might not be
                    // resolved yet
                    എങ്കിൽ (!nodep->classOrPackagep() && !defp->isUnderClass())
                        checkDeclOrder(nodep, defp);
                    nodep->typedefp(defp);
                    nodep->classOrPackagep(foundp->classOrPackagep());

                } അല്ലെങ്കിൽ എങ്കിൽ (AstParamTypeDType* സ്ഥിരം defp
                           = foundp ? VN_CAST(foundp->nodep(), ParamTypeDType) : nullptr) {
                    // A PARAMTYPEDTYPE's child REFDTYPE referencing itself is the normal
                    // AST structure for type parameters (e.g., "type T = base" has a child
                    // REFDTYPE for the default type). Only error on true recursion where
                    // a TYPEDEF contains a reference back to itself.
                    സ്ഥിരം തർക്കം isParamTypeChild = (defp == nodep->backp());
                    സ്ഥിരം തർക്കം isTypedefRecursion
                        = isParamTypeChild && VN_IS(defp->backp(), Typedef);
                    എങ്കിൽ (isTypedefRecursion) {
                        nodep->v3error("Reference to '"
                                       << m_ds.m_dotText << (m_ds.m_dotText == "" ? "" : ".")
                                       << nodep->prettyName() << "'"
                                       << " type would form a recursive definition");
                        nodep->refDTypep(nodep->findVoidDType());  // Try to reduce later errors
                    } അല്ലെങ്കിൽ {
                        nodep->refDTypep(defp);
                        nodep->classOrPackagep(foundp->classOrPackagep());
                        captureIfaceParamType(nodep, defp);
                    }
                } അല്ലെങ്കിൽ എങ്കിൽ (AstClass* സ്ഥിരം defp
                           = foundp ? VN_CAST(foundp->nodep(), Class) : nullptr) {
                    // Don't check if typedef is to a <type T>::<reference> as might not be
                    // resolved yet
                    എങ്കിൽ (!nodep->classOrPackagep()) checkDeclOrder(nodep, defp);
                    AstPin* സ്ഥിരം paramsp = nodep->paramsp();
                    എങ്കിൽ (paramsp) paramsp->unlinkFrBackWithNext();
                    AstClassRefDType* സ്ഥിരം newp
                        = പുതിയ AstClassRefDType{nodep->fileline(), defp, paramsp};
                    newp->classOrPackagep(foundp->classOrPackagep());
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
                    തിരികെ;
                } അല്ലെങ്കിൽ എങ്കിൽ (m_insideClassExtParam) {
                    തിരികെ;
                } അല്ലെങ്കിൽ {
                    എങ്കിൽ (foundp) {
                        UINFO(1, "Found sym node: " << foundp->nodep());
                        nodep->v3error("Expecting a data type: "
                                       << nodep->prettyNameQ()
                                       << ", found: " << foundp->nodep()->prettyTypeName());
                    } അല്ലെങ്കിൽ {
                        nodep->v3error("Can't find typedef/interface: " << nodep->prettyNameQ());
                    }
                }
            }
        }
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstRequireDType* nodep) override {
        // Handle type validation for localparam type assignments
        // Iterate child to resolve any ParseRef nodes
        iterateChildren(nodep);
        // Only emit error if the child is not a type.
        // Do NOT unwrap valid types here - leave that to V3Width.
        // Unwrapping here breaks type parameter resolution during cloning.
        // Allow Dot through to defer the handling until the dot expression is resolved.
        എങ്കിൽ (nodep->lhsp() && !VN_IS(nodep->lhsp(), കണ്ണി_DType) && !VN_IS(nodep->lhsp(), Dot)) {
            // Not a type - emit error
            എങ്കിൽ (VN_IS(nodep->lhsp(), Const)) {
                nodep->lhsp()->v3error("Expecting a data type, not a constant");
            } അല്ലെങ്കിൽ {
                nodep->lhsp()->v3error("Expecting a data type, not "
                                       << nodep->lhsp()->typeName() << ": '"
                                       << nodep->lhsp()->prettyName() << "'");
            }
            // Replace with void to keep AST valid and allow clean exit
            nodep->replaceWith(പുതിയ AstVoidDType{nodep->fileline()});
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
        }
    }
    ശൂന്യം visit(AstDpiExport* nodep) override {
        // AstDpiExport: Make sure the function referenced exists, then dump it
        LINKDOT_VISIT_START();
        iterateChildren(nodep);
        checkNoDot(nodep);
        VSymEnt* സ്ഥിരം foundp = m_curSymp->findIdFallback(nodep->name());
        Ast_കണ്ണി_FTask* സ്ഥിരം taskp = foundp ? VN_AS(foundp->nodep(), കണ്ണി_FTask) : nullptr;
        എങ്കിൽ (!taskp) {
            nodep->v3error(
                "Can't find definition of exported task/function: " << nodep->prettyNameQ());
        } അല്ലെങ്കിൽ എങ്കിൽ (taskp->dpiExport()) {
            nodep->v3error("Function was already DPI Exported, duplicate not allowed: "
                           << nodep->prettyNameQ());
        } അല്ലെങ്കിൽ {
            taskp->dpiExport(സത്യം);
            എങ്കിൽ (nodep->cname() != "") taskp->cname(nodep->cname());
        }
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep->unlinkFrBack()), nodep);
    }
    ശൂന്യം visit(AstDisable* nodep) override {
        LINKDOT_VISIT_START();
        checkNoDot(nodep);
        തർക്കം hasPartSelect = അസത്യം;
        സ്ഥിരം string targetPath
            = nodep->targetRefp() ? extractDottedPath(nodep->targetRefp(), hasPartSelect) : "";
        VL_RESTORER_COPY(m_ds);
        m_ds.init(m_curSymp);
        m_ds.m_dotPos = DP_FIRST;
        m_ds.m_disablep = nodep;
        iterateChildren(nodep);
        എങ്കിൽ (nodep->targetRefp()) {
            എങ്കിൽ (AstTaskRef* സ്ഥിരം taskRefp = VN_CAST(nodep->targetRefp(), TaskRef)) {
                nodep->targetp(taskRefp->taskp());
            } അല്ലെങ്കിൽ എങ്കിൽ (!VN_IS(nodep->targetRefp(), ParseRef)) {
                // If it is a ParseRef, either it couldn't be linked or it is linked to a block
                nodep->v3warn(E_UNSUPPORTED, "Node of type "
                                                 << nodep->targetRefp()->prettyTypeName()
                                                 << " referenced by disable");
                VL_DO_DANGLING(തള്ളുക_Deletep(nodep->unlinkFrBack()), nodep);
                തിരികെ;
            }
            എങ്കിൽ (nodep->targetp()) {
                nodep->targetRefp()->unlinkFrBack()->delete_വൃക്ഷം();
                എങ്കിൽ (!hasPartSelect) {
                    സ്ഥിരം string instancePath = targetInstancePath(nodep->targetp(), targetPath);
                    എങ്കിൽ (!instancePath.empty()) {
                        // Keep only the instance prefix so V3LinkJump can reference the selected
                        // instance's process queue without reparsing the disable target.
                        nodep->targetRefp(
                            പുതിയ AstVarXRef{nodep->fileline(), "", instancePath, VAccess::READ});
                    }
                }
            }
        }
    }
    ശൂന്യം visitPackageImportOrExport(Ast_കണ്ണി* nodep) {
        // No longer needed
        LINKDOT_VISIT_START();
        checkNoDot(nodep);
        എങ്കിൽ (m_statep->forParamed()) VL_DO_DANGLING(തള്ളുക_Deletep(nodep->unlinkFrBack()), nodep);
    }
    ശൂന്യം visit(AstPackageImport* nodep) override { visitPackageImportOrExport(nodep); }
    ശൂന്യം visit(AstPackageExport* nodep) override { visitPackageImportOrExport(nodep); }
    ശൂന്യം visit(AstPackageExportStarStar* nodep) override { visitPackageImportOrExport(nodep); }
    ശൂന്യം visit(AstCellRef* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstCellArrayRef* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        // Expression already iterated
    }
    ശൂന്യം visit(AstUnlinkedRef* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        // No need to iterate, if we have a UnlinkedVarXRef, we're already done
    }
    ശൂന്യം visit(Ast_Stmt_വ്യഞ്ജകം* nodep) override {
        LINKDOT_VISIT_START();
        checkNoDot(nodep);
        // Check if nodep represents a super.new call;
        എങ്കിൽ (AstNew* സ്ഥിരം newExprp = VN_CAST(nodep->exprp(), New)) {
            // in this case it was already linked, so it doesn't have a super reference
            m_explicitSuperNewp = newExprp;
        } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstDot* സ്ഥിരം dotp = VN_CAST(nodep->exprp(), Dot)) {
            എങ്കിൽ (dotp->lhsp()->name() == "super") {
                എങ്കിൽ (AstNew* സ്ഥിരം newExprp = VN_CAST(dotp->rhsp(), New)) {
                    m_explicitSuperNewp = newExprp;
                }
            }
        }
        iterateChildren(nodep);
    }

    ശൂന്യം visit(AstIfaceRefDType* nodep) override {
        എങ്കിൽ (nodep->user3SetOnce()) തിരികെ;
        LINKDOT_VISIT_START();
        എങ്കിൽ (nodep->paramsp()) {
            // If there is no parameters, there is no need to visit this node.
            AstIface* സ്ഥിരം ifacep = nodep->ifacep();
            UASSERT_OBJ(ifacep, nodep, "Port parameters of AstIfaceRefDType without ifacep()");
            എങ്കിൽ (ifacep->dead()) തിരികെ;
            checkNoDot(nodep);
            VL_RESTORER_CLEAR(m_usedPins);
            VL_RESTORER_CLEAR(m_usedDefParamPins);
            VL_RESTORER(m_pinSymp);
            m_pinSymp = m_statep->get_കണ്ണി_Sym(ifacep);
            iterateAndNextNull(nodep->paramsp());
        }
    }

    ശൂന്യം visit(AstAttrOf* nodep) override { iterateChildren(nodep); }
    ശൂന്യം visit(AstCastParse* nodep) override {
        എങ്കിൽ (nodep->user3SetOnce()) തിരികെ;
        LINKDOT_VISIT_START();
        // A cast may start a dotted expression: HERE type'(expr) . member_or_method
        സ്ഥിരം തർക്കം dotLhs = m_ds.m_dotPos == DP_FIRST;
        എങ്കിൽ (!dotLhs) checkNoDot(nodep);
        {
            VL_RESTORER_COPY(m_ds);
            m_ds.init(m_curSymp);
            iterateChildren(nodep);
        }
        എങ്കിൽ (dotLhs) m_ds.m_dotPos = DP_MEMBER;
    }

    ശൂന്യം visit(AstAssignW* nodep) override {
        LINKDOT_VISIT_START();
        checkNoDot(nodep);
        VL_RESTORER(m_replaceWithAlias);
        എങ്കിൽ (nodep->user2()) m_replaceWithAlias = അസത്യം;
        iterateChildren(nodep);
    }

    ശൂന്യം visit(AstTypedef* nodep) override {
        VL_RESTORER(m_resolvingTypedef)
        m_resolvingTypedef = സത്യം;
        iterateChildren(nodep);
    }

    ശൂന്യം visit(Ast_കണ്ണി* nodep) override {
        VL_RESTORER(m_inPackedArray);
        എങ്കിൽ (VN_IS(nodep, PackArrayDType)) {
            m_inPackedArray = സത്യം;
        } അല്ലെങ്കിൽ എങ്കിൽ (!m_inPackedArray) {
            LINKDOT_VISIT_START();
            checkNoDot(nodep);
        }
        iterateChildren(nodep);
    }

പൊതു:
    // CONSTRUCTORS
    LinkDotResolveVisitor(AstNetlist* rootp, LinkDotState* statep)
        : m_statep{statep} {
        UINFO(4, __FUNCTION__ << ": ");

        // Note: no need to clear user3 on ledger entries here.
        // VNUser3InUse (m_inuser3) already logically clears user3 on all
        // nodes via the generation counter.  The ledger may also hold stale
        // refp pointers to deleted AST nodes, so iterating it is unsafe.

        iterate(rootp);
        സ്ഥിരം std::map<std::string, Ast_കണ്ണി_Module*> modulesToRevisit
            = std::move(m_modulesToRevisit);
        m_lastDeferredp = nullptr;
        വേണ്ടി (auto& p : modulesToRevisit) {
            Ast_കണ്ണി_Module* സ്ഥിരം modp = p.second;
            modp->foreach([](Ast_കണ്ണി* സ്ഥിരം nodep) { nodep->user3(അസത്യം); });
            iterate(modp);
        }
    }
    ~LinkDotResolveVisitor() override = സ്ഥിരസ്ഥിതി;
};

//######################################################################
// Link class functions

ശൂന്യം V3LinkDot::dumpSubstep(സ്ഥിരം string& name) {
    എങ്കിൽ (dump_വൃക്ഷം_Either_Level() >= 9) {
        V3Global::dump_Check_Global_വൃക്ഷം(name);
    } അല്ലെങ്കിൽ എങ്കിൽ (debug() >= 5) {  // on high dbg level, dump even if not explicitly told to
        v3Global.rootp()->dump_വൃക്ഷം_File(v3Global.debugFilename(name + ".tree"));
    }
}

ശൂന്യം V3LinkDot::linkDotGuts(AstNetlist* rootp, VLinkDotStep step) {
    VIsCached::clear_Cache_വൃക്ഷം();  // Avoid using any stale isPure
    dumpSubstep("prelinkdot");
    LinkDotState state{rootp, step};

    { LinkDotFindVisitor{rootp, &state}; }
    dumpSubstep("prelinkdot-find");

    { LinkDotFindIfaceVisitor{rootp, &state}; }
    dumpSubstep("prelinkdot-findiface");

    എങ്കിൽ (step == LDS_SCOPED) {
        // Well after the initial link when we're ready to operate on the flat design,
        // process AstScope's.  This needs to be separate pass after whole hierarchy graph created.
        { LinkDotScopeVisitor{rootp, &state}; }
        v3Global.assertScoped(സത്യം);
        dumpSubstep("prelinkdot-scoped");
    } അല്ലെങ്കിൽ {
        // Initial link stage, resolve parameters and interfaces
        { LinkDotParamVisitor{rootp, &state}; }
        dumpSubstep("prelinkdot-param");
    }
    state.dumpSelf("prelinkdot");
    state.computeIfaceModSyms();
    state.computeIfaceVarSyms();
    state.computeScopeAliases();
    state.dumpSelf("linkdot-preresolve");
    സ്ഥിരം LinkDotResolveVisitor visitor{rootp, &state};
    state.dumpSelf("linkdot-done");
}

ശൂന്യം V3LinkDot::linkDotPrimary(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    linkDotGuts(nodep, LDS_PRIMARY);
    V3Global::dump_Check_Global_വൃക്ഷം("linkdot", 0, dump_വൃക്ഷം_Either_Level() >= 6);
}

ശൂന്യം V3LinkDot::linkDotParamed(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    linkDotGuts(nodep, LDS_PARAMED);
    V3Global::dump_Check_Global_വൃക്ഷം("linkdotparam", 0, dump_വൃക്ഷം_Either_Level() >= 3);
}

ശൂന്യം V3LinkDot::linkDotScope(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    linkDotGuts(nodep, LDS_SCOPED);
    V3Global::dump_Check_Global_വൃക്ഷം("linkdot", 0, dump_വൃക്ഷം_Either_Level() >= 3);
}
