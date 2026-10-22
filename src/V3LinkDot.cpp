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
//ଶୀର୍ଷ
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

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ଅନ୍ତର୍ଭାଵ "V3LinkDot.h"

#ଅନ୍ତର୍ଭାଵ "V3Global.h"
#ଅନ୍ତର୍ଭାଵ "V3Graph.h"
#ଅନ୍ତର୍ଭାଵ "V3LinkDotIfaceCapture.h"
#ଅନ୍ତର୍ଭାଵ "V3MemberMap.h"
#ଅନ୍ତର୍ଭାଵ "V3Parse.h"
#ଅନ୍ତର୍ଭାଵ "V3Randomize.h"
#ଅନ୍ତର୍ଭାଵ "V3Stats.h"
#ଅନ୍ତର୍ଭାଵ "V3String.h"
#ଅନ୍ତର୍ଭାଵ "V3SymTable.h"

#ଅନ୍ତର୍ଭାଵ <algorithm>
#ଅନ୍ତର୍ଭାଵ <cstdlib>
#ଅନ୍ତର୍ଭାଵ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

ସ୍ଥିର string extractDottedPath(Ast_ଗଣ୍ଠି* nodep, ତାର୍କିକ& hasPartSelect) {
    ଯଦି (AstParseRef* ଅଚଳ refp = VN_CAST(nodep, ParseRef)) {
        ଫେରନ୍ତୁ refp->name();
    } ଅନ୍ୟଥା ଯଦି (AstVarRef* ଅଚଳ refp = VN_CAST(nodep, VarRef)) {
        ଫେରନ୍ତୁ refp->name();
    } ଅନ୍ୟଥା ଯଦି (AstDot* ଅଚଳ dotp = VN_CAST(nodep, Dot)) {
        ଅଚଳ string lhs = extractDottedPath(dotp->lhsp(), hasPartSelect);
        ଅଚଳ string rhs = extractDottedPath(dotp->rhsp(), hasPartSelect);
        ଫେରନ୍ତୁ VString::dot(lhs, ".", rhs);
    } ଅନ୍ୟଥା ଯଦି (VN_IS(nodep, SelBit) || VN_IS(nodep, SelExtract) || VN_IS(nodep, SelPlus)
               || VN_IS(nodep, SelMinus)) {
        hasPartSelect = ସତ୍ୟ;
        ଫେରନ୍ତୁ "";
    }
    ଫେରନ୍ତୁ "";
}
ସ୍ଥିର string lexicalDisablePath(Ast_ଗଣ୍ଠି* ଅଚଳ targetp) {
    std::vector<string> names;
    ପାଇଁ (Ast_ଗଣ୍ଠି* curp = targetp; curp; curp = curp->aboveLoopp()) {
        ଯଦି (Ast_ଗଣ୍ଠି_Block* ଅଚଳ blockp = VN_CAST(curp, ଗଣ୍ଠି_Block)) {
            ଯଦି (blockp->name() != "") names.ଠେଲ_back(blockp->name());
        } ଅନ୍ୟଥା ଯଦି (Ast_ଗଣ୍ଠି_FTask* ଅଚଳ ftaskp = VN_CAST(curp, ଗଣ୍ଠି_FTask)) {
            names.ଠେଲ_back(ftaskp->name());
        } ଅନ୍ୟଥା ଯଦି (VN_IS(curp, ଗଣ୍ଠି_Module)) {
            ଭାଙ୍ଗ;
        }
    }
    string path;
    ପାଇଁ (auto it = names.crbegin(); it != names.crend(); ++it) {
        path = VString::dot(path, ".", *it);
    }
    ଫେରନ୍ତୁ path;
}
ସ୍ଥିର string targetInstancePath(Ast_ଗଣ୍ଠି* ଅଚଳ targetp, ଅଚଳ string& targetPath) {
    ଅଚଳ string lexicalPath = lexicalDisablePath(targetp);
    ଯଦି (lexicalPath == "" || targetPath == lexicalPath) ଫେରନ୍ତୁ "";
    ଅଚଳ string suffix = "." + lexicalPath;
    ଯଦି (!VString::endsWith(targetPath, suffix)) ଫେରନ୍ତୁ "";
    ଫେରନ୍ତୁ targetPath.substr(0, targetPath.size() - suffix.size());
}

// ######################################################################
//  Matcher classes (for suggestion matching)

ଶ୍ରେଣୀ Link_ଗଣ୍ଠି_Matcher_Class_Or_Package final : ସର୍ବସାଧାରଣ VNodeMatcher {
ସର୍ବସାଧାରଣ:
    ତାର୍କିକ ଗଣ୍ଠି_Match(ଅଚଳ Ast_ଗଣ୍ଠି* nodep) ଅଚଳ override {
        ଫେରନ୍ତୁ VN_IS(nodep, Class) || VN_IS(nodep, Package);
    }
};
ଶ୍ରେଣୀ Link_ଗଣ୍ଠି_Matcher_FTask final : ସର୍ବସାଧାରଣ VNodeMatcher {
ସର୍ବସାଧାରଣ:
    ତାର୍କିକ ଗଣ୍ଠି_Match(ଅଚଳ Ast_ଗଣ୍ଠି* nodep) ଅଚଳ override { ଫେରନ୍ତୁ VN_IS(nodep, ଗଣ୍ଠି_FTask); }
};
ଶ୍ରେଣୀ Link_ଗଣ୍ଠି_Matcher_Modport final : ସର୍ବସାଧାରଣ VNodeMatcher {
ସର୍ବସାଧାରଣ:
    ତାର୍କିକ ଗଣ୍ଠି_Match(ଅଚଳ Ast_ଗଣ୍ଠି* nodep) ଅଚଳ override { ଫେରନ୍ତୁ VN_IS(nodep, Modport); }
};
ଶ୍ରେଣୀ Link_ଗଣ୍ଠି_Matcher_Prod final : ସର୍ବସାଧାରଣ VNodeMatcher {
ସର୍ବସାଧାରଣ:
    ତାର୍କିକ ଗଣ୍ଠି_Match(ଅଚଳ Ast_ଗଣ୍ଠି* nodep) ଅଚଳ override { ଫେରନ୍ତୁ VN_IS(nodep, RSProd); }
};
ଶ୍ରେଣୀ Link_ଗଣ୍ଠି_Matcher_Var final : ସର୍ବସାଧାରଣ VNodeMatcher {
ସର୍ବସାଧାରଣ:
    ତାର୍କିକ ଗଣ୍ଠି_Match(ଅଚଳ Ast_ଗଣ୍ଠି* nodep) ଅଚଳ override {
        ଫେରନ୍ତୁ VN_IS(nodep, Var) || VN_IS(nodep, LambdaArgRef);
    }
};
ଶ୍ରେଣୀ Link_ଗଣ୍ଠି_Matcher_Var_IO final : ସର୍ବସାଧାରଣ VNodeMatcher {
ସର୍ବସାଧାରଣ:
    ତାର୍କିକ ଗଣ୍ଠି_Match(ଅଚଳ Ast_ଗଣ୍ଠି* nodep) ଅଚଳ override {
        ଅଚଳ AstVar* ଅଚଳ varp = VN_CAST(nodep, Var);
        ଯଦି (!varp) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଫେରନ୍ତୁ varp->isIO();
    }
};
ଶ୍ରେଣୀ Link_ଗଣ୍ଠି_Matcher_Var_Param final : ସର୍ବସାଧାରଣ VNodeMatcher {
ସର୍ବସାଧାରଣ:
    ତାର୍କିକ ଗଣ୍ଠି_Match(ଅଚଳ Ast_ଗଣ୍ଠି* nodep) ଅଚଳ override {
        ଅଚଳ AstVar* ଅଚଳ varp = VN_CAST(nodep, Var);
        ଯଦି (!varp) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଫେରନ୍ତୁ varp->isParam();
    }
};

//######################################################################
// LinkDot state, as a visitor of each AstNode

ଶ୍ରେଣୀ LinkDotState final {
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
    ଅଚଳ VNUser1InUse m_inuser1;
    ଅଚଳ VNUser2InUse m_inuser2;
    ଅଚଳ VNUser4InUse m_inuser4;

ସର୍ବସାଧାରଣ:
    // ENUMS
    // In order of priority, compute first ... compute last
    ଗଣନା SAMNum : uint8_t { SAMN_MODPORT, SAMN_IFTOP, SAMN__MAX };  // Values for m_scopeAliasMap

ବ୍ୟକ୍ତିଗତ:
    // TYPES
    using ScopeAliasMap = std::unordered_map<VSymEnt*, VSymEnt*>;
    using IfaceModSyms = std::vector<std::pair<AstIface*, VSymEnt*>>;

    ସ୍ଥିର LinkDotState* s_errorThisp;  // Last self, for error reporting only

    // MEMBERS
    VSymGraph m_syms;  // Symbol table by hierarchy
    VSymGraph m_mods;  // Symbol table of all module names
    VSymEnt* m_dunitEntp = nullptr;  // $unit entry
    std::multimap<std::string, VSymEnt*>
        m_nameScopeSymMap;  // Map of scope referenced by non-pretty textual name
    std::set<std::pair<Ast_ଗଣ୍ଠି_Module*, std::string>>
        m_implicitNameSet;  // For [module][signalname] if we can implicitly create it
    std::array<ScopeAliasMap, SAMN__MAX> m_scopeAliasMap;  // Map of <lhs,rhs> aliases
    std::vector<VSymEnt*> m_ifaceVarSyms;  // List of AstIfaceRefDType's to be imported
    IfaceModSyms m_ifaceModSyms;  // List of AstIface+Symbols to be processed
    ଅଚଳ VLinkDotStep m_step;  // Operational step

ସର୍ବସାଧାରଣ:
    // METHODS

    ଶୂନ୍ୟ dumpSelf(ଅଚଳ string& nameComment, ତାର୍କିକ force = ମିଥ୍ୟା) {
        ଯଦି (debug() >= 6 || dumpLevel() >= 6 || force) {
            ଅଚଳ string filename = v3Global.debugFilename(nameComment) + ".txt";
            UINFO(4, "Dumping " << filename);
            ଅଚଳ std::unique_ptr<std::ofstream> logp{V3File::new_ofstream(filename)};
            ଯଦି (logp->fail()) v3fatal("Can't write file: " << filename);
            std::ostream& os = *logp;
            // TODO the symbol table has node pointers which may be
            // dangling, as we call deleteTree in these visitors without
            // pushing for later deletion (or deleting from the symbol table)
            // So, only completely safe to call this when under --debug or --debug-leak.
            m_syms.dumpSelf(os);
            ତାର୍କିକ first = ସତ୍ୟ;
            ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା samn = 0; samn < SAMN__MAX; ++samn) {
                ଯଦି (!m_scopeAliasMap[samn].empty()) {
                    ଯଦି (first) os << "\nScopeAliasMap:\n";
                    first = ମିଥ୍ୟା;
                    ପାଇଁ (ScopeAliasMap::iterator it = m_scopeAliasMap[samn].begin();
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
    ସ୍ଥିର ଶୂନ୍ୟ preErrorDumpHandler() {
        ଯଦି (s_errorThisp) s_errorThisp->preErrorDump();
    }
    ଶୂନ୍ୟ preErrorDump() {
        ସ୍ଥିର ତାର୍କିକ s_diddump = ମିଥ୍ୟା;
        ଯଦି (!s_diddump && dump_ବୃକ୍ଷ_Level()) {
            s_diddump = ସତ୍ୟ;
            dumpSelf("linkdot-preerr", ସତ୍ୟ);
            v3Global.rootp()->dump_ବୃକ୍ଷ_File(v3Global.debugFilename("linkdot-preerr.tree"));
        }
    }

    // CONSTRUCTORS
    LinkDotState(AstNetlist* rootp, VLinkDotStep step)
        : m_syms{rootp}
        , m_mods{rootp}
        , m_step{step} {
        UINFO(4, __FUNCTION__ << ": ");
        s_errorThisp = ଏହି;
        V3Error::errorExitCb(preErrorDumpHandler);  // If get error, dump self
        readModNames();
    }
    ~LinkDotState() {
        V3Error::errorExitCb(nullptr);
        s_errorThisp = nullptr;
    }

    // ACCESSORS
    VSymGraph* symsp() { ଫେରନ୍ତୁ &m_syms; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା step_ସଂଖ୍ୟା() ଅଚଳ { ଫେରନ୍ତୁ static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(m_step); }
    ତାର୍କିକ forPrimary() ଅଚଳ { ଫେରନ୍ତୁ m_step == LDS_PRIMARY; }
    ତାର୍କିକ forParamed() ଅଚଳ { ଫେରନ୍ତୁ m_step == LDS_PARAMED; }
    ତାର୍କିକ forScopeCreation() ଅଚଳ { ଫେରନ୍ତୁ m_step == LDS_SCOPED; }

    // METHODS
    ସ୍ଥିର string ଗଣ୍ଠି_Text_Type(Ast_ଗଣ୍ଠି* nodep) {
        ଯଦି (ଅଚଳ AstVar* ଅଚଳ varp = VN_CAST(nodep, Var)) {
            ଯଦି (varp->isIO() || varp->isIfaceRef()) {
                ଫେରନ୍ତୁ "port";
            } ଅନ୍ୟଥା ଯଦି (varp->isGParam()) {
                ଫେରନ୍ତୁ "parameter";
            } ଅନ୍ୟଥା ଯଦି (varp->varType() == VVarType::LPARAM) {
                ଫେରନ୍ତୁ "local parameter";
            } ଅନ୍ୟଥା {
                ଫେରନ୍ତୁ "variable";
            }
        } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstParamTypeDType* ଅଚଳ dtypep = VN_CAST(nodep, ParamTypeDType)) {
            ଯଦି (dtypep->isGParam()) {
                ଫେରନ୍ତୁ "type parameter";
            } ଅନ୍ୟଥା {
                ଫେରନ୍ତୁ "local type parameter";
            }
        } ଅନ୍ୟଥା ଯଦି (VN_IS(nodep, Begin)) {
            ଫେରନ୍ତୁ "block";
        } ଅନ୍ୟଥା ଯଦି (VN_IS(nodep, Cell)) {
            ଫେରନ୍ତୁ "instance";
        } ଅନ୍ୟଥା ଯଦି (VN_IS(nodep, Constraint)) {
            ଫେରନ୍ତୁ "constraint";
        } ଅନ୍ୟଥା ଯଦି (VN_IS(nodep, Func)) {
            ଫେରନ୍ତୁ "function";
        } ଅନ୍ୟଥା ଯଦି (VN_IS(nodep, GenBlock)) {
            ଫେରନ୍ତୁ "generate block";
        } ଅନ୍ୟଥା ଯଦି (VN_IS(nodep, Iface)) {
            ଫେରନ୍ତୁ "interface";
        } ଅନ୍ୟଥା ଯଦି (VN_IS(nodep, RSProd)) {
            ଫେରନ୍ତୁ "randsequence production";
        } ଅନ୍ୟଥା ଯଦି (VN_IS(nodep, Task)) {
            ଫେରନ୍ତୁ "task";
        } ଅନ୍ୟଥା {
            ଫେରନ୍ତୁ nodep->prettyTypeName();
        }
    }

    Ast_ଗଣ୍ଠି_Module* findModuleSym(ଅଚଳ string& modName) {
        ଅଚଳ VSymEnt* ଅଚଳ foundp = m_mods.rootp()->findIdFallback(modName);
        ଫେରନ୍ତୁ foundp ? VN_AS(foundp->nodep(), ଗଣ୍ଠି_Module) : nullptr;
    }
    ଶୂନ୍ୟ readModNames() {
        // Look at all modules, and store pointers to all module names
        ପାଇଁ (Ast_ଗଣ୍ଠି_Module *nextp, *nodep = v3Global.rootp()->modulesp(); nodep; nodep = nextp) {
            nextp = VN_AS(nodep->nextp(), ଗଣ୍ଠି_Module);
            m_mods.rootp()->ଭର୍ତ୍ତି(nodep->name(), ନୂତନ VSymEnt{&m_mods, nodep});
        }
    }

    VSymEnt* ମୂଳ_Entp() ଅଚଳ { ଫେରନ୍ତୁ m_syms.rootp(); }
    VSymEnt* dunitEntp() ଅଚଳ { ଫେରନ୍ତୁ m_dunitEntp; }
    ଶୂନ୍ୟ checkDuplicate(VSymEnt* lookupSymp, Ast_ଗଣ୍ଠି* nodep, ଅଚଳ string& name) {
        // Lookup the given name under current symbol table
        // Insert if not found
        // Report error if there's a duplicate
        //
        // Note we only check for conflicts at the same level; it's ok if one block hides another
        // We also wouldn't want to not insert it even though it's lower down

        ଅଚଳ VSymEnt* ଅଚଳ foundp = lookupSymp->findIdFlat(name);
        Ast_ଗଣ୍ଠି* ଅଚଳ fnodep = foundp ? foundp->nodep() : nullptr;
        ଯଦି (!fnodep) {
            // Not found, will add in a moment.
            ଯଦି (forPrimary() && VSymEnt::checkSimilarname(nodep)) {
                ଅଚଳ VSymEnt* ଅଚଳ alt = lookupSymp->findSimilarIdFlat(name);
                ଯଦି (alt) {
                    nodep->v3warn(SIMILARNAME, "Declaration overlaps another with different case: "
                                                   << nodep->prettyNameQ() << '\n'
                                                   << nodep->warnContextPrimary() << '\n'
                                                   << alt->nodep()->warnOther()
                                                   << "... Location of original declaration\n"
                                                   << alt->nodep()->warnContextSecondary());
                }
            }
        } ଅନ୍ୟଥା ଯଦି (nodep == fnodep) {  // Already inserted.
            // Good.
        } ଅନ୍ୟଥା ଯଦି (foundp->imported()) {  // From package
            // We don't throw VARHIDDEN as if the import is later the symbol
            // table's import wouldn't warn
        } ଅନ୍ୟଥା ଯଦି (forPrimary() && VN_IS(nodep, GenBlock)
                   && (VN_IS(fnodep, Begin) || VN_IS(fnodep, GenBlock))) {
            // Begin: ... blocks often replicate under genif/genfor, so
            // suppress duplicate checks.  See t_gen_forif.v for an example.
        } ଅନ୍ୟଥା {
            UINFO(4, "name " << name);  // Not always same as nodep->name
            UINFO(4, "Var1 " << nodep);
            UINFO(4, "Var2 " << fnodep);
            // Quote the node's own name as for messages, if it is the symbol's
            ଅଚଳ string nameQ
                = fnodep->name() == name ? fnodep->prettyNameQ() : Ast_ଗଣ୍ଠି::prettyNameQ(name);
            ଯଦି (nodep->type() == fnodep->type()) {
                nodep->v3error("Duplicate declaration of "
                               << ଗଣ୍ଠି_Text_Type(fnodep) << ": " << nameQ << '\n'
                               << nodep->warnContextPrimary() << '\n'
                               << fnodep->warnOther() << "... Location of original declaration\n"
                               << fnodep->warnContextSecondary());
                nodep->decl_Token_ଅଙ୍କ_Set_Min(0);  // Disable checking forward typedefs
                fnodep->decl_Token_ଅଙ୍କ_Set_Min(0);  // Disable checking forward typedefs
            } ଅନ୍ୟଥା {
                nodep->v3error("Unsupported in C: "
                               << ucfirst(ଗଣ୍ଠି_Text_Type(nodep)) << " has the same name as "
                               << ଗଣ୍ଠି_Text_Type(fnodep) << ": " << nameQ << '\n'
                               << nodep->warnContextPrimary() << '\n'
                               << fnodep->warnOther() << "... Location of original declaration\n"
                               << fnodep->warnContextSecondary());
            }
        }
    }
    ଶୂନ୍ୟ ଭର୍ତ୍ତି_DUnit(AstNetlist* nodep) {
        // $unit on top scope
        VSymEnt* ଅଚଳ symp = ନୂତନ VSymEnt{&m_syms, nodep};
        UINFO(9, "      INSERTdunit se" << cvtToHex(symp));
        symp->parentp(ମୂଳ_Entp());  // Needed so backward search can find name of top module
        symp->fallbackp(nullptr);
        ମୂଳ_Entp()->ଭର୍ତ୍ତି("$unit ", symp);  // Space so can never name conflict with user code
        //
        UASSERT_OBJ(!m_dunitEntp, nodep, "Call insertDUnit only once");
        m_dunitEntp = symp;
    }
    VSymEnt* ଭର୍ତ୍ତି_ଶୀର୍ଷ_Cell(Ast_ଗଣ୍ଠି_Module* nodep, ଅଚଳ string& scopename) {
        // Only called on the module at the very top of the hierarchy
        VSymEnt* ଅଚଳ symp = ନୂତନ VSymEnt{&m_syms, nodep};
        UINFO(9, "      INSERTtop se" << cvtToHex(symp) << "  " << scopename << " " << nodep);
        symp->parentp(ମୂଳ_Entp());  // Needed so backward search can find name of top module
        symp->fallbackp(dunitEntp());  // Needed so can find $unit stuff
        nodep->user1p(symp);
        checkDuplicate(ମୂଳ_Entp(), nodep, nodep->origName());
        ମୂଳ_Entp()->ଭର୍ତ୍ତି(nodep->origName(), symp);
        ଯଦି (forScopeCreation()) m_nameScopeSymMap.emplace(scopename, symp);
        ଫେରନ୍ତୁ symp;
    }
    VSymEnt* ଭର୍ତ୍ତି_ଶୀର୍ଷ_Iface(AstCell* nodep, ଅଚଳ string& scopename) {
        VSymEnt* ଅଚଳ symp = ନୂତନ VSymEnt{&m_syms, nodep};
        UINFO(9, "      INSERTtopiface se" << cvtToHex(symp) << "  " << scopename << " " << nodep);
        symp->parentp(ମୂଳ_Entp());  // Needed so backward search can find name of top module
        symp->fallbackp(dunitEntp());  // Needed so can find $unit stuff
        nodep->user1p(symp);
        ଯଦି (nodep->modp()) nodep->modp()->user1p(symp);
        checkDuplicate(ମୂଳ_Entp(), nodep, nodep->origName());
        ମୂଳ_Entp()->ଭର୍ତ୍ତି(nodep->origName(), symp);
        ଯଦି (forScopeCreation()) m_nameScopeSymMap.emplace(scopename, symp);
        ଫେରନ୍ତୁ symp;
    }
    VSymEnt* ଭର୍ତ୍ତି_Cell(VSymEnt* abovep, VSymEnt* modSymp, AstCell* nodep,
                        ଅଚଳ string& scopename) {
        UASSERT_OBJ(abovep, nodep, "Null symbol table inserting node");
        VSymEnt* ଅଚଳ symp = ନୂତନ VSymEnt{&m_syms, nodep};
        UINFO(9, "      INSERTcel se" << cvtToHex(symp) << "  " << scopename << " above=se"
                                      << cvtToHex(abovep) << " mods=se" << cvtToHex(modSymp)
                                      << " node=" << nodep);
        symp->parentp(abovep);
        symp->fallbackp(dunitEntp());  // Needed so can find $unit stuff
        nodep->user1p(symp);
        ଯଦି (nodep->modp()) nodep->modp()->user1p(symp);
        checkDuplicate(abovep, nodep, nodep->origName());
        abovep->reinsert(nodep->origName(), symp);
        ଯଦି (forScopeCreation() && abovep != modSymp && !modSymp->findIdFlat(nodep->name())) {
            // If it's foo_DOT_bar, we need to be able to find it under "foo_DOT_bar" too.
            // Duplicates are possible, as until resolve generates might
            // have 2 same cells under an if
            modSymp->reinsert(nodep->name(), symp);
        }
        ଯଦି (forScopeCreation()) m_nameScopeSymMap.emplace(scopename, symp);
        ଫେରନ୍ତୁ symp;
    }
    ଶୂନ୍ୟ ଭର୍ତ୍ତି_Map(VSymEnt* symp, ଅଚଳ string& scopename) {
        ଯଦି (forScopeCreation()) m_nameScopeSymMap.emplace(scopename, symp);
    }

    VSymEnt* ଭର୍ତ୍ତି_Inline(VSymEnt* abovep, VSymEnt* modSymp, AstCellInline* nodep,
                          ଅଚଳ string& basename) {
        // A fake point in the hierarchy, corresponding to a named block flattened by V3Begin.
        // Names are looked up in the module with the block's flattened prefix.
        UASSERT_OBJ(abovep, nodep, "Null symbol table inserting node");
        VSymEnt* ଅଚଳ symp = ନୂତନ VSymEnt{&m_syms, nodep};
        UINFO(9, "      INSERTinl se" << cvtToHex(symp) << "  " << basename << " above=se"
                                      << cvtToHex(abovep) << " mods=se" << cvtToHex(modSymp)
                                      << " node=" << nodep);
        symp->parentp(abovep);
        symp->fallbackp(modSymp);
        symp->symPrefix(nodep->name() + "__DOT__");
        nodep->user1p(symp);
        checkDuplicate(abovep, nodep, nodep->name());
        abovep->reinsert(basename, symp);
        ଯଦି (abovep != modSymp && !modSymp->findIdFlat(nodep->name())) {
            // If it's foo_DOT_bar, we need to be able to find it under that too.
            modSymp->reinsert(nodep->name(), symp);
        }
        ଫେରନ୍ତୁ symp;
    }
    VSymEnt* ଭର୍ତ୍ତି_Block(VSymEnt* abovep, ଅଚଳ string& name, Ast_ଗଣ୍ଠି* nodep,
                         Ast_ଗଣ୍ଠି_Module* classOrPackagep) {
        // A fake point in the hierarchy, corresponding to a begin or function/task block
        // After we remove begins these will go away
        // Note we fallback to the symbol table of the parent, as we want to find variables there
        // However, cells walk the graph, so cells will appear under the begin/ftask itself
        UASSERT_OBJ(abovep, nodep, "Null symbol table inserting node");
        VSymEnt* ଅଚଳ symp = ନୂତନ VSymEnt{&m_syms, nodep};
        UINFO(9, "      INSERTblk se" << cvtToHex(symp) << "  above=se" << cvtToHex(abovep)
                                      << " pkg=" << cvtToHex(classOrPackagep)
                                      << "  node=" << nodep);
        symp->parentp(abovep);
        symp->classOrPackagep(classOrPackagep);
        symp->fallbackp(abovep);
        nodep->user1p(symp);
        ଯଦି (name != "") checkDuplicate(abovep, nodep, name);
        // Duplicates are possible, as until resolve generates might have 2 same cells under an if
        abovep->reinsert(name, symp);
        ଫେରନ୍ତୁ symp;
    }
    VSymEnt* ଭର୍ତ୍ତି_Sym(VSymEnt* abovep, ଅଚଳ string& name, Ast_ଗଣ୍ଠି* nodep,
                       Ast_ଗଣ୍ଠି_Module* classOrPackagep) {
        UASSERT_OBJ(abovep, nodep, "Null symbol table inserting node");
        VSymEnt* ଅଚଳ symp = ନୂତନ VSymEnt{&m_syms, nodep};
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
        ଫେରନ୍ତୁ symp;
    }
    ସ୍ଥିର ତାର୍କିକ exists_ଗଣ୍ଠି_Sym(Ast_ଗଣ୍ଠି* nodep) { ଫେରନ୍ତୁ nodep->user1p() != nullptr; }
    ସ୍ଥିର VSymEnt* get_ଗଣ୍ଠି_Sym(Ast_ଗଣ୍ଠି* nodep) {
        // Don't use this in ResolveVisitor, as we need to pick up the proper
        // reference under each SCOPE
        VSymEnt* ଅଚଳ symp = nodep->user1u().toSymEnt();
        UASSERT_OBJ(symp, nodep, "Module/etc never assigned a symbol entry?");
        ଫେରନ୍ତୁ symp;
    }
    VSymEnt* getScopeSym(AstScope* nodep) {
        ଅଚଳ auto it = m_nameScopeSymMap.find(nodep->name());
        UASSERT_OBJ(it != m_nameScopeSymMap.end(), nodep,
                    "Scope never assigned a symbol entry '" << nodep->name() << "'");
        ଫେରନ୍ତୁ it->second;
    }
    ଶୂନ୍ୟ implicitOkAdd(Ast_ଗଣ୍ଠି_Module* nodep, ଅଚଳ string& varname) {
        // Mark the given variable name as being allowed to be implicitly declared
        ଯଦି (nodep) {
            ଅଚଳ auto it = m_implicitNameSet.find(std::make_pair(nodep, varname));
            ଯଦି (it == m_implicitNameSet.end()) m_implicitNameSet.emplace(nodep, varname);
        }
    }
    ତାର୍କିକ implicitOk(Ast_ଗଣ୍ଠି_Module* nodep, ଅଚଳ string& varname) {
        ଫେରନ୍ତୁ nodep
               && (m_implicitNameSet.find(std::make_pair(nodep, varname))
                   != m_implicitNameSet.end());
    }

    // Track and later recurse interface modules
    ଶୂନ୍ୟ ଭର୍ତ୍ତି_Iface_Mod_Sym(AstIface* nodep, VSymEnt* symp) {
        m_ifaceModSyms.emplace_back(nodep, symp);
    }
    ଶୂନ୍ୟ computeIfaceModSyms();

    // Track and later insert interface references
    ଶୂନ୍ୟ ଭର୍ତ୍ତି_Iface_Var_Sym(VSymEnt* symp) {  // Where sym is for a VAR of dtype IFACEREFDTYPE
        m_ifaceVarSyms.ଠେଲ_back(symp);
    }
    // Iface for a raw or arrayed iface; peels nested array layers for multi-dim arrays.
    ସ୍ଥିର AstIfaceRefDType* ifaceRefFromArray(Ast_ଗଣ୍ଠି_DType* nodep) {
        ଯେତେବେଳେ (nodep) {
            ଯଦି (AstIfaceRefDType* ଅଚଳ ifp = VN_CAST(nodep, IfaceRefDType)) ଫେରନ୍ତୁ ifp;
            ଯଦି (ଅଚଳ AstBracketArrayDType* ଅଚଳ arrp = VN_CAST(nodep, BracketArrayDType)) {
                nodep = arrp->subDTypep();
            } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstUnpackArrayDType* ଅଚଳ arrp = VN_CAST(nodep, UnpackArrayDType)) {
                nodep = arrp->subDTypep();
            } ଅନ୍ୟଥା {
                ଫେରନ୍ତୁ nullptr;
            }
        }
        ଫେରନ୍ତୁ nullptr;
    }
    // Given a pin expression, resolve it to a live AstIface* (or nullptr).
    // Handles both simple VarRef and dotted VarXRef pin connections.
    AstIface* live_Iface_From_Pin_ପଦସମଷ୍ଟି(Ast_ଗଣ୍ଠି* exprp, VSymEnt* parentSymp) {
        AstVar* resolvedVarp = nullptr;
        ଯଦି (AstVarRef* ଅଚଳ refp = VN_CAST(exprp, VarRef)) {
            resolvedVarp = refp->varp();
        } ଅନ୍ୟଥା ଯଦି (AstVarXRef* ଅଚଳ xrefp = VN_CAST(exprp, VarXRef)) {
            VSymEnt* ଅଚଳ lookupSymp = parentSymp ? parentSymp->parentp() : nullptr;
            ଯଦି (lookupSymp) {
                string baddot;
                VSymEnt* okSymp = nullptr;
                ଅଚଳ string dotpath = xrefp->dotted() + "." + xrefp->name();
                VSymEnt* ଅଚଳ dotSymp
                    = findDotted(xrefp->fileline(), lookupSymp, dotpath, baddot, okSymp, ସତ୍ୟ);
                ଯଦି (dotSymp) resolvedVarp = VN_CAST(dotSymp->nodep(), Var);
            }
        }
        AstIface* resultp = nullptr;
        ଯଦି (resolvedVarp) {
            AstIfaceRefDType* ଅଚଳ irefp = ifaceRefFromArray(resolvedVarp->subDTypep());
            ଯଦି (irefp && irefp->ifaceViaCellp() && !irefp->ifaceViaCellp()->dead()) {
                resultp = irefp->ifaceViaCellp();
            }
        }
        ଫେରନ୍ତୁ resultp;
    }

    // Attempt to repair a port's AstIfaceRefDType by tracing through the
    // parent cell's pin connection to find the correct live interface.
    // Returns true if repaired or already correct; false if unresolvable.
    ତାର୍କିକ repairIfaceRef(VSymEnt* varSymp, AstVar* varp, AstIfaceRefDType* ifacerefp, ତାର୍କିକ isDead) {
        // Walk up symbol-table parents to find the enclosing AstCell
        VSymEnt* parentSymp = varSymp->parentp();
        AstCell* parentCellp = nullptr;
        ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା depth = 0; parentSymp && depth < 20; ++depth) {
            ଯଦି (AstCell* ଅଚଳ cp = VN_CAST(parentSymp->nodep(), Cell)) {
                parentCellp = cp;
                ଭାଙ୍ଗ;
            }
            parentSymp = parentSymp->parentp();
        }
        ଯଦି (!parentCellp) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        // Scan pins to find the one connected to this port variable
        ପାଇଁ (AstPin* pinp = parentCellp->pinsp(); pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            ଯଦି (pinp->modVarp() != varp && pinp->name() != varp->name()) ଜାରି;
            Ast_ଗଣ୍ଠି* ଅଚଳ exprp = pinp->exprp();
            ଯଦି (!exprp) ଫେରନ୍ତୁ ମିଥ୍ୟା;
            AstIface* ଅଚଳ newIfacep = live_Iface_From_Pin_ପଦସମଷ୍ଟି(exprp, parentSymp);
            ଯଦି (newIfacep && newIfacep != ifacerefp->ifaceViaCellp()) {
                UINFO(4, "  REPAIR-IFACE-REF var=" << varp->prettyNameQ()
                                                   << " old=" << ifacerefp->ifacep()->prettyNameQ()
                                                   << " new=" << newIfacep->prettyNameQ());
                ifacerefp->ifacep(newIfacep);
                ଫେରନ୍ତୁ ସତ୍ୟ;
            }
            ଯଦି (newIfacep) ଫେରନ୍ତୁ !isDead;  // Same interface - OK if live
            ଫେରନ୍ତୁ ମିଥ୍ୟା;
        }
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }

    ଶୂନ୍ୟ computeIfaceVarSyms() {
        ପାଇଁ (VSymEnt* varSymp : m_ifaceVarSyms) {
            AstVar* ଅଚଳ varp = varSymp ? VN_AS(varSymp->nodep(), Var) : nullptr;
            UINFO(9, "  insAllIface se" << cvtToHex(varSymp) << " " << varp);
            AstIfaceRefDType* ଅଚଳ ifacerefp = ifaceRefFromArray(varp->subDTypep());
            UASSERT_OBJ(ifacerefp, varp, "Non-ifacerefs on list!");
            ଯଦି (!ifacerefp->cellp() && !ifacerefp->isVirtual() && (varp->pin_ଅଙ୍କ() == 0)) {
                varp->v3error("Interface port declaration "
                              << varp->prettyNameQ() << " doesn't have corresponding port\n"
                              << varp->warnMore()
                                     + "... Perhaps intended an interface instantiation but "
                                       "are missing parenthesis (IEEE 1800-2023 25.3)?");
            }
            ଯଦି (!ifacerefp->ifaceViaCellp()) {
                ଯଦି (!ifacerefp->cellp()) {  // Probably a NotFoundModule, or a normal module if
                                            // made mistake
                    UINFO(1, "Associated cell " << Ast_ଗଣ୍ଠି::prettyNameQ(ifacerefp->cellName()));
                    ifacerefp->v3error("Cannot find file containing interface: "
                                       << Ast_ଗଣ୍ଠି::prettyNameQ(ifacerefp->ifaceName()));
                    ଜାରି;
                } ଅନ୍ୟଥା {
                    ifacerefp->v3fatalSrc("Unlinked interface");
                }
            } ଅନ୍ୟଥା ଯଦି (varp->isIfaceRef() && !ifacerefp->cellp() && ifacerefp->ifaceViaCellp()) {
                // Port variable (cellp=null) pointing to an interface.
                // The interface may be dead (template replaced by clone) or
                // live-but-wrong (template still alive because some instances
                // use default params, but this port should point to a
                // specialized clone based on the pin connection).
                ଅଚଳ ତାର୍କିକ isDead = ifacerefp->ifaceViaCellp()->dead();
                ଯଦି (isDead && forPrimary() && !varp->isIfaceParent()
                    && !v3Global.opt.ଶୀର୍ଷ_Ifaces_Supported()) {
                    varp->v3warn(E_UNSUPPORTED,
                                 "Unsupported: Interfaced port on top level module");
                }
                // Attempt repair: trace through parent cell's pin to find
                // the correct live interface for this port variable
                ଅଚଳ ତାର୍କିକ repaired = repairIfaceRef(varSymp, varp, ifacerefp, isDead);
                ଯଦି (!repaired && isDead) {
                    ifacerefp->v3error(
                        "Interface " << Ast_ଗଣ୍ଠି::prettyNameQ(ifacerefp->ifaceName())
                                     << " not connected as parent's interface not connected\n"
                                     << ifacerefp->warnMore()
                                     << "... Perhaps caused by another error on the parent "
                                        "interface that needs resolving\n"
                                     << ifacerefp->warnMore()
                                     << "... Or, perhaps intended an interface instantiation but "
                                        "are missing parenthesis (IEEE 1800-2023 25.3)?");
                    ଜାରି;
                }
            } ଅନ୍ୟଥା ଯଦି (ifacerefp->ifaceViaCellp()->dead()
                       || !exists_ଗଣ୍ଠି_Sym(ifacerefp->ifaceViaCellp())) {
                ifacerefp->ifaceViaCellp()->v3fatalSrc(
                    "Interface referenced by virtual iface is dead. "
                    "Dead interfaces have required linking steps skipped");
            }
            VSymEnt* ଅଚଳ ifaceSymp = get_ଗଣ୍ଠି_Sym(ifacerefp->ifaceViaCellp());
            VSymEnt* ifOrPortSymp = ifaceSymp;
            // Link Modport names to the Modport Node under the Interface
            ଯଦି (ifacerefp->isModport()) {
                VSymEnt* ଅଚଳ foundp = ifaceSymp->findIdFallback(ifacerefp->modportName());
                ତାର୍କିକ ok = ମିଥ୍ୟା;
                ଯଦି (foundp) {
                    ଯଦି (AstModport* ଅଚଳ modportp = VN_CAST(foundp->nodep(), Modport)) {
                        UINFO(4, "Link Modport: " << modportp);
                        ifacerefp->modportp(modportp);
                        ifOrPortSymp = foundp;
                        ok = ସତ୍ୟ;
                    }
                }
                ଯଦି (!ok) {
                    ଅଚଳ string suggest = suggestSymFallback(ifaceSymp, ifacerefp->modportName(),
                                                              Link_ଗଣ୍ଠି_Matcher_Modport{});
                    ifacerefp->modportFileline()->v3error(
                        "Modport not found under interface "
                        << ifacerefp->prettyNameQ(ifacerefp->ifaceName()) << ": "
                        << ifacerefp->prettyNameQ(ifacerefp->modportName()) << '\n'
                        << (suggest.empty() ? "" : ifacerefp->warnMore() + suggest));
                }
            }
            // Alias won't expand until interfaces and modport names are known; see notes at top
            ଭର୍ତ୍ତି_Scope_Alias(SAMN_IFTOP, varSymp, ifOrPortSymp);
        }
        m_ifaceVarSyms.clear();
    }

    ଶୂନ୍ୟ ଭର୍ତ୍ତି_Scope_Alias(SAMNum samn, VSymEnt* lhsp, VSymEnt* rhsp) {
        // Track and later insert scope aliases; an interface referenced by
        // a child cell connecting to that interface
        // Typically lhsp=VAR w/dtype IFACEREF, rhsp=IFACE cell
        UINFO(9, "   insertScopeAlias se" << cvtToHex(lhsp) << " se" << cvtToHex(rhsp));
        UASSERT_OBJ(
            !(VN_IS(rhsp->nodep(), Cell) && !VN_IS(VN_AS(rhsp->nodep(), Cell)->modp(), Iface)),
            rhsp->nodep(), "Got a non-IFACE alias RHS");
        m_scopeAliasMap[samn].emplace(lhsp, rhsp);
    }
    ଶୂନ୍ୟ computeScopeAliases() {
        UINFO(9, "computeIfaceAliases");
        ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା samn = 0; samn < SAMN__MAX; ++samn) {
            ପାଇଁ (ScopeAliasMap::iterator it = m_scopeAliasMap[samn].begin();
                 it != m_scopeAliasMap[samn].end(); ++it) {
                VSymEnt* ଅଚଳ lhsp = it->first;
                VSymEnt* srcp = lhsp;
                ଯେତେବେଳେ (ସତ୍ୟ) {  // Follow chain of aliases up to highest level non-alias
                    ଅଚଳ auto it2 = m_scopeAliasMap[samn].find(srcp);
                    ଯଦି (it2 != m_scopeAliasMap[samn].end()) {
                        srcp = it2->second;
                        ଜାରି;
                    } ଅନ୍ୟଥା {
                        ଭାଙ୍ଗ;
                    }
                }
                UINFO(9, "  iiasa: Insert alias se" << lhsp << " (" << lhsp->nodep()->typeName()
                                                    << ") <- se" << srcp << " " << srcp->nodep());
                // srcp should be an interface reference pointing to the interface we want to
                // import
                lhsp->importFromIface(symsp(), srcp);
                // Allow access to objects not permissible to be listed in a modport
                ଯଦି (VN_IS(srcp->nodep(), Modport)) {
                    lhsp->importFromIface(symsp(), srcp->parentp(), ସତ୍ୟ);
                }
            }
            // m_scopeAliasMap[samn].clear();  // Done with it, but put into debug file
        }
    }

ବ୍ୟକ୍ତିଗତ:
    VSymEnt* findWithAltFallback(VSymEnt* symp, ଅଚଳ string& name, ଅଚଳ string& altname) {
        VSymEnt* findp = symp->findIdFallback(name);
        ଯଦି (findp) ଫେରନ୍ତୁ findp;
        ଯଦି (altname != "") {
            UINFO(8, "     alt fallback");
            findp = symp->findIdFallback(altname);
        }
        ଫେରନ୍ତୁ findp;
    }

    VSymEnt* findWithAltFlat(VSymEnt* symp, ଅଚଳ string& name, ଅଚଳ string& altname) {
        VSymEnt* findp = symp->findIdFlat(name);
        ଯଦି (findp) ଫେରନ୍ତୁ findp;
        ଯଦି (altname != "") {
            UINFO(8, "     alt flat");
            findp = symp->findIdFlat(altname);
        }
        ଫେରନ୍ତୁ findp;
    }

ସର୍ବସାଧାରଣ:
    VSymEnt* findDotted(File_ଧାଡ଼ି* refLocationp, VSymEnt* lookupSymp, ଅଚଳ string& dotname,
                        string& baddot, VSymEnt*& okSymp, ତାର୍କିକ firstId) {
        // Given a dotted hierarchy name, return where in scope it is
        // Note when dotname=="" we just fall through and return lookupSymp
        UINFO(8, "    dottedFind se" << cvtToHex(lookupSymp) << " '" << dotname << "'");
        string leftname = dotname;
        okSymp = lookupSymp;  // So can list bad scopes
        ଯେତେବେଳେ (leftname != "") {  // foreach dotted part of xref name
            string::size_type pos = leftname.find('.');
            string ident;
            ଯଦି (pos != string::npos) {
                ident = leftname.substr(0, pos);
                leftname = leftname.substr(pos + 1);
            } ଅନ୍ୟଥା {
                ident = leftname;
                leftname = "";
            }
            baddot = ident;  // So user can see where they botched it
            okSymp = lookupSymp;
            string altIdent;
            ଯଦି (!forScopeCreation()) {
                // GENFOR Begin is foo__BRA__##__KET__ after we've genloop unrolled,
                // but presently should be just "foo".
                // Likewise cell foo__[array] before we've expanded arrays is just foo.
                // Multi-dim iface arrays append multiple __BRA__..__KET__ suffixes; strip them
                // all.
                altIdent = ident;
                ଯେତେବେଳେ (VString::endsWith(altIdent, "__KET__")) {
                    ଅଚଳ auto braPos = altIdent.rfind("__BRA__");
                    ଯଦି (braPos == string::npos) ଭାଙ୍ଗ;
                    altIdent = altIdent.substr(0, braPos);
                }
                ଯଦି (altIdent == ident) altIdent.clear();
            }
            UINFO(8, "         id " << ident << " alt " << altIdent << " left " << leftname
                                    << " at se" << lookupSymp);
            // Spec says; Look at existing module (cellnames then modname),
            // then look up (inst name or modname)
            ଯଦି (firstId) {
                // Check this module - subcellnames
                ଅଚଳ AstCell* cellp = lookupSymp ? VN_CAST(lookupSymp->nodep(), Cell)
                                                  : nullptr;  // Replicated below
                ଯଦି (VSymEnt* ଅଚଳ findSymp = findWithAltFallback(lookupSymp, ident, altIdent)) {
                    lookupSymp = findSymp;
                }

                // Check this module - cur modname
                ଅନ୍ୟଥା ଯଦି (cellp && cellp->modp()->origName() == ident) {
                }
                // $root we walk up to Netlist
                ଅନ୍ୟଥା ଯଦି (ident == "$root") {
                    lookupSymp = ମୂଳ_Entp();
                    // We've added the '$root' module, now everything else is one lower
                    ଯଦି (forScopeCreation()) {
                        lookupSymp = lookupSymp->findIdFlat(ident);
                        UASSERT(lookupSymp, "Cannot find $root module under netlist");
                    }
                }
                // Move up and check cellname + modname
                ଅନ୍ୟଥା {
                    ତାର୍କିକ crossedCell = ମିଥ୍ୟା;  // Crossed a cell boundary
                    ଯେତେବେଳେ (lookupSymp) {
                        lookupSymp = lookupSymp->parentp();
                        cellp = lookupSymp ? VN_CAST(lookupSymp->nodep(), Cell)
                                           : nullptr;  // Replicated above
                        ଯଦି (lookupSymp) {
                            UINFO(9, "    Up to " << lookupSymp);
                            ଯଦି (cellp || VN_IS(lookupSymp->nodep(), CellInline)) {
                                crossedCell = ସତ୍ୟ;
                            }
                            ଯଦି (cellp && cellp->modp()->origName() == ident) {
                                ଭାଙ୍ଗ;
                            } ଅନ୍ୟଥା ଯଦି (VSymEnt* ଅଚଳ findSym2p
                                       = findWithAltFallback(lookupSymp, ident, altIdent)) {
                                lookupSymp = findSym2p;
                                ଯଦି (crossedCell && VN_IS(lookupSymp->nodep(), Var)) {
                                    UINFO(9, "    Not found but matches var name in parent "
                                                 << lookupSymp);
                                    ଫେରନ୍ତୁ nullptr;
                                }
                                ଭାଙ୍ଗ;
                            }
                        } ଅନ୍ୟଥା {
                            ଭାଙ୍ଗ;
                        }
                    }
                    ଯଦି (!lookupSymp) ଫେରନ୍ତୁ nullptr;  // Not found
                }
            } ଅନ୍ୟଥା {  // Searching for middle path component, must be a cell or interface port
                VSymEnt* findSymp = findWithAltFlat(lookupSymp, ident, altIdent);
                ଯଦି (findSymp) {
                    lookupSymp = findSymp;
                } ଅନ୍ୟଥା {
                    // Try prefixed lookup for interface ports accessed through hierarchy
                    // (e.g., slave_inst.bus.data where bus is an interface port)
                    // After inlining, interface ports have prefixed names like
                    // top__DOT__slave_inst__DOT__bus
                    UINFO(8, "     middle-path: trying prefixed lookup for '" << ident << "'\n");
                    string baddot;
                    findSymp = findSymPrefixed(lookupSymp, ident, baddot, /*fallback=*/ସତ୍ୟ);
                    UINFO(8, "     middle-path: prefixed lookup result: "
                                 << (findSymp ? "found" : "not found") << "\n");
                    ଯଦି (findSymp) {
                        // Check if this is an interface reference variable
                        ଅଚଳ AstVar* varp = VN_CAST(findSymp->nodep(), Var);
                        ଯଦି (!varp) {
                            ଯଦି (ଅଚଳ AstVarScope* vscp = VN_CAST(findSymp->nodep(), VarScope)) {
                                varp = vscp->varp();
                            }
                        }
                        ଯଦି (varp && varp->isIfaceRef()) {
                            // Found an interface port - use findSymp directly
                            // computeScopeAliases has already imported interface members
                            // into this symbol entry, so we use it instead of redirecting
                            // to the shared modport definition (which would cause all
                            // instances to resolve to the same symbols - bug #2656)
                            lookupSymp = findSymp;
                        } ଅନ୍ୟଥା {
                            // Non-interface symbol in middle path must be a cell (module instance)
                            // to continue hierarchical resolution
                            ଯଦି (VN_IS(findSymp->nodep(), Cell)) {
                                lookupSymp = findSymp;
                            } ଅନ୍ୟଥା {
                                // Reject non-cell, non-interface symbols found via fallback
                                // to prevent accidental resolution to unrelated symbols
                                UINFO(8, "     middle-path: rejecting non-cell symbol\n");
                                ଫେରନ୍ତୁ nullptr;
                            }
                        }
                    } ଅନ୍ୟଥା {
                        ଫେରନ୍ତୁ nullptr;  // Not found
                    }
                }
            }
            ଯଦି (lookupSymp) {
                ଯଦି (ଅଚଳ AstCell* ଅଚଳ cellp = VN_CAST(lookupSymp->nodep(), Cell)) {
                    ଯଦି (ଅଚଳ Ast_ଗଣ୍ଠି_Module* ଅଚଳ modp = cellp->modp()) {
                        ଯଦି (modp->hierBlock() && !leftname.empty()) {
                            refLocationp->v3error("Cannot access scope inside hierarchical block");
                        } ଅନ୍ୟଥା ଯଦି (VN_IS(modp, NotFoundModule)) {
                            refLocationp->v3error("Dotted reference to instance that refers to "
                                                  "missing module/interface: "
                                                  << modp->prettyNameQ());
                        }
                    }
                }
                // Follow scope alias for nested interface port access
                ଯଦି (!leftname.empty()) {
                    ଅଚଳ auto aliasIt = m_scopeAliasMap[SAMN_IFTOP].find(lookupSymp);
                    ଯଦି (aliasIt != m_scopeAliasMap[SAMN_IFTOP].end()) {
                        lookupSymp = aliasIt->second;
                        // Alias may point to __Viftop VarScope; find corresponding Cell
                        ଯଦି (ଅଚଳ AstVarScope* ଅଚଳ vscp
                            = VN_CAST(lookupSymp->nodep(), VarScope)) {
                            ଅଚଳ string varName = vscp->varp()->name();
                            ସ୍ଥିର constexpr ଅଚଳ ଅକ୍ଷର* ଅଚଳ VIFTOP_SUFFIX = "__Viftop";
                            ଯଦି (VString::endsWith(varName, VIFTOP_SUFFIX)) {
                                ଅଚଳ string cellName
                                    = varName.substr(0, varName.size() - strlen(VIFTOP_SUFFIX));
                                VSymEnt* ଅଚଳ parentSymp = lookupSymp->parentp();
                                ଯଦି (parentSymp) {
                                    VSymEnt* ଅଚଳ cellSymp = parentSymp->findIdFlat(cellName);
                                    ଯଦି (cellSymp && VN_IS(cellSymp->nodep(), Cell)) {
                                        lookupSymp = cellSymp;  // Use Cell for member lookup
                                    }
                                }
                            }
                        }
                    } ଅନ୍ୟଥା {
                        // No alias; try following IfaceRefDType to interface cell
                        ଅଚଳ AstVar* varp = VN_CAST(lookupSymp->nodep(), Var);
                        ଯଦି (!varp) {
                            ଯଦି (ଅଚଳ AstVarScope* ଅଚଳ vscp
                                = VN_CAST(lookupSymp->nodep(), VarScope)) {
                                varp = vscp->varp();
                            }
                        }
                        ଯଦି (varp && varp->isIfaceRef()) {
                            ଯଦି (ଅଚଳ AstIfaceRefDType* ଅଚଳ ifaceRefp
                                = ifaceRefFromArray(varp->dtypep())) {
                                ଯଦି (ifaceRefp->cellp() && exists_ଗଣ୍ଠି_Sym(ifaceRefp->cellp())) {
                                    lookupSymp = get_ଗଣ୍ଠି_Sym(ifaceRefp->cellp());
                                } ଅନ୍ୟଥା ଯଦି (ifaceRefp->ifaceViaCellp()
                                           && exists_ଗଣ୍ଠି_Sym(ifaceRefp->ifaceViaCellp())) {
                                    lookupSymp = get_ଗଣ୍ଠି_Sym(ifaceRefp->ifaceViaCellp());
                                } ଅନ୍ୟଥା ଯଦି (ifaceRefp->ifacep()
                                           && exists_ଗଣ୍ଠି_Sym(ifaceRefp->ifacep())) {
                                    lookupSymp = get_ଗଣ୍ଠି_Sym(ifaceRefp->ifacep());
                                }
                            }
                        }
                    }
                }
            }
            firstId = ମିଥ୍ୟା;
        }
        ଫେରନ୍ତୁ lookupSymp;
    }

    ସ୍ଥିର string removeLastInlineScope(ଅଚଳ string& name) {
        string out = name;
        ଅଚଳ string dot = "__DOT__";
        ଅଚଳ size_t dotPos = out.rfind(dot, out.size() - dot.length() - 1);
        ଯଦି (dotPos == string::npos) {
            ଫେରନ୍ତୁ "";
        } ଅନ୍ୟଥା {
            ଫେରନ୍ତୁ out.erase(dotPos + dot.length(), string::npos);
        }
    }

    VSymEnt* findSymPrefixed(VSymEnt* lookupSymp, ଅଚଳ string& dotname, string& baddot,
                             ତାର୍କିକ fallback) {
        // Find symbol in given point in hierarchy, allowing prefix (post-Inline)
        // For simplicity lookupSymp may be passed nullptr result from findDotted
        ଯଦି (!lookupSymp) ଫେରନ୍ତୁ nullptr;
        UINFO(8, "    findSymPrefixed "
                     << dotname << " under se" << cvtToHex(lookupSymp)
                     << ((lookupSymp->symPrefix() == "") ? "" : " as ")
                     << ((lookupSymp->symPrefix() == "") ? "" : lookupSymp->symPrefix() + dotname)
                     << "  at se" << lookupSymp << " fallback=" << fallback);
        string prefix = lookupSymp->symPrefix();
        VSymEnt* foundp = nullptr;
        ଯେତେବେଳେ (!foundp) {
            ଯଦି (fallback) {
                foundp = lookupSymp->findIdFallback(prefix + dotname);  // Might be nullptr
            } ଅନ୍ୟଥା {
                foundp = lookupSymp->findIdFlat(prefix + dotname);  // Might be nullptr
            }
            ଯଦି (prefix.empty()) ଭାଙ୍ଗ;
            ଅଚଳ string nextPrefix = removeLastInlineScope(prefix);
            ଯଦି (prefix == nextPrefix) ଭାଙ୍ଗ;
            prefix = nextPrefix;
        }
        ଯଦି (!foundp) baddot = dotname;
        ଫେରନ୍ତୁ foundp;
    }
    ସ୍ଥିର ତାର୍କିକ checkIfClassOrPackage(ଅଚଳ VSymEnt* ଅଚଳ symp) {
        ଯଦି (VN_IS(symp->nodep(), Class) || VN_IS(symp->nodep(), Package)) ଫେରନ୍ତୁ ସତ୍ୟ;

        // Helper: check if a RefDType might resolve to a class later.
        ଅଚଳ auto isPotentialClassRef = [](ଅଚଳ AstRefDType* refp) -> ତାର୍କିକ {
            ଫେରନ୍ତୁ refp && !refp->typeofp()
                   && (!refp->classOrPackageOpp()
                       || VN_IS(refp->classOrPackageOpp(), ClassOrPackageRef));
        };

        // Helper: allow types that can represent a class/package handle or an unresolved ref.
        // BasicDType is allowed here only for null/object handles.
        ଅଚଳ auto is_Valid_Type_ଗଣ୍ଠି = [](ଅଚଳ Ast_ଗଣ୍ଠି* nodep) -> ତାର୍କିକ {
            ଫେରନ୍ତୁ VN_IS(nodep, VoidDType) || VN_IS(nodep, BasicDType)
                   || VN_IS(nodep, ClassRefDType) || VN_IS(nodep, ParseRef);
        };

        ଯଦି (ଅଚଳ AstTypedef* ଅଚଳ typedefp = VN_CAST(symp->nodep(), Typedef)) {
            ଅଚଳ Ast_ଗଣ୍ଠି_DType* dtypep = typedefp->subDTypep();
            ଯଦି (!dtypep) dtypep = typedefp->childDTypep();
            ଯଦି (VN_IS(dtypep, ClassRefDType)) ଫେରନ୍ତୁ ସତ୍ୟ;
            ଯଦି (isPotentialClassRef(VN_CAST(dtypep, RefDType))) ଫେରନ୍ତୁ ସତ୍ୟ;
        } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstParamTypeDType* ଅଚଳ paramTypep
                   = VN_CAST(symp->nodep(), ParamTypeDType)) {
            // Before V3Param the declared default is in childDTypep (possibly
            // wrapped in a RequireDType); after V3Param it is consumed and the
            // bound type is the resolved data type, e.g. a type parameter
            // inherited from a specialized base class (REQ #(Item) -> class Item).
            Ast_ଗଣ୍ଠି* childp = paramTypep->childDTypep();
            ଯଦି (ଅଚଳ AstRequireDType* ଅଚଳ reqp = VN_CAST(childp, RequireDType)) {
                childp = reqp->lhsp();
            }
            ଅଚଳ Ast_ଗଣ୍ଠି* ଅଚଳ checkp = childp ? childp : paramTypep->skipRefp();
            ଯଦି (is_Valid_Type_ଗଣ୍ଠି(checkp) || isPotentialClassRef(VN_CAST(checkp, RefDType))) {
                ଫେରନ୍ତୁ ସତ୍ୟ;
            }
        }
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }
    VSymEnt* resolveClassOrPackage(VSymEnt* lookSymp, AstClassOrPackageRef* nodep, ତାର୍କିକ fallback,
                                   ତାର୍କିକ classOnly, ଅଚଳ string& forWhat,
                                   ତାର୍କିକ deferIfUnresolved = ମିଥ୍ୟା) {
        ଯଦି (nodep->classOrPackageSkipp()) ଫେରନ୍ତୁ get_ଗଣ୍ଠି_Sym(nodep->classOrPackageSkipp());
        VSymEnt* foundp;
        VSymEnt* ସନ୍ଧାନ_Symp = lookSymp;
        ଯଦି (VL_UNCOVERABLE(ସନ୍ଧାନ_Symp && VN_IS(ସନ୍ଧାନ_Symp->nodep(), ParamTypeDType))) {
            ସନ୍ଧାନ_Symp->nodep()->v3fatalSrc(  // LCOV_EXCL_LINE
                "resolveClassOrPackage: unexpected ParamTypeDType lookup");
        }
        ଯଦି (fallback) {
            VSymEnt* currentLookSymp = ସନ୍ଧାନ_Symp;
            କର {
                foundp = currentLookSymp->findIdFlat(nodep->name());
                ଯଦି (foundp && !checkIfClassOrPackage(foundp)) foundp = nullptr;
                ଯଦି (!foundp) currentLookSymp = currentLookSymp->fallbackp();
            } ଯେତେବେଳେ (!foundp && currentLookSymp);
        } ଅନ୍ୟଥା {
            foundp = ସନ୍ଧାନ_Symp->findIdFlat(nodep->name());
            ଯଦି (foundp && !checkIfClassOrPackage(foundp)) foundp = nullptr;
        }
        ଯଦି (!foundp && v3Global.rootp()->stdPackagep()) {  // Look under implied std::
            foundp = get_ଗଣ୍ଠି_Sym(v3Global.rootp()->stdPackagep())->findIdFlat(nodep->name());
        }
        ଯଦି (foundp) {
            nodep->classOrPackageNodep(foundp->nodep());
            ଫେରନ୍ତୁ foundp;
        }
        ଯଦି (deferIfUnresolved) ଫେରନ୍ତୁ nullptr;
        ଅଚଳ string suggest
            = suggestSymFallback(lookSymp, nodep->name(), Link_ଗଣ୍ଠି_Matcher_Class_Or_Package{});
        nodep->v3error((classOnly ? "Class" : "Package/class")
                       << " for '" << forWhat  // extends/implements
                       << "' not found: " << nodep->prettyNameQ() << '\n'
                       << (suggest.empty() ? "" : nodep->warnMore() + suggest));
        ଫେରନ୍ତୁ nullptr;
    }
    string suggestSymFallback(VSymEnt* lookupSymp, ଅଚଳ string& name,
                              ଅଚଳ VNodeMatcher& matcher) {
        // Suggest alternative symbol in given point in hierarchy
        // Does not support inline, as we find user-level errors before inlining
        // For simplicity lookupSymp may be passed nullptr result from findDotted
        ଯଦି (!lookupSymp) ଫେରନ୍ତୁ "";
        VSpellCheck speller;
        lookupSymp->candidateIdFallback(&speller, &matcher);
        ଫେରନ୍ତୁ speller.best_Candidate_ସନ୍ଦେଶ(name);
    }
    string suggestSymFlat(VSymEnt* lookupSymp, ଅଚଳ string& name, ଅଚଳ VNodeMatcher& matcher,
                          ତାର୍କିକ includeType) {
        ଯଦି (!lookupSymp) ଫେରନ୍ତୁ "";
        VSpellCheck speller;
        lookupSymp->candidateIdFlat(&speller, &matcher, includeType);
        ଫେରନ୍ତୁ speller.best_Candidate_ସନ୍ଦେଶ(name);
    }
};

LinkDotState* LinkDotState::s_errorThisp = nullptr;

//======================================================================

ଶ୍ରେଣୀ LinkDotFindVisitor final : ସର୍ବସାଧାରଣ VNVisitor {
    // NODE STATE
    // Cleared on global
    //  *::user1p()             -> See LinkDotState
    //  *::user2p()             -> See LinkDotState
    //  *::user3()              // bool. Node stored in m_virtualIfaces
    //  *::user4()              -> See LinkDotState
    ଅଚଳ VNUser3InUse m_inuser3;

    // STATE
    LinkDotState* ଅଚଳ m_statep;  // State to pass between visitors, including symbol table
    Ast_ଗଣ୍ଠି_Module* m_classOrPackagep = nullptr;  // Current package
    AstClocking* m_clockingp = nullptr;  // Current clocking block
    std::vector<AstScope*> m_scopes;  // Scopes detached during repeated module visits
    uint64_t m_statScopeVisits = 0;  // Non-top scopes encountered while finding instance symbols
    VSymEnt* m_modSymp = nullptr;  // Symbol Entry for current module
    VSymEnt* m_curSymp = nullptr;  // Symbol Entry for current table, where to lookup/insert
    string
        m_hierParamsName;  // Name of module with hierarchical type parameters, empty when not used
    string m_scope;  // Scope text
    ଅଚଳ Ast_ଗଣ୍ଠି_FTask* m_ftaskp = nullptr;  // Current function/task
    ତାର୍କିକ m_inRecursion = ମିଥ୍ୟା;  // Inside a recursive module
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_param_ଅଙ୍କ = 0;  // Parameter number, for position based connection
    ତାର୍କିକ m_explicitNew = ମିଥ୍ୟା;  // Hit a "new" function
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_mod_Block_ଅଙ୍କ = 0;  // Begin block number in module, 0=none seen
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_mod_With_ଅଙ୍କ = 0;  // With block number, 0=none seen
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_mod_Arg_ଅଙ୍କ = 0;  // Arg block number for randomize(), 0=none seen
    std::vector<AstIface*> m_virtIfaces;  // interfaces used as virtual,
                                          // needed for handleUnvisitedVirtIfaces()

    // METHODS
    ଶୂନ୍ୟ makeImplicitNew(AstClass* nodep) {
        AstFunc* ଅଚଳ newp = ନୂତନ AstFunc{nodep->fileline(), "new", nullptr, nullptr};
        // If needed, super.new() call is added the 2nd pass of V3LinkDotResolve,
        // because base classes are already resolved there.
        newp->isConstructor(ସତ୍ୟ);
        nodep->addMembersp(newp);
        UINFO(8, "Made implicit new for " << nodep->name() << ": " << nodep);
        m_statep->ଭର୍ତ୍ତି_Block(m_curSymp, newp->name(), newp, m_classOrPackagep);
    }

    ସ୍ଥିର ତାର୍କିକ isHierBlockWrapper(ଅଚଳ string& name) {
        ଅଚଳ V3HierBlockOptSet& hierBlocks = v3Global.opt.hierBlocks();
        ଫେରନ୍ତୁ hierBlocks.find(name) != hierBlocks.end();
    }

    ଶୂନ୍ୟ moveExternFuncDecl(Ast_ଗଣ୍ଠି_FTask* nodep, AstClass* classp) {
        // Move an 'extern function|task' to inside a class or, from an outer to inner-class
        ଯଦି (nodep->isExternDef()) ଫେରନ୍ତୁ;
        // Move it to proper spot under the target class
        nodep->unlinkFrBack();
        classp->addStmtsp(nodep);
        nodep->isExternDef(ସତ୍ୟ);  // So we check there's a matching extern
        nodep->classOrPackagep()->unlinkFrBack()->delete_ବୃକ୍ଷ();
        // Any "Type::" reference in the function's IO are really "MovedToClass::" references
        ଯଦି (nodep->fvarp()) moveExternFuncDeclRefs(nodep->fvarp(), classp);
        ପାଇଁ (Ast_ଗଣ୍ଠି* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ଯଦି (AstVar* ଅଚଳ portp = VN_CAST(stmtp, Var)) {
                ଯଦି (portp->isIO()) moveExternFuncDeclRefs(portp, classp);
            }
        }
    }
    ଶୂନ୍ୟ moveExternFuncDeclRefs(Ast_ଗଣ୍ଠି* nodep, AstClass* classp) {
        nodep->foreach([ଏହି, classp](AstClassOrPackageRef* refp) {  //
            ଯଦି (refp->name() == classp->name() && !refp->paramsp()) {
                UINFO(9, "Cleaning up external function type for class " << refp);
                AstDot* ଅଚଳ dotp = VN_CAST(refp->backp(), Dot);
                ଯଦି (dotp && dotp->lhsp() == refp) {
                    // If had DOT(CLASSREF this, ...) the DOT can go away
                    dotp->replaceWith(dotp->rhsp()->unlinkFrBack());
                    VL_DO_DANGLING2(ଠେଲ_Deletep(dotp), dotp, refp);
                } ଅନ୍ୟଥା {
                    VL_DO_DANGLING(ଠେଲ_Deletep(refp->unlinkFrBack()), refp);
                }
            }
        });
    }

    // VISITORS
    ଶୂନ୍ୟ visit(AstNetlist* nodep) override {  // FindVisitor::
        // Process $unit or other packages
        // Not needed - dotted references not allowed from inside packages
        // for (AstNodeModule* nodep = v3Global.rootp()->modulesp();
        //     nodep; nodep=VN_AS(nodep->nextp(), NodeModule)) {
        //    if (VN_IS(nodep, Package)) {}}

        m_statep->ଭର୍ତ୍ତି_DUnit(nodep);

        // First back iterate, to find all packages. Backward as must do base
        // packages before using packages
        iterateChildrenBackwardsConst(nodep);

        // The first modules in the list are always the top modules
        // (sorted before this is called).
        // This may not be the module with isTop() set, as early in the steps,
        // wrapTop may have not been created yet.
        // $unit and the constant pool always exist, so nothing else, and nothing in $unit,
        // means nothing was given
        Ast_ଗଣ୍ଠି_Module* ଅଚଳ modulesp = nodep->modulesp();
        UASSERT_OBJ(modulesp, nodep, "$unit should always be in the netlist");
        ଯଦି (m_statep->forPrimary()) {
            ଅଚଳ ତାର୍କିକ onlyBuiltins = [&]() {
                ପାଇଁ (Ast_ଗଣ୍ଠି_Module* modp = modulesp; modp;
                     modp = VN_AS(modp->nextp(), ଗଣ୍ଠି_Module)) {
                    ଯଦି (!modp->isDollarUnit() && !modp->isConstPool()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
                }
                ଫେରନ୍ତୁ ସତ୍ୟ;
            }();
            ଯଦି (onlyBuiltins && !v3Global.rootp()->dollarUnitPkgp()->stmtsp()) {
                nodep->v3error("No top level module found");
            }
        }
        ପାଇଁ (Ast_ଗଣ୍ଠି_Module* modp = nodep->modulesp(); modp && modp->is_ଶୀର୍ଷ();
             modp = VN_AS(modp->nextp(), ଗଣ୍ଠି_Module)) {
            UINFO(8, "Top Module: " << modp);
            m_scope = "TOP";

            ଯଦି (!m_statep->forScopeCreation() && v3Global.opt.ଶୀର୍ଷ_Ifaces_Supported()) {
                ପାଇଁ (Ast_ଗଣ୍ଠି* subnodep = modp->stmtsp(); subnodep; subnodep = subnodep->nextp()) {
                    ଯଦି (AstVar* ଅଚଳ varp = VN_CAST(subnodep, Var)) {
                        ଯଦି (varp->isIfaceRef()) {
                            ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ subtypep = varp->subDTypep();
                            ଅଚଳ AstIfaceRefDType* ifacerefp = nullptr;
                            ଯଦି (VN_IS(subtypep, IfaceRefDType)) {
                                ifacerefp = VN_AS(varp->subDTypep(), IfaceRefDType);
                            } ଅନ୍ୟଥା ଯଦି (VN_IS(subtypep, BracketArrayDType)) {
                                ଅଚଳ AstBracketArrayDType* ଅଚଳ arrp
                                    = VN_AS(subtypep, BracketArrayDType);
                                ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ arrsubtypep = arrp->subDTypep();
                                ଯଦି (VN_IS(arrsubtypep, IfaceRefDType)) {
                                    ifacerefp = VN_AS(arrsubtypep, IfaceRefDType);
                                }
                            } ଅନ୍ୟଥା ଯଦି (VN_IS(subtypep, UnpackArrayDType)) {
                                ଅଚଳ AstUnpackArrayDType* ଅଚଳ arrp
                                    = VN_AS(subtypep, UnpackArrayDType);
                                ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ arrsubtypep = arrp->subDTypep();
                                ଯଦି (VN_IS(arrsubtypep, IfaceRefDType)) {
                                    ifacerefp = VN_AS(arrsubtypep, IfaceRefDType);
                                }
                            }

                            ଯଦି (ifacerefp && !ifacerefp->cellp()) {
                                // A dummy cell to keep the top level interface alive and correctly
                                // optimized for default parameter values
                                AstCell* ifacecellp
                                    = ନୂତନ AstCell{nodep->fileline(),
                                                  nodep->fileline(),
                                                  modp->name() + "__02E" + varp->name(),
                                                  ifacerefp->ifaceName(),
                                                  nullptr,
                                                  nullptr,
                                                  nullptr};
                                ifacecellp->modp(ifacerefp->ifacep());
                                m_curSymp = m_modSymp
                                    = m_statep->ଭର୍ତ୍ତି_ଶୀର୍ଷ_Iface(ifacecellp, m_scope);
                                iterate(ifacecellp);
                                VL_DO_DANGLING(ଠେଲ_Deletep(ifacecellp), ifacecellp);
                            }
                        }
                    }
                }
            }

            m_curSymp = m_modSymp = m_statep->ଭର୍ତ୍ତି_ଶୀର୍ଷ_Cell(modp, m_scope);
            { iterate(modp); }

            m_scope = "";
            m_curSymp = m_modSymp = nullptr;
        }
    }
    ଶୂନ୍ୟ visit(AstTypeTable*) override {}  // FindVisitor::
    ଶୂନ୍ୟ visit(AstIfaceRefDType* nodep) override {  // FindVisitor::
        ଯଦି (!m_statep->forScopeCreation() && nodep->isVirtual() && nodep->ifacep()
            && !nodep->ifacep()->user3()) {
            m_virtIfaces.ଠେଲ_back(nodep->ifacep());
            nodep->ifacep()->user3(ସତ୍ୟ);
        }
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Module* nodep) override {  // FindVisitor::
        // Called on top module from Netlist, other modules from the cell creating them,
        // and packages
        UINFO(8, "   " << nodep);
        // m_curSymp/m_modSymp maybe nullptr for packages and non-top modules
        // Packages will be under top after the initial phases, but until then
        // need separate handling
        ଅଚଳ ତାର୍କିକ standalonePkg
            = !m_modSymp && !m_statep->forScopeCreation() && VN_IS(nodep, Package);
        ଅଚଳ ତାର୍କିକ doit = (m_modSymp || standalonePkg);
        VL_RESTORER_COPY(m_scope);
        VL_RESTORER(m_classOrPackagep);
        VL_RESTORER(m_modSymp);
        VL_RESTORER(m_curSymp);
        VL_RESTORER(m_param_ଅଙ୍କ);
        VL_RESTORER(m_mod_Block_ଅଙ୍କ);
        VL_RESTORER(m_mod_With_ଅଙ୍କ);
        VL_RESTORER(m_mod_Arg_ଅଙ୍କ);
        ଯଦି (doit && nodep->user2()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Identically recursive module (module instantiates "
                          "itself, without changing parameters): "
                              << Ast_ଗଣ୍ଠି::prettyNameQ(nodep->origName()));
        } ଅନ୍ୟଥା ଯଦି (doit) {
            UINFO(4, "     Link Module: " << nodep);
            UASSERT_OBJ(!nodep->dead(), nodep, "Module in instance tree mislabeled as dead?");
            AstPackage* ଅଚଳ pkgp = VN_CAST(nodep, Package);
            m_classOrPackagep = pkgp;
            ଯଦି (standalonePkg) {
                ଯଦି (pkgp->isDollarUnit()) {
                    m_curSymp = m_modSymp = m_statep->dunitEntp();
                    nodep->user1p(m_curSymp);
                } ଅନ୍ୟଥା {
                    VSymEnt* ଅଚଳ upperSymp = m_statep->dunitEntp();
                    m_scope = nodep->name();
                    m_curSymp = m_modSymp = m_statep->ଭର୍ତ୍ତି_Block(upperSymp, nodep->name(), nodep,
                                                                  m_classOrPackagep);
                    UINFO(9, "New module scope " << m_curSymp);
                }
            }
            //
            m_param_ଅଙ୍କ = 0;
            m_mod_Block_ଅଙ୍କ = 0;
            m_mod_With_ଅଙ୍କ = 0;
            m_mod_Arg_ଅଙ୍କ = 0;
            // m_modSymp/m_curSymp for non-packages set by AstCell above this module
            // Iterate
            nodep->user2(ସତ୍ୟ);
            iterateChildren(nodep);
            nodep->user2(ମିଥ୍ୟା);
            nodep->user4(ସତ୍ୟ);
            // Interfaces need another pass when signals are resolved. When creating
            // scopes this is done from the AstScopes in LinkDotScopeVisitor instead, as
            // only those entries hold the AstVarScopes the modport items resolve against.
            ଯଦି (!m_statep->forScopeCreation()) {
                ଯଦି (AstIface* ଅଚଳ ifacep = VN_CAST(nodep, Iface)) {
                    m_statep->ଭର୍ତ୍ତି_Iface_Mod_Sym(ifacep, m_curSymp);
                }
            }
        } ଅନ୍ୟଥା ଯଦି (isHierBlockWrapper(nodep->name())) {
            UINFO(5, "Module is hierarchical block, must not be dead: " << nodep);
            m_scope = nodep->name();
            VSymEnt* ଅଚଳ upperSymp = m_curSymp ? m_curSymp : m_statep->ମୂଳ_Entp();
            m_curSymp = m_modSymp
                = m_statep->ଭର୍ତ୍ତି_Block(upperSymp, nodep->name() + "::", nodep, m_classOrPackagep);
            iterateChildren(nodep);
            nodep->user4(ସତ୍ୟ);
        } ଅନ୍ୟଥା {  // !doit
            ଯଦି (nodep->hierParams()) {
                UINFO(1, "Found module with hier type parameters");
                m_hierParamsName = nodep->name();
                ପାଇଁ (ଅଚଳ Ast_ଗଣ୍ଠି* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    ଯଦି (ଅଚଳ AstTypedef* ଅଚଳ tdef = VN_CAST(stmtp, Typedef)) {
                        UINFO(1, "Inserting hier type parameter typedef: " << tdef);
                        VSymEnt* ଅଚଳ upperSymp = m_curSymp ? m_curSymp : m_statep->ମୂଳ_Entp();
                        m_curSymp = m_modSymp = m_statep->ଭର୍ତ୍ତି_Block(upperSymp, nodep->name(),
                                                                      nodep, m_classOrPackagep);
                    }
                }
            } ଅନ୍ୟଥା {
                // Will be optimized away later
                // Can't remove now, as our backwards iterator will throw up
                UINFO(5, "Module not under any CELL or top - dead module: " << nodep);
            }
        }
    }

    ଶୂନ୍ୟ visit(AstClass* nodep) override {  // FindVisitor::
        UASSERT_OBJ(m_curSymp, nodep, "Class not under module/package/$unit");
        UINFO(8, "   " << nodep);
        VL_RESTORER_COPY(m_scope);
        VL_RESTORER(m_classOrPackagep);
        VL_RESTORER(m_modSymp);
        VL_RESTORER(m_curSymp);
        VL_RESTORER(m_param_ଅଙ୍କ);
        VL_RESTORER(m_mod_Block_ଅଙ୍କ);
        VL_RESTORER(m_mod_With_ଅଙ୍କ);
        VL_RESTORER(m_mod_Arg_ଅଙ୍କ);
        VL_RESTORER(m_explicitNew);
        {
            UINFO(4, "     Link Class: " << nodep);
            VSymEnt* ଅଚଳ upperSymp = m_curSymp;
            m_scope = m_scope + "." + nodep->name();
            m_classOrPackagep = nodep;
            m_curSymp = m_modSymp
                = m_statep->ଭର୍ତ୍ତି_Block(upperSymp, nodep->name(), nodep, m_classOrPackagep);
            m_statep->ଭର୍ତ୍ତି_Map(m_curSymp, m_scope);
            UINFO(9, "New module scope " << m_curSymp);
            //
            m_param_ଅଙ୍କ = 0;
            m_mod_Block_ଅଙ୍କ = 0;
            m_mod_With_ଅଙ୍କ = 0;
            m_mod_Arg_ଅଙ୍କ = 0;
            m_explicitNew = ମିଥ୍ୟା;
            // m_modSymp/m_curSymp for non-packages set by AstCell above this module
            // Iterate
            iterateChildren(nodep);
            nodep->user4(ସତ୍ୟ);
            // Implicit new needed?
            ଯଦି (!m_explicitNew && m_statep->forPrimary()) makeImplicitNew(nodep);
        }
    }
    ଶୂନ୍ୟ visit(AstClassOrPackageRef* nodep) override {  // FindVisitor::
        ଯଦି (!nodep->classOrPackageNodep() && nodep->name() == "$unit") {
            nodep->classOrPackageNodep(v3Global.rootp()->dollarUnitPkgp());
        }
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstScope* nodep) override {  // FindVisitor::
        UASSERT_OBJ(m_statep->forScopeCreation(), nodep,
                    "Scopes should only exist right after V3Scope");
        // Detach on first encounter, will re add at the end.
        // Otherwise would be visited O(cells^2) time after V3Scope
        ଯଦି (!nodep->is_ଶୀର୍ଷ()) {
            ++m_statScopeVisits;
            nodep->unlinkFrBack();
            m_scopes.ଠେଲ_back(nodep);
        }
    }
    ଶୂନ୍ୟ visit(AstCell* nodep) override {  // FindVisitor::
        UINFO(5, "   CELL under " << m_scope << " is " << nodep);
        // Process XREFs/etc inside pins
        ଯଦି (nodep->recursive() && m_inRecursion) ଫେରନ୍ତୁ;
        iterateChildren(nodep);
        // Recurse in, preserving state
        VL_RESTORER_COPY(m_scope);
        VL_RESTORER(m_modSymp);
        VL_RESTORER(m_curSymp);
        VL_RESTORER(m_param_ଅଙ୍କ);
        VL_RESTORER(m_inRecursion);
        // Where do we add it?
        VSymEnt* aboveSymp = m_curSymp;
        ଅଚଳ string origname = Ast_ଗଣ୍ଠି::dedotName(nodep->name());
        string::size_type pos = origname.rfind('.');
        ଯଦି (pos != string::npos) {
            // Flattened, find what CellInline it should live under
            ଅଚଳ string scope = origname.substr(0, pos);
            string baddot;
            VSymEnt* okSymp;
            aboveSymp
                = m_statep->findDotted(nodep->fileline(), aboveSymp, scope, baddot, okSymp, ମିଥ୍ୟା);
            UASSERT_OBJ(aboveSymp, nodep,
                        "Can't find instance insertion point at "
                            << Ast_ଗଣ୍ଠି::prettyNameQ(baddot) << " in: " << nodep->prettyNameQ());
        }
        {
            m_scope = m_scope + "." + nodep->name();
            m_curSymp = m_modSymp = m_statep->ଭର୍ତ୍ତି_Cell(aboveSymp, m_modSymp, nodep, m_scope);
            m_inRecursion = nodep->recursive();
            // We don't report NotFoundModule, as may be a unused module in a generate
            ଯଦି (nodep->modp()) iterate(nodep->modp());
        }
    }
    ଶୂନ୍ୟ visit(AstCellInline* nodep) override {  // FindVisitor::
        UINFO(5, "   CELLINLINE under " << m_scope << " is " << nodep);
        VSymEnt* aboveSymp = m_curSymp;
        std::string ident = nodep->name();
        // If baz__DOT__foo__DOT__bar, we need to find baz__DOT__foo and add bar to it.
        ଅଚଳ std::string::size_type pos = ident.rfind("__DOT__");
        ଯଦି (pos != std::string::npos) {
            ଅଚଳ std::string dotted = ident.substr(0, pos);
            ident = ident.substr(pos + std::strlen("__DOT__"));
            std::string baddot;
            VSymEnt* okSymp;
            aboveSymp = m_statep->findDotted(nodep->fileline(), aboveSymp, dotted, baddot, okSymp,
                                             ମିଥ୍ୟା);
            UASSERT_OBJ(aboveSymp, nodep,
                        "Can't find cellinline insertion point at "
                            << Ast_ଗଣ୍ଠି::prettyNameQ(baddot) << " in: " << nodep->prettyNameQ());
        }
        m_statep->ଭର୍ତ୍ତି_Inline(aboveSymp, m_modSymp, nodep, ident);
    }
    ଶୂନ୍ୟ visit(AstDefParam* nodep) override {  // FindVisitor::
        nodep->user1p(m_curSymp);
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstRefDType* nodep) override {  // FindVisitor::
        nodep->user1p(m_curSymp);
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstGenBlock* nodep) override {  // FindVisitor::
        UINFO(5, "   " << nodep);
        ଯଦି (nodep->name() == "" && nodep->unnamed()) {
            // Unnamed blocks are only important when they contain var
            // decls, so search for them. (Otherwise adding all the
            // unnamed#'s would just confuse tracing variables in
            // places such as tasks, where "task ...; begin ... end"
            // are common.
            ପାଇଁ (Ast_ଗଣ୍ଠି* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
                ଯଦି (VN_IS(itemp, Var) || VN_IS(itemp, Foreach)) {
                    std::string name;
                    ଅଚଳ std::string stepStr = m_statep->forPrimary()
                                                    ? ""
                                                    : std::to_string(m_statep->step_ସଂଖ୍ୟା()) + "_";
                    କର {
                        ++m_mod_Block_ଅଙ୍କ;
                        name = "unnamedblk" + stepStr + cvtToStr(m_mod_Block_ଅଙ୍କ);
                        // Increment again if earlier pass of V3LinkDot claimed this name
                    } ଯେତେବେଳେ (m_curSymp->findIdFlat(name));
                    nodep->name(name);
                    ଭାଙ୍ଗ;
                }
            }
        }
        ଯଦି (nodep->name() == "") {
            iterateChildren(nodep);
        } ଅନ୍ୟଥା {
            VL_RESTORER(m_curSymp);
            {
                m_curSymp
                    = m_statep->ଭର୍ତ୍ତି_Block(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
                m_curSymp->fallbackp(VL_RESTORER_PREV(m_curSymp));
                // Iterate
                iterateChildren(nodep);
            }
        }
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Block* nodep) override {  // FindVisitor::
        UINFO(5, "   " << nodep);
        ଯଦି (nodep->name() == "" && nodep->unnamed()) {
            // Unnamed blocks are only important when they contain var
            // decls, so search for them. (Otherwise adding all the
            // unnamed#'s would just confuse tracing variables in
            // places such as tasks, where "task ...; begin ... end"
            // are common.
            ଅଚଳ auto containsDecl = [](ଅଚଳ Ast_ଗଣ୍ଠି* nodesp) -> ତାର୍କିକ {
                ପାଇଁ (ଅଚଳ Ast_ଗଣ୍ଠି* np = nodesp; np; np = np->nextp()) {
                    ଯଦି (VN_IS(np, Var) || VN_IS(np, Foreach)) ଫେରନ୍ତୁ ସତ୍ୟ;
                }
                ଫେରନ୍ତୁ ମିଥ୍ୟା;
            };
            ତାର୍କିକ needName = nodep->declsp() || containsDecl(nodep->stmtsp());
            ଯଦି (ଅଚଳ AstFork* ଅଚଳ forkp = VN_CAST(nodep, Fork)) {
                needName = needName || containsDecl(forkp->forksp());
            }
            ଯଦି (needName) {
                ଅଚଳ std::string stepStr
                    = m_statep->forPrimary() ? "" : std::to_string(m_statep->step_ସଂଖ୍ୟା()) + "_";
                std::string name;
                କର {
                    ++m_mod_Block_ଅଙ୍କ;
                    name = "unnamedblk" + stepStr + cvtToStr(m_mod_Block_ଅଙ୍କ);
                    // Increment again if earlier pass of V3LinkDot claimed this name
                } ଯେତେବେଳେ (m_curSymp->findIdFlat(name));
                nodep->name(name);
            }
        }
        ଯଦି (nodep->name() == "") {
            iterateChildren(nodep);
        } ଅନ୍ୟଥା {
            VL_RESTORER(m_curSymp);
            {
                m_curSymp
                    = m_statep->ଭର୍ତ୍ତି_Block(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
                m_curSymp->fallbackp(VL_RESTORER_PREV(m_curSymp));
                // Iterate
                iterateChildren(nodep);
            }
        }
    }
    // Handle out-of-block interface method definition (IEEE 1800-2023 25.7)
    // Move task body from module into the interface's prototype task,
    // rewriting port-prefixed references in the body.
    ଶୂନ୍ୟ moveIfaceExportBody(Ast_ଗଣ୍ଠି_FTask* nodep) {
        ଅଚଳ string& portName = nodep->ifacePortName();
        UINFO(5, "   moveIfaceExportBody: port=" << portName << " task=" << nodep->name());
        // Look up the interface port variable in the module's symbol table
        VSymEnt* ଅଚଳ portSymp = m_modSymp->findIdFallback(portName);
        ଯଦି (!portSymp) {
            nodep->v3error("Interface port not found for out-of-block definition: '"
                           << portName << "' (IEEE 1800-2023 25.7)");
            nodep->unlinkFrBack();
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
            ଫେରନ୍ତୁ;
        }
        AstVar* ଅଚଳ portp = VN_CAST(portSymp->nodep(), Var);
        ଯଦି (!portp) {
            nodep->v3fatalSrc("Out-of-block definition port is not a variable: " << portName);
            ଫେରନ୍ତୁ;
        }
        // Get the interface name from the port's type
        // At this stage dtypep() may not be set yet; check childDTypep() too
        AstIfaceRefDType* ifaceRefDtp = nullptr;
        ଯଦି (portp->dtypep()) { ifaceRefDtp = VN_CAST(portp->dtypep(), IfaceRefDType); }
        ଯଦି (!ifaceRefDtp && portp->childDTypep()) {
            ifaceRefDtp = VN_CAST(portp->childDTypep(), IfaceRefDType);
        }
        ଯଦି (!ifaceRefDtp) {
            nodep->v3error("Out-of-block definition port is not an interface port: '"
                           << portName << "' (IEEE 1800-2023 25.7)");
            nodep->unlinkFrBack();
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
            ଫେରନ୍ତୁ;
        }
        ଅଚଳ string ifaceName = ifaceRefDtp->ifaceName();
        UINFO(5, "   moveIfaceExportBody: iface=" << ifaceName);
        // Find the interface module by name
        Ast_ଗଣ୍ଠି_Module* ଅଚଳ ifaceModp = m_statep->findModuleSym(ifaceName);
        ଯଦି (!ifaceModp) {
            nodep->v3fatalSrc("Interface not found for out-of-block definition: " << ifaceName);
            ଫେରନ୍ତୁ;
        }
        // Find the prototype task in the interface
        Ast_ଗଣ୍ଠି_FTask* protoTaskp = nullptr;
        ପାଇଁ (Ast_ଗଣ୍ଠି* itemp = ifaceModp->stmtsp(); itemp; itemp = itemp->nextp()) {
            ଯଦି (Ast_ଗଣ୍ଠି_FTask* ଅଚଳ ftaskp = VN_CAST(itemp, ଗଣ୍ଠି_FTask)) {
                ଯଦି (ftaskp->name() == nodep->name() && ftaskp->prototype()) {
                    protoTaskp = ftaskp;
                    ଭାଙ୍ଗ;
                }
            }
        }
        ଯଦି (!protoTaskp) {
            nodep->v3error("No matching export prototype found in interface "
                           << ifaceName << " for task " << nodep->prettyNameQ()
                           << " (IEEE 1800-2023 25.7)");
            nodep->unlinkFrBack();
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
            ଫେରନ୍ତୁ;
        }
        // Move body statements (non-port declarations) from the out-of-block task
        // to the prototype task. Rewrite port.X references to just X in the body.
        ପାଇଁ (Ast_ଗଣ୍ଠି* stmtp = nodep->stmtsp(); stmtp;) {
            Ast_ଗଣ୍ଠି* ଅଚଳ nextp = stmtp->nextp();
            // Skip port declarations -- they already exist in the prototype
            ଯଦି (ଅଚଳ AstVar* ଅଚଳ varp = VN_CAST(stmtp, Var)) {
                ଯଦି (varp->isIO()) {
                    stmtp = nextp;
                    ଜାରି;
                }
            }
            // Clone this statement and rewrite port references
            Ast_ଗଣ୍ଠି* ଅଚଳ newStmtp = stmtp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);
            rewriteIfacePortRefsSingle(newStmtp, portName);
            protoTaskp->addStmtsp(newStmtp);
            stmtp = nextp;
        }
        // Mark prototype as no longer prototype (it has a body now)
        protoTaskp->prototype(ମିଥ୍ୟା);
        // Delete the out-of-block task from the module
        nodep->unlinkFrBack();
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
    }

    // Rewrite AstDot nodes where lhsp is AstParseRef with the port name
    // to just the rhsp (stripping the port prefix).
    ସ୍ଥିର ଶୂନ୍ୟ rewriteIfacePortRefsSingle(Ast_ଗଣ୍ଠି* nodep, ଅଚଳ string& portName) {
        ଯଦି (!nodep) ଫେରନ୍ତୁ;
        // Check if this is a non-colon Dot with lhs matching portName
        ଯଦି (AstDot* ଅଚଳ dotp = VN_CAST(nodep, Dot)) {
            ଯଦି (!dotp->colon()) {
                ଅଚଳ AstParseRef* ଅଚଳ lhsRefp = VN_CAST(dotp->lhsp(), ParseRef);
                ଯଦି (lhsRefp && lhsRefp->name() == portName) {
                    UINFO(5, "   rewriteIfacePortRefs: stripping port prefix from " << dotp);
                    Ast_ଗଣ୍ଠି* ଅଚଳ rhsp = dotp->rhsp()->unlinkFrBack();
                    dotp->replaceWith(rhsp);
                    VL_DO_DANGLING(dotp->delete_ବୃକ୍ଷ(), dotp);
                    rewriteIfacePortRefsSingle(rhsp, portName);
                    ଫେରନ୍ତୁ;
                }
            }
        }
        // Recurse into all children
        ପାଇଁ (Ast_ଗଣ୍ଠି* childp : {nodep->op1p(), nodep->op2p(), nodep->op3p(), nodep->op4p()}) {
            ଯେତେବେଳେ (childp) {
                Ast_ଗଣ୍ଠି* ଅଚଳ nextp = childp->nextp();
                rewriteIfacePortRefsSingle(childp, portName);
                childp = nextp;
            }
        }
    }

    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_FTask* nodep) override {  // FindVisitor::
        // NodeTask: Remember its name for later resolution
        UINFO(5, "   " << nodep);
        UASSERT_OBJ(m_curSymp && m_modSymp, nodep, "Function/Task not under module?");
        ଯଦି (nodep->name() == "new") m_explicitNew = ସତ୍ୟ;
        // Handle out-of-block interface method definition (IEEE 25.8)
        ଯଦି (!nodep->ifacePortName().empty() && m_statep->forPrimary()) {
            moveIfaceExportBody(nodep);
            ଫେରନ୍ତୁ;  // Task has been deleted
        }
        // Remember the existing symbol table scope
        VL_RESTORER(m_classOrPackagep);
        VL_RESTORER(m_curSymp);
        VSymEnt* upSymp = m_curSymp;

        ଯଦି (VN_IS(m_curSymp->nodep(), Class)) {
            ଯଦି (VN_AS(m_curSymp->nodep(), Class)->isInterfaceClass() && !nodep->pureVirtual()
                && !nodep->isConstructor()) {
                nodep->v3error("Interface class functions must be pure virtual"
                               << " (IEEE 1800-2023 8.26): " << nodep->prettyNameQ());
            }
            ଯଦି (m_statep->forPrimary()
                && (nodep->name() == "randomize" || nodep->name() == "srandom")) {
                nodep->v3error(nodep->prettyNameQ()
                               << " is a predefined class method; redefinition not allowed"
                                  " (IEEE 1800-2023 18.6.3)");
            }
        }
        // Change to appropriate package if extern declaration (vs definition)
        ଯଦି (nodep->classOrPackagep()) {
            // class-in-class
            AstDot* ଅଚଳ dotp = VN_CAST(nodep->classOrPackagep(), Dot);
            AstClassOrPackageRef* ଅଚଳ cpackagerefp
                = VN_CAST(nodep->classOrPackagep(), ClassOrPackageRef);
            ଯଦି (dotp) {
                AstClassOrPackageRef* ଅଚଳ lhsp = VN_AS(dotp->lhsp(), ClassOrPackageRef);
                m_statep->resolveClassOrPackage(m_curSymp, lhsp, ସତ୍ୟ, ମିଥ୍ୟା,
                                                "External definition :: reference");
                AstClass* ଅଚଳ lhsclassp = VN_CAST(lhsp->classOrPackageSkipp(), Class);
                ଯଦି (!lhsclassp) {
                    nodep->v3error("Extern declaration's scope is not a defined class");
                } ଅନ୍ୟଥା {
                    m_curSymp = m_statep->get_ଗଣ୍ଠି_Sym(lhsclassp);
                    upSymp = m_curSymp;
                    AstClassOrPackageRef* ଅଚଳ rhsp = VN_AS(dotp->rhsp(), ClassOrPackageRef);
                    m_statep->resolveClassOrPackage(m_curSymp, rhsp, ସତ୍ୟ, ମିଥ୍ୟା,
                                                    "External definition :: reference");
                    AstClass* ଅଚଳ rhsclassp = VN_CAST(rhsp->classOrPackageSkipp(), Class);
                    ଯଦି (!rhsclassp) {
                        nodep->v3error("Extern declaration's scope is not a defined class");
                    } ଅନ୍ୟଥା {
                        m_curSymp = m_statep->get_ଗଣ୍ଠି_Sym(rhsclassp);
                        upSymp = m_curSymp;
                        moveExternFuncDecl(nodep, rhsclassp);
                    }
                }
            } ଅନ୍ୟଥା ଯଦି (cpackagerefp) {
                ଯଦି (!cpackagerefp->classOrPackageSkipp()) {
                    m_statep->resolveClassOrPackage(m_curSymp, cpackagerefp, ସତ୍ୟ, ମିଥ୍ୟା,
                                                    "External definition :: reference");
                }
                AstClass* ଅଚଳ classp = VN_CAST(cpackagerefp->classOrPackageSkipp(), Class);
                ଯଦି (!classp) {
                    nodep->v3error("Extern declaration's scope is not a defined class");
                } ଅନ୍ୟଥା {
                    m_curSymp = m_statep->get_ଗଣ୍ଠି_Sym(classp);
                    upSymp = m_curSymp;
                    moveExternFuncDecl(nodep, classp);
                }
            } ଅନ୍ୟଥା {
                v3fatalSrc("Unhandled extern function definition package");
            }
        }
        // Set the class as package for iteration
        ଯଦି (VN_IS(m_curSymp->nodep(), Class)) {
            m_classOrPackagep = VN_AS(m_curSymp->nodep(), Class);
        }
        // Create symbol table for the task's vars
        ଅଚଳ string name = (nodep->isExternProto() ? "extern "s : ""s) + nodep->name();
        m_curSymp = m_statep->ଭର୍ତ୍ତି_Block(m_curSymp, name, nodep, m_classOrPackagep);
        m_curSymp->fallbackp(upSymp);
        // Convert the func's range to the output variable
        // This should probably be done in the Parser instead, as then we could
        // just attach normal signal attributes to it.
        ଯଦି (!nodep->isExternProto() && nodep->fvarp() && !VN_IS(nodep->fvarp(), Var)) {
            Ast_ଗଣ୍ଠି_DType* dtypep = VN_CAST(nodep->fvarp(), ଗଣ୍ଠି_DType);
            // If unspecified, function returns one bit; however when we
            // support NEW() it could also return the class reference.
            ଯଦି (dtypep) {
                dtypep->unlinkFrBack();
            } ଅନ୍ୟଥା {
                dtypep = ନୂତନ AstBasicDType{nodep->fileline(), VBasicDTypeKwd::LOGIC};
            }
            AstVar* ଅଚଳ newvarp
                = ନୂତନ AstVar{nodep->fileline(), VVarType::VAR, nodep->name(), VFlagChildDType{},
                             dtypep};  // Not dtype resolved yet
            newvarp->direction(VDirection::OUTPUT);
            newvarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            newvarp->funcReturn(ସତ୍ୟ);
            newvarp->trace(ମିଥ୍ୟା);  // Not user visible
            nodep->fvarp(newvarp);
            // Explicit insert required, as the var name shadows the upper level's task name
            m_statep->ଭର୍ତ୍ତି_Sym(m_curSymp, newvarp->name(), newvarp, nullptr /*classOrPackagep*/);
        }
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstClocking* nodep) override {  // FindVisitor::
        VL_RESTORER(m_clockingp);
        m_clockingp = nodep;
        iterate(nodep->sentreep());
        iterateAndNextNull(nodep->itemsp());
        // If the block has no name, one cannot reference the clockvars
        VSymEnt* itSymp = nullptr;
        ଯଦି (nodep->isGlobal()  //
            && m_statep->forPrimary()) {  // else flattening may see two globals
            m_statep->checkDuplicate(m_curSymp, nodep, "__024global_clock");
            itSymp
                = m_statep->ଭର୍ତ୍ତି_Block(m_curSymp, "__024global_clock", nodep, m_classOrPackagep);
            itSymp->fallbackp(nullptr);
        }
        ଯଦି (!nodep->name().empty()) {
            itSymp = m_statep->ଭର୍ତ୍ତି_Block(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
            itSymp->fallbackp(nullptr);
        }
        ଯଦି (itSymp) {
            VL_RESTORER(m_curSymp);
            m_curSymp = itSymp;
            iterateAndNextNull(nodep->itemsp());
        }
    }
    ଶୂନ୍ୟ visit(AstClockingItem* nodep) override {  // FindVisitor::
        ଯଦି (nodep->varp()) {
            ଯଦି (m_curSymp->nodep() == m_clockingp) iterate(nodep->varp());
            ଫେରନ୍ତୁ;
        }
        std::string varname;
        Ast_ଗଣ୍ଠି_DType* dtypep;
        ଯଦି (AstAssign* ଅଚଳ assignp = nodep->assignp()) {
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rhsp = assignp->rhsp()->unlinkFrBack();
            dtypep = ନୂତନ AstRefDType{nodep->fileline(), AstRefDType::Flag_Type_Of_ପଦସମଷ୍ଟି{},
                                     rhsp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା)};
            nodep->exprp(rhsp);
            varname = assignp->lhsp()->name();
            VL_DO_DANGLING(assignp->unlinkFrBack()->delete_ବୃକ୍ଷ(), assignp);
        } ଅନ୍ୟଥା {
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ refp = nodep->exprp();
            ଅଚଳ VSymEnt* foundp = m_curSymp->findIdFallback(refp->name());
            ଯଦି (!foundp || !foundp->nodep()) {
                refp->v3error("Corresponding variable " << refp->prettyNameQ()
                                                        << " does not exist");
                VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ବୃକ୍ଷ(), nodep);
                ଫେରନ୍ତୁ;
            }
            varname = refp->name();
            dtypep = VN_AS(foundp->nodep(), Var)->childDTypep()->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);
        }
        AstVar* ଅଚଳ newvarp = ନୂତନ AstVar{nodep->fileline(), VVarType::MODULETEMP, varname,
                                           VFlagChildDType{}, dtypep};
        newvarp->lifetime(VLifetime::STATIC_EXPLICIT);
        nodep->varp(newvarp);
        iterate(nodep->exprp());
    }
    ଶୂନ୍ୟ visit(AstConstraint* nodep) override {  // FindVisitor::
        VL_RESTORER(m_curSymp);
        // Change to appropriate package if extern declaration (vs definition)
        VSymEnt* upSymp = m_curSymp;
        ଯଦି (nodep->classOrPackagep()) {
            AstClassOrPackageRef* ଅଚଳ cpackagerefp
                = VN_CAST(nodep->classOrPackagep(), ClassOrPackageRef);
            ଯଦି (!cpackagerefp) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: extern constraint definition with class-in-class");
            } ଅନ୍ୟଥା {
                ଯଦି (!cpackagerefp->classOrPackageSkipp()) {
                    m_statep->resolveClassOrPackage(m_curSymp, cpackagerefp, ସତ୍ୟ, ମିଥ୍ୟା,
                                                    "External definition :: reference");
                }
                AstClass* ଅଚଳ classp = VN_CAST(cpackagerefp->classOrPackageSkipp(), Class);
                ଯଦି (!classp) {
                    nodep->v3error("Extern declaration's scope is not a defined class");
                } ଅନ୍ୟଥା {
                    m_curSymp = m_statep->get_ଗଣ୍ଠି_Sym(classp);
                    upSymp = m_curSymp;
                    // Move it to proper spot under the target class
                    nodep->unlinkFrBack();
                    classp->addStmtsp(nodep);
                    nodep->classOrPackagep()->unlinkFrBack()->delete_ବୃକ୍ଷ();
                }
            }
        }
        // Set the class as package for iteration
        ଅଚଳ string name = (nodep->isExternProto() ? "extern "s : ""s) + nodep->name();
        m_curSymp = m_statep->ଭର୍ତ୍ତି_Block(upSymp, name, nodep, m_classOrPackagep);
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstVar* nodep) override {  // FindVisitor::
        // Var: Remember its name for later resolution
        UASSERT_OBJ(m_curSymp && m_modSymp, nodep, "Var not under module?");
        iterateChildren(nodep);
        ଯଦି (VN_IS(m_curSymp->nodep(), Class)
            && VN_AS(m_curSymp->nodep(), Class)->isInterfaceClass() && !nodep->isParam()) {
            nodep->v3error("Interface class cannot contain non-parameter members"
                           << " (IEEE 1800-2023 8.26): " << nodep->prettyNameQ());
        }
        ଯଦି (!m_statep->forScopeCreation()) {
            // Find under either a task or the module's vars
            ଅଚଳ VSymEnt* foundp = m_curSymp->findIdFallback(nodep->name());
            ଯଦି (!foundp && m_modSymp && nodep->name() == m_modSymp->nodep()->name()) {
                foundp = m_modSymp;  // Conflicts with modname?
            }
            AstVar* ଅଚଳ findvarp = foundp ? VN_CAST(foundp->nodep(), Var) : nullptr;
            // clocking items can have duplicate names (inout)
            ଯଦି (findvarp && VN_IS(findvarp->backp(), ClockingItem)
                && VN_IS(nodep->backp(), ClockingItem)) {
                AstClockingItem* ଅଚଳ itemp = VN_AS(nodep->backp(), ClockingItem);
                AstClockingItem* ଅଚଳ finditemp = VN_AS(findvarp->backp(), ClockingItem);
                UINFO(4, "ClockCompl: " << itemp << " ;; " << finditemp);
                UINFO(4, "ClockCompV: " << nodep << " ;; " << findvarp);
                ଯଦି (*itemp->exprp()->fileline() == *finditemp->exprp()->fileline()) {
                    UASSERT_OBJ(finditemp->direction() == VDirection::ନିବେଶ
                                    && itemp->direction() == VDirection::OUTPUT,
                                itemp, "Input after output?");
                    // pretend nothing found and rename
                    foundp = nullptr;
                    nodep->name("__Voutput_" + nodep->name());
                    finditemp->outputp(itemp);
                }
            }
            ତାର୍କିକ ins = ମିଥ୍ୟା;
            ଯଦି (!foundp) {
                ins = ସତ୍ୟ;
            } ଅନ୍ୟଥା ଯଦି (!findvarp && m_curSymp->findIdFlat(nodep->name()) && !foundp->imported()) {
                nodep->v3error("Unsupported in C: Variable has same name as "
                               << LinkDotState::ଗଣ୍ଠି_Text_Type(foundp->nodep()) << ": "
                               << nodep->prettyNameQ());
            } ଅନ୍ୟଥା ଯଦି (findvarp != nodep) {
                UINFO(4, "DupVar: " << nodep << " ;; " << foundp->nodep());
                UINFO(4, "    found  cur=se" << cvtToHex(m_curSymp) << " ;; parent=se"
                                             << cvtToHex(foundp->parentp()));
                ଯଦି (foundp->parentp() == m_curSymp  // Only when on same level
                    && !foundp->imported()) {  // and not from package
                    ଯଦି (VN_IS(m_curSymp->nodep(), Clocking)) {
                        nodep->v3error("Multiple clockvars with the same name not allowed");
                        ଫେରନ୍ତୁ;
                    }
                    ଅଚଳ ତାର୍କିକ nansiBad
                        = (((findvarp->isDeclTyped() || findvarp->isNet())
                            && (nodep->isDeclTyped() || nodep->isNet()))
                           || (findvarp->isIO() && nodep->isIO())  // e.g. !(output && output)
                           || findvarp->gotNansiType());  // e.g. int x && int x
                    ଅଚଳ ତାର୍କିକ ansiBad
                        = findvarp->isAnsi() || nodep->isAnsi();  // dup illegal with ANSI
                    ଯଦି (ansiBad || nansiBad) {
                        ତାର୍କିକ ansiWarn = ansiBad && !nansiBad;
                        ଯଦି (ansiWarn) {
                            ସ୍ଥିର ପୂର୍ଣ୍ଣସଂଖ୍ୟା s_didAnsiWarn = ମିଥ୍ୟା;
                            ଯଦି (s_didAnsiWarn++) ansiWarn = ମିଥ୍ୟା;
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
                    } ଅନ୍ୟଥା {
                        findvarp->combineType(nodep);
                        findvarp->fileline()->modifyStateInherit(nodep->fileline());
                        findvarp->gotNansiType(ସତ୍ୟ);
                        UASSERT_OBJ(nodep->subDTypep(), nodep, "Var has no type");
                        ଯଦି (nodep->subDTypep()->numeric().isSigned()
                            && !findvarp->subDTypep()->numeric().isSigned()) {
                            findvarp->subDTypep()->numeric(VSigning{ସତ୍ୟ});
                        }
                        Ast_ଗଣ୍ଠି_DType* ଅଚଳ varDtp = findvarp->subDTypep();
                        Ast_ଗଣ୍ଠି_DType* ଅଚଳ otherDtp = nodep->subDTypep();
                        ଯଦି (varDtp
                            && (VN_IS(varDtp, BasicDType)
                                && VN_AS(varDtp, BasicDType)->implicit())) {
                            // Then have "input foo" and "real foo" so the
                            // dtype comes from the other side.
                            VL_DO_DANGLING(varDtp->unlinkFrBack()->delete_ବୃକ୍ଷ(), varDtp);
                            findvarp->childDTypep(otherDtp->unlinkFrBack());
                        } ଅନ୍ୟଥା ଯଦି (!m_statep->forScopeCreation() && otherDtp && varDtp
                                   && !(VN_IS(otherDtp, BasicDType)
                                        && VN_AS(otherDtp, BasicDType)->implicit())) {
                            // otherDtp and varDtp both non-nullptr and neither are implicit
                            // Can't compare dtypes now as might contain parameters,
                            // defer to V3Width
                            AstAttrOf* ଅଚଳ newp
                                = ନୂତନ AstAttrOf{otherDtp->fileline(), VAttrType::VAR_PORT_DTYPE,
                                                otherDtp->unlinkFrBack()};
                            findvarp->addAttrsp(newp);
                        }
                    }
                    VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ବୃକ୍ଷ(), nodep);
                } ଅନ୍ୟଥା {
                    // User can disable the message at either point
                    ଯଦି (!(m_ftaskp && m_ftaskp->dpiImport())
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
                    ins = ସତ୍ୟ;
                }
            }
            ଯଦି (ins) {
                ଯଦି (m_statep->forPrimary() && nodep->isGParam()
                    && VN_IS(m_modSymp->nodep(), Module)
                    && (m_statep->ମୂଳ_Entp()->nodep() == m_modSymp->parentp()->nodep())) {
                    // This is the toplevel module. Check for command line overwrites of parameters
                    // We first search if the parameter is overwritten and then replace it with a
                    // new value.
                    ଯଦି (v3Global.opt.hasParameter(nodep->name())) {
                        ଅଚଳ string svalue = v3Global.opt.parameter(nodep->name());
                        ଯଦି (AstConst* ଅଚଳ valuep
                            = AstConst::parseParamLiteral(nodep->fileline(), svalue)) {
                            UINFO(9, "       replace parameter " << nodep);
                            UINFO(9, "       with " << valuep);
                            ଯଦି (nodep->valuep()) ଠେଲ_Deletep(nodep->valuep()->unlinkFrBack());
                            nodep->valuep(valuep);
                            // Do not warn ENUMVALUE when overriding an enum param from the cli.
                            nodep->fileline()->warnOff(V3ErrorCode::ENUMVALUE, ସତ୍ୟ);
                        }
                    }
                }
                m_statep->ଭର୍ତ୍ତି_Sym(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
                ଯଦି (m_statep->forPrimary() && nodep->isGParam()) {
                    ++m_param_ଅଙ୍କ;
                    VSymEnt* ଅଚଳ symp
                        = m_statep->ଭର୍ତ୍ତି_Sym(m_curSymp, "__paramNumber" + cvtToStr(m_param_ଅଙ୍କ),
                                              nodep, m_classOrPackagep);
                    symp->exported(ମିଥ୍ୟା);
                }
            }
        }
    }
    ଶୂନ୍ୟ visit(AstTypedef* nodep) override {  // FindVisitor::
        UASSERT_OBJ(m_curSymp, nodep, "Typedef not under module/package/$unit");
        ଯଦି (VN_IS(m_classOrPackagep, Class)) nodep->isUnderClass(ସତ୍ୟ);
        iterateChildren(nodep);
        m_statep->ଭର୍ତ୍ତି_Sym(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
    }
    ଶୂନ୍ୟ visit(AstTypedefFwd* nodep) override {  // FindVisitor::
        UASSERT_OBJ(m_curSymp, nodep, "Typedef not under module/package/$unit");
        iterateChildren(nodep);
        // No need to insert, only the real typedef matters, but need to track for errors
        nodep->user1p(m_curSymp);
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_UOr_Struct_DType* nodep) override {  // FindVisitor::
        UASSERT_OBJ(m_curSymp, nodep, "Struct/union dtype not under module/package/$unit");
        VL_RESTORER(m_curSymp);
        m_curSymp = m_statep->ଭର୍ତ୍ତି_Block(m_curSymp, "__Vdtype" + cvtToStr(nodep->unique_ଅଙ୍କ()),
                                          nodep, m_classOrPackagep);
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstMemberDType* nodep) override {  // FindVisitor::
        iterateChildren(nodep);
        m_statep->ଭର୍ତ୍ତି_Sym(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
    }
    ଶୂନ୍ୟ visit(AstParamTypeDType* nodep) override {  // FindVisitor::
        UASSERT_OBJ(m_curSymp, nodep, "Parameter type not under module/package/$unit");

        // Replace missing param types with provided hierarchical type params.
        ଯଦି (!m_hierParamsName.empty()) {
            ଯଦି (ଅଚଳ VSymEnt* ଅଚଳ typedefEntp = m_curSymp->findIdFallback(m_hierParamsName)) {
                ଅଚଳ AstModule* modp = VN_CAST(typedefEntp->nodep(), Module);

                ପାଇଁ (ଅଚଳ Ast_ଗଣ୍ଠି* stmtp = modp ? modp->stmtsp() : nullptr; stmtp;
                     stmtp = stmtp->nextp()) {
                    ଅଚଳ AstTypedef* tdefp = VN_CAST(stmtp, Typedef);

                    ଯଦି (tdefp && tdefp->name() == nodep->name() && m_statep->forPrimary()) {
                        UINFO(8, "Replacing type of" << nodep << "  -->with " << tdefp);
                        Ast_ଗଣ୍ଠି_DType* ଅଚଳ newDtp = tdefp->childDTypep();
                        Ast_ଗଣ୍ଠି_DType* ଅଚଳ oldDtp = nodep->childDTypep();

                        oldDtp->replaceWith(newDtp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା));
                        VL_DO_DANGLING(oldDtp->delete_ବୃକ୍ଷ(), oldDtp);
                    }
                }
            }
        }

        iterateChildren(nodep);
        m_statep->ଭର୍ତ୍ତି_Sym(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
        ଯଦି (m_statep->forPrimary() && nodep->isGParam()) {
            ++m_param_ଅଙ୍କ;
            VSymEnt* ଅଚଳ symp = m_statep->ଭର୍ତ୍ତି_Sym(
                m_curSymp, "__paramNumber" + cvtToStr(m_param_ଅଙ୍କ), nodep, m_classOrPackagep);
            symp->exported(ମିଥ୍ୟା);
        }
    }
    ଶୂନ୍ୟ visit(AstCFunc* nodep) override {  // FindVisitor::
        // For dotted resolution, ignore all AstVars under functions, otherwise shouldn't exist
        UASSERT_OBJ(!m_statep->forScopeCreation(), nodep, "No CFuncs expected in tree yet");
    }
    ଶୂନ୍ୟ visit(AstEnumItem* nodep) override {  // FindVisitor::
        // EnumItem: Remember its name for later resolution
        iterateChildren(nodep);
        // Find under either a task or the module's vars
        ଅଚଳ VSymEnt* foundp = m_curSymp->findIdFallback(nodep->name());
        ଯଦି (!foundp && m_modSymp && nodep->name() == m_modSymp->nodep()->name()) {
            foundp = m_modSymp;  // Conflicts with modname?
        }
        AstEnumItem* ଅଚଳ findvarp = foundp ? VN_CAST(foundp->nodep(), EnumItem) : nullptr;
        ତାର୍କିକ ins = ମିଥ୍ୟା;
        ଯଦି (!foundp) {
            ins = ସତ୍ୟ;
        } ଅନ୍ୟଥା ଯଦି (findvarp != nodep) {
            UINFO(4, "DupVar: " << nodep << " ;; " << foundp);
            ଯଦି (foundp->parentp() == m_curSymp  // Only when on same level
                && !foundp->imported()) {  // and not from package
                nodep->v3error("Duplicate declaration of enum value: "
                               << nodep->prettyName() << '\n'
                               << nodep->warnContextPrimary() << '\n'
                               << foundp->nodep()->warnOther()
                               << "... Location of original declaration\n"
                               << foundp->nodep()->warnContextSecondary());
            } ଅନ୍ୟଥା {
                // User can disable the message at either point
                ଯଦି (!nodep->fileline()->warnIsOff(V3ErrorCode::VARHIDDEN)
                    && !foundp->nodep()->fileline()->warnIsOff(V3ErrorCode::VARHIDDEN)) {
                    nodep->v3warn(VARHIDDEN,
                                  "Declaration of enum value hides declaration in upper scope: "
                                      << nodep->prettyName() << '\n'
                                      << nodep->warnContextPrimary() << '\n'
                                      << foundp->nodep()->warnOther()
                                      << "... Location of original declaration\n"
                                      << foundp->nodep()->warnContextSecondary());
                }
                ins = ସତ୍ୟ;
            }
        }
        ଯଦି (ins) m_statep->ଭର୍ତ୍ତି_Sym(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
    }
    ଶୂନ୍ୟ visit(AstPackageImport* nodep) override {  // FindVisitor::
        UINFO(4, "  Link: " << nodep);
        ଯଦି (!nodep->packagep()) ଫେରନ୍ତୁ;  // Errored in V3LinkCells
        VSymEnt* ଅଚଳ srcp = m_statep->get_ଗଣ୍ଠି_Sym(nodep->packagep());
        ଯଦି (nodep->name() == "*") {
            ଯଦି (nodep->packagep() != v3Global.rootp()->stdPackagep()) {
                ଯଦି (m_curSymp == m_statep->dunitEntp()) {
                    nodep->v3warn(IMPORTSTAR,
                                  "'import::*' in $unit scope may pollute global namespace");
                }
            }
        } ଅନ୍ୟଥା ଯଦି (!nodep->resolvedClassp()) {
            VSymEnt* ଅଚଳ impp = srcp->findIdFlat(nodep->name());
            ଯଦି (!impp) {
                nodep->v3error("Import object not found: " << nodep->prettyPkgNameQ());
            } ଅନ୍ୟଥା ଯଦି (nodep->resolvedClassp()) {
                UASSERT_OBJ(nodep->resolvedClassp()->classOrPackageSkipp() == impp->nodep(), nodep,
                            "Import should resolve to the same node");
            } ଅନ୍ୟଥା ଯଦି (AstClass* ଅଚଳ classp = VN_CAST(impp->nodep(), Class)) {
                nodep->resolvedClassp(
                    ନୂତନ AstClassOrPackageRef{nodep->fileline(), classp->name(), classp, nullptr});
            }
        }
        m_curSymp->importFromPackage(m_statep->symsp(), srcp, nodep->name());
        UINFO(9, "    Link Done: " << nodep);
        // No longer needed, but can't delete until any multi-instantiated modules are expanded
    }
    ଶୂନ୍ୟ visit(AstPackageExport* nodep) override {  // FindVisitor::
        UINFO(9, "  Link: " << nodep);
        ଯଦି (!nodep->packagep()) ଫେରନ୍ତୁ;  // Errored in V3LinkCells
        VSymEnt* ଅଚଳ srcp = m_statep->get_ଗଣ୍ଠି_Sym(nodep->packagep());
        ଯଦି (nodep->name() != "*") {
            VSymEnt* ଅଚଳ impp = srcp->findIdFlat(nodep->name());
            ଯଦି (!impp) {
                nodep->v3error("Export object not found: '" << nodep->packagep()->prettyName()
                                                            << "::" << nodep->prettyName() << "'");
            }
        }
        m_curSymp->exportFromPackage(m_statep->symsp(), srcp, nodep->name());
        UINFO(9, "    Link Done: " << nodep);
        // No longer needed, but can't delete until any multi-instantiated modules are expanded
    }
    ଶୂନ୍ୟ visit(AstPackageExportStarStar* nodep) override {  // FindVisitor::
        UINFO(4, "  Link: " << nodep);
        m_curSymp->exportStarStar(m_statep->symsp());
        // No longer needed, but can't delete until any multi-instantiated modules are expanded
    }

    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Foreach* nodep) override {  // FindVisitor::
        // Symbol table needs nodep->name() as the index variable's name
        VL_RESTORER(m_curSymp);
        {
            ++m_mod_With_ଅଙ୍କ;
            m_curSymp = m_statep->ଭର୍ତ୍ତି_Block(m_curSymp, "__Vforeach" + cvtToStr(m_mod_With_ଅଙ୍କ),
                                              nodep, m_classOrPackagep);
            m_curSymp->fallbackp(VL_RESTORER_PREV(m_curSymp));
            ପାଇଁ (Ast_ଗଣ୍ଠି *nextp, *argp = nodep->headerp()->elementsp(); argp; argp = nextp) {
                nextp = argp->nextp();
                AstVar* argrefp = nullptr;
                ଯଦି (AstParseRef* ଅଚଳ parserefp = VN_CAST(argp, ParseRef)) {
                    // IEEE 1800-2023 12.7.3: foreach loop variable type shall be int (2-state)
                    // This might get changed in V3Width when types resolve (e.g., for assoc
                    // arrays)
                    argrefp = ନୂତନ AstVar{parserefp->fileline(), VVarType::BLOCKTEMP,
                                         parserefp->name(), argp->findIntDType()};
                    argrefp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                    parserefp->replaceWith(argrefp);
                    VL_DO_DANGLING2(parserefp->delete_ବୃକ୍ଷ(), parserefp, argp);
                    // Insert argref's name into symbol table
                    m_statep->ଭର୍ତ୍ତି_Sym(m_curSymp, argrefp->name(), argrefp, nullptr);
                } ଅନ୍ୟଥା ଯଦି (ଅଚଳ auto largrefp = VN_CAST(argp, Var)) {
                    argrefp = largrefp;
                    // Insert argref's name into symbol table
                    m_statep->ଭର୍ତ୍ତି_Sym(m_curSymp, argrefp->name(), argrefp, nullptr);
                } ଅନ୍ୟଥା ଯଦି (VN_IS(argp, Empty)) {
                } ଅନ୍ୟଥା {
                    argp->v3error("'foreach' loop variable expects simple variable name");
                }
            }
            iterateChildren(nodep);
        }
    }

    ଶୂନ୍ୟ visit(AstRandSequence* nodep) override {
        VL_RESTORER(m_curSymp);
        m_curSymp = m_statep->ଭର୍ତ୍ତି_Block(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstRSProd* nodep) override {
        VL_RESTORER(m_curSymp);
        m_curSymp = m_statep->ଭର୍ତ୍ତି_Block(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
        ଯଦି (nodep->fvarp())
            nodep->fvarp()->v3warn(E_UNSUPPORTED,
                                   "Unsupported: randsequence production function variable");
        // Mark formal ports as port-checked so the primary resolve pass does not
        // flag them with "does not appear in port list" -- V3RandSequence will
        // later move them onto the generated task as real input ports.
        ପାଇଁ (Ast_ଗଣ୍ଠି* itp = nodep->portsp(); itp; itp = itp->nextp()) {
            VN_AS(itp, Var)->user4(ସତ୍ୟ);
        }
        iterateChildren(nodep);
    }

    ଶୂନ୍ୟ visit(AstWithParse* nodep) override {  // FindVisitor::
        // Change WITHPARSE(FUNCREF, equation) to FUNCREF(WITH(equation))
        Ast_ଗଣ୍ଠି_FTask_Ref* funcrefp = VN_CAST(nodep->funcrefp(), ଗଣ୍ଠି_FTask_Ref);
        ଯଦି (ଅଚଳ AstDot* ଅଚଳ dotp = VN_CAST(nodep->funcrefp(), Dot))
            funcrefp = VN_CAST(dotp->rhsp(), ଗଣ୍ଠି_FTask_Ref);
        UASSERT_OBJ(funcrefp, nodep, "'with' only can operate on a function/task");
        string name = funcrefp->name() == "randomize" ? "__Vrandwith_obj" : "item";
        File_ଧାଡ଼ି* argFl = nodep->fileline();
        AstArg* ଅଚଳ argsp = funcrefp->argsp();
        ଯଦି (argsp) {
            argsp->unlinkFrBackWithNext();
            ଯଦି (funcrefp->name() != "randomize") {
                ଯଦି (ଅଚଳ auto parserefp = VN_CAST(argsp->exprp(), ParseRef)) {
                    name = parserefp->name();
                    argFl = parserefp->fileline();
                } ଅନ୍ୟଥା {
                    argsp->v3error("'with' function expects simple variable name");
                }
                ଯଦି (argsp->nextp()) {
                    argsp->nextp()->v3error("'with' function expects only up to one argument");
                }
                VL_DO_DANGLING(argsp->delete_ବୃକ୍ଷ(), argsp);
            }
        }
        // Type depends on the method used, let V3Width figure it out later
        // 'with (...) { ... }' constraint_block form is randomize-only
        // (IEEE 1800-2023 7.12 vs. 18.7); array methods take 'with (expr)'.
        ଯଦି (nodep->restricted() && funcrefp->name() != "randomize") {
            nodep->v3error("'with (...) { ... }' constraint block is only valid for"
                           " randomize() (IEEE 1800-2023 18.7)");
            funcrefp->addArgsp(argsp);
            nodep->replaceWith(nodep->funcrefp()->unlinkFrBack());
            VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
            ଫେରନ୍ତୁ;
        }
        ଅଚଳ ତାର୍କିକ restrictedRandomize = nodep->restricted();
        ଯଦି (nodep->exprsp() || nodep->constraintsp()
            || restrictedRandomize) {  // Else empty expression and pretend no "with"
            AstLambdaArgRef* ଅଚଳ indexArgRefp
                = ନୂତନ AstLambdaArgRef{argFl, name + "__DOT__index", ସତ୍ୟ};
            AstLambdaArgRef* ଅଚଳ ମୂଲ୍ୟ_Arg_Refp = ନୂତନ AstLambdaArgRef{argFl, name, ମିଥ୍ୟା};
            AstWith* ଅଚଳ newp
                = ନୂତନ AstWith{nodep->fileline(), indexArgRefp, ମୂଲ୍ୟ_Arg_Refp, nullptr};
            // Harvest the identifier_list into AstWith; grammar guarantees AstParseRef.
            ଯଦି (restrictedRandomize) {
                newp->restricted(ସତ୍ୟ);
                ଯେତେବେଳେ (Ast_ଗଣ୍ଠି* ଅଚଳ itemp = nodep->exprsp()) {
                    newp->addRestrictedName(VN_AS(itemp, ParseRef)->name());
                    itemp->unlinkFrBack();
                    VL_DO_DANGLING(ଠେଲ_Deletep(itemp), itemp);
                }
            }
            Ast_ଗଣ୍ଠି* ପଦସମଷ୍ଟି_Or_Constraintsp = nullptr;
            ଯଦି (nodep->exprsp()) ପଦସମଷ୍ଟି_Or_Constraintsp = nodep->exprsp()->unlinkFrBackWithNext();
            ଯଦି (nodep->constraintsp())
                ପଦସମଷ୍ଟି_Or_Constraintsp = Ast_ଗଣ୍ଠି::addNext(
                    ପଦସମଷ୍ଟି_Or_Constraintsp, nodep->constraintsp()->unlinkFrBackWithNext());
            newp->addExprp(ପଦସମଷ୍ଟି_Or_Constraintsp);  // addExprp() tolerates nullptr
            funcrefp->withp(newp);
        }
        funcrefp->addArgsp(argsp);
        nodep->replaceWith(nodep->funcrefp()->unlinkFrBack());
        VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
    }
    ଶୂନ୍ୟ visit(AstWith* nodep) override {  // FindVisitor::
        // Symbol table needs nodep->name() as the index variable's name
        // Iteration will pickup the AstVar we made under AstWith
        VL_RESTORER(m_curSymp);
        ++m_mod_With_ଅଙ୍କ;
        m_curSymp = m_statep->ଭର୍ତ୍ତି_Block(m_curSymp, "__Vwith" + cvtToStr(m_mod_With_ଅଙ୍କ), nodep,
                                          m_classOrPackagep);
        m_curSymp->fallbackp(VL_RESTORER_PREV(m_curSymp));
        UASSERT_OBJ(nodep->indexArgRefp(), nodep, "Missing lambda argref");
        UASSERT_OBJ(nodep->ମୂଲ୍ୟ_Arg_Refp(), nodep, "Missing lambda argref");
        // Insert argref's name into symbol table
        m_statep->ଭର୍ତ୍ତି_Sym(m_curSymp, nodep->ମୂଲ୍ୟ_Arg_Refp()->name(), nodep->ମୂଲ୍ୟ_Arg_Refp(),
                            nullptr);
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstCoverWith* nodep) override {  // FindVisitor::
        // The range list is outside the scope of the filter, which declares 'item'
        iterateAndNextNull(nodep->subp());
        VL_RESTORER(m_curSymp);
        ++m_mod_With_ଅଙ୍କ;
        m_curSymp = m_statep->ଭର୍ତ୍ତି_Block(m_curSymp, "__VcoverWith" + cvtToStr(m_mod_With_ଅଙ୍କ),
                                          nodep, m_classOrPackagep);
        m_curSymp->fallbackp(VL_RESTORER_PREV(m_curSymp));
        iterateAndNextNull(nodep->itemp());
        iterateAndNextNull(nodep->filterp());
    }
    ଶୂନ୍ୟ visit(AstCoverCrossWith* nodep) override {  // FindVisitor::
        // The selection, the 'matches' count, and the cross items' types, of their coverpoints'
        // expressions with any nested scopes, are outside the filter's scope, which declares the
        // items.  As visit(AstVar) would resolve a type in the scope declaring its variable, the
        // items, generated and unique, are inserted directly.
        iterateAndNextNull(nodep->selectp());
        iterateAndNextNull(nodep->countp());
        ପାଇଁ (AstVar* varp = nodep->varsp(); varp; varp = VN_AS(varp->nextp(), Var)) {
            iterateAndNextNull(varp->childDTypep());
        }
        VL_RESTORER(m_curSymp);
        ++m_mod_With_ଅଙ୍କ;
        m_curSymp = m_statep->ଭର୍ତ୍ତି_Block(m_curSymp, "__VcoverWith" + cvtToStr(m_mod_With_ଅଙ୍କ),
                                          nodep, m_classOrPackagep);
        m_curSymp->fallbackp(VL_RESTORER_PREV(m_curSymp));
        ପାଇଁ (AstVar* varp = nodep->varsp(); varp; varp = VN_AS(varp->nextp(), Var)) {
            m_statep->ଭର୍ତ୍ତି_Sym(m_curSymp, varp->name(), varp, m_classOrPackagep);
        }
        iterateAndNextNull(nodep->filterp());
    }

    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { iterateChildren(nodep); }  // FindVisitor::

    ଶୂନ୍ୟ handleUnvisitedVirtIfaces() {
        Ast_ଗଣ୍ଠି_Module* ଅଚଳ ଶୀର୍ଷ = v3Global.rootp()->modulesp();
        m_curSymp = m_statep->get_ଗଣ୍ଠି_Sym(ଶୀର୍ଷ);
        ପାଇଁ (AstIface* ifacep : m_virtIfaces) {
            ଯଦି (!ifacep->user4()) {  // virtual interface never assigned any actual interface
                // Create dummy cell to keep the virtual interface alive
                // (later stages assume that non-dead interface has associated cell)
                AstCell* ଅଚଳ ifacecellp = ନୂତନ AstCell{ifacep->fileline(),
                                                        ifacep->fileline(),
                                                        "__VdummyInstOf" + ifacep->name(),
                                                        ifacep->name(),
                                                        nullptr,
                                                        nullptr,
                                                        nullptr};
                ifacecellp->modp(ifacep);
                ଶୀର୍ଷ->addStmtsp(ifacecellp);

                iterate(ifacecellp);
            }
        }
    }

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    LinkDotFindVisitor(AstNetlist* rootp, LinkDotState* statep)
        : m_statep{statep} {
        UINFO(4, __FUNCTION__ << ": ");
        iterate(rootp);

        ଯଦି (!m_virtIfaces.empty()) handleUnvisitedVirtIfaces();
        ପାଇଁ (AstScope* ଅଚଳ scopep : m_scopes) scopep->modp()->addStmtsp(scopep);
        ଯଦି (m_statep->forScopeCreation()) {
            V3Stats::addStat("LinkDot, Scope visits", m_statScopeVisits);
        }
    }
    ~LinkDotFindVisitor() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
};

//======================================================================

ଶ୍ରେଣୀ LinkDotFindIfaceVisitor final : ସର୍ବସାଧାରଣ VNVisitor {
    // NODE STATE
    //  *::user1p()             -> See LinkDotState

    // STATE - for current visit position (use VL_RESTORER)
    LinkDotState* ଅଚଳ m_statep;  // State to pass between visitors, including symbol table
    Ast_ଗଣ୍ଠି* m_declp = nullptr;  // Current declaring object that may soon contain IfaceRefDType

    // METHODS
    ଅଚଳ VSymEnt* findNonDeclSym(VSymEnt* symp, ଅଚଳ string& name) {
        // Find if there is a symbol of given name, ignoring the node that is declaring
        // it (m_declp) itself.  Thus if searching for "ifc" (an interface):
        //
        // module x;  // Finishes here and finds the typedef
        //   typedef foo ifc;
        //   ifc ifc;   // symp starts pointing here, but matches m_declp
        ଯେତେବେଳେ (symp) {
            ଅଚଳ VSymEnt* ଅଚଳ foundp = symp->findIdFlat(name);
            ଯଦି (foundp && foundp->nodep() != m_declp) ଫେରନ୍ତୁ foundp;
            symp = symp->fallbackp();
        }
        ଫେରନ୍ତୁ nullptr;
    }

    // VISITORS
    ଶୂନ୍ୟ visit(AstRefDType* nodep) override {  // FindIfaceVisitor::
        ଯଦି (m_statep->forPrimary() && !nodep->classOrPackagep()) {
            UINFO(9, "  FindIfc: " << nodep);
            // If under a var, ignore the var itself as might be e.g. "intf intf;"
            // Critical tests:
            //   t_interface_param_genblk.v  // Checks this does make interface
            //   t_interface_hidden.v  // Checks this doesn't making interface when hidden
            ଯଦି (m_statep->exists_ଗଣ୍ଠି_Sym(nodep)) {
                VSymEnt* symp = m_statep->get_ଗଣ୍ଠି_Sym(nodep);
                ଅଚଳ VSymEnt* foundp = findNonDeclSym(symp, nodep->name());
                Ast_ଗଣ୍ଠି* foundNodep = nullptr;
                // This: v4make test_regress/t/t_interface_param_genblk.py --debug
                // --debugi-V3LinkDot 9 Passes with this commented out:
                ଯଦି (foundp) foundNodep = foundp->nodep();
                ଯଦି (!foundNodep) foundNodep = m_statep->findModuleSym(nodep->name());
                ଯଦି (foundNodep) UINFO(9, "    Ifc foundNodep " << foundNodep);
                ଯଦି (AstIface* ଅଚଳ defp = VN_CAST(foundNodep, Iface)) {
                    // Must be found as module name, and not hidden/ by normal symbol (foundp)
                    AstIfaceRefDType* ଅଚଳ newp
                        = ନୂତନ AstIfaceRefDType{nodep->fileline(), "", nodep->name()};
                    ଯଦି (nodep->paramsp())
                        newp->addParamsp(nodep->paramsp()->unlinkFrBackWithNext());
                    newp->ifacep(defp);
                    newp->user1u(nodep->user1u());
                    UINFO(9, "    Resolved interface " << nodep << "  =>  " << defp);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
                    ଫେରନ୍ତୁ;
                }
            }
        }
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstVar* nodep) override {  // FindIfaceVisitor::
        VL_RESTORER(m_declp);
        m_declp = nodep;
        iterateChildren(nodep);
        AstIfaceRefDType* ଅଚଳ ifacerefp = LinkDotState::ifaceRefFromArray(nodep->subDTypep());
        ଯଦି (ifacerefp && m_statep->exists_ଗଣ୍ଠି_Sym(nodep)) {
            // Can't resolve until interfaces and modport names are
            // known; see notes at top
            UINFO(9, "  FindIfc Var IfaceRef " << ifacerefp);
            ଯଦି (!ifacerefp->isVirtual()) nodep->setIfaceRef();
            VSymEnt* ଅଚଳ symp = m_statep->get_ଗଣ୍ଠି_Sym(nodep);
            m_statep->ଭର୍ତ୍ତି_Iface_Var_Sym(symp);
        }
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { iterateChildren(nodep); }  // FindIfaceVisitor::

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    LinkDotFindIfaceVisitor(AstNetlist* rootp, LinkDotState* statep)
        : m_statep{statep} {
        UINFO(4, __FUNCTION__ << ": ");
        iterate(rootp);
    }
    ~LinkDotFindIfaceVisitor() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
};

//======================================================================

ଶ୍ରେଣୀ LinkDotParamVisitor final : ସର୍ବସାଧାରଣ VNVisitor {
    // NODE STATE
    //  *::user1p()             -> See LinkDotState
    //  *::user2p()             -> See LinkDotState
    //  *::user4()              -> See LinkDotState

    // STATE
    LinkDotState* ଅଚଳ m_statep;  // State to pass between visitors, including symbol table
    Ast_ଗଣ୍ଠି_Module* m_modp = nullptr;  // Current module

    ଶୂନ୍ୟ pin_Implicit_ପଦସମଷ୍ଟି_Recurse(Ast_ଗଣ୍ଠି* nodep) {
        // Under a pin, Check interconnect expression for a pin reference or a concat.
        // Create implicit variable as needed
        ଯଦି (VN_IS(nodep, Dot)) {  // Not creating a simple implied type,
            // and implying something else would just confuse later errors
        } ଅନ୍ୟଥା ଯଦି (VN_IS(nodep, VarRef) || VN_IS(nodep, ParseRef)) {
            // To prevent user errors, we should only do single bit
            // implicit vars, however some netlists (MIPS) expect single
            // bit implicit wires to get created with range 0:0 etc.
            m_statep->implicitOkAdd(m_modp, nodep->name());
        }
        // These are perhaps a little too generous, as a SELect of siga[sigb]
        // perhaps shouldn't create an implicit variable.  But, we've warned...
        ଅନ୍ୟଥା {
            ଯଦି (Ast_ଗଣ୍ଠି* ଅଚଳ refp = nodep->op1p()) pin_Implicit_ପଦସମଷ୍ଟି_Recurse(refp);
            ଯଦି (Ast_ଗଣ୍ଠି* ଅଚଳ refp = nodep->op2p()) pin_Implicit_ପଦସମଷ୍ଟି_Recurse(refp);
            ଯଦି (Ast_ଗଣ୍ଠି* ଅଚଳ refp = nodep->op3p()) pin_Implicit_ପଦସମଷ୍ଟି_Recurse(refp);
            ଯଦି (Ast_ଗଣ୍ଠି* ଅଚଳ refp = nodep->op4p()) pin_Implicit_ପଦସମଷ୍ଟି_Recurse(refp);
        }
        // Continue along a list (e.g. the terminals under AstImplicit), also after a reference
        ଯଦି (Ast_ଗଣ୍ଠି* ଅଚଳ refp = nodep->nextp()) pin_Implicit_ପଦସମଷ୍ଟି_Recurse(refp);
    }

    // VISITORS
    ଶୂନ୍ୟ visit(AstTypeTable*) override {}  // ParamVisitor::
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Module* nodep) override {  // ParamVisitor::
        UINFO(5, "   " << nodep);
        ଯଦି ((nodep->dead() || !nodep->user4()) && !nodep->hierParams()) {
            UINFO(4, "Mark dead module " << nodep);
            UASSERT_OBJ(!m_statep->forScopeCreation(), nodep,
                        "Dead module persisted past where should have removed");
            // Don't remove now, because we may have a tree of
            // parameterized modules with VARXREFs into the deleted module
            // region.  V3Dead should cleanup.
            // Downstream visitors up until V3Dead need to check for nodep->dead.
            nodep->dead(ସତ୍ୟ);
            ଫେରନ୍ତୁ;
        }
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstPin* nodep) override {  // ParamVisitor::
        // Pin: Link to submodule's port
        // Deal with implicit definitions - do before Resolve visitor as may
        // be referenced above declaration
        ଯଦି (nodep->exprp()  // Else specifically unconnected pin
            && !nodep->svImplicit()) {  // SV 19.11.3: .name pins don't allow implicit decls
            pin_Implicit_ପଦସମଷ୍ଟି_Recurse(nodep->exprp());
        }
    }
    ଶୂନ୍ୟ visit(AstDefParam* nodep) override {  // ParamVisitor::
        iterateChildren(nodep);
        nodep->v3warn(DEFPARAM, "defparam is deprecated (IEEE 1800-2023 C.4.1)\n"
                                    << nodep->warnMore()
                                    << "... Suggest use instantiation with #(."
                                    << nodep->prettyName() << "(...etc...))");
        ତାର୍କିକ hasPartSelect = ମିଥ୍ୟା;
        ଅଚଳ string path = extractDottedPath(nodep->pathp(), hasPartSelect);
        UASSERT_OBJ(!hasPartSelect && !path.empty(), nodep, "Unexpected defparam path shape");
        string baddot;
        VSymEnt* okSymp = nullptr;
        VSymEnt* ଅଚଳ foundp = m_statep->findDotted(
            nodep->fileline(), m_statep->get_ଗଣ୍ଠି_Sym(nodep), path, baddot, okSymp, ସତ୍ୟ);
        AstCell* ଅଚଳ cellp = foundp ? VN_AS(foundp->nodep(), Cell) : nullptr;
        ଯଦି (!cellp) {
            nodep->v3error("In defparam, instance " << path << " never declared");
        } ଅନ୍ୟଥା {
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ exprp = nodep->rhsp()->unlinkFrBack();
            UINFO(9, "Defparam cell " << path << "." << nodep->name() << " attach-to " << cellp
                                      << "  <= " << exprp);
            // Don't need to check the name of the defparam exists.  V3Param does.
            AstPin* ଅଚଳ pinp = ନୂତନ AstPin{nodep->fileline(),
                                            -1,  // Pin# not relevant
                                            nodep->name(), exprp};
            pinp->param(ସତ୍ୟ);
            pinp->paramPath(path);
            cellp->addParamsp(pinp);
        }
        VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ବୃକ୍ଷ(), nodep);
    }
    ଶୂନ୍ୟ visit(AstPort* nodep) override {  // ParamVisitor::
        // Port: Stash the pin number
        // Need to set pin numbers after varnames are created
        // But before we do the final resolution based on names
        VSymEnt* ଅଚଳ foundp = m_statep->get_ଗଣ୍ଠି_Sym(m_modp)->findIdFlat(nodep->name());
        AstVar* ଅଚଳ refp = foundp ? VN_CAST(foundp->nodep(), Var) : nullptr;
        ଯଦି (!foundp) {
            nodep->v3error(
                "Input/output/inout declaration not found for port: " << nodep->prettyNameQ());
        } ଅନ୍ୟଥା ଯଦି (!refp || (!refp->isIO() && !refp->isIfaceRef())) {
            nodep->v3error("Pin is not an in/out/inout/interface: " << nodep->prettyNameQ());
        } ଅନ୍ୟଥା {
            ଯଦି (refp->user4()) {
                nodep->v3error("Duplicate declaration of port: "
                               << nodep->prettyNameQ() << '\n'
                               << nodep->warnContextPrimary() << '\n'
                               << refp->warnOther() << "... Location of original declaration\n"
                               << refp->warnContextSecondary());
            }
            refp->user4(ସତ୍ୟ);
            VSymEnt* ଅଚଳ symp = m_statep->ଭର୍ତ୍ତି_Sym(m_statep->get_ଗଣ୍ଠି_Sym(m_modp),
                                                      "__pinNumber" + cvtToStr(nodep->pin_ଅଙ୍କ()),
                                                      refp, nullptr /*classOrPackagep*/);
            symp->exported(ମିଥ୍ୟା);
            refp->pin_ଅଙ୍କ(nodep->pin_ଅଙ୍କ());
            // Put the variable where the port is, so that variables stay
            // in pin number sorted order. Otherwise hierarchical or JSON
            // may botch by-position instances.
            nodep->addHereThisAsNext(refp->unlinkFrBack());
        }
        // Ports not needed any more
        VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ବୃକ୍ଷ(), nodep);
    }
    ଶୂନ୍ୟ visit(AstAssignW* nodep) override {  // ParamVisitor::
        // Deal with implicit definitions
        // We used to nodep->allowImplicit() here, but it turns out
        // normal "assigns" can also make implicit wires.  Yuk.
        pin_Implicit_ପଦସମଷ୍ଟି_Recurse(nodep->lhsp());
        iterateChildren(nodep);
    }

    ଶୂନ୍ୟ visit(AstImplicit* nodep) override {  // ParamVisitor::
        // Unsupported gates need implicit creation
        pin_Implicit_ପଦସମଷ୍ଟି_Recurse(nodep->exprsp());
        // We're done with implicit gates
        VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ବୃକ୍ଷ(), nodep);
    }
    ଶୂନ୍ୟ visit(AstClassOrPackageRef* nodep) override {  // ParamVisitor::
        ଯଦି (auto* ଅଚଳ fwdp = VN_CAST(nodep->classOrPackageNodep(), TypedefFwd)) {
            // Relink forward definitions to the "real" definition
            VSymEnt* ଅଚଳ foundp = m_statep->get_ଗଣ୍ଠି_Sym(fwdp)->findIdFallback(fwdp->name());
            ଯଦି (foundp && (VN_IS(foundp->nodep(), Class) || VN_IS(foundp->nodep(), Package))) {
                nodep->classOrPackagep(VN_AS(foundp->nodep(), ଗଣ୍ଠି_Module));
            } ଅନ୍ୟଥା ଯଦି (foundp && VN_IS(foundp->nodep(), ParamTypeDType)) {
                UASSERT(m_statep->forPrimary(), "Param types should have been resolved");
                nodep->classOrPackageNodep(VN_AS(foundp->nodep(), ParamTypeDType));
            } ଅନ୍ୟଥା {
                ଯଦି (foundp) UINFO(1, "found nodep = " << foundp->nodep());
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
    ଶୂନ୍ୟ visit(AstPull* nodep) override {  // ParamVisitor::
        // Deal with implicit definitions
        // We used to nodep->allowImplicit() here, but it turns out
        // normal "assigns" can also make implicit wires.  Yuk.
        pin_Implicit_ପଦସମଷ୍ଟି_Recurse(nodep->lhsp());
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstTypedefFwd* nodep) override {  // ParamVisitor::
        VSymEnt* ଅଚଳ foundp = m_statep->get_ଗଣ୍ଠି_Sym(nodep)->findIdFallback(nodep->name());
        ଯଦି (foundp) {
            // If the typedef was earlier in source order (tokenNum), then remember that earlier
            // point to avoid error something wasn't declared
            // Might be forward declaring something odd (with declTokenNumSetMin not implemented,
            // but should be harmless to ignore as this is just for error detection
            foundp->nodep()->decl_Token_ଅଙ୍କ_Set_Min(nodep->fileline()->token_ଅଙ୍କ());
        } ଅନ୍ୟଥା ଯଦି (v3Global.opt.pedantic()) {
            // We only check it was ever really defined in pedantic mode, as it
            // might have been in a header file referring to a module we never
            // needed so there are false positives
            nodep->v3error(
                "Forward typedef unused or does not resolve to a data type (IEEE 1800-2023 6.18): "
                << nodep->prettyNameQ());
        }
        // We only needed the forward declaration in order to parse correctly.
        // Delete later as may be ClassOrPackageRef's still pointing to it
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep->unlinkFrBack()), nodep);
    }

    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { iterateChildren(nodep); }  // ParamVisitor::

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    LinkDotParamVisitor(AstNetlist* rootp, LinkDotState* statep)
        : m_statep{statep} {
        UINFO(4, __FUNCTION__ << ": ");
        iterate(rootp);
    }
    ~LinkDotParamVisitor() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
};

//======================================================================

ଶ୍ରେଣୀ LinkDotScopeVisitor final : ସର୍ବସାଧାରଣ VNVisitor {

    // STATE
    LinkDotState* ଅଚଳ m_statep;  // State to pass between visitors, including symbol table
    ଅଚଳ AstScope* m_scopep = nullptr;  // The current scope
    VSymEnt* m_modSymp = nullptr;  // Symbol entry for current module
    // Deferred AliasScope processing - must be done outer-to-inner for correct alias resolution
    std::vector<std::pair<AstAliasScope*, VSymEnt*>> m_deferredAliasScopes;

    // METHODS
ସର୍ବସାଧାରଣ:
    // getAlias with setVarAlias and setIfaceAlias implement disjoint-set data structure.
    // This algorithm is needed for the case when multiple alias statements
    // reference partially the same variables.
    ସ୍ଥିର AstVarScope* getAlias(AstVarScope* ଅଚଳ vscp) {
        ଯଦି (vscp->user2p() && vscp != vscp->user2p()) {
            AstVarScope* ଅଚଳ aliasp = getAlias(VN_AS(vscp->user2p(), VarScope));
            vscp->user2p(aliasp);
            ଫେରନ୍ତୁ aliasp;
        } ଅନ୍ୟଥା {
            ଫେରନ୍ତୁ vscp;
        }
    }

ବ୍ୟକ୍ତିଗତ:
    // Alias an interface reference to the interface it is connected to
    ଶୂନ୍ୟ setIfaceAlias(AstVarScope* ଅଚଳ vscp, AstVarScope* ଅଚଳ aliasp) {
        UASSERT_OBJ(vscp->varp()->isIfaceRef(), vscp, "Interface alias of non-interface");
        UASSERT_OBJ(aliasp->varp()->isIfaceRef(), aliasp, "Interface alias of non-interface");
        getAlias(vscp)->user2p(getAlias(aliasp));
    }

    // Alias two variables. An output port in the lower scope survives, unless the other
    // must keep its own storage.
    ଶୂନ୍ୟ setVarAlias(AstVarScope* ap, AstVarScope* bp) {
        UASSERT_OBJ(!ap->varp()->isIfaceRef(), ap, "Variable alias of interface");
        UASSERT_OBJ(!bp->varp()->isIfaceRef(), bp, "Variable alias of interface");
        ap = getAlias(ap);
        bp = getAlias(bp);
        ଯଦି (ap == bp) ଫେରନ୍ତୁ;
        ଅଚଳ ତାର୍କିକ aWins = [&]() {
            ଅଚଳ AstVar* ଅଚଳ aVarp = ap->varp();
            ଅଚଳ AstVar* ଅଚଳ bVarp = bp->varp();
            // A primary IO is accessed directly by the user of the model, so must survive
            UASSERT_OBJ(!aVarp->isPrimaryIO() || !bVarp->isPrimaryIO(), ap,
                        "Alias of two primary IOs");
            ଯଦି (bVarp->isPrimaryIO()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
            ଯଦି (aVarp->isPrimaryIO()) ଫେରନ୍ତୁ ସତ୍ୟ;
            // A forced or public variable must keep its own storage. If both are, either works.
            ଯଦି (aVarp->isForced() != bVarp->isForced()) ଫେରନ୍ତୁ aVarp->isForced();
            ଯଦି (aVarp->isSigPublic() != bVarp->isSigPublic()) ଫେରନ୍ତୁ aVarp->isSigPublic();
            // Otherwise prefer an output port in the lower scope. This enables better V3Combine
            // due to having fewer upward hierarchical references (which prevent combining).
            ଯଦି (aVarp->direction() != VDirection::OUTPUT) ଫେରନ୍ତୁ ମିଥ୍ୟା;
            // Is 'ap' strictly below 'bp' in the hierarchy
            ପାଇଁ (ଅଚଳ AstScope* scopep = ap->scopep()->aboveScopep(); scopep;
                 scopep = scopep->aboveScopep()) {
                ଯଦି (scopep == bp->scopep()) ଫେରନ୍ତୁ ସତ୍ୟ;
            }
            ଫେରନ୍ତୁ ମିଥ୍ୟା;
        }();
        ଯଦି (aWins) {
            bp->user2p(ap);
        } ଅନ୍ୟଥା {
            ap->user2p(bp);
        }
    }

    // VISITORS
    ଶୂନ୍ୟ visit(AstNetlist* nodep) override {  // ScopeVisitor::
        // Recurse..., backward as must do packages before using packages
        iterateChildrenBackwardsConst(nodep);
    }
    ଶୂନ୍ୟ visit(AstScope* nodep) override {  // ScopeVisitor::
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
        ଯଦି (AstIface* ଅଚଳ ifacep = VN_CAST(nodep->modp(), Iface)) {
            m_statep->ଭର୍ତ୍ତି_Iface_Mod_Sym(ifacep, m_modSymp);
        }
        m_scopep = nodep;
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstVarScope* nodep) override {  // ScopeVisitor::
        ଯଦି (!nodep->varp()->isFuncLocal() && !nodep->varp()->isClassMember()) {
            VSymEnt* ଅଚଳ varSymp
                = m_statep->ଭର୍ତ୍ତି_Sym(m_modSymp, nodep->varp()->name(), nodep, nullptr);
            ଯଦି (nodep->varp()->isIfaceRef()) {
                AstIfaceRefDType* ଅଚଳ dtypep
                    = LinkDotState::ifaceRefFromArray(nodep->varp()->dtypep());
                UASSERT_OBJ(dtypep, nodep, "Non AstIfaceRefDType on isIfaceRef() var");
                ଯଦି (nodep->varp()->isIfaceParent()) {
                    UINFO(9, "Iface parent ref var " << nodep->varp()->name() << " " << nodep);
                    // Find the interface cell the var references
                    UINFO(9, "Iface parent dtype " << dtypep);
                    ଅଚଳ string ifcellname = dtypep->cellName();
                    string baddot;
                    VSymEnt* okSymp;
                    VSymEnt* cellSymp = m_statep->findDotted(nodep->fileline(), m_modSymp,
                                                             ifcellname, baddot, okSymp, ମିଥ୍ୟା);
                    UASSERT_OBJ(
                        cellSymp, nodep,
                        "No symbol for interface instance: " << nodep->prettyNameQ(ifcellname));
                    UINFO(5, "       Found interface instance: se" << cvtToHex(cellSymp) << " "
                                                                   << cellSymp->nodep());
                    ଯଦି (dtypep->modportName() != "") {
                        // Look up the modport within the interface cell's symbol table
                        VSymEnt* ଅଚଳ mpSymp = cellSymp->findIdFallback(dtypep->modportName());
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
                    m_statep->ଭର୍ତ୍ତି_Scope_Alias(LinkDotState::SAMN_IFTOP, varSymp, cellSymp);
                }
            }
        }
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_FTask* nodep) override {  // ScopeVisitor::
        VSymEnt* ଅଚଳ symp = m_statep->ଭର୍ତ୍ତି_Block(m_modSymp, nodep->name(), nodep, nullptr);
        symp->fallbackp(m_modSymp);
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Foreach* nodep) override {  // ScopeVisitor::
        VSymEnt* ଅଚଳ symp = m_statep->ଭର୍ତ୍ତି_Block(m_modSymp, nodep->name(), nodep, nullptr);
        symp->fallbackp(m_modSymp);
        // No recursion, we don't want to pick up variables
    }
    ଶୂନ୍ୟ visit(AstConstraintForeach* nodep) override {  // ScopeVisitor::
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstWith* nodep) override {  // ScopeVisitor::
        VSymEnt* ଅଚଳ symp = m_statep->ଭର୍ତ୍ତି_Block(m_modSymp, nodep->name(), nodep, nullptr);
        symp->fallbackp(m_modSymp);
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstAlias* nodep) override {  // ScopeVisitor::
        // Track aliases
        UINFOTREE(9, nodep, "", "alias");
        AstVarScope* aliasVscp = nullptr;
        ପାଇଁ (Ast_ଗଣ୍ଠି* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
            AstVarScope* vscp = nullptr;
            ଯଦି (ଅଚଳ AstVarRef* ଅଚଳ refp = VN_CAST(itemp, VarRef)) {
                vscp = refp->varScopep();
            } ଅନ୍ୟଥା {
                // Reaches into the scope of an instance, look it up by name
                ଅଚଳ AstVarXRef* ଅଚଳ xrefp = VN_AS(itemp, VarXRef);
                ଅଚଳ string scopename = xrefp->dotted() + "." + xrefp->name();
                string baddot;
                VSymEnt* okSymp;
                VSymEnt* ଅଚଳ symp = m_statep->findDotted(xrefp->fileline(), m_modSymp, scopename,
                                                           baddot, okSymp, ମିଥ୍ୟା);
                UASSERT_OBJ(symp, nodep, "No symbol for alias item: " << scopename);
                vscp = VN_CAST(symp->nodep(), VarScope);
            }
            UASSERT_OBJ(vscp, nodep, "VarScope unset");
            ଯଦି (aliasVscp) {
                setVarAlias(aliasVscp, vscp);
            } ଅନ୍ୟଥା {
                aliasVscp = vscp;
            }
        }
        iterateChildren(nodep);
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep->unlinkFrBack()), nodep);
    }
    ଶୂନ୍ୟ visit(AstAliasScope* nodep) override {  // ScopeVisitor::
        // Defer AliasScope processing - must process outer scopes before inner ones
        // so that nested interface port alias resolution works correctly
        UINFO(5, "ALIASSCOPE (deferred) " << nodep);
        m_deferredAliasScopes.emplace_back(nodep, m_modSymp);
    }
    ଶୂନ୍ୟ processAliasScope(AstAliasScope* nodep, VSymEnt* modSymp) {
        UINFO(5, "ALIASSCOPE  " << nodep);
        UINFOTREE(9, nodep, "", "avs");
        VSymEnt* rhsSymp;
        {
            AstVarRef* ଅଚଳ refp = VN_CAST(nodep->rhsp(), VarRef);
            AstVarXRef* ଅଚଳ xrefp = VN_CAST(nodep->rhsp(), VarXRef);
            UASSERT_OBJ(refp || xrefp, nodep,
                        "Unsupported: Non Var(X)Ref attached to interface pin");
            string inl
                = ((xrefp && xrefp->inlinedDots().size()) ? (xrefp->inlinedDots() + "__DOT__")
                                                          : "");
            ଅଚଳ string dottedPath
                = (xrefp && !xrefp->dotted().empty()) ? (xrefp->dotted() + ".") : "";
            VSymEnt* symp = nullptr;
            string scopename;
            ଯେତେବେଳେ (!symp) {
                scopename = refp ? refp->name()
                                 : (inl.size() ? (inl + dottedPath + xrefp->name())
                                               : (dottedPath + xrefp->name()));
                string baddot;
                VSymEnt* okSymp;
                symp = m_statep->findDotted(nodep->rhsp()->fileline(), modSymp, scopename, baddot,
                                            okSymp, ସତ୍ୟ);
                ଯଦି (inl == "") ଭାଙ୍ଗ;
                inl = LinkDotState::removeLastInlineScope(inl);
            }
            UASSERT_OBJ(symp, nodep, "No symbol for interface alias rhs");
            UINFO(5, "       Found a linked scope RHS: " << scopename << "  se" << cvtToHex(symp)
                                                         << " " << symp->nodep());
            rhsSymp = symp;
        }
        VSymEnt* lhsSymp;
        {
            ଅଚଳ AstVarXRef* ଅଚଳ xrefp = VN_CAST(nodep->lhsp(), VarXRef);
            ଅଚଳ AstVarRef* ଅଚଳ refp = VN_CAST(nodep->lhsp(), VarRef);

            UASSERT_OBJ(refp || xrefp, nodep,
                        "Unsupported: Non Var(X)Ref attached to interface pin");
            ଅଚଳ string scopename
                = refp ? refp->varp()->name() : xrefp->dotted() + "." + xrefp->name();
            string baddot;
            VSymEnt* okSymp;
            VSymEnt* ଅଚଳ symp = m_statep->findDotted(nodep->lhsp()->fileline(), modSymp,
                                                       scopename, baddot, okSymp, ମିଥ୍ୟା);
            UASSERT_OBJ(symp, nodep, "No symbol for interface alias lhs");
            UINFO(5, "       Found a linked scope LHS: " << scopename << "  se" << cvtToHex(symp)
                                                         << " " << symp->nodep());
            lhsSymp = symp;
        }
        // Remember the alias - can't do it yet because we may have additional symbols to be added,
        // or maybe an alias of an alias
        m_statep->ଭର୍ତ୍ତି_Scope_Alias(LinkDotState::SAMN_IFTOP, lhsSymp, rhsSymp);
        AstVarScope* ଅଚଳ lhsVscp = VN_CAST(lhsSymp->nodep(), VarScope);
        AstVarScope* ଅଚଳ rhsVscp = VN_CAST(rhsSymp->nodep(), VarScope);
        UASSERT_OBJ(lhsVscp && rhsVscp, nodep, "Interface alias missing variable scope");
        setIfaceAlias(lhsVscp, rhsVscp);
        // We have stored the link, we don't need these any more
        VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ବୃକ୍ଷ(), nodep);
    }
    ଶୂନ୍ୟ processDeferredAliasScopes() {
        // Sort by hierarchy depth (shallower first) so outer aliases are resolved before inner
        // Pre-compute depth map to avoid O(N log N * D) complexity in sort comparisons
        std::unordered_map<VSymEnt*, ପୂର୍ଣ୍ଣସଂଖ୍ୟା> depthMap;
        ପାଇଁ (ଅଚଳ auto& pair : m_deferredAliasScopes) {
            VSymEnt* ଅଚଳ symp = pair.second;
            ଯଦି (depthMap.find(symp) == depthMap.end()) {
                ପୂର୍ଣ୍ଣସଂଖ୍ୟା depth = 0;
                ପାଇଁ (VSymEnt* p = symp; p; p = p->parentp()) ++depth;
                depthMap[symp] = depth;
            }
        }
        std::stable_sort(m_deferredAliasScopes.begin(), m_deferredAliasScopes.end(),
                         [&depthMap](ଅଚଳ std::pair<AstAliasScope*, VSymEnt*>& a,
                                     ଅଚଳ std::pair<AstAliasScope*, VSymEnt*>& b) {
                             ଫେରନ୍ତୁ depthMap.at(a.second) < depthMap.at(b.second);
                         });
        // Process in sorted order
        ପାଇଁ (auto& pair : m_deferredAliasScopes) { processAliasScope(pair.first, pair.second); }
        m_deferredAliasScopes.clear();
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Gen* nodep) override {  // ScopeVisitor::  // LCOV_EXCL_LINE
        nodep->v3fatalSrc("Generate constructs should have been reduced out");
    }
    // For speed, don't recurse things that can't have scope
    // Note we allow AstNodeStmt's as generates may be under them
    ଶୂନ୍ୟ visit(AstCell*) override {}  // ScopeVisitor::
    ଶୂନ୍ୟ visit(AstVar*) override {}  // ScopeVisitor::
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { iterateChildren(nodep); }  // ScopeVisitor::

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    LinkDotScopeVisitor(AstNetlist* rootp, LinkDotState* statep)
        : m_statep{statep} {
        UINFO(4, __FUNCTION__ << ": ");
        iterate(rootp);
        // Process deferred AliasScopes in outer-to-inner order
        processDeferredAliasScopes();
    }
    ~LinkDotScopeVisitor() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
};

//======================================================================

// Iterate an interface to resolve modports
ଶ୍ରେଣୀ LinkDotIfaceVisitor final : ସର୍ବସାଧାରଣ VNVisitor {
    // STATE
    LinkDotState* ଅଚଳ m_statep;  // State to pass between visitors, including symbol table
    VSymEnt* m_curSymp;  // Symbol Entry for current table, where to lookup/insert

    // VISITORS
    ଶୂନ୍ୟ visit(AstModport* nodep) override {  // IfaceVisitor::
        // Modport: Remember its name for later resolution
        UINFO(5, "   fiv: " << nodep);
        VL_RESTORER(m_curSymp);
        {
            // Create symbol table for the vars
            m_curSymp = m_statep->ଭର୍ତ୍ତି_Block(m_curSymp, nodep->name(), nodep, nullptr);
            m_curSymp->fallbackp(VL_RESTORER_PREV(m_curSymp));
            iterateChildren(nodep);
        }
    }
    ଶୂନ୍ୟ visit(AstModportFTaskRef* nodep) override {  // IfaceVisitor::
        UINFO(5, "   fif: " << nodep);
        iterateChildren(nodep);
        VSymEnt* ଅଚଳ symp = m_curSymp->findIdFallback(nodep->name());
        ଯଦି (!symp) {
            nodep->v3error("Modport item not found: " << nodep->prettyNameQ());
        } ଅନ୍ୟଥା ଯଦି (Ast_ଗଣ୍ଠି_FTask* ଅଚଳ ftaskp = VN_CAST(symp->nodep(), ଗଣ୍ଠି_FTask)) {
            // Make symbol under modport that points at the _interface_'s var, not the modport.
            nodep->ftaskp(ftaskp);
            VSymEnt* ଅଚଳ subSymp
                = m_statep->ଭର୍ତ୍ତି_Sym(m_curSymp, nodep->name(), ftaskp, nullptr /*package*/);
            m_statep->ଭର୍ତ୍ତି_Scope_Alias(LinkDotState::SAMN_MODPORT, subSymp, symp);
        } ଅନ୍ୟଥା {
            nodep->v3error("Modport item is not a function/task: " << nodep->prettyNameQ());
        }
        ଯଦି (m_statep->forScopeCreation()) {
            // Done with AstModportFTaskRef.
            // Delete to prevent problems if we dead-delete pointed to ftask
            nodep->unlinkFrBack();
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
        }
    }

    // Helper to resolve remaining path through a nested interface
    // When findDotted() partially matches (okSymp set, baddot non-empty),
    // this follows the interface type to resolve the remaining path.
    // Returns the resolved symbol, or nullptr if not found.
    // On success, clears baddot; on partial match of multi-level path, updates baddot.
    VSymEnt* resolveNestedInterfacePath(File_ଧାଡ଼ି* fl, VSymEnt* okSymp, string& baddot) {
        ଯଦି (!okSymp || baddot.empty()) ଫେରନ୍ତୁ nullptr;

        ସ୍ଥିର constexpr ପୂର୍ଣ୍ଣସଂଖ୍ୟା MAX_NESTING_DEPTH = 64;
        VSymEnt* curOkSymp = okSymp;

        ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା depth = 0; depth < MAX_NESTING_DEPTH; ++depth) {
            // Try to get interface from the partially-matched symbol
            AstIface* ifacep = nullptr;
            ଯଦି (ଅଚଳ AstCell* ଅଚଳ cellp = VN_CAST(curOkSymp->nodep(), Cell)) {
                ifacep = VN_CAST(cellp->modp(), Iface);
            } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstVar* ଅଚଳ varp = VN_CAST(curOkSymp->nodep(), Var)) {
                ଯଦି (varp->isIfaceRef()) {
                    ଯଦି (ଅଚଳ AstIfaceRefDType* ଅଚଳ ifaceRefp
                        = LinkDotState::ifaceRefFromArray(varp->dtypep())) {
                        ifacep = ifaceRefp->ifaceViaCellp();
                    }
                }
            }

            ଯଦି (!ifacep || !m_statep->exists_ଗଣ୍ଠି_Sym(ifacep)) ଫେରନ୍ତୁ nullptr;

            VSymEnt* ଅଚଳ ifaceSymp = m_statep->get_ଗଣ୍ଠି_Sym(ifacep);

            ଯଦି (baddot.find('.') == string::npos) {
                // Simple identifier - direct lookup
                VSymEnt* ଅଚଳ symp = ifaceSymp->findIdFallback(baddot);
                ଯଦି (symp) baddot.clear();
                ଫେରନ୍ତୁ symp;
            }

            // Multi-level path - use findDotted for partial resolution
            string remainingBaddot;
            VSymEnt* remainingOkSymp = nullptr;
            VSymEnt* ଅଚଳ symp = m_statep->findDotted(fl, ifaceSymp, baddot, remainingBaddot,
                                                       remainingOkSymp, ସତ୍ୟ);
            ଯଦି (symp) {
                baddot = remainingBaddot;
                ଫେରନ୍ତୁ symp;
            }

            // findDotted partially matched - check progress
            ଯଦି (remainingBaddot == baddot || !remainingOkSymp) ଫେରନ୍ତୁ nullptr;

            // Continue resolving with updated state
            baddot = remainingBaddot;
            curOkSymp = remainingOkSymp;
        }

        UINFO(1, "Nested interface resolution depth limit exceeded at " << fl);
        ଫେରନ୍ତୁ nullptr;
    }

    // Resolve a modport expression to find the referenced symbol
    VSymEnt* resolveModportExpression(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp) {
        UINFO(5, "     resolveModportExpression: " << exprp);
        VSymEnt* symp = nullptr;
        ଯଦି (AstParseRef* ଅଚଳ refp = VN_CAST(exprp, ParseRef)) {
            // Simple variable reference: modport mp(input .a(sig_a))
            symp = m_curSymp->findIdFallback(refp->name());
        } ଅନ୍ୟଥା ଯଦି (AstVarRef* ଅଚଳ refp = VN_CAST(exprp, VarRef)) {
            // Already resolved VarRef (can happen if iterateChildren resolved it)
            ଯଦି (refp->varScopep()) {
                // During scope creation, VarRef may have VarScope already linked
                symp = m_statep->get_ଗଣ୍ଠି_Sym(refp->varScopep());
            } ଅନ୍ୟଥା ଯଦି (refp->varp()) {
                symp = m_curSymp->findIdFallback(refp->varp()->name());
            }
        } ଅନ୍ୟଥା ଯଦି (AstVarXRef* ଅଚଳ refp = VN_CAST(exprp, VarXRef)) {
            // Resolved VarXRef (dotted reference resolved by earlier pass)
            ଯଦି (refp->varp()) {
                // For nested interfaces, the var is in a different scope
                // First try to find the symbol via the var itself
                ଯଦି (m_statep->exists_ଗଣ୍ଠି_Sym(refp->varp())) {
                    symp = m_statep->get_ଗଣ୍ଠି_Sym(refp->varp());
                } ଅନ୍ୟଥା {
                    // Fallback: look up in current scope
                    symp = m_curSymp->findIdFallback(refp->varp()->name());
                }
            } ଅନ୍ୟଥା ଯଦି (!refp->dotted().empty()) {
                // varp not set yet - use dotted path to find the symbol
                // The dotted part (e.g., "base") is the interface cell, and name is the member
                ଅଚଳ string fullPath = refp->dotted() + "." + refp->name();
                string baddot;
                VSymEnt* okSymp = nullptr;
                symp = m_statep->findDotted(fl, m_curSymp, fullPath, baddot, okSymp, ସତ୍ୟ);
                // Handle nested interface path when findDotted had partial match
                ଯଦି (okSymp && !baddot.empty()) {
                    VSymEnt* ଅଚଳ resolved = resolveNestedInterfacePath(fl, okSymp, baddot);
                    ଯଦି (resolved) symp = resolved;
                }
            }
        } ଅନ୍ୟଥା ଯଦି (AstDot* ଅଚଳ dotp = VN_CAST(exprp, Dot)) {
            // Dotted path: modport mp(input .a(inner.sig))
            ତାର୍କିକ hasPartSelect = ମିଥ୍ୟା;
            ଅଚଳ string dottedPath = extractDottedPath(dotp, hasPartSelect);
            ଯଦି (hasPartSelect) {
                fl->v3warn(
                    E_UNSUPPORTED,
                    "Unsupported: Modport expression with part select (IEEE 1800-2023 25.5.4)");
            } ଅନ୍ୟଥା {
                string baddot;
                VSymEnt* okSymp = nullptr;
                symp = m_statep->findDotted(fl, m_curSymp, dottedPath, baddot, okSymp, ସତ୍ୟ);

                // Handle nested interface path when findDotted had partial match
                ଯଦି (okSymp && !baddot.empty()) {
                    VSymEnt* ଅଚଳ resolved = resolveNestedInterfacePath(fl, okSymp, baddot);
                    ଯଦି (resolved) symp = resolved;
                }

                ଯଦି (!symp || !baddot.empty()) {
                    fl->v3error("Can't find modport expression target: "
                                << Ast_ଗଣ୍ଠି::prettyNameQ(dottedPath));
                    symp = nullptr;
                }
            }
        } ଅନ୍ୟଥା ଯଦି (VN_IS(exprp, SelBit) || VN_IS(exprp, SelExtract)) {
            // Part select expressions not yet supported
            fl->v3warn(E_UNSUPPORTED,
                       "Unsupported: Modport expression with part select (IEEE 1800-2023 25.5.4)");
        } ଅନ୍ୟଥା {
            // Other expression types not supported
            fl->v3warn(E_UNSUPPORTED,
                       "Unsupported: Complex modport expression (IEEE 1800-2023 25.5.4)");
        }
        ଫେରନ୍ତୁ symp;
    }

    ଶୂନ୍ୟ visit(AstModportVarRef* nodep) override {  // IfaceVisitor::
        UINFO(5, "   fiv: " << nodep);
        iterateChildren(nodep);
        VSymEnt* symp = nullptr;
        ଯଦି (nodep->exprp()) {
            // Modport expression syntax: modport mp(input .port_name(expression))
            symp = resolveModportExpression(nodep->fileline(), nodep->exprp());
        } ଅନ୍ୟଥା {
            symp = m_curSymp->findIdFallback(nodep->name());
        }
        ଯଦି (!symp) {
            nodep->v3error("Modport item not found: " << nodep->prettyNameQ());
        } ଅନ୍ୟଥା ଯଦି (AstVar* ଅଚଳ varp = VN_CAST(symp->nodep(), Var)) {
            // Make symbol under modport that points at the _interface_'s var via the modport.
            // (Need modport still to test input/output markings)
            nodep->varp(varp);
            ଯଦି (nodep->exprp()) {
                // For modport expressions, insert symbol pointing to the underlying var (not the
                // ModportVarRef which will be deleted during scope creation). The virtual port
                // name maps to the real signal's var.
                VSymEnt* ଅଚଳ subSymp
                    = m_statep->ଭର୍ତ୍ତି_Sym(m_curSymp, nodep->name(), varp, nullptr /*package*/);
                m_statep->ଭର୍ତ୍ତି_Scope_Alias(LinkDotState::SAMN_MODPORT, subSymp, symp);
            } ଅନ୍ୟଥା {
                // For regular modport items, insert symbol pointing to ModportVarRef for
                // input/output marking tests.
                m_statep->ଭର୍ତ୍ତି_Sym(m_curSymp, nodep->name(), nodep, nullptr /*package*/);
            }
        } ଅନ୍ୟଥା ଯଦି (AstVarScope* ଅଚଳ vscp = VN_CAST(symp->nodep(), VarScope)) {
            // Make symbol under modport that points at the _interface_'s var, not the modport.
            nodep->varp(vscp->varp());
            // Only insert symbol for modport expression virtual ports (exprp set).
            // For regular modport items, don't insert into the shared modport table because
            // each instance would overwrite the previous one, causing wrong VarScope lookups.
            // Regular items are found via the modport's fallback to the interface.
            ଯଦି (nodep->exprp()) {
                VSymEnt* ଅଚଳ subSymp
                    = m_statep->ଭର୍ତ୍ତି_Sym(m_curSymp, nodep->name(), vscp, nullptr /*package*/);
                m_statep->ଭର୍ତ୍ତି_Scope_Alias(LinkDotState::SAMN_MODPORT, subSymp, symp);
            }
        } ଅନ୍ୟଥା {
            nodep->v3error("Modport item is not a variable: " << nodep->prettyNameQ());
        }
        ଯଦି (m_statep->forScopeCreation()) {
            // Done with AstModportVarRef.
            // Delete to prevent problems if we dead-delete pointed to variable
            nodep->unlinkFrBack();
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
        }
    }
    ଶୂନ୍ୟ visit(AstModportClockingRef* nodep) override {  // IfaceVisitor::
        UINFO(5, "   fic: " << nodep);
        iterateChildren(nodep);
        VSymEnt* ଅଚଳ symp = m_curSymp->findIdFallback(nodep->name());
        ଯଦି (!symp) {
            nodep->v3error("Modport item not found: " << nodep->prettyNameQ());
        } ଅନ୍ୟଥା ଯଦି (AstClocking* ଅଚଳ clockingp = VN_CAST(symp->nodep(), Clocking)) {
            nodep->clockingp(clockingp);
            m_statep->ଭର୍ତ୍ତି_Sym(m_curSymp, nodep->name(), nodep, nullptr /*package*/);
        } ଅନ୍ୟଥା {
            nodep->v3error("Modport item is not a clocking block: " << nodep->prettyNameQ());
        }
    }
    ଶୂନ୍ୟ visit(AstScope* nodep) override {  // IfaceVisitor::
        ଯଦି (nodep->user3SetOnce()) ଫେରନ୍ତୁ;
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { iterateChildren(nodep); }  // IfaceVisitor::

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    LinkDotIfaceVisitor(AstIface* nodep, VSymEnt* curSymp, LinkDotState* statep)
        : m_statep{statep}
        , m_curSymp{curSymp} {
        UINFO(4, __FUNCTION__ << ": ");
        iterate(nodep);
    }
    ~LinkDotIfaceVisitor() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
};

ଶୂନ୍ୟ LinkDotState::computeIfaceModSyms() {
    //  AstScope::user3()  // bool. Already iterated by a LinkDotIfaceVisitor
    ଅଚଳ VNUser3InUse user3InUse;
    ପାଇଁ (ଅଚଳ auto& itr : m_ifaceModSyms) {
        AstIface* ଅଚଳ nodep = itr.first;
        VSymEnt* ଅଚଳ symp = itr.second;
        LinkDotIfaceVisitor{nodep, symp, ଏହି};
    }
    m_ifaceModSyms.clear();
}

//======================================================================

ଶ୍ରେଣୀ LinkDotResolveVisitor final : ସର୍ବସାଧାରଣ VNVisitor {
    // NODE STATE
    // Cleared on global
    //  *::user1p()             -> See LinkDotState
    //  *::user2p()             -> See LinkDotState
    //  *::user3()              // bool. Processed
    //  *::user4()              -> See LinkDotState
    ଅଚଳ VNUser3InUse m_inuser3;

    // TYPES
    ଗଣନା DotPosition : uint8_t {
        // Must match ascii() method below
        DP_NONE = 0,  // Not under a DOT
        DP_PACKAGE,  // {package-or-class}:: DOT
        DP_FIRST,  // {scope-or-var} DOT
        DP_SCOPE,  // DOT... {scope-or-var} DOT
        DP_FINAL,  // [DOT...] {var-or-func-or-dtype} with no following dots
        DP_MEMBER  // DOT {member-name} [DOT...]
    };

    // STATE
    LinkDotState* ଅଚଳ m_statep;  // State, including dotted symbol table
    VSymEnt* m_curSymp = nullptr;  // SymEnt for current lookup point
    VSymEnt* m_modSymp = nullptr;  // SymEnt for current module
    VSymEnt* m_pinSymp = nullptr;  // SymEnt for pin lookups
    VSymEnt* m_randSymp = nullptr;  // SymEnt for randomize target class's lookups
    ଅଚଳ AstCell* m_cellp = nullptr;  // Current cell
    Ast_ଗଣ୍ଠି_Module* m_modp = nullptr;  // Current module
    Ast_ଗଣ୍ଠି_FTask* m_ftaskp = nullptr;  // Current function/task
    AstMethodCall* m_randMethodCallp = nullptr;  // Current randomize() method call
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_modport_ଅଙ୍କ = 0;  // Uniqueify modport numbers
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_indent = 0;  // Indentation (tree depth) for debug
    ତାର୍କିକ m_inSens = ମିଥ୍ୟା;  // True if in senitem
    ଅଚଳ AstWith* m_currentWithp = nullptr;  // Enclosing 'with' (nullptr if none)
    std::set<std::string>
        m_restrictedNamesUsed;  // Names from current 'with (id_list)' that resolved into target
    ତାର୍କିକ m_genericIfaceModule = ମିଥ୍ୟା;  // True if in module containing generic interface
    std::map<std::string, Ast_ଗଣ୍ଠି*> m_ifClassImpNames;  // Names imported from interface class
    // Inherited interface implementations needing a forward in the current class, by name
    std::map<std::string, ଅଚଳ VSymEnt*> m_ifForwards;
    std::set<AstClass*> m_extendsParam;  // Classes that have a parameterized super class
                                         // (except the default instances)
                                         // They are added to the set only in linkDotPrimary.
    ତାର୍କିକ m_insideClassExtParam = ମିଥ୍ୟା;  // Inside a class from m_extendsParam
    AstNew* m_explicitSuperNewp = nullptr;  // Hit a "super.new" call inside a "new" function
    std::map<Ast_ଗଣ୍ଠି*, AstPin*> m_usedPins;  // Pin used in this cell, map to duplicate
    std::map<Ast_ଗଣ୍ଠି*, std::map<std::string, AstPin*>> m_usedDefParamPins;
    // Defparam pins used for a given formal, by hierarchical path
    std::map<std::string, Ast_ଗଣ୍ଠି_Module*> m_modulesToRevisit;  // Modules to revisit a second time
    Ast_ଗଣ୍ଠି* m_lastDeferredp = nullptr;  // Last node which requested a revisit of its module
    Ast_ଗଣ୍ଠି_DType* m_packedArrayDtp = nullptr;  // Datatype reference for packed array
    ତାର୍କିକ m_inPackedArray = ମିଥ୍ୟା;  // Currently traversing a packed array tree
    ତାର୍କିକ m_replaceWithAlias
        = ସତ୍ୟ;  // Replace VarScope with an alias. Used in the handling of AstAlias
    ତାର୍କିକ m_isParam = ମିଥ୍ୟା;  // Specifies whether currently visiting param variable
    ତାର୍କିକ m_resolvingTypedef = ମିଥ୍ୟା;  // Currently traversing a Typedef tree

    ସଂରଚନା DotStates final {
        DotPosition m_dotPos;  // Scope part of dotted resolution
        VSymEnt* m_dotSymp;  // SymEnt for dotted AstParse lookup
        ଅଚଳ AstDot* m_dotp;  // Current dot
        ତାର୍କିକ m_super;  // Starts with super reference
        ତାର୍କିକ m_unresolvedCell;  // Unresolved cell, needs help from V3Param
        ତାର୍କିକ m_unresolvedClass;  // Unresolved class reference, needs help from V3Param
        ତାର୍କିକ m_unresolvedGenericIface;  // Unresolved generic interface, needs help from V3Param
        ତାର୍କିକ m_genBlk;  // Contains gen block reference
        Ast_ଗଣ୍ଠି* m_unlinkedScopep;  // Unresolved scope, needs corresponding VarXRef
        AstDisable* m_disablep;  // Disable statement under which the reference is
        ତାର୍କିକ m_dotErr;  // Error found in dotted resolution, ignore upwards
        string m_dotText;  // String of dotted names found in below parseref
        DotStates() { init(nullptr); }
        ~DotStates() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
        ଶୂନ୍ୟ init(VSymEnt* curSymp, ତାର୍କିକ unresolvedClass = ମିଥ୍ୟା) {
            m_dotPos = DP_NONE;
            m_dotSymp = curSymp;
            m_dotp = nullptr;
            m_super = ମିଥ୍ୟା;
            m_dotErr = ମିଥ୍ୟା;
            m_dotText = "";
            m_unresolvedCell = ମିଥ୍ୟା;
            m_unresolvedClass = unresolvedClass;
            m_unresolvedGenericIface = ମିଥ୍ୟା;
            m_genBlk = ମିଥ୍ୟା;
            m_unlinkedScopep = nullptr;
            m_disablep = nullptr;
        }
        string ascii() ଅଚଳ {
            ସ୍ଥିର ଅଚଳ ଅକ୍ଷର* ଅଚଳ names[]
                = {"DP_NONE", "DP_PACKAGE", "DP_FIRST", "DP_SCOPE", "DP_FINAL", "DP_MEMBER"};
            std::ostringstream sstr;
            sstr << "ds=" << names[m_dotPos];
            sstr << "  dse" << cvtToHex(m_dotSymp);
            ଅଚଳ string dsname = m_dotSymp->nodep()->name().substr(0, 8);
            sstr << "(" << m_dotSymp->nodep()->typeName() << (dsname.empty() ? "" : ":") << dsname
                 << ")";
            ଯଦି (m_dotErr) sstr << "  [dotErr]";
            ଯଦି (m_super) sstr << "  [super]";
            ଯଦି (m_unresolvedCell) sstr << "  [unrCell]";
            ଯଦି (m_unresolvedClass) sstr << "  [unrClass]";
            ଯଦି (m_unresolvedGenericIface) sstr << "  [unrGIface]";
            ଯଦି (m_genBlk) sstr << "  [genBlk]";
            sstr << "  txt=" << m_dotText;
            ଫେରନ୍ତୁ sstr.str();
        }
    } m_ds;  // State to preserve across recursions

    // METHODS - Variables
    AstVar* createImplicitVar(VSymEnt* /*lookupSymp*/, AstParseRef* nodep, Ast_ଗଣ୍ଠି_Module* modp,
                              VSymEnt* moduleSymp, ତାର୍କିକ noWarn) {
        // Create implicit after warning
        ଯଦି (!noWarn) {
            ଯଦି (nodep->fileline()->warnIsOff(V3ErrorCode::I_DEF_NETTYPE_WIRE)) {
                ଅଚଳ string suggest = m_statep->suggestSymFallback(moduleSymp, nodep->name(),
                                                                    Link_ଗଣ୍ଠି_Matcher_Var{});
                nodep->v3error("Signal definition not found, and implicit disabled with "
                               "`default_nettype: "
                               << nodep->prettyNameQ() << '\n'
                               << (suggest.empty() ? "" : nodep->warnMore() + suggest));

            }
            // Bypass looking for suggestions if IMPLICIT is turned off
            // as there could be thousands of these suppressed in large netlists
            ଅନ୍ୟଥା ଯଦି (!nodep->fileline()->warnIsOff(V3ErrorCode::IMPLICIT)) {
                ଅଚଳ string suggest = m_statep->suggestSymFallback(moduleSymp, nodep->name(),
                                                                    Link_ଗଣ୍ଠି_Matcher_Var{});
                nodep->v3warn(IMPLICIT,
                              "Signal definition not found, creating implicitly: "
                                  << nodep->prettyNameQ() << '\n'
                                  << (suggest.empty() ? "" : nodep->warnMore() + suggest));
            }
        }
        AstVar* ଅଚଳ newp
            = ନୂତନ AstVar{nodep->fileline(), VVarType::WIRE, nodep->name(), VFlagLogicPacked{}, 1};
        newp->lifetime(VLifetime::STATIC_IMPLICIT);
        newp->trace(modp->modTrace());
        modp->addStmtsp(newp);
        // Link it to signal list, must add the variable under the module;
        // current scope might be lower now
        m_statep->ଭର୍ତ୍ତି_Sym(moduleSymp, newp->name(), newp, nullptr /*classOrPackagep*/);
        ଫେରନ୍ତୁ newp;
    }
    AstVar* foundToVarp(ଅଚଳ VSymEnt* symp, Ast_ଗଣ୍ଠି* nodep, VAccess access) {
        // Return a variable if possible, auto converting a modport to variable
        ଯଦି (!symp) {
            ଫେରନ୍ତୁ nullptr;
        } ଅନ୍ୟଥା ଯଦି (VN_IS(symp->nodep(), Var)) {
            ଫେରନ୍ତୁ VN_AS(symp->nodep(), Var);
        } ଅନ୍ୟଥା ଯଦି (VN_IS(symp->nodep(), ModportVarRef)) {
            AstModportVarRef* ଅଚଳ snodep = VN_AS(symp->nodep(), ModportVarRef);
            AstVar* ଅଚଳ varp = snodep->varp();
            ଯଦି (access.isWriteOrRW() && snodep->direction().isReadOnly()) {
                nodep->v3error("Attempt to drive input-only modport: " << nodep->prettyNameQ());
            }  // else other simulators don't warn about reading, and IEEE doesn't say illegal
            ଫେରନ୍ତୁ varp;
        } ଅନ୍ୟଥା {
            ଫେରନ୍ତୁ nullptr;
        }
    }

    // Look up a virtual port through modport expression mapping.
    // When a dotted reference uses a modport with expression syntax, the virtual port name
    // maps to the real signal. This helper resolves that mapping during scope creation.
    // Returns the VarScope for the real signal, or nullptr if not found.
    AstVarScope* findVirtualPortVarScope(VSymEnt* dotSymp, ଅଚଳ string& portName) {
        ଅଚଳ AstVarScope* ଅଚଳ dotVscp = VN_CAST(dotSymp->nodep(), VarScope);
        ଅଚଳ AstVar* ଅଚଳ dotVarp = dotVscp ? dotVscp->varp() : nullptr;
        ଯଦି (!dotVarp) ଫେରନ୍ତୁ nullptr;

        Ast_ଗଣ୍ଠି_DType* dtypep = dotVarp->childDTypep();
        ଯଦି (!dtypep) dtypep = dotVarp->subDTypep();
        ଯଦି (!dtypep) ଫେରନ୍ତୁ nullptr;
        ଅଚଳ AstIfaceRefDType* ଅଚଳ ifaceRefp = VN_CAST(dtypep, IfaceRefDType);
        ଯଦି (!ifaceRefp || !ifaceRefp->modportp() || !ifaceRefp->ifaceViaCellp()) ଫେରନ୍ତୁ nullptr;

        // Get the interface cell's symbol table
        VSymEnt* ଅଚଳ ifaceCellSymp = m_statep->get_ଗଣ୍ଠି_Sym(ifaceRefp->ifaceViaCellp());

        // Look up modport by name in interface cell
        VSymEnt* ଅଚଳ modportSymp = ifaceCellSymp->findIdFallback(ifaceRefp->modportp()->name());
        ଯଦି (!modportSymp) ଫେରନ୍ତୁ nullptr;

        // Look up virtual port name in modport symbol table
        string baddot;
        VSymEnt* ଅଚଳ virtPortp = m_statep->findSymPrefixed(modportSymp, portName, baddot, ସତ୍ୟ);
        ଯଦି (!virtPortp) ଫେରନ୍ତୁ nullptr;

        // Symbol may point to VarScope or Var
        ଯଦି (AstVarScope* vscp = VN_CAST(virtPortp->nodep(), VarScope)) ଫେରନ୍ତୁ vscp;

        // Symbol points to Var - look up VarScope by name in interface cell
        ଯଦି (ଅଚଳ AstVar* ଅଚଳ realVarp = VN_CAST(virtPortp->nodep(), Var)) {
            VSymEnt* ଅଚଳ realFoundp
                = m_statep->findSymPrefixed(ifaceCellSymp, realVarp->name(), baddot, ସତ୍ୟ);
            ଫେରନ୍ତୁ realFoundp ? VN_CAST(realFoundp->nodep(), VarScope) : nullptr;
        }
        ଫେରନ୍ତୁ nullptr;
    }

    // Capture a ParamTypeDType reference for interface typedef retargeting.
    // Called when a RefDType resolves to a ParamTypeDType owned by an interface.
    ଶୂନ୍ୟ captureIfaceParamType(AstRefDType* nodep, AstParamTypeDType* defp) {
        ଯଦି (!V3LinkDotIfaceCapture::enabled() || !m_statep->forPrimary()) ଫେରନ୍ତୁ;
        Ast_ଗଣ୍ଠି_Module* ଅଚଳ defOwnerModp = V3LinkDotIfaceCapture::findOwnerModule(defp);
        ଯଦି (!defOwnerModp || !VN_IS(defOwnerModp, Iface)) ଫେରନ୍ତୁ;
        AstCell* ଅଚଳ cellForCapture
            = m_ds.m_dotSymp ? VN_CAST(m_ds.m_dotSymp->nodep(), Cell) : nullptr;
        ଯଦି (!cellForCapture) ଫେରନ୍ତୁ;
        UINFO(9, indent() << "iface capture add paramtype " << nodep
                          << " iface=" << defOwnerModp->prettyNameQ());
        V3LinkDotIfaceCapture::addParamType(nodep, cellForCapture->name(), m_modp, defp,
                                            defOwnerModp->name());
    }

    Ast_ଗଣ୍ଠି_Stmt* addImplicitSuperNewCall(AstFunc* ଅଚଳ nodep,
                                         ଅଚଳ AstClassExtends* ଅଚଳ classExtendsp) {
        // Returns the added node
        File_ଧାଡ଼ି* ଅଚଳ fl = nodep->fileline();
        AstArg* argsp = nullptr;
        ଯଦି (classExtendsp->argsp()) argsp = classExtendsp->argsp()->clone_ବୃକ୍ଷ(ସତ୍ୟ);
        AstNew* ଅଚଳ newExprp = ନୂତନ AstNew{fl, argsp};
        newExprp->isImplicit(ସତ୍ୟ);
        AstDot* ଅଚଳ superNewp = ନୂତନ AstDot{fl, ମିଥ୍ୟା, ନୂତନ AstParseRef{fl, "super"}, newExprp};
        Ast_ଗଣ୍ଠି_Stmt* ଅଚଳ superNewStmtp = superNewp->makeStmt();
        ପାଇଁ (Ast_ଗଣ୍ଠି* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            // super.new shall be the first statement (IEEE 1800-2023 8.15)
            // but some nodes (such as variable declarations and typedefs) should stay before
            ଯଦି (VN_IS(stmtp, ଗଣ୍ଠି_Stmt)) {
                stmtp->addHereThisAsNext(superNewStmtp);
                ଫେରନ୍ତୁ superNewStmtp;
            }
        }
        // There were no statements
        nodep->addStmtsp(superNewStmtp);
        ଫେରନ୍ତୁ superNewStmtp;
    }
    ଶୂନ୍ୟ checkNoDot(Ast_ଗଣ୍ଠି* nodep) {
        ଯଦି (VL_UNLIKELY(!nodep)) {
            UINFO(9, indent() << "iface capture null node passed to checkNoDot; dot state="
                              << m_ds.ascii());
            m_ds.m_dotErr = ସତ୍ୟ;
            ଫେରନ୍ତୁ;
        }
        ଯଦି (VL_UNLIKELY(m_ds.m_dotPos != DP_NONE)) {
            UINFO(9, indent() << "ds=" << m_ds.ascii());
            UINFO(9, indent() << "iface capture checkNoDot hit node=" << nodep
                              << " dotState=" << m_ds.ascii());
            ଯଦି (VL_UNLIKELY(!m_ds.m_dotp)) {
                nodep->v3error("Syntax error: Not expecting " << nodep->type()
                                                              << " here (missing dot context)");
            } ଅନ୍ୟଥା {
                nodep->v3error("Syntax error: Not expecting "
                               << nodep->type() << " under a " << nodep->backp()->type()
                               << " in dotted expression\n"
                               << nodep->warnContextPrimary() << m_ds.m_dotp->warnOther()
                               << "... Resolving this reference\n"
                               << m_ds.m_dotp->warnContextSecondary());
            }
            m_ds.m_dotErr = ସତ୍ୟ;
        }
    }
    AstVar* find_Iface_ଶୀର୍ଷ_Varp(Ast_ଗଣ୍ଠି* nodep, VSymEnt* parentEntp, ଅଚଳ string& name) {
        ଅଚଳ string findName = name + "__Viftop";
        VSymEnt* ଅଚଳ ifaceSymp = parentEntp->findIdFallback(findName);
        AstVar* ଅଚଳ iface_ଶୀର୍ଷ_Varp = ifaceSymp ? VN_AS(ifaceSymp->nodep(), Var) : nullptr;
        UASSERT_OBJ(iface_ଶୀର୍ଷ_Varp, nodep, "Can't find interface var ref: " << findName);
        ଫେରନ୍ତୁ iface_ଶୀର୍ଷ_Varp;
    }
    AstRSProd* findProd(Ast_ଗଣ୍ଠି* nodep, VSymEnt* parentEntp, ଅଚଳ string& name) {
        ଅଚଳ VSymEnt* ଅଚଳ foundp = parentEntp->findIdFallback(name);
        AstRSProd* foundNodep = foundp ? VN_CAST(foundp->nodep(), RSProd) : nullptr;
        ଯଦି (!foundNodep) {
            VSpellCheck speller;
            Link_ଗଣ୍ଠି_Matcher_Prod matcher;
            parentEntp->candidateIdFlat(&speller, &matcher, ମିଥ୍ୟା);
            ଅଚଳ string suggest = speller.best_Candidate_ସନ୍ଦେଶ(name);
            UINFO(1, "   ErrParseRef curSymp=se" << cvtToHex(parentEntp));
            nodep->v3error("Production " << Ast_ଗଣ୍ଠି::prettyNameQ(name) << " not found\n"
                                         << (suggest.empty() ? "" : nodep->warnMore() + suggest));
            ଫେରନ୍ତୁ nullptr;
        }
        ଫେରନ୍ତୁ foundNodep;
    }
    ଶୂନ୍ୟ duplicatePinError(AstPin* nodep, Ast_ଗଣ୍ଠି* origp, ଅଚଳ ଅକ୍ଷର* whatp) {
        nodep->v3error("Duplicate " << whatp << " connection: " << nodep->prettyNameQ() << '\n'
                                    << nodep->warnContextPrimary() << '\n'
                                    << origp->warnOther() << "... Location of original " << whatp
                                    << " connection\n"
                                    << origp->warnContextSecondary());
    }
    ଶୂନ୍ୟ markAndCheckPinDup(AstPin* nodep, Ast_ଗଣ୍ଠି* refp, ଅଚଳ ଅକ୍ଷର* whatp) {
        ଅଚଳ auto pair = m_usedPins.emplace(refp, nodep);
        ଯଦି (pair.second) {
            ଯଦି (!nodep->paramPath().empty())
                m_usedDefParamPins[refp].emplace(nodep->paramPath(), nodep);
            ଫେରନ୍ତୁ;
        } ଅନ୍ୟଥା {
            Ast_ଗଣ୍ଠି* ଅଚଳ origp = pair.first->second;
            ଯଦି (nodep->paramPath().empty() || VN_AS(origp, Pin)->paramPath().empty()) {
                duplicatePinError(nodep, origp, whatp);
                ଫେରନ୍ତୁ;
            }
        }

        auto& defParamPins = m_usedDefParamPins[refp];
        ଅଚଳ auto defParamIt = defParamPins.find(nodep->paramPath());
        ଯଦି (defParamIt != defParamPins.end()) {
            duplicatePinError(nodep, defParamIt->second, whatp);
            ଫେରନ୍ତୁ;
        }
        defParamPins.emplace(nodep->paramPath(), nodep);
    }
    ସ୍ଥିର ତାର୍କିକ extendsClass(ଅଚଳ AstClass* classp, ଅଚଳ AstClass* targetp) {
        // Return true if classp inherits from targetp through already resolved extends
        // If performance of this becomes a problem, use just a counter to check
        // exceeds some number of iterations then perform the more expensive analysis
        std::set<ଅଚଳ AstClass*> visited;
        std::vector<ଅଚଳ AstClass*> todo{classp};
        ଯେତେବେଳେ (!todo.empty()) {
            ଅଚଳ AstClass* ଅଚଳ currp = todo.back();
            todo.କାଢ଼_back();
            ଯଦି (currp == targetp) ଫେରନ୍ତୁ ସତ୍ୟ;
            ଯଦି (!visited.ଭର୍ତ୍ତି(currp).second) ଜାରି;
            ପାଇଁ (ଅଚଳ AstClassExtends* cextp = currp->extendsp(); cextp;
                 cextp = VN_AS(cextp->nextp(), ClassExtends)) {
                ଯଦି (ଅଚଳ AstClass* ଅଚଳ basep = cextp->classOrNullp()) todo.ଠେଲ_back(basep);
            }
        }
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }
    ସ୍ଥିର AstClocking* sensClockingp(Ast_ଗଣ୍ଠି* nodep) {
        // Return the clocking block referenced by nodep, either directly or through a modport
        ଯଦି (AstClocking* ଅଚଳ clockingp = VN_CAST(nodep, Clocking)) ଫେରନ୍ତୁ clockingp;
        ଯଦି (ଅଚଳ AstModportClockingRef* ଅଚଳ clockingRefp = VN_CAST(nodep, ModportClockingRef))
            ଫେରନ୍ତୁ clockingRefp->clockingp();
        ଫେରନ୍ତୁ nullptr;
    }
    VSymEnt* getCreateClockingEventSymEnt(AstClocking* clockingp) {
        AstVar* ଅଚଳ eventp = clockingp->ensureEventp(ସତ୍ୟ);
        ଯଦି (!eventp->user1p()) eventp->user1p(ନୂତନ VSymEnt{m_statep->symsp(), eventp});
        ଫେରନ୍ତୁ reinterpret_cast<VSymEnt*>(eventp->user1p());
    }
    VSymEnt* findInlinedDotsSym(Ast_ଗଣ୍ଠି* nodep, ଅଚଳ string& inlinedDots) {
        // Return scope that applies for searching, given renaming due to module inlining.
        // Dotted lookup is always relative to module, as maybe variable
        // name lower down with same scope name we want to ignore (t_math_divw)
        VSymEnt* dotSymp = m_modSymp;
        ଅଚଳ string inl = Ast_ଗଣ୍ଠି::dedotName(inlinedDots);
        string baddot;
        VSymEnt* okSymp = nullptr;
        dotSymp = m_statep->findDotted(nodep->fileline(), dotSymp, inl, baddot, okSymp, ମିଥ୍ୟା);
        UASSERT_OBJ(dotSymp, nodep,
                    "Couldn't resolve inlined scope " << Ast_ଗଣ୍ଠି::prettyNameQ(baddot)
                                                      << " in: " << inlinedDots);
        UINFO(9, indent() << "findInlinedDotsSym " << dotSymp
                          << " search start due to inlinedDots=" << inlinedDots);
        ଫେରନ୍ତୁ dotSymp;
    }
    VSymEnt* findDottedRefSymp(Ast_ଗଣ୍ଠି* nodep, VSymEnt* lookupSymp, ଅଚଳ string& dotted,
                               string& baddot, VSymEnt*& okSymp, ତାର୍କିକ* modportp = nullptr) {
        // Return scope to search for a relinked hierarchical reference
        // (AstVarXRef or AstEnumItemRef), or nullptr after error if not found
        // If modportp is given, set true when the reference went through a modport
        ଯଦି (modportp) *modportp = ମିଥ୍ୟା;
        VSymEnt* dotSymp = m_statep->findDotted(nodep->fileline(), lookupSymp, dotted, baddot,
                                                okSymp, ସତ୍ୟ);  // Maybe nullptr
        ଯଦି (!dotSymp) {
            nodep->v3error(
                "Can't find definition of "
                << (!baddot.empty() ? Ast_ଗଣ୍ଠି::prettyNameQ(baddot) : nodep->prettyNameQ()) << '\n'
                << nodep->warnContextPrimary());
            ଫେରନ୍ତୁ nullptr;
        }
        ଯଦି (ଅଚଳ AstVar* ଅଚଳ varp = VN_CAST(dotSymp->nodep(), Var)) {
            ଯଦି (ଅଚଳ AstIfaceRefDType* ଅଚଳ ifaceRefp
                = VN_CAST(varp->childDTypep(), IfaceRefDType)) {
                ଯଦି (ifaceRefp->modportp()) {
                    dotSymp = m_statep->get_ଗଣ୍ଠି_Sym(ifaceRefp->modportp());
                    ଯଦି (modportp) *modportp = ସତ୍ୟ;
                } ଅନ୍ୟଥା {
                    dotSymp = m_statep->get_ଗଣ୍ଠି_Sym(ifaceRefp->ifacep());
                }
            }
        } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstModportClockingRef* ଅଚଳ clockingRefp
                   = VN_CAST(dotSymp->nodep(), ModportClockingRef)) {
            dotSymp = m_statep->get_ଗଣ୍ଠି_Sym(clockingRefp->clockingp());
        }
        ଫେରନ୍ତୁ dotSymp;
    }

    ସ୍ଥିର ତାର୍କିକ isParamedClassRefDType(ଅଚଳ Ast_ଗଣ୍ଠି* classp) {
        ଯେତେବେଳେ (ଅଚଳ AstRefDType* ଅଚଳ refp = VN_CAST(classp, RefDType))
            classp = refp->subDTypep();
        ଫେରନ୍ତୁ (VN_IS(classp, ClassRefDType) && VN_AS(classp, ClassRefDType)->paramsp())
               || VN_IS(classp, ParamTypeDType);
    }
    ତାର୍କିକ isParamedClassRef(ଅଚଳ Ast_ଗଣ୍ଠି* nodep) {
        // Is this a parameterized reference to a class, or a reference to class parameter
        ଯଦି (ଅଚଳ auto* classRefp = VN_CAST(nodep, ClassOrPackageRef)) {
            ଯଦି (classRefp->paramsp()) ଫେରନ୍ତୁ ସତ୍ୟ;
            ଅଚଳ auto* classp = classRefp->classOrPackageNodep();
            ଯେତେବେଳେ (ଅଚଳ auto* typedefp = VN_CAST(classp, Typedef)) classp = typedefp->subDTypep();
            ଫେରନ୍ତୁ isParamedClassRefDType(classp);
        }
        ଫେରନ୍ତୁ isParamedClassRefDType(nodep);
    }
    VSymEnt* getThisClassSymp() {
        VSymEnt* classSymp = m_ds.m_dotSymp;
        ଯେତେବେଳେ (classSymp && !VN_IS(classSymp->nodep(), Class)) {
            classSymp = classSymp->parentp();
        }
        ଫେରନ୍ତୁ classSymp;
    }
    // Is the current scope within the given class, including within its nested classes
    ତାର୍କିକ insideClass(ଅଚଳ AstClass* classp) ଅଚଳ {
        ପାଇଁ (ଅଚଳ VSymEnt* symp = m_curSymp; symp; symp = symp->parentp()) {
            ଯଦି (symp->nodep() == classp) ଫେରନ୍ତୁ ସତ୍ୟ;
        }
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }
    // 'randomize() with' target class not yet specialized; defer to post-V3Param pass
    ତାର୍କିକ randTargetUnresolved() ଅଚଳ {
        ଫେରନ୍ତୁ m_statep->forPrimary() && m_currentWithp && m_randMethodCallp && !m_randSymp;
    }
    // Is 'funcp' of 'classp' virtual, directly or by overriding a virtual method of a base
    // class (IEEE 1800-2023 8.20)
    ତାର୍କିକ isVirtualMethod(ଅଚଳ Ast_ଗଣ୍ଠି_FTask* funcp, ଅଚଳ AstClass* classp) ଅଚଳ {
        ଯଦି (funcp->isVirtual()) ଫେରନ୍ତୁ ସତ୍ୟ;
        ପାଇଁ (ଅଚଳ AstClassExtends* cextp = classp->extendsp(); cextp;
             cextp = VN_AS(cextp->nextp(), ClassExtends)) {
            AstClass* ଅଚଳ basep = cextp->classOrNullp();
            ଯଦି (!basep) ଜାରି;
            ଅଚଳ VSymEnt* ଅଚଳ foundp = m_statep->get_ଗଣ୍ଠି_Sym(basep)->findIdFlat(funcp->name());
            ଅଚଳ Ast_ଗଣ୍ଠି_FTask* ଅଚଳ baseFuncp
                = foundp ? VN_CAST(foundp->nodep(), ଗଣ୍ଠି_FTask) : nullptr;
            ଅଚଳ AstClass* ଅଚଳ baseClassp = foundp && foundp->parentp()
                                                   ? VN_CAST(foundp->parentp()->nodep(), Class)
                                                   : nullptr;
            ଯଦି (baseFuncp && baseClassp && isVirtualMethod(baseFuncp, baseClassp)) ଫେରନ୍ତୁ ସତ୍ୟ;
        }
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }
    // Is imported 'implp' an implementation of interface method 'ifSymp' (IEEE 1800-2023
    // 8.26.2)? If its class does not derive from the interface class, it needs a forwarding
    // override in the derived class to be reachable through the interface.
    ତାର୍କିକ inheritedImplements(ଅଚଳ VSymEnt* implp, ଅଚଳ VSymEnt* ifSymp, ତାର୍କିକ& needForwardr) {
        needForwardr = ମିଥ୍ୟା;
        ଅଚଳ Ast_ଗଣ୍ଠି_FTask* ଅଚଳ implFuncp = VN_CAST(implp->nodep(), ଗଣ୍ଠି_FTask);
        ଯଦି (!implFuncp || implFuncp->pureVirtual() || implFuncp->isStatic()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଅଚଳ AstClass* ଅଚଳ implClassp
            = implp->parentp() ? VN_CAST(implp->parentp()->nodep(), Class) : nullptr;
        ଅଚଳ AstClass* ଅଚଳ ifClassp
            = ifSymp->parentp() ? VN_CAST(ifSymp->parentp()->nodep(), Class) : nullptr;
        ଯଦି (!implClassp || !ifClassp) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଯଦି (extendsClass(implClassp, ifClassp)) ଫେରନ୍ତୁ ସତ୍ୟ;
        ଯଦି (!isVirtualMethod(implFuncp, implClassp)) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        needForwardr = ସତ୍ୟ;
        ଫେରନ୍ତୁ ସତ୍ୟ;
    }
    // Add 'implp' override to 'classp' that calls super.implp, so the C++ method of an
    // implemented interface class is overridden
    ଶୂନ୍ୟ addInterfaceForward(AstClass* classp, Ast_ଗଣ୍ଠି_FTask* implp, AstClass* implClassp) {
        File_ଧାଡ଼ି* ଅଚଳ flp = implp->fileline();
        AstVar* fvarp = nullptr;
        Ast_ଗଣ୍ଠି_FTask* newp;
        ଯଦି (AstFunc* ଅଚଳ implFuncp = VN_CAST(implp, Func)) {
            fvarp = VN_AS(implFuncp->fvarp(), Var)->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);
            newp = ନୂତନ AstFunc{flp, implp->name(), nullptr, fvarp};
        } ଅନ୍ୟଥା {
            newp = ନୂତନ AstTask{flp, implp->name(), nullptr};
        }
        newp->classMethod(ସତ୍ୟ);
        newp->isVirtual(ସତ୍ୟ);
        newp->lifetime(implp->lifetime());
        AstArg* argsp = nullptr;
        ପାଇଁ (Ast_ଗଣ୍ଠି* stmtp = implp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstVar* ଅଚଳ portp = VN_CAST(stmtp, Var);
            ଯଦି (!portp || !portp->isIO()) ଜାରି;
            AstVar* ଅଚଳ newPortp = portp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);
            newp->addStmtsp(newPortp);
            argsp = Ast_ଗଣ୍ଠି::addNext(
                argsp, ନୂତନ AstArg{flp, "", ନୂତନ AstVarRef{flp, newPortp, VAccess::READ}});
        }
        Ast_ଗଣ୍ଠି_FTask_Ref* callp;
        ଯଦି (AstFunc* ଅଚଳ implFuncp = VN_CAST(implp, Func)) {
            callp = ନୂତନ AstFuncRef{flp, implFuncp, argsp};
        } ଅନ୍ୟଥା {
            callp = ନୂତନ AstTaskRef{flp, VN_AS(implp, Task), argsp};
        }
        callp->classOrPackagep(implClassp);
        callp->superReference(ସତ୍ୟ);
        ଯଦି (fvarp) {
            newp->addStmtsp(ନୂତନ AstAssign{flp, ନୂତନ AstVarRef{flp, fvarp, VAccess::WRITE}, callp});
        } ଅନ୍ୟଥା {
            newp->addStmtsp(callp->makeStmt());
        }
        UINFO(9, indent() << "Interface forward " << newp);
        classp->addMembersp(newp);
    }
    ଶୂନ୍ୟ importDerivedClass(AstClass* derivedClassp, VSymEnt* baseSymp, AstClass* baseClassp) {
        // Also used for standard 'extends' from a base class
        UINFO(8, indent() << "importDerivedClass to " << derivedClassp << " from " << baseClassp);
        ପାଇଁ (VSymEnt::const_iterator it = baseSymp->begin(); it != baseSymp->end(); ++it) {
            ଯଦି (Ast_ଗଣ୍ଠି* baseSubp = it->second->nodep()) {
                UINFO(8, indent() << "  SymFunc " << baseSubp);
                ଅଚଳ string impOrExtends
                    = baseClassp->isInterfaceClass() ? " implements " : " extends ";
                ଯଦି (VN_IS(baseSubp, ଗଣ୍ଠି_FTask)) {
                    ଅଚଳ VSymEnt* ଅଚଳ foundp = m_curSymp->findIdFlat(baseSubp->name());
                    ଅଚଳ Ast_ଗଣ୍ଠି_FTask* ଅଚଳ baseFuncp = VN_CAST(baseSubp, ଗଣ୍ଠି_FTask);
                    ଯଦି (!baseFuncp || !baseFuncp->pureVirtual()) ଜାରି;
                    ତାର୍କିକ needForward = ମିଥ୍ୟା;
                    ଅଚଳ ତାର୍କିକ existsInDerived
                        = foundp
                          && (!foundp->imported()
                              || inheritedImplements(foundp, it->second, needForward));
                    ଯଦି (needForward && m_statep->forPrimary()
                        && !derivedClassp->isInterfaceClass()) {
                        m_ifForwards.emplace(baseSubp->name(), foundp);
                    }
                    ଯଦି (m_statep->forPrimary() && !existsInDerived
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
                    ଅଚଳ auto itn = m_ifClassImpNames.find(baseSubp->name());
                    ଯଦି (m_statep->forPrimary() && !existsInDerived
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
                ଯଦି (VN_IS(baseSubp, Constraint)) {
                    ଅଚଳ VSymEnt* ଅଚଳ foundp = m_curSymp->findIdFlat(baseSubp->name());
                    ଅଚଳ AstConstraint* ଅଚଳ baseFuncp = VN_CAST(baseSubp, Constraint);
                    ଯଦି (!baseFuncp || !baseFuncp->isKwdPure()) ଜାରି;
                    ଅଚଳ ତାର୍କିକ existsInDerived = foundp && !foundp->imported();
                    ଯଦି (m_statep->forPrimary() && !existsInDerived
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
    ଶୂନ୍ୟ importSymbolsFromExtended(AstClass* ଅଚଳ nodep, AstClassExtends* ଅଚଳ cextp) {
        AstClass* ଅଚଳ baseClassp = cextp->classp();
        VSymEnt* ଅଚଳ srcp = m_statep->get_ଗଣ୍ଠି_Sym(baseClassp);
        importDerivedClass(nodep, srcp, baseClassp);
        ଯଦି (!cextp->isImplements()) m_curSymp->importFromClass(m_statep->symsp(), srcp);
    }
    ଶୂନ୍ୟ classExtendImport(AstClass* nodep) {
        // A class reference might be to a class that is later in Ast due to
        // e.g. parameterization or referring to a "class (type T) extends T"
        // Resolve it so later Class:: references into its base classes work
        symIterateNull(nodep, m_statep->get_ଗଣ୍ଠି_Sym(nodep));
    }

    ଶୂନ୍ୟ checkDeclOrder(Ast_ଗଣ୍ଠି* nodep, Ast_ଗଣ୍ଠି* declp) {
        ଅଚଳ uint32_t decl_Token_ଅଙ୍କ = declp->decl_Token_ଅଙ୍କ();
        ଯଦି (!decl_Token_ଅଙ୍କ) ଫେରନ୍ତୁ;  // Not implemented/valid on this object
        ଯଦି (nodep->fileline()->token_ଅଙ୍କ() < decl_Token_ଅଙ୍କ) {
            UINFO(1, "Related node " << nodep->fileline()->token_ଅଙ୍କ() << " " << nodep);
            UINFO(1, "Related decl " << decl_Token_ଅଙ୍କ << " " << declp);
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
    ଶୂନ୍ୟ checkMemberDeclOrder(Ast_ଗଣ୍ଠି* nodep, AstMemberDType* declp) {
        ଅଚଳ uint32_t decl_Token_ଅଙ୍କ = declp->fileline()->token_ଅଙ୍କ();
        ଯଦି (nodep->fileline()->token_ଅଙ୍କ() < decl_Token_ଅଙ୍କ) {
            UINFO(1, "Related node " << nodep->fileline()->token_ଅଙ୍କ() << " " << nodep);
            UINFO(1, "Related decl " << decl_Token_ଅଙ୍କ << " " << declp);
            nodep->v3error("Reference to "
                           << nodep->prettyNameQ() << " before declaration (IEEE 1800-2023 6.18)\n"
                           << nodep->warnMore()
                           << "... Suggest move the declaration before the reference\n"
                           << nodep->warnContextPrimary() << '\n'
                           << declp->warnOther() << "... Location of original declaration\n"
                           << declp->warnContextSecondary());
        }
    }

    ଶୂନ୍ୟ replaceWithCheckBreak(Ast_ଗଣ୍ଠି* oldp, Ast_ଗଣ୍ଠି_DType* newp) {
        // Flag now to avoid V3Broken throwing an internal error
        ଯଦି (oldp->wouldBreak(newp)) {
            newp->v3error(
                "Data type used where a non-data type is expected: " << newp->prettyNameQ());
            oldp->replaceWith(ନୂତନ AstConst{newp->fileline(), AstConst::BitFalseErroring{}});
        } ଅନ୍ୟଥା {
            oldp->replaceWith(newp);
        }
    }

    // Marks the current module to be revisited after the initial AST iteration
    ଶୂନ୍ୟ revisitLater(Ast_ଗଣ୍ଠି* deferredNodep) {
        // Need to revisit entire module to build up all the necessary context
        m_lastDeferredp = deferredNodep;
        m_modulesToRevisit.ଭର୍ତ୍ତି(std::make_pair(m_modp->name(), m_modp));
    }

    ଶୂନ୍ୟ updateVarUse(AstVar* nodep) {
        // Avoid dotted.PARAM false positive when in a parameter block
        // that is if ()'ed off by same dotted name as another block
        ଯଦି (nodep && nodep->isParam()) nodep->usedParam(ସତ୍ୟ);
    }

    ସ୍ଥିର VSymEnt* findIdFallbackSkipMemberDType(VSymEnt* lookp, ଅଚଳ string& name) {
        VSymEnt* shadowEntp = nullptr;  // Shadowing variable: not a type, kept for error report
        ଯେତେବେଳେ (lookp) {
            VSymEnt* ଅଚଳ foundp = lookp->findIdFlat(name);
            ଯଦି (foundp && !VN_IS(foundp->nodep(), MemberDType)) {
                // Non-type entries are not type candidates
                // (IEEE 1800-2023 6.18); skip them so an enclosing
                // type is found, but keep one to preserve the "found: ..." error.
                ଯଦି (VN_IS(foundp->nodep(), Typedef) || VN_IS(foundp->nodep(), ParamTypeDType)
                    || VN_IS(foundp->nodep(), Class)) {
                    ଫେରନ୍ତୁ foundp;
                }
                ଯଦି (!shadowEntp) shadowEntp = foundp;
            }
            lookp = lookp->fallbackp();
        }
        ଫେରନ୍ତୁ shadowEntp;
    }

    ଶୂନ୍ୟ symIterateChildren(Ast_ଗଣ୍ଠି* nodep, VSymEnt* symp) {
        // Iterate children, changing to given context, with restore to old context
        VL_RESTORER_COPY(m_ds);
        VL_RESTORER(m_curSymp);
        m_curSymp = symp;
        m_ds.init(m_curSymp);
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ symIterateNull(Ast_ଗଣ୍ଠି* nodep, VSymEnt* symp) {
        // Iterate node, changing to given context, with restore to old context
        VL_RESTORER_COPY(m_ds);
        VL_RESTORER(m_curSymp);
        m_curSymp = symp;
        m_ds.init(m_curSymp);
        iterateNull(nodep);
    }
    ସ୍ଥିର ଅଚଳ Ast_ଗଣ୍ଠି_DType* get_ପଦସମଷ୍ଟି_DTypep(ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* selp) {
        ଯେତେବେଳେ (ଅଚଳ Ast_ଗଣ୍ଠି_Pre_Sel* ଅଚଳ sp = VN_CAST(selp, ଗଣ୍ଠି_Pre_Sel)) selp = sp->fromp();
        ଯଦି (ଅଚଳ AstMemberSel* ଅଚଳ sp = VN_CAST(selp, MemberSel)) {
            ଯଦି (ଅଚଳ Ast_ଗଣ୍ଠି_DType* dtypep = get_ପଦସମଷ୍ଟି_DTypep(sp->fromp())) {
                ଯଦି (ଅଚଳ AstClassRefDType* ଅଚଳ classRefp = VN_CAST(dtypep, ClassRefDType)) {
                    ଅଚଳ AstClass* ଅଚଳ classp = classRefp->classp();
                    ଅଚଳ ତାର୍କିକ found = classp->existsMember(
                        [&dtypep, name = selp->name()](ଅଚଳ AstClass*, ଅଚଳ AstVar* nodep) {
                            dtypep = nodep->childDTypep();
                            ଫେରନ୍ତୁ nodep->name() == name;
                        });
                    ଯଦି (found) ଫେରନ୍ତୁ dtypep->elemDTypep();
                    selp->v3error("Class " << classRefp->prettyNameQ()
                                           << " does not contain field " << selp->prettyNameQ());
                } ଅନ୍ୟଥା {
                    selp->v3fatalSrc("Member selection on expression of type "
                                     << dtypep->prettyDTypeNameQ()
                                     << ", which is not a class type");
                }
            }
        } ଅନ୍ୟଥା ଯଦି (ଅଚଳ Ast_ଗଣ୍ଠି_Var_Ref* ଅଚଳ varRefp = VN_CAST(selp, ଗଣ୍ଠି_Var_Ref)) {
            ଫେରନ୍ତୁ varRefp->varp()->childDTypep()->elemDTypep();
        }
        ଫେରନ୍ତୁ nullptr;
    }
    ସ୍ଥିର ଅଚଳ AstVar* getNextVarp(ଅଚଳ Ast_ଗଣ୍ଠି* stmtsp) {
        // Only IO ports, as parameters are on paramsp(), not the pinsp() list paired here
        ଯେତେବେଳେ (stmtsp) {
            ଯଦି (ଅଚଳ AstVar* ଅଚଳ varp = VN_CAST(stmtsp, Var)) {
                ଯଦି (varp->isIO()) ଫେରନ୍ତୁ varp;
            }
            stmtsp = stmtsp->nextp();
        }
        ଫେରନ୍ତୁ nullptr;
    }
    // Introduce implicit parameters for modules with generic interafeces
    ଶୂନ୍ୟ addImplicitParametersOfGenericIface(AstCell* ଅଚଳ nodep, ଅଚଳ AstModule* ଅଚଳ modp) {
        // Get Param number
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା param_ଅଙ୍କ = 1;
        ପାଇଁ (AstPin* paramp = nodep->paramsp(); paramp; paramp = VN_CAST(paramp->nextp(), Pin)) {
            ++param_ଅଙ୍କ;
        }

        // Add each implicit parameter type
        ଅଚଳ AstVar* modIfaceVarp = getNextVarp(modp->stmtsp());
        ପାଇଁ (ଅଚଳ AstPin* pinp = nodep->pinsp(); pinp && modIfaceVarp;
             pinp = VN_CAST(pinp->nextp(), Pin),
                           modIfaceVarp = getNextVarp(modIfaceVarp->nextp())) {
            ଯଦି (modIfaceVarp->varType() != VVarType::IFACEREF
                || !VN_IS(modIfaceVarp->childDTypep()->elemDTypep(), IfaceGenericDType)) {
                ଜାରି;
            }
            Ast_ଗଣ୍ଠି* exprp = pinp->exprp();
            ଯଦି (!exprp) {
                modIfaceVarp->v3error("Interface port "
                                      << modIfaceVarp->prettyNameQ()
                                      << " is not connected to interface/modport pin expression");
                ଜାରି;
            }
            ଯେତେବେଳେ (ଅଚଳ Ast_ଗଣ୍ଠି_Pre_Sel* ଅଚଳ preSelp = VN_CAST(exprp, ଗଣ୍ଠି_Pre_Sel)) {
                exprp = preSelp->fromp();
            }
            // Resolve pin expression to the enclosing module's port Var. At primary
            // LinkDot the expression may still be an AstParseRef, so also look up by name.
            AstVar* enclosingVarp = nullptr;
            ଅଚଳ AstVarRef* ଅଚଳ varRefp = VN_CAST(exprp, VarRef);
            ଯଦି (varRefp) {
                enclosingVarp = varRefp->varp();
            } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstParseRef* ଅଚଳ parseRefp = VN_CAST(exprp, ParseRef)) {
                ଯଦି (m_modp) {
                    ଯଦି (VSymEnt* ଅଚଳ symp
                        = m_statep->get_ଗଣ୍ଠି_Sym(m_modp)->findIdFlat(parseRefp->name())) {
                        enclosingVarp = VN_CAST(symp->nodep(), Var);
                    }
                }
            }
            ଯଦି (enclosingVarp && enclosingVarp->varType() == VVarType::IFACEREF
                && VN_IS(enclosingVarp->childDTypep()->skipRefp(), IfaceGenericDType)) {
                // Nested generic-iface forwarding (#7454): enclosing port is itself still
                // generic, so emit a placeholder __VGIfaceParam pin carrying a VarRef to
                // the outer port. V3Param rewrites it to the concrete IfaceRefDType once
                // the enclosing module is specialized (see
                // ParamProcessor::resolveGenericIfaceForwardingPins for the ordering
                // constraint that keeps the rewrite inside V3Param).
                AstVarRef* ଅଚଳ fwdRefp
                    = ନୂତନ AstVarRef{exprp->fileline(), enclosingVarp, VAccess::READ};
                AstPin* ଅଚଳ newPinp = ନୂତନ AstPin{
                    pinp->fileline(), param_ଅଙ୍କ, "__VGIfaceParam" + modIfaceVarp->name(), fwdRefp};
                newPinp->param(ସତ୍ୟ);
                visit(newPinp);
                nodep->addParamsp(newPinp);
            } ଅନ୍ୟଥା ଯଦି (varRefp) {
                ଅଚଳ AstVar* ଅଚଳ varp = varRefp->varp();
                ଯଦି (ଅଚଳ AstIfaceRefDType* ଅଚଳ refp
                    = VN_CAST(varp->childDTypep()->elemDTypep(), IfaceRefDType)) {
                    AstIface* ଅଚଳ ifacep = VN_AS(refp->cellp()->modp(), Iface);
                    AstIfaceRefDType* newIfaceRefp;
                    ଯଦି (refp->modportp()) {
                        newIfaceRefp = ନୂତନ AstIfaceRefDType{
                            refp->fileline(), refp->modportFileline(), m_modp->name(),
                            ifacep->name(), refp->modportName()};
                    } ଅନ୍ୟଥା {
                        newIfaceRefp = ନୂତନ AstIfaceRefDType{refp->fileline(), m_modp->name(),
                                                            ifacep->name()};
                    }
                    newIfaceRefp->ifacep(ifacep);
                    ଯଦି (refp->cellp()->paramsp()) {
                        newIfaceRefp->addParamsp(refp->cellp()->paramsp()->clone_ବୃକ୍ଷ(ସତ୍ୟ));
                    }
                    UASSERT_OBJ(pinp->name().find("__pinNumber") == 0
                                    || pinp->name() == modIfaceVarp->name(),
                                pinp, "Not found interface with such name");
                    AstPin* ଅଚଳ newPinp
                        = ନୂତନ AstPin{pinp->fileline(), param_ଅଙ୍କ,
                                     "__VGIfaceParam" + modIfaceVarp->name(), newIfaceRefp};
                    newPinp->param(ସତ୍ୟ);
                    visit(newPinp);
                    nodep->addParamsp(newPinp);
                } ଅନ୍ୟଥା {
                    varRefp->v3error("Generic interfaces can only connect to an interface and "
                                     << varp->prettyNameQ() << " is "
                                     << (varp->childDTypep()
                                             ? "of type " + varp->childDTypep()->prettyDTypeNameQ()
                                             : "not an interface"));
                }
            } ଅନ୍ୟଥା {
                exprp->v3error("Expected an interface but " << exprp->prettyNameQ()
                                                            << " is not an interface");
            }
            ++param_ଅଙ୍କ;
        }
    }

    // Returns true and issues error iff 'varp' cannot be accesed with a VarXRef
    // in a hier_block given by 'scopeSymp'
    ତାର୍କିକ errorHierNonPort(AstVarXRef* refp, AstVar* varp, VSymEnt* scopeSymp) {
        // OK to access ports on hier_block
        ଯଦି (varp->isIO()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        // OK if not an instance
        ଅଚଳ AstCell* ଅଚଳ cellp = VN_CAST(scopeSymp->nodep(), Cell);
        ଯଦି (!cellp) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        // Ok if not a hier_block
        ଅଚଳ Ast_ଗଣ୍ଠି_Module* ଅଚଳ modp = cellp->modp();
        ଯଦି (!modp->hierBlock()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        // Bad
        refp->v3error("Cannot access non-port symbols inside hierarchical block");
        ଫେରନ୍ତୁ ସତ୍ୟ;
    }

#define LINKDOT_VISIT_START() \
    VL_RESTORER(m_indent); \
    ++m_indent;

    string indent() ଅଚଳ {
        string result = "";
        result.ଭର୍ତ୍ତି(0, m_indent, ':');
        ଫେରନ୍ତୁ result + " ";
    }

    // VISITORS
    ଶୂନ୍ୟ visit(AstNetlist* nodep) override {
        // Recurse..., backward as must do packages before using packages
        iterateChildrenBackwardsConst(nodep);
    }
    ଶୂନ୍ୟ visit(AstTypeTable*) override {}
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Module* nodep) override {
        ଯଦି (nodep->dead()) ଫେରନ୍ତୁ;
        LINKDOT_VISIT_START();
        UINFO(8, indent() << "visit " << nodep);
        VL_RESTORER(m_genericIfaceModule);
        ଯଦି (ଅଚଳ AstModule* ଅଚଳ modp = VN_CAST(nodep, Module)) {
            m_genericIfaceModule = modp->hasGenericIface();
        }
        checkNoDot(nodep);
        m_ds.init(m_curSymp);
        m_ds.m_dotSymp = m_curSymp = m_modSymp
            = m_statep->get_ଗଣ୍ଠି_Sym(nodep);  // Until overridden by a SCOPE
        m_cellp = nullptr;
        m_modp = nodep;
        m_modport_ଅଙ୍କ = 0;
        iterateChildren(nodep);
        m_modp = nullptr;
        m_ds.m_dotSymp = m_curSymp = m_modSymp = nullptr;
        m_ds.m_dotPos = DP_NONE;
        m_ds.m_dotErr = ମିଥ୍ୟା;
    }
    ଶୂନ୍ୟ visit(AstScope* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(8, indent() << "visit " << nodep);
        checkNoDot(nodep);
        VL_RESTORER(m_modSymp);
        VL_RESTORER(m_curSymp);
        m_ds.m_dotSymp = m_curSymp = m_modSymp = m_statep->getScopeSym(nodep);
        iterateChildren(nodep);
        m_ds.m_dotSymp = m_curSymp = m_modSymp = nullptr;
    }
    ଶୂନ୍ୟ visit(AstCellInline* nodep) override {
        LINKDOT_VISIT_START();
        checkNoDot(nodep);
        UASSERT_OBJ(m_statep->forScopeCreation(), nodep, "CellInline should not exist yet");
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep->unlinkFrBack()), nodep);
    }
    ଶୂନ୍ୟ visit(AstCell* nodep) override {
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
            ଯଦି (VN_IS(nodep->modp(), NotFoundModule)) {
                // Prevent warnings about missing pin connects
                ଯଦି (nodep->pinsp()) nodep->pinsp()->unlinkFrBackWithNext()->delete_ବୃକ୍ଷ();
                ଯଦି (nodep->paramsp()) nodep->paramsp()->unlinkFrBackWithNext()->delete_ବୃକ୍ଷ();
            }
            // Need to pass the module info to this cell, so we can link up the pin names
            // However can't use m_curSymp as pin connections need to use the
            // instantiator's symbols
            ଅନ୍ୟଥା {
                m_pinSymp = m_statep->get_ଗଣ୍ଠି_Sym(nodep->modp());
                UINFO(4, indent() << "(Backto) visit " << nodep);
                // UINFOTREE(1, nodep, "", "linkcell");
                // UINFOTREE(1, nodep->modp(), "", "linkcemd");
                iterateChildren(nodep);

                ଯଦି (m_statep->forPrimary())
                    ଯଦି (ଅଚଳ AstModule* ଅଚଳ modp = VN_CAST(nodep->modp(), Module)) {
                        ଯଦି (modp->hasGenericIface())
                            addImplicitParametersOfGenericIface(nodep, modp);
                    }
            }
        }
        // Parent module inherits child's publicity
        // This is done bottom up in the LinkBotupVisitor stage
    }
    ଶୂନ୍ୟ visit(AstClassRefDType* nodep) override {
        // Cell: Recurse inside or cleanup not founds
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        // Can be under dot if called as package::class and that class resolves, so no checkNoDot
        VL_RESTORER_CLEAR(m_usedPins);
        VL_RESTORER_CLEAR(m_usedDefParamPins);
        UASSERT_OBJ(nodep->classp(), nodep, "ClassRef has unlinked class");
        UASSERT_OBJ(m_statep->forPrimary() || !nodep->paramsp() || V3Error::error_ଗଣନା(), nodep,
                    "class reference parameter not removed by V3Param");
        VL_RESTORER(m_pinSymp);
        {
            // ClassRef's have pins, so track
            m_pinSymp = m_statep->get_ଗଣ୍ଠି_Sym(nodep->classp());
            UINFO(4, indent() << "(Backto) visit " << nodep);
            // UINFOTREE(1, nodep, "", "linkcell");
            // UINFOTREE(1, nodep->modp(), "", "linkcemd");
            iterateChildren(nodep);
        }
    }
    ଶୂନ୍ୟ visit(AstPin* nodep) override {
        // Pin: Link to submodule's port
        LINKDOT_VISIT_START();
        checkNoDot(nodep);
        iterateChildren(nodep);
        ଯଦି (!nodep->modVarp()) {
            UASSERT_OBJ(m_pinSymp, nodep, "Pin not under instance?");
            VSymEnt* ଅଚଳ foundp = m_pinSymp->findIdFlat(nodep->name());
            ଅଚଳ ଅକ୍ଷର* ଅଚଳ whatp = nodep->param() ? "parameter" : "pin";
            ଯଦି (!foundp) {
                ଯଦି (m_cellp && VN_IS(m_cellp->modp(), Primitive)
                    && (nodep->name() == "__paramNumber1" || nodep->name() == "__paramNumber2")) {
                    // Primitive parameter is really a delay2 we can just ignore
                    VL_DO_DANGLING(ଠେଲ_Deletep(nodep->unlinkFrBack()), nodep);
                    ଫେରନ୍ତୁ;
                } ଅନ୍ୟଥା ଯଦି (nodep->param() && !nodep->exprp() && !nodep->svDotName()) {
                    // Placeholder pin the parser makes for an empty '#()', to signal
                    // the user did type the '#()'. No further use, deleting.
                    VL_DO_DANGLING(ଠେଲ_Deletep(nodep->unlinkFrBack()), nodep);
                    ଫେରନ୍ତୁ;
                } ଅନ୍ୟଥା {
                    ଅଚଳ std::string suggest
                        = (nodep->param()
                               ? m_statep->suggestSymFlat(m_pinSymp, nodep->name(),
                                                          Link_ଗଣ୍ଠି_Matcher_Var_Param{}, ମିଥ୍ୟା)
                               : m_statep->suggestSymFlat(m_pinSymp, nodep->name(),
                                                          Link_ଗଣ୍ଠି_Matcher_Var_IO{}, ମିଥ୍ୟା));
                    ଅଚଳ std::string decl
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
                    VL_DO_DANGLING(ଠେଲ_Deletep(nodep->unlinkFrBack()), nodep);
                    ଫେରନ୍ତୁ;
                }
            }
            VVarType refVarType = VVarType::UNKNOWN;
            ତାର୍କିକ wrongPinType = ମିଥ୍ୟା;
            ଯଦି (AstVar* ଅଚଳ varp = VN_CAST(foundp->nodep(), Var)) {
                ଯଦି (varp->isIO() || varp->isParam() || varp->isIfaceRef()) {
                    refVarType = varp->varType();
                    nodep->modVarp(varp);
                } ଅନ୍ୟଥା {
                    wrongPinType = ସତ୍ୟ;
                }
            } ଅନ୍ୟଥା ଯଦି (AstParamTypeDType* ଅଚଳ typep = VN_CAST(foundp->nodep(), ParamTypeDType)) {
                refVarType = typep->varType();
                nodep->modPTypep(typep);
            } ଅନ୍ୟଥା {
                wrongPinType = ସତ୍ୟ;
            }
            // Don't connect parameter pin to module ports or vice versa
            ଯଦି (nodep->param() != (refVarType == VVarType::GPARAM)) wrongPinType = ସତ୍ୟ;
            ଯଦି (wrongPinType) {
                string targetType = LinkDotState::ଗଣ୍ଠି_Text_Type(foundp->nodep());
                targetType = VString::aOrAn(targetType) + ' ' + targetType;
                ଯଦି (nodep->param()) {
                    nodep->v3error("Instance attempts to override "
                                   << nodep->prettyNameQ() << " as a " << whatp << ", but it is "
                                   << targetType);
                } ଅନ୍ୟଥା {
                    nodep->v3error("Instance attempts to connect to "
                                   << nodep->prettyNameQ() << ", but it is " << targetType);
                }
                ଫେରନ୍ତୁ;
            }
            markAndCheckPinDup(nodep, foundp->nodep(), whatp);
        }
        // Early return() above when deleted
    }

    ସ୍ଥିର ତାର୍କିକ isDotThisReference(ଅଚଳ AstDot* nodep) {
        ଫେରନ୍ତୁ VN_IS(nodep->lhsp(), ParseRef) && nodep->lhsp()->name() == "this";
    }

    ଶୂନ୍ୟ visit(AstDot* nodep) override {
        // Legal under a DOT: AstDot, AstParseRef, AstPackageRef, AstNodeSel, AstPackArrayDType
        //    also a DOT can be part of an expression, but only above plus
        //    AstFTaskRef are legal children
        // Dot(PackageRef, ParseRef(text))
        // Dot(Dot(ClassOrPackageRef,ClassOrPackageRef), ParseRef(text))
        // Dot(Dot(Dot(ParseRef(text), ...
        ଯଦି (nodep->user3SetOnce()) ଫେରନ୍ତୁ;
        LINKDOT_VISIT_START();
        UINFO(8, indent() << "visit " << nodep);
        UINFO(8, indent() << m_ds.ascii());
        ଅଚଳ DotStates lastStates = m_ds;
        ଅଚଳ ତାର୍କିକ start = (m_ds.m_dotPos == DP_NONE);  // Save, as m_dotp will be changed
        ଅଚଳ DotPosition initialDotPos = m_ds.m_dotPos;
        VL_RESTORER(m_randSymp);
        {
            ଯଦି (start) {  // Starting dot sequence
                UINFOTREE(9, nodep, "", "dot-in");
                // Keep deferral from visit(AstMethodCall) for unspecialized rand target
                m_ds.init(m_curSymp, m_ds.m_unresolvedClass && randTargetUnresolved());
            }
            m_ds.m_dotp = nodep;  // Always, not just at start
            m_ds.m_dotPos = DP_FIRST;

            ଯଦି (isDotThisReference(nodep)) {
                VSymEnt* classSymp = getThisClassSymp();
                // In 'randomize() with { this.member }', 'this' refers to randomized
                // object, not the calling class (IEEE 1800-2023 18.7)
                ଯଦି (m_randSymp && m_currentWithp) classSymp = m_randSymp;
                ଯଦି (!classSymp && randTargetUnresolved()) {
                    // Bind 'this' in post-V3Param pass
                } ଅନ୍ୟଥା ଯଦି (!classSymp) {
                    nodep->v3error("'this' used outside class (IEEE 1800-2023 8.11)");
                    m_ds.m_dotErr = ସତ୍ୟ;
                } ଅନ୍ୟଥା {
                    m_ds.m_dotSymp = classSymp;
                    UINFO(8, indent() << "this. " << m_ds.ascii());
                }
            } ଅନ୍ୟଥା ଯଦି (VN_IS(nodep->lhsp(), ParseRef) && nodep->lhsp()->name() == "super") {
                ଅଚଳ VSymEnt* classSymp = getThisClassSymp();
                ଯଦି (!classSymp) {
                    nodep->v3error("'super' used outside class (IEEE 1800-2023 8.15)");
                    m_ds.m_dotErr = ସତ୍ୟ;
                } ଅନ୍ୟଥା {
                    ଅଚଳ auto classp = VN_AS(classSymp->nodep(), Class);
                    ଅଚଳ auto cextp = classp->extendsp();
                    ଯଦି (!cextp) {
                        nodep->v3error("'super' used on non-extended class (IEEE 1800-2023 8.15)");
                        m_ds.m_dotErr = ସତ୍ୟ;
                    } ଅନ୍ୟଥା {
                        ଯଦି (m_statep->forPrimary()
                            && m_extendsParam.find(classp) != m_extendsParam.end()) {
                            UINFO(9, indent()
                                         << "deferring until post-V3Param: " << nodep->lhsp());
                            m_ds.m_unresolvedClass = ସତ୍ୟ;
                        } ଅନ୍ୟଥା {
                            ଅଚଳ auto baseClassp = cextp->classp();
                            UASSERT_OBJ(baseClassp, nodep, "Bad superclass");
                            m_ds.m_dotSymp = m_statep->get_ଗଣ୍ଠି_Sym(baseClassp);
                            m_ds.m_super = ସତ୍ୟ;
                            UINFO(8, indent() << "super. " << m_ds.ascii());
                        }
                    }
                }
            } ଅନ୍ୟଥା ଯଦି (AstClassOrPackageRef* ଅଚଳ lhsp
                       = VN_CAST(nodep->lhsp(), ClassOrPackageRef)) {
                // m_ds.m_dotText communicates the cell prefix between stages
                UINFO(8, indent() << "iter.lhs   " << m_ds.ascii() << " " << nodep);
                iterateAndNextNull(lhsp);
                ଯଦି (!lhsp->classOrPackageSkipp() && lhsp->name() != "local::") {
                    revisitLater(nodep);
                    m_ds = lastStates;
                    // Resolve function args before bailing
                    ଯଦି (Ast_ଗଣ୍ଠି_FTask_Ref* ଅଚଳ ftaskrefp = VN_CAST(nodep->rhsp(), ଗଣ୍ଠି_FTask_Ref)) {
                        iterateAndNextNull(ftaskrefp->argsp());
                    }
                    ଫେରନ୍ତୁ;
                }
                m_ds.m_dotPos = DP_PACKAGE;
                // nodep->lhsp() may be a new node
                ଯଦି (AstClassOrPackageRef* ଅଚଳ classOrPackageRefp
                    = VN_CAST(nodep->lhsp(), ClassOrPackageRef)) {
                    ଯଦି (Ast_ଗଣ୍ଠି* ଅଚଳ classOrPackageNodep
                        = classOrPackageRefp->classOrPackageSkipp()) {
                        m_ds.m_dotSymp = m_statep->get_ଗଣ୍ଠି_Sym(classOrPackageNodep);
                    }
                }
                UINFO(8, indent() << "iter.ldone " << m_ds.ascii() << " " << nodep);
            } ଅନ୍ୟଥା ଯଦି (VN_IS(nodep->lhsp(), Dot) && VN_AS(nodep->lhsp(), Dot)->colon()) {
                // m_ds.m_dotText communicates the cell prefix between stages
                UINFO(8, indent() << "iter.lhs   " << m_ds.ascii() << " " << nodep);
                m_ds.m_dotPos = DP_PACKAGE;
                iterateAndNextNull(nodep->lhsp());
                // nodep->lhsp() may be a new node
                ଯଦି (AstClassOrPackageRef* ଅଚଳ crefp
                    = VN_CAST(nodep->lhsp(), ClassOrPackageRef)) {
                    ଯଦି (!crefp->classOrPackageSkipp()) {
                        revisitLater(nodep);
                        m_ds = lastStates;
                        // Resolve function args before bailing
                        ଯଦି (Ast_ଗଣ୍ଠି_FTask_Ref* ଅଚଳ ftaskrefp
                            = VN_CAST(nodep->rhsp(), ଗଣ୍ଠି_FTask_Ref)) {
                            iterateAndNextNull(ftaskrefp->argsp());
                        }
                        ଫେରନ୍ତୁ;
                    }
                }
                ଯଦି (m_lastDeferredp == nodep->lhsp()) {
                    // LHS got deferred, so this node won't be resolved. Defer it too
                    m_ds = lastStates;
                    // Resolve function args before bailing
                    ଯଦି (Ast_ଗଣ୍ଠି_FTask_Ref* ଅଚଳ ftaskrefp = VN_CAST(nodep->rhsp(), ଗଣ୍ଠି_FTask_Ref)) {
                        iterateAndNextNull(ftaskrefp->argsp());
                    }
                    ଫେରନ୍ତୁ;
                }
                UINFO(8, indent() << "iter.ldone " << m_ds.ascii() << " " << nodep);
            } ଅନ୍ୟଥା {
                m_ds.m_dotPos = DP_FIRST;
                m_ds.m_unresolvedGenericIface = ମିଥ୍ୟା;
                UINFO(8, indent() << "iter.lhs   " << m_ds.ascii() << " " << nodep);
                iterateAndNextNull(nodep->lhsp());
                UINFO(8, indent() << "iter.ldone " << m_ds.ascii() << " " << nodep);
                ଯଦି (m_ds.m_unresolvedGenericIface) {
                    m_ds.m_dotPos = initialDotPos;
                    ଫେରନ୍ତୁ;
                }
                // UINFOTREE(9, nodep, "", "dot-lho");
            }
            ଯଦି (m_statep->forPrimary() && isParamedClassRef(nodep->lhsp())) {
                // Dots of paramed classes will be linked after deparameterization
                UINFO(9, indent() << "deferring until post-V3Param: " << nodep->lhsp());
                m_ds.m_unresolvedClass = ସତ୍ୟ;
            }
            ଯଦି (m_ds.m_unresolvedCell
                && (VN_IS(nodep->lhsp(), CellRef) || VN_IS(nodep->lhsp(), CellArrayRef))) {
                m_ds.m_unlinkedScopep = nodep->lhsp();
            }
            ଯଦି (m_ds.m_dotPos == DP_FIRST) m_ds.m_dotPos = DP_SCOPE;
            ଯଦି (!m_ds.m_dotErr) {  // Once something wrong, give up
                // Top 'final' dot RHS is final RHS, else it's a
                // DOT(DOT(x,*here*),real-rhs) which we consider a RHS
                ଯଦି (start && m_ds.m_dotPos == DP_SCOPE) m_ds.m_dotPos = DP_FINAL;
                UINFO(8, indent() << "iter.rhs   " << m_ds.ascii() << " " << nodep);
                // m_ds.m_dotSymp points at lhsp()'s symbol table, so resolve RHS under that
                iterateAndNextNull(nodep->rhsp());
                UINFO(8, indent() << "iter.rdone " << m_ds.ascii() << " " << nodep);
                // UINFOTREE(9, nodep, "", "dot-rho");
            }
            ଯଦି (!m_ds.m_unresolvedClass) {
                ଯଦି (start) {
                    Ast_ଗଣ୍ଠି* newp;
                    ଯଦି (m_ds.m_dotErr) {
                        newp = ନୂତନ AstConst{nodep->fileline(), AstConst::BitFalse{}};
                    } ଅନ୍ୟଥା {
                        newp = nodep->rhsp()->unlinkFrBack();
                    }
                    UINFOTREE(9, newp, "", "dot-out");
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
                } ଅନ୍ୟଥା {  // Dot midpoint
                    Ast_ଗଣ୍ଠି* newp = nodep->rhsp()->unlinkFrBack();
                    ଯଦି (m_ds.m_unresolvedCell) {
                        AstCellRef* ଅଚଳ crp = ନୂତନ AstCellRef{
                            nodep->fileline(), nodep->name(), nodep->lhsp()->unlinkFrBack(), newp};
                        newp = crp;
                    }
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
                }
            }
        }
        ଯଦି (start) {
            m_ds = lastStates;
        } ଅନ୍ୟଥା {
            ଅଚଳ ତାର୍କିକ unresolvedClass = m_ds.m_unresolvedClass;
            m_ds.m_dotp = lastStates.m_dotp;
            m_ds.m_unresolvedClass |= unresolvedClass;
        }
        UINFO(8, indent() << "done " << m_ds.ascii() << " " << nodep);
    }
    ଶୂନ୍ୟ visit(AstDefaultClocking* nodep) override {
        ଯଦି (VSymEnt* ଅଚଳ foundp = m_curSymp->findIdFallback(nodep->name())) {
            ଯଦି (AstClocking* ଅଚଳ clockingp = VN_CAST(foundp->nodep(), Clocking)) {
                clockingp->makeDefault();
                VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ବୃକ୍ଷ(), nodep);
            } ଅନ୍ୟଥା {
                nodep->v3error(nodep->prettyNameQ() << " is not a clocking identifier");
            }
        } ଅନ୍ୟଥା {
            nodep->v3error("Can't find definition of clocking: " << nodep->prettyNameQ());
        }
    }
    ଶୂନ୍ୟ visit(AstSenItem* nodep) override {
        LINKDOT_VISIT_START();
        VL_RESTORER(m_inSens);
        m_inSens = ସତ୍ୟ;
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstPatMember* nodep) override {
        LINKDOT_VISIT_START();
        ଯଦି (nodep->varrefp()) ଫେରନ୍ତୁ;  // only do this mapping once
        // If we have a TEXT token as our key, lookup if it's a LPARAM
        ଯଦି (AstText* ଅଚଳ textp = VN_CAST(nodep->keyp(), Text)) {
            UINFO(9, indent() << "visit " << nodep);
            UINFO(9, indent() << "      " << textp);
            // Lookup
            ଯଦି (VSymEnt* ଅଚଳ foundp = m_curSymp->findIdFallback(textp->text())) {
                ଯଦି (AstVar* ଅଚଳ varp = VN_CAST(foundp->nodep(), Var)) {
                    ଯଦି (varp->isParam() || varp->isGenVar()) {
                        // Attach found Text reference to PatMember
                        nodep->varrefp(
                            ନୂତନ AstVarRef{nodep->fileline(),
                                          foundp->imported() ? foundp->classOrPackagep() : nullptr,
                                          varp, VAccess::READ});
                        UINFO(9, indent() << " new " << nodep->varrefp());
                    }
                }
                ଯଦି (AstEnumItem* ଅଚଳ itemp = VN_CAST(foundp->nodep(), EnumItem)) {
                    // Attach enum item value to PatMember
                    nodep->varrefp(
                        ନୂତନ AstEnumItemRef{nodep->fileline(), itemp, foundp->classOrPackagep()});
                    UINFO(9, indent() << " new " << itemp);
                }
            }
        }
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstParseRef* nodep) override {
        ଯଦି (nodep->user3SetOnce()) ଫେରନ୍ତୁ;
        LINKDOT_VISIT_START();
        UINFO(9, indent() << "visit " << nodep);
        UINFO(9, indent() << m_ds.ascii());
        ଯଦି (m_ds.m_unresolvedClass) ଫେରନ୍ତୁ;
        // m_curSymp is symbol table of outer expression
        // m_ds.m_dotSymp is symbol table relative to "."'s above now
        UASSERT_OBJ(m_ds.m_dotSymp, nodep, "nullptr lookup symbol table");
        // Generally resolved during Primary, but might be at param time under AstUnlinkedRef
        UASSERT_OBJ(!m_statep->forScopeCreation(), nodep, "ParseRefs should no longer exist");
        ଅଚଳ DotStates lastStates = m_ds;
        ଅଚଳ ତାର୍କିକ start = (m_ds.m_dotPos == DP_NONE);  // Save, as m_dotp will be changed
        ତାର୍କିକ first = start || m_ds.m_dotPos == DP_FIRST;

        ଯଦି (start) {
            m_ds.init(m_curSymp);
            // Note m_ds.m_dotp remains nullptr; this is a reference not under a dot
        }
        ଯଦି (nodep->name() == "super") {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: super");
            m_ds.m_dotErr = ସତ୍ୟ;
        }
        ଯଦି (nodep->name() == "this") {
            iterateChildren(nodep);
            ଯଦି (m_statep->forPrimary()) ଫେରନ୍ତୁ;  // The class might be parameterized somewhere
            ଅଚଳ VSymEnt* classSymp = getThisClassSymp();
            ଯଦି (!classSymp) {
                nodep->v3error("'this' used outside class (IEEE 1800-2023 8.11)");
                ଫେରନ୍ତୁ;
            }
            AstClass* ଅଚଳ classp = VN_AS(classSymp->nodep(), Class);
            AstClassRefDType* ଅଚଳ dtypep
                = ନୂତନ AstClassRefDType{nodep->fileline(), classp, nullptr};
            AstThisRef* ଅଚଳ newp = ନୂତନ AstThisRef{nodep->fileline(), VFlagChildDType{}, dtypep};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
            ଫେରନ୍ତୁ;
        }
        ଯଦି (m_ds.m_dotPos == DP_MEMBER && VN_IS(m_ds.m_dotp->lhsp(), LambdaArgRef)
            && nodep->name() == "index") {
            // 'with' statement's 'item.index'
            // m_ds.dotp->lhsp() was checked to know if `index` is directly after lambda arg ref.
            // If not, treat it as normal member select
            iterateChildren(nodep);
            AstLambdaArgRef* ଅଚଳ newp = ନୂତନ AstLambdaArgRef{
                nodep->fileline(), m_ds.m_dotp->lhsp()->name() + "__DOT__index", ସତ୍ୟ};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
            ଫେରନ୍ତୁ;
        } ଅନ୍ୟଥା ଯଦି (m_ds.m_dotPos == DP_MEMBER) {
            // Found a Var, everything following is membership.  {scope}.{var}.HERE {member}
            ଯଦି (m_ds.m_dotErr) {
                nodep->unlinkFrBack();  // Avoid circular node loop on errors
            } ଅନ୍ୟଥା {
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ varEtcp = VN_AS(m_ds.m_dotp->lhsp()->unlinkFrBack(), ଗଣ୍ଠି_ପଦସମଷ୍ଟି);
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ newp = ନୂତନ AstMemberSel{nodep->fileline(), varEtcp,  //
                                                           VFlagChildDType{}, nodep->name()};
                nodep->replaceWith(newp);
            }
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
        } ଅନ୍ୟଥା {
            //
            string expectWhat;
            ତାର୍କିକ allowScope = ମିଥ୍ୟା;
            ତାର୍କିକ allowVar = ମିଥ୍ୟା;
            ତାର୍କିକ allowFTask = ମିଥ୍ୟା;
            ତାର୍କିକ staticAccess = ମିଥ୍ୟା;
            ଯଦି (m_ds.m_disablep) {
                allowScope = ସତ୍ୟ;
                allowFTask = ସତ୍ୟ;
                expectWhat = "block/task";
            } ଅନ୍ୟଥା ଯଦି (m_ds.m_dotPos == DP_PACKAGE) {
                // {package-or-class}::{a}
                Ast_ଗଣ୍ଠି_Module* classOrPackagep = nullptr;
                expectWhat = "scope/variable/func";
                allowScope = ସତ୍ୟ;
                allowVar = ସତ୍ୟ;
                allowFTask = ସତ୍ୟ;
                staticAccess = ସତ୍ୟ;
                UINFO(9, "chk pkg " << m_ds.ascii() << " lhsp=" << m_ds.m_dotp->lhsp());
                UASSERT_OBJ(VN_IS(m_ds.m_dotp->lhsp(), ClassOrPackageRef), m_ds.m_dotp->lhsp(),
                            "Bad package link");
                AstClassOrPackageRef* ଅଚଳ cpackagerefp
                    = VN_AS(m_ds.m_dotp->lhsp(), ClassOrPackageRef);
                ଯଦି (cpackagerefp->name() == "local::") {
                    m_randSymp = nullptr;
                    first = ସତ୍ୟ;
                } ଅନ୍ୟଥା ଯଦି (!cpackagerefp->classOrPackageSkipp()) {
                    VSymEnt* ଅଚଳ foundp = m_statep->resolveClassOrPackage(
                        m_ds.m_dotSymp, cpackagerefp, ସତ୍ୟ, ମିଥ୍ୟା, ":: reference");
                    ଯଦି (!foundp) ଫେରନ୍ତୁ;
                    classOrPackagep = cpackagerefp->classOrPackageSkipp();
                    ଯଦି (classOrPackagep) {
                        m_ds.m_dotSymp = m_statep->get_ଗଣ୍ଠି_Sym(classOrPackagep);
                    } ଅନ୍ୟଥା {
                        m_ds = lastStates;
                        ଫେରନ୍ତୁ;
                    }
                } ଅନ୍ୟଥା {
                    classOrPackagep = cpackagerefp->classOrPackageSkipp();
                    UASSERT_OBJ(classOrPackagep, m_ds.m_dotp->lhsp(), "Bad package link");
                    m_ds.m_dotSymp = m_statep->get_ଗଣ୍ଠି_Sym(classOrPackagep);
                }
                m_ds.m_dotPos = DP_SCOPE;
            } ଅନ୍ୟଥା ଯଦି (m_ds.m_dotPos == DP_SCOPE || m_ds.m_dotPos == DP_FIRST) {
                // {a}.{b}, where {a} maybe a module name
                // or variable, where dotting into structure member
                expectWhat = "scope/variable";
                allowScope = ସତ୍ୟ;
                allowVar = ସତ୍ୟ;
            } ଅନ୍ୟଥା ଯଦି (m_ds.m_dotPos == DP_NONE) {
                expectWhat = "variable";
                allowVar = ସତ୍ୟ;
            } ଅନ୍ୟଥା ଯଦି (m_ds.m_dotPos == DP_FINAL) {
                expectWhat = "variable/method";
                allowFTask = ସତ୍ୟ;
                allowVar = ସତ୍ୟ;
            } ଅନ୍ୟଥା {
                UINFO(1, "ds=" << m_ds.ascii());
                nodep->v3fatalSrc("Unhandled DotPosition");
            }
            // Lookup
            VSymEnt* foundp;
            string baddot;
            VSymEnt* okSymp = nullptr;
            // Restricted 'with (id_list)': only id_list names bind into target class.
            ଯଦି (m_randSymp
                && (!m_currentWithp || m_currentWithp->nameResolvesToTarget(nodep->name()))) {
                foundp = m_randSymp->findIdFlat(nodep->name());
                ଯଦି (foundp) {
                    ଯଦି (m_currentWithp && m_currentWithp->restricted()) {
                        m_restrictedNamesUsed.ଭର୍ତ୍ତି(nodep->name());
                    }
                    ଯଦି (!start) m_ds.m_dotPos = DP_MEMBER;
                    ଯଦି (!m_currentWithp) {
                        UASSERT_OBJ(m_randMethodCallp, nodep, "Expected to be under randomize()");
                        // This will start failing once complex expressions are allowed on the LHS
                        // of randomize() with args
                        UASSERT_OBJ(VN_IS(m_randMethodCallp->fromp(), VarRef), m_randMethodCallp,
                                    "Expected simple randomize target");
                        // A ParseRef is used here so that the dot RHS gets resolved
                        AstMemberSel* ଅଚଳ newp = ନୂତନ AstMemberSel{
                            nodep->fileline(), m_randMethodCallp->fromp()->clone_ବୃକ୍ଷ(ମିଥ୍ୟା),
                            VFlagChildDType{}, nodep->name()};
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
                        ଫେରନ୍ତୁ;
                    }
                    UINFO(9, indent() << "randomize-with fromSym " << foundp->nodep());
                    AstLambdaArgRef* ଅଚଳ lambdaRefp
                        = ନୂତନ AstLambdaArgRef{nodep->fileline(), "__Vrandwith_obj", ମିଥ୍ୟା};
                    AstMemberSel* newp = ନୂତନ AstMemberSel{nodep->fileline(), lambdaRefp,
                                                          VFlagChildDType{}, nodep->name()};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
                    ଫେରନ୍ତୁ;
                }
            }
            ଯଦି (allowScope) {
                foundp = m_statep->findDotted(nodep->fileline(), m_ds.m_dotSymp, nodep->name(),
                                              baddot, okSymp, first);  // Maybe nullptr
            } ଅନ୍ୟଥା ଯଦି (first) {
                foundp = m_ds.m_dotSymp->findIdFallback(nodep->name());
            } ଅନ୍ୟଥା {
                foundp = m_ds.m_dotSymp->findIdFlat(nodep->name());
            }
            ଯଦି (!foundp && m_ds.m_dotp && isDotThisReference(m_ds.m_dotp)) {
                ଅଚଳ AstClass* ଅଚଳ classp = VN_CAST(m_ds.m_dotSymp->nodep(), Class);
                ଯଦି (classp && classp->isCovergroup() && classp->covergroupEnclosingClassp()) {
                    VSymEnt* ଅଚଳ enclosingClassSymp
                        = m_statep->get_ଗଣ୍ଠି_Sym(classp->covergroupEnclosingClassp());
                    foundp = enclosingClassSymp->findIdFallback(nodep->name());
                    ଯଦି (foundp) m_ds.m_dotSymp = enclosingClassSymp;
                }
            }
            // If not found in modport, check interface fallback for parameters and typedefs.
            // Parameters and typedefs are always visible through a modport (IEEE 1800-2023 25.5).
            // This mirrors the VarXRef modport parameter fallback in visit(AstVarXRef).
            ଯଦି (!foundp && VN_IS(m_ds.m_dotSymp->nodep(), Modport)
                && m_ds.m_dotSymp->fallbackp()) {
                VSymEnt* ଅଚଳ ifaceFallbackp = m_ds.m_dotSymp->fallbackp();
                VSymEnt* ଅଚଳ ifaceFoundp = ifaceFallbackp->findIdFlat(nodep->name());
                ଯଦି (ifaceFoundp) {
                    ଯଦି (ଅଚଳ AstVar* ଅଚଳ varp = VN_CAST(ifaceFoundp->nodep(), Var)) {
                        ଯଦି (varp->isParam()) foundp = ifaceFoundp;
                    } ଅନ୍ୟଥା ଯଦି (VN_IS(ifaceFoundp->nodep(), Typedef)
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
            ଯଦି (!foundp && !okSymp && VN_IS(m_ds.m_dotSymp->nodep(), Modport)) {
                okSymp = m_ds.m_dotSymp;
            }
            ଯଦି (foundp) {
                UINFO(9, indent() << "found=se" << cvtToHex(foundp) << "  exp=" << expectWhat
                                  << "  n=" << foundp->nodep());
            }
            // What fell out?
            ତାର୍କିକ ok = ମିଥ୍ୟା;
            // Special case: waiting on clocking event
            ଯଦି (m_inSens && foundp && m_ds.m_dotPos != DP_SCOPE && m_ds.m_dotPos != DP_FIRST) {
                ଯଦି (AstClocking* ଅଚଳ clockingp = sensClockingp(foundp->nodep())) {
                    foundp = getCreateClockingEventSymEnt(clockingp);
                }
            }
            ଯଦି (!foundp) {
            } ଅନ୍ୟଥା ଯଦି (VN_IS(foundp->nodep(), Cell) || VN_IS(foundp->nodep(), ଗଣ୍ଠି_Block)
                       || VN_IS(foundp->nodep(), GenBlock)
                       || VN_IS(foundp->nodep(), Netlist)  // for $root
                       || VN_IS(foundp->nodep(), Module)) {  // if top
                ଯଦି (allowScope) {
                    ok = ସତ୍ୟ;
                    m_ds.m_dotText = VString::dot(m_ds.m_dotText, ".", nodep->name());
                    m_ds.m_dotSymp = foundp;
                    m_ds.m_dotPos = DP_SCOPE;
                    ଯଦି (m_ds.m_disablep && VN_IS(foundp->nodep(), ଗଣ୍ଠି_Block)) {
                        // Possibly it is not the final link. If we are under dot and not in its
                        // last component, `targetp()` field will be overwritten by next components
                        m_ds.m_disablep->targetp(foundp->nodep());
                    }
                    ଯଦି (VN_IS(foundp->nodep(), GenBlock)) {
                        m_ds.m_genBlk = ସତ୍ୟ;
                        ଯଦି (m_ds.m_disablep) {
                            m_ds.m_disablep->v3warn(
                                E_UNSUPPORTED,
                                "Unsupported: Generate block referenced by disable");
                        }
                    }
                    // Upper AstDot visitor will handle it from here
                } ଅନ୍ୟଥା ଯଦି (VN_IS(foundp->nodep(), Cell) && allowVar) {
                    AstCell* ଅଚଳ cellp = VN_AS(foundp->nodep(), Cell);
                    ଯଦି (VN_IS(cellp->modp(), Iface)) {
                        // Interfaces can be referenced like a variable for interconnect
                        VSymEnt* ଅଚଳ cellEntp = m_statep->get_ଗଣ୍ଠି_Sym(cellp);
                        UASSERT_OBJ(cellEntp, nodep, "No interface sym entry");
                        VSymEnt* ଅଚଳ parentEntp
                            = cellEntp->parentp();  // Container of the var; probably a module or
                                                    // generate begin
                        AstVar* ଅଚଳ ifaceRefVarp
                            = find_Iface_ଶୀର୍ଷ_Varp(nodep, parentEntp, nodep->name());
                        //
                        ok = ସତ୍ୟ;
                        m_ds.m_dotSymp = foundp;
                        m_ds.m_dotPos = DP_SCOPE;
                        UINFO(9, indent() << " cell -> iface varref " << foundp->nodep());
                        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* newp;
                        ଯଦି (m_ds.m_dotText != "") {
                            newp = ନୂତନ AstVarXRef{nodep->fileline(), ifaceRefVarp, m_ds.m_dotText,
                                                  VAccess::READ};
                            ଯଦି (m_ds.m_unresolvedCell && m_ds.m_unlinkedScopep) {
                                newp = ନୂତନ AstUnlinkedRef{nodep->fileline(), newp, newp->name(),
                                                          m_ds.m_unlinkedScopep->unlinkFrBack()};
                                m_ds.m_unlinkedScopep = nullptr;
                                m_ds.m_unresolvedCell = ମିଥ୍ୟା;
                                m_ds.m_dotText = "";
                            }
                        } ଅନ୍ୟଥା {
                            AstVarRef* ଅଚଳ refp
                                = ନୂତନ AstVarRef{nodep->fileline(), ifaceRefVarp, VAccess::READ};
                            newp = refp;
                        }
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
                    } ଅନ୍ୟଥା ଯଦି (VN_IS(cellp->modp(), NotFoundModule)) {
                        cellp->modNameFileline()->v3error("Cannot find file containing interface: "
                                                          << cellp->modp()->prettyNameQ());
                    }
                }
            } ଅନ୍ୟଥା ଯଦି (allowScope && !allowFTask && VN_IS(foundp->nodep(), ଗଣ୍ଠି_FTask)) {
                // Function/task used as scope for static variable access (IEEE std 1800-2017 6.21)
                // Don't set m_dotText so the static variable resolves as a VarRef rather than
                // VarXRef. This is so that V3Begin won't move it out of the function/task's scope.
                ok = ସତ୍ୟ;
                m_ds.m_dotSymp = foundp;
                m_ds.m_dotPos = DP_SCOPE;
            } ଅନ୍ୟଥା ଯଦି (allowFTask && VN_IS(foundp->nodep(), ଗଣ୍ଠି_FTask)) {
                Ast_ଗଣ୍ଠି_FTask_Ref* taskrefp;
                ଯଦି (VN_IS(foundp->nodep(), Task)) {
                    taskrefp = ନୂତନ AstTaskRef{nodep->fileline(), nodep->name()};
                } ଅନ୍ୟଥା {
                    taskrefp = ନୂତନ AstFuncRef{nodep->fileline(), nodep->name()};
                }
                nodep->replaceWith(taskrefp);
                VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
                m_ds = lastStates;
                ଫେରନ୍ତୁ;
            } ଅନ୍ୟଥା ଯଦି (AstVar* ଅଚଳ varp = foundToVarp(foundp, nodep, VAccess::READ)) {
                // Preserve explicit 'this' references to members inherited from
                // generic classes until after specialization.
                ଯଦି (m_statep->forPrimary() && m_ds.m_dotp && isDotThisReference(m_ds.m_dotp)
                    && foundp->classOrPackagep() != m_modp
                    && VN_IS(foundp->classOrPackagep(), Class)
                    && foundp->classOrPackagep()->hasGParam()) {
                    m_ds.m_unresolvedClass = ସତ୍ୟ;
                    ଫେରନ୍ତୁ;
                }
                AstIfaceRefDType* ଅଚଳ ifacerefp
                    = LinkDotState::ifaceRefFromArray(varp->subDTypep());
                ଯଦି (varp->isIfaceRef() && m_genericIfaceModule
                    && VN_IS(varp->childDTypep(), IfaceGenericDType) && !start) {
                    // Defer only dotted member access ('d.PARAM'), as V3Param must specialize
                    // first; a standalone ref ('.x(d)') resolves via allowVar below now
                    ok = ସତ୍ୟ;
                    m_ds.m_unresolvedGenericIface = ସତ୍ୟ;
                } ଅନ୍ୟଥା ଯଦି (ifacerefp && varp->isIfaceRef()) {
                    UASSERT_OBJ(ifacerefp->ifaceViaCellp(), ifacerefp, "Unlinked interface");
                    // Really this is a scope reference into an interface
                    UINFO(9, indent() << "varref-ifaceref " << m_ds.m_dotText << "  " << nodep);
                    m_ds.m_dotText = VString::dot(m_ds.m_dotText, ".", nodep->name());
                    // If modport specified, use modport symbol table for lookup of virtual ports
                    ଯଦି (ifacerefp->modportp() && m_statep->exists_ଗଣ୍ଠି_Sym(ifacerefp->modportp())) {
                        m_ds.m_dotSymp = m_statep->get_ଗଣ୍ଠି_Sym(ifacerefp->modportp());
                    } ଅନ୍ୟଥା {
                        m_ds.m_dotSymp = m_statep->get_ଗଣ୍ଠି_Sym(ifacerefp->ifaceViaCellp());
                    }
                    m_ds.m_dotPos = DP_SCOPE;
                    ok = ସତ୍ୟ;

                    AstVarRef* ଅଚଳ refp = ନୂତନ AstVarRef{nodep->fileline(), varp, VAccess::READ};
                    nodep->replaceWith(refp);

                    VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
                } ଅନ୍ୟଥା ଯଦି (allowVar) {
                    Ast_ଗଣ୍ଠି* newp;
                    ଯଦି (m_ds.m_dotText != "") {
                        AstVarXRef* ଅଚଳ refp
                            = ନୂତନ AstVarXRef{nodep->fileline(), nodep->name(), m_ds.m_dotText,
                                             VAccess::READ};  // lvalue'ness computed later
                        refp->varp(varp);
                        // Not linked again after V3LinkLValue, so that reports writing it
                        ଯଦି (ଅଚଳ AstModportVarRef* ଅଚଳ mvarp
                            = VN_CAST(foundp->nodep(), ModportVarRef)) {
                            ଯଦି (m_statep->forParamed() && mvarp->direction().isReadOnly()) {
                                refp->readOnlyModport(ସତ୍ୟ);
                            }
                        }
                        refp->containsGenBlock(m_ds.m_genBlk);
                        m_ds.m_dotText = "";
                        ଯଦି (m_ds.m_unresolvedCell && m_ds.m_unlinkedScopep) {
                            ଅଚଳ string dotted = refp->dotted();
                            ଅଚଳ size_t pos = dotted.find("__BRA__??__KET__");
                            // Arrays of interfaces all have the same parameters
                            ଯଦି (pos != string::npos && varp->isParam()
                                && VN_IS(m_ds.m_unlinkedScopep, CellArrayRef)) {
                                refp->dotted(dotted.substr(0, pos));
                                newp = refp;
                            } ଅନ୍ୟଥା {
                                UINFO(9, indent() << "deferring until post-V3Param: " << refp);
                                newp = ନୂତନ AstUnlinkedRef{nodep->fileline(), refp, refp->name(),
                                                          m_ds.m_unlinkedScopep->unlinkFrBack()};
                                m_ds.m_unlinkedScopep = nullptr;
                                m_ds.m_unresolvedCell = ମିଥ୍ୟା;
                            }
                        } ଅନ୍ୟଥା {
                            newp = refp;
                        }
                    } ଅନ୍ୟଥା {
                        ଯଦି (staticAccess && !varp->lifetime().isStatic() && !varp->isParam()) {
                            // TODO bug4077
                            // nodep->v3error("Static access to non-static member variable "
                            //                << varp->prettyNameQ() << endl);
                        }
                        AstVarRef* ଅଚଳ refp = ନୂତନ AstVarRef{
                            nodep->fileline(), varp, VAccess::READ};  // lvalue'ness computed later
                        refp->classOrPackagep(foundp->classOrPackagep());
                        newp = refp;
                    }
                    UINFO(9, indent() << "new " << newp);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
                    m_ds.m_dotPos = DP_MEMBER;
                    ok = ସତ୍ୟ;
                }
            } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstModport* ଅଚଳ modportp = VN_CAST(foundp->nodep(), Modport)) {
                // A scope reference into an interface's modport (not
                // necessarily at a pin connection)
                UINFO(9, indent() << "cell-ref-to-modport " << m_ds.m_dotText << "  " << nodep);
                UINFO(9, indent() << "unlinked " << m_ds.m_unlinkedScopep);
                UINFO(9,
                      indent() << "dotSymp " << m_ds.m_dotSymp << " " << m_ds.m_dotSymp->nodep());
                // Iface was the previously dotted component
                ଯଦି (!m_ds.m_dotSymp || !VN_IS(m_ds.m_dotSymp->nodep(), Cell)
                    || !VN_AS(m_ds.m_dotSymp->nodep(), Cell)->modp()
                    || !VN_IS(VN_AS(m_ds.m_dotSymp->nodep(), Cell)->modp(), Iface)) {
                    nodep->v3error("Modport not referenced as <interface>."
                                   << modportp->prettyNameQ());
                } ଅନ୍ୟଥା ଯଦି (!VN_AS(m_ds.m_dotSymp->nodep(), Cell)->modp()
                           || !VN_IS(VN_AS(m_ds.m_dotSymp->nodep(), Cell)->modp(), Iface)) {
                    nodep->v3error("Modport not referenced from underneath an interface: "
                                   << modportp->prettyNameQ());
                } ଅନ୍ୟଥା {
                    AstCell* ଅଚଳ cellp = VN_AS(m_ds.m_dotSymp->nodep(), Cell);
                    UASSERT_OBJ(cellp, nodep, "Modport not referenced from an instance");
                    VSymEnt* ଅଚଳ cellEntp = m_statep->get_ଗଣ୍ଠି_Sym(cellp);
                    UASSERT_OBJ(cellEntp, nodep, "No interface sym entry");
                    VSymEnt* ଅଚଳ parentEntp
                        = cellEntp->parentp();  // Container of the var; probably a
                                                // module or generate begin
                    // We drop __BRA__??__KET__ as cells don't have that naming yet
                    AstVar* ଅଚଳ ifaceRefVarp
                        = find_Iface_ଶୀର୍ଷ_Varp(nodep, parentEntp, cellp->name());
                    //
                    ok = ସତ୍ୟ;
                    m_ds.m_dotText = VString::dot(m_ds.m_dotText, ".", nodep->name());
                    m_ds.m_dotSymp = foundp;
                    m_ds.m_dotPos = DP_SCOPE;
                    UINFO(9, indent() << "modport -> iface varref " << foundp->nodep());
                    // We lose the modport name here, so we cannot detect mismatched modports.
                    AstVarRef* ଅଚଳ refp
                        = ନୂତନ AstVarRef{nodep->fileline(), ifaceRefVarp, VAccess::READ};
                    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* newp = refp;

                    auto* ଅଚଳ cellarrayrefp = VN_CAST(m_ds.m_unlinkedScopep, CellArrayRef);
                    ଯଦି (cellarrayrefp) {
                        // iface[vec].modport became CellArrayRef(iface, lsb)
                        // Convert back to SelBit(iface, lsb)
                        UINFO(9, indent() << "Array modport to SelBit " << cellarrayrefp);
                        newp = ନୂତନ AstSelBit{cellarrayrefp->fileline(), newp,
                                             cellarrayrefp->selp()->unlinkFrBack()};
                        newp->user3(ସତ୍ୟ);  // Don't process again
                        VL_DO_DANGLING(ଠେଲ_Deletep(cellarrayrefp->unlinkFrBack()), cellarrayrefp);
                        m_ds.m_unlinkedScopep = nullptr;
                    }
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
                }
            } ଅନ୍ୟଥା ଯଦି (AstConstraint* ଅଚଳ defp = VN_CAST(foundp->nodep(), Constraint)) {
                Ast_ଗଣ୍ଠି* ଅଚଳ newp = ନୂତନ AstConstraintRef{nodep->fileline(), nullptr, defp};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
                ok = ସତ୍ୟ;
                m_ds.m_dotPos = DP_MEMBER;
                m_ds.m_dotText = "";
            } ଅନ୍ୟଥା ଯଦି (AstClass* ଅଚଳ defp = VN_CAST(foundp->nodep(), Class)) {
                ଯଦି (allowVar) {
                    ଯଦି (m_ds.m_dotPos == DP_SCOPE && !staticAccess) {
                        // Continue hierarchical lookup in the class scope.
                        m_ds.m_dotSymp = foundp;
                    } ଅନ୍ୟଥା {
                        AstRefDType* ଅଚଳ newp
                            = ନୂତନ AstRefDType{nodep->fileline(), nodep->name()};
                        replaceWithCheckBreak(nodep, newp);
                        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
                    }
                    ok = ସତ୍ୟ;
                    m_ds.m_dotText = "";
                } ଅନ୍ୟଥା {
                    (ଶୂନ୍ୟ)defp;  // Prevent unused variable warning
                }
            } ଅନ୍ୟଥା ଯଦି (AstMemberDType* ଅଚଳ defp = VN_CAST(foundp->nodep(), MemberDType)) {
                ଯଦି (allowVar) {
                    checkMemberDeclOrder(nodep, defp);
                    AstRefDType* ଅଚଳ refp = ନୂତନ AstRefDType{nodep->fileline(), nodep->name()};
                    refp->refDTypep(defp);
                    replaceWithCheckBreak(nodep, refp);
                    VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
                    ok = ସତ୍ୟ;
                    m_ds.m_dotText = "";
                }
            } ଅନ୍ୟଥା ଯଦି (AstEnumItem* ଅଚଳ valuep = VN_CAST(foundp->nodep(), EnumItem)) {
                ଯଦି (allowVar) {
                    AstEnumItemRef* ଅଚଳ refp
                        = ନୂତନ AstEnumItemRef{nodep->fileline(), valuep, foundp->classOrPackagep()};
                    Ast_ଗଣ୍ଠି* newp = refp;
                    // Hierarchical reference, relinked after V3Param as the referenced
                    // module may be specialized, similar to AstVarXRef
                    refp->dotted(m_ds.m_dotText);
                    refp->containsGenBlock(m_ds.m_genBlk);
                    ଯଦି (m_ds.m_unresolvedCell && m_ds.m_unlinkedScopep) {
                        UINFO(9, indent() << "deferring until post-V3Param: " << refp);
                        newp = ନୂତନ AstUnlinkedRef{nodep->fileline(), refp, refp->name(),
                                                  m_ds.m_unlinkedScopep->unlinkFrBack()};
                        m_ds.m_unlinkedScopep = nullptr;
                        m_ds.m_unresolvedCell = ମିଥ୍ୟା;
                    }
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
                    ok = ସତ୍ୟ;
                    m_ds.m_dotText = "";
                }
            } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstLambdaArgRef* ଅଚଳ argrefp
                       = VN_CAST(foundp->nodep(), LambdaArgRef)) {
                ଯଦି (allowVar) {
                    Ast_ଗଣ୍ଠି* ଅଚଳ newp
                        = ନୂତନ AstLambdaArgRef{nodep->fileline(), argrefp->name(), ମିଥ୍ୟା};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
                    ok = ସତ୍ୟ;
                    m_ds.m_dotPos = DP_MEMBER;
                    m_ds.m_dotText = "";
                }
            } ଅନ୍ୟଥା ଯଦି (VN_IS(foundp->nodep(), Clocking)) {
                m_ds.m_dotSymp = foundp;
                ଯଦି (m_ds.m_dotText != "") m_ds.m_dotText += "." + nodep->name();
                ok = m_ds.m_dotPos == DP_SCOPE || m_ds.m_dotPos == DP_FIRST;
            } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstModportClockingRef* ଅଚଳ clockingRefp
                       = VN_CAST(foundp->nodep(), ModportClockingRef)) {
                // Clocking block accessed through a modport - redirect to actual clocking
                m_ds.m_dotSymp = m_statep->get_ଗଣ୍ଠି_Sym(clockingRefp->clockingp());
                ଯଦି (m_ds.m_dotText != "") m_ds.m_dotText += "." + nodep->name();
                ok = m_ds.m_dotPos == DP_SCOPE || m_ds.m_dotPos == DP_FIRST;
            } ଅନ୍ୟଥା ଯଦି (ଅଚଳ Ast_ଗଣ୍ଠି_FTask* ଅଚଳ ftaskp = VN_CAST(foundp->nodep(), ଗଣ୍ଠି_FTask)) {
                ଯଦି (!ftaskp->isFunction() || ftaskp->classMethod()) {
                    ok = m_ds.m_dotPos == DP_NONE;
                    ଯଦି (ok) {
                        // The condition is true for tasks,
                        // properties and void functions.
                        // In these cases, the parentheses may be skipped.
                        // Also SV class methods can be called without parens
                        AstFuncRef* ଅଚଳ funcRefp
                            = ନୂତନ AstFuncRef{nodep->fileline(), nodep->name()};
                        nodep->replaceWith(funcRefp);
                        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
                    }
                }
            } ଅନ୍ୟଥା ଯଦି (AstTypedef* ଅଚଳ defp = VN_CAST(foundp->nodep(), Typedef)) {
                ଅଚଳ ତାର୍କିକ ifaceFinalSegmentAllowed
                    = (m_ds.m_dotPos == DP_FINAL) && m_ds.m_dotSymp
                      && (VN_IS(m_ds.m_dotSymp->nodep(), Modport)
                          || (VN_IS(m_ds.m_dotSymp->nodep(), Cell)
                              && VN_CAST(m_ds.m_dotSymp->nodep(), Cell)->modp()
                              && VN_IS(VN_CAST(m_ds.m_dotSymp->nodep(), Cell)->modp(), Iface)));
                // Allow interface typedef references even without IfaceCapture
                // The dependency graph handles resolution when IfaceCapture is disabled
                ok = (m_ds.m_dotPos == DP_NONE || m_ds.m_dotPos == DP_SCOPE
                      || ifaceFinalSegmentAllowed);
                ଯଦି (ifaceFinalSegmentAllowed) {
                    UINFO(9, indent() << "allow final-segment typedef name="
                                      << nodep->prettyNameQ() << " dotText='" << m_ds.m_dotText
                                      << "' dotSym=" << m_ds.m_dotSymp);
                }
                ଯଦି (ok) {
                    AstRefDType* ଅଚଳ refp = ନୂତନ AstRefDType{nodep->fileline(), nodep->name()};
                    // Don't check if typedef is to a <type T>::<reference> as might not be
                    // resolved yet
                    ଯଦି (m_ds.m_dotPos == DP_NONE) checkDeclOrder(nodep, defp);
                    refp->typedefp(defp);

                    V3LinkDotIfaceCapture::captureTypedefContext(
                        refp, "typedef", static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(m_ds.m_dotPos), m_ds.m_dotText,
                        m_ds.m_dotSymp, m_modp, [ଏହି]() { ଫେରନ୍ତୁ indent(); });

                    ଯଦି (VN_IS(nodep->backp(), SelExtract)) {
                        m_packedArrayDtp = refp;
                    } ଅନ୍ୟଥା {
                        replaceWithCheckBreak(nodep, refp);
                        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
                    }
                }
            } ଅନ୍ୟଥା ଯଦି (AstParamTypeDType* ଅଚଳ defp = VN_CAST(foundp->nodep(), ParamTypeDType)) {
                ok = (m_ds.m_dotPos == DP_NONE || m_ds.m_dotPos == DP_SCOPE
                      || m_ds.m_dotPos == DP_FINAL);
                ଯଦି (ok) {
                    AstRefDType* ଅଚଳ refp = ନୂତନ AstRefDType{nodep->fileline(), nodep->name()};
                    refp->refDTypep(defp);

                    V3LinkDotIfaceCapture::captureTypedefContext(
                        refp, "paramtype", static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(m_ds.m_dotPos), m_ds.m_dotText,
                        m_ds.m_dotSymp, m_modp, [ଏହି]() { ଫେରନ୍ତୁ indent(); });

                    ଯଦି (VN_IS(nodep->backp(), SelExtract)) {
                        m_packedArrayDtp = refp;
                    } ଅନ୍ୟଥା {
                        replaceWithCheckBreak(nodep, refp);
                        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
                    }
                }
            }
            ଯଦି (!ok) {
                ଯଦି (m_insideClassExtParam) {
                    // Don't throw error if the reference is inside a class that extends a param,
                    // because some members can't be linked in such a case. m_insideClassExtParam
                    // may be true only in the first stage of linking.
                    // Mark that the Dot statement can't be resolved.
                    m_ds.m_unresolvedClass = ସତ୍ୟ;
                    // If the symbol was a scope name, it would be resolved.
                    m_ds.m_dotPos = DP_MEMBER;
                } ଅନ୍ୟଥା {
                    // Cells/interfaces can't be implicit
                    ଅଚଳ ତାର୍କିକ checkImplicit
                        = (!m_ds.m_dotp && m_ds.m_dotText == "" && !m_ds.m_disablep && !foundp);
                    ଅଚଳ ତାର୍କିକ err
                        = !(checkImplicit && m_statep->implicitOk(m_modp, nodep->name()));
                    ଯଦି (err) {
                        ଯଦି (foundp) {
                            nodep->v3error("Found definition of '"
                                           << m_ds.m_dotText << (m_ds.m_dotText == "" ? "" : ".")
                                           << nodep->prettyName() << "'" << " as a "
                                           << foundp->nodep()->typeName() << " but expected a "
                                           << expectWhat);
                        } ଅନ୍ୟଥା ଯଦି (m_ds.m_dotText == "") {
                            UINFO(1, "   ErrParseRef curSymp=se" << cvtToHex(m_curSymp)
                                                                 << " ds=" << m_ds.ascii());
                            ଅଚଳ string suggest = m_statep->suggestSymFallback(
                                m_ds.m_dotSymp, nodep->name(), VNodeMatcher{});
                            nodep->v3error(
                                "Can't find definition of "
                                << expectWhat << ": " << nodep->prettyNameQ() << '\n'
                                << (suggest.empty() ? "" : nodep->warnMore() + suggest));
                        } ଅନ୍ୟଥା {
                            nodep->v3error(
                                "Can't find definition of "
                                << (!baddot.empty() ? Ast_ଗଣ୍ଠି::prettyNameQ(baddot)
                                                    : nodep->prettyNameQ())
                                << " in dotted " << expectWhat << ": '"
                                << m_ds.m_dotText + "." + nodep->prettyName() << "'\n"
                                << nodep->warnContextPrimary()
                                << (okSymp ? okSymp->cellErrorScopes(
                                                 nodep, Ast_ଗଣ୍ଠି::prettyName(m_ds.m_dotText))
                                           : ""));
                        }
                        m_ds.m_dotErr = ସତ୍ୟ;
                    }
                    ଯଦି (checkImplicit) {
                        // Create if implicit, and also if error (so only complain once)
                        // Else if a scope is allowed, making a signal won't help error cascade
                        AstVar* ଅଚଳ varp
                            = createImplicitVar(m_curSymp, nodep, m_modp, m_modSymp, err);
                        AstVarRef* ଅଚଳ newp
                            = ନୂତନ AstVarRef{nodep->fileline(), varp, VAccess::READ};
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
                    }
                }
            }
        }
        ଯଦି (start) m_ds = lastStates;
    }
    ଶୂନ୍ୟ visit(AstClassOrPackageRef* nodep) override {
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

            ଯଦି (!nodep->classOrPackageSkipp() && !nodep->classOrPackageNodep()
                && nodep->name() != "local::") {
                ଅଚଳ ତାର୍କିକ deferIfUnresolved = m_statep->forPrimary() && m_insideClassExtParam;
                m_statep->resolveClassOrPackage(m_ds.m_dotSymp, nodep, m_ds.m_dotPos != DP_PACKAGE,
                                                ମିଥ୍ୟା, ":: reference", deferIfUnresolved);
            }

            // ClassRef's have pins, so track
            ଯଦି (nodep->classOrPackageSkipp()) {
                m_pinSymp = m_statep->get_ଗଣ୍ଠି_Sym(nodep->classOrPackageSkipp());
            } ଅନ୍ୟଥା ଯଦି (nodep->name() != "local::") {
                ଫେରନ୍ତୁ;
            }

            AstClass* ଅଚଳ refClassp = VN_CAST(nodep->classOrPackageSkipp(), Class);
            // Make sure any extends() are properly imported within referenced class
            ଯଦି (refClassp && !m_statep->forPrimary()) classExtendImport(refClassp);

            ଯଦି (m_statep->forPrimary() && refClassp && !nodep->paramsp()
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
                AstPin* ଅଚଳ pinp = ନୂତନ AstPin{nodep->fileline(), 1, "__paramNumber1", nullptr};
                pinp->param(ସତ୍ୟ);
                nodep->addParamsp(pinp);
            }

            m_ds.init(m_curSymp);
            UINFO(4, indent() << "(Backto) Link ClassOrPackageRef: " << nodep);
            iterateChildren(nodep);
        }

        ଯଦି (nodep->name() == "local::") {
            ଯଦି (!m_randSymp && !m_randMethodCallp) {
                nodep->v3error("Illegal 'local::' outside 'randomize() with'"
                               " (IEEE 1800-2023 18.7.1)");
                m_ds.m_dotErr = ସତ୍ୟ;
                ଫେରନ୍ତୁ;
            }
        }
        ଯଦି (m_ds.m_dotPos == DP_PACKAGE && nodep->classOrPackageSkipp()) {
            m_ds.m_dotSymp = m_statep->get_ଗଣ୍ଠି_Sym(nodep->classOrPackageSkipp());
            UINFO(9, indent() << "set sym " << m_ds.ascii());
        }
    }
    ଶୂନ୍ୟ visit(AstConstraint* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        ଯଦି (nodep->isExternDef()) {
            ଯଦି (ଅଚଳ VSymEnt* ଅଚଳ foundp
                = m_curSymp->findIdFallback("extern " + nodep->name())) {
                ଅଚଳ AstConstraint* ଅଚଳ protop = VN_AS(foundp->nodep(), Constraint);
                // Copy specifiers.
                // External definition cannot have any specifiers, so no value will be overwritten.
                nodep->isStatic(protop->isStatic());
            } ଅନ୍ୟଥା {
                nodep->v3error("extern not found that declares " + nodep->prettyNameQ());
            }
        }
        ଯଦି (nodep->isExternProto()) {
            ଯଦି (!m_curSymp->findIdFallback(nodep->name()) && nodep->isExternExplicit()) {
                nodep->v3warn(PROTOTYPEMIS, "Definition not found for extern prototype "
                                                << nodep->prettyNameQ());
            }
        }
        VL_RESTORER(m_curSymp);
        VL_RESTORER_COPY(m_ds);
        m_ds.m_dotSymp = m_curSymp = m_statep->get_ଗଣ୍ଠି_Sym(nodep);
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstConstraintRef* nodep) override {
        ଯଦି (nodep->user3SetOnce()) ଫେରନ୍ତୁ;
        LINKDOT_VISIT_START();
        UINFO(8, indent() << "visit " << nodep);
        UINFO(8, indent() << m_ds.ascii());
        // No children defined
    }
    ଶୂନ୍ୟ visit(AstVarRef* nodep) override {
        // VarRef: Resolve its reference
        // ParseRefs are used the first pass (forPrimary) so we shouldn't get can't find
        // errors here now that we have a VarRef.
        // No checkNoDot; created and iterated from a parseRef
        LINKDOT_VISIT_START();
        iterateChildren(nodep);
        ଯଦି (!nodep->varp()) {
            UINFO(9, indent() << "linkVarRef se" << cvtToHex(m_curSymp) << "  n=" << nodep);
            UASSERT_OBJ(m_curSymp, nodep, "nullptr lookup symbol table");
            VSymEnt* ଅଚଳ foundp = m_curSymp->findIdFallback(nodep->name());
            ଯଦି (AstVar* ଅଚଳ varp
                = foundp ? foundToVarp(foundp, nodep, nodep->access()) : nullptr) {
                nodep->varp(varp);
                updateVarUse(nodep->varp());
                // Generally set by parse, but might be an import
                nodep->classOrPackagep(foundp->classOrPackagep());
            }
            ଯଦି (VL_UNCOVERABLE(!nodep->varp())) {
                nodep->v3error("Can't find definition of signal, again: "  // LCOV_EXCL_LINE
                               << nodep->prettyNameQ());
            }
        }
        AstVarScope* vscp = nodep->varScopep();
        ଯଦି (vscp && vscp->user2p() != vscp && m_replaceWithAlias) {
            vscp = LinkDotScopeVisitor::getAlias(vscp);
            nodep->varp(vscp->varp());
            nodep->varScopep(vscp);
            updateVarUse(nodep->varp());
            UINFO(7, indent() << "Resolved " << nodep);  // Also prints taskp
        }
    }
    ଶୂନ୍ୟ visit(AstVarXRef* nodep) override {
        // VarRef: Resolve its reference
        // We always link even if varp() is set, because the module we choose may change
        // due to creating new modules, flattening, etc.
        ଯଦି (nodep->user3SetOnce()) ଫେରନ୍ତୁ;
        LINKDOT_VISIT_START();
        UINFO(8, indent() << "visit " << nodep);
        // No checkNoDot; created and iterated from a parseRef
        ଯଦି (!m_modSymp) {
            // Module that is not in hierarchy.  We'll be dead code eliminating it later.
            UINFO(9, "Dead module for " << nodep);
            nodep->varp(nullptr);
        } ଅନ୍ୟଥା {
            VSymEnt* lookupSymp = m_curSymp;  // Start search at current scope
            ଯଦି (nodep->inlinedDots() != "") {  // Correct for current post-inlined scope
                lookupSymp = findInlinedDotsSym(nodep, nodep->inlinedDots());
            }
            string baddot;
            VSymEnt* okSymp;
            ତାର୍କିକ modport;
            VSymEnt* ଅଚଳ dotSymp
                = findDottedRefSymp(nodep, lookupSymp, nodep->dotted(), baddot, okSymp, &modport);
            ଯଦି (!dotSymp) ଫେରନ୍ତୁ;

            ଯଦି (!m_statep->forScopeCreation()) {
                VSymEnt* foundp = nullptr;
                ଯଦି (modport) {
                    // This is a copy of findSymPrefixed with few modifications.
                    // This searches without a fallback like findSymPrefixed but if lookup fails it
                    // will check one fallback (fallback of modport shall be interface) and take
                    // only L and G PARAMs since they should be visible from a modport always.
                    string prefix = dotSymp->symPrefix();
                    ଯେତେବେଳେ (!foundp) {
                        foundp = dotSymp->findIdFlat(prefix + nodep->name());
                        ଯଦି (foundp) ଭାଙ୍ଗ;
                        UASSERT_OBJ(dotSymp->fallbackp(), nodep, "Modports shall have fallback");
                        foundp = dotSymp->fallbackp()->findIdFlat(prefix + nodep->name());
                        ଯଦି (ଅଚଳ AstVar* ଅଚଳ varp
                            = foundToVarp(foundp, nodep, nodep->access())) {
                            ଯଦି (!varp->isParam()) foundp = nullptr;
                        }
                        ଯଦି (prefix.empty()) ଭାଙ୍ଗ;
                        string nextPrefix = LinkDotState::removeLastInlineScope(prefix);
                        ଯଦି (prefix == nextPrefix) ଭାଙ୍ଗ;
                        prefix = std::move(nextPrefix);
                    }
                    baddot = nodep->name();
                } ଅନ୍ୟଥା {
                    foundp = m_statep->findSymPrefixed(dotSymp, nodep->name(), baddot, ସତ୍ୟ);
                }
                ଯଦି (m_inSens && foundp) {
                    ଯଦି (AstClocking* ଅଚଳ clockingp = sensClockingp(foundp->nodep())) {
                        foundp = getCreateClockingEventSymEnt(clockingp);
                    }
                }
                AstVar* ଅଚଳ varp
                    = foundp ? foundToVarp(foundp, nodep, nodep->access()) : nullptr;
                nodep->varp(varp);
                updateVarUse(nodep->varp());
                UINFO(7, indent() << "Resolved " << nodep);  // Also prints varp
                // If found, check if it's ok to access in case it's in a hier_block
                ଯଦି (nodep->varp() && errorHierNonPort(nodep, nodep->varp(), dotSymp)) ଫେରନ୍ତୁ;
                ଯଦି (!nodep->varp()) {
                    nodep->v3error("Can't find definition of "
                                   << Ast_ଗଣ୍ଠି::prettyNameQ(baddot) << " in dotted signal: '"
                                   << nodep->dotted() + "." + nodep->prettyName() << "'\n"
                                   << nodep->warnContextPrimary()
                                   << okSymp->cellErrorScopes(nodep));
                    ଫେରନ୍ତୁ;
                }
            } ଅନ୍ୟଥା {
                VSymEnt* ଅଚଳ foundp
                    = m_statep->findSymPrefixed(dotSymp, nodep->name(), baddot, ସତ୍ୟ);
                AstVarScope* vscp = foundp ? VN_CAST(foundp->nodep(), VarScope) : nullptr;
                // Handle modport expression virtual ports
                ଯଦି (!vscp) vscp = findVirtualPortVarScope(dotSymp, nodep->name());
                // If found, check if it's ok to access in case it's in a hier_block
                ଯଦି (vscp && errorHierNonPort(nodep, vscp->varp(), dotSymp)) ଫେରନ୍ତୁ;
                ଯଦି (!vscp) {
                    nodep->v3error("Can't find varpin scope of "
                                   << Ast_ଗଣ୍ଠି::prettyNameQ(baddot) << " in dotted signal: '"
                                   << nodep->dotted() + "." + nodep->prettyName() << "'\n"
                                   << nodep->warnContextPrimary()
                                   << okSymp->cellErrorScopes(nodep));
                } ଅନ୍ୟଥା {
                    ଯଦି (vscp->user2p() && m_replaceWithAlias) {
                        vscp = LinkDotScopeVisitor::getAlias(vscp);
                    }
                    // Convert the VarXRef to a VarRef, so we don't need
                    // later optimizations to deal with VarXRef.
                    nodep->varp(vscp->varp());
                    nodep->varScopep(vscp);
                    updateVarUse(nodep->varp());
                    UINFO(7, indent() << "Resolved " << nodep);  // Also prints taskp
                    AstVarRef* ଅଚଳ newvscp
                        = ନୂତନ AstVarRef{nodep->fileline(), vscp, nodep->access()};
                    nodep->replaceWith(newvscp);
                    VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
                    UINFO(9, indent() << "new " << newvscp);  // Also prints taskp
                }
            }
        }
    }
    ଶୂନ୍ୟ visit(AstEnumDType* nodep) override {
        LINKDOT_VISIT_START();
        iterateChildren(nodep);
        AstRefDType* ଅଚଳ refdtypep = VN_CAST(nodep->subDTypep(), RefDType);
        ଯଦି (refdtypep && (nodep == refdtypep->subDTypep())) {
            refdtypep->v3error("Self-referential enumerated type definition");
        }
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_UOr_Struct_DType* nodep) override {
        LINKDOT_VISIT_START();
        symIterateChildren(nodep, m_statep->get_ଗଣ୍ଠି_Sym(nodep));
    }
    ଶୂନ୍ୟ visit(AstEnumItemRef* nodep) override {
        // Resolve its reference
        // EnumItemRefs are created by the first pass, but V3Param may regenerate due to
        // a parameterized class/module, so we shouldn't get can't find errors.
        // No checkNoDot; created and iterated from a parseRef
        ଯଦି (nodep->user3SetOnce()) ଫେରନ୍ତୁ;
        LINKDOT_VISIT_START();
        ଯଦି (m_statep->forParamed() && !nodep->dotted().empty() && m_modSymp) {
            // Hierarchical reference, relink as V3Param may have specialized the module
            string baddot;
            VSymEnt* okSymp = nullptr;
            VSymEnt* ଅଚଳ dotSymp
                = findDottedRefSymp(nodep, m_curSymp, nodep->dotted(), baddot, okSymp);
            ଯଦି (!dotSymp) ଫେରନ୍ତୁ;
            VSymEnt* ଅଚଳ foundp
                = m_statep->findSymPrefixed(dotSymp, nodep->name(), baddot, ସତ୍ୟ);
            AstEnumItem* ଅଚଳ itemp = foundp ? VN_CAST(foundp->nodep(), EnumItem) : nullptr;
            ଯଦି (!itemp) {
                nodep->v3error("Enum item " << nodep->prettyNameQ() << " not found in "
                                            << Ast_ଗଣ୍ଠି::prettyNameQ(nodep->dotted()) << '\n'
                                            << nodep->warnContextPrimary());
                ଫେରନ୍ତୁ;
            }
            nodep->itemp(itemp);
            UINFO(9, indent() << " relinked " << nodep);
        }
        ଯଦି (!nodep->itemp()) {
            UINFO(9, indent() << "linkEnumRef se" << cvtToHex(m_curSymp) << "  n=" << nodep);
            UASSERT_OBJ(m_curSymp, nodep, "nullptr lookup symbol table");
            VSymEnt* ଅଚଳ foundp = m_curSymp->findIdFallback(nodep->name());
            ଯଦି (AstEnumItem* ଅଚଳ itemp = foundp ? VN_CAST(foundp->nodep(), EnumItem) : nullptr) {
                nodep->itemp(itemp);
                nodep->classOrPackagep(foundp->classOrPackagep());
                UINFO(9, indent() << " resolved " << nodep);
            }
            ଯଦି (VL_UNCOVERABLE(!nodep->itemp())) {
                nodep->v3error("Can't find definition of enum item, again: "  // LCOV_EXCL_LINE
                               << nodep->prettyNameQ());
            }
        }
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstMethodCall* nodep) override {
        // Created here so should already be resolved.
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        VL_RESTORER_COPY(m_ds);
        VL_RESTORER(m_randSymp);
        VL_RESTORER(m_randMethodCallp);
        {
            m_ds.init(m_curSymp);
            ଯଦି (nodep->name() == "randomize" && (nodep->argsp() || nodep->withp())) {
                m_randMethodCallp = nodep;
                ଅଚଳ Ast_ଗଣ୍ଠି_DType* fromDtp = nodep->fromp()->dtypep();
                ଯଦି (!fromDtp) {
                    ଯଦି (ଅଚଳ Ast_ଗଣ୍ଠି_Var_Ref* ଅଚଳ varRefp = VN_CAST(nodep->fromp(), ଗଣ୍ଠି_Var_Ref)) {
                        fromDtp = varRefp->varp()->subDTypep();
                    } ଅନ୍ୟଥା {
                        fromDtp = get_ପଦସମଷ୍ଟି_DTypep(nodep->fromp());
                    }
                    ଯଦି (!fromDtp) {
                        ଯଦି (nodep->withp()) {
                            nodep->v3warn(
                                E_UNSUPPORTED,
                                "Unsupported: 'randomize() with' on complex expressions");
                        } ଅନ୍ୟଥା {
                            nodep->v3warn(E_UNSUPPORTED,
                                          "Unsupported: Inline random variable control with "
                                          "'randomize()' called on complex expressions");
                        }
                    }
                }
                ଯଦି (m_statep->forPrimary() && isParamedClassRefDType(fromDtp)) {
                    m_ds.m_unresolvedClass = ସତ୍ୟ;
                } ଅନ୍ୟଥା ଯଦି (fromDtp) {
                    ଅଚଳ AstClassRefDType* ଅଚଳ classDtp
                        = VN_CAST(fromDtp->skipRefp(), ClassRefDType);
                    ଯଦି (!classDtp)
                        nodep->v3error("'randomize() with' on a non-class-instance "
                                       << fromDtp->prettyNameQ());
                    ଅନ୍ୟଥା ଯଦି (m_statep->forPrimary() && m_extendsParam.ଗଣନା_2(classDtp->classp())) {
                        // Inherited members are unavailable until the base class is specialized.
                        // Defer lookup rather than incorrectly binding names in the caller.
                        m_ds.m_unresolvedClass = ସତ୍ୟ;
                    } ଅନ୍ୟଥା {
                        m_randSymp = m_statep->get_ଗଣ୍ଠି_Sym(classDtp->classp());
                    }
                }
            }
            iterateChildren(nodep);
        }
    }
    ଶୂନ୍ୟ visit(AstVar* nodep) override {
        LINKDOT_VISIT_START();
        checkNoDot(nodep);
        VL_RESTORER(m_isParam);
        m_isParam = nodep->varType().isParam();
        iterateChildren(nodep);
        ଯଦି (m_statep->forPrimary() && nodep->isIO() && !m_ftaskp && !nodep->user4()) {
            nodep->v3error(
                "Input/output/inout does not appear in port list: " << nodep->prettyNameQ());
        }
    }
    ଶୂନ୍ୟ visit(AstVarScope* nodep) override {
        LINKDOT_VISIT_START();
        checkNoDot(nodep);
        iterateChildren(nodep);
        AstVarScope* aliasp = LinkDotScopeVisitor::getAlias(nodep);
        ଯଦି (aliasp && aliasp != nodep && !nodep->varp()->isIfaceRef()) {
            // Aliased variable might still be references from outside,
            // eg through the VPI, and is traced, so we need the value to propagate.
            // TODO: this means external writes to the LHS (e.g.: through the VPI) don't work
            AstAssignW* ଅଚଳ assignp = ନୂତନ AstAssignW{
                nodep->fileline(), ନୂତନ AstVarRef{nodep->fileline(), nodep, VAccess::WRITE},
                ନୂତନ AstVarRef{nodep->fileline(), aliasp, VAccess::READ}};
            assignp->user2(ସତ୍ୟ);
            nodep->scopep()->addBlocksp(ନୂତନ AstAlways{assignp});
            // Propagate attributes and lint state of the replaced variable,
            // because all references to it are replaced with references to the alias variable
            aliasp->varp()->propagateAttrFrom(nodep->varp());
            aliasp->varp()->fileline()->modifyStateInherit(nodep->varp()->fileline());
        }
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_FTask_Ref* nodep) override {
        ଯଦି (nodep->user3SetOnce()) ଫେରନ୍ତୁ;
        LINKDOT_VISIT_START();
        UINFO(8, indent() << "visit " << nodep);
        UINFO(8, indent() << m_ds.ascii());
        ଯଦି (m_ds.m_dotPos != DP_MEMBER || nodep->name() != "randomize") {
            // Visit arguments at the beginning.
            // They may be visitted even if the current node can't be linked now.
            symIterateChildren(nodep, m_curSymp);
        }

        ଯଦି (m_ds.m_super) {
            ଯଦି (AstFuncRef* ଅଚଳ funcRefp = VN_CAST(nodep, FuncRef)) {
                funcRefp->superReference(ସତ୍ୟ);
            } ଅନ୍ୟଥା ଯଦି (AstTaskRef* ଅଚଳ taskRefp = VN_CAST(nodep, TaskRef)) {
                taskRefp->superReference(ସତ୍ୟ);
            }
        }

        VL_RESTORER(m_randSymp);

        ତାର୍କିକ first = !m_ds.m_dotp || m_ds.m_dotPos == DP_FIRST;
        ତାର୍କିକ staticAccess = ମିଥ୍ୟା;
        ଯଦି (m_ds.m_unresolvedClass) {
            // Unable to link before V3Param
            ଫେରନ୍ତୁ;
        } ଅନ୍ୟଥା ଯଦି (m_ds.m_unresolvedCell && m_ds.m_dotPos == DP_FINAL && m_ds.m_unlinkedScopep) {
            Ast_ଗଣ୍ଠି_FTask_Ref* ଅଚଳ newftaskp = nodep->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);
            newftaskp->dotted(m_ds.m_dotText);
            Ast_ଗଣ୍ଠି* ଅଚଳ newp = ନୂତନ AstUnlinkedRef{nodep->fileline(), newftaskp, nodep->name(),
                                                     m_ds.m_unlinkedScopep->unlinkFrBack()};
            m_ds.m_unlinkedScopep = nullptr;
            m_ds.m_unresolvedCell = ମିଥ୍ୟା;
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
            ଫେରନ୍ତୁ;
        } ଅନ୍ୟଥା ଯଦି (m_ds.m_dotp && m_ds.m_dotPos == DP_PACKAGE) {
            UASSERT_OBJ(VN_IS(m_ds.m_dotp->lhsp(), ClassOrPackageRef), m_ds.m_dotp->lhsp(),
                        "Bad package link");
            staticAccess = ସତ୍ୟ;
            AstClassOrPackageRef* ଅଚଳ cpackagerefp
                = VN_AS(m_ds.m_dotp->lhsp(), ClassOrPackageRef);
            ଯଦି (cpackagerefp->name() == "local::") {
                m_randSymp = nullptr;
                first = ସତ୍ୟ;
            } ଅନ୍ୟଥା ଯଦି (!cpackagerefp->classOrPackageSkipp()) {
                VSymEnt* ଅଚଳ foundp = m_statep->resolveClassOrPackage(
                    m_ds.m_dotSymp, cpackagerefp, ସତ୍ୟ, ମିଥ୍ୟା, ":: reference");
                ଯଦି (foundp) nodep->classOrPackagep(cpackagerefp->classOrPackageSkipp());
            } ଅନ୍ୟଥା {
                nodep->classOrPackagep(cpackagerefp->classOrPackageSkipp());
            }
            // Class/package :: HERE function() . method_called_on_function_return_value()
            m_ds.m_dotPos = DP_MEMBER;
        } ଅନ୍ୟଥା ଯଦି (m_ds.m_dotp && m_ds.m_dotPos == DP_FINAL) {
            nodep->dotted(m_ds.m_dotText);  // Maybe ""
            // Only flag FTaskRefs under generate-if/case blocks that may be
            // pruned.  GenFor and plain begin-blocks won't be pruned by V3Param.
            // A generate-case block's parent is the GenCaseItem, not the GenCase.
            // VarXRef uses the broader m_genBlk flag (set for all GenBlocks)
            // because genfor unrolling also removes variables.
            ଯଦି (m_ds.m_genBlk && m_ds.m_dotSymp) {
                ଅଚଳ Ast_ଗଣ୍ଠି* ଅଚଳ blkp = m_ds.m_dotSymp->nodep();
                ଯଦି (VN_IS(blkp, GenBlock)
                    && (VN_IS(blkp->backp(), GenIf) || VN_IS(blkp->backp(), GenCaseItem))) {
                    nodep->containsGenBlock(ସତ୍ୟ);
                }
            }
        } ଅନ୍ୟଥା ଯଦି (m_ds.m_dotp && m_ds.m_dotPos == DP_MEMBER) {
            // Found a Var, everything following is method call.
            // {scope}.{var}.HERE {method} ( ARGS )
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ varEtcp = VN_AS(m_ds.m_dotp->lhsp()->unlinkFrBack(), ଗଣ୍ଠି_ପଦସମଷ୍ଟି);
            AstArg* ଅଚଳ argsp = nodep->argsp();
            ଯଦି (argsp) argsp->unlinkFrBackWithNext();
            AstMethodCall* ଅଚଳ newp = ନୂତନ AstMethodCall{nodep->fileline(), varEtcp,
                                                          VFlagChildDType{}, nodep->name(), argsp};
            ଯଦି (AstWith* ଅଚଳ withp = nodep->withp()) newp->withp(withp->unlinkFrBack());
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
            ଫେରନ୍ତୁ;
        } ଅନ୍ୟଥା ଯଦି (m_ds.m_dotp && (m_ds.m_dotPos == DP_SCOPE || m_ds.m_dotPos == DP_FIRST)) {
            // HERE function() . method_called_on_function_return_value()
            m_ds.m_dotPos = DP_MEMBER;
            m_ds.m_dotText = "";
        } ଅନ୍ୟଥା ଯଦି (!m_ds.m_disablep) {
            // visit(AstDisable*) setup the dot handling
            checkNoDot(nodep);
        }
        ଯଦି (nodep->classOrPackagep() && nodep->taskp()) {
            // References into packages don't care about cell hierarchy.
        } ଅନ୍ୟଥା ଯଦି (!m_modSymp) {
            // Module that is not in hierarchy.  We'll be dead code eliminating it later.
            UINFO(9, indent() << "Dead module for " << nodep);
            nodep->taskp(nullptr);
        } ଅନ୍ୟଥା ଯଦି (nodep->dotted() == "" && nodep->taskp()) {
            // Earlier should have setup the links
            // Might be under a BEGIN we're not processing, so don't relink it
        } ଅନ୍ୟଥା {
            string baddot;
            VSymEnt* okSymp = nullptr;
            VSymEnt* dotSymp;
            ଯଦି (nodep->dotted().empty()) {
                // Non-'super.' dotted reference
                dotSymp = m_ds.m_dotSymp;
            } ଅନ୍ୟଥା {
                // Start search at module, as a variable of same name under a subtask isn't a
                // relevant hit however a function under a begin/end is.  So we want begins, but
                // not the function
                dotSymp = m_curSymp;
            }
            ଯଦି (nodep->classOrPackagep()) {  // Look only in specified package
                dotSymp = m_statep->get_ଗଣ୍ଠି_Sym(nodep->classOrPackagep());
                UINFO(8, indent() << "Override classOrPackage " << dotSymp);
            } ଅନ୍ୟଥା {
                ଯଦି (nodep->inlinedDots() != "") {  // Correct for current post-inlined scope
                    dotSymp = findInlinedDotsSym(nodep, nodep->inlinedDots());
                }
                dotSymp = m_statep->findDotted(nodep->fileline(), dotSymp, nodep->dotted(), baddot,
                                               okSymp, ସତ୍ୟ);  // Maybe nullptr
            }
            // Restricted 'with (id_list)': only id_list names bind into target class.
            ଯଦି (m_randSymp
                && (!m_currentWithp || m_currentWithp->nameResolvesToTarget(nodep->name()))) {
                VSymEnt* ଅଚଳ foundp = m_randSymp->findIdFlat(nodep->name());
                ଯଦି (foundp && m_currentWithp) {
                    ଯଦି (m_currentWithp->restricted()) {
                        m_restrictedNamesUsed.ଭର୍ତ୍ତି(nodep->name());
                    }
                    UINFO(9, indent() << "randomize-with fromSym " << foundp->nodep());
                    AstArg* argsp = nullptr;
                    ଯଦି (nodep->argsp()) {
                        iterateAndNextNull(nodep->argsp());
                        argsp = nodep->argsp()->unlinkFrBackWithNext();
                    }
                    ଯଦି (m_ds.m_dotPos != DP_NONE) m_ds.m_dotPos = DP_MEMBER;
                    Ast_ଗଣ୍ଠି* ଅଚଳ newp = ନୂତନ AstMethodCall{
                        nodep->fileline(),
                        ନୂତନ AstLambdaArgRef{nodep->fileline(), "__Vrandwith_obj", ମିଥ୍ୟା},
                        VFlagChildDType{}, nodep->name(), argsp};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
                    ଫେରନ୍ତୁ;
                }
            }
            ଯଦି (first && nodep->name() == "randomize" && VN_IS(m_modp, Class)
                && !VN_IS(nodep->classOrPackagep(), Package)) {
                // need special handling to avoid falling back to std::randomize
                // Skip if classOrPackagep is a Package (i.e. std::randomize resolved earlier)
                VMemberMap memberMap;
                AstFunc* ଅଚଳ randFuncp = V3Randomize::newRandomizeFunc(
                    memberMap, VN_AS(m_modp, Class), nodep->name(), ସତ୍ୟ, ସତ୍ୟ);
                nodep->taskp(randFuncp);
                nodep->classOrPackagep(VN_AS(m_modp, Class));
                m_curSymp = m_statep->ଭର୍ତ୍ତି_Block(m_curSymp, nodep->name(), randFuncp, m_modp);
                // Already linked to the class randomize function; skip findSymPrefixed
                // which may find std::randomize and overwrite classOrPackagep
                ଫେରନ୍ତୁ;
            }
            VSymEnt* ଅଚଳ foundp
                = m_statep->findSymPrefixed(dotSymp, nodep->name(), baddot, first);
            Ast_ଗଣ୍ଠି_FTask* ଅଚଳ taskp = foundp ? VN_CAST(foundp->nodep(), ଗଣ୍ଠି_FTask) : nullptr;
            Ast_ଗଣ୍ଠି_Module* ଅଚଳ taskModulep = foundp ? foundp->classOrPackagep() : nullptr;
            // Ignore deferring linking of functions inside params of classes extending
            // parametric classes if referenced function can be linked in the first pass.
            ଯଦି (!m_isParam || !taskp
                || (taskp->classMethod() && taskModulep && taskModulep->hasParameterList())) {
                ଯଦି (m_insideClassExtParam) {
                    // The reference may point to a method declared in a super class, which is
                    // proved by a parameter. In such a case, it can't be linked at the first
                    // stage. Must not do any linking, because e.g. might find an extends of an
                    // upper class because the current class (under parent) isn't yet importing
                    // it's extended class symbols.
                    ଫେରନ୍ତୁ;
                }
                ଯଦି (AstClass* ଅଚଳ targetClassp = VN_CAST(dotSymp->nodep(), Class)) {
                    ଯଦି (m_extendsParam.ଗଣନା_2(targetClassp)) {
                        // Target class has parameterized extends not yet resolved.
                        // Its inherited symbols (e.g. static functions from the base class)
                        // aren't imported yet - defer to linkDotParamed.
                        ଫେରନ୍ତୁ;
                    }
                }
            }

            ଯଦି (taskp) {
                ଯଦି (staticAccess && !taskp->isStatic()) {
                    // TODO bug4077
                    // nodep->v3error("Static access to non-static task/function "
                    //                << taskp->prettyNameQ() << endl);
                }
                nodep->taskp(taskp);
                nodep->classOrPackagep(foundp->classOrPackagep());
                UINFO(7, indent() << "Resolved " << nodep);  // Also prints taskp
            } ଅନ୍ୟଥା {
                // Note ParseRef has similar error handling/message output
                UINFO(7, indent() << "   ErrFtask curSymp=se" << cvtToHex(m_curSymp)
                                  << " dotSymp=se" << cvtToHex(dotSymp));
                ଯଦି (foundp) {
                    ଯଦି (VN_IS(foundp->nodep(), Var) && m_ds.m_dotText == "" && m_ftaskp
                        && m_ftaskp->name() == foundp->nodep()->name()) {
                        // This is a recursive reference to the function itself, not to the var
                        nodep->taskp(m_ftaskp);
                        nodep->classOrPackagep(foundp->classOrPackagep());
                        UINFO(7, indent() << "Resolved recursive " << nodep);  // Also prints taskp
                    } ଅନ୍ୟଥା {
                        nodep->v3error("Found definition of '"
                                       << m_ds.m_dotText << (m_ds.m_dotText == "" ? "" : ".")
                                       << nodep->prettyName() << "'" << " as a "
                                       << foundp->nodep()->typeName()
                                       << " but expected a task/function");
                    }
                } ଅନ୍ୟଥା ଯଦି (VN_IS(nodep, New) && !m_statep->forScopeCreation()) {
                    // Resolved in V3Width
                } ଅନ୍ୟଥା ଯଦି ((nodep->name() == "pre_randomize" || nodep->name() == "post_randomize")
                           && VN_IS(dotSymp->nodep(), Class)) {
                    AstClass* ଅଚଳ classp = VN_AS(dotSymp->nodep(), Class);
                    // Functions that might be defined by user, but if not, they get empty
                    // definitions within referenced class.
                    // TODO perhaps cleaner to add to symbol table up-front with
                    // a lambda to construct them on a reference.
                    AstFunc* ଅଚଳ funcp
                        = ନୂତନ AstFunc{nodep->fileline(), nodep->name(), nullptr, nullptr};
                    funcp->classMethod(ସତ୍ୟ);
                    funcp->dtypep(funcp->findVoidDType());
                    classp->addMembersp(funcp);
                    m_statep->ଭର୍ତ୍ତି_Block(dotSymp, nodep->name(), funcp, classp);
                    nodep->taskp(funcp);
                    nodep->classOrPackagep(classp);
                    UINFO(9, indent() << " Auto-made " << funcp);
                    ଫେରନ୍ତୁ;
                } ଅନ୍ୟଥା ଯଦି (nodep->name() == "randomize" || nodep->name() == "srandom"
                           || nodep->name() == "get_randstate" || nodep->name() == "set_randstate"
                           || nodep->name() == "rand_mode" || nodep->name() == "constraint_mode") {
                    ଯଦି (AstClass* ଅଚଳ classp = VN_CAST(m_modp, Class)) {
                        // Don't overwrite if already resolved to a package (e.g. std::randomize)
                        ଯଦି (!VN_IS(nodep->classOrPackagep(), Package))
                            nodep->classOrPackagep(classp);
                    } ଅନ୍ୟଥା ଯଦି (nodep->name() == "randomize") {
                        // A std::randomize resolved in V3Width
                        nodep->classOrPackagep(v3Global.rootp()->stdPackagep());
                    } ଅନ୍ୟଥା {
                        nodep->v3error("Calling implicit class method "
                                       << nodep->prettyNameQ() << " without being under class");
                        nodep->replaceWith(ନୂତନ AstConst{nodep->fileline(), 0});
                        VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
                        ଫେରନ୍ତୁ;
                    }
                } ଅନ୍ୟଥା ଯଦି (nodep->dotted() == "") {
                    ଯଦି (nodep->pli()) {
                        ଯଦି (v3Global.opt.bboxSys()) {
                            Ast_ଗଣ୍ଠି* newp;
                            ଯଦି (VN_IS(nodep, FuncRef)) {
                                newp = ନୂତନ AstConst{nodep->fileline(), AstConst::All0{}};
                            } ଅନ୍ୟଥା {
                                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* outp = nullptr;
                                ଯେତେବେଳେ (AstArg* ଅଚଳ argp = nodep->argsp()) {
                                    outp = Ast_ଗଣ୍ଠି::addNext(outp, argp->exprp()->unlinkFrBack());
                                    VL_DO_DANGLING(ଠେଲ_Deletep(argp->unlinkFrBack()), argp);
                                }
                                newp = ନୂତନ AstSysIgnore{nodep->fileline(), outp};
                                newp->dtypep(nodep->dtypep());
                            }
                            nodep->replaceWith(newp);
                            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
                            ଫେରନ୍ତୁ;
                        } ଅନ୍ୟଥା {
                            VSpellCheck speller;
                            V3Parse::candidatePli(&speller);
                            ଅଚଳ string suggest = speller.best_Candidate_ସନ୍ଦେଶ(nodep->prettyName());
                            nodep->v3error(
                                "Unsupported or unknown PLI call: "
                                << nodep->prettyNameQ() << '\n'
                                << (suggest.empty() ? "" : nodep->warnMore() + suggest));
                        }
                    } ଅନ୍ୟଥା {
                        ଅଚଳ string suggest = m_statep->suggestSymFallback(
                            dotSymp, nodep->name(), Link_ଗଣ୍ଠି_Matcher_FTask{});
                        nodep->v3error("Can't find definition of task/function: "
                                       << nodep->prettyNameQ() << '\n'
                                       << (suggest.empty() ? "" : nodep->warnMore() + suggest));
                    }
                } ଅନ୍ୟଥା {
                    ଅଚଳ string suggest = m_statep->suggestSymFallback(dotSymp, nodep->name(),
                                                                        Link_ଗଣ୍ଠି_Matcher_FTask{});
                    nodep->v3error("Can't find definition of "
                                   << Ast_ଗଣ୍ଠି::prettyNameQ(baddot) << " in dotted task/function: '"
                                   << nodep->dotted() + "." + nodep->prettyName() << "'\n"
                                   << (suggest.empty() ? "" : nodep->warnMore() + suggest) << '\n'
                                   << nodep->warnContextPrimary()
                                   << okSymp->cellErrorScopes(nodep));
                }
            }
        }
    }
    ଶୂନ୍ୟ visit(AstSelBit* nodep) override {
        ଯଦି (nodep->user3SetOnce()) ଫେରନ୍ତୁ;
        LINKDOT_VISIT_START();
        iterateAndNextNull(nodep->fromp());
        ଯଦି (m_ds.m_unresolvedClass) {
            UASSERT_OBJ(m_ds.m_dotPos != DP_SCOPE && m_ds.m_dotPos != DP_FIRST, nodep,
                        "Object of unresolved class on scope position in dotted reference");
            // The index does not depend on the class, and V3Param must see its class references
            symIterateNull(nodep->bitp(), m_curSymp);
            symIterateNull(nodep->attrp(), m_curSymp);
            ଫେରନ୍ତୁ;
        }
        ଯଦି (m_ds.m_dotPos == DP_SCOPE
            || m_ds.m_dotPos
                   == DP_FIRST) {  // Already under dot, so this is {modulepart} DOT {modulepart}
            UINFO(9, indent() << "deferring until after a V3Param pass: " << nodep);
            m_ds.m_dotText += "__BRA__??__KET__";
            m_ds.m_unresolvedCell = ସତ୍ୟ;
            // And pass up m_ds.m_dotText
        }
        // Pass dot state down to only fromp()
        iterateAndNextNull(nodep->fromp());
        symIterateNull(nodep->bitp(), m_curSymp);
        symIterateNull(nodep->attrp(), m_curSymp);
        ଯଦି (m_ds.m_unresolvedCell && (m_ds.m_dotPos == DP_SCOPE || m_ds.m_dotPos == DP_FIRST)) {
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ exprp = nodep->bitp()->unlinkFrBack();
            ଯଦି (AstCellArrayRef* ଅଚଳ fromArrRefp = VN_CAST(nodep->fromp(), CellArrayRef)) {
                // Multi-dim iface array access: append this select to the existing chain
                fromArrRefp->addSelp(exprp);
                AstCellArrayRef* ଅଚଳ movedp = fromArrRefp->unlinkFrBack();
                nodep->replaceWith(movedp);
                VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
            } ଅନ୍ୟଥା {
                AstCellArrayRef* ଅଚଳ newp
                    = ନୂତନ AstCellArrayRef{nodep->fileline(), nodep->fromp()->name(), exprp};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
            }
        }
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Pre_Sel* nodep) override {
        // Excludes simple AstSelBit, see above
        ଯଦି (nodep->user3SetOnce()) ଫେରନ୍ତୁ;
        LINKDOT_VISIT_START();
        ଯଦି (m_ds.m_dotPos == DP_SCOPE
            || m_ds.m_dotPos
                   == DP_FIRST) {  // Already under dot, so this is {modulepart} DOT {modulepart}
            nodep->v3error("Syntax error: Range ':', '+:' etc are not allowed in the instance "
                           "part of a dotted reference");
            m_ds.m_dotErr = ସତ୍ୟ;
            ଫେରନ୍ତୁ;
        }
        Ast_ଗଣ୍ଠି_DType* packedArrayDtp = nullptr;  // Datatype reference for packed array
        {
            VL_RESTORER(m_packedArrayDtp);
            iterateAndNextNull(nodep->fromp());
            symIterateNull(nodep->rhsp(), m_curSymp);
            symIterateNull(nodep->thsp(), m_curSymp);

            ଯଦି (m_packedArrayDtp) {
                AstRange* ଅଚଳ newRangep
                    = ନୂତନ AstRange(nodep->fileline(), nodep->rhsp()->unlinkFrBack(),
                                   nodep->thsp()->unlinkFrBack());
                AstPackArrayDType* newArrayTypep
                    = ନୂତନ AstPackArrayDType(nodep->fileline(), m_packedArrayDtp, newRangep);
                newArrayTypep->childDTypep(m_packedArrayDtp);
                newArrayTypep->refDTypep(nullptr);
                ଯଦି (VN_IS(nodep->backp(), SelExtract)) {
                    packedArrayDtp = newArrayTypep;
                } ଅନ୍ୟଥା {
                    replaceWithCheckBreak(nodep, newArrayTypep);
                    VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
                }
            } ଅନ୍ୟଥା {
                ଯଦି (nodep->attrp()) {
                    Ast_ଗଣ୍ଠି* ଅଚଳ attrp = nodep->attrp()->unlinkFrBack();
                    VL_DO_DANGLING(attrp->delete_ବୃକ୍ଷ(), attrp);
                }
                Ast_ଗଣ୍ଠି* ଅଚଳ basefromp = nodep->baseFromp(ମିଥ୍ୟା);
                ଯଦି (VN_IS(basefromp, Replicate)) {
                    // From {...}[...] syntax in IEEE 2017
                    ଯଦି (basefromp) UINFO(9, indent() << " Related node: " << basefromp);
                } ଅନ୍ୟଥା {
                    nodep->attrp(ନୂତନ AstAttrOf{nodep->fileline(), VAttrType::VAR_BASE,
                                               basefromp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା)});
                }
            }
        }

        ଯଦି (packedArrayDtp) m_packedArrayDtp = packedArrayDtp;
    }
    ଶୂନ୍ୟ visit(AstMemberSel* nodep) override {
        // checkNoDot not appropriate, can be under a dot
        LINKDOT_VISIT_START();
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstGenBlock* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        {
            VL_RESTORER(m_curSymp);
            VL_RESTORER_COPY(m_ds);
            ଯଦି (nodep->name() != "") {
                m_ds.m_dotSymp = m_curSymp = m_statep->get_ଗଣ୍ଠି_Sym(nodep);
                UINFO(5, indent() << "cur=se" << cvtToHex(m_curSymp));
            }
            iterateChildren(nodep);
        }
        UINFO(5, indent() << "cur=se" << cvtToHex(m_curSymp));
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Block* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        {
            VL_RESTORER(m_curSymp);
            VL_RESTORER_COPY(m_ds);
            ଯଦି (nodep->name() != "") {
                m_ds.m_dotSymp = m_curSymp = m_statep->get_ଗଣ୍ଠି_Sym(nodep);
                UINFO(5, indent() << "cur=se" << cvtToHex(m_curSymp));
            }
            iterateChildren(nodep);
        }
        UINFO(5, indent() << "cur=se" << cvtToHex(m_curSymp));
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_FTask* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        ଯଦି (nodep->isExternDef() && !nodep->didProto()) {
            nodep->didProto(ସତ୍ୟ);
            ଯଦି (ଅଚଳ VSymEnt* ଅଚଳ foundp
                = m_curSymp->findIdFallback("extern " + nodep->name())) {
                ଅଚଳ Ast_ଗଣ୍ଠି_FTask* ଅଚଳ protop = VN_AS(foundp->nodep(), ଗଣ୍ଠି_FTask);
                // Copy specifiers.
                // External definition cannot have any specifiers, so no value will be overwritten.
                nodep->isHideLocal(protop->isHideLocal());
                nodep->isHideProtected(protop->isHideProtected());
                nodep->isStatic(protop->isStatic());
                nodep->isVirtual(protop->isVirtual());
                nodep->lifetime(protop->lifetime());
                // Always add an FUNC_RETURN_PROTO even if task, so we can locate the prototype
                nodep->addStmtsp(ନୂତନ AstAttrOf{protop->fileline(), VAttrType::FUNC_RETURN_PROTO,
                                               protop->fvarp()
                                                   ? protop->fvarp()->clone_ବୃକ୍ଷ(ମିଥ୍ୟା)
                                                   : ନୂତନ AstVoidDType{protop->fileline()}});
                ପାଇଁ (Ast_ଗଣ୍ଠି* stmtp = protop->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    ଯଦି (AstVar* ଅଚଳ portp = VN_CAST(stmtp, Var)) {
                        AstAttrOf* ଅଚଳ attrp
                            = ନୂତନ AstAttrOf{protop->fileline(), VAttrType::FUNC_ARG_PROTO,
                                            portp->subDTypep()->clone_ବୃକ୍ଷ(ମିଥ୍ୟା)};
                        attrp->name(portp->name());
                        nodep->addStmtsp(attrp);
                    }
                }
            } ଅନ୍ୟଥା {
                nodep->v3error("extern not found that declares " + nodep->prettyNameQ());
            }
        }
        ଯଦି (nodep->isExternProto()) {
            ଯଦି (!m_curSymp->findIdFallback(nodep->name())) {
                nodep->v3warn(PROTOTYPEMIS,
                              "Definition not found for extern prototype " + nodep->prettyNameQ());
            }
        }
        VL_RESTORER(m_curSymp);
        VL_RESTORER(m_ftaskp);
        VL_RESTORER(m_explicitSuperNewp);
        {
            m_ftaskp = nodep;
            m_ds.m_dotSymp = m_curSymp = m_statep->get_ଗଣ୍ଠି_Sym(nodep);
            ଅଚଳ ତାର୍କିକ isNew = nodep->name() == "new";
            ଯଦି (isNew) m_explicitSuperNewp = nullptr;
            iterateChildren(nodep);
            ଯଦି (isNew) {
                ଅଚଳ AstClassExtends* ଅଚଳ classExtendsp = VN_AS(m_modp, Class)->extendsp();
                ଯଦି (m_explicitSuperNewp && !m_explicitSuperNewp->isImplicit() && classExtendsp
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
                ଯଦି (classExtendsp && classExtendsp->classOrNullp()) {
                    ଯଦି (!m_explicitSuperNewp && m_statep->forParamed()
                        && !VN_AS(classExtendsp->childDTypep()->skipRefp(), ClassRefDType)
                                ->classp()
                                ->isInterfaceClass()) {
                        Ast_ଗଣ୍ଠି_Stmt* ଅଚଳ superNewp
                            = addImplicitSuperNewCall(VN_AS(nodep, Func), classExtendsp);
                        UINFO(9, "created super new " << superNewp);
                        iterate(superNewp);
                    }
                }
            }
        }
        m_ds.m_dotSymp = VL_RESTORER_PREV(m_curSymp);
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Foreach* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        symIterateChildren(nodep, m_statep->get_ଗଣ୍ଠି_Sym(nodep));
    }
    ଶୂନ୍ୟ visit(AstRandSequence* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        ଯଦି (!nodep->start().empty())
            nodep->prodp(findProd(nodep, m_statep->get_ଗଣ୍ଠି_Sym(nodep), nodep->start()));
        symIterateChildren(nodep, m_statep->get_ଗଣ୍ଠି_Sym(nodep));
    }
    ଶୂନ୍ୟ visit(AstRSProd* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        symIterateChildren(nodep, m_statep->get_ଗଣ୍ଠି_Sym(nodep));
    }
    ଶୂନ୍ୟ visit(AstRSProdItem* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        nodep->prodp(findProd(nodep, m_curSymp, nodep->name()));
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstCoverWith* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        iterateAndNextNull(nodep->subp());
        // The type of 'item' is that of the coverpoint expression, which may name 'item' too
        iterateAndNextNull(nodep->itemp()->childDTypep());
        VL_RESTORER(m_curSymp);
        m_ds.m_dotSymp = m_curSymp = m_statep->get_ଗଣ୍ଠି_Sym(nodep);
        iterateAndNextNull(nodep->itemp());
        iterateAndNextNull(nodep->filterp());
        m_ds.m_dotSymp = VL_RESTORER_PREV(m_curSymp);
    }
    ଶୂନ୍ୟ visit(AstCoverCrossWith* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        iterateAndNextNull(nodep->selectp());
        iterateAndNextNull(nodep->countp());
        // The type of a cross item is that of its coverpoint's expression, which may name items
        iterateAndNextNull(nodep->varsp());
        VL_RESTORER(m_curSymp);
        m_ds.m_dotSymp = m_curSymp = m_statep->get_ଗଣ୍ଠି_Sym(nodep);
        iterateAndNextNull(nodep->filterp());
        m_ds.m_dotSymp = VL_RESTORER_PREV(m_curSymp);
    }
    ଶୂନ୍ୟ visit(AstWith* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        ଯଦି (m_ds.m_unresolvedClass) ଫେରନ୍ତୁ;
        VL_RESTORER(m_curSymp);
        VL_RESTORER(m_currentWithp);
        VL_RESTORER_COPY(m_restrictedNamesUsed);
        {
            m_ds.m_dotSymp = m_curSymp = m_statep->get_ଗଣ୍ଠି_Sym(nodep);
            m_currentWithp = nodep;
            // Validate the identifier_list once per AstWith (m_validated travels
            // with cloneTree, so cloned witnesses skip the redundant check).
            ଅଚଳ ତାର୍କିକ firstVisit = !nodep->validated();
            ଯଦି (firstVisit) {
                nodep->validated(ସତ୍ୟ);
                m_restrictedNamesUsed.clear();
                // IEEE 1800-2023 18.7: each name in the identifier_list must designate
                // a member of the randomize() target class.
                ଯଦି (nodep->restricted() && m_randSymp) {
                    ପାଇଁ (ଅଚଳ std::string& n : nodep->restrictedNames()) {
                        ଯଦି (!m_randSymp->findIdFlat(n)) {
                            nodep->v3error("Identifier '"
                                           << n
                                           << "' in 'with (...)' identifier list is not a"
                                              " member of the randomize() target class"
                                              " (IEEE 1800-2023 18.7)");
                            m_restrictedNamesUsed.ଭର୍ତ୍ତି(n);
                        }
                    }
                }
            }
            iterateChildren(nodep);
            // Flag names that the user listed but never referenced in the constraint.
            ଯଦି (firstVisit && nodep->restricted()) {
                ପାଇଁ (ଅଚଳ std::string& n : nodep->restrictedNames()) {
                    ଯଦି (!m_restrictedNamesUsed.ଗଣନା_2(n)) {
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
    ଶୂନ୍ୟ visit(AstLambdaArgRef* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        // No checknodot(nodep), visit(AstScope) will check for LambdaArgRef
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstClassExtends* nodep) override {
        // Resolve the symbol and get the class.
        // If it is a parameterized case, the class will be resolved after V3Param.cpp
        ଯଦି (nodep->user3SetOnce()) ଫେରନ୍ତୁ;
        // If the class is resolved, there is nothing more to do
        ଯଦି (nodep->classOrNullp()) ଫେରନ୍ତୁ;
        LINKDOT_VISIT_START();
        ଯଦି (m_statep->forPrimary()) {
            ଯଦି (nodep->childDTypep()) ଫେରନ୍ତୁ;
            Ast_ଗଣ୍ଠି* cprp = nodep->classOrPkgsp();
            VSymEnt* lookSymp = m_curSymp;
            ଯଦି (AstDot* ଅଚଳ dotp = VN_CAST(cprp, Dot)) {
                dotp->user3(ସତ୍ୟ);
                ଯଦି (AstClassOrPackageRef* lookNodep = VN_CAST(dotp->lhsp(), ClassOrPackageRef)) {
                    iterate(lookNodep);
                    cprp = dotp->rhsp();
                    VSymEnt* ଅଚଳ foundp = m_statep->resolveClassOrPackage(
                        lookSymp, lookNodep, ସତ୍ୟ, ମିଥ୍ୟା, nodep->verilogKwd());
                    ଯଦି (!foundp) {
                        VL_DO_DANGLING(ଠେଲ_Deletep(nodep->unlinkFrBack()), nodep);
                        ଫେରନ୍ତୁ;
                    }
                    UASSERT_OBJ(lookNodep->classOrPackageSkipp(), nodep, "Bad package link");
                    lookSymp = m_statep->get_ଗଣ୍ଠି_Sym(lookNodep->classOrPackageSkipp());
                } ଅନ୍ୟଥା {
                    dotp->lhsp()->v3error("Attempting to extend"  // LCOV_EXCL_LINE
                                          " using non-class under dot");
                }
            }
            AstClassOrPackageRef* ଅଚଳ cpackagerefp = VN_CAST(cprp, ClassOrPackageRef);
            ଯଦି (VL_UNCOVERABLE(!cpackagerefp)) {
                // Linking the extend gives an error before this is hit
                nodep->v3error("Attempting to extend using non-class");  // LCOV_EXCL_LINE
                VL_DO_DANGLING(ଠେଲ_Deletep(nodep->unlinkFrBack()), nodep);
                ଫେରନ୍ତୁ;
            }
            VSymEnt* ଅଚଳ foundp = m_statep->resolveClassOrPackage(lookSymp, cpackagerefp, ସତ୍ୟ,
                                                                    ସତ୍ୟ, nodep->verilogKwd());
            ଯଦି (!foundp) {
                VL_DO_DANGLING(ଠେଲ_Deletep(nodep->unlinkFrBack()), nodep);
                ଫେରନ୍ତୁ;
            }
            ଯଦି (AstClass* ଅଚଳ classp = VN_CAST(foundp->nodep(), Class)) {
                AstPin* paramsp = cpackagerefp->paramsp();
                ଯଦି (paramsp) {
                    paramsp = paramsp->clone_ବୃକ୍ଷ(ସତ୍ୟ);
                    nodep->parameterized(ସତ୍ୟ);
                }
                nodep->childDTypep(ନୂତନ AstClassRefDType{nodep->fileline(), classp, paramsp});
                // Link pins
                iterate(nodep->childDTypep());
            } ଅନ୍ୟଥା ଯଦି (AstParamTypeDType* ଅଚଳ paramp
                       = VN_CAST(foundp->nodep(), ParamTypeDType)) {
                AstRefDType* ଅଚଳ refParamp = ନୂତନ AstRefDType{nodep->fileline(), paramp->name()};
                refParamp->refDTypep(paramp);
                nodep->childDTypep(refParamp);
                nodep->parameterized(ସତ୍ୟ);
            } ଅନ୍ୟଥା ଯଦି (AstTypedef* ଅଚଳ typedefp = VN_CAST(foundp->nodep(), Typedef)) {
                Ast_ଗଣ୍ଠି_DType* ଅଚଳ unwrappedp = typedefp->subDTypep()->skipRefp();
                ଯଦି (AstClassRefDType* ଅଚଳ classRefp = VN_CAST(unwrappedp, ClassRefDType)) {
                    AstPin* paramsp = cpackagerefp->paramsp();
                    ଯଦି (!paramsp && classRefp->paramsp()) {
                        // No explicit #(...) on extends clause; carry over the
                        // typedef's type-parameter pins so V3Param sees them.
                        paramsp = classRefp->paramsp()->clone_ବୃକ୍ଷ(ସତ୍ୟ);
                        nodep->parameterized(ସତ୍ୟ);
                    } ଅନ୍ୟଥା ଯଦି (paramsp) {
                        paramsp = paramsp->clone_ବୃକ୍ଷ(ସତ୍ୟ);
                        nodep->parameterized(ସତ୍ୟ);
                    }
                    nodep->childDTypep(
                        ନୂତନ AstClassRefDType{nodep->fileline(), classRefp->classp(), paramsp});
                    iterate(nodep->childDTypep());
                } ଅନ୍ୟଥା {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: " << foundp->nodep()->prettyTypeName()
                                                  << " in 'class extends'");
                    ଫେରନ୍ତୁ;
                }
            } ଅନ୍ୟଥା {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: " << foundp->nodep()->prettyTypeName()
                                                             << " in 'class extends'");
                ଫେରନ୍ତୁ;
            }
            ଯଦି (!nodep->childDTypep()) nodep->v3error("Attempting to extend using non-class");
            nodep->classOrPkgsp()->unlinkFrBack()->delete_ବୃକ୍ଷ();
        } ଅନ୍ୟଥା {
            // Probably a parameter
            ଯଦି (AstRefDType* ଅଚଳ refp = VN_CAST(nodep->childDTypep(), RefDType)) {
                ଯଦି (AstClassRefDType* classRefp = VN_CAST(refp->skipRefp(), ClassRefDType)) {
                    // Resolved to a class reference.
                    refp->replaceWith(classRefp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା));
                    VL_DO_DANGLING(ଠେଲ_Deletep(refp), refp);
                } ଅନ୍ୟଥା {
                    // Unable to resolve the ref type to a class reference.
                    // Get the value of type parameter passed to the class instance,
                    // to print the helpful error message.
                    ଅଚଳ Ast_ଗଣ୍ଠି_DType* typep = refp->refDTypep();
                    ଯେତେବେଳେ (ସତ୍ୟ) {
                        ଯଦି (ଅଚଳ AstParamTypeDType* ଅଚଳ atypep
                            = VN_CAST(typep, ParamTypeDType)) {
                            typep = atypep->subDTypep();
                            ଜାରି;
                        }
                        ଯଦି (ଅଚଳ AstRequireDType* ଅଚଳ atypep = VN_CAST(typep, RequireDType)) {
                            typep = atypep->subDTypep();
                            ଜାରି;
                        }
                        ଭାଙ୍ଗ;
                    }
                    typep->v3error("Attempting to extend using non-class");
                }
            }
        }
    }
    ଶୂନ୍ୟ visit(AstClass* nodep) override {
        ଯଦି (nodep->user3SetOnce()) ଫେରନ୍ତୁ;
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
            m_ds.m_dotSymp = m_curSymp = m_modSymp = m_statep->get_ଗଣ୍ଠି_Sym(nodep);
            m_modp = nodep;
            ପୂର୍ଣ୍ଣସଂଖ୍ୟା next = 0;
            ପାଇଁ (AstClassExtends* cextp = nodep->extendsp(); cextp;
                 cextp = VN_AS(cextp->nextp(), ClassExtends)) {
                // Replace abstract reference with hard pointer
                // Will need later resolution when deal with parameters
                ଯଦି (++next == 2 && !nodep->isInterfaceClass() && !cextp->isImplements()) {
                    cextp->v3error("Multiple inheritance illegal on non-interface classes"
                                   " (IEEE 1800-2023 8.13)");
                }
                iterate(cextp);
                ଯଦି (m_statep->forPrimary()) {
                    ଯଦି (cextp->parameterized()) {
                        // Parameters in extends statement.
                        // The class can't be resolved in the current pass.
                        m_extendsParam.ଭର୍ତ୍ତି(nodep);
                        m_insideClassExtParam = ସତ୍ୟ;
                    }
                    ଯଦି (AstClassRefDType* ଅଚଳ classRefp
                        = VN_CAST(cextp->childDTypep(), ClassRefDType)) {
                        AstClass* ଅଚଳ classp = classRefp->classp();
                        ଯଦି (classp != nodep) iterate(classp);
                        ଯଦି (m_extendsParam.find(classp) != m_extendsParam.end()) {
                            // One of its super classes has parameters in extends statement.
                            // Some links may not be resolved in the first pass.
                            m_extendsParam.ଭର୍ତ୍ତି(nodep);
                            m_insideClassExtParam = ସତ୍ୟ;
                        }
                    }
                }

                ଯଦି (AstClass* ଅଚଳ baseClassp = cextp->classOrNullp()) {
                    // Already converted. Update symbol table to link unlinked members.
                    // Base class has to be visited in a case if its extends statement
                    // needs to be handled. Recursive inheritance was already checked.
                    // Must be here instead of in LinkDotParam to handle
                    // "class (type T) extends T".
                    ଯଦି (baseClassp == nodep) {
                        cextp->v3error("Attempting to extend class " << nodep->prettyNameQ()
                                                                     << " from itself");
                    } ଅନ୍ୟଥା ଯଦି (extendsClass(baseClassp, nodep)) {
                        cextp->v3error("Attempting to extend class "
                                       << nodep->prettyNameQ() << " from "
                                       << baseClassp->prettyNameQ()
                                       << ", which circularly inherits from it");
                    } ଅନ୍ୟଥା ଯଦି (cextp->isImplements() && !baseClassp->isInterfaceClass()) {
                        cextp->v3error("Attempting to implement from non-interface class "
                                       << baseClassp->prettyNameQ() << '\n'
                                       << "... Suggest use 'extends'");
                    } ଅନ୍ୟଥା ଯଦି (!cextp->isImplements() && !nodep->isInterfaceClass()
                               && baseClassp->isInterfaceClass()) {
                        cextp->v3error("Attempting to extend from interface class "
                                       << baseClassp->prettyNameQ() << '\n'
                                       << "... Suggest use 'implements'");
                    }
                    baseClassp->isExtended(ସତ୍ୟ);
                    nodep->isExtended(ସତ୍ୟ);
                    iterate(baseClassp);
                    importSymbolsFromExtended(nodep, cextp);
                    ଜାରି;
                }
            }
            m_ds.m_dotSymp = m_curSymp;

            iterateChildren(nodep);
            ପାଇଁ (ଅଚଳ auto& itr : m_ifForwards) {
                ଅଚଳ VSymEnt* ଅଚଳ implSymp = itr.second;
                addInterfaceForward(nodep, VN_AS(implSymp->nodep(), ଗଣ୍ଠି_FTask),
                                    VN_AS(implSymp->parentp()->nodep(), Class));
            }
        }
        // V3Width when determines types needs to find enum values and such
        // so add members pointing to appropriate enum values
        {
            VMemberMap memberMap;
            ପାଇଁ (VSymEnt::const_iterator it = m_curSymp->begin(); it != m_curSymp->end(); ++it) {
                Ast_ଗଣ୍ଠି* ଅଚଳ itemp = it->second->nodep();
                ଯଦି (!memberMap.findMember(nodep, it->first)) {
                    ଯଦି (AstEnumItem* ଅଚଳ aitemp = VN_CAST(itemp, EnumItem)) {
                        AstEnumItemRef* ଅଚଳ newp = ନୂତନ AstEnumItemRef{
                            aitemp->fileline(), aitemp, it->second->classOrPackagep()};
                        UINFO(8, indent()
                                     << "Class import noderef '" << it->first << "' " << newp);
                        nodep->addMembersp(newp);
                        memberMap.ଭର୍ତ୍ତି(nodep, newp);
                    }
                }
            }
        }
        m_ds.m_dotSymp = VL_RESTORER_PREV(m_curSymp);
    }

    // Collect the ClassOrPackageRefs in a chained scope operand in source order.
    // The grammar builds a scope operand only from packageClassScopeItem, which
    // always yields a ClassOrPackageRef, nested in Dots, so nothing else appears.
    ସ୍ଥିର ଶୂନ୍ୟ collectScopeRefs(Ast_ଗଣ୍ଠି* nodep, std::vector<AstClassOrPackageRef*>& refps) {
        ଯଦି (AstClassOrPackageRef* ଅଚଳ refp = VN_CAST(nodep, ClassOrPackageRef)) {
            refps.ଠେଲ_back(refp);
            ଫେରନ୍ତୁ;
        }
        AstDot* ଅଚଳ dotp = VN_CAST(nodep, Dot);
        UASSERT_OBJ(dotp, nodep, "Non-scope node in package/class scope operand");
        collectScopeRefs(dotp->lhsp(), refps);
        collectScopeRefs(dotp->rhsp(), refps);
    }

    // Resolve a chained class-scope operand (`pkg::outer::inner` in
    // `pkg::outer::inner::t`) and reduce it to its innermost ClassOrPackageRef.
    // Returns false, having reported, if a segment could not be resolved.
    ତାର୍କିକ reduceScopeDot(AstRefDType* nodep, AstDot* scopeDotp) {
        std::vector<AstClassOrPackageRef*> refps;
        collectScopeRefs(scopeDotp, refps);

        VSymEnt* scopeSymp = m_ds.m_dotSymp;
        ପାଇଁ (size_t i = 0; i < refps.size(); ++i) {
            AstClassOrPackageRef* ଅଚଳ refp = refps[i];
            ଯଦି (!refp->classOrPackageSkipp() && !refp->classOrPackageNodep()
                && !m_statep->resolveClassOrPackage(scopeSymp, refp, i == 0, ମିଥ୍ୟା,
                                                    "class/package reference")) {
                ଫେରନ୍ତୁ ମିଥ୍ୟା;  // Error already reported
            }
            ଯଦି (i + 1 == refps.size()) ଭାଙ୍ଗ;
            // A middle segment that resolved to a node with no reachable module
            // (e.g. a typedef alias of a not-yet-specialized parameterized class)
            // gives nothing to look the next segment up in.
            Ast_ଗଣ୍ଠି_Module* ଅଚଳ modp = refp->classOrPackageSkipp();
            ଯଦି (!modp) {
                refp->v3warn(E_UNSUPPORTED, "Unsupported: Multiple '::' package/class reference");
                ଫେରନ୍ତୁ ମିଥ୍ୟା;
            }
            scopeSymp = m_statep->get_ଗଣ୍ଠି_Sym(modp);
        }
        AstClassOrPackageRef* ଅଚଳ innerp = refps.back();
        innerp->unlinkFrBack();
        VL_DO_DANGLING(ଠେଲ_Deletep(scopeDotp->unlinkFrBack()), scopeDotp);
        nodep->classOrPackageOpp(innerp);
        ଫେରନ୍ତୁ ସତ୍ୟ;
    }

    ଶୂନ୍ୟ visit(AstRefDType* nodep) override {

        ଯଦି (auto* ଅଚଳ typeOfp = nodep->typeofp()) {
            iterate(typeOfp);
            // After iteration, typeofp() may have been replaced with a resolved type
            // (e.g., DOT expression like if0.rq_t resolved to REFDTYPE)
            // If it's now a NodeDType, replace this RefDType with it
            ଯଦି (Ast_ଗଣ୍ଠି_DType* ଅଚଳ resolvedDTypep = VN_CAST(nodep->typeofp(), ଗଣ୍ଠି_DType)) {
                UINFO(9,
                      indent() << "iface capture typeofp resolved to dtype, replacing RefDType\n");
                resolvedDTypep->unlinkFrBack();
                nodep->replaceWith(resolvedDTypep);
                VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
                // Originally (PR #6637) this block re-captured a resolved
                // RefDType into the iface-capture ledger when user2p held a
                // Cell, to ensure re-resolution during the paramed pass.
                // In practice, the typeofp-resolved RefDType never carries a
                // Cell in user2p: the capture happens earlier when the
                // typedef is first linked.
                ଯଦି (V3LinkDotIfaceCapture::enabled()) {
                    ଯଦି (AstRefDType* ଅଚଳ resolvedRefp = VN_CAST(resolvedDTypep, RefDType)) {
                        ଯଦି (VL_UNCOVERABLE(
                                VN_IS(resolvedRefp->user2p(), Cell))) {  // LCOV_EXCL_LINE
                            resolvedRefp->v3fatalSrc(  // LCOV_EXCL_LINE
                                "typeofp resolved RefDType has Cell in user2p;"
                                " expected to be captured already");
                        }
                    }
                }
            }
            ଫେରନ୍ତୁ;
        }

        // Resolve its reference
        ଯଦି (m_statep->forParamed() && nodep->user3()) {
            ଯଦି (V3LinkDotIfaceCapture::enabled() && nodep->user2p()) {
                UINFO(9, indent() << "iface capture clear user3 for captured typedef name="
                                  << nodep->name() << " cell=" << nodep->user2p());
            }
            nodep->user3(ମିଥ୍ୟା);
        }
        ଯଦି (nodep->user3SetOnce()) {
            ଯଦି (V3LinkDotIfaceCapture::enabled() && nodep->user2p()) {
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
            ଯଦି (VL_UNCOVERABLE(m_statep->forParamed() && nodep->classOrPackageOpp())) {
                nodep->v3fatalSrc(  // LCOV_EXCL_LINE
                    "RefDType user3 revisit with classOrPackageOpp in paramed pass");
            }
            ଫେରନ୍ତୁ;
        }
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        ଯଦି (nodep->classOrPackageOpp()) {
            // Resolve a chained scope (`pkg::cls::t`, `pkg::cls#(P)::t`) from outside in.
            // On failure reduceScopeDot has reported, so stop before using the operand.
            ଯଦି (AstDot* ଅଚଳ scopeDotp = VN_CAST(nodep->classOrPackageOpp(), Dot)) {
                ଯଦି (!reduceScopeDot(nodep, scopeDotp)) ଫେରନ୍ତୁ;
            }
            // Re-read: reduceScopeDot may have replaced the operand
            Ast_ଗଣ୍ଠି* ଅଚଳ cpackagep = nodep->classOrPackageOpp();
            UASSERT_OBJ(!VN_IS(cpackagep, Dot), cpackagep,
                        "Package/class scope operand should be reduced by now");
            ଯଦି (AstClassOrPackageRef* ଅଚଳ cpackagerefp = VN_CAST(cpackagep, ClassOrPackageRef)) {
                iterate(cpackagerefp);
                ଅଚଳ AstClass* ଅଚଳ clsp = VN_CAST(cpackagerefp->classOrPackageNodep(), Class);
                ଯଦି (clsp && clsp->hasGParam()) {
                    // Unable to link before the instantiation of parameter classes.
                    // The class reference node still has to be visited now to later link
                    // parameters.
                    iterate(cpackagep);
                    ଫେରନ୍ତୁ;
                }
                // Defer non-typedef references through typedef aliases of parameterized classes.
                ଯଦି (m_statep->forPrimary() && !VN_IS(nodep->backp(), Typedef)
                    && isParamedClassRef(cpackagerefp)) {
                    iterate(cpackagep);
                    ଫେରନ୍ତୁ;
                }

                ଅଚଳ ତାର୍କିକ doDefaultTypedef = !(m_resolvingTypedef && m_statep->forPrimary());
                ଯଦି (!cpackagerefp->classOrPackageSkipp(doDefaultTypedef)
                    && !cpackagerefp->classOrPackageNodep()) {
                    VSymEnt* ଅଚଳ foundp = m_statep->resolveClassOrPackage(
                        m_ds.m_dotSymp, cpackagerefp, ସତ୍ୟ, ମିଥ୍ୟା, "class/package reference");
                    ଯଦି (!foundp) ଫେରନ୍ତୁ;
                }
                nodep->classOrPackagep(cpackagerefp->classOrPackageSkipp(doDefaultTypedef));
                ଯଦି (!VN_IS(nodep->classOrPackagep(), Class)
                    && !VN_IS(nodep->classOrPackagep(), Package)) {
                    ଯଦି (m_statep->forPrimary()) {
                        // It may be a type that comes from parameter class that is not
                        // instantioned yet
                        iterate(cpackagep);
                        ଫେରନ୍ତୁ;
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
            VL_DO_DANGLING(ଠେଲ_Deletep(cpackagep->unlinkFrBack()), cpackagep);
        }

        ଯଦି (m_ds.m_dotp && (m_ds.m_dotPos == DP_PACKAGE || m_ds.m_dotPos == DP_SCOPE)) {
            UASSERT_OBJ(VN_IS(m_ds.m_dotp->lhsp(), ClassOrPackageRef), m_ds.m_dotp->lhsp(),
                        "Bad package link");
            auto* ଅଚଳ cpackagerefp = VN_CAST(m_ds.m_dotp->lhsp(), ClassOrPackageRef);
            UASSERT_OBJ(cpackagerefp->classOrPackageSkipp(), m_ds.m_dotp->lhsp(),
                        "Bad package link");
            nodep->classOrPackagep(cpackagerefp->classOrPackageSkipp());
            m_ds.m_dotPos = DP_SCOPE;
        } ଅନ୍ୟଥା {
            // Allow REFDTYPE under DOT when referencing interface typedefs
            // This is needed for patterns like: typedef iface.a_t a_t;
            // The dependency graph handles resolution when IfaceCapture is disabled
            ଅଚଳ ତାର୍କିକ ifaceFinalSegmentAllowed
                = (m_ds.m_dotPos == DP_FINAL) && m_ds.m_dotSymp
                  && VN_IS(m_ds.m_dotSymp->nodep(), Cell)
                  && VN_CAST(m_ds.m_dotSymp->nodep(), Cell)->modp()
                  && VN_IS(VN_CAST(m_ds.m_dotSymp->nodep(), Cell)->modp(), Iface);
            ଯଦି (!ifaceFinalSegmentAllowed) {
                checkNoDot(nodep);
            } ଅନ୍ୟଥା {
                UINFO(9, indent() << "allow REFDTYPE under DOT for iface typedef name="
                                  << nodep->prettyNameQ() << " dotSym=" << m_ds.m_dotSymp);
                // Clear the dot state so we don't propagate errors
                m_ds.m_dotPos = DP_SCOPE;
            }
        }
        ଯଦି (!nodep->typedefp() && !nodep->subDTypep()) {
            ଅଚଳ VSymEnt* foundp;
            ଯଦି (nodep->classOrPackagep()) {
                foundp = m_statep->get_ଗଣ୍ଠି_Sym(nodep->classOrPackagep())->findIdFlat(nodep->name());
            } ଅନ୍ୟଥା ଯଦି (m_ds.m_dotPos == DP_FIRST || m_ds.m_dotPos == DP_NONE) {
                foundp = findIdFallbackSkipMemberDType(m_curSymp, nodep->name());
            } ଅନ୍ୟଥା {
                // Defensive: dotPos should be DP_FIRST/DP_NONE or classOrPackagep set.
                v3fatalSrc("Unexpected dotPos="
                           << static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(m_ds.m_dotPos)  // LCOV_EXCL_LINE
                           << " in RefDType lookup for "
                           << nodep->prettyNameQ());  // LCOV_EXCL_LINE
                foundp = nullptr;  // LCOV_EXCL_LINE
            }
            {
                ଯଦି (AstTypedef* ଅଚଳ defp
                    = foundp ? VN_CAST(foundp->nodep(), Typedef) : nullptr) {
                    // Don't check if typedef is to a <type T>::<reference> as might not be
                    // resolved yet
                    ଯଦି (!nodep->classOrPackagep() && !defp->isUnderClass())
                        checkDeclOrder(nodep, defp);
                    nodep->typedefp(defp);
                    nodep->classOrPackagep(foundp->classOrPackagep());

                } ଅନ୍ୟଥା ଯଦି (AstParamTypeDType* ଅଚଳ defp
                           = foundp ? VN_CAST(foundp->nodep(), ParamTypeDType) : nullptr) {
                    // A PARAMTYPEDTYPE's child REFDTYPE referencing itself is the normal
                    // AST structure for type parameters (e.g., "type T = base" has a child
                    // REFDTYPE for the default type). Only error on true recursion where
                    // a TYPEDEF contains a reference back to itself.
                    ଅଚଳ ତାର୍କିକ isParamTypeChild = (defp == nodep->backp());
                    ଅଚଳ ତାର୍କିକ isTypedefRecursion
                        = isParamTypeChild && VN_IS(defp->backp(), Typedef);
                    ଯଦି (isTypedefRecursion) {
                        nodep->v3error("Reference to '"
                                       << m_ds.m_dotText << (m_ds.m_dotText == "" ? "" : ".")
                                       << nodep->prettyName() << "'"
                                       << " type would form a recursive definition");
                        nodep->refDTypep(nodep->findVoidDType());  // Try to reduce later errors
                    } ଅନ୍ୟଥା {
                        nodep->refDTypep(defp);
                        nodep->classOrPackagep(foundp->classOrPackagep());
                        captureIfaceParamType(nodep, defp);
                    }
                } ଅନ୍ୟଥା ଯଦି (AstClass* ଅଚଳ defp
                           = foundp ? VN_CAST(foundp->nodep(), Class) : nullptr) {
                    // Don't check if typedef is to a <type T>::<reference> as might not be
                    // resolved yet
                    ଯଦି (!nodep->classOrPackagep()) checkDeclOrder(nodep, defp);
                    AstPin* ଅଚଳ paramsp = nodep->paramsp();
                    ଯଦି (paramsp) paramsp->unlinkFrBackWithNext();
                    AstClassRefDType* ଅଚଳ newp
                        = ନୂତନ AstClassRefDType{nodep->fileline(), defp, paramsp};
                    newp->classOrPackagep(foundp->classOrPackagep());
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
                    ଫେରନ୍ତୁ;
                } ଅନ୍ୟଥା ଯଦି (m_insideClassExtParam) {
                    ଫେରନ୍ତୁ;
                } ଅନ୍ୟଥା {
                    ଯଦି (foundp) {
                        UINFO(1, "Found sym node: " << foundp->nodep());
                        nodep->v3error("Expecting a data type: "
                                       << nodep->prettyNameQ()
                                       << ", found: " << foundp->nodep()->prettyTypeName());
                    } ଅନ୍ୟଥା {
                        nodep->v3error("Can't find typedef/interface: " << nodep->prettyNameQ());
                    }
                }
            }
        }
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstRequireDType* nodep) override {
        // Handle type validation for localparam type assignments
        // Iterate child to resolve any ParseRef nodes
        iterateChildren(nodep);
        // Only emit error if the child is not a type.
        // Do NOT unwrap valid types here - leave that to V3Width.
        // Unwrapping here breaks type parameter resolution during cloning.
        // Allow Dot through to defer the handling until the dot expression is resolved.
        ଯଦି (nodep->lhsp() && !VN_IS(nodep->lhsp(), ଗଣ୍ଠି_DType) && !VN_IS(nodep->lhsp(), Dot)) {
            // Not a type - emit error
            ଯଦି (VN_IS(nodep->lhsp(), Const)) {
                nodep->lhsp()->v3error("Expecting a data type, not a constant");
            } ଅନ୍ୟଥା {
                nodep->lhsp()->v3error("Expecting a data type, not "
                                       << nodep->lhsp()->typeName() << ": '"
                                       << nodep->lhsp()->prettyName() << "'");
            }
            // Replace with void to keep AST valid and allow clean exit
            nodep->replaceWith(ନୂତନ AstVoidDType{nodep->fileline()});
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
        }
    }
    ଶୂନ୍ୟ visit(AstDpiExport* nodep) override {
        // AstDpiExport: Make sure the function referenced exists, then dump it
        LINKDOT_VISIT_START();
        iterateChildren(nodep);
        checkNoDot(nodep);
        VSymEnt* ଅଚଳ foundp = m_curSymp->findIdFallback(nodep->name());
        Ast_ଗଣ୍ଠି_FTask* ଅଚଳ taskp = foundp ? VN_AS(foundp->nodep(), ଗଣ୍ଠି_FTask) : nullptr;
        ଯଦି (!taskp) {
            nodep->v3error(
                "Can't find definition of exported task/function: " << nodep->prettyNameQ());
        } ଅନ୍ୟଥା ଯଦି (taskp->dpiExport()) {
            nodep->v3error("Function was already DPI Exported, duplicate not allowed: "
                           << nodep->prettyNameQ());
        } ଅନ୍ୟଥା {
            taskp->dpiExport(ସତ୍ୟ);
            ଯଦି (nodep->cname() != "") taskp->cname(nodep->cname());
        }
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep->unlinkFrBack()), nodep);
    }
    ଶୂନ୍ୟ visit(AstDisable* nodep) override {
        LINKDOT_VISIT_START();
        checkNoDot(nodep);
        ତାର୍କିକ hasPartSelect = ମିଥ୍ୟା;
        ଅଚଳ string targetPath
            = nodep->targetRefp() ? extractDottedPath(nodep->targetRefp(), hasPartSelect) : "";
        VL_RESTORER_COPY(m_ds);
        m_ds.init(m_curSymp);
        m_ds.m_dotPos = DP_FIRST;
        m_ds.m_disablep = nodep;
        iterateChildren(nodep);
        ଯଦି (nodep->targetRefp()) {
            ଯଦି (AstTaskRef* ଅଚଳ taskRefp = VN_CAST(nodep->targetRefp(), TaskRef)) {
                nodep->targetp(taskRefp->taskp());
            } ଅନ୍ୟଥା ଯଦି (!VN_IS(nodep->targetRefp(), ParseRef)) {
                // If it is a ParseRef, either it couldn't be linked or it is linked to a block
                nodep->v3warn(E_UNSUPPORTED, "Node of type "
                                                 << nodep->targetRefp()->prettyTypeName()
                                                 << " referenced by disable");
                VL_DO_DANGLING(ଠେଲ_Deletep(nodep->unlinkFrBack()), nodep);
                ଫେରନ୍ତୁ;
            }
            ଯଦି (nodep->targetp()) {
                nodep->targetRefp()->unlinkFrBack()->delete_ବୃକ୍ଷ();
                ଯଦି (!hasPartSelect) {
                    ଅଚଳ string instancePath = targetInstancePath(nodep->targetp(), targetPath);
                    ଯଦି (!instancePath.empty()) {
                        // Keep only the instance prefix so V3LinkJump can reference the selected
                        // instance's process queue without reparsing the disable target.
                        nodep->targetRefp(
                            ନୂତନ AstVarXRef{nodep->fileline(), "", instancePath, VAccess::READ});
                    }
                }
            }
        }
    }
    ଶୂନ୍ୟ visitPackageImportOrExport(Ast_ଗଣ୍ଠି* nodep) {
        // No longer needed
        LINKDOT_VISIT_START();
        checkNoDot(nodep);
        ଯଦି (m_statep->forParamed()) VL_DO_DANGLING(ଠେଲ_Deletep(nodep->unlinkFrBack()), nodep);
    }
    ଶୂନ୍ୟ visit(AstPackageImport* nodep) override { visitPackageImportOrExport(nodep); }
    ଶୂନ୍ୟ visit(AstPackageExport* nodep) override { visitPackageImportOrExport(nodep); }
    ଶୂନ୍ୟ visit(AstPackageExportStarStar* nodep) override { visitPackageImportOrExport(nodep); }
    ଶୂନ୍ୟ visit(AstCellRef* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstCellArrayRef* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        // Expression already iterated
    }
    ଶୂନ୍ୟ visit(AstUnlinkedRef* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        // No need to iterate, if we have a UnlinkedVarXRef, we're already done
    }
    ଶୂନ୍ୟ visit(Ast_Stmt_ପଦସମଷ୍ଟି* nodep) override {
        LINKDOT_VISIT_START();
        checkNoDot(nodep);
        // Check if nodep represents a super.new call;
        ଯଦି (AstNew* ଅଚଳ newExprp = VN_CAST(nodep->exprp(), New)) {
            // in this case it was already linked, so it doesn't have a super reference
            m_explicitSuperNewp = newExprp;
        } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstDot* ଅଚଳ dotp = VN_CAST(nodep->exprp(), Dot)) {
            ଯଦି (dotp->lhsp()->name() == "super") {
                ଯଦି (AstNew* ଅଚଳ newExprp = VN_CAST(dotp->rhsp(), New)) {
                    m_explicitSuperNewp = newExprp;
                }
            }
        }
        iterateChildren(nodep);
    }

    ଶୂନ୍ୟ visit(AstIfaceRefDType* nodep) override {
        ଯଦି (nodep->user3SetOnce()) ଫେରନ୍ତୁ;
        LINKDOT_VISIT_START();
        ଯଦି (nodep->paramsp()) {
            // If there is no parameters, there is no need to visit this node.
            AstIface* ଅଚଳ ifacep = nodep->ifacep();
            UASSERT_OBJ(ifacep, nodep, "Port parameters of AstIfaceRefDType without ifacep()");
            ଯଦି (ifacep->dead()) ଫେରନ୍ତୁ;
            checkNoDot(nodep);
            VL_RESTORER_CLEAR(m_usedPins);
            VL_RESTORER_CLEAR(m_usedDefParamPins);
            VL_RESTORER(m_pinSymp);
            m_pinSymp = m_statep->get_ଗଣ୍ଠି_Sym(ifacep);
            iterateAndNextNull(nodep->paramsp());
        }
    }

    ଶୂନ୍ୟ visit(AstAttrOf* nodep) override { iterateChildren(nodep); }
    ଶୂନ୍ୟ visit(AstCastParse* nodep) override {
        ଯଦି (nodep->user3SetOnce()) ଫେରନ୍ତୁ;
        LINKDOT_VISIT_START();
        // A cast may start a dotted expression: HERE type'(expr) . member_or_method
        ଅଚଳ ତାର୍କିକ dotLhs = m_ds.m_dotPos == DP_FIRST;
        ଯଦି (!dotLhs) checkNoDot(nodep);
        {
            VL_RESTORER_COPY(m_ds);
            m_ds.init(m_curSymp);
            iterateChildren(nodep);
        }
        ଯଦି (dotLhs) m_ds.m_dotPos = DP_MEMBER;
    }

    ଶୂନ୍ୟ visit(AstAssignW* nodep) override {
        LINKDOT_VISIT_START();
        checkNoDot(nodep);
        VL_RESTORER(m_replaceWithAlias);
        ଯଦି (nodep->user2()) m_replaceWithAlias = ମିଥ୍ୟା;
        iterateChildren(nodep);
    }

    ଶୂନ୍ୟ visit(AstTypedef* nodep) override {
        VL_RESTORER(m_resolvingTypedef)
        m_resolvingTypedef = ସତ୍ୟ;
        iterateChildren(nodep);
    }

    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override {
        VL_RESTORER(m_inPackedArray);
        ଯଦି (VN_IS(nodep, PackArrayDType)) {
            m_inPackedArray = ସତ୍ୟ;
        } ଅନ୍ୟଥା ଯଦି (!m_inPackedArray) {
            LINKDOT_VISIT_START();
            checkNoDot(nodep);
        }
        iterateChildren(nodep);
    }

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    LinkDotResolveVisitor(AstNetlist* rootp, LinkDotState* statep)
        : m_statep{statep} {
        UINFO(4, __FUNCTION__ << ": ");

        // Note: no need to clear user3 on ledger entries here.
        // VNUser3InUse (m_inuser3) already logically clears user3 on all
        // nodes via the generation counter.  The ledger may also hold stale
        // refp pointers to deleted AST nodes, so iterating it is unsafe.

        iterate(rootp);
        ଅଚଳ std::map<std::string, Ast_ଗଣ୍ଠି_Module*> modulesToRevisit
            = std::move(m_modulesToRevisit);
        m_lastDeferredp = nullptr;
        ପାଇଁ (auto& p : modulesToRevisit) {
            Ast_ଗଣ୍ଠି_Module* ଅଚଳ modp = p.second;
            modp->foreach([](Ast_ଗଣ୍ଠି* ଅଚଳ nodep) { nodep->user3(ମିଥ୍ୟା); });
            iterate(modp);
        }
    }
    ~LinkDotResolveVisitor() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
};

//######################################################################
// Link class functions

ଶୂନ୍ୟ V3LinkDot::dumpSubstep(ଅଚଳ string& name) {
    ଯଦି (dump_ବୃକ୍ଷ_Either_Level() >= 9) {
        V3Global::dump_Check_Global_ବୃକ୍ଷ(name);
    } ଅନ୍ୟଥା ଯଦି (debug() >= 5) {  // on high dbg level, dump even if not explicitly told to
        v3Global.rootp()->dump_ବୃକ୍ଷ_File(v3Global.debugFilename(name + ".tree"));
    }
}

ଶୂନ୍ୟ V3LinkDot::linkDotGuts(AstNetlist* rootp, VLinkDotStep step) {
    VIsCached::clear_Cache_ବୃକ୍ଷ();  // Avoid using any stale isPure
    dumpSubstep("prelinkdot");
    LinkDotState state{rootp, step};

    { LinkDotFindVisitor{rootp, &state}; }
    dumpSubstep("prelinkdot-find");

    { LinkDotFindIfaceVisitor{rootp, &state}; }
    dumpSubstep("prelinkdot-findiface");

    ଯଦି (step == LDS_SCOPED) {
        // Well after the initial link when we're ready to operate on the flat design,
        // process AstScope's.  This needs to be separate pass after whole hierarchy graph created.
        { LinkDotScopeVisitor{rootp, &state}; }
        v3Global.assertScoped(ସତ୍ୟ);
        dumpSubstep("prelinkdot-scoped");
    } ଅନ୍ୟଥା {
        // Initial link stage, resolve parameters and interfaces
        { LinkDotParamVisitor{rootp, &state}; }
        dumpSubstep("prelinkdot-param");
    }
    state.dumpSelf("prelinkdot");
    state.computeIfaceModSyms();
    state.computeIfaceVarSyms();
    state.computeScopeAliases();
    state.dumpSelf("linkdot-preresolve");
    ଅଚଳ LinkDotResolveVisitor visitor{rootp, &state};
    state.dumpSelf("linkdot-done");
}

ଶୂନ୍ୟ V3LinkDot::linkDotPrimary(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    linkDotGuts(nodep, LDS_PRIMARY);
    V3Global::dump_Check_Global_ବୃକ୍ଷ("linkdot", 0, dump_ବୃକ୍ଷ_Either_Level() >= 6);
}

ଶୂନ୍ୟ V3LinkDot::linkDotParamed(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    linkDotGuts(nodep, LDS_PARAMED);
    V3Global::dump_Check_Global_ବୃକ୍ଷ("linkdotparam", 0, dump_ବୃକ୍ଷ_Either_Level() >= 3);
}

ଶୂନ୍ୟ V3LinkDot::linkDotScope(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    linkDotGuts(nodep, LDS_SCOPED);
    V3Global::dump_Check_Global_ବୃକ୍ଷ("linkdot", 0, dump_ବୃକ୍ଷ_Either_Level() >= 3);
}
