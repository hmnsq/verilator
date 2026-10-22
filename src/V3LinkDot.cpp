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
//உச்சி
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

#அந்தர்பாவ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#அந்தர்பாவ "V3LinkDot.h"

#அந்தர்பாவ "V3Global.h"
#அந்தர்பாவ "V3Graph.h"
#அந்தர்பாவ "V3LinkDotIfaceCapture.h"
#அந்தர்பாவ "V3MemberMap.h"
#அந்தர்பாவ "V3Parse.h"
#அந்தர்பாவ "V3Randomize.h"
#அந்தர்பாவ "V3Stats.h"
#அந்தர்பாவ "V3String.h"
#அந்தர்பாவ "V3SymTable.h"

#அந்தர்பாவ <algorithm>
#அந்தர்பாவ <cstdlib>
#அந்தர்பாவ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

நிலையான string extractDottedPath(Ast_முனை* nodep, தருக்க& hasPartSelect) {
    எனில் (AstParseRef* மாறிலி refp = VN_CAST(nodep, ParseRef)) {
        திரும்பு refp->name();
    } இல்லையெனில் எனில் (AstVarRef* மாறிலி refp = VN_CAST(nodep, VarRef)) {
        திரும்பு refp->name();
    } இல்லையெனில் எனில் (AstDot* மாறிலி dotp = VN_CAST(nodep, Dot)) {
        மாறிலி string lhs = extractDottedPath(dotp->lhsp(), hasPartSelect);
        மாறிலி string rhs = extractDottedPath(dotp->rhsp(), hasPartSelect);
        திரும்பு VString::dot(lhs, ".", rhs);
    } இல்லையெனில் எனில் (VN_IS(nodep, SelBit) || VN_IS(nodep, SelExtract) || VN_IS(nodep, SelPlus)
               || VN_IS(nodep, SelMinus)) {
        hasPartSelect = உண்மை;
        திரும்பு "";
    }
    திரும்பு "";
}
நிலையான string lexicalDisablePath(Ast_முனை* மாறிலி targetp) {
    std::vector<string> names;
    க்கு (Ast_முனை* curp = targetp; curp; curp = curp->aboveLoopp()) {
        எனில் (Ast_முனை_Block* மாறிலி blockp = VN_CAST(curp, முனை_Block)) {
            எனில் (blockp->name() != "") names.தள்ளு_back(blockp->name());
        } இல்லையெனில் எனில் (Ast_முனை_FTask* மாறிலி ftaskp = VN_CAST(curp, முனை_FTask)) {
            names.தள்ளு_back(ftaskp->name());
        } இல்லையெனில் எனில் (VN_IS(curp, முனை_Module)) {
            நிறுத்து;
        }
    }
    string path;
    க்கு (auto it = names.crbegin(); it != names.crend(); ++it) {
        path = VString::dot(path, ".", *it);
    }
    திரும்பு path;
}
நிலையான string targetInstancePath(Ast_முனை* மாறிலி targetp, மாறிலி string& targetPath) {
    மாறிலி string lexicalPath = lexicalDisablePath(targetp);
    எனில் (lexicalPath == "" || targetPath == lexicalPath) திரும்பு "";
    மாறிலி string suffix = "." + lexicalPath;
    எனில் (!VString::endsWith(targetPath, suffix)) திரும்பு "";
    திரும்பு targetPath.substr(0, targetPath.size() - suffix.size());
}

// ######################################################################
//  Matcher classes (for suggestion matching)

வகுப்பு Link_முனை_Matcher_Class_Or_Package final : பொது VNodeMatcher {
பொது:
    தருக்க முனை_Match(மாறிலி Ast_முனை* nodep) மாறிலி override {
        திரும்பு VN_IS(nodep, Class) || VN_IS(nodep, Package);
    }
};
வகுப்பு Link_முனை_Matcher_FTask final : பொது VNodeMatcher {
பொது:
    தருக்க முனை_Match(மாறிலி Ast_முனை* nodep) மாறிலி override { திரும்பு VN_IS(nodep, முனை_FTask); }
};
வகுப்பு Link_முனை_Matcher_Modport final : பொது VNodeMatcher {
பொது:
    தருக்க முனை_Match(மாறிலி Ast_முனை* nodep) மாறிலி override { திரும்பு VN_IS(nodep, Modport); }
};
வகுப்பு Link_முனை_Matcher_Prod final : பொது VNodeMatcher {
பொது:
    தருக்க முனை_Match(மாறிலி Ast_முனை* nodep) மாறிலி override { திரும்பு VN_IS(nodep, RSProd); }
};
வகுப்பு Link_முனை_Matcher_Var final : பொது VNodeMatcher {
பொது:
    தருக்க முனை_Match(மாறிலி Ast_முனை* nodep) மாறிலி override {
        திரும்பு VN_IS(nodep, Var) || VN_IS(nodep, LambdaArgRef);
    }
};
வகுப்பு Link_முனை_Matcher_Var_IO final : பொது VNodeMatcher {
பொது:
    தருக்க முனை_Match(மாறிலி Ast_முனை* nodep) மாறிலி override {
        மாறிலி AstVar* மாறிலி varp = VN_CAST(nodep, Var);
        எனில் (!varp) திரும்பு பொய்;
        திரும்பு varp->isIO();
    }
};
வகுப்பு Link_முனை_Matcher_Var_Param final : பொது VNodeMatcher {
பொது:
    தருக்க முனை_Match(மாறிலி Ast_முனை* nodep) மாறிலி override {
        மாறிலி AstVar* மாறிலி varp = VN_CAST(nodep, Var);
        எனில் (!varp) திரும்பு பொய்;
        திரும்பு varp->isParam();
    }
};

//######################################################################
// LinkDot state, as a visitor of each AstNode

வகுப்பு LinkDotState final {
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
    மாறிலி VNUser1InUse m_inuser1;
    மாறிலி VNUser2InUse m_inuser2;
    மாறிலி VNUser4InUse m_inuser4;

பொது:
    // ENUMS
    // In order of priority, compute first ... compute last
    எண்ணிக்கை SAMNum : uint8_t { SAMN_MODPORT, SAMN_IFTOP, SAMN__MAX };  // Values for m_scopeAliasMap

தனிப்பட்ட:
    // TYPES
    using ScopeAliasMap = std::unordered_map<VSymEnt*, VSymEnt*>;
    using IfaceModSyms = std::vector<std::pair<AstIface*, VSymEnt*>>;

    நிலையான LinkDotState* s_errorThisp;  // Last self, for error reporting only

    // MEMBERS
    VSymGraph m_syms;  // Symbol table by hierarchy
    VSymGraph m_mods;  // Symbol table of all module names
    VSymEnt* m_dunitEntp = nullptr;  // $unit entry
    std::multimap<std::string, VSymEnt*>
        m_nameScopeSymMap;  // Map of scope referenced by non-pretty textual name
    std::set<std::pair<Ast_முனை_Module*, std::string>>
        m_implicitNameSet;  // For [module][signalname] if we can implicitly create it
    std::array<ScopeAliasMap, SAMN__MAX> m_scopeAliasMap;  // Map of <lhs,rhs> aliases
    std::vector<VSymEnt*> m_ifaceVarSyms;  // List of AstIfaceRefDType's to be imported
    IfaceModSyms m_ifaceModSyms;  // List of AstIface+Symbols to be processed
    மாறிலி VLinkDotStep m_step;  // Operational step

பொது:
    // METHODS

    வெறுமை dumpSelf(மாறிலி string& nameComment, தருக்க force = பொய்) {
        எனில் (debug() >= 6 || dumpLevel() >= 6 || force) {
            மாறிலி string filename = v3Global.debugFilename(nameComment) + ".txt";
            UINFO(4, "Dumping " << filename);
            மாறிலி std::unique_ptr<std::ofstream> logp{V3File::new_ofstream(filename)};
            எனில் (logp->fail()) v3fatal("Can't write file: " << filename);
            std::ostream& os = *logp;
            // TODO the symbol table has node pointers which may be
            // dangling, as we call deleteTree in these visitors without
            // pushing for later deletion (or deleting from the symbol table)
            // So, only completely safe to call this when under --debug or --debug-leak.
            m_syms.dumpSelf(os);
            தருக்க first = உண்மை;
            க்கு (முழு samn = 0; samn < SAMN__MAX; ++samn) {
                எனில் (!m_scopeAliasMap[samn].empty()) {
                    எனில் (first) os << "\nScopeAliasMap:\n";
                    first = பொய்;
                    க்கு (ScopeAliasMap::iterator it = m_scopeAliasMap[samn].begin();
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
    நிலையான வெறுமை preErrorDumpHandler() {
        எனில் (s_errorThisp) s_errorThisp->preErrorDump();
    }
    வெறுமை preErrorDump() {
        நிலையான தருக்க s_diddump = பொய்;
        எனில் (!s_diddump && dump_மரம்_Level()) {
            s_diddump = உண்மை;
            dumpSelf("linkdot-preerr", உண்மை);
            v3Global.rootp()->dump_மரம்_File(v3Global.debugFilename("linkdot-preerr.tree"));
        }
    }

    // CONSTRUCTORS
    LinkDotState(AstNetlist* rootp, VLinkDotStep step)
        : m_syms{rootp}
        , m_mods{rootp}
        , m_step{step} {
        UINFO(4, __FUNCTION__ << ": ");
        s_errorThisp = இது;
        V3Error::errorExitCb(preErrorDumpHandler);  // If get error, dump self
        readModNames();
    }
    ~LinkDotState() {
        V3Error::errorExitCb(nullptr);
        s_errorThisp = nullptr;
    }

    // ACCESSORS
    VSymGraph* symsp() { திரும்பு &m_syms; }
    முழு step_எண்() மாறிலி { திரும்பு static_cast<முழு>(m_step); }
    தருக்க forPrimary() மாறிலி { திரும்பு m_step == LDS_PRIMARY; }
    தருக்க forParamed() மாறிலி { திரும்பு m_step == LDS_PARAMED; }
    தருக்க forScopeCreation() மாறிலி { திரும்பு m_step == LDS_SCOPED; }

    // METHODS
    நிலையான string முனை_Text_Type(Ast_முனை* nodep) {
        எனில் (மாறிலி AstVar* மாறிலி varp = VN_CAST(nodep, Var)) {
            எனில் (varp->isIO() || varp->isIfaceRef()) {
                திரும்பு "port";
            } இல்லையெனில் எனில் (varp->isGParam()) {
                திரும்பு "parameter";
            } இல்லையெனில் எனில் (varp->varType() == VVarType::LPARAM) {
                திரும்பு "local parameter";
            } இல்லையெனில் {
                திரும்பு "variable";
            }
        } இல்லையெனில் எனில் (மாறிலி AstParamTypeDType* மாறிலி dtypep = VN_CAST(nodep, ParamTypeDType)) {
            எனில் (dtypep->isGParam()) {
                திரும்பு "type parameter";
            } இல்லையெனில் {
                திரும்பு "local type parameter";
            }
        } இல்லையெனில் எனில் (VN_IS(nodep, Begin)) {
            திரும்பு "block";
        } இல்லையெனில் எனில் (VN_IS(nodep, Cell)) {
            திரும்பு "instance";
        } இல்லையெனில் எனில் (VN_IS(nodep, Constraint)) {
            திரும்பு "constraint";
        } இல்லையெனில் எனில் (VN_IS(nodep, Func)) {
            திரும்பு "function";
        } இல்லையெனில் எனில் (VN_IS(nodep, GenBlock)) {
            திரும்பு "generate block";
        } இல்லையெனில் எனில் (VN_IS(nodep, Iface)) {
            திரும்பு "interface";
        } இல்லையெனில் எனில் (VN_IS(nodep, RSProd)) {
            திரும்பு "randsequence production";
        } இல்லையெனில் எனில் (VN_IS(nodep, Task)) {
            திரும்பு "task";
        } இல்லையெனில் {
            திரும்பு nodep->prettyTypeName();
        }
    }

    Ast_முனை_Module* findModuleSym(மாறிலி string& modName) {
        மாறிலி VSymEnt* மாறிலி foundp = m_mods.rootp()->findIdFallback(modName);
        திரும்பு foundp ? VN_AS(foundp->nodep(), முனை_Module) : nullptr;
    }
    வெறுமை readModNames() {
        // Look at all modules, and store pointers to all module names
        க்கு (Ast_முனை_Module *nextp, *nodep = v3Global.rootp()->modulesp(); nodep; nodep = nextp) {
            nextp = VN_AS(nodep->nextp(), முனை_Module);
            m_mods.rootp()->செருகு(nodep->name(), புதிய VSymEnt{&m_mods, nodep});
        }
    }

    VSymEnt* வேர்_Entp() மாறிலி { திரும்பு m_syms.rootp(); }
    VSymEnt* dunitEntp() மாறிலி { திரும்பு m_dunitEntp; }
    வெறுமை checkDuplicate(VSymEnt* lookupSymp, Ast_முனை* nodep, மாறிலி string& name) {
        // Lookup the given name under current symbol table
        // Insert if not found
        // Report error if there's a duplicate
        //
        // Note we only check for conflicts at the same level; it's ok if one block hides another
        // We also wouldn't want to not insert it even though it's lower down

        மாறிலி VSymEnt* மாறிலி foundp = lookupSymp->findIdFlat(name);
        Ast_முனை* மாறிலி fnodep = foundp ? foundp->nodep() : nullptr;
        எனில் (!fnodep) {
            // Not found, will add in a moment.
            எனில் (forPrimary() && VSymEnt::checkSimilarname(nodep)) {
                மாறிலி VSymEnt* மாறிலி alt = lookupSymp->findSimilarIdFlat(name);
                எனில் (alt) {
                    nodep->v3warn(SIMILARNAME, "Declaration overlaps another with different case: "
                                                   << nodep->prettyNameQ() << '\n'
                                                   << nodep->warnContextPrimary() << '\n'
                                                   << alt->nodep()->warnOther()
                                                   << "... Location of original declaration\n"
                                                   << alt->nodep()->warnContextSecondary());
                }
            }
        } இல்லையெனில் எனில் (nodep == fnodep) {  // Already inserted.
            // Good.
        } இல்லையெனில் எனில் (foundp->imported()) {  // From package
            // We don't throw VARHIDDEN as if the import is later the symbol
            // table's import wouldn't warn
        } இல்லையெனில் எனில் (forPrimary() && VN_IS(nodep, GenBlock)
                   && (VN_IS(fnodep, Begin) || VN_IS(fnodep, GenBlock))) {
            // Begin: ... blocks often replicate under genif/genfor, so
            // suppress duplicate checks.  See t_gen_forif.v for an example.
        } இல்லையெனில் {
            UINFO(4, "name " << name);  // Not always same as nodep->name
            UINFO(4, "Var1 " << nodep);
            UINFO(4, "Var2 " << fnodep);
            // Quote the node's own name as for messages, if it is the symbol's
            மாறிலி string nameQ
                = fnodep->name() == name ? fnodep->prettyNameQ() : Ast_முனை::prettyNameQ(name);
            எனில் (nodep->type() == fnodep->type()) {
                nodep->v3error("Duplicate declaration of "
                               << முனை_Text_Type(fnodep) << ": " << nameQ << '\n'
                               << nodep->warnContextPrimary() << '\n'
                               << fnodep->warnOther() << "... Location of original declaration\n"
                               << fnodep->warnContextSecondary());
                nodep->decl_Token_இலக்கம்_Set_Min(0);  // Disable checking forward typedefs
                fnodep->decl_Token_இலக்கம்_Set_Min(0);  // Disable checking forward typedefs
            } இல்லையெனில் {
                nodep->v3error("Unsupported in C: "
                               << ucfirst(முனை_Text_Type(nodep)) << " has the same name as "
                               << முனை_Text_Type(fnodep) << ": " << nameQ << '\n'
                               << nodep->warnContextPrimary() << '\n'
                               << fnodep->warnOther() << "... Location of original declaration\n"
                               << fnodep->warnContextSecondary());
            }
        }
    }
    வெறுமை செருகு_DUnit(AstNetlist* nodep) {
        // $unit on top scope
        VSymEnt* மாறிலி symp = புதிய VSymEnt{&m_syms, nodep};
        UINFO(9, "      INSERTdunit se" << cvtToHex(symp));
        symp->parentp(வேர்_Entp());  // Needed so backward search can find name of top module
        symp->fallbackp(nullptr);
        வேர்_Entp()->செருகு("$unit ", symp);  // Space so can never name conflict with user code
        //
        UASSERT_OBJ(!m_dunitEntp, nodep, "Call insertDUnit only once");
        m_dunitEntp = symp;
    }
    VSymEnt* செருகு_உச்சி_Cell(Ast_முனை_Module* nodep, மாறிலி string& scopename) {
        // Only called on the module at the very top of the hierarchy
        VSymEnt* மாறிலி symp = புதிய VSymEnt{&m_syms, nodep};
        UINFO(9, "      INSERTtop se" << cvtToHex(symp) << "  " << scopename << " " << nodep);
        symp->parentp(வேர்_Entp());  // Needed so backward search can find name of top module
        symp->fallbackp(dunitEntp());  // Needed so can find $unit stuff
        nodep->user1p(symp);
        checkDuplicate(வேர்_Entp(), nodep, nodep->origName());
        வேர்_Entp()->செருகு(nodep->origName(), symp);
        எனில் (forScopeCreation()) m_nameScopeSymMap.emplace(scopename, symp);
        திரும்பு symp;
    }
    VSymEnt* செருகு_உச்சி_Iface(AstCell* nodep, மாறிலி string& scopename) {
        VSymEnt* மாறிலி symp = புதிய VSymEnt{&m_syms, nodep};
        UINFO(9, "      INSERTtopiface se" << cvtToHex(symp) << "  " << scopename << " " << nodep);
        symp->parentp(வேர்_Entp());  // Needed so backward search can find name of top module
        symp->fallbackp(dunitEntp());  // Needed so can find $unit stuff
        nodep->user1p(symp);
        எனில் (nodep->modp()) nodep->modp()->user1p(symp);
        checkDuplicate(வேர்_Entp(), nodep, nodep->origName());
        வேர்_Entp()->செருகு(nodep->origName(), symp);
        எனில் (forScopeCreation()) m_nameScopeSymMap.emplace(scopename, symp);
        திரும்பு symp;
    }
    VSymEnt* செருகு_Cell(VSymEnt* abovep, VSymEnt* modSymp, AstCell* nodep,
                        மாறிலி string& scopename) {
        UASSERT_OBJ(abovep, nodep, "Null symbol table inserting node");
        VSymEnt* மாறிலி symp = புதிய VSymEnt{&m_syms, nodep};
        UINFO(9, "      INSERTcel se" << cvtToHex(symp) << "  " << scopename << " above=se"
                                      << cvtToHex(abovep) << " mods=se" << cvtToHex(modSymp)
                                      << " node=" << nodep);
        symp->parentp(abovep);
        symp->fallbackp(dunitEntp());  // Needed so can find $unit stuff
        nodep->user1p(symp);
        எனில் (nodep->modp()) nodep->modp()->user1p(symp);
        checkDuplicate(abovep, nodep, nodep->origName());
        abovep->reinsert(nodep->origName(), symp);
        எனில் (forScopeCreation() && abovep != modSymp && !modSymp->findIdFlat(nodep->name())) {
            // If it's foo_DOT_bar, we need to be able to find it under "foo_DOT_bar" too.
            // Duplicates are possible, as until resolve generates might
            // have 2 same cells under an if
            modSymp->reinsert(nodep->name(), symp);
        }
        எனில் (forScopeCreation()) m_nameScopeSymMap.emplace(scopename, symp);
        திரும்பு symp;
    }
    வெறுமை செருகு_Map(VSymEnt* symp, மாறிலி string& scopename) {
        எனில் (forScopeCreation()) m_nameScopeSymMap.emplace(scopename, symp);
    }

    VSymEnt* செருகு_Inline(VSymEnt* abovep, VSymEnt* modSymp, AstCellInline* nodep,
                          மாறிலி string& basename) {
        // A fake point in the hierarchy, corresponding to a named block flattened by V3Begin.
        // Names are looked up in the module with the block's flattened prefix.
        UASSERT_OBJ(abovep, nodep, "Null symbol table inserting node");
        VSymEnt* மாறிலி symp = புதிய VSymEnt{&m_syms, nodep};
        UINFO(9, "      INSERTinl se" << cvtToHex(symp) << "  " << basename << " above=se"
                                      << cvtToHex(abovep) << " mods=se" << cvtToHex(modSymp)
                                      << " node=" << nodep);
        symp->parentp(abovep);
        symp->fallbackp(modSymp);
        symp->symPrefix(nodep->name() + "__DOT__");
        nodep->user1p(symp);
        checkDuplicate(abovep, nodep, nodep->name());
        abovep->reinsert(basename, symp);
        எனில் (abovep != modSymp && !modSymp->findIdFlat(nodep->name())) {
            // If it's foo_DOT_bar, we need to be able to find it under that too.
            modSymp->reinsert(nodep->name(), symp);
        }
        திரும்பு symp;
    }
    VSymEnt* செருகு_Block(VSymEnt* abovep, மாறிலி string& name, Ast_முனை* nodep,
                         Ast_முனை_Module* classOrPackagep) {
        // A fake point in the hierarchy, corresponding to a begin or function/task block
        // After we remove begins these will go away
        // Note we fallback to the symbol table of the parent, as we want to find variables there
        // However, cells walk the graph, so cells will appear under the begin/ftask itself
        UASSERT_OBJ(abovep, nodep, "Null symbol table inserting node");
        VSymEnt* மாறிலி symp = புதிய VSymEnt{&m_syms, nodep};
        UINFO(9, "      INSERTblk se" << cvtToHex(symp) << "  above=se" << cvtToHex(abovep)
                                      << " pkg=" << cvtToHex(classOrPackagep)
                                      << "  node=" << nodep);
        symp->parentp(abovep);
        symp->classOrPackagep(classOrPackagep);
        symp->fallbackp(abovep);
        nodep->user1p(symp);
        எனில் (name != "") checkDuplicate(abovep, nodep, name);
        // Duplicates are possible, as until resolve generates might have 2 same cells under an if
        abovep->reinsert(name, symp);
        திரும்பு symp;
    }
    VSymEnt* செருகு_Sym(VSymEnt* abovep, மாறிலி string& name, Ast_முனை* nodep,
                       Ast_முனை_Module* classOrPackagep) {
        UASSERT_OBJ(abovep, nodep, "Null symbol table inserting node");
        VSymEnt* மாறிலி symp = புதிய VSymEnt{&m_syms, nodep};
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
        திரும்பு symp;
    }
    நிலையான தருக்க exists_முனை_Sym(Ast_முனை* nodep) { திரும்பு nodep->user1p() != nullptr; }
    நிலையான VSymEnt* get_முனை_Sym(Ast_முனை* nodep) {
        // Don't use this in ResolveVisitor, as we need to pick up the proper
        // reference under each SCOPE
        VSymEnt* மாறிலி symp = nodep->user1u().toSymEnt();
        UASSERT_OBJ(symp, nodep, "Module/etc never assigned a symbol entry?");
        திரும்பு symp;
    }
    VSymEnt* getScopeSym(AstScope* nodep) {
        மாறிலி auto it = m_nameScopeSymMap.find(nodep->name());
        UASSERT_OBJ(it != m_nameScopeSymMap.end(), nodep,
                    "Scope never assigned a symbol entry '" << nodep->name() << "'");
        திரும்பு it->second;
    }
    வெறுமை implicitOkAdd(Ast_முனை_Module* nodep, மாறிலி string& varname) {
        // Mark the given variable name as being allowed to be implicitly declared
        எனில் (nodep) {
            மாறிலி auto it = m_implicitNameSet.find(std::make_pair(nodep, varname));
            எனில் (it == m_implicitNameSet.end()) m_implicitNameSet.emplace(nodep, varname);
        }
    }
    தருக்க implicitOk(Ast_முனை_Module* nodep, மாறிலி string& varname) {
        திரும்பு nodep
               && (m_implicitNameSet.find(std::make_pair(nodep, varname))
                   != m_implicitNameSet.end());
    }

    // Track and later recurse interface modules
    வெறுமை செருகு_Iface_Mod_Sym(AstIface* nodep, VSymEnt* symp) {
        m_ifaceModSyms.emplace_back(nodep, symp);
    }
    வெறுமை computeIfaceModSyms();

    // Track and later insert interface references
    வெறுமை செருகு_Iface_Var_Sym(VSymEnt* symp) {  // Where sym is for a VAR of dtype IFACEREFDTYPE
        m_ifaceVarSyms.தள்ளு_back(symp);
    }
    // Iface for a raw or arrayed iface; peels nested array layers for multi-dim arrays.
    நிலையான AstIfaceRefDType* ifaceRefFromArray(Ast_முனை_DType* nodep) {
        போது (nodep) {
            எனில் (AstIfaceRefDType* மாறிலி ifp = VN_CAST(nodep, IfaceRefDType)) திரும்பு ifp;
            எனில் (மாறிலி AstBracketArrayDType* மாறிலி arrp = VN_CAST(nodep, BracketArrayDType)) {
                nodep = arrp->subDTypep();
            } இல்லையெனில் எனில் (மாறிலி AstUnpackArrayDType* மாறிலி arrp = VN_CAST(nodep, UnpackArrayDType)) {
                nodep = arrp->subDTypep();
            } இல்லையெனில் {
                திரும்பு nullptr;
            }
        }
        திரும்பு nullptr;
    }
    // Given a pin expression, resolve it to a live AstIface* (or nullptr).
    // Handles both simple VarRef and dotted VarXRef pin connections.
    AstIface* live_Iface_From_Pin_கோவை(Ast_முனை* exprp, VSymEnt* parentSymp) {
        AstVar* resolvedVarp = nullptr;
        எனில் (AstVarRef* மாறிலி refp = VN_CAST(exprp, VarRef)) {
            resolvedVarp = refp->varp();
        } இல்லையெனில் எனில் (AstVarXRef* மாறிலி xrefp = VN_CAST(exprp, VarXRef)) {
            VSymEnt* மாறிலி lookupSymp = parentSymp ? parentSymp->parentp() : nullptr;
            எனில் (lookupSymp) {
                string baddot;
                VSymEnt* okSymp = nullptr;
                மாறிலி string dotpath = xrefp->dotted() + "." + xrefp->name();
                VSymEnt* மாறிலி dotSymp
                    = findDotted(xrefp->fileline(), lookupSymp, dotpath, baddot, okSymp, உண்மை);
                எனில் (dotSymp) resolvedVarp = VN_CAST(dotSymp->nodep(), Var);
            }
        }
        AstIface* resultp = nullptr;
        எனில் (resolvedVarp) {
            AstIfaceRefDType* மாறிலி irefp = ifaceRefFromArray(resolvedVarp->subDTypep());
            எனில் (irefp && irefp->ifaceViaCellp() && !irefp->ifaceViaCellp()->dead()) {
                resultp = irefp->ifaceViaCellp();
            }
        }
        திரும்பு resultp;
    }

    // Attempt to repair a port's AstIfaceRefDType by tracing through the
    // parent cell's pin connection to find the correct live interface.
    // Returns true if repaired or already correct; false if unresolvable.
    தருக்க repairIfaceRef(VSymEnt* varSymp, AstVar* varp, AstIfaceRefDType* ifacerefp, தருக்க isDead) {
        // Walk up symbol-table parents to find the enclosing AstCell
        VSymEnt* parentSymp = varSymp->parentp();
        AstCell* parentCellp = nullptr;
        க்கு (முழு depth = 0; parentSymp && depth < 20; ++depth) {
            எனில் (AstCell* மாறிலி cp = VN_CAST(parentSymp->nodep(), Cell)) {
                parentCellp = cp;
                நிறுத்து;
            }
            parentSymp = parentSymp->parentp();
        }
        எனில் (!parentCellp) திரும்பு பொய்;
        // Scan pins to find the one connected to this port variable
        க்கு (AstPin* pinp = parentCellp->pinsp(); pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            எனில் (pinp->modVarp() != varp && pinp->name() != varp->name()) தொடர்;
            Ast_முனை* மாறிலி exprp = pinp->exprp();
            எனில் (!exprp) திரும்பு பொய்;
            AstIface* மாறிலி newIfacep = live_Iface_From_Pin_கோவை(exprp, parentSymp);
            எனில் (newIfacep && newIfacep != ifacerefp->ifaceViaCellp()) {
                UINFO(4, "  REPAIR-IFACE-REF var=" << varp->prettyNameQ()
                                                   << " old=" << ifacerefp->ifacep()->prettyNameQ()
                                                   << " new=" << newIfacep->prettyNameQ());
                ifacerefp->ifacep(newIfacep);
                திரும்பு உண்மை;
            }
            எனில் (newIfacep) திரும்பு !isDead;  // Same interface - OK if live
            திரும்பு பொய்;
        }
        திரும்பு பொய்;
    }

    வெறுமை computeIfaceVarSyms() {
        க்கு (VSymEnt* varSymp : m_ifaceVarSyms) {
            AstVar* மாறிலி varp = varSymp ? VN_AS(varSymp->nodep(), Var) : nullptr;
            UINFO(9, "  insAllIface se" << cvtToHex(varSymp) << " " << varp);
            AstIfaceRefDType* மாறிலி ifacerefp = ifaceRefFromArray(varp->subDTypep());
            UASSERT_OBJ(ifacerefp, varp, "Non-ifacerefs on list!");
            எனில் (!ifacerefp->cellp() && !ifacerefp->isVirtual() && (varp->pin_இலக்கம்() == 0)) {
                varp->v3error("Interface port declaration "
                              << varp->prettyNameQ() << " doesn't have corresponding port\n"
                              << varp->warnMore()
                                     + "... Perhaps intended an interface instantiation but "
                                       "are missing parenthesis (IEEE 1800-2023 25.3)?");
            }
            எனில் (!ifacerefp->ifaceViaCellp()) {
                எனில் (!ifacerefp->cellp()) {  // Probably a NotFoundModule, or a normal module if
                                            // made mistake
                    UINFO(1, "Associated cell " << Ast_முனை::prettyNameQ(ifacerefp->cellName()));
                    ifacerefp->v3error("Cannot find file containing interface: "
                                       << Ast_முனை::prettyNameQ(ifacerefp->ifaceName()));
                    தொடர்;
                } இல்லையெனில் {
                    ifacerefp->v3fatalSrc("Unlinked interface");
                }
            } இல்லையெனில் எனில் (varp->isIfaceRef() && !ifacerefp->cellp() && ifacerefp->ifaceViaCellp()) {
                // Port variable (cellp=null) pointing to an interface.
                // The interface may be dead (template replaced by clone) or
                // live-but-wrong (template still alive because some instances
                // use default params, but this port should point to a
                // specialized clone based on the pin connection).
                மாறிலி தருக்க isDead = ifacerefp->ifaceViaCellp()->dead();
                எனில் (isDead && forPrimary() && !varp->isIfaceParent()
                    && !v3Global.opt.உச்சி_Ifaces_Supported()) {
                    varp->v3warn(E_UNSUPPORTED,
                                 "Unsupported: Interfaced port on top level module");
                }
                // Attempt repair: trace through parent cell's pin to find
                // the correct live interface for this port variable
                மாறிலி தருக்க repaired = repairIfaceRef(varSymp, varp, ifacerefp, isDead);
                எனில் (!repaired && isDead) {
                    ifacerefp->v3error(
                        "Interface " << Ast_முனை::prettyNameQ(ifacerefp->ifaceName())
                                     << " not connected as parent's interface not connected\n"
                                     << ifacerefp->warnMore()
                                     << "... Perhaps caused by another error on the parent "
                                        "interface that needs resolving\n"
                                     << ifacerefp->warnMore()
                                     << "... Or, perhaps intended an interface instantiation but "
                                        "are missing parenthesis (IEEE 1800-2023 25.3)?");
                    தொடர்;
                }
            } இல்லையெனில் எனில் (ifacerefp->ifaceViaCellp()->dead()
                       || !exists_முனை_Sym(ifacerefp->ifaceViaCellp())) {
                ifacerefp->ifaceViaCellp()->v3fatalSrc(
                    "Interface referenced by virtual iface is dead. "
                    "Dead interfaces have required linking steps skipped");
            }
            VSymEnt* மாறிலி ifaceSymp = get_முனை_Sym(ifacerefp->ifaceViaCellp());
            VSymEnt* ifOrPortSymp = ifaceSymp;
            // Link Modport names to the Modport Node under the Interface
            எனில் (ifacerefp->isModport()) {
                VSymEnt* மாறிலி foundp = ifaceSymp->findIdFallback(ifacerefp->modportName());
                தருக்க ok = பொய்;
                எனில் (foundp) {
                    எனில் (AstModport* மாறிலி modportp = VN_CAST(foundp->nodep(), Modport)) {
                        UINFO(4, "Link Modport: " << modportp);
                        ifacerefp->modportp(modportp);
                        ifOrPortSymp = foundp;
                        ok = உண்மை;
                    }
                }
                எனில் (!ok) {
                    மாறிலி string suggest = suggestSymFallback(ifaceSymp, ifacerefp->modportName(),
                                                              Link_முனை_Matcher_Modport{});
                    ifacerefp->modportFileline()->v3error(
                        "Modport not found under interface "
                        << ifacerefp->prettyNameQ(ifacerefp->ifaceName()) << ": "
                        << ifacerefp->prettyNameQ(ifacerefp->modportName()) << '\n'
                        << (suggest.empty() ? "" : ifacerefp->warnMore() + suggest));
                }
            }
            // Alias won't expand until interfaces and modport names are known; see notes at top
            செருகு_Scope_Alias(SAMN_IFTOP, varSymp, ifOrPortSymp);
        }
        m_ifaceVarSyms.clear();
    }

    வெறுமை செருகு_Scope_Alias(SAMNum samn, VSymEnt* lhsp, VSymEnt* rhsp) {
        // Track and later insert scope aliases; an interface referenced by
        // a child cell connecting to that interface
        // Typically lhsp=VAR w/dtype IFACEREF, rhsp=IFACE cell
        UINFO(9, "   insertScopeAlias se" << cvtToHex(lhsp) << " se" << cvtToHex(rhsp));
        UASSERT_OBJ(
            !(VN_IS(rhsp->nodep(), Cell) && !VN_IS(VN_AS(rhsp->nodep(), Cell)->modp(), Iface)),
            rhsp->nodep(), "Got a non-IFACE alias RHS");
        m_scopeAliasMap[samn].emplace(lhsp, rhsp);
    }
    வெறுமை computeScopeAliases() {
        UINFO(9, "computeIfaceAliases");
        க்கு (முழு samn = 0; samn < SAMN__MAX; ++samn) {
            க்கு (ScopeAliasMap::iterator it = m_scopeAliasMap[samn].begin();
                 it != m_scopeAliasMap[samn].end(); ++it) {
                VSymEnt* மாறிலி lhsp = it->first;
                VSymEnt* srcp = lhsp;
                போது (உண்மை) {  // Follow chain of aliases up to highest level non-alias
                    மாறிலி auto it2 = m_scopeAliasMap[samn].find(srcp);
                    எனில் (it2 != m_scopeAliasMap[samn].end()) {
                        srcp = it2->second;
                        தொடர்;
                    } இல்லையெனில் {
                        நிறுத்து;
                    }
                }
                UINFO(9, "  iiasa: Insert alias se" << lhsp << " (" << lhsp->nodep()->typeName()
                                                    << ") <- se" << srcp << " " << srcp->nodep());
                // srcp should be an interface reference pointing to the interface we want to
                // import
                lhsp->importFromIface(symsp(), srcp);
                // Allow access to objects not permissible to be listed in a modport
                எனில் (VN_IS(srcp->nodep(), Modport)) {
                    lhsp->importFromIface(symsp(), srcp->parentp(), உண்மை);
                }
            }
            // m_scopeAliasMap[samn].clear();  // Done with it, but put into debug file
        }
    }

தனிப்பட்ட:
    VSymEnt* findWithAltFallback(VSymEnt* symp, மாறிலி string& name, மாறிலி string& altname) {
        VSymEnt* findp = symp->findIdFallback(name);
        எனில் (findp) திரும்பு findp;
        எனில் (altname != "") {
            UINFO(8, "     alt fallback");
            findp = symp->findIdFallback(altname);
        }
        திரும்பு findp;
    }

    VSymEnt* findWithAltFlat(VSymEnt* symp, மாறிலி string& name, மாறிலி string& altname) {
        VSymEnt* findp = symp->findIdFlat(name);
        எனில் (findp) திரும்பு findp;
        எனில் (altname != "") {
            UINFO(8, "     alt flat");
            findp = symp->findIdFlat(altname);
        }
        திரும்பு findp;
    }

பொது:
    VSymEnt* findDotted(File_வரி* refLocationp, VSymEnt* lookupSymp, மாறிலி string& dotname,
                        string& baddot, VSymEnt*& okSymp, தருக்க firstId) {
        // Given a dotted hierarchy name, return where in scope it is
        // Note when dotname=="" we just fall through and return lookupSymp
        UINFO(8, "    dottedFind se" << cvtToHex(lookupSymp) << " '" << dotname << "'");
        string leftname = dotname;
        okSymp = lookupSymp;  // So can list bad scopes
        போது (leftname != "") {  // foreach dotted part of xref name
            string::size_type pos = leftname.find('.');
            string ident;
            எனில் (pos != string::npos) {
                ident = leftname.substr(0, pos);
                leftname = leftname.substr(pos + 1);
            } இல்லையெனில் {
                ident = leftname;
                leftname = "";
            }
            baddot = ident;  // So user can see where they botched it
            okSymp = lookupSymp;
            string altIdent;
            எனில் (!forScopeCreation()) {
                // GENFOR Begin is foo__BRA__##__KET__ after we've genloop unrolled,
                // but presently should be just "foo".
                // Likewise cell foo__[array] before we've expanded arrays is just foo.
                // Multi-dim iface arrays append multiple __BRA__..__KET__ suffixes; strip them
                // all.
                altIdent = ident;
                போது (VString::endsWith(altIdent, "__KET__")) {
                    மாறிலி auto braPos = altIdent.rfind("__BRA__");
                    எனில் (braPos == string::npos) நிறுத்து;
                    altIdent = altIdent.substr(0, braPos);
                }
                எனில் (altIdent == ident) altIdent.clear();
            }
            UINFO(8, "         id " << ident << " alt " << altIdent << " left " << leftname
                                    << " at se" << lookupSymp);
            // Spec says; Look at existing module (cellnames then modname),
            // then look up (inst name or modname)
            எனில் (firstId) {
                // Check this module - subcellnames
                மாறிலி AstCell* cellp = lookupSymp ? VN_CAST(lookupSymp->nodep(), Cell)
                                                  : nullptr;  // Replicated below
                எனில் (VSymEnt* மாறிலி findSymp = findWithAltFallback(lookupSymp, ident, altIdent)) {
                    lookupSymp = findSymp;
                }

                // Check this module - cur modname
                இல்லையெனில் எனில் (cellp && cellp->modp()->origName() == ident) {
                }
                // $root we walk up to Netlist
                இல்லையெனில் எனில் (ident == "$root") {
                    lookupSymp = வேர்_Entp();
                    // We've added the '$root' module, now everything else is one lower
                    எனில் (forScopeCreation()) {
                        lookupSymp = lookupSymp->findIdFlat(ident);
                        UASSERT(lookupSymp, "Cannot find $root module under netlist");
                    }
                }
                // Move up and check cellname + modname
                இல்லையெனில் {
                    தருக்க crossedCell = பொய்;  // Crossed a cell boundary
                    போது (lookupSymp) {
                        lookupSymp = lookupSymp->parentp();
                        cellp = lookupSymp ? VN_CAST(lookupSymp->nodep(), Cell)
                                           : nullptr;  // Replicated above
                        எனில் (lookupSymp) {
                            UINFO(9, "    Up to " << lookupSymp);
                            எனில் (cellp || VN_IS(lookupSymp->nodep(), CellInline)) {
                                crossedCell = உண்மை;
                            }
                            எனில் (cellp && cellp->modp()->origName() == ident) {
                                நிறுத்து;
                            } இல்லையெனில் எனில் (VSymEnt* மாறிலி findSym2p
                                       = findWithAltFallback(lookupSymp, ident, altIdent)) {
                                lookupSymp = findSym2p;
                                எனில் (crossedCell && VN_IS(lookupSymp->nodep(), Var)) {
                                    UINFO(9, "    Not found but matches var name in parent "
                                                 << lookupSymp);
                                    திரும்பு nullptr;
                                }
                                நிறுத்து;
                            }
                        } இல்லையெனில் {
                            நிறுத்து;
                        }
                    }
                    எனில் (!lookupSymp) திரும்பு nullptr;  // Not found
                }
            } இல்லையெனில் {  // Searching for middle path component, must be a cell or interface port
                VSymEnt* findSymp = findWithAltFlat(lookupSymp, ident, altIdent);
                எனில் (findSymp) {
                    lookupSymp = findSymp;
                } இல்லையெனில் {
                    // Try prefixed lookup for interface ports accessed through hierarchy
                    // (e.g., slave_inst.bus.data where bus is an interface port)
                    // After inlining, interface ports have prefixed names like
                    // top__DOT__slave_inst__DOT__bus
                    UINFO(8, "     middle-path: trying prefixed lookup for '" << ident << "'\n");
                    string baddot;
                    findSymp = findSymPrefixed(lookupSymp, ident, baddot, /*fallback=*/உண்மை);
                    UINFO(8, "     middle-path: prefixed lookup result: "
                                 << (findSymp ? "found" : "not found") << "\n");
                    எனில் (findSymp) {
                        // Check if this is an interface reference variable
                        மாறிலி AstVar* varp = VN_CAST(findSymp->nodep(), Var);
                        எனில் (!varp) {
                            எனில் (மாறிலி AstVarScope* vscp = VN_CAST(findSymp->nodep(), VarScope)) {
                                varp = vscp->varp();
                            }
                        }
                        எனில் (varp && varp->isIfaceRef()) {
                            // Found an interface port - use findSymp directly
                            // computeScopeAliases has already imported interface members
                            // into this symbol entry, so we use it instead of redirecting
                            // to the shared modport definition (which would cause all
                            // instances to resolve to the same symbols - bug #2656)
                            lookupSymp = findSymp;
                        } இல்லையெனில் {
                            // Non-interface symbol in middle path must be a cell (module instance)
                            // to continue hierarchical resolution
                            எனில் (VN_IS(findSymp->nodep(), Cell)) {
                                lookupSymp = findSymp;
                            } இல்லையெனில் {
                                // Reject non-cell, non-interface symbols found via fallback
                                // to prevent accidental resolution to unrelated symbols
                                UINFO(8, "     middle-path: rejecting non-cell symbol\n");
                                திரும்பு nullptr;
                            }
                        }
                    } இல்லையெனில் {
                        திரும்பு nullptr;  // Not found
                    }
                }
            }
            எனில் (lookupSymp) {
                எனில் (மாறிலி AstCell* மாறிலி cellp = VN_CAST(lookupSymp->nodep(), Cell)) {
                    எனில் (மாறிலி Ast_முனை_Module* மாறிலி modp = cellp->modp()) {
                        எனில் (modp->hierBlock() && !leftname.empty()) {
                            refLocationp->v3error("Cannot access scope inside hierarchical block");
                        } இல்லையெனில் எனில் (VN_IS(modp, NotFoundModule)) {
                            refLocationp->v3error("Dotted reference to instance that refers to "
                                                  "missing module/interface: "
                                                  << modp->prettyNameQ());
                        }
                    }
                }
                // Follow scope alias for nested interface port access
                எனில் (!leftname.empty()) {
                    மாறிலி auto aliasIt = m_scopeAliasMap[SAMN_IFTOP].find(lookupSymp);
                    எனில் (aliasIt != m_scopeAliasMap[SAMN_IFTOP].end()) {
                        lookupSymp = aliasIt->second;
                        // Alias may point to __Viftop VarScope; find corresponding Cell
                        எனில் (மாறிலி AstVarScope* மாறிலி vscp
                            = VN_CAST(lookupSymp->nodep(), VarScope)) {
                            மாறிலி string varName = vscp->varp()->name();
                            நிலையான constexpr மாறிலி எழுத்து* மாறிலி VIFTOP_SUFFIX = "__Viftop";
                            எனில் (VString::endsWith(varName, VIFTOP_SUFFIX)) {
                                மாறிலி string cellName
                                    = varName.substr(0, varName.size() - strlen(VIFTOP_SUFFIX));
                                VSymEnt* மாறிலி parentSymp = lookupSymp->parentp();
                                எனில் (parentSymp) {
                                    VSymEnt* மாறிலி cellSymp = parentSymp->findIdFlat(cellName);
                                    எனில் (cellSymp && VN_IS(cellSymp->nodep(), Cell)) {
                                        lookupSymp = cellSymp;  // Use Cell for member lookup
                                    }
                                }
                            }
                        }
                    } இல்லையெனில் {
                        // No alias; try following IfaceRefDType to interface cell
                        மாறிலி AstVar* varp = VN_CAST(lookupSymp->nodep(), Var);
                        எனில் (!varp) {
                            எனில் (மாறிலி AstVarScope* மாறிலி vscp
                                = VN_CAST(lookupSymp->nodep(), VarScope)) {
                                varp = vscp->varp();
                            }
                        }
                        எனில் (varp && varp->isIfaceRef()) {
                            எனில் (மாறிலி AstIfaceRefDType* மாறிலி ifaceRefp
                                = ifaceRefFromArray(varp->dtypep())) {
                                எனில் (ifaceRefp->cellp() && exists_முனை_Sym(ifaceRefp->cellp())) {
                                    lookupSymp = get_முனை_Sym(ifaceRefp->cellp());
                                } இல்லையெனில் எனில் (ifaceRefp->ifaceViaCellp()
                                           && exists_முனை_Sym(ifaceRefp->ifaceViaCellp())) {
                                    lookupSymp = get_முனை_Sym(ifaceRefp->ifaceViaCellp());
                                } இல்லையெனில் எனில் (ifaceRefp->ifacep()
                                           && exists_முனை_Sym(ifaceRefp->ifacep())) {
                                    lookupSymp = get_முனை_Sym(ifaceRefp->ifacep());
                                }
                            }
                        }
                    }
                }
            }
            firstId = பொய்;
        }
        திரும்பு lookupSymp;
    }

    நிலையான string removeLastInlineScope(மாறிலி string& name) {
        string out = name;
        மாறிலி string dot = "__DOT__";
        மாறிலி size_t dotPos = out.rfind(dot, out.size() - dot.length() - 1);
        எனில் (dotPos == string::npos) {
            திரும்பு "";
        } இல்லையெனில் {
            திரும்பு out.erase(dotPos + dot.length(), string::npos);
        }
    }

    VSymEnt* findSymPrefixed(VSymEnt* lookupSymp, மாறிலி string& dotname, string& baddot,
                             தருக்க fallback) {
        // Find symbol in given point in hierarchy, allowing prefix (post-Inline)
        // For simplicity lookupSymp may be passed nullptr result from findDotted
        எனில் (!lookupSymp) திரும்பு nullptr;
        UINFO(8, "    findSymPrefixed "
                     << dotname << " under se" << cvtToHex(lookupSymp)
                     << ((lookupSymp->symPrefix() == "") ? "" : " as ")
                     << ((lookupSymp->symPrefix() == "") ? "" : lookupSymp->symPrefix() + dotname)
                     << "  at se" << lookupSymp << " fallback=" << fallback);
        string prefix = lookupSymp->symPrefix();
        VSymEnt* foundp = nullptr;
        போது (!foundp) {
            எனில் (fallback) {
                foundp = lookupSymp->findIdFallback(prefix + dotname);  // Might be nullptr
            } இல்லையெனில் {
                foundp = lookupSymp->findIdFlat(prefix + dotname);  // Might be nullptr
            }
            எனில் (prefix.empty()) நிறுத்து;
            மாறிலி string nextPrefix = removeLastInlineScope(prefix);
            எனில் (prefix == nextPrefix) நிறுத்து;
            prefix = nextPrefix;
        }
        எனில் (!foundp) baddot = dotname;
        திரும்பு foundp;
    }
    நிலையான தருக்க checkIfClassOrPackage(மாறிலி VSymEnt* மாறிலி symp) {
        எனில் (VN_IS(symp->nodep(), Class) || VN_IS(symp->nodep(), Package)) திரும்பு உண்மை;

        // Helper: check if a RefDType might resolve to a class later.
        மாறிலி auto isPotentialClassRef = [](மாறிலி AstRefDType* refp) -> தருக்க {
            திரும்பு refp && !refp->typeofp()
                   && (!refp->classOrPackageOpp()
                       || VN_IS(refp->classOrPackageOpp(), ClassOrPackageRef));
        };

        // Helper: allow types that can represent a class/package handle or an unresolved ref.
        // BasicDType is allowed here only for null/object handles.
        மாறிலி auto is_Valid_Type_முனை = [](மாறிலி Ast_முனை* nodep) -> தருக்க {
            திரும்பு VN_IS(nodep, VoidDType) || VN_IS(nodep, BasicDType)
                   || VN_IS(nodep, ClassRefDType) || VN_IS(nodep, ParseRef);
        };

        எனில் (மாறிலி AstTypedef* மாறிலி typedefp = VN_CAST(symp->nodep(), Typedef)) {
            மாறிலி Ast_முனை_DType* dtypep = typedefp->subDTypep();
            எனில் (!dtypep) dtypep = typedefp->childDTypep();
            எனில் (VN_IS(dtypep, ClassRefDType)) திரும்பு உண்மை;
            எனில் (isPotentialClassRef(VN_CAST(dtypep, RefDType))) திரும்பு உண்மை;
        } இல்லையெனில் எனில் (மாறிலி AstParamTypeDType* மாறிலி paramTypep
                   = VN_CAST(symp->nodep(), ParamTypeDType)) {
            // Before V3Param the declared default is in childDTypep (possibly
            // wrapped in a RequireDType); after V3Param it is consumed and the
            // bound type is the resolved data type, e.g. a type parameter
            // inherited from a specialized base class (REQ #(Item) -> class Item).
            Ast_முனை* childp = paramTypep->childDTypep();
            எனில் (மாறிலி AstRequireDType* மாறிலி reqp = VN_CAST(childp, RequireDType)) {
                childp = reqp->lhsp();
            }
            மாறிலி Ast_முனை* மாறிலி checkp = childp ? childp : paramTypep->skipRefp();
            எனில் (is_Valid_Type_முனை(checkp) || isPotentialClassRef(VN_CAST(checkp, RefDType))) {
                திரும்பு உண்மை;
            }
        }
        திரும்பு பொய்;
    }
    VSymEnt* resolveClassOrPackage(VSymEnt* lookSymp, AstClassOrPackageRef* nodep, தருக்க fallback,
                                   தருக்க classOnly, மாறிலி string& forWhat,
                                   தருக்க deferIfUnresolved = பொய்) {
        எனில் (nodep->classOrPackageSkipp()) திரும்பு get_முனை_Sym(nodep->classOrPackageSkipp());
        VSymEnt* foundp;
        VSymEnt* தேடல்_Symp = lookSymp;
        எனில் (VL_UNCOVERABLE(தேடல்_Symp && VN_IS(தேடல்_Symp->nodep(), ParamTypeDType))) {
            தேடல்_Symp->nodep()->v3fatalSrc(  // LCOV_EXCL_LINE
                "resolveClassOrPackage: unexpected ParamTypeDType lookup");
        }
        எனில் (fallback) {
            VSymEnt* currentLookSymp = தேடல்_Symp;
            செய் {
                foundp = currentLookSymp->findIdFlat(nodep->name());
                எனில் (foundp && !checkIfClassOrPackage(foundp)) foundp = nullptr;
                எனில் (!foundp) currentLookSymp = currentLookSymp->fallbackp();
            } போது (!foundp && currentLookSymp);
        } இல்லையெனில் {
            foundp = தேடல்_Symp->findIdFlat(nodep->name());
            எனில் (foundp && !checkIfClassOrPackage(foundp)) foundp = nullptr;
        }
        எனில் (!foundp && v3Global.rootp()->stdPackagep()) {  // Look under implied std::
            foundp = get_முனை_Sym(v3Global.rootp()->stdPackagep())->findIdFlat(nodep->name());
        }
        எனில் (foundp) {
            nodep->classOrPackageNodep(foundp->nodep());
            திரும்பு foundp;
        }
        எனில் (deferIfUnresolved) திரும்பு nullptr;
        மாறிலி string suggest
            = suggestSymFallback(lookSymp, nodep->name(), Link_முனை_Matcher_Class_Or_Package{});
        nodep->v3error((classOnly ? "Class" : "Package/class")
                       << " for '" << forWhat  // extends/implements
                       << "' not found: " << nodep->prettyNameQ() << '\n'
                       << (suggest.empty() ? "" : nodep->warnMore() + suggest));
        திரும்பு nullptr;
    }
    string suggestSymFallback(VSymEnt* lookupSymp, மாறிலி string& name,
                              மாறிலி VNodeMatcher& matcher) {
        // Suggest alternative symbol in given point in hierarchy
        // Does not support inline, as we find user-level errors before inlining
        // For simplicity lookupSymp may be passed nullptr result from findDotted
        எனில் (!lookupSymp) திரும்பு "";
        VSpellCheck speller;
        lookupSymp->candidateIdFallback(&speller, &matcher);
        திரும்பு speller.best_Candidate_செய்தி(name);
    }
    string suggestSymFlat(VSymEnt* lookupSymp, மாறிலி string& name, மாறிலி VNodeMatcher& matcher,
                          தருக்க includeType) {
        எனில் (!lookupSymp) திரும்பு "";
        VSpellCheck speller;
        lookupSymp->candidateIdFlat(&speller, &matcher, includeType);
        திரும்பு speller.best_Candidate_செய்தி(name);
    }
};

LinkDotState* LinkDotState::s_errorThisp = nullptr;

//======================================================================

வகுப்பு LinkDotFindVisitor final : பொது VNVisitor {
    // NODE STATE
    // Cleared on global
    //  *::user1p()             -> See LinkDotState
    //  *::user2p()             -> See LinkDotState
    //  *::user3()              // bool. Node stored in m_virtualIfaces
    //  *::user4()              -> See LinkDotState
    மாறிலி VNUser3InUse m_inuser3;

    // STATE
    LinkDotState* மாறிலி m_statep;  // State to pass between visitors, including symbol table
    Ast_முனை_Module* m_classOrPackagep = nullptr;  // Current package
    AstClocking* m_clockingp = nullptr;  // Current clocking block
    std::vector<AstScope*> m_scopes;  // Scopes detached during repeated module visits
    uint64_t m_statScopeVisits = 0;  // Non-top scopes encountered while finding instance symbols
    VSymEnt* m_modSymp = nullptr;  // Symbol Entry for current module
    VSymEnt* m_curSymp = nullptr;  // Symbol Entry for current table, where to lookup/insert
    string
        m_hierParamsName;  // Name of module with hierarchical type parameters, empty when not used
    string m_scope;  // Scope text
    மாறிலி Ast_முனை_FTask* m_ftaskp = nullptr;  // Current function/task
    தருக்க m_inRecursion = பொய்;  // Inside a recursive module
    முழு m_param_இலக்கம் = 0;  // Parameter number, for position based connection
    தருக்க m_explicitNew = பொய்;  // Hit a "new" function
    முழு m_mod_Block_இலக்கம் = 0;  // Begin block number in module, 0=none seen
    முழு m_mod_With_இலக்கம் = 0;  // With block number, 0=none seen
    முழு m_mod_Arg_இலக்கம் = 0;  // Arg block number for randomize(), 0=none seen
    std::vector<AstIface*> m_virtIfaces;  // interfaces used as virtual,
                                          // needed for handleUnvisitedVirtIfaces()

    // METHODS
    வெறுமை makeImplicitNew(AstClass* nodep) {
        AstFunc* மாறிலி newp = புதிய AstFunc{nodep->fileline(), "new", nullptr, nullptr};
        // If needed, super.new() call is added the 2nd pass of V3LinkDotResolve,
        // because base classes are already resolved there.
        newp->isConstructor(உண்மை);
        nodep->addMembersp(newp);
        UINFO(8, "Made implicit new for " << nodep->name() << ": " << nodep);
        m_statep->செருகு_Block(m_curSymp, newp->name(), newp, m_classOrPackagep);
    }

    நிலையான தருக்க isHierBlockWrapper(மாறிலி string& name) {
        மாறிலி V3HierBlockOptSet& hierBlocks = v3Global.opt.hierBlocks();
        திரும்பு hierBlocks.find(name) != hierBlocks.end();
    }

    வெறுமை moveExternFuncDecl(Ast_முனை_FTask* nodep, AstClass* classp) {
        // Move an 'extern function|task' to inside a class or, from an outer to inner-class
        எனில் (nodep->isExternDef()) திரும்பு;
        // Move it to proper spot under the target class
        nodep->unlinkFrBack();
        classp->addStmtsp(nodep);
        nodep->isExternDef(உண்மை);  // So we check there's a matching extern
        nodep->classOrPackagep()->unlinkFrBack()->delete_மரம்();
        // Any "Type::" reference in the function's IO are really "MovedToClass::" references
        எனில் (nodep->fvarp()) moveExternFuncDeclRefs(nodep->fvarp(), classp);
        க்கு (Ast_முனை* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            எனில் (AstVar* மாறிலி portp = VN_CAST(stmtp, Var)) {
                எனில் (portp->isIO()) moveExternFuncDeclRefs(portp, classp);
            }
        }
    }
    வெறுமை moveExternFuncDeclRefs(Ast_முனை* nodep, AstClass* classp) {
        nodep->foreach([இது, classp](AstClassOrPackageRef* refp) {  //
            எனில் (refp->name() == classp->name() && !refp->paramsp()) {
                UINFO(9, "Cleaning up external function type for class " << refp);
                AstDot* மாறிலி dotp = VN_CAST(refp->backp(), Dot);
                எனில் (dotp && dotp->lhsp() == refp) {
                    // If had DOT(CLASSREF this, ...) the DOT can go away
                    dotp->replaceWith(dotp->rhsp()->unlinkFrBack());
                    VL_DO_DANGLING2(தள்ளு_Deletep(dotp), dotp, refp);
                } இல்லையெனில் {
                    VL_DO_DANGLING(தள்ளு_Deletep(refp->unlinkFrBack()), refp);
                }
            }
        });
    }

    // VISITORS
    வெறுமை visit(AstNetlist* nodep) override {  // FindVisitor::
        // Process $unit or other packages
        // Not needed - dotted references not allowed from inside packages
        // for (AstNodeModule* nodep = v3Global.rootp()->modulesp();
        //     nodep; nodep=VN_AS(nodep->nextp(), NodeModule)) {
        //    if (VN_IS(nodep, Package)) {}}

        m_statep->செருகு_DUnit(nodep);

        // First back iterate, to find all packages. Backward as must do base
        // packages before using packages
        iterateChildrenBackwardsConst(nodep);

        // The first modules in the list are always the top modules
        // (sorted before this is called).
        // This may not be the module with isTop() set, as early in the steps,
        // wrapTop may have not been created yet.
        // $unit and the constant pool always exist, so nothing else, and nothing in $unit,
        // means nothing was given
        Ast_முனை_Module* மாறிலி modulesp = nodep->modulesp();
        UASSERT_OBJ(modulesp, nodep, "$unit should always be in the netlist");
        எனில் (m_statep->forPrimary()) {
            மாறிலி தருக்க onlyBuiltins = [&]() {
                க்கு (Ast_முனை_Module* modp = modulesp; modp;
                     modp = VN_AS(modp->nextp(), முனை_Module)) {
                    எனில் (!modp->isDollarUnit() && !modp->isConstPool()) திரும்பு பொய்;
                }
                திரும்பு உண்மை;
            }();
            எனில் (onlyBuiltins && !v3Global.rootp()->dollarUnitPkgp()->stmtsp()) {
                nodep->v3error("No top level module found");
            }
        }
        க்கு (Ast_முனை_Module* modp = nodep->modulesp(); modp && modp->is_உச்சி();
             modp = VN_AS(modp->nextp(), முனை_Module)) {
            UINFO(8, "Top Module: " << modp);
            m_scope = "TOP";

            எனில் (!m_statep->forScopeCreation() && v3Global.opt.உச்சி_Ifaces_Supported()) {
                க்கு (Ast_முனை* subnodep = modp->stmtsp(); subnodep; subnodep = subnodep->nextp()) {
                    எனில் (AstVar* மாறிலி varp = VN_CAST(subnodep, Var)) {
                        எனில் (varp->isIfaceRef()) {
                            மாறிலி Ast_முனை_DType* மாறிலி subtypep = varp->subDTypep();
                            மாறிலி AstIfaceRefDType* ifacerefp = nullptr;
                            எனில் (VN_IS(subtypep, IfaceRefDType)) {
                                ifacerefp = VN_AS(varp->subDTypep(), IfaceRefDType);
                            } இல்லையெனில் எனில் (VN_IS(subtypep, BracketArrayDType)) {
                                மாறிலி AstBracketArrayDType* மாறிலி arrp
                                    = VN_AS(subtypep, BracketArrayDType);
                                மாறிலி Ast_முனை_DType* மாறிலி arrsubtypep = arrp->subDTypep();
                                எனில் (VN_IS(arrsubtypep, IfaceRefDType)) {
                                    ifacerefp = VN_AS(arrsubtypep, IfaceRefDType);
                                }
                            } இல்லையெனில் எனில் (VN_IS(subtypep, UnpackArrayDType)) {
                                மாறிலி AstUnpackArrayDType* மாறிலி arrp
                                    = VN_AS(subtypep, UnpackArrayDType);
                                மாறிலி Ast_முனை_DType* மாறிலி arrsubtypep = arrp->subDTypep();
                                எனில் (VN_IS(arrsubtypep, IfaceRefDType)) {
                                    ifacerefp = VN_AS(arrsubtypep, IfaceRefDType);
                                }
                            }

                            எனில் (ifacerefp && !ifacerefp->cellp()) {
                                // A dummy cell to keep the top level interface alive and correctly
                                // optimized for default parameter values
                                AstCell* ifacecellp
                                    = புதிய AstCell{nodep->fileline(),
                                                  nodep->fileline(),
                                                  modp->name() + "__02E" + varp->name(),
                                                  ifacerefp->ifaceName(),
                                                  nullptr,
                                                  nullptr,
                                                  nullptr};
                                ifacecellp->modp(ifacerefp->ifacep());
                                m_curSymp = m_modSymp
                                    = m_statep->செருகு_உச்சி_Iface(ifacecellp, m_scope);
                                iterate(ifacecellp);
                                VL_DO_DANGLING(தள்ளு_Deletep(ifacecellp), ifacecellp);
                            }
                        }
                    }
                }
            }

            m_curSymp = m_modSymp = m_statep->செருகு_உச்சி_Cell(modp, m_scope);
            { iterate(modp); }

            m_scope = "";
            m_curSymp = m_modSymp = nullptr;
        }
    }
    வெறுமை visit(AstTypeTable*) override {}  // FindVisitor::
    வெறுமை visit(AstIfaceRefDType* nodep) override {  // FindVisitor::
        எனில் (!m_statep->forScopeCreation() && nodep->isVirtual() && nodep->ifacep()
            && !nodep->ifacep()->user3()) {
            m_virtIfaces.தள்ளு_back(nodep->ifacep());
            nodep->ifacep()->user3(உண்மை);
        }
        iterateChildren(nodep);
    }
    வெறுமை visit(Ast_முனை_Module* nodep) override {  // FindVisitor::
        // Called on top module from Netlist, other modules from the cell creating them,
        // and packages
        UINFO(8, "   " << nodep);
        // m_curSymp/m_modSymp maybe nullptr for packages and non-top modules
        // Packages will be under top after the initial phases, but until then
        // need separate handling
        மாறிலி தருக்க standalonePkg
            = !m_modSymp && !m_statep->forScopeCreation() && VN_IS(nodep, Package);
        மாறிலி தருக்க doit = (m_modSymp || standalonePkg);
        VL_RESTORER_COPY(m_scope);
        VL_RESTORER(m_classOrPackagep);
        VL_RESTORER(m_modSymp);
        VL_RESTORER(m_curSymp);
        VL_RESTORER(m_param_இலக்கம்);
        VL_RESTORER(m_mod_Block_இலக்கம்);
        VL_RESTORER(m_mod_With_இலக்கம்);
        VL_RESTORER(m_mod_Arg_இலக்கம்);
        எனில் (doit && nodep->user2()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Identically recursive module (module instantiates "
                          "itself, without changing parameters): "
                              << Ast_முனை::prettyNameQ(nodep->origName()));
        } இல்லையெனில் எனில் (doit) {
            UINFO(4, "     Link Module: " << nodep);
            UASSERT_OBJ(!nodep->dead(), nodep, "Module in instance tree mislabeled as dead?");
            AstPackage* மாறிலி pkgp = VN_CAST(nodep, Package);
            m_classOrPackagep = pkgp;
            எனில் (standalonePkg) {
                எனில் (pkgp->isDollarUnit()) {
                    m_curSymp = m_modSymp = m_statep->dunitEntp();
                    nodep->user1p(m_curSymp);
                } இல்லையெனில் {
                    VSymEnt* மாறிலி upperSymp = m_statep->dunitEntp();
                    m_scope = nodep->name();
                    m_curSymp = m_modSymp = m_statep->செருகு_Block(upperSymp, nodep->name(), nodep,
                                                                  m_classOrPackagep);
                    UINFO(9, "New module scope " << m_curSymp);
                }
            }
            //
            m_param_இலக்கம் = 0;
            m_mod_Block_இலக்கம் = 0;
            m_mod_With_இலக்கம் = 0;
            m_mod_Arg_இலக்கம் = 0;
            // m_modSymp/m_curSymp for non-packages set by AstCell above this module
            // Iterate
            nodep->user2(உண்மை);
            iterateChildren(nodep);
            nodep->user2(பொய்);
            nodep->user4(உண்மை);
            // Interfaces need another pass when signals are resolved. When creating
            // scopes this is done from the AstScopes in LinkDotScopeVisitor instead, as
            // only those entries hold the AstVarScopes the modport items resolve against.
            எனில் (!m_statep->forScopeCreation()) {
                எனில் (AstIface* மாறிலி ifacep = VN_CAST(nodep, Iface)) {
                    m_statep->செருகு_Iface_Mod_Sym(ifacep, m_curSymp);
                }
            }
        } இல்லையெனில் எனில் (isHierBlockWrapper(nodep->name())) {
            UINFO(5, "Module is hierarchical block, must not be dead: " << nodep);
            m_scope = nodep->name();
            VSymEnt* மாறிலி upperSymp = m_curSymp ? m_curSymp : m_statep->வேர்_Entp();
            m_curSymp = m_modSymp
                = m_statep->செருகு_Block(upperSymp, nodep->name() + "::", nodep, m_classOrPackagep);
            iterateChildren(nodep);
            nodep->user4(உண்மை);
        } இல்லையெனில் {  // !doit
            எனில் (nodep->hierParams()) {
                UINFO(1, "Found module with hier type parameters");
                m_hierParamsName = nodep->name();
                க்கு (மாறிலி Ast_முனை* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    எனில் (மாறிலி AstTypedef* மாறிலி tdef = VN_CAST(stmtp, Typedef)) {
                        UINFO(1, "Inserting hier type parameter typedef: " << tdef);
                        VSymEnt* மாறிலி upperSymp = m_curSymp ? m_curSymp : m_statep->வேர்_Entp();
                        m_curSymp = m_modSymp = m_statep->செருகு_Block(upperSymp, nodep->name(),
                                                                      nodep, m_classOrPackagep);
                    }
                }
            } இல்லையெனில் {
                // Will be optimized away later
                // Can't remove now, as our backwards iterator will throw up
                UINFO(5, "Module not under any CELL or top - dead module: " << nodep);
            }
        }
    }

    வெறுமை visit(AstClass* nodep) override {  // FindVisitor::
        UASSERT_OBJ(m_curSymp, nodep, "Class not under module/package/$unit");
        UINFO(8, "   " << nodep);
        VL_RESTORER_COPY(m_scope);
        VL_RESTORER(m_classOrPackagep);
        VL_RESTORER(m_modSymp);
        VL_RESTORER(m_curSymp);
        VL_RESTORER(m_param_இலக்கம்);
        VL_RESTORER(m_mod_Block_இலக்கம்);
        VL_RESTORER(m_mod_With_இலக்கம்);
        VL_RESTORER(m_mod_Arg_இலக்கம்);
        VL_RESTORER(m_explicitNew);
        {
            UINFO(4, "     Link Class: " << nodep);
            VSymEnt* மாறிலி upperSymp = m_curSymp;
            m_scope = m_scope + "." + nodep->name();
            m_classOrPackagep = nodep;
            m_curSymp = m_modSymp
                = m_statep->செருகு_Block(upperSymp, nodep->name(), nodep, m_classOrPackagep);
            m_statep->செருகு_Map(m_curSymp, m_scope);
            UINFO(9, "New module scope " << m_curSymp);
            //
            m_param_இலக்கம் = 0;
            m_mod_Block_இலக்கம் = 0;
            m_mod_With_இலக்கம் = 0;
            m_mod_Arg_இலக்கம் = 0;
            m_explicitNew = பொய்;
            // m_modSymp/m_curSymp for non-packages set by AstCell above this module
            // Iterate
            iterateChildren(nodep);
            nodep->user4(உண்மை);
            // Implicit new needed?
            எனில் (!m_explicitNew && m_statep->forPrimary()) makeImplicitNew(nodep);
        }
    }
    வெறுமை visit(AstClassOrPackageRef* nodep) override {  // FindVisitor::
        எனில் (!nodep->classOrPackageNodep() && nodep->name() == "$unit") {
            nodep->classOrPackageNodep(v3Global.rootp()->dollarUnitPkgp());
        }
        iterateChildren(nodep);
    }
    வெறுமை visit(AstScope* nodep) override {  // FindVisitor::
        UASSERT_OBJ(m_statep->forScopeCreation(), nodep,
                    "Scopes should only exist right after V3Scope");
        // Detach on first encounter, will re add at the end.
        // Otherwise would be visited O(cells^2) time after V3Scope
        எனில் (!nodep->is_உச்சி()) {
            ++m_statScopeVisits;
            nodep->unlinkFrBack();
            m_scopes.தள்ளு_back(nodep);
        }
    }
    வெறுமை visit(AstCell* nodep) override {  // FindVisitor::
        UINFO(5, "   CELL under " << m_scope << " is " << nodep);
        // Process XREFs/etc inside pins
        எனில் (nodep->recursive() && m_inRecursion) திரும்பு;
        iterateChildren(nodep);
        // Recurse in, preserving state
        VL_RESTORER_COPY(m_scope);
        VL_RESTORER(m_modSymp);
        VL_RESTORER(m_curSymp);
        VL_RESTORER(m_param_இலக்கம்);
        VL_RESTORER(m_inRecursion);
        // Where do we add it?
        VSymEnt* aboveSymp = m_curSymp;
        மாறிலி string origname = Ast_முனை::dedotName(nodep->name());
        string::size_type pos = origname.rfind('.');
        எனில் (pos != string::npos) {
            // Flattened, find what CellInline it should live under
            மாறிலி string scope = origname.substr(0, pos);
            string baddot;
            VSymEnt* okSymp;
            aboveSymp
                = m_statep->findDotted(nodep->fileline(), aboveSymp, scope, baddot, okSymp, பொய்);
            UASSERT_OBJ(aboveSymp, nodep,
                        "Can't find instance insertion point at "
                            << Ast_முனை::prettyNameQ(baddot) << " in: " << nodep->prettyNameQ());
        }
        {
            m_scope = m_scope + "." + nodep->name();
            m_curSymp = m_modSymp = m_statep->செருகு_Cell(aboveSymp, m_modSymp, nodep, m_scope);
            m_inRecursion = nodep->recursive();
            // We don't report NotFoundModule, as may be a unused module in a generate
            எனில் (nodep->modp()) iterate(nodep->modp());
        }
    }
    வெறுமை visit(AstCellInline* nodep) override {  // FindVisitor::
        UINFO(5, "   CELLINLINE under " << m_scope << " is " << nodep);
        VSymEnt* aboveSymp = m_curSymp;
        std::string ident = nodep->name();
        // If baz__DOT__foo__DOT__bar, we need to find baz__DOT__foo and add bar to it.
        மாறிலி std::string::size_type pos = ident.rfind("__DOT__");
        எனில் (pos != std::string::npos) {
            மாறிலி std::string dotted = ident.substr(0, pos);
            ident = ident.substr(pos + std::strlen("__DOT__"));
            std::string baddot;
            VSymEnt* okSymp;
            aboveSymp = m_statep->findDotted(nodep->fileline(), aboveSymp, dotted, baddot, okSymp,
                                             பொய்);
            UASSERT_OBJ(aboveSymp, nodep,
                        "Can't find cellinline insertion point at "
                            << Ast_முனை::prettyNameQ(baddot) << " in: " << nodep->prettyNameQ());
        }
        m_statep->செருகு_Inline(aboveSymp, m_modSymp, nodep, ident);
    }
    வெறுமை visit(AstDefParam* nodep) override {  // FindVisitor::
        nodep->user1p(m_curSymp);
        iterateChildren(nodep);
    }
    வெறுமை visit(AstRefDType* nodep) override {  // FindVisitor::
        nodep->user1p(m_curSymp);
        iterateChildren(nodep);
    }
    வெறுமை visit(AstGenBlock* nodep) override {  // FindVisitor::
        UINFO(5, "   " << nodep);
        எனில் (nodep->name() == "" && nodep->unnamed()) {
            // Unnamed blocks are only important when they contain var
            // decls, so search for them. (Otherwise adding all the
            // unnamed#'s would just confuse tracing variables in
            // places such as tasks, where "task ...; begin ... end"
            // are common.
            க்கு (Ast_முனை* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
                எனில் (VN_IS(itemp, Var) || VN_IS(itemp, Foreach)) {
                    std::string name;
                    மாறிலி std::string stepStr = m_statep->forPrimary()
                                                    ? ""
                                                    : std::to_string(m_statep->step_எண்()) + "_";
                    செய் {
                        ++m_mod_Block_இலக்கம்;
                        name = "unnamedblk" + stepStr + cvtToStr(m_mod_Block_இலக்கம்);
                        // Increment again if earlier pass of V3LinkDot claimed this name
                    } போது (m_curSymp->findIdFlat(name));
                    nodep->name(name);
                    நிறுத்து;
                }
            }
        }
        எனில் (nodep->name() == "") {
            iterateChildren(nodep);
        } இல்லையெனில் {
            VL_RESTORER(m_curSymp);
            {
                m_curSymp
                    = m_statep->செருகு_Block(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
                m_curSymp->fallbackp(VL_RESTORER_PREV(m_curSymp));
                // Iterate
                iterateChildren(nodep);
            }
        }
    }
    வெறுமை visit(Ast_முனை_Block* nodep) override {  // FindVisitor::
        UINFO(5, "   " << nodep);
        எனில் (nodep->name() == "" && nodep->unnamed()) {
            // Unnamed blocks are only important when they contain var
            // decls, so search for them. (Otherwise adding all the
            // unnamed#'s would just confuse tracing variables in
            // places such as tasks, where "task ...; begin ... end"
            // are common.
            மாறிலி auto containsDecl = [](மாறிலி Ast_முனை* nodesp) -> தருக்க {
                க்கு (மாறிலி Ast_முனை* np = nodesp; np; np = np->nextp()) {
                    எனில் (VN_IS(np, Var) || VN_IS(np, Foreach)) திரும்பு உண்மை;
                }
                திரும்பு பொய்;
            };
            தருக்க needName = nodep->declsp() || containsDecl(nodep->stmtsp());
            எனில் (மாறிலி AstFork* மாறிலி forkp = VN_CAST(nodep, Fork)) {
                needName = needName || containsDecl(forkp->forksp());
            }
            எனில் (needName) {
                மாறிலி std::string stepStr
                    = m_statep->forPrimary() ? "" : std::to_string(m_statep->step_எண்()) + "_";
                std::string name;
                செய் {
                    ++m_mod_Block_இலக்கம்;
                    name = "unnamedblk" + stepStr + cvtToStr(m_mod_Block_இலக்கம்);
                    // Increment again if earlier pass of V3LinkDot claimed this name
                } போது (m_curSymp->findIdFlat(name));
                nodep->name(name);
            }
        }
        எனில் (nodep->name() == "") {
            iterateChildren(nodep);
        } இல்லையெனில் {
            VL_RESTORER(m_curSymp);
            {
                m_curSymp
                    = m_statep->செருகு_Block(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
                m_curSymp->fallbackp(VL_RESTORER_PREV(m_curSymp));
                // Iterate
                iterateChildren(nodep);
            }
        }
    }
    // Handle out-of-block interface method definition (IEEE 1800-2023 25.7)
    // Move task body from module into the interface's prototype task,
    // rewriting port-prefixed references in the body.
    வெறுமை moveIfaceExportBody(Ast_முனை_FTask* nodep) {
        மாறிலி string& portName = nodep->ifacePortName();
        UINFO(5, "   moveIfaceExportBody: port=" << portName << " task=" << nodep->name());
        // Look up the interface port variable in the module's symbol table
        VSymEnt* மாறிலி portSymp = m_modSymp->findIdFallback(portName);
        எனில் (!portSymp) {
            nodep->v3error("Interface port not found for out-of-block definition: '"
                           << portName << "' (IEEE 1800-2023 25.7)");
            nodep->unlinkFrBack();
            VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
            திரும்பு;
        }
        AstVar* மாறிலி portp = VN_CAST(portSymp->nodep(), Var);
        எனில் (!portp) {
            nodep->v3fatalSrc("Out-of-block definition port is not a variable: " << portName);
            திரும்பு;
        }
        // Get the interface name from the port's type
        // At this stage dtypep() may not be set yet; check childDTypep() too
        AstIfaceRefDType* ifaceRefDtp = nullptr;
        எனில் (portp->dtypep()) { ifaceRefDtp = VN_CAST(portp->dtypep(), IfaceRefDType); }
        எனில் (!ifaceRefDtp && portp->childDTypep()) {
            ifaceRefDtp = VN_CAST(portp->childDTypep(), IfaceRefDType);
        }
        எனில் (!ifaceRefDtp) {
            nodep->v3error("Out-of-block definition port is not an interface port: '"
                           << portName << "' (IEEE 1800-2023 25.7)");
            nodep->unlinkFrBack();
            VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
            திரும்பு;
        }
        மாறிலி string ifaceName = ifaceRefDtp->ifaceName();
        UINFO(5, "   moveIfaceExportBody: iface=" << ifaceName);
        // Find the interface module by name
        Ast_முனை_Module* மாறிலி ifaceModp = m_statep->findModuleSym(ifaceName);
        எனில் (!ifaceModp) {
            nodep->v3fatalSrc("Interface not found for out-of-block definition: " << ifaceName);
            திரும்பு;
        }
        // Find the prototype task in the interface
        Ast_முனை_FTask* protoTaskp = nullptr;
        க்கு (Ast_முனை* itemp = ifaceModp->stmtsp(); itemp; itemp = itemp->nextp()) {
            எனில் (Ast_முனை_FTask* மாறிலி ftaskp = VN_CAST(itemp, முனை_FTask)) {
                எனில் (ftaskp->name() == nodep->name() && ftaskp->prototype()) {
                    protoTaskp = ftaskp;
                    நிறுத்து;
                }
            }
        }
        எனில் (!protoTaskp) {
            nodep->v3error("No matching export prototype found in interface "
                           << ifaceName << " for task " << nodep->prettyNameQ()
                           << " (IEEE 1800-2023 25.7)");
            nodep->unlinkFrBack();
            VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
            திரும்பு;
        }
        // Move body statements (non-port declarations) from the out-of-block task
        // to the prototype task. Rewrite port.X references to just X in the body.
        க்கு (Ast_முனை* stmtp = nodep->stmtsp(); stmtp;) {
            Ast_முனை* மாறிலி nextp = stmtp->nextp();
            // Skip port declarations -- they already exist in the prototype
            எனில் (மாறிலி AstVar* மாறிலி varp = VN_CAST(stmtp, Var)) {
                எனில் (varp->isIO()) {
                    stmtp = nextp;
                    தொடர்;
                }
            }
            // Clone this statement and rewrite port references
            Ast_முனை* மாறிலி newStmtp = stmtp->clone_மரம்(பொய்);
            rewriteIfacePortRefsSingle(newStmtp, portName);
            protoTaskp->addStmtsp(newStmtp);
            stmtp = nextp;
        }
        // Mark prototype as no longer prototype (it has a body now)
        protoTaskp->prototype(பொய்);
        // Delete the out-of-block task from the module
        nodep->unlinkFrBack();
        VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
    }

    // Rewrite AstDot nodes where lhsp is AstParseRef with the port name
    // to just the rhsp (stripping the port prefix).
    நிலையான வெறுமை rewriteIfacePortRefsSingle(Ast_முனை* nodep, மாறிலி string& portName) {
        எனில் (!nodep) திரும்பு;
        // Check if this is a non-colon Dot with lhs matching portName
        எனில் (AstDot* மாறிலி dotp = VN_CAST(nodep, Dot)) {
            எனில் (!dotp->colon()) {
                மாறிலி AstParseRef* மாறிலி lhsRefp = VN_CAST(dotp->lhsp(), ParseRef);
                எனில் (lhsRefp && lhsRefp->name() == portName) {
                    UINFO(5, "   rewriteIfacePortRefs: stripping port prefix from " << dotp);
                    Ast_முனை* மாறிலி rhsp = dotp->rhsp()->unlinkFrBack();
                    dotp->replaceWith(rhsp);
                    VL_DO_DANGLING(dotp->delete_மரம்(), dotp);
                    rewriteIfacePortRefsSingle(rhsp, portName);
                    திரும்பு;
                }
            }
        }
        // Recurse into all children
        க்கு (Ast_முனை* childp : {nodep->op1p(), nodep->op2p(), nodep->op3p(), nodep->op4p()}) {
            போது (childp) {
                Ast_முனை* மாறிலி nextp = childp->nextp();
                rewriteIfacePortRefsSingle(childp, portName);
                childp = nextp;
            }
        }
    }

    வெறுமை visit(Ast_முனை_FTask* nodep) override {  // FindVisitor::
        // NodeTask: Remember its name for later resolution
        UINFO(5, "   " << nodep);
        UASSERT_OBJ(m_curSymp && m_modSymp, nodep, "Function/Task not under module?");
        எனில் (nodep->name() == "new") m_explicitNew = உண்மை;
        // Handle out-of-block interface method definition (IEEE 25.8)
        எனில் (!nodep->ifacePortName().empty() && m_statep->forPrimary()) {
            moveIfaceExportBody(nodep);
            திரும்பு;  // Task has been deleted
        }
        // Remember the existing symbol table scope
        VL_RESTORER(m_classOrPackagep);
        VL_RESTORER(m_curSymp);
        VSymEnt* upSymp = m_curSymp;

        எனில் (VN_IS(m_curSymp->nodep(), Class)) {
            எனில் (VN_AS(m_curSymp->nodep(), Class)->isInterfaceClass() && !nodep->pureVirtual()
                && !nodep->isConstructor()) {
                nodep->v3error("Interface class functions must be pure virtual"
                               << " (IEEE 1800-2023 8.26): " << nodep->prettyNameQ());
            }
            எனில் (m_statep->forPrimary()
                && (nodep->name() == "randomize" || nodep->name() == "srandom")) {
                nodep->v3error(nodep->prettyNameQ()
                               << " is a predefined class method; redefinition not allowed"
                                  " (IEEE 1800-2023 18.6.3)");
            }
        }
        // Change to appropriate package if extern declaration (vs definition)
        எனில் (nodep->classOrPackagep()) {
            // class-in-class
            AstDot* மாறிலி dotp = VN_CAST(nodep->classOrPackagep(), Dot);
            AstClassOrPackageRef* மாறிலி cpackagerefp
                = VN_CAST(nodep->classOrPackagep(), ClassOrPackageRef);
            எனில் (dotp) {
                AstClassOrPackageRef* மாறிலி lhsp = VN_AS(dotp->lhsp(), ClassOrPackageRef);
                m_statep->resolveClassOrPackage(m_curSymp, lhsp, உண்மை, பொய்,
                                                "External definition :: reference");
                AstClass* மாறிலி lhsclassp = VN_CAST(lhsp->classOrPackageSkipp(), Class);
                எனில் (!lhsclassp) {
                    nodep->v3error("Extern declaration's scope is not a defined class");
                } இல்லையெனில் {
                    m_curSymp = m_statep->get_முனை_Sym(lhsclassp);
                    upSymp = m_curSymp;
                    AstClassOrPackageRef* மாறிலி rhsp = VN_AS(dotp->rhsp(), ClassOrPackageRef);
                    m_statep->resolveClassOrPackage(m_curSymp, rhsp, உண்மை, பொய்,
                                                    "External definition :: reference");
                    AstClass* மாறிலி rhsclassp = VN_CAST(rhsp->classOrPackageSkipp(), Class);
                    எனில் (!rhsclassp) {
                        nodep->v3error("Extern declaration's scope is not a defined class");
                    } இல்லையெனில் {
                        m_curSymp = m_statep->get_முனை_Sym(rhsclassp);
                        upSymp = m_curSymp;
                        moveExternFuncDecl(nodep, rhsclassp);
                    }
                }
            } இல்லையெனில் எனில் (cpackagerefp) {
                எனில் (!cpackagerefp->classOrPackageSkipp()) {
                    m_statep->resolveClassOrPackage(m_curSymp, cpackagerefp, உண்மை, பொய்,
                                                    "External definition :: reference");
                }
                AstClass* மாறிலி classp = VN_CAST(cpackagerefp->classOrPackageSkipp(), Class);
                எனில் (!classp) {
                    nodep->v3error("Extern declaration's scope is not a defined class");
                } இல்லையெனில் {
                    m_curSymp = m_statep->get_முனை_Sym(classp);
                    upSymp = m_curSymp;
                    moveExternFuncDecl(nodep, classp);
                }
            } இல்லையெனில் {
                v3fatalSrc("Unhandled extern function definition package");
            }
        }
        // Set the class as package for iteration
        எனில் (VN_IS(m_curSymp->nodep(), Class)) {
            m_classOrPackagep = VN_AS(m_curSymp->nodep(), Class);
        }
        // Create symbol table for the task's vars
        மாறிலி string name = (nodep->isExternProto() ? "extern "s : ""s) + nodep->name();
        m_curSymp = m_statep->செருகு_Block(m_curSymp, name, nodep, m_classOrPackagep);
        m_curSymp->fallbackp(upSymp);
        // Convert the func's range to the output variable
        // This should probably be done in the Parser instead, as then we could
        // just attach normal signal attributes to it.
        எனில் (!nodep->isExternProto() && nodep->fvarp() && !VN_IS(nodep->fvarp(), Var)) {
            Ast_முனை_DType* dtypep = VN_CAST(nodep->fvarp(), முனை_DType);
            // If unspecified, function returns one bit; however when we
            // support NEW() it could also return the class reference.
            எனில் (dtypep) {
                dtypep->unlinkFrBack();
            } இல்லையெனில் {
                dtypep = புதிய AstBasicDType{nodep->fileline(), VBasicDTypeKwd::LOGIC};
            }
            AstVar* மாறிலி newvarp
                = புதிய AstVar{nodep->fileline(), VVarType::VAR, nodep->name(), VFlagChildDType{},
                             dtypep};  // Not dtype resolved yet
            newvarp->direction(VDirection::OUTPUT);
            newvarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            newvarp->funcReturn(உண்மை);
            newvarp->trace(பொய்);  // Not user visible
            nodep->fvarp(newvarp);
            // Explicit insert required, as the var name shadows the upper level's task name
            m_statep->செருகு_Sym(m_curSymp, newvarp->name(), newvarp, nullptr /*classOrPackagep*/);
        }
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    வெறுமை visit(AstClocking* nodep) override {  // FindVisitor::
        VL_RESTORER(m_clockingp);
        m_clockingp = nodep;
        iterate(nodep->sentreep());
        iterateAndNextNull(nodep->itemsp());
        // If the block has no name, one cannot reference the clockvars
        VSymEnt* itSymp = nullptr;
        எனில் (nodep->isGlobal()  //
            && m_statep->forPrimary()) {  // else flattening may see two globals
            m_statep->checkDuplicate(m_curSymp, nodep, "__024global_clock");
            itSymp
                = m_statep->செருகு_Block(m_curSymp, "__024global_clock", nodep, m_classOrPackagep);
            itSymp->fallbackp(nullptr);
        }
        எனில் (!nodep->name().empty()) {
            itSymp = m_statep->செருகு_Block(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
            itSymp->fallbackp(nullptr);
        }
        எனில் (itSymp) {
            VL_RESTORER(m_curSymp);
            m_curSymp = itSymp;
            iterateAndNextNull(nodep->itemsp());
        }
    }
    வெறுமை visit(AstClockingItem* nodep) override {  // FindVisitor::
        எனில் (nodep->varp()) {
            எனில் (m_curSymp->nodep() == m_clockingp) iterate(nodep->varp());
            திரும்பு;
        }
        std::string varname;
        Ast_முனை_DType* dtypep;
        எனில் (AstAssign* மாறிலி assignp = nodep->assignp()) {
            Ast_முனை_கோவை* மாறிலி rhsp = assignp->rhsp()->unlinkFrBack();
            dtypep = புதிய AstRefDType{nodep->fileline(), AstRefDType::Flag_Type_Of_கோவை{},
                                     rhsp->clone_மரம்(பொய்)};
            nodep->exprp(rhsp);
            varname = assignp->lhsp()->name();
            VL_DO_DANGLING(assignp->unlinkFrBack()->delete_மரம்(), assignp);
        } இல்லையெனில் {
            Ast_முனை_கோவை* மாறிலி refp = nodep->exprp();
            மாறிலி VSymEnt* foundp = m_curSymp->findIdFallback(refp->name());
            எனில் (!foundp || !foundp->nodep()) {
                refp->v3error("Corresponding variable " << refp->prettyNameQ()
                                                        << " does not exist");
                VL_DO_DANGLING(nodep->unlinkFrBack()->delete_மரம்(), nodep);
                திரும்பு;
            }
            varname = refp->name();
            dtypep = VN_AS(foundp->nodep(), Var)->childDTypep()->clone_மரம்(பொய்);
        }
        AstVar* மாறிலி newvarp = புதிய AstVar{nodep->fileline(), VVarType::MODULETEMP, varname,
                                           VFlagChildDType{}, dtypep};
        newvarp->lifetime(VLifetime::STATIC_EXPLICIT);
        nodep->varp(newvarp);
        iterate(nodep->exprp());
    }
    வெறுமை visit(AstConstraint* nodep) override {  // FindVisitor::
        VL_RESTORER(m_curSymp);
        // Change to appropriate package if extern declaration (vs definition)
        VSymEnt* upSymp = m_curSymp;
        எனில் (nodep->classOrPackagep()) {
            AstClassOrPackageRef* மாறிலி cpackagerefp
                = VN_CAST(nodep->classOrPackagep(), ClassOrPackageRef);
            எனில் (!cpackagerefp) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: extern constraint definition with class-in-class");
            } இல்லையெனில் {
                எனில் (!cpackagerefp->classOrPackageSkipp()) {
                    m_statep->resolveClassOrPackage(m_curSymp, cpackagerefp, உண்மை, பொய்,
                                                    "External definition :: reference");
                }
                AstClass* மாறிலி classp = VN_CAST(cpackagerefp->classOrPackageSkipp(), Class);
                எனில் (!classp) {
                    nodep->v3error("Extern declaration's scope is not a defined class");
                } இல்லையெனில் {
                    m_curSymp = m_statep->get_முனை_Sym(classp);
                    upSymp = m_curSymp;
                    // Move it to proper spot under the target class
                    nodep->unlinkFrBack();
                    classp->addStmtsp(nodep);
                    nodep->classOrPackagep()->unlinkFrBack()->delete_மரம்();
                }
            }
        }
        // Set the class as package for iteration
        மாறிலி string name = (nodep->isExternProto() ? "extern "s : ""s) + nodep->name();
        m_curSymp = m_statep->செருகு_Block(upSymp, name, nodep, m_classOrPackagep);
        iterateChildren(nodep);
    }
    வெறுமை visit(AstVar* nodep) override {  // FindVisitor::
        // Var: Remember its name for later resolution
        UASSERT_OBJ(m_curSymp && m_modSymp, nodep, "Var not under module?");
        iterateChildren(nodep);
        எனில் (VN_IS(m_curSymp->nodep(), Class)
            && VN_AS(m_curSymp->nodep(), Class)->isInterfaceClass() && !nodep->isParam()) {
            nodep->v3error("Interface class cannot contain non-parameter members"
                           << " (IEEE 1800-2023 8.26): " << nodep->prettyNameQ());
        }
        எனில் (!m_statep->forScopeCreation()) {
            // Find under either a task or the module's vars
            மாறிலி VSymEnt* foundp = m_curSymp->findIdFallback(nodep->name());
            எனில் (!foundp && m_modSymp && nodep->name() == m_modSymp->nodep()->name()) {
                foundp = m_modSymp;  // Conflicts with modname?
            }
            AstVar* மாறிலி findvarp = foundp ? VN_CAST(foundp->nodep(), Var) : nullptr;
            // clocking items can have duplicate names (inout)
            எனில் (findvarp && VN_IS(findvarp->backp(), ClockingItem)
                && VN_IS(nodep->backp(), ClockingItem)) {
                AstClockingItem* மாறிலி itemp = VN_AS(nodep->backp(), ClockingItem);
                AstClockingItem* மாறிலி finditemp = VN_AS(findvarp->backp(), ClockingItem);
                UINFO(4, "ClockCompl: " << itemp << " ;; " << finditemp);
                UINFO(4, "ClockCompV: " << nodep << " ;; " << findvarp);
                எனில் (*itemp->exprp()->fileline() == *finditemp->exprp()->fileline()) {
                    UASSERT_OBJ(finditemp->direction() == VDirection::உள்ளீடு
                                    && itemp->direction() == VDirection::OUTPUT,
                                itemp, "Input after output?");
                    // pretend nothing found and rename
                    foundp = nullptr;
                    nodep->name("__Voutput_" + nodep->name());
                    finditemp->outputp(itemp);
                }
            }
            தருக்க ins = பொய்;
            எனில் (!foundp) {
                ins = உண்மை;
            } இல்லையெனில் எனில் (!findvarp && m_curSymp->findIdFlat(nodep->name()) && !foundp->imported()) {
                nodep->v3error("Unsupported in C: Variable has same name as "
                               << LinkDotState::முனை_Text_Type(foundp->nodep()) << ": "
                               << nodep->prettyNameQ());
            } இல்லையெனில் எனில் (findvarp != nodep) {
                UINFO(4, "DupVar: " << nodep << " ;; " << foundp->nodep());
                UINFO(4, "    found  cur=se" << cvtToHex(m_curSymp) << " ;; parent=se"
                                             << cvtToHex(foundp->parentp()));
                எனில் (foundp->parentp() == m_curSymp  // Only when on same level
                    && !foundp->imported()) {  // and not from package
                    எனில் (VN_IS(m_curSymp->nodep(), Clocking)) {
                        nodep->v3error("Multiple clockvars with the same name not allowed");
                        திரும்பு;
                    }
                    மாறிலி தருக்க nansiBad
                        = (((findvarp->isDeclTyped() || findvarp->isNet())
                            && (nodep->isDeclTyped() || nodep->isNet()))
                           || (findvarp->isIO() && nodep->isIO())  // e.g. !(output && output)
                           || findvarp->gotNansiType());  // e.g. int x && int x
                    மாறிலி தருக்க ansiBad
                        = findvarp->isAnsi() || nodep->isAnsi();  // dup illegal with ANSI
                    எனில் (ansiBad || nansiBad) {
                        தருக்க ansiWarn = ansiBad && !nansiBad;
                        எனில் (ansiWarn) {
                            நிலையான முழு s_didAnsiWarn = பொய்;
                            எனில் (s_didAnsiWarn++) ansiWarn = பொய்;
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
                    } இல்லையெனில் {
                        findvarp->combineType(nodep);
                        findvarp->fileline()->modifyStateInherit(nodep->fileline());
                        findvarp->gotNansiType(உண்மை);
                        UASSERT_OBJ(nodep->subDTypep(), nodep, "Var has no type");
                        எனில் (nodep->subDTypep()->numeric().isSigned()
                            && !findvarp->subDTypep()->numeric().isSigned()) {
                            findvarp->subDTypep()->numeric(VSigning{உண்மை});
                        }
                        Ast_முனை_DType* மாறிலி varDtp = findvarp->subDTypep();
                        Ast_முனை_DType* மாறிலி otherDtp = nodep->subDTypep();
                        எனில் (varDtp
                            && (VN_IS(varDtp, BasicDType)
                                && VN_AS(varDtp, BasicDType)->implicit())) {
                            // Then have "input foo" and "real foo" so the
                            // dtype comes from the other side.
                            VL_DO_DANGLING(varDtp->unlinkFrBack()->delete_மரம்(), varDtp);
                            findvarp->childDTypep(otherDtp->unlinkFrBack());
                        } இல்லையெனில் எனில் (!m_statep->forScopeCreation() && otherDtp && varDtp
                                   && !(VN_IS(otherDtp, BasicDType)
                                        && VN_AS(otherDtp, BasicDType)->implicit())) {
                            // otherDtp and varDtp both non-nullptr and neither are implicit
                            // Can't compare dtypes now as might contain parameters,
                            // defer to V3Width
                            AstAttrOf* மாறிலி newp
                                = புதிய AstAttrOf{otherDtp->fileline(), VAttrType::VAR_PORT_DTYPE,
                                                otherDtp->unlinkFrBack()};
                            findvarp->addAttrsp(newp);
                        }
                    }
                    VL_DO_DANGLING(nodep->unlinkFrBack()->delete_மரம்(), nodep);
                } இல்லையெனில் {
                    // User can disable the message at either point
                    எனில் (!(m_ftaskp && m_ftaskp->dpiImport())
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
                    ins = உண்மை;
                }
            }
            எனில் (ins) {
                எனில் (m_statep->forPrimary() && nodep->isGParam()
                    && VN_IS(m_modSymp->nodep(), Module)
                    && (m_statep->வேர்_Entp()->nodep() == m_modSymp->parentp()->nodep())) {
                    // This is the toplevel module. Check for command line overwrites of parameters
                    // We first search if the parameter is overwritten and then replace it with a
                    // new value.
                    எனில் (v3Global.opt.hasParameter(nodep->name())) {
                        மாறிலி string svalue = v3Global.opt.parameter(nodep->name());
                        எனில் (AstConst* மாறிலி valuep
                            = AstConst::parseParamLiteral(nodep->fileline(), svalue)) {
                            UINFO(9, "       replace parameter " << nodep);
                            UINFO(9, "       with " << valuep);
                            எனில் (nodep->valuep()) தள்ளு_Deletep(nodep->valuep()->unlinkFrBack());
                            nodep->valuep(valuep);
                            // Do not warn ENUMVALUE when overriding an enum param from the cli.
                            nodep->fileline()->warnOff(V3ErrorCode::ENUMVALUE, உண்மை);
                        }
                    }
                }
                m_statep->செருகு_Sym(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
                எனில் (m_statep->forPrimary() && nodep->isGParam()) {
                    ++m_param_இலக்கம்;
                    VSymEnt* மாறிலி symp
                        = m_statep->செருகு_Sym(m_curSymp, "__paramNumber" + cvtToStr(m_param_இலக்கம்),
                                              nodep, m_classOrPackagep);
                    symp->exported(பொய்);
                }
            }
        }
    }
    வெறுமை visit(AstTypedef* nodep) override {  // FindVisitor::
        UASSERT_OBJ(m_curSymp, nodep, "Typedef not under module/package/$unit");
        எனில் (VN_IS(m_classOrPackagep, Class)) nodep->isUnderClass(உண்மை);
        iterateChildren(nodep);
        m_statep->செருகு_Sym(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
    }
    வெறுமை visit(AstTypedefFwd* nodep) override {  // FindVisitor::
        UASSERT_OBJ(m_curSymp, nodep, "Typedef not under module/package/$unit");
        iterateChildren(nodep);
        // No need to insert, only the real typedef matters, but need to track for errors
        nodep->user1p(m_curSymp);
    }
    வெறுமை visit(Ast_முனை_UOr_Struct_DType* nodep) override {  // FindVisitor::
        UASSERT_OBJ(m_curSymp, nodep, "Struct/union dtype not under module/package/$unit");
        VL_RESTORER(m_curSymp);
        m_curSymp = m_statep->செருகு_Block(m_curSymp, "__Vdtype" + cvtToStr(nodep->unique_இலக்கம்()),
                                          nodep, m_classOrPackagep);
        iterateChildren(nodep);
    }
    வெறுமை visit(AstMemberDType* nodep) override {  // FindVisitor::
        iterateChildren(nodep);
        m_statep->செருகு_Sym(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
    }
    வெறுமை visit(AstParamTypeDType* nodep) override {  // FindVisitor::
        UASSERT_OBJ(m_curSymp, nodep, "Parameter type not under module/package/$unit");

        // Replace missing param types with provided hierarchical type params.
        எனில் (!m_hierParamsName.empty()) {
            எனில் (மாறிலி VSymEnt* மாறிலி typedefEntp = m_curSymp->findIdFallback(m_hierParamsName)) {
                மாறிலி AstModule* modp = VN_CAST(typedefEntp->nodep(), Module);

                க்கு (மாறிலி Ast_முனை* stmtp = modp ? modp->stmtsp() : nullptr; stmtp;
                     stmtp = stmtp->nextp()) {
                    மாறிலி AstTypedef* tdefp = VN_CAST(stmtp, Typedef);

                    எனில் (tdefp && tdefp->name() == nodep->name() && m_statep->forPrimary()) {
                        UINFO(8, "Replacing type of" << nodep << "  -->with " << tdefp);
                        Ast_முனை_DType* மாறிலி newDtp = tdefp->childDTypep();
                        Ast_முனை_DType* மாறிலி oldDtp = nodep->childDTypep();

                        oldDtp->replaceWith(newDtp->clone_மரம்(பொய்));
                        VL_DO_DANGLING(oldDtp->delete_மரம்(), oldDtp);
                    }
                }
            }
        }

        iterateChildren(nodep);
        m_statep->செருகு_Sym(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
        எனில் (m_statep->forPrimary() && nodep->isGParam()) {
            ++m_param_இலக்கம்;
            VSymEnt* மாறிலி symp = m_statep->செருகு_Sym(
                m_curSymp, "__paramNumber" + cvtToStr(m_param_இலக்கம்), nodep, m_classOrPackagep);
            symp->exported(பொய்);
        }
    }
    வெறுமை visit(AstCFunc* nodep) override {  // FindVisitor::
        // For dotted resolution, ignore all AstVars under functions, otherwise shouldn't exist
        UASSERT_OBJ(!m_statep->forScopeCreation(), nodep, "No CFuncs expected in tree yet");
    }
    வெறுமை visit(AstEnumItem* nodep) override {  // FindVisitor::
        // EnumItem: Remember its name for later resolution
        iterateChildren(nodep);
        // Find under either a task or the module's vars
        மாறிலி VSymEnt* foundp = m_curSymp->findIdFallback(nodep->name());
        எனில் (!foundp && m_modSymp && nodep->name() == m_modSymp->nodep()->name()) {
            foundp = m_modSymp;  // Conflicts with modname?
        }
        AstEnumItem* மாறிலி findvarp = foundp ? VN_CAST(foundp->nodep(), EnumItem) : nullptr;
        தருக்க ins = பொய்;
        எனில் (!foundp) {
            ins = உண்மை;
        } இல்லையெனில் எனில் (findvarp != nodep) {
            UINFO(4, "DupVar: " << nodep << " ;; " << foundp);
            எனில் (foundp->parentp() == m_curSymp  // Only when on same level
                && !foundp->imported()) {  // and not from package
                nodep->v3error("Duplicate declaration of enum value: "
                               << nodep->prettyName() << '\n'
                               << nodep->warnContextPrimary() << '\n'
                               << foundp->nodep()->warnOther()
                               << "... Location of original declaration\n"
                               << foundp->nodep()->warnContextSecondary());
            } இல்லையெனில் {
                // User can disable the message at either point
                எனில் (!nodep->fileline()->warnIsOff(V3ErrorCode::VARHIDDEN)
                    && !foundp->nodep()->fileline()->warnIsOff(V3ErrorCode::VARHIDDEN)) {
                    nodep->v3warn(VARHIDDEN,
                                  "Declaration of enum value hides declaration in upper scope: "
                                      << nodep->prettyName() << '\n'
                                      << nodep->warnContextPrimary() << '\n'
                                      << foundp->nodep()->warnOther()
                                      << "... Location of original declaration\n"
                                      << foundp->nodep()->warnContextSecondary());
                }
                ins = உண்மை;
            }
        }
        எனில் (ins) m_statep->செருகு_Sym(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
    }
    வெறுமை visit(AstPackageImport* nodep) override {  // FindVisitor::
        UINFO(4, "  Link: " << nodep);
        எனில் (!nodep->packagep()) திரும்பு;  // Errored in V3LinkCells
        VSymEnt* மாறிலி srcp = m_statep->get_முனை_Sym(nodep->packagep());
        எனில் (nodep->name() == "*") {
            எனில் (nodep->packagep() != v3Global.rootp()->stdPackagep()) {
                எனில் (m_curSymp == m_statep->dunitEntp()) {
                    nodep->v3warn(IMPORTSTAR,
                                  "'import::*' in $unit scope may pollute global namespace");
                }
            }
        } இல்லையெனில் எனில் (!nodep->resolvedClassp()) {
            VSymEnt* மாறிலி impp = srcp->findIdFlat(nodep->name());
            எனில் (!impp) {
                nodep->v3error("Import object not found: " << nodep->prettyPkgNameQ());
            } இல்லையெனில் எனில் (nodep->resolvedClassp()) {
                UASSERT_OBJ(nodep->resolvedClassp()->classOrPackageSkipp() == impp->nodep(), nodep,
                            "Import should resolve to the same node");
            } இல்லையெனில் எனில் (AstClass* மாறிலி classp = VN_CAST(impp->nodep(), Class)) {
                nodep->resolvedClassp(
                    புதிய AstClassOrPackageRef{nodep->fileline(), classp->name(), classp, nullptr});
            }
        }
        m_curSymp->importFromPackage(m_statep->symsp(), srcp, nodep->name());
        UINFO(9, "    Link Done: " << nodep);
        // No longer needed, but can't delete until any multi-instantiated modules are expanded
    }
    வெறுமை visit(AstPackageExport* nodep) override {  // FindVisitor::
        UINFO(9, "  Link: " << nodep);
        எனில் (!nodep->packagep()) திரும்பு;  // Errored in V3LinkCells
        VSymEnt* மாறிலி srcp = m_statep->get_முனை_Sym(nodep->packagep());
        எனில் (nodep->name() != "*") {
            VSymEnt* மாறிலி impp = srcp->findIdFlat(nodep->name());
            எனில் (!impp) {
                nodep->v3error("Export object not found: '" << nodep->packagep()->prettyName()
                                                            << "::" << nodep->prettyName() << "'");
            }
        }
        m_curSymp->exportFromPackage(m_statep->symsp(), srcp, nodep->name());
        UINFO(9, "    Link Done: " << nodep);
        // No longer needed, but can't delete until any multi-instantiated modules are expanded
    }
    வெறுமை visit(AstPackageExportStarStar* nodep) override {  // FindVisitor::
        UINFO(4, "  Link: " << nodep);
        m_curSymp->exportStarStar(m_statep->symsp());
        // No longer needed, but can't delete until any multi-instantiated modules are expanded
    }

    வெறுமை visit(Ast_முனை_Foreach* nodep) override {  // FindVisitor::
        // Symbol table needs nodep->name() as the index variable's name
        VL_RESTORER(m_curSymp);
        {
            ++m_mod_With_இலக்கம்;
            m_curSymp = m_statep->செருகு_Block(m_curSymp, "__Vforeach" + cvtToStr(m_mod_With_இலக்கம்),
                                              nodep, m_classOrPackagep);
            m_curSymp->fallbackp(VL_RESTORER_PREV(m_curSymp));
            க்கு (Ast_முனை *nextp, *argp = nodep->headerp()->elementsp(); argp; argp = nextp) {
                nextp = argp->nextp();
                AstVar* argrefp = nullptr;
                எனில் (AstParseRef* மாறிலி parserefp = VN_CAST(argp, ParseRef)) {
                    // IEEE 1800-2023 12.7.3: foreach loop variable type shall be int (2-state)
                    // This might get changed in V3Width when types resolve (e.g., for assoc
                    // arrays)
                    argrefp = புதிய AstVar{parserefp->fileline(), VVarType::BLOCKTEMP,
                                         parserefp->name(), argp->findIntDType()};
                    argrefp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                    parserefp->replaceWith(argrefp);
                    VL_DO_DANGLING2(parserefp->delete_மரம்(), parserefp, argp);
                    // Insert argref's name into symbol table
                    m_statep->செருகு_Sym(m_curSymp, argrefp->name(), argrefp, nullptr);
                } இல்லையெனில் எனில் (மாறிலி auto largrefp = VN_CAST(argp, Var)) {
                    argrefp = largrefp;
                    // Insert argref's name into symbol table
                    m_statep->செருகு_Sym(m_curSymp, argrefp->name(), argrefp, nullptr);
                } இல்லையெனில் எனில் (VN_IS(argp, Empty)) {
                } இல்லையெனில் {
                    argp->v3error("'foreach' loop variable expects simple variable name");
                }
            }
            iterateChildren(nodep);
        }
    }

    வெறுமை visit(AstRandSequence* nodep) override {
        VL_RESTORER(m_curSymp);
        m_curSymp = m_statep->செருகு_Block(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
        iterateChildren(nodep);
    }
    வெறுமை visit(AstRSProd* nodep) override {
        VL_RESTORER(m_curSymp);
        m_curSymp = m_statep->செருகு_Block(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
        எனில் (nodep->fvarp())
            nodep->fvarp()->v3warn(E_UNSUPPORTED,
                                   "Unsupported: randsequence production function variable");
        // Mark formal ports as port-checked so the primary resolve pass does not
        // flag them with "does not appear in port list" -- V3RandSequence will
        // later move them onto the generated task as real input ports.
        க்கு (Ast_முனை* itp = nodep->portsp(); itp; itp = itp->nextp()) {
            VN_AS(itp, Var)->user4(உண்மை);
        }
        iterateChildren(nodep);
    }

    வெறுமை visit(AstWithParse* nodep) override {  // FindVisitor::
        // Change WITHPARSE(FUNCREF, equation) to FUNCREF(WITH(equation))
        Ast_முனை_FTask_Ref* funcrefp = VN_CAST(nodep->funcrefp(), முனை_FTask_Ref);
        எனில் (மாறிலி AstDot* மாறிலி dotp = VN_CAST(nodep->funcrefp(), Dot))
            funcrefp = VN_CAST(dotp->rhsp(), முனை_FTask_Ref);
        UASSERT_OBJ(funcrefp, nodep, "'with' only can operate on a function/task");
        string name = funcrefp->name() == "randomize" ? "__Vrandwith_obj" : "item";
        File_வரி* argFl = nodep->fileline();
        AstArg* மாறிலி argsp = funcrefp->argsp();
        எனில் (argsp) {
            argsp->unlinkFrBackWithNext();
            எனில் (funcrefp->name() != "randomize") {
                எனில் (மாறிலி auto parserefp = VN_CAST(argsp->exprp(), ParseRef)) {
                    name = parserefp->name();
                    argFl = parserefp->fileline();
                } இல்லையெனில் {
                    argsp->v3error("'with' function expects simple variable name");
                }
                எனில் (argsp->nextp()) {
                    argsp->nextp()->v3error("'with' function expects only up to one argument");
                }
                VL_DO_DANGLING(argsp->delete_மரம்(), argsp);
            }
        }
        // Type depends on the method used, let V3Width figure it out later
        // 'with (...) { ... }' constraint_block form is randomize-only
        // (IEEE 1800-2023 7.12 vs. 18.7); array methods take 'with (expr)'.
        எனில் (nodep->restricted() && funcrefp->name() != "randomize") {
            nodep->v3error("'with (...) { ... }' constraint block is only valid for"
                           " randomize() (IEEE 1800-2023 18.7)");
            funcrefp->addArgsp(argsp);
            nodep->replaceWith(nodep->funcrefp()->unlinkFrBack());
            VL_DO_DANGLING(nodep->delete_மரம்(), nodep);
            திரும்பு;
        }
        மாறிலி தருக்க restrictedRandomize = nodep->restricted();
        எனில் (nodep->exprsp() || nodep->constraintsp()
            || restrictedRandomize) {  // Else empty expression and pretend no "with"
            AstLambdaArgRef* மாறிலி indexArgRefp
                = புதிய AstLambdaArgRef{argFl, name + "__DOT__index", உண்மை};
            AstLambdaArgRef* மாறிலி மதிப்பு_Arg_Refp = புதிய AstLambdaArgRef{argFl, name, பொய்};
            AstWith* மாறிலி newp
                = புதிய AstWith{nodep->fileline(), indexArgRefp, மதிப்பு_Arg_Refp, nullptr};
            // Harvest the identifier_list into AstWith; grammar guarantees AstParseRef.
            எனில் (restrictedRandomize) {
                newp->restricted(உண்மை);
                போது (Ast_முனை* மாறிலி itemp = nodep->exprsp()) {
                    newp->addRestrictedName(VN_AS(itemp, ParseRef)->name());
                    itemp->unlinkFrBack();
                    VL_DO_DANGLING(தள்ளு_Deletep(itemp), itemp);
                }
            }
            Ast_முனை* கோவை_Or_Constraintsp = nullptr;
            எனில் (nodep->exprsp()) கோவை_Or_Constraintsp = nodep->exprsp()->unlinkFrBackWithNext();
            எனில் (nodep->constraintsp())
                கோவை_Or_Constraintsp = Ast_முனை::addNext(
                    கோவை_Or_Constraintsp, nodep->constraintsp()->unlinkFrBackWithNext());
            newp->addExprp(கோவை_Or_Constraintsp);  // addExprp() tolerates nullptr
            funcrefp->withp(newp);
        }
        funcrefp->addArgsp(argsp);
        nodep->replaceWith(nodep->funcrefp()->unlinkFrBack());
        VL_DO_DANGLING(nodep->delete_மரம்(), nodep);
    }
    வெறுமை visit(AstWith* nodep) override {  // FindVisitor::
        // Symbol table needs nodep->name() as the index variable's name
        // Iteration will pickup the AstVar we made under AstWith
        VL_RESTORER(m_curSymp);
        ++m_mod_With_இலக்கம்;
        m_curSymp = m_statep->செருகு_Block(m_curSymp, "__Vwith" + cvtToStr(m_mod_With_இலக்கம்), nodep,
                                          m_classOrPackagep);
        m_curSymp->fallbackp(VL_RESTORER_PREV(m_curSymp));
        UASSERT_OBJ(nodep->indexArgRefp(), nodep, "Missing lambda argref");
        UASSERT_OBJ(nodep->மதிப்பு_Arg_Refp(), nodep, "Missing lambda argref");
        // Insert argref's name into symbol table
        m_statep->செருகு_Sym(m_curSymp, nodep->மதிப்பு_Arg_Refp()->name(), nodep->மதிப்பு_Arg_Refp(),
                            nullptr);
        iterateChildren(nodep);
    }
    வெறுமை visit(AstCoverWith* nodep) override {  // FindVisitor::
        // The range list is outside the scope of the filter, which declares 'item'
        iterateAndNextNull(nodep->subp());
        VL_RESTORER(m_curSymp);
        ++m_mod_With_இலக்கம்;
        m_curSymp = m_statep->செருகு_Block(m_curSymp, "__VcoverWith" + cvtToStr(m_mod_With_இலக்கம்),
                                          nodep, m_classOrPackagep);
        m_curSymp->fallbackp(VL_RESTORER_PREV(m_curSymp));
        iterateAndNextNull(nodep->itemp());
        iterateAndNextNull(nodep->filterp());
    }
    வெறுமை visit(AstCoverCrossWith* nodep) override {  // FindVisitor::
        // The selection, the 'matches' count, and the cross items' types, of their coverpoints'
        // expressions with any nested scopes, are outside the filter's scope, which declares the
        // items.  As visit(AstVar) would resolve a type in the scope declaring its variable, the
        // items, generated and unique, are inserted directly.
        iterateAndNextNull(nodep->selectp());
        iterateAndNextNull(nodep->countp());
        க்கு (AstVar* varp = nodep->varsp(); varp; varp = VN_AS(varp->nextp(), Var)) {
            iterateAndNextNull(varp->childDTypep());
        }
        VL_RESTORER(m_curSymp);
        ++m_mod_With_இலக்கம்;
        m_curSymp = m_statep->செருகு_Block(m_curSymp, "__VcoverWith" + cvtToStr(m_mod_With_இலக்கம்),
                                          nodep, m_classOrPackagep);
        m_curSymp->fallbackp(VL_RESTORER_PREV(m_curSymp));
        க்கு (AstVar* varp = nodep->varsp(); varp; varp = VN_AS(varp->nextp(), Var)) {
            m_statep->செருகு_Sym(m_curSymp, varp->name(), varp, m_classOrPackagep);
        }
        iterateAndNextNull(nodep->filterp());
    }

    வெறுமை visit(Ast_முனை* nodep) override { iterateChildren(nodep); }  // FindVisitor::

    வெறுமை handleUnvisitedVirtIfaces() {
        Ast_முனை_Module* மாறிலி உச்சி = v3Global.rootp()->modulesp();
        m_curSymp = m_statep->get_முனை_Sym(உச்சி);
        க்கு (AstIface* ifacep : m_virtIfaces) {
            எனில் (!ifacep->user4()) {  // virtual interface never assigned any actual interface
                // Create dummy cell to keep the virtual interface alive
                // (later stages assume that non-dead interface has associated cell)
                AstCell* மாறிலி ifacecellp = புதிய AstCell{ifacep->fileline(),
                                                        ifacep->fileline(),
                                                        "__VdummyInstOf" + ifacep->name(),
                                                        ifacep->name(),
                                                        nullptr,
                                                        nullptr,
                                                        nullptr};
                ifacecellp->modp(ifacep);
                உச்சி->addStmtsp(ifacecellp);

                iterate(ifacecellp);
            }
        }
    }

பொது:
    // CONSTRUCTORS
    LinkDotFindVisitor(AstNetlist* rootp, LinkDotState* statep)
        : m_statep{statep} {
        UINFO(4, __FUNCTION__ << ": ");
        iterate(rootp);

        எனில் (!m_virtIfaces.empty()) handleUnvisitedVirtIfaces();
        க்கு (AstScope* மாறிலி scopep : m_scopes) scopep->modp()->addStmtsp(scopep);
        எனில் (m_statep->forScopeCreation()) {
            V3Stats::addStat("LinkDot, Scope visits", m_statScopeVisits);
        }
    }
    ~LinkDotFindVisitor() override = இயல்பு;
};

//======================================================================

வகுப்பு LinkDotFindIfaceVisitor final : பொது VNVisitor {
    // NODE STATE
    //  *::user1p()             -> See LinkDotState

    // STATE - for current visit position (use VL_RESTORER)
    LinkDotState* மாறிலி m_statep;  // State to pass between visitors, including symbol table
    Ast_முனை* m_declp = nullptr;  // Current declaring object that may soon contain IfaceRefDType

    // METHODS
    மாறிலி VSymEnt* findNonDeclSym(VSymEnt* symp, மாறிலி string& name) {
        // Find if there is a symbol of given name, ignoring the node that is declaring
        // it (m_declp) itself.  Thus if searching for "ifc" (an interface):
        //
        // module x;  // Finishes here and finds the typedef
        //   typedef foo ifc;
        //   ifc ifc;   // symp starts pointing here, but matches m_declp
        போது (symp) {
            மாறிலி VSymEnt* மாறிலி foundp = symp->findIdFlat(name);
            எனில் (foundp && foundp->nodep() != m_declp) திரும்பு foundp;
            symp = symp->fallbackp();
        }
        திரும்பு nullptr;
    }

    // VISITORS
    வெறுமை visit(AstRefDType* nodep) override {  // FindIfaceVisitor::
        எனில் (m_statep->forPrimary() && !nodep->classOrPackagep()) {
            UINFO(9, "  FindIfc: " << nodep);
            // If under a var, ignore the var itself as might be e.g. "intf intf;"
            // Critical tests:
            //   t_interface_param_genblk.v  // Checks this does make interface
            //   t_interface_hidden.v  // Checks this doesn't making interface when hidden
            எனில் (m_statep->exists_முனை_Sym(nodep)) {
                VSymEnt* symp = m_statep->get_முனை_Sym(nodep);
                மாறிலி VSymEnt* foundp = findNonDeclSym(symp, nodep->name());
                Ast_முனை* foundNodep = nullptr;
                // This: v4make test_regress/t/t_interface_param_genblk.py --debug
                // --debugi-V3LinkDot 9 Passes with this commented out:
                எனில் (foundp) foundNodep = foundp->nodep();
                எனில் (!foundNodep) foundNodep = m_statep->findModuleSym(nodep->name());
                எனில் (foundNodep) UINFO(9, "    Ifc foundNodep " << foundNodep);
                எனில் (AstIface* மாறிலி defp = VN_CAST(foundNodep, Iface)) {
                    // Must be found as module name, and not hidden/ by normal symbol (foundp)
                    AstIfaceRefDType* மாறிலி newp
                        = புதிய AstIfaceRefDType{nodep->fileline(), "", nodep->name()};
                    எனில் (nodep->paramsp())
                        newp->addParamsp(nodep->paramsp()->unlinkFrBackWithNext());
                    newp->ifacep(defp);
                    newp->user1u(nodep->user1u());
                    UINFO(9, "    Resolved interface " << nodep << "  =>  " << defp);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
                    திரும்பு;
                }
            }
        }
        iterateChildren(nodep);
    }
    வெறுமை visit(AstVar* nodep) override {  // FindIfaceVisitor::
        VL_RESTORER(m_declp);
        m_declp = nodep;
        iterateChildren(nodep);
        AstIfaceRefDType* மாறிலி ifacerefp = LinkDotState::ifaceRefFromArray(nodep->subDTypep());
        எனில் (ifacerefp && m_statep->exists_முனை_Sym(nodep)) {
            // Can't resolve until interfaces and modport names are
            // known; see notes at top
            UINFO(9, "  FindIfc Var IfaceRef " << ifacerefp);
            எனில் (!ifacerefp->isVirtual()) nodep->setIfaceRef();
            VSymEnt* மாறிலி symp = m_statep->get_முனை_Sym(nodep);
            m_statep->செருகு_Iface_Var_Sym(symp);
        }
    }
    வெறுமை visit(Ast_முனை* nodep) override { iterateChildren(nodep); }  // FindIfaceVisitor::

பொது:
    // CONSTRUCTORS
    LinkDotFindIfaceVisitor(AstNetlist* rootp, LinkDotState* statep)
        : m_statep{statep} {
        UINFO(4, __FUNCTION__ << ": ");
        iterate(rootp);
    }
    ~LinkDotFindIfaceVisitor() override = இயல்பு;
};

//======================================================================

வகுப்பு LinkDotParamVisitor final : பொது VNVisitor {
    // NODE STATE
    //  *::user1p()             -> See LinkDotState
    //  *::user2p()             -> See LinkDotState
    //  *::user4()              -> See LinkDotState

    // STATE
    LinkDotState* மாறிலி m_statep;  // State to pass between visitors, including symbol table
    Ast_முனை_Module* m_modp = nullptr;  // Current module

    வெறுமை pin_Implicit_கோவை_Recurse(Ast_முனை* nodep) {
        // Under a pin, Check interconnect expression for a pin reference or a concat.
        // Create implicit variable as needed
        எனில் (VN_IS(nodep, Dot)) {  // Not creating a simple implied type,
            // and implying something else would just confuse later errors
        } இல்லையெனில் எனில் (VN_IS(nodep, VarRef) || VN_IS(nodep, ParseRef)) {
            // To prevent user errors, we should only do single bit
            // implicit vars, however some netlists (MIPS) expect single
            // bit implicit wires to get created with range 0:0 etc.
            m_statep->implicitOkAdd(m_modp, nodep->name());
        }
        // These are perhaps a little too generous, as a SELect of siga[sigb]
        // perhaps shouldn't create an implicit variable.  But, we've warned...
        இல்லையெனில் {
            எனில் (Ast_முனை* மாறிலி refp = nodep->op1p()) pin_Implicit_கோவை_Recurse(refp);
            எனில் (Ast_முனை* மாறிலி refp = nodep->op2p()) pin_Implicit_கோவை_Recurse(refp);
            எனில் (Ast_முனை* மாறிலி refp = nodep->op3p()) pin_Implicit_கோவை_Recurse(refp);
            எனில் (Ast_முனை* மாறிலி refp = nodep->op4p()) pin_Implicit_கோவை_Recurse(refp);
        }
        // Continue along a list (e.g. the terminals under AstImplicit), also after a reference
        எனில் (Ast_முனை* மாறிலி refp = nodep->nextp()) pin_Implicit_கோவை_Recurse(refp);
    }

    // VISITORS
    வெறுமை visit(AstTypeTable*) override {}  // ParamVisitor::
    வெறுமை visit(Ast_முனை_Module* nodep) override {  // ParamVisitor::
        UINFO(5, "   " << nodep);
        எனில் ((nodep->dead() || !nodep->user4()) && !nodep->hierParams()) {
            UINFO(4, "Mark dead module " << nodep);
            UASSERT_OBJ(!m_statep->forScopeCreation(), nodep,
                        "Dead module persisted past where should have removed");
            // Don't remove now, because we may have a tree of
            // parameterized modules with VARXREFs into the deleted module
            // region.  V3Dead should cleanup.
            // Downstream visitors up until V3Dead need to check for nodep->dead.
            nodep->dead(உண்மை);
            திரும்பு;
        }
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
    }
    வெறுமை visit(AstPin* nodep) override {  // ParamVisitor::
        // Pin: Link to submodule's port
        // Deal with implicit definitions - do before Resolve visitor as may
        // be referenced above declaration
        எனில் (nodep->exprp()  // Else specifically unconnected pin
            && !nodep->svImplicit()) {  // SV 19.11.3: .name pins don't allow implicit decls
            pin_Implicit_கோவை_Recurse(nodep->exprp());
        }
    }
    வெறுமை visit(AstDefParam* nodep) override {  // ParamVisitor::
        iterateChildren(nodep);
        nodep->v3warn(DEFPARAM, "defparam is deprecated (IEEE 1800-2023 C.4.1)\n"
                                    << nodep->warnMore()
                                    << "... Suggest use instantiation with #(."
                                    << nodep->prettyName() << "(...etc...))");
        தருக்க hasPartSelect = பொய்;
        மாறிலி string path = extractDottedPath(nodep->pathp(), hasPartSelect);
        UASSERT_OBJ(!hasPartSelect && !path.empty(), nodep, "Unexpected defparam path shape");
        string baddot;
        VSymEnt* okSymp = nullptr;
        VSymEnt* மாறிலி foundp = m_statep->findDotted(
            nodep->fileline(), m_statep->get_முனை_Sym(nodep), path, baddot, okSymp, உண்மை);
        AstCell* மாறிலி cellp = foundp ? VN_AS(foundp->nodep(), Cell) : nullptr;
        எனில் (!cellp) {
            nodep->v3error("In defparam, instance " << path << " never declared");
        } இல்லையெனில் {
            Ast_முனை_கோவை* மாறிலி exprp = nodep->rhsp()->unlinkFrBack();
            UINFO(9, "Defparam cell " << path << "." << nodep->name() << " attach-to " << cellp
                                      << "  <= " << exprp);
            // Don't need to check the name of the defparam exists.  V3Param does.
            AstPin* மாறிலி pinp = புதிய AstPin{nodep->fileline(),
                                            -1,  // Pin# not relevant
                                            nodep->name(), exprp};
            pinp->param(உண்மை);
            pinp->paramPath(path);
            cellp->addParamsp(pinp);
        }
        VL_DO_DANGLING(nodep->unlinkFrBack()->delete_மரம்(), nodep);
    }
    வெறுமை visit(AstPort* nodep) override {  // ParamVisitor::
        // Port: Stash the pin number
        // Need to set pin numbers after varnames are created
        // But before we do the final resolution based on names
        VSymEnt* மாறிலி foundp = m_statep->get_முனை_Sym(m_modp)->findIdFlat(nodep->name());
        AstVar* மாறிலி refp = foundp ? VN_CAST(foundp->nodep(), Var) : nullptr;
        எனில் (!foundp) {
            nodep->v3error(
                "Input/output/inout declaration not found for port: " << nodep->prettyNameQ());
        } இல்லையெனில் எனில் (!refp || (!refp->isIO() && !refp->isIfaceRef())) {
            nodep->v3error("Pin is not an in/out/inout/interface: " << nodep->prettyNameQ());
        } இல்லையெனில் {
            எனில் (refp->user4()) {
                nodep->v3error("Duplicate declaration of port: "
                               << nodep->prettyNameQ() << '\n'
                               << nodep->warnContextPrimary() << '\n'
                               << refp->warnOther() << "... Location of original declaration\n"
                               << refp->warnContextSecondary());
            }
            refp->user4(உண்மை);
            VSymEnt* மாறிலி symp = m_statep->செருகு_Sym(m_statep->get_முனை_Sym(m_modp),
                                                      "__pinNumber" + cvtToStr(nodep->pin_இலக்கம்()),
                                                      refp, nullptr /*classOrPackagep*/);
            symp->exported(பொய்);
            refp->pin_இலக்கம்(nodep->pin_இலக்கம்());
            // Put the variable where the port is, so that variables stay
            // in pin number sorted order. Otherwise hierarchical or JSON
            // may botch by-position instances.
            nodep->addHereThisAsNext(refp->unlinkFrBack());
        }
        // Ports not needed any more
        VL_DO_DANGLING(nodep->unlinkFrBack()->delete_மரம்(), nodep);
    }
    வெறுமை visit(AstAssignW* nodep) override {  // ParamVisitor::
        // Deal with implicit definitions
        // We used to nodep->allowImplicit() here, but it turns out
        // normal "assigns" can also make implicit wires.  Yuk.
        pin_Implicit_கோவை_Recurse(nodep->lhsp());
        iterateChildren(nodep);
    }

    வெறுமை visit(AstImplicit* nodep) override {  // ParamVisitor::
        // Unsupported gates need implicit creation
        pin_Implicit_கோவை_Recurse(nodep->exprsp());
        // We're done with implicit gates
        VL_DO_DANGLING(nodep->unlinkFrBack()->delete_மரம்(), nodep);
    }
    வெறுமை visit(AstClassOrPackageRef* nodep) override {  // ParamVisitor::
        எனில் (auto* மாறிலி fwdp = VN_CAST(nodep->classOrPackageNodep(), TypedefFwd)) {
            // Relink forward definitions to the "real" definition
            VSymEnt* மாறிலி foundp = m_statep->get_முனை_Sym(fwdp)->findIdFallback(fwdp->name());
            எனில் (foundp && (VN_IS(foundp->nodep(), Class) || VN_IS(foundp->nodep(), Package))) {
                nodep->classOrPackagep(VN_AS(foundp->nodep(), முனை_Module));
            } இல்லையெனில் எனில் (foundp && VN_IS(foundp->nodep(), ParamTypeDType)) {
                UASSERT(m_statep->forPrimary(), "Param types should have been resolved");
                nodep->classOrPackageNodep(VN_AS(foundp->nodep(), ParamTypeDType));
            } இல்லையெனில் {
                எனில் (foundp) UINFO(1, "found nodep = " << foundp->nodep());
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
    வெறுமை visit(AstPull* nodep) override {  // ParamVisitor::
        // Deal with implicit definitions
        // We used to nodep->allowImplicit() here, but it turns out
        // normal "assigns" can also make implicit wires.  Yuk.
        pin_Implicit_கோவை_Recurse(nodep->lhsp());
        iterateChildren(nodep);
    }
    வெறுமை visit(AstTypedefFwd* nodep) override {  // ParamVisitor::
        VSymEnt* மாறிலி foundp = m_statep->get_முனை_Sym(nodep)->findIdFallback(nodep->name());
        எனில் (foundp) {
            // If the typedef was earlier in source order (tokenNum), then remember that earlier
            // point to avoid error something wasn't declared
            // Might be forward declaring something odd (with declTokenNumSetMin not implemented,
            // but should be harmless to ignore as this is just for error detection
            foundp->nodep()->decl_Token_இலக்கம்_Set_Min(nodep->fileline()->token_இலக்கம்());
        } இல்லையெனில் எனில் (v3Global.opt.pedantic()) {
            // We only check it was ever really defined in pedantic mode, as it
            // might have been in a header file referring to a module we never
            // needed so there are false positives
            nodep->v3error(
                "Forward typedef unused or does not resolve to a data type (IEEE 1800-2023 6.18): "
                << nodep->prettyNameQ());
        }
        // We only needed the forward declaration in order to parse correctly.
        // Delete later as may be ClassOrPackageRef's still pointing to it
        VL_DO_DANGLING(தள்ளு_Deletep(nodep->unlinkFrBack()), nodep);
    }

    வெறுமை visit(Ast_முனை* nodep) override { iterateChildren(nodep); }  // ParamVisitor::

பொது:
    // CONSTRUCTORS
    LinkDotParamVisitor(AstNetlist* rootp, LinkDotState* statep)
        : m_statep{statep} {
        UINFO(4, __FUNCTION__ << ": ");
        iterate(rootp);
    }
    ~LinkDotParamVisitor() override = இயல்பு;
};

//======================================================================

வகுப்பு LinkDotScopeVisitor final : பொது VNVisitor {

    // STATE
    LinkDotState* மாறிலி m_statep;  // State to pass between visitors, including symbol table
    மாறிலி AstScope* m_scopep = nullptr;  // The current scope
    VSymEnt* m_modSymp = nullptr;  // Symbol entry for current module
    // Deferred AliasScope processing - must be done outer-to-inner for correct alias resolution
    std::vector<std::pair<AstAliasScope*, VSymEnt*>> m_deferredAliasScopes;

    // METHODS
பொது:
    // getAlias with setVarAlias and setIfaceAlias implement disjoint-set data structure.
    // This algorithm is needed for the case when multiple alias statements
    // reference partially the same variables.
    நிலையான AstVarScope* getAlias(AstVarScope* மாறிலி vscp) {
        எனில் (vscp->user2p() && vscp != vscp->user2p()) {
            AstVarScope* மாறிலி aliasp = getAlias(VN_AS(vscp->user2p(), VarScope));
            vscp->user2p(aliasp);
            திரும்பு aliasp;
        } இல்லையெனில் {
            திரும்பு vscp;
        }
    }

தனிப்பட்ட:
    // Alias an interface reference to the interface it is connected to
    வெறுமை setIfaceAlias(AstVarScope* மாறிலி vscp, AstVarScope* மாறிலி aliasp) {
        UASSERT_OBJ(vscp->varp()->isIfaceRef(), vscp, "Interface alias of non-interface");
        UASSERT_OBJ(aliasp->varp()->isIfaceRef(), aliasp, "Interface alias of non-interface");
        getAlias(vscp)->user2p(getAlias(aliasp));
    }

    // Alias two variables. An output port in the lower scope survives, unless the other
    // must keep its own storage.
    வெறுமை setVarAlias(AstVarScope* ap, AstVarScope* bp) {
        UASSERT_OBJ(!ap->varp()->isIfaceRef(), ap, "Variable alias of interface");
        UASSERT_OBJ(!bp->varp()->isIfaceRef(), bp, "Variable alias of interface");
        ap = getAlias(ap);
        bp = getAlias(bp);
        எனில் (ap == bp) திரும்பு;
        மாறிலி தருக்க aWins = [&]() {
            மாறிலி AstVar* மாறிலி aVarp = ap->varp();
            மாறிலி AstVar* மாறிலி bVarp = bp->varp();
            // A primary IO is accessed directly by the user of the model, so must survive
            UASSERT_OBJ(!aVarp->isPrimaryIO() || !bVarp->isPrimaryIO(), ap,
                        "Alias of two primary IOs");
            எனில் (bVarp->isPrimaryIO()) திரும்பு பொய்;
            எனில் (aVarp->isPrimaryIO()) திரும்பு உண்மை;
            // A forced or public variable must keep its own storage. If both are, either works.
            எனில் (aVarp->isForced() != bVarp->isForced()) திரும்பு aVarp->isForced();
            எனில் (aVarp->isSigPublic() != bVarp->isSigPublic()) திரும்பு aVarp->isSigPublic();
            // Otherwise prefer an output port in the lower scope. This enables better V3Combine
            // due to having fewer upward hierarchical references (which prevent combining).
            எனில் (aVarp->direction() != VDirection::OUTPUT) திரும்பு பொய்;
            // Is 'ap' strictly below 'bp' in the hierarchy
            க்கு (மாறிலி AstScope* scopep = ap->scopep()->aboveScopep(); scopep;
                 scopep = scopep->aboveScopep()) {
                எனில் (scopep == bp->scopep()) திரும்பு உண்மை;
            }
            திரும்பு பொய்;
        }();
        எனில் (aWins) {
            bp->user2p(ap);
        } இல்லையெனில் {
            ap->user2p(bp);
        }
    }

    // VISITORS
    வெறுமை visit(AstNetlist* nodep) override {  // ScopeVisitor::
        // Recurse..., backward as must do packages before using packages
        iterateChildrenBackwardsConst(nodep);
    }
    வெறுமை visit(AstScope* nodep) override {  // ScopeVisitor::
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
        எனில் (AstIface* மாறிலி ifacep = VN_CAST(nodep->modp(), Iface)) {
            m_statep->செருகு_Iface_Mod_Sym(ifacep, m_modSymp);
        }
        m_scopep = nodep;
        iterateChildren(nodep);
    }
    வெறுமை visit(AstVarScope* nodep) override {  // ScopeVisitor::
        எனில் (!nodep->varp()->isFuncLocal() && !nodep->varp()->isClassMember()) {
            VSymEnt* மாறிலி varSymp
                = m_statep->செருகு_Sym(m_modSymp, nodep->varp()->name(), nodep, nullptr);
            எனில் (nodep->varp()->isIfaceRef()) {
                AstIfaceRefDType* மாறிலி dtypep
                    = LinkDotState::ifaceRefFromArray(nodep->varp()->dtypep());
                UASSERT_OBJ(dtypep, nodep, "Non AstIfaceRefDType on isIfaceRef() var");
                எனில் (nodep->varp()->isIfaceParent()) {
                    UINFO(9, "Iface parent ref var " << nodep->varp()->name() << " " << nodep);
                    // Find the interface cell the var references
                    UINFO(9, "Iface parent dtype " << dtypep);
                    மாறிலி string ifcellname = dtypep->cellName();
                    string baddot;
                    VSymEnt* okSymp;
                    VSymEnt* cellSymp = m_statep->findDotted(nodep->fileline(), m_modSymp,
                                                             ifcellname, baddot, okSymp, பொய்);
                    UASSERT_OBJ(
                        cellSymp, nodep,
                        "No symbol for interface instance: " << nodep->prettyNameQ(ifcellname));
                    UINFO(5, "       Found interface instance: se" << cvtToHex(cellSymp) << " "
                                                                   << cellSymp->nodep());
                    எனில் (dtypep->modportName() != "") {
                        // Look up the modport within the interface cell's symbol table
                        VSymEnt* மாறிலி mpSymp = cellSymp->findIdFallback(dtypep->modportName());
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
                    m_statep->செருகு_Scope_Alias(LinkDotState::SAMN_IFTOP, varSymp, cellSymp);
                }
            }
        }
    }
    வெறுமை visit(Ast_முனை_FTask* nodep) override {  // ScopeVisitor::
        VSymEnt* மாறிலி symp = m_statep->செருகு_Block(m_modSymp, nodep->name(), nodep, nullptr);
        symp->fallbackp(m_modSymp);
        iterateChildren(nodep);
    }
    வெறுமை visit(Ast_முனை_Foreach* nodep) override {  // ScopeVisitor::
        VSymEnt* மாறிலி symp = m_statep->செருகு_Block(m_modSymp, nodep->name(), nodep, nullptr);
        symp->fallbackp(m_modSymp);
        // No recursion, we don't want to pick up variables
    }
    வெறுமை visit(AstConstraintForeach* nodep) override {  // ScopeVisitor::
        iterateChildren(nodep);
    }
    வெறுமை visit(AstWith* nodep) override {  // ScopeVisitor::
        VSymEnt* மாறிலி symp = m_statep->செருகு_Block(m_modSymp, nodep->name(), nodep, nullptr);
        symp->fallbackp(m_modSymp);
        iterateChildren(nodep);
    }
    வெறுமை visit(AstAlias* nodep) override {  // ScopeVisitor::
        // Track aliases
        UINFOTREE(9, nodep, "", "alias");
        AstVarScope* aliasVscp = nullptr;
        க்கு (Ast_முனை* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
            AstVarScope* vscp = nullptr;
            எனில் (மாறிலி AstVarRef* மாறிலி refp = VN_CAST(itemp, VarRef)) {
                vscp = refp->varScopep();
            } இல்லையெனில் {
                // Reaches into the scope of an instance, look it up by name
                மாறிலி AstVarXRef* மாறிலி xrefp = VN_AS(itemp, VarXRef);
                மாறிலி string scopename = xrefp->dotted() + "." + xrefp->name();
                string baddot;
                VSymEnt* okSymp;
                VSymEnt* மாறிலி symp = m_statep->findDotted(xrefp->fileline(), m_modSymp, scopename,
                                                           baddot, okSymp, பொய்);
                UASSERT_OBJ(symp, nodep, "No symbol for alias item: " << scopename);
                vscp = VN_CAST(symp->nodep(), VarScope);
            }
            UASSERT_OBJ(vscp, nodep, "VarScope unset");
            எனில் (aliasVscp) {
                setVarAlias(aliasVscp, vscp);
            } இல்லையெனில் {
                aliasVscp = vscp;
            }
        }
        iterateChildren(nodep);
        VL_DO_DANGLING(தள்ளு_Deletep(nodep->unlinkFrBack()), nodep);
    }
    வெறுமை visit(AstAliasScope* nodep) override {  // ScopeVisitor::
        // Defer AliasScope processing - must process outer scopes before inner ones
        // so that nested interface port alias resolution works correctly
        UINFO(5, "ALIASSCOPE (deferred) " << nodep);
        m_deferredAliasScopes.emplace_back(nodep, m_modSymp);
    }
    வெறுமை processAliasScope(AstAliasScope* nodep, VSymEnt* modSymp) {
        UINFO(5, "ALIASSCOPE  " << nodep);
        UINFOTREE(9, nodep, "", "avs");
        VSymEnt* rhsSymp;
        {
            AstVarRef* மாறிலி refp = VN_CAST(nodep->rhsp(), VarRef);
            AstVarXRef* மாறிலி xrefp = VN_CAST(nodep->rhsp(), VarXRef);
            UASSERT_OBJ(refp || xrefp, nodep,
                        "Unsupported: Non Var(X)Ref attached to interface pin");
            string inl
                = ((xrefp && xrefp->inlinedDots().size()) ? (xrefp->inlinedDots() + "__DOT__")
                                                          : "");
            மாறிலி string dottedPath
                = (xrefp && !xrefp->dotted().empty()) ? (xrefp->dotted() + ".") : "";
            VSymEnt* symp = nullptr;
            string scopename;
            போது (!symp) {
                scopename = refp ? refp->name()
                                 : (inl.size() ? (inl + dottedPath + xrefp->name())
                                               : (dottedPath + xrefp->name()));
                string baddot;
                VSymEnt* okSymp;
                symp = m_statep->findDotted(nodep->rhsp()->fileline(), modSymp, scopename, baddot,
                                            okSymp, உண்மை);
                எனில் (inl == "") நிறுத்து;
                inl = LinkDotState::removeLastInlineScope(inl);
            }
            UASSERT_OBJ(symp, nodep, "No symbol for interface alias rhs");
            UINFO(5, "       Found a linked scope RHS: " << scopename << "  se" << cvtToHex(symp)
                                                         << " " << symp->nodep());
            rhsSymp = symp;
        }
        VSymEnt* lhsSymp;
        {
            மாறிலி AstVarXRef* மாறிலி xrefp = VN_CAST(nodep->lhsp(), VarXRef);
            மாறிலி AstVarRef* மாறிலி refp = VN_CAST(nodep->lhsp(), VarRef);

            UASSERT_OBJ(refp || xrefp, nodep,
                        "Unsupported: Non Var(X)Ref attached to interface pin");
            மாறிலி string scopename
                = refp ? refp->varp()->name() : xrefp->dotted() + "." + xrefp->name();
            string baddot;
            VSymEnt* okSymp;
            VSymEnt* மாறிலி symp = m_statep->findDotted(nodep->lhsp()->fileline(), modSymp,
                                                       scopename, baddot, okSymp, பொய்);
            UASSERT_OBJ(symp, nodep, "No symbol for interface alias lhs");
            UINFO(5, "       Found a linked scope LHS: " << scopename << "  se" << cvtToHex(symp)
                                                         << " " << symp->nodep());
            lhsSymp = symp;
        }
        // Remember the alias - can't do it yet because we may have additional symbols to be added,
        // or maybe an alias of an alias
        m_statep->செருகு_Scope_Alias(LinkDotState::SAMN_IFTOP, lhsSymp, rhsSymp);
        AstVarScope* மாறிலி lhsVscp = VN_CAST(lhsSymp->nodep(), VarScope);
        AstVarScope* மாறிலி rhsVscp = VN_CAST(rhsSymp->nodep(), VarScope);
        UASSERT_OBJ(lhsVscp && rhsVscp, nodep, "Interface alias missing variable scope");
        setIfaceAlias(lhsVscp, rhsVscp);
        // We have stored the link, we don't need these any more
        VL_DO_DANGLING(nodep->unlinkFrBack()->delete_மரம்(), nodep);
    }
    வெறுமை processDeferredAliasScopes() {
        // Sort by hierarchy depth (shallower first) so outer aliases are resolved before inner
        // Pre-compute depth map to avoid O(N log N * D) complexity in sort comparisons
        std::unordered_map<VSymEnt*, முழு> depthMap;
        க்கு (மாறிலி auto& pair : m_deferredAliasScopes) {
            VSymEnt* மாறிலி symp = pair.second;
            எனில் (depthMap.find(symp) == depthMap.end()) {
                முழு depth = 0;
                க்கு (VSymEnt* p = symp; p; p = p->parentp()) ++depth;
                depthMap[symp] = depth;
            }
        }
        std::stable_sort(m_deferredAliasScopes.begin(), m_deferredAliasScopes.end(),
                         [&depthMap](மாறிலி std::pair<AstAliasScope*, VSymEnt*>& a,
                                     மாறிலி std::pair<AstAliasScope*, VSymEnt*>& b) {
                             திரும்பு depthMap.at(a.second) < depthMap.at(b.second);
                         });
        // Process in sorted order
        க்கு (auto& pair : m_deferredAliasScopes) { processAliasScope(pair.first, pair.second); }
        m_deferredAliasScopes.clear();
    }
    வெறுமை visit(Ast_முனை_Gen* nodep) override {  // ScopeVisitor::  // LCOV_EXCL_LINE
        nodep->v3fatalSrc("Generate constructs should have been reduced out");
    }
    // For speed, don't recurse things that can't have scope
    // Note we allow AstNodeStmt's as generates may be under them
    வெறுமை visit(AstCell*) override {}  // ScopeVisitor::
    வெறுமை visit(AstVar*) override {}  // ScopeVisitor::
    வெறுமை visit(Ast_முனை* nodep) override { iterateChildren(nodep); }  // ScopeVisitor::

பொது:
    // CONSTRUCTORS
    LinkDotScopeVisitor(AstNetlist* rootp, LinkDotState* statep)
        : m_statep{statep} {
        UINFO(4, __FUNCTION__ << ": ");
        iterate(rootp);
        // Process deferred AliasScopes in outer-to-inner order
        processDeferredAliasScopes();
    }
    ~LinkDotScopeVisitor() override = இயல்பு;
};

//======================================================================

// Iterate an interface to resolve modports
வகுப்பு LinkDotIfaceVisitor final : பொது VNVisitor {
    // STATE
    LinkDotState* மாறிலி m_statep;  // State to pass between visitors, including symbol table
    VSymEnt* m_curSymp;  // Symbol Entry for current table, where to lookup/insert

    // VISITORS
    வெறுமை visit(AstModport* nodep) override {  // IfaceVisitor::
        // Modport: Remember its name for later resolution
        UINFO(5, "   fiv: " << nodep);
        VL_RESTORER(m_curSymp);
        {
            // Create symbol table for the vars
            m_curSymp = m_statep->செருகு_Block(m_curSymp, nodep->name(), nodep, nullptr);
            m_curSymp->fallbackp(VL_RESTORER_PREV(m_curSymp));
            iterateChildren(nodep);
        }
    }
    வெறுமை visit(AstModportFTaskRef* nodep) override {  // IfaceVisitor::
        UINFO(5, "   fif: " << nodep);
        iterateChildren(nodep);
        VSymEnt* மாறிலி symp = m_curSymp->findIdFallback(nodep->name());
        எனில் (!symp) {
            nodep->v3error("Modport item not found: " << nodep->prettyNameQ());
        } இல்லையெனில் எனில் (Ast_முனை_FTask* மாறிலி ftaskp = VN_CAST(symp->nodep(), முனை_FTask)) {
            // Make symbol under modport that points at the _interface_'s var, not the modport.
            nodep->ftaskp(ftaskp);
            VSymEnt* மாறிலி subSymp
                = m_statep->செருகு_Sym(m_curSymp, nodep->name(), ftaskp, nullptr /*package*/);
            m_statep->செருகு_Scope_Alias(LinkDotState::SAMN_MODPORT, subSymp, symp);
        } இல்லையெனில் {
            nodep->v3error("Modport item is not a function/task: " << nodep->prettyNameQ());
        }
        எனில் (m_statep->forScopeCreation()) {
            // Done with AstModportFTaskRef.
            // Delete to prevent problems if we dead-delete pointed to ftask
            nodep->unlinkFrBack();
            VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
        }
    }

    // Helper to resolve remaining path through a nested interface
    // When findDotted() partially matches (okSymp set, baddot non-empty),
    // this follows the interface type to resolve the remaining path.
    // Returns the resolved symbol, or nullptr if not found.
    // On success, clears baddot; on partial match of multi-level path, updates baddot.
    VSymEnt* resolveNestedInterfacePath(File_வரி* fl, VSymEnt* okSymp, string& baddot) {
        எனில் (!okSymp || baddot.empty()) திரும்பு nullptr;

        நிலையான constexpr முழு MAX_NESTING_DEPTH = 64;
        VSymEnt* curOkSymp = okSymp;

        க்கு (முழு depth = 0; depth < MAX_NESTING_DEPTH; ++depth) {
            // Try to get interface from the partially-matched symbol
            AstIface* ifacep = nullptr;
            எனில் (மாறிலி AstCell* மாறிலி cellp = VN_CAST(curOkSymp->nodep(), Cell)) {
                ifacep = VN_CAST(cellp->modp(), Iface);
            } இல்லையெனில் எனில் (மாறிலி AstVar* மாறிலி varp = VN_CAST(curOkSymp->nodep(), Var)) {
                எனில் (varp->isIfaceRef()) {
                    எனில் (மாறிலி AstIfaceRefDType* மாறிலி ifaceRefp
                        = LinkDotState::ifaceRefFromArray(varp->dtypep())) {
                        ifacep = ifaceRefp->ifaceViaCellp();
                    }
                }
            }

            எனில் (!ifacep || !m_statep->exists_முனை_Sym(ifacep)) திரும்பு nullptr;

            VSymEnt* மாறிலி ifaceSymp = m_statep->get_முனை_Sym(ifacep);

            எனில் (baddot.find('.') == string::npos) {
                // Simple identifier - direct lookup
                VSymEnt* மாறிலி symp = ifaceSymp->findIdFallback(baddot);
                எனில் (symp) baddot.clear();
                திரும்பு symp;
            }

            // Multi-level path - use findDotted for partial resolution
            string remainingBaddot;
            VSymEnt* remainingOkSymp = nullptr;
            VSymEnt* மாறிலி symp = m_statep->findDotted(fl, ifaceSymp, baddot, remainingBaddot,
                                                       remainingOkSymp, உண்மை);
            எனில் (symp) {
                baddot = remainingBaddot;
                திரும்பு symp;
            }

            // findDotted partially matched - check progress
            எனில் (remainingBaddot == baddot || !remainingOkSymp) திரும்பு nullptr;

            // Continue resolving with updated state
            baddot = remainingBaddot;
            curOkSymp = remainingOkSymp;
        }

        UINFO(1, "Nested interface resolution depth limit exceeded at " << fl);
        திரும்பு nullptr;
    }

    // Resolve a modport expression to find the referenced symbol
    VSymEnt* resolveModportExpression(File_வரி* fl, Ast_முனை_கோவை* exprp) {
        UINFO(5, "     resolveModportExpression: " << exprp);
        VSymEnt* symp = nullptr;
        எனில் (AstParseRef* மாறிலி refp = VN_CAST(exprp, ParseRef)) {
            // Simple variable reference: modport mp(input .a(sig_a))
            symp = m_curSymp->findIdFallback(refp->name());
        } இல்லையெனில் எனில் (AstVarRef* மாறிலி refp = VN_CAST(exprp, VarRef)) {
            // Already resolved VarRef (can happen if iterateChildren resolved it)
            எனில் (refp->varScopep()) {
                // During scope creation, VarRef may have VarScope already linked
                symp = m_statep->get_முனை_Sym(refp->varScopep());
            } இல்லையெனில் எனில் (refp->varp()) {
                symp = m_curSymp->findIdFallback(refp->varp()->name());
            }
        } இல்லையெனில் எனில் (AstVarXRef* மாறிலி refp = VN_CAST(exprp, VarXRef)) {
            // Resolved VarXRef (dotted reference resolved by earlier pass)
            எனில் (refp->varp()) {
                // For nested interfaces, the var is in a different scope
                // First try to find the symbol via the var itself
                எனில் (m_statep->exists_முனை_Sym(refp->varp())) {
                    symp = m_statep->get_முனை_Sym(refp->varp());
                } இல்லையெனில் {
                    // Fallback: look up in current scope
                    symp = m_curSymp->findIdFallback(refp->varp()->name());
                }
            } இல்லையெனில் எனில் (!refp->dotted().empty()) {
                // varp not set yet - use dotted path to find the symbol
                // The dotted part (e.g., "base") is the interface cell, and name is the member
                மாறிலி string fullPath = refp->dotted() + "." + refp->name();
                string baddot;
                VSymEnt* okSymp = nullptr;
                symp = m_statep->findDotted(fl, m_curSymp, fullPath, baddot, okSymp, உண்மை);
                // Handle nested interface path when findDotted had partial match
                எனில் (okSymp && !baddot.empty()) {
                    VSymEnt* மாறிலி resolved = resolveNestedInterfacePath(fl, okSymp, baddot);
                    எனில் (resolved) symp = resolved;
                }
            }
        } இல்லையெனில் எனில் (AstDot* மாறிலி dotp = VN_CAST(exprp, Dot)) {
            // Dotted path: modport mp(input .a(inner.sig))
            தருக்க hasPartSelect = பொய்;
            மாறிலி string dottedPath = extractDottedPath(dotp, hasPartSelect);
            எனில் (hasPartSelect) {
                fl->v3warn(
                    E_UNSUPPORTED,
                    "Unsupported: Modport expression with part select (IEEE 1800-2023 25.5.4)");
            } இல்லையெனில் {
                string baddot;
                VSymEnt* okSymp = nullptr;
                symp = m_statep->findDotted(fl, m_curSymp, dottedPath, baddot, okSymp, உண்மை);

                // Handle nested interface path when findDotted had partial match
                எனில் (okSymp && !baddot.empty()) {
                    VSymEnt* மாறிலி resolved = resolveNestedInterfacePath(fl, okSymp, baddot);
                    எனில் (resolved) symp = resolved;
                }

                எனில் (!symp || !baddot.empty()) {
                    fl->v3error("Can't find modport expression target: "
                                << Ast_முனை::prettyNameQ(dottedPath));
                    symp = nullptr;
                }
            }
        } இல்லையெனில் எனில் (VN_IS(exprp, SelBit) || VN_IS(exprp, SelExtract)) {
            // Part select expressions not yet supported
            fl->v3warn(E_UNSUPPORTED,
                       "Unsupported: Modport expression with part select (IEEE 1800-2023 25.5.4)");
        } இல்லையெனில் {
            // Other expression types not supported
            fl->v3warn(E_UNSUPPORTED,
                       "Unsupported: Complex modport expression (IEEE 1800-2023 25.5.4)");
        }
        திரும்பு symp;
    }

    வெறுமை visit(AstModportVarRef* nodep) override {  // IfaceVisitor::
        UINFO(5, "   fiv: " << nodep);
        iterateChildren(nodep);
        VSymEnt* symp = nullptr;
        எனில் (nodep->exprp()) {
            // Modport expression syntax: modport mp(input .port_name(expression))
            symp = resolveModportExpression(nodep->fileline(), nodep->exprp());
        } இல்லையெனில் {
            symp = m_curSymp->findIdFallback(nodep->name());
        }
        எனில் (!symp) {
            nodep->v3error("Modport item not found: " << nodep->prettyNameQ());
        } இல்லையெனில் எனில் (AstVar* மாறிலி varp = VN_CAST(symp->nodep(), Var)) {
            // Make symbol under modport that points at the _interface_'s var via the modport.
            // (Need modport still to test input/output markings)
            nodep->varp(varp);
            எனில் (nodep->exprp()) {
                // For modport expressions, insert symbol pointing to the underlying var (not the
                // ModportVarRef which will be deleted during scope creation). The virtual port
                // name maps to the real signal's var.
                VSymEnt* மாறிலி subSymp
                    = m_statep->செருகு_Sym(m_curSymp, nodep->name(), varp, nullptr /*package*/);
                m_statep->செருகு_Scope_Alias(LinkDotState::SAMN_MODPORT, subSymp, symp);
            } இல்லையெனில் {
                // For regular modport items, insert symbol pointing to ModportVarRef for
                // input/output marking tests.
                m_statep->செருகு_Sym(m_curSymp, nodep->name(), nodep, nullptr /*package*/);
            }
        } இல்லையெனில் எனில் (AstVarScope* மாறிலி vscp = VN_CAST(symp->nodep(), VarScope)) {
            // Make symbol under modport that points at the _interface_'s var, not the modport.
            nodep->varp(vscp->varp());
            // Only insert symbol for modport expression virtual ports (exprp set).
            // For regular modport items, don't insert into the shared modport table because
            // each instance would overwrite the previous one, causing wrong VarScope lookups.
            // Regular items are found via the modport's fallback to the interface.
            எனில் (nodep->exprp()) {
                VSymEnt* மாறிலி subSymp
                    = m_statep->செருகு_Sym(m_curSymp, nodep->name(), vscp, nullptr /*package*/);
                m_statep->செருகு_Scope_Alias(LinkDotState::SAMN_MODPORT, subSymp, symp);
            }
        } இல்லையெனில் {
            nodep->v3error("Modport item is not a variable: " << nodep->prettyNameQ());
        }
        எனில் (m_statep->forScopeCreation()) {
            // Done with AstModportVarRef.
            // Delete to prevent problems if we dead-delete pointed to variable
            nodep->unlinkFrBack();
            VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
        }
    }
    வெறுமை visit(AstModportClockingRef* nodep) override {  // IfaceVisitor::
        UINFO(5, "   fic: " << nodep);
        iterateChildren(nodep);
        VSymEnt* மாறிலி symp = m_curSymp->findIdFallback(nodep->name());
        எனில் (!symp) {
            nodep->v3error("Modport item not found: " << nodep->prettyNameQ());
        } இல்லையெனில் எனில் (AstClocking* மாறிலி clockingp = VN_CAST(symp->nodep(), Clocking)) {
            nodep->clockingp(clockingp);
            m_statep->செருகு_Sym(m_curSymp, nodep->name(), nodep, nullptr /*package*/);
        } இல்லையெனில் {
            nodep->v3error("Modport item is not a clocking block: " << nodep->prettyNameQ());
        }
    }
    வெறுமை visit(AstScope* nodep) override {  // IfaceVisitor::
        எனில் (nodep->user3SetOnce()) திரும்பு;
        iterateChildren(nodep);
    }
    வெறுமை visit(Ast_முனை* nodep) override { iterateChildren(nodep); }  // IfaceVisitor::

பொது:
    // CONSTRUCTORS
    LinkDotIfaceVisitor(AstIface* nodep, VSymEnt* curSymp, LinkDotState* statep)
        : m_statep{statep}
        , m_curSymp{curSymp} {
        UINFO(4, __FUNCTION__ << ": ");
        iterate(nodep);
    }
    ~LinkDotIfaceVisitor() override = இயல்பு;
};

வெறுமை LinkDotState::computeIfaceModSyms() {
    //  AstScope::user3()  // bool. Already iterated by a LinkDotIfaceVisitor
    மாறிலி VNUser3InUse user3InUse;
    க்கு (மாறிலி auto& itr : m_ifaceModSyms) {
        AstIface* மாறிலி nodep = itr.first;
        VSymEnt* மாறிலி symp = itr.second;
        LinkDotIfaceVisitor{nodep, symp, இது};
    }
    m_ifaceModSyms.clear();
}

//======================================================================

வகுப்பு LinkDotResolveVisitor final : பொது VNVisitor {
    // NODE STATE
    // Cleared on global
    //  *::user1p()             -> See LinkDotState
    //  *::user2p()             -> See LinkDotState
    //  *::user3()              // bool. Processed
    //  *::user4()              -> See LinkDotState
    மாறிலி VNUser3InUse m_inuser3;

    // TYPES
    எண்ணிக்கை DotPosition : uint8_t {
        // Must match ascii() method below
        DP_NONE = 0,  // Not under a DOT
        DP_PACKAGE,  // {package-or-class}:: DOT
        DP_FIRST,  // {scope-or-var} DOT
        DP_SCOPE,  // DOT... {scope-or-var} DOT
        DP_FINAL,  // [DOT...] {var-or-func-or-dtype} with no following dots
        DP_MEMBER  // DOT {member-name} [DOT...]
    };

    // STATE
    LinkDotState* மாறிலி m_statep;  // State, including dotted symbol table
    VSymEnt* m_curSymp = nullptr;  // SymEnt for current lookup point
    VSymEnt* m_modSymp = nullptr;  // SymEnt for current module
    VSymEnt* m_pinSymp = nullptr;  // SymEnt for pin lookups
    VSymEnt* m_randSymp = nullptr;  // SymEnt for randomize target class's lookups
    மாறிலி AstCell* m_cellp = nullptr;  // Current cell
    Ast_முனை_Module* m_modp = nullptr;  // Current module
    Ast_முனை_FTask* m_ftaskp = nullptr;  // Current function/task
    AstMethodCall* m_randMethodCallp = nullptr;  // Current randomize() method call
    முழு m_modport_இலக்கம் = 0;  // Uniqueify modport numbers
    முழு m_indent = 0;  // Indentation (tree depth) for debug
    தருக்க m_inSens = பொய்;  // True if in senitem
    மாறிலி AstWith* m_currentWithp = nullptr;  // Enclosing 'with' (nullptr if none)
    std::set<std::string>
        m_restrictedNamesUsed;  // Names from current 'with (id_list)' that resolved into target
    தருக்க m_genericIfaceModule = பொய்;  // True if in module containing generic interface
    std::map<std::string, Ast_முனை*> m_ifClassImpNames;  // Names imported from interface class
    // Inherited interface implementations needing a forward in the current class, by name
    std::map<std::string, மாறிலி VSymEnt*> m_ifForwards;
    std::set<AstClass*> m_extendsParam;  // Classes that have a parameterized super class
                                         // (except the default instances)
                                         // They are added to the set only in linkDotPrimary.
    தருக்க m_insideClassExtParam = பொய்;  // Inside a class from m_extendsParam
    AstNew* m_explicitSuperNewp = nullptr;  // Hit a "super.new" call inside a "new" function
    std::map<Ast_முனை*, AstPin*> m_usedPins;  // Pin used in this cell, map to duplicate
    std::map<Ast_முனை*, std::map<std::string, AstPin*>> m_usedDefParamPins;
    // Defparam pins used for a given formal, by hierarchical path
    std::map<std::string, Ast_முனை_Module*> m_modulesToRevisit;  // Modules to revisit a second time
    Ast_முனை* m_lastDeferredp = nullptr;  // Last node which requested a revisit of its module
    Ast_முனை_DType* m_packedArrayDtp = nullptr;  // Datatype reference for packed array
    தருக்க m_inPackedArray = பொய்;  // Currently traversing a packed array tree
    தருக்க m_replaceWithAlias
        = உண்மை;  // Replace VarScope with an alias. Used in the handling of AstAlias
    தருக்க m_isParam = பொய்;  // Specifies whether currently visiting param variable
    தருக்க m_resolvingTypedef = பொய்;  // Currently traversing a Typedef tree

    கட்டமைப்பு DotStates final {
        DotPosition m_dotPos;  // Scope part of dotted resolution
        VSymEnt* m_dotSymp;  // SymEnt for dotted AstParse lookup
        மாறிலி AstDot* m_dotp;  // Current dot
        தருக்க m_super;  // Starts with super reference
        தருக்க m_unresolvedCell;  // Unresolved cell, needs help from V3Param
        தருக்க m_unresolvedClass;  // Unresolved class reference, needs help from V3Param
        தருக்க m_unresolvedGenericIface;  // Unresolved generic interface, needs help from V3Param
        தருக்க m_genBlk;  // Contains gen block reference
        Ast_முனை* m_unlinkedScopep;  // Unresolved scope, needs corresponding VarXRef
        AstDisable* m_disablep;  // Disable statement under which the reference is
        தருக்க m_dotErr;  // Error found in dotted resolution, ignore upwards
        string m_dotText;  // String of dotted names found in below parseref
        DotStates() { init(nullptr); }
        ~DotStates() = இயல்பு;
        வெறுமை init(VSymEnt* curSymp, தருக்க unresolvedClass = பொய்) {
            m_dotPos = DP_NONE;
            m_dotSymp = curSymp;
            m_dotp = nullptr;
            m_super = பொய்;
            m_dotErr = பொய்;
            m_dotText = "";
            m_unresolvedCell = பொய்;
            m_unresolvedClass = unresolvedClass;
            m_unresolvedGenericIface = பொய்;
            m_genBlk = பொய்;
            m_unlinkedScopep = nullptr;
            m_disablep = nullptr;
        }
        string ascii() மாறிலி {
            நிலையான மாறிலி எழுத்து* மாறிலி names[]
                = {"DP_NONE", "DP_PACKAGE", "DP_FIRST", "DP_SCOPE", "DP_FINAL", "DP_MEMBER"};
            std::ostringstream sstr;
            sstr << "ds=" << names[m_dotPos];
            sstr << "  dse" << cvtToHex(m_dotSymp);
            மாறிலி string dsname = m_dotSymp->nodep()->name().substr(0, 8);
            sstr << "(" << m_dotSymp->nodep()->typeName() << (dsname.empty() ? "" : ":") << dsname
                 << ")";
            எனில் (m_dotErr) sstr << "  [dotErr]";
            எனில் (m_super) sstr << "  [super]";
            எனில் (m_unresolvedCell) sstr << "  [unrCell]";
            எனில் (m_unresolvedClass) sstr << "  [unrClass]";
            எனில் (m_unresolvedGenericIface) sstr << "  [unrGIface]";
            எனில் (m_genBlk) sstr << "  [genBlk]";
            sstr << "  txt=" << m_dotText;
            திரும்பு sstr.str();
        }
    } m_ds;  // State to preserve across recursions

    // METHODS - Variables
    AstVar* createImplicitVar(VSymEnt* /*lookupSymp*/, AstParseRef* nodep, Ast_முனை_Module* modp,
                              VSymEnt* moduleSymp, தருக்க noWarn) {
        // Create implicit after warning
        எனில் (!noWarn) {
            எனில் (nodep->fileline()->warnIsOff(V3ErrorCode::I_DEF_NETTYPE_WIRE)) {
                மாறிலி string suggest = m_statep->suggestSymFallback(moduleSymp, nodep->name(),
                                                                    Link_முனை_Matcher_Var{});
                nodep->v3error("Signal definition not found, and implicit disabled with "
                               "`default_nettype: "
                               << nodep->prettyNameQ() << '\n'
                               << (suggest.empty() ? "" : nodep->warnMore() + suggest));

            }
            // Bypass looking for suggestions if IMPLICIT is turned off
            // as there could be thousands of these suppressed in large netlists
            இல்லையெனில் எனில் (!nodep->fileline()->warnIsOff(V3ErrorCode::IMPLICIT)) {
                மாறிலி string suggest = m_statep->suggestSymFallback(moduleSymp, nodep->name(),
                                                                    Link_முனை_Matcher_Var{});
                nodep->v3warn(IMPLICIT,
                              "Signal definition not found, creating implicitly: "
                                  << nodep->prettyNameQ() << '\n'
                                  << (suggest.empty() ? "" : nodep->warnMore() + suggest));
            }
        }
        AstVar* மாறிலி newp
            = புதிய AstVar{nodep->fileline(), VVarType::WIRE, nodep->name(), VFlagLogicPacked{}, 1};
        newp->lifetime(VLifetime::STATIC_IMPLICIT);
        newp->trace(modp->modTrace());
        modp->addStmtsp(newp);
        // Link it to signal list, must add the variable under the module;
        // current scope might be lower now
        m_statep->செருகு_Sym(moduleSymp, newp->name(), newp, nullptr /*classOrPackagep*/);
        திரும்பு newp;
    }
    AstVar* foundToVarp(மாறிலி VSymEnt* symp, Ast_முனை* nodep, VAccess access) {
        // Return a variable if possible, auto converting a modport to variable
        எனில் (!symp) {
            திரும்பு nullptr;
        } இல்லையெனில் எனில் (VN_IS(symp->nodep(), Var)) {
            திரும்பு VN_AS(symp->nodep(), Var);
        } இல்லையெனில் எனில் (VN_IS(symp->nodep(), ModportVarRef)) {
            AstModportVarRef* மாறிலி snodep = VN_AS(symp->nodep(), ModportVarRef);
            AstVar* மாறிலி varp = snodep->varp();
            எனில் (access.isWriteOrRW() && snodep->direction().isReadOnly()) {
                nodep->v3error("Attempt to drive input-only modport: " << nodep->prettyNameQ());
            }  // else other simulators don't warn about reading, and IEEE doesn't say illegal
            திரும்பு varp;
        } இல்லையெனில் {
            திரும்பு nullptr;
        }
    }

    // Look up a virtual port through modport expression mapping.
    // When a dotted reference uses a modport with expression syntax, the virtual port name
    // maps to the real signal. This helper resolves that mapping during scope creation.
    // Returns the VarScope for the real signal, or nullptr if not found.
    AstVarScope* findVirtualPortVarScope(VSymEnt* dotSymp, மாறிலி string& portName) {
        மாறிலி AstVarScope* மாறிலி dotVscp = VN_CAST(dotSymp->nodep(), VarScope);
        மாறிலி AstVar* மாறிலி dotVarp = dotVscp ? dotVscp->varp() : nullptr;
        எனில் (!dotVarp) திரும்பு nullptr;

        Ast_முனை_DType* dtypep = dotVarp->childDTypep();
        எனில் (!dtypep) dtypep = dotVarp->subDTypep();
        எனில் (!dtypep) திரும்பு nullptr;
        மாறிலி AstIfaceRefDType* மாறிலி ifaceRefp = VN_CAST(dtypep, IfaceRefDType);
        எனில் (!ifaceRefp || !ifaceRefp->modportp() || !ifaceRefp->ifaceViaCellp()) திரும்பு nullptr;

        // Get the interface cell's symbol table
        VSymEnt* மாறிலி ifaceCellSymp = m_statep->get_முனை_Sym(ifaceRefp->ifaceViaCellp());

        // Look up modport by name in interface cell
        VSymEnt* மாறிலி modportSymp = ifaceCellSymp->findIdFallback(ifaceRefp->modportp()->name());
        எனில் (!modportSymp) திரும்பு nullptr;

        // Look up virtual port name in modport symbol table
        string baddot;
        VSymEnt* மாறிலி virtPortp = m_statep->findSymPrefixed(modportSymp, portName, baddot, உண்மை);
        எனில் (!virtPortp) திரும்பு nullptr;

        // Symbol may point to VarScope or Var
        எனில் (AstVarScope* vscp = VN_CAST(virtPortp->nodep(), VarScope)) திரும்பு vscp;

        // Symbol points to Var - look up VarScope by name in interface cell
        எனில் (மாறிலி AstVar* மாறிலி realVarp = VN_CAST(virtPortp->nodep(), Var)) {
            VSymEnt* மாறிலி realFoundp
                = m_statep->findSymPrefixed(ifaceCellSymp, realVarp->name(), baddot, உண்மை);
            திரும்பு realFoundp ? VN_CAST(realFoundp->nodep(), VarScope) : nullptr;
        }
        திரும்பு nullptr;
    }

    // Capture a ParamTypeDType reference for interface typedef retargeting.
    // Called when a RefDType resolves to a ParamTypeDType owned by an interface.
    வெறுமை captureIfaceParamType(AstRefDType* nodep, AstParamTypeDType* defp) {
        எனில் (!V3LinkDotIfaceCapture::enabled() || !m_statep->forPrimary()) திரும்பு;
        Ast_முனை_Module* மாறிலி defOwnerModp = V3LinkDotIfaceCapture::findOwnerModule(defp);
        எனில் (!defOwnerModp || !VN_IS(defOwnerModp, Iface)) திரும்பு;
        AstCell* மாறிலி cellForCapture
            = m_ds.m_dotSymp ? VN_CAST(m_ds.m_dotSymp->nodep(), Cell) : nullptr;
        எனில் (!cellForCapture) திரும்பு;
        UINFO(9, indent() << "iface capture add paramtype " << nodep
                          << " iface=" << defOwnerModp->prettyNameQ());
        V3LinkDotIfaceCapture::addParamType(nodep, cellForCapture->name(), m_modp, defp,
                                            defOwnerModp->name());
    }

    Ast_முனை_Stmt* addImplicitSuperNewCall(AstFunc* மாறிலி nodep,
                                         மாறிலி AstClassExtends* மாறிலி classExtendsp) {
        // Returns the added node
        File_வரி* மாறிலி fl = nodep->fileline();
        AstArg* argsp = nullptr;
        எனில் (classExtendsp->argsp()) argsp = classExtendsp->argsp()->clone_மரம்(உண்மை);
        AstNew* மாறிலி newExprp = புதிய AstNew{fl, argsp};
        newExprp->isImplicit(உண்மை);
        AstDot* மாறிலி superNewp = புதிய AstDot{fl, பொய், புதிய AstParseRef{fl, "super"}, newExprp};
        Ast_முனை_Stmt* மாறிலி superNewStmtp = superNewp->makeStmt();
        க்கு (Ast_முனை* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            // super.new shall be the first statement (IEEE 1800-2023 8.15)
            // but some nodes (such as variable declarations and typedefs) should stay before
            எனில் (VN_IS(stmtp, முனை_Stmt)) {
                stmtp->addHereThisAsNext(superNewStmtp);
                திரும்பு superNewStmtp;
            }
        }
        // There were no statements
        nodep->addStmtsp(superNewStmtp);
        திரும்பு superNewStmtp;
    }
    வெறுமை checkNoDot(Ast_முனை* nodep) {
        எனில் (VL_UNLIKELY(!nodep)) {
            UINFO(9, indent() << "iface capture null node passed to checkNoDot; dot state="
                              << m_ds.ascii());
            m_ds.m_dotErr = உண்மை;
            திரும்பு;
        }
        எனில் (VL_UNLIKELY(m_ds.m_dotPos != DP_NONE)) {
            UINFO(9, indent() << "ds=" << m_ds.ascii());
            UINFO(9, indent() << "iface capture checkNoDot hit node=" << nodep
                              << " dotState=" << m_ds.ascii());
            எனில் (VL_UNLIKELY(!m_ds.m_dotp)) {
                nodep->v3error("Syntax error: Not expecting " << nodep->type()
                                                              << " here (missing dot context)");
            } இல்லையெனில் {
                nodep->v3error("Syntax error: Not expecting "
                               << nodep->type() << " under a " << nodep->backp()->type()
                               << " in dotted expression\n"
                               << nodep->warnContextPrimary() << m_ds.m_dotp->warnOther()
                               << "... Resolving this reference\n"
                               << m_ds.m_dotp->warnContextSecondary());
            }
            m_ds.m_dotErr = உண்மை;
        }
    }
    AstVar* find_Iface_உச்சி_Varp(Ast_முனை* nodep, VSymEnt* parentEntp, மாறிலி string& name) {
        மாறிலி string findName = name + "__Viftop";
        VSymEnt* மாறிலி ifaceSymp = parentEntp->findIdFallback(findName);
        AstVar* மாறிலி iface_உச்சி_Varp = ifaceSymp ? VN_AS(ifaceSymp->nodep(), Var) : nullptr;
        UASSERT_OBJ(iface_உச்சி_Varp, nodep, "Can't find interface var ref: " << findName);
        திரும்பு iface_உச்சி_Varp;
    }
    AstRSProd* findProd(Ast_முனை* nodep, VSymEnt* parentEntp, மாறிலி string& name) {
        மாறிலி VSymEnt* மாறிலி foundp = parentEntp->findIdFallback(name);
        AstRSProd* foundNodep = foundp ? VN_CAST(foundp->nodep(), RSProd) : nullptr;
        எனில் (!foundNodep) {
            VSpellCheck speller;
            Link_முனை_Matcher_Prod matcher;
            parentEntp->candidateIdFlat(&speller, &matcher, பொய்);
            மாறிலி string suggest = speller.best_Candidate_செய்தி(name);
            UINFO(1, "   ErrParseRef curSymp=se" << cvtToHex(parentEntp));
            nodep->v3error("Production " << Ast_முனை::prettyNameQ(name) << " not found\n"
                                         << (suggest.empty() ? "" : nodep->warnMore() + suggest));
            திரும்பு nullptr;
        }
        திரும்பு foundNodep;
    }
    வெறுமை duplicatePinError(AstPin* nodep, Ast_முனை* origp, மாறிலி எழுத்து* whatp) {
        nodep->v3error("Duplicate " << whatp << " connection: " << nodep->prettyNameQ() << '\n'
                                    << nodep->warnContextPrimary() << '\n'
                                    << origp->warnOther() << "... Location of original " << whatp
                                    << " connection\n"
                                    << origp->warnContextSecondary());
    }
    வெறுமை markAndCheckPinDup(AstPin* nodep, Ast_முனை* refp, மாறிலி எழுத்து* whatp) {
        மாறிலி auto pair = m_usedPins.emplace(refp, nodep);
        எனில் (pair.second) {
            எனில் (!nodep->paramPath().empty())
                m_usedDefParamPins[refp].emplace(nodep->paramPath(), nodep);
            திரும்பு;
        } இல்லையெனில் {
            Ast_முனை* மாறிலி origp = pair.first->second;
            எனில் (nodep->paramPath().empty() || VN_AS(origp, Pin)->paramPath().empty()) {
                duplicatePinError(nodep, origp, whatp);
                திரும்பு;
            }
        }

        auto& defParamPins = m_usedDefParamPins[refp];
        மாறிலி auto defParamIt = defParamPins.find(nodep->paramPath());
        எனில் (defParamIt != defParamPins.end()) {
            duplicatePinError(nodep, defParamIt->second, whatp);
            திரும்பு;
        }
        defParamPins.emplace(nodep->paramPath(), nodep);
    }
    நிலையான தருக்க extendsClass(மாறிலி AstClass* classp, மாறிலி AstClass* targetp) {
        // Return true if classp inherits from targetp through already resolved extends
        // If performance of this becomes a problem, use just a counter to check
        // exceeds some number of iterations then perform the more expensive analysis
        std::set<மாறிலி AstClass*> visited;
        std::vector<மாறிலி AstClass*> todo{classp};
        போது (!todo.empty()) {
            மாறிலி AstClass* மாறிலி currp = todo.back();
            todo.எடு_back();
            எனில் (currp == targetp) திரும்பு உண்மை;
            எனில் (!visited.செருகு(currp).second) தொடர்;
            க்கு (மாறிலி AstClassExtends* cextp = currp->extendsp(); cextp;
                 cextp = VN_AS(cextp->nextp(), ClassExtends)) {
                எனில் (மாறிலி AstClass* மாறிலி basep = cextp->classOrNullp()) todo.தள்ளு_back(basep);
            }
        }
        திரும்பு பொய்;
    }
    நிலையான AstClocking* sensClockingp(Ast_முனை* nodep) {
        // Return the clocking block referenced by nodep, either directly or through a modport
        எனில் (AstClocking* மாறிலி clockingp = VN_CAST(nodep, Clocking)) திரும்பு clockingp;
        எனில் (மாறிலி AstModportClockingRef* மாறிலி clockingRefp = VN_CAST(nodep, ModportClockingRef))
            திரும்பு clockingRefp->clockingp();
        திரும்பு nullptr;
    }
    VSymEnt* getCreateClockingEventSymEnt(AstClocking* clockingp) {
        AstVar* மாறிலி eventp = clockingp->ensureEventp(உண்மை);
        எனில் (!eventp->user1p()) eventp->user1p(புதிய VSymEnt{m_statep->symsp(), eventp});
        திரும்பு reinterpret_cast<VSymEnt*>(eventp->user1p());
    }
    VSymEnt* findInlinedDotsSym(Ast_முனை* nodep, மாறிலி string& inlinedDots) {
        // Return scope that applies for searching, given renaming due to module inlining.
        // Dotted lookup is always relative to module, as maybe variable
        // name lower down with same scope name we want to ignore (t_math_divw)
        VSymEnt* dotSymp = m_modSymp;
        மாறிலி string inl = Ast_முனை::dedotName(inlinedDots);
        string baddot;
        VSymEnt* okSymp = nullptr;
        dotSymp = m_statep->findDotted(nodep->fileline(), dotSymp, inl, baddot, okSymp, பொய்);
        UASSERT_OBJ(dotSymp, nodep,
                    "Couldn't resolve inlined scope " << Ast_முனை::prettyNameQ(baddot)
                                                      << " in: " << inlinedDots);
        UINFO(9, indent() << "findInlinedDotsSym " << dotSymp
                          << " search start due to inlinedDots=" << inlinedDots);
        திரும்பு dotSymp;
    }
    VSymEnt* findDottedRefSymp(Ast_முனை* nodep, VSymEnt* lookupSymp, மாறிலி string& dotted,
                               string& baddot, VSymEnt*& okSymp, தருக்க* modportp = nullptr) {
        // Return scope to search for a relinked hierarchical reference
        // (AstVarXRef or AstEnumItemRef), or nullptr after error if not found
        // If modportp is given, set true when the reference went through a modport
        எனில் (modportp) *modportp = பொய்;
        VSymEnt* dotSymp = m_statep->findDotted(nodep->fileline(), lookupSymp, dotted, baddot,
                                                okSymp, உண்மை);  // Maybe nullptr
        எனில் (!dotSymp) {
            nodep->v3error(
                "Can't find definition of "
                << (!baddot.empty() ? Ast_முனை::prettyNameQ(baddot) : nodep->prettyNameQ()) << '\n'
                << nodep->warnContextPrimary());
            திரும்பு nullptr;
        }
        எனில் (மாறிலி AstVar* மாறிலி varp = VN_CAST(dotSymp->nodep(), Var)) {
            எனில் (மாறிலி AstIfaceRefDType* மாறிலி ifaceRefp
                = VN_CAST(varp->childDTypep(), IfaceRefDType)) {
                எனில் (ifaceRefp->modportp()) {
                    dotSymp = m_statep->get_முனை_Sym(ifaceRefp->modportp());
                    எனில் (modportp) *modportp = உண்மை;
                } இல்லையெனில் {
                    dotSymp = m_statep->get_முனை_Sym(ifaceRefp->ifacep());
                }
            }
        } இல்லையெனில் எனில் (மாறிலி AstModportClockingRef* மாறிலி clockingRefp
                   = VN_CAST(dotSymp->nodep(), ModportClockingRef)) {
            dotSymp = m_statep->get_முனை_Sym(clockingRefp->clockingp());
        }
        திரும்பு dotSymp;
    }

    நிலையான தருக்க isParamedClassRefDType(மாறிலி Ast_முனை* classp) {
        போது (மாறிலி AstRefDType* மாறிலி refp = VN_CAST(classp, RefDType))
            classp = refp->subDTypep();
        திரும்பு (VN_IS(classp, ClassRefDType) && VN_AS(classp, ClassRefDType)->paramsp())
               || VN_IS(classp, ParamTypeDType);
    }
    தருக்க isParamedClassRef(மாறிலி Ast_முனை* nodep) {
        // Is this a parameterized reference to a class, or a reference to class parameter
        எனில் (மாறிலி auto* classRefp = VN_CAST(nodep, ClassOrPackageRef)) {
            எனில் (classRefp->paramsp()) திரும்பு உண்மை;
            மாறிலி auto* classp = classRefp->classOrPackageNodep();
            போது (மாறிலி auto* typedefp = VN_CAST(classp, Typedef)) classp = typedefp->subDTypep();
            திரும்பு isParamedClassRefDType(classp);
        }
        திரும்பு isParamedClassRefDType(nodep);
    }
    VSymEnt* getThisClassSymp() {
        VSymEnt* classSymp = m_ds.m_dotSymp;
        போது (classSymp && !VN_IS(classSymp->nodep(), Class)) {
            classSymp = classSymp->parentp();
        }
        திரும்பு classSymp;
    }
    // Is the current scope within the given class, including within its nested classes
    தருக்க insideClass(மாறிலி AstClass* classp) மாறிலி {
        க்கு (மாறிலி VSymEnt* symp = m_curSymp; symp; symp = symp->parentp()) {
            எனில் (symp->nodep() == classp) திரும்பு உண்மை;
        }
        திரும்பு பொய்;
    }
    // 'randomize() with' target class not yet specialized; defer to post-V3Param pass
    தருக்க randTargetUnresolved() மாறிலி {
        திரும்பு m_statep->forPrimary() && m_currentWithp && m_randMethodCallp && !m_randSymp;
    }
    // Is 'funcp' of 'classp' virtual, directly or by overriding a virtual method of a base
    // class (IEEE 1800-2023 8.20)
    தருக்க isVirtualMethod(மாறிலி Ast_முனை_FTask* funcp, மாறிலி AstClass* classp) மாறிலி {
        எனில் (funcp->isVirtual()) திரும்பு உண்மை;
        க்கு (மாறிலி AstClassExtends* cextp = classp->extendsp(); cextp;
             cextp = VN_AS(cextp->nextp(), ClassExtends)) {
            AstClass* மாறிலி basep = cextp->classOrNullp();
            எனில் (!basep) தொடர்;
            மாறிலி VSymEnt* மாறிலி foundp = m_statep->get_முனை_Sym(basep)->findIdFlat(funcp->name());
            மாறிலி Ast_முனை_FTask* மாறிலி baseFuncp
                = foundp ? VN_CAST(foundp->nodep(), முனை_FTask) : nullptr;
            மாறிலி AstClass* மாறிலி baseClassp = foundp && foundp->parentp()
                                                   ? VN_CAST(foundp->parentp()->nodep(), Class)
                                                   : nullptr;
            எனில் (baseFuncp && baseClassp && isVirtualMethod(baseFuncp, baseClassp)) திரும்பு உண்மை;
        }
        திரும்பு பொய்;
    }
    // Is imported 'implp' an implementation of interface method 'ifSymp' (IEEE 1800-2023
    // 8.26.2)? If its class does not derive from the interface class, it needs a forwarding
    // override in the derived class to be reachable through the interface.
    தருக்க inheritedImplements(மாறிலி VSymEnt* implp, மாறிலி VSymEnt* ifSymp, தருக்க& needForwardr) {
        needForwardr = பொய்;
        மாறிலி Ast_முனை_FTask* மாறிலி implFuncp = VN_CAST(implp->nodep(), முனை_FTask);
        எனில் (!implFuncp || implFuncp->pureVirtual() || implFuncp->isStatic()) திரும்பு பொய்;
        மாறிலி AstClass* மாறிலி implClassp
            = implp->parentp() ? VN_CAST(implp->parentp()->nodep(), Class) : nullptr;
        மாறிலி AstClass* மாறிலி ifClassp
            = ifSymp->parentp() ? VN_CAST(ifSymp->parentp()->nodep(), Class) : nullptr;
        எனில் (!implClassp || !ifClassp) திரும்பு பொய்;
        எனில் (extendsClass(implClassp, ifClassp)) திரும்பு உண்மை;
        எனில் (!isVirtualMethod(implFuncp, implClassp)) திரும்பு பொய்;
        needForwardr = உண்மை;
        திரும்பு உண்மை;
    }
    // Add 'implp' override to 'classp' that calls super.implp, so the C++ method of an
    // implemented interface class is overridden
    வெறுமை addInterfaceForward(AstClass* classp, Ast_முனை_FTask* implp, AstClass* implClassp) {
        File_வரி* மாறிலி flp = implp->fileline();
        AstVar* fvarp = nullptr;
        Ast_முனை_FTask* newp;
        எனில் (AstFunc* மாறிலி implFuncp = VN_CAST(implp, Func)) {
            fvarp = VN_AS(implFuncp->fvarp(), Var)->clone_மரம்(பொய்);
            newp = புதிய AstFunc{flp, implp->name(), nullptr, fvarp};
        } இல்லையெனில் {
            newp = புதிய AstTask{flp, implp->name(), nullptr};
        }
        newp->classMethod(உண்மை);
        newp->isVirtual(உண்மை);
        newp->lifetime(implp->lifetime());
        AstArg* argsp = nullptr;
        க்கு (Ast_முனை* stmtp = implp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstVar* மாறிலி portp = VN_CAST(stmtp, Var);
            எனில் (!portp || !portp->isIO()) தொடர்;
            AstVar* மாறிலி newPortp = portp->clone_மரம்(பொய்);
            newp->addStmtsp(newPortp);
            argsp = Ast_முனை::addNext(
                argsp, புதிய AstArg{flp, "", புதிய AstVarRef{flp, newPortp, VAccess::READ}});
        }
        Ast_முனை_FTask_Ref* callp;
        எனில் (AstFunc* மாறிலி implFuncp = VN_CAST(implp, Func)) {
            callp = புதிய AstFuncRef{flp, implFuncp, argsp};
        } இல்லையெனில் {
            callp = புதிய AstTaskRef{flp, VN_AS(implp, Task), argsp};
        }
        callp->classOrPackagep(implClassp);
        callp->superReference(உண்மை);
        எனில் (fvarp) {
            newp->addStmtsp(புதிய AstAssign{flp, புதிய AstVarRef{flp, fvarp, VAccess::WRITE}, callp});
        } இல்லையெனில் {
            newp->addStmtsp(callp->makeStmt());
        }
        UINFO(9, indent() << "Interface forward " << newp);
        classp->addMembersp(newp);
    }
    வெறுமை importDerivedClass(AstClass* derivedClassp, VSymEnt* baseSymp, AstClass* baseClassp) {
        // Also used for standard 'extends' from a base class
        UINFO(8, indent() << "importDerivedClass to " << derivedClassp << " from " << baseClassp);
        க்கு (VSymEnt::const_iterator it = baseSymp->begin(); it != baseSymp->end(); ++it) {
            எனில் (Ast_முனை* baseSubp = it->second->nodep()) {
                UINFO(8, indent() << "  SymFunc " << baseSubp);
                மாறிலி string impOrExtends
                    = baseClassp->isInterfaceClass() ? " implements " : " extends ";
                எனில் (VN_IS(baseSubp, முனை_FTask)) {
                    மாறிலி VSymEnt* மாறிலி foundp = m_curSymp->findIdFlat(baseSubp->name());
                    மாறிலி Ast_முனை_FTask* மாறிலி baseFuncp = VN_CAST(baseSubp, முனை_FTask);
                    எனில் (!baseFuncp || !baseFuncp->pureVirtual()) தொடர்;
                    தருக்க needForward = பொய்;
                    மாறிலி தருக்க existsInDerived
                        = foundp
                          && (!foundp->imported()
                              || inheritedImplements(foundp, it->second, needForward));
                    எனில் (needForward && m_statep->forPrimary()
                        && !derivedClassp->isInterfaceClass()) {
                        m_ifForwards.emplace(baseSubp->name(), foundp);
                    }
                    எனில் (m_statep->forPrimary() && !existsInDerived
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
                    மாறிலி auto itn = m_ifClassImpNames.find(baseSubp->name());
                    எனில் (m_statep->forPrimary() && !existsInDerived
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
                எனில் (VN_IS(baseSubp, Constraint)) {
                    மாறிலி VSymEnt* மாறிலி foundp = m_curSymp->findIdFlat(baseSubp->name());
                    மாறிலி AstConstraint* மாறிலி baseFuncp = VN_CAST(baseSubp, Constraint);
                    எனில் (!baseFuncp || !baseFuncp->isKwdPure()) தொடர்;
                    மாறிலி தருக்க existsInDerived = foundp && !foundp->imported();
                    எனில் (m_statep->forPrimary() && !existsInDerived
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
    வெறுமை importSymbolsFromExtended(AstClass* மாறிலி nodep, AstClassExtends* மாறிலி cextp) {
        AstClass* மாறிலி baseClassp = cextp->classp();
        VSymEnt* மாறிலி srcp = m_statep->get_முனை_Sym(baseClassp);
        importDerivedClass(nodep, srcp, baseClassp);
        எனில் (!cextp->isImplements()) m_curSymp->importFromClass(m_statep->symsp(), srcp);
    }
    வெறுமை classExtendImport(AstClass* nodep) {
        // A class reference might be to a class that is later in Ast due to
        // e.g. parameterization or referring to a "class (type T) extends T"
        // Resolve it so later Class:: references into its base classes work
        symIterateNull(nodep, m_statep->get_முனை_Sym(nodep));
    }

    வெறுமை checkDeclOrder(Ast_முனை* nodep, Ast_முனை* declp) {
        மாறிலி uint32_t decl_Token_இலக்கம் = declp->decl_Token_இலக்கம்();
        எனில் (!decl_Token_இலக்கம்) திரும்பு;  // Not implemented/valid on this object
        எனில் (nodep->fileline()->token_இலக்கம்() < decl_Token_இலக்கம்) {
            UINFO(1, "Related node " << nodep->fileline()->token_இலக்கம்() << " " << nodep);
            UINFO(1, "Related decl " << decl_Token_இலக்கம் << " " << declp);
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
    வெறுமை checkMemberDeclOrder(Ast_முனை* nodep, AstMemberDType* declp) {
        மாறிலி uint32_t decl_Token_இலக்கம் = declp->fileline()->token_இலக்கம்();
        எனில் (nodep->fileline()->token_இலக்கம்() < decl_Token_இலக்கம்) {
            UINFO(1, "Related node " << nodep->fileline()->token_இலக்கம்() << " " << nodep);
            UINFO(1, "Related decl " << decl_Token_இலக்கம் << " " << declp);
            nodep->v3error("Reference to "
                           << nodep->prettyNameQ() << " before declaration (IEEE 1800-2023 6.18)\n"
                           << nodep->warnMore()
                           << "... Suggest move the declaration before the reference\n"
                           << nodep->warnContextPrimary() << '\n'
                           << declp->warnOther() << "... Location of original declaration\n"
                           << declp->warnContextSecondary());
        }
    }

    வெறுமை replaceWithCheckBreak(Ast_முனை* oldp, Ast_முனை_DType* newp) {
        // Flag now to avoid V3Broken throwing an internal error
        எனில் (oldp->wouldBreak(newp)) {
            newp->v3error(
                "Data type used where a non-data type is expected: " << newp->prettyNameQ());
            oldp->replaceWith(புதிய AstConst{newp->fileline(), AstConst::BitFalseErroring{}});
        } இல்லையெனில் {
            oldp->replaceWith(newp);
        }
    }

    // Marks the current module to be revisited after the initial AST iteration
    வெறுமை revisitLater(Ast_முனை* deferredNodep) {
        // Need to revisit entire module to build up all the necessary context
        m_lastDeferredp = deferredNodep;
        m_modulesToRevisit.செருகு(std::make_pair(m_modp->name(), m_modp));
    }

    வெறுமை updateVarUse(AstVar* nodep) {
        // Avoid dotted.PARAM false positive when in a parameter block
        // that is if ()'ed off by same dotted name as another block
        எனில் (nodep && nodep->isParam()) nodep->usedParam(உண்மை);
    }

    நிலையான VSymEnt* findIdFallbackSkipMemberDType(VSymEnt* lookp, மாறிலி string& name) {
        VSymEnt* shadowEntp = nullptr;  // Shadowing variable: not a type, kept for error report
        போது (lookp) {
            VSymEnt* மாறிலி foundp = lookp->findIdFlat(name);
            எனில் (foundp && !VN_IS(foundp->nodep(), MemberDType)) {
                // Non-type entries are not type candidates
                // (IEEE 1800-2023 6.18); skip them so an enclosing
                // type is found, but keep one to preserve the "found: ..." error.
                எனில் (VN_IS(foundp->nodep(), Typedef) || VN_IS(foundp->nodep(), ParamTypeDType)
                    || VN_IS(foundp->nodep(), Class)) {
                    திரும்பு foundp;
                }
                எனில் (!shadowEntp) shadowEntp = foundp;
            }
            lookp = lookp->fallbackp();
        }
        திரும்பு shadowEntp;
    }

    வெறுமை symIterateChildren(Ast_முனை* nodep, VSymEnt* symp) {
        // Iterate children, changing to given context, with restore to old context
        VL_RESTORER_COPY(m_ds);
        VL_RESTORER(m_curSymp);
        m_curSymp = symp;
        m_ds.init(m_curSymp);
        iterateChildren(nodep);
    }
    வெறுமை symIterateNull(Ast_முனை* nodep, VSymEnt* symp) {
        // Iterate node, changing to given context, with restore to old context
        VL_RESTORER_COPY(m_ds);
        VL_RESTORER(m_curSymp);
        m_curSymp = symp;
        m_ds.init(m_curSymp);
        iterateNull(nodep);
    }
    நிலையான மாறிலி Ast_முனை_DType* get_கோவை_DTypep(மாறிலி Ast_முனை_கோவை* selp) {
        போது (மாறிலி Ast_முனை_Pre_Sel* மாறிலி sp = VN_CAST(selp, முனை_Pre_Sel)) selp = sp->fromp();
        எனில் (மாறிலி AstMemberSel* மாறிலி sp = VN_CAST(selp, MemberSel)) {
            எனில் (மாறிலி Ast_முனை_DType* dtypep = get_கோவை_DTypep(sp->fromp())) {
                எனில் (மாறிலி AstClassRefDType* மாறிலி classRefp = VN_CAST(dtypep, ClassRefDType)) {
                    மாறிலி AstClass* மாறிலி classp = classRefp->classp();
                    மாறிலி தருக்க found = classp->existsMember(
                        [&dtypep, name = selp->name()](மாறிலி AstClass*, மாறிலி AstVar* nodep) {
                            dtypep = nodep->childDTypep();
                            திரும்பு nodep->name() == name;
                        });
                    எனில் (found) திரும்பு dtypep->elemDTypep();
                    selp->v3error("Class " << classRefp->prettyNameQ()
                                           << " does not contain field " << selp->prettyNameQ());
                } இல்லையெனில் {
                    selp->v3fatalSrc("Member selection on expression of type "
                                     << dtypep->prettyDTypeNameQ()
                                     << ", which is not a class type");
                }
            }
        } இல்லையெனில் எனில் (மாறிலி Ast_முனை_Var_Ref* மாறிலி varRefp = VN_CAST(selp, முனை_Var_Ref)) {
            திரும்பு varRefp->varp()->childDTypep()->elemDTypep();
        }
        திரும்பு nullptr;
    }
    நிலையான மாறிலி AstVar* getNextVarp(மாறிலி Ast_முனை* stmtsp) {
        // Only IO ports, as parameters are on paramsp(), not the pinsp() list paired here
        போது (stmtsp) {
            எனில் (மாறிலி AstVar* மாறிலி varp = VN_CAST(stmtsp, Var)) {
                எனில் (varp->isIO()) திரும்பு varp;
            }
            stmtsp = stmtsp->nextp();
        }
        திரும்பு nullptr;
    }
    // Introduce implicit parameters for modules with generic interafeces
    வெறுமை addImplicitParametersOfGenericIface(AstCell* மாறிலி nodep, மாறிலி AstModule* மாறிலி modp) {
        // Get Param number
        முழு param_இலக்கம் = 1;
        க்கு (AstPin* paramp = nodep->paramsp(); paramp; paramp = VN_CAST(paramp->nextp(), Pin)) {
            ++param_இலக்கம்;
        }

        // Add each implicit parameter type
        மாறிலி AstVar* modIfaceVarp = getNextVarp(modp->stmtsp());
        க்கு (மாறிலி AstPin* pinp = nodep->pinsp(); pinp && modIfaceVarp;
             pinp = VN_CAST(pinp->nextp(), Pin),
                           modIfaceVarp = getNextVarp(modIfaceVarp->nextp())) {
            எனில் (modIfaceVarp->varType() != VVarType::IFACEREF
                || !VN_IS(modIfaceVarp->childDTypep()->elemDTypep(), IfaceGenericDType)) {
                தொடர்;
            }
            Ast_முனை* exprp = pinp->exprp();
            எனில் (!exprp) {
                modIfaceVarp->v3error("Interface port "
                                      << modIfaceVarp->prettyNameQ()
                                      << " is not connected to interface/modport pin expression");
                தொடர்;
            }
            போது (மாறிலி Ast_முனை_Pre_Sel* மாறிலி preSelp = VN_CAST(exprp, முனை_Pre_Sel)) {
                exprp = preSelp->fromp();
            }
            // Resolve pin expression to the enclosing module's port Var. At primary
            // LinkDot the expression may still be an AstParseRef, so also look up by name.
            AstVar* enclosingVarp = nullptr;
            மாறிலி AstVarRef* மாறிலி varRefp = VN_CAST(exprp, VarRef);
            எனில் (varRefp) {
                enclosingVarp = varRefp->varp();
            } இல்லையெனில் எனில் (மாறிலி AstParseRef* மாறிலி parseRefp = VN_CAST(exprp, ParseRef)) {
                எனில் (m_modp) {
                    எனில் (VSymEnt* மாறிலி symp
                        = m_statep->get_முனை_Sym(m_modp)->findIdFlat(parseRefp->name())) {
                        enclosingVarp = VN_CAST(symp->nodep(), Var);
                    }
                }
            }
            எனில் (enclosingVarp && enclosingVarp->varType() == VVarType::IFACEREF
                && VN_IS(enclosingVarp->childDTypep()->skipRefp(), IfaceGenericDType)) {
                // Nested generic-iface forwarding (#7454): enclosing port is itself still
                // generic, so emit a placeholder __VGIfaceParam pin carrying a VarRef to
                // the outer port. V3Param rewrites it to the concrete IfaceRefDType once
                // the enclosing module is specialized (see
                // ParamProcessor::resolveGenericIfaceForwardingPins for the ordering
                // constraint that keeps the rewrite inside V3Param).
                AstVarRef* மாறிலி fwdRefp
                    = புதிய AstVarRef{exprp->fileline(), enclosingVarp, VAccess::READ};
                AstPin* மாறிலி newPinp = புதிய AstPin{
                    pinp->fileline(), param_இலக்கம், "__VGIfaceParam" + modIfaceVarp->name(), fwdRefp};
                newPinp->param(உண்மை);
                visit(newPinp);
                nodep->addParamsp(newPinp);
            } இல்லையெனில் எனில் (varRefp) {
                மாறிலி AstVar* மாறிலி varp = varRefp->varp();
                எனில் (மாறிலி AstIfaceRefDType* மாறிலி refp
                    = VN_CAST(varp->childDTypep()->elemDTypep(), IfaceRefDType)) {
                    AstIface* மாறிலி ifacep = VN_AS(refp->cellp()->modp(), Iface);
                    AstIfaceRefDType* newIfaceRefp;
                    எனில் (refp->modportp()) {
                        newIfaceRefp = புதிய AstIfaceRefDType{
                            refp->fileline(), refp->modportFileline(), m_modp->name(),
                            ifacep->name(), refp->modportName()};
                    } இல்லையெனில் {
                        newIfaceRefp = புதிய AstIfaceRefDType{refp->fileline(), m_modp->name(),
                                                            ifacep->name()};
                    }
                    newIfaceRefp->ifacep(ifacep);
                    எனில் (refp->cellp()->paramsp()) {
                        newIfaceRefp->addParamsp(refp->cellp()->paramsp()->clone_மரம்(உண்மை));
                    }
                    UASSERT_OBJ(pinp->name().find("__pinNumber") == 0
                                    || pinp->name() == modIfaceVarp->name(),
                                pinp, "Not found interface with such name");
                    AstPin* மாறிலி newPinp
                        = புதிய AstPin{pinp->fileline(), param_இலக்கம்,
                                     "__VGIfaceParam" + modIfaceVarp->name(), newIfaceRefp};
                    newPinp->param(உண்மை);
                    visit(newPinp);
                    nodep->addParamsp(newPinp);
                } இல்லையெனில் {
                    varRefp->v3error("Generic interfaces can only connect to an interface and "
                                     << varp->prettyNameQ() << " is "
                                     << (varp->childDTypep()
                                             ? "of type " + varp->childDTypep()->prettyDTypeNameQ()
                                             : "not an interface"));
                }
            } இல்லையெனில் {
                exprp->v3error("Expected an interface but " << exprp->prettyNameQ()
                                                            << " is not an interface");
            }
            ++param_இலக்கம்;
        }
    }

    // Returns true and issues error iff 'varp' cannot be accesed with a VarXRef
    // in a hier_block given by 'scopeSymp'
    தருக்க errorHierNonPort(AstVarXRef* refp, AstVar* varp, VSymEnt* scopeSymp) {
        // OK to access ports on hier_block
        எனில் (varp->isIO()) திரும்பு பொய்;
        // OK if not an instance
        மாறிலி AstCell* மாறிலி cellp = VN_CAST(scopeSymp->nodep(), Cell);
        எனில் (!cellp) திரும்பு பொய்;
        // Ok if not a hier_block
        மாறிலி Ast_முனை_Module* மாறிலி modp = cellp->modp();
        எனில் (!modp->hierBlock()) திரும்பு பொய்;
        // Bad
        refp->v3error("Cannot access non-port symbols inside hierarchical block");
        திரும்பு உண்மை;
    }

#define LINKDOT_VISIT_START() \
    VL_RESTORER(m_indent); \
    ++m_indent;

    string indent() மாறிலி {
        string result = "";
        result.செருகு(0, m_indent, ':');
        திரும்பு result + " ";
    }

    // VISITORS
    வெறுமை visit(AstNetlist* nodep) override {
        // Recurse..., backward as must do packages before using packages
        iterateChildrenBackwardsConst(nodep);
    }
    வெறுமை visit(AstTypeTable*) override {}
    வெறுமை visit(Ast_முனை_Module* nodep) override {
        எனில் (nodep->dead()) திரும்பு;
        LINKDOT_VISIT_START();
        UINFO(8, indent() << "visit " << nodep);
        VL_RESTORER(m_genericIfaceModule);
        எனில் (மாறிலி AstModule* மாறிலி modp = VN_CAST(nodep, Module)) {
            m_genericIfaceModule = modp->hasGenericIface();
        }
        checkNoDot(nodep);
        m_ds.init(m_curSymp);
        m_ds.m_dotSymp = m_curSymp = m_modSymp
            = m_statep->get_முனை_Sym(nodep);  // Until overridden by a SCOPE
        m_cellp = nullptr;
        m_modp = nodep;
        m_modport_இலக்கம் = 0;
        iterateChildren(nodep);
        m_modp = nullptr;
        m_ds.m_dotSymp = m_curSymp = m_modSymp = nullptr;
        m_ds.m_dotPos = DP_NONE;
        m_ds.m_dotErr = பொய்;
    }
    வெறுமை visit(AstScope* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(8, indent() << "visit " << nodep);
        checkNoDot(nodep);
        VL_RESTORER(m_modSymp);
        VL_RESTORER(m_curSymp);
        m_ds.m_dotSymp = m_curSymp = m_modSymp = m_statep->getScopeSym(nodep);
        iterateChildren(nodep);
        m_ds.m_dotSymp = m_curSymp = m_modSymp = nullptr;
    }
    வெறுமை visit(AstCellInline* nodep) override {
        LINKDOT_VISIT_START();
        checkNoDot(nodep);
        UASSERT_OBJ(m_statep->forScopeCreation(), nodep, "CellInline should not exist yet");
        VL_DO_DANGLING(தள்ளு_Deletep(nodep->unlinkFrBack()), nodep);
    }
    வெறுமை visit(AstCell* nodep) override {
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
            எனில் (VN_IS(nodep->modp(), NotFoundModule)) {
                // Prevent warnings about missing pin connects
                எனில் (nodep->pinsp()) nodep->pinsp()->unlinkFrBackWithNext()->delete_மரம்();
                எனில் (nodep->paramsp()) nodep->paramsp()->unlinkFrBackWithNext()->delete_மரம்();
            }
            // Need to pass the module info to this cell, so we can link up the pin names
            // However can't use m_curSymp as pin connections need to use the
            // instantiator's symbols
            இல்லையெனில் {
                m_pinSymp = m_statep->get_முனை_Sym(nodep->modp());
                UINFO(4, indent() << "(Backto) visit " << nodep);
                // UINFOTREE(1, nodep, "", "linkcell");
                // UINFOTREE(1, nodep->modp(), "", "linkcemd");
                iterateChildren(nodep);

                எனில் (m_statep->forPrimary())
                    எனில் (மாறிலி AstModule* மாறிலி modp = VN_CAST(nodep->modp(), Module)) {
                        எனில் (modp->hasGenericIface())
                            addImplicitParametersOfGenericIface(nodep, modp);
                    }
            }
        }
        // Parent module inherits child's publicity
        // This is done bottom up in the LinkBotupVisitor stage
    }
    வெறுமை visit(AstClassRefDType* nodep) override {
        // Cell: Recurse inside or cleanup not founds
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        // Can be under dot if called as package::class and that class resolves, so no checkNoDot
        VL_RESTORER_CLEAR(m_usedPins);
        VL_RESTORER_CLEAR(m_usedDefParamPins);
        UASSERT_OBJ(nodep->classp(), nodep, "ClassRef has unlinked class");
        UASSERT_OBJ(m_statep->forPrimary() || !nodep->paramsp() || V3Error::error_மொத்தம்(), nodep,
                    "class reference parameter not removed by V3Param");
        VL_RESTORER(m_pinSymp);
        {
            // ClassRef's have pins, so track
            m_pinSymp = m_statep->get_முனை_Sym(nodep->classp());
            UINFO(4, indent() << "(Backto) visit " << nodep);
            // UINFOTREE(1, nodep, "", "linkcell");
            // UINFOTREE(1, nodep->modp(), "", "linkcemd");
            iterateChildren(nodep);
        }
    }
    வெறுமை visit(AstPin* nodep) override {
        // Pin: Link to submodule's port
        LINKDOT_VISIT_START();
        checkNoDot(nodep);
        iterateChildren(nodep);
        எனில் (!nodep->modVarp()) {
            UASSERT_OBJ(m_pinSymp, nodep, "Pin not under instance?");
            VSymEnt* மாறிலி foundp = m_pinSymp->findIdFlat(nodep->name());
            மாறிலி எழுத்து* மாறிலி whatp = nodep->param() ? "parameter" : "pin";
            எனில் (!foundp) {
                எனில் (m_cellp && VN_IS(m_cellp->modp(), Primitive)
                    && (nodep->name() == "__paramNumber1" || nodep->name() == "__paramNumber2")) {
                    // Primitive parameter is really a delay2 we can just ignore
                    VL_DO_DANGLING(தள்ளு_Deletep(nodep->unlinkFrBack()), nodep);
                    திரும்பு;
                } இல்லையெனில் எனில் (nodep->param() && !nodep->exprp() && !nodep->svDotName()) {
                    // Placeholder pin the parser makes for an empty '#()', to signal
                    // the user did type the '#()'. No further use, deleting.
                    VL_DO_DANGLING(தள்ளு_Deletep(nodep->unlinkFrBack()), nodep);
                    திரும்பு;
                } இல்லையெனில் {
                    மாறிலி std::string suggest
                        = (nodep->param()
                               ? m_statep->suggestSymFlat(m_pinSymp, nodep->name(),
                                                          Link_முனை_Matcher_Var_Param{}, பொய்)
                               : m_statep->suggestSymFlat(m_pinSymp, nodep->name(),
                                                          Link_முனை_Matcher_Var_IO{}, பொய்));
                    மாறிலி std::string decl
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
                    VL_DO_DANGLING(தள்ளு_Deletep(nodep->unlinkFrBack()), nodep);
                    திரும்பு;
                }
            }
            VVarType refVarType = VVarType::UNKNOWN;
            தருக்க wrongPinType = பொய்;
            எனில் (AstVar* மாறிலி varp = VN_CAST(foundp->nodep(), Var)) {
                எனில் (varp->isIO() || varp->isParam() || varp->isIfaceRef()) {
                    refVarType = varp->varType();
                    nodep->modVarp(varp);
                } இல்லையெனில் {
                    wrongPinType = உண்மை;
                }
            } இல்லையெனில் எனில் (AstParamTypeDType* மாறிலி typep = VN_CAST(foundp->nodep(), ParamTypeDType)) {
                refVarType = typep->varType();
                nodep->modPTypep(typep);
            } இல்லையெனில் {
                wrongPinType = உண்மை;
            }
            // Don't connect parameter pin to module ports or vice versa
            எனில் (nodep->param() != (refVarType == VVarType::GPARAM)) wrongPinType = உண்மை;
            எனில் (wrongPinType) {
                string targetType = LinkDotState::முனை_Text_Type(foundp->nodep());
                targetType = VString::aOrAn(targetType) + ' ' + targetType;
                எனில் (nodep->param()) {
                    nodep->v3error("Instance attempts to override "
                                   << nodep->prettyNameQ() << " as a " << whatp << ", but it is "
                                   << targetType);
                } இல்லையெனில் {
                    nodep->v3error("Instance attempts to connect to "
                                   << nodep->prettyNameQ() << ", but it is " << targetType);
                }
                திரும்பு;
            }
            markAndCheckPinDup(nodep, foundp->nodep(), whatp);
        }
        // Early return() above when deleted
    }

    நிலையான தருக்க isDotThisReference(மாறிலி AstDot* nodep) {
        திரும்பு VN_IS(nodep->lhsp(), ParseRef) && nodep->lhsp()->name() == "this";
    }

    வெறுமை visit(AstDot* nodep) override {
        // Legal under a DOT: AstDot, AstParseRef, AstPackageRef, AstNodeSel, AstPackArrayDType
        //    also a DOT can be part of an expression, but only above plus
        //    AstFTaskRef are legal children
        // Dot(PackageRef, ParseRef(text))
        // Dot(Dot(ClassOrPackageRef,ClassOrPackageRef), ParseRef(text))
        // Dot(Dot(Dot(ParseRef(text), ...
        எனில் (nodep->user3SetOnce()) திரும்பு;
        LINKDOT_VISIT_START();
        UINFO(8, indent() << "visit " << nodep);
        UINFO(8, indent() << m_ds.ascii());
        மாறிலி DotStates lastStates = m_ds;
        மாறிலி தருக்க start = (m_ds.m_dotPos == DP_NONE);  // Save, as m_dotp will be changed
        மாறிலி DotPosition initialDotPos = m_ds.m_dotPos;
        VL_RESTORER(m_randSymp);
        {
            எனில் (start) {  // Starting dot sequence
                UINFOTREE(9, nodep, "", "dot-in");
                // Keep deferral from visit(AstMethodCall) for unspecialized rand target
                m_ds.init(m_curSymp, m_ds.m_unresolvedClass && randTargetUnresolved());
            }
            m_ds.m_dotp = nodep;  // Always, not just at start
            m_ds.m_dotPos = DP_FIRST;

            எனில் (isDotThisReference(nodep)) {
                VSymEnt* classSymp = getThisClassSymp();
                // In 'randomize() with { this.member }', 'this' refers to randomized
                // object, not the calling class (IEEE 1800-2023 18.7)
                எனில் (m_randSymp && m_currentWithp) classSymp = m_randSymp;
                எனில் (!classSymp && randTargetUnresolved()) {
                    // Bind 'this' in post-V3Param pass
                } இல்லையெனில் எனில் (!classSymp) {
                    nodep->v3error("'this' used outside class (IEEE 1800-2023 8.11)");
                    m_ds.m_dotErr = உண்மை;
                } இல்லையெனில் {
                    m_ds.m_dotSymp = classSymp;
                    UINFO(8, indent() << "this. " << m_ds.ascii());
                }
            } இல்லையெனில் எனில் (VN_IS(nodep->lhsp(), ParseRef) && nodep->lhsp()->name() == "super") {
                மாறிலி VSymEnt* classSymp = getThisClassSymp();
                எனில் (!classSymp) {
                    nodep->v3error("'super' used outside class (IEEE 1800-2023 8.15)");
                    m_ds.m_dotErr = உண்மை;
                } இல்லையெனில் {
                    மாறிலி auto classp = VN_AS(classSymp->nodep(), Class);
                    மாறிலி auto cextp = classp->extendsp();
                    எனில் (!cextp) {
                        nodep->v3error("'super' used on non-extended class (IEEE 1800-2023 8.15)");
                        m_ds.m_dotErr = உண்மை;
                    } இல்லையெனில் {
                        எனில் (m_statep->forPrimary()
                            && m_extendsParam.find(classp) != m_extendsParam.end()) {
                            UINFO(9, indent()
                                         << "deferring until post-V3Param: " << nodep->lhsp());
                            m_ds.m_unresolvedClass = உண்மை;
                        } இல்லையெனில் {
                            மாறிலி auto baseClassp = cextp->classp();
                            UASSERT_OBJ(baseClassp, nodep, "Bad superclass");
                            m_ds.m_dotSymp = m_statep->get_முனை_Sym(baseClassp);
                            m_ds.m_super = உண்மை;
                            UINFO(8, indent() << "super. " << m_ds.ascii());
                        }
                    }
                }
            } இல்லையெனில் எனில் (AstClassOrPackageRef* மாறிலி lhsp
                       = VN_CAST(nodep->lhsp(), ClassOrPackageRef)) {
                // m_ds.m_dotText communicates the cell prefix between stages
                UINFO(8, indent() << "iter.lhs   " << m_ds.ascii() << " " << nodep);
                iterateAndNextNull(lhsp);
                எனில் (!lhsp->classOrPackageSkipp() && lhsp->name() != "local::") {
                    revisitLater(nodep);
                    m_ds = lastStates;
                    // Resolve function args before bailing
                    எனில் (Ast_முனை_FTask_Ref* மாறிலி ftaskrefp = VN_CAST(nodep->rhsp(), முனை_FTask_Ref)) {
                        iterateAndNextNull(ftaskrefp->argsp());
                    }
                    திரும்பு;
                }
                m_ds.m_dotPos = DP_PACKAGE;
                // nodep->lhsp() may be a new node
                எனில் (AstClassOrPackageRef* மாறிலி classOrPackageRefp
                    = VN_CAST(nodep->lhsp(), ClassOrPackageRef)) {
                    எனில் (Ast_முனை* மாறிலி classOrPackageNodep
                        = classOrPackageRefp->classOrPackageSkipp()) {
                        m_ds.m_dotSymp = m_statep->get_முனை_Sym(classOrPackageNodep);
                    }
                }
                UINFO(8, indent() << "iter.ldone " << m_ds.ascii() << " " << nodep);
            } இல்லையெனில் எனில் (VN_IS(nodep->lhsp(), Dot) && VN_AS(nodep->lhsp(), Dot)->colon()) {
                // m_ds.m_dotText communicates the cell prefix between stages
                UINFO(8, indent() << "iter.lhs   " << m_ds.ascii() << " " << nodep);
                m_ds.m_dotPos = DP_PACKAGE;
                iterateAndNextNull(nodep->lhsp());
                // nodep->lhsp() may be a new node
                எனில் (AstClassOrPackageRef* மாறிலி crefp
                    = VN_CAST(nodep->lhsp(), ClassOrPackageRef)) {
                    எனில் (!crefp->classOrPackageSkipp()) {
                        revisitLater(nodep);
                        m_ds = lastStates;
                        // Resolve function args before bailing
                        எனில் (Ast_முனை_FTask_Ref* மாறிலி ftaskrefp
                            = VN_CAST(nodep->rhsp(), முனை_FTask_Ref)) {
                            iterateAndNextNull(ftaskrefp->argsp());
                        }
                        திரும்பு;
                    }
                }
                எனில் (m_lastDeferredp == nodep->lhsp()) {
                    // LHS got deferred, so this node won't be resolved. Defer it too
                    m_ds = lastStates;
                    // Resolve function args before bailing
                    எனில் (Ast_முனை_FTask_Ref* மாறிலி ftaskrefp = VN_CAST(nodep->rhsp(), முனை_FTask_Ref)) {
                        iterateAndNextNull(ftaskrefp->argsp());
                    }
                    திரும்பு;
                }
                UINFO(8, indent() << "iter.ldone " << m_ds.ascii() << " " << nodep);
            } இல்லையெனில் {
                m_ds.m_dotPos = DP_FIRST;
                m_ds.m_unresolvedGenericIface = பொய்;
                UINFO(8, indent() << "iter.lhs   " << m_ds.ascii() << " " << nodep);
                iterateAndNextNull(nodep->lhsp());
                UINFO(8, indent() << "iter.ldone " << m_ds.ascii() << " " << nodep);
                எனில் (m_ds.m_unresolvedGenericIface) {
                    m_ds.m_dotPos = initialDotPos;
                    திரும்பு;
                }
                // UINFOTREE(9, nodep, "", "dot-lho");
            }
            எனில் (m_statep->forPrimary() && isParamedClassRef(nodep->lhsp())) {
                // Dots of paramed classes will be linked after deparameterization
                UINFO(9, indent() << "deferring until post-V3Param: " << nodep->lhsp());
                m_ds.m_unresolvedClass = உண்மை;
            }
            எனில் (m_ds.m_unresolvedCell
                && (VN_IS(nodep->lhsp(), CellRef) || VN_IS(nodep->lhsp(), CellArrayRef))) {
                m_ds.m_unlinkedScopep = nodep->lhsp();
            }
            எனில் (m_ds.m_dotPos == DP_FIRST) m_ds.m_dotPos = DP_SCOPE;
            எனில் (!m_ds.m_dotErr) {  // Once something wrong, give up
                // Top 'final' dot RHS is final RHS, else it's a
                // DOT(DOT(x,*here*),real-rhs) which we consider a RHS
                எனில் (start && m_ds.m_dotPos == DP_SCOPE) m_ds.m_dotPos = DP_FINAL;
                UINFO(8, indent() << "iter.rhs   " << m_ds.ascii() << " " << nodep);
                // m_ds.m_dotSymp points at lhsp()'s symbol table, so resolve RHS under that
                iterateAndNextNull(nodep->rhsp());
                UINFO(8, indent() << "iter.rdone " << m_ds.ascii() << " " << nodep);
                // UINFOTREE(9, nodep, "", "dot-rho");
            }
            எனில் (!m_ds.m_unresolvedClass) {
                எனில் (start) {
                    Ast_முனை* newp;
                    எனில் (m_ds.m_dotErr) {
                        newp = புதிய AstConst{nodep->fileline(), AstConst::BitFalse{}};
                    } இல்லையெனில் {
                        newp = nodep->rhsp()->unlinkFrBack();
                    }
                    UINFOTREE(9, newp, "", "dot-out");
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
                } இல்லையெனில் {  // Dot midpoint
                    Ast_முனை* newp = nodep->rhsp()->unlinkFrBack();
                    எனில் (m_ds.m_unresolvedCell) {
                        AstCellRef* மாறிலி crp = புதிய AstCellRef{
                            nodep->fileline(), nodep->name(), nodep->lhsp()->unlinkFrBack(), newp};
                        newp = crp;
                    }
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
                }
            }
        }
        எனில் (start) {
            m_ds = lastStates;
        } இல்லையெனில் {
            மாறிலி தருக்க unresolvedClass = m_ds.m_unresolvedClass;
            m_ds.m_dotp = lastStates.m_dotp;
            m_ds.m_unresolvedClass |= unresolvedClass;
        }
        UINFO(8, indent() << "done " << m_ds.ascii() << " " << nodep);
    }
    வெறுமை visit(AstDefaultClocking* nodep) override {
        எனில் (VSymEnt* மாறிலி foundp = m_curSymp->findIdFallback(nodep->name())) {
            எனில் (AstClocking* மாறிலி clockingp = VN_CAST(foundp->nodep(), Clocking)) {
                clockingp->makeDefault();
                VL_DO_DANGLING(nodep->unlinkFrBack()->delete_மரம்(), nodep);
            } இல்லையெனில் {
                nodep->v3error(nodep->prettyNameQ() << " is not a clocking identifier");
            }
        } இல்லையெனில் {
            nodep->v3error("Can't find definition of clocking: " << nodep->prettyNameQ());
        }
    }
    வெறுமை visit(AstSenItem* nodep) override {
        LINKDOT_VISIT_START();
        VL_RESTORER(m_inSens);
        m_inSens = உண்மை;
        iterateChildren(nodep);
    }
    வெறுமை visit(AstPatMember* nodep) override {
        LINKDOT_VISIT_START();
        எனில் (nodep->varrefp()) திரும்பு;  // only do this mapping once
        // If we have a TEXT token as our key, lookup if it's a LPARAM
        எனில் (AstText* மாறிலி textp = VN_CAST(nodep->keyp(), Text)) {
            UINFO(9, indent() << "visit " << nodep);
            UINFO(9, indent() << "      " << textp);
            // Lookup
            எனில் (VSymEnt* மாறிலி foundp = m_curSymp->findIdFallback(textp->text())) {
                எனில் (AstVar* மாறிலி varp = VN_CAST(foundp->nodep(), Var)) {
                    எனில் (varp->isParam() || varp->isGenVar()) {
                        // Attach found Text reference to PatMember
                        nodep->varrefp(
                            புதிய AstVarRef{nodep->fileline(),
                                          foundp->imported() ? foundp->classOrPackagep() : nullptr,
                                          varp, VAccess::READ});
                        UINFO(9, indent() << " new " << nodep->varrefp());
                    }
                }
                எனில் (AstEnumItem* மாறிலி itemp = VN_CAST(foundp->nodep(), EnumItem)) {
                    // Attach enum item value to PatMember
                    nodep->varrefp(
                        புதிய AstEnumItemRef{nodep->fileline(), itemp, foundp->classOrPackagep()});
                    UINFO(9, indent() << " new " << itemp);
                }
            }
        }
        iterateChildren(nodep);
    }
    வெறுமை visit(AstParseRef* nodep) override {
        எனில் (nodep->user3SetOnce()) திரும்பு;
        LINKDOT_VISIT_START();
        UINFO(9, indent() << "visit " << nodep);
        UINFO(9, indent() << m_ds.ascii());
        எனில் (m_ds.m_unresolvedClass) திரும்பு;
        // m_curSymp is symbol table of outer expression
        // m_ds.m_dotSymp is symbol table relative to "."'s above now
        UASSERT_OBJ(m_ds.m_dotSymp, nodep, "nullptr lookup symbol table");
        // Generally resolved during Primary, but might be at param time under AstUnlinkedRef
        UASSERT_OBJ(!m_statep->forScopeCreation(), nodep, "ParseRefs should no longer exist");
        மாறிலி DotStates lastStates = m_ds;
        மாறிலி தருக்க start = (m_ds.m_dotPos == DP_NONE);  // Save, as m_dotp will be changed
        தருக்க first = start || m_ds.m_dotPos == DP_FIRST;

        எனில் (start) {
            m_ds.init(m_curSymp);
            // Note m_ds.m_dotp remains nullptr; this is a reference not under a dot
        }
        எனில் (nodep->name() == "super") {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: super");
            m_ds.m_dotErr = உண்மை;
        }
        எனில் (nodep->name() == "this") {
            iterateChildren(nodep);
            எனில் (m_statep->forPrimary()) திரும்பு;  // The class might be parameterized somewhere
            மாறிலி VSymEnt* classSymp = getThisClassSymp();
            எனில் (!classSymp) {
                nodep->v3error("'this' used outside class (IEEE 1800-2023 8.11)");
                திரும்பு;
            }
            AstClass* மாறிலி classp = VN_AS(classSymp->nodep(), Class);
            AstClassRefDType* மாறிலி dtypep
                = புதிய AstClassRefDType{nodep->fileline(), classp, nullptr};
            AstThisRef* மாறிலி newp = புதிய AstThisRef{nodep->fileline(), VFlagChildDType{}, dtypep};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
            திரும்பு;
        }
        எனில் (m_ds.m_dotPos == DP_MEMBER && VN_IS(m_ds.m_dotp->lhsp(), LambdaArgRef)
            && nodep->name() == "index") {
            // 'with' statement's 'item.index'
            // m_ds.dotp->lhsp() was checked to know if `index` is directly after lambda arg ref.
            // If not, treat it as normal member select
            iterateChildren(nodep);
            AstLambdaArgRef* மாறிலி newp = புதிய AstLambdaArgRef{
                nodep->fileline(), m_ds.m_dotp->lhsp()->name() + "__DOT__index", உண்மை};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
            திரும்பு;
        } இல்லையெனில் எனில் (m_ds.m_dotPos == DP_MEMBER) {
            // Found a Var, everything following is membership.  {scope}.{var}.HERE {member}
            எனில் (m_ds.m_dotErr) {
                nodep->unlinkFrBack();  // Avoid circular node loop on errors
            } இல்லையெனில் {
                Ast_முனை_கோவை* மாறிலி varEtcp = VN_AS(m_ds.m_dotp->lhsp()->unlinkFrBack(), முனை_கோவை);
                Ast_முனை_கோவை* மாறிலி newp = புதிய AstMemberSel{nodep->fileline(), varEtcp,  //
                                                           VFlagChildDType{}, nodep->name()};
                nodep->replaceWith(newp);
            }
            VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
        } இல்லையெனில் {
            //
            string expectWhat;
            தருக்க allowScope = பொய்;
            தருக்க allowVar = பொய்;
            தருக்க allowFTask = பொய்;
            தருக்க staticAccess = பொய்;
            எனில் (m_ds.m_disablep) {
                allowScope = உண்மை;
                allowFTask = உண்மை;
                expectWhat = "block/task";
            } இல்லையெனில் எனில் (m_ds.m_dotPos == DP_PACKAGE) {
                // {package-or-class}::{a}
                Ast_முனை_Module* classOrPackagep = nullptr;
                expectWhat = "scope/variable/func";
                allowScope = உண்மை;
                allowVar = உண்மை;
                allowFTask = உண்மை;
                staticAccess = உண்மை;
                UINFO(9, "chk pkg " << m_ds.ascii() << " lhsp=" << m_ds.m_dotp->lhsp());
                UASSERT_OBJ(VN_IS(m_ds.m_dotp->lhsp(), ClassOrPackageRef), m_ds.m_dotp->lhsp(),
                            "Bad package link");
                AstClassOrPackageRef* மாறிலி cpackagerefp
                    = VN_AS(m_ds.m_dotp->lhsp(), ClassOrPackageRef);
                எனில் (cpackagerefp->name() == "local::") {
                    m_randSymp = nullptr;
                    first = உண்மை;
                } இல்லையெனில் எனில் (!cpackagerefp->classOrPackageSkipp()) {
                    VSymEnt* மாறிலி foundp = m_statep->resolveClassOrPackage(
                        m_ds.m_dotSymp, cpackagerefp, உண்மை, பொய், ":: reference");
                    எனில் (!foundp) திரும்பு;
                    classOrPackagep = cpackagerefp->classOrPackageSkipp();
                    எனில் (classOrPackagep) {
                        m_ds.m_dotSymp = m_statep->get_முனை_Sym(classOrPackagep);
                    } இல்லையெனில் {
                        m_ds = lastStates;
                        திரும்பு;
                    }
                } இல்லையெனில் {
                    classOrPackagep = cpackagerefp->classOrPackageSkipp();
                    UASSERT_OBJ(classOrPackagep, m_ds.m_dotp->lhsp(), "Bad package link");
                    m_ds.m_dotSymp = m_statep->get_முனை_Sym(classOrPackagep);
                }
                m_ds.m_dotPos = DP_SCOPE;
            } இல்லையெனில் எனில் (m_ds.m_dotPos == DP_SCOPE || m_ds.m_dotPos == DP_FIRST) {
                // {a}.{b}, where {a} maybe a module name
                // or variable, where dotting into structure member
                expectWhat = "scope/variable";
                allowScope = உண்மை;
                allowVar = உண்மை;
            } இல்லையெனில் எனில் (m_ds.m_dotPos == DP_NONE) {
                expectWhat = "variable";
                allowVar = உண்மை;
            } இல்லையெனில் எனில் (m_ds.m_dotPos == DP_FINAL) {
                expectWhat = "variable/method";
                allowFTask = உண்மை;
                allowVar = உண்மை;
            } இல்லையெனில் {
                UINFO(1, "ds=" << m_ds.ascii());
                nodep->v3fatalSrc("Unhandled DotPosition");
            }
            // Lookup
            VSymEnt* foundp;
            string baddot;
            VSymEnt* okSymp = nullptr;
            // Restricted 'with (id_list)': only id_list names bind into target class.
            எனில் (m_randSymp
                && (!m_currentWithp || m_currentWithp->nameResolvesToTarget(nodep->name()))) {
                foundp = m_randSymp->findIdFlat(nodep->name());
                எனில் (foundp) {
                    எனில் (m_currentWithp && m_currentWithp->restricted()) {
                        m_restrictedNamesUsed.செருகு(nodep->name());
                    }
                    எனில் (!start) m_ds.m_dotPos = DP_MEMBER;
                    எனில் (!m_currentWithp) {
                        UASSERT_OBJ(m_randMethodCallp, nodep, "Expected to be under randomize()");
                        // This will start failing once complex expressions are allowed on the LHS
                        // of randomize() with args
                        UASSERT_OBJ(VN_IS(m_randMethodCallp->fromp(), VarRef), m_randMethodCallp,
                                    "Expected simple randomize target");
                        // A ParseRef is used here so that the dot RHS gets resolved
                        AstMemberSel* மாறிலி newp = புதிய AstMemberSel{
                            nodep->fileline(), m_randMethodCallp->fromp()->clone_மரம்(பொய்),
                            VFlagChildDType{}, nodep->name()};
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
                        திரும்பு;
                    }
                    UINFO(9, indent() << "randomize-with fromSym " << foundp->nodep());
                    AstLambdaArgRef* மாறிலி lambdaRefp
                        = புதிய AstLambdaArgRef{nodep->fileline(), "__Vrandwith_obj", பொய்};
                    AstMemberSel* newp = புதிய AstMemberSel{nodep->fileline(), lambdaRefp,
                                                          VFlagChildDType{}, nodep->name()};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
                    திரும்பு;
                }
            }
            எனில் (allowScope) {
                foundp = m_statep->findDotted(nodep->fileline(), m_ds.m_dotSymp, nodep->name(),
                                              baddot, okSymp, first);  // Maybe nullptr
            } இல்லையெனில் எனில் (first) {
                foundp = m_ds.m_dotSymp->findIdFallback(nodep->name());
            } இல்லையெனில் {
                foundp = m_ds.m_dotSymp->findIdFlat(nodep->name());
            }
            எனில் (!foundp && m_ds.m_dotp && isDotThisReference(m_ds.m_dotp)) {
                மாறிலி AstClass* மாறிலி classp = VN_CAST(m_ds.m_dotSymp->nodep(), Class);
                எனில் (classp && classp->isCovergroup() && classp->covergroupEnclosingClassp()) {
                    VSymEnt* மாறிலி enclosingClassSymp
                        = m_statep->get_முனை_Sym(classp->covergroupEnclosingClassp());
                    foundp = enclosingClassSymp->findIdFallback(nodep->name());
                    எனில் (foundp) m_ds.m_dotSymp = enclosingClassSymp;
                }
            }
            // If not found in modport, check interface fallback for parameters and typedefs.
            // Parameters and typedefs are always visible through a modport (IEEE 1800-2023 25.5).
            // This mirrors the VarXRef modport parameter fallback in visit(AstVarXRef).
            எனில் (!foundp && VN_IS(m_ds.m_dotSymp->nodep(), Modport)
                && m_ds.m_dotSymp->fallbackp()) {
                VSymEnt* மாறிலி ifaceFallbackp = m_ds.m_dotSymp->fallbackp();
                VSymEnt* மாறிலி ifaceFoundp = ifaceFallbackp->findIdFlat(nodep->name());
                எனில் (ifaceFoundp) {
                    எனில் (மாறிலி AstVar* மாறிலி varp = VN_CAST(ifaceFoundp->nodep(), Var)) {
                        எனில் (varp->isParam()) foundp = ifaceFoundp;
                    } இல்லையெனில் எனில் (VN_IS(ifaceFoundp->nodep(), Typedef)
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
            எனில் (!foundp && !okSymp && VN_IS(m_ds.m_dotSymp->nodep(), Modport)) {
                okSymp = m_ds.m_dotSymp;
            }
            எனில் (foundp) {
                UINFO(9, indent() << "found=se" << cvtToHex(foundp) << "  exp=" << expectWhat
                                  << "  n=" << foundp->nodep());
            }
            // What fell out?
            தருக்க ok = பொய்;
            // Special case: waiting on clocking event
            எனில் (m_inSens && foundp && m_ds.m_dotPos != DP_SCOPE && m_ds.m_dotPos != DP_FIRST) {
                எனில் (AstClocking* மாறிலி clockingp = sensClockingp(foundp->nodep())) {
                    foundp = getCreateClockingEventSymEnt(clockingp);
                }
            }
            எனில் (!foundp) {
            } இல்லையெனில் எனில் (VN_IS(foundp->nodep(), Cell) || VN_IS(foundp->nodep(), முனை_Block)
                       || VN_IS(foundp->nodep(), GenBlock)
                       || VN_IS(foundp->nodep(), Netlist)  // for $root
                       || VN_IS(foundp->nodep(), Module)) {  // if top
                எனில் (allowScope) {
                    ok = உண்மை;
                    m_ds.m_dotText = VString::dot(m_ds.m_dotText, ".", nodep->name());
                    m_ds.m_dotSymp = foundp;
                    m_ds.m_dotPos = DP_SCOPE;
                    எனில் (m_ds.m_disablep && VN_IS(foundp->nodep(), முனை_Block)) {
                        // Possibly it is not the final link. If we are under dot and not in its
                        // last component, `targetp()` field will be overwritten by next components
                        m_ds.m_disablep->targetp(foundp->nodep());
                    }
                    எனில் (VN_IS(foundp->nodep(), GenBlock)) {
                        m_ds.m_genBlk = உண்மை;
                        எனில் (m_ds.m_disablep) {
                            m_ds.m_disablep->v3warn(
                                E_UNSUPPORTED,
                                "Unsupported: Generate block referenced by disable");
                        }
                    }
                    // Upper AstDot visitor will handle it from here
                } இல்லையெனில் எனில் (VN_IS(foundp->nodep(), Cell) && allowVar) {
                    AstCell* மாறிலி cellp = VN_AS(foundp->nodep(), Cell);
                    எனில் (VN_IS(cellp->modp(), Iface)) {
                        // Interfaces can be referenced like a variable for interconnect
                        VSymEnt* மாறிலி cellEntp = m_statep->get_முனை_Sym(cellp);
                        UASSERT_OBJ(cellEntp, nodep, "No interface sym entry");
                        VSymEnt* மாறிலி parentEntp
                            = cellEntp->parentp();  // Container of the var; probably a module or
                                                    // generate begin
                        AstVar* மாறிலி ifaceRefVarp
                            = find_Iface_உச்சி_Varp(nodep, parentEntp, nodep->name());
                        //
                        ok = உண்மை;
                        m_ds.m_dotSymp = foundp;
                        m_ds.m_dotPos = DP_SCOPE;
                        UINFO(9, indent() << " cell -> iface varref " << foundp->nodep());
                        Ast_முனை_கோவை* newp;
                        எனில் (m_ds.m_dotText != "") {
                            newp = புதிய AstVarXRef{nodep->fileline(), ifaceRefVarp, m_ds.m_dotText,
                                                  VAccess::READ};
                            எனில் (m_ds.m_unresolvedCell && m_ds.m_unlinkedScopep) {
                                newp = புதிய AstUnlinkedRef{nodep->fileline(), newp, newp->name(),
                                                          m_ds.m_unlinkedScopep->unlinkFrBack()};
                                m_ds.m_unlinkedScopep = nullptr;
                                m_ds.m_unresolvedCell = பொய்;
                                m_ds.m_dotText = "";
                            }
                        } இல்லையெனில் {
                            AstVarRef* மாறிலி refp
                                = புதிய AstVarRef{nodep->fileline(), ifaceRefVarp, VAccess::READ};
                            newp = refp;
                        }
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
                    } இல்லையெனில் எனில் (VN_IS(cellp->modp(), NotFoundModule)) {
                        cellp->modNameFileline()->v3error("Cannot find file containing interface: "
                                                          << cellp->modp()->prettyNameQ());
                    }
                }
            } இல்லையெனில் எனில் (allowScope && !allowFTask && VN_IS(foundp->nodep(), முனை_FTask)) {
                // Function/task used as scope for static variable access (IEEE std 1800-2017 6.21)
                // Don't set m_dotText so the static variable resolves as a VarRef rather than
                // VarXRef. This is so that V3Begin won't move it out of the function/task's scope.
                ok = உண்மை;
                m_ds.m_dotSymp = foundp;
                m_ds.m_dotPos = DP_SCOPE;
            } இல்லையெனில் எனில் (allowFTask && VN_IS(foundp->nodep(), முனை_FTask)) {
                Ast_முனை_FTask_Ref* taskrefp;
                எனில் (VN_IS(foundp->nodep(), Task)) {
                    taskrefp = புதிய AstTaskRef{nodep->fileline(), nodep->name()};
                } இல்லையெனில் {
                    taskrefp = புதிய AstFuncRef{nodep->fileline(), nodep->name()};
                }
                nodep->replaceWith(taskrefp);
                VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
                m_ds = lastStates;
                திரும்பு;
            } இல்லையெனில் எனில் (AstVar* மாறிலி varp = foundToVarp(foundp, nodep, VAccess::READ)) {
                // Preserve explicit 'this' references to members inherited from
                // generic classes until after specialization.
                எனில் (m_statep->forPrimary() && m_ds.m_dotp && isDotThisReference(m_ds.m_dotp)
                    && foundp->classOrPackagep() != m_modp
                    && VN_IS(foundp->classOrPackagep(), Class)
                    && foundp->classOrPackagep()->hasGParam()) {
                    m_ds.m_unresolvedClass = உண்மை;
                    திரும்பு;
                }
                AstIfaceRefDType* மாறிலி ifacerefp
                    = LinkDotState::ifaceRefFromArray(varp->subDTypep());
                எனில் (varp->isIfaceRef() && m_genericIfaceModule
                    && VN_IS(varp->childDTypep(), IfaceGenericDType) && !start) {
                    // Defer only dotted member access ('d.PARAM'), as V3Param must specialize
                    // first; a standalone ref ('.x(d)') resolves via allowVar below now
                    ok = உண்மை;
                    m_ds.m_unresolvedGenericIface = உண்மை;
                } இல்லையெனில் எனில் (ifacerefp && varp->isIfaceRef()) {
                    UASSERT_OBJ(ifacerefp->ifaceViaCellp(), ifacerefp, "Unlinked interface");
                    // Really this is a scope reference into an interface
                    UINFO(9, indent() << "varref-ifaceref " << m_ds.m_dotText << "  " << nodep);
                    m_ds.m_dotText = VString::dot(m_ds.m_dotText, ".", nodep->name());
                    // If modport specified, use modport symbol table for lookup of virtual ports
                    எனில் (ifacerefp->modportp() && m_statep->exists_முனை_Sym(ifacerefp->modportp())) {
                        m_ds.m_dotSymp = m_statep->get_முனை_Sym(ifacerefp->modportp());
                    } இல்லையெனில் {
                        m_ds.m_dotSymp = m_statep->get_முனை_Sym(ifacerefp->ifaceViaCellp());
                    }
                    m_ds.m_dotPos = DP_SCOPE;
                    ok = உண்மை;

                    AstVarRef* மாறிலி refp = புதிய AstVarRef{nodep->fileline(), varp, VAccess::READ};
                    nodep->replaceWith(refp);

                    VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
                } இல்லையெனில் எனில் (allowVar) {
                    Ast_முனை* newp;
                    எனில் (m_ds.m_dotText != "") {
                        AstVarXRef* மாறிலி refp
                            = புதிய AstVarXRef{nodep->fileline(), nodep->name(), m_ds.m_dotText,
                                             VAccess::READ};  // lvalue'ness computed later
                        refp->varp(varp);
                        // Not linked again after V3LinkLValue, so that reports writing it
                        எனில் (மாறிலி AstModportVarRef* மாறிலி mvarp
                            = VN_CAST(foundp->nodep(), ModportVarRef)) {
                            எனில் (m_statep->forParamed() && mvarp->direction().isReadOnly()) {
                                refp->readOnlyModport(உண்மை);
                            }
                        }
                        refp->containsGenBlock(m_ds.m_genBlk);
                        m_ds.m_dotText = "";
                        எனில் (m_ds.m_unresolvedCell && m_ds.m_unlinkedScopep) {
                            மாறிலி string dotted = refp->dotted();
                            மாறிலி size_t pos = dotted.find("__BRA__??__KET__");
                            // Arrays of interfaces all have the same parameters
                            எனில் (pos != string::npos && varp->isParam()
                                && VN_IS(m_ds.m_unlinkedScopep, CellArrayRef)) {
                                refp->dotted(dotted.substr(0, pos));
                                newp = refp;
                            } இல்லையெனில் {
                                UINFO(9, indent() << "deferring until post-V3Param: " << refp);
                                newp = புதிய AstUnlinkedRef{nodep->fileline(), refp, refp->name(),
                                                          m_ds.m_unlinkedScopep->unlinkFrBack()};
                                m_ds.m_unlinkedScopep = nullptr;
                                m_ds.m_unresolvedCell = பொய்;
                            }
                        } இல்லையெனில் {
                            newp = refp;
                        }
                    } இல்லையெனில் {
                        எனில் (staticAccess && !varp->lifetime().isStatic() && !varp->isParam()) {
                            // TODO bug4077
                            // nodep->v3error("Static access to non-static member variable "
                            //                << varp->prettyNameQ() << endl);
                        }
                        AstVarRef* மாறிலி refp = புதிய AstVarRef{
                            nodep->fileline(), varp, VAccess::READ};  // lvalue'ness computed later
                        refp->classOrPackagep(foundp->classOrPackagep());
                        newp = refp;
                    }
                    UINFO(9, indent() << "new " << newp);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
                    m_ds.m_dotPos = DP_MEMBER;
                    ok = உண்மை;
                }
            } இல்லையெனில் எனில் (மாறிலி AstModport* மாறிலி modportp = VN_CAST(foundp->nodep(), Modport)) {
                // A scope reference into an interface's modport (not
                // necessarily at a pin connection)
                UINFO(9, indent() << "cell-ref-to-modport " << m_ds.m_dotText << "  " << nodep);
                UINFO(9, indent() << "unlinked " << m_ds.m_unlinkedScopep);
                UINFO(9,
                      indent() << "dotSymp " << m_ds.m_dotSymp << " " << m_ds.m_dotSymp->nodep());
                // Iface was the previously dotted component
                எனில் (!m_ds.m_dotSymp || !VN_IS(m_ds.m_dotSymp->nodep(), Cell)
                    || !VN_AS(m_ds.m_dotSymp->nodep(), Cell)->modp()
                    || !VN_IS(VN_AS(m_ds.m_dotSymp->nodep(), Cell)->modp(), Iface)) {
                    nodep->v3error("Modport not referenced as <interface>."
                                   << modportp->prettyNameQ());
                } இல்லையெனில் எனில் (!VN_AS(m_ds.m_dotSymp->nodep(), Cell)->modp()
                           || !VN_IS(VN_AS(m_ds.m_dotSymp->nodep(), Cell)->modp(), Iface)) {
                    nodep->v3error("Modport not referenced from underneath an interface: "
                                   << modportp->prettyNameQ());
                } இல்லையெனில் {
                    AstCell* மாறிலி cellp = VN_AS(m_ds.m_dotSymp->nodep(), Cell);
                    UASSERT_OBJ(cellp, nodep, "Modport not referenced from an instance");
                    VSymEnt* மாறிலி cellEntp = m_statep->get_முனை_Sym(cellp);
                    UASSERT_OBJ(cellEntp, nodep, "No interface sym entry");
                    VSymEnt* மாறிலி parentEntp
                        = cellEntp->parentp();  // Container of the var; probably a
                                                // module or generate begin
                    // We drop __BRA__??__KET__ as cells don't have that naming yet
                    AstVar* மாறிலி ifaceRefVarp
                        = find_Iface_உச்சி_Varp(nodep, parentEntp, cellp->name());
                    //
                    ok = உண்மை;
                    m_ds.m_dotText = VString::dot(m_ds.m_dotText, ".", nodep->name());
                    m_ds.m_dotSymp = foundp;
                    m_ds.m_dotPos = DP_SCOPE;
                    UINFO(9, indent() << "modport -> iface varref " << foundp->nodep());
                    // We lose the modport name here, so we cannot detect mismatched modports.
                    AstVarRef* மாறிலி refp
                        = புதிய AstVarRef{nodep->fileline(), ifaceRefVarp, VAccess::READ};
                    Ast_முனை_கோவை* newp = refp;

                    auto* மாறிலி cellarrayrefp = VN_CAST(m_ds.m_unlinkedScopep, CellArrayRef);
                    எனில் (cellarrayrefp) {
                        // iface[vec].modport became CellArrayRef(iface, lsb)
                        // Convert back to SelBit(iface, lsb)
                        UINFO(9, indent() << "Array modport to SelBit " << cellarrayrefp);
                        newp = புதிய AstSelBit{cellarrayrefp->fileline(), newp,
                                             cellarrayrefp->selp()->unlinkFrBack()};
                        newp->user3(உண்மை);  // Don't process again
                        VL_DO_DANGLING(தள்ளு_Deletep(cellarrayrefp->unlinkFrBack()), cellarrayrefp);
                        m_ds.m_unlinkedScopep = nullptr;
                    }
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
                }
            } இல்லையெனில் எனில் (AstConstraint* மாறிலி defp = VN_CAST(foundp->nodep(), Constraint)) {
                Ast_முனை* மாறிலி newp = புதிய AstConstraintRef{nodep->fileline(), nullptr, defp};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
                ok = உண்மை;
                m_ds.m_dotPos = DP_MEMBER;
                m_ds.m_dotText = "";
            } இல்லையெனில் எனில் (AstClass* மாறிலி defp = VN_CAST(foundp->nodep(), Class)) {
                எனில் (allowVar) {
                    எனில் (m_ds.m_dotPos == DP_SCOPE && !staticAccess) {
                        // Continue hierarchical lookup in the class scope.
                        m_ds.m_dotSymp = foundp;
                    } இல்லையெனில் {
                        AstRefDType* மாறிலி newp
                            = புதிய AstRefDType{nodep->fileline(), nodep->name()};
                        replaceWithCheckBreak(nodep, newp);
                        VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
                    }
                    ok = உண்மை;
                    m_ds.m_dotText = "";
                } இல்லையெனில் {
                    (வெறுமை)defp;  // Prevent unused variable warning
                }
            } இல்லையெனில் எனில் (AstMemberDType* மாறிலி defp = VN_CAST(foundp->nodep(), MemberDType)) {
                எனில் (allowVar) {
                    checkMemberDeclOrder(nodep, defp);
                    AstRefDType* மாறிலி refp = புதிய AstRefDType{nodep->fileline(), nodep->name()};
                    refp->refDTypep(defp);
                    replaceWithCheckBreak(nodep, refp);
                    VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
                    ok = உண்மை;
                    m_ds.m_dotText = "";
                }
            } இல்லையெனில் எனில் (AstEnumItem* மாறிலி valuep = VN_CAST(foundp->nodep(), EnumItem)) {
                எனில் (allowVar) {
                    AstEnumItemRef* மாறிலி refp
                        = புதிய AstEnumItemRef{nodep->fileline(), valuep, foundp->classOrPackagep()};
                    Ast_முனை* newp = refp;
                    // Hierarchical reference, relinked after V3Param as the referenced
                    // module may be specialized, similar to AstVarXRef
                    refp->dotted(m_ds.m_dotText);
                    refp->containsGenBlock(m_ds.m_genBlk);
                    எனில் (m_ds.m_unresolvedCell && m_ds.m_unlinkedScopep) {
                        UINFO(9, indent() << "deferring until post-V3Param: " << refp);
                        newp = புதிய AstUnlinkedRef{nodep->fileline(), refp, refp->name(),
                                                  m_ds.m_unlinkedScopep->unlinkFrBack()};
                        m_ds.m_unlinkedScopep = nullptr;
                        m_ds.m_unresolvedCell = பொய்;
                    }
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
                    ok = உண்மை;
                    m_ds.m_dotText = "";
                }
            } இல்லையெனில் எனில் (மாறிலி AstLambdaArgRef* மாறிலி argrefp
                       = VN_CAST(foundp->nodep(), LambdaArgRef)) {
                எனில் (allowVar) {
                    Ast_முனை* மாறிலி newp
                        = புதிய AstLambdaArgRef{nodep->fileline(), argrefp->name(), பொய்};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
                    ok = உண்மை;
                    m_ds.m_dotPos = DP_MEMBER;
                    m_ds.m_dotText = "";
                }
            } இல்லையெனில் எனில் (VN_IS(foundp->nodep(), Clocking)) {
                m_ds.m_dotSymp = foundp;
                எனில் (m_ds.m_dotText != "") m_ds.m_dotText += "." + nodep->name();
                ok = m_ds.m_dotPos == DP_SCOPE || m_ds.m_dotPos == DP_FIRST;
            } இல்லையெனில் எனில் (மாறிலி AstModportClockingRef* மாறிலி clockingRefp
                       = VN_CAST(foundp->nodep(), ModportClockingRef)) {
                // Clocking block accessed through a modport - redirect to actual clocking
                m_ds.m_dotSymp = m_statep->get_முனை_Sym(clockingRefp->clockingp());
                எனில் (m_ds.m_dotText != "") m_ds.m_dotText += "." + nodep->name();
                ok = m_ds.m_dotPos == DP_SCOPE || m_ds.m_dotPos == DP_FIRST;
            } இல்லையெனில் எனில் (மாறிலி Ast_முனை_FTask* மாறிலி ftaskp = VN_CAST(foundp->nodep(), முனை_FTask)) {
                எனில் (!ftaskp->isFunction() || ftaskp->classMethod()) {
                    ok = m_ds.m_dotPos == DP_NONE;
                    எனில் (ok) {
                        // The condition is true for tasks,
                        // properties and void functions.
                        // In these cases, the parentheses may be skipped.
                        // Also SV class methods can be called without parens
                        AstFuncRef* மாறிலி funcRefp
                            = புதிய AstFuncRef{nodep->fileline(), nodep->name()};
                        nodep->replaceWith(funcRefp);
                        VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
                    }
                }
            } இல்லையெனில் எனில் (AstTypedef* மாறிலி defp = VN_CAST(foundp->nodep(), Typedef)) {
                மாறிலி தருக்க ifaceFinalSegmentAllowed
                    = (m_ds.m_dotPos == DP_FINAL) && m_ds.m_dotSymp
                      && (VN_IS(m_ds.m_dotSymp->nodep(), Modport)
                          || (VN_IS(m_ds.m_dotSymp->nodep(), Cell)
                              && VN_CAST(m_ds.m_dotSymp->nodep(), Cell)->modp()
                              && VN_IS(VN_CAST(m_ds.m_dotSymp->nodep(), Cell)->modp(), Iface)));
                // Allow interface typedef references even without IfaceCapture
                // The dependency graph handles resolution when IfaceCapture is disabled
                ok = (m_ds.m_dotPos == DP_NONE || m_ds.m_dotPos == DP_SCOPE
                      || ifaceFinalSegmentAllowed);
                எனில் (ifaceFinalSegmentAllowed) {
                    UINFO(9, indent() << "allow final-segment typedef name="
                                      << nodep->prettyNameQ() << " dotText='" << m_ds.m_dotText
                                      << "' dotSym=" << m_ds.m_dotSymp);
                }
                எனில் (ok) {
                    AstRefDType* மாறிலி refp = புதிய AstRefDType{nodep->fileline(), nodep->name()};
                    // Don't check if typedef is to a <type T>::<reference> as might not be
                    // resolved yet
                    எனில் (m_ds.m_dotPos == DP_NONE) checkDeclOrder(nodep, defp);
                    refp->typedefp(defp);

                    V3LinkDotIfaceCapture::captureTypedefContext(
                        refp, "typedef", static_cast<முழு>(m_ds.m_dotPos), m_ds.m_dotText,
                        m_ds.m_dotSymp, m_modp, [இது]() { திரும்பு indent(); });

                    எனில் (VN_IS(nodep->backp(), SelExtract)) {
                        m_packedArrayDtp = refp;
                    } இல்லையெனில் {
                        replaceWithCheckBreak(nodep, refp);
                        VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
                    }
                }
            } இல்லையெனில் எனில் (AstParamTypeDType* மாறிலி defp = VN_CAST(foundp->nodep(), ParamTypeDType)) {
                ok = (m_ds.m_dotPos == DP_NONE || m_ds.m_dotPos == DP_SCOPE
                      || m_ds.m_dotPos == DP_FINAL);
                எனில் (ok) {
                    AstRefDType* மாறிலி refp = புதிய AstRefDType{nodep->fileline(), nodep->name()};
                    refp->refDTypep(defp);

                    V3LinkDotIfaceCapture::captureTypedefContext(
                        refp, "paramtype", static_cast<முழு>(m_ds.m_dotPos), m_ds.m_dotText,
                        m_ds.m_dotSymp, m_modp, [இது]() { திரும்பு indent(); });

                    எனில் (VN_IS(nodep->backp(), SelExtract)) {
                        m_packedArrayDtp = refp;
                    } இல்லையெனில் {
                        replaceWithCheckBreak(nodep, refp);
                        VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
                    }
                }
            }
            எனில் (!ok) {
                எனில் (m_insideClassExtParam) {
                    // Don't throw error if the reference is inside a class that extends a param,
                    // because some members can't be linked in such a case. m_insideClassExtParam
                    // may be true only in the first stage of linking.
                    // Mark that the Dot statement can't be resolved.
                    m_ds.m_unresolvedClass = உண்மை;
                    // If the symbol was a scope name, it would be resolved.
                    m_ds.m_dotPos = DP_MEMBER;
                } இல்லையெனில் {
                    // Cells/interfaces can't be implicit
                    மாறிலி தருக்க checkImplicit
                        = (!m_ds.m_dotp && m_ds.m_dotText == "" && !m_ds.m_disablep && !foundp);
                    மாறிலி தருக்க err
                        = !(checkImplicit && m_statep->implicitOk(m_modp, nodep->name()));
                    எனில் (err) {
                        எனில் (foundp) {
                            nodep->v3error("Found definition of '"
                                           << m_ds.m_dotText << (m_ds.m_dotText == "" ? "" : ".")
                                           << nodep->prettyName() << "'" << " as a "
                                           << foundp->nodep()->typeName() << " but expected a "
                                           << expectWhat);
                        } இல்லையெனில் எனில் (m_ds.m_dotText == "") {
                            UINFO(1, "   ErrParseRef curSymp=se" << cvtToHex(m_curSymp)
                                                                 << " ds=" << m_ds.ascii());
                            மாறிலி string suggest = m_statep->suggestSymFallback(
                                m_ds.m_dotSymp, nodep->name(), VNodeMatcher{});
                            nodep->v3error(
                                "Can't find definition of "
                                << expectWhat << ": " << nodep->prettyNameQ() << '\n'
                                << (suggest.empty() ? "" : nodep->warnMore() + suggest));
                        } இல்லையெனில் {
                            nodep->v3error(
                                "Can't find definition of "
                                << (!baddot.empty() ? Ast_முனை::prettyNameQ(baddot)
                                                    : nodep->prettyNameQ())
                                << " in dotted " << expectWhat << ": '"
                                << m_ds.m_dotText + "." + nodep->prettyName() << "'\n"
                                << nodep->warnContextPrimary()
                                << (okSymp ? okSymp->cellErrorScopes(
                                                 nodep, Ast_முனை::prettyName(m_ds.m_dotText))
                                           : ""));
                        }
                        m_ds.m_dotErr = உண்மை;
                    }
                    எனில் (checkImplicit) {
                        // Create if implicit, and also if error (so only complain once)
                        // Else if a scope is allowed, making a signal won't help error cascade
                        AstVar* மாறிலி varp
                            = createImplicitVar(m_curSymp, nodep, m_modp, m_modSymp, err);
                        AstVarRef* மாறிலி newp
                            = புதிய AstVarRef{nodep->fileline(), varp, VAccess::READ};
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
                    }
                }
            }
        }
        எனில் (start) m_ds = lastStates;
    }
    வெறுமை visit(AstClassOrPackageRef* nodep) override {
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

            எனில் (!nodep->classOrPackageSkipp() && !nodep->classOrPackageNodep()
                && nodep->name() != "local::") {
                மாறிலி தருக்க deferIfUnresolved = m_statep->forPrimary() && m_insideClassExtParam;
                m_statep->resolveClassOrPackage(m_ds.m_dotSymp, nodep, m_ds.m_dotPos != DP_PACKAGE,
                                                பொய், ":: reference", deferIfUnresolved);
            }

            // ClassRef's have pins, so track
            எனில் (nodep->classOrPackageSkipp()) {
                m_pinSymp = m_statep->get_முனை_Sym(nodep->classOrPackageSkipp());
            } இல்லையெனில் எனில் (nodep->name() != "local::") {
                திரும்பு;
            }

            AstClass* மாறிலி refClassp = VN_CAST(nodep->classOrPackageSkipp(), Class);
            // Make sure any extends() are properly imported within referenced class
            எனில் (refClassp && !m_statep->forPrimary()) classExtendImport(refClassp);

            எனில் (m_statep->forPrimary() && refClassp && !nodep->paramsp()
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
                AstPin* மாறிலி pinp = புதிய AstPin{nodep->fileline(), 1, "__paramNumber1", nullptr};
                pinp->param(உண்மை);
                nodep->addParamsp(pinp);
            }

            m_ds.init(m_curSymp);
            UINFO(4, indent() << "(Backto) Link ClassOrPackageRef: " << nodep);
            iterateChildren(nodep);
        }

        எனில் (nodep->name() == "local::") {
            எனில் (!m_randSymp && !m_randMethodCallp) {
                nodep->v3error("Illegal 'local::' outside 'randomize() with'"
                               " (IEEE 1800-2023 18.7.1)");
                m_ds.m_dotErr = உண்மை;
                திரும்பு;
            }
        }
        எனில் (m_ds.m_dotPos == DP_PACKAGE && nodep->classOrPackageSkipp()) {
            m_ds.m_dotSymp = m_statep->get_முனை_Sym(nodep->classOrPackageSkipp());
            UINFO(9, indent() << "set sym " << m_ds.ascii());
        }
    }
    வெறுமை visit(AstConstraint* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        எனில் (nodep->isExternDef()) {
            எனில் (மாறிலி VSymEnt* மாறிலி foundp
                = m_curSymp->findIdFallback("extern " + nodep->name())) {
                மாறிலி AstConstraint* மாறிலி protop = VN_AS(foundp->nodep(), Constraint);
                // Copy specifiers.
                // External definition cannot have any specifiers, so no value will be overwritten.
                nodep->isStatic(protop->isStatic());
            } இல்லையெனில் {
                nodep->v3error("extern not found that declares " + nodep->prettyNameQ());
            }
        }
        எனில் (nodep->isExternProto()) {
            எனில் (!m_curSymp->findIdFallback(nodep->name()) && nodep->isExternExplicit()) {
                nodep->v3warn(PROTOTYPEMIS, "Definition not found for extern prototype "
                                                << nodep->prettyNameQ());
            }
        }
        VL_RESTORER(m_curSymp);
        VL_RESTORER_COPY(m_ds);
        m_ds.m_dotSymp = m_curSymp = m_statep->get_முனை_Sym(nodep);
        iterateChildren(nodep);
    }
    வெறுமை visit(AstConstraintRef* nodep) override {
        எனில் (nodep->user3SetOnce()) திரும்பு;
        LINKDOT_VISIT_START();
        UINFO(8, indent() << "visit " << nodep);
        UINFO(8, indent() << m_ds.ascii());
        // No children defined
    }
    வெறுமை visit(AstVarRef* nodep) override {
        // VarRef: Resolve its reference
        // ParseRefs are used the first pass (forPrimary) so we shouldn't get can't find
        // errors here now that we have a VarRef.
        // No checkNoDot; created and iterated from a parseRef
        LINKDOT_VISIT_START();
        iterateChildren(nodep);
        எனில் (!nodep->varp()) {
            UINFO(9, indent() << "linkVarRef se" << cvtToHex(m_curSymp) << "  n=" << nodep);
            UASSERT_OBJ(m_curSymp, nodep, "nullptr lookup symbol table");
            VSymEnt* மாறிலி foundp = m_curSymp->findIdFallback(nodep->name());
            எனில் (AstVar* மாறிலி varp
                = foundp ? foundToVarp(foundp, nodep, nodep->access()) : nullptr) {
                nodep->varp(varp);
                updateVarUse(nodep->varp());
                // Generally set by parse, but might be an import
                nodep->classOrPackagep(foundp->classOrPackagep());
            }
            எனில் (VL_UNCOVERABLE(!nodep->varp())) {
                nodep->v3error("Can't find definition of signal, again: "  // LCOV_EXCL_LINE
                               << nodep->prettyNameQ());
            }
        }
        AstVarScope* vscp = nodep->varScopep();
        எனில் (vscp && vscp->user2p() != vscp && m_replaceWithAlias) {
            vscp = LinkDotScopeVisitor::getAlias(vscp);
            nodep->varp(vscp->varp());
            nodep->varScopep(vscp);
            updateVarUse(nodep->varp());
            UINFO(7, indent() << "Resolved " << nodep);  // Also prints taskp
        }
    }
    வெறுமை visit(AstVarXRef* nodep) override {
        // VarRef: Resolve its reference
        // We always link even if varp() is set, because the module we choose may change
        // due to creating new modules, flattening, etc.
        எனில் (nodep->user3SetOnce()) திரும்பு;
        LINKDOT_VISIT_START();
        UINFO(8, indent() << "visit " << nodep);
        // No checkNoDot; created and iterated from a parseRef
        எனில் (!m_modSymp) {
            // Module that is not in hierarchy.  We'll be dead code eliminating it later.
            UINFO(9, "Dead module for " << nodep);
            nodep->varp(nullptr);
        } இல்லையெனில் {
            VSymEnt* lookupSymp = m_curSymp;  // Start search at current scope
            எனில் (nodep->inlinedDots() != "") {  // Correct for current post-inlined scope
                lookupSymp = findInlinedDotsSym(nodep, nodep->inlinedDots());
            }
            string baddot;
            VSymEnt* okSymp;
            தருக்க modport;
            VSymEnt* மாறிலி dotSymp
                = findDottedRefSymp(nodep, lookupSymp, nodep->dotted(), baddot, okSymp, &modport);
            எனில் (!dotSymp) திரும்பு;

            எனில் (!m_statep->forScopeCreation()) {
                VSymEnt* foundp = nullptr;
                எனில் (modport) {
                    // This is a copy of findSymPrefixed with few modifications.
                    // This searches without a fallback like findSymPrefixed but if lookup fails it
                    // will check one fallback (fallback of modport shall be interface) and take
                    // only L and G PARAMs since they should be visible from a modport always.
                    string prefix = dotSymp->symPrefix();
                    போது (!foundp) {
                        foundp = dotSymp->findIdFlat(prefix + nodep->name());
                        எனில் (foundp) நிறுத்து;
                        UASSERT_OBJ(dotSymp->fallbackp(), nodep, "Modports shall have fallback");
                        foundp = dotSymp->fallbackp()->findIdFlat(prefix + nodep->name());
                        எனில் (மாறிலி AstVar* மாறிலி varp
                            = foundToVarp(foundp, nodep, nodep->access())) {
                            எனில் (!varp->isParam()) foundp = nullptr;
                        }
                        எனில் (prefix.empty()) நிறுத்து;
                        string nextPrefix = LinkDotState::removeLastInlineScope(prefix);
                        எனில் (prefix == nextPrefix) நிறுத்து;
                        prefix = std::move(nextPrefix);
                    }
                    baddot = nodep->name();
                } இல்லையெனில் {
                    foundp = m_statep->findSymPrefixed(dotSymp, nodep->name(), baddot, உண்மை);
                }
                எனில் (m_inSens && foundp) {
                    எனில் (AstClocking* மாறிலி clockingp = sensClockingp(foundp->nodep())) {
                        foundp = getCreateClockingEventSymEnt(clockingp);
                    }
                }
                AstVar* மாறிலி varp
                    = foundp ? foundToVarp(foundp, nodep, nodep->access()) : nullptr;
                nodep->varp(varp);
                updateVarUse(nodep->varp());
                UINFO(7, indent() << "Resolved " << nodep);  // Also prints varp
                // If found, check if it's ok to access in case it's in a hier_block
                எனில் (nodep->varp() && errorHierNonPort(nodep, nodep->varp(), dotSymp)) திரும்பு;
                எனில் (!nodep->varp()) {
                    nodep->v3error("Can't find definition of "
                                   << Ast_முனை::prettyNameQ(baddot) << " in dotted signal: '"
                                   << nodep->dotted() + "." + nodep->prettyName() << "'\n"
                                   << nodep->warnContextPrimary()
                                   << okSymp->cellErrorScopes(nodep));
                    திரும்பு;
                }
            } இல்லையெனில் {
                VSymEnt* மாறிலி foundp
                    = m_statep->findSymPrefixed(dotSymp, nodep->name(), baddot, உண்மை);
                AstVarScope* vscp = foundp ? VN_CAST(foundp->nodep(), VarScope) : nullptr;
                // Handle modport expression virtual ports
                எனில் (!vscp) vscp = findVirtualPortVarScope(dotSymp, nodep->name());
                // If found, check if it's ok to access in case it's in a hier_block
                எனில் (vscp && errorHierNonPort(nodep, vscp->varp(), dotSymp)) திரும்பு;
                எனில் (!vscp) {
                    nodep->v3error("Can't find varpin scope of "
                                   << Ast_முனை::prettyNameQ(baddot) << " in dotted signal: '"
                                   << nodep->dotted() + "." + nodep->prettyName() << "'\n"
                                   << nodep->warnContextPrimary()
                                   << okSymp->cellErrorScopes(nodep));
                } இல்லையெனில் {
                    எனில் (vscp->user2p() && m_replaceWithAlias) {
                        vscp = LinkDotScopeVisitor::getAlias(vscp);
                    }
                    // Convert the VarXRef to a VarRef, so we don't need
                    // later optimizations to deal with VarXRef.
                    nodep->varp(vscp->varp());
                    nodep->varScopep(vscp);
                    updateVarUse(nodep->varp());
                    UINFO(7, indent() << "Resolved " << nodep);  // Also prints taskp
                    AstVarRef* மாறிலி newvscp
                        = புதிய AstVarRef{nodep->fileline(), vscp, nodep->access()};
                    nodep->replaceWith(newvscp);
                    VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
                    UINFO(9, indent() << "new " << newvscp);  // Also prints taskp
                }
            }
        }
    }
    வெறுமை visit(AstEnumDType* nodep) override {
        LINKDOT_VISIT_START();
        iterateChildren(nodep);
        AstRefDType* மாறிலி refdtypep = VN_CAST(nodep->subDTypep(), RefDType);
        எனில் (refdtypep && (nodep == refdtypep->subDTypep())) {
            refdtypep->v3error("Self-referential enumerated type definition");
        }
    }
    வெறுமை visit(Ast_முனை_UOr_Struct_DType* nodep) override {
        LINKDOT_VISIT_START();
        symIterateChildren(nodep, m_statep->get_முனை_Sym(nodep));
    }
    வெறுமை visit(AstEnumItemRef* nodep) override {
        // Resolve its reference
        // EnumItemRefs are created by the first pass, but V3Param may regenerate due to
        // a parameterized class/module, so we shouldn't get can't find errors.
        // No checkNoDot; created and iterated from a parseRef
        எனில் (nodep->user3SetOnce()) திரும்பு;
        LINKDOT_VISIT_START();
        எனில் (m_statep->forParamed() && !nodep->dotted().empty() && m_modSymp) {
            // Hierarchical reference, relink as V3Param may have specialized the module
            string baddot;
            VSymEnt* okSymp = nullptr;
            VSymEnt* மாறிலி dotSymp
                = findDottedRefSymp(nodep, m_curSymp, nodep->dotted(), baddot, okSymp);
            எனில் (!dotSymp) திரும்பு;
            VSymEnt* மாறிலி foundp
                = m_statep->findSymPrefixed(dotSymp, nodep->name(), baddot, உண்மை);
            AstEnumItem* மாறிலி itemp = foundp ? VN_CAST(foundp->nodep(), EnumItem) : nullptr;
            எனில் (!itemp) {
                nodep->v3error("Enum item " << nodep->prettyNameQ() << " not found in "
                                            << Ast_முனை::prettyNameQ(nodep->dotted()) << '\n'
                                            << nodep->warnContextPrimary());
                திரும்பு;
            }
            nodep->itemp(itemp);
            UINFO(9, indent() << " relinked " << nodep);
        }
        எனில் (!nodep->itemp()) {
            UINFO(9, indent() << "linkEnumRef se" << cvtToHex(m_curSymp) << "  n=" << nodep);
            UASSERT_OBJ(m_curSymp, nodep, "nullptr lookup symbol table");
            VSymEnt* மாறிலி foundp = m_curSymp->findIdFallback(nodep->name());
            எனில் (AstEnumItem* மாறிலி itemp = foundp ? VN_CAST(foundp->nodep(), EnumItem) : nullptr) {
                nodep->itemp(itemp);
                nodep->classOrPackagep(foundp->classOrPackagep());
                UINFO(9, indent() << " resolved " << nodep);
            }
            எனில் (VL_UNCOVERABLE(!nodep->itemp())) {
                nodep->v3error("Can't find definition of enum item, again: "  // LCOV_EXCL_LINE
                               << nodep->prettyNameQ());
            }
        }
        iterateChildren(nodep);
    }
    வெறுமை visit(AstMethodCall* nodep) override {
        // Created here so should already be resolved.
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        VL_RESTORER_COPY(m_ds);
        VL_RESTORER(m_randSymp);
        VL_RESTORER(m_randMethodCallp);
        {
            m_ds.init(m_curSymp);
            எனில் (nodep->name() == "randomize" && (nodep->argsp() || nodep->withp())) {
                m_randMethodCallp = nodep;
                மாறிலி Ast_முனை_DType* fromDtp = nodep->fromp()->dtypep();
                எனில் (!fromDtp) {
                    எனில் (மாறிலி Ast_முனை_Var_Ref* மாறிலி varRefp = VN_CAST(nodep->fromp(), முனை_Var_Ref)) {
                        fromDtp = varRefp->varp()->subDTypep();
                    } இல்லையெனில் {
                        fromDtp = get_கோவை_DTypep(nodep->fromp());
                    }
                    எனில் (!fromDtp) {
                        எனில் (nodep->withp()) {
                            nodep->v3warn(
                                E_UNSUPPORTED,
                                "Unsupported: 'randomize() with' on complex expressions");
                        } இல்லையெனில் {
                            nodep->v3warn(E_UNSUPPORTED,
                                          "Unsupported: Inline random variable control with "
                                          "'randomize()' called on complex expressions");
                        }
                    }
                }
                எனில் (m_statep->forPrimary() && isParamedClassRefDType(fromDtp)) {
                    m_ds.m_unresolvedClass = உண்மை;
                } இல்லையெனில் எனில் (fromDtp) {
                    மாறிலி AstClassRefDType* மாறிலி classDtp
                        = VN_CAST(fromDtp->skipRefp(), ClassRefDType);
                    எனில் (!classDtp)
                        nodep->v3error("'randomize() with' on a non-class-instance "
                                       << fromDtp->prettyNameQ());
                    இல்லையெனில் எனில் (m_statep->forPrimary() && m_extendsParam.மொத்தம்(classDtp->classp())) {
                        // Inherited members are unavailable until the base class is specialized.
                        // Defer lookup rather than incorrectly binding names in the caller.
                        m_ds.m_unresolvedClass = உண்மை;
                    } இல்லையெனில் {
                        m_randSymp = m_statep->get_முனை_Sym(classDtp->classp());
                    }
                }
            }
            iterateChildren(nodep);
        }
    }
    வெறுமை visit(AstVar* nodep) override {
        LINKDOT_VISIT_START();
        checkNoDot(nodep);
        VL_RESTORER(m_isParam);
        m_isParam = nodep->varType().isParam();
        iterateChildren(nodep);
        எனில் (m_statep->forPrimary() && nodep->isIO() && !m_ftaskp && !nodep->user4()) {
            nodep->v3error(
                "Input/output/inout does not appear in port list: " << nodep->prettyNameQ());
        }
    }
    வெறுமை visit(AstVarScope* nodep) override {
        LINKDOT_VISIT_START();
        checkNoDot(nodep);
        iterateChildren(nodep);
        AstVarScope* aliasp = LinkDotScopeVisitor::getAlias(nodep);
        எனில் (aliasp && aliasp != nodep && !nodep->varp()->isIfaceRef()) {
            // Aliased variable might still be references from outside,
            // eg through the VPI, and is traced, so we need the value to propagate.
            // TODO: this means external writes to the LHS (e.g.: through the VPI) don't work
            AstAssignW* மாறிலி assignp = புதிய AstAssignW{
                nodep->fileline(), புதிய AstVarRef{nodep->fileline(), nodep, VAccess::WRITE},
                புதிய AstVarRef{nodep->fileline(), aliasp, VAccess::READ}};
            assignp->user2(உண்மை);
            nodep->scopep()->addBlocksp(புதிய AstAlways{assignp});
            // Propagate attributes and lint state of the replaced variable,
            // because all references to it are replaced with references to the alias variable
            aliasp->varp()->propagateAttrFrom(nodep->varp());
            aliasp->varp()->fileline()->modifyStateInherit(nodep->varp()->fileline());
        }
    }
    வெறுமை visit(Ast_முனை_FTask_Ref* nodep) override {
        எனில் (nodep->user3SetOnce()) திரும்பு;
        LINKDOT_VISIT_START();
        UINFO(8, indent() << "visit " << nodep);
        UINFO(8, indent() << m_ds.ascii());
        எனில் (m_ds.m_dotPos != DP_MEMBER || nodep->name() != "randomize") {
            // Visit arguments at the beginning.
            // They may be visitted even if the current node can't be linked now.
            symIterateChildren(nodep, m_curSymp);
        }

        எனில் (m_ds.m_super) {
            எனில் (AstFuncRef* மாறிலி funcRefp = VN_CAST(nodep, FuncRef)) {
                funcRefp->superReference(உண்மை);
            } இல்லையெனில் எனில் (AstTaskRef* மாறிலி taskRefp = VN_CAST(nodep, TaskRef)) {
                taskRefp->superReference(உண்மை);
            }
        }

        VL_RESTORER(m_randSymp);

        தருக்க first = !m_ds.m_dotp || m_ds.m_dotPos == DP_FIRST;
        தருக்க staticAccess = பொய்;
        எனில் (m_ds.m_unresolvedClass) {
            // Unable to link before V3Param
            திரும்பு;
        } இல்லையெனில் எனில் (m_ds.m_unresolvedCell && m_ds.m_dotPos == DP_FINAL && m_ds.m_unlinkedScopep) {
            Ast_முனை_FTask_Ref* மாறிலி newftaskp = nodep->clone_மரம்(பொய்);
            newftaskp->dotted(m_ds.m_dotText);
            Ast_முனை* மாறிலி newp = புதிய AstUnlinkedRef{nodep->fileline(), newftaskp, nodep->name(),
                                                     m_ds.m_unlinkedScopep->unlinkFrBack()};
            m_ds.m_unlinkedScopep = nullptr;
            m_ds.m_unresolvedCell = பொய்;
            nodep->replaceWith(newp);
            VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
            திரும்பு;
        } இல்லையெனில் எனில் (m_ds.m_dotp && m_ds.m_dotPos == DP_PACKAGE) {
            UASSERT_OBJ(VN_IS(m_ds.m_dotp->lhsp(), ClassOrPackageRef), m_ds.m_dotp->lhsp(),
                        "Bad package link");
            staticAccess = உண்மை;
            AstClassOrPackageRef* மாறிலி cpackagerefp
                = VN_AS(m_ds.m_dotp->lhsp(), ClassOrPackageRef);
            எனில் (cpackagerefp->name() == "local::") {
                m_randSymp = nullptr;
                first = உண்மை;
            } இல்லையெனில் எனில் (!cpackagerefp->classOrPackageSkipp()) {
                VSymEnt* மாறிலி foundp = m_statep->resolveClassOrPackage(
                    m_ds.m_dotSymp, cpackagerefp, உண்மை, பொய், ":: reference");
                எனில் (foundp) nodep->classOrPackagep(cpackagerefp->classOrPackageSkipp());
            } இல்லையெனில் {
                nodep->classOrPackagep(cpackagerefp->classOrPackageSkipp());
            }
            // Class/package :: HERE function() . method_called_on_function_return_value()
            m_ds.m_dotPos = DP_MEMBER;
        } இல்லையெனில் எனில் (m_ds.m_dotp && m_ds.m_dotPos == DP_FINAL) {
            nodep->dotted(m_ds.m_dotText);  // Maybe ""
            // Only flag FTaskRefs under generate-if/case blocks that may be
            // pruned.  GenFor and plain begin-blocks won't be pruned by V3Param.
            // A generate-case block's parent is the GenCaseItem, not the GenCase.
            // VarXRef uses the broader m_genBlk flag (set for all GenBlocks)
            // because genfor unrolling also removes variables.
            எனில் (m_ds.m_genBlk && m_ds.m_dotSymp) {
                மாறிலி Ast_முனை* மாறிலி blkp = m_ds.m_dotSymp->nodep();
                எனில் (VN_IS(blkp, GenBlock)
                    && (VN_IS(blkp->backp(), GenIf) || VN_IS(blkp->backp(), GenCaseItem))) {
                    nodep->containsGenBlock(உண்மை);
                }
            }
        } இல்லையெனில் எனில் (m_ds.m_dotp && m_ds.m_dotPos == DP_MEMBER) {
            // Found a Var, everything following is method call.
            // {scope}.{var}.HERE {method} ( ARGS )
            Ast_முனை_கோவை* மாறிலி varEtcp = VN_AS(m_ds.m_dotp->lhsp()->unlinkFrBack(), முனை_கோவை);
            AstArg* மாறிலி argsp = nodep->argsp();
            எனில் (argsp) argsp->unlinkFrBackWithNext();
            AstMethodCall* மாறிலி newp = புதிய AstMethodCall{nodep->fileline(), varEtcp,
                                                          VFlagChildDType{}, nodep->name(), argsp};
            எனில் (AstWith* மாறிலி withp = nodep->withp()) newp->withp(withp->unlinkFrBack());
            nodep->replaceWith(newp);
            VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
            திரும்பு;
        } இல்லையெனில் எனில் (m_ds.m_dotp && (m_ds.m_dotPos == DP_SCOPE || m_ds.m_dotPos == DP_FIRST)) {
            // HERE function() . method_called_on_function_return_value()
            m_ds.m_dotPos = DP_MEMBER;
            m_ds.m_dotText = "";
        } இல்லையெனில் எனில் (!m_ds.m_disablep) {
            // visit(AstDisable*) setup the dot handling
            checkNoDot(nodep);
        }
        எனில் (nodep->classOrPackagep() && nodep->taskp()) {
            // References into packages don't care about cell hierarchy.
        } இல்லையெனில் எனில் (!m_modSymp) {
            // Module that is not in hierarchy.  We'll be dead code eliminating it later.
            UINFO(9, indent() << "Dead module for " << nodep);
            nodep->taskp(nullptr);
        } இல்லையெனில் எனில் (nodep->dotted() == "" && nodep->taskp()) {
            // Earlier should have setup the links
            // Might be under a BEGIN we're not processing, so don't relink it
        } இல்லையெனில் {
            string baddot;
            VSymEnt* okSymp = nullptr;
            VSymEnt* dotSymp;
            எனில் (nodep->dotted().empty()) {
                // Non-'super.' dotted reference
                dotSymp = m_ds.m_dotSymp;
            } இல்லையெனில் {
                // Start search at module, as a variable of same name under a subtask isn't a
                // relevant hit however a function under a begin/end is.  So we want begins, but
                // not the function
                dotSymp = m_curSymp;
            }
            எனில் (nodep->classOrPackagep()) {  // Look only in specified package
                dotSymp = m_statep->get_முனை_Sym(nodep->classOrPackagep());
                UINFO(8, indent() << "Override classOrPackage " << dotSymp);
            } இல்லையெனில் {
                எனில் (nodep->inlinedDots() != "") {  // Correct for current post-inlined scope
                    dotSymp = findInlinedDotsSym(nodep, nodep->inlinedDots());
                }
                dotSymp = m_statep->findDotted(nodep->fileline(), dotSymp, nodep->dotted(), baddot,
                                               okSymp, உண்மை);  // Maybe nullptr
            }
            // Restricted 'with (id_list)': only id_list names bind into target class.
            எனில் (m_randSymp
                && (!m_currentWithp || m_currentWithp->nameResolvesToTarget(nodep->name()))) {
                VSymEnt* மாறிலி foundp = m_randSymp->findIdFlat(nodep->name());
                எனில் (foundp && m_currentWithp) {
                    எனில் (m_currentWithp->restricted()) {
                        m_restrictedNamesUsed.செருகு(nodep->name());
                    }
                    UINFO(9, indent() << "randomize-with fromSym " << foundp->nodep());
                    AstArg* argsp = nullptr;
                    எனில் (nodep->argsp()) {
                        iterateAndNextNull(nodep->argsp());
                        argsp = nodep->argsp()->unlinkFrBackWithNext();
                    }
                    எனில் (m_ds.m_dotPos != DP_NONE) m_ds.m_dotPos = DP_MEMBER;
                    Ast_முனை* மாறிலி newp = புதிய AstMethodCall{
                        nodep->fileline(),
                        புதிய AstLambdaArgRef{nodep->fileline(), "__Vrandwith_obj", பொய்},
                        VFlagChildDType{}, nodep->name(), argsp};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
                    திரும்பு;
                }
            }
            எனில் (first && nodep->name() == "randomize" && VN_IS(m_modp, Class)
                && !VN_IS(nodep->classOrPackagep(), Package)) {
                // need special handling to avoid falling back to std::randomize
                // Skip if classOrPackagep is a Package (i.e. std::randomize resolved earlier)
                VMemberMap memberMap;
                AstFunc* மாறிலி randFuncp = V3Randomize::newRandomizeFunc(
                    memberMap, VN_AS(m_modp, Class), nodep->name(), உண்மை, உண்மை);
                nodep->taskp(randFuncp);
                nodep->classOrPackagep(VN_AS(m_modp, Class));
                m_curSymp = m_statep->செருகு_Block(m_curSymp, nodep->name(), randFuncp, m_modp);
                // Already linked to the class randomize function; skip findSymPrefixed
                // which may find std::randomize and overwrite classOrPackagep
                திரும்பு;
            }
            VSymEnt* மாறிலி foundp
                = m_statep->findSymPrefixed(dotSymp, nodep->name(), baddot, first);
            Ast_முனை_FTask* மாறிலி taskp = foundp ? VN_CAST(foundp->nodep(), முனை_FTask) : nullptr;
            Ast_முனை_Module* மாறிலி taskModulep = foundp ? foundp->classOrPackagep() : nullptr;
            // Ignore deferring linking of functions inside params of classes extending
            // parametric classes if referenced function can be linked in the first pass.
            எனில் (!m_isParam || !taskp
                || (taskp->classMethod() && taskModulep && taskModulep->hasParameterList())) {
                எனில் (m_insideClassExtParam) {
                    // The reference may point to a method declared in a super class, which is
                    // proved by a parameter. In such a case, it can't be linked at the first
                    // stage. Must not do any linking, because e.g. might find an extends of an
                    // upper class because the current class (under parent) isn't yet importing
                    // it's extended class symbols.
                    திரும்பு;
                }
                எனில் (AstClass* மாறிலி targetClassp = VN_CAST(dotSymp->nodep(), Class)) {
                    எனில் (m_extendsParam.மொத்தம்(targetClassp)) {
                        // Target class has parameterized extends not yet resolved.
                        // Its inherited symbols (e.g. static functions from the base class)
                        // aren't imported yet - defer to linkDotParamed.
                        திரும்பு;
                    }
                }
            }

            எனில் (taskp) {
                எனில் (staticAccess && !taskp->isStatic()) {
                    // TODO bug4077
                    // nodep->v3error("Static access to non-static task/function "
                    //                << taskp->prettyNameQ() << endl);
                }
                nodep->taskp(taskp);
                nodep->classOrPackagep(foundp->classOrPackagep());
                UINFO(7, indent() << "Resolved " << nodep);  // Also prints taskp
            } இல்லையெனில் {
                // Note ParseRef has similar error handling/message output
                UINFO(7, indent() << "   ErrFtask curSymp=se" << cvtToHex(m_curSymp)
                                  << " dotSymp=se" << cvtToHex(dotSymp));
                எனில் (foundp) {
                    எனில் (VN_IS(foundp->nodep(), Var) && m_ds.m_dotText == "" && m_ftaskp
                        && m_ftaskp->name() == foundp->nodep()->name()) {
                        // This is a recursive reference to the function itself, not to the var
                        nodep->taskp(m_ftaskp);
                        nodep->classOrPackagep(foundp->classOrPackagep());
                        UINFO(7, indent() << "Resolved recursive " << nodep);  // Also prints taskp
                    } இல்லையெனில் {
                        nodep->v3error("Found definition of '"
                                       << m_ds.m_dotText << (m_ds.m_dotText == "" ? "" : ".")
                                       << nodep->prettyName() << "'" << " as a "
                                       << foundp->nodep()->typeName()
                                       << " but expected a task/function");
                    }
                } இல்லையெனில் எனில் (VN_IS(nodep, New) && !m_statep->forScopeCreation()) {
                    // Resolved in V3Width
                } இல்லையெனில் எனில் ((nodep->name() == "pre_randomize" || nodep->name() == "post_randomize")
                           && VN_IS(dotSymp->nodep(), Class)) {
                    AstClass* மாறிலி classp = VN_AS(dotSymp->nodep(), Class);
                    // Functions that might be defined by user, but if not, they get empty
                    // definitions within referenced class.
                    // TODO perhaps cleaner to add to symbol table up-front with
                    // a lambda to construct them on a reference.
                    AstFunc* மாறிலி funcp
                        = புதிய AstFunc{nodep->fileline(), nodep->name(), nullptr, nullptr};
                    funcp->classMethod(உண்மை);
                    funcp->dtypep(funcp->findVoidDType());
                    classp->addMembersp(funcp);
                    m_statep->செருகு_Block(dotSymp, nodep->name(), funcp, classp);
                    nodep->taskp(funcp);
                    nodep->classOrPackagep(classp);
                    UINFO(9, indent() << " Auto-made " << funcp);
                    திரும்பு;
                } இல்லையெனில் எனில் (nodep->name() == "randomize" || nodep->name() == "srandom"
                           || nodep->name() == "get_randstate" || nodep->name() == "set_randstate"
                           || nodep->name() == "rand_mode" || nodep->name() == "constraint_mode") {
                    எனில் (AstClass* மாறிலி classp = VN_CAST(m_modp, Class)) {
                        // Don't overwrite if already resolved to a package (e.g. std::randomize)
                        எனில் (!VN_IS(nodep->classOrPackagep(), Package))
                            nodep->classOrPackagep(classp);
                    } இல்லையெனில் எனில் (nodep->name() == "randomize") {
                        // A std::randomize resolved in V3Width
                        nodep->classOrPackagep(v3Global.rootp()->stdPackagep());
                    } இல்லையெனில் {
                        nodep->v3error("Calling implicit class method "
                                       << nodep->prettyNameQ() << " without being under class");
                        nodep->replaceWith(புதிய AstConst{nodep->fileline(), 0});
                        VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
                        திரும்பு;
                    }
                } இல்லையெனில் எனில் (nodep->dotted() == "") {
                    எனில் (nodep->pli()) {
                        எனில் (v3Global.opt.bboxSys()) {
                            Ast_முனை* newp;
                            எனில் (VN_IS(nodep, FuncRef)) {
                                newp = புதிய AstConst{nodep->fileline(), AstConst::All0{}};
                            } இல்லையெனில் {
                                Ast_முனை_கோவை* outp = nullptr;
                                போது (AstArg* மாறிலி argp = nodep->argsp()) {
                                    outp = Ast_முனை::addNext(outp, argp->exprp()->unlinkFrBack());
                                    VL_DO_DANGLING(தள்ளு_Deletep(argp->unlinkFrBack()), argp);
                                }
                                newp = புதிய AstSysIgnore{nodep->fileline(), outp};
                                newp->dtypep(nodep->dtypep());
                            }
                            nodep->replaceWith(newp);
                            VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
                            திரும்பு;
                        } இல்லையெனில் {
                            VSpellCheck speller;
                            V3Parse::candidatePli(&speller);
                            மாறிலி string suggest = speller.best_Candidate_செய்தி(nodep->prettyName());
                            nodep->v3error(
                                "Unsupported or unknown PLI call: "
                                << nodep->prettyNameQ() << '\n'
                                << (suggest.empty() ? "" : nodep->warnMore() + suggest));
                        }
                    } இல்லையெனில் {
                        மாறிலி string suggest = m_statep->suggestSymFallback(
                            dotSymp, nodep->name(), Link_முனை_Matcher_FTask{});
                        nodep->v3error("Can't find definition of task/function: "
                                       << nodep->prettyNameQ() << '\n'
                                       << (suggest.empty() ? "" : nodep->warnMore() + suggest));
                    }
                } இல்லையெனில் {
                    மாறிலி string suggest = m_statep->suggestSymFallback(dotSymp, nodep->name(),
                                                                        Link_முனை_Matcher_FTask{});
                    nodep->v3error("Can't find definition of "
                                   << Ast_முனை::prettyNameQ(baddot) << " in dotted task/function: '"
                                   << nodep->dotted() + "." + nodep->prettyName() << "'\n"
                                   << (suggest.empty() ? "" : nodep->warnMore() + suggest) << '\n'
                                   << nodep->warnContextPrimary()
                                   << okSymp->cellErrorScopes(nodep));
                }
            }
        }
    }
    வெறுமை visit(AstSelBit* nodep) override {
        எனில் (nodep->user3SetOnce()) திரும்பு;
        LINKDOT_VISIT_START();
        iterateAndNextNull(nodep->fromp());
        எனில் (m_ds.m_unresolvedClass) {
            UASSERT_OBJ(m_ds.m_dotPos != DP_SCOPE && m_ds.m_dotPos != DP_FIRST, nodep,
                        "Object of unresolved class on scope position in dotted reference");
            // The index does not depend on the class, and V3Param must see its class references
            symIterateNull(nodep->bitp(), m_curSymp);
            symIterateNull(nodep->attrp(), m_curSymp);
            திரும்பு;
        }
        எனில் (m_ds.m_dotPos == DP_SCOPE
            || m_ds.m_dotPos
                   == DP_FIRST) {  // Already under dot, so this is {modulepart} DOT {modulepart}
            UINFO(9, indent() << "deferring until after a V3Param pass: " << nodep);
            m_ds.m_dotText += "__BRA__??__KET__";
            m_ds.m_unresolvedCell = உண்மை;
            // And pass up m_ds.m_dotText
        }
        // Pass dot state down to only fromp()
        iterateAndNextNull(nodep->fromp());
        symIterateNull(nodep->bitp(), m_curSymp);
        symIterateNull(nodep->attrp(), m_curSymp);
        எனில் (m_ds.m_unresolvedCell && (m_ds.m_dotPos == DP_SCOPE || m_ds.m_dotPos == DP_FIRST)) {
            Ast_முனை_கோவை* மாறிலி exprp = nodep->bitp()->unlinkFrBack();
            எனில் (AstCellArrayRef* மாறிலி fromArrRefp = VN_CAST(nodep->fromp(), CellArrayRef)) {
                // Multi-dim iface array access: append this select to the existing chain
                fromArrRefp->addSelp(exprp);
                AstCellArrayRef* மாறிலி movedp = fromArrRefp->unlinkFrBack();
                nodep->replaceWith(movedp);
                VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
            } இல்லையெனில் {
                AstCellArrayRef* மாறிலி newp
                    = புதிய AstCellArrayRef{nodep->fileline(), nodep->fromp()->name(), exprp};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
            }
        }
    }
    வெறுமை visit(Ast_முனை_Pre_Sel* nodep) override {
        // Excludes simple AstSelBit, see above
        எனில் (nodep->user3SetOnce()) திரும்பு;
        LINKDOT_VISIT_START();
        எனில் (m_ds.m_dotPos == DP_SCOPE
            || m_ds.m_dotPos
                   == DP_FIRST) {  // Already under dot, so this is {modulepart} DOT {modulepart}
            nodep->v3error("Syntax error: Range ':', '+:' etc are not allowed in the instance "
                           "part of a dotted reference");
            m_ds.m_dotErr = உண்மை;
            திரும்பு;
        }
        Ast_முனை_DType* packedArrayDtp = nullptr;  // Datatype reference for packed array
        {
            VL_RESTORER(m_packedArrayDtp);
            iterateAndNextNull(nodep->fromp());
            symIterateNull(nodep->rhsp(), m_curSymp);
            symIterateNull(nodep->thsp(), m_curSymp);

            எனில் (m_packedArrayDtp) {
                AstRange* மாறிலி newRangep
                    = புதிய AstRange(nodep->fileline(), nodep->rhsp()->unlinkFrBack(),
                                   nodep->thsp()->unlinkFrBack());
                AstPackArrayDType* newArrayTypep
                    = புதிய AstPackArrayDType(nodep->fileline(), m_packedArrayDtp, newRangep);
                newArrayTypep->childDTypep(m_packedArrayDtp);
                newArrayTypep->refDTypep(nullptr);
                எனில் (VN_IS(nodep->backp(), SelExtract)) {
                    packedArrayDtp = newArrayTypep;
                } இல்லையெனில் {
                    replaceWithCheckBreak(nodep, newArrayTypep);
                    VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
                }
            } இல்லையெனில் {
                எனில் (nodep->attrp()) {
                    Ast_முனை* மாறிலி attrp = nodep->attrp()->unlinkFrBack();
                    VL_DO_DANGLING(attrp->delete_மரம்(), attrp);
                }
                Ast_முனை* மாறிலி basefromp = nodep->baseFromp(பொய்);
                எனில் (VN_IS(basefromp, Replicate)) {
                    // From {...}[...] syntax in IEEE 2017
                    எனில் (basefromp) UINFO(9, indent() << " Related node: " << basefromp);
                } இல்லையெனில் {
                    nodep->attrp(புதிய AstAttrOf{nodep->fileline(), VAttrType::VAR_BASE,
                                               basefromp->clone_மரம்(பொய்)});
                }
            }
        }

        எனில் (packedArrayDtp) m_packedArrayDtp = packedArrayDtp;
    }
    வெறுமை visit(AstMemberSel* nodep) override {
        // checkNoDot not appropriate, can be under a dot
        LINKDOT_VISIT_START();
        iterateChildren(nodep);
    }
    வெறுமை visit(AstGenBlock* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        {
            VL_RESTORER(m_curSymp);
            VL_RESTORER_COPY(m_ds);
            எனில் (nodep->name() != "") {
                m_ds.m_dotSymp = m_curSymp = m_statep->get_முனை_Sym(nodep);
                UINFO(5, indent() << "cur=se" << cvtToHex(m_curSymp));
            }
            iterateChildren(nodep);
        }
        UINFO(5, indent() << "cur=se" << cvtToHex(m_curSymp));
    }
    வெறுமை visit(Ast_முனை_Block* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        {
            VL_RESTORER(m_curSymp);
            VL_RESTORER_COPY(m_ds);
            எனில் (nodep->name() != "") {
                m_ds.m_dotSymp = m_curSymp = m_statep->get_முனை_Sym(nodep);
                UINFO(5, indent() << "cur=se" << cvtToHex(m_curSymp));
            }
            iterateChildren(nodep);
        }
        UINFO(5, indent() << "cur=se" << cvtToHex(m_curSymp));
    }
    வெறுமை visit(Ast_முனை_FTask* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        எனில் (nodep->isExternDef() && !nodep->didProto()) {
            nodep->didProto(உண்மை);
            எனில் (மாறிலி VSymEnt* மாறிலி foundp
                = m_curSymp->findIdFallback("extern " + nodep->name())) {
                மாறிலி Ast_முனை_FTask* மாறிலி protop = VN_AS(foundp->nodep(), முனை_FTask);
                // Copy specifiers.
                // External definition cannot have any specifiers, so no value will be overwritten.
                nodep->isHideLocal(protop->isHideLocal());
                nodep->isHideProtected(protop->isHideProtected());
                nodep->isStatic(protop->isStatic());
                nodep->isVirtual(protop->isVirtual());
                nodep->lifetime(protop->lifetime());
                // Always add an FUNC_RETURN_PROTO even if task, so we can locate the prototype
                nodep->addStmtsp(புதிய AstAttrOf{protop->fileline(), VAttrType::FUNC_RETURN_PROTO,
                                               protop->fvarp()
                                                   ? protop->fvarp()->clone_மரம்(பொய்)
                                                   : புதிய AstVoidDType{protop->fileline()}});
                க்கு (Ast_முனை* stmtp = protop->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    எனில் (AstVar* மாறிலி portp = VN_CAST(stmtp, Var)) {
                        AstAttrOf* மாறிலி attrp
                            = புதிய AstAttrOf{protop->fileline(), VAttrType::FUNC_ARG_PROTO,
                                            portp->subDTypep()->clone_மரம்(பொய்)};
                        attrp->name(portp->name());
                        nodep->addStmtsp(attrp);
                    }
                }
            } இல்லையெனில் {
                nodep->v3error("extern not found that declares " + nodep->prettyNameQ());
            }
        }
        எனில் (nodep->isExternProto()) {
            எனில் (!m_curSymp->findIdFallback(nodep->name())) {
                nodep->v3warn(PROTOTYPEMIS,
                              "Definition not found for extern prototype " + nodep->prettyNameQ());
            }
        }
        VL_RESTORER(m_curSymp);
        VL_RESTORER(m_ftaskp);
        VL_RESTORER(m_explicitSuperNewp);
        {
            m_ftaskp = nodep;
            m_ds.m_dotSymp = m_curSymp = m_statep->get_முனை_Sym(nodep);
            மாறிலி தருக்க isNew = nodep->name() == "new";
            எனில் (isNew) m_explicitSuperNewp = nullptr;
            iterateChildren(nodep);
            எனில் (isNew) {
                மாறிலி AstClassExtends* மாறிலி classExtendsp = VN_AS(m_modp, Class)->extendsp();
                எனில் (m_explicitSuperNewp && !m_explicitSuperNewp->isImplicit() && classExtendsp
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
                எனில் (classExtendsp && classExtendsp->classOrNullp()) {
                    எனில் (!m_explicitSuperNewp && m_statep->forParamed()
                        && !VN_AS(classExtendsp->childDTypep()->skipRefp(), ClassRefDType)
                                ->classp()
                                ->isInterfaceClass()) {
                        Ast_முனை_Stmt* மாறிலி superNewp
                            = addImplicitSuperNewCall(VN_AS(nodep, Func), classExtendsp);
                        UINFO(9, "created super new " << superNewp);
                        iterate(superNewp);
                    }
                }
            }
        }
        m_ds.m_dotSymp = VL_RESTORER_PREV(m_curSymp);
    }
    வெறுமை visit(Ast_முனை_Foreach* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        symIterateChildren(nodep, m_statep->get_முனை_Sym(nodep));
    }
    வெறுமை visit(AstRandSequence* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        எனில் (!nodep->start().empty())
            nodep->prodp(findProd(nodep, m_statep->get_முனை_Sym(nodep), nodep->start()));
        symIterateChildren(nodep, m_statep->get_முனை_Sym(nodep));
    }
    வெறுமை visit(AstRSProd* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        symIterateChildren(nodep, m_statep->get_முனை_Sym(nodep));
    }
    வெறுமை visit(AstRSProdItem* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        nodep->prodp(findProd(nodep, m_curSymp, nodep->name()));
        iterateChildren(nodep);
    }
    வெறுமை visit(AstCoverWith* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        iterateAndNextNull(nodep->subp());
        // The type of 'item' is that of the coverpoint expression, which may name 'item' too
        iterateAndNextNull(nodep->itemp()->childDTypep());
        VL_RESTORER(m_curSymp);
        m_ds.m_dotSymp = m_curSymp = m_statep->get_முனை_Sym(nodep);
        iterateAndNextNull(nodep->itemp());
        iterateAndNextNull(nodep->filterp());
        m_ds.m_dotSymp = VL_RESTORER_PREV(m_curSymp);
    }
    வெறுமை visit(AstCoverCrossWith* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        iterateAndNextNull(nodep->selectp());
        iterateAndNextNull(nodep->countp());
        // The type of a cross item is that of its coverpoint's expression, which may name items
        iterateAndNextNull(nodep->varsp());
        VL_RESTORER(m_curSymp);
        m_ds.m_dotSymp = m_curSymp = m_statep->get_முனை_Sym(nodep);
        iterateAndNextNull(nodep->filterp());
        m_ds.m_dotSymp = VL_RESTORER_PREV(m_curSymp);
    }
    வெறுமை visit(AstWith* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        எனில் (m_ds.m_unresolvedClass) திரும்பு;
        VL_RESTORER(m_curSymp);
        VL_RESTORER(m_currentWithp);
        VL_RESTORER_COPY(m_restrictedNamesUsed);
        {
            m_ds.m_dotSymp = m_curSymp = m_statep->get_முனை_Sym(nodep);
            m_currentWithp = nodep;
            // Validate the identifier_list once per AstWith (m_validated travels
            // with cloneTree, so cloned witnesses skip the redundant check).
            மாறிலி தருக்க firstVisit = !nodep->validated();
            எனில் (firstVisit) {
                nodep->validated(உண்மை);
                m_restrictedNamesUsed.clear();
                // IEEE 1800-2023 18.7: each name in the identifier_list must designate
                // a member of the randomize() target class.
                எனில் (nodep->restricted() && m_randSymp) {
                    க்கு (மாறிலி std::string& n : nodep->restrictedNames()) {
                        எனில் (!m_randSymp->findIdFlat(n)) {
                            nodep->v3error("Identifier '"
                                           << n
                                           << "' in 'with (...)' identifier list is not a"
                                              " member of the randomize() target class"
                                              " (IEEE 1800-2023 18.7)");
                            m_restrictedNamesUsed.செருகு(n);
                        }
                    }
                }
            }
            iterateChildren(nodep);
            // Flag names that the user listed but never referenced in the constraint.
            எனில் (firstVisit && nodep->restricted()) {
                க்கு (மாறிலி std::string& n : nodep->restrictedNames()) {
                    எனில் (!m_restrictedNamesUsed.மொத்தம்(n)) {
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
    வெறுமை visit(AstLambdaArgRef* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        // No checknodot(nodep), visit(AstScope) will check for LambdaArgRef
        iterateChildren(nodep);
    }
    வெறுமை visit(AstClassExtends* nodep) override {
        // Resolve the symbol and get the class.
        // If it is a parameterized case, the class will be resolved after V3Param.cpp
        எனில் (nodep->user3SetOnce()) திரும்பு;
        // If the class is resolved, there is nothing more to do
        எனில் (nodep->classOrNullp()) திரும்பு;
        LINKDOT_VISIT_START();
        எனில் (m_statep->forPrimary()) {
            எனில் (nodep->childDTypep()) திரும்பு;
            Ast_முனை* cprp = nodep->classOrPkgsp();
            VSymEnt* lookSymp = m_curSymp;
            எனில் (AstDot* மாறிலி dotp = VN_CAST(cprp, Dot)) {
                dotp->user3(உண்மை);
                எனில் (AstClassOrPackageRef* lookNodep = VN_CAST(dotp->lhsp(), ClassOrPackageRef)) {
                    iterate(lookNodep);
                    cprp = dotp->rhsp();
                    VSymEnt* மாறிலி foundp = m_statep->resolveClassOrPackage(
                        lookSymp, lookNodep, உண்மை, பொய், nodep->verilogKwd());
                    எனில் (!foundp) {
                        VL_DO_DANGLING(தள்ளு_Deletep(nodep->unlinkFrBack()), nodep);
                        திரும்பு;
                    }
                    UASSERT_OBJ(lookNodep->classOrPackageSkipp(), nodep, "Bad package link");
                    lookSymp = m_statep->get_முனை_Sym(lookNodep->classOrPackageSkipp());
                } இல்லையெனில் {
                    dotp->lhsp()->v3error("Attempting to extend"  // LCOV_EXCL_LINE
                                          " using non-class under dot");
                }
            }
            AstClassOrPackageRef* மாறிலி cpackagerefp = VN_CAST(cprp, ClassOrPackageRef);
            எனில் (VL_UNCOVERABLE(!cpackagerefp)) {
                // Linking the extend gives an error before this is hit
                nodep->v3error("Attempting to extend using non-class");  // LCOV_EXCL_LINE
                VL_DO_DANGLING(தள்ளு_Deletep(nodep->unlinkFrBack()), nodep);
                திரும்பு;
            }
            VSymEnt* மாறிலி foundp = m_statep->resolveClassOrPackage(lookSymp, cpackagerefp, உண்மை,
                                                                    உண்மை, nodep->verilogKwd());
            எனில் (!foundp) {
                VL_DO_DANGLING(தள்ளு_Deletep(nodep->unlinkFrBack()), nodep);
                திரும்பு;
            }
            எனில் (AstClass* மாறிலி classp = VN_CAST(foundp->nodep(), Class)) {
                AstPin* paramsp = cpackagerefp->paramsp();
                எனில் (paramsp) {
                    paramsp = paramsp->clone_மரம்(உண்மை);
                    nodep->parameterized(உண்மை);
                }
                nodep->childDTypep(புதிய AstClassRefDType{nodep->fileline(), classp, paramsp});
                // Link pins
                iterate(nodep->childDTypep());
            } இல்லையெனில் எனில் (AstParamTypeDType* மாறிலி paramp
                       = VN_CAST(foundp->nodep(), ParamTypeDType)) {
                AstRefDType* மாறிலி refParamp = புதிய AstRefDType{nodep->fileline(), paramp->name()};
                refParamp->refDTypep(paramp);
                nodep->childDTypep(refParamp);
                nodep->parameterized(உண்மை);
            } இல்லையெனில் எனில் (AstTypedef* மாறிலி typedefp = VN_CAST(foundp->nodep(), Typedef)) {
                Ast_முனை_DType* மாறிலி unwrappedp = typedefp->subDTypep()->skipRefp();
                எனில் (AstClassRefDType* மாறிலி classRefp = VN_CAST(unwrappedp, ClassRefDType)) {
                    AstPin* paramsp = cpackagerefp->paramsp();
                    எனில் (!paramsp && classRefp->paramsp()) {
                        // No explicit #(...) on extends clause; carry over the
                        // typedef's type-parameter pins so V3Param sees them.
                        paramsp = classRefp->paramsp()->clone_மரம்(உண்மை);
                        nodep->parameterized(உண்மை);
                    } இல்லையெனில் எனில் (paramsp) {
                        paramsp = paramsp->clone_மரம்(உண்மை);
                        nodep->parameterized(உண்மை);
                    }
                    nodep->childDTypep(
                        புதிய AstClassRefDType{nodep->fileline(), classRefp->classp(), paramsp});
                    iterate(nodep->childDTypep());
                } இல்லையெனில் {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: " << foundp->nodep()->prettyTypeName()
                                                  << " in 'class extends'");
                    திரும்பு;
                }
            } இல்லையெனில் {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: " << foundp->nodep()->prettyTypeName()
                                                             << " in 'class extends'");
                திரும்பு;
            }
            எனில் (!nodep->childDTypep()) nodep->v3error("Attempting to extend using non-class");
            nodep->classOrPkgsp()->unlinkFrBack()->delete_மரம்();
        } இல்லையெனில் {
            // Probably a parameter
            எனில் (AstRefDType* மாறிலி refp = VN_CAST(nodep->childDTypep(), RefDType)) {
                எனில் (AstClassRefDType* classRefp = VN_CAST(refp->skipRefp(), ClassRefDType)) {
                    // Resolved to a class reference.
                    refp->replaceWith(classRefp->clone_மரம்(பொய்));
                    VL_DO_DANGLING(தள்ளு_Deletep(refp), refp);
                } இல்லையெனில் {
                    // Unable to resolve the ref type to a class reference.
                    // Get the value of type parameter passed to the class instance,
                    // to print the helpful error message.
                    மாறிலி Ast_முனை_DType* typep = refp->refDTypep();
                    போது (உண்மை) {
                        எனில் (மாறிலி AstParamTypeDType* மாறிலி atypep
                            = VN_CAST(typep, ParamTypeDType)) {
                            typep = atypep->subDTypep();
                            தொடர்;
                        }
                        எனில் (மாறிலி AstRequireDType* மாறிலி atypep = VN_CAST(typep, RequireDType)) {
                            typep = atypep->subDTypep();
                            தொடர்;
                        }
                        நிறுத்து;
                    }
                    typep->v3error("Attempting to extend using non-class");
                }
            }
        }
    }
    வெறுமை visit(AstClass* nodep) override {
        எனில் (nodep->user3SetOnce()) திரும்பு;
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
            m_ds.m_dotSymp = m_curSymp = m_modSymp = m_statep->get_முனை_Sym(nodep);
            m_modp = nodep;
            முழு next = 0;
            க்கு (AstClassExtends* cextp = nodep->extendsp(); cextp;
                 cextp = VN_AS(cextp->nextp(), ClassExtends)) {
                // Replace abstract reference with hard pointer
                // Will need later resolution when deal with parameters
                எனில் (++next == 2 && !nodep->isInterfaceClass() && !cextp->isImplements()) {
                    cextp->v3error("Multiple inheritance illegal on non-interface classes"
                                   " (IEEE 1800-2023 8.13)");
                }
                iterate(cextp);
                எனில் (m_statep->forPrimary()) {
                    எனில் (cextp->parameterized()) {
                        // Parameters in extends statement.
                        // The class can't be resolved in the current pass.
                        m_extendsParam.செருகு(nodep);
                        m_insideClassExtParam = உண்மை;
                    }
                    எனில் (AstClassRefDType* மாறிலி classRefp
                        = VN_CAST(cextp->childDTypep(), ClassRefDType)) {
                        AstClass* மாறிலி classp = classRefp->classp();
                        எனில் (classp != nodep) iterate(classp);
                        எனில் (m_extendsParam.find(classp) != m_extendsParam.end()) {
                            // One of its super classes has parameters in extends statement.
                            // Some links may not be resolved in the first pass.
                            m_extendsParam.செருகு(nodep);
                            m_insideClassExtParam = உண்மை;
                        }
                    }
                }

                எனில் (AstClass* மாறிலி baseClassp = cextp->classOrNullp()) {
                    // Already converted. Update symbol table to link unlinked members.
                    // Base class has to be visited in a case if its extends statement
                    // needs to be handled. Recursive inheritance was already checked.
                    // Must be here instead of in LinkDotParam to handle
                    // "class (type T) extends T".
                    எனில் (baseClassp == nodep) {
                        cextp->v3error("Attempting to extend class " << nodep->prettyNameQ()
                                                                     << " from itself");
                    } இல்லையெனில் எனில் (extendsClass(baseClassp, nodep)) {
                        cextp->v3error("Attempting to extend class "
                                       << nodep->prettyNameQ() << " from "
                                       << baseClassp->prettyNameQ()
                                       << ", which circularly inherits from it");
                    } இல்லையெனில் எனில் (cextp->isImplements() && !baseClassp->isInterfaceClass()) {
                        cextp->v3error("Attempting to implement from non-interface class "
                                       << baseClassp->prettyNameQ() << '\n'
                                       << "... Suggest use 'extends'");
                    } இல்லையெனில் எனில் (!cextp->isImplements() && !nodep->isInterfaceClass()
                               && baseClassp->isInterfaceClass()) {
                        cextp->v3error("Attempting to extend from interface class "
                                       << baseClassp->prettyNameQ() << '\n'
                                       << "... Suggest use 'implements'");
                    }
                    baseClassp->isExtended(உண்மை);
                    nodep->isExtended(உண்மை);
                    iterate(baseClassp);
                    importSymbolsFromExtended(nodep, cextp);
                    தொடர்;
                }
            }
            m_ds.m_dotSymp = m_curSymp;

            iterateChildren(nodep);
            க்கு (மாறிலி auto& itr : m_ifForwards) {
                மாறிலி VSymEnt* மாறிலி implSymp = itr.second;
                addInterfaceForward(nodep, VN_AS(implSymp->nodep(), முனை_FTask),
                                    VN_AS(implSymp->parentp()->nodep(), Class));
            }
        }
        // V3Width when determines types needs to find enum values and such
        // so add members pointing to appropriate enum values
        {
            VMemberMap memberMap;
            க்கு (VSymEnt::const_iterator it = m_curSymp->begin(); it != m_curSymp->end(); ++it) {
                Ast_முனை* மாறிலி itemp = it->second->nodep();
                எனில் (!memberMap.findMember(nodep, it->first)) {
                    எனில் (AstEnumItem* மாறிலி aitemp = VN_CAST(itemp, EnumItem)) {
                        AstEnumItemRef* மாறிலி newp = புதிய AstEnumItemRef{
                            aitemp->fileline(), aitemp, it->second->classOrPackagep()};
                        UINFO(8, indent()
                                     << "Class import noderef '" << it->first << "' " << newp);
                        nodep->addMembersp(newp);
                        memberMap.செருகு(nodep, newp);
                    }
                }
            }
        }
        m_ds.m_dotSymp = VL_RESTORER_PREV(m_curSymp);
    }

    // Collect the ClassOrPackageRefs in a chained scope operand in source order.
    // The grammar builds a scope operand only from packageClassScopeItem, which
    // always yields a ClassOrPackageRef, nested in Dots, so nothing else appears.
    நிலையான வெறுமை collectScopeRefs(Ast_முனை* nodep, std::vector<AstClassOrPackageRef*>& refps) {
        எனில் (AstClassOrPackageRef* மாறிலி refp = VN_CAST(nodep, ClassOrPackageRef)) {
            refps.தள்ளு_back(refp);
            திரும்பு;
        }
        AstDot* மாறிலி dotp = VN_CAST(nodep, Dot);
        UASSERT_OBJ(dotp, nodep, "Non-scope node in package/class scope operand");
        collectScopeRefs(dotp->lhsp(), refps);
        collectScopeRefs(dotp->rhsp(), refps);
    }

    // Resolve a chained class-scope operand (`pkg::outer::inner` in
    // `pkg::outer::inner::t`) and reduce it to its innermost ClassOrPackageRef.
    // Returns false, having reported, if a segment could not be resolved.
    தருக்க reduceScopeDot(AstRefDType* nodep, AstDot* scopeDotp) {
        std::vector<AstClassOrPackageRef*> refps;
        collectScopeRefs(scopeDotp, refps);

        VSymEnt* scopeSymp = m_ds.m_dotSymp;
        க்கு (size_t i = 0; i < refps.size(); ++i) {
            AstClassOrPackageRef* மாறிலி refp = refps[i];
            எனில் (!refp->classOrPackageSkipp() && !refp->classOrPackageNodep()
                && !m_statep->resolveClassOrPackage(scopeSymp, refp, i == 0, பொய்,
                                                    "class/package reference")) {
                திரும்பு பொய்;  // Error already reported
            }
            எனில் (i + 1 == refps.size()) நிறுத்து;
            // A middle segment that resolved to a node with no reachable module
            // (e.g. a typedef alias of a not-yet-specialized parameterized class)
            // gives nothing to look the next segment up in.
            Ast_முனை_Module* மாறிலி modp = refp->classOrPackageSkipp();
            எனில் (!modp) {
                refp->v3warn(E_UNSUPPORTED, "Unsupported: Multiple '::' package/class reference");
                திரும்பு பொய்;
            }
            scopeSymp = m_statep->get_முனை_Sym(modp);
        }
        AstClassOrPackageRef* மாறிலி innerp = refps.back();
        innerp->unlinkFrBack();
        VL_DO_DANGLING(தள்ளு_Deletep(scopeDotp->unlinkFrBack()), scopeDotp);
        nodep->classOrPackageOpp(innerp);
        திரும்பு உண்மை;
    }

    வெறுமை visit(AstRefDType* nodep) override {

        எனில் (auto* மாறிலி typeOfp = nodep->typeofp()) {
            iterate(typeOfp);
            // After iteration, typeofp() may have been replaced with a resolved type
            // (e.g., DOT expression like if0.rq_t resolved to REFDTYPE)
            // If it's now a NodeDType, replace this RefDType with it
            எனில் (Ast_முனை_DType* மாறிலி resolvedDTypep = VN_CAST(nodep->typeofp(), முனை_DType)) {
                UINFO(9,
                      indent() << "iface capture typeofp resolved to dtype, replacing RefDType\n");
                resolvedDTypep->unlinkFrBack();
                nodep->replaceWith(resolvedDTypep);
                VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
                // Originally (PR #6637) this block re-captured a resolved
                // RefDType into the iface-capture ledger when user2p held a
                // Cell, to ensure re-resolution during the paramed pass.
                // In practice, the typeofp-resolved RefDType never carries a
                // Cell in user2p: the capture happens earlier when the
                // typedef is first linked.
                எனில் (V3LinkDotIfaceCapture::enabled()) {
                    எனில் (AstRefDType* மாறிலி resolvedRefp = VN_CAST(resolvedDTypep, RefDType)) {
                        எனில் (VL_UNCOVERABLE(
                                VN_IS(resolvedRefp->user2p(), Cell))) {  // LCOV_EXCL_LINE
                            resolvedRefp->v3fatalSrc(  // LCOV_EXCL_LINE
                                "typeofp resolved RefDType has Cell in user2p;"
                                " expected to be captured already");
                        }
                    }
                }
            }
            திரும்பு;
        }

        // Resolve its reference
        எனில் (m_statep->forParamed() && nodep->user3()) {
            எனில் (V3LinkDotIfaceCapture::enabled() && nodep->user2p()) {
                UINFO(9, indent() << "iface capture clear user3 for captured typedef name="
                                  << nodep->name() << " cell=" << nodep->user2p());
            }
            nodep->user3(பொய்);
        }
        எனில் (nodep->user3SetOnce()) {
            எனில் (V3LinkDotIfaceCapture::enabled() && nodep->user2p()) {
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
            எனில் (VL_UNCOVERABLE(m_statep->forParamed() && nodep->classOrPackageOpp())) {
                nodep->v3fatalSrc(  // LCOV_EXCL_LINE
                    "RefDType user3 revisit with classOrPackageOpp in paramed pass");
            }
            திரும்பு;
        }
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        எனில் (nodep->classOrPackageOpp()) {
            // Resolve a chained scope (`pkg::cls::t`, `pkg::cls#(P)::t`) from outside in.
            // On failure reduceScopeDot has reported, so stop before using the operand.
            எனில் (AstDot* மாறிலி scopeDotp = VN_CAST(nodep->classOrPackageOpp(), Dot)) {
                எனில் (!reduceScopeDot(nodep, scopeDotp)) திரும்பு;
            }
            // Re-read: reduceScopeDot may have replaced the operand
            Ast_முனை* மாறிலி cpackagep = nodep->classOrPackageOpp();
            UASSERT_OBJ(!VN_IS(cpackagep, Dot), cpackagep,
                        "Package/class scope operand should be reduced by now");
            எனில் (AstClassOrPackageRef* மாறிலி cpackagerefp = VN_CAST(cpackagep, ClassOrPackageRef)) {
                iterate(cpackagerefp);
                மாறிலி AstClass* மாறிலி clsp = VN_CAST(cpackagerefp->classOrPackageNodep(), Class);
                எனில் (clsp && clsp->hasGParam()) {
                    // Unable to link before the instantiation of parameter classes.
                    // The class reference node still has to be visited now to later link
                    // parameters.
                    iterate(cpackagep);
                    திரும்பு;
                }
                // Defer non-typedef references through typedef aliases of parameterized classes.
                எனில் (m_statep->forPrimary() && !VN_IS(nodep->backp(), Typedef)
                    && isParamedClassRef(cpackagerefp)) {
                    iterate(cpackagep);
                    திரும்பு;
                }

                மாறிலி தருக்க doDefaultTypedef = !(m_resolvingTypedef && m_statep->forPrimary());
                எனில் (!cpackagerefp->classOrPackageSkipp(doDefaultTypedef)
                    && !cpackagerefp->classOrPackageNodep()) {
                    VSymEnt* மாறிலி foundp = m_statep->resolveClassOrPackage(
                        m_ds.m_dotSymp, cpackagerefp, உண்மை, பொய், "class/package reference");
                    எனில் (!foundp) திரும்பு;
                }
                nodep->classOrPackagep(cpackagerefp->classOrPackageSkipp(doDefaultTypedef));
                எனில் (!VN_IS(nodep->classOrPackagep(), Class)
                    && !VN_IS(nodep->classOrPackagep(), Package)) {
                    எனில் (m_statep->forPrimary()) {
                        // It may be a type that comes from parameter class that is not
                        // instantioned yet
                        iterate(cpackagep);
                        திரும்பு;
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
            VL_DO_DANGLING(தள்ளு_Deletep(cpackagep->unlinkFrBack()), cpackagep);
        }

        எனில் (m_ds.m_dotp && (m_ds.m_dotPos == DP_PACKAGE || m_ds.m_dotPos == DP_SCOPE)) {
            UASSERT_OBJ(VN_IS(m_ds.m_dotp->lhsp(), ClassOrPackageRef), m_ds.m_dotp->lhsp(),
                        "Bad package link");
            auto* மாறிலி cpackagerefp = VN_CAST(m_ds.m_dotp->lhsp(), ClassOrPackageRef);
            UASSERT_OBJ(cpackagerefp->classOrPackageSkipp(), m_ds.m_dotp->lhsp(),
                        "Bad package link");
            nodep->classOrPackagep(cpackagerefp->classOrPackageSkipp());
            m_ds.m_dotPos = DP_SCOPE;
        } இல்லையெனில் {
            // Allow REFDTYPE under DOT when referencing interface typedefs
            // This is needed for patterns like: typedef iface.a_t a_t;
            // The dependency graph handles resolution when IfaceCapture is disabled
            மாறிலி தருக்க ifaceFinalSegmentAllowed
                = (m_ds.m_dotPos == DP_FINAL) && m_ds.m_dotSymp
                  && VN_IS(m_ds.m_dotSymp->nodep(), Cell)
                  && VN_CAST(m_ds.m_dotSymp->nodep(), Cell)->modp()
                  && VN_IS(VN_CAST(m_ds.m_dotSymp->nodep(), Cell)->modp(), Iface);
            எனில் (!ifaceFinalSegmentAllowed) {
                checkNoDot(nodep);
            } இல்லையெனில் {
                UINFO(9, indent() << "allow REFDTYPE under DOT for iface typedef name="
                                  << nodep->prettyNameQ() << " dotSym=" << m_ds.m_dotSymp);
                // Clear the dot state so we don't propagate errors
                m_ds.m_dotPos = DP_SCOPE;
            }
        }
        எனில் (!nodep->typedefp() && !nodep->subDTypep()) {
            மாறிலி VSymEnt* foundp;
            எனில் (nodep->classOrPackagep()) {
                foundp = m_statep->get_முனை_Sym(nodep->classOrPackagep())->findIdFlat(nodep->name());
            } இல்லையெனில் எனில் (m_ds.m_dotPos == DP_FIRST || m_ds.m_dotPos == DP_NONE) {
                foundp = findIdFallbackSkipMemberDType(m_curSymp, nodep->name());
            } இல்லையெனில் {
                // Defensive: dotPos should be DP_FIRST/DP_NONE or classOrPackagep set.
                v3fatalSrc("Unexpected dotPos="
                           << static_cast<முழு>(m_ds.m_dotPos)  // LCOV_EXCL_LINE
                           << " in RefDType lookup for "
                           << nodep->prettyNameQ());  // LCOV_EXCL_LINE
                foundp = nullptr;  // LCOV_EXCL_LINE
            }
            {
                எனில் (AstTypedef* மாறிலி defp
                    = foundp ? VN_CAST(foundp->nodep(), Typedef) : nullptr) {
                    // Don't check if typedef is to a <type T>::<reference> as might not be
                    // resolved yet
                    எனில் (!nodep->classOrPackagep() && !defp->isUnderClass())
                        checkDeclOrder(nodep, defp);
                    nodep->typedefp(defp);
                    nodep->classOrPackagep(foundp->classOrPackagep());

                } இல்லையெனில் எனில் (AstParamTypeDType* மாறிலி defp
                           = foundp ? VN_CAST(foundp->nodep(), ParamTypeDType) : nullptr) {
                    // A PARAMTYPEDTYPE's child REFDTYPE referencing itself is the normal
                    // AST structure for type parameters (e.g., "type T = base" has a child
                    // REFDTYPE for the default type). Only error on true recursion where
                    // a TYPEDEF contains a reference back to itself.
                    மாறிலி தருக்க isParamTypeChild = (defp == nodep->backp());
                    மாறிலி தருக்க isTypedefRecursion
                        = isParamTypeChild && VN_IS(defp->backp(), Typedef);
                    எனில் (isTypedefRecursion) {
                        nodep->v3error("Reference to '"
                                       << m_ds.m_dotText << (m_ds.m_dotText == "" ? "" : ".")
                                       << nodep->prettyName() << "'"
                                       << " type would form a recursive definition");
                        nodep->refDTypep(nodep->findVoidDType());  // Try to reduce later errors
                    } இல்லையெனில் {
                        nodep->refDTypep(defp);
                        nodep->classOrPackagep(foundp->classOrPackagep());
                        captureIfaceParamType(nodep, defp);
                    }
                } இல்லையெனில் எனில் (AstClass* மாறிலி defp
                           = foundp ? VN_CAST(foundp->nodep(), Class) : nullptr) {
                    // Don't check if typedef is to a <type T>::<reference> as might not be
                    // resolved yet
                    எனில் (!nodep->classOrPackagep()) checkDeclOrder(nodep, defp);
                    AstPin* மாறிலி paramsp = nodep->paramsp();
                    எனில் (paramsp) paramsp->unlinkFrBackWithNext();
                    AstClassRefDType* மாறிலி newp
                        = புதிய AstClassRefDType{nodep->fileline(), defp, paramsp};
                    newp->classOrPackagep(foundp->classOrPackagep());
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
                    திரும்பு;
                } இல்லையெனில் எனில் (m_insideClassExtParam) {
                    திரும்பு;
                } இல்லையெனில் {
                    எனில் (foundp) {
                        UINFO(1, "Found sym node: " << foundp->nodep());
                        nodep->v3error("Expecting a data type: "
                                       << nodep->prettyNameQ()
                                       << ", found: " << foundp->nodep()->prettyTypeName());
                    } இல்லையெனில் {
                        nodep->v3error("Can't find typedef/interface: " << nodep->prettyNameQ());
                    }
                }
            }
        }
        iterateChildren(nodep);
    }
    வெறுமை visit(AstRequireDType* nodep) override {
        // Handle type validation for localparam type assignments
        // Iterate child to resolve any ParseRef nodes
        iterateChildren(nodep);
        // Only emit error if the child is not a type.
        // Do NOT unwrap valid types here - leave that to V3Width.
        // Unwrapping here breaks type parameter resolution during cloning.
        // Allow Dot through to defer the handling until the dot expression is resolved.
        எனில் (nodep->lhsp() && !VN_IS(nodep->lhsp(), முனை_DType) && !VN_IS(nodep->lhsp(), Dot)) {
            // Not a type - emit error
            எனில் (VN_IS(nodep->lhsp(), Const)) {
                nodep->lhsp()->v3error("Expecting a data type, not a constant");
            } இல்லையெனில் {
                nodep->lhsp()->v3error("Expecting a data type, not "
                                       << nodep->lhsp()->typeName() << ": '"
                                       << nodep->lhsp()->prettyName() << "'");
            }
            // Replace with void to keep AST valid and allow clean exit
            nodep->replaceWith(புதிய AstVoidDType{nodep->fileline()});
            VL_DO_DANGLING(தள்ளு_Deletep(nodep), nodep);
        }
    }
    வெறுமை visit(AstDpiExport* nodep) override {
        // AstDpiExport: Make sure the function referenced exists, then dump it
        LINKDOT_VISIT_START();
        iterateChildren(nodep);
        checkNoDot(nodep);
        VSymEnt* மாறிலி foundp = m_curSymp->findIdFallback(nodep->name());
        Ast_முனை_FTask* மாறிலி taskp = foundp ? VN_AS(foundp->nodep(), முனை_FTask) : nullptr;
        எனில் (!taskp) {
            nodep->v3error(
                "Can't find definition of exported task/function: " << nodep->prettyNameQ());
        } இல்லையெனில் எனில் (taskp->dpiExport()) {
            nodep->v3error("Function was already DPI Exported, duplicate not allowed: "
                           << nodep->prettyNameQ());
        } இல்லையெனில் {
            taskp->dpiExport(உண்மை);
            எனில் (nodep->cname() != "") taskp->cname(nodep->cname());
        }
        VL_DO_DANGLING(தள்ளு_Deletep(nodep->unlinkFrBack()), nodep);
    }
    வெறுமை visit(AstDisable* nodep) override {
        LINKDOT_VISIT_START();
        checkNoDot(nodep);
        தருக்க hasPartSelect = பொய்;
        மாறிலி string targetPath
            = nodep->targetRefp() ? extractDottedPath(nodep->targetRefp(), hasPartSelect) : "";
        VL_RESTORER_COPY(m_ds);
        m_ds.init(m_curSymp);
        m_ds.m_dotPos = DP_FIRST;
        m_ds.m_disablep = nodep;
        iterateChildren(nodep);
        எனில் (nodep->targetRefp()) {
            எனில் (AstTaskRef* மாறிலி taskRefp = VN_CAST(nodep->targetRefp(), TaskRef)) {
                nodep->targetp(taskRefp->taskp());
            } இல்லையெனில் எனில் (!VN_IS(nodep->targetRefp(), ParseRef)) {
                // If it is a ParseRef, either it couldn't be linked or it is linked to a block
                nodep->v3warn(E_UNSUPPORTED, "Node of type "
                                                 << nodep->targetRefp()->prettyTypeName()
                                                 << " referenced by disable");
                VL_DO_DANGLING(தள்ளு_Deletep(nodep->unlinkFrBack()), nodep);
                திரும்பு;
            }
            எனில் (nodep->targetp()) {
                nodep->targetRefp()->unlinkFrBack()->delete_மரம்();
                எனில் (!hasPartSelect) {
                    மாறிலி string instancePath = targetInstancePath(nodep->targetp(), targetPath);
                    எனில் (!instancePath.empty()) {
                        // Keep only the instance prefix so V3LinkJump can reference the selected
                        // instance's process queue without reparsing the disable target.
                        nodep->targetRefp(
                            புதிய AstVarXRef{nodep->fileline(), "", instancePath, VAccess::READ});
                    }
                }
            }
        }
    }
    வெறுமை visitPackageImportOrExport(Ast_முனை* nodep) {
        // No longer needed
        LINKDOT_VISIT_START();
        checkNoDot(nodep);
        எனில் (m_statep->forParamed()) VL_DO_DANGLING(தள்ளு_Deletep(nodep->unlinkFrBack()), nodep);
    }
    வெறுமை visit(AstPackageImport* nodep) override { visitPackageImportOrExport(nodep); }
    வெறுமை visit(AstPackageExport* nodep) override { visitPackageImportOrExport(nodep); }
    வெறுமை visit(AstPackageExportStarStar* nodep) override { visitPackageImportOrExport(nodep); }
    வெறுமை visit(AstCellRef* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        iterateChildren(nodep);
    }
    வெறுமை visit(AstCellArrayRef* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        // Expression already iterated
    }
    வெறுமை visit(AstUnlinkedRef* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        // No need to iterate, if we have a UnlinkedVarXRef, we're already done
    }
    வெறுமை visit(Ast_Stmt_கோவை* nodep) override {
        LINKDOT_VISIT_START();
        checkNoDot(nodep);
        // Check if nodep represents a super.new call;
        எனில் (AstNew* மாறிலி newExprp = VN_CAST(nodep->exprp(), New)) {
            // in this case it was already linked, so it doesn't have a super reference
            m_explicitSuperNewp = newExprp;
        } இல்லையெனில் எனில் (மாறிலி AstDot* மாறிலி dotp = VN_CAST(nodep->exprp(), Dot)) {
            எனில் (dotp->lhsp()->name() == "super") {
                எனில் (AstNew* மாறிலி newExprp = VN_CAST(dotp->rhsp(), New)) {
                    m_explicitSuperNewp = newExprp;
                }
            }
        }
        iterateChildren(nodep);
    }

    வெறுமை visit(AstIfaceRefDType* nodep) override {
        எனில் (nodep->user3SetOnce()) திரும்பு;
        LINKDOT_VISIT_START();
        எனில் (nodep->paramsp()) {
            // If there is no parameters, there is no need to visit this node.
            AstIface* மாறிலி ifacep = nodep->ifacep();
            UASSERT_OBJ(ifacep, nodep, "Port parameters of AstIfaceRefDType without ifacep()");
            எனில் (ifacep->dead()) திரும்பு;
            checkNoDot(nodep);
            VL_RESTORER_CLEAR(m_usedPins);
            VL_RESTORER_CLEAR(m_usedDefParamPins);
            VL_RESTORER(m_pinSymp);
            m_pinSymp = m_statep->get_முனை_Sym(ifacep);
            iterateAndNextNull(nodep->paramsp());
        }
    }

    வெறுமை visit(AstAttrOf* nodep) override { iterateChildren(nodep); }
    வெறுமை visit(AstCastParse* nodep) override {
        எனில் (nodep->user3SetOnce()) திரும்பு;
        LINKDOT_VISIT_START();
        // A cast may start a dotted expression: HERE type'(expr) . member_or_method
        மாறிலி தருக்க dotLhs = m_ds.m_dotPos == DP_FIRST;
        எனில் (!dotLhs) checkNoDot(nodep);
        {
            VL_RESTORER_COPY(m_ds);
            m_ds.init(m_curSymp);
            iterateChildren(nodep);
        }
        எனில் (dotLhs) m_ds.m_dotPos = DP_MEMBER;
    }

    வெறுமை visit(AstAssignW* nodep) override {
        LINKDOT_VISIT_START();
        checkNoDot(nodep);
        VL_RESTORER(m_replaceWithAlias);
        எனில் (nodep->user2()) m_replaceWithAlias = பொய்;
        iterateChildren(nodep);
    }

    வெறுமை visit(AstTypedef* nodep) override {
        VL_RESTORER(m_resolvingTypedef)
        m_resolvingTypedef = உண்மை;
        iterateChildren(nodep);
    }

    வெறுமை visit(Ast_முனை* nodep) override {
        VL_RESTORER(m_inPackedArray);
        எனில் (VN_IS(nodep, PackArrayDType)) {
            m_inPackedArray = உண்மை;
        } இல்லையெனில் எனில் (!m_inPackedArray) {
            LINKDOT_VISIT_START();
            checkNoDot(nodep);
        }
        iterateChildren(nodep);
    }

பொது:
    // CONSTRUCTORS
    LinkDotResolveVisitor(AstNetlist* rootp, LinkDotState* statep)
        : m_statep{statep} {
        UINFO(4, __FUNCTION__ << ": ");

        // Note: no need to clear user3 on ledger entries here.
        // VNUser3InUse (m_inuser3) already logically clears user3 on all
        // nodes via the generation counter.  The ledger may also hold stale
        // refp pointers to deleted AST nodes, so iterating it is unsafe.

        iterate(rootp);
        மாறிலி std::map<std::string, Ast_முனை_Module*> modulesToRevisit
            = std::move(m_modulesToRevisit);
        m_lastDeferredp = nullptr;
        க்கு (auto& p : modulesToRevisit) {
            Ast_முனை_Module* மாறிலி modp = p.second;
            modp->foreach([](Ast_முனை* மாறிலி nodep) { nodep->user3(பொய்); });
            iterate(modp);
        }
    }
    ~LinkDotResolveVisitor() override = இயல்பு;
};

//######################################################################
// Link class functions

வெறுமை V3LinkDot::dumpSubstep(மாறிலி string& name) {
    எனில் (dump_மரம்_Either_Level() >= 9) {
        V3Global::dump_Check_Global_மரம்(name);
    } இல்லையெனில் எனில் (debug() >= 5) {  // on high dbg level, dump even if not explicitly told to
        v3Global.rootp()->dump_மரம்_File(v3Global.debugFilename(name + ".tree"));
    }
}

வெறுமை V3LinkDot::linkDotGuts(AstNetlist* rootp, VLinkDotStep step) {
    VIsCached::clear_Cache_மரம்();  // Avoid using any stale isPure
    dumpSubstep("prelinkdot");
    LinkDotState state{rootp, step};

    { LinkDotFindVisitor{rootp, &state}; }
    dumpSubstep("prelinkdot-find");

    { LinkDotFindIfaceVisitor{rootp, &state}; }
    dumpSubstep("prelinkdot-findiface");

    எனில் (step == LDS_SCOPED) {
        // Well after the initial link when we're ready to operate on the flat design,
        // process AstScope's.  This needs to be separate pass after whole hierarchy graph created.
        { LinkDotScopeVisitor{rootp, &state}; }
        v3Global.assertScoped(உண்மை);
        dumpSubstep("prelinkdot-scoped");
    } இல்லையெனில் {
        // Initial link stage, resolve parameters and interfaces
        { LinkDotParamVisitor{rootp, &state}; }
        dumpSubstep("prelinkdot-param");
    }
    state.dumpSelf("prelinkdot");
    state.computeIfaceModSyms();
    state.computeIfaceVarSyms();
    state.computeScopeAliases();
    state.dumpSelf("linkdot-preresolve");
    மாறிலி LinkDotResolveVisitor visitor{rootp, &state};
    state.dumpSelf("linkdot-done");
}

வெறுமை V3LinkDot::linkDotPrimary(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    linkDotGuts(nodep, LDS_PRIMARY);
    V3Global::dump_Check_Global_மரம்("linkdot", 0, dump_மரம்_Either_Level() >= 6);
}

வெறுமை V3LinkDot::linkDotParamed(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    linkDotGuts(nodep, LDS_PARAMED);
    V3Global::dump_Check_Global_மரம்("linkdotparam", 0, dump_மரம்_Either_Level() >= 3);
}

வெறுமை V3LinkDot::linkDotScope(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    linkDotGuts(nodep, LDS_SCOPED);
    V3Global::dump_Check_Global_மரம்("linkdot", 0, dump_மரம்_Either_Level() >= 3);
}
