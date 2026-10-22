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
//शीर्ष
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

#समावेश "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#समावेश "V3LinkDot.h"

#समावेश "V3Global.h"
#समावेश "V3Graph.h"
#समावेश "V3LinkDotIfaceCapture.h"
#समावेश "V3MemberMap.h"
#समावेश "V3Parse.h"
#समावेश "V3Randomize.h"
#समावेश "V3Stats.h"
#समावेश "V3String.h"
#समावेश "V3SymTable.h"

#समावेश <algorithm>
#समावेश <cstdlib>
#समावेश <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

अटल string extractDottedPath(Ast_गाँठ* nodep, bool& hasPartSelect) {
    अगर (AstParseRef* स्थिर refp = VN_CAST(nodep, ParseRef)) {
        वापस refp->name();
    } अन्यथा अगर (AstVarRef* स्थिर refp = VN_CAST(nodep, VarRef)) {
        वापस refp->name();
    } अन्यथा अगर (AstDot* स्थिर dotp = VN_CAST(nodep, Dot)) {
        स्थिर string lhs = extractDottedPath(dotp->lhsp(), hasPartSelect);
        स्थिर string rhs = extractDottedPath(dotp->rhsp(), hasPartSelect);
        वापस VString::dot(lhs, ".", rhs);
    } अन्यथा अगर (VN_IS(nodep, SelBit) || VN_IS(nodep, SelExtract) || VN_IS(nodep, SelPlus)
               || VN_IS(nodep, SelMinus)) {
        hasPartSelect = true;
        वापस "";
    }
    वापस "";
}
अटल string lexicalDisablePath(Ast_गाँठ* स्थिर targetp) {
    std::vector<string> names;
    क्रम (Ast_गाँठ* curp = targetp; curp; curp = curp->aboveLoopp()) {
        अगर (Ast_गाँठ_Block* स्थिर blockp = VN_CAST(curp, गाँठ_Block)) {
            अगर (blockp->name() != "") names.धकेलो_back(blockp->name());
        } अन्यथा अगर (Ast_गाँठ_FTask* स्थिर ftaskp = VN_CAST(curp, गाँठ_FTask)) {
            names.धकेलो_back(ftaskp->name());
        } अन्यथा अगर (VN_IS(curp, गाँठ_Module)) {
            अवरोध;
        }
    }
    string path;
    क्रम (स्वतः it = names.crbegin(); it != names.crend(); ++it) {
        path = VString::dot(path, ".", *it);
    }
    वापस path;
}
अटल string targetInstancePath(Ast_गाँठ* स्थिर targetp, स्थिर string& targetPath) {
    स्थिर string lexicalPath = lexicalDisablePath(targetp);
    अगर (lexicalPath == "" || targetPath == lexicalPath) वापस "";
    स्थिर string suffix = "." + lexicalPath;
    अगर (!VString::endsWith(targetPath, suffix)) वापस "";
    वापस targetPath.substr(0, targetPath.आकार() - suffix.आकार());
}

// ######################################################################
//  Matcher classes (for suggestion matching)

श्रेणी Link_गाँठ_Matcher_Class_Or_Package final : खुला VNodeMatcher {
खुला:
    bool गाँठ_Match(स्थिर Ast_गाँठ* nodep) स्थिर override {
        वापस VN_IS(nodep, Class) || VN_IS(nodep, Package);
    }
};
श्रेणी Link_गाँठ_Matcher_FTask final : खुला VNodeMatcher {
खुला:
    bool गाँठ_Match(स्थिर Ast_गाँठ* nodep) स्थिर override { वापस VN_IS(nodep, गाँठ_FTask); }
};
श्रेणी Link_गाँठ_Matcher_Modport final : खुला VNodeMatcher {
खुला:
    bool गाँठ_Match(स्थिर Ast_गाँठ* nodep) स्थिर override { वापस VN_IS(nodep, Modport); }
};
श्रेणी Link_गाँठ_Matcher_Prod final : खुला VNodeMatcher {
खुला:
    bool गाँठ_Match(स्थिर Ast_गाँठ* nodep) स्थिर override { वापस VN_IS(nodep, RSProd); }
};
श्रेणी Link_गाँठ_Matcher_Var final : खुला VNodeMatcher {
खुला:
    bool गाँठ_Match(स्थिर Ast_गाँठ* nodep) स्थिर override {
        वापस VN_IS(nodep, Var) || VN_IS(nodep, LambdaArgRef);
    }
};
श्रेणी Link_गाँठ_Matcher_Var_IO final : खुला VNodeMatcher {
खुला:
    bool गाँठ_Match(स्थिर Ast_गाँठ* nodep) स्थिर override {
        स्थिर AstVar* स्थिर varp = VN_CAST(nodep, Var);
        अगर (!varp) वापस false;
        वापस varp->isIO();
    }
};
श्रेणी Link_गाँठ_Matcher_Var_Param final : खुला VNodeMatcher {
खुला:
    bool गाँठ_Match(स्थिर Ast_गाँठ* nodep) स्थिर override {
        स्थिर AstVar* स्थिर varp = VN_CAST(nodep, Var);
        अगर (!varp) वापस false;
        वापस varp->isParam();
    }
};

//######################################################################
// LinkDot state, as a visitor of each AstNode

श्रेणी LinkDotState final {
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
    स्थिर VNUser1InUse m_inuser1;
    स्थिर VNUser2InUse m_inuser2;
    स्थिर VNUser4InUse m_inuser4;

खुला:
    // ENUMS
    // In order of priority, compute first ... compute last
    क्रमागत SAMNum : uint8_t { SAMN_MODPORT, SAMN_IFTOP, SAMN__MAX };  // Values for m_scopeAliasMap

निजी:
    // TYPES
    using ScopeAliasMap = std::unordered_map<VSymEnt*, VSymEnt*>;
    using IfaceModSyms = std::vector<std::pair<AstIface*, VSymEnt*>>;

    अटल LinkDotState* s_errorThisp;  // Last self, for error reporting only

    // MEMBERS
    VSymGraph m_syms;  // Symbol table by hierarchy
    VSymGraph m_mods;  // Symbol table of all module names
    VSymEnt* m_dunitEntp = nullptr;  // $unit entry
    std::multimap<std::string, VSymEnt*>
        m_nameScopeSymMap;  // Map of scope referenced by non-pretty textual name
    std::set<std::pair<Ast_गाँठ_Module*, std::string>>
        m_implicitNameSet;  // For [module][signalname] if we can implicitly create it
    std::array<ScopeAliasMap, SAMN__MAX> m_scopeAliasMap;  // Map of <lhs,rhs> aliases
    std::vector<VSymEnt*> m_ifaceVarSyms;  // List of AstIfaceRefDType's to be imported
    IfaceModSyms m_ifaceModSyms;  // List of AstIface+Symbols to be processed
    स्थिर VLinkDotStep m_step;  // Operational step

खुला:
    // METHODS

    व्योम dumpSelf(स्थिर string& nameComment, bool force = false) {
        अगर (debug() >= 6 || dumpLevel() >= 6 || force) {
            स्थिर string filename = v3Global.debugFilename(nameComment) + ".txt";
            UINFO(4, "Dumping " << filename);
            स्थिर std::unique_ptr<std::बख_प्रवाह> logp{V3File::new_ofstream(filename)};
            अगर (logp->हार()) v3fatal("Can't write file: " << filename);
            std::ब_प्रवाह& os = *logp;
            // TODO the symbol table has node pointers which may be
            // dangling, as we call deleteTree in these visitors without
            // pushing for later deletion (or deleting from the symbol table)
            // So, only completely safe to call this when under --debug or --debug-leak.
            m_syms.dumpSelf(os);
            bool first = true;
            क्रम (पूर्णांक samn = 0; samn < SAMN__MAX; ++samn) {
                अगर (!m_scopeAliasMap[samn].empty()) {
                    अगर (first) os << "\nScopeAliasMap:\n";
                    first = false;
                    क्रम (ScopeAliasMap::iterator it = m_scopeAliasMap[samn].begin();
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
    अटल व्योम preErrorDumpHandler() {
        अगर (s_errorThisp) s_errorThisp->preErrorDump();
    }
    व्योम preErrorDump() {
        अटल bool s_diddump = false;
        अगर (!s_diddump && dump_वृक्ष_Level()) {
            s_diddump = true;
            dumpSelf("linkdot-preerr", true);
            v3Global.rootp()->dump_वृक्ष_File(v3Global.debugFilename("linkdot-preerr.tree"));
        }
    }

    // CONSTRUCTORS
    LinkDotState(AstNetlist* rootp, VLinkDotStep step)
        : m_syms{rootp}
        , m_mods{rootp}
        , m_step{step} {
        UINFO(4, __FUNCTION__ << ": ");
        s_errorThisp = यह;
        V3Error::errorExitCb(preErrorDumpHandler);  // If get error, dump self
        readModNames();
    }
    ~LinkDotState() {
        V3Error::errorExitCb(nullptr);
        s_errorThisp = nullptr;
    }

    // ACCESSORS
    VSymGraph* symsp() { वापस &m_syms; }
    पूर्णांक step_संख्या() स्थिर { वापस static_cast<पूर्णांक>(m_step); }
    bool forPrimary() स्थिर { वापस m_step == LDS_PRIMARY; }
    bool forParamed() स्थिर { वापस m_step == LDS_PARAMED; }
    bool forScopeCreation() स्थिर { वापस m_step == LDS_SCOPED; }

    // METHODS
    अटल string गाँठ_Text_Type(Ast_गाँठ* nodep) {
        अगर (स्थिर AstVar* स्थिर varp = VN_CAST(nodep, Var)) {
            अगर (varp->isIO() || varp->isIfaceRef()) {
                वापस "port";
            } अन्यथा अगर (varp->isGParam()) {
                वापस "parameter";
            } अन्यथा अगर (varp->varType() == VVarType::LPARAM) {
                वापस "local parameter";
            } अन्यथा {
                वापस "variable";
            }
        } अन्यथा अगर (स्थिर AstParamTypeDType* स्थिर dtypep = VN_CAST(nodep, ParamTypeDType)) {
            अगर (dtypep->isGParam()) {
                वापस "type parameter";
            } अन्यथा {
                वापस "local type parameter";
            }
        } अन्यथा अगर (VN_IS(nodep, Begin)) {
            वापस "block";
        } अन्यथा अगर (VN_IS(nodep, Cell)) {
            वापस "instance";
        } अन्यथा अगर (VN_IS(nodep, Constraint)) {
            वापस "constraint";
        } अन्यथा अगर (VN_IS(nodep, Func)) {
            वापस "function";
        } अन्यथा अगर (VN_IS(nodep, GenBlock)) {
            वापस "generate block";
        } अन्यथा अगर (VN_IS(nodep, Iface)) {
            वापस "interface";
        } अन्यथा अगर (VN_IS(nodep, RSProd)) {
            वापस "randsequence production";
        } अन्यथा अगर (VN_IS(nodep, Task)) {
            वापस "task";
        } अन्यथा {
            वापस nodep->prettyTypeName();
        }
    }

    Ast_गाँठ_Module* findModuleSym(स्थिर string& modName) {
        स्थिर VSymEnt* स्थिर foundp = m_mods.rootp()->findIdFallback(modName);
        वापस foundp ? VN_AS(foundp->nodep(), गाँठ_Module) : nullptr;
    }
    व्योम readModNames() {
        // Look at all modules, and store pointers to all module names
        क्रम (Ast_गाँठ_Module *nextp, *nodep = v3Global.rootp()->modulesp(); nodep; nodep = nextp) {
            nextp = VN_AS(nodep->nextp(), गाँठ_Module);
            m_mods.rootp()->जोड़ो(nodep->name(), नया VSymEnt{&m_mods, nodep});
        }
    }

    VSymEnt* मूल_Entp() स्थिर { वापस m_syms.rootp(); }
    VSymEnt* dunitEntp() स्थिर { वापस m_dunitEntp; }
    व्योम checkDuplicate(VSymEnt* lookupSymp, Ast_गाँठ* nodep, स्थिर string& name) {
        // Lookup the given name under current symbol table
        // Insert if not found
        // Report error if there's a duplicate
        //
        // Note we only check for conflicts at the same level; it's ok if one block hides another
        // We also wouldn't want to not insert it even though it's lower down

        स्थिर VSymEnt* स्थिर foundp = lookupSymp->findIdFlat(name);
        Ast_गाँठ* स्थिर fnodep = foundp ? foundp->nodep() : nullptr;
        अगर (!fnodep) {
            // Not found, will add in a moment.
            अगर (forPrimary() && VSymEnt::checkSimilarname(nodep)) {
                स्थिर VSymEnt* स्थिर alt = lookupSymp->findSimilarIdFlat(name);
                अगर (alt) {
                    nodep->v3warn(SIMILARNAME, "Declaration overlaps another with different case: "
                                                   << nodep->prettyNameQ() << '\n'
                                                   << nodep->warnContextPrimary() << '\n'
                                                   << alt->nodep()->warnOther()
                                                   << "... Location of original declaration\n"
                                                   << alt->nodep()->warnContextSecondary());
                }
            }
        } अन्यथा अगर (nodep == fnodep) {  // Already inserted.
            // Good.
        } अन्यथा अगर (foundp->imported()) {  // From package
            // We don't throw VARHIDDEN as if the import is later the symbol
            // table's import wouldn't warn
        } अन्यथा अगर (forPrimary() && VN_IS(nodep, GenBlock)
                   && (VN_IS(fnodep, Begin) || VN_IS(fnodep, GenBlock))) {
            // Begin: ... blocks often replicate under genif/genfor, so
            // suppress duplicate checks.  See t_gen_forif.v for an example.
        } अन्यथा {
            UINFO(4, "name " << name);  // Not always same as nodep->name
            UINFO(4, "Var1 " << nodep);
            UINFO(4, "Var2 " << fnodep);
            // Quote the node's own name as for messages, if it is the symbol's
            स्थिर string nameQ
                = fnodep->name() == name ? fnodep->prettyNameQ() : Ast_गाँठ::prettyNameQ(name);
            अगर (nodep->type() == fnodep->type()) {
                nodep->v3error("Duplicate declaration of "
                               << गाँठ_Text_Type(fnodep) << ": " << nameQ << '\n'
                               << nodep->warnContextPrimary() << '\n'
                               << fnodep->warnOther() << "... Location of original declaration\n"
                               << fnodep->warnContextSecondary());
                nodep->decl_Token_अंक_Set_Min(0);  // Disable checking forward typedefs
                fnodep->decl_Token_अंक_Set_Min(0);  // Disable checking forward typedefs
            } अन्यथा {
                nodep->v3error("Unsupported in C: "
                               << ucfirst(गाँठ_Text_Type(nodep)) << " has the same name as "
                               << गाँठ_Text_Type(fnodep) << ": " << nameQ << '\n'
                               << nodep->warnContextPrimary() << '\n'
                               << fnodep->warnOther() << "... Location of original declaration\n"
                               << fnodep->warnContextSecondary());
            }
        }
    }
    व्योम जोड़ो_DUnit(AstNetlist* nodep) {
        // $unit on top scope
        VSymEnt* स्थिर symp = नया VSymEnt{&m_syms, nodep};
        UINFO(9, "      INSERTdunit se" << cvtToHex(symp));
        symp->parentp(मूल_Entp());  // Needed so backward search can find name of top module
        symp->fallbackp(nullptr);
        मूल_Entp()->जोड़ो("$unit ", symp);  // Space so can never name conflict with user code
        //
        UASSERT_OBJ(!m_dunitEntp, nodep, "Call insertDUnit only once");
        m_dunitEntp = symp;
    }
    VSymEnt* जोड़ो_शीर्ष_Cell(Ast_गाँठ_Module* nodep, स्थिर string& scopename) {
        // Only called on the module at the very top of the hierarchy
        VSymEnt* स्थिर symp = नया VSymEnt{&m_syms, nodep};
        UINFO(9, "      INSERTtop se" << cvtToHex(symp) << "  " << scopename << " " << nodep);
        symp->parentp(मूल_Entp());  // Needed so backward search can find name of top module
        symp->fallbackp(dunitEntp());  // Needed so can find $unit stuff
        nodep->user1p(symp);
        checkDuplicate(मूल_Entp(), nodep, nodep->origName());
        मूल_Entp()->जोड़ो(nodep->origName(), symp);
        अगर (forScopeCreation()) m_nameScopeSymMap.emplace(scopename, symp);
        वापस symp;
    }
    VSymEnt* जोड़ो_शीर्ष_Iface(AstCell* nodep, स्थिर string& scopename) {
        VSymEnt* स्थिर symp = नया VSymEnt{&m_syms, nodep};
        UINFO(9, "      INSERTtopiface se" << cvtToHex(symp) << "  " << scopename << " " << nodep);
        symp->parentp(मूल_Entp());  // Needed so backward search can find name of top module
        symp->fallbackp(dunitEntp());  // Needed so can find $unit stuff
        nodep->user1p(symp);
        अगर (nodep->modp()) nodep->modp()->user1p(symp);
        checkDuplicate(मूल_Entp(), nodep, nodep->origName());
        मूल_Entp()->जोड़ो(nodep->origName(), symp);
        अगर (forScopeCreation()) m_nameScopeSymMap.emplace(scopename, symp);
        वापस symp;
    }
    VSymEnt* जोड़ो_Cell(VSymEnt* abovep, VSymEnt* modSymp, AstCell* nodep,
                        स्थिर string& scopename) {
        UASSERT_OBJ(abovep, nodep, "Null symbol table inserting node");
        VSymEnt* स्थिर symp = नया VSymEnt{&m_syms, nodep};
        UINFO(9, "      INSERTcel se" << cvtToHex(symp) << "  " << scopename << " above=se"
                                      << cvtToHex(abovep) << " mods=se" << cvtToHex(modSymp)
                                      << " node=" << nodep);
        symp->parentp(abovep);
        symp->fallbackp(dunitEntp());  // Needed so can find $unit stuff
        nodep->user1p(symp);
        अगर (nodep->modp()) nodep->modp()->user1p(symp);
        checkDuplicate(abovep, nodep, nodep->origName());
        abovep->reinsert(nodep->origName(), symp);
        अगर (forScopeCreation() && abovep != modSymp && !modSymp->findIdFlat(nodep->name())) {
            // If it's foo_DOT_bar, we need to be able to find it under "foo_DOT_bar" too.
            // Duplicates are possible, as until resolve generates might
            // have 2 same cells under an if
            modSymp->reinsert(nodep->name(), symp);
        }
        अगर (forScopeCreation()) m_nameScopeSymMap.emplace(scopename, symp);
        वापस symp;
    }
    व्योम जोड़ो_Map(VSymEnt* symp, स्थिर string& scopename) {
        अगर (forScopeCreation()) m_nameScopeSymMap.emplace(scopename, symp);
    }

    VSymEnt* जोड़ो_Inline(VSymEnt* abovep, VSymEnt* modSymp, AstCellInline* nodep,
                          स्थिर string& basename) {
        // A fake point in the hierarchy, corresponding to a named block flattened by V3Begin.
        // Names are looked up in the module with the block's flattened prefix.
        UASSERT_OBJ(abovep, nodep, "Null symbol table inserting node");
        VSymEnt* स्थिर symp = नया VSymEnt{&m_syms, nodep};
        UINFO(9, "      INSERTinl se" << cvtToHex(symp) << "  " << basename << " above=se"
                                      << cvtToHex(abovep) << " mods=se" << cvtToHex(modSymp)
                                      << " node=" << nodep);
        symp->parentp(abovep);
        symp->fallbackp(modSymp);
        symp->symPrefix(nodep->name() + "__DOT__");
        nodep->user1p(symp);
        checkDuplicate(abovep, nodep, nodep->name());
        abovep->reinsert(basename, symp);
        अगर (abovep != modSymp && !modSymp->findIdFlat(nodep->name())) {
            // If it's foo_DOT_bar, we need to be able to find it under that too.
            modSymp->reinsert(nodep->name(), symp);
        }
        वापस symp;
    }
    VSymEnt* जोड़ो_Block(VSymEnt* abovep, स्थिर string& name, Ast_गाँठ* nodep,
                         Ast_गाँठ_Module* classOrPackagep) {
        // A fake point in the hierarchy, corresponding to a begin or function/task block
        // After we remove begins these will go away
        // Note we fallback to the symbol table of the parent, as we want to find variables there
        // However, cells walk the graph, so cells will appear under the begin/ftask itself
        UASSERT_OBJ(abovep, nodep, "Null symbol table inserting node");
        VSymEnt* स्थिर symp = नया VSymEnt{&m_syms, nodep};
        UINFO(9, "      INSERTblk se" << cvtToHex(symp) << "  above=se" << cvtToHex(abovep)
                                      << " pkg=" << cvtToHex(classOrPackagep)
                                      << "  node=" << nodep);
        symp->parentp(abovep);
        symp->classOrPackagep(classOrPackagep);
        symp->fallbackp(abovep);
        nodep->user1p(symp);
        अगर (name != "") checkDuplicate(abovep, nodep, name);
        // Duplicates are possible, as until resolve generates might have 2 same cells under an if
        abovep->reinsert(name, symp);
        वापस symp;
    }
    VSymEnt* जोड़ो_Sym(VSymEnt* abovep, स्थिर string& name, Ast_गाँठ* nodep,
                       Ast_गाँठ_Module* classOrPackagep) {
        UASSERT_OBJ(abovep, nodep, "Null symbol table inserting node");
        VSymEnt* स्थिर symp = नया VSymEnt{&m_syms, nodep};
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
        वापस symp;
    }
    अटल bool exists_गाँठ_Sym(Ast_गाँठ* nodep) { वापस nodep->user1p() != nullptr; }
    अटल VSymEnt* get_गाँठ_Sym(Ast_गाँठ* nodep) {
        // Don't use this in ResolveVisitor, as we need to pick up the proper
        // reference under each SCOPE
        VSymEnt* स्थिर symp = nodep->user1u().toSymEnt();
        UASSERT_OBJ(symp, nodep, "Module/etc never assigned a symbol entry?");
        वापस symp;
    }
    VSymEnt* getScopeSym(AstScope* nodep) {
        स्थिर स्वतः it = m_nameScopeSymMap.find(nodep->name());
        UASSERT_OBJ(it != m_nameScopeSymMap.end(), nodep,
                    "Scope never assigned a symbol entry '" << nodep->name() << "'");
        वापस it->second;
    }
    व्योम implicitOkAdd(Ast_गाँठ_Module* nodep, स्थिर string& varname) {
        // Mark the given variable name as being allowed to be implicitly declared
        अगर (nodep) {
            स्थिर स्वतः it = m_implicitNameSet.find(std::make_pair(nodep, varname));
            अगर (it == m_implicitNameSet.end()) m_implicitNameSet.emplace(nodep, varname);
        }
    }
    bool implicitOk(Ast_गाँठ_Module* nodep, स्थिर string& varname) {
        वापस nodep
               && (m_implicitNameSet.find(std::make_pair(nodep, varname))
                   != m_implicitNameSet.end());
    }

    // Track and later recurse interface modules
    व्योम जोड़ो_Iface_Mod_Sym(AstIface* nodep, VSymEnt* symp) {
        m_ifaceModSyms.emplace_back(nodep, symp);
    }
    व्योम computeIfaceModSyms();

    // Track and later insert interface references
    व्योम जोड़ो_Iface_Var_Sym(VSymEnt* symp) {  // Where sym is for a VAR of dtype IFACEREFDTYPE
        m_ifaceVarSyms.धकेलो_back(symp);
    }
    // Iface for a raw or arrayed iface; peels nested array layers for multi-dim arrays.
    अटल AstIfaceRefDType* ifaceRefFromArray(Ast_गाँठ_DType* nodep) {
        जबतक (nodep) {
            अगर (AstIfaceRefDType* स्थिर ifp = VN_CAST(nodep, IfaceRefDType)) वापस ifp;
            अगर (स्थिर AstBracketArrayDType* स्थिर arrp = VN_CAST(nodep, BracketArrayDType)) {
                nodep = arrp->subDTypep();
            } अन्यथा अगर (स्थिर AstUnpackArrayDType* स्थिर arrp = VN_CAST(nodep, UnpackArrayDType)) {
                nodep = arrp->subDTypep();
            } अन्यथा {
                वापस nullptr;
            }
        }
        वापस nullptr;
    }
    // Given a pin expression, resolve it to a live AstIface* (or nullptr).
    // Handles both simple VarRef and dotted VarXRef pin connections.
    AstIface* live_Iface_From_Pin_व्यंजक(Ast_गाँठ* exprp, VSymEnt* parentSymp) {
        AstVar* resolvedVarp = nullptr;
        अगर (AstVarRef* स्थिर refp = VN_CAST(exprp, VarRef)) {
            resolvedVarp = refp->varp();
        } अन्यथा अगर (AstVarXRef* स्थिर xrefp = VN_CAST(exprp, VarXRef)) {
            VSymEnt* स्थिर lookupSymp = parentSymp ? parentSymp->parentp() : nullptr;
            अगर (lookupSymp) {
                string baddot;
                VSymEnt* okSymp = nullptr;
                स्थिर string dotpath = xrefp->dotted() + "." + xrefp->name();
                VSymEnt* स्थिर dotSymp
                    = findDotted(xrefp->fileline(), lookupSymp, dotpath, baddot, okSymp, true);
                अगर (dotSymp) resolvedVarp = VN_CAST(dotSymp->nodep(), Var);
            }
        }
        AstIface* resultp = nullptr;
        अगर (resolvedVarp) {
            AstIfaceRefDType* स्थिर irefp = ifaceRefFromArray(resolvedVarp->subDTypep());
            अगर (irefp && irefp->ifaceViaCellp() && !irefp->ifaceViaCellp()->dead()) {
                resultp = irefp->ifaceViaCellp();
            }
        }
        वापस resultp;
    }

    // Attempt to repair a port's AstIfaceRefDType by tracing through the
    // parent cell's pin connection to find the correct live interface.
    // Returns true if repaired or already correct; false if unresolvable.
    bool repairIfaceRef(VSymEnt* varSymp, AstVar* varp, AstIfaceRefDType* ifacerefp, bool isDead) {
        // Walk up symbol-table parents to find the enclosing AstCell
        VSymEnt* parentSymp = varSymp->parentp();
        AstCell* parentCellp = nullptr;
        क्रम (पूर्णांक depth = 0; parentSymp && depth < 20; ++depth) {
            अगर (AstCell* स्थिर cp = VN_CAST(parentSymp->nodep(), Cell)) {
                parentCellp = cp;
                अवरोध;
            }
            parentSymp = parentSymp->parentp();
        }
        अगर (!parentCellp) वापस false;
        // Scan pins to find the one connected to this port variable
        क्रम (AstPin* pinp = parentCellp->pinsp(); pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            अगर (pinp->modVarp() != varp && pinp->name() != varp->name()) जारी;
            Ast_गाँठ* स्थिर exprp = pinp->exprp();
            अगर (!exprp) वापस false;
            AstIface* स्थिर newIfacep = live_Iface_From_Pin_व्यंजक(exprp, parentSymp);
            अगर (newIfacep && newIfacep != ifacerefp->ifaceViaCellp()) {
                UINFO(4, "  REPAIR-IFACE-REF var=" << varp->prettyNameQ()
                                                   << " old=" << ifacerefp->ifacep()->prettyNameQ()
                                                   << " new=" << newIfacep->prettyNameQ());
                ifacerefp->ifacep(newIfacep);
                वापस true;
            }
            अगर (newIfacep) वापस !isDead;  // Same interface - OK if live
            वापस false;
        }
        वापस false;
    }

    व्योम computeIfaceVarSyms() {
        क्रम (VSymEnt* varSymp : m_ifaceVarSyms) {
            AstVar* स्थिर varp = varSymp ? VN_AS(varSymp->nodep(), Var) : nullptr;
            UINFO(9, "  insAllIface se" << cvtToHex(varSymp) << " " << varp);
            AstIfaceRefDType* स्थिर ifacerefp = ifaceRefFromArray(varp->subDTypep());
            UASSERT_OBJ(ifacerefp, varp, "Non-ifacerefs on list!");
            अगर (!ifacerefp->cellp() && !ifacerefp->isVirtual() && (varp->pin_अंक() == 0)) {
                varp->v3error("Interface port declaration "
                              << varp->prettyNameQ() << " doesn't have corresponding port\n"
                              << varp->warnMore()
                                     + "... Perhaps intended an interface instantiation but "
                                       "are missing parenthesis (IEEE 1800-2023 25.3)?");
            }
            अगर (!ifacerefp->ifaceViaCellp()) {
                अगर (!ifacerefp->cellp()) {  // Probably a NotFoundModule, or a normal module if
                                            // made mistake
                    UINFO(1, "Associated cell " << Ast_गाँठ::prettyNameQ(ifacerefp->cellName()));
                    ifacerefp->v3error("Cannot find file containing interface: "
                                       << Ast_गाँठ::prettyNameQ(ifacerefp->ifaceName()));
                    जारी;
                } अन्यथा {
                    ifacerefp->v3fatalSrc("Unlinked interface");
                }
            } अन्यथा अगर (varp->isIfaceRef() && !ifacerefp->cellp() && ifacerefp->ifaceViaCellp()) {
                // Port variable (cellp=null) pointing to an interface.
                // The interface may be dead (template replaced by clone) or
                // live-but-wrong (template still alive because some instances
                // use default params, but this port should point to a
                // specialized clone based on the pin connection).
                स्थिर bool isDead = ifacerefp->ifaceViaCellp()->dead();
                अगर (isDead && forPrimary() && !varp->isIfaceParent()
                    && !v3Global.opt.शीर्ष_Ifaces_Supported()) {
                    varp->v3warn(E_UNSUPPORTED,
                                 "Unsupported: Interfaced port on top level module");
                }
                // Attempt repair: trace through parent cell's pin to find
                // the correct live interface for this port variable
                स्थिर bool repaired = repairIfaceRef(varSymp, varp, ifacerefp, isDead);
                अगर (!repaired && isDead) {
                    ifacerefp->v3error(
                        "Interface " << Ast_गाँठ::prettyNameQ(ifacerefp->ifaceName())
                                     << " not connected as parent's interface not connected\n"
                                     << ifacerefp->warnMore()
                                     << "... Perhaps caused by another error on the parent "
                                        "interface that needs resolving\n"
                                     << ifacerefp->warnMore()
                                     << "... Or, perhaps intended an interface instantiation but "
                                        "are missing parenthesis (IEEE 1800-2023 25.3)?");
                    जारी;
                }
            } अन्यथा अगर (ifacerefp->ifaceViaCellp()->dead()
                       || !exists_गाँठ_Sym(ifacerefp->ifaceViaCellp())) {
                ifacerefp->ifaceViaCellp()->v3fatalSrc(
                    "Interface referenced by virtual iface is dead. "
                    "Dead interfaces have required linking steps skipped");
            }
            VSymEnt* स्थिर ifaceSymp = get_गाँठ_Sym(ifacerefp->ifaceViaCellp());
            VSymEnt* ifOrPortSymp = ifaceSymp;
            // Link Modport names to the Modport Node under the Interface
            अगर (ifacerefp->isModport()) {
                VSymEnt* स्थिर foundp = ifaceSymp->findIdFallback(ifacerefp->modportName());
                bool ok = false;
                अगर (foundp) {
                    अगर (AstModport* स्थिर modportp = VN_CAST(foundp->nodep(), Modport)) {
                        UINFO(4, "Link Modport: " << modportp);
                        ifacerefp->modportp(modportp);
                        ifOrPortSymp = foundp;
                        ok = true;
                    }
                }
                अगर (!ok) {
                    स्थिर string suggest = suggestSymFallback(ifaceSymp, ifacerefp->modportName(),
                                                              Link_गाँठ_Matcher_Modport{});
                    ifacerefp->modportFileline()->v3error(
                        "Modport not found under interface "
                        << ifacerefp->prettyNameQ(ifacerefp->ifaceName()) << ": "
                        << ifacerefp->prettyNameQ(ifacerefp->modportName()) << '\n'
                        << (suggest.empty() ? "" : ifacerefp->warnMore() + suggest));
                }
            }
            // Alias won't expand until interfaces and modport names are known; see notes at top
            जोड़ो_Scope_Alias(SAMN_IFTOP, varSymp, ifOrPortSymp);
        }
        m_ifaceVarSyms.सफा();
    }

    व्योम जोड़ो_Scope_Alias(SAMNum samn, VSymEnt* lhsp, VSymEnt* rhsp) {
        // Track and later insert scope aliases; an interface referenced by
        // a child cell connecting to that interface
        // Typically lhsp=VAR w/dtype IFACEREF, rhsp=IFACE cell
        UINFO(9, "   insertScopeAlias se" << cvtToHex(lhsp) << " se" << cvtToHex(rhsp));
        UASSERT_OBJ(
            !(VN_IS(rhsp->nodep(), Cell) && !VN_IS(VN_AS(rhsp->nodep(), Cell)->modp(), Iface)),
            rhsp->nodep(), "Got a non-IFACE alias RHS");
        m_scopeAliasMap[samn].emplace(lhsp, rhsp);
    }
    व्योम computeScopeAliases() {
        UINFO(9, "computeIfaceAliases");
        क्रम (पूर्णांक samn = 0; samn < SAMN__MAX; ++samn) {
            क्रम (ScopeAliasMap::iterator it = m_scopeAliasMap[samn].begin();
                 it != m_scopeAliasMap[samn].end(); ++it) {
                VSymEnt* स्थिर lhsp = it->first;
                VSymEnt* srcp = lhsp;
                जबतक (true) {  // Follow chain of aliases up to highest level non-alias
                    स्थिर स्वतः it2 = m_scopeAliasMap[samn].find(srcp);
                    अगर (it2 != m_scopeAliasMap[samn].end()) {
                        srcp = it2->second;
                        जारी;
                    } अन्यथा {
                        अवरोध;
                    }
                }
                UINFO(9, "  iiasa: Insert alias se" << lhsp << " (" << lhsp->nodep()->typeName()
                                                    << ") <- se" << srcp << " " << srcp->nodep());
                // srcp should be an interface reference pointing to the interface we want to
                // import
                lhsp->importFromIface(symsp(), srcp);
                // Allow access to objects not permissible to be listed in a modport
                अगर (VN_IS(srcp->nodep(), Modport)) {
                    lhsp->importFromIface(symsp(), srcp->parentp(), true);
                }
            }
            // m_scopeAliasMap[samn].clear();  // Done with it, but put into debug file
        }
    }

निजी:
    VSymEnt* findWithAltFallback(VSymEnt* symp, स्थिर string& name, स्थिर string& altname) {
        VSymEnt* findp = symp->findIdFallback(name);
        अगर (findp) वापस findp;
        अगर (altname != "") {
            UINFO(8, "     alt fallback");
            findp = symp->findIdFallback(altname);
        }
        वापस findp;
    }

    VSymEnt* findWithAltFlat(VSymEnt* symp, स्थिर string& name, स्थिर string& altname) {
        VSymEnt* findp = symp->findIdFlat(name);
        अगर (findp) वापस findp;
        अगर (altname != "") {
            UINFO(8, "     alt flat");
            findp = symp->findIdFlat(altname);
        }
        वापस findp;
    }

खुला:
    VSymEnt* findDotted(File_पंक्ति* refLocationp, VSymEnt* lookupSymp, स्थिर string& dotname,
                        string& baddot, VSymEnt*& okSymp, bool firstId) {
        // Given a dotted hierarchy name, return where in scope it is
        // Note when dotname=="" we just fall through and return lookupSymp
        UINFO(8, "    dottedFind se" << cvtToHex(lookupSymp) << " '" << dotname << "'");
        string leftname = dotname;
        okSymp = lookupSymp;  // So can list bad scopes
        जबतक (leftname != "") {  // foreach dotted part of xref name
            string::आकार_type pos = leftname.find('.');
            string ident;
            अगर (pos != string::npos) {
                ident = leftname.substr(0, pos);
                leftname = leftname.substr(pos + 1);
            } अन्यथा {
                ident = leftname;
                leftname = "";
            }
            baddot = ident;  // So user can see where they botched it
            okSymp = lookupSymp;
            string altIdent;
            अगर (!forScopeCreation()) {
                // GENFOR Begin is foo__BRA__##__KET__ after we've genloop unrolled,
                // but presently should be just "foo".
                // Likewise cell foo__[array] before we've expanded arrays is just foo.
                // Multi-dim iface arrays append multiple __BRA__..__KET__ suffixes; strip them
                // all.
                altIdent = ident;
                जबतक (VString::endsWith(altIdent, "__KET__")) {
                    स्थिर स्वतः braPos = altIdent.rfind("__BRA__");
                    अगर (braPos == string::npos) अवरोध;
                    altIdent = altIdent.substr(0, braPos);
                }
                अगर (altIdent == ident) altIdent.सफा();
            }
            UINFO(8, "         id " << ident << " alt " << altIdent << " left " << leftname
                                    << " at se" << lookupSymp);
            // Spec says; Look at existing module (cellnames then modname),
            // then look up (inst name or modname)
            अगर (firstId) {
                // Check this module - subcellnames
                स्थिर AstCell* cellp = lookupSymp ? VN_CAST(lookupSymp->nodep(), Cell)
                                                  : nullptr;  // Replicated below
                अगर (VSymEnt* स्थिर findSymp = findWithAltFallback(lookupSymp, ident, altIdent)) {
                    lookupSymp = findSymp;
                }

                // Check this module - cur modname
                अन्यथा अगर (cellp && cellp->modp()->origName() == ident) {
                }
                // $root we walk up to Netlist
                अन्यथा अगर (ident == "$root") {
                    lookupSymp = मूल_Entp();
                    // We've added the '$root' module, now everything else is one lower
                    अगर (forScopeCreation()) {
                        lookupSymp = lookupSymp->findIdFlat(ident);
                        UASSERT(lookupSymp, "Cannot find $root module under netlist");
                    }
                }
                // Move up and check cellname + modname
                अन्यथा {
                    bool crossedCell = false;  // Crossed a cell boundary
                    जबतक (lookupSymp) {
                        lookupSymp = lookupSymp->parentp();
                        cellp = lookupSymp ? VN_CAST(lookupSymp->nodep(), Cell)
                                           : nullptr;  // Replicated above
                        अगर (lookupSymp) {
                            UINFO(9, "    Up to " << lookupSymp);
                            अगर (cellp || VN_IS(lookupSymp->nodep(), CellInline)) {
                                crossedCell = true;
                            }
                            अगर (cellp && cellp->modp()->origName() == ident) {
                                अवरोध;
                            } अन्यथा अगर (VSymEnt* स्थिर findSym2p
                                       = findWithAltFallback(lookupSymp, ident, altIdent)) {
                                lookupSymp = findSym2p;
                                अगर (crossedCell && VN_IS(lookupSymp->nodep(), Var)) {
                                    UINFO(9, "    Not found but matches var name in parent "
                                                 << lookupSymp);
                                    वापस nullptr;
                                }
                                अवरोध;
                            }
                        } अन्यथा {
                            अवरोध;
                        }
                    }
                    अगर (!lookupSymp) वापस nullptr;  // Not found
                }
            } अन्यथा {  // Searching for middle path component, must be a cell or interface port
                VSymEnt* findSymp = findWithAltFlat(lookupSymp, ident, altIdent);
                अगर (findSymp) {
                    lookupSymp = findSymp;
                } अन्यथा {
                    // Try prefixed lookup for interface ports accessed through hierarchy
                    // (e.g., slave_inst.bus.data where bus is an interface port)
                    // After inlining, interface ports have prefixed names like
                    // top__DOT__slave_inst__DOT__bus
                    UINFO(8, "     middle-path: trying prefixed lookup for '" << ident << "'\n");
                    string baddot;
                    findSymp = findSymPrefixed(lookupSymp, ident, baddot, /*fallback=*/true);
                    UINFO(8, "     middle-path: prefixed lookup result: "
                                 << (findSymp ? "found" : "not found") << "\n");
                    अगर (findSymp) {
                        // Check if this is an interface reference variable
                        स्थिर AstVar* varp = VN_CAST(findSymp->nodep(), Var);
                        अगर (!varp) {
                            अगर (स्थिर AstVarScope* vscp = VN_CAST(findSymp->nodep(), VarScope)) {
                                varp = vscp->varp();
                            }
                        }
                        अगर (varp && varp->isIfaceRef()) {
                            // Found an interface port - use findSymp directly
                            // computeScopeAliases has already imported interface members
                            // into this symbol entry, so we use it instead of redirecting
                            // to the shared modport definition (which would cause all
                            // instances to resolve to the same symbols - bug #2656)
                            lookupSymp = findSymp;
                        } अन्यथा {
                            // Non-interface symbol in middle path must be a cell (module instance)
                            // to continue hierarchical resolution
                            अगर (VN_IS(findSymp->nodep(), Cell)) {
                                lookupSymp = findSymp;
                            } अन्यथा {
                                // Reject non-cell, non-interface symbols found via fallback
                                // to prevent accidental resolution to unrelated symbols
                                UINFO(8, "     middle-path: rejecting non-cell symbol\n");
                                वापस nullptr;
                            }
                        }
                    } अन्यथा {
                        वापस nullptr;  // Not found
                    }
                }
            }
            अगर (lookupSymp) {
                अगर (स्थिर AstCell* स्थिर cellp = VN_CAST(lookupSymp->nodep(), Cell)) {
                    अगर (स्थिर Ast_गाँठ_Module* स्थिर modp = cellp->modp()) {
                        अगर (modp->hierBlock() && !leftname.empty()) {
                            refLocationp->v3error("Cannot access scope inside hierarchical block");
                        } अन्यथा अगर (VN_IS(modp, NotFoundModule)) {
                            refLocationp->v3error("Dotted reference to instance that refers to "
                                                  "missing module/interface: "
                                                  << modp->prettyNameQ());
                        }
                    }
                }
                // Follow scope alias for nested interface port access
                अगर (!leftname.empty()) {
                    स्थिर स्वतः aliasIt = m_scopeAliasMap[SAMN_IFTOP].find(lookupSymp);
                    अगर (aliasIt != m_scopeAliasMap[SAMN_IFTOP].end()) {
                        lookupSymp = aliasIt->second;
                        // Alias may point to __Viftop VarScope; find corresponding Cell
                        अगर (स्थिर AstVarScope* स्थिर vscp
                            = VN_CAST(lookupSymp->nodep(), VarScope)) {
                            स्थिर string varName = vscp->varp()->name();
                            अटल constexpr स्थिर अक्षर* स्थिर VIFTOP_SUFFIX = "__Viftop";
                            अगर (VString::endsWith(varName, VIFTOP_SUFFIX)) {
                                स्थिर string cellName
                                    = varName.substr(0, varName.आकार() - म_माप(VIFTOP_SUFFIX));
                                VSymEnt* स्थिर parentSymp = lookupSymp->parentp();
                                अगर (parentSymp) {
                                    VSymEnt* स्थिर cellSymp = parentSymp->findIdFlat(cellName);
                                    अगर (cellSymp && VN_IS(cellSymp->nodep(), Cell)) {
                                        lookupSymp = cellSymp;  // Use Cell for member lookup
                                    }
                                }
                            }
                        }
                    } अन्यथा {
                        // No alias; try following IfaceRefDType to interface cell
                        स्थिर AstVar* varp = VN_CAST(lookupSymp->nodep(), Var);
                        अगर (!varp) {
                            अगर (स्थिर AstVarScope* स्थिर vscp
                                = VN_CAST(lookupSymp->nodep(), VarScope)) {
                                varp = vscp->varp();
                            }
                        }
                        अगर (varp && varp->isIfaceRef()) {
                            अगर (स्थिर AstIfaceRefDType* स्थिर ifaceRefp
                                = ifaceRefFromArray(varp->dtypep())) {
                                अगर (ifaceRefp->cellp() && exists_गाँठ_Sym(ifaceRefp->cellp())) {
                                    lookupSymp = get_गाँठ_Sym(ifaceRefp->cellp());
                                } अन्यथा अगर (ifaceRefp->ifaceViaCellp()
                                           && exists_गाँठ_Sym(ifaceRefp->ifaceViaCellp())) {
                                    lookupSymp = get_गाँठ_Sym(ifaceRefp->ifaceViaCellp());
                                } अन्यथा अगर (ifaceRefp->ifacep()
                                           && exists_गाँठ_Sym(ifaceRefp->ifacep())) {
                                    lookupSymp = get_गाँठ_Sym(ifaceRefp->ifacep());
                                }
                            }
                        }
                    }
                }
            }
            firstId = false;
        }
        वापस lookupSymp;
    }

    अटल string removeLastInlineScope(स्थिर string& name) {
        string out = name;
        स्थिर string dot = "__DOT__";
        स्थिर माप_प्रकार dotPos = out.rfind(dot, out.आकार() - dot.लंबाई() - 1);
        अगर (dotPos == string::npos) {
            वापस "";
        } अन्यथा {
            वापस out.erase(dotPos + dot.लंबाई(), string::npos);
        }
    }

    VSymEnt* findSymPrefixed(VSymEnt* lookupSymp, स्थिर string& dotname, string& baddot,
                             bool fallback) {
        // Find symbol in given point in hierarchy, allowing prefix (post-Inline)
        // For simplicity lookupSymp may be passed nullptr result from findDotted
        अगर (!lookupSymp) वापस nullptr;
        UINFO(8, "    findSymPrefixed "
                     << dotname << " under se" << cvtToHex(lookupSymp)
                     << ((lookupSymp->symPrefix() == "") ? "" : " as ")
                     << ((lookupSymp->symPrefix() == "") ? "" : lookupSymp->symPrefix() + dotname)
                     << "  at se" << lookupSymp << " fallback=" << fallback);
        string prefix = lookupSymp->symPrefix();
        VSymEnt* foundp = nullptr;
        जबतक (!foundp) {
            अगर (fallback) {
                foundp = lookupSymp->findIdFallback(prefix + dotname);  // Might be nullptr
            } अन्यथा {
                foundp = lookupSymp->findIdFlat(prefix + dotname);  // Might be nullptr
            }
            अगर (prefix.empty()) अवरोध;
            स्थिर string अग्र_Prefix = removeLastInlineScope(prefix);
            अगर (prefix == अग्र_Prefix) अवरोध;
            prefix = अग्र_Prefix;
        }
        अगर (!foundp) baddot = dotname;
        वापस foundp;
    }
    अटल bool checkIfClassOrPackage(स्थिर VSymEnt* स्थिर symp) {
        अगर (VN_IS(symp->nodep(), Class) || VN_IS(symp->nodep(), Package)) वापस true;

        // Helper: check if a RefDType might resolve to a class later.
        स्थिर स्वतः isPotentialClassRef = [](स्थिर AstRefDType* refp) -> bool {
            वापस refp && !refp->typeofp()
                   && (!refp->classOrPackageOpp()
                       || VN_IS(refp->classOrPackageOpp(), ClassOrPackageRef));
        };

        // Helper: allow types that can represent a class/package handle or an unresolved ref.
        // BasicDType is allowed here only for null/object handles.
        स्थिर स्वतः is_Valid_Type_गाँठ = [](स्थिर Ast_गाँठ* nodep) -> bool {
            वापस VN_IS(nodep, VoidDType) || VN_IS(nodep, BasicDType)
                   || VN_IS(nodep, ClassRefDType) || VN_IS(nodep, ParseRef);
        };

        अगर (स्थिर AstTypedef* स्थिर typedefp = VN_CAST(symp->nodep(), Typedef)) {
            स्थिर Ast_गाँठ_DType* dtypep = typedefp->subDTypep();
            अगर (!dtypep) dtypep = typedefp->childDTypep();
            अगर (VN_IS(dtypep, ClassRefDType)) वापस true;
            अगर (isPotentialClassRef(VN_CAST(dtypep, RefDType))) वापस true;
        } अन्यथा अगर (स्थिर AstParamTypeDType* स्थिर paramTypep
                   = VN_CAST(symp->nodep(), ParamTypeDType)) {
            // Before V3Param the declared default is in childDTypep (possibly
            // wrapped in a RequireDType); after V3Param it is consumed and the
            // bound type is the resolved data type, e.g. a type parameter
            // inherited from a specialized base class (REQ #(Item) -> class Item).
            Ast_गाँठ* childp = paramTypep->childDTypep();
            अगर (स्थिर AstRequireDType* स्थिर reqp = VN_CAST(childp, RequireDType)) {
                childp = reqp->lhsp();
            }
            स्थिर Ast_गाँठ* स्थिर checkp = childp ? childp : paramTypep->skipRefp();
            अगर (is_Valid_Type_गाँठ(checkp) || isPotentialClassRef(VN_CAST(checkp, RefDType))) {
                वापस true;
            }
        }
        वापस false;
    }
    VSymEnt* resolveClassOrPackage(VSymEnt* lookSymp, AstClassOrPackageRef* nodep, bool fallback,
                                   bool classOnly, स्थिर string& forWhat,
                                   bool deferIfUnresolved = false) {
        अगर (nodep->classOrPackageSkipp()) वापस get_गाँठ_Sym(nodep->classOrPackageSkipp());
        VSymEnt* foundp;
        VSymEnt* खोज_Symp = lookSymp;
        अगर (VL_UNCOVERABLE(खोज_Symp && VN_IS(खोज_Symp->nodep(), ParamTypeDType))) {
            खोज_Symp->nodep()->v3fatalSrc(  // LCOV_EXCL_LINE
                "resolveClassOrPackage: unexpected ParamTypeDType lookup");
        }
        अगर (fallback) {
            VSymEnt* currentLookSymp = खोज_Symp;
            करो {
                foundp = currentLookSymp->findIdFlat(nodep->name());
                अगर (foundp && !checkIfClassOrPackage(foundp)) foundp = nullptr;
                अगर (!foundp) currentLookSymp = currentLookSymp->fallbackp();
            } जबतक (!foundp && currentLookSymp);
        } अन्यथा {
            foundp = खोज_Symp->findIdFlat(nodep->name());
            अगर (foundp && !checkIfClassOrPackage(foundp)) foundp = nullptr;
        }
        अगर (!foundp && v3Global.rootp()->stdPackagep()) {  // Look under implied std::
            foundp = get_गाँठ_Sym(v3Global.rootp()->stdPackagep())->findIdFlat(nodep->name());
        }
        अगर (foundp) {
            nodep->classOrPackageNodep(foundp->nodep());
            वापस foundp;
        }
        अगर (deferIfUnresolved) वापस nullptr;
        स्थिर string suggest
            = suggestSymFallback(lookSymp, nodep->name(), Link_गाँठ_Matcher_Class_Or_Package{});
        nodep->v3error((classOnly ? "Class" : "Package/class")
                       << " for '" << forWhat  // extends/implements
                       << "' not found: " << nodep->prettyNameQ() << '\n'
                       << (suggest.empty() ? "" : nodep->warnMore() + suggest));
        वापस nullptr;
    }
    string suggestSymFallback(VSymEnt* lookupSymp, स्थिर string& name,
                              स्थिर VNodeMatcher& matcher) {
        // Suggest alternative symbol in given point in hierarchy
        // Does not support inline, as we find user-level errors before inlining
        // For simplicity lookupSymp may be passed nullptr result from findDotted
        अगर (!lookupSymp) वापस "";
        VSpellCheck speller;
        lookupSymp->candidateIdFallback(&speller, &matcher);
        वापस speller.best_Candidate_संदेश(name);
    }
    string suggestSymFlat(VSymEnt* lookupSymp, स्थिर string& name, स्थिर VNodeMatcher& matcher,
                          bool includeType) {
        अगर (!lookupSymp) वापस "";
        VSpellCheck speller;
        lookupSymp->candidateIdFlat(&speller, &matcher, includeType);
        वापस speller.best_Candidate_संदेश(name);
    }
};

LinkDotState* LinkDotState::s_errorThisp = nullptr;

//======================================================================

श्रेणी LinkDotFindVisitor final : खुला VNVisitor {
    // NODE STATE
    // Cleared on global
    //  *::user1p()             -> See LinkDotState
    //  *::user2p()             -> See LinkDotState
    //  *::user3()              // bool. Node stored in m_virtualIfaces
    //  *::user4()              -> See LinkDotState
    स्थिर VNUser3InUse m_inuser3;

    // STATE
    LinkDotState* स्थिर m_statep;  // State to pass between visitors, including symbol table
    Ast_गाँठ_Module* m_classOrPackagep = nullptr;  // Current package
    AstClocking* m_clockingp = nullptr;  // Current clocking block
    std::vector<AstScope*> m_scopes;  // Scopes detached during repeated module visits
    uint64_t m_statScopeVisits = 0;  // Non-top scopes encountered while finding instance symbols
    VSymEnt* m_modSymp = nullptr;  // Symbol Entry for current module
    VSymEnt* m_curSymp = nullptr;  // Symbol Entry for current table, where to lookup/insert
    string
        m_hierParamsName;  // Name of module with hierarchical type parameters, empty when not used
    string m_scope;  // Scope text
    स्थिर Ast_गाँठ_FTask* m_ftaskp = nullptr;  // Current function/task
    bool m_inRecursion = false;  // Inside a recursive module
    पूर्णांक m_param_अंक = 0;  // Parameter number, for position based connection
    bool m_explicitNew = false;  // Hit a "new" function
    पूर्णांक m_mod_Block_अंक = 0;  // Begin block number in module, 0=none seen
    पूर्णांक m_mod_With_अंक = 0;  // With block number, 0=none seen
    पूर्णांक m_mod_Arg_अंक = 0;  // Arg block number for randomize(), 0=none seen
    std::vector<AstIface*> m_virtIfaces;  // interfaces used as virtual,
                                          // needed for handleUnvisitedVirtIfaces()

    // METHODS
    व्योम makeImplicitNew(AstClass* nodep) {
        AstFunc* स्थिर newp = नया AstFunc{nodep->fileline(), "new", nullptr, nullptr};
        // If needed, super.new() call is added the 2nd pass of V3LinkDotResolve,
        // because base classes are already resolved there.
        newp->isConstructor(true);
        nodep->addMembersp(newp);
        UINFO(8, "Made implicit new for " << nodep->name() << ": " << nodep);
        m_statep->जोड़ो_Block(m_curSymp, newp->name(), newp, m_classOrPackagep);
    }

    अटल bool isHierBlockWrapper(स्थिर string& name) {
        स्थिर V3HierBlockOptSet& hierBlocks = v3Global.opt.hierBlocks();
        वापस hierBlocks.find(name) != hierBlocks.end();
    }

    व्योम moveExternFuncDecl(Ast_गाँठ_FTask* nodep, AstClass* classp) {
        // Move an 'extern function|task' to inside a class or, from an outer to inner-class
        अगर (nodep->isExternDef()) वापस;
        // Move it to proper spot under the target class
        nodep->unlinkFrBack();
        classp->addStmtsp(nodep);
        nodep->isExternDef(true);  // So we check there's a matching extern
        nodep->classOrPackagep()->unlinkFrBack()->delete_वृक्ष();
        // Any "Type::" reference in the function's IO are really "MovedToClass::" references
        अगर (nodep->fvarp()) moveExternFuncDeclRefs(nodep->fvarp(), classp);
        क्रम (Ast_गाँठ* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            अगर (AstVar* स्थिर portp = VN_CAST(stmtp, Var)) {
                अगर (portp->isIO()) moveExternFuncDeclRefs(portp, classp);
            }
        }
    }
    व्योम moveExternFuncDeclRefs(Ast_गाँठ* nodep, AstClass* classp) {
        nodep->foreach([यह, classp](AstClassOrPackageRef* refp) {  //
            अगर (refp->name() == classp->name() && !refp->paramsp()) {
                UINFO(9, "Cleaning up external function type for class " << refp);
                AstDot* स्थिर dotp = VN_CAST(refp->backp(), Dot);
                अगर (dotp && dotp->lhsp() == refp) {
                    // If had DOT(CLASSREF this, ...) the DOT can go away
                    dotp->replaceWith(dotp->rhsp()->unlinkFrBack());
                    VL_DO_DANGLING2(धकेलो_Deletep(dotp), dotp, refp);
                } अन्यथा {
                    VL_DO_DANGLING(धकेलो_Deletep(refp->unlinkFrBack()), refp);
                }
            }
        });
    }

    // VISITORS
    व्योम visit(AstNetlist* nodep) override {  // FindVisitor::
        // Process $unit or other packages
        // Not needed - dotted references not allowed from inside packages
        // for (AstNodeModule* nodep = v3Global.rootp()->modulesp();
        //     nodep; nodep=VN_AS(nodep->nextp(), NodeModule)) {
        //    if (VN_IS(nodep, Package)) {}}

        m_statep->जोड़ो_DUnit(nodep);

        // First back iterate, to find all packages. Backward as must do base
        // packages before using packages
        iterateChildrenBackwardsConst(nodep);

        // The first modules in the list are always the top modules
        // (sorted before this is called).
        // This may not be the module with isTop() set, as early in the steps,
        // wrapTop may have not been created yet.
        // $unit and the constant pool always exist, so nothing else, and nothing in $unit,
        // means nothing was given
        Ast_गाँठ_Module* स्थिर modulesp = nodep->modulesp();
        UASSERT_OBJ(modulesp, nodep, "$unit should always be in the netlist");
        अगर (m_statep->forPrimary()) {
            स्थिर bool onlyBuiltins = [&]() {
                क्रम (Ast_गाँठ_Module* modp = modulesp; modp;
                     modp = VN_AS(modp->nextp(), गाँठ_Module)) {
                    अगर (!modp->isDollarUnit() && !modp->isConstPool()) वापस false;
                }
                वापस true;
            }();
            अगर (onlyBuiltins && !v3Global.rootp()->dollarUnitPkgp()->stmtsp()) {
                nodep->v3error("No top level module found");
            }
        }
        क्रम (Ast_गाँठ_Module* modp = nodep->modulesp(); modp && modp->is_शीर्ष();
             modp = VN_AS(modp->nextp(), गाँठ_Module)) {
            UINFO(8, "Top Module: " << modp);
            m_scope = "TOP";

            अगर (!m_statep->forScopeCreation() && v3Global.opt.शीर्ष_Ifaces_Supported()) {
                क्रम (Ast_गाँठ* subnodep = modp->stmtsp(); subnodep; subnodep = subnodep->nextp()) {
                    अगर (AstVar* स्थिर varp = VN_CAST(subnodep, Var)) {
                        अगर (varp->isIfaceRef()) {
                            स्थिर Ast_गाँठ_DType* स्थिर subtypep = varp->subDTypep();
                            स्थिर AstIfaceRefDType* ifacerefp = nullptr;
                            अगर (VN_IS(subtypep, IfaceRefDType)) {
                                ifacerefp = VN_AS(varp->subDTypep(), IfaceRefDType);
                            } अन्यथा अगर (VN_IS(subtypep, BracketArrayDType)) {
                                स्थिर AstBracketArrayDType* स्थिर arrp
                                    = VN_AS(subtypep, BracketArrayDType);
                                स्थिर Ast_गाँठ_DType* स्थिर arrsubtypep = arrp->subDTypep();
                                अगर (VN_IS(arrsubtypep, IfaceRefDType)) {
                                    ifacerefp = VN_AS(arrsubtypep, IfaceRefDType);
                                }
                            } अन्यथा अगर (VN_IS(subtypep, UnpackArrayDType)) {
                                स्थिर AstUnpackArrayDType* स्थिर arrp
                                    = VN_AS(subtypep, UnpackArrayDType);
                                स्थिर Ast_गाँठ_DType* स्थिर arrsubtypep = arrp->subDTypep();
                                अगर (VN_IS(arrsubtypep, IfaceRefDType)) {
                                    ifacerefp = VN_AS(arrsubtypep, IfaceRefDType);
                                }
                            }

                            अगर (ifacerefp && !ifacerefp->cellp()) {
                                // A dummy cell to keep the top level interface alive and correctly
                                // optimized for default parameter values
                                AstCell* ifacecellp
                                    = नया AstCell{nodep->fileline(),
                                                  nodep->fileline(),
                                                  modp->name() + "__02E" + varp->name(),
                                                  ifacerefp->ifaceName(),
                                                  nullptr,
                                                  nullptr,
                                                  nullptr};
                                ifacecellp->modp(ifacerefp->ifacep());
                                m_curSymp = m_modSymp
                                    = m_statep->जोड़ो_शीर्ष_Iface(ifacecellp, m_scope);
                                iterate(ifacecellp);
                                VL_DO_DANGLING(धकेलो_Deletep(ifacecellp), ifacecellp);
                            }
                        }
                    }
                }
            }

            m_curSymp = m_modSymp = m_statep->जोड़ो_शीर्ष_Cell(modp, m_scope);
            { iterate(modp); }

            m_scope = "";
            m_curSymp = m_modSymp = nullptr;
        }
    }
    व्योम visit(AstTypeTable*) override {}  // FindVisitor::
    व्योम visit(AstIfaceRefDType* nodep) override {  // FindVisitor::
        अगर (!m_statep->forScopeCreation() && nodep->isVirtual() && nodep->ifacep()
            && !nodep->ifacep()->user3()) {
            m_virtIfaces.धकेलो_back(nodep->ifacep());
            nodep->ifacep()->user3(true);
        }
        iterateChildren(nodep);
    }
    व्योम visit(Ast_गाँठ_Module* nodep) override {  // FindVisitor::
        // Called on top module from Netlist, other modules from the cell creating them,
        // and packages
        UINFO(8, "   " << nodep);
        // m_curSymp/m_modSymp maybe nullptr for packages and non-top modules
        // Packages will be under top after the initial phases, but until then
        // need separate handling
        स्थिर bool standalonePkg
            = !m_modSymp && !m_statep->forScopeCreation() && VN_IS(nodep, Package);
        स्थिर bool doit = (m_modSymp || standalonePkg);
        VL_RESTORER_COPY(m_scope);
        VL_RESTORER(m_classOrPackagep);
        VL_RESTORER(m_modSymp);
        VL_RESTORER(m_curSymp);
        VL_RESTORER(m_param_अंक);
        VL_RESTORER(m_mod_Block_अंक);
        VL_RESTORER(m_mod_With_अंक);
        VL_RESTORER(m_mod_Arg_अंक);
        अगर (doit && nodep->user2()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Identically recursive module (module instantiates "
                          "itself, without changing parameters): "
                              << Ast_गाँठ::prettyNameQ(nodep->origName()));
        } अन्यथा अगर (doit) {
            UINFO(4, "     Link Module: " << nodep);
            UASSERT_OBJ(!nodep->dead(), nodep, "Module in instance tree mislabeled as dead?");
            AstPackage* स्थिर pkgp = VN_CAST(nodep, Package);
            m_classOrPackagep = pkgp;
            अगर (standalonePkg) {
                अगर (pkgp->isDollarUnit()) {
                    m_curSymp = m_modSymp = m_statep->dunitEntp();
                    nodep->user1p(m_curSymp);
                } अन्यथा {
                    VSymEnt* स्थिर upperSymp = m_statep->dunitEntp();
                    m_scope = nodep->name();
                    m_curSymp = m_modSymp = m_statep->जोड़ो_Block(upperSymp, nodep->name(), nodep,
                                                                  m_classOrPackagep);
                    UINFO(9, "New module scope " << m_curSymp);
                }
            }
            //
            m_param_अंक = 0;
            m_mod_Block_अंक = 0;
            m_mod_With_अंक = 0;
            m_mod_Arg_अंक = 0;
            // m_modSymp/m_curSymp for non-packages set by AstCell above this module
            // Iterate
            nodep->user2(true);
            iterateChildren(nodep);
            nodep->user2(false);
            nodep->user4(true);
            // Interfaces need another pass when signals are resolved. When creating
            // scopes this is done from the AstScopes in LinkDotScopeVisitor instead, as
            // only those entries hold the AstVarScopes the modport items resolve against.
            अगर (!m_statep->forScopeCreation()) {
                अगर (AstIface* स्थिर ifacep = VN_CAST(nodep, Iface)) {
                    m_statep->जोड़ो_Iface_Mod_Sym(ifacep, m_curSymp);
                }
            }
        } अन्यथा अगर (isHierBlockWrapper(nodep->name())) {
            UINFO(5, "Module is hierarchical block, must not be dead: " << nodep);
            m_scope = nodep->name();
            VSymEnt* स्थिर upperSymp = m_curSymp ? m_curSymp : m_statep->मूल_Entp();
            m_curSymp = m_modSymp
                = m_statep->जोड़ो_Block(upperSymp, nodep->name() + "::", nodep, m_classOrPackagep);
            iterateChildren(nodep);
            nodep->user4(true);
        } अन्यथा {  // !doit
            अगर (nodep->hierParams()) {
                UINFO(1, "Found module with hier type parameters");
                m_hierParamsName = nodep->name();
                क्रम (स्थिर Ast_गाँठ* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    अगर (स्थिर AstTypedef* स्थिर tdef = VN_CAST(stmtp, Typedef)) {
                        UINFO(1, "Inserting hier type parameter typedef: " << tdef);
                        VSymEnt* स्थिर upperSymp = m_curSymp ? m_curSymp : m_statep->मूल_Entp();
                        m_curSymp = m_modSymp = m_statep->जोड़ो_Block(upperSymp, nodep->name(),
                                                                      nodep, m_classOrPackagep);
                    }
                }
            } अन्यथा {
                // Will be optimized away later
                // Can't remove now, as our backwards iterator will throw up
                UINFO(5, "Module not under any CELL or top - dead module: " << nodep);
            }
        }
    }

    व्योम visit(AstClass* nodep) override {  // FindVisitor::
        UASSERT_OBJ(m_curSymp, nodep, "Class not under module/package/$unit");
        UINFO(8, "   " << nodep);
        VL_RESTORER_COPY(m_scope);
        VL_RESTORER(m_classOrPackagep);
        VL_RESTORER(m_modSymp);
        VL_RESTORER(m_curSymp);
        VL_RESTORER(m_param_अंक);
        VL_RESTORER(m_mod_Block_अंक);
        VL_RESTORER(m_mod_With_अंक);
        VL_RESTORER(m_mod_Arg_अंक);
        VL_RESTORER(m_explicitNew);
        {
            UINFO(4, "     Link Class: " << nodep);
            VSymEnt* स्थिर upperSymp = m_curSymp;
            m_scope = m_scope + "." + nodep->name();
            m_classOrPackagep = nodep;
            m_curSymp = m_modSymp
                = m_statep->जोड़ो_Block(upperSymp, nodep->name(), nodep, m_classOrPackagep);
            m_statep->जोड़ो_Map(m_curSymp, m_scope);
            UINFO(9, "New module scope " << m_curSymp);
            //
            m_param_अंक = 0;
            m_mod_Block_अंक = 0;
            m_mod_With_अंक = 0;
            m_mod_Arg_अंक = 0;
            m_explicitNew = false;
            // m_modSymp/m_curSymp for non-packages set by AstCell above this module
            // Iterate
            iterateChildren(nodep);
            nodep->user4(true);
            // Implicit new needed?
            अगर (!m_explicitNew && m_statep->forPrimary()) makeImplicitNew(nodep);
        }
    }
    व्योम visit(AstClassOrPackageRef* nodep) override {  // FindVisitor::
        अगर (!nodep->classOrPackageNodep() && nodep->name() == "$unit") {
            nodep->classOrPackageNodep(v3Global.rootp()->dollarUnitPkgp());
        }
        iterateChildren(nodep);
    }
    व्योम visit(AstScope* nodep) override {  // FindVisitor::
        UASSERT_OBJ(m_statep->forScopeCreation(), nodep,
                    "Scopes should only exist right after V3Scope");
        // Detach on first encounter, will re add at the end.
        // Otherwise would be visited O(cells^2) time after V3Scope
        अगर (!nodep->is_शीर्ष()) {
            ++m_statScopeVisits;
            nodep->unlinkFrBack();
            m_scopes.धकेलो_back(nodep);
        }
    }
    व्योम visit(AstCell* nodep) override {  // FindVisitor::
        UINFO(5, "   CELL under " << m_scope << " is " << nodep);
        // Process XREFs/etc inside pins
        अगर (nodep->recursive() && m_inRecursion) वापस;
        iterateChildren(nodep);
        // Recurse in, preserving state
        VL_RESTORER_COPY(m_scope);
        VL_RESTORER(m_modSymp);
        VL_RESTORER(m_curSymp);
        VL_RESTORER(m_param_अंक);
        VL_RESTORER(m_inRecursion);
        // Where do we add it?
        VSymEnt* aboveSymp = m_curSymp;
        स्थिर string origname = Ast_गाँठ::dedotName(nodep->name());
        string::आकार_type pos = origname.rfind('.');
        अगर (pos != string::npos) {
            // Flattened, find what CellInline it should live under
            स्थिर string scope = origname.substr(0, pos);
            string baddot;
            VSymEnt* okSymp;
            aboveSymp
                = m_statep->findDotted(nodep->fileline(), aboveSymp, scope, baddot, okSymp, false);
            UASSERT_OBJ(aboveSymp, nodep,
                        "Can't find instance insertion point at "
                            << Ast_गाँठ::prettyNameQ(baddot) << " in: " << nodep->prettyNameQ());
        }
        {
            m_scope = m_scope + "." + nodep->name();
            m_curSymp = m_modSymp = m_statep->जोड़ो_Cell(aboveSymp, m_modSymp, nodep, m_scope);
            m_inRecursion = nodep->recursive();
            // We don't report NotFoundModule, as may be a unused module in a generate
            अगर (nodep->modp()) iterate(nodep->modp());
        }
    }
    व्योम visit(AstCellInline* nodep) override {  // FindVisitor::
        UINFO(5, "   CELLINLINE under " << m_scope << " is " << nodep);
        VSymEnt* aboveSymp = m_curSymp;
        std::string ident = nodep->name();
        // If baz__DOT__foo__DOT__bar, we need to find baz__DOT__foo and add bar to it.
        स्थिर std::string::आकार_type pos = ident.rfind("__DOT__");
        अगर (pos != std::string::npos) {
            स्थिर std::string dotted = ident.substr(0, pos);
            ident = ident.substr(pos + std::म_माप("__DOT__"));
            std::string baddot;
            VSymEnt* okSymp;
            aboveSymp = m_statep->findDotted(nodep->fileline(), aboveSymp, dotted, baddot, okSymp,
                                             false);
            UASSERT_OBJ(aboveSymp, nodep,
                        "Can't find cellinline insertion point at "
                            << Ast_गाँठ::prettyNameQ(baddot) << " in: " << nodep->prettyNameQ());
        }
        m_statep->जोड़ो_Inline(aboveSymp, m_modSymp, nodep, ident);
    }
    व्योम visit(AstDefParam* nodep) override {  // FindVisitor::
        nodep->user1p(m_curSymp);
        iterateChildren(nodep);
    }
    व्योम visit(AstRefDType* nodep) override {  // FindVisitor::
        nodep->user1p(m_curSymp);
        iterateChildren(nodep);
    }
    व्योम visit(AstGenBlock* nodep) override {  // FindVisitor::
        UINFO(5, "   " << nodep);
        अगर (nodep->name() == "" && nodep->unnamed()) {
            // Unnamed blocks are only important when they contain var
            // decls, so search for them. (Otherwise adding all the
            // unnamed#'s would just confuse tracing variables in
            // places such as tasks, where "task ...; begin ... end"
            // are common.
            क्रम (Ast_गाँठ* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
                अगर (VN_IS(itemp, Var) || VN_IS(itemp, Foreach)) {
                    std::string name;
                    स्थिर std::string stepStr = m_statep->forPrimary()
                                                    ? ""
                                                    : std::to_string(m_statep->step_संख्या()) + "_";
                    करो {
                        ++m_mod_Block_अंक;
                        name = "unnamedblk" + stepStr + cvtToStr(m_mod_Block_अंक);
                        // Increment again if earlier pass of V3LinkDot claimed this name
                    } जबतक (m_curSymp->findIdFlat(name));
                    nodep->name(name);
                    अवरोध;
                }
            }
        }
        अगर (nodep->name() == "") {
            iterateChildren(nodep);
        } अन्यथा {
            VL_RESTORER(m_curSymp);
            {
                m_curSymp
                    = m_statep->जोड़ो_Block(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
                m_curSymp->fallbackp(VL_RESTORER_पूर्व(m_curSymp));
                // Iterate
                iterateChildren(nodep);
            }
        }
    }
    व्योम visit(Ast_गाँठ_Block* nodep) override {  // FindVisitor::
        UINFO(5, "   " << nodep);
        अगर (nodep->name() == "" && nodep->unnamed()) {
            // Unnamed blocks are only important when they contain var
            // decls, so search for them. (Otherwise adding all the
            // unnamed#'s would just confuse tracing variables in
            // places such as tasks, where "task ...; begin ... end"
            // are common.
            स्थिर स्वतः containsDecl = [](स्थिर Ast_गाँठ* nodesp) -> bool {
                क्रम (स्थिर Ast_गाँठ* np = nodesp; np; np = np->nextp()) {
                    अगर (VN_IS(np, Var) || VN_IS(np, Foreach)) वापस true;
                }
                वापस false;
            };
            bool needName = nodep->declsp() || containsDecl(nodep->stmtsp());
            अगर (स्थिर AstFork* स्थिर forkp = VN_CAST(nodep, Fork)) {
                needName = needName || containsDecl(forkp->forksp());
            }
            अगर (needName) {
                स्थिर std::string stepStr
                    = m_statep->forPrimary() ? "" : std::to_string(m_statep->step_संख्या()) + "_";
                std::string name;
                करो {
                    ++m_mod_Block_अंक;
                    name = "unnamedblk" + stepStr + cvtToStr(m_mod_Block_अंक);
                    // Increment again if earlier pass of V3LinkDot claimed this name
                } जबतक (m_curSymp->findIdFlat(name));
                nodep->name(name);
            }
        }
        अगर (nodep->name() == "") {
            iterateChildren(nodep);
        } अन्यथा {
            VL_RESTORER(m_curSymp);
            {
                m_curSymp
                    = m_statep->जोड़ो_Block(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
                m_curSymp->fallbackp(VL_RESTORER_पूर्व(m_curSymp));
                // Iterate
                iterateChildren(nodep);
            }
        }
    }
    // Handle out-of-block interface method definition (IEEE 1800-2023 25.7)
    // Move task body from module into the interface's prototype task,
    // rewriting port-prefixed references in the body.
    व्योम moveIfaceExportBody(Ast_गाँठ_FTask* nodep) {
        स्थिर string& portName = nodep->ifacePortName();
        UINFO(5, "   moveIfaceExportBody: port=" << portName << " task=" << nodep->name());
        // Look up the interface port variable in the module's symbol table
        VSymEnt* स्थिर portSymp = m_modSymp->findIdFallback(portName);
        अगर (!portSymp) {
            nodep->v3error("Interface port not found for out-of-block definition: '"
                           << portName << "' (IEEE 1800-2023 25.7)");
            nodep->unlinkFrBack();
            VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
            वापस;
        }
        AstVar* स्थिर portp = VN_CAST(portSymp->nodep(), Var);
        अगर (!portp) {
            nodep->v3fatalSrc("Out-of-block definition port is not a variable: " << portName);
            वापस;
        }
        // Get the interface name from the port's type
        // At this stage dtypep() may not be set yet; check childDTypep() too
        AstIfaceRefDType* ifaceRefDtp = nullptr;
        अगर (portp->dtypep()) { ifaceRefDtp = VN_CAST(portp->dtypep(), IfaceRefDType); }
        अगर (!ifaceRefDtp && portp->childDTypep()) {
            ifaceRefDtp = VN_CAST(portp->childDTypep(), IfaceRefDType);
        }
        अगर (!ifaceRefDtp) {
            nodep->v3error("Out-of-block definition port is not an interface port: '"
                           << portName << "' (IEEE 1800-2023 25.7)");
            nodep->unlinkFrBack();
            VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
            वापस;
        }
        स्थिर string ifaceName = ifaceRefDtp->ifaceName();
        UINFO(5, "   moveIfaceExportBody: iface=" << ifaceName);
        // Find the interface module by name
        Ast_गाँठ_Module* स्थिर ifaceModp = m_statep->findModuleSym(ifaceName);
        अगर (!ifaceModp) {
            nodep->v3fatalSrc("Interface not found for out-of-block definition: " << ifaceName);
            वापस;
        }
        // Find the prototype task in the interface
        Ast_गाँठ_FTask* protoTaskp = nullptr;
        क्रम (Ast_गाँठ* itemp = ifaceModp->stmtsp(); itemp; itemp = itemp->nextp()) {
            अगर (Ast_गाँठ_FTask* स्थिर ftaskp = VN_CAST(itemp, गाँठ_FTask)) {
                अगर (ftaskp->name() == nodep->name() && ftaskp->prototype()) {
                    protoTaskp = ftaskp;
                    अवरोध;
                }
            }
        }
        अगर (!protoTaskp) {
            nodep->v3error("No matching export prototype found in interface "
                           << ifaceName << " for task " << nodep->prettyNameQ()
                           << " (IEEE 1800-2023 25.7)");
            nodep->unlinkFrBack();
            VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
            वापस;
        }
        // Move body statements (non-port declarations) from the out-of-block task
        // to the prototype task. Rewrite port.X references to just X in the body.
        क्रम (Ast_गाँठ* stmtp = nodep->stmtsp(); stmtp;) {
            Ast_गाँठ* स्थिर nextp = stmtp->nextp();
            // Skip port declarations -- they already exist in the prototype
            अगर (स्थिर AstVar* स्थिर varp = VN_CAST(stmtp, Var)) {
                अगर (varp->isIO()) {
                    stmtp = nextp;
                    जारी;
                }
            }
            // Clone this statement and rewrite port references
            Ast_गाँठ* स्थिर newStmtp = stmtp->clone_वृक्ष(false);
            rewriteIfacePortRefsSingle(newStmtp, portName);
            protoTaskp->addStmtsp(newStmtp);
            stmtp = nextp;
        }
        // Mark prototype as no longer prototype (it has a body now)
        protoTaskp->prototype(false);
        // Delete the out-of-block task from the module
        nodep->unlinkFrBack();
        VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
    }

    // Rewrite AstDot nodes where lhsp is AstParseRef with the port name
    // to just the rhsp (stripping the port prefix).
    अटल व्योम rewriteIfacePortRefsSingle(Ast_गाँठ* nodep, स्थिर string& portName) {
        अगर (!nodep) वापस;
        // Check if this is a non-colon Dot with lhs matching portName
        अगर (AstDot* स्थिर dotp = VN_CAST(nodep, Dot)) {
            अगर (!dotp->colon()) {
                स्थिर AstParseRef* स्थिर lhsRefp = VN_CAST(dotp->lhsp(), ParseRef);
                अगर (lhsRefp && lhsRefp->name() == portName) {
                    UINFO(5, "   rewriteIfacePortRefs: stripping port prefix from " << dotp);
                    Ast_गाँठ* स्थिर rhsp = dotp->rhsp()->unlinkFrBack();
                    dotp->replaceWith(rhsp);
                    VL_DO_DANGLING(dotp->delete_वृक्ष(), dotp);
                    rewriteIfacePortRefsSingle(rhsp, portName);
                    वापस;
                }
            }
        }
        // Recurse into all children
        क्रम (Ast_गाँठ* childp : {nodep->op1p(), nodep->op2p(), nodep->op3p(), nodep->op4p()}) {
            जबतक (childp) {
                Ast_गाँठ* स्थिर nextp = childp->nextp();
                rewriteIfacePortRefsSingle(childp, portName);
                childp = nextp;
            }
        }
    }

    व्योम visit(Ast_गाँठ_FTask* nodep) override {  // FindVisitor::
        // NodeTask: Remember its name for later resolution
        UINFO(5, "   " << nodep);
        UASSERT_OBJ(m_curSymp && m_modSymp, nodep, "Function/Task not under module?");
        अगर (nodep->name() == "new") m_explicitNew = true;
        // Handle out-of-block interface method definition (IEEE 25.8)
        अगर (!nodep->ifacePortName().empty() && m_statep->forPrimary()) {
            moveIfaceExportBody(nodep);
            वापस;  // Task has been deleted
        }
        // Remember the existing symbol table scope
        VL_RESTORER(m_classOrPackagep);
        VL_RESTORER(m_curSymp);
        VSymEnt* upSymp = m_curSymp;

        अगर (VN_IS(m_curSymp->nodep(), Class)) {
            अगर (VN_AS(m_curSymp->nodep(), Class)->isInterfaceClass() && !nodep->pureVirtual()
                && !nodep->isConstructor()) {
                nodep->v3error("Interface class functions must be pure virtual"
                               << " (IEEE 1800-2023 8.26): " << nodep->prettyNameQ());
            }
            अगर (m_statep->forPrimary()
                && (nodep->name() == "randomize" || nodep->name() == "srandom")) {
                nodep->v3error(nodep->prettyNameQ()
                               << " is a predefined class method; redefinition not allowed"
                                  " (IEEE 1800-2023 18.6.3)");
            }
        }
        // Change to appropriate package if extern declaration (vs definition)
        अगर (nodep->classOrPackagep()) {
            // class-in-class
            AstDot* स्थिर dotp = VN_CAST(nodep->classOrPackagep(), Dot);
            AstClassOrPackageRef* स्थिर cpackagerefp
                = VN_CAST(nodep->classOrPackagep(), ClassOrPackageRef);
            अगर (dotp) {
                AstClassOrPackageRef* स्थिर lhsp = VN_AS(dotp->lhsp(), ClassOrPackageRef);
                m_statep->resolveClassOrPackage(m_curSymp, lhsp, true, false,
                                                "External definition :: reference");
                AstClass* स्थिर lhsclassp = VN_CAST(lhsp->classOrPackageSkipp(), Class);
                अगर (!lhsclassp) {
                    nodep->v3error("Extern declaration's scope is not a defined class");
                } अन्यथा {
                    m_curSymp = m_statep->get_गाँठ_Sym(lhsclassp);
                    upSymp = m_curSymp;
                    AstClassOrPackageRef* स्थिर rhsp = VN_AS(dotp->rhsp(), ClassOrPackageRef);
                    m_statep->resolveClassOrPackage(m_curSymp, rhsp, true, false,
                                                    "External definition :: reference");
                    AstClass* स्थिर rhsclassp = VN_CAST(rhsp->classOrPackageSkipp(), Class);
                    अगर (!rhsclassp) {
                        nodep->v3error("Extern declaration's scope is not a defined class");
                    } अन्यथा {
                        m_curSymp = m_statep->get_गाँठ_Sym(rhsclassp);
                        upSymp = m_curSymp;
                        moveExternFuncDecl(nodep, rhsclassp);
                    }
                }
            } अन्यथा अगर (cpackagerefp) {
                अगर (!cpackagerefp->classOrPackageSkipp()) {
                    m_statep->resolveClassOrPackage(m_curSymp, cpackagerefp, true, false,
                                                    "External definition :: reference");
                }
                AstClass* स्थिर classp = VN_CAST(cpackagerefp->classOrPackageSkipp(), Class);
                अगर (!classp) {
                    nodep->v3error("Extern declaration's scope is not a defined class");
                } अन्यथा {
                    m_curSymp = m_statep->get_गाँठ_Sym(classp);
                    upSymp = m_curSymp;
                    moveExternFuncDecl(nodep, classp);
                }
            } अन्यथा {
                v3fatalSrc("Unhandled extern function definition package");
            }
        }
        // Set the class as package for iteration
        अगर (VN_IS(m_curSymp->nodep(), Class)) {
            m_classOrPackagep = VN_AS(m_curSymp->nodep(), Class);
        }
        // Create symbol table for the task's vars
        स्थिर string name = (nodep->isExternProto() ? "extern "s : ""s) + nodep->name();
        m_curSymp = m_statep->जोड़ो_Block(m_curSymp, name, nodep, m_classOrPackagep);
        m_curSymp->fallbackp(upSymp);
        // Convert the func's range to the output variable
        // This should probably be done in the Parser instead, as then we could
        // just attach normal signal attributes to it.
        अगर (!nodep->isExternProto() && nodep->fvarp() && !VN_IS(nodep->fvarp(), Var)) {
            Ast_गाँठ_DType* dtypep = VN_CAST(nodep->fvarp(), गाँठ_DType);
            // If unspecified, function returns one bit; however when we
            // support NEW() it could also return the class reference.
            अगर (dtypep) {
                dtypep->unlinkFrBack();
            } अन्यथा {
                dtypep = नया AstBasicDType{nodep->fileline(), VBasicDTypeKwd::LOGIC};
            }
            AstVar* स्थिर newvarp
                = नया AstVar{nodep->fileline(), VVarType::VAR, nodep->name(), VFlagChildDType{},
                             dtypep};  // Not dtype resolved yet
            newvarp->direction(VDirection::OUTPUT);
            newvarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            newvarp->funcReturn(true);
            newvarp->trace(false);  // Not user visible
            nodep->fvarp(newvarp);
            // Explicit insert required, as the var name shadows the upper level's task name
            m_statep->जोड़ो_Sym(m_curSymp, newvarp->name(), newvarp, nullptr /*classOrPackagep*/);
        }
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    व्योम visit(AstClocking* nodep) override {  // FindVisitor::
        VL_RESTORER(m_clockingp);
        m_clockingp = nodep;
        iterate(nodep->sentreep());
        iterate_And_अग्र_Null(nodep->itemsp());
        // If the block has no name, one cannot reference the clockvars
        VSymEnt* itSymp = nullptr;
        अगर (nodep->isGlobal()  //
            && m_statep->forPrimary()) {  // else flattening may see two globals
            m_statep->checkDuplicate(m_curSymp, nodep, "__024global_clock");
            itSymp
                = m_statep->जोड़ो_Block(m_curSymp, "__024global_clock", nodep, m_classOrPackagep);
            itSymp->fallbackp(nullptr);
        }
        अगर (!nodep->name().empty()) {
            itSymp = m_statep->जोड़ो_Block(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
            itSymp->fallbackp(nullptr);
        }
        अगर (itSymp) {
            VL_RESTORER(m_curSymp);
            m_curSymp = itSymp;
            iterate_And_अग्र_Null(nodep->itemsp());
        }
    }
    व्योम visit(AstClockingItem* nodep) override {  // FindVisitor::
        अगर (nodep->varp()) {
            अगर (m_curSymp->nodep() == m_clockingp) iterate(nodep->varp());
            वापस;
        }
        std::string varname;
        Ast_गाँठ_DType* dtypep;
        अगर (AstAssign* स्थिर assignp = nodep->assignp()) {
            Ast_गाँठ_व्यंजक* स्थिर rhsp = assignp->rhsp()->unlinkFrBack();
            dtypep = नया AstRefDType{nodep->fileline(), AstRefDType::Flag_Type_Of_व्यंजक{},
                                     rhsp->clone_वृक्ष(false)};
            nodep->exprp(rhsp);
            varname = assignp->lhsp()->name();
            VL_DO_DANGLING(assignp->unlinkFrBack()->delete_वृक्ष(), assignp);
        } अन्यथा {
            Ast_गाँठ_व्यंजक* स्थिर refp = nodep->exprp();
            स्थिर VSymEnt* foundp = m_curSymp->findIdFallback(refp->name());
            अगर (!foundp || !foundp->nodep()) {
                refp->v3error("Corresponding variable " << refp->prettyNameQ()
                                                        << " does not exist");
                VL_DO_DANGLING(nodep->unlinkFrBack()->delete_वृक्ष(), nodep);
                वापस;
            }
            varname = refp->name();
            dtypep = VN_AS(foundp->nodep(), Var)->childDTypep()->clone_वृक्ष(false);
        }
        AstVar* स्थिर newvarp = नया AstVar{nodep->fileline(), VVarType::MODULETEMP, varname,
                                           VFlagChildDType{}, dtypep};
        newvarp->lifetime(VLifetime::STATIC_EXPLICIT);
        nodep->varp(newvarp);
        iterate(nodep->exprp());
    }
    व्योम visit(AstConstraint* nodep) override {  // FindVisitor::
        VL_RESTORER(m_curSymp);
        // Change to appropriate package if extern declaration (vs definition)
        VSymEnt* upSymp = m_curSymp;
        अगर (nodep->classOrPackagep()) {
            AstClassOrPackageRef* स्थिर cpackagerefp
                = VN_CAST(nodep->classOrPackagep(), ClassOrPackageRef);
            अगर (!cpackagerefp) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: extern constraint definition with class-in-class");
            } अन्यथा {
                अगर (!cpackagerefp->classOrPackageSkipp()) {
                    m_statep->resolveClassOrPackage(m_curSymp, cpackagerefp, true, false,
                                                    "External definition :: reference");
                }
                AstClass* स्थिर classp = VN_CAST(cpackagerefp->classOrPackageSkipp(), Class);
                अगर (!classp) {
                    nodep->v3error("Extern declaration's scope is not a defined class");
                } अन्यथा {
                    m_curSymp = m_statep->get_गाँठ_Sym(classp);
                    upSymp = m_curSymp;
                    // Move it to proper spot under the target class
                    nodep->unlinkFrBack();
                    classp->addStmtsp(nodep);
                    nodep->classOrPackagep()->unlinkFrBack()->delete_वृक्ष();
                }
            }
        }
        // Set the class as package for iteration
        स्थिर string name = (nodep->isExternProto() ? "extern "s : ""s) + nodep->name();
        m_curSymp = m_statep->जोड़ो_Block(upSymp, name, nodep, m_classOrPackagep);
        iterateChildren(nodep);
    }
    व्योम visit(AstVar* nodep) override {  // FindVisitor::
        // Var: Remember its name for later resolution
        UASSERT_OBJ(m_curSymp && m_modSymp, nodep, "Var not under module?");
        iterateChildren(nodep);
        अगर (VN_IS(m_curSymp->nodep(), Class)
            && VN_AS(m_curSymp->nodep(), Class)->isInterfaceClass() && !nodep->isParam()) {
            nodep->v3error("Interface class cannot contain non-parameter members"
                           << " (IEEE 1800-2023 8.26): " << nodep->prettyNameQ());
        }
        अगर (!m_statep->forScopeCreation()) {
            // Find under either a task or the module's vars
            स्थिर VSymEnt* foundp = m_curSymp->findIdFallback(nodep->name());
            अगर (!foundp && m_modSymp && nodep->name() == m_modSymp->nodep()->name()) {
                foundp = m_modSymp;  // Conflicts with modname?
            }
            AstVar* स्थिर findvarp = foundp ? VN_CAST(foundp->nodep(), Var) : nullptr;
            // clocking items can have duplicate names (inout)
            अगर (findvarp && VN_IS(findvarp->backp(), ClockingItem)
                && VN_IS(nodep->backp(), ClockingItem)) {
                AstClockingItem* स्थिर itemp = VN_AS(nodep->backp(), ClockingItem);
                AstClockingItem* स्थिर finditemp = VN_AS(findvarp->backp(), ClockingItem);
                UINFO(4, "ClockCompl: " << itemp << " ;; " << finditemp);
                UINFO(4, "ClockCompV: " << nodep << " ;; " << findvarp);
                अगर (*itemp->exprp()->fileline() == *finditemp->exprp()->fileline()) {
                    UASSERT_OBJ(finditemp->direction() == VDirection::निवेश
                                    && itemp->direction() == VDirection::OUTPUT,
                                itemp, "Input after output?");
                    // pretend nothing found and rename
                    foundp = nullptr;
                    nodep->name("__Voutput_" + nodep->name());
                    finditemp->outputp(itemp);
                }
            }
            bool म_लो = false;
            अगर (!foundp) {
                म_लो = true;
            } अन्यथा अगर (!findvarp && m_curSymp->findIdFlat(nodep->name()) && !foundp->imported()) {
                nodep->v3error("Unsupported in C: Variable has same name as "
                               << LinkDotState::गाँठ_Text_Type(foundp->nodep()) << ": "
                               << nodep->prettyNameQ());
            } अन्यथा अगर (findvarp != nodep) {
                UINFO(4, "DupVar: " << nodep << " ;; " << foundp->nodep());
                UINFO(4, "    found  cur=se" << cvtToHex(m_curSymp) << " ;; parent=se"
                                             << cvtToHex(foundp->parentp()));
                अगर (foundp->parentp() == m_curSymp  // Only when on same level
                    && !foundp->imported()) {  // and not from package
                    अगर (VN_IS(m_curSymp->nodep(), Clocking)) {
                        nodep->v3error("Multiple clockvars with the same name not allowed");
                        वापस;
                    }
                    स्थिर bool nansiBad
                        = (((findvarp->isDeclTyped() || findvarp->isNet())
                            && (nodep->isDeclTyped() || nodep->isNet()))
                           || (findvarp->isIO() && nodep->isIO())  // e.g. !(output && output)
                           || findvarp->gotNansiType());  // e.g. int x && int x
                    स्थिर bool ansiBad
                        = findvarp->isAnsi() || nodep->isAnsi();  // dup illegal with ANSI
                    अगर (ansiBad || nansiBad) {
                        bool ansiWarn = ansiBad && !nansiBad;
                        अगर (ansiWarn) {
                            अटल पूर्णांक s_didAnsiWarn = false;
                            अगर (s_didAnsiWarn++) ansiWarn = false;
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
                    } अन्यथा {
                        findvarp->combineType(nodep);
                        findvarp->fileline()->modifyStateInherit(nodep->fileline());
                        findvarp->gotNansiType(true);
                        UASSERT_OBJ(nodep->subDTypep(), nodep, "Var has no type");
                        अगर (nodep->subDTypep()->numeric().isSigned()
                            && !findvarp->subDTypep()->numeric().isSigned()) {
                            findvarp->subDTypep()->numeric(VSigning{true});
                        }
                        Ast_गाँठ_DType* स्थिर varDtp = findvarp->subDTypep();
                        Ast_गाँठ_DType* स्थिर otherDtp = nodep->subDTypep();
                        अगर (varDtp
                            && (VN_IS(varDtp, BasicDType)
                                && VN_AS(varDtp, BasicDType)->implicit())) {
                            // Then have "input foo" and "real foo" so the
                            // dtype comes from the other side.
                            VL_DO_DANGLING(varDtp->unlinkFrBack()->delete_वृक्ष(), varDtp);
                            findvarp->childDTypep(otherDtp->unlinkFrBack());
                        } अन्यथा अगर (!m_statep->forScopeCreation() && otherDtp && varDtp
                                   && !(VN_IS(otherDtp, BasicDType)
                                        && VN_AS(otherDtp, BasicDType)->implicit())) {
                            // otherDtp and varDtp both non-nullptr and neither are implicit
                            // Can't compare dtypes now as might contain parameters,
                            // defer to V3Width
                            AstAttrOf* स्थिर newp
                                = नया AstAttrOf{otherDtp->fileline(), VAttrType::VAR_PORT_DTYPE,
                                                otherDtp->unlinkFrBack()};
                            findvarp->addAttrsp(newp);
                        }
                    }
                    VL_DO_DANGLING(nodep->unlinkFrBack()->delete_वृक्ष(), nodep);
                } अन्यथा {
                    // User can disable the message at either point
                    अगर (!(m_ftaskp && m_ftaskp->dpiImport())
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
                    म_लो = true;
                }
            }
            अगर (म_लो) {
                अगर (m_statep->forPrimary() && nodep->isGParam()
                    && VN_IS(m_modSymp->nodep(), Module)
                    && (m_statep->मूल_Entp()->nodep() == m_modSymp->parentp()->nodep())) {
                    // This is the toplevel module. Check for command line overwrites of parameters
                    // We first search if the parameter is overwritten and then replace it with a
                    // new value.
                    अगर (v3Global.opt.hasParameter(nodep->name())) {
                        स्थिर string svalue = v3Global.opt.parameter(nodep->name());
                        अगर (AstConst* स्थिर valuep
                            = AstConst::parseParamLiteral(nodep->fileline(), svalue)) {
                            UINFO(9, "       replace parameter " << nodep);
                            UINFO(9, "       with " << valuep);
                            अगर (nodep->valuep()) धकेलो_Deletep(nodep->valuep()->unlinkFrBack());
                            nodep->valuep(valuep);
                            // Do not warn ENUMVALUE when overriding an enum param from the cli.
                            nodep->fileline()->warnOff(V3ErrorCode::ENUMVALUE, true);
                        }
                    }
                }
                m_statep->जोड़ो_Sym(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
                अगर (m_statep->forPrimary() && nodep->isGParam()) {
                    ++m_param_अंक;
                    VSymEnt* स्थिर symp
                        = m_statep->जोड़ो_Sym(m_curSymp, "__paramNumber" + cvtToStr(m_param_अंक),
                                              nodep, m_classOrPackagep);
                    symp->exported(false);
                }
            }
        }
    }
    व्योम visit(AstTypedef* nodep) override {  // FindVisitor::
        UASSERT_OBJ(m_curSymp, nodep, "Typedef not under module/package/$unit");
        अगर (VN_IS(m_classOrPackagep, Class)) nodep->isUnderClass(true);
        iterateChildren(nodep);
        m_statep->जोड़ो_Sym(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
    }
    व्योम visit(AstTypedefFwd* nodep) override {  // FindVisitor::
        UASSERT_OBJ(m_curSymp, nodep, "Typedef not under module/package/$unit");
        iterateChildren(nodep);
        // No need to insert, only the real typedef matters, but need to track for errors
        nodep->user1p(m_curSymp);
    }
    व्योम visit(Ast_गाँठ_UOr_Struct_DType* nodep) override {  // FindVisitor::
        UASSERT_OBJ(m_curSymp, nodep, "Struct/union dtype not under module/package/$unit");
        VL_RESTORER(m_curSymp);
        m_curSymp = m_statep->जोड़ो_Block(m_curSymp, "__Vdtype" + cvtToStr(nodep->unique_अंक()),
                                          nodep, m_classOrPackagep);
        iterateChildren(nodep);
    }
    व्योम visit(AstMemberDType* nodep) override {  // FindVisitor::
        iterateChildren(nodep);
        m_statep->जोड़ो_Sym(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
    }
    व्योम visit(AstParamTypeDType* nodep) override {  // FindVisitor::
        UASSERT_OBJ(m_curSymp, nodep, "Parameter type not under module/package/$unit");

        // Replace missing param types with provided hierarchical type params.
        अगर (!m_hierParamsName.empty()) {
            अगर (स्थिर VSymEnt* स्थिर typedefEntp = m_curSymp->findIdFallback(m_hierParamsName)) {
                स्थिर AstModule* modp = VN_CAST(typedefEntp->nodep(), Module);

                क्रम (स्थिर Ast_गाँठ* stmtp = modp ? modp->stmtsp() : nullptr; stmtp;
                     stmtp = stmtp->nextp()) {
                    स्थिर AstTypedef* tdefp = VN_CAST(stmtp, Typedef);

                    अगर (tdefp && tdefp->name() == nodep->name() && m_statep->forPrimary()) {
                        UINFO(8, "Replacing type of" << nodep << "  -->with " << tdefp);
                        Ast_गाँठ_DType* स्थिर newDtp = tdefp->childDTypep();
                        Ast_गाँठ_DType* स्थिर oldDtp = nodep->childDTypep();

                        oldDtp->replaceWith(newDtp->clone_वृक्ष(false));
                        VL_DO_DANGLING(oldDtp->delete_वृक्ष(), oldDtp);
                    }
                }
            }
        }

        iterateChildren(nodep);
        m_statep->जोड़ो_Sym(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
        अगर (m_statep->forPrimary() && nodep->isGParam()) {
            ++m_param_अंक;
            VSymEnt* स्थिर symp = m_statep->जोड़ो_Sym(
                m_curSymp, "__paramNumber" + cvtToStr(m_param_अंक), nodep, m_classOrPackagep);
            symp->exported(false);
        }
    }
    व्योम visit(AstCFunc* nodep) override {  // FindVisitor::
        // For dotted resolution, ignore all AstVars under functions, otherwise shouldn't exist
        UASSERT_OBJ(!m_statep->forScopeCreation(), nodep, "No CFuncs expected in tree yet");
    }
    व्योम visit(AstEnumItem* nodep) override {  // FindVisitor::
        // EnumItem: Remember its name for later resolution
        iterateChildren(nodep);
        // Find under either a task or the module's vars
        स्थिर VSymEnt* foundp = m_curSymp->findIdFallback(nodep->name());
        अगर (!foundp && m_modSymp && nodep->name() == m_modSymp->nodep()->name()) {
            foundp = m_modSymp;  // Conflicts with modname?
        }
        AstEnumItem* स्थिर findvarp = foundp ? VN_CAST(foundp->nodep(), EnumItem) : nullptr;
        bool म_लो = false;
        अगर (!foundp) {
            म_लो = true;
        } अन्यथा अगर (findvarp != nodep) {
            UINFO(4, "DupVar: " << nodep << " ;; " << foundp);
            अगर (foundp->parentp() == m_curSymp  // Only when on same level
                && !foundp->imported()) {  // and not from package
                nodep->v3error("Duplicate declaration of enum value: "
                               << nodep->prettyName() << '\n'
                               << nodep->warnContextPrimary() << '\n'
                               << foundp->nodep()->warnOther()
                               << "... Location of original declaration\n"
                               << foundp->nodep()->warnContextSecondary());
            } अन्यथा {
                // User can disable the message at either point
                अगर (!nodep->fileline()->warnIsOff(V3ErrorCode::VARHIDDEN)
                    && !foundp->nodep()->fileline()->warnIsOff(V3ErrorCode::VARHIDDEN)) {
                    nodep->v3warn(VARHIDDEN,
                                  "Declaration of enum value hides declaration in upper scope: "
                                      << nodep->prettyName() << '\n'
                                      << nodep->warnContextPrimary() << '\n'
                                      << foundp->nodep()->warnOther()
                                      << "... Location of original declaration\n"
                                      << foundp->nodep()->warnContextSecondary());
                }
                म_लो = true;
            }
        }
        अगर (म_लो) m_statep->जोड़ो_Sym(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
    }
    व्योम visit(AstPackageImport* nodep) override {  // FindVisitor::
        UINFO(4, "  Link: " << nodep);
        अगर (!nodep->packagep()) वापस;  // Errored in V3LinkCells
        VSymEnt* स्थिर srcp = m_statep->get_गाँठ_Sym(nodep->packagep());
        अगर (nodep->name() == "*") {
            अगर (nodep->packagep() != v3Global.rootp()->stdPackagep()) {
                अगर (m_curSymp == m_statep->dunitEntp()) {
                    nodep->v3warn(IMPORTSTAR,
                                  "'import::*' in $unit scope may pollute global namespace");
                }
            }
        } अन्यथा अगर (!nodep->resolvedClassp()) {
            VSymEnt* स्थिर impp = srcp->findIdFlat(nodep->name());
            अगर (!impp) {
                nodep->v3error("Import object not found: " << nodep->prettyPkgNameQ());
            } अन्यथा अगर (nodep->resolvedClassp()) {
                UASSERT_OBJ(nodep->resolvedClassp()->classOrPackageSkipp() == impp->nodep(), nodep,
                            "Import should resolve to the same node");
            } अन्यथा अगर (AstClass* स्थिर classp = VN_CAST(impp->nodep(), Class)) {
                nodep->resolvedClassp(
                    नया AstClassOrPackageRef{nodep->fileline(), classp->name(), classp, nullptr});
            }
        }
        m_curSymp->importFromPackage(m_statep->symsp(), srcp, nodep->name());
        UINFO(9, "    Link Done: " << nodep);
        // No longer needed, but can't delete until any multi-instantiated modules are expanded
    }
    व्योम visit(AstPackageExport* nodep) override {  // FindVisitor::
        UINFO(9, "  Link: " << nodep);
        अगर (!nodep->packagep()) वापस;  // Errored in V3LinkCells
        VSymEnt* स्थिर srcp = m_statep->get_गाँठ_Sym(nodep->packagep());
        अगर (nodep->name() != "*") {
            VSymEnt* स्थिर impp = srcp->findIdFlat(nodep->name());
            अगर (!impp) {
                nodep->v3error("Export object not found: '" << nodep->packagep()->prettyName()
                                                            << "::" << nodep->prettyName() << "'");
            }
        }
        m_curSymp->exportFromPackage(m_statep->symsp(), srcp, nodep->name());
        UINFO(9, "    Link Done: " << nodep);
        // No longer needed, but can't delete until any multi-instantiated modules are expanded
    }
    व्योम visit(AstPackageExportStarStar* nodep) override {  // FindVisitor::
        UINFO(4, "  Link: " << nodep);
        m_curSymp->exportStarStar(m_statep->symsp());
        // No longer needed, but can't delete until any multi-instantiated modules are expanded
    }

    व्योम visit(Ast_गाँठ_Foreach* nodep) override {  // FindVisitor::
        // Symbol table needs nodep->name() as the index variable's name
        VL_RESTORER(m_curSymp);
        {
            ++m_mod_With_अंक;
            m_curSymp = m_statep->जोड़ो_Block(m_curSymp, "__Vforeach" + cvtToStr(m_mod_With_अंक),
                                              nodep, m_classOrPackagep);
            m_curSymp->fallbackp(VL_RESTORER_पूर्व(m_curSymp));
            क्रम (Ast_गाँठ *nextp, *argp = nodep->headerp()->elementsp(); argp; argp = nextp) {
                nextp = argp->nextp();
                AstVar* argrefp = nullptr;
                अगर (AstParseRef* स्थिर parserefp = VN_CAST(argp, ParseRef)) {
                    // IEEE 1800-2023 12.7.3: foreach loop variable type shall be int (2-state)
                    // This might get changed in V3Width when types resolve (e.g., for assoc
                    // arrays)
                    argrefp = नया AstVar{parserefp->fileline(), VVarType::BLOCKTEMP,
                                         parserefp->name(), argp->findIntDType()};
                    argrefp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                    parserefp->replaceWith(argrefp);
                    VL_DO_DANGLING2(parserefp->delete_वृक्ष(), parserefp, argp);
                    // Insert argref's name into symbol table
                    m_statep->जोड़ो_Sym(m_curSymp, argrefp->name(), argrefp, nullptr);
                } अन्यथा अगर (स्थिर स्वतः largrefp = VN_CAST(argp, Var)) {
                    argrefp = largrefp;
                    // Insert argref's name into symbol table
                    m_statep->जोड़ो_Sym(m_curSymp, argrefp->name(), argrefp, nullptr);
                } अन्यथा अगर (VN_IS(argp, Empty)) {
                } अन्यथा {
                    argp->v3error("'foreach' loop variable expects simple variable name");
                }
            }
            iterateChildren(nodep);
        }
    }

    व्योम visit(AstRandSequence* nodep) override {
        VL_RESTORER(m_curSymp);
        m_curSymp = m_statep->जोड़ो_Block(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
        iterateChildren(nodep);
    }
    व्योम visit(AstRSProd* nodep) override {
        VL_RESTORER(m_curSymp);
        m_curSymp = m_statep->जोड़ो_Block(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
        अगर (nodep->fvarp())
            nodep->fvarp()->v3warn(E_UNSUPPORTED,
                                   "Unsupported: randsequence production function variable");
        // Mark formal ports as port-checked so the primary resolve pass does not
        // flag them with "does not appear in port list" -- V3RandSequence will
        // later move them onto the generated task as real input ports.
        क्रम (Ast_गाँठ* itp = nodep->portsp(); itp; itp = itp->nextp()) {
            VN_AS(itp, Var)->user4(true);
        }
        iterateChildren(nodep);
    }

    व्योम visit(AstWithParse* nodep) override {  // FindVisitor::
        // Change WITHPARSE(FUNCREF, equation) to FUNCREF(WITH(equation))
        Ast_गाँठ_FTask_Ref* funcrefp = VN_CAST(nodep->funcrefp(), गाँठ_FTask_Ref);
        अगर (स्थिर AstDot* स्थिर dotp = VN_CAST(nodep->funcrefp(), Dot))
            funcrefp = VN_CAST(dotp->rhsp(), गाँठ_FTask_Ref);
        UASSERT_OBJ(funcrefp, nodep, "'with' only can operate on a function/task");
        string name = funcrefp->name() == "randomize" ? "__Vrandwith_obj" : "item";
        File_पंक्ति* argFl = nodep->fileline();
        AstArg* स्थिर argsp = funcrefp->argsp();
        अगर (argsp) {
            argsp->unlink_Fr_Back_With_अग्र();
            अगर (funcrefp->name() != "randomize") {
                अगर (स्थिर स्वतः parserefp = VN_CAST(argsp->exprp(), ParseRef)) {
                    name = parserefp->name();
                    argFl = parserefp->fileline();
                } अन्यथा {
                    argsp->v3error("'with' function expects simple variable name");
                }
                अगर (argsp->nextp()) {
                    argsp->nextp()->v3error("'with' function expects only up to one argument");
                }
                VL_DO_DANGLING(argsp->delete_वृक्ष(), argsp);
            }
        }
        // Type depends on the method used, let V3Width figure it out later
        // 'with (...) { ... }' constraint_block form is randomize-only
        // (IEEE 1800-2023 7.12 vs. 18.7); array methods take 'with (expr)'.
        अगर (nodep->restricted() && funcrefp->name() != "randomize") {
            nodep->v3error("'with (...) { ... }' constraint block is only valid for"
                           " randomize() (IEEE 1800-2023 18.7)");
            funcrefp->addArgsp(argsp);
            nodep->replaceWith(nodep->funcrefp()->unlinkFrBack());
            VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
            वापस;
        }
        स्थिर bool restrictedRandomize = nodep->restricted();
        अगर (nodep->exprsp() || nodep->constraintsp()
            || restrictedRandomize) {  // Else empty expression and pretend no "with"
            AstLambdaArgRef* स्थिर सूचकांक_Arg_Refp
                = नया AstLambdaArgRef{argFl, name + "__DOT__index", true};
            AstLambdaArgRef* स्थिर मूल्य_Arg_Refp = नया AstLambdaArgRef{argFl, name, false};
            AstWith* स्थिर newp
                = नया AstWith{nodep->fileline(), सूचकांक_Arg_Refp, मूल्य_Arg_Refp, nullptr};
            // Harvest the identifier_list into AstWith; grammar guarantees AstParseRef.
            अगर (restrictedRandomize) {
                newp->restricted(true);
                जबतक (Ast_गाँठ* स्थिर itemp = nodep->exprsp()) {
                    newp->addRestrictedName(VN_AS(itemp, ParseRef)->name());
                    itemp->unlinkFrBack();
                    VL_DO_DANGLING(धकेलो_Deletep(itemp), itemp);
                }
            }
            Ast_गाँठ* व्यंजक_Or_Constraintsp = nullptr;
            अगर (nodep->exprsp()) व्यंजक_Or_Constraintsp = nodep->exprsp()->unlink_Fr_Back_With_अग्र();
            अगर (nodep->constraintsp())
                व्यंजक_Or_Constraintsp = Ast_गाँठ::add_अग्र(
                    व्यंजक_Or_Constraintsp, nodep->constraintsp()->unlink_Fr_Back_With_अग्र());
            newp->addExprp(व्यंजक_Or_Constraintsp);  // addExprp() tolerates nullptr
            funcrefp->withp(newp);
        }
        funcrefp->addArgsp(argsp);
        nodep->replaceWith(nodep->funcrefp()->unlinkFrBack());
        VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
    }
    व्योम visit(AstWith* nodep) override {  // FindVisitor::
        // Symbol table needs nodep->name() as the index variable's name
        // Iteration will pickup the AstVar we made under AstWith
        VL_RESTORER(m_curSymp);
        ++m_mod_With_अंक;
        m_curSymp = m_statep->जोड़ो_Block(m_curSymp, "__Vwith" + cvtToStr(m_mod_With_अंक), nodep,
                                          m_classOrPackagep);
        m_curSymp->fallbackp(VL_RESTORER_पूर्व(m_curSymp));
        UASSERT_OBJ(nodep->सूचकांक_Arg_Refp(), nodep, "Missing lambda argref");
        UASSERT_OBJ(nodep->मूल्य_Arg_Refp(), nodep, "Missing lambda argref");
        // Insert argref's name into symbol table
        m_statep->जोड़ो_Sym(m_curSymp, nodep->मूल्य_Arg_Refp()->name(), nodep->मूल्य_Arg_Refp(),
                            nullptr);
        iterateChildren(nodep);
    }
    व्योम visit(AstCoverWith* nodep) override {  // FindVisitor::
        // The range list is outside the scope of the filter, which declares 'item'
        iterate_And_अग्र_Null(nodep->subp());
        VL_RESTORER(m_curSymp);
        ++m_mod_With_अंक;
        m_curSymp = m_statep->जोड़ो_Block(m_curSymp, "__VcoverWith" + cvtToStr(m_mod_With_अंक),
                                          nodep, m_classOrPackagep);
        m_curSymp->fallbackp(VL_RESTORER_पूर्व(m_curSymp));
        iterate_And_अग्र_Null(nodep->itemp());
        iterate_And_अग्र_Null(nodep->filterp());
    }
    व्योम visit(AstCoverCrossWith* nodep) override {  // FindVisitor::
        // The selection, the 'matches' count, and the cross items' types, of their coverpoints'
        // expressions with any nested scopes, are outside the filter's scope, which declares the
        // items.  As visit(AstVar) would resolve a type in the scope declaring its variable, the
        // items, generated and unique, are inserted directly.
        iterate_And_अग्र_Null(nodep->selectp());
        iterate_And_अग्र_Null(nodep->countp());
        क्रम (AstVar* varp = nodep->varsp(); varp; varp = VN_AS(varp->nextp(), Var)) {
            iterate_And_अग्र_Null(varp->childDTypep());
        }
        VL_RESTORER(m_curSymp);
        ++m_mod_With_अंक;
        m_curSymp = m_statep->जोड़ो_Block(m_curSymp, "__VcoverWith" + cvtToStr(m_mod_With_अंक),
                                          nodep, m_classOrPackagep);
        m_curSymp->fallbackp(VL_RESTORER_पूर्व(m_curSymp));
        क्रम (AstVar* varp = nodep->varsp(); varp; varp = VN_AS(varp->nextp(), Var)) {
            m_statep->जोड़ो_Sym(m_curSymp, varp->name(), varp, m_classOrPackagep);
        }
        iterate_And_अग्र_Null(nodep->filterp());
    }

    व्योम visit(Ast_गाँठ* nodep) override { iterateChildren(nodep); }  // FindVisitor::

    व्योम handleUnvisitedVirtIfaces() {
        Ast_गाँठ_Module* स्थिर शीर्ष = v3Global.rootp()->modulesp();
        m_curSymp = m_statep->get_गाँठ_Sym(शीर्ष);
        क्रम (AstIface* ifacep : m_virtIfaces) {
            अगर (!ifacep->user4()) {  // virtual interface never assigned any actual interface
                // Create dummy cell to keep the virtual interface alive
                // (later stages assume that non-dead interface has associated cell)
                AstCell* स्थिर ifacecellp = नया AstCell{ifacep->fileline(),
                                                        ifacep->fileline(),
                                                        "__VdummyInstOf" + ifacep->name(),
                                                        ifacep->name(),
                                                        nullptr,
                                                        nullptr,
                                                        nullptr};
                ifacecellp->modp(ifacep);
                शीर्ष->addStmtsp(ifacecellp);

                iterate(ifacecellp);
            }
        }
    }

खुला:
    // CONSTRUCTORS
    LinkDotFindVisitor(AstNetlist* rootp, LinkDotState* statep)
        : m_statep{statep} {
        UINFO(4, __FUNCTION__ << ": ");
        iterate(rootp);

        अगर (!m_virtIfaces.empty()) handleUnvisitedVirtIfaces();
        क्रम (AstScope* स्थिर scopep : m_scopes) scopep->modp()->addStmtsp(scopep);
        अगर (m_statep->forScopeCreation()) {
            V3Stats::addStat("LinkDot, Scope visits", m_statScopeVisits);
        }
    }
    ~LinkDotFindVisitor() override = शेष;
};

//======================================================================

श्रेणी LinkDotFindIfaceVisitor final : खुला VNVisitor {
    // NODE STATE
    //  *::user1p()             -> See LinkDotState

    // STATE - for current visit position (use VL_RESTORER)
    LinkDotState* स्थिर m_statep;  // State to pass between visitors, including symbol table
    Ast_गाँठ* m_declp = nullptr;  // Current declaring object that may soon contain IfaceRefDType

    // METHODS
    स्थिर VSymEnt* findNonDeclSym(VSymEnt* symp, स्थिर string& name) {
        // Find if there is a symbol of given name, ignoring the node that is declaring
        // it (m_declp) itself.  Thus if searching for "ifc" (an interface):
        //
        // module x;  // Finishes here and finds the typedef
        //   typedef foo ifc;
        //   ifc ifc;   // symp starts pointing here, but matches m_declp
        जबतक (symp) {
            स्थिर VSymEnt* स्थिर foundp = symp->findIdFlat(name);
            अगर (foundp && foundp->nodep() != m_declp) वापस foundp;
            symp = symp->fallbackp();
        }
        वापस nullptr;
    }

    // VISITORS
    व्योम visit(AstRefDType* nodep) override {  // FindIfaceVisitor::
        अगर (m_statep->forPrimary() && !nodep->classOrPackagep()) {
            UINFO(9, "  FindIfc: " << nodep);
            // If under a var, ignore the var itself as might be e.g. "intf intf;"
            // Critical tests:
            //   t_interface_param_genblk.v  // Checks this does make interface
            //   t_interface_hidden.v  // Checks this doesn't making interface when hidden
            अगर (m_statep->exists_गाँठ_Sym(nodep)) {
                VSymEnt* symp = m_statep->get_गाँठ_Sym(nodep);
                स्थिर VSymEnt* foundp = findNonDeclSym(symp, nodep->name());
                Ast_गाँठ* foundNodep = nullptr;
                // This: v4make test_regress/t/t_interface_param_genblk.py --debug
                // --debugi-V3LinkDot 9 Passes with this commented out:
                अगर (foundp) foundNodep = foundp->nodep();
                अगर (!foundNodep) foundNodep = m_statep->findModuleSym(nodep->name());
                अगर (foundNodep) UINFO(9, "    Ifc foundNodep " << foundNodep);
                अगर (AstIface* स्थिर defp = VN_CAST(foundNodep, Iface)) {
                    // Must be found as module name, and not hidden/ by normal symbol (foundp)
                    AstIfaceRefDType* स्थिर newp
                        = नया AstIfaceRefDType{nodep->fileline(), "", nodep->name()};
                    अगर (nodep->paramsp())
                        newp->addParamsp(nodep->paramsp()->unlink_Fr_Back_With_अग्र());
                    newp->ifacep(defp);
                    newp->user1u(nodep->user1u());
                    UINFO(9, "    Resolved interface " << nodep << "  =>  " << defp);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
                    वापस;
                }
            }
        }
        iterateChildren(nodep);
    }
    व्योम visit(AstVar* nodep) override {  // FindIfaceVisitor::
        VL_RESTORER(m_declp);
        m_declp = nodep;
        iterateChildren(nodep);
        AstIfaceRefDType* स्थिर ifacerefp = LinkDotState::ifaceRefFromArray(nodep->subDTypep());
        अगर (ifacerefp && m_statep->exists_गाँठ_Sym(nodep)) {
            // Can't resolve until interfaces and modport names are
            // known; see notes at top
            UINFO(9, "  FindIfc Var IfaceRef " << ifacerefp);
            अगर (!ifacerefp->isVirtual()) nodep->setIfaceRef();
            VSymEnt* स्थिर symp = m_statep->get_गाँठ_Sym(nodep);
            m_statep->जोड़ो_Iface_Var_Sym(symp);
        }
    }
    व्योम visit(Ast_गाँठ* nodep) override { iterateChildren(nodep); }  // FindIfaceVisitor::

खुला:
    // CONSTRUCTORS
    LinkDotFindIfaceVisitor(AstNetlist* rootp, LinkDotState* statep)
        : m_statep{statep} {
        UINFO(4, __FUNCTION__ << ": ");
        iterate(rootp);
    }
    ~LinkDotFindIfaceVisitor() override = शेष;
};

//======================================================================

श्रेणी LinkDotParamVisitor final : खुला VNVisitor {
    // NODE STATE
    //  *::user1p()             -> See LinkDotState
    //  *::user2p()             -> See LinkDotState
    //  *::user4()              -> See LinkDotState

    // STATE
    LinkDotState* स्थिर m_statep;  // State to pass between visitors, including symbol table
    Ast_गाँठ_Module* m_modp = nullptr;  // Current module

    व्योम pin_Implicit_व्यंजक_Recurse(Ast_गाँठ* nodep) {
        // Under a pin, Check interconnect expression for a pin reference or a concat.
        // Create implicit variable as needed
        अगर (VN_IS(nodep, Dot)) {  // Not creating a simple implied type,
            // and implying something else would just confuse later errors
        } अन्यथा अगर (VN_IS(nodep, VarRef) || VN_IS(nodep, ParseRef)) {
            // To prevent user errors, we should only do single bit
            // implicit vars, however some netlists (MIPS) expect single
            // bit implicit wires to get created with range 0:0 etc.
            m_statep->implicitOkAdd(m_modp, nodep->name());
        }
        // These are perhaps a little too generous, as a SELect of siga[sigb]
        // perhaps shouldn't create an implicit variable.  But, we've warned...
        अन्यथा {
            अगर (Ast_गाँठ* स्थिर refp = nodep->op1p()) pin_Implicit_व्यंजक_Recurse(refp);
            अगर (Ast_गाँठ* स्थिर refp = nodep->op2p()) pin_Implicit_व्यंजक_Recurse(refp);
            अगर (Ast_गाँठ* स्थिर refp = nodep->op3p()) pin_Implicit_व्यंजक_Recurse(refp);
            अगर (Ast_गाँठ* स्थिर refp = nodep->op4p()) pin_Implicit_व्यंजक_Recurse(refp);
        }
        // Continue along a list (e.g. the terminals under AstImplicit), also after a reference
        अगर (Ast_गाँठ* स्थिर refp = nodep->nextp()) pin_Implicit_व्यंजक_Recurse(refp);
    }

    // VISITORS
    व्योम visit(AstTypeTable*) override {}  // ParamVisitor::
    व्योम visit(Ast_गाँठ_Module* nodep) override {  // ParamVisitor::
        UINFO(5, "   " << nodep);
        अगर ((nodep->dead() || !nodep->user4()) && !nodep->hierParams()) {
            UINFO(4, "Mark dead module " << nodep);
            UASSERT_OBJ(!m_statep->forScopeCreation(), nodep,
                        "Dead module persisted past where should have removed");
            // Don't remove now, because we may have a tree of
            // parameterized modules with VARXREFs into the deleted module
            // region.  V3Dead should cleanup.
            // Downstream visitors up until V3Dead need to check for nodep->dead.
            nodep->dead(true);
            वापस;
        }
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
    }
    व्योम visit(AstPin* nodep) override {  // ParamVisitor::
        // Pin: Link to submodule's port
        // Deal with implicit definitions - do before Resolve visitor as may
        // be referenced above declaration
        अगर (nodep->exprp()  // Else specifically unconnected pin
            && !nodep->svImplicit()) {  // SV 19.11.3: .name pins don't allow implicit decls
            pin_Implicit_व्यंजक_Recurse(nodep->exprp());
        }
    }
    व्योम visit(AstDefParam* nodep) override {  // ParamVisitor::
        iterateChildren(nodep);
        nodep->v3warn(DEFPARAM, "defparam is deprecated (IEEE 1800-2023 C.4.1)\n"
                                    << nodep->warnMore()
                                    << "... Suggest use instantiation with #(."
                                    << nodep->prettyName() << "(...etc...))");
        bool hasPartSelect = false;
        स्थिर string path = extractDottedPath(nodep->pathp(), hasPartSelect);
        UASSERT_OBJ(!hasPartSelect && !path.empty(), nodep, "Unexpected defparam path shape");
        string baddot;
        VSymEnt* okSymp = nullptr;
        VSymEnt* स्थिर foundp = m_statep->findDotted(
            nodep->fileline(), m_statep->get_गाँठ_Sym(nodep), path, baddot, okSymp, true);
        AstCell* स्थिर cellp = foundp ? VN_AS(foundp->nodep(), Cell) : nullptr;
        अगर (!cellp) {
            nodep->v3error("In defparam, instance " << path << " never declared");
        } अन्यथा {
            Ast_गाँठ_व्यंजक* स्थिर exprp = nodep->rhsp()->unlinkFrBack();
            UINFO(9, "Defparam cell " << path << "." << nodep->name() << " attach-to " << cellp
                                      << "  <= " << exprp);
            // Don't need to check the name of the defparam exists.  V3Param does.
            AstPin* स्थिर pinp = नया AstPin{nodep->fileline(),
                                            -1,  // Pin# not relevant
                                            nodep->name(), exprp};
            pinp->param(true);
            pinp->paramPath(path);
            cellp->addParamsp(pinp);
        }
        VL_DO_DANGLING(nodep->unlinkFrBack()->delete_वृक्ष(), nodep);
    }
    व्योम visit(AstPort* nodep) override {  // ParamVisitor::
        // Port: Stash the pin number
        // Need to set pin numbers after varnames are created
        // But before we do the final resolution based on names
        VSymEnt* स्थिर foundp = m_statep->get_गाँठ_Sym(m_modp)->findIdFlat(nodep->name());
        AstVar* स्थिर refp = foundp ? VN_CAST(foundp->nodep(), Var) : nullptr;
        अगर (!foundp) {
            nodep->v3error(
                "Input/output/inout declaration not found for port: " << nodep->prettyNameQ());
        } अन्यथा अगर (!refp || (!refp->isIO() && !refp->isIfaceRef())) {
            nodep->v3error("Pin is not an in/out/inout/interface: " << nodep->prettyNameQ());
        } अन्यथा {
            अगर (refp->user4()) {
                nodep->v3error("Duplicate declaration of port: "
                               << nodep->prettyNameQ() << '\n'
                               << nodep->warnContextPrimary() << '\n'
                               << refp->warnOther() << "... Location of original declaration\n"
                               << refp->warnContextSecondary());
            }
            refp->user4(true);
            VSymEnt* स्थिर symp = m_statep->जोड़ो_Sym(m_statep->get_गाँठ_Sym(m_modp),
                                                      "__pinNumber" + cvtToStr(nodep->pin_अंक()),
                                                      refp, nullptr /*classOrPackagep*/);
            symp->exported(false);
            refp->pin_अंक(nodep->pin_अंक());
            // Put the variable where the port is, so that variables stay
            // in pin number sorted order. Otherwise hierarchical or JSON
            // may botch by-position instances.
            nodep->add_Here_This_As_अग्र(refp->unlinkFrBack());
        }
        // Ports not needed any more
        VL_DO_DANGLING(nodep->unlinkFrBack()->delete_वृक्ष(), nodep);
    }
    व्योम visit(AstAssignW* nodep) override {  // ParamVisitor::
        // Deal with implicit definitions
        // We used to nodep->allowImplicit() here, but it turns out
        // normal "assigns" can also make implicit wires.  Yuk.
        pin_Implicit_व्यंजक_Recurse(nodep->lhsp());
        iterateChildren(nodep);
    }

    व्योम visit(AstImplicit* nodep) override {  // ParamVisitor::
        // Unsupported gates need implicit creation
        pin_Implicit_व्यंजक_Recurse(nodep->exprsp());
        // We're done with implicit gates
        VL_DO_DANGLING(nodep->unlinkFrBack()->delete_वृक्ष(), nodep);
    }
    व्योम visit(AstClassOrPackageRef* nodep) override {  // ParamVisitor::
        अगर (स्वतः* स्थिर fwdp = VN_CAST(nodep->classOrPackageNodep(), TypedefFwd)) {
            // Relink forward definitions to the "real" definition
            VSymEnt* स्थिर foundp = m_statep->get_गाँठ_Sym(fwdp)->findIdFallback(fwdp->name());
            अगर (foundp && (VN_IS(foundp->nodep(), Class) || VN_IS(foundp->nodep(), Package))) {
                nodep->classOrPackagep(VN_AS(foundp->nodep(), गाँठ_Module));
            } अन्यथा अगर (foundp && VN_IS(foundp->nodep(), ParamTypeDType)) {
                UASSERT(m_statep->forPrimary(), "Param types should have been resolved");
                nodep->classOrPackageNodep(VN_AS(foundp->nodep(), ParamTypeDType));
            } अन्यथा {
                अगर (foundp) UINFO(1, "found nodep = " << foundp->nodep());
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
    व्योम visit(AstPull* nodep) override {  // ParamVisitor::
        // Deal with implicit definitions
        // We used to nodep->allowImplicit() here, but it turns out
        // normal "assigns" can also make implicit wires.  Yuk.
        pin_Implicit_व्यंजक_Recurse(nodep->lhsp());
        iterateChildren(nodep);
    }
    व्योम visit(AstTypedefFwd* nodep) override {  // ParamVisitor::
        VSymEnt* स्थिर foundp = m_statep->get_गाँठ_Sym(nodep)->findIdFallback(nodep->name());
        अगर (foundp) {
            // If the typedef was earlier in source order (tokenNum), then remember that earlier
            // point to avoid error something wasn't declared
            // Might be forward declaring something odd (with declTokenNumSetMin not implemented,
            // but should be harmless to ignore as this is just for error detection
            foundp->nodep()->decl_Token_अंक_Set_Min(nodep->fileline()->token_अंक());
        } अन्यथा अगर (v3Global.opt.pedantic()) {
            // We only check it was ever really defined in pedantic mode, as it
            // might have been in a header file referring to a module we never
            // needed so there are false positives
            nodep->v3error(
                "Forward typedef unused or does not resolve to a data type (IEEE 1800-2023 6.18): "
                << nodep->prettyNameQ());
        }
        // We only needed the forward declaration in order to parse correctly.
        // Delete later as may be ClassOrPackageRef's still pointing to it
        VL_DO_DANGLING(धकेलो_Deletep(nodep->unlinkFrBack()), nodep);
    }

    व्योम visit(Ast_गाँठ* nodep) override { iterateChildren(nodep); }  // ParamVisitor::

खुला:
    // CONSTRUCTORS
    LinkDotParamVisitor(AstNetlist* rootp, LinkDotState* statep)
        : m_statep{statep} {
        UINFO(4, __FUNCTION__ << ": ");
        iterate(rootp);
    }
    ~LinkDotParamVisitor() override = शेष;
};

//======================================================================

श्रेणी LinkDotScopeVisitor final : खुला VNVisitor {

    // STATE
    LinkDotState* स्थिर m_statep;  // State to pass between visitors, including symbol table
    स्थिर AstScope* m_scopep = nullptr;  // The current scope
    VSymEnt* m_modSymp = nullptr;  // Symbol entry for current module
    // Deferred AliasScope processing - must be done outer-to-inner for correct alias resolution
    std::vector<std::pair<AstAliasScope*, VSymEnt*>> m_deferredAliasScopes;

    // METHODS
खुला:
    // getAlias with setVarAlias and setIfaceAlias implement disjoint-set data structure.
    // This algorithm is needed for the case when multiple alias statements
    // reference partially the same variables.
    अटल AstVarScope* getAlias(AstVarScope* स्थिर vscp) {
        अगर (vscp->user2p() && vscp != vscp->user2p()) {
            AstVarScope* स्थिर aliasp = getAlias(VN_AS(vscp->user2p(), VarScope));
            vscp->user2p(aliasp);
            वापस aliasp;
        } अन्यथा {
            वापस vscp;
        }
    }

निजी:
    // Alias an interface reference to the interface it is connected to
    व्योम setIfaceAlias(AstVarScope* स्थिर vscp, AstVarScope* स्थिर aliasp) {
        UASSERT_OBJ(vscp->varp()->isIfaceRef(), vscp, "Interface alias of non-interface");
        UASSERT_OBJ(aliasp->varp()->isIfaceRef(), aliasp, "Interface alias of non-interface");
        getAlias(vscp)->user2p(getAlias(aliasp));
    }

    // Alias two variables. An output port in the lower scope survives, unless the other
    // must keep its own storage.
    व्योम setVarAlias(AstVarScope* ap, AstVarScope* bp) {
        UASSERT_OBJ(!ap->varp()->isIfaceRef(), ap, "Variable alias of interface");
        UASSERT_OBJ(!bp->varp()->isIfaceRef(), bp, "Variable alias of interface");
        ap = getAlias(ap);
        bp = getAlias(bp);
        अगर (ap == bp) वापस;
        स्थिर bool aWins = [&]() {
            स्थिर AstVar* स्थिर aVarp = ap->varp();
            स्थिर AstVar* स्थिर bVarp = bp->varp();
            // A primary IO is accessed directly by the user of the model, so must survive
            UASSERT_OBJ(!aVarp->isPrimaryIO() || !bVarp->isPrimaryIO(), ap,
                        "Alias of two primary IOs");
            अगर (bVarp->isPrimaryIO()) वापस false;
            अगर (aVarp->isPrimaryIO()) वापस true;
            // A forced or public variable must keep its own storage. If both are, either works.
            अगर (aVarp->isForced() != bVarp->isForced()) वापस aVarp->isForced();
            अगर (aVarp->isSigPublic() != bVarp->isSigPublic()) वापस aVarp->isSigPublic();
            // Otherwise prefer an output port in the lower scope. This enables better V3Combine
            // due to having fewer upward hierarchical references (which prevent combining).
            अगर (aVarp->direction() != VDirection::OUTPUT) वापस false;
            // Is 'ap' strictly below 'bp' in the hierarchy
            क्रम (स्थिर AstScope* scopep = ap->scopep()->aboveScopep(); scopep;
                 scopep = scopep->aboveScopep()) {
                अगर (scopep == bp->scopep()) वापस true;
            }
            वापस false;
        }();
        अगर (aWins) {
            bp->user2p(ap);
        } अन्यथा {
            ap->user2p(bp);
        }
    }

    // VISITORS
    व्योम visit(AstNetlist* nodep) override {  // ScopeVisitor::
        // Recurse..., backward as must do packages before using packages
        iterateChildrenBackwardsConst(nodep);
    }
    व्योम visit(AstScope* nodep) override {  // ScopeVisitor::
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
        अगर (AstIface* स्थिर ifacep = VN_CAST(nodep->modp(), Iface)) {
            m_statep->जोड़ो_Iface_Mod_Sym(ifacep, m_modSymp);
        }
        m_scopep = nodep;
        iterateChildren(nodep);
    }
    व्योम visit(AstVarScope* nodep) override {  // ScopeVisitor::
        अगर (!nodep->varp()->isFuncLocal() && !nodep->varp()->isClassMember()) {
            VSymEnt* स्थिर varSymp
                = m_statep->जोड़ो_Sym(m_modSymp, nodep->varp()->name(), nodep, nullptr);
            अगर (nodep->varp()->isIfaceRef()) {
                AstIfaceRefDType* स्थिर dtypep
                    = LinkDotState::ifaceRefFromArray(nodep->varp()->dtypep());
                UASSERT_OBJ(dtypep, nodep, "Non AstIfaceRefDType on isIfaceRef() var");
                अगर (nodep->varp()->isIfaceParent()) {
                    UINFO(9, "Iface parent ref var " << nodep->varp()->name() << " " << nodep);
                    // Find the interface cell the var references
                    UINFO(9, "Iface parent dtype " << dtypep);
                    स्थिर string ifcellname = dtypep->cellName();
                    string baddot;
                    VSymEnt* okSymp;
                    VSymEnt* cellSymp = m_statep->findDotted(nodep->fileline(), m_modSymp,
                                                             ifcellname, baddot, okSymp, false);
                    UASSERT_OBJ(
                        cellSymp, nodep,
                        "No symbol for interface instance: " << nodep->prettyNameQ(ifcellname));
                    UINFO(5, "       Found interface instance: se" << cvtToHex(cellSymp) << " "
                                                                   << cellSymp->nodep());
                    अगर (dtypep->modportName() != "") {
                        // Look up the modport within the interface cell's symbol table
                        VSymEnt* स्थिर mpSymp = cellSymp->findIdFallback(dtypep->modportName());
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
                    m_statep->जोड़ो_Scope_Alias(LinkDotState::SAMN_IFTOP, varSymp, cellSymp);
                }
            }
        }
    }
    व्योम visit(Ast_गाँठ_FTask* nodep) override {  // ScopeVisitor::
        VSymEnt* स्थिर symp = m_statep->जोड़ो_Block(m_modSymp, nodep->name(), nodep, nullptr);
        symp->fallbackp(m_modSymp);
        iterateChildren(nodep);
    }
    व्योम visit(Ast_गाँठ_Foreach* nodep) override {  // ScopeVisitor::
        VSymEnt* स्थिर symp = m_statep->जोड़ो_Block(m_modSymp, nodep->name(), nodep, nullptr);
        symp->fallbackp(m_modSymp);
        // No recursion, we don't want to pick up variables
    }
    व्योम visit(AstConstraintForeach* nodep) override {  // ScopeVisitor::
        iterateChildren(nodep);
    }
    व्योम visit(AstWith* nodep) override {  // ScopeVisitor::
        VSymEnt* स्थिर symp = m_statep->जोड़ो_Block(m_modSymp, nodep->name(), nodep, nullptr);
        symp->fallbackp(m_modSymp);
        iterateChildren(nodep);
    }
    व्योम visit(AstAlias* nodep) override {  // ScopeVisitor::
        // Track aliases
        UINFOTREE(9, nodep, "", "alias");
        AstVarScope* aliasVscp = nullptr;
        क्रम (Ast_गाँठ* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
            AstVarScope* vscp = nullptr;
            अगर (स्थिर AstVarRef* स्थिर refp = VN_CAST(itemp, VarRef)) {
                vscp = refp->varScopep();
            } अन्यथा {
                // Reaches into the scope of an instance, look it up by name
                स्थिर AstVarXRef* स्थिर xrefp = VN_AS(itemp, VarXRef);
                स्थिर string scopename = xrefp->dotted() + "." + xrefp->name();
                string baddot;
                VSymEnt* okSymp;
                VSymEnt* स्थिर symp = m_statep->findDotted(xrefp->fileline(), m_modSymp, scopename,
                                                           baddot, okSymp, false);
                UASSERT_OBJ(symp, nodep, "No symbol for alias item: " << scopename);
                vscp = VN_CAST(symp->nodep(), VarScope);
            }
            UASSERT_OBJ(vscp, nodep, "VarScope unset");
            अगर (aliasVscp) {
                setVarAlias(aliasVscp, vscp);
            } अन्यथा {
                aliasVscp = vscp;
            }
        }
        iterateChildren(nodep);
        VL_DO_DANGLING(धकेलो_Deletep(nodep->unlinkFrBack()), nodep);
    }
    व्योम visit(AstAliasScope* nodep) override {  // ScopeVisitor::
        // Defer AliasScope processing - must process outer scopes before inner ones
        // so that nested interface port alias resolution works correctly
        UINFO(5, "ALIASSCOPE (deferred) " << nodep);
        m_deferredAliasScopes.emplace_back(nodep, m_modSymp);
    }
    व्योम processAliasScope(AstAliasScope* nodep, VSymEnt* modSymp) {
        UINFO(5, "ALIASSCOPE  " << nodep);
        UINFOTREE(9, nodep, "", "avs");
        VSymEnt* rhsSymp;
        {
            AstVarRef* स्थिर refp = VN_CAST(nodep->rhsp(), VarRef);
            AstVarXRef* स्थिर xrefp = VN_CAST(nodep->rhsp(), VarXRef);
            UASSERT_OBJ(refp || xrefp, nodep,
                        "Unsupported: Non Var(X)Ref attached to interface pin");
            string inl
                = ((xrefp && xrefp->inlinedDots().आकार()) ? (xrefp->inlinedDots() + "__DOT__")
                                                          : "");
            स्थिर string dottedPath
                = (xrefp && !xrefp->dotted().empty()) ? (xrefp->dotted() + ".") : "";
            VSymEnt* symp = nullptr;
            string scopename;
            जबतक (!symp) {
                scopename = refp ? refp->name()
                                 : (inl.आकार() ? (inl + dottedPath + xrefp->name())
                                               : (dottedPath + xrefp->name()));
                string baddot;
                VSymEnt* okSymp;
                symp = m_statep->findDotted(nodep->rhsp()->fileline(), modSymp, scopename, baddot,
                                            okSymp, true);
                अगर (inl == "") अवरोध;
                inl = LinkDotState::removeLastInlineScope(inl);
            }
            UASSERT_OBJ(symp, nodep, "No symbol for interface alias rhs");
            UINFO(5, "       Found a linked scope RHS: " << scopename << "  se" << cvtToHex(symp)
                                                         << " " << symp->nodep());
            rhsSymp = symp;
        }
        VSymEnt* lhsSymp;
        {
            स्थिर AstVarXRef* स्थिर xrefp = VN_CAST(nodep->lhsp(), VarXRef);
            स्थिर AstVarRef* स्थिर refp = VN_CAST(nodep->lhsp(), VarRef);

            UASSERT_OBJ(refp || xrefp, nodep,
                        "Unsupported: Non Var(X)Ref attached to interface pin");
            स्थिर string scopename
                = refp ? refp->varp()->name() : xrefp->dotted() + "." + xrefp->name();
            string baddot;
            VSymEnt* okSymp;
            VSymEnt* स्थिर symp = m_statep->findDotted(nodep->lhsp()->fileline(), modSymp,
                                                       scopename, baddot, okSymp, false);
            UASSERT_OBJ(symp, nodep, "No symbol for interface alias lhs");
            UINFO(5, "       Found a linked scope LHS: " << scopename << "  se" << cvtToHex(symp)
                                                         << " " << symp->nodep());
            lhsSymp = symp;
        }
        // Remember the alias - can't do it yet because we may have additional symbols to be added,
        // or maybe an alias of an alias
        m_statep->जोड़ो_Scope_Alias(LinkDotState::SAMN_IFTOP, lhsSymp, rhsSymp);
        AstVarScope* स्थिर lhsVscp = VN_CAST(lhsSymp->nodep(), VarScope);
        AstVarScope* स्थिर rhsVscp = VN_CAST(rhsSymp->nodep(), VarScope);
        UASSERT_OBJ(lhsVscp && rhsVscp, nodep, "Interface alias missing variable scope");
        setIfaceAlias(lhsVscp, rhsVscp);
        // We have stored the link, we don't need these any more
        VL_DO_DANGLING(nodep->unlinkFrBack()->delete_वृक्ष(), nodep);
    }
    व्योम processDeferredAliasScopes() {
        // Sort by hierarchy depth (shallower first) so outer aliases are resolved before inner
        // Pre-compute depth map to avoid O(N log N * D) complexity in sort comparisons
        std::unordered_map<VSymEnt*, पूर्णांक> depthMap;
        क्रम (स्थिर स्वतः& pair : m_deferredAliasScopes) {
            VSymEnt* स्थिर symp = pair.second;
            अगर (depthMap.find(symp) == depthMap.end()) {
                पूर्णांक depth = 0;
                क्रम (VSymEnt* p = symp; p; p = p->parentp()) ++depth;
                depthMap[symp] = depth;
            }
        }
        std::stable_sort(m_deferredAliasScopes.begin(), m_deferredAliasScopes.end(),
                         [&depthMap](स्थिर std::pair<AstAliasScope*, VSymEnt*>& a,
                                     स्थिर std::pair<AstAliasScope*, VSymEnt*>& b) {
                             वापस depthMap.at(a.second) < depthMap.at(b.second);
                         });
        // Process in sorted order
        क्रम (स्वतः& pair : m_deferredAliasScopes) { processAliasScope(pair.first, pair.second); }
        m_deferredAliasScopes.सफा();
    }
    व्योम visit(Ast_गाँठ_Gen* nodep) override {  // ScopeVisitor::  // LCOV_EXCL_LINE
        nodep->v3fatalSrc("Generate constructs should have been reduced out");
    }
    // For speed, don't recurse things that can't have scope
    // Note we allow AstNodeStmt's as generates may be under them
    व्योम visit(AstCell*) override {}  // ScopeVisitor::
    व्योम visit(AstVar*) override {}  // ScopeVisitor::
    व्योम visit(Ast_गाँठ* nodep) override { iterateChildren(nodep); }  // ScopeVisitor::

खुला:
    // CONSTRUCTORS
    LinkDotScopeVisitor(AstNetlist* rootp, LinkDotState* statep)
        : m_statep{statep} {
        UINFO(4, __FUNCTION__ << ": ");
        iterate(rootp);
        // Process deferred AliasScopes in outer-to-inner order
        processDeferredAliasScopes();
    }
    ~LinkDotScopeVisitor() override = शेष;
};

//======================================================================

// Iterate an interface to resolve modports
श्रेणी LinkDotIfaceVisitor final : खुला VNVisitor {
    // STATE
    LinkDotState* स्थिर m_statep;  // State to pass between visitors, including symbol table
    VSymEnt* m_curSymp;  // Symbol Entry for current table, where to lookup/insert

    // VISITORS
    व्योम visit(AstModport* nodep) override {  // IfaceVisitor::
        // Modport: Remember its name for later resolution
        UINFO(5, "   fiv: " << nodep);
        VL_RESTORER(m_curSymp);
        {
            // Create symbol table for the vars
            m_curSymp = m_statep->जोड़ो_Block(m_curSymp, nodep->name(), nodep, nullptr);
            m_curSymp->fallbackp(VL_RESTORER_पूर्व(m_curSymp));
            iterateChildren(nodep);
        }
    }
    व्योम visit(AstModportFTaskRef* nodep) override {  // IfaceVisitor::
        UINFO(5, "   fif: " << nodep);
        iterateChildren(nodep);
        VSymEnt* स्थिर symp = m_curSymp->findIdFallback(nodep->name());
        अगर (!symp) {
            nodep->v3error("Modport item not found: " << nodep->prettyNameQ());
        } अन्यथा अगर (Ast_गाँठ_FTask* स्थिर ftaskp = VN_CAST(symp->nodep(), गाँठ_FTask)) {
            // Make symbol under modport that points at the _interface_'s var, not the modport.
            nodep->ftaskp(ftaskp);
            VSymEnt* स्थिर subSymp
                = m_statep->जोड़ो_Sym(m_curSymp, nodep->name(), ftaskp, nullptr /*package*/);
            m_statep->जोड़ो_Scope_Alias(LinkDotState::SAMN_MODPORT, subSymp, symp);
        } अन्यथा {
            nodep->v3error("Modport item is not a function/task: " << nodep->prettyNameQ());
        }
        अगर (m_statep->forScopeCreation()) {
            // Done with AstModportFTaskRef.
            // Delete to prevent problems if we dead-delete pointed to ftask
            nodep->unlinkFrBack();
            VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
        }
    }

    // Helper to resolve remaining path through a nested interface
    // When findDotted() partially matches (okSymp set, baddot non-empty),
    // this follows the interface type to resolve the remaining path.
    // Returns the resolved symbol, or nullptr if not found.
    // On success, clears baddot; on partial match of multi-level path, updates baddot.
    VSymEnt* resolveNestedInterfacePath(File_पंक्ति* fl, VSymEnt* okSymp, string& baddot) {
        अगर (!okSymp || baddot.empty()) वापस nullptr;

        अटल constexpr पूर्णांक MAX_NESTING_DEPTH = 64;
        VSymEnt* curOkSymp = okSymp;

        क्रम (पूर्णांक depth = 0; depth < MAX_NESTING_DEPTH; ++depth) {
            // Try to get interface from the partially-matched symbol
            AstIface* ifacep = nullptr;
            अगर (स्थिर AstCell* स्थिर cellp = VN_CAST(curOkSymp->nodep(), Cell)) {
                ifacep = VN_CAST(cellp->modp(), Iface);
            } अन्यथा अगर (स्थिर AstVar* स्थिर varp = VN_CAST(curOkSymp->nodep(), Var)) {
                अगर (varp->isIfaceRef()) {
                    अगर (स्थिर AstIfaceRefDType* स्थिर ifaceRefp
                        = LinkDotState::ifaceRefFromArray(varp->dtypep())) {
                        ifacep = ifaceRefp->ifaceViaCellp();
                    }
                }
            }

            अगर (!ifacep || !m_statep->exists_गाँठ_Sym(ifacep)) वापस nullptr;

            VSymEnt* स्थिर ifaceSymp = m_statep->get_गाँठ_Sym(ifacep);

            अगर (baddot.find('.') == string::npos) {
                // Simple identifier - direct lookup
                VSymEnt* स्थिर symp = ifaceSymp->findIdFallback(baddot);
                अगर (symp) baddot.सफा();
                वापस symp;
            }

            // Multi-level path - use findDotted for partial resolution
            string remainingBaddot;
            VSymEnt* remainingOkSymp = nullptr;
            VSymEnt* स्थिर symp = m_statep->findDotted(fl, ifaceSymp, baddot, remainingBaddot,
                                                       remainingOkSymp, true);
            अगर (symp) {
                baddot = remainingBaddot;
                वापस symp;
            }

            // findDotted partially matched - check progress
            अगर (remainingBaddot == baddot || !remainingOkSymp) वापस nullptr;

            // Continue resolving with updated state
            baddot = remainingBaddot;
            curOkSymp = remainingOkSymp;
        }

        UINFO(1, "Nested interface resolution depth limit exceeded at " << fl);
        वापस nullptr;
    }

    // Resolve a modport expression to find the referenced symbol
    VSymEnt* resolveModportExpression(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* exprp) {
        UINFO(5, "     resolveModportExpression: " << exprp);
        VSymEnt* symp = nullptr;
        अगर (AstParseRef* स्थिर refp = VN_CAST(exprp, ParseRef)) {
            // Simple variable reference: modport mp(input .a(sig_a))
            symp = m_curSymp->findIdFallback(refp->name());
        } अन्यथा अगर (AstVarRef* स्थिर refp = VN_CAST(exprp, VarRef)) {
            // Already resolved VarRef (can happen if iterateChildren resolved it)
            अगर (refp->varScopep()) {
                // During scope creation, VarRef may have VarScope already linked
                symp = m_statep->get_गाँठ_Sym(refp->varScopep());
            } अन्यथा अगर (refp->varp()) {
                symp = m_curSymp->findIdFallback(refp->varp()->name());
            }
        } अन्यथा अगर (AstVarXRef* स्थिर refp = VN_CAST(exprp, VarXRef)) {
            // Resolved VarXRef (dotted reference resolved by earlier pass)
            अगर (refp->varp()) {
                // For nested interfaces, the var is in a different scope
                // First try to find the symbol via the var itself
                अगर (m_statep->exists_गाँठ_Sym(refp->varp())) {
                    symp = m_statep->get_गाँठ_Sym(refp->varp());
                } अन्यथा {
                    // Fallback: look up in current scope
                    symp = m_curSymp->findIdFallback(refp->varp()->name());
                }
            } अन्यथा अगर (!refp->dotted().empty()) {
                // varp not set yet - use dotted path to find the symbol
                // The dotted part (e.g., "base") is the interface cell, and name is the member
                स्थिर string fullPath = refp->dotted() + "." + refp->name();
                string baddot;
                VSymEnt* okSymp = nullptr;
                symp = m_statep->findDotted(fl, m_curSymp, fullPath, baddot, okSymp, true);
                // Handle nested interface path when findDotted had partial match
                अगर (okSymp && !baddot.empty()) {
                    VSymEnt* स्थिर resolved = resolveNestedInterfacePath(fl, okSymp, baddot);
                    अगर (resolved) symp = resolved;
                }
            }
        } अन्यथा अगर (AstDot* स्थिर dotp = VN_CAST(exprp, Dot)) {
            // Dotted path: modport mp(input .a(inner.sig))
            bool hasPartSelect = false;
            स्थिर string dottedPath = extractDottedPath(dotp, hasPartSelect);
            अगर (hasPartSelect) {
                fl->v3warn(
                    E_UNSUPPORTED,
                    "Unsupported: Modport expression with part select (IEEE 1800-2023 25.5.4)");
            } अन्यथा {
                string baddot;
                VSymEnt* okSymp = nullptr;
                symp = m_statep->findDotted(fl, m_curSymp, dottedPath, baddot, okSymp, true);

                // Handle nested interface path when findDotted had partial match
                अगर (okSymp && !baddot.empty()) {
                    VSymEnt* स्थिर resolved = resolveNestedInterfacePath(fl, okSymp, baddot);
                    अगर (resolved) symp = resolved;
                }

                अगर (!symp || !baddot.empty()) {
                    fl->v3error("Can't find modport expression target: "
                                << Ast_गाँठ::prettyNameQ(dottedPath));
                    symp = nullptr;
                }
            }
        } अन्यथा अगर (VN_IS(exprp, SelBit) || VN_IS(exprp, SelExtract)) {
            // Part select expressions not yet supported
            fl->v3warn(E_UNSUPPORTED,
                       "Unsupported: Modport expression with part select (IEEE 1800-2023 25.5.4)");
        } अन्यथा {
            // Other expression types not supported
            fl->v3warn(E_UNSUPPORTED,
                       "Unsupported: Complex modport expression (IEEE 1800-2023 25.5.4)");
        }
        वापस symp;
    }

    व्योम visit(AstModportVarRef* nodep) override {  // IfaceVisitor::
        UINFO(5, "   fiv: " << nodep);
        iterateChildren(nodep);
        VSymEnt* symp = nullptr;
        अगर (nodep->exprp()) {
            // Modport expression syntax: modport mp(input .port_name(expression))
            symp = resolveModportExpression(nodep->fileline(), nodep->exprp());
        } अन्यथा {
            symp = m_curSymp->findIdFallback(nodep->name());
        }
        अगर (!symp) {
            nodep->v3error("Modport item not found: " << nodep->prettyNameQ());
        } अन्यथा अगर (AstVar* स्थिर varp = VN_CAST(symp->nodep(), Var)) {
            // Make symbol under modport that points at the _interface_'s var via the modport.
            // (Need modport still to test input/output markings)
            nodep->varp(varp);
            अगर (nodep->exprp()) {
                // For modport expressions, insert symbol pointing to the underlying var (not the
                // ModportVarRef which will be deleted during scope creation). The virtual port
                // name maps to the real signal's var.
                VSymEnt* स्थिर subSymp
                    = m_statep->जोड़ो_Sym(m_curSymp, nodep->name(), varp, nullptr /*package*/);
                m_statep->जोड़ो_Scope_Alias(LinkDotState::SAMN_MODPORT, subSymp, symp);
            } अन्यथा {
                // For regular modport items, insert symbol pointing to ModportVarRef for
                // input/output marking tests.
                m_statep->जोड़ो_Sym(m_curSymp, nodep->name(), nodep, nullptr /*package*/);
            }
        } अन्यथा अगर (AstVarScope* स्थिर vscp = VN_CAST(symp->nodep(), VarScope)) {
            // Make symbol under modport that points at the _interface_'s var, not the modport.
            nodep->varp(vscp->varp());
            // Only insert symbol for modport expression virtual ports (exprp set).
            // For regular modport items, don't insert into the shared modport table because
            // each instance would overwrite the previous one, causing wrong VarScope lookups.
            // Regular items are found via the modport's fallback to the interface.
            अगर (nodep->exprp()) {
                VSymEnt* स्थिर subSymp
                    = m_statep->जोड़ो_Sym(m_curSymp, nodep->name(), vscp, nullptr /*package*/);
                m_statep->जोड़ो_Scope_Alias(LinkDotState::SAMN_MODPORT, subSymp, symp);
            }
        } अन्यथा {
            nodep->v3error("Modport item is not a variable: " << nodep->prettyNameQ());
        }
        अगर (m_statep->forScopeCreation()) {
            // Done with AstModportVarRef.
            // Delete to prevent problems if we dead-delete pointed to variable
            nodep->unlinkFrBack();
            VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
        }
    }
    व्योम visit(AstModportClockingRef* nodep) override {  // IfaceVisitor::
        UINFO(5, "   fic: " << nodep);
        iterateChildren(nodep);
        VSymEnt* स्थिर symp = m_curSymp->findIdFallback(nodep->name());
        अगर (!symp) {
            nodep->v3error("Modport item not found: " << nodep->prettyNameQ());
        } अन्यथा अगर (AstClocking* स्थिर clockingp = VN_CAST(symp->nodep(), Clocking)) {
            nodep->clockingp(clockingp);
            m_statep->जोड़ो_Sym(m_curSymp, nodep->name(), nodep, nullptr /*package*/);
        } अन्यथा {
            nodep->v3error("Modport item is not a clocking block: " << nodep->prettyNameQ());
        }
    }
    व्योम visit(AstScope* nodep) override {  // IfaceVisitor::
        अगर (nodep->user3SetOnce()) वापस;
        iterateChildren(nodep);
    }
    व्योम visit(Ast_गाँठ* nodep) override { iterateChildren(nodep); }  // IfaceVisitor::

खुला:
    // CONSTRUCTORS
    LinkDotIfaceVisitor(AstIface* nodep, VSymEnt* curSymp, LinkDotState* statep)
        : m_statep{statep}
        , m_curSymp{curSymp} {
        UINFO(4, __FUNCTION__ << ": ");
        iterate(nodep);
    }
    ~LinkDotIfaceVisitor() override = शेष;
};

व्योम LinkDotState::computeIfaceModSyms() {
    //  AstScope::user3()  // bool. Already iterated by a LinkDotIfaceVisitor
    स्थिर VNUser3InUse user3InUse;
    क्रम (स्थिर स्वतः& itr : m_ifaceModSyms) {
        AstIface* स्थिर nodep = itr.first;
        VSymEnt* स्थिर symp = itr.second;
        LinkDotIfaceVisitor{nodep, symp, यह};
    }
    m_ifaceModSyms.सफा();
}

//======================================================================

श्रेणी LinkDotResolveVisitor final : खुला VNVisitor {
    // NODE STATE
    // Cleared on global
    //  *::user1p()             -> See LinkDotState
    //  *::user2p()             -> See LinkDotState
    //  *::user3()              // bool. Processed
    //  *::user4()              -> See LinkDotState
    स्थिर VNUser3InUse m_inuser3;

    // TYPES
    क्रमागत DotPosition : uint8_t {
        // Must match ascii() method below
        DP_NONE = 0,  // Not under a DOT
        DP_PACKAGE,  // {package-or-class}:: DOT
        DP_FIRST,  // {scope-or-var} DOT
        DP_SCOPE,  // DOT... {scope-or-var} DOT
        DP_FINAL,  // [DOT...] {var-or-func-or-dtype} with no following dots
        DP_MEMBER  // DOT {member-name} [DOT...]
    };

    // STATE
    LinkDotState* स्थिर m_statep;  // State, including dotted symbol table
    VSymEnt* m_curSymp = nullptr;  // SymEnt for current lookup point
    VSymEnt* m_modSymp = nullptr;  // SymEnt for current module
    VSymEnt* m_pinSymp = nullptr;  // SymEnt for pin lookups
    VSymEnt* m_randSymp = nullptr;  // SymEnt for randomize target class's lookups
    स्थिर AstCell* m_cellp = nullptr;  // Current cell
    Ast_गाँठ_Module* m_modp = nullptr;  // Current module
    Ast_गाँठ_FTask* m_ftaskp = nullptr;  // Current function/task
    AstMethodCall* m_randMethodCallp = nullptr;  // Current randomize() method call
    पूर्णांक m_modport_अंक = 0;  // Uniqueify modport numbers
    पूर्णांक m_indent = 0;  // Indentation (tree depth) for debug
    bool m_inSens = false;  // True if in senitem
    स्थिर AstWith* m_currentWithp = nullptr;  // Enclosing 'with' (nullptr if none)
    std::set<std::string>
        m_restrictedNamesUsed;  // Names from current 'with (id_list)' that resolved into target
    bool m_genericIfaceModule = false;  // True if in module containing generic interface
    std::map<std::string, Ast_गाँठ*> m_ifClassImpNames;  // Names imported from interface class
    // Inherited interface implementations needing a forward in the current class, by name
    std::map<std::string, स्थिर VSymEnt*> m_ifForwards;
    std::set<AstClass*> m_extendsParam;  // Classes that have a parameterized super class
                                         // (except the default instances)
                                         // They are added to the set only in linkDotPrimary.
    bool m_insideClassExtParam = false;  // Inside a class from m_extendsParam
    AstNew* m_explicitSuperNewp = nullptr;  // Hit a "super.new" call inside a "new" function
    std::map<Ast_गाँठ*, AstPin*> m_usedPins;  // Pin used in this cell, map to duplicate
    std::map<Ast_गाँठ*, std::map<std::string, AstPin*>> m_usedDefParamPins;
    // Defparam pins used for a given formal, by hierarchical path
    std::map<std::string, Ast_गाँठ_Module*> m_modulesToRevisit;  // Modules to revisit a second time
    Ast_गाँठ* m_lastDeferredp = nullptr;  // Last node which requested a revisit of its module
    Ast_गाँठ_DType* m_packedArrayDtp = nullptr;  // Datatype reference for packed array
    bool m_inPackedArray = false;  // Currently traversing a packed array tree
    bool m_replaceWithAlias
        = true;  // Replace VarScope with an alias. Used in the handling of AstAlias
    bool m_isParam = false;  // Specifies whether currently visiting param variable
    bool m_resolvingTypedef = false;  // Currently traversing a Typedef tree

    काष्ठा DotStates final {
        DotPosition m_dotPos;  // Scope part of dotted resolution
        VSymEnt* m_dotSymp;  // SymEnt for dotted AstParse lookup
        स्थिर AstDot* m_dotp;  // Current dot
        bool m_super;  // Starts with super reference
        bool m_unresolvedCell;  // Unresolved cell, needs help from V3Param
        bool m_unresolvedClass;  // Unresolved class reference, needs help from V3Param
        bool m_unresolvedGenericIface;  // Unresolved generic interface, needs help from V3Param
        bool m_genBlk;  // Contains gen block reference
        Ast_गाँठ* m_unlinkedScopep;  // Unresolved scope, needs corresponding VarXRef
        AstDisable* m_disablep;  // Disable statement under which the reference is
        bool m_dotErr;  // Error found in dotted resolution, ignore upwards
        string m_dotText;  // String of dotted names found in below parseref
        DotStates() { init(nullptr); }
        ~DotStates() = शेष;
        व्योम init(VSymEnt* curSymp, bool unresolvedClass = false) {
            m_dotPos = DP_NONE;
            m_dotSymp = curSymp;
            m_dotp = nullptr;
            m_super = false;
            m_dotErr = false;
            m_dotText = "";
            m_unresolvedCell = false;
            m_unresolvedClass = unresolvedClass;
            m_unresolvedGenericIface = false;
            m_genBlk = false;
            m_unlinkedScopep = nullptr;
            m_disablep = nullptr;
        }
        string ascii() स्थिर {
            अटल स्थिर अक्षर* स्थिर names[]
                = {"DP_NONE", "DP_PACKAGE", "DP_FIRST", "DP_SCOPE", "DP_FINAL", "DP_MEMBER"};
            std::ostringstream sstr;
            sstr << "ds=" << names[m_dotPos];
            sstr << "  dse" << cvtToHex(m_dotSymp);
            स्थिर string dsname = m_dotSymp->nodep()->name().substr(0, 8);
            sstr << "(" << m_dotSymp->nodep()->typeName() << (dsname.empty() ? "" : ":") << dsname
                 << ")";
            अगर (m_dotErr) sstr << "  [dotErr]";
            अगर (m_super) sstr << "  [super]";
            अगर (m_unresolvedCell) sstr << "  [unrCell]";
            अगर (m_unresolvedClass) sstr << "  [unrClass]";
            अगर (m_unresolvedGenericIface) sstr << "  [unrGIface]";
            अगर (m_genBlk) sstr << "  [genBlk]";
            sstr << "  txt=" << m_dotText;
            वापस sstr.str();
        }
    } m_ds;  // State to preserve across recursions

    // METHODS - Variables
    AstVar* createImplicitVar(VSymEnt* /*lookupSymp*/, AstParseRef* nodep, Ast_गाँठ_Module* modp,
                              VSymEnt* moduleSymp, bool noWarn) {
        // Create implicit after warning
        अगर (!noWarn) {
            अगर (nodep->fileline()->warnIsOff(V3ErrorCode::I_DEF_NETTYPE_WIRE)) {
                स्थिर string suggest = m_statep->suggestSymFallback(moduleSymp, nodep->name(),
                                                                    Link_गाँठ_Matcher_Var{});
                nodep->v3error("Signal definition not found, and implicit disabled with "
                               "`default_nettype: "
                               << nodep->prettyNameQ() << '\n'
                               << (suggest.empty() ? "" : nodep->warnMore() + suggest));

            }
            // Bypass looking for suggestions if IMPLICIT is turned off
            // as there could be thousands of these suppressed in large netlists
            अन्यथा अगर (!nodep->fileline()->warnIsOff(V3ErrorCode::IMPLICIT)) {
                स्थिर string suggest = m_statep->suggestSymFallback(moduleSymp, nodep->name(),
                                                                    Link_गाँठ_Matcher_Var{});
                nodep->v3warn(IMPLICIT,
                              "Signal definition not found, creating implicitly: "
                                  << nodep->prettyNameQ() << '\n'
                                  << (suggest.empty() ? "" : nodep->warnMore() + suggest));
            }
        }
        AstVar* स्थिर newp
            = नया AstVar{nodep->fileline(), VVarType::WIRE, nodep->name(), VFlagLogicPacked{}, 1};
        newp->lifetime(VLifetime::STATIC_IMPLICIT);
        newp->trace(modp->modTrace());
        modp->addStmtsp(newp);
        // Link it to signal list, must add the variable under the module;
        // current scope might be lower now
        m_statep->जोड़ो_Sym(moduleSymp, newp->name(), newp, nullptr /*classOrPackagep*/);
        वापस newp;
    }
    AstVar* foundToVarp(स्थिर VSymEnt* symp, Ast_गाँठ* nodep, VAccess access) {
        // Return a variable if possible, auto converting a modport to variable
        अगर (!symp) {
            वापस nullptr;
        } अन्यथा अगर (VN_IS(symp->nodep(), Var)) {
            वापस VN_AS(symp->nodep(), Var);
        } अन्यथा अगर (VN_IS(symp->nodep(), ModportVarRef)) {
            AstModportVarRef* स्थिर snodep = VN_AS(symp->nodep(), ModportVarRef);
            AstVar* स्थिर varp = snodep->varp();
            अगर (access.isWriteOrRW() && snodep->direction().isReadOnly()) {
                nodep->v3error("Attempt to drive input-only modport: " << nodep->prettyNameQ());
            }  // else other simulators don't warn about reading, and IEEE doesn't say illegal
            वापस varp;
        } अन्यथा {
            वापस nullptr;
        }
    }

    // Look up a virtual port through modport expression mapping.
    // When a dotted reference uses a modport with expression syntax, the virtual port name
    // maps to the real signal. This helper resolves that mapping during scope creation.
    // Returns the VarScope for the real signal, or nullptr if not found.
    AstVarScope* findVirtualPortVarScope(VSymEnt* dotSymp, स्थिर string& portName) {
        स्थिर AstVarScope* स्थिर dotVscp = VN_CAST(dotSymp->nodep(), VarScope);
        स्थिर AstVar* स्थिर dotVarp = dotVscp ? dotVscp->varp() : nullptr;
        अगर (!dotVarp) वापस nullptr;

        Ast_गाँठ_DType* dtypep = dotVarp->childDTypep();
        अगर (!dtypep) dtypep = dotVarp->subDTypep();
        अगर (!dtypep) वापस nullptr;
        स्थिर AstIfaceRefDType* स्थिर ifaceRefp = VN_CAST(dtypep, IfaceRefDType);
        अगर (!ifaceRefp || !ifaceRefp->modportp() || !ifaceRefp->ifaceViaCellp()) वापस nullptr;

        // Get the interface cell's symbol table
        VSymEnt* स्थिर ifaceCellSymp = m_statep->get_गाँठ_Sym(ifaceRefp->ifaceViaCellp());

        // Look up modport by name in interface cell
        VSymEnt* स्थिर modportSymp = ifaceCellSymp->findIdFallback(ifaceRefp->modportp()->name());
        अगर (!modportSymp) वापस nullptr;

        // Look up virtual port name in modport symbol table
        string baddot;
        VSymEnt* स्थिर virtPortp = m_statep->findSymPrefixed(modportSymp, portName, baddot, true);
        अगर (!virtPortp) वापस nullptr;

        // Symbol may point to VarScope or Var
        अगर (AstVarScope* vscp = VN_CAST(virtPortp->nodep(), VarScope)) वापस vscp;

        // Symbol points to Var - look up VarScope by name in interface cell
        अगर (स्थिर AstVar* स्थिर realVarp = VN_CAST(virtPortp->nodep(), Var)) {
            VSymEnt* स्थिर realFoundp
                = m_statep->findSymPrefixed(ifaceCellSymp, realVarp->name(), baddot, true);
            वापस realFoundp ? VN_CAST(realFoundp->nodep(), VarScope) : nullptr;
        }
        वापस nullptr;
    }

    // Capture a ParamTypeDType reference for interface typedef retargeting.
    // Called when a RefDType resolves to a ParamTypeDType owned by an interface.
    व्योम captureIfaceParamType(AstRefDType* nodep, AstParamTypeDType* defp) {
        अगर (!V3LinkDotIfaceCapture::enabled() || !m_statep->forPrimary()) वापस;
        Ast_गाँठ_Module* स्थिर defOwnerModp = V3LinkDotIfaceCapture::findOwnerModule(defp);
        अगर (!defOwnerModp || !VN_IS(defOwnerModp, Iface)) वापस;
        AstCell* स्थिर cellForCapture
            = m_ds.m_dotSymp ? VN_CAST(m_ds.m_dotSymp->nodep(), Cell) : nullptr;
        अगर (!cellForCapture) वापस;
        UINFO(9, indent() << "iface capture add paramtype " << nodep
                          << " iface=" << defOwnerModp->prettyNameQ());
        V3LinkDotIfaceCapture::addParamType(nodep, cellForCapture->name(), m_modp, defp,
                                            defOwnerModp->name());
    }

    Ast_गाँठ_Stmt* addImplicitSuperNewCall(AstFunc* स्थिर nodep,
                                         स्थिर AstClassExtends* स्थिर classExtendsp) {
        // Returns the added node
        File_पंक्ति* स्थिर fl = nodep->fileline();
        AstArg* argsp = nullptr;
        अगर (classExtendsp->argsp()) argsp = classExtendsp->argsp()->clone_वृक्ष(true);
        AstNew* स्थिर newExprp = नया AstNew{fl, argsp};
        newExprp->isImplicit(true);
        AstDot* स्थिर superNewp = नया AstDot{fl, false, नया AstParseRef{fl, "super"}, newExprp};
        Ast_गाँठ_Stmt* स्थिर superNewStmtp = superNewp->makeStmt();
        क्रम (Ast_गाँठ* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            // super.new shall be the first statement (IEEE 1800-2023 8.15)
            // but some nodes (such as variable declarations and typedefs) should stay before
            अगर (VN_IS(stmtp, गाँठ_Stmt)) {
                stmtp->add_Here_This_As_अग्र(superNewStmtp);
                वापस superNewStmtp;
            }
        }
        // There were no statements
        nodep->addStmtsp(superNewStmtp);
        वापस superNewStmtp;
    }
    व्योम checkNoDot(Ast_गाँठ* nodep) {
        अगर (VL_UNLIKELY(!nodep)) {
            UINFO(9, indent() << "iface capture null node passed to checkNoDot; dot state="
                              << m_ds.ascii());
            m_ds.m_dotErr = true;
            वापस;
        }
        अगर (VL_UNLIKELY(m_ds.m_dotPos != DP_NONE)) {
            UINFO(9, indent() << "ds=" << m_ds.ascii());
            UINFO(9, indent() << "iface capture checkNoDot hit node=" << nodep
                              << " dotState=" << m_ds.ascii());
            अगर (VL_UNLIKELY(!m_ds.m_dotp)) {
                nodep->v3error("Syntax error: Not expecting " << nodep->type()
                                                              << " here (missing dot context)");
            } अन्यथा {
                nodep->v3error("Syntax error: Not expecting "
                               << nodep->type() << " under a " << nodep->backp()->type()
                               << " in dotted expression\n"
                               << nodep->warnContextPrimary() << m_ds.m_dotp->warnOther()
                               << "... Resolving this reference\n"
                               << m_ds.m_dotp->warnContextSecondary());
            }
            m_ds.m_dotErr = true;
        }
    }
    AstVar* find_Iface_शीर्ष_Varp(Ast_गाँठ* nodep, VSymEnt* parentEntp, स्थिर string& name) {
        स्थिर string findName = name + "__Viftop";
        VSymEnt* स्थिर ifaceSymp = parentEntp->findIdFallback(findName);
        AstVar* स्थिर iface_शीर्ष_Varp = ifaceSymp ? VN_AS(ifaceSymp->nodep(), Var) : nullptr;
        UASSERT_OBJ(iface_शीर्ष_Varp, nodep, "Can't find interface var ref: " << findName);
        वापस iface_शीर्ष_Varp;
    }
    AstRSProd* findProd(Ast_गाँठ* nodep, VSymEnt* parentEntp, स्थिर string& name) {
        स्थिर VSymEnt* स्थिर foundp = parentEntp->findIdFallback(name);
        AstRSProd* foundNodep = foundp ? VN_CAST(foundp->nodep(), RSProd) : nullptr;
        अगर (!foundNodep) {
            VSpellCheck speller;
            Link_गाँठ_Matcher_Prod matcher;
            parentEntp->candidateIdFlat(&speller, &matcher, false);
            स्थिर string suggest = speller.best_Candidate_संदेश(name);
            UINFO(1, "   ErrParseRef curSymp=se" << cvtToHex(parentEntp));
            nodep->v3error("Production " << Ast_गाँठ::prettyNameQ(name) << " not found\n"
                                         << (suggest.empty() ? "" : nodep->warnMore() + suggest));
            वापस nullptr;
        }
        वापस foundNodep;
    }
    व्योम duplicatePinError(AstPin* nodep, Ast_गाँठ* origp, स्थिर अक्षर* whatp) {
        nodep->v3error("Duplicate " << whatp << " connection: " << nodep->prettyNameQ() << '\n'
                                    << nodep->warnContextPrimary() << '\n'
                                    << origp->warnOther() << "... Location of original " << whatp
                                    << " connection\n"
                                    << origp->warnContextSecondary());
    }
    व्योम markAndCheckPinDup(AstPin* nodep, Ast_गाँठ* refp, स्थिर अक्षर* whatp) {
        स्थिर स्वतः pair = m_usedPins.emplace(refp, nodep);
        अगर (pair.second) {
            अगर (!nodep->paramPath().empty())
                m_usedDefParamPins[refp].emplace(nodep->paramPath(), nodep);
            वापस;
        } अन्यथा {
            Ast_गाँठ* स्थिर origp = pair.first->second;
            अगर (nodep->paramPath().empty() || VN_AS(origp, Pin)->paramPath().empty()) {
                duplicatePinError(nodep, origp, whatp);
                वापस;
            }
        }

        स्वतः& defParamPins = m_usedDefParamPins[refp];
        स्थिर स्वतः defParamIt = defParamPins.find(nodep->paramPath());
        अगर (defParamIt != defParamPins.end()) {
            duplicatePinError(nodep, defParamIt->second, whatp);
            वापस;
        }
        defParamPins.emplace(nodep->paramPath(), nodep);
    }
    अटल bool extendsClass(स्थिर AstClass* classp, स्थिर AstClass* targetp) {
        // Return true if classp inherits from targetp through already resolved extends
        // If performance of this becomes a problem, use just a counter to check
        // exceeds some number of iterations then perform the more expensive analysis
        std::set<स्थिर AstClass*> visited;
        std::vector<स्थिर AstClass*> todo{classp};
        जबतक (!todo.empty()) {
            स्थिर AstClass* स्थिर currp = todo.back();
            todo.निकालो_back();
            अगर (currp == targetp) वापस true;
            अगर (!visited.जोड़ो(currp).second) जारी;
            क्रम (स्थिर AstClassExtends* cextp = currp->extendsp(); cextp;
                 cextp = VN_AS(cextp->nextp(), ClassExtends)) {
                अगर (स्थिर AstClass* स्थिर basep = cextp->classOrNullp()) todo.धकेलो_back(basep);
            }
        }
        वापस false;
    }
    अटल AstClocking* sensClockingp(Ast_गाँठ* nodep) {
        // Return the clocking block referenced by nodep, either directly or through a modport
        अगर (AstClocking* स्थिर clockingp = VN_CAST(nodep, Clocking)) वापस clockingp;
        अगर (स्थिर AstModportClockingRef* स्थिर clockingRefp = VN_CAST(nodep, ModportClockingRef))
            वापस clockingRefp->clockingp();
        वापस nullptr;
    }
    VSymEnt* getCreateClockingEventSymEnt(AstClocking* clockingp) {
        AstVar* स्थिर eventp = clockingp->ensureEventp(true);
        अगर (!eventp->user1p()) eventp->user1p(नया VSymEnt{m_statep->symsp(), eventp});
        वापस reinterpret_cast<VSymEnt*>(eventp->user1p());
    }
    VSymEnt* findInlinedDotsSym(Ast_गाँठ* nodep, स्थिर string& inlinedDots) {
        // Return scope that applies for searching, given renaming due to module inlining.
        // Dotted lookup is always relative to module, as maybe variable
        // name lower down with same scope name we want to ignore (t_math_divw)
        VSymEnt* dotSymp = m_modSymp;
        स्थिर string inl = Ast_गाँठ::dedotName(inlinedDots);
        string baddot;
        VSymEnt* okSymp = nullptr;
        dotSymp = m_statep->findDotted(nodep->fileline(), dotSymp, inl, baddot, okSymp, false);
        UASSERT_OBJ(dotSymp, nodep,
                    "Couldn't resolve inlined scope " << Ast_गाँठ::prettyNameQ(baddot)
                                                      << " in: " << inlinedDots);
        UINFO(9, indent() << "findInlinedDotsSym " << dotSymp
                          << " search start due to inlinedDots=" << inlinedDots);
        वापस dotSymp;
    }
    VSymEnt* findDottedRefSymp(Ast_गाँठ* nodep, VSymEnt* lookupSymp, स्थिर string& dotted,
                               string& baddot, VSymEnt*& okSymp, bool* modportp = nullptr) {
        // Return scope to search for a relinked hierarchical reference
        // (AstVarXRef or AstEnumItemRef), or nullptr after error if not found
        // If modportp is given, set true when the reference went through a modport
        अगर (modportp) *modportp = false;
        VSymEnt* dotSymp = m_statep->findDotted(nodep->fileline(), lookupSymp, dotted, baddot,
                                                okSymp, true);  // Maybe nullptr
        अगर (!dotSymp) {
            nodep->v3error(
                "Can't find definition of "
                << (!baddot.empty() ? Ast_गाँठ::prettyNameQ(baddot) : nodep->prettyNameQ()) << '\n'
                << nodep->warnContextPrimary());
            वापस nullptr;
        }
        अगर (स्थिर AstVar* स्थिर varp = VN_CAST(dotSymp->nodep(), Var)) {
            अगर (स्थिर AstIfaceRefDType* स्थिर ifaceRefp
                = VN_CAST(varp->childDTypep(), IfaceRefDType)) {
                अगर (ifaceRefp->modportp()) {
                    dotSymp = m_statep->get_गाँठ_Sym(ifaceRefp->modportp());
                    अगर (modportp) *modportp = true;
                } अन्यथा {
                    dotSymp = m_statep->get_गाँठ_Sym(ifaceRefp->ifacep());
                }
            }
        } अन्यथा अगर (स्थिर AstModportClockingRef* स्थिर clockingRefp
                   = VN_CAST(dotSymp->nodep(), ModportClockingRef)) {
            dotSymp = m_statep->get_गाँठ_Sym(clockingRefp->clockingp());
        }
        वापस dotSymp;
    }

    अटल bool isParamedClassRefDType(स्थिर Ast_गाँठ* classp) {
        जबतक (स्थिर AstRefDType* स्थिर refp = VN_CAST(classp, RefDType))
            classp = refp->subDTypep();
        वापस (VN_IS(classp, ClassRefDType) && VN_AS(classp, ClassRefDType)->paramsp())
               || VN_IS(classp, ParamTypeDType);
    }
    bool isParamedClassRef(स्थिर Ast_गाँठ* nodep) {
        // Is this a parameterized reference to a class, or a reference to class parameter
        अगर (स्थिर स्वतः* classRefp = VN_CAST(nodep, ClassOrPackageRef)) {
            अगर (classRefp->paramsp()) वापस true;
            स्थिर स्वतः* classp = classRefp->classOrPackageNodep();
            जबतक (स्थिर स्वतः* typedefp = VN_CAST(classp, Typedef)) classp = typedefp->subDTypep();
            वापस isParamedClassRefDType(classp);
        }
        वापस isParamedClassRefDType(nodep);
    }
    VSymEnt* getThisClassSymp() {
        VSymEnt* classSymp = m_ds.m_dotSymp;
        जबतक (classSymp && !VN_IS(classSymp->nodep(), Class)) {
            classSymp = classSymp->parentp();
        }
        वापस classSymp;
    }
    // Is the current scope within the given class, including within its nested classes
    bool insideClass(स्थिर AstClass* classp) स्थिर {
        क्रम (स्थिर VSymEnt* symp = m_curSymp; symp; symp = symp->parentp()) {
            अगर (symp->nodep() == classp) वापस true;
        }
        वापस false;
    }
    // 'randomize() with' target class not yet specialized; defer to post-V3Param pass
    bool randTargetUnresolved() स्थिर {
        वापस m_statep->forPrimary() && m_currentWithp && m_randMethodCallp && !m_randSymp;
    }
    // Is 'funcp' of 'classp' virtual, directly or by overriding a virtual method of a base
    // class (IEEE 1800-2023 8.20)
    bool isVirtualMethod(स्थिर Ast_गाँठ_FTask* funcp, स्थिर AstClass* classp) स्थिर {
        अगर (funcp->isVirtual()) वापस true;
        क्रम (स्थिर AstClassExtends* cextp = classp->extendsp(); cextp;
             cextp = VN_AS(cextp->nextp(), ClassExtends)) {
            AstClass* स्थिर basep = cextp->classOrNullp();
            अगर (!basep) जारी;
            स्थिर VSymEnt* स्थिर foundp = m_statep->get_गाँठ_Sym(basep)->findIdFlat(funcp->name());
            स्थिर Ast_गाँठ_FTask* स्थिर baseFuncp
                = foundp ? VN_CAST(foundp->nodep(), गाँठ_FTask) : nullptr;
            स्थिर AstClass* स्थिर baseClassp = foundp && foundp->parentp()
                                                   ? VN_CAST(foundp->parentp()->nodep(), Class)
                                                   : nullptr;
            अगर (baseFuncp && baseClassp && isVirtualMethod(baseFuncp, baseClassp)) वापस true;
        }
        वापस false;
    }
    // Is imported 'implp' an implementation of interface method 'ifSymp' (IEEE 1800-2023
    // 8.26.2)? If its class does not derive from the interface class, it needs a forwarding
    // override in the derived class to be reachable through the interface.
    bool inheritedImplements(स्थिर VSymEnt* implp, स्थिर VSymEnt* ifSymp, bool& needForwardr) {
        needForwardr = false;
        स्थिर Ast_गाँठ_FTask* स्थिर implFuncp = VN_CAST(implp->nodep(), गाँठ_FTask);
        अगर (!implFuncp || implFuncp->pureVirtual() || implFuncp->isStatic()) वापस false;
        स्थिर AstClass* स्थिर implClassp
            = implp->parentp() ? VN_CAST(implp->parentp()->nodep(), Class) : nullptr;
        स्थिर AstClass* स्थिर ifClassp
            = ifSymp->parentp() ? VN_CAST(ifSymp->parentp()->nodep(), Class) : nullptr;
        अगर (!implClassp || !ifClassp) वापस false;
        अगर (extendsClass(implClassp, ifClassp)) वापस true;
        अगर (!isVirtualMethod(implFuncp, implClassp)) वापस false;
        needForwardr = true;
        वापस true;
    }
    // Add 'implp' override to 'classp' that calls super.implp, so the C++ method of an
    // implemented interface class is overridden
    व्योम addInterfaceForward(AstClass* classp, Ast_गाँठ_FTask* implp, AstClass* implClassp) {
        File_पंक्ति* स्थिर flp = implp->fileline();
        AstVar* fvarp = nullptr;
        Ast_गाँठ_FTask* newp;
        अगर (AstFunc* स्थिर implFuncp = VN_CAST(implp, Func)) {
            fvarp = VN_AS(implFuncp->fvarp(), Var)->clone_वृक्ष(false);
            newp = नया AstFunc{flp, implp->name(), nullptr, fvarp};
        } अन्यथा {
            newp = नया AstTask{flp, implp->name(), nullptr};
        }
        newp->classMethod(true);
        newp->isVirtual(true);
        newp->lifetime(implp->lifetime());
        AstArg* argsp = nullptr;
        क्रम (Ast_गाँठ* stmtp = implp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstVar* स्थिर portp = VN_CAST(stmtp, Var);
            अगर (!portp || !portp->isIO()) जारी;
            AstVar* स्थिर newPortp = portp->clone_वृक्ष(false);
            newp->addStmtsp(newPortp);
            argsp = Ast_गाँठ::add_अग्र(
                argsp, नया AstArg{flp, "", नया AstVarRef{flp, newPortp, VAccess::READ}});
        }
        Ast_गाँठ_FTask_Ref* callp;
        अगर (AstFunc* स्थिर implFuncp = VN_CAST(implp, Func)) {
            callp = नया AstFuncRef{flp, implFuncp, argsp};
        } अन्यथा {
            callp = नया AstTaskRef{flp, VN_AS(implp, Task), argsp};
        }
        callp->classOrPackagep(implClassp);
        callp->superReference(true);
        अगर (fvarp) {
            newp->addStmtsp(नया AstAssign{flp, नया AstVarRef{flp, fvarp, VAccess::WRITE}, callp});
        } अन्यथा {
            newp->addStmtsp(callp->makeStmt());
        }
        UINFO(9, indent() << "Interface forward " << newp);
        classp->addMembersp(newp);
    }
    व्योम importDerivedClass(AstClass* derivedClassp, VSymEnt* baseSymp, AstClass* baseClassp) {
        // Also used for standard 'extends' from a base class
        UINFO(8, indent() << "importDerivedClass to " << derivedClassp << " from " << baseClassp);
        क्रम (VSymEnt::const_iterator it = baseSymp->begin(); it != baseSymp->end(); ++it) {
            अगर (Ast_गाँठ* baseSubp = it->second->nodep()) {
                UINFO(8, indent() << "  SymFunc " << baseSubp);
                स्थिर string impOrExtends
                    = baseClassp->isInterfaceClass() ? " implements " : " extends ";
                अगर (VN_IS(baseSubp, गाँठ_FTask)) {
                    स्थिर VSymEnt* स्थिर foundp = m_curSymp->findIdFlat(baseSubp->name());
                    स्थिर Ast_गाँठ_FTask* स्थिर baseFuncp = VN_CAST(baseSubp, गाँठ_FTask);
                    अगर (!baseFuncp || !baseFuncp->pureVirtual()) जारी;
                    bool needForward = false;
                    स्थिर bool existsInDerived
                        = foundp
                          && (!foundp->imported()
                              || inheritedImplements(foundp, it->second, needForward));
                    अगर (needForward && m_statep->forPrimary()
                        && !derivedClassp->isInterfaceClass()) {
                        m_ifForwards.emplace(baseSubp->name(), foundp);
                    }
                    अगर (m_statep->forPrimary() && !existsInDerived
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
                    स्थिर स्वतः itn = m_ifClassImpNames.find(baseSubp->name());
                    अगर (m_statep->forPrimary() && !existsInDerived
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
                अगर (VN_IS(baseSubp, Constraint)) {
                    स्थिर VSymEnt* स्थिर foundp = m_curSymp->findIdFlat(baseSubp->name());
                    स्थिर AstConstraint* स्थिर baseFuncp = VN_CAST(baseSubp, Constraint);
                    अगर (!baseFuncp || !baseFuncp->isKwdPure()) जारी;
                    स्थिर bool existsInDerived = foundp && !foundp->imported();
                    अगर (m_statep->forPrimary() && !existsInDerived
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
    व्योम importSymbolsFromExtended(AstClass* स्थिर nodep, AstClassExtends* स्थिर cextp) {
        AstClass* स्थिर baseClassp = cextp->classp();
        VSymEnt* स्थिर srcp = m_statep->get_गाँठ_Sym(baseClassp);
        importDerivedClass(nodep, srcp, baseClassp);
        अगर (!cextp->isImplements()) m_curSymp->importFromClass(m_statep->symsp(), srcp);
    }
    व्योम classExtendImport(AstClass* nodep) {
        // A class reference might be to a class that is later in Ast due to
        // e.g. parameterization or referring to a "class (type T) extends T"
        // Resolve it so later Class:: references into its base classes work
        symIterateNull(nodep, m_statep->get_गाँठ_Sym(nodep));
    }

    व्योम checkDeclOrder(Ast_गाँठ* nodep, Ast_गाँठ* declp) {
        स्थिर uint32_t decl_Token_अंक = declp->decl_Token_अंक();
        अगर (!decl_Token_अंक) वापस;  // Not implemented/valid on this object
        अगर (nodep->fileline()->token_अंक() < decl_Token_अंक) {
            UINFO(1, "Related node " << nodep->fileline()->token_अंक() << " " << nodep);
            UINFO(1, "Related decl " << decl_Token_अंक << " " << declp);
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
    व्योम checkMemberDeclOrder(Ast_गाँठ* nodep, AstMemberDType* declp) {
        स्थिर uint32_t decl_Token_अंक = declp->fileline()->token_अंक();
        अगर (nodep->fileline()->token_अंक() < decl_Token_अंक) {
            UINFO(1, "Related node " << nodep->fileline()->token_अंक() << " " << nodep);
            UINFO(1, "Related decl " << decl_Token_अंक << " " << declp);
            nodep->v3error("Reference to "
                           << nodep->prettyNameQ() << " before declaration (IEEE 1800-2023 6.18)\n"
                           << nodep->warnMore()
                           << "... Suggest move the declaration before the reference\n"
                           << nodep->warnContextPrimary() << '\n'
                           << declp->warnOther() << "... Location of original declaration\n"
                           << declp->warnContextSecondary());
        }
    }

    व्योम replaceWithCheckBreak(Ast_गाँठ* oldp, Ast_गाँठ_DType* newp) {
        // Flag now to avoid V3Broken throwing an internal error
        अगर (oldp->wouldBreak(newp)) {
            newp->v3error(
                "Data type used where a non-data type is expected: " << newp->prettyNameQ());
            oldp->replaceWith(नया AstConst{newp->fileline(), AstConst::BitFalseErroring{}});
        } अन्यथा {
            oldp->replaceWith(newp);
        }
    }

    // Marks the current module to be revisited after the initial AST iteration
    व्योम revisitLater(Ast_गाँठ* deferredNodep) {
        // Need to revisit entire module to build up all the necessary context
        m_lastDeferredp = deferredNodep;
        m_modulesToRevisit.जोड़ो(std::make_pair(m_modp->name(), m_modp));
    }

    व्योम updateVarUse(AstVar* nodep) {
        // Avoid dotted.PARAM false positive when in a parameter block
        // that is if ()'ed off by same dotted name as another block
        अगर (nodep && nodep->isParam()) nodep->usedParam(true);
    }

    अटल VSymEnt* findIdFallbackSkipMemberDType(VSymEnt* lookp, स्थिर string& name) {
        VSymEnt* shadowEntp = nullptr;  // Shadowing variable: not a type, kept for error report
        जबतक (lookp) {
            VSymEnt* स्थिर foundp = lookp->findIdFlat(name);
            अगर (foundp && !VN_IS(foundp->nodep(), MemberDType)) {
                // Non-type entries are not type candidates
                // (IEEE 1800-2023 6.18); skip them so an enclosing
                // type is found, but keep one to preserve the "found: ..." error.
                अगर (VN_IS(foundp->nodep(), Typedef) || VN_IS(foundp->nodep(), ParamTypeDType)
                    || VN_IS(foundp->nodep(), Class)) {
                    वापस foundp;
                }
                अगर (!shadowEntp) shadowEntp = foundp;
            }
            lookp = lookp->fallbackp();
        }
        वापस shadowEntp;
    }

    व्योम symIterateChildren(Ast_गाँठ* nodep, VSymEnt* symp) {
        // Iterate children, changing to given context, with restore to old context
        VL_RESTORER_COPY(m_ds);
        VL_RESTORER(m_curSymp);
        m_curSymp = symp;
        m_ds.init(m_curSymp);
        iterateChildren(nodep);
    }
    व्योम symIterateNull(Ast_गाँठ* nodep, VSymEnt* symp) {
        // Iterate node, changing to given context, with restore to old context
        VL_RESTORER_COPY(m_ds);
        VL_RESTORER(m_curSymp);
        m_curSymp = symp;
        m_ds.init(m_curSymp);
        iterateNull(nodep);
    }
    अटल स्थिर Ast_गाँठ_DType* get_व्यंजक_DTypep(स्थिर Ast_गाँठ_व्यंजक* selp) {
        जबतक (स्थिर Ast_गाँठ_Pre_Sel* स्थिर sp = VN_CAST(selp, गाँठ_Pre_Sel)) selp = sp->fromp();
        अगर (स्थिर AstMemberSel* स्थिर sp = VN_CAST(selp, MemberSel)) {
            अगर (स्थिर Ast_गाँठ_DType* dtypep = get_व्यंजक_DTypep(sp->fromp())) {
                अगर (स्थिर AstClassRefDType* स्थिर classRefp = VN_CAST(dtypep, ClassRefDType)) {
                    स्थिर AstClass* स्थिर classp = classRefp->classp();
                    स्थिर bool found = classp->existsMember(
                        [&dtypep, name = selp->name()](स्थिर AstClass*, स्थिर AstVar* nodep) {
                            dtypep = nodep->childDTypep();
                            वापस nodep->name() == name;
                        });
                    अगर (found) वापस dtypep->elemDTypep();
                    selp->v3error("Class " << classRefp->prettyNameQ()
                                           << " does not contain field " << selp->prettyNameQ());
                } अन्यथा {
                    selp->v3fatalSrc("Member selection on expression of type "
                                     << dtypep->prettyDTypeNameQ()
                                     << ", which is not a class type");
                }
            }
        } अन्यथा अगर (स्थिर Ast_गाँठ_Var_Ref* स्थिर varRefp = VN_CAST(selp, गाँठ_Var_Ref)) {
            वापस varRefp->varp()->childDTypep()->elemDTypep();
        }
        वापस nullptr;
    }
    अटल स्थिर AstVar* get_अग्र_Varp(स्थिर Ast_गाँठ* stmtsp) {
        // Only IO ports, as parameters are on paramsp(), not the pinsp() list paired here
        जबतक (stmtsp) {
            अगर (स्थिर AstVar* स्थिर varp = VN_CAST(stmtsp, Var)) {
                अगर (varp->isIO()) वापस varp;
            }
            stmtsp = stmtsp->nextp();
        }
        वापस nullptr;
    }
    // Introduce implicit parameters for modules with generic interafeces
    व्योम addImplicitParametersOfGenericIface(AstCell* स्थिर nodep, स्थिर AstModule* स्थिर modp) {
        // Get Param number
        पूर्णांक param_अंक = 1;
        क्रम (AstPin* paramp = nodep->paramsp(); paramp; paramp = VN_CAST(paramp->nextp(), Pin)) {
            ++param_अंक;
        }

        // Add each implicit parameter type
        स्थिर AstVar* modIfaceVarp = get_अग्र_Varp(modp->stmtsp());
        क्रम (स्थिर AstPin* pinp = nodep->pinsp(); pinp && modIfaceVarp;
             pinp = VN_CAST(pinp->nextp(), Pin),
                           modIfaceVarp = get_अग्र_Varp(modIfaceVarp->nextp())) {
            अगर (modIfaceVarp->varType() != VVarType::IFACEREF
                || !VN_IS(modIfaceVarp->childDTypep()->elemDTypep(), IfaceGenericDType)) {
                जारी;
            }
            Ast_गाँठ* exprp = pinp->exprp();
            अगर (!exprp) {
                modIfaceVarp->v3error("Interface port "
                                      << modIfaceVarp->prettyNameQ()
                                      << " is not connected to interface/modport pin expression");
                जारी;
            }
            जबतक (स्थिर Ast_गाँठ_Pre_Sel* स्थिर preSelp = VN_CAST(exprp, गाँठ_Pre_Sel)) {
                exprp = preSelp->fromp();
            }
            // Resolve pin expression to the enclosing module's port Var. At primary
            // LinkDot the expression may still be an AstParseRef, so also look up by name.
            AstVar* enclosingVarp = nullptr;
            स्थिर AstVarRef* स्थिर varRefp = VN_CAST(exprp, VarRef);
            अगर (varRefp) {
                enclosingVarp = varRefp->varp();
            } अन्यथा अगर (स्थिर AstParseRef* स्थिर parseRefp = VN_CAST(exprp, ParseRef)) {
                अगर (m_modp) {
                    अगर (VSymEnt* स्थिर symp
                        = m_statep->get_गाँठ_Sym(m_modp)->findIdFlat(parseRefp->name())) {
                        enclosingVarp = VN_CAST(symp->nodep(), Var);
                    }
                }
            }
            अगर (enclosingVarp && enclosingVarp->varType() == VVarType::IFACEREF
                && VN_IS(enclosingVarp->childDTypep()->skipRefp(), IfaceGenericDType)) {
                // Nested generic-iface forwarding (#7454): enclosing port is itself still
                // generic, so emit a placeholder __VGIfaceParam pin carrying a VarRef to
                // the outer port. V3Param rewrites it to the concrete IfaceRefDType once
                // the enclosing module is specialized (see
                // ParamProcessor::resolveGenericIfaceForwardingPins for the ordering
                // constraint that keeps the rewrite inside V3Param).
                AstVarRef* स्थिर fwdRefp
                    = नया AstVarRef{exprp->fileline(), enclosingVarp, VAccess::READ};
                AstPin* स्थिर newPinp = नया AstPin{
                    pinp->fileline(), param_अंक, "__VGIfaceParam" + modIfaceVarp->name(), fwdRefp};
                newPinp->param(true);
                visit(newPinp);
                nodep->addParamsp(newPinp);
            } अन्यथा अगर (varRefp) {
                स्थिर AstVar* स्थिर varp = varRefp->varp();
                अगर (स्थिर AstIfaceRefDType* स्थिर refp
                    = VN_CAST(varp->childDTypep()->elemDTypep(), IfaceRefDType)) {
                    AstIface* स्थिर ifacep = VN_AS(refp->cellp()->modp(), Iface);
                    AstIfaceRefDType* newIfaceRefp;
                    अगर (refp->modportp()) {
                        newIfaceRefp = नया AstIfaceRefDType{
                            refp->fileline(), refp->modportFileline(), m_modp->name(),
                            ifacep->name(), refp->modportName()};
                    } अन्यथा {
                        newIfaceRefp = नया AstIfaceRefDType{refp->fileline(), m_modp->name(),
                                                            ifacep->name()};
                    }
                    newIfaceRefp->ifacep(ifacep);
                    अगर (refp->cellp()->paramsp()) {
                        newIfaceRefp->addParamsp(refp->cellp()->paramsp()->clone_वृक्ष(true));
                    }
                    UASSERT_OBJ(pinp->name().find("__pinNumber") == 0
                                    || pinp->name() == modIfaceVarp->name(),
                                pinp, "Not found interface with such name");
                    AstPin* स्थिर newPinp
                        = नया AstPin{pinp->fileline(), param_अंक,
                                     "__VGIfaceParam" + modIfaceVarp->name(), newIfaceRefp};
                    newPinp->param(true);
                    visit(newPinp);
                    nodep->addParamsp(newPinp);
                } अन्यथा {
                    varRefp->v3error("Generic interfaces can only connect to an interface and "
                                     << varp->prettyNameQ() << " is "
                                     << (varp->childDTypep()
                                             ? "of type " + varp->childDTypep()->prettyDTypeNameQ()
                                             : "not an interface"));
                }
            } अन्यथा {
                exprp->v3error("Expected an interface but " << exprp->prettyNameQ()
                                                            << " is not an interface");
            }
            ++param_अंक;
        }
    }

    // Returns true and issues error iff 'varp' cannot be accesed with a VarXRef
    // in a hier_block given by 'scopeSymp'
    bool errorHierNonPort(AstVarXRef* refp, AstVar* varp, VSymEnt* scopeSymp) {
        // OK to access ports on hier_block
        अगर (varp->isIO()) वापस false;
        // OK if not an instance
        स्थिर AstCell* स्थिर cellp = VN_CAST(scopeSymp->nodep(), Cell);
        अगर (!cellp) वापस false;
        // Ok if not a hier_block
        स्थिर Ast_गाँठ_Module* स्थिर modp = cellp->modp();
        अगर (!modp->hierBlock()) वापस false;
        // Bad
        refp->v3error("Cannot access non-port symbols inside hierarchical block");
        वापस true;
    }

#घोषणा LINKDOT_VISIT_START() \
    VL_RESTORER(m_indent); \
    ++m_indent;

    string indent() स्थिर {
        string परिणाम = "";
        परिणाम.जोड़ो(0, m_indent, ':');
        वापस परिणाम + " ";
    }

    // VISITORS
    व्योम visit(AstNetlist* nodep) override {
        // Recurse..., backward as must do packages before using packages
        iterateChildrenBackwardsConst(nodep);
    }
    व्योम visit(AstTypeTable*) override {}
    व्योम visit(Ast_गाँठ_Module* nodep) override {
        अगर (nodep->dead()) वापस;
        LINKDOT_VISIT_START();
        UINFO(8, indent() << "visit " << nodep);
        VL_RESTORER(m_genericIfaceModule);
        अगर (स्थिर AstModule* स्थिर modp = VN_CAST(nodep, Module)) {
            m_genericIfaceModule = modp->hasGenericIface();
        }
        checkNoDot(nodep);
        m_ds.init(m_curSymp);
        m_ds.m_dotSymp = m_curSymp = m_modSymp
            = m_statep->get_गाँठ_Sym(nodep);  // Until overridden by a SCOPE
        m_cellp = nullptr;
        m_modp = nodep;
        m_modport_अंक = 0;
        iterateChildren(nodep);
        m_modp = nullptr;
        m_ds.m_dotSymp = m_curSymp = m_modSymp = nullptr;
        m_ds.m_dotPos = DP_NONE;
        m_ds.m_dotErr = false;
    }
    व्योम visit(AstScope* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(8, indent() << "visit " << nodep);
        checkNoDot(nodep);
        VL_RESTORER(m_modSymp);
        VL_RESTORER(m_curSymp);
        m_ds.m_dotSymp = m_curSymp = m_modSymp = m_statep->getScopeSym(nodep);
        iterateChildren(nodep);
        m_ds.m_dotSymp = m_curSymp = m_modSymp = nullptr;
    }
    व्योम visit(AstCellInline* nodep) override {
        LINKDOT_VISIT_START();
        checkNoDot(nodep);
        UASSERT_OBJ(m_statep->forScopeCreation(), nodep, "CellInline should not exist yet");
        VL_DO_DANGLING(धकेलो_Deletep(nodep->unlinkFrBack()), nodep);
    }
    व्योम visit(AstCell* nodep) override {
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
            अगर (VN_IS(nodep->modp(), NotFoundModule)) {
                // Prevent warnings about missing pin connects
                अगर (nodep->pinsp()) nodep->pinsp()->unlink_Fr_Back_With_अग्र()->delete_वृक्ष();
                अगर (nodep->paramsp()) nodep->paramsp()->unlink_Fr_Back_With_अग्र()->delete_वृक्ष();
            }
            // Need to pass the module info to this cell, so we can link up the pin names
            // However can't use m_curSymp as pin connections need to use the
            // instantiator's symbols
            अन्यथा {
                m_pinSymp = m_statep->get_गाँठ_Sym(nodep->modp());
                UINFO(4, indent() << "(Backto) visit " << nodep);
                // UINFOTREE(1, nodep, "", "linkcell");
                // UINFOTREE(1, nodep->modp(), "", "linkcemd");
                iterateChildren(nodep);

                अगर (m_statep->forPrimary())
                    अगर (स्थिर AstModule* स्थिर modp = VN_CAST(nodep->modp(), Module)) {
                        अगर (modp->hasGenericIface())
                            addImplicitParametersOfGenericIface(nodep, modp);
                    }
            }
        }
        // Parent module inherits child's publicity
        // This is done bottom up in the LinkBotupVisitor stage
    }
    व्योम visit(AstClassRefDType* nodep) override {
        // Cell: Recurse inside or cleanup not founds
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        // Can be under dot if called as package::class and that class resolves, so no checkNoDot
        VL_RESTORER_CLEAR(m_usedPins);
        VL_RESTORER_CLEAR(m_usedDefParamPins);
        UASSERT_OBJ(nodep->classp(), nodep, "ClassRef has unlinked class");
        UASSERT_OBJ(m_statep->forPrimary() || !nodep->paramsp() || V3Error::error_गिनती(), nodep,
                    "class reference parameter not removed by V3Param");
        VL_RESTORER(m_pinSymp);
        {
            // ClassRef's have pins, so track
            m_pinSymp = m_statep->get_गाँठ_Sym(nodep->classp());
            UINFO(4, indent() << "(Backto) visit " << nodep);
            // UINFOTREE(1, nodep, "", "linkcell");
            // UINFOTREE(1, nodep->modp(), "", "linkcemd");
            iterateChildren(nodep);
        }
    }
    व्योम visit(AstPin* nodep) override {
        // Pin: Link to submodule's port
        LINKDOT_VISIT_START();
        checkNoDot(nodep);
        iterateChildren(nodep);
        अगर (!nodep->modVarp()) {
            UASSERT_OBJ(m_pinSymp, nodep, "Pin not under instance?");
            VSymEnt* स्थिर foundp = m_pinSymp->findIdFlat(nodep->name());
            स्थिर अक्षर* स्थिर whatp = nodep->param() ? "parameter" : "pin";
            अगर (!foundp) {
                अगर (m_cellp && VN_IS(m_cellp->modp(), Primitive)
                    && (nodep->name() == "__paramNumber1" || nodep->name() == "__paramNumber2")) {
                    // Primitive parameter is really a delay2 we can just ignore
                    VL_DO_DANGLING(धकेलो_Deletep(nodep->unlinkFrBack()), nodep);
                    वापस;
                } अन्यथा अगर (nodep->param() && !nodep->exprp() && !nodep->svDotName()) {
                    // Placeholder pin the parser makes for an empty '#()', to signal
                    // the user did type the '#()'. No further use, deleting.
                    VL_DO_DANGLING(धकेलो_Deletep(nodep->unlinkFrBack()), nodep);
                    वापस;
                } अन्यथा {
                    स्थिर std::string suggest
                        = (nodep->param()
                               ? m_statep->suggestSymFlat(m_pinSymp, nodep->name(),
                                                          Link_गाँठ_Matcher_Var_Param{}, false)
                               : m_statep->suggestSymFlat(m_pinSymp, nodep->name(),
                                                          Link_गाँठ_Matcher_Var_IO{}, false));
                    स्थिर std::string decl
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
                    VL_DO_DANGLING(धकेलो_Deletep(nodep->unlinkFrBack()), nodep);
                    वापस;
                }
            }
            VVarType refVarType = VVarType::UNKNOWN;
            bool wrongPinType = false;
            अगर (AstVar* स्थिर varp = VN_CAST(foundp->nodep(), Var)) {
                अगर (varp->isIO() || varp->isParam() || varp->isIfaceRef()) {
                    refVarType = varp->varType();
                    nodep->modVarp(varp);
                } अन्यथा {
                    wrongPinType = true;
                }
            } अन्यथा अगर (AstParamTypeDType* स्थिर typep = VN_CAST(foundp->nodep(), ParamTypeDType)) {
                refVarType = typep->varType();
                nodep->modPTypep(typep);
            } अन्यथा {
                wrongPinType = true;
            }
            // Don't connect parameter pin to module ports or vice versa
            अगर (nodep->param() != (refVarType == VVarType::GPARAM)) wrongPinType = true;
            अगर (wrongPinType) {
                string targetType = LinkDotState::गाँठ_Text_Type(foundp->nodep());
                targetType = VString::aOrAn(targetType) + ' ' + targetType;
                अगर (nodep->param()) {
                    nodep->v3error("Instance attempts to override "
                                   << nodep->prettyNameQ() << " as a " << whatp << ", but it is "
                                   << targetType);
                } अन्यथा {
                    nodep->v3error("Instance attempts to connect to "
                                   << nodep->prettyNameQ() << ", but it is " << targetType);
                }
                वापस;
            }
            markAndCheckPinDup(nodep, foundp->nodep(), whatp);
        }
        // Early return() above when deleted
    }

    अटल bool isDotThisReference(स्थिर AstDot* nodep) {
        वापस VN_IS(nodep->lhsp(), ParseRef) && nodep->lhsp()->name() == "this";
    }

    व्योम visit(AstDot* nodep) override {
        // Legal under a DOT: AstDot, AstParseRef, AstPackageRef, AstNodeSel, AstPackArrayDType
        //    also a DOT can be part of an expression, but only above plus
        //    AstFTaskRef are legal children
        // Dot(PackageRef, ParseRef(text))
        // Dot(Dot(ClassOrPackageRef,ClassOrPackageRef), ParseRef(text))
        // Dot(Dot(Dot(ParseRef(text), ...
        अगर (nodep->user3SetOnce()) वापस;
        LINKDOT_VISIT_START();
        UINFO(8, indent() << "visit " << nodep);
        UINFO(8, indent() << m_ds.ascii());
        स्थिर DotStates lastStates = m_ds;
        स्थिर bool start = (m_ds.m_dotPos == DP_NONE);  // Save, as m_dotp will be changed
        स्थिर DotPosition initialDotPos = m_ds.m_dotPos;
        VL_RESTORER(m_randSymp);
        {
            अगर (start) {  // Starting dot sequence
                UINFOTREE(9, nodep, "", "dot-in");
                // Keep deferral from visit(AstMethodCall) for unspecialized rand target
                m_ds.init(m_curSymp, m_ds.m_unresolvedClass && randTargetUnresolved());
            }
            m_ds.m_dotp = nodep;  // Always, not just at start
            m_ds.m_dotPos = DP_FIRST;

            अगर (isDotThisReference(nodep)) {
                VSymEnt* classSymp = getThisClassSymp();
                // In 'randomize() with { this.member }', 'this' refers to randomized
                // object, not the calling class (IEEE 1800-2023 18.7)
                अगर (m_randSymp && m_currentWithp) classSymp = m_randSymp;
                अगर (!classSymp && randTargetUnresolved()) {
                    // Bind 'this' in post-V3Param pass
                } अन्यथा अगर (!classSymp) {
                    nodep->v3error("'this' used outside class (IEEE 1800-2023 8.11)");
                    m_ds.m_dotErr = true;
                } अन्यथा {
                    m_ds.m_dotSymp = classSymp;
                    UINFO(8, indent() << "this. " << m_ds.ascii());
                }
            } अन्यथा अगर (VN_IS(nodep->lhsp(), ParseRef) && nodep->lhsp()->name() == "super") {
                स्थिर VSymEnt* classSymp = getThisClassSymp();
                अगर (!classSymp) {
                    nodep->v3error("'super' used outside class (IEEE 1800-2023 8.15)");
                    m_ds.m_dotErr = true;
                } अन्यथा {
                    स्थिर स्वतः classp = VN_AS(classSymp->nodep(), Class);
                    स्थिर स्वतः cextp = classp->extendsp();
                    अगर (!cextp) {
                        nodep->v3error("'super' used on non-extended class (IEEE 1800-2023 8.15)");
                        m_ds.m_dotErr = true;
                    } अन्यथा {
                        अगर (m_statep->forPrimary()
                            && m_extendsParam.find(classp) != m_extendsParam.end()) {
                            UINFO(9, indent()
                                         << "deferring until post-V3Param: " << nodep->lhsp());
                            m_ds.m_unresolvedClass = true;
                        } अन्यथा {
                            स्थिर स्वतः baseClassp = cextp->classp();
                            UASSERT_OBJ(baseClassp, nodep, "Bad superclass");
                            m_ds.m_dotSymp = m_statep->get_गाँठ_Sym(baseClassp);
                            m_ds.m_super = true;
                            UINFO(8, indent() << "super. " << m_ds.ascii());
                        }
                    }
                }
            } अन्यथा अगर (AstClassOrPackageRef* स्थिर lhsp
                       = VN_CAST(nodep->lhsp(), ClassOrPackageRef)) {
                // m_ds.m_dotText communicates the cell prefix between stages
                UINFO(8, indent() << "iter.lhs   " << m_ds.ascii() << " " << nodep);
                iterate_And_अग्र_Null(lhsp);
                अगर (!lhsp->classOrPackageSkipp() && lhsp->name() != "local::") {
                    revisitLater(nodep);
                    m_ds = lastStates;
                    // Resolve function args before bailing
                    अगर (Ast_गाँठ_FTask_Ref* स्थिर ftaskrefp = VN_CAST(nodep->rhsp(), गाँठ_FTask_Ref)) {
                        iterate_And_अग्र_Null(ftaskrefp->argsp());
                    }
                    वापस;
                }
                m_ds.m_dotPos = DP_PACKAGE;
                // nodep->lhsp() may be a new node
                अगर (AstClassOrPackageRef* स्थिर classOrPackageRefp
                    = VN_CAST(nodep->lhsp(), ClassOrPackageRef)) {
                    अगर (Ast_गाँठ* स्थिर classOrPackageNodep
                        = classOrPackageRefp->classOrPackageSkipp()) {
                        m_ds.m_dotSymp = m_statep->get_गाँठ_Sym(classOrPackageNodep);
                    }
                }
                UINFO(8, indent() << "iter.ldone " << m_ds.ascii() << " " << nodep);
            } अन्यथा अगर (VN_IS(nodep->lhsp(), Dot) && VN_AS(nodep->lhsp(), Dot)->colon()) {
                // m_ds.m_dotText communicates the cell prefix between stages
                UINFO(8, indent() << "iter.lhs   " << m_ds.ascii() << " " << nodep);
                m_ds.m_dotPos = DP_PACKAGE;
                iterate_And_अग्र_Null(nodep->lhsp());
                // nodep->lhsp() may be a new node
                अगर (AstClassOrPackageRef* स्थिर crefp
                    = VN_CAST(nodep->lhsp(), ClassOrPackageRef)) {
                    अगर (!crefp->classOrPackageSkipp()) {
                        revisitLater(nodep);
                        m_ds = lastStates;
                        // Resolve function args before bailing
                        अगर (Ast_गाँठ_FTask_Ref* स्थिर ftaskrefp
                            = VN_CAST(nodep->rhsp(), गाँठ_FTask_Ref)) {
                            iterate_And_अग्र_Null(ftaskrefp->argsp());
                        }
                        वापस;
                    }
                }
                अगर (m_lastDeferredp == nodep->lhsp()) {
                    // LHS got deferred, so this node won't be resolved. Defer it too
                    m_ds = lastStates;
                    // Resolve function args before bailing
                    अगर (Ast_गाँठ_FTask_Ref* स्थिर ftaskrefp = VN_CAST(nodep->rhsp(), गाँठ_FTask_Ref)) {
                        iterate_And_अग्र_Null(ftaskrefp->argsp());
                    }
                    वापस;
                }
                UINFO(8, indent() << "iter.ldone " << m_ds.ascii() << " " << nodep);
            } अन्यथा {
                m_ds.m_dotPos = DP_FIRST;
                m_ds.m_unresolvedGenericIface = false;
                UINFO(8, indent() << "iter.lhs   " << m_ds.ascii() << " " << nodep);
                iterate_And_अग्र_Null(nodep->lhsp());
                UINFO(8, indent() << "iter.ldone " << m_ds.ascii() << " " << nodep);
                अगर (m_ds.m_unresolvedGenericIface) {
                    m_ds.m_dotPos = initialDotPos;
                    वापस;
                }
                // UINFOTREE(9, nodep, "", "dot-lho");
            }
            अगर (m_statep->forPrimary() && isParamedClassRef(nodep->lhsp())) {
                // Dots of paramed classes will be linked after deparameterization
                UINFO(9, indent() << "deferring until post-V3Param: " << nodep->lhsp());
                m_ds.m_unresolvedClass = true;
            }
            अगर (m_ds.m_unresolvedCell
                && (VN_IS(nodep->lhsp(), CellRef) || VN_IS(nodep->lhsp(), CellArrayRef))) {
                m_ds.m_unlinkedScopep = nodep->lhsp();
            }
            अगर (m_ds.m_dotPos == DP_FIRST) m_ds.m_dotPos = DP_SCOPE;
            अगर (!m_ds.m_dotErr) {  // Once something wrong, give up
                // Top 'final' dot RHS is final RHS, else it's a
                // DOT(DOT(x,*here*),real-rhs) which we consider a RHS
                अगर (start && m_ds.m_dotPos == DP_SCOPE) m_ds.m_dotPos = DP_FINAL;
                UINFO(8, indent() << "iter.rhs   " << m_ds.ascii() << " " << nodep);
                // m_ds.m_dotSymp points at lhsp()'s symbol table, so resolve RHS under that
                iterate_And_अग्र_Null(nodep->rhsp());
                UINFO(8, indent() << "iter.rdone " << m_ds.ascii() << " " << nodep);
                // UINFOTREE(9, nodep, "", "dot-rho");
            }
            अगर (!m_ds.m_unresolvedClass) {
                अगर (start) {
                    Ast_गाँठ* newp;
                    अगर (m_ds.m_dotErr) {
                        newp = नया AstConst{nodep->fileline(), AstConst::BitFalse{}};
                    } अन्यथा {
                        newp = nodep->rhsp()->unlinkFrBack();
                    }
                    UINFOTREE(9, newp, "", "dot-out");
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
                } अन्यथा {  // Dot midpoint
                    Ast_गाँठ* newp = nodep->rhsp()->unlinkFrBack();
                    अगर (m_ds.m_unresolvedCell) {
                        AstCellRef* स्थिर crp = नया AstCellRef{
                            nodep->fileline(), nodep->name(), nodep->lhsp()->unlinkFrBack(), newp};
                        newp = crp;
                    }
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
                }
            }
        }
        अगर (start) {
            m_ds = lastStates;
        } अन्यथा {
            स्थिर bool unresolvedClass = m_ds.m_unresolvedClass;
            m_ds.m_dotp = lastStates.m_dotp;
            m_ds.m_unresolvedClass |= unresolvedClass;
        }
        UINFO(8, indent() << "done " << m_ds.ascii() << " " << nodep);
    }
    व्योम visit(AstDefaultClocking* nodep) override {
        अगर (VSymEnt* स्थिर foundp = m_curSymp->findIdFallback(nodep->name())) {
            अगर (AstClocking* स्थिर clockingp = VN_CAST(foundp->nodep(), Clocking)) {
                clockingp->makeDefault();
                VL_DO_DANGLING(nodep->unlinkFrBack()->delete_वृक्ष(), nodep);
            } अन्यथा {
                nodep->v3error(nodep->prettyNameQ() << " is not a clocking identifier");
            }
        } अन्यथा {
            nodep->v3error("Can't find definition of clocking: " << nodep->prettyNameQ());
        }
    }
    व्योम visit(AstSenItem* nodep) override {
        LINKDOT_VISIT_START();
        VL_RESTORER(m_inSens);
        m_inSens = true;
        iterateChildren(nodep);
    }
    व्योम visit(AstPatMember* nodep) override {
        LINKDOT_VISIT_START();
        अगर (nodep->varrefp()) वापस;  // only do this mapping once
        // If we have a TEXT token as our key, lookup if it's a LPARAM
        अगर (AstText* स्थिर textp = VN_CAST(nodep->keyp(), Text)) {
            UINFO(9, indent() << "visit " << nodep);
            UINFO(9, indent() << "      " << textp);
            // Lookup
            अगर (VSymEnt* स्थिर foundp = m_curSymp->findIdFallback(textp->text())) {
                अगर (AstVar* स्थिर varp = VN_CAST(foundp->nodep(), Var)) {
                    अगर (varp->isParam() || varp->isGenVar()) {
                        // Attach found Text reference to PatMember
                        nodep->varrefp(
                            नया AstVarRef{nodep->fileline(),
                                          foundp->imported() ? foundp->classOrPackagep() : nullptr,
                                          varp, VAccess::READ});
                        UINFO(9, indent() << " new " << nodep->varrefp());
                    }
                }
                अगर (AstEnumItem* स्थिर itemp = VN_CAST(foundp->nodep(), EnumItem)) {
                    // Attach enum item value to PatMember
                    nodep->varrefp(
                        नया AstEnumItemRef{nodep->fileline(), itemp, foundp->classOrPackagep()});
                    UINFO(9, indent() << " new " << itemp);
                }
            }
        }
        iterateChildren(nodep);
    }
    व्योम visit(AstParseRef* nodep) override {
        अगर (nodep->user3SetOnce()) वापस;
        LINKDOT_VISIT_START();
        UINFO(9, indent() << "visit " << nodep);
        UINFO(9, indent() << m_ds.ascii());
        अगर (m_ds.m_unresolvedClass) वापस;
        // m_curSymp is symbol table of outer expression
        // m_ds.m_dotSymp is symbol table relative to "."'s above now
        UASSERT_OBJ(m_ds.m_dotSymp, nodep, "nullptr lookup symbol table");
        // Generally resolved during Primary, but might be at param time under AstUnlinkedRef
        UASSERT_OBJ(!m_statep->forScopeCreation(), nodep, "ParseRefs should no longer exist");
        स्थिर DotStates lastStates = m_ds;
        स्थिर bool start = (m_ds.m_dotPos == DP_NONE);  // Save, as m_dotp will be changed
        bool first = start || m_ds.m_dotPos == DP_FIRST;

        अगर (start) {
            m_ds.init(m_curSymp);
            // Note m_ds.m_dotp remains nullptr; this is a reference not under a dot
        }
        अगर (nodep->name() == "super") {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: super");
            m_ds.m_dotErr = true;
        }
        अगर (nodep->name() == "this") {
            iterateChildren(nodep);
            अगर (m_statep->forPrimary()) वापस;  // The class might be parameterized somewhere
            स्थिर VSymEnt* classSymp = getThisClassSymp();
            अगर (!classSymp) {
                nodep->v3error("'this' used outside class (IEEE 1800-2023 8.11)");
                वापस;
            }
            AstClass* स्थिर classp = VN_AS(classSymp->nodep(), Class);
            AstClassRefDType* स्थिर dtypep
                = नया AstClassRefDType{nodep->fileline(), classp, nullptr};
            AstThisRef* स्थिर newp = नया AstThisRef{nodep->fileline(), VFlagChildDType{}, dtypep};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
            वापस;
        }
        अगर (m_ds.m_dotPos == DP_MEMBER && VN_IS(m_ds.m_dotp->lhsp(), LambdaArgRef)
            && nodep->name() == "index") {
            // 'with' statement's 'item.index'
            // m_ds.dotp->lhsp() was checked to know if `index` is directly after lambda arg ref.
            // If not, treat it as normal member select
            iterateChildren(nodep);
            AstLambdaArgRef* स्थिर newp = नया AstLambdaArgRef{
                nodep->fileline(), m_ds.m_dotp->lhsp()->name() + "__DOT__index", true};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
            वापस;
        } अन्यथा अगर (m_ds.m_dotPos == DP_MEMBER) {
            // Found a Var, everything following is membership.  {scope}.{var}.HERE {member}
            अगर (m_ds.m_dotErr) {
                nodep->unlinkFrBack();  // Avoid circular node loop on errors
            } अन्यथा {
                Ast_गाँठ_व्यंजक* स्थिर varEtcp = VN_AS(m_ds.m_dotp->lhsp()->unlinkFrBack(), गाँठ_व्यंजक);
                Ast_गाँठ_व्यंजक* स्थिर newp = नया AstMemberSel{nodep->fileline(), varEtcp,  //
                                                           VFlagChildDType{}, nodep->name()};
                nodep->replaceWith(newp);
            }
            VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
        } अन्यथा {
            //
            string expectWhat;
            bool allowScope = false;
            bool allowVar = false;
            bool allowFTask = false;
            bool staticAccess = false;
            अगर (m_ds.m_disablep) {
                allowScope = true;
                allowFTask = true;
                expectWhat = "block/task";
            } अन्यथा अगर (m_ds.m_dotPos == DP_PACKAGE) {
                // {package-or-class}::{a}
                Ast_गाँठ_Module* classOrPackagep = nullptr;
                expectWhat = "scope/variable/func";
                allowScope = true;
                allowVar = true;
                allowFTask = true;
                staticAccess = true;
                UINFO(9, "chk pkg " << m_ds.ascii() << " lhsp=" << m_ds.m_dotp->lhsp());
                UASSERT_OBJ(VN_IS(m_ds.m_dotp->lhsp(), ClassOrPackageRef), m_ds.m_dotp->lhsp(),
                            "Bad package link");
                AstClassOrPackageRef* स्थिर cpackagerefp
                    = VN_AS(m_ds.m_dotp->lhsp(), ClassOrPackageRef);
                अगर (cpackagerefp->name() == "local::") {
                    m_randSymp = nullptr;
                    first = true;
                } अन्यथा अगर (!cpackagerefp->classOrPackageSkipp()) {
                    VSymEnt* स्थिर foundp = m_statep->resolveClassOrPackage(
                        m_ds.m_dotSymp, cpackagerefp, true, false, ":: reference");
                    अगर (!foundp) वापस;
                    classOrPackagep = cpackagerefp->classOrPackageSkipp();
                    अगर (classOrPackagep) {
                        m_ds.m_dotSymp = m_statep->get_गाँठ_Sym(classOrPackagep);
                    } अन्यथा {
                        m_ds = lastStates;
                        वापस;
                    }
                } अन्यथा {
                    classOrPackagep = cpackagerefp->classOrPackageSkipp();
                    UASSERT_OBJ(classOrPackagep, m_ds.m_dotp->lhsp(), "Bad package link");
                    m_ds.m_dotSymp = m_statep->get_गाँठ_Sym(classOrPackagep);
                }
                m_ds.m_dotPos = DP_SCOPE;
            } अन्यथा अगर (m_ds.m_dotPos == DP_SCOPE || m_ds.m_dotPos == DP_FIRST) {
                // {a}.{b}, where {a} maybe a module name
                // or variable, where dotting into structure member
                expectWhat = "scope/variable";
                allowScope = true;
                allowVar = true;
            } अन्यथा अगर (m_ds.m_dotPos == DP_NONE) {
                expectWhat = "variable";
                allowVar = true;
            } अन्यथा अगर (m_ds.m_dotPos == DP_FINAL) {
                expectWhat = "variable/method";
                allowFTask = true;
                allowVar = true;
            } अन्यथा {
                UINFO(1, "ds=" << m_ds.ascii());
                nodep->v3fatalSrc("Unhandled DotPosition");
            }
            // Lookup
            VSymEnt* foundp;
            string baddot;
            VSymEnt* okSymp = nullptr;
            // Restricted 'with (id_list)': only id_list names bind into target class.
            अगर (m_randSymp
                && (!m_currentWithp || m_currentWithp->nameResolvesToTarget(nodep->name()))) {
                foundp = m_randSymp->findIdFlat(nodep->name());
                अगर (foundp) {
                    अगर (m_currentWithp && m_currentWithp->restricted()) {
                        m_restrictedNamesUsed.जोड़ो(nodep->name());
                    }
                    अगर (!start) m_ds.m_dotPos = DP_MEMBER;
                    अगर (!m_currentWithp) {
                        UASSERT_OBJ(m_randMethodCallp, nodep, "Expected to be under randomize()");
                        // This will start failing once complex expressions are allowed on the LHS
                        // of randomize() with args
                        UASSERT_OBJ(VN_IS(m_randMethodCallp->fromp(), VarRef), m_randMethodCallp,
                                    "Expected simple randomize target");
                        // A ParseRef is used here so that the dot RHS gets resolved
                        AstMemberSel* स्थिर newp = नया AstMemberSel{
                            nodep->fileline(), m_randMethodCallp->fromp()->clone_वृक्ष(false),
                            VFlagChildDType{}, nodep->name()};
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
                        वापस;
                    }
                    UINFO(9, indent() << "randomize-with fromSym " << foundp->nodep());
                    AstLambdaArgRef* स्थिर lambdaRefp
                        = नया AstLambdaArgRef{nodep->fileline(), "__Vrandwith_obj", false};
                    AstMemberSel* newp = नया AstMemberSel{nodep->fileline(), lambdaRefp,
                                                          VFlagChildDType{}, nodep->name()};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
                    वापस;
                }
            }
            अगर (allowScope) {
                foundp = m_statep->findDotted(nodep->fileline(), m_ds.m_dotSymp, nodep->name(),
                                              baddot, okSymp, first);  // Maybe nullptr
            } अन्यथा अगर (first) {
                foundp = m_ds.m_dotSymp->findIdFallback(nodep->name());
            } अन्यथा {
                foundp = m_ds.m_dotSymp->findIdFlat(nodep->name());
            }
            अगर (!foundp && m_ds.m_dotp && isDotThisReference(m_ds.m_dotp)) {
                स्थिर AstClass* स्थिर classp = VN_CAST(m_ds.m_dotSymp->nodep(), Class);
                अगर (classp && classp->isCovergroup() && classp->covergroupEnclosingClassp()) {
                    VSymEnt* स्थिर enclosingClassSymp
                        = m_statep->get_गाँठ_Sym(classp->covergroupEnclosingClassp());
                    foundp = enclosingClassSymp->findIdFallback(nodep->name());
                    अगर (foundp) m_ds.m_dotSymp = enclosingClassSymp;
                }
            }
            // If not found in modport, check interface fallback for parameters and typedefs.
            // Parameters and typedefs are always visible through a modport (IEEE 1800-2023 25.5).
            // This mirrors the VarXRef modport parameter fallback in visit(AstVarXRef).
            अगर (!foundp && VN_IS(m_ds.m_dotSymp->nodep(), Modport)
                && m_ds.m_dotSymp->fallbackp()) {
                VSymEnt* स्थिर ifaceFallbackp = m_ds.m_dotSymp->fallbackp();
                VSymEnt* स्थिर ifaceFoundp = ifaceFallbackp->findIdFlat(nodep->name());
                अगर (ifaceFoundp) {
                    अगर (स्थिर AstVar* स्थिर varp = VN_CAST(ifaceFoundp->nodep(), Var)) {
                        अगर (varp->isParam()) foundp = ifaceFoundp;
                    } अन्यथा अगर (VN_IS(ifaceFoundp->nodep(), Typedef)
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
            अगर (!foundp && !okSymp && VN_IS(m_ds.m_dotSymp->nodep(), Modport)) {
                okSymp = m_ds.m_dotSymp;
            }
            अगर (foundp) {
                UINFO(9, indent() << "found=se" << cvtToHex(foundp) << "  exp=" << expectWhat
                                  << "  n=" << foundp->nodep());
            }
            // What fell out?
            bool ok = false;
            // Special case: waiting on clocking event
            अगर (m_inSens && foundp && m_ds.m_dotPos != DP_SCOPE && m_ds.m_dotPos != DP_FIRST) {
                अगर (AstClocking* स्थिर clockingp = sensClockingp(foundp->nodep())) {
                    foundp = getCreateClockingEventSymEnt(clockingp);
                }
            }
            अगर (!foundp) {
            } अन्यथा अगर (VN_IS(foundp->nodep(), Cell) || VN_IS(foundp->nodep(), गाँठ_Block)
                       || VN_IS(foundp->nodep(), GenBlock)
                       || VN_IS(foundp->nodep(), Netlist)  // for $root
                       || VN_IS(foundp->nodep(), Module)) {  // if top
                अगर (allowScope) {
                    ok = true;
                    m_ds.m_dotText = VString::dot(m_ds.m_dotText, ".", nodep->name());
                    m_ds.m_dotSymp = foundp;
                    m_ds.m_dotPos = DP_SCOPE;
                    अगर (m_ds.m_disablep && VN_IS(foundp->nodep(), गाँठ_Block)) {
                        // Possibly it is not the final link. If we are under dot and not in its
                        // last component, `targetp()` field will be overwritten by next components
                        m_ds.m_disablep->targetp(foundp->nodep());
                    }
                    अगर (VN_IS(foundp->nodep(), GenBlock)) {
                        m_ds.m_genBlk = true;
                        अगर (m_ds.m_disablep) {
                            m_ds.m_disablep->v3warn(
                                E_UNSUPPORTED,
                                "Unsupported: Generate block referenced by disable");
                        }
                    }
                    // Upper AstDot visitor will handle it from here
                } अन्यथा अगर (VN_IS(foundp->nodep(), Cell) && allowVar) {
                    AstCell* स्थिर cellp = VN_AS(foundp->nodep(), Cell);
                    अगर (VN_IS(cellp->modp(), Iface)) {
                        // Interfaces can be referenced like a variable for interconnect
                        VSymEnt* स्थिर cellEntp = m_statep->get_गाँठ_Sym(cellp);
                        UASSERT_OBJ(cellEntp, nodep, "No interface sym entry");
                        VSymEnt* स्थिर parentEntp
                            = cellEntp->parentp();  // Container of the var; probably a module or
                                                    // generate begin
                        AstVar* स्थिर ifaceRefVarp
                            = find_Iface_शीर्ष_Varp(nodep, parentEntp, nodep->name());
                        //
                        ok = true;
                        m_ds.m_dotSymp = foundp;
                        m_ds.m_dotPos = DP_SCOPE;
                        UINFO(9, indent() << " cell -> iface varref " << foundp->nodep());
                        Ast_गाँठ_व्यंजक* newp;
                        अगर (m_ds.m_dotText != "") {
                            newp = नया AstVarXRef{nodep->fileline(), ifaceRefVarp, m_ds.m_dotText,
                                                  VAccess::READ};
                            अगर (m_ds.m_unresolvedCell && m_ds.m_unlinkedScopep) {
                                newp = नया AstUnlinkedRef{nodep->fileline(), newp, newp->name(),
                                                          m_ds.m_unlinkedScopep->unlinkFrBack()};
                                m_ds.m_unlinkedScopep = nullptr;
                                m_ds.m_unresolvedCell = false;
                                m_ds.m_dotText = "";
                            }
                        } अन्यथा {
                            AstVarRef* स्थिर refp
                                = नया AstVarRef{nodep->fileline(), ifaceRefVarp, VAccess::READ};
                            newp = refp;
                        }
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
                    } अन्यथा अगर (VN_IS(cellp->modp(), NotFoundModule)) {
                        cellp->modNameFileline()->v3error("Cannot find file containing interface: "
                                                          << cellp->modp()->prettyNameQ());
                    }
                }
            } अन्यथा अगर (allowScope && !allowFTask && VN_IS(foundp->nodep(), गाँठ_FTask)) {
                // Function/task used as scope for static variable access (IEEE std 1800-2017 6.21)
                // Don't set m_dotText so the static variable resolves as a VarRef rather than
                // VarXRef. This is so that V3Begin won't move it out of the function/task's scope.
                ok = true;
                m_ds.m_dotSymp = foundp;
                m_ds.m_dotPos = DP_SCOPE;
            } अन्यथा अगर (allowFTask && VN_IS(foundp->nodep(), गाँठ_FTask)) {
                Ast_गाँठ_FTask_Ref* taskrefp;
                अगर (VN_IS(foundp->nodep(), Task)) {
                    taskrefp = नया AstTaskRef{nodep->fileline(), nodep->name()};
                } अन्यथा {
                    taskrefp = नया AstFuncRef{nodep->fileline(), nodep->name()};
                }
                nodep->replaceWith(taskrefp);
                VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
                m_ds = lastStates;
                वापस;
            } अन्यथा अगर (AstVar* स्थिर varp = foundToVarp(foundp, nodep, VAccess::READ)) {
                // Preserve explicit 'this' references to members inherited from
                // generic classes until after specialization.
                अगर (m_statep->forPrimary() && m_ds.m_dotp && isDotThisReference(m_ds.m_dotp)
                    && foundp->classOrPackagep() != m_modp
                    && VN_IS(foundp->classOrPackagep(), Class)
                    && foundp->classOrPackagep()->hasGParam()) {
                    m_ds.m_unresolvedClass = true;
                    वापस;
                }
                AstIfaceRefDType* स्थिर ifacerefp
                    = LinkDotState::ifaceRefFromArray(varp->subDTypep());
                अगर (varp->isIfaceRef() && m_genericIfaceModule
                    && VN_IS(varp->childDTypep(), IfaceGenericDType) && !start) {
                    // Defer only dotted member access ('d.PARAM'), as V3Param must specialize
                    // first; a standalone ref ('.x(d)') resolves via allowVar below now
                    ok = true;
                    m_ds.m_unresolvedGenericIface = true;
                } अन्यथा अगर (ifacerefp && varp->isIfaceRef()) {
                    UASSERT_OBJ(ifacerefp->ifaceViaCellp(), ifacerefp, "Unlinked interface");
                    // Really this is a scope reference into an interface
                    UINFO(9, indent() << "varref-ifaceref " << m_ds.m_dotText << "  " << nodep);
                    m_ds.m_dotText = VString::dot(m_ds.m_dotText, ".", nodep->name());
                    // If modport specified, use modport symbol table for lookup of virtual ports
                    अगर (ifacerefp->modportp() && m_statep->exists_गाँठ_Sym(ifacerefp->modportp())) {
                        m_ds.m_dotSymp = m_statep->get_गाँठ_Sym(ifacerefp->modportp());
                    } अन्यथा {
                        m_ds.m_dotSymp = m_statep->get_गाँठ_Sym(ifacerefp->ifaceViaCellp());
                    }
                    m_ds.m_dotPos = DP_SCOPE;
                    ok = true;

                    AstVarRef* स्थिर refp = नया AstVarRef{nodep->fileline(), varp, VAccess::READ};
                    nodep->replaceWith(refp);

                    VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
                } अन्यथा अगर (allowVar) {
                    Ast_गाँठ* newp;
                    अगर (m_ds.m_dotText != "") {
                        AstVarXRef* स्थिर refp
                            = नया AstVarXRef{nodep->fileline(), nodep->name(), m_ds.m_dotText,
                                             VAccess::READ};  // lvalue'ness computed later
                        refp->varp(varp);
                        // Not linked again after V3LinkLValue, so that reports writing it
                        अगर (स्थिर AstModportVarRef* स्थिर mvarp
                            = VN_CAST(foundp->nodep(), ModportVarRef)) {
                            अगर (m_statep->forParamed() && mvarp->direction().isReadOnly()) {
                                refp->readOnlyModport(true);
                            }
                        }
                        refp->containsGenBlock(m_ds.m_genBlk);
                        m_ds.m_dotText = "";
                        अगर (m_ds.m_unresolvedCell && m_ds.m_unlinkedScopep) {
                            स्थिर string dotted = refp->dotted();
                            स्थिर माप_प्रकार pos = dotted.find("__BRA__??__KET__");
                            // Arrays of interfaces all have the same parameters
                            अगर (pos != string::npos && varp->isParam()
                                && VN_IS(m_ds.m_unlinkedScopep, CellArrayRef)) {
                                refp->dotted(dotted.substr(0, pos));
                                newp = refp;
                            } अन्यथा {
                                UINFO(9, indent() << "deferring until post-V3Param: " << refp);
                                newp = नया AstUnlinkedRef{nodep->fileline(), refp, refp->name(),
                                                          m_ds.m_unlinkedScopep->unlinkFrBack()};
                                m_ds.m_unlinkedScopep = nullptr;
                                m_ds.m_unresolvedCell = false;
                            }
                        } अन्यथा {
                            newp = refp;
                        }
                    } अन्यथा {
                        अगर (staticAccess && !varp->lifetime().isStatic() && !varp->isParam()) {
                            // TODO bug4077
                            // nodep->v3error("Static access to non-static member variable "
                            //                << varp->prettyNameQ() << endl);
                        }
                        AstVarRef* स्थिर refp = नया AstVarRef{
                            nodep->fileline(), varp, VAccess::READ};  // lvalue'ness computed later
                        refp->classOrPackagep(foundp->classOrPackagep());
                        newp = refp;
                    }
                    UINFO(9, indent() << "new " << newp);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
                    m_ds.m_dotPos = DP_MEMBER;
                    ok = true;
                }
            } अन्यथा अगर (स्थिर AstModport* स्थिर modportp = VN_CAST(foundp->nodep(), Modport)) {
                // A scope reference into an interface's modport (not
                // necessarily at a pin connection)
                UINFO(9, indent() << "cell-ref-to-modport " << m_ds.m_dotText << "  " << nodep);
                UINFO(9, indent() << "unlinked " << m_ds.m_unlinkedScopep);
                UINFO(9,
                      indent() << "dotSymp " << m_ds.m_dotSymp << " " << m_ds.m_dotSymp->nodep());
                // Iface was the previously dotted component
                अगर (!m_ds.m_dotSymp || !VN_IS(m_ds.m_dotSymp->nodep(), Cell)
                    || !VN_AS(m_ds.m_dotSymp->nodep(), Cell)->modp()
                    || !VN_IS(VN_AS(m_ds.m_dotSymp->nodep(), Cell)->modp(), Iface)) {
                    nodep->v3error("Modport not referenced as <interface>."
                                   << modportp->prettyNameQ());
                } अन्यथा अगर (!VN_AS(m_ds.m_dotSymp->nodep(), Cell)->modp()
                           || !VN_IS(VN_AS(m_ds.m_dotSymp->nodep(), Cell)->modp(), Iface)) {
                    nodep->v3error("Modport not referenced from underneath an interface: "
                                   << modportp->prettyNameQ());
                } अन्यथा {
                    AstCell* स्थिर cellp = VN_AS(m_ds.m_dotSymp->nodep(), Cell);
                    UASSERT_OBJ(cellp, nodep, "Modport not referenced from an instance");
                    VSymEnt* स्थिर cellEntp = m_statep->get_गाँठ_Sym(cellp);
                    UASSERT_OBJ(cellEntp, nodep, "No interface sym entry");
                    VSymEnt* स्थिर parentEntp
                        = cellEntp->parentp();  // Container of the var; probably a
                                                // module or generate begin
                    // We drop __BRA__??__KET__ as cells don't have that naming yet
                    AstVar* स्थिर ifaceRefVarp
                        = find_Iface_शीर्ष_Varp(nodep, parentEntp, cellp->name());
                    //
                    ok = true;
                    m_ds.m_dotText = VString::dot(m_ds.m_dotText, ".", nodep->name());
                    m_ds.m_dotSymp = foundp;
                    m_ds.m_dotPos = DP_SCOPE;
                    UINFO(9, indent() << "modport -> iface varref " << foundp->nodep());
                    // We lose the modport name here, so we cannot detect mismatched modports.
                    AstVarRef* स्थिर refp
                        = नया AstVarRef{nodep->fileline(), ifaceRefVarp, VAccess::READ};
                    Ast_गाँठ_व्यंजक* newp = refp;

                    स्वतः* स्थिर cellarrayrefp = VN_CAST(m_ds.m_unlinkedScopep, CellArrayRef);
                    अगर (cellarrayrefp) {
                        // iface[vec].modport became CellArrayRef(iface, lsb)
                        // Convert back to SelBit(iface, lsb)
                        UINFO(9, indent() << "Array modport to SelBit " << cellarrayrefp);
                        newp = नया AstSelBit{cellarrayrefp->fileline(), newp,
                                             cellarrayrefp->selp()->unlinkFrBack()};
                        newp->user3(true);  // Don't process again
                        VL_DO_DANGLING(धकेलो_Deletep(cellarrayrefp->unlinkFrBack()), cellarrayrefp);
                        m_ds.m_unlinkedScopep = nullptr;
                    }
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
                }
            } अन्यथा अगर (AstConstraint* स्थिर defp = VN_CAST(foundp->nodep(), Constraint)) {
                Ast_गाँठ* स्थिर newp = नया AstConstraintRef{nodep->fileline(), nullptr, defp};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
                ok = true;
                m_ds.m_dotPos = DP_MEMBER;
                m_ds.m_dotText = "";
            } अन्यथा अगर (AstClass* स्थिर defp = VN_CAST(foundp->nodep(), Class)) {
                अगर (allowVar) {
                    अगर (m_ds.m_dotPos == DP_SCOPE && !staticAccess) {
                        // Continue hierarchical lookup in the class scope.
                        m_ds.m_dotSymp = foundp;
                    } अन्यथा {
                        AstRefDType* स्थिर newp
                            = नया AstRefDType{nodep->fileline(), nodep->name()};
                        replaceWithCheckBreak(nodep, newp);
                        VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
                    }
                    ok = true;
                    m_ds.m_dotText = "";
                } अन्यथा {
                    (व्योम)defp;  // Prevent unused variable warning
                }
            } अन्यथा अगर (AstMemberDType* स्थिर defp = VN_CAST(foundp->nodep(), MemberDType)) {
                अगर (allowVar) {
                    checkMemberDeclOrder(nodep, defp);
                    AstRefDType* स्थिर refp = नया AstRefDType{nodep->fileline(), nodep->name()};
                    refp->refDTypep(defp);
                    replaceWithCheckBreak(nodep, refp);
                    VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
                    ok = true;
                    m_ds.m_dotText = "";
                }
            } अन्यथा अगर (AstEnumItem* स्थिर valuep = VN_CAST(foundp->nodep(), EnumItem)) {
                अगर (allowVar) {
                    AstEnumItemRef* स्थिर refp
                        = नया AstEnumItemRef{nodep->fileline(), valuep, foundp->classOrPackagep()};
                    Ast_गाँठ* newp = refp;
                    // Hierarchical reference, relinked after V3Param as the referenced
                    // module may be specialized, similar to AstVarXRef
                    refp->dotted(m_ds.m_dotText);
                    refp->containsGenBlock(m_ds.m_genBlk);
                    अगर (m_ds.m_unresolvedCell && m_ds.m_unlinkedScopep) {
                        UINFO(9, indent() << "deferring until post-V3Param: " << refp);
                        newp = नया AstUnlinkedRef{nodep->fileline(), refp, refp->name(),
                                                  m_ds.m_unlinkedScopep->unlinkFrBack()};
                        m_ds.m_unlinkedScopep = nullptr;
                        m_ds.m_unresolvedCell = false;
                    }
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
                    ok = true;
                    m_ds.m_dotText = "";
                }
            } अन्यथा अगर (स्थिर AstLambdaArgRef* स्थिर argrefp
                       = VN_CAST(foundp->nodep(), LambdaArgRef)) {
                अगर (allowVar) {
                    Ast_गाँठ* स्थिर newp
                        = नया AstLambdaArgRef{nodep->fileline(), argrefp->name(), false};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
                    ok = true;
                    m_ds.m_dotPos = DP_MEMBER;
                    m_ds.m_dotText = "";
                }
            } अन्यथा अगर (VN_IS(foundp->nodep(), Clocking)) {
                m_ds.m_dotSymp = foundp;
                अगर (m_ds.m_dotText != "") m_ds.m_dotText += "." + nodep->name();
                ok = m_ds.m_dotPos == DP_SCOPE || m_ds.m_dotPos == DP_FIRST;
            } अन्यथा अगर (स्थिर AstModportClockingRef* स्थिर clockingRefp
                       = VN_CAST(foundp->nodep(), ModportClockingRef)) {
                // Clocking block accessed through a modport - redirect to actual clocking
                m_ds.m_dotSymp = m_statep->get_गाँठ_Sym(clockingRefp->clockingp());
                अगर (m_ds.m_dotText != "") m_ds.m_dotText += "." + nodep->name();
                ok = m_ds.m_dotPos == DP_SCOPE || m_ds.m_dotPos == DP_FIRST;
            } अन्यथा अगर (स्थिर Ast_गाँठ_FTask* स्थिर ftaskp = VN_CAST(foundp->nodep(), गाँठ_FTask)) {
                अगर (!ftaskp->isFunction() || ftaskp->classMethod()) {
                    ok = m_ds.m_dotPos == DP_NONE;
                    अगर (ok) {
                        // The condition is true for tasks,
                        // properties and void functions.
                        // In these cases, the parentheses may be skipped.
                        // Also SV class methods can be called without parens
                        AstFuncRef* स्थिर funcRefp
                            = नया AstFuncRef{nodep->fileline(), nodep->name()};
                        nodep->replaceWith(funcRefp);
                        VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
                    }
                }
            } अन्यथा अगर (AstTypedef* स्थिर defp = VN_CAST(foundp->nodep(), Typedef)) {
                स्थिर bool ifaceFinalSegmentAllowed
                    = (m_ds.m_dotPos == DP_FINAL) && m_ds.m_dotSymp
                      && (VN_IS(m_ds.m_dotSymp->nodep(), Modport)
                          || (VN_IS(m_ds.m_dotSymp->nodep(), Cell)
                              && VN_CAST(m_ds.m_dotSymp->nodep(), Cell)->modp()
                              && VN_IS(VN_CAST(m_ds.m_dotSymp->nodep(), Cell)->modp(), Iface)));
                // Allow interface typedef references even without IfaceCapture
                // The dependency graph handles resolution when IfaceCapture is disabled
                ok = (m_ds.m_dotPos == DP_NONE || m_ds.m_dotPos == DP_SCOPE
                      || ifaceFinalSegmentAllowed);
                अगर (ifaceFinalSegmentAllowed) {
                    UINFO(9, indent() << "allow final-segment typedef name="
                                      << nodep->prettyNameQ() << " dotText='" << m_ds.m_dotText
                                      << "' dotSym=" << m_ds.m_dotSymp);
                }
                अगर (ok) {
                    AstRefDType* स्थिर refp = नया AstRefDType{nodep->fileline(), nodep->name()};
                    // Don't check if typedef is to a <type T>::<reference> as might not be
                    // resolved yet
                    अगर (m_ds.m_dotPos == DP_NONE) checkDeclOrder(nodep, defp);
                    refp->typedefp(defp);

                    V3LinkDotIfaceCapture::captureTypedefContext(
                        refp, "typedef", static_cast<पूर्णांक>(m_ds.m_dotPos), m_ds.m_dotText,
                        m_ds.m_dotSymp, m_modp, [यह]() { वापस indent(); });

                    अगर (VN_IS(nodep->backp(), SelExtract)) {
                        m_packedArrayDtp = refp;
                    } अन्यथा {
                        replaceWithCheckBreak(nodep, refp);
                        VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
                    }
                }
            } अन्यथा अगर (AstParamTypeDType* स्थिर defp = VN_CAST(foundp->nodep(), ParamTypeDType)) {
                ok = (m_ds.m_dotPos == DP_NONE || m_ds.m_dotPos == DP_SCOPE
                      || m_ds.m_dotPos == DP_FINAL);
                अगर (ok) {
                    AstRefDType* स्थिर refp = नया AstRefDType{nodep->fileline(), nodep->name()};
                    refp->refDTypep(defp);

                    V3LinkDotIfaceCapture::captureTypedefContext(
                        refp, "paramtype", static_cast<पूर्णांक>(m_ds.m_dotPos), m_ds.m_dotText,
                        m_ds.m_dotSymp, m_modp, [यह]() { वापस indent(); });

                    अगर (VN_IS(nodep->backp(), SelExtract)) {
                        m_packedArrayDtp = refp;
                    } अन्यथा {
                        replaceWithCheckBreak(nodep, refp);
                        VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
                    }
                }
            }
            अगर (!ok) {
                अगर (m_insideClassExtParam) {
                    // Don't throw error if the reference is inside a class that extends a param,
                    // because some members can't be linked in such a case. m_insideClassExtParam
                    // may be true only in the first stage of linking.
                    // Mark that the Dot statement can't be resolved.
                    m_ds.m_unresolvedClass = true;
                    // If the symbol was a scope name, it would be resolved.
                    m_ds.m_dotPos = DP_MEMBER;
                } अन्यथा {
                    // Cells/interfaces can't be implicit
                    स्थिर bool checkImplicit
                        = (!m_ds.m_dotp && m_ds.m_dotText == "" && !m_ds.m_disablep && !foundp);
                    स्थिर bool err
                        = !(checkImplicit && m_statep->implicitOk(m_modp, nodep->name()));
                    अगर (err) {
                        अगर (foundp) {
                            nodep->v3error("Found definition of '"
                                           << m_ds.m_dotText << (m_ds.m_dotText == "" ? "" : ".")
                                           << nodep->prettyName() << "'" << " as a "
                                           << foundp->nodep()->typeName() << " but expected a "
                                           << expectWhat);
                        } अन्यथा अगर (m_ds.m_dotText == "") {
                            UINFO(1, "   ErrParseRef curSymp=se" << cvtToHex(m_curSymp)
                                                                 << " ds=" << m_ds.ascii());
                            स्थिर string suggest = m_statep->suggestSymFallback(
                                m_ds.m_dotSymp, nodep->name(), VNodeMatcher{});
                            nodep->v3error(
                                "Can't find definition of "
                                << expectWhat << ": " << nodep->prettyNameQ() << '\n'
                                << (suggest.empty() ? "" : nodep->warnMore() + suggest));
                        } अन्यथा {
                            nodep->v3error(
                                "Can't find definition of "
                                << (!baddot.empty() ? Ast_गाँठ::prettyNameQ(baddot)
                                                    : nodep->prettyNameQ())
                                << " in dotted " << expectWhat << ": '"
                                << m_ds.m_dotText + "." + nodep->prettyName() << "'\n"
                                << nodep->warnContextPrimary()
                                << (okSymp ? okSymp->cellErrorScopes(
                                                 nodep, Ast_गाँठ::prettyName(m_ds.m_dotText))
                                           : ""));
                        }
                        m_ds.m_dotErr = true;
                    }
                    अगर (checkImplicit) {
                        // Create if implicit, and also if error (so only complain once)
                        // Else if a scope is allowed, making a signal won't help error cascade
                        AstVar* स्थिर varp
                            = createImplicitVar(m_curSymp, nodep, m_modp, m_modSymp, err);
                        AstVarRef* स्थिर newp
                            = नया AstVarRef{nodep->fileline(), varp, VAccess::READ};
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
                    }
                }
            }
        }
        अगर (start) m_ds = lastStates;
    }
    व्योम visit(AstClassOrPackageRef* nodep) override {
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

            अगर (!nodep->classOrPackageSkipp() && !nodep->classOrPackageNodep()
                && nodep->name() != "local::") {
                स्थिर bool deferIfUnresolved = m_statep->forPrimary() && m_insideClassExtParam;
                m_statep->resolveClassOrPackage(m_ds.m_dotSymp, nodep, m_ds.m_dotPos != DP_PACKAGE,
                                                false, ":: reference", deferIfUnresolved);
            }

            // ClassRef's have pins, so track
            अगर (nodep->classOrPackageSkipp()) {
                m_pinSymp = m_statep->get_गाँठ_Sym(nodep->classOrPackageSkipp());
            } अन्यथा अगर (nodep->name() != "local::") {
                वापस;
            }

            AstClass* स्थिर refClassp = VN_CAST(nodep->classOrPackageSkipp(), Class);
            // Make sure any extends() are properly imported within referenced class
            अगर (refClassp && !m_statep->forPrimary()) classExtendImport(refClassp);

            अगर (m_statep->forPrimary() && refClassp && !nodep->paramsp()
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
                AstPin* स्थिर pinp = नया AstPin{nodep->fileline(), 1, "__paramNumber1", nullptr};
                pinp->param(true);
                nodep->addParamsp(pinp);
            }

            m_ds.init(m_curSymp);
            UINFO(4, indent() << "(Backto) Link ClassOrPackageRef: " << nodep);
            iterateChildren(nodep);
        }

        अगर (nodep->name() == "local::") {
            अगर (!m_randSymp && !m_randMethodCallp) {
                nodep->v3error("Illegal 'local::' outside 'randomize() with'"
                               " (IEEE 1800-2023 18.7.1)");
                m_ds.m_dotErr = true;
                वापस;
            }
        }
        अगर (m_ds.m_dotPos == DP_PACKAGE && nodep->classOrPackageSkipp()) {
            m_ds.m_dotSymp = m_statep->get_गाँठ_Sym(nodep->classOrPackageSkipp());
            UINFO(9, indent() << "set sym " << m_ds.ascii());
        }
    }
    व्योम visit(AstConstraint* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        अगर (nodep->isExternDef()) {
            अगर (स्थिर VSymEnt* स्थिर foundp
                = m_curSymp->findIdFallback("extern " + nodep->name())) {
                स्थिर AstConstraint* स्थिर protop = VN_AS(foundp->nodep(), Constraint);
                // Copy specifiers.
                // External definition cannot have any specifiers, so no value will be overwritten.
                nodep->isStatic(protop->isStatic());
            } अन्यथा {
                nodep->v3error("extern not found that declares " + nodep->prettyNameQ());
            }
        }
        अगर (nodep->isExternProto()) {
            अगर (!m_curSymp->findIdFallback(nodep->name()) && nodep->isExternExplicit()) {
                nodep->v3warn(PROTOTYPEMIS, "Definition not found for extern prototype "
                                                << nodep->prettyNameQ());
            }
        }
        VL_RESTORER(m_curSymp);
        VL_RESTORER_COPY(m_ds);
        m_ds.m_dotSymp = m_curSymp = m_statep->get_गाँठ_Sym(nodep);
        iterateChildren(nodep);
    }
    व्योम visit(AstConstraintRef* nodep) override {
        अगर (nodep->user3SetOnce()) वापस;
        LINKDOT_VISIT_START();
        UINFO(8, indent() << "visit " << nodep);
        UINFO(8, indent() << m_ds.ascii());
        // No children defined
    }
    व्योम visit(AstVarRef* nodep) override {
        // VarRef: Resolve its reference
        // ParseRefs are used the first pass (forPrimary) so we shouldn't get can't find
        // errors here now that we have a VarRef.
        // No checkNoDot; created and iterated from a parseRef
        LINKDOT_VISIT_START();
        iterateChildren(nodep);
        अगर (!nodep->varp()) {
            UINFO(9, indent() << "linkVarRef se" << cvtToHex(m_curSymp) << "  n=" << nodep);
            UASSERT_OBJ(m_curSymp, nodep, "nullptr lookup symbol table");
            VSymEnt* स्थिर foundp = m_curSymp->findIdFallback(nodep->name());
            अगर (AstVar* स्थिर varp
                = foundp ? foundToVarp(foundp, nodep, nodep->access()) : nullptr) {
                nodep->varp(varp);
                updateVarUse(nodep->varp());
                // Generally set by parse, but might be an import
                nodep->classOrPackagep(foundp->classOrPackagep());
            }
            अगर (VL_UNCOVERABLE(!nodep->varp())) {
                nodep->v3error("Can't find definition of signal, again: "  // LCOV_EXCL_LINE
                               << nodep->prettyNameQ());
            }
        }
        AstVarScope* vscp = nodep->varScopep();
        अगर (vscp && vscp->user2p() != vscp && m_replaceWithAlias) {
            vscp = LinkDotScopeVisitor::getAlias(vscp);
            nodep->varp(vscp->varp());
            nodep->varScopep(vscp);
            updateVarUse(nodep->varp());
            UINFO(7, indent() << "Resolved " << nodep);  // Also prints taskp
        }
    }
    व्योम visit(AstVarXRef* nodep) override {
        // VarRef: Resolve its reference
        // We always link even if varp() is set, because the module we choose may change
        // due to creating new modules, flattening, etc.
        अगर (nodep->user3SetOnce()) वापस;
        LINKDOT_VISIT_START();
        UINFO(8, indent() << "visit " << nodep);
        // No checkNoDot; created and iterated from a parseRef
        अगर (!m_modSymp) {
            // Module that is not in hierarchy.  We'll be dead code eliminating it later.
            UINFO(9, "Dead module for " << nodep);
            nodep->varp(nullptr);
        } अन्यथा {
            VSymEnt* lookupSymp = m_curSymp;  // Start search at current scope
            अगर (nodep->inlinedDots() != "") {  // Correct for current post-inlined scope
                lookupSymp = findInlinedDotsSym(nodep, nodep->inlinedDots());
            }
            string baddot;
            VSymEnt* okSymp;
            bool modport;
            VSymEnt* स्थिर dotSymp
                = findDottedRefSymp(nodep, lookupSymp, nodep->dotted(), baddot, okSymp, &modport);
            अगर (!dotSymp) वापस;

            अगर (!m_statep->forScopeCreation()) {
                VSymEnt* foundp = nullptr;
                अगर (modport) {
                    // This is a copy of findSymPrefixed with few modifications.
                    // This searches without a fallback like findSymPrefixed but if lookup fails it
                    // will check one fallback (fallback of modport shall be interface) and take
                    // only L and G PARAMs since they should be visible from a modport always.
                    string prefix = dotSymp->symPrefix();
                    जबतक (!foundp) {
                        foundp = dotSymp->findIdFlat(prefix + nodep->name());
                        अगर (foundp) अवरोध;
                        UASSERT_OBJ(dotSymp->fallbackp(), nodep, "Modports shall have fallback");
                        foundp = dotSymp->fallbackp()->findIdFlat(prefix + nodep->name());
                        अगर (स्थिर AstVar* स्थिर varp
                            = foundToVarp(foundp, nodep, nodep->access())) {
                            अगर (!varp->isParam()) foundp = nullptr;
                        }
                        अगर (prefix.empty()) अवरोध;
                        string अग्र_Prefix = LinkDotState::removeLastInlineScope(prefix);
                        अगर (prefix == अग्र_Prefix) अवरोध;
                        prefix = std::move(अग्र_Prefix);
                    }
                    baddot = nodep->name();
                } अन्यथा {
                    foundp = m_statep->findSymPrefixed(dotSymp, nodep->name(), baddot, true);
                }
                अगर (m_inSens && foundp) {
                    अगर (AstClocking* स्थिर clockingp = sensClockingp(foundp->nodep())) {
                        foundp = getCreateClockingEventSymEnt(clockingp);
                    }
                }
                AstVar* स्थिर varp
                    = foundp ? foundToVarp(foundp, nodep, nodep->access()) : nullptr;
                nodep->varp(varp);
                updateVarUse(nodep->varp());
                UINFO(7, indent() << "Resolved " << nodep);  // Also prints varp
                // If found, check if it's ok to access in case it's in a hier_block
                अगर (nodep->varp() && errorHierNonPort(nodep, nodep->varp(), dotSymp)) वापस;
                अगर (!nodep->varp()) {
                    nodep->v3error("Can't find definition of "
                                   << Ast_गाँठ::prettyNameQ(baddot) << " in dotted signal: '"
                                   << nodep->dotted() + "." + nodep->prettyName() << "'\n"
                                   << nodep->warnContextPrimary()
                                   << okSymp->cellErrorScopes(nodep));
                    वापस;
                }
            } अन्यथा {
                VSymEnt* स्थिर foundp
                    = m_statep->findSymPrefixed(dotSymp, nodep->name(), baddot, true);
                AstVarScope* vscp = foundp ? VN_CAST(foundp->nodep(), VarScope) : nullptr;
                // Handle modport expression virtual ports
                अगर (!vscp) vscp = findVirtualPortVarScope(dotSymp, nodep->name());
                // If found, check if it's ok to access in case it's in a hier_block
                अगर (vscp && errorHierNonPort(nodep, vscp->varp(), dotSymp)) वापस;
                अगर (!vscp) {
                    nodep->v3error("Can't find varpin scope of "
                                   << Ast_गाँठ::prettyNameQ(baddot) << " in dotted signal: '"
                                   << nodep->dotted() + "." + nodep->prettyName() << "'\n"
                                   << nodep->warnContextPrimary()
                                   << okSymp->cellErrorScopes(nodep));
                } अन्यथा {
                    अगर (vscp->user2p() && m_replaceWithAlias) {
                        vscp = LinkDotScopeVisitor::getAlias(vscp);
                    }
                    // Convert the VarXRef to a VarRef, so we don't need
                    // later optimizations to deal with VarXRef.
                    nodep->varp(vscp->varp());
                    nodep->varScopep(vscp);
                    updateVarUse(nodep->varp());
                    UINFO(7, indent() << "Resolved " << nodep);  // Also prints taskp
                    AstVarRef* स्थिर newvscp
                        = नया AstVarRef{nodep->fileline(), vscp, nodep->access()};
                    nodep->replaceWith(newvscp);
                    VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
                    UINFO(9, indent() << "new " << newvscp);  // Also prints taskp
                }
            }
        }
    }
    व्योम visit(AstEnumDType* nodep) override {
        LINKDOT_VISIT_START();
        iterateChildren(nodep);
        AstRefDType* स्थिर refdtypep = VN_CAST(nodep->subDTypep(), RefDType);
        अगर (refdtypep && (nodep == refdtypep->subDTypep())) {
            refdtypep->v3error("Self-referential enumerated type definition");
        }
    }
    व्योम visit(Ast_गाँठ_UOr_Struct_DType* nodep) override {
        LINKDOT_VISIT_START();
        symIterateChildren(nodep, m_statep->get_गाँठ_Sym(nodep));
    }
    व्योम visit(AstEnumItemRef* nodep) override {
        // Resolve its reference
        // EnumItemRefs are created by the first pass, but V3Param may regenerate due to
        // a parameterized class/module, so we shouldn't get can't find errors.
        // No checkNoDot; created and iterated from a parseRef
        अगर (nodep->user3SetOnce()) वापस;
        LINKDOT_VISIT_START();
        अगर (m_statep->forParamed() && !nodep->dotted().empty() && m_modSymp) {
            // Hierarchical reference, relink as V3Param may have specialized the module
            string baddot;
            VSymEnt* okSymp = nullptr;
            VSymEnt* स्थिर dotSymp
                = findDottedRefSymp(nodep, m_curSymp, nodep->dotted(), baddot, okSymp);
            अगर (!dotSymp) वापस;
            VSymEnt* स्थिर foundp
                = m_statep->findSymPrefixed(dotSymp, nodep->name(), baddot, true);
            AstEnumItem* स्थिर itemp = foundp ? VN_CAST(foundp->nodep(), EnumItem) : nullptr;
            अगर (!itemp) {
                nodep->v3error("Enum item " << nodep->prettyNameQ() << " not found in "
                                            << Ast_गाँठ::prettyNameQ(nodep->dotted()) << '\n'
                                            << nodep->warnContextPrimary());
                वापस;
            }
            nodep->itemp(itemp);
            UINFO(9, indent() << " relinked " << nodep);
        }
        अगर (!nodep->itemp()) {
            UINFO(9, indent() << "linkEnumRef se" << cvtToHex(m_curSymp) << "  n=" << nodep);
            UASSERT_OBJ(m_curSymp, nodep, "nullptr lookup symbol table");
            VSymEnt* स्थिर foundp = m_curSymp->findIdFallback(nodep->name());
            अगर (AstEnumItem* स्थिर itemp = foundp ? VN_CAST(foundp->nodep(), EnumItem) : nullptr) {
                nodep->itemp(itemp);
                nodep->classOrPackagep(foundp->classOrPackagep());
                UINFO(9, indent() << " resolved " << nodep);
            }
            अगर (VL_UNCOVERABLE(!nodep->itemp())) {
                nodep->v3error("Can't find definition of enum item, again: "  // LCOV_EXCL_LINE
                               << nodep->prettyNameQ());
            }
        }
        iterateChildren(nodep);
    }
    व्योम visit(AstMethodCall* nodep) override {
        // Created here so should already be resolved.
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        VL_RESTORER_COPY(m_ds);
        VL_RESTORER(m_randSymp);
        VL_RESTORER(m_randMethodCallp);
        {
            m_ds.init(m_curSymp);
            अगर (nodep->name() == "randomize" && (nodep->argsp() || nodep->withp())) {
                m_randMethodCallp = nodep;
                स्थिर Ast_गाँठ_DType* fromDtp = nodep->fromp()->dtypep();
                अगर (!fromDtp) {
                    अगर (स्थिर Ast_गाँठ_Var_Ref* स्थिर varRefp = VN_CAST(nodep->fromp(), गाँठ_Var_Ref)) {
                        fromDtp = varRefp->varp()->subDTypep();
                    } अन्यथा {
                        fromDtp = get_व्यंजक_DTypep(nodep->fromp());
                    }
                    अगर (!fromDtp) {
                        अगर (nodep->withp()) {
                            nodep->v3warn(
                                E_UNSUPPORTED,
                                "Unsupported: 'randomize() with' on complex expressions");
                        } अन्यथा {
                            nodep->v3warn(E_UNSUPPORTED,
                                          "Unsupported: Inline random variable control with "
                                          "'randomize()' called on complex expressions");
                        }
                    }
                }
                अगर (m_statep->forPrimary() && isParamedClassRefDType(fromDtp)) {
                    m_ds.m_unresolvedClass = true;
                } अन्यथा अगर (fromDtp) {
                    स्थिर AstClassRefDType* स्थिर classDtp
                        = VN_CAST(fromDtp->skipRefp(), ClassRefDType);
                    अगर (!classDtp)
                        nodep->v3error("'randomize() with' on a non-class-instance "
                                       << fromDtp->prettyNameQ());
                    अन्यथा अगर (m_statep->forPrimary() && m_extendsParam.गिनती(classDtp->classp())) {
                        // Inherited members are unavailable until the base class is specialized.
                        // Defer lookup rather than incorrectly binding names in the caller.
                        m_ds.m_unresolvedClass = true;
                    } अन्यथा {
                        m_randSymp = m_statep->get_गाँठ_Sym(classDtp->classp());
                    }
                }
            }
            iterateChildren(nodep);
        }
    }
    व्योम visit(AstVar* nodep) override {
        LINKDOT_VISIT_START();
        checkNoDot(nodep);
        VL_RESTORER(m_isParam);
        m_isParam = nodep->varType().isParam();
        iterateChildren(nodep);
        अगर (m_statep->forPrimary() && nodep->isIO() && !m_ftaskp && !nodep->user4()) {
            nodep->v3error(
                "Input/output/inout does not appear in port list: " << nodep->prettyNameQ());
        }
    }
    व्योम visit(AstVarScope* nodep) override {
        LINKDOT_VISIT_START();
        checkNoDot(nodep);
        iterateChildren(nodep);
        AstVarScope* aliasp = LinkDotScopeVisitor::getAlias(nodep);
        अगर (aliasp && aliasp != nodep && !nodep->varp()->isIfaceRef()) {
            // Aliased variable might still be references from outside,
            // eg through the VPI, and is traced, so we need the value to propagate.
            // TODO: this means external writes to the LHS (e.g.: through the VPI) don't work
            AstAssignW* स्थिर assignp = नया AstAssignW{
                nodep->fileline(), नया AstVarRef{nodep->fileline(), nodep, VAccess::WRITE},
                नया AstVarRef{nodep->fileline(), aliasp, VAccess::READ}};
            assignp->user2(true);
            nodep->scopep()->addBlocksp(नया AstAlways{assignp});
            // Propagate attributes and lint state of the replaced variable,
            // because all references to it are replaced with references to the alias variable
            aliasp->varp()->propagateAttrFrom(nodep->varp());
            aliasp->varp()->fileline()->modifyStateInherit(nodep->varp()->fileline());
        }
    }
    व्योम visit(Ast_गाँठ_FTask_Ref* nodep) override {
        अगर (nodep->user3SetOnce()) वापस;
        LINKDOT_VISIT_START();
        UINFO(8, indent() << "visit " << nodep);
        UINFO(8, indent() << m_ds.ascii());
        अगर (m_ds.m_dotPos != DP_MEMBER || nodep->name() != "randomize") {
            // Visit arguments at the beginning.
            // They may be visitted even if the current node can't be linked now.
            symIterateChildren(nodep, m_curSymp);
        }

        अगर (m_ds.m_super) {
            अगर (AstFuncRef* स्थिर funcRefp = VN_CAST(nodep, FuncRef)) {
                funcRefp->superReference(true);
            } अन्यथा अगर (AstTaskRef* स्थिर taskRefp = VN_CAST(nodep, TaskRef)) {
                taskRefp->superReference(true);
            }
        }

        VL_RESTORER(m_randSymp);

        bool first = !m_ds.m_dotp || m_ds.m_dotPos == DP_FIRST;
        bool staticAccess = false;
        अगर (m_ds.m_unresolvedClass) {
            // Unable to link before V3Param
            वापस;
        } अन्यथा अगर (m_ds.m_unresolvedCell && m_ds.m_dotPos == DP_FINAL && m_ds.m_unlinkedScopep) {
            Ast_गाँठ_FTask_Ref* स्थिर newftaskp = nodep->clone_वृक्ष(false);
            newftaskp->dotted(m_ds.m_dotText);
            Ast_गाँठ* स्थिर newp = नया AstUnlinkedRef{nodep->fileline(), newftaskp, nodep->name(),
                                                     m_ds.m_unlinkedScopep->unlinkFrBack()};
            m_ds.m_unlinkedScopep = nullptr;
            m_ds.m_unresolvedCell = false;
            nodep->replaceWith(newp);
            VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
            वापस;
        } अन्यथा अगर (m_ds.m_dotp && m_ds.m_dotPos == DP_PACKAGE) {
            UASSERT_OBJ(VN_IS(m_ds.m_dotp->lhsp(), ClassOrPackageRef), m_ds.m_dotp->lhsp(),
                        "Bad package link");
            staticAccess = true;
            AstClassOrPackageRef* स्थिर cpackagerefp
                = VN_AS(m_ds.m_dotp->lhsp(), ClassOrPackageRef);
            अगर (cpackagerefp->name() == "local::") {
                m_randSymp = nullptr;
                first = true;
            } अन्यथा अगर (!cpackagerefp->classOrPackageSkipp()) {
                VSymEnt* स्थिर foundp = m_statep->resolveClassOrPackage(
                    m_ds.m_dotSymp, cpackagerefp, true, false, ":: reference");
                अगर (foundp) nodep->classOrPackagep(cpackagerefp->classOrPackageSkipp());
            } अन्यथा {
                nodep->classOrPackagep(cpackagerefp->classOrPackageSkipp());
            }
            // Class/package :: HERE function() . method_called_on_function_return_value()
            m_ds.m_dotPos = DP_MEMBER;
        } अन्यथा अगर (m_ds.m_dotp && m_ds.m_dotPos == DP_FINAL) {
            nodep->dotted(m_ds.m_dotText);  // Maybe ""
            // Only flag FTaskRefs under generate-if/case blocks that may be
            // pruned.  GenFor and plain begin-blocks won't be pruned by V3Param.
            // A generate-case block's parent is the GenCaseItem, not the GenCase.
            // VarXRef uses the broader m_genBlk flag (set for all GenBlocks)
            // because genfor unrolling also removes variables.
            अगर (m_ds.m_genBlk && m_ds.m_dotSymp) {
                स्थिर Ast_गाँठ* स्थिर blkp = m_ds.m_dotSymp->nodep();
                अगर (VN_IS(blkp, GenBlock)
                    && (VN_IS(blkp->backp(), GenIf) || VN_IS(blkp->backp(), GenCaseItem))) {
                    nodep->containsGenBlock(true);
                }
            }
        } अन्यथा अगर (m_ds.m_dotp && m_ds.m_dotPos == DP_MEMBER) {
            // Found a Var, everything following is method call.
            // {scope}.{var}.HERE {method} ( ARGS )
            Ast_गाँठ_व्यंजक* स्थिर varEtcp = VN_AS(m_ds.m_dotp->lhsp()->unlinkFrBack(), गाँठ_व्यंजक);
            AstArg* स्थिर argsp = nodep->argsp();
            अगर (argsp) argsp->unlink_Fr_Back_With_अग्र();
            AstMethodCall* स्थिर newp = नया AstMethodCall{nodep->fileline(), varEtcp,
                                                          VFlagChildDType{}, nodep->name(), argsp};
            अगर (AstWith* स्थिर withp = nodep->withp()) newp->withp(withp->unlinkFrBack());
            nodep->replaceWith(newp);
            VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
            वापस;
        } अन्यथा अगर (m_ds.m_dotp && (m_ds.m_dotPos == DP_SCOPE || m_ds.m_dotPos == DP_FIRST)) {
            // HERE function() . method_called_on_function_return_value()
            m_ds.m_dotPos = DP_MEMBER;
            m_ds.m_dotText = "";
        } अन्यथा अगर (!m_ds.m_disablep) {
            // visit(AstDisable*) setup the dot handling
            checkNoDot(nodep);
        }
        अगर (nodep->classOrPackagep() && nodep->taskp()) {
            // References into packages don't care about cell hierarchy.
        } अन्यथा अगर (!m_modSymp) {
            // Module that is not in hierarchy.  We'll be dead code eliminating it later.
            UINFO(9, indent() << "Dead module for " << nodep);
            nodep->taskp(nullptr);
        } अन्यथा अगर (nodep->dotted() == "" && nodep->taskp()) {
            // Earlier should have setup the links
            // Might be under a BEGIN we're not processing, so don't relink it
        } अन्यथा {
            string baddot;
            VSymEnt* okSymp = nullptr;
            VSymEnt* dotSymp;
            अगर (nodep->dotted().empty()) {
                // Non-'super.' dotted reference
                dotSymp = m_ds.m_dotSymp;
            } अन्यथा {
                // Start search at module, as a variable of same name under a subtask isn't a
                // relevant hit however a function under a begin/end is.  So we want begins, but
                // not the function
                dotSymp = m_curSymp;
            }
            अगर (nodep->classOrPackagep()) {  // Look only in specified package
                dotSymp = m_statep->get_गाँठ_Sym(nodep->classOrPackagep());
                UINFO(8, indent() << "Override classOrPackage " << dotSymp);
            } अन्यथा {
                अगर (nodep->inlinedDots() != "") {  // Correct for current post-inlined scope
                    dotSymp = findInlinedDotsSym(nodep, nodep->inlinedDots());
                }
                dotSymp = m_statep->findDotted(nodep->fileline(), dotSymp, nodep->dotted(), baddot,
                                               okSymp, true);  // Maybe nullptr
            }
            // Restricted 'with (id_list)': only id_list names bind into target class.
            अगर (m_randSymp
                && (!m_currentWithp || m_currentWithp->nameResolvesToTarget(nodep->name()))) {
                VSymEnt* स्थिर foundp = m_randSymp->findIdFlat(nodep->name());
                अगर (foundp && m_currentWithp) {
                    अगर (m_currentWithp->restricted()) {
                        m_restrictedNamesUsed.जोड़ो(nodep->name());
                    }
                    UINFO(9, indent() << "randomize-with fromSym " << foundp->nodep());
                    AstArg* argsp = nullptr;
                    अगर (nodep->argsp()) {
                        iterate_And_अग्र_Null(nodep->argsp());
                        argsp = nodep->argsp()->unlink_Fr_Back_With_अग्र();
                    }
                    अगर (m_ds.m_dotPos != DP_NONE) m_ds.m_dotPos = DP_MEMBER;
                    Ast_गाँठ* स्थिर newp = नया AstMethodCall{
                        nodep->fileline(),
                        नया AstLambdaArgRef{nodep->fileline(), "__Vrandwith_obj", false},
                        VFlagChildDType{}, nodep->name(), argsp};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
                    वापस;
                }
            }
            अगर (first && nodep->name() == "randomize" && VN_IS(m_modp, Class)
                && !VN_IS(nodep->classOrPackagep(), Package)) {
                // need special handling to avoid falling back to std::randomize
                // Skip if classOrPackagep is a Package (i.e. std::randomize resolved earlier)
                VMemberMap memberMap;
                AstFunc* स्थिर randFuncp = V3Randomize::newRandomizeFunc(
                    memberMap, VN_AS(m_modp, Class), nodep->name(), true, true);
                nodep->taskp(randFuncp);
                nodep->classOrPackagep(VN_AS(m_modp, Class));
                m_curSymp = m_statep->जोड़ो_Block(m_curSymp, nodep->name(), randFuncp, m_modp);
                // Already linked to the class randomize function; skip findSymPrefixed
                // which may find std::randomize and overwrite classOrPackagep
                वापस;
            }
            VSymEnt* स्थिर foundp
                = m_statep->findSymPrefixed(dotSymp, nodep->name(), baddot, first);
            Ast_गाँठ_FTask* स्थिर taskp = foundp ? VN_CAST(foundp->nodep(), गाँठ_FTask) : nullptr;
            Ast_गाँठ_Module* स्थिर taskModulep = foundp ? foundp->classOrPackagep() : nullptr;
            // Ignore deferring linking of functions inside params of classes extending
            // parametric classes if referenced function can be linked in the first pass.
            अगर (!m_isParam || !taskp
                || (taskp->classMethod() && taskModulep && taskModulep->hasParameterList())) {
                अगर (m_insideClassExtParam) {
                    // The reference may point to a method declared in a super class, which is
                    // proved by a parameter. In such a case, it can't be linked at the first
                    // stage. Must not do any linking, because e.g. might find an extends of an
                    // upper class because the current class (under parent) isn't yet importing
                    // it's extended class symbols.
                    वापस;
                }
                अगर (AstClass* स्थिर targetClassp = VN_CAST(dotSymp->nodep(), Class)) {
                    अगर (m_extendsParam.गिनती(targetClassp)) {
                        // Target class has parameterized extends not yet resolved.
                        // Its inherited symbols (e.g. static functions from the base class)
                        // aren't imported yet - defer to linkDotParamed.
                        वापस;
                    }
                }
            }

            अगर (taskp) {
                अगर (staticAccess && !taskp->isStatic()) {
                    // TODO bug4077
                    // nodep->v3error("Static access to non-static task/function "
                    //                << taskp->prettyNameQ() << endl);
                }
                nodep->taskp(taskp);
                nodep->classOrPackagep(foundp->classOrPackagep());
                UINFO(7, indent() << "Resolved " << nodep);  // Also prints taskp
            } अन्यथा {
                // Note ParseRef has similar error handling/message output
                UINFO(7, indent() << "   ErrFtask curSymp=se" << cvtToHex(m_curSymp)
                                  << " dotSymp=se" << cvtToHex(dotSymp));
                अगर (foundp) {
                    अगर (VN_IS(foundp->nodep(), Var) && m_ds.m_dotText == "" && m_ftaskp
                        && m_ftaskp->name() == foundp->nodep()->name()) {
                        // This is a recursive reference to the function itself, not to the var
                        nodep->taskp(m_ftaskp);
                        nodep->classOrPackagep(foundp->classOrPackagep());
                        UINFO(7, indent() << "Resolved recursive " << nodep);  // Also prints taskp
                    } अन्यथा {
                        nodep->v3error("Found definition of '"
                                       << m_ds.m_dotText << (m_ds.m_dotText == "" ? "" : ".")
                                       << nodep->prettyName() << "'" << " as a "
                                       << foundp->nodep()->typeName()
                                       << " but expected a task/function");
                    }
                } अन्यथा अगर (VN_IS(nodep, New) && !m_statep->forScopeCreation()) {
                    // Resolved in V3Width
                } अन्यथा अगर ((nodep->name() == "pre_randomize" || nodep->name() == "post_randomize")
                           && VN_IS(dotSymp->nodep(), Class)) {
                    AstClass* स्थिर classp = VN_AS(dotSymp->nodep(), Class);
                    // Functions that might be defined by user, but if not, they get empty
                    // definitions within referenced class.
                    // TODO perhaps cleaner to add to symbol table up-front with
                    // a lambda to construct them on a reference.
                    AstFunc* स्थिर funcp
                        = नया AstFunc{nodep->fileline(), nodep->name(), nullptr, nullptr};
                    funcp->classMethod(true);
                    funcp->dtypep(funcp->findVoidDType());
                    classp->addMembersp(funcp);
                    m_statep->जोड़ो_Block(dotSymp, nodep->name(), funcp, classp);
                    nodep->taskp(funcp);
                    nodep->classOrPackagep(classp);
                    UINFO(9, indent() << " Auto-made " << funcp);
                    वापस;
                } अन्यथा अगर (nodep->name() == "randomize" || nodep->name() == "srandom"
                           || nodep->name() == "get_randstate" || nodep->name() == "set_randstate"
                           || nodep->name() == "rand_mode" || nodep->name() == "constraint_mode") {
                    अगर (AstClass* स्थिर classp = VN_CAST(m_modp, Class)) {
                        // Don't overwrite if already resolved to a package (e.g. std::randomize)
                        अगर (!VN_IS(nodep->classOrPackagep(), Package))
                            nodep->classOrPackagep(classp);
                    } अन्यथा अगर (nodep->name() == "randomize") {
                        // A std::randomize resolved in V3Width
                        nodep->classOrPackagep(v3Global.rootp()->stdPackagep());
                    } अन्यथा {
                        nodep->v3error("Calling implicit class method "
                                       << nodep->prettyNameQ() << " without being under class");
                        nodep->replaceWith(नया AstConst{nodep->fileline(), 0});
                        VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
                        वापस;
                    }
                } अन्यथा अगर (nodep->dotted() == "") {
                    अगर (nodep->pli()) {
                        अगर (v3Global.opt.bboxSys()) {
                            Ast_गाँठ* newp;
                            अगर (VN_IS(nodep, FuncRef)) {
                                newp = नया AstConst{nodep->fileline(), AstConst::All0{}};
                            } अन्यथा {
                                Ast_गाँठ_व्यंजक* outp = nullptr;
                                जबतक (AstArg* स्थिर argp = nodep->argsp()) {
                                    outp = Ast_गाँठ::add_अग्र(outp, argp->exprp()->unlinkFrBack());
                                    VL_DO_DANGLING(धकेलो_Deletep(argp->unlinkFrBack()), argp);
                                }
                                newp = नया AstSysIgnore{nodep->fileline(), outp};
                                newp->dtypep(nodep->dtypep());
                            }
                            nodep->replaceWith(newp);
                            VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
                            वापस;
                        } अन्यथा {
                            VSpellCheck speller;
                            V3Parse::candidatePli(&speller);
                            स्थिर string suggest = speller.best_Candidate_संदेश(nodep->prettyName());
                            nodep->v3error(
                                "Unsupported or unknown PLI call: "
                                << nodep->prettyNameQ() << '\n'
                                << (suggest.empty() ? "" : nodep->warnMore() + suggest));
                        }
                    } अन्यथा {
                        स्थिर string suggest = m_statep->suggestSymFallback(
                            dotSymp, nodep->name(), Link_गाँठ_Matcher_FTask{});
                        nodep->v3error("Can't find definition of task/function: "
                                       << nodep->prettyNameQ() << '\n'
                                       << (suggest.empty() ? "" : nodep->warnMore() + suggest));
                    }
                } अन्यथा {
                    स्थिर string suggest = m_statep->suggestSymFallback(dotSymp, nodep->name(),
                                                                        Link_गाँठ_Matcher_FTask{});
                    nodep->v3error("Can't find definition of "
                                   << Ast_गाँठ::prettyNameQ(baddot) << " in dotted task/function: '"
                                   << nodep->dotted() + "." + nodep->prettyName() << "'\n"
                                   << (suggest.empty() ? "" : nodep->warnMore() + suggest) << '\n'
                                   << nodep->warnContextPrimary()
                                   << okSymp->cellErrorScopes(nodep));
                }
            }
        }
    }
    व्योम visit(AstSelBit* nodep) override {
        अगर (nodep->user3SetOnce()) वापस;
        LINKDOT_VISIT_START();
        iterate_And_अग्र_Null(nodep->fromp());
        अगर (m_ds.m_unresolvedClass) {
            UASSERT_OBJ(m_ds.m_dotPos != DP_SCOPE && m_ds.m_dotPos != DP_FIRST, nodep,
                        "Object of unresolved class on scope position in dotted reference");
            // The index does not depend on the class, and V3Param must see its class references
            symIterateNull(nodep->bitp(), m_curSymp);
            symIterateNull(nodep->attrp(), m_curSymp);
            वापस;
        }
        अगर (m_ds.m_dotPos == DP_SCOPE
            || m_ds.m_dotPos
                   == DP_FIRST) {  // Already under dot, so this is {modulepart} DOT {modulepart}
            UINFO(9, indent() << "deferring until after a V3Param pass: " << nodep);
            m_ds.m_dotText += "__BRA__??__KET__";
            m_ds.m_unresolvedCell = true;
            // And pass up m_ds.m_dotText
        }
        // Pass dot state down to only fromp()
        iterate_And_अग्र_Null(nodep->fromp());
        symIterateNull(nodep->bitp(), m_curSymp);
        symIterateNull(nodep->attrp(), m_curSymp);
        अगर (m_ds.m_unresolvedCell && (m_ds.m_dotPos == DP_SCOPE || m_ds.m_dotPos == DP_FIRST)) {
            Ast_गाँठ_व्यंजक* स्थिर exprp = nodep->bitp()->unlinkFrBack();
            अगर (AstCellArrayRef* स्थिर fromArrRefp = VN_CAST(nodep->fromp(), CellArrayRef)) {
                // Multi-dim iface array access: append this select to the existing chain
                fromArrRefp->addSelp(exprp);
                AstCellArrayRef* स्थिर movedp = fromArrRefp->unlinkFrBack();
                nodep->replaceWith(movedp);
                VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
            } अन्यथा {
                AstCellArrayRef* स्थिर newp
                    = नया AstCellArrayRef{nodep->fileline(), nodep->fromp()->name(), exprp};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
            }
        }
    }
    व्योम visit(Ast_गाँठ_Pre_Sel* nodep) override {
        // Excludes simple AstSelBit, see above
        अगर (nodep->user3SetOnce()) वापस;
        LINKDOT_VISIT_START();
        अगर (m_ds.m_dotPos == DP_SCOPE
            || m_ds.m_dotPos
                   == DP_FIRST) {  // Already under dot, so this is {modulepart} DOT {modulepart}
            nodep->v3error("Syntax error: Range ':', '+:' etc are not allowed in the instance "
                           "part of a dotted reference");
            m_ds.m_dotErr = true;
            वापस;
        }
        Ast_गाँठ_DType* packedArrayDtp = nullptr;  // Datatype reference for packed array
        {
            VL_RESTORER(m_packedArrayDtp);
            iterate_And_अग्र_Null(nodep->fromp());
            symIterateNull(nodep->rhsp(), m_curSymp);
            symIterateNull(nodep->thsp(), m_curSymp);

            अगर (m_packedArrayDtp) {
                AstRange* स्थिर newRangep
                    = नया AstRange(nodep->fileline(), nodep->rhsp()->unlinkFrBack(),
                                   nodep->thsp()->unlinkFrBack());
                AstPackArrayDType* newArrayTypep
                    = नया AstPackArrayDType(nodep->fileline(), m_packedArrayDtp, newRangep);
                newArrayTypep->childDTypep(m_packedArrayDtp);
                newArrayTypep->refDTypep(nullptr);
                अगर (VN_IS(nodep->backp(), SelExtract)) {
                    packedArrayDtp = newArrayTypep;
                } अन्यथा {
                    replaceWithCheckBreak(nodep, newArrayTypep);
                    VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
                }
            } अन्यथा {
                अगर (nodep->attrp()) {
                    Ast_गाँठ* स्थिर attrp = nodep->attrp()->unlinkFrBack();
                    VL_DO_DANGLING(attrp->delete_वृक्ष(), attrp);
                }
                Ast_गाँठ* स्थिर basefromp = nodep->baseFromp(false);
                अगर (VN_IS(basefromp, Replicate)) {
                    // From {...}[...] syntax in IEEE 2017
                    अगर (basefromp) UINFO(9, indent() << " Related node: " << basefromp);
                } अन्यथा {
                    nodep->attrp(नया AstAttrOf{nodep->fileline(), VAttrType::VAR_BASE,
                                               basefromp->clone_वृक्ष(false)});
                }
            }
        }

        अगर (packedArrayDtp) m_packedArrayDtp = packedArrayDtp;
    }
    व्योम visit(AstMemberSel* nodep) override {
        // checkNoDot not appropriate, can be under a dot
        LINKDOT_VISIT_START();
        iterateChildren(nodep);
    }
    व्योम visit(AstGenBlock* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        {
            VL_RESTORER(m_curSymp);
            VL_RESTORER_COPY(m_ds);
            अगर (nodep->name() != "") {
                m_ds.m_dotSymp = m_curSymp = m_statep->get_गाँठ_Sym(nodep);
                UINFO(5, indent() << "cur=se" << cvtToHex(m_curSymp));
            }
            iterateChildren(nodep);
        }
        UINFO(5, indent() << "cur=se" << cvtToHex(m_curSymp));
    }
    व्योम visit(Ast_गाँठ_Block* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        {
            VL_RESTORER(m_curSymp);
            VL_RESTORER_COPY(m_ds);
            अगर (nodep->name() != "") {
                m_ds.m_dotSymp = m_curSymp = m_statep->get_गाँठ_Sym(nodep);
                UINFO(5, indent() << "cur=se" << cvtToHex(m_curSymp));
            }
            iterateChildren(nodep);
        }
        UINFO(5, indent() << "cur=se" << cvtToHex(m_curSymp));
    }
    व्योम visit(Ast_गाँठ_FTask* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        अगर (nodep->isExternDef() && !nodep->didProto()) {
            nodep->didProto(true);
            अगर (स्थिर VSymEnt* स्थिर foundp
                = m_curSymp->findIdFallback("extern " + nodep->name())) {
                स्थिर Ast_गाँठ_FTask* स्थिर protop = VN_AS(foundp->nodep(), गाँठ_FTask);
                // Copy specifiers.
                // External definition cannot have any specifiers, so no value will be overwritten.
                nodep->isHideLocal(protop->isHideLocal());
                nodep->isHideProtected(protop->isHideProtected());
                nodep->isStatic(protop->isStatic());
                nodep->isVirtual(protop->isVirtual());
                nodep->lifetime(protop->lifetime());
                // Always add an FUNC_RETURN_PROTO even if task, so we can locate the prototype
                nodep->addStmtsp(नया AstAttrOf{protop->fileline(), VAttrType::FUNC_RETURN_PROTO,
                                               protop->fvarp()
                                                   ? protop->fvarp()->clone_वृक्ष(false)
                                                   : नया AstVoidDType{protop->fileline()}});
                क्रम (Ast_गाँठ* stmtp = protop->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    अगर (AstVar* स्थिर portp = VN_CAST(stmtp, Var)) {
                        AstAttrOf* स्थिर attrp
                            = नया AstAttrOf{protop->fileline(), VAttrType::FUNC_ARG_PROTO,
                                            portp->subDTypep()->clone_वृक्ष(false)};
                        attrp->name(portp->name());
                        nodep->addStmtsp(attrp);
                    }
                }
            } अन्यथा {
                nodep->v3error("extern not found that declares " + nodep->prettyNameQ());
            }
        }
        अगर (nodep->isExternProto()) {
            अगर (!m_curSymp->findIdFallback(nodep->name())) {
                nodep->v3warn(PROTOTYPEMIS,
                              "Definition not found for extern prototype " + nodep->prettyNameQ());
            }
        }
        VL_RESTORER(m_curSymp);
        VL_RESTORER(m_ftaskp);
        VL_RESTORER(m_explicitSuperNewp);
        {
            m_ftaskp = nodep;
            m_ds.m_dotSymp = m_curSymp = m_statep->get_गाँठ_Sym(nodep);
            स्थिर bool isNew = nodep->name() == "new";
            अगर (isNew) m_explicitSuperNewp = nullptr;
            iterateChildren(nodep);
            अगर (isNew) {
                स्थिर AstClassExtends* स्थिर classExtendsp = VN_AS(m_modp, Class)->extendsp();
                अगर (m_explicitSuperNewp && !m_explicitSuperNewp->isImplicit() && classExtendsp
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
                अगर (classExtendsp && classExtendsp->classOrNullp()) {
                    अगर (!m_explicitSuperNewp && m_statep->forParamed()
                        && !VN_AS(classExtendsp->childDTypep()->skipRefp(), ClassRefDType)
                                ->classp()
                                ->isInterfaceClass()) {
                        Ast_गाँठ_Stmt* स्थिर superNewp
                            = addImplicitSuperNewCall(VN_AS(nodep, Func), classExtendsp);
                        UINFO(9, "created super new " << superNewp);
                        iterate(superNewp);
                    }
                }
            }
        }
        m_ds.m_dotSymp = VL_RESTORER_पूर्व(m_curSymp);
    }
    व्योम visit(Ast_गाँठ_Foreach* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        symIterateChildren(nodep, m_statep->get_गाँठ_Sym(nodep));
    }
    व्योम visit(AstRandSequence* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        अगर (!nodep->start().empty())
            nodep->prodp(findProd(nodep, m_statep->get_गाँठ_Sym(nodep), nodep->start()));
        symIterateChildren(nodep, m_statep->get_गाँठ_Sym(nodep));
    }
    व्योम visit(AstRSProd* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        symIterateChildren(nodep, m_statep->get_गाँठ_Sym(nodep));
    }
    व्योम visit(AstRSProdItem* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        nodep->prodp(findProd(nodep, m_curSymp, nodep->name()));
        iterateChildren(nodep);
    }
    व्योम visit(AstCoverWith* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        iterate_And_अग्र_Null(nodep->subp());
        // The type of 'item' is that of the coverpoint expression, which may name 'item' too
        iterate_And_अग्र_Null(nodep->itemp()->childDTypep());
        VL_RESTORER(m_curSymp);
        m_ds.m_dotSymp = m_curSymp = m_statep->get_गाँठ_Sym(nodep);
        iterate_And_अग्र_Null(nodep->itemp());
        iterate_And_अग्र_Null(nodep->filterp());
        m_ds.m_dotSymp = VL_RESTORER_पूर्व(m_curSymp);
    }
    व्योम visit(AstCoverCrossWith* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        iterate_And_अग्र_Null(nodep->selectp());
        iterate_And_अग्र_Null(nodep->countp());
        // The type of a cross item is that of its coverpoint's expression, which may name items
        iterate_And_अग्र_Null(nodep->varsp());
        VL_RESTORER(m_curSymp);
        m_ds.m_dotSymp = m_curSymp = m_statep->get_गाँठ_Sym(nodep);
        iterate_And_अग्र_Null(nodep->filterp());
        m_ds.m_dotSymp = VL_RESTORER_पूर्व(m_curSymp);
    }
    व्योम visit(AstWith* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        अगर (m_ds.m_unresolvedClass) वापस;
        VL_RESTORER(m_curSymp);
        VL_RESTORER(m_currentWithp);
        VL_RESTORER_COPY(m_restrictedNamesUsed);
        {
            m_ds.m_dotSymp = m_curSymp = m_statep->get_गाँठ_Sym(nodep);
            m_currentWithp = nodep;
            // Validate the identifier_list once per AstWith (m_validated travels
            // with cloneTree, so cloned witnesses skip the redundant check).
            स्थिर bool firstVisit = !nodep->validated();
            अगर (firstVisit) {
                nodep->validated(true);
                m_restrictedNamesUsed.सफा();
                // IEEE 1800-2023 18.7: each name in the identifier_list must designate
                // a member of the randomize() target class.
                अगर (nodep->restricted() && m_randSymp) {
                    क्रम (स्थिर std::string& n : nodep->restrictedNames()) {
                        अगर (!m_randSymp->findIdFlat(n)) {
                            nodep->v3error("Identifier '"
                                           << n
                                           << "' in 'with (...)' identifier list is not a"
                                              " member of the randomize() target class"
                                              " (IEEE 1800-2023 18.7)");
                            m_restrictedNamesUsed.जोड़ो(n);
                        }
                    }
                }
            }
            iterateChildren(nodep);
            // Flag names that the user listed but never referenced in the constraint.
            अगर (firstVisit && nodep->restricted()) {
                क्रम (स्थिर std::string& n : nodep->restrictedNames()) {
                    अगर (!m_restrictedNamesUsed.गिनती(n)) {
                        nodep->v3warn(UNUSED, "Identifier '"
                                                  << n
                                                  << "' in 'with (...)' identifier list is"
                                                     " not referenced in the constraint"
                                                     " block");
                    }
                }
            }
        }
        m_ds.m_dotSymp = VL_RESTORER_पूर्व(m_curSymp);
    }
    व्योम visit(AstLambdaArgRef* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        // No checknodot(nodep), visit(AstScope) will check for LambdaArgRef
        iterateChildren(nodep);
    }
    व्योम visit(AstClassExtends* nodep) override {
        // Resolve the symbol and get the class.
        // If it is a parameterized case, the class will be resolved after V3Param.cpp
        अगर (nodep->user3SetOnce()) वापस;
        // If the class is resolved, there is nothing more to do
        अगर (nodep->classOrNullp()) वापस;
        LINKDOT_VISIT_START();
        अगर (m_statep->forPrimary()) {
            अगर (nodep->childDTypep()) वापस;
            Ast_गाँठ* cprp = nodep->classOrPkgsp();
            VSymEnt* lookSymp = m_curSymp;
            अगर (AstDot* स्थिर dotp = VN_CAST(cprp, Dot)) {
                dotp->user3(true);
                अगर (AstClassOrPackageRef* lookNodep = VN_CAST(dotp->lhsp(), ClassOrPackageRef)) {
                    iterate(lookNodep);
                    cprp = dotp->rhsp();
                    VSymEnt* स्थिर foundp = m_statep->resolveClassOrPackage(
                        lookSymp, lookNodep, true, false, nodep->verilogKwd());
                    अगर (!foundp) {
                        VL_DO_DANGLING(धकेलो_Deletep(nodep->unlinkFrBack()), nodep);
                        वापस;
                    }
                    UASSERT_OBJ(lookNodep->classOrPackageSkipp(), nodep, "Bad package link");
                    lookSymp = m_statep->get_गाँठ_Sym(lookNodep->classOrPackageSkipp());
                } अन्यथा {
                    dotp->lhsp()->v3error("Attempting to extend"  // LCOV_EXCL_LINE
                                          " using non-class under dot");
                }
            }
            AstClassOrPackageRef* स्थिर cpackagerefp = VN_CAST(cprp, ClassOrPackageRef);
            अगर (VL_UNCOVERABLE(!cpackagerefp)) {
                // Linking the extend gives an error before this is hit
                nodep->v3error("Attempting to extend using non-class");  // LCOV_EXCL_LINE
                VL_DO_DANGLING(धकेलो_Deletep(nodep->unlinkFrBack()), nodep);
                वापस;
            }
            VSymEnt* स्थिर foundp = m_statep->resolveClassOrPackage(lookSymp, cpackagerefp, true,
                                                                    true, nodep->verilogKwd());
            अगर (!foundp) {
                VL_DO_DANGLING(धकेलो_Deletep(nodep->unlinkFrBack()), nodep);
                वापस;
            }
            अगर (AstClass* स्थिर classp = VN_CAST(foundp->nodep(), Class)) {
                AstPin* paramsp = cpackagerefp->paramsp();
                अगर (paramsp) {
                    paramsp = paramsp->clone_वृक्ष(true);
                    nodep->parameterized(true);
                }
                nodep->childDTypep(नया AstClassRefDType{nodep->fileline(), classp, paramsp});
                // Link pins
                iterate(nodep->childDTypep());
            } अन्यथा अगर (AstParamTypeDType* स्थिर paramp
                       = VN_CAST(foundp->nodep(), ParamTypeDType)) {
                AstRefDType* स्थिर refParamp = नया AstRefDType{nodep->fileline(), paramp->name()};
                refParamp->refDTypep(paramp);
                nodep->childDTypep(refParamp);
                nodep->parameterized(true);
            } अन्यथा अगर (AstTypedef* स्थिर typedefp = VN_CAST(foundp->nodep(), Typedef)) {
                Ast_गाँठ_DType* स्थिर unwrappedp = typedefp->subDTypep()->skipRefp();
                अगर (AstClassRefDType* स्थिर classRefp = VN_CAST(unwrappedp, ClassRefDType)) {
                    AstPin* paramsp = cpackagerefp->paramsp();
                    अगर (!paramsp && classRefp->paramsp()) {
                        // No explicit #(...) on extends clause; carry over the
                        // typedef's type-parameter pins so V3Param sees them.
                        paramsp = classRefp->paramsp()->clone_वृक्ष(true);
                        nodep->parameterized(true);
                    } अन्यथा अगर (paramsp) {
                        paramsp = paramsp->clone_वृक्ष(true);
                        nodep->parameterized(true);
                    }
                    nodep->childDTypep(
                        नया AstClassRefDType{nodep->fileline(), classRefp->classp(), paramsp});
                    iterate(nodep->childDTypep());
                } अन्यथा {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: " << foundp->nodep()->prettyTypeName()
                                                  << " in 'class extends'");
                    वापस;
                }
            } अन्यथा {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: " << foundp->nodep()->prettyTypeName()
                                                             << " in 'class extends'");
                वापस;
            }
            अगर (!nodep->childDTypep()) nodep->v3error("Attempting to extend using non-class");
            nodep->classOrPkgsp()->unlinkFrBack()->delete_वृक्ष();
        } अन्यथा {
            // Probably a parameter
            अगर (AstRefDType* स्थिर refp = VN_CAST(nodep->childDTypep(), RefDType)) {
                अगर (AstClassRefDType* classRefp = VN_CAST(refp->skipRefp(), ClassRefDType)) {
                    // Resolved to a class reference.
                    refp->replaceWith(classRefp->clone_वृक्ष(false));
                    VL_DO_DANGLING(धकेलो_Deletep(refp), refp);
                } अन्यथा {
                    // Unable to resolve the ref type to a class reference.
                    // Get the value of type parameter passed to the class instance,
                    // to print the helpful error message.
                    स्थिर Ast_गाँठ_DType* typep = refp->refDTypep();
                    जबतक (true) {
                        अगर (स्थिर AstParamTypeDType* स्थिर atypep
                            = VN_CAST(typep, ParamTypeDType)) {
                            typep = atypep->subDTypep();
                            जारी;
                        }
                        अगर (स्थिर AstRequireDType* स्थिर atypep = VN_CAST(typep, RequireDType)) {
                            typep = atypep->subDTypep();
                            जारी;
                        }
                        अवरोध;
                    }
                    typep->v3error("Attempting to extend using non-class");
                }
            }
        }
    }
    व्योम visit(AstClass* nodep) override {
        अगर (nodep->user3SetOnce()) वापस;
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
            m_ifForwards.सफा();
            m_ds.init(m_curSymp);
            m_insideClassExtParam = nodep->isCovergroup() && nodep->covergroupEnclosingClassp()
                                    && m_extendsParam.find(nodep->covergroupEnclosingClassp())
                                           != m_extendsParam.end();
            // Until overridden by a SCOPE
            m_ds.m_dotSymp = m_curSymp = m_modSymp = m_statep->get_गाँठ_Sym(nodep);
            m_modp = nodep;
            पूर्णांक अग्र = 0;
            क्रम (AstClassExtends* cextp = nodep->extendsp(); cextp;
                 cextp = VN_AS(cextp->nextp(), ClassExtends)) {
                // Replace abstract reference with hard pointer
                // Will need later resolution when deal with parameters
                अगर (++अग्र == 2 && !nodep->isInterfaceClass() && !cextp->isImplements()) {
                    cextp->v3error("Multiple inheritance illegal on non-interface classes"
                                   " (IEEE 1800-2023 8.13)");
                }
                iterate(cextp);
                अगर (m_statep->forPrimary()) {
                    अगर (cextp->parameterized()) {
                        // Parameters in extends statement.
                        // The class can't be resolved in the current pass.
                        m_extendsParam.जोड़ो(nodep);
                        m_insideClassExtParam = true;
                    }
                    अगर (AstClassRefDType* स्थिर classRefp
                        = VN_CAST(cextp->childDTypep(), ClassRefDType)) {
                        AstClass* स्थिर classp = classRefp->classp();
                        अगर (classp != nodep) iterate(classp);
                        अगर (m_extendsParam.find(classp) != m_extendsParam.end()) {
                            // One of its super classes has parameters in extends statement.
                            // Some links may not be resolved in the first pass.
                            m_extendsParam.जोड़ो(nodep);
                            m_insideClassExtParam = true;
                        }
                    }
                }

                अगर (AstClass* स्थिर baseClassp = cextp->classOrNullp()) {
                    // Already converted. Update symbol table to link unlinked members.
                    // Base class has to be visited in a case if its extends statement
                    // needs to be handled. Recursive inheritance was already checked.
                    // Must be here instead of in LinkDotParam to handle
                    // "class (type T) extends T".
                    अगर (baseClassp == nodep) {
                        cextp->v3error("Attempting to extend class " << nodep->prettyNameQ()
                                                                     << " from itself");
                    } अन्यथा अगर (extendsClass(baseClassp, nodep)) {
                        cextp->v3error("Attempting to extend class "
                                       << nodep->prettyNameQ() << " from "
                                       << baseClassp->prettyNameQ()
                                       << ", which circularly inherits from it");
                    } अन्यथा अगर (cextp->isImplements() && !baseClassp->isInterfaceClass()) {
                        cextp->v3error("Attempting to implement from non-interface class "
                                       << baseClassp->prettyNameQ() << '\n'
                                       << "... Suggest use 'extends'");
                    } अन्यथा अगर (!cextp->isImplements() && !nodep->isInterfaceClass()
                               && baseClassp->isInterfaceClass()) {
                        cextp->v3error("Attempting to extend from interface class "
                                       << baseClassp->prettyNameQ() << '\n'
                                       << "... Suggest use 'implements'");
                    }
                    baseClassp->isExtended(true);
                    nodep->isExtended(true);
                    iterate(baseClassp);
                    importSymbolsFromExtended(nodep, cextp);
                    जारी;
                }
            }
            m_ds.m_dotSymp = m_curSymp;

            iterateChildren(nodep);
            क्रम (स्थिर स्वतः& itr : m_ifForwards) {
                स्थिर VSymEnt* स्थिर implSymp = itr.second;
                addInterfaceForward(nodep, VN_AS(implSymp->nodep(), गाँठ_FTask),
                                    VN_AS(implSymp->parentp()->nodep(), Class));
            }
        }
        // V3Width when determines types needs to find enum values and such
        // so add members pointing to appropriate enum values
        {
            VMemberMap memberMap;
            क्रम (VSymEnt::const_iterator it = m_curSymp->begin(); it != m_curSymp->end(); ++it) {
                Ast_गाँठ* स्थिर itemp = it->second->nodep();
                अगर (!memberMap.findMember(nodep, it->first)) {
                    अगर (AstEnumItem* स्थिर aitemp = VN_CAST(itemp, EnumItem)) {
                        AstEnumItemRef* स्थिर newp = नया AstEnumItemRef{
                            aitemp->fileline(), aitemp, it->second->classOrPackagep()};
                        UINFO(8, indent()
                                     << "Class import noderef '" << it->first << "' " << newp);
                        nodep->addMembersp(newp);
                        memberMap.जोड़ो(nodep, newp);
                    }
                }
            }
        }
        m_ds.m_dotSymp = VL_RESTORER_पूर्व(m_curSymp);
    }

    // Collect the ClassOrPackageRefs in a chained scope operand in source order.
    // The grammar builds a scope operand only from packageClassScopeItem, which
    // always yields a ClassOrPackageRef, nested in Dots, so nothing else appears.
    अटल व्योम collectScopeRefs(Ast_गाँठ* nodep, std::vector<AstClassOrPackageRef*>& refps) {
        अगर (AstClassOrPackageRef* स्थिर refp = VN_CAST(nodep, ClassOrPackageRef)) {
            refps.धकेलो_back(refp);
            वापस;
        }
        AstDot* स्थिर dotp = VN_CAST(nodep, Dot);
        UASSERT_OBJ(dotp, nodep, "Non-scope node in package/class scope operand");
        collectScopeRefs(dotp->lhsp(), refps);
        collectScopeRefs(dotp->rhsp(), refps);
    }

    // Resolve a chained class-scope operand (`pkg::outer::inner` in
    // `pkg::outer::inner::t`) and reduce it to its innermost ClassOrPackageRef.
    // Returns false, having reported, if a segment could not be resolved.
    bool reduceScopeDot(AstRefDType* nodep, AstDot* scopeDotp) {
        std::vector<AstClassOrPackageRef*> refps;
        collectScopeRefs(scopeDotp, refps);

        VSymEnt* scopeSymp = m_ds.m_dotSymp;
        क्रम (माप_प्रकार i = 0; i < refps.आकार(); ++i) {
            AstClassOrPackageRef* स्थिर refp = refps[i];
            अगर (!refp->classOrPackageSkipp() && !refp->classOrPackageNodep()
                && !m_statep->resolveClassOrPackage(scopeSymp, refp, i == 0, false,
                                                    "class/package reference")) {
                वापस false;  // Error already reported
            }
            अगर (i + 1 == refps.आकार()) अवरोध;
            // A middle segment that resolved to a node with no reachable module
            // (e.g. a typedef alias of a not-yet-specialized parameterized class)
            // gives nothing to look the next segment up in.
            Ast_गाँठ_Module* स्थिर modp = refp->classOrPackageSkipp();
            अगर (!modp) {
                refp->v3warn(E_UNSUPPORTED, "Unsupported: Multiple '::' package/class reference");
                वापस false;
            }
            scopeSymp = m_statep->get_गाँठ_Sym(modp);
        }
        AstClassOrPackageRef* स्थिर innerp = refps.back();
        innerp->unlinkFrBack();
        VL_DO_DANGLING(धकेलो_Deletep(scopeDotp->unlinkFrBack()), scopeDotp);
        nodep->classOrPackageOpp(innerp);
        वापस true;
    }

    व्योम visit(AstRefDType* nodep) override {

        अगर (स्वतः* स्थिर typeOfp = nodep->typeofp()) {
            iterate(typeOfp);
            // After iteration, typeofp() may have been replaced with a resolved type
            // (e.g., DOT expression like if0.rq_t resolved to REFDTYPE)
            // If it's now a NodeDType, replace this RefDType with it
            अगर (Ast_गाँठ_DType* स्थिर resolvedDTypep = VN_CAST(nodep->typeofp(), गाँठ_DType)) {
                UINFO(9,
                      indent() << "iface capture typeofp resolved to dtype, replacing RefDType\n");
                resolvedDTypep->unlinkFrBack();
                nodep->replaceWith(resolvedDTypep);
                VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
                // Originally (PR #6637) this block re-captured a resolved
                // RefDType into the iface-capture ledger when user2p held a
                // Cell, to ensure re-resolution during the paramed pass.
                // In practice, the typeofp-resolved RefDType never carries a
                // Cell in user2p: the capture happens earlier when the
                // typedef is first linked.
                अगर (V3LinkDotIfaceCapture::enabled()) {
                    अगर (AstRefDType* स्थिर resolvedRefp = VN_CAST(resolvedDTypep, RefDType)) {
                        अगर (VL_UNCOVERABLE(
                                VN_IS(resolvedRefp->user2p(), Cell))) {  // LCOV_EXCL_LINE
                            resolvedRefp->v3fatalSrc(  // LCOV_EXCL_LINE
                                "typeofp resolved RefDType has Cell in user2p;"
                                " expected to be captured already");
                        }
                    }
                }
            }
            वापस;
        }

        // Resolve its reference
        अगर (m_statep->forParamed() && nodep->user3()) {
            अगर (V3LinkDotIfaceCapture::enabled() && nodep->user2p()) {
                UINFO(9, indent() << "iface capture clear user3 for captured typedef name="
                                  << nodep->name() << " cell=" << nodep->user2p());
            }
            nodep->user3(false);
        }
        अगर (nodep->user3SetOnce()) {
            अगर (V3LinkDotIfaceCapture::enabled() && nodep->user2p()) {
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
            अगर (VL_UNCOVERABLE(m_statep->forParamed() && nodep->classOrPackageOpp())) {
                nodep->v3fatalSrc(  // LCOV_EXCL_LINE
                    "RefDType user3 revisit with classOrPackageOpp in paramed pass");
            }
            वापस;
        }
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        अगर (nodep->classOrPackageOpp()) {
            // Resolve a chained scope (`pkg::cls::t`, `pkg::cls#(P)::t`) from outside in.
            // On failure reduceScopeDot has reported, so stop before using the operand.
            अगर (AstDot* स्थिर scopeDotp = VN_CAST(nodep->classOrPackageOpp(), Dot)) {
                अगर (!reduceScopeDot(nodep, scopeDotp)) वापस;
            }
            // Re-read: reduceScopeDot may have replaced the operand
            Ast_गाँठ* स्थिर cpackagep = nodep->classOrPackageOpp();
            UASSERT_OBJ(!VN_IS(cpackagep, Dot), cpackagep,
                        "Package/class scope operand should be reduced by now");
            अगर (AstClassOrPackageRef* स्थिर cpackagerefp = VN_CAST(cpackagep, ClassOrPackageRef)) {
                iterate(cpackagerefp);
                स्थिर AstClass* स्थिर clsp = VN_CAST(cpackagerefp->classOrPackageNodep(), Class);
                अगर (clsp && clsp->hasGParam()) {
                    // Unable to link before the instantiation of parameter classes.
                    // The class reference node still has to be visited now to later link
                    // parameters.
                    iterate(cpackagep);
                    वापस;
                }
                // Defer non-typedef references through typedef aliases of parameterized classes.
                अगर (m_statep->forPrimary() && !VN_IS(nodep->backp(), Typedef)
                    && isParamedClassRef(cpackagerefp)) {
                    iterate(cpackagep);
                    वापस;
                }

                स्थिर bool doDefaultTypedef = !(m_resolvingTypedef && m_statep->forPrimary());
                अगर (!cpackagerefp->classOrPackageSkipp(doDefaultTypedef)
                    && !cpackagerefp->classOrPackageNodep()) {
                    VSymEnt* स्थिर foundp = m_statep->resolveClassOrPackage(
                        m_ds.m_dotSymp, cpackagerefp, true, false, "class/package reference");
                    अगर (!foundp) वापस;
                }
                nodep->classOrPackagep(cpackagerefp->classOrPackageSkipp(doDefaultTypedef));
                अगर (!VN_IS(nodep->classOrPackagep(), Class)
                    && !VN_IS(nodep->classOrPackagep(), Package)) {
                    अगर (m_statep->forPrimary()) {
                        // It may be a type that comes from parameter class that is not
                        // instantioned yet
                        iterate(cpackagep);
                        वापस;
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
            VL_DO_DANGLING(धकेलो_Deletep(cpackagep->unlinkFrBack()), cpackagep);
        }

        अगर (m_ds.m_dotp && (m_ds.m_dotPos == DP_PACKAGE || m_ds.m_dotPos == DP_SCOPE)) {
            UASSERT_OBJ(VN_IS(m_ds.m_dotp->lhsp(), ClassOrPackageRef), m_ds.m_dotp->lhsp(),
                        "Bad package link");
            स्वतः* स्थिर cpackagerefp = VN_CAST(m_ds.m_dotp->lhsp(), ClassOrPackageRef);
            UASSERT_OBJ(cpackagerefp->classOrPackageSkipp(), m_ds.m_dotp->lhsp(),
                        "Bad package link");
            nodep->classOrPackagep(cpackagerefp->classOrPackageSkipp());
            m_ds.m_dotPos = DP_SCOPE;
        } अन्यथा {
            // Allow REFDTYPE under DOT when referencing interface typedefs
            // This is needed for patterns like: typedef iface.a_t a_t;
            // The dependency graph handles resolution when IfaceCapture is disabled
            स्थिर bool ifaceFinalSegmentAllowed
                = (m_ds.m_dotPos == DP_FINAL) && m_ds.m_dotSymp
                  && VN_IS(m_ds.m_dotSymp->nodep(), Cell)
                  && VN_CAST(m_ds.m_dotSymp->nodep(), Cell)->modp()
                  && VN_IS(VN_CAST(m_ds.m_dotSymp->nodep(), Cell)->modp(), Iface);
            अगर (!ifaceFinalSegmentAllowed) {
                checkNoDot(nodep);
            } अन्यथा {
                UINFO(9, indent() << "allow REFDTYPE under DOT for iface typedef name="
                                  << nodep->prettyNameQ() << " dotSym=" << m_ds.m_dotSymp);
                // Clear the dot state so we don't propagate errors
                m_ds.m_dotPos = DP_SCOPE;
            }
        }
        अगर (!nodep->typedefp() && !nodep->subDTypep()) {
            स्थिर VSymEnt* foundp;
            अगर (nodep->classOrPackagep()) {
                foundp = m_statep->get_गाँठ_Sym(nodep->classOrPackagep())->findIdFlat(nodep->name());
            } अन्यथा अगर (m_ds.m_dotPos == DP_FIRST || m_ds.m_dotPos == DP_NONE) {
                foundp = findIdFallbackSkipMemberDType(m_curSymp, nodep->name());
            } अन्यथा {
                // Defensive: dotPos should be DP_FIRST/DP_NONE or classOrPackagep set.
                v3fatalSrc("Unexpected dotPos="
                           << static_cast<पूर्णांक>(m_ds.m_dotPos)  // LCOV_EXCL_LINE
                           << " in RefDType lookup for "
                           << nodep->prettyNameQ());  // LCOV_EXCL_LINE
                foundp = nullptr;  // LCOV_EXCL_LINE
            }
            {
                अगर (AstTypedef* स्थिर defp
                    = foundp ? VN_CAST(foundp->nodep(), Typedef) : nullptr) {
                    // Don't check if typedef is to a <type T>::<reference> as might not be
                    // resolved yet
                    अगर (!nodep->classOrPackagep() && !defp->isUnderClass())
                        checkDeclOrder(nodep, defp);
                    nodep->typedefp(defp);
                    nodep->classOrPackagep(foundp->classOrPackagep());

                } अन्यथा अगर (AstParamTypeDType* स्थिर defp
                           = foundp ? VN_CAST(foundp->nodep(), ParamTypeDType) : nullptr) {
                    // A PARAMTYPEDTYPE's child REFDTYPE referencing itself is the normal
                    // AST structure for type parameters (e.g., "type T = base" has a child
                    // REFDTYPE for the default type). Only error on true recursion where
                    // a TYPEDEF contains a reference back to itself.
                    स्थिर bool isParamTypeChild = (defp == nodep->backp());
                    स्थिर bool isTypedefRecursion
                        = isParamTypeChild && VN_IS(defp->backp(), Typedef);
                    अगर (isTypedefRecursion) {
                        nodep->v3error("Reference to '"
                                       << m_ds.m_dotText << (m_ds.m_dotText == "" ? "" : ".")
                                       << nodep->prettyName() << "'"
                                       << " type would form a recursive definition");
                        nodep->refDTypep(nodep->findVoidDType());  // Try to reduce later errors
                    } अन्यथा {
                        nodep->refDTypep(defp);
                        nodep->classOrPackagep(foundp->classOrPackagep());
                        captureIfaceParamType(nodep, defp);
                    }
                } अन्यथा अगर (AstClass* स्थिर defp
                           = foundp ? VN_CAST(foundp->nodep(), Class) : nullptr) {
                    // Don't check if typedef is to a <type T>::<reference> as might not be
                    // resolved yet
                    अगर (!nodep->classOrPackagep()) checkDeclOrder(nodep, defp);
                    AstPin* स्थिर paramsp = nodep->paramsp();
                    अगर (paramsp) paramsp->unlink_Fr_Back_With_अग्र();
                    AstClassRefDType* स्थिर newp
                        = नया AstClassRefDType{nodep->fileline(), defp, paramsp};
                    newp->classOrPackagep(foundp->classOrPackagep());
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
                    वापस;
                } अन्यथा अगर (m_insideClassExtParam) {
                    वापस;
                } अन्यथा {
                    अगर (foundp) {
                        UINFO(1, "Found sym node: " << foundp->nodep());
                        nodep->v3error("Expecting a data type: "
                                       << nodep->prettyNameQ()
                                       << ", found: " << foundp->nodep()->prettyTypeName());
                    } अन्यथा {
                        nodep->v3error("Can't find typedef/interface: " << nodep->prettyNameQ());
                    }
                }
            }
        }
        iterateChildren(nodep);
    }
    व्योम visit(AstRequireDType* nodep) override {
        // Handle type validation for localparam type assignments
        // Iterate child to resolve any ParseRef nodes
        iterateChildren(nodep);
        // Only emit error if the child is not a type.
        // Do NOT unwrap valid types here - leave that to V3Width.
        // Unwrapping here breaks type parameter resolution during cloning.
        // Allow Dot through to defer the handling until the dot expression is resolved.
        अगर (nodep->lhsp() && !VN_IS(nodep->lhsp(), गाँठ_DType) && !VN_IS(nodep->lhsp(), Dot)) {
            // Not a type - emit error
            अगर (VN_IS(nodep->lhsp(), Const)) {
                nodep->lhsp()->v3error("Expecting a data type, not a constant");
            } अन्यथा {
                nodep->lhsp()->v3error("Expecting a data type, not "
                                       << nodep->lhsp()->typeName() << ": '"
                                       << nodep->lhsp()->prettyName() << "'");
            }
            // Replace with void to keep AST valid and allow clean exit
            nodep->replaceWith(नया AstVoidDType{nodep->fileline()});
            VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
        }
    }
    व्योम visit(AstDpiExport* nodep) override {
        // AstDpiExport: Make sure the function referenced exists, then dump it
        LINKDOT_VISIT_START();
        iterateChildren(nodep);
        checkNoDot(nodep);
        VSymEnt* स्थिर foundp = m_curSymp->findIdFallback(nodep->name());
        Ast_गाँठ_FTask* स्थिर taskp = foundp ? VN_AS(foundp->nodep(), गाँठ_FTask) : nullptr;
        अगर (!taskp) {
            nodep->v3error(
                "Can't find definition of exported task/function: " << nodep->prettyNameQ());
        } अन्यथा अगर (taskp->dpiExport()) {
            nodep->v3error("Function was already DPI Exported, duplicate not allowed: "
                           << nodep->prettyNameQ());
        } अन्यथा {
            taskp->dpiExport(true);
            अगर (nodep->cname() != "") taskp->cname(nodep->cname());
        }
        VL_DO_DANGLING(धकेलो_Deletep(nodep->unlinkFrBack()), nodep);
    }
    व्योम visit(AstDisable* nodep) override {
        LINKDOT_VISIT_START();
        checkNoDot(nodep);
        bool hasPartSelect = false;
        स्थिर string targetPath
            = nodep->targetRefp() ? extractDottedPath(nodep->targetRefp(), hasPartSelect) : "";
        VL_RESTORER_COPY(m_ds);
        m_ds.init(m_curSymp);
        m_ds.m_dotPos = DP_FIRST;
        m_ds.m_disablep = nodep;
        iterateChildren(nodep);
        अगर (nodep->targetRefp()) {
            अगर (AstTaskRef* स्थिर taskRefp = VN_CAST(nodep->targetRefp(), TaskRef)) {
                nodep->targetp(taskRefp->taskp());
            } अन्यथा अगर (!VN_IS(nodep->targetRefp(), ParseRef)) {
                // If it is a ParseRef, either it couldn't be linked or it is linked to a block
                nodep->v3warn(E_UNSUPPORTED, "Node of type "
                                                 << nodep->targetRefp()->prettyTypeName()
                                                 << " referenced by disable");
                VL_DO_DANGLING(धकेलो_Deletep(nodep->unlinkFrBack()), nodep);
                वापस;
            }
            अगर (nodep->targetp()) {
                nodep->targetRefp()->unlinkFrBack()->delete_वृक्ष();
                अगर (!hasPartSelect) {
                    स्थिर string instancePath = targetInstancePath(nodep->targetp(), targetPath);
                    अगर (!instancePath.empty()) {
                        // Keep only the instance prefix so V3LinkJump can reference the selected
                        // instance's process queue without reparsing the disable target.
                        nodep->targetRefp(
                            नया AstVarXRef{nodep->fileline(), "", instancePath, VAccess::READ});
                    }
                }
            }
        }
    }
    व्योम visitPackageImportOrExport(Ast_गाँठ* nodep) {
        // No longer needed
        LINKDOT_VISIT_START();
        checkNoDot(nodep);
        अगर (m_statep->forParamed()) VL_DO_DANGLING(धकेलो_Deletep(nodep->unlinkFrBack()), nodep);
    }
    व्योम visit(AstPackageImport* nodep) override { visitPackageImportOrExport(nodep); }
    व्योम visit(AstPackageExport* nodep) override { visitPackageImportOrExport(nodep); }
    व्योम visit(AstPackageExportStarStar* nodep) override { visitPackageImportOrExport(nodep); }
    व्योम visit(AstCellRef* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        iterateChildren(nodep);
    }
    व्योम visit(AstCellArrayRef* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        // Expression already iterated
    }
    व्योम visit(AstUnlinkedRef* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        // No need to iterate, if we have a UnlinkedVarXRef, we're already done
    }
    व्योम visit(Ast_Stmt_व्यंजक* nodep) override {
        LINKDOT_VISIT_START();
        checkNoDot(nodep);
        // Check if nodep represents a super.new call;
        अगर (AstNew* स्थिर newExprp = VN_CAST(nodep->exprp(), New)) {
            // in this case it was already linked, so it doesn't have a super reference
            m_explicitSuperNewp = newExprp;
        } अन्यथा अगर (स्थिर AstDot* स्थिर dotp = VN_CAST(nodep->exprp(), Dot)) {
            अगर (dotp->lhsp()->name() == "super") {
                अगर (AstNew* स्थिर newExprp = VN_CAST(dotp->rhsp(), New)) {
                    m_explicitSuperNewp = newExprp;
                }
            }
        }
        iterateChildren(nodep);
    }

    व्योम visit(AstIfaceRefDType* nodep) override {
        अगर (nodep->user3SetOnce()) वापस;
        LINKDOT_VISIT_START();
        अगर (nodep->paramsp()) {
            // If there is no parameters, there is no need to visit this node.
            AstIface* स्थिर ifacep = nodep->ifacep();
            UASSERT_OBJ(ifacep, nodep, "Port parameters of AstIfaceRefDType without ifacep()");
            अगर (ifacep->dead()) वापस;
            checkNoDot(nodep);
            VL_RESTORER_CLEAR(m_usedPins);
            VL_RESTORER_CLEAR(m_usedDefParamPins);
            VL_RESTORER(m_pinSymp);
            m_pinSymp = m_statep->get_गाँठ_Sym(ifacep);
            iterate_And_अग्र_Null(nodep->paramsp());
        }
    }

    व्योम visit(AstAttrOf* nodep) override { iterateChildren(nodep); }
    व्योम visit(AstCastParse* nodep) override {
        अगर (nodep->user3SetOnce()) वापस;
        LINKDOT_VISIT_START();
        // A cast may start a dotted expression: HERE type'(expr) . member_or_method
        स्थिर bool dotLhs = m_ds.m_dotPos == DP_FIRST;
        अगर (!dotLhs) checkNoDot(nodep);
        {
            VL_RESTORER_COPY(m_ds);
            m_ds.init(m_curSymp);
            iterateChildren(nodep);
        }
        अगर (dotLhs) m_ds.m_dotPos = DP_MEMBER;
    }

    व्योम visit(AstAssignW* nodep) override {
        LINKDOT_VISIT_START();
        checkNoDot(nodep);
        VL_RESTORER(m_replaceWithAlias);
        अगर (nodep->user2()) m_replaceWithAlias = false;
        iterateChildren(nodep);
    }

    व्योम visit(AstTypedef* nodep) override {
        VL_RESTORER(m_resolvingTypedef)
        m_resolvingTypedef = true;
        iterateChildren(nodep);
    }

    व्योम visit(Ast_गाँठ* nodep) override {
        VL_RESTORER(m_inPackedArray);
        अगर (VN_IS(nodep, PackArrayDType)) {
            m_inPackedArray = true;
        } अन्यथा अगर (!m_inPackedArray) {
            LINKDOT_VISIT_START();
            checkNoDot(nodep);
        }
        iterateChildren(nodep);
    }

खुला:
    // CONSTRUCTORS
    LinkDotResolveVisitor(AstNetlist* rootp, LinkDotState* statep)
        : m_statep{statep} {
        UINFO(4, __FUNCTION__ << ": ");

        // Note: no need to clear user3 on ledger entries here.
        // VNUser3InUse (m_inuser3) already logically clears user3 on all
        // nodes via the generation counter.  The ledger may also hold stale
        // refp pointers to deleted AST nodes, so iterating it is unsafe.

        iterate(rootp);
        स्थिर std::map<std::string, Ast_गाँठ_Module*> modulesToRevisit
            = std::move(m_modulesToRevisit);
        m_lastDeferredp = nullptr;
        क्रम (स्वतः& p : modulesToRevisit) {
            Ast_गाँठ_Module* स्थिर modp = p.second;
            modp->foreach([](Ast_गाँठ* स्थिर nodep) { nodep->user3(false); });
            iterate(modp);
        }
    }
    ~LinkDotResolveVisitor() override = शेष;
};

//######################################################################
// Link class functions

व्योम V3LinkDot::dumpSubstep(स्थिर string& name) {
    अगर (dump_वृक्ष_Either_Level() >= 9) {
        V3Global::dump_Check_Global_वृक्ष(name);
    } अन्यथा अगर (debug() >= 5) {  // on high dbg level, dump even if not explicitly told to
        v3Global.rootp()->dump_वृक्ष_File(v3Global.debugFilename(name + ".tree"));
    }
}

व्योम V3LinkDot::linkDotGuts(AstNetlist* rootp, VLinkDotStep step) {
    VIsCached::clear_Cache_वृक्ष();  // Avoid using any stale isPure
    dumpSubstep("prelinkdot");
    LinkDotState state{rootp, step};

    { LinkDotFindVisitor{rootp, &state}; }
    dumpSubstep("prelinkdot-find");

    { LinkDotFindIfaceVisitor{rootp, &state}; }
    dumpSubstep("prelinkdot-findiface");

    अगर (step == LDS_SCOPED) {
        // Well after the initial link when we're ready to operate on the flat design,
        // process AstScope's.  This needs to be separate pass after whole hierarchy graph created.
        { LinkDotScopeVisitor{rootp, &state}; }
        v3Global.assertScoped(true);
        dumpSubstep("prelinkdot-scoped");
    } अन्यथा {
        // Initial link stage, resolve parameters and interfaces
        { LinkDotParamVisitor{rootp, &state}; }
        dumpSubstep("prelinkdot-param");
    }
    state.dumpSelf("prelinkdot");
    state.computeIfaceModSyms();
    state.computeIfaceVarSyms();
    state.computeScopeAliases();
    state.dumpSelf("linkdot-preresolve");
    स्थिर LinkDotResolveVisitor visitor{rootp, &state};
    state.dumpSelf("linkdot-done");
}

व्योम V3LinkDot::linkDotPrimary(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    linkDotGuts(nodep, LDS_PRIMARY);
    V3Global::dump_Check_Global_वृक्ष("linkdot", 0, dump_वृक्ष_Either_Level() >= 6);
}

व्योम V3LinkDot::linkDotParamed(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    linkDotGuts(nodep, LDS_PARAMED);
    V3Global::dump_Check_Global_वृक्ष("linkdotparam", 0, dump_वृक्ष_Either_Level() >= 3);
}

व्योम V3LinkDot::linkDotScope(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    linkDotGuts(nodep, LDS_SCOPED);
    V3Global::dump_Check_Global_वृक्ष("linkdot", 0, dump_वृक्ष_Either_Level() >= 3);
}
