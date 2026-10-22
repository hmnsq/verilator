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
//ತುದಿ
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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3LinkDot.h"

#ಅನ್ತರ್ಭಾವ "V3Global.h"
#ಅನ್ತರ್ಭಾವ "V3Graph.h"
#ಅನ್ತರ್ಭಾವ "V3LinkDotIfaceCapture.h"
#ಅನ್ತರ್ಭಾವ "V3MemberMap.h"
#ಅನ್ತರ್ಭಾವ "V3Parse.h"
#ಅನ್ತರ್ಭಾವ "V3Randomize.h"
#ಅನ್ತರ್ಭಾವ "V3Stats.h"
#ಅನ್ತರ್ಭಾವ "V3String.h"
#ಅನ್ತರ್ಭಾವ "V3SymTable.h"

#ಅನ್ತರ್ಭಾವ <algorithm>
#ಅನ್ತರ್ಭಾವ <cstdlib>
#ಅನ್ತರ್ಭಾವ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

ಸ್ಥಿರ string extractDottedPath(Ast_ಗಂಟು* nodep, ತಾರ್ಕಿಕ& hasPartSelect) {
    ಯದಿ (AstParseRef* ಧ್ರುವ refp = VN_CAST(nodep, ParseRef)) {
        ಪ್ರತ್ಯಾಗಮ refp->name();
    } ಅನ್ಯಥಾ ಯದಿ (AstVarRef* ಧ್ರುವ refp = VN_CAST(nodep, VarRef)) {
        ಪ್ರತ್ಯಾಗಮ refp->name();
    } ಅನ್ಯಥಾ ಯದಿ (AstDot* ಧ್ರುವ dotp = VN_CAST(nodep, Dot)) {
        ಧ್ರುವ string lhs = extractDottedPath(dotp->lhsp(), hasPartSelect);
        ಧ್ರುವ string rhs = extractDottedPath(dotp->rhsp(), hasPartSelect);
        ಪ್ರತ್ಯಾಗಮ VString::dot(lhs, ".", rhs);
    } ಅನ್ಯಥಾ ಯದಿ (VN_IS(nodep, SelBit) || VN_IS(nodep, SelExtract) || VN_IS(nodep, SelPlus)
               || VN_IS(nodep, SelMinus)) {
        hasPartSelect = ಸತ್ಯ;
        ಪ್ರತ್ಯಾಗಮ "";
    }
    ಪ್ರತ್ಯಾಗಮ "";
}
ಸ್ಥಿರ string lexicalDisablePath(Ast_ಗಂಟು* ಧ್ರುವ targetp) {
    std::vector<string> names;
    ಪ್ರತಿ (Ast_ಗಂಟು* curp = targetp; curp; curp = curp->aboveLoopp()) {
        ಯದಿ (Ast_ಗಂಟು_Block* ಧ್ರುವ blockp = VN_CAST(curp, ಗಂಟು_Block)) {
            ಯದಿ (blockp->name() != "") names.ತಳ್ಳು_back(blockp->name());
        } ಅನ್ಯಥಾ ಯದಿ (Ast_ಗಂಟು_FTask* ಧ್ರುವ ftaskp = VN_CAST(curp, ಗಂಟು_FTask)) {
            names.ತಳ್ಳು_back(ftaskp->name());
        } ಅನ್ಯಥಾ ಯದಿ (VN_IS(curp, ಗಂಟು_Module)) {
            ಭಙ್ಗ;
        }
    }
    string path;
    ಪ್ರತಿ (auto it = names.crbegin(); it != names.crend(); ++it) {
        path = VString::dot(path, ".", *it);
    }
    ಪ್ರತ್ಯಾಗಮ path;
}
ಸ್ಥಿರ string targetInstancePath(Ast_ಗಂಟು* ಧ್ರುವ targetp, ಧ್ರುವ string& targetPath) {
    ಧ್ರುವ string lexicalPath = lexicalDisablePath(targetp);
    ಯದಿ (lexicalPath == "" || targetPath == lexicalPath) ಪ್ರತ್ಯಾಗಮ "";
    ಧ್ರುವ string suffix = "." + lexicalPath;
    ಯದಿ (!VString::endsWith(targetPath, suffix)) ಪ್ರತ್ಯಾಗಮ "";
    ಪ್ರತ್ಯಾಗಮ targetPath.substr(0, targetPath.size() - suffix.size());
}

// ######################################################################
//  Matcher classes (for suggestion matching)

ವರ್ಗ Link_ಗಂಟು_Matcher_Class_Or_Package final : ಸಾರ್ವಜನಿಕ VNodeMatcher {
ಸಾರ್ವಜನಿಕ:
    ತಾರ್ಕಿಕ ಗಂಟು_Match(ಧ್ರುವ Ast_ಗಂಟು* nodep) ಧ್ರುವ override {
        ಪ್ರತ್ಯಾಗಮ VN_IS(nodep, Class) || VN_IS(nodep, Package);
    }
};
ವರ್ಗ Link_ಗಂಟು_Matcher_FTask final : ಸಾರ್ವಜನಿಕ VNodeMatcher {
ಸಾರ್ವಜನಿಕ:
    ತಾರ್ಕಿಕ ಗಂಟು_Match(ಧ್ರುವ Ast_ಗಂಟು* nodep) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ VN_IS(nodep, ಗಂಟು_FTask); }
};
ವರ್ಗ Link_ಗಂಟು_Matcher_Modport final : ಸಾರ್ವಜನಿಕ VNodeMatcher {
ಸಾರ್ವಜನಿಕ:
    ತಾರ್ಕಿಕ ಗಂಟು_Match(ಧ್ರುವ Ast_ಗಂಟು* nodep) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ VN_IS(nodep, Modport); }
};
ವರ್ಗ Link_ಗಂಟು_Matcher_Prod final : ಸಾರ್ವಜನಿಕ VNodeMatcher {
ಸಾರ್ವಜನಿಕ:
    ತಾರ್ಕಿಕ ಗಂಟು_Match(ಧ್ರುವ Ast_ಗಂಟು* nodep) ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ VN_IS(nodep, RSProd); }
};
ವರ್ಗ Link_ಗಂಟು_Matcher_Var final : ಸಾರ್ವಜನಿಕ VNodeMatcher {
ಸಾರ್ವಜನಿಕ:
    ತಾರ್ಕಿಕ ಗಂಟು_Match(ಧ್ರುವ Ast_ಗಂಟು* nodep) ಧ್ರುವ override {
        ಪ್ರತ್ಯಾಗಮ VN_IS(nodep, Var) || VN_IS(nodep, LambdaArgRef);
    }
};
ವರ್ಗ Link_ಗಂಟು_Matcher_Var_IO final : ಸಾರ್ವಜನಿಕ VNodeMatcher {
ಸಾರ್ವಜನಿಕ:
    ತಾರ್ಕಿಕ ಗಂಟು_Match(ಧ್ರುವ Ast_ಗಂಟು* nodep) ಧ್ರುವ override {
        ಧ್ರುವ AstVar* ಧ್ರುವ varp = VN_CAST(nodep, Var);
        ಯದಿ (!varp) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಪ್ರತ್ಯಾಗಮ varp->isIO();
    }
};
ವರ್ಗ Link_ಗಂಟು_Matcher_Var_Param final : ಸಾರ್ವಜನಿಕ VNodeMatcher {
ಸಾರ್ವಜನಿಕ:
    ತಾರ್ಕಿಕ ಗಂಟು_Match(ಧ್ರುವ Ast_ಗಂಟು* nodep) ಧ್ರುವ override {
        ಧ್ರುವ AstVar* ಧ್ರುವ varp = VN_CAST(nodep, Var);
        ಯದಿ (!varp) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಪ್ರತ್ಯಾಗಮ varp->isParam();
    }
};

//######################################################################
// LinkDot state, as a visitor of each AstNode

ವರ್ಗ LinkDotState final {
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
    ಧ್ರುವ VNUser1InUse m_inuser1;
    ಧ್ರುವ VNUser2InUse m_inuser2;
    ಧ್ರುವ VNUser4InUse m_inuser4;

ಸಾರ್ವಜನಿಕ:
    // ENUMS
    // In order of priority, compute first ... compute last
    ಗಣನಾ SAMNum : uint8_t { SAMN_MODPORT, SAMN_IFTOP, SAMN__MAX };  // Values for m_scopeAliasMap

ಗುಪ್ತ:
    // TYPES
    using ScopeAliasMap = std::unordered_map<VSymEnt*, VSymEnt*>;
    using IfaceModSyms = std::vector<std::pair<AstIface*, VSymEnt*>>;

    ಸ್ಥಿರ LinkDotState* s_errorThisp;  // Last self, for error reporting only

    // MEMBERS
    VSymGraph m_syms;  // Symbol table by hierarchy
    VSymGraph m_mods;  // Symbol table of all module names
    VSymEnt* m_dunitEntp = nullptr;  // $unit entry
    std::multimap<std::string, VSymEnt*>
        m_nameScopeSymMap;  // Map of scope referenced by non-pretty textual name
    std::set<std::pair<Ast_ಗಂಟು_Module*, std::string>>
        m_implicitNameSet;  // For [module][signalname] if we can implicitly create it
    std::array<ScopeAliasMap, SAMN__MAX> m_scopeAliasMap;  // Map of <lhs,rhs> aliases
    std::vector<VSymEnt*> m_ifaceVarSyms;  // List of AstIfaceRefDType's to be imported
    IfaceModSyms m_ifaceModSyms;  // List of AstIface+Symbols to be processed
    ಧ್ರುವ VLinkDotStep m_step;  // Operational step

ಸಾರ್ವಜನಿಕ:
    // METHODS

    ಶೂನ್ಯ dumpSelf(ಧ್ರುವ string& nameComment, ತಾರ್ಕಿಕ force = ಅಸತ್ಯ) {
        ಯದಿ (debug() >= 6 || dumpLevel() >= 6 || force) {
            ಧ್ರುವ string filename = v3Global.debugFilename(nameComment) + ".txt";
            UINFO(4, "Dumping " << filename);
            ಧ್ರುವ std::unique_ptr<std::ofstream> logp{V3File::new_ofstream(filename)};
            ಯದಿ (logp->fail()) v3fatal("Can't write file: " << filename);
            std::ostream& os = *logp;
            // TODO the symbol table has node pointers which may be
            // dangling, as we call deleteTree in these visitors without
            // pushing for later deletion (or deleting from the symbol table)
            // So, only completely safe to call this when under --debug or --debug-leak.
            m_syms.dumpSelf(os);
            ತಾರ್ಕಿಕ first = ಸತ್ಯ;
            ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ samn = 0; samn < SAMN__MAX; ++samn) {
                ಯದಿ (!m_scopeAliasMap[samn].empty()) {
                    ಯದಿ (first) os << "\nScopeAliasMap:\n";
                    first = ಅಸತ್ಯ;
                    ಪ್ರತಿ (ScopeAliasMap::iterator it = m_scopeAliasMap[samn].begin();
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
    ಸ್ಥಿರ ಶೂನ್ಯ preErrorDumpHandler() {
        ಯದಿ (s_errorThisp) s_errorThisp->preErrorDump();
    }
    ಶೂನ್ಯ preErrorDump() {
        ಸ್ಥಿರ ತಾರ್ಕಿಕ s_diddump = ಅಸತ್ಯ;
        ಯದಿ (!s_diddump && dump_ವೃಕ್ಷ_Level()) {
            s_diddump = ಸತ್ಯ;
            dumpSelf("linkdot-preerr", ಸತ್ಯ);
            v3Global.rootp()->dump_ವೃಕ್ಷ_File(v3Global.debugFilename("linkdot-preerr.tree"));
        }
    }

    // CONSTRUCTORS
    LinkDotState(AstNetlist* rootp, VLinkDotStep step)
        : m_syms{rootp}
        , m_mods{rootp}
        , m_step{step} {
        UINFO(4, __FUNCTION__ << ": ");
        s_errorThisp = ಏತತ್;
        V3Error::errorExitCb(preErrorDumpHandler);  // If get error, dump self
        readModNames();
    }
    ~LinkDotState() {
        V3Error::errorExitCb(nullptr);
        s_errorThisp = nullptr;
    }

    // ACCESSORS
    VSymGraph* symsp() { ಪ್ರತ್ಯಾಗಮ &m_syms; }
    ಪೂರ್ಣಾಙ್ಕ step_ಸಂಖ್ಯೆ() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ static_cast<ಪೂರ್ಣಾಙ್ಕ>(m_step); }
    ತಾರ್ಕಿಕ forPrimary() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_step == LDS_PRIMARY; }
    ತಾರ್ಕಿಕ forParamed() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_step == LDS_PARAMED; }
    ತಾರ್ಕಿಕ forScopeCreation() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_step == LDS_SCOPED; }

    // METHODS
    ಸ್ಥಿರ string ಗಂಟು_Text_Type(Ast_ಗಂಟು* nodep) {
        ಯದಿ (ಧ್ರುವ AstVar* ಧ್ರುವ varp = VN_CAST(nodep, Var)) {
            ಯದಿ (varp->isIO() || varp->isIfaceRef()) {
                ಪ್ರತ್ಯಾಗಮ "port";
            } ಅನ್ಯಥಾ ಯದಿ (varp->isGParam()) {
                ಪ್ರತ್ಯಾಗಮ "parameter";
            } ಅನ್ಯಥಾ ಯದಿ (varp->varType() == VVarType::LPARAM) {
                ಪ್ರತ್ಯಾಗಮ "local parameter";
            } ಅನ್ಯಥಾ {
                ಪ್ರತ್ಯಾಗಮ "variable";
            }
        } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstParamTypeDType* ಧ್ರುವ dtypep = VN_CAST(nodep, ParamTypeDType)) {
            ಯದಿ (dtypep->isGParam()) {
                ಪ್ರತ್ಯಾಗಮ "type parameter";
            } ಅನ್ಯಥಾ {
                ಪ್ರತ್ಯಾಗಮ "local type parameter";
            }
        } ಅನ್ಯಥಾ ಯದಿ (VN_IS(nodep, Begin)) {
            ಪ್ರತ್ಯಾಗಮ "block";
        } ಅನ್ಯಥಾ ಯದಿ (VN_IS(nodep, Cell)) {
            ಪ್ರತ್ಯಾಗಮ "instance";
        } ಅನ್ಯಥಾ ಯದಿ (VN_IS(nodep, Constraint)) {
            ಪ್ರತ್ಯಾಗಮ "constraint";
        } ಅನ್ಯಥಾ ಯದಿ (VN_IS(nodep, Func)) {
            ಪ್ರತ್ಯಾಗಮ "function";
        } ಅನ್ಯಥಾ ಯದಿ (VN_IS(nodep, GenBlock)) {
            ಪ್ರತ್ಯಾಗಮ "generate block";
        } ಅನ್ಯಥಾ ಯದಿ (VN_IS(nodep, Iface)) {
            ಪ್ರತ್ಯಾಗಮ "interface";
        } ಅನ್ಯಥಾ ಯದಿ (VN_IS(nodep, RSProd)) {
            ಪ್ರತ್ಯಾಗಮ "randsequence production";
        } ಅನ್ಯಥಾ ಯದಿ (VN_IS(nodep, Task)) {
            ಪ್ರತ್ಯಾಗಮ "task";
        } ಅನ್ಯಥಾ {
            ಪ್ರತ್ಯಾಗಮ nodep->prettyTypeName();
        }
    }

    Ast_ಗಂಟು_Module* findModuleSym(ಧ್ರುವ string& modName) {
        ಧ್ರುವ VSymEnt* ಧ್ರುವ foundp = m_mods.rootp()->findIdFallback(modName);
        ಪ್ರತ್ಯಾಗಮ foundp ? VN_AS(foundp->nodep(), ಗಂಟು_Module) : nullptr;
    }
    ಶೂನ್ಯ readModNames() {
        // Look at all modules, and store pointers to all module names
        ಪ್ರತಿ (Ast_ಗಂಟು_Module *nextp, *nodep = v3Global.rootp()->modulesp(); nodep; nodep = nextp) {
            nextp = VN_AS(nodep->nextp(), ಗಂಟು_Module);
            m_mods.rootp()->ಸೇರಿಸು(nodep->name(), ನೂತನ VSymEnt{&m_mods, nodep});
        }
    }

    VSymEnt* ಬೇರು_Entp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_syms.rootp(); }
    VSymEnt* dunitEntp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_dunitEntp; }
    ಶೂನ್ಯ checkDuplicate(VSymEnt* lookupSymp, Ast_ಗಂಟು* nodep, ಧ್ರುವ string& name) {
        // Lookup the given name under current symbol table
        // Insert if not found
        // Report error if there's a duplicate
        //
        // Note we only check for conflicts at the same level; it's ok if one block hides another
        // We also wouldn't want to not insert it even though it's lower down

        ಧ್ರುವ VSymEnt* ಧ್ರುವ foundp = lookupSymp->findIdFlat(name);
        Ast_ಗಂಟು* ಧ್ರುವ fnodep = foundp ? foundp->nodep() : nullptr;
        ಯದಿ (!fnodep) {
            // Not found, will add in a moment.
            ಯದಿ (forPrimary() && VSymEnt::checkSimilarname(nodep)) {
                ಧ್ರುವ VSymEnt* ಧ್ರುವ alt = lookupSymp->findSimilarIdFlat(name);
                ಯದಿ (alt) {
                    nodep->v3warn(SIMILARNAME, "Declaration overlaps another with different case: "
                                                   << nodep->prettyNameQ() << '\n'
                                                   << nodep->warnContextPrimary() << '\n'
                                                   << alt->nodep()->warnOther()
                                                   << "... Location of original declaration\n"
                                                   << alt->nodep()->warnContextSecondary());
                }
            }
        } ಅನ್ಯಥಾ ಯದಿ (nodep == fnodep) {  // Already inserted.
            // Good.
        } ಅನ್ಯಥಾ ಯದಿ (foundp->imported()) {  // From package
            // We don't throw VARHIDDEN as if the import is later the symbol
            // table's import wouldn't warn
        } ಅನ್ಯಥಾ ಯದಿ (forPrimary() && VN_IS(nodep, GenBlock)
                   && (VN_IS(fnodep, Begin) || VN_IS(fnodep, GenBlock))) {
            // Begin: ... blocks often replicate under genif/genfor, so
            // suppress duplicate checks.  See t_gen_forif.v for an example.
        } ಅನ್ಯಥಾ {
            UINFO(4, "name " << name);  // Not always same as nodep->name
            UINFO(4, "Var1 " << nodep);
            UINFO(4, "Var2 " << fnodep);
            // Quote the node's own name as for messages, if it is the symbol's
            ಧ್ರುವ string nameQ
                = fnodep->name() == name ? fnodep->prettyNameQ() : Ast_ಗಂಟು::prettyNameQ(name);
            ಯದಿ (nodep->type() == fnodep->type()) {
                nodep->v3error("Duplicate declaration of "
                               << ಗಂಟು_Text_Type(fnodep) << ": " << nameQ << '\n'
                               << nodep->warnContextPrimary() << '\n'
                               << fnodep->warnOther() << "... Location of original declaration\n"
                               << fnodep->warnContextSecondary());
                nodep->decl_Token_ಅಂಕೆ_Set_Min(0);  // Disable checking forward typedefs
                fnodep->decl_Token_ಅಂಕೆ_Set_Min(0);  // Disable checking forward typedefs
            } ಅನ್ಯಥಾ {
                nodep->v3error("Unsupported in C: "
                               << ucfirst(ಗಂಟು_Text_Type(nodep)) << " has the same name as "
                               << ಗಂಟು_Text_Type(fnodep) << ": " << nameQ << '\n'
                               << nodep->warnContextPrimary() << '\n'
                               << fnodep->warnOther() << "... Location of original declaration\n"
                               << fnodep->warnContextSecondary());
            }
        }
    }
    ಶೂನ್ಯ ಸೇರಿಸು_DUnit(AstNetlist* nodep) {
        // $unit on top scope
        VSymEnt* ಧ್ರುವ symp = ನೂತನ VSymEnt{&m_syms, nodep};
        UINFO(9, "      INSERTdunit se" << cvtToHex(symp));
        symp->parentp(ಬೇರು_Entp());  // Needed so backward search can find name of top module
        symp->fallbackp(nullptr);
        ಬೇರು_Entp()->ಸೇರಿಸು("$unit ", symp);  // Space so can never name conflict with user code
        //
        UASSERT_OBJ(!m_dunitEntp, nodep, "Call insertDUnit only once");
        m_dunitEntp = symp;
    }
    VSymEnt* ಸೇರಿಸು_ತುದಿ_Cell(Ast_ಗಂಟು_Module* nodep, ಧ್ರುವ string& scopename) {
        // Only called on the module at the very top of the hierarchy
        VSymEnt* ಧ್ರುವ symp = ನೂತನ VSymEnt{&m_syms, nodep};
        UINFO(9, "      INSERTtop se" << cvtToHex(symp) << "  " << scopename << " " << nodep);
        symp->parentp(ಬೇರು_Entp());  // Needed so backward search can find name of top module
        symp->fallbackp(dunitEntp());  // Needed so can find $unit stuff
        nodep->user1p(symp);
        checkDuplicate(ಬೇರು_Entp(), nodep, nodep->origName());
        ಬೇರು_Entp()->ಸೇರಿಸು(nodep->origName(), symp);
        ಯದಿ (forScopeCreation()) m_nameScopeSymMap.emplace(scopename, symp);
        ಪ್ರತ್ಯಾಗಮ symp;
    }
    VSymEnt* ಸೇರಿಸು_ತುದಿ_Iface(AstCell* nodep, ಧ್ರುವ string& scopename) {
        VSymEnt* ಧ್ರುವ symp = ನೂತನ VSymEnt{&m_syms, nodep};
        UINFO(9, "      INSERTtopiface se" << cvtToHex(symp) << "  " << scopename << " " << nodep);
        symp->parentp(ಬೇರು_Entp());  // Needed so backward search can find name of top module
        symp->fallbackp(dunitEntp());  // Needed so can find $unit stuff
        nodep->user1p(symp);
        ಯದಿ (nodep->modp()) nodep->modp()->user1p(symp);
        checkDuplicate(ಬೇರು_Entp(), nodep, nodep->origName());
        ಬೇರು_Entp()->ಸೇರಿಸು(nodep->origName(), symp);
        ಯದಿ (forScopeCreation()) m_nameScopeSymMap.emplace(scopename, symp);
        ಪ್ರತ್ಯಾಗಮ symp;
    }
    VSymEnt* ಸೇರಿಸು_Cell(VSymEnt* abovep, VSymEnt* modSymp, AstCell* nodep,
                        ಧ್ರುವ string& scopename) {
        UASSERT_OBJ(abovep, nodep, "Null symbol table inserting node");
        VSymEnt* ಧ್ರುವ symp = ನೂತನ VSymEnt{&m_syms, nodep};
        UINFO(9, "      INSERTcel se" << cvtToHex(symp) << "  " << scopename << " above=se"
                                      << cvtToHex(abovep) << " mods=se" << cvtToHex(modSymp)
                                      << " node=" << nodep);
        symp->parentp(abovep);
        symp->fallbackp(dunitEntp());  // Needed so can find $unit stuff
        nodep->user1p(symp);
        ಯದಿ (nodep->modp()) nodep->modp()->user1p(symp);
        checkDuplicate(abovep, nodep, nodep->origName());
        abovep->reinsert(nodep->origName(), symp);
        ಯದಿ (forScopeCreation() && abovep != modSymp && !modSymp->findIdFlat(nodep->name())) {
            // If it's foo_DOT_bar, we need to be able to find it under "foo_DOT_bar" too.
            // Duplicates are possible, as until resolve generates might
            // have 2 same cells under an if
            modSymp->reinsert(nodep->name(), symp);
        }
        ಯದಿ (forScopeCreation()) m_nameScopeSymMap.emplace(scopename, symp);
        ಪ್ರತ್ಯಾಗಮ symp;
    }
    ಶೂನ್ಯ ಸೇರಿಸು_Map(VSymEnt* symp, ಧ್ರುವ string& scopename) {
        ಯದಿ (forScopeCreation()) m_nameScopeSymMap.emplace(scopename, symp);
    }

    VSymEnt* ಸೇರಿಸು_Inline(VSymEnt* abovep, VSymEnt* modSymp, AstCellInline* nodep,
                          ಧ್ರುವ string& basename) {
        // A fake point in the hierarchy, corresponding to a named block flattened by V3Begin.
        // Names are looked up in the module with the block's flattened prefix.
        UASSERT_OBJ(abovep, nodep, "Null symbol table inserting node");
        VSymEnt* ಧ್ರುವ symp = ನೂತನ VSymEnt{&m_syms, nodep};
        UINFO(9, "      INSERTinl se" << cvtToHex(symp) << "  " << basename << " above=se"
                                      << cvtToHex(abovep) << " mods=se" << cvtToHex(modSymp)
                                      << " node=" << nodep);
        symp->parentp(abovep);
        symp->fallbackp(modSymp);
        symp->symPrefix(nodep->name() + "__DOT__");
        nodep->user1p(symp);
        checkDuplicate(abovep, nodep, nodep->name());
        abovep->reinsert(basename, symp);
        ಯದಿ (abovep != modSymp && !modSymp->findIdFlat(nodep->name())) {
            // If it's foo_DOT_bar, we need to be able to find it under that too.
            modSymp->reinsert(nodep->name(), symp);
        }
        ಪ್ರತ್ಯಾಗಮ symp;
    }
    VSymEnt* ಸೇರಿಸು_Block(VSymEnt* abovep, ಧ್ರುವ string& name, Ast_ಗಂಟು* nodep,
                         Ast_ಗಂಟು_Module* classOrPackagep) {
        // A fake point in the hierarchy, corresponding to a begin or function/task block
        // After we remove begins these will go away
        // Note we fallback to the symbol table of the parent, as we want to find variables there
        // However, cells walk the graph, so cells will appear under the begin/ftask itself
        UASSERT_OBJ(abovep, nodep, "Null symbol table inserting node");
        VSymEnt* ಧ್ರುವ symp = ನೂತನ VSymEnt{&m_syms, nodep};
        UINFO(9, "      INSERTblk se" << cvtToHex(symp) << "  above=se" << cvtToHex(abovep)
                                      << " pkg=" << cvtToHex(classOrPackagep)
                                      << "  node=" << nodep);
        symp->parentp(abovep);
        symp->classOrPackagep(classOrPackagep);
        symp->fallbackp(abovep);
        nodep->user1p(symp);
        ಯದಿ (name != "") checkDuplicate(abovep, nodep, name);
        // Duplicates are possible, as until resolve generates might have 2 same cells under an if
        abovep->reinsert(name, symp);
        ಪ್ರತ್ಯಾಗಮ symp;
    }
    VSymEnt* ಸೇರಿಸು_Sym(VSymEnt* abovep, ಧ್ರುವ string& name, Ast_ಗಂಟು* nodep,
                       Ast_ಗಂಟು_Module* classOrPackagep) {
        UASSERT_OBJ(abovep, nodep, "Null symbol table inserting node");
        VSymEnt* ಧ್ರುವ symp = ನೂತನ VSymEnt{&m_syms, nodep};
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
        ಪ್ರತ್ಯಾಗಮ symp;
    }
    ಸ್ಥಿರ ತಾರ್ಕಿಕ exists_ಗಂಟು_Sym(Ast_ಗಂಟು* nodep) { ಪ್ರತ್ಯಾಗಮ nodep->user1p() != nullptr; }
    ಸ್ಥಿರ VSymEnt* get_ಗಂಟು_Sym(Ast_ಗಂಟು* nodep) {
        // Don't use this in ResolveVisitor, as we need to pick up the proper
        // reference under each SCOPE
        VSymEnt* ಧ್ರುವ symp = nodep->user1u().toSymEnt();
        UASSERT_OBJ(symp, nodep, "Module/etc never assigned a symbol entry?");
        ಪ್ರತ್ಯಾಗಮ symp;
    }
    VSymEnt* getScopeSym(AstScope* nodep) {
        ಧ್ರುವ auto it = m_nameScopeSymMap.find(nodep->name());
        UASSERT_OBJ(it != m_nameScopeSymMap.end(), nodep,
                    "Scope never assigned a symbol entry '" << nodep->name() << "'");
        ಪ್ರತ್ಯಾಗಮ it->second;
    }
    ಶೂನ್ಯ implicitOkAdd(Ast_ಗಂಟು_Module* nodep, ಧ್ರುವ string& varname) {
        // Mark the given variable name as being allowed to be implicitly declared
        ಯದಿ (nodep) {
            ಧ್ರುವ auto it = m_implicitNameSet.find(std::make_pair(nodep, varname));
            ಯದಿ (it == m_implicitNameSet.end()) m_implicitNameSet.emplace(nodep, varname);
        }
    }
    ತಾರ್ಕಿಕ implicitOk(Ast_ಗಂಟು_Module* nodep, ಧ್ರುವ string& varname) {
        ಪ್ರತ್ಯಾಗಮ nodep
               && (m_implicitNameSet.find(std::make_pair(nodep, varname))
                   != m_implicitNameSet.end());
    }

    // Track and later recurse interface modules
    ಶೂನ್ಯ ಸೇರಿಸು_Iface_Mod_Sym(AstIface* nodep, VSymEnt* symp) {
        m_ifaceModSyms.emplace_back(nodep, symp);
    }
    ಶೂನ್ಯ computeIfaceModSyms();

    // Track and later insert interface references
    ಶೂನ್ಯ ಸೇರಿಸು_Iface_Var_Sym(VSymEnt* symp) {  // Where sym is for a VAR of dtype IFACEREFDTYPE
        m_ifaceVarSyms.ತಳ್ಳು_back(symp);
    }
    // Iface for a raw or arrayed iface; peels nested array layers for multi-dim arrays.
    ಸ್ಥಿರ AstIfaceRefDType* ifaceRefFromArray(Ast_ಗಂಟು_DType* nodep) {
        ಯಾವತ್ (nodep) {
            ಯದಿ (AstIfaceRefDType* ಧ್ರುವ ifp = VN_CAST(nodep, IfaceRefDType)) ಪ್ರತ್ಯಾಗಮ ifp;
            ಯದಿ (ಧ್ರುವ AstBracketArrayDType* ಧ್ರುವ arrp = VN_CAST(nodep, BracketArrayDType)) {
                nodep = arrp->subDTypep();
            } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstUnpackArrayDType* ಧ್ರುವ arrp = VN_CAST(nodep, UnpackArrayDType)) {
                nodep = arrp->subDTypep();
            } ಅನ್ಯಥಾ {
                ಪ್ರತ್ಯಾಗಮ nullptr;
            }
        }
        ಪ್ರತ್ಯಾಗಮ nullptr;
    }
    // Given a pin expression, resolve it to a live AstIface* (or nullptr).
    // Handles both simple VarRef and dotted VarXRef pin connections.
    AstIface* live_Iface_From_Pin_ಅಭಿವ್ಯಕ್ತಿ(Ast_ಗಂಟು* exprp, VSymEnt* parentSymp) {
        AstVar* resolvedVarp = nullptr;
        ಯದಿ (AstVarRef* ಧ್ರುವ refp = VN_CAST(exprp, VarRef)) {
            resolvedVarp = refp->varp();
        } ಅನ್ಯಥಾ ಯದಿ (AstVarXRef* ಧ್ರುವ xrefp = VN_CAST(exprp, VarXRef)) {
            VSymEnt* ಧ್ರುವ lookupSymp = parentSymp ? parentSymp->parentp() : nullptr;
            ಯದಿ (lookupSymp) {
                string baddot;
                VSymEnt* okSymp = nullptr;
                ಧ್ರುವ string dotpath = xrefp->dotted() + "." + xrefp->name();
                VSymEnt* ಧ್ರುವ dotSymp
                    = findDotted(xrefp->fileline(), lookupSymp, dotpath, baddot, okSymp, ಸತ್ಯ);
                ಯದಿ (dotSymp) resolvedVarp = VN_CAST(dotSymp->nodep(), Var);
            }
        }
        AstIface* resultp = nullptr;
        ಯದಿ (resolvedVarp) {
            AstIfaceRefDType* ಧ್ರುವ irefp = ifaceRefFromArray(resolvedVarp->subDTypep());
            ಯದಿ (irefp && irefp->ifaceViaCellp() && !irefp->ifaceViaCellp()->dead()) {
                resultp = irefp->ifaceViaCellp();
            }
        }
        ಪ್ರತ್ಯಾಗಮ resultp;
    }

    // Attempt to repair a port's AstIfaceRefDType by tracing through the
    // parent cell's pin connection to find the correct live interface.
    // Returns true if repaired or already correct; false if unresolvable.
    ತಾರ್ಕಿಕ repairIfaceRef(VSymEnt* varSymp, AstVar* varp, AstIfaceRefDType* ifacerefp, ತಾರ್ಕಿಕ isDead) {
        // Walk up symbol-table parents to find the enclosing AstCell
        VSymEnt* parentSymp = varSymp->parentp();
        AstCell* parentCellp = nullptr;
        ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ depth = 0; parentSymp && depth < 20; ++depth) {
            ಯದಿ (AstCell* ಧ್ರುವ cp = VN_CAST(parentSymp->nodep(), Cell)) {
                parentCellp = cp;
                ಭಙ್ಗ;
            }
            parentSymp = parentSymp->parentp();
        }
        ಯದಿ (!parentCellp) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        // Scan pins to find the one connected to this port variable
        ಪ್ರತಿ (AstPin* pinp = parentCellp->pinsp(); pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            ಯದಿ (pinp->modVarp() != varp && pinp->name() != varp->name()) ಅನುವರ್ತ;
            Ast_ಗಂಟು* ಧ್ರುವ exprp = pinp->exprp();
            ಯದಿ (!exprp) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            AstIface* ಧ್ರುವ newIfacep = live_Iface_From_Pin_ಅಭಿವ್ಯಕ್ತಿ(exprp, parentSymp);
            ಯದಿ (newIfacep && newIfacep != ifacerefp->ifaceViaCellp()) {
                UINFO(4, "  REPAIR-IFACE-REF var=" << varp->prettyNameQ()
                                                   << " old=" << ifacerefp->ifacep()->prettyNameQ()
                                                   << " new=" << newIfacep->prettyNameQ());
                ifacerefp->ifacep(newIfacep);
                ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
            }
            ಯದಿ (newIfacep) ಪ್ರತ್ಯಾಗಮ !isDead;  // Same interface - OK if live
            ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        }
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }

    ಶೂನ್ಯ computeIfaceVarSyms() {
        ಪ್ರತಿ (VSymEnt* varSymp : m_ifaceVarSyms) {
            AstVar* ಧ್ರುವ varp = varSymp ? VN_AS(varSymp->nodep(), Var) : nullptr;
            UINFO(9, "  insAllIface se" << cvtToHex(varSymp) << " " << varp);
            AstIfaceRefDType* ಧ್ರುವ ifacerefp = ifaceRefFromArray(varp->subDTypep());
            UASSERT_OBJ(ifacerefp, varp, "Non-ifacerefs on list!");
            ಯದಿ (!ifacerefp->cellp() && !ifacerefp->isVirtual() && (varp->pin_ಅಂಕೆ() == 0)) {
                varp->v3error("Interface port declaration "
                              << varp->prettyNameQ() << " doesn't have corresponding port\n"
                              << varp->warnMore()
                                     + "... Perhaps intended an interface instantiation but "
                                       "are missing parenthesis (IEEE 1800-2023 25.3)?");
            }
            ಯದಿ (!ifacerefp->ifaceViaCellp()) {
                ಯದಿ (!ifacerefp->cellp()) {  // Probably a NotFoundModule, or a normal module if
                                            // made mistake
                    UINFO(1, "Associated cell " << Ast_ಗಂಟು::prettyNameQ(ifacerefp->cellName()));
                    ifacerefp->v3error("Cannot find file containing interface: "
                                       << Ast_ಗಂಟು::prettyNameQ(ifacerefp->ifaceName()));
                    ಅನುವರ್ತ;
                } ಅನ್ಯಥಾ {
                    ifacerefp->v3fatalSrc("Unlinked interface");
                }
            } ಅನ್ಯಥಾ ಯದಿ (varp->isIfaceRef() && !ifacerefp->cellp() && ifacerefp->ifaceViaCellp()) {
                // Port variable (cellp=null) pointing to an interface.
                // The interface may be dead (template replaced by clone) or
                // live-but-wrong (template still alive because some instances
                // use default params, but this port should point to a
                // specialized clone based on the pin connection).
                ಧ್ರುವ ತಾರ್ಕಿಕ isDead = ifacerefp->ifaceViaCellp()->dead();
                ಯದಿ (isDead && forPrimary() && !varp->isIfaceParent()
                    && !v3Global.opt.ತುದಿ_Ifaces_Supported()) {
                    varp->v3warn(E_UNSUPPORTED,
                                 "Unsupported: Interfaced port on top level module");
                }
                // Attempt repair: trace through parent cell's pin to find
                // the correct live interface for this port variable
                ಧ್ರುವ ತಾರ್ಕಿಕ repaired = repairIfaceRef(varSymp, varp, ifacerefp, isDead);
                ಯದಿ (!repaired && isDead) {
                    ifacerefp->v3error(
                        "Interface " << Ast_ಗಂಟು::prettyNameQ(ifacerefp->ifaceName())
                                     << " not connected as parent's interface not connected\n"
                                     << ifacerefp->warnMore()
                                     << "... Perhaps caused by another error on the parent "
                                        "interface that needs resolving\n"
                                     << ifacerefp->warnMore()
                                     << "... Or, perhaps intended an interface instantiation but "
                                        "are missing parenthesis (IEEE 1800-2023 25.3)?");
                    ಅನುವರ್ತ;
                }
            } ಅನ್ಯಥಾ ಯದಿ (ifacerefp->ifaceViaCellp()->dead()
                       || !exists_ಗಂಟು_Sym(ifacerefp->ifaceViaCellp())) {
                ifacerefp->ifaceViaCellp()->v3fatalSrc(
                    "Interface referenced by virtual iface is dead. "
                    "Dead interfaces have required linking steps skipped");
            }
            VSymEnt* ಧ್ರುವ ifaceSymp = get_ಗಂಟು_Sym(ifacerefp->ifaceViaCellp());
            VSymEnt* ifOrPortSymp = ifaceSymp;
            // Link Modport names to the Modport Node under the Interface
            ಯದಿ (ifacerefp->isModport()) {
                VSymEnt* ಧ್ರುವ foundp = ifaceSymp->findIdFallback(ifacerefp->modportName());
                ತಾರ್ಕಿಕ ok = ಅಸತ್ಯ;
                ಯದಿ (foundp) {
                    ಯದಿ (AstModport* ಧ್ರುವ modportp = VN_CAST(foundp->nodep(), Modport)) {
                        UINFO(4, "Link Modport: " << modportp);
                        ifacerefp->modportp(modportp);
                        ifOrPortSymp = foundp;
                        ok = ಸತ್ಯ;
                    }
                }
                ಯದಿ (!ok) {
                    ಧ್ರುವ string suggest = suggestSymFallback(ifaceSymp, ifacerefp->modportName(),
                                                              Link_ಗಂಟು_Matcher_Modport{});
                    ifacerefp->modportFileline()->v3error(
                        "Modport not found under interface "
                        << ifacerefp->prettyNameQ(ifacerefp->ifaceName()) << ": "
                        << ifacerefp->prettyNameQ(ifacerefp->modportName()) << '\n'
                        << (suggest.empty() ? "" : ifacerefp->warnMore() + suggest));
                }
            }
            // Alias won't expand until interfaces and modport names are known; see notes at top
            ಸೇರಿಸು_Scope_Alias(SAMN_IFTOP, varSymp, ifOrPortSymp);
        }
        m_ifaceVarSyms.clear();
    }

    ಶೂನ್ಯ ಸೇರಿಸು_Scope_Alias(SAMNum samn, VSymEnt* lhsp, VSymEnt* rhsp) {
        // Track and later insert scope aliases; an interface referenced by
        // a child cell connecting to that interface
        // Typically lhsp=VAR w/dtype IFACEREF, rhsp=IFACE cell
        UINFO(9, "   insertScopeAlias se" << cvtToHex(lhsp) << " se" << cvtToHex(rhsp));
        UASSERT_OBJ(
            !(VN_IS(rhsp->nodep(), Cell) && !VN_IS(VN_AS(rhsp->nodep(), Cell)->modp(), Iface)),
            rhsp->nodep(), "Got a non-IFACE alias RHS");
        m_scopeAliasMap[samn].emplace(lhsp, rhsp);
    }
    ಶೂನ್ಯ computeScopeAliases() {
        UINFO(9, "computeIfaceAliases");
        ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ samn = 0; samn < SAMN__MAX; ++samn) {
            ಪ್ರತಿ (ScopeAliasMap::iterator it = m_scopeAliasMap[samn].begin();
                 it != m_scopeAliasMap[samn].end(); ++it) {
                VSymEnt* ಧ್ರುವ lhsp = it->first;
                VSymEnt* srcp = lhsp;
                ಯಾವತ್ (ಸತ್ಯ) {  // Follow chain of aliases up to highest level non-alias
                    ಧ್ರುವ auto it2 = m_scopeAliasMap[samn].find(srcp);
                    ಯದಿ (it2 != m_scopeAliasMap[samn].end()) {
                        srcp = it2->second;
                        ಅನುವರ್ತ;
                    } ಅನ್ಯಥಾ {
                        ಭಙ್ಗ;
                    }
                }
                UINFO(9, "  iiasa: Insert alias se" << lhsp << " (" << lhsp->nodep()->typeName()
                                                    << ") <- se" << srcp << " " << srcp->nodep());
                // srcp should be an interface reference pointing to the interface we want to
                // import
                lhsp->importFromIface(symsp(), srcp);
                // Allow access to objects not permissible to be listed in a modport
                ಯದಿ (VN_IS(srcp->nodep(), Modport)) {
                    lhsp->importFromIface(symsp(), srcp->parentp(), ಸತ್ಯ);
                }
            }
            // m_scopeAliasMap[samn].clear();  // Done with it, but put into debug file
        }
    }

ಗುಪ್ತ:
    VSymEnt* findWithAltFallback(VSymEnt* symp, ಧ್ರುವ string& name, ಧ್ರುವ string& altname) {
        VSymEnt* findp = symp->findIdFallback(name);
        ಯದಿ (findp) ಪ್ರತ್ಯಾಗಮ findp;
        ಯದಿ (altname != "") {
            UINFO(8, "     alt fallback");
            findp = symp->findIdFallback(altname);
        }
        ಪ್ರತ್ಯಾಗಮ findp;
    }

    VSymEnt* findWithAltFlat(VSymEnt* symp, ಧ್ರುವ string& name, ಧ್ರುವ string& altname) {
        VSymEnt* findp = symp->findIdFlat(name);
        ಯದಿ (findp) ಪ್ರತ್ಯಾಗಮ findp;
        ಯದಿ (altname != "") {
            UINFO(8, "     alt flat");
            findp = symp->findIdFlat(altname);
        }
        ಪ್ರತ್ಯಾಗಮ findp;
    }

ಸಾರ್ವಜನಿಕ:
    VSymEnt* findDotted(File_ಸಾಲು* refLocationp, VSymEnt* lookupSymp, ಧ್ರುವ string& dotname,
                        string& baddot, VSymEnt*& okSymp, ತಾರ್ಕಿಕ firstId) {
        // Given a dotted hierarchy name, return where in scope it is
        // Note when dotname=="" we just fall through and return lookupSymp
        UINFO(8, "    dottedFind se" << cvtToHex(lookupSymp) << " '" << dotname << "'");
        string leftname = dotname;
        okSymp = lookupSymp;  // So can list bad scopes
        ಯಾವತ್ (leftname != "") {  // foreach dotted part of xref name
            string::size_type pos = leftname.find('.');
            string ident;
            ಯದಿ (pos != string::npos) {
                ident = leftname.substr(0, pos);
                leftname = leftname.substr(pos + 1);
            } ಅನ್ಯಥಾ {
                ident = leftname;
                leftname = "";
            }
            baddot = ident;  // So user can see where they botched it
            okSymp = lookupSymp;
            string altIdent;
            ಯದಿ (!forScopeCreation()) {
                // GENFOR Begin is foo__BRA__##__KET__ after we've genloop unrolled,
                // but presently should be just "foo".
                // Likewise cell foo__[array] before we've expanded arrays is just foo.
                // Multi-dim iface arrays append multiple __BRA__..__KET__ suffixes; strip them
                // all.
                altIdent = ident;
                ಯಾವತ್ (VString::endsWith(altIdent, "__KET__")) {
                    ಧ್ರುವ auto braPos = altIdent.rfind("__BRA__");
                    ಯದಿ (braPos == string::npos) ಭಙ್ಗ;
                    altIdent = altIdent.substr(0, braPos);
                }
                ಯದಿ (altIdent == ident) altIdent.clear();
            }
            UINFO(8, "         id " << ident << " alt " << altIdent << " left " << leftname
                                    << " at se" << lookupSymp);
            // Spec says; Look at existing module (cellnames then modname),
            // then look up (inst name or modname)
            ಯದಿ (firstId) {
                // Check this module - subcellnames
                ಧ್ರುವ AstCell* cellp = lookupSymp ? VN_CAST(lookupSymp->nodep(), Cell)
                                                  : nullptr;  // Replicated below
                ಯದಿ (VSymEnt* ಧ್ರುವ findSymp = findWithAltFallback(lookupSymp, ident, altIdent)) {
                    lookupSymp = findSymp;
                }

                // Check this module - cur modname
                ಅನ್ಯಥಾ ಯದಿ (cellp && cellp->modp()->origName() == ident) {
                }
                // $root we walk up to Netlist
                ಅನ್ಯಥಾ ಯದಿ (ident == "$root") {
                    lookupSymp = ಬೇರು_Entp();
                    // We've added the '$root' module, now everything else is one lower
                    ಯದಿ (forScopeCreation()) {
                        lookupSymp = lookupSymp->findIdFlat(ident);
                        UASSERT(lookupSymp, "Cannot find $root module under netlist");
                    }
                }
                // Move up and check cellname + modname
                ಅನ್ಯಥಾ {
                    ತಾರ್ಕಿಕ crossedCell = ಅಸತ್ಯ;  // Crossed a cell boundary
                    ಯಾವತ್ (lookupSymp) {
                        lookupSymp = lookupSymp->parentp();
                        cellp = lookupSymp ? VN_CAST(lookupSymp->nodep(), Cell)
                                           : nullptr;  // Replicated above
                        ಯದಿ (lookupSymp) {
                            UINFO(9, "    Up to " << lookupSymp);
                            ಯದಿ (cellp || VN_IS(lookupSymp->nodep(), CellInline)) {
                                crossedCell = ಸತ್ಯ;
                            }
                            ಯದಿ (cellp && cellp->modp()->origName() == ident) {
                                ಭಙ್ಗ;
                            } ಅನ್ಯಥಾ ಯದಿ (VSymEnt* ಧ್ರುವ findSym2p
                                       = findWithAltFallback(lookupSymp, ident, altIdent)) {
                                lookupSymp = findSym2p;
                                ಯದಿ (crossedCell && VN_IS(lookupSymp->nodep(), Var)) {
                                    UINFO(9, "    Not found but matches var name in parent "
                                                 << lookupSymp);
                                    ಪ್ರತ್ಯಾಗಮ nullptr;
                                }
                                ಭಙ್ಗ;
                            }
                        } ಅನ್ಯಥಾ {
                            ಭಙ್ಗ;
                        }
                    }
                    ಯದಿ (!lookupSymp) ಪ್ರತ್ಯಾಗಮ nullptr;  // Not found
                }
            } ಅನ್ಯಥಾ {  // Searching for middle path component, must be a cell or interface port
                VSymEnt* findSymp = findWithAltFlat(lookupSymp, ident, altIdent);
                ಯದಿ (findSymp) {
                    lookupSymp = findSymp;
                } ಅನ್ಯಥಾ {
                    // Try prefixed lookup for interface ports accessed through hierarchy
                    // (e.g., slave_inst.bus.data where bus is an interface port)
                    // After inlining, interface ports have prefixed names like
                    // top__DOT__slave_inst__DOT__bus
                    UINFO(8, "     middle-path: trying prefixed lookup for '" << ident << "'\n");
                    string baddot;
                    findSymp = findSymPrefixed(lookupSymp, ident, baddot, /*fallback=*/ಸತ್ಯ);
                    UINFO(8, "     middle-path: prefixed lookup result: "
                                 << (findSymp ? "found" : "not found") << "\n");
                    ಯದಿ (findSymp) {
                        // Check if this is an interface reference variable
                        ಧ್ರುವ AstVar* varp = VN_CAST(findSymp->nodep(), Var);
                        ಯದಿ (!varp) {
                            ಯದಿ (ಧ್ರುವ AstVarScope* vscp = VN_CAST(findSymp->nodep(), VarScope)) {
                                varp = vscp->varp();
                            }
                        }
                        ಯದಿ (varp && varp->isIfaceRef()) {
                            // Found an interface port - use findSymp directly
                            // computeScopeAliases has already imported interface members
                            // into this symbol entry, so we use it instead of redirecting
                            // to the shared modport definition (which would cause all
                            // instances to resolve to the same symbols - bug #2656)
                            lookupSymp = findSymp;
                        } ಅನ್ಯಥಾ {
                            // Non-interface symbol in middle path must be a cell (module instance)
                            // to continue hierarchical resolution
                            ಯದಿ (VN_IS(findSymp->nodep(), Cell)) {
                                lookupSymp = findSymp;
                            } ಅನ್ಯಥಾ {
                                // Reject non-cell, non-interface symbols found via fallback
                                // to prevent accidental resolution to unrelated symbols
                                UINFO(8, "     middle-path: rejecting non-cell symbol\n");
                                ಪ್ರತ್ಯಾಗಮ nullptr;
                            }
                        }
                    } ಅನ್ಯಥಾ {
                        ಪ್ರತ್ಯಾಗಮ nullptr;  // Not found
                    }
                }
            }
            ಯದಿ (lookupSymp) {
                ಯದಿ (ಧ್ರುವ AstCell* ಧ್ರುವ cellp = VN_CAST(lookupSymp->nodep(), Cell)) {
                    ಯದಿ (ಧ್ರುವ Ast_ಗಂಟು_Module* ಧ್ರುವ modp = cellp->modp()) {
                        ಯದಿ (modp->hierBlock() && !leftname.empty()) {
                            refLocationp->v3error("Cannot access scope inside hierarchical block");
                        } ಅನ್ಯಥಾ ಯದಿ (VN_IS(modp, NotFoundModule)) {
                            refLocationp->v3error("Dotted reference to instance that refers to "
                                                  "missing module/interface: "
                                                  << modp->prettyNameQ());
                        }
                    }
                }
                // Follow scope alias for nested interface port access
                ಯದಿ (!leftname.empty()) {
                    ಧ್ರುವ auto aliasIt = m_scopeAliasMap[SAMN_IFTOP].find(lookupSymp);
                    ಯದಿ (aliasIt != m_scopeAliasMap[SAMN_IFTOP].end()) {
                        lookupSymp = aliasIt->second;
                        // Alias may point to __Viftop VarScope; find corresponding Cell
                        ಯದಿ (ಧ್ರುವ AstVarScope* ಧ್ರುವ vscp
                            = VN_CAST(lookupSymp->nodep(), VarScope)) {
                            ಧ್ರುವ string varName = vscp->varp()->name();
                            ಸ್ಥಿರ constexpr ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ VIFTOP_SUFFIX = "__Viftop";
                            ಯದಿ (VString::endsWith(varName, VIFTOP_SUFFIX)) {
                                ಧ್ರುವ string cellName
                                    = varName.substr(0, varName.size() - strlen(VIFTOP_SUFFIX));
                                VSymEnt* ಧ್ರುವ parentSymp = lookupSymp->parentp();
                                ಯದಿ (parentSymp) {
                                    VSymEnt* ಧ್ರುವ cellSymp = parentSymp->findIdFlat(cellName);
                                    ಯದಿ (cellSymp && VN_IS(cellSymp->nodep(), Cell)) {
                                        lookupSymp = cellSymp;  // Use Cell for member lookup
                                    }
                                }
                            }
                        }
                    } ಅನ್ಯಥಾ {
                        // No alias; try following IfaceRefDType to interface cell
                        ಧ್ರುವ AstVar* varp = VN_CAST(lookupSymp->nodep(), Var);
                        ಯದಿ (!varp) {
                            ಯದಿ (ಧ್ರುವ AstVarScope* ಧ್ರುವ vscp
                                = VN_CAST(lookupSymp->nodep(), VarScope)) {
                                varp = vscp->varp();
                            }
                        }
                        ಯದಿ (varp && varp->isIfaceRef()) {
                            ಯದಿ (ಧ್ರುವ AstIfaceRefDType* ಧ್ರುವ ifaceRefp
                                = ifaceRefFromArray(varp->dtypep())) {
                                ಯದಿ (ifaceRefp->cellp() && exists_ಗಂಟು_Sym(ifaceRefp->cellp())) {
                                    lookupSymp = get_ಗಂಟು_Sym(ifaceRefp->cellp());
                                } ಅನ್ಯಥಾ ಯದಿ (ifaceRefp->ifaceViaCellp()
                                           && exists_ಗಂಟು_Sym(ifaceRefp->ifaceViaCellp())) {
                                    lookupSymp = get_ಗಂಟು_Sym(ifaceRefp->ifaceViaCellp());
                                } ಅನ್ಯಥಾ ಯದಿ (ifaceRefp->ifacep()
                                           && exists_ಗಂಟು_Sym(ifaceRefp->ifacep())) {
                                    lookupSymp = get_ಗಂಟು_Sym(ifaceRefp->ifacep());
                                }
                            }
                        }
                    }
                }
            }
            firstId = ಅಸತ್ಯ;
        }
        ಪ್ರತ್ಯಾಗಮ lookupSymp;
    }

    ಸ್ಥಿರ string removeLastInlineScope(ಧ್ರುವ string& name) {
        string out = name;
        ಧ್ರುವ string dot = "__DOT__";
        ಧ್ರುವ size_t dotPos = out.rfind(dot, out.size() - dot.length() - 1);
        ಯದಿ (dotPos == string::npos) {
            ಪ್ರತ್ಯಾಗಮ "";
        } ಅನ್ಯಥಾ {
            ಪ್ರತ್ಯಾಗಮ out.erase(dotPos + dot.length(), string::npos);
        }
    }

    VSymEnt* findSymPrefixed(VSymEnt* lookupSymp, ಧ್ರುವ string& dotname, string& baddot,
                             ತಾರ್ಕಿಕ fallback) {
        // Find symbol in given point in hierarchy, allowing prefix (post-Inline)
        // For simplicity lookupSymp may be passed nullptr result from findDotted
        ಯದಿ (!lookupSymp) ಪ್ರತ್ಯಾಗಮ nullptr;
        UINFO(8, "    findSymPrefixed "
                     << dotname << " under se" << cvtToHex(lookupSymp)
                     << ((lookupSymp->symPrefix() == "") ? "" : " as ")
                     << ((lookupSymp->symPrefix() == "") ? "" : lookupSymp->symPrefix() + dotname)
                     << "  at se" << lookupSymp << " fallback=" << fallback);
        string prefix = lookupSymp->symPrefix();
        VSymEnt* foundp = nullptr;
        ಯಾವತ್ (!foundp) {
            ಯದಿ (fallback) {
                foundp = lookupSymp->findIdFallback(prefix + dotname);  // Might be nullptr
            } ಅನ್ಯಥಾ {
                foundp = lookupSymp->findIdFlat(prefix + dotname);  // Might be nullptr
            }
            ಯದಿ (prefix.empty()) ಭಙ್ಗ;
            ಧ್ರುವ string nextPrefix = removeLastInlineScope(prefix);
            ಯದಿ (prefix == nextPrefix) ಭಙ್ಗ;
            prefix = nextPrefix;
        }
        ಯದಿ (!foundp) baddot = dotname;
        ಪ್ರತ್ಯಾಗಮ foundp;
    }
    ಸ್ಥಿರ ತಾರ್ಕಿಕ checkIfClassOrPackage(ಧ್ರುವ VSymEnt* ಧ್ರುವ symp) {
        ಯದಿ (VN_IS(symp->nodep(), Class) || VN_IS(symp->nodep(), Package)) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;

        // Helper: check if a RefDType might resolve to a class later.
        ಧ್ರುವ auto isPotentialClassRef = [](ಧ್ರುವ AstRefDType* refp) -> ತಾರ್ಕಿಕ {
            ಪ್ರತ್ಯಾಗಮ refp && !refp->typeofp()
                   && (!refp->classOrPackageOpp()
                       || VN_IS(refp->classOrPackageOpp(), ClassOrPackageRef));
        };

        // Helper: allow types that can represent a class/package handle or an unresolved ref.
        // BasicDType is allowed here only for null/object handles.
        ಧ್ರುವ auto is_Valid_Type_ಗಂಟು = [](ಧ್ರುವ Ast_ಗಂಟು* nodep) -> ತಾರ್ಕಿಕ {
            ಪ್ರತ್ಯಾಗಮ VN_IS(nodep, VoidDType) || VN_IS(nodep, BasicDType)
                   || VN_IS(nodep, ClassRefDType) || VN_IS(nodep, ParseRef);
        };

        ಯದಿ (ಧ್ರುವ AstTypedef* ಧ್ರುವ typedefp = VN_CAST(symp->nodep(), Typedef)) {
            ಧ್ರುವ Ast_ಗಂಟು_DType* dtypep = typedefp->subDTypep();
            ಯದಿ (!dtypep) dtypep = typedefp->childDTypep();
            ಯದಿ (VN_IS(dtypep, ClassRefDType)) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
            ಯದಿ (isPotentialClassRef(VN_CAST(dtypep, RefDType))) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstParamTypeDType* ಧ್ರುವ paramTypep
                   = VN_CAST(symp->nodep(), ParamTypeDType)) {
            // Before V3Param the declared default is in childDTypep (possibly
            // wrapped in a RequireDType); after V3Param it is consumed and the
            // bound type is the resolved data type, e.g. a type parameter
            // inherited from a specialized base class (REQ #(Item) -> class Item).
            Ast_ಗಂಟು* childp = paramTypep->childDTypep();
            ಯದಿ (ಧ್ರುವ AstRequireDType* ಧ್ರುವ reqp = VN_CAST(childp, RequireDType)) {
                childp = reqp->lhsp();
            }
            ಧ್ರುವ Ast_ಗಂಟು* ಧ್ರುವ checkp = childp ? childp : paramTypep->skipRefp();
            ಯದಿ (is_Valid_Type_ಗಂಟು(checkp) || isPotentialClassRef(VN_CAST(checkp, RefDType))) {
                ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
            }
        }
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }
    VSymEnt* resolveClassOrPackage(VSymEnt* lookSymp, AstClassOrPackageRef* nodep, ತಾರ್ಕಿಕ fallback,
                                   ತಾರ್ಕಿಕ classOnly, ಧ್ರುವ string& forWhat,
                                   ತಾರ್ಕಿಕ deferIfUnresolved = ಅಸತ್ಯ) {
        ಯದಿ (nodep->classOrPackageSkipp()) ಪ್ರತ್ಯಾಗಮ get_ಗಂಟು_Sym(nodep->classOrPackageSkipp());
        VSymEnt* foundp;
        VSymEnt* ಹುಡುಕು_Symp = lookSymp;
        ಯದಿ (VL_UNCOVERABLE(ಹುಡುಕು_Symp && VN_IS(ಹುಡುಕು_Symp->nodep(), ParamTypeDType))) {
            ಹುಡುಕು_Symp->nodep()->v3fatalSrc(  // LCOV_EXCL_LINE
                "resolveClassOrPackage: unexpected ParamTypeDType lookup");
        }
        ಯದಿ (fallback) {
            VSymEnt* currentLookSymp = ಹುಡುಕು_Symp;
            ಕುರು {
                foundp = currentLookSymp->findIdFlat(nodep->name());
                ಯದಿ (foundp && !checkIfClassOrPackage(foundp)) foundp = nullptr;
                ಯದಿ (!foundp) currentLookSymp = currentLookSymp->fallbackp();
            } ಯಾವತ್ (!foundp && currentLookSymp);
        } ಅನ್ಯಥಾ {
            foundp = ಹುಡುಕು_Symp->findIdFlat(nodep->name());
            ಯದಿ (foundp && !checkIfClassOrPackage(foundp)) foundp = nullptr;
        }
        ಯದಿ (!foundp && v3Global.rootp()->stdPackagep()) {  // Look under implied std::
            foundp = get_ಗಂಟು_Sym(v3Global.rootp()->stdPackagep())->findIdFlat(nodep->name());
        }
        ಯದಿ (foundp) {
            nodep->classOrPackageNodep(foundp->nodep());
            ಪ್ರತ್ಯಾಗಮ foundp;
        }
        ಯದಿ (deferIfUnresolved) ಪ್ರತ್ಯಾಗಮ nullptr;
        ಧ್ರುವ string suggest
            = suggestSymFallback(lookSymp, nodep->name(), Link_ಗಂಟು_Matcher_Class_Or_Package{});
        nodep->v3error((classOnly ? "Class" : "Package/class")
                       << " for '" << forWhat  // extends/implements
                       << "' not found: " << nodep->prettyNameQ() << '\n'
                       << (suggest.empty() ? "" : nodep->warnMore() + suggest));
        ಪ್ರತ್ಯಾಗಮ nullptr;
    }
    string suggestSymFallback(VSymEnt* lookupSymp, ಧ್ರುವ string& name,
                              ಧ್ರುವ VNodeMatcher& matcher) {
        // Suggest alternative symbol in given point in hierarchy
        // Does not support inline, as we find user-level errors before inlining
        // For simplicity lookupSymp may be passed nullptr result from findDotted
        ಯದಿ (!lookupSymp) ಪ್ರತ್ಯಾಗಮ "";
        VSpellCheck speller;
        lookupSymp->candidateIdFallback(&speller, &matcher);
        ಪ್ರತ್ಯಾಗಮ speller.best_Candidate_ಸಂದೇಶ(name);
    }
    string suggestSymFlat(VSymEnt* lookupSymp, ಧ್ರುವ string& name, ಧ್ರುವ VNodeMatcher& matcher,
                          ತಾರ್ಕಿಕ includeType) {
        ಯದಿ (!lookupSymp) ಪ್ರತ್ಯಾಗಮ "";
        VSpellCheck speller;
        lookupSymp->candidateIdFlat(&speller, &matcher, includeType);
        ಪ್ರತ್ಯಾಗಮ speller.best_Candidate_ಸಂದೇಶ(name);
    }
};

LinkDotState* LinkDotState::s_errorThisp = nullptr;

//======================================================================

ವರ್ಗ LinkDotFindVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // NODE STATE
    // Cleared on global
    //  *::user1p()             -> See LinkDotState
    //  *::user2p()             -> See LinkDotState
    //  *::user3()              // bool. Node stored in m_virtualIfaces
    //  *::user4()              -> See LinkDotState
    ಧ್ರುವ VNUser3InUse m_inuser3;

    // STATE
    LinkDotState* ಧ್ರುವ m_statep;  // State to pass between visitors, including symbol table
    Ast_ಗಂಟು_Module* m_classOrPackagep = nullptr;  // Current package
    AstClocking* m_clockingp = nullptr;  // Current clocking block
    std::vector<AstScope*> m_scopes;  // Scopes detached during repeated module visits
    uint64_t m_statScopeVisits = 0;  // Non-top scopes encountered while finding instance symbols
    VSymEnt* m_modSymp = nullptr;  // Symbol Entry for current module
    VSymEnt* m_curSymp = nullptr;  // Symbol Entry for current table, where to lookup/insert
    string
        m_hierParamsName;  // Name of module with hierarchical type parameters, empty when not used
    string m_scope;  // Scope text
    ಧ್ರುವ Ast_ಗಂಟು_FTask* m_ftaskp = nullptr;  // Current function/task
    ತಾರ್ಕಿಕ m_inRecursion = ಅಸತ್ಯ;  // Inside a recursive module
    ಪೂರ್ಣಾಙ್ಕ m_param_ಅಂಕೆ = 0;  // Parameter number, for position based connection
    ತಾರ್ಕಿಕ m_explicitNew = ಅಸತ್ಯ;  // Hit a "new" function
    ಪೂರ್ಣಾಙ್ಕ m_mod_Block_ಅಂಕೆ = 0;  // Begin block number in module, 0=none seen
    ಪೂರ್ಣಾಙ್ಕ m_mod_With_ಅಂಕೆ = 0;  // With block number, 0=none seen
    ಪೂರ್ಣಾಙ್ಕ m_mod_Arg_ಅಂಕೆ = 0;  // Arg block number for randomize(), 0=none seen
    std::vector<AstIface*> m_virtIfaces;  // interfaces used as virtual,
                                          // needed for handleUnvisitedVirtIfaces()

    // METHODS
    ಶೂನ್ಯ makeImplicitNew(AstClass* nodep) {
        AstFunc* ಧ್ರುವ newp = ನೂತನ AstFunc{nodep->fileline(), "new", nullptr, nullptr};
        // If needed, super.new() call is added the 2nd pass of V3LinkDotResolve,
        // because base classes are already resolved there.
        newp->isConstructor(ಸತ್ಯ);
        nodep->addMembersp(newp);
        UINFO(8, "Made implicit new for " << nodep->name() << ": " << nodep);
        m_statep->ಸೇರಿಸು_Block(m_curSymp, newp->name(), newp, m_classOrPackagep);
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ isHierBlockWrapper(ಧ್ರುವ string& name) {
        ಧ್ರುವ V3HierBlockOptSet& hierBlocks = v3Global.opt.hierBlocks();
        ಪ್ರತ್ಯಾಗಮ hierBlocks.find(name) != hierBlocks.end();
    }

    ಶೂನ್ಯ moveExternFuncDecl(Ast_ಗಂಟು_FTask* nodep, AstClass* classp) {
        // Move an 'extern function|task' to inside a class or, from an outer to inner-class
        ಯದಿ (nodep->isExternDef()) ಪ್ರತ್ಯಾಗಮ;
        // Move it to proper spot under the target class
        nodep->unlinkFrBack();
        classp->addStmtsp(nodep);
        nodep->isExternDef(ಸತ್ಯ);  // So we check there's a matching extern
        nodep->classOrPackagep()->unlinkFrBack()->delete_ವೃಕ್ಷ();
        // Any "Type::" reference in the function's IO are really "MovedToClass::" references
        ಯದಿ (nodep->fvarp()) moveExternFuncDeclRefs(nodep->fvarp(), classp);
        ಪ್ರತಿ (Ast_ಗಂಟು* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ಯದಿ (AstVar* ಧ್ರುವ portp = VN_CAST(stmtp, Var)) {
                ಯದಿ (portp->isIO()) moveExternFuncDeclRefs(portp, classp);
            }
        }
    }
    ಶೂನ್ಯ moveExternFuncDeclRefs(Ast_ಗಂಟು* nodep, AstClass* classp) {
        nodep->foreach([ಏತತ್, classp](AstClassOrPackageRef* refp) {  //
            ಯದಿ (refp->name() == classp->name() && !refp->paramsp()) {
                UINFO(9, "Cleaning up external function type for class " << refp);
                AstDot* ಧ್ರುವ dotp = VN_CAST(refp->backp(), Dot);
                ಯದಿ (dotp && dotp->lhsp() == refp) {
                    // If had DOT(CLASSREF this, ...) the DOT can go away
                    dotp->replaceWith(dotp->rhsp()->unlinkFrBack());
                    VL_DO_DANGLING2(ತಳ್ಳು_Deletep(dotp), dotp, refp);
                } ಅನ್ಯಥಾ {
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(refp->unlinkFrBack()), refp);
                }
            }
        });
    }

    // VISITORS
    ಶೂನ್ಯ visit(AstNetlist* nodep) override {  // FindVisitor::
        // Process $unit or other packages
        // Not needed - dotted references not allowed from inside packages
        // for (AstNodeModule* nodep = v3Global.rootp()->modulesp();
        //     nodep; nodep=VN_AS(nodep->nextp(), NodeModule)) {
        //    if (VN_IS(nodep, Package)) {}}

        m_statep->ಸೇರಿಸು_DUnit(nodep);

        // First back iterate, to find all packages. Backward as must do base
        // packages before using packages
        iterateChildrenBackwardsConst(nodep);

        // The first modules in the list are always the top modules
        // (sorted before this is called).
        // This may not be the module with isTop() set, as early in the steps,
        // wrapTop may have not been created yet.
        // $unit and the constant pool always exist, so nothing else, and nothing in $unit,
        // means nothing was given
        Ast_ಗಂಟು_Module* ಧ್ರುವ modulesp = nodep->modulesp();
        UASSERT_OBJ(modulesp, nodep, "$unit should always be in the netlist");
        ಯದಿ (m_statep->forPrimary()) {
            ಧ್ರುವ ತಾರ್ಕಿಕ onlyBuiltins = [&]() {
                ಪ್ರತಿ (Ast_ಗಂಟು_Module* modp = modulesp; modp;
                     modp = VN_AS(modp->nextp(), ಗಂಟು_Module)) {
                    ಯದಿ (!modp->isDollarUnit() && !modp->isConstPool()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
                }
                ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
            }();
            ಯದಿ (onlyBuiltins && !v3Global.rootp()->dollarUnitPkgp()->stmtsp()) {
                nodep->v3error("No top level module found");
            }
        }
        ಪ್ರತಿ (Ast_ಗಂಟು_Module* modp = nodep->modulesp(); modp && modp->is_ತುದಿ();
             modp = VN_AS(modp->nextp(), ಗಂಟು_Module)) {
            UINFO(8, "Top Module: " << modp);
            m_scope = "TOP";

            ಯದಿ (!m_statep->forScopeCreation() && v3Global.opt.ತುದಿ_Ifaces_Supported()) {
                ಪ್ರತಿ (Ast_ಗಂಟು* subnodep = modp->stmtsp(); subnodep; subnodep = subnodep->nextp()) {
                    ಯದಿ (AstVar* ಧ್ರುವ varp = VN_CAST(subnodep, Var)) {
                        ಯದಿ (varp->isIfaceRef()) {
                            ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ subtypep = varp->subDTypep();
                            ಧ್ರುವ AstIfaceRefDType* ifacerefp = nullptr;
                            ಯದಿ (VN_IS(subtypep, IfaceRefDType)) {
                                ifacerefp = VN_AS(varp->subDTypep(), IfaceRefDType);
                            } ಅನ್ಯಥಾ ಯದಿ (VN_IS(subtypep, BracketArrayDType)) {
                                ಧ್ರುವ AstBracketArrayDType* ಧ್ರುವ arrp
                                    = VN_AS(subtypep, BracketArrayDType);
                                ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ arrsubtypep = arrp->subDTypep();
                                ಯದಿ (VN_IS(arrsubtypep, IfaceRefDType)) {
                                    ifacerefp = VN_AS(arrsubtypep, IfaceRefDType);
                                }
                            } ಅನ್ಯಥಾ ಯದಿ (VN_IS(subtypep, UnpackArrayDType)) {
                                ಧ್ರುವ AstUnpackArrayDType* ಧ್ರುವ arrp
                                    = VN_AS(subtypep, UnpackArrayDType);
                                ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ arrsubtypep = arrp->subDTypep();
                                ಯದಿ (VN_IS(arrsubtypep, IfaceRefDType)) {
                                    ifacerefp = VN_AS(arrsubtypep, IfaceRefDType);
                                }
                            }

                            ಯದಿ (ifacerefp && !ifacerefp->cellp()) {
                                // A dummy cell to keep the top level interface alive and correctly
                                // optimized for default parameter values
                                AstCell* ifacecellp
                                    = ನೂತನ AstCell{nodep->fileline(),
                                                  nodep->fileline(),
                                                  modp->name() + "__02E" + varp->name(),
                                                  ifacerefp->ifaceName(),
                                                  nullptr,
                                                  nullptr,
                                                  nullptr};
                                ifacecellp->modp(ifacerefp->ifacep());
                                m_curSymp = m_modSymp
                                    = m_statep->ಸೇರಿಸು_ತುದಿ_Iface(ifacecellp, m_scope);
                                iterate(ifacecellp);
                                VL_DO_DANGLING(ತಳ್ಳು_Deletep(ifacecellp), ifacecellp);
                            }
                        }
                    }
                }
            }

            m_curSymp = m_modSymp = m_statep->ಸೇರಿಸು_ತುದಿ_Cell(modp, m_scope);
            { iterate(modp); }

            m_scope = "";
            m_curSymp = m_modSymp = nullptr;
        }
    }
    ಶೂನ್ಯ visit(AstTypeTable*) override {}  // FindVisitor::
    ಶೂನ್ಯ visit(AstIfaceRefDType* nodep) override {  // FindVisitor::
        ಯದಿ (!m_statep->forScopeCreation() && nodep->isVirtual() && nodep->ifacep()
            && !nodep->ifacep()->user3()) {
            m_virtIfaces.ತಳ್ಳು_back(nodep->ifacep());
            nodep->ifacep()->user3(ಸತ್ಯ);
        }
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Module* nodep) override {  // FindVisitor::
        // Called on top module from Netlist, other modules from the cell creating them,
        // and packages
        UINFO(8, "   " << nodep);
        // m_curSymp/m_modSymp maybe nullptr for packages and non-top modules
        // Packages will be under top after the initial phases, but until then
        // need separate handling
        ಧ್ರುವ ತಾರ್ಕಿಕ standalonePkg
            = !m_modSymp && !m_statep->forScopeCreation() && VN_IS(nodep, Package);
        ಧ್ರುವ ತಾರ್ಕಿಕ doit = (m_modSymp || standalonePkg);
        VL_RESTORER_COPY(m_scope);
        VL_RESTORER(m_classOrPackagep);
        VL_RESTORER(m_modSymp);
        VL_RESTORER(m_curSymp);
        VL_RESTORER(m_param_ಅಂಕೆ);
        VL_RESTORER(m_mod_Block_ಅಂಕೆ);
        VL_RESTORER(m_mod_With_ಅಂಕೆ);
        VL_RESTORER(m_mod_Arg_ಅಂಕೆ);
        ಯದಿ (doit && nodep->user2()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Identically recursive module (module instantiates "
                          "itself, without changing parameters): "
                              << Ast_ಗಂಟು::prettyNameQ(nodep->origName()));
        } ಅನ್ಯಥಾ ಯದಿ (doit) {
            UINFO(4, "     Link Module: " << nodep);
            UASSERT_OBJ(!nodep->dead(), nodep, "Module in instance tree mislabeled as dead?");
            AstPackage* ಧ್ರುವ pkgp = VN_CAST(nodep, Package);
            m_classOrPackagep = pkgp;
            ಯದಿ (standalonePkg) {
                ಯದಿ (pkgp->isDollarUnit()) {
                    m_curSymp = m_modSymp = m_statep->dunitEntp();
                    nodep->user1p(m_curSymp);
                } ಅನ್ಯಥಾ {
                    VSymEnt* ಧ್ರುವ upperSymp = m_statep->dunitEntp();
                    m_scope = nodep->name();
                    m_curSymp = m_modSymp = m_statep->ಸೇರಿಸು_Block(upperSymp, nodep->name(), nodep,
                                                                  m_classOrPackagep);
                    UINFO(9, "New module scope " << m_curSymp);
                }
            }
            //
            m_param_ಅಂಕೆ = 0;
            m_mod_Block_ಅಂಕೆ = 0;
            m_mod_With_ಅಂಕೆ = 0;
            m_mod_Arg_ಅಂಕೆ = 0;
            // m_modSymp/m_curSymp for non-packages set by AstCell above this module
            // Iterate
            nodep->user2(ಸತ್ಯ);
            iterateChildren(nodep);
            nodep->user2(ಅಸತ್ಯ);
            nodep->user4(ಸತ್ಯ);
            // Interfaces need another pass when signals are resolved. When creating
            // scopes this is done from the AstScopes in LinkDotScopeVisitor instead, as
            // only those entries hold the AstVarScopes the modport items resolve against.
            ಯದಿ (!m_statep->forScopeCreation()) {
                ಯದಿ (AstIface* ಧ್ರುವ ifacep = VN_CAST(nodep, Iface)) {
                    m_statep->ಸೇರಿಸು_Iface_Mod_Sym(ifacep, m_curSymp);
                }
            }
        } ಅನ್ಯಥಾ ಯದಿ (isHierBlockWrapper(nodep->name())) {
            UINFO(5, "Module is hierarchical block, must not be dead: " << nodep);
            m_scope = nodep->name();
            VSymEnt* ಧ್ರುವ upperSymp = m_curSymp ? m_curSymp : m_statep->ಬೇರು_Entp();
            m_curSymp = m_modSymp
                = m_statep->ಸೇರಿಸು_Block(upperSymp, nodep->name() + "::", nodep, m_classOrPackagep);
            iterateChildren(nodep);
            nodep->user4(ಸತ್ಯ);
        } ಅನ್ಯಥಾ {  // !doit
            ಯದಿ (nodep->hierParams()) {
                UINFO(1, "Found module with hier type parameters");
                m_hierParamsName = nodep->name();
                ಪ್ರತಿ (ಧ್ರುವ Ast_ಗಂಟು* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    ಯದಿ (ಧ್ರುವ AstTypedef* ಧ್ರುವ tdef = VN_CAST(stmtp, Typedef)) {
                        UINFO(1, "Inserting hier type parameter typedef: " << tdef);
                        VSymEnt* ಧ್ರುವ upperSymp = m_curSymp ? m_curSymp : m_statep->ಬೇರು_Entp();
                        m_curSymp = m_modSymp = m_statep->ಸೇರಿಸು_Block(upperSymp, nodep->name(),
                                                                      nodep, m_classOrPackagep);
                    }
                }
            } ಅನ್ಯಥಾ {
                // Will be optimized away later
                // Can't remove now, as our backwards iterator will throw up
                UINFO(5, "Module not under any CELL or top - dead module: " << nodep);
            }
        }
    }

    ಶೂನ್ಯ visit(AstClass* nodep) override {  // FindVisitor::
        UASSERT_OBJ(m_curSymp, nodep, "Class not under module/package/$unit");
        UINFO(8, "   " << nodep);
        VL_RESTORER_COPY(m_scope);
        VL_RESTORER(m_classOrPackagep);
        VL_RESTORER(m_modSymp);
        VL_RESTORER(m_curSymp);
        VL_RESTORER(m_param_ಅಂಕೆ);
        VL_RESTORER(m_mod_Block_ಅಂಕೆ);
        VL_RESTORER(m_mod_With_ಅಂಕೆ);
        VL_RESTORER(m_mod_Arg_ಅಂಕೆ);
        VL_RESTORER(m_explicitNew);
        {
            UINFO(4, "     Link Class: " << nodep);
            VSymEnt* ಧ್ರುವ upperSymp = m_curSymp;
            m_scope = m_scope + "." + nodep->name();
            m_classOrPackagep = nodep;
            m_curSymp = m_modSymp
                = m_statep->ಸೇರಿಸು_Block(upperSymp, nodep->name(), nodep, m_classOrPackagep);
            m_statep->ಸೇರಿಸು_Map(m_curSymp, m_scope);
            UINFO(9, "New module scope " << m_curSymp);
            //
            m_param_ಅಂಕೆ = 0;
            m_mod_Block_ಅಂಕೆ = 0;
            m_mod_With_ಅಂಕೆ = 0;
            m_mod_Arg_ಅಂಕೆ = 0;
            m_explicitNew = ಅಸತ್ಯ;
            // m_modSymp/m_curSymp for non-packages set by AstCell above this module
            // Iterate
            iterateChildren(nodep);
            nodep->user4(ಸತ್ಯ);
            // Implicit new needed?
            ಯದಿ (!m_explicitNew && m_statep->forPrimary()) makeImplicitNew(nodep);
        }
    }
    ಶೂನ್ಯ visit(AstClassOrPackageRef* nodep) override {  // FindVisitor::
        ಯದಿ (!nodep->classOrPackageNodep() && nodep->name() == "$unit") {
            nodep->classOrPackageNodep(v3Global.rootp()->dollarUnitPkgp());
        }
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstScope* nodep) override {  // FindVisitor::
        UASSERT_OBJ(m_statep->forScopeCreation(), nodep,
                    "Scopes should only exist right after V3Scope");
        // Detach on first encounter, will re add at the end.
        // Otherwise would be visited O(cells^2) time after V3Scope
        ಯದಿ (!nodep->is_ತುದಿ()) {
            ++m_statScopeVisits;
            nodep->unlinkFrBack();
            m_scopes.ತಳ್ಳು_back(nodep);
        }
    }
    ಶೂನ್ಯ visit(AstCell* nodep) override {  // FindVisitor::
        UINFO(5, "   CELL under " << m_scope << " is " << nodep);
        // Process XREFs/etc inside pins
        ಯದಿ (nodep->recursive() && m_inRecursion) ಪ್ರತ್ಯಾಗಮ;
        iterateChildren(nodep);
        // Recurse in, preserving state
        VL_RESTORER_COPY(m_scope);
        VL_RESTORER(m_modSymp);
        VL_RESTORER(m_curSymp);
        VL_RESTORER(m_param_ಅಂಕೆ);
        VL_RESTORER(m_inRecursion);
        // Where do we add it?
        VSymEnt* aboveSymp = m_curSymp;
        ಧ್ರುವ string origname = Ast_ಗಂಟು::dedotName(nodep->name());
        string::size_type pos = origname.rfind('.');
        ಯದಿ (pos != string::npos) {
            // Flattened, find what CellInline it should live under
            ಧ್ರುವ string scope = origname.substr(0, pos);
            string baddot;
            VSymEnt* okSymp;
            aboveSymp
                = m_statep->findDotted(nodep->fileline(), aboveSymp, scope, baddot, okSymp, ಅಸತ್ಯ);
            UASSERT_OBJ(aboveSymp, nodep,
                        "Can't find instance insertion point at "
                            << Ast_ಗಂಟು::prettyNameQ(baddot) << " in: " << nodep->prettyNameQ());
        }
        {
            m_scope = m_scope + "." + nodep->name();
            m_curSymp = m_modSymp = m_statep->ಸೇರಿಸು_Cell(aboveSymp, m_modSymp, nodep, m_scope);
            m_inRecursion = nodep->recursive();
            // We don't report NotFoundModule, as may be a unused module in a generate
            ಯದಿ (nodep->modp()) iterate(nodep->modp());
        }
    }
    ಶೂನ್ಯ visit(AstCellInline* nodep) override {  // FindVisitor::
        UINFO(5, "   CELLINLINE under " << m_scope << " is " << nodep);
        VSymEnt* aboveSymp = m_curSymp;
        std::string ident = nodep->name();
        // If baz__DOT__foo__DOT__bar, we need to find baz__DOT__foo and add bar to it.
        ಧ್ರುವ std::string::size_type pos = ident.rfind("__DOT__");
        ಯದಿ (pos != std::string::npos) {
            ಧ್ರುವ std::string dotted = ident.substr(0, pos);
            ident = ident.substr(pos + std::strlen("__DOT__"));
            std::string baddot;
            VSymEnt* okSymp;
            aboveSymp = m_statep->findDotted(nodep->fileline(), aboveSymp, dotted, baddot, okSymp,
                                             ಅಸತ್ಯ);
            UASSERT_OBJ(aboveSymp, nodep,
                        "Can't find cellinline insertion point at "
                            << Ast_ಗಂಟು::prettyNameQ(baddot) << " in: " << nodep->prettyNameQ());
        }
        m_statep->ಸೇರಿಸು_Inline(aboveSymp, m_modSymp, nodep, ident);
    }
    ಶೂನ್ಯ visit(AstDefParam* nodep) override {  // FindVisitor::
        nodep->user1p(m_curSymp);
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstRefDType* nodep) override {  // FindVisitor::
        nodep->user1p(m_curSymp);
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstGenBlock* nodep) override {  // FindVisitor::
        UINFO(5, "   " << nodep);
        ಯದಿ (nodep->name() == "" && nodep->unnamed()) {
            // Unnamed blocks are only important when they contain var
            // decls, so search for them. (Otherwise adding all the
            // unnamed#'s would just confuse tracing variables in
            // places such as tasks, where "task ...; begin ... end"
            // are common.
            ಪ್ರತಿ (Ast_ಗಂಟು* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
                ಯದಿ (VN_IS(itemp, Var) || VN_IS(itemp, Foreach)) {
                    std::string name;
                    ಧ್ರುವ std::string stepStr = m_statep->forPrimary()
                                                    ? ""
                                                    : std::to_string(m_statep->step_ಸಂಖ್ಯೆ()) + "_";
                    ಕುರು {
                        ++m_mod_Block_ಅಂಕೆ;
                        name = "unnamedblk" + stepStr + cvtToStr(m_mod_Block_ಅಂಕೆ);
                        // Increment again if earlier pass of V3LinkDot claimed this name
                    } ಯಾವತ್ (m_curSymp->findIdFlat(name));
                    nodep->name(name);
                    ಭಙ್ಗ;
                }
            }
        }
        ಯದಿ (nodep->name() == "") {
            iterateChildren(nodep);
        } ಅನ್ಯಥಾ {
            VL_RESTORER(m_curSymp);
            {
                m_curSymp
                    = m_statep->ಸೇರಿಸು_Block(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
                m_curSymp->fallbackp(VL_RESTORER_PREV(m_curSymp));
                // Iterate
                iterateChildren(nodep);
            }
        }
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Block* nodep) override {  // FindVisitor::
        UINFO(5, "   " << nodep);
        ಯದಿ (nodep->name() == "" && nodep->unnamed()) {
            // Unnamed blocks are only important when they contain var
            // decls, so search for them. (Otherwise adding all the
            // unnamed#'s would just confuse tracing variables in
            // places such as tasks, where "task ...; begin ... end"
            // are common.
            ಧ್ರುವ auto containsDecl = [](ಧ್ರುವ Ast_ಗಂಟು* nodesp) -> ತಾರ್ಕಿಕ {
                ಪ್ರತಿ (ಧ್ರುವ Ast_ಗಂಟು* np = nodesp; np; np = np->nextp()) {
                    ಯದಿ (VN_IS(np, Var) || VN_IS(np, Foreach)) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
                }
                ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            };
            ತಾರ್ಕಿಕ needName = nodep->declsp() || containsDecl(nodep->stmtsp());
            ಯದಿ (ಧ್ರುವ AstFork* ಧ್ರುವ forkp = VN_CAST(nodep, Fork)) {
                needName = needName || containsDecl(forkp->forksp());
            }
            ಯದಿ (needName) {
                ಧ್ರುವ std::string stepStr
                    = m_statep->forPrimary() ? "" : std::to_string(m_statep->step_ಸಂಖ್ಯೆ()) + "_";
                std::string name;
                ಕುರು {
                    ++m_mod_Block_ಅಂಕೆ;
                    name = "unnamedblk" + stepStr + cvtToStr(m_mod_Block_ಅಂಕೆ);
                    // Increment again if earlier pass of V3LinkDot claimed this name
                } ಯಾವತ್ (m_curSymp->findIdFlat(name));
                nodep->name(name);
            }
        }
        ಯದಿ (nodep->name() == "") {
            iterateChildren(nodep);
        } ಅನ್ಯಥಾ {
            VL_RESTORER(m_curSymp);
            {
                m_curSymp
                    = m_statep->ಸೇರಿಸು_Block(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
                m_curSymp->fallbackp(VL_RESTORER_PREV(m_curSymp));
                // Iterate
                iterateChildren(nodep);
            }
        }
    }
    // Handle out-of-block interface method definition (IEEE 1800-2023 25.7)
    // Move task body from module into the interface's prototype task,
    // rewriting port-prefixed references in the body.
    ಶೂನ್ಯ moveIfaceExportBody(Ast_ಗಂಟು_FTask* nodep) {
        ಧ್ರುವ string& portName = nodep->ifacePortName();
        UINFO(5, "   moveIfaceExportBody: port=" << portName << " task=" << nodep->name());
        // Look up the interface port variable in the module's symbol table
        VSymEnt* ಧ್ರುವ portSymp = m_modSymp->findIdFallback(portName);
        ಯದಿ (!portSymp) {
            nodep->v3error("Interface port not found for out-of-block definition: '"
                           << portName << "' (IEEE 1800-2023 25.7)");
            nodep->unlinkFrBack();
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }
        AstVar* ಧ್ರುವ portp = VN_CAST(portSymp->nodep(), Var);
        ಯದಿ (!portp) {
            nodep->v3fatalSrc("Out-of-block definition port is not a variable: " << portName);
            ಪ್ರತ್ಯಾಗಮ;
        }
        // Get the interface name from the port's type
        // At this stage dtypep() may not be set yet; check childDTypep() too
        AstIfaceRefDType* ifaceRefDtp = nullptr;
        ಯದಿ (portp->dtypep()) { ifaceRefDtp = VN_CAST(portp->dtypep(), IfaceRefDType); }
        ಯದಿ (!ifaceRefDtp && portp->childDTypep()) {
            ifaceRefDtp = VN_CAST(portp->childDTypep(), IfaceRefDType);
        }
        ಯದಿ (!ifaceRefDtp) {
            nodep->v3error("Out-of-block definition port is not an interface port: '"
                           << portName << "' (IEEE 1800-2023 25.7)");
            nodep->unlinkFrBack();
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }
        ಧ್ರುವ string ifaceName = ifaceRefDtp->ifaceName();
        UINFO(5, "   moveIfaceExportBody: iface=" << ifaceName);
        // Find the interface module by name
        Ast_ಗಂಟು_Module* ಧ್ರುವ ifaceModp = m_statep->findModuleSym(ifaceName);
        ಯದಿ (!ifaceModp) {
            nodep->v3fatalSrc("Interface not found for out-of-block definition: " << ifaceName);
            ಪ್ರತ್ಯಾಗಮ;
        }
        // Find the prototype task in the interface
        Ast_ಗಂಟು_FTask* protoTaskp = nullptr;
        ಪ್ರತಿ (Ast_ಗಂಟು* itemp = ifaceModp->stmtsp(); itemp; itemp = itemp->nextp()) {
            ಯದಿ (Ast_ಗಂಟು_FTask* ಧ್ರುವ ftaskp = VN_CAST(itemp, ಗಂಟು_FTask)) {
                ಯದಿ (ftaskp->name() == nodep->name() && ftaskp->prototype()) {
                    protoTaskp = ftaskp;
                    ಭಙ್ಗ;
                }
            }
        }
        ಯದಿ (!protoTaskp) {
            nodep->v3error("No matching export prototype found in interface "
                           << ifaceName << " for task " << nodep->prettyNameQ()
                           << " (IEEE 1800-2023 25.7)");
            nodep->unlinkFrBack();
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }
        // Move body statements (non-port declarations) from the out-of-block task
        // to the prototype task. Rewrite port.X references to just X in the body.
        ಪ್ರತಿ (Ast_ಗಂಟು* stmtp = nodep->stmtsp(); stmtp;) {
            Ast_ಗಂಟು* ಧ್ರುವ nextp = stmtp->nextp();
            // Skip port declarations -- they already exist in the prototype
            ಯದಿ (ಧ್ರುವ AstVar* ಧ್ರುವ varp = VN_CAST(stmtp, Var)) {
                ಯದಿ (varp->isIO()) {
                    stmtp = nextp;
                    ಅನುವರ್ತ;
                }
            }
            // Clone this statement and rewrite port references
            Ast_ಗಂಟು* ಧ್ರುವ newStmtp = stmtp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);
            rewriteIfacePortRefsSingle(newStmtp, portName);
            protoTaskp->addStmtsp(newStmtp);
            stmtp = nextp;
        }
        // Mark prototype as no longer prototype (it has a body now)
        protoTaskp->prototype(ಅಸತ್ಯ);
        // Delete the out-of-block task from the module
        nodep->unlinkFrBack();
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }

    // Rewrite AstDot nodes where lhsp is AstParseRef with the port name
    // to just the rhsp (stripping the port prefix).
    ಸ್ಥಿರ ಶೂನ್ಯ rewriteIfacePortRefsSingle(Ast_ಗಂಟು* nodep, ಧ್ರುವ string& portName) {
        ಯದಿ (!nodep) ಪ್ರತ್ಯಾಗಮ;
        // Check if this is a non-colon Dot with lhs matching portName
        ಯದಿ (AstDot* ಧ್ರುವ dotp = VN_CAST(nodep, Dot)) {
            ಯದಿ (!dotp->colon()) {
                ಧ್ರುವ AstParseRef* ಧ್ರುವ lhsRefp = VN_CAST(dotp->lhsp(), ParseRef);
                ಯದಿ (lhsRefp && lhsRefp->name() == portName) {
                    UINFO(5, "   rewriteIfacePortRefs: stripping port prefix from " << dotp);
                    Ast_ಗಂಟು* ಧ್ರುವ rhsp = dotp->rhsp()->unlinkFrBack();
                    dotp->replaceWith(rhsp);
                    VL_DO_DANGLING(dotp->delete_ವೃಕ್ಷ(), dotp);
                    rewriteIfacePortRefsSingle(rhsp, portName);
                    ಪ್ರತ್ಯಾಗಮ;
                }
            }
        }
        // Recurse into all children
        ಪ್ರತಿ (Ast_ಗಂಟು* childp : {nodep->op1p(), nodep->op2p(), nodep->op3p(), nodep->op4p()}) {
            ಯಾವತ್ (childp) {
                Ast_ಗಂಟು* ಧ್ರುವ nextp = childp->nextp();
                rewriteIfacePortRefsSingle(childp, portName);
                childp = nextp;
            }
        }
    }

    ಶೂನ್ಯ visit(Ast_ಗಂಟು_FTask* nodep) override {  // FindVisitor::
        // NodeTask: Remember its name for later resolution
        UINFO(5, "   " << nodep);
        UASSERT_OBJ(m_curSymp && m_modSymp, nodep, "Function/Task not under module?");
        ಯದಿ (nodep->name() == "new") m_explicitNew = ಸತ್ಯ;
        // Handle out-of-block interface method definition (IEEE 25.8)
        ಯದಿ (!nodep->ifacePortName().empty() && m_statep->forPrimary()) {
            moveIfaceExportBody(nodep);
            ಪ್ರತ್ಯಾಗಮ;  // Task has been deleted
        }
        // Remember the existing symbol table scope
        VL_RESTORER(m_classOrPackagep);
        VL_RESTORER(m_curSymp);
        VSymEnt* upSymp = m_curSymp;

        ಯದಿ (VN_IS(m_curSymp->nodep(), Class)) {
            ಯದಿ (VN_AS(m_curSymp->nodep(), Class)->isInterfaceClass() && !nodep->pureVirtual()
                && !nodep->isConstructor()) {
                nodep->v3error("Interface class functions must be pure virtual"
                               << " (IEEE 1800-2023 8.26): " << nodep->prettyNameQ());
            }
            ಯದಿ (m_statep->forPrimary()
                && (nodep->name() == "randomize" || nodep->name() == "srandom")) {
                nodep->v3error(nodep->prettyNameQ()
                               << " is a predefined class method; redefinition not allowed"
                                  " (IEEE 1800-2023 18.6.3)");
            }
        }
        // Change to appropriate package if extern declaration (vs definition)
        ಯದಿ (nodep->classOrPackagep()) {
            // class-in-class
            AstDot* ಧ್ರುವ dotp = VN_CAST(nodep->classOrPackagep(), Dot);
            AstClassOrPackageRef* ಧ್ರುವ cpackagerefp
                = VN_CAST(nodep->classOrPackagep(), ClassOrPackageRef);
            ಯದಿ (dotp) {
                AstClassOrPackageRef* ಧ್ರುವ lhsp = VN_AS(dotp->lhsp(), ClassOrPackageRef);
                m_statep->resolveClassOrPackage(m_curSymp, lhsp, ಸತ್ಯ, ಅಸತ್ಯ,
                                                "External definition :: reference");
                AstClass* ಧ್ರುವ lhsclassp = VN_CAST(lhsp->classOrPackageSkipp(), Class);
                ಯದಿ (!lhsclassp) {
                    nodep->v3error("Extern declaration's scope is not a defined class");
                } ಅನ್ಯಥಾ {
                    m_curSymp = m_statep->get_ಗಂಟು_Sym(lhsclassp);
                    upSymp = m_curSymp;
                    AstClassOrPackageRef* ಧ್ರುವ rhsp = VN_AS(dotp->rhsp(), ClassOrPackageRef);
                    m_statep->resolveClassOrPackage(m_curSymp, rhsp, ಸತ್ಯ, ಅಸತ್ಯ,
                                                    "External definition :: reference");
                    AstClass* ಧ್ರುವ rhsclassp = VN_CAST(rhsp->classOrPackageSkipp(), Class);
                    ಯದಿ (!rhsclassp) {
                        nodep->v3error("Extern declaration's scope is not a defined class");
                    } ಅನ್ಯಥಾ {
                        m_curSymp = m_statep->get_ಗಂಟು_Sym(rhsclassp);
                        upSymp = m_curSymp;
                        moveExternFuncDecl(nodep, rhsclassp);
                    }
                }
            } ಅನ್ಯಥಾ ಯದಿ (cpackagerefp) {
                ಯದಿ (!cpackagerefp->classOrPackageSkipp()) {
                    m_statep->resolveClassOrPackage(m_curSymp, cpackagerefp, ಸತ್ಯ, ಅಸತ್ಯ,
                                                    "External definition :: reference");
                }
                AstClass* ಧ್ರುವ classp = VN_CAST(cpackagerefp->classOrPackageSkipp(), Class);
                ಯದಿ (!classp) {
                    nodep->v3error("Extern declaration's scope is not a defined class");
                } ಅನ್ಯಥಾ {
                    m_curSymp = m_statep->get_ಗಂಟು_Sym(classp);
                    upSymp = m_curSymp;
                    moveExternFuncDecl(nodep, classp);
                }
            } ಅನ್ಯಥಾ {
                v3fatalSrc("Unhandled extern function definition package");
            }
        }
        // Set the class as package for iteration
        ಯದಿ (VN_IS(m_curSymp->nodep(), Class)) {
            m_classOrPackagep = VN_AS(m_curSymp->nodep(), Class);
        }
        // Create symbol table for the task's vars
        ಧ್ರುವ string name = (nodep->isExternProto() ? "extern "s : ""s) + nodep->name();
        m_curSymp = m_statep->ಸೇರಿಸು_Block(m_curSymp, name, nodep, m_classOrPackagep);
        m_curSymp->fallbackp(upSymp);
        // Convert the func's range to the output variable
        // This should probably be done in the Parser instead, as then we could
        // just attach normal signal attributes to it.
        ಯದಿ (!nodep->isExternProto() && nodep->fvarp() && !VN_IS(nodep->fvarp(), Var)) {
            Ast_ಗಂಟು_DType* dtypep = VN_CAST(nodep->fvarp(), ಗಂಟು_DType);
            // If unspecified, function returns one bit; however when we
            // support NEW() it could also return the class reference.
            ಯದಿ (dtypep) {
                dtypep->unlinkFrBack();
            } ಅನ್ಯಥಾ {
                dtypep = ನೂತನ AstBasicDType{nodep->fileline(), VBasicDTypeKwd::LOGIC};
            }
            AstVar* ಧ್ರುವ newvarp
                = ನೂತನ AstVar{nodep->fileline(), VVarType::VAR, nodep->name(), VFlagChildDType{},
                             dtypep};  // Not dtype resolved yet
            newvarp->direction(VDirection::OUTPUT);
            newvarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            newvarp->funcReturn(ಸತ್ಯ);
            newvarp->trace(ಅಸತ್ಯ);  // Not user visible
            nodep->fvarp(newvarp);
            // Explicit insert required, as the var name shadows the upper level's task name
            m_statep->ಸೇರಿಸು_Sym(m_curSymp, newvarp->name(), newvarp, nullptr /*classOrPackagep*/);
        }
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstClocking* nodep) override {  // FindVisitor::
        VL_RESTORER(m_clockingp);
        m_clockingp = nodep;
        iterate(nodep->sentreep());
        iterateAndNextNull(nodep->itemsp());
        // If the block has no name, one cannot reference the clockvars
        VSymEnt* itSymp = nullptr;
        ಯದಿ (nodep->isGlobal()  //
            && m_statep->forPrimary()) {  // else flattening may see two globals
            m_statep->checkDuplicate(m_curSymp, nodep, "__024global_clock");
            itSymp
                = m_statep->ಸೇರಿಸು_Block(m_curSymp, "__024global_clock", nodep, m_classOrPackagep);
            itSymp->fallbackp(nullptr);
        }
        ಯದಿ (!nodep->name().empty()) {
            itSymp = m_statep->ಸೇರಿಸು_Block(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
            itSymp->fallbackp(nullptr);
        }
        ಯದಿ (itSymp) {
            VL_RESTORER(m_curSymp);
            m_curSymp = itSymp;
            iterateAndNextNull(nodep->itemsp());
        }
    }
    ಶೂನ್ಯ visit(AstClockingItem* nodep) override {  // FindVisitor::
        ಯದಿ (nodep->varp()) {
            ಯದಿ (m_curSymp->nodep() == m_clockingp) iterate(nodep->varp());
            ಪ್ರತ್ಯಾಗಮ;
        }
        std::string varname;
        Ast_ಗಂಟು_DType* dtypep;
        ಯದಿ (AstAssign* ಧ್ರುವ assignp = nodep->assignp()) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rhsp = assignp->rhsp()->unlinkFrBack();
            dtypep = ನೂತನ AstRefDType{nodep->fileline(), AstRefDType::Flag_Type_Of_ಅಭಿವ್ಯಕ್ತಿ{},
                                     rhsp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ)};
            nodep->exprp(rhsp);
            varname = assignp->lhsp()->name();
            VL_DO_DANGLING(assignp->unlinkFrBack()->delete_ವೃಕ್ಷ(), assignp);
        } ಅನ್ಯಥಾ {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ refp = nodep->exprp();
            ಧ್ರುವ VSymEnt* foundp = m_curSymp->findIdFallback(refp->name());
            ಯದಿ (!foundp || !foundp->nodep()) {
                refp->v3error("Corresponding variable " << refp->prettyNameQ()
                                                        << " does not exist");
                VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ವೃಕ್ಷ(), nodep);
                ಪ್ರತ್ಯಾಗಮ;
            }
            varname = refp->name();
            dtypep = VN_AS(foundp->nodep(), Var)->childDTypep()->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);
        }
        AstVar* ಧ್ರುವ newvarp = ನೂತನ AstVar{nodep->fileline(), VVarType::MODULETEMP, varname,
                                           VFlagChildDType{}, dtypep};
        newvarp->lifetime(VLifetime::STATIC_EXPLICIT);
        nodep->varp(newvarp);
        iterate(nodep->exprp());
    }
    ಶೂನ್ಯ visit(AstConstraint* nodep) override {  // FindVisitor::
        VL_RESTORER(m_curSymp);
        // Change to appropriate package if extern declaration (vs definition)
        VSymEnt* upSymp = m_curSymp;
        ಯದಿ (nodep->classOrPackagep()) {
            AstClassOrPackageRef* ಧ್ರುವ cpackagerefp
                = VN_CAST(nodep->classOrPackagep(), ClassOrPackageRef);
            ಯದಿ (!cpackagerefp) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: extern constraint definition with class-in-class");
            } ಅನ್ಯಥಾ {
                ಯದಿ (!cpackagerefp->classOrPackageSkipp()) {
                    m_statep->resolveClassOrPackage(m_curSymp, cpackagerefp, ಸತ್ಯ, ಅಸತ್ಯ,
                                                    "External definition :: reference");
                }
                AstClass* ಧ್ರುವ classp = VN_CAST(cpackagerefp->classOrPackageSkipp(), Class);
                ಯದಿ (!classp) {
                    nodep->v3error("Extern declaration's scope is not a defined class");
                } ಅನ್ಯಥಾ {
                    m_curSymp = m_statep->get_ಗಂಟು_Sym(classp);
                    upSymp = m_curSymp;
                    // Move it to proper spot under the target class
                    nodep->unlinkFrBack();
                    classp->addStmtsp(nodep);
                    nodep->classOrPackagep()->unlinkFrBack()->delete_ವೃಕ್ಷ();
                }
            }
        }
        // Set the class as package for iteration
        ಧ್ರುವ string name = (nodep->isExternProto() ? "extern "s : ""s) + nodep->name();
        m_curSymp = m_statep->ಸೇರಿಸು_Block(upSymp, name, nodep, m_classOrPackagep);
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstVar* nodep) override {  // FindVisitor::
        // Var: Remember its name for later resolution
        UASSERT_OBJ(m_curSymp && m_modSymp, nodep, "Var not under module?");
        iterateChildren(nodep);
        ಯದಿ (VN_IS(m_curSymp->nodep(), Class)
            && VN_AS(m_curSymp->nodep(), Class)->isInterfaceClass() && !nodep->isParam()) {
            nodep->v3error("Interface class cannot contain non-parameter members"
                           << " (IEEE 1800-2023 8.26): " << nodep->prettyNameQ());
        }
        ಯದಿ (!m_statep->forScopeCreation()) {
            // Find under either a task or the module's vars
            ಧ್ರುವ VSymEnt* foundp = m_curSymp->findIdFallback(nodep->name());
            ಯದಿ (!foundp && m_modSymp && nodep->name() == m_modSymp->nodep()->name()) {
                foundp = m_modSymp;  // Conflicts with modname?
            }
            AstVar* ಧ್ರುವ findvarp = foundp ? VN_CAST(foundp->nodep(), Var) : nullptr;
            // clocking items can have duplicate names (inout)
            ಯದಿ (findvarp && VN_IS(findvarp->backp(), ClockingItem)
                && VN_IS(nodep->backp(), ClockingItem)) {
                AstClockingItem* ಧ್ರುವ itemp = VN_AS(nodep->backp(), ClockingItem);
                AstClockingItem* ಧ್ರುವ finditemp = VN_AS(findvarp->backp(), ClockingItem);
                UINFO(4, "ClockCompl: " << itemp << " ;; " << finditemp);
                UINFO(4, "ClockCompV: " << nodep << " ;; " << findvarp);
                ಯದಿ (*itemp->exprp()->fileline() == *finditemp->exprp()->fileline()) {
                    UASSERT_OBJ(finditemp->direction() == VDirection::ಒಳಹರಿವು
                                    && itemp->direction() == VDirection::OUTPUT,
                                itemp, "Input after output?");
                    // pretend nothing found and rename
                    foundp = nullptr;
                    nodep->name("__Voutput_" + nodep->name());
                    finditemp->outputp(itemp);
                }
            }
            ತಾರ್ಕಿಕ ins = ಅಸತ್ಯ;
            ಯದಿ (!foundp) {
                ins = ಸತ್ಯ;
            } ಅನ್ಯಥಾ ಯದಿ (!findvarp && m_curSymp->findIdFlat(nodep->name()) && !foundp->imported()) {
                nodep->v3error("Unsupported in C: Variable has same name as "
                               << LinkDotState::ಗಂಟು_Text_Type(foundp->nodep()) << ": "
                               << nodep->prettyNameQ());
            } ಅನ್ಯಥಾ ಯದಿ (findvarp != nodep) {
                UINFO(4, "DupVar: " << nodep << " ;; " << foundp->nodep());
                UINFO(4, "    found  cur=se" << cvtToHex(m_curSymp) << " ;; parent=se"
                                             << cvtToHex(foundp->parentp()));
                ಯದಿ (foundp->parentp() == m_curSymp  // Only when on same level
                    && !foundp->imported()) {  // and not from package
                    ಯದಿ (VN_IS(m_curSymp->nodep(), Clocking)) {
                        nodep->v3error("Multiple clockvars with the same name not allowed");
                        ಪ್ರತ್ಯಾಗಮ;
                    }
                    ಧ್ರುವ ತಾರ್ಕಿಕ nansiBad
                        = (((findvarp->isDeclTyped() || findvarp->isNet())
                            && (nodep->isDeclTyped() || nodep->isNet()))
                           || (findvarp->isIO() && nodep->isIO())  // e.g. !(output && output)
                           || findvarp->gotNansiType());  // e.g. int x && int x
                    ಧ್ರುವ ತಾರ್ಕಿಕ ansiBad
                        = findvarp->isAnsi() || nodep->isAnsi();  // dup illegal with ANSI
                    ಯದಿ (ansiBad || nansiBad) {
                        ತಾರ್ಕಿಕ ansiWarn = ansiBad && !nansiBad;
                        ಯದಿ (ansiWarn) {
                            ಸ್ಥಿರ ಪೂರ್ಣಾಙ್ಕ s_didAnsiWarn = ಅಸತ್ಯ;
                            ಯದಿ (s_didAnsiWarn++) ansiWarn = ಅಸತ್ಯ;
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
                    } ಅನ್ಯಥಾ {
                        findvarp->combineType(nodep);
                        findvarp->fileline()->modifyStateInherit(nodep->fileline());
                        findvarp->gotNansiType(ಸತ್ಯ);
                        UASSERT_OBJ(nodep->subDTypep(), nodep, "Var has no type");
                        ಯದಿ (nodep->subDTypep()->numeric().isSigned()
                            && !findvarp->subDTypep()->numeric().isSigned()) {
                            findvarp->subDTypep()->numeric(VSigning{ಸತ್ಯ});
                        }
                        Ast_ಗಂಟು_DType* ಧ್ರುವ varDtp = findvarp->subDTypep();
                        Ast_ಗಂಟು_DType* ಧ್ರುವ otherDtp = nodep->subDTypep();
                        ಯದಿ (varDtp
                            && (VN_IS(varDtp, BasicDType)
                                && VN_AS(varDtp, BasicDType)->implicit())) {
                            // Then have "input foo" and "real foo" so the
                            // dtype comes from the other side.
                            VL_DO_DANGLING(varDtp->unlinkFrBack()->delete_ವೃಕ್ಷ(), varDtp);
                            findvarp->childDTypep(otherDtp->unlinkFrBack());
                        } ಅನ್ಯಥಾ ಯದಿ (!m_statep->forScopeCreation() && otherDtp && varDtp
                                   && !(VN_IS(otherDtp, BasicDType)
                                        && VN_AS(otherDtp, BasicDType)->implicit())) {
                            // otherDtp and varDtp both non-nullptr and neither are implicit
                            // Can't compare dtypes now as might contain parameters,
                            // defer to V3Width
                            AstAttrOf* ಧ್ರುವ newp
                                = ನೂತನ AstAttrOf{otherDtp->fileline(), VAttrType::VAR_PORT_DTYPE,
                                                otherDtp->unlinkFrBack()};
                            findvarp->addAttrsp(newp);
                        }
                    }
                    VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ವೃಕ್ಷ(), nodep);
                } ಅನ್ಯಥಾ {
                    // User can disable the message at either point
                    ಯದಿ (!(m_ftaskp && m_ftaskp->dpiImport())
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
                    ins = ಸತ್ಯ;
                }
            }
            ಯದಿ (ins) {
                ಯದಿ (m_statep->forPrimary() && nodep->isGParam()
                    && VN_IS(m_modSymp->nodep(), Module)
                    && (m_statep->ಬೇರು_Entp()->nodep() == m_modSymp->parentp()->nodep())) {
                    // This is the toplevel module. Check for command line overwrites of parameters
                    // We first search if the parameter is overwritten and then replace it with a
                    // new value.
                    ಯದಿ (v3Global.opt.hasParameter(nodep->name())) {
                        ಧ್ರುವ string svalue = v3Global.opt.parameter(nodep->name());
                        ಯದಿ (AstConst* ಧ್ರುವ valuep
                            = AstConst::parseParamLiteral(nodep->fileline(), svalue)) {
                            UINFO(9, "       replace parameter " << nodep);
                            UINFO(9, "       with " << valuep);
                            ಯದಿ (nodep->valuep()) ತಳ್ಳು_Deletep(nodep->valuep()->unlinkFrBack());
                            nodep->valuep(valuep);
                            // Do not warn ENUMVALUE when overriding an enum param from the cli.
                            nodep->fileline()->warnOff(V3ErrorCode::ENUMVALUE, ಸತ್ಯ);
                        }
                    }
                }
                m_statep->ಸೇರಿಸು_Sym(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
                ಯದಿ (m_statep->forPrimary() && nodep->isGParam()) {
                    ++m_param_ಅಂಕೆ;
                    VSymEnt* ಧ್ರುವ symp
                        = m_statep->ಸೇರಿಸು_Sym(m_curSymp, "__paramNumber" + cvtToStr(m_param_ಅಂಕೆ),
                                              nodep, m_classOrPackagep);
                    symp->exported(ಅಸತ್ಯ);
                }
            }
        }
    }
    ಶೂನ್ಯ visit(AstTypedef* nodep) override {  // FindVisitor::
        UASSERT_OBJ(m_curSymp, nodep, "Typedef not under module/package/$unit");
        ಯದಿ (VN_IS(m_classOrPackagep, Class)) nodep->isUnderClass(ಸತ್ಯ);
        iterateChildren(nodep);
        m_statep->ಸೇರಿಸು_Sym(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
    }
    ಶೂನ್ಯ visit(AstTypedefFwd* nodep) override {  // FindVisitor::
        UASSERT_OBJ(m_curSymp, nodep, "Typedef not under module/package/$unit");
        iterateChildren(nodep);
        // No need to insert, only the real typedef matters, but need to track for errors
        nodep->user1p(m_curSymp);
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_UOr_Struct_DType* nodep) override {  // FindVisitor::
        UASSERT_OBJ(m_curSymp, nodep, "Struct/union dtype not under module/package/$unit");
        VL_RESTORER(m_curSymp);
        m_curSymp = m_statep->ಸೇರಿಸು_Block(m_curSymp, "__Vdtype" + cvtToStr(nodep->unique_ಅಂಕೆ()),
                                          nodep, m_classOrPackagep);
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstMemberDType* nodep) override {  // FindVisitor::
        iterateChildren(nodep);
        m_statep->ಸೇರಿಸು_Sym(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
    }
    ಶೂನ್ಯ visit(AstParamTypeDType* nodep) override {  // FindVisitor::
        UASSERT_OBJ(m_curSymp, nodep, "Parameter type not under module/package/$unit");

        // Replace missing param types with provided hierarchical type params.
        ಯದಿ (!m_hierParamsName.empty()) {
            ಯದಿ (ಧ್ರುವ VSymEnt* ಧ್ರುವ typedefEntp = m_curSymp->findIdFallback(m_hierParamsName)) {
                ಧ್ರುವ AstModule* modp = VN_CAST(typedefEntp->nodep(), Module);

                ಪ್ರತಿ (ಧ್ರುವ Ast_ಗಂಟು* stmtp = modp ? modp->stmtsp() : nullptr; stmtp;
                     stmtp = stmtp->nextp()) {
                    ಧ್ರುವ AstTypedef* tdefp = VN_CAST(stmtp, Typedef);

                    ಯದಿ (tdefp && tdefp->name() == nodep->name() && m_statep->forPrimary()) {
                        UINFO(8, "Replacing type of" << nodep << "  -->with " << tdefp);
                        Ast_ಗಂಟು_DType* ಧ್ರುವ newDtp = tdefp->childDTypep();
                        Ast_ಗಂಟು_DType* ಧ್ರುವ oldDtp = nodep->childDTypep();

                        oldDtp->replaceWith(newDtp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ));
                        VL_DO_DANGLING(oldDtp->delete_ವೃಕ್ಷ(), oldDtp);
                    }
                }
            }
        }

        iterateChildren(nodep);
        m_statep->ಸೇರಿಸು_Sym(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
        ಯದಿ (m_statep->forPrimary() && nodep->isGParam()) {
            ++m_param_ಅಂಕೆ;
            VSymEnt* ಧ್ರುವ symp = m_statep->ಸೇರಿಸು_Sym(
                m_curSymp, "__paramNumber" + cvtToStr(m_param_ಅಂಕೆ), nodep, m_classOrPackagep);
            symp->exported(ಅಸತ್ಯ);
        }
    }
    ಶೂನ್ಯ visit(AstCFunc* nodep) override {  // FindVisitor::
        // For dotted resolution, ignore all AstVars under functions, otherwise shouldn't exist
        UASSERT_OBJ(!m_statep->forScopeCreation(), nodep, "No CFuncs expected in tree yet");
    }
    ಶೂನ್ಯ visit(AstEnumItem* nodep) override {  // FindVisitor::
        // EnumItem: Remember its name for later resolution
        iterateChildren(nodep);
        // Find under either a task or the module's vars
        ಧ್ರುವ VSymEnt* foundp = m_curSymp->findIdFallback(nodep->name());
        ಯದಿ (!foundp && m_modSymp && nodep->name() == m_modSymp->nodep()->name()) {
            foundp = m_modSymp;  // Conflicts with modname?
        }
        AstEnumItem* ಧ್ರುವ findvarp = foundp ? VN_CAST(foundp->nodep(), EnumItem) : nullptr;
        ತಾರ್ಕಿಕ ins = ಅಸತ್ಯ;
        ಯದಿ (!foundp) {
            ins = ಸತ್ಯ;
        } ಅನ್ಯಥಾ ಯದಿ (findvarp != nodep) {
            UINFO(4, "DupVar: " << nodep << " ;; " << foundp);
            ಯದಿ (foundp->parentp() == m_curSymp  // Only when on same level
                && !foundp->imported()) {  // and not from package
                nodep->v3error("Duplicate declaration of enum value: "
                               << nodep->prettyName() << '\n'
                               << nodep->warnContextPrimary() << '\n'
                               << foundp->nodep()->warnOther()
                               << "... Location of original declaration\n"
                               << foundp->nodep()->warnContextSecondary());
            } ಅನ್ಯಥಾ {
                // User can disable the message at either point
                ಯದಿ (!nodep->fileline()->warnIsOff(V3ErrorCode::VARHIDDEN)
                    && !foundp->nodep()->fileline()->warnIsOff(V3ErrorCode::VARHIDDEN)) {
                    nodep->v3warn(VARHIDDEN,
                                  "Declaration of enum value hides declaration in upper scope: "
                                      << nodep->prettyName() << '\n'
                                      << nodep->warnContextPrimary() << '\n'
                                      << foundp->nodep()->warnOther()
                                      << "... Location of original declaration\n"
                                      << foundp->nodep()->warnContextSecondary());
                }
                ins = ಸತ್ಯ;
            }
        }
        ಯದಿ (ins) m_statep->ಸೇರಿಸು_Sym(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
    }
    ಶೂನ್ಯ visit(AstPackageImport* nodep) override {  // FindVisitor::
        UINFO(4, "  Link: " << nodep);
        ಯದಿ (!nodep->packagep()) ಪ್ರತ್ಯಾಗಮ;  // Errored in V3LinkCells
        VSymEnt* ಧ್ರುವ srcp = m_statep->get_ಗಂಟು_Sym(nodep->packagep());
        ಯದಿ (nodep->name() == "*") {
            ಯದಿ (nodep->packagep() != v3Global.rootp()->stdPackagep()) {
                ಯದಿ (m_curSymp == m_statep->dunitEntp()) {
                    nodep->v3warn(IMPORTSTAR,
                                  "'import::*' in $unit scope may pollute global namespace");
                }
            }
        } ಅನ್ಯಥಾ ಯದಿ (!nodep->resolvedClassp()) {
            VSymEnt* ಧ್ರುವ impp = srcp->findIdFlat(nodep->name());
            ಯದಿ (!impp) {
                nodep->v3error("Import object not found: " << nodep->prettyPkgNameQ());
            } ಅನ್ಯಥಾ ಯದಿ (nodep->resolvedClassp()) {
                UASSERT_OBJ(nodep->resolvedClassp()->classOrPackageSkipp() == impp->nodep(), nodep,
                            "Import should resolve to the same node");
            } ಅನ್ಯಥಾ ಯದಿ (AstClass* ಧ್ರುವ classp = VN_CAST(impp->nodep(), Class)) {
                nodep->resolvedClassp(
                    ನೂತನ AstClassOrPackageRef{nodep->fileline(), classp->name(), classp, nullptr});
            }
        }
        m_curSymp->importFromPackage(m_statep->symsp(), srcp, nodep->name());
        UINFO(9, "    Link Done: " << nodep);
        // No longer needed, but can't delete until any multi-instantiated modules are expanded
    }
    ಶೂನ್ಯ visit(AstPackageExport* nodep) override {  // FindVisitor::
        UINFO(9, "  Link: " << nodep);
        ಯದಿ (!nodep->packagep()) ಪ್ರತ್ಯಾಗಮ;  // Errored in V3LinkCells
        VSymEnt* ಧ್ರುವ srcp = m_statep->get_ಗಂಟು_Sym(nodep->packagep());
        ಯದಿ (nodep->name() != "*") {
            VSymEnt* ಧ್ರುವ impp = srcp->findIdFlat(nodep->name());
            ಯದಿ (!impp) {
                nodep->v3error("Export object not found: '" << nodep->packagep()->prettyName()
                                                            << "::" << nodep->prettyName() << "'");
            }
        }
        m_curSymp->exportFromPackage(m_statep->symsp(), srcp, nodep->name());
        UINFO(9, "    Link Done: " << nodep);
        // No longer needed, but can't delete until any multi-instantiated modules are expanded
    }
    ಶೂನ್ಯ visit(AstPackageExportStarStar* nodep) override {  // FindVisitor::
        UINFO(4, "  Link: " << nodep);
        m_curSymp->exportStarStar(m_statep->symsp());
        // No longer needed, but can't delete until any multi-instantiated modules are expanded
    }

    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Foreach* nodep) override {  // FindVisitor::
        // Symbol table needs nodep->name() as the index variable's name
        VL_RESTORER(m_curSymp);
        {
            ++m_mod_With_ಅಂಕೆ;
            m_curSymp = m_statep->ಸೇರಿಸು_Block(m_curSymp, "__Vforeach" + cvtToStr(m_mod_With_ಅಂಕೆ),
                                              nodep, m_classOrPackagep);
            m_curSymp->fallbackp(VL_RESTORER_PREV(m_curSymp));
            ಪ್ರತಿ (Ast_ಗಂಟು *nextp, *argp = nodep->headerp()->elementsp(); argp; argp = nextp) {
                nextp = argp->nextp();
                AstVar* argrefp = nullptr;
                ಯದಿ (AstParseRef* ಧ್ರುವ parserefp = VN_CAST(argp, ParseRef)) {
                    // IEEE 1800-2023 12.7.3: foreach loop variable type shall be int (2-state)
                    // This might get changed in V3Width when types resolve (e.g., for assoc
                    // arrays)
                    argrefp = ನೂತನ AstVar{parserefp->fileline(), VVarType::BLOCKTEMP,
                                         parserefp->name(), argp->findIntDType()};
                    argrefp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                    parserefp->replaceWith(argrefp);
                    VL_DO_DANGLING2(parserefp->delete_ವೃಕ್ಷ(), parserefp, argp);
                    // Insert argref's name into symbol table
                    m_statep->ಸೇರಿಸು_Sym(m_curSymp, argrefp->name(), argrefp, nullptr);
                } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ auto largrefp = VN_CAST(argp, Var)) {
                    argrefp = largrefp;
                    // Insert argref's name into symbol table
                    m_statep->ಸೇರಿಸು_Sym(m_curSymp, argrefp->name(), argrefp, nullptr);
                } ಅನ್ಯಥಾ ಯದಿ (VN_IS(argp, Empty)) {
                } ಅನ್ಯಥಾ {
                    argp->v3error("'foreach' loop variable expects simple variable name");
                }
            }
            iterateChildren(nodep);
        }
    }

    ಶೂನ್ಯ visit(AstRandSequence* nodep) override {
        VL_RESTORER(m_curSymp);
        m_curSymp = m_statep->ಸೇರಿಸು_Block(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstRSProd* nodep) override {
        VL_RESTORER(m_curSymp);
        m_curSymp = m_statep->ಸೇರಿಸು_Block(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
        ಯದಿ (nodep->fvarp())
            nodep->fvarp()->v3warn(E_UNSUPPORTED,
                                   "Unsupported: randsequence production function variable");
        // Mark formal ports as port-checked so the primary resolve pass does not
        // flag them with "does not appear in port list" -- V3RandSequence will
        // later move them onto the generated task as real input ports.
        ಪ್ರತಿ (Ast_ಗಂಟು* itp = nodep->portsp(); itp; itp = itp->nextp()) {
            VN_AS(itp, Var)->user4(ಸತ್ಯ);
        }
        iterateChildren(nodep);
    }

    ಶೂನ್ಯ visit(AstWithParse* nodep) override {  // FindVisitor::
        // Change WITHPARSE(FUNCREF, equation) to FUNCREF(WITH(equation))
        Ast_ಗಂಟು_FTask_Ref* funcrefp = VN_CAST(nodep->funcrefp(), ಗಂಟು_FTask_Ref);
        ಯದಿ (ಧ್ರುವ AstDot* ಧ್ರುವ dotp = VN_CAST(nodep->funcrefp(), Dot))
            funcrefp = VN_CAST(dotp->rhsp(), ಗಂಟು_FTask_Ref);
        UASSERT_OBJ(funcrefp, nodep, "'with' only can operate on a function/task");
        string name = funcrefp->name() == "randomize" ? "__Vrandwith_obj" : "item";
        File_ಸಾಲು* argFl = nodep->fileline();
        AstArg* ಧ್ರುವ argsp = funcrefp->argsp();
        ಯದಿ (argsp) {
            argsp->unlinkFrBackWithNext();
            ಯದಿ (funcrefp->name() != "randomize") {
                ಯದಿ (ಧ್ರುವ auto parserefp = VN_CAST(argsp->exprp(), ParseRef)) {
                    name = parserefp->name();
                    argFl = parserefp->fileline();
                } ಅನ್ಯಥಾ {
                    argsp->v3error("'with' function expects simple variable name");
                }
                ಯದಿ (argsp->nextp()) {
                    argsp->nextp()->v3error("'with' function expects only up to one argument");
                }
                VL_DO_DANGLING(argsp->delete_ವೃಕ್ಷ(), argsp);
            }
        }
        // Type depends on the method used, let V3Width figure it out later
        // 'with (...) { ... }' constraint_block form is randomize-only
        // (IEEE 1800-2023 7.12 vs. 18.7); array methods take 'with (expr)'.
        ಯದಿ (nodep->restricted() && funcrefp->name() != "randomize") {
            nodep->v3error("'with (...) { ... }' constraint block is only valid for"
                           " randomize() (IEEE 1800-2023 18.7)");
            funcrefp->addArgsp(argsp);
            nodep->replaceWith(nodep->funcrefp()->unlinkFrBack());
            VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }
        ಧ್ರುವ ತಾರ್ಕಿಕ restrictedRandomize = nodep->restricted();
        ಯದಿ (nodep->exprsp() || nodep->constraintsp()
            || restrictedRandomize) {  // Else empty expression and pretend no "with"
            AstLambdaArgRef* ಧ್ರುವ indexArgRefp
                = ನೂತನ AstLambdaArgRef{argFl, name + "__DOT__index", ಸತ್ಯ};
            AstLambdaArgRef* ಧ್ರುವ ಮೌಲ್ಯ_Arg_Refp = ನೂತನ AstLambdaArgRef{argFl, name, ಅಸತ್ಯ};
            AstWith* ಧ್ರುವ newp
                = ನೂತನ AstWith{nodep->fileline(), indexArgRefp, ಮೌಲ್ಯ_Arg_Refp, nullptr};
            // Harvest the identifier_list into AstWith; grammar guarantees AstParseRef.
            ಯದಿ (restrictedRandomize) {
                newp->restricted(ಸತ್ಯ);
                ಯಾವತ್ (Ast_ಗಂಟು* ಧ್ರುವ itemp = nodep->exprsp()) {
                    newp->addRestrictedName(VN_AS(itemp, ParseRef)->name());
                    itemp->unlinkFrBack();
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(itemp), itemp);
                }
            }
            Ast_ಗಂಟು* ಅಭಿವ್ಯಕ್ತಿ_Or_Constraintsp = nullptr;
            ಯದಿ (nodep->exprsp()) ಅಭಿವ್ಯಕ್ತಿ_Or_Constraintsp = nodep->exprsp()->unlinkFrBackWithNext();
            ಯದಿ (nodep->constraintsp())
                ಅಭಿವ್ಯಕ್ತಿ_Or_Constraintsp = Ast_ಗಂಟು::addNext(
                    ಅಭಿವ್ಯಕ್ತಿ_Or_Constraintsp, nodep->constraintsp()->unlinkFrBackWithNext());
            newp->addExprp(ಅಭಿವ್ಯಕ್ತಿ_Or_Constraintsp);  // addExprp() tolerates nullptr
            funcrefp->withp(newp);
        }
        funcrefp->addArgsp(argsp);
        nodep->replaceWith(nodep->funcrefp()->unlinkFrBack());
        VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
    }
    ಶೂನ್ಯ visit(AstWith* nodep) override {  // FindVisitor::
        // Symbol table needs nodep->name() as the index variable's name
        // Iteration will pickup the AstVar we made under AstWith
        VL_RESTORER(m_curSymp);
        ++m_mod_With_ಅಂಕೆ;
        m_curSymp = m_statep->ಸೇರಿಸು_Block(m_curSymp, "__Vwith" + cvtToStr(m_mod_With_ಅಂಕೆ), nodep,
                                          m_classOrPackagep);
        m_curSymp->fallbackp(VL_RESTORER_PREV(m_curSymp));
        UASSERT_OBJ(nodep->indexArgRefp(), nodep, "Missing lambda argref");
        UASSERT_OBJ(nodep->ಮೌಲ್ಯ_Arg_Refp(), nodep, "Missing lambda argref");
        // Insert argref's name into symbol table
        m_statep->ಸೇರಿಸು_Sym(m_curSymp, nodep->ಮೌಲ್ಯ_Arg_Refp()->name(), nodep->ಮೌಲ್ಯ_Arg_Refp(),
                            nullptr);
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstCoverWith* nodep) override {  // FindVisitor::
        // The range list is outside the scope of the filter, which declares 'item'
        iterateAndNextNull(nodep->subp());
        VL_RESTORER(m_curSymp);
        ++m_mod_With_ಅಂಕೆ;
        m_curSymp = m_statep->ಸೇರಿಸು_Block(m_curSymp, "__VcoverWith" + cvtToStr(m_mod_With_ಅಂಕೆ),
                                          nodep, m_classOrPackagep);
        m_curSymp->fallbackp(VL_RESTORER_PREV(m_curSymp));
        iterateAndNextNull(nodep->itemp());
        iterateAndNextNull(nodep->filterp());
    }
    ಶೂನ್ಯ visit(AstCoverCrossWith* nodep) override {  // FindVisitor::
        // The selection, the 'matches' count, and the cross items' types, of their coverpoints'
        // expressions with any nested scopes, are outside the filter's scope, which declares the
        // items.  As visit(AstVar) would resolve a type in the scope declaring its variable, the
        // items, generated and unique, are inserted directly.
        iterateAndNextNull(nodep->selectp());
        iterateAndNextNull(nodep->countp());
        ಪ್ರತಿ (AstVar* varp = nodep->varsp(); varp; varp = VN_AS(varp->nextp(), Var)) {
            iterateAndNextNull(varp->childDTypep());
        }
        VL_RESTORER(m_curSymp);
        ++m_mod_With_ಅಂಕೆ;
        m_curSymp = m_statep->ಸೇರಿಸು_Block(m_curSymp, "__VcoverWith" + cvtToStr(m_mod_With_ಅಂಕೆ),
                                          nodep, m_classOrPackagep);
        m_curSymp->fallbackp(VL_RESTORER_PREV(m_curSymp));
        ಪ್ರತಿ (AstVar* varp = nodep->varsp(); varp; varp = VN_AS(varp->nextp(), Var)) {
            m_statep->ಸೇರಿಸು_Sym(m_curSymp, varp->name(), varp, m_classOrPackagep);
        }
        iterateAndNextNull(nodep->filterp());
    }

    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }  // FindVisitor::

    ಶೂನ್ಯ handleUnvisitedVirtIfaces() {
        Ast_ಗಂಟು_Module* ಧ್ರುವ ತುದಿ = v3Global.rootp()->modulesp();
        m_curSymp = m_statep->get_ಗಂಟು_Sym(ತುದಿ);
        ಪ್ರತಿ (AstIface* ifacep : m_virtIfaces) {
            ಯದಿ (!ifacep->user4()) {  // virtual interface never assigned any actual interface
                // Create dummy cell to keep the virtual interface alive
                // (later stages assume that non-dead interface has associated cell)
                AstCell* ಧ್ರುವ ifacecellp = ನೂತನ AstCell{ifacep->fileline(),
                                                        ifacep->fileline(),
                                                        "__VdummyInstOf" + ifacep->name(),
                                                        ifacep->name(),
                                                        nullptr,
                                                        nullptr,
                                                        nullptr};
                ifacecellp->modp(ifacep);
                ತುದಿ->addStmtsp(ifacecellp);

                iterate(ifacecellp);
            }
        }
    }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    LinkDotFindVisitor(AstNetlist* rootp, LinkDotState* statep)
        : m_statep{statep} {
        UINFO(4, __FUNCTION__ << ": ");
        iterate(rootp);

        ಯದಿ (!m_virtIfaces.empty()) handleUnvisitedVirtIfaces();
        ಪ್ರತಿ (AstScope* ಧ್ರುವ scopep : m_scopes) scopep->modp()->addStmtsp(scopep);
        ಯದಿ (m_statep->forScopeCreation()) {
            V3Stats::addStat("LinkDot, Scope visits", m_statScopeVisits);
        }
    }
    ~LinkDotFindVisitor() override = ಸ್ವಭಾವ;
};

//======================================================================

ವರ್ಗ LinkDotFindIfaceVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // NODE STATE
    //  *::user1p()             -> See LinkDotState

    // STATE - for current visit position (use VL_RESTORER)
    LinkDotState* ಧ್ರುವ m_statep;  // State to pass between visitors, including symbol table
    Ast_ಗಂಟು* m_declp = nullptr;  // Current declaring object that may soon contain IfaceRefDType

    // METHODS
    ಧ್ರುವ VSymEnt* findNonDeclSym(VSymEnt* symp, ಧ್ರುವ string& name) {
        // Find if there is a symbol of given name, ignoring the node that is declaring
        // it (m_declp) itself.  Thus if searching for "ifc" (an interface):
        //
        // module x;  // Finishes here and finds the typedef
        //   typedef foo ifc;
        //   ifc ifc;   // symp starts pointing here, but matches m_declp
        ಯಾವತ್ (symp) {
            ಧ್ರುವ VSymEnt* ಧ್ರುವ foundp = symp->findIdFlat(name);
            ಯದಿ (foundp && foundp->nodep() != m_declp) ಪ್ರತ್ಯಾಗಮ foundp;
            symp = symp->fallbackp();
        }
        ಪ್ರತ್ಯಾಗಮ nullptr;
    }

    // VISITORS
    ಶೂನ್ಯ visit(AstRefDType* nodep) override {  // FindIfaceVisitor::
        ಯದಿ (m_statep->forPrimary() && !nodep->classOrPackagep()) {
            UINFO(9, "  FindIfc: " << nodep);
            // If under a var, ignore the var itself as might be e.g. "intf intf;"
            // Critical tests:
            //   t_interface_param_genblk.v  // Checks this does make interface
            //   t_interface_hidden.v  // Checks this doesn't making interface when hidden
            ಯದಿ (m_statep->exists_ಗಂಟು_Sym(nodep)) {
                VSymEnt* symp = m_statep->get_ಗಂಟು_Sym(nodep);
                ಧ್ರುವ VSymEnt* foundp = findNonDeclSym(symp, nodep->name());
                Ast_ಗಂಟು* foundNodep = nullptr;
                // This: v4make test_regress/t/t_interface_param_genblk.py --debug
                // --debugi-V3LinkDot 9 Passes with this commented out:
                ಯದಿ (foundp) foundNodep = foundp->nodep();
                ಯದಿ (!foundNodep) foundNodep = m_statep->findModuleSym(nodep->name());
                ಯದಿ (foundNodep) UINFO(9, "    Ifc foundNodep " << foundNodep);
                ಯದಿ (AstIface* ಧ್ರುವ defp = VN_CAST(foundNodep, Iface)) {
                    // Must be found as module name, and not hidden/ by normal symbol (foundp)
                    AstIfaceRefDType* ಧ್ರುವ newp
                        = ನೂತನ AstIfaceRefDType{nodep->fileline(), "", nodep->name()};
                    ಯದಿ (nodep->paramsp())
                        newp->addParamsp(nodep->paramsp()->unlinkFrBackWithNext());
                    newp->ifacep(defp);
                    newp->user1u(nodep->user1u());
                    UINFO(9, "    Resolved interface " << nodep << "  =>  " << defp);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                    ಪ್ರತ್ಯಾಗಮ;
                }
            }
        }
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstVar* nodep) override {  // FindIfaceVisitor::
        VL_RESTORER(m_declp);
        m_declp = nodep;
        iterateChildren(nodep);
        AstIfaceRefDType* ಧ್ರುವ ifacerefp = LinkDotState::ifaceRefFromArray(nodep->subDTypep());
        ಯದಿ (ifacerefp && m_statep->exists_ಗಂಟು_Sym(nodep)) {
            // Can't resolve until interfaces and modport names are
            // known; see notes at top
            UINFO(9, "  FindIfc Var IfaceRef " << ifacerefp);
            ಯದಿ (!ifacerefp->isVirtual()) nodep->setIfaceRef();
            VSymEnt* ಧ್ರುವ symp = m_statep->get_ಗಂಟು_Sym(nodep);
            m_statep->ಸೇರಿಸು_Iface_Var_Sym(symp);
        }
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }  // FindIfaceVisitor::

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    LinkDotFindIfaceVisitor(AstNetlist* rootp, LinkDotState* statep)
        : m_statep{statep} {
        UINFO(4, __FUNCTION__ << ": ");
        iterate(rootp);
    }
    ~LinkDotFindIfaceVisitor() override = ಸ್ವಭಾವ;
};

//======================================================================

ವರ್ಗ LinkDotParamVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // NODE STATE
    //  *::user1p()             -> See LinkDotState
    //  *::user2p()             -> See LinkDotState
    //  *::user4()              -> See LinkDotState

    // STATE
    LinkDotState* ಧ್ರುವ m_statep;  // State to pass between visitors, including symbol table
    Ast_ಗಂಟು_Module* m_modp = nullptr;  // Current module

    ಶೂನ್ಯ pin_Implicit_ಅಭಿವ್ಯಕ್ತಿ_Recurse(Ast_ಗಂಟು* nodep) {
        // Under a pin, Check interconnect expression for a pin reference or a concat.
        // Create implicit variable as needed
        ಯದಿ (VN_IS(nodep, Dot)) {  // Not creating a simple implied type,
            // and implying something else would just confuse later errors
        } ಅನ್ಯಥಾ ಯದಿ (VN_IS(nodep, VarRef) || VN_IS(nodep, ParseRef)) {
            // To prevent user errors, we should only do single bit
            // implicit vars, however some netlists (MIPS) expect single
            // bit implicit wires to get created with range 0:0 etc.
            m_statep->implicitOkAdd(m_modp, nodep->name());
        }
        // These are perhaps a little too generous, as a SELect of siga[sigb]
        // perhaps shouldn't create an implicit variable.  But, we've warned...
        ಅನ್ಯಥಾ {
            ಯದಿ (Ast_ಗಂಟು* ಧ್ರುವ refp = nodep->op1p()) pin_Implicit_ಅಭಿವ್ಯಕ್ತಿ_Recurse(refp);
            ಯದಿ (Ast_ಗಂಟು* ಧ್ರುವ refp = nodep->op2p()) pin_Implicit_ಅಭಿವ್ಯಕ್ತಿ_Recurse(refp);
            ಯದಿ (Ast_ಗಂಟು* ಧ್ರುವ refp = nodep->op3p()) pin_Implicit_ಅಭಿವ್ಯಕ್ತಿ_Recurse(refp);
            ಯದಿ (Ast_ಗಂಟು* ಧ್ರುವ refp = nodep->op4p()) pin_Implicit_ಅಭಿವ್ಯಕ್ತಿ_Recurse(refp);
        }
        // Continue along a list (e.g. the terminals under AstImplicit), also after a reference
        ಯದಿ (Ast_ಗಂಟು* ಧ್ರುವ refp = nodep->nextp()) pin_Implicit_ಅಭಿವ್ಯಕ್ತಿ_Recurse(refp);
    }

    // VISITORS
    ಶೂನ್ಯ visit(AstTypeTable*) override {}  // ParamVisitor::
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Module* nodep) override {  // ParamVisitor::
        UINFO(5, "   " << nodep);
        ಯದಿ ((nodep->dead() || !nodep->user4()) && !nodep->hierParams()) {
            UINFO(4, "Mark dead module " << nodep);
            UASSERT_OBJ(!m_statep->forScopeCreation(), nodep,
                        "Dead module persisted past where should have removed");
            // Don't remove now, because we may have a tree of
            // parameterized modules with VARXREFs into the deleted module
            // region.  V3Dead should cleanup.
            // Downstream visitors up until V3Dead need to check for nodep->dead.
            nodep->dead(ಸತ್ಯ);
            ಪ್ರತ್ಯಾಗಮ;
        }
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstPin* nodep) override {  // ParamVisitor::
        // Pin: Link to submodule's port
        // Deal with implicit definitions - do before Resolve visitor as may
        // be referenced above declaration
        ಯದಿ (nodep->exprp()  // Else specifically unconnected pin
            && !nodep->svImplicit()) {  // SV 19.11.3: .name pins don't allow implicit decls
            pin_Implicit_ಅಭಿವ್ಯಕ್ತಿ_Recurse(nodep->exprp());
        }
    }
    ಶೂನ್ಯ visit(AstDefParam* nodep) override {  // ParamVisitor::
        iterateChildren(nodep);
        nodep->v3warn(DEFPARAM, "defparam is deprecated (IEEE 1800-2023 C.4.1)\n"
                                    << nodep->warnMore()
                                    << "... Suggest use instantiation with #(."
                                    << nodep->prettyName() << "(...etc...))");
        ತಾರ್ಕಿಕ hasPartSelect = ಅಸತ್ಯ;
        ಧ್ರುವ string path = extractDottedPath(nodep->pathp(), hasPartSelect);
        UASSERT_OBJ(!hasPartSelect && !path.empty(), nodep, "Unexpected defparam path shape");
        string baddot;
        VSymEnt* okSymp = nullptr;
        VSymEnt* ಧ್ರುವ foundp = m_statep->findDotted(
            nodep->fileline(), m_statep->get_ಗಂಟು_Sym(nodep), path, baddot, okSymp, ಸತ್ಯ);
        AstCell* ಧ್ರುವ cellp = foundp ? VN_AS(foundp->nodep(), Cell) : nullptr;
        ಯದಿ (!cellp) {
            nodep->v3error("In defparam, instance " << path << " never declared");
        } ಅನ್ಯಥಾ {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ exprp = nodep->rhsp()->unlinkFrBack();
            UINFO(9, "Defparam cell " << path << "." << nodep->name() << " attach-to " << cellp
                                      << "  <= " << exprp);
            // Don't need to check the name of the defparam exists.  V3Param does.
            AstPin* ಧ್ರುವ pinp = ನೂತನ AstPin{nodep->fileline(),
                                            -1,  // Pin# not relevant
                                            nodep->name(), exprp};
            pinp->param(ಸತ್ಯ);
            pinp->paramPath(path);
            cellp->addParamsp(pinp);
        }
        VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ವೃಕ್ಷ(), nodep);
    }
    ಶೂನ್ಯ visit(AstPort* nodep) override {  // ParamVisitor::
        // Port: Stash the pin number
        // Need to set pin numbers after varnames are created
        // But before we do the final resolution based on names
        VSymEnt* ಧ್ರುವ foundp = m_statep->get_ಗಂಟು_Sym(m_modp)->findIdFlat(nodep->name());
        AstVar* ಧ್ರುವ refp = foundp ? VN_CAST(foundp->nodep(), Var) : nullptr;
        ಯದಿ (!foundp) {
            nodep->v3error(
                "Input/output/inout declaration not found for port: " << nodep->prettyNameQ());
        } ಅನ್ಯಥಾ ಯದಿ (!refp || (!refp->isIO() && !refp->isIfaceRef())) {
            nodep->v3error("Pin is not an in/out/inout/interface: " << nodep->prettyNameQ());
        } ಅನ್ಯಥಾ {
            ಯದಿ (refp->user4()) {
                nodep->v3error("Duplicate declaration of port: "
                               << nodep->prettyNameQ() << '\n'
                               << nodep->warnContextPrimary() << '\n'
                               << refp->warnOther() << "... Location of original declaration\n"
                               << refp->warnContextSecondary());
            }
            refp->user4(ಸತ್ಯ);
            VSymEnt* ಧ್ರುವ symp = m_statep->ಸೇರಿಸು_Sym(m_statep->get_ಗಂಟು_Sym(m_modp),
                                                      "__pinNumber" + cvtToStr(nodep->pin_ಅಂಕೆ()),
                                                      refp, nullptr /*classOrPackagep*/);
            symp->exported(ಅಸತ್ಯ);
            refp->pin_ಅಂಕೆ(nodep->pin_ಅಂಕೆ());
            // Put the variable where the port is, so that variables stay
            // in pin number sorted order. Otherwise hierarchical or JSON
            // may botch by-position instances.
            nodep->addHereThisAsNext(refp->unlinkFrBack());
        }
        // Ports not needed any more
        VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ವೃಕ್ಷ(), nodep);
    }
    ಶೂನ್ಯ visit(AstAssignW* nodep) override {  // ParamVisitor::
        // Deal with implicit definitions
        // We used to nodep->allowImplicit() here, but it turns out
        // normal "assigns" can also make implicit wires.  Yuk.
        pin_Implicit_ಅಭಿವ್ಯಕ್ತಿ_Recurse(nodep->lhsp());
        iterateChildren(nodep);
    }

    ಶೂನ್ಯ visit(AstImplicit* nodep) override {  // ParamVisitor::
        // Unsupported gates need implicit creation
        pin_Implicit_ಅಭಿವ್ಯಕ್ತಿ_Recurse(nodep->exprsp());
        // We're done with implicit gates
        VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ವೃಕ್ಷ(), nodep);
    }
    ಶೂನ್ಯ visit(AstClassOrPackageRef* nodep) override {  // ParamVisitor::
        ಯದಿ (auto* ಧ್ರುವ fwdp = VN_CAST(nodep->classOrPackageNodep(), TypedefFwd)) {
            // Relink forward definitions to the "real" definition
            VSymEnt* ಧ್ರುವ foundp = m_statep->get_ಗಂಟು_Sym(fwdp)->findIdFallback(fwdp->name());
            ಯದಿ (foundp && (VN_IS(foundp->nodep(), Class) || VN_IS(foundp->nodep(), Package))) {
                nodep->classOrPackagep(VN_AS(foundp->nodep(), ಗಂಟು_Module));
            } ಅನ್ಯಥಾ ಯದಿ (foundp && VN_IS(foundp->nodep(), ParamTypeDType)) {
                UASSERT(m_statep->forPrimary(), "Param types should have been resolved");
                nodep->classOrPackageNodep(VN_AS(foundp->nodep(), ParamTypeDType));
            } ಅನ್ಯಥಾ {
                ಯದಿ (foundp) UINFO(1, "found nodep = " << foundp->nodep());
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
    ಶೂನ್ಯ visit(AstPull* nodep) override {  // ParamVisitor::
        // Deal with implicit definitions
        // We used to nodep->allowImplicit() here, but it turns out
        // normal "assigns" can also make implicit wires.  Yuk.
        pin_Implicit_ಅಭಿವ್ಯಕ್ತಿ_Recurse(nodep->lhsp());
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstTypedefFwd* nodep) override {  // ParamVisitor::
        VSymEnt* ಧ್ರುವ foundp = m_statep->get_ಗಂಟು_Sym(nodep)->findIdFallback(nodep->name());
        ಯದಿ (foundp) {
            // If the typedef was earlier in source order (tokenNum), then remember that earlier
            // point to avoid error something wasn't declared
            // Might be forward declaring something odd (with declTokenNumSetMin not implemented,
            // but should be harmless to ignore as this is just for error detection
            foundp->nodep()->decl_Token_ಅಂಕೆ_Set_Min(nodep->fileline()->token_ಅಂಕೆ());
        } ಅನ್ಯಥಾ ಯದಿ (v3Global.opt.pedantic()) {
            // We only check it was ever really defined in pedantic mode, as it
            // might have been in a header file referring to a module we never
            // needed so there are false positives
            nodep->v3error(
                "Forward typedef unused or does not resolve to a data type (IEEE 1800-2023 6.18): "
                << nodep->prettyNameQ());
        }
        // We only needed the forward declaration in order to parse correctly.
        // Delete later as may be ClassOrPackageRef's still pointing to it
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlinkFrBack()), nodep);
    }

    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }  // ParamVisitor::

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    LinkDotParamVisitor(AstNetlist* rootp, LinkDotState* statep)
        : m_statep{statep} {
        UINFO(4, __FUNCTION__ << ": ");
        iterate(rootp);
    }
    ~LinkDotParamVisitor() override = ಸ್ವಭಾವ;
};

//======================================================================

ವರ್ಗ LinkDotScopeVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {

    // STATE
    LinkDotState* ಧ್ರುವ m_statep;  // State to pass between visitors, including symbol table
    ಧ್ರುವ AstScope* m_scopep = nullptr;  // The current scope
    VSymEnt* m_modSymp = nullptr;  // Symbol entry for current module
    // Deferred AliasScope processing - must be done outer-to-inner for correct alias resolution
    std::vector<std::pair<AstAliasScope*, VSymEnt*>> m_deferredAliasScopes;

    // METHODS
ಸಾರ್ವಜನಿಕ:
    // getAlias with setVarAlias and setIfaceAlias implement disjoint-set data structure.
    // This algorithm is needed for the case when multiple alias statements
    // reference partially the same variables.
    ಸ್ಥಿರ AstVarScope* getAlias(AstVarScope* ಧ್ರುವ vscp) {
        ಯದಿ (vscp->user2p() && vscp != vscp->user2p()) {
            AstVarScope* ಧ್ರುವ aliasp = getAlias(VN_AS(vscp->user2p(), VarScope));
            vscp->user2p(aliasp);
            ಪ್ರತ್ಯಾಗಮ aliasp;
        } ಅನ್ಯಥಾ {
            ಪ್ರತ್ಯಾಗಮ vscp;
        }
    }

ಗುಪ್ತ:
    // Alias an interface reference to the interface it is connected to
    ಶೂನ್ಯ setIfaceAlias(AstVarScope* ಧ್ರುವ vscp, AstVarScope* ಧ್ರುವ aliasp) {
        UASSERT_OBJ(vscp->varp()->isIfaceRef(), vscp, "Interface alias of non-interface");
        UASSERT_OBJ(aliasp->varp()->isIfaceRef(), aliasp, "Interface alias of non-interface");
        getAlias(vscp)->user2p(getAlias(aliasp));
    }

    // Alias two variables. An output port in the lower scope survives, unless the other
    // must keep its own storage.
    ಶೂನ್ಯ setVarAlias(AstVarScope* ap, AstVarScope* bp) {
        UASSERT_OBJ(!ap->varp()->isIfaceRef(), ap, "Variable alias of interface");
        UASSERT_OBJ(!bp->varp()->isIfaceRef(), bp, "Variable alias of interface");
        ap = getAlias(ap);
        bp = getAlias(bp);
        ಯದಿ (ap == bp) ಪ್ರತ್ಯಾಗಮ;
        ಧ್ರುವ ತಾರ್ಕಿಕ aWins = [&]() {
            ಧ್ರುವ AstVar* ಧ್ರುವ aVarp = ap->varp();
            ಧ್ರುವ AstVar* ಧ್ರುವ bVarp = bp->varp();
            // A primary IO is accessed directly by the user of the model, so must survive
            UASSERT_OBJ(!aVarp->isPrimaryIO() || !bVarp->isPrimaryIO(), ap,
                        "Alias of two primary IOs");
            ಯದಿ (bVarp->isPrimaryIO()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            ಯದಿ (aVarp->isPrimaryIO()) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
            // A forced or public variable must keep its own storage. If both are, either works.
            ಯದಿ (aVarp->isForced() != bVarp->isForced()) ಪ್ರತ್ಯಾಗಮ aVarp->isForced();
            ಯದಿ (aVarp->isSigPublic() != bVarp->isSigPublic()) ಪ್ರತ್ಯಾಗಮ aVarp->isSigPublic();
            // Otherwise prefer an output port in the lower scope. This enables better V3Combine
            // due to having fewer upward hierarchical references (which prevent combining).
            ಯದಿ (aVarp->direction() != VDirection::OUTPUT) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            // Is 'ap' strictly below 'bp' in the hierarchy
            ಪ್ರತಿ (ಧ್ರುವ AstScope* scopep = ap->scopep()->aboveScopep(); scopep;
                 scopep = scopep->aboveScopep()) {
                ಯದಿ (scopep == bp->scopep()) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
            }
            ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        }();
        ಯದಿ (aWins) {
            bp->user2p(ap);
        } ಅನ್ಯಥಾ {
            ap->user2p(bp);
        }
    }

    // VISITORS
    ಶೂನ್ಯ visit(AstNetlist* nodep) override {  // ScopeVisitor::
        // Recurse..., backward as must do packages before using packages
        iterateChildrenBackwardsConst(nodep);
    }
    ಶೂನ್ಯ visit(AstScope* nodep) override {  // ScopeVisitor::
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
        ಯದಿ (AstIface* ಧ್ರುವ ifacep = VN_CAST(nodep->modp(), Iface)) {
            m_statep->ಸೇರಿಸು_Iface_Mod_Sym(ifacep, m_modSymp);
        }
        m_scopep = nodep;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstVarScope* nodep) override {  // ScopeVisitor::
        ಯದಿ (!nodep->varp()->isFuncLocal() && !nodep->varp()->isClassMember()) {
            VSymEnt* ಧ್ರುವ varSymp
                = m_statep->ಸೇರಿಸು_Sym(m_modSymp, nodep->varp()->name(), nodep, nullptr);
            ಯದಿ (nodep->varp()->isIfaceRef()) {
                AstIfaceRefDType* ಧ್ರುವ dtypep
                    = LinkDotState::ifaceRefFromArray(nodep->varp()->dtypep());
                UASSERT_OBJ(dtypep, nodep, "Non AstIfaceRefDType on isIfaceRef() var");
                ಯದಿ (nodep->varp()->isIfaceParent()) {
                    UINFO(9, "Iface parent ref var " << nodep->varp()->name() << " " << nodep);
                    // Find the interface cell the var references
                    UINFO(9, "Iface parent dtype " << dtypep);
                    ಧ್ರುವ string ifcellname = dtypep->cellName();
                    string baddot;
                    VSymEnt* okSymp;
                    VSymEnt* cellSymp = m_statep->findDotted(nodep->fileline(), m_modSymp,
                                                             ifcellname, baddot, okSymp, ಅಸತ್ಯ);
                    UASSERT_OBJ(
                        cellSymp, nodep,
                        "No symbol for interface instance: " << nodep->prettyNameQ(ifcellname));
                    UINFO(5, "       Found interface instance: se" << cvtToHex(cellSymp) << " "
                                                                   << cellSymp->nodep());
                    ಯದಿ (dtypep->modportName() != "") {
                        // Look up the modport within the interface cell's symbol table
                        VSymEnt* ಧ್ರುವ mpSymp = cellSymp->findIdFallback(dtypep->modportName());
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
                    m_statep->ಸೇರಿಸು_Scope_Alias(LinkDotState::SAMN_IFTOP, varSymp, cellSymp);
                }
            }
        }
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_FTask* nodep) override {  // ScopeVisitor::
        VSymEnt* ಧ್ರುವ symp = m_statep->ಸೇರಿಸು_Block(m_modSymp, nodep->name(), nodep, nullptr);
        symp->fallbackp(m_modSymp);
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Foreach* nodep) override {  // ScopeVisitor::
        VSymEnt* ಧ್ರುವ symp = m_statep->ಸೇರಿಸು_Block(m_modSymp, nodep->name(), nodep, nullptr);
        symp->fallbackp(m_modSymp);
        // No recursion, we don't want to pick up variables
    }
    ಶೂನ್ಯ visit(AstConstraintForeach* nodep) override {  // ScopeVisitor::
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstWith* nodep) override {  // ScopeVisitor::
        VSymEnt* ಧ್ರುವ symp = m_statep->ಸೇರಿಸು_Block(m_modSymp, nodep->name(), nodep, nullptr);
        symp->fallbackp(m_modSymp);
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstAlias* nodep) override {  // ScopeVisitor::
        // Track aliases
        UINFOTREE(9, nodep, "", "alias");
        AstVarScope* aliasVscp = nullptr;
        ಪ್ರತಿ (Ast_ಗಂಟು* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
            AstVarScope* vscp = nullptr;
            ಯದಿ (ಧ್ರುವ AstVarRef* ಧ್ರುವ refp = VN_CAST(itemp, VarRef)) {
                vscp = refp->varScopep();
            } ಅನ್ಯಥಾ {
                // Reaches into the scope of an instance, look it up by name
                ಧ್ರುವ AstVarXRef* ಧ್ರುವ xrefp = VN_AS(itemp, VarXRef);
                ಧ್ರುವ string scopename = xrefp->dotted() + "." + xrefp->name();
                string baddot;
                VSymEnt* okSymp;
                VSymEnt* ಧ್ರುವ symp = m_statep->findDotted(xrefp->fileline(), m_modSymp, scopename,
                                                           baddot, okSymp, ಅಸತ್ಯ);
                UASSERT_OBJ(symp, nodep, "No symbol for alias item: " << scopename);
                vscp = VN_CAST(symp->nodep(), VarScope);
            }
            UASSERT_OBJ(vscp, nodep, "VarScope unset");
            ಯದಿ (aliasVscp) {
                setVarAlias(aliasVscp, vscp);
            } ಅನ್ಯಥಾ {
                aliasVscp = vscp;
            }
        }
        iterateChildren(nodep);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlinkFrBack()), nodep);
    }
    ಶೂನ್ಯ visit(AstAliasScope* nodep) override {  // ScopeVisitor::
        // Defer AliasScope processing - must process outer scopes before inner ones
        // so that nested interface port alias resolution works correctly
        UINFO(5, "ALIASSCOPE (deferred) " << nodep);
        m_deferredAliasScopes.emplace_back(nodep, m_modSymp);
    }
    ಶೂನ್ಯ processAliasScope(AstAliasScope* nodep, VSymEnt* modSymp) {
        UINFO(5, "ALIASSCOPE  " << nodep);
        UINFOTREE(9, nodep, "", "avs");
        VSymEnt* rhsSymp;
        {
            AstVarRef* ಧ್ರುವ refp = VN_CAST(nodep->rhsp(), VarRef);
            AstVarXRef* ಧ್ರುವ xrefp = VN_CAST(nodep->rhsp(), VarXRef);
            UASSERT_OBJ(refp || xrefp, nodep,
                        "Unsupported: Non Var(X)Ref attached to interface pin");
            string inl
                = ((xrefp && xrefp->inlinedDots().size()) ? (xrefp->inlinedDots() + "__DOT__")
                                                          : "");
            ಧ್ರುವ string dottedPath
                = (xrefp && !xrefp->dotted().empty()) ? (xrefp->dotted() + ".") : "";
            VSymEnt* symp = nullptr;
            string scopename;
            ಯಾವತ್ (!symp) {
                scopename = refp ? refp->name()
                                 : (inl.size() ? (inl + dottedPath + xrefp->name())
                                               : (dottedPath + xrefp->name()));
                string baddot;
                VSymEnt* okSymp;
                symp = m_statep->findDotted(nodep->rhsp()->fileline(), modSymp, scopename, baddot,
                                            okSymp, ಸತ್ಯ);
                ಯದಿ (inl == "") ಭಙ್ಗ;
                inl = LinkDotState::removeLastInlineScope(inl);
            }
            UASSERT_OBJ(symp, nodep, "No symbol for interface alias rhs");
            UINFO(5, "       Found a linked scope RHS: " << scopename << "  se" << cvtToHex(symp)
                                                         << " " << symp->nodep());
            rhsSymp = symp;
        }
        VSymEnt* lhsSymp;
        {
            ಧ್ರುವ AstVarXRef* ಧ್ರುವ xrefp = VN_CAST(nodep->lhsp(), VarXRef);
            ಧ್ರುವ AstVarRef* ಧ್ರುವ refp = VN_CAST(nodep->lhsp(), VarRef);

            UASSERT_OBJ(refp || xrefp, nodep,
                        "Unsupported: Non Var(X)Ref attached to interface pin");
            ಧ್ರುವ string scopename
                = refp ? refp->varp()->name() : xrefp->dotted() + "." + xrefp->name();
            string baddot;
            VSymEnt* okSymp;
            VSymEnt* ಧ್ರುವ symp = m_statep->findDotted(nodep->lhsp()->fileline(), modSymp,
                                                       scopename, baddot, okSymp, ಅಸತ್ಯ);
            UASSERT_OBJ(symp, nodep, "No symbol for interface alias lhs");
            UINFO(5, "       Found a linked scope LHS: " << scopename << "  se" << cvtToHex(symp)
                                                         << " " << symp->nodep());
            lhsSymp = symp;
        }
        // Remember the alias - can't do it yet because we may have additional symbols to be added,
        // or maybe an alias of an alias
        m_statep->ಸೇರಿಸು_Scope_Alias(LinkDotState::SAMN_IFTOP, lhsSymp, rhsSymp);
        AstVarScope* ಧ್ರುವ lhsVscp = VN_CAST(lhsSymp->nodep(), VarScope);
        AstVarScope* ಧ್ರುವ rhsVscp = VN_CAST(rhsSymp->nodep(), VarScope);
        UASSERT_OBJ(lhsVscp && rhsVscp, nodep, "Interface alias missing variable scope");
        setIfaceAlias(lhsVscp, rhsVscp);
        // We have stored the link, we don't need these any more
        VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ವೃಕ್ಷ(), nodep);
    }
    ಶೂನ್ಯ processDeferredAliasScopes() {
        // Sort by hierarchy depth (shallower first) so outer aliases are resolved before inner
        // Pre-compute depth map to avoid O(N log N * D) complexity in sort comparisons
        std::unordered_map<VSymEnt*, ಪೂರ್ಣಾಙ್ಕ> depthMap;
        ಪ್ರತಿ (ಧ್ರುವ auto& pair : m_deferredAliasScopes) {
            VSymEnt* ಧ್ರುವ symp = pair.second;
            ಯದಿ (depthMap.find(symp) == depthMap.end()) {
                ಪೂರ್ಣಾಙ್ಕ depth = 0;
                ಪ್ರತಿ (VSymEnt* p = symp; p; p = p->parentp()) ++depth;
                depthMap[symp] = depth;
            }
        }
        std::stable_sort(m_deferredAliasScopes.begin(), m_deferredAliasScopes.end(),
                         [&depthMap](ಧ್ರುವ std::pair<AstAliasScope*, VSymEnt*>& a,
                                     ಧ್ರುವ std::pair<AstAliasScope*, VSymEnt*>& b) {
                             ಪ್ರತ್ಯಾಗಮ depthMap.at(a.second) < depthMap.at(b.second);
                         });
        // Process in sorted order
        ಪ್ರತಿ (auto& pair : m_deferredAliasScopes) { processAliasScope(pair.first, pair.second); }
        m_deferredAliasScopes.clear();
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Gen* nodep) override {  // ScopeVisitor::  // LCOV_EXCL_LINE
        nodep->v3fatalSrc("Generate constructs should have been reduced out");
    }
    // For speed, don't recurse things that can't have scope
    // Note we allow AstNodeStmt's as generates may be under them
    ಶೂನ್ಯ visit(AstCell*) override {}  // ScopeVisitor::
    ಶೂನ್ಯ visit(AstVar*) override {}  // ScopeVisitor::
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }  // ScopeVisitor::

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    LinkDotScopeVisitor(AstNetlist* rootp, LinkDotState* statep)
        : m_statep{statep} {
        UINFO(4, __FUNCTION__ << ": ");
        iterate(rootp);
        // Process deferred AliasScopes in outer-to-inner order
        processDeferredAliasScopes();
    }
    ~LinkDotScopeVisitor() override = ಸ್ವಭಾವ;
};

//======================================================================

// Iterate an interface to resolve modports
ವರ್ಗ LinkDotIfaceVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // STATE
    LinkDotState* ಧ್ರುವ m_statep;  // State to pass between visitors, including symbol table
    VSymEnt* m_curSymp;  // Symbol Entry for current table, where to lookup/insert

    // VISITORS
    ಶೂನ್ಯ visit(AstModport* nodep) override {  // IfaceVisitor::
        // Modport: Remember its name for later resolution
        UINFO(5, "   fiv: " << nodep);
        VL_RESTORER(m_curSymp);
        {
            // Create symbol table for the vars
            m_curSymp = m_statep->ಸೇರಿಸು_Block(m_curSymp, nodep->name(), nodep, nullptr);
            m_curSymp->fallbackp(VL_RESTORER_PREV(m_curSymp));
            iterateChildren(nodep);
        }
    }
    ಶೂನ್ಯ visit(AstModportFTaskRef* nodep) override {  // IfaceVisitor::
        UINFO(5, "   fif: " << nodep);
        iterateChildren(nodep);
        VSymEnt* ಧ್ರುವ symp = m_curSymp->findIdFallback(nodep->name());
        ಯದಿ (!symp) {
            nodep->v3error("Modport item not found: " << nodep->prettyNameQ());
        } ಅನ್ಯಥಾ ಯದಿ (Ast_ಗಂಟು_FTask* ಧ್ರುವ ftaskp = VN_CAST(symp->nodep(), ಗಂಟು_FTask)) {
            // Make symbol under modport that points at the _interface_'s var, not the modport.
            nodep->ftaskp(ftaskp);
            VSymEnt* ಧ್ರುವ subSymp
                = m_statep->ಸೇರಿಸು_Sym(m_curSymp, nodep->name(), ftaskp, nullptr /*package*/);
            m_statep->ಸೇರಿಸು_Scope_Alias(LinkDotState::SAMN_MODPORT, subSymp, symp);
        } ಅನ್ಯಥಾ {
            nodep->v3error("Modport item is not a function/task: " << nodep->prettyNameQ());
        }
        ಯದಿ (m_statep->forScopeCreation()) {
            // Done with AstModportFTaskRef.
            // Delete to prevent problems if we dead-delete pointed to ftask
            nodep->unlinkFrBack();
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        }
    }

    // Helper to resolve remaining path through a nested interface
    // When findDotted() partially matches (okSymp set, baddot non-empty),
    // this follows the interface type to resolve the remaining path.
    // Returns the resolved symbol, or nullptr if not found.
    // On success, clears baddot; on partial match of multi-level path, updates baddot.
    VSymEnt* resolveNestedInterfacePath(File_ಸಾಲು* fl, VSymEnt* okSymp, string& baddot) {
        ಯದಿ (!okSymp || baddot.empty()) ಪ್ರತ್ಯಾಗಮ nullptr;

        ಸ್ಥಿರ constexpr ಪೂರ್ಣಾಙ್ಕ MAX_NESTING_DEPTH = 64;
        VSymEnt* curOkSymp = okSymp;

        ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ depth = 0; depth < MAX_NESTING_DEPTH; ++depth) {
            // Try to get interface from the partially-matched symbol
            AstIface* ifacep = nullptr;
            ಯದಿ (ಧ್ರುವ AstCell* ಧ್ರುವ cellp = VN_CAST(curOkSymp->nodep(), Cell)) {
                ifacep = VN_CAST(cellp->modp(), Iface);
            } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstVar* ಧ್ರುವ varp = VN_CAST(curOkSymp->nodep(), Var)) {
                ಯದಿ (varp->isIfaceRef()) {
                    ಯದಿ (ಧ್ರುವ AstIfaceRefDType* ಧ್ರುವ ifaceRefp
                        = LinkDotState::ifaceRefFromArray(varp->dtypep())) {
                        ifacep = ifaceRefp->ifaceViaCellp();
                    }
                }
            }

            ಯದಿ (!ifacep || !m_statep->exists_ಗಂಟು_Sym(ifacep)) ಪ್ರತ್ಯಾಗಮ nullptr;

            VSymEnt* ಧ್ರುವ ifaceSymp = m_statep->get_ಗಂಟು_Sym(ifacep);

            ಯದಿ (baddot.find('.') == string::npos) {
                // Simple identifier - direct lookup
                VSymEnt* ಧ್ರುವ symp = ifaceSymp->findIdFallback(baddot);
                ಯದಿ (symp) baddot.clear();
                ಪ್ರತ್ಯಾಗಮ symp;
            }

            // Multi-level path - use findDotted for partial resolution
            string remainingBaddot;
            VSymEnt* remainingOkSymp = nullptr;
            VSymEnt* ಧ್ರುವ symp = m_statep->findDotted(fl, ifaceSymp, baddot, remainingBaddot,
                                                       remainingOkSymp, ಸತ್ಯ);
            ಯದಿ (symp) {
                baddot = remainingBaddot;
                ಪ್ರತ್ಯಾಗಮ symp;
            }

            // findDotted partially matched - check progress
            ಯದಿ (remainingBaddot == baddot || !remainingOkSymp) ಪ್ರತ್ಯಾಗಮ nullptr;

            // Continue resolving with updated state
            baddot = remainingBaddot;
            curOkSymp = remainingOkSymp;
        }

        UINFO(1, "Nested interface resolution depth limit exceeded at " << fl);
        ಪ್ರತ್ಯಾಗಮ nullptr;
    }

    // Resolve a modport expression to find the referenced symbol
    VSymEnt* resolveModportExpression(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp) {
        UINFO(5, "     resolveModportExpression: " << exprp);
        VSymEnt* symp = nullptr;
        ಯದಿ (AstParseRef* ಧ್ರುವ refp = VN_CAST(exprp, ParseRef)) {
            // Simple variable reference: modport mp(input .a(sig_a))
            symp = m_curSymp->findIdFallback(refp->name());
        } ಅನ್ಯಥಾ ಯದಿ (AstVarRef* ಧ್ರುವ refp = VN_CAST(exprp, VarRef)) {
            // Already resolved VarRef (can happen if iterateChildren resolved it)
            ಯದಿ (refp->varScopep()) {
                // During scope creation, VarRef may have VarScope already linked
                symp = m_statep->get_ಗಂಟು_Sym(refp->varScopep());
            } ಅನ್ಯಥಾ ಯದಿ (refp->varp()) {
                symp = m_curSymp->findIdFallback(refp->varp()->name());
            }
        } ಅನ್ಯಥಾ ಯದಿ (AstVarXRef* ಧ್ರುವ refp = VN_CAST(exprp, VarXRef)) {
            // Resolved VarXRef (dotted reference resolved by earlier pass)
            ಯದಿ (refp->varp()) {
                // For nested interfaces, the var is in a different scope
                // First try to find the symbol via the var itself
                ಯದಿ (m_statep->exists_ಗಂಟು_Sym(refp->varp())) {
                    symp = m_statep->get_ಗಂಟು_Sym(refp->varp());
                } ಅನ್ಯಥಾ {
                    // Fallback: look up in current scope
                    symp = m_curSymp->findIdFallback(refp->varp()->name());
                }
            } ಅನ್ಯಥಾ ಯದಿ (!refp->dotted().empty()) {
                // varp not set yet - use dotted path to find the symbol
                // The dotted part (e.g., "base") is the interface cell, and name is the member
                ಧ್ರುವ string fullPath = refp->dotted() + "." + refp->name();
                string baddot;
                VSymEnt* okSymp = nullptr;
                symp = m_statep->findDotted(fl, m_curSymp, fullPath, baddot, okSymp, ಸತ್ಯ);
                // Handle nested interface path when findDotted had partial match
                ಯದಿ (okSymp && !baddot.empty()) {
                    VSymEnt* ಧ್ರುವ resolved = resolveNestedInterfacePath(fl, okSymp, baddot);
                    ಯದಿ (resolved) symp = resolved;
                }
            }
        } ಅನ್ಯಥಾ ಯದಿ (AstDot* ಧ್ರುವ dotp = VN_CAST(exprp, Dot)) {
            // Dotted path: modport mp(input .a(inner.sig))
            ತಾರ್ಕಿಕ hasPartSelect = ಅಸತ್ಯ;
            ಧ್ರುವ string dottedPath = extractDottedPath(dotp, hasPartSelect);
            ಯದಿ (hasPartSelect) {
                fl->v3warn(
                    E_UNSUPPORTED,
                    "Unsupported: Modport expression with part select (IEEE 1800-2023 25.5.4)");
            } ಅನ್ಯಥಾ {
                string baddot;
                VSymEnt* okSymp = nullptr;
                symp = m_statep->findDotted(fl, m_curSymp, dottedPath, baddot, okSymp, ಸತ್ಯ);

                // Handle nested interface path when findDotted had partial match
                ಯದಿ (okSymp && !baddot.empty()) {
                    VSymEnt* ಧ್ರುವ resolved = resolveNestedInterfacePath(fl, okSymp, baddot);
                    ಯದಿ (resolved) symp = resolved;
                }

                ಯದಿ (!symp || !baddot.empty()) {
                    fl->v3error("Can't find modport expression target: "
                                << Ast_ಗಂಟು::prettyNameQ(dottedPath));
                    symp = nullptr;
                }
            }
        } ಅನ್ಯಥಾ ಯದಿ (VN_IS(exprp, SelBit) || VN_IS(exprp, SelExtract)) {
            // Part select expressions not yet supported
            fl->v3warn(E_UNSUPPORTED,
                       "Unsupported: Modport expression with part select (IEEE 1800-2023 25.5.4)");
        } ಅನ್ಯಥಾ {
            // Other expression types not supported
            fl->v3warn(E_UNSUPPORTED,
                       "Unsupported: Complex modport expression (IEEE 1800-2023 25.5.4)");
        }
        ಪ್ರತ್ಯಾಗಮ symp;
    }

    ಶೂನ್ಯ visit(AstModportVarRef* nodep) override {  // IfaceVisitor::
        UINFO(5, "   fiv: " << nodep);
        iterateChildren(nodep);
        VSymEnt* symp = nullptr;
        ಯದಿ (nodep->exprp()) {
            // Modport expression syntax: modport mp(input .port_name(expression))
            symp = resolveModportExpression(nodep->fileline(), nodep->exprp());
        } ಅನ್ಯಥಾ {
            symp = m_curSymp->findIdFallback(nodep->name());
        }
        ಯದಿ (!symp) {
            nodep->v3error("Modport item not found: " << nodep->prettyNameQ());
        } ಅನ್ಯಥಾ ಯದಿ (AstVar* ಧ್ರುವ varp = VN_CAST(symp->nodep(), Var)) {
            // Make symbol under modport that points at the _interface_'s var via the modport.
            // (Need modport still to test input/output markings)
            nodep->varp(varp);
            ಯದಿ (nodep->exprp()) {
                // For modport expressions, insert symbol pointing to the underlying var (not the
                // ModportVarRef which will be deleted during scope creation). The virtual port
                // name maps to the real signal's var.
                VSymEnt* ಧ್ರುವ subSymp
                    = m_statep->ಸೇರಿಸು_Sym(m_curSymp, nodep->name(), varp, nullptr /*package*/);
                m_statep->ಸೇರಿಸು_Scope_Alias(LinkDotState::SAMN_MODPORT, subSymp, symp);
            } ಅನ್ಯಥಾ {
                // For regular modport items, insert symbol pointing to ModportVarRef for
                // input/output marking tests.
                m_statep->ಸೇರಿಸು_Sym(m_curSymp, nodep->name(), nodep, nullptr /*package*/);
            }
        } ಅನ್ಯಥಾ ಯದಿ (AstVarScope* ಧ್ರುವ vscp = VN_CAST(symp->nodep(), VarScope)) {
            // Make symbol under modport that points at the _interface_'s var, not the modport.
            nodep->varp(vscp->varp());
            // Only insert symbol for modport expression virtual ports (exprp set).
            // For regular modport items, don't insert into the shared modport table because
            // each instance would overwrite the previous one, causing wrong VarScope lookups.
            // Regular items are found via the modport's fallback to the interface.
            ಯದಿ (nodep->exprp()) {
                VSymEnt* ಧ್ರುವ subSymp
                    = m_statep->ಸೇರಿಸು_Sym(m_curSymp, nodep->name(), vscp, nullptr /*package*/);
                m_statep->ಸೇರಿಸು_Scope_Alias(LinkDotState::SAMN_MODPORT, subSymp, symp);
            }
        } ಅನ್ಯಥಾ {
            nodep->v3error("Modport item is not a variable: " << nodep->prettyNameQ());
        }
        ಯದಿ (m_statep->forScopeCreation()) {
            // Done with AstModportVarRef.
            // Delete to prevent problems if we dead-delete pointed to variable
            nodep->unlinkFrBack();
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        }
    }
    ಶೂನ್ಯ visit(AstModportClockingRef* nodep) override {  // IfaceVisitor::
        UINFO(5, "   fic: " << nodep);
        iterateChildren(nodep);
        VSymEnt* ಧ್ರುವ symp = m_curSymp->findIdFallback(nodep->name());
        ಯದಿ (!symp) {
            nodep->v3error("Modport item not found: " << nodep->prettyNameQ());
        } ಅನ್ಯಥಾ ಯದಿ (AstClocking* ಧ್ರುವ clockingp = VN_CAST(symp->nodep(), Clocking)) {
            nodep->clockingp(clockingp);
            m_statep->ಸೇರಿಸು_Sym(m_curSymp, nodep->name(), nodep, nullptr /*package*/);
        } ಅನ್ಯಥಾ {
            nodep->v3error("Modport item is not a clocking block: " << nodep->prettyNameQ());
        }
    }
    ಶೂನ್ಯ visit(AstScope* nodep) override {  // IfaceVisitor::
        ಯದಿ (nodep->user3SetOnce()) ಪ್ರತ್ಯಾಗಮ;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }  // IfaceVisitor::

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    LinkDotIfaceVisitor(AstIface* nodep, VSymEnt* curSymp, LinkDotState* statep)
        : m_statep{statep}
        , m_curSymp{curSymp} {
        UINFO(4, __FUNCTION__ << ": ");
        iterate(nodep);
    }
    ~LinkDotIfaceVisitor() override = ಸ್ವಭಾವ;
};

ಶೂನ್ಯ LinkDotState::computeIfaceModSyms() {
    //  AstScope::user3()  // bool. Already iterated by a LinkDotIfaceVisitor
    ಧ್ರುವ VNUser3InUse user3InUse;
    ಪ್ರತಿ (ಧ್ರುವ auto& itr : m_ifaceModSyms) {
        AstIface* ಧ್ರುವ nodep = itr.first;
        VSymEnt* ಧ್ರುವ symp = itr.second;
        LinkDotIfaceVisitor{nodep, symp, ಏತತ್};
    }
    m_ifaceModSyms.clear();
}

//======================================================================

ವರ್ಗ LinkDotResolveVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // NODE STATE
    // Cleared on global
    //  *::user1p()             -> See LinkDotState
    //  *::user2p()             -> See LinkDotState
    //  *::user3()              // bool. Processed
    //  *::user4()              -> See LinkDotState
    ಧ್ರುವ VNUser3InUse m_inuser3;

    // TYPES
    ಗಣನಾ DotPosition : uint8_t {
        // Must match ascii() method below
        DP_NONE = 0,  // Not under a DOT
        DP_PACKAGE,  // {package-or-class}:: DOT
        DP_FIRST,  // {scope-or-var} DOT
        DP_SCOPE,  // DOT... {scope-or-var} DOT
        DP_FINAL,  // [DOT...] {var-or-func-or-dtype} with no following dots
        DP_MEMBER  // DOT {member-name} [DOT...]
    };

    // STATE
    LinkDotState* ಧ್ರುವ m_statep;  // State, including dotted symbol table
    VSymEnt* m_curSymp = nullptr;  // SymEnt for current lookup point
    VSymEnt* m_modSymp = nullptr;  // SymEnt for current module
    VSymEnt* m_pinSymp = nullptr;  // SymEnt for pin lookups
    VSymEnt* m_randSymp = nullptr;  // SymEnt for randomize target class's lookups
    ಧ್ರುವ AstCell* m_cellp = nullptr;  // Current cell
    Ast_ಗಂಟು_Module* m_modp = nullptr;  // Current module
    Ast_ಗಂಟು_FTask* m_ftaskp = nullptr;  // Current function/task
    AstMethodCall* m_randMethodCallp = nullptr;  // Current randomize() method call
    ಪೂರ್ಣಾಙ್ಕ m_modport_ಅಂಕೆ = 0;  // Uniqueify modport numbers
    ಪೂರ್ಣಾಙ್ಕ m_indent = 0;  // Indentation (tree depth) for debug
    ತಾರ್ಕಿಕ m_inSens = ಅಸತ್ಯ;  // True if in senitem
    ಧ್ರುವ AstWith* m_currentWithp = nullptr;  // Enclosing 'with' (nullptr if none)
    std::set<std::string>
        m_restrictedNamesUsed;  // Names from current 'with (id_list)' that resolved into target
    ತಾರ್ಕಿಕ m_genericIfaceModule = ಅಸತ್ಯ;  // True if in module containing generic interface
    std::map<std::string, Ast_ಗಂಟು*> m_ifClassImpNames;  // Names imported from interface class
    // Inherited interface implementations needing a forward in the current class, by name
    std::map<std::string, ಧ್ರುವ VSymEnt*> m_ifForwards;
    std::set<AstClass*> m_extendsParam;  // Classes that have a parameterized super class
                                         // (except the default instances)
                                         // They are added to the set only in linkDotPrimary.
    ತಾರ್ಕಿಕ m_insideClassExtParam = ಅಸತ್ಯ;  // Inside a class from m_extendsParam
    AstNew* m_explicitSuperNewp = nullptr;  // Hit a "super.new" call inside a "new" function
    std::map<Ast_ಗಂಟು*, AstPin*> m_usedPins;  // Pin used in this cell, map to duplicate
    std::map<Ast_ಗಂಟು*, std::map<std::string, AstPin*>> m_usedDefParamPins;
    // Defparam pins used for a given formal, by hierarchical path
    std::map<std::string, Ast_ಗಂಟು_Module*> m_modulesToRevisit;  // Modules to revisit a second time
    Ast_ಗಂಟು* m_lastDeferredp = nullptr;  // Last node which requested a revisit of its module
    Ast_ಗಂಟು_DType* m_packedArrayDtp = nullptr;  // Datatype reference for packed array
    ತಾರ್ಕಿಕ m_inPackedArray = ಅಸತ್ಯ;  // Currently traversing a packed array tree
    ತಾರ್ಕಿಕ m_replaceWithAlias
        = ಸತ್ಯ;  // Replace VarScope with an alias. Used in the handling of AstAlias
    ತಾರ್ಕಿಕ m_isParam = ಅಸತ್ಯ;  // Specifies whether currently visiting param variable
    ತಾರ್ಕಿಕ m_resolvingTypedef = ಅಸತ್ಯ;  // Currently traversing a Typedef tree

    ಸಂರಚನಾ DotStates final {
        DotPosition m_dotPos;  // Scope part of dotted resolution
        VSymEnt* m_dotSymp;  // SymEnt for dotted AstParse lookup
        ಧ್ರುವ AstDot* m_dotp;  // Current dot
        ತಾರ್ಕಿಕ m_super;  // Starts with super reference
        ತಾರ್ಕಿಕ m_unresolvedCell;  // Unresolved cell, needs help from V3Param
        ತಾರ್ಕಿಕ m_unresolvedClass;  // Unresolved class reference, needs help from V3Param
        ತಾರ್ಕಿಕ m_unresolvedGenericIface;  // Unresolved generic interface, needs help from V3Param
        ತಾರ್ಕಿಕ m_genBlk;  // Contains gen block reference
        Ast_ಗಂಟು* m_unlinkedScopep;  // Unresolved scope, needs corresponding VarXRef
        AstDisable* m_disablep;  // Disable statement under which the reference is
        ತಾರ್ಕಿಕ m_dotErr;  // Error found in dotted resolution, ignore upwards
        string m_dotText;  // String of dotted names found in below parseref
        DotStates() { init(nullptr); }
        ~DotStates() = ಸ್ವಭಾವ;
        ಶೂನ್ಯ init(VSymEnt* curSymp, ತಾರ್ಕಿಕ unresolvedClass = ಅಸತ್ಯ) {
            m_dotPos = DP_NONE;
            m_dotSymp = curSymp;
            m_dotp = nullptr;
            m_super = ಅಸತ್ಯ;
            m_dotErr = ಅಸತ್ಯ;
            m_dotText = "";
            m_unresolvedCell = ಅಸತ್ಯ;
            m_unresolvedClass = unresolvedClass;
            m_unresolvedGenericIface = ಅಸತ್ಯ;
            m_genBlk = ಅಸತ್ಯ;
            m_unlinkedScopep = nullptr;
            m_disablep = nullptr;
        }
        string ascii() ಧ್ರುವ {
            ಸ್ಥಿರ ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ names[]
                = {"DP_NONE", "DP_PACKAGE", "DP_FIRST", "DP_SCOPE", "DP_FINAL", "DP_MEMBER"};
            std::ostringstream sstr;
            sstr << "ds=" << names[m_dotPos];
            sstr << "  dse" << cvtToHex(m_dotSymp);
            ಧ್ರುವ string dsname = m_dotSymp->nodep()->name().substr(0, 8);
            sstr << "(" << m_dotSymp->nodep()->typeName() << (dsname.empty() ? "" : ":") << dsname
                 << ")";
            ಯದಿ (m_dotErr) sstr << "  [dotErr]";
            ಯದಿ (m_super) sstr << "  [super]";
            ಯದಿ (m_unresolvedCell) sstr << "  [unrCell]";
            ಯದಿ (m_unresolvedClass) sstr << "  [unrClass]";
            ಯದಿ (m_unresolvedGenericIface) sstr << "  [unrGIface]";
            ಯದಿ (m_genBlk) sstr << "  [genBlk]";
            sstr << "  txt=" << m_dotText;
            ಪ್ರತ್ಯಾಗಮ sstr.str();
        }
    } m_ds;  // State to preserve across recursions

    // METHODS - Variables
    AstVar* createImplicitVar(VSymEnt* /*lookupSymp*/, AstParseRef* nodep, Ast_ಗಂಟು_Module* modp,
                              VSymEnt* moduleSymp, ತಾರ್ಕಿಕ noWarn) {
        // Create implicit after warning
        ಯದಿ (!noWarn) {
            ಯದಿ (nodep->fileline()->warnIsOff(V3ErrorCode::I_DEF_NETTYPE_WIRE)) {
                ಧ್ರುವ string suggest = m_statep->suggestSymFallback(moduleSymp, nodep->name(),
                                                                    Link_ಗಂಟು_Matcher_Var{});
                nodep->v3error("Signal definition not found, and implicit disabled with "
                               "`default_nettype: "
                               << nodep->prettyNameQ() << '\n'
                               << (suggest.empty() ? "" : nodep->warnMore() + suggest));

            }
            // Bypass looking for suggestions if IMPLICIT is turned off
            // as there could be thousands of these suppressed in large netlists
            ಅನ್ಯಥಾ ಯದಿ (!nodep->fileline()->warnIsOff(V3ErrorCode::IMPLICIT)) {
                ಧ್ರುವ string suggest = m_statep->suggestSymFallback(moduleSymp, nodep->name(),
                                                                    Link_ಗಂಟು_Matcher_Var{});
                nodep->v3warn(IMPLICIT,
                              "Signal definition not found, creating implicitly: "
                                  << nodep->prettyNameQ() << '\n'
                                  << (suggest.empty() ? "" : nodep->warnMore() + suggest));
            }
        }
        AstVar* ಧ್ರುವ newp
            = ನೂತನ AstVar{nodep->fileline(), VVarType::WIRE, nodep->name(), VFlagLogicPacked{}, 1};
        newp->lifetime(VLifetime::STATIC_IMPLICIT);
        newp->trace(modp->modTrace());
        modp->addStmtsp(newp);
        // Link it to signal list, must add the variable under the module;
        // current scope might be lower now
        m_statep->ಸೇರಿಸು_Sym(moduleSymp, newp->name(), newp, nullptr /*classOrPackagep*/);
        ಪ್ರತ್ಯಾಗಮ newp;
    }
    AstVar* foundToVarp(ಧ್ರುವ VSymEnt* symp, Ast_ಗಂಟು* nodep, VAccess access) {
        // Return a variable if possible, auto converting a modport to variable
        ಯದಿ (!symp) {
            ಪ್ರತ್ಯಾಗಮ nullptr;
        } ಅನ್ಯಥಾ ಯದಿ (VN_IS(symp->nodep(), Var)) {
            ಪ್ರತ್ಯಾಗಮ VN_AS(symp->nodep(), Var);
        } ಅನ್ಯಥಾ ಯದಿ (VN_IS(symp->nodep(), ModportVarRef)) {
            AstModportVarRef* ಧ್ರುವ snodep = VN_AS(symp->nodep(), ModportVarRef);
            AstVar* ಧ್ರುವ varp = snodep->varp();
            ಯದಿ (access.isWriteOrRW() && snodep->direction().isReadOnly()) {
                nodep->v3error("Attempt to drive input-only modport: " << nodep->prettyNameQ());
            }  // else other simulators don't warn about reading, and IEEE doesn't say illegal
            ಪ್ರತ್ಯಾಗಮ varp;
        } ಅನ್ಯಥಾ {
            ಪ್ರತ್ಯಾಗಮ nullptr;
        }
    }

    // Look up a virtual port through modport expression mapping.
    // When a dotted reference uses a modport with expression syntax, the virtual port name
    // maps to the real signal. This helper resolves that mapping during scope creation.
    // Returns the VarScope for the real signal, or nullptr if not found.
    AstVarScope* findVirtualPortVarScope(VSymEnt* dotSymp, ಧ್ರುವ string& portName) {
        ಧ್ರುವ AstVarScope* ಧ್ರುವ dotVscp = VN_CAST(dotSymp->nodep(), VarScope);
        ಧ್ರುವ AstVar* ಧ್ರುವ dotVarp = dotVscp ? dotVscp->varp() : nullptr;
        ಯದಿ (!dotVarp) ಪ್ರತ್ಯಾಗಮ nullptr;

        Ast_ಗಂಟು_DType* dtypep = dotVarp->childDTypep();
        ಯದಿ (!dtypep) dtypep = dotVarp->subDTypep();
        ಯದಿ (!dtypep) ಪ್ರತ್ಯಾಗಮ nullptr;
        ಧ್ರುವ AstIfaceRefDType* ಧ್ರುವ ifaceRefp = VN_CAST(dtypep, IfaceRefDType);
        ಯದಿ (!ifaceRefp || !ifaceRefp->modportp() || !ifaceRefp->ifaceViaCellp()) ಪ್ರತ್ಯಾಗಮ nullptr;

        // Get the interface cell's symbol table
        VSymEnt* ಧ್ರುವ ifaceCellSymp = m_statep->get_ಗಂಟು_Sym(ifaceRefp->ifaceViaCellp());

        // Look up modport by name in interface cell
        VSymEnt* ಧ್ರುವ modportSymp = ifaceCellSymp->findIdFallback(ifaceRefp->modportp()->name());
        ಯದಿ (!modportSymp) ಪ್ರತ್ಯಾಗಮ nullptr;

        // Look up virtual port name in modport symbol table
        string baddot;
        VSymEnt* ಧ್ರುವ virtPortp = m_statep->findSymPrefixed(modportSymp, portName, baddot, ಸತ್ಯ);
        ಯದಿ (!virtPortp) ಪ್ರತ್ಯಾಗಮ nullptr;

        // Symbol may point to VarScope or Var
        ಯದಿ (AstVarScope* vscp = VN_CAST(virtPortp->nodep(), VarScope)) ಪ್ರತ್ಯಾಗಮ vscp;

        // Symbol points to Var - look up VarScope by name in interface cell
        ಯದಿ (ಧ್ರುವ AstVar* ಧ್ರುವ realVarp = VN_CAST(virtPortp->nodep(), Var)) {
            VSymEnt* ಧ್ರುವ realFoundp
                = m_statep->findSymPrefixed(ifaceCellSymp, realVarp->name(), baddot, ಸತ್ಯ);
            ಪ್ರತ್ಯಾಗಮ realFoundp ? VN_CAST(realFoundp->nodep(), VarScope) : nullptr;
        }
        ಪ್ರತ್ಯಾಗಮ nullptr;
    }

    // Capture a ParamTypeDType reference for interface typedef retargeting.
    // Called when a RefDType resolves to a ParamTypeDType owned by an interface.
    ಶೂನ್ಯ captureIfaceParamType(AstRefDType* nodep, AstParamTypeDType* defp) {
        ಯದಿ (!V3LinkDotIfaceCapture::enabled() || !m_statep->forPrimary()) ಪ್ರತ್ಯಾಗಮ;
        Ast_ಗಂಟು_Module* ಧ್ರುವ defOwnerModp = V3LinkDotIfaceCapture::findOwnerModule(defp);
        ಯದಿ (!defOwnerModp || !VN_IS(defOwnerModp, Iface)) ಪ್ರತ್ಯಾಗಮ;
        AstCell* ಧ್ರುವ cellForCapture
            = m_ds.m_dotSymp ? VN_CAST(m_ds.m_dotSymp->nodep(), Cell) : nullptr;
        ಯದಿ (!cellForCapture) ಪ್ರತ್ಯಾಗಮ;
        UINFO(9, indent() << "iface capture add paramtype " << nodep
                          << " iface=" << defOwnerModp->prettyNameQ());
        V3LinkDotIfaceCapture::addParamType(nodep, cellForCapture->name(), m_modp, defp,
                                            defOwnerModp->name());
    }

    Ast_ಗಂಟು_Stmt* addImplicitSuperNewCall(AstFunc* ಧ್ರುವ nodep,
                                         ಧ್ರುವ AstClassExtends* ಧ್ರುವ classExtendsp) {
        // Returns the added node
        File_ಸಾಲು* ಧ್ರುವ fl = nodep->fileline();
        AstArg* argsp = nullptr;
        ಯದಿ (classExtendsp->argsp()) argsp = classExtendsp->argsp()->clone_ವೃಕ್ಷ(ಸತ್ಯ);
        AstNew* ಧ್ರುವ newExprp = ನೂತನ AstNew{fl, argsp};
        newExprp->isImplicit(ಸತ್ಯ);
        AstDot* ಧ್ರುವ superNewp = ನೂತನ AstDot{fl, ಅಸತ್ಯ, ನೂತನ AstParseRef{fl, "super"}, newExprp};
        Ast_ಗಂಟು_Stmt* ಧ್ರುವ superNewStmtp = superNewp->makeStmt();
        ಪ್ರತಿ (Ast_ಗಂಟು* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            // super.new shall be the first statement (IEEE 1800-2023 8.15)
            // but some nodes (such as variable declarations and typedefs) should stay before
            ಯದಿ (VN_IS(stmtp, ಗಂಟು_Stmt)) {
                stmtp->addHereThisAsNext(superNewStmtp);
                ಪ್ರತ್ಯಾಗಮ superNewStmtp;
            }
        }
        // There were no statements
        nodep->addStmtsp(superNewStmtp);
        ಪ್ರತ್ಯಾಗಮ superNewStmtp;
    }
    ಶೂನ್ಯ checkNoDot(Ast_ಗಂಟು* nodep) {
        ಯದಿ (VL_UNLIKELY(!nodep)) {
            UINFO(9, indent() << "iface capture null node passed to checkNoDot; dot state="
                              << m_ds.ascii());
            m_ds.m_dotErr = ಸತ್ಯ;
            ಪ್ರತ್ಯಾಗಮ;
        }
        ಯದಿ (VL_UNLIKELY(m_ds.m_dotPos != DP_NONE)) {
            UINFO(9, indent() << "ds=" << m_ds.ascii());
            UINFO(9, indent() << "iface capture checkNoDot hit node=" << nodep
                              << " dotState=" << m_ds.ascii());
            ಯದಿ (VL_UNLIKELY(!m_ds.m_dotp)) {
                nodep->v3error("Syntax error: Not expecting " << nodep->type()
                                                              << " here (missing dot context)");
            } ಅನ್ಯಥಾ {
                nodep->v3error("Syntax error: Not expecting "
                               << nodep->type() << " under a " << nodep->backp()->type()
                               << " in dotted expression\n"
                               << nodep->warnContextPrimary() << m_ds.m_dotp->warnOther()
                               << "... Resolving this reference\n"
                               << m_ds.m_dotp->warnContextSecondary());
            }
            m_ds.m_dotErr = ಸತ್ಯ;
        }
    }
    AstVar* find_Iface_ತುದಿ_Varp(Ast_ಗಂಟು* nodep, VSymEnt* parentEntp, ಧ್ರುವ string& name) {
        ಧ್ರುವ string findName = name + "__Viftop";
        VSymEnt* ಧ್ರುವ ifaceSymp = parentEntp->findIdFallback(findName);
        AstVar* ಧ್ರುವ iface_ತುದಿ_Varp = ifaceSymp ? VN_AS(ifaceSymp->nodep(), Var) : nullptr;
        UASSERT_OBJ(iface_ತುದಿ_Varp, nodep, "Can't find interface var ref: " << findName);
        ಪ್ರತ್ಯಾಗಮ iface_ತುದಿ_Varp;
    }
    AstRSProd* findProd(Ast_ಗಂಟು* nodep, VSymEnt* parentEntp, ಧ್ರುವ string& name) {
        ಧ್ರುವ VSymEnt* ಧ್ರುವ foundp = parentEntp->findIdFallback(name);
        AstRSProd* foundNodep = foundp ? VN_CAST(foundp->nodep(), RSProd) : nullptr;
        ಯದಿ (!foundNodep) {
            VSpellCheck speller;
            Link_ಗಂಟು_Matcher_Prod matcher;
            parentEntp->candidateIdFlat(&speller, &matcher, ಅಸತ್ಯ);
            ಧ್ರುವ string suggest = speller.best_Candidate_ಸಂದೇಶ(name);
            UINFO(1, "   ErrParseRef curSymp=se" << cvtToHex(parentEntp));
            nodep->v3error("Production " << Ast_ಗಂಟು::prettyNameQ(name) << " not found\n"
                                         << (suggest.empty() ? "" : nodep->warnMore() + suggest));
            ಪ್ರತ್ಯಾಗಮ nullptr;
        }
        ಪ್ರತ್ಯಾಗಮ foundNodep;
    }
    ಶೂನ್ಯ duplicatePinError(AstPin* nodep, Ast_ಗಂಟು* origp, ಧ್ರುವ ವರ್ಣ* whatp) {
        nodep->v3error("Duplicate " << whatp << " connection: " << nodep->prettyNameQ() << '\n'
                                    << nodep->warnContextPrimary() << '\n'
                                    << origp->warnOther() << "... Location of original " << whatp
                                    << " connection\n"
                                    << origp->warnContextSecondary());
    }
    ಶೂನ್ಯ markAndCheckPinDup(AstPin* nodep, Ast_ಗಂಟು* refp, ಧ್ರುವ ವರ್ಣ* whatp) {
        ಧ್ರುವ auto pair = m_usedPins.emplace(refp, nodep);
        ಯದಿ (pair.second) {
            ಯದಿ (!nodep->paramPath().empty())
                m_usedDefParamPins[refp].emplace(nodep->paramPath(), nodep);
            ಪ್ರತ್ಯಾಗಮ;
        } ಅನ್ಯಥಾ {
            Ast_ಗಂಟು* ಧ್ರುವ origp = pair.first->second;
            ಯದಿ (nodep->paramPath().empty() || VN_AS(origp, Pin)->paramPath().empty()) {
                duplicatePinError(nodep, origp, whatp);
                ಪ್ರತ್ಯಾಗಮ;
            }
        }

        auto& defParamPins = m_usedDefParamPins[refp];
        ಧ್ರುವ auto defParamIt = defParamPins.find(nodep->paramPath());
        ಯದಿ (defParamIt != defParamPins.end()) {
            duplicatePinError(nodep, defParamIt->second, whatp);
            ಪ್ರತ್ಯಾಗಮ;
        }
        defParamPins.emplace(nodep->paramPath(), nodep);
    }
    ಸ್ಥಿರ ತಾರ್ಕಿಕ extendsClass(ಧ್ರುವ AstClass* classp, ಧ್ರುವ AstClass* targetp) {
        // Return true if classp inherits from targetp through already resolved extends
        // If performance of this becomes a problem, use just a counter to check
        // exceeds some number of iterations then perform the more expensive analysis
        std::set<ಧ್ರುವ AstClass*> visited;
        std::vector<ಧ್ರುವ AstClass*> todo{classp};
        ಯಾವತ್ (!todo.empty()) {
            ಧ್ರುವ AstClass* ಧ್ರುವ currp = todo.back();
            todo.ತೆಗೆ_back();
            ಯದಿ (currp == targetp) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
            ಯದಿ (!visited.ಸೇರಿಸು(currp).second) ಅನುವರ್ತ;
            ಪ್ರತಿ (ಧ್ರುವ AstClassExtends* cextp = currp->extendsp(); cextp;
                 cextp = VN_AS(cextp->nextp(), ClassExtends)) {
                ಯದಿ (ಧ್ರುವ AstClass* ಧ್ರುವ basep = cextp->classOrNullp()) todo.ತಳ್ಳು_back(basep);
            }
        }
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }
    ಸ್ಥಿರ AstClocking* sensClockingp(Ast_ಗಂಟು* nodep) {
        // Return the clocking block referenced by nodep, either directly or through a modport
        ಯದಿ (AstClocking* ಧ್ರುವ clockingp = VN_CAST(nodep, Clocking)) ಪ್ರತ್ಯಾಗಮ clockingp;
        ಯದಿ (ಧ್ರುವ AstModportClockingRef* ಧ್ರುವ clockingRefp = VN_CAST(nodep, ModportClockingRef))
            ಪ್ರತ್ಯಾಗಮ clockingRefp->clockingp();
        ಪ್ರತ್ಯಾಗಮ nullptr;
    }
    VSymEnt* getCreateClockingEventSymEnt(AstClocking* clockingp) {
        AstVar* ಧ್ರುವ eventp = clockingp->ensureEventp(ಸತ್ಯ);
        ಯದಿ (!eventp->user1p()) eventp->user1p(ನೂತನ VSymEnt{m_statep->symsp(), eventp});
        ಪ್ರತ್ಯಾಗಮ reinterpret_cast<VSymEnt*>(eventp->user1p());
    }
    VSymEnt* findInlinedDotsSym(Ast_ಗಂಟು* nodep, ಧ್ರುವ string& inlinedDots) {
        // Return scope that applies for searching, given renaming due to module inlining.
        // Dotted lookup is always relative to module, as maybe variable
        // name lower down with same scope name we want to ignore (t_math_divw)
        VSymEnt* dotSymp = m_modSymp;
        ಧ್ರುವ string inl = Ast_ಗಂಟು::dedotName(inlinedDots);
        string baddot;
        VSymEnt* okSymp = nullptr;
        dotSymp = m_statep->findDotted(nodep->fileline(), dotSymp, inl, baddot, okSymp, ಅಸತ್ಯ);
        UASSERT_OBJ(dotSymp, nodep,
                    "Couldn't resolve inlined scope " << Ast_ಗಂಟು::prettyNameQ(baddot)
                                                      << " in: " << inlinedDots);
        UINFO(9, indent() << "findInlinedDotsSym " << dotSymp
                          << " search start due to inlinedDots=" << inlinedDots);
        ಪ್ರತ್ಯಾಗಮ dotSymp;
    }
    VSymEnt* findDottedRefSymp(Ast_ಗಂಟು* nodep, VSymEnt* lookupSymp, ಧ್ರುವ string& dotted,
                               string& baddot, VSymEnt*& okSymp, ತಾರ್ಕಿಕ* modportp = nullptr) {
        // Return scope to search for a relinked hierarchical reference
        // (AstVarXRef or AstEnumItemRef), or nullptr after error if not found
        // If modportp is given, set true when the reference went through a modport
        ಯದಿ (modportp) *modportp = ಅಸತ್ಯ;
        VSymEnt* dotSymp = m_statep->findDotted(nodep->fileline(), lookupSymp, dotted, baddot,
                                                okSymp, ಸತ್ಯ);  // Maybe nullptr
        ಯದಿ (!dotSymp) {
            nodep->v3error(
                "Can't find definition of "
                << (!baddot.empty() ? Ast_ಗಂಟು::prettyNameQ(baddot) : nodep->prettyNameQ()) << '\n'
                << nodep->warnContextPrimary());
            ಪ್ರತ್ಯಾಗಮ nullptr;
        }
        ಯದಿ (ಧ್ರುವ AstVar* ಧ್ರುವ varp = VN_CAST(dotSymp->nodep(), Var)) {
            ಯದಿ (ಧ್ರುವ AstIfaceRefDType* ಧ್ರುವ ifaceRefp
                = VN_CAST(varp->childDTypep(), IfaceRefDType)) {
                ಯದಿ (ifaceRefp->modportp()) {
                    dotSymp = m_statep->get_ಗಂಟು_Sym(ifaceRefp->modportp());
                    ಯದಿ (modportp) *modportp = ಸತ್ಯ;
                } ಅನ್ಯಥಾ {
                    dotSymp = m_statep->get_ಗಂಟು_Sym(ifaceRefp->ifacep());
                }
            }
        } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstModportClockingRef* ಧ್ರುವ clockingRefp
                   = VN_CAST(dotSymp->nodep(), ModportClockingRef)) {
            dotSymp = m_statep->get_ಗಂಟು_Sym(clockingRefp->clockingp());
        }
        ಪ್ರತ್ಯಾಗಮ dotSymp;
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ isParamedClassRefDType(ಧ್ರುವ Ast_ಗಂಟು* classp) {
        ಯಾವತ್ (ಧ್ರುವ AstRefDType* ಧ್ರುವ refp = VN_CAST(classp, RefDType))
            classp = refp->subDTypep();
        ಪ್ರತ್ಯಾಗಮ (VN_IS(classp, ClassRefDType) && VN_AS(classp, ClassRefDType)->paramsp())
               || VN_IS(classp, ParamTypeDType);
    }
    ತಾರ್ಕಿಕ isParamedClassRef(ಧ್ರುವ Ast_ಗಂಟು* nodep) {
        // Is this a parameterized reference to a class, or a reference to class parameter
        ಯದಿ (ಧ್ರುವ auto* classRefp = VN_CAST(nodep, ClassOrPackageRef)) {
            ಯದಿ (classRefp->paramsp()) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
            ಧ್ರುವ auto* classp = classRefp->classOrPackageNodep();
            ಯಾವತ್ (ಧ್ರುವ auto* typedefp = VN_CAST(classp, Typedef)) classp = typedefp->subDTypep();
            ಪ್ರತ್ಯಾಗಮ isParamedClassRefDType(classp);
        }
        ಪ್ರತ್ಯಾಗಮ isParamedClassRefDType(nodep);
    }
    VSymEnt* getThisClassSymp() {
        VSymEnt* classSymp = m_ds.m_dotSymp;
        ಯಾವತ್ (classSymp && !VN_IS(classSymp->nodep(), Class)) {
            classSymp = classSymp->parentp();
        }
        ಪ್ರತ್ಯಾಗಮ classSymp;
    }
    // Is the current scope within the given class, including within its nested classes
    ತಾರ್ಕಿಕ insideClass(ಧ್ರುವ AstClass* classp) ಧ್ರುವ {
        ಪ್ರತಿ (ಧ್ರುವ VSymEnt* symp = m_curSymp; symp; symp = symp->parentp()) {
            ಯದಿ (symp->nodep() == classp) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        }
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }
    // 'randomize() with' target class not yet specialized; defer to post-V3Param pass
    ತಾರ್ಕಿಕ randTargetUnresolved() ಧ್ರುವ {
        ಪ್ರತ್ಯಾಗಮ m_statep->forPrimary() && m_currentWithp && m_randMethodCallp && !m_randSymp;
    }
    // Is 'funcp' of 'classp' virtual, directly or by overriding a virtual method of a base
    // class (IEEE 1800-2023 8.20)
    ತಾರ್ಕಿಕ isVirtualMethod(ಧ್ರುವ Ast_ಗಂಟು_FTask* funcp, ಧ್ರುವ AstClass* classp) ಧ್ರುವ {
        ಯದಿ (funcp->isVirtual()) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        ಪ್ರತಿ (ಧ್ರುವ AstClassExtends* cextp = classp->extendsp(); cextp;
             cextp = VN_AS(cextp->nextp(), ClassExtends)) {
            AstClass* ಧ್ರುವ basep = cextp->classOrNullp();
            ಯದಿ (!basep) ಅನುವರ್ತ;
            ಧ್ರುವ VSymEnt* ಧ್ರುವ foundp = m_statep->get_ಗಂಟು_Sym(basep)->findIdFlat(funcp->name());
            ಧ್ರುವ Ast_ಗಂಟು_FTask* ಧ್ರುವ baseFuncp
                = foundp ? VN_CAST(foundp->nodep(), ಗಂಟು_FTask) : nullptr;
            ಧ್ರುವ AstClass* ಧ್ರುವ baseClassp = foundp && foundp->parentp()
                                                   ? VN_CAST(foundp->parentp()->nodep(), Class)
                                                   : nullptr;
            ಯದಿ (baseFuncp && baseClassp && isVirtualMethod(baseFuncp, baseClassp)) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        }
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }
    // Is imported 'implp' an implementation of interface method 'ifSymp' (IEEE 1800-2023
    // 8.26.2)? If its class does not derive from the interface class, it needs a forwarding
    // override in the derived class to be reachable through the interface.
    ತಾರ್ಕಿಕ inheritedImplements(ಧ್ರುವ VSymEnt* implp, ಧ್ರುವ VSymEnt* ifSymp, ತಾರ್ಕಿಕ& needForwardr) {
        needForwardr = ಅಸತ್ಯ;
        ಧ್ರುವ Ast_ಗಂಟು_FTask* ಧ್ರುವ implFuncp = VN_CAST(implp->nodep(), ಗಂಟು_FTask);
        ಯದಿ (!implFuncp || implFuncp->pureVirtual() || implFuncp->isStatic()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಧ್ರುವ AstClass* ಧ್ರುವ implClassp
            = implp->parentp() ? VN_CAST(implp->parentp()->nodep(), Class) : nullptr;
        ಧ್ರುವ AstClass* ಧ್ರುವ ifClassp
            = ifSymp->parentp() ? VN_CAST(ifSymp->parentp()->nodep(), Class) : nullptr;
        ಯದಿ (!implClassp || !ifClassp) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಯದಿ (extendsClass(implClassp, ifClassp)) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        ಯದಿ (!isVirtualMethod(implFuncp, implClassp)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        needForwardr = ಸತ್ಯ;
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }
    // Add 'implp' override to 'classp' that calls super.implp, so the C++ method of an
    // implemented interface class is overridden
    ಶೂನ್ಯ addInterfaceForward(AstClass* classp, Ast_ಗಂಟು_FTask* implp, AstClass* implClassp) {
        File_ಸಾಲು* ಧ್ರುವ flp = implp->fileline();
        AstVar* fvarp = nullptr;
        Ast_ಗಂಟು_FTask* newp;
        ಯದಿ (AstFunc* ಧ್ರುವ implFuncp = VN_CAST(implp, Func)) {
            fvarp = VN_AS(implFuncp->fvarp(), Var)->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);
            newp = ನೂತನ AstFunc{flp, implp->name(), nullptr, fvarp};
        } ಅನ್ಯಥಾ {
            newp = ನೂತನ AstTask{flp, implp->name(), nullptr};
        }
        newp->classMethod(ಸತ್ಯ);
        newp->isVirtual(ಸತ್ಯ);
        newp->lifetime(implp->lifetime());
        AstArg* argsp = nullptr;
        ಪ್ರತಿ (Ast_ಗಂಟು* stmtp = implp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstVar* ಧ್ರುವ portp = VN_CAST(stmtp, Var);
            ಯದಿ (!portp || !portp->isIO()) ಅನುವರ್ತ;
            AstVar* ಧ್ರುವ newPortp = portp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);
            newp->addStmtsp(newPortp);
            argsp = Ast_ಗಂಟು::addNext(
                argsp, ನೂತನ AstArg{flp, "", ನೂತನ AstVarRef{flp, newPortp, VAccess::READ}});
        }
        Ast_ಗಂಟು_FTask_Ref* callp;
        ಯದಿ (AstFunc* ಧ್ರುವ implFuncp = VN_CAST(implp, Func)) {
            callp = ನೂತನ AstFuncRef{flp, implFuncp, argsp};
        } ಅನ್ಯಥಾ {
            callp = ನೂತನ AstTaskRef{flp, VN_AS(implp, Task), argsp};
        }
        callp->classOrPackagep(implClassp);
        callp->superReference(ಸತ್ಯ);
        ಯದಿ (fvarp) {
            newp->addStmtsp(ನೂತನ AstAssign{flp, ನೂತನ AstVarRef{flp, fvarp, VAccess::WRITE}, callp});
        } ಅನ್ಯಥಾ {
            newp->addStmtsp(callp->makeStmt());
        }
        UINFO(9, indent() << "Interface forward " << newp);
        classp->addMembersp(newp);
    }
    ಶೂನ್ಯ importDerivedClass(AstClass* derivedClassp, VSymEnt* baseSymp, AstClass* baseClassp) {
        // Also used for standard 'extends' from a base class
        UINFO(8, indent() << "importDerivedClass to " << derivedClassp << " from " << baseClassp);
        ಪ್ರತಿ (VSymEnt::const_iterator it = baseSymp->begin(); it != baseSymp->end(); ++it) {
            ಯದಿ (Ast_ಗಂಟು* baseSubp = it->second->nodep()) {
                UINFO(8, indent() << "  SymFunc " << baseSubp);
                ಧ್ರುವ string impOrExtends
                    = baseClassp->isInterfaceClass() ? " implements " : " extends ";
                ಯದಿ (VN_IS(baseSubp, ಗಂಟು_FTask)) {
                    ಧ್ರುವ VSymEnt* ಧ್ರುವ foundp = m_curSymp->findIdFlat(baseSubp->name());
                    ಧ್ರುವ Ast_ಗಂಟು_FTask* ಧ್ರುವ baseFuncp = VN_CAST(baseSubp, ಗಂಟು_FTask);
                    ಯದಿ (!baseFuncp || !baseFuncp->pureVirtual()) ಅನುವರ್ತ;
                    ತಾರ್ಕಿಕ needForward = ಅಸತ್ಯ;
                    ಧ್ರುವ ತಾರ್ಕಿಕ existsInDerived
                        = foundp
                          && (!foundp->imported()
                              || inheritedImplements(foundp, it->second, needForward));
                    ಯದಿ (needForward && m_statep->forPrimary()
                        && !derivedClassp->isInterfaceClass()) {
                        m_ifForwards.emplace(baseSubp->name(), foundp);
                    }
                    ಯದಿ (m_statep->forPrimary() && !existsInDerived
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
                    ಧ್ರುವ auto itn = m_ifClassImpNames.find(baseSubp->name());
                    ಯದಿ (m_statep->forPrimary() && !existsInDerived
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
                ಯದಿ (VN_IS(baseSubp, Constraint)) {
                    ಧ್ರುವ VSymEnt* ಧ್ರುವ foundp = m_curSymp->findIdFlat(baseSubp->name());
                    ಧ್ರುವ AstConstraint* ಧ್ರುವ baseFuncp = VN_CAST(baseSubp, Constraint);
                    ಯದಿ (!baseFuncp || !baseFuncp->isKwdPure()) ಅನುವರ್ತ;
                    ಧ್ರುವ ತಾರ್ಕಿಕ existsInDerived = foundp && !foundp->imported();
                    ಯದಿ (m_statep->forPrimary() && !existsInDerived
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
    ಶೂನ್ಯ importSymbolsFromExtended(AstClass* ಧ್ರುವ nodep, AstClassExtends* ಧ್ರುವ cextp) {
        AstClass* ಧ್ರುವ baseClassp = cextp->classp();
        VSymEnt* ಧ್ರುವ srcp = m_statep->get_ಗಂಟು_Sym(baseClassp);
        importDerivedClass(nodep, srcp, baseClassp);
        ಯದಿ (!cextp->isImplements()) m_curSymp->importFromClass(m_statep->symsp(), srcp);
    }
    ಶೂನ್ಯ classExtendImport(AstClass* nodep) {
        // A class reference might be to a class that is later in Ast due to
        // e.g. parameterization or referring to a "class (type T) extends T"
        // Resolve it so later Class:: references into its base classes work
        symIterateNull(nodep, m_statep->get_ಗಂಟು_Sym(nodep));
    }

    ಶೂನ್ಯ checkDeclOrder(Ast_ಗಂಟು* nodep, Ast_ಗಂಟು* declp) {
        ಧ್ರುವ uint32_t decl_Token_ಅಂಕೆ = declp->decl_Token_ಅಂಕೆ();
        ಯದಿ (!decl_Token_ಅಂಕೆ) ಪ್ರತ್ಯಾಗಮ;  // Not implemented/valid on this object
        ಯದಿ (nodep->fileline()->token_ಅಂಕೆ() < decl_Token_ಅಂಕೆ) {
            UINFO(1, "Related node " << nodep->fileline()->token_ಅಂಕೆ() << " " << nodep);
            UINFO(1, "Related decl " << decl_Token_ಅಂಕೆ << " " << declp);
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
    ಶೂನ್ಯ checkMemberDeclOrder(Ast_ಗಂಟು* nodep, AstMemberDType* declp) {
        ಧ್ರುವ uint32_t decl_Token_ಅಂಕೆ = declp->fileline()->token_ಅಂಕೆ();
        ಯದಿ (nodep->fileline()->token_ಅಂಕೆ() < decl_Token_ಅಂಕೆ) {
            UINFO(1, "Related node " << nodep->fileline()->token_ಅಂಕೆ() << " " << nodep);
            UINFO(1, "Related decl " << decl_Token_ಅಂಕೆ << " " << declp);
            nodep->v3error("Reference to "
                           << nodep->prettyNameQ() << " before declaration (IEEE 1800-2023 6.18)\n"
                           << nodep->warnMore()
                           << "... Suggest move the declaration before the reference\n"
                           << nodep->warnContextPrimary() << '\n'
                           << declp->warnOther() << "... Location of original declaration\n"
                           << declp->warnContextSecondary());
        }
    }

    ಶೂನ್ಯ replaceWithCheckBreak(Ast_ಗಂಟು* oldp, Ast_ಗಂಟು_DType* newp) {
        // Flag now to avoid V3Broken throwing an internal error
        ಯದಿ (oldp->wouldBreak(newp)) {
            newp->v3error(
                "Data type used where a non-data type is expected: " << newp->prettyNameQ());
            oldp->replaceWith(ನೂತನ AstConst{newp->fileline(), AstConst::BitFalseErroring{}});
        } ಅನ್ಯಥಾ {
            oldp->replaceWith(newp);
        }
    }

    // Marks the current module to be revisited after the initial AST iteration
    ಶೂನ್ಯ revisitLater(Ast_ಗಂಟು* deferredNodep) {
        // Need to revisit entire module to build up all the necessary context
        m_lastDeferredp = deferredNodep;
        m_modulesToRevisit.ಸೇರಿಸು(std::make_pair(m_modp->name(), m_modp));
    }

    ಶೂನ್ಯ updateVarUse(AstVar* nodep) {
        // Avoid dotted.PARAM false positive when in a parameter block
        // that is if ()'ed off by same dotted name as another block
        ಯದಿ (nodep && nodep->isParam()) nodep->usedParam(ಸತ್ಯ);
    }

    ಸ್ಥಿರ VSymEnt* findIdFallbackSkipMemberDType(VSymEnt* lookp, ಧ್ರುವ string& name) {
        VSymEnt* shadowEntp = nullptr;  // Shadowing variable: not a type, kept for error report
        ಯಾವತ್ (lookp) {
            VSymEnt* ಧ್ರುವ foundp = lookp->findIdFlat(name);
            ಯದಿ (foundp && !VN_IS(foundp->nodep(), MemberDType)) {
                // Non-type entries are not type candidates
                // (IEEE 1800-2023 6.18); skip them so an enclosing
                // type is found, but keep one to preserve the "found: ..." error.
                ಯದಿ (VN_IS(foundp->nodep(), Typedef) || VN_IS(foundp->nodep(), ParamTypeDType)
                    || VN_IS(foundp->nodep(), Class)) {
                    ಪ್ರತ್ಯಾಗಮ foundp;
                }
                ಯದಿ (!shadowEntp) shadowEntp = foundp;
            }
            lookp = lookp->fallbackp();
        }
        ಪ್ರತ್ಯಾಗಮ shadowEntp;
    }

    ಶೂನ್ಯ symIterateChildren(Ast_ಗಂಟು* nodep, VSymEnt* symp) {
        // Iterate children, changing to given context, with restore to old context
        VL_RESTORER_COPY(m_ds);
        VL_RESTORER(m_curSymp);
        m_curSymp = symp;
        m_ds.init(m_curSymp);
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ symIterateNull(Ast_ಗಂಟು* nodep, VSymEnt* symp) {
        // Iterate node, changing to given context, with restore to old context
        VL_RESTORER_COPY(m_ds);
        VL_RESTORER(m_curSymp);
        m_curSymp = symp;
        m_ds.init(m_curSymp);
        iterateNull(nodep);
    }
    ಸ್ಥಿರ ಧ್ರುವ Ast_ಗಂಟು_DType* get_ಅಭಿವ್ಯಕ್ತಿ_DTypep(ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* selp) {
        ಯಾವತ್ (ಧ್ರುವ Ast_ಗಂಟು_Pre_Sel* ಧ್ರುವ sp = VN_CAST(selp, ಗಂಟು_Pre_Sel)) selp = sp->fromp();
        ಯದಿ (ಧ್ರುವ AstMemberSel* ಧ್ರುವ sp = VN_CAST(selp, MemberSel)) {
            ಯದಿ (ಧ್ರುವ Ast_ಗಂಟು_DType* dtypep = get_ಅಭಿವ್ಯಕ್ತಿ_DTypep(sp->fromp())) {
                ಯದಿ (ಧ್ರುವ AstClassRefDType* ಧ್ರುವ classRefp = VN_CAST(dtypep, ClassRefDType)) {
                    ಧ್ರುವ AstClass* ಧ್ರುವ classp = classRefp->classp();
                    ಧ್ರುವ ತಾರ್ಕಿಕ found = classp->existsMember(
                        [&dtypep, name = selp->name()](ಧ್ರುವ AstClass*, ಧ್ರುವ AstVar* nodep) {
                            dtypep = nodep->childDTypep();
                            ಪ್ರತ್ಯಾಗಮ nodep->name() == name;
                        });
                    ಯದಿ (found) ಪ್ರತ್ಯಾಗಮ dtypep->elemDTypep();
                    selp->v3error("Class " << classRefp->prettyNameQ()
                                           << " does not contain field " << selp->prettyNameQ());
                } ಅನ್ಯಥಾ {
                    selp->v3fatalSrc("Member selection on expression of type "
                                     << dtypep->prettyDTypeNameQ()
                                     << ", which is not a class type");
                }
            }
        } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ Ast_ಗಂಟು_Var_Ref* ಧ್ರುವ varRefp = VN_CAST(selp, ಗಂಟು_Var_Ref)) {
            ಪ್ರತ್ಯಾಗಮ varRefp->varp()->childDTypep()->elemDTypep();
        }
        ಪ್ರತ್ಯಾಗಮ nullptr;
    }
    ಸ್ಥಿರ ಧ್ರುವ AstVar* getNextVarp(ಧ್ರುವ Ast_ಗಂಟು* stmtsp) {
        // Only IO ports, as parameters are on paramsp(), not the pinsp() list paired here
        ಯಾವತ್ (stmtsp) {
            ಯದಿ (ಧ್ರುವ AstVar* ಧ್ರುವ varp = VN_CAST(stmtsp, Var)) {
                ಯದಿ (varp->isIO()) ಪ್ರತ್ಯಾಗಮ varp;
            }
            stmtsp = stmtsp->nextp();
        }
        ಪ್ರತ್ಯಾಗಮ nullptr;
    }
    // Introduce implicit parameters for modules with generic interafeces
    ಶೂನ್ಯ addImplicitParametersOfGenericIface(AstCell* ಧ್ರುವ nodep, ಧ್ರುವ AstModule* ಧ್ರುವ modp) {
        // Get Param number
        ಪೂರ್ಣಾಙ್ಕ param_ಅಂಕೆ = 1;
        ಪ್ರತಿ (AstPin* paramp = nodep->paramsp(); paramp; paramp = VN_CAST(paramp->nextp(), Pin)) {
            ++param_ಅಂಕೆ;
        }

        // Add each implicit parameter type
        ಧ್ರುವ AstVar* modIfaceVarp = getNextVarp(modp->stmtsp());
        ಪ್ರತಿ (ಧ್ರುವ AstPin* pinp = nodep->pinsp(); pinp && modIfaceVarp;
             pinp = VN_CAST(pinp->nextp(), Pin),
                           modIfaceVarp = getNextVarp(modIfaceVarp->nextp())) {
            ಯದಿ (modIfaceVarp->varType() != VVarType::IFACEREF
                || !VN_IS(modIfaceVarp->childDTypep()->elemDTypep(), IfaceGenericDType)) {
                ಅನುವರ್ತ;
            }
            Ast_ಗಂಟು* exprp = pinp->exprp();
            ಯದಿ (!exprp) {
                modIfaceVarp->v3error("Interface port "
                                      << modIfaceVarp->prettyNameQ()
                                      << " is not connected to interface/modport pin expression");
                ಅನುವರ್ತ;
            }
            ಯಾವತ್ (ಧ್ರುವ Ast_ಗಂಟು_Pre_Sel* ಧ್ರುವ preSelp = VN_CAST(exprp, ಗಂಟು_Pre_Sel)) {
                exprp = preSelp->fromp();
            }
            // Resolve pin expression to the enclosing module's port Var. At primary
            // LinkDot the expression may still be an AstParseRef, so also look up by name.
            AstVar* enclosingVarp = nullptr;
            ಧ್ರುವ AstVarRef* ಧ್ರುವ varRefp = VN_CAST(exprp, VarRef);
            ಯದಿ (varRefp) {
                enclosingVarp = varRefp->varp();
            } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstParseRef* ಧ್ರುವ parseRefp = VN_CAST(exprp, ParseRef)) {
                ಯದಿ (m_modp) {
                    ಯದಿ (VSymEnt* ಧ್ರುವ symp
                        = m_statep->get_ಗಂಟು_Sym(m_modp)->findIdFlat(parseRefp->name())) {
                        enclosingVarp = VN_CAST(symp->nodep(), Var);
                    }
                }
            }
            ಯದಿ (enclosingVarp && enclosingVarp->varType() == VVarType::IFACEREF
                && VN_IS(enclosingVarp->childDTypep()->skipRefp(), IfaceGenericDType)) {
                // Nested generic-iface forwarding (#7454): enclosing port is itself still
                // generic, so emit a placeholder __VGIfaceParam pin carrying a VarRef to
                // the outer port. V3Param rewrites it to the concrete IfaceRefDType once
                // the enclosing module is specialized (see
                // ParamProcessor::resolveGenericIfaceForwardingPins for the ordering
                // constraint that keeps the rewrite inside V3Param).
                AstVarRef* ಧ್ರುವ fwdRefp
                    = ನೂತನ AstVarRef{exprp->fileline(), enclosingVarp, VAccess::READ};
                AstPin* ಧ್ರುವ newPinp = ನೂತನ AstPin{
                    pinp->fileline(), param_ಅಂಕೆ, "__VGIfaceParam" + modIfaceVarp->name(), fwdRefp};
                newPinp->param(ಸತ್ಯ);
                visit(newPinp);
                nodep->addParamsp(newPinp);
            } ಅನ್ಯಥಾ ಯದಿ (varRefp) {
                ಧ್ರುವ AstVar* ಧ್ರುವ varp = varRefp->varp();
                ಯದಿ (ಧ್ರುವ AstIfaceRefDType* ಧ್ರುವ refp
                    = VN_CAST(varp->childDTypep()->elemDTypep(), IfaceRefDType)) {
                    AstIface* ಧ್ರುವ ifacep = VN_AS(refp->cellp()->modp(), Iface);
                    AstIfaceRefDType* newIfaceRefp;
                    ಯದಿ (refp->modportp()) {
                        newIfaceRefp = ನೂತನ AstIfaceRefDType{
                            refp->fileline(), refp->modportFileline(), m_modp->name(),
                            ifacep->name(), refp->modportName()};
                    } ಅನ್ಯಥಾ {
                        newIfaceRefp = ನೂತನ AstIfaceRefDType{refp->fileline(), m_modp->name(),
                                                            ifacep->name()};
                    }
                    newIfaceRefp->ifacep(ifacep);
                    ಯದಿ (refp->cellp()->paramsp()) {
                        newIfaceRefp->addParamsp(refp->cellp()->paramsp()->clone_ವೃಕ್ಷ(ಸತ್ಯ));
                    }
                    UASSERT_OBJ(pinp->name().find("__pinNumber") == 0
                                    || pinp->name() == modIfaceVarp->name(),
                                pinp, "Not found interface with such name");
                    AstPin* ಧ್ರುವ newPinp
                        = ನೂತನ AstPin{pinp->fileline(), param_ಅಂಕೆ,
                                     "__VGIfaceParam" + modIfaceVarp->name(), newIfaceRefp};
                    newPinp->param(ಸತ್ಯ);
                    visit(newPinp);
                    nodep->addParamsp(newPinp);
                } ಅನ್ಯಥಾ {
                    varRefp->v3error("Generic interfaces can only connect to an interface and "
                                     << varp->prettyNameQ() << " is "
                                     << (varp->childDTypep()
                                             ? "of type " + varp->childDTypep()->prettyDTypeNameQ()
                                             : "not an interface"));
                }
            } ಅನ್ಯಥಾ {
                exprp->v3error("Expected an interface but " << exprp->prettyNameQ()
                                                            << " is not an interface");
            }
            ++param_ಅಂಕೆ;
        }
    }

    // Returns true and issues error iff 'varp' cannot be accesed with a VarXRef
    // in a hier_block given by 'scopeSymp'
    ತಾರ್ಕಿಕ errorHierNonPort(AstVarXRef* refp, AstVar* varp, VSymEnt* scopeSymp) {
        // OK to access ports on hier_block
        ಯದಿ (varp->isIO()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        // OK if not an instance
        ಧ್ರುವ AstCell* ಧ್ರುವ cellp = VN_CAST(scopeSymp->nodep(), Cell);
        ಯದಿ (!cellp) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        // Ok if not a hier_block
        ಧ್ರುವ Ast_ಗಂಟು_Module* ಧ್ರುವ modp = cellp->modp();
        ಯದಿ (!modp->hierBlock()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        // Bad
        refp->v3error("Cannot access non-port symbols inside hierarchical block");
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }

#define LINKDOT_VISIT_START() \
    VL_RESTORER(m_indent); \
    ++m_indent;

    string indent() ಧ್ರುವ {
        string result = "";
        result.ಸೇರಿಸು(0, m_indent, ':');
        ಪ್ರತ್ಯಾಗಮ result + " ";
    }

    // VISITORS
    ಶೂನ್ಯ visit(AstNetlist* nodep) override {
        // Recurse..., backward as must do packages before using packages
        iterateChildrenBackwardsConst(nodep);
    }
    ಶೂನ್ಯ visit(AstTypeTable*) override {}
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Module* nodep) override {
        ಯದಿ (nodep->dead()) ಪ್ರತ್ಯಾಗಮ;
        LINKDOT_VISIT_START();
        UINFO(8, indent() << "visit " << nodep);
        VL_RESTORER(m_genericIfaceModule);
        ಯದಿ (ಧ್ರುವ AstModule* ಧ್ರುವ modp = VN_CAST(nodep, Module)) {
            m_genericIfaceModule = modp->hasGenericIface();
        }
        checkNoDot(nodep);
        m_ds.init(m_curSymp);
        m_ds.m_dotSymp = m_curSymp = m_modSymp
            = m_statep->get_ಗಂಟು_Sym(nodep);  // Until overridden by a SCOPE
        m_cellp = nullptr;
        m_modp = nodep;
        m_modport_ಅಂಕೆ = 0;
        iterateChildren(nodep);
        m_modp = nullptr;
        m_ds.m_dotSymp = m_curSymp = m_modSymp = nullptr;
        m_ds.m_dotPos = DP_NONE;
        m_ds.m_dotErr = ಅಸತ್ಯ;
    }
    ಶೂನ್ಯ visit(AstScope* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(8, indent() << "visit " << nodep);
        checkNoDot(nodep);
        VL_RESTORER(m_modSymp);
        VL_RESTORER(m_curSymp);
        m_ds.m_dotSymp = m_curSymp = m_modSymp = m_statep->getScopeSym(nodep);
        iterateChildren(nodep);
        m_ds.m_dotSymp = m_curSymp = m_modSymp = nullptr;
    }
    ಶೂನ್ಯ visit(AstCellInline* nodep) override {
        LINKDOT_VISIT_START();
        checkNoDot(nodep);
        UASSERT_OBJ(m_statep->forScopeCreation(), nodep, "CellInline should not exist yet");
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlinkFrBack()), nodep);
    }
    ಶೂನ್ಯ visit(AstCell* nodep) override {
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
            ಯದಿ (VN_IS(nodep->modp(), NotFoundModule)) {
                // Prevent warnings about missing pin connects
                ಯದಿ (nodep->pinsp()) nodep->pinsp()->unlinkFrBackWithNext()->delete_ವೃಕ್ಷ();
                ಯದಿ (nodep->paramsp()) nodep->paramsp()->unlinkFrBackWithNext()->delete_ವೃಕ್ಷ();
            }
            // Need to pass the module info to this cell, so we can link up the pin names
            // However can't use m_curSymp as pin connections need to use the
            // instantiator's symbols
            ಅನ್ಯಥಾ {
                m_pinSymp = m_statep->get_ಗಂಟು_Sym(nodep->modp());
                UINFO(4, indent() << "(Backto) visit " << nodep);
                // UINFOTREE(1, nodep, "", "linkcell");
                // UINFOTREE(1, nodep->modp(), "", "linkcemd");
                iterateChildren(nodep);

                ಯದಿ (m_statep->forPrimary())
                    ಯದಿ (ಧ್ರುವ AstModule* ಧ್ರುವ modp = VN_CAST(nodep->modp(), Module)) {
                        ಯದಿ (modp->hasGenericIface())
                            addImplicitParametersOfGenericIface(nodep, modp);
                    }
            }
        }
        // Parent module inherits child's publicity
        // This is done bottom up in the LinkBotupVisitor stage
    }
    ಶೂನ್ಯ visit(AstClassRefDType* nodep) override {
        // Cell: Recurse inside or cleanup not founds
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        // Can be under dot if called as package::class and that class resolves, so no checkNoDot
        VL_RESTORER_CLEAR(m_usedPins);
        VL_RESTORER_CLEAR(m_usedDefParamPins);
        UASSERT_OBJ(nodep->classp(), nodep, "ClassRef has unlinked class");
        UASSERT_OBJ(m_statep->forPrimary() || !nodep->paramsp() || V3Error::error_ಎಣಿಕೆ(), nodep,
                    "class reference parameter not removed by V3Param");
        VL_RESTORER(m_pinSymp);
        {
            // ClassRef's have pins, so track
            m_pinSymp = m_statep->get_ಗಂಟು_Sym(nodep->classp());
            UINFO(4, indent() << "(Backto) visit " << nodep);
            // UINFOTREE(1, nodep, "", "linkcell");
            // UINFOTREE(1, nodep->modp(), "", "linkcemd");
            iterateChildren(nodep);
        }
    }
    ಶೂನ್ಯ visit(AstPin* nodep) override {
        // Pin: Link to submodule's port
        LINKDOT_VISIT_START();
        checkNoDot(nodep);
        iterateChildren(nodep);
        ಯದಿ (!nodep->modVarp()) {
            UASSERT_OBJ(m_pinSymp, nodep, "Pin not under instance?");
            VSymEnt* ಧ್ರುವ foundp = m_pinSymp->findIdFlat(nodep->name());
            ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ whatp = nodep->param() ? "parameter" : "pin";
            ಯದಿ (!foundp) {
                ಯದಿ (m_cellp && VN_IS(m_cellp->modp(), Primitive)
                    && (nodep->name() == "__paramNumber1" || nodep->name() == "__paramNumber2")) {
                    // Primitive parameter is really a delay2 we can just ignore
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlinkFrBack()), nodep);
                    ಪ್ರತ್ಯಾಗಮ;
                } ಅನ್ಯಥಾ ಯದಿ (nodep->param() && !nodep->exprp() && !nodep->svDotName()) {
                    // Placeholder pin the parser makes for an empty '#()', to signal
                    // the user did type the '#()'. No further use, deleting.
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlinkFrBack()), nodep);
                    ಪ್ರತ್ಯಾಗಮ;
                } ಅನ್ಯಥಾ {
                    ಧ್ರುವ std::string suggest
                        = (nodep->param()
                               ? m_statep->suggestSymFlat(m_pinSymp, nodep->name(),
                                                          Link_ಗಂಟು_Matcher_Var_Param{}, ಅಸತ್ಯ)
                               : m_statep->suggestSymFlat(m_pinSymp, nodep->name(),
                                                          Link_ಗಂಟು_Matcher_Var_IO{}, ಅಸತ್ಯ));
                    ಧ್ರುವ std::string decl
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
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlinkFrBack()), nodep);
                    ಪ್ರತ್ಯಾಗಮ;
                }
            }
            VVarType refVarType = VVarType::UNKNOWN;
            ತಾರ್ಕಿಕ wrongPinType = ಅಸತ್ಯ;
            ಯದಿ (AstVar* ಧ್ರುವ varp = VN_CAST(foundp->nodep(), Var)) {
                ಯದಿ (varp->isIO() || varp->isParam() || varp->isIfaceRef()) {
                    refVarType = varp->varType();
                    nodep->modVarp(varp);
                } ಅನ್ಯಥಾ {
                    wrongPinType = ಸತ್ಯ;
                }
            } ಅನ್ಯಥಾ ಯದಿ (AstParamTypeDType* ಧ್ರುವ typep = VN_CAST(foundp->nodep(), ParamTypeDType)) {
                refVarType = typep->varType();
                nodep->modPTypep(typep);
            } ಅನ್ಯಥಾ {
                wrongPinType = ಸತ್ಯ;
            }
            // Don't connect parameter pin to module ports or vice versa
            ಯದಿ (nodep->param() != (refVarType == VVarType::GPARAM)) wrongPinType = ಸತ್ಯ;
            ಯದಿ (wrongPinType) {
                string targetType = LinkDotState::ಗಂಟು_Text_Type(foundp->nodep());
                targetType = VString::aOrAn(targetType) + ' ' + targetType;
                ಯದಿ (nodep->param()) {
                    nodep->v3error("Instance attempts to override "
                                   << nodep->prettyNameQ() << " as a " << whatp << ", but it is "
                                   << targetType);
                } ಅನ್ಯಥಾ {
                    nodep->v3error("Instance attempts to connect to "
                                   << nodep->prettyNameQ() << ", but it is " << targetType);
                }
                ಪ್ರತ್ಯಾಗಮ;
            }
            markAndCheckPinDup(nodep, foundp->nodep(), whatp);
        }
        // Early return() above when deleted
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ isDotThisReference(ಧ್ರುವ AstDot* nodep) {
        ಪ್ರತ್ಯಾಗಮ VN_IS(nodep->lhsp(), ParseRef) && nodep->lhsp()->name() == "this";
    }

    ಶೂನ್ಯ visit(AstDot* nodep) override {
        // Legal under a DOT: AstDot, AstParseRef, AstPackageRef, AstNodeSel, AstPackArrayDType
        //    also a DOT can be part of an expression, but only above plus
        //    AstFTaskRef are legal children
        // Dot(PackageRef, ParseRef(text))
        // Dot(Dot(ClassOrPackageRef,ClassOrPackageRef), ParseRef(text))
        // Dot(Dot(Dot(ParseRef(text), ...
        ಯದಿ (nodep->user3SetOnce()) ಪ್ರತ್ಯಾಗಮ;
        LINKDOT_VISIT_START();
        UINFO(8, indent() << "visit " << nodep);
        UINFO(8, indent() << m_ds.ascii());
        ಧ್ರುವ DotStates lastStates = m_ds;
        ಧ್ರುವ ತಾರ್ಕಿಕ start = (m_ds.m_dotPos == DP_NONE);  // Save, as m_dotp will be changed
        ಧ್ರುವ DotPosition initialDotPos = m_ds.m_dotPos;
        VL_RESTORER(m_randSymp);
        {
            ಯದಿ (start) {  // Starting dot sequence
                UINFOTREE(9, nodep, "", "dot-in");
                // Keep deferral from visit(AstMethodCall) for unspecialized rand target
                m_ds.init(m_curSymp, m_ds.m_unresolvedClass && randTargetUnresolved());
            }
            m_ds.m_dotp = nodep;  // Always, not just at start
            m_ds.m_dotPos = DP_FIRST;

            ಯದಿ (isDotThisReference(nodep)) {
                VSymEnt* classSymp = getThisClassSymp();
                // In 'randomize() with { this.member }', 'this' refers to randomized
                // object, not the calling class (IEEE 1800-2023 18.7)
                ಯದಿ (m_randSymp && m_currentWithp) classSymp = m_randSymp;
                ಯದಿ (!classSymp && randTargetUnresolved()) {
                    // Bind 'this' in post-V3Param pass
                } ಅನ್ಯಥಾ ಯದಿ (!classSymp) {
                    nodep->v3error("'this' used outside class (IEEE 1800-2023 8.11)");
                    m_ds.m_dotErr = ಸತ್ಯ;
                } ಅನ್ಯಥಾ {
                    m_ds.m_dotSymp = classSymp;
                    UINFO(8, indent() << "this. " << m_ds.ascii());
                }
            } ಅನ್ಯಥಾ ಯದಿ (VN_IS(nodep->lhsp(), ParseRef) && nodep->lhsp()->name() == "super") {
                ಧ್ರುವ VSymEnt* classSymp = getThisClassSymp();
                ಯದಿ (!classSymp) {
                    nodep->v3error("'super' used outside class (IEEE 1800-2023 8.15)");
                    m_ds.m_dotErr = ಸತ್ಯ;
                } ಅನ್ಯಥಾ {
                    ಧ್ರುವ auto classp = VN_AS(classSymp->nodep(), Class);
                    ಧ್ರುವ auto cextp = classp->extendsp();
                    ಯದಿ (!cextp) {
                        nodep->v3error("'super' used on non-extended class (IEEE 1800-2023 8.15)");
                        m_ds.m_dotErr = ಸತ್ಯ;
                    } ಅನ್ಯಥಾ {
                        ಯದಿ (m_statep->forPrimary()
                            && m_extendsParam.find(classp) != m_extendsParam.end()) {
                            UINFO(9, indent()
                                         << "deferring until post-V3Param: " << nodep->lhsp());
                            m_ds.m_unresolvedClass = ಸತ್ಯ;
                        } ಅನ್ಯಥಾ {
                            ಧ್ರುವ auto baseClassp = cextp->classp();
                            UASSERT_OBJ(baseClassp, nodep, "Bad superclass");
                            m_ds.m_dotSymp = m_statep->get_ಗಂಟು_Sym(baseClassp);
                            m_ds.m_super = ಸತ್ಯ;
                            UINFO(8, indent() << "super. " << m_ds.ascii());
                        }
                    }
                }
            } ಅನ್ಯಥಾ ಯದಿ (AstClassOrPackageRef* ಧ್ರುವ lhsp
                       = VN_CAST(nodep->lhsp(), ClassOrPackageRef)) {
                // m_ds.m_dotText communicates the cell prefix between stages
                UINFO(8, indent() << "iter.lhs   " << m_ds.ascii() << " " << nodep);
                iterateAndNextNull(lhsp);
                ಯದಿ (!lhsp->classOrPackageSkipp() && lhsp->name() != "local::") {
                    revisitLater(nodep);
                    m_ds = lastStates;
                    // Resolve function args before bailing
                    ಯದಿ (Ast_ಗಂಟು_FTask_Ref* ಧ್ರುವ ftaskrefp = VN_CAST(nodep->rhsp(), ಗಂಟು_FTask_Ref)) {
                        iterateAndNextNull(ftaskrefp->argsp());
                    }
                    ಪ್ರತ್ಯಾಗಮ;
                }
                m_ds.m_dotPos = DP_PACKAGE;
                // nodep->lhsp() may be a new node
                ಯದಿ (AstClassOrPackageRef* ಧ್ರುವ classOrPackageRefp
                    = VN_CAST(nodep->lhsp(), ClassOrPackageRef)) {
                    ಯದಿ (Ast_ಗಂಟು* ಧ್ರುವ classOrPackageNodep
                        = classOrPackageRefp->classOrPackageSkipp()) {
                        m_ds.m_dotSymp = m_statep->get_ಗಂಟು_Sym(classOrPackageNodep);
                    }
                }
                UINFO(8, indent() << "iter.ldone " << m_ds.ascii() << " " << nodep);
            } ಅನ್ಯಥಾ ಯದಿ (VN_IS(nodep->lhsp(), Dot) && VN_AS(nodep->lhsp(), Dot)->colon()) {
                // m_ds.m_dotText communicates the cell prefix between stages
                UINFO(8, indent() << "iter.lhs   " << m_ds.ascii() << " " << nodep);
                m_ds.m_dotPos = DP_PACKAGE;
                iterateAndNextNull(nodep->lhsp());
                // nodep->lhsp() may be a new node
                ಯದಿ (AstClassOrPackageRef* ಧ್ರುವ crefp
                    = VN_CAST(nodep->lhsp(), ClassOrPackageRef)) {
                    ಯದಿ (!crefp->classOrPackageSkipp()) {
                        revisitLater(nodep);
                        m_ds = lastStates;
                        // Resolve function args before bailing
                        ಯದಿ (Ast_ಗಂಟು_FTask_Ref* ಧ್ರುವ ftaskrefp
                            = VN_CAST(nodep->rhsp(), ಗಂಟು_FTask_Ref)) {
                            iterateAndNextNull(ftaskrefp->argsp());
                        }
                        ಪ್ರತ್ಯಾಗಮ;
                    }
                }
                ಯದಿ (m_lastDeferredp == nodep->lhsp()) {
                    // LHS got deferred, so this node won't be resolved. Defer it too
                    m_ds = lastStates;
                    // Resolve function args before bailing
                    ಯದಿ (Ast_ಗಂಟು_FTask_Ref* ಧ್ರುವ ftaskrefp = VN_CAST(nodep->rhsp(), ಗಂಟು_FTask_Ref)) {
                        iterateAndNextNull(ftaskrefp->argsp());
                    }
                    ಪ್ರತ್ಯಾಗಮ;
                }
                UINFO(8, indent() << "iter.ldone " << m_ds.ascii() << " " << nodep);
            } ಅನ್ಯಥಾ {
                m_ds.m_dotPos = DP_FIRST;
                m_ds.m_unresolvedGenericIface = ಅಸತ್ಯ;
                UINFO(8, indent() << "iter.lhs   " << m_ds.ascii() << " " << nodep);
                iterateAndNextNull(nodep->lhsp());
                UINFO(8, indent() << "iter.ldone " << m_ds.ascii() << " " << nodep);
                ಯದಿ (m_ds.m_unresolvedGenericIface) {
                    m_ds.m_dotPos = initialDotPos;
                    ಪ್ರತ್ಯಾಗಮ;
                }
                // UINFOTREE(9, nodep, "", "dot-lho");
            }
            ಯದಿ (m_statep->forPrimary() && isParamedClassRef(nodep->lhsp())) {
                // Dots of paramed classes will be linked after deparameterization
                UINFO(9, indent() << "deferring until post-V3Param: " << nodep->lhsp());
                m_ds.m_unresolvedClass = ಸತ್ಯ;
            }
            ಯದಿ (m_ds.m_unresolvedCell
                && (VN_IS(nodep->lhsp(), CellRef) || VN_IS(nodep->lhsp(), CellArrayRef))) {
                m_ds.m_unlinkedScopep = nodep->lhsp();
            }
            ಯದಿ (m_ds.m_dotPos == DP_FIRST) m_ds.m_dotPos = DP_SCOPE;
            ಯದಿ (!m_ds.m_dotErr) {  // Once something wrong, give up
                // Top 'final' dot RHS is final RHS, else it's a
                // DOT(DOT(x,*here*),real-rhs) which we consider a RHS
                ಯದಿ (start && m_ds.m_dotPos == DP_SCOPE) m_ds.m_dotPos = DP_FINAL;
                UINFO(8, indent() << "iter.rhs   " << m_ds.ascii() << " " << nodep);
                // m_ds.m_dotSymp points at lhsp()'s symbol table, so resolve RHS under that
                iterateAndNextNull(nodep->rhsp());
                UINFO(8, indent() << "iter.rdone " << m_ds.ascii() << " " << nodep);
                // UINFOTREE(9, nodep, "", "dot-rho");
            }
            ಯದಿ (!m_ds.m_unresolvedClass) {
                ಯದಿ (start) {
                    Ast_ಗಂಟು* newp;
                    ಯದಿ (m_ds.m_dotErr) {
                        newp = ನೂತನ AstConst{nodep->fileline(), AstConst::BitFalse{}};
                    } ಅನ್ಯಥಾ {
                        newp = nodep->rhsp()->unlinkFrBack();
                    }
                    UINFOTREE(9, newp, "", "dot-out");
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                } ಅನ್ಯಥಾ {  // Dot midpoint
                    Ast_ಗಂಟು* newp = nodep->rhsp()->unlinkFrBack();
                    ಯದಿ (m_ds.m_unresolvedCell) {
                        AstCellRef* ಧ್ರುವ crp = ನೂತನ AstCellRef{
                            nodep->fileline(), nodep->name(), nodep->lhsp()->unlinkFrBack(), newp};
                        newp = crp;
                    }
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                }
            }
        }
        ಯದಿ (start) {
            m_ds = lastStates;
        } ಅನ್ಯಥಾ {
            ಧ್ರುವ ತಾರ್ಕಿಕ unresolvedClass = m_ds.m_unresolvedClass;
            m_ds.m_dotp = lastStates.m_dotp;
            m_ds.m_unresolvedClass |= unresolvedClass;
        }
        UINFO(8, indent() << "done " << m_ds.ascii() << " " << nodep);
    }
    ಶೂನ್ಯ visit(AstDefaultClocking* nodep) override {
        ಯದಿ (VSymEnt* ಧ್ರುವ foundp = m_curSymp->findIdFallback(nodep->name())) {
            ಯದಿ (AstClocking* ಧ್ರುವ clockingp = VN_CAST(foundp->nodep(), Clocking)) {
                clockingp->makeDefault();
                VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ವೃಕ್ಷ(), nodep);
            } ಅನ್ಯಥಾ {
                nodep->v3error(nodep->prettyNameQ() << " is not a clocking identifier");
            }
        } ಅನ್ಯಥಾ {
            nodep->v3error("Can't find definition of clocking: " << nodep->prettyNameQ());
        }
    }
    ಶೂನ್ಯ visit(AstSenItem* nodep) override {
        LINKDOT_VISIT_START();
        VL_RESTORER(m_inSens);
        m_inSens = ಸತ್ಯ;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstPatMember* nodep) override {
        LINKDOT_VISIT_START();
        ಯದಿ (nodep->varrefp()) ಪ್ರತ್ಯಾಗಮ;  // only do this mapping once
        // If we have a TEXT token as our key, lookup if it's a LPARAM
        ಯದಿ (AstText* ಧ್ರುವ textp = VN_CAST(nodep->keyp(), Text)) {
            UINFO(9, indent() << "visit " << nodep);
            UINFO(9, indent() << "      " << textp);
            // Lookup
            ಯದಿ (VSymEnt* ಧ್ರುವ foundp = m_curSymp->findIdFallback(textp->text())) {
                ಯದಿ (AstVar* ಧ್ರುವ varp = VN_CAST(foundp->nodep(), Var)) {
                    ಯದಿ (varp->isParam() || varp->isGenVar()) {
                        // Attach found Text reference to PatMember
                        nodep->varrefp(
                            ನೂತನ AstVarRef{nodep->fileline(),
                                          foundp->imported() ? foundp->classOrPackagep() : nullptr,
                                          varp, VAccess::READ});
                        UINFO(9, indent() << " new " << nodep->varrefp());
                    }
                }
                ಯದಿ (AstEnumItem* ಧ್ರುವ itemp = VN_CAST(foundp->nodep(), EnumItem)) {
                    // Attach enum item value to PatMember
                    nodep->varrefp(
                        ನೂತನ AstEnumItemRef{nodep->fileline(), itemp, foundp->classOrPackagep()});
                    UINFO(9, indent() << " new " << itemp);
                }
            }
        }
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstParseRef* nodep) override {
        ಯದಿ (nodep->user3SetOnce()) ಪ್ರತ್ಯಾಗಮ;
        LINKDOT_VISIT_START();
        UINFO(9, indent() << "visit " << nodep);
        UINFO(9, indent() << m_ds.ascii());
        ಯದಿ (m_ds.m_unresolvedClass) ಪ್ರತ್ಯಾಗಮ;
        // m_curSymp is symbol table of outer expression
        // m_ds.m_dotSymp is symbol table relative to "."'s above now
        UASSERT_OBJ(m_ds.m_dotSymp, nodep, "nullptr lookup symbol table");
        // Generally resolved during Primary, but might be at param time under AstUnlinkedRef
        UASSERT_OBJ(!m_statep->forScopeCreation(), nodep, "ParseRefs should no longer exist");
        ಧ್ರುವ DotStates lastStates = m_ds;
        ಧ್ರುವ ತಾರ್ಕಿಕ start = (m_ds.m_dotPos == DP_NONE);  // Save, as m_dotp will be changed
        ತಾರ್ಕಿಕ first = start || m_ds.m_dotPos == DP_FIRST;

        ಯದಿ (start) {
            m_ds.init(m_curSymp);
            // Note m_ds.m_dotp remains nullptr; this is a reference not under a dot
        }
        ಯದಿ (nodep->name() == "super") {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: super");
            m_ds.m_dotErr = ಸತ್ಯ;
        }
        ಯದಿ (nodep->name() == "this") {
            iterateChildren(nodep);
            ಯದಿ (m_statep->forPrimary()) ಪ್ರತ್ಯಾಗಮ;  // The class might be parameterized somewhere
            ಧ್ರುವ VSymEnt* classSymp = getThisClassSymp();
            ಯದಿ (!classSymp) {
                nodep->v3error("'this' used outside class (IEEE 1800-2023 8.11)");
                ಪ್ರತ್ಯಾಗಮ;
            }
            AstClass* ಧ್ರುವ classp = VN_AS(classSymp->nodep(), Class);
            AstClassRefDType* ಧ್ರುವ dtypep
                = ನೂತನ AstClassRefDType{nodep->fileline(), classp, nullptr};
            AstThisRef* ಧ್ರುವ newp = ನೂತನ AstThisRef{nodep->fileline(), VFlagChildDType{}, dtypep};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }
        ಯದಿ (m_ds.m_dotPos == DP_MEMBER && VN_IS(m_ds.m_dotp->lhsp(), LambdaArgRef)
            && nodep->name() == "index") {
            // 'with' statement's 'item.index'
            // m_ds.dotp->lhsp() was checked to know if `index` is directly after lambda arg ref.
            // If not, treat it as normal member select
            iterateChildren(nodep);
            AstLambdaArgRef* ಧ್ರುವ newp = ನೂತನ AstLambdaArgRef{
                nodep->fileline(), m_ds.m_dotp->lhsp()->name() + "__DOT__index", ಸತ್ಯ};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            ಪ್ರತ್ಯಾಗಮ;
        } ಅನ್ಯಥಾ ಯದಿ (m_ds.m_dotPos == DP_MEMBER) {
            // Found a Var, everything following is membership.  {scope}.{var}.HERE {member}
            ಯದಿ (m_ds.m_dotErr) {
                nodep->unlinkFrBack();  // Avoid circular node loop on errors
            } ಅನ್ಯಥಾ {
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ varEtcp = VN_AS(m_ds.m_dotp->lhsp()->unlinkFrBack(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ);
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ newp = ನೂತನ AstMemberSel{nodep->fileline(), varEtcp,  //
                                                           VFlagChildDType{}, nodep->name()};
                nodep->replaceWith(newp);
            }
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        } ಅನ್ಯಥಾ {
            //
            string expectWhat;
            ತಾರ್ಕಿಕ allowScope = ಅಸತ್ಯ;
            ತಾರ್ಕಿಕ allowVar = ಅಸತ್ಯ;
            ತಾರ್ಕಿಕ allowFTask = ಅಸತ್ಯ;
            ತಾರ್ಕಿಕ staticAccess = ಅಸತ್ಯ;
            ಯದಿ (m_ds.m_disablep) {
                allowScope = ಸತ್ಯ;
                allowFTask = ಸತ್ಯ;
                expectWhat = "block/task";
            } ಅನ್ಯಥಾ ಯದಿ (m_ds.m_dotPos == DP_PACKAGE) {
                // {package-or-class}::{a}
                Ast_ಗಂಟು_Module* classOrPackagep = nullptr;
                expectWhat = "scope/variable/func";
                allowScope = ಸತ್ಯ;
                allowVar = ಸತ್ಯ;
                allowFTask = ಸತ್ಯ;
                staticAccess = ಸತ್ಯ;
                UINFO(9, "chk pkg " << m_ds.ascii() << " lhsp=" << m_ds.m_dotp->lhsp());
                UASSERT_OBJ(VN_IS(m_ds.m_dotp->lhsp(), ClassOrPackageRef), m_ds.m_dotp->lhsp(),
                            "Bad package link");
                AstClassOrPackageRef* ಧ್ರುವ cpackagerefp
                    = VN_AS(m_ds.m_dotp->lhsp(), ClassOrPackageRef);
                ಯದಿ (cpackagerefp->name() == "local::") {
                    m_randSymp = nullptr;
                    first = ಸತ್ಯ;
                } ಅನ್ಯಥಾ ಯದಿ (!cpackagerefp->classOrPackageSkipp()) {
                    VSymEnt* ಧ್ರುವ foundp = m_statep->resolveClassOrPackage(
                        m_ds.m_dotSymp, cpackagerefp, ಸತ್ಯ, ಅಸತ್ಯ, ":: reference");
                    ಯದಿ (!foundp) ಪ್ರತ್ಯಾಗಮ;
                    classOrPackagep = cpackagerefp->classOrPackageSkipp();
                    ಯದಿ (classOrPackagep) {
                        m_ds.m_dotSymp = m_statep->get_ಗಂಟು_Sym(classOrPackagep);
                    } ಅನ್ಯಥಾ {
                        m_ds = lastStates;
                        ಪ್ರತ್ಯಾಗಮ;
                    }
                } ಅನ್ಯಥಾ {
                    classOrPackagep = cpackagerefp->classOrPackageSkipp();
                    UASSERT_OBJ(classOrPackagep, m_ds.m_dotp->lhsp(), "Bad package link");
                    m_ds.m_dotSymp = m_statep->get_ಗಂಟು_Sym(classOrPackagep);
                }
                m_ds.m_dotPos = DP_SCOPE;
            } ಅನ್ಯಥಾ ಯದಿ (m_ds.m_dotPos == DP_SCOPE || m_ds.m_dotPos == DP_FIRST) {
                // {a}.{b}, where {a} maybe a module name
                // or variable, where dotting into structure member
                expectWhat = "scope/variable";
                allowScope = ಸತ್ಯ;
                allowVar = ಸತ್ಯ;
            } ಅನ್ಯಥಾ ಯದಿ (m_ds.m_dotPos == DP_NONE) {
                expectWhat = "variable";
                allowVar = ಸತ್ಯ;
            } ಅನ್ಯಥಾ ಯದಿ (m_ds.m_dotPos == DP_FINAL) {
                expectWhat = "variable/method";
                allowFTask = ಸತ್ಯ;
                allowVar = ಸತ್ಯ;
            } ಅನ್ಯಥಾ {
                UINFO(1, "ds=" << m_ds.ascii());
                nodep->v3fatalSrc("Unhandled DotPosition");
            }
            // Lookup
            VSymEnt* foundp;
            string baddot;
            VSymEnt* okSymp = nullptr;
            // Restricted 'with (id_list)': only id_list names bind into target class.
            ಯದಿ (m_randSymp
                && (!m_currentWithp || m_currentWithp->nameResolvesToTarget(nodep->name()))) {
                foundp = m_randSymp->findIdFlat(nodep->name());
                ಯದಿ (foundp) {
                    ಯದಿ (m_currentWithp && m_currentWithp->restricted()) {
                        m_restrictedNamesUsed.ಸೇರಿಸು(nodep->name());
                    }
                    ಯದಿ (!start) m_ds.m_dotPos = DP_MEMBER;
                    ಯದಿ (!m_currentWithp) {
                        UASSERT_OBJ(m_randMethodCallp, nodep, "Expected to be under randomize()");
                        // This will start failing once complex expressions are allowed on the LHS
                        // of randomize() with args
                        UASSERT_OBJ(VN_IS(m_randMethodCallp->fromp(), VarRef), m_randMethodCallp,
                                    "Expected simple randomize target");
                        // A ParseRef is used here so that the dot RHS gets resolved
                        AstMemberSel* ಧ್ರುವ newp = ನೂತನ AstMemberSel{
                            nodep->fileline(), m_randMethodCallp->fromp()->clone_ವೃಕ್ಷ(ಅಸತ್ಯ),
                            VFlagChildDType{}, nodep->name()};
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                        ಪ್ರತ್ಯಾಗಮ;
                    }
                    UINFO(9, indent() << "randomize-with fromSym " << foundp->nodep());
                    AstLambdaArgRef* ಧ್ರುವ lambdaRefp
                        = ನೂತನ AstLambdaArgRef{nodep->fileline(), "__Vrandwith_obj", ಅಸತ್ಯ};
                    AstMemberSel* newp = ನೂತನ AstMemberSel{nodep->fileline(), lambdaRefp,
                                                          VFlagChildDType{}, nodep->name()};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                    ಪ್ರತ್ಯಾಗಮ;
                }
            }
            ಯದಿ (allowScope) {
                foundp = m_statep->findDotted(nodep->fileline(), m_ds.m_dotSymp, nodep->name(),
                                              baddot, okSymp, first);  // Maybe nullptr
            } ಅನ್ಯಥಾ ಯದಿ (first) {
                foundp = m_ds.m_dotSymp->findIdFallback(nodep->name());
            } ಅನ್ಯಥಾ {
                foundp = m_ds.m_dotSymp->findIdFlat(nodep->name());
            }
            ಯದಿ (!foundp && m_ds.m_dotp && isDotThisReference(m_ds.m_dotp)) {
                ಧ್ರುವ AstClass* ಧ್ರುವ classp = VN_CAST(m_ds.m_dotSymp->nodep(), Class);
                ಯದಿ (classp && classp->isCovergroup() && classp->covergroupEnclosingClassp()) {
                    VSymEnt* ಧ್ರುವ enclosingClassSymp
                        = m_statep->get_ಗಂಟು_Sym(classp->covergroupEnclosingClassp());
                    foundp = enclosingClassSymp->findIdFallback(nodep->name());
                    ಯದಿ (foundp) m_ds.m_dotSymp = enclosingClassSymp;
                }
            }
            // If not found in modport, check interface fallback for parameters and typedefs.
            // Parameters and typedefs are always visible through a modport (IEEE 1800-2023 25.5).
            // This mirrors the VarXRef modport parameter fallback in visit(AstVarXRef).
            ಯದಿ (!foundp && VN_IS(m_ds.m_dotSymp->nodep(), Modport)
                && m_ds.m_dotSymp->fallbackp()) {
                VSymEnt* ಧ್ರುವ ifaceFallbackp = m_ds.m_dotSymp->fallbackp();
                VSymEnt* ಧ್ರುವ ifaceFoundp = ifaceFallbackp->findIdFlat(nodep->name());
                ಯದಿ (ifaceFoundp) {
                    ಯದಿ (ಧ್ರುವ AstVar* ಧ್ರುವ varp = VN_CAST(ifaceFoundp->nodep(), Var)) {
                        ಯದಿ (varp->isParam()) foundp = ifaceFoundp;
                    } ಅನ್ಯಥಾ ಯದಿ (VN_IS(ifaceFoundp->nodep(), Typedef)
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
            ಯದಿ (!foundp && !okSymp && VN_IS(m_ds.m_dotSymp->nodep(), Modport)) {
                okSymp = m_ds.m_dotSymp;
            }
            ಯದಿ (foundp) {
                UINFO(9, indent() << "found=se" << cvtToHex(foundp) << "  exp=" << expectWhat
                                  << "  n=" << foundp->nodep());
            }
            // What fell out?
            ತಾರ್ಕಿಕ ok = ಅಸತ್ಯ;
            // Special case: waiting on clocking event
            ಯದಿ (m_inSens && foundp && m_ds.m_dotPos != DP_SCOPE && m_ds.m_dotPos != DP_FIRST) {
                ಯದಿ (AstClocking* ಧ್ರುವ clockingp = sensClockingp(foundp->nodep())) {
                    foundp = getCreateClockingEventSymEnt(clockingp);
                }
            }
            ಯದಿ (!foundp) {
            } ಅನ್ಯಥಾ ಯದಿ (VN_IS(foundp->nodep(), Cell) || VN_IS(foundp->nodep(), ಗಂಟು_Block)
                       || VN_IS(foundp->nodep(), GenBlock)
                       || VN_IS(foundp->nodep(), Netlist)  // for $root
                       || VN_IS(foundp->nodep(), Module)) {  // if top
                ಯದಿ (allowScope) {
                    ok = ಸತ್ಯ;
                    m_ds.m_dotText = VString::dot(m_ds.m_dotText, ".", nodep->name());
                    m_ds.m_dotSymp = foundp;
                    m_ds.m_dotPos = DP_SCOPE;
                    ಯದಿ (m_ds.m_disablep && VN_IS(foundp->nodep(), ಗಂಟು_Block)) {
                        // Possibly it is not the final link. If we are under dot and not in its
                        // last component, `targetp()` field will be overwritten by next components
                        m_ds.m_disablep->targetp(foundp->nodep());
                    }
                    ಯದಿ (VN_IS(foundp->nodep(), GenBlock)) {
                        m_ds.m_genBlk = ಸತ್ಯ;
                        ಯದಿ (m_ds.m_disablep) {
                            m_ds.m_disablep->v3warn(
                                E_UNSUPPORTED,
                                "Unsupported: Generate block referenced by disable");
                        }
                    }
                    // Upper AstDot visitor will handle it from here
                } ಅನ್ಯಥಾ ಯದಿ (VN_IS(foundp->nodep(), Cell) && allowVar) {
                    AstCell* ಧ್ರುವ cellp = VN_AS(foundp->nodep(), Cell);
                    ಯದಿ (VN_IS(cellp->modp(), Iface)) {
                        // Interfaces can be referenced like a variable for interconnect
                        VSymEnt* ಧ್ರುವ cellEntp = m_statep->get_ಗಂಟು_Sym(cellp);
                        UASSERT_OBJ(cellEntp, nodep, "No interface sym entry");
                        VSymEnt* ಧ್ರುವ parentEntp
                            = cellEntp->parentp();  // Container of the var; probably a module or
                                                    // generate begin
                        AstVar* ಧ್ರುವ ifaceRefVarp
                            = find_Iface_ತುದಿ_Varp(nodep, parentEntp, nodep->name());
                        //
                        ok = ಸತ್ಯ;
                        m_ds.m_dotSymp = foundp;
                        m_ds.m_dotPos = DP_SCOPE;
                        UINFO(9, indent() << " cell -> iface varref " << foundp->nodep());
                        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newp;
                        ಯದಿ (m_ds.m_dotText != "") {
                            newp = ನೂತನ AstVarXRef{nodep->fileline(), ifaceRefVarp, m_ds.m_dotText,
                                                  VAccess::READ};
                            ಯದಿ (m_ds.m_unresolvedCell && m_ds.m_unlinkedScopep) {
                                newp = ನೂತನ AstUnlinkedRef{nodep->fileline(), newp, newp->name(),
                                                          m_ds.m_unlinkedScopep->unlinkFrBack()};
                                m_ds.m_unlinkedScopep = nullptr;
                                m_ds.m_unresolvedCell = ಅಸತ್ಯ;
                                m_ds.m_dotText = "";
                            }
                        } ಅನ್ಯಥಾ {
                            AstVarRef* ಧ್ರುವ refp
                                = ನೂತನ AstVarRef{nodep->fileline(), ifaceRefVarp, VAccess::READ};
                            newp = refp;
                        }
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                    } ಅನ್ಯಥಾ ಯದಿ (VN_IS(cellp->modp(), NotFoundModule)) {
                        cellp->modNameFileline()->v3error("Cannot find file containing interface: "
                                                          << cellp->modp()->prettyNameQ());
                    }
                }
            } ಅನ್ಯಥಾ ಯದಿ (allowScope && !allowFTask && VN_IS(foundp->nodep(), ಗಂಟು_FTask)) {
                // Function/task used as scope for static variable access (IEEE std 1800-2017 6.21)
                // Don't set m_dotText so the static variable resolves as a VarRef rather than
                // VarXRef. This is so that V3Begin won't move it out of the function/task's scope.
                ok = ಸತ್ಯ;
                m_ds.m_dotSymp = foundp;
                m_ds.m_dotPos = DP_SCOPE;
            } ಅನ್ಯಥಾ ಯದಿ (allowFTask && VN_IS(foundp->nodep(), ಗಂಟು_FTask)) {
                Ast_ಗಂಟು_FTask_Ref* taskrefp;
                ಯದಿ (VN_IS(foundp->nodep(), Task)) {
                    taskrefp = ನೂತನ AstTaskRef{nodep->fileline(), nodep->name()};
                } ಅನ್ಯಥಾ {
                    taskrefp = ನೂತನ AstFuncRef{nodep->fileline(), nodep->name()};
                }
                nodep->replaceWith(taskrefp);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                m_ds = lastStates;
                ಪ್ರತ್ಯಾಗಮ;
            } ಅನ್ಯಥಾ ಯದಿ (AstVar* ಧ್ರುವ varp = foundToVarp(foundp, nodep, VAccess::READ)) {
                // Preserve explicit 'this' references to members inherited from
                // generic classes until after specialization.
                ಯದಿ (m_statep->forPrimary() && m_ds.m_dotp && isDotThisReference(m_ds.m_dotp)
                    && foundp->classOrPackagep() != m_modp
                    && VN_IS(foundp->classOrPackagep(), Class)
                    && foundp->classOrPackagep()->hasGParam()) {
                    m_ds.m_unresolvedClass = ಸತ್ಯ;
                    ಪ್ರತ್ಯಾಗಮ;
                }
                AstIfaceRefDType* ಧ್ರುವ ifacerefp
                    = LinkDotState::ifaceRefFromArray(varp->subDTypep());
                ಯದಿ (varp->isIfaceRef() && m_genericIfaceModule
                    && VN_IS(varp->childDTypep(), IfaceGenericDType) && !start) {
                    // Defer only dotted member access ('d.PARAM'), as V3Param must specialize
                    // first; a standalone ref ('.x(d)') resolves via allowVar below now
                    ok = ಸತ್ಯ;
                    m_ds.m_unresolvedGenericIface = ಸತ್ಯ;
                } ಅನ್ಯಥಾ ಯದಿ (ifacerefp && varp->isIfaceRef()) {
                    UASSERT_OBJ(ifacerefp->ifaceViaCellp(), ifacerefp, "Unlinked interface");
                    // Really this is a scope reference into an interface
                    UINFO(9, indent() << "varref-ifaceref " << m_ds.m_dotText << "  " << nodep);
                    m_ds.m_dotText = VString::dot(m_ds.m_dotText, ".", nodep->name());
                    // If modport specified, use modport symbol table for lookup of virtual ports
                    ಯದಿ (ifacerefp->modportp() && m_statep->exists_ಗಂಟು_Sym(ifacerefp->modportp())) {
                        m_ds.m_dotSymp = m_statep->get_ಗಂಟು_Sym(ifacerefp->modportp());
                    } ಅನ್ಯಥಾ {
                        m_ds.m_dotSymp = m_statep->get_ಗಂಟು_Sym(ifacerefp->ifaceViaCellp());
                    }
                    m_ds.m_dotPos = DP_SCOPE;
                    ok = ಸತ್ಯ;

                    AstVarRef* ಧ್ರುವ refp = ನೂತನ AstVarRef{nodep->fileline(), varp, VAccess::READ};
                    nodep->replaceWith(refp);

                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                } ಅನ್ಯಥಾ ಯದಿ (allowVar) {
                    Ast_ಗಂಟು* newp;
                    ಯದಿ (m_ds.m_dotText != "") {
                        AstVarXRef* ಧ್ರುವ refp
                            = ನೂತನ AstVarXRef{nodep->fileline(), nodep->name(), m_ds.m_dotText,
                                             VAccess::READ};  // lvalue'ness computed later
                        refp->varp(varp);
                        // Not linked again after V3LinkLValue, so that reports writing it
                        ಯದಿ (ಧ್ರುವ AstModportVarRef* ಧ್ರುವ mvarp
                            = VN_CAST(foundp->nodep(), ModportVarRef)) {
                            ಯದಿ (m_statep->forParamed() && mvarp->direction().isReadOnly()) {
                                refp->readOnlyModport(ಸತ್ಯ);
                            }
                        }
                        refp->containsGenBlock(m_ds.m_genBlk);
                        m_ds.m_dotText = "";
                        ಯದಿ (m_ds.m_unresolvedCell && m_ds.m_unlinkedScopep) {
                            ಧ್ರುವ string dotted = refp->dotted();
                            ಧ್ರುವ size_t pos = dotted.find("__BRA__??__KET__");
                            // Arrays of interfaces all have the same parameters
                            ಯದಿ (pos != string::npos && varp->isParam()
                                && VN_IS(m_ds.m_unlinkedScopep, CellArrayRef)) {
                                refp->dotted(dotted.substr(0, pos));
                                newp = refp;
                            } ಅನ್ಯಥಾ {
                                UINFO(9, indent() << "deferring until post-V3Param: " << refp);
                                newp = ನೂತನ AstUnlinkedRef{nodep->fileline(), refp, refp->name(),
                                                          m_ds.m_unlinkedScopep->unlinkFrBack()};
                                m_ds.m_unlinkedScopep = nullptr;
                                m_ds.m_unresolvedCell = ಅಸತ್ಯ;
                            }
                        } ಅನ್ಯಥಾ {
                            newp = refp;
                        }
                    } ಅನ್ಯಥಾ {
                        ಯದಿ (staticAccess && !varp->lifetime().isStatic() && !varp->isParam()) {
                            // TODO bug4077
                            // nodep->v3error("Static access to non-static member variable "
                            //                << varp->prettyNameQ() << endl);
                        }
                        AstVarRef* ಧ್ರುವ refp = ನೂತನ AstVarRef{
                            nodep->fileline(), varp, VAccess::READ};  // lvalue'ness computed later
                        refp->classOrPackagep(foundp->classOrPackagep());
                        newp = refp;
                    }
                    UINFO(9, indent() << "new " << newp);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                    m_ds.m_dotPos = DP_MEMBER;
                    ok = ಸತ್ಯ;
                }
            } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstModport* ಧ್ರುವ modportp = VN_CAST(foundp->nodep(), Modport)) {
                // A scope reference into an interface's modport (not
                // necessarily at a pin connection)
                UINFO(9, indent() << "cell-ref-to-modport " << m_ds.m_dotText << "  " << nodep);
                UINFO(9, indent() << "unlinked " << m_ds.m_unlinkedScopep);
                UINFO(9,
                      indent() << "dotSymp " << m_ds.m_dotSymp << " " << m_ds.m_dotSymp->nodep());
                // Iface was the previously dotted component
                ಯದಿ (!m_ds.m_dotSymp || !VN_IS(m_ds.m_dotSymp->nodep(), Cell)
                    || !VN_AS(m_ds.m_dotSymp->nodep(), Cell)->modp()
                    || !VN_IS(VN_AS(m_ds.m_dotSymp->nodep(), Cell)->modp(), Iface)) {
                    nodep->v3error("Modport not referenced as <interface>."
                                   << modportp->prettyNameQ());
                } ಅನ್ಯಥಾ ಯದಿ (!VN_AS(m_ds.m_dotSymp->nodep(), Cell)->modp()
                           || !VN_IS(VN_AS(m_ds.m_dotSymp->nodep(), Cell)->modp(), Iface)) {
                    nodep->v3error("Modport not referenced from underneath an interface: "
                                   << modportp->prettyNameQ());
                } ಅನ್ಯಥಾ {
                    AstCell* ಧ್ರುವ cellp = VN_AS(m_ds.m_dotSymp->nodep(), Cell);
                    UASSERT_OBJ(cellp, nodep, "Modport not referenced from an instance");
                    VSymEnt* ಧ್ರುವ cellEntp = m_statep->get_ಗಂಟು_Sym(cellp);
                    UASSERT_OBJ(cellEntp, nodep, "No interface sym entry");
                    VSymEnt* ಧ್ರುವ parentEntp
                        = cellEntp->parentp();  // Container of the var; probably a
                                                // module or generate begin
                    // We drop __BRA__??__KET__ as cells don't have that naming yet
                    AstVar* ಧ್ರುವ ifaceRefVarp
                        = find_Iface_ತುದಿ_Varp(nodep, parentEntp, cellp->name());
                    //
                    ok = ಸತ್ಯ;
                    m_ds.m_dotText = VString::dot(m_ds.m_dotText, ".", nodep->name());
                    m_ds.m_dotSymp = foundp;
                    m_ds.m_dotPos = DP_SCOPE;
                    UINFO(9, indent() << "modport -> iface varref " << foundp->nodep());
                    // We lose the modport name here, so we cannot detect mismatched modports.
                    AstVarRef* ಧ್ರುವ refp
                        = ನೂತನ AstVarRef{nodep->fileline(), ifaceRefVarp, VAccess::READ};
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newp = refp;

                    auto* ಧ್ರುವ cellarrayrefp = VN_CAST(m_ds.m_unlinkedScopep, CellArrayRef);
                    ಯದಿ (cellarrayrefp) {
                        // iface[vec].modport became CellArrayRef(iface, lsb)
                        // Convert back to SelBit(iface, lsb)
                        UINFO(9, indent() << "Array modport to SelBit " << cellarrayrefp);
                        newp = ನೂತನ AstSelBit{cellarrayrefp->fileline(), newp,
                                             cellarrayrefp->selp()->unlinkFrBack()};
                        newp->user3(ಸತ್ಯ);  // Don't process again
                        VL_DO_DANGLING(ತಳ್ಳು_Deletep(cellarrayrefp->unlinkFrBack()), cellarrayrefp);
                        m_ds.m_unlinkedScopep = nullptr;
                    }
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                }
            } ಅನ್ಯಥಾ ಯದಿ (AstConstraint* ಧ್ರುವ defp = VN_CAST(foundp->nodep(), Constraint)) {
                Ast_ಗಂಟು* ಧ್ರುವ newp = ನೂತನ AstConstraintRef{nodep->fileline(), nullptr, defp};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                ok = ಸತ್ಯ;
                m_ds.m_dotPos = DP_MEMBER;
                m_ds.m_dotText = "";
            } ಅನ್ಯಥಾ ಯದಿ (AstClass* ಧ್ರುವ defp = VN_CAST(foundp->nodep(), Class)) {
                ಯದಿ (allowVar) {
                    ಯದಿ (m_ds.m_dotPos == DP_SCOPE && !staticAccess) {
                        // Continue hierarchical lookup in the class scope.
                        m_ds.m_dotSymp = foundp;
                    } ಅನ್ಯಥಾ {
                        AstRefDType* ಧ್ರುವ newp
                            = ನೂತನ AstRefDType{nodep->fileline(), nodep->name()};
                        replaceWithCheckBreak(nodep, newp);
                        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                    }
                    ok = ಸತ್ಯ;
                    m_ds.m_dotText = "";
                } ಅನ್ಯಥಾ {
                    (ಶೂನ್ಯ)defp;  // Prevent unused variable warning
                }
            } ಅನ್ಯಥಾ ಯದಿ (AstMemberDType* ಧ್ರುವ defp = VN_CAST(foundp->nodep(), MemberDType)) {
                ಯದಿ (allowVar) {
                    checkMemberDeclOrder(nodep, defp);
                    AstRefDType* ಧ್ರುವ refp = ನೂತನ AstRefDType{nodep->fileline(), nodep->name()};
                    refp->refDTypep(defp);
                    replaceWithCheckBreak(nodep, refp);
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                    ok = ಸತ್ಯ;
                    m_ds.m_dotText = "";
                }
            } ಅನ್ಯಥಾ ಯದಿ (AstEnumItem* ಧ್ರುವ valuep = VN_CAST(foundp->nodep(), EnumItem)) {
                ಯದಿ (allowVar) {
                    AstEnumItemRef* ಧ್ರುವ refp
                        = ನೂತನ AstEnumItemRef{nodep->fileline(), valuep, foundp->classOrPackagep()};
                    Ast_ಗಂಟು* newp = refp;
                    // Hierarchical reference, relinked after V3Param as the referenced
                    // module may be specialized, similar to AstVarXRef
                    refp->dotted(m_ds.m_dotText);
                    refp->containsGenBlock(m_ds.m_genBlk);
                    ಯದಿ (m_ds.m_unresolvedCell && m_ds.m_unlinkedScopep) {
                        UINFO(9, indent() << "deferring until post-V3Param: " << refp);
                        newp = ನೂತನ AstUnlinkedRef{nodep->fileline(), refp, refp->name(),
                                                  m_ds.m_unlinkedScopep->unlinkFrBack()};
                        m_ds.m_unlinkedScopep = nullptr;
                        m_ds.m_unresolvedCell = ಅಸತ್ಯ;
                    }
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                    ok = ಸತ್ಯ;
                    m_ds.m_dotText = "";
                }
            } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstLambdaArgRef* ಧ್ರುವ argrefp
                       = VN_CAST(foundp->nodep(), LambdaArgRef)) {
                ಯದಿ (allowVar) {
                    Ast_ಗಂಟು* ಧ್ರುವ newp
                        = ನೂತನ AstLambdaArgRef{nodep->fileline(), argrefp->name(), ಅಸತ್ಯ};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                    ok = ಸತ್ಯ;
                    m_ds.m_dotPos = DP_MEMBER;
                    m_ds.m_dotText = "";
                }
            } ಅನ್ಯಥಾ ಯದಿ (VN_IS(foundp->nodep(), Clocking)) {
                m_ds.m_dotSymp = foundp;
                ಯದಿ (m_ds.m_dotText != "") m_ds.m_dotText += "." + nodep->name();
                ok = m_ds.m_dotPos == DP_SCOPE || m_ds.m_dotPos == DP_FIRST;
            } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstModportClockingRef* ಧ್ರುವ clockingRefp
                       = VN_CAST(foundp->nodep(), ModportClockingRef)) {
                // Clocking block accessed through a modport - redirect to actual clocking
                m_ds.m_dotSymp = m_statep->get_ಗಂಟು_Sym(clockingRefp->clockingp());
                ಯದಿ (m_ds.m_dotText != "") m_ds.m_dotText += "." + nodep->name();
                ok = m_ds.m_dotPos == DP_SCOPE || m_ds.m_dotPos == DP_FIRST;
            } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ Ast_ಗಂಟು_FTask* ಧ್ರುವ ftaskp = VN_CAST(foundp->nodep(), ಗಂಟು_FTask)) {
                ಯದಿ (!ftaskp->isFunction() || ftaskp->classMethod()) {
                    ok = m_ds.m_dotPos == DP_NONE;
                    ಯದಿ (ok) {
                        // The condition is true for tasks,
                        // properties and void functions.
                        // In these cases, the parentheses may be skipped.
                        // Also SV class methods can be called without parens
                        AstFuncRef* ಧ್ರುವ funcRefp
                            = ನೂತನ AstFuncRef{nodep->fileline(), nodep->name()};
                        nodep->replaceWith(funcRefp);
                        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                    }
                }
            } ಅನ್ಯಥಾ ಯದಿ (AstTypedef* ಧ್ರುವ defp = VN_CAST(foundp->nodep(), Typedef)) {
                ಧ್ರುವ ತಾರ್ಕಿಕ ifaceFinalSegmentAllowed
                    = (m_ds.m_dotPos == DP_FINAL) && m_ds.m_dotSymp
                      && (VN_IS(m_ds.m_dotSymp->nodep(), Modport)
                          || (VN_IS(m_ds.m_dotSymp->nodep(), Cell)
                              && VN_CAST(m_ds.m_dotSymp->nodep(), Cell)->modp()
                              && VN_IS(VN_CAST(m_ds.m_dotSymp->nodep(), Cell)->modp(), Iface)));
                // Allow interface typedef references even without IfaceCapture
                // The dependency graph handles resolution when IfaceCapture is disabled
                ok = (m_ds.m_dotPos == DP_NONE || m_ds.m_dotPos == DP_SCOPE
                      || ifaceFinalSegmentAllowed);
                ಯದಿ (ifaceFinalSegmentAllowed) {
                    UINFO(9, indent() << "allow final-segment typedef name="
                                      << nodep->prettyNameQ() << " dotText='" << m_ds.m_dotText
                                      << "' dotSym=" << m_ds.m_dotSymp);
                }
                ಯದಿ (ok) {
                    AstRefDType* ಧ್ರುವ refp = ನೂತನ AstRefDType{nodep->fileline(), nodep->name()};
                    // Don't check if typedef is to a <type T>::<reference> as might not be
                    // resolved yet
                    ಯದಿ (m_ds.m_dotPos == DP_NONE) checkDeclOrder(nodep, defp);
                    refp->typedefp(defp);

                    V3LinkDotIfaceCapture::captureTypedefContext(
                        refp, "typedef", static_cast<ಪೂರ್ಣಾಙ್ಕ>(m_ds.m_dotPos), m_ds.m_dotText,
                        m_ds.m_dotSymp, m_modp, [ಏತತ್]() { ಪ್ರತ್ಯಾಗಮ indent(); });

                    ಯದಿ (VN_IS(nodep->backp(), SelExtract)) {
                        m_packedArrayDtp = refp;
                    } ಅನ್ಯಥಾ {
                        replaceWithCheckBreak(nodep, refp);
                        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                    }
                }
            } ಅನ್ಯಥಾ ಯದಿ (AstParamTypeDType* ಧ್ರುವ defp = VN_CAST(foundp->nodep(), ParamTypeDType)) {
                ok = (m_ds.m_dotPos == DP_NONE || m_ds.m_dotPos == DP_SCOPE
                      || m_ds.m_dotPos == DP_FINAL);
                ಯದಿ (ok) {
                    AstRefDType* ಧ್ರುವ refp = ನೂತನ AstRefDType{nodep->fileline(), nodep->name()};
                    refp->refDTypep(defp);

                    V3LinkDotIfaceCapture::captureTypedefContext(
                        refp, "paramtype", static_cast<ಪೂರ್ಣಾಙ್ಕ>(m_ds.m_dotPos), m_ds.m_dotText,
                        m_ds.m_dotSymp, m_modp, [ಏತತ್]() { ಪ್ರತ್ಯಾಗಮ indent(); });

                    ಯದಿ (VN_IS(nodep->backp(), SelExtract)) {
                        m_packedArrayDtp = refp;
                    } ಅನ್ಯಥಾ {
                        replaceWithCheckBreak(nodep, refp);
                        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                    }
                }
            }
            ಯದಿ (!ok) {
                ಯದಿ (m_insideClassExtParam) {
                    // Don't throw error if the reference is inside a class that extends a param,
                    // because some members can't be linked in such a case. m_insideClassExtParam
                    // may be true only in the first stage of linking.
                    // Mark that the Dot statement can't be resolved.
                    m_ds.m_unresolvedClass = ಸತ್ಯ;
                    // If the symbol was a scope name, it would be resolved.
                    m_ds.m_dotPos = DP_MEMBER;
                } ಅನ್ಯಥಾ {
                    // Cells/interfaces can't be implicit
                    ಧ್ರುವ ತಾರ್ಕಿಕ checkImplicit
                        = (!m_ds.m_dotp && m_ds.m_dotText == "" && !m_ds.m_disablep && !foundp);
                    ಧ್ರುವ ತಾರ್ಕಿಕ err
                        = !(checkImplicit && m_statep->implicitOk(m_modp, nodep->name()));
                    ಯದಿ (err) {
                        ಯದಿ (foundp) {
                            nodep->v3error("Found definition of '"
                                           << m_ds.m_dotText << (m_ds.m_dotText == "" ? "" : ".")
                                           << nodep->prettyName() << "'" << " as a "
                                           << foundp->nodep()->typeName() << " but expected a "
                                           << expectWhat);
                        } ಅನ್ಯಥಾ ಯದಿ (m_ds.m_dotText == "") {
                            UINFO(1, "   ErrParseRef curSymp=se" << cvtToHex(m_curSymp)
                                                                 << " ds=" << m_ds.ascii());
                            ಧ್ರುವ string suggest = m_statep->suggestSymFallback(
                                m_ds.m_dotSymp, nodep->name(), VNodeMatcher{});
                            nodep->v3error(
                                "Can't find definition of "
                                << expectWhat << ": " << nodep->prettyNameQ() << '\n'
                                << (suggest.empty() ? "" : nodep->warnMore() + suggest));
                        } ಅನ್ಯಥಾ {
                            nodep->v3error(
                                "Can't find definition of "
                                << (!baddot.empty() ? Ast_ಗಂಟು::prettyNameQ(baddot)
                                                    : nodep->prettyNameQ())
                                << " in dotted " << expectWhat << ": '"
                                << m_ds.m_dotText + "." + nodep->prettyName() << "'\n"
                                << nodep->warnContextPrimary()
                                << (okSymp ? okSymp->cellErrorScopes(
                                                 nodep, Ast_ಗಂಟು::prettyName(m_ds.m_dotText))
                                           : ""));
                        }
                        m_ds.m_dotErr = ಸತ್ಯ;
                    }
                    ಯದಿ (checkImplicit) {
                        // Create if implicit, and also if error (so only complain once)
                        // Else if a scope is allowed, making a signal won't help error cascade
                        AstVar* ಧ್ರುವ varp
                            = createImplicitVar(m_curSymp, nodep, m_modp, m_modSymp, err);
                        AstVarRef* ಧ್ರುವ newp
                            = ನೂತನ AstVarRef{nodep->fileline(), varp, VAccess::READ};
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                    }
                }
            }
        }
        ಯದಿ (start) m_ds = lastStates;
    }
    ಶೂನ್ಯ visit(AstClassOrPackageRef* nodep) override {
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

            ಯದಿ (!nodep->classOrPackageSkipp() && !nodep->classOrPackageNodep()
                && nodep->name() != "local::") {
                ಧ್ರುವ ತಾರ್ಕಿಕ deferIfUnresolved = m_statep->forPrimary() && m_insideClassExtParam;
                m_statep->resolveClassOrPackage(m_ds.m_dotSymp, nodep, m_ds.m_dotPos != DP_PACKAGE,
                                                ಅಸತ್ಯ, ":: reference", deferIfUnresolved);
            }

            // ClassRef's have pins, so track
            ಯದಿ (nodep->classOrPackageSkipp()) {
                m_pinSymp = m_statep->get_ಗಂಟು_Sym(nodep->classOrPackageSkipp());
            } ಅನ್ಯಥಾ ಯದಿ (nodep->name() != "local::") {
                ಪ್ರತ್ಯಾಗಮ;
            }

            AstClass* ಧ್ರುವ refClassp = VN_CAST(nodep->classOrPackageSkipp(), Class);
            // Make sure any extends() are properly imported within referenced class
            ಯದಿ (refClassp && !m_statep->forPrimary()) classExtendImport(refClassp);

            ಯದಿ (m_statep->forPrimary() && refClassp && !nodep->paramsp()
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
                AstPin* ಧ್ರುವ pinp = ನೂತನ AstPin{nodep->fileline(), 1, "__paramNumber1", nullptr};
                pinp->param(ಸತ್ಯ);
                nodep->addParamsp(pinp);
            }

            m_ds.init(m_curSymp);
            UINFO(4, indent() << "(Backto) Link ClassOrPackageRef: " << nodep);
            iterateChildren(nodep);
        }

        ಯದಿ (nodep->name() == "local::") {
            ಯದಿ (!m_randSymp && !m_randMethodCallp) {
                nodep->v3error("Illegal 'local::' outside 'randomize() with'"
                               " (IEEE 1800-2023 18.7.1)");
                m_ds.m_dotErr = ಸತ್ಯ;
                ಪ್ರತ್ಯಾಗಮ;
            }
        }
        ಯದಿ (m_ds.m_dotPos == DP_PACKAGE && nodep->classOrPackageSkipp()) {
            m_ds.m_dotSymp = m_statep->get_ಗಂಟು_Sym(nodep->classOrPackageSkipp());
            UINFO(9, indent() << "set sym " << m_ds.ascii());
        }
    }
    ಶೂನ್ಯ visit(AstConstraint* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        ಯದಿ (nodep->isExternDef()) {
            ಯದಿ (ಧ್ರುವ VSymEnt* ಧ್ರುವ foundp
                = m_curSymp->findIdFallback("extern " + nodep->name())) {
                ಧ್ರುವ AstConstraint* ಧ್ರುವ protop = VN_AS(foundp->nodep(), Constraint);
                // Copy specifiers.
                // External definition cannot have any specifiers, so no value will be overwritten.
                nodep->isStatic(protop->isStatic());
            } ಅನ್ಯಥಾ {
                nodep->v3error("extern not found that declares " + nodep->prettyNameQ());
            }
        }
        ಯದಿ (nodep->isExternProto()) {
            ಯದಿ (!m_curSymp->findIdFallback(nodep->name()) && nodep->isExternExplicit()) {
                nodep->v3warn(PROTOTYPEMIS, "Definition not found for extern prototype "
                                                << nodep->prettyNameQ());
            }
        }
        VL_RESTORER(m_curSymp);
        VL_RESTORER_COPY(m_ds);
        m_ds.m_dotSymp = m_curSymp = m_statep->get_ಗಂಟು_Sym(nodep);
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstConstraintRef* nodep) override {
        ಯದಿ (nodep->user3SetOnce()) ಪ್ರತ್ಯಾಗಮ;
        LINKDOT_VISIT_START();
        UINFO(8, indent() << "visit " << nodep);
        UINFO(8, indent() << m_ds.ascii());
        // No children defined
    }
    ಶೂನ್ಯ visit(AstVarRef* nodep) override {
        // VarRef: Resolve its reference
        // ParseRefs are used the first pass (forPrimary) so we shouldn't get can't find
        // errors here now that we have a VarRef.
        // No checkNoDot; created and iterated from a parseRef
        LINKDOT_VISIT_START();
        iterateChildren(nodep);
        ಯದಿ (!nodep->varp()) {
            UINFO(9, indent() << "linkVarRef se" << cvtToHex(m_curSymp) << "  n=" << nodep);
            UASSERT_OBJ(m_curSymp, nodep, "nullptr lookup symbol table");
            VSymEnt* ಧ್ರುವ foundp = m_curSymp->findIdFallback(nodep->name());
            ಯದಿ (AstVar* ಧ್ರುವ varp
                = foundp ? foundToVarp(foundp, nodep, nodep->access()) : nullptr) {
                nodep->varp(varp);
                updateVarUse(nodep->varp());
                // Generally set by parse, but might be an import
                nodep->classOrPackagep(foundp->classOrPackagep());
            }
            ಯದಿ (VL_UNCOVERABLE(!nodep->varp())) {
                nodep->v3error("Can't find definition of signal, again: "  // LCOV_EXCL_LINE
                               << nodep->prettyNameQ());
            }
        }
        AstVarScope* vscp = nodep->varScopep();
        ಯದಿ (vscp && vscp->user2p() != vscp && m_replaceWithAlias) {
            vscp = LinkDotScopeVisitor::getAlias(vscp);
            nodep->varp(vscp->varp());
            nodep->varScopep(vscp);
            updateVarUse(nodep->varp());
            UINFO(7, indent() << "Resolved " << nodep);  // Also prints taskp
        }
    }
    ಶೂನ್ಯ visit(AstVarXRef* nodep) override {
        // VarRef: Resolve its reference
        // We always link even if varp() is set, because the module we choose may change
        // due to creating new modules, flattening, etc.
        ಯದಿ (nodep->user3SetOnce()) ಪ್ರತ್ಯಾಗಮ;
        LINKDOT_VISIT_START();
        UINFO(8, indent() << "visit " << nodep);
        // No checkNoDot; created and iterated from a parseRef
        ಯದಿ (!m_modSymp) {
            // Module that is not in hierarchy.  We'll be dead code eliminating it later.
            UINFO(9, "Dead module for " << nodep);
            nodep->varp(nullptr);
        } ಅನ್ಯಥಾ {
            VSymEnt* lookupSymp = m_curSymp;  // Start search at current scope
            ಯದಿ (nodep->inlinedDots() != "") {  // Correct for current post-inlined scope
                lookupSymp = findInlinedDotsSym(nodep, nodep->inlinedDots());
            }
            string baddot;
            VSymEnt* okSymp;
            ತಾರ್ಕಿಕ modport;
            VSymEnt* ಧ್ರುವ dotSymp
                = findDottedRefSymp(nodep, lookupSymp, nodep->dotted(), baddot, okSymp, &modport);
            ಯದಿ (!dotSymp) ಪ್ರತ್ಯಾಗಮ;

            ಯದಿ (!m_statep->forScopeCreation()) {
                VSymEnt* foundp = nullptr;
                ಯದಿ (modport) {
                    // This is a copy of findSymPrefixed with few modifications.
                    // This searches without a fallback like findSymPrefixed but if lookup fails it
                    // will check one fallback (fallback of modport shall be interface) and take
                    // only L and G PARAMs since they should be visible from a modport always.
                    string prefix = dotSymp->symPrefix();
                    ಯಾವತ್ (!foundp) {
                        foundp = dotSymp->findIdFlat(prefix + nodep->name());
                        ಯದಿ (foundp) ಭಙ್ಗ;
                        UASSERT_OBJ(dotSymp->fallbackp(), nodep, "Modports shall have fallback");
                        foundp = dotSymp->fallbackp()->findIdFlat(prefix + nodep->name());
                        ಯದಿ (ಧ್ರುವ AstVar* ಧ್ರುವ varp
                            = foundToVarp(foundp, nodep, nodep->access())) {
                            ಯದಿ (!varp->isParam()) foundp = nullptr;
                        }
                        ಯದಿ (prefix.empty()) ಭಙ್ಗ;
                        string nextPrefix = LinkDotState::removeLastInlineScope(prefix);
                        ಯದಿ (prefix == nextPrefix) ಭಙ್ಗ;
                        prefix = std::move(nextPrefix);
                    }
                    baddot = nodep->name();
                } ಅನ್ಯಥಾ {
                    foundp = m_statep->findSymPrefixed(dotSymp, nodep->name(), baddot, ಸತ್ಯ);
                }
                ಯದಿ (m_inSens && foundp) {
                    ಯದಿ (AstClocking* ಧ್ರುವ clockingp = sensClockingp(foundp->nodep())) {
                        foundp = getCreateClockingEventSymEnt(clockingp);
                    }
                }
                AstVar* ಧ್ರುವ varp
                    = foundp ? foundToVarp(foundp, nodep, nodep->access()) : nullptr;
                nodep->varp(varp);
                updateVarUse(nodep->varp());
                UINFO(7, indent() << "Resolved " << nodep);  // Also prints varp
                // If found, check if it's ok to access in case it's in a hier_block
                ಯದಿ (nodep->varp() && errorHierNonPort(nodep, nodep->varp(), dotSymp)) ಪ್ರತ್ಯಾಗಮ;
                ಯದಿ (!nodep->varp()) {
                    nodep->v3error("Can't find definition of "
                                   << Ast_ಗಂಟು::prettyNameQ(baddot) << " in dotted signal: '"
                                   << nodep->dotted() + "." + nodep->prettyName() << "'\n"
                                   << nodep->warnContextPrimary()
                                   << okSymp->cellErrorScopes(nodep));
                    ಪ್ರತ್ಯಾಗಮ;
                }
            } ಅನ್ಯಥಾ {
                VSymEnt* ಧ್ರುವ foundp
                    = m_statep->findSymPrefixed(dotSymp, nodep->name(), baddot, ಸತ್ಯ);
                AstVarScope* vscp = foundp ? VN_CAST(foundp->nodep(), VarScope) : nullptr;
                // Handle modport expression virtual ports
                ಯದಿ (!vscp) vscp = findVirtualPortVarScope(dotSymp, nodep->name());
                // If found, check if it's ok to access in case it's in a hier_block
                ಯದಿ (vscp && errorHierNonPort(nodep, vscp->varp(), dotSymp)) ಪ್ರತ್ಯಾಗಮ;
                ಯದಿ (!vscp) {
                    nodep->v3error("Can't find varpin scope of "
                                   << Ast_ಗಂಟು::prettyNameQ(baddot) << " in dotted signal: '"
                                   << nodep->dotted() + "." + nodep->prettyName() << "'\n"
                                   << nodep->warnContextPrimary()
                                   << okSymp->cellErrorScopes(nodep));
                } ಅನ್ಯಥಾ {
                    ಯದಿ (vscp->user2p() && m_replaceWithAlias) {
                        vscp = LinkDotScopeVisitor::getAlias(vscp);
                    }
                    // Convert the VarXRef to a VarRef, so we don't need
                    // later optimizations to deal with VarXRef.
                    nodep->varp(vscp->varp());
                    nodep->varScopep(vscp);
                    updateVarUse(nodep->varp());
                    UINFO(7, indent() << "Resolved " << nodep);  // Also prints taskp
                    AstVarRef* ಧ್ರುವ newvscp
                        = ನೂತನ AstVarRef{nodep->fileline(), vscp, nodep->access()};
                    nodep->replaceWith(newvscp);
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                    UINFO(9, indent() << "new " << newvscp);  // Also prints taskp
                }
            }
        }
    }
    ಶೂನ್ಯ visit(AstEnumDType* nodep) override {
        LINKDOT_VISIT_START();
        iterateChildren(nodep);
        AstRefDType* ಧ್ರುವ refdtypep = VN_CAST(nodep->subDTypep(), RefDType);
        ಯದಿ (refdtypep && (nodep == refdtypep->subDTypep())) {
            refdtypep->v3error("Self-referential enumerated type definition");
        }
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_UOr_Struct_DType* nodep) override {
        LINKDOT_VISIT_START();
        symIterateChildren(nodep, m_statep->get_ಗಂಟು_Sym(nodep));
    }
    ಶೂನ್ಯ visit(AstEnumItemRef* nodep) override {
        // Resolve its reference
        // EnumItemRefs are created by the first pass, but V3Param may regenerate due to
        // a parameterized class/module, so we shouldn't get can't find errors.
        // No checkNoDot; created and iterated from a parseRef
        ಯದಿ (nodep->user3SetOnce()) ಪ್ರತ್ಯಾಗಮ;
        LINKDOT_VISIT_START();
        ಯದಿ (m_statep->forParamed() && !nodep->dotted().empty() && m_modSymp) {
            // Hierarchical reference, relink as V3Param may have specialized the module
            string baddot;
            VSymEnt* okSymp = nullptr;
            VSymEnt* ಧ್ರುವ dotSymp
                = findDottedRefSymp(nodep, m_curSymp, nodep->dotted(), baddot, okSymp);
            ಯದಿ (!dotSymp) ಪ್ರತ್ಯಾಗಮ;
            VSymEnt* ಧ್ರುವ foundp
                = m_statep->findSymPrefixed(dotSymp, nodep->name(), baddot, ಸತ್ಯ);
            AstEnumItem* ಧ್ರುವ itemp = foundp ? VN_CAST(foundp->nodep(), EnumItem) : nullptr;
            ಯದಿ (!itemp) {
                nodep->v3error("Enum item " << nodep->prettyNameQ() << " not found in "
                                            << Ast_ಗಂಟು::prettyNameQ(nodep->dotted()) << '\n'
                                            << nodep->warnContextPrimary());
                ಪ್ರತ್ಯಾಗಮ;
            }
            nodep->itemp(itemp);
            UINFO(9, indent() << " relinked " << nodep);
        }
        ಯದಿ (!nodep->itemp()) {
            UINFO(9, indent() << "linkEnumRef se" << cvtToHex(m_curSymp) << "  n=" << nodep);
            UASSERT_OBJ(m_curSymp, nodep, "nullptr lookup symbol table");
            VSymEnt* ಧ್ರುವ foundp = m_curSymp->findIdFallback(nodep->name());
            ಯದಿ (AstEnumItem* ಧ್ರುವ itemp = foundp ? VN_CAST(foundp->nodep(), EnumItem) : nullptr) {
                nodep->itemp(itemp);
                nodep->classOrPackagep(foundp->classOrPackagep());
                UINFO(9, indent() << " resolved " << nodep);
            }
            ಯದಿ (VL_UNCOVERABLE(!nodep->itemp())) {
                nodep->v3error("Can't find definition of enum item, again: "  // LCOV_EXCL_LINE
                               << nodep->prettyNameQ());
            }
        }
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstMethodCall* nodep) override {
        // Created here so should already be resolved.
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        VL_RESTORER_COPY(m_ds);
        VL_RESTORER(m_randSymp);
        VL_RESTORER(m_randMethodCallp);
        {
            m_ds.init(m_curSymp);
            ಯದಿ (nodep->name() == "randomize" && (nodep->argsp() || nodep->withp())) {
                m_randMethodCallp = nodep;
                ಧ್ರುವ Ast_ಗಂಟು_DType* fromDtp = nodep->fromp()->dtypep();
                ಯದಿ (!fromDtp) {
                    ಯದಿ (ಧ್ರುವ Ast_ಗಂಟು_Var_Ref* ಧ್ರುವ varRefp = VN_CAST(nodep->fromp(), ಗಂಟು_Var_Ref)) {
                        fromDtp = varRefp->varp()->subDTypep();
                    } ಅನ್ಯಥಾ {
                        fromDtp = get_ಅಭಿವ್ಯಕ್ತಿ_DTypep(nodep->fromp());
                    }
                    ಯದಿ (!fromDtp) {
                        ಯದಿ (nodep->withp()) {
                            nodep->v3warn(
                                E_UNSUPPORTED,
                                "Unsupported: 'randomize() with' on complex expressions");
                        } ಅನ್ಯಥಾ {
                            nodep->v3warn(E_UNSUPPORTED,
                                          "Unsupported: Inline random variable control with "
                                          "'randomize()' called on complex expressions");
                        }
                    }
                }
                ಯದಿ (m_statep->forPrimary() && isParamedClassRefDType(fromDtp)) {
                    m_ds.m_unresolvedClass = ಸತ್ಯ;
                } ಅನ್ಯಥಾ ಯದಿ (fromDtp) {
                    ಧ್ರುವ AstClassRefDType* ಧ್ರುವ classDtp
                        = VN_CAST(fromDtp->skipRefp(), ClassRefDType);
                    ಯದಿ (!classDtp)
                        nodep->v3error("'randomize() with' on a non-class-instance "
                                       << fromDtp->prettyNameQ());
                    ಅನ್ಯಥಾ ಯದಿ (m_statep->forPrimary() && m_extendsParam.ಎಣಿಕೆ(classDtp->classp())) {
                        // Inherited members are unavailable until the base class is specialized.
                        // Defer lookup rather than incorrectly binding names in the caller.
                        m_ds.m_unresolvedClass = ಸತ್ಯ;
                    } ಅನ್ಯಥಾ {
                        m_randSymp = m_statep->get_ಗಂಟು_Sym(classDtp->classp());
                    }
                }
            }
            iterateChildren(nodep);
        }
    }
    ಶೂನ್ಯ visit(AstVar* nodep) override {
        LINKDOT_VISIT_START();
        checkNoDot(nodep);
        VL_RESTORER(m_isParam);
        m_isParam = nodep->varType().isParam();
        iterateChildren(nodep);
        ಯದಿ (m_statep->forPrimary() && nodep->isIO() && !m_ftaskp && !nodep->user4()) {
            nodep->v3error(
                "Input/output/inout does not appear in port list: " << nodep->prettyNameQ());
        }
    }
    ಶೂನ್ಯ visit(AstVarScope* nodep) override {
        LINKDOT_VISIT_START();
        checkNoDot(nodep);
        iterateChildren(nodep);
        AstVarScope* aliasp = LinkDotScopeVisitor::getAlias(nodep);
        ಯದಿ (aliasp && aliasp != nodep && !nodep->varp()->isIfaceRef()) {
            // Aliased variable might still be references from outside,
            // eg through the VPI, and is traced, so we need the value to propagate.
            // TODO: this means external writes to the LHS (e.g.: through the VPI) don't work
            AstAssignW* ಧ್ರುವ assignp = ನೂತನ AstAssignW{
                nodep->fileline(), ನೂತನ AstVarRef{nodep->fileline(), nodep, VAccess::WRITE},
                ನೂತನ AstVarRef{nodep->fileline(), aliasp, VAccess::READ}};
            assignp->user2(ಸತ್ಯ);
            nodep->scopep()->addBlocksp(ನೂತನ AstAlways{assignp});
            // Propagate attributes and lint state of the replaced variable,
            // because all references to it are replaced with references to the alias variable
            aliasp->varp()->propagateAttrFrom(nodep->varp());
            aliasp->varp()->fileline()->modifyStateInherit(nodep->varp()->fileline());
        }
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_FTask_Ref* nodep) override {
        ಯದಿ (nodep->user3SetOnce()) ಪ್ರತ್ಯಾಗಮ;
        LINKDOT_VISIT_START();
        UINFO(8, indent() << "visit " << nodep);
        UINFO(8, indent() << m_ds.ascii());
        ಯದಿ (m_ds.m_dotPos != DP_MEMBER || nodep->name() != "randomize") {
            // Visit arguments at the beginning.
            // They may be visitted even if the current node can't be linked now.
            symIterateChildren(nodep, m_curSymp);
        }

        ಯದಿ (m_ds.m_super) {
            ಯದಿ (AstFuncRef* ಧ್ರುವ funcRefp = VN_CAST(nodep, FuncRef)) {
                funcRefp->superReference(ಸತ್ಯ);
            } ಅನ್ಯಥಾ ಯದಿ (AstTaskRef* ಧ್ರುವ taskRefp = VN_CAST(nodep, TaskRef)) {
                taskRefp->superReference(ಸತ್ಯ);
            }
        }

        VL_RESTORER(m_randSymp);

        ತಾರ್ಕಿಕ first = !m_ds.m_dotp || m_ds.m_dotPos == DP_FIRST;
        ತಾರ್ಕಿಕ staticAccess = ಅಸತ್ಯ;
        ಯದಿ (m_ds.m_unresolvedClass) {
            // Unable to link before V3Param
            ಪ್ರತ್ಯಾಗಮ;
        } ಅನ್ಯಥಾ ಯದಿ (m_ds.m_unresolvedCell && m_ds.m_dotPos == DP_FINAL && m_ds.m_unlinkedScopep) {
            Ast_ಗಂಟು_FTask_Ref* ಧ್ರುವ newftaskp = nodep->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);
            newftaskp->dotted(m_ds.m_dotText);
            Ast_ಗಂಟು* ಧ್ರುವ newp = ನೂತನ AstUnlinkedRef{nodep->fileline(), newftaskp, nodep->name(),
                                                     m_ds.m_unlinkedScopep->unlinkFrBack()};
            m_ds.m_unlinkedScopep = nullptr;
            m_ds.m_unresolvedCell = ಅಸತ್ಯ;
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            ಪ್ರತ್ಯಾಗಮ;
        } ಅನ್ಯಥಾ ಯದಿ (m_ds.m_dotp && m_ds.m_dotPos == DP_PACKAGE) {
            UASSERT_OBJ(VN_IS(m_ds.m_dotp->lhsp(), ClassOrPackageRef), m_ds.m_dotp->lhsp(),
                        "Bad package link");
            staticAccess = ಸತ್ಯ;
            AstClassOrPackageRef* ಧ್ರುವ cpackagerefp
                = VN_AS(m_ds.m_dotp->lhsp(), ClassOrPackageRef);
            ಯದಿ (cpackagerefp->name() == "local::") {
                m_randSymp = nullptr;
                first = ಸತ್ಯ;
            } ಅನ್ಯಥಾ ಯದಿ (!cpackagerefp->classOrPackageSkipp()) {
                VSymEnt* ಧ್ರುವ foundp = m_statep->resolveClassOrPackage(
                    m_ds.m_dotSymp, cpackagerefp, ಸತ್ಯ, ಅಸತ್ಯ, ":: reference");
                ಯದಿ (foundp) nodep->classOrPackagep(cpackagerefp->classOrPackageSkipp());
            } ಅನ್ಯಥಾ {
                nodep->classOrPackagep(cpackagerefp->classOrPackageSkipp());
            }
            // Class/package :: HERE function() . method_called_on_function_return_value()
            m_ds.m_dotPos = DP_MEMBER;
        } ಅನ್ಯಥಾ ಯದಿ (m_ds.m_dotp && m_ds.m_dotPos == DP_FINAL) {
            nodep->dotted(m_ds.m_dotText);  // Maybe ""
            // Only flag FTaskRefs under generate-if/case blocks that may be
            // pruned.  GenFor and plain begin-blocks won't be pruned by V3Param.
            // A generate-case block's parent is the GenCaseItem, not the GenCase.
            // VarXRef uses the broader m_genBlk flag (set for all GenBlocks)
            // because genfor unrolling also removes variables.
            ಯದಿ (m_ds.m_genBlk && m_ds.m_dotSymp) {
                ಧ್ರುವ Ast_ಗಂಟು* ಧ್ರುವ blkp = m_ds.m_dotSymp->nodep();
                ಯದಿ (VN_IS(blkp, GenBlock)
                    && (VN_IS(blkp->backp(), GenIf) || VN_IS(blkp->backp(), GenCaseItem))) {
                    nodep->containsGenBlock(ಸತ್ಯ);
                }
            }
        } ಅನ್ಯಥಾ ಯದಿ (m_ds.m_dotp && m_ds.m_dotPos == DP_MEMBER) {
            // Found a Var, everything following is method call.
            // {scope}.{var}.HERE {method} ( ARGS )
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ varEtcp = VN_AS(m_ds.m_dotp->lhsp()->unlinkFrBack(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ);
            AstArg* ಧ್ರುವ argsp = nodep->argsp();
            ಯದಿ (argsp) argsp->unlinkFrBackWithNext();
            AstMethodCall* ಧ್ರುವ newp = ನೂತನ AstMethodCall{nodep->fileline(), varEtcp,
                                                          VFlagChildDType{}, nodep->name(), argsp};
            ಯದಿ (AstWith* ಧ್ರುವ withp = nodep->withp()) newp->withp(withp->unlinkFrBack());
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            ಪ್ರತ್ಯಾಗಮ;
        } ಅನ್ಯಥಾ ಯದಿ (m_ds.m_dotp && (m_ds.m_dotPos == DP_SCOPE || m_ds.m_dotPos == DP_FIRST)) {
            // HERE function() . method_called_on_function_return_value()
            m_ds.m_dotPos = DP_MEMBER;
            m_ds.m_dotText = "";
        } ಅನ್ಯಥಾ ಯದಿ (!m_ds.m_disablep) {
            // visit(AstDisable*) setup the dot handling
            checkNoDot(nodep);
        }
        ಯದಿ (nodep->classOrPackagep() && nodep->taskp()) {
            // References into packages don't care about cell hierarchy.
        } ಅನ್ಯಥಾ ಯದಿ (!m_modSymp) {
            // Module that is not in hierarchy.  We'll be dead code eliminating it later.
            UINFO(9, indent() << "Dead module for " << nodep);
            nodep->taskp(nullptr);
        } ಅನ್ಯಥಾ ಯದಿ (nodep->dotted() == "" && nodep->taskp()) {
            // Earlier should have setup the links
            // Might be under a BEGIN we're not processing, so don't relink it
        } ಅನ್ಯಥಾ {
            string baddot;
            VSymEnt* okSymp = nullptr;
            VSymEnt* dotSymp;
            ಯದಿ (nodep->dotted().empty()) {
                // Non-'super.' dotted reference
                dotSymp = m_ds.m_dotSymp;
            } ಅನ್ಯಥಾ {
                // Start search at module, as a variable of same name under a subtask isn't a
                // relevant hit however a function under a begin/end is.  So we want begins, but
                // not the function
                dotSymp = m_curSymp;
            }
            ಯದಿ (nodep->classOrPackagep()) {  // Look only in specified package
                dotSymp = m_statep->get_ಗಂಟು_Sym(nodep->classOrPackagep());
                UINFO(8, indent() << "Override classOrPackage " << dotSymp);
            } ಅನ್ಯಥಾ {
                ಯದಿ (nodep->inlinedDots() != "") {  // Correct for current post-inlined scope
                    dotSymp = findInlinedDotsSym(nodep, nodep->inlinedDots());
                }
                dotSymp = m_statep->findDotted(nodep->fileline(), dotSymp, nodep->dotted(), baddot,
                                               okSymp, ಸತ್ಯ);  // Maybe nullptr
            }
            // Restricted 'with (id_list)': only id_list names bind into target class.
            ಯದಿ (m_randSymp
                && (!m_currentWithp || m_currentWithp->nameResolvesToTarget(nodep->name()))) {
                VSymEnt* ಧ್ರುವ foundp = m_randSymp->findIdFlat(nodep->name());
                ಯದಿ (foundp && m_currentWithp) {
                    ಯದಿ (m_currentWithp->restricted()) {
                        m_restrictedNamesUsed.ಸೇರಿಸು(nodep->name());
                    }
                    UINFO(9, indent() << "randomize-with fromSym " << foundp->nodep());
                    AstArg* argsp = nullptr;
                    ಯದಿ (nodep->argsp()) {
                        iterateAndNextNull(nodep->argsp());
                        argsp = nodep->argsp()->unlinkFrBackWithNext();
                    }
                    ಯದಿ (m_ds.m_dotPos != DP_NONE) m_ds.m_dotPos = DP_MEMBER;
                    Ast_ಗಂಟು* ಧ್ರುವ newp = ನೂತನ AstMethodCall{
                        nodep->fileline(),
                        ನೂತನ AstLambdaArgRef{nodep->fileline(), "__Vrandwith_obj", ಅಸತ್ಯ},
                        VFlagChildDType{}, nodep->name(), argsp};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                    ಪ್ರತ್ಯಾಗಮ;
                }
            }
            ಯದಿ (first && nodep->name() == "randomize" && VN_IS(m_modp, Class)
                && !VN_IS(nodep->classOrPackagep(), Package)) {
                // need special handling to avoid falling back to std::randomize
                // Skip if classOrPackagep is a Package (i.e. std::randomize resolved earlier)
                VMemberMap memberMap;
                AstFunc* ಧ್ರುವ randFuncp = V3Randomize::newRandomizeFunc(
                    memberMap, VN_AS(m_modp, Class), nodep->name(), ಸತ್ಯ, ಸತ್ಯ);
                nodep->taskp(randFuncp);
                nodep->classOrPackagep(VN_AS(m_modp, Class));
                m_curSymp = m_statep->ಸೇರಿಸು_Block(m_curSymp, nodep->name(), randFuncp, m_modp);
                // Already linked to the class randomize function; skip findSymPrefixed
                // which may find std::randomize and overwrite classOrPackagep
                ಪ್ರತ್ಯಾಗಮ;
            }
            VSymEnt* ಧ್ರುವ foundp
                = m_statep->findSymPrefixed(dotSymp, nodep->name(), baddot, first);
            Ast_ಗಂಟು_FTask* ಧ್ರುವ taskp = foundp ? VN_CAST(foundp->nodep(), ಗಂಟು_FTask) : nullptr;
            Ast_ಗಂಟು_Module* ಧ್ರುವ taskModulep = foundp ? foundp->classOrPackagep() : nullptr;
            // Ignore deferring linking of functions inside params of classes extending
            // parametric classes if referenced function can be linked in the first pass.
            ಯದಿ (!m_isParam || !taskp
                || (taskp->classMethod() && taskModulep && taskModulep->hasParameterList())) {
                ಯದಿ (m_insideClassExtParam) {
                    // The reference may point to a method declared in a super class, which is
                    // proved by a parameter. In such a case, it can't be linked at the first
                    // stage. Must not do any linking, because e.g. might find an extends of an
                    // upper class because the current class (under parent) isn't yet importing
                    // it's extended class symbols.
                    ಪ್ರತ್ಯಾಗಮ;
                }
                ಯದಿ (AstClass* ಧ್ರುವ targetClassp = VN_CAST(dotSymp->nodep(), Class)) {
                    ಯದಿ (m_extendsParam.ಎಣಿಕೆ(targetClassp)) {
                        // Target class has parameterized extends not yet resolved.
                        // Its inherited symbols (e.g. static functions from the base class)
                        // aren't imported yet - defer to linkDotParamed.
                        ಪ್ರತ್ಯಾಗಮ;
                    }
                }
            }

            ಯದಿ (taskp) {
                ಯದಿ (staticAccess && !taskp->isStatic()) {
                    // TODO bug4077
                    // nodep->v3error("Static access to non-static task/function "
                    //                << taskp->prettyNameQ() << endl);
                }
                nodep->taskp(taskp);
                nodep->classOrPackagep(foundp->classOrPackagep());
                UINFO(7, indent() << "Resolved " << nodep);  // Also prints taskp
            } ಅನ್ಯಥಾ {
                // Note ParseRef has similar error handling/message output
                UINFO(7, indent() << "   ErrFtask curSymp=se" << cvtToHex(m_curSymp)
                                  << " dotSymp=se" << cvtToHex(dotSymp));
                ಯದಿ (foundp) {
                    ಯದಿ (VN_IS(foundp->nodep(), Var) && m_ds.m_dotText == "" && m_ftaskp
                        && m_ftaskp->name() == foundp->nodep()->name()) {
                        // This is a recursive reference to the function itself, not to the var
                        nodep->taskp(m_ftaskp);
                        nodep->classOrPackagep(foundp->classOrPackagep());
                        UINFO(7, indent() << "Resolved recursive " << nodep);  // Also prints taskp
                    } ಅನ್ಯಥಾ {
                        nodep->v3error("Found definition of '"
                                       << m_ds.m_dotText << (m_ds.m_dotText == "" ? "" : ".")
                                       << nodep->prettyName() << "'" << " as a "
                                       << foundp->nodep()->typeName()
                                       << " but expected a task/function");
                    }
                } ಅನ್ಯಥಾ ಯದಿ (VN_IS(nodep, New) && !m_statep->forScopeCreation()) {
                    // Resolved in V3Width
                } ಅನ್ಯಥಾ ಯದಿ ((nodep->name() == "pre_randomize" || nodep->name() == "post_randomize")
                           && VN_IS(dotSymp->nodep(), Class)) {
                    AstClass* ಧ್ರುವ classp = VN_AS(dotSymp->nodep(), Class);
                    // Functions that might be defined by user, but if not, they get empty
                    // definitions within referenced class.
                    // TODO perhaps cleaner to add to symbol table up-front with
                    // a lambda to construct them on a reference.
                    AstFunc* ಧ್ರುವ funcp
                        = ನೂತನ AstFunc{nodep->fileline(), nodep->name(), nullptr, nullptr};
                    funcp->classMethod(ಸತ್ಯ);
                    funcp->dtypep(funcp->findVoidDType());
                    classp->addMembersp(funcp);
                    m_statep->ಸೇರಿಸು_Block(dotSymp, nodep->name(), funcp, classp);
                    nodep->taskp(funcp);
                    nodep->classOrPackagep(classp);
                    UINFO(9, indent() << " Auto-made " << funcp);
                    ಪ್ರತ್ಯಾಗಮ;
                } ಅನ್ಯಥಾ ಯದಿ (nodep->name() == "randomize" || nodep->name() == "srandom"
                           || nodep->name() == "get_randstate" || nodep->name() == "set_randstate"
                           || nodep->name() == "rand_mode" || nodep->name() == "constraint_mode") {
                    ಯದಿ (AstClass* ಧ್ರುವ classp = VN_CAST(m_modp, Class)) {
                        // Don't overwrite if already resolved to a package (e.g. std::randomize)
                        ಯದಿ (!VN_IS(nodep->classOrPackagep(), Package))
                            nodep->classOrPackagep(classp);
                    } ಅನ್ಯಥಾ ಯದಿ (nodep->name() == "randomize") {
                        // A std::randomize resolved in V3Width
                        nodep->classOrPackagep(v3Global.rootp()->stdPackagep());
                    } ಅನ್ಯಥಾ {
                        nodep->v3error("Calling implicit class method "
                                       << nodep->prettyNameQ() << " without being under class");
                        nodep->replaceWith(ನೂತನ AstConst{nodep->fileline(), 0});
                        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                        ಪ್ರತ್ಯಾಗಮ;
                    }
                } ಅನ್ಯಥಾ ಯದಿ (nodep->dotted() == "") {
                    ಯದಿ (nodep->pli()) {
                        ಯದಿ (v3Global.opt.bboxSys()) {
                            Ast_ಗಂಟು* newp;
                            ಯದಿ (VN_IS(nodep, FuncRef)) {
                                newp = ನೂತನ AstConst{nodep->fileline(), AstConst::All0{}};
                            } ಅನ್ಯಥಾ {
                                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* outp = nullptr;
                                ಯಾವತ್ (AstArg* ಧ್ರುವ argp = nodep->argsp()) {
                                    outp = Ast_ಗಂಟು::addNext(outp, argp->exprp()->unlinkFrBack());
                                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(argp->unlinkFrBack()), argp);
                                }
                                newp = ನೂತನ AstSysIgnore{nodep->fileline(), outp};
                                newp->dtypep(nodep->dtypep());
                            }
                            nodep->replaceWith(newp);
                            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                            ಪ್ರತ್ಯಾಗಮ;
                        } ಅನ್ಯಥಾ {
                            VSpellCheck speller;
                            V3Parse::candidatePli(&speller);
                            ಧ್ರುವ string suggest = speller.best_Candidate_ಸಂದೇಶ(nodep->prettyName());
                            nodep->v3error(
                                "Unsupported or unknown PLI call: "
                                << nodep->prettyNameQ() << '\n'
                                << (suggest.empty() ? "" : nodep->warnMore() + suggest));
                        }
                    } ಅನ್ಯಥಾ {
                        ಧ್ರುವ string suggest = m_statep->suggestSymFallback(
                            dotSymp, nodep->name(), Link_ಗಂಟು_Matcher_FTask{});
                        nodep->v3error("Can't find definition of task/function: "
                                       << nodep->prettyNameQ() << '\n'
                                       << (suggest.empty() ? "" : nodep->warnMore() + suggest));
                    }
                } ಅನ್ಯಥಾ {
                    ಧ್ರುವ string suggest = m_statep->suggestSymFallback(dotSymp, nodep->name(),
                                                                        Link_ಗಂಟು_Matcher_FTask{});
                    nodep->v3error("Can't find definition of "
                                   << Ast_ಗಂಟು::prettyNameQ(baddot) << " in dotted task/function: '"
                                   << nodep->dotted() + "." + nodep->prettyName() << "'\n"
                                   << (suggest.empty() ? "" : nodep->warnMore() + suggest) << '\n'
                                   << nodep->warnContextPrimary()
                                   << okSymp->cellErrorScopes(nodep));
                }
            }
        }
    }
    ಶೂನ್ಯ visit(AstSelBit* nodep) override {
        ಯದಿ (nodep->user3SetOnce()) ಪ್ರತ್ಯಾಗಮ;
        LINKDOT_VISIT_START();
        iterateAndNextNull(nodep->fromp());
        ಯದಿ (m_ds.m_unresolvedClass) {
            UASSERT_OBJ(m_ds.m_dotPos != DP_SCOPE && m_ds.m_dotPos != DP_FIRST, nodep,
                        "Object of unresolved class on scope position in dotted reference");
            // The index does not depend on the class, and V3Param must see its class references
            symIterateNull(nodep->bitp(), m_curSymp);
            symIterateNull(nodep->attrp(), m_curSymp);
            ಪ್ರತ್ಯಾಗಮ;
        }
        ಯದಿ (m_ds.m_dotPos == DP_SCOPE
            || m_ds.m_dotPos
                   == DP_FIRST) {  // Already under dot, so this is {modulepart} DOT {modulepart}
            UINFO(9, indent() << "deferring until after a V3Param pass: " << nodep);
            m_ds.m_dotText += "__BRA__??__KET__";
            m_ds.m_unresolvedCell = ಸತ್ಯ;
            // And pass up m_ds.m_dotText
        }
        // Pass dot state down to only fromp()
        iterateAndNextNull(nodep->fromp());
        symIterateNull(nodep->bitp(), m_curSymp);
        symIterateNull(nodep->attrp(), m_curSymp);
        ಯದಿ (m_ds.m_unresolvedCell && (m_ds.m_dotPos == DP_SCOPE || m_ds.m_dotPos == DP_FIRST)) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ exprp = nodep->bitp()->unlinkFrBack();
            ಯದಿ (AstCellArrayRef* ಧ್ರುವ fromArrRefp = VN_CAST(nodep->fromp(), CellArrayRef)) {
                // Multi-dim iface array access: append this select to the existing chain
                fromArrRefp->addSelp(exprp);
                AstCellArrayRef* ಧ್ರುವ movedp = fromArrRefp->unlinkFrBack();
                nodep->replaceWith(movedp);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            } ಅನ್ಯಥಾ {
                AstCellArrayRef* ಧ್ರುವ newp
                    = ನೂತನ AstCellArrayRef{nodep->fileline(), nodep->fromp()->name(), exprp};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            }
        }
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Pre_Sel* nodep) override {
        // Excludes simple AstSelBit, see above
        ಯದಿ (nodep->user3SetOnce()) ಪ್ರತ್ಯಾಗಮ;
        LINKDOT_VISIT_START();
        ಯದಿ (m_ds.m_dotPos == DP_SCOPE
            || m_ds.m_dotPos
                   == DP_FIRST) {  // Already under dot, so this is {modulepart} DOT {modulepart}
            nodep->v3error("Syntax error: Range ':', '+:' etc are not allowed in the instance "
                           "part of a dotted reference");
            m_ds.m_dotErr = ಸತ್ಯ;
            ಪ್ರತ್ಯಾಗಮ;
        }
        Ast_ಗಂಟು_DType* packedArrayDtp = nullptr;  // Datatype reference for packed array
        {
            VL_RESTORER(m_packedArrayDtp);
            iterateAndNextNull(nodep->fromp());
            symIterateNull(nodep->rhsp(), m_curSymp);
            symIterateNull(nodep->thsp(), m_curSymp);

            ಯದಿ (m_packedArrayDtp) {
                AstRange* ಧ್ರುವ newRangep
                    = ನೂತನ AstRange(nodep->fileline(), nodep->rhsp()->unlinkFrBack(),
                                   nodep->thsp()->unlinkFrBack());
                AstPackArrayDType* newArrayTypep
                    = ನೂತನ AstPackArrayDType(nodep->fileline(), m_packedArrayDtp, newRangep);
                newArrayTypep->childDTypep(m_packedArrayDtp);
                newArrayTypep->refDTypep(nullptr);
                ಯದಿ (VN_IS(nodep->backp(), SelExtract)) {
                    packedArrayDtp = newArrayTypep;
                } ಅನ್ಯಥಾ {
                    replaceWithCheckBreak(nodep, newArrayTypep);
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                }
            } ಅನ್ಯಥಾ {
                ಯದಿ (nodep->attrp()) {
                    Ast_ಗಂಟು* ಧ್ರುವ attrp = nodep->attrp()->unlinkFrBack();
                    VL_DO_DANGLING(attrp->delete_ವೃಕ್ಷ(), attrp);
                }
                Ast_ಗಂಟು* ಧ್ರುವ basefromp = nodep->baseFromp(ಅಸತ್ಯ);
                ಯದಿ (VN_IS(basefromp, Replicate)) {
                    // From {...}[...] syntax in IEEE 2017
                    ಯದಿ (basefromp) UINFO(9, indent() << " Related node: " << basefromp);
                } ಅನ್ಯಥಾ {
                    nodep->attrp(ನೂತನ AstAttrOf{nodep->fileline(), VAttrType::VAR_BASE,
                                               basefromp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ)});
                }
            }
        }

        ಯದಿ (packedArrayDtp) m_packedArrayDtp = packedArrayDtp;
    }
    ಶೂನ್ಯ visit(AstMemberSel* nodep) override {
        // checkNoDot not appropriate, can be under a dot
        LINKDOT_VISIT_START();
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstGenBlock* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        {
            VL_RESTORER(m_curSymp);
            VL_RESTORER_COPY(m_ds);
            ಯದಿ (nodep->name() != "") {
                m_ds.m_dotSymp = m_curSymp = m_statep->get_ಗಂಟು_Sym(nodep);
                UINFO(5, indent() << "cur=se" << cvtToHex(m_curSymp));
            }
            iterateChildren(nodep);
        }
        UINFO(5, indent() << "cur=se" << cvtToHex(m_curSymp));
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Block* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        {
            VL_RESTORER(m_curSymp);
            VL_RESTORER_COPY(m_ds);
            ಯದಿ (nodep->name() != "") {
                m_ds.m_dotSymp = m_curSymp = m_statep->get_ಗಂಟು_Sym(nodep);
                UINFO(5, indent() << "cur=se" << cvtToHex(m_curSymp));
            }
            iterateChildren(nodep);
        }
        UINFO(5, indent() << "cur=se" << cvtToHex(m_curSymp));
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_FTask* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        ಯದಿ (nodep->isExternDef() && !nodep->didProto()) {
            nodep->didProto(ಸತ್ಯ);
            ಯದಿ (ಧ್ರುವ VSymEnt* ಧ್ರುವ foundp
                = m_curSymp->findIdFallback("extern " + nodep->name())) {
                ಧ್ರುವ Ast_ಗಂಟು_FTask* ಧ್ರುವ protop = VN_AS(foundp->nodep(), ಗಂಟು_FTask);
                // Copy specifiers.
                // External definition cannot have any specifiers, so no value will be overwritten.
                nodep->isHideLocal(protop->isHideLocal());
                nodep->isHideProtected(protop->isHideProtected());
                nodep->isStatic(protop->isStatic());
                nodep->isVirtual(protop->isVirtual());
                nodep->lifetime(protop->lifetime());
                // Always add an FUNC_RETURN_PROTO even if task, so we can locate the prototype
                nodep->addStmtsp(ನೂತನ AstAttrOf{protop->fileline(), VAttrType::FUNC_RETURN_PROTO,
                                               protop->fvarp()
                                                   ? protop->fvarp()->clone_ವೃಕ್ಷ(ಅಸತ್ಯ)
                                                   : ನೂತನ AstVoidDType{protop->fileline()}});
                ಪ್ರತಿ (Ast_ಗಂಟು* stmtp = protop->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    ಯದಿ (AstVar* ಧ್ರುವ portp = VN_CAST(stmtp, Var)) {
                        AstAttrOf* ಧ್ರುವ attrp
                            = ನೂತನ AstAttrOf{protop->fileline(), VAttrType::FUNC_ARG_PROTO,
                                            portp->subDTypep()->clone_ವೃಕ್ಷ(ಅಸತ್ಯ)};
                        attrp->name(portp->name());
                        nodep->addStmtsp(attrp);
                    }
                }
            } ಅನ್ಯಥಾ {
                nodep->v3error("extern not found that declares " + nodep->prettyNameQ());
            }
        }
        ಯದಿ (nodep->isExternProto()) {
            ಯದಿ (!m_curSymp->findIdFallback(nodep->name())) {
                nodep->v3warn(PROTOTYPEMIS,
                              "Definition not found for extern prototype " + nodep->prettyNameQ());
            }
        }
        VL_RESTORER(m_curSymp);
        VL_RESTORER(m_ftaskp);
        VL_RESTORER(m_explicitSuperNewp);
        {
            m_ftaskp = nodep;
            m_ds.m_dotSymp = m_curSymp = m_statep->get_ಗಂಟು_Sym(nodep);
            ಧ್ರುವ ತಾರ್ಕಿಕ isNew = nodep->name() == "new";
            ಯದಿ (isNew) m_explicitSuperNewp = nullptr;
            iterateChildren(nodep);
            ಯದಿ (isNew) {
                ಧ್ರುವ AstClassExtends* ಧ್ರುವ classExtendsp = VN_AS(m_modp, Class)->extendsp();
                ಯದಿ (m_explicitSuperNewp && !m_explicitSuperNewp->isImplicit() && classExtendsp
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
                ಯದಿ (classExtendsp && classExtendsp->classOrNullp()) {
                    ಯದಿ (!m_explicitSuperNewp && m_statep->forParamed()
                        && !VN_AS(classExtendsp->childDTypep()->skipRefp(), ClassRefDType)
                                ->classp()
                                ->isInterfaceClass()) {
                        Ast_ಗಂಟು_Stmt* ಧ್ರುವ superNewp
                            = addImplicitSuperNewCall(VN_AS(nodep, Func), classExtendsp);
                        UINFO(9, "created super new " << superNewp);
                        iterate(superNewp);
                    }
                }
            }
        }
        m_ds.m_dotSymp = VL_RESTORER_PREV(m_curSymp);
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Foreach* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        symIterateChildren(nodep, m_statep->get_ಗಂಟು_Sym(nodep));
    }
    ಶೂನ್ಯ visit(AstRandSequence* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        ಯದಿ (!nodep->start().empty())
            nodep->prodp(findProd(nodep, m_statep->get_ಗಂಟು_Sym(nodep), nodep->start()));
        symIterateChildren(nodep, m_statep->get_ಗಂಟು_Sym(nodep));
    }
    ಶೂನ್ಯ visit(AstRSProd* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        symIterateChildren(nodep, m_statep->get_ಗಂಟು_Sym(nodep));
    }
    ಶೂನ್ಯ visit(AstRSProdItem* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        nodep->prodp(findProd(nodep, m_curSymp, nodep->name()));
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstCoverWith* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        iterateAndNextNull(nodep->subp());
        // The type of 'item' is that of the coverpoint expression, which may name 'item' too
        iterateAndNextNull(nodep->itemp()->childDTypep());
        VL_RESTORER(m_curSymp);
        m_ds.m_dotSymp = m_curSymp = m_statep->get_ಗಂಟು_Sym(nodep);
        iterateAndNextNull(nodep->itemp());
        iterateAndNextNull(nodep->filterp());
        m_ds.m_dotSymp = VL_RESTORER_PREV(m_curSymp);
    }
    ಶೂನ್ಯ visit(AstCoverCrossWith* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        iterateAndNextNull(nodep->selectp());
        iterateAndNextNull(nodep->countp());
        // The type of a cross item is that of its coverpoint's expression, which may name items
        iterateAndNextNull(nodep->varsp());
        VL_RESTORER(m_curSymp);
        m_ds.m_dotSymp = m_curSymp = m_statep->get_ಗಂಟು_Sym(nodep);
        iterateAndNextNull(nodep->filterp());
        m_ds.m_dotSymp = VL_RESTORER_PREV(m_curSymp);
    }
    ಶೂನ್ಯ visit(AstWith* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        ಯದಿ (m_ds.m_unresolvedClass) ಪ್ರತ್ಯಾಗಮ;
        VL_RESTORER(m_curSymp);
        VL_RESTORER(m_currentWithp);
        VL_RESTORER_COPY(m_restrictedNamesUsed);
        {
            m_ds.m_dotSymp = m_curSymp = m_statep->get_ಗಂಟು_Sym(nodep);
            m_currentWithp = nodep;
            // Validate the identifier_list once per AstWith (m_validated travels
            // with cloneTree, so cloned witnesses skip the redundant check).
            ಧ್ರುವ ತಾರ್ಕಿಕ firstVisit = !nodep->validated();
            ಯದಿ (firstVisit) {
                nodep->validated(ಸತ್ಯ);
                m_restrictedNamesUsed.clear();
                // IEEE 1800-2023 18.7: each name in the identifier_list must designate
                // a member of the randomize() target class.
                ಯದಿ (nodep->restricted() && m_randSymp) {
                    ಪ್ರತಿ (ಧ್ರುವ std::string& n : nodep->restrictedNames()) {
                        ಯದಿ (!m_randSymp->findIdFlat(n)) {
                            nodep->v3error("Identifier '"
                                           << n
                                           << "' in 'with (...)' identifier list is not a"
                                              " member of the randomize() target class"
                                              " (IEEE 1800-2023 18.7)");
                            m_restrictedNamesUsed.ಸೇರಿಸು(n);
                        }
                    }
                }
            }
            iterateChildren(nodep);
            // Flag names that the user listed but never referenced in the constraint.
            ಯದಿ (firstVisit && nodep->restricted()) {
                ಪ್ರತಿ (ಧ್ರುವ std::string& n : nodep->restrictedNames()) {
                    ಯದಿ (!m_restrictedNamesUsed.ಎಣಿಕೆ(n)) {
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
    ಶೂನ್ಯ visit(AstLambdaArgRef* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        // No checknodot(nodep), visit(AstScope) will check for LambdaArgRef
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstClassExtends* nodep) override {
        // Resolve the symbol and get the class.
        // If it is a parameterized case, the class will be resolved after V3Param.cpp
        ಯದಿ (nodep->user3SetOnce()) ಪ್ರತ್ಯಾಗಮ;
        // If the class is resolved, there is nothing more to do
        ಯದಿ (nodep->classOrNullp()) ಪ್ರತ್ಯಾಗಮ;
        LINKDOT_VISIT_START();
        ಯದಿ (m_statep->forPrimary()) {
            ಯದಿ (nodep->childDTypep()) ಪ್ರತ್ಯಾಗಮ;
            Ast_ಗಂಟು* cprp = nodep->classOrPkgsp();
            VSymEnt* lookSymp = m_curSymp;
            ಯದಿ (AstDot* ಧ್ರುವ dotp = VN_CAST(cprp, Dot)) {
                dotp->user3(ಸತ್ಯ);
                ಯದಿ (AstClassOrPackageRef* lookNodep = VN_CAST(dotp->lhsp(), ClassOrPackageRef)) {
                    iterate(lookNodep);
                    cprp = dotp->rhsp();
                    VSymEnt* ಧ್ರುವ foundp = m_statep->resolveClassOrPackage(
                        lookSymp, lookNodep, ಸತ್ಯ, ಅಸತ್ಯ, nodep->verilogKwd());
                    ಯದಿ (!foundp) {
                        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlinkFrBack()), nodep);
                        ಪ್ರತ್ಯಾಗಮ;
                    }
                    UASSERT_OBJ(lookNodep->classOrPackageSkipp(), nodep, "Bad package link");
                    lookSymp = m_statep->get_ಗಂಟು_Sym(lookNodep->classOrPackageSkipp());
                } ಅನ್ಯಥಾ {
                    dotp->lhsp()->v3error("Attempting to extend"  // LCOV_EXCL_LINE
                                          " using non-class under dot");
                }
            }
            AstClassOrPackageRef* ಧ್ರುವ cpackagerefp = VN_CAST(cprp, ClassOrPackageRef);
            ಯದಿ (VL_UNCOVERABLE(!cpackagerefp)) {
                // Linking the extend gives an error before this is hit
                nodep->v3error("Attempting to extend using non-class");  // LCOV_EXCL_LINE
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlinkFrBack()), nodep);
                ಪ್ರತ್ಯಾಗಮ;
            }
            VSymEnt* ಧ್ರುವ foundp = m_statep->resolveClassOrPackage(lookSymp, cpackagerefp, ಸತ್ಯ,
                                                                    ಸತ್ಯ, nodep->verilogKwd());
            ಯದಿ (!foundp) {
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlinkFrBack()), nodep);
                ಪ್ರತ್ಯಾಗಮ;
            }
            ಯದಿ (AstClass* ಧ್ರುವ classp = VN_CAST(foundp->nodep(), Class)) {
                AstPin* paramsp = cpackagerefp->paramsp();
                ಯದಿ (paramsp) {
                    paramsp = paramsp->clone_ವೃಕ್ಷ(ಸತ್ಯ);
                    nodep->parameterized(ಸತ್ಯ);
                }
                nodep->childDTypep(ನೂತನ AstClassRefDType{nodep->fileline(), classp, paramsp});
                // Link pins
                iterate(nodep->childDTypep());
            } ಅನ್ಯಥಾ ಯದಿ (AstParamTypeDType* ಧ್ರುವ paramp
                       = VN_CAST(foundp->nodep(), ParamTypeDType)) {
                AstRefDType* ಧ್ರುವ refParamp = ನೂತನ AstRefDType{nodep->fileline(), paramp->name()};
                refParamp->refDTypep(paramp);
                nodep->childDTypep(refParamp);
                nodep->parameterized(ಸತ್ಯ);
            } ಅನ್ಯಥಾ ಯದಿ (AstTypedef* ಧ್ರುವ typedefp = VN_CAST(foundp->nodep(), Typedef)) {
                Ast_ಗಂಟು_DType* ಧ್ರುವ unwrappedp = typedefp->subDTypep()->skipRefp();
                ಯದಿ (AstClassRefDType* ಧ್ರುವ classRefp = VN_CAST(unwrappedp, ClassRefDType)) {
                    AstPin* paramsp = cpackagerefp->paramsp();
                    ಯದಿ (!paramsp && classRefp->paramsp()) {
                        // No explicit #(...) on extends clause; carry over the
                        // typedef's type-parameter pins so V3Param sees them.
                        paramsp = classRefp->paramsp()->clone_ವೃಕ್ಷ(ಸತ್ಯ);
                        nodep->parameterized(ಸತ್ಯ);
                    } ಅನ್ಯಥಾ ಯದಿ (paramsp) {
                        paramsp = paramsp->clone_ವೃಕ್ಷ(ಸತ್ಯ);
                        nodep->parameterized(ಸತ್ಯ);
                    }
                    nodep->childDTypep(
                        ನೂತನ AstClassRefDType{nodep->fileline(), classRefp->classp(), paramsp});
                    iterate(nodep->childDTypep());
                } ಅನ್ಯಥಾ {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: " << foundp->nodep()->prettyTypeName()
                                                  << " in 'class extends'");
                    ಪ್ರತ್ಯಾಗಮ;
                }
            } ಅನ್ಯಥಾ {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: " << foundp->nodep()->prettyTypeName()
                                                             << " in 'class extends'");
                ಪ್ರತ್ಯಾಗಮ;
            }
            ಯದಿ (!nodep->childDTypep()) nodep->v3error("Attempting to extend using non-class");
            nodep->classOrPkgsp()->unlinkFrBack()->delete_ವೃಕ್ಷ();
        } ಅನ್ಯಥಾ {
            // Probably a parameter
            ಯದಿ (AstRefDType* ಧ್ರುವ refp = VN_CAST(nodep->childDTypep(), RefDType)) {
                ಯದಿ (AstClassRefDType* classRefp = VN_CAST(refp->skipRefp(), ClassRefDType)) {
                    // Resolved to a class reference.
                    refp->replaceWith(classRefp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ));
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(refp), refp);
                } ಅನ್ಯಥಾ {
                    // Unable to resolve the ref type to a class reference.
                    // Get the value of type parameter passed to the class instance,
                    // to print the helpful error message.
                    ಧ್ರುವ Ast_ಗಂಟು_DType* typep = refp->refDTypep();
                    ಯಾವತ್ (ಸತ್ಯ) {
                        ಯದಿ (ಧ್ರುವ AstParamTypeDType* ಧ್ರುವ atypep
                            = VN_CAST(typep, ParamTypeDType)) {
                            typep = atypep->subDTypep();
                            ಅನುವರ್ತ;
                        }
                        ಯದಿ (ಧ್ರುವ AstRequireDType* ಧ್ರುವ atypep = VN_CAST(typep, RequireDType)) {
                            typep = atypep->subDTypep();
                            ಅನುವರ್ತ;
                        }
                        ಭಙ್ಗ;
                    }
                    typep->v3error("Attempting to extend using non-class");
                }
            }
        }
    }
    ಶೂನ್ಯ visit(AstClass* nodep) override {
        ಯದಿ (nodep->user3SetOnce()) ಪ್ರತ್ಯಾಗಮ;
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
            m_ds.m_dotSymp = m_curSymp = m_modSymp = m_statep->get_ಗಂಟು_Sym(nodep);
            m_modp = nodep;
            ಪೂರ್ಣಾಙ್ಕ next = 0;
            ಪ್ರತಿ (AstClassExtends* cextp = nodep->extendsp(); cextp;
                 cextp = VN_AS(cextp->nextp(), ClassExtends)) {
                // Replace abstract reference with hard pointer
                // Will need later resolution when deal with parameters
                ಯದಿ (++next == 2 && !nodep->isInterfaceClass() && !cextp->isImplements()) {
                    cextp->v3error("Multiple inheritance illegal on non-interface classes"
                                   " (IEEE 1800-2023 8.13)");
                }
                iterate(cextp);
                ಯದಿ (m_statep->forPrimary()) {
                    ಯದಿ (cextp->parameterized()) {
                        // Parameters in extends statement.
                        // The class can't be resolved in the current pass.
                        m_extendsParam.ಸೇರಿಸು(nodep);
                        m_insideClassExtParam = ಸತ್ಯ;
                    }
                    ಯದಿ (AstClassRefDType* ಧ್ರುವ classRefp
                        = VN_CAST(cextp->childDTypep(), ClassRefDType)) {
                        AstClass* ಧ್ರುವ classp = classRefp->classp();
                        ಯದಿ (classp != nodep) iterate(classp);
                        ಯದಿ (m_extendsParam.find(classp) != m_extendsParam.end()) {
                            // One of its super classes has parameters in extends statement.
                            // Some links may not be resolved in the first pass.
                            m_extendsParam.ಸೇರಿಸು(nodep);
                            m_insideClassExtParam = ಸತ್ಯ;
                        }
                    }
                }

                ಯದಿ (AstClass* ಧ್ರುವ baseClassp = cextp->classOrNullp()) {
                    // Already converted. Update symbol table to link unlinked members.
                    // Base class has to be visited in a case if its extends statement
                    // needs to be handled. Recursive inheritance was already checked.
                    // Must be here instead of in LinkDotParam to handle
                    // "class (type T) extends T".
                    ಯದಿ (baseClassp == nodep) {
                        cextp->v3error("Attempting to extend class " << nodep->prettyNameQ()
                                                                     << " from itself");
                    } ಅನ್ಯಥಾ ಯದಿ (extendsClass(baseClassp, nodep)) {
                        cextp->v3error("Attempting to extend class "
                                       << nodep->prettyNameQ() << " from "
                                       << baseClassp->prettyNameQ()
                                       << ", which circularly inherits from it");
                    } ಅನ್ಯಥಾ ಯದಿ (cextp->isImplements() && !baseClassp->isInterfaceClass()) {
                        cextp->v3error("Attempting to implement from non-interface class "
                                       << baseClassp->prettyNameQ() << '\n'
                                       << "... Suggest use 'extends'");
                    } ಅನ್ಯಥಾ ಯದಿ (!cextp->isImplements() && !nodep->isInterfaceClass()
                               && baseClassp->isInterfaceClass()) {
                        cextp->v3error("Attempting to extend from interface class "
                                       << baseClassp->prettyNameQ() << '\n'
                                       << "... Suggest use 'implements'");
                    }
                    baseClassp->isExtended(ಸತ್ಯ);
                    nodep->isExtended(ಸತ್ಯ);
                    iterate(baseClassp);
                    importSymbolsFromExtended(nodep, cextp);
                    ಅನುವರ್ತ;
                }
            }
            m_ds.m_dotSymp = m_curSymp;

            iterateChildren(nodep);
            ಪ್ರತಿ (ಧ್ರುವ auto& itr : m_ifForwards) {
                ಧ್ರುವ VSymEnt* ಧ್ರುವ implSymp = itr.second;
                addInterfaceForward(nodep, VN_AS(implSymp->nodep(), ಗಂಟು_FTask),
                                    VN_AS(implSymp->parentp()->nodep(), Class));
            }
        }
        // V3Width when determines types needs to find enum values and such
        // so add members pointing to appropriate enum values
        {
            VMemberMap memberMap;
            ಪ್ರತಿ (VSymEnt::const_iterator it = m_curSymp->begin(); it != m_curSymp->end(); ++it) {
                Ast_ಗಂಟು* ಧ್ರುವ itemp = it->second->nodep();
                ಯದಿ (!memberMap.findMember(nodep, it->first)) {
                    ಯದಿ (AstEnumItem* ಧ್ರುವ aitemp = VN_CAST(itemp, EnumItem)) {
                        AstEnumItemRef* ಧ್ರುವ newp = ನೂತನ AstEnumItemRef{
                            aitemp->fileline(), aitemp, it->second->classOrPackagep()};
                        UINFO(8, indent()
                                     << "Class import noderef '" << it->first << "' " << newp);
                        nodep->addMembersp(newp);
                        memberMap.ಸೇರಿಸು(nodep, newp);
                    }
                }
            }
        }
        m_ds.m_dotSymp = VL_RESTORER_PREV(m_curSymp);
    }

    // Collect the ClassOrPackageRefs in a chained scope operand in source order.
    // The grammar builds a scope operand only from packageClassScopeItem, which
    // always yields a ClassOrPackageRef, nested in Dots, so nothing else appears.
    ಸ್ಥಿರ ಶೂನ್ಯ collectScopeRefs(Ast_ಗಂಟು* nodep, std::vector<AstClassOrPackageRef*>& refps) {
        ಯದಿ (AstClassOrPackageRef* ಧ್ರುವ refp = VN_CAST(nodep, ClassOrPackageRef)) {
            refps.ತಳ್ಳು_back(refp);
            ಪ್ರತ್ಯಾಗಮ;
        }
        AstDot* ಧ್ರುವ dotp = VN_CAST(nodep, Dot);
        UASSERT_OBJ(dotp, nodep, "Non-scope node in package/class scope operand");
        collectScopeRefs(dotp->lhsp(), refps);
        collectScopeRefs(dotp->rhsp(), refps);
    }

    // Resolve a chained class-scope operand (`pkg::outer::inner` in
    // `pkg::outer::inner::t`) and reduce it to its innermost ClassOrPackageRef.
    // Returns false, having reported, if a segment could not be resolved.
    ತಾರ್ಕಿಕ reduceScopeDot(AstRefDType* nodep, AstDot* scopeDotp) {
        std::vector<AstClassOrPackageRef*> refps;
        collectScopeRefs(scopeDotp, refps);

        VSymEnt* scopeSymp = m_ds.m_dotSymp;
        ಪ್ರತಿ (size_t i = 0; i < refps.size(); ++i) {
            AstClassOrPackageRef* ಧ್ರುವ refp = refps[i];
            ಯದಿ (!refp->classOrPackageSkipp() && !refp->classOrPackageNodep()
                && !m_statep->resolveClassOrPackage(scopeSymp, refp, i == 0, ಅಸತ್ಯ,
                                                    "class/package reference")) {
                ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;  // Error already reported
            }
            ಯದಿ (i + 1 == refps.size()) ಭಙ್ಗ;
            // A middle segment that resolved to a node with no reachable module
            // (e.g. a typedef alias of a not-yet-specialized parameterized class)
            // gives nothing to look the next segment up in.
            Ast_ಗಂಟು_Module* ಧ್ರುವ modp = refp->classOrPackageSkipp();
            ಯದಿ (!modp) {
                refp->v3warn(E_UNSUPPORTED, "Unsupported: Multiple '::' package/class reference");
                ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            }
            scopeSymp = m_statep->get_ಗಂಟು_Sym(modp);
        }
        AstClassOrPackageRef* ಧ್ರುವ innerp = refps.back();
        innerp->unlinkFrBack();
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(scopeDotp->unlinkFrBack()), scopeDotp);
        nodep->classOrPackageOpp(innerp);
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }

    ಶೂನ್ಯ visit(AstRefDType* nodep) override {

        ಯದಿ (auto* ಧ್ರುವ typeOfp = nodep->typeofp()) {
            iterate(typeOfp);
            // After iteration, typeofp() may have been replaced with a resolved type
            // (e.g., DOT expression like if0.rq_t resolved to REFDTYPE)
            // If it's now a NodeDType, replace this RefDType with it
            ಯದಿ (Ast_ಗಂಟು_DType* ಧ್ರುವ resolvedDTypep = VN_CAST(nodep->typeofp(), ಗಂಟು_DType)) {
                UINFO(9,
                      indent() << "iface capture typeofp resolved to dtype, replacing RefDType\n");
                resolvedDTypep->unlinkFrBack();
                nodep->replaceWith(resolvedDTypep);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                // Originally (PR #6637) this block re-captured a resolved
                // RefDType into the iface-capture ledger when user2p held a
                // Cell, to ensure re-resolution during the paramed pass.
                // In practice, the typeofp-resolved RefDType never carries a
                // Cell in user2p: the capture happens earlier when the
                // typedef is first linked.
                ಯದಿ (V3LinkDotIfaceCapture::enabled()) {
                    ಯದಿ (AstRefDType* ಧ್ರುವ resolvedRefp = VN_CAST(resolvedDTypep, RefDType)) {
                        ಯದಿ (VL_UNCOVERABLE(
                                VN_IS(resolvedRefp->user2p(), Cell))) {  // LCOV_EXCL_LINE
                            resolvedRefp->v3fatalSrc(  // LCOV_EXCL_LINE
                                "typeofp resolved RefDType has Cell in user2p;"
                                " expected to be captured already");
                        }
                    }
                }
            }
            ಪ್ರತ್ಯಾಗಮ;
        }

        // Resolve its reference
        ಯದಿ (m_statep->forParamed() && nodep->user3()) {
            ಯದಿ (V3LinkDotIfaceCapture::enabled() && nodep->user2p()) {
                UINFO(9, indent() << "iface capture clear user3 for captured typedef name="
                                  << nodep->name() << " cell=" << nodep->user2p());
            }
            nodep->user3(ಅಸತ್ಯ);
        }
        ಯದಿ (nodep->user3SetOnce()) {
            ಯದಿ (V3LinkDotIfaceCapture::enabled() && nodep->user2p()) {
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
            ಯದಿ (VL_UNCOVERABLE(m_statep->forParamed() && nodep->classOrPackageOpp())) {
                nodep->v3fatalSrc(  // LCOV_EXCL_LINE
                    "RefDType user3 revisit with classOrPackageOpp in paramed pass");
            }
            ಪ್ರತ್ಯಾಗಮ;
        }
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        ಯದಿ (nodep->classOrPackageOpp()) {
            // Resolve a chained scope (`pkg::cls::t`, `pkg::cls#(P)::t`) from outside in.
            // On failure reduceScopeDot has reported, so stop before using the operand.
            ಯದಿ (AstDot* ಧ್ರುವ scopeDotp = VN_CAST(nodep->classOrPackageOpp(), Dot)) {
                ಯದಿ (!reduceScopeDot(nodep, scopeDotp)) ಪ್ರತ್ಯಾಗಮ;
            }
            // Re-read: reduceScopeDot may have replaced the operand
            Ast_ಗಂಟು* ಧ್ರುವ cpackagep = nodep->classOrPackageOpp();
            UASSERT_OBJ(!VN_IS(cpackagep, Dot), cpackagep,
                        "Package/class scope operand should be reduced by now");
            ಯದಿ (AstClassOrPackageRef* ಧ್ರುವ cpackagerefp = VN_CAST(cpackagep, ClassOrPackageRef)) {
                iterate(cpackagerefp);
                ಧ್ರುವ AstClass* ಧ್ರುವ clsp = VN_CAST(cpackagerefp->classOrPackageNodep(), Class);
                ಯದಿ (clsp && clsp->hasGParam()) {
                    // Unable to link before the instantiation of parameter classes.
                    // The class reference node still has to be visited now to later link
                    // parameters.
                    iterate(cpackagep);
                    ಪ್ರತ್ಯಾಗಮ;
                }
                // Defer non-typedef references through typedef aliases of parameterized classes.
                ಯದಿ (m_statep->forPrimary() && !VN_IS(nodep->backp(), Typedef)
                    && isParamedClassRef(cpackagerefp)) {
                    iterate(cpackagep);
                    ಪ್ರತ್ಯಾಗಮ;
                }

                ಧ್ರುವ ತಾರ್ಕಿಕ doDefaultTypedef = !(m_resolvingTypedef && m_statep->forPrimary());
                ಯದಿ (!cpackagerefp->classOrPackageSkipp(doDefaultTypedef)
                    && !cpackagerefp->classOrPackageNodep()) {
                    VSymEnt* ಧ್ರುವ foundp = m_statep->resolveClassOrPackage(
                        m_ds.m_dotSymp, cpackagerefp, ಸತ್ಯ, ಅಸತ್ಯ, "class/package reference");
                    ಯದಿ (!foundp) ಪ್ರತ್ಯಾಗಮ;
                }
                nodep->classOrPackagep(cpackagerefp->classOrPackageSkipp(doDefaultTypedef));
                ಯದಿ (!VN_IS(nodep->classOrPackagep(), Class)
                    && !VN_IS(nodep->classOrPackagep(), Package)) {
                    ಯದಿ (m_statep->forPrimary()) {
                        // It may be a type that comes from parameter class that is not
                        // instantioned yet
                        iterate(cpackagep);
                        ಪ್ರತ್ಯಾಗಮ;
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
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(cpackagep->unlinkFrBack()), cpackagep);
        }

        ಯದಿ (m_ds.m_dotp && (m_ds.m_dotPos == DP_PACKAGE || m_ds.m_dotPos == DP_SCOPE)) {
            UASSERT_OBJ(VN_IS(m_ds.m_dotp->lhsp(), ClassOrPackageRef), m_ds.m_dotp->lhsp(),
                        "Bad package link");
            auto* ಧ್ರುವ cpackagerefp = VN_CAST(m_ds.m_dotp->lhsp(), ClassOrPackageRef);
            UASSERT_OBJ(cpackagerefp->classOrPackageSkipp(), m_ds.m_dotp->lhsp(),
                        "Bad package link");
            nodep->classOrPackagep(cpackagerefp->classOrPackageSkipp());
            m_ds.m_dotPos = DP_SCOPE;
        } ಅನ್ಯಥಾ {
            // Allow REFDTYPE under DOT when referencing interface typedefs
            // This is needed for patterns like: typedef iface.a_t a_t;
            // The dependency graph handles resolution when IfaceCapture is disabled
            ಧ್ರುವ ತಾರ್ಕಿಕ ifaceFinalSegmentAllowed
                = (m_ds.m_dotPos == DP_FINAL) && m_ds.m_dotSymp
                  && VN_IS(m_ds.m_dotSymp->nodep(), Cell)
                  && VN_CAST(m_ds.m_dotSymp->nodep(), Cell)->modp()
                  && VN_IS(VN_CAST(m_ds.m_dotSymp->nodep(), Cell)->modp(), Iface);
            ಯದಿ (!ifaceFinalSegmentAllowed) {
                checkNoDot(nodep);
            } ಅನ್ಯಥಾ {
                UINFO(9, indent() << "allow REFDTYPE under DOT for iface typedef name="
                                  << nodep->prettyNameQ() << " dotSym=" << m_ds.m_dotSymp);
                // Clear the dot state so we don't propagate errors
                m_ds.m_dotPos = DP_SCOPE;
            }
        }
        ಯದಿ (!nodep->typedefp() && !nodep->subDTypep()) {
            ಧ್ರುವ VSymEnt* foundp;
            ಯದಿ (nodep->classOrPackagep()) {
                foundp = m_statep->get_ಗಂಟು_Sym(nodep->classOrPackagep())->findIdFlat(nodep->name());
            } ಅನ್ಯಥಾ ಯದಿ (m_ds.m_dotPos == DP_FIRST || m_ds.m_dotPos == DP_NONE) {
                foundp = findIdFallbackSkipMemberDType(m_curSymp, nodep->name());
            } ಅನ್ಯಥಾ {
                // Defensive: dotPos should be DP_FIRST/DP_NONE or classOrPackagep set.
                v3fatalSrc("Unexpected dotPos="
                           << static_cast<ಪೂರ್ಣಾಙ್ಕ>(m_ds.m_dotPos)  // LCOV_EXCL_LINE
                           << " in RefDType lookup for "
                           << nodep->prettyNameQ());  // LCOV_EXCL_LINE
                foundp = nullptr;  // LCOV_EXCL_LINE
            }
            {
                ಯದಿ (AstTypedef* ಧ್ರುವ defp
                    = foundp ? VN_CAST(foundp->nodep(), Typedef) : nullptr) {
                    // Don't check if typedef is to a <type T>::<reference> as might not be
                    // resolved yet
                    ಯದಿ (!nodep->classOrPackagep() && !defp->isUnderClass())
                        checkDeclOrder(nodep, defp);
                    nodep->typedefp(defp);
                    nodep->classOrPackagep(foundp->classOrPackagep());

                } ಅನ್ಯಥಾ ಯದಿ (AstParamTypeDType* ಧ್ರುವ defp
                           = foundp ? VN_CAST(foundp->nodep(), ParamTypeDType) : nullptr) {
                    // A PARAMTYPEDTYPE's child REFDTYPE referencing itself is the normal
                    // AST structure for type parameters (e.g., "type T = base" has a child
                    // REFDTYPE for the default type). Only error on true recursion where
                    // a TYPEDEF contains a reference back to itself.
                    ಧ್ರುವ ತಾರ್ಕಿಕ isParamTypeChild = (defp == nodep->backp());
                    ಧ್ರುವ ತಾರ್ಕಿಕ isTypedefRecursion
                        = isParamTypeChild && VN_IS(defp->backp(), Typedef);
                    ಯದಿ (isTypedefRecursion) {
                        nodep->v3error("Reference to '"
                                       << m_ds.m_dotText << (m_ds.m_dotText == "" ? "" : ".")
                                       << nodep->prettyName() << "'"
                                       << " type would form a recursive definition");
                        nodep->refDTypep(nodep->findVoidDType());  // Try to reduce later errors
                    } ಅನ್ಯಥಾ {
                        nodep->refDTypep(defp);
                        nodep->classOrPackagep(foundp->classOrPackagep());
                        captureIfaceParamType(nodep, defp);
                    }
                } ಅನ್ಯಥಾ ಯದಿ (AstClass* ಧ್ರುವ defp
                           = foundp ? VN_CAST(foundp->nodep(), Class) : nullptr) {
                    // Don't check if typedef is to a <type T>::<reference> as might not be
                    // resolved yet
                    ಯದಿ (!nodep->classOrPackagep()) checkDeclOrder(nodep, defp);
                    AstPin* ಧ್ರುವ paramsp = nodep->paramsp();
                    ಯದಿ (paramsp) paramsp->unlinkFrBackWithNext();
                    AstClassRefDType* ಧ್ರುವ newp
                        = ನೂತನ AstClassRefDType{nodep->fileline(), defp, paramsp};
                    newp->classOrPackagep(foundp->classOrPackagep());
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                    ಪ್ರತ್ಯಾಗಮ;
                } ಅನ್ಯಥಾ ಯದಿ (m_insideClassExtParam) {
                    ಪ್ರತ್ಯಾಗಮ;
                } ಅನ್ಯಥಾ {
                    ಯದಿ (foundp) {
                        UINFO(1, "Found sym node: " << foundp->nodep());
                        nodep->v3error("Expecting a data type: "
                                       << nodep->prettyNameQ()
                                       << ", found: " << foundp->nodep()->prettyTypeName());
                    } ಅನ್ಯಥಾ {
                        nodep->v3error("Can't find typedef/interface: " << nodep->prettyNameQ());
                    }
                }
            }
        }
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstRequireDType* nodep) override {
        // Handle type validation for localparam type assignments
        // Iterate child to resolve any ParseRef nodes
        iterateChildren(nodep);
        // Only emit error if the child is not a type.
        // Do NOT unwrap valid types here - leave that to V3Width.
        // Unwrapping here breaks type parameter resolution during cloning.
        // Allow Dot through to defer the handling until the dot expression is resolved.
        ಯದಿ (nodep->lhsp() && !VN_IS(nodep->lhsp(), ಗಂಟು_DType) && !VN_IS(nodep->lhsp(), Dot)) {
            // Not a type - emit error
            ಯದಿ (VN_IS(nodep->lhsp(), Const)) {
                nodep->lhsp()->v3error("Expecting a data type, not a constant");
            } ಅನ್ಯಥಾ {
                nodep->lhsp()->v3error("Expecting a data type, not "
                                       << nodep->lhsp()->typeName() << ": '"
                                       << nodep->lhsp()->prettyName() << "'");
            }
            // Replace with void to keep AST valid and allow clean exit
            nodep->replaceWith(ನೂತನ AstVoidDType{nodep->fileline()});
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        }
    }
    ಶೂನ್ಯ visit(AstDpiExport* nodep) override {
        // AstDpiExport: Make sure the function referenced exists, then dump it
        LINKDOT_VISIT_START();
        iterateChildren(nodep);
        checkNoDot(nodep);
        VSymEnt* ಧ್ರುವ foundp = m_curSymp->findIdFallback(nodep->name());
        Ast_ಗಂಟು_FTask* ಧ್ರುವ taskp = foundp ? VN_AS(foundp->nodep(), ಗಂಟು_FTask) : nullptr;
        ಯದಿ (!taskp) {
            nodep->v3error(
                "Can't find definition of exported task/function: " << nodep->prettyNameQ());
        } ಅನ್ಯಥಾ ಯದಿ (taskp->dpiExport()) {
            nodep->v3error("Function was already DPI Exported, duplicate not allowed: "
                           << nodep->prettyNameQ());
        } ಅನ್ಯಥಾ {
            taskp->dpiExport(ಸತ್ಯ);
            ಯದಿ (nodep->cname() != "") taskp->cname(nodep->cname());
        }
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlinkFrBack()), nodep);
    }
    ಶೂನ್ಯ visit(AstDisable* nodep) override {
        LINKDOT_VISIT_START();
        checkNoDot(nodep);
        ತಾರ್ಕಿಕ hasPartSelect = ಅಸತ್ಯ;
        ಧ್ರುವ string targetPath
            = nodep->targetRefp() ? extractDottedPath(nodep->targetRefp(), hasPartSelect) : "";
        VL_RESTORER_COPY(m_ds);
        m_ds.init(m_curSymp);
        m_ds.m_dotPos = DP_FIRST;
        m_ds.m_disablep = nodep;
        iterateChildren(nodep);
        ಯದಿ (nodep->targetRefp()) {
            ಯದಿ (AstTaskRef* ಧ್ರುವ taskRefp = VN_CAST(nodep->targetRefp(), TaskRef)) {
                nodep->targetp(taskRefp->taskp());
            } ಅನ್ಯಥಾ ಯದಿ (!VN_IS(nodep->targetRefp(), ParseRef)) {
                // If it is a ParseRef, either it couldn't be linked or it is linked to a block
                nodep->v3warn(E_UNSUPPORTED, "Node of type "
                                                 << nodep->targetRefp()->prettyTypeName()
                                                 << " referenced by disable");
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlinkFrBack()), nodep);
                ಪ್ರತ್ಯಾಗಮ;
            }
            ಯದಿ (nodep->targetp()) {
                nodep->targetRefp()->unlinkFrBack()->delete_ವೃಕ್ಷ();
                ಯದಿ (!hasPartSelect) {
                    ಧ್ರುವ string instancePath = targetInstancePath(nodep->targetp(), targetPath);
                    ಯದಿ (!instancePath.empty()) {
                        // Keep only the instance prefix so V3LinkJump can reference the selected
                        // instance's process queue without reparsing the disable target.
                        nodep->targetRefp(
                            ನೂತನ AstVarXRef{nodep->fileline(), "", instancePath, VAccess::READ});
                    }
                }
            }
        }
    }
    ಶೂನ್ಯ visitPackageImportOrExport(Ast_ಗಂಟು* nodep) {
        // No longer needed
        LINKDOT_VISIT_START();
        checkNoDot(nodep);
        ಯದಿ (m_statep->forParamed()) VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlinkFrBack()), nodep);
    }
    ಶೂನ್ಯ visit(AstPackageImport* nodep) override { visitPackageImportOrExport(nodep); }
    ಶೂನ್ಯ visit(AstPackageExport* nodep) override { visitPackageImportOrExport(nodep); }
    ಶೂನ್ಯ visit(AstPackageExportStarStar* nodep) override { visitPackageImportOrExport(nodep); }
    ಶೂನ್ಯ visit(AstCellRef* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstCellArrayRef* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        // Expression already iterated
    }
    ಶೂನ್ಯ visit(AstUnlinkedRef* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        // No need to iterate, if we have a UnlinkedVarXRef, we're already done
    }
    ಶೂನ್ಯ visit(Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ* nodep) override {
        LINKDOT_VISIT_START();
        checkNoDot(nodep);
        // Check if nodep represents a super.new call;
        ಯದಿ (AstNew* ಧ್ರುವ newExprp = VN_CAST(nodep->exprp(), New)) {
            // in this case it was already linked, so it doesn't have a super reference
            m_explicitSuperNewp = newExprp;
        } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstDot* ಧ್ರುವ dotp = VN_CAST(nodep->exprp(), Dot)) {
            ಯದಿ (dotp->lhsp()->name() == "super") {
                ಯದಿ (AstNew* ಧ್ರುವ newExprp = VN_CAST(dotp->rhsp(), New)) {
                    m_explicitSuperNewp = newExprp;
                }
            }
        }
        iterateChildren(nodep);
    }

    ಶೂನ್ಯ visit(AstIfaceRefDType* nodep) override {
        ಯದಿ (nodep->user3SetOnce()) ಪ್ರತ್ಯಾಗಮ;
        LINKDOT_VISIT_START();
        ಯದಿ (nodep->paramsp()) {
            // If there is no parameters, there is no need to visit this node.
            AstIface* ಧ್ರುವ ifacep = nodep->ifacep();
            UASSERT_OBJ(ifacep, nodep, "Port parameters of AstIfaceRefDType without ifacep()");
            ಯದಿ (ifacep->dead()) ಪ್ರತ್ಯಾಗಮ;
            checkNoDot(nodep);
            VL_RESTORER_CLEAR(m_usedPins);
            VL_RESTORER_CLEAR(m_usedDefParamPins);
            VL_RESTORER(m_pinSymp);
            m_pinSymp = m_statep->get_ಗಂಟು_Sym(ifacep);
            iterateAndNextNull(nodep->paramsp());
        }
    }

    ಶೂನ್ಯ visit(AstAttrOf* nodep) override { iterateChildren(nodep); }
    ಶೂನ್ಯ visit(AstCastParse* nodep) override {
        ಯದಿ (nodep->user3SetOnce()) ಪ್ರತ್ಯಾಗಮ;
        LINKDOT_VISIT_START();
        // A cast may start a dotted expression: HERE type'(expr) . member_or_method
        ಧ್ರುವ ತಾರ್ಕಿಕ dotLhs = m_ds.m_dotPos == DP_FIRST;
        ಯದಿ (!dotLhs) checkNoDot(nodep);
        {
            VL_RESTORER_COPY(m_ds);
            m_ds.init(m_curSymp);
            iterateChildren(nodep);
        }
        ಯದಿ (dotLhs) m_ds.m_dotPos = DP_MEMBER;
    }

    ಶೂನ್ಯ visit(AstAssignW* nodep) override {
        LINKDOT_VISIT_START();
        checkNoDot(nodep);
        VL_RESTORER(m_replaceWithAlias);
        ಯದಿ (nodep->user2()) m_replaceWithAlias = ಅಸತ್ಯ;
        iterateChildren(nodep);
    }

    ಶೂನ್ಯ visit(AstTypedef* nodep) override {
        VL_RESTORER(m_resolvingTypedef)
        m_resolvingTypedef = ಸತ್ಯ;
        iterateChildren(nodep);
    }

    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override {
        VL_RESTORER(m_inPackedArray);
        ಯದಿ (VN_IS(nodep, PackArrayDType)) {
            m_inPackedArray = ಸತ್ಯ;
        } ಅನ್ಯಥಾ ಯದಿ (!m_inPackedArray) {
            LINKDOT_VISIT_START();
            checkNoDot(nodep);
        }
        iterateChildren(nodep);
    }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    LinkDotResolveVisitor(AstNetlist* rootp, LinkDotState* statep)
        : m_statep{statep} {
        UINFO(4, __FUNCTION__ << ": ");

        // Note: no need to clear user3 on ledger entries here.
        // VNUser3InUse (m_inuser3) already logically clears user3 on all
        // nodes via the generation counter.  The ledger may also hold stale
        // refp pointers to deleted AST nodes, so iterating it is unsafe.

        iterate(rootp);
        ಧ್ರುವ std::map<std::string, Ast_ಗಂಟು_Module*> modulesToRevisit
            = std::move(m_modulesToRevisit);
        m_lastDeferredp = nullptr;
        ಪ್ರತಿ (auto& p : modulesToRevisit) {
            Ast_ಗಂಟು_Module* ಧ್ರುವ modp = p.second;
            modp->foreach([](Ast_ಗಂಟು* ಧ್ರುವ nodep) { nodep->user3(ಅಸತ್ಯ); });
            iterate(modp);
        }
    }
    ~LinkDotResolveVisitor() override = ಸ್ವಭಾವ;
};

//######################################################################
// Link class functions

ಶೂನ್ಯ V3LinkDot::dumpSubstep(ಧ್ರುವ string& name) {
    ಯದಿ (dump_ವೃಕ್ಷ_Either_Level() >= 9) {
        V3Global::dump_Check_Global_ವೃಕ್ಷ(name);
    } ಅನ್ಯಥಾ ಯದಿ (debug() >= 5) {  // on high dbg level, dump even if not explicitly told to
        v3Global.rootp()->dump_ವೃಕ್ಷ_File(v3Global.debugFilename(name + ".tree"));
    }
}

ಶೂನ್ಯ V3LinkDot::linkDotGuts(AstNetlist* rootp, VLinkDotStep step) {
    VIsCached::clear_Cache_ವೃಕ್ಷ();  // Avoid using any stale isPure
    dumpSubstep("prelinkdot");
    LinkDotState state{rootp, step};

    { LinkDotFindVisitor{rootp, &state}; }
    dumpSubstep("prelinkdot-find");

    { LinkDotFindIfaceVisitor{rootp, &state}; }
    dumpSubstep("prelinkdot-findiface");

    ಯದಿ (step == LDS_SCOPED) {
        // Well after the initial link when we're ready to operate on the flat design,
        // process AstScope's.  This needs to be separate pass after whole hierarchy graph created.
        { LinkDotScopeVisitor{rootp, &state}; }
        v3Global.assertScoped(ಸತ್ಯ);
        dumpSubstep("prelinkdot-scoped");
    } ಅನ್ಯಥಾ {
        // Initial link stage, resolve parameters and interfaces
        { LinkDotParamVisitor{rootp, &state}; }
        dumpSubstep("prelinkdot-param");
    }
    state.dumpSelf("prelinkdot");
    state.computeIfaceModSyms();
    state.computeIfaceVarSyms();
    state.computeScopeAliases();
    state.dumpSelf("linkdot-preresolve");
    ಧ್ರುವ LinkDotResolveVisitor visitor{rootp, &state};
    state.dumpSelf("linkdot-done");
}

ಶೂನ್ಯ V3LinkDot::linkDotPrimary(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    linkDotGuts(nodep, LDS_PRIMARY);
    V3Global::dump_Check_Global_ವೃಕ್ಷ("linkdot", 0, dump_ವೃಕ್ಷ_Either_Level() >= 6);
}

ಶೂನ್ಯ V3LinkDot::linkDotParamed(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    linkDotGuts(nodep, LDS_PARAMED);
    V3Global::dump_Check_Global_ವೃಕ್ಷ("linkdotparam", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}

ಶೂನ್ಯ V3LinkDot::linkDotScope(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    linkDotGuts(nodep, LDS_SCOPED);
    V3Global::dump_Check_Global_ವೃಕ್ಷ("linkdot", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}
