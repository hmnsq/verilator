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
//ટોચ
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

#અન્તર્ભાવ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#અન્તર્ભાવ "V3LinkDot.h"

#અન્તર્ભાવ "V3Global.h"
#અન્તર્ભાવ "V3Graph.h"
#અન્તર્ભાવ "V3LinkDotIfaceCapture.h"
#અન્તર્ભાવ "V3MemberMap.h"
#અન્તર્ભાવ "V3Parse.h"
#અન્તર્ભાવ "V3Randomize.h"
#અન્તર્ભાવ "V3Stats.h"
#અન્તર્ભાવ "V3String.h"
#અન્તર્ભાવ "V3SymTable.h"

#અન્તર્ભાવ <algorithm>
#અન્તર્ભાવ <cstdlib>
#અન્તર્ભાવ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

સ્થિર string extractDottedPath(Ast_ગાંઠ* nodep, તાર્કિક& hasPartSelect) {
    જો (AstParseRef* અચલ refp = VN_CAST(nodep, ParseRef)) {
        પાછા refp->name();
    } નહીંતર જો (AstVarRef* અચલ refp = VN_CAST(nodep, VarRef)) {
        પાછા refp->name();
    } નહીંતર જો (AstDot* અચલ dotp = VN_CAST(nodep, Dot)) {
        અચલ string lhs = extractDottedPath(dotp->lhsp(), hasPartSelect);
        અચલ string rhs = extractDottedPath(dotp->rhsp(), hasPartSelect);
        પાછા VString::dot(lhs, ".", rhs);
    } નહીંતર જો (VN_IS(nodep, SelBit) || VN_IS(nodep, SelExtract) || VN_IS(nodep, SelPlus)
               || VN_IS(nodep, SelMinus)) {
        hasPartSelect = સાચું;
        પાછા "";
    }
    પાછા "";
}
સ્થિર string lexicalDisablePath(Ast_ગાંઠ* અચલ targetp) {
    std::vector<string> names;
    માટે (Ast_ગાંઠ* curp = targetp; curp; curp = curp->aboveLoopp()) {
        જો (Ast_ગાંઠ_Block* અચલ blockp = VN_CAST(curp, ગાંઠ_Block)) {
            જો (blockp->name() != "") names.ધકેલો_back(blockp->name());
        } નહીંતર જો (Ast_ગાંઠ_FTask* અચલ ftaskp = VN_CAST(curp, ગાંઠ_FTask)) {
            names.ધકેલો_back(ftaskp->name());
        } નહીંતર જો (VN_IS(curp, ગાંઠ_Module)) {
            તોડો;
        }
    }
    string path;
    માટે (auto it = names.crbegin(); it != names.crend(); ++it) {
        path = VString::dot(path, ".", *it);
    }
    પાછા path;
}
સ્થિર string targetInstancePath(Ast_ગાંઠ* અચલ targetp, અચલ string& targetPath) {
    અચલ string lexicalPath = lexicalDisablePath(targetp);
    જો (lexicalPath == "" || targetPath == lexicalPath) પાછા "";
    અચલ string suffix = "." + lexicalPath;
    જો (!VString::endsWith(targetPath, suffix)) પાછા "";
    પાછા targetPath.substr(0, targetPath.size() - suffix.size());
}

// ######################################################################
//  Matcher classes (for suggestion matching)

વર્ગ Link_ગાંઠ_Matcher_Class_Or_Package final : જાહેર VNodeMatcher {
જાહેર:
    તાર્કિક ગાંઠ_Match(અચલ Ast_ગાંઠ* nodep) અચલ override {
        પાછા VN_IS(nodep, Class) || VN_IS(nodep, Package);
    }
};
વર્ગ Link_ગાંઠ_Matcher_FTask final : જાહેર VNodeMatcher {
જાહેર:
    તાર્કિક ગાંઠ_Match(અચલ Ast_ગાંઠ* nodep) અચલ override { પાછા VN_IS(nodep, ગાંઠ_FTask); }
};
વર્ગ Link_ગાંઠ_Matcher_Modport final : જાહેર VNodeMatcher {
જાહેર:
    તાર્કિક ગાંઠ_Match(અચલ Ast_ગાંઠ* nodep) અચલ override { પાછા VN_IS(nodep, Modport); }
};
વર્ગ Link_ગાંઠ_Matcher_Prod final : જાહેર VNodeMatcher {
જાહેર:
    તાર્કિક ગાંઠ_Match(અચલ Ast_ગાંઠ* nodep) અચલ override { પાછા VN_IS(nodep, RSProd); }
};
વર્ગ Link_ગાંઠ_Matcher_Var final : જાહેર VNodeMatcher {
જાહેર:
    તાર્કિક ગાંઠ_Match(અચલ Ast_ગાંઠ* nodep) અચલ override {
        પાછા VN_IS(nodep, Var) || VN_IS(nodep, LambdaArgRef);
    }
};
વર્ગ Link_ગાંઠ_Matcher_Var_IO final : જાહેર VNodeMatcher {
જાહેર:
    તાર્કિક ગાંઠ_Match(અચલ Ast_ગાંઠ* nodep) અચલ override {
        અચલ AstVar* અચલ varp = VN_CAST(nodep, Var);
        જો (!varp) પાછા ખોટું;
        પાછા varp->isIO();
    }
};
વર્ગ Link_ગાંઠ_Matcher_Var_Param final : જાહેર VNodeMatcher {
જાહેર:
    તાર્કિક ગાંઠ_Match(અચલ Ast_ગાંઠ* nodep) અચલ override {
        અચલ AstVar* અચલ varp = VN_CAST(nodep, Var);
        જો (!varp) પાછા ખોટું;
        પાછા varp->isParam();
    }
};

//######################################################################
// LinkDot state, as a visitor of each AstNode

વર્ગ LinkDotState final {
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
    અચલ VNUser1InUse m_inuser1;
    અચલ VNUser2InUse m_inuser2;
    અચલ VNUser4InUse m_inuser4;

જાહેર:
    // ENUMS
    // In order of priority, compute first ... compute last
    ગણના SAMNum : uint8_t { SAMN_MODPORT, SAMN_IFTOP, SAMN__MAX };  // Values for m_scopeAliasMap

ખાનગી:
    // TYPES
    using ScopeAliasMap = std::unordered_map<VSymEnt*, VSymEnt*>;
    using IfaceModSyms = std::vector<std::pair<AstIface*, VSymEnt*>>;

    સ્થિર LinkDotState* s_errorThisp;  // Last self, for error reporting only

    // MEMBERS
    VSymGraph m_syms;  // Symbol table by hierarchy
    VSymGraph m_mods;  // Symbol table of all module names
    VSymEnt* m_dunitEntp = nullptr;  // $unit entry
    std::multimap<std::string, VSymEnt*>
        m_nameScopeSymMap;  // Map of scope referenced by non-pretty textual name
    std::set<std::pair<Ast_ગાંઠ_Module*, std::string>>
        m_implicitNameSet;  // For [module][signalname] if we can implicitly create it
    std::array<ScopeAliasMap, SAMN__MAX> m_scopeAliasMap;  // Map of <lhs,rhs> aliases
    std::vector<VSymEnt*> m_ifaceVarSyms;  // List of AstIfaceRefDType's to be imported
    IfaceModSyms m_ifaceModSyms;  // List of AstIface+Symbols to be processed
    અચલ VLinkDotStep m_step;  // Operational step

જાહેર:
    // METHODS

    શૂન્ય dumpSelf(અચલ string& nameComment, તાર્કિક force = ખોટું) {
        જો (debug() >= 6 || dumpLevel() >= 6 || force) {
            અચલ string filename = v3Global.debugFilename(nameComment) + ".txt";
            UINFO(4, "Dumping " << filename);
            અચલ std::unique_ptr<std::ofstream> logp{V3File::new_ofstream(filename)};
            જો (logp->fail()) v3fatal("Can't write file: " << filename);
            std::ostream& os = *logp;
            // TODO the symbol table has node pointers which may be
            // dangling, as we call deleteTree in these visitors without
            // pushing for later deletion (or deleting from the symbol table)
            // So, only completely safe to call this when under --debug or --debug-leak.
            m_syms.dumpSelf(os);
            તાર્કિક first = સાચું;
            માટે (પૂર્ણાંક samn = 0; samn < SAMN__MAX; ++samn) {
                જો (!m_scopeAliasMap[samn].empty()) {
                    જો (first) os << "\nScopeAliasMap:\n";
                    first = ખોટું;
                    માટે (ScopeAliasMap::iterator it = m_scopeAliasMap[samn].begin();
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
    સ્થિર શૂન્ય preErrorDumpHandler() {
        જો (s_errorThisp) s_errorThisp->preErrorDump();
    }
    શૂન્ય preErrorDump() {
        સ્થિર તાર્કિક s_diddump = ખોટું;
        જો (!s_diddump && dump_વૃક્ષ_Level()) {
            s_diddump = સાચું;
            dumpSelf("linkdot-preerr", સાચું);
            v3Global.rootp()->dump_વૃક્ષ_File(v3Global.debugFilename("linkdot-preerr.tree"));
        }
    }

    // CONSTRUCTORS
    LinkDotState(AstNetlist* rootp, VLinkDotStep step)
        : m_syms{rootp}
        , m_mods{rootp}
        , m_step{step} {
        UINFO(4, __FUNCTION__ << ": ");
        s_errorThisp = આ;
        V3Error::errorExitCb(preErrorDumpHandler);  // If get error, dump self
        readModNames();
    }
    ~LinkDotState() {
        V3Error::errorExitCb(nullptr);
        s_errorThisp = nullptr;
    }

    // ACCESSORS
    VSymGraph* symsp() { પાછા &m_syms; }
    પૂર્ણાંક step_સંખ્યા() અચલ { પાછા static_cast<પૂર્ણાંક>(m_step); }
    તાર્કિક forPrimary() અચલ { પાછા m_step == LDS_PRIMARY; }
    તાર્કિક forParamed() અચલ { પાછા m_step == LDS_PARAMED; }
    તાર્કિક forScopeCreation() અચલ { પાછા m_step == LDS_SCOPED; }

    // METHODS
    સ્થિર string ગાંઠ_Text_Type(Ast_ગાંઠ* nodep) {
        જો (અચલ AstVar* અચલ varp = VN_CAST(nodep, Var)) {
            જો (varp->isIO() || varp->isIfaceRef()) {
                પાછા "port";
            } નહીંતર જો (varp->isGParam()) {
                પાછા "parameter";
            } નહીંતર જો (varp->varType() == VVarType::LPARAM) {
                પાછા "local parameter";
            } નહીંતર {
                પાછા "variable";
            }
        } નહીંતર જો (અચલ AstParamTypeDType* અચલ dtypep = VN_CAST(nodep, ParamTypeDType)) {
            જો (dtypep->isGParam()) {
                પાછા "type parameter";
            } નહીંતર {
                પાછા "local type parameter";
            }
        } નહીંતર જો (VN_IS(nodep, Begin)) {
            પાછા "block";
        } નહીંતર જો (VN_IS(nodep, Cell)) {
            પાછા "instance";
        } નહીંતર જો (VN_IS(nodep, Constraint)) {
            પાછા "constraint";
        } નહીંતર જો (VN_IS(nodep, Func)) {
            પાછા "function";
        } નહીંતર જો (VN_IS(nodep, GenBlock)) {
            પાછા "generate block";
        } નહીંતર જો (VN_IS(nodep, Iface)) {
            પાછા "interface";
        } નહીંતર જો (VN_IS(nodep, RSProd)) {
            પાછા "randsequence production";
        } નહીંતર જો (VN_IS(nodep, Task)) {
            પાછા "task";
        } નહીંતર {
            પાછા nodep->prettyTypeName();
        }
    }

    Ast_ગાંઠ_Module* findModuleSym(અચલ string& modName) {
        અચલ VSymEnt* અચલ foundp = m_mods.rootp()->findIdFallback(modName);
        પાછા foundp ? VN_AS(foundp->nodep(), ગાંઠ_Module) : nullptr;
    }
    શૂન્ય readModNames() {
        // Look at all modules, and store pointers to all module names
        માટે (Ast_ગાંઠ_Module *nextp, *nodep = v3Global.rootp()->modulesp(); nodep; nodep = nextp) {
            nextp = VN_AS(nodep->nextp(), ગાંઠ_Module);
            m_mods.rootp()->ઉમેરો(nodep->name(), નવું VSymEnt{&m_mods, nodep});
        }
    }

    VSymEnt* મૂળ_Entp() અચલ { પાછા m_syms.rootp(); }
    VSymEnt* dunitEntp() અચલ { પાછા m_dunitEntp; }
    શૂન્ય checkDuplicate(VSymEnt* lookupSymp, Ast_ગાંઠ* nodep, અચલ string& name) {
        // Lookup the given name under current symbol table
        // Insert if not found
        // Report error if there's a duplicate
        //
        // Note we only check for conflicts at the same level; it's ok if one block hides another
        // We also wouldn't want to not insert it even though it's lower down

        અચલ VSymEnt* અચલ foundp = lookupSymp->findIdFlat(name);
        Ast_ગાંઠ* અચલ fnodep = foundp ? foundp->nodep() : nullptr;
        જો (!fnodep) {
            // Not found, will add in a moment.
            જો (forPrimary() && VSymEnt::checkSimilarname(nodep)) {
                અચલ VSymEnt* અચલ alt = lookupSymp->findSimilarIdFlat(name);
                જો (alt) {
                    nodep->v3warn(SIMILARNAME, "Declaration overlaps another with different case: "
                                                   << nodep->prettyNameQ() << '\n'
                                                   << nodep->warnContextPrimary() << '\n'
                                                   << alt->nodep()->warnOther()
                                                   << "... Location of original declaration\n"
                                                   << alt->nodep()->warnContextSecondary());
                }
            }
        } નહીંતર જો (nodep == fnodep) {  // Already inserted.
            // Good.
        } નહીંતર જો (foundp->imported()) {  // From package
            // We don't throw VARHIDDEN as if the import is later the symbol
            // table's import wouldn't warn
        } નહીંતર જો (forPrimary() && VN_IS(nodep, GenBlock)
                   && (VN_IS(fnodep, Begin) || VN_IS(fnodep, GenBlock))) {
            // Begin: ... blocks often replicate under genif/genfor, so
            // suppress duplicate checks.  See t_gen_forif.v for an example.
        } નહીંતર {
            UINFO(4, "name " << name);  // Not always same as nodep->name
            UINFO(4, "Var1 " << nodep);
            UINFO(4, "Var2 " << fnodep);
            // Quote the node's own name as for messages, if it is the symbol's
            અચલ string nameQ
                = fnodep->name() == name ? fnodep->prettyNameQ() : Ast_ગાંઠ::prettyNameQ(name);
            જો (nodep->type() == fnodep->type()) {
                nodep->v3error("Duplicate declaration of "
                               << ગાંઠ_Text_Type(fnodep) << ": " << nameQ << '\n'
                               << nodep->warnContextPrimary() << '\n'
                               << fnodep->warnOther() << "... Location of original declaration\n"
                               << fnodep->warnContextSecondary());
                nodep->decl_Token_અંક_Set_Min(0);  // Disable checking forward typedefs
                fnodep->decl_Token_અંક_Set_Min(0);  // Disable checking forward typedefs
            } નહીંતર {
                nodep->v3error("Unsupported in C: "
                               << ucfirst(ગાંઠ_Text_Type(nodep)) << " has the same name as "
                               << ગાંઠ_Text_Type(fnodep) << ": " << nameQ << '\n'
                               << nodep->warnContextPrimary() << '\n'
                               << fnodep->warnOther() << "... Location of original declaration\n"
                               << fnodep->warnContextSecondary());
            }
        }
    }
    શૂન્ય ઉમેરો_DUnit(AstNetlist* nodep) {
        // $unit on top scope
        VSymEnt* અચલ symp = નવું VSymEnt{&m_syms, nodep};
        UINFO(9, "      INSERTdunit se" << cvtToHex(symp));
        symp->parentp(મૂળ_Entp());  // Needed so backward search can find name of top module
        symp->fallbackp(nullptr);
        મૂળ_Entp()->ઉમેરો("$unit ", symp);  // Space so can never name conflict with user code
        //
        UASSERT_OBJ(!m_dunitEntp, nodep, "Call insertDUnit only once");
        m_dunitEntp = symp;
    }
    VSymEnt* ઉમેરો_ટોચ_Cell(Ast_ગાંઠ_Module* nodep, અચલ string& scopename) {
        // Only called on the module at the very top of the hierarchy
        VSymEnt* અચલ symp = નવું VSymEnt{&m_syms, nodep};
        UINFO(9, "      INSERTtop se" << cvtToHex(symp) << "  " << scopename << " " << nodep);
        symp->parentp(મૂળ_Entp());  // Needed so backward search can find name of top module
        symp->fallbackp(dunitEntp());  // Needed so can find $unit stuff
        nodep->user1p(symp);
        checkDuplicate(મૂળ_Entp(), nodep, nodep->origName());
        મૂળ_Entp()->ઉમેરો(nodep->origName(), symp);
        જો (forScopeCreation()) m_nameScopeSymMap.emplace(scopename, symp);
        પાછા symp;
    }
    VSymEnt* ઉમેરો_ટોચ_Iface(AstCell* nodep, અચલ string& scopename) {
        VSymEnt* અચલ symp = નવું VSymEnt{&m_syms, nodep};
        UINFO(9, "      INSERTtopiface se" << cvtToHex(symp) << "  " << scopename << " " << nodep);
        symp->parentp(મૂળ_Entp());  // Needed so backward search can find name of top module
        symp->fallbackp(dunitEntp());  // Needed so can find $unit stuff
        nodep->user1p(symp);
        જો (nodep->modp()) nodep->modp()->user1p(symp);
        checkDuplicate(મૂળ_Entp(), nodep, nodep->origName());
        મૂળ_Entp()->ઉમેરો(nodep->origName(), symp);
        જો (forScopeCreation()) m_nameScopeSymMap.emplace(scopename, symp);
        પાછા symp;
    }
    VSymEnt* ઉમેરો_Cell(VSymEnt* abovep, VSymEnt* modSymp, AstCell* nodep,
                        અચલ string& scopename) {
        UASSERT_OBJ(abovep, nodep, "Null symbol table inserting node");
        VSymEnt* અચલ symp = નવું VSymEnt{&m_syms, nodep};
        UINFO(9, "      INSERTcel se" << cvtToHex(symp) << "  " << scopename << " above=se"
                                      << cvtToHex(abovep) << " mods=se" << cvtToHex(modSymp)
                                      << " node=" << nodep);
        symp->parentp(abovep);
        symp->fallbackp(dunitEntp());  // Needed so can find $unit stuff
        nodep->user1p(symp);
        જો (nodep->modp()) nodep->modp()->user1p(symp);
        checkDuplicate(abovep, nodep, nodep->origName());
        abovep->reinsert(nodep->origName(), symp);
        જો (forScopeCreation() && abovep != modSymp && !modSymp->findIdFlat(nodep->name())) {
            // If it's foo_DOT_bar, we need to be able to find it under "foo_DOT_bar" too.
            // Duplicates are possible, as until resolve generates might
            // have 2 same cells under an if
            modSymp->reinsert(nodep->name(), symp);
        }
        જો (forScopeCreation()) m_nameScopeSymMap.emplace(scopename, symp);
        પાછા symp;
    }
    શૂન્ય ઉમેરો_Map(VSymEnt* symp, અચલ string& scopename) {
        જો (forScopeCreation()) m_nameScopeSymMap.emplace(scopename, symp);
    }

    VSymEnt* ઉમેરો_Inline(VSymEnt* abovep, VSymEnt* modSymp, AstCellInline* nodep,
                          અચલ string& basename) {
        // A fake point in the hierarchy, corresponding to a named block flattened by V3Begin.
        // Names are looked up in the module with the block's flattened prefix.
        UASSERT_OBJ(abovep, nodep, "Null symbol table inserting node");
        VSymEnt* અચલ symp = નવું VSymEnt{&m_syms, nodep};
        UINFO(9, "      INSERTinl se" << cvtToHex(symp) << "  " << basename << " above=se"
                                      << cvtToHex(abovep) << " mods=se" << cvtToHex(modSymp)
                                      << " node=" << nodep);
        symp->parentp(abovep);
        symp->fallbackp(modSymp);
        symp->symPrefix(nodep->name() + "__DOT__");
        nodep->user1p(symp);
        checkDuplicate(abovep, nodep, nodep->name());
        abovep->reinsert(basename, symp);
        જો (abovep != modSymp && !modSymp->findIdFlat(nodep->name())) {
            // If it's foo_DOT_bar, we need to be able to find it under that too.
            modSymp->reinsert(nodep->name(), symp);
        }
        પાછા symp;
    }
    VSymEnt* ઉમેરો_Block(VSymEnt* abovep, અચલ string& name, Ast_ગાંઠ* nodep,
                         Ast_ગાંઠ_Module* classOrPackagep) {
        // A fake point in the hierarchy, corresponding to a begin or function/task block
        // After we remove begins these will go away
        // Note we fallback to the symbol table of the parent, as we want to find variables there
        // However, cells walk the graph, so cells will appear under the begin/ftask itself
        UASSERT_OBJ(abovep, nodep, "Null symbol table inserting node");
        VSymEnt* અચલ symp = નવું VSymEnt{&m_syms, nodep};
        UINFO(9, "      INSERTblk se" << cvtToHex(symp) << "  above=se" << cvtToHex(abovep)
                                      << " pkg=" << cvtToHex(classOrPackagep)
                                      << "  node=" << nodep);
        symp->parentp(abovep);
        symp->classOrPackagep(classOrPackagep);
        symp->fallbackp(abovep);
        nodep->user1p(symp);
        જો (name != "") checkDuplicate(abovep, nodep, name);
        // Duplicates are possible, as until resolve generates might have 2 same cells under an if
        abovep->reinsert(name, symp);
        પાછા symp;
    }
    VSymEnt* ઉમેરો_Sym(VSymEnt* abovep, અચલ string& name, Ast_ગાંઠ* nodep,
                       Ast_ગાંઠ_Module* classOrPackagep) {
        UASSERT_OBJ(abovep, nodep, "Null symbol table inserting node");
        VSymEnt* અચલ symp = નવું VSymEnt{&m_syms, nodep};
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
        પાછા symp;
    }
    સ્થિર તાર્કિક exists_ગાંઠ_Sym(Ast_ગાંઠ* nodep) { પાછા nodep->user1p() != nullptr; }
    સ્થિર VSymEnt* get_ગાંઠ_Sym(Ast_ગાંઠ* nodep) {
        // Don't use this in ResolveVisitor, as we need to pick up the proper
        // reference under each SCOPE
        VSymEnt* અચલ symp = nodep->user1u().toSymEnt();
        UASSERT_OBJ(symp, nodep, "Module/etc never assigned a symbol entry?");
        પાછા symp;
    }
    VSymEnt* getScopeSym(AstScope* nodep) {
        અચલ auto it = m_nameScopeSymMap.find(nodep->name());
        UASSERT_OBJ(it != m_nameScopeSymMap.end(), nodep,
                    "Scope never assigned a symbol entry '" << nodep->name() << "'");
        પાછા it->second;
    }
    શૂન્ય implicitOkAdd(Ast_ગાંઠ_Module* nodep, અચલ string& varname) {
        // Mark the given variable name as being allowed to be implicitly declared
        જો (nodep) {
            અચલ auto it = m_implicitNameSet.find(std::make_pair(nodep, varname));
            જો (it == m_implicitNameSet.end()) m_implicitNameSet.emplace(nodep, varname);
        }
    }
    તાર્કિક implicitOk(Ast_ગાંઠ_Module* nodep, અચલ string& varname) {
        પાછા nodep
               && (m_implicitNameSet.find(std::make_pair(nodep, varname))
                   != m_implicitNameSet.end());
    }

    // Track and later recurse interface modules
    શૂન્ય ઉમેરો_Iface_Mod_Sym(AstIface* nodep, VSymEnt* symp) {
        m_ifaceModSyms.emplace_back(nodep, symp);
    }
    શૂન્ય computeIfaceModSyms();

    // Track and later insert interface references
    શૂન્ય ઉમેરો_Iface_Var_Sym(VSymEnt* symp) {  // Where sym is for a VAR of dtype IFACEREFDTYPE
        m_ifaceVarSyms.ધકેલો_back(symp);
    }
    // Iface for a raw or arrayed iface; peels nested array layers for multi-dim arrays.
    સ્થિર AstIfaceRefDType* ifaceRefFromArray(Ast_ગાંઠ_DType* nodep) {
        જ્યારે (nodep) {
            જો (AstIfaceRefDType* અચલ ifp = VN_CAST(nodep, IfaceRefDType)) પાછા ifp;
            જો (અચલ AstBracketArrayDType* અચલ arrp = VN_CAST(nodep, BracketArrayDType)) {
                nodep = arrp->subDTypep();
            } નહીંતર જો (અચલ AstUnpackArrayDType* અચલ arrp = VN_CAST(nodep, UnpackArrayDType)) {
                nodep = arrp->subDTypep();
            } નહીંતર {
                પાછા nullptr;
            }
        }
        પાછા nullptr;
    }
    // Given a pin expression, resolve it to a live AstIface* (or nullptr).
    // Handles both simple VarRef and dotted VarXRef pin connections.
    AstIface* live_Iface_From_Pin_પદાવલિ(Ast_ગાંઠ* exprp, VSymEnt* parentSymp) {
        AstVar* resolvedVarp = nullptr;
        જો (AstVarRef* અચલ refp = VN_CAST(exprp, VarRef)) {
            resolvedVarp = refp->varp();
        } નહીંતર જો (AstVarXRef* અચલ xrefp = VN_CAST(exprp, VarXRef)) {
            VSymEnt* અચલ lookupSymp = parentSymp ? parentSymp->parentp() : nullptr;
            જો (lookupSymp) {
                string baddot;
                VSymEnt* okSymp = nullptr;
                અચલ string dotpath = xrefp->dotted() + "." + xrefp->name();
                VSymEnt* અચલ dotSymp
                    = findDotted(xrefp->fileline(), lookupSymp, dotpath, baddot, okSymp, સાચું);
                જો (dotSymp) resolvedVarp = VN_CAST(dotSymp->nodep(), Var);
            }
        }
        AstIface* resultp = nullptr;
        જો (resolvedVarp) {
            AstIfaceRefDType* અચલ irefp = ifaceRefFromArray(resolvedVarp->subDTypep());
            જો (irefp && irefp->ifaceViaCellp() && !irefp->ifaceViaCellp()->dead()) {
                resultp = irefp->ifaceViaCellp();
            }
        }
        પાછા resultp;
    }

    // Attempt to repair a port's AstIfaceRefDType by tracing through the
    // parent cell's pin connection to find the correct live interface.
    // Returns true if repaired or already correct; false if unresolvable.
    તાર્કિક repairIfaceRef(VSymEnt* varSymp, AstVar* varp, AstIfaceRefDType* ifacerefp, તાર્કિક isDead) {
        // Walk up symbol-table parents to find the enclosing AstCell
        VSymEnt* parentSymp = varSymp->parentp();
        AstCell* parentCellp = nullptr;
        માટે (પૂર્ણાંક depth = 0; parentSymp && depth < 20; ++depth) {
            જો (AstCell* અચલ cp = VN_CAST(parentSymp->nodep(), Cell)) {
                parentCellp = cp;
                તોડો;
            }
            parentSymp = parentSymp->parentp();
        }
        જો (!parentCellp) પાછા ખોટું;
        // Scan pins to find the one connected to this port variable
        માટે (AstPin* pinp = parentCellp->pinsp(); pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            જો (pinp->modVarp() != varp && pinp->name() != varp->name()) ચાલુ;
            Ast_ગાંઠ* અચલ exprp = pinp->exprp();
            જો (!exprp) પાછા ખોટું;
            AstIface* અચલ newIfacep = live_Iface_From_Pin_પદાવલિ(exprp, parentSymp);
            જો (newIfacep && newIfacep != ifacerefp->ifaceViaCellp()) {
                UINFO(4, "  REPAIR-IFACE-REF var=" << varp->prettyNameQ()
                                                   << " old=" << ifacerefp->ifacep()->prettyNameQ()
                                                   << " new=" << newIfacep->prettyNameQ());
                ifacerefp->ifacep(newIfacep);
                પાછા સાચું;
            }
            જો (newIfacep) પાછા !isDead;  // Same interface - OK if live
            પાછા ખોટું;
        }
        પાછા ખોટું;
    }

    શૂન્ય computeIfaceVarSyms() {
        માટે (VSymEnt* varSymp : m_ifaceVarSyms) {
            AstVar* અચલ varp = varSymp ? VN_AS(varSymp->nodep(), Var) : nullptr;
            UINFO(9, "  insAllIface se" << cvtToHex(varSymp) << " " << varp);
            AstIfaceRefDType* અચલ ifacerefp = ifaceRefFromArray(varp->subDTypep());
            UASSERT_OBJ(ifacerefp, varp, "Non-ifacerefs on list!");
            જો (!ifacerefp->cellp() && !ifacerefp->isVirtual() && (varp->pin_અંક() == 0)) {
                varp->v3error("Interface port declaration "
                              << varp->prettyNameQ() << " doesn't have corresponding port\n"
                              << varp->warnMore()
                                     + "... Perhaps intended an interface instantiation but "
                                       "are missing parenthesis (IEEE 1800-2023 25.3)?");
            }
            જો (!ifacerefp->ifaceViaCellp()) {
                જો (!ifacerefp->cellp()) {  // Probably a NotFoundModule, or a normal module if
                                            // made mistake
                    UINFO(1, "Associated cell " << Ast_ગાંઠ::prettyNameQ(ifacerefp->cellName()));
                    ifacerefp->v3error("Cannot find file containing interface: "
                                       << Ast_ગાંઠ::prettyNameQ(ifacerefp->ifaceName()));
                    ચાલુ;
                } નહીંતર {
                    ifacerefp->v3fatalSrc("Unlinked interface");
                }
            } નહીંતર જો (varp->isIfaceRef() && !ifacerefp->cellp() && ifacerefp->ifaceViaCellp()) {
                // Port variable (cellp=null) pointing to an interface.
                // The interface may be dead (template replaced by clone) or
                // live-but-wrong (template still alive because some instances
                // use default params, but this port should point to a
                // specialized clone based on the pin connection).
                અચલ તાર્કિક isDead = ifacerefp->ifaceViaCellp()->dead();
                જો (isDead && forPrimary() && !varp->isIfaceParent()
                    && !v3Global.opt.ટોચ_Ifaces_Supported()) {
                    varp->v3warn(E_UNSUPPORTED,
                                 "Unsupported: Interfaced port on top level module");
                }
                // Attempt repair: trace through parent cell's pin to find
                // the correct live interface for this port variable
                અચલ તાર્કિક repaired = repairIfaceRef(varSymp, varp, ifacerefp, isDead);
                જો (!repaired && isDead) {
                    ifacerefp->v3error(
                        "Interface " << Ast_ગાંઠ::prettyNameQ(ifacerefp->ifaceName())
                                     << " not connected as parent's interface not connected\n"
                                     << ifacerefp->warnMore()
                                     << "... Perhaps caused by another error on the parent "
                                        "interface that needs resolving\n"
                                     << ifacerefp->warnMore()
                                     << "... Or, perhaps intended an interface instantiation but "
                                        "are missing parenthesis (IEEE 1800-2023 25.3)?");
                    ચાલુ;
                }
            } નહીંતર જો (ifacerefp->ifaceViaCellp()->dead()
                       || !exists_ગાંઠ_Sym(ifacerefp->ifaceViaCellp())) {
                ifacerefp->ifaceViaCellp()->v3fatalSrc(
                    "Interface referenced by virtual iface is dead. "
                    "Dead interfaces have required linking steps skipped");
            }
            VSymEnt* અચલ ifaceSymp = get_ગાંઠ_Sym(ifacerefp->ifaceViaCellp());
            VSymEnt* ifOrPortSymp = ifaceSymp;
            // Link Modport names to the Modport Node under the Interface
            જો (ifacerefp->isModport()) {
                VSymEnt* અચલ foundp = ifaceSymp->findIdFallback(ifacerefp->modportName());
                તાર્કિક ok = ખોટું;
                જો (foundp) {
                    જો (AstModport* અચલ modportp = VN_CAST(foundp->nodep(), Modport)) {
                        UINFO(4, "Link Modport: " << modportp);
                        ifacerefp->modportp(modportp);
                        ifOrPortSymp = foundp;
                        ok = સાચું;
                    }
                }
                જો (!ok) {
                    અચલ string suggest = suggestSymFallback(ifaceSymp, ifacerefp->modportName(),
                                                              Link_ગાંઠ_Matcher_Modport{});
                    ifacerefp->modportFileline()->v3error(
                        "Modport not found under interface "
                        << ifacerefp->prettyNameQ(ifacerefp->ifaceName()) << ": "
                        << ifacerefp->prettyNameQ(ifacerefp->modportName()) << '\n'
                        << (suggest.empty() ? "" : ifacerefp->warnMore() + suggest));
                }
            }
            // Alias won't expand until interfaces and modport names are known; see notes at top
            ઉમેરો_Scope_Alias(SAMN_IFTOP, varSymp, ifOrPortSymp);
        }
        m_ifaceVarSyms.clear();
    }

    શૂન્ય ઉમેરો_Scope_Alias(SAMNum samn, VSymEnt* lhsp, VSymEnt* rhsp) {
        // Track and later insert scope aliases; an interface referenced by
        // a child cell connecting to that interface
        // Typically lhsp=VAR w/dtype IFACEREF, rhsp=IFACE cell
        UINFO(9, "   insertScopeAlias se" << cvtToHex(lhsp) << " se" << cvtToHex(rhsp));
        UASSERT_OBJ(
            !(VN_IS(rhsp->nodep(), Cell) && !VN_IS(VN_AS(rhsp->nodep(), Cell)->modp(), Iface)),
            rhsp->nodep(), "Got a non-IFACE alias RHS");
        m_scopeAliasMap[samn].emplace(lhsp, rhsp);
    }
    શૂન્ય computeScopeAliases() {
        UINFO(9, "computeIfaceAliases");
        માટે (પૂર્ણાંક samn = 0; samn < SAMN__MAX; ++samn) {
            માટે (ScopeAliasMap::iterator it = m_scopeAliasMap[samn].begin();
                 it != m_scopeAliasMap[samn].end(); ++it) {
                VSymEnt* અચલ lhsp = it->first;
                VSymEnt* srcp = lhsp;
                જ્યારે (સાચું) {  // Follow chain of aliases up to highest level non-alias
                    અચલ auto it2 = m_scopeAliasMap[samn].find(srcp);
                    જો (it2 != m_scopeAliasMap[samn].end()) {
                        srcp = it2->second;
                        ચાલુ;
                    } નહીંતર {
                        તોડો;
                    }
                }
                UINFO(9, "  iiasa: Insert alias se" << lhsp << " (" << lhsp->nodep()->typeName()
                                                    << ") <- se" << srcp << " " << srcp->nodep());
                // srcp should be an interface reference pointing to the interface we want to
                // import
                lhsp->importFromIface(symsp(), srcp);
                // Allow access to objects not permissible to be listed in a modport
                જો (VN_IS(srcp->nodep(), Modport)) {
                    lhsp->importFromIface(symsp(), srcp->parentp(), સાચું);
                }
            }
            // m_scopeAliasMap[samn].clear();  // Done with it, but put into debug file
        }
    }

ખાનગી:
    VSymEnt* findWithAltFallback(VSymEnt* symp, અચલ string& name, અચલ string& altname) {
        VSymEnt* findp = symp->findIdFallback(name);
        જો (findp) પાછા findp;
        જો (altname != "") {
            UINFO(8, "     alt fallback");
            findp = symp->findIdFallback(altname);
        }
        પાછા findp;
    }

    VSymEnt* findWithAltFlat(VSymEnt* symp, અચલ string& name, અચલ string& altname) {
        VSymEnt* findp = symp->findIdFlat(name);
        જો (findp) પાછા findp;
        જો (altname != "") {
            UINFO(8, "     alt flat");
            findp = symp->findIdFlat(altname);
        }
        પાછા findp;
    }

જાહેર:
    VSymEnt* findDotted(File_લીટી* refLocationp, VSymEnt* lookupSymp, અચલ string& dotname,
                        string& baddot, VSymEnt*& okSymp, તાર્કિક firstId) {
        // Given a dotted hierarchy name, return where in scope it is
        // Note when dotname=="" we just fall through and return lookupSymp
        UINFO(8, "    dottedFind se" << cvtToHex(lookupSymp) << " '" << dotname << "'");
        string leftname = dotname;
        okSymp = lookupSymp;  // So can list bad scopes
        જ્યારે (leftname != "") {  // foreach dotted part of xref name
            string::size_type pos = leftname.find('.');
            string ident;
            જો (pos != string::npos) {
                ident = leftname.substr(0, pos);
                leftname = leftname.substr(pos + 1);
            } નહીંતર {
                ident = leftname;
                leftname = "";
            }
            baddot = ident;  // So user can see where they botched it
            okSymp = lookupSymp;
            string altIdent;
            જો (!forScopeCreation()) {
                // GENFOR Begin is foo__BRA__##__KET__ after we've genloop unrolled,
                // but presently should be just "foo".
                // Likewise cell foo__[array] before we've expanded arrays is just foo.
                // Multi-dim iface arrays append multiple __BRA__..__KET__ suffixes; strip them
                // all.
                altIdent = ident;
                જ્યારે (VString::endsWith(altIdent, "__KET__")) {
                    અચલ auto braPos = altIdent.rfind("__BRA__");
                    જો (braPos == string::npos) તોડો;
                    altIdent = altIdent.substr(0, braPos);
                }
                જો (altIdent == ident) altIdent.clear();
            }
            UINFO(8, "         id " << ident << " alt " << altIdent << " left " << leftname
                                    << " at se" << lookupSymp);
            // Spec says; Look at existing module (cellnames then modname),
            // then look up (inst name or modname)
            જો (firstId) {
                // Check this module - subcellnames
                અચલ AstCell* cellp = lookupSymp ? VN_CAST(lookupSymp->nodep(), Cell)
                                                  : nullptr;  // Replicated below
                જો (VSymEnt* અચલ findSymp = findWithAltFallback(lookupSymp, ident, altIdent)) {
                    lookupSymp = findSymp;
                }

                // Check this module - cur modname
                નહીંતર જો (cellp && cellp->modp()->origName() == ident) {
                }
                // $root we walk up to Netlist
                નહીંતર જો (ident == "$root") {
                    lookupSymp = મૂળ_Entp();
                    // We've added the '$root' module, now everything else is one lower
                    જો (forScopeCreation()) {
                        lookupSymp = lookupSymp->findIdFlat(ident);
                        UASSERT(lookupSymp, "Cannot find $root module under netlist");
                    }
                }
                // Move up and check cellname + modname
                નહીંતર {
                    તાર્કિક crossedCell = ખોટું;  // Crossed a cell boundary
                    જ્યારે (lookupSymp) {
                        lookupSymp = lookupSymp->parentp();
                        cellp = lookupSymp ? VN_CAST(lookupSymp->nodep(), Cell)
                                           : nullptr;  // Replicated above
                        જો (lookupSymp) {
                            UINFO(9, "    Up to " << lookupSymp);
                            જો (cellp || VN_IS(lookupSymp->nodep(), CellInline)) {
                                crossedCell = સાચું;
                            }
                            જો (cellp && cellp->modp()->origName() == ident) {
                                તોડો;
                            } નહીંતર જો (VSymEnt* અચલ findSym2p
                                       = findWithAltFallback(lookupSymp, ident, altIdent)) {
                                lookupSymp = findSym2p;
                                જો (crossedCell && VN_IS(lookupSymp->nodep(), Var)) {
                                    UINFO(9, "    Not found but matches var name in parent "
                                                 << lookupSymp);
                                    પાછા nullptr;
                                }
                                તોડો;
                            }
                        } નહીંતર {
                            તોડો;
                        }
                    }
                    જો (!lookupSymp) પાછા nullptr;  // Not found
                }
            } નહીંતર {  // Searching for middle path component, must be a cell or interface port
                VSymEnt* findSymp = findWithAltFlat(lookupSymp, ident, altIdent);
                જો (findSymp) {
                    lookupSymp = findSymp;
                } નહીંતર {
                    // Try prefixed lookup for interface ports accessed through hierarchy
                    // (e.g., slave_inst.bus.data where bus is an interface port)
                    // After inlining, interface ports have prefixed names like
                    // top__DOT__slave_inst__DOT__bus
                    UINFO(8, "     middle-path: trying prefixed lookup for '" << ident << "'\n");
                    string baddot;
                    findSymp = findSymPrefixed(lookupSymp, ident, baddot, /*fallback=*/સાચું);
                    UINFO(8, "     middle-path: prefixed lookup result: "
                                 << (findSymp ? "found" : "not found") << "\n");
                    જો (findSymp) {
                        // Check if this is an interface reference variable
                        અચલ AstVar* varp = VN_CAST(findSymp->nodep(), Var);
                        જો (!varp) {
                            જો (અચલ AstVarScope* vscp = VN_CAST(findSymp->nodep(), VarScope)) {
                                varp = vscp->varp();
                            }
                        }
                        જો (varp && varp->isIfaceRef()) {
                            // Found an interface port - use findSymp directly
                            // computeScopeAliases has already imported interface members
                            // into this symbol entry, so we use it instead of redirecting
                            // to the shared modport definition (which would cause all
                            // instances to resolve to the same symbols - bug #2656)
                            lookupSymp = findSymp;
                        } નહીંતર {
                            // Non-interface symbol in middle path must be a cell (module instance)
                            // to continue hierarchical resolution
                            જો (VN_IS(findSymp->nodep(), Cell)) {
                                lookupSymp = findSymp;
                            } નહીંતર {
                                // Reject non-cell, non-interface symbols found via fallback
                                // to prevent accidental resolution to unrelated symbols
                                UINFO(8, "     middle-path: rejecting non-cell symbol\n");
                                પાછા nullptr;
                            }
                        }
                    } નહીંતર {
                        પાછા nullptr;  // Not found
                    }
                }
            }
            જો (lookupSymp) {
                જો (અચલ AstCell* અચલ cellp = VN_CAST(lookupSymp->nodep(), Cell)) {
                    જો (અચલ Ast_ગાંઠ_Module* અચલ modp = cellp->modp()) {
                        જો (modp->hierBlock() && !leftname.empty()) {
                            refLocationp->v3error("Cannot access scope inside hierarchical block");
                        } નહીંતર જો (VN_IS(modp, NotFoundModule)) {
                            refLocationp->v3error("Dotted reference to instance that refers to "
                                                  "missing module/interface: "
                                                  << modp->prettyNameQ());
                        }
                    }
                }
                // Follow scope alias for nested interface port access
                જો (!leftname.empty()) {
                    અચલ auto aliasIt = m_scopeAliasMap[SAMN_IFTOP].find(lookupSymp);
                    જો (aliasIt != m_scopeAliasMap[SAMN_IFTOP].end()) {
                        lookupSymp = aliasIt->second;
                        // Alias may point to __Viftop VarScope; find corresponding Cell
                        જો (અચલ AstVarScope* અચલ vscp
                            = VN_CAST(lookupSymp->nodep(), VarScope)) {
                            અચલ string varName = vscp->varp()->name();
                            સ્થિર constexpr અચલ અક્ષર* અચલ VIFTOP_SUFFIX = "__Viftop";
                            જો (VString::endsWith(varName, VIFTOP_SUFFIX)) {
                                અચલ string cellName
                                    = varName.substr(0, varName.size() - strlen(VIFTOP_SUFFIX));
                                VSymEnt* અચલ parentSymp = lookupSymp->parentp();
                                જો (parentSymp) {
                                    VSymEnt* અચલ cellSymp = parentSymp->findIdFlat(cellName);
                                    જો (cellSymp && VN_IS(cellSymp->nodep(), Cell)) {
                                        lookupSymp = cellSymp;  // Use Cell for member lookup
                                    }
                                }
                            }
                        }
                    } નહીંતર {
                        // No alias; try following IfaceRefDType to interface cell
                        અચલ AstVar* varp = VN_CAST(lookupSymp->nodep(), Var);
                        જો (!varp) {
                            જો (અચલ AstVarScope* અચલ vscp
                                = VN_CAST(lookupSymp->nodep(), VarScope)) {
                                varp = vscp->varp();
                            }
                        }
                        જો (varp && varp->isIfaceRef()) {
                            જો (અચલ AstIfaceRefDType* અચલ ifaceRefp
                                = ifaceRefFromArray(varp->dtypep())) {
                                જો (ifaceRefp->cellp() && exists_ગાંઠ_Sym(ifaceRefp->cellp())) {
                                    lookupSymp = get_ગાંઠ_Sym(ifaceRefp->cellp());
                                } નહીંતર જો (ifaceRefp->ifaceViaCellp()
                                           && exists_ગાંઠ_Sym(ifaceRefp->ifaceViaCellp())) {
                                    lookupSymp = get_ગાંઠ_Sym(ifaceRefp->ifaceViaCellp());
                                } નહીંતર જો (ifaceRefp->ifacep()
                                           && exists_ગાંઠ_Sym(ifaceRefp->ifacep())) {
                                    lookupSymp = get_ગાંઠ_Sym(ifaceRefp->ifacep());
                                }
                            }
                        }
                    }
                }
            }
            firstId = ખોટું;
        }
        પાછા lookupSymp;
    }

    સ્થિર string removeLastInlineScope(અચલ string& name) {
        string out = name;
        અચલ string dot = "__DOT__";
        અચલ size_t dotPos = out.rfind(dot, out.size() - dot.length() - 1);
        જો (dotPos == string::npos) {
            પાછા "";
        } નહીંતર {
            પાછા out.erase(dotPos + dot.length(), string::npos);
        }
    }

    VSymEnt* findSymPrefixed(VSymEnt* lookupSymp, અચલ string& dotname, string& baddot,
                             તાર્કિક fallback) {
        // Find symbol in given point in hierarchy, allowing prefix (post-Inline)
        // For simplicity lookupSymp may be passed nullptr result from findDotted
        જો (!lookupSymp) પાછા nullptr;
        UINFO(8, "    findSymPrefixed "
                     << dotname << " under se" << cvtToHex(lookupSymp)
                     << ((lookupSymp->symPrefix() == "") ? "" : " as ")
                     << ((lookupSymp->symPrefix() == "") ? "" : lookupSymp->symPrefix() + dotname)
                     << "  at se" << lookupSymp << " fallback=" << fallback);
        string prefix = lookupSymp->symPrefix();
        VSymEnt* foundp = nullptr;
        જ્યારે (!foundp) {
            જો (fallback) {
                foundp = lookupSymp->findIdFallback(prefix + dotname);  // Might be nullptr
            } નહીંતર {
                foundp = lookupSymp->findIdFlat(prefix + dotname);  // Might be nullptr
            }
            જો (prefix.empty()) તોડો;
            અચલ string nextPrefix = removeLastInlineScope(prefix);
            જો (prefix == nextPrefix) તોડો;
            prefix = nextPrefix;
        }
        જો (!foundp) baddot = dotname;
        પાછા foundp;
    }
    સ્થિર તાર્કિક checkIfClassOrPackage(અચલ VSymEnt* અચલ symp) {
        જો (VN_IS(symp->nodep(), Class) || VN_IS(symp->nodep(), Package)) પાછા સાચું;

        // Helper: check if a RefDType might resolve to a class later.
        અચલ auto isPotentialClassRef = [](અચલ AstRefDType* refp) -> તાર્કિક {
            પાછા refp && !refp->typeofp()
                   && (!refp->classOrPackageOpp()
                       || VN_IS(refp->classOrPackageOpp(), ClassOrPackageRef));
        };

        // Helper: allow types that can represent a class/package handle or an unresolved ref.
        // BasicDType is allowed here only for null/object handles.
        અચલ auto is_Valid_Type_ગાંઠ = [](અચલ Ast_ગાંઠ* nodep) -> તાર્કિક {
            પાછા VN_IS(nodep, VoidDType) || VN_IS(nodep, BasicDType)
                   || VN_IS(nodep, ClassRefDType) || VN_IS(nodep, ParseRef);
        };

        જો (અચલ AstTypedef* અચલ typedefp = VN_CAST(symp->nodep(), Typedef)) {
            અચલ Ast_ગાંઠ_DType* dtypep = typedefp->subDTypep();
            જો (!dtypep) dtypep = typedefp->childDTypep();
            જો (VN_IS(dtypep, ClassRefDType)) પાછા સાચું;
            જો (isPotentialClassRef(VN_CAST(dtypep, RefDType))) પાછા સાચું;
        } નહીંતર જો (અચલ AstParamTypeDType* અચલ paramTypep
                   = VN_CAST(symp->nodep(), ParamTypeDType)) {
            // Before V3Param the declared default is in childDTypep (possibly
            // wrapped in a RequireDType); after V3Param it is consumed and the
            // bound type is the resolved data type, e.g. a type parameter
            // inherited from a specialized base class (REQ #(Item) -> class Item).
            Ast_ગાંઠ* childp = paramTypep->childDTypep();
            જો (અચલ AstRequireDType* અચલ reqp = VN_CAST(childp, RequireDType)) {
                childp = reqp->lhsp();
            }
            અચલ Ast_ગાંઠ* અચલ checkp = childp ? childp : paramTypep->skipRefp();
            જો (is_Valid_Type_ગાંઠ(checkp) || isPotentialClassRef(VN_CAST(checkp, RefDType))) {
                પાછા સાચું;
            }
        }
        પાછા ખોટું;
    }
    VSymEnt* resolveClassOrPackage(VSymEnt* lookSymp, AstClassOrPackageRef* nodep, તાર્કિક fallback,
                                   તાર્કિક classOnly, અચલ string& forWhat,
                                   તાર્કિક deferIfUnresolved = ખોટું) {
        જો (nodep->classOrPackageSkipp()) પાછા get_ગાંઠ_Sym(nodep->classOrPackageSkipp());
        VSymEnt* foundp;
        VSymEnt* શોધ_Symp = lookSymp;
        જો (VL_UNCOVERABLE(શોધ_Symp && VN_IS(શોધ_Symp->nodep(), ParamTypeDType))) {
            શોધ_Symp->nodep()->v3fatalSrc(  // LCOV_EXCL_LINE
                "resolveClassOrPackage: unexpected ParamTypeDType lookup");
        }
        જો (fallback) {
            VSymEnt* currentLookSymp = શોધ_Symp;
            કરો {
                foundp = currentLookSymp->findIdFlat(nodep->name());
                જો (foundp && !checkIfClassOrPackage(foundp)) foundp = nullptr;
                જો (!foundp) currentLookSymp = currentLookSymp->fallbackp();
            } જ્યારે (!foundp && currentLookSymp);
        } નહીંતર {
            foundp = શોધ_Symp->findIdFlat(nodep->name());
            જો (foundp && !checkIfClassOrPackage(foundp)) foundp = nullptr;
        }
        જો (!foundp && v3Global.rootp()->stdPackagep()) {  // Look under implied std::
            foundp = get_ગાંઠ_Sym(v3Global.rootp()->stdPackagep())->findIdFlat(nodep->name());
        }
        જો (foundp) {
            nodep->classOrPackageNodep(foundp->nodep());
            પાછા foundp;
        }
        જો (deferIfUnresolved) પાછા nullptr;
        અચલ string suggest
            = suggestSymFallback(lookSymp, nodep->name(), Link_ગાંઠ_Matcher_Class_Or_Package{});
        nodep->v3error((classOnly ? "Class" : "Package/class")
                       << " for '" << forWhat  // extends/implements
                       << "' not found: " << nodep->prettyNameQ() << '\n'
                       << (suggest.empty() ? "" : nodep->warnMore() + suggest));
        પાછા nullptr;
    }
    string suggestSymFallback(VSymEnt* lookupSymp, અચલ string& name,
                              અચલ VNodeMatcher& matcher) {
        // Suggest alternative symbol in given point in hierarchy
        // Does not support inline, as we find user-level errors before inlining
        // For simplicity lookupSymp may be passed nullptr result from findDotted
        જો (!lookupSymp) પાછા "";
        VSpellCheck speller;
        lookupSymp->candidateIdFallback(&speller, &matcher);
        પાછા speller.best_Candidate_સંદેશ(name);
    }
    string suggestSymFlat(VSymEnt* lookupSymp, અચલ string& name, અચલ VNodeMatcher& matcher,
                          તાર્કિક includeType) {
        જો (!lookupSymp) પાછા "";
        VSpellCheck speller;
        lookupSymp->candidateIdFlat(&speller, &matcher, includeType);
        પાછા speller.best_Candidate_સંદેશ(name);
    }
};

LinkDotState* LinkDotState::s_errorThisp = nullptr;

//======================================================================

વર્ગ LinkDotFindVisitor final : જાહેર VNVisitor {
    // NODE STATE
    // Cleared on global
    //  *::user1p()             -> See LinkDotState
    //  *::user2p()             -> See LinkDotState
    //  *::user3()              // bool. Node stored in m_virtualIfaces
    //  *::user4()              -> See LinkDotState
    અચલ VNUser3InUse m_inuser3;

    // STATE
    LinkDotState* અચલ m_statep;  // State to pass between visitors, including symbol table
    Ast_ગાંઠ_Module* m_classOrPackagep = nullptr;  // Current package
    AstClocking* m_clockingp = nullptr;  // Current clocking block
    std::vector<AstScope*> m_scopes;  // Scopes detached during repeated module visits
    uint64_t m_statScopeVisits = 0;  // Non-top scopes encountered while finding instance symbols
    VSymEnt* m_modSymp = nullptr;  // Symbol Entry for current module
    VSymEnt* m_curSymp = nullptr;  // Symbol Entry for current table, where to lookup/insert
    string
        m_hierParamsName;  // Name of module with hierarchical type parameters, empty when not used
    string m_scope;  // Scope text
    અચલ Ast_ગાંઠ_FTask* m_ftaskp = nullptr;  // Current function/task
    તાર્કિક m_inRecursion = ખોટું;  // Inside a recursive module
    પૂર્ણાંક m_param_અંક = 0;  // Parameter number, for position based connection
    તાર્કિક m_explicitNew = ખોટું;  // Hit a "new" function
    પૂર્ણાંક m_mod_Block_અંક = 0;  // Begin block number in module, 0=none seen
    પૂર્ણાંક m_mod_With_અંક = 0;  // With block number, 0=none seen
    પૂર્ણાંક m_mod_Arg_અંક = 0;  // Arg block number for randomize(), 0=none seen
    std::vector<AstIface*> m_virtIfaces;  // interfaces used as virtual,
                                          // needed for handleUnvisitedVirtIfaces()

    // METHODS
    શૂન્ય makeImplicitNew(AstClass* nodep) {
        AstFunc* અચલ newp = નવું AstFunc{nodep->fileline(), "new", nullptr, nullptr};
        // If needed, super.new() call is added the 2nd pass of V3LinkDotResolve,
        // because base classes are already resolved there.
        newp->isConstructor(સાચું);
        nodep->addMembersp(newp);
        UINFO(8, "Made implicit new for " << nodep->name() << ": " << nodep);
        m_statep->ઉમેરો_Block(m_curSymp, newp->name(), newp, m_classOrPackagep);
    }

    સ્થિર તાર્કિક isHierBlockWrapper(અચલ string& name) {
        અચલ V3HierBlockOptSet& hierBlocks = v3Global.opt.hierBlocks();
        પાછા hierBlocks.find(name) != hierBlocks.end();
    }

    શૂન્ય moveExternFuncDecl(Ast_ગાંઠ_FTask* nodep, AstClass* classp) {
        // Move an 'extern function|task' to inside a class or, from an outer to inner-class
        જો (nodep->isExternDef()) પાછા;
        // Move it to proper spot under the target class
        nodep->unlinkFrBack();
        classp->addStmtsp(nodep);
        nodep->isExternDef(સાચું);  // So we check there's a matching extern
        nodep->classOrPackagep()->unlinkFrBack()->delete_વૃક્ષ();
        // Any "Type::" reference in the function's IO are really "MovedToClass::" references
        જો (nodep->fvarp()) moveExternFuncDeclRefs(nodep->fvarp(), classp);
        માટે (Ast_ગાંઠ* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            જો (AstVar* અચલ portp = VN_CAST(stmtp, Var)) {
                જો (portp->isIO()) moveExternFuncDeclRefs(portp, classp);
            }
        }
    }
    શૂન્ય moveExternFuncDeclRefs(Ast_ગાંઠ* nodep, AstClass* classp) {
        nodep->foreach([આ, classp](AstClassOrPackageRef* refp) {  //
            જો (refp->name() == classp->name() && !refp->paramsp()) {
                UINFO(9, "Cleaning up external function type for class " << refp);
                AstDot* અચલ dotp = VN_CAST(refp->backp(), Dot);
                જો (dotp && dotp->lhsp() == refp) {
                    // If had DOT(CLASSREF this, ...) the DOT can go away
                    dotp->replaceWith(dotp->rhsp()->unlinkFrBack());
                    VL_DO_DANGLING2(ધકેલો_Deletep(dotp), dotp, refp);
                } નહીંતર {
                    VL_DO_DANGLING(ધકેલો_Deletep(refp->unlinkFrBack()), refp);
                }
            }
        });
    }

    // VISITORS
    શૂન્ય visit(AstNetlist* nodep) override {  // FindVisitor::
        // Process $unit or other packages
        // Not needed - dotted references not allowed from inside packages
        // for (AstNodeModule* nodep = v3Global.rootp()->modulesp();
        //     nodep; nodep=VN_AS(nodep->nextp(), NodeModule)) {
        //    if (VN_IS(nodep, Package)) {}}

        m_statep->ઉમેરો_DUnit(nodep);

        // First back iterate, to find all packages. Backward as must do base
        // packages before using packages
        iterateChildrenBackwardsConst(nodep);

        // The first modules in the list are always the top modules
        // (sorted before this is called).
        // This may not be the module with isTop() set, as early in the steps,
        // wrapTop may have not been created yet.
        // $unit and the constant pool always exist, so nothing else, and nothing in $unit,
        // means nothing was given
        Ast_ગાંઠ_Module* અચલ modulesp = nodep->modulesp();
        UASSERT_OBJ(modulesp, nodep, "$unit should always be in the netlist");
        જો (m_statep->forPrimary()) {
            અચલ તાર્કિક onlyBuiltins = [&]() {
                માટે (Ast_ગાંઠ_Module* modp = modulesp; modp;
                     modp = VN_AS(modp->nextp(), ગાંઠ_Module)) {
                    જો (!modp->isDollarUnit() && !modp->isConstPool()) પાછા ખોટું;
                }
                પાછા સાચું;
            }();
            જો (onlyBuiltins && !v3Global.rootp()->dollarUnitPkgp()->stmtsp()) {
                nodep->v3error("No top level module found");
            }
        }
        માટે (Ast_ગાંઠ_Module* modp = nodep->modulesp(); modp && modp->is_ટોચ();
             modp = VN_AS(modp->nextp(), ગાંઠ_Module)) {
            UINFO(8, "Top Module: " << modp);
            m_scope = "TOP";

            જો (!m_statep->forScopeCreation() && v3Global.opt.ટોચ_Ifaces_Supported()) {
                માટે (Ast_ગાંઠ* subnodep = modp->stmtsp(); subnodep; subnodep = subnodep->nextp()) {
                    જો (AstVar* અચલ varp = VN_CAST(subnodep, Var)) {
                        જો (varp->isIfaceRef()) {
                            અચલ Ast_ગાંઠ_DType* અચલ subtypep = varp->subDTypep();
                            અચલ AstIfaceRefDType* ifacerefp = nullptr;
                            જો (VN_IS(subtypep, IfaceRefDType)) {
                                ifacerefp = VN_AS(varp->subDTypep(), IfaceRefDType);
                            } નહીંતર જો (VN_IS(subtypep, BracketArrayDType)) {
                                અચલ AstBracketArrayDType* અચલ arrp
                                    = VN_AS(subtypep, BracketArrayDType);
                                અચલ Ast_ગાંઠ_DType* અચલ arrsubtypep = arrp->subDTypep();
                                જો (VN_IS(arrsubtypep, IfaceRefDType)) {
                                    ifacerefp = VN_AS(arrsubtypep, IfaceRefDType);
                                }
                            } નહીંતર જો (VN_IS(subtypep, UnpackArrayDType)) {
                                અચલ AstUnpackArrayDType* અચલ arrp
                                    = VN_AS(subtypep, UnpackArrayDType);
                                અચલ Ast_ગાંઠ_DType* અચલ arrsubtypep = arrp->subDTypep();
                                જો (VN_IS(arrsubtypep, IfaceRefDType)) {
                                    ifacerefp = VN_AS(arrsubtypep, IfaceRefDType);
                                }
                            }

                            જો (ifacerefp && !ifacerefp->cellp()) {
                                // A dummy cell to keep the top level interface alive and correctly
                                // optimized for default parameter values
                                AstCell* ifacecellp
                                    = નવું AstCell{nodep->fileline(),
                                                  nodep->fileline(),
                                                  modp->name() + "__02E" + varp->name(),
                                                  ifacerefp->ifaceName(),
                                                  nullptr,
                                                  nullptr,
                                                  nullptr};
                                ifacecellp->modp(ifacerefp->ifacep());
                                m_curSymp = m_modSymp
                                    = m_statep->ઉમેરો_ટોચ_Iface(ifacecellp, m_scope);
                                iterate(ifacecellp);
                                VL_DO_DANGLING(ધકેલો_Deletep(ifacecellp), ifacecellp);
                            }
                        }
                    }
                }
            }

            m_curSymp = m_modSymp = m_statep->ઉમેરો_ટોચ_Cell(modp, m_scope);
            { iterate(modp); }

            m_scope = "";
            m_curSymp = m_modSymp = nullptr;
        }
    }
    શૂન્ય visit(AstTypeTable*) override {}  // FindVisitor::
    શૂન્ય visit(AstIfaceRefDType* nodep) override {  // FindVisitor::
        જો (!m_statep->forScopeCreation() && nodep->isVirtual() && nodep->ifacep()
            && !nodep->ifacep()->user3()) {
            m_virtIfaces.ધકેલો_back(nodep->ifacep());
            nodep->ifacep()->user3(સાચું);
        }
        iterateChildren(nodep);
    }
    શૂન્ય visit(Ast_ગાંઠ_Module* nodep) override {  // FindVisitor::
        // Called on top module from Netlist, other modules from the cell creating them,
        // and packages
        UINFO(8, "   " << nodep);
        // m_curSymp/m_modSymp maybe nullptr for packages and non-top modules
        // Packages will be under top after the initial phases, but until then
        // need separate handling
        અચલ તાર્કિક standalonePkg
            = !m_modSymp && !m_statep->forScopeCreation() && VN_IS(nodep, Package);
        અચલ તાર્કિક doit = (m_modSymp || standalonePkg);
        VL_RESTORER_COPY(m_scope);
        VL_RESTORER(m_classOrPackagep);
        VL_RESTORER(m_modSymp);
        VL_RESTORER(m_curSymp);
        VL_RESTORER(m_param_અંક);
        VL_RESTORER(m_mod_Block_અંક);
        VL_RESTORER(m_mod_With_અંક);
        VL_RESTORER(m_mod_Arg_અંક);
        જો (doit && nodep->user2()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Identically recursive module (module instantiates "
                          "itself, without changing parameters): "
                              << Ast_ગાંઠ::prettyNameQ(nodep->origName()));
        } નહીંતર જો (doit) {
            UINFO(4, "     Link Module: " << nodep);
            UASSERT_OBJ(!nodep->dead(), nodep, "Module in instance tree mislabeled as dead?");
            AstPackage* અચલ pkgp = VN_CAST(nodep, Package);
            m_classOrPackagep = pkgp;
            જો (standalonePkg) {
                જો (pkgp->isDollarUnit()) {
                    m_curSymp = m_modSymp = m_statep->dunitEntp();
                    nodep->user1p(m_curSymp);
                } નહીંતર {
                    VSymEnt* અચલ upperSymp = m_statep->dunitEntp();
                    m_scope = nodep->name();
                    m_curSymp = m_modSymp = m_statep->ઉમેરો_Block(upperSymp, nodep->name(), nodep,
                                                                  m_classOrPackagep);
                    UINFO(9, "New module scope " << m_curSymp);
                }
            }
            //
            m_param_અંક = 0;
            m_mod_Block_અંક = 0;
            m_mod_With_અંક = 0;
            m_mod_Arg_અંક = 0;
            // m_modSymp/m_curSymp for non-packages set by AstCell above this module
            // Iterate
            nodep->user2(સાચું);
            iterateChildren(nodep);
            nodep->user2(ખોટું);
            nodep->user4(સાચું);
            // Interfaces need another pass when signals are resolved. When creating
            // scopes this is done from the AstScopes in LinkDotScopeVisitor instead, as
            // only those entries hold the AstVarScopes the modport items resolve against.
            જો (!m_statep->forScopeCreation()) {
                જો (AstIface* અચલ ifacep = VN_CAST(nodep, Iface)) {
                    m_statep->ઉમેરો_Iface_Mod_Sym(ifacep, m_curSymp);
                }
            }
        } નહીંતર જો (isHierBlockWrapper(nodep->name())) {
            UINFO(5, "Module is hierarchical block, must not be dead: " << nodep);
            m_scope = nodep->name();
            VSymEnt* અચલ upperSymp = m_curSymp ? m_curSymp : m_statep->મૂળ_Entp();
            m_curSymp = m_modSymp
                = m_statep->ઉમેરો_Block(upperSymp, nodep->name() + "::", nodep, m_classOrPackagep);
            iterateChildren(nodep);
            nodep->user4(સાચું);
        } નહીંતર {  // !doit
            જો (nodep->hierParams()) {
                UINFO(1, "Found module with hier type parameters");
                m_hierParamsName = nodep->name();
                માટે (અચલ Ast_ગાંઠ* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    જો (અચલ AstTypedef* અચલ tdef = VN_CAST(stmtp, Typedef)) {
                        UINFO(1, "Inserting hier type parameter typedef: " << tdef);
                        VSymEnt* અચલ upperSymp = m_curSymp ? m_curSymp : m_statep->મૂળ_Entp();
                        m_curSymp = m_modSymp = m_statep->ઉમેરો_Block(upperSymp, nodep->name(),
                                                                      nodep, m_classOrPackagep);
                    }
                }
            } નહીંતર {
                // Will be optimized away later
                // Can't remove now, as our backwards iterator will throw up
                UINFO(5, "Module not under any CELL or top - dead module: " << nodep);
            }
        }
    }

    શૂન્ય visit(AstClass* nodep) override {  // FindVisitor::
        UASSERT_OBJ(m_curSymp, nodep, "Class not under module/package/$unit");
        UINFO(8, "   " << nodep);
        VL_RESTORER_COPY(m_scope);
        VL_RESTORER(m_classOrPackagep);
        VL_RESTORER(m_modSymp);
        VL_RESTORER(m_curSymp);
        VL_RESTORER(m_param_અંક);
        VL_RESTORER(m_mod_Block_અંક);
        VL_RESTORER(m_mod_With_અંક);
        VL_RESTORER(m_mod_Arg_અંક);
        VL_RESTORER(m_explicitNew);
        {
            UINFO(4, "     Link Class: " << nodep);
            VSymEnt* અચલ upperSymp = m_curSymp;
            m_scope = m_scope + "." + nodep->name();
            m_classOrPackagep = nodep;
            m_curSymp = m_modSymp
                = m_statep->ઉમેરો_Block(upperSymp, nodep->name(), nodep, m_classOrPackagep);
            m_statep->ઉમેરો_Map(m_curSymp, m_scope);
            UINFO(9, "New module scope " << m_curSymp);
            //
            m_param_અંક = 0;
            m_mod_Block_અંક = 0;
            m_mod_With_અંક = 0;
            m_mod_Arg_અંક = 0;
            m_explicitNew = ખોટું;
            // m_modSymp/m_curSymp for non-packages set by AstCell above this module
            // Iterate
            iterateChildren(nodep);
            nodep->user4(સાચું);
            // Implicit new needed?
            જો (!m_explicitNew && m_statep->forPrimary()) makeImplicitNew(nodep);
        }
    }
    શૂન્ય visit(AstClassOrPackageRef* nodep) override {  // FindVisitor::
        જો (!nodep->classOrPackageNodep() && nodep->name() == "$unit") {
            nodep->classOrPackageNodep(v3Global.rootp()->dollarUnitPkgp());
        }
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstScope* nodep) override {  // FindVisitor::
        UASSERT_OBJ(m_statep->forScopeCreation(), nodep,
                    "Scopes should only exist right after V3Scope");
        // Detach on first encounter, will re add at the end.
        // Otherwise would be visited O(cells^2) time after V3Scope
        જો (!nodep->is_ટોચ()) {
            ++m_statScopeVisits;
            nodep->unlinkFrBack();
            m_scopes.ધકેલો_back(nodep);
        }
    }
    શૂન્ય visit(AstCell* nodep) override {  // FindVisitor::
        UINFO(5, "   CELL under " << m_scope << " is " << nodep);
        // Process XREFs/etc inside pins
        જો (nodep->recursive() && m_inRecursion) પાછા;
        iterateChildren(nodep);
        // Recurse in, preserving state
        VL_RESTORER_COPY(m_scope);
        VL_RESTORER(m_modSymp);
        VL_RESTORER(m_curSymp);
        VL_RESTORER(m_param_અંક);
        VL_RESTORER(m_inRecursion);
        // Where do we add it?
        VSymEnt* aboveSymp = m_curSymp;
        અચલ string origname = Ast_ગાંઠ::dedotName(nodep->name());
        string::size_type pos = origname.rfind('.');
        જો (pos != string::npos) {
            // Flattened, find what CellInline it should live under
            અચલ string scope = origname.substr(0, pos);
            string baddot;
            VSymEnt* okSymp;
            aboveSymp
                = m_statep->findDotted(nodep->fileline(), aboveSymp, scope, baddot, okSymp, ખોટું);
            UASSERT_OBJ(aboveSymp, nodep,
                        "Can't find instance insertion point at "
                            << Ast_ગાંઠ::prettyNameQ(baddot) << " in: " << nodep->prettyNameQ());
        }
        {
            m_scope = m_scope + "." + nodep->name();
            m_curSymp = m_modSymp = m_statep->ઉમેરો_Cell(aboveSymp, m_modSymp, nodep, m_scope);
            m_inRecursion = nodep->recursive();
            // We don't report NotFoundModule, as may be a unused module in a generate
            જો (nodep->modp()) iterate(nodep->modp());
        }
    }
    શૂન્ય visit(AstCellInline* nodep) override {  // FindVisitor::
        UINFO(5, "   CELLINLINE under " << m_scope << " is " << nodep);
        VSymEnt* aboveSymp = m_curSymp;
        std::string ident = nodep->name();
        // If baz__DOT__foo__DOT__bar, we need to find baz__DOT__foo and add bar to it.
        અચલ std::string::size_type pos = ident.rfind("__DOT__");
        જો (pos != std::string::npos) {
            અચલ std::string dotted = ident.substr(0, pos);
            ident = ident.substr(pos + std::strlen("__DOT__"));
            std::string baddot;
            VSymEnt* okSymp;
            aboveSymp = m_statep->findDotted(nodep->fileline(), aboveSymp, dotted, baddot, okSymp,
                                             ખોટું);
            UASSERT_OBJ(aboveSymp, nodep,
                        "Can't find cellinline insertion point at "
                            << Ast_ગાંઠ::prettyNameQ(baddot) << " in: " << nodep->prettyNameQ());
        }
        m_statep->ઉમેરો_Inline(aboveSymp, m_modSymp, nodep, ident);
    }
    શૂન્ય visit(AstDefParam* nodep) override {  // FindVisitor::
        nodep->user1p(m_curSymp);
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstRefDType* nodep) override {  // FindVisitor::
        nodep->user1p(m_curSymp);
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstGenBlock* nodep) override {  // FindVisitor::
        UINFO(5, "   " << nodep);
        જો (nodep->name() == "" && nodep->unnamed()) {
            // Unnamed blocks are only important when they contain var
            // decls, so search for them. (Otherwise adding all the
            // unnamed#'s would just confuse tracing variables in
            // places such as tasks, where "task ...; begin ... end"
            // are common.
            માટે (Ast_ગાંઠ* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
                જો (VN_IS(itemp, Var) || VN_IS(itemp, Foreach)) {
                    std::string name;
                    અચલ std::string stepStr = m_statep->forPrimary()
                                                    ? ""
                                                    : std::to_string(m_statep->step_સંખ્યા()) + "_";
                    કરો {
                        ++m_mod_Block_અંક;
                        name = "unnamedblk" + stepStr + cvtToStr(m_mod_Block_અંક);
                        // Increment again if earlier pass of V3LinkDot claimed this name
                    } જ્યારે (m_curSymp->findIdFlat(name));
                    nodep->name(name);
                    તોડો;
                }
            }
        }
        જો (nodep->name() == "") {
            iterateChildren(nodep);
        } નહીંતર {
            VL_RESTORER(m_curSymp);
            {
                m_curSymp
                    = m_statep->ઉમેરો_Block(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
                m_curSymp->fallbackp(VL_RESTORER_PREV(m_curSymp));
                // Iterate
                iterateChildren(nodep);
            }
        }
    }
    શૂન્ય visit(Ast_ગાંઠ_Block* nodep) override {  // FindVisitor::
        UINFO(5, "   " << nodep);
        જો (nodep->name() == "" && nodep->unnamed()) {
            // Unnamed blocks are only important when they contain var
            // decls, so search for them. (Otherwise adding all the
            // unnamed#'s would just confuse tracing variables in
            // places such as tasks, where "task ...; begin ... end"
            // are common.
            અચલ auto containsDecl = [](અચલ Ast_ગાંઠ* nodesp) -> તાર્કિક {
                માટે (અચલ Ast_ગાંઠ* np = nodesp; np; np = np->nextp()) {
                    જો (VN_IS(np, Var) || VN_IS(np, Foreach)) પાછા સાચું;
                }
                પાછા ખોટું;
            };
            તાર્કિક needName = nodep->declsp() || containsDecl(nodep->stmtsp());
            જો (અચલ AstFork* અચલ forkp = VN_CAST(nodep, Fork)) {
                needName = needName || containsDecl(forkp->forksp());
            }
            જો (needName) {
                અચલ std::string stepStr
                    = m_statep->forPrimary() ? "" : std::to_string(m_statep->step_સંખ્યા()) + "_";
                std::string name;
                કરો {
                    ++m_mod_Block_અંક;
                    name = "unnamedblk" + stepStr + cvtToStr(m_mod_Block_અંક);
                    // Increment again if earlier pass of V3LinkDot claimed this name
                } જ્યારે (m_curSymp->findIdFlat(name));
                nodep->name(name);
            }
        }
        જો (nodep->name() == "") {
            iterateChildren(nodep);
        } નહીંતર {
            VL_RESTORER(m_curSymp);
            {
                m_curSymp
                    = m_statep->ઉમેરો_Block(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
                m_curSymp->fallbackp(VL_RESTORER_PREV(m_curSymp));
                // Iterate
                iterateChildren(nodep);
            }
        }
    }
    // Handle out-of-block interface method definition (IEEE 1800-2023 25.7)
    // Move task body from module into the interface's prototype task,
    // rewriting port-prefixed references in the body.
    શૂન્ય moveIfaceExportBody(Ast_ગાંઠ_FTask* nodep) {
        અચલ string& portName = nodep->ifacePortName();
        UINFO(5, "   moveIfaceExportBody: port=" << portName << " task=" << nodep->name());
        // Look up the interface port variable in the module's symbol table
        VSymEnt* અચલ portSymp = m_modSymp->findIdFallback(portName);
        જો (!portSymp) {
            nodep->v3error("Interface port not found for out-of-block definition: '"
                           << portName << "' (IEEE 1800-2023 25.7)");
            nodep->unlinkFrBack();
            VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
            પાછા;
        }
        AstVar* અચલ portp = VN_CAST(portSymp->nodep(), Var);
        જો (!portp) {
            nodep->v3fatalSrc("Out-of-block definition port is not a variable: " << portName);
            પાછા;
        }
        // Get the interface name from the port's type
        // At this stage dtypep() may not be set yet; check childDTypep() too
        AstIfaceRefDType* ifaceRefDtp = nullptr;
        જો (portp->dtypep()) { ifaceRefDtp = VN_CAST(portp->dtypep(), IfaceRefDType); }
        જો (!ifaceRefDtp && portp->childDTypep()) {
            ifaceRefDtp = VN_CAST(portp->childDTypep(), IfaceRefDType);
        }
        જો (!ifaceRefDtp) {
            nodep->v3error("Out-of-block definition port is not an interface port: '"
                           << portName << "' (IEEE 1800-2023 25.7)");
            nodep->unlinkFrBack();
            VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
            પાછા;
        }
        અચલ string ifaceName = ifaceRefDtp->ifaceName();
        UINFO(5, "   moveIfaceExportBody: iface=" << ifaceName);
        // Find the interface module by name
        Ast_ગાંઠ_Module* અચલ ifaceModp = m_statep->findModuleSym(ifaceName);
        જો (!ifaceModp) {
            nodep->v3fatalSrc("Interface not found for out-of-block definition: " << ifaceName);
            પાછા;
        }
        // Find the prototype task in the interface
        Ast_ગાંઠ_FTask* protoTaskp = nullptr;
        માટે (Ast_ગાંઠ* itemp = ifaceModp->stmtsp(); itemp; itemp = itemp->nextp()) {
            જો (Ast_ગાંઠ_FTask* અચલ ftaskp = VN_CAST(itemp, ગાંઠ_FTask)) {
                જો (ftaskp->name() == nodep->name() && ftaskp->prototype()) {
                    protoTaskp = ftaskp;
                    તોડો;
                }
            }
        }
        જો (!protoTaskp) {
            nodep->v3error("No matching export prototype found in interface "
                           << ifaceName << " for task " << nodep->prettyNameQ()
                           << " (IEEE 1800-2023 25.7)");
            nodep->unlinkFrBack();
            VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
            પાછા;
        }
        // Move body statements (non-port declarations) from the out-of-block task
        // to the prototype task. Rewrite port.X references to just X in the body.
        માટે (Ast_ગાંઠ* stmtp = nodep->stmtsp(); stmtp;) {
            Ast_ગાંઠ* અચલ nextp = stmtp->nextp();
            // Skip port declarations -- they already exist in the prototype
            જો (અચલ AstVar* અચલ varp = VN_CAST(stmtp, Var)) {
                જો (varp->isIO()) {
                    stmtp = nextp;
                    ચાલુ;
                }
            }
            // Clone this statement and rewrite port references
            Ast_ગાંઠ* અચલ newStmtp = stmtp->clone_વૃક્ષ(ખોટું);
            rewriteIfacePortRefsSingle(newStmtp, portName);
            protoTaskp->addStmtsp(newStmtp);
            stmtp = nextp;
        }
        // Mark prototype as no longer prototype (it has a body now)
        protoTaskp->prototype(ખોટું);
        // Delete the out-of-block task from the module
        nodep->unlinkFrBack();
        VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
    }

    // Rewrite AstDot nodes where lhsp is AstParseRef with the port name
    // to just the rhsp (stripping the port prefix).
    સ્થિર શૂન્ય rewriteIfacePortRefsSingle(Ast_ગાંઠ* nodep, અચલ string& portName) {
        જો (!nodep) પાછા;
        // Check if this is a non-colon Dot with lhs matching portName
        જો (AstDot* અચલ dotp = VN_CAST(nodep, Dot)) {
            જો (!dotp->colon()) {
                અચલ AstParseRef* અચલ lhsRefp = VN_CAST(dotp->lhsp(), ParseRef);
                જો (lhsRefp && lhsRefp->name() == portName) {
                    UINFO(5, "   rewriteIfacePortRefs: stripping port prefix from " << dotp);
                    Ast_ગાંઠ* અચલ rhsp = dotp->rhsp()->unlinkFrBack();
                    dotp->replaceWith(rhsp);
                    VL_DO_DANGLING(dotp->delete_વૃક્ષ(), dotp);
                    rewriteIfacePortRefsSingle(rhsp, portName);
                    પાછા;
                }
            }
        }
        // Recurse into all children
        માટે (Ast_ગાંઠ* childp : {nodep->op1p(), nodep->op2p(), nodep->op3p(), nodep->op4p()}) {
            જ્યારે (childp) {
                Ast_ગાંઠ* અચલ nextp = childp->nextp();
                rewriteIfacePortRefsSingle(childp, portName);
                childp = nextp;
            }
        }
    }

    શૂન્ય visit(Ast_ગાંઠ_FTask* nodep) override {  // FindVisitor::
        // NodeTask: Remember its name for later resolution
        UINFO(5, "   " << nodep);
        UASSERT_OBJ(m_curSymp && m_modSymp, nodep, "Function/Task not under module?");
        જો (nodep->name() == "new") m_explicitNew = સાચું;
        // Handle out-of-block interface method definition (IEEE 25.8)
        જો (!nodep->ifacePortName().empty() && m_statep->forPrimary()) {
            moveIfaceExportBody(nodep);
            પાછા;  // Task has been deleted
        }
        // Remember the existing symbol table scope
        VL_RESTORER(m_classOrPackagep);
        VL_RESTORER(m_curSymp);
        VSymEnt* upSymp = m_curSymp;

        જો (VN_IS(m_curSymp->nodep(), Class)) {
            જો (VN_AS(m_curSymp->nodep(), Class)->isInterfaceClass() && !nodep->pureVirtual()
                && !nodep->isConstructor()) {
                nodep->v3error("Interface class functions must be pure virtual"
                               << " (IEEE 1800-2023 8.26): " << nodep->prettyNameQ());
            }
            જો (m_statep->forPrimary()
                && (nodep->name() == "randomize" || nodep->name() == "srandom")) {
                nodep->v3error(nodep->prettyNameQ()
                               << " is a predefined class method; redefinition not allowed"
                                  " (IEEE 1800-2023 18.6.3)");
            }
        }
        // Change to appropriate package if extern declaration (vs definition)
        જો (nodep->classOrPackagep()) {
            // class-in-class
            AstDot* અચલ dotp = VN_CAST(nodep->classOrPackagep(), Dot);
            AstClassOrPackageRef* અચલ cpackagerefp
                = VN_CAST(nodep->classOrPackagep(), ClassOrPackageRef);
            જો (dotp) {
                AstClassOrPackageRef* અચલ lhsp = VN_AS(dotp->lhsp(), ClassOrPackageRef);
                m_statep->resolveClassOrPackage(m_curSymp, lhsp, સાચું, ખોટું,
                                                "External definition :: reference");
                AstClass* અચલ lhsclassp = VN_CAST(lhsp->classOrPackageSkipp(), Class);
                જો (!lhsclassp) {
                    nodep->v3error("Extern declaration's scope is not a defined class");
                } નહીંતર {
                    m_curSymp = m_statep->get_ગાંઠ_Sym(lhsclassp);
                    upSymp = m_curSymp;
                    AstClassOrPackageRef* અચલ rhsp = VN_AS(dotp->rhsp(), ClassOrPackageRef);
                    m_statep->resolveClassOrPackage(m_curSymp, rhsp, સાચું, ખોટું,
                                                    "External definition :: reference");
                    AstClass* અચલ rhsclassp = VN_CAST(rhsp->classOrPackageSkipp(), Class);
                    જો (!rhsclassp) {
                        nodep->v3error("Extern declaration's scope is not a defined class");
                    } નહીંતર {
                        m_curSymp = m_statep->get_ગાંઠ_Sym(rhsclassp);
                        upSymp = m_curSymp;
                        moveExternFuncDecl(nodep, rhsclassp);
                    }
                }
            } નહીંતર જો (cpackagerefp) {
                જો (!cpackagerefp->classOrPackageSkipp()) {
                    m_statep->resolveClassOrPackage(m_curSymp, cpackagerefp, સાચું, ખોટું,
                                                    "External definition :: reference");
                }
                AstClass* અચલ classp = VN_CAST(cpackagerefp->classOrPackageSkipp(), Class);
                જો (!classp) {
                    nodep->v3error("Extern declaration's scope is not a defined class");
                } નહીંતર {
                    m_curSymp = m_statep->get_ગાંઠ_Sym(classp);
                    upSymp = m_curSymp;
                    moveExternFuncDecl(nodep, classp);
                }
            } નહીંતર {
                v3fatalSrc("Unhandled extern function definition package");
            }
        }
        // Set the class as package for iteration
        જો (VN_IS(m_curSymp->nodep(), Class)) {
            m_classOrPackagep = VN_AS(m_curSymp->nodep(), Class);
        }
        // Create symbol table for the task's vars
        અચલ string name = (nodep->isExternProto() ? "extern "s : ""s) + nodep->name();
        m_curSymp = m_statep->ઉમેરો_Block(m_curSymp, name, nodep, m_classOrPackagep);
        m_curSymp->fallbackp(upSymp);
        // Convert the func's range to the output variable
        // This should probably be done in the Parser instead, as then we could
        // just attach normal signal attributes to it.
        જો (!nodep->isExternProto() && nodep->fvarp() && !VN_IS(nodep->fvarp(), Var)) {
            Ast_ગાંઠ_DType* dtypep = VN_CAST(nodep->fvarp(), ગાંઠ_DType);
            // If unspecified, function returns one bit; however when we
            // support NEW() it could also return the class reference.
            જો (dtypep) {
                dtypep->unlinkFrBack();
            } નહીંતર {
                dtypep = નવું AstBasicDType{nodep->fileline(), VBasicDTypeKwd::LOGIC};
            }
            AstVar* અચલ newvarp
                = નવું AstVar{nodep->fileline(), VVarType::VAR, nodep->name(), VFlagChildDType{},
                             dtypep};  // Not dtype resolved yet
            newvarp->direction(VDirection::OUTPUT);
            newvarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            newvarp->funcReturn(સાચું);
            newvarp->trace(ખોટું);  // Not user visible
            nodep->fvarp(newvarp);
            // Explicit insert required, as the var name shadows the upper level's task name
            m_statep->ઉમેરો_Sym(m_curSymp, newvarp->name(), newvarp, nullptr /*classOrPackagep*/);
        }
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstClocking* nodep) override {  // FindVisitor::
        VL_RESTORER(m_clockingp);
        m_clockingp = nodep;
        iterate(nodep->sentreep());
        iterateAndNextNull(nodep->itemsp());
        // If the block has no name, one cannot reference the clockvars
        VSymEnt* itSymp = nullptr;
        જો (nodep->isGlobal()  //
            && m_statep->forPrimary()) {  // else flattening may see two globals
            m_statep->checkDuplicate(m_curSymp, nodep, "__024global_clock");
            itSymp
                = m_statep->ઉમેરો_Block(m_curSymp, "__024global_clock", nodep, m_classOrPackagep);
            itSymp->fallbackp(nullptr);
        }
        જો (!nodep->name().empty()) {
            itSymp = m_statep->ઉમેરો_Block(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
            itSymp->fallbackp(nullptr);
        }
        જો (itSymp) {
            VL_RESTORER(m_curSymp);
            m_curSymp = itSymp;
            iterateAndNextNull(nodep->itemsp());
        }
    }
    શૂન્ય visit(AstClockingItem* nodep) override {  // FindVisitor::
        જો (nodep->varp()) {
            જો (m_curSymp->nodep() == m_clockingp) iterate(nodep->varp());
            પાછા;
        }
        std::string varname;
        Ast_ગાંઠ_DType* dtypep;
        જો (AstAssign* અચલ assignp = nodep->assignp()) {
            Ast_ગાંઠ_પદાવલિ* અચલ rhsp = assignp->rhsp()->unlinkFrBack();
            dtypep = નવું AstRefDType{nodep->fileline(), AstRefDType::Flag_Type_Of_પદાવલિ{},
                                     rhsp->clone_વૃક્ષ(ખોટું)};
            nodep->exprp(rhsp);
            varname = assignp->lhsp()->name();
            VL_DO_DANGLING(assignp->unlinkFrBack()->delete_વૃક્ષ(), assignp);
        } નહીંતર {
            Ast_ગાંઠ_પદાવલિ* અચલ refp = nodep->exprp();
            અચલ VSymEnt* foundp = m_curSymp->findIdFallback(refp->name());
            જો (!foundp || !foundp->nodep()) {
                refp->v3error("Corresponding variable " << refp->prettyNameQ()
                                                        << " does not exist");
                VL_DO_DANGLING(nodep->unlinkFrBack()->delete_વૃક્ષ(), nodep);
                પાછા;
            }
            varname = refp->name();
            dtypep = VN_AS(foundp->nodep(), Var)->childDTypep()->clone_વૃક્ષ(ખોટું);
        }
        AstVar* અચલ newvarp = નવું AstVar{nodep->fileline(), VVarType::MODULETEMP, varname,
                                           VFlagChildDType{}, dtypep};
        newvarp->lifetime(VLifetime::STATIC_EXPLICIT);
        nodep->varp(newvarp);
        iterate(nodep->exprp());
    }
    શૂન્ય visit(AstConstraint* nodep) override {  // FindVisitor::
        VL_RESTORER(m_curSymp);
        // Change to appropriate package if extern declaration (vs definition)
        VSymEnt* upSymp = m_curSymp;
        જો (nodep->classOrPackagep()) {
            AstClassOrPackageRef* અચલ cpackagerefp
                = VN_CAST(nodep->classOrPackagep(), ClassOrPackageRef);
            જો (!cpackagerefp) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: extern constraint definition with class-in-class");
            } નહીંતર {
                જો (!cpackagerefp->classOrPackageSkipp()) {
                    m_statep->resolveClassOrPackage(m_curSymp, cpackagerefp, સાચું, ખોટું,
                                                    "External definition :: reference");
                }
                AstClass* અચલ classp = VN_CAST(cpackagerefp->classOrPackageSkipp(), Class);
                જો (!classp) {
                    nodep->v3error("Extern declaration's scope is not a defined class");
                } નહીંતર {
                    m_curSymp = m_statep->get_ગાંઠ_Sym(classp);
                    upSymp = m_curSymp;
                    // Move it to proper spot under the target class
                    nodep->unlinkFrBack();
                    classp->addStmtsp(nodep);
                    nodep->classOrPackagep()->unlinkFrBack()->delete_વૃક્ષ();
                }
            }
        }
        // Set the class as package for iteration
        અચલ string name = (nodep->isExternProto() ? "extern "s : ""s) + nodep->name();
        m_curSymp = m_statep->ઉમેરો_Block(upSymp, name, nodep, m_classOrPackagep);
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstVar* nodep) override {  // FindVisitor::
        // Var: Remember its name for later resolution
        UASSERT_OBJ(m_curSymp && m_modSymp, nodep, "Var not under module?");
        iterateChildren(nodep);
        જો (VN_IS(m_curSymp->nodep(), Class)
            && VN_AS(m_curSymp->nodep(), Class)->isInterfaceClass() && !nodep->isParam()) {
            nodep->v3error("Interface class cannot contain non-parameter members"
                           << " (IEEE 1800-2023 8.26): " << nodep->prettyNameQ());
        }
        જો (!m_statep->forScopeCreation()) {
            // Find under either a task or the module's vars
            અચલ VSymEnt* foundp = m_curSymp->findIdFallback(nodep->name());
            જો (!foundp && m_modSymp && nodep->name() == m_modSymp->nodep()->name()) {
                foundp = m_modSymp;  // Conflicts with modname?
            }
            AstVar* અચલ findvarp = foundp ? VN_CAST(foundp->nodep(), Var) : nullptr;
            // clocking items can have duplicate names (inout)
            જો (findvarp && VN_IS(findvarp->backp(), ClockingItem)
                && VN_IS(nodep->backp(), ClockingItem)) {
                AstClockingItem* અચલ itemp = VN_AS(nodep->backp(), ClockingItem);
                AstClockingItem* અચલ finditemp = VN_AS(findvarp->backp(), ClockingItem);
                UINFO(4, "ClockCompl: " << itemp << " ;; " << finditemp);
                UINFO(4, "ClockCompV: " << nodep << " ;; " << findvarp);
                જો (*itemp->exprp()->fileline() == *finditemp->exprp()->fileline()) {
                    UASSERT_OBJ(finditemp->direction() == VDirection::આગત
                                    && itemp->direction() == VDirection::OUTPUT,
                                itemp, "Input after output?");
                    // pretend nothing found and rename
                    foundp = nullptr;
                    nodep->name("__Voutput_" + nodep->name());
                    finditemp->outputp(itemp);
                }
            }
            તાર્કિક ins = ખોટું;
            જો (!foundp) {
                ins = સાચું;
            } નહીંતર જો (!findvarp && m_curSymp->findIdFlat(nodep->name()) && !foundp->imported()) {
                nodep->v3error("Unsupported in C: Variable has same name as "
                               << LinkDotState::ગાંઠ_Text_Type(foundp->nodep()) << ": "
                               << nodep->prettyNameQ());
            } નહીંતર જો (findvarp != nodep) {
                UINFO(4, "DupVar: " << nodep << " ;; " << foundp->nodep());
                UINFO(4, "    found  cur=se" << cvtToHex(m_curSymp) << " ;; parent=se"
                                             << cvtToHex(foundp->parentp()));
                જો (foundp->parentp() == m_curSymp  // Only when on same level
                    && !foundp->imported()) {  // and not from package
                    જો (VN_IS(m_curSymp->nodep(), Clocking)) {
                        nodep->v3error("Multiple clockvars with the same name not allowed");
                        પાછા;
                    }
                    અચલ તાર્કિક nansiBad
                        = (((findvarp->isDeclTyped() || findvarp->isNet())
                            && (nodep->isDeclTyped() || nodep->isNet()))
                           || (findvarp->isIO() && nodep->isIO())  // e.g. !(output && output)
                           || findvarp->gotNansiType());  // e.g. int x && int x
                    અચલ તાર્કિક ansiBad
                        = findvarp->isAnsi() || nodep->isAnsi();  // dup illegal with ANSI
                    જો (ansiBad || nansiBad) {
                        તાર્કિક ansiWarn = ansiBad && !nansiBad;
                        જો (ansiWarn) {
                            સ્થિર પૂર્ણાંક s_didAnsiWarn = ખોટું;
                            જો (s_didAnsiWarn++) ansiWarn = ખોટું;
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
                    } નહીંતર {
                        findvarp->combineType(nodep);
                        findvarp->fileline()->modifyStateInherit(nodep->fileline());
                        findvarp->gotNansiType(સાચું);
                        UASSERT_OBJ(nodep->subDTypep(), nodep, "Var has no type");
                        જો (nodep->subDTypep()->numeric().isSigned()
                            && !findvarp->subDTypep()->numeric().isSigned()) {
                            findvarp->subDTypep()->numeric(VSigning{સાચું});
                        }
                        Ast_ગાંઠ_DType* અચલ varDtp = findvarp->subDTypep();
                        Ast_ગાંઠ_DType* અચલ otherDtp = nodep->subDTypep();
                        જો (varDtp
                            && (VN_IS(varDtp, BasicDType)
                                && VN_AS(varDtp, BasicDType)->implicit())) {
                            // Then have "input foo" and "real foo" so the
                            // dtype comes from the other side.
                            VL_DO_DANGLING(varDtp->unlinkFrBack()->delete_વૃક્ષ(), varDtp);
                            findvarp->childDTypep(otherDtp->unlinkFrBack());
                        } નહીંતર જો (!m_statep->forScopeCreation() && otherDtp && varDtp
                                   && !(VN_IS(otherDtp, BasicDType)
                                        && VN_AS(otherDtp, BasicDType)->implicit())) {
                            // otherDtp and varDtp both non-nullptr and neither are implicit
                            // Can't compare dtypes now as might contain parameters,
                            // defer to V3Width
                            AstAttrOf* અચલ newp
                                = નવું AstAttrOf{otherDtp->fileline(), VAttrType::VAR_PORT_DTYPE,
                                                otherDtp->unlinkFrBack()};
                            findvarp->addAttrsp(newp);
                        }
                    }
                    VL_DO_DANGLING(nodep->unlinkFrBack()->delete_વૃક્ષ(), nodep);
                } નહીંતર {
                    // User can disable the message at either point
                    જો (!(m_ftaskp && m_ftaskp->dpiImport())
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
                    ins = સાચું;
                }
            }
            જો (ins) {
                જો (m_statep->forPrimary() && nodep->isGParam()
                    && VN_IS(m_modSymp->nodep(), Module)
                    && (m_statep->મૂળ_Entp()->nodep() == m_modSymp->parentp()->nodep())) {
                    // This is the toplevel module. Check for command line overwrites of parameters
                    // We first search if the parameter is overwritten and then replace it with a
                    // new value.
                    જો (v3Global.opt.hasParameter(nodep->name())) {
                        અચલ string svalue = v3Global.opt.parameter(nodep->name());
                        જો (AstConst* અચલ valuep
                            = AstConst::parseParamLiteral(nodep->fileline(), svalue)) {
                            UINFO(9, "       replace parameter " << nodep);
                            UINFO(9, "       with " << valuep);
                            જો (nodep->valuep()) ધકેલો_Deletep(nodep->valuep()->unlinkFrBack());
                            nodep->valuep(valuep);
                            // Do not warn ENUMVALUE when overriding an enum param from the cli.
                            nodep->fileline()->warnOff(V3ErrorCode::ENUMVALUE, સાચું);
                        }
                    }
                }
                m_statep->ઉમેરો_Sym(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
                જો (m_statep->forPrimary() && nodep->isGParam()) {
                    ++m_param_અંક;
                    VSymEnt* અચલ symp
                        = m_statep->ઉમેરો_Sym(m_curSymp, "__paramNumber" + cvtToStr(m_param_અંક),
                                              nodep, m_classOrPackagep);
                    symp->exported(ખોટું);
                }
            }
        }
    }
    શૂન્ય visit(AstTypedef* nodep) override {  // FindVisitor::
        UASSERT_OBJ(m_curSymp, nodep, "Typedef not under module/package/$unit");
        જો (VN_IS(m_classOrPackagep, Class)) nodep->isUnderClass(સાચું);
        iterateChildren(nodep);
        m_statep->ઉમેરો_Sym(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
    }
    શૂન્ય visit(AstTypedefFwd* nodep) override {  // FindVisitor::
        UASSERT_OBJ(m_curSymp, nodep, "Typedef not under module/package/$unit");
        iterateChildren(nodep);
        // No need to insert, only the real typedef matters, but need to track for errors
        nodep->user1p(m_curSymp);
    }
    શૂન્ય visit(Ast_ગાંઠ_UOr_Struct_DType* nodep) override {  // FindVisitor::
        UASSERT_OBJ(m_curSymp, nodep, "Struct/union dtype not under module/package/$unit");
        VL_RESTORER(m_curSymp);
        m_curSymp = m_statep->ઉમેરો_Block(m_curSymp, "__Vdtype" + cvtToStr(nodep->unique_અંક()),
                                          nodep, m_classOrPackagep);
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstMemberDType* nodep) override {  // FindVisitor::
        iterateChildren(nodep);
        m_statep->ઉમેરો_Sym(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
    }
    શૂન્ય visit(AstParamTypeDType* nodep) override {  // FindVisitor::
        UASSERT_OBJ(m_curSymp, nodep, "Parameter type not under module/package/$unit");

        // Replace missing param types with provided hierarchical type params.
        જો (!m_hierParamsName.empty()) {
            જો (અચલ VSymEnt* અચલ typedefEntp = m_curSymp->findIdFallback(m_hierParamsName)) {
                અચલ AstModule* modp = VN_CAST(typedefEntp->nodep(), Module);

                માટે (અચલ Ast_ગાંઠ* stmtp = modp ? modp->stmtsp() : nullptr; stmtp;
                     stmtp = stmtp->nextp()) {
                    અચલ AstTypedef* tdefp = VN_CAST(stmtp, Typedef);

                    જો (tdefp && tdefp->name() == nodep->name() && m_statep->forPrimary()) {
                        UINFO(8, "Replacing type of" << nodep << "  -->with " << tdefp);
                        Ast_ગાંઠ_DType* અચલ newDtp = tdefp->childDTypep();
                        Ast_ગાંઠ_DType* અચલ oldDtp = nodep->childDTypep();

                        oldDtp->replaceWith(newDtp->clone_વૃક્ષ(ખોટું));
                        VL_DO_DANGLING(oldDtp->delete_વૃક્ષ(), oldDtp);
                    }
                }
            }
        }

        iterateChildren(nodep);
        m_statep->ઉમેરો_Sym(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
        જો (m_statep->forPrimary() && nodep->isGParam()) {
            ++m_param_અંક;
            VSymEnt* અચલ symp = m_statep->ઉમેરો_Sym(
                m_curSymp, "__paramNumber" + cvtToStr(m_param_અંક), nodep, m_classOrPackagep);
            symp->exported(ખોટું);
        }
    }
    શૂન્ય visit(AstCFunc* nodep) override {  // FindVisitor::
        // For dotted resolution, ignore all AstVars under functions, otherwise shouldn't exist
        UASSERT_OBJ(!m_statep->forScopeCreation(), nodep, "No CFuncs expected in tree yet");
    }
    શૂન્ય visit(AstEnumItem* nodep) override {  // FindVisitor::
        // EnumItem: Remember its name for later resolution
        iterateChildren(nodep);
        // Find under either a task or the module's vars
        અચલ VSymEnt* foundp = m_curSymp->findIdFallback(nodep->name());
        જો (!foundp && m_modSymp && nodep->name() == m_modSymp->nodep()->name()) {
            foundp = m_modSymp;  // Conflicts with modname?
        }
        AstEnumItem* અચલ findvarp = foundp ? VN_CAST(foundp->nodep(), EnumItem) : nullptr;
        તાર્કિક ins = ખોટું;
        જો (!foundp) {
            ins = સાચું;
        } નહીંતર જો (findvarp != nodep) {
            UINFO(4, "DupVar: " << nodep << " ;; " << foundp);
            જો (foundp->parentp() == m_curSymp  // Only when on same level
                && !foundp->imported()) {  // and not from package
                nodep->v3error("Duplicate declaration of enum value: "
                               << nodep->prettyName() << '\n'
                               << nodep->warnContextPrimary() << '\n'
                               << foundp->nodep()->warnOther()
                               << "... Location of original declaration\n"
                               << foundp->nodep()->warnContextSecondary());
            } નહીંતર {
                // User can disable the message at either point
                જો (!nodep->fileline()->warnIsOff(V3ErrorCode::VARHIDDEN)
                    && !foundp->nodep()->fileline()->warnIsOff(V3ErrorCode::VARHIDDEN)) {
                    nodep->v3warn(VARHIDDEN,
                                  "Declaration of enum value hides declaration in upper scope: "
                                      << nodep->prettyName() << '\n'
                                      << nodep->warnContextPrimary() << '\n'
                                      << foundp->nodep()->warnOther()
                                      << "... Location of original declaration\n"
                                      << foundp->nodep()->warnContextSecondary());
                }
                ins = સાચું;
            }
        }
        જો (ins) m_statep->ઉમેરો_Sym(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
    }
    શૂન્ય visit(AstPackageImport* nodep) override {  // FindVisitor::
        UINFO(4, "  Link: " << nodep);
        જો (!nodep->packagep()) પાછા;  // Errored in V3LinkCells
        VSymEnt* અચલ srcp = m_statep->get_ગાંઠ_Sym(nodep->packagep());
        જો (nodep->name() == "*") {
            જો (nodep->packagep() != v3Global.rootp()->stdPackagep()) {
                જો (m_curSymp == m_statep->dunitEntp()) {
                    nodep->v3warn(IMPORTSTAR,
                                  "'import::*' in $unit scope may pollute global namespace");
                }
            }
        } નહીંતર જો (!nodep->resolvedClassp()) {
            VSymEnt* અચલ impp = srcp->findIdFlat(nodep->name());
            જો (!impp) {
                nodep->v3error("Import object not found: " << nodep->prettyPkgNameQ());
            } નહીંતર જો (nodep->resolvedClassp()) {
                UASSERT_OBJ(nodep->resolvedClassp()->classOrPackageSkipp() == impp->nodep(), nodep,
                            "Import should resolve to the same node");
            } નહીંતર જો (AstClass* અચલ classp = VN_CAST(impp->nodep(), Class)) {
                nodep->resolvedClassp(
                    નવું AstClassOrPackageRef{nodep->fileline(), classp->name(), classp, nullptr});
            }
        }
        m_curSymp->importFromPackage(m_statep->symsp(), srcp, nodep->name());
        UINFO(9, "    Link Done: " << nodep);
        // No longer needed, but can't delete until any multi-instantiated modules are expanded
    }
    શૂન્ય visit(AstPackageExport* nodep) override {  // FindVisitor::
        UINFO(9, "  Link: " << nodep);
        જો (!nodep->packagep()) પાછા;  // Errored in V3LinkCells
        VSymEnt* અચલ srcp = m_statep->get_ગાંઠ_Sym(nodep->packagep());
        જો (nodep->name() != "*") {
            VSymEnt* અચલ impp = srcp->findIdFlat(nodep->name());
            જો (!impp) {
                nodep->v3error("Export object not found: '" << nodep->packagep()->prettyName()
                                                            << "::" << nodep->prettyName() << "'");
            }
        }
        m_curSymp->exportFromPackage(m_statep->symsp(), srcp, nodep->name());
        UINFO(9, "    Link Done: " << nodep);
        // No longer needed, but can't delete until any multi-instantiated modules are expanded
    }
    શૂન્ય visit(AstPackageExportStarStar* nodep) override {  // FindVisitor::
        UINFO(4, "  Link: " << nodep);
        m_curSymp->exportStarStar(m_statep->symsp());
        // No longer needed, but can't delete until any multi-instantiated modules are expanded
    }

    શૂન્ય visit(Ast_ગાંઠ_Foreach* nodep) override {  // FindVisitor::
        // Symbol table needs nodep->name() as the index variable's name
        VL_RESTORER(m_curSymp);
        {
            ++m_mod_With_અંક;
            m_curSymp = m_statep->ઉમેરો_Block(m_curSymp, "__Vforeach" + cvtToStr(m_mod_With_અંક),
                                              nodep, m_classOrPackagep);
            m_curSymp->fallbackp(VL_RESTORER_PREV(m_curSymp));
            માટે (Ast_ગાંઠ *nextp, *argp = nodep->headerp()->elementsp(); argp; argp = nextp) {
                nextp = argp->nextp();
                AstVar* argrefp = nullptr;
                જો (AstParseRef* અચલ parserefp = VN_CAST(argp, ParseRef)) {
                    // IEEE 1800-2023 12.7.3: foreach loop variable type shall be int (2-state)
                    // This might get changed in V3Width when types resolve (e.g., for assoc
                    // arrays)
                    argrefp = નવું AstVar{parserefp->fileline(), VVarType::BLOCKTEMP,
                                         parserefp->name(), argp->findIntDType()};
                    argrefp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                    parserefp->replaceWith(argrefp);
                    VL_DO_DANGLING2(parserefp->delete_વૃક્ષ(), parserefp, argp);
                    // Insert argref's name into symbol table
                    m_statep->ઉમેરો_Sym(m_curSymp, argrefp->name(), argrefp, nullptr);
                } નહીંતર જો (અચલ auto largrefp = VN_CAST(argp, Var)) {
                    argrefp = largrefp;
                    // Insert argref's name into symbol table
                    m_statep->ઉમેરો_Sym(m_curSymp, argrefp->name(), argrefp, nullptr);
                } નહીંતર જો (VN_IS(argp, Empty)) {
                } નહીંતર {
                    argp->v3error("'foreach' loop variable expects simple variable name");
                }
            }
            iterateChildren(nodep);
        }
    }

    શૂન્ય visit(AstRandSequence* nodep) override {
        VL_RESTORER(m_curSymp);
        m_curSymp = m_statep->ઉમેરો_Block(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstRSProd* nodep) override {
        VL_RESTORER(m_curSymp);
        m_curSymp = m_statep->ઉમેરો_Block(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
        જો (nodep->fvarp())
            nodep->fvarp()->v3warn(E_UNSUPPORTED,
                                   "Unsupported: randsequence production function variable");
        // Mark formal ports as port-checked so the primary resolve pass does not
        // flag them with "does not appear in port list" -- V3RandSequence will
        // later move them onto the generated task as real input ports.
        માટે (Ast_ગાંઠ* itp = nodep->portsp(); itp; itp = itp->nextp()) {
            VN_AS(itp, Var)->user4(સાચું);
        }
        iterateChildren(nodep);
    }

    શૂન્ય visit(AstWithParse* nodep) override {  // FindVisitor::
        // Change WITHPARSE(FUNCREF, equation) to FUNCREF(WITH(equation))
        Ast_ગાંઠ_FTask_Ref* funcrefp = VN_CAST(nodep->funcrefp(), ગાંઠ_FTask_Ref);
        જો (અચલ AstDot* અચલ dotp = VN_CAST(nodep->funcrefp(), Dot))
            funcrefp = VN_CAST(dotp->rhsp(), ગાંઠ_FTask_Ref);
        UASSERT_OBJ(funcrefp, nodep, "'with' only can operate on a function/task");
        string name = funcrefp->name() == "randomize" ? "__Vrandwith_obj" : "item";
        File_લીટી* argFl = nodep->fileline();
        AstArg* અચલ argsp = funcrefp->argsp();
        જો (argsp) {
            argsp->unlinkFrBackWithNext();
            જો (funcrefp->name() != "randomize") {
                જો (અચલ auto parserefp = VN_CAST(argsp->exprp(), ParseRef)) {
                    name = parserefp->name();
                    argFl = parserefp->fileline();
                } નહીંતર {
                    argsp->v3error("'with' function expects simple variable name");
                }
                જો (argsp->nextp()) {
                    argsp->nextp()->v3error("'with' function expects only up to one argument");
                }
                VL_DO_DANGLING(argsp->delete_વૃક્ષ(), argsp);
            }
        }
        // Type depends on the method used, let V3Width figure it out later
        // 'with (...) { ... }' constraint_block form is randomize-only
        // (IEEE 1800-2023 7.12 vs. 18.7); array methods take 'with (expr)'.
        જો (nodep->restricted() && funcrefp->name() != "randomize") {
            nodep->v3error("'with (...) { ... }' constraint block is only valid for"
                           " randomize() (IEEE 1800-2023 18.7)");
            funcrefp->addArgsp(argsp);
            nodep->replaceWith(nodep->funcrefp()->unlinkFrBack());
            VL_DO_DANGLING(nodep->delete_વૃક્ષ(), nodep);
            પાછા;
        }
        અચલ તાર્કિક restrictedRandomize = nodep->restricted();
        જો (nodep->exprsp() || nodep->constraintsp()
            || restrictedRandomize) {  // Else empty expression and pretend no "with"
            AstLambdaArgRef* અચલ indexArgRefp
                = નવું AstLambdaArgRef{argFl, name + "__DOT__index", સાચું};
            AstLambdaArgRef* અચલ કિંમત_Arg_Refp = નવું AstLambdaArgRef{argFl, name, ખોટું};
            AstWith* અચલ newp
                = નવું AstWith{nodep->fileline(), indexArgRefp, કિંમત_Arg_Refp, nullptr};
            // Harvest the identifier_list into AstWith; grammar guarantees AstParseRef.
            જો (restrictedRandomize) {
                newp->restricted(સાચું);
                જ્યારે (Ast_ગાંઠ* અચલ itemp = nodep->exprsp()) {
                    newp->addRestrictedName(VN_AS(itemp, ParseRef)->name());
                    itemp->unlinkFrBack();
                    VL_DO_DANGLING(ધકેલો_Deletep(itemp), itemp);
                }
            }
            Ast_ગાંઠ* પદાવલિ_Or_Constraintsp = nullptr;
            જો (nodep->exprsp()) પદાવલિ_Or_Constraintsp = nodep->exprsp()->unlinkFrBackWithNext();
            જો (nodep->constraintsp())
                પદાવલિ_Or_Constraintsp = Ast_ગાંઠ::addNext(
                    પદાવલિ_Or_Constraintsp, nodep->constraintsp()->unlinkFrBackWithNext());
            newp->addExprp(પદાવલિ_Or_Constraintsp);  // addExprp() tolerates nullptr
            funcrefp->withp(newp);
        }
        funcrefp->addArgsp(argsp);
        nodep->replaceWith(nodep->funcrefp()->unlinkFrBack());
        VL_DO_DANGLING(nodep->delete_વૃક્ષ(), nodep);
    }
    શૂન્ય visit(AstWith* nodep) override {  // FindVisitor::
        // Symbol table needs nodep->name() as the index variable's name
        // Iteration will pickup the AstVar we made under AstWith
        VL_RESTORER(m_curSymp);
        ++m_mod_With_અંક;
        m_curSymp = m_statep->ઉમેરો_Block(m_curSymp, "__Vwith" + cvtToStr(m_mod_With_અંક), nodep,
                                          m_classOrPackagep);
        m_curSymp->fallbackp(VL_RESTORER_PREV(m_curSymp));
        UASSERT_OBJ(nodep->indexArgRefp(), nodep, "Missing lambda argref");
        UASSERT_OBJ(nodep->કિંમત_Arg_Refp(), nodep, "Missing lambda argref");
        // Insert argref's name into symbol table
        m_statep->ઉમેરો_Sym(m_curSymp, nodep->કિંમત_Arg_Refp()->name(), nodep->કિંમત_Arg_Refp(),
                            nullptr);
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstCoverWith* nodep) override {  // FindVisitor::
        // The range list is outside the scope of the filter, which declares 'item'
        iterateAndNextNull(nodep->subp());
        VL_RESTORER(m_curSymp);
        ++m_mod_With_અંક;
        m_curSymp = m_statep->ઉમેરો_Block(m_curSymp, "__VcoverWith" + cvtToStr(m_mod_With_અંક),
                                          nodep, m_classOrPackagep);
        m_curSymp->fallbackp(VL_RESTORER_PREV(m_curSymp));
        iterateAndNextNull(nodep->itemp());
        iterateAndNextNull(nodep->filterp());
    }
    શૂન્ય visit(AstCoverCrossWith* nodep) override {  // FindVisitor::
        // The selection, the 'matches' count, and the cross items' types, of their coverpoints'
        // expressions with any nested scopes, are outside the filter's scope, which declares the
        // items.  As visit(AstVar) would resolve a type in the scope declaring its variable, the
        // items, generated and unique, are inserted directly.
        iterateAndNextNull(nodep->selectp());
        iterateAndNextNull(nodep->countp());
        માટે (AstVar* varp = nodep->varsp(); varp; varp = VN_AS(varp->nextp(), Var)) {
            iterateAndNextNull(varp->childDTypep());
        }
        VL_RESTORER(m_curSymp);
        ++m_mod_With_અંક;
        m_curSymp = m_statep->ઉમેરો_Block(m_curSymp, "__VcoverWith" + cvtToStr(m_mod_With_અંક),
                                          nodep, m_classOrPackagep);
        m_curSymp->fallbackp(VL_RESTORER_PREV(m_curSymp));
        માટે (AstVar* varp = nodep->varsp(); varp; varp = VN_AS(varp->nextp(), Var)) {
            m_statep->ઉમેરો_Sym(m_curSymp, varp->name(), varp, m_classOrPackagep);
        }
        iterateAndNextNull(nodep->filterp());
    }

    શૂન્ય visit(Ast_ગાંઠ* nodep) override { iterateChildren(nodep); }  // FindVisitor::

    શૂન્ય handleUnvisitedVirtIfaces() {
        Ast_ગાંઠ_Module* અચલ ટોચ = v3Global.rootp()->modulesp();
        m_curSymp = m_statep->get_ગાંઠ_Sym(ટોચ);
        માટે (AstIface* ifacep : m_virtIfaces) {
            જો (!ifacep->user4()) {  // virtual interface never assigned any actual interface
                // Create dummy cell to keep the virtual interface alive
                // (later stages assume that non-dead interface has associated cell)
                AstCell* અચલ ifacecellp = નવું AstCell{ifacep->fileline(),
                                                        ifacep->fileline(),
                                                        "__VdummyInstOf" + ifacep->name(),
                                                        ifacep->name(),
                                                        nullptr,
                                                        nullptr,
                                                        nullptr};
                ifacecellp->modp(ifacep);
                ટોચ->addStmtsp(ifacecellp);

                iterate(ifacecellp);
            }
        }
    }

જાહેર:
    // CONSTRUCTORS
    LinkDotFindVisitor(AstNetlist* rootp, LinkDotState* statep)
        : m_statep{statep} {
        UINFO(4, __FUNCTION__ << ": ");
        iterate(rootp);

        જો (!m_virtIfaces.empty()) handleUnvisitedVirtIfaces();
        માટે (AstScope* અચલ scopep : m_scopes) scopep->modp()->addStmtsp(scopep);
        જો (m_statep->forScopeCreation()) {
            V3Stats::addStat("LinkDot, Scope visits", m_statScopeVisits);
        }
    }
    ~LinkDotFindVisitor() override = મૂળભૂત;
};

//======================================================================

વર્ગ LinkDotFindIfaceVisitor final : જાહેર VNVisitor {
    // NODE STATE
    //  *::user1p()             -> See LinkDotState

    // STATE - for current visit position (use VL_RESTORER)
    LinkDotState* અચલ m_statep;  // State to pass between visitors, including symbol table
    Ast_ગાંઠ* m_declp = nullptr;  // Current declaring object that may soon contain IfaceRefDType

    // METHODS
    અચલ VSymEnt* findNonDeclSym(VSymEnt* symp, અચલ string& name) {
        // Find if there is a symbol of given name, ignoring the node that is declaring
        // it (m_declp) itself.  Thus if searching for "ifc" (an interface):
        //
        // module x;  // Finishes here and finds the typedef
        //   typedef foo ifc;
        //   ifc ifc;   // symp starts pointing here, but matches m_declp
        જ્યારે (symp) {
            અચલ VSymEnt* અચલ foundp = symp->findIdFlat(name);
            જો (foundp && foundp->nodep() != m_declp) પાછા foundp;
            symp = symp->fallbackp();
        }
        પાછા nullptr;
    }

    // VISITORS
    શૂન્ય visit(AstRefDType* nodep) override {  // FindIfaceVisitor::
        જો (m_statep->forPrimary() && !nodep->classOrPackagep()) {
            UINFO(9, "  FindIfc: " << nodep);
            // If under a var, ignore the var itself as might be e.g. "intf intf;"
            // Critical tests:
            //   t_interface_param_genblk.v  // Checks this does make interface
            //   t_interface_hidden.v  // Checks this doesn't making interface when hidden
            જો (m_statep->exists_ગાંઠ_Sym(nodep)) {
                VSymEnt* symp = m_statep->get_ગાંઠ_Sym(nodep);
                અચલ VSymEnt* foundp = findNonDeclSym(symp, nodep->name());
                Ast_ગાંઠ* foundNodep = nullptr;
                // This: v4make test_regress/t/t_interface_param_genblk.py --debug
                // --debugi-V3LinkDot 9 Passes with this commented out:
                જો (foundp) foundNodep = foundp->nodep();
                જો (!foundNodep) foundNodep = m_statep->findModuleSym(nodep->name());
                જો (foundNodep) UINFO(9, "    Ifc foundNodep " << foundNodep);
                જો (AstIface* અચલ defp = VN_CAST(foundNodep, Iface)) {
                    // Must be found as module name, and not hidden/ by normal symbol (foundp)
                    AstIfaceRefDType* અચલ newp
                        = નવું AstIfaceRefDType{nodep->fileline(), "", nodep->name()};
                    જો (nodep->paramsp())
                        newp->addParamsp(nodep->paramsp()->unlinkFrBackWithNext());
                    newp->ifacep(defp);
                    newp->user1u(nodep->user1u());
                    UINFO(9, "    Resolved interface " << nodep << "  =>  " << defp);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
                    પાછા;
                }
            }
        }
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstVar* nodep) override {  // FindIfaceVisitor::
        VL_RESTORER(m_declp);
        m_declp = nodep;
        iterateChildren(nodep);
        AstIfaceRefDType* અચલ ifacerefp = LinkDotState::ifaceRefFromArray(nodep->subDTypep());
        જો (ifacerefp && m_statep->exists_ગાંઠ_Sym(nodep)) {
            // Can't resolve until interfaces and modport names are
            // known; see notes at top
            UINFO(9, "  FindIfc Var IfaceRef " << ifacerefp);
            જો (!ifacerefp->isVirtual()) nodep->setIfaceRef();
            VSymEnt* અચલ symp = m_statep->get_ગાંઠ_Sym(nodep);
            m_statep->ઉમેરો_Iface_Var_Sym(symp);
        }
    }
    શૂન્ય visit(Ast_ગાંઠ* nodep) override { iterateChildren(nodep); }  // FindIfaceVisitor::

જાહેર:
    // CONSTRUCTORS
    LinkDotFindIfaceVisitor(AstNetlist* rootp, LinkDotState* statep)
        : m_statep{statep} {
        UINFO(4, __FUNCTION__ << ": ");
        iterate(rootp);
    }
    ~LinkDotFindIfaceVisitor() override = મૂળભૂત;
};

//======================================================================

વર્ગ LinkDotParamVisitor final : જાહેર VNVisitor {
    // NODE STATE
    //  *::user1p()             -> See LinkDotState
    //  *::user2p()             -> See LinkDotState
    //  *::user4()              -> See LinkDotState

    // STATE
    LinkDotState* અચલ m_statep;  // State to pass between visitors, including symbol table
    Ast_ગાંઠ_Module* m_modp = nullptr;  // Current module

    શૂન્ય pin_Implicit_પદાવલિ_Recurse(Ast_ગાંઠ* nodep) {
        // Under a pin, Check interconnect expression for a pin reference or a concat.
        // Create implicit variable as needed
        જો (VN_IS(nodep, Dot)) {  // Not creating a simple implied type,
            // and implying something else would just confuse later errors
        } નહીંતર જો (VN_IS(nodep, VarRef) || VN_IS(nodep, ParseRef)) {
            // To prevent user errors, we should only do single bit
            // implicit vars, however some netlists (MIPS) expect single
            // bit implicit wires to get created with range 0:0 etc.
            m_statep->implicitOkAdd(m_modp, nodep->name());
        }
        // These are perhaps a little too generous, as a SELect of siga[sigb]
        // perhaps shouldn't create an implicit variable.  But, we've warned...
        નહીંતર {
            જો (Ast_ગાંઠ* અચલ refp = nodep->op1p()) pin_Implicit_પદાવલિ_Recurse(refp);
            જો (Ast_ગાંઠ* અચલ refp = nodep->op2p()) pin_Implicit_પદાવલિ_Recurse(refp);
            જો (Ast_ગાંઠ* અચલ refp = nodep->op3p()) pin_Implicit_પદાવલિ_Recurse(refp);
            જો (Ast_ગાંઠ* અચલ refp = nodep->op4p()) pin_Implicit_પદાવલિ_Recurse(refp);
        }
        // Continue along a list (e.g. the terminals under AstImplicit), also after a reference
        જો (Ast_ગાંઠ* અચલ refp = nodep->nextp()) pin_Implicit_પદાવલિ_Recurse(refp);
    }

    // VISITORS
    શૂન્ય visit(AstTypeTable*) override {}  // ParamVisitor::
    શૂન્ય visit(Ast_ગાંઠ_Module* nodep) override {  // ParamVisitor::
        UINFO(5, "   " << nodep);
        જો ((nodep->dead() || !nodep->user4()) && !nodep->hierParams()) {
            UINFO(4, "Mark dead module " << nodep);
            UASSERT_OBJ(!m_statep->forScopeCreation(), nodep,
                        "Dead module persisted past where should have removed");
            // Don't remove now, because we may have a tree of
            // parameterized modules with VARXREFs into the deleted module
            // region.  V3Dead should cleanup.
            // Downstream visitors up until V3Dead need to check for nodep->dead.
            nodep->dead(સાચું);
            પાછા;
        }
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstPin* nodep) override {  // ParamVisitor::
        // Pin: Link to submodule's port
        // Deal with implicit definitions - do before Resolve visitor as may
        // be referenced above declaration
        જો (nodep->exprp()  // Else specifically unconnected pin
            && !nodep->svImplicit()) {  // SV 19.11.3: .name pins don't allow implicit decls
            pin_Implicit_પદાવલિ_Recurse(nodep->exprp());
        }
    }
    શૂન્ય visit(AstDefParam* nodep) override {  // ParamVisitor::
        iterateChildren(nodep);
        nodep->v3warn(DEFPARAM, "defparam is deprecated (IEEE 1800-2023 C.4.1)\n"
                                    << nodep->warnMore()
                                    << "... Suggest use instantiation with #(."
                                    << nodep->prettyName() << "(...etc...))");
        તાર્કિક hasPartSelect = ખોટું;
        અચલ string path = extractDottedPath(nodep->pathp(), hasPartSelect);
        UASSERT_OBJ(!hasPartSelect && !path.empty(), nodep, "Unexpected defparam path shape");
        string baddot;
        VSymEnt* okSymp = nullptr;
        VSymEnt* અચલ foundp = m_statep->findDotted(
            nodep->fileline(), m_statep->get_ગાંઠ_Sym(nodep), path, baddot, okSymp, સાચું);
        AstCell* અચલ cellp = foundp ? VN_AS(foundp->nodep(), Cell) : nullptr;
        જો (!cellp) {
            nodep->v3error("In defparam, instance " << path << " never declared");
        } નહીંતર {
            Ast_ગાંઠ_પદાવલિ* અચલ exprp = nodep->rhsp()->unlinkFrBack();
            UINFO(9, "Defparam cell " << path << "." << nodep->name() << " attach-to " << cellp
                                      << "  <= " << exprp);
            // Don't need to check the name of the defparam exists.  V3Param does.
            AstPin* અચલ pinp = નવું AstPin{nodep->fileline(),
                                            -1,  // Pin# not relevant
                                            nodep->name(), exprp};
            pinp->param(સાચું);
            pinp->paramPath(path);
            cellp->addParamsp(pinp);
        }
        VL_DO_DANGLING(nodep->unlinkFrBack()->delete_વૃક્ષ(), nodep);
    }
    શૂન્ય visit(AstPort* nodep) override {  // ParamVisitor::
        // Port: Stash the pin number
        // Need to set pin numbers after varnames are created
        // But before we do the final resolution based on names
        VSymEnt* અચલ foundp = m_statep->get_ગાંઠ_Sym(m_modp)->findIdFlat(nodep->name());
        AstVar* અચલ refp = foundp ? VN_CAST(foundp->nodep(), Var) : nullptr;
        જો (!foundp) {
            nodep->v3error(
                "Input/output/inout declaration not found for port: " << nodep->prettyNameQ());
        } નહીંતર જો (!refp || (!refp->isIO() && !refp->isIfaceRef())) {
            nodep->v3error("Pin is not an in/out/inout/interface: " << nodep->prettyNameQ());
        } નહીંતર {
            જો (refp->user4()) {
                nodep->v3error("Duplicate declaration of port: "
                               << nodep->prettyNameQ() << '\n'
                               << nodep->warnContextPrimary() << '\n'
                               << refp->warnOther() << "... Location of original declaration\n"
                               << refp->warnContextSecondary());
            }
            refp->user4(સાચું);
            VSymEnt* અચલ symp = m_statep->ઉમેરો_Sym(m_statep->get_ગાંઠ_Sym(m_modp),
                                                      "__pinNumber" + cvtToStr(nodep->pin_અંક()),
                                                      refp, nullptr /*classOrPackagep*/);
            symp->exported(ખોટું);
            refp->pin_અંક(nodep->pin_અંક());
            // Put the variable where the port is, so that variables stay
            // in pin number sorted order. Otherwise hierarchical or JSON
            // may botch by-position instances.
            nodep->addHereThisAsNext(refp->unlinkFrBack());
        }
        // Ports not needed any more
        VL_DO_DANGLING(nodep->unlinkFrBack()->delete_વૃક્ષ(), nodep);
    }
    શૂન્ય visit(AstAssignW* nodep) override {  // ParamVisitor::
        // Deal with implicit definitions
        // We used to nodep->allowImplicit() here, but it turns out
        // normal "assigns" can also make implicit wires.  Yuk.
        pin_Implicit_પદાવલિ_Recurse(nodep->lhsp());
        iterateChildren(nodep);
    }

    શૂન્ય visit(AstImplicit* nodep) override {  // ParamVisitor::
        // Unsupported gates need implicit creation
        pin_Implicit_પદાવલિ_Recurse(nodep->exprsp());
        // We're done with implicit gates
        VL_DO_DANGLING(nodep->unlinkFrBack()->delete_વૃક્ષ(), nodep);
    }
    શૂન્ય visit(AstClassOrPackageRef* nodep) override {  // ParamVisitor::
        જો (auto* અચલ fwdp = VN_CAST(nodep->classOrPackageNodep(), TypedefFwd)) {
            // Relink forward definitions to the "real" definition
            VSymEnt* અચલ foundp = m_statep->get_ગાંઠ_Sym(fwdp)->findIdFallback(fwdp->name());
            જો (foundp && (VN_IS(foundp->nodep(), Class) || VN_IS(foundp->nodep(), Package))) {
                nodep->classOrPackagep(VN_AS(foundp->nodep(), ગાંઠ_Module));
            } નહીંતર જો (foundp && VN_IS(foundp->nodep(), ParamTypeDType)) {
                UASSERT(m_statep->forPrimary(), "Param types should have been resolved");
                nodep->classOrPackageNodep(VN_AS(foundp->nodep(), ParamTypeDType));
            } નહીંતર {
                જો (foundp) UINFO(1, "found nodep = " << foundp->nodep());
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
    શૂન્ય visit(AstPull* nodep) override {  // ParamVisitor::
        // Deal with implicit definitions
        // We used to nodep->allowImplicit() here, but it turns out
        // normal "assigns" can also make implicit wires.  Yuk.
        pin_Implicit_પદાવલિ_Recurse(nodep->lhsp());
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstTypedefFwd* nodep) override {  // ParamVisitor::
        VSymEnt* અચલ foundp = m_statep->get_ગાંઠ_Sym(nodep)->findIdFallback(nodep->name());
        જો (foundp) {
            // If the typedef was earlier in source order (tokenNum), then remember that earlier
            // point to avoid error something wasn't declared
            // Might be forward declaring something odd (with declTokenNumSetMin not implemented,
            // but should be harmless to ignore as this is just for error detection
            foundp->nodep()->decl_Token_અંક_Set_Min(nodep->fileline()->token_અંક());
        } નહીંતર જો (v3Global.opt.pedantic()) {
            // We only check it was ever really defined in pedantic mode, as it
            // might have been in a header file referring to a module we never
            // needed so there are false positives
            nodep->v3error(
                "Forward typedef unused or does not resolve to a data type (IEEE 1800-2023 6.18): "
                << nodep->prettyNameQ());
        }
        // We only needed the forward declaration in order to parse correctly.
        // Delete later as may be ClassOrPackageRef's still pointing to it
        VL_DO_DANGLING(ધકેલો_Deletep(nodep->unlinkFrBack()), nodep);
    }

    શૂન્ય visit(Ast_ગાંઠ* nodep) override { iterateChildren(nodep); }  // ParamVisitor::

જાહેર:
    // CONSTRUCTORS
    LinkDotParamVisitor(AstNetlist* rootp, LinkDotState* statep)
        : m_statep{statep} {
        UINFO(4, __FUNCTION__ << ": ");
        iterate(rootp);
    }
    ~LinkDotParamVisitor() override = મૂળભૂત;
};

//======================================================================

વર્ગ LinkDotScopeVisitor final : જાહેર VNVisitor {

    // STATE
    LinkDotState* અચલ m_statep;  // State to pass between visitors, including symbol table
    અચલ AstScope* m_scopep = nullptr;  // The current scope
    VSymEnt* m_modSymp = nullptr;  // Symbol entry for current module
    // Deferred AliasScope processing - must be done outer-to-inner for correct alias resolution
    std::vector<std::pair<AstAliasScope*, VSymEnt*>> m_deferredAliasScopes;

    // METHODS
જાહેર:
    // getAlias with setVarAlias and setIfaceAlias implement disjoint-set data structure.
    // This algorithm is needed for the case when multiple alias statements
    // reference partially the same variables.
    સ્થિર AstVarScope* getAlias(AstVarScope* અચલ vscp) {
        જો (vscp->user2p() && vscp != vscp->user2p()) {
            AstVarScope* અચલ aliasp = getAlias(VN_AS(vscp->user2p(), VarScope));
            vscp->user2p(aliasp);
            પાછા aliasp;
        } નહીંતર {
            પાછા vscp;
        }
    }

ખાનગી:
    // Alias an interface reference to the interface it is connected to
    શૂન્ય setIfaceAlias(AstVarScope* અચલ vscp, AstVarScope* અચલ aliasp) {
        UASSERT_OBJ(vscp->varp()->isIfaceRef(), vscp, "Interface alias of non-interface");
        UASSERT_OBJ(aliasp->varp()->isIfaceRef(), aliasp, "Interface alias of non-interface");
        getAlias(vscp)->user2p(getAlias(aliasp));
    }

    // Alias two variables. An output port in the lower scope survives, unless the other
    // must keep its own storage.
    શૂન્ય setVarAlias(AstVarScope* ap, AstVarScope* bp) {
        UASSERT_OBJ(!ap->varp()->isIfaceRef(), ap, "Variable alias of interface");
        UASSERT_OBJ(!bp->varp()->isIfaceRef(), bp, "Variable alias of interface");
        ap = getAlias(ap);
        bp = getAlias(bp);
        જો (ap == bp) પાછા;
        અચલ તાર્કિક aWins = [&]() {
            અચલ AstVar* અચલ aVarp = ap->varp();
            અચલ AstVar* અચલ bVarp = bp->varp();
            // A primary IO is accessed directly by the user of the model, so must survive
            UASSERT_OBJ(!aVarp->isPrimaryIO() || !bVarp->isPrimaryIO(), ap,
                        "Alias of two primary IOs");
            જો (bVarp->isPrimaryIO()) પાછા ખોટું;
            જો (aVarp->isPrimaryIO()) પાછા સાચું;
            // A forced or public variable must keep its own storage. If both are, either works.
            જો (aVarp->isForced() != bVarp->isForced()) પાછા aVarp->isForced();
            જો (aVarp->isSigPublic() != bVarp->isSigPublic()) પાછા aVarp->isSigPublic();
            // Otherwise prefer an output port in the lower scope. This enables better V3Combine
            // due to having fewer upward hierarchical references (which prevent combining).
            જો (aVarp->direction() != VDirection::OUTPUT) પાછા ખોટું;
            // Is 'ap' strictly below 'bp' in the hierarchy
            માટે (અચલ AstScope* scopep = ap->scopep()->aboveScopep(); scopep;
                 scopep = scopep->aboveScopep()) {
                જો (scopep == bp->scopep()) પાછા સાચું;
            }
            પાછા ખોટું;
        }();
        જો (aWins) {
            bp->user2p(ap);
        } નહીંતર {
            ap->user2p(bp);
        }
    }

    // VISITORS
    શૂન્ય visit(AstNetlist* nodep) override {  // ScopeVisitor::
        // Recurse..., backward as must do packages before using packages
        iterateChildrenBackwardsConst(nodep);
    }
    શૂન્ય visit(AstScope* nodep) override {  // ScopeVisitor::
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
        જો (AstIface* અચલ ifacep = VN_CAST(nodep->modp(), Iface)) {
            m_statep->ઉમેરો_Iface_Mod_Sym(ifacep, m_modSymp);
        }
        m_scopep = nodep;
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstVarScope* nodep) override {  // ScopeVisitor::
        જો (!nodep->varp()->isFuncLocal() && !nodep->varp()->isClassMember()) {
            VSymEnt* અચલ varSymp
                = m_statep->ઉમેરો_Sym(m_modSymp, nodep->varp()->name(), nodep, nullptr);
            જો (nodep->varp()->isIfaceRef()) {
                AstIfaceRefDType* અચલ dtypep
                    = LinkDotState::ifaceRefFromArray(nodep->varp()->dtypep());
                UASSERT_OBJ(dtypep, nodep, "Non AstIfaceRefDType on isIfaceRef() var");
                જો (nodep->varp()->isIfaceParent()) {
                    UINFO(9, "Iface parent ref var " << nodep->varp()->name() << " " << nodep);
                    // Find the interface cell the var references
                    UINFO(9, "Iface parent dtype " << dtypep);
                    અચલ string ifcellname = dtypep->cellName();
                    string baddot;
                    VSymEnt* okSymp;
                    VSymEnt* cellSymp = m_statep->findDotted(nodep->fileline(), m_modSymp,
                                                             ifcellname, baddot, okSymp, ખોટું);
                    UASSERT_OBJ(
                        cellSymp, nodep,
                        "No symbol for interface instance: " << nodep->prettyNameQ(ifcellname));
                    UINFO(5, "       Found interface instance: se" << cvtToHex(cellSymp) << " "
                                                                   << cellSymp->nodep());
                    જો (dtypep->modportName() != "") {
                        // Look up the modport within the interface cell's symbol table
                        VSymEnt* અચલ mpSymp = cellSymp->findIdFallback(dtypep->modportName());
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
                    m_statep->ઉમેરો_Scope_Alias(LinkDotState::SAMN_IFTOP, varSymp, cellSymp);
                }
            }
        }
    }
    શૂન્ય visit(Ast_ગાંઠ_FTask* nodep) override {  // ScopeVisitor::
        VSymEnt* અચલ symp = m_statep->ઉમેરો_Block(m_modSymp, nodep->name(), nodep, nullptr);
        symp->fallbackp(m_modSymp);
        iterateChildren(nodep);
    }
    શૂન્ય visit(Ast_ગાંઠ_Foreach* nodep) override {  // ScopeVisitor::
        VSymEnt* અચલ symp = m_statep->ઉમેરો_Block(m_modSymp, nodep->name(), nodep, nullptr);
        symp->fallbackp(m_modSymp);
        // No recursion, we don't want to pick up variables
    }
    શૂન્ય visit(AstConstraintForeach* nodep) override {  // ScopeVisitor::
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstWith* nodep) override {  // ScopeVisitor::
        VSymEnt* અચલ symp = m_statep->ઉમેરો_Block(m_modSymp, nodep->name(), nodep, nullptr);
        symp->fallbackp(m_modSymp);
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstAlias* nodep) override {  // ScopeVisitor::
        // Track aliases
        UINFOTREE(9, nodep, "", "alias");
        AstVarScope* aliasVscp = nullptr;
        માટે (Ast_ગાંઠ* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
            AstVarScope* vscp = nullptr;
            જો (અચલ AstVarRef* અચલ refp = VN_CAST(itemp, VarRef)) {
                vscp = refp->varScopep();
            } નહીંતર {
                // Reaches into the scope of an instance, look it up by name
                અચલ AstVarXRef* અચલ xrefp = VN_AS(itemp, VarXRef);
                અચલ string scopename = xrefp->dotted() + "." + xrefp->name();
                string baddot;
                VSymEnt* okSymp;
                VSymEnt* અચલ symp = m_statep->findDotted(xrefp->fileline(), m_modSymp, scopename,
                                                           baddot, okSymp, ખોટું);
                UASSERT_OBJ(symp, nodep, "No symbol for alias item: " << scopename);
                vscp = VN_CAST(symp->nodep(), VarScope);
            }
            UASSERT_OBJ(vscp, nodep, "VarScope unset");
            જો (aliasVscp) {
                setVarAlias(aliasVscp, vscp);
            } નહીંતર {
                aliasVscp = vscp;
            }
        }
        iterateChildren(nodep);
        VL_DO_DANGLING(ધકેલો_Deletep(nodep->unlinkFrBack()), nodep);
    }
    શૂન્ય visit(AstAliasScope* nodep) override {  // ScopeVisitor::
        // Defer AliasScope processing - must process outer scopes before inner ones
        // so that nested interface port alias resolution works correctly
        UINFO(5, "ALIASSCOPE (deferred) " << nodep);
        m_deferredAliasScopes.emplace_back(nodep, m_modSymp);
    }
    શૂન્ય processAliasScope(AstAliasScope* nodep, VSymEnt* modSymp) {
        UINFO(5, "ALIASSCOPE  " << nodep);
        UINFOTREE(9, nodep, "", "avs");
        VSymEnt* rhsSymp;
        {
            AstVarRef* અચલ refp = VN_CAST(nodep->rhsp(), VarRef);
            AstVarXRef* અચલ xrefp = VN_CAST(nodep->rhsp(), VarXRef);
            UASSERT_OBJ(refp || xrefp, nodep,
                        "Unsupported: Non Var(X)Ref attached to interface pin");
            string inl
                = ((xrefp && xrefp->inlinedDots().size()) ? (xrefp->inlinedDots() + "__DOT__")
                                                          : "");
            અચલ string dottedPath
                = (xrefp && !xrefp->dotted().empty()) ? (xrefp->dotted() + ".") : "";
            VSymEnt* symp = nullptr;
            string scopename;
            જ્યારે (!symp) {
                scopename = refp ? refp->name()
                                 : (inl.size() ? (inl + dottedPath + xrefp->name())
                                               : (dottedPath + xrefp->name()));
                string baddot;
                VSymEnt* okSymp;
                symp = m_statep->findDotted(nodep->rhsp()->fileline(), modSymp, scopename, baddot,
                                            okSymp, સાચું);
                જો (inl == "") તોડો;
                inl = LinkDotState::removeLastInlineScope(inl);
            }
            UASSERT_OBJ(symp, nodep, "No symbol for interface alias rhs");
            UINFO(5, "       Found a linked scope RHS: " << scopename << "  se" << cvtToHex(symp)
                                                         << " " << symp->nodep());
            rhsSymp = symp;
        }
        VSymEnt* lhsSymp;
        {
            અચલ AstVarXRef* અચલ xrefp = VN_CAST(nodep->lhsp(), VarXRef);
            અચલ AstVarRef* અચલ refp = VN_CAST(nodep->lhsp(), VarRef);

            UASSERT_OBJ(refp || xrefp, nodep,
                        "Unsupported: Non Var(X)Ref attached to interface pin");
            અચલ string scopename
                = refp ? refp->varp()->name() : xrefp->dotted() + "." + xrefp->name();
            string baddot;
            VSymEnt* okSymp;
            VSymEnt* અચલ symp = m_statep->findDotted(nodep->lhsp()->fileline(), modSymp,
                                                       scopename, baddot, okSymp, ખોટું);
            UASSERT_OBJ(symp, nodep, "No symbol for interface alias lhs");
            UINFO(5, "       Found a linked scope LHS: " << scopename << "  se" << cvtToHex(symp)
                                                         << " " << symp->nodep());
            lhsSymp = symp;
        }
        // Remember the alias - can't do it yet because we may have additional symbols to be added,
        // or maybe an alias of an alias
        m_statep->ઉમેરો_Scope_Alias(LinkDotState::SAMN_IFTOP, lhsSymp, rhsSymp);
        AstVarScope* અચલ lhsVscp = VN_CAST(lhsSymp->nodep(), VarScope);
        AstVarScope* અચલ rhsVscp = VN_CAST(rhsSymp->nodep(), VarScope);
        UASSERT_OBJ(lhsVscp && rhsVscp, nodep, "Interface alias missing variable scope");
        setIfaceAlias(lhsVscp, rhsVscp);
        // We have stored the link, we don't need these any more
        VL_DO_DANGLING(nodep->unlinkFrBack()->delete_વૃક્ષ(), nodep);
    }
    શૂન્ય processDeferredAliasScopes() {
        // Sort by hierarchy depth (shallower first) so outer aliases are resolved before inner
        // Pre-compute depth map to avoid O(N log N * D) complexity in sort comparisons
        std::unordered_map<VSymEnt*, પૂર્ણાંક> depthMap;
        માટે (અચલ auto& pair : m_deferredAliasScopes) {
            VSymEnt* અચલ symp = pair.second;
            જો (depthMap.find(symp) == depthMap.end()) {
                પૂર્ણાંક depth = 0;
                માટે (VSymEnt* p = symp; p; p = p->parentp()) ++depth;
                depthMap[symp] = depth;
            }
        }
        std::stable_sort(m_deferredAliasScopes.begin(), m_deferredAliasScopes.end(),
                         [&depthMap](અચલ std::pair<AstAliasScope*, VSymEnt*>& a,
                                     અચલ std::pair<AstAliasScope*, VSymEnt*>& b) {
                             પાછા depthMap.at(a.second) < depthMap.at(b.second);
                         });
        // Process in sorted order
        માટે (auto& pair : m_deferredAliasScopes) { processAliasScope(pair.first, pair.second); }
        m_deferredAliasScopes.clear();
    }
    શૂન્ય visit(Ast_ગાંઠ_Gen* nodep) override {  // ScopeVisitor::  // LCOV_EXCL_LINE
        nodep->v3fatalSrc("Generate constructs should have been reduced out");
    }
    // For speed, don't recurse things that can't have scope
    // Note we allow AstNodeStmt's as generates may be under them
    શૂન્ય visit(AstCell*) override {}  // ScopeVisitor::
    શૂન્ય visit(AstVar*) override {}  // ScopeVisitor::
    શૂન્ય visit(Ast_ગાંઠ* nodep) override { iterateChildren(nodep); }  // ScopeVisitor::

જાહેર:
    // CONSTRUCTORS
    LinkDotScopeVisitor(AstNetlist* rootp, LinkDotState* statep)
        : m_statep{statep} {
        UINFO(4, __FUNCTION__ << ": ");
        iterate(rootp);
        // Process deferred AliasScopes in outer-to-inner order
        processDeferredAliasScopes();
    }
    ~LinkDotScopeVisitor() override = મૂળભૂત;
};

//======================================================================

// Iterate an interface to resolve modports
વર્ગ LinkDotIfaceVisitor final : જાહેર VNVisitor {
    // STATE
    LinkDotState* અચલ m_statep;  // State to pass between visitors, including symbol table
    VSymEnt* m_curSymp;  // Symbol Entry for current table, where to lookup/insert

    // VISITORS
    શૂન્ય visit(AstModport* nodep) override {  // IfaceVisitor::
        // Modport: Remember its name for later resolution
        UINFO(5, "   fiv: " << nodep);
        VL_RESTORER(m_curSymp);
        {
            // Create symbol table for the vars
            m_curSymp = m_statep->ઉમેરો_Block(m_curSymp, nodep->name(), nodep, nullptr);
            m_curSymp->fallbackp(VL_RESTORER_PREV(m_curSymp));
            iterateChildren(nodep);
        }
    }
    શૂન્ય visit(AstModportFTaskRef* nodep) override {  // IfaceVisitor::
        UINFO(5, "   fif: " << nodep);
        iterateChildren(nodep);
        VSymEnt* અચલ symp = m_curSymp->findIdFallback(nodep->name());
        જો (!symp) {
            nodep->v3error("Modport item not found: " << nodep->prettyNameQ());
        } નહીંતર જો (Ast_ગાંઠ_FTask* અચલ ftaskp = VN_CAST(symp->nodep(), ગાંઠ_FTask)) {
            // Make symbol under modport that points at the _interface_'s var, not the modport.
            nodep->ftaskp(ftaskp);
            VSymEnt* અચલ subSymp
                = m_statep->ઉમેરો_Sym(m_curSymp, nodep->name(), ftaskp, nullptr /*package*/);
            m_statep->ઉમેરો_Scope_Alias(LinkDotState::SAMN_MODPORT, subSymp, symp);
        } નહીંતર {
            nodep->v3error("Modport item is not a function/task: " << nodep->prettyNameQ());
        }
        જો (m_statep->forScopeCreation()) {
            // Done with AstModportFTaskRef.
            // Delete to prevent problems if we dead-delete pointed to ftask
            nodep->unlinkFrBack();
            VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
        }
    }

    // Helper to resolve remaining path through a nested interface
    // When findDotted() partially matches (okSymp set, baddot non-empty),
    // this follows the interface type to resolve the remaining path.
    // Returns the resolved symbol, or nullptr if not found.
    // On success, clears baddot; on partial match of multi-level path, updates baddot.
    VSymEnt* resolveNestedInterfacePath(File_લીટી* fl, VSymEnt* okSymp, string& baddot) {
        જો (!okSymp || baddot.empty()) પાછા nullptr;

        સ્થિર constexpr પૂર્ણાંક MAX_NESTING_DEPTH = 64;
        VSymEnt* curOkSymp = okSymp;

        માટે (પૂર્ણાંક depth = 0; depth < MAX_NESTING_DEPTH; ++depth) {
            // Try to get interface from the partially-matched symbol
            AstIface* ifacep = nullptr;
            જો (અચલ AstCell* અચલ cellp = VN_CAST(curOkSymp->nodep(), Cell)) {
                ifacep = VN_CAST(cellp->modp(), Iface);
            } નહીંતર જો (અચલ AstVar* અચલ varp = VN_CAST(curOkSymp->nodep(), Var)) {
                જો (varp->isIfaceRef()) {
                    જો (અચલ AstIfaceRefDType* અચલ ifaceRefp
                        = LinkDotState::ifaceRefFromArray(varp->dtypep())) {
                        ifacep = ifaceRefp->ifaceViaCellp();
                    }
                }
            }

            જો (!ifacep || !m_statep->exists_ગાંઠ_Sym(ifacep)) પાછા nullptr;

            VSymEnt* અચલ ifaceSymp = m_statep->get_ગાંઠ_Sym(ifacep);

            જો (baddot.find('.') == string::npos) {
                // Simple identifier - direct lookup
                VSymEnt* અચલ symp = ifaceSymp->findIdFallback(baddot);
                જો (symp) baddot.clear();
                પાછા symp;
            }

            // Multi-level path - use findDotted for partial resolution
            string remainingBaddot;
            VSymEnt* remainingOkSymp = nullptr;
            VSymEnt* અચલ symp = m_statep->findDotted(fl, ifaceSymp, baddot, remainingBaddot,
                                                       remainingOkSymp, સાચું);
            જો (symp) {
                baddot = remainingBaddot;
                પાછા symp;
            }

            // findDotted partially matched - check progress
            જો (remainingBaddot == baddot || !remainingOkSymp) પાછા nullptr;

            // Continue resolving with updated state
            baddot = remainingBaddot;
            curOkSymp = remainingOkSymp;
        }

        UINFO(1, "Nested interface resolution depth limit exceeded at " << fl);
        પાછા nullptr;
    }

    // Resolve a modport expression to find the referenced symbol
    VSymEnt* resolveModportExpression(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* exprp) {
        UINFO(5, "     resolveModportExpression: " << exprp);
        VSymEnt* symp = nullptr;
        જો (AstParseRef* અચલ refp = VN_CAST(exprp, ParseRef)) {
            // Simple variable reference: modport mp(input .a(sig_a))
            symp = m_curSymp->findIdFallback(refp->name());
        } નહીંતર જો (AstVarRef* અચલ refp = VN_CAST(exprp, VarRef)) {
            // Already resolved VarRef (can happen if iterateChildren resolved it)
            જો (refp->varScopep()) {
                // During scope creation, VarRef may have VarScope already linked
                symp = m_statep->get_ગાંઠ_Sym(refp->varScopep());
            } નહીંતર જો (refp->varp()) {
                symp = m_curSymp->findIdFallback(refp->varp()->name());
            }
        } નહીંતર જો (AstVarXRef* અચલ refp = VN_CAST(exprp, VarXRef)) {
            // Resolved VarXRef (dotted reference resolved by earlier pass)
            જો (refp->varp()) {
                // For nested interfaces, the var is in a different scope
                // First try to find the symbol via the var itself
                જો (m_statep->exists_ગાંઠ_Sym(refp->varp())) {
                    symp = m_statep->get_ગાંઠ_Sym(refp->varp());
                } નહીંતર {
                    // Fallback: look up in current scope
                    symp = m_curSymp->findIdFallback(refp->varp()->name());
                }
            } નહીંતર જો (!refp->dotted().empty()) {
                // varp not set yet - use dotted path to find the symbol
                // The dotted part (e.g., "base") is the interface cell, and name is the member
                અચલ string fullPath = refp->dotted() + "." + refp->name();
                string baddot;
                VSymEnt* okSymp = nullptr;
                symp = m_statep->findDotted(fl, m_curSymp, fullPath, baddot, okSymp, સાચું);
                // Handle nested interface path when findDotted had partial match
                જો (okSymp && !baddot.empty()) {
                    VSymEnt* અચલ resolved = resolveNestedInterfacePath(fl, okSymp, baddot);
                    જો (resolved) symp = resolved;
                }
            }
        } નહીંતર જો (AstDot* અચલ dotp = VN_CAST(exprp, Dot)) {
            // Dotted path: modport mp(input .a(inner.sig))
            તાર્કિક hasPartSelect = ખોટું;
            અચલ string dottedPath = extractDottedPath(dotp, hasPartSelect);
            જો (hasPartSelect) {
                fl->v3warn(
                    E_UNSUPPORTED,
                    "Unsupported: Modport expression with part select (IEEE 1800-2023 25.5.4)");
            } નહીંતર {
                string baddot;
                VSymEnt* okSymp = nullptr;
                symp = m_statep->findDotted(fl, m_curSymp, dottedPath, baddot, okSymp, સાચું);

                // Handle nested interface path when findDotted had partial match
                જો (okSymp && !baddot.empty()) {
                    VSymEnt* અચલ resolved = resolveNestedInterfacePath(fl, okSymp, baddot);
                    જો (resolved) symp = resolved;
                }

                જો (!symp || !baddot.empty()) {
                    fl->v3error("Can't find modport expression target: "
                                << Ast_ગાંઠ::prettyNameQ(dottedPath));
                    symp = nullptr;
                }
            }
        } નહીંતર જો (VN_IS(exprp, SelBit) || VN_IS(exprp, SelExtract)) {
            // Part select expressions not yet supported
            fl->v3warn(E_UNSUPPORTED,
                       "Unsupported: Modport expression with part select (IEEE 1800-2023 25.5.4)");
        } નહીંતર {
            // Other expression types not supported
            fl->v3warn(E_UNSUPPORTED,
                       "Unsupported: Complex modport expression (IEEE 1800-2023 25.5.4)");
        }
        પાછા symp;
    }

    શૂન્ય visit(AstModportVarRef* nodep) override {  // IfaceVisitor::
        UINFO(5, "   fiv: " << nodep);
        iterateChildren(nodep);
        VSymEnt* symp = nullptr;
        જો (nodep->exprp()) {
            // Modport expression syntax: modport mp(input .port_name(expression))
            symp = resolveModportExpression(nodep->fileline(), nodep->exprp());
        } નહીંતર {
            symp = m_curSymp->findIdFallback(nodep->name());
        }
        જો (!symp) {
            nodep->v3error("Modport item not found: " << nodep->prettyNameQ());
        } નહીંતર જો (AstVar* અચલ varp = VN_CAST(symp->nodep(), Var)) {
            // Make symbol under modport that points at the _interface_'s var via the modport.
            // (Need modport still to test input/output markings)
            nodep->varp(varp);
            જો (nodep->exprp()) {
                // For modport expressions, insert symbol pointing to the underlying var (not the
                // ModportVarRef which will be deleted during scope creation). The virtual port
                // name maps to the real signal's var.
                VSymEnt* અચલ subSymp
                    = m_statep->ઉમેરો_Sym(m_curSymp, nodep->name(), varp, nullptr /*package*/);
                m_statep->ઉમેરો_Scope_Alias(LinkDotState::SAMN_MODPORT, subSymp, symp);
            } નહીંતર {
                // For regular modport items, insert symbol pointing to ModportVarRef for
                // input/output marking tests.
                m_statep->ઉમેરો_Sym(m_curSymp, nodep->name(), nodep, nullptr /*package*/);
            }
        } નહીંતર જો (AstVarScope* અચલ vscp = VN_CAST(symp->nodep(), VarScope)) {
            // Make symbol under modport that points at the _interface_'s var, not the modport.
            nodep->varp(vscp->varp());
            // Only insert symbol for modport expression virtual ports (exprp set).
            // For regular modport items, don't insert into the shared modport table because
            // each instance would overwrite the previous one, causing wrong VarScope lookups.
            // Regular items are found via the modport's fallback to the interface.
            જો (nodep->exprp()) {
                VSymEnt* અચલ subSymp
                    = m_statep->ઉમેરો_Sym(m_curSymp, nodep->name(), vscp, nullptr /*package*/);
                m_statep->ઉમેરો_Scope_Alias(LinkDotState::SAMN_MODPORT, subSymp, symp);
            }
        } નહીંતર {
            nodep->v3error("Modport item is not a variable: " << nodep->prettyNameQ());
        }
        જો (m_statep->forScopeCreation()) {
            // Done with AstModportVarRef.
            // Delete to prevent problems if we dead-delete pointed to variable
            nodep->unlinkFrBack();
            VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
        }
    }
    શૂન્ય visit(AstModportClockingRef* nodep) override {  // IfaceVisitor::
        UINFO(5, "   fic: " << nodep);
        iterateChildren(nodep);
        VSymEnt* અચલ symp = m_curSymp->findIdFallback(nodep->name());
        જો (!symp) {
            nodep->v3error("Modport item not found: " << nodep->prettyNameQ());
        } નહીંતર જો (AstClocking* અચલ clockingp = VN_CAST(symp->nodep(), Clocking)) {
            nodep->clockingp(clockingp);
            m_statep->ઉમેરો_Sym(m_curSymp, nodep->name(), nodep, nullptr /*package*/);
        } નહીંતર {
            nodep->v3error("Modport item is not a clocking block: " << nodep->prettyNameQ());
        }
    }
    શૂન્ય visit(AstScope* nodep) override {  // IfaceVisitor::
        જો (nodep->user3SetOnce()) પાછા;
        iterateChildren(nodep);
    }
    શૂન્ય visit(Ast_ગાંઠ* nodep) override { iterateChildren(nodep); }  // IfaceVisitor::

જાહેર:
    // CONSTRUCTORS
    LinkDotIfaceVisitor(AstIface* nodep, VSymEnt* curSymp, LinkDotState* statep)
        : m_statep{statep}
        , m_curSymp{curSymp} {
        UINFO(4, __FUNCTION__ << ": ");
        iterate(nodep);
    }
    ~LinkDotIfaceVisitor() override = મૂળભૂત;
};

શૂન્ય LinkDotState::computeIfaceModSyms() {
    //  AstScope::user3()  // bool. Already iterated by a LinkDotIfaceVisitor
    અચલ VNUser3InUse user3InUse;
    માટે (અચલ auto& itr : m_ifaceModSyms) {
        AstIface* અચલ nodep = itr.first;
        VSymEnt* અચલ symp = itr.second;
        LinkDotIfaceVisitor{nodep, symp, આ};
    }
    m_ifaceModSyms.clear();
}

//======================================================================

વર્ગ LinkDotResolveVisitor final : જાહેર VNVisitor {
    // NODE STATE
    // Cleared on global
    //  *::user1p()             -> See LinkDotState
    //  *::user2p()             -> See LinkDotState
    //  *::user3()              // bool. Processed
    //  *::user4()              -> See LinkDotState
    અચલ VNUser3InUse m_inuser3;

    // TYPES
    ગણના DotPosition : uint8_t {
        // Must match ascii() method below
        DP_NONE = 0,  // Not under a DOT
        DP_PACKAGE,  // {package-or-class}:: DOT
        DP_FIRST,  // {scope-or-var} DOT
        DP_SCOPE,  // DOT... {scope-or-var} DOT
        DP_FINAL,  // [DOT...] {var-or-func-or-dtype} with no following dots
        DP_MEMBER  // DOT {member-name} [DOT...]
    };

    // STATE
    LinkDotState* અચલ m_statep;  // State, including dotted symbol table
    VSymEnt* m_curSymp = nullptr;  // SymEnt for current lookup point
    VSymEnt* m_modSymp = nullptr;  // SymEnt for current module
    VSymEnt* m_pinSymp = nullptr;  // SymEnt for pin lookups
    VSymEnt* m_randSymp = nullptr;  // SymEnt for randomize target class's lookups
    અચલ AstCell* m_cellp = nullptr;  // Current cell
    Ast_ગાંઠ_Module* m_modp = nullptr;  // Current module
    Ast_ગાંઠ_FTask* m_ftaskp = nullptr;  // Current function/task
    AstMethodCall* m_randMethodCallp = nullptr;  // Current randomize() method call
    પૂર્ણાંક m_modport_અંક = 0;  // Uniqueify modport numbers
    પૂર્ણાંક m_indent = 0;  // Indentation (tree depth) for debug
    તાર્કિક m_inSens = ખોટું;  // True if in senitem
    અચલ AstWith* m_currentWithp = nullptr;  // Enclosing 'with' (nullptr if none)
    std::set<std::string>
        m_restrictedNamesUsed;  // Names from current 'with (id_list)' that resolved into target
    તાર્કિક m_genericIfaceModule = ખોટું;  // True if in module containing generic interface
    std::map<std::string, Ast_ગાંઠ*> m_ifClassImpNames;  // Names imported from interface class
    // Inherited interface implementations needing a forward in the current class, by name
    std::map<std::string, અચલ VSymEnt*> m_ifForwards;
    std::set<AstClass*> m_extendsParam;  // Classes that have a parameterized super class
                                         // (except the default instances)
                                         // They are added to the set only in linkDotPrimary.
    તાર્કિક m_insideClassExtParam = ખોટું;  // Inside a class from m_extendsParam
    AstNew* m_explicitSuperNewp = nullptr;  // Hit a "super.new" call inside a "new" function
    std::map<Ast_ગાંઠ*, AstPin*> m_usedPins;  // Pin used in this cell, map to duplicate
    std::map<Ast_ગાંઠ*, std::map<std::string, AstPin*>> m_usedDefParamPins;
    // Defparam pins used for a given formal, by hierarchical path
    std::map<std::string, Ast_ગાંઠ_Module*> m_modulesToRevisit;  // Modules to revisit a second time
    Ast_ગાંઠ* m_lastDeferredp = nullptr;  // Last node which requested a revisit of its module
    Ast_ગાંઠ_DType* m_packedArrayDtp = nullptr;  // Datatype reference for packed array
    તાર્કિક m_inPackedArray = ખોટું;  // Currently traversing a packed array tree
    તાર્કિક m_replaceWithAlias
        = સાચું;  // Replace VarScope with an alias. Used in the handling of AstAlias
    તાર્કિક m_isParam = ખોટું;  // Specifies whether currently visiting param variable
    તાર્કિક m_resolvingTypedef = ખોટું;  // Currently traversing a Typedef tree

    સંરચના DotStates final {
        DotPosition m_dotPos;  // Scope part of dotted resolution
        VSymEnt* m_dotSymp;  // SymEnt for dotted AstParse lookup
        અચલ AstDot* m_dotp;  // Current dot
        તાર્કિક m_super;  // Starts with super reference
        તાર્કિક m_unresolvedCell;  // Unresolved cell, needs help from V3Param
        તાર્કિક m_unresolvedClass;  // Unresolved class reference, needs help from V3Param
        તાર્કિક m_unresolvedGenericIface;  // Unresolved generic interface, needs help from V3Param
        તાર્કિક m_genBlk;  // Contains gen block reference
        Ast_ગાંઠ* m_unlinkedScopep;  // Unresolved scope, needs corresponding VarXRef
        AstDisable* m_disablep;  // Disable statement under which the reference is
        તાર્કિક m_dotErr;  // Error found in dotted resolution, ignore upwards
        string m_dotText;  // String of dotted names found in below parseref
        DotStates() { init(nullptr); }
        ~DotStates() = મૂળભૂત;
        શૂન્ય init(VSymEnt* curSymp, તાર્કિક unresolvedClass = ખોટું) {
            m_dotPos = DP_NONE;
            m_dotSymp = curSymp;
            m_dotp = nullptr;
            m_super = ખોટું;
            m_dotErr = ખોટું;
            m_dotText = "";
            m_unresolvedCell = ખોટું;
            m_unresolvedClass = unresolvedClass;
            m_unresolvedGenericIface = ખોટું;
            m_genBlk = ખોટું;
            m_unlinkedScopep = nullptr;
            m_disablep = nullptr;
        }
        string ascii() અચલ {
            સ્થિર અચલ અક્ષર* અચલ names[]
                = {"DP_NONE", "DP_PACKAGE", "DP_FIRST", "DP_SCOPE", "DP_FINAL", "DP_MEMBER"};
            std::ostringstream sstr;
            sstr << "ds=" << names[m_dotPos];
            sstr << "  dse" << cvtToHex(m_dotSymp);
            અચલ string dsname = m_dotSymp->nodep()->name().substr(0, 8);
            sstr << "(" << m_dotSymp->nodep()->typeName() << (dsname.empty() ? "" : ":") << dsname
                 << ")";
            જો (m_dotErr) sstr << "  [dotErr]";
            જો (m_super) sstr << "  [super]";
            જો (m_unresolvedCell) sstr << "  [unrCell]";
            જો (m_unresolvedClass) sstr << "  [unrClass]";
            જો (m_unresolvedGenericIface) sstr << "  [unrGIface]";
            જો (m_genBlk) sstr << "  [genBlk]";
            sstr << "  txt=" << m_dotText;
            પાછા sstr.str();
        }
    } m_ds;  // State to preserve across recursions

    // METHODS - Variables
    AstVar* createImplicitVar(VSymEnt* /*lookupSymp*/, AstParseRef* nodep, Ast_ગાંઠ_Module* modp,
                              VSymEnt* moduleSymp, તાર્કિક noWarn) {
        // Create implicit after warning
        જો (!noWarn) {
            જો (nodep->fileline()->warnIsOff(V3ErrorCode::I_DEF_NETTYPE_WIRE)) {
                અચલ string suggest = m_statep->suggestSymFallback(moduleSymp, nodep->name(),
                                                                    Link_ગાંઠ_Matcher_Var{});
                nodep->v3error("Signal definition not found, and implicit disabled with "
                               "`default_nettype: "
                               << nodep->prettyNameQ() << '\n'
                               << (suggest.empty() ? "" : nodep->warnMore() + suggest));

            }
            // Bypass looking for suggestions if IMPLICIT is turned off
            // as there could be thousands of these suppressed in large netlists
            નહીંતર જો (!nodep->fileline()->warnIsOff(V3ErrorCode::IMPLICIT)) {
                અચલ string suggest = m_statep->suggestSymFallback(moduleSymp, nodep->name(),
                                                                    Link_ગાંઠ_Matcher_Var{});
                nodep->v3warn(IMPLICIT,
                              "Signal definition not found, creating implicitly: "
                                  << nodep->prettyNameQ() << '\n'
                                  << (suggest.empty() ? "" : nodep->warnMore() + suggest));
            }
        }
        AstVar* અચલ newp
            = નવું AstVar{nodep->fileline(), VVarType::WIRE, nodep->name(), VFlagLogicPacked{}, 1};
        newp->lifetime(VLifetime::STATIC_IMPLICIT);
        newp->trace(modp->modTrace());
        modp->addStmtsp(newp);
        // Link it to signal list, must add the variable under the module;
        // current scope might be lower now
        m_statep->ઉમેરો_Sym(moduleSymp, newp->name(), newp, nullptr /*classOrPackagep*/);
        પાછા newp;
    }
    AstVar* foundToVarp(અચલ VSymEnt* symp, Ast_ગાંઠ* nodep, VAccess access) {
        // Return a variable if possible, auto converting a modport to variable
        જો (!symp) {
            પાછા nullptr;
        } નહીંતર જો (VN_IS(symp->nodep(), Var)) {
            પાછા VN_AS(symp->nodep(), Var);
        } નહીંતર જો (VN_IS(symp->nodep(), ModportVarRef)) {
            AstModportVarRef* અચલ snodep = VN_AS(symp->nodep(), ModportVarRef);
            AstVar* અચલ varp = snodep->varp();
            જો (access.isWriteOrRW() && snodep->direction().isReadOnly()) {
                nodep->v3error("Attempt to drive input-only modport: " << nodep->prettyNameQ());
            }  // else other simulators don't warn about reading, and IEEE doesn't say illegal
            પાછા varp;
        } નહીંતર {
            પાછા nullptr;
        }
    }

    // Look up a virtual port through modport expression mapping.
    // When a dotted reference uses a modport with expression syntax, the virtual port name
    // maps to the real signal. This helper resolves that mapping during scope creation.
    // Returns the VarScope for the real signal, or nullptr if not found.
    AstVarScope* findVirtualPortVarScope(VSymEnt* dotSymp, અચલ string& portName) {
        અચલ AstVarScope* અચલ dotVscp = VN_CAST(dotSymp->nodep(), VarScope);
        અચલ AstVar* અચલ dotVarp = dotVscp ? dotVscp->varp() : nullptr;
        જો (!dotVarp) પાછા nullptr;

        Ast_ગાંઠ_DType* dtypep = dotVarp->childDTypep();
        જો (!dtypep) dtypep = dotVarp->subDTypep();
        જો (!dtypep) પાછા nullptr;
        અચલ AstIfaceRefDType* અચલ ifaceRefp = VN_CAST(dtypep, IfaceRefDType);
        જો (!ifaceRefp || !ifaceRefp->modportp() || !ifaceRefp->ifaceViaCellp()) પાછા nullptr;

        // Get the interface cell's symbol table
        VSymEnt* અચલ ifaceCellSymp = m_statep->get_ગાંઠ_Sym(ifaceRefp->ifaceViaCellp());

        // Look up modport by name in interface cell
        VSymEnt* અચલ modportSymp = ifaceCellSymp->findIdFallback(ifaceRefp->modportp()->name());
        જો (!modportSymp) પાછા nullptr;

        // Look up virtual port name in modport symbol table
        string baddot;
        VSymEnt* અચલ virtPortp = m_statep->findSymPrefixed(modportSymp, portName, baddot, સાચું);
        જો (!virtPortp) પાછા nullptr;

        // Symbol may point to VarScope or Var
        જો (AstVarScope* vscp = VN_CAST(virtPortp->nodep(), VarScope)) પાછા vscp;

        // Symbol points to Var - look up VarScope by name in interface cell
        જો (અચલ AstVar* અચલ realVarp = VN_CAST(virtPortp->nodep(), Var)) {
            VSymEnt* અચલ realFoundp
                = m_statep->findSymPrefixed(ifaceCellSymp, realVarp->name(), baddot, સાચું);
            પાછા realFoundp ? VN_CAST(realFoundp->nodep(), VarScope) : nullptr;
        }
        પાછા nullptr;
    }

    // Capture a ParamTypeDType reference for interface typedef retargeting.
    // Called when a RefDType resolves to a ParamTypeDType owned by an interface.
    શૂન્ય captureIfaceParamType(AstRefDType* nodep, AstParamTypeDType* defp) {
        જો (!V3LinkDotIfaceCapture::enabled() || !m_statep->forPrimary()) પાછા;
        Ast_ગાંઠ_Module* અચલ defOwnerModp = V3LinkDotIfaceCapture::findOwnerModule(defp);
        જો (!defOwnerModp || !VN_IS(defOwnerModp, Iface)) પાછા;
        AstCell* અચલ cellForCapture
            = m_ds.m_dotSymp ? VN_CAST(m_ds.m_dotSymp->nodep(), Cell) : nullptr;
        જો (!cellForCapture) પાછા;
        UINFO(9, indent() << "iface capture add paramtype " << nodep
                          << " iface=" << defOwnerModp->prettyNameQ());
        V3LinkDotIfaceCapture::addParamType(nodep, cellForCapture->name(), m_modp, defp,
                                            defOwnerModp->name());
    }

    Ast_ગાંઠ_Stmt* addImplicitSuperNewCall(AstFunc* અચલ nodep,
                                         અચલ AstClassExtends* અચલ classExtendsp) {
        // Returns the added node
        File_લીટી* અચલ fl = nodep->fileline();
        AstArg* argsp = nullptr;
        જો (classExtendsp->argsp()) argsp = classExtendsp->argsp()->clone_વૃક્ષ(સાચું);
        AstNew* અચલ newExprp = નવું AstNew{fl, argsp};
        newExprp->isImplicit(સાચું);
        AstDot* અચલ superNewp = નવું AstDot{fl, ખોટું, નવું AstParseRef{fl, "super"}, newExprp};
        Ast_ગાંઠ_Stmt* અચલ superNewStmtp = superNewp->makeStmt();
        માટે (Ast_ગાંઠ* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            // super.new shall be the first statement (IEEE 1800-2023 8.15)
            // but some nodes (such as variable declarations and typedefs) should stay before
            જો (VN_IS(stmtp, ગાંઠ_Stmt)) {
                stmtp->addHereThisAsNext(superNewStmtp);
                પાછા superNewStmtp;
            }
        }
        // There were no statements
        nodep->addStmtsp(superNewStmtp);
        પાછા superNewStmtp;
    }
    શૂન્ય checkNoDot(Ast_ગાંઠ* nodep) {
        જો (VL_UNLIKELY(!nodep)) {
            UINFO(9, indent() << "iface capture null node passed to checkNoDot; dot state="
                              << m_ds.ascii());
            m_ds.m_dotErr = સાચું;
            પાછા;
        }
        જો (VL_UNLIKELY(m_ds.m_dotPos != DP_NONE)) {
            UINFO(9, indent() << "ds=" << m_ds.ascii());
            UINFO(9, indent() << "iface capture checkNoDot hit node=" << nodep
                              << " dotState=" << m_ds.ascii());
            જો (VL_UNLIKELY(!m_ds.m_dotp)) {
                nodep->v3error("Syntax error: Not expecting " << nodep->type()
                                                              << " here (missing dot context)");
            } નહીંતર {
                nodep->v3error("Syntax error: Not expecting "
                               << nodep->type() << " under a " << nodep->backp()->type()
                               << " in dotted expression\n"
                               << nodep->warnContextPrimary() << m_ds.m_dotp->warnOther()
                               << "... Resolving this reference\n"
                               << m_ds.m_dotp->warnContextSecondary());
            }
            m_ds.m_dotErr = સાચું;
        }
    }
    AstVar* find_Iface_ટોચ_Varp(Ast_ગાંઠ* nodep, VSymEnt* parentEntp, અચલ string& name) {
        અચલ string findName = name + "__Viftop";
        VSymEnt* અચલ ifaceSymp = parentEntp->findIdFallback(findName);
        AstVar* અચલ iface_ટોચ_Varp = ifaceSymp ? VN_AS(ifaceSymp->nodep(), Var) : nullptr;
        UASSERT_OBJ(iface_ટોચ_Varp, nodep, "Can't find interface var ref: " << findName);
        પાછા iface_ટોચ_Varp;
    }
    AstRSProd* findProd(Ast_ગાંઠ* nodep, VSymEnt* parentEntp, અચલ string& name) {
        અચલ VSymEnt* અચલ foundp = parentEntp->findIdFallback(name);
        AstRSProd* foundNodep = foundp ? VN_CAST(foundp->nodep(), RSProd) : nullptr;
        જો (!foundNodep) {
            VSpellCheck speller;
            Link_ગાંઠ_Matcher_Prod matcher;
            parentEntp->candidateIdFlat(&speller, &matcher, ખોટું);
            અચલ string suggest = speller.best_Candidate_સંદેશ(name);
            UINFO(1, "   ErrParseRef curSymp=se" << cvtToHex(parentEntp));
            nodep->v3error("Production " << Ast_ગાંઠ::prettyNameQ(name) << " not found\n"
                                         << (suggest.empty() ? "" : nodep->warnMore() + suggest));
            પાછા nullptr;
        }
        પાછા foundNodep;
    }
    શૂન્ય duplicatePinError(AstPin* nodep, Ast_ગાંઠ* origp, અચલ અક્ષર* whatp) {
        nodep->v3error("Duplicate " << whatp << " connection: " << nodep->prettyNameQ() << '\n'
                                    << nodep->warnContextPrimary() << '\n'
                                    << origp->warnOther() << "... Location of original " << whatp
                                    << " connection\n"
                                    << origp->warnContextSecondary());
    }
    શૂન્ય markAndCheckPinDup(AstPin* nodep, Ast_ગાંઠ* refp, અચલ અક્ષર* whatp) {
        અચલ auto pair = m_usedPins.emplace(refp, nodep);
        જો (pair.second) {
            જો (!nodep->paramPath().empty())
                m_usedDefParamPins[refp].emplace(nodep->paramPath(), nodep);
            પાછા;
        } નહીંતર {
            Ast_ગાંઠ* અચલ origp = pair.first->second;
            જો (nodep->paramPath().empty() || VN_AS(origp, Pin)->paramPath().empty()) {
                duplicatePinError(nodep, origp, whatp);
                પાછા;
            }
        }

        auto& defParamPins = m_usedDefParamPins[refp];
        અચલ auto defParamIt = defParamPins.find(nodep->paramPath());
        જો (defParamIt != defParamPins.end()) {
            duplicatePinError(nodep, defParamIt->second, whatp);
            પાછા;
        }
        defParamPins.emplace(nodep->paramPath(), nodep);
    }
    સ્થિર તાર્કિક extendsClass(અચલ AstClass* classp, અચલ AstClass* targetp) {
        // Return true if classp inherits from targetp through already resolved extends
        // If performance of this becomes a problem, use just a counter to check
        // exceeds some number of iterations then perform the more expensive analysis
        std::set<અચલ AstClass*> visited;
        std::vector<અચલ AstClass*> todo{classp};
        જ્યારે (!todo.empty()) {
            અચલ AstClass* અચલ currp = todo.back();
            todo.કાઢો_back();
            જો (currp == targetp) પાછા સાચું;
            જો (!visited.ઉમેરો(currp).second) ચાલુ;
            માટે (અચલ AstClassExtends* cextp = currp->extendsp(); cextp;
                 cextp = VN_AS(cextp->nextp(), ClassExtends)) {
                જો (અચલ AstClass* અચલ basep = cextp->classOrNullp()) todo.ધકેલો_back(basep);
            }
        }
        પાછા ખોટું;
    }
    સ્થિર AstClocking* sensClockingp(Ast_ગાંઠ* nodep) {
        // Return the clocking block referenced by nodep, either directly or through a modport
        જો (AstClocking* અચલ clockingp = VN_CAST(nodep, Clocking)) પાછા clockingp;
        જો (અચલ AstModportClockingRef* અચલ clockingRefp = VN_CAST(nodep, ModportClockingRef))
            પાછા clockingRefp->clockingp();
        પાછા nullptr;
    }
    VSymEnt* getCreateClockingEventSymEnt(AstClocking* clockingp) {
        AstVar* અચલ eventp = clockingp->ensureEventp(સાચું);
        જો (!eventp->user1p()) eventp->user1p(નવું VSymEnt{m_statep->symsp(), eventp});
        પાછા reinterpret_cast<VSymEnt*>(eventp->user1p());
    }
    VSymEnt* findInlinedDotsSym(Ast_ગાંઠ* nodep, અચલ string& inlinedDots) {
        // Return scope that applies for searching, given renaming due to module inlining.
        // Dotted lookup is always relative to module, as maybe variable
        // name lower down with same scope name we want to ignore (t_math_divw)
        VSymEnt* dotSymp = m_modSymp;
        અચલ string inl = Ast_ગાંઠ::dedotName(inlinedDots);
        string baddot;
        VSymEnt* okSymp = nullptr;
        dotSymp = m_statep->findDotted(nodep->fileline(), dotSymp, inl, baddot, okSymp, ખોટું);
        UASSERT_OBJ(dotSymp, nodep,
                    "Couldn't resolve inlined scope " << Ast_ગાંઠ::prettyNameQ(baddot)
                                                      << " in: " << inlinedDots);
        UINFO(9, indent() << "findInlinedDotsSym " << dotSymp
                          << " search start due to inlinedDots=" << inlinedDots);
        પાછા dotSymp;
    }
    VSymEnt* findDottedRefSymp(Ast_ગાંઠ* nodep, VSymEnt* lookupSymp, અચલ string& dotted,
                               string& baddot, VSymEnt*& okSymp, તાર્કિક* modportp = nullptr) {
        // Return scope to search for a relinked hierarchical reference
        // (AstVarXRef or AstEnumItemRef), or nullptr after error if not found
        // If modportp is given, set true when the reference went through a modport
        જો (modportp) *modportp = ખોટું;
        VSymEnt* dotSymp = m_statep->findDotted(nodep->fileline(), lookupSymp, dotted, baddot,
                                                okSymp, સાચું);  // Maybe nullptr
        જો (!dotSymp) {
            nodep->v3error(
                "Can't find definition of "
                << (!baddot.empty() ? Ast_ગાંઠ::prettyNameQ(baddot) : nodep->prettyNameQ()) << '\n'
                << nodep->warnContextPrimary());
            પાછા nullptr;
        }
        જો (અચલ AstVar* અચલ varp = VN_CAST(dotSymp->nodep(), Var)) {
            જો (અચલ AstIfaceRefDType* અચલ ifaceRefp
                = VN_CAST(varp->childDTypep(), IfaceRefDType)) {
                જો (ifaceRefp->modportp()) {
                    dotSymp = m_statep->get_ગાંઠ_Sym(ifaceRefp->modportp());
                    જો (modportp) *modportp = સાચું;
                } નહીંતર {
                    dotSymp = m_statep->get_ગાંઠ_Sym(ifaceRefp->ifacep());
                }
            }
        } નહીંતર જો (અચલ AstModportClockingRef* અચલ clockingRefp
                   = VN_CAST(dotSymp->nodep(), ModportClockingRef)) {
            dotSymp = m_statep->get_ગાંઠ_Sym(clockingRefp->clockingp());
        }
        પાછા dotSymp;
    }

    સ્થિર તાર્કિક isParamedClassRefDType(અચલ Ast_ગાંઠ* classp) {
        જ્યારે (અચલ AstRefDType* અચલ refp = VN_CAST(classp, RefDType))
            classp = refp->subDTypep();
        પાછા (VN_IS(classp, ClassRefDType) && VN_AS(classp, ClassRefDType)->paramsp())
               || VN_IS(classp, ParamTypeDType);
    }
    તાર્કિક isParamedClassRef(અચલ Ast_ગાંઠ* nodep) {
        // Is this a parameterized reference to a class, or a reference to class parameter
        જો (અચલ auto* classRefp = VN_CAST(nodep, ClassOrPackageRef)) {
            જો (classRefp->paramsp()) પાછા સાચું;
            અચલ auto* classp = classRefp->classOrPackageNodep();
            જ્યારે (અચલ auto* typedefp = VN_CAST(classp, Typedef)) classp = typedefp->subDTypep();
            પાછા isParamedClassRefDType(classp);
        }
        પાછા isParamedClassRefDType(nodep);
    }
    VSymEnt* getThisClassSymp() {
        VSymEnt* classSymp = m_ds.m_dotSymp;
        જ્યારે (classSymp && !VN_IS(classSymp->nodep(), Class)) {
            classSymp = classSymp->parentp();
        }
        પાછા classSymp;
    }
    // Is the current scope within the given class, including within its nested classes
    તાર્કિક insideClass(અચલ AstClass* classp) અચલ {
        માટે (અચલ VSymEnt* symp = m_curSymp; symp; symp = symp->parentp()) {
            જો (symp->nodep() == classp) પાછા સાચું;
        }
        પાછા ખોટું;
    }
    // 'randomize() with' target class not yet specialized; defer to post-V3Param pass
    તાર્કિક randTargetUnresolved() અચલ {
        પાછા m_statep->forPrimary() && m_currentWithp && m_randMethodCallp && !m_randSymp;
    }
    // Is 'funcp' of 'classp' virtual, directly or by overriding a virtual method of a base
    // class (IEEE 1800-2023 8.20)
    તાર્કિક isVirtualMethod(અચલ Ast_ગાંઠ_FTask* funcp, અચલ AstClass* classp) અચલ {
        જો (funcp->isVirtual()) પાછા સાચું;
        માટે (અચલ AstClassExtends* cextp = classp->extendsp(); cextp;
             cextp = VN_AS(cextp->nextp(), ClassExtends)) {
            AstClass* અચલ basep = cextp->classOrNullp();
            જો (!basep) ચાલુ;
            અચલ VSymEnt* અચલ foundp = m_statep->get_ગાંઠ_Sym(basep)->findIdFlat(funcp->name());
            અચલ Ast_ગાંઠ_FTask* અચલ baseFuncp
                = foundp ? VN_CAST(foundp->nodep(), ગાંઠ_FTask) : nullptr;
            અચલ AstClass* અચલ baseClassp = foundp && foundp->parentp()
                                                   ? VN_CAST(foundp->parentp()->nodep(), Class)
                                                   : nullptr;
            જો (baseFuncp && baseClassp && isVirtualMethod(baseFuncp, baseClassp)) પાછા સાચું;
        }
        પાછા ખોટું;
    }
    // Is imported 'implp' an implementation of interface method 'ifSymp' (IEEE 1800-2023
    // 8.26.2)? If its class does not derive from the interface class, it needs a forwarding
    // override in the derived class to be reachable through the interface.
    તાર્કિક inheritedImplements(અચલ VSymEnt* implp, અચલ VSymEnt* ifSymp, તાર્કિક& needForwardr) {
        needForwardr = ખોટું;
        અચલ Ast_ગાંઠ_FTask* અચલ implFuncp = VN_CAST(implp->nodep(), ગાંઠ_FTask);
        જો (!implFuncp || implFuncp->pureVirtual() || implFuncp->isStatic()) પાછા ખોટું;
        અચલ AstClass* અચલ implClassp
            = implp->parentp() ? VN_CAST(implp->parentp()->nodep(), Class) : nullptr;
        અચલ AstClass* અચલ ifClassp
            = ifSymp->parentp() ? VN_CAST(ifSymp->parentp()->nodep(), Class) : nullptr;
        જો (!implClassp || !ifClassp) પાછા ખોટું;
        જો (extendsClass(implClassp, ifClassp)) પાછા સાચું;
        જો (!isVirtualMethod(implFuncp, implClassp)) પાછા ખોટું;
        needForwardr = સાચું;
        પાછા સાચું;
    }
    // Add 'implp' override to 'classp' that calls super.implp, so the C++ method of an
    // implemented interface class is overridden
    શૂન્ય addInterfaceForward(AstClass* classp, Ast_ગાંઠ_FTask* implp, AstClass* implClassp) {
        File_લીટી* અચલ flp = implp->fileline();
        AstVar* fvarp = nullptr;
        Ast_ગાંઠ_FTask* newp;
        જો (AstFunc* અચલ implFuncp = VN_CAST(implp, Func)) {
            fvarp = VN_AS(implFuncp->fvarp(), Var)->clone_વૃક્ષ(ખોટું);
            newp = નવું AstFunc{flp, implp->name(), nullptr, fvarp};
        } નહીંતર {
            newp = નવું AstTask{flp, implp->name(), nullptr};
        }
        newp->classMethod(સાચું);
        newp->isVirtual(સાચું);
        newp->lifetime(implp->lifetime());
        AstArg* argsp = nullptr;
        માટે (Ast_ગાંઠ* stmtp = implp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstVar* અચલ portp = VN_CAST(stmtp, Var);
            જો (!portp || !portp->isIO()) ચાલુ;
            AstVar* અચલ newPortp = portp->clone_વૃક્ષ(ખોટું);
            newp->addStmtsp(newPortp);
            argsp = Ast_ગાંઠ::addNext(
                argsp, નવું AstArg{flp, "", નવું AstVarRef{flp, newPortp, VAccess::READ}});
        }
        Ast_ગાંઠ_FTask_Ref* callp;
        જો (AstFunc* અચલ implFuncp = VN_CAST(implp, Func)) {
            callp = નવું AstFuncRef{flp, implFuncp, argsp};
        } નહીંતર {
            callp = નવું AstTaskRef{flp, VN_AS(implp, Task), argsp};
        }
        callp->classOrPackagep(implClassp);
        callp->superReference(સાચું);
        જો (fvarp) {
            newp->addStmtsp(નવું AstAssign{flp, નવું AstVarRef{flp, fvarp, VAccess::WRITE}, callp});
        } નહીંતર {
            newp->addStmtsp(callp->makeStmt());
        }
        UINFO(9, indent() << "Interface forward " << newp);
        classp->addMembersp(newp);
    }
    શૂન્ય importDerivedClass(AstClass* derivedClassp, VSymEnt* baseSymp, AstClass* baseClassp) {
        // Also used for standard 'extends' from a base class
        UINFO(8, indent() << "importDerivedClass to " << derivedClassp << " from " << baseClassp);
        માટે (VSymEnt::const_iterator it = baseSymp->begin(); it != baseSymp->end(); ++it) {
            જો (Ast_ગાંઠ* baseSubp = it->second->nodep()) {
                UINFO(8, indent() << "  SymFunc " << baseSubp);
                અચલ string impOrExtends
                    = baseClassp->isInterfaceClass() ? " implements " : " extends ";
                જો (VN_IS(baseSubp, ગાંઠ_FTask)) {
                    અચલ VSymEnt* અચલ foundp = m_curSymp->findIdFlat(baseSubp->name());
                    અચલ Ast_ગાંઠ_FTask* અચલ baseFuncp = VN_CAST(baseSubp, ગાંઠ_FTask);
                    જો (!baseFuncp || !baseFuncp->pureVirtual()) ચાલુ;
                    તાર્કિક needForward = ખોટું;
                    અચલ તાર્કિક existsInDerived
                        = foundp
                          && (!foundp->imported()
                              || inheritedImplements(foundp, it->second, needForward));
                    જો (needForward && m_statep->forPrimary()
                        && !derivedClassp->isInterfaceClass()) {
                        m_ifForwards.emplace(baseSubp->name(), foundp);
                    }
                    જો (m_statep->forPrimary() && !existsInDerived
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
                    અચલ auto itn = m_ifClassImpNames.find(baseSubp->name());
                    જો (m_statep->forPrimary() && !existsInDerived
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
                જો (VN_IS(baseSubp, Constraint)) {
                    અચલ VSymEnt* અચલ foundp = m_curSymp->findIdFlat(baseSubp->name());
                    અચલ AstConstraint* અચલ baseFuncp = VN_CAST(baseSubp, Constraint);
                    જો (!baseFuncp || !baseFuncp->isKwdPure()) ચાલુ;
                    અચલ તાર્કિક existsInDerived = foundp && !foundp->imported();
                    જો (m_statep->forPrimary() && !existsInDerived
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
    શૂન્ય importSymbolsFromExtended(AstClass* અચલ nodep, AstClassExtends* અચલ cextp) {
        AstClass* અચલ baseClassp = cextp->classp();
        VSymEnt* અચલ srcp = m_statep->get_ગાંઠ_Sym(baseClassp);
        importDerivedClass(nodep, srcp, baseClassp);
        જો (!cextp->isImplements()) m_curSymp->importFromClass(m_statep->symsp(), srcp);
    }
    શૂન્ય classExtendImport(AstClass* nodep) {
        // A class reference might be to a class that is later in Ast due to
        // e.g. parameterization or referring to a "class (type T) extends T"
        // Resolve it so later Class:: references into its base classes work
        symIterateNull(nodep, m_statep->get_ગાંઠ_Sym(nodep));
    }

    શૂન્ય checkDeclOrder(Ast_ગાંઠ* nodep, Ast_ગાંઠ* declp) {
        અચલ uint32_t decl_Token_અંક = declp->decl_Token_અંક();
        જો (!decl_Token_અંક) પાછા;  // Not implemented/valid on this object
        જો (nodep->fileline()->token_અંક() < decl_Token_અંક) {
            UINFO(1, "Related node " << nodep->fileline()->token_અંક() << " " << nodep);
            UINFO(1, "Related decl " << decl_Token_અંક << " " << declp);
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
    શૂન્ય checkMemberDeclOrder(Ast_ગાંઠ* nodep, AstMemberDType* declp) {
        અચલ uint32_t decl_Token_અંક = declp->fileline()->token_અંક();
        જો (nodep->fileline()->token_અંક() < decl_Token_અંક) {
            UINFO(1, "Related node " << nodep->fileline()->token_અંક() << " " << nodep);
            UINFO(1, "Related decl " << decl_Token_અંક << " " << declp);
            nodep->v3error("Reference to "
                           << nodep->prettyNameQ() << " before declaration (IEEE 1800-2023 6.18)\n"
                           << nodep->warnMore()
                           << "... Suggest move the declaration before the reference\n"
                           << nodep->warnContextPrimary() << '\n'
                           << declp->warnOther() << "... Location of original declaration\n"
                           << declp->warnContextSecondary());
        }
    }

    શૂન્ય replaceWithCheckBreak(Ast_ગાંઠ* oldp, Ast_ગાંઠ_DType* newp) {
        // Flag now to avoid V3Broken throwing an internal error
        જો (oldp->wouldBreak(newp)) {
            newp->v3error(
                "Data type used where a non-data type is expected: " << newp->prettyNameQ());
            oldp->replaceWith(નવું AstConst{newp->fileline(), AstConst::BitFalseErroring{}});
        } નહીંતર {
            oldp->replaceWith(newp);
        }
    }

    // Marks the current module to be revisited after the initial AST iteration
    શૂન્ય revisitLater(Ast_ગાંઠ* deferredNodep) {
        // Need to revisit entire module to build up all the necessary context
        m_lastDeferredp = deferredNodep;
        m_modulesToRevisit.ઉમેરો(std::make_pair(m_modp->name(), m_modp));
    }

    શૂન્ય updateVarUse(AstVar* nodep) {
        // Avoid dotted.PARAM false positive when in a parameter block
        // that is if ()'ed off by same dotted name as another block
        જો (nodep && nodep->isParam()) nodep->usedParam(સાચું);
    }

    સ્થિર VSymEnt* findIdFallbackSkipMemberDType(VSymEnt* lookp, અચલ string& name) {
        VSymEnt* shadowEntp = nullptr;  // Shadowing variable: not a type, kept for error report
        જ્યારે (lookp) {
            VSymEnt* અચલ foundp = lookp->findIdFlat(name);
            જો (foundp && !VN_IS(foundp->nodep(), MemberDType)) {
                // Non-type entries are not type candidates
                // (IEEE 1800-2023 6.18); skip them so an enclosing
                // type is found, but keep one to preserve the "found: ..." error.
                જો (VN_IS(foundp->nodep(), Typedef) || VN_IS(foundp->nodep(), ParamTypeDType)
                    || VN_IS(foundp->nodep(), Class)) {
                    પાછા foundp;
                }
                જો (!shadowEntp) shadowEntp = foundp;
            }
            lookp = lookp->fallbackp();
        }
        પાછા shadowEntp;
    }

    શૂન્ય symIterateChildren(Ast_ગાંઠ* nodep, VSymEnt* symp) {
        // Iterate children, changing to given context, with restore to old context
        VL_RESTORER_COPY(m_ds);
        VL_RESTORER(m_curSymp);
        m_curSymp = symp;
        m_ds.init(m_curSymp);
        iterateChildren(nodep);
    }
    શૂન્ય symIterateNull(Ast_ગાંઠ* nodep, VSymEnt* symp) {
        // Iterate node, changing to given context, with restore to old context
        VL_RESTORER_COPY(m_ds);
        VL_RESTORER(m_curSymp);
        m_curSymp = symp;
        m_ds.init(m_curSymp);
        iterateNull(nodep);
    }
    સ્થિર અચલ Ast_ગાંઠ_DType* get_પદાવલિ_DTypep(અચલ Ast_ગાંઠ_પદાવલિ* selp) {
        જ્યારે (અચલ Ast_ગાંઠ_Pre_Sel* અચલ sp = VN_CAST(selp, ગાંઠ_Pre_Sel)) selp = sp->fromp();
        જો (અચલ AstMemberSel* અચલ sp = VN_CAST(selp, MemberSel)) {
            જો (અચલ Ast_ગાંઠ_DType* dtypep = get_પદાવલિ_DTypep(sp->fromp())) {
                જો (અચલ AstClassRefDType* અચલ classRefp = VN_CAST(dtypep, ClassRefDType)) {
                    અચલ AstClass* અચલ classp = classRefp->classp();
                    અચલ તાર્કિક found = classp->existsMember(
                        [&dtypep, name = selp->name()](અચલ AstClass*, અચલ AstVar* nodep) {
                            dtypep = nodep->childDTypep();
                            પાછા nodep->name() == name;
                        });
                    જો (found) પાછા dtypep->elemDTypep();
                    selp->v3error("Class " << classRefp->prettyNameQ()
                                           << " does not contain field " << selp->prettyNameQ());
                } નહીંતર {
                    selp->v3fatalSrc("Member selection on expression of type "
                                     << dtypep->prettyDTypeNameQ()
                                     << ", which is not a class type");
                }
            }
        } નહીંતર જો (અચલ Ast_ગાંઠ_Var_Ref* અચલ varRefp = VN_CAST(selp, ગાંઠ_Var_Ref)) {
            પાછા varRefp->varp()->childDTypep()->elemDTypep();
        }
        પાછા nullptr;
    }
    સ્થિર અચલ AstVar* getNextVarp(અચલ Ast_ગાંઠ* stmtsp) {
        // Only IO ports, as parameters are on paramsp(), not the pinsp() list paired here
        જ્યારે (stmtsp) {
            જો (અચલ AstVar* અચલ varp = VN_CAST(stmtsp, Var)) {
                જો (varp->isIO()) પાછા varp;
            }
            stmtsp = stmtsp->nextp();
        }
        પાછા nullptr;
    }
    // Introduce implicit parameters for modules with generic interafeces
    શૂન્ય addImplicitParametersOfGenericIface(AstCell* અચલ nodep, અચલ AstModule* અચલ modp) {
        // Get Param number
        પૂર્ણાંક param_અંક = 1;
        માટે (AstPin* paramp = nodep->paramsp(); paramp; paramp = VN_CAST(paramp->nextp(), Pin)) {
            ++param_અંક;
        }

        // Add each implicit parameter type
        અચલ AstVar* modIfaceVarp = getNextVarp(modp->stmtsp());
        માટે (અચલ AstPin* pinp = nodep->pinsp(); pinp && modIfaceVarp;
             pinp = VN_CAST(pinp->nextp(), Pin),
                           modIfaceVarp = getNextVarp(modIfaceVarp->nextp())) {
            જો (modIfaceVarp->varType() != VVarType::IFACEREF
                || !VN_IS(modIfaceVarp->childDTypep()->elemDTypep(), IfaceGenericDType)) {
                ચાલુ;
            }
            Ast_ગાંઠ* exprp = pinp->exprp();
            જો (!exprp) {
                modIfaceVarp->v3error("Interface port "
                                      << modIfaceVarp->prettyNameQ()
                                      << " is not connected to interface/modport pin expression");
                ચાલુ;
            }
            જ્યારે (અચલ Ast_ગાંઠ_Pre_Sel* અચલ preSelp = VN_CAST(exprp, ગાંઠ_Pre_Sel)) {
                exprp = preSelp->fromp();
            }
            // Resolve pin expression to the enclosing module's port Var. At primary
            // LinkDot the expression may still be an AstParseRef, so also look up by name.
            AstVar* enclosingVarp = nullptr;
            અચલ AstVarRef* અચલ varRefp = VN_CAST(exprp, VarRef);
            જો (varRefp) {
                enclosingVarp = varRefp->varp();
            } નહીંતર જો (અચલ AstParseRef* અચલ parseRefp = VN_CAST(exprp, ParseRef)) {
                જો (m_modp) {
                    જો (VSymEnt* અચલ symp
                        = m_statep->get_ગાંઠ_Sym(m_modp)->findIdFlat(parseRefp->name())) {
                        enclosingVarp = VN_CAST(symp->nodep(), Var);
                    }
                }
            }
            જો (enclosingVarp && enclosingVarp->varType() == VVarType::IFACEREF
                && VN_IS(enclosingVarp->childDTypep()->skipRefp(), IfaceGenericDType)) {
                // Nested generic-iface forwarding (#7454): enclosing port is itself still
                // generic, so emit a placeholder __VGIfaceParam pin carrying a VarRef to
                // the outer port. V3Param rewrites it to the concrete IfaceRefDType once
                // the enclosing module is specialized (see
                // ParamProcessor::resolveGenericIfaceForwardingPins for the ordering
                // constraint that keeps the rewrite inside V3Param).
                AstVarRef* અચલ fwdRefp
                    = નવું AstVarRef{exprp->fileline(), enclosingVarp, VAccess::READ};
                AstPin* અચલ newPinp = નવું AstPin{
                    pinp->fileline(), param_અંક, "__VGIfaceParam" + modIfaceVarp->name(), fwdRefp};
                newPinp->param(સાચું);
                visit(newPinp);
                nodep->addParamsp(newPinp);
            } નહીંતર જો (varRefp) {
                અચલ AstVar* અચલ varp = varRefp->varp();
                જો (અચલ AstIfaceRefDType* અચલ refp
                    = VN_CAST(varp->childDTypep()->elemDTypep(), IfaceRefDType)) {
                    AstIface* અચલ ifacep = VN_AS(refp->cellp()->modp(), Iface);
                    AstIfaceRefDType* newIfaceRefp;
                    જો (refp->modportp()) {
                        newIfaceRefp = નવું AstIfaceRefDType{
                            refp->fileline(), refp->modportFileline(), m_modp->name(),
                            ifacep->name(), refp->modportName()};
                    } નહીંતર {
                        newIfaceRefp = નવું AstIfaceRefDType{refp->fileline(), m_modp->name(),
                                                            ifacep->name()};
                    }
                    newIfaceRefp->ifacep(ifacep);
                    જો (refp->cellp()->paramsp()) {
                        newIfaceRefp->addParamsp(refp->cellp()->paramsp()->clone_વૃક્ષ(સાચું));
                    }
                    UASSERT_OBJ(pinp->name().find("__pinNumber") == 0
                                    || pinp->name() == modIfaceVarp->name(),
                                pinp, "Not found interface with such name");
                    AstPin* અચલ newPinp
                        = નવું AstPin{pinp->fileline(), param_અંક,
                                     "__VGIfaceParam" + modIfaceVarp->name(), newIfaceRefp};
                    newPinp->param(સાચું);
                    visit(newPinp);
                    nodep->addParamsp(newPinp);
                } નહીંતર {
                    varRefp->v3error("Generic interfaces can only connect to an interface and "
                                     << varp->prettyNameQ() << " is "
                                     << (varp->childDTypep()
                                             ? "of type " + varp->childDTypep()->prettyDTypeNameQ()
                                             : "not an interface"));
                }
            } નહીંતર {
                exprp->v3error("Expected an interface but " << exprp->prettyNameQ()
                                                            << " is not an interface");
            }
            ++param_અંક;
        }
    }

    // Returns true and issues error iff 'varp' cannot be accesed with a VarXRef
    // in a hier_block given by 'scopeSymp'
    તાર્કિક errorHierNonPort(AstVarXRef* refp, AstVar* varp, VSymEnt* scopeSymp) {
        // OK to access ports on hier_block
        જો (varp->isIO()) પાછા ખોટું;
        // OK if not an instance
        અચલ AstCell* અચલ cellp = VN_CAST(scopeSymp->nodep(), Cell);
        જો (!cellp) પાછા ખોટું;
        // Ok if not a hier_block
        અચલ Ast_ગાંઠ_Module* અચલ modp = cellp->modp();
        જો (!modp->hierBlock()) પાછા ખોટું;
        // Bad
        refp->v3error("Cannot access non-port symbols inside hierarchical block");
        પાછા સાચું;
    }

#define LINKDOT_VISIT_START() \
    VL_RESTORER(m_indent); \
    ++m_indent;

    string indent() અચલ {
        string result = "";
        result.ઉમેરો(0, m_indent, ':');
        પાછા result + " ";
    }

    // VISITORS
    શૂન્ય visit(AstNetlist* nodep) override {
        // Recurse..., backward as must do packages before using packages
        iterateChildrenBackwardsConst(nodep);
    }
    શૂન્ય visit(AstTypeTable*) override {}
    શૂન્ય visit(Ast_ગાંઠ_Module* nodep) override {
        જો (nodep->dead()) પાછા;
        LINKDOT_VISIT_START();
        UINFO(8, indent() << "visit " << nodep);
        VL_RESTORER(m_genericIfaceModule);
        જો (અચલ AstModule* અચલ modp = VN_CAST(nodep, Module)) {
            m_genericIfaceModule = modp->hasGenericIface();
        }
        checkNoDot(nodep);
        m_ds.init(m_curSymp);
        m_ds.m_dotSymp = m_curSymp = m_modSymp
            = m_statep->get_ગાંઠ_Sym(nodep);  // Until overridden by a SCOPE
        m_cellp = nullptr;
        m_modp = nodep;
        m_modport_અંક = 0;
        iterateChildren(nodep);
        m_modp = nullptr;
        m_ds.m_dotSymp = m_curSymp = m_modSymp = nullptr;
        m_ds.m_dotPos = DP_NONE;
        m_ds.m_dotErr = ખોટું;
    }
    શૂન્ય visit(AstScope* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(8, indent() << "visit " << nodep);
        checkNoDot(nodep);
        VL_RESTORER(m_modSymp);
        VL_RESTORER(m_curSymp);
        m_ds.m_dotSymp = m_curSymp = m_modSymp = m_statep->getScopeSym(nodep);
        iterateChildren(nodep);
        m_ds.m_dotSymp = m_curSymp = m_modSymp = nullptr;
    }
    શૂન્ય visit(AstCellInline* nodep) override {
        LINKDOT_VISIT_START();
        checkNoDot(nodep);
        UASSERT_OBJ(m_statep->forScopeCreation(), nodep, "CellInline should not exist yet");
        VL_DO_DANGLING(ધકેલો_Deletep(nodep->unlinkFrBack()), nodep);
    }
    શૂન્ય visit(AstCell* nodep) override {
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
            જો (VN_IS(nodep->modp(), NotFoundModule)) {
                // Prevent warnings about missing pin connects
                જો (nodep->pinsp()) nodep->pinsp()->unlinkFrBackWithNext()->delete_વૃક્ષ();
                જો (nodep->paramsp()) nodep->paramsp()->unlinkFrBackWithNext()->delete_વૃક્ષ();
            }
            // Need to pass the module info to this cell, so we can link up the pin names
            // However can't use m_curSymp as pin connections need to use the
            // instantiator's symbols
            નહીંતર {
                m_pinSymp = m_statep->get_ગાંઠ_Sym(nodep->modp());
                UINFO(4, indent() << "(Backto) visit " << nodep);
                // UINFOTREE(1, nodep, "", "linkcell");
                // UINFOTREE(1, nodep->modp(), "", "linkcemd");
                iterateChildren(nodep);

                જો (m_statep->forPrimary())
                    જો (અચલ AstModule* અચલ modp = VN_CAST(nodep->modp(), Module)) {
                        જો (modp->hasGenericIface())
                            addImplicitParametersOfGenericIface(nodep, modp);
                    }
            }
        }
        // Parent module inherits child's publicity
        // This is done bottom up in the LinkBotupVisitor stage
    }
    શૂન્ય visit(AstClassRefDType* nodep) override {
        // Cell: Recurse inside or cleanup not founds
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        // Can be under dot if called as package::class and that class resolves, so no checkNoDot
        VL_RESTORER_CLEAR(m_usedPins);
        VL_RESTORER_CLEAR(m_usedDefParamPins);
        UASSERT_OBJ(nodep->classp(), nodep, "ClassRef has unlinked class");
        UASSERT_OBJ(m_statep->forPrimary() || !nodep->paramsp() || V3Error::error_ગણતરી(), nodep,
                    "class reference parameter not removed by V3Param");
        VL_RESTORER(m_pinSymp);
        {
            // ClassRef's have pins, so track
            m_pinSymp = m_statep->get_ગાંઠ_Sym(nodep->classp());
            UINFO(4, indent() << "(Backto) visit " << nodep);
            // UINFOTREE(1, nodep, "", "linkcell");
            // UINFOTREE(1, nodep->modp(), "", "linkcemd");
            iterateChildren(nodep);
        }
    }
    શૂન્ય visit(AstPin* nodep) override {
        // Pin: Link to submodule's port
        LINKDOT_VISIT_START();
        checkNoDot(nodep);
        iterateChildren(nodep);
        જો (!nodep->modVarp()) {
            UASSERT_OBJ(m_pinSymp, nodep, "Pin not under instance?");
            VSymEnt* અચલ foundp = m_pinSymp->findIdFlat(nodep->name());
            અચલ અક્ષર* અચલ whatp = nodep->param() ? "parameter" : "pin";
            જો (!foundp) {
                જો (m_cellp && VN_IS(m_cellp->modp(), Primitive)
                    && (nodep->name() == "__paramNumber1" || nodep->name() == "__paramNumber2")) {
                    // Primitive parameter is really a delay2 we can just ignore
                    VL_DO_DANGLING(ધકેલો_Deletep(nodep->unlinkFrBack()), nodep);
                    પાછા;
                } નહીંતર જો (nodep->param() && !nodep->exprp() && !nodep->svDotName()) {
                    // Placeholder pin the parser makes for an empty '#()', to signal
                    // the user did type the '#()'. No further use, deleting.
                    VL_DO_DANGLING(ધકેલો_Deletep(nodep->unlinkFrBack()), nodep);
                    પાછા;
                } નહીંતર {
                    અચલ std::string suggest
                        = (nodep->param()
                               ? m_statep->suggestSymFlat(m_pinSymp, nodep->name(),
                                                          Link_ગાંઠ_Matcher_Var_Param{}, ખોટું)
                               : m_statep->suggestSymFlat(m_pinSymp, nodep->name(),
                                                          Link_ગાંઠ_Matcher_Var_IO{}, ખોટું));
                    અચલ std::string decl
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
                    VL_DO_DANGLING(ધકેલો_Deletep(nodep->unlinkFrBack()), nodep);
                    પાછા;
                }
            }
            VVarType refVarType = VVarType::UNKNOWN;
            તાર્કિક wrongPinType = ખોટું;
            જો (AstVar* અચલ varp = VN_CAST(foundp->nodep(), Var)) {
                જો (varp->isIO() || varp->isParam() || varp->isIfaceRef()) {
                    refVarType = varp->varType();
                    nodep->modVarp(varp);
                } નહીંતર {
                    wrongPinType = સાચું;
                }
            } નહીંતર જો (AstParamTypeDType* અચલ typep = VN_CAST(foundp->nodep(), ParamTypeDType)) {
                refVarType = typep->varType();
                nodep->modPTypep(typep);
            } નહીંતર {
                wrongPinType = સાચું;
            }
            // Don't connect parameter pin to module ports or vice versa
            જો (nodep->param() != (refVarType == VVarType::GPARAM)) wrongPinType = સાચું;
            જો (wrongPinType) {
                string targetType = LinkDotState::ગાંઠ_Text_Type(foundp->nodep());
                targetType = VString::aOrAn(targetType) + ' ' + targetType;
                જો (nodep->param()) {
                    nodep->v3error("Instance attempts to override "
                                   << nodep->prettyNameQ() << " as a " << whatp << ", but it is "
                                   << targetType);
                } નહીંતર {
                    nodep->v3error("Instance attempts to connect to "
                                   << nodep->prettyNameQ() << ", but it is " << targetType);
                }
                પાછા;
            }
            markAndCheckPinDup(nodep, foundp->nodep(), whatp);
        }
        // Early return() above when deleted
    }

    સ્થિર તાર્કિક isDotThisReference(અચલ AstDot* nodep) {
        પાછા VN_IS(nodep->lhsp(), ParseRef) && nodep->lhsp()->name() == "this";
    }

    શૂન્ય visit(AstDot* nodep) override {
        // Legal under a DOT: AstDot, AstParseRef, AstPackageRef, AstNodeSel, AstPackArrayDType
        //    also a DOT can be part of an expression, but only above plus
        //    AstFTaskRef are legal children
        // Dot(PackageRef, ParseRef(text))
        // Dot(Dot(ClassOrPackageRef,ClassOrPackageRef), ParseRef(text))
        // Dot(Dot(Dot(ParseRef(text), ...
        જો (nodep->user3SetOnce()) પાછા;
        LINKDOT_VISIT_START();
        UINFO(8, indent() << "visit " << nodep);
        UINFO(8, indent() << m_ds.ascii());
        અચલ DotStates lastStates = m_ds;
        અચલ તાર્કિક start = (m_ds.m_dotPos == DP_NONE);  // Save, as m_dotp will be changed
        અચલ DotPosition initialDotPos = m_ds.m_dotPos;
        VL_RESTORER(m_randSymp);
        {
            જો (start) {  // Starting dot sequence
                UINFOTREE(9, nodep, "", "dot-in");
                // Keep deferral from visit(AstMethodCall) for unspecialized rand target
                m_ds.init(m_curSymp, m_ds.m_unresolvedClass && randTargetUnresolved());
            }
            m_ds.m_dotp = nodep;  // Always, not just at start
            m_ds.m_dotPos = DP_FIRST;

            જો (isDotThisReference(nodep)) {
                VSymEnt* classSymp = getThisClassSymp();
                // In 'randomize() with { this.member }', 'this' refers to randomized
                // object, not the calling class (IEEE 1800-2023 18.7)
                જો (m_randSymp && m_currentWithp) classSymp = m_randSymp;
                જો (!classSymp && randTargetUnresolved()) {
                    // Bind 'this' in post-V3Param pass
                } નહીંતર જો (!classSymp) {
                    nodep->v3error("'this' used outside class (IEEE 1800-2023 8.11)");
                    m_ds.m_dotErr = સાચું;
                } નહીંતર {
                    m_ds.m_dotSymp = classSymp;
                    UINFO(8, indent() << "this. " << m_ds.ascii());
                }
            } નહીંતર જો (VN_IS(nodep->lhsp(), ParseRef) && nodep->lhsp()->name() == "super") {
                અચલ VSymEnt* classSymp = getThisClassSymp();
                જો (!classSymp) {
                    nodep->v3error("'super' used outside class (IEEE 1800-2023 8.15)");
                    m_ds.m_dotErr = સાચું;
                } નહીંતર {
                    અચલ auto classp = VN_AS(classSymp->nodep(), Class);
                    અચલ auto cextp = classp->extendsp();
                    જો (!cextp) {
                        nodep->v3error("'super' used on non-extended class (IEEE 1800-2023 8.15)");
                        m_ds.m_dotErr = સાચું;
                    } નહીંતર {
                        જો (m_statep->forPrimary()
                            && m_extendsParam.find(classp) != m_extendsParam.end()) {
                            UINFO(9, indent()
                                         << "deferring until post-V3Param: " << nodep->lhsp());
                            m_ds.m_unresolvedClass = સાચું;
                        } નહીંતર {
                            અચલ auto baseClassp = cextp->classp();
                            UASSERT_OBJ(baseClassp, nodep, "Bad superclass");
                            m_ds.m_dotSymp = m_statep->get_ગાંઠ_Sym(baseClassp);
                            m_ds.m_super = સાચું;
                            UINFO(8, indent() << "super. " << m_ds.ascii());
                        }
                    }
                }
            } નહીંતર જો (AstClassOrPackageRef* અચલ lhsp
                       = VN_CAST(nodep->lhsp(), ClassOrPackageRef)) {
                // m_ds.m_dotText communicates the cell prefix between stages
                UINFO(8, indent() << "iter.lhs   " << m_ds.ascii() << " " << nodep);
                iterateAndNextNull(lhsp);
                જો (!lhsp->classOrPackageSkipp() && lhsp->name() != "local::") {
                    revisitLater(nodep);
                    m_ds = lastStates;
                    // Resolve function args before bailing
                    જો (Ast_ગાંઠ_FTask_Ref* અચલ ftaskrefp = VN_CAST(nodep->rhsp(), ગાંઠ_FTask_Ref)) {
                        iterateAndNextNull(ftaskrefp->argsp());
                    }
                    પાછા;
                }
                m_ds.m_dotPos = DP_PACKAGE;
                // nodep->lhsp() may be a new node
                જો (AstClassOrPackageRef* અચલ classOrPackageRefp
                    = VN_CAST(nodep->lhsp(), ClassOrPackageRef)) {
                    જો (Ast_ગાંઠ* અચલ classOrPackageNodep
                        = classOrPackageRefp->classOrPackageSkipp()) {
                        m_ds.m_dotSymp = m_statep->get_ગાંઠ_Sym(classOrPackageNodep);
                    }
                }
                UINFO(8, indent() << "iter.ldone " << m_ds.ascii() << " " << nodep);
            } નહીંતર જો (VN_IS(nodep->lhsp(), Dot) && VN_AS(nodep->lhsp(), Dot)->colon()) {
                // m_ds.m_dotText communicates the cell prefix between stages
                UINFO(8, indent() << "iter.lhs   " << m_ds.ascii() << " " << nodep);
                m_ds.m_dotPos = DP_PACKAGE;
                iterateAndNextNull(nodep->lhsp());
                // nodep->lhsp() may be a new node
                જો (AstClassOrPackageRef* અચલ crefp
                    = VN_CAST(nodep->lhsp(), ClassOrPackageRef)) {
                    જો (!crefp->classOrPackageSkipp()) {
                        revisitLater(nodep);
                        m_ds = lastStates;
                        // Resolve function args before bailing
                        જો (Ast_ગાંઠ_FTask_Ref* અચલ ftaskrefp
                            = VN_CAST(nodep->rhsp(), ગાંઠ_FTask_Ref)) {
                            iterateAndNextNull(ftaskrefp->argsp());
                        }
                        પાછા;
                    }
                }
                જો (m_lastDeferredp == nodep->lhsp()) {
                    // LHS got deferred, so this node won't be resolved. Defer it too
                    m_ds = lastStates;
                    // Resolve function args before bailing
                    જો (Ast_ગાંઠ_FTask_Ref* અચલ ftaskrefp = VN_CAST(nodep->rhsp(), ગાંઠ_FTask_Ref)) {
                        iterateAndNextNull(ftaskrefp->argsp());
                    }
                    પાછા;
                }
                UINFO(8, indent() << "iter.ldone " << m_ds.ascii() << " " << nodep);
            } નહીંતર {
                m_ds.m_dotPos = DP_FIRST;
                m_ds.m_unresolvedGenericIface = ખોટું;
                UINFO(8, indent() << "iter.lhs   " << m_ds.ascii() << " " << nodep);
                iterateAndNextNull(nodep->lhsp());
                UINFO(8, indent() << "iter.ldone " << m_ds.ascii() << " " << nodep);
                જો (m_ds.m_unresolvedGenericIface) {
                    m_ds.m_dotPos = initialDotPos;
                    પાછા;
                }
                // UINFOTREE(9, nodep, "", "dot-lho");
            }
            જો (m_statep->forPrimary() && isParamedClassRef(nodep->lhsp())) {
                // Dots of paramed classes will be linked after deparameterization
                UINFO(9, indent() << "deferring until post-V3Param: " << nodep->lhsp());
                m_ds.m_unresolvedClass = સાચું;
            }
            જો (m_ds.m_unresolvedCell
                && (VN_IS(nodep->lhsp(), CellRef) || VN_IS(nodep->lhsp(), CellArrayRef))) {
                m_ds.m_unlinkedScopep = nodep->lhsp();
            }
            જો (m_ds.m_dotPos == DP_FIRST) m_ds.m_dotPos = DP_SCOPE;
            જો (!m_ds.m_dotErr) {  // Once something wrong, give up
                // Top 'final' dot RHS is final RHS, else it's a
                // DOT(DOT(x,*here*),real-rhs) which we consider a RHS
                જો (start && m_ds.m_dotPos == DP_SCOPE) m_ds.m_dotPos = DP_FINAL;
                UINFO(8, indent() << "iter.rhs   " << m_ds.ascii() << " " << nodep);
                // m_ds.m_dotSymp points at lhsp()'s symbol table, so resolve RHS under that
                iterateAndNextNull(nodep->rhsp());
                UINFO(8, indent() << "iter.rdone " << m_ds.ascii() << " " << nodep);
                // UINFOTREE(9, nodep, "", "dot-rho");
            }
            જો (!m_ds.m_unresolvedClass) {
                જો (start) {
                    Ast_ગાંઠ* newp;
                    જો (m_ds.m_dotErr) {
                        newp = નવું AstConst{nodep->fileline(), AstConst::BitFalse{}};
                    } નહીંતર {
                        newp = nodep->rhsp()->unlinkFrBack();
                    }
                    UINFOTREE(9, newp, "", "dot-out");
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
                } નહીંતર {  // Dot midpoint
                    Ast_ગાંઠ* newp = nodep->rhsp()->unlinkFrBack();
                    જો (m_ds.m_unresolvedCell) {
                        AstCellRef* અચલ crp = નવું AstCellRef{
                            nodep->fileline(), nodep->name(), nodep->lhsp()->unlinkFrBack(), newp};
                        newp = crp;
                    }
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
                }
            }
        }
        જો (start) {
            m_ds = lastStates;
        } નહીંતર {
            અચલ તાર્કિક unresolvedClass = m_ds.m_unresolvedClass;
            m_ds.m_dotp = lastStates.m_dotp;
            m_ds.m_unresolvedClass |= unresolvedClass;
        }
        UINFO(8, indent() << "done " << m_ds.ascii() << " " << nodep);
    }
    શૂન્ય visit(AstDefaultClocking* nodep) override {
        જો (VSymEnt* અચલ foundp = m_curSymp->findIdFallback(nodep->name())) {
            જો (AstClocking* અચલ clockingp = VN_CAST(foundp->nodep(), Clocking)) {
                clockingp->makeDefault();
                VL_DO_DANGLING(nodep->unlinkFrBack()->delete_વૃક્ષ(), nodep);
            } નહીંતર {
                nodep->v3error(nodep->prettyNameQ() << " is not a clocking identifier");
            }
        } નહીંતર {
            nodep->v3error("Can't find definition of clocking: " << nodep->prettyNameQ());
        }
    }
    શૂન્ય visit(AstSenItem* nodep) override {
        LINKDOT_VISIT_START();
        VL_RESTORER(m_inSens);
        m_inSens = સાચું;
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstPatMember* nodep) override {
        LINKDOT_VISIT_START();
        જો (nodep->varrefp()) પાછા;  // only do this mapping once
        // If we have a TEXT token as our key, lookup if it's a LPARAM
        જો (AstText* અચલ textp = VN_CAST(nodep->keyp(), Text)) {
            UINFO(9, indent() << "visit " << nodep);
            UINFO(9, indent() << "      " << textp);
            // Lookup
            જો (VSymEnt* અચલ foundp = m_curSymp->findIdFallback(textp->text())) {
                જો (AstVar* અચલ varp = VN_CAST(foundp->nodep(), Var)) {
                    જો (varp->isParam() || varp->isGenVar()) {
                        // Attach found Text reference to PatMember
                        nodep->varrefp(
                            નવું AstVarRef{nodep->fileline(),
                                          foundp->imported() ? foundp->classOrPackagep() : nullptr,
                                          varp, VAccess::READ});
                        UINFO(9, indent() << " new " << nodep->varrefp());
                    }
                }
                જો (AstEnumItem* અચલ itemp = VN_CAST(foundp->nodep(), EnumItem)) {
                    // Attach enum item value to PatMember
                    nodep->varrefp(
                        નવું AstEnumItemRef{nodep->fileline(), itemp, foundp->classOrPackagep()});
                    UINFO(9, indent() << " new " << itemp);
                }
            }
        }
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstParseRef* nodep) override {
        જો (nodep->user3SetOnce()) પાછા;
        LINKDOT_VISIT_START();
        UINFO(9, indent() << "visit " << nodep);
        UINFO(9, indent() << m_ds.ascii());
        જો (m_ds.m_unresolvedClass) પાછા;
        // m_curSymp is symbol table of outer expression
        // m_ds.m_dotSymp is symbol table relative to "."'s above now
        UASSERT_OBJ(m_ds.m_dotSymp, nodep, "nullptr lookup symbol table");
        // Generally resolved during Primary, but might be at param time under AstUnlinkedRef
        UASSERT_OBJ(!m_statep->forScopeCreation(), nodep, "ParseRefs should no longer exist");
        અચલ DotStates lastStates = m_ds;
        અચલ તાર્કિક start = (m_ds.m_dotPos == DP_NONE);  // Save, as m_dotp will be changed
        તાર્કિક first = start || m_ds.m_dotPos == DP_FIRST;

        જો (start) {
            m_ds.init(m_curSymp);
            // Note m_ds.m_dotp remains nullptr; this is a reference not under a dot
        }
        જો (nodep->name() == "super") {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: super");
            m_ds.m_dotErr = સાચું;
        }
        જો (nodep->name() == "this") {
            iterateChildren(nodep);
            જો (m_statep->forPrimary()) પાછા;  // The class might be parameterized somewhere
            અચલ VSymEnt* classSymp = getThisClassSymp();
            જો (!classSymp) {
                nodep->v3error("'this' used outside class (IEEE 1800-2023 8.11)");
                પાછા;
            }
            AstClass* અચલ classp = VN_AS(classSymp->nodep(), Class);
            AstClassRefDType* અચલ dtypep
                = નવું AstClassRefDType{nodep->fileline(), classp, nullptr};
            AstThisRef* અચલ newp = નવું AstThisRef{nodep->fileline(), VFlagChildDType{}, dtypep};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
            પાછા;
        }
        જો (m_ds.m_dotPos == DP_MEMBER && VN_IS(m_ds.m_dotp->lhsp(), LambdaArgRef)
            && nodep->name() == "index") {
            // 'with' statement's 'item.index'
            // m_ds.dotp->lhsp() was checked to know if `index` is directly after lambda arg ref.
            // If not, treat it as normal member select
            iterateChildren(nodep);
            AstLambdaArgRef* અચલ newp = નવું AstLambdaArgRef{
                nodep->fileline(), m_ds.m_dotp->lhsp()->name() + "__DOT__index", સાચું};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
            પાછા;
        } નહીંતર જો (m_ds.m_dotPos == DP_MEMBER) {
            // Found a Var, everything following is membership.  {scope}.{var}.HERE {member}
            જો (m_ds.m_dotErr) {
                nodep->unlinkFrBack();  // Avoid circular node loop on errors
            } નહીંતર {
                Ast_ગાંઠ_પદાવલિ* અચલ varEtcp = VN_AS(m_ds.m_dotp->lhsp()->unlinkFrBack(), ગાંઠ_પદાવલિ);
                Ast_ગાંઠ_પદાવલિ* અચલ newp = નવું AstMemberSel{nodep->fileline(), varEtcp,  //
                                                           VFlagChildDType{}, nodep->name()};
                nodep->replaceWith(newp);
            }
            VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
        } નહીંતર {
            //
            string expectWhat;
            તાર્કિક allowScope = ખોટું;
            તાર્કિક allowVar = ખોટું;
            તાર્કિક allowFTask = ખોટું;
            તાર્કિક staticAccess = ખોટું;
            જો (m_ds.m_disablep) {
                allowScope = સાચું;
                allowFTask = સાચું;
                expectWhat = "block/task";
            } નહીંતર જો (m_ds.m_dotPos == DP_PACKAGE) {
                // {package-or-class}::{a}
                Ast_ગાંઠ_Module* classOrPackagep = nullptr;
                expectWhat = "scope/variable/func";
                allowScope = સાચું;
                allowVar = સાચું;
                allowFTask = સાચું;
                staticAccess = સાચું;
                UINFO(9, "chk pkg " << m_ds.ascii() << " lhsp=" << m_ds.m_dotp->lhsp());
                UASSERT_OBJ(VN_IS(m_ds.m_dotp->lhsp(), ClassOrPackageRef), m_ds.m_dotp->lhsp(),
                            "Bad package link");
                AstClassOrPackageRef* અચલ cpackagerefp
                    = VN_AS(m_ds.m_dotp->lhsp(), ClassOrPackageRef);
                જો (cpackagerefp->name() == "local::") {
                    m_randSymp = nullptr;
                    first = સાચું;
                } નહીંતર જો (!cpackagerefp->classOrPackageSkipp()) {
                    VSymEnt* અચલ foundp = m_statep->resolveClassOrPackage(
                        m_ds.m_dotSymp, cpackagerefp, સાચું, ખોટું, ":: reference");
                    જો (!foundp) પાછા;
                    classOrPackagep = cpackagerefp->classOrPackageSkipp();
                    જો (classOrPackagep) {
                        m_ds.m_dotSymp = m_statep->get_ગાંઠ_Sym(classOrPackagep);
                    } નહીંતર {
                        m_ds = lastStates;
                        પાછા;
                    }
                } નહીંતર {
                    classOrPackagep = cpackagerefp->classOrPackageSkipp();
                    UASSERT_OBJ(classOrPackagep, m_ds.m_dotp->lhsp(), "Bad package link");
                    m_ds.m_dotSymp = m_statep->get_ગાંઠ_Sym(classOrPackagep);
                }
                m_ds.m_dotPos = DP_SCOPE;
            } નહીંતર જો (m_ds.m_dotPos == DP_SCOPE || m_ds.m_dotPos == DP_FIRST) {
                // {a}.{b}, where {a} maybe a module name
                // or variable, where dotting into structure member
                expectWhat = "scope/variable";
                allowScope = સાચું;
                allowVar = સાચું;
            } નહીંતર જો (m_ds.m_dotPos == DP_NONE) {
                expectWhat = "variable";
                allowVar = સાચું;
            } નહીંતર જો (m_ds.m_dotPos == DP_FINAL) {
                expectWhat = "variable/method";
                allowFTask = સાચું;
                allowVar = સાચું;
            } નહીંતર {
                UINFO(1, "ds=" << m_ds.ascii());
                nodep->v3fatalSrc("Unhandled DotPosition");
            }
            // Lookup
            VSymEnt* foundp;
            string baddot;
            VSymEnt* okSymp = nullptr;
            // Restricted 'with (id_list)': only id_list names bind into target class.
            જો (m_randSymp
                && (!m_currentWithp || m_currentWithp->nameResolvesToTarget(nodep->name()))) {
                foundp = m_randSymp->findIdFlat(nodep->name());
                જો (foundp) {
                    જો (m_currentWithp && m_currentWithp->restricted()) {
                        m_restrictedNamesUsed.ઉમેરો(nodep->name());
                    }
                    જો (!start) m_ds.m_dotPos = DP_MEMBER;
                    જો (!m_currentWithp) {
                        UASSERT_OBJ(m_randMethodCallp, nodep, "Expected to be under randomize()");
                        // This will start failing once complex expressions are allowed on the LHS
                        // of randomize() with args
                        UASSERT_OBJ(VN_IS(m_randMethodCallp->fromp(), VarRef), m_randMethodCallp,
                                    "Expected simple randomize target");
                        // A ParseRef is used here so that the dot RHS gets resolved
                        AstMemberSel* અચલ newp = નવું AstMemberSel{
                            nodep->fileline(), m_randMethodCallp->fromp()->clone_વૃક્ષ(ખોટું),
                            VFlagChildDType{}, nodep->name()};
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
                        પાછા;
                    }
                    UINFO(9, indent() << "randomize-with fromSym " << foundp->nodep());
                    AstLambdaArgRef* અચલ lambdaRefp
                        = નવું AstLambdaArgRef{nodep->fileline(), "__Vrandwith_obj", ખોટું};
                    AstMemberSel* newp = નવું AstMemberSel{nodep->fileline(), lambdaRefp,
                                                          VFlagChildDType{}, nodep->name()};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
                    પાછા;
                }
            }
            જો (allowScope) {
                foundp = m_statep->findDotted(nodep->fileline(), m_ds.m_dotSymp, nodep->name(),
                                              baddot, okSymp, first);  // Maybe nullptr
            } નહીંતર જો (first) {
                foundp = m_ds.m_dotSymp->findIdFallback(nodep->name());
            } નહીંતર {
                foundp = m_ds.m_dotSymp->findIdFlat(nodep->name());
            }
            જો (!foundp && m_ds.m_dotp && isDotThisReference(m_ds.m_dotp)) {
                અચલ AstClass* અચલ classp = VN_CAST(m_ds.m_dotSymp->nodep(), Class);
                જો (classp && classp->isCovergroup() && classp->covergroupEnclosingClassp()) {
                    VSymEnt* અચલ enclosingClassSymp
                        = m_statep->get_ગાંઠ_Sym(classp->covergroupEnclosingClassp());
                    foundp = enclosingClassSymp->findIdFallback(nodep->name());
                    જો (foundp) m_ds.m_dotSymp = enclosingClassSymp;
                }
            }
            // If not found in modport, check interface fallback for parameters and typedefs.
            // Parameters and typedefs are always visible through a modport (IEEE 1800-2023 25.5).
            // This mirrors the VarXRef modport parameter fallback in visit(AstVarXRef).
            જો (!foundp && VN_IS(m_ds.m_dotSymp->nodep(), Modport)
                && m_ds.m_dotSymp->fallbackp()) {
                VSymEnt* અચલ ifaceFallbackp = m_ds.m_dotSymp->fallbackp();
                VSymEnt* અચલ ifaceFoundp = ifaceFallbackp->findIdFlat(nodep->name());
                જો (ifaceFoundp) {
                    જો (અચલ AstVar* અચલ varp = VN_CAST(ifaceFoundp->nodep(), Var)) {
                        જો (varp->isParam()) foundp = ifaceFoundp;
                    } નહીંતર જો (VN_IS(ifaceFoundp->nodep(), Typedef)
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
            જો (!foundp && !okSymp && VN_IS(m_ds.m_dotSymp->nodep(), Modport)) {
                okSymp = m_ds.m_dotSymp;
            }
            જો (foundp) {
                UINFO(9, indent() << "found=se" << cvtToHex(foundp) << "  exp=" << expectWhat
                                  << "  n=" << foundp->nodep());
            }
            // What fell out?
            તાર્કિક ok = ખોટું;
            // Special case: waiting on clocking event
            જો (m_inSens && foundp && m_ds.m_dotPos != DP_SCOPE && m_ds.m_dotPos != DP_FIRST) {
                જો (AstClocking* અચલ clockingp = sensClockingp(foundp->nodep())) {
                    foundp = getCreateClockingEventSymEnt(clockingp);
                }
            }
            જો (!foundp) {
            } નહીંતર જો (VN_IS(foundp->nodep(), Cell) || VN_IS(foundp->nodep(), ગાંઠ_Block)
                       || VN_IS(foundp->nodep(), GenBlock)
                       || VN_IS(foundp->nodep(), Netlist)  // for $root
                       || VN_IS(foundp->nodep(), Module)) {  // if top
                જો (allowScope) {
                    ok = સાચું;
                    m_ds.m_dotText = VString::dot(m_ds.m_dotText, ".", nodep->name());
                    m_ds.m_dotSymp = foundp;
                    m_ds.m_dotPos = DP_SCOPE;
                    જો (m_ds.m_disablep && VN_IS(foundp->nodep(), ગાંઠ_Block)) {
                        // Possibly it is not the final link. If we are under dot and not in its
                        // last component, `targetp()` field will be overwritten by next components
                        m_ds.m_disablep->targetp(foundp->nodep());
                    }
                    જો (VN_IS(foundp->nodep(), GenBlock)) {
                        m_ds.m_genBlk = સાચું;
                        જો (m_ds.m_disablep) {
                            m_ds.m_disablep->v3warn(
                                E_UNSUPPORTED,
                                "Unsupported: Generate block referenced by disable");
                        }
                    }
                    // Upper AstDot visitor will handle it from here
                } નહીંતર જો (VN_IS(foundp->nodep(), Cell) && allowVar) {
                    AstCell* અચલ cellp = VN_AS(foundp->nodep(), Cell);
                    જો (VN_IS(cellp->modp(), Iface)) {
                        // Interfaces can be referenced like a variable for interconnect
                        VSymEnt* અચલ cellEntp = m_statep->get_ગાંઠ_Sym(cellp);
                        UASSERT_OBJ(cellEntp, nodep, "No interface sym entry");
                        VSymEnt* અચલ parentEntp
                            = cellEntp->parentp();  // Container of the var; probably a module or
                                                    // generate begin
                        AstVar* અચલ ifaceRefVarp
                            = find_Iface_ટોચ_Varp(nodep, parentEntp, nodep->name());
                        //
                        ok = સાચું;
                        m_ds.m_dotSymp = foundp;
                        m_ds.m_dotPos = DP_SCOPE;
                        UINFO(9, indent() << " cell -> iface varref " << foundp->nodep());
                        Ast_ગાંઠ_પદાવલિ* newp;
                        જો (m_ds.m_dotText != "") {
                            newp = નવું AstVarXRef{nodep->fileline(), ifaceRefVarp, m_ds.m_dotText,
                                                  VAccess::READ};
                            જો (m_ds.m_unresolvedCell && m_ds.m_unlinkedScopep) {
                                newp = નવું AstUnlinkedRef{nodep->fileline(), newp, newp->name(),
                                                          m_ds.m_unlinkedScopep->unlinkFrBack()};
                                m_ds.m_unlinkedScopep = nullptr;
                                m_ds.m_unresolvedCell = ખોટું;
                                m_ds.m_dotText = "";
                            }
                        } નહીંતર {
                            AstVarRef* અચલ refp
                                = નવું AstVarRef{nodep->fileline(), ifaceRefVarp, VAccess::READ};
                            newp = refp;
                        }
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
                    } નહીંતર જો (VN_IS(cellp->modp(), NotFoundModule)) {
                        cellp->modNameFileline()->v3error("Cannot find file containing interface: "
                                                          << cellp->modp()->prettyNameQ());
                    }
                }
            } નહીંતર જો (allowScope && !allowFTask && VN_IS(foundp->nodep(), ગાંઠ_FTask)) {
                // Function/task used as scope for static variable access (IEEE std 1800-2017 6.21)
                // Don't set m_dotText so the static variable resolves as a VarRef rather than
                // VarXRef. This is so that V3Begin won't move it out of the function/task's scope.
                ok = સાચું;
                m_ds.m_dotSymp = foundp;
                m_ds.m_dotPos = DP_SCOPE;
            } નહીંતર જો (allowFTask && VN_IS(foundp->nodep(), ગાંઠ_FTask)) {
                Ast_ગાંઠ_FTask_Ref* taskrefp;
                જો (VN_IS(foundp->nodep(), Task)) {
                    taskrefp = નવું AstTaskRef{nodep->fileline(), nodep->name()};
                } નહીંતર {
                    taskrefp = નવું AstFuncRef{nodep->fileline(), nodep->name()};
                }
                nodep->replaceWith(taskrefp);
                VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
                m_ds = lastStates;
                પાછા;
            } નહીંતર જો (AstVar* અચલ varp = foundToVarp(foundp, nodep, VAccess::READ)) {
                // Preserve explicit 'this' references to members inherited from
                // generic classes until after specialization.
                જો (m_statep->forPrimary() && m_ds.m_dotp && isDotThisReference(m_ds.m_dotp)
                    && foundp->classOrPackagep() != m_modp
                    && VN_IS(foundp->classOrPackagep(), Class)
                    && foundp->classOrPackagep()->hasGParam()) {
                    m_ds.m_unresolvedClass = સાચું;
                    પાછા;
                }
                AstIfaceRefDType* અચલ ifacerefp
                    = LinkDotState::ifaceRefFromArray(varp->subDTypep());
                જો (varp->isIfaceRef() && m_genericIfaceModule
                    && VN_IS(varp->childDTypep(), IfaceGenericDType) && !start) {
                    // Defer only dotted member access ('d.PARAM'), as V3Param must specialize
                    // first; a standalone ref ('.x(d)') resolves via allowVar below now
                    ok = સાચું;
                    m_ds.m_unresolvedGenericIface = સાચું;
                } નહીંતર જો (ifacerefp && varp->isIfaceRef()) {
                    UASSERT_OBJ(ifacerefp->ifaceViaCellp(), ifacerefp, "Unlinked interface");
                    // Really this is a scope reference into an interface
                    UINFO(9, indent() << "varref-ifaceref " << m_ds.m_dotText << "  " << nodep);
                    m_ds.m_dotText = VString::dot(m_ds.m_dotText, ".", nodep->name());
                    // If modport specified, use modport symbol table for lookup of virtual ports
                    જો (ifacerefp->modportp() && m_statep->exists_ગાંઠ_Sym(ifacerefp->modportp())) {
                        m_ds.m_dotSymp = m_statep->get_ગાંઠ_Sym(ifacerefp->modportp());
                    } નહીંતર {
                        m_ds.m_dotSymp = m_statep->get_ગાંઠ_Sym(ifacerefp->ifaceViaCellp());
                    }
                    m_ds.m_dotPos = DP_SCOPE;
                    ok = સાચું;

                    AstVarRef* અચલ refp = નવું AstVarRef{nodep->fileline(), varp, VAccess::READ};
                    nodep->replaceWith(refp);

                    VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
                } નહીંતર જો (allowVar) {
                    Ast_ગાંઠ* newp;
                    જો (m_ds.m_dotText != "") {
                        AstVarXRef* અચલ refp
                            = નવું AstVarXRef{nodep->fileline(), nodep->name(), m_ds.m_dotText,
                                             VAccess::READ};  // lvalue'ness computed later
                        refp->varp(varp);
                        // Not linked again after V3LinkLValue, so that reports writing it
                        જો (અચલ AstModportVarRef* અચલ mvarp
                            = VN_CAST(foundp->nodep(), ModportVarRef)) {
                            જો (m_statep->forParamed() && mvarp->direction().isReadOnly()) {
                                refp->readOnlyModport(સાચું);
                            }
                        }
                        refp->containsGenBlock(m_ds.m_genBlk);
                        m_ds.m_dotText = "";
                        જો (m_ds.m_unresolvedCell && m_ds.m_unlinkedScopep) {
                            અચલ string dotted = refp->dotted();
                            અચલ size_t pos = dotted.find("__BRA__??__KET__");
                            // Arrays of interfaces all have the same parameters
                            જો (pos != string::npos && varp->isParam()
                                && VN_IS(m_ds.m_unlinkedScopep, CellArrayRef)) {
                                refp->dotted(dotted.substr(0, pos));
                                newp = refp;
                            } નહીંતર {
                                UINFO(9, indent() << "deferring until post-V3Param: " << refp);
                                newp = નવું AstUnlinkedRef{nodep->fileline(), refp, refp->name(),
                                                          m_ds.m_unlinkedScopep->unlinkFrBack()};
                                m_ds.m_unlinkedScopep = nullptr;
                                m_ds.m_unresolvedCell = ખોટું;
                            }
                        } નહીંતર {
                            newp = refp;
                        }
                    } નહીંતર {
                        જો (staticAccess && !varp->lifetime().isStatic() && !varp->isParam()) {
                            // TODO bug4077
                            // nodep->v3error("Static access to non-static member variable "
                            //                << varp->prettyNameQ() << endl);
                        }
                        AstVarRef* અચલ refp = નવું AstVarRef{
                            nodep->fileline(), varp, VAccess::READ};  // lvalue'ness computed later
                        refp->classOrPackagep(foundp->classOrPackagep());
                        newp = refp;
                    }
                    UINFO(9, indent() << "new " << newp);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
                    m_ds.m_dotPos = DP_MEMBER;
                    ok = સાચું;
                }
            } નહીંતર જો (અચલ AstModport* અચલ modportp = VN_CAST(foundp->nodep(), Modport)) {
                // A scope reference into an interface's modport (not
                // necessarily at a pin connection)
                UINFO(9, indent() << "cell-ref-to-modport " << m_ds.m_dotText << "  " << nodep);
                UINFO(9, indent() << "unlinked " << m_ds.m_unlinkedScopep);
                UINFO(9,
                      indent() << "dotSymp " << m_ds.m_dotSymp << " " << m_ds.m_dotSymp->nodep());
                // Iface was the previously dotted component
                જો (!m_ds.m_dotSymp || !VN_IS(m_ds.m_dotSymp->nodep(), Cell)
                    || !VN_AS(m_ds.m_dotSymp->nodep(), Cell)->modp()
                    || !VN_IS(VN_AS(m_ds.m_dotSymp->nodep(), Cell)->modp(), Iface)) {
                    nodep->v3error("Modport not referenced as <interface>."
                                   << modportp->prettyNameQ());
                } નહીંતર જો (!VN_AS(m_ds.m_dotSymp->nodep(), Cell)->modp()
                           || !VN_IS(VN_AS(m_ds.m_dotSymp->nodep(), Cell)->modp(), Iface)) {
                    nodep->v3error("Modport not referenced from underneath an interface: "
                                   << modportp->prettyNameQ());
                } નહીંતર {
                    AstCell* અચલ cellp = VN_AS(m_ds.m_dotSymp->nodep(), Cell);
                    UASSERT_OBJ(cellp, nodep, "Modport not referenced from an instance");
                    VSymEnt* અચલ cellEntp = m_statep->get_ગાંઠ_Sym(cellp);
                    UASSERT_OBJ(cellEntp, nodep, "No interface sym entry");
                    VSymEnt* અચલ parentEntp
                        = cellEntp->parentp();  // Container of the var; probably a
                                                // module or generate begin
                    // We drop __BRA__??__KET__ as cells don't have that naming yet
                    AstVar* અચલ ifaceRefVarp
                        = find_Iface_ટોચ_Varp(nodep, parentEntp, cellp->name());
                    //
                    ok = સાચું;
                    m_ds.m_dotText = VString::dot(m_ds.m_dotText, ".", nodep->name());
                    m_ds.m_dotSymp = foundp;
                    m_ds.m_dotPos = DP_SCOPE;
                    UINFO(9, indent() << "modport -> iface varref " << foundp->nodep());
                    // We lose the modport name here, so we cannot detect mismatched modports.
                    AstVarRef* અચલ refp
                        = નવું AstVarRef{nodep->fileline(), ifaceRefVarp, VAccess::READ};
                    Ast_ગાંઠ_પદાવલિ* newp = refp;

                    auto* અચલ cellarrayrefp = VN_CAST(m_ds.m_unlinkedScopep, CellArrayRef);
                    જો (cellarrayrefp) {
                        // iface[vec].modport became CellArrayRef(iface, lsb)
                        // Convert back to SelBit(iface, lsb)
                        UINFO(9, indent() << "Array modport to SelBit " << cellarrayrefp);
                        newp = નવું AstSelBit{cellarrayrefp->fileline(), newp,
                                             cellarrayrefp->selp()->unlinkFrBack()};
                        newp->user3(સાચું);  // Don't process again
                        VL_DO_DANGLING(ધકેલો_Deletep(cellarrayrefp->unlinkFrBack()), cellarrayrefp);
                        m_ds.m_unlinkedScopep = nullptr;
                    }
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
                }
            } નહીંતર જો (AstConstraint* અચલ defp = VN_CAST(foundp->nodep(), Constraint)) {
                Ast_ગાંઠ* અચલ newp = નવું AstConstraintRef{nodep->fileline(), nullptr, defp};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
                ok = સાચું;
                m_ds.m_dotPos = DP_MEMBER;
                m_ds.m_dotText = "";
            } નહીંતર જો (AstClass* અચલ defp = VN_CAST(foundp->nodep(), Class)) {
                જો (allowVar) {
                    જો (m_ds.m_dotPos == DP_SCOPE && !staticAccess) {
                        // Continue hierarchical lookup in the class scope.
                        m_ds.m_dotSymp = foundp;
                    } નહીંતર {
                        AstRefDType* અચલ newp
                            = નવું AstRefDType{nodep->fileline(), nodep->name()};
                        replaceWithCheckBreak(nodep, newp);
                        VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
                    }
                    ok = સાચું;
                    m_ds.m_dotText = "";
                } નહીંતર {
                    (શૂન્ય)defp;  // Prevent unused variable warning
                }
            } નહીંતર જો (AstMemberDType* અચલ defp = VN_CAST(foundp->nodep(), MemberDType)) {
                જો (allowVar) {
                    checkMemberDeclOrder(nodep, defp);
                    AstRefDType* અચલ refp = નવું AstRefDType{nodep->fileline(), nodep->name()};
                    refp->refDTypep(defp);
                    replaceWithCheckBreak(nodep, refp);
                    VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
                    ok = સાચું;
                    m_ds.m_dotText = "";
                }
            } નહીંતર જો (AstEnumItem* અચલ valuep = VN_CAST(foundp->nodep(), EnumItem)) {
                જો (allowVar) {
                    AstEnumItemRef* અચલ refp
                        = નવું AstEnumItemRef{nodep->fileline(), valuep, foundp->classOrPackagep()};
                    Ast_ગાંઠ* newp = refp;
                    // Hierarchical reference, relinked after V3Param as the referenced
                    // module may be specialized, similar to AstVarXRef
                    refp->dotted(m_ds.m_dotText);
                    refp->containsGenBlock(m_ds.m_genBlk);
                    જો (m_ds.m_unresolvedCell && m_ds.m_unlinkedScopep) {
                        UINFO(9, indent() << "deferring until post-V3Param: " << refp);
                        newp = નવું AstUnlinkedRef{nodep->fileline(), refp, refp->name(),
                                                  m_ds.m_unlinkedScopep->unlinkFrBack()};
                        m_ds.m_unlinkedScopep = nullptr;
                        m_ds.m_unresolvedCell = ખોટું;
                    }
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
                    ok = સાચું;
                    m_ds.m_dotText = "";
                }
            } નહીંતર જો (અચલ AstLambdaArgRef* અચલ argrefp
                       = VN_CAST(foundp->nodep(), LambdaArgRef)) {
                જો (allowVar) {
                    Ast_ગાંઠ* અચલ newp
                        = નવું AstLambdaArgRef{nodep->fileline(), argrefp->name(), ખોટું};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
                    ok = સાચું;
                    m_ds.m_dotPos = DP_MEMBER;
                    m_ds.m_dotText = "";
                }
            } નહીંતર જો (VN_IS(foundp->nodep(), Clocking)) {
                m_ds.m_dotSymp = foundp;
                જો (m_ds.m_dotText != "") m_ds.m_dotText += "." + nodep->name();
                ok = m_ds.m_dotPos == DP_SCOPE || m_ds.m_dotPos == DP_FIRST;
            } નહીંતર જો (અચલ AstModportClockingRef* અચલ clockingRefp
                       = VN_CAST(foundp->nodep(), ModportClockingRef)) {
                // Clocking block accessed through a modport - redirect to actual clocking
                m_ds.m_dotSymp = m_statep->get_ગાંઠ_Sym(clockingRefp->clockingp());
                જો (m_ds.m_dotText != "") m_ds.m_dotText += "." + nodep->name();
                ok = m_ds.m_dotPos == DP_SCOPE || m_ds.m_dotPos == DP_FIRST;
            } નહીંતર જો (અચલ Ast_ગાંઠ_FTask* અચલ ftaskp = VN_CAST(foundp->nodep(), ગાંઠ_FTask)) {
                જો (!ftaskp->isFunction() || ftaskp->classMethod()) {
                    ok = m_ds.m_dotPos == DP_NONE;
                    જો (ok) {
                        // The condition is true for tasks,
                        // properties and void functions.
                        // In these cases, the parentheses may be skipped.
                        // Also SV class methods can be called without parens
                        AstFuncRef* અચલ funcRefp
                            = નવું AstFuncRef{nodep->fileline(), nodep->name()};
                        nodep->replaceWith(funcRefp);
                        VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
                    }
                }
            } નહીંતર જો (AstTypedef* અચલ defp = VN_CAST(foundp->nodep(), Typedef)) {
                અચલ તાર્કિક ifaceFinalSegmentAllowed
                    = (m_ds.m_dotPos == DP_FINAL) && m_ds.m_dotSymp
                      && (VN_IS(m_ds.m_dotSymp->nodep(), Modport)
                          || (VN_IS(m_ds.m_dotSymp->nodep(), Cell)
                              && VN_CAST(m_ds.m_dotSymp->nodep(), Cell)->modp()
                              && VN_IS(VN_CAST(m_ds.m_dotSymp->nodep(), Cell)->modp(), Iface)));
                // Allow interface typedef references even without IfaceCapture
                // The dependency graph handles resolution when IfaceCapture is disabled
                ok = (m_ds.m_dotPos == DP_NONE || m_ds.m_dotPos == DP_SCOPE
                      || ifaceFinalSegmentAllowed);
                જો (ifaceFinalSegmentAllowed) {
                    UINFO(9, indent() << "allow final-segment typedef name="
                                      << nodep->prettyNameQ() << " dotText='" << m_ds.m_dotText
                                      << "' dotSym=" << m_ds.m_dotSymp);
                }
                જો (ok) {
                    AstRefDType* અચલ refp = નવું AstRefDType{nodep->fileline(), nodep->name()};
                    // Don't check if typedef is to a <type T>::<reference> as might not be
                    // resolved yet
                    જો (m_ds.m_dotPos == DP_NONE) checkDeclOrder(nodep, defp);
                    refp->typedefp(defp);

                    V3LinkDotIfaceCapture::captureTypedefContext(
                        refp, "typedef", static_cast<પૂર્ણાંક>(m_ds.m_dotPos), m_ds.m_dotText,
                        m_ds.m_dotSymp, m_modp, [આ]() { પાછા indent(); });

                    જો (VN_IS(nodep->backp(), SelExtract)) {
                        m_packedArrayDtp = refp;
                    } નહીંતર {
                        replaceWithCheckBreak(nodep, refp);
                        VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
                    }
                }
            } નહીંતર જો (AstParamTypeDType* અચલ defp = VN_CAST(foundp->nodep(), ParamTypeDType)) {
                ok = (m_ds.m_dotPos == DP_NONE || m_ds.m_dotPos == DP_SCOPE
                      || m_ds.m_dotPos == DP_FINAL);
                જો (ok) {
                    AstRefDType* અચલ refp = નવું AstRefDType{nodep->fileline(), nodep->name()};
                    refp->refDTypep(defp);

                    V3LinkDotIfaceCapture::captureTypedefContext(
                        refp, "paramtype", static_cast<પૂર્ણાંક>(m_ds.m_dotPos), m_ds.m_dotText,
                        m_ds.m_dotSymp, m_modp, [આ]() { પાછા indent(); });

                    જો (VN_IS(nodep->backp(), SelExtract)) {
                        m_packedArrayDtp = refp;
                    } નહીંતર {
                        replaceWithCheckBreak(nodep, refp);
                        VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
                    }
                }
            }
            જો (!ok) {
                જો (m_insideClassExtParam) {
                    // Don't throw error if the reference is inside a class that extends a param,
                    // because some members can't be linked in such a case. m_insideClassExtParam
                    // may be true only in the first stage of linking.
                    // Mark that the Dot statement can't be resolved.
                    m_ds.m_unresolvedClass = સાચું;
                    // If the symbol was a scope name, it would be resolved.
                    m_ds.m_dotPos = DP_MEMBER;
                } નહીંતર {
                    // Cells/interfaces can't be implicit
                    અચલ તાર્કિક checkImplicit
                        = (!m_ds.m_dotp && m_ds.m_dotText == "" && !m_ds.m_disablep && !foundp);
                    અચલ તાર્કિક err
                        = !(checkImplicit && m_statep->implicitOk(m_modp, nodep->name()));
                    જો (err) {
                        જો (foundp) {
                            nodep->v3error("Found definition of '"
                                           << m_ds.m_dotText << (m_ds.m_dotText == "" ? "" : ".")
                                           << nodep->prettyName() << "'" << " as a "
                                           << foundp->nodep()->typeName() << " but expected a "
                                           << expectWhat);
                        } નહીંતર જો (m_ds.m_dotText == "") {
                            UINFO(1, "   ErrParseRef curSymp=se" << cvtToHex(m_curSymp)
                                                                 << " ds=" << m_ds.ascii());
                            અચલ string suggest = m_statep->suggestSymFallback(
                                m_ds.m_dotSymp, nodep->name(), VNodeMatcher{});
                            nodep->v3error(
                                "Can't find definition of "
                                << expectWhat << ": " << nodep->prettyNameQ() << '\n'
                                << (suggest.empty() ? "" : nodep->warnMore() + suggest));
                        } નહીંતર {
                            nodep->v3error(
                                "Can't find definition of "
                                << (!baddot.empty() ? Ast_ગાંઠ::prettyNameQ(baddot)
                                                    : nodep->prettyNameQ())
                                << " in dotted " << expectWhat << ": '"
                                << m_ds.m_dotText + "." + nodep->prettyName() << "'\n"
                                << nodep->warnContextPrimary()
                                << (okSymp ? okSymp->cellErrorScopes(
                                                 nodep, Ast_ગાંઠ::prettyName(m_ds.m_dotText))
                                           : ""));
                        }
                        m_ds.m_dotErr = સાચું;
                    }
                    જો (checkImplicit) {
                        // Create if implicit, and also if error (so only complain once)
                        // Else if a scope is allowed, making a signal won't help error cascade
                        AstVar* અચલ varp
                            = createImplicitVar(m_curSymp, nodep, m_modp, m_modSymp, err);
                        AstVarRef* અચલ newp
                            = નવું AstVarRef{nodep->fileline(), varp, VAccess::READ};
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
                    }
                }
            }
        }
        જો (start) m_ds = lastStates;
    }
    શૂન્ય visit(AstClassOrPackageRef* nodep) override {
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

            જો (!nodep->classOrPackageSkipp() && !nodep->classOrPackageNodep()
                && nodep->name() != "local::") {
                અચલ તાર્કિક deferIfUnresolved = m_statep->forPrimary() && m_insideClassExtParam;
                m_statep->resolveClassOrPackage(m_ds.m_dotSymp, nodep, m_ds.m_dotPos != DP_PACKAGE,
                                                ખોટું, ":: reference", deferIfUnresolved);
            }

            // ClassRef's have pins, so track
            જો (nodep->classOrPackageSkipp()) {
                m_pinSymp = m_statep->get_ગાંઠ_Sym(nodep->classOrPackageSkipp());
            } નહીંતર જો (nodep->name() != "local::") {
                પાછા;
            }

            AstClass* અચલ refClassp = VN_CAST(nodep->classOrPackageSkipp(), Class);
            // Make sure any extends() are properly imported within referenced class
            જો (refClassp && !m_statep->forPrimary()) classExtendImport(refClassp);

            જો (m_statep->forPrimary() && refClassp && !nodep->paramsp()
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
                AstPin* અચલ pinp = નવું AstPin{nodep->fileline(), 1, "__paramNumber1", nullptr};
                pinp->param(સાચું);
                nodep->addParamsp(pinp);
            }

            m_ds.init(m_curSymp);
            UINFO(4, indent() << "(Backto) Link ClassOrPackageRef: " << nodep);
            iterateChildren(nodep);
        }

        જો (nodep->name() == "local::") {
            જો (!m_randSymp && !m_randMethodCallp) {
                nodep->v3error("Illegal 'local::' outside 'randomize() with'"
                               " (IEEE 1800-2023 18.7.1)");
                m_ds.m_dotErr = સાચું;
                પાછા;
            }
        }
        જો (m_ds.m_dotPos == DP_PACKAGE && nodep->classOrPackageSkipp()) {
            m_ds.m_dotSymp = m_statep->get_ગાંઠ_Sym(nodep->classOrPackageSkipp());
            UINFO(9, indent() << "set sym " << m_ds.ascii());
        }
    }
    શૂન્ય visit(AstConstraint* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        જો (nodep->isExternDef()) {
            જો (અચલ VSymEnt* અચલ foundp
                = m_curSymp->findIdFallback("extern " + nodep->name())) {
                અચલ AstConstraint* અચલ protop = VN_AS(foundp->nodep(), Constraint);
                // Copy specifiers.
                // External definition cannot have any specifiers, so no value will be overwritten.
                nodep->isStatic(protop->isStatic());
            } નહીંતર {
                nodep->v3error("extern not found that declares " + nodep->prettyNameQ());
            }
        }
        જો (nodep->isExternProto()) {
            જો (!m_curSymp->findIdFallback(nodep->name()) && nodep->isExternExplicit()) {
                nodep->v3warn(PROTOTYPEMIS, "Definition not found for extern prototype "
                                                << nodep->prettyNameQ());
            }
        }
        VL_RESTORER(m_curSymp);
        VL_RESTORER_COPY(m_ds);
        m_ds.m_dotSymp = m_curSymp = m_statep->get_ગાંઠ_Sym(nodep);
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstConstraintRef* nodep) override {
        જો (nodep->user3SetOnce()) પાછા;
        LINKDOT_VISIT_START();
        UINFO(8, indent() << "visit " << nodep);
        UINFO(8, indent() << m_ds.ascii());
        // No children defined
    }
    શૂન્ય visit(AstVarRef* nodep) override {
        // VarRef: Resolve its reference
        // ParseRefs are used the first pass (forPrimary) so we shouldn't get can't find
        // errors here now that we have a VarRef.
        // No checkNoDot; created and iterated from a parseRef
        LINKDOT_VISIT_START();
        iterateChildren(nodep);
        જો (!nodep->varp()) {
            UINFO(9, indent() << "linkVarRef se" << cvtToHex(m_curSymp) << "  n=" << nodep);
            UASSERT_OBJ(m_curSymp, nodep, "nullptr lookup symbol table");
            VSymEnt* અચલ foundp = m_curSymp->findIdFallback(nodep->name());
            જો (AstVar* અચલ varp
                = foundp ? foundToVarp(foundp, nodep, nodep->access()) : nullptr) {
                nodep->varp(varp);
                updateVarUse(nodep->varp());
                // Generally set by parse, but might be an import
                nodep->classOrPackagep(foundp->classOrPackagep());
            }
            જો (VL_UNCOVERABLE(!nodep->varp())) {
                nodep->v3error("Can't find definition of signal, again: "  // LCOV_EXCL_LINE
                               << nodep->prettyNameQ());
            }
        }
        AstVarScope* vscp = nodep->varScopep();
        જો (vscp && vscp->user2p() != vscp && m_replaceWithAlias) {
            vscp = LinkDotScopeVisitor::getAlias(vscp);
            nodep->varp(vscp->varp());
            nodep->varScopep(vscp);
            updateVarUse(nodep->varp());
            UINFO(7, indent() << "Resolved " << nodep);  // Also prints taskp
        }
    }
    શૂન્ય visit(AstVarXRef* nodep) override {
        // VarRef: Resolve its reference
        // We always link even if varp() is set, because the module we choose may change
        // due to creating new modules, flattening, etc.
        જો (nodep->user3SetOnce()) પાછા;
        LINKDOT_VISIT_START();
        UINFO(8, indent() << "visit " << nodep);
        // No checkNoDot; created and iterated from a parseRef
        જો (!m_modSymp) {
            // Module that is not in hierarchy.  We'll be dead code eliminating it later.
            UINFO(9, "Dead module for " << nodep);
            nodep->varp(nullptr);
        } નહીંતર {
            VSymEnt* lookupSymp = m_curSymp;  // Start search at current scope
            જો (nodep->inlinedDots() != "") {  // Correct for current post-inlined scope
                lookupSymp = findInlinedDotsSym(nodep, nodep->inlinedDots());
            }
            string baddot;
            VSymEnt* okSymp;
            તાર્કિક modport;
            VSymEnt* અચલ dotSymp
                = findDottedRefSymp(nodep, lookupSymp, nodep->dotted(), baddot, okSymp, &modport);
            જો (!dotSymp) પાછા;

            જો (!m_statep->forScopeCreation()) {
                VSymEnt* foundp = nullptr;
                જો (modport) {
                    // This is a copy of findSymPrefixed with few modifications.
                    // This searches without a fallback like findSymPrefixed but if lookup fails it
                    // will check one fallback (fallback of modport shall be interface) and take
                    // only L and G PARAMs since they should be visible from a modport always.
                    string prefix = dotSymp->symPrefix();
                    જ્યારે (!foundp) {
                        foundp = dotSymp->findIdFlat(prefix + nodep->name());
                        જો (foundp) તોડો;
                        UASSERT_OBJ(dotSymp->fallbackp(), nodep, "Modports shall have fallback");
                        foundp = dotSymp->fallbackp()->findIdFlat(prefix + nodep->name());
                        જો (અચલ AstVar* અચલ varp
                            = foundToVarp(foundp, nodep, nodep->access())) {
                            જો (!varp->isParam()) foundp = nullptr;
                        }
                        જો (prefix.empty()) તોડો;
                        string nextPrefix = LinkDotState::removeLastInlineScope(prefix);
                        જો (prefix == nextPrefix) તોડો;
                        prefix = std::move(nextPrefix);
                    }
                    baddot = nodep->name();
                } નહીંતર {
                    foundp = m_statep->findSymPrefixed(dotSymp, nodep->name(), baddot, સાચું);
                }
                જો (m_inSens && foundp) {
                    જો (AstClocking* અચલ clockingp = sensClockingp(foundp->nodep())) {
                        foundp = getCreateClockingEventSymEnt(clockingp);
                    }
                }
                AstVar* અચલ varp
                    = foundp ? foundToVarp(foundp, nodep, nodep->access()) : nullptr;
                nodep->varp(varp);
                updateVarUse(nodep->varp());
                UINFO(7, indent() << "Resolved " << nodep);  // Also prints varp
                // If found, check if it's ok to access in case it's in a hier_block
                જો (nodep->varp() && errorHierNonPort(nodep, nodep->varp(), dotSymp)) પાછા;
                જો (!nodep->varp()) {
                    nodep->v3error("Can't find definition of "
                                   << Ast_ગાંઠ::prettyNameQ(baddot) << " in dotted signal: '"
                                   << nodep->dotted() + "." + nodep->prettyName() << "'\n"
                                   << nodep->warnContextPrimary()
                                   << okSymp->cellErrorScopes(nodep));
                    પાછા;
                }
            } નહીંતર {
                VSymEnt* અચલ foundp
                    = m_statep->findSymPrefixed(dotSymp, nodep->name(), baddot, સાચું);
                AstVarScope* vscp = foundp ? VN_CAST(foundp->nodep(), VarScope) : nullptr;
                // Handle modport expression virtual ports
                જો (!vscp) vscp = findVirtualPortVarScope(dotSymp, nodep->name());
                // If found, check if it's ok to access in case it's in a hier_block
                જો (vscp && errorHierNonPort(nodep, vscp->varp(), dotSymp)) પાછા;
                જો (!vscp) {
                    nodep->v3error("Can't find varpin scope of "
                                   << Ast_ગાંઠ::prettyNameQ(baddot) << " in dotted signal: '"
                                   << nodep->dotted() + "." + nodep->prettyName() << "'\n"
                                   << nodep->warnContextPrimary()
                                   << okSymp->cellErrorScopes(nodep));
                } નહીંતર {
                    જો (vscp->user2p() && m_replaceWithAlias) {
                        vscp = LinkDotScopeVisitor::getAlias(vscp);
                    }
                    // Convert the VarXRef to a VarRef, so we don't need
                    // later optimizations to deal with VarXRef.
                    nodep->varp(vscp->varp());
                    nodep->varScopep(vscp);
                    updateVarUse(nodep->varp());
                    UINFO(7, indent() << "Resolved " << nodep);  // Also prints taskp
                    AstVarRef* અચલ newvscp
                        = નવું AstVarRef{nodep->fileline(), vscp, nodep->access()};
                    nodep->replaceWith(newvscp);
                    VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
                    UINFO(9, indent() << "new " << newvscp);  // Also prints taskp
                }
            }
        }
    }
    શૂન્ય visit(AstEnumDType* nodep) override {
        LINKDOT_VISIT_START();
        iterateChildren(nodep);
        AstRefDType* અચલ refdtypep = VN_CAST(nodep->subDTypep(), RefDType);
        જો (refdtypep && (nodep == refdtypep->subDTypep())) {
            refdtypep->v3error("Self-referential enumerated type definition");
        }
    }
    શૂન્ય visit(Ast_ગાંઠ_UOr_Struct_DType* nodep) override {
        LINKDOT_VISIT_START();
        symIterateChildren(nodep, m_statep->get_ગાંઠ_Sym(nodep));
    }
    શૂન્ય visit(AstEnumItemRef* nodep) override {
        // Resolve its reference
        // EnumItemRefs are created by the first pass, but V3Param may regenerate due to
        // a parameterized class/module, so we shouldn't get can't find errors.
        // No checkNoDot; created and iterated from a parseRef
        જો (nodep->user3SetOnce()) પાછા;
        LINKDOT_VISIT_START();
        જો (m_statep->forParamed() && !nodep->dotted().empty() && m_modSymp) {
            // Hierarchical reference, relink as V3Param may have specialized the module
            string baddot;
            VSymEnt* okSymp = nullptr;
            VSymEnt* અચલ dotSymp
                = findDottedRefSymp(nodep, m_curSymp, nodep->dotted(), baddot, okSymp);
            જો (!dotSymp) પાછા;
            VSymEnt* અચલ foundp
                = m_statep->findSymPrefixed(dotSymp, nodep->name(), baddot, સાચું);
            AstEnumItem* અચલ itemp = foundp ? VN_CAST(foundp->nodep(), EnumItem) : nullptr;
            જો (!itemp) {
                nodep->v3error("Enum item " << nodep->prettyNameQ() << " not found in "
                                            << Ast_ગાંઠ::prettyNameQ(nodep->dotted()) << '\n'
                                            << nodep->warnContextPrimary());
                પાછા;
            }
            nodep->itemp(itemp);
            UINFO(9, indent() << " relinked " << nodep);
        }
        જો (!nodep->itemp()) {
            UINFO(9, indent() << "linkEnumRef se" << cvtToHex(m_curSymp) << "  n=" << nodep);
            UASSERT_OBJ(m_curSymp, nodep, "nullptr lookup symbol table");
            VSymEnt* અચલ foundp = m_curSymp->findIdFallback(nodep->name());
            જો (AstEnumItem* અચલ itemp = foundp ? VN_CAST(foundp->nodep(), EnumItem) : nullptr) {
                nodep->itemp(itemp);
                nodep->classOrPackagep(foundp->classOrPackagep());
                UINFO(9, indent() << " resolved " << nodep);
            }
            જો (VL_UNCOVERABLE(!nodep->itemp())) {
                nodep->v3error("Can't find definition of enum item, again: "  // LCOV_EXCL_LINE
                               << nodep->prettyNameQ());
            }
        }
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstMethodCall* nodep) override {
        // Created here so should already be resolved.
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        VL_RESTORER_COPY(m_ds);
        VL_RESTORER(m_randSymp);
        VL_RESTORER(m_randMethodCallp);
        {
            m_ds.init(m_curSymp);
            જો (nodep->name() == "randomize" && (nodep->argsp() || nodep->withp())) {
                m_randMethodCallp = nodep;
                અચલ Ast_ગાંઠ_DType* fromDtp = nodep->fromp()->dtypep();
                જો (!fromDtp) {
                    જો (અચલ Ast_ગાંઠ_Var_Ref* અચલ varRefp = VN_CAST(nodep->fromp(), ગાંઠ_Var_Ref)) {
                        fromDtp = varRefp->varp()->subDTypep();
                    } નહીંતર {
                        fromDtp = get_પદાવલિ_DTypep(nodep->fromp());
                    }
                    જો (!fromDtp) {
                        જો (nodep->withp()) {
                            nodep->v3warn(
                                E_UNSUPPORTED,
                                "Unsupported: 'randomize() with' on complex expressions");
                        } નહીંતર {
                            nodep->v3warn(E_UNSUPPORTED,
                                          "Unsupported: Inline random variable control with "
                                          "'randomize()' called on complex expressions");
                        }
                    }
                }
                જો (m_statep->forPrimary() && isParamedClassRefDType(fromDtp)) {
                    m_ds.m_unresolvedClass = સાચું;
                } નહીંતર જો (fromDtp) {
                    અચલ AstClassRefDType* અચલ classDtp
                        = VN_CAST(fromDtp->skipRefp(), ClassRefDType);
                    જો (!classDtp)
                        nodep->v3error("'randomize() with' on a non-class-instance "
                                       << fromDtp->prettyNameQ());
                    નહીંતર જો (m_statep->forPrimary() && m_extendsParam.ગણતરી(classDtp->classp())) {
                        // Inherited members are unavailable until the base class is specialized.
                        // Defer lookup rather than incorrectly binding names in the caller.
                        m_ds.m_unresolvedClass = સાચું;
                    } નહીંતર {
                        m_randSymp = m_statep->get_ગાંઠ_Sym(classDtp->classp());
                    }
                }
            }
            iterateChildren(nodep);
        }
    }
    શૂન્ય visit(AstVar* nodep) override {
        LINKDOT_VISIT_START();
        checkNoDot(nodep);
        VL_RESTORER(m_isParam);
        m_isParam = nodep->varType().isParam();
        iterateChildren(nodep);
        જો (m_statep->forPrimary() && nodep->isIO() && !m_ftaskp && !nodep->user4()) {
            nodep->v3error(
                "Input/output/inout does not appear in port list: " << nodep->prettyNameQ());
        }
    }
    શૂન્ય visit(AstVarScope* nodep) override {
        LINKDOT_VISIT_START();
        checkNoDot(nodep);
        iterateChildren(nodep);
        AstVarScope* aliasp = LinkDotScopeVisitor::getAlias(nodep);
        જો (aliasp && aliasp != nodep && !nodep->varp()->isIfaceRef()) {
            // Aliased variable might still be references from outside,
            // eg through the VPI, and is traced, so we need the value to propagate.
            // TODO: this means external writes to the LHS (e.g.: through the VPI) don't work
            AstAssignW* અચલ assignp = નવું AstAssignW{
                nodep->fileline(), નવું AstVarRef{nodep->fileline(), nodep, VAccess::WRITE},
                નવું AstVarRef{nodep->fileline(), aliasp, VAccess::READ}};
            assignp->user2(સાચું);
            nodep->scopep()->addBlocksp(નવું AstAlways{assignp});
            // Propagate attributes and lint state of the replaced variable,
            // because all references to it are replaced with references to the alias variable
            aliasp->varp()->propagateAttrFrom(nodep->varp());
            aliasp->varp()->fileline()->modifyStateInherit(nodep->varp()->fileline());
        }
    }
    શૂન્ય visit(Ast_ગાંઠ_FTask_Ref* nodep) override {
        જો (nodep->user3SetOnce()) પાછા;
        LINKDOT_VISIT_START();
        UINFO(8, indent() << "visit " << nodep);
        UINFO(8, indent() << m_ds.ascii());
        જો (m_ds.m_dotPos != DP_MEMBER || nodep->name() != "randomize") {
            // Visit arguments at the beginning.
            // They may be visitted even if the current node can't be linked now.
            symIterateChildren(nodep, m_curSymp);
        }

        જો (m_ds.m_super) {
            જો (AstFuncRef* અચલ funcRefp = VN_CAST(nodep, FuncRef)) {
                funcRefp->superReference(સાચું);
            } નહીંતર જો (AstTaskRef* અચલ taskRefp = VN_CAST(nodep, TaskRef)) {
                taskRefp->superReference(સાચું);
            }
        }

        VL_RESTORER(m_randSymp);

        તાર્કિક first = !m_ds.m_dotp || m_ds.m_dotPos == DP_FIRST;
        તાર્કિક staticAccess = ખોટું;
        જો (m_ds.m_unresolvedClass) {
            // Unable to link before V3Param
            પાછા;
        } નહીંતર જો (m_ds.m_unresolvedCell && m_ds.m_dotPos == DP_FINAL && m_ds.m_unlinkedScopep) {
            Ast_ગાંઠ_FTask_Ref* અચલ newftaskp = nodep->clone_વૃક્ષ(ખોટું);
            newftaskp->dotted(m_ds.m_dotText);
            Ast_ગાંઠ* અચલ newp = નવું AstUnlinkedRef{nodep->fileline(), newftaskp, nodep->name(),
                                                     m_ds.m_unlinkedScopep->unlinkFrBack()};
            m_ds.m_unlinkedScopep = nullptr;
            m_ds.m_unresolvedCell = ખોટું;
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
            પાછા;
        } નહીંતર જો (m_ds.m_dotp && m_ds.m_dotPos == DP_PACKAGE) {
            UASSERT_OBJ(VN_IS(m_ds.m_dotp->lhsp(), ClassOrPackageRef), m_ds.m_dotp->lhsp(),
                        "Bad package link");
            staticAccess = સાચું;
            AstClassOrPackageRef* અચલ cpackagerefp
                = VN_AS(m_ds.m_dotp->lhsp(), ClassOrPackageRef);
            જો (cpackagerefp->name() == "local::") {
                m_randSymp = nullptr;
                first = સાચું;
            } નહીંતર જો (!cpackagerefp->classOrPackageSkipp()) {
                VSymEnt* અચલ foundp = m_statep->resolveClassOrPackage(
                    m_ds.m_dotSymp, cpackagerefp, સાચું, ખોટું, ":: reference");
                જો (foundp) nodep->classOrPackagep(cpackagerefp->classOrPackageSkipp());
            } નહીંતર {
                nodep->classOrPackagep(cpackagerefp->classOrPackageSkipp());
            }
            // Class/package :: HERE function() . method_called_on_function_return_value()
            m_ds.m_dotPos = DP_MEMBER;
        } નહીંતર જો (m_ds.m_dotp && m_ds.m_dotPos == DP_FINAL) {
            nodep->dotted(m_ds.m_dotText);  // Maybe ""
            // Only flag FTaskRefs under generate-if/case blocks that may be
            // pruned.  GenFor and plain begin-blocks won't be pruned by V3Param.
            // A generate-case block's parent is the GenCaseItem, not the GenCase.
            // VarXRef uses the broader m_genBlk flag (set for all GenBlocks)
            // because genfor unrolling also removes variables.
            જો (m_ds.m_genBlk && m_ds.m_dotSymp) {
                અચલ Ast_ગાંઠ* અચલ blkp = m_ds.m_dotSymp->nodep();
                જો (VN_IS(blkp, GenBlock)
                    && (VN_IS(blkp->backp(), GenIf) || VN_IS(blkp->backp(), GenCaseItem))) {
                    nodep->containsGenBlock(સાચું);
                }
            }
        } નહીંતર જો (m_ds.m_dotp && m_ds.m_dotPos == DP_MEMBER) {
            // Found a Var, everything following is method call.
            // {scope}.{var}.HERE {method} ( ARGS )
            Ast_ગાંઠ_પદાવલિ* અચલ varEtcp = VN_AS(m_ds.m_dotp->lhsp()->unlinkFrBack(), ગાંઠ_પદાવલિ);
            AstArg* અચલ argsp = nodep->argsp();
            જો (argsp) argsp->unlinkFrBackWithNext();
            AstMethodCall* અચલ newp = નવું AstMethodCall{nodep->fileline(), varEtcp,
                                                          VFlagChildDType{}, nodep->name(), argsp};
            જો (AstWith* અચલ withp = nodep->withp()) newp->withp(withp->unlinkFrBack());
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
            પાછા;
        } નહીંતર જો (m_ds.m_dotp && (m_ds.m_dotPos == DP_SCOPE || m_ds.m_dotPos == DP_FIRST)) {
            // HERE function() . method_called_on_function_return_value()
            m_ds.m_dotPos = DP_MEMBER;
            m_ds.m_dotText = "";
        } નહીંતર જો (!m_ds.m_disablep) {
            // visit(AstDisable*) setup the dot handling
            checkNoDot(nodep);
        }
        જો (nodep->classOrPackagep() && nodep->taskp()) {
            // References into packages don't care about cell hierarchy.
        } નહીંતર જો (!m_modSymp) {
            // Module that is not in hierarchy.  We'll be dead code eliminating it later.
            UINFO(9, indent() << "Dead module for " << nodep);
            nodep->taskp(nullptr);
        } નહીંતર જો (nodep->dotted() == "" && nodep->taskp()) {
            // Earlier should have setup the links
            // Might be under a BEGIN we're not processing, so don't relink it
        } નહીંતર {
            string baddot;
            VSymEnt* okSymp = nullptr;
            VSymEnt* dotSymp;
            જો (nodep->dotted().empty()) {
                // Non-'super.' dotted reference
                dotSymp = m_ds.m_dotSymp;
            } નહીંતર {
                // Start search at module, as a variable of same name under a subtask isn't a
                // relevant hit however a function under a begin/end is.  So we want begins, but
                // not the function
                dotSymp = m_curSymp;
            }
            જો (nodep->classOrPackagep()) {  // Look only in specified package
                dotSymp = m_statep->get_ગાંઠ_Sym(nodep->classOrPackagep());
                UINFO(8, indent() << "Override classOrPackage " << dotSymp);
            } નહીંતર {
                જો (nodep->inlinedDots() != "") {  // Correct for current post-inlined scope
                    dotSymp = findInlinedDotsSym(nodep, nodep->inlinedDots());
                }
                dotSymp = m_statep->findDotted(nodep->fileline(), dotSymp, nodep->dotted(), baddot,
                                               okSymp, સાચું);  // Maybe nullptr
            }
            // Restricted 'with (id_list)': only id_list names bind into target class.
            જો (m_randSymp
                && (!m_currentWithp || m_currentWithp->nameResolvesToTarget(nodep->name()))) {
                VSymEnt* અચલ foundp = m_randSymp->findIdFlat(nodep->name());
                જો (foundp && m_currentWithp) {
                    જો (m_currentWithp->restricted()) {
                        m_restrictedNamesUsed.ઉમેરો(nodep->name());
                    }
                    UINFO(9, indent() << "randomize-with fromSym " << foundp->nodep());
                    AstArg* argsp = nullptr;
                    જો (nodep->argsp()) {
                        iterateAndNextNull(nodep->argsp());
                        argsp = nodep->argsp()->unlinkFrBackWithNext();
                    }
                    જો (m_ds.m_dotPos != DP_NONE) m_ds.m_dotPos = DP_MEMBER;
                    Ast_ગાંઠ* અચલ newp = નવું AstMethodCall{
                        nodep->fileline(),
                        નવું AstLambdaArgRef{nodep->fileline(), "__Vrandwith_obj", ખોટું},
                        VFlagChildDType{}, nodep->name(), argsp};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
                    પાછા;
                }
            }
            જો (first && nodep->name() == "randomize" && VN_IS(m_modp, Class)
                && !VN_IS(nodep->classOrPackagep(), Package)) {
                // need special handling to avoid falling back to std::randomize
                // Skip if classOrPackagep is a Package (i.e. std::randomize resolved earlier)
                VMemberMap memberMap;
                AstFunc* અચલ randFuncp = V3Randomize::newRandomizeFunc(
                    memberMap, VN_AS(m_modp, Class), nodep->name(), સાચું, સાચું);
                nodep->taskp(randFuncp);
                nodep->classOrPackagep(VN_AS(m_modp, Class));
                m_curSymp = m_statep->ઉમેરો_Block(m_curSymp, nodep->name(), randFuncp, m_modp);
                // Already linked to the class randomize function; skip findSymPrefixed
                // which may find std::randomize and overwrite classOrPackagep
                પાછા;
            }
            VSymEnt* અચલ foundp
                = m_statep->findSymPrefixed(dotSymp, nodep->name(), baddot, first);
            Ast_ગાંઠ_FTask* અચલ taskp = foundp ? VN_CAST(foundp->nodep(), ગાંઠ_FTask) : nullptr;
            Ast_ગાંઠ_Module* અચલ taskModulep = foundp ? foundp->classOrPackagep() : nullptr;
            // Ignore deferring linking of functions inside params of classes extending
            // parametric classes if referenced function can be linked in the first pass.
            જો (!m_isParam || !taskp
                || (taskp->classMethod() && taskModulep && taskModulep->hasParameterList())) {
                જો (m_insideClassExtParam) {
                    // The reference may point to a method declared in a super class, which is
                    // proved by a parameter. In such a case, it can't be linked at the first
                    // stage. Must not do any linking, because e.g. might find an extends of an
                    // upper class because the current class (under parent) isn't yet importing
                    // it's extended class symbols.
                    પાછા;
                }
                જો (AstClass* અચલ targetClassp = VN_CAST(dotSymp->nodep(), Class)) {
                    જો (m_extendsParam.ગણતરી(targetClassp)) {
                        // Target class has parameterized extends not yet resolved.
                        // Its inherited symbols (e.g. static functions from the base class)
                        // aren't imported yet - defer to linkDotParamed.
                        પાછા;
                    }
                }
            }

            જો (taskp) {
                જો (staticAccess && !taskp->isStatic()) {
                    // TODO bug4077
                    // nodep->v3error("Static access to non-static task/function "
                    //                << taskp->prettyNameQ() << endl);
                }
                nodep->taskp(taskp);
                nodep->classOrPackagep(foundp->classOrPackagep());
                UINFO(7, indent() << "Resolved " << nodep);  // Also prints taskp
            } નહીંતર {
                // Note ParseRef has similar error handling/message output
                UINFO(7, indent() << "   ErrFtask curSymp=se" << cvtToHex(m_curSymp)
                                  << " dotSymp=se" << cvtToHex(dotSymp));
                જો (foundp) {
                    જો (VN_IS(foundp->nodep(), Var) && m_ds.m_dotText == "" && m_ftaskp
                        && m_ftaskp->name() == foundp->nodep()->name()) {
                        // This is a recursive reference to the function itself, not to the var
                        nodep->taskp(m_ftaskp);
                        nodep->classOrPackagep(foundp->classOrPackagep());
                        UINFO(7, indent() << "Resolved recursive " << nodep);  // Also prints taskp
                    } નહીંતર {
                        nodep->v3error("Found definition of '"
                                       << m_ds.m_dotText << (m_ds.m_dotText == "" ? "" : ".")
                                       << nodep->prettyName() << "'" << " as a "
                                       << foundp->nodep()->typeName()
                                       << " but expected a task/function");
                    }
                } નહીંતર જો (VN_IS(nodep, New) && !m_statep->forScopeCreation()) {
                    // Resolved in V3Width
                } નહીંતર જો ((nodep->name() == "pre_randomize" || nodep->name() == "post_randomize")
                           && VN_IS(dotSymp->nodep(), Class)) {
                    AstClass* અચલ classp = VN_AS(dotSymp->nodep(), Class);
                    // Functions that might be defined by user, but if not, they get empty
                    // definitions within referenced class.
                    // TODO perhaps cleaner to add to symbol table up-front with
                    // a lambda to construct them on a reference.
                    AstFunc* અચલ funcp
                        = નવું AstFunc{nodep->fileline(), nodep->name(), nullptr, nullptr};
                    funcp->classMethod(સાચું);
                    funcp->dtypep(funcp->findVoidDType());
                    classp->addMembersp(funcp);
                    m_statep->ઉમેરો_Block(dotSymp, nodep->name(), funcp, classp);
                    nodep->taskp(funcp);
                    nodep->classOrPackagep(classp);
                    UINFO(9, indent() << " Auto-made " << funcp);
                    પાછા;
                } નહીંતર જો (nodep->name() == "randomize" || nodep->name() == "srandom"
                           || nodep->name() == "get_randstate" || nodep->name() == "set_randstate"
                           || nodep->name() == "rand_mode" || nodep->name() == "constraint_mode") {
                    જો (AstClass* અચલ classp = VN_CAST(m_modp, Class)) {
                        // Don't overwrite if already resolved to a package (e.g. std::randomize)
                        જો (!VN_IS(nodep->classOrPackagep(), Package))
                            nodep->classOrPackagep(classp);
                    } નહીંતર જો (nodep->name() == "randomize") {
                        // A std::randomize resolved in V3Width
                        nodep->classOrPackagep(v3Global.rootp()->stdPackagep());
                    } નહીંતર {
                        nodep->v3error("Calling implicit class method "
                                       << nodep->prettyNameQ() << " without being under class");
                        nodep->replaceWith(નવું AstConst{nodep->fileline(), 0});
                        VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
                        પાછા;
                    }
                } નહીંતર જો (nodep->dotted() == "") {
                    જો (nodep->pli()) {
                        જો (v3Global.opt.bboxSys()) {
                            Ast_ગાંઠ* newp;
                            જો (VN_IS(nodep, FuncRef)) {
                                newp = નવું AstConst{nodep->fileline(), AstConst::All0{}};
                            } નહીંતર {
                                Ast_ગાંઠ_પદાવલિ* outp = nullptr;
                                જ્યારે (AstArg* અચલ argp = nodep->argsp()) {
                                    outp = Ast_ગાંઠ::addNext(outp, argp->exprp()->unlinkFrBack());
                                    VL_DO_DANGLING(ધકેલો_Deletep(argp->unlinkFrBack()), argp);
                                }
                                newp = નવું AstSysIgnore{nodep->fileline(), outp};
                                newp->dtypep(nodep->dtypep());
                            }
                            nodep->replaceWith(newp);
                            VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
                            પાછા;
                        } નહીંતર {
                            VSpellCheck speller;
                            V3Parse::candidatePli(&speller);
                            અચલ string suggest = speller.best_Candidate_સંદેશ(nodep->prettyName());
                            nodep->v3error(
                                "Unsupported or unknown PLI call: "
                                << nodep->prettyNameQ() << '\n'
                                << (suggest.empty() ? "" : nodep->warnMore() + suggest));
                        }
                    } નહીંતર {
                        અચલ string suggest = m_statep->suggestSymFallback(
                            dotSymp, nodep->name(), Link_ગાંઠ_Matcher_FTask{});
                        nodep->v3error("Can't find definition of task/function: "
                                       << nodep->prettyNameQ() << '\n'
                                       << (suggest.empty() ? "" : nodep->warnMore() + suggest));
                    }
                } નહીંતર {
                    અચલ string suggest = m_statep->suggestSymFallback(dotSymp, nodep->name(),
                                                                        Link_ગાંઠ_Matcher_FTask{});
                    nodep->v3error("Can't find definition of "
                                   << Ast_ગાંઠ::prettyNameQ(baddot) << " in dotted task/function: '"
                                   << nodep->dotted() + "." + nodep->prettyName() << "'\n"
                                   << (suggest.empty() ? "" : nodep->warnMore() + suggest) << '\n'
                                   << nodep->warnContextPrimary()
                                   << okSymp->cellErrorScopes(nodep));
                }
            }
        }
    }
    શૂન્ય visit(AstSelBit* nodep) override {
        જો (nodep->user3SetOnce()) પાછા;
        LINKDOT_VISIT_START();
        iterateAndNextNull(nodep->fromp());
        જો (m_ds.m_unresolvedClass) {
            UASSERT_OBJ(m_ds.m_dotPos != DP_SCOPE && m_ds.m_dotPos != DP_FIRST, nodep,
                        "Object of unresolved class on scope position in dotted reference");
            // The index does not depend on the class, and V3Param must see its class references
            symIterateNull(nodep->bitp(), m_curSymp);
            symIterateNull(nodep->attrp(), m_curSymp);
            પાછા;
        }
        જો (m_ds.m_dotPos == DP_SCOPE
            || m_ds.m_dotPos
                   == DP_FIRST) {  // Already under dot, so this is {modulepart} DOT {modulepart}
            UINFO(9, indent() << "deferring until after a V3Param pass: " << nodep);
            m_ds.m_dotText += "__BRA__??__KET__";
            m_ds.m_unresolvedCell = સાચું;
            // And pass up m_ds.m_dotText
        }
        // Pass dot state down to only fromp()
        iterateAndNextNull(nodep->fromp());
        symIterateNull(nodep->bitp(), m_curSymp);
        symIterateNull(nodep->attrp(), m_curSymp);
        જો (m_ds.m_unresolvedCell && (m_ds.m_dotPos == DP_SCOPE || m_ds.m_dotPos == DP_FIRST)) {
            Ast_ગાંઠ_પદાવલિ* અચલ exprp = nodep->bitp()->unlinkFrBack();
            જો (AstCellArrayRef* અચલ fromArrRefp = VN_CAST(nodep->fromp(), CellArrayRef)) {
                // Multi-dim iface array access: append this select to the existing chain
                fromArrRefp->addSelp(exprp);
                AstCellArrayRef* અચલ movedp = fromArrRefp->unlinkFrBack();
                nodep->replaceWith(movedp);
                VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
            } નહીંતર {
                AstCellArrayRef* અચલ newp
                    = નવું AstCellArrayRef{nodep->fileline(), nodep->fromp()->name(), exprp};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
            }
        }
    }
    શૂન્ય visit(Ast_ગાંઠ_Pre_Sel* nodep) override {
        // Excludes simple AstSelBit, see above
        જો (nodep->user3SetOnce()) પાછા;
        LINKDOT_VISIT_START();
        જો (m_ds.m_dotPos == DP_SCOPE
            || m_ds.m_dotPos
                   == DP_FIRST) {  // Already under dot, so this is {modulepart} DOT {modulepart}
            nodep->v3error("Syntax error: Range ':', '+:' etc are not allowed in the instance "
                           "part of a dotted reference");
            m_ds.m_dotErr = સાચું;
            પાછા;
        }
        Ast_ગાંઠ_DType* packedArrayDtp = nullptr;  // Datatype reference for packed array
        {
            VL_RESTORER(m_packedArrayDtp);
            iterateAndNextNull(nodep->fromp());
            symIterateNull(nodep->rhsp(), m_curSymp);
            symIterateNull(nodep->thsp(), m_curSymp);

            જો (m_packedArrayDtp) {
                AstRange* અચલ newRangep
                    = નવું AstRange(nodep->fileline(), nodep->rhsp()->unlinkFrBack(),
                                   nodep->thsp()->unlinkFrBack());
                AstPackArrayDType* newArrayTypep
                    = નવું AstPackArrayDType(nodep->fileline(), m_packedArrayDtp, newRangep);
                newArrayTypep->childDTypep(m_packedArrayDtp);
                newArrayTypep->refDTypep(nullptr);
                જો (VN_IS(nodep->backp(), SelExtract)) {
                    packedArrayDtp = newArrayTypep;
                } નહીંતર {
                    replaceWithCheckBreak(nodep, newArrayTypep);
                    VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
                }
            } નહીંતર {
                જો (nodep->attrp()) {
                    Ast_ગાંઠ* અચલ attrp = nodep->attrp()->unlinkFrBack();
                    VL_DO_DANGLING(attrp->delete_વૃક્ષ(), attrp);
                }
                Ast_ગાંઠ* અચલ basefromp = nodep->baseFromp(ખોટું);
                જો (VN_IS(basefromp, Replicate)) {
                    // From {...}[...] syntax in IEEE 2017
                    જો (basefromp) UINFO(9, indent() << " Related node: " << basefromp);
                } નહીંતર {
                    nodep->attrp(નવું AstAttrOf{nodep->fileline(), VAttrType::VAR_BASE,
                                               basefromp->clone_વૃક્ષ(ખોટું)});
                }
            }
        }

        જો (packedArrayDtp) m_packedArrayDtp = packedArrayDtp;
    }
    શૂન્ય visit(AstMemberSel* nodep) override {
        // checkNoDot not appropriate, can be under a dot
        LINKDOT_VISIT_START();
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstGenBlock* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        {
            VL_RESTORER(m_curSymp);
            VL_RESTORER_COPY(m_ds);
            જો (nodep->name() != "") {
                m_ds.m_dotSymp = m_curSymp = m_statep->get_ગાંઠ_Sym(nodep);
                UINFO(5, indent() << "cur=se" << cvtToHex(m_curSymp));
            }
            iterateChildren(nodep);
        }
        UINFO(5, indent() << "cur=se" << cvtToHex(m_curSymp));
    }
    શૂન્ય visit(Ast_ગાંઠ_Block* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        {
            VL_RESTORER(m_curSymp);
            VL_RESTORER_COPY(m_ds);
            જો (nodep->name() != "") {
                m_ds.m_dotSymp = m_curSymp = m_statep->get_ગાંઠ_Sym(nodep);
                UINFO(5, indent() << "cur=se" << cvtToHex(m_curSymp));
            }
            iterateChildren(nodep);
        }
        UINFO(5, indent() << "cur=se" << cvtToHex(m_curSymp));
    }
    શૂન્ય visit(Ast_ગાંઠ_FTask* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        જો (nodep->isExternDef() && !nodep->didProto()) {
            nodep->didProto(સાચું);
            જો (અચલ VSymEnt* અચલ foundp
                = m_curSymp->findIdFallback("extern " + nodep->name())) {
                અચલ Ast_ગાંઠ_FTask* અચલ protop = VN_AS(foundp->nodep(), ગાંઠ_FTask);
                // Copy specifiers.
                // External definition cannot have any specifiers, so no value will be overwritten.
                nodep->isHideLocal(protop->isHideLocal());
                nodep->isHideProtected(protop->isHideProtected());
                nodep->isStatic(protop->isStatic());
                nodep->isVirtual(protop->isVirtual());
                nodep->lifetime(protop->lifetime());
                // Always add an FUNC_RETURN_PROTO even if task, so we can locate the prototype
                nodep->addStmtsp(નવું AstAttrOf{protop->fileline(), VAttrType::FUNC_RETURN_PROTO,
                                               protop->fvarp()
                                                   ? protop->fvarp()->clone_વૃક્ષ(ખોટું)
                                                   : નવું AstVoidDType{protop->fileline()}});
                માટે (Ast_ગાંઠ* stmtp = protop->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    જો (AstVar* અચલ portp = VN_CAST(stmtp, Var)) {
                        AstAttrOf* અચલ attrp
                            = નવું AstAttrOf{protop->fileline(), VAttrType::FUNC_ARG_PROTO,
                                            portp->subDTypep()->clone_વૃક્ષ(ખોટું)};
                        attrp->name(portp->name());
                        nodep->addStmtsp(attrp);
                    }
                }
            } નહીંતર {
                nodep->v3error("extern not found that declares " + nodep->prettyNameQ());
            }
        }
        જો (nodep->isExternProto()) {
            જો (!m_curSymp->findIdFallback(nodep->name())) {
                nodep->v3warn(PROTOTYPEMIS,
                              "Definition not found for extern prototype " + nodep->prettyNameQ());
            }
        }
        VL_RESTORER(m_curSymp);
        VL_RESTORER(m_ftaskp);
        VL_RESTORER(m_explicitSuperNewp);
        {
            m_ftaskp = nodep;
            m_ds.m_dotSymp = m_curSymp = m_statep->get_ગાંઠ_Sym(nodep);
            અચલ તાર્કિક isNew = nodep->name() == "new";
            જો (isNew) m_explicitSuperNewp = nullptr;
            iterateChildren(nodep);
            જો (isNew) {
                અચલ AstClassExtends* અચલ classExtendsp = VN_AS(m_modp, Class)->extendsp();
                જો (m_explicitSuperNewp && !m_explicitSuperNewp->isImplicit() && classExtendsp
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
                જો (classExtendsp && classExtendsp->classOrNullp()) {
                    જો (!m_explicitSuperNewp && m_statep->forParamed()
                        && !VN_AS(classExtendsp->childDTypep()->skipRefp(), ClassRefDType)
                                ->classp()
                                ->isInterfaceClass()) {
                        Ast_ગાંઠ_Stmt* અચલ superNewp
                            = addImplicitSuperNewCall(VN_AS(nodep, Func), classExtendsp);
                        UINFO(9, "created super new " << superNewp);
                        iterate(superNewp);
                    }
                }
            }
        }
        m_ds.m_dotSymp = VL_RESTORER_PREV(m_curSymp);
    }
    શૂન્ય visit(Ast_ગાંઠ_Foreach* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        symIterateChildren(nodep, m_statep->get_ગાંઠ_Sym(nodep));
    }
    શૂન્ય visit(AstRandSequence* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        જો (!nodep->start().empty())
            nodep->prodp(findProd(nodep, m_statep->get_ગાંઠ_Sym(nodep), nodep->start()));
        symIterateChildren(nodep, m_statep->get_ગાંઠ_Sym(nodep));
    }
    શૂન્ય visit(AstRSProd* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        symIterateChildren(nodep, m_statep->get_ગાંઠ_Sym(nodep));
    }
    શૂન્ય visit(AstRSProdItem* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        nodep->prodp(findProd(nodep, m_curSymp, nodep->name()));
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstCoverWith* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        iterateAndNextNull(nodep->subp());
        // The type of 'item' is that of the coverpoint expression, which may name 'item' too
        iterateAndNextNull(nodep->itemp()->childDTypep());
        VL_RESTORER(m_curSymp);
        m_ds.m_dotSymp = m_curSymp = m_statep->get_ગાંઠ_Sym(nodep);
        iterateAndNextNull(nodep->itemp());
        iterateAndNextNull(nodep->filterp());
        m_ds.m_dotSymp = VL_RESTORER_PREV(m_curSymp);
    }
    શૂન્ય visit(AstCoverCrossWith* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        iterateAndNextNull(nodep->selectp());
        iterateAndNextNull(nodep->countp());
        // The type of a cross item is that of its coverpoint's expression, which may name items
        iterateAndNextNull(nodep->varsp());
        VL_RESTORER(m_curSymp);
        m_ds.m_dotSymp = m_curSymp = m_statep->get_ગાંઠ_Sym(nodep);
        iterateAndNextNull(nodep->filterp());
        m_ds.m_dotSymp = VL_RESTORER_PREV(m_curSymp);
    }
    શૂન્ય visit(AstWith* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        જો (m_ds.m_unresolvedClass) પાછા;
        VL_RESTORER(m_curSymp);
        VL_RESTORER(m_currentWithp);
        VL_RESTORER_COPY(m_restrictedNamesUsed);
        {
            m_ds.m_dotSymp = m_curSymp = m_statep->get_ગાંઠ_Sym(nodep);
            m_currentWithp = nodep;
            // Validate the identifier_list once per AstWith (m_validated travels
            // with cloneTree, so cloned witnesses skip the redundant check).
            અચલ તાર્કિક firstVisit = !nodep->validated();
            જો (firstVisit) {
                nodep->validated(સાચું);
                m_restrictedNamesUsed.clear();
                // IEEE 1800-2023 18.7: each name in the identifier_list must designate
                // a member of the randomize() target class.
                જો (nodep->restricted() && m_randSymp) {
                    માટે (અચલ std::string& n : nodep->restrictedNames()) {
                        જો (!m_randSymp->findIdFlat(n)) {
                            nodep->v3error("Identifier '"
                                           << n
                                           << "' in 'with (...)' identifier list is not a"
                                              " member of the randomize() target class"
                                              " (IEEE 1800-2023 18.7)");
                            m_restrictedNamesUsed.ઉમેરો(n);
                        }
                    }
                }
            }
            iterateChildren(nodep);
            // Flag names that the user listed but never referenced in the constraint.
            જો (firstVisit && nodep->restricted()) {
                માટે (અચલ std::string& n : nodep->restrictedNames()) {
                    જો (!m_restrictedNamesUsed.ગણતરી(n)) {
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
    શૂન્ય visit(AstLambdaArgRef* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        // No checknodot(nodep), visit(AstScope) will check for LambdaArgRef
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstClassExtends* nodep) override {
        // Resolve the symbol and get the class.
        // If it is a parameterized case, the class will be resolved after V3Param.cpp
        જો (nodep->user3SetOnce()) પાછા;
        // If the class is resolved, there is nothing more to do
        જો (nodep->classOrNullp()) પાછા;
        LINKDOT_VISIT_START();
        જો (m_statep->forPrimary()) {
            જો (nodep->childDTypep()) પાછા;
            Ast_ગાંઠ* cprp = nodep->classOrPkgsp();
            VSymEnt* lookSymp = m_curSymp;
            જો (AstDot* અચલ dotp = VN_CAST(cprp, Dot)) {
                dotp->user3(સાચું);
                જો (AstClassOrPackageRef* lookNodep = VN_CAST(dotp->lhsp(), ClassOrPackageRef)) {
                    iterate(lookNodep);
                    cprp = dotp->rhsp();
                    VSymEnt* અચલ foundp = m_statep->resolveClassOrPackage(
                        lookSymp, lookNodep, સાચું, ખોટું, nodep->verilogKwd());
                    જો (!foundp) {
                        VL_DO_DANGLING(ધકેલો_Deletep(nodep->unlinkFrBack()), nodep);
                        પાછા;
                    }
                    UASSERT_OBJ(lookNodep->classOrPackageSkipp(), nodep, "Bad package link");
                    lookSymp = m_statep->get_ગાંઠ_Sym(lookNodep->classOrPackageSkipp());
                } નહીંતર {
                    dotp->lhsp()->v3error("Attempting to extend"  // LCOV_EXCL_LINE
                                          " using non-class under dot");
                }
            }
            AstClassOrPackageRef* અચલ cpackagerefp = VN_CAST(cprp, ClassOrPackageRef);
            જો (VL_UNCOVERABLE(!cpackagerefp)) {
                // Linking the extend gives an error before this is hit
                nodep->v3error("Attempting to extend using non-class");  // LCOV_EXCL_LINE
                VL_DO_DANGLING(ધકેલો_Deletep(nodep->unlinkFrBack()), nodep);
                પાછા;
            }
            VSymEnt* અચલ foundp = m_statep->resolveClassOrPackage(lookSymp, cpackagerefp, સાચું,
                                                                    સાચું, nodep->verilogKwd());
            જો (!foundp) {
                VL_DO_DANGLING(ધકેલો_Deletep(nodep->unlinkFrBack()), nodep);
                પાછા;
            }
            જો (AstClass* અચલ classp = VN_CAST(foundp->nodep(), Class)) {
                AstPin* paramsp = cpackagerefp->paramsp();
                જો (paramsp) {
                    paramsp = paramsp->clone_વૃક્ષ(સાચું);
                    nodep->parameterized(સાચું);
                }
                nodep->childDTypep(નવું AstClassRefDType{nodep->fileline(), classp, paramsp});
                // Link pins
                iterate(nodep->childDTypep());
            } નહીંતર જો (AstParamTypeDType* અચલ paramp
                       = VN_CAST(foundp->nodep(), ParamTypeDType)) {
                AstRefDType* અચલ refParamp = નવું AstRefDType{nodep->fileline(), paramp->name()};
                refParamp->refDTypep(paramp);
                nodep->childDTypep(refParamp);
                nodep->parameterized(સાચું);
            } નહીંતર જો (AstTypedef* અચલ typedefp = VN_CAST(foundp->nodep(), Typedef)) {
                Ast_ગાંઠ_DType* અચલ unwrappedp = typedefp->subDTypep()->skipRefp();
                જો (AstClassRefDType* અચલ classRefp = VN_CAST(unwrappedp, ClassRefDType)) {
                    AstPin* paramsp = cpackagerefp->paramsp();
                    જો (!paramsp && classRefp->paramsp()) {
                        // No explicit #(...) on extends clause; carry over the
                        // typedef's type-parameter pins so V3Param sees them.
                        paramsp = classRefp->paramsp()->clone_વૃક્ષ(સાચું);
                        nodep->parameterized(સાચું);
                    } નહીંતર જો (paramsp) {
                        paramsp = paramsp->clone_વૃક્ષ(સાચું);
                        nodep->parameterized(સાચું);
                    }
                    nodep->childDTypep(
                        નવું AstClassRefDType{nodep->fileline(), classRefp->classp(), paramsp});
                    iterate(nodep->childDTypep());
                } નહીંતર {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: " << foundp->nodep()->prettyTypeName()
                                                  << " in 'class extends'");
                    પાછા;
                }
            } નહીંતર {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: " << foundp->nodep()->prettyTypeName()
                                                             << " in 'class extends'");
                પાછા;
            }
            જો (!nodep->childDTypep()) nodep->v3error("Attempting to extend using non-class");
            nodep->classOrPkgsp()->unlinkFrBack()->delete_વૃક્ષ();
        } નહીંતર {
            // Probably a parameter
            જો (AstRefDType* અચલ refp = VN_CAST(nodep->childDTypep(), RefDType)) {
                જો (AstClassRefDType* classRefp = VN_CAST(refp->skipRefp(), ClassRefDType)) {
                    // Resolved to a class reference.
                    refp->replaceWith(classRefp->clone_વૃક્ષ(ખોટું));
                    VL_DO_DANGLING(ધકેલો_Deletep(refp), refp);
                } નહીંતર {
                    // Unable to resolve the ref type to a class reference.
                    // Get the value of type parameter passed to the class instance,
                    // to print the helpful error message.
                    અચલ Ast_ગાંઠ_DType* typep = refp->refDTypep();
                    જ્યારે (સાચું) {
                        જો (અચલ AstParamTypeDType* અચલ atypep
                            = VN_CAST(typep, ParamTypeDType)) {
                            typep = atypep->subDTypep();
                            ચાલુ;
                        }
                        જો (અચલ AstRequireDType* અચલ atypep = VN_CAST(typep, RequireDType)) {
                            typep = atypep->subDTypep();
                            ચાલુ;
                        }
                        તોડો;
                    }
                    typep->v3error("Attempting to extend using non-class");
                }
            }
        }
    }
    શૂન્ય visit(AstClass* nodep) override {
        જો (nodep->user3SetOnce()) પાછા;
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
            m_ds.m_dotSymp = m_curSymp = m_modSymp = m_statep->get_ગાંઠ_Sym(nodep);
            m_modp = nodep;
            પૂર્ણાંક next = 0;
            માટે (AstClassExtends* cextp = nodep->extendsp(); cextp;
                 cextp = VN_AS(cextp->nextp(), ClassExtends)) {
                // Replace abstract reference with hard pointer
                // Will need later resolution when deal with parameters
                જો (++next == 2 && !nodep->isInterfaceClass() && !cextp->isImplements()) {
                    cextp->v3error("Multiple inheritance illegal on non-interface classes"
                                   " (IEEE 1800-2023 8.13)");
                }
                iterate(cextp);
                જો (m_statep->forPrimary()) {
                    જો (cextp->parameterized()) {
                        // Parameters in extends statement.
                        // The class can't be resolved in the current pass.
                        m_extendsParam.ઉમેરો(nodep);
                        m_insideClassExtParam = સાચું;
                    }
                    જો (AstClassRefDType* અચલ classRefp
                        = VN_CAST(cextp->childDTypep(), ClassRefDType)) {
                        AstClass* અચલ classp = classRefp->classp();
                        જો (classp != nodep) iterate(classp);
                        જો (m_extendsParam.find(classp) != m_extendsParam.end()) {
                            // One of its super classes has parameters in extends statement.
                            // Some links may not be resolved in the first pass.
                            m_extendsParam.ઉમેરો(nodep);
                            m_insideClassExtParam = સાચું;
                        }
                    }
                }

                જો (AstClass* અચલ baseClassp = cextp->classOrNullp()) {
                    // Already converted. Update symbol table to link unlinked members.
                    // Base class has to be visited in a case if its extends statement
                    // needs to be handled. Recursive inheritance was already checked.
                    // Must be here instead of in LinkDotParam to handle
                    // "class (type T) extends T".
                    જો (baseClassp == nodep) {
                        cextp->v3error("Attempting to extend class " << nodep->prettyNameQ()
                                                                     << " from itself");
                    } નહીંતર જો (extendsClass(baseClassp, nodep)) {
                        cextp->v3error("Attempting to extend class "
                                       << nodep->prettyNameQ() << " from "
                                       << baseClassp->prettyNameQ()
                                       << ", which circularly inherits from it");
                    } નહીંતર જો (cextp->isImplements() && !baseClassp->isInterfaceClass()) {
                        cextp->v3error("Attempting to implement from non-interface class "
                                       << baseClassp->prettyNameQ() << '\n'
                                       << "... Suggest use 'extends'");
                    } નહીંતર જો (!cextp->isImplements() && !nodep->isInterfaceClass()
                               && baseClassp->isInterfaceClass()) {
                        cextp->v3error("Attempting to extend from interface class "
                                       << baseClassp->prettyNameQ() << '\n'
                                       << "... Suggest use 'implements'");
                    }
                    baseClassp->isExtended(સાચું);
                    nodep->isExtended(સાચું);
                    iterate(baseClassp);
                    importSymbolsFromExtended(nodep, cextp);
                    ચાલુ;
                }
            }
            m_ds.m_dotSymp = m_curSymp;

            iterateChildren(nodep);
            માટે (અચલ auto& itr : m_ifForwards) {
                અચલ VSymEnt* અચલ implSymp = itr.second;
                addInterfaceForward(nodep, VN_AS(implSymp->nodep(), ગાંઠ_FTask),
                                    VN_AS(implSymp->parentp()->nodep(), Class));
            }
        }
        // V3Width when determines types needs to find enum values and such
        // so add members pointing to appropriate enum values
        {
            VMemberMap memberMap;
            માટે (VSymEnt::const_iterator it = m_curSymp->begin(); it != m_curSymp->end(); ++it) {
                Ast_ગાંઠ* અચલ itemp = it->second->nodep();
                જો (!memberMap.findMember(nodep, it->first)) {
                    જો (AstEnumItem* અચલ aitemp = VN_CAST(itemp, EnumItem)) {
                        AstEnumItemRef* અચલ newp = નવું AstEnumItemRef{
                            aitemp->fileline(), aitemp, it->second->classOrPackagep()};
                        UINFO(8, indent()
                                     << "Class import noderef '" << it->first << "' " << newp);
                        nodep->addMembersp(newp);
                        memberMap.ઉમેરો(nodep, newp);
                    }
                }
            }
        }
        m_ds.m_dotSymp = VL_RESTORER_PREV(m_curSymp);
    }

    // Collect the ClassOrPackageRefs in a chained scope operand in source order.
    // The grammar builds a scope operand only from packageClassScopeItem, which
    // always yields a ClassOrPackageRef, nested in Dots, so nothing else appears.
    સ્થિર શૂન્ય collectScopeRefs(Ast_ગાંઠ* nodep, std::vector<AstClassOrPackageRef*>& refps) {
        જો (AstClassOrPackageRef* અચલ refp = VN_CAST(nodep, ClassOrPackageRef)) {
            refps.ધકેલો_back(refp);
            પાછા;
        }
        AstDot* અચલ dotp = VN_CAST(nodep, Dot);
        UASSERT_OBJ(dotp, nodep, "Non-scope node in package/class scope operand");
        collectScopeRefs(dotp->lhsp(), refps);
        collectScopeRefs(dotp->rhsp(), refps);
    }

    // Resolve a chained class-scope operand (`pkg::outer::inner` in
    // `pkg::outer::inner::t`) and reduce it to its innermost ClassOrPackageRef.
    // Returns false, having reported, if a segment could not be resolved.
    તાર્કિક reduceScopeDot(AstRefDType* nodep, AstDot* scopeDotp) {
        std::vector<AstClassOrPackageRef*> refps;
        collectScopeRefs(scopeDotp, refps);

        VSymEnt* scopeSymp = m_ds.m_dotSymp;
        માટે (size_t i = 0; i < refps.size(); ++i) {
            AstClassOrPackageRef* અચલ refp = refps[i];
            જો (!refp->classOrPackageSkipp() && !refp->classOrPackageNodep()
                && !m_statep->resolveClassOrPackage(scopeSymp, refp, i == 0, ખોટું,
                                                    "class/package reference")) {
                પાછા ખોટું;  // Error already reported
            }
            જો (i + 1 == refps.size()) તોડો;
            // A middle segment that resolved to a node with no reachable module
            // (e.g. a typedef alias of a not-yet-specialized parameterized class)
            // gives nothing to look the next segment up in.
            Ast_ગાંઠ_Module* અચલ modp = refp->classOrPackageSkipp();
            જો (!modp) {
                refp->v3warn(E_UNSUPPORTED, "Unsupported: Multiple '::' package/class reference");
                પાછા ખોટું;
            }
            scopeSymp = m_statep->get_ગાંઠ_Sym(modp);
        }
        AstClassOrPackageRef* અચલ innerp = refps.back();
        innerp->unlinkFrBack();
        VL_DO_DANGLING(ધકેલો_Deletep(scopeDotp->unlinkFrBack()), scopeDotp);
        nodep->classOrPackageOpp(innerp);
        પાછા સાચું;
    }

    શૂન્ય visit(AstRefDType* nodep) override {

        જો (auto* અચલ typeOfp = nodep->typeofp()) {
            iterate(typeOfp);
            // After iteration, typeofp() may have been replaced with a resolved type
            // (e.g., DOT expression like if0.rq_t resolved to REFDTYPE)
            // If it's now a NodeDType, replace this RefDType with it
            જો (Ast_ગાંઠ_DType* અચલ resolvedDTypep = VN_CAST(nodep->typeofp(), ગાંઠ_DType)) {
                UINFO(9,
                      indent() << "iface capture typeofp resolved to dtype, replacing RefDType\n");
                resolvedDTypep->unlinkFrBack();
                nodep->replaceWith(resolvedDTypep);
                VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
                // Originally (PR #6637) this block re-captured a resolved
                // RefDType into the iface-capture ledger when user2p held a
                // Cell, to ensure re-resolution during the paramed pass.
                // In practice, the typeofp-resolved RefDType never carries a
                // Cell in user2p: the capture happens earlier when the
                // typedef is first linked.
                જો (V3LinkDotIfaceCapture::enabled()) {
                    જો (AstRefDType* અચલ resolvedRefp = VN_CAST(resolvedDTypep, RefDType)) {
                        જો (VL_UNCOVERABLE(
                                VN_IS(resolvedRefp->user2p(), Cell))) {  // LCOV_EXCL_LINE
                            resolvedRefp->v3fatalSrc(  // LCOV_EXCL_LINE
                                "typeofp resolved RefDType has Cell in user2p;"
                                " expected to be captured already");
                        }
                    }
                }
            }
            પાછા;
        }

        // Resolve its reference
        જો (m_statep->forParamed() && nodep->user3()) {
            જો (V3LinkDotIfaceCapture::enabled() && nodep->user2p()) {
                UINFO(9, indent() << "iface capture clear user3 for captured typedef name="
                                  << nodep->name() << " cell=" << nodep->user2p());
            }
            nodep->user3(ખોટું);
        }
        જો (nodep->user3SetOnce()) {
            જો (V3LinkDotIfaceCapture::enabled() && nodep->user2p()) {
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
            જો (VL_UNCOVERABLE(m_statep->forParamed() && nodep->classOrPackageOpp())) {
                nodep->v3fatalSrc(  // LCOV_EXCL_LINE
                    "RefDType user3 revisit with classOrPackageOpp in paramed pass");
            }
            પાછા;
        }
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        જો (nodep->classOrPackageOpp()) {
            // Resolve a chained scope (`pkg::cls::t`, `pkg::cls#(P)::t`) from outside in.
            // On failure reduceScopeDot has reported, so stop before using the operand.
            જો (AstDot* અચલ scopeDotp = VN_CAST(nodep->classOrPackageOpp(), Dot)) {
                જો (!reduceScopeDot(nodep, scopeDotp)) પાછા;
            }
            // Re-read: reduceScopeDot may have replaced the operand
            Ast_ગાંઠ* અચલ cpackagep = nodep->classOrPackageOpp();
            UASSERT_OBJ(!VN_IS(cpackagep, Dot), cpackagep,
                        "Package/class scope operand should be reduced by now");
            જો (AstClassOrPackageRef* અચલ cpackagerefp = VN_CAST(cpackagep, ClassOrPackageRef)) {
                iterate(cpackagerefp);
                અચલ AstClass* અચલ clsp = VN_CAST(cpackagerefp->classOrPackageNodep(), Class);
                જો (clsp && clsp->hasGParam()) {
                    // Unable to link before the instantiation of parameter classes.
                    // The class reference node still has to be visited now to later link
                    // parameters.
                    iterate(cpackagep);
                    પાછા;
                }
                // Defer non-typedef references through typedef aliases of parameterized classes.
                જો (m_statep->forPrimary() && !VN_IS(nodep->backp(), Typedef)
                    && isParamedClassRef(cpackagerefp)) {
                    iterate(cpackagep);
                    પાછા;
                }

                અચલ તાર્કિક doDefaultTypedef = !(m_resolvingTypedef && m_statep->forPrimary());
                જો (!cpackagerefp->classOrPackageSkipp(doDefaultTypedef)
                    && !cpackagerefp->classOrPackageNodep()) {
                    VSymEnt* અચલ foundp = m_statep->resolveClassOrPackage(
                        m_ds.m_dotSymp, cpackagerefp, સાચું, ખોટું, "class/package reference");
                    જો (!foundp) પાછા;
                }
                nodep->classOrPackagep(cpackagerefp->classOrPackageSkipp(doDefaultTypedef));
                જો (!VN_IS(nodep->classOrPackagep(), Class)
                    && !VN_IS(nodep->classOrPackagep(), Package)) {
                    જો (m_statep->forPrimary()) {
                        // It may be a type that comes from parameter class that is not
                        // instantioned yet
                        iterate(cpackagep);
                        પાછા;
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
            VL_DO_DANGLING(ધકેલો_Deletep(cpackagep->unlinkFrBack()), cpackagep);
        }

        જો (m_ds.m_dotp && (m_ds.m_dotPos == DP_PACKAGE || m_ds.m_dotPos == DP_SCOPE)) {
            UASSERT_OBJ(VN_IS(m_ds.m_dotp->lhsp(), ClassOrPackageRef), m_ds.m_dotp->lhsp(),
                        "Bad package link");
            auto* અચલ cpackagerefp = VN_CAST(m_ds.m_dotp->lhsp(), ClassOrPackageRef);
            UASSERT_OBJ(cpackagerefp->classOrPackageSkipp(), m_ds.m_dotp->lhsp(),
                        "Bad package link");
            nodep->classOrPackagep(cpackagerefp->classOrPackageSkipp());
            m_ds.m_dotPos = DP_SCOPE;
        } નહીંતર {
            // Allow REFDTYPE under DOT when referencing interface typedefs
            // This is needed for patterns like: typedef iface.a_t a_t;
            // The dependency graph handles resolution when IfaceCapture is disabled
            અચલ તાર્કિક ifaceFinalSegmentAllowed
                = (m_ds.m_dotPos == DP_FINAL) && m_ds.m_dotSymp
                  && VN_IS(m_ds.m_dotSymp->nodep(), Cell)
                  && VN_CAST(m_ds.m_dotSymp->nodep(), Cell)->modp()
                  && VN_IS(VN_CAST(m_ds.m_dotSymp->nodep(), Cell)->modp(), Iface);
            જો (!ifaceFinalSegmentAllowed) {
                checkNoDot(nodep);
            } નહીંતર {
                UINFO(9, indent() << "allow REFDTYPE under DOT for iface typedef name="
                                  << nodep->prettyNameQ() << " dotSym=" << m_ds.m_dotSymp);
                // Clear the dot state so we don't propagate errors
                m_ds.m_dotPos = DP_SCOPE;
            }
        }
        જો (!nodep->typedefp() && !nodep->subDTypep()) {
            અચલ VSymEnt* foundp;
            જો (nodep->classOrPackagep()) {
                foundp = m_statep->get_ગાંઠ_Sym(nodep->classOrPackagep())->findIdFlat(nodep->name());
            } નહીંતર જો (m_ds.m_dotPos == DP_FIRST || m_ds.m_dotPos == DP_NONE) {
                foundp = findIdFallbackSkipMemberDType(m_curSymp, nodep->name());
            } નહીંતર {
                // Defensive: dotPos should be DP_FIRST/DP_NONE or classOrPackagep set.
                v3fatalSrc("Unexpected dotPos="
                           << static_cast<પૂર્ણાંક>(m_ds.m_dotPos)  // LCOV_EXCL_LINE
                           << " in RefDType lookup for "
                           << nodep->prettyNameQ());  // LCOV_EXCL_LINE
                foundp = nullptr;  // LCOV_EXCL_LINE
            }
            {
                જો (AstTypedef* અચલ defp
                    = foundp ? VN_CAST(foundp->nodep(), Typedef) : nullptr) {
                    // Don't check if typedef is to a <type T>::<reference> as might not be
                    // resolved yet
                    જો (!nodep->classOrPackagep() && !defp->isUnderClass())
                        checkDeclOrder(nodep, defp);
                    nodep->typedefp(defp);
                    nodep->classOrPackagep(foundp->classOrPackagep());

                } નહીંતર જો (AstParamTypeDType* અચલ defp
                           = foundp ? VN_CAST(foundp->nodep(), ParamTypeDType) : nullptr) {
                    // A PARAMTYPEDTYPE's child REFDTYPE referencing itself is the normal
                    // AST structure for type parameters (e.g., "type T = base" has a child
                    // REFDTYPE for the default type). Only error on true recursion where
                    // a TYPEDEF contains a reference back to itself.
                    અચલ તાર્કિક isParamTypeChild = (defp == nodep->backp());
                    અચલ તાર્કિક isTypedefRecursion
                        = isParamTypeChild && VN_IS(defp->backp(), Typedef);
                    જો (isTypedefRecursion) {
                        nodep->v3error("Reference to '"
                                       << m_ds.m_dotText << (m_ds.m_dotText == "" ? "" : ".")
                                       << nodep->prettyName() << "'"
                                       << " type would form a recursive definition");
                        nodep->refDTypep(nodep->findVoidDType());  // Try to reduce later errors
                    } નહીંતર {
                        nodep->refDTypep(defp);
                        nodep->classOrPackagep(foundp->classOrPackagep());
                        captureIfaceParamType(nodep, defp);
                    }
                } નહીંતર જો (AstClass* અચલ defp
                           = foundp ? VN_CAST(foundp->nodep(), Class) : nullptr) {
                    // Don't check if typedef is to a <type T>::<reference> as might not be
                    // resolved yet
                    જો (!nodep->classOrPackagep()) checkDeclOrder(nodep, defp);
                    AstPin* અચલ paramsp = nodep->paramsp();
                    જો (paramsp) paramsp->unlinkFrBackWithNext();
                    AstClassRefDType* અચલ newp
                        = નવું AstClassRefDType{nodep->fileline(), defp, paramsp};
                    newp->classOrPackagep(foundp->classOrPackagep());
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
                    પાછા;
                } નહીંતર જો (m_insideClassExtParam) {
                    પાછા;
                } નહીંતર {
                    જો (foundp) {
                        UINFO(1, "Found sym node: " << foundp->nodep());
                        nodep->v3error("Expecting a data type: "
                                       << nodep->prettyNameQ()
                                       << ", found: " << foundp->nodep()->prettyTypeName());
                    } નહીંતર {
                        nodep->v3error("Can't find typedef/interface: " << nodep->prettyNameQ());
                    }
                }
            }
        }
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstRequireDType* nodep) override {
        // Handle type validation for localparam type assignments
        // Iterate child to resolve any ParseRef nodes
        iterateChildren(nodep);
        // Only emit error if the child is not a type.
        // Do NOT unwrap valid types here - leave that to V3Width.
        // Unwrapping here breaks type parameter resolution during cloning.
        // Allow Dot through to defer the handling until the dot expression is resolved.
        જો (nodep->lhsp() && !VN_IS(nodep->lhsp(), ગાંઠ_DType) && !VN_IS(nodep->lhsp(), Dot)) {
            // Not a type - emit error
            જો (VN_IS(nodep->lhsp(), Const)) {
                nodep->lhsp()->v3error("Expecting a data type, not a constant");
            } નહીંતર {
                nodep->lhsp()->v3error("Expecting a data type, not "
                                       << nodep->lhsp()->typeName() << ": '"
                                       << nodep->lhsp()->prettyName() << "'");
            }
            // Replace with void to keep AST valid and allow clean exit
            nodep->replaceWith(નવું AstVoidDType{nodep->fileline()});
            VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
        }
    }
    શૂન્ય visit(AstDpiExport* nodep) override {
        // AstDpiExport: Make sure the function referenced exists, then dump it
        LINKDOT_VISIT_START();
        iterateChildren(nodep);
        checkNoDot(nodep);
        VSymEnt* અચલ foundp = m_curSymp->findIdFallback(nodep->name());
        Ast_ગાંઠ_FTask* અચલ taskp = foundp ? VN_AS(foundp->nodep(), ગાંઠ_FTask) : nullptr;
        જો (!taskp) {
            nodep->v3error(
                "Can't find definition of exported task/function: " << nodep->prettyNameQ());
        } નહીંતર જો (taskp->dpiExport()) {
            nodep->v3error("Function was already DPI Exported, duplicate not allowed: "
                           << nodep->prettyNameQ());
        } નહીંતર {
            taskp->dpiExport(સાચું);
            જો (nodep->cname() != "") taskp->cname(nodep->cname());
        }
        VL_DO_DANGLING(ધકેલો_Deletep(nodep->unlinkFrBack()), nodep);
    }
    શૂન્ય visit(AstDisable* nodep) override {
        LINKDOT_VISIT_START();
        checkNoDot(nodep);
        તાર્કિક hasPartSelect = ખોટું;
        અચલ string targetPath
            = nodep->targetRefp() ? extractDottedPath(nodep->targetRefp(), hasPartSelect) : "";
        VL_RESTORER_COPY(m_ds);
        m_ds.init(m_curSymp);
        m_ds.m_dotPos = DP_FIRST;
        m_ds.m_disablep = nodep;
        iterateChildren(nodep);
        જો (nodep->targetRefp()) {
            જો (AstTaskRef* અચલ taskRefp = VN_CAST(nodep->targetRefp(), TaskRef)) {
                nodep->targetp(taskRefp->taskp());
            } નહીંતર જો (!VN_IS(nodep->targetRefp(), ParseRef)) {
                // If it is a ParseRef, either it couldn't be linked or it is linked to a block
                nodep->v3warn(E_UNSUPPORTED, "Node of type "
                                                 << nodep->targetRefp()->prettyTypeName()
                                                 << " referenced by disable");
                VL_DO_DANGLING(ધકેલો_Deletep(nodep->unlinkFrBack()), nodep);
                પાછા;
            }
            જો (nodep->targetp()) {
                nodep->targetRefp()->unlinkFrBack()->delete_વૃક્ષ();
                જો (!hasPartSelect) {
                    અચલ string instancePath = targetInstancePath(nodep->targetp(), targetPath);
                    જો (!instancePath.empty()) {
                        // Keep only the instance prefix so V3LinkJump can reference the selected
                        // instance's process queue without reparsing the disable target.
                        nodep->targetRefp(
                            નવું AstVarXRef{nodep->fileline(), "", instancePath, VAccess::READ});
                    }
                }
            }
        }
    }
    શૂન્ય visitPackageImportOrExport(Ast_ગાંઠ* nodep) {
        // No longer needed
        LINKDOT_VISIT_START();
        checkNoDot(nodep);
        જો (m_statep->forParamed()) VL_DO_DANGLING(ધકેલો_Deletep(nodep->unlinkFrBack()), nodep);
    }
    શૂન્ય visit(AstPackageImport* nodep) override { visitPackageImportOrExport(nodep); }
    શૂન્ય visit(AstPackageExport* nodep) override { visitPackageImportOrExport(nodep); }
    શૂન્ય visit(AstPackageExportStarStar* nodep) override { visitPackageImportOrExport(nodep); }
    શૂન્ય visit(AstCellRef* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstCellArrayRef* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        // Expression already iterated
    }
    શૂન્ય visit(AstUnlinkedRef* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        // No need to iterate, if we have a UnlinkedVarXRef, we're already done
    }
    શૂન્ય visit(Ast_Stmt_પદાવલિ* nodep) override {
        LINKDOT_VISIT_START();
        checkNoDot(nodep);
        // Check if nodep represents a super.new call;
        જો (AstNew* અચલ newExprp = VN_CAST(nodep->exprp(), New)) {
            // in this case it was already linked, so it doesn't have a super reference
            m_explicitSuperNewp = newExprp;
        } નહીંતર જો (અચલ AstDot* અચલ dotp = VN_CAST(nodep->exprp(), Dot)) {
            જો (dotp->lhsp()->name() == "super") {
                જો (AstNew* અચલ newExprp = VN_CAST(dotp->rhsp(), New)) {
                    m_explicitSuperNewp = newExprp;
                }
            }
        }
        iterateChildren(nodep);
    }

    શૂન્ય visit(AstIfaceRefDType* nodep) override {
        જો (nodep->user3SetOnce()) પાછા;
        LINKDOT_VISIT_START();
        જો (nodep->paramsp()) {
            // If there is no parameters, there is no need to visit this node.
            AstIface* અચલ ifacep = nodep->ifacep();
            UASSERT_OBJ(ifacep, nodep, "Port parameters of AstIfaceRefDType without ifacep()");
            જો (ifacep->dead()) પાછા;
            checkNoDot(nodep);
            VL_RESTORER_CLEAR(m_usedPins);
            VL_RESTORER_CLEAR(m_usedDefParamPins);
            VL_RESTORER(m_pinSymp);
            m_pinSymp = m_statep->get_ગાંઠ_Sym(ifacep);
            iterateAndNextNull(nodep->paramsp());
        }
    }

    શૂન્ય visit(AstAttrOf* nodep) override { iterateChildren(nodep); }
    શૂન્ય visit(AstCastParse* nodep) override {
        જો (nodep->user3SetOnce()) પાછા;
        LINKDOT_VISIT_START();
        // A cast may start a dotted expression: HERE type'(expr) . member_or_method
        અચલ તાર્કિક dotLhs = m_ds.m_dotPos == DP_FIRST;
        જો (!dotLhs) checkNoDot(nodep);
        {
            VL_RESTORER_COPY(m_ds);
            m_ds.init(m_curSymp);
            iterateChildren(nodep);
        }
        જો (dotLhs) m_ds.m_dotPos = DP_MEMBER;
    }

    શૂન્ય visit(AstAssignW* nodep) override {
        LINKDOT_VISIT_START();
        checkNoDot(nodep);
        VL_RESTORER(m_replaceWithAlias);
        જો (nodep->user2()) m_replaceWithAlias = ખોટું;
        iterateChildren(nodep);
    }

    શૂન્ય visit(AstTypedef* nodep) override {
        VL_RESTORER(m_resolvingTypedef)
        m_resolvingTypedef = સાચું;
        iterateChildren(nodep);
    }

    શૂન્ય visit(Ast_ગાંઠ* nodep) override {
        VL_RESTORER(m_inPackedArray);
        જો (VN_IS(nodep, PackArrayDType)) {
            m_inPackedArray = સાચું;
        } નહીંતર જો (!m_inPackedArray) {
            LINKDOT_VISIT_START();
            checkNoDot(nodep);
        }
        iterateChildren(nodep);
    }

જાહેર:
    // CONSTRUCTORS
    LinkDotResolveVisitor(AstNetlist* rootp, LinkDotState* statep)
        : m_statep{statep} {
        UINFO(4, __FUNCTION__ << ": ");

        // Note: no need to clear user3 on ledger entries here.
        // VNUser3InUse (m_inuser3) already logically clears user3 on all
        // nodes via the generation counter.  The ledger may also hold stale
        // refp pointers to deleted AST nodes, so iterating it is unsafe.

        iterate(rootp);
        અચલ std::map<std::string, Ast_ગાંઠ_Module*> modulesToRevisit
            = std::move(m_modulesToRevisit);
        m_lastDeferredp = nullptr;
        માટે (auto& p : modulesToRevisit) {
            Ast_ગાંઠ_Module* અચલ modp = p.second;
            modp->foreach([](Ast_ગાંઠ* અચલ nodep) { nodep->user3(ખોટું); });
            iterate(modp);
        }
    }
    ~LinkDotResolveVisitor() override = મૂળભૂત;
};

//######################################################################
// Link class functions

શૂન્ય V3LinkDot::dumpSubstep(અચલ string& name) {
    જો (dump_વૃક્ષ_Either_Level() >= 9) {
        V3Global::dump_Check_Global_વૃક્ષ(name);
    } નહીંતર જો (debug() >= 5) {  // on high dbg level, dump even if not explicitly told to
        v3Global.rootp()->dump_વૃક્ષ_File(v3Global.debugFilename(name + ".tree"));
    }
}

શૂન્ય V3LinkDot::linkDotGuts(AstNetlist* rootp, VLinkDotStep step) {
    VIsCached::clear_Cache_વૃક્ષ();  // Avoid using any stale isPure
    dumpSubstep("prelinkdot");
    LinkDotState state{rootp, step};

    { LinkDotFindVisitor{rootp, &state}; }
    dumpSubstep("prelinkdot-find");

    { LinkDotFindIfaceVisitor{rootp, &state}; }
    dumpSubstep("prelinkdot-findiface");

    જો (step == LDS_SCOPED) {
        // Well after the initial link when we're ready to operate on the flat design,
        // process AstScope's.  This needs to be separate pass after whole hierarchy graph created.
        { LinkDotScopeVisitor{rootp, &state}; }
        v3Global.assertScoped(સાચું);
        dumpSubstep("prelinkdot-scoped");
    } નહીંતર {
        // Initial link stage, resolve parameters and interfaces
        { LinkDotParamVisitor{rootp, &state}; }
        dumpSubstep("prelinkdot-param");
    }
    state.dumpSelf("prelinkdot");
    state.computeIfaceModSyms();
    state.computeIfaceVarSyms();
    state.computeScopeAliases();
    state.dumpSelf("linkdot-preresolve");
    અચલ LinkDotResolveVisitor visitor{rootp, &state};
    state.dumpSelf("linkdot-done");
}

શૂન્ય V3LinkDot::linkDotPrimary(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    linkDotGuts(nodep, LDS_PRIMARY);
    V3Global::dump_Check_Global_વૃક્ષ("linkdot", 0, dump_વૃક્ષ_Either_Level() >= 6);
}

શૂન્ય V3LinkDot::linkDotParamed(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    linkDotGuts(nodep, LDS_PARAMED);
    V3Global::dump_Check_Global_વૃક્ષ("linkdotparam", 0, dump_વૃક્ષ_Either_Level() >= 3);
}

શૂન્ય V3LinkDot::linkDotScope(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    linkDotGuts(nodep, LDS_SCOPED);
    V3Global::dump_Check_Global_વૃક્ષ("linkdot", 0, dump_વૃક્ષ_Either_Level() >= 3);
}
