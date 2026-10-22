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

ಸ್ಥಿರ string ಹೊರತೆಗೆ_Dotted_ಮಾರ್ಗ(Ast_ಗಂಟು* nodep, ತಾರ್ಕಿಕ& hasPartSelect) {
    ಆದರೆ (AstParseRef* ಸ್ಥಿರಾಂಕ refp = VN_CAST(nodep, ParseRef)) {
        ಹಿಂತಿರುಗಿ refp->name();
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstVarRef* ಸ್ಥಿರಾಂಕ refp = VN_CAST(nodep, VarRef)) {
        ಹಿಂತಿರುಗಿ refp->name();
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstDot* ಸ್ಥಿರಾಂಕ dotp = VN_CAST(nodep, Dot)) {
        ಸ್ಥಿರಾಂಕ string lhs = ಹೊರತೆಗೆ_Dotted_ಮಾರ್ಗ(dotp->lhsp(), hasPartSelect);
        ಸ್ಥಿರಾಂಕ string rhs = ಹೊರತೆಗೆ_Dotted_ಮಾರ್ಗ(dotp->rhsp(), hasPartSelect);
        ಹಿಂತಿರುಗಿ VString::dot(lhs, ".", rhs);
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(nodep, Sel_ಬಿಟ್) || VN_IS(nodep, Sel_ಹೊರತೆಗೆ) || VN_IS(nodep, SelPlus)
               || VN_IS(nodep, SelMinus)) {
        hasPartSelect = ನಿಜ;
        ಹಿಂತಿರುಗಿ "";
    }
    ಹಿಂತಿರುಗಿ "";
}
ಸ್ಥಿರ string lexical_Disable_ಮಾರ್ಗ(Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ targetp) {
    std::ಸದಿಶ<string> ಹೆಸರುಗಳು;
    ಗಾಗಿ (Ast_ಗಂಟು* curp = targetp; curp; curp = curp->aboveLoopp()) {
        ಆದರೆ (Ast_ಗಂಟು_ಖಂಡ* ಸ್ಥಿರಾಂಕ blockp = VN_CAST(curp, ಗಂಟು_ಖಂಡ)) {
            ಆದರೆ (blockp->name() != "") ಹೆಸರುಗಳು.ತಳ್ಳು_ಹಿಂಭಾಗ(blockp->name());
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (Ast_ಗಂಟು_FTask* ಸ್ಥಿರಾಂಕ ftaskp = VN_CAST(curp, ಗಂಟು_FTask)) {
            ಹೆಸರುಗಳು.ತಳ್ಳು_ಹಿಂಭಾಗ(ftaskp->name());
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(curp, ಗಂಟು_Module)) {
            ನಿಲ್ಲಿಸು;
        }
    }
    string ಮಾರ್ಗ;
    ಗಾಗಿ (ಸ್ವಯಂಚಾಲಿತ it = ಹೆಸರುಗಳು.crbegin(); it != ಹೆಸರುಗಳು.crend(); ++it) {
        ಮಾರ್ಗ = VString::dot(ಮಾರ್ಗ, ".", *it);
    }
    ಹಿಂತಿರುಗಿ ಮಾರ್ಗ;
}
ಸ್ಥಿರ string ಗುರಿ_Instance_ಮಾರ್ಗ(Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ targetp, ಸ್ಥಿರಾಂಕ string& ಗುರಿ_ಮಾರ್ಗ) {
    ಸ್ಥಿರಾಂಕ string lexical_ಮಾರ್ಗ = lexical_Disable_ಮಾರ್ಗ(targetp);
    ಆದರೆ (lexical_ಮಾರ್ಗ == "" || ಗುರಿ_ಮಾರ್ಗ == lexical_ಮಾರ್ಗ) ಹಿಂತಿರುಗಿ "";
    ಸ್ಥಿರಾಂಕ string suffix = "." + lexical_ಮಾರ್ಗ;
    ಆದರೆ (!VString::ತುದಿಗಳು_With(ಗುರಿ_ಮಾರ್ಗ, suffix)) ಹಿಂತಿರುಗಿ "";
    ಹಿಂತಿರುಗಿ ಗುರಿ_ಮಾರ್ಗ.substr(0, ಗುರಿ_ಮಾರ್ಗ.ಗಾತ್ರ_2() - suffix.ಗಾತ್ರ_2());
}

// ######################################################################
//  Matcher classes (for suggestion matching)

ವರ್ಗ Link_ಗಂಟು_Matcher_Class_Or_Package final : ಸಾರ್ವಜನಿಕ VNodeMatcher {
ಸಾರ್ವಜನಿಕ:
    ತಾರ್ಕಿಕ ಗಂಟು_ಹೊಂದಿಕೆ(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep) ಸ್ಥಿರಾಂಕ override {
        ಹಿಂತಿರುಗಿ VN_IS(nodep, Class) || VN_IS(nodep, Package);
    }
};
ವರ್ಗ Link_ಗಂಟು_Matcher_FTask final : ಸಾರ್ವಜನಿಕ VNodeMatcher {
ಸಾರ್ವಜನಿಕ:
    ತಾರ್ಕಿಕ ಗಂಟು_ಹೊಂದಿಕೆ(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ VN_IS(nodep, ಗಂಟು_FTask); }
};
ವರ್ಗ Link_ಗಂಟು_Matcher_Modport final : ಸಾರ್ವಜನಿಕ VNodeMatcher {
ಸಾರ್ವಜನಿಕ:
    ತಾರ್ಕಿಕ ಗಂಟು_ಹೊಂದಿಕೆ(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ VN_IS(nodep, Modport); }
};
ವರ್ಗ Link_ಗಂಟು_Matcher_Prod final : ಸಾರ್ವಜನಿಕ VNodeMatcher {
ಸಾರ್ವಜನಿಕ:
    ತಾರ್ಕಿಕ ಗಂಟು_ಹೊಂದಿಕೆ(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep) ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ VN_IS(nodep, RSProd); }
};
ವರ್ಗ Link_ಗಂಟು_Matcher_Var final : ಸಾರ್ವಜನಿಕ VNodeMatcher {
ಸಾರ್ವಜನಿಕ:
    ತಾರ್ಕಿಕ ಗಂಟು_ಹೊಂದಿಕೆ(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep) ಸ್ಥಿರಾಂಕ override {
        ಹಿಂತಿರುಗಿ VN_IS(nodep, Var) || VN_IS(nodep, LambdaArgRef);
    }
};
ವರ್ಗ Link_ಗಂಟು_Matcher_Var_IO final : ಸಾರ್ವಜನಿಕ VNodeMatcher {
ಸಾರ್ವಜನಿಕ:
    ತಾರ್ಕಿಕ ಗಂಟು_ಹೊಂದಿಕೆ(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep) ಸ್ಥಿರಾಂಕ override {
        ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ varp = VN_CAST(nodep, Var);
        ಆದರೆ (!varp) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಹಿಂತಿರುಗಿ varp->isIO();
    }
};
ವರ್ಗ Link_ಗಂಟು_Matcher_Var_Param final : ಸಾರ್ವಜನಿಕ VNodeMatcher {
ಸಾರ್ವಜನಿಕ:
    ತಾರ್ಕಿಕ ಗಂಟು_ಹೊಂದಿಕೆ(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep) ಸ್ಥಿರಾಂಕ override {
        ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ varp = VN_CAST(nodep, Var);
        ಆದರೆ (!varp) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಹಿಂತಿರುಗಿ varp->isParam();
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
    ಸ್ಥಿರಾಂಕ VNUser1_ಒಳ_Use m_inuser1;
    ಸ್ಥಿರಾಂಕ VNUser2_ಒಳ_Use m_inuser2;
    ಸ್ಥಿರಾಂಕ VNUser4_ಒಳ_Use m_inuser4;

ಸಾರ್ವಜನಿಕ:
    // ENUMS
    // In order of priority, compute first ... compute last
    ಎಣಿಕೆ SAMNum : uint8_t { SAMN_MODPORT, SAMN_IFTOP, SAMN__MAX };  // Values for m_scopeAliasMap

ಖಾಸಗಿ:
    // TYPES
    ಬಳಸಿ ScopeAliasMap = std::unordered_map<VSymEnt*, VSymEnt*>;
    ಬಳಸಿ IfaceModSyms = std::ಸದಿಶ<std::pair<AstIface*, VSymEnt*>>;

    ಸ್ಥಿರ LinkDotState* s_errorThisp;  // Last self, for error reporting only

    // MEMBERS
    VSym_ನಕ್ಷೆ m_syms;  // Symbol table by hierarchy
    VSym_ನಕ್ಷೆ m_mods;  // Symbol table of all module names
    VSymEnt* m_dunitEntp = ಶೂನ್ಯಸೂಚಕ;  // $unit entry
    std::multimap<std::string, VSymEnt*>
        m_nameScopeSymMap;  // Map of scope referenced by non-pretty textual name
    std::ಗಣ<std::pair<Ast_ಗಂಟು_Module*, std::string>>
        m_implicit_Name_ಗಣ;  // For [module][signalname] if we can implicitly create it
    std::array<ScopeAliasMap, SAMN__MAX> m_scopeAliasMap;  // Map of <lhs,rhs> aliases
    std::ಸದಿಶ<VSymEnt*> m_ifaceVarSyms;  // List of AstIfaceRefDType's to be imported
    IfaceModSyms m_ifaceModSyms;  // List of AstIface+Symbols to be processed
    ಸ್ಥಿರಾಂಕ VLink_Dot_ಹೆಜ್ಜೆ m_ಹೆಜ್ಜೆ;  // Operational step

ಸಾರ್ವಜನಿಕ:
    // METHODS

    ಶೂನ್ಯ dumpSelf(ಸ್ಥಿರಾಂಕ string& nameComment, ತಾರ್ಕಿಕ force = ಸುಳ್ಳು) {
        ಆದರೆ (debug() >= 6 || dumpLevel() >= 6 || force) {
            ಸ್ಥಿರಾಂಕ string filename = v3Global.debugFilename(nameComment) + ".txt";
            UINFO(4, "Dumping " << filename);
            ಸ್ಥಿರಾಂಕ std::unique_ptr<std::ofstream> logp{V3File::new_ofstream(filename)};
            ಆದರೆ (logp->fail()) v3fatal("Can't write file: " << filename);
            std::ostream& os = *logp;
            // TODO the symbol table has node pointers which may be
            // dangling, as we call deleteTree in these visitors without
            // pushing for later deletion (or deleting from the symbol table)
            // So, only completely safe to call this when under --debug or --debug-leak.
            m_syms.dumpSelf(os);
            ತಾರ್ಕಿಕ ಮೊದಲ = ನಿಜ;
            ಗಾಗಿ (ಪೂರ್ಣಾಂಕ samn = 0; samn < SAMN__MAX; ++samn) {
                ಆದರೆ (!m_scopeAliasMap[samn].ಖಾಲಿಗಳು()) {
                    ಆದರೆ (ಮೊದಲ) os << "\nScopeAliasMap:\n";
                    ಮೊದಲ = ಸುಳ್ಳು;
                    ಗಾಗಿ (ScopeAliasMap::iterator it = m_scopeAliasMap[samn].begin();
                         it != m_scopeAliasMap[samn].ಕೊನೆ(); ++it) {
                        // left side is what we will import into
                        os << "\t" << samn << "\t" << it->ಮೊದಲ << " ("
                           << it->ಮೊದಲ->nodep()->typeName() << ") <- " << it->ಎರಡನೇ << " "
                           << it->ಎರಡನೇ->nodep() << '\n';
                    }
                }
            }
        }
    }
    ಸ್ಥಿರ ಶೂನ್ಯ preErrorDumpHandler() {
        ಆದರೆ (s_errorThisp) s_errorThisp->preErrorDump();
    }
    ಶೂನ್ಯ preErrorDump() {
        ಸ್ಥಿರ ತಾರ್ಕಿಕ s_diddump = ಸುಳ್ಳು;
        ಆದರೆ (!s_diddump && dump_ವೃಕ್ಷ_Level()) {
            s_diddump = ನಿಜ;
            dumpSelf("linkdot-preerr", ನಿಜ);
            v3Global.rootp()->dump_ವೃಕ್ಷ_File(v3Global.debugFilename("linkdot-preerr.tree"));
        }
    }

    // CONSTRUCTORS
    LinkDotState(AstNetlist* rootp, VLink_Dot_ಹೆಜ್ಜೆ ಹೆಜ್ಜೆ)
        : m_syms{rootp}
        , m_mods{rootp}
        , m_ಹೆಜ್ಜೆ{ಹೆಜ್ಜೆ} {
        UINFO(4, __FUNCTION__ << ": ");
        s_errorThisp = ಇದು;
        V3Error::errorExitCb(preErrorDumpHandler);  // If get error, dump self
        read_Mod_ಹೆಸರುಗಳು();
    }
    ~LinkDotState() {
        V3Error::errorExitCb(ಶೂನ್ಯಸೂಚಕ);
        s_errorThisp = ಶೂನ್ಯಸೂಚಕ;
    }

    // ACCESSORS
    VSym_ನಕ್ಷೆ* symsp() { ಹಿಂತಿರುಗಿ &m_syms; }
    ಪೂರ್ಣಾಂಕ ಹೆಜ್ಜೆ_ಸಂಖ್ಯೆ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ static_cast<ಪೂರ್ಣಾಂಕ>(m_ಹೆಜ್ಜೆ); }
    ತಾರ್ಕಿಕ forPrimary() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಹೆಜ್ಜೆ == LDS_PRIMARY; }
    ತಾರ್ಕಿಕ forParamed() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಹೆಜ್ಜೆ == LDS_PARAMED; }
    ತಾರ್ಕಿಕ forScopeCreation() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಹೆಜ್ಜೆ == LDS_SCOPED; }

    // METHODS
    ಸ್ಥಿರ string ಗಂಟು_ಪಠ್ಯ_Type(Ast_ಗಂಟು* nodep) {
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ varp = VN_CAST(nodep, Var)) {
            ಆದರೆ (varp->isIO() || varp->isIfaceRef()) {
                ಹಿಂತಿರುಗಿ "port";
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (varp->isGParam()) {
                ಹಿಂತಿರುಗಿ "parameter";
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (varp->varType() == VVarType::LPARAM) {
                ಹಿಂತಿರುಗಿ "local parameter";
            } ಇಲ್ಲದಿದ್ದರೆ {
                ಹಿಂತಿರುಗಿ "variable";
            }
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ AstParamTypeDType* ಸ್ಥಿರಾಂಕ dtypep = VN_CAST(nodep, ParamTypeDType)) {
            ಆದರೆ (dtypep->isGParam()) {
                ಹಿಂತಿರುಗಿ "type parameter";
            } ಇಲ್ಲದಿದ್ದರೆ {
                ಹಿಂತಿರುಗಿ "local type parameter";
            }
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(nodep, Begin)) {
            ಹಿಂತಿರುಗಿ "block";
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(nodep, Cell)) {
            ಹಿಂತಿರುಗಿ "instance";
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(nodep, Constraint)) {
            ಹಿಂತಿರುಗಿ "constraint";
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(nodep, Func)) {
            ಹಿಂತಿರುಗಿ "function";
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(nodep, Gen_ಖಂಡ)) {
            ಹಿಂತಿರುಗಿ "generate block";
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(nodep, Iface)) {
            ಹಿಂತಿರುಗಿ "interface";
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(nodep, RSProd)) {
            ಹಿಂತಿರುಗಿ "randsequence production";
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(nodep, Task)) {
            ಹಿಂತಿರುಗಿ "task";
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಹಿಂತಿರುಗಿ nodep->prettyTypeName();
        }
    }

    Ast_ಗಂಟು_Module* ಕಂಡುಹಿಡಿ_Module_Sym(ಸ್ಥಿರಾಂಕ string& modName) {
        ಸ್ಥಿರಾಂಕ VSymEnt* ಸ್ಥಿರಾಂಕ foundp = m_mods.rootp()->ಕಂಡುಹಿಡಿ_Id_Fallback(modName);
        ಹಿಂತಿರುಗಿ foundp ? VN_AS(foundp->nodep(), ಗಂಟು_Module) : ಶೂನ್ಯಸೂಚಕ;
    }
    ಶೂನ್ಯ read_Mod_ಹೆಸರುಗಳು() {
        // Look at all modules, and store pointers to all module names
        ಗಾಗಿ (Ast_ಗಂಟು_Module *nextp, *nodep = v3Global.rootp()->modulesp(); nodep; nodep = nextp) {
            nextp = VN_AS(nodep->nextp(), ಗಂಟು_Module);
            m_mods.rootp()->ಸೇರಿಸು(nodep->name(), ಹೊಸ VSymEnt{&m_mods, nodep});
        }
    }

    VSymEnt* ಬೇರು_Entp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_syms.rootp(); }
    VSymEnt* dunitEntp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_dunitEntp; }
    ಶೂನ್ಯ checkDuplicate(VSymEnt* lookupSymp, Ast_ಗಂಟು* nodep, ಸ್ಥಿರಾಂಕ string& name) {
        // Lookup the given name under current symbol table
        // Insert if not found
        // Report error if there's a duplicate
        //
        // Note we only check for conflicts at the same level; it's ok if one block hides another
        // We also wouldn't want to not insert it even though it's lower down

        ಸ್ಥಿರಾಂಕ VSymEnt* ಸ್ಥಿರಾಂಕ foundp = lookupSymp->ಕಂಡುಹಿಡಿ_Id_Flat(name);
        Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ fnodep = foundp ? foundp->nodep() : ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (!fnodep) {
            // Not found, will add in a moment.
            ಆದರೆ (forPrimary() && VSymEnt::checkSimilarname(nodep)) {
                ಸ್ಥಿರಾಂಕ VSymEnt* ಸ್ಥಿರಾಂಕ alt = lookupSymp->ಕಂಡುಹಿಡಿ_Similar_Id_Flat(name);
                ಆದರೆ (alt) {
                    nodep->v3warn(SIMILARNAME, "Declaration overlaps another with different case: "
                                                   << nodep->prettyNameQ() << '\n'
                                                   << nodep->warnContextPrimary() << '\n'
                                                   << alt->nodep()->warnOther()
                                                   << "... Location of original declaration\n"
                                                   << alt->nodep()->warnContextSecondary());
                }
            }
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep == fnodep) {  // Already inserted.
            // Good.
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (foundp->imported()) {  // From package
            // We don't throw VARHIDDEN as if the import is later the symbol
            // table's import wouldn't warn
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (forPrimary() && VN_IS(nodep, Gen_ಖಂಡ)
                   && (VN_IS(fnodep, Begin) || VN_IS(fnodep, Gen_ಖಂಡ))) {
            // Begin: ... blocks often replicate under genif/genfor, so
            // suppress duplicate checks.  See t_gen_forif.v for an example.
        } ಇಲ್ಲದಿದ್ದರೆ {
            UINFO(4, "name " << name);  // Not always same as nodep->name
            UINFO(4, "Var1 " << nodep);
            UINFO(4, "Var2 " << fnodep);
            // Quote the node's own name as for messages, if it is the symbol's
            ಸ್ಥಿರಾಂಕ string nameQ
                = fnodep->name() == name ? fnodep->prettyNameQ() : Ast_ಗಂಟು::prettyNameQ(name);
            ಆದರೆ (nodep->type() == fnodep->type()) {
                nodep->v3error("Duplicate declaration of "
                               << ಗಂಟು_ಪಠ್ಯ_Type(fnodep) << ": " << nameQ << '\n'
                               << nodep->warnContextPrimary() << '\n'
                               << fnodep->warnOther() << "... Location of original declaration\n"
                               << fnodep->warnContextSecondary());
                nodep->decl_Token_ಅಂಕೆ_ಗಣ_Min(0);  // Disable checking forward typedefs
                fnodep->decl_Token_ಅಂಕೆ_ಗಣ_Min(0);  // Disable checking forward typedefs
            } ಇಲ್ಲದಿದ್ದರೆ {
                nodep->v3error("Unsupported in C: "
                               << ucfirst(ಗಂಟು_ಪಠ್ಯ_Type(nodep)) << " has the same name as "
                               << ಗಂಟು_ಪಠ್ಯ_Type(fnodep) << ": " << nameQ << '\n'
                               << nodep->warnContextPrimary() << '\n'
                               << fnodep->warnOther() << "... Location of original declaration\n"
                               << fnodep->warnContextSecondary());
            }
        }
    }
    ಶೂನ್ಯ ಸೇರಿಸು_DUnit(AstNetlist* nodep) {
        // $unit on top scope
        VSymEnt* ಸ್ಥಿರಾಂಕ symp = ಹೊಸ VSymEnt{&m_syms, nodep};
        UINFO(9, "      INSERTdunit se" << cvt_ಗಮ್ಯ_Hex(symp));
        symp->parentp(ಬೇರು_Entp());  // Needed so backward search can find name of top module
        symp->fallbackp(ಶೂನ್ಯಸೂಚಕ);
        ಬೇರು_Entp()->ಸೇರಿಸು("$unit ", symp);  // Space so can never name conflict with user code
        //
        UASSERT_OBJ(!m_dunitEntp, nodep, "Call insertDUnit only once");
        m_dunitEntp = symp;
    }
    VSymEnt* ಸೇರಿಸು_ತುದಿ_Cell(Ast_ಗಂಟು_Module* nodep, ಸ್ಥಿರಾಂಕ string& scopename) {
        // Only called on the module at the very top of the hierarchy
        VSymEnt* ಸ್ಥಿರಾಂಕ symp = ಹೊಸ VSymEnt{&m_syms, nodep};
        UINFO(9, "      INSERTtop se" << cvt_ಗಮ್ಯ_Hex(symp) << "  " << scopename << " " << nodep);
        symp->parentp(ಬೇರು_Entp());  // Needed so backward search can find name of top module
        symp->fallbackp(dunitEntp());  // Needed so can find $unit stuff
        nodep->user1p(symp);
        checkDuplicate(ಬೇರು_Entp(), nodep, nodep->origName());
        ಬೇರು_Entp()->ಸೇರಿಸು(nodep->origName(), symp);
        ಆದರೆ (forScopeCreation()) m_nameScopeSymMap.emplace(scopename, symp);
        ಹಿಂತಿರುಗಿ symp;
    }
    VSymEnt* ಸೇರಿಸು_ತುದಿ_Iface(AstCell* nodep, ಸ್ಥಿರಾಂಕ string& scopename) {
        VSymEnt* ಸ್ಥಿರಾಂಕ symp = ಹೊಸ VSymEnt{&m_syms, nodep};
        UINFO(9, "      INSERTtopiface se" << cvt_ಗಮ್ಯ_Hex(symp) << "  " << scopename << " " << nodep);
        symp->parentp(ಬೇರು_Entp());  // Needed so backward search can find name of top module
        symp->fallbackp(dunitEntp());  // Needed so can find $unit stuff
        nodep->user1p(symp);
        ಆದರೆ (nodep->modp()) nodep->modp()->user1p(symp);
        checkDuplicate(ಬೇರು_Entp(), nodep, nodep->origName());
        ಬೇರು_Entp()->ಸೇರಿಸು(nodep->origName(), symp);
        ಆದರೆ (forScopeCreation()) m_nameScopeSymMap.emplace(scopename, symp);
        ಹಿಂತಿರುಗಿ symp;
    }
    VSymEnt* ಸೇರಿಸು_Cell(VSymEnt* abovep, VSymEnt* modSymp, AstCell* nodep,
                        ಸ್ಥಿರಾಂಕ string& scopename) {
        UASSERT_OBJ(abovep, nodep, "Null symbol table inserting node");
        VSymEnt* ಸ್ಥಿರಾಂಕ symp = ಹೊಸ VSymEnt{&m_syms, nodep};
        UINFO(9, "      INSERTcel se" << cvt_ಗಮ್ಯ_Hex(symp) << "  " << scopename << " above=se"
                                      << cvt_ಗಮ್ಯ_Hex(abovep) << " mods=se" << cvt_ಗಮ್ಯ_Hex(modSymp)
                                      << " node=" << nodep);
        symp->parentp(abovep);
        symp->fallbackp(dunitEntp());  // Needed so can find $unit stuff
        nodep->user1p(symp);
        ಆದರೆ (nodep->modp()) nodep->modp()->user1p(symp);
        checkDuplicate(abovep, nodep, nodep->origName());
        abovep->reinsert(nodep->origName(), symp);
        ಆದರೆ (forScopeCreation() && abovep != modSymp && !modSymp->ಕಂಡುಹಿಡಿ_Id_Flat(nodep->name())) {
            // If it's foo_DOT_bar, we need to be able to find it under "foo_DOT_bar" too.
            // Duplicates are possible, as until resolve generates might
            // have 2 same cells under an if
            modSymp->reinsert(nodep->name(), symp);
        }
        ಆದರೆ (forScopeCreation()) m_nameScopeSymMap.emplace(scopename, symp);
        ಹಿಂತಿರುಗಿ symp;
    }
    ಶೂನ್ಯ ಸೇರಿಸು_Map(VSymEnt* symp, ಸ್ಥಿರಾಂಕ string& scopename) {
        ಆದರೆ (forScopeCreation()) m_nameScopeSymMap.emplace(scopename, symp);
    }

    VSymEnt* ಸೇರಿಸು_Inline(VSymEnt* abovep, VSymEnt* modSymp, AstCellInline* nodep,
                          ಸ್ಥಿರಾಂಕ string& basename) {
        // A fake point in the hierarchy, corresponding to a named block flattened by V3Begin.
        // Names are looked up in the module with the block's flattened prefix.
        UASSERT_OBJ(abovep, nodep, "Null symbol table inserting node");
        VSymEnt* ಸ್ಥಿರಾಂಕ symp = ಹೊಸ VSymEnt{&m_syms, nodep};
        UINFO(9, "      INSERTinl se" << cvt_ಗಮ್ಯ_Hex(symp) << "  " << basename << " above=se"
                                      << cvt_ಗಮ್ಯ_Hex(abovep) << " mods=se" << cvt_ಗಮ್ಯ_Hex(modSymp)
                                      << " node=" << nodep);
        symp->parentp(abovep);
        symp->fallbackp(modSymp);
        symp->sym_ಪೂರ್ವಪ್ರತ್ಯಯ(nodep->name() + "__DOT__");
        nodep->user1p(symp);
        checkDuplicate(abovep, nodep, nodep->name());
        abovep->reinsert(basename, symp);
        ಆದರೆ (abovep != modSymp && !modSymp->ಕಂಡುಹಿಡಿ_Id_Flat(nodep->name())) {
            // If it's foo_DOT_bar, we need to be able to find it under that too.
            modSymp->reinsert(nodep->name(), symp);
        }
        ಹಿಂತಿರುಗಿ symp;
    }
    VSymEnt* ಸೇರಿಸು_ಖಂಡ(VSymEnt* abovep, ಸ್ಥಿರಾಂಕ string& name, Ast_ಗಂಟು* nodep,
                         Ast_ಗಂಟು_Module* classOrPackagep) {
        // A fake point in the hierarchy, corresponding to a begin or function/task block
        // After we remove begins these will go away
        // Note we fallback to the symbol table of the parent, as we want to find variables there
        // However, cells walk the graph, so cells will appear under the begin/ftask itself
        UASSERT_OBJ(abovep, nodep, "Null symbol table inserting node");
        VSymEnt* ಸ್ಥಿರಾಂಕ symp = ಹೊಸ VSymEnt{&m_syms, nodep};
        UINFO(9, "      INSERTblk se" << cvt_ಗಮ್ಯ_Hex(symp) << "  above=se" << cvt_ಗಮ್ಯ_Hex(abovep)
                                      << " pkg=" << cvt_ಗಮ್ಯ_Hex(classOrPackagep)
                                      << "  node=" << nodep);
        symp->parentp(abovep);
        symp->classOrPackagep(classOrPackagep);
        symp->fallbackp(abovep);
        nodep->user1p(symp);
        ಆದರೆ (name != "") checkDuplicate(abovep, nodep, name);
        // Duplicates are possible, as until resolve generates might have 2 same cells under an if
        abovep->reinsert(name, symp);
        ಹಿಂತಿರುಗಿ symp;
    }
    VSymEnt* ಸೇರಿಸು_Sym(VSymEnt* abovep, ಸ್ಥಿರಾಂಕ string& name, Ast_ಗಂಟು* nodep,
                       Ast_ಗಂಟು_Module* classOrPackagep) {
        UASSERT_OBJ(abovep, nodep, "Null symbol table inserting node");
        VSymEnt* ಸ್ಥಿರಾಂಕ symp = ಹೊಸ VSymEnt{&m_syms, nodep};
        UINFO(9, "      INSERTsym se" << cvt_ಗಮ್ಯ_Hex(symp) << "  name='" << name << "' above=se"
                                      << cvt_ಗಮ್ಯ_Hex(abovep) << " pkg=" << cvt_ಗಮ್ಯ_Hex(classOrPackagep)
                                      << "  node=" << nodep);
        // We don't remember the ent associated with each node, because we
        // need a unique scope entry for each instantiation
        symp->classOrPackagep(classOrPackagep);
        symp->parentp(abovep);
        symp->fallbackp(abovep);
        nodep->user1p(symp);
        checkDuplicate(abovep, nodep, name);
        abovep->reinsert(name, symp);
        ಹಿಂತಿರುಗಿ symp;
    }
    ಸ್ಥಿರ ತಾರ್ಕಿಕ exists_ಗಂಟು_Sym(Ast_ಗಂಟು* nodep) { ಹಿಂತಿರುಗಿ nodep->user1p() != ಶೂನ್ಯಸೂಚಕ; }
    ಸ್ಥಿರ VSymEnt* get_ಗಂಟು_Sym(Ast_ಗಂಟು* nodep) {
        // Don't use this in ResolveVisitor, as we need to pick up the proper
        // reference under each SCOPE
        VSymEnt* ಸ್ಥಿರಾಂಕ symp = nodep->user1u().ಗಮ್ಯ_Sym_Ent();
        UASSERT_OBJ(symp, nodep, "Module/etc never assigned a symbol entry?");
        ಹಿಂತಿರುಗಿ symp;
    }
    VSymEnt* getScopeSym(AstScope* nodep) {
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ it = m_nameScopeSymMap.ಕಂಡುಹಿಡಿ(nodep->name());
        UASSERT_OBJ(it != m_nameScopeSymMap.ಕೊನೆ(), nodep,
                    "Scope never assigned a symbol entry '" << nodep->name() << "'");
        ಹಿಂತಿರುಗಿ it->ಎರಡನೇ;
    }
    ಶೂನ್ಯ implicit_Ok_ಕೂಡಿಸು(Ast_ಗಂಟು_Module* nodep, ಸ್ಥಿರಾಂಕ string& varname) {
        // Mark the given variable name as being allowed to be implicitly declared
        ಆದರೆ (nodep) {
            ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ it = m_implicit_Name_ಗಣ.ಕಂಡುಹಿಡಿ(std::make_pair(nodep, varname));
            ಆದರೆ (it == m_implicit_Name_ಗಣ.ಕೊನೆ()) m_implicit_Name_ಗಣ.emplace(nodep, varname);
        }
    }
    ತಾರ್ಕಿಕ implicitOk(Ast_ಗಂಟು_Module* nodep, ಸ್ಥಿರಾಂಕ string& varname) {
        ಹಿಂತಿರುಗಿ nodep
               && (m_implicit_Name_ಗಣ.ಕಂಡುಹಿಡಿ(std::make_pair(nodep, varname))
                   != m_implicit_Name_ಗಣ.ಕೊನೆ());
    }

    // Track and later recurse interface modules
    ಶೂನ್ಯ ಸೇರಿಸು_Iface_Mod_Sym(AstIface* nodep, VSymEnt* symp) {
        m_ifaceModSyms.emplace_ಹಿಂಭಾಗ(nodep, symp);
    }
    ಶೂನ್ಯ computeIfaceModSyms();

    // Track and later insert interface references
    ಶೂನ್ಯ ಸೇರಿಸು_Iface_Var_Sym(VSymEnt* symp) {  // Where sym is for a VAR of dtype IFACEREFDTYPE
        m_ifaceVarSyms.ತಳ್ಳು_ಹಿಂಭಾಗ(symp);
    }
    // Iface for a raw or arrayed iface; peels nested array layers for multi-dim arrays.
    ಸ್ಥಿರ AstIfaceRefDType* ifaceRefFromArray(Ast_ಗಂಟು_DType* nodep) {
        ಆಗ (nodep) {
            ಆದರೆ (AstIfaceRefDType* ಸ್ಥಿರಾಂಕ ifp = VN_CAST(nodep, IfaceRefDType)) ಹಿಂತಿರುಗಿ ifp;
            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstBracketArrayDType* ಸ್ಥಿರಾಂಕ arrp = VN_CAST(nodep, BracketArrayDType)) {
                nodep = arrp->subDTypep();
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ AstUnpackArrayDType* ಸ್ಥಿರಾಂಕ arrp = VN_CAST(nodep, UnpackArrayDType)) {
                nodep = arrp->subDTypep();
            } ಇಲ್ಲದಿದ್ದರೆ {
                ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
            }
        }
        ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
    }
    // Given a pin expression, resolve it to a live AstIface* (or nullptr).
    // Handles both simple VarRef and dotted VarXRef pin connections.
    AstIface* ಜೀವಂತ_Iface_From_Pin_ಅಭಿವ್ಯಕ್ತಿ(Ast_ಗಂಟು* exprp, VSymEnt* ಪೋಷಕ_Symp) {
        AstVar* resolvedVarp = ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (AstVarRef* ಸ್ಥಿರಾಂಕ refp = VN_CAST(exprp, VarRef)) {
            resolvedVarp = refp->varp();
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstVarXRef* ಸ್ಥಿರಾಂಕ xrefp = VN_CAST(exprp, VarXRef)) {
            VSymEnt* ಸ್ಥಿರಾಂಕ lookupSymp = ಪೋಷಕ_Symp ? ಪೋಷಕ_Symp->parentp() : ಶೂನ್ಯಸೂಚಕ;
            ಆದರೆ (lookupSymp) {
                string baddot;
                VSymEnt* okSymp = ಶೂನ್ಯಸೂಚಕ;
                ಸ್ಥಿರಾಂಕ string dotpath = xrefp->dotted() + "." + xrefp->name();
                VSymEnt* ಸ್ಥಿರಾಂಕ dotSymp
                    = ಕಂಡುಹಿಡಿ_Dotted(xrefp->fileline(), lookupSymp, dotpath, baddot, okSymp, ನಿಜ);
                ಆದರೆ (dotSymp) resolvedVarp = VN_CAST(dotSymp->nodep(), Var);
            }
        }
        AstIface* resultp = ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (resolvedVarp) {
            AstIfaceRefDType* ಸ್ಥಿರಾಂಕ irefp = ifaceRefFromArray(resolvedVarp->subDTypep());
            ಆದರೆ (irefp && irefp->iface_ಮೂಲಕ_Cellp() && !irefp->iface_ಮೂಲಕ_Cellp()->dead()) {
                resultp = irefp->iface_ಮೂಲಕ_Cellp();
            }
        }
        ಹಿಂತಿರುಗಿ resultp;
    }

    // Attempt to repair a port's AstIfaceRefDType by tracing through the
    // parent cell's pin connection to find the correct live interface.
    // Returns true if repaired or already correct; false if unresolvable.
    ತಾರ್ಕಿಕ repairIfaceRef(VSymEnt* varSymp, AstVar* varp, AstIfaceRefDType* ifacerefp, ತಾರ್ಕಿಕ isDead) {
        // Walk up symbol-table parents to find the enclosing AstCell
        VSymEnt* ಪೋಷಕ_Symp = varSymp->parentp();
        AstCell* ಪೋಷಕ_Cellp = ಶೂನ್ಯಸೂಚಕ;
        ಗಾಗಿ (ಪೂರ್ಣಾಂಕ ಆಳ = 0; ಪೋಷಕ_Symp && ಆಳ < 20; ++ಆಳ) {
            ಆದರೆ (AstCell* ಸ್ಥಿರಾಂಕ cp = VN_CAST(ಪೋಷಕ_Symp->nodep(), Cell)) {
                ಪೋಷಕ_Cellp = cp;
                ನಿಲ್ಲಿಸು;
            }
            ಪೋಷಕ_Symp = ಪೋಷಕ_Symp->parentp();
        }
        ಆದರೆ (!ಪೋಷಕ_Cellp) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        // Scan pins to find the one connected to this port variable
        ಗಾಗಿ (AstPin* pinp = ಪೋಷಕ_Cellp->pinsp(); pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            ಆದರೆ (pinp->modVarp() != varp && pinp->name() != varp->name()) ಮುಂದುವರಿ;
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ exprp = pinp->exprp();
            ಆದರೆ (!exprp) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            AstIface* ಸ್ಥಿರಾಂಕ newIfacep = ಜೀವಂತ_Iface_From_Pin_ಅಭಿವ್ಯಕ್ತಿ(exprp, ಪೋಷಕ_Symp);
            ಆದರೆ (newIfacep && newIfacep != ifacerefp->iface_ಮೂಲಕ_Cellp()) {
                UINFO(4, "  REPAIR-IFACE-REF var=" << varp->prettyNameQ()
                                                   << " old=" << ifacerefp->ifacep()->prettyNameQ()
                                                   << " new=" << newIfacep->prettyNameQ());
                ifacerefp->ifacep(newIfacep);
                ಹಿಂತಿರುಗಿ ನಿಜ;
            }
            ಆದರೆ (newIfacep) ಹಿಂತಿರುಗಿ !isDead;  // Same interface - OK if live
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        }
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }

    ಶೂನ್ಯ computeIfaceVarSyms() {
        ಗಾಗಿ (VSymEnt* varSymp : m_ifaceVarSyms) {
            AstVar* ಸ್ಥಿರಾಂಕ varp = varSymp ? VN_AS(varSymp->nodep(), Var) : ಶೂನ್ಯಸೂಚಕ;
            UINFO(9, "  insAllIface se" << cvt_ಗಮ್ಯ_Hex(varSymp) << " " << varp);
            AstIfaceRefDType* ಸ್ಥಿರಾಂಕ ifacerefp = ifaceRefFromArray(varp->subDTypep());
            UASSERT_OBJ(ifacerefp, varp, "Non-ifacerefs on list!");
            ಆದರೆ (!ifacerefp->cellp() && !ifacerefp->isVirtual() && (varp->pin_ಅಂಕೆ() == 0)) {
                varp->v3error("Interface port declaration "
                              << varp->prettyNameQ() << " doesn't have corresponding port\n"
                              << varp->warn_ಇನ್ನಷ್ಟು()
                                     + "... Perhaps intended an interface instantiation but "
                                       "are missing parenthesis (IEEE 1800-2023 25.3)?");
            }
            ಆದರೆ (!ifacerefp->iface_ಮೂಲಕ_Cellp()) {
                ಆದರೆ (!ifacerefp->cellp()) {  // Probably a NotFoundModule, or a normal module if
                                            // made mistake
                    UINFO(1, "Associated cell " << Ast_ಗಂಟು::prettyNameQ(ifacerefp->cellName()));
                    ifacerefp->v3error("Cannot find file containing interface: "
                                       << Ast_ಗಂಟು::prettyNameQ(ifacerefp->ifaceName()));
                    ಮುಂದುವರಿ;
                } ಇಲ್ಲದಿದ್ದರೆ {
                    ifacerefp->v3fatalSrc("Unlinked interface");
                }
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (varp->isIfaceRef() && !ifacerefp->cellp() && ifacerefp->iface_ಮೂಲಕ_Cellp()) {
                // Port variable (cellp=null) pointing to an interface.
                // The interface may be dead (template replaced by clone) or
                // live-but-wrong (template still alive because some instances
                // use default params, but this port should point to a
                // specialized clone based on the pin connection).
                ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ isDead = ifacerefp->iface_ಮೂಲಕ_Cellp()->dead();
                ಆದರೆ (isDead && forPrimary() && !varp->is_Iface_ಪೋಷಕ()
                    && !v3Global.opt.ತುದಿ_Ifaces_Supported()) {
                    varp->v3warn(E_UNSUPPORTED,
                                 "Unsupported: Interfaced port on top level module");
                }
                // Attempt repair: trace through parent cell's pin to find
                // the correct live interface for this port variable
                ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ repaired = repairIfaceRef(varSymp, varp, ifacerefp, isDead);
                ಆದರೆ (!repaired && isDead) {
                    ifacerefp->v3error(
                        "Interface " << Ast_ಗಂಟು::prettyNameQ(ifacerefp->ifaceName())
                                     << " not connected as parent's interface not connected\n"
                                     << ifacerefp->warn_ಇನ್ನಷ್ಟು()
                                     << "... Perhaps caused by another error on the parent "
                                        "interface that needs resolving\n"
                                     << ifacerefp->warn_ಇನ್ನಷ್ಟು()
                                     << "... Or, perhaps intended an interface instantiation but "
                                        "are missing parenthesis (IEEE 1800-2023 25.3)?");
                    ಮುಂದುವರಿ;
                }
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ifacerefp->iface_ಮೂಲಕ_Cellp()->dead()
                       || !exists_ಗಂಟು_Sym(ifacerefp->iface_ಮೂಲಕ_Cellp())) {
                ifacerefp->iface_ಮೂಲಕ_Cellp()->v3fatalSrc(
                    "Interface referenced by virtual iface is dead. "
                    "Dead interfaces have required linking steps skipped");
            }
            VSymEnt* ಸ್ಥಿರಾಂಕ ifaceSymp = get_ಗಂಟು_Sym(ifacerefp->iface_ಮೂಲಕ_Cellp());
            VSymEnt* if_Or_ಪೋರ್ಟ್_Symp = ifaceSymp;
            // Link Modport names to the Modport Node under the Interface
            ಆದರೆ (ifacerefp->isModport()) {
                VSymEnt* ಸ್ಥಿರಾಂಕ foundp = ifaceSymp->ಕಂಡುಹಿಡಿ_Id_Fallback(ifacerefp->modportName());
                ತಾರ್ಕಿಕ ok = ಸುಳ್ಳು;
                ಆದರೆ (foundp) {
                    ಆದರೆ (AstModport* ಸ್ಥಿರಾಂಕ modportp = VN_CAST(foundp->nodep(), Modport)) {
                        UINFO(4, "Link Modport: " << modportp);
                        ifacerefp->modportp(modportp);
                        if_Or_ಪೋರ್ಟ್_Symp = foundp;
                        ok = ನಿಜ;
                    }
                }
                ಆದರೆ (!ok) {
                    ಸ್ಥಿರಾಂಕ string suggest = suggestSymFallback(ifaceSymp, ifacerefp->modportName(),
                                                              Link_ಗಂಟು_Matcher_Modport{});
                    ifacerefp->modportFileline()->v3error(
                        "Modport not found under interface "
                        << ifacerefp->prettyNameQ(ifacerefp->ifaceName()) << ": "
                        << ifacerefp->prettyNameQ(ifacerefp->modportName()) << '\n'
                        << (suggest.ಖಾಲಿಗಳು() ? "" : ifacerefp->warn_ಇನ್ನಷ್ಟು() + suggest));
                }
            }
            // Alias won't expand until interfaces and modport names are known; see notes at top
            ಸೇರಿಸು_Scope_Alias(SAMN_IFTOP, varSymp, if_Or_ಪೋರ್ಟ್_Symp);
        }
        m_ifaceVarSyms.clear();
    }

    ಶೂನ್ಯ ಸೇರಿಸು_Scope_Alias(SAMNum samn, VSymEnt* lhsp, VSymEnt* rhsp) {
        // Track and later insert scope aliases; an interface referenced by
        // a child cell connecting to that interface
        // Typically lhsp=VAR w/dtype IFACEREF, rhsp=IFACE cell
        UINFO(9, "   insertScopeAlias se" << cvt_ಗಮ್ಯ_Hex(lhsp) << " se" << cvt_ಗಮ್ಯ_Hex(rhsp));
        UASSERT_OBJ(
            !(VN_IS(rhsp->nodep(), Cell) && !VN_IS(VN_AS(rhsp->nodep(), Cell)->modp(), Iface)),
            rhsp->nodep(), "Got a non-IFACE alias RHS");
        m_scopeAliasMap[samn].emplace(lhsp, rhsp);
    }
    ಶೂನ್ಯ computeScopeAliases() {
        UINFO(9, "computeIfaceAliases");
        ಗಾಗಿ (ಪೂರ್ಣಾಂಕ samn = 0; samn < SAMN__MAX; ++samn) {
            ಗಾಗಿ (ScopeAliasMap::iterator it = m_scopeAliasMap[samn].begin();
                 it != m_scopeAliasMap[samn].ಕೊನೆ(); ++it) {
                VSymEnt* ಸ್ಥಿರಾಂಕ lhsp = it->ಮೊದಲ;
                VSymEnt* srcp = lhsp;
                ಆಗ (ನಿಜ) {  // Follow chain of aliases up to highest level non-alias
                    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ it2 = m_scopeAliasMap[samn].ಕಂಡುಹಿಡಿ(srcp);
                    ಆದರೆ (it2 != m_scopeAliasMap[samn].ಕೊನೆ()) {
                        srcp = it2->ಎರಡನೇ;
                        ಮುಂದುವರಿ;
                    } ಇಲ್ಲದಿದ್ದರೆ {
                        ನಿಲ್ಲಿಸು;
                    }
                }
                UINFO(9, "  iiasa: Insert alias se" << lhsp << " (" << lhsp->nodep()->typeName()
                                                    << ") <- se" << srcp << " " << srcp->nodep());
                // srcp should be an interface reference pointing to the interface we want to
                // import
                lhsp->importFromIface(symsp(), srcp);
                // Allow access to objects not permissible to be listed in a modport
                ಆದರೆ (VN_IS(srcp->nodep(), Modport)) {
                    lhsp->importFromIface(symsp(), srcp->parentp(), ನಿಜ);
                }
            }
            // m_scopeAliasMap[samn].clear();  // Done with it, but put into debug file
        }
    }

ಖಾಸಗಿ:
    VSymEnt* ಕಂಡುಹಿಡಿ_With_Alt_Fallback(VSymEnt* symp, ಸ್ಥಿರಾಂಕ string& name, ಸ್ಥಿರಾಂಕ string& altname) {
        VSymEnt* findp = symp->ಕಂಡುಹಿಡಿ_Id_Fallback(name);
        ಆದರೆ (findp) ಹಿಂತಿರುಗಿ findp;
        ಆದರೆ (altname != "") {
            UINFO(8, "     alt fallback");
            findp = symp->ಕಂಡುಹಿಡಿ_Id_Fallback(altname);
        }
        ಹಿಂತಿರುಗಿ findp;
    }

    VSymEnt* ಕಂಡುಹಿಡಿ_With_Alt_Flat(VSymEnt* symp, ಸ್ಥಿರಾಂಕ string& name, ಸ್ಥಿರಾಂಕ string& altname) {
        VSymEnt* findp = symp->ಕಂಡುಹಿಡಿ_Id_Flat(name);
        ಆದರೆ (findp) ಹಿಂತಿರುಗಿ findp;
        ಆದರೆ (altname != "") {
            UINFO(8, "     alt flat");
            findp = symp->ಕಂಡುಹಿಡಿ_Id_Flat(altname);
        }
        ಹಿಂತಿರುಗಿ findp;
    }

ಸಾರ್ವಜನಿಕ:
    VSymEnt* ಕಂಡುಹಿಡಿ_Dotted(File_ಸಾಲು* refLocationp, VSymEnt* lookupSymp, ಸ್ಥಿರಾಂಕ string& dotname,
                        string& baddot, VSymEnt*& okSymp, ತಾರ್ಕಿಕ ಮೊದಲ_Id) {
        // Given a dotted hierarchy name, return where in scope it is
        // Note when dotname=="" we just fall through and return lookupSymp
        UINFO(8, "    dottedFind se" << cvt_ಗಮ್ಯ_Hex(lookupSymp) << " '" << dotname << "'");
        string leftname = dotname;
        okSymp = lookupSymp;  // So can list bad scopes
        ಆಗ (leftname != "") {  // foreach dotted part of xref name
            string::ಗಾತ್ರ_type pos = leftname.ಕಂಡುಹಿಡಿ('.');
            string ident;
            ಆದರೆ (pos != string::npos) {
                ident = leftname.substr(0, pos);
                leftname = leftname.substr(pos + 1);
            } ಇಲ್ಲದಿದ್ದರೆ {
                ident = leftname;
                leftname = "";
            }
            baddot = ident;  // So user can see where they botched it
            okSymp = lookupSymp;
            string altIdent;
            ಆದರೆ (!forScopeCreation()) {
                // GENFOR Begin is foo__BRA__##__KET__ after we've genloop unrolled,
                // but presently should be just "foo".
                // Likewise cell foo__[array] before we've expanded arrays is just foo.
                // Multi-dim iface arrays append multiple __BRA__..__KET__ suffixes; strip them
                // all.
                altIdent = ident;
                ಆಗ (VString::ತುದಿಗಳು_With(altIdent, "__KET__")) {
                    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ braPos = altIdent.rfind("__BRA__");
                    ಆದರೆ (braPos == string::npos) ನಿಲ್ಲಿಸು;
                    altIdent = altIdent.substr(0, braPos);
                }
                ಆದರೆ (altIdent == ident) altIdent.clear();
            }
            UINFO(8, "         id " << ident << " alt " << altIdent << " left " << leftname
                                    << " at se" << lookupSymp);
            // Spec says; Look at existing module (cellnames then modname),
            // then look up (inst name or modname)
            ಆದರೆ (ಮೊದಲ_Id) {
                // Check this module - subcellnames
                ಸ್ಥಿರಾಂಕ AstCell* cellp = lookupSymp ? VN_CAST(lookupSymp->nodep(), Cell)
                                                  : ಶೂನ್ಯಸೂಚಕ;  // Replicated below
                ಆದರೆ (VSymEnt* ಸ್ಥಿರಾಂಕ ಕಂಡುಹಿಡಿ_Symp = ಕಂಡುಹಿಡಿ_With_Alt_Fallback(lookupSymp, ident, altIdent)) {
                    lookupSymp = ಕಂಡುಹಿಡಿ_Symp;
                }

                // Check this module - cur modname
                ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (cellp && cellp->modp()->origName() == ident) {
                }
                // $root we walk up to Netlist
                ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ident == "$root") {
                    lookupSymp = ಬೇರು_Entp();
                    // We've added the '$root' module, now everything else is one lower
                    ಆದರೆ (forScopeCreation()) {
                        lookupSymp = lookupSymp->ಕಂಡುಹಿಡಿ_Id_Flat(ident);
                        UASSERT(lookupSymp, "Cannot find $root module under netlist");
                    }
                }
                // Move up and check cellname + modname
                ಇಲ್ಲದಿದ್ದರೆ {
                    ತಾರ್ಕಿಕ crossedCell = ಸುಳ್ಳು;  // Crossed a cell boundary
                    ಆಗ (lookupSymp) {
                        lookupSymp = lookupSymp->parentp();
                        cellp = lookupSymp ? VN_CAST(lookupSymp->nodep(), Cell)
                                           : ಶೂನ್ಯಸೂಚಕ;  // Replicated above
                        ಆದರೆ (lookupSymp) {
                            UINFO(9, "    Up to " << lookupSymp);
                            ಆದರೆ (cellp || VN_IS(lookupSymp->nodep(), CellInline)) {
                                crossedCell = ನಿಜ;
                            }
                            ಆದರೆ (cellp && cellp->modp()->origName() == ident) {
                                ನಿಲ್ಲಿಸು;
                            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VSymEnt* ಸ್ಥಿರಾಂಕ ಕಂಡುಹಿಡಿ_Sym2p
                                       = ಕಂಡುಹಿಡಿ_With_Alt_Fallback(lookupSymp, ident, altIdent)) {
                                lookupSymp = ಕಂಡುಹಿಡಿ_Sym2p;
                                ಆದರೆ (crossedCell && VN_IS(lookupSymp->nodep(), Var)) {
                                    UINFO(9, "    Not found but matches var name in parent "
                                                 << lookupSymp);
                                    ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
                                }
                                ನಿಲ್ಲಿಸು;
                            }
                        } ಇಲ್ಲದಿದ್ದರೆ {
                            ನಿಲ್ಲಿಸು;
                        }
                    }
                    ಆದರೆ (!lookupSymp) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;  // Not found
                }
            } ಇಲ್ಲದಿದ್ದರೆ {  // Searching for middle path component, must be a cell or interface port
                VSymEnt* ಕಂಡುಹಿಡಿ_Symp = ಕಂಡುಹಿಡಿ_With_Alt_Flat(lookupSymp, ident, altIdent);
                ಆದರೆ (ಕಂಡುಹಿಡಿ_Symp) {
                    lookupSymp = ಕಂಡುಹಿಡಿ_Symp;
                } ಇಲ್ಲದಿದ್ದರೆ {
                    // Try prefixed lookup for interface ports accessed through hierarchy
                    // (e.g., slave_inst.bus.data where bus is an interface port)
                    // After inlining, interface ports have prefixed names like
                    // top__DOT__slave_inst__DOT__bus
                    UINFO(8, "     middle-path: trying prefixed lookup for '" << ident << "'\n");
                    string baddot;
                    ಕಂಡುಹಿಡಿ_Symp = ಕಂಡುಹಿಡಿ_Sym_Prefixed(lookupSymp, ident, baddot, /*fallback=*/ನಿಜ);
                    UINFO(8, "     middle-path: prefixed lookup result: "
                                 << (ಕಂಡುಹಿಡಿ_Symp ? "found" : "not found") << "\n");
                    ಆದರೆ (ಕಂಡುಹಿಡಿ_Symp) {
                        // Check if this is an interface reference variable
                        ಸ್ಥಿರಾಂಕ AstVar* varp = VN_CAST(ಕಂಡುಹಿಡಿ_Symp->nodep(), Var);
                        ಆದರೆ (!varp) {
                            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstVarScope* vscp = VN_CAST(ಕಂಡುಹಿಡಿ_Symp->nodep(), VarScope)) {
                                varp = vscp->varp();
                            }
                        }
                        ಆದರೆ (varp && varp->isIfaceRef()) {
                            // Found an interface port - use findSymp directly
                            // computeScopeAliases has already imported interface members
                            // into this symbol entry, so we use it instead of redirecting
                            // to the shared modport definition (which would cause all
                            // instances to resolve to the same symbols - bug #2656)
                            lookupSymp = ಕಂಡುಹಿಡಿ_Symp;
                        } ಇಲ್ಲದಿದ್ದರೆ {
                            // Non-interface symbol in middle path must be a cell (module instance)
                            // to continue hierarchical resolution
                            ಆದರೆ (VN_IS(ಕಂಡುಹಿಡಿ_Symp->nodep(), Cell)) {
                                lookupSymp = ಕಂಡುಹಿಡಿ_Symp;
                            } ಇಲ್ಲದಿದ್ದರೆ {
                                // Reject non-cell, non-interface symbols found via fallback
                                // to prevent accidental resolution to unrelated symbols
                                UINFO(8, "     middle-path: rejecting non-cell symbol\n");
                                ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
                            }
                        }
                    } ಇಲ್ಲದಿದ್ದರೆ {
                        ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;  // Not found
                    }
                }
            }
            ಆದರೆ (lookupSymp) {
                ಆದರೆ (ಸ್ಥಿರಾಂಕ AstCell* ಸ್ಥಿರಾಂಕ cellp = VN_CAST(lookupSymp->nodep(), Cell)) {
                    ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ modp = cellp->modp()) {
                        ಆದರೆ (modp->hier_ಖಂಡ() && !leftname.ಖಾಲಿಗಳು()) {
                            refLocationp->v3error("Cannot access scope inside hierarchical block");
                        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(modp, Not_ಸಿಕ್ಕಿತು_Module)) {
                            refLocationp->v3error("Dotted reference to instance that refers to "
                                                  "missing module/interface: "
                                                  << modp->prettyNameQ());
                        }
                    }
                }
                // Follow scope alias for nested interface port access
                ಆದರೆ (!leftname.ಖಾಲಿಗಳು()) {
                    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ aliasIt = m_scopeAliasMap[SAMN_IFTOP].ಕಂಡುಹಿಡಿ(lookupSymp);
                    ಆದರೆ (aliasIt != m_scopeAliasMap[SAMN_IFTOP].ಕೊನೆ()) {
                        lookupSymp = aliasIt->ಎರಡನೇ;
                        // Alias may point to __Viftop VarScope; find corresponding Cell
                        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstVarScope* ಸ್ಥಿರಾಂಕ vscp
                            = VN_CAST(lookupSymp->nodep(), VarScope)) {
                            ಸ್ಥಿರಾಂಕ string varName = vscp->varp()->name();
                            ಸ್ಥಿರ constexpr ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ VIFTOP_SUFFIX = "__Viftop";
                            ಆದರೆ (VString::ತುದಿಗಳು_With(varName, VIFTOP_SUFFIX)) {
                                ಸ್ಥಿರಾಂಕ string cellName
                                    = varName.substr(0, varName.ಗಾತ್ರ_2() - ಅಕ್ಷರಸರಣಿ_ಉದ್ದ(VIFTOP_SUFFIX));
                                VSymEnt* ಸ್ಥಿರಾಂಕ ಪೋಷಕ_Symp = lookupSymp->parentp();
                                ಆದರೆ (ಪೋಷಕ_Symp) {
                                    VSymEnt* ಸ್ಥಿರಾಂಕ cellSymp = ಪೋಷಕ_Symp->ಕಂಡುಹಿಡಿ_Id_Flat(cellName);
                                    ಆದರೆ (cellSymp && VN_IS(cellSymp->nodep(), Cell)) {
                                        lookupSymp = cellSymp;  // Use Cell for member lookup
                                    }
                                }
                            }
                        }
                    } ಇಲ್ಲದಿದ್ದರೆ {
                        // No alias; try following IfaceRefDType to interface cell
                        ಸ್ಥಿರಾಂಕ AstVar* varp = VN_CAST(lookupSymp->nodep(), Var);
                        ಆದರೆ (!varp) {
                            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstVarScope* ಸ್ಥಿರಾಂಕ vscp
                                = VN_CAST(lookupSymp->nodep(), VarScope)) {
                                varp = vscp->varp();
                            }
                        }
                        ಆದರೆ (varp && varp->isIfaceRef()) {
                            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstIfaceRefDType* ಸ್ಥಿರಾಂಕ ifaceRefp
                                = ifaceRefFromArray(varp->dtypep())) {
                                ಆದರೆ (ifaceRefp->cellp() && exists_ಗಂಟು_Sym(ifaceRefp->cellp())) {
                                    lookupSymp = get_ಗಂಟು_Sym(ifaceRefp->cellp());
                                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ifaceRefp->iface_ಮೂಲಕ_Cellp()
                                           && exists_ಗಂಟು_Sym(ifaceRefp->iface_ಮೂಲಕ_Cellp())) {
                                    lookupSymp = get_ಗಂಟು_Sym(ifaceRefp->iface_ಮೂಲಕ_Cellp());
                                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ifaceRefp->ifacep()
                                           && exists_ಗಂಟು_Sym(ifaceRefp->ifacep())) {
                                    lookupSymp = get_ಗಂಟು_Sym(ifaceRefp->ifacep());
                                }
                            }
                        }
                    }
                }
            }
            ಮೊದಲ_Id = ಸುಳ್ಳು;
        }
        ಹಿಂತಿರುಗಿ lookupSymp;
    }

    ಸ್ಥಿರ string remove_ಕೊನೆಯ_Inline_Scope(ಸ್ಥಿರಾಂಕ string& name) {
        string ಹೊರಹೋಗು = name;
        ಸ್ಥಿರಾಂಕ string dot = "__DOT__";
        ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t dotPos = ಹೊರಹೋಗು.rfind(dot, ಹೊರಹೋಗು.ಗಾತ್ರ_2() - dot.ಉದ್ದಳತೆ() - 1);
        ಆದರೆ (dotPos == string::npos) {
            ಹಿಂತಿರುಗಿ "";
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಹಿಂತಿರುಗಿ ಹೊರಹೋಗು.erase(dotPos + dot.ಉದ್ದಳತೆ(), string::npos);
        }
    }

    VSymEnt* ಕಂಡುಹಿಡಿ_Sym_Prefixed(VSymEnt* lookupSymp, ಸ್ಥಿರಾಂಕ string& dotname, string& baddot,
                             ತಾರ್ಕಿಕ fallback) {
        // Find symbol in given point in hierarchy, allowing prefix (post-Inline)
        // For simplicity lookupSymp may be passed nullptr result from findDotted
        ಆದರೆ (!lookupSymp) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        UINFO(8, "    findSymPrefixed "
                     << dotname << " under se" << cvt_ಗಮ್ಯ_Hex(lookupSymp)
                     << ((lookupSymp->sym_ಪೂರ್ವಪ್ರತ್ಯಯ() == "") ? "" : " as ")
                     << ((lookupSymp->sym_ಪೂರ್ವಪ್ರತ್ಯಯ() == "") ? "" : lookupSymp->sym_ಪೂರ್ವಪ್ರತ್ಯಯ() + dotname)
                     << "  at se" << lookupSymp << " fallback=" << fallback);
        string ಪೂರ್ವಪ್ರತ್ಯಯ = lookupSymp->sym_ಪೂರ್ವಪ್ರತ್ಯಯ();
        VSymEnt* foundp = ಶೂನ್ಯಸೂಚಕ;
        ಆಗ (!foundp) {
            ಆದರೆ (fallback) {
                foundp = lookupSymp->ಕಂಡುಹಿಡಿ_Id_Fallback(ಪೂರ್ವಪ್ರತ್ಯಯ + dotname);  // Might be nullptr
            } ಇಲ್ಲದಿದ್ದರೆ {
                foundp = lookupSymp->ಕಂಡುಹಿಡಿ_Id_Flat(ಪೂರ್ವಪ್ರತ್ಯಯ + dotname);  // Might be nullptr
            }
            ಆದರೆ (ಪೂರ್ವಪ್ರತ್ಯಯ.ಖಾಲಿಗಳು()) ನಿಲ್ಲಿಸು;
            ಸ್ಥಿರಾಂಕ string ಮುಂದಿನ_ಪೂರ್ವಪ್ರತ್ಯಯ = remove_ಕೊನೆಯ_Inline_Scope(ಪೂರ್ವಪ್ರತ್ಯಯ);
            ಆದರೆ (ಪೂರ್ವಪ್ರತ್ಯಯ == ಮುಂದಿನ_ಪೂರ್ವಪ್ರತ್ಯಯ) ನಿಲ್ಲಿಸು;
            ಪೂರ್ವಪ್ರತ್ಯಯ = ಮುಂದಿನ_ಪೂರ್ವಪ್ರತ್ಯಯ;
        }
        ಆದರೆ (!foundp) baddot = dotname;
        ಹಿಂತಿರುಗಿ foundp;
    }
    ಸ್ಥಿರ ತಾರ್ಕಿಕ checkIfClassOrPackage(ಸ್ಥಿರಾಂಕ VSymEnt* ಸ್ಥಿರಾಂಕ symp) {
        ಆದರೆ (VN_IS(symp->nodep(), Class) || VN_IS(symp->nodep(), Package)) ಹಿಂತಿರುಗಿ ನಿಜ;

        // Helper: check if a RefDType might resolve to a class later.
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ isPotentialClassRef = [](ಸ್ಥಿರಾಂಕ AstRefDType* refp) -> ತಾರ್ಕಿಕ {
            ಹಿಂತಿರುಗಿ refp && !refp->typeofp()
                   && (!refp->classOrPackageOpp()
                       || VN_IS(refp->classOrPackageOpp(), ClassOrPackageRef));
        };

        // Helper: allow types that can represent a class/package handle or an unresolved ref.
        // BasicDType is allowed here only for null/object handles.
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ is_Valid_Type_ಗಂಟು = [](ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep) -> ತಾರ್ಕಿಕ {
            ಹಿಂತಿರುಗಿ VN_IS(nodep, VoidDType) || VN_IS(nodep, BasicDType)
                   || VN_IS(nodep, ClassRefDType) || VN_IS(nodep, ParseRef);
        };

        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstTypedef* ಸ್ಥಿರಾಂಕ typedefp = VN_CAST(symp->nodep(), Typedef)) {
            ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* dtypep = typedefp->subDTypep();
            ಆದರೆ (!dtypep) dtypep = typedefp->ಮಗು_DTypep();
            ಆದರೆ (VN_IS(dtypep, ClassRefDType)) ಹಿಂತಿರುಗಿ ನಿಜ;
            ಆದರೆ (isPotentialClassRef(VN_CAST(dtypep, RefDType))) ಹಿಂತಿರುಗಿ ನಿಜ;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ AstParamTypeDType* ಸ್ಥಿರಾಂಕ paramTypep
                   = VN_CAST(symp->nodep(), ParamTypeDType)) {
            // Before V3Param the declared default is in childDTypep (possibly
            // wrapped in a RequireDType); after V3Param it is consumed and the
            // bound type is the resolved data type, e.g. a type parameter
            // inherited from a specialized base class (REQ #(Item) -> class Item).
            Ast_ಗಂಟು* childp = paramTypep->ಮಗು_DTypep();
            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstRequireDType* ಸ್ಥಿರಾಂಕ reqp = VN_CAST(childp, RequireDType)) {
                childp = reqp->lhsp();
            }
            ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ checkp = childp ? childp : paramTypep->skipRefp();
            ಆದರೆ (is_Valid_Type_ಗಂಟು(checkp) || isPotentialClassRef(VN_CAST(checkp, RefDType))) {
                ಹಿಂತಿರುಗಿ ನಿಜ;
            }
        }
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }
    VSymEnt* resolveClassOrPackage(VSymEnt* lookSymp, AstClassOrPackageRef* nodep, ತಾರ್ಕಿಕ fallback,
                                   ತಾರ್ಕಿಕ classOnly, ಸ್ಥಿರಾಂಕ string& forWhat,
                                   ತಾರ್ಕಿಕ deferIfUnresolved = ಸುಳ್ಳು) {
        ಆದರೆ (nodep->classOrPackageSkipp()) ಹಿಂತಿರುಗಿ get_ಗಂಟು_Sym(nodep->classOrPackageSkipp());
        VSymEnt* foundp;
        VSymEnt* ಹುಡುಕು_Symp = lookSymp;
        ಆದರೆ (VL_UNCOVERABLE(ಹುಡುಕು_Symp && VN_IS(ಹುಡುಕು_Symp->nodep(), ParamTypeDType))) {
            ಹುಡುಕು_Symp->nodep()->v3fatalSrc(  // LCOV_EXCL_LINE
                "resolveClassOrPackage: unexpected ParamTypeDType lookup");
        }
        ಆದರೆ (fallback) {
            VSymEnt* currentLookSymp = ಹುಡುಕು_Symp;
            ಮಾಡು {
                foundp = currentLookSymp->ಕಂಡುಹಿಡಿ_Id_Flat(nodep->name());
                ಆದರೆ (foundp && !checkIfClassOrPackage(foundp)) foundp = ಶೂನ್ಯಸೂಚಕ;
                ಆದರೆ (!foundp) currentLookSymp = currentLookSymp->fallbackp();
            } ಆಗ (!foundp && currentLookSymp);
        } ಇಲ್ಲದಿದ್ದರೆ {
            foundp = ಹುಡುಕು_Symp->ಕಂಡುಹಿಡಿ_Id_Flat(nodep->name());
            ಆದರೆ (foundp && !checkIfClassOrPackage(foundp)) foundp = ಶೂನ್ಯಸೂಚಕ;
        }
        ಆದರೆ (!foundp && v3Global.rootp()->stdPackagep()) {  // Look under implied std::
            foundp = get_ಗಂಟು_Sym(v3Global.rootp()->stdPackagep())->ಕಂಡುಹಿಡಿ_Id_Flat(nodep->name());
        }
        ಆದರೆ (foundp) {
            nodep->classOrPackageNodep(foundp->nodep());
            ಹಿಂತಿರುಗಿ foundp;
        }
        ಆದರೆ (deferIfUnresolved) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        ಸ್ಥಿರಾಂಕ string suggest
            = suggestSymFallback(lookSymp, nodep->name(), Link_ಗಂಟು_Matcher_Class_Or_Package{});
        nodep->v3error((classOnly ? "Class" : "Package/class")
                       << " for '" << forWhat  // extends/implements
                       << "' not found: " << nodep->prettyNameQ() << '\n'
                       << (suggest.ಖಾಲಿಗಳು() ? "" : nodep->warn_ಇನ್ನಷ್ಟು() + suggest));
        ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
    }
    string suggestSymFallback(VSymEnt* lookupSymp, ಸ್ಥಿರಾಂಕ string& name,
                              ಸ್ಥಿರಾಂಕ VNodeMatcher& matcher) {
        // Suggest alternative symbol in given point in hierarchy
        // Does not support inline, as we find user-level errors before inlining
        // For simplicity lookupSymp may be passed nullptr result from findDotted
        ಆದರೆ (!lookupSymp) ಹಿಂತಿರುಗಿ "";
        VSpellCheck speller;
        lookupSymp->candidateIdFallback(&speller, &matcher);
        ಹಿಂತಿರುಗಿ speller.ಉತ್ತಮ_Candidate_ಸಂದೇಶ(name);
    }
    string suggestSymFlat(VSymEnt* lookupSymp, ಸ್ಥಿರಾಂಕ string& name, ಸ್ಥಿರಾಂಕ VNodeMatcher& matcher,
                          ತಾರ್ಕಿಕ includeType) {
        ಆದರೆ (!lookupSymp) ಹಿಂತಿರುಗಿ "";
        VSpellCheck speller;
        lookupSymp->candidateIdFlat(&speller, &matcher, includeType);
        ಹಿಂತಿರುಗಿ speller.ಉತ್ತಮ_Candidate_ಸಂದೇಶ(name);
    }
};

LinkDotState* LinkDotState::s_errorThisp = ಶೂನ್ಯಸೂಚಕ;

//======================================================================

ವರ್ಗ Link_Dot_ಕಂಡುಹಿಡಿ_Visitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // NODE STATE
    // Cleared on global
    //  *::user1p()             -> See LinkDotState
    //  *::user2p()             -> See LinkDotState
    //  *::user3()              // bool. Node stored in m_virtualIfaces
    //  *::user4()              -> See LinkDotState
    ಸ್ಥಿರಾಂಕ VNUser3_ಒಳ_Use m_inuser3;

    // STATE
    LinkDotState* ಸ್ಥಿರಾಂಕ m_statep;  // State to pass between visitors, including symbol table
    Ast_ಗಂಟು_Module* m_classOrPackagep = ಶೂನ್ಯಸೂಚಕ;  // Current package
    AstClocking* m_clockingp = ಶೂನ್ಯಸೂಚಕ;  // Current clocking block
    std::ಸದಿಶ<AstScope*> m_scopes;  // Scopes detached during repeated module visits
    uint64_t m_statScopeVisits = 0;  // Non-top scopes encountered while finding instance symbols
    VSymEnt* m_modSymp = ಶೂನ್ಯಸೂಚಕ;  // Symbol Entry for current module
    VSymEnt* m_curSymp = ಶೂನ್ಯಸೂಚಕ;  // Symbol Entry for current table, where to lookup/insert
    string
        m_hierParamsName;  // Name of module with hierarchical type parameters, empty when not used
    string m_scope;  // Scope text
    ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_FTask* m_ftaskp = ಶೂನ್ಯಸೂಚಕ;  // Current function/task
    ತಾರ್ಕಿಕ m_ಒಳ_Recursion = ಸುಳ್ಳು;  // Inside a recursive module
    ಪೂರ್ಣಾಂಕ m_param_ಅಂಕೆ = 0;  // Parameter number, for position based connection
    ತಾರ್ಕಿಕ m_explicitNew = ಸುಳ್ಳು;  // Hit a "new" function
    ಪೂರ್ಣಾಂಕ m_mod_ಖಂಡ_ಅಂಕೆ = 0;  // Begin block number in module, 0=none seen
    ಪೂರ್ಣಾಂಕ m_mod_With_ಅಂಕೆ = 0;  // With block number, 0=none seen
    ಪೂರ್ಣಾಂಕ m_mod_Arg_ಅಂಕೆ = 0;  // Arg block number for randomize(), 0=none seen
    std::ಸದಿಶ<AstIface*> m_virtIfaces;  // interfaces used as virtual,
                                          // needed for handleUnvisitedVirtIfaces()

    // METHODS
    ಶೂನ್ಯ makeImplicitNew(AstClass* nodep) {
        AstFunc* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ AstFunc{nodep->fileline(), "new", ಶೂನ್ಯಸೂಚಕ, ಶೂನ್ಯಸೂಚಕ};
        // If needed, super.new() call is added the 2nd pass of V3LinkDotResolve,
        // because base classes are already resolved there.
        newp->isConstructor(ನಿಜ);
        nodep->ಕೂಡಿಸು_Membersp(newp);
        UINFO(8, "Made implicit new for " << nodep->name() << ": " << nodep);
        m_statep->ಸೇರಿಸು_ಖಂಡ(m_curSymp, newp->name(), newp, m_classOrPackagep);
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ is_Hier_ಖಂಡ_Wrapper(ಸ್ಥಿರಾಂಕ string& name) {
        ಸ್ಥಿರಾಂಕ V3_Hier_ಖಂಡ_Opt_ಗಣ& hier_ಖಂಡಗಳು = v3Global.opt.hier_ಖಂಡಗಳು();
        ಹಿಂತಿರುಗಿ hier_ಖಂಡಗಳು.ಕಂಡುಹಿಡಿ(name) != hier_ಖಂಡಗಳು.ಕೊನೆ();
    }

    ಶೂನ್ಯ moveExternFuncDecl(Ast_ಗಂಟು_FTask* nodep, AstClass* classp) {
        // Move an 'extern function|task' to inside a class or, from an outer to inner-class
        ಆದರೆ (nodep->isExternDef()) ಹಿಂತಿರುಗಿ;
        // Move it to proper spot under the target class
        nodep->unlink_Fr_ಹಿಂಭಾಗ();
        classp->ಕೂಡಿಸು_Stmtsp(nodep);
        nodep->isExternDef(ನಿಜ);  // So we check there's a matching extern
        nodep->classOrPackagep()->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ();
        // Any "Type::" reference in the function's IO are really "MovedToClass::" references
        ಆದರೆ (nodep->fvarp()) moveExternFuncDeclRefs(nodep->fvarp(), classp);
        ಗಾಗಿ (Ast_ಗಂಟು* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ಆದರೆ (AstVar* ಸ್ಥಿರಾಂಕ portp = VN_CAST(stmtp, Var)) {
                ಆದರೆ (portp->isIO()) moveExternFuncDeclRefs(portp, classp);
            }
        }
    }
    ಶೂನ್ಯ moveExternFuncDeclRefs(Ast_ಗಂಟು* nodep, AstClass* classp) {
        nodep->foreach([ಇದು, classp](AstClassOrPackageRef* refp) {  //
            ಆದರೆ (refp->name() == classp->name() && !refp->paramsp()) {
                UINFO(9, "Cleaning up external function type for class " << refp);
                AstDot* ಸ್ಥಿರಾಂಕ dotp = VN_CAST(refp->backp(), Dot);
                ಆದರೆ (dotp && dotp->lhsp() == refp) {
                    // If had DOT(CLASSREF this, ...) the DOT can go away
                    dotp->replaceWith(dotp->rhsp()->unlink_Fr_ಹಿಂಭಾಗ());
                    VL_DO_DANGLING2(ತಳ್ಳು_Deletep(dotp), dotp, refp);
                } ಇಲ್ಲದಿದ್ದರೆ {
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(refp->unlink_Fr_ಹಿಂಭಾಗ()), refp);
                }
            }
        });
    }

    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(AstNetlist* nodep) override {  // FindVisitor::
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
        Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ modulesp = nodep->modulesp();
        UASSERT_OBJ(modulesp, nodep, "$unit should always be in the netlist");
        ಆದರೆ (m_statep->forPrimary()) {
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ onlyBuiltins = [&]() {
                ಗಾಗಿ (Ast_ಗಂಟು_Module* modp = modulesp; modp;
                     modp = VN_AS(modp->nextp(), ಗಂಟು_Module)) {
                    ಆದರೆ (!modp->isDollarUnit() && !modp->isConstPool()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
                }
                ಹಿಂತಿರುಗಿ ನಿಜ;
            }();
            ಆದರೆ (onlyBuiltins && !v3Global.rootp()->dollarUnitPkgp()->stmtsp()) {
                nodep->v3error("No top level module found");
            }
        }
        ಗಾಗಿ (Ast_ಗಂಟು_Module* modp = nodep->modulesp(); modp && modp->is_ತುದಿ();
             modp = VN_AS(modp->nextp(), ಗಂಟು_Module)) {
            UINFO(8, "Top Module: " << modp);
            m_scope = "TOP";

            ಆದರೆ (!m_statep->forScopeCreation() && v3Global.opt.ತುದಿ_Ifaces_Supported()) {
                ಗಾಗಿ (Ast_ಗಂಟು* subnodep = modp->stmtsp(); subnodep; subnodep = subnodep->nextp()) {
                    ಆದರೆ (AstVar* ಸ್ಥಿರಾಂಕ varp = VN_CAST(subnodep, Var)) {
                        ಆದರೆ (varp->isIfaceRef()) {
                            ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ subtypep = varp->subDTypep();
                            ಸ್ಥಿರಾಂಕ AstIfaceRefDType* ifacerefp = ಶೂನ್ಯಸೂಚಕ;
                            ಆದರೆ (VN_IS(subtypep, IfaceRefDType)) {
                                ifacerefp = VN_AS(varp->subDTypep(), IfaceRefDType);
                            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(subtypep, BracketArrayDType)) {
                                ಸ್ಥಿರಾಂಕ AstBracketArrayDType* ಸ್ಥಿರಾಂಕ arrp
                                    = VN_AS(subtypep, BracketArrayDType);
                                ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ arrsubtypep = arrp->subDTypep();
                                ಆದರೆ (VN_IS(arrsubtypep, IfaceRefDType)) {
                                    ifacerefp = VN_AS(arrsubtypep, IfaceRefDType);
                                }
                            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(subtypep, UnpackArrayDType)) {
                                ಸ್ಥಿರಾಂಕ AstUnpackArrayDType* ಸ್ಥಿರಾಂಕ arrp
                                    = VN_AS(subtypep, UnpackArrayDType);
                                ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ arrsubtypep = arrp->subDTypep();
                                ಆದರೆ (VN_IS(arrsubtypep, IfaceRefDType)) {
                                    ifacerefp = VN_AS(arrsubtypep, IfaceRefDType);
                                }
                            }

                            ಆದರೆ (ifacerefp && !ifacerefp->cellp()) {
                                // A dummy cell to keep the top level interface alive and correctly
                                // optimized for default parameter values
                                AstCell* ifacecellp
                                    = ಹೊಸ AstCell{nodep->fileline(),
                                                  nodep->fileline(),
                                                  modp->name() + "__02E" + varp->name(),
                                                  ifacerefp->ifaceName(),
                                                  ಶೂನ್ಯಸೂಚಕ,
                                                  ಶೂನ್ಯಸೂಚಕ,
                                                  ಶೂನ್ಯಸೂಚಕ};
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
            m_curSymp = m_modSymp = ಶೂನ್ಯಸೂಚಕ;
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Type_ಕೋಷ್ಟಕ*) override {}  // FindVisitor::
    ಶೂನ್ಯ ಭೇಟಿ(AstIfaceRefDType* nodep) override {  // FindVisitor::
        ಆದರೆ (!m_statep->forScopeCreation() && nodep->isVirtual() && nodep->ifacep()
            && !nodep->ifacep()->user3()) {
            m_virtIfaces.ತಳ್ಳು_ಹಿಂಭಾಗ(nodep->ifacep());
            nodep->ifacep()->user3(ನಿಜ);
        }
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Module* nodep) override {  // FindVisitor::
        // Called on top module from Netlist, other modules from the cell creating them,
        // and packages
        UINFO(8, "   " << nodep);
        // m_curSymp/m_modSymp maybe nullptr for packages and non-top modules
        // Packages will be under top after the initial phases, but until then
        // need separate handling
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ standalonePkg
            = !m_modSymp && !m_statep->forScopeCreation() && VN_IS(nodep, Package);
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ doit = (m_modSymp || standalonePkg);
        VL_RESTORER_COPY(m_scope);
        VL_RESTORER(m_classOrPackagep);
        VL_RESTORER(m_modSymp);
        VL_RESTORER(m_curSymp);
        VL_RESTORER(m_param_ಅಂಕೆ);
        VL_RESTORER(m_mod_ಖಂಡ_ಅಂಕೆ);
        VL_RESTORER(m_mod_With_ಅಂಕೆ);
        VL_RESTORER(m_mod_Arg_ಅಂಕೆ);
        ಆದರೆ (doit && nodep->user2()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Identically recursive module (module instantiates "
                          "itself, without changing parameters): "
                              << Ast_ಗಂಟು::prettyNameQ(nodep->origName()));
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (doit) {
            UINFO(4, "     Link Module: " << nodep);
            UASSERT_OBJ(!nodep->dead(), nodep, "Module in instance tree mislabeled as dead?");
            AstPackage* ಸ್ಥಿರಾಂಕ pkgp = VN_CAST(nodep, Package);
            m_classOrPackagep = pkgp;
            ಆದರೆ (standalonePkg) {
                ಆದರೆ (pkgp->isDollarUnit()) {
                    m_curSymp = m_modSymp = m_statep->dunitEntp();
                    nodep->user1p(m_curSymp);
                } ಇಲ್ಲದಿದ್ದರೆ {
                    VSymEnt* ಸ್ಥಿರಾಂಕ ಊರ್ಧ್ವ_Symp = m_statep->dunitEntp();
                    m_scope = nodep->name();
                    m_curSymp = m_modSymp = m_statep->ಸೇರಿಸು_ಖಂಡ(ಊರ್ಧ್ವ_Symp, nodep->name(), nodep,
                                                                  m_classOrPackagep);
                    UINFO(9, "New module scope " << m_curSymp);
                }
            }
            //
            m_param_ಅಂಕೆ = 0;
            m_mod_ಖಂಡ_ಅಂಕೆ = 0;
            m_mod_With_ಅಂಕೆ = 0;
            m_mod_Arg_ಅಂಕೆ = 0;
            // m_modSymp/m_curSymp for non-packages set by AstCell above this module
            // Iterate
            nodep->user2(ನಿಜ);
            iterateChildren(nodep);
            nodep->user2(ಸುಳ್ಳು);
            nodep->user4(ನಿಜ);
            // Interfaces need another pass when signals are resolved. When creating
            // scopes this is done from the AstScopes in LinkDotScopeVisitor instead, as
            // only those entries hold the AstVarScopes the modport items resolve against.
            ಆದರೆ (!m_statep->forScopeCreation()) {
                ಆದರೆ (AstIface* ಸ್ಥಿರಾಂಕ ifacep = VN_CAST(nodep, Iface)) {
                    m_statep->ಸೇರಿಸು_Iface_Mod_Sym(ifacep, m_curSymp);
                }
            }
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (is_Hier_ಖಂಡ_Wrapper(nodep->name())) {
            UINFO(5, "Module is hierarchical block, must not be dead: " << nodep);
            m_scope = nodep->name();
            VSymEnt* ಸ್ಥಿರಾಂಕ ಊರ್ಧ್ವ_Symp = m_curSymp ? m_curSymp : m_statep->ಬೇರು_Entp();
            m_curSymp = m_modSymp
                = m_statep->ಸೇರಿಸು_ಖಂಡ(ಊರ್ಧ್ವ_Symp, nodep->name() + "::", nodep, m_classOrPackagep);
            iterateChildren(nodep);
            nodep->user4(ನಿಜ);
        } ಇಲ್ಲದಿದ್ದರೆ {  // !doit
            ಆದರೆ (nodep->hierParams()) {
                UINFO(1, "Found module with hier type parameters");
                m_hierParamsName = nodep->name();
                ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    ಆದರೆ (ಸ್ಥಿರಾಂಕ AstTypedef* ಸ್ಥಿರಾಂಕ tdef = VN_CAST(stmtp, Typedef)) {
                        UINFO(1, "Inserting hier type parameter typedef: " << tdef);
                        VSymEnt* ಸ್ಥಿರಾಂಕ ಊರ್ಧ್ವ_Symp = m_curSymp ? m_curSymp : m_statep->ಬೇರು_Entp();
                        m_curSymp = m_modSymp = m_statep->ಸೇರಿಸು_ಖಂಡ(ಊರ್ಧ್ವ_Symp, nodep->name(),
                                                                      nodep, m_classOrPackagep);
                    }
                }
            } ಇಲ್ಲದಿದ್ದರೆ {
                // Will be optimized away later
                // Can't remove now, as our backwards iterator will throw up
                UINFO(5, "Module not under any CELL or top - dead module: " << nodep);
            }
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstClass* nodep) override {  // FindVisitor::
        UASSERT_OBJ(m_curSymp, nodep, "Class not under module/package/$unit");
        UINFO(8, "   " << nodep);
        VL_RESTORER_COPY(m_scope);
        VL_RESTORER(m_classOrPackagep);
        VL_RESTORER(m_modSymp);
        VL_RESTORER(m_curSymp);
        VL_RESTORER(m_param_ಅಂಕೆ);
        VL_RESTORER(m_mod_ಖಂಡ_ಅಂಕೆ);
        VL_RESTORER(m_mod_With_ಅಂಕೆ);
        VL_RESTORER(m_mod_Arg_ಅಂಕೆ);
        VL_RESTORER(m_explicitNew);
        {
            UINFO(4, "     Link Class: " << nodep);
            VSymEnt* ಸ್ಥಿರಾಂಕ ಊರ್ಧ್ವ_Symp = m_curSymp;
            m_scope = m_scope + "." + nodep->name();
            m_classOrPackagep = nodep;
            m_curSymp = m_modSymp
                = m_statep->ಸೇರಿಸು_ಖಂಡ(ಊರ್ಧ್ವ_Symp, nodep->name(), nodep, m_classOrPackagep);
            m_statep->ಸೇರಿಸು_Map(m_curSymp, m_scope);
            UINFO(9, "New module scope " << m_curSymp);
            //
            m_param_ಅಂಕೆ = 0;
            m_mod_ಖಂಡ_ಅಂಕೆ = 0;
            m_mod_With_ಅಂಕೆ = 0;
            m_mod_Arg_ಅಂಕೆ = 0;
            m_explicitNew = ಸುಳ್ಳು;
            // m_modSymp/m_curSymp for non-packages set by AstCell above this module
            // Iterate
            iterateChildren(nodep);
            nodep->user4(ನಿಜ);
            // Implicit new needed?
            ಆದರೆ (!m_explicitNew && m_statep->forPrimary()) makeImplicitNew(nodep);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstClassOrPackageRef* nodep) override {  // FindVisitor::
        ಆದರೆ (!nodep->classOrPackageNodep() && nodep->name() == "$unit") {
            nodep->classOrPackageNodep(v3Global.rootp()->dollarUnitPkgp());
        }
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstScope* nodep) override {  // FindVisitor::
        UASSERT_OBJ(m_statep->forScopeCreation(), nodep,
                    "Scopes should only exist right after V3Scope");
        // Detach on first encounter, will re add at the end.
        // Otherwise would be visited O(cells^2) time after V3Scope
        ಆದರೆ (!nodep->is_ತುದಿ()) {
            ++m_statScopeVisits;
            nodep->unlink_Fr_ಹಿಂಭಾಗ();
            m_scopes.ತಳ್ಳು_ಹಿಂಭಾಗ(nodep);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCell* nodep) override {  // FindVisitor::
        UINFO(5, "   CELL under " << m_scope << " is " << nodep);
        // Process XREFs/etc inside pins
        ಆದರೆ (nodep->recursive() && m_ಒಳ_Recursion) ಹಿಂತಿರುಗಿ;
        iterateChildren(nodep);
        // Recurse in, preserving state
        VL_RESTORER_COPY(m_scope);
        VL_RESTORER(m_modSymp);
        VL_RESTORER(m_curSymp);
        VL_RESTORER(m_param_ಅಂಕೆ);
        VL_RESTORER(m_ಒಳ_Recursion);
        // Where do we add it?
        VSymEnt* aboveSymp = m_curSymp;
        ಸ್ಥಿರಾಂಕ string origname = Ast_ಗಂಟು::dedotName(nodep->name());
        string::ಗಾತ್ರ_type pos = origname.rfind('.');
        ಆದರೆ (pos != string::npos) {
            // Flattened, find what CellInline it should live under
            ಸ್ಥಿರಾಂಕ string scope = origname.substr(0, pos);
            string baddot;
            VSymEnt* okSymp;
            aboveSymp
                = m_statep->ಕಂಡುಹಿಡಿ_Dotted(nodep->fileline(), aboveSymp, scope, baddot, okSymp, ಸುಳ್ಳು);
            UASSERT_OBJ(aboveSymp, nodep,
                        "Can't find instance insertion point at "
                            << Ast_ಗಂಟು::prettyNameQ(baddot) << " in: " << nodep->prettyNameQ());
        }
        {
            m_scope = m_scope + "." + nodep->name();
            m_curSymp = m_modSymp = m_statep->ಸೇರಿಸು_Cell(aboveSymp, m_modSymp, nodep, m_scope);
            m_ಒಳ_Recursion = nodep->recursive();
            // We don't report NotFoundModule, as may be a unused module in a generate
            ಆದರೆ (nodep->modp()) iterate(nodep->modp());
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCellInline* nodep) override {  // FindVisitor::
        UINFO(5, "   CELLINLINE under " << m_scope << " is " << nodep);
        VSymEnt* aboveSymp = m_curSymp;
        std::string ident = nodep->name();
        // If baz__DOT__foo__DOT__bar, we need to find baz__DOT__foo and add bar to it.
        ಸ್ಥಿರಾಂಕ std::string::ಗಾತ್ರ_type pos = ident.rfind("__DOT__");
        ಆದರೆ (pos != std::string::npos) {
            ಸ್ಥಿರಾಂಕ std::string dotted = ident.substr(0, pos);
            ident = ident.substr(pos + std::ಅಕ್ಷರಸರಣಿ_ಉದ್ದ("__DOT__"));
            std::string baddot;
            VSymEnt* okSymp;
            aboveSymp = m_statep->ಕಂಡುಹಿಡಿ_Dotted(nodep->fileline(), aboveSymp, dotted, baddot, okSymp,
                                             ಸುಳ್ಳು);
            UASSERT_OBJ(aboveSymp, nodep,
                        "Can't find cellinline insertion point at "
                            << Ast_ಗಂಟು::prettyNameQ(baddot) << " in: " << nodep->prettyNameQ());
        }
        m_statep->ಸೇರಿಸು_Inline(aboveSymp, m_modSymp, nodep, ident);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstDefParam* nodep) override {  // FindVisitor::
        nodep->user1p(m_curSymp);
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstRefDType* nodep) override {  // FindVisitor::
        nodep->user1p(m_curSymp);
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Gen_ಖಂಡ* nodep) override {  // FindVisitor::
        UINFO(5, "   " << nodep);
        ಆದರೆ (nodep->name() == "" && nodep->unnamed()) {
            // Unnamed blocks are only important when they contain var
            // decls, so search for them. (Otherwise adding all the
            // unnamed#'s would just confuse tracing variables in
            // places such as tasks, where "task ...; begin ... end"
            // are common.
            ಗಾಗಿ (Ast_ಗಂಟು* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
                ಆದರೆ (VN_IS(itemp, Var) || VN_IS(itemp, Foreach)) {
                    std::string name;
                    ಸ್ಥಿರಾಂಕ std::string ಹೆಜ್ಜೆ_Str = m_statep->forPrimary()
                                                    ? ""
                                                    : std::ಗಮ್ಯ_string(m_statep->ಹೆಜ್ಜೆ_ಸಂಖ್ಯೆ()) + "_";
                    ಮಾಡು {
                        ++m_mod_ಖಂಡ_ಅಂಕೆ;
                        name = "unnamedblk" + ಹೆಜ್ಜೆ_Str + cvt_ಗಮ್ಯ_Str(m_mod_ಖಂಡ_ಅಂಕೆ);
                        // Increment again if earlier pass of V3LinkDot claimed this name
                    } ಆಗ (m_curSymp->ಕಂಡುಹಿಡಿ_Id_Flat(name));
                    nodep->name(name);
                    ನಿಲ್ಲಿಸು;
                }
            }
        }
        ಆದರೆ (nodep->name() == "") {
            iterateChildren(nodep);
        } ಇಲ್ಲದಿದ್ದರೆ {
            VL_RESTORER(m_curSymp);
            {
                m_curSymp
                    = m_statep->ಸೇರಿಸು_ಖಂಡ(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
                m_curSymp->fallbackp(VL_RESTORER_PREV(m_curSymp));
                // Iterate
                iterateChildren(nodep);
            }
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_ಖಂಡ* nodep) override {  // FindVisitor::
        UINFO(5, "   " << nodep);
        ಆದರೆ (nodep->name() == "" && nodep->unnamed()) {
            // Unnamed blocks are only important when they contain var
            // decls, so search for them. (Otherwise adding all the
            // unnamed#'s would just confuse tracing variables in
            // places such as tasks, where "task ...; begin ... end"
            // are common.
            ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ ಒಳಗೊಂಡಿದೆ_Decl = [](ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodesp) -> ತಾರ್ಕಿಕ {
                ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* np = nodesp; np; np = np->nextp()) {
                    ಆದರೆ (VN_IS(np, Var) || VN_IS(np, Foreach)) ಹಿಂತಿರುಗಿ ನಿಜ;
                }
                ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            };
            ತಾರ್ಕಿಕ needName = nodep->declsp() || ಒಳಗೊಂಡಿದೆ_Decl(nodep->stmtsp());
            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstFork* ಸ್ಥಿರಾಂಕ forkp = VN_CAST(nodep, Fork)) {
                needName = needName || ಒಳಗೊಂಡಿದೆ_Decl(forkp->forksp());
            }
            ಆದರೆ (needName) {
                ಸ್ಥಿರಾಂಕ std::string ಹೆಜ್ಜೆ_Str
                    = m_statep->forPrimary() ? "" : std::ಗಮ್ಯ_string(m_statep->ಹೆಜ್ಜೆ_ಸಂಖ್ಯೆ()) + "_";
                std::string name;
                ಮಾಡು {
                    ++m_mod_ಖಂಡ_ಅಂಕೆ;
                    name = "unnamedblk" + ಹೆಜ್ಜೆ_Str + cvt_ಗಮ್ಯ_Str(m_mod_ಖಂಡ_ಅಂಕೆ);
                    // Increment again if earlier pass of V3LinkDot claimed this name
                } ಆಗ (m_curSymp->ಕಂಡುಹಿಡಿ_Id_Flat(name));
                nodep->name(name);
            }
        }
        ಆದರೆ (nodep->name() == "") {
            iterateChildren(nodep);
        } ಇಲ್ಲದಿದ್ದರೆ {
            VL_RESTORER(m_curSymp);
            {
                m_curSymp
                    = m_statep->ಸೇರಿಸು_ಖಂಡ(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
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
        ಸ್ಥಿರಾಂಕ string& ಪೋರ್ಟ್_Name = nodep->iface_ಪೋರ್ಟ್_Name();
        UINFO(5, "   moveIfaceExportBody: port=" << ಪೋರ್ಟ್_Name << " task=" << nodep->name());
        // Look up the interface port variable in the module's symbol table
        VSymEnt* ಸ್ಥಿರಾಂಕ ಪೋರ್ಟ್_Symp = m_modSymp->ಕಂಡುಹಿಡಿ_Id_Fallback(ಪೋರ್ಟ್_Name);
        ಆದರೆ (!ಪೋರ್ಟ್_Symp) {
            nodep->v3error("Interface port not found for out-of-block definition: '"
                           << ಪೋರ್ಟ್_Name << "' (IEEE 1800-2023 25.7)");
            nodep->unlink_Fr_ಹಿಂಭಾಗ();
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            ಹಿಂತಿರುಗಿ;
        }
        AstVar* ಸ್ಥಿರಾಂಕ portp = VN_CAST(ಪೋರ್ಟ್_Symp->nodep(), Var);
        ಆದರೆ (!portp) {
            nodep->v3fatalSrc("Out-of-block definition port is not a variable: " << ಪೋರ್ಟ್_Name);
            ಹಿಂತಿರುಗಿ;
        }
        // Get the interface name from the port's type
        // At this stage dtypep() may not be set yet; check childDTypep() too
        AstIfaceRefDType* ifaceRefDtp = ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (portp->dtypep()) { ifaceRefDtp = VN_CAST(portp->dtypep(), IfaceRefDType); }
        ಆದರೆ (!ifaceRefDtp && portp->ಮಗು_DTypep()) {
            ifaceRefDtp = VN_CAST(portp->ಮಗು_DTypep(), IfaceRefDType);
        }
        ಆದರೆ (!ifaceRefDtp) {
            nodep->v3error("Out-of-block definition port is not an interface port: '"
                           << ಪೋರ್ಟ್_Name << "' (IEEE 1800-2023 25.7)");
            nodep->unlink_Fr_ಹಿಂಭಾಗ();
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            ಹಿಂತಿರುಗಿ;
        }
        ಸ್ಥಿರಾಂಕ string ifaceName = ifaceRefDtp->ifaceName();
        UINFO(5, "   moveIfaceExportBody: iface=" << ifaceName);
        // Find the interface module by name
        Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ ifaceModp = m_statep->ಕಂಡುಹಿಡಿ_Module_Sym(ifaceName);
        ಆದರೆ (!ifaceModp) {
            nodep->v3fatalSrc("Interface not found for out-of-block definition: " << ifaceName);
            ಹಿಂತಿರುಗಿ;
        }
        // Find the prototype task in the interface
        Ast_ಗಂಟು_FTask* protoTaskp = ಶೂನ್ಯಸೂಚಕ;
        ಗಾಗಿ (Ast_ಗಂಟು* itemp = ifaceModp->stmtsp(); itemp; itemp = itemp->nextp()) {
            ಆದರೆ (Ast_ಗಂಟು_FTask* ಸ್ಥಿರಾಂಕ ftaskp = VN_CAST(itemp, ಗಂಟು_FTask)) {
                ಆದರೆ (ftaskp->name() == nodep->name() && ftaskp->prototype()) {
                    protoTaskp = ftaskp;
                    ನಿಲ್ಲಿಸು;
                }
            }
        }
        ಆದರೆ (!protoTaskp) {
            nodep->v3error("No matching export prototype found in interface "
                           << ifaceName << " for task " << nodep->prettyNameQ()
                           << " (IEEE 1800-2023 25.7)");
            nodep->unlink_Fr_ಹಿಂಭಾಗ();
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            ಹಿಂತಿರುಗಿ;
        }
        // Move body statements (non-port declarations) from the out-of-block task
        // to the prototype task. Rewrite port.X references to just X in the body.
        ಗಾಗಿ (Ast_ಗಂಟು* stmtp = nodep->stmtsp(); stmtp;) {
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ nextp = stmtp->nextp();
            // Skip port declarations -- they already exist in the prototype
            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ varp = VN_CAST(stmtp, Var)) {
                ಆದರೆ (varp->isIO()) {
                    stmtp = nextp;
                    ಮುಂದುವರಿ;
                }
            }
            // Clone this statement and rewrite port references
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ newStmtp = stmtp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);
            rewrite_Iface_ಪೋರ್ಟ್_Refs_Single(newStmtp, ಪೋರ್ಟ್_Name);
            protoTaskp->ಕೂಡಿಸು_Stmtsp(newStmtp);
            stmtp = nextp;
        }
        // Mark prototype as no longer prototype (it has a body now)
        protoTaskp->prototype(ಸುಳ್ಳು);
        // Delete the out-of-block task from the module
        nodep->unlink_Fr_ಹಿಂಭಾಗ();
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }

    // Rewrite AstDot nodes where lhsp is AstParseRef with the port name
    // to just the rhsp (stripping the port prefix).
    ಸ್ಥಿರ ಶೂನ್ಯ rewrite_Iface_ಪೋರ್ಟ್_Refs_Single(Ast_ಗಂಟು* nodep, ಸ್ಥಿರಾಂಕ string& ಪೋರ್ಟ್_Name) {
        ಆದರೆ (!nodep) ಹಿಂತಿರುಗಿ;
        // Check if this is a non-colon Dot with lhs matching portName
        ಆದರೆ (AstDot* ಸ್ಥಿರಾಂಕ dotp = VN_CAST(nodep, Dot)) {
            ಆದರೆ (!dotp->colon()) {
                ಸ್ಥಿರಾಂಕ AstParseRef* ಸ್ಥಿರಾಂಕ lhsRefp = VN_CAST(dotp->lhsp(), ParseRef);
                ಆದರೆ (lhsRefp && lhsRefp->name() == ಪೋರ್ಟ್_Name) {
                    UINFO(5, "   rewriteIfacePortRefs: stripping port prefix from " << dotp);
                    Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ rhsp = dotp->rhsp()->unlink_Fr_ಹಿಂಭಾಗ();
                    dotp->replaceWith(rhsp);
                    VL_DO_DANGLING(dotp->delete_ವೃಕ್ಷ(), dotp);
                    rewrite_Iface_ಪೋರ್ಟ್_Refs_Single(rhsp, ಪೋರ್ಟ್_Name);
                    ಹಿಂತಿರುಗಿ;
                }
            }
        }
        // Recurse into all children
        ಗಾಗಿ (Ast_ಗಂಟು* childp : {nodep->op1p(), nodep->op2p(), nodep->op3p(), nodep->op4p()}) {
            ಆಗ (childp) {
                Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ nextp = childp->nextp();
                rewrite_Iface_ಪೋರ್ಟ್_Refs_Single(childp, ಪೋರ್ಟ್_Name);
                childp = nextp;
            }
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_FTask* nodep) override {  // FindVisitor::
        // NodeTask: Remember its name for later resolution
        UINFO(5, "   " << nodep);
        UASSERT_OBJ(m_curSymp && m_modSymp, nodep, "Function/Task not under module?");
        ಆದರೆ (nodep->name() == "new") m_explicitNew = ನಿಜ;
        // Handle out-of-block interface method definition (IEEE 25.8)
        ಆದರೆ (!nodep->iface_ಪೋರ್ಟ್_Name().ಖಾಲಿಗಳು() && m_statep->forPrimary()) {
            moveIfaceExportBody(nodep);
            ಹಿಂತಿರುಗಿ;  // Task has been deleted
        }
        // Remember the existing symbol table scope
        VL_RESTORER(m_classOrPackagep);
        VL_RESTORER(m_curSymp);
        VSymEnt* ಮೇಲೆ_Symp = m_curSymp;

        ಆದರೆ (VN_IS(m_curSymp->nodep(), Class)) {
            ಆದರೆ (VN_AS(m_curSymp->nodep(), Class)->isInterfaceClass() && !nodep->ಶುದ್ಧ_Virtual()
                && !nodep->isConstructor()) {
                nodep->v3error("Interface class functions must be pure virtual"
                               << " (IEEE 1800-2023 8.26): " << nodep->prettyNameQ());
            }
            ಆದರೆ (m_statep->forPrimary()
                && (nodep->name() == "randomize" || nodep->name() == "srandom")) {
                nodep->v3error(nodep->prettyNameQ()
                               << " is a predefined class method; redefinition not allowed"
                                  " (IEEE 1800-2023 18.6.3)");
            }
        }
        // Change to appropriate package if extern declaration (vs definition)
        ಆದರೆ (nodep->classOrPackagep()) {
            // class-in-class
            AstDot* ಸ್ಥಿರಾಂಕ dotp = VN_CAST(nodep->classOrPackagep(), Dot);
            AstClassOrPackageRef* ಸ್ಥಿರಾಂಕ cpackagerefp
                = VN_CAST(nodep->classOrPackagep(), ClassOrPackageRef);
            ಆದರೆ (dotp) {
                AstClassOrPackageRef* ಸ್ಥಿರಾಂಕ lhsp = VN_AS(dotp->lhsp(), ClassOrPackageRef);
                m_statep->resolveClassOrPackage(m_curSymp, lhsp, ನಿಜ, ಸುಳ್ಳು,
                                                "External definition :: reference");
                AstClass* ಸ್ಥಿರಾಂಕ lhsclassp = VN_CAST(lhsp->classOrPackageSkipp(), Class);
                ಆದರೆ (!lhsclassp) {
                    nodep->v3error("Extern declaration's scope is not a defined class");
                } ಇಲ್ಲದಿದ್ದರೆ {
                    m_curSymp = m_statep->get_ಗಂಟು_Sym(lhsclassp);
                    ಮೇಲೆ_Symp = m_curSymp;
                    AstClassOrPackageRef* ಸ್ಥಿರಾಂಕ rhsp = VN_AS(dotp->rhsp(), ClassOrPackageRef);
                    m_statep->resolveClassOrPackage(m_curSymp, rhsp, ನಿಜ, ಸುಳ್ಳು,
                                                    "External definition :: reference");
                    AstClass* ಸ್ಥಿರಾಂಕ rhsclassp = VN_CAST(rhsp->classOrPackageSkipp(), Class);
                    ಆದರೆ (!rhsclassp) {
                        nodep->v3error("Extern declaration's scope is not a defined class");
                    } ಇಲ್ಲದಿದ್ದರೆ {
                        m_curSymp = m_statep->get_ಗಂಟು_Sym(rhsclassp);
                        ಮೇಲೆ_Symp = m_curSymp;
                        moveExternFuncDecl(nodep, rhsclassp);
                    }
                }
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (cpackagerefp) {
                ಆದರೆ (!cpackagerefp->classOrPackageSkipp()) {
                    m_statep->resolveClassOrPackage(m_curSymp, cpackagerefp, ನಿಜ, ಸುಳ್ಳು,
                                                    "External definition :: reference");
                }
                AstClass* ಸ್ಥಿರಾಂಕ classp = VN_CAST(cpackagerefp->classOrPackageSkipp(), Class);
                ಆದರೆ (!classp) {
                    nodep->v3error("Extern declaration's scope is not a defined class");
                } ಇಲ್ಲದಿದ್ದರೆ {
                    m_curSymp = m_statep->get_ಗಂಟು_Sym(classp);
                    ಮೇಲೆ_Symp = m_curSymp;
                    moveExternFuncDecl(nodep, classp);
                }
            } ಇಲ್ಲದಿದ್ದರೆ {
                v3fatalSrc("Unhandled extern function definition package");
            }
        }
        // Set the class as package for iteration
        ಆದರೆ (VN_IS(m_curSymp->nodep(), Class)) {
            m_classOrPackagep = VN_AS(m_curSymp->nodep(), Class);
        }
        // Create symbol table for the task's vars
        ಸ್ಥಿರಾಂಕ string name = (nodep->isExternProto() ? "extern "s : ""s) + nodep->name();
        m_curSymp = m_statep->ಸೇರಿಸು_ಖಂಡ(m_curSymp, name, nodep, m_classOrPackagep);
        m_curSymp->fallbackp(ಮೇಲೆ_Symp);
        // Convert the func's range to the output variable
        // This should probably be done in the Parser instead, as then we could
        // just attach normal signal attributes to it.
        ಆದರೆ (!nodep->isExternProto() && nodep->fvarp() && !VN_IS(nodep->fvarp(), Var)) {
            Ast_ಗಂಟು_DType* dtypep = VN_CAST(nodep->fvarp(), ಗಂಟು_DType);
            // If unspecified, function returns one bit; however when we
            // support NEW() it could also return the class reference.
            ಆದರೆ (dtypep) {
                dtypep->unlink_Fr_ಹಿಂಭಾಗ();
            } ಇಲ್ಲದಿದ್ದರೆ {
                dtypep = ಹೊಸ AstBasicDType{nodep->fileline(), VBasicDTypeKwd::LOGIC};
            }
            AstVar* ಸ್ಥಿರಾಂಕ newvarp
                = ಹೊಸ AstVar{nodep->fileline(), VVarType::VAR, nodep->name(), VFlag_ಮಗು_DType{},
                             dtypep};  // Not dtype resolved yet
            newvarp->direction(VDirection::OUTPUT);
            newvarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            newvarp->funcReturn(ನಿಜ);
            newvarp->trace(ಸುಳ್ಳು);  // Not user visible
            nodep->fvarp(newvarp);
            // Explicit insert required, as the var name shadows the upper level's task name
            m_statep->ಸೇರಿಸು_Sym(m_curSymp, newvarp->name(), newvarp, ಶೂನ್ಯಸೂಚಕ /*classOrPackagep*/);
        }
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstClocking* nodep) override {  // FindVisitor::
        VL_RESTORER(m_clockingp);
        m_clockingp = nodep;
        iterate(nodep->sentreep());
        iterate_And_ಮುಂದಿನ_Null(nodep->itemsp());
        // If the block has no name, one cannot reference the clockvars
        VSymEnt* itSymp = ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (nodep->isGlobal()  //
            && m_statep->forPrimary()) {  // else flattening may see two globals
            m_statep->checkDuplicate(m_curSymp, nodep, "__024global_clock");
            itSymp
                = m_statep->ಸೇರಿಸು_ಖಂಡ(m_curSymp, "__024global_clock", nodep, m_classOrPackagep);
            itSymp->fallbackp(ಶೂನ್ಯಸೂಚಕ);
        }
        ಆದರೆ (!nodep->name().ಖಾಲಿಗಳು()) {
            itSymp = m_statep->ಸೇರಿಸು_ಖಂಡ(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
            itSymp->fallbackp(ಶೂನ್ಯಸೂಚಕ);
        }
        ಆದರೆ (itSymp) {
            VL_RESTORER(m_curSymp);
            m_curSymp = itSymp;
            iterate_And_ಮುಂದಿನ_Null(nodep->itemsp());
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Clocking_ಅಂಶ* nodep) override {  // FindVisitor::
        ಆದರೆ (nodep->varp()) {
            ಆದರೆ (m_curSymp->nodep() == m_clockingp) iterate(nodep->varp());
            ಹಿಂತಿರುಗಿ;
        }
        std::string varname;
        Ast_ಗಂಟು_DType* dtypep;
        ಆದರೆ (Ast_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ assignp = nodep->assignp()) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rhsp = assignp->rhsp()->unlink_Fr_ಹಿಂಭಾಗ();
            dtypep = ಹೊಸ AstRefDType{nodep->fileline(), AstRefDType::Flag_Type_ದ_ಅಭಿವ್ಯಕ್ತಿ{},
                                     rhsp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು)};
            nodep->exprp(rhsp);
            varname = assignp->lhsp()->name();
            VL_DO_DANGLING(assignp->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ(), assignp);
        } ಇಲ್ಲದಿದ್ದರೆ {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ refp = nodep->exprp();
            ಸ್ಥಿರಾಂಕ VSymEnt* foundp = m_curSymp->ಕಂಡುಹಿಡಿ_Id_Fallback(refp->name());
            ಆದರೆ (!foundp || !foundp->nodep()) {
                refp->v3error("Corresponding variable " << refp->prettyNameQ()
                                                        << " does not exist");
                VL_DO_DANGLING(nodep->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ(), nodep);
                ಹಿಂತಿರುಗಿ;
            }
            varname = refp->name();
            dtypep = VN_AS(foundp->nodep(), Var)->ಮಗು_DTypep()->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);
        }
        AstVar* ಸ್ಥಿರಾಂಕ newvarp = ಹೊಸ AstVar{nodep->fileline(), VVarType::MODULETEMP, varname,
                                           VFlag_ಮಗು_DType{}, dtypep};
        newvarp->lifetime(VLifetime::STATIC_EXPLICIT);
        nodep->varp(newvarp);
        iterate(nodep->exprp());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstConstraint* nodep) override {  // FindVisitor::
        VL_RESTORER(m_curSymp);
        // Change to appropriate package if extern declaration (vs definition)
        VSymEnt* ಮೇಲೆ_Symp = m_curSymp;
        ಆದರೆ (nodep->classOrPackagep()) {
            AstClassOrPackageRef* ಸ್ಥಿರಾಂಕ cpackagerefp
                = VN_CAST(nodep->classOrPackagep(), ClassOrPackageRef);
            ಆದರೆ (!cpackagerefp) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: extern constraint definition with class-in-class");
            } ಇಲ್ಲದಿದ್ದರೆ {
                ಆದರೆ (!cpackagerefp->classOrPackageSkipp()) {
                    m_statep->resolveClassOrPackage(m_curSymp, cpackagerefp, ನಿಜ, ಸುಳ್ಳು,
                                                    "External definition :: reference");
                }
                AstClass* ಸ್ಥಿರಾಂಕ classp = VN_CAST(cpackagerefp->classOrPackageSkipp(), Class);
                ಆದರೆ (!classp) {
                    nodep->v3error("Extern declaration's scope is not a defined class");
                } ಇಲ್ಲದಿದ್ದರೆ {
                    m_curSymp = m_statep->get_ಗಂಟು_Sym(classp);
                    ಮೇಲೆ_Symp = m_curSymp;
                    // Move it to proper spot under the target class
                    nodep->unlink_Fr_ಹಿಂಭಾಗ();
                    classp->ಕೂಡಿಸು_Stmtsp(nodep);
                    nodep->classOrPackagep()->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ();
                }
            }
        }
        // Set the class as package for iteration
        ಸ್ಥಿರಾಂಕ string name = (nodep->isExternProto() ? "extern "s : ""s) + nodep->name();
        m_curSymp = m_statep->ಸೇರಿಸು_ಖಂಡ(ಮೇಲೆ_Symp, name, nodep, m_classOrPackagep);
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstVar* nodep) override {  // FindVisitor::
        // Var: Remember its name for later resolution
        UASSERT_OBJ(m_curSymp && m_modSymp, nodep, "Var not under module?");
        iterateChildren(nodep);
        ಆದರೆ (VN_IS(m_curSymp->nodep(), Class)
            && VN_AS(m_curSymp->nodep(), Class)->isInterfaceClass() && !nodep->isParam()) {
            nodep->v3error("Interface class cannot contain non-parameter members"
                           << " (IEEE 1800-2023 8.26): " << nodep->prettyNameQ());
        }
        ಆದರೆ (!m_statep->forScopeCreation()) {
            // Find under either a task or the module's vars
            ಸ್ಥಿರಾಂಕ VSymEnt* foundp = m_curSymp->ಕಂಡುಹಿಡಿ_Id_Fallback(nodep->name());
            ಆದರೆ (!foundp && m_modSymp && nodep->name() == m_modSymp->nodep()->name()) {
                foundp = m_modSymp;  // Conflicts with modname?
            }
            AstVar* ಸ್ಥಿರಾಂಕ findvarp = foundp ? VN_CAST(foundp->nodep(), Var) : ಶೂನ್ಯಸೂಚಕ;
            // clocking items can have duplicate names (inout)
            ಆದರೆ (findvarp && VN_IS(findvarp->backp(), Clocking_ಅಂಶ)
                && VN_IS(nodep->backp(), Clocking_ಅಂಶ)) {
                Ast_Clocking_ಅಂಶ* ಸ್ಥಿರಾಂಕ itemp = VN_AS(nodep->backp(), Clocking_ಅಂಶ);
                Ast_Clocking_ಅಂಶ* ಸ್ಥಿರಾಂಕ finditemp = VN_AS(findvarp->backp(), Clocking_ಅಂಶ);
                UINFO(4, "ClockCompl: " << itemp << " ;; " << finditemp);
                UINFO(4, "ClockCompV: " << nodep << " ;; " << findvarp);
                ಆದರೆ (*itemp->exprp()->fileline() == *finditemp->exprp()->fileline()) {
                    UASSERT_OBJ(finditemp->direction() == VDirection::ಒಳಹರಿವು
                                    && itemp->direction() == VDirection::OUTPUT,
                                itemp, "Input after output?");
                    // pretend nothing found and rename
                    foundp = ಶೂನ್ಯಸೂಚಕ;
                    nodep->name("__Voutput_" + nodep->name());
                    finditemp->outputp(itemp);
                }
            }
            ತಾರ್ಕಿಕ ins = ಸುಳ್ಳು;
            ಆದರೆ (!foundp) {
                ins = ನಿಜ;
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!findvarp && m_curSymp->ಕಂಡುಹಿಡಿ_Id_Flat(nodep->name()) && !foundp->imported()) {
                nodep->v3error("Unsupported in C: Variable has same name as "
                               << LinkDotState::ಗಂಟು_ಪಠ್ಯ_Type(foundp->nodep()) << ": "
                               << nodep->prettyNameQ());
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (findvarp != nodep) {
                UINFO(4, "DupVar: " << nodep << " ;; " << foundp->nodep());
                UINFO(4, "    found  cur=se" << cvt_ಗಮ್ಯ_Hex(m_curSymp) << " ;; parent=se"
                                             << cvt_ಗಮ್ಯ_Hex(foundp->parentp()));
                ಆದರೆ (foundp->parentp() == m_curSymp  // Only when on same level
                    && !foundp->imported()) {  // and not from package
                    ಆದರೆ (VN_IS(m_curSymp->nodep(), Clocking)) {
                        nodep->v3error("Multiple clockvars with the same name not allowed");
                        ಹಿಂತಿರುಗಿ;
                    }
                    ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ nansiBad
                        = (((findvarp->isDeclTyped() || findvarp->isNet())
                            && (nodep->isDeclTyped() || nodep->isNet()))
                           || (findvarp->isIO() && nodep->isIO())  // e.g. !(output && output)
                           || findvarp->gotNansiType());  // e.g. int x && int x
                    ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ ansiBad
                        = findvarp->isAnsi() || nodep->isAnsi();  // dup illegal with ANSI
                    ಆದರೆ (ansiBad || nansiBad) {
                        ತಾರ್ಕಿಕ ansiWarn = ansiBad && !nansiBad;
                        ಆದರೆ (ansiWarn) {
                            ಸ್ಥಿರ ಪೂರ್ಣಾಂಕ s_didAnsiWarn = ಸುಳ್ಳು;
                            ಆದರೆ (s_didAnsiWarn++) ansiWarn = ಸುಳ್ಳು;
                        }
                        nodep->v3error("Duplicate declaration of signal: "
                                       << nodep->prettyNameQ() << '\n'
                                       << (ansiWarn ? nodep->warn_ಇನ್ನಷ್ಟು()
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
                    } ಇಲ್ಲದಿದ್ದರೆ {
                        findvarp->combineType(nodep);
                        findvarp->fileline()->modifyStateInherit(nodep->fileline());
                        findvarp->gotNansiType(ನಿಜ);
                        UASSERT_OBJ(nodep->subDTypep(), nodep, "Var has no type");
                        ಆದರೆ (nodep->subDTypep()->numeric().isSigned()
                            && !findvarp->subDTypep()->numeric().isSigned()) {
                            findvarp->subDTypep()->numeric(VSigning{ನಿಜ});
                        }
                        Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ varDtp = findvarp->subDTypep();
                        Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ otherDtp = nodep->subDTypep();
                        ಆದರೆ (varDtp
                            && (VN_IS(varDtp, BasicDType)
                                && VN_AS(varDtp, BasicDType)->implicit())) {
                            // Then have "input foo" and "real foo" so the
                            // dtype comes from the other side.
                            VL_DO_DANGLING(varDtp->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ(), varDtp);
                            findvarp->ಮಗು_DTypep(otherDtp->unlink_Fr_ಹಿಂಭಾಗ());
                        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!m_statep->forScopeCreation() && otherDtp && varDtp
                                   && !(VN_IS(otherDtp, BasicDType)
                                        && VN_AS(otherDtp, BasicDType)->implicit())) {
                            // otherDtp and varDtp both non-nullptr and neither are implicit
                            // Can't compare dtypes now as might contain parameters,
                            // defer to V3Width
                            Ast_Attr_ದ* ಸ್ಥಿರಾಂಕ newp
                                = ಹೊಸ Ast_Attr_ದ{otherDtp->fileline(), VAttrType::VAR_ಪೋರ್ಟ್_DTYPE,
                                                otherDtp->unlink_Fr_ಹಿಂಭಾಗ()};
                            findvarp->ಕೂಡಿಸು_Attrsp(newp);
                        }
                    }
                    VL_DO_DANGLING(nodep->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ(), nodep);
                } ಇಲ್ಲದಿದ್ದರೆ {
                    // User can disable the message at either point
                    ಆದರೆ (!(m_ftaskp && m_ftaskp->dpiImport())
                        && (!m_ftaskp || m_ftaskp != foundp->nodep())  // Not the function's
                                                                       // variable hiding function
                        && !nodep->fileline()->warnIsOff(V3_Error_ಸಂಕೇತ::VARHIDDEN)
                        && !foundp->nodep()->fileline()->warnIsOff(V3_Error_ಸಂಕೇತ::VARHIDDEN)) {
                        nodep->v3warn(VARHIDDEN,
                                      "Declaration of signal hides declaration in upper scope: "
                                          << nodep->prettyNameQ() << '\n'
                                          << nodep->warnContextPrimary() << '\n'
                                          << foundp->nodep()->warnOther()
                                          << "... Location of original declaration\n"
                                          << foundp->nodep()->warnContextSecondary());
                    }
                    ins = ನಿಜ;
                }
            }
            ಆದರೆ (ins) {
                ಆದರೆ (m_statep->forPrimary() && nodep->isGParam()
                    && VN_IS(m_modSymp->nodep(), Module)
                    && (m_statep->ಬೇರು_Entp()->nodep() == m_modSymp->parentp()->nodep())) {
                    // This is the toplevel module. Check for command line overwrites of parameters
                    // We first search if the parameter is overwritten and then replace it with a
                    // new value.
                    ಆದರೆ (v3Global.opt.hasParameter(nodep->name())) {
                        ಸ್ಥಿರಾಂಕ string svalue = v3Global.opt.parameter(nodep->name());
                        ಆದರೆ (AstConst* ಸ್ಥಿರಾಂಕ valuep
                            = AstConst::parseParamLiteral(nodep->fileline(), svalue)) {
                            UINFO(9, "       replace parameter " << nodep);
                            UINFO(9, "       with " << valuep);
                            ಆದರೆ (nodep->valuep()) ತಳ್ಳು_Deletep(nodep->valuep()->unlink_Fr_ಹಿಂಭಾಗ());
                            nodep->valuep(valuep);
                            // Do not warn ENUMVALUE when overriding an enum param from the cli.
                            nodep->fileline()->warnOff(V3_Error_ಸಂಕೇತ::ENUMVALUE, ನಿಜ);
                        }
                    }
                }
                m_statep->ಸೇರಿಸು_Sym(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
                ಆದರೆ (m_statep->forPrimary() && nodep->isGParam()) {
                    ++m_param_ಅಂಕೆ;
                    VSymEnt* ಸ್ಥಿರಾಂಕ symp
                        = m_statep->ಸೇರಿಸು_Sym(m_curSymp, "__paramNumber" + cvt_ಗಮ್ಯ_Str(m_param_ಅಂಕೆ),
                                              nodep, m_classOrPackagep);
                    symp->exported(ಸುಳ್ಳು);
                }
            }
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstTypedef* nodep) override {  // FindVisitor::
        UASSERT_OBJ(m_curSymp, nodep, "Typedef not under module/package/$unit");
        ಆದರೆ (VN_IS(m_classOrPackagep, Class)) nodep->isUnderClass(ನಿಜ);
        iterateChildren(nodep);
        m_statep->ಸೇರಿಸು_Sym(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstTypedefFwd* nodep) override {  // FindVisitor::
        UASSERT_OBJ(m_curSymp, nodep, "Typedef not under module/package/$unit");
        iterateChildren(nodep);
        // No need to insert, only the real typedef matters, but need to track for errors
        nodep->user1p(m_curSymp);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_UOr_Struct_DType* nodep) override {  // FindVisitor::
        UASSERT_OBJ(m_curSymp, nodep, "Struct/union dtype not under module/package/$unit");
        VL_RESTORER(m_curSymp);
        m_curSymp = m_statep->ಸೇರಿಸು_ಖಂಡ(m_curSymp, "__Vdtype" + cvt_ಗಮ್ಯ_Str(nodep->unique_ಅಂಕೆ()),
                                          nodep, m_classOrPackagep);
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstMemberDType* nodep) override {  // FindVisitor::
        iterateChildren(nodep);
        m_statep->ಸೇರಿಸು_Sym(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstParamTypeDType* nodep) override {  // FindVisitor::
        UASSERT_OBJ(m_curSymp, nodep, "Parameter type not under module/package/$unit");

        // Replace missing param types with provided hierarchical type params.
        ಆದರೆ (!m_hierParamsName.ಖಾಲಿಗಳು()) {
            ಆದರೆ (ಸ್ಥಿರಾಂಕ VSymEnt* ಸ್ಥಿರಾಂಕ typedefEntp = m_curSymp->ಕಂಡುಹಿಡಿ_Id_Fallback(m_hierParamsName)) {
                ಸ್ಥಿರಾಂಕ AstModule* modp = VN_CAST(typedefEntp->nodep(), Module);

                ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* stmtp = modp ? modp->stmtsp() : ಶೂನ್ಯಸೂಚಕ; stmtp;
                     stmtp = stmtp->nextp()) {
                    ಸ್ಥಿರಾಂಕ AstTypedef* tdefp = VN_CAST(stmtp, Typedef);

                    ಆದರೆ (tdefp && tdefp->name() == nodep->name() && m_statep->forPrimary()) {
                        UINFO(8, "Replacing type of" << nodep << "  -->with " << tdefp);
                        Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ newDtp = tdefp->ಮಗು_DTypep();
                        Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ oldDtp = nodep->ಮಗು_DTypep();

                        oldDtp->replaceWith(newDtp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು));
                        VL_DO_DANGLING(oldDtp->delete_ವೃಕ್ಷ(), oldDtp);
                    }
                }
            }
        }

        iterateChildren(nodep);
        m_statep->ಸೇರಿಸು_Sym(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
        ಆದರೆ (m_statep->forPrimary() && nodep->isGParam()) {
            ++m_param_ಅಂಕೆ;
            VSymEnt* ಸ್ಥಿರಾಂಕ symp = m_statep->ಸೇರಿಸು_Sym(
                m_curSymp, "__paramNumber" + cvt_ಗಮ್ಯ_Str(m_param_ಅಂಕೆ), nodep, m_classOrPackagep);
            symp->exported(ಸುಳ್ಳು);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCFunc* nodep) override {  // FindVisitor::
        // For dotted resolution, ignore all AstVars under functions, otherwise shouldn't exist
        UASSERT_OBJ(!m_statep->forScopeCreation(), nodep, "No CFuncs expected in tree yet");
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Enum_ಅಂಶ* nodep) override {  // FindVisitor::
        // EnumItem: Remember its name for later resolution
        iterateChildren(nodep);
        // Find under either a task or the module's vars
        ಸ್ಥಿರಾಂಕ VSymEnt* foundp = m_curSymp->ಕಂಡುಹಿಡಿ_Id_Fallback(nodep->name());
        ಆದರೆ (!foundp && m_modSymp && nodep->name() == m_modSymp->nodep()->name()) {
            foundp = m_modSymp;  // Conflicts with modname?
        }
        Ast_Enum_ಅಂಶ* ಸ್ಥಿರಾಂಕ findvarp = foundp ? VN_CAST(foundp->nodep(), Enum_ಅಂಶ) : ಶೂನ್ಯಸೂಚಕ;
        ತಾರ್ಕಿಕ ins = ಸುಳ್ಳು;
        ಆದರೆ (!foundp) {
            ins = ನಿಜ;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (findvarp != nodep) {
            UINFO(4, "DupVar: " << nodep << " ;; " << foundp);
            ಆದರೆ (foundp->parentp() == m_curSymp  // Only when on same level
                && !foundp->imported()) {  // and not from package
                nodep->v3error("Duplicate declaration of enum value: "
                               << nodep->prettyName() << '\n'
                               << nodep->warnContextPrimary() << '\n'
                               << foundp->nodep()->warnOther()
                               << "... Location of original declaration\n"
                               << foundp->nodep()->warnContextSecondary());
            } ಇಲ್ಲದಿದ್ದರೆ {
                // User can disable the message at either point
                ಆದರೆ (!nodep->fileline()->warnIsOff(V3_Error_ಸಂಕೇತ::VARHIDDEN)
                    && !foundp->nodep()->fileline()->warnIsOff(V3_Error_ಸಂಕೇತ::VARHIDDEN)) {
                    nodep->v3warn(VARHIDDEN,
                                  "Declaration of enum value hides declaration in upper scope: "
                                      << nodep->prettyName() << '\n'
                                      << nodep->warnContextPrimary() << '\n'
                                      << foundp->nodep()->warnOther()
                                      << "... Location of original declaration\n"
                                      << foundp->nodep()->warnContextSecondary());
                }
                ins = ನಿಜ;
            }
        }
        ಆದರೆ (ins) m_statep->ಸೇರಿಸು_Sym(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstPackageImport* nodep) override {  // FindVisitor::
        UINFO(4, "  Link: " << nodep);
        ಆದರೆ (!nodep->packagep()) ಹಿಂತಿರುಗಿ;  // Errored in V3LinkCells
        VSymEnt* ಸ್ಥಿರಾಂಕ srcp = m_statep->get_ಗಂಟು_Sym(nodep->packagep());
        ಆದರೆ (nodep->name() == "*") {
            ಆದರೆ (nodep->packagep() != v3Global.rootp()->stdPackagep()) {
                ಆದರೆ (m_curSymp == m_statep->dunitEntp()) {
                    nodep->v3warn(IMPORTSTAR,
                                  "'import::*' in $unit scope may pollute global namespace");
                }
            }
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!nodep->resolvedClassp()) {
            VSymEnt* ಸ್ಥಿರಾಂಕ impp = srcp->ಕಂಡುಹಿಡಿ_Id_Flat(nodep->name());
            ಆದರೆ (!impp) {
                nodep->v3error("Import object not found: " << nodep->prettyPkgNameQ());
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->resolvedClassp()) {
                UASSERT_OBJ(nodep->resolvedClassp()->classOrPackageSkipp() == impp->nodep(), nodep,
                            "Import should resolve to the same node");
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstClass* ಸ್ಥಿರಾಂಕ classp = VN_CAST(impp->nodep(), Class)) {
                nodep->resolvedClassp(
                    ಹೊಸ AstClassOrPackageRef{nodep->fileline(), classp->name(), classp, ಶೂನ್ಯಸೂಚಕ});
            }
        }
        m_curSymp->importFromPackage(m_statep->symsp(), srcp, nodep->name());
        UINFO(9, "    Link Done: " << nodep);
        // No longer needed, but can't delete until any multi-instantiated modules are expanded
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstPackageExport* nodep) override {  // FindVisitor::
        UINFO(9, "  Link: " << nodep);
        ಆದರೆ (!nodep->packagep()) ಹಿಂತಿರುಗಿ;  // Errored in V3LinkCells
        VSymEnt* ಸ್ಥಿರಾಂಕ srcp = m_statep->get_ಗಂಟು_Sym(nodep->packagep());
        ಆದರೆ (nodep->name() != "*") {
            VSymEnt* ಸ್ಥಿರಾಂಕ impp = srcp->ಕಂಡುಹಿಡಿ_Id_Flat(nodep->name());
            ಆದರೆ (!impp) {
                nodep->v3error("Export object not found: '" << nodep->packagep()->prettyName()
                                                            << "::" << nodep->prettyName() << "'");
            }
        }
        m_curSymp->exportFromPackage(m_statep->symsp(), srcp, nodep->name());
        UINFO(9, "    Link Done: " << nodep);
        // No longer needed, but can't delete until any multi-instantiated modules are expanded
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Package_Export_ನಕ್ಷತ್ರ_ನಕ್ಷತ್ರ* nodep) override {  // FindVisitor::
        UINFO(4, "  Link: " << nodep);
        m_curSymp->export_ನಕ್ಷತ್ರ_ನಕ್ಷತ್ರ(m_statep->symsp());
        // No longer needed, but can't delete until any multi-instantiated modules are expanded
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Foreach* nodep) override {  // FindVisitor::
        // Symbol table needs nodep->name() as the index variable's name
        VL_RESTORER(m_curSymp);
        {
            ++m_mod_With_ಅಂಕೆ;
            m_curSymp = m_statep->ಸೇರಿಸು_ಖಂಡ(m_curSymp, "__Vforeach" + cvt_ಗಮ್ಯ_Str(m_mod_With_ಅಂಕೆ),
                                              nodep, m_classOrPackagep);
            m_curSymp->fallbackp(VL_RESTORER_PREV(m_curSymp));
            ಗಾಗಿ (Ast_ಗಂಟು *nextp, *argp = nodep->headerp()->elementsp(); argp; argp = nextp) {
                nextp = argp->nextp();
                AstVar* argrefp = ಶೂನ್ಯಸೂಚಕ;
                ಆದರೆ (AstParseRef* ಸ್ಥಿರಾಂಕ parserefp = VN_CAST(argp, ParseRef)) {
                    // IEEE 1800-2023 12.7.3: foreach loop variable type shall be int (2-state)
                    // This might get changed in V3Width when types resolve (e.g., for assoc
                    // arrays)
                    argrefp = ಹೊಸ AstVar{parserefp->fileline(), VVarType::BLOCKTEMP,
                                         parserefp->name(), argp->ಕಂಡುಹಿಡಿ_Int_DType()};
                    argrefp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                    parserefp->replaceWith(argrefp);
                    VL_DO_DANGLING2(parserefp->delete_ವೃಕ್ಷ(), parserefp, argp);
                    // Insert argref's name into symbol table
                    m_statep->ಸೇರಿಸು_Sym(m_curSymp, argrefp->name(), argrefp, ಶೂನ್ಯಸೂಚಕ);
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ largrefp = VN_CAST(argp, Var)) {
                    argrefp = largrefp;
                    // Insert argref's name into symbol table
                    m_statep->ಸೇರಿಸು_Sym(m_curSymp, argrefp->name(), argrefp, ಶೂನ್ಯಸೂಚಕ);
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(argp, ಖಾಲಿಗಳು_2)) {
                } ಇಲ್ಲದಿದ್ದರೆ {
                    argp->v3error("'foreach' loop variable expects simple variable name");
                }
            }
            iterateChildren(nodep);
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstRandSequence* nodep) override {
        VL_RESTORER(m_curSymp);
        m_curSymp = m_statep->ಸೇರಿಸು_ಖಂಡ(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstRSProd* nodep) override {
        VL_RESTORER(m_curSymp);
        m_curSymp = m_statep->ಸೇರಿಸು_ಖಂಡ(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
        ಆದರೆ (nodep->fvarp())
            nodep->fvarp()->v3warn(E_UNSUPPORTED,
                                   "Unsupported: randsequence production function variable");
        // Mark formal ports as port-checked so the primary resolve pass does not
        // flag them with "does not appear in port list" -- V3RandSequence will
        // later move them onto the generated task as real input ports.
        ಗಾಗಿ (Ast_ಗಂಟು* itp = nodep->portsp(); itp; itp = itp->nextp()) {
            VN_AS(itp, Var)->user4(ನಿಜ);
        }
        iterateChildren(nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstWithParse* nodep) override {  // FindVisitor::
        // Change WITHPARSE(FUNCREF, equation) to FUNCREF(WITH(equation))
        Ast_ಗಂಟು_FTask_Ref* funcrefp = VN_CAST(nodep->funcrefp(), ಗಂಟು_FTask_Ref);
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstDot* ಸ್ಥಿರಾಂಕ dotp = VN_CAST(nodep->funcrefp(), Dot))
            funcrefp = VN_CAST(dotp->rhsp(), ಗಂಟು_FTask_Ref);
        UASSERT_OBJ(funcrefp, nodep, "'with' only can operate on a function/task");
        string name = funcrefp->name() == "randomize" ? "__Vrandwith_obj" : "item";
        File_ಸಾಲು* argFl = nodep->fileline();
        AstArg* ಸ್ಥಿರಾಂಕ argsp = funcrefp->argsp();
        ಆದರೆ (argsp) {
            argsp->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ();
            ಆದರೆ (funcrefp->name() != "randomize") {
                ಆದರೆ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ parserefp = VN_CAST(argsp->exprp(), ParseRef)) {
                    name = parserefp->name();
                    argFl = parserefp->fileline();
                } ಇಲ್ಲದಿದ್ದರೆ {
                    argsp->v3error("'with' function expects simple variable name");
                }
                ಆದರೆ (argsp->nextp()) {
                    argsp->nextp()->v3error("'with' function expects only up to one argument");
                }
                VL_DO_DANGLING(argsp->delete_ವೃಕ್ಷ(), argsp);
            }
        }
        // Type depends on the method used, let V3Width figure it out later
        // 'with (...) { ... }' constraint_block form is randomize-only
        // (IEEE 1800-2023 7.12 vs. 18.7); array methods take 'with (expr)'.
        ಆದರೆ (nodep->restricted() && funcrefp->name() != "randomize") {
            nodep->v3error("'with (...) { ... }' constraint block is only valid for"
                           " randomize() (IEEE 1800-2023 18.7)");
            funcrefp->ಕೂಡಿಸು_Argsp(argsp);
            nodep->replaceWith(nodep->funcrefp()->unlink_Fr_ಹಿಂಭಾಗ());
            VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
            ಹಿಂತಿರುಗಿ;
        }
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ restrictedRandomize = nodep->restricted();
        ಆದರೆ (nodep->exprsp() || nodep->constraintsp()
            || restrictedRandomize) {  // Else empty expression and pretend no "with"
            AstLambdaArgRef* ಸ್ಥಿರಾಂಕ ಸೂಚ್ಯಂಕ_Arg_Refp
                = ಹೊಸ AstLambdaArgRef{argFl, name + "__DOT__index", ನಿಜ};
            AstLambdaArgRef* ಸ್ಥಿರಾಂಕ ಮೌಲ್ಯ_Arg_Refp = ಹೊಸ AstLambdaArgRef{argFl, name, ಸುಳ್ಳು};
            AstWith* ಸ್ಥಿರಾಂಕ newp
                = ಹೊಸ AstWith{nodep->fileline(), ಸೂಚ್ಯಂಕ_Arg_Refp, ಮೌಲ್ಯ_Arg_Refp, ಶೂನ್ಯಸೂಚಕ};
            // Harvest the identifier_list into AstWith; grammar guarantees AstParseRef.
            ಆದರೆ (restrictedRandomize) {
                newp->restricted(ನಿಜ);
                ಆಗ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ itemp = nodep->exprsp()) {
                    newp->ಕೂಡಿಸು_Restricted_Name(VN_AS(itemp, ParseRef)->name());
                    itemp->unlink_Fr_ಹಿಂಭಾಗ();
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(itemp), itemp);
                }
            }
            Ast_ಗಂಟು* ಅಭಿವ್ಯಕ್ತಿ_Or_Constraintsp = ಶೂನ್ಯಸೂಚಕ;
            ಆದರೆ (nodep->exprsp()) ಅಭಿವ್ಯಕ್ತಿ_Or_Constraintsp = nodep->exprsp()->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ();
            ಆದರೆ (nodep->constraintsp())
                ಅಭಿವ್ಯಕ್ತಿ_Or_Constraintsp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(
                    ಅಭಿವ್ಯಕ್ತಿ_Or_Constraintsp, nodep->constraintsp()->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ());
            newp->ಕೂಡಿಸು_Exprp(ಅಭಿವ್ಯಕ್ತಿ_Or_Constraintsp);  // addExprp() tolerates nullptr
            funcrefp->withp(newp);
        }
        funcrefp->ಕೂಡಿಸು_Argsp(argsp);
        nodep->replaceWith(nodep->funcrefp()->unlink_Fr_ಹಿಂಭಾಗ());
        VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstWith* nodep) override {  // FindVisitor::
        // Symbol table needs nodep->name() as the index variable's name
        // Iteration will pickup the AstVar we made under AstWith
        VL_RESTORER(m_curSymp);
        ++m_mod_With_ಅಂಕೆ;
        m_curSymp = m_statep->ಸೇರಿಸು_ಖಂಡ(m_curSymp, "__Vwith" + cvt_ಗಮ್ಯ_Str(m_mod_With_ಅಂಕೆ), nodep,
                                          m_classOrPackagep);
        m_curSymp->fallbackp(VL_RESTORER_PREV(m_curSymp));
        UASSERT_OBJ(nodep->ಸೂಚ್ಯಂಕ_Arg_Refp(), nodep, "Missing lambda argref");
        UASSERT_OBJ(nodep->ಮೌಲ್ಯ_Arg_Refp(), nodep, "Missing lambda argref");
        // Insert argref's name into symbol table
        m_statep->ಸೇರಿಸು_Sym(m_curSymp, nodep->ಮೌಲ್ಯ_Arg_Refp()->name(), nodep->ಮೌಲ್ಯ_Arg_Refp(),
                            ಶೂನ್ಯಸೂಚಕ);
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCoverWith* nodep) override {  // FindVisitor::
        // The range list is outside the scope of the filter, which declares 'item'
        iterate_And_ಮುಂದಿನ_Null(nodep->subp());
        VL_RESTORER(m_curSymp);
        ++m_mod_With_ಅಂಕೆ;
        m_curSymp = m_statep->ಸೇರಿಸು_ಖಂಡ(m_curSymp, "__VcoverWith" + cvt_ಗಮ್ಯ_Str(m_mod_With_ಅಂಕೆ),
                                          nodep, m_classOrPackagep);
        m_curSymp->fallbackp(VL_RESTORER_PREV(m_curSymp));
        iterate_And_ಮುಂದಿನ_Null(nodep->itemp());
        iterate_And_ಮುಂದಿನ_Null(nodep->filterp());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCoverCrossWith* nodep) override {  // FindVisitor::
        // The selection, the 'matches' count, and the cross items' types, of their coverpoints'
        // expressions with any nested scopes, are outside the filter's scope, which declares the
        // items.  As visit(AstVar) would resolve a type in the scope declaring its variable, the
        // items, generated and unique, are inserted directly.
        iterate_And_ಮುಂದಿನ_Null(nodep->selectp());
        iterate_And_ಮುಂದಿನ_Null(nodep->countp());
        ಗಾಗಿ (AstVar* varp = nodep->varsp(); varp; varp = VN_AS(varp->nextp(), Var)) {
            iterate_And_ಮುಂದಿನ_Null(varp->ಮಗು_DTypep());
        }
        VL_RESTORER(m_curSymp);
        ++m_mod_With_ಅಂಕೆ;
        m_curSymp = m_statep->ಸೇರಿಸು_ಖಂಡ(m_curSymp, "__VcoverWith" + cvt_ಗಮ್ಯ_Str(m_mod_With_ಅಂಕೆ),
                                          nodep, m_classOrPackagep);
        m_curSymp->fallbackp(VL_RESTORER_PREV(m_curSymp));
        ಗಾಗಿ (AstVar* varp = nodep->varsp(); varp; varp = VN_AS(varp->nextp(), Var)) {
            m_statep->ಸೇರಿಸು_Sym(m_curSymp, varp->name(), varp, m_classOrPackagep);
        }
        iterate_And_ಮುಂದಿನ_Null(nodep->filterp());
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }  // FindVisitor::

    ಶೂನ್ಯ handleUnvisitedVirtIfaces() {
        Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ ತುದಿ = v3Global.rootp()->modulesp();
        m_curSymp = m_statep->get_ಗಂಟು_Sym(ತುದಿ);
        ಗಾಗಿ (AstIface* ifacep : m_virtIfaces) {
            ಆದರೆ (!ifacep->user4()) {  // virtual interface never assigned any actual interface
                // Create dummy cell to keep the virtual interface alive
                // (later stages assume that non-dead interface has associated cell)
                AstCell* ಸ್ಥಿರಾಂಕ ifacecellp = ಹೊಸ AstCell{ifacep->fileline(),
                                                        ifacep->fileline(),
                                                        "__VdummyInstOf" + ifacep->name(),
                                                        ifacep->name(),
                                                        ಶೂನ್ಯಸೂಚಕ,
                                                        ಶೂನ್ಯಸೂಚಕ,
                                                        ಶೂನ್ಯಸೂಚಕ};
                ifacecellp->modp(ifacep);
                ತುದಿ->ಕೂಡಿಸು_Stmtsp(ifacecellp);

                iterate(ifacecellp);
            }
        }
    }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    Link_Dot_ಕಂಡುಹಿಡಿ_Visitor(AstNetlist* rootp, LinkDotState* statep)
        : m_statep{statep} {
        UINFO(4, __FUNCTION__ << ": ");
        iterate(rootp);

        ಆದರೆ (!m_virtIfaces.ಖಾಲಿಗಳು()) handleUnvisitedVirtIfaces();
        ಗಾಗಿ (AstScope* ಸ್ಥಿರಾಂಕ scopep : m_scopes) scopep->modp()->ಕೂಡಿಸು_Stmtsp(scopep);
        ಆದರೆ (m_statep->forScopeCreation()) {
            V3Stats::ಕೂಡಿಸು_Stat("LinkDot, Scope visits", m_statScopeVisits);
        }
    }
    ~Link_Dot_ಕಂಡುಹಿಡಿ_Visitor() override = ಪೂರ್ವನಿಯೋಜಿತ;
};

//======================================================================

ವರ್ಗ Link_Dot_ಕಂಡುಹಿಡಿ_Iface_Visitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // NODE STATE
    //  *::user1p()             -> See LinkDotState

    // STATE - for current visit position (use VL_RESTORER)
    LinkDotState* ಸ್ಥಿರಾಂಕ m_statep;  // State to pass between visitors, including symbol table
    Ast_ಗಂಟು* m_declp = ಶೂನ್ಯಸೂಚಕ;  // Current declaring object that may soon contain IfaceRefDType

    // METHODS
    ಸ್ಥಿರಾಂಕ VSymEnt* ಕಂಡುಹಿಡಿ_Non_Decl_Sym(VSymEnt* symp, ಸ್ಥಿರಾಂಕ string& name) {
        // Find if there is a symbol of given name, ignoring the node that is declaring
        // it (m_declp) itself.  Thus if searching for "ifc" (an interface):
        //
        // module x;  // Finishes here and finds the typedef
        //   typedef foo ifc;
        //   ifc ifc;   // symp starts pointing here, but matches m_declp
        ಆಗ (symp) {
            ಸ್ಥಿರಾಂಕ VSymEnt* ಸ್ಥಿರಾಂಕ foundp = symp->ಕಂಡುಹಿಡಿ_Id_Flat(name);
            ಆದರೆ (foundp && foundp->nodep() != m_declp) ಹಿಂತಿರುಗಿ foundp;
            symp = symp->fallbackp();
        }
        ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
    }

    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(AstRefDType* nodep) override {  // FindIfaceVisitor::
        ಆದರೆ (m_statep->forPrimary() && !nodep->classOrPackagep()) {
            UINFO(9, "  FindIfc: " << nodep);
            // If under a var, ignore the var itself as might be e.g. "intf intf;"
            // Critical tests:
            //   t_interface_param_genblk.v  // Checks this does make interface
            //   t_interface_hidden.v  // Checks this doesn't making interface when hidden
            ಆದರೆ (m_statep->exists_ಗಂಟು_Sym(nodep)) {
                VSymEnt* symp = m_statep->get_ಗಂಟು_Sym(nodep);
                ಸ್ಥಿರಾಂಕ VSymEnt* foundp = ಕಂಡುಹಿಡಿ_Non_Decl_Sym(symp, nodep->name());
                Ast_ಗಂಟು* ಸಿಕ್ಕಿತು_Nodep = ಶೂನ್ಯಸೂಚಕ;
                // This: v4make test_regress/t/t_interface_param_genblk.py --debug
                // --debugi-V3LinkDot 9 Passes with this commented out:
                ಆದರೆ (foundp) ಸಿಕ್ಕಿತು_Nodep = foundp->nodep();
                ಆದರೆ (!ಸಿಕ್ಕಿತು_Nodep) ಸಿಕ್ಕಿತು_Nodep = m_statep->ಕಂಡುಹಿಡಿ_Module_Sym(nodep->name());
                ಆದರೆ (ಸಿಕ್ಕಿತು_Nodep) UINFO(9, "    Ifc foundNodep " << ಸಿಕ್ಕಿತು_Nodep);
                ಆದರೆ (AstIface* ಸ್ಥಿರಾಂಕ defp = VN_CAST(ಸಿಕ್ಕಿತು_Nodep, Iface)) {
                    // Must be found as module name, and not hidden/ by normal symbol (foundp)
                    AstIfaceRefDType* ಸ್ಥಿರಾಂಕ newp
                        = ಹೊಸ AstIfaceRefDType{nodep->fileline(), "", nodep->name()};
                    ಆದರೆ (nodep->paramsp())
                        newp->ಕೂಡಿಸು_Paramsp(nodep->paramsp()->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ());
                    newp->ifacep(defp);
                    newp->user1u(nodep->user1u());
                    UINFO(9, "    Resolved interface " << nodep << "  =>  " << defp);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                    ಹಿಂತಿರುಗಿ;
                }
            }
        }
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstVar* nodep) override {  // FindIfaceVisitor::
        VL_RESTORER(m_declp);
        m_declp = nodep;
        iterateChildren(nodep);
        AstIfaceRefDType* ಸ್ಥಿರಾಂಕ ifacerefp = LinkDotState::ifaceRefFromArray(nodep->subDTypep());
        ಆದರೆ (ifacerefp && m_statep->exists_ಗಂಟು_Sym(nodep)) {
            // Can't resolve until interfaces and modport names are
            // known; see notes at top
            UINFO(9, "  FindIfc Var IfaceRef " << ifacerefp);
            ಆದರೆ (!ifacerefp->isVirtual()) nodep->ಗಣ_Iface_Ref();
            VSymEnt* ಸ್ಥಿರಾಂಕ symp = m_statep->get_ಗಂಟು_Sym(nodep);
            m_statep->ಸೇರಿಸು_Iface_Var_Sym(symp);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }  // FindIfaceVisitor::

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    Link_Dot_ಕಂಡುಹಿಡಿ_Iface_Visitor(AstNetlist* rootp, LinkDotState* statep)
        : m_statep{statep} {
        UINFO(4, __FUNCTION__ << ": ");
        iterate(rootp);
    }
    ~Link_Dot_ಕಂಡುಹಿಡಿ_Iface_Visitor() override = ಪೂರ್ವನಿಯೋಜಿತ;
};

//======================================================================

ವರ್ಗ LinkDotParamVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // NODE STATE
    //  *::user1p()             -> See LinkDotState
    //  *::user2p()             -> See LinkDotState
    //  *::user4()              -> See LinkDotState

    // STATE
    LinkDotState* ಸ್ಥಿರಾಂಕ m_statep;  // State to pass between visitors, including symbol table
    Ast_ಗಂಟು_Module* m_modp = ಶೂನ್ಯಸೂಚಕ;  // Current module

    ಶೂನ್ಯ pin_Implicit_ಅಭಿವ್ಯಕ್ತಿ_Recurse(Ast_ಗಂಟು* nodep) {
        // Under a pin, Check interconnect expression for a pin reference or a concat.
        // Create implicit variable as needed
        ಆದರೆ (VN_IS(nodep, Dot)) {  // Not creating a simple implied type,
            // and implying something else would just confuse later errors
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(nodep, VarRef) || VN_IS(nodep, ParseRef)) {
            // To prevent user errors, we should only do single bit
            // implicit vars, however some netlists (MIPS) expect single
            // bit implicit wires to get created with range 0:0 etc.
            m_statep->implicit_Ok_ಕೂಡಿಸು(m_modp, nodep->name());
        }
        // These are perhaps a little too generous, as a SELect of siga[sigb]
        // perhaps shouldn't create an implicit variable.  But, we've warned...
        ಇಲ್ಲದಿದ್ದರೆ {
            ಆದರೆ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ refp = nodep->op1p()) pin_Implicit_ಅಭಿವ್ಯಕ್ತಿ_Recurse(refp);
            ಆದರೆ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ refp = nodep->op2p()) pin_Implicit_ಅಭಿವ್ಯಕ್ತಿ_Recurse(refp);
            ಆದರೆ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ refp = nodep->op3p()) pin_Implicit_ಅಭಿವ್ಯಕ್ತಿ_Recurse(refp);
            ಆದರೆ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ refp = nodep->op4p()) pin_Implicit_ಅಭಿವ್ಯಕ್ತಿ_Recurse(refp);
        }
        // Continue along a list (e.g. the terminals under AstImplicit), also after a reference
        ಆದರೆ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ refp = nodep->nextp()) pin_Implicit_ಅಭಿವ್ಯಕ್ತಿ_Recurse(refp);
    }

    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Type_ಕೋಷ್ಟಕ*) override {}  // ParamVisitor::
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Module* nodep) override {  // ParamVisitor::
        UINFO(5, "   " << nodep);
        ಆದರೆ ((nodep->dead() || !nodep->user4()) && !nodep->hierParams()) {
            UINFO(4, "Mark dead module " << nodep);
            UASSERT_OBJ(!m_statep->forScopeCreation(), nodep,
                        "Dead module persisted past where should have removed");
            // Don't remove now, because we may have a tree of
            // parameterized modules with VARXREFs into the deleted module
            // region.  V3Dead should cleanup.
            // Downstream visitors up until V3Dead need to check for nodep->dead.
            nodep->dead(ನಿಜ);
            ಹಿಂತಿರುಗಿ;
        }
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstPin* nodep) override {  // ParamVisitor::
        // Pin: Link to submodule's port
        // Deal with implicit definitions - do before Resolve visitor as may
        // be referenced above declaration
        ಆದರೆ (nodep->exprp()  // Else specifically unconnected pin
            && !nodep->svImplicit()) {  // SV 19.11.3: .name pins don't allow implicit decls
            pin_Implicit_ಅಭಿವ್ಯಕ್ತಿ_Recurse(nodep->exprp());
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstDefParam* nodep) override {  // ParamVisitor::
        iterateChildren(nodep);
        nodep->v3warn(DEFPARAM, "defparam is deprecated (IEEE 1800-2023 C.4.1)\n"
                                    << nodep->warn_ಇನ್ನಷ್ಟು()
                                    << "... Suggest use instantiation with #(."
                                    << nodep->prettyName() << "(...etc...))");
        ತಾರ್ಕಿಕ hasPartSelect = ಸುಳ್ಳು;
        ಸ್ಥಿರಾಂಕ string ಮಾರ್ಗ = ಹೊರತೆಗೆ_Dotted_ಮಾರ್ಗ(nodep->pathp(), hasPartSelect);
        UASSERT_OBJ(!hasPartSelect && !ಮಾರ್ಗ.ಖಾಲಿಗಳು(), nodep, "Unexpected defparam path shape");
        string baddot;
        VSymEnt* okSymp = ಶೂನ್ಯಸೂಚಕ;
        VSymEnt* ಸ್ಥಿರಾಂಕ foundp = m_statep->ಕಂಡುಹಿಡಿ_Dotted(
            nodep->fileline(), m_statep->get_ಗಂಟು_Sym(nodep), ಮಾರ್ಗ, baddot, okSymp, ನಿಜ);
        AstCell* ಸ್ಥಿರಾಂಕ cellp = foundp ? VN_AS(foundp->nodep(), Cell) : ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (!cellp) {
            nodep->v3error("In defparam, instance " << ಮಾರ್ಗ << " never declared");
        } ಇಲ್ಲದಿದ್ದರೆ {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ exprp = nodep->rhsp()->unlink_Fr_ಹಿಂಭಾಗ();
            UINFO(9, "Defparam cell " << ಮಾರ್ಗ << "." << nodep->name() << " attach-to " << cellp
                                      << "  <= " << exprp);
            // Don't need to check the name of the defparam exists.  V3Param does.
            AstPin* ಸ್ಥಿರಾಂಕ pinp = ಹೊಸ AstPin{nodep->fileline(),
                                            -1,  // Pin# not relevant
                                            nodep->name(), exprp};
            pinp->param(ನಿಜ);
            pinp->param_ಮಾರ್ಗ(ಮಾರ್ಗ);
            cellp->ಕೂಡಿಸು_Paramsp(pinp);
        }
        VL_DO_DANGLING(nodep->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ(), nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಪೋರ್ಟ್* nodep) override {  // ParamVisitor::
        // Port: Stash the pin number
        // Need to set pin numbers after varnames are created
        // But before we do the final resolution based on names
        VSymEnt* ಸ್ಥಿರಾಂಕ foundp = m_statep->get_ಗಂಟು_Sym(m_modp)->ಕಂಡುಹಿಡಿ_Id_Flat(nodep->name());
        AstVar* ಸ್ಥಿರಾಂಕ refp = foundp ? VN_CAST(foundp->nodep(), Var) : ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (!foundp) {
            nodep->v3error(
                "Input/output/inout declaration not found for port: " << nodep->prettyNameQ());
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!refp || (!refp->isIO() && !refp->isIfaceRef())) {
            nodep->v3error("Pin is not an in/out/inout/interface: " << nodep->prettyNameQ());
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಆದರೆ (refp->user4()) {
                nodep->v3error("Duplicate declaration of port: "
                               << nodep->prettyNameQ() << '\n'
                               << nodep->warnContextPrimary() << '\n'
                               << refp->warnOther() << "... Location of original declaration\n"
                               << refp->warnContextSecondary());
            }
            refp->user4(ನಿಜ);
            VSymEnt* ಸ್ಥಿರಾಂಕ symp = m_statep->ಸೇರಿಸು_Sym(m_statep->get_ಗಂಟು_Sym(m_modp),
                                                      "__pinNumber" + cvt_ಗಮ್ಯ_Str(nodep->pin_ಅಂಕೆ()),
                                                      refp, ಶೂನ್ಯಸೂಚಕ /*classOrPackagep*/);
            symp->exported(ಸುಳ್ಳು);
            refp->pin_ಅಂಕೆ(nodep->pin_ಅಂಕೆ());
            // Put the variable where the port is, so that variables stay
            // in pin number sorted order. Otherwise hierarchical or JSON
            // may botch by-position instances.
            nodep->ಕೂಡಿಸು_ಇಲ್ಲಿ_This_As_ಮುಂದಿನ(refp->unlink_Fr_ಹಿಂಭಾಗ());
        }
        // Ports not needed any more
        VL_DO_DANGLING(nodep->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ(), nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ನಿಯೋಜಿಸು_W* nodep) override {  // ParamVisitor::
        // Deal with implicit definitions
        // We used to nodep->allowImplicit() here, but it turns out
        // normal "assigns" can also make implicit wires.  Yuk.
        pin_Implicit_ಅಭಿವ್ಯಕ್ತಿ_Recurse(nodep->lhsp());
        iterateChildren(nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstImplicit* nodep) override {  // ParamVisitor::
        // Unsupported gates need implicit creation
        pin_Implicit_ಅಭಿವ್ಯಕ್ತಿ_Recurse(nodep->exprsp());
        // We're done with implicit gates
        VL_DO_DANGLING(nodep->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ(), nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstClassOrPackageRef* nodep) override {  // ParamVisitor::
        ಆದರೆ (ಸ್ವಯಂಚಾಲಿತ* ಸ್ಥಿರಾಂಕ fwdp = VN_CAST(nodep->classOrPackageNodep(), TypedefFwd)) {
            // Relink forward definitions to the "real" definition
            VSymEnt* ಸ್ಥಿರಾಂಕ foundp = m_statep->get_ಗಂಟು_Sym(fwdp)->ಕಂಡುಹಿಡಿ_Id_Fallback(fwdp->name());
            ಆದರೆ (foundp && (VN_IS(foundp->nodep(), Class) || VN_IS(foundp->nodep(), Package))) {
                nodep->classOrPackagep(VN_AS(foundp->nodep(), ಗಂಟು_Module));
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (foundp && VN_IS(foundp->nodep(), ParamTypeDType)) {
                UASSERT(m_statep->forPrimary(), "Param types should have been resolved");
                nodep->classOrPackageNodep(VN_AS(foundp->nodep(), ParamTypeDType));
            } ಇಲ್ಲದಿದ್ದರೆ {
                ಆದರೆ (foundp) UINFO(1, "found nodep = " << foundp->nodep());
                nodep->v3error(
                    "Forward typedef used as class/package does not resolve to class/package: "
                    << nodep->prettyNameQ() << '\n'
                    << nodep->warnContextPrimary() << '\n'
                    << (foundp ? nodep->warn_ಇನ್ನಷ್ಟು() + "... Object with matching name\n"
                                     + foundp->nodep()->warnContextSecondary()
                               : ""));
            }
        }
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstPull* nodep) override {  // ParamVisitor::
        // Deal with implicit definitions
        // We used to nodep->allowImplicit() here, but it turns out
        // normal "assigns" can also make implicit wires.  Yuk.
        pin_Implicit_ಅಭಿವ್ಯಕ್ತಿ_Recurse(nodep->lhsp());
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstTypedefFwd* nodep) override {  // ParamVisitor::
        VSymEnt* ಸ್ಥಿರಾಂಕ foundp = m_statep->get_ಗಂಟು_Sym(nodep)->ಕಂಡುಹಿಡಿ_Id_Fallback(nodep->name());
        ಆದರೆ (foundp) {
            // If the typedef was earlier in source order (tokenNum), then remember that earlier
            // point to avoid error something wasn't declared
            // Might be forward declaring something odd (with declTokenNumSetMin not implemented,
            // but should be harmless to ignore as this is just for error detection
            foundp->nodep()->decl_Token_ಅಂಕೆ_ಗಣ_Min(nodep->fileline()->token_ಅಂಕೆ());
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (v3Global.opt.pedantic()) {
            // We only check it was ever really defined in pedantic mode, as it
            // might have been in a header file referring to a module we never
            // needed so there are false positives
            nodep->v3error(
                "Forward typedef unused or does not resolve to a data type (IEEE 1800-2023 6.18): "
                << nodep->prettyNameQ());
        }
        // We only needed the forward declaration in order to parse correctly.
        // Delete later as may be ClassOrPackageRef's still pointing to it
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlink_Fr_ಹಿಂಭಾಗ()), nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }  // ParamVisitor::

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    LinkDotParamVisitor(AstNetlist* rootp, LinkDotState* statep)
        : m_statep{statep} {
        UINFO(4, __FUNCTION__ << ": ");
        iterate(rootp);
    }
    ~LinkDotParamVisitor() override = ಪೂರ್ವನಿಯೋಜಿತ;
};

//======================================================================

ವರ್ಗ LinkDotScopeVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {

    // STATE
    LinkDotState* ಸ್ಥಿರಾಂಕ m_statep;  // State to pass between visitors, including symbol table
    ಸ್ಥಿರಾಂಕ AstScope* m_scopep = ಶೂನ್ಯಸೂಚಕ;  // The current scope
    VSymEnt* m_modSymp = ಶೂನ್ಯಸೂಚಕ;  // Symbol entry for current module
    // Deferred AliasScope processing - must be done outer-to-inner for correct alias resolution
    std::ಸದಿಶ<std::pair<AstAliasScope*, VSymEnt*>> m_deferredAliasScopes;

    // METHODS
ಸಾರ್ವಜನಿಕ:
    // getAlias with setVarAlias and setIfaceAlias implement disjoint-set data structure.
    // This algorithm is needed for the case when multiple alias statements
    // reference partially the same variables.
    ಸ್ಥಿರ AstVarScope* getAlias(AstVarScope* ಸ್ಥಿರಾಂಕ vscp) {
        ಆದರೆ (vscp->user2p() && vscp != vscp->user2p()) {
            AstVarScope* ಸ್ಥಿರಾಂಕ aliasp = getAlias(VN_AS(vscp->user2p(), VarScope));
            vscp->user2p(aliasp);
            ಹಿಂತಿರುಗಿ aliasp;
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಹಿಂತಿರುಗಿ vscp;
        }
    }

ಖಾಸಗಿ:
    // Alias an interface reference to the interface it is connected to
    ಶೂನ್ಯ ಗಣ_Iface_Alias(AstVarScope* ಸ್ಥಿರಾಂಕ vscp, AstVarScope* ಸ್ಥಿರಾಂಕ aliasp) {
        UASSERT_OBJ(vscp->varp()->isIfaceRef(), vscp, "Interface alias of non-interface");
        UASSERT_OBJ(aliasp->varp()->isIfaceRef(), aliasp, "Interface alias of non-interface");
        getAlias(vscp)->user2p(getAlias(aliasp));
    }

    // Alias two variables. An output port in the lower scope survives, unless the other
    // must keep its own storage.
    ಶೂನ್ಯ ಗಣ_Var_Alias(AstVarScope* ap, AstVarScope* bp) {
        UASSERT_OBJ(!ap->varp()->isIfaceRef(), ap, "Variable alias of interface");
        UASSERT_OBJ(!bp->varp()->isIfaceRef(), bp, "Variable alias of interface");
        ap = getAlias(ap);
        bp = getAlias(bp);
        ಆದರೆ (ap == bp) ಹಿಂತಿರುಗಿ;
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ aWins = [&]() {
            ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ aVarp = ap->varp();
            ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ bVarp = bp->varp();
            // A primary IO is accessed directly by the user of the model, so must survive
            UASSERT_OBJ(!aVarp->isPrimaryIO() || !bVarp->isPrimaryIO(), ap,
                        "Alias of two primary IOs");
            ಆದರೆ (bVarp->isPrimaryIO()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            ಆದರೆ (aVarp->isPrimaryIO()) ಹಿಂತಿರುಗಿ ನಿಜ;
            // A forced or public variable must keep its own storage. If both are, either works.
            ಆದರೆ (aVarp->isForced() != bVarp->isForced()) ಹಿಂತಿರುಗಿ aVarp->isForced();
            ಆದರೆ (aVarp->is_Sig_ಬಹಿರಂಗ() != bVarp->is_Sig_ಬಹಿರಂಗ()) ಹಿಂತಿರುಗಿ aVarp->is_Sig_ಬಹಿರಂಗ();
            // Otherwise prefer an output port in the lower scope. This enables better V3Combine
            // due to having fewer upward hierarchical references (which prevent combining).
            ಆದರೆ (aVarp->direction() != VDirection::OUTPUT) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            // Is 'ap' strictly below 'bp' in the hierarchy
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ AstScope* scopep = ap->scopep()->aboveScopep(); scopep;
                 scopep = scopep->aboveScopep()) {
                ಆದರೆ (scopep == bp->scopep()) ಹಿಂತಿರುಗಿ ನಿಜ;
            }
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        }();
        ಆದರೆ (aWins) {
            bp->user2p(ap);
        } ಇಲ್ಲದಿದ್ದರೆ {
            ap->user2p(bp);
        }
    }

    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(AstNetlist* nodep) override {  // ScopeVisitor::
        // Recurse..., backward as must do packages before using packages
        iterateChildrenBackwardsConst(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstScope* nodep) override {  // ScopeVisitor::
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
        ಆದರೆ (AstIface* ಸ್ಥಿರಾಂಕ ifacep = VN_CAST(nodep->modp(), Iface)) {
            m_statep->ಸೇರಿಸು_Iface_Mod_Sym(ifacep, m_modSymp);
        }
        m_scopep = nodep;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstVarScope* nodep) override {  // ScopeVisitor::
        ಆದರೆ (!nodep->varp()->isFuncLocal() && !nodep->varp()->isClassMember()) {
            VSymEnt* ಸ್ಥಿರಾಂಕ varSymp
                = m_statep->ಸೇರಿಸು_Sym(m_modSymp, nodep->varp()->name(), nodep, ಶೂನ್ಯಸೂಚಕ);
            ಆದರೆ (nodep->varp()->isIfaceRef()) {
                AstIfaceRefDType* ಸ್ಥಿರಾಂಕ dtypep
                    = LinkDotState::ifaceRefFromArray(nodep->varp()->dtypep());
                UASSERT_OBJ(dtypep, nodep, "Non AstIfaceRefDType on isIfaceRef() var");
                ಆದರೆ (nodep->varp()->is_Iface_ಪೋಷಕ()) {
                    UINFO(9, "Iface parent ref var " << nodep->varp()->name() << " " << nodep);
                    // Find the interface cell the var references
                    UINFO(9, "Iface parent dtype " << dtypep);
                    ಸ್ಥಿರಾಂಕ string ifcellname = dtypep->cellName();
                    string baddot;
                    VSymEnt* okSymp;
                    VSymEnt* cellSymp = m_statep->ಕಂಡುಹಿಡಿ_Dotted(nodep->fileline(), m_modSymp,
                                                             ifcellname, baddot, okSymp, ಸುಳ್ಳು);
                    UASSERT_OBJ(
                        cellSymp, nodep,
                        "No symbol for interface instance: " << nodep->prettyNameQ(ifcellname));
                    UINFO(5, "       Found interface instance: se" << cvt_ಗಮ್ಯ_Hex(cellSymp) << " "
                                                                   << cellSymp->nodep());
                    ಆದರೆ (dtypep->modportName() != "") {
                        // Look up the modport within the interface cell's symbol table
                        VSymEnt* ಸ್ಥಿರಾಂಕ mpSymp = cellSymp->ಕಂಡುಹಿಡಿ_Id_Fallback(dtypep->modportName());
                        UASSERT_OBJ(mpSymp, nodep,
                                    "No symbol for interface modport: "
                                        << nodep->prettyNameQ(dtypep->modportName()));
                        cellSymp = mpSymp;
                        UINFO(5, "       Found modport cell: se" << cvt_ಗಮ್ಯ_Hex(cellSymp) << " "
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
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_FTask* nodep) override {  // ScopeVisitor::
        VSymEnt* ಸ್ಥಿರಾಂಕ symp = m_statep->ಸೇರಿಸು_ಖಂಡ(m_modSymp, nodep->name(), nodep, ಶೂನ್ಯಸೂಚಕ);
        symp->fallbackp(m_modSymp);
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Foreach* nodep) override {  // ScopeVisitor::
        VSymEnt* ಸ್ಥಿರಾಂಕ symp = m_statep->ಸೇರಿಸು_ಖಂಡ(m_modSymp, nodep->name(), nodep, ಶೂನ್ಯಸೂಚಕ);
        symp->fallbackp(m_modSymp);
        // No recursion, we don't want to pick up variables
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstConstraintForeach* nodep) override {  // ScopeVisitor::
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstWith* nodep) override {  // ScopeVisitor::
        VSymEnt* ಸ್ಥಿರಾಂಕ symp = m_statep->ಸೇರಿಸು_ಖಂಡ(m_modSymp, nodep->name(), nodep, ಶೂನ್ಯಸೂಚಕ);
        symp->fallbackp(m_modSymp);
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstAlias* nodep) override {  // ScopeVisitor::
        // Track aliases
        UINFOTREE(9, nodep, "", "alias");
        AstVarScope* aliasVscp = ಶೂನ್ಯಸೂಚಕ;
        ಗಾಗಿ (Ast_ಗಂಟು* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
            AstVarScope* vscp = ಶೂನ್ಯಸೂಚಕ;
            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstVarRef* ಸ್ಥಿರಾಂಕ refp = VN_CAST(itemp, VarRef)) {
                vscp = refp->varScopep();
            } ಇಲ್ಲದಿದ್ದರೆ {
                // Reaches into the scope of an instance, look it up by name
                ಸ್ಥಿರಾಂಕ AstVarXRef* ಸ್ಥಿರಾಂಕ xrefp = VN_AS(itemp, VarXRef);
                ಸ್ಥಿರಾಂಕ string scopename = xrefp->dotted() + "." + xrefp->name();
                string baddot;
                VSymEnt* okSymp;
                VSymEnt* ಸ್ಥಿರಾಂಕ symp = m_statep->ಕಂಡುಹಿಡಿ_Dotted(xrefp->fileline(), m_modSymp, scopename,
                                                           baddot, okSymp, ಸುಳ್ಳು);
                UASSERT_OBJ(symp, nodep, "No symbol for alias item: " << scopename);
                vscp = VN_CAST(symp->nodep(), VarScope);
            }
            UASSERT_OBJ(vscp, nodep, "VarScope unset");
            ಆದರೆ (aliasVscp) {
                ಗಣ_Var_Alias(aliasVscp, vscp);
            } ಇಲ್ಲದಿದ್ದರೆ {
                aliasVscp = vscp;
            }
        }
        iterateChildren(nodep);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlink_Fr_ಹಿಂಭಾಗ()), nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstAliasScope* nodep) override {  // ScopeVisitor::
        // Defer AliasScope processing - must process outer scopes before inner ones
        // so that nested interface port alias resolution works correctly
        UINFO(5, "ALIASSCOPE (deferred) " << nodep);
        m_deferredAliasScopes.emplace_ಹಿಂಭಾಗ(nodep, m_modSymp);
    }
    ಶೂನ್ಯ processAliasScope(AstAliasScope* nodep, VSymEnt* modSymp) {
        UINFO(5, "ALIASSCOPE  " << nodep);
        UINFOTREE(9, nodep, "", "avs");
        VSymEnt* rhsSymp;
        {
            AstVarRef* ಸ್ಥಿರಾಂಕ refp = VN_CAST(nodep->rhsp(), VarRef);
            AstVarXRef* ಸ್ಥಿರಾಂಕ xrefp = VN_CAST(nodep->rhsp(), VarXRef);
            UASSERT_OBJ(refp || xrefp, nodep,
                        "Unsupported: Non Var(X)Ref attached to interface pin");
            string inl
                = ((xrefp && xrefp->inlinedDots().ಗಾತ್ರ_2()) ? (xrefp->inlinedDots() + "__DOT__")
                                                          : "");
            ಸ್ಥಿರಾಂಕ string dotted_ಮಾರ್ಗ
                = (xrefp && !xrefp->dotted().ಖಾಲಿಗಳು()) ? (xrefp->dotted() + ".") : "";
            VSymEnt* symp = ಶೂನ್ಯಸೂಚಕ;
            string scopename;
            ಆಗ (!symp) {
                scopename = refp ? refp->name()
                                 : (inl.ಗಾತ್ರ_2() ? (inl + dotted_ಮಾರ್ಗ + xrefp->name())
                                               : (dotted_ಮಾರ್ಗ + xrefp->name()));
                string baddot;
                VSymEnt* okSymp;
                symp = m_statep->ಕಂಡುಹಿಡಿ_Dotted(nodep->rhsp()->fileline(), modSymp, scopename, baddot,
                                            okSymp, ನಿಜ);
                ಆದರೆ (inl == "") ನಿಲ್ಲಿಸು;
                inl = LinkDotState::remove_ಕೊನೆಯ_Inline_Scope(inl);
            }
            UASSERT_OBJ(symp, nodep, "No symbol for interface alias rhs");
            UINFO(5, "       Found a linked scope RHS: " << scopename << "  se" << cvt_ಗಮ್ಯ_Hex(symp)
                                                         << " " << symp->nodep());
            rhsSymp = symp;
        }
        VSymEnt* lhsSymp;
        {
            ಸ್ಥಿರಾಂಕ AstVarXRef* ಸ್ಥಿರಾಂಕ xrefp = VN_CAST(nodep->lhsp(), VarXRef);
            ಸ್ಥಿರಾಂಕ AstVarRef* ಸ್ಥಿರಾಂಕ refp = VN_CAST(nodep->lhsp(), VarRef);

            UASSERT_OBJ(refp || xrefp, nodep,
                        "Unsupported: Non Var(X)Ref attached to interface pin");
            ಸ್ಥಿರಾಂಕ string scopename
                = refp ? refp->varp()->name() : xrefp->dotted() + "." + xrefp->name();
            string baddot;
            VSymEnt* okSymp;
            VSymEnt* ಸ್ಥಿರಾಂಕ symp = m_statep->ಕಂಡುಹಿಡಿ_Dotted(nodep->lhsp()->fileline(), modSymp,
                                                       scopename, baddot, okSymp, ಸುಳ್ಳು);
            UASSERT_OBJ(symp, nodep, "No symbol for interface alias lhs");
            UINFO(5, "       Found a linked scope LHS: " << scopename << "  se" << cvt_ಗಮ್ಯ_Hex(symp)
                                                         << " " << symp->nodep());
            lhsSymp = symp;
        }
        // Remember the alias - can't do it yet because we may have additional symbols to be added,
        // or maybe an alias of an alias
        m_statep->ಸೇರಿಸು_Scope_Alias(LinkDotState::SAMN_IFTOP, lhsSymp, rhsSymp);
        AstVarScope* ಸ್ಥಿರಾಂಕ lhsVscp = VN_CAST(lhsSymp->nodep(), VarScope);
        AstVarScope* ಸ್ಥಿರಾಂಕ rhsVscp = VN_CAST(rhsSymp->nodep(), VarScope);
        UASSERT_OBJ(lhsVscp && rhsVscp, nodep, "Interface alias missing variable scope");
        ಗಣ_Iface_Alias(lhsVscp, rhsVscp);
        // We have stored the link, we don't need these any more
        VL_DO_DANGLING(nodep->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ(), nodep);
    }
    ಶೂನ್ಯ processDeferredAliasScopes() {
        // Sort by hierarchy depth (shallower first) so outer aliases are resolved before inner
        // Pre-compute depth map to avoid O(N log N * D) complexity in sort comparisons
        std::unordered_map<VSymEnt*, ಪೂರ್ಣಾಂಕ> ಆಳ_Map;
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& pair : m_deferredAliasScopes) {
            VSymEnt* ಸ್ಥಿರಾಂಕ symp = pair.ಎರಡನೇ;
            ಆದರೆ (ಆಳ_Map.ಕಂಡುಹಿಡಿ(symp) == ಆಳ_Map.ಕೊನೆ()) {
                ಪೂರ್ಣಾಂಕ ಆಳ = 0;
                ಗಾಗಿ (VSymEnt* p = symp; p; p = p->parentp()) ++ಆಳ;
                ಆಳ_Map[symp] = ಆಳ;
            }
        }
        std::stable_ವಿಂಗಡಿಸು(m_deferredAliasScopes.begin(), m_deferredAliasScopes.ಕೊನೆ(),
                         [&ಆಳ_Map](ಸ್ಥಿರಾಂಕ std::pair<AstAliasScope*, VSymEnt*>& a,
                                     ಸ್ಥಿರಾಂಕ std::pair<AstAliasScope*, VSymEnt*>& b) {
                             ಹಿಂತಿರುಗಿ ಆಳ_Map.at(a.ಎರಡನೇ) < ಆಳ_Map.at(b.ಎರಡನೇ);
                         });
        // Process in sorted order
        ಗಾಗಿ (ಸ್ವಯಂಚಾಲಿತ& pair : m_deferredAliasScopes) { processAliasScope(pair.ಮೊದಲ, pair.ಎರಡನೇ); }
        m_deferredAliasScopes.clear();
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Gen* nodep) override {  // ScopeVisitor::  // LCOV_EXCL_LINE
        nodep->v3fatalSrc("Generate constructs should have been reduced out");
    }
    // For speed, don't recurse things that can't have scope
    // Note we allow AstNodeStmt's as generates may be under them
    ಶೂನ್ಯ ಭೇಟಿ(AstCell*) override {}  // ScopeVisitor::
    ಶೂನ್ಯ ಭೇಟಿ(AstVar*) override {}  // ScopeVisitor::
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }  // ScopeVisitor::

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    LinkDotScopeVisitor(AstNetlist* rootp, LinkDotState* statep)
        : m_statep{statep} {
        UINFO(4, __FUNCTION__ << ": ");
        iterate(rootp);
        // Process deferred AliasScopes in outer-to-inner order
        processDeferredAliasScopes();
    }
    ~LinkDotScopeVisitor() override = ಪೂರ್ವನಿಯೋಜಿತ;
};

//======================================================================

// Iterate an interface to resolve modports
ವರ್ಗ LinkDotIfaceVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // STATE
    LinkDotState* ಸ್ಥಿರಾಂಕ m_statep;  // State to pass between visitors, including symbol table
    VSymEnt* m_curSymp;  // Symbol Entry for current table, where to lookup/insert

    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(AstModport* nodep) override {  // IfaceVisitor::
        // Modport: Remember its name for later resolution
        UINFO(5, "   fiv: " << nodep);
        VL_RESTORER(m_curSymp);
        {
            // Create symbol table for the vars
            m_curSymp = m_statep->ಸೇರಿಸು_ಖಂಡ(m_curSymp, nodep->name(), nodep, ಶೂನ್ಯಸೂಚಕ);
            m_curSymp->fallbackp(VL_RESTORER_PREV(m_curSymp));
            iterateChildren(nodep);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstModportFTaskRef* nodep) override {  // IfaceVisitor::
        UINFO(5, "   fif: " << nodep);
        iterateChildren(nodep);
        VSymEnt* ಸ್ಥಿರಾಂಕ symp = m_curSymp->ಕಂಡುಹಿಡಿ_Id_Fallback(nodep->name());
        ಆದರೆ (!symp) {
            nodep->v3error("Modport item not found: " << nodep->prettyNameQ());
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (Ast_ಗಂಟು_FTask* ಸ್ಥಿರಾಂಕ ftaskp = VN_CAST(symp->nodep(), ಗಂಟು_FTask)) {
            // Make symbol under modport that points at the _interface_'s var, not the modport.
            nodep->ftaskp(ftaskp);
            VSymEnt* ಸ್ಥಿರಾಂಕ subSymp
                = m_statep->ಸೇರಿಸು_Sym(m_curSymp, nodep->name(), ftaskp, ಶೂನ್ಯಸೂಚಕ /*package*/);
            m_statep->ಸೇರಿಸು_Scope_Alias(LinkDotState::SAMN_MODPORT, subSymp, symp);
        } ಇಲ್ಲದಿದ್ದರೆ {
            nodep->v3error("Modport item is not a function/task: " << nodep->prettyNameQ());
        }
        ಆದರೆ (m_statep->forScopeCreation()) {
            // Done with AstModportFTaskRef.
            // Delete to prevent problems if we dead-delete pointed to ftask
            nodep->unlink_Fr_ಹಿಂಭಾಗ();
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        }
    }

    // Helper to resolve remaining path through a nested interface
    // When findDotted() partially matches (okSymp set, baddot non-empty),
    // this follows the interface type to resolve the remaining path.
    // Returns the resolved symbol, or nullptr if not found.
    // On success, clears baddot; on partial match of multi-level path, updates baddot.
    VSymEnt* resolve_Nested_Interface_ಮಾರ್ಗ(File_ಸಾಲು* fl, VSymEnt* okSymp, string& baddot) {
        ಆದರೆ (!okSymp || baddot.ಖಾಲಿಗಳು()) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;

        ಸ್ಥಿರ constexpr ಪೂರ್ಣಾಂಕ MAX_NESTING_ಆಳ = 64;
        VSymEnt* curOkSymp = okSymp;

        ಗಾಗಿ (ಪೂರ್ಣಾಂಕ ಆಳ = 0; ಆಳ < MAX_NESTING_ಆಳ; ++ಆಳ) {
            // Try to get interface from the partially-matched symbol
            AstIface* ifacep = ಶೂನ್ಯಸೂಚಕ;
            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstCell* ಸ್ಥಿರಾಂಕ cellp = VN_CAST(curOkSymp->nodep(), Cell)) {
                ifacep = VN_CAST(cellp->modp(), Iface);
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ varp = VN_CAST(curOkSymp->nodep(), Var)) {
                ಆದರೆ (varp->isIfaceRef()) {
                    ಆದರೆ (ಸ್ಥಿರಾಂಕ AstIfaceRefDType* ಸ್ಥಿರಾಂಕ ifaceRefp
                        = LinkDotState::ifaceRefFromArray(varp->dtypep())) {
                        ifacep = ifaceRefp->iface_ಮೂಲಕ_Cellp();
                    }
                }
            }

            ಆದರೆ (!ifacep || !m_statep->exists_ಗಂಟು_Sym(ifacep)) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;

            VSymEnt* ಸ್ಥಿರಾಂಕ ifaceSymp = m_statep->get_ಗಂಟು_Sym(ifacep);

            ಆದರೆ (baddot.ಕಂಡುಹಿಡಿ('.') == string::npos) {
                // Simple identifier - direct lookup
                VSymEnt* ಸ್ಥಿರಾಂಕ symp = ifaceSymp->ಕಂಡುಹಿಡಿ_Id_Fallback(baddot);
                ಆದರೆ (symp) baddot.clear();
                ಹಿಂತಿರುಗಿ symp;
            }

            // Multi-level path - use findDotted for partial resolution
            string ಉಳಿದ_Baddot;
            VSymEnt* ಉಳಿದ_Ok_Symp = ಶೂನ್ಯಸೂಚಕ;
            VSymEnt* ಸ್ಥಿರಾಂಕ symp = m_statep->ಕಂಡುಹಿಡಿ_Dotted(fl, ifaceSymp, baddot, ಉಳಿದ_Baddot,
                                                       ಉಳಿದ_Ok_Symp, ನಿಜ);
            ಆದರೆ (symp) {
                baddot = ಉಳಿದ_Baddot;
                ಹಿಂತಿರುಗಿ symp;
            }

            // findDotted partially matched - check progress
            ಆದರೆ (ಉಳಿದ_Baddot == baddot || !ಉಳಿದ_Ok_Symp) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;

            // Continue resolving with updated state
            baddot = ಉಳಿದ_Baddot;
            curOkSymp = ಉಳಿದ_Ok_Symp;
        }

        UINFO(1, "Nested interface resolution depth limit exceeded at " << fl);
        ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
    }

    // Resolve a modport expression to find the referenced symbol
    VSymEnt* resolve_Modport_ಅಭಿವ್ಯಕ್ತಿ(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp) {
        UINFO(5, "     resolveModportExpression: " << exprp);
        VSymEnt* symp = ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (AstParseRef* ಸ್ಥಿರಾಂಕ refp = VN_CAST(exprp, ParseRef)) {
            // Simple variable reference: modport mp(input .a(sig_a))
            symp = m_curSymp->ಕಂಡುಹಿಡಿ_Id_Fallback(refp->name());
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstVarRef* ಸ್ಥಿರಾಂಕ refp = VN_CAST(exprp, VarRef)) {
            // Already resolved VarRef (can happen if iterateChildren resolved it)
            ಆದರೆ (refp->varScopep()) {
                // During scope creation, VarRef may have VarScope already linked
                symp = m_statep->get_ಗಂಟು_Sym(refp->varScopep());
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (refp->varp()) {
                symp = m_curSymp->ಕಂಡುಹಿಡಿ_Id_Fallback(refp->varp()->name());
            }
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstVarXRef* ಸ್ಥಿರಾಂಕ refp = VN_CAST(exprp, VarXRef)) {
            // Resolved VarXRef (dotted reference resolved by earlier pass)
            ಆದರೆ (refp->varp()) {
                // For nested interfaces, the var is in a different scope
                // First try to find the symbol via the var itself
                ಆದರೆ (m_statep->exists_ಗಂಟು_Sym(refp->varp())) {
                    symp = m_statep->get_ಗಂಟು_Sym(refp->varp());
                } ಇಲ್ಲದಿದ್ದರೆ {
                    // Fallback: look up in current scope
                    symp = m_curSymp->ಕಂಡುಹಿಡಿ_Id_Fallback(refp->varp()->name());
                }
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!refp->dotted().ಖಾಲಿಗಳು()) {
                // varp not set yet - use dotted path to find the symbol
                // The dotted part (e.g., "base") is the interface cell, and name is the member
                ಸ್ಥಿರಾಂಕ string ತುಂಬಿದ_ಮಾರ್ಗ = refp->dotted() + "." + refp->name();
                string baddot;
                VSymEnt* okSymp = ಶೂನ್ಯಸೂಚಕ;
                symp = m_statep->ಕಂಡುಹಿಡಿ_Dotted(fl, m_curSymp, ತುಂಬಿದ_ಮಾರ್ಗ, baddot, okSymp, ನಿಜ);
                // Handle nested interface path when findDotted had partial match
                ಆದರೆ (okSymp && !baddot.ಖಾಲಿಗಳು()) {
                    VSymEnt* ಸ್ಥಿರಾಂಕ resolved = resolve_Nested_Interface_ಮಾರ್ಗ(fl, okSymp, baddot);
                    ಆದರೆ (resolved) symp = resolved;
                }
            }
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstDot* ಸ್ಥಿರಾಂಕ dotp = VN_CAST(exprp, Dot)) {
            // Dotted path: modport mp(input .a(inner.sig))
            ತಾರ್ಕಿಕ hasPartSelect = ಸುಳ್ಳು;
            ಸ್ಥಿರಾಂಕ string dotted_ಮಾರ್ಗ = ಹೊರತೆಗೆ_Dotted_ಮಾರ್ಗ(dotp, hasPartSelect);
            ಆದರೆ (hasPartSelect) {
                fl->v3warn(
                    E_UNSUPPORTED,
                    "Unsupported: Modport expression with part select (IEEE 1800-2023 25.5.4)");
            } ಇಲ್ಲದಿದ್ದರೆ {
                string baddot;
                VSymEnt* okSymp = ಶೂನ್ಯಸೂಚಕ;
                symp = m_statep->ಕಂಡುಹಿಡಿ_Dotted(fl, m_curSymp, dotted_ಮಾರ್ಗ, baddot, okSymp, ನಿಜ);

                // Handle nested interface path when findDotted had partial match
                ಆದರೆ (okSymp && !baddot.ಖಾಲಿಗಳು()) {
                    VSymEnt* ಸ್ಥಿರಾಂಕ resolved = resolve_Nested_Interface_ಮಾರ್ಗ(fl, okSymp, baddot);
                    ಆದರೆ (resolved) symp = resolved;
                }

                ಆದರೆ (!symp || !baddot.ಖಾಲಿಗಳು()) {
                    fl->v3error("Can't find modport expression target: "
                                << Ast_ಗಂಟು::prettyNameQ(dotted_ಮಾರ್ಗ));
                    symp = ಶೂನ್ಯಸೂಚಕ;
                }
            }
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(exprp, Sel_ಬಿಟ್) || VN_IS(exprp, Sel_ಹೊರತೆಗೆ)) {
            // Part select expressions not yet supported
            fl->v3warn(E_UNSUPPORTED,
                       "Unsupported: Modport expression with part select (IEEE 1800-2023 25.5.4)");
        } ಇಲ್ಲದಿದ್ದರೆ {
            // Other expression types not supported
            fl->v3warn(E_UNSUPPORTED,
                       "Unsupported: Complex modport expression (IEEE 1800-2023 25.5.4)");
        }
        ಹಿಂತಿರುಗಿ symp;
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstModportVarRef* nodep) override {  // IfaceVisitor::
        UINFO(5, "   fiv: " << nodep);
        iterateChildren(nodep);
        VSymEnt* symp = ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (nodep->exprp()) {
            // Modport expression syntax: modport mp(input .port_name(expression))
            symp = resolve_Modport_ಅಭಿವ್ಯಕ್ತಿ(nodep->fileline(), nodep->exprp());
        } ಇಲ್ಲದಿದ್ದರೆ {
            symp = m_curSymp->ಕಂಡುಹಿಡಿ_Id_Fallback(nodep->name());
        }
        ಆದರೆ (!symp) {
            nodep->v3error("Modport item not found: " << nodep->prettyNameQ());
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstVar* ಸ್ಥಿರಾಂಕ varp = VN_CAST(symp->nodep(), Var)) {
            // Make symbol under modport that points at the _interface_'s var via the modport.
            // (Need modport still to test input/output markings)
            nodep->varp(varp);
            ಆದರೆ (nodep->exprp()) {
                // For modport expressions, insert symbol pointing to the underlying var (not the
                // ModportVarRef which will be deleted during scope creation). The virtual port
                // name maps to the real signal's var.
                VSymEnt* ಸ್ಥಿರಾಂಕ subSymp
                    = m_statep->ಸೇರಿಸು_Sym(m_curSymp, nodep->name(), varp, ಶೂನ್ಯಸೂಚಕ /*package*/);
                m_statep->ಸೇರಿಸು_Scope_Alias(LinkDotState::SAMN_MODPORT, subSymp, symp);
            } ಇಲ್ಲದಿದ್ದರೆ {
                // For regular modport items, insert symbol pointing to ModportVarRef for
                // input/output marking tests.
                m_statep->ಸೇರಿಸು_Sym(m_curSymp, nodep->name(), nodep, ಶೂನ್ಯಸೂಚಕ /*package*/);
            }
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstVarScope* ಸ್ಥಿರಾಂಕ vscp = VN_CAST(symp->nodep(), VarScope)) {
            // Make symbol under modport that points at the _interface_'s var, not the modport.
            nodep->varp(vscp->varp());
            // Only insert symbol for modport expression virtual ports (exprp set).
            // For regular modport items, don't insert into the shared modport table because
            // each instance would overwrite the previous one, causing wrong VarScope lookups.
            // Regular items are found via the modport's fallback to the interface.
            ಆದರೆ (nodep->exprp()) {
                VSymEnt* ಸ್ಥಿರಾಂಕ subSymp
                    = m_statep->ಸೇರಿಸು_Sym(m_curSymp, nodep->name(), vscp, ಶೂನ್ಯಸೂಚಕ /*package*/);
                m_statep->ಸೇರಿಸು_Scope_Alias(LinkDotState::SAMN_MODPORT, subSymp, symp);
            }
        } ಇಲ್ಲದಿದ್ದರೆ {
            nodep->v3error("Modport item is not a variable: " << nodep->prettyNameQ());
        }
        ಆದರೆ (m_statep->forScopeCreation()) {
            // Done with AstModportVarRef.
            // Delete to prevent problems if we dead-delete pointed to variable
            nodep->unlink_Fr_ಹಿಂಭಾಗ();
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstModportClockingRef* nodep) override {  // IfaceVisitor::
        UINFO(5, "   fic: " << nodep);
        iterateChildren(nodep);
        VSymEnt* ಸ್ಥಿರಾಂಕ symp = m_curSymp->ಕಂಡುಹಿಡಿ_Id_Fallback(nodep->name());
        ಆದರೆ (!symp) {
            nodep->v3error("Modport item not found: " << nodep->prettyNameQ());
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstClocking* ಸ್ಥಿರಾಂಕ clockingp = VN_CAST(symp->nodep(), Clocking)) {
            nodep->clockingp(clockingp);
            m_statep->ಸೇರಿಸು_Sym(m_curSymp, nodep->name(), nodep, ಶೂನ್ಯಸೂಚಕ /*package*/);
        } ಇಲ್ಲದಿದ್ದರೆ {
            nodep->v3error("Modport item is not a clocking block: " << nodep->prettyNameQ());
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstScope* nodep) override {  // IfaceVisitor::
        ಆದರೆ (nodep->user3_ಗಣ_Once()) ಹಿಂತಿರುಗಿ;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }  // IfaceVisitor::

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    LinkDotIfaceVisitor(AstIface* nodep, VSymEnt* curSymp, LinkDotState* statep)
        : m_statep{statep}
        , m_curSymp{curSymp} {
        UINFO(4, __FUNCTION__ << ": ");
        iterate(nodep);
    }
    ~LinkDotIfaceVisitor() override = ಪೂರ್ವನಿಯೋಜಿತ;
};

ಶೂನ್ಯ LinkDotState::computeIfaceModSyms() {
    //  AstScope::user3()  // bool. Already iterated by a LinkDotIfaceVisitor
    ಸ್ಥಿರಾಂಕ VNUser3_ಒಳ_Use user3_ಒಳ_Use;
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& itr : m_ifaceModSyms) {
        AstIface* ಸ್ಥಿರಾಂಕ nodep = itr.ಮೊದಲ;
        VSymEnt* ಸ್ಥಿರಾಂಕ symp = itr.ಎರಡನೇ;
        LinkDotIfaceVisitor{nodep, symp, ಇದು};
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
    ಸ್ಥಿರಾಂಕ VNUser3_ಒಳ_Use m_inuser3;

    // TYPES
    ಎಣಿಕೆ DotPosition : uint8_t {
        // Must match ascii() method below
        DP_NONE = 0,  // Not under a DOT
        DP_PACKAGE,  // {package-or-class}:: DOT
        DP_ಮೊದಲ,  // {scope-or-var} DOT
        DP_SCOPE,  // DOT... {scope-or-var} DOT
        DP_FINAL,  // [DOT...] {var-or-func-or-dtype} with no following dots
        DP_MEMBER  // DOT {member-name} [DOT...]
    };

    // STATE
    LinkDotState* ಸ್ಥಿರಾಂಕ m_statep;  // State, including dotted symbol table
    VSymEnt* m_curSymp = ಶೂನ್ಯಸೂಚಕ;  // SymEnt for current lookup point
    VSymEnt* m_modSymp = ಶೂನ್ಯಸೂಚಕ;  // SymEnt for current module
    VSymEnt* m_pinSymp = ಶೂನ್ಯಸೂಚಕ;  // SymEnt for pin lookups
    VSymEnt* m_randSymp = ಶೂನ್ಯಸೂಚಕ;  // SymEnt for randomize target class's lookups
    ಸ್ಥಿರಾಂಕ AstCell* m_cellp = ಶೂನ್ಯಸೂಚಕ;  // Current cell
    Ast_ಗಂಟು_Module* m_modp = ಶೂನ್ಯಸೂಚಕ;  // Current module
    Ast_ಗಂಟು_FTask* m_ftaskp = ಶೂನ್ಯಸೂಚಕ;  // Current function/task
    Ast_ವಿಧಾನ_Call* m_rand_ವಿಧಾನ_Callp = ಶೂನ್ಯಸೂಚಕ;  // Current randomize() method call
    ಪೂರ್ಣಾಂಕ m_modport_ಅಂಕೆ = 0;  // Uniqueify modport numbers
    ಪೂರ್ಣಾಂಕ m_indent = 0;  // Indentation (tree depth) for debug
    ತಾರ್ಕಿಕ m_ಒಳ_Sens = ಸುಳ್ಳು;  // True if in senitem
    ಸ್ಥಿರಾಂಕ AstWith* m_currentWithp = ಶೂನ್ಯಸೂಚಕ;  // Enclosing 'with' (nullptr if none)
    std::ಗಣ<std::string>
        m_restricted_ಹೆಸರುಗಳು_ಬಳಸಿದ;  // Names from current 'with (id_list)' that resolved into target
    ತಾರ್ಕಿಕ m_genericIfaceModule = ಸುಳ್ಳು;  // True if in module containing generic interface
    std::map<std::string, Ast_ಗಂಟು*> m_if_Class_Imp_ಹೆಸರುಗಳು;  // Names imported from interface class
    // Inherited interface implementations needing a forward in the current class, by name
    std::map<std::string, ಸ್ಥಿರಾಂಕ VSymEnt*> m_ifForwards;
    std::ಗಣ<AstClass*> m_extendsParam;  // Classes that have a parameterized super class
                                         // (except the default instances)
                                         // They are added to the set only in linkDotPrimary.
    ತಾರ್ಕಿಕ m_insideClassExtParam = ಸುಳ್ಳು;  // Inside a class from m_extendsParam
    AstNew* m_explicitSuperNewp = ಶೂನ್ಯಸೂಚಕ;  // Hit a "super.new" call inside a "new" function
    std::map<Ast_ಗಂಟು*, AstPin*> m_ಬಳಸಿದ_Pins;  // Pin used in this cell, map to duplicate
    std::map<Ast_ಗಂಟು*, std::map<std::string, AstPin*>> m_ಬಳಸಿದ_Def_Param_Pins;
    // Defparam pins used for a given formal, by hierarchical path
    std::map<std::string, Ast_ಗಂಟು_Module*> m_modules_ಗಮ್ಯ_Revisit;  // Modules to revisit a second time
    Ast_ಗಂಟು* m_ಕೊನೆಯ_Deferredp = ಶೂನ್ಯಸೂಚಕ;  // Last node which requested a revisit of its module
    Ast_ಗಂಟು_DType* m_packedArrayDtp = ಶೂನ್ಯಸೂಚಕ;  // Datatype reference for packed array
    ತಾರ್ಕಿಕ m_ಒಳ_Packed_Array = ಸುಳ್ಳು;  // Currently traversing a packed array tree
    ತಾರ್ಕಿಕ m_replaceWithAlias
        = ನಿಜ;  // Replace VarScope with an alias. Used in the handling of AstAlias
    ತಾರ್ಕಿಕ m_isParam = ಸುಳ್ಳು;  // Specifies whether currently visiting param variable
    ತಾರ್ಕಿಕ m_resolvingTypedef = ಸುಳ್ಳು;  // Currently traversing a Typedef tree

    ರಚನೆ Dot_ಸ್ಥಿತಿಗಳು final {
        DotPosition m_dotPos;  // Scope part of dotted resolution
        VSymEnt* m_dotSymp;  // SymEnt for dotted AstParse lookup
        ಸ್ಥಿರಾಂಕ AstDot* m_dotp;  // Current dot
        ತಾರ್ಕಿಕ m_super;  // Starts with super reference
        ತಾರ್ಕಿಕ m_unresolvedCell;  // Unresolved cell, needs help from V3Param
        ತಾರ್ಕಿಕ m_unresolvedClass;  // Unresolved class reference, needs help from V3Param
        ತಾರ್ಕಿಕ m_unresolvedGenericIface;  // Unresolved generic interface, needs help from V3Param
        ತಾರ್ಕಿಕ m_genBlk;  // Contains gen block reference
        Ast_ಗಂಟು* m_unlinkedScopep;  // Unresolved scope, needs corresponding VarXRef
        AstDisable* m_disablep;  // Disable statement under which the reference is
        ತಾರ್ಕಿಕ m_dotErr;  // Error found in dotted resolution, ignore upwards
        string m_dot_ಪಠ್ಯ;  // String of dotted names found in below parseref
        Dot_ಸ್ಥಿತಿಗಳು() { init(ಶೂನ್ಯಸೂಚಕ); }
        ~Dot_ಸ್ಥಿತಿಗಳು() = ಪೂರ್ವನಿಯೋಜಿತ;
        ಶೂನ್ಯ init(VSymEnt* curSymp, ತಾರ್ಕಿಕ unresolvedClass = ಸುಳ್ಳು) {
            m_dotPos = DP_NONE;
            m_dotSymp = curSymp;
            m_dotp = ಶೂನ್ಯಸೂಚಕ;
            m_super = ಸುಳ್ಳು;
            m_dotErr = ಸುಳ್ಳು;
            m_dot_ಪಠ್ಯ = "";
            m_unresolvedCell = ಸುಳ್ಳು;
            m_unresolvedClass = unresolvedClass;
            m_unresolvedGenericIface = ಸುಳ್ಳು;
            m_genBlk = ಸುಳ್ಳು;
            m_unlinkedScopep = ಶೂನ್ಯಸೂಚಕ;
            m_disablep = ಶೂನ್ಯಸೂಚಕ;
        }
        string ascii() ಸ್ಥಿರಾಂಕ {
            ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ ಹೆಸರುಗಳು[]
                = {"DP_NONE", "DP_PACKAGE", "DP_FIRST", "DP_SCOPE", "DP_FINAL", "DP_MEMBER"};
            std::ostringstream sstr;
            sstr << "ds=" << ಹೆಸರುಗಳು[m_dotPos];
            sstr << "  dse" << cvt_ಗಮ್ಯ_Hex(m_dotSymp);
            ಸ್ಥಿರಾಂಕ string dsname = m_dotSymp->nodep()->name().substr(0, 8);
            sstr << "(" << m_dotSymp->nodep()->typeName() << (dsname.ಖಾಲಿಗಳು() ? "" : ":") << dsname
                 << ")";
            ಆದರೆ (m_dotErr) sstr << "  [dotErr]";
            ಆದರೆ (m_super) sstr << "  [super]";
            ಆದರೆ (m_unresolvedCell) sstr << "  [unrCell]";
            ಆದರೆ (m_unresolvedClass) sstr << "  [unrClass]";
            ಆದರೆ (m_unresolvedGenericIface) sstr << "  [unrGIface]";
            ಆದರೆ (m_genBlk) sstr << "  [genBlk]";
            sstr << "  txt=" << m_dot_ಪಠ್ಯ;
            ಹಿಂತಿರುಗಿ sstr.str();
        }
    } m_ds;  // State to preserve across recursions

    // METHODS - Variables
    AstVar* createImplicitVar(VSymEnt* /*lookupSymp*/, AstParseRef* nodep, Ast_ಗಂಟು_Module* modp,
                              VSymEnt* moduleSymp, ತಾರ್ಕಿಕ noWarn) {
        // Create implicit after warning
        ಆದರೆ (!noWarn) {
            ಆದರೆ (nodep->fileline()->warnIsOff(V3_Error_ಸಂಕೇತ::I_DEF_NETTYPE_WIRE)) {
                ಸ್ಥಿರಾಂಕ string suggest = m_statep->suggestSymFallback(moduleSymp, nodep->name(),
                                                                    Link_ಗಂಟು_Matcher_Var{});
                nodep->v3error("Signal definition not found, and implicit disabled with "
                               "`default_nettype: "
                               << nodep->prettyNameQ() << '\n'
                               << (suggest.ಖಾಲಿಗಳು() ? "" : nodep->warn_ಇನ್ನಷ್ಟು() + suggest));

            }
            // Bypass looking for suggestions if IMPLICIT is turned off
            // as there could be thousands of these suppressed in large netlists
            ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!nodep->fileline()->warnIsOff(V3_Error_ಸಂಕೇತ::IMPLICIT)) {
                ಸ್ಥಿರಾಂಕ string suggest = m_statep->suggestSymFallback(moduleSymp, nodep->name(),
                                                                    Link_ಗಂಟು_Matcher_Var{});
                nodep->v3warn(IMPLICIT,
                              "Signal definition not found, creating implicitly: "
                                  << nodep->prettyNameQ() << '\n'
                                  << (suggest.ಖಾಲಿಗಳು() ? "" : nodep->warn_ಇನ್ನಷ್ಟು() + suggest));
            }
        }
        AstVar* ಸ್ಥಿರಾಂಕ newp
            = ಹೊಸ AstVar{nodep->fileline(), VVarType::WIRE, nodep->name(), VFlagLogicPacked{}, 1};
        newp->lifetime(VLifetime::STATIC_IMPLICIT);
        newp->trace(modp->modTrace());
        modp->ಕೂಡಿಸು_Stmtsp(newp);
        // Link it to signal list, must add the variable under the module;
        // current scope might be lower now
        m_statep->ಸೇರಿಸು_Sym(moduleSymp, newp->name(), newp, ಶೂನ್ಯಸೂಚಕ /*classOrPackagep*/);
        ಹಿಂತಿರುಗಿ newp;
    }
    AstVar* ಸಿಕ್ಕಿತು_ಗಮ್ಯ_Varp(ಸ್ಥಿರಾಂಕ VSymEnt* symp, Ast_ಗಂಟು* nodep, VAccess access) {
        // Return a variable if possible, auto converting a modport to variable
        ಆದರೆ (!symp) {
            ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(symp->nodep(), Var)) {
            ಹಿಂತಿರುಗಿ VN_AS(symp->nodep(), Var);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(symp->nodep(), ModportVarRef)) {
            AstModportVarRef* ಸ್ಥಿರಾಂಕ snodep = VN_AS(symp->nodep(), ModportVarRef);
            AstVar* ಸ್ಥಿರಾಂಕ varp = snodep->varp();
            ಆದರೆ (access.isWriteOrRW() && snodep->direction().isReadOnly()) {
                nodep->v3error("Attempt to drive input-only modport: " << nodep->prettyNameQ());
            }  // else other simulators don't warn about reading, and IEEE doesn't say illegal
            ಹಿಂತಿರುಗಿ varp;
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        }
    }

    // Look up a virtual port through modport expression mapping.
    // When a dotted reference uses a modport with expression syntax, the virtual port name
    // maps to the real signal. This helper resolves that mapping during scope creation.
    // Returns the VarScope for the real signal, or nullptr if not found.
    AstVarScope* ಕಂಡುಹಿಡಿ_Virtual_ಪೋರ್ಟ್_Var_Scope(VSymEnt* dotSymp, ಸ್ಥಿರಾಂಕ string& ಪೋರ್ಟ್_Name) {
        ಸ್ಥಿರಾಂಕ AstVarScope* ಸ್ಥಿರಾಂಕ dotVscp = VN_CAST(dotSymp->nodep(), VarScope);
        ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ dotVarp = dotVscp ? dotVscp->varp() : ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (!dotVarp) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;

        Ast_ಗಂಟು_DType* dtypep = dotVarp->ಮಗು_DTypep();
        ಆದರೆ (!dtypep) dtypep = dotVarp->subDTypep();
        ಆದರೆ (!dtypep) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        ಸ್ಥಿರಾಂಕ AstIfaceRefDType* ಸ್ಥಿರಾಂಕ ifaceRefp = VN_CAST(dtypep, IfaceRefDType);
        ಆದರೆ (!ifaceRefp || !ifaceRefp->modportp() || !ifaceRefp->iface_ಮೂಲಕ_Cellp()) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;

        // Get the interface cell's symbol table
        VSymEnt* ಸ್ಥಿರಾಂಕ ifaceCellSymp = m_statep->get_ಗಂಟು_Sym(ifaceRefp->iface_ಮೂಲಕ_Cellp());

        // Look up modport by name in interface cell
        VSymEnt* ಸ್ಥಿರಾಂಕ modportSymp = ifaceCellSymp->ಕಂಡುಹಿಡಿ_Id_Fallback(ifaceRefp->modportp()->name());
        ಆದರೆ (!modportSymp) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;

        // Look up virtual port name in modport symbol table
        string baddot;
        VSymEnt* ಸ್ಥಿರಾಂಕ virtPortp = m_statep->ಕಂಡುಹಿಡಿ_Sym_Prefixed(modportSymp, ಪೋರ್ಟ್_Name, baddot, ನಿಜ);
        ಆದರೆ (!virtPortp) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;

        // Symbol may point to VarScope or Var
        ಆದರೆ (AstVarScope* vscp = VN_CAST(virtPortp->nodep(), VarScope)) ಹಿಂತಿರುಗಿ vscp;

        // Symbol points to Var - look up VarScope by name in interface cell
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ realVarp = VN_CAST(virtPortp->nodep(), Var)) {
            VSymEnt* ಸ್ಥಿರಾಂಕ realFoundp
                = m_statep->ಕಂಡುಹಿಡಿ_Sym_Prefixed(ifaceCellSymp, realVarp->name(), baddot, ನಿಜ);
            ಹಿಂತಿರುಗಿ realFoundp ? VN_CAST(realFoundp->nodep(), VarScope) : ಶೂನ್ಯಸೂಚಕ;
        }
        ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
    }

    // Capture a ParamTypeDType reference for interface typedef retargeting.
    // Called when a RefDType resolves to a ParamTypeDType owned by an interface.
    ಶೂನ್ಯ captureIfaceParamType(AstRefDType* nodep, AstParamTypeDType* defp) {
        ಆದರೆ (!V3LinkDotIfaceCapture::enabled() || !m_statep->forPrimary()) ಹಿಂತಿರುಗಿ;
        Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ defOwnerModp = V3LinkDotIfaceCapture::ಕಂಡುಹಿಡಿ_Owner_Module(defp);
        ಆದರೆ (!defOwnerModp || !VN_IS(defOwnerModp, Iface)) ಹಿಂತಿರುಗಿ;
        AstCell* ಸ್ಥಿರಾಂಕ cellForCapture
            = m_ds.m_dotSymp ? VN_CAST(m_ds.m_dotSymp->nodep(), Cell) : ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (!cellForCapture) ಹಿಂತಿರುಗಿ;
        UINFO(9, indent() << "iface capture add paramtype " << nodep
                          << " iface=" << defOwnerModp->prettyNameQ());
        V3LinkDotIfaceCapture::ಕೂಡಿಸು_Param_Type(nodep, cellForCapture->name(), m_modp, defp,
                                            defOwnerModp->name());
    }

    Ast_ಗಂಟು_Stmt* ಕೂಡಿಸು_Implicit_Super_New_Call(AstFunc* ಸ್ಥಿರಾಂಕ nodep,
                                         ಸ್ಥಿರಾಂಕ AstClassExtends* ಸ್ಥಿರಾಂಕ classExtendsp) {
        // Returns the added node
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = nodep->fileline();
        AstArg* argsp = ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (classExtendsp->argsp()) argsp = classExtendsp->argsp()->clone_ವೃಕ್ಷ(ನಿಜ);
        AstNew* ಸ್ಥಿರಾಂಕ newExprp = ಹೊಸ AstNew{fl, argsp};
        newExprp->isImplicit(ನಿಜ);
        AstDot* ಸ್ಥಿರಾಂಕ superNewp = ಹೊಸ AstDot{fl, ಸುಳ್ಳು, ಹೊಸ AstParseRef{fl, "super"}, newExprp};
        Ast_ಗಂಟು_Stmt* ಸ್ಥಿರಾಂಕ superNewStmtp = superNewp->makeStmt();
        ಗಾಗಿ (Ast_ಗಂಟು* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            // super.new shall be the first statement (IEEE 1800-2023 8.15)
            // but some nodes (such as variable declarations and typedefs) should stay before
            ಆದರೆ (VN_IS(stmtp, ಗಂಟು_Stmt)) {
                stmtp->ಕೂಡಿಸು_ಇಲ್ಲಿ_This_As_ಮುಂದಿನ(superNewStmtp);
                ಹಿಂತಿರುಗಿ superNewStmtp;
            }
        }
        // There were no statements
        nodep->ಕೂಡಿಸು_Stmtsp(superNewStmtp);
        ಹಿಂತಿರುಗಿ superNewStmtp;
    }
    ಶೂನ್ಯ checkNoDot(Ast_ಗಂಟು* nodep) {
        ಆದರೆ (VL_UNLIKELY(!nodep)) {
            UINFO(9, indent() << "iface capture null node passed to checkNoDot; dot state="
                              << m_ds.ascii());
            m_ds.m_dotErr = ನಿಜ;
            ಹಿಂತಿರುಗಿ;
        }
        ಆದರೆ (VL_UNLIKELY(m_ds.m_dotPos != DP_NONE)) {
            UINFO(9, indent() << "ds=" << m_ds.ascii());
            UINFO(9, indent() << "iface capture checkNoDot hit node=" << nodep
                              << " dotState=" << m_ds.ascii());
            ಆದರೆ (VL_UNLIKELY(!m_ds.m_dotp)) {
                nodep->v3error("Syntax error: Not expecting " << nodep->type()
                                                              << " here (missing dot context)");
            } ಇಲ್ಲದಿದ್ದರೆ {
                nodep->v3error("Syntax error: Not expecting "
                               << nodep->type() << " under a " << nodep->backp()->type()
                               << " in dotted expression\n"
                               << nodep->warnContextPrimary() << m_ds.m_dotp->warnOther()
                               << "... Resolving this reference\n"
                               << m_ds.m_dotp->warnContextSecondary());
            }
            m_ds.m_dotErr = ನಿಜ;
        }
    }
    AstVar* ಕಂಡುಹಿಡಿ_Iface_ತುದಿ_Varp(Ast_ಗಂಟು* nodep, VSymEnt* ಪೋಷಕ_Entp, ಸ್ಥಿರಾಂಕ string& name) {
        ಸ್ಥಿರಾಂಕ string ಕಂಡುಹಿಡಿ_Name = name + "__Viftop";
        VSymEnt* ಸ್ಥಿರಾಂಕ ifaceSymp = ಪೋಷಕ_Entp->ಕಂಡುಹಿಡಿ_Id_Fallback(ಕಂಡುಹಿಡಿ_Name);
        AstVar* ಸ್ಥಿರಾಂಕ iface_ತುದಿ_Varp = ifaceSymp ? VN_AS(ifaceSymp->nodep(), Var) : ಶೂನ್ಯಸೂಚಕ;
        UASSERT_OBJ(iface_ತುದಿ_Varp, nodep, "Can't find interface var ref: " << ಕಂಡುಹಿಡಿ_Name);
        ಹಿಂತಿರುಗಿ iface_ತುದಿ_Varp;
    }
    AstRSProd* ಕಂಡುಹಿಡಿ_Prod(Ast_ಗಂಟು* nodep, VSymEnt* ಪೋಷಕ_Entp, ಸ್ಥಿರಾಂಕ string& name) {
        ಸ್ಥಿರಾಂಕ VSymEnt* ಸ್ಥಿರಾಂಕ foundp = ಪೋಷಕ_Entp->ಕಂಡುಹಿಡಿ_Id_Fallback(name);
        AstRSProd* ಸಿಕ್ಕಿತು_Nodep = foundp ? VN_CAST(foundp->nodep(), RSProd) : ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (!ಸಿಕ್ಕಿತು_Nodep) {
            VSpellCheck speller;
            Link_ಗಂಟು_Matcher_Prod matcher;
            ಪೋಷಕ_Entp->candidateIdFlat(&speller, &matcher, ಸುಳ್ಳು);
            ಸ್ಥಿರಾಂಕ string suggest = speller.ಉತ್ತಮ_Candidate_ಸಂದೇಶ(name);
            UINFO(1, "   ErrParseRef curSymp=se" << cvt_ಗಮ್ಯ_Hex(ಪೋಷಕ_Entp));
            nodep->v3error("Production " << Ast_ಗಂಟು::prettyNameQ(name) << " not found\n"
                                         << (suggest.ಖಾಲಿಗಳು() ? "" : nodep->warn_ಇನ್ನಷ್ಟು() + suggest));
            ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        }
        ಹಿಂತಿರುಗಿ ಸಿಕ್ಕಿತು_Nodep;
    }
    ಶೂನ್ಯ duplicatePinError(AstPin* nodep, Ast_ಗಂಟು* origp, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* whatp) {
        nodep->v3error("Duplicate " << whatp << " connection: " << nodep->prettyNameQ() << '\n'
                                    << nodep->warnContextPrimary() << '\n'
                                    << origp->warnOther() << "... Location of original " << whatp
                                    << " connection\n"
                                    << origp->warnContextSecondary());
    }
    ಶೂನ್ಯ markAndCheckPinDup(AstPin* nodep, Ast_ಗಂಟು* refp, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* whatp) {
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ pair = m_ಬಳಸಿದ_Pins.emplace(refp, nodep);
        ಆದರೆ (pair.ಎರಡನೇ) {
            ಆದರೆ (!nodep->param_ಮಾರ್ಗ().ಖಾಲಿಗಳು())
                m_ಬಳಸಿದ_Def_Param_Pins[refp].emplace(nodep->param_ಮಾರ್ಗ(), nodep);
            ಹಿಂತಿರುಗಿ;
        } ಇಲ್ಲದಿದ್ದರೆ {
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ origp = pair.ಮೊದಲ->ಎರಡನೇ;
            ಆದರೆ (nodep->param_ಮಾರ್ಗ().ಖಾಲಿಗಳು() || VN_AS(origp, Pin)->param_ಮಾರ್ಗ().ಖಾಲಿಗಳು()) {
                duplicatePinError(nodep, origp, whatp);
                ಹಿಂತಿರುಗಿ;
            }
        }

        ಸ್ವಯಂಚಾಲಿತ& defParamPins = m_ಬಳಸಿದ_Def_Param_Pins[refp];
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ defParamIt = defParamPins.ಕಂಡುಹಿಡಿ(nodep->param_ಮಾರ್ಗ());
        ಆದರೆ (defParamIt != defParamPins.ಕೊನೆ()) {
            duplicatePinError(nodep, defParamIt->ಎರಡನೇ, whatp);
            ಹಿಂತಿರುಗಿ;
        }
        defParamPins.emplace(nodep->param_ಮಾರ್ಗ(), nodep);
    }
    ಸ್ಥಿರ ತಾರ್ಕಿಕ extendsClass(ಸ್ಥಿರಾಂಕ AstClass* classp, ಸ್ಥಿರಾಂಕ AstClass* targetp) {
        // Return true if classp inherits from targetp through already resolved extends
        // If performance of this becomes a problem, use just a counter to check
        // exceeds some number of iterations then perform the more expensive analysis
        std::ಗಣ<ಸ್ಥಿರಾಂಕ AstClass*> ಭೇಟಿಯಾದ;
        std::ಸದಿಶ<ಸ್ಥಿರಾಂಕ AstClass*> todo{classp};
        ಆಗ (!todo.ಖಾಲಿಗಳು()) {
            ಸ್ಥಿರಾಂಕ AstClass* ಸ್ಥಿರಾಂಕ currp = todo.ಹಿಂಭಾಗ();
            todo.ತೆಗೆ_ಹಿಂಭಾಗ();
            ಆದರೆ (currp == targetp) ಹಿಂತಿರುಗಿ ನಿಜ;
            ಆದರೆ (!ಭೇಟಿಯಾದ.ಸೇರಿಸು(currp).ಎರಡನೇ) ಮುಂದುವರಿ;
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ AstClassExtends* cextp = currp->extendsp(); cextp;
                 cextp = VN_AS(cextp->nextp(), ClassExtends)) {
                ಆದರೆ (ಸ್ಥಿರಾಂಕ AstClass* ಸ್ಥಿರಾಂಕ basep = cextp->classOrNullp()) todo.ತಳ್ಳು_ಹಿಂಭಾಗ(basep);
            }
        }
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }
    ಸ್ಥಿರ AstClocking* sensClockingp(Ast_ಗಂಟು* nodep) {
        // Return the clocking block referenced by nodep, either directly or through a modport
        ಆದರೆ (AstClocking* ಸ್ಥಿರಾಂಕ clockingp = VN_CAST(nodep, Clocking)) ಹಿಂತಿರುಗಿ clockingp;
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstModportClockingRef* ಸ್ಥಿರಾಂಕ clockingRefp = VN_CAST(nodep, ModportClockingRef))
            ಹಿಂತಿರುಗಿ clockingRefp->clockingp();
        ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
    }
    VSymEnt* getCreateClockingEventSymEnt(AstClocking* clockingp) {
        AstVar* ಸ್ಥಿರಾಂಕ eventp = clockingp->ensureEventp(ನಿಜ);
        ಆದರೆ (!eventp->user1p()) eventp->user1p(ಹೊಸ VSymEnt{m_statep->symsp(), eventp});
        ಹಿಂತಿರುಗಿ reinterpret_cast<VSymEnt*>(eventp->user1p());
    }
    VSymEnt* ಕಂಡುಹಿಡಿ_Inlined_Dots_Sym(Ast_ಗಂಟು* nodep, ಸ್ಥಿರಾಂಕ string& inlinedDots) {
        // Return scope that applies for searching, given renaming due to module inlining.
        // Dotted lookup is always relative to module, as maybe variable
        // name lower down with same scope name we want to ignore (t_math_divw)
        VSymEnt* dotSymp = m_modSymp;
        ಸ್ಥಿರಾಂಕ string inl = Ast_ಗಂಟು::dedotName(inlinedDots);
        string baddot;
        VSymEnt* okSymp = ಶೂನ್ಯಸೂಚಕ;
        dotSymp = m_statep->ಕಂಡುಹಿಡಿ_Dotted(nodep->fileline(), dotSymp, inl, baddot, okSymp, ಸುಳ್ಳು);
        UASSERT_OBJ(dotSymp, nodep,
                    "Couldn't resolve inlined scope " << Ast_ಗಂಟು::prettyNameQ(baddot)
                                                      << " in: " << inlinedDots);
        UINFO(9, indent() << "findInlinedDotsSym " << dotSymp
                          << " search start due to inlinedDots=" << inlinedDots);
        ಹಿಂತಿರುಗಿ dotSymp;
    }
    VSymEnt* ಕಂಡುಹಿಡಿ_Dotted_Ref_Symp(Ast_ಗಂಟು* nodep, VSymEnt* lookupSymp, ಸ್ಥಿರಾಂಕ string& dotted,
                               string& baddot, VSymEnt*& okSymp, ತಾರ್ಕಿಕ* modportp = ಶೂನ್ಯಸೂಚಕ) {
        // Return scope to search for a relinked hierarchical reference
        // (AstVarXRef or AstEnumItemRef), or nullptr after error if not found
        // If modportp is given, set true when the reference went through a modport
        ಆದರೆ (modportp) *modportp = ಸುಳ್ಳು;
        VSymEnt* dotSymp = m_statep->ಕಂಡುಹಿಡಿ_Dotted(nodep->fileline(), lookupSymp, dotted, baddot,
                                                okSymp, ನಿಜ);  // Maybe nullptr
        ಆದರೆ (!dotSymp) {
            nodep->v3error(
                "Can't find definition of "
                << (!baddot.ಖಾಲಿಗಳು() ? Ast_ಗಂಟು::prettyNameQ(baddot) : nodep->prettyNameQ()) << '\n'
                << nodep->warnContextPrimary());
            ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        }
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ varp = VN_CAST(dotSymp->nodep(), Var)) {
            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstIfaceRefDType* ಸ್ಥಿರಾಂಕ ifaceRefp
                = VN_CAST(varp->ಮಗು_DTypep(), IfaceRefDType)) {
                ಆದರೆ (ifaceRefp->modportp()) {
                    dotSymp = m_statep->get_ಗಂಟು_Sym(ifaceRefp->modportp());
                    ಆದರೆ (modportp) *modportp = ನಿಜ;
                } ಇಲ್ಲದಿದ್ದರೆ {
                    dotSymp = m_statep->get_ಗಂಟು_Sym(ifaceRefp->ifacep());
                }
            }
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ AstModportClockingRef* ಸ್ಥಿರಾಂಕ clockingRefp
                   = VN_CAST(dotSymp->nodep(), ModportClockingRef)) {
            dotSymp = m_statep->get_ಗಂಟು_Sym(clockingRefp->clockingp());
        }
        ಹಿಂತಿರುಗಿ dotSymp;
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ isParamedClassRefDType(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* classp) {
        ಆಗ (ಸ್ಥಿರಾಂಕ AstRefDType* ಸ್ಥಿರಾಂಕ refp = VN_CAST(classp, RefDType))
            classp = refp->subDTypep();
        ಹಿಂತಿರುಗಿ (VN_IS(classp, ClassRefDType) && VN_AS(classp, ClassRefDType)->paramsp())
               || VN_IS(classp, ParamTypeDType);
    }
    ತಾರ್ಕಿಕ isParamedClassRef(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep) {
        // Is this a parameterized reference to a class, or a reference to class parameter
        ಆದರೆ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ* classRefp = VN_CAST(nodep, ClassOrPackageRef)) {
            ಆದರೆ (classRefp->paramsp()) ಹಿಂತಿರುಗಿ ನಿಜ;
            ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ* classp = classRefp->classOrPackageNodep();
            ಆಗ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ* typedefp = VN_CAST(classp, Typedef)) classp = typedefp->subDTypep();
            ಹಿಂತಿರುಗಿ isParamedClassRefDType(classp);
        }
        ಹಿಂತಿರುಗಿ isParamedClassRefDType(nodep);
    }
    VSymEnt* getThisClassSymp() {
        VSymEnt* classSymp = m_ds.m_dotSymp;
        ಆಗ (classSymp && !VN_IS(classSymp->nodep(), Class)) {
            classSymp = classSymp->parentp();
        }
        ಹಿಂತಿರುಗಿ classSymp;
    }
    // Is the current scope within the given class, including within its nested classes
    ತಾರ್ಕಿಕ insideClass(ಸ್ಥಿರಾಂಕ AstClass* classp) ಸ್ಥಿರಾಂಕ {
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ VSymEnt* symp = m_curSymp; symp; symp = symp->parentp()) {
            ಆದರೆ (symp->nodep() == classp) ಹಿಂತಿರುಗಿ ನಿಜ;
        }
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }
    // 'randomize() with' target class not yet specialized; defer to post-V3Param pass
    ತಾರ್ಕಿಕ rand_ಗುರಿ_Unresolved() ಸ್ಥಿರಾಂಕ {
        ಹಿಂತಿರುಗಿ m_statep->forPrimary() && m_currentWithp && m_rand_ವಿಧಾನ_Callp && !m_randSymp;
    }
    // Is 'funcp' of 'classp' virtual, directly or by overriding a virtual method of a base
    // class (IEEE 1800-2023 8.20)
    ತಾರ್ಕಿಕ is_Virtual_ವಿಧಾನ(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_FTask* funcp, ಸ್ಥಿರಾಂಕ AstClass* classp) ಸ್ಥಿರಾಂಕ {
        ಆದರೆ (funcp->isVirtual()) ಹಿಂತಿರುಗಿ ನಿಜ;
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ AstClassExtends* cextp = classp->extendsp(); cextp;
             cextp = VN_AS(cextp->nextp(), ClassExtends)) {
            AstClass* ಸ್ಥಿರಾಂಕ basep = cextp->classOrNullp();
            ಆದರೆ (!basep) ಮುಂದುವರಿ;
            ಸ್ಥಿರಾಂಕ VSymEnt* ಸ್ಥಿರಾಂಕ foundp = m_statep->get_ಗಂಟು_Sym(basep)->ಕಂಡುಹಿಡಿ_Id_Flat(funcp->name());
            ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_FTask* ಸ್ಥಿರಾಂಕ ತಳ_Funcp
                = foundp ? VN_CAST(foundp->nodep(), ಗಂಟು_FTask) : ಶೂನ್ಯಸೂಚಕ;
            ಸ್ಥಿರಾಂಕ AstClass* ಸ್ಥಿರಾಂಕ ತಳ_Classp = foundp && foundp->parentp()
                                                   ? VN_CAST(foundp->parentp()->nodep(), Class)
                                                   : ಶೂನ್ಯಸೂಚಕ;
            ಆದರೆ (ತಳ_Funcp && ತಳ_Classp && is_Virtual_ವಿಧಾನ(ತಳ_Funcp, ತಳ_Classp)) ಹಿಂತಿರುಗಿ ನಿಜ;
        }
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }
    // Is imported 'implp' an implementation of interface method 'ifSymp' (IEEE 1800-2023
    // 8.26.2)? If its class does not derive from the interface class, it needs a forwarding
    // override in the derived class to be reachable through the interface.
    ತಾರ್ಕಿಕ inheritedImplements(ಸ್ಥಿರಾಂಕ VSymEnt* implp, ಸ್ಥಿರಾಂಕ VSymEnt* ifSymp, ತಾರ್ಕಿಕ& needForwardr) {
        needForwardr = ಸುಳ್ಳು;
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_FTask* ಸ್ಥಿರಾಂಕ implFuncp = VN_CAST(implp->nodep(), ಗಂಟು_FTask);
        ಆದರೆ (!implFuncp || implFuncp->ಶುದ್ಧ_Virtual() || implFuncp->isStatic()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಸ್ಥಿರಾಂಕ AstClass* ಸ್ಥಿರಾಂಕ implClassp
            = implp->parentp() ? VN_CAST(implp->parentp()->nodep(), Class) : ಶೂನ್ಯಸೂಚಕ;
        ಸ್ಥಿರಾಂಕ AstClass* ಸ್ಥಿರಾಂಕ ifClassp
            = ifSymp->parentp() ? VN_CAST(ifSymp->parentp()->nodep(), Class) : ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (!implClassp || !ifClassp) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಆದರೆ (extendsClass(implClassp, ifClassp)) ಹಿಂತಿರುಗಿ ನಿಜ;
        ಆದರೆ (!is_Virtual_ವಿಧಾನ(implFuncp, implClassp)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        needForwardr = ನಿಜ;
        ಹಿಂತಿರುಗಿ ನಿಜ;
    }
    // Add 'implp' override to 'classp' that calls super.implp, so the C++ method of an
    // implemented interface class is overridden
    ಶೂನ್ಯ ಕೂಡಿಸು_Interface_Forward(AstClass* classp, Ast_ಗಂಟು_FTask* implp, AstClass* implClassp) {
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = implp->fileline();
        AstVar* fvarp = ಶೂನ್ಯಸೂಚಕ;
        Ast_ಗಂಟು_FTask* newp;
        ಆದರೆ (AstFunc* ಸ್ಥಿರಾಂಕ implFuncp = VN_CAST(implp, Func)) {
            fvarp = VN_AS(implFuncp->fvarp(), Var)->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);
            newp = ಹೊಸ AstFunc{flp, implp->name(), ಶೂನ್ಯಸೂಚಕ, fvarp};
        } ಇಲ್ಲದಿದ್ದರೆ {
            newp = ಹೊಸ AstTask{flp, implp->name(), ಶೂನ್ಯಸೂಚಕ};
        }
        newp->class_ವಿಧಾನ(ನಿಜ);
        newp->isVirtual(ನಿಜ);
        newp->lifetime(implp->lifetime());
        AstArg* argsp = ಶೂನ್ಯಸೂಚಕ;
        ಗಾಗಿ (Ast_ಗಂಟು* stmtp = implp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstVar* ಸ್ಥಿರಾಂಕ portp = VN_CAST(stmtp, Var);
            ಆದರೆ (!portp || !portp->isIO()) ಮುಂದುವರಿ;
            AstVar* ಸ್ಥಿರಾಂಕ newPortp = portp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);
            newp->ಕೂಡಿಸು_Stmtsp(newPortp);
            argsp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(
                argsp, ಹೊಸ AstArg{flp, "", ಹೊಸ AstVarRef{flp, newPortp, VAccess::READ}});
        }
        Ast_ಗಂಟು_FTask_Ref* callp;
        ಆದರೆ (AstFunc* ಸ್ಥಿರಾಂಕ implFuncp = VN_CAST(implp, Func)) {
            callp = ಹೊಸ AstFuncRef{flp, implFuncp, argsp};
        } ಇಲ್ಲದಿದ್ದರೆ {
            callp = ಹೊಸ AstTaskRef{flp, VN_AS(implp, Task), argsp};
        }
        callp->classOrPackagep(implClassp);
        callp->superReference(ನಿಜ);
        ಆದರೆ (fvarp) {
            newp->ಕೂಡಿಸು_Stmtsp(ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, ಹೊಸ AstVarRef{flp, fvarp, VAccess::WRITE}, callp});
        } ಇಲ್ಲದಿದ್ದರೆ {
            newp->ಕೂಡಿಸು_Stmtsp(callp->makeStmt());
        }
        UINFO(9, indent() << "Interface forward " << newp);
        classp->ಕೂಡಿಸು_Membersp(newp);
    }
    ಶೂನ್ಯ importDerivedClass(AstClass* derivedClassp, VSymEnt* ತಳ_Symp, AstClass* ತಳ_Classp) {
        // Also used for standard 'extends' from a base class
        UINFO(8, indent() << "importDerivedClass to " << derivedClassp << " from " << ತಳ_Classp);
        ಗಾಗಿ (VSymEnt::const_iterator it = ತಳ_Symp->begin(); it != ತಳ_Symp->ಕೊನೆ(); ++it) {
            ಆದರೆ (Ast_ಗಂಟು* ತಳ_Subp = it->ಎರಡನೇ->nodep()) {
                UINFO(8, indent() << "  SymFunc " << ತಳ_Subp);
                ಸ್ಥಿರಾಂಕ string impOrExtends
                    = ತಳ_Classp->isInterfaceClass() ? " implements " : " extends ";
                ಆದರೆ (VN_IS(ತಳ_Subp, ಗಂಟು_FTask)) {
                    ಸ್ಥಿರಾಂಕ VSymEnt* ಸ್ಥಿರಾಂಕ foundp = m_curSymp->ಕಂಡುಹಿಡಿ_Id_Flat(ತಳ_Subp->name());
                    ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_FTask* ಸ್ಥಿರಾಂಕ ತಳ_Funcp = VN_CAST(ತಳ_Subp, ಗಂಟು_FTask);
                    ಆದರೆ (!ತಳ_Funcp || !ತಳ_Funcp->ಶುದ್ಧ_Virtual()) ಮುಂದುವರಿ;
                    ತಾರ್ಕಿಕ needForward = ಸುಳ್ಳು;
                    ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ exists_ಒಳ_Derived
                        = foundp
                          && (!foundp->imported()
                              || inheritedImplements(foundp, it->ಎರಡನೇ, needForward));
                    ಆದರೆ (needForward && m_statep->forPrimary()
                        && !derivedClassp->isInterfaceClass()) {
                        m_ifForwards.emplace(ತಳ_Subp->name(), foundp);
                    }
                    ಆದರೆ (m_statep->forPrimary() && !exists_ಒಳ_Derived
                        && !derivedClassp->isInterfaceClass() && !derivedClassp->isVirtual()) {
                        derivedClassp->v3error(
                            "Class " << derivedClassp->prettyNameQ() << impOrExtends
                                     << ತಳ_Classp->prettyNameQ()
                                     << " but is missing implementation for "
                                     << ತಳ_Subp->prettyNameQ() << " (IEEE 1800-2023 8.26)\n"
                                     << derivedClassp->warnContextPrimary() << '\n'
                                     << ತಳ_Subp->warnOther()
                                     << "... Location of interface class's function\n"
                                     << ತಳ_Subp->warnContextSecondary());
                    }
                    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ itn = m_if_Class_Imp_ಹೆಸರುಗಳು.ಕಂಡುಹಿಡಿ(ತಳ_Subp->name());
                    ಆದರೆ (m_statep->forPrimary() && !exists_ಒಳ_Derived
                        && itn != m_if_Class_Imp_ಹೆಸರುಗಳು.ಕೊನೆ()
                        && itn->ಎರಡನೇ != ತಳ_Subp) {  // Not exact same function from diamond
                        derivedClassp->v3error(
                            "Class " << derivedClassp->prettyNameQ() << impOrExtends
                                     << ತಳ_Classp->prettyNameQ()
                                     << " but missing inheritance conflict resolution for "
                                     << ತಳ_Subp->prettyNameQ() << " (IEEE 1800-2023 8.26.6.2)\n"
                                     << derivedClassp->warnContextPrimary() << '\n'
                                     << ತಳ_Subp->warnOther()
                                     << "... Location of interface class's function\n"
                                     << ತಳ_Subp->warnContextSecondary());
                    }
                    m_if_Class_Imp_ಹೆಸರುಗಳು.emplace(ತಳ_Subp->name(), ತಳ_Subp);
                }
                ಆದರೆ (VN_IS(ತಳ_Subp, Constraint)) {
                    ಸ್ಥಿರಾಂಕ VSymEnt* ಸ್ಥಿರಾಂಕ foundp = m_curSymp->ಕಂಡುಹಿಡಿ_Id_Flat(ತಳ_Subp->name());
                    ಸ್ಥಿರಾಂಕ AstConstraint* ಸ್ಥಿರಾಂಕ ತಳ_Funcp = VN_CAST(ತಳ_Subp, Constraint);
                    ಆದರೆ (!ತಳ_Funcp || !ತಳ_Funcp->is_Kwd_ಶುದ್ಧ()) ಮುಂದುವರಿ;
                    ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ exists_ಒಳ_Derived = foundp && !foundp->imported();
                    ಆದರೆ (m_statep->forPrimary() && !exists_ಒಳ_Derived
                        && !derivedClassp->isInterfaceClass() && !derivedClassp->isVirtual()) {
                        derivedClassp->v3error(
                            "Class " << derivedClassp->prettyNameQ() << impOrExtends
                                     << ತಳ_Classp->prettyNameQ()
                                     << " but is missing constraint implementation for "
                                     << ತಳ_Subp->prettyNameQ() << " (IEEE 1800-2023 18.5.2)\n"
                                     << derivedClassp->warnContextPrimary() << '\n'
                                     << ತಳ_Subp->warnOther()
                                     << "... Location of interface class's pure constraint\n"
                                     << ತಳ_Subp->warnContextSecondary());
                    }
                }
            }
        }
    }
    ಶೂನ್ಯ importSymbolsFromExtended(AstClass* ಸ್ಥಿರಾಂಕ nodep, AstClassExtends* ಸ್ಥಿರಾಂಕ cextp) {
        AstClass* ಸ್ಥಿರಾಂಕ ತಳ_Classp = cextp->classp();
        VSymEnt* ಸ್ಥಿರಾಂಕ srcp = m_statep->get_ಗಂಟು_Sym(ತಳ_Classp);
        importDerivedClass(nodep, srcp, ತಳ_Classp);
        ಆದರೆ (!cextp->isImplements()) m_curSymp->importFromClass(m_statep->symsp(), srcp);
    }
    ಶೂನ್ಯ classExtendImport(AstClass* nodep) {
        // A class reference might be to a class that is later in Ast due to
        // e.g. parameterization or referring to a "class (type T) extends T"
        // Resolve it so later Class:: references into its base classes work
        symIterateNull(nodep, m_statep->get_ಗಂಟು_Sym(nodep));
    }

    ಶೂನ್ಯ check_Decl_ಕ್ರಮ(Ast_ಗಂಟು* nodep, Ast_ಗಂಟು* declp) {
        ಸ್ಥಿರಾಂಕ uint32_t decl_Token_ಅಂಕೆ = declp->decl_Token_ಅಂಕೆ();
        ಆದರೆ (!decl_Token_ಅಂಕೆ) ಹಿಂತಿರುಗಿ;  // Not implemented/valid on this object
        ಆದರೆ (nodep->fileline()->token_ಅಂಕೆ() < decl_Token_ಅಂಕೆ) {
            UINFO(1, "Related node " << nodep->fileline()->token_ಅಂಕೆ() << " " << nodep);
            UINFO(1, "Related decl " << decl_Token_ಅಂಕೆ << " " << declp);
            nodep->v3error("Reference to "
                           << nodep->prettyNameQ() << " before declaration (IEEE 1800-2023 6.18)\n"
                           << nodep->warn_ಇನ್ನಷ್ಟು()
                           << "... Suggest move the declaration before the reference, or use a "
                              "forward typedef\n"
                           << nodep->warnContextPrimary() << '\n'
                           << declp->warnOther() << "... Location of original declaration\n"
                           << declp->warnContextSecondary());
        }
    }
    ಶೂನ್ಯ check_Member_Decl_ಕ್ರಮ(Ast_ಗಂಟು* nodep, AstMemberDType* declp) {
        ಸ್ಥಿರಾಂಕ uint32_t decl_Token_ಅಂಕೆ = declp->fileline()->token_ಅಂಕೆ();
        ಆದರೆ (nodep->fileline()->token_ಅಂಕೆ() < decl_Token_ಅಂಕೆ) {
            UINFO(1, "Related node " << nodep->fileline()->token_ಅಂಕೆ() << " " << nodep);
            UINFO(1, "Related decl " << decl_Token_ಅಂಕೆ << " " << declp);
            nodep->v3error("Reference to "
                           << nodep->prettyNameQ() << " before declaration (IEEE 1800-2023 6.18)\n"
                           << nodep->warn_ಇನ್ನಷ್ಟು()
                           << "... Suggest move the declaration before the reference\n"
                           << nodep->warnContextPrimary() << '\n'
                           << declp->warnOther() << "... Location of original declaration\n"
                           << declp->warnContextSecondary());
        }
    }

    ಶೂನ್ಯ replaceWithCheckBreak(Ast_ಗಂಟು* oldp, Ast_ಗಂಟು_DType* newp) {
        // Flag now to avoid V3Broken throwing an internal error
        ಆದರೆ (oldp->wouldBreak(newp)) {
            newp->v3error(
                "Data type used where a non-data type is expected: " << newp->prettyNameQ());
            oldp->replaceWith(ಹೊಸ AstConst{newp->fileline(), AstConst::ಬಿಟ್_False_Erroring{}});
        } ಇಲ್ಲದಿದ್ದರೆ {
            oldp->replaceWith(newp);
        }
    }

    // Marks the current module to be revisited after the initial AST iteration
    ಶೂನ್ಯ revisitLater(Ast_ಗಂಟು* deferredNodep) {
        // Need to revisit entire module to build up all the necessary context
        m_ಕೊನೆಯ_Deferredp = deferredNodep;
        m_modules_ಗಮ್ಯ_Revisit.ಸೇರಿಸು(std::make_pair(m_modp->name(), m_modp));
    }

    ಶೂನ್ಯ updateVarUse(AstVar* nodep) {
        // Avoid dotted.PARAM false positive when in a parameter block
        // that is if ()'ed off by same dotted name as another block
        ಆದರೆ (nodep && nodep->isParam()) nodep->ಬಳಸಿದ_Param(ನಿಜ);
    }

    ಸ್ಥಿರ VSymEnt* ಕಂಡುಹಿಡಿ_Id_Fallback_Skip_Member_DType(VSymEnt* lookp, ಸ್ಥಿರಾಂಕ string& name) {
        VSymEnt* shadowEntp = ಶೂನ್ಯಸೂಚಕ;  // Shadowing variable: not a type, kept for error report
        ಆಗ (lookp) {
            VSymEnt* ಸ್ಥಿರಾಂಕ foundp = lookp->ಕಂಡುಹಿಡಿ_Id_Flat(name);
            ಆದರೆ (foundp && !VN_IS(foundp->nodep(), MemberDType)) {
                // Non-type entries are not type candidates
                // (IEEE 1800-2023 6.18); skip them so an enclosing
                // type is found, but keep one to preserve the "found: ..." error.
                ಆದರೆ (VN_IS(foundp->nodep(), Typedef) || VN_IS(foundp->nodep(), ParamTypeDType)
                    || VN_IS(foundp->nodep(), Class)) {
                    ಹಿಂತಿರುಗಿ foundp;
                }
                ಆದರೆ (!shadowEntp) shadowEntp = foundp;
            }
            lookp = lookp->fallbackp();
        }
        ಹಿಂತಿರುಗಿ shadowEntp;
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
    ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* get_ಅಭಿವ್ಯಕ್ತಿ_DTypep(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* selp) {
        ಆಗ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Pre_Sel* ಸ್ಥಿರಾಂಕ sp = VN_CAST(selp, ಗಂಟು_Pre_Sel)) selp = sp->fromp();
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstMemberSel* ಸ್ಥಿರಾಂಕ sp = VN_CAST(selp, MemberSel)) {
            ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* dtypep = get_ಅಭಿವ್ಯಕ್ತಿ_DTypep(sp->fromp())) {
                ಆದರೆ (ಸ್ಥಿರಾಂಕ AstClassRefDType* ಸ್ಥಿರಾಂಕ classRefp = VN_CAST(dtypep, ClassRefDType)) {
                    ಸ್ಥಿರಾಂಕ AstClass* ಸ್ಥಿರಾಂಕ classp = classRefp->classp();
                    ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ ಸಿಕ್ಕಿತು = classp->existsMember(
                        [&dtypep, name = selp->name()](ಸ್ಥಿರಾಂಕ AstClass*, ಸ್ಥಿರಾಂಕ AstVar* nodep) {
                            dtypep = nodep->ಮಗು_DTypep();
                            ಹಿಂತಿರುಗಿ nodep->name() == name;
                        });
                    ಆದರೆ (ಸಿಕ್ಕಿತು) ಹಿಂತಿರುಗಿ dtypep->elemDTypep();
                    selp->v3error("Class " << classRefp->prettyNameQ()
                                           << " does not contain field " << selp->prettyNameQ());
                } ಇಲ್ಲದಿದ್ದರೆ {
                    selp->v3fatalSrc("Member selection on expression of type "
                                     << dtypep->prettyDTypeNameQ()
                                     << ", which is not a class type");
                }
            }
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Var_Ref* ಸ್ಥಿರಾಂಕ varRefp = VN_CAST(selp, ಗಂಟು_Var_Ref)) {
            ಹಿಂತಿರುಗಿ varRefp->varp()->ಮಗು_DTypep()->elemDTypep();
        }
        ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
    }
    ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ AstVar* get_ಮುಂದಿನ_Varp(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* stmtsp) {
        // Only IO ports, as parameters are on paramsp(), not the pinsp() list paired here
        ಆಗ (stmtsp) {
            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ varp = VN_CAST(stmtsp, Var)) {
                ಆದರೆ (varp->isIO()) ಹಿಂತಿರುಗಿ varp;
            }
            stmtsp = stmtsp->nextp();
        }
        ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
    }
    // Introduce implicit parameters for modules with generic interafeces
    ಶೂನ್ಯ ಕೂಡಿಸು_Implicit_Parameters_ದ_Generic_Iface(AstCell* ಸ್ಥಿರಾಂಕ nodep, ಸ್ಥಿರಾಂಕ AstModule* ಸ್ಥಿರಾಂಕ modp) {
        // Get Param number
        ಪೂರ್ಣಾಂಕ param_ಅಂಕೆ = 1;
        ಗಾಗಿ (AstPin* paramp = nodep->paramsp(); paramp; paramp = VN_CAST(paramp->nextp(), Pin)) {
            ++param_ಅಂಕೆ;
        }

        // Add each implicit parameter type
        ಸ್ಥಿರಾಂಕ AstVar* modIfaceVarp = get_ಮುಂದಿನ_Varp(modp->stmtsp());
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ AstPin* pinp = nodep->pinsp(); pinp && modIfaceVarp;
             pinp = VN_CAST(pinp->nextp(), Pin),
                           modIfaceVarp = get_ಮುಂದಿನ_Varp(modIfaceVarp->nextp())) {
            ಆದರೆ (modIfaceVarp->varType() != VVarType::IFACEREF
                || !VN_IS(modIfaceVarp->ಮಗು_DTypep()->elemDTypep(), IfaceGenericDType)) {
                ಮುಂದುವರಿ;
            }
            Ast_ಗಂಟು* exprp = pinp->exprp();
            ಆದರೆ (!exprp) {
                modIfaceVarp->v3error("Interface port "
                                      << modIfaceVarp->prettyNameQ()
                                      << " is not connected to interface/modport pin expression");
                ಮುಂದುವರಿ;
            }
            ಆಗ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Pre_Sel* ಸ್ಥಿರಾಂಕ preSelp = VN_CAST(exprp, ಗಂಟು_Pre_Sel)) {
                exprp = preSelp->fromp();
            }
            // Resolve pin expression to the enclosing module's port Var. At primary
            // LinkDot the expression may still be an AstParseRef, so also look up by name.
            AstVar* enclosingVarp = ಶೂನ್ಯಸೂಚಕ;
            ಸ್ಥಿರಾಂಕ AstVarRef* ಸ್ಥಿರಾಂಕ varRefp = VN_CAST(exprp, VarRef);
            ಆದರೆ (varRefp) {
                enclosingVarp = varRefp->varp();
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ AstParseRef* ಸ್ಥಿರಾಂಕ parseRefp = VN_CAST(exprp, ParseRef)) {
                ಆದರೆ (m_modp) {
                    ಆದರೆ (VSymEnt* ಸ್ಥಿರಾಂಕ symp
                        = m_statep->get_ಗಂಟು_Sym(m_modp)->ಕಂಡುಹಿಡಿ_Id_Flat(parseRefp->name())) {
                        enclosingVarp = VN_CAST(symp->nodep(), Var);
                    }
                }
            }
            ಆದರೆ (enclosingVarp && enclosingVarp->varType() == VVarType::IFACEREF
                && VN_IS(enclosingVarp->ಮಗು_DTypep()->skipRefp(), IfaceGenericDType)) {
                // Nested generic-iface forwarding (#7454): enclosing port is itself still
                // generic, so emit a placeholder __VGIfaceParam pin carrying a VarRef to
                // the outer port. V3Param rewrites it to the concrete IfaceRefDType once
                // the enclosing module is specialized (see
                // ParamProcessor::resolveGenericIfaceForwardingPins for the ordering
                // constraint that keeps the rewrite inside V3Param).
                AstVarRef* ಸ್ಥಿರಾಂಕ fwdRefp
                    = ಹೊಸ AstVarRef{exprp->fileline(), enclosingVarp, VAccess::READ};
                AstPin* ಸ್ಥಿರಾಂಕ newPinp = ಹೊಸ AstPin{
                    pinp->fileline(), param_ಅಂಕೆ, "__VGIfaceParam" + modIfaceVarp->name(), fwdRefp};
                newPinp->param(ನಿಜ);
                ಭೇಟಿ(newPinp);
                nodep->ಕೂಡಿಸು_Paramsp(newPinp);
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (varRefp) {
                ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ varp = varRefp->varp();
                ಆದರೆ (ಸ್ಥಿರಾಂಕ AstIfaceRefDType* ಸ್ಥಿರಾಂಕ refp
                    = VN_CAST(varp->ಮಗು_DTypep()->elemDTypep(), IfaceRefDType)) {
                    AstIface* ಸ್ಥಿರಾಂಕ ifacep = VN_AS(refp->cellp()->modp(), Iface);
                    AstIfaceRefDType* newIfaceRefp;
                    ಆದರೆ (refp->modportp()) {
                        newIfaceRefp = ಹೊಸ AstIfaceRefDType{
                            refp->fileline(), refp->modportFileline(), m_modp->name(),
                            ifacep->name(), refp->modportName()};
                    } ಇಲ್ಲದಿದ್ದರೆ {
                        newIfaceRefp = ಹೊಸ AstIfaceRefDType{refp->fileline(), m_modp->name(),
                                                            ifacep->name()};
                    }
                    newIfaceRefp->ifacep(ifacep);
                    ಆದರೆ (refp->cellp()->paramsp()) {
                        newIfaceRefp->ಕೂಡಿಸು_Paramsp(refp->cellp()->paramsp()->clone_ವೃಕ್ಷ(ನಿಜ));
                    }
                    UASSERT_OBJ(pinp->name().ಕಂಡುಹಿಡಿ("__pinNumber") == 0
                                    || pinp->name() == modIfaceVarp->name(),
                                pinp, "Not found interface with such name");
                    AstPin* ಸ್ಥಿರಾಂಕ newPinp
                        = ಹೊಸ AstPin{pinp->fileline(), param_ಅಂಕೆ,
                                     "__VGIfaceParam" + modIfaceVarp->name(), newIfaceRefp};
                    newPinp->param(ನಿಜ);
                    ಭೇಟಿ(newPinp);
                    nodep->ಕೂಡಿಸು_Paramsp(newPinp);
                } ಇಲ್ಲದಿದ್ದರೆ {
                    varRefp->v3error("Generic interfaces can only connect to an interface and "
                                     << varp->prettyNameQ() << " is "
                                     << (varp->ಮಗು_DTypep()
                                             ? "of type " + varp->ಮಗು_DTypep()->prettyDTypeNameQ()
                                             : "not an interface"));
                }
            } ಇಲ್ಲದಿದ್ದರೆ {
                exprp->v3error("Expected an interface but " << exprp->prettyNameQ()
                                                            << " is not an interface");
            }
            ++param_ಅಂಕೆ;
        }
    }

    // Returns true and issues error iff 'varp' cannot be accesed with a VarXRef
    // in a hier_block given by 'scopeSymp'
    ತಾರ್ಕಿಕ error_Hier_Non_ಪೋರ್ಟ್(AstVarXRef* refp, AstVar* varp, VSymEnt* scopeSymp) {
        // OK to access ports on hier_block
        ಆದರೆ (varp->isIO()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        // OK if not an instance
        ಸ್ಥಿರಾಂಕ AstCell* ಸ್ಥಿರಾಂಕ cellp = VN_CAST(scopeSymp->nodep(), Cell);
        ಆದರೆ (!cellp) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        // Ok if not a hier_block
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ modp = cellp->modp();
        ಆದರೆ (!modp->hier_ಖಂಡ()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        // Bad
        refp->v3error("Cannot access non-port symbols inside hierarchical block");
        ಹಿಂತಿರುಗಿ ನಿಜ;
    }

#ವ್ಯಾಖ್ಯಾನಿಸು LINKDOT_ಭೇಟಿ_ಆರಂಭ() \
    VL_RESTORER(m_indent); \
    ++m_indent;

    string indent() ಸ್ಥಿರಾಂಕ {
        string ಫಲಿತಾಂಶ = "";
        ಫಲಿತಾಂಶ.ಸೇರಿಸು(0, m_indent, ':');
        ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ + " ";
    }

    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(AstNetlist* nodep) override {
        // Recurse..., backward as must do packages before using packages
        iterateChildrenBackwardsConst(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Type_ಕೋಷ್ಟಕ*) override {}
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Module* nodep) override {
        ಆದರೆ (nodep->dead()) ಹಿಂತಿರುಗಿ;
        LINKDOT_ಭೇಟಿ_ಆರಂಭ();
        UINFO(8, indent() << "visit " << nodep);
        VL_RESTORER(m_genericIfaceModule);
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstModule* ಸ್ಥಿರಾಂಕ modp = VN_CAST(nodep, Module)) {
            m_genericIfaceModule = modp->hasGenericIface();
        }
        checkNoDot(nodep);
        m_ds.init(m_curSymp);
        m_ds.m_dotSymp = m_curSymp = m_modSymp
            = m_statep->get_ಗಂಟು_Sym(nodep);  // Until overridden by a SCOPE
        m_cellp = ಶೂನ್ಯಸೂಚಕ;
        m_modp = nodep;
        m_modport_ಅಂಕೆ = 0;
        iterateChildren(nodep);
        m_modp = ಶೂನ್ಯಸೂಚಕ;
        m_ds.m_dotSymp = m_curSymp = m_modSymp = ಶೂನ್ಯಸೂಚಕ;
        m_ds.m_dotPos = DP_NONE;
        m_ds.m_dotErr = ಸುಳ್ಳು;
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstScope* nodep) override {
        LINKDOT_ಭೇಟಿ_ಆರಂಭ();
        UINFO(8, indent() << "visit " << nodep);
        checkNoDot(nodep);
        VL_RESTORER(m_modSymp);
        VL_RESTORER(m_curSymp);
        m_ds.m_dotSymp = m_curSymp = m_modSymp = m_statep->getScopeSym(nodep);
        iterateChildren(nodep);
        m_ds.m_dotSymp = m_curSymp = m_modSymp = ಶೂನ್ಯಸೂಚಕ;
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCellInline* nodep) override {
        LINKDOT_ಭೇಟಿ_ಆರಂಭ();
        checkNoDot(nodep);
        UASSERT_OBJ(m_statep->forScopeCreation(), nodep, "CellInline should not exist yet");
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlink_Fr_ಹಿಂಭಾಗ()), nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCell* nodep) override {
        // Cell: Recurse inside or cleanup not founds
        LINKDOT_ಭೇಟಿ_ಆರಂಭ();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        VL_RESTORER_CLEAR(m_ಬಳಸಿದ_Pins);
        VL_RESTORER_CLEAR(m_ಬಳಸಿದ_Def_Param_Pins);
        UASSERT_OBJ(nodep->modp(), nodep,
                    "Instance has unlinked module");  // V3LinkCell should have errored out
        VL_RESTORER(m_cellp);
        VL_RESTORER(m_pinSymp);
        {
            m_cellp = nodep;
            ಆದರೆ (VN_IS(nodep->modp(), Not_ಸಿಕ್ಕಿತು_Module)) {
                // Prevent warnings about missing pin connects
                ಆದರೆ (nodep->pinsp()) nodep->pinsp()->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ()->delete_ವೃಕ್ಷ();
                ಆದರೆ (nodep->paramsp()) nodep->paramsp()->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ()->delete_ವೃಕ್ಷ();
            }
            // Need to pass the module info to this cell, so we can link up the pin names
            // However can't use m_curSymp as pin connections need to use the
            // instantiator's symbols
            ಇಲ್ಲದಿದ್ದರೆ {
                m_pinSymp = m_statep->get_ಗಂಟು_Sym(nodep->modp());
                UINFO(4, indent() << "(Backto) visit " << nodep);
                // UINFOTREE(1, nodep, "", "linkcell");
                // UINFOTREE(1, nodep->modp(), "", "linkcemd");
                iterateChildren(nodep);

                ಆದರೆ (m_statep->forPrimary())
                    ಆದರೆ (ಸ್ಥಿರಾಂಕ AstModule* ಸ್ಥಿರಾಂಕ modp = VN_CAST(nodep->modp(), Module)) {
                        ಆದರೆ (modp->hasGenericIface())
                            ಕೂಡಿಸು_Implicit_Parameters_ದ_Generic_Iface(nodep, modp);
                    }
            }
        }
        // Parent module inherits child's publicity
        // This is done bottom up in the LinkBotupVisitor stage
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstClassRefDType* nodep) override {
        // Cell: Recurse inside or cleanup not founds
        LINKDOT_ಭೇಟಿ_ಆರಂಭ();
        UINFO(5, indent() << "visit " << nodep);
        // Can be under dot if called as package::class and that class resolves, so no checkNoDot
        VL_RESTORER_CLEAR(m_ಬಳಸಿದ_Pins);
        VL_RESTORER_CLEAR(m_ಬಳಸಿದ_Def_Param_Pins);
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
    ಶೂನ್ಯ ಭೇಟಿ(AstPin* nodep) override {
        // Pin: Link to submodule's port
        LINKDOT_ಭೇಟಿ_ಆರಂಭ();
        checkNoDot(nodep);
        iterateChildren(nodep);
        ಆದರೆ (!nodep->modVarp()) {
            UASSERT_OBJ(m_pinSymp, nodep, "Pin not under instance?");
            VSymEnt* ಸ್ಥಿರಾಂಕ foundp = m_pinSymp->ಕಂಡುಹಿಡಿ_Id_Flat(nodep->name());
            ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ whatp = nodep->param() ? "parameter" : "pin";
            ಆದರೆ (!foundp) {
                ಆದರೆ (m_cellp && VN_IS(m_cellp->modp(), Primitive)
                    && (nodep->name() == "__paramNumber1" || nodep->name() == "__paramNumber2")) {
                    // Primitive parameter is really a delay2 we can just ignore
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlink_Fr_ಹಿಂಭಾಗ()), nodep);
                    ಹಿಂತಿರುಗಿ;
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->param() && !nodep->exprp() && !nodep->svDotName()) {
                    // Placeholder pin the parser makes for an empty '#()', to signal
                    // the user did type the '#()'. No further use, deleting.
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlink_Fr_ಹಿಂಭಾಗ()), nodep);
                    ಹಿಂತಿರುಗಿ;
                } ಇಲ್ಲದಿದ್ದರೆ {
                    ಸ್ಥಿರಾಂಕ std::string suggest
                        = (nodep->param()
                               ? m_statep->suggestSymFlat(m_pinSymp, nodep->name(),
                                                          Link_ಗಂಟು_Matcher_Var_Param{}, ಸುಳ್ಳು)
                               : m_statep->suggestSymFlat(m_pinSymp, nodep->name(),
                                                          Link_ಗಂಟು_Matcher_Var_IO{}, ಸುಳ್ಳು));
                    ಸ್ಥಿರಾಂಕ std::string decl
                        = ((m_cellp && m_cellp->modp())
                               ? "\n"s + nodep->warn_ಇನ್ನಷ್ಟು() + "... Location of instance's "
                                     + m_cellp->modp()->verilogKwd() + " declaration\n"
                                     + m_cellp->modp()->warnContextSecondary()
                               : "");
                    nodep->v3warn(PINNOTFOUND,
                                  ucfirst(whatp)
                                      << " not found: " << nodep->prettyNameQ() << '\n'
                                      << (suggest.ಖಾಲಿಗಳು() ? "" : nodep->warn_ಇನ್ನಷ್ಟು() + suggest)
                                      << '\n'
                                      << nodep->warnContextPrimary() << decl);
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlink_Fr_ಹಿಂಭಾಗ()), nodep);
                    ಹಿಂತಿರುಗಿ;
                }
            }
            VVarType refVarType = VVarType::UNKNOWN;
            ತಾರ್ಕಿಕ wrongPinType = ಸುಳ್ಳು;
            ಆದರೆ (AstVar* ಸ್ಥಿರಾಂಕ varp = VN_CAST(foundp->nodep(), Var)) {
                ಆದರೆ (varp->isIO() || varp->isParam() || varp->isIfaceRef()) {
                    refVarType = varp->varType();
                    nodep->modVarp(varp);
                } ಇಲ್ಲದಿದ್ದರೆ {
                    wrongPinType = ನಿಜ;
                }
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstParamTypeDType* ಸ್ಥಿರಾಂಕ typep = VN_CAST(foundp->nodep(), ParamTypeDType)) {
                refVarType = typep->varType();
                nodep->modPTypep(typep);
            } ಇಲ್ಲದಿದ್ದರೆ {
                wrongPinType = ನಿಜ;
            }
            // Don't connect parameter pin to module ports or vice versa
            ಆದರೆ (nodep->param() != (refVarType == VVarType::GPARAM)) wrongPinType = ನಿಜ;
            ಆದರೆ (wrongPinType) {
                string ಗುರಿ_Type = LinkDotState::ಗಂಟು_ಪಠ್ಯ_Type(foundp->nodep());
                ಗುರಿ_Type = VString::aOrAn(ಗುರಿ_Type) + ' ' + ಗುರಿ_Type;
                ಆದರೆ (nodep->param()) {
                    nodep->v3error("Instance attempts to override "
                                   << nodep->prettyNameQ() << " as a " << whatp << ", but it is "
                                   << ಗುರಿ_Type);
                } ಇಲ್ಲದಿದ್ದರೆ {
                    nodep->v3error("Instance attempts to connect to "
                                   << nodep->prettyNameQ() << ", but it is " << ಗುರಿ_Type);
                }
                ಹಿಂತಿರುಗಿ;
            }
            markAndCheckPinDup(nodep, foundp->nodep(), whatp);
        }
        // Early return() above when deleted
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ isDotThisReference(ಸ್ಥಿರಾಂಕ AstDot* nodep) {
        ಹಿಂತಿರುಗಿ VN_IS(nodep->lhsp(), ParseRef) && nodep->lhsp()->name() == "this";
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstDot* nodep) override {
        // Legal under a DOT: AstDot, AstParseRef, AstPackageRef, AstNodeSel, AstPackArrayDType
        //    also a DOT can be part of an expression, but only above plus
        //    AstFTaskRef are legal children
        // Dot(PackageRef, ParseRef(text))
        // Dot(Dot(ClassOrPackageRef,ClassOrPackageRef), ParseRef(text))
        // Dot(Dot(Dot(ParseRef(text), ...
        ಆದರೆ (nodep->user3_ಗಣ_Once()) ಹಿಂತಿರುಗಿ;
        LINKDOT_ಭೇಟಿ_ಆರಂಭ();
        UINFO(8, indent() << "visit " << nodep);
        UINFO(8, indent() << m_ds.ascii());
        ಸ್ಥಿರಾಂಕ Dot_ಸ್ಥಿತಿಗಳು ಕೊನೆಯ_ಸ್ಥಿತಿಗಳು = m_ds;
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ ಆರಂಭ = (m_ds.m_dotPos == DP_NONE);  // Save, as m_dotp will be changed
        ಸ್ಥಿರಾಂಕ DotPosition initialDotPos = m_ds.m_dotPos;
        VL_RESTORER(m_randSymp);
        {
            ಆದರೆ (ಆರಂಭ) {  // Starting dot sequence
                UINFOTREE(9, nodep, "", "dot-in");
                // Keep deferral from visit(AstMethodCall) for unspecialized rand target
                m_ds.init(m_curSymp, m_ds.m_unresolvedClass && rand_ಗುರಿ_Unresolved());
            }
            m_ds.m_dotp = nodep;  // Always, not just at start
            m_ds.m_dotPos = DP_ಮೊದಲ;

            ಆದರೆ (isDotThisReference(nodep)) {
                VSymEnt* classSymp = getThisClassSymp();
                // In 'randomize() with { this.member }', 'this' refers to randomized
                // object, not the calling class (IEEE 1800-2023 18.7)
                ಆದರೆ (m_randSymp && m_currentWithp) classSymp = m_randSymp;
                ಆದರೆ (!classSymp && rand_ಗುರಿ_Unresolved()) {
                    // Bind 'this' in post-V3Param pass
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!classSymp) {
                    nodep->v3error("'this' used outside class (IEEE 1800-2023 8.11)");
                    m_ds.m_dotErr = ನಿಜ;
                } ಇಲ್ಲದಿದ್ದರೆ {
                    m_ds.m_dotSymp = classSymp;
                    UINFO(8, indent() << "this. " << m_ds.ascii());
                }
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(nodep->lhsp(), ParseRef) && nodep->lhsp()->name() == "super") {
                ಸ್ಥಿರಾಂಕ VSymEnt* classSymp = getThisClassSymp();
                ಆದರೆ (!classSymp) {
                    nodep->v3error("'super' used outside class (IEEE 1800-2023 8.15)");
                    m_ds.m_dotErr = ನಿಜ;
                } ಇಲ್ಲದಿದ್ದರೆ {
                    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ classp = VN_AS(classSymp->nodep(), Class);
                    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ cextp = classp->extendsp();
                    ಆದರೆ (!cextp) {
                        nodep->v3error("'super' used on non-extended class (IEEE 1800-2023 8.15)");
                        m_ds.m_dotErr = ನಿಜ;
                    } ಇಲ್ಲದಿದ್ದರೆ {
                        ಆದರೆ (m_statep->forPrimary()
                            && m_extendsParam.ಕಂಡುಹಿಡಿ(classp) != m_extendsParam.ಕೊನೆ()) {
                            UINFO(9, indent()
                                         << "deferring until post-V3Param: " << nodep->lhsp());
                            m_ds.m_unresolvedClass = ನಿಜ;
                        } ಇಲ್ಲದಿದ್ದರೆ {
                            ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ ತಳ_Classp = cextp->classp();
                            UASSERT_OBJ(ತಳ_Classp, nodep, "Bad superclass");
                            m_ds.m_dotSymp = m_statep->get_ಗಂಟು_Sym(ತಳ_Classp);
                            m_ds.m_super = ನಿಜ;
                            UINFO(8, indent() << "super. " << m_ds.ascii());
                        }
                    }
                }
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstClassOrPackageRef* ಸ್ಥಿರಾಂಕ lhsp
                       = VN_CAST(nodep->lhsp(), ClassOrPackageRef)) {
                // m_ds.m_dotText communicates the cell prefix between stages
                UINFO(8, indent() << "iter.lhs   " << m_ds.ascii() << " " << nodep);
                iterate_And_ಮುಂದಿನ_Null(lhsp);
                ಆದರೆ (!lhsp->classOrPackageSkipp() && lhsp->name() != "local::") {
                    revisitLater(nodep);
                    m_ds = ಕೊನೆಯ_ಸ್ಥಿತಿಗಳು;
                    // Resolve function args before bailing
                    ಆದರೆ (Ast_ಗಂಟು_FTask_Ref* ಸ್ಥಿರಾಂಕ ftaskrefp = VN_CAST(nodep->rhsp(), ಗಂಟು_FTask_Ref)) {
                        iterate_And_ಮುಂದಿನ_Null(ftaskrefp->argsp());
                    }
                    ಹಿಂತಿರುಗಿ;
                }
                m_ds.m_dotPos = DP_PACKAGE;
                // nodep->lhsp() may be a new node
                ಆದರೆ (AstClassOrPackageRef* ಸ್ಥಿರಾಂಕ classOrPackageRefp
                    = VN_CAST(nodep->lhsp(), ClassOrPackageRef)) {
                    ಆದರೆ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ classOrPackageNodep
                        = classOrPackageRefp->classOrPackageSkipp()) {
                        m_ds.m_dotSymp = m_statep->get_ಗಂಟು_Sym(classOrPackageNodep);
                    }
                }
                UINFO(8, indent() << "iter.ldone " << m_ds.ascii() << " " << nodep);
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(nodep->lhsp(), Dot) && VN_AS(nodep->lhsp(), Dot)->colon()) {
                // m_ds.m_dotText communicates the cell prefix between stages
                UINFO(8, indent() << "iter.lhs   " << m_ds.ascii() << " " << nodep);
                m_ds.m_dotPos = DP_PACKAGE;
                iterate_And_ಮುಂದಿನ_Null(nodep->lhsp());
                // nodep->lhsp() may be a new node
                ಆದರೆ (AstClassOrPackageRef* ಸ್ಥಿರಾಂಕ crefp
                    = VN_CAST(nodep->lhsp(), ClassOrPackageRef)) {
                    ಆದರೆ (!crefp->classOrPackageSkipp()) {
                        revisitLater(nodep);
                        m_ds = ಕೊನೆಯ_ಸ್ಥಿತಿಗಳು;
                        // Resolve function args before bailing
                        ಆದರೆ (Ast_ಗಂಟು_FTask_Ref* ಸ್ಥಿರಾಂಕ ftaskrefp
                            = VN_CAST(nodep->rhsp(), ಗಂಟು_FTask_Ref)) {
                            iterate_And_ಮುಂದಿನ_Null(ftaskrefp->argsp());
                        }
                        ಹಿಂತಿರುಗಿ;
                    }
                }
                ಆದರೆ (m_ಕೊನೆಯ_Deferredp == nodep->lhsp()) {
                    // LHS got deferred, so this node won't be resolved. Defer it too
                    m_ds = ಕೊನೆಯ_ಸ್ಥಿತಿಗಳು;
                    // Resolve function args before bailing
                    ಆದರೆ (Ast_ಗಂಟು_FTask_Ref* ಸ್ಥಿರಾಂಕ ftaskrefp = VN_CAST(nodep->rhsp(), ಗಂಟು_FTask_Ref)) {
                        iterate_And_ಮುಂದಿನ_Null(ftaskrefp->argsp());
                    }
                    ಹಿಂತಿರುಗಿ;
                }
                UINFO(8, indent() << "iter.ldone " << m_ds.ascii() << " " << nodep);
            } ಇಲ್ಲದಿದ್ದರೆ {
                m_ds.m_dotPos = DP_ಮೊದಲ;
                m_ds.m_unresolvedGenericIface = ಸುಳ್ಳು;
                UINFO(8, indent() << "iter.lhs   " << m_ds.ascii() << " " << nodep);
                iterate_And_ಮುಂದಿನ_Null(nodep->lhsp());
                UINFO(8, indent() << "iter.ldone " << m_ds.ascii() << " " << nodep);
                ಆದರೆ (m_ds.m_unresolvedGenericIface) {
                    m_ds.m_dotPos = initialDotPos;
                    ಹಿಂತಿರುಗಿ;
                }
                // UINFOTREE(9, nodep, "", "dot-lho");
            }
            ಆದರೆ (m_statep->forPrimary() && isParamedClassRef(nodep->lhsp())) {
                // Dots of paramed classes will be linked after deparameterization
                UINFO(9, indent() << "deferring until post-V3Param: " << nodep->lhsp());
                m_ds.m_unresolvedClass = ನಿಜ;
            }
            ಆದರೆ (m_ds.m_unresolvedCell
                && (VN_IS(nodep->lhsp(), CellRef) || VN_IS(nodep->lhsp(), CellArrayRef))) {
                m_ds.m_unlinkedScopep = nodep->lhsp();
            }
            ಆದರೆ (m_ds.m_dotPos == DP_ಮೊದಲ) m_ds.m_dotPos = DP_SCOPE;
            ಆದರೆ (!m_ds.m_dotErr) {  // Once something wrong, give up
                // Top 'final' dot RHS is final RHS, else it's a
                // DOT(DOT(x,*here*),real-rhs) which we consider a RHS
                ಆದರೆ (ಆರಂಭ && m_ds.m_dotPos == DP_SCOPE) m_ds.m_dotPos = DP_FINAL;
                UINFO(8, indent() << "iter.rhs   " << m_ds.ascii() << " " << nodep);
                // m_ds.m_dotSymp points at lhsp()'s symbol table, so resolve RHS under that
                iterate_And_ಮುಂದಿನ_Null(nodep->rhsp());
                UINFO(8, indent() << "iter.rdone " << m_ds.ascii() << " " << nodep);
                // UINFOTREE(9, nodep, "", "dot-rho");
            }
            ಆದರೆ (!m_ds.m_unresolvedClass) {
                ಆದರೆ (ಆರಂಭ) {
                    Ast_ಗಂಟು* newp;
                    ಆದರೆ (m_ds.m_dotErr) {
                        newp = ಹೊಸ AstConst{nodep->fileline(), AstConst::ಬಿಟ್_False{}};
                    } ಇಲ್ಲದಿದ್ದರೆ {
                        newp = nodep->rhsp()->unlink_Fr_ಹಿಂಭಾಗ();
                    }
                    UINFOTREE(9, newp, "", "dot-out");
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                } ಇಲ್ಲದಿದ್ದರೆ {  // Dot midpoint
                    Ast_ಗಂಟು* newp = nodep->rhsp()->unlink_Fr_ಹಿಂಭಾಗ();
                    ಆದರೆ (m_ds.m_unresolvedCell) {
                        AstCellRef* ಸ್ಥಿರಾಂಕ crp = ಹೊಸ AstCellRef{
                            nodep->fileline(), nodep->name(), nodep->lhsp()->unlink_Fr_ಹಿಂಭಾಗ(), newp};
                        newp = crp;
                    }
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                }
            }
        }
        ಆದರೆ (ಆರಂಭ) {
            m_ds = ಕೊನೆಯ_ಸ್ಥಿತಿಗಳು;
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ unresolvedClass = m_ds.m_unresolvedClass;
            m_ds.m_dotp = ಕೊನೆಯ_ಸ್ಥಿತಿಗಳು.m_dotp;
            m_ds.m_unresolvedClass |= unresolvedClass;
        }
        UINFO(8, indent() << "done " << m_ds.ascii() << " " << nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstDefaultClocking* nodep) override {
        ಆದರೆ (VSymEnt* ಸ್ಥಿರಾಂಕ foundp = m_curSymp->ಕಂಡುಹಿಡಿ_Id_Fallback(nodep->name())) {
            ಆದರೆ (AstClocking* ಸ್ಥಿರಾಂಕ clockingp = VN_CAST(foundp->nodep(), Clocking)) {
                clockingp->makeDefault();
                VL_DO_DANGLING(nodep->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ(), nodep);
            } ಇಲ್ಲದಿದ್ದರೆ {
                nodep->v3error(nodep->prettyNameQ() << " is not a clocking identifier");
            }
        } ಇಲ್ಲದಿದ್ದರೆ {
            nodep->v3error("Can't find definition of clocking: " << nodep->prettyNameQ());
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Sen_ಅಂಶ* nodep) override {
        LINKDOT_ಭೇಟಿ_ಆರಂಭ();
        VL_RESTORER(m_ಒಳ_Sens);
        m_ಒಳ_Sens = ನಿಜ;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstPatMember* nodep) override {
        LINKDOT_ಭೇಟಿ_ಆರಂಭ();
        ಆದರೆ (nodep->varrefp()) ಹಿಂತಿರುಗಿ;  // only do this mapping once
        // If we have a TEXT token as our key, lookup if it's a LPARAM
        ಆದರೆ (Ast_ಪಠ್ಯ* ಸ್ಥಿರಾಂಕ textp = VN_CAST(nodep->keyp(), ಪಠ್ಯ_2)) {
            UINFO(9, indent() << "visit " << nodep);
            UINFO(9, indent() << "      " << textp);
            // Lookup
            ಆದರೆ (VSymEnt* ಸ್ಥಿರಾಂಕ foundp = m_curSymp->ಕಂಡುಹಿಡಿ_Id_Fallback(textp->ಪಠ್ಯ())) {
                ಆದರೆ (AstVar* ಸ್ಥಿರಾಂಕ varp = VN_CAST(foundp->nodep(), Var)) {
                    ಆದರೆ (varp->isParam() || varp->isGenVar()) {
                        // Attach found Text reference to PatMember
                        nodep->varrefp(
                            ಹೊಸ AstVarRef{nodep->fileline(),
                                          foundp->imported() ? foundp->classOrPackagep() : ಶೂನ್ಯಸೂಚಕ,
                                          varp, VAccess::READ});
                        UINFO(9, indent() << " new " << nodep->varrefp());
                    }
                }
                ಆದರೆ (Ast_Enum_ಅಂಶ* ಸ್ಥಿರಾಂಕ itemp = VN_CAST(foundp->nodep(), Enum_ಅಂಶ)) {
                    // Attach enum item value to PatMember
                    nodep->varrefp(
                        ಹೊಸ Ast_Enum_ಅಂಶ_Ref{nodep->fileline(), itemp, foundp->classOrPackagep()});
                    UINFO(9, indent() << " new " << itemp);
                }
            }
        }
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstParseRef* nodep) override {
        ಆದರೆ (nodep->user3_ಗಣ_Once()) ಹಿಂತಿರುಗಿ;
        LINKDOT_ಭೇಟಿ_ಆರಂಭ();
        UINFO(9, indent() << "visit " << nodep);
        UINFO(9, indent() << m_ds.ascii());
        ಆದರೆ (m_ds.m_unresolvedClass) ಹಿಂತಿರುಗಿ;
        // m_curSymp is symbol table of outer expression
        // m_ds.m_dotSymp is symbol table relative to "."'s above now
        UASSERT_OBJ(m_ds.m_dotSymp, nodep, "nullptr lookup symbol table");
        // Generally resolved during Primary, but might be at param time under AstUnlinkedRef
        UASSERT_OBJ(!m_statep->forScopeCreation(), nodep, "ParseRefs should no longer exist");
        ಸ್ಥಿರಾಂಕ Dot_ಸ್ಥಿತಿಗಳು ಕೊನೆಯ_ಸ್ಥಿತಿಗಳು = m_ds;
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ ಆರಂಭ = (m_ds.m_dotPos == DP_NONE);  // Save, as m_dotp will be changed
        ತಾರ್ಕಿಕ ಮೊದಲ = ಆರಂಭ || m_ds.m_dotPos == DP_ಮೊದಲ;

        ಆದರೆ (ಆರಂಭ) {
            m_ds.init(m_curSymp);
            // Note m_ds.m_dotp remains nullptr; this is a reference not under a dot
        }
        ಆದರೆ (nodep->name() == "super") {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: super");
            m_ds.m_dotErr = ನಿಜ;
        }
        ಆದರೆ (nodep->name() == "this") {
            iterateChildren(nodep);
            ಆದರೆ (m_statep->forPrimary()) ಹಿಂತಿರುಗಿ;  // The class might be parameterized somewhere
            ಸ್ಥಿರಾಂಕ VSymEnt* classSymp = getThisClassSymp();
            ಆದರೆ (!classSymp) {
                nodep->v3error("'this' used outside class (IEEE 1800-2023 8.11)");
                ಹಿಂತಿರುಗಿ;
            }
            AstClass* ಸ್ಥಿರಾಂಕ classp = VN_AS(classSymp->nodep(), Class);
            AstClassRefDType* ಸ್ಥಿರಾಂಕ dtypep
                = ಹೊಸ AstClassRefDType{nodep->fileline(), classp, ಶೂನ್ಯಸೂಚಕ};
            AstThisRef* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ AstThisRef{nodep->fileline(), VFlag_ಮಗು_DType{}, dtypep};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            ಹಿಂತಿರುಗಿ;
        }
        ಆದರೆ (m_ds.m_dotPos == DP_MEMBER && VN_IS(m_ds.m_dotp->lhsp(), LambdaArgRef)
            && nodep->name() == "index") {
            // 'with' statement's 'item.index'
            // m_ds.dotp->lhsp() was checked to know if `index` is directly after lambda arg ref.
            // If not, treat it as normal member select
            iterateChildren(nodep);
            AstLambdaArgRef* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ AstLambdaArgRef{
                nodep->fileline(), m_ds.m_dotp->lhsp()->name() + "__DOT__index", ನಿಜ};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            ಹಿಂತಿರುಗಿ;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (m_ds.m_dotPos == DP_MEMBER) {
            // Found a Var, everything following is membership.  {scope}.{var}.HERE {member}
            ಆದರೆ (m_ds.m_dotErr) {
                nodep->unlink_Fr_ಹಿಂಭಾಗ();  // Avoid circular node loop on errors
            } ಇಲ್ಲದಿದ್ದರೆ {
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ varEtcp = VN_AS(m_ds.m_dotp->lhsp()->unlink_Fr_ಹಿಂಭಾಗ(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ);
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ AstMemberSel{nodep->fileline(), varEtcp,  //
                                                           VFlag_ಮಗು_DType{}, nodep->name()};
                nodep->replaceWith(newp);
            }
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        } ಇಲ್ಲದಿದ್ದರೆ {
            //
            string expectWhat;
            ತಾರ್ಕಿಕ allowScope = ಸುಳ್ಳು;
            ತಾರ್ಕಿಕ allowVar = ಸುಳ್ಳು;
            ತಾರ್ಕಿಕ allowFTask = ಸುಳ್ಳು;
            ತಾರ್ಕಿಕ staticAccess = ಸುಳ್ಳು;
            ಆದರೆ (m_ds.m_disablep) {
                allowScope = ನಿಜ;
                allowFTask = ನಿಜ;
                expectWhat = "block/task";
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (m_ds.m_dotPos == DP_PACKAGE) {
                // {package-or-class}::{a}
                Ast_ಗಂಟು_Module* classOrPackagep = ಶೂನ್ಯಸೂಚಕ;
                expectWhat = "scope/variable/func";
                allowScope = ನಿಜ;
                allowVar = ನಿಜ;
                allowFTask = ನಿಜ;
                staticAccess = ನಿಜ;
                UINFO(9, "chk pkg " << m_ds.ascii() << " lhsp=" << m_ds.m_dotp->lhsp());
                UASSERT_OBJ(VN_IS(m_ds.m_dotp->lhsp(), ClassOrPackageRef), m_ds.m_dotp->lhsp(),
                            "Bad package link");
                AstClassOrPackageRef* ಸ್ಥಿರಾಂಕ cpackagerefp
                    = VN_AS(m_ds.m_dotp->lhsp(), ClassOrPackageRef);
                ಆದರೆ (cpackagerefp->name() == "local::") {
                    m_randSymp = ಶೂನ್ಯಸೂಚಕ;
                    ಮೊದಲ = ನಿಜ;
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!cpackagerefp->classOrPackageSkipp()) {
                    VSymEnt* ಸ್ಥಿರಾಂಕ foundp = m_statep->resolveClassOrPackage(
                        m_ds.m_dotSymp, cpackagerefp, ನಿಜ, ಸುಳ್ಳು, ":: reference");
                    ಆದರೆ (!foundp) ಹಿಂತಿರುಗಿ;
                    classOrPackagep = cpackagerefp->classOrPackageSkipp();
                    ಆದರೆ (classOrPackagep) {
                        m_ds.m_dotSymp = m_statep->get_ಗಂಟು_Sym(classOrPackagep);
                    } ಇಲ್ಲದಿದ್ದರೆ {
                        m_ds = ಕೊನೆಯ_ಸ್ಥಿತಿಗಳು;
                        ಹಿಂತಿರುಗಿ;
                    }
                } ಇಲ್ಲದಿದ್ದರೆ {
                    classOrPackagep = cpackagerefp->classOrPackageSkipp();
                    UASSERT_OBJ(classOrPackagep, m_ds.m_dotp->lhsp(), "Bad package link");
                    m_ds.m_dotSymp = m_statep->get_ಗಂಟು_Sym(classOrPackagep);
                }
                m_ds.m_dotPos = DP_SCOPE;
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (m_ds.m_dotPos == DP_SCOPE || m_ds.m_dotPos == DP_ಮೊದಲ) {
                // {a}.{b}, where {a} maybe a module name
                // or variable, where dotting into structure member
                expectWhat = "scope/variable";
                allowScope = ನಿಜ;
                allowVar = ನಿಜ;
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (m_ds.m_dotPos == DP_NONE) {
                expectWhat = "variable";
                allowVar = ನಿಜ;
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (m_ds.m_dotPos == DP_FINAL) {
                expectWhat = "variable/method";
                allowFTask = ನಿಜ;
                allowVar = ನಿಜ;
            } ಇಲ್ಲದಿದ್ದರೆ {
                UINFO(1, "ds=" << m_ds.ascii());
                nodep->v3fatalSrc("Unhandled DotPosition");
            }
            // Lookup
            VSymEnt* foundp;
            string baddot;
            VSymEnt* okSymp = ಶೂನ್ಯಸೂಚಕ;
            // Restricted 'with (id_list)': only id_list names bind into target class.
            ಆದರೆ (m_randSymp
                && (!m_currentWithp || m_currentWithp->name_Resolves_ಗಮ್ಯ_ಗುರಿ(nodep->name()))) {
                foundp = m_randSymp->ಕಂಡುಹಿಡಿ_Id_Flat(nodep->name());
                ಆದರೆ (foundp) {
                    ಆದರೆ (m_currentWithp && m_currentWithp->restricted()) {
                        m_restricted_ಹೆಸರುಗಳು_ಬಳಸಿದ.ಸೇರಿಸು(nodep->name());
                    }
                    ಆದರೆ (!ಆರಂಭ) m_ds.m_dotPos = DP_MEMBER;
                    ಆದರೆ (!m_currentWithp) {
                        UASSERT_OBJ(m_rand_ವಿಧಾನ_Callp, nodep, "Expected to be under randomize()");
                        // This will start failing once complex expressions are allowed on the LHS
                        // of randomize() with args
                        UASSERT_OBJ(VN_IS(m_rand_ವಿಧಾನ_Callp->fromp(), VarRef), m_rand_ವಿಧಾನ_Callp,
                                    "Expected simple randomize target");
                        // A ParseRef is used here so that the dot RHS gets resolved
                        AstMemberSel* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ AstMemberSel{
                            nodep->fileline(), m_rand_ವಿಧಾನ_Callp->fromp()->clone_ವೃಕ್ಷ(ಸುಳ್ಳು),
                            VFlag_ಮಗು_DType{}, nodep->name()};
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                        ಹಿಂತಿರುಗಿ;
                    }
                    UINFO(9, indent() << "randomize-with fromSym " << foundp->nodep());
                    AstLambdaArgRef* ಸ್ಥಿರಾಂಕ lambdaRefp
                        = ಹೊಸ AstLambdaArgRef{nodep->fileline(), "__Vrandwith_obj", ಸುಳ್ಳು};
                    AstMemberSel* newp = ಹೊಸ AstMemberSel{nodep->fileline(), lambdaRefp,
                                                          VFlag_ಮಗು_DType{}, nodep->name()};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                    ಹಿಂತಿರುಗಿ;
                }
            }
            ಆದರೆ (allowScope) {
                foundp = m_statep->ಕಂಡುಹಿಡಿ_Dotted(nodep->fileline(), m_ds.m_dotSymp, nodep->name(),
                                              baddot, okSymp, ಮೊದಲ);  // Maybe nullptr
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಮೊದಲ) {
                foundp = m_ds.m_dotSymp->ಕಂಡುಹಿಡಿ_Id_Fallback(nodep->name());
            } ಇಲ್ಲದಿದ್ದರೆ {
                foundp = m_ds.m_dotSymp->ಕಂಡುಹಿಡಿ_Id_Flat(nodep->name());
            }
            ಆದರೆ (!foundp && m_ds.m_dotp && isDotThisReference(m_ds.m_dotp)) {
                ಸ್ಥಿರಾಂಕ AstClass* ಸ್ಥಿರಾಂಕ classp = VN_CAST(m_ds.m_dotSymp->nodep(), Class);
                ಆದರೆ (classp && classp->isCovergroup() && classp->covergroupEnclosingClassp()) {
                    VSymEnt* ಸ್ಥಿರಾಂಕ enclosingClassSymp
                        = m_statep->get_ಗಂಟು_Sym(classp->covergroupEnclosingClassp());
                    foundp = enclosingClassSymp->ಕಂಡುಹಿಡಿ_Id_Fallback(nodep->name());
                    ಆದರೆ (foundp) m_ds.m_dotSymp = enclosingClassSymp;
                }
            }
            // If not found in modport, check interface fallback for parameters and typedefs.
            // Parameters and typedefs are always visible through a modport (IEEE 1800-2023 25.5).
            // This mirrors the VarXRef modport parameter fallback in visit(AstVarXRef).
            ಆದರೆ (!foundp && VN_IS(m_ds.m_dotSymp->nodep(), Modport)
                && m_ds.m_dotSymp->fallbackp()) {
                VSymEnt* ಸ್ಥಿರಾಂಕ ifaceFallbackp = m_ds.m_dotSymp->fallbackp();
                VSymEnt* ಸ್ಥಿರಾಂಕ ifaceFoundp = ifaceFallbackp->ಕಂಡುಹಿಡಿ_Id_Flat(nodep->name());
                ಆದರೆ (ifaceFoundp) {
                    ಆದರೆ (ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ varp = VN_CAST(ifaceFoundp->nodep(), Var)) {
                        ಆದರೆ (varp->isParam()) foundp = ifaceFoundp;
                    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(ifaceFoundp->nodep(), Typedef)
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
            ಆದರೆ (!foundp && !okSymp && VN_IS(m_ds.m_dotSymp->nodep(), Modport)) {
                okSymp = m_ds.m_dotSymp;
            }
            ಆದರೆ (foundp) {
                UINFO(9, indent() << "found=se" << cvt_ಗಮ್ಯ_Hex(foundp) << "  exp=" << expectWhat
                                  << "  n=" << foundp->nodep());
            }
            // What fell out?
            ತಾರ್ಕಿಕ ok = ಸುಳ್ಳು;
            // Special case: waiting on clocking event
            ಆದರೆ (m_ಒಳ_Sens && foundp && m_ds.m_dotPos != DP_SCOPE && m_ds.m_dotPos != DP_ಮೊದಲ) {
                ಆದರೆ (AstClocking* ಸ್ಥಿರಾಂಕ clockingp = sensClockingp(foundp->nodep())) {
                    foundp = getCreateClockingEventSymEnt(clockingp);
                }
            }
            ಆದರೆ (!foundp) {
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(foundp->nodep(), Cell) || VN_IS(foundp->nodep(), ಗಂಟು_ಖಂಡ)
                       || VN_IS(foundp->nodep(), Gen_ಖಂಡ)
                       || VN_IS(foundp->nodep(), Netlist)  // for $root
                       || VN_IS(foundp->nodep(), Module)) {  // if top
                ಆದರೆ (allowScope) {
                    ok = ನಿಜ;
                    m_ds.m_dot_ಪಠ್ಯ = VString::dot(m_ds.m_dot_ಪಠ್ಯ, ".", nodep->name());
                    m_ds.m_dotSymp = foundp;
                    m_ds.m_dotPos = DP_SCOPE;
                    ಆದರೆ (m_ds.m_disablep && VN_IS(foundp->nodep(), ಗಂಟು_ಖಂಡ)) {
                        // Possibly it is not the final link. If we are under dot and not in its
                        // last component, `targetp()` field will be overwritten by next components
                        m_ds.m_disablep->targetp(foundp->nodep());
                    }
                    ಆದರೆ (VN_IS(foundp->nodep(), Gen_ಖಂಡ)) {
                        m_ds.m_genBlk = ನಿಜ;
                        ಆದರೆ (m_ds.m_disablep) {
                            m_ds.m_disablep->v3warn(
                                E_UNSUPPORTED,
                                "Unsupported: Generate block referenced by disable");
                        }
                    }
                    // Upper AstDot visitor will handle it from here
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(foundp->nodep(), Cell) && allowVar) {
                    AstCell* ಸ್ಥಿರಾಂಕ cellp = VN_AS(foundp->nodep(), Cell);
                    ಆದರೆ (VN_IS(cellp->modp(), Iface)) {
                        // Interfaces can be referenced like a variable for interconnect
                        VSymEnt* ಸ್ಥಿರಾಂಕ cellEntp = m_statep->get_ಗಂಟು_Sym(cellp);
                        UASSERT_OBJ(cellEntp, nodep, "No interface sym entry");
                        VSymEnt* ಸ್ಥಿರಾಂಕ ಪೋಷಕ_Entp
                            = cellEntp->parentp();  // Container of the var; probably a module or
                                                    // generate begin
                        AstVar* ಸ್ಥಿರಾಂಕ ifaceRefVarp
                            = ಕಂಡುಹಿಡಿ_Iface_ತುದಿ_Varp(nodep, ಪೋಷಕ_Entp, nodep->name());
                        //
                        ok = ನಿಜ;
                        m_ds.m_dotSymp = foundp;
                        m_ds.m_dotPos = DP_SCOPE;
                        UINFO(9, indent() << " cell -> iface varref " << foundp->nodep());
                        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newp;
                        ಆದರೆ (m_ds.m_dot_ಪಠ್ಯ != "") {
                            newp = ಹೊಸ AstVarXRef{nodep->fileline(), ifaceRefVarp, m_ds.m_dot_ಪಠ್ಯ,
                                                  VAccess::READ};
                            ಆದರೆ (m_ds.m_unresolvedCell && m_ds.m_unlinkedScopep) {
                                newp = ಹೊಸ AstUnlinkedRef{nodep->fileline(), newp, newp->name(),
                                                          m_ds.m_unlinkedScopep->unlink_Fr_ಹಿಂಭಾಗ()};
                                m_ds.m_unlinkedScopep = ಶೂನ್ಯಸೂಚಕ;
                                m_ds.m_unresolvedCell = ಸುಳ್ಳು;
                                m_ds.m_dot_ಪಠ್ಯ = "";
                            }
                        } ಇಲ್ಲದಿದ್ದರೆ {
                            AstVarRef* ಸ್ಥಿರಾಂಕ refp
                                = ಹೊಸ AstVarRef{nodep->fileline(), ifaceRefVarp, VAccess::READ};
                            newp = refp;
                        }
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(cellp->modp(), Not_ಸಿಕ್ಕಿತು_Module)) {
                        cellp->modNameFileline()->v3error("Cannot find file containing interface: "
                                                          << cellp->modp()->prettyNameQ());
                    }
                }
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (allowScope && !allowFTask && VN_IS(foundp->nodep(), ಗಂಟು_FTask)) {
                // Function/task used as scope for static variable access (IEEE std 1800-2017 6.21)
                // Don't set m_dotText so the static variable resolves as a VarRef rather than
                // VarXRef. This is so that V3Begin won't move it out of the function/task's scope.
                ok = ನಿಜ;
                m_ds.m_dotSymp = foundp;
                m_ds.m_dotPos = DP_SCOPE;
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (allowFTask && VN_IS(foundp->nodep(), ಗಂಟು_FTask)) {
                Ast_ಗಂಟು_FTask_Ref* taskrefp;
                ಆದರೆ (VN_IS(foundp->nodep(), Task)) {
                    taskrefp = ಹೊಸ AstTaskRef{nodep->fileline(), nodep->name()};
                } ಇಲ್ಲದಿದ್ದರೆ {
                    taskrefp = ಹೊಸ AstFuncRef{nodep->fileline(), nodep->name()};
                }
                nodep->replaceWith(taskrefp);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                m_ds = ಕೊನೆಯ_ಸ್ಥಿತಿಗಳು;
                ಹಿಂತಿರುಗಿ;
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstVar* ಸ್ಥಿರಾಂಕ varp = ಸಿಕ್ಕಿತು_ಗಮ್ಯ_Varp(foundp, nodep, VAccess::READ)) {
                // Preserve explicit 'this' references to members inherited from
                // generic classes until after specialization.
                ಆದರೆ (m_statep->forPrimary() && m_ds.m_dotp && isDotThisReference(m_ds.m_dotp)
                    && foundp->classOrPackagep() != m_modp
                    && VN_IS(foundp->classOrPackagep(), Class)
                    && foundp->classOrPackagep()->hasGParam()) {
                    m_ds.m_unresolvedClass = ನಿಜ;
                    ಹಿಂತಿರುಗಿ;
                }
                AstIfaceRefDType* ಸ್ಥಿರಾಂಕ ifacerefp
                    = LinkDotState::ifaceRefFromArray(varp->subDTypep());
                ಆದರೆ (varp->isIfaceRef() && m_genericIfaceModule
                    && VN_IS(varp->ಮಗು_DTypep(), IfaceGenericDType) && !ಆರಂಭ) {
                    // Defer only dotted member access ('d.PARAM'), as V3Param must specialize
                    // first; a standalone ref ('.x(d)') resolves via allowVar below now
                    ok = ನಿಜ;
                    m_ds.m_unresolvedGenericIface = ನಿಜ;
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ifacerefp && varp->isIfaceRef()) {
                    UASSERT_OBJ(ifacerefp->iface_ಮೂಲಕ_Cellp(), ifacerefp, "Unlinked interface");
                    // Really this is a scope reference into an interface
                    UINFO(9, indent() << "varref-ifaceref " << m_ds.m_dot_ಪಠ್ಯ << "  " << nodep);
                    m_ds.m_dot_ಪಠ್ಯ = VString::dot(m_ds.m_dot_ಪಠ್ಯ, ".", nodep->name());
                    // If modport specified, use modport symbol table for lookup of virtual ports
                    ಆದರೆ (ifacerefp->modportp() && m_statep->exists_ಗಂಟು_Sym(ifacerefp->modportp())) {
                        m_ds.m_dotSymp = m_statep->get_ಗಂಟು_Sym(ifacerefp->modportp());
                    } ಇಲ್ಲದಿದ್ದರೆ {
                        m_ds.m_dotSymp = m_statep->get_ಗಂಟು_Sym(ifacerefp->iface_ಮೂಲಕ_Cellp());
                    }
                    m_ds.m_dotPos = DP_SCOPE;
                    ok = ನಿಜ;

                    AstVarRef* ಸ್ಥಿರಾಂಕ refp = ಹೊಸ AstVarRef{nodep->fileline(), varp, VAccess::READ};
                    nodep->replaceWith(refp);

                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (allowVar) {
                    Ast_ಗಂಟು* newp;
                    ಆದರೆ (m_ds.m_dot_ಪಠ್ಯ != "") {
                        AstVarXRef* ಸ್ಥಿರಾಂಕ refp
                            = ಹೊಸ AstVarXRef{nodep->fileline(), nodep->name(), m_ds.m_dot_ಪಠ್ಯ,
                                             VAccess::READ};  // lvalue'ness computed later
                        refp->varp(varp);
                        // Not linked again after V3LinkLValue, so that reports writing it
                        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstModportVarRef* ಸ್ಥಿರಾಂಕ mvarp
                            = VN_CAST(foundp->nodep(), ModportVarRef)) {
                            ಆದರೆ (m_statep->forParamed() && mvarp->direction().isReadOnly()) {
                                refp->readOnlyModport(ನಿಜ);
                            }
                        }
                        refp->ಒಳಗೊಂಡಿದೆ_Gen_ಖಂಡ(m_ds.m_genBlk);
                        m_ds.m_dot_ಪಠ್ಯ = "";
                        ಆದರೆ (m_ds.m_unresolvedCell && m_ds.m_unlinkedScopep) {
                            ಸ್ಥಿರಾಂಕ string dotted = refp->dotted();
                            ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t pos = dotted.ಕಂಡುಹಿಡಿ("__BRA__??__KET__");
                            // Arrays of interfaces all have the same parameters
                            ಆದರೆ (pos != string::npos && varp->isParam()
                                && VN_IS(m_ds.m_unlinkedScopep, CellArrayRef)) {
                                refp->dotted(dotted.substr(0, pos));
                                newp = refp;
                            } ಇಲ್ಲದಿದ್ದರೆ {
                                UINFO(9, indent() << "deferring until post-V3Param: " << refp);
                                newp = ಹೊಸ AstUnlinkedRef{nodep->fileline(), refp, refp->name(),
                                                          m_ds.m_unlinkedScopep->unlink_Fr_ಹಿಂಭಾಗ()};
                                m_ds.m_unlinkedScopep = ಶೂನ್ಯಸೂಚಕ;
                                m_ds.m_unresolvedCell = ಸುಳ್ಳು;
                            }
                        } ಇಲ್ಲದಿದ್ದರೆ {
                            newp = refp;
                        }
                    } ಇಲ್ಲದಿದ್ದರೆ {
                        ಆದರೆ (staticAccess && !varp->lifetime().isStatic() && !varp->isParam()) {
                            // TODO bug4077
                            // nodep->v3error("Static access to non-static member variable "
                            //                << varp->prettyNameQ() << endl);
                        }
                        AstVarRef* ಸ್ಥಿರಾಂಕ refp = ಹೊಸ AstVarRef{
                            nodep->fileline(), varp, VAccess::READ};  // lvalue'ness computed later
                        refp->classOrPackagep(foundp->classOrPackagep());
                        newp = refp;
                    }
                    UINFO(9, indent() << "new " << newp);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                    m_ds.m_dotPos = DP_MEMBER;
                    ok = ನಿಜ;
                }
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ AstModport* ಸ್ಥಿರಾಂಕ modportp = VN_CAST(foundp->nodep(), Modport)) {
                // A scope reference into an interface's modport (not
                // necessarily at a pin connection)
                UINFO(9, indent() << "cell-ref-to-modport " << m_ds.m_dot_ಪಠ್ಯ << "  " << nodep);
                UINFO(9, indent() << "unlinked " << m_ds.m_unlinkedScopep);
                UINFO(9,
                      indent() << "dotSymp " << m_ds.m_dotSymp << " " << m_ds.m_dotSymp->nodep());
                // Iface was the previously dotted component
                ಆದರೆ (!m_ds.m_dotSymp || !VN_IS(m_ds.m_dotSymp->nodep(), Cell)
                    || !VN_AS(m_ds.m_dotSymp->nodep(), Cell)->modp()
                    || !VN_IS(VN_AS(m_ds.m_dotSymp->nodep(), Cell)->modp(), Iface)) {
                    nodep->v3error("Modport not referenced as <interface>."
                                   << modportp->prettyNameQ());
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!VN_AS(m_ds.m_dotSymp->nodep(), Cell)->modp()
                           || !VN_IS(VN_AS(m_ds.m_dotSymp->nodep(), Cell)->modp(), Iface)) {
                    nodep->v3error("Modport not referenced from underneath an interface: "
                                   << modportp->prettyNameQ());
                } ಇಲ್ಲದಿದ್ದರೆ {
                    AstCell* ಸ್ಥಿರಾಂಕ cellp = VN_AS(m_ds.m_dotSymp->nodep(), Cell);
                    UASSERT_OBJ(cellp, nodep, "Modport not referenced from an instance");
                    VSymEnt* ಸ್ಥಿರಾಂಕ cellEntp = m_statep->get_ಗಂಟು_Sym(cellp);
                    UASSERT_OBJ(cellEntp, nodep, "No interface sym entry");
                    VSymEnt* ಸ್ಥಿರಾಂಕ ಪೋಷಕ_Entp
                        = cellEntp->parentp();  // Container of the var; probably a
                                                // module or generate begin
                    // We drop __BRA__??__KET__ as cells don't have that naming yet
                    AstVar* ಸ್ಥಿರಾಂಕ ifaceRefVarp
                        = ಕಂಡುಹಿಡಿ_Iface_ತುದಿ_Varp(nodep, ಪೋಷಕ_Entp, cellp->name());
                    //
                    ok = ನಿಜ;
                    m_ds.m_dot_ಪಠ್ಯ = VString::dot(m_ds.m_dot_ಪಠ್ಯ, ".", nodep->name());
                    m_ds.m_dotSymp = foundp;
                    m_ds.m_dotPos = DP_SCOPE;
                    UINFO(9, indent() << "modport -> iface varref " << foundp->nodep());
                    // We lose the modport name here, so we cannot detect mismatched modports.
                    AstVarRef* ಸ್ಥಿರಾಂಕ refp
                        = ಹೊಸ AstVarRef{nodep->fileline(), ifaceRefVarp, VAccess::READ};
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newp = refp;

                    ಸ್ವಯಂಚಾಲಿತ* ಸ್ಥಿರಾಂಕ cellarrayrefp = VN_CAST(m_ds.m_unlinkedScopep, CellArrayRef);
                    ಆದರೆ (cellarrayrefp) {
                        // iface[vec].modport became CellArrayRef(iface, lsb)
                        // Convert back to SelBit(iface, lsb)
                        UINFO(9, indent() << "Array modport to SelBit " << cellarrayrefp);
                        newp = ಹೊಸ Ast_Sel_ಬಿಟ್{cellarrayrefp->fileline(), newp,
                                             cellarrayrefp->selp()->unlink_Fr_ಹಿಂಭಾಗ()};
                        newp->user3(ನಿಜ);  // Don't process again
                        VL_DO_DANGLING(ತಳ್ಳು_Deletep(cellarrayrefp->unlink_Fr_ಹಿಂಭಾಗ()), cellarrayrefp);
                        m_ds.m_unlinkedScopep = ಶೂನ್ಯಸೂಚಕ;
                    }
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                }
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstConstraint* ಸ್ಥಿರಾಂಕ defp = VN_CAST(foundp->nodep(), Constraint)) {
                Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ AstConstraintRef{nodep->fileline(), ಶೂನ್ಯಸೂಚಕ, defp};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                ok = ನಿಜ;
                m_ds.m_dotPos = DP_MEMBER;
                m_ds.m_dot_ಪಠ್ಯ = "";
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstClass* ಸ್ಥಿರಾಂಕ defp = VN_CAST(foundp->nodep(), Class)) {
                ಆದರೆ (allowVar) {
                    ಆದರೆ (m_ds.m_dotPos == DP_SCOPE && !staticAccess) {
                        // Continue hierarchical lookup in the class scope.
                        m_ds.m_dotSymp = foundp;
                    } ಇಲ್ಲದಿದ್ದರೆ {
                        AstRefDType* ಸ್ಥಿರಾಂಕ newp
                            = ಹೊಸ AstRefDType{nodep->fileline(), nodep->name()};
                        replaceWithCheckBreak(nodep, newp);
                        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                    }
                    ok = ನಿಜ;
                    m_ds.m_dot_ಪಠ್ಯ = "";
                } ಇಲ್ಲದಿದ್ದರೆ {
                    (ಶೂನ್ಯ)defp;  // Prevent unused variable warning
                }
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstMemberDType* ಸ್ಥಿರಾಂಕ defp = VN_CAST(foundp->nodep(), MemberDType)) {
                ಆದರೆ (allowVar) {
                    check_Member_Decl_ಕ್ರಮ(nodep, defp);
                    AstRefDType* ಸ್ಥಿರಾಂಕ refp = ಹೊಸ AstRefDType{nodep->fileline(), nodep->name()};
                    refp->refDTypep(defp);
                    replaceWithCheckBreak(nodep, refp);
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                    ok = ನಿಜ;
                    m_ds.m_dot_ಪಠ್ಯ = "";
                }
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (Ast_Enum_ಅಂಶ* ಸ್ಥಿರಾಂಕ valuep = VN_CAST(foundp->nodep(), Enum_ಅಂಶ)) {
                ಆದರೆ (allowVar) {
                    Ast_Enum_ಅಂಶ_Ref* ಸ್ಥಿರಾಂಕ refp
                        = ಹೊಸ Ast_Enum_ಅಂಶ_Ref{nodep->fileline(), valuep, foundp->classOrPackagep()};
                    Ast_ಗಂಟು* newp = refp;
                    // Hierarchical reference, relinked after V3Param as the referenced
                    // module may be specialized, similar to AstVarXRef
                    refp->dotted(m_ds.m_dot_ಪಠ್ಯ);
                    refp->ಒಳಗೊಂಡಿದೆ_Gen_ಖಂಡ(m_ds.m_genBlk);
                    ಆದರೆ (m_ds.m_unresolvedCell && m_ds.m_unlinkedScopep) {
                        UINFO(9, indent() << "deferring until post-V3Param: " << refp);
                        newp = ಹೊಸ AstUnlinkedRef{nodep->fileline(), refp, refp->name(),
                                                  m_ds.m_unlinkedScopep->unlink_Fr_ಹಿಂಭಾಗ()};
                        m_ds.m_unlinkedScopep = ಶೂನ್ಯಸೂಚಕ;
                        m_ds.m_unresolvedCell = ಸುಳ್ಳು;
                    }
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                    ok = ನಿಜ;
                    m_ds.m_dot_ಪಠ್ಯ = "";
                }
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ AstLambdaArgRef* ಸ್ಥಿರಾಂಕ argrefp
                       = VN_CAST(foundp->nodep(), LambdaArgRef)) {
                ಆದರೆ (allowVar) {
                    Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ newp
                        = ಹೊಸ AstLambdaArgRef{nodep->fileline(), argrefp->name(), ಸುಳ್ಳು};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                    ok = ನಿಜ;
                    m_ds.m_dotPos = DP_MEMBER;
                    m_ds.m_dot_ಪಠ್ಯ = "";
                }
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(foundp->nodep(), Clocking)) {
                m_ds.m_dotSymp = foundp;
                ಆದರೆ (m_ds.m_dot_ಪಠ್ಯ != "") m_ds.m_dot_ಪಠ್ಯ += "." + nodep->name();
                ok = m_ds.m_dotPos == DP_SCOPE || m_ds.m_dotPos == DP_ಮೊದಲ;
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ AstModportClockingRef* ಸ್ಥಿರಾಂಕ clockingRefp
                       = VN_CAST(foundp->nodep(), ModportClockingRef)) {
                // Clocking block accessed through a modport - redirect to actual clocking
                m_ds.m_dotSymp = m_statep->get_ಗಂಟು_Sym(clockingRefp->clockingp());
                ಆದರೆ (m_ds.m_dot_ಪಠ್ಯ != "") m_ds.m_dot_ಪಠ್ಯ += "." + nodep->name();
                ok = m_ds.m_dotPos == DP_SCOPE || m_ds.m_dotPos == DP_ಮೊದಲ;
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_FTask* ಸ್ಥಿರಾಂಕ ftaskp = VN_CAST(foundp->nodep(), ಗಂಟು_FTask)) {
                ಆದರೆ (!ftaskp->isFunction() || ftaskp->class_ವಿಧಾನ()) {
                    ok = m_ds.m_dotPos == DP_NONE;
                    ಆದರೆ (ok) {
                        // The condition is true for tasks,
                        // properties and void functions.
                        // In these cases, the parentheses may be skipped.
                        // Also SV class methods can be called without parens
                        AstFuncRef* ಸ್ಥಿರಾಂಕ funcRefp
                            = ಹೊಸ AstFuncRef{nodep->fileline(), nodep->name()};
                        nodep->replaceWith(funcRefp);
                        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                    }
                }
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstTypedef* ಸ್ಥಿರಾಂಕ defp = VN_CAST(foundp->nodep(), Typedef)) {
                ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ ifaceFinalSegmentAllowed
                    = (m_ds.m_dotPos == DP_FINAL) && m_ds.m_dotSymp
                      && (VN_IS(m_ds.m_dotSymp->nodep(), Modport)
                          || (VN_IS(m_ds.m_dotSymp->nodep(), Cell)
                              && VN_CAST(m_ds.m_dotSymp->nodep(), Cell)->modp()
                              && VN_IS(VN_CAST(m_ds.m_dotSymp->nodep(), Cell)->modp(), Iface)));
                // Allow interface typedef references even without IfaceCapture
                // The dependency graph handles resolution when IfaceCapture is disabled
                ok = (m_ds.m_dotPos == DP_NONE || m_ds.m_dotPos == DP_SCOPE
                      || ifaceFinalSegmentAllowed);
                ಆದರೆ (ifaceFinalSegmentAllowed) {
                    UINFO(9, indent() << "allow final-segment typedef name="
                                      << nodep->prettyNameQ() << " dotText='" << m_ds.m_dot_ಪಠ್ಯ
                                      << "' dotSym=" << m_ds.m_dotSymp);
                }
                ಆದರೆ (ok) {
                    AstRefDType* ಸ್ಥಿರಾಂಕ refp = ಹೊಸ AstRefDType{nodep->fileline(), nodep->name()};
                    // Don't check if typedef is to a <type T>::<reference> as might not be
                    // resolved yet
                    ಆದರೆ (m_ds.m_dotPos == DP_NONE) check_Decl_ಕ್ರಮ(nodep, defp);
                    refp->typedefp(defp);

                    V3LinkDotIfaceCapture::captureTypedefContext(
                        refp, "typedef", static_cast<ಪೂರ್ಣಾಂಕ>(m_ds.m_dotPos), m_ds.m_dot_ಪಠ್ಯ,
                        m_ds.m_dotSymp, m_modp, [ಇದು]() { ಹಿಂತಿರುಗಿ indent(); });

                    ಆದರೆ (VN_IS(nodep->backp(), Sel_ಹೊರತೆಗೆ)) {
                        m_packedArrayDtp = refp;
                    } ಇಲ್ಲದಿದ್ದರೆ {
                        replaceWithCheckBreak(nodep, refp);
                        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                    }
                }
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstParamTypeDType* ಸ್ಥಿರಾಂಕ defp = VN_CAST(foundp->nodep(), ParamTypeDType)) {
                ok = (m_ds.m_dotPos == DP_NONE || m_ds.m_dotPos == DP_SCOPE
                      || m_ds.m_dotPos == DP_FINAL);
                ಆದರೆ (ok) {
                    AstRefDType* ಸ್ಥಿರಾಂಕ refp = ಹೊಸ AstRefDType{nodep->fileline(), nodep->name()};
                    refp->refDTypep(defp);

                    V3LinkDotIfaceCapture::captureTypedefContext(
                        refp, "paramtype", static_cast<ಪೂರ್ಣಾಂಕ>(m_ds.m_dotPos), m_ds.m_dot_ಪಠ್ಯ,
                        m_ds.m_dotSymp, m_modp, [ಇದು]() { ಹಿಂತಿರುಗಿ indent(); });

                    ಆದರೆ (VN_IS(nodep->backp(), Sel_ಹೊರತೆಗೆ)) {
                        m_packedArrayDtp = refp;
                    } ಇಲ್ಲದಿದ್ದರೆ {
                        replaceWithCheckBreak(nodep, refp);
                        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                    }
                }
            }
            ಆದರೆ (!ok) {
                ಆದರೆ (m_insideClassExtParam) {
                    // Don't throw error if the reference is inside a class that extends a param,
                    // because some members can't be linked in such a case. m_insideClassExtParam
                    // may be true only in the first stage of linking.
                    // Mark that the Dot statement can't be resolved.
                    m_ds.m_unresolvedClass = ನಿಜ;
                    // If the symbol was a scope name, it would be resolved.
                    m_ds.m_dotPos = DP_MEMBER;
                } ಇಲ್ಲದಿದ್ದರೆ {
                    // Cells/interfaces can't be implicit
                    ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ checkImplicit
                        = (!m_ds.m_dotp && m_ds.m_dot_ಪಠ್ಯ == "" && !m_ds.m_disablep && !foundp);
                    ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ err
                        = !(checkImplicit && m_statep->implicitOk(m_modp, nodep->name()));
                    ಆದರೆ (err) {
                        ಆದರೆ (foundp) {
                            nodep->v3error("Found definition of '"
                                           << m_ds.m_dot_ಪಠ್ಯ << (m_ds.m_dot_ಪಠ್ಯ == "" ? "" : ".")
                                           << nodep->prettyName() << "'" << " as a "
                                           << foundp->nodep()->typeName() << " but expected a "
                                           << expectWhat);
                        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (m_ds.m_dot_ಪಠ್ಯ == "") {
                            UINFO(1, "   ErrParseRef curSymp=se" << cvt_ಗಮ್ಯ_Hex(m_curSymp)
                                                                 << " ds=" << m_ds.ascii());
                            ಸ್ಥಿರಾಂಕ string suggest = m_statep->suggestSymFallback(
                                m_ds.m_dotSymp, nodep->name(), VNodeMatcher{});
                            nodep->v3error(
                                "Can't find definition of "
                                << expectWhat << ": " << nodep->prettyNameQ() << '\n'
                                << (suggest.ಖಾಲಿಗಳು() ? "" : nodep->warn_ಇನ್ನಷ್ಟು() + suggest));
                        } ಇಲ್ಲದಿದ್ದರೆ {
                            nodep->v3error(
                                "Can't find definition of "
                                << (!baddot.ಖಾಲಿಗಳು() ? Ast_ಗಂಟು::prettyNameQ(baddot)
                                                    : nodep->prettyNameQ())
                                << " in dotted " << expectWhat << ": '"
                                << m_ds.m_dot_ಪಠ್ಯ + "." + nodep->prettyName() << "'\n"
                                << nodep->warnContextPrimary()
                                << (okSymp ? okSymp->cellErrorScopes(
                                                 nodep, Ast_ಗಂಟು::prettyName(m_ds.m_dot_ಪಠ್ಯ))
                                           : ""));
                        }
                        m_ds.m_dotErr = ನಿಜ;
                    }
                    ಆದರೆ (checkImplicit) {
                        // Create if implicit, and also if error (so only complain once)
                        // Else if a scope is allowed, making a signal won't help error cascade
                        AstVar* ಸ್ಥಿರಾಂಕ varp
                            = createImplicitVar(m_curSymp, nodep, m_modp, m_modSymp, err);
                        AstVarRef* ಸ್ಥಿರಾಂಕ newp
                            = ಹೊಸ AstVarRef{nodep->fileline(), varp, VAccess::READ};
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                    }
                }
            }
        }
        ಆದರೆ (ಆರಂಭ) m_ds = ಕೊನೆಯ_ಸ್ಥಿತಿಗಳು;
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstClassOrPackageRef* nodep) override {
        // Class: Recurse inside or cleanup not founds
        // checkNoDot not appropriate, can be under a dot
        LINKDOT_ಭೇಟಿ_ಆರಂಭ();
        UINFO(8, indent() << "visit " << nodep);
        UINFO(9, indent() << m_ds.ascii());
        VL_RESTORER_CLEAR(m_ಬಳಸಿದ_Pins);
        VL_RESTORER_CLEAR(m_ಬಳಸಿದ_Def_Param_Pins);
        UASSERT_OBJ(m_statep->forPrimary() || !nodep->paramsp(), nodep,
                    "class reference parameter not removed by V3Param");
        {
            VL_RESTORER_COPY(m_ds);
            VL_RESTORER(m_pinSymp);

            ಆದರೆ (!nodep->classOrPackageSkipp() && !nodep->classOrPackageNodep()
                && nodep->name() != "local::") {
                ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ deferIfUnresolved = m_statep->forPrimary() && m_insideClassExtParam;
                m_statep->resolveClassOrPackage(m_ds.m_dotSymp, nodep, m_ds.m_dotPos != DP_PACKAGE,
                                                ಸುಳ್ಳು, ":: reference", deferIfUnresolved);
            }

            // ClassRef's have pins, so track
            ಆದರೆ (nodep->classOrPackageSkipp()) {
                m_pinSymp = m_statep->get_ಗಂಟು_Sym(nodep->classOrPackageSkipp());
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->name() != "local::") {
                ಹಿಂತಿರುಗಿ;
            }

            AstClass* ಸ್ಥಿರಾಂಕ refClassp = VN_CAST(nodep->classOrPackageSkipp(), Class);
            // Make sure any extends() are properly imported within referenced class
            ಆದರೆ (refClassp && !m_statep->forPrimary()) classExtendImport(refClassp);

            ಆದರೆ (m_statep->forPrimary() && refClassp && !nodep->paramsp()
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
                        << nodep->warn_ಇನ್ನಷ್ಟು() << "... Suggest use '"
                        << nodep->classOrPackageNodep()->prettyName() << "#()'");
                // As other simulators do, use the default specialization. Add the same
                // placeholder pin V3LinkCells makes for '#()' so it is linked below.
                AstPin* ಸ್ಥಿರಾಂಕ pinp = ಹೊಸ AstPin{nodep->fileline(), 1, "__paramNumber1", ಶೂನ್ಯಸೂಚಕ};
                pinp->param(ನಿಜ);
                nodep->ಕೂಡಿಸು_Paramsp(pinp);
            }

            m_ds.init(m_curSymp);
            UINFO(4, indent() << "(Backto) Link ClassOrPackageRef: " << nodep);
            iterateChildren(nodep);
        }

        ಆದರೆ (nodep->name() == "local::") {
            ಆದರೆ (!m_randSymp && !m_rand_ವಿಧಾನ_Callp) {
                nodep->v3error("Illegal 'local::' outside 'randomize() with'"
                               " (IEEE 1800-2023 18.7.1)");
                m_ds.m_dotErr = ನಿಜ;
                ಹಿಂತಿರುಗಿ;
            }
        }
        ಆದರೆ (m_ds.m_dotPos == DP_PACKAGE && nodep->classOrPackageSkipp()) {
            m_ds.m_dotSymp = m_statep->get_ಗಂಟು_Sym(nodep->classOrPackageSkipp());
            UINFO(9, indent() << "set sym " << m_ds.ascii());
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstConstraint* nodep) override {
        LINKDOT_ಭೇಟಿ_ಆರಂಭ();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        ಆದರೆ (nodep->isExternDef()) {
            ಆದರೆ (ಸ್ಥಿರಾಂಕ VSymEnt* ಸ್ಥಿರಾಂಕ foundp
                = m_curSymp->ಕಂಡುಹಿಡಿ_Id_Fallback("extern " + nodep->name())) {
                ಸ್ಥಿರಾಂಕ AstConstraint* ಸ್ಥಿರಾಂಕ protop = VN_AS(foundp->nodep(), Constraint);
                // Copy specifiers.
                // External definition cannot have any specifiers, so no value will be overwritten.
                nodep->isStatic(protop->isStatic());
            } ಇಲ್ಲದಿದ್ದರೆ {
                nodep->v3error("extern not found that declares " + nodep->prettyNameQ());
            }
        }
        ಆದರೆ (nodep->isExternProto()) {
            ಆದರೆ (!m_curSymp->ಕಂಡುಹಿಡಿ_Id_Fallback(nodep->name()) && nodep->isExternExplicit()) {
                nodep->v3warn(PROTOTYPEMIS, "Definition not found for extern prototype "
                                                << nodep->prettyNameQ());
            }
        }
        VL_RESTORER(m_curSymp);
        VL_RESTORER_COPY(m_ds);
        m_ds.m_dotSymp = m_curSymp = m_statep->get_ಗಂಟು_Sym(nodep);
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstConstraintRef* nodep) override {
        ಆದರೆ (nodep->user3_ಗಣ_Once()) ಹಿಂತಿರುಗಿ;
        LINKDOT_ಭೇಟಿ_ಆರಂಭ();
        UINFO(8, indent() << "visit " << nodep);
        UINFO(8, indent() << m_ds.ascii());
        // No children defined
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstVarRef* nodep) override {
        // VarRef: Resolve its reference
        // ParseRefs are used the first pass (forPrimary) so we shouldn't get can't find
        // errors here now that we have a VarRef.
        // No checkNoDot; created and iterated from a parseRef
        LINKDOT_ಭೇಟಿ_ಆರಂಭ();
        iterateChildren(nodep);
        ಆದರೆ (!nodep->varp()) {
            UINFO(9, indent() << "linkVarRef se" << cvt_ಗಮ್ಯ_Hex(m_curSymp) << "  n=" << nodep);
            UASSERT_OBJ(m_curSymp, nodep, "nullptr lookup symbol table");
            VSymEnt* ಸ್ಥಿರಾಂಕ foundp = m_curSymp->ಕಂಡುಹಿಡಿ_Id_Fallback(nodep->name());
            ಆದರೆ (AstVar* ಸ್ಥಿರಾಂಕ varp
                = foundp ? ಸಿಕ್ಕಿತು_ಗಮ್ಯ_Varp(foundp, nodep, nodep->access()) : ಶೂನ್ಯಸೂಚಕ) {
                nodep->varp(varp);
                updateVarUse(nodep->varp());
                // Generally set by parse, but might be an import
                nodep->classOrPackagep(foundp->classOrPackagep());
            }
            ಆದರೆ (VL_UNCOVERABLE(!nodep->varp())) {
                nodep->v3error("Can't find definition of signal, again: "  // LCOV_EXCL_LINE
                               << nodep->prettyNameQ());
            }
        }
        AstVarScope* vscp = nodep->varScopep();
        ಆದರೆ (vscp && vscp->user2p() != vscp && m_replaceWithAlias) {
            vscp = LinkDotScopeVisitor::getAlias(vscp);
            nodep->varp(vscp->varp());
            nodep->varScopep(vscp);
            updateVarUse(nodep->varp());
            UINFO(7, indent() << "Resolved " << nodep);  // Also prints taskp
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstVarXRef* nodep) override {
        // VarRef: Resolve its reference
        // We always link even if varp() is set, because the module we choose may change
        // due to creating new modules, flattening, etc.
        ಆದರೆ (nodep->user3_ಗಣ_Once()) ಹಿಂತಿರುಗಿ;
        LINKDOT_ಭೇಟಿ_ಆರಂಭ();
        UINFO(8, indent() << "visit " << nodep);
        // No checkNoDot; created and iterated from a parseRef
        ಆದರೆ (!m_modSymp) {
            // Module that is not in hierarchy.  We'll be dead code eliminating it later.
            UINFO(9, "Dead module for " << nodep);
            nodep->varp(ಶೂನ್ಯಸೂಚಕ);
        } ಇಲ್ಲದಿದ್ದರೆ {
            VSymEnt* lookupSymp = m_curSymp;  // Start search at current scope
            ಆದರೆ (nodep->inlinedDots() != "") {  // Correct for current post-inlined scope
                lookupSymp = ಕಂಡುಹಿಡಿ_Inlined_Dots_Sym(nodep, nodep->inlinedDots());
            }
            string baddot;
            VSymEnt* okSymp;
            ತಾರ್ಕಿಕ modport;
            VSymEnt* ಸ್ಥಿರಾಂಕ dotSymp
                = ಕಂಡುಹಿಡಿ_Dotted_Ref_Symp(nodep, lookupSymp, nodep->dotted(), baddot, okSymp, &modport);
            ಆದರೆ (!dotSymp) ಹಿಂತಿರುಗಿ;

            ಆದರೆ (!m_statep->forScopeCreation()) {
                VSymEnt* foundp = ಶೂನ್ಯಸೂಚಕ;
                ಆದರೆ (modport) {
                    // This is a copy of findSymPrefixed with few modifications.
                    // This searches without a fallback like findSymPrefixed but if lookup fails it
                    // will check one fallback (fallback of modport shall be interface) and take
                    // only L and G PARAMs since they should be visible from a modport always.
                    string ಪೂರ್ವಪ್ರತ್ಯಯ = dotSymp->sym_ಪೂರ್ವಪ್ರತ್ಯಯ();
                    ಆಗ (!foundp) {
                        foundp = dotSymp->ಕಂಡುಹಿಡಿ_Id_Flat(ಪೂರ್ವಪ್ರತ್ಯಯ + nodep->name());
                        ಆದರೆ (foundp) ನಿಲ್ಲಿಸು;
                        UASSERT_OBJ(dotSymp->fallbackp(), nodep, "Modports shall have fallback");
                        foundp = dotSymp->fallbackp()->ಕಂಡುಹಿಡಿ_Id_Flat(ಪೂರ್ವಪ್ರತ್ಯಯ + nodep->name());
                        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ varp
                            = ಸಿಕ್ಕಿತು_ಗಮ್ಯ_Varp(foundp, nodep, nodep->access())) {
                            ಆದರೆ (!varp->isParam()) foundp = ಶೂನ್ಯಸೂಚಕ;
                        }
                        ಆದರೆ (ಪೂರ್ವಪ್ರತ್ಯಯ.ಖಾಲಿಗಳು()) ನಿಲ್ಲಿಸು;
                        string ಮುಂದಿನ_ಪೂರ್ವಪ್ರತ್ಯಯ = LinkDotState::remove_ಕೊನೆಯ_Inline_Scope(ಪೂರ್ವಪ್ರತ್ಯಯ);
                        ಆದರೆ (ಪೂರ್ವಪ್ರತ್ಯಯ == ಮುಂದಿನ_ಪೂರ್ವಪ್ರತ್ಯಯ) ನಿಲ್ಲಿಸು;
                        ಪೂರ್ವಪ್ರತ್ಯಯ = std::move(ಮುಂದಿನ_ಪೂರ್ವಪ್ರತ್ಯಯ);
                    }
                    baddot = nodep->name();
                } ಇಲ್ಲದಿದ್ದರೆ {
                    foundp = m_statep->ಕಂಡುಹಿಡಿ_Sym_Prefixed(dotSymp, nodep->name(), baddot, ನಿಜ);
                }
                ಆದರೆ (m_ಒಳ_Sens && foundp) {
                    ಆದರೆ (AstClocking* ಸ್ಥಿರಾಂಕ clockingp = sensClockingp(foundp->nodep())) {
                        foundp = getCreateClockingEventSymEnt(clockingp);
                    }
                }
                AstVar* ಸ್ಥಿರಾಂಕ varp
                    = foundp ? ಸಿಕ್ಕಿತು_ಗಮ್ಯ_Varp(foundp, nodep, nodep->access()) : ಶೂನ್ಯಸೂಚಕ;
                nodep->varp(varp);
                updateVarUse(nodep->varp());
                UINFO(7, indent() << "Resolved " << nodep);  // Also prints varp
                // If found, check if it's ok to access in case it's in a hier_block
                ಆದರೆ (nodep->varp() && error_Hier_Non_ಪೋರ್ಟ್(nodep, nodep->varp(), dotSymp)) ಹಿಂತಿರುಗಿ;
                ಆದರೆ (!nodep->varp()) {
                    nodep->v3error("Can't find definition of "
                                   << Ast_ಗಂಟು::prettyNameQ(baddot) << " in dotted signal: '"
                                   << nodep->dotted() + "." + nodep->prettyName() << "'\n"
                                   << nodep->warnContextPrimary()
                                   << okSymp->cellErrorScopes(nodep));
                    ಹಿಂತಿರುಗಿ;
                }
            } ಇಲ್ಲದಿದ್ದರೆ {
                VSymEnt* ಸ್ಥಿರಾಂಕ foundp
                    = m_statep->ಕಂಡುಹಿಡಿ_Sym_Prefixed(dotSymp, nodep->name(), baddot, ನಿಜ);
                AstVarScope* vscp = foundp ? VN_CAST(foundp->nodep(), VarScope) : ಶೂನ್ಯಸೂಚಕ;
                // Handle modport expression virtual ports
                ಆದರೆ (!vscp) vscp = ಕಂಡುಹಿಡಿ_Virtual_ಪೋರ್ಟ್_Var_Scope(dotSymp, nodep->name());
                // If found, check if it's ok to access in case it's in a hier_block
                ಆದರೆ (vscp && error_Hier_Non_ಪೋರ್ಟ್(nodep, vscp->varp(), dotSymp)) ಹಿಂತಿರುಗಿ;
                ಆದರೆ (!vscp) {
                    nodep->v3error("Can't find varpin scope of "
                                   << Ast_ಗಂಟು::prettyNameQ(baddot) << " in dotted signal: '"
                                   << nodep->dotted() + "." + nodep->prettyName() << "'\n"
                                   << nodep->warnContextPrimary()
                                   << okSymp->cellErrorScopes(nodep));
                } ಇಲ್ಲದಿದ್ದರೆ {
                    ಆದರೆ (vscp->user2p() && m_replaceWithAlias) {
                        vscp = LinkDotScopeVisitor::getAlias(vscp);
                    }
                    // Convert the VarXRef to a VarRef, so we don't need
                    // later optimizations to deal with VarXRef.
                    nodep->varp(vscp->varp());
                    nodep->varScopep(vscp);
                    updateVarUse(nodep->varp());
                    UINFO(7, indent() << "Resolved " << nodep);  // Also prints taskp
                    AstVarRef* ಸ್ಥಿರಾಂಕ newvscp
                        = ಹೊಸ AstVarRef{nodep->fileline(), vscp, nodep->access()};
                    nodep->replaceWith(newvscp);
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                    UINFO(9, indent() << "new " << newvscp);  // Also prints taskp
                }
            }
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstEnumDType* nodep) override {
        LINKDOT_ಭೇಟಿ_ಆರಂಭ();
        iterateChildren(nodep);
        AstRefDType* ಸ್ಥಿರಾಂಕ refdtypep = VN_CAST(nodep->subDTypep(), RefDType);
        ಆದರೆ (refdtypep && (nodep == refdtypep->subDTypep())) {
            refdtypep->v3error("Self-referential enumerated type definition");
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_UOr_Struct_DType* nodep) override {
        LINKDOT_ಭೇಟಿ_ಆರಂಭ();
        symIterateChildren(nodep, m_statep->get_ಗಂಟು_Sym(nodep));
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Enum_ಅಂಶ_Ref* nodep) override {
        // Resolve its reference
        // EnumItemRefs are created by the first pass, but V3Param may regenerate due to
        // a parameterized class/module, so we shouldn't get can't find errors.
        // No checkNoDot; created and iterated from a parseRef
        ಆದರೆ (nodep->user3_ಗಣ_Once()) ಹಿಂತಿರುಗಿ;
        LINKDOT_ಭೇಟಿ_ಆರಂಭ();
        ಆದರೆ (m_statep->forParamed() && !nodep->dotted().ಖಾಲಿಗಳು() && m_modSymp) {
            // Hierarchical reference, relink as V3Param may have specialized the module
            string baddot;
            VSymEnt* okSymp = ಶೂನ್ಯಸೂಚಕ;
            VSymEnt* ಸ್ಥಿರಾಂಕ dotSymp
                = ಕಂಡುಹಿಡಿ_Dotted_Ref_Symp(nodep, m_curSymp, nodep->dotted(), baddot, okSymp);
            ಆದರೆ (!dotSymp) ಹಿಂತಿರುಗಿ;
            VSymEnt* ಸ್ಥಿರಾಂಕ foundp
                = m_statep->ಕಂಡುಹಿಡಿ_Sym_Prefixed(dotSymp, nodep->name(), baddot, ನಿಜ);
            Ast_Enum_ಅಂಶ* ಸ್ಥಿರಾಂಕ itemp = foundp ? VN_CAST(foundp->nodep(), Enum_ಅಂಶ) : ಶೂನ್ಯಸೂಚಕ;
            ಆದರೆ (!itemp) {
                nodep->v3error("Enum item " << nodep->prettyNameQ() << " not found in "
                                            << Ast_ಗಂಟು::prettyNameQ(nodep->dotted()) << '\n'
                                            << nodep->warnContextPrimary());
                ಹಿಂತಿರುಗಿ;
            }
            nodep->itemp(itemp);
            UINFO(9, indent() << " relinked " << nodep);
        }
        ಆದರೆ (!nodep->itemp()) {
            UINFO(9, indent() << "linkEnumRef se" << cvt_ಗಮ್ಯ_Hex(m_curSymp) << "  n=" << nodep);
            UASSERT_OBJ(m_curSymp, nodep, "nullptr lookup symbol table");
            VSymEnt* ಸ್ಥಿರಾಂಕ foundp = m_curSymp->ಕಂಡುಹಿಡಿ_Id_Fallback(nodep->name());
            ಆದರೆ (Ast_Enum_ಅಂಶ* ಸ್ಥಿರಾಂಕ itemp = foundp ? VN_CAST(foundp->nodep(), Enum_ಅಂಶ) : ಶೂನ್ಯಸೂಚಕ) {
                nodep->itemp(itemp);
                nodep->classOrPackagep(foundp->classOrPackagep());
                UINFO(9, indent() << " resolved " << nodep);
            }
            ಆದರೆ (VL_UNCOVERABLE(!nodep->itemp())) {
                nodep->v3error("Can't find definition of enum item, again: "  // LCOV_EXCL_LINE
                               << nodep->prettyNameQ());
            }
        }
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ವಿಧಾನ_Call* nodep) override {
        // Created here so should already be resolved.
        LINKDOT_ಭೇಟಿ_ಆರಂಭ();
        UINFO(5, indent() << "visit " << nodep);
        VL_RESTORER_COPY(m_ds);
        VL_RESTORER(m_randSymp);
        VL_RESTORER(m_rand_ವಿಧಾನ_Callp);
        {
            m_ds.init(m_curSymp);
            ಆದರೆ (nodep->name() == "randomize" && (nodep->argsp() || nodep->withp())) {
                m_rand_ವಿಧಾನ_Callp = nodep;
                ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* fromDtp = nodep->fromp()->dtypep();
                ಆದರೆ (!fromDtp) {
                    ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Var_Ref* ಸ್ಥಿರಾಂಕ varRefp = VN_CAST(nodep->fromp(), ಗಂಟು_Var_Ref)) {
                        fromDtp = varRefp->varp()->subDTypep();
                    } ಇಲ್ಲದಿದ್ದರೆ {
                        fromDtp = get_ಅಭಿವ್ಯಕ್ತಿ_DTypep(nodep->fromp());
                    }
                    ಆದರೆ (!fromDtp) {
                        ಆದರೆ (nodep->withp()) {
                            nodep->v3warn(
                                E_UNSUPPORTED,
                                "Unsupported: 'randomize() with' on complex expressions");
                        } ಇಲ್ಲದಿದ್ದರೆ {
                            nodep->v3warn(E_UNSUPPORTED,
                                          "Unsupported: Inline random variable control with "
                                          "'randomize()' called on complex expressions");
                        }
                    }
                }
                ಆದರೆ (m_statep->forPrimary() && isParamedClassRefDType(fromDtp)) {
                    m_ds.m_unresolvedClass = ನಿಜ;
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (fromDtp) {
                    ಸ್ಥಿರಾಂಕ AstClassRefDType* ಸ್ಥಿರಾಂಕ classDtp
                        = VN_CAST(fromDtp->skipRefp(), ClassRefDType);
                    ಆದರೆ (!classDtp)
                        nodep->v3error("'randomize() with' on a non-class-instance "
                                       << fromDtp->prettyNameQ());
                    ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (m_statep->forPrimary() && m_extendsParam.ಎಣಿಕೆ_2(classDtp->classp())) {
                        // Inherited members are unavailable until the base class is specialized.
                        // Defer lookup rather than incorrectly binding names in the caller.
                        m_ds.m_unresolvedClass = ನಿಜ;
                    } ಇಲ್ಲದಿದ್ದರೆ {
                        m_randSymp = m_statep->get_ಗಂಟು_Sym(classDtp->classp());
                    }
                }
            }
            iterateChildren(nodep);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstVar* nodep) override {
        LINKDOT_ಭೇಟಿ_ಆರಂಭ();
        checkNoDot(nodep);
        VL_RESTORER(m_isParam);
        m_isParam = nodep->varType().isParam();
        iterateChildren(nodep);
        ಆದರೆ (m_statep->forPrimary() && nodep->isIO() && !m_ftaskp && !nodep->user4()) {
            nodep->v3error(
                "Input/output/inout does not appear in port list: " << nodep->prettyNameQ());
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstVarScope* nodep) override {
        LINKDOT_ಭೇಟಿ_ಆರಂಭ();
        checkNoDot(nodep);
        iterateChildren(nodep);
        AstVarScope* aliasp = LinkDotScopeVisitor::getAlias(nodep);
        ಆದರೆ (aliasp && aliasp != nodep && !nodep->varp()->isIfaceRef()) {
            // Aliased variable might still be references from outside,
            // eg through the VPI, and is traced, so we need the value to propagate.
            // TODO: this means external writes to the LHS (e.g.: through the VPI) don't work
            Ast_ನಿಯೋಜಿಸು_W* ಸ್ಥಿರಾಂಕ assignp = ಹೊಸ Ast_ನಿಯೋಜಿಸು_W{
                nodep->fileline(), ಹೊಸ AstVarRef{nodep->fileline(), nodep, VAccess::WRITE},
                ಹೊಸ AstVarRef{nodep->fileline(), aliasp, VAccess::READ}};
            assignp->user2(ನಿಜ);
            nodep->scopep()->ಕೂಡಿಸು_Blocksp(ಹೊಸ AstAlways{assignp});
            // Propagate attributes and lint state of the replaced variable,
            // because all references to it are replaced with references to the alias variable
            aliasp->varp()->propagateAttrFrom(nodep->varp());
            aliasp->varp()->fileline()->modifyStateInherit(nodep->varp()->fileline());
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_FTask_Ref* nodep) override {
        ಆದರೆ (nodep->user3_ಗಣ_Once()) ಹಿಂತಿರುಗಿ;
        LINKDOT_ಭೇಟಿ_ಆರಂಭ();
        UINFO(8, indent() << "visit " << nodep);
        UINFO(8, indent() << m_ds.ascii());
        ಆದರೆ (m_ds.m_dotPos != DP_MEMBER || nodep->name() != "randomize") {
            // Visit arguments at the beginning.
            // They may be visitted even if the current node can't be linked now.
            symIterateChildren(nodep, m_curSymp);
        }

        ಆದರೆ (m_ds.m_super) {
            ಆದರೆ (AstFuncRef* ಸ್ಥಿರಾಂಕ funcRefp = VN_CAST(nodep, FuncRef)) {
                funcRefp->superReference(ನಿಜ);
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstTaskRef* ಸ್ಥಿರಾಂಕ taskRefp = VN_CAST(nodep, TaskRef)) {
                taskRefp->superReference(ನಿಜ);
            }
        }

        VL_RESTORER(m_randSymp);

        ತಾರ್ಕಿಕ ಮೊದಲ = !m_ds.m_dotp || m_ds.m_dotPos == DP_ಮೊದಲ;
        ತಾರ್ಕಿಕ staticAccess = ಸುಳ್ಳು;
        ಆದರೆ (m_ds.m_unresolvedClass) {
            // Unable to link before V3Param
            ಹಿಂತಿರುಗಿ;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (m_ds.m_unresolvedCell && m_ds.m_dotPos == DP_FINAL && m_ds.m_unlinkedScopep) {
            Ast_ಗಂಟು_FTask_Ref* ಸ್ಥಿರಾಂಕ newftaskp = nodep->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);
            newftaskp->dotted(m_ds.m_dot_ಪಠ್ಯ);
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ AstUnlinkedRef{nodep->fileline(), newftaskp, nodep->name(),
                                                     m_ds.m_unlinkedScopep->unlink_Fr_ಹಿಂಭಾಗ()};
            m_ds.m_unlinkedScopep = ಶೂನ್ಯಸೂಚಕ;
            m_ds.m_unresolvedCell = ಸುಳ್ಳು;
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            ಹಿಂತಿರುಗಿ;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (m_ds.m_dotp && m_ds.m_dotPos == DP_PACKAGE) {
            UASSERT_OBJ(VN_IS(m_ds.m_dotp->lhsp(), ClassOrPackageRef), m_ds.m_dotp->lhsp(),
                        "Bad package link");
            staticAccess = ನಿಜ;
            AstClassOrPackageRef* ಸ್ಥಿರಾಂಕ cpackagerefp
                = VN_AS(m_ds.m_dotp->lhsp(), ClassOrPackageRef);
            ಆದರೆ (cpackagerefp->name() == "local::") {
                m_randSymp = ಶೂನ್ಯಸೂಚಕ;
                ಮೊದಲ = ನಿಜ;
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!cpackagerefp->classOrPackageSkipp()) {
                VSymEnt* ಸ್ಥಿರಾಂಕ foundp = m_statep->resolveClassOrPackage(
                    m_ds.m_dotSymp, cpackagerefp, ನಿಜ, ಸುಳ್ಳು, ":: reference");
                ಆದರೆ (foundp) nodep->classOrPackagep(cpackagerefp->classOrPackageSkipp());
            } ಇಲ್ಲದಿದ್ದರೆ {
                nodep->classOrPackagep(cpackagerefp->classOrPackageSkipp());
            }
            // Class/package :: HERE function() . method_called_on_function_return_value()
            m_ds.m_dotPos = DP_MEMBER;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (m_ds.m_dotp && m_ds.m_dotPos == DP_FINAL) {
            nodep->dotted(m_ds.m_dot_ಪಠ್ಯ);  // Maybe ""
            // Only flag FTaskRefs under generate-if/case blocks that may be
            // pruned.  GenFor and plain begin-blocks won't be pruned by V3Param.
            // A generate-case block's parent is the GenCaseItem, not the GenCase.
            // VarXRef uses the broader m_genBlk flag (set for all GenBlocks)
            // because genfor unrolling also removes variables.
            ಆದರೆ (m_ds.m_genBlk && m_ds.m_dotSymp) {
                ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ blkp = m_ds.m_dotSymp->nodep();
                ಆದರೆ (VN_IS(blkp, Gen_ಖಂಡ)
                    && (VN_IS(blkp->backp(), GenIf) || VN_IS(blkp->backp(), Gen_Case_ಅಂಶ))) {
                    nodep->ಒಳಗೊಂಡಿದೆ_Gen_ಖಂಡ(ನಿಜ);
                }
            }
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (m_ds.m_dotp && m_ds.m_dotPos == DP_MEMBER) {
            // Found a Var, everything following is method call.
            // {scope}.{var}.HERE {method} ( ARGS )
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ varEtcp = VN_AS(m_ds.m_dotp->lhsp()->unlink_Fr_ಹಿಂಭಾಗ(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ);
            AstArg* ಸ್ಥಿರಾಂಕ argsp = nodep->argsp();
            ಆದರೆ (argsp) argsp->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ();
            Ast_ವಿಧಾನ_Call* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ Ast_ವಿಧಾನ_Call{nodep->fileline(), varEtcp,
                                                          VFlag_ಮಗು_DType{}, nodep->name(), argsp};
            ಆದರೆ (AstWith* ಸ್ಥಿರಾಂಕ withp = nodep->withp()) newp->withp(withp->unlink_Fr_ಹಿಂಭಾಗ());
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            ಹಿಂತಿರುಗಿ;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (m_ds.m_dotp && (m_ds.m_dotPos == DP_SCOPE || m_ds.m_dotPos == DP_ಮೊದಲ)) {
            // HERE function() . method_called_on_function_return_value()
            m_ds.m_dotPos = DP_MEMBER;
            m_ds.m_dot_ಪಠ್ಯ = "";
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!m_ds.m_disablep) {
            // visit(AstDisable*) setup the dot handling
            checkNoDot(nodep);
        }
        ಆದರೆ (nodep->classOrPackagep() && nodep->taskp()) {
            // References into packages don't care about cell hierarchy.
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!m_modSymp) {
            // Module that is not in hierarchy.  We'll be dead code eliminating it later.
            UINFO(9, indent() << "Dead module for " << nodep);
            nodep->taskp(ಶೂನ್ಯಸೂಚಕ);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->dotted() == "" && nodep->taskp()) {
            // Earlier should have setup the links
            // Might be under a BEGIN we're not processing, so don't relink it
        } ಇಲ್ಲದಿದ್ದರೆ {
            string baddot;
            VSymEnt* okSymp = ಶೂನ್ಯಸೂಚಕ;
            VSymEnt* dotSymp;
            ಆದರೆ (nodep->dotted().ಖಾಲಿಗಳು()) {
                // Non-'super.' dotted reference
                dotSymp = m_ds.m_dotSymp;
            } ಇಲ್ಲದಿದ್ದರೆ {
                // Start search at module, as a variable of same name under a subtask isn't a
                // relevant hit however a function under a begin/end is.  So we want begins, but
                // not the function
                dotSymp = m_curSymp;
            }
            ಆದರೆ (nodep->classOrPackagep()) {  // Look only in specified package
                dotSymp = m_statep->get_ಗಂಟು_Sym(nodep->classOrPackagep());
                UINFO(8, indent() << "Override classOrPackage " << dotSymp);
            } ಇಲ್ಲದಿದ್ದರೆ {
                ಆದರೆ (nodep->inlinedDots() != "") {  // Correct for current post-inlined scope
                    dotSymp = ಕಂಡುಹಿಡಿ_Inlined_Dots_Sym(nodep, nodep->inlinedDots());
                }
                dotSymp = m_statep->ಕಂಡುಹಿಡಿ_Dotted(nodep->fileline(), dotSymp, nodep->dotted(), baddot,
                                               okSymp, ನಿಜ);  // Maybe nullptr
            }
            // Restricted 'with (id_list)': only id_list names bind into target class.
            ಆದರೆ (m_randSymp
                && (!m_currentWithp || m_currentWithp->name_Resolves_ಗಮ್ಯ_ಗುರಿ(nodep->name()))) {
                VSymEnt* ಸ್ಥಿರಾಂಕ foundp = m_randSymp->ಕಂಡುಹಿಡಿ_Id_Flat(nodep->name());
                ಆದರೆ (foundp && m_currentWithp) {
                    ಆದರೆ (m_currentWithp->restricted()) {
                        m_restricted_ಹೆಸರುಗಳು_ಬಳಸಿದ.ಸೇರಿಸು(nodep->name());
                    }
                    UINFO(9, indent() << "randomize-with fromSym " << foundp->nodep());
                    AstArg* argsp = ಶೂನ್ಯಸೂಚಕ;
                    ಆದರೆ (nodep->argsp()) {
                        iterate_And_ಮುಂದಿನ_Null(nodep->argsp());
                        argsp = nodep->argsp()->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ();
                    }
                    ಆದರೆ (m_ds.m_dotPos != DP_NONE) m_ds.m_dotPos = DP_MEMBER;
                    Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ Ast_ವಿಧಾನ_Call{
                        nodep->fileline(),
                        ಹೊಸ AstLambdaArgRef{nodep->fileline(), "__Vrandwith_obj", ಸುಳ್ಳು},
                        VFlag_ಮಗು_DType{}, nodep->name(), argsp};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                    ಹಿಂತಿರುಗಿ;
                }
            }
            ಆದರೆ (ಮೊದಲ && nodep->name() == "randomize" && VN_IS(m_modp, Class)
                && !VN_IS(nodep->classOrPackagep(), Package)) {
                // need special handling to avoid falling back to std::randomize
                // Skip if classOrPackagep is a Package (i.e. std::randomize resolved earlier)
                VMemberMap memberMap;
                AstFunc* ಸ್ಥಿರಾಂಕ randFuncp = V3Randomize::newRandomizeFunc(
                    memberMap, VN_AS(m_modp, Class), nodep->name(), ನಿಜ, ನಿಜ);
                nodep->taskp(randFuncp);
                nodep->classOrPackagep(VN_AS(m_modp, Class));
                m_curSymp = m_statep->ಸೇರಿಸು_ಖಂಡ(m_curSymp, nodep->name(), randFuncp, m_modp);
                // Already linked to the class randomize function; skip findSymPrefixed
                // which may find std::randomize and overwrite classOrPackagep
                ಹಿಂತಿರುಗಿ;
            }
            VSymEnt* ಸ್ಥಿರಾಂಕ foundp
                = m_statep->ಕಂಡುಹಿಡಿ_Sym_Prefixed(dotSymp, nodep->name(), baddot, ಮೊದಲ);
            Ast_ಗಂಟು_FTask* ಸ್ಥಿರಾಂಕ taskp = foundp ? VN_CAST(foundp->nodep(), ಗಂಟು_FTask) : ಶೂನ್ಯಸೂಚಕ;
            Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ taskModulep = foundp ? foundp->classOrPackagep() : ಶೂನ್ಯಸೂಚಕ;
            // Ignore deferring linking of functions inside params of classes extending
            // parametric classes if referenced function can be linked in the first pass.
            ಆದರೆ (!m_isParam || !taskp
                || (taskp->class_ವಿಧಾನ() && taskModulep && taskModulep->hasParameterList())) {
                ಆದರೆ (m_insideClassExtParam) {
                    // The reference may point to a method declared in a super class, which is
                    // proved by a parameter. In such a case, it can't be linked at the first
                    // stage. Must not do any linking, because e.g. might find an extends of an
                    // upper class because the current class (under parent) isn't yet importing
                    // it's extended class symbols.
                    ಹಿಂತಿರುಗಿ;
                }
                ಆದರೆ (AstClass* ಸ್ಥಿರಾಂಕ ಗುರಿ_Classp = VN_CAST(dotSymp->nodep(), Class)) {
                    ಆದರೆ (m_extendsParam.ಎಣಿಕೆ_2(ಗುರಿ_Classp)) {
                        // Target class has parameterized extends not yet resolved.
                        // Its inherited symbols (e.g. static functions from the base class)
                        // aren't imported yet - defer to linkDotParamed.
                        ಹಿಂತಿರುಗಿ;
                    }
                }
            }

            ಆದರೆ (taskp) {
                ಆದರೆ (staticAccess && !taskp->isStatic()) {
                    // TODO bug4077
                    // nodep->v3error("Static access to non-static task/function "
                    //                << taskp->prettyNameQ() << endl);
                }
                nodep->taskp(taskp);
                nodep->classOrPackagep(foundp->classOrPackagep());
                UINFO(7, indent() << "Resolved " << nodep);  // Also prints taskp
            } ಇಲ್ಲದಿದ್ದರೆ {
                // Note ParseRef has similar error handling/message output
                UINFO(7, indent() << "   ErrFtask curSymp=se" << cvt_ಗಮ್ಯ_Hex(m_curSymp)
                                  << " dotSymp=se" << cvt_ಗಮ್ಯ_Hex(dotSymp));
                ಆದರೆ (foundp) {
                    ಆದರೆ (VN_IS(foundp->nodep(), Var) && m_ds.m_dot_ಪಠ್ಯ == "" && m_ftaskp
                        && m_ftaskp->name() == foundp->nodep()->name()) {
                        // This is a recursive reference to the function itself, not to the var
                        nodep->taskp(m_ftaskp);
                        nodep->classOrPackagep(foundp->classOrPackagep());
                        UINFO(7, indent() << "Resolved recursive " << nodep);  // Also prints taskp
                    } ಇಲ್ಲದಿದ್ದರೆ {
                        nodep->v3error("Found definition of '"
                                       << m_ds.m_dot_ಪಠ್ಯ << (m_ds.m_dot_ಪಠ್ಯ == "" ? "" : ".")
                                       << nodep->prettyName() << "'" << " as a "
                                       << foundp->nodep()->typeName()
                                       << " but expected a task/function");
                    }
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(nodep, New) && !m_statep->forScopeCreation()) {
                    // Resolved in V3Width
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ ((nodep->name() == "pre_randomize" || nodep->name() == "post_randomize")
                           && VN_IS(dotSymp->nodep(), Class)) {
                    AstClass* ಸ್ಥಿರಾಂಕ classp = VN_AS(dotSymp->nodep(), Class);
                    // Functions that might be defined by user, but if not, they get empty
                    // definitions within referenced class.
                    // TODO perhaps cleaner to add to symbol table up-front with
                    // a lambda to construct them on a reference.
                    AstFunc* ಸ್ಥಿರಾಂಕ funcp
                        = ಹೊಸ AstFunc{nodep->fileline(), nodep->name(), ಶೂನ್ಯಸೂಚಕ, ಶೂನ್ಯಸೂಚಕ};
                    funcp->class_ವಿಧಾನ(ನಿಜ);
                    funcp->dtypep(funcp->ಕಂಡುಹಿಡಿ_Void_DType());
                    classp->ಕೂಡಿಸು_Membersp(funcp);
                    m_statep->ಸೇರಿಸು_ಖಂಡ(dotSymp, nodep->name(), funcp, classp);
                    nodep->taskp(funcp);
                    nodep->classOrPackagep(classp);
                    UINFO(9, indent() << " Auto-made " << funcp);
                    ಹಿಂತಿರುಗಿ;
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->name() == "randomize" || nodep->name() == "srandom"
                           || nodep->name() == "get_randstate" || nodep->name() == "set_randstate"
                           || nodep->name() == "rand_mode" || nodep->name() == "constraint_mode") {
                    ಆದರೆ (AstClass* ಸ್ಥಿರಾಂಕ classp = VN_CAST(m_modp, Class)) {
                        // Don't overwrite if already resolved to a package (e.g. std::randomize)
                        ಆದರೆ (!VN_IS(nodep->classOrPackagep(), Package))
                            nodep->classOrPackagep(classp);
                    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->name() == "randomize") {
                        // A std::randomize resolved in V3Width
                        nodep->classOrPackagep(v3Global.rootp()->stdPackagep());
                    } ಇಲ್ಲದಿದ್ದರೆ {
                        nodep->v3error("Calling implicit class method "
                                       << nodep->prettyNameQ() << " without being under class");
                        nodep->replaceWith(ಹೊಸ AstConst{nodep->fileline(), 0});
                        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                        ಹಿಂತಿರುಗಿ;
                    }
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->dotted() == "") {
                    ಆದರೆ (nodep->pli()) {
                        ಆದರೆ (v3Global.opt.bboxSys()) {
                            Ast_ಗಂಟು* newp;
                            ಆದರೆ (VN_IS(nodep, FuncRef)) {
                                newp = ಹೊಸ AstConst{nodep->fileline(), AstConst::All0{}};
                            } ಇಲ್ಲದಿದ್ದರೆ {
                                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* outp = ಶೂನ್ಯಸೂಚಕ;
                                ಆಗ (AstArg* ಸ್ಥಿರಾಂಕ argp = nodep->argsp()) {
                                    outp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(outp, argp->exprp()->unlink_Fr_ಹಿಂಭಾಗ());
                                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(argp->unlink_Fr_ಹಿಂಭಾಗ()), argp);
                                }
                                newp = ಹೊಸ AstSysIgnore{nodep->fileline(), outp};
                                newp->dtypep(nodep->dtypep());
                            }
                            nodep->replaceWith(newp);
                            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                            ಹಿಂತಿರುಗಿ;
                        } ಇಲ್ಲದಿದ್ದರೆ {
                            VSpellCheck speller;
                            V3Parse::candidatePli(&speller);
                            ಸ್ಥಿರಾಂಕ string suggest = speller.ಉತ್ತಮ_Candidate_ಸಂದೇಶ(nodep->prettyName());
                            nodep->v3error(
                                "Unsupported or unknown PLI call: "
                                << nodep->prettyNameQ() << '\n'
                                << (suggest.ಖಾಲಿಗಳು() ? "" : nodep->warn_ಇನ್ನಷ್ಟು() + suggest));
                        }
                    } ಇಲ್ಲದಿದ್ದರೆ {
                        ಸ್ಥಿರಾಂಕ string suggest = m_statep->suggestSymFallback(
                            dotSymp, nodep->name(), Link_ಗಂಟು_Matcher_FTask{});
                        nodep->v3error("Can't find definition of task/function: "
                                       << nodep->prettyNameQ() << '\n'
                                       << (suggest.ಖಾಲಿಗಳು() ? "" : nodep->warn_ಇನ್ನಷ್ಟು() + suggest));
                    }
                } ಇಲ್ಲದಿದ್ದರೆ {
                    ಸ್ಥಿರಾಂಕ string suggest = m_statep->suggestSymFallback(dotSymp, nodep->name(),
                                                                        Link_ಗಂಟು_Matcher_FTask{});
                    nodep->v3error("Can't find definition of "
                                   << Ast_ಗಂಟು::prettyNameQ(baddot) << " in dotted task/function: '"
                                   << nodep->dotted() + "." + nodep->prettyName() << "'\n"
                                   << (suggest.ಖಾಲಿಗಳು() ? "" : nodep->warn_ಇನ್ನಷ್ಟು() + suggest) << '\n'
                                   << nodep->warnContextPrimary()
                                   << okSymp->cellErrorScopes(nodep));
                }
            }
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Sel_ಬಿಟ್* nodep) override {
        ಆದರೆ (nodep->user3_ಗಣ_Once()) ಹಿಂತಿರುಗಿ;
        LINKDOT_ಭೇಟಿ_ಆರಂಭ();
        iterate_And_ಮುಂದಿನ_Null(nodep->fromp());
        ಆದರೆ (m_ds.m_unresolvedClass) {
            UASSERT_OBJ(m_ds.m_dotPos != DP_SCOPE && m_ds.m_dotPos != DP_ಮೊದಲ, nodep,
                        "Object of unresolved class on scope position in dotted reference");
            // The index does not depend on the class, and V3Param must see its class references
            symIterateNull(nodep->bitp(), m_curSymp);
            symIterateNull(nodep->attrp(), m_curSymp);
            ಹಿಂತಿರುಗಿ;
        }
        ಆದರೆ (m_ds.m_dotPos == DP_SCOPE
            || m_ds.m_dotPos
                   == DP_ಮೊದಲ) {  // Already under dot, so this is {modulepart} DOT {modulepart}
            UINFO(9, indent() << "deferring until after a V3Param pass: " << nodep);
            m_ds.m_dot_ಪಠ್ಯ += "__BRA__??__KET__";
            m_ds.m_unresolvedCell = ನಿಜ;
            // And pass up m_ds.m_dotText
        }
        // Pass dot state down to only fromp()
        iterate_And_ಮುಂದಿನ_Null(nodep->fromp());
        symIterateNull(nodep->bitp(), m_curSymp);
        symIterateNull(nodep->attrp(), m_curSymp);
        ಆದರೆ (m_ds.m_unresolvedCell && (m_ds.m_dotPos == DP_SCOPE || m_ds.m_dotPos == DP_ಮೊದಲ)) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ exprp = nodep->bitp()->unlink_Fr_ಹಿಂಭಾಗ();
            ಆದರೆ (AstCellArrayRef* ಸ್ಥಿರಾಂಕ fromArrRefp = VN_CAST(nodep->fromp(), CellArrayRef)) {
                // Multi-dim iface array access: append this select to the existing chain
                fromArrRefp->ಕೂಡಿಸು_Selp(exprp);
                AstCellArrayRef* ಸ್ಥಿರಾಂಕ movedp = fromArrRefp->unlink_Fr_ಹಿಂಭಾಗ();
                nodep->replaceWith(movedp);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            } ಇಲ್ಲದಿದ್ದರೆ {
                AstCellArrayRef* ಸ್ಥಿರಾಂಕ newp
                    = ಹೊಸ AstCellArrayRef{nodep->fileline(), nodep->fromp()->name(), exprp};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            }
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Pre_Sel* nodep) override {
        // Excludes simple AstSelBit, see above
        ಆದರೆ (nodep->user3_ಗಣ_Once()) ಹಿಂತಿರುಗಿ;
        LINKDOT_ಭೇಟಿ_ಆರಂಭ();
        ಆದರೆ (m_ds.m_dotPos == DP_SCOPE
            || m_ds.m_dotPos
                   == DP_ಮೊದಲ) {  // Already under dot, so this is {modulepart} DOT {modulepart}
            nodep->v3error("Syntax error: Range ':', '+:' etc are not allowed in the instance "
                           "part of a dotted reference");
            m_ds.m_dotErr = ನಿಜ;
            ಹಿಂತಿರುಗಿ;
        }
        Ast_ಗಂಟು_DType* packedArrayDtp = ಶೂನ್ಯಸೂಚಕ;  // Datatype reference for packed array
        {
            VL_RESTORER(m_packedArrayDtp);
            iterate_And_ಮುಂದಿನ_Null(nodep->fromp());
            symIterateNull(nodep->rhsp(), m_curSymp);
            symIterateNull(nodep->thsp(), m_curSymp);

            ಆದರೆ (m_packedArrayDtp) {
                AstRange* ಸ್ಥಿರಾಂಕ newRangep
                    = ಹೊಸ AstRange(nodep->fileline(), nodep->rhsp()->unlink_Fr_ಹಿಂಭಾಗ(),
                                   nodep->thsp()->unlink_Fr_ಹಿಂಭಾಗ());
                AstPackArrayDType* newArrayTypep
                    = ಹೊಸ AstPackArrayDType(nodep->fileline(), m_packedArrayDtp, newRangep);
                newArrayTypep->ಮಗು_DTypep(m_packedArrayDtp);
                newArrayTypep->refDTypep(ಶೂನ್ಯಸೂಚಕ);
                ಆದರೆ (VN_IS(nodep->backp(), Sel_ಹೊರತೆಗೆ)) {
                    packedArrayDtp = newArrayTypep;
                } ಇಲ್ಲದಿದ್ದರೆ {
                    replaceWithCheckBreak(nodep, newArrayTypep);
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                }
            } ಇಲ್ಲದಿದ್ದರೆ {
                ಆದರೆ (nodep->attrp()) {
                    Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ attrp = nodep->attrp()->unlink_Fr_ಹಿಂಭಾಗ();
                    VL_DO_DANGLING(attrp->delete_ವೃಕ್ಷ(), attrp);
                }
                Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ basefromp = nodep->ತಳ_Fromp(ಸುಳ್ಳು);
                ಆದರೆ (VN_IS(basefromp, Replicate)) {
                    // From {...}[...] syntax in IEEE 2017
                    ಆದರೆ (basefromp) UINFO(9, indent() << " Related node: " << basefromp);
                } ಇಲ್ಲದಿದ್ದರೆ {
                    nodep->attrp(ಹೊಸ Ast_Attr_ದ{nodep->fileline(), VAttrType::VAR_ತಳ,
                                               basefromp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು)});
                }
            }
        }

        ಆದರೆ (packedArrayDtp) m_packedArrayDtp = packedArrayDtp;
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstMemberSel* nodep) override {
        // checkNoDot not appropriate, can be under a dot
        LINKDOT_ಭೇಟಿ_ಆರಂಭ();
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Gen_ಖಂಡ* nodep) override {
        LINKDOT_ಭೇಟಿ_ಆರಂಭ();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        {
            VL_RESTORER(m_curSymp);
            VL_RESTORER_COPY(m_ds);
            ಆದರೆ (nodep->name() != "") {
                m_ds.m_dotSymp = m_curSymp = m_statep->get_ಗಂಟು_Sym(nodep);
                UINFO(5, indent() << "cur=se" << cvt_ಗಮ್ಯ_Hex(m_curSymp));
            }
            iterateChildren(nodep);
        }
        UINFO(5, indent() << "cur=se" << cvt_ಗಮ್ಯ_Hex(m_curSymp));
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_ಖಂಡ* nodep) override {
        LINKDOT_ಭೇಟಿ_ಆರಂಭ();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        {
            VL_RESTORER(m_curSymp);
            VL_RESTORER_COPY(m_ds);
            ಆದರೆ (nodep->name() != "") {
                m_ds.m_dotSymp = m_curSymp = m_statep->get_ಗಂಟು_Sym(nodep);
                UINFO(5, indent() << "cur=se" << cvt_ಗಮ್ಯ_Hex(m_curSymp));
            }
            iterateChildren(nodep);
        }
        UINFO(5, indent() << "cur=se" << cvt_ಗಮ್ಯ_Hex(m_curSymp));
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_FTask* nodep) override {
        LINKDOT_ಭೇಟಿ_ಆರಂಭ();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        ಆದರೆ (nodep->isExternDef() && !nodep->didProto()) {
            nodep->didProto(ನಿಜ);
            ಆದರೆ (ಸ್ಥಿರಾಂಕ VSymEnt* ಸ್ಥಿರಾಂಕ foundp
                = m_curSymp->ಕಂಡುಹಿಡಿ_Id_Fallback("extern " + nodep->name())) {
                ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_FTask* ಸ್ಥಿರಾಂಕ protop = VN_AS(foundp->nodep(), ಗಂಟು_FTask);
                // Copy specifiers.
                // External definition cannot have any specifiers, so no value will be overwritten.
                nodep->isHideLocal(protop->isHideLocal());
                nodep->isHideProtected(protop->isHideProtected());
                nodep->isStatic(protop->isStatic());
                nodep->isVirtual(protop->isVirtual());
                nodep->lifetime(protop->lifetime());
                // Always add an FUNC_RETURN_PROTO even if task, so we can locate the prototype
                nodep->ಕೂಡಿಸು_Stmtsp(ಹೊಸ Ast_Attr_ದ{protop->fileline(), VAttrType::FUNC_RETURN_PROTO,
                                               protop->fvarp()
                                                   ? protop->fvarp()->clone_ವೃಕ್ಷ(ಸುಳ್ಳು)
                                                   : ಹೊಸ AstVoidDType{protop->fileline()}});
                ಗಾಗಿ (Ast_ಗಂಟು* stmtp = protop->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    ಆದರೆ (AstVar* ಸ್ಥಿರಾಂಕ portp = VN_CAST(stmtp, Var)) {
                        Ast_Attr_ದ* ಸ್ಥಿರಾಂಕ attrp
                            = ಹೊಸ Ast_Attr_ದ{protop->fileline(), VAttrType::FUNC_ARG_PROTO,
                                            portp->subDTypep()->clone_ವೃಕ್ಷ(ಸುಳ್ಳು)};
                        attrp->name(portp->name());
                        nodep->ಕೂಡಿಸು_Stmtsp(attrp);
                    }
                }
            } ಇಲ್ಲದಿದ್ದರೆ {
                nodep->v3error("extern not found that declares " + nodep->prettyNameQ());
            }
        }
        ಆದರೆ (nodep->isExternProto()) {
            ಆದರೆ (!m_curSymp->ಕಂಡುಹಿಡಿ_Id_Fallback(nodep->name())) {
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
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ isNew = nodep->name() == "new";
            ಆದರೆ (isNew) m_explicitSuperNewp = ಶೂನ್ಯಸೂಚಕ;
            iterateChildren(nodep);
            ಆದರೆ (isNew) {
                ಸ್ಥಿರಾಂಕ AstClassExtends* ಸ್ಥಿರಾಂಕ classExtendsp = VN_AS(m_modp, Class)->extendsp();
                ಆದರೆ (m_explicitSuperNewp && !m_explicitSuperNewp->isImplicit() && classExtendsp
                    && classExtendsp->argsp()) {
                    m_explicitSuperNewp->v3error(
                        "Explicit super.new not allowed with class "
                        "extends arguments (IEEE 1800-2023 8.17)\n"
                        << m_explicitSuperNewp->warn_ಇನ್ನಷ್ಟು() << "... Suggest remove super.new\n"
                        << m_explicitSuperNewp->warnContextPrimary() << '\n'
                        << classExtendsp->argsp()->warnOther()
                        << "... Location of extends argument(s)\n"
                        << classExtendsp->argsp()->warnContextSecondary());
                }
                ಆದರೆ (classExtendsp && classExtendsp->classOrNullp()) {
                    ಆದರೆ (!m_explicitSuperNewp && m_statep->forParamed()
                        && !VN_AS(classExtendsp->ಮಗು_DTypep()->skipRefp(), ClassRefDType)
                                ->classp()
                                ->isInterfaceClass()) {
                        Ast_ಗಂಟು_Stmt* ಸ್ಥಿರಾಂಕ superNewp
                            = ಕೂಡಿಸು_Implicit_Super_New_Call(VN_AS(nodep, Func), classExtendsp);
                        UINFO(9, "created super new " << superNewp);
                        iterate(superNewp);
                    }
                }
            }
        }
        m_ds.m_dotSymp = VL_RESTORER_PREV(m_curSymp);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Foreach* nodep) override {
        LINKDOT_ಭೇಟಿ_ಆರಂಭ();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        symIterateChildren(nodep, m_statep->get_ಗಂಟು_Sym(nodep));
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstRandSequence* nodep) override {
        LINKDOT_ಭೇಟಿ_ಆರಂಭ();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        ಆದರೆ (!nodep->ಆರಂಭ().ಖಾಲಿಗಳು())
            nodep->prodp(ಕಂಡುಹಿಡಿ_Prod(nodep, m_statep->get_ಗಂಟು_Sym(nodep), nodep->ಆರಂಭ()));
        symIterateChildren(nodep, m_statep->get_ಗಂಟು_Sym(nodep));
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstRSProd* nodep) override {
        LINKDOT_ಭೇಟಿ_ಆರಂಭ();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        symIterateChildren(nodep, m_statep->get_ಗಂಟು_Sym(nodep));
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_RSProd_ಅಂಶ* nodep) override {
        LINKDOT_ಭೇಟಿ_ಆರಂಭ();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        nodep->prodp(ಕಂಡುಹಿಡಿ_Prod(nodep, m_curSymp, nodep->name()));
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCoverWith* nodep) override {
        LINKDOT_ಭೇಟಿ_ಆರಂಭ();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        iterate_And_ಮುಂದಿನ_Null(nodep->subp());
        // The type of 'item' is that of the coverpoint expression, which may name 'item' too
        iterate_And_ಮುಂದಿನ_Null(nodep->itemp()->ಮಗು_DTypep());
        VL_RESTORER(m_curSymp);
        m_ds.m_dotSymp = m_curSymp = m_statep->get_ಗಂಟು_Sym(nodep);
        iterate_And_ಮುಂದಿನ_Null(nodep->itemp());
        iterate_And_ಮುಂದಿನ_Null(nodep->filterp());
        m_ds.m_dotSymp = VL_RESTORER_PREV(m_curSymp);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCoverCrossWith* nodep) override {
        LINKDOT_ಭೇಟಿ_ಆರಂಭ();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        iterate_And_ಮುಂದಿನ_Null(nodep->selectp());
        iterate_And_ಮುಂದಿನ_Null(nodep->countp());
        // The type of a cross item is that of its coverpoint's expression, which may name items
        iterate_And_ಮುಂದಿನ_Null(nodep->varsp());
        VL_RESTORER(m_curSymp);
        m_ds.m_dotSymp = m_curSymp = m_statep->get_ಗಂಟು_Sym(nodep);
        iterate_And_ಮುಂದಿನ_Null(nodep->filterp());
        m_ds.m_dotSymp = VL_RESTORER_PREV(m_curSymp);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstWith* nodep) override {
        LINKDOT_ಭೇಟಿ_ಆರಂಭ();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        ಆದರೆ (m_ds.m_unresolvedClass) ಹಿಂತಿರುಗಿ;
        VL_RESTORER(m_curSymp);
        VL_RESTORER(m_currentWithp);
        VL_RESTORER_COPY(m_restricted_ಹೆಸರುಗಳು_ಬಳಸಿದ);
        {
            m_ds.m_dotSymp = m_curSymp = m_statep->get_ಗಂಟು_Sym(nodep);
            m_currentWithp = nodep;
            // Validate the identifier_list once per AstWith (m_validated travels
            // with cloneTree, so cloned witnesses skip the redundant check).
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ ಮೊದಲ_ಭೇಟಿ = !nodep->validated();
            ಆದರೆ (ಮೊದಲ_ಭೇಟಿ) {
                nodep->validated(ನಿಜ);
                m_restricted_ಹೆಸರುಗಳು_ಬಳಸಿದ.clear();
                // IEEE 1800-2023 18.7: each name in the identifier_list must designate
                // a member of the randomize() target class.
                ಆದರೆ (nodep->restricted() && m_randSymp) {
                    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ std::string& n : nodep->restricted_ಹೆಸರುಗಳು()) {
                        ಆದರೆ (!m_randSymp->ಕಂಡುಹಿಡಿ_Id_Flat(n)) {
                            nodep->v3error("Identifier '"
                                           << n
                                           << "' in 'with (...)' identifier list is not a"
                                              " member of the randomize() target class"
                                              " (IEEE 1800-2023 18.7)");
                            m_restricted_ಹೆಸರುಗಳು_ಬಳಸಿದ.ಸೇರಿಸು(n);
                        }
                    }
                }
            }
            iterateChildren(nodep);
            // Flag names that the user listed but never referenced in the constraint.
            ಆದರೆ (ಮೊದಲ_ಭೇಟಿ && nodep->restricted()) {
                ಗಾಗಿ (ಸ್ಥಿರಾಂಕ std::string& n : nodep->restricted_ಹೆಸರುಗಳು()) {
                    ಆದರೆ (!m_restricted_ಹೆಸರುಗಳು_ಬಳಸಿದ.ಎಣಿಕೆ_2(n)) {
                        nodep->v3warn(ಬಳಸದ_2, "Identifier '"
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
    ಶೂನ್ಯ ಭೇಟಿ(AstLambdaArgRef* nodep) override {
        LINKDOT_ಭೇಟಿ_ಆರಂಭ();
        UINFO(5, indent() << "visit " << nodep);
        // No checknodot(nodep), visit(AstScope) will check for LambdaArgRef
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstClassExtends* nodep) override {
        // Resolve the symbol and get the class.
        // If it is a parameterized case, the class will be resolved after V3Param.cpp
        ಆದರೆ (nodep->user3_ಗಣ_Once()) ಹಿಂತಿರುಗಿ;
        // If the class is resolved, there is nothing more to do
        ಆದರೆ (nodep->classOrNullp()) ಹಿಂತಿರುಗಿ;
        LINKDOT_ಭೇಟಿ_ಆರಂಭ();
        ಆದರೆ (m_statep->forPrimary()) {
            ಆದರೆ (nodep->ಮಗು_DTypep()) ಹಿಂತಿರುಗಿ;
            Ast_ಗಂಟು* cprp = nodep->classOrPkgsp();
            VSymEnt* lookSymp = m_curSymp;
            ಆದರೆ (AstDot* ಸ್ಥಿರಾಂಕ dotp = VN_CAST(cprp, Dot)) {
                dotp->user3(ನಿಜ);
                ಆದರೆ (AstClassOrPackageRef* lookNodep = VN_CAST(dotp->lhsp(), ClassOrPackageRef)) {
                    iterate(lookNodep);
                    cprp = dotp->rhsp();
                    VSymEnt* ಸ್ಥಿರಾಂಕ foundp = m_statep->resolveClassOrPackage(
                        lookSymp, lookNodep, ನಿಜ, ಸುಳ್ಳು, nodep->verilogKwd());
                    ಆದರೆ (!foundp) {
                        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlink_Fr_ಹಿಂಭಾಗ()), nodep);
                        ಹಿಂತಿರುಗಿ;
                    }
                    UASSERT_OBJ(lookNodep->classOrPackageSkipp(), nodep, "Bad package link");
                    lookSymp = m_statep->get_ಗಂಟು_Sym(lookNodep->classOrPackageSkipp());
                } ಇಲ್ಲದಿದ್ದರೆ {
                    dotp->lhsp()->v3error("Attempting to extend"  // LCOV_EXCL_LINE
                                          " using non-class under dot");
                }
            }
            AstClassOrPackageRef* ಸ್ಥಿರಾಂಕ cpackagerefp = VN_CAST(cprp, ClassOrPackageRef);
            ಆದರೆ (VL_UNCOVERABLE(!cpackagerefp)) {
                // Linking the extend gives an error before this is hit
                nodep->v3error("Attempting to extend using non-class");  // LCOV_EXCL_LINE
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlink_Fr_ಹಿಂಭಾಗ()), nodep);
                ಹಿಂತಿರುಗಿ;
            }
            VSymEnt* ಸ್ಥಿರಾಂಕ foundp = m_statep->resolveClassOrPackage(lookSymp, cpackagerefp, ನಿಜ,
                                                                    ನಿಜ, nodep->verilogKwd());
            ಆದರೆ (!foundp) {
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlink_Fr_ಹಿಂಭಾಗ()), nodep);
                ಹಿಂತಿರುಗಿ;
            }
            ಆದರೆ (AstClass* ಸ್ಥಿರಾಂಕ classp = VN_CAST(foundp->nodep(), Class)) {
                AstPin* paramsp = cpackagerefp->paramsp();
                ಆದರೆ (paramsp) {
                    paramsp = paramsp->clone_ವೃಕ್ಷ(ನಿಜ);
                    nodep->parameterized(ನಿಜ);
                }
                nodep->ಮಗು_DTypep(ಹೊಸ AstClassRefDType{nodep->fileline(), classp, paramsp});
                // Link pins
                iterate(nodep->ಮಗು_DTypep());
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstParamTypeDType* ಸ್ಥಿರಾಂಕ paramp
                       = VN_CAST(foundp->nodep(), ParamTypeDType)) {
                AstRefDType* ಸ್ಥಿರಾಂಕ refParamp = ಹೊಸ AstRefDType{nodep->fileline(), paramp->name()};
                refParamp->refDTypep(paramp);
                nodep->ಮಗು_DTypep(refParamp);
                nodep->parameterized(ನಿಜ);
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstTypedef* ಸ್ಥಿರಾಂಕ typedefp = VN_CAST(foundp->nodep(), Typedef)) {
                Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ unwrappedp = typedefp->subDTypep()->skipRefp();
                ಆದರೆ (AstClassRefDType* ಸ್ಥಿರಾಂಕ classRefp = VN_CAST(unwrappedp, ClassRefDType)) {
                    AstPin* paramsp = cpackagerefp->paramsp();
                    ಆದರೆ (!paramsp && classRefp->paramsp()) {
                        // No explicit #(...) on extends clause; carry over the
                        // typedef's type-parameter pins so V3Param sees them.
                        paramsp = classRefp->paramsp()->clone_ವೃಕ್ಷ(ನಿಜ);
                        nodep->parameterized(ನಿಜ);
                    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (paramsp) {
                        paramsp = paramsp->clone_ವೃಕ್ಷ(ನಿಜ);
                        nodep->parameterized(ನಿಜ);
                    }
                    nodep->ಮಗು_DTypep(
                        ಹೊಸ AstClassRefDType{nodep->fileline(), classRefp->classp(), paramsp});
                    iterate(nodep->ಮಗು_DTypep());
                } ಇಲ್ಲದಿದ್ದರೆ {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: " << foundp->nodep()->prettyTypeName()
                                                  << " in 'class extends'");
                    ಹಿಂತಿರುಗಿ;
                }
            } ಇಲ್ಲದಿದ್ದರೆ {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: " << foundp->nodep()->prettyTypeName()
                                                             << " in 'class extends'");
                ಹಿಂತಿರುಗಿ;
            }
            ಆದರೆ (!nodep->ಮಗು_DTypep()) nodep->v3error("Attempting to extend using non-class");
            nodep->classOrPkgsp()->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ();
        } ಇಲ್ಲದಿದ್ದರೆ {
            // Probably a parameter
            ಆದರೆ (AstRefDType* ಸ್ಥಿರಾಂಕ refp = VN_CAST(nodep->ಮಗು_DTypep(), RefDType)) {
                ಆದರೆ (AstClassRefDType* classRefp = VN_CAST(refp->skipRefp(), ClassRefDType)) {
                    // Resolved to a class reference.
                    refp->replaceWith(classRefp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು));
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(refp), refp);
                } ಇಲ್ಲದಿದ್ದರೆ {
                    // Unable to resolve the ref type to a class reference.
                    // Get the value of type parameter passed to the class instance,
                    // to print the helpful error message.
                    ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* typep = refp->refDTypep();
                    ಆಗ (ನಿಜ) {
                        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstParamTypeDType* ಸ್ಥಿರಾಂಕ atypep
                            = VN_CAST(typep, ParamTypeDType)) {
                            typep = atypep->subDTypep();
                            ಮುಂದುವರಿ;
                        }
                        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstRequireDType* ಸ್ಥಿರಾಂಕ atypep = VN_CAST(typep, RequireDType)) {
                            typep = atypep->subDTypep();
                            ಮುಂದುವರಿ;
                        }
                        ನಿಲ್ಲಿಸು;
                    }
                    typep->v3error("Attempting to extend using non-class");
                }
            }
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstClass* nodep) override {
        ಆದರೆ (nodep->user3_ಗಣ_Once()) ಹಿಂತಿರುಗಿ;
        LINKDOT_ಭೇಟಿ_ಆರಂಭ();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        VL_RESTORER(m_curSymp);
        VL_RESTORER(m_modSymp);
        VL_RESTORER(m_modp);
        VL_RESTORER_COPY(m_if_Class_Imp_ಹೆಸರುಗಳು);
        VL_RESTORER_COPY(m_ifForwards);
        VL_RESTORER(m_insideClassExtParam);
        {
            m_ifForwards.clear();
            m_ds.init(m_curSymp);
            m_insideClassExtParam = nodep->isCovergroup() && nodep->covergroupEnclosingClassp()
                                    && m_extendsParam.ಕಂಡುಹಿಡಿ(nodep->covergroupEnclosingClassp())
                                           != m_extendsParam.ಕೊನೆ();
            // Until overridden by a SCOPE
            m_ds.m_dotSymp = m_curSymp = m_modSymp = m_statep->get_ಗಂಟು_Sym(nodep);
            m_modp = nodep;
            ಪೂರ್ಣಾಂಕ ಮುಂದಿನ = 0;
            ಗಾಗಿ (AstClassExtends* cextp = nodep->extendsp(); cextp;
                 cextp = VN_AS(cextp->nextp(), ClassExtends)) {
                // Replace abstract reference with hard pointer
                // Will need later resolution when deal with parameters
                ಆದರೆ (++ಮುಂದಿನ == 2 && !nodep->isInterfaceClass() && !cextp->isImplements()) {
                    cextp->v3error("Multiple inheritance illegal on non-interface classes"
                                   " (IEEE 1800-2023 8.13)");
                }
                iterate(cextp);
                ಆದರೆ (m_statep->forPrimary()) {
                    ಆದರೆ (cextp->parameterized()) {
                        // Parameters in extends statement.
                        // The class can't be resolved in the current pass.
                        m_extendsParam.ಸೇರಿಸು(nodep);
                        m_insideClassExtParam = ನಿಜ;
                    }
                    ಆದರೆ (AstClassRefDType* ಸ್ಥಿರಾಂಕ classRefp
                        = VN_CAST(cextp->ಮಗು_DTypep(), ClassRefDType)) {
                        AstClass* ಸ್ಥಿರಾಂಕ classp = classRefp->classp();
                        ಆದರೆ (classp != nodep) iterate(classp);
                        ಆದರೆ (m_extendsParam.ಕಂಡುಹಿಡಿ(classp) != m_extendsParam.ಕೊನೆ()) {
                            // One of its super classes has parameters in extends statement.
                            // Some links may not be resolved in the first pass.
                            m_extendsParam.ಸೇರಿಸು(nodep);
                            m_insideClassExtParam = ನಿಜ;
                        }
                    }
                }

                ಆದರೆ (AstClass* ಸ್ಥಿರಾಂಕ ತಳ_Classp = cextp->classOrNullp()) {
                    // Already converted. Update symbol table to link unlinked members.
                    // Base class has to be visited in a case if its extends statement
                    // needs to be handled. Recursive inheritance was already checked.
                    // Must be here instead of in LinkDotParam to handle
                    // "class (type T) extends T".
                    ಆದರೆ (ತಳ_Classp == nodep) {
                        cextp->v3error("Attempting to extend class " << nodep->prettyNameQ()
                                                                     << " from itself");
                    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (extendsClass(ತಳ_Classp, nodep)) {
                        cextp->v3error("Attempting to extend class "
                                       << nodep->prettyNameQ() << " from "
                                       << ತಳ_Classp->prettyNameQ()
                                       << ", which circularly inherits from it");
                    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (cextp->isImplements() && !ತಳ_Classp->isInterfaceClass()) {
                        cextp->v3error("Attempting to implement from non-interface class "
                                       << ತಳ_Classp->prettyNameQ() << '\n'
                                       << "... Suggest use 'extends'");
                    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!cextp->isImplements() && !nodep->isInterfaceClass()
                               && ತಳ_Classp->isInterfaceClass()) {
                        cextp->v3error("Attempting to extend from interface class "
                                       << ತಳ_Classp->prettyNameQ() << '\n'
                                       << "... Suggest use 'implements'");
                    }
                    ತಳ_Classp->isExtended(ನಿಜ);
                    nodep->isExtended(ನಿಜ);
                    iterate(ತಳ_Classp);
                    importSymbolsFromExtended(nodep, cextp);
                    ಮುಂದುವರಿ;
                }
            }
            m_ds.m_dotSymp = m_curSymp;

            iterateChildren(nodep);
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& itr : m_ifForwards) {
                ಸ್ಥಿರಾಂಕ VSymEnt* ಸ್ಥಿರಾಂಕ implSymp = itr.ಎರಡನೇ;
                ಕೂಡಿಸು_Interface_Forward(nodep, VN_AS(implSymp->nodep(), ಗಂಟು_FTask),
                                    VN_AS(implSymp->parentp()->nodep(), Class));
            }
        }
        // V3Width when determines types needs to find enum values and such
        // so add members pointing to appropriate enum values
        {
            VMemberMap memberMap;
            ಗಾಗಿ (VSymEnt::const_iterator it = m_curSymp->begin(); it != m_curSymp->ಕೊನೆ(); ++it) {
                Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ itemp = it->ಎರಡನೇ->nodep();
                ಆದರೆ (!memberMap.ಕಂಡುಹಿಡಿ_Member(nodep, it->ಮೊದಲ)) {
                    ಆದರೆ (Ast_Enum_ಅಂಶ* ಸ್ಥಿರಾಂಕ aitemp = VN_CAST(itemp, Enum_ಅಂಶ)) {
                        Ast_Enum_ಅಂಶ_Ref* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ Ast_Enum_ಅಂಶ_Ref{
                            aitemp->fileline(), aitemp, it->ಎರಡನೇ->classOrPackagep()};
                        UINFO(8, indent()
                                     << "Class import noderef '" << it->ಮೊದಲ << "' " << newp);
                        nodep->ಕೂಡಿಸು_Membersp(newp);
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
    ಸ್ಥಿರ ಶೂನ್ಯ collectScopeRefs(Ast_ಗಂಟು* nodep, std::ಸದಿಶ<AstClassOrPackageRef*>& refps) {
        ಆದರೆ (AstClassOrPackageRef* ಸ್ಥಿರಾಂಕ refp = VN_CAST(nodep, ClassOrPackageRef)) {
            refps.ತಳ್ಳು_ಹಿಂಭಾಗ(refp);
            ಹಿಂತಿರುಗಿ;
        }
        AstDot* ಸ್ಥಿರಾಂಕ dotp = VN_CAST(nodep, Dot);
        UASSERT_OBJ(dotp, nodep, "Non-scope node in package/class scope operand");
        collectScopeRefs(dotp->lhsp(), refps);
        collectScopeRefs(dotp->rhsp(), refps);
    }

    // Resolve a chained class-scope operand (`pkg::outer::inner` in
    // `pkg::outer::inner::t`) and reduce it to its innermost ClassOrPackageRef.
    // Returns false, having reported, if a segment could not be resolved.
    ತಾರ್ಕಿಕ reduceScopeDot(AstRefDType* nodep, AstDot* scopeDotp) {
        std::ಸದಿಶ<AstClassOrPackageRef*> refps;
        collectScopeRefs(scopeDotp, refps);

        VSymEnt* scopeSymp = m_ds.m_dotSymp;
        ಗಾಗಿ (ಗಾತ್ರ_t i = 0; i < refps.ಗಾತ್ರ_2(); ++i) {
            AstClassOrPackageRef* ಸ್ಥಿರಾಂಕ refp = refps[i];
            ಆದರೆ (!refp->classOrPackageSkipp() && !refp->classOrPackageNodep()
                && !m_statep->resolveClassOrPackage(scopeSymp, refp, i == 0, ಸುಳ್ಳು,
                                                    "class/package reference")) {
                ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;  // Error already reported
            }
            ಆದರೆ (i + 1 == refps.ಗಾತ್ರ_2()) ನಿಲ್ಲಿಸು;
            // A middle segment that resolved to a node with no reachable module
            // (e.g. a typedef alias of a not-yet-specialized parameterized class)
            // gives nothing to look the next segment up in.
            Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ modp = refp->classOrPackageSkipp();
            ಆದರೆ (!modp) {
                refp->v3warn(E_UNSUPPORTED, "Unsupported: Multiple '::' package/class reference");
                ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            }
            scopeSymp = m_statep->get_ಗಂಟು_Sym(modp);
        }
        AstClassOrPackageRef* ಸ್ಥಿರಾಂಕ innerp = refps.ಹಿಂಭಾಗ();
        innerp->unlink_Fr_ಹಿಂಭಾಗ();
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(scopeDotp->unlink_Fr_ಹಿಂಭಾಗ()), scopeDotp);
        nodep->classOrPackageOpp(innerp);
        ಹಿಂತಿರುಗಿ ನಿಜ;
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstRefDType* nodep) override {

        ಆದರೆ (ಸ್ವಯಂಚಾಲಿತ* ಸ್ಥಿರಾಂಕ typeOfp = nodep->typeofp()) {
            iterate(typeOfp);
            // After iteration, typeofp() may have been replaced with a resolved type
            // (e.g., DOT expression like if0.rq_t resolved to REFDTYPE)
            // If it's now a NodeDType, replace this RefDType with it
            ಆದರೆ (Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ resolvedDTypep = VN_CAST(nodep->typeofp(), ಗಂಟು_DType)) {
                UINFO(9,
                      indent() << "iface capture typeofp resolved to dtype, replacing RefDType\n");
                resolvedDTypep->unlink_Fr_ಹಿಂಭಾಗ();
                nodep->replaceWith(resolvedDTypep);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                // Originally (PR #6637) this block re-captured a resolved
                // RefDType into the iface-capture ledger when user2p held a
                // Cell, to ensure re-resolution during the paramed pass.
                // In practice, the typeofp-resolved RefDType never carries a
                // Cell in user2p: the capture happens earlier when the
                // typedef is first linked.
                ಆದರೆ (V3LinkDotIfaceCapture::enabled()) {
                    ಆದರೆ (AstRefDType* ಸ್ಥಿರಾಂಕ resolvedRefp = VN_CAST(resolvedDTypep, RefDType)) {
                        ಆದರೆ (VL_UNCOVERABLE(
                                VN_IS(resolvedRefp->user2p(), Cell))) {  // LCOV_EXCL_LINE
                            resolvedRefp->v3fatalSrc(  // LCOV_EXCL_LINE
                                "typeofp resolved RefDType has Cell in user2p;"
                                " expected to be captured already");
                        }
                    }
                }
            }
            ಹಿಂತಿರುಗಿ;
        }

        // Resolve its reference
        ಆದರೆ (m_statep->forParamed() && nodep->user3()) {
            ಆದರೆ (V3LinkDotIfaceCapture::enabled() && nodep->user2p()) {
                UINFO(9, indent() << "iface capture clear user3 for captured typedef name="
                                  << nodep->name() << " cell=" << nodep->user2p());
            }
            nodep->user3(ಸುಳ್ಳು);
        }
        ಆದರೆ (nodep->user3_ಗಣ_Once()) {
            ಆದರೆ (V3LinkDotIfaceCapture::enabled() && nodep->user2p()) {
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
            ಆದರೆ (VL_UNCOVERABLE(m_statep->forParamed() && nodep->classOrPackageOpp())) {
                nodep->v3fatalSrc(  // LCOV_EXCL_LINE
                    "RefDType user3 revisit with classOrPackageOpp in paramed pass");
            }
            ಹಿಂತಿರುಗಿ;
        }
        LINKDOT_ಭೇಟಿ_ಆರಂಭ();
        UINFO(5, indent() << "visit " << nodep);
        ಆದರೆ (nodep->classOrPackageOpp()) {
            // Resolve a chained scope (`pkg::cls::t`, `pkg::cls#(P)::t`) from outside in.
            // On failure reduceScopeDot has reported, so stop before using the operand.
            ಆದರೆ (AstDot* ಸ್ಥಿರಾಂಕ scopeDotp = VN_CAST(nodep->classOrPackageOpp(), Dot)) {
                ಆದರೆ (!reduceScopeDot(nodep, scopeDotp)) ಹಿಂತಿರುಗಿ;
            }
            // Re-read: reduceScopeDot may have replaced the operand
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ cpackagep = nodep->classOrPackageOpp();
            UASSERT_OBJ(!VN_IS(cpackagep, Dot), cpackagep,
                        "Package/class scope operand should be reduced by now");
            ಆದರೆ (AstClassOrPackageRef* ಸ್ಥಿರಾಂಕ cpackagerefp = VN_CAST(cpackagep, ClassOrPackageRef)) {
                iterate(cpackagerefp);
                ಸ್ಥಿರಾಂಕ AstClass* ಸ್ಥಿರಾಂಕ clsp = VN_CAST(cpackagerefp->classOrPackageNodep(), Class);
                ಆದರೆ (clsp && clsp->hasGParam()) {
                    // Unable to link before the instantiation of parameter classes.
                    // The class reference node still has to be visited now to later link
                    // parameters.
                    iterate(cpackagep);
                    ಹಿಂತಿರುಗಿ;
                }
                // Defer non-typedef references through typedef aliases of parameterized classes.
                ಆದರೆ (m_statep->forPrimary() && !VN_IS(nodep->backp(), Typedef)
                    && isParamedClassRef(cpackagerefp)) {
                    iterate(cpackagep);
                    ಹಿಂತಿರುಗಿ;
                }

                ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ doDefaultTypedef = !(m_resolvingTypedef && m_statep->forPrimary());
                ಆದರೆ (!cpackagerefp->classOrPackageSkipp(doDefaultTypedef)
                    && !cpackagerefp->classOrPackageNodep()) {
                    VSymEnt* ಸ್ಥಿರಾಂಕ foundp = m_statep->resolveClassOrPackage(
                        m_ds.m_dotSymp, cpackagerefp, ನಿಜ, ಸುಳ್ಳು, "class/package reference");
                    ಆದರೆ (!foundp) ಹಿಂತಿರುಗಿ;
                }
                nodep->classOrPackagep(cpackagerefp->classOrPackageSkipp(doDefaultTypedef));
                ಆದರೆ (!VN_IS(nodep->classOrPackagep(), Class)
                    && !VN_IS(nodep->classOrPackagep(), Package)) {
                    ಆದರೆ (m_statep->forPrimary()) {
                        // It may be a type that comes from parameter class that is not
                        // instantioned yet
                        iterate(cpackagep);
                        ಹಿಂತಿರುಗಿ;
                    }
                    // Likely impossible, as error thrown earlier
                    cpackagerefp->v3error(  // LCOV_EXCL_LINE
                        "'::' expected to reference a class/package but referenced '"
                        << (nodep->classOrPackagep() ? nodep->classOrPackagep()->prettyTypeName()
                                                     : "<unresolved-object>")
                        << "'\n"
                        << cpackagerefp->warn_ಇನ್ನಷ್ಟು() + "... Suggest '.' instead of '::'");
                }
            }
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(cpackagep->unlink_Fr_ಹಿಂಭಾಗ()), cpackagep);
        }

        ಆದರೆ (m_ds.m_dotp && (m_ds.m_dotPos == DP_PACKAGE || m_ds.m_dotPos == DP_SCOPE)) {
            UASSERT_OBJ(VN_IS(m_ds.m_dotp->lhsp(), ClassOrPackageRef), m_ds.m_dotp->lhsp(),
                        "Bad package link");
            ಸ್ವಯಂಚಾಲಿತ* ಸ್ಥಿರಾಂಕ cpackagerefp = VN_CAST(m_ds.m_dotp->lhsp(), ClassOrPackageRef);
            UASSERT_OBJ(cpackagerefp->classOrPackageSkipp(), m_ds.m_dotp->lhsp(),
                        "Bad package link");
            nodep->classOrPackagep(cpackagerefp->classOrPackageSkipp());
            m_ds.m_dotPos = DP_SCOPE;
        } ಇಲ್ಲದಿದ್ದರೆ {
            // Allow REFDTYPE under DOT when referencing interface typedefs
            // This is needed for patterns like: typedef iface.a_t a_t;
            // The dependency graph handles resolution when IfaceCapture is disabled
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ ifaceFinalSegmentAllowed
                = (m_ds.m_dotPos == DP_FINAL) && m_ds.m_dotSymp
                  && VN_IS(m_ds.m_dotSymp->nodep(), Cell)
                  && VN_CAST(m_ds.m_dotSymp->nodep(), Cell)->modp()
                  && VN_IS(VN_CAST(m_ds.m_dotSymp->nodep(), Cell)->modp(), Iface);
            ಆದರೆ (!ifaceFinalSegmentAllowed) {
                checkNoDot(nodep);
            } ಇಲ್ಲದಿದ್ದರೆ {
                UINFO(9, indent() << "allow REFDTYPE under DOT for iface typedef name="
                                  << nodep->prettyNameQ() << " dotSym=" << m_ds.m_dotSymp);
                // Clear the dot state so we don't propagate errors
                m_ds.m_dotPos = DP_SCOPE;
            }
        }
        ಆದರೆ (!nodep->typedefp() && !nodep->subDTypep()) {
            ಸ್ಥಿರಾಂಕ VSymEnt* foundp;
            ಆದರೆ (nodep->classOrPackagep()) {
                foundp = m_statep->get_ಗಂಟು_Sym(nodep->classOrPackagep())->ಕಂಡುಹಿಡಿ_Id_Flat(nodep->name());
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (m_ds.m_dotPos == DP_ಮೊದಲ || m_ds.m_dotPos == DP_NONE) {
                foundp = ಕಂಡುಹಿಡಿ_Id_Fallback_Skip_Member_DType(m_curSymp, nodep->name());
            } ಇಲ್ಲದಿದ್ದರೆ {
                // Defensive: dotPos should be DP_FIRST/DP_NONE or classOrPackagep set.
                v3fatalSrc("Unexpected dotPos="
                           << static_cast<ಪೂರ್ಣಾಂಕ>(m_ds.m_dotPos)  // LCOV_EXCL_LINE
                           << " in RefDType lookup for "
                           << nodep->prettyNameQ());  // LCOV_EXCL_LINE
                foundp = ಶೂನ್ಯಸೂಚಕ;  // LCOV_EXCL_LINE
            }
            {
                ಆದರೆ (AstTypedef* ಸ್ಥಿರಾಂಕ defp
                    = foundp ? VN_CAST(foundp->nodep(), Typedef) : ಶೂನ್ಯಸೂಚಕ) {
                    // Don't check if typedef is to a <type T>::<reference> as might not be
                    // resolved yet
                    ಆದರೆ (!nodep->classOrPackagep() && !defp->isUnderClass())
                        check_Decl_ಕ್ರಮ(nodep, defp);
                    nodep->typedefp(defp);
                    nodep->classOrPackagep(foundp->classOrPackagep());

                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstParamTypeDType* ಸ್ಥಿರಾಂಕ defp
                           = foundp ? VN_CAST(foundp->nodep(), ParamTypeDType) : ಶೂನ್ಯಸೂಚಕ) {
                    // A PARAMTYPEDTYPE's child REFDTYPE referencing itself is the normal
                    // AST structure for type parameters (e.g., "type T = base" has a child
                    // REFDTYPE for the default type). Only error on true recursion where
                    // a TYPEDEF contains a reference back to itself.
                    ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ is_Param_Type_ಮಗು = (defp == nodep->backp());
                    ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ isTypedefRecursion
                        = is_Param_Type_ಮಗು && VN_IS(defp->backp(), Typedef);
                    ಆದರೆ (isTypedefRecursion) {
                        nodep->v3error("Reference to '"
                                       << m_ds.m_dot_ಪಠ್ಯ << (m_ds.m_dot_ಪಠ್ಯ == "" ? "" : ".")
                                       << nodep->prettyName() << "'"
                                       << " type would form a recursive definition");
                        nodep->refDTypep(nodep->ಕಂಡುಹಿಡಿ_Void_DType());  // Try to reduce later errors
                    } ಇಲ್ಲದಿದ್ದರೆ {
                        nodep->refDTypep(defp);
                        nodep->classOrPackagep(foundp->classOrPackagep());
                        captureIfaceParamType(nodep, defp);
                    }
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstClass* ಸ್ಥಿರಾಂಕ defp
                           = foundp ? VN_CAST(foundp->nodep(), Class) : ಶೂನ್ಯಸೂಚಕ) {
                    // Don't check if typedef is to a <type T>::<reference> as might not be
                    // resolved yet
                    ಆದರೆ (!nodep->classOrPackagep()) check_Decl_ಕ್ರಮ(nodep, defp);
                    AstPin* ಸ್ಥಿರಾಂಕ paramsp = nodep->paramsp();
                    ಆದರೆ (paramsp) paramsp->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ();
                    AstClassRefDType* ಸ್ಥಿರಾಂಕ newp
                        = ಹೊಸ AstClassRefDType{nodep->fileline(), defp, paramsp};
                    newp->classOrPackagep(foundp->classOrPackagep());
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                    ಹಿಂತಿರುಗಿ;
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (m_insideClassExtParam) {
                    ಹಿಂತಿರುಗಿ;
                } ಇಲ್ಲದಿದ್ದರೆ {
                    ಆದರೆ (foundp) {
                        UINFO(1, "Found sym node: " << foundp->nodep());
                        nodep->v3error("Expecting a data type: "
                                       << nodep->prettyNameQ()
                                       << ", found: " << foundp->nodep()->prettyTypeName());
                    } ಇಲ್ಲದಿದ್ದರೆ {
                        nodep->v3error("Can't find typedef/interface: " << nodep->prettyNameQ());
                    }
                }
            }
        }
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstRequireDType* nodep) override {
        // Handle type validation for localparam type assignments
        // Iterate child to resolve any ParseRef nodes
        iterateChildren(nodep);
        // Only emit error if the child is not a type.
        // Do NOT unwrap valid types here - leave that to V3Width.
        // Unwrapping here breaks type parameter resolution during cloning.
        // Allow Dot through to defer the handling until the dot expression is resolved.
        ಆದರೆ (nodep->lhsp() && !VN_IS(nodep->lhsp(), ಗಂಟು_DType) && !VN_IS(nodep->lhsp(), Dot)) {
            // Not a type - emit error
            ಆದರೆ (VN_IS(nodep->lhsp(), Const)) {
                nodep->lhsp()->v3error("Expecting a data type, not a constant");
            } ಇಲ್ಲದಿದ್ದರೆ {
                nodep->lhsp()->v3error("Expecting a data type, not "
                                       << nodep->lhsp()->typeName() << ": '"
                                       << nodep->lhsp()->prettyName() << "'");
            }
            // Replace with void to keep AST valid and allow clean exit
            nodep->replaceWith(ಹೊಸ AstVoidDType{nodep->fileline()});
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstDpiExport* nodep) override {
        // AstDpiExport: Make sure the function referenced exists, then dump it
        LINKDOT_ಭೇಟಿ_ಆರಂಭ();
        iterateChildren(nodep);
        checkNoDot(nodep);
        VSymEnt* ಸ್ಥಿರಾಂಕ foundp = m_curSymp->ಕಂಡುಹಿಡಿ_Id_Fallback(nodep->name());
        Ast_ಗಂಟು_FTask* ಸ್ಥಿರಾಂಕ taskp = foundp ? VN_AS(foundp->nodep(), ಗಂಟು_FTask) : ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (!taskp) {
            nodep->v3error(
                "Can't find definition of exported task/function: " << nodep->prettyNameQ());
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (taskp->dpiExport()) {
            nodep->v3error("Function was already DPI Exported, duplicate not allowed: "
                           << nodep->prettyNameQ());
        } ಇಲ್ಲದಿದ್ದರೆ {
            taskp->dpiExport(ನಿಜ);
            ಆದರೆ (nodep->cname() != "") taskp->cname(nodep->cname());
        }
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlink_Fr_ಹಿಂಭಾಗ()), nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstDisable* nodep) override {
        LINKDOT_ಭೇಟಿ_ಆರಂಭ();
        checkNoDot(nodep);
        ತಾರ್ಕಿಕ hasPartSelect = ಸುಳ್ಳು;
        ಸ್ಥಿರಾಂಕ string ಗುರಿ_ಮಾರ್ಗ
            = nodep->ಗುರಿ_Refp() ? ಹೊರತೆಗೆ_Dotted_ಮಾರ್ಗ(nodep->ಗುರಿ_Refp(), hasPartSelect) : "";
        VL_RESTORER_COPY(m_ds);
        m_ds.init(m_curSymp);
        m_ds.m_dotPos = DP_ಮೊದಲ;
        m_ds.m_disablep = nodep;
        iterateChildren(nodep);
        ಆದರೆ (nodep->ಗುರಿ_Refp()) {
            ಆದರೆ (AstTaskRef* ಸ್ಥಿರಾಂಕ taskRefp = VN_CAST(nodep->ಗುರಿ_Refp(), TaskRef)) {
                nodep->targetp(taskRefp->taskp());
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!VN_IS(nodep->ಗುರಿ_Refp(), ParseRef)) {
                // If it is a ParseRef, either it couldn't be linked or it is linked to a block
                nodep->v3warn(E_UNSUPPORTED, "Node of type "
                                                 << nodep->ಗುರಿ_Refp()->prettyTypeName()
                                                 << " referenced by disable");
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlink_Fr_ಹಿಂಭಾಗ()), nodep);
                ಹಿಂತಿರುಗಿ;
            }
            ಆದರೆ (nodep->targetp()) {
                nodep->ಗುರಿ_Refp()->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ();
                ಆದರೆ (!hasPartSelect) {
                    ಸ್ಥಿರಾಂಕ string instance_ಮಾರ್ಗ = ಗುರಿ_Instance_ಮಾರ್ಗ(nodep->targetp(), ಗುರಿ_ಮಾರ್ಗ);
                    ಆದರೆ (!instance_ಮಾರ್ಗ.ಖಾಲಿಗಳು()) {
                        // Keep only the instance prefix so V3LinkJump can reference the selected
                        // instance's process queue without reparsing the disable target.
                        nodep->ಗುರಿ_Refp(
                            ಹೊಸ AstVarXRef{nodep->fileline(), "", instance_ಮಾರ್ಗ, VAccess::READ});
                    }
                }
            }
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ_Package_Import_Or_Export(Ast_ಗಂಟು* nodep) {
        // No longer needed
        LINKDOT_ಭೇಟಿ_ಆರಂಭ();
        checkNoDot(nodep);
        ಆದರೆ (m_statep->forParamed()) VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlink_Fr_ಹಿಂಭಾಗ()), nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstPackageImport* nodep) override { ಭೇಟಿ_Package_Import_Or_Export(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(AstPackageExport* nodep) override { ಭೇಟಿ_Package_Import_Or_Export(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Package_Export_ನಕ್ಷತ್ರ_ನಕ್ಷತ್ರ* nodep) override { ಭೇಟಿ_Package_Import_Or_Export(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(AstCellRef* nodep) override {
        LINKDOT_ಭೇಟಿ_ಆರಂಭ();
        UINFO(5, indent() << "visit " << nodep);
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCellArrayRef* nodep) override {
        LINKDOT_ಭೇಟಿ_ಆರಂಭ();
        UINFO(5, indent() << "visit " << nodep);
        // Expression already iterated
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstUnlinkedRef* nodep) override {
        LINKDOT_ಭೇಟಿ_ಆರಂಭ();
        UINFO(5, indent() << "visit " << nodep);
        // No need to iterate, if we have a UnlinkedVarXRef, we're already done
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ* nodep) override {
        LINKDOT_ಭೇಟಿ_ಆರಂಭ();
        checkNoDot(nodep);
        // Check if nodep represents a super.new call;
        ಆದರೆ (AstNew* ಸ್ಥಿರಾಂಕ newExprp = VN_CAST(nodep->exprp(), New)) {
            // in this case it was already linked, so it doesn't have a super reference
            m_explicitSuperNewp = newExprp;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ AstDot* ಸ್ಥಿರಾಂಕ dotp = VN_CAST(nodep->exprp(), Dot)) {
            ಆದರೆ (dotp->lhsp()->name() == "super") {
                ಆದರೆ (AstNew* ಸ್ಥಿರಾಂಕ newExprp = VN_CAST(dotp->rhsp(), New)) {
                    m_explicitSuperNewp = newExprp;
                }
            }
        }
        iterateChildren(nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstIfaceRefDType* nodep) override {
        ಆದರೆ (nodep->user3_ಗಣ_Once()) ಹಿಂತಿರುಗಿ;
        LINKDOT_ಭೇಟಿ_ಆರಂಭ();
        ಆದರೆ (nodep->paramsp()) {
            // If there is no parameters, there is no need to visit this node.
            AstIface* ಸ್ಥಿರಾಂಕ ifacep = nodep->ifacep();
            UASSERT_OBJ(ifacep, nodep, "Port parameters of AstIfaceRefDType without ifacep()");
            ಆದರೆ (ifacep->dead()) ಹಿಂತಿರುಗಿ;
            checkNoDot(nodep);
            VL_RESTORER_CLEAR(m_ಬಳಸಿದ_Pins);
            VL_RESTORER_CLEAR(m_ಬಳಸಿದ_Def_Param_Pins);
            VL_RESTORER(m_pinSymp);
            m_pinSymp = m_statep->get_ಗಂಟು_Sym(ifacep);
            iterate_And_ಮುಂದಿನ_Null(nodep->paramsp());
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_Attr_ದ* nodep) override { iterateChildren(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(AstCastParse* nodep) override {
        ಆದರೆ (nodep->user3_ಗಣ_Once()) ಹಿಂತಿರುಗಿ;
        LINKDOT_ಭೇಟಿ_ಆರಂಭ();
        // A cast may start a dotted expression: HERE type'(expr) . member_or_method
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ dotLhs = m_ds.m_dotPos == DP_ಮೊದಲ;
        ಆದರೆ (!dotLhs) checkNoDot(nodep);
        {
            VL_RESTORER_COPY(m_ds);
            m_ds.init(m_curSymp);
            iterateChildren(nodep);
        }
        ಆದರೆ (dotLhs) m_ds.m_dotPos = DP_MEMBER;
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ನಿಯೋಜಿಸು_W* nodep) override {
        LINKDOT_ಭೇಟಿ_ಆರಂಭ();
        checkNoDot(nodep);
        VL_RESTORER(m_replaceWithAlias);
        ಆದರೆ (nodep->user2()) m_replaceWithAlias = ಸುಳ್ಳು;
        iterateChildren(nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstTypedef* nodep) override {
        VL_RESTORER(m_resolvingTypedef)
        m_resolvingTypedef = ನಿಜ;
        iterateChildren(nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override {
        VL_RESTORER(m_ಒಳ_Packed_Array);
        ಆದರೆ (VN_IS(nodep, PackArrayDType)) {
            m_ಒಳ_Packed_Array = ನಿಜ;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!m_ಒಳ_Packed_Array) {
            LINKDOT_ಭೇಟಿ_ಆರಂಭ();
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
        ಸ್ಥಿರಾಂಕ std::map<std::string, Ast_ಗಂಟು_Module*> modules_ಗಮ್ಯ_Revisit
            = std::move(m_modules_ಗಮ್ಯ_Revisit);
        m_ಕೊನೆಯ_Deferredp = ಶೂನ್ಯಸೂಚಕ;
        ಗಾಗಿ (ಸ್ವಯಂಚಾಲಿತ& p : modules_ಗಮ್ಯ_Revisit) {
            Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ modp = p.ಎರಡನೇ;
            modp->foreach([](Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ nodep) { nodep->user3(ಸುಳ್ಳು); });
            iterate(modp);
        }
    }
    ~LinkDotResolveVisitor() override = ಪೂರ್ವನಿಯೋಜಿತ;
};

//######################################################################
// Link class functions

ಶೂನ್ಯ V3LinkDot::dumpSubstep(ಸ್ಥಿರಾಂಕ string& name) {
    ಆದರೆ (dump_ವೃಕ್ಷ_Either_Level() >= 9) {
        V3Global::dump_Check_Global_ವೃಕ್ಷ(name);
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (debug() >= 5) {  // on high dbg level, dump even if not explicitly told to
        v3Global.rootp()->dump_ವೃಕ್ಷ_File(v3Global.debugFilename(name + ".tree"));
    }
}

ಶೂನ್ಯ V3LinkDot::linkDotGuts(AstNetlist* rootp, VLink_Dot_ಹೆಜ್ಜೆ ಹೆಜ್ಜೆ) {
    VIsCached::clear_Cache_ವೃಕ್ಷ();  // Avoid using any stale isPure
    dumpSubstep("prelinkdot");
    LinkDotState state{rootp, ಹೆಜ್ಜೆ};

    { Link_Dot_ಕಂಡುಹಿಡಿ_Visitor{rootp, &state}; }
    dumpSubstep("prelinkdot-find");

    { Link_Dot_ಕಂಡುಹಿಡಿ_Iface_Visitor{rootp, &state}; }
    dumpSubstep("prelinkdot-findiface");

    ಆದರೆ (ಹೆಜ್ಜೆ == LDS_SCOPED) {
        // Well after the initial link when we're ready to operate on the flat design,
        // process AstScope's.  This needs to be separate pass after whole hierarchy graph created.
        { LinkDotScopeVisitor{rootp, &state}; }
        v3Global.assertScoped(ನಿಜ);
        dumpSubstep("prelinkdot-scoped");
    } ಇಲ್ಲದಿದ್ದರೆ {
        // Initial link stage, resolve parameters and interfaces
        { LinkDotParamVisitor{rootp, &state}; }
        dumpSubstep("prelinkdot-param");
    }
    state.dumpSelf("prelinkdot");
    state.computeIfaceModSyms();
    state.computeIfaceVarSyms();
    state.computeScopeAliases();
    state.dumpSelf("linkdot-preresolve");
    ಸ್ಥಿರಾಂಕ LinkDotResolveVisitor visitor{rootp, &state};
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
