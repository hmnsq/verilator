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
//ਸਿਖਰ
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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3LinkDot.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Global.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Graph.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3LinkDotIfaceCapture.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3MemberMap.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Parse.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Randomize.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Stats.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3String.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3SymTable.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <algorithm>
#ਅਨ੍ਤਰ੍ਭਾਵ <cstdlib>
#ਅਨ੍ਤਰ੍ਭਾਵ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

ਸਥਿਰ string ਕੱਢੋ_Dotted_ਰਸਤਾ(Ast_ਗੰਢ* nodep, ਤਾਰਕਿਕ& hasPartSelect) {
    ਜੇ (AstParseRef* ਅਚਲ refp = VN_CAST(nodep, ParseRef)) {
        ਵਾਪਸ refp->name();
    } ਨਹੀਂਤਾਂ ਜੇ (AstVarRef* ਅਚਲ refp = VN_CAST(nodep, VarRef)) {
        ਵਾਪਸ refp->name();
    } ਨਹੀਂਤਾਂ ਜੇ (AstDot* ਅਚਲ dotp = VN_CAST(nodep, Dot)) {
        ਅਚਲ string lhs = ਕੱਢੋ_Dotted_ਰਸਤਾ(dotp->lhsp(), hasPartSelect);
        ਅਚਲ string rhs = ਕੱਢੋ_Dotted_ਰਸਤਾ(dotp->rhsp(), hasPartSelect);
        ਵਾਪਸ VString::dot(lhs, ".", rhs);
    } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(nodep, Sel_ਦੋਆਧਾਰੀ_ਅੰਕ) || VN_IS(nodep, Sel_ਕੱਢੋ) || VN_IS(nodep, SelPlus)
               || VN_IS(nodep, SelMinus)) {
        hasPartSelect = ਸੱਚ;
        ਵਾਪਸ "";
    }
    ਵਾਪਸ "";
}
ਸਥਿਰ string lexical_Disable_ਰਸਤਾ(Ast_ਗੰਢ* ਅਚਲ targetp) {
    std::ਸਦਿਸ਼<string> ਨਾਂ;
    ਲਈ (Ast_ਗੰਢ* curp = targetp; curp; curp = curp->aboveLoopp()) {
        ਜੇ (Ast_ਗੰਢ_ਖੰਡ* ਅਚਲ blockp = VN_CAST(curp, ਗੰਢ_ਖੰਡ)) {
            ਜੇ (blockp->name() != "") ਨਾਂ.ਧੱਕੋ_ਪਿੱਠ(blockp->name());
        } ਨਹੀਂਤਾਂ ਜੇ (Ast_ਗੰਢ_FTask* ਅਚਲ ftaskp = VN_CAST(curp, ਗੰਢ_FTask)) {
            ਨਾਂ.ਧੱਕੋ_ਪਿੱਠ(ftaskp->name());
        } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(curp, ਗੰਢ_Module)) {
            ਰੋਕੋ;
        }
    }
    string ਰਸਤਾ;
    ਲਈ (ਸਵੈਚਾਲਿਤ it = ਨਾਂ.crbegin(); it != ਨਾਂ.crend(); ++it) {
        ਰਸਤਾ = VString::dot(ਰਸਤਾ, ".", *it);
    }
    ਵਾਪਸ ਰਸਤਾ;
}
ਸਥਿਰ string ਨਿਸ਼ਾਨਾ_Instance_ਰਸਤਾ(Ast_ਗੰਢ* ਅਚਲ targetp, ਅਚਲ string& ਨਿਸ਼ਾਨਾ_ਰਸਤਾ) {
    ਅਚਲ string lexical_ਰਸਤਾ = lexical_Disable_ਰਸਤਾ(targetp);
    ਜੇ (lexical_ਰਸਤਾ == "" || ਨਿਸ਼ਾਨਾ_ਰਸਤਾ == lexical_ਰਸਤਾ) ਵਾਪਸ "";
    ਅਚਲ string suffix = "." + lexical_ਰਸਤਾ;
    ਜੇ (!VString::ਸਿਰੇ_With(ਨਿਸ਼ਾਨਾ_ਰਸਤਾ, suffix)) ਵਾਪਸ "";
    ਵਾਪਸ ਨਿਸ਼ਾਨਾ_ਰਸਤਾ.substr(0, ਨਿਸ਼ਾਨਾ_ਰਸਤਾ.ਮਾਪ() - suffix.ਮਾਪ());
}

// ######################################################################
//  Matcher classes (for suggestion matching)

ਵਰਗ Link_ਗੰਢ_Matcher_Class_Or_Package final : ਜਨਤਕ VNodeMatcher {
ਜਨਤਕ:
    ਤਾਰਕਿਕ ਗੰਢ_ਮੇਲ(ਅਚਲ Ast_ਗੰਢ* nodep) ਅਚਲ override {
        ਵਾਪਸ VN_IS(nodep, Class) || VN_IS(nodep, Package);
    }
};
ਵਰਗ Link_ਗੰਢ_Matcher_FTask final : ਜਨਤਕ VNodeMatcher {
ਜਨਤਕ:
    ਤਾਰਕਿਕ ਗੰਢ_ਮੇਲ(ਅਚਲ Ast_ਗੰਢ* nodep) ਅਚਲ override { ਵਾਪਸ VN_IS(nodep, ਗੰਢ_FTask); }
};
ਵਰਗ Link_ਗੰਢ_Matcher_Modport final : ਜਨਤਕ VNodeMatcher {
ਜਨਤਕ:
    ਤਾਰਕਿਕ ਗੰਢ_ਮੇਲ(ਅਚਲ Ast_ਗੰਢ* nodep) ਅਚਲ override { ਵਾਪਸ VN_IS(nodep, Modport); }
};
ਵਰਗ Link_ਗੰਢ_Matcher_Prod final : ਜਨਤਕ VNodeMatcher {
ਜਨਤਕ:
    ਤਾਰਕਿਕ ਗੰਢ_ਮੇਲ(ਅਚਲ Ast_ਗੰਢ* nodep) ਅਚਲ override { ਵਾਪਸ VN_IS(nodep, RSProd); }
};
ਵਰਗ Link_ਗੰਢ_Matcher_Var final : ਜਨਤਕ VNodeMatcher {
ਜਨਤਕ:
    ਤਾਰਕਿਕ ਗੰਢ_ਮੇਲ(ਅਚਲ Ast_ਗੰਢ* nodep) ਅਚਲ override {
        ਵਾਪਸ VN_IS(nodep, Var) || VN_IS(nodep, LambdaArgRef);
    }
};
ਵਰਗ Link_ਗੰਢ_Matcher_Var_IO final : ਜਨਤਕ VNodeMatcher {
ਜਨਤਕ:
    ਤਾਰਕਿਕ ਗੰਢ_ਮੇਲ(ਅਚਲ Ast_ਗੰਢ* nodep) ਅਚਲ override {
        ਅਚਲ AstVar* ਅਚਲ varp = VN_CAST(nodep, Var);
        ਜੇ (!varp) ਵਾਪਸ ਝੂਠ;
        ਵਾਪਸ varp->isIO();
    }
};
ਵਰਗ Link_ਗੰਢ_Matcher_Var_Param final : ਜਨਤਕ VNodeMatcher {
ਜਨਤਕ:
    ਤਾਰਕਿਕ ਗੰਢ_ਮੇਲ(ਅਚਲ Ast_ਗੰਢ* nodep) ਅਚਲ override {
        ਅਚਲ AstVar* ਅਚਲ varp = VN_CAST(nodep, Var);
        ਜੇ (!varp) ਵਾਪਸ ਝੂਠ;
        ਵਾਪਸ varp->isParam();
    }
};

//######################################################################
// LinkDot state, as a visitor of each AstNode

ਵਰਗ LinkDotState final {
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
    ਅਚਲ VNUser1_ਵਿੱਚ_Use m_inuser1;
    ਅਚਲ VNUser2_ਵਿੱਚ_Use m_inuser2;
    ਅਚਲ VNUser4_ਵਿੱਚ_Use m_inuser4;

ਜਨਤਕ:
    // ENUMS
    // In order of priority, compute first ... compute last
    ਗਿਣਤੀ SAMNum : uint8_t { SAMN_MODPORT, SAMN_IFTOP, SAMN__MAX };  // Values for m_scopeAliasMap

ਨਿਜੀ:
    // TYPES
    ਵਰਤੋਂ ScopeAliasMap = std::unordered_map<VSymEnt*, VSymEnt*>;
    ਵਰਤੋਂ IfaceModSyms = std::ਸਦਿਸ਼<std::pair<AstIface*, VSymEnt*>>;

    ਸਥਿਰ LinkDotState* s_errorThisp;  // Last self, for error reporting only

    // MEMBERS
    VSym_ਗ੍ਰਾਫ m_syms;  // Symbol table by hierarchy
    VSym_ਗ੍ਰਾਫ m_mods;  // Symbol table of all module names
    VSymEnt* m_dunitEntp = ਖਾਲੀ_ਸੂਚਕ;  // $unit entry
    std::multimap<std::string, VSymEnt*>
        m_nameScopeSymMap;  // Map of scope referenced by non-pretty textual name
    std::ਸੈੱਟ<std::pair<Ast_ਗੰਢ_Module*, std::string>>
        m_implicit_Name_ਸੈੱਟ;  // For [module][signalname] if we can implicitly create it
    std::array<ScopeAliasMap, SAMN__MAX> m_scopeAliasMap;  // Map of <lhs,rhs> aliases
    std::ਸਦਿਸ਼<VSymEnt*> m_ifaceVarSyms;  // List of AstIfaceRefDType's to be imported
    IfaceModSyms m_ifaceModSyms;  // List of AstIface+Symbols to be processed
    ਅਚਲ VLink_Dot_ਕਦਮ m_ਕਦਮ;  // Operational step

ਜਨਤਕ:
    // METHODS

    ਖਾਲੀ dumpSelf(ਅਚਲ string& nameComment, ਤਾਰਕਿਕ force = ਝੂਠ) {
        ਜੇ (debug() >= 6 || dumpLevel() >= 6 || force) {
            ਅਚਲ string filename = v3Global.debugFilename(nameComment) + ".txt";
            UINFO(4, "Dumping " << filename);
            ਅਚਲ std::unique_ptr<std::ofstream> logp{V3File::new_ofstream(filename)};
            ਜੇ (logp->fail()) v3fatal("Can't write file: " << filename);
            std::ostream& os = *logp;
            // TODO the symbol table has node pointers which may be
            // dangling, as we call deleteTree in these visitors without
            // pushing for later deletion (or deleting from the symbol table)
            // So, only completely safe to call this when under --debug or --debug-leak.
            m_syms.dumpSelf(os);
            ਤਾਰਕਿਕ ਪਹਿਲਾ = ਸੱਚ;
            ਲਈ (ਪੂਰਨ samn = 0; samn < SAMN__MAX; ++samn) {
                ਜੇ (!m_scopeAliasMap[samn].ਖਾਲੀ_ਥਾਵਾਂ()) {
                    ਜੇ (ਪਹਿਲਾ) os << "\nScopeAliasMap:\n";
                    ਪਹਿਲਾ = ਝੂਠ;
                    ਲਈ (ScopeAliasMap::iterator it = m_scopeAliasMap[samn].begin();
                         it != m_scopeAliasMap[samn].ਸਿਰਾ(); ++it) {
                        // left side is what we will import into
                        os << "\t" << samn << "\t" << it->ਪਹਿਲਾ << " ("
                           << it->ਪਹਿਲਾ->nodep()->typeName() << ") <- " << it->ਦੂਜਾ << " "
                           << it->ਦੂਜਾ->nodep() << '\n';
                    }
                }
            }
        }
    }
    ਸਥਿਰ ਖਾਲੀ preErrorDumpHandler() {
        ਜੇ (s_errorThisp) s_errorThisp->preErrorDump();
    }
    ਖਾਲੀ preErrorDump() {
        ਸਥਿਰ ਤਾਰਕਿਕ s_diddump = ਝੂਠ;
        ਜੇ (!s_diddump && dump_ਰੁੱਖ_Level()) {
            s_diddump = ਸੱਚ;
            dumpSelf("linkdot-preerr", ਸੱਚ);
            v3Global.rootp()->dump_ਰੁੱਖ_File(v3Global.debugFilename("linkdot-preerr.tree"));
        }
    }

    // CONSTRUCTORS
    LinkDotState(AstNetlist* rootp, VLink_Dot_ਕਦਮ ਕਦਮ)
        : m_syms{rootp}
        , m_mods{rootp}
        , m_ਕਦਮ{ਕਦਮ} {
        UINFO(4, __FUNCTION__ << ": ");
        s_errorThisp = ਇਹ;
        V3Error::errorExitCb(preErrorDumpHandler);  // If get error, dump self
        read_Mod_ਨਾਂ();
    }
    ~LinkDotState() {
        V3Error::errorExitCb(ਖਾਲੀ_ਸੂਚਕ);
        s_errorThisp = ਖਾਲੀ_ਸੂਚਕ;
    }

    // ACCESSORS
    VSym_ਗ੍ਰਾਫ* symsp() { ਵਾਪਸ &m_syms; }
    ਪੂਰਨ ਕਦਮ_ਸੰਖਿਆ() ਅਚਲ { ਵਾਪਸ static_cast<ਪੂਰਨ>(m_ਕਦਮ); }
    ਤਾਰਕਿਕ forPrimary() ਅਚਲ { ਵਾਪਸ m_ਕਦਮ == LDS_PRIMARY; }
    ਤਾਰਕਿਕ forParamed() ਅਚਲ { ਵਾਪਸ m_ਕਦਮ == LDS_PARAMED; }
    ਤਾਰਕਿਕ forScopeCreation() ਅਚਲ { ਵਾਪਸ m_ਕਦਮ == LDS_SCOPED; }

    // METHODS
    ਸਥਿਰ string ਗੰਢ_ਪਾਠ_Type(Ast_ਗੰਢ* nodep) {
        ਜੇ (ਅਚਲ AstVar* ਅਚਲ varp = VN_CAST(nodep, Var)) {
            ਜੇ (varp->isIO() || varp->isIfaceRef()) {
                ਵਾਪਸ "port";
            } ਨਹੀਂਤਾਂ ਜੇ (varp->isGParam()) {
                ਵਾਪਸ "parameter";
            } ਨਹੀਂਤਾਂ ਜੇ (varp->varType() == VVarType::LPARAM) {
                ਵਾਪਸ "local parameter";
            } ਨਹੀਂਤਾਂ {
                ਵਾਪਸ "variable";
            }
        } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ AstParamTypeDType* ਅਚਲ dtypep = VN_CAST(nodep, ParamTypeDType)) {
            ਜੇ (dtypep->isGParam()) {
                ਵਾਪਸ "type parameter";
            } ਨਹੀਂਤਾਂ {
                ਵਾਪਸ "local type parameter";
            }
        } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(nodep, Begin)) {
            ਵਾਪਸ "block";
        } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(nodep, Cell)) {
            ਵਾਪਸ "instance";
        } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(nodep, Constraint)) {
            ਵਾਪਸ "constraint";
        } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(nodep, Func)) {
            ਵਾਪਸ "function";
        } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(nodep, Gen_ਖੰਡ)) {
            ਵਾਪਸ "generate block";
        } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(nodep, Iface)) {
            ਵਾਪਸ "interface";
        } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(nodep, RSProd)) {
            ਵਾਪਸ "randsequence production";
        } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(nodep, Task)) {
            ਵਾਪਸ "task";
        } ਨਹੀਂਤਾਂ {
            ਵਾਪਸ nodep->prettyTypeName();
        }
    }

    Ast_ਗੰਢ_Module* ਲੱਭੋ_Module_Sym(ਅਚਲ string& modName) {
        ਅਚਲ VSymEnt* ਅਚਲ foundp = m_mods.rootp()->ਲੱਭੋ_Id_Fallback(modName);
        ਵਾਪਸ foundp ? VN_AS(foundp->nodep(), ਗੰਢ_Module) : ਖਾਲੀ_ਸੂਚਕ;
    }
    ਖਾਲੀ read_Mod_ਨਾਂ() {
        // Look at all modules, and store pointers to all module names
        ਲਈ (Ast_ਗੰਢ_Module *nextp, *nodep = v3Global.rootp()->modulesp(); nodep; nodep = nextp) {
            nextp = VN_AS(nodep->nextp(), ਗੰਢ_Module);
            m_mods.rootp()->ਪਾਓ(nodep->name(), ਨਵਾਂ VSymEnt{&m_mods, nodep});
        }
    }

    VSymEnt* ਜੜ੍ਹ_Entp() ਅਚਲ { ਵਾਪਸ m_syms.rootp(); }
    VSymEnt* dunitEntp() ਅਚਲ { ਵਾਪਸ m_dunitEntp; }
    ਖਾਲੀ checkDuplicate(VSymEnt* lookupSymp, Ast_ਗੰਢ* nodep, ਅਚਲ string& name) {
        // Lookup the given name under current symbol table
        // Insert if not found
        // Report error if there's a duplicate
        //
        // Note we only check for conflicts at the same level; it's ok if one block hides another
        // We also wouldn't want to not insert it even though it's lower down

        ਅਚਲ VSymEnt* ਅਚਲ foundp = lookupSymp->ਲੱਭੋ_Id_Flat(name);
        Ast_ਗੰਢ* ਅਚਲ fnodep = foundp ? foundp->nodep() : ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (!fnodep) {
            // Not found, will add in a moment.
            ਜੇ (forPrimary() && VSymEnt::checkSimilarname(nodep)) {
                ਅਚਲ VSymEnt* ਅਚਲ alt = lookupSymp->ਲੱਭੋ_Similar_Id_Flat(name);
                ਜੇ (alt) {
                    nodep->v3warn(SIMILARNAME, "Declaration overlaps another with different case: "
                                                   << nodep->prettyNameQ() << '\n'
                                                   << nodep->warnContextPrimary() << '\n'
                                                   << alt->nodep()->warnOther()
                                                   << "... Location of original declaration\n"
                                                   << alt->nodep()->warnContextSecondary());
                }
            }
        } ਨਹੀਂਤਾਂ ਜੇ (nodep == fnodep) {  // Already inserted.
            // Good.
        } ਨਹੀਂਤਾਂ ਜੇ (foundp->imported()) {  // From package
            // We don't throw VARHIDDEN as if the import is later the symbol
            // table's import wouldn't warn
        } ਨਹੀਂਤਾਂ ਜੇ (forPrimary() && VN_IS(nodep, Gen_ਖੰਡ)
                   && (VN_IS(fnodep, Begin) || VN_IS(fnodep, Gen_ਖੰਡ))) {
            // Begin: ... blocks often replicate under genif/genfor, so
            // suppress duplicate checks.  See t_gen_forif.v for an example.
        } ਨਹੀਂਤਾਂ {
            UINFO(4, "name " << name);  // Not always same as nodep->name
            UINFO(4, "Var1 " << nodep);
            UINFO(4, "Var2 " << fnodep);
            // Quote the node's own name as for messages, if it is the symbol's
            ਅਚਲ string nameQ
                = fnodep->name() == name ? fnodep->prettyNameQ() : Ast_ਗੰਢ::prettyNameQ(name);
            ਜੇ (nodep->type() == fnodep->type()) {
                nodep->v3error("Duplicate declaration of "
                               << ਗੰਢ_ਪਾਠ_Type(fnodep) << ": " << nameQ << '\n'
                               << nodep->warnContextPrimary() << '\n'
                               << fnodep->warnOther() << "... Location of original declaration\n"
                               << fnodep->warnContextSecondary());
                nodep->decl_Token_ਅੰਕ_ਸੈੱਟ_Min(0);  // Disable checking forward typedefs
                fnodep->decl_Token_ਅੰਕ_ਸੈੱਟ_Min(0);  // Disable checking forward typedefs
            } ਨਹੀਂਤਾਂ {
                nodep->v3error("Unsupported in C: "
                               << ucfirst(ਗੰਢ_ਪਾਠ_Type(nodep)) << " has the same name as "
                               << ਗੰਢ_ਪਾਠ_Type(fnodep) << ": " << nameQ << '\n'
                               << nodep->warnContextPrimary() << '\n'
                               << fnodep->warnOther() << "... Location of original declaration\n"
                               << fnodep->warnContextSecondary());
            }
        }
    }
    ਖਾਲੀ ਪਾਓ_DUnit(AstNetlist* nodep) {
        // $unit on top scope
        VSymEnt* ਅਚਲ symp = ਨਵਾਂ VSymEnt{&m_syms, nodep};
        UINFO(9, "      INSERTdunit se" << cvt_ਮੰਜ਼ਿਲ_Hex(symp));
        symp->parentp(ਜੜ੍ਹ_Entp());  // Needed so backward search can find name of top module
        symp->fallbackp(ਖਾਲੀ_ਸੂਚਕ);
        ਜੜ੍ਹ_Entp()->ਪਾਓ("$unit ", symp);  // Space so can never name conflict with user code
        //
        UASSERT_OBJ(!m_dunitEntp, nodep, "Call insertDUnit only once");
        m_dunitEntp = symp;
    }
    VSymEnt* ਪਾਓ_ਸਿਖਰ_Cell(Ast_ਗੰਢ_Module* nodep, ਅਚਲ string& scopename) {
        // Only called on the module at the very top of the hierarchy
        VSymEnt* ਅਚਲ symp = ਨਵਾਂ VSymEnt{&m_syms, nodep};
        UINFO(9, "      INSERTtop se" << cvt_ਮੰਜ਼ਿਲ_Hex(symp) << "  " << scopename << " " << nodep);
        symp->parentp(ਜੜ੍ਹ_Entp());  // Needed so backward search can find name of top module
        symp->fallbackp(dunitEntp());  // Needed so can find $unit stuff
        nodep->user1p(symp);
        checkDuplicate(ਜੜ੍ਹ_Entp(), nodep, nodep->origName());
        ਜੜ੍ਹ_Entp()->ਪਾਓ(nodep->origName(), symp);
        ਜੇ (forScopeCreation()) m_nameScopeSymMap.emplace(scopename, symp);
        ਵਾਪਸ symp;
    }
    VSymEnt* ਪਾਓ_ਸਿਖਰ_Iface(AstCell* nodep, ਅਚਲ string& scopename) {
        VSymEnt* ਅਚਲ symp = ਨਵਾਂ VSymEnt{&m_syms, nodep};
        UINFO(9, "      INSERTtopiface se" << cvt_ਮੰਜ਼ਿਲ_Hex(symp) << "  " << scopename << " " << nodep);
        symp->parentp(ਜੜ੍ਹ_Entp());  // Needed so backward search can find name of top module
        symp->fallbackp(dunitEntp());  // Needed so can find $unit stuff
        nodep->user1p(symp);
        ਜੇ (nodep->modp()) nodep->modp()->user1p(symp);
        checkDuplicate(ਜੜ੍ਹ_Entp(), nodep, nodep->origName());
        ਜੜ੍ਹ_Entp()->ਪਾਓ(nodep->origName(), symp);
        ਜੇ (forScopeCreation()) m_nameScopeSymMap.emplace(scopename, symp);
        ਵਾਪਸ symp;
    }
    VSymEnt* ਪਾਓ_Cell(VSymEnt* abovep, VSymEnt* modSymp, AstCell* nodep,
                        ਅਚਲ string& scopename) {
        UASSERT_OBJ(abovep, nodep, "Null symbol table inserting node");
        VSymEnt* ਅਚਲ symp = ਨਵਾਂ VSymEnt{&m_syms, nodep};
        UINFO(9, "      INSERTcel se" << cvt_ਮੰਜ਼ਿਲ_Hex(symp) << "  " << scopename << " above=se"
                                      << cvt_ਮੰਜ਼ਿਲ_Hex(abovep) << " mods=se" << cvt_ਮੰਜ਼ਿਲ_Hex(modSymp)
                                      << " node=" << nodep);
        symp->parentp(abovep);
        symp->fallbackp(dunitEntp());  // Needed so can find $unit stuff
        nodep->user1p(symp);
        ਜੇ (nodep->modp()) nodep->modp()->user1p(symp);
        checkDuplicate(abovep, nodep, nodep->origName());
        abovep->reinsert(nodep->origName(), symp);
        ਜੇ (forScopeCreation() && abovep != modSymp && !modSymp->ਲੱਭੋ_Id_Flat(nodep->name())) {
            // If it's foo_DOT_bar, we need to be able to find it under "foo_DOT_bar" too.
            // Duplicates are possible, as until resolve generates might
            // have 2 same cells under an if
            modSymp->reinsert(nodep->name(), symp);
        }
        ਜੇ (forScopeCreation()) m_nameScopeSymMap.emplace(scopename, symp);
        ਵਾਪਸ symp;
    }
    ਖਾਲੀ ਪਾਓ_Map(VSymEnt* symp, ਅਚਲ string& scopename) {
        ਜੇ (forScopeCreation()) m_nameScopeSymMap.emplace(scopename, symp);
    }

    VSymEnt* ਪਾਓ_Inline(VSymEnt* abovep, VSymEnt* modSymp, AstCellInline* nodep,
                          ਅਚਲ string& basename) {
        // A fake point in the hierarchy, corresponding to a named block flattened by V3Begin.
        // Names are looked up in the module with the block's flattened prefix.
        UASSERT_OBJ(abovep, nodep, "Null symbol table inserting node");
        VSymEnt* ਅਚਲ symp = ਨਵਾਂ VSymEnt{&m_syms, nodep};
        UINFO(9, "      INSERTinl se" << cvt_ਮੰਜ਼ਿਲ_Hex(symp) << "  " << basename << " above=se"
                                      << cvt_ਮੰਜ਼ਿਲ_Hex(abovep) << " mods=se" << cvt_ਮੰਜ਼ਿਲ_Hex(modSymp)
                                      << " node=" << nodep);
        symp->parentp(abovep);
        symp->fallbackp(modSymp);
        symp->sym_ਅਗੇਤਰ(nodep->name() + "__DOT__");
        nodep->user1p(symp);
        checkDuplicate(abovep, nodep, nodep->name());
        abovep->reinsert(basename, symp);
        ਜੇ (abovep != modSymp && !modSymp->ਲੱਭੋ_Id_Flat(nodep->name())) {
            // If it's foo_DOT_bar, we need to be able to find it under that too.
            modSymp->reinsert(nodep->name(), symp);
        }
        ਵਾਪਸ symp;
    }
    VSymEnt* ਪਾਓ_ਖੰਡ(VSymEnt* abovep, ਅਚਲ string& name, Ast_ਗੰਢ* nodep,
                         Ast_ਗੰਢ_Module* classOrPackagep) {
        // A fake point in the hierarchy, corresponding to a begin or function/task block
        // After we remove begins these will go away
        // Note we fallback to the symbol table of the parent, as we want to find variables there
        // However, cells walk the graph, so cells will appear under the begin/ftask itself
        UASSERT_OBJ(abovep, nodep, "Null symbol table inserting node");
        VSymEnt* ਅਚਲ symp = ਨਵਾਂ VSymEnt{&m_syms, nodep};
        UINFO(9, "      INSERTblk se" << cvt_ਮੰਜ਼ਿਲ_Hex(symp) << "  above=se" << cvt_ਮੰਜ਼ਿਲ_Hex(abovep)
                                      << " pkg=" << cvt_ਮੰਜ਼ਿਲ_Hex(classOrPackagep)
                                      << "  node=" << nodep);
        symp->parentp(abovep);
        symp->classOrPackagep(classOrPackagep);
        symp->fallbackp(abovep);
        nodep->user1p(symp);
        ਜੇ (name != "") checkDuplicate(abovep, nodep, name);
        // Duplicates are possible, as until resolve generates might have 2 same cells under an if
        abovep->reinsert(name, symp);
        ਵਾਪਸ symp;
    }
    VSymEnt* ਪਾਓ_Sym(VSymEnt* abovep, ਅਚਲ string& name, Ast_ਗੰਢ* nodep,
                       Ast_ਗੰਢ_Module* classOrPackagep) {
        UASSERT_OBJ(abovep, nodep, "Null symbol table inserting node");
        VSymEnt* ਅਚਲ symp = ਨਵਾਂ VSymEnt{&m_syms, nodep};
        UINFO(9, "      INSERTsym se" << cvt_ਮੰਜ਼ਿਲ_Hex(symp) << "  name='" << name << "' above=se"
                                      << cvt_ਮੰਜ਼ਿਲ_Hex(abovep) << " pkg=" << cvt_ਮੰਜ਼ਿਲ_Hex(classOrPackagep)
                                      << "  node=" << nodep);
        // We don't remember the ent associated with each node, because we
        // need a unique scope entry for each instantiation
        symp->classOrPackagep(classOrPackagep);
        symp->parentp(abovep);
        symp->fallbackp(abovep);
        nodep->user1p(symp);
        checkDuplicate(abovep, nodep, name);
        abovep->reinsert(name, symp);
        ਵਾਪਸ symp;
    }
    ਸਥਿਰ ਤਾਰਕਿਕ exists_ਗੰਢ_Sym(Ast_ਗੰਢ* nodep) { ਵਾਪਸ nodep->user1p() != ਖਾਲੀ_ਸੂਚਕ; }
    ਸਥਿਰ VSymEnt* get_ਗੰਢ_Sym(Ast_ਗੰਢ* nodep) {
        // Don't use this in ResolveVisitor, as we need to pick up the proper
        // reference under each SCOPE
        VSymEnt* ਅਚਲ symp = nodep->user1u().ਮੰਜ਼ਿਲ_Sym_Ent();
        UASSERT_OBJ(symp, nodep, "Module/etc never assigned a symbol entry?");
        ਵਾਪਸ symp;
    }
    VSymEnt* getScopeSym(AstScope* nodep) {
        ਅਚਲ ਸਵੈਚਾਲਿਤ it = m_nameScopeSymMap.ਲੱਭੋ(nodep->name());
        UASSERT_OBJ(it != m_nameScopeSymMap.ਸਿਰਾ(), nodep,
                    "Scope never assigned a symbol entry '" << nodep->name() << "'");
        ਵਾਪਸ it->ਦੂਜਾ;
    }
    ਖਾਲੀ implicit_Ok_ਜੋੜੋ(Ast_ਗੰਢ_Module* nodep, ਅਚਲ string& varname) {
        // Mark the given variable name as being allowed to be implicitly declared
        ਜੇ (nodep) {
            ਅਚਲ ਸਵੈਚਾਲਿਤ it = m_implicit_Name_ਸੈੱਟ.ਲੱਭੋ(std::make_pair(nodep, varname));
            ਜੇ (it == m_implicit_Name_ਸੈੱਟ.ਸਿਰਾ()) m_implicit_Name_ਸੈੱਟ.emplace(nodep, varname);
        }
    }
    ਤਾਰਕਿਕ implicitOk(Ast_ਗੰਢ_Module* nodep, ਅਚਲ string& varname) {
        ਵਾਪਸ nodep
               && (m_implicit_Name_ਸੈੱਟ.ਲੱਭੋ(std::make_pair(nodep, varname))
                   != m_implicit_Name_ਸੈੱਟ.ਸਿਰਾ());
    }

    // Track and later recurse interface modules
    ਖਾਲੀ ਪਾਓ_Iface_Mod_Sym(AstIface* nodep, VSymEnt* symp) {
        m_ifaceModSyms.emplace_ਪਿੱਠ(nodep, symp);
    }
    ਖਾਲੀ computeIfaceModSyms();

    // Track and later insert interface references
    ਖਾਲੀ ਪਾਓ_Iface_Var_Sym(VSymEnt* symp) {  // Where sym is for a VAR of dtype IFACEREFDTYPE
        m_ifaceVarSyms.ਧੱਕੋ_ਪਿੱਠ(symp);
    }
    // Iface for a raw or arrayed iface; peels nested array layers for multi-dim arrays.
    ਸਥਿਰ AstIfaceRefDType* ifaceRefFromArray(Ast_ਗੰਢ_DType* nodep) {
        ਜਦੋਂ (nodep) {
            ਜੇ (AstIfaceRefDType* ਅਚਲ ifp = VN_CAST(nodep, IfaceRefDType)) ਵਾਪਸ ifp;
            ਜੇ (ਅਚਲ AstBracketArrayDType* ਅਚਲ arrp = VN_CAST(nodep, BracketArrayDType)) {
                nodep = arrp->subDTypep();
            } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ AstUnpackArrayDType* ਅਚਲ arrp = VN_CAST(nodep, UnpackArrayDType)) {
                nodep = arrp->subDTypep();
            } ਨਹੀਂਤਾਂ {
                ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
            }
        }
        ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
    }
    // Given a pin expression, resolve it to a live AstIface* (or nullptr).
    // Handles both simple VarRef and dotted VarXRef pin connections.
    AstIface* ਜੀਵਤ_Iface_From_Pin_ਵਿਅੰਜਕ(Ast_ਗੰਢ* exprp, VSymEnt* ਮਾਪਾ_Symp) {
        AstVar* resolvedVarp = ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (AstVarRef* ਅਚਲ refp = VN_CAST(exprp, VarRef)) {
            resolvedVarp = refp->varp();
        } ਨਹੀਂਤਾਂ ਜੇ (AstVarXRef* ਅਚਲ xrefp = VN_CAST(exprp, VarXRef)) {
            VSymEnt* ਅਚਲ lookupSymp = ਮਾਪਾ_Symp ? ਮਾਪਾ_Symp->parentp() : ਖਾਲੀ_ਸੂਚਕ;
            ਜੇ (lookupSymp) {
                string baddot;
                VSymEnt* okSymp = ਖਾਲੀ_ਸੂਚਕ;
                ਅਚਲ string dotpath = xrefp->dotted() + "." + xrefp->name();
                VSymEnt* ਅਚਲ dotSymp
                    = ਲੱਭੋ_Dotted(xrefp->fileline(), lookupSymp, dotpath, baddot, okSymp, ਸੱਚ);
                ਜੇ (dotSymp) resolvedVarp = VN_CAST(dotSymp->nodep(), Var);
            }
        }
        AstIface* resultp = ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (resolvedVarp) {
            AstIfaceRefDType* ਅਚਲ irefp = ifaceRefFromArray(resolvedVarp->subDTypep());
            ਜੇ (irefp && irefp->iface_ਰਾਹੀਂ_Cellp() && !irefp->iface_ਰਾਹੀਂ_Cellp()->dead()) {
                resultp = irefp->iface_ਰਾਹੀਂ_Cellp();
            }
        }
        ਵਾਪਸ resultp;
    }

    // Attempt to repair a port's AstIfaceRefDType by tracing through the
    // parent cell's pin connection to find the correct live interface.
    // Returns true if repaired or already correct; false if unresolvable.
    ਤਾਰਕਿਕ repairIfaceRef(VSymEnt* varSymp, AstVar* varp, AstIfaceRefDType* ifacerefp, ਤਾਰਕਿਕ isDead) {
        // Walk up symbol-table parents to find the enclosing AstCell
        VSymEnt* ਮਾਪਾ_Symp = varSymp->parentp();
        AstCell* ਮਾਪਾ_Cellp = ਖਾਲੀ_ਸੂਚਕ;
        ਲਈ (ਪੂਰਨ ਡੂੰਘਾਈ = 0; ਮਾਪਾ_Symp && ਡੂੰਘਾਈ < 20; ++ਡੂੰਘਾਈ) {
            ਜੇ (AstCell* ਅਚਲ cp = VN_CAST(ਮਾਪਾ_Symp->nodep(), Cell)) {
                ਮਾਪਾ_Cellp = cp;
                ਰੋਕੋ;
            }
            ਮਾਪਾ_Symp = ਮਾਪਾ_Symp->parentp();
        }
        ਜੇ (!ਮਾਪਾ_Cellp) ਵਾਪਸ ਝੂਠ;
        // Scan pins to find the one connected to this port variable
        ਲਈ (AstPin* pinp = ਮਾਪਾ_Cellp->pinsp(); pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            ਜੇ (pinp->modVarp() != varp && pinp->name() != varp->name()) ਜਾਰੀ;
            Ast_ਗੰਢ* ਅਚਲ exprp = pinp->exprp();
            ਜੇ (!exprp) ਵਾਪਸ ਝੂਠ;
            AstIface* ਅਚਲ newIfacep = ਜੀਵਤ_Iface_From_Pin_ਵਿਅੰਜਕ(exprp, ਮਾਪਾ_Symp);
            ਜੇ (newIfacep && newIfacep != ifacerefp->iface_ਰਾਹੀਂ_Cellp()) {
                UINFO(4, "  REPAIR-IFACE-REF var=" << varp->prettyNameQ()
                                                   << " old=" << ifacerefp->ifacep()->prettyNameQ()
                                                   << " new=" << newIfacep->prettyNameQ());
                ifacerefp->ifacep(newIfacep);
                ਵਾਪਸ ਸੱਚ;
            }
            ਜੇ (newIfacep) ਵਾਪਸ !isDead;  // Same interface - OK if live
            ਵਾਪਸ ਝੂਠ;
        }
        ਵਾਪਸ ਝੂਠ;
    }

    ਖਾਲੀ computeIfaceVarSyms() {
        ਲਈ (VSymEnt* varSymp : m_ifaceVarSyms) {
            AstVar* ਅਚਲ varp = varSymp ? VN_AS(varSymp->nodep(), Var) : ਖਾਲੀ_ਸੂਚਕ;
            UINFO(9, "  insAllIface se" << cvt_ਮੰਜ਼ਿਲ_Hex(varSymp) << " " << varp);
            AstIfaceRefDType* ਅਚਲ ifacerefp = ifaceRefFromArray(varp->subDTypep());
            UASSERT_OBJ(ifacerefp, varp, "Non-ifacerefs on list!");
            ਜੇ (!ifacerefp->cellp() && !ifacerefp->isVirtual() && (varp->pin_ਅੰਕ() == 0)) {
                varp->v3error("Interface port declaration "
                              << varp->prettyNameQ() << " doesn't have corresponding port\n"
                              << varp->warn_ਹੋਰ()
                                     + "... Perhaps intended an interface instantiation but "
                                       "are missing parenthesis (IEEE 1800-2023 25.3)?");
            }
            ਜੇ (!ifacerefp->iface_ਰਾਹੀਂ_Cellp()) {
                ਜੇ (!ifacerefp->cellp()) {  // Probably a NotFoundModule, or a normal module if
                                            // made mistake
                    UINFO(1, "Associated cell " << Ast_ਗੰਢ::prettyNameQ(ifacerefp->cellName()));
                    ifacerefp->v3error("Cannot find file containing interface: "
                                       << Ast_ਗੰਢ::prettyNameQ(ifacerefp->ifaceName()));
                    ਜਾਰੀ;
                } ਨਹੀਂਤਾਂ {
                    ifacerefp->v3fatalSrc("Unlinked interface");
                }
            } ਨਹੀਂਤਾਂ ਜੇ (varp->isIfaceRef() && !ifacerefp->cellp() && ifacerefp->iface_ਰਾਹੀਂ_Cellp()) {
                // Port variable (cellp=null) pointing to an interface.
                // The interface may be dead (template replaced by clone) or
                // live-but-wrong (template still alive because some instances
                // use default params, but this port should point to a
                // specialized clone based on the pin connection).
                ਅਚਲ ਤਾਰਕਿਕ isDead = ifacerefp->iface_ਰਾਹੀਂ_Cellp()->dead();
                ਜੇ (isDead && forPrimary() && !varp->is_Iface_ਮਾਪਾ()
                    && !v3Global.opt.ਸਿਖਰ_Ifaces_Supported()) {
                    varp->v3warn(E_UNSUPPORTED,
                                 "Unsupported: Interfaced port on top level module");
                }
                // Attempt repair: trace through parent cell's pin to find
                // the correct live interface for this port variable
                ਅਚਲ ਤਾਰਕਿਕ repaired = repairIfaceRef(varSymp, varp, ifacerefp, isDead);
                ਜੇ (!repaired && isDead) {
                    ifacerefp->v3error(
                        "Interface " << Ast_ਗੰਢ::prettyNameQ(ifacerefp->ifaceName())
                                     << " not connected as parent's interface not connected\n"
                                     << ifacerefp->warn_ਹੋਰ()
                                     << "... Perhaps caused by another error on the parent "
                                        "interface that needs resolving\n"
                                     << ifacerefp->warn_ਹੋਰ()
                                     << "... Or, perhaps intended an interface instantiation but "
                                        "are missing parenthesis (IEEE 1800-2023 25.3)?");
                    ਜਾਰੀ;
                }
            } ਨਹੀਂਤਾਂ ਜੇ (ifacerefp->iface_ਰਾਹੀਂ_Cellp()->dead()
                       || !exists_ਗੰਢ_Sym(ifacerefp->iface_ਰਾਹੀਂ_Cellp())) {
                ifacerefp->iface_ਰਾਹੀਂ_Cellp()->v3fatalSrc(
                    "Interface referenced by virtual iface is dead. "
                    "Dead interfaces have required linking steps skipped");
            }
            VSymEnt* ਅਚਲ ifaceSymp = get_ਗੰਢ_Sym(ifacerefp->iface_ਰਾਹੀਂ_Cellp());
            VSymEnt* if_Or_ਪੋਰਟ_Symp = ifaceSymp;
            // Link Modport names to the Modport Node under the Interface
            ਜੇ (ifacerefp->isModport()) {
                VSymEnt* ਅਚਲ foundp = ifaceSymp->ਲੱਭੋ_Id_Fallback(ifacerefp->modportName());
                ਤਾਰਕਿਕ ok = ਝੂਠ;
                ਜੇ (foundp) {
                    ਜੇ (AstModport* ਅਚਲ modportp = VN_CAST(foundp->nodep(), Modport)) {
                        UINFO(4, "Link Modport: " << modportp);
                        ifacerefp->modportp(modportp);
                        if_Or_ਪੋਰਟ_Symp = foundp;
                        ok = ਸੱਚ;
                    }
                }
                ਜੇ (!ok) {
                    ਅਚਲ string suggest = suggestSymFallback(ifaceSymp, ifacerefp->modportName(),
                                                              Link_ਗੰਢ_Matcher_Modport{});
                    ifacerefp->modportFileline()->v3error(
                        "Modport not found under interface "
                        << ifacerefp->prettyNameQ(ifacerefp->ifaceName()) << ": "
                        << ifacerefp->prettyNameQ(ifacerefp->modportName()) << '\n'
                        << (suggest.ਖਾਲੀ_ਥਾਵਾਂ() ? "" : ifacerefp->warn_ਹੋਰ() + suggest));
                }
            }
            // Alias won't expand until interfaces and modport names are known; see notes at top
            ਪਾਓ_Scope_Alias(SAMN_IFTOP, varSymp, if_Or_ਪੋਰਟ_Symp);
        }
        m_ifaceVarSyms.clear();
    }

    ਖਾਲੀ ਪਾਓ_Scope_Alias(SAMNum samn, VSymEnt* lhsp, VSymEnt* rhsp) {
        // Track and later insert scope aliases; an interface referenced by
        // a child cell connecting to that interface
        // Typically lhsp=VAR w/dtype IFACEREF, rhsp=IFACE cell
        UINFO(9, "   insertScopeAlias se" << cvt_ਮੰਜ਼ਿਲ_Hex(lhsp) << " se" << cvt_ਮੰਜ਼ਿਲ_Hex(rhsp));
        UASSERT_OBJ(
            !(VN_IS(rhsp->nodep(), Cell) && !VN_IS(VN_AS(rhsp->nodep(), Cell)->modp(), Iface)),
            rhsp->nodep(), "Got a non-IFACE alias RHS");
        m_scopeAliasMap[samn].emplace(lhsp, rhsp);
    }
    ਖਾਲੀ computeScopeAliases() {
        UINFO(9, "computeIfaceAliases");
        ਲਈ (ਪੂਰਨ samn = 0; samn < SAMN__MAX; ++samn) {
            ਲਈ (ScopeAliasMap::iterator it = m_scopeAliasMap[samn].begin();
                 it != m_scopeAliasMap[samn].ਸਿਰਾ(); ++it) {
                VSymEnt* ਅਚਲ lhsp = it->ਪਹਿਲਾ;
                VSymEnt* srcp = lhsp;
                ਜਦੋਂ (ਸੱਚ) {  // Follow chain of aliases up to highest level non-alias
                    ਅਚਲ ਸਵੈਚਾਲਿਤ it2 = m_scopeAliasMap[samn].ਲੱਭੋ(srcp);
                    ਜੇ (it2 != m_scopeAliasMap[samn].ਸਿਰਾ()) {
                        srcp = it2->ਦੂਜਾ;
                        ਜਾਰੀ;
                    } ਨਹੀਂਤਾਂ {
                        ਰੋਕੋ;
                    }
                }
                UINFO(9, "  iiasa: Insert alias se" << lhsp << " (" << lhsp->nodep()->typeName()
                                                    << ") <- se" << srcp << " " << srcp->nodep());
                // srcp should be an interface reference pointing to the interface we want to
                // import
                lhsp->importFromIface(symsp(), srcp);
                // Allow access to objects not permissible to be listed in a modport
                ਜੇ (VN_IS(srcp->nodep(), Modport)) {
                    lhsp->importFromIface(symsp(), srcp->parentp(), ਸੱਚ);
                }
            }
            // m_scopeAliasMap[samn].clear();  // Done with it, but put into debug file
        }
    }

ਨਿਜੀ:
    VSymEnt* ਲੱਭੋ_With_Alt_Fallback(VSymEnt* symp, ਅਚਲ string& name, ਅਚਲ string& altname) {
        VSymEnt* findp = symp->ਲੱਭੋ_Id_Fallback(name);
        ਜੇ (findp) ਵਾਪਸ findp;
        ਜੇ (altname != "") {
            UINFO(8, "     alt fallback");
            findp = symp->ਲੱਭੋ_Id_Fallback(altname);
        }
        ਵਾਪਸ findp;
    }

    VSymEnt* ਲੱਭੋ_With_Alt_Flat(VSymEnt* symp, ਅਚਲ string& name, ਅਚਲ string& altname) {
        VSymEnt* findp = symp->ਲੱਭੋ_Id_Flat(name);
        ਜੇ (findp) ਵਾਪਸ findp;
        ਜੇ (altname != "") {
            UINFO(8, "     alt flat");
            findp = symp->ਲੱਭੋ_Id_Flat(altname);
        }
        ਵਾਪਸ findp;
    }

ਜਨਤਕ:
    VSymEnt* ਲੱਭੋ_Dotted(File_ਸਤਰ* refLocationp, VSymEnt* lookupSymp, ਅਚਲ string& dotname,
                        string& baddot, VSymEnt*& okSymp, ਤਾਰਕਿਕ ਪਹਿਲਾ_Id) {
        // Given a dotted hierarchy name, return where in scope it is
        // Note when dotname=="" we just fall through and return lookupSymp
        UINFO(8, "    dottedFind se" << cvt_ਮੰਜ਼ਿਲ_Hex(lookupSymp) << " '" << dotname << "'");
        string leftname = dotname;
        okSymp = lookupSymp;  // So can list bad scopes
        ਜਦੋਂ (leftname != "") {  // foreach dotted part of xref name
            string::ਮਾਪ_type pos = leftname.ਲੱਭੋ('.');
            string ident;
            ਜੇ (pos != string::npos) {
                ident = leftname.substr(0, pos);
                leftname = leftname.substr(pos + 1);
            } ਨਹੀਂਤਾਂ {
                ident = leftname;
                leftname = "";
            }
            baddot = ident;  // So user can see where they botched it
            okSymp = lookupSymp;
            string altIdent;
            ਜੇ (!forScopeCreation()) {
                // GENFOR Begin is foo__BRA__##__KET__ after we've genloop unrolled,
                // but presently should be just "foo".
                // Likewise cell foo__[array] before we've expanded arrays is just foo.
                // Multi-dim iface arrays append multiple __BRA__..__KET__ suffixes; strip them
                // all.
                altIdent = ident;
                ਜਦੋਂ (VString::ਸਿਰੇ_With(altIdent, "__KET__")) {
                    ਅਚਲ ਸਵੈਚਾਲਿਤ braPos = altIdent.rfind("__BRA__");
                    ਜੇ (braPos == string::npos) ਰੋਕੋ;
                    altIdent = altIdent.substr(0, braPos);
                }
                ਜੇ (altIdent == ident) altIdent.clear();
            }
            UINFO(8, "         id " << ident << " alt " << altIdent << " left " << leftname
                                    << " at se" << lookupSymp);
            // Spec says; Look at existing module (cellnames then modname),
            // then look up (inst name or modname)
            ਜੇ (ਪਹਿਲਾ_Id) {
                // Check this module - subcellnames
                ਅਚਲ AstCell* cellp = lookupSymp ? VN_CAST(lookupSymp->nodep(), Cell)
                                                  : ਖਾਲੀ_ਸੂਚਕ;  // Replicated below
                ਜੇ (VSymEnt* ਅਚਲ ਲੱਭੋ_Symp = ਲੱਭੋ_With_Alt_Fallback(lookupSymp, ident, altIdent)) {
                    lookupSymp = ਲੱਭੋ_Symp;
                }

                // Check this module - cur modname
                ਨਹੀਂਤਾਂ ਜੇ (cellp && cellp->modp()->origName() == ident) {
                }
                // $root we walk up to Netlist
                ਨਹੀਂਤਾਂ ਜੇ (ident == "$root") {
                    lookupSymp = ਜੜ੍ਹ_Entp();
                    // We've added the '$root' module, now everything else is one lower
                    ਜੇ (forScopeCreation()) {
                        lookupSymp = lookupSymp->ਲੱਭੋ_Id_Flat(ident);
                        UASSERT(lookupSymp, "Cannot find $root module under netlist");
                    }
                }
                // Move up and check cellname + modname
                ਨਹੀਂਤਾਂ {
                    ਤਾਰਕਿਕ crossedCell = ਝੂਠ;  // Crossed a cell boundary
                    ਜਦੋਂ (lookupSymp) {
                        lookupSymp = lookupSymp->parentp();
                        cellp = lookupSymp ? VN_CAST(lookupSymp->nodep(), Cell)
                                           : ਖਾਲੀ_ਸੂਚਕ;  // Replicated above
                        ਜੇ (lookupSymp) {
                            UINFO(9, "    Up to " << lookupSymp);
                            ਜੇ (cellp || VN_IS(lookupSymp->nodep(), CellInline)) {
                                crossedCell = ਸੱਚ;
                            }
                            ਜੇ (cellp && cellp->modp()->origName() == ident) {
                                ਰੋਕੋ;
                            } ਨਹੀਂਤਾਂ ਜੇ (VSymEnt* ਅਚਲ ਲੱਭੋ_Sym2p
                                       = ਲੱਭੋ_With_Alt_Fallback(lookupSymp, ident, altIdent)) {
                                lookupSymp = ਲੱਭੋ_Sym2p;
                                ਜੇ (crossedCell && VN_IS(lookupSymp->nodep(), Var)) {
                                    UINFO(9, "    Not found but matches var name in parent "
                                                 << lookupSymp);
                                    ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
                                }
                                ਰੋਕੋ;
                            }
                        } ਨਹੀਂਤਾਂ {
                            ਰੋਕੋ;
                        }
                    }
                    ਜੇ (!lookupSymp) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;  // Not found
                }
            } ਨਹੀਂਤਾਂ {  // Searching for middle path component, must be a cell or interface port
                VSymEnt* ਲੱਭੋ_Symp = ਲੱਭੋ_With_Alt_Flat(lookupSymp, ident, altIdent);
                ਜੇ (ਲੱਭੋ_Symp) {
                    lookupSymp = ਲੱਭੋ_Symp;
                } ਨਹੀਂਤਾਂ {
                    // Try prefixed lookup for interface ports accessed through hierarchy
                    // (e.g., slave_inst.bus.data where bus is an interface port)
                    // After inlining, interface ports have prefixed names like
                    // top__DOT__slave_inst__DOT__bus
                    UINFO(8, "     middle-path: trying prefixed lookup for '" << ident << "'\n");
                    string baddot;
                    ਲੱਭੋ_Symp = ਲੱਭੋ_Sym_Prefixed(lookupSymp, ident, baddot, /*fallback=*/ਸੱਚ);
                    UINFO(8, "     middle-path: prefixed lookup result: "
                                 << (ਲੱਭੋ_Symp ? "found" : "not found") << "\n");
                    ਜੇ (ਲੱਭੋ_Symp) {
                        // Check if this is an interface reference variable
                        ਅਚਲ AstVar* varp = VN_CAST(ਲੱਭੋ_Symp->nodep(), Var);
                        ਜੇ (!varp) {
                            ਜੇ (ਅਚਲ AstVarScope* vscp = VN_CAST(ਲੱਭੋ_Symp->nodep(), VarScope)) {
                                varp = vscp->varp();
                            }
                        }
                        ਜੇ (varp && varp->isIfaceRef()) {
                            // Found an interface port - use findSymp directly
                            // computeScopeAliases has already imported interface members
                            // into this symbol entry, so we use it instead of redirecting
                            // to the shared modport definition (which would cause all
                            // instances to resolve to the same symbols - bug #2656)
                            lookupSymp = ਲੱਭੋ_Symp;
                        } ਨਹੀਂਤਾਂ {
                            // Non-interface symbol in middle path must be a cell (module instance)
                            // to continue hierarchical resolution
                            ਜੇ (VN_IS(ਲੱਭੋ_Symp->nodep(), Cell)) {
                                lookupSymp = ਲੱਭੋ_Symp;
                            } ਨਹੀਂਤਾਂ {
                                // Reject non-cell, non-interface symbols found via fallback
                                // to prevent accidental resolution to unrelated symbols
                                UINFO(8, "     middle-path: rejecting non-cell symbol\n");
                                ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
                            }
                        }
                    } ਨਹੀਂਤਾਂ {
                        ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;  // Not found
                    }
                }
            }
            ਜੇ (lookupSymp) {
                ਜੇ (ਅਚਲ AstCell* ਅਚਲ cellp = VN_CAST(lookupSymp->nodep(), Cell)) {
                    ਜੇ (ਅਚਲ Ast_ਗੰਢ_Module* ਅਚਲ modp = cellp->modp()) {
                        ਜੇ (modp->hier_ਖੰਡ() && !leftname.ਖਾਲੀ_ਥਾਵਾਂ()) {
                            refLocationp->v3error("Cannot access scope inside hierarchical block");
                        } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(modp, Not_ਮਿਲਿਆ_Module)) {
                            refLocationp->v3error("Dotted reference to instance that refers to "
                                                  "missing module/interface: "
                                                  << modp->prettyNameQ());
                        }
                    }
                }
                // Follow scope alias for nested interface port access
                ਜੇ (!leftname.ਖਾਲੀ_ਥਾਵਾਂ()) {
                    ਅਚਲ ਸਵੈਚਾਲਿਤ aliasIt = m_scopeAliasMap[SAMN_IFTOP].ਲੱਭੋ(lookupSymp);
                    ਜੇ (aliasIt != m_scopeAliasMap[SAMN_IFTOP].ਸਿਰਾ()) {
                        lookupSymp = aliasIt->ਦੂਜਾ;
                        // Alias may point to __Viftop VarScope; find corresponding Cell
                        ਜੇ (ਅਚਲ AstVarScope* ਅਚਲ vscp
                            = VN_CAST(lookupSymp->nodep(), VarScope)) {
                            ਅਚਲ string varName = vscp->varp()->name();
                            ਸਥਿਰ constexpr ਅਚਲ ਅੱਖਰ* ਅਚਲ VIFTOP_SUFFIX = "__Viftop";
                            ਜੇ (VString::ਸਿਰੇ_With(varName, VIFTOP_SUFFIX)) {
                                ਅਚਲ string cellName
                                    = varName.substr(0, varName.ਮਾਪ() - ਲੜੀ_ਲੰਬਾਈ(VIFTOP_SUFFIX));
                                VSymEnt* ਅਚਲ ਮਾਪਾ_Symp = lookupSymp->parentp();
                                ਜੇ (ਮਾਪਾ_Symp) {
                                    VSymEnt* ਅਚਲ cellSymp = ਮਾਪਾ_Symp->ਲੱਭੋ_Id_Flat(cellName);
                                    ਜੇ (cellSymp && VN_IS(cellSymp->nodep(), Cell)) {
                                        lookupSymp = cellSymp;  // Use Cell for member lookup
                                    }
                                }
                            }
                        }
                    } ਨਹੀਂਤਾਂ {
                        // No alias; try following IfaceRefDType to interface cell
                        ਅਚਲ AstVar* varp = VN_CAST(lookupSymp->nodep(), Var);
                        ਜੇ (!varp) {
                            ਜੇ (ਅਚਲ AstVarScope* ਅਚਲ vscp
                                = VN_CAST(lookupSymp->nodep(), VarScope)) {
                                varp = vscp->varp();
                            }
                        }
                        ਜੇ (varp && varp->isIfaceRef()) {
                            ਜੇ (ਅਚਲ AstIfaceRefDType* ਅਚਲ ifaceRefp
                                = ifaceRefFromArray(varp->dtypep())) {
                                ਜੇ (ifaceRefp->cellp() && exists_ਗੰਢ_Sym(ifaceRefp->cellp())) {
                                    lookupSymp = get_ਗੰਢ_Sym(ifaceRefp->cellp());
                                } ਨਹੀਂਤਾਂ ਜੇ (ifaceRefp->iface_ਰਾਹੀਂ_Cellp()
                                           && exists_ਗੰਢ_Sym(ifaceRefp->iface_ਰਾਹੀਂ_Cellp())) {
                                    lookupSymp = get_ਗੰਢ_Sym(ifaceRefp->iface_ਰਾਹੀਂ_Cellp());
                                } ਨਹੀਂਤਾਂ ਜੇ (ifaceRefp->ifacep()
                                           && exists_ਗੰਢ_Sym(ifaceRefp->ifacep())) {
                                    lookupSymp = get_ਗੰਢ_Sym(ifaceRefp->ifacep());
                                }
                            }
                        }
                    }
                }
            }
            ਪਹਿਲਾ_Id = ਝੂਠ;
        }
        ਵਾਪਸ lookupSymp;
    }

    ਸਥਿਰ string remove_ਆਖਰੀ_Inline_Scope(ਅਚਲ string& name) {
        string ਬਾਹਰਲਾ = name;
        ਅਚਲ string dot = "__DOT__";
        ਅਚਲ ਮਾਪ_t dotPos = ਬਾਹਰਲਾ.rfind(dot, ਬਾਹਰਲਾ.ਮਾਪ() - dot.ਲੰਮਾਈ() - 1);
        ਜੇ (dotPos == string::npos) {
            ਵਾਪਸ "";
        } ਨਹੀਂਤਾਂ {
            ਵਾਪਸ ਬਾਹਰਲਾ.erase(dotPos + dot.ਲੰਮਾਈ(), string::npos);
        }
    }

    VSymEnt* ਲੱਭੋ_Sym_Prefixed(VSymEnt* lookupSymp, ਅਚਲ string& dotname, string& baddot,
                             ਤਾਰਕਿਕ fallback) {
        // Find symbol in given point in hierarchy, allowing prefix (post-Inline)
        // For simplicity lookupSymp may be passed nullptr result from findDotted
        ਜੇ (!lookupSymp) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        UINFO(8, "    findSymPrefixed "
                     << dotname << " under se" << cvt_ਮੰਜ਼ਿਲ_Hex(lookupSymp)
                     << ((lookupSymp->sym_ਅਗੇਤਰ() == "") ? "" : " as ")
                     << ((lookupSymp->sym_ਅਗੇਤਰ() == "") ? "" : lookupSymp->sym_ਅਗੇਤਰ() + dotname)
                     << "  at se" << lookupSymp << " fallback=" << fallback);
        string ਅਗੇਤਰ = lookupSymp->sym_ਅਗੇਤਰ();
        VSymEnt* foundp = ਖਾਲੀ_ਸੂਚਕ;
        ਜਦੋਂ (!foundp) {
            ਜੇ (fallback) {
                foundp = lookupSymp->ਲੱਭੋ_Id_Fallback(ਅਗੇਤਰ + dotname);  // Might be nullptr
            } ਨਹੀਂਤਾਂ {
                foundp = lookupSymp->ਲੱਭੋ_Id_Flat(ਅਗੇਤਰ + dotname);  // Might be nullptr
            }
            ਜੇ (ਅਗੇਤਰ.ਖਾਲੀ_ਥਾਵਾਂ()) ਰੋਕੋ;
            ਅਚਲ string ਅਗਲਾ_ਅਗੇਤਰ = remove_ਆਖਰੀ_Inline_Scope(ਅਗੇਤਰ);
            ਜੇ (ਅਗੇਤਰ == ਅਗਲਾ_ਅਗੇਤਰ) ਰੋਕੋ;
            ਅਗੇਤਰ = ਅਗਲਾ_ਅਗੇਤਰ;
        }
        ਜੇ (!foundp) baddot = dotname;
        ਵਾਪਸ foundp;
    }
    ਸਥਿਰ ਤਾਰਕਿਕ checkIfClassOrPackage(ਅਚਲ VSymEnt* ਅਚਲ symp) {
        ਜੇ (VN_IS(symp->nodep(), Class) || VN_IS(symp->nodep(), Package)) ਵਾਪਸ ਸੱਚ;

        // Helper: check if a RefDType might resolve to a class later.
        ਅਚਲ ਸਵੈਚਾਲਿਤ isPotentialClassRef = [](ਅਚਲ AstRefDType* refp) -> ਤਾਰਕਿਕ {
            ਵਾਪਸ refp && !refp->typeofp()
                   && (!refp->classOrPackageOpp()
                       || VN_IS(refp->classOrPackageOpp(), ClassOrPackageRef));
        };

        // Helper: allow types that can represent a class/package handle or an unresolved ref.
        // BasicDType is allowed here only for null/object handles.
        ਅਚਲ ਸਵੈਚਾਲਿਤ is_Valid_Type_ਗੰਢ = [](ਅਚਲ Ast_ਗੰਢ* nodep) -> ਤਾਰਕਿਕ {
            ਵਾਪਸ VN_IS(nodep, VoidDType) || VN_IS(nodep, BasicDType)
                   || VN_IS(nodep, ClassRefDType) || VN_IS(nodep, ParseRef);
        };

        ਜੇ (ਅਚਲ AstTypedef* ਅਚਲ typedefp = VN_CAST(symp->nodep(), Typedef)) {
            ਅਚਲ Ast_ਗੰਢ_DType* dtypep = typedefp->subDTypep();
            ਜੇ (!dtypep) dtypep = typedefp->ਬੱਚਾ_DTypep();
            ਜੇ (VN_IS(dtypep, ClassRefDType)) ਵਾਪਸ ਸੱਚ;
            ਜੇ (isPotentialClassRef(VN_CAST(dtypep, RefDType))) ਵਾਪਸ ਸੱਚ;
        } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ AstParamTypeDType* ਅਚਲ paramTypep
                   = VN_CAST(symp->nodep(), ParamTypeDType)) {
            // Before V3Param the declared default is in childDTypep (possibly
            // wrapped in a RequireDType); after V3Param it is consumed and the
            // bound type is the resolved data type, e.g. a type parameter
            // inherited from a specialized base class (REQ #(Item) -> class Item).
            Ast_ਗੰਢ* childp = paramTypep->ਬੱਚਾ_DTypep();
            ਜੇ (ਅਚਲ AstRequireDType* ਅਚਲ reqp = VN_CAST(childp, RequireDType)) {
                childp = reqp->lhsp();
            }
            ਅਚਲ Ast_ਗੰਢ* ਅਚਲ checkp = childp ? childp : paramTypep->skipRefp();
            ਜੇ (is_Valid_Type_ਗੰਢ(checkp) || isPotentialClassRef(VN_CAST(checkp, RefDType))) {
                ਵਾਪਸ ਸੱਚ;
            }
        }
        ਵਾਪਸ ਝੂਠ;
    }
    VSymEnt* resolveClassOrPackage(VSymEnt* lookSymp, AstClassOrPackageRef* nodep, ਤਾਰਕਿਕ fallback,
                                   ਤਾਰਕਿਕ classOnly, ਅਚਲ string& forWhat,
                                   ਤਾਰਕਿਕ deferIfUnresolved = ਝੂਠ) {
        ਜੇ (nodep->classOrPackageSkipp()) ਵਾਪਸ get_ਗੰਢ_Sym(nodep->classOrPackageSkipp());
        VSymEnt* foundp;
        VSymEnt* ਖੋਜ_Symp = lookSymp;
        ਜੇ (VL_UNCOVERABLE(ਖੋਜ_Symp && VN_IS(ਖੋਜ_Symp->nodep(), ParamTypeDType))) {
            ਖੋਜ_Symp->nodep()->v3fatalSrc(  // LCOV_EXCL_LINE
                "resolveClassOrPackage: unexpected ParamTypeDType lookup");
        }
        ਜੇ (fallback) {
            VSymEnt* currentLookSymp = ਖੋਜ_Symp;
            ਕਰੋ {
                foundp = currentLookSymp->ਲੱਭੋ_Id_Flat(nodep->name());
                ਜੇ (foundp && !checkIfClassOrPackage(foundp)) foundp = ਖਾਲੀ_ਸੂਚਕ;
                ਜੇ (!foundp) currentLookSymp = currentLookSymp->fallbackp();
            } ਜਦੋਂ (!foundp && currentLookSymp);
        } ਨਹੀਂਤਾਂ {
            foundp = ਖੋਜ_Symp->ਲੱਭੋ_Id_Flat(nodep->name());
            ਜੇ (foundp && !checkIfClassOrPackage(foundp)) foundp = ਖਾਲੀ_ਸੂਚਕ;
        }
        ਜੇ (!foundp && v3Global.rootp()->stdPackagep()) {  // Look under implied std::
            foundp = get_ਗੰਢ_Sym(v3Global.rootp()->stdPackagep())->ਲੱਭੋ_Id_Flat(nodep->name());
        }
        ਜੇ (foundp) {
            nodep->classOrPackageNodep(foundp->nodep());
            ਵਾਪਸ foundp;
        }
        ਜੇ (deferIfUnresolved) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        ਅਚਲ string suggest
            = suggestSymFallback(lookSymp, nodep->name(), Link_ਗੰਢ_Matcher_Class_Or_Package{});
        nodep->v3error((classOnly ? "Class" : "Package/class")
                       << " for '" << forWhat  // extends/implements
                       << "' not found: " << nodep->prettyNameQ() << '\n'
                       << (suggest.ਖਾਲੀ_ਥਾਵਾਂ() ? "" : nodep->warn_ਹੋਰ() + suggest));
        ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
    }
    string suggestSymFallback(VSymEnt* lookupSymp, ਅਚਲ string& name,
                              ਅਚਲ VNodeMatcher& matcher) {
        // Suggest alternative symbol in given point in hierarchy
        // Does not support inline, as we find user-level errors before inlining
        // For simplicity lookupSymp may be passed nullptr result from findDotted
        ਜੇ (!lookupSymp) ਵਾਪਸ "";
        VSpellCheck speller;
        lookupSymp->candidateIdFallback(&speller, &matcher);
        ਵਾਪਸ speller.ਉੱਤਮ_Candidate_ਸੁਨੇਹਾ(name);
    }
    string suggestSymFlat(VSymEnt* lookupSymp, ਅਚਲ string& name, ਅਚਲ VNodeMatcher& matcher,
                          ਤਾਰਕਿਕ includeType) {
        ਜੇ (!lookupSymp) ਵਾਪਸ "";
        VSpellCheck speller;
        lookupSymp->candidateIdFlat(&speller, &matcher, includeType);
        ਵਾਪਸ speller.ਉੱਤਮ_Candidate_ਸੁਨੇਹਾ(name);
    }
};

LinkDotState* LinkDotState::s_errorThisp = ਖਾਲੀ_ਸੂਚਕ;

//======================================================================

ਵਰਗ Link_Dot_ਲੱਭੋ_Visitor final : ਜਨਤਕ VNVisitor {
    // NODE STATE
    // Cleared on global
    //  *::user1p()             -> See LinkDotState
    //  *::user2p()             -> See LinkDotState
    //  *::user3()              // bool. Node stored in m_virtualIfaces
    //  *::user4()              -> See LinkDotState
    ਅਚਲ VNUser3_ਵਿੱਚ_Use m_inuser3;

    // STATE
    LinkDotState* ਅਚਲ m_statep;  // State to pass between visitors, including symbol table
    Ast_ਗੰਢ_Module* m_classOrPackagep = ਖਾਲੀ_ਸੂਚਕ;  // Current package
    AstClocking* m_clockingp = ਖਾਲੀ_ਸੂਚਕ;  // Current clocking block
    std::ਸਦਿਸ਼<AstScope*> m_scopes;  // Scopes detached during repeated module visits
    uint64_t m_statScopeVisits = 0;  // Non-top scopes encountered while finding instance symbols
    VSymEnt* m_modSymp = ਖਾਲੀ_ਸੂਚਕ;  // Symbol Entry for current module
    VSymEnt* m_curSymp = ਖਾਲੀ_ਸੂਚਕ;  // Symbol Entry for current table, where to lookup/insert
    string
        m_hierParamsName;  // Name of module with hierarchical type parameters, empty when not used
    string m_scope;  // Scope text
    ਅਚਲ Ast_ਗੰਢ_FTask* m_ftaskp = ਖਾਲੀ_ਸੂਚਕ;  // Current function/task
    ਤਾਰਕਿਕ m_ਵਿੱਚ_Recursion = ਝੂਠ;  // Inside a recursive module
    ਪੂਰਨ m_param_ਅੰਕ = 0;  // Parameter number, for position based connection
    ਤਾਰਕਿਕ m_explicitNew = ਝੂਠ;  // Hit a "new" function
    ਪੂਰਨ m_mod_ਖੰਡ_ਅੰਕ = 0;  // Begin block number in module, 0=none seen
    ਪੂਰਨ m_mod_With_ਅੰਕ = 0;  // With block number, 0=none seen
    ਪੂਰਨ m_mod_Arg_ਅੰਕ = 0;  // Arg block number for randomize(), 0=none seen
    std::ਸਦਿਸ਼<AstIface*> m_virtIfaces;  // interfaces used as virtual,
                                          // needed for handleUnvisitedVirtIfaces()

    // METHODS
    ਖਾਲੀ makeImplicitNew(AstClass* nodep) {
        AstFunc* ਅਚਲ newp = ਨਵਾਂ AstFunc{nodep->fileline(), "new", ਖਾਲੀ_ਸੂਚਕ, ਖਾਲੀ_ਸੂਚਕ};
        // If needed, super.new() call is added the 2nd pass of V3LinkDotResolve,
        // because base classes are already resolved there.
        newp->isConstructor(ਸੱਚ);
        nodep->ਜੋੜੋ_Membersp(newp);
        UINFO(8, "Made implicit new for " << nodep->name() << ": " << nodep);
        m_statep->ਪਾਓ_ਖੰਡ(m_curSymp, newp->name(), newp, m_classOrPackagep);
    }

    ਸਥਿਰ ਤਾਰਕਿਕ is_Hier_ਖੰਡ_Wrapper(ਅਚਲ string& name) {
        ਅਚਲ V3_Hier_ਖੰਡ_Opt_ਸੈੱਟ& hier_ਖੰਡਾਂ = v3Global.opt.hier_ਖੰਡਾਂ();
        ਵਾਪਸ hier_ਖੰਡਾਂ.ਲੱਭੋ(name) != hier_ਖੰਡਾਂ.ਸਿਰਾ();
    }

    ਖਾਲੀ moveExternFuncDecl(Ast_ਗੰਢ_FTask* nodep, AstClass* classp) {
        // Move an 'extern function|task' to inside a class or, from an outer to inner-class
        ਜੇ (nodep->isExternDef()) ਵਾਪਸ;
        // Move it to proper spot under the target class
        nodep->unlink_Fr_ਪਿੱਠ();
        classp->ਜੋੜੋ_Stmtsp(nodep);
        nodep->isExternDef(ਸੱਚ);  // So we check there's a matching extern
        nodep->classOrPackagep()->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ();
        // Any "Type::" reference in the function's IO are really "MovedToClass::" references
        ਜੇ (nodep->fvarp()) moveExternFuncDeclRefs(nodep->fvarp(), classp);
        ਲਈ (Ast_ਗੰਢ* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ਜੇ (AstVar* ਅਚਲ portp = VN_CAST(stmtp, Var)) {
                ਜੇ (portp->isIO()) moveExternFuncDeclRefs(portp, classp);
            }
        }
    }
    ਖਾਲੀ moveExternFuncDeclRefs(Ast_ਗੰਢ* nodep, AstClass* classp) {
        nodep->foreach([ਇਹ, classp](AstClassOrPackageRef* refp) {  //
            ਜੇ (refp->name() == classp->name() && !refp->paramsp()) {
                UINFO(9, "Cleaning up external function type for class " << refp);
                AstDot* ਅਚਲ dotp = VN_CAST(refp->backp(), Dot);
                ਜੇ (dotp && dotp->lhsp() == refp) {
                    // If had DOT(CLASSREF this, ...) the DOT can go away
                    dotp->replaceWith(dotp->rhsp()->unlink_Fr_ਪਿੱਠ());
                    VL_DO_DANGLING2(ਧੱਕੋ_Deletep(dotp), dotp, refp);
                } ਨਹੀਂਤਾਂ {
                    VL_DO_DANGLING(ਧੱਕੋ_Deletep(refp->unlink_Fr_ਪਿੱਠ()), refp);
                }
            }
        });
    }

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(AstNetlist* nodep) override {  // FindVisitor::
        // Process $unit or other packages
        // Not needed - dotted references not allowed from inside packages
        // for (AstNodeModule* nodep = v3Global.rootp()->modulesp();
        //     nodep; nodep=VN_AS(nodep->nextp(), NodeModule)) {
        //    if (VN_IS(nodep, Package)) {}}

        m_statep->ਪਾਓ_DUnit(nodep);

        // First back iterate, to find all packages. Backward as must do base
        // packages before using packages
        iterateChildrenBackwardsConst(nodep);

        // The first modules in the list are always the top modules
        // (sorted before this is called).
        // This may not be the module with isTop() set, as early in the steps,
        // wrapTop may have not been created yet.
        // $unit and the constant pool always exist, so nothing else, and nothing in $unit,
        // means nothing was given
        Ast_ਗੰਢ_Module* ਅਚਲ modulesp = nodep->modulesp();
        UASSERT_OBJ(modulesp, nodep, "$unit should always be in the netlist");
        ਜੇ (m_statep->forPrimary()) {
            ਅਚਲ ਤਾਰਕਿਕ onlyBuiltins = [&]() {
                ਲਈ (Ast_ਗੰਢ_Module* modp = modulesp; modp;
                     modp = VN_AS(modp->nextp(), ਗੰਢ_Module)) {
                    ਜੇ (!modp->isDollarUnit() && !modp->isConstPool()) ਵਾਪਸ ਝੂਠ;
                }
                ਵਾਪਸ ਸੱਚ;
            }();
            ਜੇ (onlyBuiltins && !v3Global.rootp()->dollarUnitPkgp()->stmtsp()) {
                nodep->v3error("No top level module found");
            }
        }
        ਲਈ (Ast_ਗੰਢ_Module* modp = nodep->modulesp(); modp && modp->is_ਸਿਖਰ();
             modp = VN_AS(modp->nextp(), ਗੰਢ_Module)) {
            UINFO(8, "Top Module: " << modp);
            m_scope = "TOP";

            ਜੇ (!m_statep->forScopeCreation() && v3Global.opt.ਸਿਖਰ_Ifaces_Supported()) {
                ਲਈ (Ast_ਗੰਢ* subnodep = modp->stmtsp(); subnodep; subnodep = subnodep->nextp()) {
                    ਜੇ (AstVar* ਅਚਲ varp = VN_CAST(subnodep, Var)) {
                        ਜੇ (varp->isIfaceRef()) {
                            ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ subtypep = varp->subDTypep();
                            ਅਚਲ AstIfaceRefDType* ifacerefp = ਖਾਲੀ_ਸੂਚਕ;
                            ਜੇ (VN_IS(subtypep, IfaceRefDType)) {
                                ifacerefp = VN_AS(varp->subDTypep(), IfaceRefDType);
                            } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(subtypep, BracketArrayDType)) {
                                ਅਚਲ AstBracketArrayDType* ਅਚਲ arrp
                                    = VN_AS(subtypep, BracketArrayDType);
                                ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ arrsubtypep = arrp->subDTypep();
                                ਜੇ (VN_IS(arrsubtypep, IfaceRefDType)) {
                                    ifacerefp = VN_AS(arrsubtypep, IfaceRefDType);
                                }
                            } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(subtypep, UnpackArrayDType)) {
                                ਅਚਲ AstUnpackArrayDType* ਅਚਲ arrp
                                    = VN_AS(subtypep, UnpackArrayDType);
                                ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ arrsubtypep = arrp->subDTypep();
                                ਜੇ (VN_IS(arrsubtypep, IfaceRefDType)) {
                                    ifacerefp = VN_AS(arrsubtypep, IfaceRefDType);
                                }
                            }

                            ਜੇ (ifacerefp && !ifacerefp->cellp()) {
                                // A dummy cell to keep the top level interface alive and correctly
                                // optimized for default parameter values
                                AstCell* ifacecellp
                                    = ਨਵਾਂ AstCell{nodep->fileline(),
                                                  nodep->fileline(),
                                                  modp->name() + "__02E" + varp->name(),
                                                  ifacerefp->ifaceName(),
                                                  ਖਾਲੀ_ਸੂਚਕ,
                                                  ਖਾਲੀ_ਸੂਚਕ,
                                                  ਖਾਲੀ_ਸੂਚਕ};
                                ifacecellp->modp(ifacerefp->ifacep());
                                m_curSymp = m_modSymp
                                    = m_statep->ਪਾਓ_ਸਿਖਰ_Iface(ifacecellp, m_scope);
                                iterate(ifacecellp);
                                VL_DO_DANGLING(ਧੱਕੋ_Deletep(ifacecellp), ifacecellp);
                            }
                        }
                    }
                }
            }

            m_curSymp = m_modSymp = m_statep->ਪਾਓ_ਸਿਖਰ_Cell(modp, m_scope);
            { iterate(modp); }

            m_scope = "";
            m_curSymp = m_modSymp = ਖਾਲੀ_ਸੂਚਕ;
        }
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Type_ਸਾਰਣੀ*) override {}  // FindVisitor::
    ਖਾਲੀ ਦੇਖੋ(AstIfaceRefDType* nodep) override {  // FindVisitor::
        ਜੇ (!m_statep->forScopeCreation() && nodep->isVirtual() && nodep->ifacep()
            && !nodep->ifacep()->user3()) {
            m_virtIfaces.ਧੱਕੋ_ਪਿੱਠ(nodep->ifacep());
            nodep->ifacep()->user3(ਸੱਚ);
        }
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Module* nodep) override {  // FindVisitor::
        // Called on top module from Netlist, other modules from the cell creating them,
        // and packages
        UINFO(8, "   " << nodep);
        // m_curSymp/m_modSymp maybe nullptr for packages and non-top modules
        // Packages will be under top after the initial phases, but until then
        // need separate handling
        ਅਚਲ ਤਾਰਕਿਕ standalonePkg
            = !m_modSymp && !m_statep->forScopeCreation() && VN_IS(nodep, Package);
        ਅਚਲ ਤਾਰਕਿਕ doit = (m_modSymp || standalonePkg);
        VL_RESTORER_COPY(m_scope);
        VL_RESTORER(m_classOrPackagep);
        VL_RESTORER(m_modSymp);
        VL_RESTORER(m_curSymp);
        VL_RESTORER(m_param_ਅੰਕ);
        VL_RESTORER(m_mod_ਖੰਡ_ਅੰਕ);
        VL_RESTORER(m_mod_With_ਅੰਕ);
        VL_RESTORER(m_mod_Arg_ਅੰਕ);
        ਜੇ (doit && nodep->user2()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Identically recursive module (module instantiates "
                          "itself, without changing parameters): "
                              << Ast_ਗੰਢ::prettyNameQ(nodep->origName()));
        } ਨਹੀਂਤਾਂ ਜੇ (doit) {
            UINFO(4, "     Link Module: " << nodep);
            UASSERT_OBJ(!nodep->dead(), nodep, "Module in instance tree mislabeled as dead?");
            AstPackage* ਅਚਲ pkgp = VN_CAST(nodep, Package);
            m_classOrPackagep = pkgp;
            ਜੇ (standalonePkg) {
                ਜੇ (pkgp->isDollarUnit()) {
                    m_curSymp = m_modSymp = m_statep->dunitEntp();
                    nodep->user1p(m_curSymp);
                } ਨਹੀਂਤਾਂ {
                    VSymEnt* ਅਚਲ ਉੱਤਲਾ_Symp = m_statep->dunitEntp();
                    m_scope = nodep->name();
                    m_curSymp = m_modSymp = m_statep->ਪਾਓ_ਖੰਡ(ਉੱਤਲਾ_Symp, nodep->name(), nodep,
                                                                  m_classOrPackagep);
                    UINFO(9, "New module scope " << m_curSymp);
                }
            }
            //
            m_param_ਅੰਕ = 0;
            m_mod_ਖੰਡ_ਅੰਕ = 0;
            m_mod_With_ਅੰਕ = 0;
            m_mod_Arg_ਅੰਕ = 0;
            // m_modSymp/m_curSymp for non-packages set by AstCell above this module
            // Iterate
            nodep->user2(ਸੱਚ);
            iterateChildren(nodep);
            nodep->user2(ਝੂਠ);
            nodep->user4(ਸੱਚ);
            // Interfaces need another pass when signals are resolved. When creating
            // scopes this is done from the AstScopes in LinkDotScopeVisitor instead, as
            // only those entries hold the AstVarScopes the modport items resolve against.
            ਜੇ (!m_statep->forScopeCreation()) {
                ਜੇ (AstIface* ਅਚਲ ifacep = VN_CAST(nodep, Iface)) {
                    m_statep->ਪਾਓ_Iface_Mod_Sym(ifacep, m_curSymp);
                }
            }
        } ਨਹੀਂਤਾਂ ਜੇ (is_Hier_ਖੰਡ_Wrapper(nodep->name())) {
            UINFO(5, "Module is hierarchical block, must not be dead: " << nodep);
            m_scope = nodep->name();
            VSymEnt* ਅਚਲ ਉੱਤਲਾ_Symp = m_curSymp ? m_curSymp : m_statep->ਜੜ੍ਹ_Entp();
            m_curSymp = m_modSymp
                = m_statep->ਪਾਓ_ਖੰਡ(ਉੱਤਲਾ_Symp, nodep->name() + "::", nodep, m_classOrPackagep);
            iterateChildren(nodep);
            nodep->user4(ਸੱਚ);
        } ਨਹੀਂਤਾਂ {  // !doit
            ਜੇ (nodep->hierParams()) {
                UINFO(1, "Found module with hier type parameters");
                m_hierParamsName = nodep->name();
                ਲਈ (ਅਚਲ Ast_ਗੰਢ* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    ਜੇ (ਅਚਲ AstTypedef* ਅਚਲ tdef = VN_CAST(stmtp, Typedef)) {
                        UINFO(1, "Inserting hier type parameter typedef: " << tdef);
                        VSymEnt* ਅਚਲ ਉੱਤਲਾ_Symp = m_curSymp ? m_curSymp : m_statep->ਜੜ੍ਹ_Entp();
                        m_curSymp = m_modSymp = m_statep->ਪਾਓ_ਖੰਡ(ਉੱਤਲਾ_Symp, nodep->name(),
                                                                      nodep, m_classOrPackagep);
                    }
                }
            } ਨਹੀਂਤਾਂ {
                // Will be optimized away later
                // Can't remove now, as our backwards iterator will throw up
                UINFO(5, "Module not under any CELL or top - dead module: " << nodep);
            }
        }
    }

    ਖਾਲੀ ਦੇਖੋ(AstClass* nodep) override {  // FindVisitor::
        UASSERT_OBJ(m_curSymp, nodep, "Class not under module/package/$unit");
        UINFO(8, "   " << nodep);
        VL_RESTORER_COPY(m_scope);
        VL_RESTORER(m_classOrPackagep);
        VL_RESTORER(m_modSymp);
        VL_RESTORER(m_curSymp);
        VL_RESTORER(m_param_ਅੰਕ);
        VL_RESTORER(m_mod_ਖੰਡ_ਅੰਕ);
        VL_RESTORER(m_mod_With_ਅੰਕ);
        VL_RESTORER(m_mod_Arg_ਅੰਕ);
        VL_RESTORER(m_explicitNew);
        {
            UINFO(4, "     Link Class: " << nodep);
            VSymEnt* ਅਚਲ ਉੱਤਲਾ_Symp = m_curSymp;
            m_scope = m_scope + "." + nodep->name();
            m_classOrPackagep = nodep;
            m_curSymp = m_modSymp
                = m_statep->ਪਾਓ_ਖੰਡ(ਉੱਤਲਾ_Symp, nodep->name(), nodep, m_classOrPackagep);
            m_statep->ਪਾਓ_Map(m_curSymp, m_scope);
            UINFO(9, "New module scope " << m_curSymp);
            //
            m_param_ਅੰਕ = 0;
            m_mod_ਖੰਡ_ਅੰਕ = 0;
            m_mod_With_ਅੰਕ = 0;
            m_mod_Arg_ਅੰਕ = 0;
            m_explicitNew = ਝੂਠ;
            // m_modSymp/m_curSymp for non-packages set by AstCell above this module
            // Iterate
            iterateChildren(nodep);
            nodep->user4(ਸੱਚ);
            // Implicit new needed?
            ਜੇ (!m_explicitNew && m_statep->forPrimary()) makeImplicitNew(nodep);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstClassOrPackageRef* nodep) override {  // FindVisitor::
        ਜੇ (!nodep->classOrPackageNodep() && nodep->name() == "$unit") {
            nodep->classOrPackageNodep(v3Global.rootp()->dollarUnitPkgp());
        }
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstScope* nodep) override {  // FindVisitor::
        UASSERT_OBJ(m_statep->forScopeCreation(), nodep,
                    "Scopes should only exist right after V3Scope");
        // Detach on first encounter, will re add at the end.
        // Otherwise would be visited O(cells^2) time after V3Scope
        ਜੇ (!nodep->is_ਸਿਖਰ()) {
            ++m_statScopeVisits;
            nodep->unlink_Fr_ਪਿੱਠ();
            m_scopes.ਧੱਕੋ_ਪਿੱਠ(nodep);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstCell* nodep) override {  // FindVisitor::
        UINFO(5, "   CELL under " << m_scope << " is " << nodep);
        // Process XREFs/etc inside pins
        ਜੇ (nodep->recursive() && m_ਵਿੱਚ_Recursion) ਵਾਪਸ;
        iterateChildren(nodep);
        // Recurse in, preserving state
        VL_RESTORER_COPY(m_scope);
        VL_RESTORER(m_modSymp);
        VL_RESTORER(m_curSymp);
        VL_RESTORER(m_param_ਅੰਕ);
        VL_RESTORER(m_ਵਿੱਚ_Recursion);
        // Where do we add it?
        VSymEnt* aboveSymp = m_curSymp;
        ਅਚਲ string origname = Ast_ਗੰਢ::dedotName(nodep->name());
        string::ਮਾਪ_type pos = origname.rfind('.');
        ਜੇ (pos != string::npos) {
            // Flattened, find what CellInline it should live under
            ਅਚਲ string scope = origname.substr(0, pos);
            string baddot;
            VSymEnt* okSymp;
            aboveSymp
                = m_statep->ਲੱਭੋ_Dotted(nodep->fileline(), aboveSymp, scope, baddot, okSymp, ਝੂਠ);
            UASSERT_OBJ(aboveSymp, nodep,
                        "Can't find instance insertion point at "
                            << Ast_ਗੰਢ::prettyNameQ(baddot) << " in: " << nodep->prettyNameQ());
        }
        {
            m_scope = m_scope + "." + nodep->name();
            m_curSymp = m_modSymp = m_statep->ਪਾਓ_Cell(aboveSymp, m_modSymp, nodep, m_scope);
            m_ਵਿੱਚ_Recursion = nodep->recursive();
            // We don't report NotFoundModule, as may be a unused module in a generate
            ਜੇ (nodep->modp()) iterate(nodep->modp());
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstCellInline* nodep) override {  // FindVisitor::
        UINFO(5, "   CELLINLINE under " << m_scope << " is " << nodep);
        VSymEnt* aboveSymp = m_curSymp;
        std::string ident = nodep->name();
        // If baz__DOT__foo__DOT__bar, we need to find baz__DOT__foo and add bar to it.
        ਅਚਲ std::string::ਮਾਪ_type pos = ident.rfind("__DOT__");
        ਜੇ (pos != std::string::npos) {
            ਅਚਲ std::string dotted = ident.substr(0, pos);
            ident = ident.substr(pos + std::ਲੜੀ_ਲੰਬਾਈ("__DOT__"));
            std::string baddot;
            VSymEnt* okSymp;
            aboveSymp = m_statep->ਲੱਭੋ_Dotted(nodep->fileline(), aboveSymp, dotted, baddot, okSymp,
                                             ਝੂਠ);
            UASSERT_OBJ(aboveSymp, nodep,
                        "Can't find cellinline insertion point at "
                            << Ast_ਗੰਢ::prettyNameQ(baddot) << " in: " << nodep->prettyNameQ());
        }
        m_statep->ਪਾਓ_Inline(aboveSymp, m_modSymp, nodep, ident);
    }
    ਖਾਲੀ ਦੇਖੋ(AstDefParam* nodep) override {  // FindVisitor::
        nodep->user1p(m_curSymp);
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstRefDType* nodep) override {  // FindVisitor::
        nodep->user1p(m_curSymp);
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Gen_ਖੰਡ* nodep) override {  // FindVisitor::
        UINFO(5, "   " << nodep);
        ਜੇ (nodep->name() == "" && nodep->unnamed()) {
            // Unnamed blocks are only important when they contain var
            // decls, so search for them. (Otherwise adding all the
            // unnamed#'s would just confuse tracing variables in
            // places such as tasks, where "task ...; begin ... end"
            // are common.
            ਲਈ (Ast_ਗੰਢ* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
                ਜੇ (VN_IS(itemp, Var) || VN_IS(itemp, Foreach)) {
                    std::string name;
                    ਅਚਲ std::string ਕਦਮ_Str = m_statep->forPrimary()
                                                    ? ""
                                                    : std::ਮੰਜ਼ਿਲ_string(m_statep->ਕਦਮ_ਸੰਖਿਆ()) + "_";
                    ਕਰੋ {
                        ++m_mod_ਖੰਡ_ਅੰਕ;
                        name = "unnamedblk" + ਕਦਮ_Str + cvt_ਮੰਜ਼ਿਲ_Str(m_mod_ਖੰਡ_ਅੰਕ);
                        // Increment again if earlier pass of V3LinkDot claimed this name
                    } ਜਦੋਂ (m_curSymp->ਲੱਭੋ_Id_Flat(name));
                    nodep->name(name);
                    ਰੋਕੋ;
                }
            }
        }
        ਜੇ (nodep->name() == "") {
            iterateChildren(nodep);
        } ਨਹੀਂਤਾਂ {
            VL_RESTORER(m_curSymp);
            {
                m_curSymp
                    = m_statep->ਪਾਓ_ਖੰਡ(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
                m_curSymp->fallbackp(VL_RESTORER_PREV(m_curSymp));
                // Iterate
                iterateChildren(nodep);
            }
        }
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_ਖੰਡ* nodep) override {  // FindVisitor::
        UINFO(5, "   " << nodep);
        ਜੇ (nodep->name() == "" && nodep->unnamed()) {
            // Unnamed blocks are only important when they contain var
            // decls, so search for them. (Otherwise adding all the
            // unnamed#'s would just confuse tracing variables in
            // places such as tasks, where "task ...; begin ... end"
            // are common.
            ਅਚਲ ਸਵੈਚਾਲਿਤ ਸ਼ਾਮਲ_ਹੈ_Decl = [](ਅਚਲ Ast_ਗੰਢ* nodesp) -> ਤਾਰਕਿਕ {
                ਲਈ (ਅਚਲ Ast_ਗੰਢ* np = nodesp; np; np = np->nextp()) {
                    ਜੇ (VN_IS(np, Var) || VN_IS(np, Foreach)) ਵਾਪਸ ਸੱਚ;
                }
                ਵਾਪਸ ਝੂਠ;
            };
            ਤਾਰਕਿਕ needName = nodep->declsp() || ਸ਼ਾਮਲ_ਹੈ_Decl(nodep->stmtsp());
            ਜੇ (ਅਚਲ AstFork* ਅਚਲ forkp = VN_CAST(nodep, Fork)) {
                needName = needName || ਸ਼ਾਮਲ_ਹੈ_Decl(forkp->forksp());
            }
            ਜੇ (needName) {
                ਅਚਲ std::string ਕਦਮ_Str
                    = m_statep->forPrimary() ? "" : std::ਮੰਜ਼ਿਲ_string(m_statep->ਕਦਮ_ਸੰਖਿਆ()) + "_";
                std::string name;
                ਕਰੋ {
                    ++m_mod_ਖੰਡ_ਅੰਕ;
                    name = "unnamedblk" + ਕਦਮ_Str + cvt_ਮੰਜ਼ਿਲ_Str(m_mod_ਖੰਡ_ਅੰਕ);
                    // Increment again if earlier pass of V3LinkDot claimed this name
                } ਜਦੋਂ (m_curSymp->ਲੱਭੋ_Id_Flat(name));
                nodep->name(name);
            }
        }
        ਜੇ (nodep->name() == "") {
            iterateChildren(nodep);
        } ਨਹੀਂਤਾਂ {
            VL_RESTORER(m_curSymp);
            {
                m_curSymp
                    = m_statep->ਪਾਓ_ਖੰਡ(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
                m_curSymp->fallbackp(VL_RESTORER_PREV(m_curSymp));
                // Iterate
                iterateChildren(nodep);
            }
        }
    }
    // Handle out-of-block interface method definition (IEEE 1800-2023 25.7)
    // Move task body from module into the interface's prototype task,
    // rewriting port-prefixed references in the body.
    ਖਾਲੀ moveIfaceExportBody(Ast_ਗੰਢ_FTask* nodep) {
        ਅਚਲ string& ਪੋਰਟ_Name = nodep->iface_ਪੋਰਟ_Name();
        UINFO(5, "   moveIfaceExportBody: port=" << ਪੋਰਟ_Name << " task=" << nodep->name());
        // Look up the interface port variable in the module's symbol table
        VSymEnt* ਅਚਲ ਪੋਰਟ_Symp = m_modSymp->ਲੱਭੋ_Id_Fallback(ਪੋਰਟ_Name);
        ਜੇ (!ਪੋਰਟ_Symp) {
            nodep->v3error("Interface port not found for out-of-block definition: '"
                           << ਪੋਰਟ_Name << "' (IEEE 1800-2023 25.7)");
            nodep->unlink_Fr_ਪਿੱਠ();
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
            ਵਾਪਸ;
        }
        AstVar* ਅਚਲ portp = VN_CAST(ਪੋਰਟ_Symp->nodep(), Var);
        ਜੇ (!portp) {
            nodep->v3fatalSrc("Out-of-block definition port is not a variable: " << ਪੋਰਟ_Name);
            ਵਾਪਸ;
        }
        // Get the interface name from the port's type
        // At this stage dtypep() may not be set yet; check childDTypep() too
        AstIfaceRefDType* ifaceRefDtp = ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (portp->dtypep()) { ifaceRefDtp = VN_CAST(portp->dtypep(), IfaceRefDType); }
        ਜੇ (!ifaceRefDtp && portp->ਬੱਚਾ_DTypep()) {
            ifaceRefDtp = VN_CAST(portp->ਬੱਚਾ_DTypep(), IfaceRefDType);
        }
        ਜੇ (!ifaceRefDtp) {
            nodep->v3error("Out-of-block definition port is not an interface port: '"
                           << ਪੋਰਟ_Name << "' (IEEE 1800-2023 25.7)");
            nodep->unlink_Fr_ਪਿੱਠ();
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
            ਵਾਪਸ;
        }
        ਅਚਲ string ifaceName = ifaceRefDtp->ifaceName();
        UINFO(5, "   moveIfaceExportBody: iface=" << ifaceName);
        // Find the interface module by name
        Ast_ਗੰਢ_Module* ਅਚਲ ifaceModp = m_statep->ਲੱਭੋ_Module_Sym(ifaceName);
        ਜੇ (!ifaceModp) {
            nodep->v3fatalSrc("Interface not found for out-of-block definition: " << ifaceName);
            ਵਾਪਸ;
        }
        // Find the prototype task in the interface
        Ast_ਗੰਢ_FTask* protoTaskp = ਖਾਲੀ_ਸੂਚਕ;
        ਲਈ (Ast_ਗੰਢ* itemp = ifaceModp->stmtsp(); itemp; itemp = itemp->nextp()) {
            ਜੇ (Ast_ਗੰਢ_FTask* ਅਚਲ ftaskp = VN_CAST(itemp, ਗੰਢ_FTask)) {
                ਜੇ (ftaskp->name() == nodep->name() && ftaskp->prototype()) {
                    protoTaskp = ftaskp;
                    ਰੋਕੋ;
                }
            }
        }
        ਜੇ (!protoTaskp) {
            nodep->v3error("No matching export prototype found in interface "
                           << ifaceName << " for task " << nodep->prettyNameQ()
                           << " (IEEE 1800-2023 25.7)");
            nodep->unlink_Fr_ਪਿੱਠ();
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
            ਵਾਪਸ;
        }
        // Move body statements (non-port declarations) from the out-of-block task
        // to the prototype task. Rewrite port.X references to just X in the body.
        ਲਈ (Ast_ਗੰਢ* stmtp = nodep->stmtsp(); stmtp;) {
            Ast_ਗੰਢ* ਅਚਲ nextp = stmtp->nextp();
            // Skip port declarations -- they already exist in the prototype
            ਜੇ (ਅਚਲ AstVar* ਅਚਲ varp = VN_CAST(stmtp, Var)) {
                ਜੇ (varp->isIO()) {
                    stmtp = nextp;
                    ਜਾਰੀ;
                }
            }
            // Clone this statement and rewrite port references
            Ast_ਗੰਢ* ਅਚਲ newStmtp = stmtp->clone_ਰੁੱਖ(ਝੂਠ);
            rewrite_Iface_ਪੋਰਟ_Refs_Single(newStmtp, ਪੋਰਟ_Name);
            protoTaskp->ਜੋੜੋ_Stmtsp(newStmtp);
            stmtp = nextp;
        }
        // Mark prototype as no longer prototype (it has a body now)
        protoTaskp->prototype(ਝੂਠ);
        // Delete the out-of-block task from the module
        nodep->unlink_Fr_ਪਿੱਠ();
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
    }

    // Rewrite AstDot nodes where lhsp is AstParseRef with the port name
    // to just the rhsp (stripping the port prefix).
    ਸਥਿਰ ਖਾਲੀ rewrite_Iface_ਪੋਰਟ_Refs_Single(Ast_ਗੰਢ* nodep, ਅਚਲ string& ਪੋਰਟ_Name) {
        ਜੇ (!nodep) ਵਾਪਸ;
        // Check if this is a non-colon Dot with lhs matching portName
        ਜੇ (AstDot* ਅਚਲ dotp = VN_CAST(nodep, Dot)) {
            ਜੇ (!dotp->colon()) {
                ਅਚਲ AstParseRef* ਅਚਲ lhsRefp = VN_CAST(dotp->lhsp(), ParseRef);
                ਜੇ (lhsRefp && lhsRefp->name() == ਪੋਰਟ_Name) {
                    UINFO(5, "   rewriteIfacePortRefs: stripping port prefix from " << dotp);
                    Ast_ਗੰਢ* ਅਚਲ rhsp = dotp->rhsp()->unlink_Fr_ਪਿੱਠ();
                    dotp->replaceWith(rhsp);
                    VL_DO_DANGLING(dotp->delete_ਰੁੱਖ(), dotp);
                    rewrite_Iface_ਪੋਰਟ_Refs_Single(rhsp, ਪੋਰਟ_Name);
                    ਵਾਪਸ;
                }
            }
        }
        // Recurse into all children
        ਲਈ (Ast_ਗੰਢ* childp : {nodep->op1p(), nodep->op2p(), nodep->op3p(), nodep->op4p()}) {
            ਜਦੋਂ (childp) {
                Ast_ਗੰਢ* ਅਚਲ nextp = childp->nextp();
                rewrite_Iface_ਪੋਰਟ_Refs_Single(childp, ਪੋਰਟ_Name);
                childp = nextp;
            }
        }
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_FTask* nodep) override {  // FindVisitor::
        // NodeTask: Remember its name for later resolution
        UINFO(5, "   " << nodep);
        UASSERT_OBJ(m_curSymp && m_modSymp, nodep, "Function/Task not under module?");
        ਜੇ (nodep->name() == "new") m_explicitNew = ਸੱਚ;
        // Handle out-of-block interface method definition (IEEE 25.8)
        ਜੇ (!nodep->iface_ਪੋਰਟ_Name().ਖਾਲੀ_ਥਾਵਾਂ() && m_statep->forPrimary()) {
            moveIfaceExportBody(nodep);
            ਵਾਪਸ;  // Task has been deleted
        }
        // Remember the existing symbol table scope
        VL_RESTORER(m_classOrPackagep);
        VL_RESTORER(m_curSymp);
        VSymEnt* ਉੱਪਰ_Symp = m_curSymp;

        ਜੇ (VN_IS(m_curSymp->nodep(), Class)) {
            ਜੇ (VN_AS(m_curSymp->nodep(), Class)->isInterfaceClass() && !nodep->ਸ਼ੁੱਧ_Virtual()
                && !nodep->isConstructor()) {
                nodep->v3error("Interface class functions must be pure virtual"
                               << " (IEEE 1800-2023 8.26): " << nodep->prettyNameQ());
            }
            ਜੇ (m_statep->forPrimary()
                && (nodep->name() == "randomize" || nodep->name() == "srandom")) {
                nodep->v3error(nodep->prettyNameQ()
                               << " is a predefined class method; redefinition not allowed"
                                  " (IEEE 1800-2023 18.6.3)");
            }
        }
        // Change to appropriate package if extern declaration (vs definition)
        ਜੇ (nodep->classOrPackagep()) {
            // class-in-class
            AstDot* ਅਚਲ dotp = VN_CAST(nodep->classOrPackagep(), Dot);
            AstClassOrPackageRef* ਅਚਲ cpackagerefp
                = VN_CAST(nodep->classOrPackagep(), ClassOrPackageRef);
            ਜੇ (dotp) {
                AstClassOrPackageRef* ਅਚਲ lhsp = VN_AS(dotp->lhsp(), ClassOrPackageRef);
                m_statep->resolveClassOrPackage(m_curSymp, lhsp, ਸੱਚ, ਝੂਠ,
                                                "External definition :: reference");
                AstClass* ਅਚਲ lhsclassp = VN_CAST(lhsp->classOrPackageSkipp(), Class);
                ਜੇ (!lhsclassp) {
                    nodep->v3error("Extern declaration's scope is not a defined class");
                } ਨਹੀਂਤਾਂ {
                    m_curSymp = m_statep->get_ਗੰਢ_Sym(lhsclassp);
                    ਉੱਪਰ_Symp = m_curSymp;
                    AstClassOrPackageRef* ਅਚਲ rhsp = VN_AS(dotp->rhsp(), ClassOrPackageRef);
                    m_statep->resolveClassOrPackage(m_curSymp, rhsp, ਸੱਚ, ਝੂਠ,
                                                    "External definition :: reference");
                    AstClass* ਅਚਲ rhsclassp = VN_CAST(rhsp->classOrPackageSkipp(), Class);
                    ਜੇ (!rhsclassp) {
                        nodep->v3error("Extern declaration's scope is not a defined class");
                    } ਨਹੀਂਤਾਂ {
                        m_curSymp = m_statep->get_ਗੰਢ_Sym(rhsclassp);
                        ਉੱਪਰ_Symp = m_curSymp;
                        moveExternFuncDecl(nodep, rhsclassp);
                    }
                }
            } ਨਹੀਂਤਾਂ ਜੇ (cpackagerefp) {
                ਜੇ (!cpackagerefp->classOrPackageSkipp()) {
                    m_statep->resolveClassOrPackage(m_curSymp, cpackagerefp, ਸੱਚ, ਝੂਠ,
                                                    "External definition :: reference");
                }
                AstClass* ਅਚਲ classp = VN_CAST(cpackagerefp->classOrPackageSkipp(), Class);
                ਜੇ (!classp) {
                    nodep->v3error("Extern declaration's scope is not a defined class");
                } ਨਹੀਂਤਾਂ {
                    m_curSymp = m_statep->get_ਗੰਢ_Sym(classp);
                    ਉੱਪਰ_Symp = m_curSymp;
                    moveExternFuncDecl(nodep, classp);
                }
            } ਨਹੀਂਤਾਂ {
                v3fatalSrc("Unhandled extern function definition package");
            }
        }
        // Set the class as package for iteration
        ਜੇ (VN_IS(m_curSymp->nodep(), Class)) {
            m_classOrPackagep = VN_AS(m_curSymp->nodep(), Class);
        }
        // Create symbol table for the task's vars
        ਅਚਲ string name = (nodep->isExternProto() ? "extern "s : ""s) + nodep->name();
        m_curSymp = m_statep->ਪਾਓ_ਖੰਡ(m_curSymp, name, nodep, m_classOrPackagep);
        m_curSymp->fallbackp(ਉੱਪਰ_Symp);
        // Convert the func's range to the output variable
        // This should probably be done in the Parser instead, as then we could
        // just attach normal signal attributes to it.
        ਜੇ (!nodep->isExternProto() && nodep->fvarp() && !VN_IS(nodep->fvarp(), Var)) {
            Ast_ਗੰਢ_DType* dtypep = VN_CAST(nodep->fvarp(), ਗੰਢ_DType);
            // If unspecified, function returns one bit; however when we
            // support NEW() it could also return the class reference.
            ਜੇ (dtypep) {
                dtypep->unlink_Fr_ਪਿੱਠ();
            } ਨਹੀਂਤਾਂ {
                dtypep = ਨਵਾਂ AstBasicDType{nodep->fileline(), VBasicDTypeKwd::LOGIC};
            }
            AstVar* ਅਚਲ newvarp
                = ਨਵਾਂ AstVar{nodep->fileline(), VVarType::VAR, nodep->name(), VFlag_ਬੱਚਾ_DType{},
                             dtypep};  // Not dtype resolved yet
            newvarp->direction(VDirection::OUTPUT);
            newvarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            newvarp->funcReturn(ਸੱਚ);
            newvarp->trace(ਝੂਠ);  // Not user visible
            nodep->fvarp(newvarp);
            // Explicit insert required, as the var name shadows the upper level's task name
            m_statep->ਪਾਓ_Sym(m_curSymp, newvarp->name(), newvarp, ਖਾਲੀ_ਸੂਚਕ /*classOrPackagep*/);
        }
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstClocking* nodep) override {  // FindVisitor::
        VL_RESTORER(m_clockingp);
        m_clockingp = nodep;
        iterate(nodep->sentreep());
        iterate_And_ਅਗਲਾ_Null(nodep->itemsp());
        // If the block has no name, one cannot reference the clockvars
        VSymEnt* itSymp = ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (nodep->isGlobal()  //
            && m_statep->forPrimary()) {  // else flattening may see two globals
            m_statep->checkDuplicate(m_curSymp, nodep, "__024global_clock");
            itSymp
                = m_statep->ਪਾਓ_ਖੰਡ(m_curSymp, "__024global_clock", nodep, m_classOrPackagep);
            itSymp->fallbackp(ਖਾਲੀ_ਸੂਚਕ);
        }
        ਜੇ (!nodep->name().ਖਾਲੀ_ਥਾਵਾਂ()) {
            itSymp = m_statep->ਪਾਓ_ਖੰਡ(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
            itSymp->fallbackp(ਖਾਲੀ_ਸੂਚਕ);
        }
        ਜੇ (itSymp) {
            VL_RESTORER(m_curSymp);
            m_curSymp = itSymp;
            iterate_And_ਅਗਲਾ_Null(nodep->itemsp());
        }
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Clocking_ਵਸਤੂ* nodep) override {  // FindVisitor::
        ਜੇ (nodep->varp()) {
            ਜੇ (m_curSymp->nodep() == m_clockingp) iterate(nodep->varp());
            ਵਾਪਸ;
        }
        std::string varname;
        Ast_ਗੰਢ_DType* dtypep;
        ਜੇ (Ast_ਸੌਂਪੋ* ਅਚਲ assignp = nodep->assignp()) {
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rhsp = assignp->rhsp()->unlink_Fr_ਪਿੱਠ();
            dtypep = ਨਵਾਂ AstRefDType{nodep->fileline(), AstRefDType::Flag_Type_ਦਾ_ਵਿਅੰਜਕ{},
                                     rhsp->clone_ਰੁੱਖ(ਝੂਠ)};
            nodep->exprp(rhsp);
            varname = assignp->lhsp()->name();
            VL_DO_DANGLING(assignp->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ(), assignp);
        } ਨਹੀਂਤਾਂ {
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ refp = nodep->exprp();
            ਅਚਲ VSymEnt* foundp = m_curSymp->ਲੱਭੋ_Id_Fallback(refp->name());
            ਜੇ (!foundp || !foundp->nodep()) {
                refp->v3error("Corresponding variable " << refp->prettyNameQ()
                                                        << " does not exist");
                VL_DO_DANGLING(nodep->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ(), nodep);
                ਵਾਪਸ;
            }
            varname = refp->name();
            dtypep = VN_AS(foundp->nodep(), Var)->ਬੱਚਾ_DTypep()->clone_ਰੁੱਖ(ਝੂਠ);
        }
        AstVar* ਅਚਲ newvarp = ਨਵਾਂ AstVar{nodep->fileline(), VVarType::MODULETEMP, varname,
                                           VFlag_ਬੱਚਾ_DType{}, dtypep};
        newvarp->lifetime(VLifetime::STATIC_EXPLICIT);
        nodep->varp(newvarp);
        iterate(nodep->exprp());
    }
    ਖਾਲੀ ਦੇਖੋ(AstConstraint* nodep) override {  // FindVisitor::
        VL_RESTORER(m_curSymp);
        // Change to appropriate package if extern declaration (vs definition)
        VSymEnt* ਉੱਪਰ_Symp = m_curSymp;
        ਜੇ (nodep->classOrPackagep()) {
            AstClassOrPackageRef* ਅਚਲ cpackagerefp
                = VN_CAST(nodep->classOrPackagep(), ClassOrPackageRef);
            ਜੇ (!cpackagerefp) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: extern constraint definition with class-in-class");
            } ਨਹੀਂਤਾਂ {
                ਜੇ (!cpackagerefp->classOrPackageSkipp()) {
                    m_statep->resolveClassOrPackage(m_curSymp, cpackagerefp, ਸੱਚ, ਝੂਠ,
                                                    "External definition :: reference");
                }
                AstClass* ਅਚਲ classp = VN_CAST(cpackagerefp->classOrPackageSkipp(), Class);
                ਜੇ (!classp) {
                    nodep->v3error("Extern declaration's scope is not a defined class");
                } ਨਹੀਂਤਾਂ {
                    m_curSymp = m_statep->get_ਗੰਢ_Sym(classp);
                    ਉੱਪਰ_Symp = m_curSymp;
                    // Move it to proper spot under the target class
                    nodep->unlink_Fr_ਪਿੱਠ();
                    classp->ਜੋੜੋ_Stmtsp(nodep);
                    nodep->classOrPackagep()->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ();
                }
            }
        }
        // Set the class as package for iteration
        ਅਚਲ string name = (nodep->isExternProto() ? "extern "s : ""s) + nodep->name();
        m_curSymp = m_statep->ਪਾਓ_ਖੰਡ(ਉੱਪਰ_Symp, name, nodep, m_classOrPackagep);
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstVar* nodep) override {  // FindVisitor::
        // Var: Remember its name for later resolution
        UASSERT_OBJ(m_curSymp && m_modSymp, nodep, "Var not under module?");
        iterateChildren(nodep);
        ਜੇ (VN_IS(m_curSymp->nodep(), Class)
            && VN_AS(m_curSymp->nodep(), Class)->isInterfaceClass() && !nodep->isParam()) {
            nodep->v3error("Interface class cannot contain non-parameter members"
                           << " (IEEE 1800-2023 8.26): " << nodep->prettyNameQ());
        }
        ਜੇ (!m_statep->forScopeCreation()) {
            // Find under either a task or the module's vars
            ਅਚਲ VSymEnt* foundp = m_curSymp->ਲੱਭੋ_Id_Fallback(nodep->name());
            ਜੇ (!foundp && m_modSymp && nodep->name() == m_modSymp->nodep()->name()) {
                foundp = m_modSymp;  // Conflicts with modname?
            }
            AstVar* ਅਚਲ findvarp = foundp ? VN_CAST(foundp->nodep(), Var) : ਖਾਲੀ_ਸੂਚਕ;
            // clocking items can have duplicate names (inout)
            ਜੇ (findvarp && VN_IS(findvarp->backp(), Clocking_ਵਸਤੂ)
                && VN_IS(nodep->backp(), Clocking_ਵਸਤੂ)) {
                Ast_Clocking_ਵਸਤੂ* ਅਚਲ itemp = VN_AS(nodep->backp(), Clocking_ਵਸਤੂ);
                Ast_Clocking_ਵਸਤੂ* ਅਚਲ finditemp = VN_AS(findvarp->backp(), Clocking_ਵਸਤੂ);
                UINFO(4, "ClockCompl: " << itemp << " ;; " << finditemp);
                UINFO(4, "ClockCompV: " << nodep << " ;; " << findvarp);
                ਜੇ (*itemp->exprp()->fileline() == *finditemp->exprp()->fileline()) {
                    UASSERT_OBJ(finditemp->direction() == VDirection::ਨਿਵੇਸ਼
                                    && itemp->direction() == VDirection::OUTPUT,
                                itemp, "Input after output?");
                    // pretend nothing found and rename
                    foundp = ਖਾਲੀ_ਸੂਚਕ;
                    nodep->name("__Voutput_" + nodep->name());
                    finditemp->outputp(itemp);
                }
            }
            ਤਾਰਕਿਕ ins = ਝੂਠ;
            ਜੇ (!foundp) {
                ins = ਸੱਚ;
            } ਨਹੀਂਤਾਂ ਜੇ (!findvarp && m_curSymp->ਲੱਭੋ_Id_Flat(nodep->name()) && !foundp->imported()) {
                nodep->v3error("Unsupported in C: Variable has same name as "
                               << LinkDotState::ਗੰਢ_ਪਾਠ_Type(foundp->nodep()) << ": "
                               << nodep->prettyNameQ());
            } ਨਹੀਂਤਾਂ ਜੇ (findvarp != nodep) {
                UINFO(4, "DupVar: " << nodep << " ;; " << foundp->nodep());
                UINFO(4, "    found  cur=se" << cvt_ਮੰਜ਼ਿਲ_Hex(m_curSymp) << " ;; parent=se"
                                             << cvt_ਮੰਜ਼ਿਲ_Hex(foundp->parentp()));
                ਜੇ (foundp->parentp() == m_curSymp  // Only when on same level
                    && !foundp->imported()) {  // and not from package
                    ਜੇ (VN_IS(m_curSymp->nodep(), Clocking)) {
                        nodep->v3error("Multiple clockvars with the same name not allowed");
                        ਵਾਪਸ;
                    }
                    ਅਚਲ ਤਾਰਕਿਕ nansiBad
                        = (((findvarp->isDeclTyped() || findvarp->isNet())
                            && (nodep->isDeclTyped() || nodep->isNet()))
                           || (findvarp->isIO() && nodep->isIO())  // e.g. !(output && output)
                           || findvarp->gotNansiType());  // e.g. int x && int x
                    ਅਚਲ ਤਾਰਕਿਕ ansiBad
                        = findvarp->isAnsi() || nodep->isAnsi();  // dup illegal with ANSI
                    ਜੇ (ansiBad || nansiBad) {
                        ਤਾਰਕਿਕ ansiWarn = ansiBad && !nansiBad;
                        ਜੇ (ansiWarn) {
                            ਸਥਿਰ ਪੂਰਨ s_didAnsiWarn = ਝੂਠ;
                            ਜੇ (s_didAnsiWarn++) ansiWarn = ਝੂਠ;
                        }
                        nodep->v3error("Duplicate declaration of signal: "
                                       << nodep->prettyNameQ() << '\n'
                                       << (ansiWarn ? nodep->warn_ਹੋਰ()
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
                    } ਨਹੀਂਤਾਂ {
                        findvarp->combineType(nodep);
                        findvarp->fileline()->modifyStateInherit(nodep->fileline());
                        findvarp->gotNansiType(ਸੱਚ);
                        UASSERT_OBJ(nodep->subDTypep(), nodep, "Var has no type");
                        ਜੇ (nodep->subDTypep()->numeric().isSigned()
                            && !findvarp->subDTypep()->numeric().isSigned()) {
                            findvarp->subDTypep()->numeric(VSigning{ਸੱਚ});
                        }
                        Ast_ਗੰਢ_DType* ਅਚਲ varDtp = findvarp->subDTypep();
                        Ast_ਗੰਢ_DType* ਅਚਲ otherDtp = nodep->subDTypep();
                        ਜੇ (varDtp
                            && (VN_IS(varDtp, BasicDType)
                                && VN_AS(varDtp, BasicDType)->implicit())) {
                            // Then have "input foo" and "real foo" so the
                            // dtype comes from the other side.
                            VL_DO_DANGLING(varDtp->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ(), varDtp);
                            findvarp->ਬੱਚਾ_DTypep(otherDtp->unlink_Fr_ਪਿੱਠ());
                        } ਨਹੀਂਤਾਂ ਜੇ (!m_statep->forScopeCreation() && otherDtp && varDtp
                                   && !(VN_IS(otherDtp, BasicDType)
                                        && VN_AS(otherDtp, BasicDType)->implicit())) {
                            // otherDtp and varDtp both non-nullptr and neither are implicit
                            // Can't compare dtypes now as might contain parameters,
                            // defer to V3Width
                            Ast_Attr_ਦਾ* ਅਚਲ newp
                                = ਨਵਾਂ Ast_Attr_ਦਾ{otherDtp->fileline(), VAttrType::VAR_ਪੋਰਟ_DTYPE,
                                                otherDtp->unlink_Fr_ਪਿੱਠ()};
                            findvarp->ਜੋੜੋ_Attrsp(newp);
                        }
                    }
                    VL_DO_DANGLING(nodep->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ(), nodep);
                } ਨਹੀਂਤਾਂ {
                    // User can disable the message at either point
                    ਜੇ (!(m_ftaskp && m_ftaskp->dpiImport())
                        && (!m_ftaskp || m_ftaskp != foundp->nodep())  // Not the function's
                                                                       // variable hiding function
                        && !nodep->fileline()->warnIsOff(V3_Error_ਕੋਡ::VARHIDDEN)
                        && !foundp->nodep()->fileline()->warnIsOff(V3_Error_ਕੋਡ::VARHIDDEN)) {
                        nodep->v3warn(VARHIDDEN,
                                      "Declaration of signal hides declaration in upper scope: "
                                          << nodep->prettyNameQ() << '\n'
                                          << nodep->warnContextPrimary() << '\n'
                                          << foundp->nodep()->warnOther()
                                          << "... Location of original declaration\n"
                                          << foundp->nodep()->warnContextSecondary());
                    }
                    ins = ਸੱਚ;
                }
            }
            ਜੇ (ins) {
                ਜੇ (m_statep->forPrimary() && nodep->isGParam()
                    && VN_IS(m_modSymp->nodep(), Module)
                    && (m_statep->ਜੜ੍ਹ_Entp()->nodep() == m_modSymp->parentp()->nodep())) {
                    // This is the toplevel module. Check for command line overwrites of parameters
                    // We first search if the parameter is overwritten and then replace it with a
                    // new value.
                    ਜੇ (v3Global.opt.hasParameter(nodep->name())) {
                        ਅਚਲ string svalue = v3Global.opt.parameter(nodep->name());
                        ਜੇ (AstConst* ਅਚਲ valuep
                            = AstConst::parseParamLiteral(nodep->fileline(), svalue)) {
                            UINFO(9, "       replace parameter " << nodep);
                            UINFO(9, "       with " << valuep);
                            ਜੇ (nodep->valuep()) ਧੱਕੋ_Deletep(nodep->valuep()->unlink_Fr_ਪਿੱਠ());
                            nodep->valuep(valuep);
                            // Do not warn ENUMVALUE when overriding an enum param from the cli.
                            nodep->fileline()->warnOff(V3_Error_ਕੋਡ::ENUMVALUE, ਸੱਚ);
                        }
                    }
                }
                m_statep->ਪਾਓ_Sym(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
                ਜੇ (m_statep->forPrimary() && nodep->isGParam()) {
                    ++m_param_ਅੰਕ;
                    VSymEnt* ਅਚਲ symp
                        = m_statep->ਪਾਓ_Sym(m_curSymp, "__paramNumber" + cvt_ਮੰਜ਼ਿਲ_Str(m_param_ਅੰਕ),
                                              nodep, m_classOrPackagep);
                    symp->exported(ਝੂਠ);
                }
            }
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstTypedef* nodep) override {  // FindVisitor::
        UASSERT_OBJ(m_curSymp, nodep, "Typedef not under module/package/$unit");
        ਜੇ (VN_IS(m_classOrPackagep, Class)) nodep->isUnderClass(ਸੱਚ);
        iterateChildren(nodep);
        m_statep->ਪਾਓ_Sym(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstTypedefFwd* nodep) override {  // FindVisitor::
        UASSERT_OBJ(m_curSymp, nodep, "Typedef not under module/package/$unit");
        iterateChildren(nodep);
        // No need to insert, only the real typedef matters, but need to track for errors
        nodep->user1p(m_curSymp);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_UOr_Struct_DType* nodep) override {  // FindVisitor::
        UASSERT_OBJ(m_curSymp, nodep, "Struct/union dtype not under module/package/$unit");
        VL_RESTORER(m_curSymp);
        m_curSymp = m_statep->ਪਾਓ_ਖੰਡ(m_curSymp, "__Vdtype" + cvt_ਮੰਜ਼ਿਲ_Str(nodep->unique_ਅੰਕ()),
                                          nodep, m_classOrPackagep);
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstMemberDType* nodep) override {  // FindVisitor::
        iterateChildren(nodep);
        m_statep->ਪਾਓ_Sym(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstParamTypeDType* nodep) override {  // FindVisitor::
        UASSERT_OBJ(m_curSymp, nodep, "Parameter type not under module/package/$unit");

        // Replace missing param types with provided hierarchical type params.
        ਜੇ (!m_hierParamsName.ਖਾਲੀ_ਥਾਵਾਂ()) {
            ਜੇ (ਅਚਲ VSymEnt* ਅਚਲ typedefEntp = m_curSymp->ਲੱਭੋ_Id_Fallback(m_hierParamsName)) {
                ਅਚਲ AstModule* modp = VN_CAST(typedefEntp->nodep(), Module);

                ਲਈ (ਅਚਲ Ast_ਗੰਢ* stmtp = modp ? modp->stmtsp() : ਖਾਲੀ_ਸੂਚਕ; stmtp;
                     stmtp = stmtp->nextp()) {
                    ਅਚਲ AstTypedef* tdefp = VN_CAST(stmtp, Typedef);

                    ਜੇ (tdefp && tdefp->name() == nodep->name() && m_statep->forPrimary()) {
                        UINFO(8, "Replacing type of" << nodep << "  -->with " << tdefp);
                        Ast_ਗੰਢ_DType* ਅਚਲ newDtp = tdefp->ਬੱਚਾ_DTypep();
                        Ast_ਗੰਢ_DType* ਅਚਲ oldDtp = nodep->ਬੱਚਾ_DTypep();

                        oldDtp->replaceWith(newDtp->clone_ਰੁੱਖ(ਝੂਠ));
                        VL_DO_DANGLING(oldDtp->delete_ਰੁੱਖ(), oldDtp);
                    }
                }
            }
        }

        iterateChildren(nodep);
        m_statep->ਪਾਓ_Sym(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
        ਜੇ (m_statep->forPrimary() && nodep->isGParam()) {
            ++m_param_ਅੰਕ;
            VSymEnt* ਅਚਲ symp = m_statep->ਪਾਓ_Sym(
                m_curSymp, "__paramNumber" + cvt_ਮੰਜ਼ਿਲ_Str(m_param_ਅੰਕ), nodep, m_classOrPackagep);
            symp->exported(ਝੂਠ);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstCFunc* nodep) override {  // FindVisitor::
        // For dotted resolution, ignore all AstVars under functions, otherwise shouldn't exist
        UASSERT_OBJ(!m_statep->forScopeCreation(), nodep, "No CFuncs expected in tree yet");
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Enum_ਵਸਤੂ* nodep) override {  // FindVisitor::
        // EnumItem: Remember its name for later resolution
        iterateChildren(nodep);
        // Find under either a task or the module's vars
        ਅਚਲ VSymEnt* foundp = m_curSymp->ਲੱਭੋ_Id_Fallback(nodep->name());
        ਜੇ (!foundp && m_modSymp && nodep->name() == m_modSymp->nodep()->name()) {
            foundp = m_modSymp;  // Conflicts with modname?
        }
        Ast_Enum_ਵਸਤੂ* ਅਚਲ findvarp = foundp ? VN_CAST(foundp->nodep(), Enum_ਵਸਤੂ) : ਖਾਲੀ_ਸੂਚਕ;
        ਤਾਰਕਿਕ ins = ਝੂਠ;
        ਜੇ (!foundp) {
            ins = ਸੱਚ;
        } ਨਹੀਂਤਾਂ ਜੇ (findvarp != nodep) {
            UINFO(4, "DupVar: " << nodep << " ;; " << foundp);
            ਜੇ (foundp->parentp() == m_curSymp  // Only when on same level
                && !foundp->imported()) {  // and not from package
                nodep->v3error("Duplicate declaration of enum value: "
                               << nodep->prettyName() << '\n'
                               << nodep->warnContextPrimary() << '\n'
                               << foundp->nodep()->warnOther()
                               << "... Location of original declaration\n"
                               << foundp->nodep()->warnContextSecondary());
            } ਨਹੀਂਤਾਂ {
                // User can disable the message at either point
                ਜੇ (!nodep->fileline()->warnIsOff(V3_Error_ਕੋਡ::VARHIDDEN)
                    && !foundp->nodep()->fileline()->warnIsOff(V3_Error_ਕੋਡ::VARHIDDEN)) {
                    nodep->v3warn(VARHIDDEN,
                                  "Declaration of enum value hides declaration in upper scope: "
                                      << nodep->prettyName() << '\n'
                                      << nodep->warnContextPrimary() << '\n'
                                      << foundp->nodep()->warnOther()
                                      << "... Location of original declaration\n"
                                      << foundp->nodep()->warnContextSecondary());
                }
                ins = ਸੱਚ;
            }
        }
        ਜੇ (ins) m_statep->ਪਾਓ_Sym(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstPackageImport* nodep) override {  // FindVisitor::
        UINFO(4, "  Link: " << nodep);
        ਜੇ (!nodep->packagep()) ਵਾਪਸ;  // Errored in V3LinkCells
        VSymEnt* ਅਚਲ srcp = m_statep->get_ਗੰਢ_Sym(nodep->packagep());
        ਜੇ (nodep->name() == "*") {
            ਜੇ (nodep->packagep() != v3Global.rootp()->stdPackagep()) {
                ਜੇ (m_curSymp == m_statep->dunitEntp()) {
                    nodep->v3warn(IMPORTSTAR,
                                  "'import::*' in $unit scope may pollute global namespace");
                }
            }
        } ਨਹੀਂਤਾਂ ਜੇ (!nodep->resolvedClassp()) {
            VSymEnt* ਅਚਲ impp = srcp->ਲੱਭੋ_Id_Flat(nodep->name());
            ਜੇ (!impp) {
                nodep->v3error("Import object not found: " << nodep->prettyPkgNameQ());
            } ਨਹੀਂਤਾਂ ਜੇ (nodep->resolvedClassp()) {
                UASSERT_OBJ(nodep->resolvedClassp()->classOrPackageSkipp() == impp->nodep(), nodep,
                            "Import should resolve to the same node");
            } ਨਹੀਂਤਾਂ ਜੇ (AstClass* ਅਚਲ classp = VN_CAST(impp->nodep(), Class)) {
                nodep->resolvedClassp(
                    ਨਵਾਂ AstClassOrPackageRef{nodep->fileline(), classp->name(), classp, ਖਾਲੀ_ਸੂਚਕ});
            }
        }
        m_curSymp->importFromPackage(m_statep->symsp(), srcp, nodep->name());
        UINFO(9, "    Link Done: " << nodep);
        // No longer needed, but can't delete until any multi-instantiated modules are expanded
    }
    ਖਾਲੀ ਦੇਖੋ(AstPackageExport* nodep) override {  // FindVisitor::
        UINFO(9, "  Link: " << nodep);
        ਜੇ (!nodep->packagep()) ਵਾਪਸ;  // Errored in V3LinkCells
        VSymEnt* ਅਚਲ srcp = m_statep->get_ਗੰਢ_Sym(nodep->packagep());
        ਜੇ (nodep->name() != "*") {
            VSymEnt* ਅਚਲ impp = srcp->ਲੱਭੋ_Id_Flat(nodep->name());
            ਜੇ (!impp) {
                nodep->v3error("Export object not found: '" << nodep->packagep()->prettyName()
                                                            << "::" << nodep->prettyName() << "'");
            }
        }
        m_curSymp->exportFromPackage(m_statep->symsp(), srcp, nodep->name());
        UINFO(9, "    Link Done: " << nodep);
        // No longer needed, but can't delete until any multi-instantiated modules are expanded
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Package_Export_ਤਾਰਾ_ਤਾਰਾ* nodep) override {  // FindVisitor::
        UINFO(4, "  Link: " << nodep);
        m_curSymp->export_ਤਾਰਾ_ਤਾਰਾ(m_statep->symsp());
        // No longer needed, but can't delete until any multi-instantiated modules are expanded
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Foreach* nodep) override {  // FindVisitor::
        // Symbol table needs nodep->name() as the index variable's name
        VL_RESTORER(m_curSymp);
        {
            ++m_mod_With_ਅੰਕ;
            m_curSymp = m_statep->ਪਾਓ_ਖੰਡ(m_curSymp, "__Vforeach" + cvt_ਮੰਜ਼ਿਲ_Str(m_mod_With_ਅੰਕ),
                                              nodep, m_classOrPackagep);
            m_curSymp->fallbackp(VL_RESTORER_PREV(m_curSymp));
            ਲਈ (Ast_ਗੰਢ *nextp, *argp = nodep->headerp()->elementsp(); argp; argp = nextp) {
                nextp = argp->nextp();
                AstVar* argrefp = ਖਾਲੀ_ਸੂਚਕ;
                ਜੇ (AstParseRef* ਅਚਲ parserefp = VN_CAST(argp, ParseRef)) {
                    // IEEE 1800-2023 12.7.3: foreach loop variable type shall be int (2-state)
                    // This might get changed in V3Width when types resolve (e.g., for assoc
                    // arrays)
                    argrefp = ਨਵਾਂ AstVar{parserefp->fileline(), VVarType::BLOCKTEMP,
                                         parserefp->name(), argp->ਲੱਭੋ_Int_DType()};
                    argrefp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                    parserefp->replaceWith(argrefp);
                    VL_DO_DANGLING2(parserefp->delete_ਰੁੱਖ(), parserefp, argp);
                    // Insert argref's name into symbol table
                    m_statep->ਪਾਓ_Sym(m_curSymp, argrefp->name(), argrefp, ਖਾਲੀ_ਸੂਚਕ);
                } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ ਸਵੈਚਾਲਿਤ largrefp = VN_CAST(argp, Var)) {
                    argrefp = largrefp;
                    // Insert argref's name into symbol table
                    m_statep->ਪਾਓ_Sym(m_curSymp, argrefp->name(), argrefp, ਖਾਲੀ_ਸੂਚਕ);
                } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(argp, ਖਾਲੀ_ਥਾਵਾਂ_2)) {
                } ਨਹੀਂਤਾਂ {
                    argp->v3error("'foreach' loop variable expects simple variable name");
                }
            }
            iterateChildren(nodep);
        }
    }

    ਖਾਲੀ ਦੇਖੋ(AstRandSequence* nodep) override {
        VL_RESTORER(m_curSymp);
        m_curSymp = m_statep->ਪਾਓ_ਖੰਡ(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstRSProd* nodep) override {
        VL_RESTORER(m_curSymp);
        m_curSymp = m_statep->ਪਾਓ_ਖੰਡ(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
        ਜੇ (nodep->fvarp())
            nodep->fvarp()->v3warn(E_UNSUPPORTED,
                                   "Unsupported: randsequence production function variable");
        // Mark formal ports as port-checked so the primary resolve pass does not
        // flag them with "does not appear in port list" -- V3RandSequence will
        // later move them onto the generated task as real input ports.
        ਲਈ (Ast_ਗੰਢ* itp = nodep->portsp(); itp; itp = itp->nextp()) {
            VN_AS(itp, Var)->user4(ਸੱਚ);
        }
        iterateChildren(nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(AstWithParse* nodep) override {  // FindVisitor::
        // Change WITHPARSE(FUNCREF, equation) to FUNCREF(WITH(equation))
        Ast_ਗੰਢ_FTask_Ref* funcrefp = VN_CAST(nodep->funcrefp(), ਗੰਢ_FTask_Ref);
        ਜੇ (ਅਚਲ AstDot* ਅਚਲ dotp = VN_CAST(nodep->funcrefp(), Dot))
            funcrefp = VN_CAST(dotp->rhsp(), ਗੰਢ_FTask_Ref);
        UASSERT_OBJ(funcrefp, nodep, "'with' only can operate on a function/task");
        string name = funcrefp->name() == "randomize" ? "__Vrandwith_obj" : "item";
        File_ਸਤਰ* argFl = nodep->fileline();
        AstArg* ਅਚਲ argsp = funcrefp->argsp();
        ਜੇ (argsp) {
            argsp->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ();
            ਜੇ (funcrefp->name() != "randomize") {
                ਜੇ (ਅਚਲ ਸਵੈਚਾਲਿਤ parserefp = VN_CAST(argsp->exprp(), ParseRef)) {
                    name = parserefp->name();
                    argFl = parserefp->fileline();
                } ਨਹੀਂਤਾਂ {
                    argsp->v3error("'with' function expects simple variable name");
                }
                ਜੇ (argsp->nextp()) {
                    argsp->nextp()->v3error("'with' function expects only up to one argument");
                }
                VL_DO_DANGLING(argsp->delete_ਰੁੱਖ(), argsp);
            }
        }
        // Type depends on the method used, let V3Width figure it out later
        // 'with (...) { ... }' constraint_block form is randomize-only
        // (IEEE 1800-2023 7.12 vs. 18.7); array methods take 'with (expr)'.
        ਜੇ (nodep->restricted() && funcrefp->name() != "randomize") {
            nodep->v3error("'with (...) { ... }' constraint block is only valid for"
                           " randomize() (IEEE 1800-2023 18.7)");
            funcrefp->ਜੋੜੋ_Argsp(argsp);
            nodep->replaceWith(nodep->funcrefp()->unlink_Fr_ਪਿੱਠ());
            VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
            ਵਾਪਸ;
        }
        ਅਚਲ ਤਾਰਕਿਕ restrictedRandomize = nodep->restricted();
        ਜੇ (nodep->exprsp() || nodep->constraintsp()
            || restrictedRandomize) {  // Else empty expression and pretend no "with"
            AstLambdaArgRef* ਅਚਲ ਸੂਚਕਾਂਕ_Arg_Refp
                = ਨਵਾਂ AstLambdaArgRef{argFl, name + "__DOT__index", ਸੱਚ};
            AstLambdaArgRef* ਅਚਲ ਮੁੱਲ_Arg_Refp = ਨਵਾਂ AstLambdaArgRef{argFl, name, ਝੂਠ};
            AstWith* ਅਚਲ newp
                = ਨਵਾਂ AstWith{nodep->fileline(), ਸੂਚਕਾਂਕ_Arg_Refp, ਮੁੱਲ_Arg_Refp, ਖਾਲੀ_ਸੂਚਕ};
            // Harvest the identifier_list into AstWith; grammar guarantees AstParseRef.
            ਜੇ (restrictedRandomize) {
                newp->restricted(ਸੱਚ);
                ਜਦੋਂ (Ast_ਗੰਢ* ਅਚਲ itemp = nodep->exprsp()) {
                    newp->ਜੋੜੋ_Restricted_Name(VN_AS(itemp, ParseRef)->name());
                    itemp->unlink_Fr_ਪਿੱਠ();
                    VL_DO_DANGLING(ਧੱਕੋ_Deletep(itemp), itemp);
                }
            }
            Ast_ਗੰਢ* ਵਿਅੰਜਕ_Or_Constraintsp = ਖਾਲੀ_ਸੂਚਕ;
            ਜੇ (nodep->exprsp()) ਵਿਅੰਜਕ_Or_Constraintsp = nodep->exprsp()->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ();
            ਜੇ (nodep->constraintsp())
                ਵਿਅੰਜਕ_Or_Constraintsp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(
                    ਵਿਅੰਜਕ_Or_Constraintsp, nodep->constraintsp()->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ());
            newp->ਜੋੜੋ_Exprp(ਵਿਅੰਜਕ_Or_Constraintsp);  // addExprp() tolerates nullptr
            funcrefp->withp(newp);
        }
        funcrefp->ਜੋੜੋ_Argsp(argsp);
        nodep->replaceWith(nodep->funcrefp()->unlink_Fr_ਪਿੱਠ());
        VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstWith* nodep) override {  // FindVisitor::
        // Symbol table needs nodep->name() as the index variable's name
        // Iteration will pickup the AstVar we made under AstWith
        VL_RESTORER(m_curSymp);
        ++m_mod_With_ਅੰਕ;
        m_curSymp = m_statep->ਪਾਓ_ਖੰਡ(m_curSymp, "__Vwith" + cvt_ਮੰਜ਼ਿਲ_Str(m_mod_With_ਅੰਕ), nodep,
                                          m_classOrPackagep);
        m_curSymp->fallbackp(VL_RESTORER_PREV(m_curSymp));
        UASSERT_OBJ(nodep->ਸੂਚਕਾਂਕ_Arg_Refp(), nodep, "Missing lambda argref");
        UASSERT_OBJ(nodep->ਮੁੱਲ_Arg_Refp(), nodep, "Missing lambda argref");
        // Insert argref's name into symbol table
        m_statep->ਪਾਓ_Sym(m_curSymp, nodep->ਮੁੱਲ_Arg_Refp()->name(), nodep->ਮੁੱਲ_Arg_Refp(),
                            ਖਾਲੀ_ਸੂਚਕ);
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstCoverWith* nodep) override {  // FindVisitor::
        // The range list is outside the scope of the filter, which declares 'item'
        iterate_And_ਅਗਲਾ_Null(nodep->subp());
        VL_RESTORER(m_curSymp);
        ++m_mod_With_ਅੰਕ;
        m_curSymp = m_statep->ਪਾਓ_ਖੰਡ(m_curSymp, "__VcoverWith" + cvt_ਮੰਜ਼ਿਲ_Str(m_mod_With_ਅੰਕ),
                                          nodep, m_classOrPackagep);
        m_curSymp->fallbackp(VL_RESTORER_PREV(m_curSymp));
        iterate_And_ਅਗਲਾ_Null(nodep->itemp());
        iterate_And_ਅਗਲਾ_Null(nodep->filterp());
    }
    ਖਾਲੀ ਦੇਖੋ(AstCoverCrossWith* nodep) override {  // FindVisitor::
        // The selection, the 'matches' count, and the cross items' types, of their coverpoints'
        // expressions with any nested scopes, are outside the filter's scope, which declares the
        // items.  As visit(AstVar) would resolve a type in the scope declaring its variable, the
        // items, generated and unique, are inserted directly.
        iterate_And_ਅਗਲਾ_Null(nodep->selectp());
        iterate_And_ਅਗਲਾ_Null(nodep->countp());
        ਲਈ (AstVar* varp = nodep->varsp(); varp; varp = VN_AS(varp->nextp(), Var)) {
            iterate_And_ਅਗਲਾ_Null(varp->ਬੱਚਾ_DTypep());
        }
        VL_RESTORER(m_curSymp);
        ++m_mod_With_ਅੰਕ;
        m_curSymp = m_statep->ਪਾਓ_ਖੰਡ(m_curSymp, "__VcoverWith" + cvt_ਮੰਜ਼ਿਲ_Str(m_mod_With_ਅੰਕ),
                                          nodep, m_classOrPackagep);
        m_curSymp->fallbackp(VL_RESTORER_PREV(m_curSymp));
        ਲਈ (AstVar* varp = nodep->varsp(); varp; varp = VN_AS(varp->nextp(), Var)) {
            m_statep->ਪਾਓ_Sym(m_curSymp, varp->name(), varp, m_classOrPackagep);
        }
        iterate_And_ਅਗਲਾ_Null(nodep->filterp());
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { iterateChildren(nodep); }  // FindVisitor::

    ਖਾਲੀ handleUnvisitedVirtIfaces() {
        Ast_ਗੰਢ_Module* ਅਚਲ ਸਿਖਰ = v3Global.rootp()->modulesp();
        m_curSymp = m_statep->get_ਗੰਢ_Sym(ਸਿਖਰ);
        ਲਈ (AstIface* ifacep : m_virtIfaces) {
            ਜੇ (!ifacep->user4()) {  // virtual interface never assigned any actual interface
                // Create dummy cell to keep the virtual interface alive
                // (later stages assume that non-dead interface has associated cell)
                AstCell* ਅਚਲ ifacecellp = ਨਵਾਂ AstCell{ifacep->fileline(),
                                                        ifacep->fileline(),
                                                        "__VdummyInstOf" + ifacep->name(),
                                                        ifacep->name(),
                                                        ਖਾਲੀ_ਸੂਚਕ,
                                                        ਖਾਲੀ_ਸੂਚਕ,
                                                        ਖਾਲੀ_ਸੂਚਕ};
                ifacecellp->modp(ifacep);
                ਸਿਖਰ->ਜੋੜੋ_Stmtsp(ifacecellp);

                iterate(ifacecellp);
            }
        }
    }

ਜਨਤਕ:
    // CONSTRUCTORS
    Link_Dot_ਲੱਭੋ_Visitor(AstNetlist* rootp, LinkDotState* statep)
        : m_statep{statep} {
        UINFO(4, __FUNCTION__ << ": ");
        iterate(rootp);

        ਜੇ (!m_virtIfaces.ਖਾਲੀ_ਥਾਵਾਂ()) handleUnvisitedVirtIfaces();
        ਲਈ (AstScope* ਅਚਲ scopep : m_scopes) scopep->modp()->ਜੋੜੋ_Stmtsp(scopep);
        ਜੇ (m_statep->forScopeCreation()) {
            V3Stats::ਜੋੜੋ_Stat("LinkDot, Scope visits", m_statScopeVisits);
        }
    }
    ~Link_Dot_ਲੱਭੋ_Visitor() override = ਮੂਲ;
};

//======================================================================

ਵਰਗ Link_Dot_ਲੱਭੋ_Iface_Visitor final : ਜਨਤਕ VNVisitor {
    // NODE STATE
    //  *::user1p()             -> See LinkDotState

    // STATE - for current visit position (use VL_RESTORER)
    LinkDotState* ਅਚਲ m_statep;  // State to pass between visitors, including symbol table
    Ast_ਗੰਢ* m_declp = ਖਾਲੀ_ਸੂਚਕ;  // Current declaring object that may soon contain IfaceRefDType

    // METHODS
    ਅਚਲ VSymEnt* ਲੱਭੋ_Non_Decl_Sym(VSymEnt* symp, ਅਚਲ string& name) {
        // Find if there is a symbol of given name, ignoring the node that is declaring
        // it (m_declp) itself.  Thus if searching for "ifc" (an interface):
        //
        // module x;  // Finishes here and finds the typedef
        //   typedef foo ifc;
        //   ifc ifc;   // symp starts pointing here, but matches m_declp
        ਜਦੋਂ (symp) {
            ਅਚਲ VSymEnt* ਅਚਲ foundp = symp->ਲੱਭੋ_Id_Flat(name);
            ਜੇ (foundp && foundp->nodep() != m_declp) ਵਾਪਸ foundp;
            symp = symp->fallbackp();
        }
        ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
    }

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(AstRefDType* nodep) override {  // FindIfaceVisitor::
        ਜੇ (m_statep->forPrimary() && !nodep->classOrPackagep()) {
            UINFO(9, "  FindIfc: " << nodep);
            // If under a var, ignore the var itself as might be e.g. "intf intf;"
            // Critical tests:
            //   t_interface_param_genblk.v  // Checks this does make interface
            //   t_interface_hidden.v  // Checks this doesn't making interface when hidden
            ਜੇ (m_statep->exists_ਗੰਢ_Sym(nodep)) {
                VSymEnt* symp = m_statep->get_ਗੰਢ_Sym(nodep);
                ਅਚਲ VSymEnt* foundp = ਲੱਭੋ_Non_Decl_Sym(symp, nodep->name());
                Ast_ਗੰਢ* ਮਿਲਿਆ_Nodep = ਖਾਲੀ_ਸੂਚਕ;
                // This: v4make test_regress/t/t_interface_param_genblk.py --debug
                // --debugi-V3LinkDot 9 Passes with this commented out:
                ਜੇ (foundp) ਮਿਲਿਆ_Nodep = foundp->nodep();
                ਜੇ (!ਮਿਲਿਆ_Nodep) ਮਿਲਿਆ_Nodep = m_statep->ਲੱਭੋ_Module_Sym(nodep->name());
                ਜੇ (ਮਿਲਿਆ_Nodep) UINFO(9, "    Ifc foundNodep " << ਮਿਲਿਆ_Nodep);
                ਜੇ (AstIface* ਅਚਲ defp = VN_CAST(ਮਿਲਿਆ_Nodep, Iface)) {
                    // Must be found as module name, and not hidden/ by normal symbol (foundp)
                    AstIfaceRefDType* ਅਚਲ newp
                        = ਨਵਾਂ AstIfaceRefDType{nodep->fileline(), "", nodep->name()};
                    ਜੇ (nodep->paramsp())
                        newp->ਜੋੜੋ_Paramsp(nodep->paramsp()->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ());
                    newp->ifacep(defp);
                    newp->user1u(nodep->user1u());
                    UINFO(9, "    Resolved interface " << nodep << "  =>  " << defp);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
                    ਵਾਪਸ;
                }
            }
        }
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstVar* nodep) override {  // FindIfaceVisitor::
        VL_RESTORER(m_declp);
        m_declp = nodep;
        iterateChildren(nodep);
        AstIfaceRefDType* ਅਚਲ ifacerefp = LinkDotState::ifaceRefFromArray(nodep->subDTypep());
        ਜੇ (ifacerefp && m_statep->exists_ਗੰਢ_Sym(nodep)) {
            // Can't resolve until interfaces and modport names are
            // known; see notes at top
            UINFO(9, "  FindIfc Var IfaceRef " << ifacerefp);
            ਜੇ (!ifacerefp->isVirtual()) nodep->ਸੈੱਟ_Iface_Ref();
            VSymEnt* ਅਚਲ symp = m_statep->get_ਗੰਢ_Sym(nodep);
            m_statep->ਪਾਓ_Iface_Var_Sym(symp);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { iterateChildren(nodep); }  // FindIfaceVisitor::

ਜਨਤਕ:
    // CONSTRUCTORS
    Link_Dot_ਲੱਭੋ_Iface_Visitor(AstNetlist* rootp, LinkDotState* statep)
        : m_statep{statep} {
        UINFO(4, __FUNCTION__ << ": ");
        iterate(rootp);
    }
    ~Link_Dot_ਲੱਭੋ_Iface_Visitor() override = ਮੂਲ;
};

//======================================================================

ਵਰਗ LinkDotParamVisitor final : ਜਨਤਕ VNVisitor {
    // NODE STATE
    //  *::user1p()             -> See LinkDotState
    //  *::user2p()             -> See LinkDotState
    //  *::user4()              -> See LinkDotState

    // STATE
    LinkDotState* ਅਚਲ m_statep;  // State to pass between visitors, including symbol table
    Ast_ਗੰਢ_Module* m_modp = ਖਾਲੀ_ਸੂਚਕ;  // Current module

    ਖਾਲੀ pin_Implicit_ਵਿਅੰਜਕ_Recurse(Ast_ਗੰਢ* nodep) {
        // Under a pin, Check interconnect expression for a pin reference or a concat.
        // Create implicit variable as needed
        ਜੇ (VN_IS(nodep, Dot)) {  // Not creating a simple implied type,
            // and implying something else would just confuse later errors
        } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(nodep, VarRef) || VN_IS(nodep, ParseRef)) {
            // To prevent user errors, we should only do single bit
            // implicit vars, however some netlists (MIPS) expect single
            // bit implicit wires to get created with range 0:0 etc.
            m_statep->implicit_Ok_ਜੋੜੋ(m_modp, nodep->name());
        }
        // These are perhaps a little too generous, as a SELect of siga[sigb]
        // perhaps shouldn't create an implicit variable.  But, we've warned...
        ਨਹੀਂਤਾਂ {
            ਜੇ (Ast_ਗੰਢ* ਅਚਲ refp = nodep->op1p()) pin_Implicit_ਵਿਅੰਜਕ_Recurse(refp);
            ਜੇ (Ast_ਗੰਢ* ਅਚਲ refp = nodep->op2p()) pin_Implicit_ਵਿਅੰਜਕ_Recurse(refp);
            ਜੇ (Ast_ਗੰਢ* ਅਚਲ refp = nodep->op3p()) pin_Implicit_ਵਿਅੰਜਕ_Recurse(refp);
            ਜੇ (Ast_ਗੰਢ* ਅਚਲ refp = nodep->op4p()) pin_Implicit_ਵਿਅੰਜਕ_Recurse(refp);
        }
        // Continue along a list (e.g. the terminals under AstImplicit), also after a reference
        ਜੇ (Ast_ਗੰਢ* ਅਚਲ refp = nodep->nextp()) pin_Implicit_ਵਿਅੰਜਕ_Recurse(refp);
    }

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(Ast_Type_ਸਾਰਣੀ*) override {}  // ParamVisitor::
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Module* nodep) override {  // ParamVisitor::
        UINFO(5, "   " << nodep);
        ਜੇ ((nodep->dead() || !nodep->user4()) && !nodep->hierParams()) {
            UINFO(4, "Mark dead module " << nodep);
            UASSERT_OBJ(!m_statep->forScopeCreation(), nodep,
                        "Dead module persisted past where should have removed");
            // Don't remove now, because we may have a tree of
            // parameterized modules with VARXREFs into the deleted module
            // region.  V3Dead should cleanup.
            // Downstream visitors up until V3Dead need to check for nodep->dead.
            nodep->dead(ਸੱਚ);
            ਵਾਪਸ;
        }
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstPin* nodep) override {  // ParamVisitor::
        // Pin: Link to submodule's port
        // Deal with implicit definitions - do before Resolve visitor as may
        // be referenced above declaration
        ਜੇ (nodep->exprp()  // Else specifically unconnected pin
            && !nodep->svImplicit()) {  // SV 19.11.3: .name pins don't allow implicit decls
            pin_Implicit_ਵਿਅੰਜਕ_Recurse(nodep->exprp());
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstDefParam* nodep) override {  // ParamVisitor::
        iterateChildren(nodep);
        nodep->v3warn(DEFPARAM, "defparam is deprecated (IEEE 1800-2023 C.4.1)\n"
                                    << nodep->warn_ਹੋਰ()
                                    << "... Suggest use instantiation with #(."
                                    << nodep->prettyName() << "(...etc...))");
        ਤਾਰਕਿਕ hasPartSelect = ਝੂਠ;
        ਅਚਲ string ਰਸਤਾ = ਕੱਢੋ_Dotted_ਰਸਤਾ(nodep->pathp(), hasPartSelect);
        UASSERT_OBJ(!hasPartSelect && !ਰਸਤਾ.ਖਾਲੀ_ਥਾਵਾਂ(), nodep, "Unexpected defparam path shape");
        string baddot;
        VSymEnt* okSymp = ਖਾਲੀ_ਸੂਚਕ;
        VSymEnt* ਅਚਲ foundp = m_statep->ਲੱਭੋ_Dotted(
            nodep->fileline(), m_statep->get_ਗੰਢ_Sym(nodep), ਰਸਤਾ, baddot, okSymp, ਸੱਚ);
        AstCell* ਅਚਲ cellp = foundp ? VN_AS(foundp->nodep(), Cell) : ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (!cellp) {
            nodep->v3error("In defparam, instance " << ਰਸਤਾ << " never declared");
        } ਨਹੀਂਤਾਂ {
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ exprp = nodep->rhsp()->unlink_Fr_ਪਿੱਠ();
            UINFO(9, "Defparam cell " << ਰਸਤਾ << "." << nodep->name() << " attach-to " << cellp
                                      << "  <= " << exprp);
            // Don't need to check the name of the defparam exists.  V3Param does.
            AstPin* ਅਚਲ pinp = ਨਵਾਂ AstPin{nodep->fileline(),
                                            -1,  // Pin# not relevant
                                            nodep->name(), exprp};
            pinp->param(ਸੱਚ);
            pinp->param_ਰਸਤਾ(ਰਸਤਾ);
            cellp->ਜੋੜੋ_Paramsp(pinp);
        }
        VL_DO_DANGLING(nodep->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ(), nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਪੋਰਟ* nodep) override {  // ParamVisitor::
        // Port: Stash the pin number
        // Need to set pin numbers after varnames are created
        // But before we do the final resolution based on names
        VSymEnt* ਅਚਲ foundp = m_statep->get_ਗੰਢ_Sym(m_modp)->ਲੱਭੋ_Id_Flat(nodep->name());
        AstVar* ਅਚਲ refp = foundp ? VN_CAST(foundp->nodep(), Var) : ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (!foundp) {
            nodep->v3error(
                "Input/output/inout declaration not found for port: " << nodep->prettyNameQ());
        } ਨਹੀਂਤਾਂ ਜੇ (!refp || (!refp->isIO() && !refp->isIfaceRef())) {
            nodep->v3error("Pin is not an in/out/inout/interface: " << nodep->prettyNameQ());
        } ਨਹੀਂਤਾਂ {
            ਜੇ (refp->user4()) {
                nodep->v3error("Duplicate declaration of port: "
                               << nodep->prettyNameQ() << '\n'
                               << nodep->warnContextPrimary() << '\n'
                               << refp->warnOther() << "... Location of original declaration\n"
                               << refp->warnContextSecondary());
            }
            refp->user4(ਸੱਚ);
            VSymEnt* ਅਚਲ symp = m_statep->ਪਾਓ_Sym(m_statep->get_ਗੰਢ_Sym(m_modp),
                                                      "__pinNumber" + cvt_ਮੰਜ਼ਿਲ_Str(nodep->pin_ਅੰਕ()),
                                                      refp, ਖਾਲੀ_ਸੂਚਕ /*classOrPackagep*/);
            symp->exported(ਝੂਠ);
            refp->pin_ਅੰਕ(nodep->pin_ਅੰਕ());
            // Put the variable where the port is, so that variables stay
            // in pin number sorted order. Otherwise hierarchical or JSON
            // may botch by-position instances.
            nodep->ਜੋੜੋ_ਇੱਥੇ_This_As_ਅਗਲਾ(refp->unlink_Fr_ਪਿੱਠ());
        }
        // Ports not needed any more
        VL_DO_DANGLING(nodep->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ(), nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਸੌਂਪੋ_W* nodep) override {  // ParamVisitor::
        // Deal with implicit definitions
        // We used to nodep->allowImplicit() here, but it turns out
        // normal "assigns" can also make implicit wires.  Yuk.
        pin_Implicit_ਵਿਅੰਜਕ_Recurse(nodep->lhsp());
        iterateChildren(nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(AstImplicit* nodep) override {  // ParamVisitor::
        // Unsupported gates need implicit creation
        pin_Implicit_ਵਿਅੰਜਕ_Recurse(nodep->exprsp());
        // We're done with implicit gates
        VL_DO_DANGLING(nodep->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ(), nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstClassOrPackageRef* nodep) override {  // ParamVisitor::
        ਜੇ (ਸਵੈਚਾਲਿਤ* ਅਚਲ fwdp = VN_CAST(nodep->classOrPackageNodep(), TypedefFwd)) {
            // Relink forward definitions to the "real" definition
            VSymEnt* ਅਚਲ foundp = m_statep->get_ਗੰਢ_Sym(fwdp)->ਲੱਭੋ_Id_Fallback(fwdp->name());
            ਜੇ (foundp && (VN_IS(foundp->nodep(), Class) || VN_IS(foundp->nodep(), Package))) {
                nodep->classOrPackagep(VN_AS(foundp->nodep(), ਗੰਢ_Module));
            } ਨਹੀਂਤਾਂ ਜੇ (foundp && VN_IS(foundp->nodep(), ParamTypeDType)) {
                UASSERT(m_statep->forPrimary(), "Param types should have been resolved");
                nodep->classOrPackageNodep(VN_AS(foundp->nodep(), ParamTypeDType));
            } ਨਹੀਂਤਾਂ {
                ਜੇ (foundp) UINFO(1, "found nodep = " << foundp->nodep());
                nodep->v3error(
                    "Forward typedef used as class/package does not resolve to class/package: "
                    << nodep->prettyNameQ() << '\n'
                    << nodep->warnContextPrimary() << '\n'
                    << (foundp ? nodep->warn_ਹੋਰ() + "... Object with matching name\n"
                                     + foundp->nodep()->warnContextSecondary()
                               : ""));
            }
        }
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstPull* nodep) override {  // ParamVisitor::
        // Deal with implicit definitions
        // We used to nodep->allowImplicit() here, but it turns out
        // normal "assigns" can also make implicit wires.  Yuk.
        pin_Implicit_ਵਿਅੰਜਕ_Recurse(nodep->lhsp());
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstTypedefFwd* nodep) override {  // ParamVisitor::
        VSymEnt* ਅਚਲ foundp = m_statep->get_ਗੰਢ_Sym(nodep)->ਲੱਭੋ_Id_Fallback(nodep->name());
        ਜੇ (foundp) {
            // If the typedef was earlier in source order (tokenNum), then remember that earlier
            // point to avoid error something wasn't declared
            // Might be forward declaring something odd (with declTokenNumSetMin not implemented,
            // but should be harmless to ignore as this is just for error detection
            foundp->nodep()->decl_Token_ਅੰਕ_ਸੈੱਟ_Min(nodep->fileline()->token_ਅੰਕ());
        } ਨਹੀਂਤਾਂ ਜੇ (v3Global.opt.pedantic()) {
            // We only check it was ever really defined in pedantic mode, as it
            // might have been in a header file referring to a module we never
            // needed so there are false positives
            nodep->v3error(
                "Forward typedef unused or does not resolve to a data type (IEEE 1800-2023 6.18): "
                << nodep->prettyNameQ());
        }
        // We only needed the forward declaration in order to parse correctly.
        // Delete later as may be ClassOrPackageRef's still pointing to it
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep->unlink_Fr_ਪਿੱਠ()), nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { iterateChildren(nodep); }  // ParamVisitor::

ਜਨਤਕ:
    // CONSTRUCTORS
    LinkDotParamVisitor(AstNetlist* rootp, LinkDotState* statep)
        : m_statep{statep} {
        UINFO(4, __FUNCTION__ << ": ");
        iterate(rootp);
    }
    ~LinkDotParamVisitor() override = ਮੂਲ;
};

//======================================================================

ਵਰਗ LinkDotScopeVisitor final : ਜਨਤਕ VNVisitor {

    // STATE
    LinkDotState* ਅਚਲ m_statep;  // State to pass between visitors, including symbol table
    ਅਚਲ AstScope* m_scopep = ਖਾਲੀ_ਸੂਚਕ;  // The current scope
    VSymEnt* m_modSymp = ਖਾਲੀ_ਸੂਚਕ;  // Symbol entry for current module
    // Deferred AliasScope processing - must be done outer-to-inner for correct alias resolution
    std::ਸਦਿਸ਼<std::pair<AstAliasScope*, VSymEnt*>> m_deferredAliasScopes;

    // METHODS
ਜਨਤਕ:
    // getAlias with setVarAlias and setIfaceAlias implement disjoint-set data structure.
    // This algorithm is needed for the case when multiple alias statements
    // reference partially the same variables.
    ਸਥਿਰ AstVarScope* getAlias(AstVarScope* ਅਚਲ vscp) {
        ਜੇ (vscp->user2p() && vscp != vscp->user2p()) {
            AstVarScope* ਅਚਲ aliasp = getAlias(VN_AS(vscp->user2p(), VarScope));
            vscp->user2p(aliasp);
            ਵਾਪਸ aliasp;
        } ਨਹੀਂਤਾਂ {
            ਵਾਪਸ vscp;
        }
    }

ਨਿਜੀ:
    // Alias an interface reference to the interface it is connected to
    ਖਾਲੀ ਸੈੱਟ_Iface_Alias(AstVarScope* ਅਚਲ vscp, AstVarScope* ਅਚਲ aliasp) {
        UASSERT_OBJ(vscp->varp()->isIfaceRef(), vscp, "Interface alias of non-interface");
        UASSERT_OBJ(aliasp->varp()->isIfaceRef(), aliasp, "Interface alias of non-interface");
        getAlias(vscp)->user2p(getAlias(aliasp));
    }

    // Alias two variables. An output port in the lower scope survives, unless the other
    // must keep its own storage.
    ਖਾਲੀ ਸੈੱਟ_Var_Alias(AstVarScope* ap, AstVarScope* bp) {
        UASSERT_OBJ(!ap->varp()->isIfaceRef(), ap, "Variable alias of interface");
        UASSERT_OBJ(!bp->varp()->isIfaceRef(), bp, "Variable alias of interface");
        ap = getAlias(ap);
        bp = getAlias(bp);
        ਜੇ (ap == bp) ਵਾਪਸ;
        ਅਚਲ ਤਾਰਕਿਕ aWins = [&]() {
            ਅਚਲ AstVar* ਅਚਲ aVarp = ap->varp();
            ਅਚਲ AstVar* ਅਚਲ bVarp = bp->varp();
            // A primary IO is accessed directly by the user of the model, so must survive
            UASSERT_OBJ(!aVarp->isPrimaryIO() || !bVarp->isPrimaryIO(), ap,
                        "Alias of two primary IOs");
            ਜੇ (bVarp->isPrimaryIO()) ਵਾਪਸ ਝੂਠ;
            ਜੇ (aVarp->isPrimaryIO()) ਵਾਪਸ ਸੱਚ;
            // A forced or public variable must keep its own storage. If both are, either works.
            ਜੇ (aVarp->isForced() != bVarp->isForced()) ਵਾਪਸ aVarp->isForced();
            ਜੇ (aVarp->is_Sig_ਜਨਤਕ() != bVarp->is_Sig_ਜਨਤਕ()) ਵਾਪਸ aVarp->is_Sig_ਜਨਤਕ();
            // Otherwise prefer an output port in the lower scope. This enables better V3Combine
            // due to having fewer upward hierarchical references (which prevent combining).
            ਜੇ (aVarp->direction() != VDirection::OUTPUT) ਵਾਪਸ ਝੂਠ;
            // Is 'ap' strictly below 'bp' in the hierarchy
            ਲਈ (ਅਚਲ AstScope* scopep = ap->scopep()->aboveScopep(); scopep;
                 scopep = scopep->aboveScopep()) {
                ਜੇ (scopep == bp->scopep()) ਵਾਪਸ ਸੱਚ;
            }
            ਵਾਪਸ ਝੂਠ;
        }();
        ਜੇ (aWins) {
            bp->user2p(ap);
        } ਨਹੀਂਤਾਂ {
            ap->user2p(bp);
        }
    }

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(AstNetlist* nodep) override {  // ScopeVisitor::
        // Recurse..., backward as must do packages before using packages
        iterateChildrenBackwardsConst(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstScope* nodep) override {  // ScopeVisitor::
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
        ਜੇ (AstIface* ਅਚਲ ifacep = VN_CAST(nodep->modp(), Iface)) {
            m_statep->ਪਾਓ_Iface_Mod_Sym(ifacep, m_modSymp);
        }
        m_scopep = nodep;
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstVarScope* nodep) override {  // ScopeVisitor::
        ਜੇ (!nodep->varp()->isFuncLocal() && !nodep->varp()->isClassMember()) {
            VSymEnt* ਅਚਲ varSymp
                = m_statep->ਪਾਓ_Sym(m_modSymp, nodep->varp()->name(), nodep, ਖਾਲੀ_ਸੂਚਕ);
            ਜੇ (nodep->varp()->isIfaceRef()) {
                AstIfaceRefDType* ਅਚਲ dtypep
                    = LinkDotState::ifaceRefFromArray(nodep->varp()->dtypep());
                UASSERT_OBJ(dtypep, nodep, "Non AstIfaceRefDType on isIfaceRef() var");
                ਜੇ (nodep->varp()->is_Iface_ਮਾਪਾ()) {
                    UINFO(9, "Iface parent ref var " << nodep->varp()->name() << " " << nodep);
                    // Find the interface cell the var references
                    UINFO(9, "Iface parent dtype " << dtypep);
                    ਅਚਲ string ifcellname = dtypep->cellName();
                    string baddot;
                    VSymEnt* okSymp;
                    VSymEnt* cellSymp = m_statep->ਲੱਭੋ_Dotted(nodep->fileline(), m_modSymp,
                                                             ifcellname, baddot, okSymp, ਝੂਠ);
                    UASSERT_OBJ(
                        cellSymp, nodep,
                        "No symbol for interface instance: " << nodep->prettyNameQ(ifcellname));
                    UINFO(5, "       Found interface instance: se" << cvt_ਮੰਜ਼ਿਲ_Hex(cellSymp) << " "
                                                                   << cellSymp->nodep());
                    ਜੇ (dtypep->modportName() != "") {
                        // Look up the modport within the interface cell's symbol table
                        VSymEnt* ਅਚਲ mpSymp = cellSymp->ਲੱਭੋ_Id_Fallback(dtypep->modportName());
                        UASSERT_OBJ(mpSymp, nodep,
                                    "No symbol for interface modport: "
                                        << nodep->prettyNameQ(dtypep->modportName()));
                        cellSymp = mpSymp;
                        UINFO(5, "       Found modport cell: se" << cvt_ਮੰਜ਼ਿਲ_Hex(cellSymp) << " "
                                                                 << mpSymp->nodep());
                    }
                    // Interface reference; need to put whole thing into
                    // symtable, but can't clone it now as we may have a later
                    // alias for it.
                    m_statep->ਪਾਓ_Scope_Alias(LinkDotState::SAMN_IFTOP, varSymp, cellSymp);
                }
            }
        }
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_FTask* nodep) override {  // ScopeVisitor::
        VSymEnt* ਅਚਲ symp = m_statep->ਪਾਓ_ਖੰਡ(m_modSymp, nodep->name(), nodep, ਖਾਲੀ_ਸੂਚਕ);
        symp->fallbackp(m_modSymp);
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Foreach* nodep) override {  // ScopeVisitor::
        VSymEnt* ਅਚਲ symp = m_statep->ਪਾਓ_ਖੰਡ(m_modSymp, nodep->name(), nodep, ਖਾਲੀ_ਸੂਚਕ);
        symp->fallbackp(m_modSymp);
        // No recursion, we don't want to pick up variables
    }
    ਖਾਲੀ ਦੇਖੋ(AstConstraintForeach* nodep) override {  // ScopeVisitor::
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstWith* nodep) override {  // ScopeVisitor::
        VSymEnt* ਅਚਲ symp = m_statep->ਪਾਓ_ਖੰਡ(m_modSymp, nodep->name(), nodep, ਖਾਲੀ_ਸੂਚਕ);
        symp->fallbackp(m_modSymp);
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstAlias* nodep) override {  // ScopeVisitor::
        // Track aliases
        UINFOTREE(9, nodep, "", "alias");
        AstVarScope* aliasVscp = ਖਾਲੀ_ਸੂਚਕ;
        ਲਈ (Ast_ਗੰਢ* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
            AstVarScope* vscp = ਖਾਲੀ_ਸੂਚਕ;
            ਜੇ (ਅਚਲ AstVarRef* ਅਚਲ refp = VN_CAST(itemp, VarRef)) {
                vscp = refp->varScopep();
            } ਨਹੀਂਤਾਂ {
                // Reaches into the scope of an instance, look it up by name
                ਅਚਲ AstVarXRef* ਅਚਲ xrefp = VN_AS(itemp, VarXRef);
                ਅਚਲ string scopename = xrefp->dotted() + "." + xrefp->name();
                string baddot;
                VSymEnt* okSymp;
                VSymEnt* ਅਚਲ symp = m_statep->ਲੱਭੋ_Dotted(xrefp->fileline(), m_modSymp, scopename,
                                                           baddot, okSymp, ਝੂਠ);
                UASSERT_OBJ(symp, nodep, "No symbol for alias item: " << scopename);
                vscp = VN_CAST(symp->nodep(), VarScope);
            }
            UASSERT_OBJ(vscp, nodep, "VarScope unset");
            ਜੇ (aliasVscp) {
                ਸੈੱਟ_Var_Alias(aliasVscp, vscp);
            } ਨਹੀਂਤਾਂ {
                aliasVscp = vscp;
            }
        }
        iterateChildren(nodep);
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep->unlink_Fr_ਪਿੱਠ()), nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstAliasScope* nodep) override {  // ScopeVisitor::
        // Defer AliasScope processing - must process outer scopes before inner ones
        // so that nested interface port alias resolution works correctly
        UINFO(5, "ALIASSCOPE (deferred) " << nodep);
        m_deferredAliasScopes.emplace_ਪਿੱਠ(nodep, m_modSymp);
    }
    ਖਾਲੀ processAliasScope(AstAliasScope* nodep, VSymEnt* modSymp) {
        UINFO(5, "ALIASSCOPE  " << nodep);
        UINFOTREE(9, nodep, "", "avs");
        VSymEnt* rhsSymp;
        {
            AstVarRef* ਅਚਲ refp = VN_CAST(nodep->rhsp(), VarRef);
            AstVarXRef* ਅਚਲ xrefp = VN_CAST(nodep->rhsp(), VarXRef);
            UASSERT_OBJ(refp || xrefp, nodep,
                        "Unsupported: Non Var(X)Ref attached to interface pin");
            string inl
                = ((xrefp && xrefp->inlinedDots().ਮਾਪ()) ? (xrefp->inlinedDots() + "__DOT__")
                                                          : "");
            ਅਚਲ string dotted_ਰਸਤਾ
                = (xrefp && !xrefp->dotted().ਖਾਲੀ_ਥਾਵਾਂ()) ? (xrefp->dotted() + ".") : "";
            VSymEnt* symp = ਖਾਲੀ_ਸੂਚਕ;
            string scopename;
            ਜਦੋਂ (!symp) {
                scopename = refp ? refp->name()
                                 : (inl.ਮਾਪ() ? (inl + dotted_ਰਸਤਾ + xrefp->name())
                                               : (dotted_ਰਸਤਾ + xrefp->name()));
                string baddot;
                VSymEnt* okSymp;
                symp = m_statep->ਲੱਭੋ_Dotted(nodep->rhsp()->fileline(), modSymp, scopename, baddot,
                                            okSymp, ਸੱਚ);
                ਜੇ (inl == "") ਰੋਕੋ;
                inl = LinkDotState::remove_ਆਖਰੀ_Inline_Scope(inl);
            }
            UASSERT_OBJ(symp, nodep, "No symbol for interface alias rhs");
            UINFO(5, "       Found a linked scope RHS: " << scopename << "  se" << cvt_ਮੰਜ਼ਿਲ_Hex(symp)
                                                         << " " << symp->nodep());
            rhsSymp = symp;
        }
        VSymEnt* lhsSymp;
        {
            ਅਚਲ AstVarXRef* ਅਚਲ xrefp = VN_CAST(nodep->lhsp(), VarXRef);
            ਅਚਲ AstVarRef* ਅਚਲ refp = VN_CAST(nodep->lhsp(), VarRef);

            UASSERT_OBJ(refp || xrefp, nodep,
                        "Unsupported: Non Var(X)Ref attached to interface pin");
            ਅਚਲ string scopename
                = refp ? refp->varp()->name() : xrefp->dotted() + "." + xrefp->name();
            string baddot;
            VSymEnt* okSymp;
            VSymEnt* ਅਚਲ symp = m_statep->ਲੱਭੋ_Dotted(nodep->lhsp()->fileline(), modSymp,
                                                       scopename, baddot, okSymp, ਝੂਠ);
            UASSERT_OBJ(symp, nodep, "No symbol for interface alias lhs");
            UINFO(5, "       Found a linked scope LHS: " << scopename << "  se" << cvt_ਮੰਜ਼ਿਲ_Hex(symp)
                                                         << " " << symp->nodep());
            lhsSymp = symp;
        }
        // Remember the alias - can't do it yet because we may have additional symbols to be added,
        // or maybe an alias of an alias
        m_statep->ਪਾਓ_Scope_Alias(LinkDotState::SAMN_IFTOP, lhsSymp, rhsSymp);
        AstVarScope* ਅਚਲ lhsVscp = VN_CAST(lhsSymp->nodep(), VarScope);
        AstVarScope* ਅਚਲ rhsVscp = VN_CAST(rhsSymp->nodep(), VarScope);
        UASSERT_OBJ(lhsVscp && rhsVscp, nodep, "Interface alias missing variable scope");
        ਸੈੱਟ_Iface_Alias(lhsVscp, rhsVscp);
        // We have stored the link, we don't need these any more
        VL_DO_DANGLING(nodep->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ(), nodep);
    }
    ਖਾਲੀ processDeferredAliasScopes() {
        // Sort by hierarchy depth (shallower first) so outer aliases are resolved before inner
        // Pre-compute depth map to avoid O(N log N * D) complexity in sort comparisons
        std::unordered_map<VSymEnt*, ਪੂਰਨ> ਡੂੰਘਾਈ_Map;
        ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& pair : m_deferredAliasScopes) {
            VSymEnt* ਅਚਲ symp = pair.ਦੂਜਾ;
            ਜੇ (ਡੂੰਘਾਈ_Map.ਲੱਭੋ(symp) == ਡੂੰਘਾਈ_Map.ਸਿਰਾ()) {
                ਪੂਰਨ ਡੂੰਘਾਈ = 0;
                ਲਈ (VSymEnt* p = symp; p; p = p->parentp()) ++ਡੂੰਘਾਈ;
                ਡੂੰਘਾਈ_Map[symp] = ਡੂੰਘਾਈ;
            }
        }
        std::stable_ਛਾਂਟੋ(m_deferredAliasScopes.begin(), m_deferredAliasScopes.ਸਿਰਾ(),
                         [&ਡੂੰਘਾਈ_Map](ਅਚਲ std::pair<AstAliasScope*, VSymEnt*>& a,
                                     ਅਚਲ std::pair<AstAliasScope*, VSymEnt*>& b) {
                             ਵਾਪਸ ਡੂੰਘਾਈ_Map.at(a.ਦੂਜਾ) < ਡੂੰਘਾਈ_Map.at(b.ਦੂਜਾ);
                         });
        // Process in sorted order
        ਲਈ (ਸਵੈਚਾਲਿਤ& pair : m_deferredAliasScopes) { processAliasScope(pair.ਪਹਿਲਾ, pair.ਦੂਜਾ); }
        m_deferredAliasScopes.clear();
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Gen* nodep) override {  // ScopeVisitor::  // LCOV_EXCL_LINE
        nodep->v3fatalSrc("Generate constructs should have been reduced out");
    }
    // For speed, don't recurse things that can't have scope
    // Note we allow AstNodeStmt's as generates may be under them
    ਖਾਲੀ ਦੇਖੋ(AstCell*) override {}  // ScopeVisitor::
    ਖਾਲੀ ਦੇਖੋ(AstVar*) override {}  // ScopeVisitor::
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { iterateChildren(nodep); }  // ScopeVisitor::

ਜਨਤਕ:
    // CONSTRUCTORS
    LinkDotScopeVisitor(AstNetlist* rootp, LinkDotState* statep)
        : m_statep{statep} {
        UINFO(4, __FUNCTION__ << ": ");
        iterate(rootp);
        // Process deferred AliasScopes in outer-to-inner order
        processDeferredAliasScopes();
    }
    ~LinkDotScopeVisitor() override = ਮੂਲ;
};

//======================================================================

// Iterate an interface to resolve modports
ਵਰਗ LinkDotIfaceVisitor final : ਜਨਤਕ VNVisitor {
    // STATE
    LinkDotState* ਅਚਲ m_statep;  // State to pass between visitors, including symbol table
    VSymEnt* m_curSymp;  // Symbol Entry for current table, where to lookup/insert

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(AstModport* nodep) override {  // IfaceVisitor::
        // Modport: Remember its name for later resolution
        UINFO(5, "   fiv: " << nodep);
        VL_RESTORER(m_curSymp);
        {
            // Create symbol table for the vars
            m_curSymp = m_statep->ਪਾਓ_ਖੰਡ(m_curSymp, nodep->name(), nodep, ਖਾਲੀ_ਸੂਚਕ);
            m_curSymp->fallbackp(VL_RESTORER_PREV(m_curSymp));
            iterateChildren(nodep);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstModportFTaskRef* nodep) override {  // IfaceVisitor::
        UINFO(5, "   fif: " << nodep);
        iterateChildren(nodep);
        VSymEnt* ਅਚਲ symp = m_curSymp->ਲੱਭੋ_Id_Fallback(nodep->name());
        ਜੇ (!symp) {
            nodep->v3error("Modport item not found: " << nodep->prettyNameQ());
        } ਨਹੀਂਤਾਂ ਜੇ (Ast_ਗੰਢ_FTask* ਅਚਲ ftaskp = VN_CAST(symp->nodep(), ਗੰਢ_FTask)) {
            // Make symbol under modport that points at the _interface_'s var, not the modport.
            nodep->ftaskp(ftaskp);
            VSymEnt* ਅਚਲ subSymp
                = m_statep->ਪਾਓ_Sym(m_curSymp, nodep->name(), ftaskp, ਖਾਲੀ_ਸੂਚਕ /*package*/);
            m_statep->ਪਾਓ_Scope_Alias(LinkDotState::SAMN_MODPORT, subSymp, symp);
        } ਨਹੀਂਤਾਂ {
            nodep->v3error("Modport item is not a function/task: " << nodep->prettyNameQ());
        }
        ਜੇ (m_statep->forScopeCreation()) {
            // Done with AstModportFTaskRef.
            // Delete to prevent problems if we dead-delete pointed to ftask
            nodep->unlink_Fr_ਪਿੱਠ();
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
        }
    }

    // Helper to resolve remaining path through a nested interface
    // When findDotted() partially matches (okSymp set, baddot non-empty),
    // this follows the interface type to resolve the remaining path.
    // Returns the resolved symbol, or nullptr if not found.
    // On success, clears baddot; on partial match of multi-level path, updates baddot.
    VSymEnt* resolve_Nested_Interface_ਰਸਤਾ(File_ਸਤਰ* fl, VSymEnt* okSymp, string& baddot) {
        ਜੇ (!okSymp || baddot.ਖਾਲੀ_ਥਾਵਾਂ()) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;

        ਸਥਿਰ constexpr ਪੂਰਨ MAX_NESTING_ਡੂੰਘਾਈ = 64;
        VSymEnt* curOkSymp = okSymp;

        ਲਈ (ਪੂਰਨ ਡੂੰਘਾਈ = 0; ਡੂੰਘਾਈ < MAX_NESTING_ਡੂੰਘਾਈ; ++ਡੂੰਘਾਈ) {
            // Try to get interface from the partially-matched symbol
            AstIface* ifacep = ਖਾਲੀ_ਸੂਚਕ;
            ਜੇ (ਅਚਲ AstCell* ਅਚਲ cellp = VN_CAST(curOkSymp->nodep(), Cell)) {
                ifacep = VN_CAST(cellp->modp(), Iface);
            } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ AstVar* ਅਚਲ varp = VN_CAST(curOkSymp->nodep(), Var)) {
                ਜੇ (varp->isIfaceRef()) {
                    ਜੇ (ਅਚਲ AstIfaceRefDType* ਅਚਲ ifaceRefp
                        = LinkDotState::ifaceRefFromArray(varp->dtypep())) {
                        ifacep = ifaceRefp->iface_ਰਾਹੀਂ_Cellp();
                    }
                }
            }

            ਜੇ (!ifacep || !m_statep->exists_ਗੰਢ_Sym(ifacep)) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;

            VSymEnt* ਅਚਲ ifaceSymp = m_statep->get_ਗੰਢ_Sym(ifacep);

            ਜੇ (baddot.ਲੱਭੋ('.') == string::npos) {
                // Simple identifier - direct lookup
                VSymEnt* ਅਚਲ symp = ifaceSymp->ਲੱਭੋ_Id_Fallback(baddot);
                ਜੇ (symp) baddot.clear();
                ਵਾਪਸ symp;
            }

            // Multi-level path - use findDotted for partial resolution
            string ਬਚਿਆ_Baddot;
            VSymEnt* ਬਚਿਆ_Ok_Symp = ਖਾਲੀ_ਸੂਚਕ;
            VSymEnt* ਅਚਲ symp = m_statep->ਲੱਭੋ_Dotted(fl, ifaceSymp, baddot, ਬਚਿਆ_Baddot,
                                                       ਬਚਿਆ_Ok_Symp, ਸੱਚ);
            ਜੇ (symp) {
                baddot = ਬਚਿਆ_Baddot;
                ਵਾਪਸ symp;
            }

            // findDotted partially matched - check progress
            ਜੇ (ਬਚਿਆ_Baddot == baddot || !ਬਚਿਆ_Ok_Symp) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;

            // Continue resolving with updated state
            baddot = ਬਚਿਆ_Baddot;
            curOkSymp = ਬਚਿਆ_Ok_Symp;
        }

        UINFO(1, "Nested interface resolution depth limit exceeded at " << fl);
        ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
    }

    // Resolve a modport expression to find the referenced symbol
    VSymEnt* resolve_Modport_ਵਿਅੰਜਕ(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp) {
        UINFO(5, "     resolveModportExpression: " << exprp);
        VSymEnt* symp = ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (AstParseRef* ਅਚਲ refp = VN_CAST(exprp, ParseRef)) {
            // Simple variable reference: modport mp(input .a(sig_a))
            symp = m_curSymp->ਲੱਭੋ_Id_Fallback(refp->name());
        } ਨਹੀਂਤਾਂ ਜੇ (AstVarRef* ਅਚਲ refp = VN_CAST(exprp, VarRef)) {
            // Already resolved VarRef (can happen if iterateChildren resolved it)
            ਜੇ (refp->varScopep()) {
                // During scope creation, VarRef may have VarScope already linked
                symp = m_statep->get_ਗੰਢ_Sym(refp->varScopep());
            } ਨਹੀਂਤਾਂ ਜੇ (refp->varp()) {
                symp = m_curSymp->ਲੱਭੋ_Id_Fallback(refp->varp()->name());
            }
        } ਨਹੀਂਤਾਂ ਜੇ (AstVarXRef* ਅਚਲ refp = VN_CAST(exprp, VarXRef)) {
            // Resolved VarXRef (dotted reference resolved by earlier pass)
            ਜੇ (refp->varp()) {
                // For nested interfaces, the var is in a different scope
                // First try to find the symbol via the var itself
                ਜੇ (m_statep->exists_ਗੰਢ_Sym(refp->varp())) {
                    symp = m_statep->get_ਗੰਢ_Sym(refp->varp());
                } ਨਹੀਂਤਾਂ {
                    // Fallback: look up in current scope
                    symp = m_curSymp->ਲੱਭੋ_Id_Fallback(refp->varp()->name());
                }
            } ਨਹੀਂਤਾਂ ਜੇ (!refp->dotted().ਖਾਲੀ_ਥਾਵਾਂ()) {
                // varp not set yet - use dotted path to find the symbol
                // The dotted part (e.g., "base") is the interface cell, and name is the member
                ਅਚਲ string ਭਰਿਆ_ਰਸਤਾ = refp->dotted() + "." + refp->name();
                string baddot;
                VSymEnt* okSymp = ਖਾਲੀ_ਸੂਚਕ;
                symp = m_statep->ਲੱਭੋ_Dotted(fl, m_curSymp, ਭਰਿਆ_ਰਸਤਾ, baddot, okSymp, ਸੱਚ);
                // Handle nested interface path when findDotted had partial match
                ਜੇ (okSymp && !baddot.ਖਾਲੀ_ਥਾਵਾਂ()) {
                    VSymEnt* ਅਚਲ resolved = resolve_Nested_Interface_ਰਸਤਾ(fl, okSymp, baddot);
                    ਜੇ (resolved) symp = resolved;
                }
            }
        } ਨਹੀਂਤਾਂ ਜੇ (AstDot* ਅਚਲ dotp = VN_CAST(exprp, Dot)) {
            // Dotted path: modport mp(input .a(inner.sig))
            ਤਾਰਕਿਕ hasPartSelect = ਝੂਠ;
            ਅਚਲ string dotted_ਰਸਤਾ = ਕੱਢੋ_Dotted_ਰਸਤਾ(dotp, hasPartSelect);
            ਜੇ (hasPartSelect) {
                fl->v3warn(
                    E_UNSUPPORTED,
                    "Unsupported: Modport expression with part select (IEEE 1800-2023 25.5.4)");
            } ਨਹੀਂਤਾਂ {
                string baddot;
                VSymEnt* okSymp = ਖਾਲੀ_ਸੂਚਕ;
                symp = m_statep->ਲੱਭੋ_Dotted(fl, m_curSymp, dotted_ਰਸਤਾ, baddot, okSymp, ਸੱਚ);

                // Handle nested interface path when findDotted had partial match
                ਜੇ (okSymp && !baddot.ਖਾਲੀ_ਥਾਵਾਂ()) {
                    VSymEnt* ਅਚਲ resolved = resolve_Nested_Interface_ਰਸਤਾ(fl, okSymp, baddot);
                    ਜੇ (resolved) symp = resolved;
                }

                ਜੇ (!symp || !baddot.ਖਾਲੀ_ਥਾਵਾਂ()) {
                    fl->v3error("Can't find modport expression target: "
                                << Ast_ਗੰਢ::prettyNameQ(dotted_ਰਸਤਾ));
                    symp = ਖਾਲੀ_ਸੂਚਕ;
                }
            }
        } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(exprp, Sel_ਦੋਆਧਾਰੀ_ਅੰਕ) || VN_IS(exprp, Sel_ਕੱਢੋ)) {
            // Part select expressions not yet supported
            fl->v3warn(E_UNSUPPORTED,
                       "Unsupported: Modport expression with part select (IEEE 1800-2023 25.5.4)");
        } ਨਹੀਂਤਾਂ {
            // Other expression types not supported
            fl->v3warn(E_UNSUPPORTED,
                       "Unsupported: Complex modport expression (IEEE 1800-2023 25.5.4)");
        }
        ਵਾਪਸ symp;
    }

    ਖਾਲੀ ਦੇਖੋ(AstModportVarRef* nodep) override {  // IfaceVisitor::
        UINFO(5, "   fiv: " << nodep);
        iterateChildren(nodep);
        VSymEnt* symp = ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (nodep->exprp()) {
            // Modport expression syntax: modport mp(input .port_name(expression))
            symp = resolve_Modport_ਵਿਅੰਜਕ(nodep->fileline(), nodep->exprp());
        } ਨਹੀਂਤਾਂ {
            symp = m_curSymp->ਲੱਭੋ_Id_Fallback(nodep->name());
        }
        ਜੇ (!symp) {
            nodep->v3error("Modport item not found: " << nodep->prettyNameQ());
        } ਨਹੀਂਤਾਂ ਜੇ (AstVar* ਅਚਲ varp = VN_CAST(symp->nodep(), Var)) {
            // Make symbol under modport that points at the _interface_'s var via the modport.
            // (Need modport still to test input/output markings)
            nodep->varp(varp);
            ਜੇ (nodep->exprp()) {
                // For modport expressions, insert symbol pointing to the underlying var (not the
                // ModportVarRef which will be deleted during scope creation). The virtual port
                // name maps to the real signal's var.
                VSymEnt* ਅਚਲ subSymp
                    = m_statep->ਪਾਓ_Sym(m_curSymp, nodep->name(), varp, ਖਾਲੀ_ਸੂਚਕ /*package*/);
                m_statep->ਪਾਓ_Scope_Alias(LinkDotState::SAMN_MODPORT, subSymp, symp);
            } ਨਹੀਂਤਾਂ {
                // For regular modport items, insert symbol pointing to ModportVarRef for
                // input/output marking tests.
                m_statep->ਪਾਓ_Sym(m_curSymp, nodep->name(), nodep, ਖਾਲੀ_ਸੂਚਕ /*package*/);
            }
        } ਨਹੀਂਤਾਂ ਜੇ (AstVarScope* ਅਚਲ vscp = VN_CAST(symp->nodep(), VarScope)) {
            // Make symbol under modport that points at the _interface_'s var, not the modport.
            nodep->varp(vscp->varp());
            // Only insert symbol for modport expression virtual ports (exprp set).
            // For regular modport items, don't insert into the shared modport table because
            // each instance would overwrite the previous one, causing wrong VarScope lookups.
            // Regular items are found via the modport's fallback to the interface.
            ਜੇ (nodep->exprp()) {
                VSymEnt* ਅਚਲ subSymp
                    = m_statep->ਪਾਓ_Sym(m_curSymp, nodep->name(), vscp, ਖਾਲੀ_ਸੂਚਕ /*package*/);
                m_statep->ਪਾਓ_Scope_Alias(LinkDotState::SAMN_MODPORT, subSymp, symp);
            }
        } ਨਹੀਂਤਾਂ {
            nodep->v3error("Modport item is not a variable: " << nodep->prettyNameQ());
        }
        ਜੇ (m_statep->forScopeCreation()) {
            // Done with AstModportVarRef.
            // Delete to prevent problems if we dead-delete pointed to variable
            nodep->unlink_Fr_ਪਿੱਠ();
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstModportClockingRef* nodep) override {  // IfaceVisitor::
        UINFO(5, "   fic: " << nodep);
        iterateChildren(nodep);
        VSymEnt* ਅਚਲ symp = m_curSymp->ਲੱਭੋ_Id_Fallback(nodep->name());
        ਜੇ (!symp) {
            nodep->v3error("Modport item not found: " << nodep->prettyNameQ());
        } ਨਹੀਂਤਾਂ ਜੇ (AstClocking* ਅਚਲ clockingp = VN_CAST(symp->nodep(), Clocking)) {
            nodep->clockingp(clockingp);
            m_statep->ਪਾਓ_Sym(m_curSymp, nodep->name(), nodep, ਖਾਲੀ_ਸੂਚਕ /*package*/);
        } ਨਹੀਂਤਾਂ {
            nodep->v3error("Modport item is not a clocking block: " << nodep->prettyNameQ());
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstScope* nodep) override {  // IfaceVisitor::
        ਜੇ (nodep->user3_ਸੈੱਟ_Once()) ਵਾਪਸ;
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { iterateChildren(nodep); }  // IfaceVisitor::

ਜਨਤਕ:
    // CONSTRUCTORS
    LinkDotIfaceVisitor(AstIface* nodep, VSymEnt* curSymp, LinkDotState* statep)
        : m_statep{statep}
        , m_curSymp{curSymp} {
        UINFO(4, __FUNCTION__ << ": ");
        iterate(nodep);
    }
    ~LinkDotIfaceVisitor() override = ਮੂਲ;
};

ਖਾਲੀ LinkDotState::computeIfaceModSyms() {
    //  AstScope::user3()  // bool. Already iterated by a LinkDotIfaceVisitor
    ਅਚਲ VNUser3_ਵਿੱਚ_Use user3_ਵਿੱਚ_Use;
    ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& itr : m_ifaceModSyms) {
        AstIface* ਅਚਲ nodep = itr.ਪਹਿਲਾ;
        VSymEnt* ਅਚਲ symp = itr.ਦੂਜਾ;
        LinkDotIfaceVisitor{nodep, symp, ਇਹ};
    }
    m_ifaceModSyms.clear();
}

//======================================================================

ਵਰਗ LinkDotResolveVisitor final : ਜਨਤਕ VNVisitor {
    // NODE STATE
    // Cleared on global
    //  *::user1p()             -> See LinkDotState
    //  *::user2p()             -> See LinkDotState
    //  *::user3()              // bool. Processed
    //  *::user4()              -> See LinkDotState
    ਅਚਲ VNUser3_ਵਿੱਚ_Use m_inuser3;

    // TYPES
    ਗਿਣਤੀ DotPosition : uint8_t {
        // Must match ascii() method below
        DP_NONE = 0,  // Not under a DOT
        DP_PACKAGE,  // {package-or-class}:: DOT
        DP_ਪਹਿਲਾ,  // {scope-or-var} DOT
        DP_SCOPE,  // DOT... {scope-or-var} DOT
        DP_FINAL,  // [DOT...] {var-or-func-or-dtype} with no following dots
        DP_MEMBER  // DOT {member-name} [DOT...]
    };

    // STATE
    LinkDotState* ਅਚਲ m_statep;  // State, including dotted symbol table
    VSymEnt* m_curSymp = ਖਾਲੀ_ਸੂਚਕ;  // SymEnt for current lookup point
    VSymEnt* m_modSymp = ਖਾਲੀ_ਸੂਚਕ;  // SymEnt for current module
    VSymEnt* m_pinSymp = ਖਾਲੀ_ਸੂਚਕ;  // SymEnt for pin lookups
    VSymEnt* m_randSymp = ਖਾਲੀ_ਸੂਚਕ;  // SymEnt for randomize target class's lookups
    ਅਚਲ AstCell* m_cellp = ਖਾਲੀ_ਸੂਚਕ;  // Current cell
    Ast_ਗੰਢ_Module* m_modp = ਖਾਲੀ_ਸੂਚਕ;  // Current module
    Ast_ਗੰਢ_FTask* m_ftaskp = ਖਾਲੀ_ਸੂਚਕ;  // Current function/task
    Ast_ਵਿਧੀ_Call* m_rand_ਵਿਧੀ_Callp = ਖਾਲੀ_ਸੂਚਕ;  // Current randomize() method call
    ਪੂਰਨ m_modport_ਅੰਕ = 0;  // Uniqueify modport numbers
    ਪੂਰਨ m_indent = 0;  // Indentation (tree depth) for debug
    ਤਾਰਕਿਕ m_ਵਿੱਚ_Sens = ਝੂਠ;  // True if in senitem
    ਅਚਲ AstWith* m_currentWithp = ਖਾਲੀ_ਸੂਚਕ;  // Enclosing 'with' (nullptr if none)
    std::ਸੈੱਟ<std::string>
        m_restricted_ਨਾਂ_ਵਰਤਿਆ;  // Names from current 'with (id_list)' that resolved into target
    ਤਾਰਕਿਕ m_genericIfaceModule = ਝੂਠ;  // True if in module containing generic interface
    std::map<std::string, Ast_ਗੰਢ*> m_if_Class_Imp_ਨਾਂ;  // Names imported from interface class
    // Inherited interface implementations needing a forward in the current class, by name
    std::map<std::string, ਅਚਲ VSymEnt*> m_ifForwards;
    std::ਸੈੱਟ<AstClass*> m_extendsParam;  // Classes that have a parameterized super class
                                         // (except the default instances)
                                         // They are added to the set only in linkDotPrimary.
    ਤਾਰਕਿਕ m_insideClassExtParam = ਝੂਠ;  // Inside a class from m_extendsParam
    AstNew* m_explicitSuperNewp = ਖਾਲੀ_ਸੂਚਕ;  // Hit a "super.new" call inside a "new" function
    std::map<Ast_ਗੰਢ*, AstPin*> m_ਵਰਤਿਆ_Pins;  // Pin used in this cell, map to duplicate
    std::map<Ast_ਗੰਢ*, std::map<std::string, AstPin*>> m_ਵਰਤਿਆ_Def_Param_Pins;
    // Defparam pins used for a given formal, by hierarchical path
    std::map<std::string, Ast_ਗੰਢ_Module*> m_modules_ਮੰਜ਼ਿਲ_Revisit;  // Modules to revisit a second time
    Ast_ਗੰਢ* m_ਆਖਰੀ_Deferredp = ਖਾਲੀ_ਸੂਚਕ;  // Last node which requested a revisit of its module
    Ast_ਗੰਢ_DType* m_packedArrayDtp = ਖਾਲੀ_ਸੂਚਕ;  // Datatype reference for packed array
    ਤਾਰਕਿਕ m_ਵਿੱਚ_Packed_Array = ਝੂਠ;  // Currently traversing a packed array tree
    ਤਾਰਕਿਕ m_replaceWithAlias
        = ਸੱਚ;  // Replace VarScope with an alias. Used in the handling of AstAlias
    ਤਾਰਕਿਕ m_isParam = ਝੂਠ;  // Specifies whether currently visiting param variable
    ਤਾਰਕਿਕ m_resolvingTypedef = ਝੂਠ;  // Currently traversing a Typedef tree

    ਬਣਤਰ Dot_ਹਾਲਤਾਂ final {
        DotPosition m_dotPos;  // Scope part of dotted resolution
        VSymEnt* m_dotSymp;  // SymEnt for dotted AstParse lookup
        ਅਚਲ AstDot* m_dotp;  // Current dot
        ਤਾਰਕਿਕ m_super;  // Starts with super reference
        ਤਾਰਕਿਕ m_unresolvedCell;  // Unresolved cell, needs help from V3Param
        ਤਾਰਕਿਕ m_unresolvedClass;  // Unresolved class reference, needs help from V3Param
        ਤਾਰਕਿਕ m_unresolvedGenericIface;  // Unresolved generic interface, needs help from V3Param
        ਤਾਰਕਿਕ m_genBlk;  // Contains gen block reference
        Ast_ਗੰਢ* m_unlinkedScopep;  // Unresolved scope, needs corresponding VarXRef
        AstDisable* m_disablep;  // Disable statement under which the reference is
        ਤਾਰਕਿਕ m_dotErr;  // Error found in dotted resolution, ignore upwards
        string m_dot_ਪਾਠ;  // String of dotted names found in below parseref
        Dot_ਹਾਲਤਾਂ() { init(ਖਾਲੀ_ਸੂਚਕ); }
        ~Dot_ਹਾਲਤਾਂ() = ਮੂਲ;
        ਖਾਲੀ init(VSymEnt* curSymp, ਤਾਰਕਿਕ unresolvedClass = ਝੂਠ) {
            m_dotPos = DP_NONE;
            m_dotSymp = curSymp;
            m_dotp = ਖਾਲੀ_ਸੂਚਕ;
            m_super = ਝੂਠ;
            m_dotErr = ਝੂਠ;
            m_dot_ਪਾਠ = "";
            m_unresolvedCell = ਝੂਠ;
            m_unresolvedClass = unresolvedClass;
            m_unresolvedGenericIface = ਝੂਠ;
            m_genBlk = ਝੂਠ;
            m_unlinkedScopep = ਖਾਲੀ_ਸੂਚਕ;
            m_disablep = ਖਾਲੀ_ਸੂਚਕ;
        }
        string ascii() ਅਚਲ {
            ਸਥਿਰ ਅਚਲ ਅੱਖਰ* ਅਚਲ ਨਾਂ[]
                = {"DP_NONE", "DP_PACKAGE", "DP_FIRST", "DP_SCOPE", "DP_FINAL", "DP_MEMBER"};
            std::ostringstream sstr;
            sstr << "ds=" << ਨਾਂ[m_dotPos];
            sstr << "  dse" << cvt_ਮੰਜ਼ਿਲ_Hex(m_dotSymp);
            ਅਚਲ string dsname = m_dotSymp->nodep()->name().substr(0, 8);
            sstr << "(" << m_dotSymp->nodep()->typeName() << (dsname.ਖਾਲੀ_ਥਾਵਾਂ() ? "" : ":") << dsname
                 << ")";
            ਜੇ (m_dotErr) sstr << "  [dotErr]";
            ਜੇ (m_super) sstr << "  [super]";
            ਜੇ (m_unresolvedCell) sstr << "  [unrCell]";
            ਜੇ (m_unresolvedClass) sstr << "  [unrClass]";
            ਜੇ (m_unresolvedGenericIface) sstr << "  [unrGIface]";
            ਜੇ (m_genBlk) sstr << "  [genBlk]";
            sstr << "  txt=" << m_dot_ਪਾਠ;
            ਵਾਪਸ sstr.str();
        }
    } m_ds;  // State to preserve across recursions

    // METHODS - Variables
    AstVar* createImplicitVar(VSymEnt* /*lookupSymp*/, AstParseRef* nodep, Ast_ਗੰਢ_Module* modp,
                              VSymEnt* moduleSymp, ਤਾਰਕਿਕ noWarn) {
        // Create implicit after warning
        ਜੇ (!noWarn) {
            ਜੇ (nodep->fileline()->warnIsOff(V3_Error_ਕੋਡ::I_DEF_NETTYPE_WIRE)) {
                ਅਚਲ string suggest = m_statep->suggestSymFallback(moduleSymp, nodep->name(),
                                                                    Link_ਗੰਢ_Matcher_Var{});
                nodep->v3error("Signal definition not found, and implicit disabled with "
                               "`default_nettype: "
                               << nodep->prettyNameQ() << '\n'
                               << (suggest.ਖਾਲੀ_ਥਾਵਾਂ() ? "" : nodep->warn_ਹੋਰ() + suggest));

            }
            // Bypass looking for suggestions if IMPLICIT is turned off
            // as there could be thousands of these suppressed in large netlists
            ਨਹੀਂਤਾਂ ਜੇ (!nodep->fileline()->warnIsOff(V3_Error_ਕੋਡ::IMPLICIT)) {
                ਅਚਲ string suggest = m_statep->suggestSymFallback(moduleSymp, nodep->name(),
                                                                    Link_ਗੰਢ_Matcher_Var{});
                nodep->v3warn(IMPLICIT,
                              "Signal definition not found, creating implicitly: "
                                  << nodep->prettyNameQ() << '\n'
                                  << (suggest.ਖਾਲੀ_ਥਾਵਾਂ() ? "" : nodep->warn_ਹੋਰ() + suggest));
            }
        }
        AstVar* ਅਚਲ newp
            = ਨਵਾਂ AstVar{nodep->fileline(), VVarType::WIRE, nodep->name(), VFlagLogicPacked{}, 1};
        newp->lifetime(VLifetime::STATIC_IMPLICIT);
        newp->trace(modp->modTrace());
        modp->ਜੋੜੋ_Stmtsp(newp);
        // Link it to signal list, must add the variable under the module;
        // current scope might be lower now
        m_statep->ਪਾਓ_Sym(moduleSymp, newp->name(), newp, ਖਾਲੀ_ਸੂਚਕ /*classOrPackagep*/);
        ਵਾਪਸ newp;
    }
    AstVar* ਮਿਲਿਆ_ਮੰਜ਼ਿਲ_Varp(ਅਚਲ VSymEnt* symp, Ast_ਗੰਢ* nodep, VAccess access) {
        // Return a variable if possible, auto converting a modport to variable
        ਜੇ (!symp) {
            ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(symp->nodep(), Var)) {
            ਵਾਪਸ VN_AS(symp->nodep(), Var);
        } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(symp->nodep(), ModportVarRef)) {
            AstModportVarRef* ਅਚਲ snodep = VN_AS(symp->nodep(), ModportVarRef);
            AstVar* ਅਚਲ varp = snodep->varp();
            ਜੇ (access.isWriteOrRW() && snodep->direction().isReadOnly()) {
                nodep->v3error("Attempt to drive input-only modport: " << nodep->prettyNameQ());
            }  // else other simulators don't warn about reading, and IEEE doesn't say illegal
            ਵਾਪਸ varp;
        } ਨਹੀਂਤਾਂ {
            ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        }
    }

    // Look up a virtual port through modport expression mapping.
    // When a dotted reference uses a modport with expression syntax, the virtual port name
    // maps to the real signal. This helper resolves that mapping during scope creation.
    // Returns the VarScope for the real signal, or nullptr if not found.
    AstVarScope* ਲੱਭੋ_Virtual_ਪੋਰਟ_Var_Scope(VSymEnt* dotSymp, ਅਚਲ string& ਪੋਰਟ_Name) {
        ਅਚਲ AstVarScope* ਅਚਲ dotVscp = VN_CAST(dotSymp->nodep(), VarScope);
        ਅਚਲ AstVar* ਅਚਲ dotVarp = dotVscp ? dotVscp->varp() : ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (!dotVarp) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;

        Ast_ਗੰਢ_DType* dtypep = dotVarp->ਬੱਚਾ_DTypep();
        ਜੇ (!dtypep) dtypep = dotVarp->subDTypep();
        ਜੇ (!dtypep) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        ਅਚਲ AstIfaceRefDType* ਅਚਲ ifaceRefp = VN_CAST(dtypep, IfaceRefDType);
        ਜੇ (!ifaceRefp || !ifaceRefp->modportp() || !ifaceRefp->iface_ਰਾਹੀਂ_Cellp()) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;

        // Get the interface cell's symbol table
        VSymEnt* ਅਚਲ ifaceCellSymp = m_statep->get_ਗੰਢ_Sym(ifaceRefp->iface_ਰਾਹੀਂ_Cellp());

        // Look up modport by name in interface cell
        VSymEnt* ਅਚਲ modportSymp = ifaceCellSymp->ਲੱਭੋ_Id_Fallback(ifaceRefp->modportp()->name());
        ਜੇ (!modportSymp) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;

        // Look up virtual port name in modport symbol table
        string baddot;
        VSymEnt* ਅਚਲ virtPortp = m_statep->ਲੱਭੋ_Sym_Prefixed(modportSymp, ਪੋਰਟ_Name, baddot, ਸੱਚ);
        ਜੇ (!virtPortp) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;

        // Symbol may point to VarScope or Var
        ਜੇ (AstVarScope* vscp = VN_CAST(virtPortp->nodep(), VarScope)) ਵਾਪਸ vscp;

        // Symbol points to Var - look up VarScope by name in interface cell
        ਜੇ (ਅਚਲ AstVar* ਅਚਲ realVarp = VN_CAST(virtPortp->nodep(), Var)) {
            VSymEnt* ਅਚਲ realFoundp
                = m_statep->ਲੱਭੋ_Sym_Prefixed(ifaceCellSymp, realVarp->name(), baddot, ਸੱਚ);
            ਵਾਪਸ realFoundp ? VN_CAST(realFoundp->nodep(), VarScope) : ਖਾਲੀ_ਸੂਚਕ;
        }
        ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
    }

    // Capture a ParamTypeDType reference for interface typedef retargeting.
    // Called when a RefDType resolves to a ParamTypeDType owned by an interface.
    ਖਾਲੀ captureIfaceParamType(AstRefDType* nodep, AstParamTypeDType* defp) {
        ਜੇ (!V3LinkDotIfaceCapture::enabled() || !m_statep->forPrimary()) ਵਾਪਸ;
        Ast_ਗੰਢ_Module* ਅਚਲ defOwnerModp = V3LinkDotIfaceCapture::ਲੱਭੋ_Owner_Module(defp);
        ਜੇ (!defOwnerModp || !VN_IS(defOwnerModp, Iface)) ਵਾਪਸ;
        AstCell* ਅਚਲ cellForCapture
            = m_ds.m_dotSymp ? VN_CAST(m_ds.m_dotSymp->nodep(), Cell) : ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (!cellForCapture) ਵਾਪਸ;
        UINFO(9, indent() << "iface capture add paramtype " << nodep
                          << " iface=" << defOwnerModp->prettyNameQ());
        V3LinkDotIfaceCapture::ਜੋੜੋ_Param_Type(nodep, cellForCapture->name(), m_modp, defp,
                                            defOwnerModp->name());
    }

    Ast_ਗੰਢ_Stmt* ਜੋੜੋ_Implicit_Super_New_Call(AstFunc* ਅਚਲ nodep,
                                         ਅਚਲ AstClassExtends* ਅਚਲ classExtendsp) {
        // Returns the added node
        File_ਸਤਰ* ਅਚਲ fl = nodep->fileline();
        AstArg* argsp = ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (classExtendsp->argsp()) argsp = classExtendsp->argsp()->clone_ਰੁੱਖ(ਸੱਚ);
        AstNew* ਅਚਲ newExprp = ਨਵਾਂ AstNew{fl, argsp};
        newExprp->isImplicit(ਸੱਚ);
        AstDot* ਅਚਲ superNewp = ਨਵਾਂ AstDot{fl, ਝੂਠ, ਨਵਾਂ AstParseRef{fl, "super"}, newExprp};
        Ast_ਗੰਢ_Stmt* ਅਚਲ superNewStmtp = superNewp->makeStmt();
        ਲਈ (Ast_ਗੰਢ* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            // super.new shall be the first statement (IEEE 1800-2023 8.15)
            // but some nodes (such as variable declarations and typedefs) should stay before
            ਜੇ (VN_IS(stmtp, ਗੰਢ_Stmt)) {
                stmtp->ਜੋੜੋ_ਇੱਥੇ_This_As_ਅਗਲਾ(superNewStmtp);
                ਵਾਪਸ superNewStmtp;
            }
        }
        // There were no statements
        nodep->ਜੋੜੋ_Stmtsp(superNewStmtp);
        ਵਾਪਸ superNewStmtp;
    }
    ਖਾਲੀ checkNoDot(Ast_ਗੰਢ* nodep) {
        ਜੇ (VL_UNLIKELY(!nodep)) {
            UINFO(9, indent() << "iface capture null node passed to checkNoDot; dot state="
                              << m_ds.ascii());
            m_ds.m_dotErr = ਸੱਚ;
            ਵਾਪਸ;
        }
        ਜੇ (VL_UNLIKELY(m_ds.m_dotPos != DP_NONE)) {
            UINFO(9, indent() << "ds=" << m_ds.ascii());
            UINFO(9, indent() << "iface capture checkNoDot hit node=" << nodep
                              << " dotState=" << m_ds.ascii());
            ਜੇ (VL_UNLIKELY(!m_ds.m_dotp)) {
                nodep->v3error("Syntax error: Not expecting " << nodep->type()
                                                              << " here (missing dot context)");
            } ਨਹੀਂਤਾਂ {
                nodep->v3error("Syntax error: Not expecting "
                               << nodep->type() << " under a " << nodep->backp()->type()
                               << " in dotted expression\n"
                               << nodep->warnContextPrimary() << m_ds.m_dotp->warnOther()
                               << "... Resolving this reference\n"
                               << m_ds.m_dotp->warnContextSecondary());
            }
            m_ds.m_dotErr = ਸੱਚ;
        }
    }
    AstVar* ਲੱਭੋ_Iface_ਸਿਖਰ_Varp(Ast_ਗੰਢ* nodep, VSymEnt* ਮਾਪਾ_Entp, ਅਚਲ string& name) {
        ਅਚਲ string ਲੱਭੋ_Name = name + "__Viftop";
        VSymEnt* ਅਚਲ ifaceSymp = ਮਾਪਾ_Entp->ਲੱਭੋ_Id_Fallback(ਲੱਭੋ_Name);
        AstVar* ਅਚਲ iface_ਸਿਖਰ_Varp = ifaceSymp ? VN_AS(ifaceSymp->nodep(), Var) : ਖਾਲੀ_ਸੂਚਕ;
        UASSERT_OBJ(iface_ਸਿਖਰ_Varp, nodep, "Can't find interface var ref: " << ਲੱਭੋ_Name);
        ਵਾਪਸ iface_ਸਿਖਰ_Varp;
    }
    AstRSProd* ਲੱਭੋ_Prod(Ast_ਗੰਢ* nodep, VSymEnt* ਮਾਪਾ_Entp, ਅਚਲ string& name) {
        ਅਚਲ VSymEnt* ਅਚਲ foundp = ਮਾਪਾ_Entp->ਲੱਭੋ_Id_Fallback(name);
        AstRSProd* ਮਿਲਿਆ_Nodep = foundp ? VN_CAST(foundp->nodep(), RSProd) : ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (!ਮਿਲਿਆ_Nodep) {
            VSpellCheck speller;
            Link_ਗੰਢ_Matcher_Prod matcher;
            ਮਾਪਾ_Entp->candidateIdFlat(&speller, &matcher, ਝੂਠ);
            ਅਚਲ string suggest = speller.ਉੱਤਮ_Candidate_ਸੁਨੇਹਾ(name);
            UINFO(1, "   ErrParseRef curSymp=se" << cvt_ਮੰਜ਼ਿਲ_Hex(ਮਾਪਾ_Entp));
            nodep->v3error("Production " << Ast_ਗੰਢ::prettyNameQ(name) << " not found\n"
                                         << (suggest.ਖਾਲੀ_ਥਾਵਾਂ() ? "" : nodep->warn_ਹੋਰ() + suggest));
            ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        }
        ਵਾਪਸ ਮਿਲਿਆ_Nodep;
    }
    ਖਾਲੀ duplicatePinError(AstPin* nodep, Ast_ਗੰਢ* origp, ਅਚਲ ਅੱਖਰ* whatp) {
        nodep->v3error("Duplicate " << whatp << " connection: " << nodep->prettyNameQ() << '\n'
                                    << nodep->warnContextPrimary() << '\n'
                                    << origp->warnOther() << "... Location of original " << whatp
                                    << " connection\n"
                                    << origp->warnContextSecondary());
    }
    ਖਾਲੀ markAndCheckPinDup(AstPin* nodep, Ast_ਗੰਢ* refp, ਅਚਲ ਅੱਖਰ* whatp) {
        ਅਚਲ ਸਵੈਚਾਲਿਤ pair = m_ਵਰਤਿਆ_Pins.emplace(refp, nodep);
        ਜੇ (pair.ਦੂਜਾ) {
            ਜੇ (!nodep->param_ਰਸਤਾ().ਖਾਲੀ_ਥਾਵਾਂ())
                m_ਵਰਤਿਆ_Def_Param_Pins[refp].emplace(nodep->param_ਰਸਤਾ(), nodep);
            ਵਾਪਸ;
        } ਨਹੀਂਤਾਂ {
            Ast_ਗੰਢ* ਅਚਲ origp = pair.ਪਹਿਲਾ->ਦੂਜਾ;
            ਜੇ (nodep->param_ਰਸਤਾ().ਖਾਲੀ_ਥਾਵਾਂ() || VN_AS(origp, Pin)->param_ਰਸਤਾ().ਖਾਲੀ_ਥਾਵਾਂ()) {
                duplicatePinError(nodep, origp, whatp);
                ਵਾਪਸ;
            }
        }

        ਸਵੈਚਾਲਿਤ& defParamPins = m_ਵਰਤਿਆ_Def_Param_Pins[refp];
        ਅਚਲ ਸਵੈਚਾਲਿਤ defParamIt = defParamPins.ਲੱਭੋ(nodep->param_ਰਸਤਾ());
        ਜੇ (defParamIt != defParamPins.ਸਿਰਾ()) {
            duplicatePinError(nodep, defParamIt->ਦੂਜਾ, whatp);
            ਵਾਪਸ;
        }
        defParamPins.emplace(nodep->param_ਰਸਤਾ(), nodep);
    }
    ਸਥਿਰ ਤਾਰਕਿਕ extendsClass(ਅਚਲ AstClass* classp, ਅਚਲ AstClass* targetp) {
        // Return true if classp inherits from targetp through already resolved extends
        // If performance of this becomes a problem, use just a counter to check
        // exceeds some number of iterations then perform the more expensive analysis
        std::ਸੈੱਟ<ਅਚਲ AstClass*> ਦੇਖਿਆ;
        std::ਸਦਿਸ਼<ਅਚਲ AstClass*> todo{classp};
        ਜਦੋਂ (!todo.ਖਾਲੀ_ਥਾਵਾਂ()) {
            ਅਚਲ AstClass* ਅਚਲ currp = todo.ਪਿੱਠ();
            todo.ਕੱਢੋ_ਪਿੱਠ();
            ਜੇ (currp == targetp) ਵਾਪਸ ਸੱਚ;
            ਜੇ (!ਦੇਖਿਆ.ਪਾਓ(currp).ਦੂਜਾ) ਜਾਰੀ;
            ਲਈ (ਅਚਲ AstClassExtends* cextp = currp->extendsp(); cextp;
                 cextp = VN_AS(cextp->nextp(), ClassExtends)) {
                ਜੇ (ਅਚਲ AstClass* ਅਚਲ basep = cextp->classOrNullp()) todo.ਧੱਕੋ_ਪਿੱਠ(basep);
            }
        }
        ਵਾਪਸ ਝੂਠ;
    }
    ਸਥਿਰ AstClocking* sensClockingp(Ast_ਗੰਢ* nodep) {
        // Return the clocking block referenced by nodep, either directly or through a modport
        ਜੇ (AstClocking* ਅਚਲ clockingp = VN_CAST(nodep, Clocking)) ਵਾਪਸ clockingp;
        ਜੇ (ਅਚਲ AstModportClockingRef* ਅਚਲ clockingRefp = VN_CAST(nodep, ModportClockingRef))
            ਵਾਪਸ clockingRefp->clockingp();
        ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
    }
    VSymEnt* getCreateClockingEventSymEnt(AstClocking* clockingp) {
        AstVar* ਅਚਲ eventp = clockingp->ensureEventp(ਸੱਚ);
        ਜੇ (!eventp->user1p()) eventp->user1p(ਨਵਾਂ VSymEnt{m_statep->symsp(), eventp});
        ਵਾਪਸ reinterpret_cast<VSymEnt*>(eventp->user1p());
    }
    VSymEnt* ਲੱਭੋ_Inlined_Dots_Sym(Ast_ਗੰਢ* nodep, ਅਚਲ string& inlinedDots) {
        // Return scope that applies for searching, given renaming due to module inlining.
        // Dotted lookup is always relative to module, as maybe variable
        // name lower down with same scope name we want to ignore (t_math_divw)
        VSymEnt* dotSymp = m_modSymp;
        ਅਚਲ string inl = Ast_ਗੰਢ::dedotName(inlinedDots);
        string baddot;
        VSymEnt* okSymp = ਖਾਲੀ_ਸੂਚਕ;
        dotSymp = m_statep->ਲੱਭੋ_Dotted(nodep->fileline(), dotSymp, inl, baddot, okSymp, ਝੂਠ);
        UASSERT_OBJ(dotSymp, nodep,
                    "Couldn't resolve inlined scope " << Ast_ਗੰਢ::prettyNameQ(baddot)
                                                      << " in: " << inlinedDots);
        UINFO(9, indent() << "findInlinedDotsSym " << dotSymp
                          << " search start due to inlinedDots=" << inlinedDots);
        ਵਾਪਸ dotSymp;
    }
    VSymEnt* ਲੱਭੋ_Dotted_Ref_Symp(Ast_ਗੰਢ* nodep, VSymEnt* lookupSymp, ਅਚਲ string& dotted,
                               string& baddot, VSymEnt*& okSymp, ਤਾਰਕਿਕ* modportp = ਖਾਲੀ_ਸੂਚਕ) {
        // Return scope to search for a relinked hierarchical reference
        // (AstVarXRef or AstEnumItemRef), or nullptr after error if not found
        // If modportp is given, set true when the reference went through a modport
        ਜੇ (modportp) *modportp = ਝੂਠ;
        VSymEnt* dotSymp = m_statep->ਲੱਭੋ_Dotted(nodep->fileline(), lookupSymp, dotted, baddot,
                                                okSymp, ਸੱਚ);  // Maybe nullptr
        ਜੇ (!dotSymp) {
            nodep->v3error(
                "Can't find definition of "
                << (!baddot.ਖਾਲੀ_ਥਾਵਾਂ() ? Ast_ਗੰਢ::prettyNameQ(baddot) : nodep->prettyNameQ()) << '\n'
                << nodep->warnContextPrimary());
            ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        }
        ਜੇ (ਅਚਲ AstVar* ਅਚਲ varp = VN_CAST(dotSymp->nodep(), Var)) {
            ਜੇ (ਅਚਲ AstIfaceRefDType* ਅਚਲ ifaceRefp
                = VN_CAST(varp->ਬੱਚਾ_DTypep(), IfaceRefDType)) {
                ਜੇ (ifaceRefp->modportp()) {
                    dotSymp = m_statep->get_ਗੰਢ_Sym(ifaceRefp->modportp());
                    ਜੇ (modportp) *modportp = ਸੱਚ;
                } ਨਹੀਂਤਾਂ {
                    dotSymp = m_statep->get_ਗੰਢ_Sym(ifaceRefp->ifacep());
                }
            }
        } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ AstModportClockingRef* ਅਚਲ clockingRefp
                   = VN_CAST(dotSymp->nodep(), ModportClockingRef)) {
            dotSymp = m_statep->get_ਗੰਢ_Sym(clockingRefp->clockingp());
        }
        ਵਾਪਸ dotSymp;
    }

    ਸਥਿਰ ਤਾਰਕਿਕ isParamedClassRefDType(ਅਚਲ Ast_ਗੰਢ* classp) {
        ਜਦੋਂ (ਅਚਲ AstRefDType* ਅਚਲ refp = VN_CAST(classp, RefDType))
            classp = refp->subDTypep();
        ਵਾਪਸ (VN_IS(classp, ClassRefDType) && VN_AS(classp, ClassRefDType)->paramsp())
               || VN_IS(classp, ParamTypeDType);
    }
    ਤਾਰਕਿਕ isParamedClassRef(ਅਚਲ Ast_ਗੰਢ* nodep) {
        // Is this a parameterized reference to a class, or a reference to class parameter
        ਜੇ (ਅਚਲ ਸਵੈਚਾਲਿਤ* classRefp = VN_CAST(nodep, ClassOrPackageRef)) {
            ਜੇ (classRefp->paramsp()) ਵਾਪਸ ਸੱਚ;
            ਅਚਲ ਸਵੈਚਾਲਿਤ* classp = classRefp->classOrPackageNodep();
            ਜਦੋਂ (ਅਚਲ ਸਵੈਚਾਲਿਤ* typedefp = VN_CAST(classp, Typedef)) classp = typedefp->subDTypep();
            ਵਾਪਸ isParamedClassRefDType(classp);
        }
        ਵਾਪਸ isParamedClassRefDType(nodep);
    }
    VSymEnt* getThisClassSymp() {
        VSymEnt* classSymp = m_ds.m_dotSymp;
        ਜਦੋਂ (classSymp && !VN_IS(classSymp->nodep(), Class)) {
            classSymp = classSymp->parentp();
        }
        ਵਾਪਸ classSymp;
    }
    // Is the current scope within the given class, including within its nested classes
    ਤਾਰਕਿਕ insideClass(ਅਚਲ AstClass* classp) ਅਚਲ {
        ਲਈ (ਅਚਲ VSymEnt* symp = m_curSymp; symp; symp = symp->parentp()) {
            ਜੇ (symp->nodep() == classp) ਵਾਪਸ ਸੱਚ;
        }
        ਵਾਪਸ ਝੂਠ;
    }
    // 'randomize() with' target class not yet specialized; defer to post-V3Param pass
    ਤਾਰਕਿਕ rand_ਨਿਸ਼ਾਨਾ_Unresolved() ਅਚਲ {
        ਵਾਪਸ m_statep->forPrimary() && m_currentWithp && m_rand_ਵਿਧੀ_Callp && !m_randSymp;
    }
    // Is 'funcp' of 'classp' virtual, directly or by overriding a virtual method of a base
    // class (IEEE 1800-2023 8.20)
    ਤਾਰਕਿਕ is_Virtual_ਵਿਧੀ(ਅਚਲ Ast_ਗੰਢ_FTask* funcp, ਅਚਲ AstClass* classp) ਅਚਲ {
        ਜੇ (funcp->isVirtual()) ਵਾਪਸ ਸੱਚ;
        ਲਈ (ਅਚਲ AstClassExtends* cextp = classp->extendsp(); cextp;
             cextp = VN_AS(cextp->nextp(), ClassExtends)) {
            AstClass* ਅਚਲ basep = cextp->classOrNullp();
            ਜੇ (!basep) ਜਾਰੀ;
            ਅਚਲ VSymEnt* ਅਚਲ foundp = m_statep->get_ਗੰਢ_Sym(basep)->ਲੱਭੋ_Id_Flat(funcp->name());
            ਅਚਲ Ast_ਗੰਢ_FTask* ਅਚਲ ਅਧਾਰ_Funcp
                = foundp ? VN_CAST(foundp->nodep(), ਗੰਢ_FTask) : ਖਾਲੀ_ਸੂਚਕ;
            ਅਚਲ AstClass* ਅਚਲ ਅਧਾਰ_Classp = foundp && foundp->parentp()
                                                   ? VN_CAST(foundp->parentp()->nodep(), Class)
                                                   : ਖਾਲੀ_ਸੂਚਕ;
            ਜੇ (ਅਧਾਰ_Funcp && ਅਧਾਰ_Classp && is_Virtual_ਵਿਧੀ(ਅਧਾਰ_Funcp, ਅਧਾਰ_Classp)) ਵਾਪਸ ਸੱਚ;
        }
        ਵਾਪਸ ਝੂਠ;
    }
    // Is imported 'implp' an implementation of interface method 'ifSymp' (IEEE 1800-2023
    // 8.26.2)? If its class does not derive from the interface class, it needs a forwarding
    // override in the derived class to be reachable through the interface.
    ਤਾਰਕਿਕ inheritedImplements(ਅਚਲ VSymEnt* implp, ਅਚਲ VSymEnt* ifSymp, ਤਾਰਕਿਕ& needForwardr) {
        needForwardr = ਝੂਠ;
        ਅਚਲ Ast_ਗੰਢ_FTask* ਅਚਲ implFuncp = VN_CAST(implp->nodep(), ਗੰਢ_FTask);
        ਜੇ (!implFuncp || implFuncp->ਸ਼ੁੱਧ_Virtual() || implFuncp->isStatic()) ਵਾਪਸ ਝੂਠ;
        ਅਚਲ AstClass* ਅਚਲ implClassp
            = implp->parentp() ? VN_CAST(implp->parentp()->nodep(), Class) : ਖਾਲੀ_ਸੂਚਕ;
        ਅਚਲ AstClass* ਅਚਲ ifClassp
            = ifSymp->parentp() ? VN_CAST(ifSymp->parentp()->nodep(), Class) : ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (!implClassp || !ifClassp) ਵਾਪਸ ਝੂਠ;
        ਜੇ (extendsClass(implClassp, ifClassp)) ਵਾਪਸ ਸੱਚ;
        ਜੇ (!is_Virtual_ਵਿਧੀ(implFuncp, implClassp)) ਵਾਪਸ ਝੂਠ;
        needForwardr = ਸੱਚ;
        ਵਾਪਸ ਸੱਚ;
    }
    // Add 'implp' override to 'classp' that calls super.implp, so the C++ method of an
    // implemented interface class is overridden
    ਖਾਲੀ ਜੋੜੋ_Interface_Forward(AstClass* classp, Ast_ਗੰਢ_FTask* implp, AstClass* implClassp) {
        File_ਸਤਰ* ਅਚਲ flp = implp->fileline();
        AstVar* fvarp = ਖਾਲੀ_ਸੂਚਕ;
        Ast_ਗੰਢ_FTask* newp;
        ਜੇ (AstFunc* ਅਚਲ implFuncp = VN_CAST(implp, Func)) {
            fvarp = VN_AS(implFuncp->fvarp(), Var)->clone_ਰੁੱਖ(ਝੂਠ);
            newp = ਨਵਾਂ AstFunc{flp, implp->name(), ਖਾਲੀ_ਸੂਚਕ, fvarp};
        } ਨਹੀਂਤਾਂ {
            newp = ਨਵਾਂ AstTask{flp, implp->name(), ਖਾਲੀ_ਸੂਚਕ};
        }
        newp->class_ਵਿਧੀ(ਸੱਚ);
        newp->isVirtual(ਸੱਚ);
        newp->lifetime(implp->lifetime());
        AstArg* argsp = ਖਾਲੀ_ਸੂਚਕ;
        ਲਈ (Ast_ਗੰਢ* stmtp = implp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstVar* ਅਚਲ portp = VN_CAST(stmtp, Var);
            ਜੇ (!portp || !portp->isIO()) ਜਾਰੀ;
            AstVar* ਅਚਲ newPortp = portp->clone_ਰੁੱਖ(ਝੂਠ);
            newp->ਜੋੜੋ_Stmtsp(newPortp);
            argsp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(
                argsp, ਨਵਾਂ AstArg{flp, "", ਨਵਾਂ AstVarRef{flp, newPortp, VAccess::READ}});
        }
        Ast_ਗੰਢ_FTask_Ref* callp;
        ਜੇ (AstFunc* ਅਚਲ implFuncp = VN_CAST(implp, Func)) {
            callp = ਨਵਾਂ AstFuncRef{flp, implFuncp, argsp};
        } ਨਹੀਂਤਾਂ {
            callp = ਨਵਾਂ AstTaskRef{flp, VN_AS(implp, Task), argsp};
        }
        callp->classOrPackagep(implClassp);
        callp->superReference(ਸੱਚ);
        ਜੇ (fvarp) {
            newp->ਜੋੜੋ_Stmtsp(ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, ਨਵਾਂ AstVarRef{flp, fvarp, VAccess::WRITE}, callp});
        } ਨਹੀਂਤਾਂ {
            newp->ਜੋੜੋ_Stmtsp(callp->makeStmt());
        }
        UINFO(9, indent() << "Interface forward " << newp);
        classp->ਜੋੜੋ_Membersp(newp);
    }
    ਖਾਲੀ importDerivedClass(AstClass* derivedClassp, VSymEnt* ਅਧਾਰ_Symp, AstClass* ਅਧਾਰ_Classp) {
        // Also used for standard 'extends' from a base class
        UINFO(8, indent() << "importDerivedClass to " << derivedClassp << " from " << ਅਧਾਰ_Classp);
        ਲਈ (VSymEnt::const_iterator it = ਅਧਾਰ_Symp->begin(); it != ਅਧਾਰ_Symp->ਸਿਰਾ(); ++it) {
            ਜੇ (Ast_ਗੰਢ* ਅਧਾਰ_Subp = it->ਦੂਜਾ->nodep()) {
                UINFO(8, indent() << "  SymFunc " << ਅਧਾਰ_Subp);
                ਅਚਲ string impOrExtends
                    = ਅਧਾਰ_Classp->isInterfaceClass() ? " implements " : " extends ";
                ਜੇ (VN_IS(ਅਧਾਰ_Subp, ਗੰਢ_FTask)) {
                    ਅਚਲ VSymEnt* ਅਚਲ foundp = m_curSymp->ਲੱਭੋ_Id_Flat(ਅਧਾਰ_Subp->name());
                    ਅਚਲ Ast_ਗੰਢ_FTask* ਅਚਲ ਅਧਾਰ_Funcp = VN_CAST(ਅਧਾਰ_Subp, ਗੰਢ_FTask);
                    ਜੇ (!ਅਧਾਰ_Funcp || !ਅਧਾਰ_Funcp->ਸ਼ੁੱਧ_Virtual()) ਜਾਰੀ;
                    ਤਾਰਕਿਕ needForward = ਝੂਠ;
                    ਅਚਲ ਤਾਰਕਿਕ exists_ਵਿੱਚ_Derived
                        = foundp
                          && (!foundp->imported()
                              || inheritedImplements(foundp, it->ਦੂਜਾ, needForward));
                    ਜੇ (needForward && m_statep->forPrimary()
                        && !derivedClassp->isInterfaceClass()) {
                        m_ifForwards.emplace(ਅਧਾਰ_Subp->name(), foundp);
                    }
                    ਜੇ (m_statep->forPrimary() && !exists_ਵਿੱਚ_Derived
                        && !derivedClassp->isInterfaceClass() && !derivedClassp->isVirtual()) {
                        derivedClassp->v3error(
                            "Class " << derivedClassp->prettyNameQ() << impOrExtends
                                     << ਅਧਾਰ_Classp->prettyNameQ()
                                     << " but is missing implementation for "
                                     << ਅਧਾਰ_Subp->prettyNameQ() << " (IEEE 1800-2023 8.26)\n"
                                     << derivedClassp->warnContextPrimary() << '\n'
                                     << ਅਧਾਰ_Subp->warnOther()
                                     << "... Location of interface class's function\n"
                                     << ਅਧਾਰ_Subp->warnContextSecondary());
                    }
                    ਅਚਲ ਸਵੈਚਾਲਿਤ itn = m_if_Class_Imp_ਨਾਂ.ਲੱਭੋ(ਅਧਾਰ_Subp->name());
                    ਜੇ (m_statep->forPrimary() && !exists_ਵਿੱਚ_Derived
                        && itn != m_if_Class_Imp_ਨਾਂ.ਸਿਰਾ()
                        && itn->ਦੂਜਾ != ਅਧਾਰ_Subp) {  // Not exact same function from diamond
                        derivedClassp->v3error(
                            "Class " << derivedClassp->prettyNameQ() << impOrExtends
                                     << ਅਧਾਰ_Classp->prettyNameQ()
                                     << " but missing inheritance conflict resolution for "
                                     << ਅਧਾਰ_Subp->prettyNameQ() << " (IEEE 1800-2023 8.26.6.2)\n"
                                     << derivedClassp->warnContextPrimary() << '\n'
                                     << ਅਧਾਰ_Subp->warnOther()
                                     << "... Location of interface class's function\n"
                                     << ਅਧਾਰ_Subp->warnContextSecondary());
                    }
                    m_if_Class_Imp_ਨਾਂ.emplace(ਅਧਾਰ_Subp->name(), ਅਧਾਰ_Subp);
                }
                ਜੇ (VN_IS(ਅਧਾਰ_Subp, Constraint)) {
                    ਅਚਲ VSymEnt* ਅਚਲ foundp = m_curSymp->ਲੱਭੋ_Id_Flat(ਅਧਾਰ_Subp->name());
                    ਅਚਲ AstConstraint* ਅਚਲ ਅਧਾਰ_Funcp = VN_CAST(ਅਧਾਰ_Subp, Constraint);
                    ਜੇ (!ਅਧਾਰ_Funcp || !ਅਧਾਰ_Funcp->is_Kwd_ਸ਼ੁੱਧ()) ਜਾਰੀ;
                    ਅਚਲ ਤਾਰਕਿਕ exists_ਵਿੱਚ_Derived = foundp && !foundp->imported();
                    ਜੇ (m_statep->forPrimary() && !exists_ਵਿੱਚ_Derived
                        && !derivedClassp->isInterfaceClass() && !derivedClassp->isVirtual()) {
                        derivedClassp->v3error(
                            "Class " << derivedClassp->prettyNameQ() << impOrExtends
                                     << ਅਧਾਰ_Classp->prettyNameQ()
                                     << " but is missing constraint implementation for "
                                     << ਅਧਾਰ_Subp->prettyNameQ() << " (IEEE 1800-2023 18.5.2)\n"
                                     << derivedClassp->warnContextPrimary() << '\n'
                                     << ਅਧਾਰ_Subp->warnOther()
                                     << "... Location of interface class's pure constraint\n"
                                     << ਅਧਾਰ_Subp->warnContextSecondary());
                    }
                }
            }
        }
    }
    ਖਾਲੀ importSymbolsFromExtended(AstClass* ਅਚਲ nodep, AstClassExtends* ਅਚਲ cextp) {
        AstClass* ਅਚਲ ਅਧਾਰ_Classp = cextp->classp();
        VSymEnt* ਅਚਲ srcp = m_statep->get_ਗੰਢ_Sym(ਅਧਾਰ_Classp);
        importDerivedClass(nodep, srcp, ਅਧਾਰ_Classp);
        ਜੇ (!cextp->isImplements()) m_curSymp->importFromClass(m_statep->symsp(), srcp);
    }
    ਖਾਲੀ classExtendImport(AstClass* nodep) {
        // A class reference might be to a class that is later in Ast due to
        // e.g. parameterization or referring to a "class (type T) extends T"
        // Resolve it so later Class:: references into its base classes work
        symIterateNull(nodep, m_statep->get_ਗੰਢ_Sym(nodep));
    }

    ਖਾਲੀ check_Decl_ਕ੍ਰਮ(Ast_ਗੰਢ* nodep, Ast_ਗੰਢ* declp) {
        ਅਚਲ uint32_t decl_Token_ਅੰਕ = declp->decl_Token_ਅੰਕ();
        ਜੇ (!decl_Token_ਅੰਕ) ਵਾਪਸ;  // Not implemented/valid on this object
        ਜੇ (nodep->fileline()->token_ਅੰਕ() < decl_Token_ਅੰਕ) {
            UINFO(1, "Related node " << nodep->fileline()->token_ਅੰਕ() << " " << nodep);
            UINFO(1, "Related decl " << decl_Token_ਅੰਕ << " " << declp);
            nodep->v3error("Reference to "
                           << nodep->prettyNameQ() << " before declaration (IEEE 1800-2023 6.18)\n"
                           << nodep->warn_ਹੋਰ()
                           << "... Suggest move the declaration before the reference, or use a "
                              "forward typedef\n"
                           << nodep->warnContextPrimary() << '\n'
                           << declp->warnOther() << "... Location of original declaration\n"
                           << declp->warnContextSecondary());
        }
    }
    ਖਾਲੀ check_Member_Decl_ਕ੍ਰਮ(Ast_ਗੰਢ* nodep, AstMemberDType* declp) {
        ਅਚਲ uint32_t decl_Token_ਅੰਕ = declp->fileline()->token_ਅੰਕ();
        ਜੇ (nodep->fileline()->token_ਅੰਕ() < decl_Token_ਅੰਕ) {
            UINFO(1, "Related node " << nodep->fileline()->token_ਅੰਕ() << " " << nodep);
            UINFO(1, "Related decl " << decl_Token_ਅੰਕ << " " << declp);
            nodep->v3error("Reference to "
                           << nodep->prettyNameQ() << " before declaration (IEEE 1800-2023 6.18)\n"
                           << nodep->warn_ਹੋਰ()
                           << "... Suggest move the declaration before the reference\n"
                           << nodep->warnContextPrimary() << '\n'
                           << declp->warnOther() << "... Location of original declaration\n"
                           << declp->warnContextSecondary());
        }
    }

    ਖਾਲੀ replaceWithCheckBreak(Ast_ਗੰਢ* oldp, Ast_ਗੰਢ_DType* newp) {
        // Flag now to avoid V3Broken throwing an internal error
        ਜੇ (oldp->wouldBreak(newp)) {
            newp->v3error(
                "Data type used where a non-data type is expected: " << newp->prettyNameQ());
            oldp->replaceWith(ਨਵਾਂ AstConst{newp->fileline(), AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_False_Erroring{}});
        } ਨਹੀਂਤਾਂ {
            oldp->replaceWith(newp);
        }
    }

    // Marks the current module to be revisited after the initial AST iteration
    ਖਾਲੀ revisitLater(Ast_ਗੰਢ* deferredNodep) {
        // Need to revisit entire module to build up all the necessary context
        m_ਆਖਰੀ_Deferredp = deferredNodep;
        m_modules_ਮੰਜ਼ਿਲ_Revisit.ਪਾਓ(std::make_pair(m_modp->name(), m_modp));
    }

    ਖਾਲੀ updateVarUse(AstVar* nodep) {
        // Avoid dotted.PARAM false positive when in a parameter block
        // that is if ()'ed off by same dotted name as another block
        ਜੇ (nodep && nodep->isParam()) nodep->ਵਰਤਿਆ_Param(ਸੱਚ);
    }

    ਸਥਿਰ VSymEnt* ਲੱਭੋ_Id_Fallback_Skip_Member_DType(VSymEnt* lookp, ਅਚਲ string& name) {
        VSymEnt* shadowEntp = ਖਾਲੀ_ਸੂਚਕ;  // Shadowing variable: not a type, kept for error report
        ਜਦੋਂ (lookp) {
            VSymEnt* ਅਚਲ foundp = lookp->ਲੱਭੋ_Id_Flat(name);
            ਜੇ (foundp && !VN_IS(foundp->nodep(), MemberDType)) {
                // Non-type entries are not type candidates
                // (IEEE 1800-2023 6.18); skip them so an enclosing
                // type is found, but keep one to preserve the "found: ..." error.
                ਜੇ (VN_IS(foundp->nodep(), Typedef) || VN_IS(foundp->nodep(), ParamTypeDType)
                    || VN_IS(foundp->nodep(), Class)) {
                    ਵਾਪਸ foundp;
                }
                ਜੇ (!shadowEntp) shadowEntp = foundp;
            }
            lookp = lookp->fallbackp();
        }
        ਵਾਪਸ shadowEntp;
    }

    ਖਾਲੀ symIterateChildren(Ast_ਗੰਢ* nodep, VSymEnt* symp) {
        // Iterate children, changing to given context, with restore to old context
        VL_RESTORER_COPY(m_ds);
        VL_RESTORER(m_curSymp);
        m_curSymp = symp;
        m_ds.init(m_curSymp);
        iterateChildren(nodep);
    }
    ਖਾਲੀ symIterateNull(Ast_ਗੰਢ* nodep, VSymEnt* symp) {
        // Iterate node, changing to given context, with restore to old context
        VL_RESTORER_COPY(m_ds);
        VL_RESTORER(m_curSymp);
        m_curSymp = symp;
        m_ds.init(m_curSymp);
        iterateNull(nodep);
    }
    ਸਥਿਰ ਅਚਲ Ast_ਗੰਢ_DType* get_ਵਿਅੰਜਕ_DTypep(ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* selp) {
        ਜਦੋਂ (ਅਚਲ Ast_ਗੰਢ_Pre_Sel* ਅਚਲ sp = VN_CAST(selp, ਗੰਢ_Pre_Sel)) selp = sp->fromp();
        ਜੇ (ਅਚਲ AstMemberSel* ਅਚਲ sp = VN_CAST(selp, MemberSel)) {
            ਜੇ (ਅਚਲ Ast_ਗੰਢ_DType* dtypep = get_ਵਿਅੰਜਕ_DTypep(sp->fromp())) {
                ਜੇ (ਅਚਲ AstClassRefDType* ਅਚਲ classRefp = VN_CAST(dtypep, ClassRefDType)) {
                    ਅਚਲ AstClass* ਅਚਲ classp = classRefp->classp();
                    ਅਚਲ ਤਾਰਕਿਕ ਮਿਲਿਆ = classp->existsMember(
                        [&dtypep, name = selp->name()](ਅਚਲ AstClass*, ਅਚਲ AstVar* nodep) {
                            dtypep = nodep->ਬੱਚਾ_DTypep();
                            ਵਾਪਸ nodep->name() == name;
                        });
                    ਜੇ (ਮਿਲਿਆ) ਵਾਪਸ dtypep->elemDTypep();
                    selp->v3error("Class " << classRefp->prettyNameQ()
                                           << " does not contain field " << selp->prettyNameQ());
                } ਨਹੀਂਤਾਂ {
                    selp->v3fatalSrc("Member selection on expression of type "
                                     << dtypep->prettyDTypeNameQ()
                                     << ", which is not a class type");
                }
            }
        } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ Ast_ਗੰਢ_Var_Ref* ਅਚਲ varRefp = VN_CAST(selp, ਗੰਢ_Var_Ref)) {
            ਵਾਪਸ varRefp->varp()->ਬੱਚਾ_DTypep()->elemDTypep();
        }
        ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
    }
    ਸਥਿਰ ਅਚਲ AstVar* get_ਅਗਲਾ_Varp(ਅਚਲ Ast_ਗੰਢ* stmtsp) {
        // Only IO ports, as parameters are on paramsp(), not the pinsp() list paired here
        ਜਦੋਂ (stmtsp) {
            ਜੇ (ਅਚਲ AstVar* ਅਚਲ varp = VN_CAST(stmtsp, Var)) {
                ਜੇ (varp->isIO()) ਵਾਪਸ varp;
            }
            stmtsp = stmtsp->nextp();
        }
        ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
    }
    // Introduce implicit parameters for modules with generic interafeces
    ਖਾਲੀ ਜੋੜੋ_Implicit_Parameters_ਦਾ_Generic_Iface(AstCell* ਅਚਲ nodep, ਅਚਲ AstModule* ਅਚਲ modp) {
        // Get Param number
        ਪੂਰਨ param_ਅੰਕ = 1;
        ਲਈ (AstPin* paramp = nodep->paramsp(); paramp; paramp = VN_CAST(paramp->nextp(), Pin)) {
            ++param_ਅੰਕ;
        }

        // Add each implicit parameter type
        ਅਚਲ AstVar* modIfaceVarp = get_ਅਗਲਾ_Varp(modp->stmtsp());
        ਲਈ (ਅਚਲ AstPin* pinp = nodep->pinsp(); pinp && modIfaceVarp;
             pinp = VN_CAST(pinp->nextp(), Pin),
                           modIfaceVarp = get_ਅਗਲਾ_Varp(modIfaceVarp->nextp())) {
            ਜੇ (modIfaceVarp->varType() != VVarType::IFACEREF
                || !VN_IS(modIfaceVarp->ਬੱਚਾ_DTypep()->elemDTypep(), IfaceGenericDType)) {
                ਜਾਰੀ;
            }
            Ast_ਗੰਢ* exprp = pinp->exprp();
            ਜੇ (!exprp) {
                modIfaceVarp->v3error("Interface port "
                                      << modIfaceVarp->prettyNameQ()
                                      << " is not connected to interface/modport pin expression");
                ਜਾਰੀ;
            }
            ਜਦੋਂ (ਅਚਲ Ast_ਗੰਢ_Pre_Sel* ਅਚਲ preSelp = VN_CAST(exprp, ਗੰਢ_Pre_Sel)) {
                exprp = preSelp->fromp();
            }
            // Resolve pin expression to the enclosing module's port Var. At primary
            // LinkDot the expression may still be an AstParseRef, so also look up by name.
            AstVar* enclosingVarp = ਖਾਲੀ_ਸੂਚਕ;
            ਅਚਲ AstVarRef* ਅਚਲ varRefp = VN_CAST(exprp, VarRef);
            ਜੇ (varRefp) {
                enclosingVarp = varRefp->varp();
            } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ AstParseRef* ਅਚਲ parseRefp = VN_CAST(exprp, ParseRef)) {
                ਜੇ (m_modp) {
                    ਜੇ (VSymEnt* ਅਚਲ symp
                        = m_statep->get_ਗੰਢ_Sym(m_modp)->ਲੱਭੋ_Id_Flat(parseRefp->name())) {
                        enclosingVarp = VN_CAST(symp->nodep(), Var);
                    }
                }
            }
            ਜੇ (enclosingVarp && enclosingVarp->varType() == VVarType::IFACEREF
                && VN_IS(enclosingVarp->ਬੱਚਾ_DTypep()->skipRefp(), IfaceGenericDType)) {
                // Nested generic-iface forwarding (#7454): enclosing port is itself still
                // generic, so emit a placeholder __VGIfaceParam pin carrying a VarRef to
                // the outer port. V3Param rewrites it to the concrete IfaceRefDType once
                // the enclosing module is specialized (see
                // ParamProcessor::resolveGenericIfaceForwardingPins for the ordering
                // constraint that keeps the rewrite inside V3Param).
                AstVarRef* ਅਚਲ fwdRefp
                    = ਨਵਾਂ AstVarRef{exprp->fileline(), enclosingVarp, VAccess::READ};
                AstPin* ਅਚਲ newPinp = ਨਵਾਂ AstPin{
                    pinp->fileline(), param_ਅੰਕ, "__VGIfaceParam" + modIfaceVarp->name(), fwdRefp};
                newPinp->param(ਸੱਚ);
                ਦੇਖੋ(newPinp);
                nodep->ਜੋੜੋ_Paramsp(newPinp);
            } ਨਹੀਂਤਾਂ ਜੇ (varRefp) {
                ਅਚਲ AstVar* ਅਚਲ varp = varRefp->varp();
                ਜੇ (ਅਚਲ AstIfaceRefDType* ਅਚਲ refp
                    = VN_CAST(varp->ਬੱਚਾ_DTypep()->elemDTypep(), IfaceRefDType)) {
                    AstIface* ਅਚਲ ifacep = VN_AS(refp->cellp()->modp(), Iface);
                    AstIfaceRefDType* newIfaceRefp;
                    ਜੇ (refp->modportp()) {
                        newIfaceRefp = ਨਵਾਂ AstIfaceRefDType{
                            refp->fileline(), refp->modportFileline(), m_modp->name(),
                            ifacep->name(), refp->modportName()};
                    } ਨਹੀਂਤਾਂ {
                        newIfaceRefp = ਨਵਾਂ AstIfaceRefDType{refp->fileline(), m_modp->name(),
                                                            ifacep->name()};
                    }
                    newIfaceRefp->ifacep(ifacep);
                    ਜੇ (refp->cellp()->paramsp()) {
                        newIfaceRefp->ਜੋੜੋ_Paramsp(refp->cellp()->paramsp()->clone_ਰੁੱਖ(ਸੱਚ));
                    }
                    UASSERT_OBJ(pinp->name().ਲੱਭੋ("__pinNumber") == 0
                                    || pinp->name() == modIfaceVarp->name(),
                                pinp, "Not found interface with such name");
                    AstPin* ਅਚਲ newPinp
                        = ਨਵਾਂ AstPin{pinp->fileline(), param_ਅੰਕ,
                                     "__VGIfaceParam" + modIfaceVarp->name(), newIfaceRefp};
                    newPinp->param(ਸੱਚ);
                    ਦੇਖੋ(newPinp);
                    nodep->ਜੋੜੋ_Paramsp(newPinp);
                } ਨਹੀਂਤਾਂ {
                    varRefp->v3error("Generic interfaces can only connect to an interface and "
                                     << varp->prettyNameQ() << " is "
                                     << (varp->ਬੱਚਾ_DTypep()
                                             ? "of type " + varp->ਬੱਚਾ_DTypep()->prettyDTypeNameQ()
                                             : "not an interface"));
                }
            } ਨਹੀਂਤਾਂ {
                exprp->v3error("Expected an interface but " << exprp->prettyNameQ()
                                                            << " is not an interface");
            }
            ++param_ਅੰਕ;
        }
    }

    // Returns true and issues error iff 'varp' cannot be accesed with a VarXRef
    // in a hier_block given by 'scopeSymp'
    ਤਾਰਕਿਕ error_Hier_Non_ਪੋਰਟ(AstVarXRef* refp, AstVar* varp, VSymEnt* scopeSymp) {
        // OK to access ports on hier_block
        ਜੇ (varp->isIO()) ਵਾਪਸ ਝੂਠ;
        // OK if not an instance
        ਅਚਲ AstCell* ਅਚਲ cellp = VN_CAST(scopeSymp->nodep(), Cell);
        ਜੇ (!cellp) ਵਾਪਸ ਝੂਠ;
        // Ok if not a hier_block
        ਅਚਲ Ast_ਗੰਢ_Module* ਅਚਲ modp = cellp->modp();
        ਜੇ (!modp->hier_ਖੰਡ()) ਵਾਪਸ ਝੂਠ;
        // Bad
        refp->v3error("Cannot access non-port symbols inside hierarchical block");
        ਵਾਪਸ ਸੱਚ;
    }

#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ LINKDOT_ਦੇਖੋ_ਸ਼ੁਰੂ() \
    VL_RESTORER(m_indent); \
    ++m_indent;

    string indent() ਅਚਲ {
        string ਨਤੀਜਾ = "";
        ਨਤੀਜਾ.ਪਾਓ(0, m_indent, ':');
        ਵਾਪਸ ਨਤੀਜਾ + " ";
    }

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(AstNetlist* nodep) override {
        // Recurse..., backward as must do packages before using packages
        iterateChildrenBackwardsConst(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Type_ਸਾਰਣੀ*) override {}
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Module* nodep) override {
        ਜੇ (nodep->dead()) ਵਾਪਸ;
        LINKDOT_ਦੇਖੋ_ਸ਼ੁਰੂ();
        UINFO(8, indent() << "visit " << nodep);
        VL_RESTORER(m_genericIfaceModule);
        ਜੇ (ਅਚਲ AstModule* ਅਚਲ modp = VN_CAST(nodep, Module)) {
            m_genericIfaceModule = modp->hasGenericIface();
        }
        checkNoDot(nodep);
        m_ds.init(m_curSymp);
        m_ds.m_dotSymp = m_curSymp = m_modSymp
            = m_statep->get_ਗੰਢ_Sym(nodep);  // Until overridden by a SCOPE
        m_cellp = ਖਾਲੀ_ਸੂਚਕ;
        m_modp = nodep;
        m_modport_ਅੰਕ = 0;
        iterateChildren(nodep);
        m_modp = ਖਾਲੀ_ਸੂਚਕ;
        m_ds.m_dotSymp = m_curSymp = m_modSymp = ਖਾਲੀ_ਸੂਚਕ;
        m_ds.m_dotPos = DP_NONE;
        m_ds.m_dotErr = ਝੂਠ;
    }
    ਖਾਲੀ ਦੇਖੋ(AstScope* nodep) override {
        LINKDOT_ਦੇਖੋ_ਸ਼ੁਰੂ();
        UINFO(8, indent() << "visit " << nodep);
        checkNoDot(nodep);
        VL_RESTORER(m_modSymp);
        VL_RESTORER(m_curSymp);
        m_ds.m_dotSymp = m_curSymp = m_modSymp = m_statep->getScopeSym(nodep);
        iterateChildren(nodep);
        m_ds.m_dotSymp = m_curSymp = m_modSymp = ਖਾਲੀ_ਸੂਚਕ;
    }
    ਖਾਲੀ ਦੇਖੋ(AstCellInline* nodep) override {
        LINKDOT_ਦੇਖੋ_ਸ਼ੁਰੂ();
        checkNoDot(nodep);
        UASSERT_OBJ(m_statep->forScopeCreation(), nodep, "CellInline should not exist yet");
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep->unlink_Fr_ਪਿੱਠ()), nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstCell* nodep) override {
        // Cell: Recurse inside or cleanup not founds
        LINKDOT_ਦੇਖੋ_ਸ਼ੁਰੂ();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        VL_RESTORER_CLEAR(m_ਵਰਤਿਆ_Pins);
        VL_RESTORER_CLEAR(m_ਵਰਤਿਆ_Def_Param_Pins);
        UASSERT_OBJ(nodep->modp(), nodep,
                    "Instance has unlinked module");  // V3LinkCell should have errored out
        VL_RESTORER(m_cellp);
        VL_RESTORER(m_pinSymp);
        {
            m_cellp = nodep;
            ਜੇ (VN_IS(nodep->modp(), Not_ਮਿਲਿਆ_Module)) {
                // Prevent warnings about missing pin connects
                ਜੇ (nodep->pinsp()) nodep->pinsp()->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ()->delete_ਰੁੱਖ();
                ਜੇ (nodep->paramsp()) nodep->paramsp()->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ()->delete_ਰੁੱਖ();
            }
            // Need to pass the module info to this cell, so we can link up the pin names
            // However can't use m_curSymp as pin connections need to use the
            // instantiator's symbols
            ਨਹੀਂਤਾਂ {
                m_pinSymp = m_statep->get_ਗੰਢ_Sym(nodep->modp());
                UINFO(4, indent() << "(Backto) visit " << nodep);
                // UINFOTREE(1, nodep, "", "linkcell");
                // UINFOTREE(1, nodep->modp(), "", "linkcemd");
                iterateChildren(nodep);

                ਜੇ (m_statep->forPrimary())
                    ਜੇ (ਅਚਲ AstModule* ਅਚਲ modp = VN_CAST(nodep->modp(), Module)) {
                        ਜੇ (modp->hasGenericIface())
                            ਜੋੜੋ_Implicit_Parameters_ਦਾ_Generic_Iface(nodep, modp);
                    }
            }
        }
        // Parent module inherits child's publicity
        // This is done bottom up in the LinkBotupVisitor stage
    }
    ਖਾਲੀ ਦੇਖੋ(AstClassRefDType* nodep) override {
        // Cell: Recurse inside or cleanup not founds
        LINKDOT_ਦੇਖੋ_ਸ਼ੁਰੂ();
        UINFO(5, indent() << "visit " << nodep);
        // Can be under dot if called as package::class and that class resolves, so no checkNoDot
        VL_RESTORER_CLEAR(m_ਵਰਤਿਆ_Pins);
        VL_RESTORER_CLEAR(m_ਵਰਤਿਆ_Def_Param_Pins);
        UASSERT_OBJ(nodep->classp(), nodep, "ClassRef has unlinked class");
        UASSERT_OBJ(m_statep->forPrimary() || !nodep->paramsp() || V3Error::error_ਗਿਣਤੀ(), nodep,
                    "class reference parameter not removed by V3Param");
        VL_RESTORER(m_pinSymp);
        {
            // ClassRef's have pins, so track
            m_pinSymp = m_statep->get_ਗੰਢ_Sym(nodep->classp());
            UINFO(4, indent() << "(Backto) visit " << nodep);
            // UINFOTREE(1, nodep, "", "linkcell");
            // UINFOTREE(1, nodep->modp(), "", "linkcemd");
            iterateChildren(nodep);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstPin* nodep) override {
        // Pin: Link to submodule's port
        LINKDOT_ਦੇਖੋ_ਸ਼ੁਰੂ();
        checkNoDot(nodep);
        iterateChildren(nodep);
        ਜੇ (!nodep->modVarp()) {
            UASSERT_OBJ(m_pinSymp, nodep, "Pin not under instance?");
            VSymEnt* ਅਚਲ foundp = m_pinSymp->ਲੱਭੋ_Id_Flat(nodep->name());
            ਅਚਲ ਅੱਖਰ* ਅਚਲ whatp = nodep->param() ? "parameter" : "pin";
            ਜੇ (!foundp) {
                ਜੇ (m_cellp && VN_IS(m_cellp->modp(), Primitive)
                    && (nodep->name() == "__paramNumber1" || nodep->name() == "__paramNumber2")) {
                    // Primitive parameter is really a delay2 we can just ignore
                    VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep->unlink_Fr_ਪਿੱਠ()), nodep);
                    ਵਾਪਸ;
                } ਨਹੀਂਤਾਂ ਜੇ (nodep->param() && !nodep->exprp() && !nodep->svDotName()) {
                    // Placeholder pin the parser makes for an empty '#()', to signal
                    // the user did type the '#()'. No further use, deleting.
                    VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep->unlink_Fr_ਪਿੱਠ()), nodep);
                    ਵਾਪਸ;
                } ਨਹੀਂਤਾਂ {
                    ਅਚਲ std::string suggest
                        = (nodep->param()
                               ? m_statep->suggestSymFlat(m_pinSymp, nodep->name(),
                                                          Link_ਗੰਢ_Matcher_Var_Param{}, ਝੂਠ)
                               : m_statep->suggestSymFlat(m_pinSymp, nodep->name(),
                                                          Link_ਗੰਢ_Matcher_Var_IO{}, ਝੂਠ));
                    ਅਚਲ std::string decl
                        = ((m_cellp && m_cellp->modp())
                               ? "\n"s + nodep->warn_ਹੋਰ() + "... Location of instance's "
                                     + m_cellp->modp()->verilogKwd() + " declaration\n"
                                     + m_cellp->modp()->warnContextSecondary()
                               : "");
                    nodep->v3warn(PINNOTFOUND,
                                  ucfirst(whatp)
                                      << " not found: " << nodep->prettyNameQ() << '\n'
                                      << (suggest.ਖਾਲੀ_ਥਾਵਾਂ() ? "" : nodep->warn_ਹੋਰ() + suggest)
                                      << '\n'
                                      << nodep->warnContextPrimary() << decl);
                    VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep->unlink_Fr_ਪਿੱਠ()), nodep);
                    ਵਾਪਸ;
                }
            }
            VVarType refVarType = VVarType::UNKNOWN;
            ਤਾਰਕਿਕ wrongPinType = ਝੂਠ;
            ਜੇ (AstVar* ਅਚਲ varp = VN_CAST(foundp->nodep(), Var)) {
                ਜੇ (varp->isIO() || varp->isParam() || varp->isIfaceRef()) {
                    refVarType = varp->varType();
                    nodep->modVarp(varp);
                } ਨਹੀਂਤਾਂ {
                    wrongPinType = ਸੱਚ;
                }
            } ਨਹੀਂਤਾਂ ਜੇ (AstParamTypeDType* ਅਚਲ typep = VN_CAST(foundp->nodep(), ParamTypeDType)) {
                refVarType = typep->varType();
                nodep->modPTypep(typep);
            } ਨਹੀਂਤਾਂ {
                wrongPinType = ਸੱਚ;
            }
            // Don't connect parameter pin to module ports or vice versa
            ਜੇ (nodep->param() != (refVarType == VVarType::GPARAM)) wrongPinType = ਸੱਚ;
            ਜੇ (wrongPinType) {
                string ਨਿਸ਼ਾਨਾ_Type = LinkDotState::ਗੰਢ_ਪਾਠ_Type(foundp->nodep());
                ਨਿਸ਼ਾਨਾ_Type = VString::aOrAn(ਨਿਸ਼ਾਨਾ_Type) + ' ' + ਨਿਸ਼ਾਨਾ_Type;
                ਜੇ (nodep->param()) {
                    nodep->v3error("Instance attempts to override "
                                   << nodep->prettyNameQ() << " as a " << whatp << ", but it is "
                                   << ਨਿਸ਼ਾਨਾ_Type);
                } ਨਹੀਂਤਾਂ {
                    nodep->v3error("Instance attempts to connect to "
                                   << nodep->prettyNameQ() << ", but it is " << ਨਿਸ਼ਾਨਾ_Type);
                }
                ਵਾਪਸ;
            }
            markAndCheckPinDup(nodep, foundp->nodep(), whatp);
        }
        // Early return() above when deleted
    }

    ਸਥਿਰ ਤਾਰਕਿਕ isDotThisReference(ਅਚਲ AstDot* nodep) {
        ਵਾਪਸ VN_IS(nodep->lhsp(), ParseRef) && nodep->lhsp()->name() == "this";
    }

    ਖਾਲੀ ਦੇਖੋ(AstDot* nodep) override {
        // Legal under a DOT: AstDot, AstParseRef, AstPackageRef, AstNodeSel, AstPackArrayDType
        //    also a DOT can be part of an expression, but only above plus
        //    AstFTaskRef are legal children
        // Dot(PackageRef, ParseRef(text))
        // Dot(Dot(ClassOrPackageRef,ClassOrPackageRef), ParseRef(text))
        // Dot(Dot(Dot(ParseRef(text), ...
        ਜੇ (nodep->user3_ਸੈੱਟ_Once()) ਵਾਪਸ;
        LINKDOT_ਦੇਖੋ_ਸ਼ੁਰੂ();
        UINFO(8, indent() << "visit " << nodep);
        UINFO(8, indent() << m_ds.ascii());
        ਅਚਲ Dot_ਹਾਲਤਾਂ ਆਖਰੀ_ਹਾਲਤਾਂ = m_ds;
        ਅਚਲ ਤਾਰਕਿਕ ਸ਼ੁਰੂ = (m_ds.m_dotPos == DP_NONE);  // Save, as m_dotp will be changed
        ਅਚਲ DotPosition initialDotPos = m_ds.m_dotPos;
        VL_RESTORER(m_randSymp);
        {
            ਜੇ (ਸ਼ੁਰੂ) {  // Starting dot sequence
                UINFOTREE(9, nodep, "", "dot-in");
                // Keep deferral from visit(AstMethodCall) for unspecialized rand target
                m_ds.init(m_curSymp, m_ds.m_unresolvedClass && rand_ਨਿਸ਼ਾਨਾ_Unresolved());
            }
            m_ds.m_dotp = nodep;  // Always, not just at start
            m_ds.m_dotPos = DP_ਪਹਿਲਾ;

            ਜੇ (isDotThisReference(nodep)) {
                VSymEnt* classSymp = getThisClassSymp();
                // In 'randomize() with { this.member }', 'this' refers to randomized
                // object, not the calling class (IEEE 1800-2023 18.7)
                ਜੇ (m_randSymp && m_currentWithp) classSymp = m_randSymp;
                ਜੇ (!classSymp && rand_ਨਿਸ਼ਾਨਾ_Unresolved()) {
                    // Bind 'this' in post-V3Param pass
                } ਨਹੀਂਤਾਂ ਜੇ (!classSymp) {
                    nodep->v3error("'this' used outside class (IEEE 1800-2023 8.11)");
                    m_ds.m_dotErr = ਸੱਚ;
                } ਨਹੀਂਤਾਂ {
                    m_ds.m_dotSymp = classSymp;
                    UINFO(8, indent() << "this. " << m_ds.ascii());
                }
            } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(nodep->lhsp(), ParseRef) && nodep->lhsp()->name() == "super") {
                ਅਚਲ VSymEnt* classSymp = getThisClassSymp();
                ਜੇ (!classSymp) {
                    nodep->v3error("'super' used outside class (IEEE 1800-2023 8.15)");
                    m_ds.m_dotErr = ਸੱਚ;
                } ਨਹੀਂਤਾਂ {
                    ਅਚਲ ਸਵੈਚਾਲਿਤ classp = VN_AS(classSymp->nodep(), Class);
                    ਅਚਲ ਸਵੈਚਾਲਿਤ cextp = classp->extendsp();
                    ਜੇ (!cextp) {
                        nodep->v3error("'super' used on non-extended class (IEEE 1800-2023 8.15)");
                        m_ds.m_dotErr = ਸੱਚ;
                    } ਨਹੀਂਤਾਂ {
                        ਜੇ (m_statep->forPrimary()
                            && m_extendsParam.ਲੱਭੋ(classp) != m_extendsParam.ਸਿਰਾ()) {
                            UINFO(9, indent()
                                         << "deferring until post-V3Param: " << nodep->lhsp());
                            m_ds.m_unresolvedClass = ਸੱਚ;
                        } ਨਹੀਂਤਾਂ {
                            ਅਚਲ ਸਵੈਚਾਲਿਤ ਅਧਾਰ_Classp = cextp->classp();
                            UASSERT_OBJ(ਅਧਾਰ_Classp, nodep, "Bad superclass");
                            m_ds.m_dotSymp = m_statep->get_ਗੰਢ_Sym(ਅਧਾਰ_Classp);
                            m_ds.m_super = ਸੱਚ;
                            UINFO(8, indent() << "super. " << m_ds.ascii());
                        }
                    }
                }
            } ਨਹੀਂਤਾਂ ਜੇ (AstClassOrPackageRef* ਅਚਲ lhsp
                       = VN_CAST(nodep->lhsp(), ClassOrPackageRef)) {
                // m_ds.m_dotText communicates the cell prefix between stages
                UINFO(8, indent() << "iter.lhs   " << m_ds.ascii() << " " << nodep);
                iterate_And_ਅਗਲਾ_Null(lhsp);
                ਜੇ (!lhsp->classOrPackageSkipp() && lhsp->name() != "local::") {
                    revisitLater(nodep);
                    m_ds = ਆਖਰੀ_ਹਾਲਤਾਂ;
                    // Resolve function args before bailing
                    ਜੇ (Ast_ਗੰਢ_FTask_Ref* ਅਚਲ ftaskrefp = VN_CAST(nodep->rhsp(), ਗੰਢ_FTask_Ref)) {
                        iterate_And_ਅਗਲਾ_Null(ftaskrefp->argsp());
                    }
                    ਵਾਪਸ;
                }
                m_ds.m_dotPos = DP_PACKAGE;
                // nodep->lhsp() may be a new node
                ਜੇ (AstClassOrPackageRef* ਅਚਲ classOrPackageRefp
                    = VN_CAST(nodep->lhsp(), ClassOrPackageRef)) {
                    ਜੇ (Ast_ਗੰਢ* ਅਚਲ classOrPackageNodep
                        = classOrPackageRefp->classOrPackageSkipp()) {
                        m_ds.m_dotSymp = m_statep->get_ਗੰਢ_Sym(classOrPackageNodep);
                    }
                }
                UINFO(8, indent() << "iter.ldone " << m_ds.ascii() << " " << nodep);
            } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(nodep->lhsp(), Dot) && VN_AS(nodep->lhsp(), Dot)->colon()) {
                // m_ds.m_dotText communicates the cell prefix between stages
                UINFO(8, indent() << "iter.lhs   " << m_ds.ascii() << " " << nodep);
                m_ds.m_dotPos = DP_PACKAGE;
                iterate_And_ਅਗਲਾ_Null(nodep->lhsp());
                // nodep->lhsp() may be a new node
                ਜੇ (AstClassOrPackageRef* ਅਚਲ crefp
                    = VN_CAST(nodep->lhsp(), ClassOrPackageRef)) {
                    ਜੇ (!crefp->classOrPackageSkipp()) {
                        revisitLater(nodep);
                        m_ds = ਆਖਰੀ_ਹਾਲਤਾਂ;
                        // Resolve function args before bailing
                        ਜੇ (Ast_ਗੰਢ_FTask_Ref* ਅਚਲ ftaskrefp
                            = VN_CAST(nodep->rhsp(), ਗੰਢ_FTask_Ref)) {
                            iterate_And_ਅਗਲਾ_Null(ftaskrefp->argsp());
                        }
                        ਵਾਪਸ;
                    }
                }
                ਜੇ (m_ਆਖਰੀ_Deferredp == nodep->lhsp()) {
                    // LHS got deferred, so this node won't be resolved. Defer it too
                    m_ds = ਆਖਰੀ_ਹਾਲਤਾਂ;
                    // Resolve function args before bailing
                    ਜੇ (Ast_ਗੰਢ_FTask_Ref* ਅਚਲ ftaskrefp = VN_CAST(nodep->rhsp(), ਗੰਢ_FTask_Ref)) {
                        iterate_And_ਅਗਲਾ_Null(ftaskrefp->argsp());
                    }
                    ਵਾਪਸ;
                }
                UINFO(8, indent() << "iter.ldone " << m_ds.ascii() << " " << nodep);
            } ਨਹੀਂਤਾਂ {
                m_ds.m_dotPos = DP_ਪਹਿਲਾ;
                m_ds.m_unresolvedGenericIface = ਝੂਠ;
                UINFO(8, indent() << "iter.lhs   " << m_ds.ascii() << " " << nodep);
                iterate_And_ਅਗਲਾ_Null(nodep->lhsp());
                UINFO(8, indent() << "iter.ldone " << m_ds.ascii() << " " << nodep);
                ਜੇ (m_ds.m_unresolvedGenericIface) {
                    m_ds.m_dotPos = initialDotPos;
                    ਵਾਪਸ;
                }
                // UINFOTREE(9, nodep, "", "dot-lho");
            }
            ਜੇ (m_statep->forPrimary() && isParamedClassRef(nodep->lhsp())) {
                // Dots of paramed classes will be linked after deparameterization
                UINFO(9, indent() << "deferring until post-V3Param: " << nodep->lhsp());
                m_ds.m_unresolvedClass = ਸੱਚ;
            }
            ਜੇ (m_ds.m_unresolvedCell
                && (VN_IS(nodep->lhsp(), CellRef) || VN_IS(nodep->lhsp(), CellArrayRef))) {
                m_ds.m_unlinkedScopep = nodep->lhsp();
            }
            ਜੇ (m_ds.m_dotPos == DP_ਪਹਿਲਾ) m_ds.m_dotPos = DP_SCOPE;
            ਜੇ (!m_ds.m_dotErr) {  // Once something wrong, give up
                // Top 'final' dot RHS is final RHS, else it's a
                // DOT(DOT(x,*here*),real-rhs) which we consider a RHS
                ਜੇ (ਸ਼ੁਰੂ && m_ds.m_dotPos == DP_SCOPE) m_ds.m_dotPos = DP_FINAL;
                UINFO(8, indent() << "iter.rhs   " << m_ds.ascii() << " " << nodep);
                // m_ds.m_dotSymp points at lhsp()'s symbol table, so resolve RHS under that
                iterate_And_ਅਗਲਾ_Null(nodep->rhsp());
                UINFO(8, indent() << "iter.rdone " << m_ds.ascii() << " " << nodep);
                // UINFOTREE(9, nodep, "", "dot-rho");
            }
            ਜੇ (!m_ds.m_unresolvedClass) {
                ਜੇ (ਸ਼ੁਰੂ) {
                    Ast_ਗੰਢ* newp;
                    ਜੇ (m_ds.m_dotErr) {
                        newp = ਨਵਾਂ AstConst{nodep->fileline(), AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_False{}};
                    } ਨਹੀਂਤਾਂ {
                        newp = nodep->rhsp()->unlink_Fr_ਪਿੱਠ();
                    }
                    UINFOTREE(9, newp, "", "dot-out");
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
                } ਨਹੀਂਤਾਂ {  // Dot midpoint
                    Ast_ਗੰਢ* newp = nodep->rhsp()->unlink_Fr_ਪਿੱਠ();
                    ਜੇ (m_ds.m_unresolvedCell) {
                        AstCellRef* ਅਚਲ crp = ਨਵਾਂ AstCellRef{
                            nodep->fileline(), nodep->name(), nodep->lhsp()->unlink_Fr_ਪਿੱਠ(), newp};
                        newp = crp;
                    }
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
                }
            }
        }
        ਜੇ (ਸ਼ੁਰੂ) {
            m_ds = ਆਖਰੀ_ਹਾਲਤਾਂ;
        } ਨਹੀਂਤਾਂ {
            ਅਚਲ ਤਾਰਕਿਕ unresolvedClass = m_ds.m_unresolvedClass;
            m_ds.m_dotp = ਆਖਰੀ_ਹਾਲਤਾਂ.m_dotp;
            m_ds.m_unresolvedClass |= unresolvedClass;
        }
        UINFO(8, indent() << "done " << m_ds.ascii() << " " << nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstDefaultClocking* nodep) override {
        ਜੇ (VSymEnt* ਅਚਲ foundp = m_curSymp->ਲੱਭੋ_Id_Fallback(nodep->name())) {
            ਜੇ (AstClocking* ਅਚਲ clockingp = VN_CAST(foundp->nodep(), Clocking)) {
                clockingp->makeDefault();
                VL_DO_DANGLING(nodep->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ(), nodep);
            } ਨਹੀਂਤਾਂ {
                nodep->v3error(nodep->prettyNameQ() << " is not a clocking identifier");
            }
        } ਨਹੀਂਤਾਂ {
            nodep->v3error("Can't find definition of clocking: " << nodep->prettyNameQ());
        }
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Sen_ਵਸਤੂ* nodep) override {
        LINKDOT_ਦੇਖੋ_ਸ਼ੁਰੂ();
        VL_RESTORER(m_ਵਿੱਚ_Sens);
        m_ਵਿੱਚ_Sens = ਸੱਚ;
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstPatMember* nodep) override {
        LINKDOT_ਦੇਖੋ_ਸ਼ੁਰੂ();
        ਜੇ (nodep->varrefp()) ਵਾਪਸ;  // only do this mapping once
        // If we have a TEXT token as our key, lookup if it's a LPARAM
        ਜੇ (Ast_ਪਾਠ* ਅਚਲ textp = VN_CAST(nodep->keyp(), ਪਾਠ_2)) {
            UINFO(9, indent() << "visit " << nodep);
            UINFO(9, indent() << "      " << textp);
            // Lookup
            ਜੇ (VSymEnt* ਅਚਲ foundp = m_curSymp->ਲੱਭੋ_Id_Fallback(textp->ਪਾਠ())) {
                ਜੇ (AstVar* ਅਚਲ varp = VN_CAST(foundp->nodep(), Var)) {
                    ਜੇ (varp->isParam() || varp->isGenVar()) {
                        // Attach found Text reference to PatMember
                        nodep->varrefp(
                            ਨਵਾਂ AstVarRef{nodep->fileline(),
                                          foundp->imported() ? foundp->classOrPackagep() : ਖਾਲੀ_ਸੂਚਕ,
                                          varp, VAccess::READ});
                        UINFO(9, indent() << " new " << nodep->varrefp());
                    }
                }
                ਜੇ (Ast_Enum_ਵਸਤੂ* ਅਚਲ itemp = VN_CAST(foundp->nodep(), Enum_ਵਸਤੂ)) {
                    // Attach enum item value to PatMember
                    nodep->varrefp(
                        ਨਵਾਂ Ast_Enum_ਵਸਤੂ_Ref{nodep->fileline(), itemp, foundp->classOrPackagep()});
                    UINFO(9, indent() << " new " << itemp);
                }
            }
        }
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstParseRef* nodep) override {
        ਜੇ (nodep->user3_ਸੈੱਟ_Once()) ਵਾਪਸ;
        LINKDOT_ਦੇਖੋ_ਸ਼ੁਰੂ();
        UINFO(9, indent() << "visit " << nodep);
        UINFO(9, indent() << m_ds.ascii());
        ਜੇ (m_ds.m_unresolvedClass) ਵਾਪਸ;
        // m_curSymp is symbol table of outer expression
        // m_ds.m_dotSymp is symbol table relative to "."'s above now
        UASSERT_OBJ(m_ds.m_dotSymp, nodep, "nullptr lookup symbol table");
        // Generally resolved during Primary, but might be at param time under AstUnlinkedRef
        UASSERT_OBJ(!m_statep->forScopeCreation(), nodep, "ParseRefs should no longer exist");
        ਅਚਲ Dot_ਹਾਲਤਾਂ ਆਖਰੀ_ਹਾਲਤਾਂ = m_ds;
        ਅਚਲ ਤਾਰਕਿਕ ਸ਼ੁਰੂ = (m_ds.m_dotPos == DP_NONE);  // Save, as m_dotp will be changed
        ਤਾਰਕਿਕ ਪਹਿਲਾ = ਸ਼ੁਰੂ || m_ds.m_dotPos == DP_ਪਹਿਲਾ;

        ਜੇ (ਸ਼ੁਰੂ) {
            m_ds.init(m_curSymp);
            // Note m_ds.m_dotp remains nullptr; this is a reference not under a dot
        }
        ਜੇ (nodep->name() == "super") {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: super");
            m_ds.m_dotErr = ਸੱਚ;
        }
        ਜੇ (nodep->name() == "this") {
            iterateChildren(nodep);
            ਜੇ (m_statep->forPrimary()) ਵਾਪਸ;  // The class might be parameterized somewhere
            ਅਚਲ VSymEnt* classSymp = getThisClassSymp();
            ਜੇ (!classSymp) {
                nodep->v3error("'this' used outside class (IEEE 1800-2023 8.11)");
                ਵਾਪਸ;
            }
            AstClass* ਅਚਲ classp = VN_AS(classSymp->nodep(), Class);
            AstClassRefDType* ਅਚਲ dtypep
                = ਨਵਾਂ AstClassRefDType{nodep->fileline(), classp, ਖਾਲੀ_ਸੂਚਕ};
            AstThisRef* ਅਚਲ newp = ਨਵਾਂ AstThisRef{nodep->fileline(), VFlag_ਬੱਚਾ_DType{}, dtypep};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
            ਵਾਪਸ;
        }
        ਜੇ (m_ds.m_dotPos == DP_MEMBER && VN_IS(m_ds.m_dotp->lhsp(), LambdaArgRef)
            && nodep->name() == "index") {
            // 'with' statement's 'item.index'
            // m_ds.dotp->lhsp() was checked to know if `index` is directly after lambda arg ref.
            // If not, treat it as normal member select
            iterateChildren(nodep);
            AstLambdaArgRef* ਅਚਲ newp = ਨਵਾਂ AstLambdaArgRef{
                nodep->fileline(), m_ds.m_dotp->lhsp()->name() + "__DOT__index", ਸੱਚ};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
            ਵਾਪਸ;
        } ਨਹੀਂਤਾਂ ਜੇ (m_ds.m_dotPos == DP_MEMBER) {
            // Found a Var, everything following is membership.  {scope}.{var}.HERE {member}
            ਜੇ (m_ds.m_dotErr) {
                nodep->unlink_Fr_ਪਿੱਠ();  // Avoid circular node loop on errors
            } ਨਹੀਂਤਾਂ {
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ varEtcp = VN_AS(m_ds.m_dotp->lhsp()->unlink_Fr_ਪਿੱਠ(), ਗੰਢ_ਵਿਅੰਜਕ);
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ newp = ਨਵਾਂ AstMemberSel{nodep->fileline(), varEtcp,  //
                                                           VFlag_ਬੱਚਾ_DType{}, nodep->name()};
                nodep->replaceWith(newp);
            }
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
        } ਨਹੀਂਤਾਂ {
            //
            string expectWhat;
            ਤਾਰਕਿਕ allowScope = ਝੂਠ;
            ਤਾਰਕਿਕ allowVar = ਝੂਠ;
            ਤਾਰਕਿਕ allowFTask = ਝੂਠ;
            ਤਾਰਕਿਕ staticAccess = ਝੂਠ;
            ਜੇ (m_ds.m_disablep) {
                allowScope = ਸੱਚ;
                allowFTask = ਸੱਚ;
                expectWhat = "block/task";
            } ਨਹੀਂਤਾਂ ਜੇ (m_ds.m_dotPos == DP_PACKAGE) {
                // {package-or-class}::{a}
                Ast_ਗੰਢ_Module* classOrPackagep = ਖਾਲੀ_ਸੂਚਕ;
                expectWhat = "scope/variable/func";
                allowScope = ਸੱਚ;
                allowVar = ਸੱਚ;
                allowFTask = ਸੱਚ;
                staticAccess = ਸੱਚ;
                UINFO(9, "chk pkg " << m_ds.ascii() << " lhsp=" << m_ds.m_dotp->lhsp());
                UASSERT_OBJ(VN_IS(m_ds.m_dotp->lhsp(), ClassOrPackageRef), m_ds.m_dotp->lhsp(),
                            "Bad package link");
                AstClassOrPackageRef* ਅਚਲ cpackagerefp
                    = VN_AS(m_ds.m_dotp->lhsp(), ClassOrPackageRef);
                ਜੇ (cpackagerefp->name() == "local::") {
                    m_randSymp = ਖਾਲੀ_ਸੂਚਕ;
                    ਪਹਿਲਾ = ਸੱਚ;
                } ਨਹੀਂਤਾਂ ਜੇ (!cpackagerefp->classOrPackageSkipp()) {
                    VSymEnt* ਅਚਲ foundp = m_statep->resolveClassOrPackage(
                        m_ds.m_dotSymp, cpackagerefp, ਸੱਚ, ਝੂਠ, ":: reference");
                    ਜੇ (!foundp) ਵਾਪਸ;
                    classOrPackagep = cpackagerefp->classOrPackageSkipp();
                    ਜੇ (classOrPackagep) {
                        m_ds.m_dotSymp = m_statep->get_ਗੰਢ_Sym(classOrPackagep);
                    } ਨਹੀਂਤਾਂ {
                        m_ds = ਆਖਰੀ_ਹਾਲਤਾਂ;
                        ਵਾਪਸ;
                    }
                } ਨਹੀਂਤਾਂ {
                    classOrPackagep = cpackagerefp->classOrPackageSkipp();
                    UASSERT_OBJ(classOrPackagep, m_ds.m_dotp->lhsp(), "Bad package link");
                    m_ds.m_dotSymp = m_statep->get_ਗੰਢ_Sym(classOrPackagep);
                }
                m_ds.m_dotPos = DP_SCOPE;
            } ਨਹੀਂਤਾਂ ਜੇ (m_ds.m_dotPos == DP_SCOPE || m_ds.m_dotPos == DP_ਪਹਿਲਾ) {
                // {a}.{b}, where {a} maybe a module name
                // or variable, where dotting into structure member
                expectWhat = "scope/variable";
                allowScope = ਸੱਚ;
                allowVar = ਸੱਚ;
            } ਨਹੀਂਤਾਂ ਜੇ (m_ds.m_dotPos == DP_NONE) {
                expectWhat = "variable";
                allowVar = ਸੱਚ;
            } ਨਹੀਂਤਾਂ ਜੇ (m_ds.m_dotPos == DP_FINAL) {
                expectWhat = "variable/method";
                allowFTask = ਸੱਚ;
                allowVar = ਸੱਚ;
            } ਨਹੀਂਤਾਂ {
                UINFO(1, "ds=" << m_ds.ascii());
                nodep->v3fatalSrc("Unhandled DotPosition");
            }
            // Lookup
            VSymEnt* foundp;
            string baddot;
            VSymEnt* okSymp = ਖਾਲੀ_ਸੂਚਕ;
            // Restricted 'with (id_list)': only id_list names bind into target class.
            ਜੇ (m_randSymp
                && (!m_currentWithp || m_currentWithp->name_Resolves_ਮੰਜ਼ਿਲ_ਨਿਸ਼ਾਨਾ(nodep->name()))) {
                foundp = m_randSymp->ਲੱਭੋ_Id_Flat(nodep->name());
                ਜੇ (foundp) {
                    ਜੇ (m_currentWithp && m_currentWithp->restricted()) {
                        m_restricted_ਨਾਂ_ਵਰਤਿਆ.ਪਾਓ(nodep->name());
                    }
                    ਜੇ (!ਸ਼ੁਰੂ) m_ds.m_dotPos = DP_MEMBER;
                    ਜੇ (!m_currentWithp) {
                        UASSERT_OBJ(m_rand_ਵਿਧੀ_Callp, nodep, "Expected to be under randomize()");
                        // This will start failing once complex expressions are allowed on the LHS
                        // of randomize() with args
                        UASSERT_OBJ(VN_IS(m_rand_ਵਿਧੀ_Callp->fromp(), VarRef), m_rand_ਵਿਧੀ_Callp,
                                    "Expected simple randomize target");
                        // A ParseRef is used here so that the dot RHS gets resolved
                        AstMemberSel* ਅਚਲ newp = ਨਵਾਂ AstMemberSel{
                            nodep->fileline(), m_rand_ਵਿਧੀ_Callp->fromp()->clone_ਰੁੱਖ(ਝੂਠ),
                            VFlag_ਬੱਚਾ_DType{}, nodep->name()};
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
                        ਵਾਪਸ;
                    }
                    UINFO(9, indent() << "randomize-with fromSym " << foundp->nodep());
                    AstLambdaArgRef* ਅਚਲ lambdaRefp
                        = ਨਵਾਂ AstLambdaArgRef{nodep->fileline(), "__Vrandwith_obj", ਝੂਠ};
                    AstMemberSel* newp = ਨਵਾਂ AstMemberSel{nodep->fileline(), lambdaRefp,
                                                          VFlag_ਬੱਚਾ_DType{}, nodep->name()};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
                    ਵਾਪਸ;
                }
            }
            ਜੇ (allowScope) {
                foundp = m_statep->ਲੱਭੋ_Dotted(nodep->fileline(), m_ds.m_dotSymp, nodep->name(),
                                              baddot, okSymp, ਪਹਿਲਾ);  // Maybe nullptr
            } ਨਹੀਂਤਾਂ ਜੇ (ਪਹਿਲਾ) {
                foundp = m_ds.m_dotSymp->ਲੱਭੋ_Id_Fallback(nodep->name());
            } ਨਹੀਂਤਾਂ {
                foundp = m_ds.m_dotSymp->ਲੱਭੋ_Id_Flat(nodep->name());
            }
            ਜੇ (!foundp && m_ds.m_dotp && isDotThisReference(m_ds.m_dotp)) {
                ਅਚਲ AstClass* ਅਚਲ classp = VN_CAST(m_ds.m_dotSymp->nodep(), Class);
                ਜੇ (classp && classp->isCovergroup() && classp->covergroupEnclosingClassp()) {
                    VSymEnt* ਅਚਲ enclosingClassSymp
                        = m_statep->get_ਗੰਢ_Sym(classp->covergroupEnclosingClassp());
                    foundp = enclosingClassSymp->ਲੱਭੋ_Id_Fallback(nodep->name());
                    ਜੇ (foundp) m_ds.m_dotSymp = enclosingClassSymp;
                }
            }
            // If not found in modport, check interface fallback for parameters and typedefs.
            // Parameters and typedefs are always visible through a modport (IEEE 1800-2023 25.5).
            // This mirrors the VarXRef modport parameter fallback in visit(AstVarXRef).
            ਜੇ (!foundp && VN_IS(m_ds.m_dotSymp->nodep(), Modport)
                && m_ds.m_dotSymp->fallbackp()) {
                VSymEnt* ਅਚਲ ifaceFallbackp = m_ds.m_dotSymp->fallbackp();
                VSymEnt* ਅਚਲ ifaceFoundp = ifaceFallbackp->ਲੱਭੋ_Id_Flat(nodep->name());
                ਜੇ (ifaceFoundp) {
                    ਜੇ (ਅਚਲ AstVar* ਅਚਲ varp = VN_CAST(ifaceFoundp->nodep(), Var)) {
                        ਜੇ (varp->isParam()) foundp = ifaceFoundp;
                    } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(ifaceFoundp->nodep(), Typedef)
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
            ਜੇ (!foundp && !okSymp && VN_IS(m_ds.m_dotSymp->nodep(), Modport)) {
                okSymp = m_ds.m_dotSymp;
            }
            ਜੇ (foundp) {
                UINFO(9, indent() << "found=se" << cvt_ਮੰਜ਼ਿਲ_Hex(foundp) << "  exp=" << expectWhat
                                  << "  n=" << foundp->nodep());
            }
            // What fell out?
            ਤਾਰਕਿਕ ok = ਝੂਠ;
            // Special case: waiting on clocking event
            ਜੇ (m_ਵਿੱਚ_Sens && foundp && m_ds.m_dotPos != DP_SCOPE && m_ds.m_dotPos != DP_ਪਹਿਲਾ) {
                ਜੇ (AstClocking* ਅਚਲ clockingp = sensClockingp(foundp->nodep())) {
                    foundp = getCreateClockingEventSymEnt(clockingp);
                }
            }
            ਜੇ (!foundp) {
            } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(foundp->nodep(), Cell) || VN_IS(foundp->nodep(), ਗੰਢ_ਖੰਡ)
                       || VN_IS(foundp->nodep(), Gen_ਖੰਡ)
                       || VN_IS(foundp->nodep(), Netlist)  // for $root
                       || VN_IS(foundp->nodep(), Module)) {  // if top
                ਜੇ (allowScope) {
                    ok = ਸੱਚ;
                    m_ds.m_dot_ਪਾਠ = VString::dot(m_ds.m_dot_ਪਾਠ, ".", nodep->name());
                    m_ds.m_dotSymp = foundp;
                    m_ds.m_dotPos = DP_SCOPE;
                    ਜੇ (m_ds.m_disablep && VN_IS(foundp->nodep(), ਗੰਢ_ਖੰਡ)) {
                        // Possibly it is not the final link. If we are under dot and not in its
                        // last component, `targetp()` field will be overwritten by next components
                        m_ds.m_disablep->targetp(foundp->nodep());
                    }
                    ਜੇ (VN_IS(foundp->nodep(), Gen_ਖੰਡ)) {
                        m_ds.m_genBlk = ਸੱਚ;
                        ਜੇ (m_ds.m_disablep) {
                            m_ds.m_disablep->v3warn(
                                E_UNSUPPORTED,
                                "Unsupported: Generate block referenced by disable");
                        }
                    }
                    // Upper AstDot visitor will handle it from here
                } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(foundp->nodep(), Cell) && allowVar) {
                    AstCell* ਅਚਲ cellp = VN_AS(foundp->nodep(), Cell);
                    ਜੇ (VN_IS(cellp->modp(), Iface)) {
                        // Interfaces can be referenced like a variable for interconnect
                        VSymEnt* ਅਚਲ cellEntp = m_statep->get_ਗੰਢ_Sym(cellp);
                        UASSERT_OBJ(cellEntp, nodep, "No interface sym entry");
                        VSymEnt* ਅਚਲ ਮਾਪਾ_Entp
                            = cellEntp->parentp();  // Container of the var; probably a module or
                                                    // generate begin
                        AstVar* ਅਚਲ ifaceRefVarp
                            = ਲੱਭੋ_Iface_ਸਿਖਰ_Varp(nodep, ਮਾਪਾ_Entp, nodep->name());
                        //
                        ok = ਸੱਚ;
                        m_ds.m_dotSymp = foundp;
                        m_ds.m_dotPos = DP_SCOPE;
                        UINFO(9, indent() << " cell -> iface varref " << foundp->nodep());
                        Ast_ਗੰਢ_ਵਿਅੰਜਕ* newp;
                        ਜੇ (m_ds.m_dot_ਪਾਠ != "") {
                            newp = ਨਵਾਂ AstVarXRef{nodep->fileline(), ifaceRefVarp, m_ds.m_dot_ਪਾਠ,
                                                  VAccess::READ};
                            ਜੇ (m_ds.m_unresolvedCell && m_ds.m_unlinkedScopep) {
                                newp = ਨਵਾਂ AstUnlinkedRef{nodep->fileline(), newp, newp->name(),
                                                          m_ds.m_unlinkedScopep->unlink_Fr_ਪਿੱਠ()};
                                m_ds.m_unlinkedScopep = ਖਾਲੀ_ਸੂਚਕ;
                                m_ds.m_unresolvedCell = ਝੂਠ;
                                m_ds.m_dot_ਪਾਠ = "";
                            }
                        } ਨਹੀਂਤਾਂ {
                            AstVarRef* ਅਚਲ refp
                                = ਨਵਾਂ AstVarRef{nodep->fileline(), ifaceRefVarp, VAccess::READ};
                            newp = refp;
                        }
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
                    } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(cellp->modp(), Not_ਮਿਲਿਆ_Module)) {
                        cellp->modNameFileline()->v3error("Cannot find file containing interface: "
                                                          << cellp->modp()->prettyNameQ());
                    }
                }
            } ਨਹੀਂਤਾਂ ਜੇ (allowScope && !allowFTask && VN_IS(foundp->nodep(), ਗੰਢ_FTask)) {
                // Function/task used as scope for static variable access (IEEE std 1800-2017 6.21)
                // Don't set m_dotText so the static variable resolves as a VarRef rather than
                // VarXRef. This is so that V3Begin won't move it out of the function/task's scope.
                ok = ਸੱਚ;
                m_ds.m_dotSymp = foundp;
                m_ds.m_dotPos = DP_SCOPE;
            } ਨਹੀਂਤਾਂ ਜੇ (allowFTask && VN_IS(foundp->nodep(), ਗੰਢ_FTask)) {
                Ast_ਗੰਢ_FTask_Ref* taskrefp;
                ਜੇ (VN_IS(foundp->nodep(), Task)) {
                    taskrefp = ਨਵਾਂ AstTaskRef{nodep->fileline(), nodep->name()};
                } ਨਹੀਂਤਾਂ {
                    taskrefp = ਨਵਾਂ AstFuncRef{nodep->fileline(), nodep->name()};
                }
                nodep->replaceWith(taskrefp);
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
                m_ds = ਆਖਰੀ_ਹਾਲਤਾਂ;
                ਵਾਪਸ;
            } ਨਹੀਂਤਾਂ ਜੇ (AstVar* ਅਚਲ varp = ਮਿਲਿਆ_ਮੰਜ਼ਿਲ_Varp(foundp, nodep, VAccess::READ)) {
                // Preserve explicit 'this' references to members inherited from
                // generic classes until after specialization.
                ਜੇ (m_statep->forPrimary() && m_ds.m_dotp && isDotThisReference(m_ds.m_dotp)
                    && foundp->classOrPackagep() != m_modp
                    && VN_IS(foundp->classOrPackagep(), Class)
                    && foundp->classOrPackagep()->hasGParam()) {
                    m_ds.m_unresolvedClass = ਸੱਚ;
                    ਵਾਪਸ;
                }
                AstIfaceRefDType* ਅਚਲ ifacerefp
                    = LinkDotState::ifaceRefFromArray(varp->subDTypep());
                ਜੇ (varp->isIfaceRef() && m_genericIfaceModule
                    && VN_IS(varp->ਬੱਚਾ_DTypep(), IfaceGenericDType) && !ਸ਼ੁਰੂ) {
                    // Defer only dotted member access ('d.PARAM'), as V3Param must specialize
                    // first; a standalone ref ('.x(d)') resolves via allowVar below now
                    ok = ਸੱਚ;
                    m_ds.m_unresolvedGenericIface = ਸੱਚ;
                } ਨਹੀਂਤਾਂ ਜੇ (ifacerefp && varp->isIfaceRef()) {
                    UASSERT_OBJ(ifacerefp->iface_ਰਾਹੀਂ_Cellp(), ifacerefp, "Unlinked interface");
                    // Really this is a scope reference into an interface
                    UINFO(9, indent() << "varref-ifaceref " << m_ds.m_dot_ਪਾਠ << "  " << nodep);
                    m_ds.m_dot_ਪਾਠ = VString::dot(m_ds.m_dot_ਪਾਠ, ".", nodep->name());
                    // If modport specified, use modport symbol table for lookup of virtual ports
                    ਜੇ (ifacerefp->modportp() && m_statep->exists_ਗੰਢ_Sym(ifacerefp->modportp())) {
                        m_ds.m_dotSymp = m_statep->get_ਗੰਢ_Sym(ifacerefp->modportp());
                    } ਨਹੀਂਤਾਂ {
                        m_ds.m_dotSymp = m_statep->get_ਗੰਢ_Sym(ifacerefp->iface_ਰਾਹੀਂ_Cellp());
                    }
                    m_ds.m_dotPos = DP_SCOPE;
                    ok = ਸੱਚ;

                    AstVarRef* ਅਚਲ refp = ਨਵਾਂ AstVarRef{nodep->fileline(), varp, VAccess::READ};
                    nodep->replaceWith(refp);

                    VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
                } ਨਹੀਂਤਾਂ ਜੇ (allowVar) {
                    Ast_ਗੰਢ* newp;
                    ਜੇ (m_ds.m_dot_ਪਾਠ != "") {
                        AstVarXRef* ਅਚਲ refp
                            = ਨਵਾਂ AstVarXRef{nodep->fileline(), nodep->name(), m_ds.m_dot_ਪਾਠ,
                                             VAccess::READ};  // lvalue'ness computed later
                        refp->varp(varp);
                        // Not linked again after V3LinkLValue, so that reports writing it
                        ਜੇ (ਅਚਲ AstModportVarRef* ਅਚਲ mvarp
                            = VN_CAST(foundp->nodep(), ModportVarRef)) {
                            ਜੇ (m_statep->forParamed() && mvarp->direction().isReadOnly()) {
                                refp->readOnlyModport(ਸੱਚ);
                            }
                        }
                        refp->ਸ਼ਾਮਲ_ਹੈ_Gen_ਖੰਡ(m_ds.m_genBlk);
                        m_ds.m_dot_ਪਾਠ = "";
                        ਜੇ (m_ds.m_unresolvedCell && m_ds.m_unlinkedScopep) {
                            ਅਚਲ string dotted = refp->dotted();
                            ਅਚਲ ਮਾਪ_t pos = dotted.ਲੱਭੋ("__BRA__??__KET__");
                            // Arrays of interfaces all have the same parameters
                            ਜੇ (pos != string::npos && varp->isParam()
                                && VN_IS(m_ds.m_unlinkedScopep, CellArrayRef)) {
                                refp->dotted(dotted.substr(0, pos));
                                newp = refp;
                            } ਨਹੀਂਤਾਂ {
                                UINFO(9, indent() << "deferring until post-V3Param: " << refp);
                                newp = ਨਵਾਂ AstUnlinkedRef{nodep->fileline(), refp, refp->name(),
                                                          m_ds.m_unlinkedScopep->unlink_Fr_ਪਿੱਠ()};
                                m_ds.m_unlinkedScopep = ਖਾਲੀ_ਸੂਚਕ;
                                m_ds.m_unresolvedCell = ਝੂਠ;
                            }
                        } ਨਹੀਂਤਾਂ {
                            newp = refp;
                        }
                    } ਨਹੀਂਤਾਂ {
                        ਜੇ (staticAccess && !varp->lifetime().isStatic() && !varp->isParam()) {
                            // TODO bug4077
                            // nodep->v3error("Static access to non-static member variable "
                            //                << varp->prettyNameQ() << endl);
                        }
                        AstVarRef* ਅਚਲ refp = ਨਵਾਂ AstVarRef{
                            nodep->fileline(), varp, VAccess::READ};  // lvalue'ness computed later
                        refp->classOrPackagep(foundp->classOrPackagep());
                        newp = refp;
                    }
                    UINFO(9, indent() << "new " << newp);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
                    m_ds.m_dotPos = DP_MEMBER;
                    ok = ਸੱਚ;
                }
            } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ AstModport* ਅਚਲ modportp = VN_CAST(foundp->nodep(), Modport)) {
                // A scope reference into an interface's modport (not
                // necessarily at a pin connection)
                UINFO(9, indent() << "cell-ref-to-modport " << m_ds.m_dot_ਪਾਠ << "  " << nodep);
                UINFO(9, indent() << "unlinked " << m_ds.m_unlinkedScopep);
                UINFO(9,
                      indent() << "dotSymp " << m_ds.m_dotSymp << " " << m_ds.m_dotSymp->nodep());
                // Iface was the previously dotted component
                ਜੇ (!m_ds.m_dotSymp || !VN_IS(m_ds.m_dotSymp->nodep(), Cell)
                    || !VN_AS(m_ds.m_dotSymp->nodep(), Cell)->modp()
                    || !VN_IS(VN_AS(m_ds.m_dotSymp->nodep(), Cell)->modp(), Iface)) {
                    nodep->v3error("Modport not referenced as <interface>."
                                   << modportp->prettyNameQ());
                } ਨਹੀਂਤਾਂ ਜੇ (!VN_AS(m_ds.m_dotSymp->nodep(), Cell)->modp()
                           || !VN_IS(VN_AS(m_ds.m_dotSymp->nodep(), Cell)->modp(), Iface)) {
                    nodep->v3error("Modport not referenced from underneath an interface: "
                                   << modportp->prettyNameQ());
                } ਨਹੀਂਤਾਂ {
                    AstCell* ਅਚਲ cellp = VN_AS(m_ds.m_dotSymp->nodep(), Cell);
                    UASSERT_OBJ(cellp, nodep, "Modport not referenced from an instance");
                    VSymEnt* ਅਚਲ cellEntp = m_statep->get_ਗੰਢ_Sym(cellp);
                    UASSERT_OBJ(cellEntp, nodep, "No interface sym entry");
                    VSymEnt* ਅਚਲ ਮਾਪਾ_Entp
                        = cellEntp->parentp();  // Container of the var; probably a
                                                // module or generate begin
                    // We drop __BRA__??__KET__ as cells don't have that naming yet
                    AstVar* ਅਚਲ ifaceRefVarp
                        = ਲੱਭੋ_Iface_ਸਿਖਰ_Varp(nodep, ਮਾਪਾ_Entp, cellp->name());
                    //
                    ok = ਸੱਚ;
                    m_ds.m_dot_ਪਾਠ = VString::dot(m_ds.m_dot_ਪਾਠ, ".", nodep->name());
                    m_ds.m_dotSymp = foundp;
                    m_ds.m_dotPos = DP_SCOPE;
                    UINFO(9, indent() << "modport -> iface varref " << foundp->nodep());
                    // We lose the modport name here, so we cannot detect mismatched modports.
                    AstVarRef* ਅਚਲ refp
                        = ਨਵਾਂ AstVarRef{nodep->fileline(), ifaceRefVarp, VAccess::READ};
                    Ast_ਗੰਢ_ਵਿਅੰਜਕ* newp = refp;

                    ਸਵੈਚਾਲਿਤ* ਅਚਲ cellarrayrefp = VN_CAST(m_ds.m_unlinkedScopep, CellArrayRef);
                    ਜੇ (cellarrayrefp) {
                        // iface[vec].modport became CellArrayRef(iface, lsb)
                        // Convert back to SelBit(iface, lsb)
                        UINFO(9, indent() << "Array modport to SelBit " << cellarrayrefp);
                        newp = ਨਵਾਂ Ast_Sel_ਦੋਆਧਾਰੀ_ਅੰਕ{cellarrayrefp->fileline(), newp,
                                             cellarrayrefp->selp()->unlink_Fr_ਪਿੱਠ()};
                        newp->user3(ਸੱਚ);  // Don't process again
                        VL_DO_DANGLING(ਧੱਕੋ_Deletep(cellarrayrefp->unlink_Fr_ਪਿੱਠ()), cellarrayrefp);
                        m_ds.m_unlinkedScopep = ਖਾਲੀ_ਸੂਚਕ;
                    }
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
                }
            } ਨਹੀਂਤਾਂ ਜੇ (AstConstraint* ਅਚਲ defp = VN_CAST(foundp->nodep(), Constraint)) {
                Ast_ਗੰਢ* ਅਚਲ newp = ਨਵਾਂ AstConstraintRef{nodep->fileline(), ਖਾਲੀ_ਸੂਚਕ, defp};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
                ok = ਸੱਚ;
                m_ds.m_dotPos = DP_MEMBER;
                m_ds.m_dot_ਪਾਠ = "";
            } ਨਹੀਂਤਾਂ ਜੇ (AstClass* ਅਚਲ defp = VN_CAST(foundp->nodep(), Class)) {
                ਜੇ (allowVar) {
                    ਜੇ (m_ds.m_dotPos == DP_SCOPE && !staticAccess) {
                        // Continue hierarchical lookup in the class scope.
                        m_ds.m_dotSymp = foundp;
                    } ਨਹੀਂਤਾਂ {
                        AstRefDType* ਅਚਲ newp
                            = ਨਵਾਂ AstRefDType{nodep->fileline(), nodep->name()};
                        replaceWithCheckBreak(nodep, newp);
                        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
                    }
                    ok = ਸੱਚ;
                    m_ds.m_dot_ਪਾਠ = "";
                } ਨਹੀਂਤਾਂ {
                    (ਖਾਲੀ)defp;  // Prevent unused variable warning
                }
            } ਨਹੀਂਤਾਂ ਜੇ (AstMemberDType* ਅਚਲ defp = VN_CAST(foundp->nodep(), MemberDType)) {
                ਜੇ (allowVar) {
                    check_Member_Decl_ਕ੍ਰਮ(nodep, defp);
                    AstRefDType* ਅਚਲ refp = ਨਵਾਂ AstRefDType{nodep->fileline(), nodep->name()};
                    refp->refDTypep(defp);
                    replaceWithCheckBreak(nodep, refp);
                    VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
                    ok = ਸੱਚ;
                    m_ds.m_dot_ਪਾਠ = "";
                }
            } ਨਹੀਂਤਾਂ ਜੇ (Ast_Enum_ਵਸਤੂ* ਅਚਲ valuep = VN_CAST(foundp->nodep(), Enum_ਵਸਤੂ)) {
                ਜੇ (allowVar) {
                    Ast_Enum_ਵਸਤੂ_Ref* ਅਚਲ refp
                        = ਨਵਾਂ Ast_Enum_ਵਸਤੂ_Ref{nodep->fileline(), valuep, foundp->classOrPackagep()};
                    Ast_ਗੰਢ* newp = refp;
                    // Hierarchical reference, relinked after V3Param as the referenced
                    // module may be specialized, similar to AstVarXRef
                    refp->dotted(m_ds.m_dot_ਪਾਠ);
                    refp->ਸ਼ਾਮਲ_ਹੈ_Gen_ਖੰਡ(m_ds.m_genBlk);
                    ਜੇ (m_ds.m_unresolvedCell && m_ds.m_unlinkedScopep) {
                        UINFO(9, indent() << "deferring until post-V3Param: " << refp);
                        newp = ਨਵਾਂ AstUnlinkedRef{nodep->fileline(), refp, refp->name(),
                                                  m_ds.m_unlinkedScopep->unlink_Fr_ਪਿੱਠ()};
                        m_ds.m_unlinkedScopep = ਖਾਲੀ_ਸੂਚਕ;
                        m_ds.m_unresolvedCell = ਝੂਠ;
                    }
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
                    ok = ਸੱਚ;
                    m_ds.m_dot_ਪਾਠ = "";
                }
            } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ AstLambdaArgRef* ਅਚਲ argrefp
                       = VN_CAST(foundp->nodep(), LambdaArgRef)) {
                ਜੇ (allowVar) {
                    Ast_ਗੰਢ* ਅਚਲ newp
                        = ਨਵਾਂ AstLambdaArgRef{nodep->fileline(), argrefp->name(), ਝੂਠ};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
                    ok = ਸੱਚ;
                    m_ds.m_dotPos = DP_MEMBER;
                    m_ds.m_dot_ਪਾਠ = "";
                }
            } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(foundp->nodep(), Clocking)) {
                m_ds.m_dotSymp = foundp;
                ਜੇ (m_ds.m_dot_ਪਾਠ != "") m_ds.m_dot_ਪਾਠ += "." + nodep->name();
                ok = m_ds.m_dotPos == DP_SCOPE || m_ds.m_dotPos == DP_ਪਹਿਲਾ;
            } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ AstModportClockingRef* ਅਚਲ clockingRefp
                       = VN_CAST(foundp->nodep(), ModportClockingRef)) {
                // Clocking block accessed through a modport - redirect to actual clocking
                m_ds.m_dotSymp = m_statep->get_ਗੰਢ_Sym(clockingRefp->clockingp());
                ਜੇ (m_ds.m_dot_ਪਾਠ != "") m_ds.m_dot_ਪਾਠ += "." + nodep->name();
                ok = m_ds.m_dotPos == DP_SCOPE || m_ds.m_dotPos == DP_ਪਹਿਲਾ;
            } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ Ast_ਗੰਢ_FTask* ਅਚਲ ftaskp = VN_CAST(foundp->nodep(), ਗੰਢ_FTask)) {
                ਜੇ (!ftaskp->isFunction() || ftaskp->class_ਵਿਧੀ()) {
                    ok = m_ds.m_dotPos == DP_NONE;
                    ਜੇ (ok) {
                        // The condition is true for tasks,
                        // properties and void functions.
                        // In these cases, the parentheses may be skipped.
                        // Also SV class methods can be called without parens
                        AstFuncRef* ਅਚਲ funcRefp
                            = ਨਵਾਂ AstFuncRef{nodep->fileline(), nodep->name()};
                        nodep->replaceWith(funcRefp);
                        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
                    }
                }
            } ਨਹੀਂਤਾਂ ਜੇ (AstTypedef* ਅਚਲ defp = VN_CAST(foundp->nodep(), Typedef)) {
                ਅਚਲ ਤਾਰਕਿਕ ifaceFinalSegmentAllowed
                    = (m_ds.m_dotPos == DP_FINAL) && m_ds.m_dotSymp
                      && (VN_IS(m_ds.m_dotSymp->nodep(), Modport)
                          || (VN_IS(m_ds.m_dotSymp->nodep(), Cell)
                              && VN_CAST(m_ds.m_dotSymp->nodep(), Cell)->modp()
                              && VN_IS(VN_CAST(m_ds.m_dotSymp->nodep(), Cell)->modp(), Iface)));
                // Allow interface typedef references even without IfaceCapture
                // The dependency graph handles resolution when IfaceCapture is disabled
                ok = (m_ds.m_dotPos == DP_NONE || m_ds.m_dotPos == DP_SCOPE
                      || ifaceFinalSegmentAllowed);
                ਜੇ (ifaceFinalSegmentAllowed) {
                    UINFO(9, indent() << "allow final-segment typedef name="
                                      << nodep->prettyNameQ() << " dotText='" << m_ds.m_dot_ਪਾਠ
                                      << "' dotSym=" << m_ds.m_dotSymp);
                }
                ਜੇ (ok) {
                    AstRefDType* ਅਚਲ refp = ਨਵਾਂ AstRefDType{nodep->fileline(), nodep->name()};
                    // Don't check if typedef is to a <type T>::<reference> as might not be
                    // resolved yet
                    ਜੇ (m_ds.m_dotPos == DP_NONE) check_Decl_ਕ੍ਰਮ(nodep, defp);
                    refp->typedefp(defp);

                    V3LinkDotIfaceCapture::captureTypedefContext(
                        refp, "typedef", static_cast<ਪੂਰਨ>(m_ds.m_dotPos), m_ds.m_dot_ਪਾਠ,
                        m_ds.m_dotSymp, m_modp, [ਇਹ]() { ਵਾਪਸ indent(); });

                    ਜੇ (VN_IS(nodep->backp(), Sel_ਕੱਢੋ)) {
                        m_packedArrayDtp = refp;
                    } ਨਹੀਂਤਾਂ {
                        replaceWithCheckBreak(nodep, refp);
                        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
                    }
                }
            } ਨਹੀਂਤਾਂ ਜੇ (AstParamTypeDType* ਅਚਲ defp = VN_CAST(foundp->nodep(), ParamTypeDType)) {
                ok = (m_ds.m_dotPos == DP_NONE || m_ds.m_dotPos == DP_SCOPE
                      || m_ds.m_dotPos == DP_FINAL);
                ਜੇ (ok) {
                    AstRefDType* ਅਚਲ refp = ਨਵਾਂ AstRefDType{nodep->fileline(), nodep->name()};
                    refp->refDTypep(defp);

                    V3LinkDotIfaceCapture::captureTypedefContext(
                        refp, "paramtype", static_cast<ਪੂਰਨ>(m_ds.m_dotPos), m_ds.m_dot_ਪਾਠ,
                        m_ds.m_dotSymp, m_modp, [ਇਹ]() { ਵਾਪਸ indent(); });

                    ਜੇ (VN_IS(nodep->backp(), Sel_ਕੱਢੋ)) {
                        m_packedArrayDtp = refp;
                    } ਨਹੀਂਤਾਂ {
                        replaceWithCheckBreak(nodep, refp);
                        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
                    }
                }
            }
            ਜੇ (!ok) {
                ਜੇ (m_insideClassExtParam) {
                    // Don't throw error if the reference is inside a class that extends a param,
                    // because some members can't be linked in such a case. m_insideClassExtParam
                    // may be true only in the first stage of linking.
                    // Mark that the Dot statement can't be resolved.
                    m_ds.m_unresolvedClass = ਸੱਚ;
                    // If the symbol was a scope name, it would be resolved.
                    m_ds.m_dotPos = DP_MEMBER;
                } ਨਹੀਂਤਾਂ {
                    // Cells/interfaces can't be implicit
                    ਅਚਲ ਤਾਰਕਿਕ checkImplicit
                        = (!m_ds.m_dotp && m_ds.m_dot_ਪਾਠ == "" && !m_ds.m_disablep && !foundp);
                    ਅਚਲ ਤਾਰਕਿਕ err
                        = !(checkImplicit && m_statep->implicitOk(m_modp, nodep->name()));
                    ਜੇ (err) {
                        ਜੇ (foundp) {
                            nodep->v3error("Found definition of '"
                                           << m_ds.m_dot_ਪਾਠ << (m_ds.m_dot_ਪਾਠ == "" ? "" : ".")
                                           << nodep->prettyName() << "'" << " as a "
                                           << foundp->nodep()->typeName() << " but expected a "
                                           << expectWhat);
                        } ਨਹੀਂਤਾਂ ਜੇ (m_ds.m_dot_ਪਾਠ == "") {
                            UINFO(1, "   ErrParseRef curSymp=se" << cvt_ਮੰਜ਼ਿਲ_Hex(m_curSymp)
                                                                 << " ds=" << m_ds.ascii());
                            ਅਚਲ string suggest = m_statep->suggestSymFallback(
                                m_ds.m_dotSymp, nodep->name(), VNodeMatcher{});
                            nodep->v3error(
                                "Can't find definition of "
                                << expectWhat << ": " << nodep->prettyNameQ() << '\n'
                                << (suggest.ਖਾਲੀ_ਥਾਵਾਂ() ? "" : nodep->warn_ਹੋਰ() + suggest));
                        } ਨਹੀਂਤਾਂ {
                            nodep->v3error(
                                "Can't find definition of "
                                << (!baddot.ਖਾਲੀ_ਥਾਵਾਂ() ? Ast_ਗੰਢ::prettyNameQ(baddot)
                                                    : nodep->prettyNameQ())
                                << " in dotted " << expectWhat << ": '"
                                << m_ds.m_dot_ਪਾਠ + "." + nodep->prettyName() << "'\n"
                                << nodep->warnContextPrimary()
                                << (okSymp ? okSymp->cellErrorScopes(
                                                 nodep, Ast_ਗੰਢ::prettyName(m_ds.m_dot_ਪਾਠ))
                                           : ""));
                        }
                        m_ds.m_dotErr = ਸੱਚ;
                    }
                    ਜੇ (checkImplicit) {
                        // Create if implicit, and also if error (so only complain once)
                        // Else if a scope is allowed, making a signal won't help error cascade
                        AstVar* ਅਚਲ varp
                            = createImplicitVar(m_curSymp, nodep, m_modp, m_modSymp, err);
                        AstVarRef* ਅਚਲ newp
                            = ਨਵਾਂ AstVarRef{nodep->fileline(), varp, VAccess::READ};
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
                    }
                }
            }
        }
        ਜੇ (ਸ਼ੁਰੂ) m_ds = ਆਖਰੀ_ਹਾਲਤਾਂ;
    }
    ਖਾਲੀ ਦੇਖੋ(AstClassOrPackageRef* nodep) override {
        // Class: Recurse inside or cleanup not founds
        // checkNoDot not appropriate, can be under a dot
        LINKDOT_ਦੇਖੋ_ਸ਼ੁਰੂ();
        UINFO(8, indent() << "visit " << nodep);
        UINFO(9, indent() << m_ds.ascii());
        VL_RESTORER_CLEAR(m_ਵਰਤਿਆ_Pins);
        VL_RESTORER_CLEAR(m_ਵਰਤਿਆ_Def_Param_Pins);
        UASSERT_OBJ(m_statep->forPrimary() || !nodep->paramsp(), nodep,
                    "class reference parameter not removed by V3Param");
        {
            VL_RESTORER_COPY(m_ds);
            VL_RESTORER(m_pinSymp);

            ਜੇ (!nodep->classOrPackageSkipp() && !nodep->classOrPackageNodep()
                && nodep->name() != "local::") {
                ਅਚਲ ਤਾਰਕਿਕ deferIfUnresolved = m_statep->forPrimary() && m_insideClassExtParam;
                m_statep->resolveClassOrPackage(m_ds.m_dotSymp, nodep, m_ds.m_dotPos != DP_PACKAGE,
                                                ਝੂਠ, ":: reference", deferIfUnresolved);
            }

            // ClassRef's have pins, so track
            ਜੇ (nodep->classOrPackageSkipp()) {
                m_pinSymp = m_statep->get_ਗੰਢ_Sym(nodep->classOrPackageSkipp());
            } ਨਹੀਂਤਾਂ ਜੇ (nodep->name() != "local::") {
                ਵਾਪਸ;
            }

            AstClass* ਅਚਲ refClassp = VN_CAST(nodep->classOrPackageSkipp(), Class);
            // Make sure any extends() are properly imported within referenced class
            ਜੇ (refClassp && !m_statep->forPrimary()) classExtendImport(refClassp);

            ਜੇ (m_statep->forPrimary() && refClassp && !nodep->paramsp()
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
                        << nodep->warn_ਹੋਰ() << "... Suggest use '"
                        << nodep->classOrPackageNodep()->prettyName() << "#()'");
                // As other simulators do, use the default specialization. Add the same
                // placeholder pin V3LinkCells makes for '#()' so it is linked below.
                AstPin* ਅਚਲ pinp = ਨਵਾਂ AstPin{nodep->fileline(), 1, "__paramNumber1", ਖਾਲੀ_ਸੂਚਕ};
                pinp->param(ਸੱਚ);
                nodep->ਜੋੜੋ_Paramsp(pinp);
            }

            m_ds.init(m_curSymp);
            UINFO(4, indent() << "(Backto) Link ClassOrPackageRef: " << nodep);
            iterateChildren(nodep);
        }

        ਜੇ (nodep->name() == "local::") {
            ਜੇ (!m_randSymp && !m_rand_ਵਿਧੀ_Callp) {
                nodep->v3error("Illegal 'local::' outside 'randomize() with'"
                               " (IEEE 1800-2023 18.7.1)");
                m_ds.m_dotErr = ਸੱਚ;
                ਵਾਪਸ;
            }
        }
        ਜੇ (m_ds.m_dotPos == DP_PACKAGE && nodep->classOrPackageSkipp()) {
            m_ds.m_dotSymp = m_statep->get_ਗੰਢ_Sym(nodep->classOrPackageSkipp());
            UINFO(9, indent() << "set sym " << m_ds.ascii());
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstConstraint* nodep) override {
        LINKDOT_ਦੇਖੋ_ਸ਼ੁਰੂ();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        ਜੇ (nodep->isExternDef()) {
            ਜੇ (ਅਚਲ VSymEnt* ਅਚਲ foundp
                = m_curSymp->ਲੱਭੋ_Id_Fallback("extern " + nodep->name())) {
                ਅਚਲ AstConstraint* ਅਚਲ protop = VN_AS(foundp->nodep(), Constraint);
                // Copy specifiers.
                // External definition cannot have any specifiers, so no value will be overwritten.
                nodep->isStatic(protop->isStatic());
            } ਨਹੀਂਤਾਂ {
                nodep->v3error("extern not found that declares " + nodep->prettyNameQ());
            }
        }
        ਜੇ (nodep->isExternProto()) {
            ਜੇ (!m_curSymp->ਲੱਭੋ_Id_Fallback(nodep->name()) && nodep->isExternExplicit()) {
                nodep->v3warn(PROTOTYPEMIS, "Definition not found for extern prototype "
                                                << nodep->prettyNameQ());
            }
        }
        VL_RESTORER(m_curSymp);
        VL_RESTORER_COPY(m_ds);
        m_ds.m_dotSymp = m_curSymp = m_statep->get_ਗੰਢ_Sym(nodep);
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstConstraintRef* nodep) override {
        ਜੇ (nodep->user3_ਸੈੱਟ_Once()) ਵਾਪਸ;
        LINKDOT_ਦੇਖੋ_ਸ਼ੁਰੂ();
        UINFO(8, indent() << "visit " << nodep);
        UINFO(8, indent() << m_ds.ascii());
        // No children defined
    }
    ਖਾਲੀ ਦੇਖੋ(AstVarRef* nodep) override {
        // VarRef: Resolve its reference
        // ParseRefs are used the first pass (forPrimary) so we shouldn't get can't find
        // errors here now that we have a VarRef.
        // No checkNoDot; created and iterated from a parseRef
        LINKDOT_ਦੇਖੋ_ਸ਼ੁਰੂ();
        iterateChildren(nodep);
        ਜੇ (!nodep->varp()) {
            UINFO(9, indent() << "linkVarRef se" << cvt_ਮੰਜ਼ਿਲ_Hex(m_curSymp) << "  n=" << nodep);
            UASSERT_OBJ(m_curSymp, nodep, "nullptr lookup symbol table");
            VSymEnt* ਅਚਲ foundp = m_curSymp->ਲੱਭੋ_Id_Fallback(nodep->name());
            ਜੇ (AstVar* ਅਚਲ varp
                = foundp ? ਮਿਲਿਆ_ਮੰਜ਼ਿਲ_Varp(foundp, nodep, nodep->access()) : ਖਾਲੀ_ਸੂਚਕ) {
                nodep->varp(varp);
                updateVarUse(nodep->varp());
                // Generally set by parse, but might be an import
                nodep->classOrPackagep(foundp->classOrPackagep());
            }
            ਜੇ (VL_UNCOVERABLE(!nodep->varp())) {
                nodep->v3error("Can't find definition of signal, again: "  // LCOV_EXCL_LINE
                               << nodep->prettyNameQ());
            }
        }
        AstVarScope* vscp = nodep->varScopep();
        ਜੇ (vscp && vscp->user2p() != vscp && m_replaceWithAlias) {
            vscp = LinkDotScopeVisitor::getAlias(vscp);
            nodep->varp(vscp->varp());
            nodep->varScopep(vscp);
            updateVarUse(nodep->varp());
            UINFO(7, indent() << "Resolved " << nodep);  // Also prints taskp
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstVarXRef* nodep) override {
        // VarRef: Resolve its reference
        // We always link even if varp() is set, because the module we choose may change
        // due to creating new modules, flattening, etc.
        ਜੇ (nodep->user3_ਸੈੱਟ_Once()) ਵਾਪਸ;
        LINKDOT_ਦੇਖੋ_ਸ਼ੁਰੂ();
        UINFO(8, indent() << "visit " << nodep);
        // No checkNoDot; created and iterated from a parseRef
        ਜੇ (!m_modSymp) {
            // Module that is not in hierarchy.  We'll be dead code eliminating it later.
            UINFO(9, "Dead module for " << nodep);
            nodep->varp(ਖਾਲੀ_ਸੂਚਕ);
        } ਨਹੀਂਤਾਂ {
            VSymEnt* lookupSymp = m_curSymp;  // Start search at current scope
            ਜੇ (nodep->inlinedDots() != "") {  // Correct for current post-inlined scope
                lookupSymp = ਲੱਭੋ_Inlined_Dots_Sym(nodep, nodep->inlinedDots());
            }
            string baddot;
            VSymEnt* okSymp;
            ਤਾਰਕਿਕ modport;
            VSymEnt* ਅਚਲ dotSymp
                = ਲੱਭੋ_Dotted_Ref_Symp(nodep, lookupSymp, nodep->dotted(), baddot, okSymp, &modport);
            ਜੇ (!dotSymp) ਵਾਪਸ;

            ਜੇ (!m_statep->forScopeCreation()) {
                VSymEnt* foundp = ਖਾਲੀ_ਸੂਚਕ;
                ਜੇ (modport) {
                    // This is a copy of findSymPrefixed with few modifications.
                    // This searches without a fallback like findSymPrefixed but if lookup fails it
                    // will check one fallback (fallback of modport shall be interface) and take
                    // only L and G PARAMs since they should be visible from a modport always.
                    string ਅਗੇਤਰ = dotSymp->sym_ਅਗੇਤਰ();
                    ਜਦੋਂ (!foundp) {
                        foundp = dotSymp->ਲੱਭੋ_Id_Flat(ਅਗੇਤਰ + nodep->name());
                        ਜੇ (foundp) ਰੋਕੋ;
                        UASSERT_OBJ(dotSymp->fallbackp(), nodep, "Modports shall have fallback");
                        foundp = dotSymp->fallbackp()->ਲੱਭੋ_Id_Flat(ਅਗੇਤਰ + nodep->name());
                        ਜੇ (ਅਚਲ AstVar* ਅਚਲ varp
                            = ਮਿਲਿਆ_ਮੰਜ਼ਿਲ_Varp(foundp, nodep, nodep->access())) {
                            ਜੇ (!varp->isParam()) foundp = ਖਾਲੀ_ਸੂਚਕ;
                        }
                        ਜੇ (ਅਗੇਤਰ.ਖਾਲੀ_ਥਾਵਾਂ()) ਰੋਕੋ;
                        string ਅਗਲਾ_ਅਗੇਤਰ = LinkDotState::remove_ਆਖਰੀ_Inline_Scope(ਅਗੇਤਰ);
                        ਜੇ (ਅਗੇਤਰ == ਅਗਲਾ_ਅਗੇਤਰ) ਰੋਕੋ;
                        ਅਗੇਤਰ = std::move(ਅਗਲਾ_ਅਗੇਤਰ);
                    }
                    baddot = nodep->name();
                } ਨਹੀਂਤਾਂ {
                    foundp = m_statep->ਲੱਭੋ_Sym_Prefixed(dotSymp, nodep->name(), baddot, ਸੱਚ);
                }
                ਜੇ (m_ਵਿੱਚ_Sens && foundp) {
                    ਜੇ (AstClocking* ਅਚਲ clockingp = sensClockingp(foundp->nodep())) {
                        foundp = getCreateClockingEventSymEnt(clockingp);
                    }
                }
                AstVar* ਅਚਲ varp
                    = foundp ? ਮਿਲਿਆ_ਮੰਜ਼ਿਲ_Varp(foundp, nodep, nodep->access()) : ਖਾਲੀ_ਸੂਚਕ;
                nodep->varp(varp);
                updateVarUse(nodep->varp());
                UINFO(7, indent() << "Resolved " << nodep);  // Also prints varp
                // If found, check if it's ok to access in case it's in a hier_block
                ਜੇ (nodep->varp() && error_Hier_Non_ਪੋਰਟ(nodep, nodep->varp(), dotSymp)) ਵਾਪਸ;
                ਜੇ (!nodep->varp()) {
                    nodep->v3error("Can't find definition of "
                                   << Ast_ਗੰਢ::prettyNameQ(baddot) << " in dotted signal: '"
                                   << nodep->dotted() + "." + nodep->prettyName() << "'\n"
                                   << nodep->warnContextPrimary()
                                   << okSymp->cellErrorScopes(nodep));
                    ਵਾਪਸ;
                }
            } ਨਹੀਂਤਾਂ {
                VSymEnt* ਅਚਲ foundp
                    = m_statep->ਲੱਭੋ_Sym_Prefixed(dotSymp, nodep->name(), baddot, ਸੱਚ);
                AstVarScope* vscp = foundp ? VN_CAST(foundp->nodep(), VarScope) : ਖਾਲੀ_ਸੂਚਕ;
                // Handle modport expression virtual ports
                ਜੇ (!vscp) vscp = ਲੱਭੋ_Virtual_ਪੋਰਟ_Var_Scope(dotSymp, nodep->name());
                // If found, check if it's ok to access in case it's in a hier_block
                ਜੇ (vscp && error_Hier_Non_ਪੋਰਟ(nodep, vscp->varp(), dotSymp)) ਵਾਪਸ;
                ਜੇ (!vscp) {
                    nodep->v3error("Can't find varpin scope of "
                                   << Ast_ਗੰਢ::prettyNameQ(baddot) << " in dotted signal: '"
                                   << nodep->dotted() + "." + nodep->prettyName() << "'\n"
                                   << nodep->warnContextPrimary()
                                   << okSymp->cellErrorScopes(nodep));
                } ਨਹੀਂਤਾਂ {
                    ਜੇ (vscp->user2p() && m_replaceWithAlias) {
                        vscp = LinkDotScopeVisitor::getAlias(vscp);
                    }
                    // Convert the VarXRef to a VarRef, so we don't need
                    // later optimizations to deal with VarXRef.
                    nodep->varp(vscp->varp());
                    nodep->varScopep(vscp);
                    updateVarUse(nodep->varp());
                    UINFO(7, indent() << "Resolved " << nodep);  // Also prints taskp
                    AstVarRef* ਅਚਲ newvscp
                        = ਨਵਾਂ AstVarRef{nodep->fileline(), vscp, nodep->access()};
                    nodep->replaceWith(newvscp);
                    VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
                    UINFO(9, indent() << "new " << newvscp);  // Also prints taskp
                }
            }
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstEnumDType* nodep) override {
        LINKDOT_ਦੇਖੋ_ਸ਼ੁਰੂ();
        iterateChildren(nodep);
        AstRefDType* ਅਚਲ refdtypep = VN_CAST(nodep->subDTypep(), RefDType);
        ਜੇ (refdtypep && (nodep == refdtypep->subDTypep())) {
            refdtypep->v3error("Self-referential enumerated type definition");
        }
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_UOr_Struct_DType* nodep) override {
        LINKDOT_ਦੇਖੋ_ਸ਼ੁਰੂ();
        symIterateChildren(nodep, m_statep->get_ਗੰਢ_Sym(nodep));
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Enum_ਵਸਤੂ_Ref* nodep) override {
        // Resolve its reference
        // EnumItemRefs are created by the first pass, but V3Param may regenerate due to
        // a parameterized class/module, so we shouldn't get can't find errors.
        // No checkNoDot; created and iterated from a parseRef
        ਜੇ (nodep->user3_ਸੈੱਟ_Once()) ਵਾਪਸ;
        LINKDOT_ਦੇਖੋ_ਸ਼ੁਰੂ();
        ਜੇ (m_statep->forParamed() && !nodep->dotted().ਖਾਲੀ_ਥਾਵਾਂ() && m_modSymp) {
            // Hierarchical reference, relink as V3Param may have specialized the module
            string baddot;
            VSymEnt* okSymp = ਖਾਲੀ_ਸੂਚਕ;
            VSymEnt* ਅਚਲ dotSymp
                = ਲੱਭੋ_Dotted_Ref_Symp(nodep, m_curSymp, nodep->dotted(), baddot, okSymp);
            ਜੇ (!dotSymp) ਵਾਪਸ;
            VSymEnt* ਅਚਲ foundp
                = m_statep->ਲੱਭੋ_Sym_Prefixed(dotSymp, nodep->name(), baddot, ਸੱਚ);
            Ast_Enum_ਵਸਤੂ* ਅਚਲ itemp = foundp ? VN_CAST(foundp->nodep(), Enum_ਵਸਤੂ) : ਖਾਲੀ_ਸੂਚਕ;
            ਜੇ (!itemp) {
                nodep->v3error("Enum item " << nodep->prettyNameQ() << " not found in "
                                            << Ast_ਗੰਢ::prettyNameQ(nodep->dotted()) << '\n'
                                            << nodep->warnContextPrimary());
                ਵਾਪਸ;
            }
            nodep->itemp(itemp);
            UINFO(9, indent() << " relinked " << nodep);
        }
        ਜੇ (!nodep->itemp()) {
            UINFO(9, indent() << "linkEnumRef se" << cvt_ਮੰਜ਼ਿਲ_Hex(m_curSymp) << "  n=" << nodep);
            UASSERT_OBJ(m_curSymp, nodep, "nullptr lookup symbol table");
            VSymEnt* ਅਚਲ foundp = m_curSymp->ਲੱਭੋ_Id_Fallback(nodep->name());
            ਜੇ (Ast_Enum_ਵਸਤੂ* ਅਚਲ itemp = foundp ? VN_CAST(foundp->nodep(), Enum_ਵਸਤੂ) : ਖਾਲੀ_ਸੂਚਕ) {
                nodep->itemp(itemp);
                nodep->classOrPackagep(foundp->classOrPackagep());
                UINFO(9, indent() << " resolved " << nodep);
            }
            ਜੇ (VL_UNCOVERABLE(!nodep->itemp())) {
                nodep->v3error("Can't find definition of enum item, again: "  // LCOV_EXCL_LINE
                               << nodep->prettyNameQ());
            }
        }
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਵਿਧੀ_Call* nodep) override {
        // Created here so should already be resolved.
        LINKDOT_ਦੇਖੋ_ਸ਼ੁਰੂ();
        UINFO(5, indent() << "visit " << nodep);
        VL_RESTORER_COPY(m_ds);
        VL_RESTORER(m_randSymp);
        VL_RESTORER(m_rand_ਵਿਧੀ_Callp);
        {
            m_ds.init(m_curSymp);
            ਜੇ (nodep->name() == "randomize" && (nodep->argsp() || nodep->withp())) {
                m_rand_ਵਿਧੀ_Callp = nodep;
                ਅਚਲ Ast_ਗੰਢ_DType* fromDtp = nodep->fromp()->dtypep();
                ਜੇ (!fromDtp) {
                    ਜੇ (ਅਚਲ Ast_ਗੰਢ_Var_Ref* ਅਚਲ varRefp = VN_CAST(nodep->fromp(), ਗੰਢ_Var_Ref)) {
                        fromDtp = varRefp->varp()->subDTypep();
                    } ਨਹੀਂਤਾਂ {
                        fromDtp = get_ਵਿਅੰਜਕ_DTypep(nodep->fromp());
                    }
                    ਜੇ (!fromDtp) {
                        ਜੇ (nodep->withp()) {
                            nodep->v3warn(
                                E_UNSUPPORTED,
                                "Unsupported: 'randomize() with' on complex expressions");
                        } ਨਹੀਂਤਾਂ {
                            nodep->v3warn(E_UNSUPPORTED,
                                          "Unsupported: Inline random variable control with "
                                          "'randomize()' called on complex expressions");
                        }
                    }
                }
                ਜੇ (m_statep->forPrimary() && isParamedClassRefDType(fromDtp)) {
                    m_ds.m_unresolvedClass = ਸੱਚ;
                } ਨਹੀਂਤਾਂ ਜੇ (fromDtp) {
                    ਅਚਲ AstClassRefDType* ਅਚਲ classDtp
                        = VN_CAST(fromDtp->skipRefp(), ClassRefDType);
                    ਜੇ (!classDtp)
                        nodep->v3error("'randomize() with' on a non-class-instance "
                                       << fromDtp->prettyNameQ());
                    ਨਹੀਂਤਾਂ ਜੇ (m_statep->forPrimary() && m_extendsParam.ਗਿਣਤੀ_2(classDtp->classp())) {
                        // Inherited members are unavailable until the base class is specialized.
                        // Defer lookup rather than incorrectly binding names in the caller.
                        m_ds.m_unresolvedClass = ਸੱਚ;
                    } ਨਹੀਂਤਾਂ {
                        m_randSymp = m_statep->get_ਗੰਢ_Sym(classDtp->classp());
                    }
                }
            }
            iterateChildren(nodep);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstVar* nodep) override {
        LINKDOT_ਦੇਖੋ_ਸ਼ੁਰੂ();
        checkNoDot(nodep);
        VL_RESTORER(m_isParam);
        m_isParam = nodep->varType().isParam();
        iterateChildren(nodep);
        ਜੇ (m_statep->forPrimary() && nodep->isIO() && !m_ftaskp && !nodep->user4()) {
            nodep->v3error(
                "Input/output/inout does not appear in port list: " << nodep->prettyNameQ());
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstVarScope* nodep) override {
        LINKDOT_ਦੇਖੋ_ਸ਼ੁਰੂ();
        checkNoDot(nodep);
        iterateChildren(nodep);
        AstVarScope* aliasp = LinkDotScopeVisitor::getAlias(nodep);
        ਜੇ (aliasp && aliasp != nodep && !nodep->varp()->isIfaceRef()) {
            // Aliased variable might still be references from outside,
            // eg through the VPI, and is traced, so we need the value to propagate.
            // TODO: this means external writes to the LHS (e.g.: through the VPI) don't work
            Ast_ਸੌਂਪੋ_W* ਅਚਲ assignp = ਨਵਾਂ Ast_ਸੌਂਪੋ_W{
                nodep->fileline(), ਨਵਾਂ AstVarRef{nodep->fileline(), nodep, VAccess::WRITE},
                ਨਵਾਂ AstVarRef{nodep->fileline(), aliasp, VAccess::READ}};
            assignp->user2(ਸੱਚ);
            nodep->scopep()->ਜੋੜੋ_Blocksp(ਨਵਾਂ AstAlways{assignp});
            // Propagate attributes and lint state of the replaced variable,
            // because all references to it are replaced with references to the alias variable
            aliasp->varp()->propagateAttrFrom(nodep->varp());
            aliasp->varp()->fileline()->modifyStateInherit(nodep->varp()->fileline());
        }
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_FTask_Ref* nodep) override {
        ਜੇ (nodep->user3_ਸੈੱਟ_Once()) ਵਾਪਸ;
        LINKDOT_ਦੇਖੋ_ਸ਼ੁਰੂ();
        UINFO(8, indent() << "visit " << nodep);
        UINFO(8, indent() << m_ds.ascii());
        ਜੇ (m_ds.m_dotPos != DP_MEMBER || nodep->name() != "randomize") {
            // Visit arguments at the beginning.
            // They may be visitted even if the current node can't be linked now.
            symIterateChildren(nodep, m_curSymp);
        }

        ਜੇ (m_ds.m_super) {
            ਜੇ (AstFuncRef* ਅਚਲ funcRefp = VN_CAST(nodep, FuncRef)) {
                funcRefp->superReference(ਸੱਚ);
            } ਨਹੀਂਤਾਂ ਜੇ (AstTaskRef* ਅਚਲ taskRefp = VN_CAST(nodep, TaskRef)) {
                taskRefp->superReference(ਸੱਚ);
            }
        }

        VL_RESTORER(m_randSymp);

        ਤਾਰਕਿਕ ਪਹਿਲਾ = !m_ds.m_dotp || m_ds.m_dotPos == DP_ਪਹਿਲਾ;
        ਤਾਰਕਿਕ staticAccess = ਝੂਠ;
        ਜੇ (m_ds.m_unresolvedClass) {
            // Unable to link before V3Param
            ਵਾਪਸ;
        } ਨਹੀਂਤਾਂ ਜੇ (m_ds.m_unresolvedCell && m_ds.m_dotPos == DP_FINAL && m_ds.m_unlinkedScopep) {
            Ast_ਗੰਢ_FTask_Ref* ਅਚਲ newftaskp = nodep->clone_ਰੁੱਖ(ਝੂਠ);
            newftaskp->dotted(m_ds.m_dot_ਪਾਠ);
            Ast_ਗੰਢ* ਅਚਲ newp = ਨਵਾਂ AstUnlinkedRef{nodep->fileline(), newftaskp, nodep->name(),
                                                     m_ds.m_unlinkedScopep->unlink_Fr_ਪਿੱਠ()};
            m_ds.m_unlinkedScopep = ਖਾਲੀ_ਸੂਚਕ;
            m_ds.m_unresolvedCell = ਝੂਠ;
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
            ਵਾਪਸ;
        } ਨਹੀਂਤਾਂ ਜੇ (m_ds.m_dotp && m_ds.m_dotPos == DP_PACKAGE) {
            UASSERT_OBJ(VN_IS(m_ds.m_dotp->lhsp(), ClassOrPackageRef), m_ds.m_dotp->lhsp(),
                        "Bad package link");
            staticAccess = ਸੱਚ;
            AstClassOrPackageRef* ਅਚਲ cpackagerefp
                = VN_AS(m_ds.m_dotp->lhsp(), ClassOrPackageRef);
            ਜੇ (cpackagerefp->name() == "local::") {
                m_randSymp = ਖਾਲੀ_ਸੂਚਕ;
                ਪਹਿਲਾ = ਸੱਚ;
            } ਨਹੀਂਤਾਂ ਜੇ (!cpackagerefp->classOrPackageSkipp()) {
                VSymEnt* ਅਚਲ foundp = m_statep->resolveClassOrPackage(
                    m_ds.m_dotSymp, cpackagerefp, ਸੱਚ, ਝੂਠ, ":: reference");
                ਜੇ (foundp) nodep->classOrPackagep(cpackagerefp->classOrPackageSkipp());
            } ਨਹੀਂਤਾਂ {
                nodep->classOrPackagep(cpackagerefp->classOrPackageSkipp());
            }
            // Class/package :: HERE function() . method_called_on_function_return_value()
            m_ds.m_dotPos = DP_MEMBER;
        } ਨਹੀਂਤਾਂ ਜੇ (m_ds.m_dotp && m_ds.m_dotPos == DP_FINAL) {
            nodep->dotted(m_ds.m_dot_ਪਾਠ);  // Maybe ""
            // Only flag FTaskRefs under generate-if/case blocks that may be
            // pruned.  GenFor and plain begin-blocks won't be pruned by V3Param.
            // A generate-case block's parent is the GenCaseItem, not the GenCase.
            // VarXRef uses the broader m_genBlk flag (set for all GenBlocks)
            // because genfor unrolling also removes variables.
            ਜੇ (m_ds.m_genBlk && m_ds.m_dotSymp) {
                ਅਚਲ Ast_ਗੰਢ* ਅਚਲ blkp = m_ds.m_dotSymp->nodep();
                ਜੇ (VN_IS(blkp, Gen_ਖੰਡ)
                    && (VN_IS(blkp->backp(), GenIf) || VN_IS(blkp->backp(), Gen_Case_ਵਸਤੂ))) {
                    nodep->ਸ਼ਾਮਲ_ਹੈ_Gen_ਖੰਡ(ਸੱਚ);
                }
            }
        } ਨਹੀਂਤਾਂ ਜੇ (m_ds.m_dotp && m_ds.m_dotPos == DP_MEMBER) {
            // Found a Var, everything following is method call.
            // {scope}.{var}.HERE {method} ( ARGS )
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ varEtcp = VN_AS(m_ds.m_dotp->lhsp()->unlink_Fr_ਪਿੱਠ(), ਗੰਢ_ਵਿਅੰਜਕ);
            AstArg* ਅਚਲ argsp = nodep->argsp();
            ਜੇ (argsp) argsp->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ();
            Ast_ਵਿਧੀ_Call* ਅਚਲ newp = ਨਵਾਂ Ast_ਵਿਧੀ_Call{nodep->fileline(), varEtcp,
                                                          VFlag_ਬੱਚਾ_DType{}, nodep->name(), argsp};
            ਜੇ (AstWith* ਅਚਲ withp = nodep->withp()) newp->withp(withp->unlink_Fr_ਪਿੱਠ());
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
            ਵਾਪਸ;
        } ਨਹੀਂਤਾਂ ਜੇ (m_ds.m_dotp && (m_ds.m_dotPos == DP_SCOPE || m_ds.m_dotPos == DP_ਪਹਿਲਾ)) {
            // HERE function() . method_called_on_function_return_value()
            m_ds.m_dotPos = DP_MEMBER;
            m_ds.m_dot_ਪਾਠ = "";
        } ਨਹੀਂਤਾਂ ਜੇ (!m_ds.m_disablep) {
            // visit(AstDisable*) setup the dot handling
            checkNoDot(nodep);
        }
        ਜੇ (nodep->classOrPackagep() && nodep->taskp()) {
            // References into packages don't care about cell hierarchy.
        } ਨਹੀਂਤਾਂ ਜੇ (!m_modSymp) {
            // Module that is not in hierarchy.  We'll be dead code eliminating it later.
            UINFO(9, indent() << "Dead module for " << nodep);
            nodep->taskp(ਖਾਲੀ_ਸੂਚਕ);
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->dotted() == "" && nodep->taskp()) {
            // Earlier should have setup the links
            // Might be under a BEGIN we're not processing, so don't relink it
        } ਨਹੀਂਤਾਂ {
            string baddot;
            VSymEnt* okSymp = ਖਾਲੀ_ਸੂਚਕ;
            VSymEnt* dotSymp;
            ਜੇ (nodep->dotted().ਖਾਲੀ_ਥਾਵਾਂ()) {
                // Non-'super.' dotted reference
                dotSymp = m_ds.m_dotSymp;
            } ਨਹੀਂਤਾਂ {
                // Start search at module, as a variable of same name under a subtask isn't a
                // relevant hit however a function under a begin/end is.  So we want begins, but
                // not the function
                dotSymp = m_curSymp;
            }
            ਜੇ (nodep->classOrPackagep()) {  // Look only in specified package
                dotSymp = m_statep->get_ਗੰਢ_Sym(nodep->classOrPackagep());
                UINFO(8, indent() << "Override classOrPackage " << dotSymp);
            } ਨਹੀਂਤਾਂ {
                ਜੇ (nodep->inlinedDots() != "") {  // Correct for current post-inlined scope
                    dotSymp = ਲੱਭੋ_Inlined_Dots_Sym(nodep, nodep->inlinedDots());
                }
                dotSymp = m_statep->ਲੱਭੋ_Dotted(nodep->fileline(), dotSymp, nodep->dotted(), baddot,
                                               okSymp, ਸੱਚ);  // Maybe nullptr
            }
            // Restricted 'with (id_list)': only id_list names bind into target class.
            ਜੇ (m_randSymp
                && (!m_currentWithp || m_currentWithp->name_Resolves_ਮੰਜ਼ਿਲ_ਨਿਸ਼ਾਨਾ(nodep->name()))) {
                VSymEnt* ਅਚਲ foundp = m_randSymp->ਲੱਭੋ_Id_Flat(nodep->name());
                ਜੇ (foundp && m_currentWithp) {
                    ਜੇ (m_currentWithp->restricted()) {
                        m_restricted_ਨਾਂ_ਵਰਤਿਆ.ਪਾਓ(nodep->name());
                    }
                    UINFO(9, indent() << "randomize-with fromSym " << foundp->nodep());
                    AstArg* argsp = ਖਾਲੀ_ਸੂਚਕ;
                    ਜੇ (nodep->argsp()) {
                        iterate_And_ਅਗਲਾ_Null(nodep->argsp());
                        argsp = nodep->argsp()->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ();
                    }
                    ਜੇ (m_ds.m_dotPos != DP_NONE) m_ds.m_dotPos = DP_MEMBER;
                    Ast_ਗੰਢ* ਅਚਲ newp = ਨਵਾਂ Ast_ਵਿਧੀ_Call{
                        nodep->fileline(),
                        ਨਵਾਂ AstLambdaArgRef{nodep->fileline(), "__Vrandwith_obj", ਝੂਠ},
                        VFlag_ਬੱਚਾ_DType{}, nodep->name(), argsp};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
                    ਵਾਪਸ;
                }
            }
            ਜੇ (ਪਹਿਲਾ && nodep->name() == "randomize" && VN_IS(m_modp, Class)
                && !VN_IS(nodep->classOrPackagep(), Package)) {
                // need special handling to avoid falling back to std::randomize
                // Skip if classOrPackagep is a Package (i.e. std::randomize resolved earlier)
                VMemberMap memberMap;
                AstFunc* ਅਚਲ randFuncp = V3Randomize::newRandomizeFunc(
                    memberMap, VN_AS(m_modp, Class), nodep->name(), ਸੱਚ, ਸੱਚ);
                nodep->taskp(randFuncp);
                nodep->classOrPackagep(VN_AS(m_modp, Class));
                m_curSymp = m_statep->ਪਾਓ_ਖੰਡ(m_curSymp, nodep->name(), randFuncp, m_modp);
                // Already linked to the class randomize function; skip findSymPrefixed
                // which may find std::randomize and overwrite classOrPackagep
                ਵਾਪਸ;
            }
            VSymEnt* ਅਚਲ foundp
                = m_statep->ਲੱਭੋ_Sym_Prefixed(dotSymp, nodep->name(), baddot, ਪਹਿਲਾ);
            Ast_ਗੰਢ_FTask* ਅਚਲ taskp = foundp ? VN_CAST(foundp->nodep(), ਗੰਢ_FTask) : ਖਾਲੀ_ਸੂਚਕ;
            Ast_ਗੰਢ_Module* ਅਚਲ taskModulep = foundp ? foundp->classOrPackagep() : ਖਾਲੀ_ਸੂਚਕ;
            // Ignore deferring linking of functions inside params of classes extending
            // parametric classes if referenced function can be linked in the first pass.
            ਜੇ (!m_isParam || !taskp
                || (taskp->class_ਵਿਧੀ() && taskModulep && taskModulep->hasParameterList())) {
                ਜੇ (m_insideClassExtParam) {
                    // The reference may point to a method declared in a super class, which is
                    // proved by a parameter. In such a case, it can't be linked at the first
                    // stage. Must not do any linking, because e.g. might find an extends of an
                    // upper class because the current class (under parent) isn't yet importing
                    // it's extended class symbols.
                    ਵਾਪਸ;
                }
                ਜੇ (AstClass* ਅਚਲ ਨਿਸ਼ਾਨਾ_Classp = VN_CAST(dotSymp->nodep(), Class)) {
                    ਜੇ (m_extendsParam.ਗਿਣਤੀ_2(ਨਿਸ਼ਾਨਾ_Classp)) {
                        // Target class has parameterized extends not yet resolved.
                        // Its inherited symbols (e.g. static functions from the base class)
                        // aren't imported yet - defer to linkDotParamed.
                        ਵਾਪਸ;
                    }
                }
            }

            ਜੇ (taskp) {
                ਜੇ (staticAccess && !taskp->isStatic()) {
                    // TODO bug4077
                    // nodep->v3error("Static access to non-static task/function "
                    //                << taskp->prettyNameQ() << endl);
                }
                nodep->taskp(taskp);
                nodep->classOrPackagep(foundp->classOrPackagep());
                UINFO(7, indent() << "Resolved " << nodep);  // Also prints taskp
            } ਨਹੀਂਤਾਂ {
                // Note ParseRef has similar error handling/message output
                UINFO(7, indent() << "   ErrFtask curSymp=se" << cvt_ਮੰਜ਼ਿਲ_Hex(m_curSymp)
                                  << " dotSymp=se" << cvt_ਮੰਜ਼ਿਲ_Hex(dotSymp));
                ਜੇ (foundp) {
                    ਜੇ (VN_IS(foundp->nodep(), Var) && m_ds.m_dot_ਪਾਠ == "" && m_ftaskp
                        && m_ftaskp->name() == foundp->nodep()->name()) {
                        // This is a recursive reference to the function itself, not to the var
                        nodep->taskp(m_ftaskp);
                        nodep->classOrPackagep(foundp->classOrPackagep());
                        UINFO(7, indent() << "Resolved recursive " << nodep);  // Also prints taskp
                    } ਨਹੀਂਤਾਂ {
                        nodep->v3error("Found definition of '"
                                       << m_ds.m_dot_ਪਾਠ << (m_ds.m_dot_ਪਾਠ == "" ? "" : ".")
                                       << nodep->prettyName() << "'" << " as a "
                                       << foundp->nodep()->typeName()
                                       << " but expected a task/function");
                    }
                } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(nodep, New) && !m_statep->forScopeCreation()) {
                    // Resolved in V3Width
                } ਨਹੀਂਤਾਂ ਜੇ ((nodep->name() == "pre_randomize" || nodep->name() == "post_randomize")
                           && VN_IS(dotSymp->nodep(), Class)) {
                    AstClass* ਅਚਲ classp = VN_AS(dotSymp->nodep(), Class);
                    // Functions that might be defined by user, but if not, they get empty
                    // definitions within referenced class.
                    // TODO perhaps cleaner to add to symbol table up-front with
                    // a lambda to construct them on a reference.
                    AstFunc* ਅਚਲ funcp
                        = ਨਵਾਂ AstFunc{nodep->fileline(), nodep->name(), ਖਾਲੀ_ਸੂਚਕ, ਖਾਲੀ_ਸੂਚਕ};
                    funcp->class_ਵਿਧੀ(ਸੱਚ);
                    funcp->dtypep(funcp->ਲੱਭੋ_Void_DType());
                    classp->ਜੋੜੋ_Membersp(funcp);
                    m_statep->ਪਾਓ_ਖੰਡ(dotSymp, nodep->name(), funcp, classp);
                    nodep->taskp(funcp);
                    nodep->classOrPackagep(classp);
                    UINFO(9, indent() << " Auto-made " << funcp);
                    ਵਾਪਸ;
                } ਨਹੀਂਤਾਂ ਜੇ (nodep->name() == "randomize" || nodep->name() == "srandom"
                           || nodep->name() == "get_randstate" || nodep->name() == "set_randstate"
                           || nodep->name() == "rand_mode" || nodep->name() == "constraint_mode") {
                    ਜੇ (AstClass* ਅਚਲ classp = VN_CAST(m_modp, Class)) {
                        // Don't overwrite if already resolved to a package (e.g. std::randomize)
                        ਜੇ (!VN_IS(nodep->classOrPackagep(), Package))
                            nodep->classOrPackagep(classp);
                    } ਨਹੀਂਤਾਂ ਜੇ (nodep->name() == "randomize") {
                        // A std::randomize resolved in V3Width
                        nodep->classOrPackagep(v3Global.rootp()->stdPackagep());
                    } ਨਹੀਂਤਾਂ {
                        nodep->v3error("Calling implicit class method "
                                       << nodep->prettyNameQ() << " without being under class");
                        nodep->replaceWith(ਨਵਾਂ AstConst{nodep->fileline(), 0});
                        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
                        ਵਾਪਸ;
                    }
                } ਨਹੀਂਤਾਂ ਜੇ (nodep->dotted() == "") {
                    ਜੇ (nodep->pli()) {
                        ਜੇ (v3Global.opt.bboxSys()) {
                            Ast_ਗੰਢ* newp;
                            ਜੇ (VN_IS(nodep, FuncRef)) {
                                newp = ਨਵਾਂ AstConst{nodep->fileline(), AstConst::All0{}};
                            } ਨਹੀਂਤਾਂ {
                                Ast_ਗੰਢ_ਵਿਅੰਜਕ* outp = ਖਾਲੀ_ਸੂਚਕ;
                                ਜਦੋਂ (AstArg* ਅਚਲ argp = nodep->argsp()) {
                                    outp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(outp, argp->exprp()->unlink_Fr_ਪਿੱਠ());
                                    VL_DO_DANGLING(ਧੱਕੋ_Deletep(argp->unlink_Fr_ਪਿੱਠ()), argp);
                                }
                                newp = ਨਵਾਂ AstSysIgnore{nodep->fileline(), outp};
                                newp->dtypep(nodep->dtypep());
                            }
                            nodep->replaceWith(newp);
                            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
                            ਵਾਪਸ;
                        } ਨਹੀਂਤਾਂ {
                            VSpellCheck speller;
                            V3Parse::candidatePli(&speller);
                            ਅਚਲ string suggest = speller.ਉੱਤਮ_Candidate_ਸੁਨੇਹਾ(nodep->prettyName());
                            nodep->v3error(
                                "Unsupported or unknown PLI call: "
                                << nodep->prettyNameQ() << '\n'
                                << (suggest.ਖਾਲੀ_ਥਾਵਾਂ() ? "" : nodep->warn_ਹੋਰ() + suggest));
                        }
                    } ਨਹੀਂਤਾਂ {
                        ਅਚਲ string suggest = m_statep->suggestSymFallback(
                            dotSymp, nodep->name(), Link_ਗੰਢ_Matcher_FTask{});
                        nodep->v3error("Can't find definition of task/function: "
                                       << nodep->prettyNameQ() << '\n'
                                       << (suggest.ਖਾਲੀ_ਥਾਵਾਂ() ? "" : nodep->warn_ਹੋਰ() + suggest));
                    }
                } ਨਹੀਂਤਾਂ {
                    ਅਚਲ string suggest = m_statep->suggestSymFallback(dotSymp, nodep->name(),
                                                                        Link_ਗੰਢ_Matcher_FTask{});
                    nodep->v3error("Can't find definition of "
                                   << Ast_ਗੰਢ::prettyNameQ(baddot) << " in dotted task/function: '"
                                   << nodep->dotted() + "." + nodep->prettyName() << "'\n"
                                   << (suggest.ਖਾਲੀ_ਥਾਵਾਂ() ? "" : nodep->warn_ਹੋਰ() + suggest) << '\n'
                                   << nodep->warnContextPrimary()
                                   << okSymp->cellErrorScopes(nodep));
                }
            }
        }
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Sel_ਦੋਆਧਾਰੀ_ਅੰਕ* nodep) override {
        ਜੇ (nodep->user3_ਸੈੱਟ_Once()) ਵਾਪਸ;
        LINKDOT_ਦੇਖੋ_ਸ਼ੁਰੂ();
        iterate_And_ਅਗਲਾ_Null(nodep->fromp());
        ਜੇ (m_ds.m_unresolvedClass) {
            UASSERT_OBJ(m_ds.m_dotPos != DP_SCOPE && m_ds.m_dotPos != DP_ਪਹਿਲਾ, nodep,
                        "Object of unresolved class on scope position in dotted reference");
            // The index does not depend on the class, and V3Param must see its class references
            symIterateNull(nodep->bitp(), m_curSymp);
            symIterateNull(nodep->attrp(), m_curSymp);
            ਵਾਪਸ;
        }
        ਜੇ (m_ds.m_dotPos == DP_SCOPE
            || m_ds.m_dotPos
                   == DP_ਪਹਿਲਾ) {  // Already under dot, so this is {modulepart} DOT {modulepart}
            UINFO(9, indent() << "deferring until after a V3Param pass: " << nodep);
            m_ds.m_dot_ਪਾਠ += "__BRA__??__KET__";
            m_ds.m_unresolvedCell = ਸੱਚ;
            // And pass up m_ds.m_dotText
        }
        // Pass dot state down to only fromp()
        iterate_And_ਅਗਲਾ_Null(nodep->fromp());
        symIterateNull(nodep->bitp(), m_curSymp);
        symIterateNull(nodep->attrp(), m_curSymp);
        ਜੇ (m_ds.m_unresolvedCell && (m_ds.m_dotPos == DP_SCOPE || m_ds.m_dotPos == DP_ਪਹਿਲਾ)) {
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ exprp = nodep->bitp()->unlink_Fr_ਪਿੱਠ();
            ਜੇ (AstCellArrayRef* ਅਚਲ fromArrRefp = VN_CAST(nodep->fromp(), CellArrayRef)) {
                // Multi-dim iface array access: append this select to the existing chain
                fromArrRefp->ਜੋੜੋ_Selp(exprp);
                AstCellArrayRef* ਅਚਲ movedp = fromArrRefp->unlink_Fr_ਪਿੱਠ();
                nodep->replaceWith(movedp);
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
            } ਨਹੀਂਤਾਂ {
                AstCellArrayRef* ਅਚਲ newp
                    = ਨਵਾਂ AstCellArrayRef{nodep->fileline(), nodep->fromp()->name(), exprp};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
            }
        }
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Pre_Sel* nodep) override {
        // Excludes simple AstSelBit, see above
        ਜੇ (nodep->user3_ਸੈੱਟ_Once()) ਵਾਪਸ;
        LINKDOT_ਦੇਖੋ_ਸ਼ੁਰੂ();
        ਜੇ (m_ds.m_dotPos == DP_SCOPE
            || m_ds.m_dotPos
                   == DP_ਪਹਿਲਾ) {  // Already under dot, so this is {modulepart} DOT {modulepart}
            nodep->v3error("Syntax error: Range ':', '+:' etc are not allowed in the instance "
                           "part of a dotted reference");
            m_ds.m_dotErr = ਸੱਚ;
            ਵਾਪਸ;
        }
        Ast_ਗੰਢ_DType* packedArrayDtp = ਖਾਲੀ_ਸੂਚਕ;  // Datatype reference for packed array
        {
            VL_RESTORER(m_packedArrayDtp);
            iterate_And_ਅਗਲਾ_Null(nodep->fromp());
            symIterateNull(nodep->rhsp(), m_curSymp);
            symIterateNull(nodep->thsp(), m_curSymp);

            ਜੇ (m_packedArrayDtp) {
                AstRange* ਅਚਲ newRangep
                    = ਨਵਾਂ AstRange(nodep->fileline(), nodep->rhsp()->unlink_Fr_ਪਿੱਠ(),
                                   nodep->thsp()->unlink_Fr_ਪਿੱਠ());
                AstPackArrayDType* newArrayTypep
                    = ਨਵਾਂ AstPackArrayDType(nodep->fileline(), m_packedArrayDtp, newRangep);
                newArrayTypep->ਬੱਚਾ_DTypep(m_packedArrayDtp);
                newArrayTypep->refDTypep(ਖਾਲੀ_ਸੂਚਕ);
                ਜੇ (VN_IS(nodep->backp(), Sel_ਕੱਢੋ)) {
                    packedArrayDtp = newArrayTypep;
                } ਨਹੀਂਤਾਂ {
                    replaceWithCheckBreak(nodep, newArrayTypep);
                    VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
                }
            } ਨਹੀਂਤਾਂ {
                ਜੇ (nodep->attrp()) {
                    Ast_ਗੰਢ* ਅਚਲ attrp = nodep->attrp()->unlink_Fr_ਪਿੱਠ();
                    VL_DO_DANGLING(attrp->delete_ਰੁੱਖ(), attrp);
                }
                Ast_ਗੰਢ* ਅਚਲ basefromp = nodep->ਅਧਾਰ_Fromp(ਝੂਠ);
                ਜੇ (VN_IS(basefromp, Replicate)) {
                    // From {...}[...] syntax in IEEE 2017
                    ਜੇ (basefromp) UINFO(9, indent() << " Related node: " << basefromp);
                } ਨਹੀਂਤਾਂ {
                    nodep->attrp(ਨਵਾਂ Ast_Attr_ਦਾ{nodep->fileline(), VAttrType::VAR_ਅਧਾਰ,
                                               basefromp->clone_ਰੁੱਖ(ਝੂਠ)});
                }
            }
        }

        ਜੇ (packedArrayDtp) m_packedArrayDtp = packedArrayDtp;
    }
    ਖਾਲੀ ਦੇਖੋ(AstMemberSel* nodep) override {
        // checkNoDot not appropriate, can be under a dot
        LINKDOT_ਦੇਖੋ_ਸ਼ੁਰੂ();
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Gen_ਖੰਡ* nodep) override {
        LINKDOT_ਦੇਖੋ_ਸ਼ੁਰੂ();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        {
            VL_RESTORER(m_curSymp);
            VL_RESTORER_COPY(m_ds);
            ਜੇ (nodep->name() != "") {
                m_ds.m_dotSymp = m_curSymp = m_statep->get_ਗੰਢ_Sym(nodep);
                UINFO(5, indent() << "cur=se" << cvt_ਮੰਜ਼ਿਲ_Hex(m_curSymp));
            }
            iterateChildren(nodep);
        }
        UINFO(5, indent() << "cur=se" << cvt_ਮੰਜ਼ਿਲ_Hex(m_curSymp));
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_ਖੰਡ* nodep) override {
        LINKDOT_ਦੇਖੋ_ਸ਼ੁਰੂ();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        {
            VL_RESTORER(m_curSymp);
            VL_RESTORER_COPY(m_ds);
            ਜੇ (nodep->name() != "") {
                m_ds.m_dotSymp = m_curSymp = m_statep->get_ਗੰਢ_Sym(nodep);
                UINFO(5, indent() << "cur=se" << cvt_ਮੰਜ਼ਿਲ_Hex(m_curSymp));
            }
            iterateChildren(nodep);
        }
        UINFO(5, indent() << "cur=se" << cvt_ਮੰਜ਼ਿਲ_Hex(m_curSymp));
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_FTask* nodep) override {
        LINKDOT_ਦੇਖੋ_ਸ਼ੁਰੂ();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        ਜੇ (nodep->isExternDef() && !nodep->didProto()) {
            nodep->didProto(ਸੱਚ);
            ਜੇ (ਅਚਲ VSymEnt* ਅਚਲ foundp
                = m_curSymp->ਲੱਭੋ_Id_Fallback("extern " + nodep->name())) {
                ਅਚਲ Ast_ਗੰਢ_FTask* ਅਚਲ protop = VN_AS(foundp->nodep(), ਗੰਢ_FTask);
                // Copy specifiers.
                // External definition cannot have any specifiers, so no value will be overwritten.
                nodep->isHideLocal(protop->isHideLocal());
                nodep->isHideProtected(protop->isHideProtected());
                nodep->isStatic(protop->isStatic());
                nodep->isVirtual(protop->isVirtual());
                nodep->lifetime(protop->lifetime());
                // Always add an FUNC_RETURN_PROTO even if task, so we can locate the prototype
                nodep->ਜੋੜੋ_Stmtsp(ਨਵਾਂ Ast_Attr_ਦਾ{protop->fileline(), VAttrType::FUNC_RETURN_PROTO,
                                               protop->fvarp()
                                                   ? protop->fvarp()->clone_ਰੁੱਖ(ਝੂਠ)
                                                   : ਨਵਾਂ AstVoidDType{protop->fileline()}});
                ਲਈ (Ast_ਗੰਢ* stmtp = protop->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    ਜੇ (AstVar* ਅਚਲ portp = VN_CAST(stmtp, Var)) {
                        Ast_Attr_ਦਾ* ਅਚਲ attrp
                            = ਨਵਾਂ Ast_Attr_ਦਾ{protop->fileline(), VAttrType::FUNC_ARG_PROTO,
                                            portp->subDTypep()->clone_ਰੁੱਖ(ਝੂਠ)};
                        attrp->name(portp->name());
                        nodep->ਜੋੜੋ_Stmtsp(attrp);
                    }
                }
            } ਨਹੀਂਤਾਂ {
                nodep->v3error("extern not found that declares " + nodep->prettyNameQ());
            }
        }
        ਜੇ (nodep->isExternProto()) {
            ਜੇ (!m_curSymp->ਲੱਭੋ_Id_Fallback(nodep->name())) {
                nodep->v3warn(PROTOTYPEMIS,
                              "Definition not found for extern prototype " + nodep->prettyNameQ());
            }
        }
        VL_RESTORER(m_curSymp);
        VL_RESTORER(m_ftaskp);
        VL_RESTORER(m_explicitSuperNewp);
        {
            m_ftaskp = nodep;
            m_ds.m_dotSymp = m_curSymp = m_statep->get_ਗੰਢ_Sym(nodep);
            ਅਚਲ ਤਾਰਕਿਕ isNew = nodep->name() == "new";
            ਜੇ (isNew) m_explicitSuperNewp = ਖਾਲੀ_ਸੂਚਕ;
            iterateChildren(nodep);
            ਜੇ (isNew) {
                ਅਚਲ AstClassExtends* ਅਚਲ classExtendsp = VN_AS(m_modp, Class)->extendsp();
                ਜੇ (m_explicitSuperNewp && !m_explicitSuperNewp->isImplicit() && classExtendsp
                    && classExtendsp->argsp()) {
                    m_explicitSuperNewp->v3error(
                        "Explicit super.new not allowed with class "
                        "extends arguments (IEEE 1800-2023 8.17)\n"
                        << m_explicitSuperNewp->warn_ਹੋਰ() << "... Suggest remove super.new\n"
                        << m_explicitSuperNewp->warnContextPrimary() << '\n'
                        << classExtendsp->argsp()->warnOther()
                        << "... Location of extends argument(s)\n"
                        << classExtendsp->argsp()->warnContextSecondary());
                }
                ਜੇ (classExtendsp && classExtendsp->classOrNullp()) {
                    ਜੇ (!m_explicitSuperNewp && m_statep->forParamed()
                        && !VN_AS(classExtendsp->ਬੱਚਾ_DTypep()->skipRefp(), ClassRefDType)
                                ->classp()
                                ->isInterfaceClass()) {
                        Ast_ਗੰਢ_Stmt* ਅਚਲ superNewp
                            = ਜੋੜੋ_Implicit_Super_New_Call(VN_AS(nodep, Func), classExtendsp);
                        UINFO(9, "created super new " << superNewp);
                        iterate(superNewp);
                    }
                }
            }
        }
        m_ds.m_dotSymp = VL_RESTORER_PREV(m_curSymp);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Foreach* nodep) override {
        LINKDOT_ਦੇਖੋ_ਸ਼ੁਰੂ();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        symIterateChildren(nodep, m_statep->get_ਗੰਢ_Sym(nodep));
    }
    ਖਾਲੀ ਦੇਖੋ(AstRandSequence* nodep) override {
        LINKDOT_ਦੇਖੋ_ਸ਼ੁਰੂ();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        ਜੇ (!nodep->ਸ਼ੁਰੂ().ਖਾਲੀ_ਥਾਵਾਂ())
            nodep->prodp(ਲੱਭੋ_Prod(nodep, m_statep->get_ਗੰਢ_Sym(nodep), nodep->ਸ਼ੁਰੂ()));
        symIterateChildren(nodep, m_statep->get_ਗੰਢ_Sym(nodep));
    }
    ਖਾਲੀ ਦੇਖੋ(AstRSProd* nodep) override {
        LINKDOT_ਦੇਖੋ_ਸ਼ੁਰੂ();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        symIterateChildren(nodep, m_statep->get_ਗੰਢ_Sym(nodep));
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_RSProd_ਵਸਤੂ* nodep) override {
        LINKDOT_ਦੇਖੋ_ਸ਼ੁਰੂ();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        nodep->prodp(ਲੱਭੋ_Prod(nodep, m_curSymp, nodep->name()));
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstCoverWith* nodep) override {
        LINKDOT_ਦੇਖੋ_ਸ਼ੁਰੂ();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        iterate_And_ਅਗਲਾ_Null(nodep->subp());
        // The type of 'item' is that of the coverpoint expression, which may name 'item' too
        iterate_And_ਅਗਲਾ_Null(nodep->itemp()->ਬੱਚਾ_DTypep());
        VL_RESTORER(m_curSymp);
        m_ds.m_dotSymp = m_curSymp = m_statep->get_ਗੰਢ_Sym(nodep);
        iterate_And_ਅਗਲਾ_Null(nodep->itemp());
        iterate_And_ਅਗਲਾ_Null(nodep->filterp());
        m_ds.m_dotSymp = VL_RESTORER_PREV(m_curSymp);
    }
    ਖਾਲੀ ਦੇਖੋ(AstCoverCrossWith* nodep) override {
        LINKDOT_ਦੇਖੋ_ਸ਼ੁਰੂ();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        iterate_And_ਅਗਲਾ_Null(nodep->selectp());
        iterate_And_ਅਗਲਾ_Null(nodep->countp());
        // The type of a cross item is that of its coverpoint's expression, which may name items
        iterate_And_ਅਗਲਾ_Null(nodep->varsp());
        VL_RESTORER(m_curSymp);
        m_ds.m_dotSymp = m_curSymp = m_statep->get_ਗੰਢ_Sym(nodep);
        iterate_And_ਅਗਲਾ_Null(nodep->filterp());
        m_ds.m_dotSymp = VL_RESTORER_PREV(m_curSymp);
    }
    ਖਾਲੀ ਦੇਖੋ(AstWith* nodep) override {
        LINKDOT_ਦੇਖੋ_ਸ਼ੁਰੂ();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        ਜੇ (m_ds.m_unresolvedClass) ਵਾਪਸ;
        VL_RESTORER(m_curSymp);
        VL_RESTORER(m_currentWithp);
        VL_RESTORER_COPY(m_restricted_ਨਾਂ_ਵਰਤਿਆ);
        {
            m_ds.m_dotSymp = m_curSymp = m_statep->get_ਗੰਢ_Sym(nodep);
            m_currentWithp = nodep;
            // Validate the identifier_list once per AstWith (m_validated travels
            // with cloneTree, so cloned witnesses skip the redundant check).
            ਅਚਲ ਤਾਰਕਿਕ ਪਹਿਲਾ_ਦੇਖੋ = !nodep->validated();
            ਜੇ (ਪਹਿਲਾ_ਦੇਖੋ) {
                nodep->validated(ਸੱਚ);
                m_restricted_ਨਾਂ_ਵਰਤਿਆ.clear();
                // IEEE 1800-2023 18.7: each name in the identifier_list must designate
                // a member of the randomize() target class.
                ਜੇ (nodep->restricted() && m_randSymp) {
                    ਲਈ (ਅਚਲ std::string& n : nodep->restricted_ਨਾਂ()) {
                        ਜੇ (!m_randSymp->ਲੱਭੋ_Id_Flat(n)) {
                            nodep->v3error("Identifier '"
                                           << n
                                           << "' in 'with (...)' identifier list is not a"
                                              " member of the randomize() target class"
                                              " (IEEE 1800-2023 18.7)");
                            m_restricted_ਨਾਂ_ਵਰਤਿਆ.ਪਾਓ(n);
                        }
                    }
                }
            }
            iterateChildren(nodep);
            // Flag names that the user listed but never referenced in the constraint.
            ਜੇ (ਪਹਿਲਾ_ਦੇਖੋ && nodep->restricted()) {
                ਲਈ (ਅਚਲ std::string& n : nodep->restricted_ਨਾਂ()) {
                    ਜੇ (!m_restricted_ਨਾਂ_ਵਰਤਿਆ.ਗਿਣਤੀ_2(n)) {
                        nodep->v3warn(ਅਣਵਰਤਿਆ_2, "Identifier '"
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
    ਖਾਲੀ ਦੇਖੋ(AstLambdaArgRef* nodep) override {
        LINKDOT_ਦੇਖੋ_ਸ਼ੁਰੂ();
        UINFO(5, indent() << "visit " << nodep);
        // No checknodot(nodep), visit(AstScope) will check for LambdaArgRef
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstClassExtends* nodep) override {
        // Resolve the symbol and get the class.
        // If it is a parameterized case, the class will be resolved after V3Param.cpp
        ਜੇ (nodep->user3_ਸੈੱਟ_Once()) ਵਾਪਸ;
        // If the class is resolved, there is nothing more to do
        ਜੇ (nodep->classOrNullp()) ਵਾਪਸ;
        LINKDOT_ਦੇਖੋ_ਸ਼ੁਰੂ();
        ਜੇ (m_statep->forPrimary()) {
            ਜੇ (nodep->ਬੱਚਾ_DTypep()) ਵਾਪਸ;
            Ast_ਗੰਢ* cprp = nodep->classOrPkgsp();
            VSymEnt* lookSymp = m_curSymp;
            ਜੇ (AstDot* ਅਚਲ dotp = VN_CAST(cprp, Dot)) {
                dotp->user3(ਸੱਚ);
                ਜੇ (AstClassOrPackageRef* lookNodep = VN_CAST(dotp->lhsp(), ClassOrPackageRef)) {
                    iterate(lookNodep);
                    cprp = dotp->rhsp();
                    VSymEnt* ਅਚਲ foundp = m_statep->resolveClassOrPackage(
                        lookSymp, lookNodep, ਸੱਚ, ਝੂਠ, nodep->verilogKwd());
                    ਜੇ (!foundp) {
                        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep->unlink_Fr_ਪਿੱਠ()), nodep);
                        ਵਾਪਸ;
                    }
                    UASSERT_OBJ(lookNodep->classOrPackageSkipp(), nodep, "Bad package link");
                    lookSymp = m_statep->get_ਗੰਢ_Sym(lookNodep->classOrPackageSkipp());
                } ਨਹੀਂਤਾਂ {
                    dotp->lhsp()->v3error("Attempting to extend"  // LCOV_EXCL_LINE
                                          " using non-class under dot");
                }
            }
            AstClassOrPackageRef* ਅਚਲ cpackagerefp = VN_CAST(cprp, ClassOrPackageRef);
            ਜੇ (VL_UNCOVERABLE(!cpackagerefp)) {
                // Linking the extend gives an error before this is hit
                nodep->v3error("Attempting to extend using non-class");  // LCOV_EXCL_LINE
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep->unlink_Fr_ਪਿੱਠ()), nodep);
                ਵਾਪਸ;
            }
            VSymEnt* ਅਚਲ foundp = m_statep->resolveClassOrPackage(lookSymp, cpackagerefp, ਸੱਚ,
                                                                    ਸੱਚ, nodep->verilogKwd());
            ਜੇ (!foundp) {
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep->unlink_Fr_ਪਿੱਠ()), nodep);
                ਵਾਪਸ;
            }
            ਜੇ (AstClass* ਅਚਲ classp = VN_CAST(foundp->nodep(), Class)) {
                AstPin* paramsp = cpackagerefp->paramsp();
                ਜੇ (paramsp) {
                    paramsp = paramsp->clone_ਰੁੱਖ(ਸੱਚ);
                    nodep->parameterized(ਸੱਚ);
                }
                nodep->ਬੱਚਾ_DTypep(ਨਵਾਂ AstClassRefDType{nodep->fileline(), classp, paramsp});
                // Link pins
                iterate(nodep->ਬੱਚਾ_DTypep());
            } ਨਹੀਂਤਾਂ ਜੇ (AstParamTypeDType* ਅਚਲ paramp
                       = VN_CAST(foundp->nodep(), ParamTypeDType)) {
                AstRefDType* ਅਚਲ refParamp = ਨਵਾਂ AstRefDType{nodep->fileline(), paramp->name()};
                refParamp->refDTypep(paramp);
                nodep->ਬੱਚਾ_DTypep(refParamp);
                nodep->parameterized(ਸੱਚ);
            } ਨਹੀਂਤਾਂ ਜੇ (AstTypedef* ਅਚਲ typedefp = VN_CAST(foundp->nodep(), Typedef)) {
                Ast_ਗੰਢ_DType* ਅਚਲ unwrappedp = typedefp->subDTypep()->skipRefp();
                ਜੇ (AstClassRefDType* ਅਚਲ classRefp = VN_CAST(unwrappedp, ClassRefDType)) {
                    AstPin* paramsp = cpackagerefp->paramsp();
                    ਜੇ (!paramsp && classRefp->paramsp()) {
                        // No explicit #(...) on extends clause; carry over the
                        // typedef's type-parameter pins so V3Param sees them.
                        paramsp = classRefp->paramsp()->clone_ਰੁੱਖ(ਸੱਚ);
                        nodep->parameterized(ਸੱਚ);
                    } ਨਹੀਂਤਾਂ ਜੇ (paramsp) {
                        paramsp = paramsp->clone_ਰੁੱਖ(ਸੱਚ);
                        nodep->parameterized(ਸੱਚ);
                    }
                    nodep->ਬੱਚਾ_DTypep(
                        ਨਵਾਂ AstClassRefDType{nodep->fileline(), classRefp->classp(), paramsp});
                    iterate(nodep->ਬੱਚਾ_DTypep());
                } ਨਹੀਂਤਾਂ {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: " << foundp->nodep()->prettyTypeName()
                                                  << " in 'class extends'");
                    ਵਾਪਸ;
                }
            } ਨਹੀਂਤਾਂ {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: " << foundp->nodep()->prettyTypeName()
                                                             << " in 'class extends'");
                ਵਾਪਸ;
            }
            ਜੇ (!nodep->ਬੱਚਾ_DTypep()) nodep->v3error("Attempting to extend using non-class");
            nodep->classOrPkgsp()->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ();
        } ਨਹੀਂਤਾਂ {
            // Probably a parameter
            ਜੇ (AstRefDType* ਅਚਲ refp = VN_CAST(nodep->ਬੱਚਾ_DTypep(), RefDType)) {
                ਜੇ (AstClassRefDType* classRefp = VN_CAST(refp->skipRefp(), ClassRefDType)) {
                    // Resolved to a class reference.
                    refp->replaceWith(classRefp->clone_ਰੁੱਖ(ਝੂਠ));
                    VL_DO_DANGLING(ਧੱਕੋ_Deletep(refp), refp);
                } ਨਹੀਂਤਾਂ {
                    // Unable to resolve the ref type to a class reference.
                    // Get the value of type parameter passed to the class instance,
                    // to print the helpful error message.
                    ਅਚਲ Ast_ਗੰਢ_DType* typep = refp->refDTypep();
                    ਜਦੋਂ (ਸੱਚ) {
                        ਜੇ (ਅਚਲ AstParamTypeDType* ਅਚਲ atypep
                            = VN_CAST(typep, ParamTypeDType)) {
                            typep = atypep->subDTypep();
                            ਜਾਰੀ;
                        }
                        ਜੇ (ਅਚਲ AstRequireDType* ਅਚਲ atypep = VN_CAST(typep, RequireDType)) {
                            typep = atypep->subDTypep();
                            ਜਾਰੀ;
                        }
                        ਰੋਕੋ;
                    }
                    typep->v3error("Attempting to extend using non-class");
                }
            }
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstClass* nodep) override {
        ਜੇ (nodep->user3_ਸੈੱਟ_Once()) ਵਾਪਸ;
        LINKDOT_ਦੇਖੋ_ਸ਼ੁਰੂ();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        VL_RESTORER(m_curSymp);
        VL_RESTORER(m_modSymp);
        VL_RESTORER(m_modp);
        VL_RESTORER_COPY(m_if_Class_Imp_ਨਾਂ);
        VL_RESTORER_COPY(m_ifForwards);
        VL_RESTORER(m_insideClassExtParam);
        {
            m_ifForwards.clear();
            m_ds.init(m_curSymp);
            m_insideClassExtParam = nodep->isCovergroup() && nodep->covergroupEnclosingClassp()
                                    && m_extendsParam.ਲੱਭੋ(nodep->covergroupEnclosingClassp())
                                           != m_extendsParam.ਸਿਰਾ();
            // Until overridden by a SCOPE
            m_ds.m_dotSymp = m_curSymp = m_modSymp = m_statep->get_ਗੰਢ_Sym(nodep);
            m_modp = nodep;
            ਪੂਰਨ ਅਗਲਾ = 0;
            ਲਈ (AstClassExtends* cextp = nodep->extendsp(); cextp;
                 cextp = VN_AS(cextp->nextp(), ClassExtends)) {
                // Replace abstract reference with hard pointer
                // Will need later resolution when deal with parameters
                ਜੇ (++ਅਗਲਾ == 2 && !nodep->isInterfaceClass() && !cextp->isImplements()) {
                    cextp->v3error("Multiple inheritance illegal on non-interface classes"
                                   " (IEEE 1800-2023 8.13)");
                }
                iterate(cextp);
                ਜੇ (m_statep->forPrimary()) {
                    ਜੇ (cextp->parameterized()) {
                        // Parameters in extends statement.
                        // The class can't be resolved in the current pass.
                        m_extendsParam.ਪਾਓ(nodep);
                        m_insideClassExtParam = ਸੱਚ;
                    }
                    ਜੇ (AstClassRefDType* ਅਚਲ classRefp
                        = VN_CAST(cextp->ਬੱਚਾ_DTypep(), ClassRefDType)) {
                        AstClass* ਅਚਲ classp = classRefp->classp();
                        ਜੇ (classp != nodep) iterate(classp);
                        ਜੇ (m_extendsParam.ਲੱਭੋ(classp) != m_extendsParam.ਸਿਰਾ()) {
                            // One of its super classes has parameters in extends statement.
                            // Some links may not be resolved in the first pass.
                            m_extendsParam.ਪਾਓ(nodep);
                            m_insideClassExtParam = ਸੱਚ;
                        }
                    }
                }

                ਜੇ (AstClass* ਅਚਲ ਅਧਾਰ_Classp = cextp->classOrNullp()) {
                    // Already converted. Update symbol table to link unlinked members.
                    // Base class has to be visited in a case if its extends statement
                    // needs to be handled. Recursive inheritance was already checked.
                    // Must be here instead of in LinkDotParam to handle
                    // "class (type T) extends T".
                    ਜੇ (ਅਧਾਰ_Classp == nodep) {
                        cextp->v3error("Attempting to extend class " << nodep->prettyNameQ()
                                                                     << " from itself");
                    } ਨਹੀਂਤਾਂ ਜੇ (extendsClass(ਅਧਾਰ_Classp, nodep)) {
                        cextp->v3error("Attempting to extend class "
                                       << nodep->prettyNameQ() << " from "
                                       << ਅਧਾਰ_Classp->prettyNameQ()
                                       << ", which circularly inherits from it");
                    } ਨਹੀਂਤਾਂ ਜੇ (cextp->isImplements() && !ਅਧਾਰ_Classp->isInterfaceClass()) {
                        cextp->v3error("Attempting to implement from non-interface class "
                                       << ਅਧਾਰ_Classp->prettyNameQ() << '\n'
                                       << "... Suggest use 'extends'");
                    } ਨਹੀਂਤਾਂ ਜੇ (!cextp->isImplements() && !nodep->isInterfaceClass()
                               && ਅਧਾਰ_Classp->isInterfaceClass()) {
                        cextp->v3error("Attempting to extend from interface class "
                                       << ਅਧਾਰ_Classp->prettyNameQ() << '\n'
                                       << "... Suggest use 'implements'");
                    }
                    ਅਧਾਰ_Classp->isExtended(ਸੱਚ);
                    nodep->isExtended(ਸੱਚ);
                    iterate(ਅਧਾਰ_Classp);
                    importSymbolsFromExtended(nodep, cextp);
                    ਜਾਰੀ;
                }
            }
            m_ds.m_dotSymp = m_curSymp;

            iterateChildren(nodep);
            ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& itr : m_ifForwards) {
                ਅਚਲ VSymEnt* ਅਚਲ implSymp = itr.ਦੂਜਾ;
                ਜੋੜੋ_Interface_Forward(nodep, VN_AS(implSymp->nodep(), ਗੰਢ_FTask),
                                    VN_AS(implSymp->parentp()->nodep(), Class));
            }
        }
        // V3Width when determines types needs to find enum values and such
        // so add members pointing to appropriate enum values
        {
            VMemberMap memberMap;
            ਲਈ (VSymEnt::const_iterator it = m_curSymp->begin(); it != m_curSymp->ਸਿਰਾ(); ++it) {
                Ast_ਗੰਢ* ਅਚਲ itemp = it->ਦੂਜਾ->nodep();
                ਜੇ (!memberMap.ਲੱਭੋ_Member(nodep, it->ਪਹਿਲਾ)) {
                    ਜੇ (Ast_Enum_ਵਸਤੂ* ਅਚਲ aitemp = VN_CAST(itemp, Enum_ਵਸਤੂ)) {
                        Ast_Enum_ਵਸਤੂ_Ref* ਅਚਲ newp = ਨਵਾਂ Ast_Enum_ਵਸਤੂ_Ref{
                            aitemp->fileline(), aitemp, it->ਦੂਜਾ->classOrPackagep()};
                        UINFO(8, indent()
                                     << "Class import noderef '" << it->ਪਹਿਲਾ << "' " << newp);
                        nodep->ਜੋੜੋ_Membersp(newp);
                        memberMap.ਪਾਓ(nodep, newp);
                    }
                }
            }
        }
        m_ds.m_dotSymp = VL_RESTORER_PREV(m_curSymp);
    }

    // Collect the ClassOrPackageRefs in a chained scope operand in source order.
    // The grammar builds a scope operand only from packageClassScopeItem, which
    // always yields a ClassOrPackageRef, nested in Dots, so nothing else appears.
    ਸਥਿਰ ਖਾਲੀ collectScopeRefs(Ast_ਗੰਢ* nodep, std::ਸਦਿਸ਼<AstClassOrPackageRef*>& refps) {
        ਜੇ (AstClassOrPackageRef* ਅਚਲ refp = VN_CAST(nodep, ClassOrPackageRef)) {
            refps.ਧੱਕੋ_ਪਿੱਠ(refp);
            ਵਾਪਸ;
        }
        AstDot* ਅਚਲ dotp = VN_CAST(nodep, Dot);
        UASSERT_OBJ(dotp, nodep, "Non-scope node in package/class scope operand");
        collectScopeRefs(dotp->lhsp(), refps);
        collectScopeRefs(dotp->rhsp(), refps);
    }

    // Resolve a chained class-scope operand (`pkg::outer::inner` in
    // `pkg::outer::inner::t`) and reduce it to its innermost ClassOrPackageRef.
    // Returns false, having reported, if a segment could not be resolved.
    ਤਾਰਕਿਕ reduceScopeDot(AstRefDType* nodep, AstDot* scopeDotp) {
        std::ਸਦਿਸ਼<AstClassOrPackageRef*> refps;
        collectScopeRefs(scopeDotp, refps);

        VSymEnt* scopeSymp = m_ds.m_dotSymp;
        ਲਈ (ਮਾਪ_t i = 0; i < refps.ਮਾਪ(); ++i) {
            AstClassOrPackageRef* ਅਚਲ refp = refps[i];
            ਜੇ (!refp->classOrPackageSkipp() && !refp->classOrPackageNodep()
                && !m_statep->resolveClassOrPackage(scopeSymp, refp, i == 0, ਝੂਠ,
                                                    "class/package reference")) {
                ਵਾਪਸ ਝੂਠ;  // Error already reported
            }
            ਜੇ (i + 1 == refps.ਮਾਪ()) ਰੋਕੋ;
            // A middle segment that resolved to a node with no reachable module
            // (e.g. a typedef alias of a not-yet-specialized parameterized class)
            // gives nothing to look the next segment up in.
            Ast_ਗੰਢ_Module* ਅਚਲ modp = refp->classOrPackageSkipp();
            ਜੇ (!modp) {
                refp->v3warn(E_UNSUPPORTED, "Unsupported: Multiple '::' package/class reference");
                ਵਾਪਸ ਝੂਠ;
            }
            scopeSymp = m_statep->get_ਗੰਢ_Sym(modp);
        }
        AstClassOrPackageRef* ਅਚਲ innerp = refps.ਪਿੱਠ();
        innerp->unlink_Fr_ਪਿੱਠ();
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(scopeDotp->unlink_Fr_ਪਿੱਠ()), scopeDotp);
        nodep->classOrPackageOpp(innerp);
        ਵਾਪਸ ਸੱਚ;
    }

    ਖਾਲੀ ਦੇਖੋ(AstRefDType* nodep) override {

        ਜੇ (ਸਵੈਚਾਲਿਤ* ਅਚਲ typeOfp = nodep->typeofp()) {
            iterate(typeOfp);
            // After iteration, typeofp() may have been replaced with a resolved type
            // (e.g., DOT expression like if0.rq_t resolved to REFDTYPE)
            // If it's now a NodeDType, replace this RefDType with it
            ਜੇ (Ast_ਗੰਢ_DType* ਅਚਲ resolvedDTypep = VN_CAST(nodep->typeofp(), ਗੰਢ_DType)) {
                UINFO(9,
                      indent() << "iface capture typeofp resolved to dtype, replacing RefDType\n");
                resolvedDTypep->unlink_Fr_ਪਿੱਠ();
                nodep->replaceWith(resolvedDTypep);
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
                // Originally (PR #6637) this block re-captured a resolved
                // RefDType into the iface-capture ledger when user2p held a
                // Cell, to ensure re-resolution during the paramed pass.
                // In practice, the typeofp-resolved RefDType never carries a
                // Cell in user2p: the capture happens earlier when the
                // typedef is first linked.
                ਜੇ (V3LinkDotIfaceCapture::enabled()) {
                    ਜੇ (AstRefDType* ਅਚਲ resolvedRefp = VN_CAST(resolvedDTypep, RefDType)) {
                        ਜੇ (VL_UNCOVERABLE(
                                VN_IS(resolvedRefp->user2p(), Cell))) {  // LCOV_EXCL_LINE
                            resolvedRefp->v3fatalSrc(  // LCOV_EXCL_LINE
                                "typeofp resolved RefDType has Cell in user2p;"
                                " expected to be captured already");
                        }
                    }
                }
            }
            ਵਾਪਸ;
        }

        // Resolve its reference
        ਜੇ (m_statep->forParamed() && nodep->user3()) {
            ਜੇ (V3LinkDotIfaceCapture::enabled() && nodep->user2p()) {
                UINFO(9, indent() << "iface capture clear user3 for captured typedef name="
                                  << nodep->name() << " cell=" << nodep->user2p());
            }
            nodep->user3(ਝੂਠ);
        }
        ਜੇ (nodep->user3_ਸੈੱਟ_Once()) {
            ਜੇ (V3LinkDotIfaceCapture::enabled() && nodep->user2p()) {
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
            ਜੇ (VL_UNCOVERABLE(m_statep->forParamed() && nodep->classOrPackageOpp())) {
                nodep->v3fatalSrc(  // LCOV_EXCL_LINE
                    "RefDType user3 revisit with classOrPackageOpp in paramed pass");
            }
            ਵਾਪਸ;
        }
        LINKDOT_ਦੇਖੋ_ਸ਼ੁਰੂ();
        UINFO(5, indent() << "visit " << nodep);
        ਜੇ (nodep->classOrPackageOpp()) {
            // Resolve a chained scope (`pkg::cls::t`, `pkg::cls#(P)::t`) from outside in.
            // On failure reduceScopeDot has reported, so stop before using the operand.
            ਜੇ (AstDot* ਅਚਲ scopeDotp = VN_CAST(nodep->classOrPackageOpp(), Dot)) {
                ਜੇ (!reduceScopeDot(nodep, scopeDotp)) ਵਾਪਸ;
            }
            // Re-read: reduceScopeDot may have replaced the operand
            Ast_ਗੰਢ* ਅਚਲ cpackagep = nodep->classOrPackageOpp();
            UASSERT_OBJ(!VN_IS(cpackagep, Dot), cpackagep,
                        "Package/class scope operand should be reduced by now");
            ਜੇ (AstClassOrPackageRef* ਅਚਲ cpackagerefp = VN_CAST(cpackagep, ClassOrPackageRef)) {
                iterate(cpackagerefp);
                ਅਚਲ AstClass* ਅਚਲ clsp = VN_CAST(cpackagerefp->classOrPackageNodep(), Class);
                ਜੇ (clsp && clsp->hasGParam()) {
                    // Unable to link before the instantiation of parameter classes.
                    // The class reference node still has to be visited now to later link
                    // parameters.
                    iterate(cpackagep);
                    ਵਾਪਸ;
                }
                // Defer non-typedef references through typedef aliases of parameterized classes.
                ਜੇ (m_statep->forPrimary() && !VN_IS(nodep->backp(), Typedef)
                    && isParamedClassRef(cpackagerefp)) {
                    iterate(cpackagep);
                    ਵਾਪਸ;
                }

                ਅਚਲ ਤਾਰਕਿਕ doDefaultTypedef = !(m_resolvingTypedef && m_statep->forPrimary());
                ਜੇ (!cpackagerefp->classOrPackageSkipp(doDefaultTypedef)
                    && !cpackagerefp->classOrPackageNodep()) {
                    VSymEnt* ਅਚਲ foundp = m_statep->resolveClassOrPackage(
                        m_ds.m_dotSymp, cpackagerefp, ਸੱਚ, ਝੂਠ, "class/package reference");
                    ਜੇ (!foundp) ਵਾਪਸ;
                }
                nodep->classOrPackagep(cpackagerefp->classOrPackageSkipp(doDefaultTypedef));
                ਜੇ (!VN_IS(nodep->classOrPackagep(), Class)
                    && !VN_IS(nodep->classOrPackagep(), Package)) {
                    ਜੇ (m_statep->forPrimary()) {
                        // It may be a type that comes from parameter class that is not
                        // instantioned yet
                        iterate(cpackagep);
                        ਵਾਪਸ;
                    }
                    // Likely impossible, as error thrown earlier
                    cpackagerefp->v3error(  // LCOV_EXCL_LINE
                        "'::' expected to reference a class/package but referenced '"
                        << (nodep->classOrPackagep() ? nodep->classOrPackagep()->prettyTypeName()
                                                     : "<unresolved-object>")
                        << "'\n"
                        << cpackagerefp->warn_ਹੋਰ() + "... Suggest '.' instead of '::'");
                }
            }
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(cpackagep->unlink_Fr_ਪਿੱਠ()), cpackagep);
        }

        ਜੇ (m_ds.m_dotp && (m_ds.m_dotPos == DP_PACKAGE || m_ds.m_dotPos == DP_SCOPE)) {
            UASSERT_OBJ(VN_IS(m_ds.m_dotp->lhsp(), ClassOrPackageRef), m_ds.m_dotp->lhsp(),
                        "Bad package link");
            ਸਵੈਚਾਲਿਤ* ਅਚਲ cpackagerefp = VN_CAST(m_ds.m_dotp->lhsp(), ClassOrPackageRef);
            UASSERT_OBJ(cpackagerefp->classOrPackageSkipp(), m_ds.m_dotp->lhsp(),
                        "Bad package link");
            nodep->classOrPackagep(cpackagerefp->classOrPackageSkipp());
            m_ds.m_dotPos = DP_SCOPE;
        } ਨਹੀਂਤਾਂ {
            // Allow REFDTYPE under DOT when referencing interface typedefs
            // This is needed for patterns like: typedef iface.a_t a_t;
            // The dependency graph handles resolution when IfaceCapture is disabled
            ਅਚਲ ਤਾਰਕਿਕ ifaceFinalSegmentAllowed
                = (m_ds.m_dotPos == DP_FINAL) && m_ds.m_dotSymp
                  && VN_IS(m_ds.m_dotSymp->nodep(), Cell)
                  && VN_CAST(m_ds.m_dotSymp->nodep(), Cell)->modp()
                  && VN_IS(VN_CAST(m_ds.m_dotSymp->nodep(), Cell)->modp(), Iface);
            ਜੇ (!ifaceFinalSegmentAllowed) {
                checkNoDot(nodep);
            } ਨਹੀਂਤਾਂ {
                UINFO(9, indent() << "allow REFDTYPE under DOT for iface typedef name="
                                  << nodep->prettyNameQ() << " dotSym=" << m_ds.m_dotSymp);
                // Clear the dot state so we don't propagate errors
                m_ds.m_dotPos = DP_SCOPE;
            }
        }
        ਜੇ (!nodep->typedefp() && !nodep->subDTypep()) {
            ਅਚਲ VSymEnt* foundp;
            ਜੇ (nodep->classOrPackagep()) {
                foundp = m_statep->get_ਗੰਢ_Sym(nodep->classOrPackagep())->ਲੱਭੋ_Id_Flat(nodep->name());
            } ਨਹੀਂਤਾਂ ਜੇ (m_ds.m_dotPos == DP_ਪਹਿਲਾ || m_ds.m_dotPos == DP_NONE) {
                foundp = ਲੱਭੋ_Id_Fallback_Skip_Member_DType(m_curSymp, nodep->name());
            } ਨਹੀਂਤਾਂ {
                // Defensive: dotPos should be DP_FIRST/DP_NONE or classOrPackagep set.
                v3fatalSrc("Unexpected dotPos="
                           << static_cast<ਪੂਰਨ>(m_ds.m_dotPos)  // LCOV_EXCL_LINE
                           << " in RefDType lookup for "
                           << nodep->prettyNameQ());  // LCOV_EXCL_LINE
                foundp = ਖਾਲੀ_ਸੂਚਕ;  // LCOV_EXCL_LINE
            }
            {
                ਜੇ (AstTypedef* ਅਚਲ defp
                    = foundp ? VN_CAST(foundp->nodep(), Typedef) : ਖਾਲੀ_ਸੂਚਕ) {
                    // Don't check if typedef is to a <type T>::<reference> as might not be
                    // resolved yet
                    ਜੇ (!nodep->classOrPackagep() && !defp->isUnderClass())
                        check_Decl_ਕ੍ਰਮ(nodep, defp);
                    nodep->typedefp(defp);
                    nodep->classOrPackagep(foundp->classOrPackagep());

                } ਨਹੀਂਤਾਂ ਜੇ (AstParamTypeDType* ਅਚਲ defp
                           = foundp ? VN_CAST(foundp->nodep(), ParamTypeDType) : ਖਾਲੀ_ਸੂਚਕ) {
                    // A PARAMTYPEDTYPE's child REFDTYPE referencing itself is the normal
                    // AST structure for type parameters (e.g., "type T = base" has a child
                    // REFDTYPE for the default type). Only error on true recursion where
                    // a TYPEDEF contains a reference back to itself.
                    ਅਚਲ ਤਾਰਕਿਕ is_Param_Type_ਬੱਚਾ = (defp == nodep->backp());
                    ਅਚਲ ਤਾਰਕਿਕ isTypedefRecursion
                        = is_Param_Type_ਬੱਚਾ && VN_IS(defp->backp(), Typedef);
                    ਜੇ (isTypedefRecursion) {
                        nodep->v3error("Reference to '"
                                       << m_ds.m_dot_ਪਾਠ << (m_ds.m_dot_ਪਾਠ == "" ? "" : ".")
                                       << nodep->prettyName() << "'"
                                       << " type would form a recursive definition");
                        nodep->refDTypep(nodep->ਲੱਭੋ_Void_DType());  // Try to reduce later errors
                    } ਨਹੀਂਤਾਂ {
                        nodep->refDTypep(defp);
                        nodep->classOrPackagep(foundp->classOrPackagep());
                        captureIfaceParamType(nodep, defp);
                    }
                } ਨਹੀਂਤਾਂ ਜੇ (AstClass* ਅਚਲ defp
                           = foundp ? VN_CAST(foundp->nodep(), Class) : ਖਾਲੀ_ਸੂਚਕ) {
                    // Don't check if typedef is to a <type T>::<reference> as might not be
                    // resolved yet
                    ਜੇ (!nodep->classOrPackagep()) check_Decl_ਕ੍ਰਮ(nodep, defp);
                    AstPin* ਅਚਲ paramsp = nodep->paramsp();
                    ਜੇ (paramsp) paramsp->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ();
                    AstClassRefDType* ਅਚਲ newp
                        = ਨਵਾਂ AstClassRefDType{nodep->fileline(), defp, paramsp};
                    newp->classOrPackagep(foundp->classOrPackagep());
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
                    ਵਾਪਸ;
                } ਨਹੀਂਤਾਂ ਜੇ (m_insideClassExtParam) {
                    ਵਾਪਸ;
                } ਨਹੀਂਤਾਂ {
                    ਜੇ (foundp) {
                        UINFO(1, "Found sym node: " << foundp->nodep());
                        nodep->v3error("Expecting a data type: "
                                       << nodep->prettyNameQ()
                                       << ", found: " << foundp->nodep()->prettyTypeName());
                    } ਨਹੀਂਤਾਂ {
                        nodep->v3error("Can't find typedef/interface: " << nodep->prettyNameQ());
                    }
                }
            }
        }
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstRequireDType* nodep) override {
        // Handle type validation for localparam type assignments
        // Iterate child to resolve any ParseRef nodes
        iterateChildren(nodep);
        // Only emit error if the child is not a type.
        // Do NOT unwrap valid types here - leave that to V3Width.
        // Unwrapping here breaks type parameter resolution during cloning.
        // Allow Dot through to defer the handling until the dot expression is resolved.
        ਜੇ (nodep->lhsp() && !VN_IS(nodep->lhsp(), ਗੰਢ_DType) && !VN_IS(nodep->lhsp(), Dot)) {
            // Not a type - emit error
            ਜੇ (VN_IS(nodep->lhsp(), Const)) {
                nodep->lhsp()->v3error("Expecting a data type, not a constant");
            } ਨਹੀਂਤਾਂ {
                nodep->lhsp()->v3error("Expecting a data type, not "
                                       << nodep->lhsp()->typeName() << ": '"
                                       << nodep->lhsp()->prettyName() << "'");
            }
            // Replace with void to keep AST valid and allow clean exit
            nodep->replaceWith(ਨਵਾਂ AstVoidDType{nodep->fileline()});
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstDpiExport* nodep) override {
        // AstDpiExport: Make sure the function referenced exists, then dump it
        LINKDOT_ਦੇਖੋ_ਸ਼ੁਰੂ();
        iterateChildren(nodep);
        checkNoDot(nodep);
        VSymEnt* ਅਚਲ foundp = m_curSymp->ਲੱਭੋ_Id_Fallback(nodep->name());
        Ast_ਗੰਢ_FTask* ਅਚਲ taskp = foundp ? VN_AS(foundp->nodep(), ਗੰਢ_FTask) : ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (!taskp) {
            nodep->v3error(
                "Can't find definition of exported task/function: " << nodep->prettyNameQ());
        } ਨਹੀਂਤਾਂ ਜੇ (taskp->dpiExport()) {
            nodep->v3error("Function was already DPI Exported, duplicate not allowed: "
                           << nodep->prettyNameQ());
        } ਨਹੀਂਤਾਂ {
            taskp->dpiExport(ਸੱਚ);
            ਜੇ (nodep->cname() != "") taskp->cname(nodep->cname());
        }
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep->unlink_Fr_ਪਿੱਠ()), nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstDisable* nodep) override {
        LINKDOT_ਦੇਖੋ_ਸ਼ੁਰੂ();
        checkNoDot(nodep);
        ਤਾਰਕਿਕ hasPartSelect = ਝੂਠ;
        ਅਚਲ string ਨਿਸ਼ਾਨਾ_ਰਸਤਾ
            = nodep->ਨਿਸ਼ਾਨਾ_Refp() ? ਕੱਢੋ_Dotted_ਰਸਤਾ(nodep->ਨਿਸ਼ਾਨਾ_Refp(), hasPartSelect) : "";
        VL_RESTORER_COPY(m_ds);
        m_ds.init(m_curSymp);
        m_ds.m_dotPos = DP_ਪਹਿਲਾ;
        m_ds.m_disablep = nodep;
        iterateChildren(nodep);
        ਜੇ (nodep->ਨਿਸ਼ਾਨਾ_Refp()) {
            ਜੇ (AstTaskRef* ਅਚਲ taskRefp = VN_CAST(nodep->ਨਿਸ਼ਾਨਾ_Refp(), TaskRef)) {
                nodep->targetp(taskRefp->taskp());
            } ਨਹੀਂਤਾਂ ਜੇ (!VN_IS(nodep->ਨਿਸ਼ਾਨਾ_Refp(), ParseRef)) {
                // If it is a ParseRef, either it couldn't be linked or it is linked to a block
                nodep->v3warn(E_UNSUPPORTED, "Node of type "
                                                 << nodep->ਨਿਸ਼ਾਨਾ_Refp()->prettyTypeName()
                                                 << " referenced by disable");
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep->unlink_Fr_ਪਿੱਠ()), nodep);
                ਵਾਪਸ;
            }
            ਜੇ (nodep->targetp()) {
                nodep->ਨਿਸ਼ਾਨਾ_Refp()->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ();
                ਜੇ (!hasPartSelect) {
                    ਅਚਲ string instance_ਰਸਤਾ = ਨਿਸ਼ਾਨਾ_Instance_ਰਸਤਾ(nodep->targetp(), ਨਿਸ਼ਾਨਾ_ਰਸਤਾ);
                    ਜੇ (!instance_ਰਸਤਾ.ਖਾਲੀ_ਥਾਵਾਂ()) {
                        // Keep only the instance prefix so V3LinkJump can reference the selected
                        // instance's process queue without reparsing the disable target.
                        nodep->ਨਿਸ਼ਾਨਾ_Refp(
                            ਨਵਾਂ AstVarXRef{nodep->fileline(), "", instance_ਰਸਤਾ, VAccess::READ});
                    }
                }
            }
        }
    }
    ਖਾਲੀ ਦੇਖੋ_Package_Import_Or_Export(Ast_ਗੰਢ* nodep) {
        // No longer needed
        LINKDOT_ਦੇਖੋ_ਸ਼ੁਰੂ();
        checkNoDot(nodep);
        ਜੇ (m_statep->forParamed()) VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep->unlink_Fr_ਪਿੱਠ()), nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstPackageImport* nodep) override { ਦੇਖੋ_Package_Import_Or_Export(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstPackageExport* nodep) override { ਦੇਖੋ_Package_Import_Or_Export(nodep); }
    ਖਾਲੀ ਦੇਖੋ(Ast_Package_Export_ਤਾਰਾ_ਤਾਰਾ* nodep) override { ਦੇਖੋ_Package_Import_Or_Export(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstCellRef* nodep) override {
        LINKDOT_ਦੇਖੋ_ਸ਼ੁਰੂ();
        UINFO(5, indent() << "visit " << nodep);
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstCellArrayRef* nodep) override {
        LINKDOT_ਦੇਖੋ_ਸ਼ੁਰੂ();
        UINFO(5, indent() << "visit " << nodep);
        // Expression already iterated
    }
    ਖਾਲੀ ਦੇਖੋ(AstUnlinkedRef* nodep) override {
        LINKDOT_ਦੇਖੋ_ਸ਼ੁਰੂ();
        UINFO(5, indent() << "visit " << nodep);
        // No need to iterate, if we have a UnlinkedVarXRef, we're already done
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Stmt_ਵਿਅੰਜਕ* nodep) override {
        LINKDOT_ਦੇਖੋ_ਸ਼ੁਰੂ();
        checkNoDot(nodep);
        // Check if nodep represents a super.new call;
        ਜੇ (AstNew* ਅਚਲ newExprp = VN_CAST(nodep->exprp(), New)) {
            // in this case it was already linked, so it doesn't have a super reference
            m_explicitSuperNewp = newExprp;
        } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ AstDot* ਅਚਲ dotp = VN_CAST(nodep->exprp(), Dot)) {
            ਜੇ (dotp->lhsp()->name() == "super") {
                ਜੇ (AstNew* ਅਚਲ newExprp = VN_CAST(dotp->rhsp(), New)) {
                    m_explicitSuperNewp = newExprp;
                }
            }
        }
        iterateChildren(nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(AstIfaceRefDType* nodep) override {
        ਜੇ (nodep->user3_ਸੈੱਟ_Once()) ਵਾਪਸ;
        LINKDOT_ਦੇਖੋ_ਸ਼ੁਰੂ();
        ਜੇ (nodep->paramsp()) {
            // If there is no parameters, there is no need to visit this node.
            AstIface* ਅਚਲ ifacep = nodep->ifacep();
            UASSERT_OBJ(ifacep, nodep, "Port parameters of AstIfaceRefDType without ifacep()");
            ਜੇ (ifacep->dead()) ਵਾਪਸ;
            checkNoDot(nodep);
            VL_RESTORER_CLEAR(m_ਵਰਤਿਆ_Pins);
            VL_RESTORER_CLEAR(m_ਵਰਤਿਆ_Def_Param_Pins);
            VL_RESTORER(m_pinSymp);
            m_pinSymp = m_statep->get_ਗੰਢ_Sym(ifacep);
            iterate_And_ਅਗਲਾ_Null(nodep->paramsp());
        }
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_Attr_ਦਾ* nodep) override { iterateChildren(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstCastParse* nodep) override {
        ਜੇ (nodep->user3_ਸੈੱਟ_Once()) ਵਾਪਸ;
        LINKDOT_ਦੇਖੋ_ਸ਼ੁਰੂ();
        // A cast may start a dotted expression: HERE type'(expr) . member_or_method
        ਅਚਲ ਤਾਰਕਿਕ dotLhs = m_ds.m_dotPos == DP_ਪਹਿਲਾ;
        ਜੇ (!dotLhs) checkNoDot(nodep);
        {
            VL_RESTORER_COPY(m_ds);
            m_ds.init(m_curSymp);
            iterateChildren(nodep);
        }
        ਜੇ (dotLhs) m_ds.m_dotPos = DP_MEMBER;
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਸੌਂਪੋ_W* nodep) override {
        LINKDOT_ਦੇਖੋ_ਸ਼ੁਰੂ();
        checkNoDot(nodep);
        VL_RESTORER(m_replaceWithAlias);
        ਜੇ (nodep->user2()) m_replaceWithAlias = ਝੂਠ;
        iterateChildren(nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(AstTypedef* nodep) override {
        VL_RESTORER(m_resolvingTypedef)
        m_resolvingTypedef = ਸੱਚ;
        iterateChildren(nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override {
        VL_RESTORER(m_ਵਿੱਚ_Packed_Array);
        ਜੇ (VN_IS(nodep, PackArrayDType)) {
            m_ਵਿੱਚ_Packed_Array = ਸੱਚ;
        } ਨਹੀਂਤਾਂ ਜੇ (!m_ਵਿੱਚ_Packed_Array) {
            LINKDOT_ਦੇਖੋ_ਸ਼ੁਰੂ();
            checkNoDot(nodep);
        }
        iterateChildren(nodep);
    }

ਜਨਤਕ:
    // CONSTRUCTORS
    LinkDotResolveVisitor(AstNetlist* rootp, LinkDotState* statep)
        : m_statep{statep} {
        UINFO(4, __FUNCTION__ << ": ");

        // Note: no need to clear user3 on ledger entries here.
        // VNUser3InUse (m_inuser3) already logically clears user3 on all
        // nodes via the generation counter.  The ledger may also hold stale
        // refp pointers to deleted AST nodes, so iterating it is unsafe.

        iterate(rootp);
        ਅਚਲ std::map<std::string, Ast_ਗੰਢ_Module*> modules_ਮੰਜ਼ਿਲ_Revisit
            = std::move(m_modules_ਮੰਜ਼ਿਲ_Revisit);
        m_ਆਖਰੀ_Deferredp = ਖਾਲੀ_ਸੂਚਕ;
        ਲਈ (ਸਵੈਚਾਲਿਤ& p : modules_ਮੰਜ਼ਿਲ_Revisit) {
            Ast_ਗੰਢ_Module* ਅਚਲ modp = p.ਦੂਜਾ;
            modp->foreach([](Ast_ਗੰਢ* ਅਚਲ nodep) { nodep->user3(ਝੂਠ); });
            iterate(modp);
        }
    }
    ~LinkDotResolveVisitor() override = ਮੂਲ;
};

//######################################################################
// Link class functions

ਖਾਲੀ V3LinkDot::dumpSubstep(ਅਚਲ string& name) {
    ਜੇ (dump_ਰੁੱਖ_Either_Level() >= 9) {
        V3Global::dump_Check_Global_ਰੁੱਖ(name);
    } ਨਹੀਂਤਾਂ ਜੇ (debug() >= 5) {  // on high dbg level, dump even if not explicitly told to
        v3Global.rootp()->dump_ਰੁੱਖ_File(v3Global.debugFilename(name + ".tree"));
    }
}

ਖਾਲੀ V3LinkDot::linkDotGuts(AstNetlist* rootp, VLink_Dot_ਕਦਮ ਕਦਮ) {
    VIsCached::clear_Cache_ਰੁੱਖ();  // Avoid using any stale isPure
    dumpSubstep("prelinkdot");
    LinkDotState state{rootp, ਕਦਮ};

    { Link_Dot_ਲੱਭੋ_Visitor{rootp, &state}; }
    dumpSubstep("prelinkdot-find");

    { Link_Dot_ਲੱਭੋ_Iface_Visitor{rootp, &state}; }
    dumpSubstep("prelinkdot-findiface");

    ਜੇ (ਕਦਮ == LDS_SCOPED) {
        // Well after the initial link when we're ready to operate on the flat design,
        // process AstScope's.  This needs to be separate pass after whole hierarchy graph created.
        { LinkDotScopeVisitor{rootp, &state}; }
        v3Global.assertScoped(ਸੱਚ);
        dumpSubstep("prelinkdot-scoped");
    } ਨਹੀਂਤਾਂ {
        // Initial link stage, resolve parameters and interfaces
        { LinkDotParamVisitor{rootp, &state}; }
        dumpSubstep("prelinkdot-param");
    }
    state.dumpSelf("prelinkdot");
    state.computeIfaceModSyms();
    state.computeIfaceVarSyms();
    state.computeScopeAliases();
    state.dumpSelf("linkdot-preresolve");
    ਅਚਲ LinkDotResolveVisitor visitor{rootp, &state};
    state.dumpSelf("linkdot-done");
}

ਖਾਲੀ V3LinkDot::linkDotPrimary(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    linkDotGuts(nodep, LDS_PRIMARY);
    V3Global::dump_Check_Global_ਰੁੱਖ("linkdot", 0, dump_ਰੁੱਖ_Either_Level() >= 6);
}

ਖਾਲੀ V3LinkDot::linkDotParamed(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    linkDotGuts(nodep, LDS_PARAMED);
    V3Global::dump_Check_Global_ਰੁੱਖ("linkdotparam", 0, dump_ਰੁੱਖ_Either_Level() >= 3);
}

ਖਾਲੀ V3LinkDot::linkDotScope(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    linkDotGuts(nodep, LDS_SCOPED);
    V3Global::dump_Check_Global_ਰੁੱਖ("linkdot", 0, dump_ਰੁੱਖ_Either_Level() >= 3);
}
