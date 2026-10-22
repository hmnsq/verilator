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
// TOP
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

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3LinkDot.h"

#ضمن "V3Global.h"
#ضمن "V3Graph.h"
#ضمن "V3LinkDotIfaceCapture.h"
#ضمن "V3MemberMap.h"
#ضمن "V3Parse.h"
#ضمن "V3Randomize.h"
#ضمن "V3Stats.h"
#ضمن "V3String.h"
#ضمن "V3SymTable.h"

#ضمن <algorithm>
#ضمن <cstdlib>
#ضمن <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

ساكن string extractDottedPath(AstNode* nodep, منطقي& hasPartSelect) {
    𐤀𐤌 (AstParseRef* ثابت refp = VN_CAST(nodep, ParseRef)) {
        أرجع refp->name();
    } وإلا 𐤀𐤌 (AstVarRef* ثابت refp = VN_CAST(nodep, VarRef)) {
        أرجع refp->name();
    } وإلا 𐤀𐤌 (AstDot* ثابت dotp = VN_CAST(nodep, Dot)) {
        ثابت string lhs = extractDottedPath(dotp->lhsp(), hasPartSelect);
        ثابت string rhs = extractDottedPath(dotp->rhsp(), hasPartSelect);
        أرجع VString::dot(lhs, ".", rhs);
    } وإلا 𐤀𐤌 (VN_IS(nodep, SelBit) || VN_IS(nodep, SelExtract) || VN_IS(nodep, SelPlus)
               || VN_IS(nodep, SelMinus)) {
        hasPartSelect = true;
        أرجع "";
    }
    أرجع "";
}
ساكن string lexicalDisablePath(AstNode* ثابت targetp) {
    std::vector<string> names;
    لكل (AstNode* curp = targetp; curp; curp = curp->aboveLoopp()) {
        𐤀𐤌 (AstNodeBlock* ثابت blockp = VN_CAST(curp, NodeBlock)) {
            𐤀𐤌 (blockp->name() != "") names.push_back(blockp->name());
        } وإلا 𐤀𐤌 (AstNodeFTask* ثابت ftaskp = VN_CAST(curp, NodeFTask)) {
            names.push_back(ftaskp->name());
        } وإلا 𐤀𐤌 (VN_IS(curp, NodeModule)) {
            اكسر;
        }
    }
    string path;
    لكل (auto it = names.crbegin(); it != names.crend(); ++it) {
        path = VString::dot(path, ".", *it);
    }
    أرجع path;
}
ساكن string targetInstancePath(AstNode* ثابت targetp, ثابت string& targetPath) {
    ثابت string lexicalPath = lexicalDisablePath(targetp);
    𐤀𐤌 (lexicalPath == "" || targetPath == lexicalPath) أرجع "";
    ثابت string suffix = "." + lexicalPath;
    𐤀𐤌 (!VString::endsWith(targetPath, suffix)) أرجع "";
    أرجع targetPath.substr(0, targetPath.size() - suffix.size());
}

// ######################################################################
//  Matcher classes (for suggestion matching)

صنف LinkNodeMatcherClassOrPackage final : عام VNodeMatcher {
عام:
    منطقي nodeMatch(ثابت AstNode* nodep) ثابت override {
        أرجع VN_IS(nodep, Class) || VN_IS(nodep, Package);
    }
};
صنف LinkNodeMatcherFTask final : عام VNodeMatcher {
عام:
    منطقي nodeMatch(ثابت AstNode* nodep) ثابت override { أرجع VN_IS(nodep, NodeFTask); }
};
صنف LinkNodeMatcherModport final : عام VNodeMatcher {
عام:
    منطقي nodeMatch(ثابت AstNode* nodep) ثابت override { أرجع VN_IS(nodep, Modport); }
};
صنف LinkNodeMatcherProd final : عام VNodeMatcher {
عام:
    منطقي nodeMatch(ثابت AstNode* nodep) ثابت override { أرجع VN_IS(nodep, RSProd); }
};
صنف LinkNodeMatcherVar final : عام VNodeMatcher {
عام:
    منطقي nodeMatch(ثابت AstNode* nodep) ثابت override {
        أرجع VN_IS(nodep, Var) || VN_IS(nodep, LambdaArgRef);
    }
};
صنف LinkNodeMatcherVarIO final : عام VNodeMatcher {
عام:
    منطقي nodeMatch(ثابت AstNode* nodep) ثابت override {
        ثابت AstVar* ثابت varp = VN_CAST(nodep, Var);
        𐤀𐤌 (!varp) أرجع خطأ;
        أرجع varp->isIO();
    }
};
صنف LinkNodeMatcherVarParam final : عام VNodeMatcher {
عام:
    منطقي nodeMatch(ثابت AstNode* nodep) ثابت override {
        ثابت AstVar* ثابت varp = VN_CAST(nodep, Var);
        𐤀𐤌 (!varp) أرجع خطأ;
        أرجع varp->isParam();
    }
};

//######################################################################
// LinkDot state, as a visitor of each AstNode

صنف LinkDotState final {
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

عام:
    // ENUMS
    // In order of priority, compute first ... compute last
    تعداد SAMNum : uint8_t { SAMN_MODPORT, SAMN_IFTOP, SAMN__MAX };  // Values for m_scopeAliasMap

خاص:
    // TYPES
    using ScopeAliasMap = std::unordered_map<VSymEnt*, VSymEnt*>;
    using IfaceModSyms = std::vector<std::pair<AstIface*, VSymEnt*>>;

    ساكن LinkDotState* s_errorThisp;  // Last self, for error reporting only

    // MEMBERS
    VSymGraph m_syms;  // Symbol table by hierarchy
    VSymGraph m_mods;  // Symbol table of all module names
    VSymEnt* m_dunitEntp = nullptr;  // $unit entry
    std::multimap<std::string, VSymEnt*>
        m_nameScopeSymMap;  // Map of scope referenced by non-pretty textual name
    std::set<std::pair<AstNodeModule*, std::string>>
        m_implicitNameSet;  // For [module][signalname] if we can implicitly create it
    std::array<ScopeAliasMap, SAMN__MAX> m_scopeAliasMap;  // Map of <lhs,rhs> aliases
    std::vector<VSymEnt*> m_ifaceVarSyms;  // List of AstIfaceRefDType's to be imported
    IfaceModSyms m_ifaceModSyms;  // List of AstIface+Symbols to be processed
    ثابت VLinkDotStep m_step;  // Operational step

عام:
    // METHODS

    فراغ dumpSelf(ثابت string& nameComment, منطقي force = خطأ) {
        𐤀𐤌 (debug() >= 6 || dumpLevel() >= 6 || force) {
            ثابت string filename = v3Global.debugFilename(nameComment) + ".txt";
            UINFO(4, "Dumping " << filename);
            ثابت std::unique_ptr<std::ofstream> logp{V3File::new_ofstream(filename)};
            𐤀𐤌 (logp->fail()) v3fatal("Can't write file: " << filename);
            std::ostream& os = *logp;
            // TODO the symbol table has node pointers which may be
            // dangling, as we call deleteTree in these visitors without
            // pushing for later deletion (or deleting from the symbol table)
            // So, only completely safe to call this when under --debug or --debug-leak.
            m_syms.dumpSelf(os);
            منطقي first = true;
            لكل (صحيح samn = 0; samn < SAMN__MAX; ++samn) {
                𐤀𐤌 (!m_scopeAliasMap[samn].empty()) {
                    𐤀𐤌 (first) os << "\nScopeAliasMap:\n";
                    first = خطأ;
                    لكل (ScopeAliasMap::iterator it = m_scopeAliasMap[samn].begin();
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
    ساكن فراغ preErrorDumpHandler() {
        𐤀𐤌 (s_errorThisp) s_errorThisp->preErrorDump();
    }
    فراغ preErrorDump() {
        ساكن منطقي s_diddump = خطأ;
        𐤀𐤌 (!s_diddump && dumpTreeLevel()) {
            s_diddump = true;
            dumpSelf("linkdot-preerr", true);
            v3Global.rootp()->dumpTreeFile(v3Global.debugFilename("linkdot-preerr.tree"));
        }
    }

    // CONSTRUCTORS
    LinkDotState(AstNetlist* rootp, VLinkDotStep step)
        : m_syms{rootp}
        , m_mods{rootp}
        , m_step{step} {
        UINFO(4, __FUNCTION__ << ": ");
        s_errorThisp = هذا;
        V3Error::errorExitCb(preErrorDumpHandler);  // If get error, dump self
        readModNames();
    }
    ~LinkDotState() {
        V3Error::errorExitCb(nullptr);
        s_errorThisp = nullptr;
    }

    // ACCESSORS
    VSymGraph* symsp() { أرجع &m_syms; }
    صحيح stepNumber() ثابت { أرجع static_cast<صحيح>(m_step); }
    منطقي forPrimary() ثابت { أرجع m_step == LDS_PRIMARY; }
    منطقي forParamed() ثابت { أرجع m_step == LDS_PARAMED; }
    منطقي forScopeCreation() ثابت { أرجع m_step == LDS_SCOPED; }

    // METHODS
    ساكن string nodeTextType(AstNode* nodep) {
        𐤀𐤌 (ثابت AstVar* ثابت varp = VN_CAST(nodep, Var)) {
            𐤀𐤌 (varp->isIO() || varp->isIfaceRef()) {
                أرجع "port";
            } وإلا 𐤀𐤌 (varp->isGParam()) {
                أرجع "parameter";
            } وإلا 𐤀𐤌 (varp->varType() == VVarType::LPARAM) {
                أرجع "local parameter";
            } وإلا {
                أرجع "variable";
            }
        } وإلا 𐤀𐤌 (ثابت AstParamTypeDType* ثابت dtypep = VN_CAST(nodep, ParamTypeDType)) {
            𐤀𐤌 (dtypep->isGParam()) {
                أرجع "type parameter";
            } وإلا {
                أرجع "local type parameter";
            }
        } وإلا 𐤀𐤌 (VN_IS(nodep, Begin)) {
            أرجع "block";
        } وإلا 𐤀𐤌 (VN_IS(nodep, Cell)) {
            أرجع "instance";
        } وإلا 𐤀𐤌 (VN_IS(nodep, Constraint)) {
            أرجع "constraint";
        } وإلا 𐤀𐤌 (VN_IS(nodep, Func)) {
            أرجع "function";
        } وإلا 𐤀𐤌 (VN_IS(nodep, GenBlock)) {
            أرجع "generate block";
        } وإلا 𐤀𐤌 (VN_IS(nodep, Iface)) {
            أرجع "interface";
        } وإلا 𐤀𐤌 (VN_IS(nodep, RSProd)) {
            أرجع "randsequence production";
        } وإلا 𐤀𐤌 (VN_IS(nodep, Task)) {
            أرجع "task";
        } وإلا {
            أرجع nodep->prettyTypeName();
        }
    }

    AstNodeModule* findModuleSym(ثابت string& modName) {
        ثابت VSymEnt* ثابت foundp = m_mods.rootp()->findIdFallback(modName);
        أرجع foundp ? VN_AS(foundp->nodep(), NodeModule) : nullptr;
    }
    فراغ readModNames() {
        // Look at all modules, and store pointers to all module names
        لكل (AstNodeModule *nextp, *nodep = v3Global.rootp()->modulesp(); nodep; nodep = nextp) {
            nextp = VN_AS(nodep->nextp(), NodeModule);
            m_mods.rootp()->insert(nodep->name(), جديد VSymEnt{&m_mods, nodep});
        }
    }

    VSymEnt* rootEntp() ثابت { أرجع m_syms.rootp(); }
    VSymEnt* dunitEntp() ثابت { أرجع m_dunitEntp; }
    فراغ checkDuplicate(VSymEnt* lookupSymp, AstNode* nodep, ثابت string& name) {
        // Lookup the given name under current symbol table
        // Insert if not found
        // Report error if there's a duplicate
        //
        // Note we only check for conflicts at the same level; it's ok if one block hides another
        // We also wouldn't want to not insert it even though it's lower down

        ثابت VSymEnt* ثابت foundp = lookupSymp->findIdFlat(name);
        AstNode* ثابت fnodep = foundp ? foundp->nodep() : nullptr;
        𐤀𐤌 (!fnodep) {
            // Not found, will add in a moment.
            𐤀𐤌 (forPrimary() && VSymEnt::checkSimilarname(nodep)) {
                ثابت VSymEnt* ثابت alt = lookupSymp->findSimilarIdFlat(name);
                𐤀𐤌 (alt) {
                    nodep->v3warn(SIMILARNAME, "Declaration overlaps another with different case: "
                                                   << nodep->prettyNameQ() << '\n'
                                                   << nodep->warnContextPrimary() << '\n'
                                                   << alt->nodep()->warnOther()
                                                   << "... Location of original declaration\n"
                                                   << alt->nodep()->warnContextSecondary());
                }
            }
        } وإلا 𐤀𐤌 (nodep == fnodep) {  // Already inserted.
            // Good.
        } وإلا 𐤀𐤌 (foundp->imported()) {  // From package
            // We don't throw VARHIDDEN as if the import is later the symbol
            // table's import wouldn't warn
        } وإلا 𐤀𐤌 (forPrimary() && VN_IS(nodep, GenBlock)
                   && (VN_IS(fnodep, Begin) || VN_IS(fnodep, GenBlock))) {
            // Begin: ... blocks often replicate under genif/genfor, so
            // suppress duplicate checks.  See t_gen_forif.v for an example.
        } وإلا {
            UINFO(4, "name " << name);  // Not always same as nodep->name
            UINFO(4, "Var1 " << nodep);
            UINFO(4, "Var2 " << fnodep);
            // Quote the node's own name as for messages, if it is the symbol's
            ثابت string nameQ
                = fnodep->name() == name ? fnodep->prettyNameQ() : AstNode::prettyNameQ(name);
            𐤀𐤌 (nodep->type() == fnodep->type()) {
                nodep->v3error("Duplicate declaration of "
                               << nodeTextType(fnodep) << ": " << nameQ << '\n'
                               << nodep->warnContextPrimary() << '\n'
                               << fnodep->warnOther() << "... Location of original declaration\n"
                               << fnodep->warnContextSecondary());
                nodep->declTokenNumSetMin(0);  // Disable checking forward typedefs
                fnodep->declTokenNumSetMin(0);  // Disable checking forward typedefs
            } وإلا {
                nodep->v3error("Unsupported in C: "
                               << ucfirst(nodeTextType(nodep)) << " has the same name as "
                               << nodeTextType(fnodep) << ": " << nameQ << '\n'
                               << nodep->warnContextPrimary() << '\n'
                               << fnodep->warnOther() << "... Location of original declaration\n"
                               << fnodep->warnContextSecondary());
            }
        }
    }
    فراغ insertDUnit(AstNetlist* nodep) {
        // $unit on top scope
        VSymEnt* ثابت symp = جديد VSymEnt{&m_syms, nodep};
        UINFO(9, "      INSERTdunit se" << cvtToHex(symp));
        symp->parentp(rootEntp());  // Needed so backward search can find name of top module
        symp->fallbackp(nullptr);
        rootEntp()->insert("$unit ", symp);  // Space so can never name conflict with user code
        //
        UASSERT_OBJ(!m_dunitEntp, nodep, "Call insertDUnit only once");
        m_dunitEntp = symp;
    }
    VSymEnt* insertTopCell(AstNodeModule* nodep, ثابت string& scopename) {
        // Only called on the module at the very top of the hierarchy
        VSymEnt* ثابت symp = جديد VSymEnt{&m_syms, nodep};
        UINFO(9, "      INSERTtop se" << cvtToHex(symp) << "  " << scopename << " " << nodep);
        symp->parentp(rootEntp());  // Needed so backward search can find name of top module
        symp->fallbackp(dunitEntp());  // Needed so can find $unit stuff
        nodep->user1p(symp);
        checkDuplicate(rootEntp(), nodep, nodep->origName());
        rootEntp()->insert(nodep->origName(), symp);
        𐤀𐤌 (forScopeCreation()) m_nameScopeSymMap.emplace(scopename, symp);
        أرجع symp;
    }
    VSymEnt* insertTopIface(AstCell* nodep, ثابت string& scopename) {
        VSymEnt* ثابت symp = جديد VSymEnt{&m_syms, nodep};
        UINFO(9, "      INSERTtopiface se" << cvtToHex(symp) << "  " << scopename << " " << nodep);
        symp->parentp(rootEntp());  // Needed so backward search can find name of top module
        symp->fallbackp(dunitEntp());  // Needed so can find $unit stuff
        nodep->user1p(symp);
        𐤀𐤌 (nodep->modp()) nodep->modp()->user1p(symp);
        checkDuplicate(rootEntp(), nodep, nodep->origName());
        rootEntp()->insert(nodep->origName(), symp);
        𐤀𐤌 (forScopeCreation()) m_nameScopeSymMap.emplace(scopename, symp);
        أرجع symp;
    }
    VSymEnt* insertCell(VSymEnt* abovep, VSymEnt* modSymp, AstCell* nodep,
                        ثابت string& scopename) {
        UASSERT_OBJ(abovep, nodep, "Null symbol table inserting node");
        VSymEnt* ثابت symp = جديد VSymEnt{&m_syms, nodep};
        UINFO(9, "      INSERTcel se" << cvtToHex(symp) << "  " << scopename << " above=se"
                                      << cvtToHex(abovep) << " mods=se" << cvtToHex(modSymp)
                                      << " node=" << nodep);
        symp->parentp(abovep);
        symp->fallbackp(dunitEntp());  // Needed so can find $unit stuff
        nodep->user1p(symp);
        𐤀𐤌 (nodep->modp()) nodep->modp()->user1p(symp);
        checkDuplicate(abovep, nodep, nodep->origName());
        abovep->reinsert(nodep->origName(), symp);
        𐤀𐤌 (forScopeCreation() && abovep != modSymp && !modSymp->findIdFlat(nodep->name())) {
            // If it's foo_DOT_bar, we need to be able to find it under "foo_DOT_bar" too.
            // Duplicates are possible, as until resolve generates might
            // have 2 same cells under an if
            modSymp->reinsert(nodep->name(), symp);
        }
        𐤀𐤌 (forScopeCreation()) m_nameScopeSymMap.emplace(scopename, symp);
        أرجع symp;
    }
    فراغ insertMap(VSymEnt* symp, ثابت string& scopename) {
        𐤀𐤌 (forScopeCreation()) m_nameScopeSymMap.emplace(scopename, symp);
    }

    VSymEnt* insertInline(VSymEnt* abovep, VSymEnt* modSymp, AstCellInline* nodep,
                          ثابت string& basename) {
        // A fake point in the hierarchy, corresponding to a named block flattened by V3Begin.
        // Names are looked up in the module with the block's flattened prefix.
        UASSERT_OBJ(abovep, nodep, "Null symbol table inserting node");
        VSymEnt* ثابت symp = جديد VSymEnt{&m_syms, nodep};
        UINFO(9, "      INSERTinl se" << cvtToHex(symp) << "  " << basename << " above=se"
                                      << cvtToHex(abovep) << " mods=se" << cvtToHex(modSymp)
                                      << " node=" << nodep);
        symp->parentp(abovep);
        symp->fallbackp(modSymp);
        symp->symPrefix(nodep->name() + "__DOT__");
        nodep->user1p(symp);
        checkDuplicate(abovep, nodep, nodep->name());
        abovep->reinsert(basename, symp);
        𐤀𐤌 (abovep != modSymp && !modSymp->findIdFlat(nodep->name())) {
            // If it's foo_DOT_bar, we need to be able to find it under that too.
            modSymp->reinsert(nodep->name(), symp);
        }
        أرجع symp;
    }
    VSymEnt* insertBlock(VSymEnt* abovep, ثابت string& name, AstNode* nodep,
                         AstNodeModule* classOrPackagep) {
        // A fake point in the hierarchy, corresponding to a begin or function/task block
        // After we remove begins these will go away
        // Note we fallback to the symbol table of the parent, as we want to find variables there
        // However, cells walk the graph, so cells will appear under the begin/ftask itself
        UASSERT_OBJ(abovep, nodep, "Null symbol table inserting node");
        VSymEnt* ثابت symp = جديد VSymEnt{&m_syms, nodep};
        UINFO(9, "      INSERTblk se" << cvtToHex(symp) << "  above=se" << cvtToHex(abovep)
                                      << " pkg=" << cvtToHex(classOrPackagep)
                                      << "  node=" << nodep);
        symp->parentp(abovep);
        symp->classOrPackagep(classOrPackagep);
        symp->fallbackp(abovep);
        nodep->user1p(symp);
        𐤀𐤌 (name != "") checkDuplicate(abovep, nodep, name);
        // Duplicates are possible, as until resolve generates might have 2 same cells under an if
        abovep->reinsert(name, symp);
        أرجع symp;
    }
    VSymEnt* insertSym(VSymEnt* abovep, ثابت string& name, AstNode* nodep,
                       AstNodeModule* classOrPackagep) {
        UASSERT_OBJ(abovep, nodep, "Null symbol table inserting node");
        VSymEnt* ثابت symp = جديد VSymEnt{&m_syms, nodep};
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
        أرجع symp;
    }
    ساكن منطقي existsNodeSym(AstNode* nodep) { أرجع nodep->user1p() != nullptr; }
    ساكن VSymEnt* getNodeSym(AstNode* nodep) {
        // Don't use this in ResolveVisitor, as we need to pick up the proper
        // reference under each SCOPE
        VSymEnt* ثابت symp = nodep->user1u().toSymEnt();
        UASSERT_OBJ(symp, nodep, "Module/etc never assigned a symbol entry?");
        أرجع symp;
    }
    VSymEnt* getScopeSym(AstScope* nodep) {
        ثابت auto it = m_nameScopeSymMap.find(nodep->name());
        UASSERT_OBJ(it != m_nameScopeSymMap.end(), nodep,
                    "Scope never assigned a symbol entry '" << nodep->name() << "'");
        أرجع it->second;
    }
    فراغ implicitOkAdd(AstNodeModule* nodep, ثابت string& varname) {
        // Mark the given variable name as being allowed to be implicitly declared
        𐤀𐤌 (nodep) {
            ثابت auto it = m_implicitNameSet.find(std::make_pair(nodep, varname));
            𐤀𐤌 (it == m_implicitNameSet.end()) m_implicitNameSet.emplace(nodep, varname);
        }
    }
    منطقي implicitOk(AstNodeModule* nodep, ثابت string& varname) {
        أرجع nodep
               && (m_implicitNameSet.find(std::make_pair(nodep, varname))
                   != m_implicitNameSet.end());
    }

    // Track and later recurse interface modules
    فراغ insertIfaceModSym(AstIface* nodep, VSymEnt* symp) {
        m_ifaceModSyms.emplace_back(nodep, symp);
    }
    فراغ computeIfaceModSyms();

    // Track and later insert interface references
    فراغ insertIfaceVarSym(VSymEnt* symp) {  // Where sym is for a VAR of dtype IFACEREFDTYPE
        m_ifaceVarSyms.push_back(symp);
    }
    // Iface for a raw or arrayed iface; peels nested array layers for multi-dim arrays.
    ساكن AstIfaceRefDType* ifaceRefFromArray(AstNodeDType* nodep) {
        طالما (nodep) {
            𐤀𐤌 (AstIfaceRefDType* ثابت ifp = VN_CAST(nodep, IfaceRefDType)) أرجع ifp;
            𐤀𐤌 (ثابت AstBracketArrayDType* ثابت arrp = VN_CAST(nodep, BracketArrayDType)) {
                nodep = arrp->subDTypep();
            } وإلا 𐤀𐤌 (ثابت AstUnpackArrayDType* ثابت arrp = VN_CAST(nodep, UnpackArrayDType)) {
                nodep = arrp->subDTypep();
            } وإلا {
                أرجع nullptr;
            }
        }
        أرجع nullptr;
    }
    // Given a pin expression, resolve it to a live AstIface* (or nullptr).
    // Handles both simple VarRef and dotted VarXRef pin connections.
    AstIface* liveIfaceFromPinExpr(AstNode* exprp, VSymEnt* parentSymp) {
        AstVar* resolvedVarp = nullptr;
        𐤀𐤌 (AstVarRef* ثابت refp = VN_CAST(exprp, VarRef)) {
            resolvedVarp = refp->varp();
        } وإلا 𐤀𐤌 (AstVarXRef* ثابت xrefp = VN_CAST(exprp, VarXRef)) {
            VSymEnt* ثابت lookupSymp = parentSymp ? parentSymp->parentp() : nullptr;
            𐤀𐤌 (lookupSymp) {
                string baddot;
                VSymEnt* okSymp = nullptr;
                ثابت string dotpath = xrefp->dotted() + "." + xrefp->name();
                VSymEnt* ثابت dotSymp
                    = findDotted(xrefp->fileline(), lookupSymp, dotpath, baddot, okSymp, true);
                𐤀𐤌 (dotSymp) resolvedVarp = VN_CAST(dotSymp->nodep(), Var);
            }
        }
        AstIface* resultp = nullptr;
        𐤀𐤌 (resolvedVarp) {
            AstIfaceRefDType* ثابت irefp = ifaceRefFromArray(resolvedVarp->subDTypep());
            𐤀𐤌 (irefp && irefp->ifaceViaCellp() && !irefp->ifaceViaCellp()->dead()) {
                resultp = irefp->ifaceViaCellp();
            }
        }
        أرجع resultp;
    }

    // Attempt to repair a port's AstIfaceRefDType by tracing through the
    // parent cell's pin connection to find the correct live interface.
    // Returns true if repaired or already correct; false if unresolvable.
    منطقي repairIfaceRef(VSymEnt* varSymp, AstVar* varp, AstIfaceRefDType* ifacerefp, منطقي isDead) {
        // Walk up symbol-table parents to find the enclosing AstCell
        VSymEnt* parentSymp = varSymp->parentp();
        AstCell* parentCellp = nullptr;
        لكل (صحيح depth = 0; parentSymp && depth < 20; ++depth) {
            𐤀𐤌 (AstCell* ثابت cp = VN_CAST(parentSymp->nodep(), Cell)) {
                parentCellp = cp;
                اكسر;
            }
            parentSymp = parentSymp->parentp();
        }
        𐤀𐤌 (!parentCellp) أرجع خطأ;
        // Scan pins to find the one connected to this port variable
        لكل (AstPin* pinp = parentCellp->pinsp(); pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            𐤀𐤌 (pinp->modVarp() != varp && pinp->name() != varp->name()) استمر;
            AstNode* ثابت exprp = pinp->exprp();
            𐤀𐤌 (!exprp) أرجع خطأ;
            AstIface* ثابت newIfacep = liveIfaceFromPinExpr(exprp, parentSymp);
            𐤀𐤌 (newIfacep && newIfacep != ifacerefp->ifaceViaCellp()) {
                UINFO(4, "  REPAIR-IFACE-REF var=" << varp->prettyNameQ()
                                                   << " old=" << ifacerefp->ifacep()->prettyNameQ()
                                                   << " new=" << newIfacep->prettyNameQ());
                ifacerefp->ifacep(newIfacep);
                أرجع true;
            }
            𐤀𐤌 (newIfacep) أرجع !isDead;  // Same interface - OK if live
            أرجع خطأ;
        }
        أرجع خطأ;
    }

    فراغ computeIfaceVarSyms() {
        لكل (VSymEnt* varSymp : m_ifaceVarSyms) {
            AstVar* ثابت varp = varSymp ? VN_AS(varSymp->nodep(), Var) : nullptr;
            UINFO(9, "  insAllIface se" << cvtToHex(varSymp) << " " << varp);
            AstIfaceRefDType* ثابت ifacerefp = ifaceRefFromArray(varp->subDTypep());
            UASSERT_OBJ(ifacerefp, varp, "Non-ifacerefs on list!");
            𐤀𐤌 (!ifacerefp->cellp() && !ifacerefp->isVirtual() && (varp->pinNum() == 0)) {
                varp->v3error("Interface port declaration "
                              << varp->prettyNameQ() << " doesn't have corresponding port\n"
                              << varp->warnMore()
                                     + "... Perhaps intended an interface instantiation but "
                                       "are missing parenthesis (IEEE 1800-2023 25.3)?");
            }
            𐤀𐤌 (!ifacerefp->ifaceViaCellp()) {
                𐤀𐤌 (!ifacerefp->cellp()) {  // Probably a NotFoundModule, or a normal module if
                                            // made mistake
                    UINFO(1, "Associated cell " << AstNode::prettyNameQ(ifacerefp->cellName()));
                    ifacerefp->v3error("Cannot find file containing interface: "
                                       << AstNode::prettyNameQ(ifacerefp->ifaceName()));
                    استمر;
                } وإلا {
                    ifacerefp->v3fatalSrc("Unlinked interface");
                }
            } وإلا 𐤀𐤌 (varp->isIfaceRef() && !ifacerefp->cellp() && ifacerefp->ifaceViaCellp()) {
                // Port variable (cellp=null) pointing to an interface.
                // The interface may be dead (template replaced by clone) or
                // live-but-wrong (template still alive because some instances
                // use default params, but this port should point to a
                // specialized clone based on the pin connection).
                ثابت منطقي isDead = ifacerefp->ifaceViaCellp()->dead();
                𐤀𐤌 (isDead && forPrimary() && !varp->isIfaceParent()
                    && !v3Global.opt.topIfacesSupported()) {
                    varp->v3warn(E_UNSUPPORTED,
                                 "Unsupported: Interfaced port on top level module");
                }
                // Attempt repair: trace through parent cell's pin to find
                // the correct live interface for this port variable
                ثابت منطقي repaired = repairIfaceRef(varSymp, varp, ifacerefp, isDead);
                𐤀𐤌 (!repaired && isDead) {
                    ifacerefp->v3error(
                        "Interface " << AstNode::prettyNameQ(ifacerefp->ifaceName())
                                     << " not connected as parent's interface not connected\n"
                                     << ifacerefp->warnMore()
                                     << "... Perhaps caused by another error on the parent "
                                        "interface that needs resolving\n"
                                     << ifacerefp->warnMore()
                                     << "... Or, perhaps intended an interface instantiation but "
                                        "are missing parenthesis (IEEE 1800-2023 25.3)?");
                    استمر;
                }
            } وإلا 𐤀𐤌 (ifacerefp->ifaceViaCellp()->dead()
                       || !existsNodeSym(ifacerefp->ifaceViaCellp())) {
                ifacerefp->ifaceViaCellp()->v3fatalSrc(
                    "Interface referenced by virtual iface is dead. "
                    "Dead interfaces have required linking steps skipped");
            }
            VSymEnt* ثابت ifaceSymp = getNodeSym(ifacerefp->ifaceViaCellp());
            VSymEnt* ifOrPortSymp = ifaceSymp;
            // Link Modport names to the Modport Node under the Interface
            𐤀𐤌 (ifacerefp->isModport()) {
                VSymEnt* ثابت foundp = ifaceSymp->findIdFallback(ifacerefp->modportName());
                منطقي ok = خطأ;
                𐤀𐤌 (foundp) {
                    𐤀𐤌 (AstModport* ثابت modportp = VN_CAST(foundp->nodep(), Modport)) {
                        UINFO(4, "Link Modport: " << modportp);
                        ifacerefp->modportp(modportp);
                        ifOrPortSymp = foundp;
                        ok = true;
                    }
                }
                𐤀𐤌 (!ok) {
                    ثابت string suggest = suggestSymFallback(ifaceSymp, ifacerefp->modportName(),
                                                              LinkNodeMatcherModport{});
                    ifacerefp->modportFileline()->v3error(
                        "Modport not found under interface "
                        << ifacerefp->prettyNameQ(ifacerefp->ifaceName()) << ": "
                        << ifacerefp->prettyNameQ(ifacerefp->modportName()) << '\n'
                        << (suggest.empty() ? "" : ifacerefp->warnMore() + suggest));
                }
            }
            // Alias won't expand until interfaces and modport names are known; see notes at top
            insertScopeAlias(SAMN_IFTOP, varSymp, ifOrPortSymp);
        }
        m_ifaceVarSyms.clear();
    }

    فراغ insertScopeAlias(SAMNum samn, VSymEnt* lhsp, VSymEnt* rhsp) {
        // Track and later insert scope aliases; an interface referenced by
        // a child cell connecting to that interface
        // Typically lhsp=VAR w/dtype IFACEREF, rhsp=IFACE cell
        UINFO(9, "   insertScopeAlias se" << cvtToHex(lhsp) << " se" << cvtToHex(rhsp));
        UASSERT_OBJ(
            !(VN_IS(rhsp->nodep(), Cell) && !VN_IS(VN_AS(rhsp->nodep(), Cell)->modp(), Iface)),
            rhsp->nodep(), "Got a non-IFACE alias RHS");
        m_scopeAliasMap[samn].emplace(lhsp, rhsp);
    }
    فراغ computeScopeAliases() {
        UINFO(9, "computeIfaceAliases");
        لكل (صحيح samn = 0; samn < SAMN__MAX; ++samn) {
            لكل (ScopeAliasMap::iterator it = m_scopeAliasMap[samn].begin();
                 it != m_scopeAliasMap[samn].end(); ++it) {
                VSymEnt* ثابت lhsp = it->first;
                VSymEnt* srcp = lhsp;
                طالما (true) {  // Follow chain of aliases up to highest level non-alias
                    ثابت auto it2 = m_scopeAliasMap[samn].find(srcp);
                    𐤀𐤌 (it2 != m_scopeAliasMap[samn].end()) {
                        srcp = it2->second;
                        استمر;
                    } وإلا {
                        اكسر;
                    }
                }
                UINFO(9, "  iiasa: Insert alias se" << lhsp << " (" << lhsp->nodep()->typeName()
                                                    << ") <- se" << srcp << " " << srcp->nodep());
                // srcp should be an interface reference pointing to the interface we want to
                // import
                lhsp->importFromIface(symsp(), srcp);
                // Allow access to objects not permissible to be listed in a modport
                𐤀𐤌 (VN_IS(srcp->nodep(), Modport)) {
                    lhsp->importFromIface(symsp(), srcp->parentp(), true);
                }
            }
            // m_scopeAliasMap[samn].clear();  // Done with it, but put into debug file
        }
    }

خاص:
    VSymEnt* findWithAltFallback(VSymEnt* symp, ثابت string& name, ثابت string& altname) {
        VSymEnt* findp = symp->findIdFallback(name);
        𐤀𐤌 (findp) أرجع findp;
        𐤀𐤌 (altname != "") {
            UINFO(8, "     alt fallback");
            findp = symp->findIdFallback(altname);
        }
        أرجع findp;
    }

    VSymEnt* findWithAltFlat(VSymEnt* symp, ثابت string& name, ثابت string& altname) {
        VSymEnt* findp = symp->findIdFlat(name);
        𐤀𐤌 (findp) أرجع findp;
        𐤀𐤌 (altname != "") {
            UINFO(8, "     alt flat");
            findp = symp->findIdFlat(altname);
        }
        أرجع findp;
    }

عام:
    VSymEnt* findDotted(FileLine* refLocationp, VSymEnt* lookupSymp, ثابت string& dotname,
                        string& baddot, VSymEnt*& okSymp, منطقي firstId) {
        // Given a dotted hierarchy name, return where in scope it is
        // Note when dotname=="" we just fall through and return lookupSymp
        UINFO(8, "    dottedFind se" << cvtToHex(lookupSymp) << " '" << dotname << "'");
        string leftname = dotname;
        okSymp = lookupSymp;  // So can list bad scopes
        طالما (leftname != "") {  // foreach dotted part of xref name
            string::size_type pos = leftname.find('.');
            string ident;
            𐤀𐤌 (pos != string::npos) {
                ident = leftname.substr(0, pos);
                leftname = leftname.substr(pos + 1);
            } وإلا {
                ident = leftname;
                leftname = "";
            }
            baddot = ident;  // So user can see where they botched it
            okSymp = lookupSymp;
            string altIdent;
            𐤀𐤌 (!forScopeCreation()) {
                // GENFOR Begin is foo__BRA__##__KET__ after we've genloop unrolled,
                // but presently should be just "foo".
                // Likewise cell foo__[array] before we've expanded arrays is just foo.
                // Multi-dim iface arrays append multiple __BRA__..__KET__ suffixes; strip them
                // all.
                altIdent = ident;
                طالما (VString::endsWith(altIdent, "__KET__")) {
                    ثابت auto braPos = altIdent.rfind("__BRA__");
                    𐤀𐤌 (braPos == string::npos) اكسر;
                    altIdent = altIdent.substr(0, braPos);
                }
                𐤀𐤌 (altIdent == ident) altIdent.clear();
            }
            UINFO(8, "         id " << ident << " alt " << altIdent << " left " << leftname
                                    << " at se" << lookupSymp);
            // Spec says; Look at existing module (cellnames then modname),
            // then look up (inst name or modname)
            𐤀𐤌 (firstId) {
                // Check this module - subcellnames
                ثابت AstCell* cellp = lookupSymp ? VN_CAST(lookupSymp->nodep(), Cell)
                                                  : nullptr;  // Replicated below
                𐤀𐤌 (VSymEnt* ثابت findSymp = findWithAltFallback(lookupSymp, ident, altIdent)) {
                    lookupSymp = findSymp;
                }

                // Check this module - cur modname
                وإلا 𐤀𐤌 (cellp && cellp->modp()->origName() == ident) {
                }
                // $root we walk up to Netlist
                وإلا 𐤀𐤌 (ident == "$root") {
                    lookupSymp = rootEntp();
                    // We've added the '$root' module, now everything else is one lower
                    𐤀𐤌 (forScopeCreation()) {
                        lookupSymp = lookupSymp->findIdFlat(ident);
                        UASSERT(lookupSymp, "Cannot find $root module under netlist");
                    }
                }
                // Move up and check cellname + modname
                وإلا {
                    منطقي crossedCell = خطأ;  // Crossed a cell boundary
                    طالما (lookupSymp) {
                        lookupSymp = lookupSymp->parentp();
                        cellp = lookupSymp ? VN_CAST(lookupSymp->nodep(), Cell)
                                           : nullptr;  // Replicated above
                        𐤀𐤌 (lookupSymp) {
                            UINFO(9, "    Up to " << lookupSymp);
                            𐤀𐤌 (cellp || VN_IS(lookupSymp->nodep(), CellInline)) {
                                crossedCell = true;
                            }
                            𐤀𐤌 (cellp && cellp->modp()->origName() == ident) {
                                اكسر;
                            } وإلا 𐤀𐤌 (VSymEnt* ثابت findSym2p
                                       = findWithAltFallback(lookupSymp, ident, altIdent)) {
                                lookupSymp = findSym2p;
                                𐤀𐤌 (crossedCell && VN_IS(lookupSymp->nodep(), Var)) {
                                    UINFO(9, "    Not found but matches var name in parent "
                                                 << lookupSymp);
                                    أرجع nullptr;
                                }
                                اكسر;
                            }
                        } وإلا {
                            اكسر;
                        }
                    }
                    𐤀𐤌 (!lookupSymp) أرجع nullptr;  // Not found
                }
            } وإلا {  // Searching for middle path component, must be a cell or interface port
                VSymEnt* findSymp = findWithAltFlat(lookupSymp, ident, altIdent);
                𐤀𐤌 (findSymp) {
                    lookupSymp = findSymp;
                } وإلا {
                    // Try prefixed lookup for interface ports accessed through hierarchy
                    // (e.g., slave_inst.bus.data where bus is an interface port)
                    // After inlining, interface ports have prefixed names like
                    // top__DOT__slave_inst__DOT__bus
                    UINFO(8, "     middle-path: trying prefixed lookup for '" << ident << "'\n");
                    string baddot;
                    findSymp = findSymPrefixed(lookupSymp, ident, baddot, /*fallback=*/true);
                    UINFO(8, "     middle-path: prefixed lookup result: "
                                 << (findSymp ? "found" : "not found") << "\n");
                    𐤀𐤌 (findSymp) {
                        // Check if this is an interface reference variable
                        ثابت AstVar* varp = VN_CAST(findSymp->nodep(), Var);
                        𐤀𐤌 (!varp) {
                            𐤀𐤌 (ثابت AstVarScope* vscp = VN_CAST(findSymp->nodep(), VarScope)) {
                                varp = vscp->varp();
                            }
                        }
                        𐤀𐤌 (varp && varp->isIfaceRef()) {
                            // Found an interface port - use findSymp directly
                            // computeScopeAliases has already imported interface members
                            // into this symbol entry, so we use it instead of redirecting
                            // to the shared modport definition (which would cause all
                            // instances to resolve to the same symbols - bug #2656)
                            lookupSymp = findSymp;
                        } وإلا {
                            // Non-interface symbol in middle path must be a cell (module instance)
                            // to continue hierarchical resolution
                            𐤀𐤌 (VN_IS(findSymp->nodep(), Cell)) {
                                lookupSymp = findSymp;
                            } وإلا {
                                // Reject non-cell, non-interface symbols found via fallback
                                // to prevent accidental resolution to unrelated symbols
                                UINFO(8, "     middle-path: rejecting non-cell symbol\n");
                                أرجع nullptr;
                            }
                        }
                    } وإلا {
                        أرجع nullptr;  // Not found
                    }
                }
            }
            𐤀𐤌 (lookupSymp) {
                𐤀𐤌 (ثابت AstCell* ثابت cellp = VN_CAST(lookupSymp->nodep(), Cell)) {
                    𐤀𐤌 (ثابت AstNodeModule* ثابت modp = cellp->modp()) {
                        𐤀𐤌 (modp->hierBlock() && !leftname.empty()) {
                            refLocationp->v3error("Cannot access scope inside hierarchical block");
                        } وإلا 𐤀𐤌 (VN_IS(modp, NotFoundModule)) {
                            refLocationp->v3error("Dotted reference to instance that refers to "
                                                  "missing module/interface: "
                                                  << modp->prettyNameQ());
                        }
                    }
                }
                // Follow scope alias for nested interface port access
                𐤀𐤌 (!leftname.empty()) {
                    ثابت auto aliasIt = m_scopeAliasMap[SAMN_IFTOP].find(lookupSymp);
                    𐤀𐤌 (aliasIt != m_scopeAliasMap[SAMN_IFTOP].end()) {
                        lookupSymp = aliasIt->second;
                        // Alias may point to __Viftop VarScope; find corresponding Cell
                        𐤀𐤌 (ثابت AstVarScope* ثابت vscp
                            = VN_CAST(lookupSymp->nodep(), VarScope)) {
                            ثابت string varName = vscp->varp()->name();
                            ساكن constexpr ثابت حرف* ثابت VIFTOP_SUFFIX = "__Viftop";
                            𐤀𐤌 (VString::endsWith(varName, VIFTOP_SUFFIX)) {
                                ثابت string cellName
                                    = varName.substr(0, varName.size() - strlen(VIFTOP_SUFFIX));
                                VSymEnt* ثابت parentSymp = lookupSymp->parentp();
                                𐤀𐤌 (parentSymp) {
                                    VSymEnt* ثابت cellSymp = parentSymp->findIdFlat(cellName);
                                    𐤀𐤌 (cellSymp && VN_IS(cellSymp->nodep(), Cell)) {
                                        lookupSymp = cellSymp;  // Use Cell for member lookup
                                    }
                                }
                            }
                        }
                    } وإلا {
                        // No alias; try following IfaceRefDType to interface cell
                        ثابت AstVar* varp = VN_CAST(lookupSymp->nodep(), Var);
                        𐤀𐤌 (!varp) {
                            𐤀𐤌 (ثابت AstVarScope* ثابت vscp
                                = VN_CAST(lookupSymp->nodep(), VarScope)) {
                                varp = vscp->varp();
                            }
                        }
                        𐤀𐤌 (varp && varp->isIfaceRef()) {
                            𐤀𐤌 (ثابت AstIfaceRefDType* ثابت ifaceRefp
                                = ifaceRefFromArray(varp->dtypep())) {
                                𐤀𐤌 (ifaceRefp->cellp() && existsNodeSym(ifaceRefp->cellp())) {
                                    lookupSymp = getNodeSym(ifaceRefp->cellp());
                                } وإلا 𐤀𐤌 (ifaceRefp->ifaceViaCellp()
                                           && existsNodeSym(ifaceRefp->ifaceViaCellp())) {
                                    lookupSymp = getNodeSym(ifaceRefp->ifaceViaCellp());
                                } وإلا 𐤀𐤌 (ifaceRefp->ifacep()
                                           && existsNodeSym(ifaceRefp->ifacep())) {
                                    lookupSymp = getNodeSym(ifaceRefp->ifacep());
                                }
                            }
                        }
                    }
                }
            }
            firstId = خطأ;
        }
        أرجع lookupSymp;
    }

    ساكن string removeLastInlineScope(ثابت string& name) {
        string out = name;
        ثابت string dot = "__DOT__";
        ثابت size_t dotPos = out.rfind(dot, out.size() - dot.length() - 1);
        𐤀𐤌 (dotPos == string::npos) {
            أرجع "";
        } وإلا {
            أرجع out.erase(dotPos + dot.length(), string::npos);
        }
    }

    VSymEnt* findSymPrefixed(VSymEnt* lookupSymp, ثابت string& dotname, string& baddot,
                             منطقي fallback) {
        // Find symbol in given point in hierarchy, allowing prefix (post-Inline)
        // For simplicity lookupSymp may be passed nullptr result from findDotted
        𐤀𐤌 (!lookupSymp) أرجع nullptr;
        UINFO(8, "    findSymPrefixed "
                     << dotname << " under se" << cvtToHex(lookupSymp)
                     << ((lookupSymp->symPrefix() == "") ? "" : " as ")
                     << ((lookupSymp->symPrefix() == "") ? "" : lookupSymp->symPrefix() + dotname)
                     << "  at se" << lookupSymp << " fallback=" << fallback);
        string prefix = lookupSymp->symPrefix();
        VSymEnt* foundp = nullptr;
        طالما (!foundp) {
            𐤀𐤌 (fallback) {
                foundp = lookupSymp->findIdFallback(prefix + dotname);  // Might be nullptr
            } وإلا {
                foundp = lookupSymp->findIdFlat(prefix + dotname);  // Might be nullptr
            }
            𐤀𐤌 (prefix.empty()) اكسر;
            ثابت string nextPrefix = removeLastInlineScope(prefix);
            𐤀𐤌 (prefix == nextPrefix) اكسر;
            prefix = nextPrefix;
        }
        𐤀𐤌 (!foundp) baddot = dotname;
        أرجع foundp;
    }
    ساكن منطقي checkIfClassOrPackage(ثابت VSymEnt* ثابت symp) {
        𐤀𐤌 (VN_IS(symp->nodep(), Class) || VN_IS(symp->nodep(), Package)) أرجع true;

        // Helper: check if a RefDType might resolve to a class later.
        ثابت auto isPotentialClassRef = [](ثابت AstRefDType* refp) -> منطقي {
            أرجع refp && !refp->typeofp()
                   && (!refp->classOrPackageOpp()
                       || VN_IS(refp->classOrPackageOpp(), ClassOrPackageRef));
        };

        // Helper: allow types that can represent a class/package handle or an unresolved ref.
        // BasicDType is allowed here only for null/object handles.
        ثابت auto isValidTypeNode = [](ثابت AstNode* nodep) -> منطقي {
            أرجع VN_IS(nodep, VoidDType) || VN_IS(nodep, BasicDType)
                   || VN_IS(nodep, ClassRefDType) || VN_IS(nodep, ParseRef);
        };

        𐤀𐤌 (ثابت AstTypedef* ثابت typedefp = VN_CAST(symp->nodep(), Typedef)) {
            ثابت AstNodeDType* dtypep = typedefp->subDTypep();
            𐤀𐤌 (!dtypep) dtypep = typedefp->childDTypep();
            𐤀𐤌 (VN_IS(dtypep, ClassRefDType)) أرجع true;
            𐤀𐤌 (isPotentialClassRef(VN_CAST(dtypep, RefDType))) أرجع true;
        } وإلا 𐤀𐤌 (ثابت AstParamTypeDType* ثابت paramTypep
                   = VN_CAST(symp->nodep(), ParamTypeDType)) {
            // Before V3Param the declared default is in childDTypep (possibly
            // wrapped in a RequireDType); after V3Param it is consumed and the
            // bound type is the resolved data type, e.g. a type parameter
            // inherited from a specialized base class (REQ #(Item) -> class Item).
            AstNode* childp = paramTypep->childDTypep();
            𐤀𐤌 (ثابت AstRequireDType* ثابت reqp = VN_CAST(childp, RequireDType)) {
                childp = reqp->lhsp();
            }
            ثابت AstNode* ثابت checkp = childp ? childp : paramTypep->skipRefp();
            𐤀𐤌 (isValidTypeNode(checkp) || isPotentialClassRef(VN_CAST(checkp, RefDType))) {
                أرجع true;
            }
        }
        أرجع خطأ;
    }
    VSymEnt* resolveClassOrPackage(VSymEnt* lookSymp, AstClassOrPackageRef* nodep, منطقي fallback,
                                   منطقي classOnly, ثابت string& forWhat,
                                   منطقي deferIfUnresolved = خطأ) {
        𐤀𐤌 (nodep->classOrPackageSkipp()) أرجع getNodeSym(nodep->classOrPackageSkipp());
        VSymEnt* foundp;
        VSymEnt* searchSymp = lookSymp;
        𐤀𐤌 (VL_UNCOVERABLE(searchSymp && VN_IS(searchSymp->nodep(), ParamTypeDType))) {
            searchSymp->nodep()->v3fatalSrc(  // LCOV_EXCL_LINE
                "resolveClassOrPackage: unexpected ParamTypeDType lookup");
        }
        𐤀𐤌 (fallback) {
            VSymEnt* currentLookSymp = searchSymp;
            𐤐𐤏𐤋 {
                foundp = currentLookSymp->findIdFlat(nodep->name());
                𐤀𐤌 (foundp && !checkIfClassOrPackage(foundp)) foundp = nullptr;
                𐤀𐤌 (!foundp) currentLookSymp = currentLookSymp->fallbackp();
            } طالما (!foundp && currentLookSymp);
        } وإلا {
            foundp = searchSymp->findIdFlat(nodep->name());
            𐤀𐤌 (foundp && !checkIfClassOrPackage(foundp)) foundp = nullptr;
        }
        𐤀𐤌 (!foundp && v3Global.rootp()->stdPackagep()) {  // Look under implied std::
            foundp = getNodeSym(v3Global.rootp()->stdPackagep())->findIdFlat(nodep->name());
        }
        𐤀𐤌 (foundp) {
            nodep->classOrPackageNodep(foundp->nodep());
            أرجع foundp;
        }
        𐤀𐤌 (deferIfUnresolved) أرجع nullptr;
        ثابت string suggest
            = suggestSymFallback(lookSymp, nodep->name(), LinkNodeMatcherClassOrPackage{});
        nodep->v3error((classOnly ? "Class" : "Package/class")
                       << " for '" << forWhat  // extends/implements
                       << "' not found: " << nodep->prettyNameQ() << '\n'
                       << (suggest.empty() ? "" : nodep->warnMore() + suggest));
        أرجع nullptr;
    }
    string suggestSymFallback(VSymEnt* lookupSymp, ثابت string& name,
                              ثابت VNodeMatcher& matcher) {
        // Suggest alternative symbol in given point in hierarchy
        // Does not support inline, as we find user-level errors before inlining
        // For simplicity lookupSymp may be passed nullptr result from findDotted
        𐤀𐤌 (!lookupSymp) أرجع "";
        VSpellCheck speller;
        lookupSymp->candidateIdFallback(&speller, &matcher);
        أرجع speller.bestCandidateMsg(name);
    }
    string suggestSymFlat(VSymEnt* lookupSymp, ثابت string& name, ثابت VNodeMatcher& matcher,
                          منطقي includeType) {
        𐤀𐤌 (!lookupSymp) أرجع "";
        VSpellCheck speller;
        lookupSymp->candidateIdFlat(&speller, &matcher, includeType);
        أرجع speller.bestCandidateMsg(name);
    }
};

LinkDotState* LinkDotState::s_errorThisp = nullptr;

//======================================================================

صنف LinkDotFindVisitor final : عام VNVisitor {
    // NODE STATE
    // Cleared on global
    //  *::user1p()             -> See LinkDotState
    //  *::user2p()             -> See LinkDotState
    //  *::user3()              // bool. Node stored in m_virtualIfaces
    //  *::user4()              -> See LinkDotState
    ثابت VNUser3InUse m_inuser3;

    // STATE
    LinkDotState* ثابت m_statep;  // State to pass between visitors, including symbol table
    AstNodeModule* m_classOrPackagep = nullptr;  // Current package
    AstClocking* m_clockingp = nullptr;  // Current clocking block
    std::vector<AstScope*> m_scopes;  // Scopes detached during repeated module visits
    uint64_t m_statScopeVisits = 0;  // Non-top scopes encountered while finding instance symbols
    VSymEnt* m_modSymp = nullptr;  // Symbol Entry for current module
    VSymEnt* m_curSymp = nullptr;  // Symbol Entry for current table, where to lookup/insert
    string
        m_hierParamsName;  // Name of module with hierarchical type parameters, empty when not used
    string m_scope;  // Scope text
    ثابت AstNodeFTask* m_ftaskp = nullptr;  // Current function/task
    منطقي m_inRecursion = خطأ;  // Inside a recursive module
    صحيح m_paramNum = 0;  // Parameter number, for position based connection
    منطقي m_explicitNew = خطأ;  // Hit a "new" function
    صحيح m_modBlockNum = 0;  // Begin block number in module, 0=none seen
    صحيح m_modWithNum = 0;  // With block number, 0=none seen
    صحيح m_modArgNum = 0;  // Arg block number for randomize(), 0=none seen
    std::vector<AstIface*> m_virtIfaces;  // interfaces used as virtual,
                                          // needed for handleUnvisitedVirtIfaces()

    // METHODS
    فراغ makeImplicitNew(AstClass* nodep) {
        AstFunc* ثابت newp = جديد AstFunc{nodep->fileline(), "new", nullptr, nullptr};
        // If needed, super.new() call is added the 2nd pass of V3LinkDotResolve,
        // because base classes are already resolved there.
        newp->isConstructor(true);
        nodep->addMembersp(newp);
        UINFO(8, "Made implicit new for " << nodep->name() << ": " << nodep);
        m_statep->insertBlock(m_curSymp, newp->name(), newp, m_classOrPackagep);
    }

    ساكن منطقي isHierBlockWrapper(ثابت string& name) {
        ثابت V3HierBlockOptSet& hierBlocks = v3Global.opt.hierBlocks();
        أرجع hierBlocks.find(name) != hierBlocks.end();
    }

    فراغ moveExternFuncDecl(AstNodeFTask* nodep, AstClass* classp) {
        // Move an 'extern function|task' to inside a class or, from an outer to inner-class
        𐤀𐤌 (nodep->isExternDef()) أرجع;
        // Move it to proper spot under the target class
        nodep->unlinkFrBack();
        classp->addStmtsp(nodep);
        nodep->isExternDef(true);  // So we check there's a matching extern
        nodep->classOrPackagep()->unlinkFrBack()->deleteTree();
        // Any "Type::" reference in the function's IO are really "MovedToClass::" references
        𐤀𐤌 (nodep->fvarp()) moveExternFuncDeclRefs(nodep->fvarp(), classp);
        لكل (AstNode* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            𐤀𐤌 (AstVar* ثابت portp = VN_CAST(stmtp, Var)) {
                𐤀𐤌 (portp->isIO()) moveExternFuncDeclRefs(portp, classp);
            }
        }
    }
    فراغ moveExternFuncDeclRefs(AstNode* nodep, AstClass* classp) {
        nodep->foreach([هذا, classp](AstClassOrPackageRef* refp) {  //
            𐤀𐤌 (refp->name() == classp->name() && !refp->paramsp()) {
                UINFO(9, "Cleaning up external function type for class " << refp);
                AstDot* ثابت dotp = VN_CAST(refp->backp(), Dot);
                𐤀𐤌 (dotp && dotp->lhsp() == refp) {
                    // If had DOT(CLASSREF this, ...) the DOT can go away
                    dotp->replaceWith(dotp->rhsp()->unlinkFrBack());
                    VL_DO_DANGLING2(pushDeletep(dotp), dotp, refp);
                } وإلا {
                    VL_DO_DANGLING(pushDeletep(refp->unlinkFrBack()), refp);
                }
            }
        });
    }

    // VISITORS
    فراغ visit(AstNetlist* nodep) override {  // FindVisitor::
        // Process $unit or other packages
        // Not needed - dotted references not allowed from inside packages
        // for (AstNodeModule* nodep = v3Global.rootp()->modulesp();
        //     nodep; nodep=VN_AS(nodep->nextp(), NodeModule)) {
        //    if (VN_IS(nodep, Package)) {}}

        m_statep->insertDUnit(nodep);

        // First back iterate, to find all packages. Backward as must do base
        // packages before using packages
        iterateChildrenBackwardsConst(nodep);

        // The first modules in the list are always the top modules
        // (sorted before this is called).
        // This may not be the module with isTop() set, as early in the steps,
        // wrapTop may have not been created yet.
        // $unit and the constant pool always exist, so nothing else, and nothing in $unit,
        // means nothing was given
        AstNodeModule* ثابت modulesp = nodep->modulesp();
        UASSERT_OBJ(modulesp, nodep, "$unit should always be in the netlist");
        𐤀𐤌 (m_statep->forPrimary()) {
            ثابت منطقي onlyBuiltins = [&]() {
                لكل (AstNodeModule* modp = modulesp; modp;
                     modp = VN_AS(modp->nextp(), NodeModule)) {
                    𐤀𐤌 (!modp->isDollarUnit() && !modp->isConstPool()) أرجع خطأ;
                }
                أرجع true;
            }();
            𐤀𐤌 (onlyBuiltins && !v3Global.rootp()->dollarUnitPkgp()->stmtsp()) {
                nodep->v3error("No top level module found");
            }
        }
        لكل (AstNodeModule* modp = nodep->modulesp(); modp && modp->isTop();
             modp = VN_AS(modp->nextp(), NodeModule)) {
            UINFO(8, "Top Module: " << modp);
            m_scope = "TOP";

            𐤀𐤌 (!m_statep->forScopeCreation() && v3Global.opt.topIfacesSupported()) {
                لكل (AstNode* subnodep = modp->stmtsp(); subnodep; subnodep = subnodep->nextp()) {
                    𐤀𐤌 (AstVar* ثابت varp = VN_CAST(subnodep, Var)) {
                        𐤀𐤌 (varp->isIfaceRef()) {
                            ثابت AstNodeDType* ثابت subtypep = varp->subDTypep();
                            ثابت AstIfaceRefDType* ifacerefp = nullptr;
                            𐤀𐤌 (VN_IS(subtypep, IfaceRefDType)) {
                                ifacerefp = VN_AS(varp->subDTypep(), IfaceRefDType);
                            } وإلا 𐤀𐤌 (VN_IS(subtypep, BracketArrayDType)) {
                                ثابت AstBracketArrayDType* ثابت arrp
                                    = VN_AS(subtypep, BracketArrayDType);
                                ثابت AstNodeDType* ثابت arrsubtypep = arrp->subDTypep();
                                𐤀𐤌 (VN_IS(arrsubtypep, IfaceRefDType)) {
                                    ifacerefp = VN_AS(arrsubtypep, IfaceRefDType);
                                }
                            } وإلا 𐤀𐤌 (VN_IS(subtypep, UnpackArrayDType)) {
                                ثابت AstUnpackArrayDType* ثابت arrp
                                    = VN_AS(subtypep, UnpackArrayDType);
                                ثابت AstNodeDType* ثابت arrsubtypep = arrp->subDTypep();
                                𐤀𐤌 (VN_IS(arrsubtypep, IfaceRefDType)) {
                                    ifacerefp = VN_AS(arrsubtypep, IfaceRefDType);
                                }
                            }

                            𐤀𐤌 (ifacerefp && !ifacerefp->cellp()) {
                                // A dummy cell to keep the top level interface alive and correctly
                                // optimized for default parameter values
                                AstCell* ifacecellp
                                    = جديد AstCell{nodep->fileline(),
                                                  nodep->fileline(),
                                                  modp->name() + "__02E" + varp->name(),
                                                  ifacerefp->ifaceName(),
                                                  nullptr,
                                                  nullptr,
                                                  nullptr};
                                ifacecellp->modp(ifacerefp->ifacep());
                                m_curSymp = m_modSymp
                                    = m_statep->insertTopIface(ifacecellp, m_scope);
                                iterate(ifacecellp);
                                VL_DO_DANGLING(pushDeletep(ifacecellp), ifacecellp);
                            }
                        }
                    }
                }
            }

            m_curSymp = m_modSymp = m_statep->insertTopCell(modp, m_scope);
            { iterate(modp); }

            m_scope = "";
            m_curSymp = m_modSymp = nullptr;
        }
    }
    فراغ visit(AstTypeTable*) override {}  // FindVisitor::
    فراغ visit(AstIfaceRefDType* nodep) override {  // FindVisitor::
        𐤀𐤌 (!m_statep->forScopeCreation() && nodep->isVirtual() && nodep->ifacep()
            && !nodep->ifacep()->user3()) {
            m_virtIfaces.push_back(nodep->ifacep());
            nodep->ifacep()->user3(true);
        }
        iterateChildren(nodep);
    }
    فراغ visit(AstNodeModule* nodep) override {  // FindVisitor::
        // Called on top module from Netlist, other modules from the cell creating them,
        // and packages
        UINFO(8, "   " << nodep);
        // m_curSymp/m_modSymp maybe nullptr for packages and non-top modules
        // Packages will be under top after the initial phases, but until then
        // need separate handling
        ثابت منطقي standalonePkg
            = !m_modSymp && !m_statep->forScopeCreation() && VN_IS(nodep, Package);
        ثابت منطقي doit = (m_modSymp || standalonePkg);
        VL_RESTORER_COPY(m_scope);
        VL_RESTORER(m_classOrPackagep);
        VL_RESTORER(m_modSymp);
        VL_RESTORER(m_curSymp);
        VL_RESTORER(m_paramNum);
        VL_RESTORER(m_modBlockNum);
        VL_RESTORER(m_modWithNum);
        VL_RESTORER(m_modArgNum);
        𐤀𐤌 (doit && nodep->user2()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Identically recursive module (module instantiates "
                          "itself, without changing parameters): "
                              << AstNode::prettyNameQ(nodep->origName()));
        } وإلا 𐤀𐤌 (doit) {
            UINFO(4, "     Link Module: " << nodep);
            UASSERT_OBJ(!nodep->dead(), nodep, "Module in instance tree mislabeled as dead?");
            AstPackage* ثابت pkgp = VN_CAST(nodep, Package);
            m_classOrPackagep = pkgp;
            𐤀𐤌 (standalonePkg) {
                𐤀𐤌 (pkgp->isDollarUnit()) {
                    m_curSymp = m_modSymp = m_statep->dunitEntp();
                    nodep->user1p(m_curSymp);
                } وإلا {
                    VSymEnt* ثابت upperSymp = m_statep->dunitEntp();
                    m_scope = nodep->name();
                    m_curSymp = m_modSymp = m_statep->insertBlock(upperSymp, nodep->name(), nodep,
                                                                  m_classOrPackagep);
                    UINFO(9, "New module scope " << m_curSymp);
                }
            }
            //
            m_paramNum = 0;
            m_modBlockNum = 0;
            m_modWithNum = 0;
            m_modArgNum = 0;
            // m_modSymp/m_curSymp for non-packages set by AstCell above this module
            // Iterate
            nodep->user2(true);
            iterateChildren(nodep);
            nodep->user2(خطأ);
            nodep->user4(true);
            // Interfaces need another pass when signals are resolved. When creating
            // scopes this is done from the AstScopes in LinkDotScopeVisitor instead, as
            // only those entries hold the AstVarScopes the modport items resolve against.
            𐤀𐤌 (!m_statep->forScopeCreation()) {
                𐤀𐤌 (AstIface* ثابت ifacep = VN_CAST(nodep, Iface)) {
                    m_statep->insertIfaceModSym(ifacep, m_curSymp);
                }
            }
        } وإلا 𐤀𐤌 (isHierBlockWrapper(nodep->name())) {
            UINFO(5, "Module is hierarchical block, must not be dead: " << nodep);
            m_scope = nodep->name();
            VSymEnt* ثابت upperSymp = m_curSymp ? m_curSymp : m_statep->rootEntp();
            m_curSymp = m_modSymp
                = m_statep->insertBlock(upperSymp, nodep->name() + "::", nodep, m_classOrPackagep);
            iterateChildren(nodep);
            nodep->user4(true);
        } وإلا {  // !doit
            𐤀𐤌 (nodep->hierParams()) {
                UINFO(1, "Found module with hier type parameters");
                m_hierParamsName = nodep->name();
                لكل (ثابت AstNode* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    𐤀𐤌 (ثابت AstTypedef* ثابت tdef = VN_CAST(stmtp, Typedef)) {
                        UINFO(1, "Inserting hier type parameter typedef: " << tdef);
                        VSymEnt* ثابت upperSymp = m_curSymp ? m_curSymp : m_statep->rootEntp();
                        m_curSymp = m_modSymp = m_statep->insertBlock(upperSymp, nodep->name(),
                                                                      nodep, m_classOrPackagep);
                    }
                }
            } وإلا {
                // Will be optimized away later
                // Can't remove now, as our backwards iterator will throw up
                UINFO(5, "Module not under any CELL or top - dead module: " << nodep);
            }
        }
    }

    فراغ visit(AstClass* nodep) override {  // FindVisitor::
        UASSERT_OBJ(m_curSymp, nodep, "Class not under module/package/$unit");
        UINFO(8, "   " << nodep);
        VL_RESTORER_COPY(m_scope);
        VL_RESTORER(m_classOrPackagep);
        VL_RESTORER(m_modSymp);
        VL_RESTORER(m_curSymp);
        VL_RESTORER(m_paramNum);
        VL_RESTORER(m_modBlockNum);
        VL_RESTORER(m_modWithNum);
        VL_RESTORER(m_modArgNum);
        VL_RESTORER(m_explicitNew);
        {
            UINFO(4, "     Link Class: " << nodep);
            VSymEnt* ثابت upperSymp = m_curSymp;
            m_scope = m_scope + "." + nodep->name();
            m_classOrPackagep = nodep;
            m_curSymp = m_modSymp
                = m_statep->insertBlock(upperSymp, nodep->name(), nodep, m_classOrPackagep);
            m_statep->insertMap(m_curSymp, m_scope);
            UINFO(9, "New module scope " << m_curSymp);
            //
            m_paramNum = 0;
            m_modBlockNum = 0;
            m_modWithNum = 0;
            m_modArgNum = 0;
            m_explicitNew = خطأ;
            // m_modSymp/m_curSymp for non-packages set by AstCell above this module
            // Iterate
            iterateChildren(nodep);
            nodep->user4(true);
            // Implicit new needed?
            𐤀𐤌 (!m_explicitNew && m_statep->forPrimary()) makeImplicitNew(nodep);
        }
    }
    فراغ visit(AstClassOrPackageRef* nodep) override {  // FindVisitor::
        𐤀𐤌 (!nodep->classOrPackageNodep() && nodep->name() == "$unit") {
            nodep->classOrPackageNodep(v3Global.rootp()->dollarUnitPkgp());
        }
        iterateChildren(nodep);
    }
    فراغ visit(AstScope* nodep) override {  // FindVisitor::
        UASSERT_OBJ(m_statep->forScopeCreation(), nodep,
                    "Scopes should only exist right after V3Scope");
        // Detach on first encounter, will re add at the end.
        // Otherwise would be visited O(cells^2) time after V3Scope
        𐤀𐤌 (!nodep->isTop()) {
            ++m_statScopeVisits;
            nodep->unlinkFrBack();
            m_scopes.push_back(nodep);
        }
    }
    فراغ visit(AstCell* nodep) override {  // FindVisitor::
        UINFO(5, "   CELL under " << m_scope << " is " << nodep);
        // Process XREFs/etc inside pins
        𐤀𐤌 (nodep->recursive() && m_inRecursion) أرجع;
        iterateChildren(nodep);
        // Recurse in, preserving state
        VL_RESTORER_COPY(m_scope);
        VL_RESTORER(m_modSymp);
        VL_RESTORER(m_curSymp);
        VL_RESTORER(m_paramNum);
        VL_RESTORER(m_inRecursion);
        // Where do we add it?
        VSymEnt* aboveSymp = m_curSymp;
        ثابت string origname = AstNode::dedotName(nodep->name());
        string::size_type pos = origname.rfind('.');
        𐤀𐤌 (pos != string::npos) {
            // Flattened, find what CellInline it should live under
            ثابت string scope = origname.substr(0, pos);
            string baddot;
            VSymEnt* okSymp;
            aboveSymp
                = m_statep->findDotted(nodep->fileline(), aboveSymp, scope, baddot, okSymp, خطأ);
            UASSERT_OBJ(aboveSymp, nodep,
                        "Can't find instance insertion point at "
                            << AstNode::prettyNameQ(baddot) << " in: " << nodep->prettyNameQ());
        }
        {
            m_scope = m_scope + "." + nodep->name();
            m_curSymp = m_modSymp = m_statep->insertCell(aboveSymp, m_modSymp, nodep, m_scope);
            m_inRecursion = nodep->recursive();
            // We don't report NotFoundModule, as may be a unused module in a generate
            𐤀𐤌 (nodep->modp()) iterate(nodep->modp());
        }
    }
    فراغ visit(AstCellInline* nodep) override {  // FindVisitor::
        UINFO(5, "   CELLINLINE under " << m_scope << " is " << nodep);
        VSymEnt* aboveSymp = m_curSymp;
        std::string ident = nodep->name();
        // If baz__DOT__foo__DOT__bar, we need to find baz__DOT__foo and add bar to it.
        ثابت std::string::size_type pos = ident.rfind("__DOT__");
        𐤀𐤌 (pos != std::string::npos) {
            ثابت std::string dotted = ident.substr(0, pos);
            ident = ident.substr(pos + std::strlen("__DOT__"));
            std::string baddot;
            VSymEnt* okSymp;
            aboveSymp = m_statep->findDotted(nodep->fileline(), aboveSymp, dotted, baddot, okSymp,
                                             خطأ);
            UASSERT_OBJ(aboveSymp, nodep,
                        "Can't find cellinline insertion point at "
                            << AstNode::prettyNameQ(baddot) << " in: " << nodep->prettyNameQ());
        }
        m_statep->insertInline(aboveSymp, m_modSymp, nodep, ident);
    }
    فراغ visit(AstDefParam* nodep) override {  // FindVisitor::
        nodep->user1p(m_curSymp);
        iterateChildren(nodep);
    }
    فراغ visit(AstRefDType* nodep) override {  // FindVisitor::
        nodep->user1p(m_curSymp);
        iterateChildren(nodep);
    }
    فراغ visit(AstGenBlock* nodep) override {  // FindVisitor::
        UINFO(5, "   " << nodep);
        𐤀𐤌 (nodep->name() == "" && nodep->unnamed()) {
            // Unnamed blocks are only important when they contain var
            // decls, so search for them. (Otherwise adding all the
            // unnamed#'s would just confuse tracing variables in
            // places such as tasks, where "task ...; begin ... end"
            // are common.
            لكل (AstNode* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
                𐤀𐤌 (VN_IS(itemp, Var) || VN_IS(itemp, Foreach)) {
                    std::string name;
                    ثابت std::string stepStr = m_statep->forPrimary()
                                                    ? ""
                                                    : std::to_string(m_statep->stepNumber()) + "_";
                    𐤐𐤏𐤋 {
                        ++m_modBlockNum;
                        name = "unnamedblk" + stepStr + cvtToStr(m_modBlockNum);
                        // Increment again if earlier pass of V3LinkDot claimed this name
                    } طالما (m_curSymp->findIdFlat(name));
                    nodep->name(name);
                    اكسر;
                }
            }
        }
        𐤀𐤌 (nodep->name() == "") {
            iterateChildren(nodep);
        } وإلا {
            VL_RESTORER(m_curSymp);
            {
                m_curSymp
                    = m_statep->insertBlock(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
                m_curSymp->fallbackp(VL_RESTORER_PREV(m_curSymp));
                // Iterate
                iterateChildren(nodep);
            }
        }
    }
    فراغ visit(AstNodeBlock* nodep) override {  // FindVisitor::
        UINFO(5, "   " << nodep);
        𐤀𐤌 (nodep->name() == "" && nodep->unnamed()) {
            // Unnamed blocks are only important when they contain var
            // decls, so search for them. (Otherwise adding all the
            // unnamed#'s would just confuse tracing variables in
            // places such as tasks, where "task ...; begin ... end"
            // are common.
            ثابت auto containsDecl = [](ثابت AstNode* nodesp) -> منطقي {
                لكل (ثابت AstNode* np = nodesp; np; np = np->nextp()) {
                    𐤀𐤌 (VN_IS(np, Var) || VN_IS(np, Foreach)) أرجع true;
                }
                أرجع خطأ;
            };
            منطقي needName = nodep->declsp() || containsDecl(nodep->stmtsp());
            𐤀𐤌 (ثابت AstFork* ثابت forkp = VN_CAST(nodep, Fork)) {
                needName = needName || containsDecl(forkp->forksp());
            }
            𐤀𐤌 (needName) {
                ثابت std::string stepStr
                    = m_statep->forPrimary() ? "" : std::to_string(m_statep->stepNumber()) + "_";
                std::string name;
                𐤐𐤏𐤋 {
                    ++m_modBlockNum;
                    name = "unnamedblk" + stepStr + cvtToStr(m_modBlockNum);
                    // Increment again if earlier pass of V3LinkDot claimed this name
                } طالما (m_curSymp->findIdFlat(name));
                nodep->name(name);
            }
        }
        𐤀𐤌 (nodep->name() == "") {
            iterateChildren(nodep);
        } وإلا {
            VL_RESTORER(m_curSymp);
            {
                m_curSymp
                    = m_statep->insertBlock(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
                m_curSymp->fallbackp(VL_RESTORER_PREV(m_curSymp));
                // Iterate
                iterateChildren(nodep);
            }
        }
    }
    // Handle out-of-block interface method definition (IEEE 1800-2023 25.7)
    // Move task body from module into the interface's prototype task,
    // rewriting port-prefixed references in the body.
    فراغ moveIfaceExportBody(AstNodeFTask* nodep) {
        ثابت string& portName = nodep->ifacePortName();
        UINFO(5, "   moveIfaceExportBody: port=" << portName << " task=" << nodep->name());
        // Look up the interface port variable in the module's symbol table
        VSymEnt* ثابت portSymp = m_modSymp->findIdFallback(portName);
        𐤀𐤌 (!portSymp) {
            nodep->v3error("Interface port not found for out-of-block definition: '"
                           << portName << "' (IEEE 1800-2023 25.7)");
            nodep->unlinkFrBack();
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
            أرجع;
        }
        AstVar* ثابت portp = VN_CAST(portSymp->nodep(), Var);
        𐤀𐤌 (!portp) {
            nodep->v3fatalSrc("Out-of-block definition port is not a variable: " << portName);
            أرجع;
        }
        // Get the interface name from the port's type
        // At this stage dtypep() may not be set yet; check childDTypep() too
        AstIfaceRefDType* ifaceRefDtp = nullptr;
        𐤀𐤌 (portp->dtypep()) { ifaceRefDtp = VN_CAST(portp->dtypep(), IfaceRefDType); }
        𐤀𐤌 (!ifaceRefDtp && portp->childDTypep()) {
            ifaceRefDtp = VN_CAST(portp->childDTypep(), IfaceRefDType);
        }
        𐤀𐤌 (!ifaceRefDtp) {
            nodep->v3error("Out-of-block definition port is not an interface port: '"
                           << portName << "' (IEEE 1800-2023 25.7)");
            nodep->unlinkFrBack();
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
            أرجع;
        }
        ثابت string ifaceName = ifaceRefDtp->ifaceName();
        UINFO(5, "   moveIfaceExportBody: iface=" << ifaceName);
        // Find the interface module by name
        AstNodeModule* ثابت ifaceModp = m_statep->findModuleSym(ifaceName);
        𐤀𐤌 (!ifaceModp) {
            nodep->v3fatalSrc("Interface not found for out-of-block definition: " << ifaceName);
            أرجع;
        }
        // Find the prototype task in the interface
        AstNodeFTask* protoTaskp = nullptr;
        لكل (AstNode* itemp = ifaceModp->stmtsp(); itemp; itemp = itemp->nextp()) {
            𐤀𐤌 (AstNodeFTask* ثابت ftaskp = VN_CAST(itemp, NodeFTask)) {
                𐤀𐤌 (ftaskp->name() == nodep->name() && ftaskp->prototype()) {
                    protoTaskp = ftaskp;
                    اكسر;
                }
            }
        }
        𐤀𐤌 (!protoTaskp) {
            nodep->v3error("No matching export prototype found in interface "
                           << ifaceName << " for task " << nodep->prettyNameQ()
                           << " (IEEE 1800-2023 25.7)");
            nodep->unlinkFrBack();
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
            أرجع;
        }
        // Move body statements (non-port declarations) from the out-of-block task
        // to the prototype task. Rewrite port.X references to just X in the body.
        لكل (AstNode* stmtp = nodep->stmtsp(); stmtp;) {
            AstNode* ثابت nextp = stmtp->nextp();
            // Skip port declarations -- they already exist in the prototype
            𐤀𐤌 (ثابت AstVar* ثابت varp = VN_CAST(stmtp, Var)) {
                𐤀𐤌 (varp->isIO()) {
                    stmtp = nextp;
                    استمر;
                }
            }
            // Clone this statement and rewrite port references
            AstNode* ثابت newStmtp = stmtp->cloneTree(خطأ);
            rewriteIfacePortRefsSingle(newStmtp, portName);
            protoTaskp->addStmtsp(newStmtp);
            stmtp = nextp;
        }
        // Mark prototype as no longer prototype (it has a body now)
        protoTaskp->prototype(خطأ);
        // Delete the out-of-block task from the module
        nodep->unlinkFrBack();
        VL_DO_DANGLING(pushDeletep(nodep), nodep);
    }

    // Rewrite AstDot nodes where lhsp is AstParseRef with the port name
    // to just the rhsp (stripping the port prefix).
    ساكن فراغ rewriteIfacePortRefsSingle(AstNode* nodep, ثابت string& portName) {
        𐤀𐤌 (!nodep) أرجع;
        // Check if this is a non-colon Dot with lhs matching portName
        𐤀𐤌 (AstDot* ثابت dotp = VN_CAST(nodep, Dot)) {
            𐤀𐤌 (!dotp->colon()) {
                ثابت AstParseRef* ثابت lhsRefp = VN_CAST(dotp->lhsp(), ParseRef);
                𐤀𐤌 (lhsRefp && lhsRefp->name() == portName) {
                    UINFO(5, "   rewriteIfacePortRefs: stripping port prefix from " << dotp);
                    AstNode* ثابت rhsp = dotp->rhsp()->unlinkFrBack();
                    dotp->replaceWith(rhsp);
                    VL_DO_DANGLING(dotp->deleteTree(), dotp);
                    rewriteIfacePortRefsSingle(rhsp, portName);
                    أرجع;
                }
            }
        }
        // Recurse into all children
        لكل (AstNode* childp : {nodep->op1p(), nodep->op2p(), nodep->op3p(), nodep->op4p()}) {
            طالما (childp) {
                AstNode* ثابت nextp = childp->nextp();
                rewriteIfacePortRefsSingle(childp, portName);
                childp = nextp;
            }
        }
    }

    فراغ visit(AstNodeFTask* nodep) override {  // FindVisitor::
        // NodeTask: Remember its name for later resolution
        UINFO(5, "   " << nodep);
        UASSERT_OBJ(m_curSymp && m_modSymp, nodep, "Function/Task not under module?");
        𐤀𐤌 (nodep->name() == "new") m_explicitNew = true;
        // Handle out-of-block interface method definition (IEEE 25.8)
        𐤀𐤌 (!nodep->ifacePortName().empty() && m_statep->forPrimary()) {
            moveIfaceExportBody(nodep);
            أرجع;  // Task has been deleted
        }
        // Remember the existing symbol table scope
        VL_RESTORER(m_classOrPackagep);
        VL_RESTORER(m_curSymp);
        VSymEnt* upSymp = m_curSymp;

        𐤀𐤌 (VN_IS(m_curSymp->nodep(), Class)) {
            𐤀𐤌 (VN_AS(m_curSymp->nodep(), Class)->isInterfaceClass() && !nodep->pureVirtual()
                && !nodep->isConstructor()) {
                nodep->v3error("Interface class functions must be pure virtual"
                               << " (IEEE 1800-2023 8.26): " << nodep->prettyNameQ());
            }
            𐤀𐤌 (m_statep->forPrimary()
                && (nodep->name() == "randomize" || nodep->name() == "srandom")) {
                nodep->v3error(nodep->prettyNameQ()
                               << " is a predefined class method; redefinition not allowed"
                                  " (IEEE 1800-2023 18.6.3)");
            }
        }
        // Change to appropriate package if extern declaration (vs definition)
        𐤀𐤌 (nodep->classOrPackagep()) {
            // class-in-class
            AstDot* ثابت dotp = VN_CAST(nodep->classOrPackagep(), Dot);
            AstClassOrPackageRef* ثابت cpackagerefp
                = VN_CAST(nodep->classOrPackagep(), ClassOrPackageRef);
            𐤀𐤌 (dotp) {
                AstClassOrPackageRef* ثابت lhsp = VN_AS(dotp->lhsp(), ClassOrPackageRef);
                m_statep->resolveClassOrPackage(m_curSymp, lhsp, true, خطأ,
                                                "External definition :: reference");
                AstClass* ثابت lhsclassp = VN_CAST(lhsp->classOrPackageSkipp(), Class);
                𐤀𐤌 (!lhsclassp) {
                    nodep->v3error("Extern declaration's scope is not a defined class");
                } وإلا {
                    m_curSymp = m_statep->getNodeSym(lhsclassp);
                    upSymp = m_curSymp;
                    AstClassOrPackageRef* ثابت rhsp = VN_AS(dotp->rhsp(), ClassOrPackageRef);
                    m_statep->resolveClassOrPackage(m_curSymp, rhsp, true, خطأ,
                                                    "External definition :: reference");
                    AstClass* ثابت rhsclassp = VN_CAST(rhsp->classOrPackageSkipp(), Class);
                    𐤀𐤌 (!rhsclassp) {
                        nodep->v3error("Extern declaration's scope is not a defined class");
                    } وإلا {
                        m_curSymp = m_statep->getNodeSym(rhsclassp);
                        upSymp = m_curSymp;
                        moveExternFuncDecl(nodep, rhsclassp);
                    }
                }
            } وإلا 𐤀𐤌 (cpackagerefp) {
                𐤀𐤌 (!cpackagerefp->classOrPackageSkipp()) {
                    m_statep->resolveClassOrPackage(m_curSymp, cpackagerefp, true, خطأ,
                                                    "External definition :: reference");
                }
                AstClass* ثابت classp = VN_CAST(cpackagerefp->classOrPackageSkipp(), Class);
                𐤀𐤌 (!classp) {
                    nodep->v3error("Extern declaration's scope is not a defined class");
                } وإلا {
                    m_curSymp = m_statep->getNodeSym(classp);
                    upSymp = m_curSymp;
                    moveExternFuncDecl(nodep, classp);
                }
            } وإلا {
                v3fatalSrc("Unhandled extern function definition package");
            }
        }
        // Set the class as package for iteration
        𐤀𐤌 (VN_IS(m_curSymp->nodep(), Class)) {
            m_classOrPackagep = VN_AS(m_curSymp->nodep(), Class);
        }
        // Create symbol table for the task's vars
        ثابت string name = (nodep->isExternProto() ? "extern "s : ""s) + nodep->name();
        m_curSymp = m_statep->insertBlock(m_curSymp, name, nodep, m_classOrPackagep);
        m_curSymp->fallbackp(upSymp);
        // Convert the func's range to the output variable
        // This should probably be done in the Parser instead, as then we could
        // just attach normal signal attributes to it.
        𐤀𐤌 (!nodep->isExternProto() && nodep->fvarp() && !VN_IS(nodep->fvarp(), Var)) {
            AstNodeDType* dtypep = VN_CAST(nodep->fvarp(), NodeDType);
            // If unspecified, function returns one bit; however when we
            // support NEW() it could also return the class reference.
            𐤀𐤌 (dtypep) {
                dtypep->unlinkFrBack();
            } وإلا {
                dtypep = جديد AstBasicDType{nodep->fileline(), VBasicDTypeKwd::LOGIC};
            }
            AstVar* ثابت newvarp
                = جديد AstVar{nodep->fileline(), VVarType::VAR, nodep->name(), VFlagChildDType{},
                             dtypep};  // Not dtype resolved yet
            newvarp->direction(VDirection::OUTPUT);
            newvarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            newvarp->funcReturn(true);
            newvarp->trace(خطأ);  // Not user visible
            nodep->fvarp(newvarp);
            // Explicit insert required, as the var name shadows the upper level's task name
            m_statep->insertSym(m_curSymp, newvarp->name(), newvarp, nullptr /*classOrPackagep*/);
        }
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    فراغ visit(AstClocking* nodep) override {  // FindVisitor::
        VL_RESTORER(m_clockingp);
        m_clockingp = nodep;
        iterate(nodep->sentreep());
        iterateAndNextNull(nodep->itemsp());
        // If the block has no name, one cannot reference the clockvars
        VSymEnt* itSymp = nullptr;
        𐤀𐤌 (nodep->isGlobal()  //
            && m_statep->forPrimary()) {  // else flattening may see two globals
            m_statep->checkDuplicate(m_curSymp, nodep, "__024global_clock");
            itSymp
                = m_statep->insertBlock(m_curSymp, "__024global_clock", nodep, m_classOrPackagep);
            itSymp->fallbackp(nullptr);
        }
        𐤀𐤌 (!nodep->name().empty()) {
            itSymp = m_statep->insertBlock(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
            itSymp->fallbackp(nullptr);
        }
        𐤀𐤌 (itSymp) {
            VL_RESTORER(m_curSymp);
            m_curSymp = itSymp;
            iterateAndNextNull(nodep->itemsp());
        }
    }
    فراغ visit(AstClockingItem* nodep) override {  // FindVisitor::
        𐤀𐤌 (nodep->varp()) {
            𐤀𐤌 (m_curSymp->nodep() == m_clockingp) iterate(nodep->varp());
            أرجع;
        }
        std::string varname;
        AstNodeDType* dtypep;
        𐤀𐤌 (AstAssign* ثابت assignp = nodep->assignp()) {
            AstNodeExpr* ثابت rhsp = assignp->rhsp()->unlinkFrBack();
            dtypep = جديد AstRefDType{nodep->fileline(), AstRefDType::FlagTypeOfExpr{},
                                     rhsp->cloneTree(خطأ)};
            nodep->exprp(rhsp);
            varname = assignp->lhsp()->name();
            VL_DO_DANGLING(assignp->unlinkFrBack()->deleteTree(), assignp);
        } وإلا {
            AstNodeExpr* ثابت refp = nodep->exprp();
            ثابت VSymEnt* foundp = m_curSymp->findIdFallback(refp->name());
            𐤀𐤌 (!foundp || !foundp->nodep()) {
                refp->v3error("Corresponding variable " << refp->prettyNameQ()
                                                        << " does not exist");
                VL_DO_DANGLING(nodep->unlinkFrBack()->deleteTree(), nodep);
                أرجع;
            }
            varname = refp->name();
            dtypep = VN_AS(foundp->nodep(), Var)->childDTypep()->cloneTree(خطأ);
        }
        AstVar* ثابت newvarp = جديد AstVar{nodep->fileline(), VVarType::MODULETEMP, varname,
                                           VFlagChildDType{}, dtypep};
        newvarp->lifetime(VLifetime::STATIC_EXPLICIT);
        nodep->varp(newvarp);
        iterate(nodep->exprp());
    }
    فراغ visit(AstConstraint* nodep) override {  // FindVisitor::
        VL_RESTORER(m_curSymp);
        // Change to appropriate package if extern declaration (vs definition)
        VSymEnt* upSymp = m_curSymp;
        𐤀𐤌 (nodep->classOrPackagep()) {
            AstClassOrPackageRef* ثابت cpackagerefp
                = VN_CAST(nodep->classOrPackagep(), ClassOrPackageRef);
            𐤀𐤌 (!cpackagerefp) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: extern constraint definition with class-in-class");
            } وإلا {
                𐤀𐤌 (!cpackagerefp->classOrPackageSkipp()) {
                    m_statep->resolveClassOrPackage(m_curSymp, cpackagerefp, true, خطأ,
                                                    "External definition :: reference");
                }
                AstClass* ثابت classp = VN_CAST(cpackagerefp->classOrPackageSkipp(), Class);
                𐤀𐤌 (!classp) {
                    nodep->v3error("Extern declaration's scope is not a defined class");
                } وإلا {
                    m_curSymp = m_statep->getNodeSym(classp);
                    upSymp = m_curSymp;
                    // Move it to proper spot under the target class
                    nodep->unlinkFrBack();
                    classp->addStmtsp(nodep);
                    nodep->classOrPackagep()->unlinkFrBack()->deleteTree();
                }
            }
        }
        // Set the class as package for iteration
        ثابت string name = (nodep->isExternProto() ? "extern "s : ""s) + nodep->name();
        m_curSymp = m_statep->insertBlock(upSymp, name, nodep, m_classOrPackagep);
        iterateChildren(nodep);
    }
    فراغ visit(AstVar* nodep) override {  // FindVisitor::
        // Var: Remember its name for later resolution
        UASSERT_OBJ(m_curSymp && m_modSymp, nodep, "Var not under module?");
        iterateChildren(nodep);
        𐤀𐤌 (VN_IS(m_curSymp->nodep(), Class)
            && VN_AS(m_curSymp->nodep(), Class)->isInterfaceClass() && !nodep->isParam()) {
            nodep->v3error("Interface class cannot contain non-parameter members"
                           << " (IEEE 1800-2023 8.26): " << nodep->prettyNameQ());
        }
        𐤀𐤌 (!m_statep->forScopeCreation()) {
            // Find under either a task or the module's vars
            ثابت VSymEnt* foundp = m_curSymp->findIdFallback(nodep->name());
            𐤀𐤌 (!foundp && m_modSymp && nodep->name() == m_modSymp->nodep()->name()) {
                foundp = m_modSymp;  // Conflicts with modname?
            }
            AstVar* ثابت findvarp = foundp ? VN_CAST(foundp->nodep(), Var) : nullptr;
            // clocking items can have duplicate names (inout)
            𐤀𐤌 (findvarp && VN_IS(findvarp->backp(), ClockingItem)
                && VN_IS(nodep->backp(), ClockingItem)) {
                AstClockingItem* ثابت itemp = VN_AS(nodep->backp(), ClockingItem);
                AstClockingItem* ثابت finditemp = VN_AS(findvarp->backp(), ClockingItem);
                UINFO(4, "ClockCompl: " << itemp << " ;; " << finditemp);
                UINFO(4, "ClockCompV: " << nodep << " ;; " << findvarp);
                𐤀𐤌 (*itemp->exprp()->fileline() == *finditemp->exprp()->fileline()) {
                    UASSERT_OBJ(finditemp->direction() == VDirection::INPUT
                                    && itemp->direction() == VDirection::OUTPUT,
                                itemp, "Input after output?");
                    // pretend nothing found and rename
                    foundp = nullptr;
                    nodep->name("__Voutput_" + nodep->name());
                    finditemp->outputp(itemp);
                }
            }
            منطقي ins = خطأ;
            𐤀𐤌 (!foundp) {
                ins = true;
            } وإلا 𐤀𐤌 (!findvarp && m_curSymp->findIdFlat(nodep->name()) && !foundp->imported()) {
                nodep->v3error("Unsupported in C: Variable has same name as "
                               << LinkDotState::nodeTextType(foundp->nodep()) << ": "
                               << nodep->prettyNameQ());
            } وإلا 𐤀𐤌 (findvarp != nodep) {
                UINFO(4, "DupVar: " << nodep << " ;; " << foundp->nodep());
                UINFO(4, "    found  cur=se" << cvtToHex(m_curSymp) << " ;; parent=se"
                                             << cvtToHex(foundp->parentp()));
                𐤀𐤌 (foundp->parentp() == m_curSymp  // Only when on same level
                    && !foundp->imported()) {  // and not from package
                    𐤀𐤌 (VN_IS(m_curSymp->nodep(), Clocking)) {
                        nodep->v3error("Multiple clockvars with the same name not allowed");
                        أرجع;
                    }
                    ثابت منطقي nansiBad
                        = (((findvarp->isDeclTyped() || findvarp->isNet())
                            && (nodep->isDeclTyped() || nodep->isNet()))
                           || (findvarp->isIO() && nodep->isIO())  // e.g. !(output && output)
                           || findvarp->gotNansiType());  // e.g. int x && int x
                    ثابت منطقي ansiBad
                        = findvarp->isAnsi() || nodep->isAnsi();  // dup illegal with ANSI
                    𐤀𐤌 (ansiBad || nansiBad) {
                        منطقي ansiWarn = ansiBad && !nansiBad;
                        𐤀𐤌 (ansiWarn) {
                            ساكن صحيح s_didAnsiWarn = خطأ;
                            𐤀𐤌 (s_didAnsiWarn++) ansiWarn = خطأ;
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
                    } وإلا {
                        findvarp->combineType(nodep);
                        findvarp->fileline()->modifyStateInherit(nodep->fileline());
                        findvarp->gotNansiType(true);
                        UASSERT_OBJ(nodep->subDTypep(), nodep, "Var has no type");
                        𐤀𐤌 (nodep->subDTypep()->numeric().isSigned()
                            && !findvarp->subDTypep()->numeric().isSigned()) {
                            findvarp->subDTypep()->numeric(VSigning{true});
                        }
                        AstNodeDType* ثابت varDtp = findvarp->subDTypep();
                        AstNodeDType* ثابت otherDtp = nodep->subDTypep();
                        𐤀𐤌 (varDtp
                            && (VN_IS(varDtp, BasicDType)
                                && VN_AS(varDtp, BasicDType)->implicit())) {
                            // Then have "input foo" and "real foo" so the
                            // dtype comes from the other side.
                            VL_DO_DANGLING(varDtp->unlinkFrBack()->deleteTree(), varDtp);
                            findvarp->childDTypep(otherDtp->unlinkFrBack());
                        } وإلا 𐤀𐤌 (!m_statep->forScopeCreation() && otherDtp && varDtp
                                   && !(VN_IS(otherDtp, BasicDType)
                                        && VN_AS(otherDtp, BasicDType)->implicit())) {
                            // otherDtp and varDtp both non-nullptr and neither are implicit
                            // Can't compare dtypes now as might contain parameters,
                            // defer to V3Width
                            AstAttrOf* ثابت newp
                                = جديد AstAttrOf{otherDtp->fileline(), VAttrType::VAR_PORT_DTYPE,
                                                otherDtp->unlinkFrBack()};
                            findvarp->addAttrsp(newp);
                        }
                    }
                    VL_DO_DANGLING(nodep->unlinkFrBack()->deleteTree(), nodep);
                } وإلا {
                    // User can disable the message at either point
                    𐤀𐤌 (!(m_ftaskp && m_ftaskp->dpiImport())
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
                    ins = true;
                }
            }
            𐤀𐤌 (ins) {
                𐤀𐤌 (m_statep->forPrimary() && nodep->isGParam()
                    && VN_IS(m_modSymp->nodep(), Module)
                    && (m_statep->rootEntp()->nodep() == m_modSymp->parentp()->nodep())) {
                    // This is the toplevel module. Check for command line overwrites of parameters
                    // We first search if the parameter is overwritten and then replace it with a
                    // new value.
                    𐤀𐤌 (v3Global.opt.hasParameter(nodep->name())) {
                        ثابت string svalue = v3Global.opt.parameter(nodep->name());
                        𐤀𐤌 (AstConst* ثابت valuep
                            = AstConst::parseParamLiteral(nodep->fileline(), svalue)) {
                            UINFO(9, "       replace parameter " << nodep);
                            UINFO(9, "       with " << valuep);
                            𐤀𐤌 (nodep->valuep()) pushDeletep(nodep->valuep()->unlinkFrBack());
                            nodep->valuep(valuep);
                            // Do not warn ENUMVALUE when overriding an enum param from the cli.
                            nodep->fileline()->warnOff(V3ErrorCode::ENUMVALUE, true);
                        }
                    }
                }
                m_statep->insertSym(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
                𐤀𐤌 (m_statep->forPrimary() && nodep->isGParam()) {
                    ++m_paramNum;
                    VSymEnt* ثابت symp
                        = m_statep->insertSym(m_curSymp, "__paramNumber" + cvtToStr(m_paramNum),
                                              nodep, m_classOrPackagep);
                    symp->exported(خطأ);
                }
            }
        }
    }
    فراغ visit(AstTypedef* nodep) override {  // FindVisitor::
        UASSERT_OBJ(m_curSymp, nodep, "Typedef not under module/package/$unit");
        𐤀𐤌 (VN_IS(m_classOrPackagep, Class)) nodep->isUnderClass(true);
        iterateChildren(nodep);
        m_statep->insertSym(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
    }
    فراغ visit(AstTypedefFwd* nodep) override {  // FindVisitor::
        UASSERT_OBJ(m_curSymp, nodep, "Typedef not under module/package/$unit");
        iterateChildren(nodep);
        // No need to insert, only the real typedef matters, but need to track for errors
        nodep->user1p(m_curSymp);
    }
    فراغ visit(AstNodeUOrStructDType* nodep) override {  // FindVisitor::
        UASSERT_OBJ(m_curSymp, nodep, "Struct/union dtype not under module/package/$unit");
        VL_RESTORER(m_curSymp);
        m_curSymp = m_statep->insertBlock(m_curSymp, "__Vdtype" + cvtToStr(nodep->uniqueNum()),
                                          nodep, m_classOrPackagep);
        iterateChildren(nodep);
    }
    فراغ visit(AstMemberDType* nodep) override {  // FindVisitor::
        iterateChildren(nodep);
        m_statep->insertSym(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
    }
    فراغ visit(AstParamTypeDType* nodep) override {  // FindVisitor::
        UASSERT_OBJ(m_curSymp, nodep, "Parameter type not under module/package/$unit");

        // Replace missing param types with provided hierarchical type params.
        𐤀𐤌 (!m_hierParamsName.empty()) {
            𐤀𐤌 (ثابت VSymEnt* ثابت typedefEntp = m_curSymp->findIdFallback(m_hierParamsName)) {
                ثابت AstModule* modp = VN_CAST(typedefEntp->nodep(), Module);

                لكل (ثابت AstNode* stmtp = modp ? modp->stmtsp() : nullptr; stmtp;
                     stmtp = stmtp->nextp()) {
                    ثابت AstTypedef* tdefp = VN_CAST(stmtp, Typedef);

                    𐤀𐤌 (tdefp && tdefp->name() == nodep->name() && m_statep->forPrimary()) {
                        UINFO(8, "Replacing type of" << nodep << "  -->with " << tdefp);
                        AstNodeDType* ثابت newDtp = tdefp->childDTypep();
                        AstNodeDType* ثابت oldDtp = nodep->childDTypep();

                        oldDtp->replaceWith(newDtp->cloneTree(خطأ));
                        VL_DO_DANGLING(oldDtp->deleteTree(), oldDtp);
                    }
                }
            }
        }

        iterateChildren(nodep);
        m_statep->insertSym(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
        𐤀𐤌 (m_statep->forPrimary() && nodep->isGParam()) {
            ++m_paramNum;
            VSymEnt* ثابت symp = m_statep->insertSym(
                m_curSymp, "__paramNumber" + cvtToStr(m_paramNum), nodep, m_classOrPackagep);
            symp->exported(خطأ);
        }
    }
    فراغ visit(AstCFunc* nodep) override {  // FindVisitor::
        // For dotted resolution, ignore all AstVars under functions, otherwise shouldn't exist
        UASSERT_OBJ(!m_statep->forScopeCreation(), nodep, "No CFuncs expected in tree yet");
    }
    فراغ visit(AstEnumItem* nodep) override {  // FindVisitor::
        // EnumItem: Remember its name for later resolution
        iterateChildren(nodep);
        // Find under either a task or the module's vars
        ثابت VSymEnt* foundp = m_curSymp->findIdFallback(nodep->name());
        𐤀𐤌 (!foundp && m_modSymp && nodep->name() == m_modSymp->nodep()->name()) {
            foundp = m_modSymp;  // Conflicts with modname?
        }
        AstEnumItem* ثابت findvarp = foundp ? VN_CAST(foundp->nodep(), EnumItem) : nullptr;
        منطقي ins = خطأ;
        𐤀𐤌 (!foundp) {
            ins = true;
        } وإلا 𐤀𐤌 (findvarp != nodep) {
            UINFO(4, "DupVar: " << nodep << " ;; " << foundp);
            𐤀𐤌 (foundp->parentp() == m_curSymp  // Only when on same level
                && !foundp->imported()) {  // and not from package
                nodep->v3error("Duplicate declaration of enum value: "
                               << nodep->prettyName() << '\n'
                               << nodep->warnContextPrimary() << '\n'
                               << foundp->nodep()->warnOther()
                               << "... Location of original declaration\n"
                               << foundp->nodep()->warnContextSecondary());
            } وإلا {
                // User can disable the message at either point
                𐤀𐤌 (!nodep->fileline()->warnIsOff(V3ErrorCode::VARHIDDEN)
                    && !foundp->nodep()->fileline()->warnIsOff(V3ErrorCode::VARHIDDEN)) {
                    nodep->v3warn(VARHIDDEN,
                                  "Declaration of enum value hides declaration in upper scope: "
                                      << nodep->prettyName() << '\n'
                                      << nodep->warnContextPrimary() << '\n'
                                      << foundp->nodep()->warnOther()
                                      << "... Location of original declaration\n"
                                      << foundp->nodep()->warnContextSecondary());
                }
                ins = true;
            }
        }
        𐤀𐤌 (ins) m_statep->insertSym(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
    }
    فراغ visit(AstPackageImport* nodep) override {  // FindVisitor::
        UINFO(4, "  Link: " << nodep);
        𐤀𐤌 (!nodep->packagep()) أرجع;  // Errored in V3LinkCells
        VSymEnt* ثابت srcp = m_statep->getNodeSym(nodep->packagep());
        𐤀𐤌 (nodep->name() == "*") {
            𐤀𐤌 (nodep->packagep() != v3Global.rootp()->stdPackagep()) {
                𐤀𐤌 (m_curSymp == m_statep->dunitEntp()) {
                    nodep->v3warn(IMPORTSTAR,
                                  "'import::*' in $unit scope may pollute global namespace");
                }
            }
        } وإلا 𐤀𐤌 (!nodep->resolvedClassp()) {
            VSymEnt* ثابت impp = srcp->findIdFlat(nodep->name());
            𐤀𐤌 (!impp) {
                nodep->v3error("Import object not found: " << nodep->prettyPkgNameQ());
            } وإلا 𐤀𐤌 (nodep->resolvedClassp()) {
                UASSERT_OBJ(nodep->resolvedClassp()->classOrPackageSkipp() == impp->nodep(), nodep,
                            "Import should resolve to the same node");
            } وإلا 𐤀𐤌 (AstClass* ثابت classp = VN_CAST(impp->nodep(), Class)) {
                nodep->resolvedClassp(
                    جديد AstClassOrPackageRef{nodep->fileline(), classp->name(), classp, nullptr});
            }
        }
        m_curSymp->importFromPackage(m_statep->symsp(), srcp, nodep->name());
        UINFO(9, "    Link Done: " << nodep);
        // No longer needed, but can't delete until any multi-instantiated modules are expanded
    }
    فراغ visit(AstPackageExport* nodep) override {  // FindVisitor::
        UINFO(9, "  Link: " << nodep);
        𐤀𐤌 (!nodep->packagep()) أرجع;  // Errored in V3LinkCells
        VSymEnt* ثابت srcp = m_statep->getNodeSym(nodep->packagep());
        𐤀𐤌 (nodep->name() != "*") {
            VSymEnt* ثابت impp = srcp->findIdFlat(nodep->name());
            𐤀𐤌 (!impp) {
                nodep->v3error("Export object not found: '" << nodep->packagep()->prettyName()
                                                            << "::" << nodep->prettyName() << "'");
            }
        }
        m_curSymp->exportFromPackage(m_statep->symsp(), srcp, nodep->name());
        UINFO(9, "    Link Done: " << nodep);
        // No longer needed, but can't delete until any multi-instantiated modules are expanded
    }
    فراغ visit(AstPackageExportStarStar* nodep) override {  // FindVisitor::
        UINFO(4, "  Link: " << nodep);
        m_curSymp->exportStarStar(m_statep->symsp());
        // No longer needed, but can't delete until any multi-instantiated modules are expanded
    }

    فراغ visit(AstNodeForeach* nodep) override {  // FindVisitor::
        // Symbol table needs nodep->name() as the index variable's name
        VL_RESTORER(m_curSymp);
        {
            ++m_modWithNum;
            m_curSymp = m_statep->insertBlock(m_curSymp, "__Vforeach" + cvtToStr(m_modWithNum),
                                              nodep, m_classOrPackagep);
            m_curSymp->fallbackp(VL_RESTORER_PREV(m_curSymp));
            لكل (AstNode *nextp, *argp = nodep->headerp()->elementsp(); argp; argp = nextp) {
                nextp = argp->nextp();
                AstVar* argrefp = nullptr;
                𐤀𐤌 (AstParseRef* ثابت parserefp = VN_CAST(argp, ParseRef)) {
                    // IEEE 1800-2023 12.7.3: foreach loop variable type shall be int (2-state)
                    // This might get changed in V3Width when types resolve (e.g., for assoc
                    // arrays)
                    argrefp = جديد AstVar{parserefp->fileline(), VVarType::BLOCKTEMP,
                                         parserefp->name(), argp->findIntDType()};
                    argrefp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                    parserefp->replaceWith(argrefp);
                    VL_DO_DANGLING2(parserefp->deleteTree(), parserefp, argp);
                    // Insert argref's name into symbol table
                    m_statep->insertSym(m_curSymp, argrefp->name(), argrefp, nullptr);
                } وإلا 𐤀𐤌 (ثابت auto largrefp = VN_CAST(argp, Var)) {
                    argrefp = largrefp;
                    // Insert argref's name into symbol table
                    m_statep->insertSym(m_curSymp, argrefp->name(), argrefp, nullptr);
                } وإلا 𐤀𐤌 (VN_IS(argp, Empty)) {
                } وإلا {
                    argp->v3error("'foreach' loop variable expects simple variable name");
                }
            }
            iterateChildren(nodep);
        }
    }

    فراغ visit(AstRandSequence* nodep) override {
        VL_RESTORER(m_curSymp);
        m_curSymp = m_statep->insertBlock(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
        iterateChildren(nodep);
    }
    فراغ visit(AstRSProd* nodep) override {
        VL_RESTORER(m_curSymp);
        m_curSymp = m_statep->insertBlock(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
        𐤀𐤌 (nodep->fvarp())
            nodep->fvarp()->v3warn(E_UNSUPPORTED,
                                   "Unsupported: randsequence production function variable");
        // Mark formal ports as port-checked so the primary resolve pass does not
        // flag them with "does not appear in port list" -- V3RandSequence will
        // later move them onto the generated task as real input ports.
        لكل (AstNode* itp = nodep->portsp(); itp; itp = itp->nextp()) {
            VN_AS(itp, Var)->user4(true);
        }
        iterateChildren(nodep);
    }

    فراغ visit(AstWithParse* nodep) override {  // FindVisitor::
        // Change WITHPARSE(FUNCREF, equation) to FUNCREF(WITH(equation))
        AstNodeFTaskRef* funcrefp = VN_CAST(nodep->funcrefp(), NodeFTaskRef);
        𐤀𐤌 (ثابت AstDot* ثابت dotp = VN_CAST(nodep->funcrefp(), Dot))
            funcrefp = VN_CAST(dotp->rhsp(), NodeFTaskRef);
        UASSERT_OBJ(funcrefp, nodep, "'with' only can operate on a function/task");
        string name = funcrefp->name() == "randomize" ? "__Vrandwith_obj" : "item";
        FileLine* argFl = nodep->fileline();
        AstArg* ثابت argsp = funcrefp->argsp();
        𐤀𐤌 (argsp) {
            argsp->unlinkFrBackWithNext();
            𐤀𐤌 (funcrefp->name() != "randomize") {
                𐤀𐤌 (ثابت auto parserefp = VN_CAST(argsp->exprp(), ParseRef)) {
                    name = parserefp->name();
                    argFl = parserefp->fileline();
                } وإلا {
                    argsp->v3error("'with' function expects simple variable name");
                }
                𐤀𐤌 (argsp->nextp()) {
                    argsp->nextp()->v3error("'with' function expects only up to one argument");
                }
                VL_DO_DANGLING(argsp->deleteTree(), argsp);
            }
        }
        // Type depends on the method used, let V3Width figure it out later
        // 'with (...) { ... }' constraint_block form is randomize-only
        // (IEEE 1800-2023 7.12 vs. 18.7); array methods take 'with (expr)'.
        𐤀𐤌 (nodep->restricted() && funcrefp->name() != "randomize") {
            nodep->v3error("'with (...) { ... }' constraint block is only valid for"
                           " randomize() (IEEE 1800-2023 18.7)");
            funcrefp->addArgsp(argsp);
            nodep->replaceWith(nodep->funcrefp()->unlinkFrBack());
            VL_DO_DANGLING(nodep->deleteTree(), nodep);
            أرجع;
        }
        ثابت منطقي restrictedRandomize = nodep->restricted();
        𐤀𐤌 (nodep->exprsp() || nodep->constraintsp()
            || restrictedRandomize) {  // Else empty expression and pretend no "with"
            AstLambdaArgRef* ثابت indexArgRefp
                = جديد AstLambdaArgRef{argFl, name + "__DOT__index", true};
            AstLambdaArgRef* ثابت valueArgRefp = جديد AstLambdaArgRef{argFl, name, خطأ};
            AstWith* ثابت newp
                = جديد AstWith{nodep->fileline(), indexArgRefp, valueArgRefp, nullptr};
            // Harvest the identifier_list into AstWith; grammar guarantees AstParseRef.
            𐤀𐤌 (restrictedRandomize) {
                newp->restricted(true);
                طالما (AstNode* ثابت itemp = nodep->exprsp()) {
                    newp->addRestrictedName(VN_AS(itemp, ParseRef)->name());
                    itemp->unlinkFrBack();
                    VL_DO_DANGLING(pushDeletep(itemp), itemp);
                }
            }
            AstNode* exprOrConstraintsp = nullptr;
            𐤀𐤌 (nodep->exprsp()) exprOrConstraintsp = nodep->exprsp()->unlinkFrBackWithNext();
            𐤀𐤌 (nodep->constraintsp())
                exprOrConstraintsp = AstNode::addNext(
                    exprOrConstraintsp, nodep->constraintsp()->unlinkFrBackWithNext());
            newp->addExprp(exprOrConstraintsp);  // addExprp() tolerates nullptr
            funcrefp->withp(newp);
        }
        funcrefp->addArgsp(argsp);
        nodep->replaceWith(nodep->funcrefp()->unlinkFrBack());
        VL_DO_DANGLING(nodep->deleteTree(), nodep);
    }
    فراغ visit(AstWith* nodep) override {  // FindVisitor::
        // Symbol table needs nodep->name() as the index variable's name
        // Iteration will pickup the AstVar we made under AstWith
        VL_RESTORER(m_curSymp);
        ++m_modWithNum;
        m_curSymp = m_statep->insertBlock(m_curSymp, "__Vwith" + cvtToStr(m_modWithNum), nodep,
                                          m_classOrPackagep);
        m_curSymp->fallbackp(VL_RESTORER_PREV(m_curSymp));
        UASSERT_OBJ(nodep->indexArgRefp(), nodep, "Missing lambda argref");
        UASSERT_OBJ(nodep->valueArgRefp(), nodep, "Missing lambda argref");
        // Insert argref's name into symbol table
        m_statep->insertSym(m_curSymp, nodep->valueArgRefp()->name(), nodep->valueArgRefp(),
                            nullptr);
        iterateChildren(nodep);
    }
    فراغ visit(AstCoverWith* nodep) override {  // FindVisitor::
        // The range list is outside the scope of the filter, which declares 'item'
        iterateAndNextNull(nodep->subp());
        VL_RESTORER(m_curSymp);
        ++m_modWithNum;
        m_curSymp = m_statep->insertBlock(m_curSymp, "__VcoverWith" + cvtToStr(m_modWithNum),
                                          nodep, m_classOrPackagep);
        m_curSymp->fallbackp(VL_RESTORER_PREV(m_curSymp));
        iterateAndNextNull(nodep->itemp());
        iterateAndNextNull(nodep->filterp());
    }
    فراغ visit(AstCoverCrossWith* nodep) override {  // FindVisitor::
        // The selection, the 'matches' count, and the cross items' types, of their coverpoints'
        // expressions with any nested scopes, are outside the filter's scope, which declares the
        // items.  As visit(AstVar) would resolve a type in the scope declaring its variable, the
        // items, generated and unique, are inserted directly.
        iterateAndNextNull(nodep->selectp());
        iterateAndNextNull(nodep->countp());
        لكل (AstVar* varp = nodep->varsp(); varp; varp = VN_AS(varp->nextp(), Var)) {
            iterateAndNextNull(varp->childDTypep());
        }
        VL_RESTORER(m_curSymp);
        ++m_modWithNum;
        m_curSymp = m_statep->insertBlock(m_curSymp, "__VcoverWith" + cvtToStr(m_modWithNum),
                                          nodep, m_classOrPackagep);
        m_curSymp->fallbackp(VL_RESTORER_PREV(m_curSymp));
        لكل (AstVar* varp = nodep->varsp(); varp; varp = VN_AS(varp->nextp(), Var)) {
            m_statep->insertSym(m_curSymp, varp->name(), varp, m_classOrPackagep);
        }
        iterateAndNextNull(nodep->filterp());
    }

    فراغ visit(AstNode* nodep) override { iterateChildren(nodep); }  // FindVisitor::

    فراغ handleUnvisitedVirtIfaces() {
        AstNodeModule* ثابت top = v3Global.rootp()->modulesp();
        m_curSymp = m_statep->getNodeSym(top);
        لكل (AstIface* ifacep : m_virtIfaces) {
            𐤀𐤌 (!ifacep->user4()) {  // virtual interface never assigned any actual interface
                // Create dummy cell to keep the virtual interface alive
                // (later stages assume that non-dead interface has associated cell)
                AstCell* ثابت ifacecellp = جديد AstCell{ifacep->fileline(),
                                                        ifacep->fileline(),
                                                        "__VdummyInstOf" + ifacep->name(),
                                                        ifacep->name(),
                                                        nullptr,
                                                        nullptr,
                                                        nullptr};
                ifacecellp->modp(ifacep);
                top->addStmtsp(ifacecellp);

                iterate(ifacecellp);
            }
        }
    }

عام:
    // CONSTRUCTORS
    LinkDotFindVisitor(AstNetlist* rootp, LinkDotState* statep)
        : m_statep{statep} {
        UINFO(4, __FUNCTION__ << ": ");
        iterate(rootp);

        𐤀𐤌 (!m_virtIfaces.empty()) handleUnvisitedVirtIfaces();
        لكل (AstScope* ثابت scopep : m_scopes) scopep->modp()->addStmtsp(scopep);
        𐤀𐤌 (m_statep->forScopeCreation()) {
            V3Stats::addStat("LinkDot, Scope visits", m_statScopeVisits);
        }
    }
    ~LinkDotFindVisitor() override = افتراضي;
};

//======================================================================

صنف LinkDotFindIfaceVisitor final : عام VNVisitor {
    // NODE STATE
    //  *::user1p()             -> See LinkDotState

    // STATE - for current visit position (use VL_RESTORER)
    LinkDotState* ثابت m_statep;  // State to pass between visitors, including symbol table
    AstNode* m_declp = nullptr;  // Current declaring object that may soon contain IfaceRefDType

    // METHODS
    ثابت VSymEnt* findNonDeclSym(VSymEnt* symp, ثابت string& name) {
        // Find if there is a symbol of given name, ignoring the node that is declaring
        // it (m_declp) itself.  Thus if searching for "ifc" (an interface):
        //
        // module x;  // Finishes here and finds the typedef
        //   typedef foo ifc;
        //   ifc ifc;   // symp starts pointing here, but matches m_declp
        طالما (symp) {
            ثابت VSymEnt* ثابت foundp = symp->findIdFlat(name);
            𐤀𐤌 (foundp && foundp->nodep() != m_declp) أرجع foundp;
            symp = symp->fallbackp();
        }
        أرجع nullptr;
    }

    // VISITORS
    فراغ visit(AstRefDType* nodep) override {  // FindIfaceVisitor::
        𐤀𐤌 (m_statep->forPrimary() && !nodep->classOrPackagep()) {
            UINFO(9, "  FindIfc: " << nodep);
            // If under a var, ignore the var itself as might be e.g. "intf intf;"
            // Critical tests:
            //   t_interface_param_genblk.v  // Checks this does make interface
            //   t_interface_hidden.v  // Checks this doesn't making interface when hidden
            𐤀𐤌 (m_statep->existsNodeSym(nodep)) {
                VSymEnt* symp = m_statep->getNodeSym(nodep);
                ثابت VSymEnt* foundp = findNonDeclSym(symp, nodep->name());
                AstNode* foundNodep = nullptr;
                // This: v4make test_regress/t/t_interface_param_genblk.py --debug
                // --debugi-V3LinkDot 9 Passes with this commented out:
                𐤀𐤌 (foundp) foundNodep = foundp->nodep();
                𐤀𐤌 (!foundNodep) foundNodep = m_statep->findModuleSym(nodep->name());
                𐤀𐤌 (foundNodep) UINFO(9, "    Ifc foundNodep " << foundNodep);
                𐤀𐤌 (AstIface* ثابت defp = VN_CAST(foundNodep, Iface)) {
                    // Must be found as module name, and not hidden/ by normal symbol (foundp)
                    AstIfaceRefDType* ثابت newp
                        = جديد AstIfaceRefDType{nodep->fileline(), "", nodep->name()};
                    𐤀𐤌 (nodep->paramsp())
                        newp->addParamsp(nodep->paramsp()->unlinkFrBackWithNext());
                    newp->ifacep(defp);
                    newp->user1u(nodep->user1u());
                    UINFO(9, "    Resolved interface " << nodep << "  =>  " << defp);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(pushDeletep(nodep), nodep);
                    أرجع;
                }
            }
        }
        iterateChildren(nodep);
    }
    فراغ visit(AstVar* nodep) override {  // FindIfaceVisitor::
        VL_RESTORER(m_declp);
        m_declp = nodep;
        iterateChildren(nodep);
        AstIfaceRefDType* ثابت ifacerefp = LinkDotState::ifaceRefFromArray(nodep->subDTypep());
        𐤀𐤌 (ifacerefp && m_statep->existsNodeSym(nodep)) {
            // Can't resolve until interfaces and modport names are
            // known; see notes at top
            UINFO(9, "  FindIfc Var IfaceRef " << ifacerefp);
            𐤀𐤌 (!ifacerefp->isVirtual()) nodep->setIfaceRef();
            VSymEnt* ثابت symp = m_statep->getNodeSym(nodep);
            m_statep->insertIfaceVarSym(symp);
        }
    }
    فراغ visit(AstNode* nodep) override { iterateChildren(nodep); }  // FindIfaceVisitor::

عام:
    // CONSTRUCTORS
    LinkDotFindIfaceVisitor(AstNetlist* rootp, LinkDotState* statep)
        : m_statep{statep} {
        UINFO(4, __FUNCTION__ << ": ");
        iterate(rootp);
    }
    ~LinkDotFindIfaceVisitor() override = افتراضي;
};

//======================================================================

صنف LinkDotParamVisitor final : عام VNVisitor {
    // NODE STATE
    //  *::user1p()             -> See LinkDotState
    //  *::user2p()             -> See LinkDotState
    //  *::user4()              -> See LinkDotState

    // STATE
    LinkDotState* ثابت m_statep;  // State to pass between visitors, including symbol table
    AstNodeModule* m_modp = nullptr;  // Current module

    فراغ pinImplicitExprRecurse(AstNode* nodep) {
        // Under a pin, Check interconnect expression for a pin reference or a concat.
        // Create implicit variable as needed
        𐤀𐤌 (VN_IS(nodep, Dot)) {  // Not creating a simple implied type,
            // and implying something else would just confuse later errors
        } وإلا 𐤀𐤌 (VN_IS(nodep, VarRef) || VN_IS(nodep, ParseRef)) {
            // To prevent user errors, we should only do single bit
            // implicit vars, however some netlists (MIPS) expect single
            // bit implicit wires to get created with range 0:0 etc.
            m_statep->implicitOkAdd(m_modp, nodep->name());
        }
        // These are perhaps a little too generous, as a SELect of siga[sigb]
        // perhaps shouldn't create an implicit variable.  But, we've warned...
        وإلا {
            𐤀𐤌 (AstNode* ثابت refp = nodep->op1p()) pinImplicitExprRecurse(refp);
            𐤀𐤌 (AstNode* ثابت refp = nodep->op2p()) pinImplicitExprRecurse(refp);
            𐤀𐤌 (AstNode* ثابت refp = nodep->op3p()) pinImplicitExprRecurse(refp);
            𐤀𐤌 (AstNode* ثابت refp = nodep->op4p()) pinImplicitExprRecurse(refp);
        }
        // Continue along a list (e.g. the terminals under AstImplicit), also after a reference
        𐤀𐤌 (AstNode* ثابت refp = nodep->nextp()) pinImplicitExprRecurse(refp);
    }

    // VISITORS
    فراغ visit(AstTypeTable*) override {}  // ParamVisitor::
    فراغ visit(AstNodeModule* nodep) override {  // ParamVisitor::
        UINFO(5, "   " << nodep);
        𐤀𐤌 ((nodep->dead() || !nodep->user4()) && !nodep->hierParams()) {
            UINFO(4, "Mark dead module " << nodep);
            UASSERT_OBJ(!m_statep->forScopeCreation(), nodep,
                        "Dead module persisted past where should have removed");
            // Don't remove now, because we may have a tree of
            // parameterized modules with VARXREFs into the deleted module
            // region.  V3Dead should cleanup.
            // Downstream visitors up until V3Dead need to check for nodep->dead.
            nodep->dead(true);
            أرجع;
        }
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
    }
    فراغ visit(AstPin* nodep) override {  // ParamVisitor::
        // Pin: Link to submodule's port
        // Deal with implicit definitions - do before Resolve visitor as may
        // be referenced above declaration
        𐤀𐤌 (nodep->exprp()  // Else specifically unconnected pin
            && !nodep->svImplicit()) {  // SV 19.11.3: .name pins don't allow implicit decls
            pinImplicitExprRecurse(nodep->exprp());
        }
    }
    فراغ visit(AstDefParam* nodep) override {  // ParamVisitor::
        iterateChildren(nodep);
        nodep->v3warn(DEFPARAM, "defparam is deprecated (IEEE 1800-2023 C.4.1)\n"
                                    << nodep->warnMore()
                                    << "... Suggest use instantiation with #(."
                                    << nodep->prettyName() << "(...etc...))");
        منطقي hasPartSelect = خطأ;
        ثابت string path = extractDottedPath(nodep->pathp(), hasPartSelect);
        UASSERT_OBJ(!hasPartSelect && !path.empty(), nodep, "Unexpected defparam path shape");
        string baddot;
        VSymEnt* okSymp = nullptr;
        VSymEnt* ثابت foundp = m_statep->findDotted(
            nodep->fileline(), m_statep->getNodeSym(nodep), path, baddot, okSymp, true);
        AstCell* ثابت cellp = foundp ? VN_AS(foundp->nodep(), Cell) : nullptr;
        𐤀𐤌 (!cellp) {
            nodep->v3error("In defparam, instance " << path << " never declared");
        } وإلا {
            AstNodeExpr* ثابت exprp = nodep->rhsp()->unlinkFrBack();
            UINFO(9, "Defparam cell " << path << "." << nodep->name() << " attach-to " << cellp
                                      << "  <= " << exprp);
            // Don't need to check the name of the defparam exists.  V3Param does.
            AstPin* ثابت pinp = جديد AstPin{nodep->fileline(),
                                            -1,  // Pin# not relevant
                                            nodep->name(), exprp};
            pinp->param(true);
            pinp->paramPath(path);
            cellp->addParamsp(pinp);
        }
        VL_DO_DANGLING(nodep->unlinkFrBack()->deleteTree(), nodep);
    }
    فراغ visit(AstPort* nodep) override {  // ParamVisitor::
        // Port: Stash the pin number
        // Need to set pin numbers after varnames are created
        // But before we do the final resolution based on names
        VSymEnt* ثابت foundp = m_statep->getNodeSym(m_modp)->findIdFlat(nodep->name());
        AstVar* ثابت refp = foundp ? VN_CAST(foundp->nodep(), Var) : nullptr;
        𐤀𐤌 (!foundp) {
            nodep->v3error(
                "Input/output/inout declaration not found for port: " << nodep->prettyNameQ());
        } وإلا 𐤀𐤌 (!refp || (!refp->isIO() && !refp->isIfaceRef())) {
            nodep->v3error("Pin is not an in/out/inout/interface: " << nodep->prettyNameQ());
        } وإلا {
            𐤀𐤌 (refp->user4()) {
                nodep->v3error("Duplicate declaration of port: "
                               << nodep->prettyNameQ() << '\n'
                               << nodep->warnContextPrimary() << '\n'
                               << refp->warnOther() << "... Location of original declaration\n"
                               << refp->warnContextSecondary());
            }
            refp->user4(true);
            VSymEnt* ثابت symp = m_statep->insertSym(m_statep->getNodeSym(m_modp),
                                                      "__pinNumber" + cvtToStr(nodep->pinNum()),
                                                      refp, nullptr /*classOrPackagep*/);
            symp->exported(خطأ);
            refp->pinNum(nodep->pinNum());
            // Put the variable where the port is, so that variables stay
            // in pin number sorted order. Otherwise hierarchical or JSON
            // may botch by-position instances.
            nodep->addHereThisAsNext(refp->unlinkFrBack());
        }
        // Ports not needed any more
        VL_DO_DANGLING(nodep->unlinkFrBack()->deleteTree(), nodep);
    }
    فراغ visit(AstAssignW* nodep) override {  // ParamVisitor::
        // Deal with implicit definitions
        // We used to nodep->allowImplicit() here, but it turns out
        // normal "assigns" can also make implicit wires.  Yuk.
        pinImplicitExprRecurse(nodep->lhsp());
        iterateChildren(nodep);
    }

    فراغ visit(AstImplicit* nodep) override {  // ParamVisitor::
        // Unsupported gates need implicit creation
        pinImplicitExprRecurse(nodep->exprsp());
        // We're done with implicit gates
        VL_DO_DANGLING(nodep->unlinkFrBack()->deleteTree(), nodep);
    }
    فراغ visit(AstClassOrPackageRef* nodep) override {  // ParamVisitor::
        𐤀𐤌 (auto* ثابت fwdp = VN_CAST(nodep->classOrPackageNodep(), TypedefFwd)) {
            // Relink forward definitions to the "real" definition
            VSymEnt* ثابت foundp = m_statep->getNodeSym(fwdp)->findIdFallback(fwdp->name());
            𐤀𐤌 (foundp && (VN_IS(foundp->nodep(), Class) || VN_IS(foundp->nodep(), Package))) {
                nodep->classOrPackagep(VN_AS(foundp->nodep(), NodeModule));
            } وإلا 𐤀𐤌 (foundp && VN_IS(foundp->nodep(), ParamTypeDType)) {
                UASSERT(m_statep->forPrimary(), "Param types should have been resolved");
                nodep->classOrPackageNodep(VN_AS(foundp->nodep(), ParamTypeDType));
            } وإلا {
                𐤀𐤌 (foundp) UINFO(1, "found nodep = " << foundp->nodep());
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
    فراغ visit(AstPull* nodep) override {  // ParamVisitor::
        // Deal with implicit definitions
        // We used to nodep->allowImplicit() here, but it turns out
        // normal "assigns" can also make implicit wires.  Yuk.
        pinImplicitExprRecurse(nodep->lhsp());
        iterateChildren(nodep);
    }
    فراغ visit(AstTypedefFwd* nodep) override {  // ParamVisitor::
        VSymEnt* ثابت foundp = m_statep->getNodeSym(nodep)->findIdFallback(nodep->name());
        𐤀𐤌 (foundp) {
            // If the typedef was earlier in source order (tokenNum), then remember that earlier
            // point to avoid error something wasn't declared
            // Might be forward declaring something odd (with declTokenNumSetMin not implemented,
            // but should be harmless to ignore as this is just for error detection
            foundp->nodep()->declTokenNumSetMin(nodep->fileline()->tokenNum());
        } وإلا 𐤀𐤌 (v3Global.opt.pedantic()) {
            // We only check it was ever really defined in pedantic mode, as it
            // might have been in a header file referring to a module we never
            // needed so there are false positives
            nodep->v3error(
                "Forward typedef unused or does not resolve to a data type (IEEE 1800-2023 6.18): "
                << nodep->prettyNameQ());
        }
        // We only needed the forward declaration in order to parse correctly.
        // Delete later as may be ClassOrPackageRef's still pointing to it
        VL_DO_DANGLING(pushDeletep(nodep->unlinkFrBack()), nodep);
    }

    فراغ visit(AstNode* nodep) override { iterateChildren(nodep); }  // ParamVisitor::

عام:
    // CONSTRUCTORS
    LinkDotParamVisitor(AstNetlist* rootp, LinkDotState* statep)
        : m_statep{statep} {
        UINFO(4, __FUNCTION__ << ": ");
        iterate(rootp);
    }
    ~LinkDotParamVisitor() override = افتراضي;
};

//======================================================================

صنف LinkDotScopeVisitor final : عام VNVisitor {

    // STATE
    LinkDotState* ثابت m_statep;  // State to pass between visitors, including symbol table
    ثابت AstScope* m_scopep = nullptr;  // The current scope
    VSymEnt* m_modSymp = nullptr;  // Symbol entry for current module
    // Deferred AliasScope processing - must be done outer-to-inner for correct alias resolution
    std::vector<std::pair<AstAliasScope*, VSymEnt*>> m_deferredAliasScopes;

    // METHODS
عام:
    // getAlias with setVarAlias and setIfaceAlias implement disjoint-set data structure.
    // This algorithm is needed for the case when multiple alias statements
    // reference partially the same variables.
    ساكن AstVarScope* getAlias(AstVarScope* ثابت vscp) {
        𐤀𐤌 (vscp->user2p() && vscp != vscp->user2p()) {
            AstVarScope* ثابت aliasp = getAlias(VN_AS(vscp->user2p(), VarScope));
            vscp->user2p(aliasp);
            أرجع aliasp;
        } وإلا {
            أرجع vscp;
        }
    }

خاص:
    // Alias an interface reference to the interface it is connected to
    فراغ setIfaceAlias(AstVarScope* ثابت vscp, AstVarScope* ثابت aliasp) {
        UASSERT_OBJ(vscp->varp()->isIfaceRef(), vscp, "Interface alias of non-interface");
        UASSERT_OBJ(aliasp->varp()->isIfaceRef(), aliasp, "Interface alias of non-interface");
        getAlias(vscp)->user2p(getAlias(aliasp));
    }

    // Alias two variables. An output port in the lower scope survives, unless the other
    // must keep its own storage.
    فراغ setVarAlias(AstVarScope* ap, AstVarScope* bp) {
        UASSERT_OBJ(!ap->varp()->isIfaceRef(), ap, "Variable alias of interface");
        UASSERT_OBJ(!bp->varp()->isIfaceRef(), bp, "Variable alias of interface");
        ap = getAlias(ap);
        bp = getAlias(bp);
        𐤀𐤌 (ap == bp) أرجع;
        ثابت منطقي aWins = [&]() {
            ثابت AstVar* ثابت aVarp = ap->varp();
            ثابت AstVar* ثابت bVarp = bp->varp();
            // A primary IO is accessed directly by the user of the model, so must survive
            UASSERT_OBJ(!aVarp->isPrimaryIO() || !bVarp->isPrimaryIO(), ap,
                        "Alias of two primary IOs");
            𐤀𐤌 (bVarp->isPrimaryIO()) أرجع خطأ;
            𐤀𐤌 (aVarp->isPrimaryIO()) أرجع true;
            // A forced or public variable must keep its own storage. If both are, either works.
            𐤀𐤌 (aVarp->isForced() != bVarp->isForced()) أرجع aVarp->isForced();
            𐤀𐤌 (aVarp->isSigPublic() != bVarp->isSigPublic()) أرجع aVarp->isSigPublic();
            // Otherwise prefer an output port in the lower scope. This enables better V3Combine
            // due to having fewer upward hierarchical references (which prevent combining).
            𐤀𐤌 (aVarp->direction() != VDirection::OUTPUT) أرجع خطأ;
            // Is 'ap' strictly below 'bp' in the hierarchy
            لكل (ثابت AstScope* scopep = ap->scopep()->aboveScopep(); scopep;
                 scopep = scopep->aboveScopep()) {
                𐤀𐤌 (scopep == bp->scopep()) أرجع true;
            }
            أرجع خطأ;
        }();
        𐤀𐤌 (aWins) {
            bp->user2p(ap);
        } وإلا {
            ap->user2p(bp);
        }
    }

    // VISITORS
    فراغ visit(AstNetlist* nodep) override {  // ScopeVisitor::
        // Recurse..., backward as must do packages before using packages
        iterateChildrenBackwardsConst(nodep);
    }
    فراغ visit(AstScope* nodep) override {  // ScopeVisitor::
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
        𐤀𐤌 (AstIface* ثابت ifacep = VN_CAST(nodep->modp(), Iface)) {
            m_statep->insertIfaceModSym(ifacep, m_modSymp);
        }
        m_scopep = nodep;
        iterateChildren(nodep);
    }
    فراغ visit(AstVarScope* nodep) override {  // ScopeVisitor::
        𐤀𐤌 (!nodep->varp()->isFuncLocal() && !nodep->varp()->isClassMember()) {
            VSymEnt* ثابت varSymp
                = m_statep->insertSym(m_modSymp, nodep->varp()->name(), nodep, nullptr);
            𐤀𐤌 (nodep->varp()->isIfaceRef()) {
                AstIfaceRefDType* ثابت dtypep
                    = LinkDotState::ifaceRefFromArray(nodep->varp()->dtypep());
                UASSERT_OBJ(dtypep, nodep, "Non AstIfaceRefDType on isIfaceRef() var");
                𐤀𐤌 (nodep->varp()->isIfaceParent()) {
                    UINFO(9, "Iface parent ref var " << nodep->varp()->name() << " " << nodep);
                    // Find the interface cell the var references
                    UINFO(9, "Iface parent dtype " << dtypep);
                    ثابت string ifcellname = dtypep->cellName();
                    string baddot;
                    VSymEnt* okSymp;
                    VSymEnt* cellSymp = m_statep->findDotted(nodep->fileline(), m_modSymp,
                                                             ifcellname, baddot, okSymp, خطأ);
                    UASSERT_OBJ(
                        cellSymp, nodep,
                        "No symbol for interface instance: " << nodep->prettyNameQ(ifcellname));
                    UINFO(5, "       Found interface instance: se" << cvtToHex(cellSymp) << " "
                                                                   << cellSymp->nodep());
                    𐤀𐤌 (dtypep->modportName() != "") {
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
                    m_statep->insertScopeAlias(LinkDotState::SAMN_IFTOP, varSymp, cellSymp);
                }
            }
        }
    }
    فراغ visit(AstNodeFTask* nodep) override {  // ScopeVisitor::
        VSymEnt* ثابت symp = m_statep->insertBlock(m_modSymp, nodep->name(), nodep, nullptr);
        symp->fallbackp(m_modSymp);
        iterateChildren(nodep);
    }
    فراغ visit(AstNodeForeach* nodep) override {  // ScopeVisitor::
        VSymEnt* ثابت symp = m_statep->insertBlock(m_modSymp, nodep->name(), nodep, nullptr);
        symp->fallbackp(m_modSymp);
        // No recursion, we don't want to pick up variables
    }
    فراغ visit(AstConstraintForeach* nodep) override {  // ScopeVisitor::
        iterateChildren(nodep);
    }
    فراغ visit(AstWith* nodep) override {  // ScopeVisitor::
        VSymEnt* ثابت symp = m_statep->insertBlock(m_modSymp, nodep->name(), nodep, nullptr);
        symp->fallbackp(m_modSymp);
        iterateChildren(nodep);
    }
    فراغ visit(AstAlias* nodep) override {  // ScopeVisitor::
        // Track aliases
        UINFOTREE(9, nodep, "", "alias");
        AstVarScope* aliasVscp = nullptr;
        لكل (AstNode* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
            AstVarScope* vscp = nullptr;
            𐤀𐤌 (ثابت AstVarRef* ثابت refp = VN_CAST(itemp, VarRef)) {
                vscp = refp->varScopep();
            } وإلا {
                // Reaches into the scope of an instance, look it up by name
                ثابت AstVarXRef* ثابت xrefp = VN_AS(itemp, VarXRef);
                ثابت string scopename = xrefp->dotted() + "." + xrefp->name();
                string baddot;
                VSymEnt* okSymp;
                VSymEnt* ثابت symp = m_statep->findDotted(xrefp->fileline(), m_modSymp, scopename,
                                                           baddot, okSymp, خطأ);
                UASSERT_OBJ(symp, nodep, "No symbol for alias item: " << scopename);
                vscp = VN_CAST(symp->nodep(), VarScope);
            }
            UASSERT_OBJ(vscp, nodep, "VarScope unset");
            𐤀𐤌 (aliasVscp) {
                setVarAlias(aliasVscp, vscp);
            } وإلا {
                aliasVscp = vscp;
            }
        }
        iterateChildren(nodep);
        VL_DO_DANGLING(pushDeletep(nodep->unlinkFrBack()), nodep);
    }
    فراغ visit(AstAliasScope* nodep) override {  // ScopeVisitor::
        // Defer AliasScope processing - must process outer scopes before inner ones
        // so that nested interface port alias resolution works correctly
        UINFO(5, "ALIASSCOPE (deferred) " << nodep);
        m_deferredAliasScopes.emplace_back(nodep, m_modSymp);
    }
    فراغ processAliasScope(AstAliasScope* nodep, VSymEnt* modSymp) {
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
            طالما (!symp) {
                scopename = refp ? refp->name()
                                 : (inl.size() ? (inl + dottedPath + xrefp->name())
                                               : (dottedPath + xrefp->name()));
                string baddot;
                VSymEnt* okSymp;
                symp = m_statep->findDotted(nodep->rhsp()->fileline(), modSymp, scopename, baddot,
                                            okSymp, true);
                𐤀𐤌 (inl == "") اكسر;
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
                                                       scopename, baddot, okSymp, خطأ);
            UASSERT_OBJ(symp, nodep, "No symbol for interface alias lhs");
            UINFO(5, "       Found a linked scope LHS: " << scopename << "  se" << cvtToHex(symp)
                                                         << " " << symp->nodep());
            lhsSymp = symp;
        }
        // Remember the alias - can't do it yet because we may have additional symbols to be added,
        // or maybe an alias of an alias
        m_statep->insertScopeAlias(LinkDotState::SAMN_IFTOP, lhsSymp, rhsSymp);
        AstVarScope* ثابت lhsVscp = VN_CAST(lhsSymp->nodep(), VarScope);
        AstVarScope* ثابت rhsVscp = VN_CAST(rhsSymp->nodep(), VarScope);
        UASSERT_OBJ(lhsVscp && rhsVscp, nodep, "Interface alias missing variable scope");
        setIfaceAlias(lhsVscp, rhsVscp);
        // We have stored the link, we don't need these any more
        VL_DO_DANGLING(nodep->unlinkFrBack()->deleteTree(), nodep);
    }
    فراغ processDeferredAliasScopes() {
        // Sort by hierarchy depth (shallower first) so outer aliases are resolved before inner
        // Pre-compute depth map to avoid O(N log N * D) complexity in sort comparisons
        std::unordered_map<VSymEnt*, صحيح> depthMap;
        لكل (ثابت auto& pair : m_deferredAliasScopes) {
            VSymEnt* ثابت symp = pair.second;
            𐤀𐤌 (depthMap.find(symp) == depthMap.end()) {
                صحيح depth = 0;
                لكل (VSymEnt* p = symp; p; p = p->parentp()) ++depth;
                depthMap[symp] = depth;
            }
        }
        std::stable_sort(m_deferredAliasScopes.begin(), m_deferredAliasScopes.end(),
                         [&depthMap](ثابت std::pair<AstAliasScope*, VSymEnt*>& a,
                                     ثابت std::pair<AstAliasScope*, VSymEnt*>& b) {
                             أرجع depthMap.at(a.second) < depthMap.at(b.second);
                         });
        // Process in sorted order
        لكل (auto& pair : m_deferredAliasScopes) { processAliasScope(pair.first, pair.second); }
        m_deferredAliasScopes.clear();
    }
    فراغ visit(AstNodeGen* nodep) override {  // ScopeVisitor::  // LCOV_EXCL_LINE
        nodep->v3fatalSrc("Generate constructs should have been reduced out");
    }
    // For speed, don't recurse things that can't have scope
    // Note we allow AstNodeStmt's as generates may be under them
    فراغ visit(AstCell*) override {}  // ScopeVisitor::
    فراغ visit(AstVar*) override {}  // ScopeVisitor::
    فراغ visit(AstNode* nodep) override { iterateChildren(nodep); }  // ScopeVisitor::

عام:
    // CONSTRUCTORS
    LinkDotScopeVisitor(AstNetlist* rootp, LinkDotState* statep)
        : m_statep{statep} {
        UINFO(4, __FUNCTION__ << ": ");
        iterate(rootp);
        // Process deferred AliasScopes in outer-to-inner order
        processDeferredAliasScopes();
    }
    ~LinkDotScopeVisitor() override = افتراضي;
};

//======================================================================

// Iterate an interface to resolve modports
صنف LinkDotIfaceVisitor final : عام VNVisitor {
    // STATE
    LinkDotState* ثابت m_statep;  // State to pass between visitors, including symbol table
    VSymEnt* m_curSymp;  // Symbol Entry for current table, where to lookup/insert

    // VISITORS
    فراغ visit(AstModport* nodep) override {  // IfaceVisitor::
        // Modport: Remember its name for later resolution
        UINFO(5, "   fiv: " << nodep);
        VL_RESTORER(m_curSymp);
        {
            // Create symbol table for the vars
            m_curSymp = m_statep->insertBlock(m_curSymp, nodep->name(), nodep, nullptr);
            m_curSymp->fallbackp(VL_RESTORER_PREV(m_curSymp));
            iterateChildren(nodep);
        }
    }
    فراغ visit(AstModportFTaskRef* nodep) override {  // IfaceVisitor::
        UINFO(5, "   fif: " << nodep);
        iterateChildren(nodep);
        VSymEnt* ثابت symp = m_curSymp->findIdFallback(nodep->name());
        𐤀𐤌 (!symp) {
            nodep->v3error("Modport item not found: " << nodep->prettyNameQ());
        } وإلا 𐤀𐤌 (AstNodeFTask* ثابت ftaskp = VN_CAST(symp->nodep(), NodeFTask)) {
            // Make symbol under modport that points at the _interface_'s var, not the modport.
            nodep->ftaskp(ftaskp);
            VSymEnt* ثابت subSymp
                = m_statep->insertSym(m_curSymp, nodep->name(), ftaskp, nullptr /*package*/);
            m_statep->insertScopeAlias(LinkDotState::SAMN_MODPORT, subSymp, symp);
        } وإلا {
            nodep->v3error("Modport item is not a function/task: " << nodep->prettyNameQ());
        }
        𐤀𐤌 (m_statep->forScopeCreation()) {
            // Done with AstModportFTaskRef.
            // Delete to prevent problems if we dead-delete pointed to ftask
            nodep->unlinkFrBack();
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
        }
    }

    // Helper to resolve remaining path through a nested interface
    // When findDotted() partially matches (okSymp set, baddot non-empty),
    // this follows the interface type to resolve the remaining path.
    // Returns the resolved symbol, or nullptr if not found.
    // On success, clears baddot; on partial match of multi-level path, updates baddot.
    VSymEnt* resolveNestedInterfacePath(FileLine* fl, VSymEnt* okSymp, string& baddot) {
        𐤀𐤌 (!okSymp || baddot.empty()) أرجع nullptr;

        ساكن constexpr صحيح MAX_NESTING_DEPTH = 64;
        VSymEnt* curOkSymp = okSymp;

        لكل (صحيح depth = 0; depth < MAX_NESTING_DEPTH; ++depth) {
            // Try to get interface from the partially-matched symbol
            AstIface* ifacep = nullptr;
            𐤀𐤌 (ثابت AstCell* ثابت cellp = VN_CAST(curOkSymp->nodep(), Cell)) {
                ifacep = VN_CAST(cellp->modp(), Iface);
            } وإلا 𐤀𐤌 (ثابت AstVar* ثابت varp = VN_CAST(curOkSymp->nodep(), Var)) {
                𐤀𐤌 (varp->isIfaceRef()) {
                    𐤀𐤌 (ثابت AstIfaceRefDType* ثابت ifaceRefp
                        = LinkDotState::ifaceRefFromArray(varp->dtypep())) {
                        ifacep = ifaceRefp->ifaceViaCellp();
                    }
                }
            }

            𐤀𐤌 (!ifacep || !m_statep->existsNodeSym(ifacep)) أرجع nullptr;

            VSymEnt* ثابت ifaceSymp = m_statep->getNodeSym(ifacep);

            𐤀𐤌 (baddot.find('.') == string::npos) {
                // Simple identifier - direct lookup
                VSymEnt* ثابت symp = ifaceSymp->findIdFallback(baddot);
                𐤀𐤌 (symp) baddot.clear();
                أرجع symp;
            }

            // Multi-level path - use findDotted for partial resolution
            string remainingBaddot;
            VSymEnt* remainingOkSymp = nullptr;
            VSymEnt* ثابت symp = m_statep->findDotted(fl, ifaceSymp, baddot, remainingBaddot,
                                                       remainingOkSymp, true);
            𐤀𐤌 (symp) {
                baddot = remainingBaddot;
                أرجع symp;
            }

            // findDotted partially matched - check progress
            𐤀𐤌 (remainingBaddot == baddot || !remainingOkSymp) أرجع nullptr;

            // Continue resolving with updated state
            baddot = remainingBaddot;
            curOkSymp = remainingOkSymp;
        }

        UINFO(1, "Nested interface resolution depth limit exceeded at " << fl);
        أرجع nullptr;
    }

    // Resolve a modport expression to find the referenced symbol
    VSymEnt* resolveModportExpression(FileLine* fl, AstNodeExpr* exprp) {
        UINFO(5, "     resolveModportExpression: " << exprp);
        VSymEnt* symp = nullptr;
        𐤀𐤌 (AstParseRef* ثابت refp = VN_CAST(exprp, ParseRef)) {
            // Simple variable reference: modport mp(input .a(sig_a))
            symp = m_curSymp->findIdFallback(refp->name());
        } وإلا 𐤀𐤌 (AstVarRef* ثابت refp = VN_CAST(exprp, VarRef)) {
            // Already resolved VarRef (can happen if iterateChildren resolved it)
            𐤀𐤌 (refp->varScopep()) {
                // During scope creation, VarRef may have VarScope already linked
                symp = m_statep->getNodeSym(refp->varScopep());
            } وإلا 𐤀𐤌 (refp->varp()) {
                symp = m_curSymp->findIdFallback(refp->varp()->name());
            }
        } وإلا 𐤀𐤌 (AstVarXRef* ثابت refp = VN_CAST(exprp, VarXRef)) {
            // Resolved VarXRef (dotted reference resolved by earlier pass)
            𐤀𐤌 (refp->varp()) {
                // For nested interfaces, the var is in a different scope
                // First try to find the symbol via the var itself
                𐤀𐤌 (m_statep->existsNodeSym(refp->varp())) {
                    symp = m_statep->getNodeSym(refp->varp());
                } وإلا {
                    // Fallback: look up in current scope
                    symp = m_curSymp->findIdFallback(refp->varp()->name());
                }
            } وإلا 𐤀𐤌 (!refp->dotted().empty()) {
                // varp not set yet - use dotted path to find the symbol
                // The dotted part (e.g., "base") is the interface cell, and name is the member
                ثابت string fullPath = refp->dotted() + "." + refp->name();
                string baddot;
                VSymEnt* okSymp = nullptr;
                symp = m_statep->findDotted(fl, m_curSymp, fullPath, baddot, okSymp, true);
                // Handle nested interface path when findDotted had partial match
                𐤀𐤌 (okSymp && !baddot.empty()) {
                    VSymEnt* ثابت resolved = resolveNestedInterfacePath(fl, okSymp, baddot);
                    𐤀𐤌 (resolved) symp = resolved;
                }
            }
        } وإلا 𐤀𐤌 (AstDot* ثابت dotp = VN_CAST(exprp, Dot)) {
            // Dotted path: modport mp(input .a(inner.sig))
            منطقي hasPartSelect = خطأ;
            ثابت string dottedPath = extractDottedPath(dotp, hasPartSelect);
            𐤀𐤌 (hasPartSelect) {
                fl->v3warn(
                    E_UNSUPPORTED,
                    "Unsupported: Modport expression with part select (IEEE 1800-2023 25.5.4)");
            } وإلا {
                string baddot;
                VSymEnt* okSymp = nullptr;
                symp = m_statep->findDotted(fl, m_curSymp, dottedPath, baddot, okSymp, true);

                // Handle nested interface path when findDotted had partial match
                𐤀𐤌 (okSymp && !baddot.empty()) {
                    VSymEnt* ثابت resolved = resolveNestedInterfacePath(fl, okSymp, baddot);
                    𐤀𐤌 (resolved) symp = resolved;
                }

                𐤀𐤌 (!symp || !baddot.empty()) {
                    fl->v3error("Can't find modport expression target: "
                                << AstNode::prettyNameQ(dottedPath));
                    symp = nullptr;
                }
            }
        } وإلا 𐤀𐤌 (VN_IS(exprp, SelBit) || VN_IS(exprp, SelExtract)) {
            // Part select expressions not yet supported
            fl->v3warn(E_UNSUPPORTED,
                       "Unsupported: Modport expression with part select (IEEE 1800-2023 25.5.4)");
        } وإلا {
            // Other expression types not supported
            fl->v3warn(E_UNSUPPORTED,
                       "Unsupported: Complex modport expression (IEEE 1800-2023 25.5.4)");
        }
        أرجع symp;
    }

    فراغ visit(AstModportVarRef* nodep) override {  // IfaceVisitor::
        UINFO(5, "   fiv: " << nodep);
        iterateChildren(nodep);
        VSymEnt* symp = nullptr;
        𐤀𐤌 (nodep->exprp()) {
            // Modport expression syntax: modport mp(input .port_name(expression))
            symp = resolveModportExpression(nodep->fileline(), nodep->exprp());
        } وإلا {
            symp = m_curSymp->findIdFallback(nodep->name());
        }
        𐤀𐤌 (!symp) {
            nodep->v3error("Modport item not found: " << nodep->prettyNameQ());
        } وإلا 𐤀𐤌 (AstVar* ثابت varp = VN_CAST(symp->nodep(), Var)) {
            // Make symbol under modport that points at the _interface_'s var via the modport.
            // (Need modport still to test input/output markings)
            nodep->varp(varp);
            𐤀𐤌 (nodep->exprp()) {
                // For modport expressions, insert symbol pointing to the underlying var (not the
                // ModportVarRef which will be deleted during scope creation). The virtual port
                // name maps to the real signal's var.
                VSymEnt* ثابت subSymp
                    = m_statep->insertSym(m_curSymp, nodep->name(), varp, nullptr /*package*/);
                m_statep->insertScopeAlias(LinkDotState::SAMN_MODPORT, subSymp, symp);
            } وإلا {
                // For regular modport items, insert symbol pointing to ModportVarRef for
                // input/output marking tests.
                m_statep->insertSym(m_curSymp, nodep->name(), nodep, nullptr /*package*/);
            }
        } وإلا 𐤀𐤌 (AstVarScope* ثابت vscp = VN_CAST(symp->nodep(), VarScope)) {
            // Make symbol under modport that points at the _interface_'s var, not the modport.
            nodep->varp(vscp->varp());
            // Only insert symbol for modport expression virtual ports (exprp set).
            // For regular modport items, don't insert into the shared modport table because
            // each instance would overwrite the previous one, causing wrong VarScope lookups.
            // Regular items are found via the modport's fallback to the interface.
            𐤀𐤌 (nodep->exprp()) {
                VSymEnt* ثابت subSymp
                    = m_statep->insertSym(m_curSymp, nodep->name(), vscp, nullptr /*package*/);
                m_statep->insertScopeAlias(LinkDotState::SAMN_MODPORT, subSymp, symp);
            }
        } وإلا {
            nodep->v3error("Modport item is not a variable: " << nodep->prettyNameQ());
        }
        𐤀𐤌 (m_statep->forScopeCreation()) {
            // Done with AstModportVarRef.
            // Delete to prevent problems if we dead-delete pointed to variable
            nodep->unlinkFrBack();
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
        }
    }
    فراغ visit(AstModportClockingRef* nodep) override {  // IfaceVisitor::
        UINFO(5, "   fic: " << nodep);
        iterateChildren(nodep);
        VSymEnt* ثابت symp = m_curSymp->findIdFallback(nodep->name());
        𐤀𐤌 (!symp) {
            nodep->v3error("Modport item not found: " << nodep->prettyNameQ());
        } وإلا 𐤀𐤌 (AstClocking* ثابت clockingp = VN_CAST(symp->nodep(), Clocking)) {
            nodep->clockingp(clockingp);
            m_statep->insertSym(m_curSymp, nodep->name(), nodep, nullptr /*package*/);
        } وإلا {
            nodep->v3error("Modport item is not a clocking block: " << nodep->prettyNameQ());
        }
    }
    فراغ visit(AstScope* nodep) override {  // IfaceVisitor::
        𐤀𐤌 (nodep->user3SetOnce()) أرجع;
        iterateChildren(nodep);
    }
    فراغ visit(AstNode* nodep) override { iterateChildren(nodep); }  // IfaceVisitor::

عام:
    // CONSTRUCTORS
    LinkDotIfaceVisitor(AstIface* nodep, VSymEnt* curSymp, LinkDotState* statep)
        : m_statep{statep}
        , m_curSymp{curSymp} {
        UINFO(4, __FUNCTION__ << ": ");
        iterate(nodep);
    }
    ~LinkDotIfaceVisitor() override = افتراضي;
};

فراغ LinkDotState::computeIfaceModSyms() {
    //  AstScope::user3()  // bool. Already iterated by a LinkDotIfaceVisitor
    ثابت VNUser3InUse user3InUse;
    لكل (ثابت auto& itr : m_ifaceModSyms) {
        AstIface* ثابت nodep = itr.first;
        VSymEnt* ثابت symp = itr.second;
        LinkDotIfaceVisitor{nodep, symp, هذا};
    }
    m_ifaceModSyms.clear();
}

//======================================================================

صنف LinkDotResolveVisitor final : عام VNVisitor {
    // NODE STATE
    // Cleared on global
    //  *::user1p()             -> See LinkDotState
    //  *::user2p()             -> See LinkDotState
    //  *::user3()              // bool. Processed
    //  *::user4()              -> See LinkDotState
    ثابت VNUser3InUse m_inuser3;

    // TYPES
    تعداد DotPosition : uint8_t {
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
    AstNodeModule* m_modp = nullptr;  // Current module
    AstNodeFTask* m_ftaskp = nullptr;  // Current function/task
    AstMethodCall* m_randMethodCallp = nullptr;  // Current randomize() method call
    صحيح m_modportNum = 0;  // Uniqueify modport numbers
    صحيح m_indent = 0;  // Indentation (tree depth) for debug
    منطقي m_inSens = خطأ;  // True if in senitem
    ثابت AstWith* m_currentWithp = nullptr;  // Enclosing 'with' (nullptr if none)
    std::set<std::string>
        m_restrictedNamesUsed;  // Names from current 'with (id_list)' that resolved into target
    منطقي m_genericIfaceModule = خطأ;  // True if in module containing generic interface
    std::map<std::string, AstNode*> m_ifClassImpNames;  // Names imported from interface class
    // Inherited interface implementations needing a forward in the current class, by name
    std::map<std::string, ثابت VSymEnt*> m_ifForwards;
    std::set<AstClass*> m_extendsParam;  // Classes that have a parameterized super class
                                         // (except the default instances)
                                         // They are added to the set only in linkDotPrimary.
    منطقي m_insideClassExtParam = خطأ;  // Inside a class from m_extendsParam
    AstNew* m_explicitSuperNewp = nullptr;  // Hit a "super.new" call inside a "new" function
    std::map<AstNode*, AstPin*> m_usedPins;  // Pin used in this cell, map to duplicate
    std::map<AstNode*, std::map<std::string, AstPin*>> m_usedDefParamPins;
    // Defparam pins used for a given formal, by hierarchical path
    std::map<std::string, AstNodeModule*> m_modulesToRevisit;  // Modules to revisit a second time
    AstNode* m_lastDeferredp = nullptr;  // Last node which requested a revisit of its module
    AstNodeDType* m_packedArrayDtp = nullptr;  // Datatype reference for packed array
    منطقي m_inPackedArray = خطأ;  // Currently traversing a packed array tree
    منطقي m_replaceWithAlias
        = true;  // Replace VarScope with an alias. Used in the handling of AstAlias
    منطقي m_isParam = خطأ;  // Specifies whether currently visiting param variable
    منطقي m_resolvingTypedef = خطأ;  // Currently traversing a Typedef tree

    بنية DotStates final {
        DotPosition m_dotPos;  // Scope part of dotted resolution
        VSymEnt* m_dotSymp;  // SymEnt for dotted AstParse lookup
        ثابت AstDot* m_dotp;  // Current dot
        منطقي m_super;  // Starts with super reference
        منطقي m_unresolvedCell;  // Unresolved cell, needs help from V3Param
        منطقي m_unresolvedClass;  // Unresolved class reference, needs help from V3Param
        منطقي m_unresolvedGenericIface;  // Unresolved generic interface, needs help from V3Param
        منطقي m_genBlk;  // Contains gen block reference
        AstNode* m_unlinkedScopep;  // Unresolved scope, needs corresponding VarXRef
        AstDisable* m_disablep;  // Disable statement under which the reference is
        منطقي m_dotErr;  // Error found in dotted resolution, ignore upwards
        string m_dotText;  // String of dotted names found in below parseref
        DotStates() { init(nullptr); }
        ~DotStates() = افتراضي;
        فراغ init(VSymEnt* curSymp, منطقي unresolvedClass = خطأ) {
            m_dotPos = DP_NONE;
            m_dotSymp = curSymp;
            m_dotp = nullptr;
            m_super = خطأ;
            m_dotErr = خطأ;
            m_dotText = "";
            m_unresolvedCell = خطأ;
            m_unresolvedClass = unresolvedClass;
            m_unresolvedGenericIface = خطأ;
            m_genBlk = خطأ;
            m_unlinkedScopep = nullptr;
            m_disablep = nullptr;
        }
        string ascii() ثابت {
            ساكن ثابت حرف* ثابت names[]
                = {"DP_NONE", "DP_PACKAGE", "DP_FIRST", "DP_SCOPE", "DP_FINAL", "DP_MEMBER"};
            std::ostringstream sstr;
            sstr << "ds=" << names[m_dotPos];
            sstr << "  dse" << cvtToHex(m_dotSymp);
            ثابت string dsname = m_dotSymp->nodep()->name().substr(0, 8);
            sstr << "(" << m_dotSymp->nodep()->typeName() << (dsname.empty() ? "" : ":") << dsname
                 << ")";
            𐤀𐤌 (m_dotErr) sstr << "  [dotErr]";
            𐤀𐤌 (m_super) sstr << "  [super]";
            𐤀𐤌 (m_unresolvedCell) sstr << "  [unrCell]";
            𐤀𐤌 (m_unresolvedClass) sstr << "  [unrClass]";
            𐤀𐤌 (m_unresolvedGenericIface) sstr << "  [unrGIface]";
            𐤀𐤌 (m_genBlk) sstr << "  [genBlk]";
            sstr << "  txt=" << m_dotText;
            أرجع sstr.str();
        }
    } m_ds;  // State to preserve across recursions

    // METHODS - Variables
    AstVar* createImplicitVar(VSymEnt* /*lookupSymp*/, AstParseRef* nodep, AstNodeModule* modp,
                              VSymEnt* moduleSymp, منطقي noWarn) {
        // Create implicit after warning
        𐤀𐤌 (!noWarn) {
            𐤀𐤌 (nodep->fileline()->warnIsOff(V3ErrorCode::I_DEF_NETTYPE_WIRE)) {
                ثابت string suggest = m_statep->suggestSymFallback(moduleSymp, nodep->name(),
                                                                    LinkNodeMatcherVar{});
                nodep->v3error("Signal definition not found, and implicit disabled with "
                               "`default_nettype: "
                               << nodep->prettyNameQ() << '\n'
                               << (suggest.empty() ? "" : nodep->warnMore() + suggest));

            }
            // Bypass looking for suggestions if IMPLICIT is turned off
            // as there could be thousands of these suppressed in large netlists
            وإلا 𐤀𐤌 (!nodep->fileline()->warnIsOff(V3ErrorCode::IMPLICIT)) {
                ثابت string suggest = m_statep->suggestSymFallback(moduleSymp, nodep->name(),
                                                                    LinkNodeMatcherVar{});
                nodep->v3warn(IMPLICIT,
                              "Signal definition not found, creating implicitly: "
                                  << nodep->prettyNameQ() << '\n'
                                  << (suggest.empty() ? "" : nodep->warnMore() + suggest));
            }
        }
        AstVar* ثابت newp
            = جديد AstVar{nodep->fileline(), VVarType::WIRE, nodep->name(), VFlagLogicPacked{}, 1};
        newp->lifetime(VLifetime::STATIC_IMPLICIT);
        newp->trace(modp->modTrace());
        modp->addStmtsp(newp);
        // Link it to signal list, must add the variable under the module;
        // current scope might be lower now
        m_statep->insertSym(moduleSymp, newp->name(), newp, nullptr /*classOrPackagep*/);
        أرجع newp;
    }
    AstVar* foundToVarp(ثابت VSymEnt* symp, AstNode* nodep, VAccess access) {
        // Return a variable if possible, auto converting a modport to variable
        𐤀𐤌 (!symp) {
            أرجع nullptr;
        } وإلا 𐤀𐤌 (VN_IS(symp->nodep(), Var)) {
            أرجع VN_AS(symp->nodep(), Var);
        } وإلا 𐤀𐤌 (VN_IS(symp->nodep(), ModportVarRef)) {
            AstModportVarRef* ثابت snodep = VN_AS(symp->nodep(), ModportVarRef);
            AstVar* ثابت varp = snodep->varp();
            𐤀𐤌 (access.isWriteOrRW() && snodep->direction().isReadOnly()) {
                nodep->v3error("Attempt to drive input-only modport: " << nodep->prettyNameQ());
            }  // else other simulators don't warn about reading, and IEEE doesn't say illegal
            أرجع varp;
        } وإلا {
            أرجع nullptr;
        }
    }

    // Look up a virtual port through modport expression mapping.
    // When a dotted reference uses a modport with expression syntax, the virtual port name
    // maps to the real signal. This helper resolves that mapping during scope creation.
    // Returns the VarScope for the real signal, or nullptr if not found.
    AstVarScope* findVirtualPortVarScope(VSymEnt* dotSymp, ثابت string& portName) {
        ثابت AstVarScope* ثابت dotVscp = VN_CAST(dotSymp->nodep(), VarScope);
        ثابت AstVar* ثابت dotVarp = dotVscp ? dotVscp->varp() : nullptr;
        𐤀𐤌 (!dotVarp) أرجع nullptr;

        AstNodeDType* dtypep = dotVarp->childDTypep();
        𐤀𐤌 (!dtypep) dtypep = dotVarp->subDTypep();
        𐤀𐤌 (!dtypep) أرجع nullptr;
        ثابت AstIfaceRefDType* ثابت ifaceRefp = VN_CAST(dtypep, IfaceRefDType);
        𐤀𐤌 (!ifaceRefp || !ifaceRefp->modportp() || !ifaceRefp->ifaceViaCellp()) أرجع nullptr;

        // Get the interface cell's symbol table
        VSymEnt* ثابت ifaceCellSymp = m_statep->getNodeSym(ifaceRefp->ifaceViaCellp());

        // Look up modport by name in interface cell
        VSymEnt* ثابت modportSymp = ifaceCellSymp->findIdFallback(ifaceRefp->modportp()->name());
        𐤀𐤌 (!modportSymp) أرجع nullptr;

        // Look up virtual port name in modport symbol table
        string baddot;
        VSymEnt* ثابت virtPortp = m_statep->findSymPrefixed(modportSymp, portName, baddot, true);
        𐤀𐤌 (!virtPortp) أرجع nullptr;

        // Symbol may point to VarScope or Var
        𐤀𐤌 (AstVarScope* vscp = VN_CAST(virtPortp->nodep(), VarScope)) أرجع vscp;

        // Symbol points to Var - look up VarScope by name in interface cell
        𐤀𐤌 (ثابت AstVar* ثابت realVarp = VN_CAST(virtPortp->nodep(), Var)) {
            VSymEnt* ثابت realFoundp
                = m_statep->findSymPrefixed(ifaceCellSymp, realVarp->name(), baddot, true);
            أرجع realFoundp ? VN_CAST(realFoundp->nodep(), VarScope) : nullptr;
        }
        أرجع nullptr;
    }

    // Capture a ParamTypeDType reference for interface typedef retargeting.
    // Called when a RefDType resolves to a ParamTypeDType owned by an interface.
    فراغ captureIfaceParamType(AstRefDType* nodep, AstParamTypeDType* defp) {
        𐤀𐤌 (!V3LinkDotIfaceCapture::enabled() || !m_statep->forPrimary()) أرجع;
        AstNodeModule* ثابت defOwnerModp = V3LinkDotIfaceCapture::findOwnerModule(defp);
        𐤀𐤌 (!defOwnerModp || !VN_IS(defOwnerModp, Iface)) أرجع;
        AstCell* ثابت cellForCapture
            = m_ds.m_dotSymp ? VN_CAST(m_ds.m_dotSymp->nodep(), Cell) : nullptr;
        𐤀𐤌 (!cellForCapture) أرجع;
        UINFO(9, indent() << "iface capture add paramtype " << nodep
                          << " iface=" << defOwnerModp->prettyNameQ());
        V3LinkDotIfaceCapture::addParamType(nodep, cellForCapture->name(), m_modp, defp,
                                            defOwnerModp->name());
    }

    AstNodeStmt* addImplicitSuperNewCall(AstFunc* ثابت nodep,
                                         ثابت AstClassExtends* ثابت classExtendsp) {
        // Returns the added node
        FileLine* ثابت fl = nodep->fileline();
        AstArg* argsp = nullptr;
        𐤀𐤌 (classExtendsp->argsp()) argsp = classExtendsp->argsp()->cloneTree(true);
        AstNew* ثابت newExprp = جديد AstNew{fl, argsp};
        newExprp->isImplicit(true);
        AstDot* ثابت superNewp = جديد AstDot{fl, خطأ, جديد AstParseRef{fl, "super"}, newExprp};
        AstNodeStmt* ثابت superNewStmtp = superNewp->makeStmt();
        لكل (AstNode* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            // super.new shall be the first statement (IEEE 1800-2023 8.15)
            // but some nodes (such as variable declarations and typedefs) should stay before
            𐤀𐤌 (VN_IS(stmtp, NodeStmt)) {
                stmtp->addHereThisAsNext(superNewStmtp);
                أرجع superNewStmtp;
            }
        }
        // There were no statements
        nodep->addStmtsp(superNewStmtp);
        أرجع superNewStmtp;
    }
    فراغ checkNoDot(AstNode* nodep) {
        𐤀𐤌 (VL_UNLIKELY(!nodep)) {
            UINFO(9, indent() << "iface capture null node passed to checkNoDot; dot state="
                              << m_ds.ascii());
            m_ds.m_dotErr = true;
            أرجع;
        }
        𐤀𐤌 (VL_UNLIKELY(m_ds.m_dotPos != DP_NONE)) {
            UINFO(9, indent() << "ds=" << m_ds.ascii());
            UINFO(9, indent() << "iface capture checkNoDot hit node=" << nodep
                              << " dotState=" << m_ds.ascii());
            𐤀𐤌 (VL_UNLIKELY(!m_ds.m_dotp)) {
                nodep->v3error("Syntax error: Not expecting " << nodep->type()
                                                              << " here (missing dot context)");
            } وإلا {
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
    AstVar* findIfaceTopVarp(AstNode* nodep, VSymEnt* parentEntp, ثابت string& name) {
        ثابت string findName = name + "__Viftop";
        VSymEnt* ثابت ifaceSymp = parentEntp->findIdFallback(findName);
        AstVar* ثابت ifaceTopVarp = ifaceSymp ? VN_AS(ifaceSymp->nodep(), Var) : nullptr;
        UASSERT_OBJ(ifaceTopVarp, nodep, "Can't find interface var ref: " << findName);
        أرجع ifaceTopVarp;
    }
    AstRSProd* findProd(AstNode* nodep, VSymEnt* parentEntp, ثابت string& name) {
        ثابت VSymEnt* ثابت foundp = parentEntp->findIdFallback(name);
        AstRSProd* foundNodep = foundp ? VN_CAST(foundp->nodep(), RSProd) : nullptr;
        𐤀𐤌 (!foundNodep) {
            VSpellCheck speller;
            LinkNodeMatcherProd matcher;
            parentEntp->candidateIdFlat(&speller, &matcher, خطأ);
            ثابت string suggest = speller.bestCandidateMsg(name);
            UINFO(1, "   ErrParseRef curSymp=se" << cvtToHex(parentEntp));
            nodep->v3error("Production " << AstNode::prettyNameQ(name) << " not found\n"
                                         << (suggest.empty() ? "" : nodep->warnMore() + suggest));
            أرجع nullptr;
        }
        أرجع foundNodep;
    }
    فراغ duplicatePinError(AstPin* nodep, AstNode* origp, ثابت حرف* whatp) {
        nodep->v3error("Duplicate " << whatp << " connection: " << nodep->prettyNameQ() << '\n'
                                    << nodep->warnContextPrimary() << '\n'
                                    << origp->warnOther() << "... Location of original " << whatp
                                    << " connection\n"
                                    << origp->warnContextSecondary());
    }
    فراغ markAndCheckPinDup(AstPin* nodep, AstNode* refp, ثابت حرف* whatp) {
        ثابت auto pair = m_usedPins.emplace(refp, nodep);
        𐤀𐤌 (pair.second) {
            𐤀𐤌 (!nodep->paramPath().empty())
                m_usedDefParamPins[refp].emplace(nodep->paramPath(), nodep);
            أرجع;
        } وإلا {
            AstNode* ثابت origp = pair.first->second;
            𐤀𐤌 (nodep->paramPath().empty() || VN_AS(origp, Pin)->paramPath().empty()) {
                duplicatePinError(nodep, origp, whatp);
                أرجع;
            }
        }

        auto& defParamPins = m_usedDefParamPins[refp];
        ثابت auto defParamIt = defParamPins.find(nodep->paramPath());
        𐤀𐤌 (defParamIt != defParamPins.end()) {
            duplicatePinError(nodep, defParamIt->second, whatp);
            أرجع;
        }
        defParamPins.emplace(nodep->paramPath(), nodep);
    }
    ساكن منطقي extendsClass(ثابت AstClass* classp, ثابت AstClass* targetp) {
        // Return true if classp inherits from targetp through already resolved extends
        // If performance of this becomes a problem, use just a counter to check
        // exceeds some number of iterations then perform the more expensive analysis
        std::set<ثابت AstClass*> visited;
        std::vector<ثابت AstClass*> todo{classp};
        طالما (!todo.empty()) {
            ثابت AstClass* ثابت currp = todo.back();
            todo.pop_back();
            𐤀𐤌 (currp == targetp) أرجع true;
            𐤀𐤌 (!visited.insert(currp).second) استمر;
            لكل (ثابت AstClassExtends* cextp = currp->extendsp(); cextp;
                 cextp = VN_AS(cextp->nextp(), ClassExtends)) {
                𐤀𐤌 (ثابت AstClass* ثابت basep = cextp->classOrNullp()) todo.push_back(basep);
            }
        }
        أرجع خطأ;
    }
    ساكن AstClocking* sensClockingp(AstNode* nodep) {
        // Return the clocking block referenced by nodep, either directly or through a modport
        𐤀𐤌 (AstClocking* ثابت clockingp = VN_CAST(nodep, Clocking)) أرجع clockingp;
        𐤀𐤌 (ثابت AstModportClockingRef* ثابت clockingRefp = VN_CAST(nodep, ModportClockingRef))
            أرجع clockingRefp->clockingp();
        أرجع nullptr;
    }
    VSymEnt* getCreateClockingEventSymEnt(AstClocking* clockingp) {
        AstVar* ثابت eventp = clockingp->ensureEventp(true);
        𐤀𐤌 (!eventp->user1p()) eventp->user1p(جديد VSymEnt{m_statep->symsp(), eventp});
        أرجع reinterpret_cast<VSymEnt*>(eventp->user1p());
    }
    VSymEnt* findInlinedDotsSym(AstNode* nodep, ثابت string& inlinedDots) {
        // Return scope that applies for searching, given renaming due to module inlining.
        // Dotted lookup is always relative to module, as maybe variable
        // name lower down with same scope name we want to ignore (t_math_divw)
        VSymEnt* dotSymp = m_modSymp;
        ثابت string inl = AstNode::dedotName(inlinedDots);
        string baddot;
        VSymEnt* okSymp = nullptr;
        dotSymp = m_statep->findDotted(nodep->fileline(), dotSymp, inl, baddot, okSymp, خطأ);
        UASSERT_OBJ(dotSymp, nodep,
                    "Couldn't resolve inlined scope " << AstNode::prettyNameQ(baddot)
                                                      << " in: " << inlinedDots);
        UINFO(9, indent() << "findInlinedDotsSym " << dotSymp
                          << " search start due to inlinedDots=" << inlinedDots);
        أرجع dotSymp;
    }
    VSymEnt* findDottedRefSymp(AstNode* nodep, VSymEnt* lookupSymp, ثابت string& dotted,
                               string& baddot, VSymEnt*& okSymp, منطقي* modportp = nullptr) {
        // Return scope to search for a relinked hierarchical reference
        // (AstVarXRef or AstEnumItemRef), or nullptr after error if not found
        // If modportp is given, set true when the reference went through a modport
        𐤀𐤌 (modportp) *modportp = خطأ;
        VSymEnt* dotSymp = m_statep->findDotted(nodep->fileline(), lookupSymp, dotted, baddot,
                                                okSymp, true);  // Maybe nullptr
        𐤀𐤌 (!dotSymp) {
            nodep->v3error(
                "Can't find definition of "
                << (!baddot.empty() ? AstNode::prettyNameQ(baddot) : nodep->prettyNameQ()) << '\n'
                << nodep->warnContextPrimary());
            أرجع nullptr;
        }
        𐤀𐤌 (ثابت AstVar* ثابت varp = VN_CAST(dotSymp->nodep(), Var)) {
            𐤀𐤌 (ثابت AstIfaceRefDType* ثابت ifaceRefp
                = VN_CAST(varp->childDTypep(), IfaceRefDType)) {
                𐤀𐤌 (ifaceRefp->modportp()) {
                    dotSymp = m_statep->getNodeSym(ifaceRefp->modportp());
                    𐤀𐤌 (modportp) *modportp = true;
                } وإلا {
                    dotSymp = m_statep->getNodeSym(ifaceRefp->ifacep());
                }
            }
        } وإلا 𐤀𐤌 (ثابت AstModportClockingRef* ثابت clockingRefp
                   = VN_CAST(dotSymp->nodep(), ModportClockingRef)) {
            dotSymp = m_statep->getNodeSym(clockingRefp->clockingp());
        }
        أرجع dotSymp;
    }

    ساكن منطقي isParamedClassRefDType(ثابت AstNode* classp) {
        طالما (ثابت AstRefDType* ثابت refp = VN_CAST(classp, RefDType))
            classp = refp->subDTypep();
        أرجع (VN_IS(classp, ClassRefDType) && VN_AS(classp, ClassRefDType)->paramsp())
               || VN_IS(classp, ParamTypeDType);
    }
    منطقي isParamedClassRef(ثابت AstNode* nodep) {
        // Is this a parameterized reference to a class, or a reference to class parameter
        𐤀𐤌 (ثابت auto* classRefp = VN_CAST(nodep, ClassOrPackageRef)) {
            𐤀𐤌 (classRefp->paramsp()) أرجع true;
            ثابت auto* classp = classRefp->classOrPackageNodep();
            طالما (ثابت auto* typedefp = VN_CAST(classp, Typedef)) classp = typedefp->subDTypep();
            أرجع isParamedClassRefDType(classp);
        }
        أرجع isParamedClassRefDType(nodep);
    }
    VSymEnt* getThisClassSymp() {
        VSymEnt* classSymp = m_ds.m_dotSymp;
        طالما (classSymp && !VN_IS(classSymp->nodep(), Class)) {
            classSymp = classSymp->parentp();
        }
        أرجع classSymp;
    }
    // Is the current scope within the given class, including within its nested classes
    منطقي insideClass(ثابت AstClass* classp) ثابت {
        لكل (ثابت VSymEnt* symp = m_curSymp; symp; symp = symp->parentp()) {
            𐤀𐤌 (symp->nodep() == classp) أرجع true;
        }
        أرجع خطأ;
    }
    // 'randomize() with' target class not yet specialized; defer to post-V3Param pass
    منطقي randTargetUnresolved() ثابت {
        أرجع m_statep->forPrimary() && m_currentWithp && m_randMethodCallp && !m_randSymp;
    }
    // Is 'funcp' of 'classp' virtual, directly or by overriding a virtual method of a base
    // class (IEEE 1800-2023 8.20)
    منطقي isVirtualMethod(ثابت AstNodeFTask* funcp, ثابت AstClass* classp) ثابت {
        𐤀𐤌 (funcp->isVirtual()) أرجع true;
        لكل (ثابت AstClassExtends* cextp = classp->extendsp(); cextp;
             cextp = VN_AS(cextp->nextp(), ClassExtends)) {
            AstClass* ثابت basep = cextp->classOrNullp();
            𐤀𐤌 (!basep) استمر;
            ثابت VSymEnt* ثابت foundp = m_statep->getNodeSym(basep)->findIdFlat(funcp->name());
            ثابت AstNodeFTask* ثابت baseFuncp
                = foundp ? VN_CAST(foundp->nodep(), NodeFTask) : nullptr;
            ثابت AstClass* ثابت baseClassp = foundp && foundp->parentp()
                                                   ? VN_CAST(foundp->parentp()->nodep(), Class)
                                                   : nullptr;
            𐤀𐤌 (baseFuncp && baseClassp && isVirtualMethod(baseFuncp, baseClassp)) أرجع true;
        }
        أرجع خطأ;
    }
    // Is imported 'implp' an implementation of interface method 'ifSymp' (IEEE 1800-2023
    // 8.26.2)? If its class does not derive from the interface class, it needs a forwarding
    // override in the derived class to be reachable through the interface.
    منطقي inheritedImplements(ثابت VSymEnt* implp, ثابت VSymEnt* ifSymp, منطقي& needForwardr) {
        needForwardr = خطأ;
        ثابت AstNodeFTask* ثابت implFuncp = VN_CAST(implp->nodep(), NodeFTask);
        𐤀𐤌 (!implFuncp || implFuncp->pureVirtual() || implFuncp->isStatic()) أرجع خطأ;
        ثابت AstClass* ثابت implClassp
            = implp->parentp() ? VN_CAST(implp->parentp()->nodep(), Class) : nullptr;
        ثابت AstClass* ثابت ifClassp
            = ifSymp->parentp() ? VN_CAST(ifSymp->parentp()->nodep(), Class) : nullptr;
        𐤀𐤌 (!implClassp || !ifClassp) أرجع خطأ;
        𐤀𐤌 (extendsClass(implClassp, ifClassp)) أرجع true;
        𐤀𐤌 (!isVirtualMethod(implFuncp, implClassp)) أرجع خطأ;
        needForwardr = true;
        أرجع true;
    }
    // Add 'implp' override to 'classp' that calls super.implp, so the C++ method of an
    // implemented interface class is overridden
    فراغ addInterfaceForward(AstClass* classp, AstNodeFTask* implp, AstClass* implClassp) {
        FileLine* ثابت flp = implp->fileline();
        AstVar* fvarp = nullptr;
        AstNodeFTask* newp;
        𐤀𐤌 (AstFunc* ثابت implFuncp = VN_CAST(implp, Func)) {
            fvarp = VN_AS(implFuncp->fvarp(), Var)->cloneTree(خطأ);
            newp = جديد AstFunc{flp, implp->name(), nullptr, fvarp};
        } وإلا {
            newp = جديد AstTask{flp, implp->name(), nullptr};
        }
        newp->classMethod(true);
        newp->isVirtual(true);
        newp->lifetime(implp->lifetime());
        AstArg* argsp = nullptr;
        لكل (AstNode* stmtp = implp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstVar* ثابت portp = VN_CAST(stmtp, Var);
            𐤀𐤌 (!portp || !portp->isIO()) استمر;
            AstVar* ثابت newPortp = portp->cloneTree(خطأ);
            newp->addStmtsp(newPortp);
            argsp = AstNode::addNext(
                argsp, جديد AstArg{flp, "", جديد AstVarRef{flp, newPortp, VAccess::READ}});
        }
        AstNodeFTaskRef* callp;
        𐤀𐤌 (AstFunc* ثابت implFuncp = VN_CAST(implp, Func)) {
            callp = جديد AstFuncRef{flp, implFuncp, argsp};
        } وإلا {
            callp = جديد AstTaskRef{flp, VN_AS(implp, Task), argsp};
        }
        callp->classOrPackagep(implClassp);
        callp->superReference(true);
        𐤀𐤌 (fvarp) {
            newp->addStmtsp(جديد AstAssign{flp, جديد AstVarRef{flp, fvarp, VAccess::WRITE}, callp});
        } وإلا {
            newp->addStmtsp(callp->makeStmt());
        }
        UINFO(9, indent() << "Interface forward " << newp);
        classp->addMembersp(newp);
    }
    فراغ importDerivedClass(AstClass* derivedClassp, VSymEnt* baseSymp, AstClass* baseClassp) {
        // Also used for standard 'extends' from a base class
        UINFO(8, indent() << "importDerivedClass to " << derivedClassp << " from " << baseClassp);
        لكل (VSymEnt::const_iterator it = baseSymp->begin(); it != baseSymp->end(); ++it) {
            𐤀𐤌 (AstNode* baseSubp = it->second->nodep()) {
                UINFO(8, indent() << "  SymFunc " << baseSubp);
                ثابت string impOrExtends
                    = baseClassp->isInterfaceClass() ? " implements " : " extends ";
                𐤀𐤌 (VN_IS(baseSubp, NodeFTask)) {
                    ثابت VSymEnt* ثابت foundp = m_curSymp->findIdFlat(baseSubp->name());
                    ثابت AstNodeFTask* ثابت baseFuncp = VN_CAST(baseSubp, NodeFTask);
                    𐤀𐤌 (!baseFuncp || !baseFuncp->pureVirtual()) استمر;
                    منطقي needForward = خطأ;
                    ثابت منطقي existsInDerived
                        = foundp
                          && (!foundp->imported()
                              || inheritedImplements(foundp, it->second, needForward));
                    𐤀𐤌 (needForward && m_statep->forPrimary()
                        && !derivedClassp->isInterfaceClass()) {
                        m_ifForwards.emplace(baseSubp->name(), foundp);
                    }
                    𐤀𐤌 (m_statep->forPrimary() && !existsInDerived
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
                    𐤀𐤌 (m_statep->forPrimary() && !existsInDerived
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
                𐤀𐤌 (VN_IS(baseSubp, Constraint)) {
                    ثابت VSymEnt* ثابت foundp = m_curSymp->findIdFlat(baseSubp->name());
                    ثابت AstConstraint* ثابت baseFuncp = VN_CAST(baseSubp, Constraint);
                    𐤀𐤌 (!baseFuncp || !baseFuncp->isKwdPure()) استمر;
                    ثابت منطقي existsInDerived = foundp && !foundp->imported();
                    𐤀𐤌 (m_statep->forPrimary() && !existsInDerived
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
    فراغ importSymbolsFromExtended(AstClass* ثابت nodep, AstClassExtends* ثابت cextp) {
        AstClass* ثابت baseClassp = cextp->classp();
        VSymEnt* ثابت srcp = m_statep->getNodeSym(baseClassp);
        importDerivedClass(nodep, srcp, baseClassp);
        𐤀𐤌 (!cextp->isImplements()) m_curSymp->importFromClass(m_statep->symsp(), srcp);
    }
    فراغ classExtendImport(AstClass* nodep) {
        // A class reference might be to a class that is later in Ast due to
        // e.g. parameterization or referring to a "class (type T) extends T"
        // Resolve it so later Class:: references into its base classes work
        symIterateNull(nodep, m_statep->getNodeSym(nodep));
    }

    فراغ checkDeclOrder(AstNode* nodep, AstNode* declp) {
        ثابت uint32_t declTokenNum = declp->declTokenNum();
        𐤀𐤌 (!declTokenNum) أرجع;  // Not implemented/valid on this object
        𐤀𐤌 (nodep->fileline()->tokenNum() < declTokenNum) {
            UINFO(1, "Related node " << nodep->fileline()->tokenNum() << " " << nodep);
            UINFO(1, "Related decl " << declTokenNum << " " << declp);
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
    فراغ checkMemberDeclOrder(AstNode* nodep, AstMemberDType* declp) {
        ثابت uint32_t declTokenNum = declp->fileline()->tokenNum();
        𐤀𐤌 (nodep->fileline()->tokenNum() < declTokenNum) {
            UINFO(1, "Related node " << nodep->fileline()->tokenNum() << " " << nodep);
            UINFO(1, "Related decl " << declTokenNum << " " << declp);
            nodep->v3error("Reference to "
                           << nodep->prettyNameQ() << " before declaration (IEEE 1800-2023 6.18)\n"
                           << nodep->warnMore()
                           << "... Suggest move the declaration before the reference\n"
                           << nodep->warnContextPrimary() << '\n'
                           << declp->warnOther() << "... Location of original declaration\n"
                           << declp->warnContextSecondary());
        }
    }

    فراغ replaceWithCheckBreak(AstNode* oldp, AstNodeDType* newp) {
        // Flag now to avoid V3Broken throwing an internal error
        𐤀𐤌 (oldp->wouldBreak(newp)) {
            newp->v3error(
                "Data type used where a non-data type is expected: " << newp->prettyNameQ());
            oldp->replaceWith(جديد AstConst{newp->fileline(), AstConst::BitFalseErroring{}});
        } وإلا {
            oldp->replaceWith(newp);
        }
    }

    // Marks the current module to be revisited after the initial AST iteration
    فراغ revisitLater(AstNode* deferredNodep) {
        // Need to revisit entire module to build up all the necessary context
        m_lastDeferredp = deferredNodep;
        m_modulesToRevisit.insert(std::make_pair(m_modp->name(), m_modp));
    }

    فراغ updateVarUse(AstVar* nodep) {
        // Avoid dotted.PARAM false positive when in a parameter block
        // that is if ()'ed off by same dotted name as another block
        𐤀𐤌 (nodep && nodep->isParam()) nodep->usedParam(true);
    }

    ساكن VSymEnt* findIdFallbackSkipMemberDType(VSymEnt* lookp, ثابت string& name) {
        VSymEnt* shadowEntp = nullptr;  // Shadowing variable: not a type, kept for error report
        طالما (lookp) {
            VSymEnt* ثابت foundp = lookp->findIdFlat(name);
            𐤀𐤌 (foundp && !VN_IS(foundp->nodep(), MemberDType)) {
                // Non-type entries are not type candidates
                // (IEEE 1800-2023 6.18); skip them so an enclosing
                // type is found, but keep one to preserve the "found: ..." error.
                𐤀𐤌 (VN_IS(foundp->nodep(), Typedef) || VN_IS(foundp->nodep(), ParamTypeDType)
                    || VN_IS(foundp->nodep(), Class)) {
                    أرجع foundp;
                }
                𐤀𐤌 (!shadowEntp) shadowEntp = foundp;
            }
            lookp = lookp->fallbackp();
        }
        أرجع shadowEntp;
    }

    فراغ symIterateChildren(AstNode* nodep, VSymEnt* symp) {
        // Iterate children, changing to given context, with restore to old context
        VL_RESTORER_COPY(m_ds);
        VL_RESTORER(m_curSymp);
        m_curSymp = symp;
        m_ds.init(m_curSymp);
        iterateChildren(nodep);
    }
    فراغ symIterateNull(AstNode* nodep, VSymEnt* symp) {
        // Iterate node, changing to given context, with restore to old context
        VL_RESTORER_COPY(m_ds);
        VL_RESTORER(m_curSymp);
        m_curSymp = symp;
        m_ds.init(m_curSymp);
        iterateNull(nodep);
    }
    ساكن ثابت AstNodeDType* getExprDTypep(ثابت AstNodeExpr* selp) {
        طالما (ثابت AstNodePreSel* ثابت sp = VN_CAST(selp, NodePreSel)) selp = sp->fromp();
        𐤀𐤌 (ثابت AstMemberSel* ثابت sp = VN_CAST(selp, MemberSel)) {
            𐤀𐤌 (ثابت AstNodeDType* dtypep = getExprDTypep(sp->fromp())) {
                𐤀𐤌 (ثابت AstClassRefDType* ثابت classRefp = VN_CAST(dtypep, ClassRefDType)) {
                    ثابت AstClass* ثابت classp = classRefp->classp();
                    ثابت منطقي found = classp->existsMember(
                        [&dtypep, name = selp->name()](ثابت AstClass*, ثابت AstVar* nodep) {
                            dtypep = nodep->childDTypep();
                            أرجع nodep->name() == name;
                        });
                    𐤀𐤌 (found) أرجع dtypep->elemDTypep();
                    selp->v3error("Class " << classRefp->prettyNameQ()
                                           << " does not contain field " << selp->prettyNameQ());
                } وإلا {
                    selp->v3fatalSrc("Member selection on expression of type "
                                     << dtypep->prettyDTypeNameQ()
                                     << ", which is not a class type");
                }
            }
        } وإلا 𐤀𐤌 (ثابت AstNodeVarRef* ثابت varRefp = VN_CAST(selp, NodeVarRef)) {
            أرجع varRefp->varp()->childDTypep()->elemDTypep();
        }
        أرجع nullptr;
    }
    ساكن ثابت AstVar* getNextVarp(ثابت AstNode* stmtsp) {
        // Only IO ports, as parameters are on paramsp(), not the pinsp() list paired here
        طالما (stmtsp) {
            𐤀𐤌 (ثابت AstVar* ثابت varp = VN_CAST(stmtsp, Var)) {
                𐤀𐤌 (varp->isIO()) أرجع varp;
            }
            stmtsp = stmtsp->nextp();
        }
        أرجع nullptr;
    }
    // Introduce implicit parameters for modules with generic interafeces
    فراغ addImplicitParametersOfGenericIface(AstCell* ثابت nodep, ثابت AstModule* ثابت modp) {
        // Get Param number
        صحيح paramNum = 1;
        لكل (AstPin* paramp = nodep->paramsp(); paramp; paramp = VN_CAST(paramp->nextp(), Pin)) {
            ++paramNum;
        }

        // Add each implicit parameter type
        ثابت AstVar* modIfaceVarp = getNextVarp(modp->stmtsp());
        لكل (ثابت AstPin* pinp = nodep->pinsp(); pinp && modIfaceVarp;
             pinp = VN_CAST(pinp->nextp(), Pin),
                           modIfaceVarp = getNextVarp(modIfaceVarp->nextp())) {
            𐤀𐤌 (modIfaceVarp->varType() != VVarType::IFACEREF
                || !VN_IS(modIfaceVarp->childDTypep()->elemDTypep(), IfaceGenericDType)) {
                استمر;
            }
            AstNode* exprp = pinp->exprp();
            𐤀𐤌 (!exprp) {
                modIfaceVarp->v3error("Interface port "
                                      << modIfaceVarp->prettyNameQ()
                                      << " is not connected to interface/modport pin expression");
                استمر;
            }
            طالما (ثابت AstNodePreSel* ثابت preSelp = VN_CAST(exprp, NodePreSel)) {
                exprp = preSelp->fromp();
            }
            // Resolve pin expression to the enclosing module's port Var. At primary
            // LinkDot the expression may still be an AstParseRef, so also look up by name.
            AstVar* enclosingVarp = nullptr;
            ثابت AstVarRef* ثابت varRefp = VN_CAST(exprp, VarRef);
            𐤀𐤌 (varRefp) {
                enclosingVarp = varRefp->varp();
            } وإلا 𐤀𐤌 (ثابت AstParseRef* ثابت parseRefp = VN_CAST(exprp, ParseRef)) {
                𐤀𐤌 (m_modp) {
                    𐤀𐤌 (VSymEnt* ثابت symp
                        = m_statep->getNodeSym(m_modp)->findIdFlat(parseRefp->name())) {
                        enclosingVarp = VN_CAST(symp->nodep(), Var);
                    }
                }
            }
            𐤀𐤌 (enclosingVarp && enclosingVarp->varType() == VVarType::IFACEREF
                && VN_IS(enclosingVarp->childDTypep()->skipRefp(), IfaceGenericDType)) {
                // Nested generic-iface forwarding (#7454): enclosing port is itself still
                // generic, so emit a placeholder __VGIfaceParam pin carrying a VarRef to
                // the outer port. V3Param rewrites it to the concrete IfaceRefDType once
                // the enclosing module is specialized (see
                // ParamProcessor::resolveGenericIfaceForwardingPins for the ordering
                // constraint that keeps the rewrite inside V3Param).
                AstVarRef* ثابت fwdRefp
                    = جديد AstVarRef{exprp->fileline(), enclosingVarp, VAccess::READ};
                AstPin* ثابت newPinp = جديد AstPin{
                    pinp->fileline(), paramNum, "__VGIfaceParam" + modIfaceVarp->name(), fwdRefp};
                newPinp->param(true);
                visit(newPinp);
                nodep->addParamsp(newPinp);
            } وإلا 𐤀𐤌 (varRefp) {
                ثابت AstVar* ثابت varp = varRefp->varp();
                𐤀𐤌 (ثابت AstIfaceRefDType* ثابت refp
                    = VN_CAST(varp->childDTypep()->elemDTypep(), IfaceRefDType)) {
                    AstIface* ثابت ifacep = VN_AS(refp->cellp()->modp(), Iface);
                    AstIfaceRefDType* newIfaceRefp;
                    𐤀𐤌 (refp->modportp()) {
                        newIfaceRefp = جديد AstIfaceRefDType{
                            refp->fileline(), refp->modportFileline(), m_modp->name(),
                            ifacep->name(), refp->modportName()};
                    } وإلا {
                        newIfaceRefp = جديد AstIfaceRefDType{refp->fileline(), m_modp->name(),
                                                            ifacep->name()};
                    }
                    newIfaceRefp->ifacep(ifacep);
                    𐤀𐤌 (refp->cellp()->paramsp()) {
                        newIfaceRefp->addParamsp(refp->cellp()->paramsp()->cloneTree(true));
                    }
                    UASSERT_OBJ(pinp->name().find("__pinNumber") == 0
                                    || pinp->name() == modIfaceVarp->name(),
                                pinp, "Not found interface with such name");
                    AstPin* ثابت newPinp
                        = جديد AstPin{pinp->fileline(), paramNum,
                                     "__VGIfaceParam" + modIfaceVarp->name(), newIfaceRefp};
                    newPinp->param(true);
                    visit(newPinp);
                    nodep->addParamsp(newPinp);
                } وإلا {
                    varRefp->v3error("Generic interfaces can only connect to an interface and "
                                     << varp->prettyNameQ() << " is "
                                     << (varp->childDTypep()
                                             ? "of type " + varp->childDTypep()->prettyDTypeNameQ()
                                             : "not an interface"));
                }
            } وإلا {
                exprp->v3error("Expected an interface but " << exprp->prettyNameQ()
                                                            << " is not an interface");
            }
            ++paramNum;
        }
    }

    // Returns true and issues error iff 'varp' cannot be accesed with a VarXRef
    // in a hier_block given by 'scopeSymp'
    منطقي errorHierNonPort(AstVarXRef* refp, AstVar* varp, VSymEnt* scopeSymp) {
        // OK to access ports on hier_block
        𐤀𐤌 (varp->isIO()) أرجع خطأ;
        // OK if not an instance
        ثابت AstCell* ثابت cellp = VN_CAST(scopeSymp->nodep(), Cell);
        𐤀𐤌 (!cellp) أرجع خطأ;
        // Ok if not a hier_block
        ثابت AstNodeModule* ثابت modp = cellp->modp();
        𐤀𐤌 (!modp->hierBlock()) أرجع خطأ;
        // Bad
        refp->v3error("Cannot access non-port symbols inside hierarchical block");
        أرجع true;
    }

#define LINKDOT_VISIT_START() \
    VL_RESTORER(m_indent); \
    ++m_indent;

    string indent() ثابت {
        string result = "";
        result.insert(0, m_indent, ':');
        أرجع result + " ";
    }

    // VISITORS
    فراغ visit(AstNetlist* nodep) override {
        // Recurse..., backward as must do packages before using packages
        iterateChildrenBackwardsConst(nodep);
    }
    فراغ visit(AstTypeTable*) override {}
    فراغ visit(AstNodeModule* nodep) override {
        𐤀𐤌 (nodep->dead()) أرجع;
        LINKDOT_VISIT_START();
        UINFO(8, indent() << "visit " << nodep);
        VL_RESTORER(m_genericIfaceModule);
        𐤀𐤌 (ثابت AstModule* ثابت modp = VN_CAST(nodep, Module)) {
            m_genericIfaceModule = modp->hasGenericIface();
        }
        checkNoDot(nodep);
        m_ds.init(m_curSymp);
        m_ds.m_dotSymp = m_curSymp = m_modSymp
            = m_statep->getNodeSym(nodep);  // Until overridden by a SCOPE
        m_cellp = nullptr;
        m_modp = nodep;
        m_modportNum = 0;
        iterateChildren(nodep);
        m_modp = nullptr;
        m_ds.m_dotSymp = m_curSymp = m_modSymp = nullptr;
        m_ds.m_dotPos = DP_NONE;
        m_ds.m_dotErr = خطأ;
    }
    فراغ visit(AstScope* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(8, indent() << "visit " << nodep);
        checkNoDot(nodep);
        VL_RESTORER(m_modSymp);
        VL_RESTORER(m_curSymp);
        m_ds.m_dotSymp = m_curSymp = m_modSymp = m_statep->getScopeSym(nodep);
        iterateChildren(nodep);
        m_ds.m_dotSymp = m_curSymp = m_modSymp = nullptr;
    }
    فراغ visit(AstCellInline* nodep) override {
        LINKDOT_VISIT_START();
        checkNoDot(nodep);
        UASSERT_OBJ(m_statep->forScopeCreation(), nodep, "CellInline should not exist yet");
        VL_DO_DANGLING(pushDeletep(nodep->unlinkFrBack()), nodep);
    }
    فراغ visit(AstCell* nodep) override {
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
            𐤀𐤌 (VN_IS(nodep->modp(), NotFoundModule)) {
                // Prevent warnings about missing pin connects
                𐤀𐤌 (nodep->pinsp()) nodep->pinsp()->unlinkFrBackWithNext()->deleteTree();
                𐤀𐤌 (nodep->paramsp()) nodep->paramsp()->unlinkFrBackWithNext()->deleteTree();
            }
            // Need to pass the module info to this cell, so we can link up the pin names
            // However can't use m_curSymp as pin connections need to use the
            // instantiator's symbols
            وإلا {
                m_pinSymp = m_statep->getNodeSym(nodep->modp());
                UINFO(4, indent() << "(Backto) visit " << nodep);
                // UINFOTREE(1, nodep, "", "linkcell");
                // UINFOTREE(1, nodep->modp(), "", "linkcemd");
                iterateChildren(nodep);

                𐤀𐤌 (m_statep->forPrimary())
                    𐤀𐤌 (ثابت AstModule* ثابت modp = VN_CAST(nodep->modp(), Module)) {
                        𐤀𐤌 (modp->hasGenericIface())
                            addImplicitParametersOfGenericIface(nodep, modp);
                    }
            }
        }
        // Parent module inherits child's publicity
        // This is done bottom up in the LinkBotupVisitor stage
    }
    فراغ visit(AstClassRefDType* nodep) override {
        // Cell: Recurse inside or cleanup not founds
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        // Can be under dot if called as package::class and that class resolves, so no checkNoDot
        VL_RESTORER_CLEAR(m_usedPins);
        VL_RESTORER_CLEAR(m_usedDefParamPins);
        UASSERT_OBJ(nodep->classp(), nodep, "ClassRef has unlinked class");
        UASSERT_OBJ(m_statep->forPrimary() || !nodep->paramsp() || V3Error::errorCount(), nodep,
                    "class reference parameter not removed by V3Param");
        VL_RESTORER(m_pinSymp);
        {
            // ClassRef's have pins, so track
            m_pinSymp = m_statep->getNodeSym(nodep->classp());
            UINFO(4, indent() << "(Backto) visit " << nodep);
            // UINFOTREE(1, nodep, "", "linkcell");
            // UINFOTREE(1, nodep->modp(), "", "linkcemd");
            iterateChildren(nodep);
        }
    }
    فراغ visit(AstPin* nodep) override {
        // Pin: Link to submodule's port
        LINKDOT_VISIT_START();
        checkNoDot(nodep);
        iterateChildren(nodep);
        𐤀𐤌 (!nodep->modVarp()) {
            UASSERT_OBJ(m_pinSymp, nodep, "Pin not under instance?");
            VSymEnt* ثابت foundp = m_pinSymp->findIdFlat(nodep->name());
            ثابت حرف* ثابت whatp = nodep->param() ? "parameter" : "pin";
            𐤀𐤌 (!foundp) {
                𐤀𐤌 (m_cellp && VN_IS(m_cellp->modp(), Primitive)
                    && (nodep->name() == "__paramNumber1" || nodep->name() == "__paramNumber2")) {
                    // Primitive parameter is really a delay2 we can just ignore
                    VL_DO_DANGLING(pushDeletep(nodep->unlinkFrBack()), nodep);
                    أرجع;
                } وإلا 𐤀𐤌 (nodep->param() && !nodep->exprp() && !nodep->svDotName()) {
                    // Placeholder pin the parser makes for an empty '#()', to signal
                    // the user did type the '#()'. No further use, deleting.
                    VL_DO_DANGLING(pushDeletep(nodep->unlinkFrBack()), nodep);
                    أرجع;
                } وإلا {
                    ثابت std::string suggest
                        = (nodep->param()
                               ? m_statep->suggestSymFlat(m_pinSymp, nodep->name(),
                                                          LinkNodeMatcherVarParam{}, خطأ)
                               : m_statep->suggestSymFlat(m_pinSymp, nodep->name(),
                                                          LinkNodeMatcherVarIO{}, خطأ));
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
                    VL_DO_DANGLING(pushDeletep(nodep->unlinkFrBack()), nodep);
                    أرجع;
                }
            }
            VVarType refVarType = VVarType::UNKNOWN;
            منطقي wrongPinType = خطأ;
            𐤀𐤌 (AstVar* ثابت varp = VN_CAST(foundp->nodep(), Var)) {
                𐤀𐤌 (varp->isIO() || varp->isParam() || varp->isIfaceRef()) {
                    refVarType = varp->varType();
                    nodep->modVarp(varp);
                } وإلا {
                    wrongPinType = true;
                }
            } وإلا 𐤀𐤌 (AstParamTypeDType* ثابت typep = VN_CAST(foundp->nodep(), ParamTypeDType)) {
                refVarType = typep->varType();
                nodep->modPTypep(typep);
            } وإلا {
                wrongPinType = true;
            }
            // Don't connect parameter pin to module ports or vice versa
            𐤀𐤌 (nodep->param() != (refVarType == VVarType::GPARAM)) wrongPinType = true;
            𐤀𐤌 (wrongPinType) {
                string targetType = LinkDotState::nodeTextType(foundp->nodep());
                targetType = VString::aOrAn(targetType) + ' ' + targetType;
                𐤀𐤌 (nodep->param()) {
                    nodep->v3error("Instance attempts to override "
                                   << nodep->prettyNameQ() << " as a " << whatp << ", but it is "
                                   << targetType);
                } وإلا {
                    nodep->v3error("Instance attempts to connect to "
                                   << nodep->prettyNameQ() << ", but it is " << targetType);
                }
                أرجع;
            }
            markAndCheckPinDup(nodep, foundp->nodep(), whatp);
        }
        // Early return() above when deleted
    }

    ساكن منطقي isDotThisReference(ثابت AstDot* nodep) {
        أرجع VN_IS(nodep->lhsp(), ParseRef) && nodep->lhsp()->name() == "this";
    }

    فراغ visit(AstDot* nodep) override {
        // Legal under a DOT: AstDot, AstParseRef, AstPackageRef, AstNodeSel, AstPackArrayDType
        //    also a DOT can be part of an expression, but only above plus
        //    AstFTaskRef are legal children
        // Dot(PackageRef, ParseRef(text))
        // Dot(Dot(ClassOrPackageRef,ClassOrPackageRef), ParseRef(text))
        // Dot(Dot(Dot(ParseRef(text), ...
        𐤀𐤌 (nodep->user3SetOnce()) أرجع;
        LINKDOT_VISIT_START();
        UINFO(8, indent() << "visit " << nodep);
        UINFO(8, indent() << m_ds.ascii());
        ثابت DotStates lastStates = m_ds;
        ثابت منطقي start = (m_ds.m_dotPos == DP_NONE);  // Save, as m_dotp will be changed
        ثابت DotPosition initialDotPos = m_ds.m_dotPos;
        VL_RESTORER(m_randSymp);
        {
            𐤀𐤌 (start) {  // Starting dot sequence
                UINFOTREE(9, nodep, "", "dot-in");
                // Keep deferral from visit(AstMethodCall) for unspecialized rand target
                m_ds.init(m_curSymp, m_ds.m_unresolvedClass && randTargetUnresolved());
            }
            m_ds.m_dotp = nodep;  // Always, not just at start
            m_ds.m_dotPos = DP_FIRST;

            𐤀𐤌 (isDotThisReference(nodep)) {
                VSymEnt* classSymp = getThisClassSymp();
                // In 'randomize() with { this.member }', 'this' refers to randomized
                // object, not the calling class (IEEE 1800-2023 18.7)
                𐤀𐤌 (m_randSymp && m_currentWithp) classSymp = m_randSymp;
                𐤀𐤌 (!classSymp && randTargetUnresolved()) {
                    // Bind 'this' in post-V3Param pass
                } وإلا 𐤀𐤌 (!classSymp) {
                    nodep->v3error("'this' used outside class (IEEE 1800-2023 8.11)");
                    m_ds.m_dotErr = true;
                } وإلا {
                    m_ds.m_dotSymp = classSymp;
                    UINFO(8, indent() << "this. " << m_ds.ascii());
                }
            } وإلا 𐤀𐤌 (VN_IS(nodep->lhsp(), ParseRef) && nodep->lhsp()->name() == "super") {
                ثابت VSymEnt* classSymp = getThisClassSymp();
                𐤀𐤌 (!classSymp) {
                    nodep->v3error("'super' used outside class (IEEE 1800-2023 8.15)");
                    m_ds.m_dotErr = true;
                } وإلا {
                    ثابت auto classp = VN_AS(classSymp->nodep(), Class);
                    ثابت auto cextp = classp->extendsp();
                    𐤀𐤌 (!cextp) {
                        nodep->v3error("'super' used on non-extended class (IEEE 1800-2023 8.15)");
                        m_ds.m_dotErr = true;
                    } وإلا {
                        𐤀𐤌 (m_statep->forPrimary()
                            && m_extendsParam.find(classp) != m_extendsParam.end()) {
                            UINFO(9, indent()
                                         << "deferring until post-V3Param: " << nodep->lhsp());
                            m_ds.m_unresolvedClass = true;
                        } وإلا {
                            ثابت auto baseClassp = cextp->classp();
                            UASSERT_OBJ(baseClassp, nodep, "Bad superclass");
                            m_ds.m_dotSymp = m_statep->getNodeSym(baseClassp);
                            m_ds.m_super = true;
                            UINFO(8, indent() << "super. " << m_ds.ascii());
                        }
                    }
                }
            } وإلا 𐤀𐤌 (AstClassOrPackageRef* ثابت lhsp
                       = VN_CAST(nodep->lhsp(), ClassOrPackageRef)) {
                // m_ds.m_dotText communicates the cell prefix between stages
                UINFO(8, indent() << "iter.lhs   " << m_ds.ascii() << " " << nodep);
                iterateAndNextNull(lhsp);
                𐤀𐤌 (!lhsp->classOrPackageSkipp() && lhsp->name() != "local::") {
                    revisitLater(nodep);
                    m_ds = lastStates;
                    // Resolve function args before bailing
                    𐤀𐤌 (AstNodeFTaskRef* ثابت ftaskrefp = VN_CAST(nodep->rhsp(), NodeFTaskRef)) {
                        iterateAndNextNull(ftaskrefp->argsp());
                    }
                    أرجع;
                }
                m_ds.m_dotPos = DP_PACKAGE;
                // nodep->lhsp() may be a new node
                𐤀𐤌 (AstClassOrPackageRef* ثابت classOrPackageRefp
                    = VN_CAST(nodep->lhsp(), ClassOrPackageRef)) {
                    𐤀𐤌 (AstNode* ثابت classOrPackageNodep
                        = classOrPackageRefp->classOrPackageSkipp()) {
                        m_ds.m_dotSymp = m_statep->getNodeSym(classOrPackageNodep);
                    }
                }
                UINFO(8, indent() << "iter.ldone " << m_ds.ascii() << " " << nodep);
            } وإلا 𐤀𐤌 (VN_IS(nodep->lhsp(), Dot) && VN_AS(nodep->lhsp(), Dot)->colon()) {
                // m_ds.m_dotText communicates the cell prefix between stages
                UINFO(8, indent() << "iter.lhs   " << m_ds.ascii() << " " << nodep);
                m_ds.m_dotPos = DP_PACKAGE;
                iterateAndNextNull(nodep->lhsp());
                // nodep->lhsp() may be a new node
                𐤀𐤌 (AstClassOrPackageRef* ثابت crefp
                    = VN_CAST(nodep->lhsp(), ClassOrPackageRef)) {
                    𐤀𐤌 (!crefp->classOrPackageSkipp()) {
                        revisitLater(nodep);
                        m_ds = lastStates;
                        // Resolve function args before bailing
                        𐤀𐤌 (AstNodeFTaskRef* ثابت ftaskrefp
                            = VN_CAST(nodep->rhsp(), NodeFTaskRef)) {
                            iterateAndNextNull(ftaskrefp->argsp());
                        }
                        أرجع;
                    }
                }
                𐤀𐤌 (m_lastDeferredp == nodep->lhsp()) {
                    // LHS got deferred, so this node won't be resolved. Defer it too
                    m_ds = lastStates;
                    // Resolve function args before bailing
                    𐤀𐤌 (AstNodeFTaskRef* ثابت ftaskrefp = VN_CAST(nodep->rhsp(), NodeFTaskRef)) {
                        iterateAndNextNull(ftaskrefp->argsp());
                    }
                    أرجع;
                }
                UINFO(8, indent() << "iter.ldone " << m_ds.ascii() << " " << nodep);
            } وإلا {
                m_ds.m_dotPos = DP_FIRST;
                m_ds.m_unresolvedGenericIface = خطأ;
                UINFO(8, indent() << "iter.lhs   " << m_ds.ascii() << " " << nodep);
                iterateAndNextNull(nodep->lhsp());
                UINFO(8, indent() << "iter.ldone " << m_ds.ascii() << " " << nodep);
                𐤀𐤌 (m_ds.m_unresolvedGenericIface) {
                    m_ds.m_dotPos = initialDotPos;
                    أرجع;
                }
                // UINFOTREE(9, nodep, "", "dot-lho");
            }
            𐤀𐤌 (m_statep->forPrimary() && isParamedClassRef(nodep->lhsp())) {
                // Dots of paramed classes will be linked after deparameterization
                UINFO(9, indent() << "deferring until post-V3Param: " << nodep->lhsp());
                m_ds.m_unresolvedClass = true;
            }
            𐤀𐤌 (m_ds.m_unresolvedCell
                && (VN_IS(nodep->lhsp(), CellRef) || VN_IS(nodep->lhsp(), CellArrayRef))) {
                m_ds.m_unlinkedScopep = nodep->lhsp();
            }
            𐤀𐤌 (m_ds.m_dotPos == DP_FIRST) m_ds.m_dotPos = DP_SCOPE;
            𐤀𐤌 (!m_ds.m_dotErr) {  // Once something wrong, give up
                // Top 'final' dot RHS is final RHS, else it's a
                // DOT(DOT(x,*here*),real-rhs) which we consider a RHS
                𐤀𐤌 (start && m_ds.m_dotPos == DP_SCOPE) m_ds.m_dotPos = DP_FINAL;
                UINFO(8, indent() << "iter.rhs   " << m_ds.ascii() << " " << nodep);
                // m_ds.m_dotSymp points at lhsp()'s symbol table, so resolve RHS under that
                iterateAndNextNull(nodep->rhsp());
                UINFO(8, indent() << "iter.rdone " << m_ds.ascii() << " " << nodep);
                // UINFOTREE(9, nodep, "", "dot-rho");
            }
            𐤀𐤌 (!m_ds.m_unresolvedClass) {
                𐤀𐤌 (start) {
                    AstNode* newp;
                    𐤀𐤌 (m_ds.m_dotErr) {
                        newp = جديد AstConst{nodep->fileline(), AstConst::BitFalse{}};
                    } وإلا {
                        newp = nodep->rhsp()->unlinkFrBack();
                    }
                    UINFOTREE(9, newp, "", "dot-out");
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(pushDeletep(nodep), nodep);
                } وإلا {  // Dot midpoint
                    AstNode* newp = nodep->rhsp()->unlinkFrBack();
                    𐤀𐤌 (m_ds.m_unresolvedCell) {
                        AstCellRef* ثابت crp = جديد AstCellRef{
                            nodep->fileline(), nodep->name(), nodep->lhsp()->unlinkFrBack(), newp};
                        newp = crp;
                    }
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(pushDeletep(nodep), nodep);
                }
            }
        }
        𐤀𐤌 (start) {
            m_ds = lastStates;
        } وإلا {
            ثابت منطقي unresolvedClass = m_ds.m_unresolvedClass;
            m_ds.m_dotp = lastStates.m_dotp;
            m_ds.m_unresolvedClass |= unresolvedClass;
        }
        UINFO(8, indent() << "done " << m_ds.ascii() << " " << nodep);
    }
    فراغ visit(AstDefaultClocking* nodep) override {
        𐤀𐤌 (VSymEnt* ثابت foundp = m_curSymp->findIdFallback(nodep->name())) {
            𐤀𐤌 (AstClocking* ثابت clockingp = VN_CAST(foundp->nodep(), Clocking)) {
                clockingp->makeDefault();
                VL_DO_DANGLING(nodep->unlinkFrBack()->deleteTree(), nodep);
            } وإلا {
                nodep->v3error(nodep->prettyNameQ() << " is not a clocking identifier");
            }
        } وإلا {
            nodep->v3error("Can't find definition of clocking: " << nodep->prettyNameQ());
        }
    }
    فراغ visit(AstSenItem* nodep) override {
        LINKDOT_VISIT_START();
        VL_RESTORER(m_inSens);
        m_inSens = true;
        iterateChildren(nodep);
    }
    فراغ visit(AstPatMember* nodep) override {
        LINKDOT_VISIT_START();
        𐤀𐤌 (nodep->varrefp()) أرجع;  // only do this mapping once
        // If we have a TEXT token as our key, lookup if it's a LPARAM
        𐤀𐤌 (AstText* ثابت textp = VN_CAST(nodep->keyp(), Text)) {
            UINFO(9, indent() << "visit " << nodep);
            UINFO(9, indent() << "      " << textp);
            // Lookup
            𐤀𐤌 (VSymEnt* ثابت foundp = m_curSymp->findIdFallback(textp->text())) {
                𐤀𐤌 (AstVar* ثابت varp = VN_CAST(foundp->nodep(), Var)) {
                    𐤀𐤌 (varp->isParam() || varp->isGenVar()) {
                        // Attach found Text reference to PatMember
                        nodep->varrefp(
                            جديد AstVarRef{nodep->fileline(),
                                          foundp->imported() ? foundp->classOrPackagep() : nullptr,
                                          varp, VAccess::READ});
                        UINFO(9, indent() << " new " << nodep->varrefp());
                    }
                }
                𐤀𐤌 (AstEnumItem* ثابت itemp = VN_CAST(foundp->nodep(), EnumItem)) {
                    // Attach enum item value to PatMember
                    nodep->varrefp(
                        جديد AstEnumItemRef{nodep->fileline(), itemp, foundp->classOrPackagep()});
                    UINFO(9, indent() << " new " << itemp);
                }
            }
        }
        iterateChildren(nodep);
    }
    فراغ visit(AstParseRef* nodep) override {
        𐤀𐤌 (nodep->user3SetOnce()) أرجع;
        LINKDOT_VISIT_START();
        UINFO(9, indent() << "visit " << nodep);
        UINFO(9, indent() << m_ds.ascii());
        𐤀𐤌 (m_ds.m_unresolvedClass) أرجع;
        // m_curSymp is symbol table of outer expression
        // m_ds.m_dotSymp is symbol table relative to "."'s above now
        UASSERT_OBJ(m_ds.m_dotSymp, nodep, "nullptr lookup symbol table");
        // Generally resolved during Primary, but might be at param time under AstUnlinkedRef
        UASSERT_OBJ(!m_statep->forScopeCreation(), nodep, "ParseRefs should no longer exist");
        ثابت DotStates lastStates = m_ds;
        ثابت منطقي start = (m_ds.m_dotPos == DP_NONE);  // Save, as m_dotp will be changed
        منطقي first = start || m_ds.m_dotPos == DP_FIRST;

        𐤀𐤌 (start) {
            m_ds.init(m_curSymp);
            // Note m_ds.m_dotp remains nullptr; this is a reference not under a dot
        }
        𐤀𐤌 (nodep->name() == "super") {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: super");
            m_ds.m_dotErr = true;
        }
        𐤀𐤌 (nodep->name() == "this") {
            iterateChildren(nodep);
            𐤀𐤌 (m_statep->forPrimary()) أرجع;  // The class might be parameterized somewhere
            ثابت VSymEnt* classSymp = getThisClassSymp();
            𐤀𐤌 (!classSymp) {
                nodep->v3error("'this' used outside class (IEEE 1800-2023 8.11)");
                أرجع;
            }
            AstClass* ثابت classp = VN_AS(classSymp->nodep(), Class);
            AstClassRefDType* ثابت dtypep
                = جديد AstClassRefDType{nodep->fileline(), classp, nullptr};
            AstThisRef* ثابت newp = جديد AstThisRef{nodep->fileline(), VFlagChildDType{}, dtypep};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
            أرجع;
        }
        𐤀𐤌 (m_ds.m_dotPos == DP_MEMBER && VN_IS(m_ds.m_dotp->lhsp(), LambdaArgRef)
            && nodep->name() == "index") {
            // 'with' statement's 'item.index'
            // m_ds.dotp->lhsp() was checked to know if `index` is directly after lambda arg ref.
            // If not, treat it as normal member select
            iterateChildren(nodep);
            AstLambdaArgRef* ثابت newp = جديد AstLambdaArgRef{
                nodep->fileline(), m_ds.m_dotp->lhsp()->name() + "__DOT__index", true};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
            أرجع;
        } وإلا 𐤀𐤌 (m_ds.m_dotPos == DP_MEMBER) {
            // Found a Var, everything following is membership.  {scope}.{var}.HERE {member}
            𐤀𐤌 (m_ds.m_dotErr) {
                nodep->unlinkFrBack();  // Avoid circular node loop on errors
            } وإلا {
                AstNodeExpr* ثابت varEtcp = VN_AS(m_ds.m_dotp->lhsp()->unlinkFrBack(), NodeExpr);
                AstNodeExpr* ثابت newp = جديد AstMemberSel{nodep->fileline(), varEtcp,  //
                                                           VFlagChildDType{}, nodep->name()};
                nodep->replaceWith(newp);
            }
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
        } وإلا {
            //
            string expectWhat;
            منطقي allowScope = خطأ;
            منطقي allowVar = خطأ;
            منطقي allowFTask = خطأ;
            منطقي staticAccess = خطأ;
            𐤀𐤌 (m_ds.m_disablep) {
                allowScope = true;
                allowFTask = true;
                expectWhat = "block/task";
            } وإلا 𐤀𐤌 (m_ds.m_dotPos == DP_PACKAGE) {
                // {package-or-class}::{a}
                AstNodeModule* classOrPackagep = nullptr;
                expectWhat = "scope/variable/func";
                allowScope = true;
                allowVar = true;
                allowFTask = true;
                staticAccess = true;
                UINFO(9, "chk pkg " << m_ds.ascii() << " lhsp=" << m_ds.m_dotp->lhsp());
                UASSERT_OBJ(VN_IS(m_ds.m_dotp->lhsp(), ClassOrPackageRef), m_ds.m_dotp->lhsp(),
                            "Bad package link");
                AstClassOrPackageRef* ثابت cpackagerefp
                    = VN_AS(m_ds.m_dotp->lhsp(), ClassOrPackageRef);
                𐤀𐤌 (cpackagerefp->name() == "local::") {
                    m_randSymp = nullptr;
                    first = true;
                } وإلا 𐤀𐤌 (!cpackagerefp->classOrPackageSkipp()) {
                    VSymEnt* ثابت foundp = m_statep->resolveClassOrPackage(
                        m_ds.m_dotSymp, cpackagerefp, true, خطأ, ":: reference");
                    𐤀𐤌 (!foundp) أرجع;
                    classOrPackagep = cpackagerefp->classOrPackageSkipp();
                    𐤀𐤌 (classOrPackagep) {
                        m_ds.m_dotSymp = m_statep->getNodeSym(classOrPackagep);
                    } وإلا {
                        m_ds = lastStates;
                        أرجع;
                    }
                } وإلا {
                    classOrPackagep = cpackagerefp->classOrPackageSkipp();
                    UASSERT_OBJ(classOrPackagep, m_ds.m_dotp->lhsp(), "Bad package link");
                    m_ds.m_dotSymp = m_statep->getNodeSym(classOrPackagep);
                }
                m_ds.m_dotPos = DP_SCOPE;
            } وإلا 𐤀𐤌 (m_ds.m_dotPos == DP_SCOPE || m_ds.m_dotPos == DP_FIRST) {
                // {a}.{b}, where {a} maybe a module name
                // or variable, where dotting into structure member
                expectWhat = "scope/variable";
                allowScope = true;
                allowVar = true;
            } وإلا 𐤀𐤌 (m_ds.m_dotPos == DP_NONE) {
                expectWhat = "variable";
                allowVar = true;
            } وإلا 𐤀𐤌 (m_ds.m_dotPos == DP_FINAL) {
                expectWhat = "variable/method";
                allowFTask = true;
                allowVar = true;
            } وإلا {
                UINFO(1, "ds=" << m_ds.ascii());
                nodep->v3fatalSrc("Unhandled DotPosition");
            }
            // Lookup
            VSymEnt* foundp;
            string baddot;
            VSymEnt* okSymp = nullptr;
            // Restricted 'with (id_list)': only id_list names bind into target class.
            𐤀𐤌 (m_randSymp
                && (!m_currentWithp || m_currentWithp->nameResolvesToTarget(nodep->name()))) {
                foundp = m_randSymp->findIdFlat(nodep->name());
                𐤀𐤌 (foundp) {
                    𐤀𐤌 (m_currentWithp && m_currentWithp->restricted()) {
                        m_restrictedNamesUsed.insert(nodep->name());
                    }
                    𐤀𐤌 (!start) m_ds.m_dotPos = DP_MEMBER;
                    𐤀𐤌 (!m_currentWithp) {
                        UASSERT_OBJ(m_randMethodCallp, nodep, "Expected to be under randomize()");
                        // This will start failing once complex expressions are allowed on the LHS
                        // of randomize() with args
                        UASSERT_OBJ(VN_IS(m_randMethodCallp->fromp(), VarRef), m_randMethodCallp,
                                    "Expected simple randomize target");
                        // A ParseRef is used here so that the dot RHS gets resolved
                        AstMemberSel* ثابت newp = جديد AstMemberSel{
                            nodep->fileline(), m_randMethodCallp->fromp()->cloneTree(خطأ),
                            VFlagChildDType{}, nodep->name()};
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(pushDeletep(nodep), nodep);
                        أرجع;
                    }
                    UINFO(9, indent() << "randomize-with fromSym " << foundp->nodep());
                    AstLambdaArgRef* ثابت lambdaRefp
                        = جديد AstLambdaArgRef{nodep->fileline(), "__Vrandwith_obj", خطأ};
                    AstMemberSel* newp = جديد AstMemberSel{nodep->fileline(), lambdaRefp,
                                                          VFlagChildDType{}, nodep->name()};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(pushDeletep(nodep), nodep);
                    أرجع;
                }
            }
            𐤀𐤌 (allowScope) {
                foundp = m_statep->findDotted(nodep->fileline(), m_ds.m_dotSymp, nodep->name(),
                                              baddot, okSymp, first);  // Maybe nullptr
            } وإلا 𐤀𐤌 (first) {
                foundp = m_ds.m_dotSymp->findIdFallback(nodep->name());
            } وإلا {
                foundp = m_ds.m_dotSymp->findIdFlat(nodep->name());
            }
            𐤀𐤌 (!foundp && m_ds.m_dotp && isDotThisReference(m_ds.m_dotp)) {
                ثابت AstClass* ثابت classp = VN_CAST(m_ds.m_dotSymp->nodep(), Class);
                𐤀𐤌 (classp && classp->isCovergroup() && classp->covergroupEnclosingClassp()) {
                    VSymEnt* ثابت enclosingClassSymp
                        = m_statep->getNodeSym(classp->covergroupEnclosingClassp());
                    foundp = enclosingClassSymp->findIdFallback(nodep->name());
                    𐤀𐤌 (foundp) m_ds.m_dotSymp = enclosingClassSymp;
                }
            }
            // If not found in modport, check interface fallback for parameters and typedefs.
            // Parameters and typedefs are always visible through a modport (IEEE 1800-2023 25.5).
            // This mirrors the VarXRef modport parameter fallback in visit(AstVarXRef).
            𐤀𐤌 (!foundp && VN_IS(m_ds.m_dotSymp->nodep(), Modport)
                && m_ds.m_dotSymp->fallbackp()) {
                VSymEnt* ثابت ifaceFallbackp = m_ds.m_dotSymp->fallbackp();
                VSymEnt* ثابت ifaceFoundp = ifaceFallbackp->findIdFlat(nodep->name());
                𐤀𐤌 (ifaceFoundp) {
                    𐤀𐤌 (ثابت AstVar* ثابت varp = VN_CAST(ifaceFoundp->nodep(), Var)) {
                        𐤀𐤌 (varp->isParam()) foundp = ifaceFoundp;
                    } وإلا 𐤀𐤌 (VN_IS(ifaceFoundp->nodep(), Typedef)
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
            𐤀𐤌 (!foundp && !okSymp && VN_IS(m_ds.m_dotSymp->nodep(), Modport)) {
                okSymp = m_ds.m_dotSymp;
            }
            𐤀𐤌 (foundp) {
                UINFO(9, indent() << "found=se" << cvtToHex(foundp) << "  exp=" << expectWhat
                                  << "  n=" << foundp->nodep());
            }
            // What fell out?
            منطقي ok = خطأ;
            // Special case: waiting on clocking event
            𐤀𐤌 (m_inSens && foundp && m_ds.m_dotPos != DP_SCOPE && m_ds.m_dotPos != DP_FIRST) {
                𐤀𐤌 (AstClocking* ثابت clockingp = sensClockingp(foundp->nodep())) {
                    foundp = getCreateClockingEventSymEnt(clockingp);
                }
            }
            𐤀𐤌 (!foundp) {
            } وإلا 𐤀𐤌 (VN_IS(foundp->nodep(), Cell) || VN_IS(foundp->nodep(), NodeBlock)
                       || VN_IS(foundp->nodep(), GenBlock)
                       || VN_IS(foundp->nodep(), Netlist)  // for $root
                       || VN_IS(foundp->nodep(), Module)) {  // if top
                𐤀𐤌 (allowScope) {
                    ok = true;
                    m_ds.m_dotText = VString::dot(m_ds.m_dotText, ".", nodep->name());
                    m_ds.m_dotSymp = foundp;
                    m_ds.m_dotPos = DP_SCOPE;
                    𐤀𐤌 (m_ds.m_disablep && VN_IS(foundp->nodep(), NodeBlock)) {
                        // Possibly it is not the final link. If we are under dot and not in its
                        // last component, `targetp()` field will be overwritten by next components
                        m_ds.m_disablep->targetp(foundp->nodep());
                    }
                    𐤀𐤌 (VN_IS(foundp->nodep(), GenBlock)) {
                        m_ds.m_genBlk = true;
                        𐤀𐤌 (m_ds.m_disablep) {
                            m_ds.m_disablep->v3warn(
                                E_UNSUPPORTED,
                                "Unsupported: Generate block referenced by disable");
                        }
                    }
                    // Upper AstDot visitor will handle it from here
                } وإلا 𐤀𐤌 (VN_IS(foundp->nodep(), Cell) && allowVar) {
                    AstCell* ثابت cellp = VN_AS(foundp->nodep(), Cell);
                    𐤀𐤌 (VN_IS(cellp->modp(), Iface)) {
                        // Interfaces can be referenced like a variable for interconnect
                        VSymEnt* ثابت cellEntp = m_statep->getNodeSym(cellp);
                        UASSERT_OBJ(cellEntp, nodep, "No interface sym entry");
                        VSymEnt* ثابت parentEntp
                            = cellEntp->parentp();  // Container of the var; probably a module or
                                                    // generate begin
                        AstVar* ثابت ifaceRefVarp
                            = findIfaceTopVarp(nodep, parentEntp, nodep->name());
                        //
                        ok = true;
                        m_ds.m_dotSymp = foundp;
                        m_ds.m_dotPos = DP_SCOPE;
                        UINFO(9, indent() << " cell -> iface varref " << foundp->nodep());
                        AstNodeExpr* newp;
                        𐤀𐤌 (m_ds.m_dotText != "") {
                            newp = جديد AstVarXRef{nodep->fileline(), ifaceRefVarp, m_ds.m_dotText,
                                                  VAccess::READ};
                            𐤀𐤌 (m_ds.m_unresolvedCell && m_ds.m_unlinkedScopep) {
                                newp = جديد AstUnlinkedRef{nodep->fileline(), newp, newp->name(),
                                                          m_ds.m_unlinkedScopep->unlinkFrBack()};
                                m_ds.m_unlinkedScopep = nullptr;
                                m_ds.m_unresolvedCell = خطأ;
                                m_ds.m_dotText = "";
                            }
                        } وإلا {
                            AstVarRef* ثابت refp
                                = جديد AstVarRef{nodep->fileline(), ifaceRefVarp, VAccess::READ};
                            newp = refp;
                        }
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(pushDeletep(nodep), nodep);
                    } وإلا 𐤀𐤌 (VN_IS(cellp->modp(), NotFoundModule)) {
                        cellp->modNameFileline()->v3error("Cannot find file containing interface: "
                                                          << cellp->modp()->prettyNameQ());
                    }
                }
            } وإلا 𐤀𐤌 (allowScope && !allowFTask && VN_IS(foundp->nodep(), NodeFTask)) {
                // Function/task used as scope for static variable access (IEEE std 1800-2017 6.21)
                // Don't set m_dotText so the static variable resolves as a VarRef rather than
                // VarXRef. This is so that V3Begin won't move it out of the function/task's scope.
                ok = true;
                m_ds.m_dotSymp = foundp;
                m_ds.m_dotPos = DP_SCOPE;
            } وإلا 𐤀𐤌 (allowFTask && VN_IS(foundp->nodep(), NodeFTask)) {
                AstNodeFTaskRef* taskrefp;
                𐤀𐤌 (VN_IS(foundp->nodep(), Task)) {
                    taskrefp = جديد AstTaskRef{nodep->fileline(), nodep->name()};
                } وإلا {
                    taskrefp = جديد AstFuncRef{nodep->fileline(), nodep->name()};
                }
                nodep->replaceWith(taskrefp);
                VL_DO_DANGLING(pushDeletep(nodep), nodep);
                m_ds = lastStates;
                أرجع;
            } وإلا 𐤀𐤌 (AstVar* ثابت varp = foundToVarp(foundp, nodep, VAccess::READ)) {
                // Preserve explicit 'this' references to members inherited from
                // generic classes until after specialization.
                𐤀𐤌 (m_statep->forPrimary() && m_ds.m_dotp && isDotThisReference(m_ds.m_dotp)
                    && foundp->classOrPackagep() != m_modp
                    && VN_IS(foundp->classOrPackagep(), Class)
                    && foundp->classOrPackagep()->hasGParam()) {
                    m_ds.m_unresolvedClass = true;
                    أرجع;
                }
                AstIfaceRefDType* ثابت ifacerefp
                    = LinkDotState::ifaceRefFromArray(varp->subDTypep());
                𐤀𐤌 (varp->isIfaceRef() && m_genericIfaceModule
                    && VN_IS(varp->childDTypep(), IfaceGenericDType) && !start) {
                    // Defer only dotted member access ('d.PARAM'), as V3Param must specialize
                    // first; a standalone ref ('.x(d)') resolves via allowVar below now
                    ok = true;
                    m_ds.m_unresolvedGenericIface = true;
                } وإلا 𐤀𐤌 (ifacerefp && varp->isIfaceRef()) {
                    UASSERT_OBJ(ifacerefp->ifaceViaCellp(), ifacerefp, "Unlinked interface");
                    // Really this is a scope reference into an interface
                    UINFO(9, indent() << "varref-ifaceref " << m_ds.m_dotText << "  " << nodep);
                    m_ds.m_dotText = VString::dot(m_ds.m_dotText, ".", nodep->name());
                    // If modport specified, use modport symbol table for lookup of virtual ports
                    𐤀𐤌 (ifacerefp->modportp() && m_statep->existsNodeSym(ifacerefp->modportp())) {
                        m_ds.m_dotSymp = m_statep->getNodeSym(ifacerefp->modportp());
                    } وإلا {
                        m_ds.m_dotSymp = m_statep->getNodeSym(ifacerefp->ifaceViaCellp());
                    }
                    m_ds.m_dotPos = DP_SCOPE;
                    ok = true;

                    AstVarRef* ثابت refp = جديد AstVarRef{nodep->fileline(), varp, VAccess::READ};
                    nodep->replaceWith(refp);

                    VL_DO_DANGLING(pushDeletep(nodep), nodep);
                } وإلا 𐤀𐤌 (allowVar) {
                    AstNode* newp;
                    𐤀𐤌 (m_ds.m_dotText != "") {
                        AstVarXRef* ثابت refp
                            = جديد AstVarXRef{nodep->fileline(), nodep->name(), m_ds.m_dotText,
                                             VAccess::READ};  // lvalue'ness computed later
                        refp->varp(varp);
                        // Not linked again after V3LinkLValue, so that reports writing it
                        𐤀𐤌 (ثابت AstModportVarRef* ثابت mvarp
                            = VN_CAST(foundp->nodep(), ModportVarRef)) {
                            𐤀𐤌 (m_statep->forParamed() && mvarp->direction().isReadOnly()) {
                                refp->readOnlyModport(true);
                            }
                        }
                        refp->containsGenBlock(m_ds.m_genBlk);
                        m_ds.m_dotText = "";
                        𐤀𐤌 (m_ds.m_unresolvedCell && m_ds.m_unlinkedScopep) {
                            ثابت string dotted = refp->dotted();
                            ثابت size_t pos = dotted.find("__BRA__??__KET__");
                            // Arrays of interfaces all have the same parameters
                            𐤀𐤌 (pos != string::npos && varp->isParam()
                                && VN_IS(m_ds.m_unlinkedScopep, CellArrayRef)) {
                                refp->dotted(dotted.substr(0, pos));
                                newp = refp;
                            } وإلا {
                                UINFO(9, indent() << "deferring until post-V3Param: " << refp);
                                newp = جديد AstUnlinkedRef{nodep->fileline(), refp, refp->name(),
                                                          m_ds.m_unlinkedScopep->unlinkFrBack()};
                                m_ds.m_unlinkedScopep = nullptr;
                                m_ds.m_unresolvedCell = خطأ;
                            }
                        } وإلا {
                            newp = refp;
                        }
                    } وإلا {
                        𐤀𐤌 (staticAccess && !varp->lifetime().isStatic() && !varp->isParam()) {
                            // TODO bug4077
                            // nodep->v3error("Static access to non-static member variable "
                            //                << varp->prettyNameQ() << endl);
                        }
                        AstVarRef* ثابت refp = جديد AstVarRef{
                            nodep->fileline(), varp, VAccess::READ};  // lvalue'ness computed later
                        refp->classOrPackagep(foundp->classOrPackagep());
                        newp = refp;
                    }
                    UINFO(9, indent() << "new " << newp);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(pushDeletep(nodep), nodep);
                    m_ds.m_dotPos = DP_MEMBER;
                    ok = true;
                }
            } وإلا 𐤀𐤌 (ثابت AstModport* ثابت modportp = VN_CAST(foundp->nodep(), Modport)) {
                // A scope reference into an interface's modport (not
                // necessarily at a pin connection)
                UINFO(9, indent() << "cell-ref-to-modport " << m_ds.m_dotText << "  " << nodep);
                UINFO(9, indent() << "unlinked " << m_ds.m_unlinkedScopep);
                UINFO(9,
                      indent() << "dotSymp " << m_ds.m_dotSymp << " " << m_ds.m_dotSymp->nodep());
                // Iface was the previously dotted component
                𐤀𐤌 (!m_ds.m_dotSymp || !VN_IS(m_ds.m_dotSymp->nodep(), Cell)
                    || !VN_AS(m_ds.m_dotSymp->nodep(), Cell)->modp()
                    || !VN_IS(VN_AS(m_ds.m_dotSymp->nodep(), Cell)->modp(), Iface)) {
                    nodep->v3error("Modport not referenced as <interface>."
                                   << modportp->prettyNameQ());
                } وإلا 𐤀𐤌 (!VN_AS(m_ds.m_dotSymp->nodep(), Cell)->modp()
                           || !VN_IS(VN_AS(m_ds.m_dotSymp->nodep(), Cell)->modp(), Iface)) {
                    nodep->v3error("Modport not referenced from underneath an interface: "
                                   << modportp->prettyNameQ());
                } وإلا {
                    AstCell* ثابت cellp = VN_AS(m_ds.m_dotSymp->nodep(), Cell);
                    UASSERT_OBJ(cellp, nodep, "Modport not referenced from an instance");
                    VSymEnt* ثابت cellEntp = m_statep->getNodeSym(cellp);
                    UASSERT_OBJ(cellEntp, nodep, "No interface sym entry");
                    VSymEnt* ثابت parentEntp
                        = cellEntp->parentp();  // Container of the var; probably a
                                                // module or generate begin
                    // We drop __BRA__??__KET__ as cells don't have that naming yet
                    AstVar* ثابت ifaceRefVarp
                        = findIfaceTopVarp(nodep, parentEntp, cellp->name());
                    //
                    ok = true;
                    m_ds.m_dotText = VString::dot(m_ds.m_dotText, ".", nodep->name());
                    m_ds.m_dotSymp = foundp;
                    m_ds.m_dotPos = DP_SCOPE;
                    UINFO(9, indent() << "modport -> iface varref " << foundp->nodep());
                    // We lose the modport name here, so we cannot detect mismatched modports.
                    AstVarRef* ثابت refp
                        = جديد AstVarRef{nodep->fileline(), ifaceRefVarp, VAccess::READ};
                    AstNodeExpr* newp = refp;

                    auto* ثابت cellarrayrefp = VN_CAST(m_ds.m_unlinkedScopep, CellArrayRef);
                    𐤀𐤌 (cellarrayrefp) {
                        // iface[vec].modport became CellArrayRef(iface, lsb)
                        // Convert back to SelBit(iface, lsb)
                        UINFO(9, indent() << "Array modport to SelBit " << cellarrayrefp);
                        newp = جديد AstSelBit{cellarrayrefp->fileline(), newp,
                                             cellarrayrefp->selp()->unlinkFrBack()};
                        newp->user3(true);  // Don't process again
                        VL_DO_DANGLING(pushDeletep(cellarrayrefp->unlinkFrBack()), cellarrayrefp);
                        m_ds.m_unlinkedScopep = nullptr;
                    }
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(pushDeletep(nodep), nodep);
                }
            } وإلا 𐤀𐤌 (AstConstraint* ثابت defp = VN_CAST(foundp->nodep(), Constraint)) {
                AstNode* ثابت newp = جديد AstConstraintRef{nodep->fileline(), nullptr, defp};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(pushDeletep(nodep), nodep);
                ok = true;
                m_ds.m_dotPos = DP_MEMBER;
                m_ds.m_dotText = "";
            } وإلا 𐤀𐤌 (AstClass* ثابت defp = VN_CAST(foundp->nodep(), Class)) {
                𐤀𐤌 (allowVar) {
                    𐤀𐤌 (m_ds.m_dotPos == DP_SCOPE && !staticAccess) {
                        // Continue hierarchical lookup in the class scope.
                        m_ds.m_dotSymp = foundp;
                    } وإلا {
                        AstRefDType* ثابت newp
                            = جديد AstRefDType{nodep->fileline(), nodep->name()};
                        replaceWithCheckBreak(nodep, newp);
                        VL_DO_DANGLING(pushDeletep(nodep), nodep);
                    }
                    ok = true;
                    m_ds.m_dotText = "";
                } وإلا {
                    (فراغ)defp;  // Prevent unused variable warning
                }
            } وإلا 𐤀𐤌 (AstMemberDType* ثابت defp = VN_CAST(foundp->nodep(), MemberDType)) {
                𐤀𐤌 (allowVar) {
                    checkMemberDeclOrder(nodep, defp);
                    AstRefDType* ثابت refp = جديد AstRefDType{nodep->fileline(), nodep->name()};
                    refp->refDTypep(defp);
                    replaceWithCheckBreak(nodep, refp);
                    VL_DO_DANGLING(pushDeletep(nodep), nodep);
                    ok = true;
                    m_ds.m_dotText = "";
                }
            } وإلا 𐤀𐤌 (AstEnumItem* ثابت valuep = VN_CAST(foundp->nodep(), EnumItem)) {
                𐤀𐤌 (allowVar) {
                    AstEnumItemRef* ثابت refp
                        = جديد AstEnumItemRef{nodep->fileline(), valuep, foundp->classOrPackagep()};
                    AstNode* newp = refp;
                    // Hierarchical reference, relinked after V3Param as the referenced
                    // module may be specialized, similar to AstVarXRef
                    refp->dotted(m_ds.m_dotText);
                    refp->containsGenBlock(m_ds.m_genBlk);
                    𐤀𐤌 (m_ds.m_unresolvedCell && m_ds.m_unlinkedScopep) {
                        UINFO(9, indent() << "deferring until post-V3Param: " << refp);
                        newp = جديد AstUnlinkedRef{nodep->fileline(), refp, refp->name(),
                                                  m_ds.m_unlinkedScopep->unlinkFrBack()};
                        m_ds.m_unlinkedScopep = nullptr;
                        m_ds.m_unresolvedCell = خطأ;
                    }
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(pushDeletep(nodep), nodep);
                    ok = true;
                    m_ds.m_dotText = "";
                }
            } وإلا 𐤀𐤌 (ثابت AstLambdaArgRef* ثابت argrefp
                       = VN_CAST(foundp->nodep(), LambdaArgRef)) {
                𐤀𐤌 (allowVar) {
                    AstNode* ثابت newp
                        = جديد AstLambdaArgRef{nodep->fileline(), argrefp->name(), خطأ};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(pushDeletep(nodep), nodep);
                    ok = true;
                    m_ds.m_dotPos = DP_MEMBER;
                    m_ds.m_dotText = "";
                }
            } وإلا 𐤀𐤌 (VN_IS(foundp->nodep(), Clocking)) {
                m_ds.m_dotSymp = foundp;
                𐤀𐤌 (m_ds.m_dotText != "") m_ds.m_dotText += "." + nodep->name();
                ok = m_ds.m_dotPos == DP_SCOPE || m_ds.m_dotPos == DP_FIRST;
            } وإلا 𐤀𐤌 (ثابت AstModportClockingRef* ثابت clockingRefp
                       = VN_CAST(foundp->nodep(), ModportClockingRef)) {
                // Clocking block accessed through a modport - redirect to actual clocking
                m_ds.m_dotSymp = m_statep->getNodeSym(clockingRefp->clockingp());
                𐤀𐤌 (m_ds.m_dotText != "") m_ds.m_dotText += "." + nodep->name();
                ok = m_ds.m_dotPos == DP_SCOPE || m_ds.m_dotPos == DP_FIRST;
            } وإلا 𐤀𐤌 (ثابت AstNodeFTask* ثابت ftaskp = VN_CAST(foundp->nodep(), NodeFTask)) {
                𐤀𐤌 (!ftaskp->isFunction() || ftaskp->classMethod()) {
                    ok = m_ds.m_dotPos == DP_NONE;
                    𐤀𐤌 (ok) {
                        // The condition is true for tasks,
                        // properties and void functions.
                        // In these cases, the parentheses may be skipped.
                        // Also SV class methods can be called without parens
                        AstFuncRef* ثابت funcRefp
                            = جديد AstFuncRef{nodep->fileline(), nodep->name()};
                        nodep->replaceWith(funcRefp);
                        VL_DO_DANGLING(pushDeletep(nodep), nodep);
                    }
                }
            } وإلا 𐤀𐤌 (AstTypedef* ثابت defp = VN_CAST(foundp->nodep(), Typedef)) {
                ثابت منطقي ifaceFinalSegmentAllowed
                    = (m_ds.m_dotPos == DP_FINAL) && m_ds.m_dotSymp
                      && (VN_IS(m_ds.m_dotSymp->nodep(), Modport)
                          || (VN_IS(m_ds.m_dotSymp->nodep(), Cell)
                              && VN_CAST(m_ds.m_dotSymp->nodep(), Cell)->modp()
                              && VN_IS(VN_CAST(m_ds.m_dotSymp->nodep(), Cell)->modp(), Iface)));
                // Allow interface typedef references even without IfaceCapture
                // The dependency graph handles resolution when IfaceCapture is disabled
                ok = (m_ds.m_dotPos == DP_NONE || m_ds.m_dotPos == DP_SCOPE
                      || ifaceFinalSegmentAllowed);
                𐤀𐤌 (ifaceFinalSegmentAllowed) {
                    UINFO(9, indent() << "allow final-segment typedef name="
                                      << nodep->prettyNameQ() << " dotText='" << m_ds.m_dotText
                                      << "' dotSym=" << m_ds.m_dotSymp);
                }
                𐤀𐤌 (ok) {
                    AstRefDType* ثابت refp = جديد AstRefDType{nodep->fileline(), nodep->name()};
                    // Don't check if typedef is to a <type T>::<reference> as might not be
                    // resolved yet
                    𐤀𐤌 (m_ds.m_dotPos == DP_NONE) checkDeclOrder(nodep, defp);
                    refp->typedefp(defp);

                    V3LinkDotIfaceCapture::captureTypedefContext(
                        refp, "typedef", static_cast<صحيح>(m_ds.m_dotPos), m_ds.m_dotText,
                        m_ds.m_dotSymp, m_modp, [هذا]() { أرجع indent(); });

                    𐤀𐤌 (VN_IS(nodep->backp(), SelExtract)) {
                        m_packedArrayDtp = refp;
                    } وإلا {
                        replaceWithCheckBreak(nodep, refp);
                        VL_DO_DANGLING(pushDeletep(nodep), nodep);
                    }
                }
            } وإلا 𐤀𐤌 (AstParamTypeDType* ثابت defp = VN_CAST(foundp->nodep(), ParamTypeDType)) {
                ok = (m_ds.m_dotPos == DP_NONE || m_ds.m_dotPos == DP_SCOPE
                      || m_ds.m_dotPos == DP_FINAL);
                𐤀𐤌 (ok) {
                    AstRefDType* ثابت refp = جديد AstRefDType{nodep->fileline(), nodep->name()};
                    refp->refDTypep(defp);

                    V3LinkDotIfaceCapture::captureTypedefContext(
                        refp, "paramtype", static_cast<صحيح>(m_ds.m_dotPos), m_ds.m_dotText,
                        m_ds.m_dotSymp, m_modp, [هذا]() { أرجع indent(); });

                    𐤀𐤌 (VN_IS(nodep->backp(), SelExtract)) {
                        m_packedArrayDtp = refp;
                    } وإلا {
                        replaceWithCheckBreak(nodep, refp);
                        VL_DO_DANGLING(pushDeletep(nodep), nodep);
                    }
                }
            }
            𐤀𐤌 (!ok) {
                𐤀𐤌 (m_insideClassExtParam) {
                    // Don't throw error if the reference is inside a class that extends a param,
                    // because some members can't be linked in such a case. m_insideClassExtParam
                    // may be true only in the first stage of linking.
                    // Mark that the Dot statement can't be resolved.
                    m_ds.m_unresolvedClass = true;
                    // If the symbol was a scope name, it would be resolved.
                    m_ds.m_dotPos = DP_MEMBER;
                } وإلا {
                    // Cells/interfaces can't be implicit
                    ثابت منطقي checkImplicit
                        = (!m_ds.m_dotp && m_ds.m_dotText == "" && !m_ds.m_disablep && !foundp);
                    ثابت منطقي err
                        = !(checkImplicit && m_statep->implicitOk(m_modp, nodep->name()));
                    𐤀𐤌 (err) {
                        𐤀𐤌 (foundp) {
                            nodep->v3error("Found definition of '"
                                           << m_ds.m_dotText << (m_ds.m_dotText == "" ? "" : ".")
                                           << nodep->prettyName() << "'" << " as a "
                                           << foundp->nodep()->typeName() << " but expected a "
                                           << expectWhat);
                        } وإلا 𐤀𐤌 (m_ds.m_dotText == "") {
                            UINFO(1, "   ErrParseRef curSymp=se" << cvtToHex(m_curSymp)
                                                                 << " ds=" << m_ds.ascii());
                            ثابت string suggest = m_statep->suggestSymFallback(
                                m_ds.m_dotSymp, nodep->name(), VNodeMatcher{});
                            nodep->v3error(
                                "Can't find definition of "
                                << expectWhat << ": " << nodep->prettyNameQ() << '\n'
                                << (suggest.empty() ? "" : nodep->warnMore() + suggest));
                        } وإلا {
                            nodep->v3error(
                                "Can't find definition of "
                                << (!baddot.empty() ? AstNode::prettyNameQ(baddot)
                                                    : nodep->prettyNameQ())
                                << " in dotted " << expectWhat << ": '"
                                << m_ds.m_dotText + "." + nodep->prettyName() << "'\n"
                                << nodep->warnContextPrimary()
                                << (okSymp ? okSymp->cellErrorScopes(
                                                 nodep, AstNode::prettyName(m_ds.m_dotText))
                                           : ""));
                        }
                        m_ds.m_dotErr = true;
                    }
                    𐤀𐤌 (checkImplicit) {
                        // Create if implicit, and also if error (so only complain once)
                        // Else if a scope is allowed, making a signal won't help error cascade
                        AstVar* ثابت varp
                            = createImplicitVar(m_curSymp, nodep, m_modp, m_modSymp, err);
                        AstVarRef* ثابت newp
                            = جديد AstVarRef{nodep->fileline(), varp, VAccess::READ};
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(pushDeletep(nodep), nodep);
                    }
                }
            }
        }
        𐤀𐤌 (start) m_ds = lastStates;
    }
    فراغ visit(AstClassOrPackageRef* nodep) override {
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

            𐤀𐤌 (!nodep->classOrPackageSkipp() && !nodep->classOrPackageNodep()
                && nodep->name() != "local::") {
                ثابت منطقي deferIfUnresolved = m_statep->forPrimary() && m_insideClassExtParam;
                m_statep->resolveClassOrPackage(m_ds.m_dotSymp, nodep, m_ds.m_dotPos != DP_PACKAGE,
                                                خطأ, ":: reference", deferIfUnresolved);
            }

            // ClassRef's have pins, so track
            𐤀𐤌 (nodep->classOrPackageSkipp()) {
                m_pinSymp = m_statep->getNodeSym(nodep->classOrPackageSkipp());
            } وإلا 𐤀𐤌 (nodep->name() != "local::") {
                أرجع;
            }

            AstClass* ثابت refClassp = VN_CAST(nodep->classOrPackageSkipp(), Class);
            // Make sure any extends() are properly imported within referenced class
            𐤀𐤌 (refClassp && !m_statep->forPrimary()) classExtendImport(refClassp);

            𐤀𐤌 (m_statep->forPrimary() && refClassp && !nodep->paramsp()
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
                AstPin* ثابت pinp = جديد AstPin{nodep->fileline(), 1, "__paramNumber1", nullptr};
                pinp->param(true);
                nodep->addParamsp(pinp);
            }

            m_ds.init(m_curSymp);
            UINFO(4, indent() << "(Backto) Link ClassOrPackageRef: " << nodep);
            iterateChildren(nodep);
        }

        𐤀𐤌 (nodep->name() == "local::") {
            𐤀𐤌 (!m_randSymp && !m_randMethodCallp) {
                nodep->v3error("Illegal 'local::' outside 'randomize() with'"
                               " (IEEE 1800-2023 18.7.1)");
                m_ds.m_dotErr = true;
                أرجع;
            }
        }
        𐤀𐤌 (m_ds.m_dotPos == DP_PACKAGE && nodep->classOrPackageSkipp()) {
            m_ds.m_dotSymp = m_statep->getNodeSym(nodep->classOrPackageSkipp());
            UINFO(9, indent() << "set sym " << m_ds.ascii());
        }
    }
    فراغ visit(AstConstraint* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        𐤀𐤌 (nodep->isExternDef()) {
            𐤀𐤌 (ثابت VSymEnt* ثابت foundp
                = m_curSymp->findIdFallback("extern " + nodep->name())) {
                ثابت AstConstraint* ثابت protop = VN_AS(foundp->nodep(), Constraint);
                // Copy specifiers.
                // External definition cannot have any specifiers, so no value will be overwritten.
                nodep->isStatic(protop->isStatic());
            } وإلا {
                nodep->v3error("extern not found that declares " + nodep->prettyNameQ());
            }
        }
        𐤀𐤌 (nodep->isExternProto()) {
            𐤀𐤌 (!m_curSymp->findIdFallback(nodep->name()) && nodep->isExternExplicit()) {
                nodep->v3warn(PROTOTYPEMIS, "Definition not found for extern prototype "
                                                << nodep->prettyNameQ());
            }
        }
        VL_RESTORER(m_curSymp);
        VL_RESTORER_COPY(m_ds);
        m_ds.m_dotSymp = m_curSymp = m_statep->getNodeSym(nodep);
        iterateChildren(nodep);
    }
    فراغ visit(AstConstraintRef* nodep) override {
        𐤀𐤌 (nodep->user3SetOnce()) أرجع;
        LINKDOT_VISIT_START();
        UINFO(8, indent() << "visit " << nodep);
        UINFO(8, indent() << m_ds.ascii());
        // No children defined
    }
    فراغ visit(AstVarRef* nodep) override {
        // VarRef: Resolve its reference
        // ParseRefs are used the first pass (forPrimary) so we shouldn't get can't find
        // errors here now that we have a VarRef.
        // No checkNoDot; created and iterated from a parseRef
        LINKDOT_VISIT_START();
        iterateChildren(nodep);
        𐤀𐤌 (!nodep->varp()) {
            UINFO(9, indent() << "linkVarRef se" << cvtToHex(m_curSymp) << "  n=" << nodep);
            UASSERT_OBJ(m_curSymp, nodep, "nullptr lookup symbol table");
            VSymEnt* ثابت foundp = m_curSymp->findIdFallback(nodep->name());
            𐤀𐤌 (AstVar* ثابت varp
                = foundp ? foundToVarp(foundp, nodep, nodep->access()) : nullptr) {
                nodep->varp(varp);
                updateVarUse(nodep->varp());
                // Generally set by parse, but might be an import
                nodep->classOrPackagep(foundp->classOrPackagep());
            }
            𐤀𐤌 (VL_UNCOVERABLE(!nodep->varp())) {
                nodep->v3error("Can't find definition of signal, again: "  // LCOV_EXCL_LINE
                               << nodep->prettyNameQ());
            }
        }
        AstVarScope* vscp = nodep->varScopep();
        𐤀𐤌 (vscp && vscp->user2p() != vscp && m_replaceWithAlias) {
            vscp = LinkDotScopeVisitor::getAlias(vscp);
            nodep->varp(vscp->varp());
            nodep->varScopep(vscp);
            updateVarUse(nodep->varp());
            UINFO(7, indent() << "Resolved " << nodep);  // Also prints taskp
        }
    }
    فراغ visit(AstVarXRef* nodep) override {
        // VarRef: Resolve its reference
        // We always link even if varp() is set, because the module we choose may change
        // due to creating new modules, flattening, etc.
        𐤀𐤌 (nodep->user3SetOnce()) أرجع;
        LINKDOT_VISIT_START();
        UINFO(8, indent() << "visit " << nodep);
        // No checkNoDot; created and iterated from a parseRef
        𐤀𐤌 (!m_modSymp) {
            // Module that is not in hierarchy.  We'll be dead code eliminating it later.
            UINFO(9, "Dead module for " << nodep);
            nodep->varp(nullptr);
        } وإلا {
            VSymEnt* lookupSymp = m_curSymp;  // Start search at current scope
            𐤀𐤌 (nodep->inlinedDots() != "") {  // Correct for current post-inlined scope
                lookupSymp = findInlinedDotsSym(nodep, nodep->inlinedDots());
            }
            string baddot;
            VSymEnt* okSymp;
            منطقي modport;
            VSymEnt* ثابت dotSymp
                = findDottedRefSymp(nodep, lookupSymp, nodep->dotted(), baddot, okSymp, &modport);
            𐤀𐤌 (!dotSymp) أرجع;

            𐤀𐤌 (!m_statep->forScopeCreation()) {
                VSymEnt* foundp = nullptr;
                𐤀𐤌 (modport) {
                    // This is a copy of findSymPrefixed with few modifications.
                    // This searches without a fallback like findSymPrefixed but if lookup fails it
                    // will check one fallback (fallback of modport shall be interface) and take
                    // only L and G PARAMs since they should be visible from a modport always.
                    string prefix = dotSymp->symPrefix();
                    طالما (!foundp) {
                        foundp = dotSymp->findIdFlat(prefix + nodep->name());
                        𐤀𐤌 (foundp) اكسر;
                        UASSERT_OBJ(dotSymp->fallbackp(), nodep, "Modports shall have fallback");
                        foundp = dotSymp->fallbackp()->findIdFlat(prefix + nodep->name());
                        𐤀𐤌 (ثابت AstVar* ثابت varp
                            = foundToVarp(foundp, nodep, nodep->access())) {
                            𐤀𐤌 (!varp->isParam()) foundp = nullptr;
                        }
                        𐤀𐤌 (prefix.empty()) اكسر;
                        string nextPrefix = LinkDotState::removeLastInlineScope(prefix);
                        𐤀𐤌 (prefix == nextPrefix) اكسر;
                        prefix = std::move(nextPrefix);
                    }
                    baddot = nodep->name();
                } وإلا {
                    foundp = m_statep->findSymPrefixed(dotSymp, nodep->name(), baddot, true);
                }
                𐤀𐤌 (m_inSens && foundp) {
                    𐤀𐤌 (AstClocking* ثابت clockingp = sensClockingp(foundp->nodep())) {
                        foundp = getCreateClockingEventSymEnt(clockingp);
                    }
                }
                AstVar* ثابت varp
                    = foundp ? foundToVarp(foundp, nodep, nodep->access()) : nullptr;
                nodep->varp(varp);
                updateVarUse(nodep->varp());
                UINFO(7, indent() << "Resolved " << nodep);  // Also prints varp
                // If found, check if it's ok to access in case it's in a hier_block
                𐤀𐤌 (nodep->varp() && errorHierNonPort(nodep, nodep->varp(), dotSymp)) أرجع;
                𐤀𐤌 (!nodep->varp()) {
                    nodep->v3error("Can't find definition of "
                                   << AstNode::prettyNameQ(baddot) << " in dotted signal: '"
                                   << nodep->dotted() + "." + nodep->prettyName() << "'\n"
                                   << nodep->warnContextPrimary()
                                   << okSymp->cellErrorScopes(nodep));
                    أرجع;
                }
            } وإلا {
                VSymEnt* ثابت foundp
                    = m_statep->findSymPrefixed(dotSymp, nodep->name(), baddot, true);
                AstVarScope* vscp = foundp ? VN_CAST(foundp->nodep(), VarScope) : nullptr;
                // Handle modport expression virtual ports
                𐤀𐤌 (!vscp) vscp = findVirtualPortVarScope(dotSymp, nodep->name());
                // If found, check if it's ok to access in case it's in a hier_block
                𐤀𐤌 (vscp && errorHierNonPort(nodep, vscp->varp(), dotSymp)) أرجع;
                𐤀𐤌 (!vscp) {
                    nodep->v3error("Can't find varpin scope of "
                                   << AstNode::prettyNameQ(baddot) << " in dotted signal: '"
                                   << nodep->dotted() + "." + nodep->prettyName() << "'\n"
                                   << nodep->warnContextPrimary()
                                   << okSymp->cellErrorScopes(nodep));
                } وإلا {
                    𐤀𐤌 (vscp->user2p() && m_replaceWithAlias) {
                        vscp = LinkDotScopeVisitor::getAlias(vscp);
                    }
                    // Convert the VarXRef to a VarRef, so we don't need
                    // later optimizations to deal with VarXRef.
                    nodep->varp(vscp->varp());
                    nodep->varScopep(vscp);
                    updateVarUse(nodep->varp());
                    UINFO(7, indent() << "Resolved " << nodep);  // Also prints taskp
                    AstVarRef* ثابت newvscp
                        = جديد AstVarRef{nodep->fileline(), vscp, nodep->access()};
                    nodep->replaceWith(newvscp);
                    VL_DO_DANGLING(pushDeletep(nodep), nodep);
                    UINFO(9, indent() << "new " << newvscp);  // Also prints taskp
                }
            }
        }
    }
    فراغ visit(AstEnumDType* nodep) override {
        LINKDOT_VISIT_START();
        iterateChildren(nodep);
        AstRefDType* ثابت refdtypep = VN_CAST(nodep->subDTypep(), RefDType);
        𐤀𐤌 (refdtypep && (nodep == refdtypep->subDTypep())) {
            refdtypep->v3error("Self-referential enumerated type definition");
        }
    }
    فراغ visit(AstNodeUOrStructDType* nodep) override {
        LINKDOT_VISIT_START();
        symIterateChildren(nodep, m_statep->getNodeSym(nodep));
    }
    فراغ visit(AstEnumItemRef* nodep) override {
        // Resolve its reference
        // EnumItemRefs are created by the first pass, but V3Param may regenerate due to
        // a parameterized class/module, so we shouldn't get can't find errors.
        // No checkNoDot; created and iterated from a parseRef
        𐤀𐤌 (nodep->user3SetOnce()) أرجع;
        LINKDOT_VISIT_START();
        𐤀𐤌 (m_statep->forParamed() && !nodep->dotted().empty() && m_modSymp) {
            // Hierarchical reference, relink as V3Param may have specialized the module
            string baddot;
            VSymEnt* okSymp = nullptr;
            VSymEnt* ثابت dotSymp
                = findDottedRefSymp(nodep, m_curSymp, nodep->dotted(), baddot, okSymp);
            𐤀𐤌 (!dotSymp) أرجع;
            VSymEnt* ثابت foundp
                = m_statep->findSymPrefixed(dotSymp, nodep->name(), baddot, true);
            AstEnumItem* ثابت itemp = foundp ? VN_CAST(foundp->nodep(), EnumItem) : nullptr;
            𐤀𐤌 (!itemp) {
                nodep->v3error("Enum item " << nodep->prettyNameQ() << " not found in "
                                            << AstNode::prettyNameQ(nodep->dotted()) << '\n'
                                            << nodep->warnContextPrimary());
                أرجع;
            }
            nodep->itemp(itemp);
            UINFO(9, indent() << " relinked " << nodep);
        }
        𐤀𐤌 (!nodep->itemp()) {
            UINFO(9, indent() << "linkEnumRef se" << cvtToHex(m_curSymp) << "  n=" << nodep);
            UASSERT_OBJ(m_curSymp, nodep, "nullptr lookup symbol table");
            VSymEnt* ثابت foundp = m_curSymp->findIdFallback(nodep->name());
            𐤀𐤌 (AstEnumItem* ثابت itemp = foundp ? VN_CAST(foundp->nodep(), EnumItem) : nullptr) {
                nodep->itemp(itemp);
                nodep->classOrPackagep(foundp->classOrPackagep());
                UINFO(9, indent() << " resolved " << nodep);
            }
            𐤀𐤌 (VL_UNCOVERABLE(!nodep->itemp())) {
                nodep->v3error("Can't find definition of enum item, again: "  // LCOV_EXCL_LINE
                               << nodep->prettyNameQ());
            }
        }
        iterateChildren(nodep);
    }
    فراغ visit(AstMethodCall* nodep) override {
        // Created here so should already be resolved.
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        VL_RESTORER_COPY(m_ds);
        VL_RESTORER(m_randSymp);
        VL_RESTORER(m_randMethodCallp);
        {
            m_ds.init(m_curSymp);
            𐤀𐤌 (nodep->name() == "randomize" && (nodep->argsp() || nodep->withp())) {
                m_randMethodCallp = nodep;
                ثابت AstNodeDType* fromDtp = nodep->fromp()->dtypep();
                𐤀𐤌 (!fromDtp) {
                    𐤀𐤌 (ثابت AstNodeVarRef* ثابت varRefp = VN_CAST(nodep->fromp(), NodeVarRef)) {
                        fromDtp = varRefp->varp()->subDTypep();
                    } وإلا {
                        fromDtp = getExprDTypep(nodep->fromp());
                    }
                    𐤀𐤌 (!fromDtp) {
                        𐤀𐤌 (nodep->withp()) {
                            nodep->v3warn(
                                E_UNSUPPORTED,
                                "Unsupported: 'randomize() with' on complex expressions");
                        } وإلا {
                            nodep->v3warn(E_UNSUPPORTED,
                                          "Unsupported: Inline random variable control with "
                                          "'randomize()' called on complex expressions");
                        }
                    }
                }
                𐤀𐤌 (m_statep->forPrimary() && isParamedClassRefDType(fromDtp)) {
                    m_ds.m_unresolvedClass = true;
                } وإلا 𐤀𐤌 (fromDtp) {
                    ثابت AstClassRefDType* ثابت classDtp
                        = VN_CAST(fromDtp->skipRefp(), ClassRefDType);
                    𐤀𐤌 (!classDtp)
                        nodep->v3error("'randomize() with' on a non-class-instance "
                                       << fromDtp->prettyNameQ());
                    وإلا 𐤀𐤌 (m_statep->forPrimary() && m_extendsParam.count(classDtp->classp())) {
                        // Inherited members are unavailable until the base class is specialized.
                        // Defer lookup rather than incorrectly binding names in the caller.
                        m_ds.m_unresolvedClass = true;
                    } وإلا {
                        m_randSymp = m_statep->getNodeSym(classDtp->classp());
                    }
                }
            }
            iterateChildren(nodep);
        }
    }
    فراغ visit(AstVar* nodep) override {
        LINKDOT_VISIT_START();
        checkNoDot(nodep);
        VL_RESTORER(m_isParam);
        m_isParam = nodep->varType().isParam();
        iterateChildren(nodep);
        𐤀𐤌 (m_statep->forPrimary() && nodep->isIO() && !m_ftaskp && !nodep->user4()) {
            nodep->v3error(
                "Input/output/inout does not appear in port list: " << nodep->prettyNameQ());
        }
    }
    فراغ visit(AstVarScope* nodep) override {
        LINKDOT_VISIT_START();
        checkNoDot(nodep);
        iterateChildren(nodep);
        AstVarScope* aliasp = LinkDotScopeVisitor::getAlias(nodep);
        𐤀𐤌 (aliasp && aliasp != nodep && !nodep->varp()->isIfaceRef()) {
            // Aliased variable might still be references from outside,
            // eg through the VPI, and is traced, so we need the value to propagate.
            // TODO: this means external writes to the LHS (e.g.: through the VPI) don't work
            AstAssignW* ثابت assignp = جديد AstAssignW{
                nodep->fileline(), جديد AstVarRef{nodep->fileline(), nodep, VAccess::WRITE},
                جديد AstVarRef{nodep->fileline(), aliasp, VAccess::READ}};
            assignp->user2(true);
            nodep->scopep()->addBlocksp(جديد AstAlways{assignp});
            // Propagate attributes and lint state of the replaced variable,
            // because all references to it are replaced with references to the alias variable
            aliasp->varp()->propagateAttrFrom(nodep->varp());
            aliasp->varp()->fileline()->modifyStateInherit(nodep->varp()->fileline());
        }
    }
    فراغ visit(AstNodeFTaskRef* nodep) override {
        𐤀𐤌 (nodep->user3SetOnce()) أرجع;
        LINKDOT_VISIT_START();
        UINFO(8, indent() << "visit " << nodep);
        UINFO(8, indent() << m_ds.ascii());
        𐤀𐤌 (m_ds.m_dotPos != DP_MEMBER || nodep->name() != "randomize") {
            // Visit arguments at the beginning.
            // They may be visitted even if the current node can't be linked now.
            symIterateChildren(nodep, m_curSymp);
        }

        𐤀𐤌 (m_ds.m_super) {
            𐤀𐤌 (AstFuncRef* ثابت funcRefp = VN_CAST(nodep, FuncRef)) {
                funcRefp->superReference(true);
            } وإلا 𐤀𐤌 (AstTaskRef* ثابت taskRefp = VN_CAST(nodep, TaskRef)) {
                taskRefp->superReference(true);
            }
        }

        VL_RESTORER(m_randSymp);

        منطقي first = !m_ds.m_dotp || m_ds.m_dotPos == DP_FIRST;
        منطقي staticAccess = خطأ;
        𐤀𐤌 (m_ds.m_unresolvedClass) {
            // Unable to link before V3Param
            أرجع;
        } وإلا 𐤀𐤌 (m_ds.m_unresolvedCell && m_ds.m_dotPos == DP_FINAL && m_ds.m_unlinkedScopep) {
            AstNodeFTaskRef* ثابت newftaskp = nodep->cloneTree(خطأ);
            newftaskp->dotted(m_ds.m_dotText);
            AstNode* ثابت newp = جديد AstUnlinkedRef{nodep->fileline(), newftaskp, nodep->name(),
                                                     m_ds.m_unlinkedScopep->unlinkFrBack()};
            m_ds.m_unlinkedScopep = nullptr;
            m_ds.m_unresolvedCell = خطأ;
            nodep->replaceWith(newp);
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
            أرجع;
        } وإلا 𐤀𐤌 (m_ds.m_dotp && m_ds.m_dotPos == DP_PACKAGE) {
            UASSERT_OBJ(VN_IS(m_ds.m_dotp->lhsp(), ClassOrPackageRef), m_ds.m_dotp->lhsp(),
                        "Bad package link");
            staticAccess = true;
            AstClassOrPackageRef* ثابت cpackagerefp
                = VN_AS(m_ds.m_dotp->lhsp(), ClassOrPackageRef);
            𐤀𐤌 (cpackagerefp->name() == "local::") {
                m_randSymp = nullptr;
                first = true;
            } وإلا 𐤀𐤌 (!cpackagerefp->classOrPackageSkipp()) {
                VSymEnt* ثابت foundp = m_statep->resolveClassOrPackage(
                    m_ds.m_dotSymp, cpackagerefp, true, خطأ, ":: reference");
                𐤀𐤌 (foundp) nodep->classOrPackagep(cpackagerefp->classOrPackageSkipp());
            } وإلا {
                nodep->classOrPackagep(cpackagerefp->classOrPackageSkipp());
            }
            // Class/package :: HERE function() . method_called_on_function_return_value()
            m_ds.m_dotPos = DP_MEMBER;
        } وإلا 𐤀𐤌 (m_ds.m_dotp && m_ds.m_dotPos == DP_FINAL) {
            nodep->dotted(m_ds.m_dotText);  // Maybe ""
            // Only flag FTaskRefs under generate-if/case blocks that may be
            // pruned.  GenFor and plain begin-blocks won't be pruned by V3Param.
            // A generate-case block's parent is the GenCaseItem, not the GenCase.
            // VarXRef uses the broader m_genBlk flag (set for all GenBlocks)
            // because genfor unrolling also removes variables.
            𐤀𐤌 (m_ds.m_genBlk && m_ds.m_dotSymp) {
                ثابت AstNode* ثابت blkp = m_ds.m_dotSymp->nodep();
                𐤀𐤌 (VN_IS(blkp, GenBlock)
                    && (VN_IS(blkp->backp(), GenIf) || VN_IS(blkp->backp(), GenCaseItem))) {
                    nodep->containsGenBlock(true);
                }
            }
        } وإلا 𐤀𐤌 (m_ds.m_dotp && m_ds.m_dotPos == DP_MEMBER) {
            // Found a Var, everything following is method call.
            // {scope}.{var}.HERE {method} ( ARGS )
            AstNodeExpr* ثابت varEtcp = VN_AS(m_ds.m_dotp->lhsp()->unlinkFrBack(), NodeExpr);
            AstArg* ثابت argsp = nodep->argsp();
            𐤀𐤌 (argsp) argsp->unlinkFrBackWithNext();
            AstMethodCall* ثابت newp = جديد AstMethodCall{nodep->fileline(), varEtcp,
                                                          VFlagChildDType{}, nodep->name(), argsp};
            𐤀𐤌 (AstWith* ثابت withp = nodep->withp()) newp->withp(withp->unlinkFrBack());
            nodep->replaceWith(newp);
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
            أرجع;
        } وإلا 𐤀𐤌 (m_ds.m_dotp && (m_ds.m_dotPos == DP_SCOPE || m_ds.m_dotPos == DP_FIRST)) {
            // HERE function() . method_called_on_function_return_value()
            m_ds.m_dotPos = DP_MEMBER;
            m_ds.m_dotText = "";
        } وإلا 𐤀𐤌 (!m_ds.m_disablep) {
            // visit(AstDisable*) setup the dot handling
            checkNoDot(nodep);
        }
        𐤀𐤌 (nodep->classOrPackagep() && nodep->taskp()) {
            // References into packages don't care about cell hierarchy.
        } وإلا 𐤀𐤌 (!m_modSymp) {
            // Module that is not in hierarchy.  We'll be dead code eliminating it later.
            UINFO(9, indent() << "Dead module for " << nodep);
            nodep->taskp(nullptr);
        } وإلا 𐤀𐤌 (nodep->dotted() == "" && nodep->taskp()) {
            // Earlier should have setup the links
            // Might be under a BEGIN we're not processing, so don't relink it
        } وإلا {
            string baddot;
            VSymEnt* okSymp = nullptr;
            VSymEnt* dotSymp;
            𐤀𐤌 (nodep->dotted().empty()) {
                // Non-'super.' dotted reference
                dotSymp = m_ds.m_dotSymp;
            } وإلا {
                // Start search at module, as a variable of same name under a subtask isn't a
                // relevant hit however a function under a begin/end is.  So we want begins, but
                // not the function
                dotSymp = m_curSymp;
            }
            𐤀𐤌 (nodep->classOrPackagep()) {  // Look only in specified package
                dotSymp = m_statep->getNodeSym(nodep->classOrPackagep());
                UINFO(8, indent() << "Override classOrPackage " << dotSymp);
            } وإلا {
                𐤀𐤌 (nodep->inlinedDots() != "") {  // Correct for current post-inlined scope
                    dotSymp = findInlinedDotsSym(nodep, nodep->inlinedDots());
                }
                dotSymp = m_statep->findDotted(nodep->fileline(), dotSymp, nodep->dotted(), baddot,
                                               okSymp, true);  // Maybe nullptr
            }
            // Restricted 'with (id_list)': only id_list names bind into target class.
            𐤀𐤌 (m_randSymp
                && (!m_currentWithp || m_currentWithp->nameResolvesToTarget(nodep->name()))) {
                VSymEnt* ثابت foundp = m_randSymp->findIdFlat(nodep->name());
                𐤀𐤌 (foundp && m_currentWithp) {
                    𐤀𐤌 (m_currentWithp->restricted()) {
                        m_restrictedNamesUsed.insert(nodep->name());
                    }
                    UINFO(9, indent() << "randomize-with fromSym " << foundp->nodep());
                    AstArg* argsp = nullptr;
                    𐤀𐤌 (nodep->argsp()) {
                        iterateAndNextNull(nodep->argsp());
                        argsp = nodep->argsp()->unlinkFrBackWithNext();
                    }
                    𐤀𐤌 (m_ds.m_dotPos != DP_NONE) m_ds.m_dotPos = DP_MEMBER;
                    AstNode* ثابت newp = جديد AstMethodCall{
                        nodep->fileline(),
                        جديد AstLambdaArgRef{nodep->fileline(), "__Vrandwith_obj", خطأ},
                        VFlagChildDType{}, nodep->name(), argsp};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(pushDeletep(nodep), nodep);
                    أرجع;
                }
            }
            𐤀𐤌 (first && nodep->name() == "randomize" && VN_IS(m_modp, Class)
                && !VN_IS(nodep->classOrPackagep(), Package)) {
                // need special handling to avoid falling back to std::randomize
                // Skip if classOrPackagep is a Package (i.e. std::randomize resolved earlier)
                VMemberMap memberMap;
                AstFunc* ثابت randFuncp = V3Randomize::newRandomizeFunc(
                    memberMap, VN_AS(m_modp, Class), nodep->name(), true, true);
                nodep->taskp(randFuncp);
                nodep->classOrPackagep(VN_AS(m_modp, Class));
                m_curSymp = m_statep->insertBlock(m_curSymp, nodep->name(), randFuncp, m_modp);
                // Already linked to the class randomize function; skip findSymPrefixed
                // which may find std::randomize and overwrite classOrPackagep
                أرجع;
            }
            VSymEnt* ثابت foundp
                = m_statep->findSymPrefixed(dotSymp, nodep->name(), baddot, first);
            AstNodeFTask* ثابت taskp = foundp ? VN_CAST(foundp->nodep(), NodeFTask) : nullptr;
            AstNodeModule* ثابت taskModulep = foundp ? foundp->classOrPackagep() : nullptr;
            // Ignore deferring linking of functions inside params of classes extending
            // parametric classes if referenced function can be linked in the first pass.
            𐤀𐤌 (!m_isParam || !taskp
                || (taskp->classMethod() && taskModulep && taskModulep->hasParameterList())) {
                𐤀𐤌 (m_insideClassExtParam) {
                    // The reference may point to a method declared in a super class, which is
                    // proved by a parameter. In such a case, it can't be linked at the first
                    // stage. Must not do any linking, because e.g. might find an extends of an
                    // upper class because the current class (under parent) isn't yet importing
                    // it's extended class symbols.
                    أرجع;
                }
                𐤀𐤌 (AstClass* ثابت targetClassp = VN_CAST(dotSymp->nodep(), Class)) {
                    𐤀𐤌 (m_extendsParam.count(targetClassp)) {
                        // Target class has parameterized extends not yet resolved.
                        // Its inherited symbols (e.g. static functions from the base class)
                        // aren't imported yet - defer to linkDotParamed.
                        أرجع;
                    }
                }
            }

            𐤀𐤌 (taskp) {
                𐤀𐤌 (staticAccess && !taskp->isStatic()) {
                    // TODO bug4077
                    // nodep->v3error("Static access to non-static task/function "
                    //                << taskp->prettyNameQ() << endl);
                }
                nodep->taskp(taskp);
                nodep->classOrPackagep(foundp->classOrPackagep());
                UINFO(7, indent() << "Resolved " << nodep);  // Also prints taskp
            } وإلا {
                // Note ParseRef has similar error handling/message output
                UINFO(7, indent() << "   ErrFtask curSymp=se" << cvtToHex(m_curSymp)
                                  << " dotSymp=se" << cvtToHex(dotSymp));
                𐤀𐤌 (foundp) {
                    𐤀𐤌 (VN_IS(foundp->nodep(), Var) && m_ds.m_dotText == "" && m_ftaskp
                        && m_ftaskp->name() == foundp->nodep()->name()) {
                        // This is a recursive reference to the function itself, not to the var
                        nodep->taskp(m_ftaskp);
                        nodep->classOrPackagep(foundp->classOrPackagep());
                        UINFO(7, indent() << "Resolved recursive " << nodep);  // Also prints taskp
                    } وإلا {
                        nodep->v3error("Found definition of '"
                                       << m_ds.m_dotText << (m_ds.m_dotText == "" ? "" : ".")
                                       << nodep->prettyName() << "'" << " as a "
                                       << foundp->nodep()->typeName()
                                       << " but expected a task/function");
                    }
                } وإلا 𐤀𐤌 (VN_IS(nodep, New) && !m_statep->forScopeCreation()) {
                    // Resolved in V3Width
                } وإلا 𐤀𐤌 ((nodep->name() == "pre_randomize" || nodep->name() == "post_randomize")
                           && VN_IS(dotSymp->nodep(), Class)) {
                    AstClass* ثابت classp = VN_AS(dotSymp->nodep(), Class);
                    // Functions that might be defined by user, but if not, they get empty
                    // definitions within referenced class.
                    // TODO perhaps cleaner to add to symbol table up-front with
                    // a lambda to construct them on a reference.
                    AstFunc* ثابت funcp
                        = جديد AstFunc{nodep->fileline(), nodep->name(), nullptr, nullptr};
                    funcp->classMethod(true);
                    funcp->dtypep(funcp->findVoidDType());
                    classp->addMembersp(funcp);
                    m_statep->insertBlock(dotSymp, nodep->name(), funcp, classp);
                    nodep->taskp(funcp);
                    nodep->classOrPackagep(classp);
                    UINFO(9, indent() << " Auto-made " << funcp);
                    أرجع;
                } وإلا 𐤀𐤌 (nodep->name() == "randomize" || nodep->name() == "srandom"
                           || nodep->name() == "get_randstate" || nodep->name() == "set_randstate"
                           || nodep->name() == "rand_mode" || nodep->name() == "constraint_mode") {
                    𐤀𐤌 (AstClass* ثابت classp = VN_CAST(m_modp, Class)) {
                        // Don't overwrite if already resolved to a package (e.g. std::randomize)
                        𐤀𐤌 (!VN_IS(nodep->classOrPackagep(), Package))
                            nodep->classOrPackagep(classp);
                    } وإلا 𐤀𐤌 (nodep->name() == "randomize") {
                        // A std::randomize resolved in V3Width
                        nodep->classOrPackagep(v3Global.rootp()->stdPackagep());
                    } وإلا {
                        nodep->v3error("Calling implicit class method "
                                       << nodep->prettyNameQ() << " without being under class");
                        nodep->replaceWith(جديد AstConst{nodep->fileline(), 0});
                        VL_DO_DANGLING(pushDeletep(nodep), nodep);
                        أرجع;
                    }
                } وإلا 𐤀𐤌 (nodep->dotted() == "") {
                    𐤀𐤌 (nodep->pli()) {
                        𐤀𐤌 (v3Global.opt.bboxSys()) {
                            AstNode* newp;
                            𐤀𐤌 (VN_IS(nodep, FuncRef)) {
                                newp = جديد AstConst{nodep->fileline(), AstConst::All0{}};
                            } وإلا {
                                AstNodeExpr* outp = nullptr;
                                طالما (AstArg* ثابت argp = nodep->argsp()) {
                                    outp = AstNode::addNext(outp, argp->exprp()->unlinkFrBack());
                                    VL_DO_DANGLING(pushDeletep(argp->unlinkFrBack()), argp);
                                }
                                newp = جديد AstSysIgnore{nodep->fileline(), outp};
                                newp->dtypep(nodep->dtypep());
                            }
                            nodep->replaceWith(newp);
                            VL_DO_DANGLING(pushDeletep(nodep), nodep);
                            أرجع;
                        } وإلا {
                            VSpellCheck speller;
                            V3Parse::candidatePli(&speller);
                            ثابت string suggest = speller.bestCandidateMsg(nodep->prettyName());
                            nodep->v3error(
                                "Unsupported or unknown PLI call: "
                                << nodep->prettyNameQ() << '\n'
                                << (suggest.empty() ? "" : nodep->warnMore() + suggest));
                        }
                    } وإلا {
                        ثابت string suggest = m_statep->suggestSymFallback(
                            dotSymp, nodep->name(), LinkNodeMatcherFTask{});
                        nodep->v3error("Can't find definition of task/function: "
                                       << nodep->prettyNameQ() << '\n'
                                       << (suggest.empty() ? "" : nodep->warnMore() + suggest));
                    }
                } وإلا {
                    ثابت string suggest = m_statep->suggestSymFallback(dotSymp, nodep->name(),
                                                                        LinkNodeMatcherFTask{});
                    nodep->v3error("Can't find definition of "
                                   << AstNode::prettyNameQ(baddot) << " in dotted task/function: '"
                                   << nodep->dotted() + "." + nodep->prettyName() << "'\n"
                                   << (suggest.empty() ? "" : nodep->warnMore() + suggest) << '\n'
                                   << nodep->warnContextPrimary()
                                   << okSymp->cellErrorScopes(nodep));
                }
            }
        }
    }
    فراغ visit(AstSelBit* nodep) override {
        𐤀𐤌 (nodep->user3SetOnce()) أرجع;
        LINKDOT_VISIT_START();
        iterateAndNextNull(nodep->fromp());
        𐤀𐤌 (m_ds.m_unresolvedClass) {
            UASSERT_OBJ(m_ds.m_dotPos != DP_SCOPE && m_ds.m_dotPos != DP_FIRST, nodep,
                        "Object of unresolved class on scope position in dotted reference");
            // The index does not depend on the class, and V3Param must see its class references
            symIterateNull(nodep->bitp(), m_curSymp);
            symIterateNull(nodep->attrp(), m_curSymp);
            أرجع;
        }
        𐤀𐤌 (m_ds.m_dotPos == DP_SCOPE
            || m_ds.m_dotPos
                   == DP_FIRST) {  // Already under dot, so this is {modulepart} DOT {modulepart}
            UINFO(9, indent() << "deferring until after a V3Param pass: " << nodep);
            m_ds.m_dotText += "__BRA__??__KET__";
            m_ds.m_unresolvedCell = true;
            // And pass up m_ds.m_dotText
        }
        // Pass dot state down to only fromp()
        iterateAndNextNull(nodep->fromp());
        symIterateNull(nodep->bitp(), m_curSymp);
        symIterateNull(nodep->attrp(), m_curSymp);
        𐤀𐤌 (m_ds.m_unresolvedCell && (m_ds.m_dotPos == DP_SCOPE || m_ds.m_dotPos == DP_FIRST)) {
            AstNodeExpr* ثابت exprp = nodep->bitp()->unlinkFrBack();
            𐤀𐤌 (AstCellArrayRef* ثابت fromArrRefp = VN_CAST(nodep->fromp(), CellArrayRef)) {
                // Multi-dim iface array access: append this select to the existing chain
                fromArrRefp->addSelp(exprp);
                AstCellArrayRef* ثابت movedp = fromArrRefp->unlinkFrBack();
                nodep->replaceWith(movedp);
                VL_DO_DANGLING(pushDeletep(nodep), nodep);
            } وإلا {
                AstCellArrayRef* ثابت newp
                    = جديد AstCellArrayRef{nodep->fileline(), nodep->fromp()->name(), exprp};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(pushDeletep(nodep), nodep);
            }
        }
    }
    فراغ visit(AstNodePreSel* nodep) override {
        // Excludes simple AstSelBit, see above
        𐤀𐤌 (nodep->user3SetOnce()) أرجع;
        LINKDOT_VISIT_START();
        𐤀𐤌 (m_ds.m_dotPos == DP_SCOPE
            || m_ds.m_dotPos
                   == DP_FIRST) {  // Already under dot, so this is {modulepart} DOT {modulepart}
            nodep->v3error("Syntax error: Range ':', '+:' etc are not allowed in the instance "
                           "part of a dotted reference");
            m_ds.m_dotErr = true;
            أرجع;
        }
        AstNodeDType* packedArrayDtp = nullptr;  // Datatype reference for packed array
        {
            VL_RESTORER(m_packedArrayDtp);
            iterateAndNextNull(nodep->fromp());
            symIterateNull(nodep->rhsp(), m_curSymp);
            symIterateNull(nodep->thsp(), m_curSymp);

            𐤀𐤌 (m_packedArrayDtp) {
                AstRange* ثابت newRangep
                    = جديد AstRange(nodep->fileline(), nodep->rhsp()->unlinkFrBack(),
                                   nodep->thsp()->unlinkFrBack());
                AstPackArrayDType* newArrayTypep
                    = جديد AstPackArrayDType(nodep->fileline(), m_packedArrayDtp, newRangep);
                newArrayTypep->childDTypep(m_packedArrayDtp);
                newArrayTypep->refDTypep(nullptr);
                𐤀𐤌 (VN_IS(nodep->backp(), SelExtract)) {
                    packedArrayDtp = newArrayTypep;
                } وإلا {
                    replaceWithCheckBreak(nodep, newArrayTypep);
                    VL_DO_DANGLING(pushDeletep(nodep), nodep);
                }
            } وإلا {
                𐤀𐤌 (nodep->attrp()) {
                    AstNode* ثابت attrp = nodep->attrp()->unlinkFrBack();
                    VL_DO_DANGLING(attrp->deleteTree(), attrp);
                }
                AstNode* ثابت basefromp = nodep->baseFromp(خطأ);
                𐤀𐤌 (VN_IS(basefromp, Replicate)) {
                    // From {...}[...] syntax in IEEE 2017
                    𐤀𐤌 (basefromp) UINFO(9, indent() << " Related node: " << basefromp);
                } وإلا {
                    nodep->attrp(جديد AstAttrOf{nodep->fileline(), VAttrType::VAR_BASE,
                                               basefromp->cloneTree(خطأ)});
                }
            }
        }

        𐤀𐤌 (packedArrayDtp) m_packedArrayDtp = packedArrayDtp;
    }
    فراغ visit(AstMemberSel* nodep) override {
        // checkNoDot not appropriate, can be under a dot
        LINKDOT_VISIT_START();
        iterateChildren(nodep);
    }
    فراغ visit(AstGenBlock* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        {
            VL_RESTORER(m_curSymp);
            VL_RESTORER_COPY(m_ds);
            𐤀𐤌 (nodep->name() != "") {
                m_ds.m_dotSymp = m_curSymp = m_statep->getNodeSym(nodep);
                UINFO(5, indent() << "cur=se" << cvtToHex(m_curSymp));
            }
            iterateChildren(nodep);
        }
        UINFO(5, indent() << "cur=se" << cvtToHex(m_curSymp));
    }
    فراغ visit(AstNodeBlock* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        {
            VL_RESTORER(m_curSymp);
            VL_RESTORER_COPY(m_ds);
            𐤀𐤌 (nodep->name() != "") {
                m_ds.m_dotSymp = m_curSymp = m_statep->getNodeSym(nodep);
                UINFO(5, indent() << "cur=se" << cvtToHex(m_curSymp));
            }
            iterateChildren(nodep);
        }
        UINFO(5, indent() << "cur=se" << cvtToHex(m_curSymp));
    }
    فراغ visit(AstNodeFTask* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        𐤀𐤌 (nodep->isExternDef() && !nodep->didProto()) {
            nodep->didProto(true);
            𐤀𐤌 (ثابت VSymEnt* ثابت foundp
                = m_curSymp->findIdFallback("extern " + nodep->name())) {
                ثابت AstNodeFTask* ثابت protop = VN_AS(foundp->nodep(), NodeFTask);
                // Copy specifiers.
                // External definition cannot have any specifiers, so no value will be overwritten.
                nodep->isHideLocal(protop->isHideLocal());
                nodep->isHideProtected(protop->isHideProtected());
                nodep->isStatic(protop->isStatic());
                nodep->isVirtual(protop->isVirtual());
                nodep->lifetime(protop->lifetime());
                // Always add an FUNC_RETURN_PROTO even if task, so we can locate the prototype
                nodep->addStmtsp(جديد AstAttrOf{protop->fileline(), VAttrType::FUNC_RETURN_PROTO,
                                               protop->fvarp()
                                                   ? protop->fvarp()->cloneTree(خطأ)
                                                   : جديد AstVoidDType{protop->fileline()}});
                لكل (AstNode* stmtp = protop->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    𐤀𐤌 (AstVar* ثابت portp = VN_CAST(stmtp, Var)) {
                        AstAttrOf* ثابت attrp
                            = جديد AstAttrOf{protop->fileline(), VAttrType::FUNC_ARG_PROTO,
                                            portp->subDTypep()->cloneTree(خطأ)};
                        attrp->name(portp->name());
                        nodep->addStmtsp(attrp);
                    }
                }
            } وإلا {
                nodep->v3error("extern not found that declares " + nodep->prettyNameQ());
            }
        }
        𐤀𐤌 (nodep->isExternProto()) {
            𐤀𐤌 (!m_curSymp->findIdFallback(nodep->name())) {
                nodep->v3warn(PROTOTYPEMIS,
                              "Definition not found for extern prototype " + nodep->prettyNameQ());
            }
        }
        VL_RESTORER(m_curSymp);
        VL_RESTORER(m_ftaskp);
        VL_RESTORER(m_explicitSuperNewp);
        {
            m_ftaskp = nodep;
            m_ds.m_dotSymp = m_curSymp = m_statep->getNodeSym(nodep);
            ثابت منطقي isNew = nodep->name() == "new";
            𐤀𐤌 (isNew) m_explicitSuperNewp = nullptr;
            iterateChildren(nodep);
            𐤀𐤌 (isNew) {
                ثابت AstClassExtends* ثابت classExtendsp = VN_AS(m_modp, Class)->extendsp();
                𐤀𐤌 (m_explicitSuperNewp && !m_explicitSuperNewp->isImplicit() && classExtendsp
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
                𐤀𐤌 (classExtendsp && classExtendsp->classOrNullp()) {
                    𐤀𐤌 (!m_explicitSuperNewp && m_statep->forParamed()
                        && !VN_AS(classExtendsp->childDTypep()->skipRefp(), ClassRefDType)
                                ->classp()
                                ->isInterfaceClass()) {
                        AstNodeStmt* ثابت superNewp
                            = addImplicitSuperNewCall(VN_AS(nodep, Func), classExtendsp);
                        UINFO(9, "created super new " << superNewp);
                        iterate(superNewp);
                    }
                }
            }
        }
        m_ds.m_dotSymp = VL_RESTORER_PREV(m_curSymp);
    }
    فراغ visit(AstNodeForeach* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        symIterateChildren(nodep, m_statep->getNodeSym(nodep));
    }
    فراغ visit(AstRandSequence* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        𐤀𐤌 (!nodep->start().empty())
            nodep->prodp(findProd(nodep, m_statep->getNodeSym(nodep), nodep->start()));
        symIterateChildren(nodep, m_statep->getNodeSym(nodep));
    }
    فراغ visit(AstRSProd* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        symIterateChildren(nodep, m_statep->getNodeSym(nodep));
    }
    فراغ visit(AstRSProdItem* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        nodep->prodp(findProd(nodep, m_curSymp, nodep->name()));
        iterateChildren(nodep);
    }
    فراغ visit(AstCoverWith* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        iterateAndNextNull(nodep->subp());
        // The type of 'item' is that of the coverpoint expression, which may name 'item' too
        iterateAndNextNull(nodep->itemp()->childDTypep());
        VL_RESTORER(m_curSymp);
        m_ds.m_dotSymp = m_curSymp = m_statep->getNodeSym(nodep);
        iterateAndNextNull(nodep->itemp());
        iterateAndNextNull(nodep->filterp());
        m_ds.m_dotSymp = VL_RESTORER_PREV(m_curSymp);
    }
    فراغ visit(AstCoverCrossWith* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        iterateAndNextNull(nodep->selectp());
        iterateAndNextNull(nodep->countp());
        // The type of a cross item is that of its coverpoint's expression, which may name items
        iterateAndNextNull(nodep->varsp());
        VL_RESTORER(m_curSymp);
        m_ds.m_dotSymp = m_curSymp = m_statep->getNodeSym(nodep);
        iterateAndNextNull(nodep->filterp());
        m_ds.m_dotSymp = VL_RESTORER_PREV(m_curSymp);
    }
    فراغ visit(AstWith* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        𐤀𐤌 (m_ds.m_unresolvedClass) أرجع;
        VL_RESTORER(m_curSymp);
        VL_RESTORER(m_currentWithp);
        VL_RESTORER_COPY(m_restrictedNamesUsed);
        {
            m_ds.m_dotSymp = m_curSymp = m_statep->getNodeSym(nodep);
            m_currentWithp = nodep;
            // Validate the identifier_list once per AstWith (m_validated travels
            // with cloneTree, so cloned witnesses skip the redundant check).
            ثابت منطقي firstVisit = !nodep->validated();
            𐤀𐤌 (firstVisit) {
                nodep->validated(true);
                m_restrictedNamesUsed.clear();
                // IEEE 1800-2023 18.7: each name in the identifier_list must designate
                // a member of the randomize() target class.
                𐤀𐤌 (nodep->restricted() && m_randSymp) {
                    لكل (ثابت std::string& n : nodep->restrictedNames()) {
                        𐤀𐤌 (!m_randSymp->findIdFlat(n)) {
                            nodep->v3error("Identifier '"
                                           << n
                                           << "' in 'with (...)' identifier list is not a"
                                              " member of the randomize() target class"
                                              " (IEEE 1800-2023 18.7)");
                            m_restrictedNamesUsed.insert(n);
                        }
                    }
                }
            }
            iterateChildren(nodep);
            // Flag names that the user listed but never referenced in the constraint.
            𐤀𐤌 (firstVisit && nodep->restricted()) {
                لكل (ثابت std::string& n : nodep->restrictedNames()) {
                    𐤀𐤌 (!m_restrictedNamesUsed.count(n)) {
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
    فراغ visit(AstLambdaArgRef* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        // No checknodot(nodep), visit(AstScope) will check for LambdaArgRef
        iterateChildren(nodep);
    }
    فراغ visit(AstClassExtends* nodep) override {
        // Resolve the symbol and get the class.
        // If it is a parameterized case, the class will be resolved after V3Param.cpp
        𐤀𐤌 (nodep->user3SetOnce()) أرجع;
        // If the class is resolved, there is nothing more to do
        𐤀𐤌 (nodep->classOrNullp()) أرجع;
        LINKDOT_VISIT_START();
        𐤀𐤌 (m_statep->forPrimary()) {
            𐤀𐤌 (nodep->childDTypep()) أرجع;
            AstNode* cprp = nodep->classOrPkgsp();
            VSymEnt* lookSymp = m_curSymp;
            𐤀𐤌 (AstDot* ثابت dotp = VN_CAST(cprp, Dot)) {
                dotp->user3(true);
                𐤀𐤌 (AstClassOrPackageRef* lookNodep = VN_CAST(dotp->lhsp(), ClassOrPackageRef)) {
                    iterate(lookNodep);
                    cprp = dotp->rhsp();
                    VSymEnt* ثابت foundp = m_statep->resolveClassOrPackage(
                        lookSymp, lookNodep, true, خطأ, nodep->verilogKwd());
                    𐤀𐤌 (!foundp) {
                        VL_DO_DANGLING(pushDeletep(nodep->unlinkFrBack()), nodep);
                        أرجع;
                    }
                    UASSERT_OBJ(lookNodep->classOrPackageSkipp(), nodep, "Bad package link");
                    lookSymp = m_statep->getNodeSym(lookNodep->classOrPackageSkipp());
                } وإلا {
                    dotp->lhsp()->v3error("Attempting to extend"  // LCOV_EXCL_LINE
                                          " using non-class under dot");
                }
            }
            AstClassOrPackageRef* ثابت cpackagerefp = VN_CAST(cprp, ClassOrPackageRef);
            𐤀𐤌 (VL_UNCOVERABLE(!cpackagerefp)) {
                // Linking the extend gives an error before this is hit
                nodep->v3error("Attempting to extend using non-class");  // LCOV_EXCL_LINE
                VL_DO_DANGLING(pushDeletep(nodep->unlinkFrBack()), nodep);
                أرجع;
            }
            VSymEnt* ثابت foundp = m_statep->resolveClassOrPackage(lookSymp, cpackagerefp, true,
                                                                    true, nodep->verilogKwd());
            𐤀𐤌 (!foundp) {
                VL_DO_DANGLING(pushDeletep(nodep->unlinkFrBack()), nodep);
                أرجع;
            }
            𐤀𐤌 (AstClass* ثابت classp = VN_CAST(foundp->nodep(), Class)) {
                AstPin* paramsp = cpackagerefp->paramsp();
                𐤀𐤌 (paramsp) {
                    paramsp = paramsp->cloneTree(true);
                    nodep->parameterized(true);
                }
                nodep->childDTypep(جديد AstClassRefDType{nodep->fileline(), classp, paramsp});
                // Link pins
                iterate(nodep->childDTypep());
            } وإلا 𐤀𐤌 (AstParamTypeDType* ثابت paramp
                       = VN_CAST(foundp->nodep(), ParamTypeDType)) {
                AstRefDType* ثابت refParamp = جديد AstRefDType{nodep->fileline(), paramp->name()};
                refParamp->refDTypep(paramp);
                nodep->childDTypep(refParamp);
                nodep->parameterized(true);
            } وإلا 𐤀𐤌 (AstTypedef* ثابت typedefp = VN_CAST(foundp->nodep(), Typedef)) {
                AstNodeDType* ثابت unwrappedp = typedefp->subDTypep()->skipRefp();
                𐤀𐤌 (AstClassRefDType* ثابت classRefp = VN_CAST(unwrappedp, ClassRefDType)) {
                    AstPin* paramsp = cpackagerefp->paramsp();
                    𐤀𐤌 (!paramsp && classRefp->paramsp()) {
                        // No explicit #(...) on extends clause; carry over the
                        // typedef's type-parameter pins so V3Param sees them.
                        paramsp = classRefp->paramsp()->cloneTree(true);
                        nodep->parameterized(true);
                    } وإلا 𐤀𐤌 (paramsp) {
                        paramsp = paramsp->cloneTree(true);
                        nodep->parameterized(true);
                    }
                    nodep->childDTypep(
                        جديد AstClassRefDType{nodep->fileline(), classRefp->classp(), paramsp});
                    iterate(nodep->childDTypep());
                } وإلا {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: " << foundp->nodep()->prettyTypeName()
                                                  << " in 'class extends'");
                    أرجع;
                }
            } وإلا {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: " << foundp->nodep()->prettyTypeName()
                                                             << " in 'class extends'");
                أرجع;
            }
            𐤀𐤌 (!nodep->childDTypep()) nodep->v3error("Attempting to extend using non-class");
            nodep->classOrPkgsp()->unlinkFrBack()->deleteTree();
        } وإلا {
            // Probably a parameter
            𐤀𐤌 (AstRefDType* ثابت refp = VN_CAST(nodep->childDTypep(), RefDType)) {
                𐤀𐤌 (AstClassRefDType* classRefp = VN_CAST(refp->skipRefp(), ClassRefDType)) {
                    // Resolved to a class reference.
                    refp->replaceWith(classRefp->cloneTree(خطأ));
                    VL_DO_DANGLING(pushDeletep(refp), refp);
                } وإلا {
                    // Unable to resolve the ref type to a class reference.
                    // Get the value of type parameter passed to the class instance,
                    // to print the helpful error message.
                    ثابت AstNodeDType* typep = refp->refDTypep();
                    طالما (true) {
                        𐤀𐤌 (ثابت AstParamTypeDType* ثابت atypep
                            = VN_CAST(typep, ParamTypeDType)) {
                            typep = atypep->subDTypep();
                            استمر;
                        }
                        𐤀𐤌 (ثابت AstRequireDType* ثابت atypep = VN_CAST(typep, RequireDType)) {
                            typep = atypep->subDTypep();
                            استمر;
                        }
                        اكسر;
                    }
                    typep->v3error("Attempting to extend using non-class");
                }
            }
        }
    }
    فراغ visit(AstClass* nodep) override {
        𐤀𐤌 (nodep->user3SetOnce()) أرجع;
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
            m_ds.m_dotSymp = m_curSymp = m_modSymp = m_statep->getNodeSym(nodep);
            m_modp = nodep;
            صحيح next = 0;
            لكل (AstClassExtends* cextp = nodep->extendsp(); cextp;
                 cextp = VN_AS(cextp->nextp(), ClassExtends)) {
                // Replace abstract reference with hard pointer
                // Will need later resolution when deal with parameters
                𐤀𐤌 (++next == 2 && !nodep->isInterfaceClass() && !cextp->isImplements()) {
                    cextp->v3error("Multiple inheritance illegal on non-interface classes"
                                   " (IEEE 1800-2023 8.13)");
                }
                iterate(cextp);
                𐤀𐤌 (m_statep->forPrimary()) {
                    𐤀𐤌 (cextp->parameterized()) {
                        // Parameters in extends statement.
                        // The class can't be resolved in the current pass.
                        m_extendsParam.insert(nodep);
                        m_insideClassExtParam = true;
                    }
                    𐤀𐤌 (AstClassRefDType* ثابت classRefp
                        = VN_CAST(cextp->childDTypep(), ClassRefDType)) {
                        AstClass* ثابت classp = classRefp->classp();
                        𐤀𐤌 (classp != nodep) iterate(classp);
                        𐤀𐤌 (m_extendsParam.find(classp) != m_extendsParam.end()) {
                            // One of its super classes has parameters in extends statement.
                            // Some links may not be resolved in the first pass.
                            m_extendsParam.insert(nodep);
                            m_insideClassExtParam = true;
                        }
                    }
                }

                𐤀𐤌 (AstClass* ثابت baseClassp = cextp->classOrNullp()) {
                    // Already converted. Update symbol table to link unlinked members.
                    // Base class has to be visited in a case if its extends statement
                    // needs to be handled. Recursive inheritance was already checked.
                    // Must be here instead of in LinkDotParam to handle
                    // "class (type T) extends T".
                    𐤀𐤌 (baseClassp == nodep) {
                        cextp->v3error("Attempting to extend class " << nodep->prettyNameQ()
                                                                     << " from itself");
                    } وإلا 𐤀𐤌 (extendsClass(baseClassp, nodep)) {
                        cextp->v3error("Attempting to extend class "
                                       << nodep->prettyNameQ() << " from "
                                       << baseClassp->prettyNameQ()
                                       << ", which circularly inherits from it");
                    } وإلا 𐤀𐤌 (cextp->isImplements() && !baseClassp->isInterfaceClass()) {
                        cextp->v3error("Attempting to implement from non-interface class "
                                       << baseClassp->prettyNameQ() << '\n'
                                       << "... Suggest use 'extends'");
                    } وإلا 𐤀𐤌 (!cextp->isImplements() && !nodep->isInterfaceClass()
                               && baseClassp->isInterfaceClass()) {
                        cextp->v3error("Attempting to extend from interface class "
                                       << baseClassp->prettyNameQ() << '\n'
                                       << "... Suggest use 'implements'");
                    }
                    baseClassp->isExtended(true);
                    nodep->isExtended(true);
                    iterate(baseClassp);
                    importSymbolsFromExtended(nodep, cextp);
                    استمر;
                }
            }
            m_ds.m_dotSymp = m_curSymp;

            iterateChildren(nodep);
            لكل (ثابت auto& itr : m_ifForwards) {
                ثابت VSymEnt* ثابت implSymp = itr.second;
                addInterfaceForward(nodep, VN_AS(implSymp->nodep(), NodeFTask),
                                    VN_AS(implSymp->parentp()->nodep(), Class));
            }
        }
        // V3Width when determines types needs to find enum values and such
        // so add members pointing to appropriate enum values
        {
            VMemberMap memberMap;
            لكل (VSymEnt::const_iterator it = m_curSymp->begin(); it != m_curSymp->end(); ++it) {
                AstNode* ثابت itemp = it->second->nodep();
                𐤀𐤌 (!memberMap.findMember(nodep, it->first)) {
                    𐤀𐤌 (AstEnumItem* ثابت aitemp = VN_CAST(itemp, EnumItem)) {
                        AstEnumItemRef* ثابت newp = جديد AstEnumItemRef{
                            aitemp->fileline(), aitemp, it->second->classOrPackagep()};
                        UINFO(8, indent()
                                     << "Class import noderef '" << it->first << "' " << newp);
                        nodep->addMembersp(newp);
                        memberMap.insert(nodep, newp);
                    }
                }
            }
        }
        m_ds.m_dotSymp = VL_RESTORER_PREV(m_curSymp);
    }

    // Collect the ClassOrPackageRefs in a chained scope operand in source order.
    // The grammar builds a scope operand only from packageClassScopeItem, which
    // always yields a ClassOrPackageRef, nested in Dots, so nothing else appears.
    ساكن فراغ collectScopeRefs(AstNode* nodep, std::vector<AstClassOrPackageRef*>& refps) {
        𐤀𐤌 (AstClassOrPackageRef* ثابت refp = VN_CAST(nodep, ClassOrPackageRef)) {
            refps.push_back(refp);
            أرجع;
        }
        AstDot* ثابت dotp = VN_CAST(nodep, Dot);
        UASSERT_OBJ(dotp, nodep, "Non-scope node in package/class scope operand");
        collectScopeRefs(dotp->lhsp(), refps);
        collectScopeRefs(dotp->rhsp(), refps);
    }

    // Resolve a chained class-scope operand (`pkg::outer::inner` in
    // `pkg::outer::inner::t`) and reduce it to its innermost ClassOrPackageRef.
    // Returns false, having reported, if a segment could not be resolved.
    منطقي reduceScopeDot(AstRefDType* nodep, AstDot* scopeDotp) {
        std::vector<AstClassOrPackageRef*> refps;
        collectScopeRefs(scopeDotp, refps);

        VSymEnt* scopeSymp = m_ds.m_dotSymp;
        لكل (size_t i = 0; i < refps.size(); ++i) {
            AstClassOrPackageRef* ثابت refp = refps[i];
            𐤀𐤌 (!refp->classOrPackageSkipp() && !refp->classOrPackageNodep()
                && !m_statep->resolveClassOrPackage(scopeSymp, refp, i == 0, خطأ,
                                                    "class/package reference")) {
                أرجع خطأ;  // Error already reported
            }
            𐤀𐤌 (i + 1 == refps.size()) اكسر;
            // A middle segment that resolved to a node with no reachable module
            // (e.g. a typedef alias of a not-yet-specialized parameterized class)
            // gives nothing to look the next segment up in.
            AstNodeModule* ثابت modp = refp->classOrPackageSkipp();
            𐤀𐤌 (!modp) {
                refp->v3warn(E_UNSUPPORTED, "Unsupported: Multiple '::' package/class reference");
                أرجع خطأ;
            }
            scopeSymp = m_statep->getNodeSym(modp);
        }
        AstClassOrPackageRef* ثابت innerp = refps.back();
        innerp->unlinkFrBack();
        VL_DO_DANGLING(pushDeletep(scopeDotp->unlinkFrBack()), scopeDotp);
        nodep->classOrPackageOpp(innerp);
        أرجع true;
    }

    فراغ visit(AstRefDType* nodep) override {

        𐤀𐤌 (auto* ثابت typeOfp = nodep->typeofp()) {
            iterate(typeOfp);
            // After iteration, typeofp() may have been replaced with a resolved type
            // (e.g., DOT expression like if0.rq_t resolved to REFDTYPE)
            // If it's now a NodeDType, replace this RefDType with it
            𐤀𐤌 (AstNodeDType* ثابت resolvedDTypep = VN_CAST(nodep->typeofp(), NodeDType)) {
                UINFO(9,
                      indent() << "iface capture typeofp resolved to dtype, replacing RefDType\n");
                resolvedDTypep->unlinkFrBack();
                nodep->replaceWith(resolvedDTypep);
                VL_DO_DANGLING(pushDeletep(nodep), nodep);
                // Originally (PR #6637) this block re-captured a resolved
                // RefDType into the iface-capture ledger when user2p held a
                // Cell, to ensure re-resolution during the paramed pass.
                // In practice, the typeofp-resolved RefDType never carries a
                // Cell in user2p: the capture happens earlier when the
                // typedef is first linked.
                𐤀𐤌 (V3LinkDotIfaceCapture::enabled()) {
                    𐤀𐤌 (AstRefDType* ثابت resolvedRefp = VN_CAST(resolvedDTypep, RefDType)) {
                        𐤀𐤌 (VL_UNCOVERABLE(
                                VN_IS(resolvedRefp->user2p(), Cell))) {  // LCOV_EXCL_LINE
                            resolvedRefp->v3fatalSrc(  // LCOV_EXCL_LINE
                                "typeofp resolved RefDType has Cell in user2p;"
                                " expected to be captured already");
                        }
                    }
                }
            }
            أرجع;
        }

        // Resolve its reference
        𐤀𐤌 (m_statep->forParamed() && nodep->user3()) {
            𐤀𐤌 (V3LinkDotIfaceCapture::enabled() && nodep->user2p()) {
                UINFO(9, indent() << "iface capture clear user3 for captured typedef name="
                                  << nodep->name() << " cell=" << nodep->user2p());
            }
            nodep->user3(خطأ);
        }
        𐤀𐤌 (nodep->user3SetOnce()) {
            𐤀𐤌 (V3LinkDotIfaceCapture::enabled() && nodep->user2p()) {
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
            𐤀𐤌 (VL_UNCOVERABLE(m_statep->forParamed() && nodep->classOrPackageOpp())) {
                nodep->v3fatalSrc(  // LCOV_EXCL_LINE
                    "RefDType user3 revisit with classOrPackageOpp in paramed pass");
            }
            أرجع;
        }
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        𐤀𐤌 (nodep->classOrPackageOpp()) {
            // Resolve a chained scope (`pkg::cls::t`, `pkg::cls#(P)::t`) from outside in.
            // On failure reduceScopeDot has reported, so stop before using the operand.
            𐤀𐤌 (AstDot* ثابت scopeDotp = VN_CAST(nodep->classOrPackageOpp(), Dot)) {
                𐤀𐤌 (!reduceScopeDot(nodep, scopeDotp)) أرجع;
            }
            // Re-read: reduceScopeDot may have replaced the operand
            AstNode* ثابت cpackagep = nodep->classOrPackageOpp();
            UASSERT_OBJ(!VN_IS(cpackagep, Dot), cpackagep,
                        "Package/class scope operand should be reduced by now");
            𐤀𐤌 (AstClassOrPackageRef* ثابت cpackagerefp = VN_CAST(cpackagep, ClassOrPackageRef)) {
                iterate(cpackagerefp);
                ثابت AstClass* ثابت clsp = VN_CAST(cpackagerefp->classOrPackageNodep(), Class);
                𐤀𐤌 (clsp && clsp->hasGParam()) {
                    // Unable to link before the instantiation of parameter classes.
                    // The class reference node still has to be visited now to later link
                    // parameters.
                    iterate(cpackagep);
                    أرجع;
                }
                // Defer non-typedef references through typedef aliases of parameterized classes.
                𐤀𐤌 (m_statep->forPrimary() && !VN_IS(nodep->backp(), Typedef)
                    && isParamedClassRef(cpackagerefp)) {
                    iterate(cpackagep);
                    أرجع;
                }

                ثابت منطقي doDefaultTypedef = !(m_resolvingTypedef && m_statep->forPrimary());
                𐤀𐤌 (!cpackagerefp->classOrPackageSkipp(doDefaultTypedef)
                    && !cpackagerefp->classOrPackageNodep()) {
                    VSymEnt* ثابت foundp = m_statep->resolveClassOrPackage(
                        m_ds.m_dotSymp, cpackagerefp, true, خطأ, "class/package reference");
                    𐤀𐤌 (!foundp) أرجع;
                }
                nodep->classOrPackagep(cpackagerefp->classOrPackageSkipp(doDefaultTypedef));
                𐤀𐤌 (!VN_IS(nodep->classOrPackagep(), Class)
                    && !VN_IS(nodep->classOrPackagep(), Package)) {
                    𐤀𐤌 (m_statep->forPrimary()) {
                        // It may be a type that comes from parameter class that is not
                        // instantioned yet
                        iterate(cpackagep);
                        أرجع;
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
            VL_DO_DANGLING(pushDeletep(cpackagep->unlinkFrBack()), cpackagep);
        }

        𐤀𐤌 (m_ds.m_dotp && (m_ds.m_dotPos == DP_PACKAGE || m_ds.m_dotPos == DP_SCOPE)) {
            UASSERT_OBJ(VN_IS(m_ds.m_dotp->lhsp(), ClassOrPackageRef), m_ds.m_dotp->lhsp(),
                        "Bad package link");
            auto* ثابت cpackagerefp = VN_CAST(m_ds.m_dotp->lhsp(), ClassOrPackageRef);
            UASSERT_OBJ(cpackagerefp->classOrPackageSkipp(), m_ds.m_dotp->lhsp(),
                        "Bad package link");
            nodep->classOrPackagep(cpackagerefp->classOrPackageSkipp());
            m_ds.m_dotPos = DP_SCOPE;
        } وإلا {
            // Allow REFDTYPE under DOT when referencing interface typedefs
            // This is needed for patterns like: typedef iface.a_t a_t;
            // The dependency graph handles resolution when IfaceCapture is disabled
            ثابت منطقي ifaceFinalSegmentAllowed
                = (m_ds.m_dotPos == DP_FINAL) && m_ds.m_dotSymp
                  && VN_IS(m_ds.m_dotSymp->nodep(), Cell)
                  && VN_CAST(m_ds.m_dotSymp->nodep(), Cell)->modp()
                  && VN_IS(VN_CAST(m_ds.m_dotSymp->nodep(), Cell)->modp(), Iface);
            𐤀𐤌 (!ifaceFinalSegmentAllowed) {
                checkNoDot(nodep);
            } وإلا {
                UINFO(9, indent() << "allow REFDTYPE under DOT for iface typedef name="
                                  << nodep->prettyNameQ() << " dotSym=" << m_ds.m_dotSymp);
                // Clear the dot state so we don't propagate errors
                m_ds.m_dotPos = DP_SCOPE;
            }
        }
        𐤀𐤌 (!nodep->typedefp() && !nodep->subDTypep()) {
            ثابت VSymEnt* foundp;
            𐤀𐤌 (nodep->classOrPackagep()) {
                foundp = m_statep->getNodeSym(nodep->classOrPackagep())->findIdFlat(nodep->name());
            } وإلا 𐤀𐤌 (m_ds.m_dotPos == DP_FIRST || m_ds.m_dotPos == DP_NONE) {
                foundp = findIdFallbackSkipMemberDType(m_curSymp, nodep->name());
            } وإلا {
                // Defensive: dotPos should be DP_FIRST/DP_NONE or classOrPackagep set.
                v3fatalSrc("Unexpected dotPos="
                           << static_cast<صحيح>(m_ds.m_dotPos)  // LCOV_EXCL_LINE
                           << " in RefDType lookup for "
                           << nodep->prettyNameQ());  // LCOV_EXCL_LINE
                foundp = nullptr;  // LCOV_EXCL_LINE
            }
            {
                𐤀𐤌 (AstTypedef* ثابت defp
                    = foundp ? VN_CAST(foundp->nodep(), Typedef) : nullptr) {
                    // Don't check if typedef is to a <type T>::<reference> as might not be
                    // resolved yet
                    𐤀𐤌 (!nodep->classOrPackagep() && !defp->isUnderClass())
                        checkDeclOrder(nodep, defp);
                    nodep->typedefp(defp);
                    nodep->classOrPackagep(foundp->classOrPackagep());

                } وإلا 𐤀𐤌 (AstParamTypeDType* ثابت defp
                           = foundp ? VN_CAST(foundp->nodep(), ParamTypeDType) : nullptr) {
                    // A PARAMTYPEDTYPE's child REFDTYPE referencing itself is the normal
                    // AST structure for type parameters (e.g., "type T = base" has a child
                    // REFDTYPE for the default type). Only error on true recursion where
                    // a TYPEDEF contains a reference back to itself.
                    ثابت منطقي isParamTypeChild = (defp == nodep->backp());
                    ثابت منطقي isTypedefRecursion
                        = isParamTypeChild && VN_IS(defp->backp(), Typedef);
                    𐤀𐤌 (isTypedefRecursion) {
                        nodep->v3error("Reference to '"
                                       << m_ds.m_dotText << (m_ds.m_dotText == "" ? "" : ".")
                                       << nodep->prettyName() << "'"
                                       << " type would form a recursive definition");
                        nodep->refDTypep(nodep->findVoidDType());  // Try to reduce later errors
                    } وإلا {
                        nodep->refDTypep(defp);
                        nodep->classOrPackagep(foundp->classOrPackagep());
                        captureIfaceParamType(nodep, defp);
                    }
                } وإلا 𐤀𐤌 (AstClass* ثابت defp
                           = foundp ? VN_CAST(foundp->nodep(), Class) : nullptr) {
                    // Don't check if typedef is to a <type T>::<reference> as might not be
                    // resolved yet
                    𐤀𐤌 (!nodep->classOrPackagep()) checkDeclOrder(nodep, defp);
                    AstPin* ثابت paramsp = nodep->paramsp();
                    𐤀𐤌 (paramsp) paramsp->unlinkFrBackWithNext();
                    AstClassRefDType* ثابت newp
                        = جديد AstClassRefDType{nodep->fileline(), defp, paramsp};
                    newp->classOrPackagep(foundp->classOrPackagep());
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(pushDeletep(nodep), nodep);
                    أرجع;
                } وإلا 𐤀𐤌 (m_insideClassExtParam) {
                    أرجع;
                } وإلا {
                    𐤀𐤌 (foundp) {
                        UINFO(1, "Found sym node: " << foundp->nodep());
                        nodep->v3error("Expecting a data type: "
                                       << nodep->prettyNameQ()
                                       << ", found: " << foundp->nodep()->prettyTypeName());
                    } وإلا {
                        nodep->v3error("Can't find typedef/interface: " << nodep->prettyNameQ());
                    }
                }
            }
        }
        iterateChildren(nodep);
    }
    فراغ visit(AstRequireDType* nodep) override {
        // Handle type validation for localparam type assignments
        // Iterate child to resolve any ParseRef nodes
        iterateChildren(nodep);
        // Only emit error if the child is not a type.
        // Do NOT unwrap valid types here - leave that to V3Width.
        // Unwrapping here breaks type parameter resolution during cloning.
        // Allow Dot through to defer the handling until the dot expression is resolved.
        𐤀𐤌 (nodep->lhsp() && !VN_IS(nodep->lhsp(), NodeDType) && !VN_IS(nodep->lhsp(), Dot)) {
            // Not a type - emit error
            𐤀𐤌 (VN_IS(nodep->lhsp(), Const)) {
                nodep->lhsp()->v3error("Expecting a data type, not a constant");
            } وإلا {
                nodep->lhsp()->v3error("Expecting a data type, not "
                                       << nodep->lhsp()->typeName() << ": '"
                                       << nodep->lhsp()->prettyName() << "'");
            }
            // Replace with void to keep AST valid and allow clean exit
            nodep->replaceWith(جديد AstVoidDType{nodep->fileline()});
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
        }
    }
    فراغ visit(AstDpiExport* nodep) override {
        // AstDpiExport: Make sure the function referenced exists, then dump it
        LINKDOT_VISIT_START();
        iterateChildren(nodep);
        checkNoDot(nodep);
        VSymEnt* ثابت foundp = m_curSymp->findIdFallback(nodep->name());
        AstNodeFTask* ثابت taskp = foundp ? VN_AS(foundp->nodep(), NodeFTask) : nullptr;
        𐤀𐤌 (!taskp) {
            nodep->v3error(
                "Can't find definition of exported task/function: " << nodep->prettyNameQ());
        } وإلا 𐤀𐤌 (taskp->dpiExport()) {
            nodep->v3error("Function was already DPI Exported, duplicate not allowed: "
                           << nodep->prettyNameQ());
        } وإلا {
            taskp->dpiExport(true);
            𐤀𐤌 (nodep->cname() != "") taskp->cname(nodep->cname());
        }
        VL_DO_DANGLING(pushDeletep(nodep->unlinkFrBack()), nodep);
    }
    فراغ visit(AstDisable* nodep) override {
        LINKDOT_VISIT_START();
        checkNoDot(nodep);
        منطقي hasPartSelect = خطأ;
        ثابت string targetPath
            = nodep->targetRefp() ? extractDottedPath(nodep->targetRefp(), hasPartSelect) : "";
        VL_RESTORER_COPY(m_ds);
        m_ds.init(m_curSymp);
        m_ds.m_dotPos = DP_FIRST;
        m_ds.m_disablep = nodep;
        iterateChildren(nodep);
        𐤀𐤌 (nodep->targetRefp()) {
            𐤀𐤌 (AstTaskRef* ثابت taskRefp = VN_CAST(nodep->targetRefp(), TaskRef)) {
                nodep->targetp(taskRefp->taskp());
            } وإلا 𐤀𐤌 (!VN_IS(nodep->targetRefp(), ParseRef)) {
                // If it is a ParseRef, either it couldn't be linked or it is linked to a block
                nodep->v3warn(E_UNSUPPORTED, "Node of type "
                                                 << nodep->targetRefp()->prettyTypeName()
                                                 << " referenced by disable");
                VL_DO_DANGLING(pushDeletep(nodep->unlinkFrBack()), nodep);
                أرجع;
            }
            𐤀𐤌 (nodep->targetp()) {
                nodep->targetRefp()->unlinkFrBack()->deleteTree();
                𐤀𐤌 (!hasPartSelect) {
                    ثابت string instancePath = targetInstancePath(nodep->targetp(), targetPath);
                    𐤀𐤌 (!instancePath.empty()) {
                        // Keep only the instance prefix so V3LinkJump can reference the selected
                        // instance's process queue without reparsing the disable target.
                        nodep->targetRefp(
                            جديد AstVarXRef{nodep->fileline(), "", instancePath, VAccess::READ});
                    }
                }
            }
        }
    }
    فراغ visitPackageImportOrExport(AstNode* nodep) {
        // No longer needed
        LINKDOT_VISIT_START();
        checkNoDot(nodep);
        𐤀𐤌 (m_statep->forParamed()) VL_DO_DANGLING(pushDeletep(nodep->unlinkFrBack()), nodep);
    }
    فراغ visit(AstPackageImport* nodep) override { visitPackageImportOrExport(nodep); }
    فراغ visit(AstPackageExport* nodep) override { visitPackageImportOrExport(nodep); }
    فراغ visit(AstPackageExportStarStar* nodep) override { visitPackageImportOrExport(nodep); }
    فراغ visit(AstCellRef* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        iterateChildren(nodep);
    }
    فراغ visit(AstCellArrayRef* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        // Expression already iterated
    }
    فراغ visit(AstUnlinkedRef* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        // No need to iterate, if we have a UnlinkedVarXRef, we're already done
    }
    فراغ visit(AstStmtExpr* nodep) override {
        LINKDOT_VISIT_START();
        checkNoDot(nodep);
        // Check if nodep represents a super.new call;
        𐤀𐤌 (AstNew* ثابت newExprp = VN_CAST(nodep->exprp(), New)) {
            // in this case it was already linked, so it doesn't have a super reference
            m_explicitSuperNewp = newExprp;
        } وإلا 𐤀𐤌 (ثابت AstDot* ثابت dotp = VN_CAST(nodep->exprp(), Dot)) {
            𐤀𐤌 (dotp->lhsp()->name() == "super") {
                𐤀𐤌 (AstNew* ثابت newExprp = VN_CAST(dotp->rhsp(), New)) {
                    m_explicitSuperNewp = newExprp;
                }
            }
        }
        iterateChildren(nodep);
    }

    فراغ visit(AstIfaceRefDType* nodep) override {
        𐤀𐤌 (nodep->user3SetOnce()) أرجع;
        LINKDOT_VISIT_START();
        𐤀𐤌 (nodep->paramsp()) {
            // If there is no parameters, there is no need to visit this node.
            AstIface* ثابت ifacep = nodep->ifacep();
            UASSERT_OBJ(ifacep, nodep, "Port parameters of AstIfaceRefDType without ifacep()");
            𐤀𐤌 (ifacep->dead()) أرجع;
            checkNoDot(nodep);
            VL_RESTORER_CLEAR(m_usedPins);
            VL_RESTORER_CLEAR(m_usedDefParamPins);
            VL_RESTORER(m_pinSymp);
            m_pinSymp = m_statep->getNodeSym(ifacep);
            iterateAndNextNull(nodep->paramsp());
        }
    }

    فراغ visit(AstAttrOf* nodep) override { iterateChildren(nodep); }
    فراغ visit(AstCastParse* nodep) override {
        𐤀𐤌 (nodep->user3SetOnce()) أرجع;
        LINKDOT_VISIT_START();
        // A cast may start a dotted expression: HERE type'(expr) . member_or_method
        ثابت منطقي dotLhs = m_ds.m_dotPos == DP_FIRST;
        𐤀𐤌 (!dotLhs) checkNoDot(nodep);
        {
            VL_RESTORER_COPY(m_ds);
            m_ds.init(m_curSymp);
            iterateChildren(nodep);
        }
        𐤀𐤌 (dotLhs) m_ds.m_dotPos = DP_MEMBER;
    }

    فراغ visit(AstAssignW* nodep) override {
        LINKDOT_VISIT_START();
        checkNoDot(nodep);
        VL_RESTORER(m_replaceWithAlias);
        𐤀𐤌 (nodep->user2()) m_replaceWithAlias = خطأ;
        iterateChildren(nodep);
    }

    فراغ visit(AstTypedef* nodep) override {
        VL_RESTORER(m_resolvingTypedef)
        m_resolvingTypedef = true;
        iterateChildren(nodep);
    }

    فراغ visit(AstNode* nodep) override {
        VL_RESTORER(m_inPackedArray);
        𐤀𐤌 (VN_IS(nodep, PackArrayDType)) {
            m_inPackedArray = true;
        } وإلا 𐤀𐤌 (!m_inPackedArray) {
            LINKDOT_VISIT_START();
            checkNoDot(nodep);
        }
        iterateChildren(nodep);
    }

عام:
    // CONSTRUCTORS
    LinkDotResolveVisitor(AstNetlist* rootp, LinkDotState* statep)
        : m_statep{statep} {
        UINFO(4, __FUNCTION__ << ": ");

        // Note: no need to clear user3 on ledger entries here.
        // VNUser3InUse (m_inuser3) already logically clears user3 on all
        // nodes via the generation counter.  The ledger may also hold stale
        // refp pointers to deleted AST nodes, so iterating it is unsafe.

        iterate(rootp);
        ثابت std::map<std::string, AstNodeModule*> modulesToRevisit
            = std::move(m_modulesToRevisit);
        m_lastDeferredp = nullptr;
        لكل (auto& p : modulesToRevisit) {
            AstNodeModule* ثابت modp = p.second;
            modp->foreach([](AstNode* ثابت nodep) { nodep->user3(خطأ); });
            iterate(modp);
        }
    }
    ~LinkDotResolveVisitor() override = افتراضي;
};

//######################################################################
// Link class functions

فراغ V3LinkDot::dumpSubstep(ثابت string& name) {
    𐤀𐤌 (dumpTreeEitherLevel() >= 9) {
        V3Global::dumpCheckGlobalTree(name);
    } وإلا 𐤀𐤌 (debug() >= 5) {  // on high dbg level, dump even if not explicitly told to
        v3Global.rootp()->dumpTreeFile(v3Global.debugFilename(name + ".tree"));
    }
}

فراغ V3LinkDot::linkDotGuts(AstNetlist* rootp, VLinkDotStep step) {
    VIsCached::clearCacheTree();  // Avoid using any stale isPure
    dumpSubstep("prelinkdot");
    LinkDotState state{rootp, step};

    { LinkDotFindVisitor{rootp, &state}; }
    dumpSubstep("prelinkdot-find");

    { LinkDotFindIfaceVisitor{rootp, &state}; }
    dumpSubstep("prelinkdot-findiface");

    𐤀𐤌 (step == LDS_SCOPED) {
        // Well after the initial link when we're ready to operate on the flat design,
        // process AstScope's.  This needs to be separate pass after whole hierarchy graph created.
        { LinkDotScopeVisitor{rootp, &state}; }
        v3Global.assertScoped(true);
        dumpSubstep("prelinkdot-scoped");
    } وإلا {
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

فراغ V3LinkDot::linkDotPrimary(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    linkDotGuts(nodep, LDS_PRIMARY);
    V3Global::dumpCheckGlobalTree("linkdot", 0, dumpTreeEitherLevel() >= 6);
}

فراغ V3LinkDot::linkDotParamed(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    linkDotGuts(nodep, LDS_PARAMED);
    V3Global::dumpCheckGlobalTree("linkdotparam", 0, dumpTreeEitherLevel() >= 3);
}

فراغ V3LinkDot::linkDotScope(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    linkDotGuts(nodep, LDS_SCOPED);
    V3Global::dumpCheckGlobalTree("linkdot", 0, dumpTreeEitherLevel() >= 3);
}
