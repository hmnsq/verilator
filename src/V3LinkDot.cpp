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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3LinkDot.h"

#अन्तर्भाव "V3Global.h"
#अन्तर्भाव "V3Graph.h"
#अन्तर्भाव "V3LinkDotIfaceCapture.h"
#अन्तर्भाव "V3MemberMap.h"
#अन्तर्भाव "V3Parse.h"
#अन्तर्भाव "V3Randomize.h"
#अन्तर्भाव "V3Stats.h"
#अन्तर्भाव "V3String.h"
#अन्तर्भाव "V3SymTable.h"

#अन्तर्भाव <algorithm>
#अन्तर्भाव <cstdlib>
#अन्तर्भाव <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

स्थिर string extractDottedPath(Ast_गाठ* nodep, तार्किक& hasPartSelect) {
    जर (AstParseRef* स्थिरांक refp = VN_CAST(nodep, ParseRef)) {
        परत refp->name();
    } अन्यथा जर (AstVarRef* स्थिरांक refp = VN_CAST(nodep, VarRef)) {
        परत refp->name();
    } अन्यथा जर (AstDot* स्थिरांक dotp = VN_CAST(nodep, Dot)) {
        स्थिरांक string lhs = extractDottedPath(dotp->lhsp(), hasPartSelect);
        स्थिरांक string rhs = extractDottedPath(dotp->rhsp(), hasPartSelect);
        परत VString::dot(lhs, ".", rhs);
    } अन्यथा जर (VN_IS(nodep, SelBit) || VN_IS(nodep, SelExtract) || VN_IS(nodep, SelPlus)
               || VN_IS(nodep, SelMinus)) {
        hasPartSelect = सत्य;
        परत "";
    }
    परत "";
}
स्थिर string lexicalDisablePath(Ast_गाठ* स्थिरांक targetp) {
    std::vector<string> names;
    साठी (Ast_गाठ* curp = targetp; curp; curp = curp->aboveLoopp()) {
        जर (Ast_गाठ_Block* स्थिरांक blockp = VN_CAST(curp, गाठ_Block)) {
            जर (blockp->name() != "") names.ढकला_back(blockp->name());
        } अन्यथा जर (Ast_गाठ_FTask* स्थिरांक ftaskp = VN_CAST(curp, गाठ_FTask)) {
            names.ढकला_back(ftaskp->name());
        } अन्यथा जर (VN_IS(curp, गाठ_Module)) {
            थांबा;
        }
    }
    string path;
    साठी (स्वतः it = names.crbegin(); it != names.crend(); ++it) {
        path = VString::dot(path, ".", *it);
    }
    परत path;
}
स्थिर string targetInstancePath(Ast_गाठ* स्थिरांक targetp, स्थिरांक string& targetPath) {
    स्थिरांक string lexicalPath = lexicalDisablePath(targetp);
    जर (lexicalPath == "" || targetPath == lexicalPath) परत "";
    स्थिरांक string suffix = "." + lexicalPath;
    जर (!VString::endsWith(targetPath, suffix)) परत "";
    परत targetPath.substr(0, targetPath.size() - suffix.size());
}

// ######################################################################
//  Matcher classes (for suggestion matching)

वर्ग Link_गाठ_Matcher_Class_Or_Package final : सार्वजनिक VNodeMatcher {
सार्वजनिक:
    तार्किक गाठ_Match(स्थिरांक Ast_गाठ* nodep) स्थिरांक override {
        परत VN_IS(nodep, Class) || VN_IS(nodep, Package);
    }
};
वर्ग Link_गाठ_Matcher_FTask final : सार्वजनिक VNodeMatcher {
सार्वजनिक:
    तार्किक गाठ_Match(स्थिरांक Ast_गाठ* nodep) स्थिरांक override { परत VN_IS(nodep, गाठ_FTask); }
};
वर्ग Link_गाठ_Matcher_Modport final : सार्वजनिक VNodeMatcher {
सार्वजनिक:
    तार्किक गाठ_Match(स्थिरांक Ast_गाठ* nodep) स्थिरांक override { परत VN_IS(nodep, Modport); }
};
वर्ग Link_गाठ_Matcher_Prod final : सार्वजनिक VNodeMatcher {
सार्वजनिक:
    तार्किक गाठ_Match(स्थिरांक Ast_गाठ* nodep) स्थिरांक override { परत VN_IS(nodep, RSProd); }
};
वर्ग Link_गाठ_Matcher_Var final : सार्वजनिक VNodeMatcher {
सार्वजनिक:
    तार्किक गाठ_Match(स्थिरांक Ast_गाठ* nodep) स्थिरांक override {
        परत VN_IS(nodep, Var) || VN_IS(nodep, LambdaArgRef);
    }
};
वर्ग Link_गाठ_Matcher_Var_IO final : सार्वजनिक VNodeMatcher {
सार्वजनिक:
    तार्किक गाठ_Match(स्थिरांक Ast_गाठ* nodep) स्थिरांक override {
        स्थिरांक AstVar* स्थिरांक varp = VN_CAST(nodep, Var);
        जर (!varp) परत असत्य;
        परत varp->isIO();
    }
};
वर्ग Link_गाठ_Matcher_Var_Param final : सार्वजनिक VNodeMatcher {
सार्वजनिक:
    तार्किक गाठ_Match(स्थिरांक Ast_गाठ* nodep) स्थिरांक override {
        स्थिरांक AstVar* स्थिरांक varp = VN_CAST(nodep, Var);
        जर (!varp) परत असत्य;
        परत varp->isParam();
    }
};

//######################################################################
// LinkDot state, as a visitor of each AstNode

वर्ग LinkDotState final {
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
    स्थिरांक VNUser1InUse m_inuser1;
    स्थिरांक VNUser2InUse m_inuser2;
    स्थिरांक VNUser4InUse m_inuser4;

सार्वजनिक:
    // ENUMS
    // In order of priority, compute first ... compute last
    गणना SAMNum : uint8_t { SAMN_MODPORT, SAMN_IFTOP, SAMN__MAX };  // Values for m_scopeAliasMap

खाजगी:
    // TYPES
    using ScopeAliasMap = std::unordered_map<VSymEnt*, VSymEnt*>;
    using IfaceModSyms = std::vector<std::pair<AstIface*, VSymEnt*>>;

    स्थिर LinkDotState* s_errorThisp;  // Last self, for error reporting only

    // MEMBERS
    VSymGraph m_syms;  // Symbol table by hierarchy
    VSymGraph m_mods;  // Symbol table of all module names
    VSymEnt* m_dunitEntp = nullptr;  // $unit entry
    std::multimap<std::string, VSymEnt*>
        m_nameScopeSymMap;  // Map of scope referenced by non-pretty textual name
    std::set<std::pair<Ast_गाठ_Module*, std::string>>
        m_implicitNameSet;  // For [module][signalname] if we can implicitly create it
    std::array<ScopeAliasMap, SAMN__MAX> m_scopeAliasMap;  // Map of <lhs,rhs> aliases
    std::vector<VSymEnt*> m_ifaceVarSyms;  // List of AstIfaceRefDType's to be imported
    IfaceModSyms m_ifaceModSyms;  // List of AstIface+Symbols to be processed
    स्थिरांक VLinkDotStep m_step;  // Operational step

सार्वजनिक:
    // METHODS

    शून्य dumpSelf(स्थिरांक string& nameComment, तार्किक force = असत्य) {
        जर (debug() >= 6 || dumpLevel() >= 6 || force) {
            स्थिरांक string filename = v3Global.debugFilename(nameComment) + ".txt";
            UINFO(4, "Dumping " << filename);
            स्थिरांक std::unique_ptr<std::ofstream> logp{V3File::new_ofstream(filename)};
            जर (logp->fail()) v3fatal("Can't write file: " << filename);
            std::ostream& os = *logp;
            // TODO the symbol table has node pointers which may be
            // dangling, as we call deleteTree in these visitors without
            // pushing for later deletion (or deleting from the symbol table)
            // So, only completely safe to call this when under --debug or --debug-leak.
            m_syms.dumpSelf(os);
            तार्किक first = सत्य;
            साठी (पूर्णांक samn = 0; samn < SAMN__MAX; ++samn) {
                जर (!m_scopeAliasMap[samn].empty()) {
                    जर (first) os << "\nScopeAliasMap:\n";
                    first = असत्य;
                    साठी (ScopeAliasMap::iterator it = m_scopeAliasMap[samn].begin();
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
    स्थिर शून्य preErrorDumpHandler() {
        जर (s_errorThisp) s_errorThisp->preErrorDump();
    }
    शून्य preErrorDump() {
        स्थिर तार्किक s_diddump = असत्य;
        जर (!s_diddump && dump_वृक्ष_Level()) {
            s_diddump = सत्य;
            dumpSelf("linkdot-preerr", सत्य);
            v3Global.rootp()->dump_वृक्ष_File(v3Global.debugFilename("linkdot-preerr.tree"));
        }
    }

    // CONSTRUCTORS
    LinkDotState(AstNetlist* rootp, VLinkDotStep step)
        : m_syms{rootp}
        , m_mods{rootp}
        , m_step{step} {
        UINFO(4, __FUNCTION__ << ": ");
        s_errorThisp = हे;
        V3Error::errorExitCb(preErrorDumpHandler);  // If get error, dump self
        readModNames();
    }
    ~LinkDotState() {
        V3Error::errorExitCb(nullptr);
        s_errorThisp = nullptr;
    }

    // ACCESSORS
    VSymGraph* symsp() { परत &m_syms; }
    पूर्णांक step_संख्या() स्थिरांक { परत static_cast<पूर्णांक>(m_step); }
    तार्किक forPrimary() स्थिरांक { परत m_step == LDS_PRIMARY; }
    तार्किक forParamed() स्थिरांक { परत m_step == LDS_PARAMED; }
    तार्किक forScopeCreation() स्थिरांक { परत m_step == LDS_SCOPED; }

    // METHODS
    स्थिर string गाठ_Text_Type(Ast_गाठ* nodep) {
        जर (स्थिरांक AstVar* स्थिरांक varp = VN_CAST(nodep, Var)) {
            जर (varp->isIO() || varp->isIfaceRef()) {
                परत "port";
            } अन्यथा जर (varp->isGParam()) {
                परत "parameter";
            } अन्यथा जर (varp->varType() == VVarType::LPARAM) {
                परत "local parameter";
            } अन्यथा {
                परत "variable";
            }
        } अन्यथा जर (स्थिरांक AstParamTypeDType* स्थिरांक dtypep = VN_CAST(nodep, ParamTypeDType)) {
            जर (dtypep->isGParam()) {
                परत "type parameter";
            } अन्यथा {
                परत "local type parameter";
            }
        } अन्यथा जर (VN_IS(nodep, Begin)) {
            परत "block";
        } अन्यथा जर (VN_IS(nodep, Cell)) {
            परत "instance";
        } अन्यथा जर (VN_IS(nodep, Constraint)) {
            परत "constraint";
        } अन्यथा जर (VN_IS(nodep, Func)) {
            परत "function";
        } अन्यथा जर (VN_IS(nodep, GenBlock)) {
            परत "generate block";
        } अन्यथा जर (VN_IS(nodep, Iface)) {
            परत "interface";
        } अन्यथा जर (VN_IS(nodep, RSProd)) {
            परत "randsequence production";
        } अन्यथा जर (VN_IS(nodep, Task)) {
            परत "task";
        } अन्यथा {
            परत nodep->prettyTypeName();
        }
    }

    Ast_गाठ_Module* findModuleSym(स्थिरांक string& modName) {
        स्थिरांक VSymEnt* स्थिरांक foundp = m_mods.rootp()->findIdFallback(modName);
        परत foundp ? VN_AS(foundp->nodep(), गाठ_Module) : nullptr;
    }
    शून्य readModNames() {
        // Look at all modules, and store pointers to all module names
        साठी (Ast_गाठ_Module *nextp, *nodep = v3Global.rootp()->modulesp(); nodep; nodep = nextp) {
            nextp = VN_AS(nodep->nextp(), गाठ_Module);
            m_mods.rootp()->घाला(nodep->name(), नवीन VSymEnt{&m_mods, nodep});
        }
    }

    VSymEnt* मूळ_Entp() स्थिरांक { परत m_syms.rootp(); }
    VSymEnt* dunitEntp() स्थिरांक { परत m_dunitEntp; }
    शून्य checkDuplicate(VSymEnt* lookupSymp, Ast_गाठ* nodep, स्थिरांक string& name) {
        // Lookup the given name under current symbol table
        // Insert if not found
        // Report error if there's a duplicate
        //
        // Note we only check for conflicts at the same level; it's ok if one block hides another
        // We also wouldn't want to not insert it even though it's lower down

        स्थिरांक VSymEnt* स्थिरांक foundp = lookupSymp->findIdFlat(name);
        Ast_गाठ* स्थिरांक fnodep = foundp ? foundp->nodep() : nullptr;
        जर (!fnodep) {
            // Not found, will add in a moment.
            जर (forPrimary() && VSymEnt::checkSimilarname(nodep)) {
                स्थिरांक VSymEnt* स्थिरांक alt = lookupSymp->findSimilarIdFlat(name);
                जर (alt) {
                    nodep->v3warn(SIMILARNAME, "Declaration overlaps another with different case: "
                                                   << nodep->prettyNameQ() << '\n'
                                                   << nodep->warnContextPrimary() << '\n'
                                                   << alt->nodep()->warnOther()
                                                   << "... Location of original declaration\n"
                                                   << alt->nodep()->warnContextSecondary());
                }
            }
        } अन्यथा जर (nodep == fnodep) {  // Already inserted.
            // Good.
        } अन्यथा जर (foundp->imported()) {  // From package
            // We don't throw VARHIDDEN as if the import is later the symbol
            // table's import wouldn't warn
        } अन्यथा जर (forPrimary() && VN_IS(nodep, GenBlock)
                   && (VN_IS(fnodep, Begin) || VN_IS(fnodep, GenBlock))) {
            // Begin: ... blocks often replicate under genif/genfor, so
            // suppress duplicate checks.  See t_gen_forif.v for an example.
        } अन्यथा {
            UINFO(4, "name " << name);  // Not always same as nodep->name
            UINFO(4, "Var1 " << nodep);
            UINFO(4, "Var2 " << fnodep);
            // Quote the node's own name as for messages, if it is the symbol's
            स्थिरांक string nameQ
                = fnodep->name() == name ? fnodep->prettyNameQ() : Ast_गाठ::prettyNameQ(name);
            जर (nodep->type() == fnodep->type()) {
                nodep->v3error("Duplicate declaration of "
                               << गाठ_Text_Type(fnodep) << ": " << nameQ << '\n'
                               << nodep->warnContextPrimary() << '\n'
                               << fnodep->warnOther() << "... Location of original declaration\n"
                               << fnodep->warnContextSecondary());
                nodep->decl_Token_अंक_Set_Min(0);  // Disable checking forward typedefs
                fnodep->decl_Token_अंक_Set_Min(0);  // Disable checking forward typedefs
            } अन्यथा {
                nodep->v3error("Unsupported in C: "
                               << ucfirst(गाठ_Text_Type(nodep)) << " has the same name as "
                               << गाठ_Text_Type(fnodep) << ": " << nameQ << '\n'
                               << nodep->warnContextPrimary() << '\n'
                               << fnodep->warnOther() << "... Location of original declaration\n"
                               << fnodep->warnContextSecondary());
            }
        }
    }
    शून्य घाला_DUnit(AstNetlist* nodep) {
        // $unit on top scope
        VSymEnt* स्थिरांक symp = नवीन VSymEnt{&m_syms, nodep};
        UINFO(9, "      INSERTdunit se" << cvtToHex(symp));
        symp->parentp(मूळ_Entp());  // Needed so backward search can find name of top module
        symp->fallbackp(nullptr);
        मूळ_Entp()->घाला("$unit ", symp);  // Space so can never name conflict with user code
        //
        UASSERT_OBJ(!m_dunitEntp, nodep, "Call insertDUnit only once");
        m_dunitEntp = symp;
    }
    VSymEnt* घाला_शीर्ष_Cell(Ast_गाठ_Module* nodep, स्थिरांक string& scopename) {
        // Only called on the module at the very top of the hierarchy
        VSymEnt* स्थिरांक symp = नवीन VSymEnt{&m_syms, nodep};
        UINFO(9, "      INSERTtop se" << cvtToHex(symp) << "  " << scopename << " " << nodep);
        symp->parentp(मूळ_Entp());  // Needed so backward search can find name of top module
        symp->fallbackp(dunitEntp());  // Needed so can find $unit stuff
        nodep->user1p(symp);
        checkDuplicate(मूळ_Entp(), nodep, nodep->origName());
        मूळ_Entp()->घाला(nodep->origName(), symp);
        जर (forScopeCreation()) m_nameScopeSymMap.emplace(scopename, symp);
        परत symp;
    }
    VSymEnt* घाला_शीर्ष_Iface(AstCell* nodep, स्थिरांक string& scopename) {
        VSymEnt* स्थिरांक symp = नवीन VSymEnt{&m_syms, nodep};
        UINFO(9, "      INSERTtopiface se" << cvtToHex(symp) << "  " << scopename << " " << nodep);
        symp->parentp(मूळ_Entp());  // Needed so backward search can find name of top module
        symp->fallbackp(dunitEntp());  // Needed so can find $unit stuff
        nodep->user1p(symp);
        जर (nodep->modp()) nodep->modp()->user1p(symp);
        checkDuplicate(मूळ_Entp(), nodep, nodep->origName());
        मूळ_Entp()->घाला(nodep->origName(), symp);
        जर (forScopeCreation()) m_nameScopeSymMap.emplace(scopename, symp);
        परत symp;
    }
    VSymEnt* घाला_Cell(VSymEnt* abovep, VSymEnt* modSymp, AstCell* nodep,
                        स्थिरांक string& scopename) {
        UASSERT_OBJ(abovep, nodep, "Null symbol table inserting node");
        VSymEnt* स्थिरांक symp = नवीन VSymEnt{&m_syms, nodep};
        UINFO(9, "      INSERTcel se" << cvtToHex(symp) << "  " << scopename << " above=se"
                                      << cvtToHex(abovep) << " mods=se" << cvtToHex(modSymp)
                                      << " node=" << nodep);
        symp->parentp(abovep);
        symp->fallbackp(dunitEntp());  // Needed so can find $unit stuff
        nodep->user1p(symp);
        जर (nodep->modp()) nodep->modp()->user1p(symp);
        checkDuplicate(abovep, nodep, nodep->origName());
        abovep->reinsert(nodep->origName(), symp);
        जर (forScopeCreation() && abovep != modSymp && !modSymp->findIdFlat(nodep->name())) {
            // If it's foo_DOT_bar, we need to be able to find it under "foo_DOT_bar" too.
            // Duplicates are possible, as until resolve generates might
            // have 2 same cells under an if
            modSymp->reinsert(nodep->name(), symp);
        }
        जर (forScopeCreation()) m_nameScopeSymMap.emplace(scopename, symp);
        परत symp;
    }
    शून्य घाला_Map(VSymEnt* symp, स्थिरांक string& scopename) {
        जर (forScopeCreation()) m_nameScopeSymMap.emplace(scopename, symp);
    }

    VSymEnt* घाला_Inline(VSymEnt* abovep, VSymEnt* modSymp, AstCellInline* nodep,
                          स्थिरांक string& basename) {
        // A fake point in the hierarchy, corresponding to a named block flattened by V3Begin.
        // Names are looked up in the module with the block's flattened prefix.
        UASSERT_OBJ(abovep, nodep, "Null symbol table inserting node");
        VSymEnt* स्थिरांक symp = नवीन VSymEnt{&m_syms, nodep};
        UINFO(9, "      INSERTinl se" << cvtToHex(symp) << "  " << basename << " above=se"
                                      << cvtToHex(abovep) << " mods=se" << cvtToHex(modSymp)
                                      << " node=" << nodep);
        symp->parentp(abovep);
        symp->fallbackp(modSymp);
        symp->symPrefix(nodep->name() + "__DOT__");
        nodep->user1p(symp);
        checkDuplicate(abovep, nodep, nodep->name());
        abovep->reinsert(basename, symp);
        जर (abovep != modSymp && !modSymp->findIdFlat(nodep->name())) {
            // If it's foo_DOT_bar, we need to be able to find it under that too.
            modSymp->reinsert(nodep->name(), symp);
        }
        परत symp;
    }
    VSymEnt* घाला_Block(VSymEnt* abovep, स्थिरांक string& name, Ast_गाठ* nodep,
                         Ast_गाठ_Module* classOrPackagep) {
        // A fake point in the hierarchy, corresponding to a begin or function/task block
        // After we remove begins these will go away
        // Note we fallback to the symbol table of the parent, as we want to find variables there
        // However, cells walk the graph, so cells will appear under the begin/ftask itself
        UASSERT_OBJ(abovep, nodep, "Null symbol table inserting node");
        VSymEnt* स्थिरांक symp = नवीन VSymEnt{&m_syms, nodep};
        UINFO(9, "      INSERTblk se" << cvtToHex(symp) << "  above=se" << cvtToHex(abovep)
                                      << " pkg=" << cvtToHex(classOrPackagep)
                                      << "  node=" << nodep);
        symp->parentp(abovep);
        symp->classOrPackagep(classOrPackagep);
        symp->fallbackp(abovep);
        nodep->user1p(symp);
        जर (name != "") checkDuplicate(abovep, nodep, name);
        // Duplicates are possible, as until resolve generates might have 2 same cells under an if
        abovep->reinsert(name, symp);
        परत symp;
    }
    VSymEnt* घाला_Sym(VSymEnt* abovep, स्थिरांक string& name, Ast_गाठ* nodep,
                       Ast_गाठ_Module* classOrPackagep) {
        UASSERT_OBJ(abovep, nodep, "Null symbol table inserting node");
        VSymEnt* स्थिरांक symp = नवीन VSymEnt{&m_syms, nodep};
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
        परत symp;
    }
    स्थिर तार्किक exists_गाठ_Sym(Ast_गाठ* nodep) { परत nodep->user1p() != nullptr; }
    स्थिर VSymEnt* get_गाठ_Sym(Ast_गाठ* nodep) {
        // Don't use this in ResolveVisitor, as we need to pick up the proper
        // reference under each SCOPE
        VSymEnt* स्थिरांक symp = nodep->user1u().toSymEnt();
        UASSERT_OBJ(symp, nodep, "Module/etc never assigned a symbol entry?");
        परत symp;
    }
    VSymEnt* getScopeSym(AstScope* nodep) {
        स्थिरांक स्वतः it = m_nameScopeSymMap.find(nodep->name());
        UASSERT_OBJ(it != m_nameScopeSymMap.end(), nodep,
                    "Scope never assigned a symbol entry '" << nodep->name() << "'");
        परत it->second;
    }
    शून्य implicitOkAdd(Ast_गाठ_Module* nodep, स्थिरांक string& varname) {
        // Mark the given variable name as being allowed to be implicitly declared
        जर (nodep) {
            स्थिरांक स्वतः it = m_implicitNameSet.find(std::make_pair(nodep, varname));
            जर (it == m_implicitNameSet.end()) m_implicitNameSet.emplace(nodep, varname);
        }
    }
    तार्किक implicitOk(Ast_गाठ_Module* nodep, स्थिरांक string& varname) {
        परत nodep
               && (m_implicitNameSet.find(std::make_pair(nodep, varname))
                   != m_implicitNameSet.end());
    }

    // Track and later recurse interface modules
    शून्य घाला_Iface_Mod_Sym(AstIface* nodep, VSymEnt* symp) {
        m_ifaceModSyms.emplace_back(nodep, symp);
    }
    शून्य computeIfaceModSyms();

    // Track and later insert interface references
    शून्य घाला_Iface_Var_Sym(VSymEnt* symp) {  // Where sym is for a VAR of dtype IFACEREFDTYPE
        m_ifaceVarSyms.ढकला_back(symp);
    }
    // Iface for a raw or arrayed iface; peels nested array layers for multi-dim arrays.
    स्थिर AstIfaceRefDType* ifaceRefFromArray(Ast_गाठ_DType* nodep) {
        पर्यंत (nodep) {
            जर (AstIfaceRefDType* स्थिरांक ifp = VN_CAST(nodep, IfaceRefDType)) परत ifp;
            जर (स्थिरांक AstBracketArrayDType* स्थिरांक arrp = VN_CAST(nodep, BracketArrayDType)) {
                nodep = arrp->subDTypep();
            } अन्यथा जर (स्थिरांक AstUnpackArrayDType* स्थिरांक arrp = VN_CAST(nodep, UnpackArrayDType)) {
                nodep = arrp->subDTypep();
            } अन्यथा {
                परत nullptr;
            }
        }
        परत nullptr;
    }
    // Given a pin expression, resolve it to a live AstIface* (or nullptr).
    // Handles both simple VarRef and dotted VarXRef pin connections.
    AstIface* live_Iface_From_Pin_अभिव्यक्ती(Ast_गाठ* exprp, VSymEnt* parentSymp) {
        AstVar* resolvedVarp = nullptr;
        जर (AstVarRef* स्थिरांक refp = VN_CAST(exprp, VarRef)) {
            resolvedVarp = refp->varp();
        } अन्यथा जर (AstVarXRef* स्थिरांक xrefp = VN_CAST(exprp, VarXRef)) {
            VSymEnt* स्थिरांक lookupSymp = parentSymp ? parentSymp->parentp() : nullptr;
            जर (lookupSymp) {
                string baddot;
                VSymEnt* okSymp = nullptr;
                स्थिरांक string dotpath = xrefp->dotted() + "." + xrefp->name();
                VSymEnt* स्थिरांक dotSymp
                    = findDotted(xrefp->fileline(), lookupSymp, dotpath, baddot, okSymp, सत्य);
                जर (dotSymp) resolvedVarp = VN_CAST(dotSymp->nodep(), Var);
            }
        }
        AstIface* resultp = nullptr;
        जर (resolvedVarp) {
            AstIfaceRefDType* स्थिरांक irefp = ifaceRefFromArray(resolvedVarp->subDTypep());
            जर (irefp && irefp->ifaceViaCellp() && !irefp->ifaceViaCellp()->dead()) {
                resultp = irefp->ifaceViaCellp();
            }
        }
        परत resultp;
    }

    // Attempt to repair a port's AstIfaceRefDType by tracing through the
    // parent cell's pin connection to find the correct live interface.
    // Returns true if repaired or already correct; false if unresolvable.
    तार्किक repairIfaceRef(VSymEnt* varSymp, AstVar* varp, AstIfaceRefDType* ifacerefp, तार्किक isDead) {
        // Walk up symbol-table parents to find the enclosing AstCell
        VSymEnt* parentSymp = varSymp->parentp();
        AstCell* parentCellp = nullptr;
        साठी (पूर्णांक depth = 0; parentSymp && depth < 20; ++depth) {
            जर (AstCell* स्थिरांक cp = VN_CAST(parentSymp->nodep(), Cell)) {
                parentCellp = cp;
                थांबा;
            }
            parentSymp = parentSymp->parentp();
        }
        जर (!parentCellp) परत असत्य;
        // Scan pins to find the one connected to this port variable
        साठी (AstPin* pinp = parentCellp->pinsp(); pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            जर (pinp->modVarp() != varp && pinp->name() != varp->name()) सुरूठेवा;
            Ast_गाठ* स्थिरांक exprp = pinp->exprp();
            जर (!exprp) परत असत्य;
            AstIface* स्थिरांक newIfacep = live_Iface_From_Pin_अभिव्यक्ती(exprp, parentSymp);
            जर (newIfacep && newIfacep != ifacerefp->ifaceViaCellp()) {
                UINFO(4, "  REPAIR-IFACE-REF var=" << varp->prettyNameQ()
                                                   << " old=" << ifacerefp->ifacep()->prettyNameQ()
                                                   << " new=" << newIfacep->prettyNameQ());
                ifacerefp->ifacep(newIfacep);
                परत सत्य;
            }
            जर (newIfacep) परत !isDead;  // Same interface - OK if live
            परत असत्य;
        }
        परत असत्य;
    }

    शून्य computeIfaceVarSyms() {
        साठी (VSymEnt* varSymp : m_ifaceVarSyms) {
            AstVar* स्थिरांक varp = varSymp ? VN_AS(varSymp->nodep(), Var) : nullptr;
            UINFO(9, "  insAllIface se" << cvtToHex(varSymp) << " " << varp);
            AstIfaceRefDType* स्थिरांक ifacerefp = ifaceRefFromArray(varp->subDTypep());
            UASSERT_OBJ(ifacerefp, varp, "Non-ifacerefs on list!");
            जर (!ifacerefp->cellp() && !ifacerefp->isVirtual() && (varp->pin_अंक() == 0)) {
                varp->v3error("Interface port declaration "
                              << varp->prettyNameQ() << " doesn't have corresponding port\n"
                              << varp->warnMore()
                                     + "... Perhaps intended an interface instantiation but "
                                       "are missing parenthesis (IEEE 1800-2023 25.3)?");
            }
            जर (!ifacerefp->ifaceViaCellp()) {
                जर (!ifacerefp->cellp()) {  // Probably a NotFoundModule, or a normal module if
                                            // made mistake
                    UINFO(1, "Associated cell " << Ast_गाठ::prettyNameQ(ifacerefp->cellName()));
                    ifacerefp->v3error("Cannot find file containing interface: "
                                       << Ast_गाठ::prettyNameQ(ifacerefp->ifaceName()));
                    सुरूठेवा;
                } अन्यथा {
                    ifacerefp->v3fatalSrc("Unlinked interface");
                }
            } अन्यथा जर (varp->isIfaceRef() && !ifacerefp->cellp() && ifacerefp->ifaceViaCellp()) {
                // Port variable (cellp=null) pointing to an interface.
                // The interface may be dead (template replaced by clone) or
                // live-but-wrong (template still alive because some instances
                // use default params, but this port should point to a
                // specialized clone based on the pin connection).
                स्थिरांक तार्किक isDead = ifacerefp->ifaceViaCellp()->dead();
                जर (isDead && forPrimary() && !varp->isIfaceParent()
                    && !v3Global.opt.शीर्ष_Ifaces_Supported()) {
                    varp->v3warn(E_UNSUPPORTED,
                                 "Unsupported: Interfaced port on top level module");
                }
                // Attempt repair: trace through parent cell's pin to find
                // the correct live interface for this port variable
                स्थिरांक तार्किक repaired = repairIfaceRef(varSymp, varp, ifacerefp, isDead);
                जर (!repaired && isDead) {
                    ifacerefp->v3error(
                        "Interface " << Ast_गाठ::prettyNameQ(ifacerefp->ifaceName())
                                     << " not connected as parent's interface not connected\n"
                                     << ifacerefp->warnMore()
                                     << "... Perhaps caused by another error on the parent "
                                        "interface that needs resolving\n"
                                     << ifacerefp->warnMore()
                                     << "... Or, perhaps intended an interface instantiation but "
                                        "are missing parenthesis (IEEE 1800-2023 25.3)?");
                    सुरूठेवा;
                }
            } अन्यथा जर (ifacerefp->ifaceViaCellp()->dead()
                       || !exists_गाठ_Sym(ifacerefp->ifaceViaCellp())) {
                ifacerefp->ifaceViaCellp()->v3fatalSrc(
                    "Interface referenced by virtual iface is dead. "
                    "Dead interfaces have required linking steps skipped");
            }
            VSymEnt* स्थिरांक ifaceSymp = get_गाठ_Sym(ifacerefp->ifaceViaCellp());
            VSymEnt* ifOrPortSymp = ifaceSymp;
            // Link Modport names to the Modport Node under the Interface
            जर (ifacerefp->isModport()) {
                VSymEnt* स्थिरांक foundp = ifaceSymp->findIdFallback(ifacerefp->modportName());
                तार्किक ok = असत्य;
                जर (foundp) {
                    जर (AstModport* स्थिरांक modportp = VN_CAST(foundp->nodep(), Modport)) {
                        UINFO(4, "Link Modport: " << modportp);
                        ifacerefp->modportp(modportp);
                        ifOrPortSymp = foundp;
                        ok = सत्य;
                    }
                }
                जर (!ok) {
                    स्थिरांक string suggest = suggestSymFallback(ifaceSymp, ifacerefp->modportName(),
                                                              Link_गाठ_Matcher_Modport{});
                    ifacerefp->modportFileline()->v3error(
                        "Modport not found under interface "
                        << ifacerefp->prettyNameQ(ifacerefp->ifaceName()) << ": "
                        << ifacerefp->prettyNameQ(ifacerefp->modportName()) << '\n'
                        << (suggest.empty() ? "" : ifacerefp->warnMore() + suggest));
                }
            }
            // Alias won't expand until interfaces and modport names are known; see notes at top
            घाला_Scope_Alias(SAMN_IFTOP, varSymp, ifOrPortSymp);
        }
        m_ifaceVarSyms.clear();
    }

    शून्य घाला_Scope_Alias(SAMNum samn, VSymEnt* lhsp, VSymEnt* rhsp) {
        // Track and later insert scope aliases; an interface referenced by
        // a child cell connecting to that interface
        // Typically lhsp=VAR w/dtype IFACEREF, rhsp=IFACE cell
        UINFO(9, "   insertScopeAlias se" << cvtToHex(lhsp) << " se" << cvtToHex(rhsp));
        UASSERT_OBJ(
            !(VN_IS(rhsp->nodep(), Cell) && !VN_IS(VN_AS(rhsp->nodep(), Cell)->modp(), Iface)),
            rhsp->nodep(), "Got a non-IFACE alias RHS");
        m_scopeAliasMap[samn].emplace(lhsp, rhsp);
    }
    शून्य computeScopeAliases() {
        UINFO(9, "computeIfaceAliases");
        साठी (पूर्णांक samn = 0; samn < SAMN__MAX; ++samn) {
            साठी (ScopeAliasMap::iterator it = m_scopeAliasMap[samn].begin();
                 it != m_scopeAliasMap[samn].end(); ++it) {
                VSymEnt* स्थिरांक lhsp = it->first;
                VSymEnt* srcp = lhsp;
                पर्यंत (सत्य) {  // Follow chain of aliases up to highest level non-alias
                    स्थिरांक स्वतः it2 = m_scopeAliasMap[samn].find(srcp);
                    जर (it2 != m_scopeAliasMap[samn].end()) {
                        srcp = it2->second;
                        सुरूठेवा;
                    } अन्यथा {
                        थांबा;
                    }
                }
                UINFO(9, "  iiasa: Insert alias se" << lhsp << " (" << lhsp->nodep()->typeName()
                                                    << ") <- se" << srcp << " " << srcp->nodep());
                // srcp should be an interface reference pointing to the interface we want to
                // import
                lhsp->importFromIface(symsp(), srcp);
                // Allow access to objects not permissible to be listed in a modport
                जर (VN_IS(srcp->nodep(), Modport)) {
                    lhsp->importFromIface(symsp(), srcp->parentp(), सत्य);
                }
            }
            // m_scopeAliasMap[samn].clear();  // Done with it, but put into debug file
        }
    }

खाजगी:
    VSymEnt* findWithAltFallback(VSymEnt* symp, स्थिरांक string& name, स्थिरांक string& altname) {
        VSymEnt* findp = symp->findIdFallback(name);
        जर (findp) परत findp;
        जर (altname != "") {
            UINFO(8, "     alt fallback");
            findp = symp->findIdFallback(altname);
        }
        परत findp;
    }

    VSymEnt* findWithAltFlat(VSymEnt* symp, स्थिरांक string& name, स्थिरांक string& altname) {
        VSymEnt* findp = symp->findIdFlat(name);
        जर (findp) परत findp;
        जर (altname != "") {
            UINFO(8, "     alt flat");
            findp = symp->findIdFlat(altname);
        }
        परत findp;
    }

सार्वजनिक:
    VSymEnt* findDotted(File_ओळ* refLocationp, VSymEnt* lookupSymp, स्थिरांक string& dotname,
                        string& baddot, VSymEnt*& okSymp, तार्किक firstId) {
        // Given a dotted hierarchy name, return where in scope it is
        // Note when dotname=="" we just fall through and return lookupSymp
        UINFO(8, "    dottedFind se" << cvtToHex(lookupSymp) << " '" << dotname << "'");
        string leftname = dotname;
        okSymp = lookupSymp;  // So can list bad scopes
        पर्यंत (leftname != "") {  // foreach dotted part of xref name
            string::size_type pos = leftname.find('.');
            string ident;
            जर (pos != string::npos) {
                ident = leftname.substr(0, pos);
                leftname = leftname.substr(pos + 1);
            } अन्यथा {
                ident = leftname;
                leftname = "";
            }
            baddot = ident;  // So user can see where they botched it
            okSymp = lookupSymp;
            string altIdent;
            जर (!forScopeCreation()) {
                // GENFOR Begin is foo__BRA__##__KET__ after we've genloop unrolled,
                // but presently should be just "foo".
                // Likewise cell foo__[array] before we've expanded arrays is just foo.
                // Multi-dim iface arrays append multiple __BRA__..__KET__ suffixes; strip them
                // all.
                altIdent = ident;
                पर्यंत (VString::endsWith(altIdent, "__KET__")) {
                    स्थिरांक स्वतः braPos = altIdent.rfind("__BRA__");
                    जर (braPos == string::npos) थांबा;
                    altIdent = altIdent.substr(0, braPos);
                }
                जर (altIdent == ident) altIdent.clear();
            }
            UINFO(8, "         id " << ident << " alt " << altIdent << " left " << leftname
                                    << " at se" << lookupSymp);
            // Spec says; Look at existing module (cellnames then modname),
            // then look up (inst name or modname)
            जर (firstId) {
                // Check this module - subcellnames
                स्थिरांक AstCell* cellp = lookupSymp ? VN_CAST(lookupSymp->nodep(), Cell)
                                                  : nullptr;  // Replicated below
                जर (VSymEnt* स्थिरांक findSymp = findWithAltFallback(lookupSymp, ident, altIdent)) {
                    lookupSymp = findSymp;
                }

                // Check this module - cur modname
                अन्यथा जर (cellp && cellp->modp()->origName() == ident) {
                }
                // $root we walk up to Netlist
                अन्यथा जर (ident == "$root") {
                    lookupSymp = मूळ_Entp();
                    // We've added the '$root' module, now everything else is one lower
                    जर (forScopeCreation()) {
                        lookupSymp = lookupSymp->findIdFlat(ident);
                        UASSERT(lookupSymp, "Cannot find $root module under netlist");
                    }
                }
                // Move up and check cellname + modname
                अन्यथा {
                    तार्किक crossedCell = असत्य;  // Crossed a cell boundary
                    पर्यंत (lookupSymp) {
                        lookupSymp = lookupSymp->parentp();
                        cellp = lookupSymp ? VN_CAST(lookupSymp->nodep(), Cell)
                                           : nullptr;  // Replicated above
                        जर (lookupSymp) {
                            UINFO(9, "    Up to " << lookupSymp);
                            जर (cellp || VN_IS(lookupSymp->nodep(), CellInline)) {
                                crossedCell = सत्य;
                            }
                            जर (cellp && cellp->modp()->origName() == ident) {
                                थांबा;
                            } अन्यथा जर (VSymEnt* स्थिरांक findSym2p
                                       = findWithAltFallback(lookupSymp, ident, altIdent)) {
                                lookupSymp = findSym2p;
                                जर (crossedCell && VN_IS(lookupSymp->nodep(), Var)) {
                                    UINFO(9, "    Not found but matches var name in parent "
                                                 << lookupSymp);
                                    परत nullptr;
                                }
                                थांबा;
                            }
                        } अन्यथा {
                            थांबा;
                        }
                    }
                    जर (!lookupSymp) परत nullptr;  // Not found
                }
            } अन्यथा {  // Searching for middle path component, must be a cell or interface port
                VSymEnt* findSymp = findWithAltFlat(lookupSymp, ident, altIdent);
                जर (findSymp) {
                    lookupSymp = findSymp;
                } अन्यथा {
                    // Try prefixed lookup for interface ports accessed through hierarchy
                    // (e.g., slave_inst.bus.data where bus is an interface port)
                    // After inlining, interface ports have prefixed names like
                    // top__DOT__slave_inst__DOT__bus
                    UINFO(8, "     middle-path: trying prefixed lookup for '" << ident << "'\n");
                    string baddot;
                    findSymp = findSymPrefixed(lookupSymp, ident, baddot, /*fallback=*/सत्य);
                    UINFO(8, "     middle-path: prefixed lookup result: "
                                 << (findSymp ? "found" : "not found") << "\n");
                    जर (findSymp) {
                        // Check if this is an interface reference variable
                        स्थिरांक AstVar* varp = VN_CAST(findSymp->nodep(), Var);
                        जर (!varp) {
                            जर (स्थिरांक AstVarScope* vscp = VN_CAST(findSymp->nodep(), VarScope)) {
                                varp = vscp->varp();
                            }
                        }
                        जर (varp && varp->isIfaceRef()) {
                            // Found an interface port - use findSymp directly
                            // computeScopeAliases has already imported interface members
                            // into this symbol entry, so we use it instead of redirecting
                            // to the shared modport definition (which would cause all
                            // instances to resolve to the same symbols - bug #2656)
                            lookupSymp = findSymp;
                        } अन्यथा {
                            // Non-interface symbol in middle path must be a cell (module instance)
                            // to continue hierarchical resolution
                            जर (VN_IS(findSymp->nodep(), Cell)) {
                                lookupSymp = findSymp;
                            } अन्यथा {
                                // Reject non-cell, non-interface symbols found via fallback
                                // to prevent accidental resolution to unrelated symbols
                                UINFO(8, "     middle-path: rejecting non-cell symbol\n");
                                परत nullptr;
                            }
                        }
                    } अन्यथा {
                        परत nullptr;  // Not found
                    }
                }
            }
            जर (lookupSymp) {
                जर (स्थिरांक AstCell* स्थिरांक cellp = VN_CAST(lookupSymp->nodep(), Cell)) {
                    जर (स्थिरांक Ast_गाठ_Module* स्थिरांक modp = cellp->modp()) {
                        जर (modp->hierBlock() && !leftname.empty()) {
                            refLocationp->v3error("Cannot access scope inside hierarchical block");
                        } अन्यथा जर (VN_IS(modp, NotFoundModule)) {
                            refLocationp->v3error("Dotted reference to instance that refers to "
                                                  "missing module/interface: "
                                                  << modp->prettyNameQ());
                        }
                    }
                }
                // Follow scope alias for nested interface port access
                जर (!leftname.empty()) {
                    स्थिरांक स्वतः aliasIt = m_scopeAliasMap[SAMN_IFTOP].find(lookupSymp);
                    जर (aliasIt != m_scopeAliasMap[SAMN_IFTOP].end()) {
                        lookupSymp = aliasIt->second;
                        // Alias may point to __Viftop VarScope; find corresponding Cell
                        जर (स्थिरांक AstVarScope* स्थिरांक vscp
                            = VN_CAST(lookupSymp->nodep(), VarScope)) {
                            स्थिरांक string varName = vscp->varp()->name();
                            स्थिर constexpr स्थिरांक अक्षर* स्थिरांक VIFTOP_SUFFIX = "__Viftop";
                            जर (VString::endsWith(varName, VIFTOP_SUFFIX)) {
                                स्थिरांक string cellName
                                    = varName.substr(0, varName.size() - म_माप(VIFTOP_SUFFIX));
                                VSymEnt* स्थिरांक parentSymp = lookupSymp->parentp();
                                जर (parentSymp) {
                                    VSymEnt* स्थिरांक cellSymp = parentSymp->findIdFlat(cellName);
                                    जर (cellSymp && VN_IS(cellSymp->nodep(), Cell)) {
                                        lookupSymp = cellSymp;  // Use Cell for member lookup
                                    }
                                }
                            }
                        }
                    } अन्यथा {
                        // No alias; try following IfaceRefDType to interface cell
                        स्थिरांक AstVar* varp = VN_CAST(lookupSymp->nodep(), Var);
                        जर (!varp) {
                            जर (स्थिरांक AstVarScope* स्थिरांक vscp
                                = VN_CAST(lookupSymp->nodep(), VarScope)) {
                                varp = vscp->varp();
                            }
                        }
                        जर (varp && varp->isIfaceRef()) {
                            जर (स्थिरांक AstIfaceRefDType* स्थिरांक ifaceRefp
                                = ifaceRefFromArray(varp->dtypep())) {
                                जर (ifaceRefp->cellp() && exists_गाठ_Sym(ifaceRefp->cellp())) {
                                    lookupSymp = get_गाठ_Sym(ifaceRefp->cellp());
                                } अन्यथा जर (ifaceRefp->ifaceViaCellp()
                                           && exists_गाठ_Sym(ifaceRefp->ifaceViaCellp())) {
                                    lookupSymp = get_गाठ_Sym(ifaceRefp->ifaceViaCellp());
                                } अन्यथा जर (ifaceRefp->ifacep()
                                           && exists_गाठ_Sym(ifaceRefp->ifacep())) {
                                    lookupSymp = get_गाठ_Sym(ifaceRefp->ifacep());
                                }
                            }
                        }
                    }
                }
            }
            firstId = असत्य;
        }
        परत lookupSymp;
    }

    स्थिर string removeLastInlineScope(स्थिरांक string& name) {
        string out = name;
        स्थिरांक string dot = "__DOT__";
        स्थिरांक माप_प्रकार dotPos = out.rfind(dot, out.size() - dot.length() - 1);
        जर (dotPos == string::npos) {
            परत "";
        } अन्यथा {
            परत out.erase(dotPos + dot.length(), string::npos);
        }
    }

    VSymEnt* findSymPrefixed(VSymEnt* lookupSymp, स्थिरांक string& dotname, string& baddot,
                             तार्किक fallback) {
        // Find symbol in given point in hierarchy, allowing prefix (post-Inline)
        // For simplicity lookupSymp may be passed nullptr result from findDotted
        जर (!lookupSymp) परत nullptr;
        UINFO(8, "    findSymPrefixed "
                     << dotname << " under se" << cvtToHex(lookupSymp)
                     << ((lookupSymp->symPrefix() == "") ? "" : " as ")
                     << ((lookupSymp->symPrefix() == "") ? "" : lookupSymp->symPrefix() + dotname)
                     << "  at se" << lookupSymp << " fallback=" << fallback);
        string prefix = lookupSymp->symPrefix();
        VSymEnt* foundp = nullptr;
        पर्यंत (!foundp) {
            जर (fallback) {
                foundp = lookupSymp->findIdFallback(prefix + dotname);  // Might be nullptr
            } अन्यथा {
                foundp = lookupSymp->findIdFlat(prefix + dotname);  // Might be nullptr
            }
            जर (prefix.empty()) थांबा;
            स्थिरांक string nextPrefix = removeLastInlineScope(prefix);
            जर (prefix == nextPrefix) थांबा;
            prefix = nextPrefix;
        }
        जर (!foundp) baddot = dotname;
        परत foundp;
    }
    स्थिर तार्किक checkIfClassOrPackage(स्थिरांक VSymEnt* स्थिरांक symp) {
        जर (VN_IS(symp->nodep(), Class) || VN_IS(symp->nodep(), Package)) परत सत्य;

        // Helper: check if a RefDType might resolve to a class later.
        स्थिरांक स्वतः isPotentialClassRef = [](स्थिरांक AstRefDType* refp) -> तार्किक {
            परत refp && !refp->typeofp()
                   && (!refp->classOrPackageOpp()
                       || VN_IS(refp->classOrPackageOpp(), ClassOrPackageRef));
        };

        // Helper: allow types that can represent a class/package handle or an unresolved ref.
        // BasicDType is allowed here only for null/object handles.
        स्थिरांक स्वतः is_Valid_Type_गाठ = [](स्थिरांक Ast_गाठ* nodep) -> तार्किक {
            परत VN_IS(nodep, VoidDType) || VN_IS(nodep, BasicDType)
                   || VN_IS(nodep, ClassRefDType) || VN_IS(nodep, ParseRef);
        };

        जर (स्थिरांक AstTypedef* स्थिरांक typedefp = VN_CAST(symp->nodep(), Typedef)) {
            स्थिरांक Ast_गाठ_DType* dtypep = typedefp->subDTypep();
            जर (!dtypep) dtypep = typedefp->childDTypep();
            जर (VN_IS(dtypep, ClassRefDType)) परत सत्य;
            जर (isPotentialClassRef(VN_CAST(dtypep, RefDType))) परत सत्य;
        } अन्यथा जर (स्थिरांक AstParamTypeDType* स्थिरांक paramTypep
                   = VN_CAST(symp->nodep(), ParamTypeDType)) {
            // Before V3Param the declared default is in childDTypep (possibly
            // wrapped in a RequireDType); after V3Param it is consumed and the
            // bound type is the resolved data type, e.g. a type parameter
            // inherited from a specialized base class (REQ #(Item) -> class Item).
            Ast_गाठ* childp = paramTypep->childDTypep();
            जर (स्थिरांक AstRequireDType* स्थिरांक reqp = VN_CAST(childp, RequireDType)) {
                childp = reqp->lhsp();
            }
            स्थिरांक Ast_गाठ* स्थिरांक checkp = childp ? childp : paramTypep->skipRefp();
            जर (is_Valid_Type_गाठ(checkp) || isPotentialClassRef(VN_CAST(checkp, RefDType))) {
                परत सत्य;
            }
        }
        परत असत्य;
    }
    VSymEnt* resolveClassOrPackage(VSymEnt* lookSymp, AstClassOrPackageRef* nodep, तार्किक fallback,
                                   तार्किक classOnly, स्थिरांक string& forWhat,
                                   तार्किक deferIfUnresolved = असत्य) {
        जर (nodep->classOrPackageSkipp()) परत get_गाठ_Sym(nodep->classOrPackageSkipp());
        VSymEnt* foundp;
        VSymEnt* शोध_Symp = lookSymp;
        जर (VL_UNCOVERABLE(शोध_Symp && VN_IS(शोध_Symp->nodep(), ParamTypeDType))) {
            शोध_Symp->nodep()->v3fatalSrc(  // LCOV_EXCL_LINE
                "resolveClassOrPackage: unexpected ParamTypeDType lookup");
        }
        जर (fallback) {
            VSymEnt* currentLookSymp = शोध_Symp;
            करा {
                foundp = currentLookSymp->findIdFlat(nodep->name());
                जर (foundp && !checkIfClassOrPackage(foundp)) foundp = nullptr;
                जर (!foundp) currentLookSymp = currentLookSymp->fallbackp();
            } पर्यंत (!foundp && currentLookSymp);
        } अन्यथा {
            foundp = शोध_Symp->findIdFlat(nodep->name());
            जर (foundp && !checkIfClassOrPackage(foundp)) foundp = nullptr;
        }
        जर (!foundp && v3Global.rootp()->stdPackagep()) {  // Look under implied std::
            foundp = get_गाठ_Sym(v3Global.rootp()->stdPackagep())->findIdFlat(nodep->name());
        }
        जर (foundp) {
            nodep->classOrPackageNodep(foundp->nodep());
            परत foundp;
        }
        जर (deferIfUnresolved) परत nullptr;
        स्थिरांक string suggest
            = suggestSymFallback(lookSymp, nodep->name(), Link_गाठ_Matcher_Class_Or_Package{});
        nodep->v3error((classOnly ? "Class" : "Package/class")
                       << " for '" << forWhat  // extends/implements
                       << "' not found: " << nodep->prettyNameQ() << '\n'
                       << (suggest.empty() ? "" : nodep->warnMore() + suggest));
        परत nullptr;
    }
    string suggestSymFallback(VSymEnt* lookupSymp, स्थिरांक string& name,
                              स्थिरांक VNodeMatcher& matcher) {
        // Suggest alternative symbol in given point in hierarchy
        // Does not support inline, as we find user-level errors before inlining
        // For simplicity lookupSymp may be passed nullptr result from findDotted
        जर (!lookupSymp) परत "";
        VSpellCheck speller;
        lookupSymp->candidateIdFallback(&speller, &matcher);
        परत speller.best_Candidate_संदेश(name);
    }
    string suggestSymFlat(VSymEnt* lookupSymp, स्थिरांक string& name, स्थिरांक VNodeMatcher& matcher,
                          तार्किक includeType) {
        जर (!lookupSymp) परत "";
        VSpellCheck speller;
        lookupSymp->candidateIdFlat(&speller, &matcher, includeType);
        परत speller.best_Candidate_संदेश(name);
    }
};

LinkDotState* LinkDotState::s_errorThisp = nullptr;

//======================================================================

वर्ग LinkDotFindVisitor final : सार्वजनिक VNVisitor {
    // NODE STATE
    // Cleared on global
    //  *::user1p()             -> See LinkDotState
    //  *::user2p()             -> See LinkDotState
    //  *::user3()              // bool. Node stored in m_virtualIfaces
    //  *::user4()              -> See LinkDotState
    स्थिरांक VNUser3InUse m_inuser3;

    // STATE
    LinkDotState* स्थिरांक m_statep;  // State to pass between visitors, including symbol table
    Ast_गाठ_Module* m_classOrPackagep = nullptr;  // Current package
    AstClocking* m_clockingp = nullptr;  // Current clocking block
    std::vector<AstScope*> m_scopes;  // Scopes detached during repeated module visits
    uint64_t m_statScopeVisits = 0;  // Non-top scopes encountered while finding instance symbols
    VSymEnt* m_modSymp = nullptr;  // Symbol Entry for current module
    VSymEnt* m_curSymp = nullptr;  // Symbol Entry for current table, where to lookup/insert
    string
        m_hierParamsName;  // Name of module with hierarchical type parameters, empty when not used
    string m_scope;  // Scope text
    स्थिरांक Ast_गाठ_FTask* m_ftaskp = nullptr;  // Current function/task
    तार्किक m_inRecursion = असत्य;  // Inside a recursive module
    पूर्णांक m_param_अंक = 0;  // Parameter number, for position based connection
    तार्किक m_explicitNew = असत्य;  // Hit a "new" function
    पूर्णांक m_mod_Block_अंक = 0;  // Begin block number in module, 0=none seen
    पूर्णांक m_mod_With_अंक = 0;  // With block number, 0=none seen
    पूर्णांक m_mod_Arg_अंक = 0;  // Arg block number for randomize(), 0=none seen
    std::vector<AstIface*> m_virtIfaces;  // interfaces used as virtual,
                                          // needed for handleUnvisitedVirtIfaces()

    // METHODS
    शून्य makeImplicitNew(AstClass* nodep) {
        AstFunc* स्थिरांक newp = नवीन AstFunc{nodep->fileline(), "new", nullptr, nullptr};
        // If needed, super.new() call is added the 2nd pass of V3LinkDotResolve,
        // because base classes are already resolved there.
        newp->isConstructor(सत्य);
        nodep->addMembersp(newp);
        UINFO(8, "Made implicit new for " << nodep->name() << ": " << nodep);
        m_statep->घाला_Block(m_curSymp, newp->name(), newp, m_classOrPackagep);
    }

    स्थिर तार्किक isHierBlockWrapper(स्थिरांक string& name) {
        स्थिरांक V3HierBlockOptSet& hierBlocks = v3Global.opt.hierBlocks();
        परत hierBlocks.find(name) != hierBlocks.end();
    }

    शून्य moveExternFuncDecl(Ast_गाठ_FTask* nodep, AstClass* classp) {
        // Move an 'extern function|task' to inside a class or, from an outer to inner-class
        जर (nodep->isExternDef()) परत;
        // Move it to proper spot under the target class
        nodep->unlinkFrBack();
        classp->addStmtsp(nodep);
        nodep->isExternDef(सत्य);  // So we check there's a matching extern
        nodep->classOrPackagep()->unlinkFrBack()->delete_वृक्ष();
        // Any "Type::" reference in the function's IO are really "MovedToClass::" references
        जर (nodep->fvarp()) moveExternFuncDeclRefs(nodep->fvarp(), classp);
        साठी (Ast_गाठ* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            जर (AstVar* स्थिरांक portp = VN_CAST(stmtp, Var)) {
                जर (portp->isIO()) moveExternFuncDeclRefs(portp, classp);
            }
        }
    }
    शून्य moveExternFuncDeclRefs(Ast_गाठ* nodep, AstClass* classp) {
        nodep->foreach([हे, classp](AstClassOrPackageRef* refp) {  //
            जर (refp->name() == classp->name() && !refp->paramsp()) {
                UINFO(9, "Cleaning up external function type for class " << refp);
                AstDot* स्थिरांक dotp = VN_CAST(refp->backp(), Dot);
                जर (dotp && dotp->lhsp() == refp) {
                    // If had DOT(CLASSREF this, ...) the DOT can go away
                    dotp->replaceWith(dotp->rhsp()->unlinkFrBack());
                    VL_DO_DANGLING2(ढकला_Deletep(dotp), dotp, refp);
                } अन्यथा {
                    VL_DO_DANGLING(ढकला_Deletep(refp->unlinkFrBack()), refp);
                }
            }
        });
    }

    // VISITORS
    शून्य visit(AstNetlist* nodep) override {  // FindVisitor::
        // Process $unit or other packages
        // Not needed - dotted references not allowed from inside packages
        // for (AstNodeModule* nodep = v3Global.rootp()->modulesp();
        //     nodep; nodep=VN_AS(nodep->nextp(), NodeModule)) {
        //    if (VN_IS(nodep, Package)) {}}

        m_statep->घाला_DUnit(nodep);

        // First back iterate, to find all packages. Backward as must do base
        // packages before using packages
        iterateChildrenBackwardsConst(nodep);

        // The first modules in the list are always the top modules
        // (sorted before this is called).
        // This may not be the module with isTop() set, as early in the steps,
        // wrapTop may have not been created yet.
        // $unit and the constant pool always exist, so nothing else, and nothing in $unit,
        // means nothing was given
        Ast_गाठ_Module* स्थिरांक modulesp = nodep->modulesp();
        UASSERT_OBJ(modulesp, nodep, "$unit should always be in the netlist");
        जर (m_statep->forPrimary()) {
            स्थिरांक तार्किक onlyBuiltins = [&]() {
                साठी (Ast_गाठ_Module* modp = modulesp; modp;
                     modp = VN_AS(modp->nextp(), गाठ_Module)) {
                    जर (!modp->isDollarUnit() && !modp->isConstPool()) परत असत्य;
                }
                परत सत्य;
            }();
            जर (onlyBuiltins && !v3Global.rootp()->dollarUnitPkgp()->stmtsp()) {
                nodep->v3error("No top level module found");
            }
        }
        साठी (Ast_गाठ_Module* modp = nodep->modulesp(); modp && modp->is_शीर्ष();
             modp = VN_AS(modp->nextp(), गाठ_Module)) {
            UINFO(8, "Top Module: " << modp);
            m_scope = "TOP";

            जर (!m_statep->forScopeCreation() && v3Global.opt.शीर्ष_Ifaces_Supported()) {
                साठी (Ast_गाठ* subnodep = modp->stmtsp(); subnodep; subnodep = subnodep->nextp()) {
                    जर (AstVar* स्थिरांक varp = VN_CAST(subnodep, Var)) {
                        जर (varp->isIfaceRef()) {
                            स्थिरांक Ast_गाठ_DType* स्थिरांक subtypep = varp->subDTypep();
                            स्थिरांक AstIfaceRefDType* ifacerefp = nullptr;
                            जर (VN_IS(subtypep, IfaceRefDType)) {
                                ifacerefp = VN_AS(varp->subDTypep(), IfaceRefDType);
                            } अन्यथा जर (VN_IS(subtypep, BracketArrayDType)) {
                                स्थिरांक AstBracketArrayDType* स्थिरांक arrp
                                    = VN_AS(subtypep, BracketArrayDType);
                                स्थिरांक Ast_गाठ_DType* स्थिरांक arrsubtypep = arrp->subDTypep();
                                जर (VN_IS(arrsubtypep, IfaceRefDType)) {
                                    ifacerefp = VN_AS(arrsubtypep, IfaceRefDType);
                                }
                            } अन्यथा जर (VN_IS(subtypep, UnpackArrayDType)) {
                                स्थिरांक AstUnpackArrayDType* स्थिरांक arrp
                                    = VN_AS(subtypep, UnpackArrayDType);
                                स्थिरांक Ast_गाठ_DType* स्थिरांक arrsubtypep = arrp->subDTypep();
                                जर (VN_IS(arrsubtypep, IfaceRefDType)) {
                                    ifacerefp = VN_AS(arrsubtypep, IfaceRefDType);
                                }
                            }

                            जर (ifacerefp && !ifacerefp->cellp()) {
                                // A dummy cell to keep the top level interface alive and correctly
                                // optimized for default parameter values
                                AstCell* ifacecellp
                                    = नवीन AstCell{nodep->fileline(),
                                                  nodep->fileline(),
                                                  modp->name() + "__02E" + varp->name(),
                                                  ifacerefp->ifaceName(),
                                                  nullptr,
                                                  nullptr,
                                                  nullptr};
                                ifacecellp->modp(ifacerefp->ifacep());
                                m_curSymp = m_modSymp
                                    = m_statep->घाला_शीर्ष_Iface(ifacecellp, m_scope);
                                iterate(ifacecellp);
                                VL_DO_DANGLING(ढकला_Deletep(ifacecellp), ifacecellp);
                            }
                        }
                    }
                }
            }

            m_curSymp = m_modSymp = m_statep->घाला_शीर्ष_Cell(modp, m_scope);
            { iterate(modp); }

            m_scope = "";
            m_curSymp = m_modSymp = nullptr;
        }
    }
    शून्य visit(AstTypeTable*) override {}  // FindVisitor::
    शून्य visit(AstIfaceRefDType* nodep) override {  // FindVisitor::
        जर (!m_statep->forScopeCreation() && nodep->isVirtual() && nodep->ifacep()
            && !nodep->ifacep()->user3()) {
            m_virtIfaces.ढकला_back(nodep->ifacep());
            nodep->ifacep()->user3(सत्य);
        }
        iterateChildren(nodep);
    }
    शून्य visit(Ast_गाठ_Module* nodep) override {  // FindVisitor::
        // Called on top module from Netlist, other modules from the cell creating them,
        // and packages
        UINFO(8, "   " << nodep);
        // m_curSymp/m_modSymp maybe nullptr for packages and non-top modules
        // Packages will be under top after the initial phases, but until then
        // need separate handling
        स्थिरांक तार्किक standalonePkg
            = !m_modSymp && !m_statep->forScopeCreation() && VN_IS(nodep, Package);
        स्थिरांक तार्किक doit = (m_modSymp || standalonePkg);
        VL_RESTORER_COPY(m_scope);
        VL_RESTORER(m_classOrPackagep);
        VL_RESTORER(m_modSymp);
        VL_RESTORER(m_curSymp);
        VL_RESTORER(m_param_अंक);
        VL_RESTORER(m_mod_Block_अंक);
        VL_RESTORER(m_mod_With_अंक);
        VL_RESTORER(m_mod_Arg_अंक);
        जर (doit && nodep->user2()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Identically recursive module (module instantiates "
                          "itself, without changing parameters): "
                              << Ast_गाठ::prettyNameQ(nodep->origName()));
        } अन्यथा जर (doit) {
            UINFO(4, "     Link Module: " << nodep);
            UASSERT_OBJ(!nodep->dead(), nodep, "Module in instance tree mislabeled as dead?");
            AstPackage* स्थिरांक pkgp = VN_CAST(nodep, Package);
            m_classOrPackagep = pkgp;
            जर (standalonePkg) {
                जर (pkgp->isDollarUnit()) {
                    m_curSymp = m_modSymp = m_statep->dunitEntp();
                    nodep->user1p(m_curSymp);
                } अन्यथा {
                    VSymEnt* स्थिरांक upperSymp = m_statep->dunitEntp();
                    m_scope = nodep->name();
                    m_curSymp = m_modSymp = m_statep->घाला_Block(upperSymp, nodep->name(), nodep,
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
            nodep->user2(सत्य);
            iterateChildren(nodep);
            nodep->user2(असत्य);
            nodep->user4(सत्य);
            // Interfaces need another pass when signals are resolved. When creating
            // scopes this is done from the AstScopes in LinkDotScopeVisitor instead, as
            // only those entries hold the AstVarScopes the modport items resolve against.
            जर (!m_statep->forScopeCreation()) {
                जर (AstIface* स्थिरांक ifacep = VN_CAST(nodep, Iface)) {
                    m_statep->घाला_Iface_Mod_Sym(ifacep, m_curSymp);
                }
            }
        } अन्यथा जर (isHierBlockWrapper(nodep->name())) {
            UINFO(5, "Module is hierarchical block, must not be dead: " << nodep);
            m_scope = nodep->name();
            VSymEnt* स्थिरांक upperSymp = m_curSymp ? m_curSymp : m_statep->मूळ_Entp();
            m_curSymp = m_modSymp
                = m_statep->घाला_Block(upperSymp, nodep->name() + "::", nodep, m_classOrPackagep);
            iterateChildren(nodep);
            nodep->user4(सत्य);
        } अन्यथा {  // !doit
            जर (nodep->hierParams()) {
                UINFO(1, "Found module with hier type parameters");
                m_hierParamsName = nodep->name();
                साठी (स्थिरांक Ast_गाठ* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    जर (स्थिरांक AstTypedef* स्थिरांक tdef = VN_CAST(stmtp, Typedef)) {
                        UINFO(1, "Inserting hier type parameter typedef: " << tdef);
                        VSymEnt* स्थिरांक upperSymp = m_curSymp ? m_curSymp : m_statep->मूळ_Entp();
                        m_curSymp = m_modSymp = m_statep->घाला_Block(upperSymp, nodep->name(),
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

    शून्य visit(AstClass* nodep) override {  // FindVisitor::
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
            VSymEnt* स्थिरांक upperSymp = m_curSymp;
            m_scope = m_scope + "." + nodep->name();
            m_classOrPackagep = nodep;
            m_curSymp = m_modSymp
                = m_statep->घाला_Block(upperSymp, nodep->name(), nodep, m_classOrPackagep);
            m_statep->घाला_Map(m_curSymp, m_scope);
            UINFO(9, "New module scope " << m_curSymp);
            //
            m_param_अंक = 0;
            m_mod_Block_अंक = 0;
            m_mod_With_अंक = 0;
            m_mod_Arg_अंक = 0;
            m_explicitNew = असत्य;
            // m_modSymp/m_curSymp for non-packages set by AstCell above this module
            // Iterate
            iterateChildren(nodep);
            nodep->user4(सत्य);
            // Implicit new needed?
            जर (!m_explicitNew && m_statep->forPrimary()) makeImplicitNew(nodep);
        }
    }
    शून्य visit(AstClassOrPackageRef* nodep) override {  // FindVisitor::
        जर (!nodep->classOrPackageNodep() && nodep->name() == "$unit") {
            nodep->classOrPackageNodep(v3Global.rootp()->dollarUnitPkgp());
        }
        iterateChildren(nodep);
    }
    शून्य visit(AstScope* nodep) override {  // FindVisitor::
        UASSERT_OBJ(m_statep->forScopeCreation(), nodep,
                    "Scopes should only exist right after V3Scope");
        // Detach on first encounter, will re add at the end.
        // Otherwise would be visited O(cells^2) time after V3Scope
        जर (!nodep->is_शीर्ष()) {
            ++m_statScopeVisits;
            nodep->unlinkFrBack();
            m_scopes.ढकला_back(nodep);
        }
    }
    शून्य visit(AstCell* nodep) override {  // FindVisitor::
        UINFO(5, "   CELL under " << m_scope << " is " << nodep);
        // Process XREFs/etc inside pins
        जर (nodep->recursive() && m_inRecursion) परत;
        iterateChildren(nodep);
        // Recurse in, preserving state
        VL_RESTORER_COPY(m_scope);
        VL_RESTORER(m_modSymp);
        VL_RESTORER(m_curSymp);
        VL_RESTORER(m_param_अंक);
        VL_RESTORER(m_inRecursion);
        // Where do we add it?
        VSymEnt* aboveSymp = m_curSymp;
        स्थिरांक string origname = Ast_गाठ::dedotName(nodep->name());
        string::size_type pos = origname.rfind('.');
        जर (pos != string::npos) {
            // Flattened, find what CellInline it should live under
            स्थिरांक string scope = origname.substr(0, pos);
            string baddot;
            VSymEnt* okSymp;
            aboveSymp
                = m_statep->findDotted(nodep->fileline(), aboveSymp, scope, baddot, okSymp, असत्य);
            UASSERT_OBJ(aboveSymp, nodep,
                        "Can't find instance insertion point at "
                            << Ast_गाठ::prettyNameQ(baddot) << " in: " << nodep->prettyNameQ());
        }
        {
            m_scope = m_scope + "." + nodep->name();
            m_curSymp = m_modSymp = m_statep->घाला_Cell(aboveSymp, m_modSymp, nodep, m_scope);
            m_inRecursion = nodep->recursive();
            // We don't report NotFoundModule, as may be a unused module in a generate
            जर (nodep->modp()) iterate(nodep->modp());
        }
    }
    शून्य visit(AstCellInline* nodep) override {  // FindVisitor::
        UINFO(5, "   CELLINLINE under " << m_scope << " is " << nodep);
        VSymEnt* aboveSymp = m_curSymp;
        std::string ident = nodep->name();
        // If baz__DOT__foo__DOT__bar, we need to find baz__DOT__foo and add bar to it.
        स्थिरांक std::string::size_type pos = ident.rfind("__DOT__");
        जर (pos != std::string::npos) {
            स्थिरांक std::string dotted = ident.substr(0, pos);
            ident = ident.substr(pos + std::म_माप("__DOT__"));
            std::string baddot;
            VSymEnt* okSymp;
            aboveSymp = m_statep->findDotted(nodep->fileline(), aboveSymp, dotted, baddot, okSymp,
                                             असत्य);
            UASSERT_OBJ(aboveSymp, nodep,
                        "Can't find cellinline insertion point at "
                            << Ast_गाठ::prettyNameQ(baddot) << " in: " << nodep->prettyNameQ());
        }
        m_statep->घाला_Inline(aboveSymp, m_modSymp, nodep, ident);
    }
    शून्य visit(AstDefParam* nodep) override {  // FindVisitor::
        nodep->user1p(m_curSymp);
        iterateChildren(nodep);
    }
    शून्य visit(AstRefDType* nodep) override {  // FindVisitor::
        nodep->user1p(m_curSymp);
        iterateChildren(nodep);
    }
    शून्य visit(AstGenBlock* nodep) override {  // FindVisitor::
        UINFO(5, "   " << nodep);
        जर (nodep->name() == "" && nodep->unnamed()) {
            // Unnamed blocks are only important when they contain var
            // decls, so search for them. (Otherwise adding all the
            // unnamed#'s would just confuse tracing variables in
            // places such as tasks, where "task ...; begin ... end"
            // are common.
            साठी (Ast_गाठ* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
                जर (VN_IS(itemp, Var) || VN_IS(itemp, Foreach)) {
                    std::string name;
                    स्थिरांक std::string stepStr = m_statep->forPrimary()
                                                    ? ""
                                                    : std::to_string(m_statep->step_संख्या()) + "_";
                    करा {
                        ++m_mod_Block_अंक;
                        name = "unnamedblk" + stepStr + cvtToStr(m_mod_Block_अंक);
                        // Increment again if earlier pass of V3LinkDot claimed this name
                    } पर्यंत (m_curSymp->findIdFlat(name));
                    nodep->name(name);
                    थांबा;
                }
            }
        }
        जर (nodep->name() == "") {
            iterateChildren(nodep);
        } अन्यथा {
            VL_RESTORER(m_curSymp);
            {
                m_curSymp
                    = m_statep->घाला_Block(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
                m_curSymp->fallbackp(VL_RESTORER_PREV(m_curSymp));
                // Iterate
                iterateChildren(nodep);
            }
        }
    }
    शून्य visit(Ast_गाठ_Block* nodep) override {  // FindVisitor::
        UINFO(5, "   " << nodep);
        जर (nodep->name() == "" && nodep->unnamed()) {
            // Unnamed blocks are only important when they contain var
            // decls, so search for them. (Otherwise adding all the
            // unnamed#'s would just confuse tracing variables in
            // places such as tasks, where "task ...; begin ... end"
            // are common.
            स्थिरांक स्वतः containsDecl = [](स्थिरांक Ast_गाठ* nodesp) -> तार्किक {
                साठी (स्थिरांक Ast_गाठ* np = nodesp; np; np = np->nextp()) {
                    जर (VN_IS(np, Var) || VN_IS(np, Foreach)) परत सत्य;
                }
                परत असत्य;
            };
            तार्किक needName = nodep->declsp() || containsDecl(nodep->stmtsp());
            जर (स्थिरांक AstFork* स्थिरांक forkp = VN_CAST(nodep, Fork)) {
                needName = needName || containsDecl(forkp->forksp());
            }
            जर (needName) {
                स्थिरांक std::string stepStr
                    = m_statep->forPrimary() ? "" : std::to_string(m_statep->step_संख्या()) + "_";
                std::string name;
                करा {
                    ++m_mod_Block_अंक;
                    name = "unnamedblk" + stepStr + cvtToStr(m_mod_Block_अंक);
                    // Increment again if earlier pass of V3LinkDot claimed this name
                } पर्यंत (m_curSymp->findIdFlat(name));
                nodep->name(name);
            }
        }
        जर (nodep->name() == "") {
            iterateChildren(nodep);
        } अन्यथा {
            VL_RESTORER(m_curSymp);
            {
                m_curSymp
                    = m_statep->घाला_Block(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
                m_curSymp->fallbackp(VL_RESTORER_PREV(m_curSymp));
                // Iterate
                iterateChildren(nodep);
            }
        }
    }
    // Handle out-of-block interface method definition (IEEE 1800-2023 25.7)
    // Move task body from module into the interface's prototype task,
    // rewriting port-prefixed references in the body.
    शून्य moveIfaceExportBody(Ast_गाठ_FTask* nodep) {
        स्थिरांक string& portName = nodep->ifacePortName();
        UINFO(5, "   moveIfaceExportBody: port=" << portName << " task=" << nodep->name());
        // Look up the interface port variable in the module's symbol table
        VSymEnt* स्थिरांक portSymp = m_modSymp->findIdFallback(portName);
        जर (!portSymp) {
            nodep->v3error("Interface port not found for out-of-block definition: '"
                           << portName << "' (IEEE 1800-2023 25.7)");
            nodep->unlinkFrBack();
            VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
            परत;
        }
        AstVar* स्थिरांक portp = VN_CAST(portSymp->nodep(), Var);
        जर (!portp) {
            nodep->v3fatalSrc("Out-of-block definition port is not a variable: " << portName);
            परत;
        }
        // Get the interface name from the port's type
        // At this stage dtypep() may not be set yet; check childDTypep() too
        AstIfaceRefDType* ifaceRefDtp = nullptr;
        जर (portp->dtypep()) { ifaceRefDtp = VN_CAST(portp->dtypep(), IfaceRefDType); }
        जर (!ifaceRefDtp && portp->childDTypep()) {
            ifaceRefDtp = VN_CAST(portp->childDTypep(), IfaceRefDType);
        }
        जर (!ifaceRefDtp) {
            nodep->v3error("Out-of-block definition port is not an interface port: '"
                           << portName << "' (IEEE 1800-2023 25.7)");
            nodep->unlinkFrBack();
            VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
            परत;
        }
        स्थिरांक string ifaceName = ifaceRefDtp->ifaceName();
        UINFO(5, "   moveIfaceExportBody: iface=" << ifaceName);
        // Find the interface module by name
        Ast_गाठ_Module* स्थिरांक ifaceModp = m_statep->findModuleSym(ifaceName);
        जर (!ifaceModp) {
            nodep->v3fatalSrc("Interface not found for out-of-block definition: " << ifaceName);
            परत;
        }
        // Find the prototype task in the interface
        Ast_गाठ_FTask* protoTaskp = nullptr;
        साठी (Ast_गाठ* itemp = ifaceModp->stmtsp(); itemp; itemp = itemp->nextp()) {
            जर (Ast_गाठ_FTask* स्थिरांक ftaskp = VN_CAST(itemp, गाठ_FTask)) {
                जर (ftaskp->name() == nodep->name() && ftaskp->prototype()) {
                    protoTaskp = ftaskp;
                    थांबा;
                }
            }
        }
        जर (!protoTaskp) {
            nodep->v3error("No matching export prototype found in interface "
                           << ifaceName << " for task " << nodep->prettyNameQ()
                           << " (IEEE 1800-2023 25.7)");
            nodep->unlinkFrBack();
            VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
            परत;
        }
        // Move body statements (non-port declarations) from the out-of-block task
        // to the prototype task. Rewrite port.X references to just X in the body.
        साठी (Ast_गाठ* stmtp = nodep->stmtsp(); stmtp;) {
            Ast_गाठ* स्थिरांक nextp = stmtp->nextp();
            // Skip port declarations -- they already exist in the prototype
            जर (स्थिरांक AstVar* स्थिरांक varp = VN_CAST(stmtp, Var)) {
                जर (varp->isIO()) {
                    stmtp = nextp;
                    सुरूठेवा;
                }
            }
            // Clone this statement and rewrite port references
            Ast_गाठ* स्थिरांक newStmtp = stmtp->clone_वृक्ष(असत्य);
            rewriteIfacePortRefsSingle(newStmtp, portName);
            protoTaskp->addStmtsp(newStmtp);
            stmtp = nextp;
        }
        // Mark prototype as no longer prototype (it has a body now)
        protoTaskp->prototype(असत्य);
        // Delete the out-of-block task from the module
        nodep->unlinkFrBack();
        VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
    }

    // Rewrite AstDot nodes where lhsp is AstParseRef with the port name
    // to just the rhsp (stripping the port prefix).
    स्थिर शून्य rewriteIfacePortRefsSingle(Ast_गाठ* nodep, स्थिरांक string& portName) {
        जर (!nodep) परत;
        // Check if this is a non-colon Dot with lhs matching portName
        जर (AstDot* स्थिरांक dotp = VN_CAST(nodep, Dot)) {
            जर (!dotp->colon()) {
                स्थिरांक AstParseRef* स्थिरांक lhsRefp = VN_CAST(dotp->lhsp(), ParseRef);
                जर (lhsRefp && lhsRefp->name() == portName) {
                    UINFO(5, "   rewriteIfacePortRefs: stripping port prefix from " << dotp);
                    Ast_गाठ* स्थिरांक rhsp = dotp->rhsp()->unlinkFrBack();
                    dotp->replaceWith(rhsp);
                    VL_DO_DANGLING(dotp->delete_वृक्ष(), dotp);
                    rewriteIfacePortRefsSingle(rhsp, portName);
                    परत;
                }
            }
        }
        // Recurse into all children
        साठी (Ast_गाठ* childp : {nodep->op1p(), nodep->op2p(), nodep->op3p(), nodep->op4p()}) {
            पर्यंत (childp) {
                Ast_गाठ* स्थिरांक nextp = childp->nextp();
                rewriteIfacePortRefsSingle(childp, portName);
                childp = nextp;
            }
        }
    }

    शून्य visit(Ast_गाठ_FTask* nodep) override {  // FindVisitor::
        // NodeTask: Remember its name for later resolution
        UINFO(5, "   " << nodep);
        UASSERT_OBJ(m_curSymp && m_modSymp, nodep, "Function/Task not under module?");
        जर (nodep->name() == "new") m_explicitNew = सत्य;
        // Handle out-of-block interface method definition (IEEE 25.8)
        जर (!nodep->ifacePortName().empty() && m_statep->forPrimary()) {
            moveIfaceExportBody(nodep);
            परत;  // Task has been deleted
        }
        // Remember the existing symbol table scope
        VL_RESTORER(m_classOrPackagep);
        VL_RESTORER(m_curSymp);
        VSymEnt* upSymp = m_curSymp;

        जर (VN_IS(m_curSymp->nodep(), Class)) {
            जर (VN_AS(m_curSymp->nodep(), Class)->isInterfaceClass() && !nodep->pureVirtual()
                && !nodep->isConstructor()) {
                nodep->v3error("Interface class functions must be pure virtual"
                               << " (IEEE 1800-2023 8.26): " << nodep->prettyNameQ());
            }
            जर (m_statep->forPrimary()
                && (nodep->name() == "randomize" || nodep->name() == "srandom")) {
                nodep->v3error(nodep->prettyNameQ()
                               << " is a predefined class method; redefinition not allowed"
                                  " (IEEE 1800-2023 18.6.3)");
            }
        }
        // Change to appropriate package if extern declaration (vs definition)
        जर (nodep->classOrPackagep()) {
            // class-in-class
            AstDot* स्थिरांक dotp = VN_CAST(nodep->classOrPackagep(), Dot);
            AstClassOrPackageRef* स्थिरांक cpackagerefp
                = VN_CAST(nodep->classOrPackagep(), ClassOrPackageRef);
            जर (dotp) {
                AstClassOrPackageRef* स्थिरांक lhsp = VN_AS(dotp->lhsp(), ClassOrPackageRef);
                m_statep->resolveClassOrPackage(m_curSymp, lhsp, सत्य, असत्य,
                                                "External definition :: reference");
                AstClass* स्थिरांक lhsclassp = VN_CAST(lhsp->classOrPackageSkipp(), Class);
                जर (!lhsclassp) {
                    nodep->v3error("Extern declaration's scope is not a defined class");
                } अन्यथा {
                    m_curSymp = m_statep->get_गाठ_Sym(lhsclassp);
                    upSymp = m_curSymp;
                    AstClassOrPackageRef* स्थिरांक rhsp = VN_AS(dotp->rhsp(), ClassOrPackageRef);
                    m_statep->resolveClassOrPackage(m_curSymp, rhsp, सत्य, असत्य,
                                                    "External definition :: reference");
                    AstClass* स्थिरांक rhsclassp = VN_CAST(rhsp->classOrPackageSkipp(), Class);
                    जर (!rhsclassp) {
                        nodep->v3error("Extern declaration's scope is not a defined class");
                    } अन्यथा {
                        m_curSymp = m_statep->get_गाठ_Sym(rhsclassp);
                        upSymp = m_curSymp;
                        moveExternFuncDecl(nodep, rhsclassp);
                    }
                }
            } अन्यथा जर (cpackagerefp) {
                जर (!cpackagerefp->classOrPackageSkipp()) {
                    m_statep->resolveClassOrPackage(m_curSymp, cpackagerefp, सत्य, असत्य,
                                                    "External definition :: reference");
                }
                AstClass* स्थिरांक classp = VN_CAST(cpackagerefp->classOrPackageSkipp(), Class);
                जर (!classp) {
                    nodep->v3error("Extern declaration's scope is not a defined class");
                } अन्यथा {
                    m_curSymp = m_statep->get_गाठ_Sym(classp);
                    upSymp = m_curSymp;
                    moveExternFuncDecl(nodep, classp);
                }
            } अन्यथा {
                v3fatalSrc("Unhandled extern function definition package");
            }
        }
        // Set the class as package for iteration
        जर (VN_IS(m_curSymp->nodep(), Class)) {
            m_classOrPackagep = VN_AS(m_curSymp->nodep(), Class);
        }
        // Create symbol table for the task's vars
        स्थिरांक string name = (nodep->isExternProto() ? "extern "s : ""s) + nodep->name();
        m_curSymp = m_statep->घाला_Block(m_curSymp, name, nodep, m_classOrPackagep);
        m_curSymp->fallbackp(upSymp);
        // Convert the func's range to the output variable
        // This should probably be done in the Parser instead, as then we could
        // just attach normal signal attributes to it.
        जर (!nodep->isExternProto() && nodep->fvarp() && !VN_IS(nodep->fvarp(), Var)) {
            Ast_गाठ_DType* dtypep = VN_CAST(nodep->fvarp(), गाठ_DType);
            // If unspecified, function returns one bit; however when we
            // support NEW() it could also return the class reference.
            जर (dtypep) {
                dtypep->unlinkFrBack();
            } अन्यथा {
                dtypep = नवीन AstBasicDType{nodep->fileline(), VBasicDTypeKwd::LOGIC};
            }
            AstVar* स्थिरांक newvarp
                = नवीन AstVar{nodep->fileline(), VVarType::VAR, nodep->name(), VFlagChildDType{},
                             dtypep};  // Not dtype resolved yet
            newvarp->direction(VDirection::OUTPUT);
            newvarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            newvarp->funcReturn(सत्य);
            newvarp->trace(असत्य);  // Not user visible
            nodep->fvarp(newvarp);
            // Explicit insert required, as the var name shadows the upper level's task name
            m_statep->घाला_Sym(m_curSymp, newvarp->name(), newvarp, nullptr /*classOrPackagep*/);
        }
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    शून्य visit(AstClocking* nodep) override {  // FindVisitor::
        VL_RESTORER(m_clockingp);
        m_clockingp = nodep;
        iterate(nodep->sentreep());
        iterateAndNextNull(nodep->itemsp());
        // If the block has no name, one cannot reference the clockvars
        VSymEnt* itSymp = nullptr;
        जर (nodep->isGlobal()  //
            && m_statep->forPrimary()) {  // else flattening may see two globals
            m_statep->checkDuplicate(m_curSymp, nodep, "__024global_clock");
            itSymp
                = m_statep->घाला_Block(m_curSymp, "__024global_clock", nodep, m_classOrPackagep);
            itSymp->fallbackp(nullptr);
        }
        जर (!nodep->name().empty()) {
            itSymp = m_statep->घाला_Block(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
            itSymp->fallbackp(nullptr);
        }
        जर (itSymp) {
            VL_RESTORER(m_curSymp);
            m_curSymp = itSymp;
            iterateAndNextNull(nodep->itemsp());
        }
    }
    शून्य visit(AstClockingItem* nodep) override {  // FindVisitor::
        जर (nodep->varp()) {
            जर (m_curSymp->nodep() == m_clockingp) iterate(nodep->varp());
            परत;
        }
        std::string varname;
        Ast_गाठ_DType* dtypep;
        जर (AstAssign* स्थिरांक assignp = nodep->assignp()) {
            Ast_गाठ_अभिव्यक्ती* स्थिरांक rhsp = assignp->rhsp()->unlinkFrBack();
            dtypep = नवीन AstRefDType{nodep->fileline(), AstRefDType::Flag_Type_Of_अभिव्यक्ती{},
                                     rhsp->clone_वृक्ष(असत्य)};
            nodep->exprp(rhsp);
            varname = assignp->lhsp()->name();
            VL_DO_DANGLING(assignp->unlinkFrBack()->delete_वृक्ष(), assignp);
        } अन्यथा {
            Ast_गाठ_अभिव्यक्ती* स्थिरांक refp = nodep->exprp();
            स्थिरांक VSymEnt* foundp = m_curSymp->findIdFallback(refp->name());
            जर (!foundp || !foundp->nodep()) {
                refp->v3error("Corresponding variable " << refp->prettyNameQ()
                                                        << " does not exist");
                VL_DO_DANGLING(nodep->unlinkFrBack()->delete_वृक्ष(), nodep);
                परत;
            }
            varname = refp->name();
            dtypep = VN_AS(foundp->nodep(), Var)->childDTypep()->clone_वृक्ष(असत्य);
        }
        AstVar* स्थिरांक newvarp = नवीन AstVar{nodep->fileline(), VVarType::MODULETEMP, varname,
                                           VFlagChildDType{}, dtypep};
        newvarp->lifetime(VLifetime::STATIC_EXPLICIT);
        nodep->varp(newvarp);
        iterate(nodep->exprp());
    }
    शून्य visit(AstConstraint* nodep) override {  // FindVisitor::
        VL_RESTORER(m_curSymp);
        // Change to appropriate package if extern declaration (vs definition)
        VSymEnt* upSymp = m_curSymp;
        जर (nodep->classOrPackagep()) {
            AstClassOrPackageRef* स्थिरांक cpackagerefp
                = VN_CAST(nodep->classOrPackagep(), ClassOrPackageRef);
            जर (!cpackagerefp) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: extern constraint definition with class-in-class");
            } अन्यथा {
                जर (!cpackagerefp->classOrPackageSkipp()) {
                    m_statep->resolveClassOrPackage(m_curSymp, cpackagerefp, सत्य, असत्य,
                                                    "External definition :: reference");
                }
                AstClass* स्थिरांक classp = VN_CAST(cpackagerefp->classOrPackageSkipp(), Class);
                जर (!classp) {
                    nodep->v3error("Extern declaration's scope is not a defined class");
                } अन्यथा {
                    m_curSymp = m_statep->get_गाठ_Sym(classp);
                    upSymp = m_curSymp;
                    // Move it to proper spot under the target class
                    nodep->unlinkFrBack();
                    classp->addStmtsp(nodep);
                    nodep->classOrPackagep()->unlinkFrBack()->delete_वृक्ष();
                }
            }
        }
        // Set the class as package for iteration
        स्थिरांक string name = (nodep->isExternProto() ? "extern "s : ""s) + nodep->name();
        m_curSymp = m_statep->घाला_Block(upSymp, name, nodep, m_classOrPackagep);
        iterateChildren(nodep);
    }
    शून्य visit(AstVar* nodep) override {  // FindVisitor::
        // Var: Remember its name for later resolution
        UASSERT_OBJ(m_curSymp && m_modSymp, nodep, "Var not under module?");
        iterateChildren(nodep);
        जर (VN_IS(m_curSymp->nodep(), Class)
            && VN_AS(m_curSymp->nodep(), Class)->isInterfaceClass() && !nodep->isParam()) {
            nodep->v3error("Interface class cannot contain non-parameter members"
                           << " (IEEE 1800-2023 8.26): " << nodep->prettyNameQ());
        }
        जर (!m_statep->forScopeCreation()) {
            // Find under either a task or the module's vars
            स्थिरांक VSymEnt* foundp = m_curSymp->findIdFallback(nodep->name());
            जर (!foundp && m_modSymp && nodep->name() == m_modSymp->nodep()->name()) {
                foundp = m_modSymp;  // Conflicts with modname?
            }
            AstVar* स्थिरांक findvarp = foundp ? VN_CAST(foundp->nodep(), Var) : nullptr;
            // clocking items can have duplicate names (inout)
            जर (findvarp && VN_IS(findvarp->backp(), ClockingItem)
                && VN_IS(nodep->backp(), ClockingItem)) {
                AstClockingItem* स्थिरांक itemp = VN_AS(nodep->backp(), ClockingItem);
                AstClockingItem* स्थिरांक finditemp = VN_AS(findvarp->backp(), ClockingItem);
                UINFO(4, "ClockCompl: " << itemp << " ;; " << finditemp);
                UINFO(4, "ClockCompV: " << nodep << " ;; " << findvarp);
                जर (*itemp->exprp()->fileline() == *finditemp->exprp()->fileline()) {
                    UASSERT_OBJ(finditemp->direction() == VDirection::आदान
                                    && itemp->direction() == VDirection::OUTPUT,
                                itemp, "Input after output?");
                    // pretend nothing found and rename
                    foundp = nullptr;
                    nodep->name("__Voutput_" + nodep->name());
                    finditemp->outputp(itemp);
                }
            }
            तार्किक ins = असत्य;
            जर (!foundp) {
                ins = सत्य;
            } अन्यथा जर (!findvarp && m_curSymp->findIdFlat(nodep->name()) && !foundp->imported()) {
                nodep->v3error("Unsupported in C: Variable has same name as "
                               << LinkDotState::गाठ_Text_Type(foundp->nodep()) << ": "
                               << nodep->prettyNameQ());
            } अन्यथा जर (findvarp != nodep) {
                UINFO(4, "DupVar: " << nodep << " ;; " << foundp->nodep());
                UINFO(4, "    found  cur=se" << cvtToHex(m_curSymp) << " ;; parent=se"
                                             << cvtToHex(foundp->parentp()));
                जर (foundp->parentp() == m_curSymp  // Only when on same level
                    && !foundp->imported()) {  // and not from package
                    जर (VN_IS(m_curSymp->nodep(), Clocking)) {
                        nodep->v3error("Multiple clockvars with the same name not allowed");
                        परत;
                    }
                    स्थिरांक तार्किक nansiBad
                        = (((findvarp->isDeclTyped() || findvarp->isNet())
                            && (nodep->isDeclTyped() || nodep->isNet()))
                           || (findvarp->isIO() && nodep->isIO())  // e.g. !(output && output)
                           || findvarp->gotNansiType());  // e.g. int x && int x
                    स्थिरांक तार्किक ansiBad
                        = findvarp->isAnsi() || nodep->isAnsi();  // dup illegal with ANSI
                    जर (ansiBad || nansiBad) {
                        तार्किक ansiWarn = ansiBad && !nansiBad;
                        जर (ansiWarn) {
                            स्थिर पूर्णांक s_didAnsiWarn = असत्य;
                            जर (s_didAnsiWarn++) ansiWarn = असत्य;
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
                        findvarp->gotNansiType(सत्य);
                        UASSERT_OBJ(nodep->subDTypep(), nodep, "Var has no type");
                        जर (nodep->subDTypep()->numeric().isSigned()
                            && !findvarp->subDTypep()->numeric().isSigned()) {
                            findvarp->subDTypep()->numeric(VSigning{सत्य});
                        }
                        Ast_गाठ_DType* स्थिरांक varDtp = findvarp->subDTypep();
                        Ast_गाठ_DType* स्थिरांक otherDtp = nodep->subDTypep();
                        जर (varDtp
                            && (VN_IS(varDtp, BasicDType)
                                && VN_AS(varDtp, BasicDType)->implicit())) {
                            // Then have "input foo" and "real foo" so the
                            // dtype comes from the other side.
                            VL_DO_DANGLING(varDtp->unlinkFrBack()->delete_वृक्ष(), varDtp);
                            findvarp->childDTypep(otherDtp->unlinkFrBack());
                        } अन्यथा जर (!m_statep->forScopeCreation() && otherDtp && varDtp
                                   && !(VN_IS(otherDtp, BasicDType)
                                        && VN_AS(otherDtp, BasicDType)->implicit())) {
                            // otherDtp and varDtp both non-nullptr and neither are implicit
                            // Can't compare dtypes now as might contain parameters,
                            // defer to V3Width
                            AstAttrOf* स्थिरांक newp
                                = नवीन AstAttrOf{otherDtp->fileline(), VAttrType::VAR_PORT_DTYPE,
                                                otherDtp->unlinkFrBack()};
                            findvarp->addAttrsp(newp);
                        }
                    }
                    VL_DO_DANGLING(nodep->unlinkFrBack()->delete_वृक्ष(), nodep);
                } अन्यथा {
                    // User can disable the message at either point
                    जर (!(m_ftaskp && m_ftaskp->dpiImport())
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
                    ins = सत्य;
                }
            }
            जर (ins) {
                जर (m_statep->forPrimary() && nodep->isGParam()
                    && VN_IS(m_modSymp->nodep(), Module)
                    && (m_statep->मूळ_Entp()->nodep() == m_modSymp->parentp()->nodep())) {
                    // This is the toplevel module. Check for command line overwrites of parameters
                    // We first search if the parameter is overwritten and then replace it with a
                    // new value.
                    जर (v3Global.opt.hasParameter(nodep->name())) {
                        स्थिरांक string svalue = v3Global.opt.parameter(nodep->name());
                        जर (AstConst* स्थिरांक valuep
                            = AstConst::parseParamLiteral(nodep->fileline(), svalue)) {
                            UINFO(9, "       replace parameter " << nodep);
                            UINFO(9, "       with " << valuep);
                            जर (nodep->valuep()) ढकला_Deletep(nodep->valuep()->unlinkFrBack());
                            nodep->valuep(valuep);
                            // Do not warn ENUMVALUE when overriding an enum param from the cli.
                            nodep->fileline()->warnOff(V3ErrorCode::ENUMVALUE, सत्य);
                        }
                    }
                }
                m_statep->घाला_Sym(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
                जर (m_statep->forPrimary() && nodep->isGParam()) {
                    ++m_param_अंक;
                    VSymEnt* स्थिरांक symp
                        = m_statep->घाला_Sym(m_curSymp, "__paramNumber" + cvtToStr(m_param_अंक),
                                              nodep, m_classOrPackagep);
                    symp->exported(असत्य);
                }
            }
        }
    }
    शून्य visit(AstTypedef* nodep) override {  // FindVisitor::
        UASSERT_OBJ(m_curSymp, nodep, "Typedef not under module/package/$unit");
        जर (VN_IS(m_classOrPackagep, Class)) nodep->isUnderClass(सत्य);
        iterateChildren(nodep);
        m_statep->घाला_Sym(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
    }
    शून्य visit(AstTypedefFwd* nodep) override {  // FindVisitor::
        UASSERT_OBJ(m_curSymp, nodep, "Typedef not under module/package/$unit");
        iterateChildren(nodep);
        // No need to insert, only the real typedef matters, but need to track for errors
        nodep->user1p(m_curSymp);
    }
    शून्य visit(Ast_गाठ_UOr_Struct_DType* nodep) override {  // FindVisitor::
        UASSERT_OBJ(m_curSymp, nodep, "Struct/union dtype not under module/package/$unit");
        VL_RESTORER(m_curSymp);
        m_curSymp = m_statep->घाला_Block(m_curSymp, "__Vdtype" + cvtToStr(nodep->unique_अंक()),
                                          nodep, m_classOrPackagep);
        iterateChildren(nodep);
    }
    शून्य visit(AstMemberDType* nodep) override {  // FindVisitor::
        iterateChildren(nodep);
        m_statep->घाला_Sym(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
    }
    शून्य visit(AstParamTypeDType* nodep) override {  // FindVisitor::
        UASSERT_OBJ(m_curSymp, nodep, "Parameter type not under module/package/$unit");

        // Replace missing param types with provided hierarchical type params.
        जर (!m_hierParamsName.empty()) {
            जर (स्थिरांक VSymEnt* स्थिरांक typedefEntp = m_curSymp->findIdFallback(m_hierParamsName)) {
                स्थिरांक AstModule* modp = VN_CAST(typedefEntp->nodep(), Module);

                साठी (स्थिरांक Ast_गाठ* stmtp = modp ? modp->stmtsp() : nullptr; stmtp;
                     stmtp = stmtp->nextp()) {
                    स्थिरांक AstTypedef* tdefp = VN_CAST(stmtp, Typedef);

                    जर (tdefp && tdefp->name() == nodep->name() && m_statep->forPrimary()) {
                        UINFO(8, "Replacing type of" << nodep << "  -->with " << tdefp);
                        Ast_गाठ_DType* स्थिरांक newDtp = tdefp->childDTypep();
                        Ast_गाठ_DType* स्थिरांक oldDtp = nodep->childDTypep();

                        oldDtp->replaceWith(newDtp->clone_वृक्ष(असत्य));
                        VL_DO_DANGLING(oldDtp->delete_वृक्ष(), oldDtp);
                    }
                }
            }
        }

        iterateChildren(nodep);
        m_statep->घाला_Sym(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
        जर (m_statep->forPrimary() && nodep->isGParam()) {
            ++m_param_अंक;
            VSymEnt* स्थिरांक symp = m_statep->घाला_Sym(
                m_curSymp, "__paramNumber" + cvtToStr(m_param_अंक), nodep, m_classOrPackagep);
            symp->exported(असत्य);
        }
    }
    शून्य visit(AstCFunc* nodep) override {  // FindVisitor::
        // For dotted resolution, ignore all AstVars under functions, otherwise shouldn't exist
        UASSERT_OBJ(!m_statep->forScopeCreation(), nodep, "No CFuncs expected in tree yet");
    }
    शून्य visit(AstEnumItem* nodep) override {  // FindVisitor::
        // EnumItem: Remember its name for later resolution
        iterateChildren(nodep);
        // Find under either a task or the module's vars
        स्थिरांक VSymEnt* foundp = m_curSymp->findIdFallback(nodep->name());
        जर (!foundp && m_modSymp && nodep->name() == m_modSymp->nodep()->name()) {
            foundp = m_modSymp;  // Conflicts with modname?
        }
        AstEnumItem* स्थिरांक findvarp = foundp ? VN_CAST(foundp->nodep(), EnumItem) : nullptr;
        तार्किक ins = असत्य;
        जर (!foundp) {
            ins = सत्य;
        } अन्यथा जर (findvarp != nodep) {
            UINFO(4, "DupVar: " << nodep << " ;; " << foundp);
            जर (foundp->parentp() == m_curSymp  // Only when on same level
                && !foundp->imported()) {  // and not from package
                nodep->v3error("Duplicate declaration of enum value: "
                               << nodep->prettyName() << '\n'
                               << nodep->warnContextPrimary() << '\n'
                               << foundp->nodep()->warnOther()
                               << "... Location of original declaration\n"
                               << foundp->nodep()->warnContextSecondary());
            } अन्यथा {
                // User can disable the message at either point
                जर (!nodep->fileline()->warnIsOff(V3ErrorCode::VARHIDDEN)
                    && !foundp->nodep()->fileline()->warnIsOff(V3ErrorCode::VARHIDDEN)) {
                    nodep->v3warn(VARHIDDEN,
                                  "Declaration of enum value hides declaration in upper scope: "
                                      << nodep->prettyName() << '\n'
                                      << nodep->warnContextPrimary() << '\n'
                                      << foundp->nodep()->warnOther()
                                      << "... Location of original declaration\n"
                                      << foundp->nodep()->warnContextSecondary());
                }
                ins = सत्य;
            }
        }
        जर (ins) m_statep->घाला_Sym(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
    }
    शून्य visit(AstPackageImport* nodep) override {  // FindVisitor::
        UINFO(4, "  Link: " << nodep);
        जर (!nodep->packagep()) परत;  // Errored in V3LinkCells
        VSymEnt* स्थिरांक srcp = m_statep->get_गाठ_Sym(nodep->packagep());
        जर (nodep->name() == "*") {
            जर (nodep->packagep() != v3Global.rootp()->stdPackagep()) {
                जर (m_curSymp == m_statep->dunitEntp()) {
                    nodep->v3warn(IMPORTSTAR,
                                  "'import::*' in $unit scope may pollute global namespace");
                }
            }
        } अन्यथा जर (!nodep->resolvedClassp()) {
            VSymEnt* स्थिरांक impp = srcp->findIdFlat(nodep->name());
            जर (!impp) {
                nodep->v3error("Import object not found: " << nodep->prettyPkgNameQ());
            } अन्यथा जर (nodep->resolvedClassp()) {
                UASSERT_OBJ(nodep->resolvedClassp()->classOrPackageSkipp() == impp->nodep(), nodep,
                            "Import should resolve to the same node");
            } अन्यथा जर (AstClass* स्थिरांक classp = VN_CAST(impp->nodep(), Class)) {
                nodep->resolvedClassp(
                    नवीन AstClassOrPackageRef{nodep->fileline(), classp->name(), classp, nullptr});
            }
        }
        m_curSymp->importFromPackage(m_statep->symsp(), srcp, nodep->name());
        UINFO(9, "    Link Done: " << nodep);
        // No longer needed, but can't delete until any multi-instantiated modules are expanded
    }
    शून्य visit(AstPackageExport* nodep) override {  // FindVisitor::
        UINFO(9, "  Link: " << nodep);
        जर (!nodep->packagep()) परत;  // Errored in V3LinkCells
        VSymEnt* स्थिरांक srcp = m_statep->get_गाठ_Sym(nodep->packagep());
        जर (nodep->name() != "*") {
            VSymEnt* स्थिरांक impp = srcp->findIdFlat(nodep->name());
            जर (!impp) {
                nodep->v3error("Export object not found: '" << nodep->packagep()->prettyName()
                                                            << "::" << nodep->prettyName() << "'");
            }
        }
        m_curSymp->exportFromPackage(m_statep->symsp(), srcp, nodep->name());
        UINFO(9, "    Link Done: " << nodep);
        // No longer needed, but can't delete until any multi-instantiated modules are expanded
    }
    शून्य visit(AstPackageExportStarStar* nodep) override {  // FindVisitor::
        UINFO(4, "  Link: " << nodep);
        m_curSymp->exportStarStar(m_statep->symsp());
        // No longer needed, but can't delete until any multi-instantiated modules are expanded
    }

    शून्य visit(Ast_गाठ_Foreach* nodep) override {  // FindVisitor::
        // Symbol table needs nodep->name() as the index variable's name
        VL_RESTORER(m_curSymp);
        {
            ++m_mod_With_अंक;
            m_curSymp = m_statep->घाला_Block(m_curSymp, "__Vforeach" + cvtToStr(m_mod_With_अंक),
                                              nodep, m_classOrPackagep);
            m_curSymp->fallbackp(VL_RESTORER_PREV(m_curSymp));
            साठी (Ast_गाठ *nextp, *argp = nodep->headerp()->elementsp(); argp; argp = nextp) {
                nextp = argp->nextp();
                AstVar* argrefp = nullptr;
                जर (AstParseRef* स्थिरांक parserefp = VN_CAST(argp, ParseRef)) {
                    // IEEE 1800-2023 12.7.3: foreach loop variable type shall be int (2-state)
                    // This might get changed in V3Width when types resolve (e.g., for assoc
                    // arrays)
                    argrefp = नवीन AstVar{parserefp->fileline(), VVarType::BLOCKTEMP,
                                         parserefp->name(), argp->findIntDType()};
                    argrefp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                    parserefp->replaceWith(argrefp);
                    VL_DO_DANGLING2(parserefp->delete_वृक्ष(), parserefp, argp);
                    // Insert argref's name into symbol table
                    m_statep->घाला_Sym(m_curSymp, argrefp->name(), argrefp, nullptr);
                } अन्यथा जर (स्थिरांक स्वतः largrefp = VN_CAST(argp, Var)) {
                    argrefp = largrefp;
                    // Insert argref's name into symbol table
                    m_statep->घाला_Sym(m_curSymp, argrefp->name(), argrefp, nullptr);
                } अन्यथा जर (VN_IS(argp, Empty)) {
                } अन्यथा {
                    argp->v3error("'foreach' loop variable expects simple variable name");
                }
            }
            iterateChildren(nodep);
        }
    }

    शून्य visit(AstRandSequence* nodep) override {
        VL_RESTORER(m_curSymp);
        m_curSymp = m_statep->घाला_Block(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
        iterateChildren(nodep);
    }
    शून्य visit(AstRSProd* nodep) override {
        VL_RESTORER(m_curSymp);
        m_curSymp = m_statep->घाला_Block(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
        जर (nodep->fvarp())
            nodep->fvarp()->v3warn(E_UNSUPPORTED,
                                   "Unsupported: randsequence production function variable");
        // Mark formal ports as port-checked so the primary resolve pass does not
        // flag them with "does not appear in port list" -- V3RandSequence will
        // later move them onto the generated task as real input ports.
        साठी (Ast_गाठ* itp = nodep->portsp(); itp; itp = itp->nextp()) {
            VN_AS(itp, Var)->user4(सत्य);
        }
        iterateChildren(nodep);
    }

    शून्य visit(AstWithParse* nodep) override {  // FindVisitor::
        // Change WITHPARSE(FUNCREF, equation) to FUNCREF(WITH(equation))
        Ast_गाठ_FTask_Ref* funcrefp = VN_CAST(nodep->funcrefp(), गाठ_FTask_Ref);
        जर (स्थिरांक AstDot* स्थिरांक dotp = VN_CAST(nodep->funcrefp(), Dot))
            funcrefp = VN_CAST(dotp->rhsp(), गाठ_FTask_Ref);
        UASSERT_OBJ(funcrefp, nodep, "'with' only can operate on a function/task");
        string name = funcrefp->name() == "randomize" ? "__Vrandwith_obj" : "item";
        File_ओळ* argFl = nodep->fileline();
        AstArg* स्थिरांक argsp = funcrefp->argsp();
        जर (argsp) {
            argsp->unlinkFrBackWithNext();
            जर (funcrefp->name() != "randomize") {
                जर (स्थिरांक स्वतः parserefp = VN_CAST(argsp->exprp(), ParseRef)) {
                    name = parserefp->name();
                    argFl = parserefp->fileline();
                } अन्यथा {
                    argsp->v3error("'with' function expects simple variable name");
                }
                जर (argsp->nextp()) {
                    argsp->nextp()->v3error("'with' function expects only up to one argument");
                }
                VL_DO_DANGLING(argsp->delete_वृक्ष(), argsp);
            }
        }
        // Type depends on the method used, let V3Width figure it out later
        // 'with (...) { ... }' constraint_block form is randomize-only
        // (IEEE 1800-2023 7.12 vs. 18.7); array methods take 'with (expr)'.
        जर (nodep->restricted() && funcrefp->name() != "randomize") {
            nodep->v3error("'with (...) { ... }' constraint block is only valid for"
                           " randomize() (IEEE 1800-2023 18.7)");
            funcrefp->addArgsp(argsp);
            nodep->replaceWith(nodep->funcrefp()->unlinkFrBack());
            VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
            परत;
        }
        स्थिरांक तार्किक restrictedRandomize = nodep->restricted();
        जर (nodep->exprsp() || nodep->constraintsp()
            || restrictedRandomize) {  // Else empty expression and pretend no "with"
            AstLambdaArgRef* स्थिरांक indexArgRefp
                = नवीन AstLambdaArgRef{argFl, name + "__DOT__index", सत्य};
            AstLambdaArgRef* स्थिरांक मूल्य_Arg_Refp = नवीन AstLambdaArgRef{argFl, name, असत्य};
            AstWith* स्थिरांक newp
                = नवीन AstWith{nodep->fileline(), indexArgRefp, मूल्य_Arg_Refp, nullptr};
            // Harvest the identifier_list into AstWith; grammar guarantees AstParseRef.
            जर (restrictedRandomize) {
                newp->restricted(सत्य);
                पर्यंत (Ast_गाठ* स्थिरांक itemp = nodep->exprsp()) {
                    newp->addRestrictedName(VN_AS(itemp, ParseRef)->name());
                    itemp->unlinkFrBack();
                    VL_DO_DANGLING(ढकला_Deletep(itemp), itemp);
                }
            }
            Ast_गाठ* अभिव्यक्ती_Or_Constraintsp = nullptr;
            जर (nodep->exprsp()) अभिव्यक्ती_Or_Constraintsp = nodep->exprsp()->unlinkFrBackWithNext();
            जर (nodep->constraintsp())
                अभिव्यक्ती_Or_Constraintsp = Ast_गाठ::addNext(
                    अभिव्यक्ती_Or_Constraintsp, nodep->constraintsp()->unlinkFrBackWithNext());
            newp->addExprp(अभिव्यक्ती_Or_Constraintsp);  // addExprp() tolerates nullptr
            funcrefp->withp(newp);
        }
        funcrefp->addArgsp(argsp);
        nodep->replaceWith(nodep->funcrefp()->unlinkFrBack());
        VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
    }
    शून्य visit(AstWith* nodep) override {  // FindVisitor::
        // Symbol table needs nodep->name() as the index variable's name
        // Iteration will pickup the AstVar we made under AstWith
        VL_RESTORER(m_curSymp);
        ++m_mod_With_अंक;
        m_curSymp = m_statep->घाला_Block(m_curSymp, "__Vwith" + cvtToStr(m_mod_With_अंक), nodep,
                                          m_classOrPackagep);
        m_curSymp->fallbackp(VL_RESTORER_PREV(m_curSymp));
        UASSERT_OBJ(nodep->indexArgRefp(), nodep, "Missing lambda argref");
        UASSERT_OBJ(nodep->मूल्य_Arg_Refp(), nodep, "Missing lambda argref");
        // Insert argref's name into symbol table
        m_statep->घाला_Sym(m_curSymp, nodep->मूल्य_Arg_Refp()->name(), nodep->मूल्य_Arg_Refp(),
                            nullptr);
        iterateChildren(nodep);
    }
    शून्य visit(AstCoverWith* nodep) override {  // FindVisitor::
        // The range list is outside the scope of the filter, which declares 'item'
        iterateAndNextNull(nodep->subp());
        VL_RESTORER(m_curSymp);
        ++m_mod_With_अंक;
        m_curSymp = m_statep->घाला_Block(m_curSymp, "__VcoverWith" + cvtToStr(m_mod_With_अंक),
                                          nodep, m_classOrPackagep);
        m_curSymp->fallbackp(VL_RESTORER_PREV(m_curSymp));
        iterateAndNextNull(nodep->itemp());
        iterateAndNextNull(nodep->filterp());
    }
    शून्य visit(AstCoverCrossWith* nodep) override {  // FindVisitor::
        // The selection, the 'matches' count, and the cross items' types, of their coverpoints'
        // expressions with any nested scopes, are outside the filter's scope, which declares the
        // items.  As visit(AstVar) would resolve a type in the scope declaring its variable, the
        // items, generated and unique, are inserted directly.
        iterateAndNextNull(nodep->selectp());
        iterateAndNextNull(nodep->countp());
        साठी (AstVar* varp = nodep->varsp(); varp; varp = VN_AS(varp->nextp(), Var)) {
            iterateAndNextNull(varp->childDTypep());
        }
        VL_RESTORER(m_curSymp);
        ++m_mod_With_अंक;
        m_curSymp = m_statep->घाला_Block(m_curSymp, "__VcoverWith" + cvtToStr(m_mod_With_अंक),
                                          nodep, m_classOrPackagep);
        m_curSymp->fallbackp(VL_RESTORER_PREV(m_curSymp));
        साठी (AstVar* varp = nodep->varsp(); varp; varp = VN_AS(varp->nextp(), Var)) {
            m_statep->घाला_Sym(m_curSymp, varp->name(), varp, m_classOrPackagep);
        }
        iterateAndNextNull(nodep->filterp());
    }

    शून्य visit(Ast_गाठ* nodep) override { iterateChildren(nodep); }  // FindVisitor::

    शून्य handleUnvisitedVirtIfaces() {
        Ast_गाठ_Module* स्थिरांक शीर्ष = v3Global.rootp()->modulesp();
        m_curSymp = m_statep->get_गाठ_Sym(शीर्ष);
        साठी (AstIface* ifacep : m_virtIfaces) {
            जर (!ifacep->user4()) {  // virtual interface never assigned any actual interface
                // Create dummy cell to keep the virtual interface alive
                // (later stages assume that non-dead interface has associated cell)
                AstCell* स्थिरांक ifacecellp = नवीन AstCell{ifacep->fileline(),
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

सार्वजनिक:
    // CONSTRUCTORS
    LinkDotFindVisitor(AstNetlist* rootp, LinkDotState* statep)
        : m_statep{statep} {
        UINFO(4, __FUNCTION__ << ": ");
        iterate(rootp);

        जर (!m_virtIfaces.empty()) handleUnvisitedVirtIfaces();
        साठी (AstScope* स्थिरांक scopep : m_scopes) scopep->modp()->addStmtsp(scopep);
        जर (m_statep->forScopeCreation()) {
            V3Stats::addStat("LinkDot, Scope visits", m_statScopeVisits);
        }
    }
    ~LinkDotFindVisitor() override = पूर्वनिर्धारित;
};

//======================================================================

वर्ग LinkDotFindIfaceVisitor final : सार्वजनिक VNVisitor {
    // NODE STATE
    //  *::user1p()             -> See LinkDotState

    // STATE - for current visit position (use VL_RESTORER)
    LinkDotState* स्थिरांक m_statep;  // State to pass between visitors, including symbol table
    Ast_गाठ* m_declp = nullptr;  // Current declaring object that may soon contain IfaceRefDType

    // METHODS
    स्थिरांक VSymEnt* findNonDeclSym(VSymEnt* symp, स्थिरांक string& name) {
        // Find if there is a symbol of given name, ignoring the node that is declaring
        // it (m_declp) itself.  Thus if searching for "ifc" (an interface):
        //
        // module x;  // Finishes here and finds the typedef
        //   typedef foo ifc;
        //   ifc ifc;   // symp starts pointing here, but matches m_declp
        पर्यंत (symp) {
            स्थिरांक VSymEnt* स्थिरांक foundp = symp->findIdFlat(name);
            जर (foundp && foundp->nodep() != m_declp) परत foundp;
            symp = symp->fallbackp();
        }
        परत nullptr;
    }

    // VISITORS
    शून्य visit(AstRefDType* nodep) override {  // FindIfaceVisitor::
        जर (m_statep->forPrimary() && !nodep->classOrPackagep()) {
            UINFO(9, "  FindIfc: " << nodep);
            // If under a var, ignore the var itself as might be e.g. "intf intf;"
            // Critical tests:
            //   t_interface_param_genblk.v  // Checks this does make interface
            //   t_interface_hidden.v  // Checks this doesn't making interface when hidden
            जर (m_statep->exists_गाठ_Sym(nodep)) {
                VSymEnt* symp = m_statep->get_गाठ_Sym(nodep);
                स्थिरांक VSymEnt* foundp = findNonDeclSym(symp, nodep->name());
                Ast_गाठ* foundNodep = nullptr;
                // This: v4make test_regress/t/t_interface_param_genblk.py --debug
                // --debugi-V3LinkDot 9 Passes with this commented out:
                जर (foundp) foundNodep = foundp->nodep();
                जर (!foundNodep) foundNodep = m_statep->findModuleSym(nodep->name());
                जर (foundNodep) UINFO(9, "    Ifc foundNodep " << foundNodep);
                जर (AstIface* स्थिरांक defp = VN_CAST(foundNodep, Iface)) {
                    // Must be found as module name, and not hidden/ by normal symbol (foundp)
                    AstIfaceRefDType* स्थिरांक newp
                        = नवीन AstIfaceRefDType{nodep->fileline(), "", nodep->name()};
                    जर (nodep->paramsp())
                        newp->addParamsp(nodep->paramsp()->unlinkFrBackWithNext());
                    newp->ifacep(defp);
                    newp->user1u(nodep->user1u());
                    UINFO(9, "    Resolved interface " << nodep << "  =>  " << defp);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
                    परत;
                }
            }
        }
        iterateChildren(nodep);
    }
    शून्य visit(AstVar* nodep) override {  // FindIfaceVisitor::
        VL_RESTORER(m_declp);
        m_declp = nodep;
        iterateChildren(nodep);
        AstIfaceRefDType* स्थिरांक ifacerefp = LinkDotState::ifaceRefFromArray(nodep->subDTypep());
        जर (ifacerefp && m_statep->exists_गाठ_Sym(nodep)) {
            // Can't resolve until interfaces and modport names are
            // known; see notes at top
            UINFO(9, "  FindIfc Var IfaceRef " << ifacerefp);
            जर (!ifacerefp->isVirtual()) nodep->setIfaceRef();
            VSymEnt* स्थिरांक symp = m_statep->get_गाठ_Sym(nodep);
            m_statep->घाला_Iface_Var_Sym(symp);
        }
    }
    शून्य visit(Ast_गाठ* nodep) override { iterateChildren(nodep); }  // FindIfaceVisitor::

सार्वजनिक:
    // CONSTRUCTORS
    LinkDotFindIfaceVisitor(AstNetlist* rootp, LinkDotState* statep)
        : m_statep{statep} {
        UINFO(4, __FUNCTION__ << ": ");
        iterate(rootp);
    }
    ~LinkDotFindIfaceVisitor() override = पूर्वनिर्धारित;
};

//======================================================================

वर्ग LinkDotParamVisitor final : सार्वजनिक VNVisitor {
    // NODE STATE
    //  *::user1p()             -> See LinkDotState
    //  *::user2p()             -> See LinkDotState
    //  *::user4()              -> See LinkDotState

    // STATE
    LinkDotState* स्थिरांक m_statep;  // State to pass between visitors, including symbol table
    Ast_गाठ_Module* m_modp = nullptr;  // Current module

    शून्य pin_Implicit_अभिव्यक्ती_Recurse(Ast_गाठ* nodep) {
        // Under a pin, Check interconnect expression for a pin reference or a concat.
        // Create implicit variable as needed
        जर (VN_IS(nodep, Dot)) {  // Not creating a simple implied type,
            // and implying something else would just confuse later errors
        } अन्यथा जर (VN_IS(nodep, VarRef) || VN_IS(nodep, ParseRef)) {
            // To prevent user errors, we should only do single bit
            // implicit vars, however some netlists (MIPS) expect single
            // bit implicit wires to get created with range 0:0 etc.
            m_statep->implicitOkAdd(m_modp, nodep->name());
        }
        // These are perhaps a little too generous, as a SELect of siga[sigb]
        // perhaps shouldn't create an implicit variable.  But, we've warned...
        अन्यथा {
            जर (Ast_गाठ* स्थिरांक refp = nodep->op1p()) pin_Implicit_अभिव्यक्ती_Recurse(refp);
            जर (Ast_गाठ* स्थिरांक refp = nodep->op2p()) pin_Implicit_अभिव्यक्ती_Recurse(refp);
            जर (Ast_गाठ* स्थिरांक refp = nodep->op3p()) pin_Implicit_अभिव्यक्ती_Recurse(refp);
            जर (Ast_गाठ* स्थिरांक refp = nodep->op4p()) pin_Implicit_अभिव्यक्ती_Recurse(refp);
        }
        // Continue along a list (e.g. the terminals under AstImplicit), also after a reference
        जर (Ast_गाठ* स्थिरांक refp = nodep->nextp()) pin_Implicit_अभिव्यक्ती_Recurse(refp);
    }

    // VISITORS
    शून्य visit(AstTypeTable*) override {}  // ParamVisitor::
    शून्य visit(Ast_गाठ_Module* nodep) override {  // ParamVisitor::
        UINFO(5, "   " << nodep);
        जर ((nodep->dead() || !nodep->user4()) && !nodep->hierParams()) {
            UINFO(4, "Mark dead module " << nodep);
            UASSERT_OBJ(!m_statep->forScopeCreation(), nodep,
                        "Dead module persisted past where should have removed");
            // Don't remove now, because we may have a tree of
            // parameterized modules with VARXREFs into the deleted module
            // region.  V3Dead should cleanup.
            // Downstream visitors up until V3Dead need to check for nodep->dead.
            nodep->dead(सत्य);
            परत;
        }
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
    }
    शून्य visit(AstPin* nodep) override {  // ParamVisitor::
        // Pin: Link to submodule's port
        // Deal with implicit definitions - do before Resolve visitor as may
        // be referenced above declaration
        जर (nodep->exprp()  // Else specifically unconnected pin
            && !nodep->svImplicit()) {  // SV 19.11.3: .name pins don't allow implicit decls
            pin_Implicit_अभिव्यक्ती_Recurse(nodep->exprp());
        }
    }
    शून्य visit(AstDefParam* nodep) override {  // ParamVisitor::
        iterateChildren(nodep);
        nodep->v3warn(DEFPARAM, "defparam is deprecated (IEEE 1800-2023 C.4.1)\n"
                                    << nodep->warnMore()
                                    << "... Suggest use instantiation with #(."
                                    << nodep->prettyName() << "(...etc...))");
        तार्किक hasPartSelect = असत्य;
        स्थिरांक string path = extractDottedPath(nodep->pathp(), hasPartSelect);
        UASSERT_OBJ(!hasPartSelect && !path.empty(), nodep, "Unexpected defparam path shape");
        string baddot;
        VSymEnt* okSymp = nullptr;
        VSymEnt* स्थिरांक foundp = m_statep->findDotted(
            nodep->fileline(), m_statep->get_गाठ_Sym(nodep), path, baddot, okSymp, सत्य);
        AstCell* स्थिरांक cellp = foundp ? VN_AS(foundp->nodep(), Cell) : nullptr;
        जर (!cellp) {
            nodep->v3error("In defparam, instance " << path << " never declared");
        } अन्यथा {
            Ast_गाठ_अभिव्यक्ती* स्थिरांक exprp = nodep->rhsp()->unlinkFrBack();
            UINFO(9, "Defparam cell " << path << "." << nodep->name() << " attach-to " << cellp
                                      << "  <= " << exprp);
            // Don't need to check the name of the defparam exists.  V3Param does.
            AstPin* स्थिरांक pinp = नवीन AstPin{nodep->fileline(),
                                            -1,  // Pin# not relevant
                                            nodep->name(), exprp};
            pinp->param(सत्य);
            pinp->paramPath(path);
            cellp->addParamsp(pinp);
        }
        VL_DO_DANGLING(nodep->unlinkFrBack()->delete_वृक्ष(), nodep);
    }
    शून्य visit(AstPort* nodep) override {  // ParamVisitor::
        // Port: Stash the pin number
        // Need to set pin numbers after varnames are created
        // But before we do the final resolution based on names
        VSymEnt* स्थिरांक foundp = m_statep->get_गाठ_Sym(m_modp)->findIdFlat(nodep->name());
        AstVar* स्थिरांक refp = foundp ? VN_CAST(foundp->nodep(), Var) : nullptr;
        जर (!foundp) {
            nodep->v3error(
                "Input/output/inout declaration not found for port: " << nodep->prettyNameQ());
        } अन्यथा जर (!refp || (!refp->isIO() && !refp->isIfaceRef())) {
            nodep->v3error("Pin is not an in/out/inout/interface: " << nodep->prettyNameQ());
        } अन्यथा {
            जर (refp->user4()) {
                nodep->v3error("Duplicate declaration of port: "
                               << nodep->prettyNameQ() << '\n'
                               << nodep->warnContextPrimary() << '\n'
                               << refp->warnOther() << "... Location of original declaration\n"
                               << refp->warnContextSecondary());
            }
            refp->user4(सत्य);
            VSymEnt* स्थिरांक symp = m_statep->घाला_Sym(m_statep->get_गाठ_Sym(m_modp),
                                                      "__pinNumber" + cvtToStr(nodep->pin_अंक()),
                                                      refp, nullptr /*classOrPackagep*/);
            symp->exported(असत्य);
            refp->pin_अंक(nodep->pin_अंक());
            // Put the variable where the port is, so that variables stay
            // in pin number sorted order. Otherwise hierarchical or JSON
            // may botch by-position instances.
            nodep->addHereThisAsNext(refp->unlinkFrBack());
        }
        // Ports not needed any more
        VL_DO_DANGLING(nodep->unlinkFrBack()->delete_वृक्ष(), nodep);
    }
    शून्य visit(AstAssignW* nodep) override {  // ParamVisitor::
        // Deal with implicit definitions
        // We used to nodep->allowImplicit() here, but it turns out
        // normal "assigns" can also make implicit wires.  Yuk.
        pin_Implicit_अभिव्यक्ती_Recurse(nodep->lhsp());
        iterateChildren(nodep);
    }

    शून्य visit(AstImplicit* nodep) override {  // ParamVisitor::
        // Unsupported gates need implicit creation
        pin_Implicit_अभिव्यक्ती_Recurse(nodep->exprsp());
        // We're done with implicit gates
        VL_DO_DANGLING(nodep->unlinkFrBack()->delete_वृक्ष(), nodep);
    }
    शून्य visit(AstClassOrPackageRef* nodep) override {  // ParamVisitor::
        जर (स्वतः* स्थिरांक fwdp = VN_CAST(nodep->classOrPackageNodep(), TypedefFwd)) {
            // Relink forward definitions to the "real" definition
            VSymEnt* स्थिरांक foundp = m_statep->get_गाठ_Sym(fwdp)->findIdFallback(fwdp->name());
            जर (foundp && (VN_IS(foundp->nodep(), Class) || VN_IS(foundp->nodep(), Package))) {
                nodep->classOrPackagep(VN_AS(foundp->nodep(), गाठ_Module));
            } अन्यथा जर (foundp && VN_IS(foundp->nodep(), ParamTypeDType)) {
                UASSERT(m_statep->forPrimary(), "Param types should have been resolved");
                nodep->classOrPackageNodep(VN_AS(foundp->nodep(), ParamTypeDType));
            } अन्यथा {
                जर (foundp) UINFO(1, "found nodep = " << foundp->nodep());
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
    शून्य visit(AstPull* nodep) override {  // ParamVisitor::
        // Deal with implicit definitions
        // We used to nodep->allowImplicit() here, but it turns out
        // normal "assigns" can also make implicit wires.  Yuk.
        pin_Implicit_अभिव्यक्ती_Recurse(nodep->lhsp());
        iterateChildren(nodep);
    }
    शून्य visit(AstTypedefFwd* nodep) override {  // ParamVisitor::
        VSymEnt* स्थिरांक foundp = m_statep->get_गाठ_Sym(nodep)->findIdFallback(nodep->name());
        जर (foundp) {
            // If the typedef was earlier in source order (tokenNum), then remember that earlier
            // point to avoid error something wasn't declared
            // Might be forward declaring something odd (with declTokenNumSetMin not implemented,
            // but should be harmless to ignore as this is just for error detection
            foundp->nodep()->decl_Token_अंक_Set_Min(nodep->fileline()->token_अंक());
        } अन्यथा जर (v3Global.opt.pedantic()) {
            // We only check it was ever really defined in pedantic mode, as it
            // might have been in a header file referring to a module we never
            // needed so there are false positives
            nodep->v3error(
                "Forward typedef unused or does not resolve to a data type (IEEE 1800-2023 6.18): "
                << nodep->prettyNameQ());
        }
        // We only needed the forward declaration in order to parse correctly.
        // Delete later as may be ClassOrPackageRef's still pointing to it
        VL_DO_DANGLING(ढकला_Deletep(nodep->unlinkFrBack()), nodep);
    }

    शून्य visit(Ast_गाठ* nodep) override { iterateChildren(nodep); }  // ParamVisitor::

सार्वजनिक:
    // CONSTRUCTORS
    LinkDotParamVisitor(AstNetlist* rootp, LinkDotState* statep)
        : m_statep{statep} {
        UINFO(4, __FUNCTION__ << ": ");
        iterate(rootp);
    }
    ~LinkDotParamVisitor() override = पूर्वनिर्धारित;
};

//======================================================================

वर्ग LinkDotScopeVisitor final : सार्वजनिक VNVisitor {

    // STATE
    LinkDotState* स्थिरांक m_statep;  // State to pass between visitors, including symbol table
    स्थिरांक AstScope* m_scopep = nullptr;  // The current scope
    VSymEnt* m_modSymp = nullptr;  // Symbol entry for current module
    // Deferred AliasScope processing - must be done outer-to-inner for correct alias resolution
    std::vector<std::pair<AstAliasScope*, VSymEnt*>> m_deferredAliasScopes;

    // METHODS
सार्वजनिक:
    // getAlias with setVarAlias and setIfaceAlias implement disjoint-set data structure.
    // This algorithm is needed for the case when multiple alias statements
    // reference partially the same variables.
    स्थिर AstVarScope* getAlias(AstVarScope* स्थिरांक vscp) {
        जर (vscp->user2p() && vscp != vscp->user2p()) {
            AstVarScope* स्थिरांक aliasp = getAlias(VN_AS(vscp->user2p(), VarScope));
            vscp->user2p(aliasp);
            परत aliasp;
        } अन्यथा {
            परत vscp;
        }
    }

खाजगी:
    // Alias an interface reference to the interface it is connected to
    शून्य setIfaceAlias(AstVarScope* स्थिरांक vscp, AstVarScope* स्थिरांक aliasp) {
        UASSERT_OBJ(vscp->varp()->isIfaceRef(), vscp, "Interface alias of non-interface");
        UASSERT_OBJ(aliasp->varp()->isIfaceRef(), aliasp, "Interface alias of non-interface");
        getAlias(vscp)->user2p(getAlias(aliasp));
    }

    // Alias two variables. An output port in the lower scope survives, unless the other
    // must keep its own storage.
    शून्य setVarAlias(AstVarScope* ap, AstVarScope* bp) {
        UASSERT_OBJ(!ap->varp()->isIfaceRef(), ap, "Variable alias of interface");
        UASSERT_OBJ(!bp->varp()->isIfaceRef(), bp, "Variable alias of interface");
        ap = getAlias(ap);
        bp = getAlias(bp);
        जर (ap == bp) परत;
        स्थिरांक तार्किक aWins = [&]() {
            स्थिरांक AstVar* स्थिरांक aVarp = ap->varp();
            स्थिरांक AstVar* स्थिरांक bVarp = bp->varp();
            // A primary IO is accessed directly by the user of the model, so must survive
            UASSERT_OBJ(!aVarp->isPrimaryIO() || !bVarp->isPrimaryIO(), ap,
                        "Alias of two primary IOs");
            जर (bVarp->isPrimaryIO()) परत असत्य;
            जर (aVarp->isPrimaryIO()) परत सत्य;
            // A forced or public variable must keep its own storage. If both are, either works.
            जर (aVarp->isForced() != bVarp->isForced()) परत aVarp->isForced();
            जर (aVarp->isSigPublic() != bVarp->isSigPublic()) परत aVarp->isSigPublic();
            // Otherwise prefer an output port in the lower scope. This enables better V3Combine
            // due to having fewer upward hierarchical references (which prevent combining).
            जर (aVarp->direction() != VDirection::OUTPUT) परत असत्य;
            // Is 'ap' strictly below 'bp' in the hierarchy
            साठी (स्थिरांक AstScope* scopep = ap->scopep()->aboveScopep(); scopep;
                 scopep = scopep->aboveScopep()) {
                जर (scopep == bp->scopep()) परत सत्य;
            }
            परत असत्य;
        }();
        जर (aWins) {
            bp->user2p(ap);
        } अन्यथा {
            ap->user2p(bp);
        }
    }

    // VISITORS
    शून्य visit(AstNetlist* nodep) override {  // ScopeVisitor::
        // Recurse..., backward as must do packages before using packages
        iterateChildrenBackwardsConst(nodep);
    }
    शून्य visit(AstScope* nodep) override {  // ScopeVisitor::
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
        जर (AstIface* स्थिरांक ifacep = VN_CAST(nodep->modp(), Iface)) {
            m_statep->घाला_Iface_Mod_Sym(ifacep, m_modSymp);
        }
        m_scopep = nodep;
        iterateChildren(nodep);
    }
    शून्य visit(AstVarScope* nodep) override {  // ScopeVisitor::
        जर (!nodep->varp()->isFuncLocal() && !nodep->varp()->isClassMember()) {
            VSymEnt* स्थिरांक varSymp
                = m_statep->घाला_Sym(m_modSymp, nodep->varp()->name(), nodep, nullptr);
            जर (nodep->varp()->isIfaceRef()) {
                AstIfaceRefDType* स्थिरांक dtypep
                    = LinkDotState::ifaceRefFromArray(nodep->varp()->dtypep());
                UASSERT_OBJ(dtypep, nodep, "Non AstIfaceRefDType on isIfaceRef() var");
                जर (nodep->varp()->isIfaceParent()) {
                    UINFO(9, "Iface parent ref var " << nodep->varp()->name() << " " << nodep);
                    // Find the interface cell the var references
                    UINFO(9, "Iface parent dtype " << dtypep);
                    स्थिरांक string ifcellname = dtypep->cellName();
                    string baddot;
                    VSymEnt* okSymp;
                    VSymEnt* cellSymp = m_statep->findDotted(nodep->fileline(), m_modSymp,
                                                             ifcellname, baddot, okSymp, असत्य);
                    UASSERT_OBJ(
                        cellSymp, nodep,
                        "No symbol for interface instance: " << nodep->prettyNameQ(ifcellname));
                    UINFO(5, "       Found interface instance: se" << cvtToHex(cellSymp) << " "
                                                                   << cellSymp->nodep());
                    जर (dtypep->modportName() != "") {
                        // Look up the modport within the interface cell's symbol table
                        VSymEnt* स्थिरांक mpSymp = cellSymp->findIdFallback(dtypep->modportName());
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
                    m_statep->घाला_Scope_Alias(LinkDotState::SAMN_IFTOP, varSymp, cellSymp);
                }
            }
        }
    }
    शून्य visit(Ast_गाठ_FTask* nodep) override {  // ScopeVisitor::
        VSymEnt* स्थिरांक symp = m_statep->घाला_Block(m_modSymp, nodep->name(), nodep, nullptr);
        symp->fallbackp(m_modSymp);
        iterateChildren(nodep);
    }
    शून्य visit(Ast_गाठ_Foreach* nodep) override {  // ScopeVisitor::
        VSymEnt* स्थिरांक symp = m_statep->घाला_Block(m_modSymp, nodep->name(), nodep, nullptr);
        symp->fallbackp(m_modSymp);
        // No recursion, we don't want to pick up variables
    }
    शून्य visit(AstConstraintForeach* nodep) override {  // ScopeVisitor::
        iterateChildren(nodep);
    }
    शून्य visit(AstWith* nodep) override {  // ScopeVisitor::
        VSymEnt* स्थिरांक symp = m_statep->घाला_Block(m_modSymp, nodep->name(), nodep, nullptr);
        symp->fallbackp(m_modSymp);
        iterateChildren(nodep);
    }
    शून्य visit(AstAlias* nodep) override {  // ScopeVisitor::
        // Track aliases
        UINFOTREE(9, nodep, "", "alias");
        AstVarScope* aliasVscp = nullptr;
        साठी (Ast_गाठ* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
            AstVarScope* vscp = nullptr;
            जर (स्थिरांक AstVarRef* स्थिरांक refp = VN_CAST(itemp, VarRef)) {
                vscp = refp->varScopep();
            } अन्यथा {
                // Reaches into the scope of an instance, look it up by name
                स्थिरांक AstVarXRef* स्थिरांक xrefp = VN_AS(itemp, VarXRef);
                स्थिरांक string scopename = xrefp->dotted() + "." + xrefp->name();
                string baddot;
                VSymEnt* okSymp;
                VSymEnt* स्थिरांक symp = m_statep->findDotted(xrefp->fileline(), m_modSymp, scopename,
                                                           baddot, okSymp, असत्य);
                UASSERT_OBJ(symp, nodep, "No symbol for alias item: " << scopename);
                vscp = VN_CAST(symp->nodep(), VarScope);
            }
            UASSERT_OBJ(vscp, nodep, "VarScope unset");
            जर (aliasVscp) {
                setVarAlias(aliasVscp, vscp);
            } अन्यथा {
                aliasVscp = vscp;
            }
        }
        iterateChildren(nodep);
        VL_DO_DANGLING(ढकला_Deletep(nodep->unlinkFrBack()), nodep);
    }
    शून्य visit(AstAliasScope* nodep) override {  // ScopeVisitor::
        // Defer AliasScope processing - must process outer scopes before inner ones
        // so that nested interface port alias resolution works correctly
        UINFO(5, "ALIASSCOPE (deferred) " << nodep);
        m_deferredAliasScopes.emplace_back(nodep, m_modSymp);
    }
    शून्य processAliasScope(AstAliasScope* nodep, VSymEnt* modSymp) {
        UINFO(5, "ALIASSCOPE  " << nodep);
        UINFOTREE(9, nodep, "", "avs");
        VSymEnt* rhsSymp;
        {
            AstVarRef* स्थिरांक refp = VN_CAST(nodep->rhsp(), VarRef);
            AstVarXRef* स्थिरांक xrefp = VN_CAST(nodep->rhsp(), VarXRef);
            UASSERT_OBJ(refp || xrefp, nodep,
                        "Unsupported: Non Var(X)Ref attached to interface pin");
            string inl
                = ((xrefp && xrefp->inlinedDots().size()) ? (xrefp->inlinedDots() + "__DOT__")
                                                          : "");
            स्थिरांक string dottedPath
                = (xrefp && !xrefp->dotted().empty()) ? (xrefp->dotted() + ".") : "";
            VSymEnt* symp = nullptr;
            string scopename;
            पर्यंत (!symp) {
                scopename = refp ? refp->name()
                                 : (inl.size() ? (inl + dottedPath + xrefp->name())
                                               : (dottedPath + xrefp->name()));
                string baddot;
                VSymEnt* okSymp;
                symp = m_statep->findDotted(nodep->rhsp()->fileline(), modSymp, scopename, baddot,
                                            okSymp, सत्य);
                जर (inl == "") थांबा;
                inl = LinkDotState::removeLastInlineScope(inl);
            }
            UASSERT_OBJ(symp, nodep, "No symbol for interface alias rhs");
            UINFO(5, "       Found a linked scope RHS: " << scopename << "  se" << cvtToHex(symp)
                                                         << " " << symp->nodep());
            rhsSymp = symp;
        }
        VSymEnt* lhsSymp;
        {
            स्थिरांक AstVarXRef* स्थिरांक xrefp = VN_CAST(nodep->lhsp(), VarXRef);
            स्थिरांक AstVarRef* स्थिरांक refp = VN_CAST(nodep->lhsp(), VarRef);

            UASSERT_OBJ(refp || xrefp, nodep,
                        "Unsupported: Non Var(X)Ref attached to interface pin");
            स्थिरांक string scopename
                = refp ? refp->varp()->name() : xrefp->dotted() + "." + xrefp->name();
            string baddot;
            VSymEnt* okSymp;
            VSymEnt* स्थिरांक symp = m_statep->findDotted(nodep->lhsp()->fileline(), modSymp,
                                                       scopename, baddot, okSymp, असत्य);
            UASSERT_OBJ(symp, nodep, "No symbol for interface alias lhs");
            UINFO(5, "       Found a linked scope LHS: " << scopename << "  se" << cvtToHex(symp)
                                                         << " " << symp->nodep());
            lhsSymp = symp;
        }
        // Remember the alias - can't do it yet because we may have additional symbols to be added,
        // or maybe an alias of an alias
        m_statep->घाला_Scope_Alias(LinkDotState::SAMN_IFTOP, lhsSymp, rhsSymp);
        AstVarScope* स्थिरांक lhsVscp = VN_CAST(lhsSymp->nodep(), VarScope);
        AstVarScope* स्थिरांक rhsVscp = VN_CAST(rhsSymp->nodep(), VarScope);
        UASSERT_OBJ(lhsVscp && rhsVscp, nodep, "Interface alias missing variable scope");
        setIfaceAlias(lhsVscp, rhsVscp);
        // We have stored the link, we don't need these any more
        VL_DO_DANGLING(nodep->unlinkFrBack()->delete_वृक्ष(), nodep);
    }
    शून्य processDeferredAliasScopes() {
        // Sort by hierarchy depth (shallower first) so outer aliases are resolved before inner
        // Pre-compute depth map to avoid O(N log N * D) complexity in sort comparisons
        std::unordered_map<VSymEnt*, पूर्णांक> depthMap;
        साठी (स्थिरांक स्वतः& pair : m_deferredAliasScopes) {
            VSymEnt* स्थिरांक symp = pair.second;
            जर (depthMap.find(symp) == depthMap.end()) {
                पूर्णांक depth = 0;
                साठी (VSymEnt* p = symp; p; p = p->parentp()) ++depth;
                depthMap[symp] = depth;
            }
        }
        std::stable_sort(m_deferredAliasScopes.begin(), m_deferredAliasScopes.end(),
                         [&depthMap](स्थिरांक std::pair<AstAliasScope*, VSymEnt*>& a,
                                     स्थिरांक std::pair<AstAliasScope*, VSymEnt*>& b) {
                             परत depthMap.at(a.second) < depthMap.at(b.second);
                         });
        // Process in sorted order
        साठी (स्वतः& pair : m_deferredAliasScopes) { processAliasScope(pair.first, pair.second); }
        m_deferredAliasScopes.clear();
    }
    शून्य visit(Ast_गाठ_Gen* nodep) override {  // ScopeVisitor::  // LCOV_EXCL_LINE
        nodep->v3fatalSrc("Generate constructs should have been reduced out");
    }
    // For speed, don't recurse things that can't have scope
    // Note we allow AstNodeStmt's as generates may be under them
    शून्य visit(AstCell*) override {}  // ScopeVisitor::
    शून्य visit(AstVar*) override {}  // ScopeVisitor::
    शून्य visit(Ast_गाठ* nodep) override { iterateChildren(nodep); }  // ScopeVisitor::

सार्वजनिक:
    // CONSTRUCTORS
    LinkDotScopeVisitor(AstNetlist* rootp, LinkDotState* statep)
        : m_statep{statep} {
        UINFO(4, __FUNCTION__ << ": ");
        iterate(rootp);
        // Process deferred AliasScopes in outer-to-inner order
        processDeferredAliasScopes();
    }
    ~LinkDotScopeVisitor() override = पूर्वनिर्धारित;
};

//======================================================================

// Iterate an interface to resolve modports
वर्ग LinkDotIfaceVisitor final : सार्वजनिक VNVisitor {
    // STATE
    LinkDotState* स्थिरांक m_statep;  // State to pass between visitors, including symbol table
    VSymEnt* m_curSymp;  // Symbol Entry for current table, where to lookup/insert

    // VISITORS
    शून्य visit(AstModport* nodep) override {  // IfaceVisitor::
        // Modport: Remember its name for later resolution
        UINFO(5, "   fiv: " << nodep);
        VL_RESTORER(m_curSymp);
        {
            // Create symbol table for the vars
            m_curSymp = m_statep->घाला_Block(m_curSymp, nodep->name(), nodep, nullptr);
            m_curSymp->fallbackp(VL_RESTORER_PREV(m_curSymp));
            iterateChildren(nodep);
        }
    }
    शून्य visit(AstModportFTaskRef* nodep) override {  // IfaceVisitor::
        UINFO(5, "   fif: " << nodep);
        iterateChildren(nodep);
        VSymEnt* स्थिरांक symp = m_curSymp->findIdFallback(nodep->name());
        जर (!symp) {
            nodep->v3error("Modport item not found: " << nodep->prettyNameQ());
        } अन्यथा जर (Ast_गाठ_FTask* स्थिरांक ftaskp = VN_CAST(symp->nodep(), गाठ_FTask)) {
            // Make symbol under modport that points at the _interface_'s var, not the modport.
            nodep->ftaskp(ftaskp);
            VSymEnt* स्थिरांक subSymp
                = m_statep->घाला_Sym(m_curSymp, nodep->name(), ftaskp, nullptr /*package*/);
            m_statep->घाला_Scope_Alias(LinkDotState::SAMN_MODPORT, subSymp, symp);
        } अन्यथा {
            nodep->v3error("Modport item is not a function/task: " << nodep->prettyNameQ());
        }
        जर (m_statep->forScopeCreation()) {
            // Done with AstModportFTaskRef.
            // Delete to prevent problems if we dead-delete pointed to ftask
            nodep->unlinkFrBack();
            VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
        }
    }

    // Helper to resolve remaining path through a nested interface
    // When findDotted() partially matches (okSymp set, baddot non-empty),
    // this follows the interface type to resolve the remaining path.
    // Returns the resolved symbol, or nullptr if not found.
    // On success, clears baddot; on partial match of multi-level path, updates baddot.
    VSymEnt* resolveNestedInterfacePath(File_ओळ* fl, VSymEnt* okSymp, string& baddot) {
        जर (!okSymp || baddot.empty()) परत nullptr;

        स्थिर constexpr पूर्णांक MAX_NESTING_DEPTH = 64;
        VSymEnt* curOkSymp = okSymp;

        साठी (पूर्णांक depth = 0; depth < MAX_NESTING_DEPTH; ++depth) {
            // Try to get interface from the partially-matched symbol
            AstIface* ifacep = nullptr;
            जर (स्थिरांक AstCell* स्थिरांक cellp = VN_CAST(curOkSymp->nodep(), Cell)) {
                ifacep = VN_CAST(cellp->modp(), Iface);
            } अन्यथा जर (स्थिरांक AstVar* स्थिरांक varp = VN_CAST(curOkSymp->nodep(), Var)) {
                जर (varp->isIfaceRef()) {
                    जर (स्थिरांक AstIfaceRefDType* स्थिरांक ifaceRefp
                        = LinkDotState::ifaceRefFromArray(varp->dtypep())) {
                        ifacep = ifaceRefp->ifaceViaCellp();
                    }
                }
            }

            जर (!ifacep || !m_statep->exists_गाठ_Sym(ifacep)) परत nullptr;

            VSymEnt* स्थिरांक ifaceSymp = m_statep->get_गाठ_Sym(ifacep);

            जर (baddot.find('.') == string::npos) {
                // Simple identifier - direct lookup
                VSymEnt* स्थिरांक symp = ifaceSymp->findIdFallback(baddot);
                जर (symp) baddot.clear();
                परत symp;
            }

            // Multi-level path - use findDotted for partial resolution
            string remainingBaddot;
            VSymEnt* remainingOkSymp = nullptr;
            VSymEnt* स्थिरांक symp = m_statep->findDotted(fl, ifaceSymp, baddot, remainingBaddot,
                                                       remainingOkSymp, सत्य);
            जर (symp) {
                baddot = remainingBaddot;
                परत symp;
            }

            // findDotted partially matched - check progress
            जर (remainingBaddot == baddot || !remainingOkSymp) परत nullptr;

            // Continue resolving with updated state
            baddot = remainingBaddot;
            curOkSymp = remainingOkSymp;
        }

        UINFO(1, "Nested interface resolution depth limit exceeded at " << fl);
        परत nullptr;
    }

    // Resolve a modport expression to find the referenced symbol
    VSymEnt* resolveModportExpression(File_ओळ* fl, Ast_गाठ_अभिव्यक्ती* exprp) {
        UINFO(5, "     resolveModportExpression: " << exprp);
        VSymEnt* symp = nullptr;
        जर (AstParseRef* स्थिरांक refp = VN_CAST(exprp, ParseRef)) {
            // Simple variable reference: modport mp(input .a(sig_a))
            symp = m_curSymp->findIdFallback(refp->name());
        } अन्यथा जर (AstVarRef* स्थिरांक refp = VN_CAST(exprp, VarRef)) {
            // Already resolved VarRef (can happen if iterateChildren resolved it)
            जर (refp->varScopep()) {
                // During scope creation, VarRef may have VarScope already linked
                symp = m_statep->get_गाठ_Sym(refp->varScopep());
            } अन्यथा जर (refp->varp()) {
                symp = m_curSymp->findIdFallback(refp->varp()->name());
            }
        } अन्यथा जर (AstVarXRef* स्थिरांक refp = VN_CAST(exprp, VarXRef)) {
            // Resolved VarXRef (dotted reference resolved by earlier pass)
            जर (refp->varp()) {
                // For nested interfaces, the var is in a different scope
                // First try to find the symbol via the var itself
                जर (m_statep->exists_गाठ_Sym(refp->varp())) {
                    symp = m_statep->get_गाठ_Sym(refp->varp());
                } अन्यथा {
                    // Fallback: look up in current scope
                    symp = m_curSymp->findIdFallback(refp->varp()->name());
                }
            } अन्यथा जर (!refp->dotted().empty()) {
                // varp not set yet - use dotted path to find the symbol
                // The dotted part (e.g., "base") is the interface cell, and name is the member
                स्थिरांक string fullPath = refp->dotted() + "." + refp->name();
                string baddot;
                VSymEnt* okSymp = nullptr;
                symp = m_statep->findDotted(fl, m_curSymp, fullPath, baddot, okSymp, सत्य);
                // Handle nested interface path when findDotted had partial match
                जर (okSymp && !baddot.empty()) {
                    VSymEnt* स्थिरांक resolved = resolveNestedInterfacePath(fl, okSymp, baddot);
                    जर (resolved) symp = resolved;
                }
            }
        } अन्यथा जर (AstDot* स्थिरांक dotp = VN_CAST(exprp, Dot)) {
            // Dotted path: modport mp(input .a(inner.sig))
            तार्किक hasPartSelect = असत्य;
            स्थिरांक string dottedPath = extractDottedPath(dotp, hasPartSelect);
            जर (hasPartSelect) {
                fl->v3warn(
                    E_UNSUPPORTED,
                    "Unsupported: Modport expression with part select (IEEE 1800-2023 25.5.4)");
            } अन्यथा {
                string baddot;
                VSymEnt* okSymp = nullptr;
                symp = m_statep->findDotted(fl, m_curSymp, dottedPath, baddot, okSymp, सत्य);

                // Handle nested interface path when findDotted had partial match
                जर (okSymp && !baddot.empty()) {
                    VSymEnt* स्थिरांक resolved = resolveNestedInterfacePath(fl, okSymp, baddot);
                    जर (resolved) symp = resolved;
                }

                जर (!symp || !baddot.empty()) {
                    fl->v3error("Can't find modport expression target: "
                                << Ast_गाठ::prettyNameQ(dottedPath));
                    symp = nullptr;
                }
            }
        } अन्यथा जर (VN_IS(exprp, SelBit) || VN_IS(exprp, SelExtract)) {
            // Part select expressions not yet supported
            fl->v3warn(E_UNSUPPORTED,
                       "Unsupported: Modport expression with part select (IEEE 1800-2023 25.5.4)");
        } अन्यथा {
            // Other expression types not supported
            fl->v3warn(E_UNSUPPORTED,
                       "Unsupported: Complex modport expression (IEEE 1800-2023 25.5.4)");
        }
        परत symp;
    }

    शून्य visit(AstModportVarRef* nodep) override {  // IfaceVisitor::
        UINFO(5, "   fiv: " << nodep);
        iterateChildren(nodep);
        VSymEnt* symp = nullptr;
        जर (nodep->exprp()) {
            // Modport expression syntax: modport mp(input .port_name(expression))
            symp = resolveModportExpression(nodep->fileline(), nodep->exprp());
        } अन्यथा {
            symp = m_curSymp->findIdFallback(nodep->name());
        }
        जर (!symp) {
            nodep->v3error("Modport item not found: " << nodep->prettyNameQ());
        } अन्यथा जर (AstVar* स्थिरांक varp = VN_CAST(symp->nodep(), Var)) {
            // Make symbol under modport that points at the _interface_'s var via the modport.
            // (Need modport still to test input/output markings)
            nodep->varp(varp);
            जर (nodep->exprp()) {
                // For modport expressions, insert symbol pointing to the underlying var (not the
                // ModportVarRef which will be deleted during scope creation). The virtual port
                // name maps to the real signal's var.
                VSymEnt* स्थिरांक subSymp
                    = m_statep->घाला_Sym(m_curSymp, nodep->name(), varp, nullptr /*package*/);
                m_statep->घाला_Scope_Alias(LinkDotState::SAMN_MODPORT, subSymp, symp);
            } अन्यथा {
                // For regular modport items, insert symbol pointing to ModportVarRef for
                // input/output marking tests.
                m_statep->घाला_Sym(m_curSymp, nodep->name(), nodep, nullptr /*package*/);
            }
        } अन्यथा जर (AstVarScope* स्थिरांक vscp = VN_CAST(symp->nodep(), VarScope)) {
            // Make symbol under modport that points at the _interface_'s var, not the modport.
            nodep->varp(vscp->varp());
            // Only insert symbol for modport expression virtual ports (exprp set).
            // For regular modport items, don't insert into the shared modport table because
            // each instance would overwrite the previous one, causing wrong VarScope lookups.
            // Regular items are found via the modport's fallback to the interface.
            जर (nodep->exprp()) {
                VSymEnt* स्थिरांक subSymp
                    = m_statep->घाला_Sym(m_curSymp, nodep->name(), vscp, nullptr /*package*/);
                m_statep->घाला_Scope_Alias(LinkDotState::SAMN_MODPORT, subSymp, symp);
            }
        } अन्यथा {
            nodep->v3error("Modport item is not a variable: " << nodep->prettyNameQ());
        }
        जर (m_statep->forScopeCreation()) {
            // Done with AstModportVarRef.
            // Delete to prevent problems if we dead-delete pointed to variable
            nodep->unlinkFrBack();
            VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
        }
    }
    शून्य visit(AstModportClockingRef* nodep) override {  // IfaceVisitor::
        UINFO(5, "   fic: " << nodep);
        iterateChildren(nodep);
        VSymEnt* स्थिरांक symp = m_curSymp->findIdFallback(nodep->name());
        जर (!symp) {
            nodep->v3error("Modport item not found: " << nodep->prettyNameQ());
        } अन्यथा जर (AstClocking* स्थिरांक clockingp = VN_CAST(symp->nodep(), Clocking)) {
            nodep->clockingp(clockingp);
            m_statep->घाला_Sym(m_curSymp, nodep->name(), nodep, nullptr /*package*/);
        } अन्यथा {
            nodep->v3error("Modport item is not a clocking block: " << nodep->prettyNameQ());
        }
    }
    शून्य visit(AstScope* nodep) override {  // IfaceVisitor::
        जर (nodep->user3SetOnce()) परत;
        iterateChildren(nodep);
    }
    शून्य visit(Ast_गाठ* nodep) override { iterateChildren(nodep); }  // IfaceVisitor::

सार्वजनिक:
    // CONSTRUCTORS
    LinkDotIfaceVisitor(AstIface* nodep, VSymEnt* curSymp, LinkDotState* statep)
        : m_statep{statep}
        , m_curSymp{curSymp} {
        UINFO(4, __FUNCTION__ << ": ");
        iterate(nodep);
    }
    ~LinkDotIfaceVisitor() override = पूर्वनिर्धारित;
};

शून्य LinkDotState::computeIfaceModSyms() {
    //  AstScope::user3()  // bool. Already iterated by a LinkDotIfaceVisitor
    स्थिरांक VNUser3InUse user3InUse;
    साठी (स्थिरांक स्वतः& itr : m_ifaceModSyms) {
        AstIface* स्थिरांक nodep = itr.first;
        VSymEnt* स्थिरांक symp = itr.second;
        LinkDotIfaceVisitor{nodep, symp, हे};
    }
    m_ifaceModSyms.clear();
}

//======================================================================

वर्ग LinkDotResolveVisitor final : सार्वजनिक VNVisitor {
    // NODE STATE
    // Cleared on global
    //  *::user1p()             -> See LinkDotState
    //  *::user2p()             -> See LinkDotState
    //  *::user3()              // bool. Processed
    //  *::user4()              -> See LinkDotState
    स्थिरांक VNUser3InUse m_inuser3;

    // TYPES
    गणना DotPosition : uint8_t {
        // Must match ascii() method below
        DP_NONE = 0,  // Not under a DOT
        DP_PACKAGE,  // {package-or-class}:: DOT
        DP_FIRST,  // {scope-or-var} DOT
        DP_SCOPE,  // DOT... {scope-or-var} DOT
        DP_FINAL,  // [DOT...] {var-or-func-or-dtype} with no following dots
        DP_MEMBER  // DOT {member-name} [DOT...]
    };

    // STATE
    LinkDotState* स्थिरांक m_statep;  // State, including dotted symbol table
    VSymEnt* m_curSymp = nullptr;  // SymEnt for current lookup point
    VSymEnt* m_modSymp = nullptr;  // SymEnt for current module
    VSymEnt* m_pinSymp = nullptr;  // SymEnt for pin lookups
    VSymEnt* m_randSymp = nullptr;  // SymEnt for randomize target class's lookups
    स्थिरांक AstCell* m_cellp = nullptr;  // Current cell
    Ast_गाठ_Module* m_modp = nullptr;  // Current module
    Ast_गाठ_FTask* m_ftaskp = nullptr;  // Current function/task
    AstMethodCall* m_randMethodCallp = nullptr;  // Current randomize() method call
    पूर्णांक m_modport_अंक = 0;  // Uniqueify modport numbers
    पूर्णांक m_indent = 0;  // Indentation (tree depth) for debug
    तार्किक m_inSens = असत्य;  // True if in senitem
    स्थिरांक AstWith* m_currentWithp = nullptr;  // Enclosing 'with' (nullptr if none)
    std::set<std::string>
        m_restrictedNamesUsed;  // Names from current 'with (id_list)' that resolved into target
    तार्किक m_genericIfaceModule = असत्य;  // True if in module containing generic interface
    std::map<std::string, Ast_गाठ*> m_ifClassImpNames;  // Names imported from interface class
    // Inherited interface implementations needing a forward in the current class, by name
    std::map<std::string, स्थिरांक VSymEnt*> m_ifForwards;
    std::set<AstClass*> m_extendsParam;  // Classes that have a parameterized super class
                                         // (except the default instances)
                                         // They are added to the set only in linkDotPrimary.
    तार्किक m_insideClassExtParam = असत्य;  // Inside a class from m_extendsParam
    AstNew* m_explicitSuperNewp = nullptr;  // Hit a "super.new" call inside a "new" function
    std::map<Ast_गाठ*, AstPin*> m_usedPins;  // Pin used in this cell, map to duplicate
    std::map<Ast_गाठ*, std::map<std::string, AstPin*>> m_usedDefParamPins;
    // Defparam pins used for a given formal, by hierarchical path
    std::map<std::string, Ast_गाठ_Module*> m_modulesToRevisit;  // Modules to revisit a second time
    Ast_गाठ* m_lastDeferredp = nullptr;  // Last node which requested a revisit of its module
    Ast_गाठ_DType* m_packedArrayDtp = nullptr;  // Datatype reference for packed array
    तार्किक m_inPackedArray = असत्य;  // Currently traversing a packed array tree
    तार्किक m_replaceWithAlias
        = सत्य;  // Replace VarScope with an alias. Used in the handling of AstAlias
    तार्किक m_isParam = असत्य;  // Specifies whether currently visiting param variable
    तार्किक m_resolvingTypedef = असत्य;  // Currently traversing a Typedef tree

    संरचना DotStates final {
        DotPosition m_dotPos;  // Scope part of dotted resolution
        VSymEnt* m_dotSymp;  // SymEnt for dotted AstParse lookup
        स्थिरांक AstDot* m_dotp;  // Current dot
        तार्किक m_super;  // Starts with super reference
        तार्किक m_unresolvedCell;  // Unresolved cell, needs help from V3Param
        तार्किक m_unresolvedClass;  // Unresolved class reference, needs help from V3Param
        तार्किक m_unresolvedGenericIface;  // Unresolved generic interface, needs help from V3Param
        तार्किक m_genBlk;  // Contains gen block reference
        Ast_गाठ* m_unlinkedScopep;  // Unresolved scope, needs corresponding VarXRef
        AstDisable* m_disablep;  // Disable statement under which the reference is
        तार्किक m_dotErr;  // Error found in dotted resolution, ignore upwards
        string m_dotText;  // String of dotted names found in below parseref
        DotStates() { init(nullptr); }
        ~DotStates() = पूर्वनिर्धारित;
        शून्य init(VSymEnt* curSymp, तार्किक unresolvedClass = असत्य) {
            m_dotPos = DP_NONE;
            m_dotSymp = curSymp;
            m_dotp = nullptr;
            m_super = असत्य;
            m_dotErr = असत्य;
            m_dotText = "";
            m_unresolvedCell = असत्य;
            m_unresolvedClass = unresolvedClass;
            m_unresolvedGenericIface = असत्य;
            m_genBlk = असत्य;
            m_unlinkedScopep = nullptr;
            m_disablep = nullptr;
        }
        string ascii() स्थिरांक {
            स्थिर स्थिरांक अक्षर* स्थिरांक names[]
                = {"DP_NONE", "DP_PACKAGE", "DP_FIRST", "DP_SCOPE", "DP_FINAL", "DP_MEMBER"};
            std::ostringstream sstr;
            sstr << "ds=" << names[m_dotPos];
            sstr << "  dse" << cvtToHex(m_dotSymp);
            स्थिरांक string dsname = m_dotSymp->nodep()->name().substr(0, 8);
            sstr << "(" << m_dotSymp->nodep()->typeName() << (dsname.empty() ? "" : ":") << dsname
                 << ")";
            जर (m_dotErr) sstr << "  [dotErr]";
            जर (m_super) sstr << "  [super]";
            जर (m_unresolvedCell) sstr << "  [unrCell]";
            जर (m_unresolvedClass) sstr << "  [unrClass]";
            जर (m_unresolvedGenericIface) sstr << "  [unrGIface]";
            जर (m_genBlk) sstr << "  [genBlk]";
            sstr << "  txt=" << m_dotText;
            परत sstr.str();
        }
    } m_ds;  // State to preserve across recursions

    // METHODS - Variables
    AstVar* createImplicitVar(VSymEnt* /*lookupSymp*/, AstParseRef* nodep, Ast_गाठ_Module* modp,
                              VSymEnt* moduleSymp, तार्किक noWarn) {
        // Create implicit after warning
        जर (!noWarn) {
            जर (nodep->fileline()->warnIsOff(V3ErrorCode::I_DEF_NETTYPE_WIRE)) {
                स्थिरांक string suggest = m_statep->suggestSymFallback(moduleSymp, nodep->name(),
                                                                    Link_गाठ_Matcher_Var{});
                nodep->v3error("Signal definition not found, and implicit disabled with "
                               "`default_nettype: "
                               << nodep->prettyNameQ() << '\n'
                               << (suggest.empty() ? "" : nodep->warnMore() + suggest));

            }
            // Bypass looking for suggestions if IMPLICIT is turned off
            // as there could be thousands of these suppressed in large netlists
            अन्यथा जर (!nodep->fileline()->warnIsOff(V3ErrorCode::IMPLICIT)) {
                स्थिरांक string suggest = m_statep->suggestSymFallback(moduleSymp, nodep->name(),
                                                                    Link_गाठ_Matcher_Var{});
                nodep->v3warn(IMPLICIT,
                              "Signal definition not found, creating implicitly: "
                                  << nodep->prettyNameQ() << '\n'
                                  << (suggest.empty() ? "" : nodep->warnMore() + suggest));
            }
        }
        AstVar* स्थिरांक newp
            = नवीन AstVar{nodep->fileline(), VVarType::WIRE, nodep->name(), VFlagLogicPacked{}, 1};
        newp->lifetime(VLifetime::STATIC_IMPLICIT);
        newp->trace(modp->modTrace());
        modp->addStmtsp(newp);
        // Link it to signal list, must add the variable under the module;
        // current scope might be lower now
        m_statep->घाला_Sym(moduleSymp, newp->name(), newp, nullptr /*classOrPackagep*/);
        परत newp;
    }
    AstVar* foundToVarp(स्थिरांक VSymEnt* symp, Ast_गाठ* nodep, VAccess access) {
        // Return a variable if possible, auto converting a modport to variable
        जर (!symp) {
            परत nullptr;
        } अन्यथा जर (VN_IS(symp->nodep(), Var)) {
            परत VN_AS(symp->nodep(), Var);
        } अन्यथा जर (VN_IS(symp->nodep(), ModportVarRef)) {
            AstModportVarRef* स्थिरांक snodep = VN_AS(symp->nodep(), ModportVarRef);
            AstVar* स्थिरांक varp = snodep->varp();
            जर (access.isWriteOrRW() && snodep->direction().isReadOnly()) {
                nodep->v3error("Attempt to drive input-only modport: " << nodep->prettyNameQ());
            }  // else other simulators don't warn about reading, and IEEE doesn't say illegal
            परत varp;
        } अन्यथा {
            परत nullptr;
        }
    }

    // Look up a virtual port through modport expression mapping.
    // When a dotted reference uses a modport with expression syntax, the virtual port name
    // maps to the real signal. This helper resolves that mapping during scope creation.
    // Returns the VarScope for the real signal, or nullptr if not found.
    AstVarScope* findVirtualPortVarScope(VSymEnt* dotSymp, स्थिरांक string& portName) {
        स्थिरांक AstVarScope* स्थिरांक dotVscp = VN_CAST(dotSymp->nodep(), VarScope);
        स्थिरांक AstVar* स्थिरांक dotVarp = dotVscp ? dotVscp->varp() : nullptr;
        जर (!dotVarp) परत nullptr;

        Ast_गाठ_DType* dtypep = dotVarp->childDTypep();
        जर (!dtypep) dtypep = dotVarp->subDTypep();
        जर (!dtypep) परत nullptr;
        स्थिरांक AstIfaceRefDType* स्थिरांक ifaceRefp = VN_CAST(dtypep, IfaceRefDType);
        जर (!ifaceRefp || !ifaceRefp->modportp() || !ifaceRefp->ifaceViaCellp()) परत nullptr;

        // Get the interface cell's symbol table
        VSymEnt* स्थिरांक ifaceCellSymp = m_statep->get_गाठ_Sym(ifaceRefp->ifaceViaCellp());

        // Look up modport by name in interface cell
        VSymEnt* स्थिरांक modportSymp = ifaceCellSymp->findIdFallback(ifaceRefp->modportp()->name());
        जर (!modportSymp) परत nullptr;

        // Look up virtual port name in modport symbol table
        string baddot;
        VSymEnt* स्थिरांक virtPortp = m_statep->findSymPrefixed(modportSymp, portName, baddot, सत्य);
        जर (!virtPortp) परत nullptr;

        // Symbol may point to VarScope or Var
        जर (AstVarScope* vscp = VN_CAST(virtPortp->nodep(), VarScope)) परत vscp;

        // Symbol points to Var - look up VarScope by name in interface cell
        जर (स्थिरांक AstVar* स्थिरांक realVarp = VN_CAST(virtPortp->nodep(), Var)) {
            VSymEnt* स्थिरांक realFoundp
                = m_statep->findSymPrefixed(ifaceCellSymp, realVarp->name(), baddot, सत्य);
            परत realFoundp ? VN_CAST(realFoundp->nodep(), VarScope) : nullptr;
        }
        परत nullptr;
    }

    // Capture a ParamTypeDType reference for interface typedef retargeting.
    // Called when a RefDType resolves to a ParamTypeDType owned by an interface.
    शून्य captureIfaceParamType(AstRefDType* nodep, AstParamTypeDType* defp) {
        जर (!V3LinkDotIfaceCapture::enabled() || !m_statep->forPrimary()) परत;
        Ast_गाठ_Module* स्थिरांक defOwnerModp = V3LinkDotIfaceCapture::findOwnerModule(defp);
        जर (!defOwnerModp || !VN_IS(defOwnerModp, Iface)) परत;
        AstCell* स्थिरांक cellForCapture
            = m_ds.m_dotSymp ? VN_CAST(m_ds.m_dotSymp->nodep(), Cell) : nullptr;
        जर (!cellForCapture) परत;
        UINFO(9, indent() << "iface capture add paramtype " << nodep
                          << " iface=" << defOwnerModp->prettyNameQ());
        V3LinkDotIfaceCapture::addParamType(nodep, cellForCapture->name(), m_modp, defp,
                                            defOwnerModp->name());
    }

    Ast_गाठ_Stmt* addImplicitSuperNewCall(AstFunc* स्थिरांक nodep,
                                         स्थिरांक AstClassExtends* स्थिरांक classExtendsp) {
        // Returns the added node
        File_ओळ* स्थिरांक fl = nodep->fileline();
        AstArg* argsp = nullptr;
        जर (classExtendsp->argsp()) argsp = classExtendsp->argsp()->clone_वृक्ष(सत्य);
        AstNew* स्थिरांक newExprp = नवीन AstNew{fl, argsp};
        newExprp->isImplicit(सत्य);
        AstDot* स्थिरांक superNewp = नवीन AstDot{fl, असत्य, नवीन AstParseRef{fl, "super"}, newExprp};
        Ast_गाठ_Stmt* स्थिरांक superNewStmtp = superNewp->makeStmt();
        साठी (Ast_गाठ* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            // super.new shall be the first statement (IEEE 1800-2023 8.15)
            // but some nodes (such as variable declarations and typedefs) should stay before
            जर (VN_IS(stmtp, गाठ_Stmt)) {
                stmtp->addHereThisAsNext(superNewStmtp);
                परत superNewStmtp;
            }
        }
        // There were no statements
        nodep->addStmtsp(superNewStmtp);
        परत superNewStmtp;
    }
    शून्य checkNoDot(Ast_गाठ* nodep) {
        जर (VL_UNLIKELY(!nodep)) {
            UINFO(9, indent() << "iface capture null node passed to checkNoDot; dot state="
                              << m_ds.ascii());
            m_ds.m_dotErr = सत्य;
            परत;
        }
        जर (VL_UNLIKELY(m_ds.m_dotPos != DP_NONE)) {
            UINFO(9, indent() << "ds=" << m_ds.ascii());
            UINFO(9, indent() << "iface capture checkNoDot hit node=" << nodep
                              << " dotState=" << m_ds.ascii());
            जर (VL_UNLIKELY(!m_ds.m_dotp)) {
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
            m_ds.m_dotErr = सत्य;
        }
    }
    AstVar* find_Iface_शीर्ष_Varp(Ast_गाठ* nodep, VSymEnt* parentEntp, स्थिरांक string& name) {
        स्थिरांक string findName = name + "__Viftop";
        VSymEnt* स्थिरांक ifaceSymp = parentEntp->findIdFallback(findName);
        AstVar* स्थिरांक iface_शीर्ष_Varp = ifaceSymp ? VN_AS(ifaceSymp->nodep(), Var) : nullptr;
        UASSERT_OBJ(iface_शीर्ष_Varp, nodep, "Can't find interface var ref: " << findName);
        परत iface_शीर्ष_Varp;
    }
    AstRSProd* findProd(Ast_गाठ* nodep, VSymEnt* parentEntp, स्थिरांक string& name) {
        स्थिरांक VSymEnt* स्थिरांक foundp = parentEntp->findIdFallback(name);
        AstRSProd* foundNodep = foundp ? VN_CAST(foundp->nodep(), RSProd) : nullptr;
        जर (!foundNodep) {
            VSpellCheck speller;
            Link_गाठ_Matcher_Prod matcher;
            parentEntp->candidateIdFlat(&speller, &matcher, असत्य);
            स्थिरांक string suggest = speller.best_Candidate_संदेश(name);
            UINFO(1, "   ErrParseRef curSymp=se" << cvtToHex(parentEntp));
            nodep->v3error("Production " << Ast_गाठ::prettyNameQ(name) << " not found\n"
                                         << (suggest.empty() ? "" : nodep->warnMore() + suggest));
            परत nullptr;
        }
        परत foundNodep;
    }
    शून्य duplicatePinError(AstPin* nodep, Ast_गाठ* origp, स्थिरांक अक्षर* whatp) {
        nodep->v3error("Duplicate " << whatp << " connection: " << nodep->prettyNameQ() << '\n'
                                    << nodep->warnContextPrimary() << '\n'
                                    << origp->warnOther() << "... Location of original " << whatp
                                    << " connection\n"
                                    << origp->warnContextSecondary());
    }
    शून्य markAndCheckPinDup(AstPin* nodep, Ast_गाठ* refp, स्थिरांक अक्षर* whatp) {
        स्थिरांक स्वतः pair = m_usedPins.emplace(refp, nodep);
        जर (pair.second) {
            जर (!nodep->paramPath().empty())
                m_usedDefParamPins[refp].emplace(nodep->paramPath(), nodep);
            परत;
        } अन्यथा {
            Ast_गाठ* स्थिरांक origp = pair.first->second;
            जर (nodep->paramPath().empty() || VN_AS(origp, Pin)->paramPath().empty()) {
                duplicatePinError(nodep, origp, whatp);
                परत;
            }
        }

        स्वतः& defParamPins = m_usedDefParamPins[refp];
        स्थिरांक स्वतः defParamIt = defParamPins.find(nodep->paramPath());
        जर (defParamIt != defParamPins.end()) {
            duplicatePinError(nodep, defParamIt->second, whatp);
            परत;
        }
        defParamPins.emplace(nodep->paramPath(), nodep);
    }
    स्थिर तार्किक extendsClass(स्थिरांक AstClass* classp, स्थिरांक AstClass* targetp) {
        // Return true if classp inherits from targetp through already resolved extends
        // If performance of this becomes a problem, use just a counter to check
        // exceeds some number of iterations then perform the more expensive analysis
        std::set<स्थिरांक AstClass*> visited;
        std::vector<स्थिरांक AstClass*> todo{classp};
        पर्यंत (!todo.empty()) {
            स्थिरांक AstClass* स्थिरांक currp = todo.back();
            todo.काढा_back();
            जर (currp == targetp) परत सत्य;
            जर (!visited.घाला(currp).second) सुरूठेवा;
            साठी (स्थिरांक AstClassExtends* cextp = currp->extendsp(); cextp;
                 cextp = VN_AS(cextp->nextp(), ClassExtends)) {
                जर (स्थिरांक AstClass* स्थिरांक basep = cextp->classOrNullp()) todo.ढकला_back(basep);
            }
        }
        परत असत्य;
    }
    स्थिर AstClocking* sensClockingp(Ast_गाठ* nodep) {
        // Return the clocking block referenced by nodep, either directly or through a modport
        जर (AstClocking* स्थिरांक clockingp = VN_CAST(nodep, Clocking)) परत clockingp;
        जर (स्थिरांक AstModportClockingRef* स्थिरांक clockingRefp = VN_CAST(nodep, ModportClockingRef))
            परत clockingRefp->clockingp();
        परत nullptr;
    }
    VSymEnt* getCreateClockingEventSymEnt(AstClocking* clockingp) {
        AstVar* स्थिरांक eventp = clockingp->ensureEventp(सत्य);
        जर (!eventp->user1p()) eventp->user1p(नवीन VSymEnt{m_statep->symsp(), eventp});
        परत reinterpret_cast<VSymEnt*>(eventp->user1p());
    }
    VSymEnt* findInlinedDotsSym(Ast_गाठ* nodep, स्थिरांक string& inlinedDots) {
        // Return scope that applies for searching, given renaming due to module inlining.
        // Dotted lookup is always relative to module, as maybe variable
        // name lower down with same scope name we want to ignore (t_math_divw)
        VSymEnt* dotSymp = m_modSymp;
        स्थिरांक string inl = Ast_गाठ::dedotName(inlinedDots);
        string baddot;
        VSymEnt* okSymp = nullptr;
        dotSymp = m_statep->findDotted(nodep->fileline(), dotSymp, inl, baddot, okSymp, असत्य);
        UASSERT_OBJ(dotSymp, nodep,
                    "Couldn't resolve inlined scope " << Ast_गाठ::prettyNameQ(baddot)
                                                      << " in: " << inlinedDots);
        UINFO(9, indent() << "findInlinedDotsSym " << dotSymp
                          << " search start due to inlinedDots=" << inlinedDots);
        परत dotSymp;
    }
    VSymEnt* findDottedRefSymp(Ast_गाठ* nodep, VSymEnt* lookupSymp, स्थिरांक string& dotted,
                               string& baddot, VSymEnt*& okSymp, तार्किक* modportp = nullptr) {
        // Return scope to search for a relinked hierarchical reference
        // (AstVarXRef or AstEnumItemRef), or nullptr after error if not found
        // If modportp is given, set true when the reference went through a modport
        जर (modportp) *modportp = असत्य;
        VSymEnt* dotSymp = m_statep->findDotted(nodep->fileline(), lookupSymp, dotted, baddot,
                                                okSymp, सत्य);  // Maybe nullptr
        जर (!dotSymp) {
            nodep->v3error(
                "Can't find definition of "
                << (!baddot.empty() ? Ast_गाठ::prettyNameQ(baddot) : nodep->prettyNameQ()) << '\n'
                << nodep->warnContextPrimary());
            परत nullptr;
        }
        जर (स्थिरांक AstVar* स्थिरांक varp = VN_CAST(dotSymp->nodep(), Var)) {
            जर (स्थिरांक AstIfaceRefDType* स्थिरांक ifaceRefp
                = VN_CAST(varp->childDTypep(), IfaceRefDType)) {
                जर (ifaceRefp->modportp()) {
                    dotSymp = m_statep->get_गाठ_Sym(ifaceRefp->modportp());
                    जर (modportp) *modportp = सत्य;
                } अन्यथा {
                    dotSymp = m_statep->get_गाठ_Sym(ifaceRefp->ifacep());
                }
            }
        } अन्यथा जर (स्थिरांक AstModportClockingRef* स्थिरांक clockingRefp
                   = VN_CAST(dotSymp->nodep(), ModportClockingRef)) {
            dotSymp = m_statep->get_गाठ_Sym(clockingRefp->clockingp());
        }
        परत dotSymp;
    }

    स्थिर तार्किक isParamedClassRefDType(स्थिरांक Ast_गाठ* classp) {
        पर्यंत (स्थिरांक AstRefDType* स्थिरांक refp = VN_CAST(classp, RefDType))
            classp = refp->subDTypep();
        परत (VN_IS(classp, ClassRefDType) && VN_AS(classp, ClassRefDType)->paramsp())
               || VN_IS(classp, ParamTypeDType);
    }
    तार्किक isParamedClassRef(स्थिरांक Ast_गाठ* nodep) {
        // Is this a parameterized reference to a class, or a reference to class parameter
        जर (स्थिरांक स्वतः* classRefp = VN_CAST(nodep, ClassOrPackageRef)) {
            जर (classRefp->paramsp()) परत सत्य;
            स्थिरांक स्वतः* classp = classRefp->classOrPackageNodep();
            पर्यंत (स्थिरांक स्वतः* typedefp = VN_CAST(classp, Typedef)) classp = typedefp->subDTypep();
            परत isParamedClassRefDType(classp);
        }
        परत isParamedClassRefDType(nodep);
    }
    VSymEnt* getThisClassSymp() {
        VSymEnt* classSymp = m_ds.m_dotSymp;
        पर्यंत (classSymp && !VN_IS(classSymp->nodep(), Class)) {
            classSymp = classSymp->parentp();
        }
        परत classSymp;
    }
    // Is the current scope within the given class, including within its nested classes
    तार्किक insideClass(स्थिरांक AstClass* classp) स्थिरांक {
        साठी (स्थिरांक VSymEnt* symp = m_curSymp; symp; symp = symp->parentp()) {
            जर (symp->nodep() == classp) परत सत्य;
        }
        परत असत्य;
    }
    // 'randomize() with' target class not yet specialized; defer to post-V3Param pass
    तार्किक randTargetUnresolved() स्थिरांक {
        परत m_statep->forPrimary() && m_currentWithp && m_randMethodCallp && !m_randSymp;
    }
    // Is 'funcp' of 'classp' virtual, directly or by overriding a virtual method of a base
    // class (IEEE 1800-2023 8.20)
    तार्किक isVirtualMethod(स्थिरांक Ast_गाठ_FTask* funcp, स्थिरांक AstClass* classp) स्थिरांक {
        जर (funcp->isVirtual()) परत सत्य;
        साठी (स्थिरांक AstClassExtends* cextp = classp->extendsp(); cextp;
             cextp = VN_AS(cextp->nextp(), ClassExtends)) {
            AstClass* स्थिरांक basep = cextp->classOrNullp();
            जर (!basep) सुरूठेवा;
            स्थिरांक VSymEnt* स्थिरांक foundp = m_statep->get_गाठ_Sym(basep)->findIdFlat(funcp->name());
            स्थिरांक Ast_गाठ_FTask* स्थिरांक baseFuncp
                = foundp ? VN_CAST(foundp->nodep(), गाठ_FTask) : nullptr;
            स्थिरांक AstClass* स्थिरांक baseClassp = foundp && foundp->parentp()
                                                   ? VN_CAST(foundp->parentp()->nodep(), Class)
                                                   : nullptr;
            जर (baseFuncp && baseClassp && isVirtualMethod(baseFuncp, baseClassp)) परत सत्य;
        }
        परत असत्य;
    }
    // Is imported 'implp' an implementation of interface method 'ifSymp' (IEEE 1800-2023
    // 8.26.2)? If its class does not derive from the interface class, it needs a forwarding
    // override in the derived class to be reachable through the interface.
    तार्किक inheritedImplements(स्थिरांक VSymEnt* implp, स्थिरांक VSymEnt* ifSymp, तार्किक& needForwardr) {
        needForwardr = असत्य;
        स्थिरांक Ast_गाठ_FTask* स्थिरांक implFuncp = VN_CAST(implp->nodep(), गाठ_FTask);
        जर (!implFuncp || implFuncp->pureVirtual() || implFuncp->isStatic()) परत असत्य;
        स्थिरांक AstClass* स्थिरांक implClassp
            = implp->parentp() ? VN_CAST(implp->parentp()->nodep(), Class) : nullptr;
        स्थिरांक AstClass* स्थिरांक ifClassp
            = ifSymp->parentp() ? VN_CAST(ifSymp->parentp()->nodep(), Class) : nullptr;
        जर (!implClassp || !ifClassp) परत असत्य;
        जर (extendsClass(implClassp, ifClassp)) परत सत्य;
        जर (!isVirtualMethod(implFuncp, implClassp)) परत असत्य;
        needForwardr = सत्य;
        परत सत्य;
    }
    // Add 'implp' override to 'classp' that calls super.implp, so the C++ method of an
    // implemented interface class is overridden
    शून्य addInterfaceForward(AstClass* classp, Ast_गाठ_FTask* implp, AstClass* implClassp) {
        File_ओळ* स्थिरांक flp = implp->fileline();
        AstVar* fvarp = nullptr;
        Ast_गाठ_FTask* newp;
        जर (AstFunc* स्थिरांक implFuncp = VN_CAST(implp, Func)) {
            fvarp = VN_AS(implFuncp->fvarp(), Var)->clone_वृक्ष(असत्य);
            newp = नवीन AstFunc{flp, implp->name(), nullptr, fvarp};
        } अन्यथा {
            newp = नवीन AstTask{flp, implp->name(), nullptr};
        }
        newp->classMethod(सत्य);
        newp->isVirtual(सत्य);
        newp->lifetime(implp->lifetime());
        AstArg* argsp = nullptr;
        साठी (Ast_गाठ* stmtp = implp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstVar* स्थिरांक portp = VN_CAST(stmtp, Var);
            जर (!portp || !portp->isIO()) सुरूठेवा;
            AstVar* स्थिरांक newPortp = portp->clone_वृक्ष(असत्य);
            newp->addStmtsp(newPortp);
            argsp = Ast_गाठ::addNext(
                argsp, नवीन AstArg{flp, "", नवीन AstVarRef{flp, newPortp, VAccess::READ}});
        }
        Ast_गाठ_FTask_Ref* callp;
        जर (AstFunc* स्थिरांक implFuncp = VN_CAST(implp, Func)) {
            callp = नवीन AstFuncRef{flp, implFuncp, argsp};
        } अन्यथा {
            callp = नवीन AstTaskRef{flp, VN_AS(implp, Task), argsp};
        }
        callp->classOrPackagep(implClassp);
        callp->superReference(सत्य);
        जर (fvarp) {
            newp->addStmtsp(नवीन AstAssign{flp, नवीन AstVarRef{flp, fvarp, VAccess::WRITE}, callp});
        } अन्यथा {
            newp->addStmtsp(callp->makeStmt());
        }
        UINFO(9, indent() << "Interface forward " << newp);
        classp->addMembersp(newp);
    }
    शून्य importDerivedClass(AstClass* derivedClassp, VSymEnt* baseSymp, AstClass* baseClassp) {
        // Also used for standard 'extends' from a base class
        UINFO(8, indent() << "importDerivedClass to " << derivedClassp << " from " << baseClassp);
        साठी (VSymEnt::const_iterator it = baseSymp->begin(); it != baseSymp->end(); ++it) {
            जर (Ast_गाठ* baseSubp = it->second->nodep()) {
                UINFO(8, indent() << "  SymFunc " << baseSubp);
                स्थिरांक string impOrExtends
                    = baseClassp->isInterfaceClass() ? " implements " : " extends ";
                जर (VN_IS(baseSubp, गाठ_FTask)) {
                    स्थिरांक VSymEnt* स्थिरांक foundp = m_curSymp->findIdFlat(baseSubp->name());
                    स्थिरांक Ast_गाठ_FTask* स्थिरांक baseFuncp = VN_CAST(baseSubp, गाठ_FTask);
                    जर (!baseFuncp || !baseFuncp->pureVirtual()) सुरूठेवा;
                    तार्किक needForward = असत्य;
                    स्थिरांक तार्किक existsInDerived
                        = foundp
                          && (!foundp->imported()
                              || inheritedImplements(foundp, it->second, needForward));
                    जर (needForward && m_statep->forPrimary()
                        && !derivedClassp->isInterfaceClass()) {
                        m_ifForwards.emplace(baseSubp->name(), foundp);
                    }
                    जर (m_statep->forPrimary() && !existsInDerived
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
                    स्थिरांक स्वतः itn = m_ifClassImpNames.find(baseSubp->name());
                    जर (m_statep->forPrimary() && !existsInDerived
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
                जर (VN_IS(baseSubp, Constraint)) {
                    स्थिरांक VSymEnt* स्थिरांक foundp = m_curSymp->findIdFlat(baseSubp->name());
                    स्थिरांक AstConstraint* स्थिरांक baseFuncp = VN_CAST(baseSubp, Constraint);
                    जर (!baseFuncp || !baseFuncp->isKwdPure()) सुरूठेवा;
                    स्थिरांक तार्किक existsInDerived = foundp && !foundp->imported();
                    जर (m_statep->forPrimary() && !existsInDerived
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
    शून्य importSymbolsFromExtended(AstClass* स्थिरांक nodep, AstClassExtends* स्थिरांक cextp) {
        AstClass* स्थिरांक baseClassp = cextp->classp();
        VSymEnt* स्थिरांक srcp = m_statep->get_गाठ_Sym(baseClassp);
        importDerivedClass(nodep, srcp, baseClassp);
        जर (!cextp->isImplements()) m_curSymp->importFromClass(m_statep->symsp(), srcp);
    }
    शून्य classExtendImport(AstClass* nodep) {
        // A class reference might be to a class that is later in Ast due to
        // e.g. parameterization or referring to a "class (type T) extends T"
        // Resolve it so later Class:: references into its base classes work
        symIterateNull(nodep, m_statep->get_गाठ_Sym(nodep));
    }

    शून्य checkDeclOrder(Ast_गाठ* nodep, Ast_गाठ* declp) {
        स्थिरांक uint32_t decl_Token_अंक = declp->decl_Token_अंक();
        जर (!decl_Token_अंक) परत;  // Not implemented/valid on this object
        जर (nodep->fileline()->token_अंक() < decl_Token_अंक) {
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
    शून्य checkMemberDeclOrder(Ast_गाठ* nodep, AstMemberDType* declp) {
        स्थिरांक uint32_t decl_Token_अंक = declp->fileline()->token_अंक();
        जर (nodep->fileline()->token_अंक() < decl_Token_अंक) {
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

    शून्य replaceWithCheckBreak(Ast_गाठ* oldp, Ast_गाठ_DType* newp) {
        // Flag now to avoid V3Broken throwing an internal error
        जर (oldp->wouldBreak(newp)) {
            newp->v3error(
                "Data type used where a non-data type is expected: " << newp->prettyNameQ());
            oldp->replaceWith(नवीन AstConst{newp->fileline(), AstConst::BitFalseErroring{}});
        } अन्यथा {
            oldp->replaceWith(newp);
        }
    }

    // Marks the current module to be revisited after the initial AST iteration
    शून्य revisitLater(Ast_गाठ* deferredNodep) {
        // Need to revisit entire module to build up all the necessary context
        m_lastDeferredp = deferredNodep;
        m_modulesToRevisit.घाला(std::make_pair(m_modp->name(), m_modp));
    }

    शून्य updateVarUse(AstVar* nodep) {
        // Avoid dotted.PARAM false positive when in a parameter block
        // that is if ()'ed off by same dotted name as another block
        जर (nodep && nodep->isParam()) nodep->usedParam(सत्य);
    }

    स्थिर VSymEnt* findIdFallbackSkipMemberDType(VSymEnt* lookp, स्थिरांक string& name) {
        VSymEnt* shadowEntp = nullptr;  // Shadowing variable: not a type, kept for error report
        पर्यंत (lookp) {
            VSymEnt* स्थिरांक foundp = lookp->findIdFlat(name);
            जर (foundp && !VN_IS(foundp->nodep(), MemberDType)) {
                // Non-type entries are not type candidates
                // (IEEE 1800-2023 6.18); skip them so an enclosing
                // type is found, but keep one to preserve the "found: ..." error.
                जर (VN_IS(foundp->nodep(), Typedef) || VN_IS(foundp->nodep(), ParamTypeDType)
                    || VN_IS(foundp->nodep(), Class)) {
                    परत foundp;
                }
                जर (!shadowEntp) shadowEntp = foundp;
            }
            lookp = lookp->fallbackp();
        }
        परत shadowEntp;
    }

    शून्य symIterateChildren(Ast_गाठ* nodep, VSymEnt* symp) {
        // Iterate children, changing to given context, with restore to old context
        VL_RESTORER_COPY(m_ds);
        VL_RESTORER(m_curSymp);
        m_curSymp = symp;
        m_ds.init(m_curSymp);
        iterateChildren(nodep);
    }
    शून्य symIterateNull(Ast_गाठ* nodep, VSymEnt* symp) {
        // Iterate node, changing to given context, with restore to old context
        VL_RESTORER_COPY(m_ds);
        VL_RESTORER(m_curSymp);
        m_curSymp = symp;
        m_ds.init(m_curSymp);
        iterateNull(nodep);
    }
    स्थिर स्थिरांक Ast_गाठ_DType* get_अभिव्यक्ती_DTypep(स्थिरांक Ast_गाठ_अभिव्यक्ती* selp) {
        पर्यंत (स्थिरांक Ast_गाठ_Pre_Sel* स्थिरांक sp = VN_CAST(selp, गाठ_Pre_Sel)) selp = sp->fromp();
        जर (स्थिरांक AstMemberSel* स्थिरांक sp = VN_CAST(selp, MemberSel)) {
            जर (स्थिरांक Ast_गाठ_DType* dtypep = get_अभिव्यक्ती_DTypep(sp->fromp())) {
                जर (स्थिरांक AstClassRefDType* स्थिरांक classRefp = VN_CAST(dtypep, ClassRefDType)) {
                    स्थिरांक AstClass* स्थिरांक classp = classRefp->classp();
                    स्थिरांक तार्किक found = classp->existsMember(
                        [&dtypep, name = selp->name()](स्थिरांक AstClass*, स्थिरांक AstVar* nodep) {
                            dtypep = nodep->childDTypep();
                            परत nodep->name() == name;
                        });
                    जर (found) परत dtypep->elemDTypep();
                    selp->v3error("Class " << classRefp->prettyNameQ()
                                           << " does not contain field " << selp->prettyNameQ());
                } अन्यथा {
                    selp->v3fatalSrc("Member selection on expression of type "
                                     << dtypep->prettyDTypeNameQ()
                                     << ", which is not a class type");
                }
            }
        } अन्यथा जर (स्थिरांक Ast_गाठ_Var_Ref* स्थिरांक varRefp = VN_CAST(selp, गाठ_Var_Ref)) {
            परत varRefp->varp()->childDTypep()->elemDTypep();
        }
        परत nullptr;
    }
    स्थिर स्थिरांक AstVar* getNextVarp(स्थिरांक Ast_गाठ* stmtsp) {
        // Only IO ports, as parameters are on paramsp(), not the pinsp() list paired here
        पर्यंत (stmtsp) {
            जर (स्थिरांक AstVar* स्थिरांक varp = VN_CAST(stmtsp, Var)) {
                जर (varp->isIO()) परत varp;
            }
            stmtsp = stmtsp->nextp();
        }
        परत nullptr;
    }
    // Introduce implicit parameters for modules with generic interafeces
    शून्य addImplicitParametersOfGenericIface(AstCell* स्थिरांक nodep, स्थिरांक AstModule* स्थिरांक modp) {
        // Get Param number
        पूर्णांक param_अंक = 1;
        साठी (AstPin* paramp = nodep->paramsp(); paramp; paramp = VN_CAST(paramp->nextp(), Pin)) {
            ++param_अंक;
        }

        // Add each implicit parameter type
        स्थिरांक AstVar* modIfaceVarp = getNextVarp(modp->stmtsp());
        साठी (स्थिरांक AstPin* pinp = nodep->pinsp(); pinp && modIfaceVarp;
             pinp = VN_CAST(pinp->nextp(), Pin),
                           modIfaceVarp = getNextVarp(modIfaceVarp->nextp())) {
            जर (modIfaceVarp->varType() != VVarType::IFACEREF
                || !VN_IS(modIfaceVarp->childDTypep()->elemDTypep(), IfaceGenericDType)) {
                सुरूठेवा;
            }
            Ast_गाठ* exprp = pinp->exprp();
            जर (!exprp) {
                modIfaceVarp->v3error("Interface port "
                                      << modIfaceVarp->prettyNameQ()
                                      << " is not connected to interface/modport pin expression");
                सुरूठेवा;
            }
            पर्यंत (स्थिरांक Ast_गाठ_Pre_Sel* स्थिरांक preSelp = VN_CAST(exprp, गाठ_Pre_Sel)) {
                exprp = preSelp->fromp();
            }
            // Resolve pin expression to the enclosing module's port Var. At primary
            // LinkDot the expression may still be an AstParseRef, so also look up by name.
            AstVar* enclosingVarp = nullptr;
            स्थिरांक AstVarRef* स्थिरांक varRefp = VN_CAST(exprp, VarRef);
            जर (varRefp) {
                enclosingVarp = varRefp->varp();
            } अन्यथा जर (स्थिरांक AstParseRef* स्थिरांक parseRefp = VN_CAST(exprp, ParseRef)) {
                जर (m_modp) {
                    जर (VSymEnt* स्थिरांक symp
                        = m_statep->get_गाठ_Sym(m_modp)->findIdFlat(parseRefp->name())) {
                        enclosingVarp = VN_CAST(symp->nodep(), Var);
                    }
                }
            }
            जर (enclosingVarp && enclosingVarp->varType() == VVarType::IFACEREF
                && VN_IS(enclosingVarp->childDTypep()->skipRefp(), IfaceGenericDType)) {
                // Nested generic-iface forwarding (#7454): enclosing port is itself still
                // generic, so emit a placeholder __VGIfaceParam pin carrying a VarRef to
                // the outer port. V3Param rewrites it to the concrete IfaceRefDType once
                // the enclosing module is specialized (see
                // ParamProcessor::resolveGenericIfaceForwardingPins for the ordering
                // constraint that keeps the rewrite inside V3Param).
                AstVarRef* स्थिरांक fwdRefp
                    = नवीन AstVarRef{exprp->fileline(), enclosingVarp, VAccess::READ};
                AstPin* स्थिरांक newPinp = नवीन AstPin{
                    pinp->fileline(), param_अंक, "__VGIfaceParam" + modIfaceVarp->name(), fwdRefp};
                newPinp->param(सत्य);
                visit(newPinp);
                nodep->addParamsp(newPinp);
            } अन्यथा जर (varRefp) {
                स्थिरांक AstVar* स्थिरांक varp = varRefp->varp();
                जर (स्थिरांक AstIfaceRefDType* स्थिरांक refp
                    = VN_CAST(varp->childDTypep()->elemDTypep(), IfaceRefDType)) {
                    AstIface* स्थिरांक ifacep = VN_AS(refp->cellp()->modp(), Iface);
                    AstIfaceRefDType* newIfaceRefp;
                    जर (refp->modportp()) {
                        newIfaceRefp = नवीन AstIfaceRefDType{
                            refp->fileline(), refp->modportFileline(), m_modp->name(),
                            ifacep->name(), refp->modportName()};
                    } अन्यथा {
                        newIfaceRefp = नवीन AstIfaceRefDType{refp->fileline(), m_modp->name(),
                                                            ifacep->name()};
                    }
                    newIfaceRefp->ifacep(ifacep);
                    जर (refp->cellp()->paramsp()) {
                        newIfaceRefp->addParamsp(refp->cellp()->paramsp()->clone_वृक्ष(सत्य));
                    }
                    UASSERT_OBJ(pinp->name().find("__pinNumber") == 0
                                    || pinp->name() == modIfaceVarp->name(),
                                pinp, "Not found interface with such name");
                    AstPin* स्थिरांक newPinp
                        = नवीन AstPin{pinp->fileline(), param_अंक,
                                     "__VGIfaceParam" + modIfaceVarp->name(), newIfaceRefp};
                    newPinp->param(सत्य);
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
    तार्किक errorHierNonPort(AstVarXRef* refp, AstVar* varp, VSymEnt* scopeSymp) {
        // OK to access ports on hier_block
        जर (varp->isIO()) परत असत्य;
        // OK if not an instance
        स्थिरांक AstCell* स्थिरांक cellp = VN_CAST(scopeSymp->nodep(), Cell);
        जर (!cellp) परत असत्य;
        // Ok if not a hier_block
        स्थिरांक Ast_गाठ_Module* स्थिरांक modp = cellp->modp();
        जर (!modp->hierBlock()) परत असत्य;
        // Bad
        refp->v3error("Cannot access non-port symbols inside hierarchical block");
        परत सत्य;
    }

#define LINKDOT_VISIT_START() \
    VL_RESTORER(m_indent); \
    ++m_indent;

    string indent() स्थिरांक {
        string result = "";
        result.घाला(0, m_indent, ':');
        परत result + " ";
    }

    // VISITORS
    शून्य visit(AstNetlist* nodep) override {
        // Recurse..., backward as must do packages before using packages
        iterateChildrenBackwardsConst(nodep);
    }
    शून्य visit(AstTypeTable*) override {}
    शून्य visit(Ast_गाठ_Module* nodep) override {
        जर (nodep->dead()) परत;
        LINKDOT_VISIT_START();
        UINFO(8, indent() << "visit " << nodep);
        VL_RESTORER(m_genericIfaceModule);
        जर (स्थिरांक AstModule* स्थिरांक modp = VN_CAST(nodep, Module)) {
            m_genericIfaceModule = modp->hasGenericIface();
        }
        checkNoDot(nodep);
        m_ds.init(m_curSymp);
        m_ds.m_dotSymp = m_curSymp = m_modSymp
            = m_statep->get_गाठ_Sym(nodep);  // Until overridden by a SCOPE
        m_cellp = nullptr;
        m_modp = nodep;
        m_modport_अंक = 0;
        iterateChildren(nodep);
        m_modp = nullptr;
        m_ds.m_dotSymp = m_curSymp = m_modSymp = nullptr;
        m_ds.m_dotPos = DP_NONE;
        m_ds.m_dotErr = असत्य;
    }
    शून्य visit(AstScope* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(8, indent() << "visit " << nodep);
        checkNoDot(nodep);
        VL_RESTORER(m_modSymp);
        VL_RESTORER(m_curSymp);
        m_ds.m_dotSymp = m_curSymp = m_modSymp = m_statep->getScopeSym(nodep);
        iterateChildren(nodep);
        m_ds.m_dotSymp = m_curSymp = m_modSymp = nullptr;
    }
    शून्य visit(AstCellInline* nodep) override {
        LINKDOT_VISIT_START();
        checkNoDot(nodep);
        UASSERT_OBJ(m_statep->forScopeCreation(), nodep, "CellInline should not exist yet");
        VL_DO_DANGLING(ढकला_Deletep(nodep->unlinkFrBack()), nodep);
    }
    शून्य visit(AstCell* nodep) override {
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
            जर (VN_IS(nodep->modp(), NotFoundModule)) {
                // Prevent warnings about missing pin connects
                जर (nodep->pinsp()) nodep->pinsp()->unlinkFrBackWithNext()->delete_वृक्ष();
                जर (nodep->paramsp()) nodep->paramsp()->unlinkFrBackWithNext()->delete_वृक्ष();
            }
            // Need to pass the module info to this cell, so we can link up the pin names
            // However can't use m_curSymp as pin connections need to use the
            // instantiator's symbols
            अन्यथा {
                m_pinSymp = m_statep->get_गाठ_Sym(nodep->modp());
                UINFO(4, indent() << "(Backto) visit " << nodep);
                // UINFOTREE(1, nodep, "", "linkcell");
                // UINFOTREE(1, nodep->modp(), "", "linkcemd");
                iterateChildren(nodep);

                जर (m_statep->forPrimary())
                    जर (स्थिरांक AstModule* स्थिरांक modp = VN_CAST(nodep->modp(), Module)) {
                        जर (modp->hasGenericIface())
                            addImplicitParametersOfGenericIface(nodep, modp);
                    }
            }
        }
        // Parent module inherits child's publicity
        // This is done bottom up in the LinkBotupVisitor stage
    }
    शून्य visit(AstClassRefDType* nodep) override {
        // Cell: Recurse inside or cleanup not founds
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        // Can be under dot if called as package::class and that class resolves, so no checkNoDot
        VL_RESTORER_CLEAR(m_usedPins);
        VL_RESTORER_CLEAR(m_usedDefParamPins);
        UASSERT_OBJ(nodep->classp(), nodep, "ClassRef has unlinked class");
        UASSERT_OBJ(m_statep->forPrimary() || !nodep->paramsp() || V3Error::error_मोजणी(), nodep,
                    "class reference parameter not removed by V3Param");
        VL_RESTORER(m_pinSymp);
        {
            // ClassRef's have pins, so track
            m_pinSymp = m_statep->get_गाठ_Sym(nodep->classp());
            UINFO(4, indent() << "(Backto) visit " << nodep);
            // UINFOTREE(1, nodep, "", "linkcell");
            // UINFOTREE(1, nodep->modp(), "", "linkcemd");
            iterateChildren(nodep);
        }
    }
    शून्य visit(AstPin* nodep) override {
        // Pin: Link to submodule's port
        LINKDOT_VISIT_START();
        checkNoDot(nodep);
        iterateChildren(nodep);
        जर (!nodep->modVarp()) {
            UASSERT_OBJ(m_pinSymp, nodep, "Pin not under instance?");
            VSymEnt* स्थिरांक foundp = m_pinSymp->findIdFlat(nodep->name());
            स्थिरांक अक्षर* स्थिरांक whatp = nodep->param() ? "parameter" : "pin";
            जर (!foundp) {
                जर (m_cellp && VN_IS(m_cellp->modp(), Primitive)
                    && (nodep->name() == "__paramNumber1" || nodep->name() == "__paramNumber2")) {
                    // Primitive parameter is really a delay2 we can just ignore
                    VL_DO_DANGLING(ढकला_Deletep(nodep->unlinkFrBack()), nodep);
                    परत;
                } अन्यथा जर (nodep->param() && !nodep->exprp() && !nodep->svDotName()) {
                    // Placeholder pin the parser makes for an empty '#()', to signal
                    // the user did type the '#()'. No further use, deleting.
                    VL_DO_DANGLING(ढकला_Deletep(nodep->unlinkFrBack()), nodep);
                    परत;
                } अन्यथा {
                    स्थिरांक std::string suggest
                        = (nodep->param()
                               ? m_statep->suggestSymFlat(m_pinSymp, nodep->name(),
                                                          Link_गाठ_Matcher_Var_Param{}, असत्य)
                               : m_statep->suggestSymFlat(m_pinSymp, nodep->name(),
                                                          Link_गाठ_Matcher_Var_IO{}, असत्य));
                    स्थिरांक std::string decl
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
                    VL_DO_DANGLING(ढकला_Deletep(nodep->unlinkFrBack()), nodep);
                    परत;
                }
            }
            VVarType refVarType = VVarType::UNKNOWN;
            तार्किक wrongPinType = असत्य;
            जर (AstVar* स्थिरांक varp = VN_CAST(foundp->nodep(), Var)) {
                जर (varp->isIO() || varp->isParam() || varp->isIfaceRef()) {
                    refVarType = varp->varType();
                    nodep->modVarp(varp);
                } अन्यथा {
                    wrongPinType = सत्य;
                }
            } अन्यथा जर (AstParamTypeDType* स्थिरांक typep = VN_CAST(foundp->nodep(), ParamTypeDType)) {
                refVarType = typep->varType();
                nodep->modPTypep(typep);
            } अन्यथा {
                wrongPinType = सत्य;
            }
            // Don't connect parameter pin to module ports or vice versa
            जर (nodep->param() != (refVarType == VVarType::GPARAM)) wrongPinType = सत्य;
            जर (wrongPinType) {
                string targetType = LinkDotState::गाठ_Text_Type(foundp->nodep());
                targetType = VString::aOrAn(targetType) + ' ' + targetType;
                जर (nodep->param()) {
                    nodep->v3error("Instance attempts to override "
                                   << nodep->prettyNameQ() << " as a " << whatp << ", but it is "
                                   << targetType);
                } अन्यथा {
                    nodep->v3error("Instance attempts to connect to "
                                   << nodep->prettyNameQ() << ", but it is " << targetType);
                }
                परत;
            }
            markAndCheckPinDup(nodep, foundp->nodep(), whatp);
        }
        // Early return() above when deleted
    }

    स्थिर तार्किक isDotThisReference(स्थिरांक AstDot* nodep) {
        परत VN_IS(nodep->lhsp(), ParseRef) && nodep->lhsp()->name() == "this";
    }

    शून्य visit(AstDot* nodep) override {
        // Legal under a DOT: AstDot, AstParseRef, AstPackageRef, AstNodeSel, AstPackArrayDType
        //    also a DOT can be part of an expression, but only above plus
        //    AstFTaskRef are legal children
        // Dot(PackageRef, ParseRef(text))
        // Dot(Dot(ClassOrPackageRef,ClassOrPackageRef), ParseRef(text))
        // Dot(Dot(Dot(ParseRef(text), ...
        जर (nodep->user3SetOnce()) परत;
        LINKDOT_VISIT_START();
        UINFO(8, indent() << "visit " << nodep);
        UINFO(8, indent() << m_ds.ascii());
        स्थिरांक DotStates lastStates = m_ds;
        स्थिरांक तार्किक start = (m_ds.m_dotPos == DP_NONE);  // Save, as m_dotp will be changed
        स्थिरांक DotPosition initialDotPos = m_ds.m_dotPos;
        VL_RESTORER(m_randSymp);
        {
            जर (start) {  // Starting dot sequence
                UINFOTREE(9, nodep, "", "dot-in");
                // Keep deferral from visit(AstMethodCall) for unspecialized rand target
                m_ds.init(m_curSymp, m_ds.m_unresolvedClass && randTargetUnresolved());
            }
            m_ds.m_dotp = nodep;  // Always, not just at start
            m_ds.m_dotPos = DP_FIRST;

            जर (isDotThisReference(nodep)) {
                VSymEnt* classSymp = getThisClassSymp();
                // In 'randomize() with { this.member }', 'this' refers to randomized
                // object, not the calling class (IEEE 1800-2023 18.7)
                जर (m_randSymp && m_currentWithp) classSymp = m_randSymp;
                जर (!classSymp && randTargetUnresolved()) {
                    // Bind 'this' in post-V3Param pass
                } अन्यथा जर (!classSymp) {
                    nodep->v3error("'this' used outside class (IEEE 1800-2023 8.11)");
                    m_ds.m_dotErr = सत्य;
                } अन्यथा {
                    m_ds.m_dotSymp = classSymp;
                    UINFO(8, indent() << "this. " << m_ds.ascii());
                }
            } अन्यथा जर (VN_IS(nodep->lhsp(), ParseRef) && nodep->lhsp()->name() == "super") {
                स्थिरांक VSymEnt* classSymp = getThisClassSymp();
                जर (!classSymp) {
                    nodep->v3error("'super' used outside class (IEEE 1800-2023 8.15)");
                    m_ds.m_dotErr = सत्य;
                } अन्यथा {
                    स्थिरांक स्वतः classp = VN_AS(classSymp->nodep(), Class);
                    स्थिरांक स्वतः cextp = classp->extendsp();
                    जर (!cextp) {
                        nodep->v3error("'super' used on non-extended class (IEEE 1800-2023 8.15)");
                        m_ds.m_dotErr = सत्य;
                    } अन्यथा {
                        जर (m_statep->forPrimary()
                            && m_extendsParam.find(classp) != m_extendsParam.end()) {
                            UINFO(9, indent()
                                         << "deferring until post-V3Param: " << nodep->lhsp());
                            m_ds.m_unresolvedClass = सत्य;
                        } अन्यथा {
                            स्थिरांक स्वतः baseClassp = cextp->classp();
                            UASSERT_OBJ(baseClassp, nodep, "Bad superclass");
                            m_ds.m_dotSymp = m_statep->get_गाठ_Sym(baseClassp);
                            m_ds.m_super = सत्य;
                            UINFO(8, indent() << "super. " << m_ds.ascii());
                        }
                    }
                }
            } अन्यथा जर (AstClassOrPackageRef* स्थिरांक lhsp
                       = VN_CAST(nodep->lhsp(), ClassOrPackageRef)) {
                // m_ds.m_dotText communicates the cell prefix between stages
                UINFO(8, indent() << "iter.lhs   " << m_ds.ascii() << " " << nodep);
                iterateAndNextNull(lhsp);
                जर (!lhsp->classOrPackageSkipp() && lhsp->name() != "local::") {
                    revisitLater(nodep);
                    m_ds = lastStates;
                    // Resolve function args before bailing
                    जर (Ast_गाठ_FTask_Ref* स्थिरांक ftaskrefp = VN_CAST(nodep->rhsp(), गाठ_FTask_Ref)) {
                        iterateAndNextNull(ftaskrefp->argsp());
                    }
                    परत;
                }
                m_ds.m_dotPos = DP_PACKAGE;
                // nodep->lhsp() may be a new node
                जर (AstClassOrPackageRef* स्थिरांक classOrPackageRefp
                    = VN_CAST(nodep->lhsp(), ClassOrPackageRef)) {
                    जर (Ast_गाठ* स्थिरांक classOrPackageNodep
                        = classOrPackageRefp->classOrPackageSkipp()) {
                        m_ds.m_dotSymp = m_statep->get_गाठ_Sym(classOrPackageNodep);
                    }
                }
                UINFO(8, indent() << "iter.ldone " << m_ds.ascii() << " " << nodep);
            } अन्यथा जर (VN_IS(nodep->lhsp(), Dot) && VN_AS(nodep->lhsp(), Dot)->colon()) {
                // m_ds.m_dotText communicates the cell prefix between stages
                UINFO(8, indent() << "iter.lhs   " << m_ds.ascii() << " " << nodep);
                m_ds.m_dotPos = DP_PACKAGE;
                iterateAndNextNull(nodep->lhsp());
                // nodep->lhsp() may be a new node
                जर (AstClassOrPackageRef* स्थिरांक crefp
                    = VN_CAST(nodep->lhsp(), ClassOrPackageRef)) {
                    जर (!crefp->classOrPackageSkipp()) {
                        revisitLater(nodep);
                        m_ds = lastStates;
                        // Resolve function args before bailing
                        जर (Ast_गाठ_FTask_Ref* स्थिरांक ftaskrefp
                            = VN_CAST(nodep->rhsp(), गाठ_FTask_Ref)) {
                            iterateAndNextNull(ftaskrefp->argsp());
                        }
                        परत;
                    }
                }
                जर (m_lastDeferredp == nodep->lhsp()) {
                    // LHS got deferred, so this node won't be resolved. Defer it too
                    m_ds = lastStates;
                    // Resolve function args before bailing
                    जर (Ast_गाठ_FTask_Ref* स्थिरांक ftaskrefp = VN_CAST(nodep->rhsp(), गाठ_FTask_Ref)) {
                        iterateAndNextNull(ftaskrefp->argsp());
                    }
                    परत;
                }
                UINFO(8, indent() << "iter.ldone " << m_ds.ascii() << " " << nodep);
            } अन्यथा {
                m_ds.m_dotPos = DP_FIRST;
                m_ds.m_unresolvedGenericIface = असत्य;
                UINFO(8, indent() << "iter.lhs   " << m_ds.ascii() << " " << nodep);
                iterateAndNextNull(nodep->lhsp());
                UINFO(8, indent() << "iter.ldone " << m_ds.ascii() << " " << nodep);
                जर (m_ds.m_unresolvedGenericIface) {
                    m_ds.m_dotPos = initialDotPos;
                    परत;
                }
                // UINFOTREE(9, nodep, "", "dot-lho");
            }
            जर (m_statep->forPrimary() && isParamedClassRef(nodep->lhsp())) {
                // Dots of paramed classes will be linked after deparameterization
                UINFO(9, indent() << "deferring until post-V3Param: " << nodep->lhsp());
                m_ds.m_unresolvedClass = सत्य;
            }
            जर (m_ds.m_unresolvedCell
                && (VN_IS(nodep->lhsp(), CellRef) || VN_IS(nodep->lhsp(), CellArrayRef))) {
                m_ds.m_unlinkedScopep = nodep->lhsp();
            }
            जर (m_ds.m_dotPos == DP_FIRST) m_ds.m_dotPos = DP_SCOPE;
            जर (!m_ds.m_dotErr) {  // Once something wrong, give up
                // Top 'final' dot RHS is final RHS, else it's a
                // DOT(DOT(x,*here*),real-rhs) which we consider a RHS
                जर (start && m_ds.m_dotPos == DP_SCOPE) m_ds.m_dotPos = DP_FINAL;
                UINFO(8, indent() << "iter.rhs   " << m_ds.ascii() << " " << nodep);
                // m_ds.m_dotSymp points at lhsp()'s symbol table, so resolve RHS under that
                iterateAndNextNull(nodep->rhsp());
                UINFO(8, indent() << "iter.rdone " << m_ds.ascii() << " " << nodep);
                // UINFOTREE(9, nodep, "", "dot-rho");
            }
            जर (!m_ds.m_unresolvedClass) {
                जर (start) {
                    Ast_गाठ* newp;
                    जर (m_ds.m_dotErr) {
                        newp = नवीन AstConst{nodep->fileline(), AstConst::BitFalse{}};
                    } अन्यथा {
                        newp = nodep->rhsp()->unlinkFrBack();
                    }
                    UINFOTREE(9, newp, "", "dot-out");
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
                } अन्यथा {  // Dot midpoint
                    Ast_गाठ* newp = nodep->rhsp()->unlinkFrBack();
                    जर (m_ds.m_unresolvedCell) {
                        AstCellRef* स्थिरांक crp = नवीन AstCellRef{
                            nodep->fileline(), nodep->name(), nodep->lhsp()->unlinkFrBack(), newp};
                        newp = crp;
                    }
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
                }
            }
        }
        जर (start) {
            m_ds = lastStates;
        } अन्यथा {
            स्थिरांक तार्किक unresolvedClass = m_ds.m_unresolvedClass;
            m_ds.m_dotp = lastStates.m_dotp;
            m_ds.m_unresolvedClass |= unresolvedClass;
        }
        UINFO(8, indent() << "done " << m_ds.ascii() << " " << nodep);
    }
    शून्य visit(AstDefaultClocking* nodep) override {
        जर (VSymEnt* स्थिरांक foundp = m_curSymp->findIdFallback(nodep->name())) {
            जर (AstClocking* स्थिरांक clockingp = VN_CAST(foundp->nodep(), Clocking)) {
                clockingp->makeDefault();
                VL_DO_DANGLING(nodep->unlinkFrBack()->delete_वृक्ष(), nodep);
            } अन्यथा {
                nodep->v3error(nodep->prettyNameQ() << " is not a clocking identifier");
            }
        } अन्यथा {
            nodep->v3error("Can't find definition of clocking: " << nodep->prettyNameQ());
        }
    }
    शून्य visit(AstSenItem* nodep) override {
        LINKDOT_VISIT_START();
        VL_RESTORER(m_inSens);
        m_inSens = सत्य;
        iterateChildren(nodep);
    }
    शून्य visit(AstPatMember* nodep) override {
        LINKDOT_VISIT_START();
        जर (nodep->varrefp()) परत;  // only do this mapping once
        // If we have a TEXT token as our key, lookup if it's a LPARAM
        जर (AstText* स्थिरांक textp = VN_CAST(nodep->keyp(), Text)) {
            UINFO(9, indent() << "visit " << nodep);
            UINFO(9, indent() << "      " << textp);
            // Lookup
            जर (VSymEnt* स्थिरांक foundp = m_curSymp->findIdFallback(textp->text())) {
                जर (AstVar* स्थिरांक varp = VN_CAST(foundp->nodep(), Var)) {
                    जर (varp->isParam() || varp->isGenVar()) {
                        // Attach found Text reference to PatMember
                        nodep->varrefp(
                            नवीन AstVarRef{nodep->fileline(),
                                          foundp->imported() ? foundp->classOrPackagep() : nullptr,
                                          varp, VAccess::READ});
                        UINFO(9, indent() << " new " << nodep->varrefp());
                    }
                }
                जर (AstEnumItem* स्थिरांक itemp = VN_CAST(foundp->nodep(), EnumItem)) {
                    // Attach enum item value to PatMember
                    nodep->varrefp(
                        नवीन AstEnumItemRef{nodep->fileline(), itemp, foundp->classOrPackagep()});
                    UINFO(9, indent() << " new " << itemp);
                }
            }
        }
        iterateChildren(nodep);
    }
    शून्य visit(AstParseRef* nodep) override {
        जर (nodep->user3SetOnce()) परत;
        LINKDOT_VISIT_START();
        UINFO(9, indent() << "visit " << nodep);
        UINFO(9, indent() << m_ds.ascii());
        जर (m_ds.m_unresolvedClass) परत;
        // m_curSymp is symbol table of outer expression
        // m_ds.m_dotSymp is symbol table relative to "."'s above now
        UASSERT_OBJ(m_ds.m_dotSymp, nodep, "nullptr lookup symbol table");
        // Generally resolved during Primary, but might be at param time under AstUnlinkedRef
        UASSERT_OBJ(!m_statep->forScopeCreation(), nodep, "ParseRefs should no longer exist");
        स्थिरांक DotStates lastStates = m_ds;
        स्थिरांक तार्किक start = (m_ds.m_dotPos == DP_NONE);  // Save, as m_dotp will be changed
        तार्किक first = start || m_ds.m_dotPos == DP_FIRST;

        जर (start) {
            m_ds.init(m_curSymp);
            // Note m_ds.m_dotp remains nullptr; this is a reference not under a dot
        }
        जर (nodep->name() == "super") {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: super");
            m_ds.m_dotErr = सत्य;
        }
        जर (nodep->name() == "this") {
            iterateChildren(nodep);
            जर (m_statep->forPrimary()) परत;  // The class might be parameterized somewhere
            स्थिरांक VSymEnt* classSymp = getThisClassSymp();
            जर (!classSymp) {
                nodep->v3error("'this' used outside class (IEEE 1800-2023 8.11)");
                परत;
            }
            AstClass* स्थिरांक classp = VN_AS(classSymp->nodep(), Class);
            AstClassRefDType* स्थिरांक dtypep
                = नवीन AstClassRefDType{nodep->fileline(), classp, nullptr};
            AstThisRef* स्थिरांक newp = नवीन AstThisRef{nodep->fileline(), VFlagChildDType{}, dtypep};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
            परत;
        }
        जर (m_ds.m_dotPos == DP_MEMBER && VN_IS(m_ds.m_dotp->lhsp(), LambdaArgRef)
            && nodep->name() == "index") {
            // 'with' statement's 'item.index'
            // m_ds.dotp->lhsp() was checked to know if `index` is directly after lambda arg ref.
            // If not, treat it as normal member select
            iterateChildren(nodep);
            AstLambdaArgRef* स्थिरांक newp = नवीन AstLambdaArgRef{
                nodep->fileline(), m_ds.m_dotp->lhsp()->name() + "__DOT__index", सत्य};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
            परत;
        } अन्यथा जर (m_ds.m_dotPos == DP_MEMBER) {
            // Found a Var, everything following is membership.  {scope}.{var}.HERE {member}
            जर (m_ds.m_dotErr) {
                nodep->unlinkFrBack();  // Avoid circular node loop on errors
            } अन्यथा {
                Ast_गाठ_अभिव्यक्ती* स्थिरांक varEtcp = VN_AS(m_ds.m_dotp->lhsp()->unlinkFrBack(), गाठ_अभिव्यक्ती);
                Ast_गाठ_अभिव्यक्ती* स्थिरांक newp = नवीन AstMemberSel{nodep->fileline(), varEtcp,  //
                                                           VFlagChildDType{}, nodep->name()};
                nodep->replaceWith(newp);
            }
            VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
        } अन्यथा {
            //
            string expectWhat;
            तार्किक allowScope = असत्य;
            तार्किक allowVar = असत्य;
            तार्किक allowFTask = असत्य;
            तार्किक staticAccess = असत्य;
            जर (m_ds.m_disablep) {
                allowScope = सत्य;
                allowFTask = सत्य;
                expectWhat = "block/task";
            } अन्यथा जर (m_ds.m_dotPos == DP_PACKAGE) {
                // {package-or-class}::{a}
                Ast_गाठ_Module* classOrPackagep = nullptr;
                expectWhat = "scope/variable/func";
                allowScope = सत्य;
                allowVar = सत्य;
                allowFTask = सत्य;
                staticAccess = सत्य;
                UINFO(9, "chk pkg " << m_ds.ascii() << " lhsp=" << m_ds.m_dotp->lhsp());
                UASSERT_OBJ(VN_IS(m_ds.m_dotp->lhsp(), ClassOrPackageRef), m_ds.m_dotp->lhsp(),
                            "Bad package link");
                AstClassOrPackageRef* स्थिरांक cpackagerefp
                    = VN_AS(m_ds.m_dotp->lhsp(), ClassOrPackageRef);
                जर (cpackagerefp->name() == "local::") {
                    m_randSymp = nullptr;
                    first = सत्य;
                } अन्यथा जर (!cpackagerefp->classOrPackageSkipp()) {
                    VSymEnt* स्थिरांक foundp = m_statep->resolveClassOrPackage(
                        m_ds.m_dotSymp, cpackagerefp, सत्य, असत्य, ":: reference");
                    जर (!foundp) परत;
                    classOrPackagep = cpackagerefp->classOrPackageSkipp();
                    जर (classOrPackagep) {
                        m_ds.m_dotSymp = m_statep->get_गाठ_Sym(classOrPackagep);
                    } अन्यथा {
                        m_ds = lastStates;
                        परत;
                    }
                } अन्यथा {
                    classOrPackagep = cpackagerefp->classOrPackageSkipp();
                    UASSERT_OBJ(classOrPackagep, m_ds.m_dotp->lhsp(), "Bad package link");
                    m_ds.m_dotSymp = m_statep->get_गाठ_Sym(classOrPackagep);
                }
                m_ds.m_dotPos = DP_SCOPE;
            } अन्यथा जर (m_ds.m_dotPos == DP_SCOPE || m_ds.m_dotPos == DP_FIRST) {
                // {a}.{b}, where {a} maybe a module name
                // or variable, where dotting into structure member
                expectWhat = "scope/variable";
                allowScope = सत्य;
                allowVar = सत्य;
            } अन्यथा जर (m_ds.m_dotPos == DP_NONE) {
                expectWhat = "variable";
                allowVar = सत्य;
            } अन्यथा जर (m_ds.m_dotPos == DP_FINAL) {
                expectWhat = "variable/method";
                allowFTask = सत्य;
                allowVar = सत्य;
            } अन्यथा {
                UINFO(1, "ds=" << m_ds.ascii());
                nodep->v3fatalSrc("Unhandled DotPosition");
            }
            // Lookup
            VSymEnt* foundp;
            string baddot;
            VSymEnt* okSymp = nullptr;
            // Restricted 'with (id_list)': only id_list names bind into target class.
            जर (m_randSymp
                && (!m_currentWithp || m_currentWithp->nameResolvesToTarget(nodep->name()))) {
                foundp = m_randSymp->findIdFlat(nodep->name());
                जर (foundp) {
                    जर (m_currentWithp && m_currentWithp->restricted()) {
                        m_restrictedNamesUsed.घाला(nodep->name());
                    }
                    जर (!start) m_ds.m_dotPos = DP_MEMBER;
                    जर (!m_currentWithp) {
                        UASSERT_OBJ(m_randMethodCallp, nodep, "Expected to be under randomize()");
                        // This will start failing once complex expressions are allowed on the LHS
                        // of randomize() with args
                        UASSERT_OBJ(VN_IS(m_randMethodCallp->fromp(), VarRef), m_randMethodCallp,
                                    "Expected simple randomize target");
                        // A ParseRef is used here so that the dot RHS gets resolved
                        AstMemberSel* स्थिरांक newp = नवीन AstMemberSel{
                            nodep->fileline(), m_randMethodCallp->fromp()->clone_वृक्ष(असत्य),
                            VFlagChildDType{}, nodep->name()};
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
                        परत;
                    }
                    UINFO(9, indent() << "randomize-with fromSym " << foundp->nodep());
                    AstLambdaArgRef* स्थिरांक lambdaRefp
                        = नवीन AstLambdaArgRef{nodep->fileline(), "__Vrandwith_obj", असत्य};
                    AstMemberSel* newp = नवीन AstMemberSel{nodep->fileline(), lambdaRefp,
                                                          VFlagChildDType{}, nodep->name()};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
                    परत;
                }
            }
            जर (allowScope) {
                foundp = m_statep->findDotted(nodep->fileline(), m_ds.m_dotSymp, nodep->name(),
                                              baddot, okSymp, first);  // Maybe nullptr
            } अन्यथा जर (first) {
                foundp = m_ds.m_dotSymp->findIdFallback(nodep->name());
            } अन्यथा {
                foundp = m_ds.m_dotSymp->findIdFlat(nodep->name());
            }
            जर (!foundp && m_ds.m_dotp && isDotThisReference(m_ds.m_dotp)) {
                स्थिरांक AstClass* स्थिरांक classp = VN_CAST(m_ds.m_dotSymp->nodep(), Class);
                जर (classp && classp->isCovergroup() && classp->covergroupEnclosingClassp()) {
                    VSymEnt* स्थिरांक enclosingClassSymp
                        = m_statep->get_गाठ_Sym(classp->covergroupEnclosingClassp());
                    foundp = enclosingClassSymp->findIdFallback(nodep->name());
                    जर (foundp) m_ds.m_dotSymp = enclosingClassSymp;
                }
            }
            // If not found in modport, check interface fallback for parameters and typedefs.
            // Parameters and typedefs are always visible through a modport (IEEE 1800-2023 25.5).
            // This mirrors the VarXRef modport parameter fallback in visit(AstVarXRef).
            जर (!foundp && VN_IS(m_ds.m_dotSymp->nodep(), Modport)
                && m_ds.m_dotSymp->fallbackp()) {
                VSymEnt* स्थिरांक ifaceFallbackp = m_ds.m_dotSymp->fallbackp();
                VSymEnt* स्थिरांक ifaceFoundp = ifaceFallbackp->findIdFlat(nodep->name());
                जर (ifaceFoundp) {
                    जर (स्थिरांक AstVar* स्थिरांक varp = VN_CAST(ifaceFoundp->nodep(), Var)) {
                        जर (varp->isParam()) foundp = ifaceFoundp;
                    } अन्यथा जर (VN_IS(ifaceFoundp->nodep(), Typedef)
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
            जर (!foundp && !okSymp && VN_IS(m_ds.m_dotSymp->nodep(), Modport)) {
                okSymp = m_ds.m_dotSymp;
            }
            जर (foundp) {
                UINFO(9, indent() << "found=se" << cvtToHex(foundp) << "  exp=" << expectWhat
                                  << "  n=" << foundp->nodep());
            }
            // What fell out?
            तार्किक ok = असत्य;
            // Special case: waiting on clocking event
            जर (m_inSens && foundp && m_ds.m_dotPos != DP_SCOPE && m_ds.m_dotPos != DP_FIRST) {
                जर (AstClocking* स्थिरांक clockingp = sensClockingp(foundp->nodep())) {
                    foundp = getCreateClockingEventSymEnt(clockingp);
                }
            }
            जर (!foundp) {
            } अन्यथा जर (VN_IS(foundp->nodep(), Cell) || VN_IS(foundp->nodep(), गाठ_Block)
                       || VN_IS(foundp->nodep(), GenBlock)
                       || VN_IS(foundp->nodep(), Netlist)  // for $root
                       || VN_IS(foundp->nodep(), Module)) {  // if top
                जर (allowScope) {
                    ok = सत्य;
                    m_ds.m_dotText = VString::dot(m_ds.m_dotText, ".", nodep->name());
                    m_ds.m_dotSymp = foundp;
                    m_ds.m_dotPos = DP_SCOPE;
                    जर (m_ds.m_disablep && VN_IS(foundp->nodep(), गाठ_Block)) {
                        // Possibly it is not the final link. If we are under dot and not in its
                        // last component, `targetp()` field will be overwritten by next components
                        m_ds.m_disablep->targetp(foundp->nodep());
                    }
                    जर (VN_IS(foundp->nodep(), GenBlock)) {
                        m_ds.m_genBlk = सत्य;
                        जर (m_ds.m_disablep) {
                            m_ds.m_disablep->v3warn(
                                E_UNSUPPORTED,
                                "Unsupported: Generate block referenced by disable");
                        }
                    }
                    // Upper AstDot visitor will handle it from here
                } अन्यथा जर (VN_IS(foundp->nodep(), Cell) && allowVar) {
                    AstCell* स्थिरांक cellp = VN_AS(foundp->nodep(), Cell);
                    जर (VN_IS(cellp->modp(), Iface)) {
                        // Interfaces can be referenced like a variable for interconnect
                        VSymEnt* स्थिरांक cellEntp = m_statep->get_गाठ_Sym(cellp);
                        UASSERT_OBJ(cellEntp, nodep, "No interface sym entry");
                        VSymEnt* स्थिरांक parentEntp
                            = cellEntp->parentp();  // Container of the var; probably a module or
                                                    // generate begin
                        AstVar* स्थिरांक ifaceRefVarp
                            = find_Iface_शीर्ष_Varp(nodep, parentEntp, nodep->name());
                        //
                        ok = सत्य;
                        m_ds.m_dotSymp = foundp;
                        m_ds.m_dotPos = DP_SCOPE;
                        UINFO(9, indent() << " cell -> iface varref " << foundp->nodep());
                        Ast_गाठ_अभिव्यक्ती* newp;
                        जर (m_ds.m_dotText != "") {
                            newp = नवीन AstVarXRef{nodep->fileline(), ifaceRefVarp, m_ds.m_dotText,
                                                  VAccess::READ};
                            जर (m_ds.m_unresolvedCell && m_ds.m_unlinkedScopep) {
                                newp = नवीन AstUnlinkedRef{nodep->fileline(), newp, newp->name(),
                                                          m_ds.m_unlinkedScopep->unlinkFrBack()};
                                m_ds.m_unlinkedScopep = nullptr;
                                m_ds.m_unresolvedCell = असत्य;
                                m_ds.m_dotText = "";
                            }
                        } अन्यथा {
                            AstVarRef* स्थिरांक refp
                                = नवीन AstVarRef{nodep->fileline(), ifaceRefVarp, VAccess::READ};
                            newp = refp;
                        }
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
                    } अन्यथा जर (VN_IS(cellp->modp(), NotFoundModule)) {
                        cellp->modNameFileline()->v3error("Cannot find file containing interface: "
                                                          << cellp->modp()->prettyNameQ());
                    }
                }
            } अन्यथा जर (allowScope && !allowFTask && VN_IS(foundp->nodep(), गाठ_FTask)) {
                // Function/task used as scope for static variable access (IEEE std 1800-2017 6.21)
                // Don't set m_dotText so the static variable resolves as a VarRef rather than
                // VarXRef. This is so that V3Begin won't move it out of the function/task's scope.
                ok = सत्य;
                m_ds.m_dotSymp = foundp;
                m_ds.m_dotPos = DP_SCOPE;
            } अन्यथा जर (allowFTask && VN_IS(foundp->nodep(), गाठ_FTask)) {
                Ast_गाठ_FTask_Ref* taskrefp;
                जर (VN_IS(foundp->nodep(), Task)) {
                    taskrefp = नवीन AstTaskRef{nodep->fileline(), nodep->name()};
                } अन्यथा {
                    taskrefp = नवीन AstFuncRef{nodep->fileline(), nodep->name()};
                }
                nodep->replaceWith(taskrefp);
                VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
                m_ds = lastStates;
                परत;
            } अन्यथा जर (AstVar* स्थिरांक varp = foundToVarp(foundp, nodep, VAccess::READ)) {
                // Preserve explicit 'this' references to members inherited from
                // generic classes until after specialization.
                जर (m_statep->forPrimary() && m_ds.m_dotp && isDotThisReference(m_ds.m_dotp)
                    && foundp->classOrPackagep() != m_modp
                    && VN_IS(foundp->classOrPackagep(), Class)
                    && foundp->classOrPackagep()->hasGParam()) {
                    m_ds.m_unresolvedClass = सत्य;
                    परत;
                }
                AstIfaceRefDType* स्थिरांक ifacerefp
                    = LinkDotState::ifaceRefFromArray(varp->subDTypep());
                जर (varp->isIfaceRef() && m_genericIfaceModule
                    && VN_IS(varp->childDTypep(), IfaceGenericDType) && !start) {
                    // Defer only dotted member access ('d.PARAM'), as V3Param must specialize
                    // first; a standalone ref ('.x(d)') resolves via allowVar below now
                    ok = सत्य;
                    m_ds.m_unresolvedGenericIface = सत्य;
                } अन्यथा जर (ifacerefp && varp->isIfaceRef()) {
                    UASSERT_OBJ(ifacerefp->ifaceViaCellp(), ifacerefp, "Unlinked interface");
                    // Really this is a scope reference into an interface
                    UINFO(9, indent() << "varref-ifaceref " << m_ds.m_dotText << "  " << nodep);
                    m_ds.m_dotText = VString::dot(m_ds.m_dotText, ".", nodep->name());
                    // If modport specified, use modport symbol table for lookup of virtual ports
                    जर (ifacerefp->modportp() && m_statep->exists_गाठ_Sym(ifacerefp->modportp())) {
                        m_ds.m_dotSymp = m_statep->get_गाठ_Sym(ifacerefp->modportp());
                    } अन्यथा {
                        m_ds.m_dotSymp = m_statep->get_गाठ_Sym(ifacerefp->ifaceViaCellp());
                    }
                    m_ds.m_dotPos = DP_SCOPE;
                    ok = सत्य;

                    AstVarRef* स्थिरांक refp = नवीन AstVarRef{nodep->fileline(), varp, VAccess::READ};
                    nodep->replaceWith(refp);

                    VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
                } अन्यथा जर (allowVar) {
                    Ast_गाठ* newp;
                    जर (m_ds.m_dotText != "") {
                        AstVarXRef* स्थिरांक refp
                            = नवीन AstVarXRef{nodep->fileline(), nodep->name(), m_ds.m_dotText,
                                             VAccess::READ};  // lvalue'ness computed later
                        refp->varp(varp);
                        // Not linked again after V3LinkLValue, so that reports writing it
                        जर (स्थिरांक AstModportVarRef* स्थिरांक mvarp
                            = VN_CAST(foundp->nodep(), ModportVarRef)) {
                            जर (m_statep->forParamed() && mvarp->direction().isReadOnly()) {
                                refp->readOnlyModport(सत्य);
                            }
                        }
                        refp->containsGenBlock(m_ds.m_genBlk);
                        m_ds.m_dotText = "";
                        जर (m_ds.m_unresolvedCell && m_ds.m_unlinkedScopep) {
                            स्थिरांक string dotted = refp->dotted();
                            स्थिरांक माप_प्रकार pos = dotted.find("__BRA__??__KET__");
                            // Arrays of interfaces all have the same parameters
                            जर (pos != string::npos && varp->isParam()
                                && VN_IS(m_ds.m_unlinkedScopep, CellArrayRef)) {
                                refp->dotted(dotted.substr(0, pos));
                                newp = refp;
                            } अन्यथा {
                                UINFO(9, indent() << "deferring until post-V3Param: " << refp);
                                newp = नवीन AstUnlinkedRef{nodep->fileline(), refp, refp->name(),
                                                          m_ds.m_unlinkedScopep->unlinkFrBack()};
                                m_ds.m_unlinkedScopep = nullptr;
                                m_ds.m_unresolvedCell = असत्य;
                            }
                        } अन्यथा {
                            newp = refp;
                        }
                    } अन्यथा {
                        जर (staticAccess && !varp->lifetime().isStatic() && !varp->isParam()) {
                            // TODO bug4077
                            // nodep->v3error("Static access to non-static member variable "
                            //                << varp->prettyNameQ() << endl);
                        }
                        AstVarRef* स्थिरांक refp = नवीन AstVarRef{
                            nodep->fileline(), varp, VAccess::READ};  // lvalue'ness computed later
                        refp->classOrPackagep(foundp->classOrPackagep());
                        newp = refp;
                    }
                    UINFO(9, indent() << "new " << newp);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
                    m_ds.m_dotPos = DP_MEMBER;
                    ok = सत्य;
                }
            } अन्यथा जर (स्थिरांक AstModport* स्थिरांक modportp = VN_CAST(foundp->nodep(), Modport)) {
                // A scope reference into an interface's modport (not
                // necessarily at a pin connection)
                UINFO(9, indent() << "cell-ref-to-modport " << m_ds.m_dotText << "  " << nodep);
                UINFO(9, indent() << "unlinked " << m_ds.m_unlinkedScopep);
                UINFO(9,
                      indent() << "dotSymp " << m_ds.m_dotSymp << " " << m_ds.m_dotSymp->nodep());
                // Iface was the previously dotted component
                जर (!m_ds.m_dotSymp || !VN_IS(m_ds.m_dotSymp->nodep(), Cell)
                    || !VN_AS(m_ds.m_dotSymp->nodep(), Cell)->modp()
                    || !VN_IS(VN_AS(m_ds.m_dotSymp->nodep(), Cell)->modp(), Iface)) {
                    nodep->v3error("Modport not referenced as <interface>."
                                   << modportp->prettyNameQ());
                } अन्यथा जर (!VN_AS(m_ds.m_dotSymp->nodep(), Cell)->modp()
                           || !VN_IS(VN_AS(m_ds.m_dotSymp->nodep(), Cell)->modp(), Iface)) {
                    nodep->v3error("Modport not referenced from underneath an interface: "
                                   << modportp->prettyNameQ());
                } अन्यथा {
                    AstCell* स्थिरांक cellp = VN_AS(m_ds.m_dotSymp->nodep(), Cell);
                    UASSERT_OBJ(cellp, nodep, "Modport not referenced from an instance");
                    VSymEnt* स्थिरांक cellEntp = m_statep->get_गाठ_Sym(cellp);
                    UASSERT_OBJ(cellEntp, nodep, "No interface sym entry");
                    VSymEnt* स्थिरांक parentEntp
                        = cellEntp->parentp();  // Container of the var; probably a
                                                // module or generate begin
                    // We drop __BRA__??__KET__ as cells don't have that naming yet
                    AstVar* स्थिरांक ifaceRefVarp
                        = find_Iface_शीर्ष_Varp(nodep, parentEntp, cellp->name());
                    //
                    ok = सत्य;
                    m_ds.m_dotText = VString::dot(m_ds.m_dotText, ".", nodep->name());
                    m_ds.m_dotSymp = foundp;
                    m_ds.m_dotPos = DP_SCOPE;
                    UINFO(9, indent() << "modport -> iface varref " << foundp->nodep());
                    // We lose the modport name here, so we cannot detect mismatched modports.
                    AstVarRef* स्थिरांक refp
                        = नवीन AstVarRef{nodep->fileline(), ifaceRefVarp, VAccess::READ};
                    Ast_गाठ_अभिव्यक्ती* newp = refp;

                    स्वतः* स्थिरांक cellarrayrefp = VN_CAST(m_ds.m_unlinkedScopep, CellArrayRef);
                    जर (cellarrayrefp) {
                        // iface[vec].modport became CellArrayRef(iface, lsb)
                        // Convert back to SelBit(iface, lsb)
                        UINFO(9, indent() << "Array modport to SelBit " << cellarrayrefp);
                        newp = नवीन AstSelBit{cellarrayrefp->fileline(), newp,
                                             cellarrayrefp->selp()->unlinkFrBack()};
                        newp->user3(सत्य);  // Don't process again
                        VL_DO_DANGLING(ढकला_Deletep(cellarrayrefp->unlinkFrBack()), cellarrayrefp);
                        m_ds.m_unlinkedScopep = nullptr;
                    }
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
                }
            } अन्यथा जर (AstConstraint* स्थिरांक defp = VN_CAST(foundp->nodep(), Constraint)) {
                Ast_गाठ* स्थिरांक newp = नवीन AstConstraintRef{nodep->fileline(), nullptr, defp};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
                ok = सत्य;
                m_ds.m_dotPos = DP_MEMBER;
                m_ds.m_dotText = "";
            } अन्यथा जर (AstClass* स्थिरांक defp = VN_CAST(foundp->nodep(), Class)) {
                जर (allowVar) {
                    जर (m_ds.m_dotPos == DP_SCOPE && !staticAccess) {
                        // Continue hierarchical lookup in the class scope.
                        m_ds.m_dotSymp = foundp;
                    } अन्यथा {
                        AstRefDType* स्थिरांक newp
                            = नवीन AstRefDType{nodep->fileline(), nodep->name()};
                        replaceWithCheckBreak(nodep, newp);
                        VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
                    }
                    ok = सत्य;
                    m_ds.m_dotText = "";
                } अन्यथा {
                    (शून्य)defp;  // Prevent unused variable warning
                }
            } अन्यथा जर (AstMemberDType* स्थिरांक defp = VN_CAST(foundp->nodep(), MemberDType)) {
                जर (allowVar) {
                    checkMemberDeclOrder(nodep, defp);
                    AstRefDType* स्थिरांक refp = नवीन AstRefDType{nodep->fileline(), nodep->name()};
                    refp->refDTypep(defp);
                    replaceWithCheckBreak(nodep, refp);
                    VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
                    ok = सत्य;
                    m_ds.m_dotText = "";
                }
            } अन्यथा जर (AstEnumItem* स्थिरांक valuep = VN_CAST(foundp->nodep(), EnumItem)) {
                जर (allowVar) {
                    AstEnumItemRef* स्थिरांक refp
                        = नवीन AstEnumItemRef{nodep->fileline(), valuep, foundp->classOrPackagep()};
                    Ast_गाठ* newp = refp;
                    // Hierarchical reference, relinked after V3Param as the referenced
                    // module may be specialized, similar to AstVarXRef
                    refp->dotted(m_ds.m_dotText);
                    refp->containsGenBlock(m_ds.m_genBlk);
                    जर (m_ds.m_unresolvedCell && m_ds.m_unlinkedScopep) {
                        UINFO(9, indent() << "deferring until post-V3Param: " << refp);
                        newp = नवीन AstUnlinkedRef{nodep->fileline(), refp, refp->name(),
                                                  m_ds.m_unlinkedScopep->unlinkFrBack()};
                        m_ds.m_unlinkedScopep = nullptr;
                        m_ds.m_unresolvedCell = असत्य;
                    }
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
                    ok = सत्य;
                    m_ds.m_dotText = "";
                }
            } अन्यथा जर (स्थिरांक AstLambdaArgRef* स्थिरांक argrefp
                       = VN_CAST(foundp->nodep(), LambdaArgRef)) {
                जर (allowVar) {
                    Ast_गाठ* स्थिरांक newp
                        = नवीन AstLambdaArgRef{nodep->fileline(), argrefp->name(), असत्य};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
                    ok = सत्य;
                    m_ds.m_dotPos = DP_MEMBER;
                    m_ds.m_dotText = "";
                }
            } अन्यथा जर (VN_IS(foundp->nodep(), Clocking)) {
                m_ds.m_dotSymp = foundp;
                जर (m_ds.m_dotText != "") m_ds.m_dotText += "." + nodep->name();
                ok = m_ds.m_dotPos == DP_SCOPE || m_ds.m_dotPos == DP_FIRST;
            } अन्यथा जर (स्थिरांक AstModportClockingRef* स्थिरांक clockingRefp
                       = VN_CAST(foundp->nodep(), ModportClockingRef)) {
                // Clocking block accessed through a modport - redirect to actual clocking
                m_ds.m_dotSymp = m_statep->get_गाठ_Sym(clockingRefp->clockingp());
                जर (m_ds.m_dotText != "") m_ds.m_dotText += "." + nodep->name();
                ok = m_ds.m_dotPos == DP_SCOPE || m_ds.m_dotPos == DP_FIRST;
            } अन्यथा जर (स्थिरांक Ast_गाठ_FTask* स्थिरांक ftaskp = VN_CAST(foundp->nodep(), गाठ_FTask)) {
                जर (!ftaskp->isFunction() || ftaskp->classMethod()) {
                    ok = m_ds.m_dotPos == DP_NONE;
                    जर (ok) {
                        // The condition is true for tasks,
                        // properties and void functions.
                        // In these cases, the parentheses may be skipped.
                        // Also SV class methods can be called without parens
                        AstFuncRef* स्थिरांक funcRefp
                            = नवीन AstFuncRef{nodep->fileline(), nodep->name()};
                        nodep->replaceWith(funcRefp);
                        VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
                    }
                }
            } अन्यथा जर (AstTypedef* स्थिरांक defp = VN_CAST(foundp->nodep(), Typedef)) {
                स्थिरांक तार्किक ifaceFinalSegmentAllowed
                    = (m_ds.m_dotPos == DP_FINAL) && m_ds.m_dotSymp
                      && (VN_IS(m_ds.m_dotSymp->nodep(), Modport)
                          || (VN_IS(m_ds.m_dotSymp->nodep(), Cell)
                              && VN_CAST(m_ds.m_dotSymp->nodep(), Cell)->modp()
                              && VN_IS(VN_CAST(m_ds.m_dotSymp->nodep(), Cell)->modp(), Iface)));
                // Allow interface typedef references even without IfaceCapture
                // The dependency graph handles resolution when IfaceCapture is disabled
                ok = (m_ds.m_dotPos == DP_NONE || m_ds.m_dotPos == DP_SCOPE
                      || ifaceFinalSegmentAllowed);
                जर (ifaceFinalSegmentAllowed) {
                    UINFO(9, indent() << "allow final-segment typedef name="
                                      << nodep->prettyNameQ() << " dotText='" << m_ds.m_dotText
                                      << "' dotSym=" << m_ds.m_dotSymp);
                }
                जर (ok) {
                    AstRefDType* स्थिरांक refp = नवीन AstRefDType{nodep->fileline(), nodep->name()};
                    // Don't check if typedef is to a <type T>::<reference> as might not be
                    // resolved yet
                    जर (m_ds.m_dotPos == DP_NONE) checkDeclOrder(nodep, defp);
                    refp->typedefp(defp);

                    V3LinkDotIfaceCapture::captureTypedefContext(
                        refp, "typedef", static_cast<पूर्णांक>(m_ds.m_dotPos), m_ds.m_dotText,
                        m_ds.m_dotSymp, m_modp, [हे]() { परत indent(); });

                    जर (VN_IS(nodep->backp(), SelExtract)) {
                        m_packedArrayDtp = refp;
                    } अन्यथा {
                        replaceWithCheckBreak(nodep, refp);
                        VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
                    }
                }
            } अन्यथा जर (AstParamTypeDType* स्थिरांक defp = VN_CAST(foundp->nodep(), ParamTypeDType)) {
                ok = (m_ds.m_dotPos == DP_NONE || m_ds.m_dotPos == DP_SCOPE
                      || m_ds.m_dotPos == DP_FINAL);
                जर (ok) {
                    AstRefDType* स्थिरांक refp = नवीन AstRefDType{nodep->fileline(), nodep->name()};
                    refp->refDTypep(defp);

                    V3LinkDotIfaceCapture::captureTypedefContext(
                        refp, "paramtype", static_cast<पूर्णांक>(m_ds.m_dotPos), m_ds.m_dotText,
                        m_ds.m_dotSymp, m_modp, [हे]() { परत indent(); });

                    जर (VN_IS(nodep->backp(), SelExtract)) {
                        m_packedArrayDtp = refp;
                    } अन्यथा {
                        replaceWithCheckBreak(nodep, refp);
                        VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
                    }
                }
            }
            जर (!ok) {
                जर (m_insideClassExtParam) {
                    // Don't throw error if the reference is inside a class that extends a param,
                    // because some members can't be linked in such a case. m_insideClassExtParam
                    // may be true only in the first stage of linking.
                    // Mark that the Dot statement can't be resolved.
                    m_ds.m_unresolvedClass = सत्य;
                    // If the symbol was a scope name, it would be resolved.
                    m_ds.m_dotPos = DP_MEMBER;
                } अन्यथा {
                    // Cells/interfaces can't be implicit
                    स्थिरांक तार्किक checkImplicit
                        = (!m_ds.m_dotp && m_ds.m_dotText == "" && !m_ds.m_disablep && !foundp);
                    स्थिरांक तार्किक err
                        = !(checkImplicit && m_statep->implicitOk(m_modp, nodep->name()));
                    जर (err) {
                        जर (foundp) {
                            nodep->v3error("Found definition of '"
                                           << m_ds.m_dotText << (m_ds.m_dotText == "" ? "" : ".")
                                           << nodep->prettyName() << "'" << " as a "
                                           << foundp->nodep()->typeName() << " but expected a "
                                           << expectWhat);
                        } अन्यथा जर (m_ds.m_dotText == "") {
                            UINFO(1, "   ErrParseRef curSymp=se" << cvtToHex(m_curSymp)
                                                                 << " ds=" << m_ds.ascii());
                            स्थिरांक string suggest = m_statep->suggestSymFallback(
                                m_ds.m_dotSymp, nodep->name(), VNodeMatcher{});
                            nodep->v3error(
                                "Can't find definition of "
                                << expectWhat << ": " << nodep->prettyNameQ() << '\n'
                                << (suggest.empty() ? "" : nodep->warnMore() + suggest));
                        } अन्यथा {
                            nodep->v3error(
                                "Can't find definition of "
                                << (!baddot.empty() ? Ast_गाठ::prettyNameQ(baddot)
                                                    : nodep->prettyNameQ())
                                << " in dotted " << expectWhat << ": '"
                                << m_ds.m_dotText + "." + nodep->prettyName() << "'\n"
                                << nodep->warnContextPrimary()
                                << (okSymp ? okSymp->cellErrorScopes(
                                                 nodep, Ast_गाठ::prettyName(m_ds.m_dotText))
                                           : ""));
                        }
                        m_ds.m_dotErr = सत्य;
                    }
                    जर (checkImplicit) {
                        // Create if implicit, and also if error (so only complain once)
                        // Else if a scope is allowed, making a signal won't help error cascade
                        AstVar* स्थिरांक varp
                            = createImplicitVar(m_curSymp, nodep, m_modp, m_modSymp, err);
                        AstVarRef* स्थिरांक newp
                            = नवीन AstVarRef{nodep->fileline(), varp, VAccess::READ};
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
                    }
                }
            }
        }
        जर (start) m_ds = lastStates;
    }
    शून्य visit(AstClassOrPackageRef* nodep) override {
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

            जर (!nodep->classOrPackageSkipp() && !nodep->classOrPackageNodep()
                && nodep->name() != "local::") {
                स्थिरांक तार्किक deferIfUnresolved = m_statep->forPrimary() && m_insideClassExtParam;
                m_statep->resolveClassOrPackage(m_ds.m_dotSymp, nodep, m_ds.m_dotPos != DP_PACKAGE,
                                                असत्य, ":: reference", deferIfUnresolved);
            }

            // ClassRef's have pins, so track
            जर (nodep->classOrPackageSkipp()) {
                m_pinSymp = m_statep->get_गाठ_Sym(nodep->classOrPackageSkipp());
            } अन्यथा जर (nodep->name() != "local::") {
                परत;
            }

            AstClass* स्थिरांक refClassp = VN_CAST(nodep->classOrPackageSkipp(), Class);
            // Make sure any extends() are properly imported within referenced class
            जर (refClassp && !m_statep->forPrimary()) classExtendImport(refClassp);

            जर (m_statep->forPrimary() && refClassp && !nodep->paramsp()
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
                AstPin* स्थिरांक pinp = नवीन AstPin{nodep->fileline(), 1, "__paramNumber1", nullptr};
                pinp->param(सत्य);
                nodep->addParamsp(pinp);
            }

            m_ds.init(m_curSymp);
            UINFO(4, indent() << "(Backto) Link ClassOrPackageRef: " << nodep);
            iterateChildren(nodep);
        }

        जर (nodep->name() == "local::") {
            जर (!m_randSymp && !m_randMethodCallp) {
                nodep->v3error("Illegal 'local::' outside 'randomize() with'"
                               " (IEEE 1800-2023 18.7.1)");
                m_ds.m_dotErr = सत्य;
                परत;
            }
        }
        जर (m_ds.m_dotPos == DP_PACKAGE && nodep->classOrPackageSkipp()) {
            m_ds.m_dotSymp = m_statep->get_गाठ_Sym(nodep->classOrPackageSkipp());
            UINFO(9, indent() << "set sym " << m_ds.ascii());
        }
    }
    शून्य visit(AstConstraint* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        जर (nodep->isExternDef()) {
            जर (स्थिरांक VSymEnt* स्थिरांक foundp
                = m_curSymp->findIdFallback("extern " + nodep->name())) {
                स्थिरांक AstConstraint* स्थिरांक protop = VN_AS(foundp->nodep(), Constraint);
                // Copy specifiers.
                // External definition cannot have any specifiers, so no value will be overwritten.
                nodep->isStatic(protop->isStatic());
            } अन्यथा {
                nodep->v3error("extern not found that declares " + nodep->prettyNameQ());
            }
        }
        जर (nodep->isExternProto()) {
            जर (!m_curSymp->findIdFallback(nodep->name()) && nodep->isExternExplicit()) {
                nodep->v3warn(PROTOTYPEMIS, "Definition not found for extern prototype "
                                                << nodep->prettyNameQ());
            }
        }
        VL_RESTORER(m_curSymp);
        VL_RESTORER_COPY(m_ds);
        m_ds.m_dotSymp = m_curSymp = m_statep->get_गाठ_Sym(nodep);
        iterateChildren(nodep);
    }
    शून्य visit(AstConstraintRef* nodep) override {
        जर (nodep->user3SetOnce()) परत;
        LINKDOT_VISIT_START();
        UINFO(8, indent() << "visit " << nodep);
        UINFO(8, indent() << m_ds.ascii());
        // No children defined
    }
    शून्य visit(AstVarRef* nodep) override {
        // VarRef: Resolve its reference
        // ParseRefs are used the first pass (forPrimary) so we shouldn't get can't find
        // errors here now that we have a VarRef.
        // No checkNoDot; created and iterated from a parseRef
        LINKDOT_VISIT_START();
        iterateChildren(nodep);
        जर (!nodep->varp()) {
            UINFO(9, indent() << "linkVarRef se" << cvtToHex(m_curSymp) << "  n=" << nodep);
            UASSERT_OBJ(m_curSymp, nodep, "nullptr lookup symbol table");
            VSymEnt* स्थिरांक foundp = m_curSymp->findIdFallback(nodep->name());
            जर (AstVar* स्थिरांक varp
                = foundp ? foundToVarp(foundp, nodep, nodep->access()) : nullptr) {
                nodep->varp(varp);
                updateVarUse(nodep->varp());
                // Generally set by parse, but might be an import
                nodep->classOrPackagep(foundp->classOrPackagep());
            }
            जर (VL_UNCOVERABLE(!nodep->varp())) {
                nodep->v3error("Can't find definition of signal, again: "  // LCOV_EXCL_LINE
                               << nodep->prettyNameQ());
            }
        }
        AstVarScope* vscp = nodep->varScopep();
        जर (vscp && vscp->user2p() != vscp && m_replaceWithAlias) {
            vscp = LinkDotScopeVisitor::getAlias(vscp);
            nodep->varp(vscp->varp());
            nodep->varScopep(vscp);
            updateVarUse(nodep->varp());
            UINFO(7, indent() << "Resolved " << nodep);  // Also prints taskp
        }
    }
    शून्य visit(AstVarXRef* nodep) override {
        // VarRef: Resolve its reference
        // We always link even if varp() is set, because the module we choose may change
        // due to creating new modules, flattening, etc.
        जर (nodep->user3SetOnce()) परत;
        LINKDOT_VISIT_START();
        UINFO(8, indent() << "visit " << nodep);
        // No checkNoDot; created and iterated from a parseRef
        जर (!m_modSymp) {
            // Module that is not in hierarchy.  We'll be dead code eliminating it later.
            UINFO(9, "Dead module for " << nodep);
            nodep->varp(nullptr);
        } अन्यथा {
            VSymEnt* lookupSymp = m_curSymp;  // Start search at current scope
            जर (nodep->inlinedDots() != "") {  // Correct for current post-inlined scope
                lookupSymp = findInlinedDotsSym(nodep, nodep->inlinedDots());
            }
            string baddot;
            VSymEnt* okSymp;
            तार्किक modport;
            VSymEnt* स्थिरांक dotSymp
                = findDottedRefSymp(nodep, lookupSymp, nodep->dotted(), baddot, okSymp, &modport);
            जर (!dotSymp) परत;

            जर (!m_statep->forScopeCreation()) {
                VSymEnt* foundp = nullptr;
                जर (modport) {
                    // This is a copy of findSymPrefixed with few modifications.
                    // This searches without a fallback like findSymPrefixed but if lookup fails it
                    // will check one fallback (fallback of modport shall be interface) and take
                    // only L and G PARAMs since they should be visible from a modport always.
                    string prefix = dotSymp->symPrefix();
                    पर्यंत (!foundp) {
                        foundp = dotSymp->findIdFlat(prefix + nodep->name());
                        जर (foundp) थांबा;
                        UASSERT_OBJ(dotSymp->fallbackp(), nodep, "Modports shall have fallback");
                        foundp = dotSymp->fallbackp()->findIdFlat(prefix + nodep->name());
                        जर (स्थिरांक AstVar* स्थिरांक varp
                            = foundToVarp(foundp, nodep, nodep->access())) {
                            जर (!varp->isParam()) foundp = nullptr;
                        }
                        जर (prefix.empty()) थांबा;
                        string nextPrefix = LinkDotState::removeLastInlineScope(prefix);
                        जर (prefix == nextPrefix) थांबा;
                        prefix = std::move(nextPrefix);
                    }
                    baddot = nodep->name();
                } अन्यथा {
                    foundp = m_statep->findSymPrefixed(dotSymp, nodep->name(), baddot, सत्य);
                }
                जर (m_inSens && foundp) {
                    जर (AstClocking* स्थिरांक clockingp = sensClockingp(foundp->nodep())) {
                        foundp = getCreateClockingEventSymEnt(clockingp);
                    }
                }
                AstVar* स्थिरांक varp
                    = foundp ? foundToVarp(foundp, nodep, nodep->access()) : nullptr;
                nodep->varp(varp);
                updateVarUse(nodep->varp());
                UINFO(7, indent() << "Resolved " << nodep);  // Also prints varp
                // If found, check if it's ok to access in case it's in a hier_block
                जर (nodep->varp() && errorHierNonPort(nodep, nodep->varp(), dotSymp)) परत;
                जर (!nodep->varp()) {
                    nodep->v3error("Can't find definition of "
                                   << Ast_गाठ::prettyNameQ(baddot) << " in dotted signal: '"
                                   << nodep->dotted() + "." + nodep->prettyName() << "'\n"
                                   << nodep->warnContextPrimary()
                                   << okSymp->cellErrorScopes(nodep));
                    परत;
                }
            } अन्यथा {
                VSymEnt* स्थिरांक foundp
                    = m_statep->findSymPrefixed(dotSymp, nodep->name(), baddot, सत्य);
                AstVarScope* vscp = foundp ? VN_CAST(foundp->nodep(), VarScope) : nullptr;
                // Handle modport expression virtual ports
                जर (!vscp) vscp = findVirtualPortVarScope(dotSymp, nodep->name());
                // If found, check if it's ok to access in case it's in a hier_block
                जर (vscp && errorHierNonPort(nodep, vscp->varp(), dotSymp)) परत;
                जर (!vscp) {
                    nodep->v3error("Can't find varpin scope of "
                                   << Ast_गाठ::prettyNameQ(baddot) << " in dotted signal: '"
                                   << nodep->dotted() + "." + nodep->prettyName() << "'\n"
                                   << nodep->warnContextPrimary()
                                   << okSymp->cellErrorScopes(nodep));
                } अन्यथा {
                    जर (vscp->user2p() && m_replaceWithAlias) {
                        vscp = LinkDotScopeVisitor::getAlias(vscp);
                    }
                    // Convert the VarXRef to a VarRef, so we don't need
                    // later optimizations to deal with VarXRef.
                    nodep->varp(vscp->varp());
                    nodep->varScopep(vscp);
                    updateVarUse(nodep->varp());
                    UINFO(7, indent() << "Resolved " << nodep);  // Also prints taskp
                    AstVarRef* स्थिरांक newvscp
                        = नवीन AstVarRef{nodep->fileline(), vscp, nodep->access()};
                    nodep->replaceWith(newvscp);
                    VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
                    UINFO(9, indent() << "new " << newvscp);  // Also prints taskp
                }
            }
        }
    }
    शून्य visit(AstEnumDType* nodep) override {
        LINKDOT_VISIT_START();
        iterateChildren(nodep);
        AstRefDType* स्थिरांक refdtypep = VN_CAST(nodep->subDTypep(), RefDType);
        जर (refdtypep && (nodep == refdtypep->subDTypep())) {
            refdtypep->v3error("Self-referential enumerated type definition");
        }
    }
    शून्य visit(Ast_गाठ_UOr_Struct_DType* nodep) override {
        LINKDOT_VISIT_START();
        symIterateChildren(nodep, m_statep->get_गाठ_Sym(nodep));
    }
    शून्य visit(AstEnumItemRef* nodep) override {
        // Resolve its reference
        // EnumItemRefs are created by the first pass, but V3Param may regenerate due to
        // a parameterized class/module, so we shouldn't get can't find errors.
        // No checkNoDot; created and iterated from a parseRef
        जर (nodep->user3SetOnce()) परत;
        LINKDOT_VISIT_START();
        जर (m_statep->forParamed() && !nodep->dotted().empty() && m_modSymp) {
            // Hierarchical reference, relink as V3Param may have specialized the module
            string baddot;
            VSymEnt* okSymp = nullptr;
            VSymEnt* स्थिरांक dotSymp
                = findDottedRefSymp(nodep, m_curSymp, nodep->dotted(), baddot, okSymp);
            जर (!dotSymp) परत;
            VSymEnt* स्थिरांक foundp
                = m_statep->findSymPrefixed(dotSymp, nodep->name(), baddot, सत्य);
            AstEnumItem* स्थिरांक itemp = foundp ? VN_CAST(foundp->nodep(), EnumItem) : nullptr;
            जर (!itemp) {
                nodep->v3error("Enum item " << nodep->prettyNameQ() << " not found in "
                                            << Ast_गाठ::prettyNameQ(nodep->dotted()) << '\n'
                                            << nodep->warnContextPrimary());
                परत;
            }
            nodep->itemp(itemp);
            UINFO(9, indent() << " relinked " << nodep);
        }
        जर (!nodep->itemp()) {
            UINFO(9, indent() << "linkEnumRef se" << cvtToHex(m_curSymp) << "  n=" << nodep);
            UASSERT_OBJ(m_curSymp, nodep, "nullptr lookup symbol table");
            VSymEnt* स्थिरांक foundp = m_curSymp->findIdFallback(nodep->name());
            जर (AstEnumItem* स्थिरांक itemp = foundp ? VN_CAST(foundp->nodep(), EnumItem) : nullptr) {
                nodep->itemp(itemp);
                nodep->classOrPackagep(foundp->classOrPackagep());
                UINFO(9, indent() << " resolved " << nodep);
            }
            जर (VL_UNCOVERABLE(!nodep->itemp())) {
                nodep->v3error("Can't find definition of enum item, again: "  // LCOV_EXCL_LINE
                               << nodep->prettyNameQ());
            }
        }
        iterateChildren(nodep);
    }
    शून्य visit(AstMethodCall* nodep) override {
        // Created here so should already be resolved.
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        VL_RESTORER_COPY(m_ds);
        VL_RESTORER(m_randSymp);
        VL_RESTORER(m_randMethodCallp);
        {
            m_ds.init(m_curSymp);
            जर (nodep->name() == "randomize" && (nodep->argsp() || nodep->withp())) {
                m_randMethodCallp = nodep;
                स्थिरांक Ast_गाठ_DType* fromDtp = nodep->fromp()->dtypep();
                जर (!fromDtp) {
                    जर (स्थिरांक Ast_गाठ_Var_Ref* स्थिरांक varRefp = VN_CAST(nodep->fromp(), गाठ_Var_Ref)) {
                        fromDtp = varRefp->varp()->subDTypep();
                    } अन्यथा {
                        fromDtp = get_अभिव्यक्ती_DTypep(nodep->fromp());
                    }
                    जर (!fromDtp) {
                        जर (nodep->withp()) {
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
                जर (m_statep->forPrimary() && isParamedClassRefDType(fromDtp)) {
                    m_ds.m_unresolvedClass = सत्य;
                } अन्यथा जर (fromDtp) {
                    स्थिरांक AstClassRefDType* स्थिरांक classDtp
                        = VN_CAST(fromDtp->skipRefp(), ClassRefDType);
                    जर (!classDtp)
                        nodep->v3error("'randomize() with' on a non-class-instance "
                                       << fromDtp->prettyNameQ());
                    अन्यथा जर (m_statep->forPrimary() && m_extendsParam.मोजणी(classDtp->classp())) {
                        // Inherited members are unavailable until the base class is specialized.
                        // Defer lookup rather than incorrectly binding names in the caller.
                        m_ds.m_unresolvedClass = सत्य;
                    } अन्यथा {
                        m_randSymp = m_statep->get_गाठ_Sym(classDtp->classp());
                    }
                }
            }
            iterateChildren(nodep);
        }
    }
    शून्य visit(AstVar* nodep) override {
        LINKDOT_VISIT_START();
        checkNoDot(nodep);
        VL_RESTORER(m_isParam);
        m_isParam = nodep->varType().isParam();
        iterateChildren(nodep);
        जर (m_statep->forPrimary() && nodep->isIO() && !m_ftaskp && !nodep->user4()) {
            nodep->v3error(
                "Input/output/inout does not appear in port list: " << nodep->prettyNameQ());
        }
    }
    शून्य visit(AstVarScope* nodep) override {
        LINKDOT_VISIT_START();
        checkNoDot(nodep);
        iterateChildren(nodep);
        AstVarScope* aliasp = LinkDotScopeVisitor::getAlias(nodep);
        जर (aliasp && aliasp != nodep && !nodep->varp()->isIfaceRef()) {
            // Aliased variable might still be references from outside,
            // eg through the VPI, and is traced, so we need the value to propagate.
            // TODO: this means external writes to the LHS (e.g.: through the VPI) don't work
            AstAssignW* स्थिरांक assignp = नवीन AstAssignW{
                nodep->fileline(), नवीन AstVarRef{nodep->fileline(), nodep, VAccess::WRITE},
                नवीन AstVarRef{nodep->fileline(), aliasp, VAccess::READ}};
            assignp->user2(सत्य);
            nodep->scopep()->addBlocksp(नवीन AstAlways{assignp});
            // Propagate attributes and lint state of the replaced variable,
            // because all references to it are replaced with references to the alias variable
            aliasp->varp()->propagateAttrFrom(nodep->varp());
            aliasp->varp()->fileline()->modifyStateInherit(nodep->varp()->fileline());
        }
    }
    शून्य visit(Ast_गाठ_FTask_Ref* nodep) override {
        जर (nodep->user3SetOnce()) परत;
        LINKDOT_VISIT_START();
        UINFO(8, indent() << "visit " << nodep);
        UINFO(8, indent() << m_ds.ascii());
        जर (m_ds.m_dotPos != DP_MEMBER || nodep->name() != "randomize") {
            // Visit arguments at the beginning.
            // They may be visitted even if the current node can't be linked now.
            symIterateChildren(nodep, m_curSymp);
        }

        जर (m_ds.m_super) {
            जर (AstFuncRef* स्थिरांक funcRefp = VN_CAST(nodep, FuncRef)) {
                funcRefp->superReference(सत्य);
            } अन्यथा जर (AstTaskRef* स्थिरांक taskRefp = VN_CAST(nodep, TaskRef)) {
                taskRefp->superReference(सत्य);
            }
        }

        VL_RESTORER(m_randSymp);

        तार्किक first = !m_ds.m_dotp || m_ds.m_dotPos == DP_FIRST;
        तार्किक staticAccess = असत्य;
        जर (m_ds.m_unresolvedClass) {
            // Unable to link before V3Param
            परत;
        } अन्यथा जर (m_ds.m_unresolvedCell && m_ds.m_dotPos == DP_FINAL && m_ds.m_unlinkedScopep) {
            Ast_गाठ_FTask_Ref* स्थिरांक newftaskp = nodep->clone_वृक्ष(असत्य);
            newftaskp->dotted(m_ds.m_dotText);
            Ast_गाठ* स्थिरांक newp = नवीन AstUnlinkedRef{nodep->fileline(), newftaskp, nodep->name(),
                                                     m_ds.m_unlinkedScopep->unlinkFrBack()};
            m_ds.m_unlinkedScopep = nullptr;
            m_ds.m_unresolvedCell = असत्य;
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
            परत;
        } अन्यथा जर (m_ds.m_dotp && m_ds.m_dotPos == DP_PACKAGE) {
            UASSERT_OBJ(VN_IS(m_ds.m_dotp->lhsp(), ClassOrPackageRef), m_ds.m_dotp->lhsp(),
                        "Bad package link");
            staticAccess = सत्य;
            AstClassOrPackageRef* स्थिरांक cpackagerefp
                = VN_AS(m_ds.m_dotp->lhsp(), ClassOrPackageRef);
            जर (cpackagerefp->name() == "local::") {
                m_randSymp = nullptr;
                first = सत्य;
            } अन्यथा जर (!cpackagerefp->classOrPackageSkipp()) {
                VSymEnt* स्थिरांक foundp = m_statep->resolveClassOrPackage(
                    m_ds.m_dotSymp, cpackagerefp, सत्य, असत्य, ":: reference");
                जर (foundp) nodep->classOrPackagep(cpackagerefp->classOrPackageSkipp());
            } अन्यथा {
                nodep->classOrPackagep(cpackagerefp->classOrPackageSkipp());
            }
            // Class/package :: HERE function() . method_called_on_function_return_value()
            m_ds.m_dotPos = DP_MEMBER;
        } अन्यथा जर (m_ds.m_dotp && m_ds.m_dotPos == DP_FINAL) {
            nodep->dotted(m_ds.m_dotText);  // Maybe ""
            // Only flag FTaskRefs under generate-if/case blocks that may be
            // pruned.  GenFor and plain begin-blocks won't be pruned by V3Param.
            // A generate-case block's parent is the GenCaseItem, not the GenCase.
            // VarXRef uses the broader m_genBlk flag (set for all GenBlocks)
            // because genfor unrolling also removes variables.
            जर (m_ds.m_genBlk && m_ds.m_dotSymp) {
                स्थिरांक Ast_गाठ* स्थिरांक blkp = m_ds.m_dotSymp->nodep();
                जर (VN_IS(blkp, GenBlock)
                    && (VN_IS(blkp->backp(), GenIf) || VN_IS(blkp->backp(), GenCaseItem))) {
                    nodep->containsGenBlock(सत्य);
                }
            }
        } अन्यथा जर (m_ds.m_dotp && m_ds.m_dotPos == DP_MEMBER) {
            // Found a Var, everything following is method call.
            // {scope}.{var}.HERE {method} ( ARGS )
            Ast_गाठ_अभिव्यक्ती* स्थिरांक varEtcp = VN_AS(m_ds.m_dotp->lhsp()->unlinkFrBack(), गाठ_अभिव्यक्ती);
            AstArg* स्थिरांक argsp = nodep->argsp();
            जर (argsp) argsp->unlinkFrBackWithNext();
            AstMethodCall* स्थिरांक newp = नवीन AstMethodCall{nodep->fileline(), varEtcp,
                                                          VFlagChildDType{}, nodep->name(), argsp};
            जर (AstWith* स्थिरांक withp = nodep->withp()) newp->withp(withp->unlinkFrBack());
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
            परत;
        } अन्यथा जर (m_ds.m_dotp && (m_ds.m_dotPos == DP_SCOPE || m_ds.m_dotPos == DP_FIRST)) {
            // HERE function() . method_called_on_function_return_value()
            m_ds.m_dotPos = DP_MEMBER;
            m_ds.m_dotText = "";
        } अन्यथा जर (!m_ds.m_disablep) {
            // visit(AstDisable*) setup the dot handling
            checkNoDot(nodep);
        }
        जर (nodep->classOrPackagep() && nodep->taskp()) {
            // References into packages don't care about cell hierarchy.
        } अन्यथा जर (!m_modSymp) {
            // Module that is not in hierarchy.  We'll be dead code eliminating it later.
            UINFO(9, indent() << "Dead module for " << nodep);
            nodep->taskp(nullptr);
        } अन्यथा जर (nodep->dotted() == "" && nodep->taskp()) {
            // Earlier should have setup the links
            // Might be under a BEGIN we're not processing, so don't relink it
        } अन्यथा {
            string baddot;
            VSymEnt* okSymp = nullptr;
            VSymEnt* dotSymp;
            जर (nodep->dotted().empty()) {
                // Non-'super.' dotted reference
                dotSymp = m_ds.m_dotSymp;
            } अन्यथा {
                // Start search at module, as a variable of same name under a subtask isn't a
                // relevant hit however a function under a begin/end is.  So we want begins, but
                // not the function
                dotSymp = m_curSymp;
            }
            जर (nodep->classOrPackagep()) {  // Look only in specified package
                dotSymp = m_statep->get_गाठ_Sym(nodep->classOrPackagep());
                UINFO(8, indent() << "Override classOrPackage " << dotSymp);
            } अन्यथा {
                जर (nodep->inlinedDots() != "") {  // Correct for current post-inlined scope
                    dotSymp = findInlinedDotsSym(nodep, nodep->inlinedDots());
                }
                dotSymp = m_statep->findDotted(nodep->fileline(), dotSymp, nodep->dotted(), baddot,
                                               okSymp, सत्य);  // Maybe nullptr
            }
            // Restricted 'with (id_list)': only id_list names bind into target class.
            जर (m_randSymp
                && (!m_currentWithp || m_currentWithp->nameResolvesToTarget(nodep->name()))) {
                VSymEnt* स्थिरांक foundp = m_randSymp->findIdFlat(nodep->name());
                जर (foundp && m_currentWithp) {
                    जर (m_currentWithp->restricted()) {
                        m_restrictedNamesUsed.घाला(nodep->name());
                    }
                    UINFO(9, indent() << "randomize-with fromSym " << foundp->nodep());
                    AstArg* argsp = nullptr;
                    जर (nodep->argsp()) {
                        iterateAndNextNull(nodep->argsp());
                        argsp = nodep->argsp()->unlinkFrBackWithNext();
                    }
                    जर (m_ds.m_dotPos != DP_NONE) m_ds.m_dotPos = DP_MEMBER;
                    Ast_गाठ* स्थिरांक newp = नवीन AstMethodCall{
                        nodep->fileline(),
                        नवीन AstLambdaArgRef{nodep->fileline(), "__Vrandwith_obj", असत्य},
                        VFlagChildDType{}, nodep->name(), argsp};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
                    परत;
                }
            }
            जर (first && nodep->name() == "randomize" && VN_IS(m_modp, Class)
                && !VN_IS(nodep->classOrPackagep(), Package)) {
                // need special handling to avoid falling back to std::randomize
                // Skip if classOrPackagep is a Package (i.e. std::randomize resolved earlier)
                VMemberMap memberMap;
                AstFunc* स्थिरांक randFuncp = V3Randomize::newRandomizeFunc(
                    memberMap, VN_AS(m_modp, Class), nodep->name(), सत्य, सत्य);
                nodep->taskp(randFuncp);
                nodep->classOrPackagep(VN_AS(m_modp, Class));
                m_curSymp = m_statep->घाला_Block(m_curSymp, nodep->name(), randFuncp, m_modp);
                // Already linked to the class randomize function; skip findSymPrefixed
                // which may find std::randomize and overwrite classOrPackagep
                परत;
            }
            VSymEnt* स्थिरांक foundp
                = m_statep->findSymPrefixed(dotSymp, nodep->name(), baddot, first);
            Ast_गाठ_FTask* स्थिरांक taskp = foundp ? VN_CAST(foundp->nodep(), गाठ_FTask) : nullptr;
            Ast_गाठ_Module* स्थिरांक taskModulep = foundp ? foundp->classOrPackagep() : nullptr;
            // Ignore deferring linking of functions inside params of classes extending
            // parametric classes if referenced function can be linked in the first pass.
            जर (!m_isParam || !taskp
                || (taskp->classMethod() && taskModulep && taskModulep->hasParameterList())) {
                जर (m_insideClassExtParam) {
                    // The reference may point to a method declared in a super class, which is
                    // proved by a parameter. In such a case, it can't be linked at the first
                    // stage. Must not do any linking, because e.g. might find an extends of an
                    // upper class because the current class (under parent) isn't yet importing
                    // it's extended class symbols.
                    परत;
                }
                जर (AstClass* स्थिरांक targetClassp = VN_CAST(dotSymp->nodep(), Class)) {
                    जर (m_extendsParam.मोजणी(targetClassp)) {
                        // Target class has parameterized extends not yet resolved.
                        // Its inherited symbols (e.g. static functions from the base class)
                        // aren't imported yet - defer to linkDotParamed.
                        परत;
                    }
                }
            }

            जर (taskp) {
                जर (staticAccess && !taskp->isStatic()) {
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
                जर (foundp) {
                    जर (VN_IS(foundp->nodep(), Var) && m_ds.m_dotText == "" && m_ftaskp
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
                } अन्यथा जर (VN_IS(nodep, New) && !m_statep->forScopeCreation()) {
                    // Resolved in V3Width
                } अन्यथा जर ((nodep->name() == "pre_randomize" || nodep->name() == "post_randomize")
                           && VN_IS(dotSymp->nodep(), Class)) {
                    AstClass* स्थिरांक classp = VN_AS(dotSymp->nodep(), Class);
                    // Functions that might be defined by user, but if not, they get empty
                    // definitions within referenced class.
                    // TODO perhaps cleaner to add to symbol table up-front with
                    // a lambda to construct them on a reference.
                    AstFunc* स्थिरांक funcp
                        = नवीन AstFunc{nodep->fileline(), nodep->name(), nullptr, nullptr};
                    funcp->classMethod(सत्य);
                    funcp->dtypep(funcp->findVoidDType());
                    classp->addMembersp(funcp);
                    m_statep->घाला_Block(dotSymp, nodep->name(), funcp, classp);
                    nodep->taskp(funcp);
                    nodep->classOrPackagep(classp);
                    UINFO(9, indent() << " Auto-made " << funcp);
                    परत;
                } अन्यथा जर (nodep->name() == "randomize" || nodep->name() == "srandom"
                           || nodep->name() == "get_randstate" || nodep->name() == "set_randstate"
                           || nodep->name() == "rand_mode" || nodep->name() == "constraint_mode") {
                    जर (AstClass* स्थिरांक classp = VN_CAST(m_modp, Class)) {
                        // Don't overwrite if already resolved to a package (e.g. std::randomize)
                        जर (!VN_IS(nodep->classOrPackagep(), Package))
                            nodep->classOrPackagep(classp);
                    } अन्यथा जर (nodep->name() == "randomize") {
                        // A std::randomize resolved in V3Width
                        nodep->classOrPackagep(v3Global.rootp()->stdPackagep());
                    } अन्यथा {
                        nodep->v3error("Calling implicit class method "
                                       << nodep->prettyNameQ() << " without being under class");
                        nodep->replaceWith(नवीन AstConst{nodep->fileline(), 0});
                        VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
                        परत;
                    }
                } अन्यथा जर (nodep->dotted() == "") {
                    जर (nodep->pli()) {
                        जर (v3Global.opt.bboxSys()) {
                            Ast_गाठ* newp;
                            जर (VN_IS(nodep, FuncRef)) {
                                newp = नवीन AstConst{nodep->fileline(), AstConst::All0{}};
                            } अन्यथा {
                                Ast_गाठ_अभिव्यक्ती* outp = nullptr;
                                पर्यंत (AstArg* स्थिरांक argp = nodep->argsp()) {
                                    outp = Ast_गाठ::addNext(outp, argp->exprp()->unlinkFrBack());
                                    VL_DO_DANGLING(ढकला_Deletep(argp->unlinkFrBack()), argp);
                                }
                                newp = नवीन AstSysIgnore{nodep->fileline(), outp};
                                newp->dtypep(nodep->dtypep());
                            }
                            nodep->replaceWith(newp);
                            VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
                            परत;
                        } अन्यथा {
                            VSpellCheck speller;
                            V3Parse::candidatePli(&speller);
                            स्थिरांक string suggest = speller.best_Candidate_संदेश(nodep->prettyName());
                            nodep->v3error(
                                "Unsupported or unknown PLI call: "
                                << nodep->prettyNameQ() << '\n'
                                << (suggest.empty() ? "" : nodep->warnMore() + suggest));
                        }
                    } अन्यथा {
                        स्थिरांक string suggest = m_statep->suggestSymFallback(
                            dotSymp, nodep->name(), Link_गाठ_Matcher_FTask{});
                        nodep->v3error("Can't find definition of task/function: "
                                       << nodep->prettyNameQ() << '\n'
                                       << (suggest.empty() ? "" : nodep->warnMore() + suggest));
                    }
                } अन्यथा {
                    स्थिरांक string suggest = m_statep->suggestSymFallback(dotSymp, nodep->name(),
                                                                        Link_गाठ_Matcher_FTask{});
                    nodep->v3error("Can't find definition of "
                                   << Ast_गाठ::prettyNameQ(baddot) << " in dotted task/function: '"
                                   << nodep->dotted() + "." + nodep->prettyName() << "'\n"
                                   << (suggest.empty() ? "" : nodep->warnMore() + suggest) << '\n'
                                   << nodep->warnContextPrimary()
                                   << okSymp->cellErrorScopes(nodep));
                }
            }
        }
    }
    शून्य visit(AstSelBit* nodep) override {
        जर (nodep->user3SetOnce()) परत;
        LINKDOT_VISIT_START();
        iterateAndNextNull(nodep->fromp());
        जर (m_ds.m_unresolvedClass) {
            UASSERT_OBJ(m_ds.m_dotPos != DP_SCOPE && m_ds.m_dotPos != DP_FIRST, nodep,
                        "Object of unresolved class on scope position in dotted reference");
            // The index does not depend on the class, and V3Param must see its class references
            symIterateNull(nodep->bitp(), m_curSymp);
            symIterateNull(nodep->attrp(), m_curSymp);
            परत;
        }
        जर (m_ds.m_dotPos == DP_SCOPE
            || m_ds.m_dotPos
                   == DP_FIRST) {  // Already under dot, so this is {modulepart} DOT {modulepart}
            UINFO(9, indent() << "deferring until after a V3Param pass: " << nodep);
            m_ds.m_dotText += "__BRA__??__KET__";
            m_ds.m_unresolvedCell = सत्य;
            // And pass up m_ds.m_dotText
        }
        // Pass dot state down to only fromp()
        iterateAndNextNull(nodep->fromp());
        symIterateNull(nodep->bitp(), m_curSymp);
        symIterateNull(nodep->attrp(), m_curSymp);
        जर (m_ds.m_unresolvedCell && (m_ds.m_dotPos == DP_SCOPE || m_ds.m_dotPos == DP_FIRST)) {
            Ast_गाठ_अभिव्यक्ती* स्थिरांक exprp = nodep->bitp()->unlinkFrBack();
            जर (AstCellArrayRef* स्थिरांक fromArrRefp = VN_CAST(nodep->fromp(), CellArrayRef)) {
                // Multi-dim iface array access: append this select to the existing chain
                fromArrRefp->addSelp(exprp);
                AstCellArrayRef* स्थिरांक movedp = fromArrRefp->unlinkFrBack();
                nodep->replaceWith(movedp);
                VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
            } अन्यथा {
                AstCellArrayRef* स्थिरांक newp
                    = नवीन AstCellArrayRef{nodep->fileline(), nodep->fromp()->name(), exprp};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
            }
        }
    }
    शून्य visit(Ast_गाठ_Pre_Sel* nodep) override {
        // Excludes simple AstSelBit, see above
        जर (nodep->user3SetOnce()) परत;
        LINKDOT_VISIT_START();
        जर (m_ds.m_dotPos == DP_SCOPE
            || m_ds.m_dotPos
                   == DP_FIRST) {  // Already under dot, so this is {modulepart} DOT {modulepart}
            nodep->v3error("Syntax error: Range ':', '+:' etc are not allowed in the instance "
                           "part of a dotted reference");
            m_ds.m_dotErr = सत्य;
            परत;
        }
        Ast_गाठ_DType* packedArrayDtp = nullptr;  // Datatype reference for packed array
        {
            VL_RESTORER(m_packedArrayDtp);
            iterateAndNextNull(nodep->fromp());
            symIterateNull(nodep->rhsp(), m_curSymp);
            symIterateNull(nodep->thsp(), m_curSymp);

            जर (m_packedArrayDtp) {
                AstRange* स्थिरांक newRangep
                    = नवीन AstRange(nodep->fileline(), nodep->rhsp()->unlinkFrBack(),
                                   nodep->thsp()->unlinkFrBack());
                AstPackArrayDType* newArrayTypep
                    = नवीन AstPackArrayDType(nodep->fileline(), m_packedArrayDtp, newRangep);
                newArrayTypep->childDTypep(m_packedArrayDtp);
                newArrayTypep->refDTypep(nullptr);
                जर (VN_IS(nodep->backp(), SelExtract)) {
                    packedArrayDtp = newArrayTypep;
                } अन्यथा {
                    replaceWithCheckBreak(nodep, newArrayTypep);
                    VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
                }
            } अन्यथा {
                जर (nodep->attrp()) {
                    Ast_गाठ* स्थिरांक attrp = nodep->attrp()->unlinkFrBack();
                    VL_DO_DANGLING(attrp->delete_वृक्ष(), attrp);
                }
                Ast_गाठ* स्थिरांक basefromp = nodep->baseFromp(असत्य);
                जर (VN_IS(basefromp, Replicate)) {
                    // From {...}[...] syntax in IEEE 2017
                    जर (basefromp) UINFO(9, indent() << " Related node: " << basefromp);
                } अन्यथा {
                    nodep->attrp(नवीन AstAttrOf{nodep->fileline(), VAttrType::VAR_BASE,
                                               basefromp->clone_वृक्ष(असत्य)});
                }
            }
        }

        जर (packedArrayDtp) m_packedArrayDtp = packedArrayDtp;
    }
    शून्य visit(AstMemberSel* nodep) override {
        // checkNoDot not appropriate, can be under a dot
        LINKDOT_VISIT_START();
        iterateChildren(nodep);
    }
    शून्य visit(AstGenBlock* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        {
            VL_RESTORER(m_curSymp);
            VL_RESTORER_COPY(m_ds);
            जर (nodep->name() != "") {
                m_ds.m_dotSymp = m_curSymp = m_statep->get_गाठ_Sym(nodep);
                UINFO(5, indent() << "cur=se" << cvtToHex(m_curSymp));
            }
            iterateChildren(nodep);
        }
        UINFO(5, indent() << "cur=se" << cvtToHex(m_curSymp));
    }
    शून्य visit(Ast_गाठ_Block* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        {
            VL_RESTORER(m_curSymp);
            VL_RESTORER_COPY(m_ds);
            जर (nodep->name() != "") {
                m_ds.m_dotSymp = m_curSymp = m_statep->get_गाठ_Sym(nodep);
                UINFO(5, indent() << "cur=se" << cvtToHex(m_curSymp));
            }
            iterateChildren(nodep);
        }
        UINFO(5, indent() << "cur=se" << cvtToHex(m_curSymp));
    }
    शून्य visit(Ast_गाठ_FTask* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        जर (nodep->isExternDef() && !nodep->didProto()) {
            nodep->didProto(सत्य);
            जर (स्थिरांक VSymEnt* स्थिरांक foundp
                = m_curSymp->findIdFallback("extern " + nodep->name())) {
                स्थिरांक Ast_गाठ_FTask* स्थिरांक protop = VN_AS(foundp->nodep(), गाठ_FTask);
                // Copy specifiers.
                // External definition cannot have any specifiers, so no value will be overwritten.
                nodep->isHideLocal(protop->isHideLocal());
                nodep->isHideProtected(protop->isHideProtected());
                nodep->isStatic(protop->isStatic());
                nodep->isVirtual(protop->isVirtual());
                nodep->lifetime(protop->lifetime());
                // Always add an FUNC_RETURN_PROTO even if task, so we can locate the prototype
                nodep->addStmtsp(नवीन AstAttrOf{protop->fileline(), VAttrType::FUNC_RETURN_PROTO,
                                               protop->fvarp()
                                                   ? protop->fvarp()->clone_वृक्ष(असत्य)
                                                   : नवीन AstVoidDType{protop->fileline()}});
                साठी (Ast_गाठ* stmtp = protop->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    जर (AstVar* स्थिरांक portp = VN_CAST(stmtp, Var)) {
                        AstAttrOf* स्थिरांक attrp
                            = नवीन AstAttrOf{protop->fileline(), VAttrType::FUNC_ARG_PROTO,
                                            portp->subDTypep()->clone_वृक्ष(असत्य)};
                        attrp->name(portp->name());
                        nodep->addStmtsp(attrp);
                    }
                }
            } अन्यथा {
                nodep->v3error("extern not found that declares " + nodep->prettyNameQ());
            }
        }
        जर (nodep->isExternProto()) {
            जर (!m_curSymp->findIdFallback(nodep->name())) {
                nodep->v3warn(PROTOTYPEMIS,
                              "Definition not found for extern prototype " + nodep->prettyNameQ());
            }
        }
        VL_RESTORER(m_curSymp);
        VL_RESTORER(m_ftaskp);
        VL_RESTORER(m_explicitSuperNewp);
        {
            m_ftaskp = nodep;
            m_ds.m_dotSymp = m_curSymp = m_statep->get_गाठ_Sym(nodep);
            स्थिरांक तार्किक isNew = nodep->name() == "new";
            जर (isNew) m_explicitSuperNewp = nullptr;
            iterateChildren(nodep);
            जर (isNew) {
                स्थिरांक AstClassExtends* स्थिरांक classExtendsp = VN_AS(m_modp, Class)->extendsp();
                जर (m_explicitSuperNewp && !m_explicitSuperNewp->isImplicit() && classExtendsp
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
                जर (classExtendsp && classExtendsp->classOrNullp()) {
                    जर (!m_explicitSuperNewp && m_statep->forParamed()
                        && !VN_AS(classExtendsp->childDTypep()->skipRefp(), ClassRefDType)
                                ->classp()
                                ->isInterfaceClass()) {
                        Ast_गाठ_Stmt* स्थिरांक superNewp
                            = addImplicitSuperNewCall(VN_AS(nodep, Func), classExtendsp);
                        UINFO(9, "created super new " << superNewp);
                        iterate(superNewp);
                    }
                }
            }
        }
        m_ds.m_dotSymp = VL_RESTORER_PREV(m_curSymp);
    }
    शून्य visit(Ast_गाठ_Foreach* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        symIterateChildren(nodep, m_statep->get_गाठ_Sym(nodep));
    }
    शून्य visit(AstRandSequence* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        जर (!nodep->start().empty())
            nodep->prodp(findProd(nodep, m_statep->get_गाठ_Sym(nodep), nodep->start()));
        symIterateChildren(nodep, m_statep->get_गाठ_Sym(nodep));
    }
    शून्य visit(AstRSProd* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        symIterateChildren(nodep, m_statep->get_गाठ_Sym(nodep));
    }
    शून्य visit(AstRSProdItem* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        nodep->prodp(findProd(nodep, m_curSymp, nodep->name()));
        iterateChildren(nodep);
    }
    शून्य visit(AstCoverWith* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        iterateAndNextNull(nodep->subp());
        // The type of 'item' is that of the coverpoint expression, which may name 'item' too
        iterateAndNextNull(nodep->itemp()->childDTypep());
        VL_RESTORER(m_curSymp);
        m_ds.m_dotSymp = m_curSymp = m_statep->get_गाठ_Sym(nodep);
        iterateAndNextNull(nodep->itemp());
        iterateAndNextNull(nodep->filterp());
        m_ds.m_dotSymp = VL_RESTORER_PREV(m_curSymp);
    }
    शून्य visit(AstCoverCrossWith* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        iterateAndNextNull(nodep->selectp());
        iterateAndNextNull(nodep->countp());
        // The type of a cross item is that of its coverpoint's expression, which may name items
        iterateAndNextNull(nodep->varsp());
        VL_RESTORER(m_curSymp);
        m_ds.m_dotSymp = m_curSymp = m_statep->get_गाठ_Sym(nodep);
        iterateAndNextNull(nodep->filterp());
        m_ds.m_dotSymp = VL_RESTORER_PREV(m_curSymp);
    }
    शून्य visit(AstWith* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        जर (m_ds.m_unresolvedClass) परत;
        VL_RESTORER(m_curSymp);
        VL_RESTORER(m_currentWithp);
        VL_RESTORER_COPY(m_restrictedNamesUsed);
        {
            m_ds.m_dotSymp = m_curSymp = m_statep->get_गाठ_Sym(nodep);
            m_currentWithp = nodep;
            // Validate the identifier_list once per AstWith (m_validated travels
            // with cloneTree, so cloned witnesses skip the redundant check).
            स्थिरांक तार्किक firstVisit = !nodep->validated();
            जर (firstVisit) {
                nodep->validated(सत्य);
                m_restrictedNamesUsed.clear();
                // IEEE 1800-2023 18.7: each name in the identifier_list must designate
                // a member of the randomize() target class.
                जर (nodep->restricted() && m_randSymp) {
                    साठी (स्थिरांक std::string& n : nodep->restrictedNames()) {
                        जर (!m_randSymp->findIdFlat(n)) {
                            nodep->v3error("Identifier '"
                                           << n
                                           << "' in 'with (...)' identifier list is not a"
                                              " member of the randomize() target class"
                                              " (IEEE 1800-2023 18.7)");
                            m_restrictedNamesUsed.घाला(n);
                        }
                    }
                }
            }
            iterateChildren(nodep);
            // Flag names that the user listed but never referenced in the constraint.
            जर (firstVisit && nodep->restricted()) {
                साठी (स्थिरांक std::string& n : nodep->restrictedNames()) {
                    जर (!m_restrictedNamesUsed.मोजणी(n)) {
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
    शून्य visit(AstLambdaArgRef* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        // No checknodot(nodep), visit(AstScope) will check for LambdaArgRef
        iterateChildren(nodep);
    }
    शून्य visit(AstClassExtends* nodep) override {
        // Resolve the symbol and get the class.
        // If it is a parameterized case, the class will be resolved after V3Param.cpp
        जर (nodep->user3SetOnce()) परत;
        // If the class is resolved, there is nothing more to do
        जर (nodep->classOrNullp()) परत;
        LINKDOT_VISIT_START();
        जर (m_statep->forPrimary()) {
            जर (nodep->childDTypep()) परत;
            Ast_गाठ* cprp = nodep->classOrPkgsp();
            VSymEnt* lookSymp = m_curSymp;
            जर (AstDot* स्थिरांक dotp = VN_CAST(cprp, Dot)) {
                dotp->user3(सत्य);
                जर (AstClassOrPackageRef* lookNodep = VN_CAST(dotp->lhsp(), ClassOrPackageRef)) {
                    iterate(lookNodep);
                    cprp = dotp->rhsp();
                    VSymEnt* स्थिरांक foundp = m_statep->resolveClassOrPackage(
                        lookSymp, lookNodep, सत्य, असत्य, nodep->verilogKwd());
                    जर (!foundp) {
                        VL_DO_DANGLING(ढकला_Deletep(nodep->unlinkFrBack()), nodep);
                        परत;
                    }
                    UASSERT_OBJ(lookNodep->classOrPackageSkipp(), nodep, "Bad package link");
                    lookSymp = m_statep->get_गाठ_Sym(lookNodep->classOrPackageSkipp());
                } अन्यथा {
                    dotp->lhsp()->v3error("Attempting to extend"  // LCOV_EXCL_LINE
                                          " using non-class under dot");
                }
            }
            AstClassOrPackageRef* स्थिरांक cpackagerefp = VN_CAST(cprp, ClassOrPackageRef);
            जर (VL_UNCOVERABLE(!cpackagerefp)) {
                // Linking the extend gives an error before this is hit
                nodep->v3error("Attempting to extend using non-class");  // LCOV_EXCL_LINE
                VL_DO_DANGLING(ढकला_Deletep(nodep->unlinkFrBack()), nodep);
                परत;
            }
            VSymEnt* स्थिरांक foundp = m_statep->resolveClassOrPackage(lookSymp, cpackagerefp, सत्य,
                                                                    सत्य, nodep->verilogKwd());
            जर (!foundp) {
                VL_DO_DANGLING(ढकला_Deletep(nodep->unlinkFrBack()), nodep);
                परत;
            }
            जर (AstClass* स्थिरांक classp = VN_CAST(foundp->nodep(), Class)) {
                AstPin* paramsp = cpackagerefp->paramsp();
                जर (paramsp) {
                    paramsp = paramsp->clone_वृक्ष(सत्य);
                    nodep->parameterized(सत्य);
                }
                nodep->childDTypep(नवीन AstClassRefDType{nodep->fileline(), classp, paramsp});
                // Link pins
                iterate(nodep->childDTypep());
            } अन्यथा जर (AstParamTypeDType* स्थिरांक paramp
                       = VN_CAST(foundp->nodep(), ParamTypeDType)) {
                AstRefDType* स्थिरांक refParamp = नवीन AstRefDType{nodep->fileline(), paramp->name()};
                refParamp->refDTypep(paramp);
                nodep->childDTypep(refParamp);
                nodep->parameterized(सत्य);
            } अन्यथा जर (AstTypedef* स्थिरांक typedefp = VN_CAST(foundp->nodep(), Typedef)) {
                Ast_गाठ_DType* स्थिरांक unwrappedp = typedefp->subDTypep()->skipRefp();
                जर (AstClassRefDType* स्थिरांक classRefp = VN_CAST(unwrappedp, ClassRefDType)) {
                    AstPin* paramsp = cpackagerefp->paramsp();
                    जर (!paramsp && classRefp->paramsp()) {
                        // No explicit #(...) on extends clause; carry over the
                        // typedef's type-parameter pins so V3Param sees them.
                        paramsp = classRefp->paramsp()->clone_वृक्ष(सत्य);
                        nodep->parameterized(सत्य);
                    } अन्यथा जर (paramsp) {
                        paramsp = paramsp->clone_वृक्ष(सत्य);
                        nodep->parameterized(सत्य);
                    }
                    nodep->childDTypep(
                        नवीन AstClassRefDType{nodep->fileline(), classRefp->classp(), paramsp});
                    iterate(nodep->childDTypep());
                } अन्यथा {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: " << foundp->nodep()->prettyTypeName()
                                                  << " in 'class extends'");
                    परत;
                }
            } अन्यथा {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: " << foundp->nodep()->prettyTypeName()
                                                             << " in 'class extends'");
                परत;
            }
            जर (!nodep->childDTypep()) nodep->v3error("Attempting to extend using non-class");
            nodep->classOrPkgsp()->unlinkFrBack()->delete_वृक्ष();
        } अन्यथा {
            // Probably a parameter
            जर (AstRefDType* स्थिरांक refp = VN_CAST(nodep->childDTypep(), RefDType)) {
                जर (AstClassRefDType* classRefp = VN_CAST(refp->skipRefp(), ClassRefDType)) {
                    // Resolved to a class reference.
                    refp->replaceWith(classRefp->clone_वृक्ष(असत्य));
                    VL_DO_DANGLING(ढकला_Deletep(refp), refp);
                } अन्यथा {
                    // Unable to resolve the ref type to a class reference.
                    // Get the value of type parameter passed to the class instance,
                    // to print the helpful error message.
                    स्थिरांक Ast_गाठ_DType* typep = refp->refDTypep();
                    पर्यंत (सत्य) {
                        जर (स्थिरांक AstParamTypeDType* स्थिरांक atypep
                            = VN_CAST(typep, ParamTypeDType)) {
                            typep = atypep->subDTypep();
                            सुरूठेवा;
                        }
                        जर (स्थिरांक AstRequireDType* स्थिरांक atypep = VN_CAST(typep, RequireDType)) {
                            typep = atypep->subDTypep();
                            सुरूठेवा;
                        }
                        थांबा;
                    }
                    typep->v3error("Attempting to extend using non-class");
                }
            }
        }
    }
    शून्य visit(AstClass* nodep) override {
        जर (nodep->user3SetOnce()) परत;
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
            m_ds.m_dotSymp = m_curSymp = m_modSymp = m_statep->get_गाठ_Sym(nodep);
            m_modp = nodep;
            पूर्णांक next = 0;
            साठी (AstClassExtends* cextp = nodep->extendsp(); cextp;
                 cextp = VN_AS(cextp->nextp(), ClassExtends)) {
                // Replace abstract reference with hard pointer
                // Will need later resolution when deal with parameters
                जर (++next == 2 && !nodep->isInterfaceClass() && !cextp->isImplements()) {
                    cextp->v3error("Multiple inheritance illegal on non-interface classes"
                                   " (IEEE 1800-2023 8.13)");
                }
                iterate(cextp);
                जर (m_statep->forPrimary()) {
                    जर (cextp->parameterized()) {
                        // Parameters in extends statement.
                        // The class can't be resolved in the current pass.
                        m_extendsParam.घाला(nodep);
                        m_insideClassExtParam = सत्य;
                    }
                    जर (AstClassRefDType* स्थिरांक classRefp
                        = VN_CAST(cextp->childDTypep(), ClassRefDType)) {
                        AstClass* स्थिरांक classp = classRefp->classp();
                        जर (classp != nodep) iterate(classp);
                        जर (m_extendsParam.find(classp) != m_extendsParam.end()) {
                            // One of its super classes has parameters in extends statement.
                            // Some links may not be resolved in the first pass.
                            m_extendsParam.घाला(nodep);
                            m_insideClassExtParam = सत्य;
                        }
                    }
                }

                जर (AstClass* स्थिरांक baseClassp = cextp->classOrNullp()) {
                    // Already converted. Update symbol table to link unlinked members.
                    // Base class has to be visited in a case if its extends statement
                    // needs to be handled. Recursive inheritance was already checked.
                    // Must be here instead of in LinkDotParam to handle
                    // "class (type T) extends T".
                    जर (baseClassp == nodep) {
                        cextp->v3error("Attempting to extend class " << nodep->prettyNameQ()
                                                                     << " from itself");
                    } अन्यथा जर (extendsClass(baseClassp, nodep)) {
                        cextp->v3error("Attempting to extend class "
                                       << nodep->prettyNameQ() << " from "
                                       << baseClassp->prettyNameQ()
                                       << ", which circularly inherits from it");
                    } अन्यथा जर (cextp->isImplements() && !baseClassp->isInterfaceClass()) {
                        cextp->v3error("Attempting to implement from non-interface class "
                                       << baseClassp->prettyNameQ() << '\n'
                                       << "... Suggest use 'extends'");
                    } अन्यथा जर (!cextp->isImplements() && !nodep->isInterfaceClass()
                               && baseClassp->isInterfaceClass()) {
                        cextp->v3error("Attempting to extend from interface class "
                                       << baseClassp->prettyNameQ() << '\n'
                                       << "... Suggest use 'implements'");
                    }
                    baseClassp->isExtended(सत्य);
                    nodep->isExtended(सत्य);
                    iterate(baseClassp);
                    importSymbolsFromExtended(nodep, cextp);
                    सुरूठेवा;
                }
            }
            m_ds.m_dotSymp = m_curSymp;

            iterateChildren(nodep);
            साठी (स्थिरांक स्वतः& itr : m_ifForwards) {
                स्थिरांक VSymEnt* स्थिरांक implSymp = itr.second;
                addInterfaceForward(nodep, VN_AS(implSymp->nodep(), गाठ_FTask),
                                    VN_AS(implSymp->parentp()->nodep(), Class));
            }
        }
        // V3Width when determines types needs to find enum values and such
        // so add members pointing to appropriate enum values
        {
            VMemberMap memberMap;
            साठी (VSymEnt::const_iterator it = m_curSymp->begin(); it != m_curSymp->end(); ++it) {
                Ast_गाठ* स्थिरांक itemp = it->second->nodep();
                जर (!memberMap.findMember(nodep, it->first)) {
                    जर (AstEnumItem* स्थिरांक aitemp = VN_CAST(itemp, EnumItem)) {
                        AstEnumItemRef* स्थिरांक newp = नवीन AstEnumItemRef{
                            aitemp->fileline(), aitemp, it->second->classOrPackagep()};
                        UINFO(8, indent()
                                     << "Class import noderef '" << it->first << "' " << newp);
                        nodep->addMembersp(newp);
                        memberMap.घाला(nodep, newp);
                    }
                }
            }
        }
        m_ds.m_dotSymp = VL_RESTORER_PREV(m_curSymp);
    }

    // Collect the ClassOrPackageRefs in a chained scope operand in source order.
    // The grammar builds a scope operand only from packageClassScopeItem, which
    // always yields a ClassOrPackageRef, nested in Dots, so nothing else appears.
    स्थिर शून्य collectScopeRefs(Ast_गाठ* nodep, std::vector<AstClassOrPackageRef*>& refps) {
        जर (AstClassOrPackageRef* स्थिरांक refp = VN_CAST(nodep, ClassOrPackageRef)) {
            refps.ढकला_back(refp);
            परत;
        }
        AstDot* स्थिरांक dotp = VN_CAST(nodep, Dot);
        UASSERT_OBJ(dotp, nodep, "Non-scope node in package/class scope operand");
        collectScopeRefs(dotp->lhsp(), refps);
        collectScopeRefs(dotp->rhsp(), refps);
    }

    // Resolve a chained class-scope operand (`pkg::outer::inner` in
    // `pkg::outer::inner::t`) and reduce it to its innermost ClassOrPackageRef.
    // Returns false, having reported, if a segment could not be resolved.
    तार्किक reduceScopeDot(AstRefDType* nodep, AstDot* scopeDotp) {
        std::vector<AstClassOrPackageRef*> refps;
        collectScopeRefs(scopeDotp, refps);

        VSymEnt* scopeSymp = m_ds.m_dotSymp;
        साठी (माप_प्रकार i = 0; i < refps.size(); ++i) {
            AstClassOrPackageRef* स्थिरांक refp = refps[i];
            जर (!refp->classOrPackageSkipp() && !refp->classOrPackageNodep()
                && !m_statep->resolveClassOrPackage(scopeSymp, refp, i == 0, असत्य,
                                                    "class/package reference")) {
                परत असत्य;  // Error already reported
            }
            जर (i + 1 == refps.size()) थांबा;
            // A middle segment that resolved to a node with no reachable module
            // (e.g. a typedef alias of a not-yet-specialized parameterized class)
            // gives nothing to look the next segment up in.
            Ast_गाठ_Module* स्थिरांक modp = refp->classOrPackageSkipp();
            जर (!modp) {
                refp->v3warn(E_UNSUPPORTED, "Unsupported: Multiple '::' package/class reference");
                परत असत्य;
            }
            scopeSymp = m_statep->get_गाठ_Sym(modp);
        }
        AstClassOrPackageRef* स्थिरांक innerp = refps.back();
        innerp->unlinkFrBack();
        VL_DO_DANGLING(ढकला_Deletep(scopeDotp->unlinkFrBack()), scopeDotp);
        nodep->classOrPackageOpp(innerp);
        परत सत्य;
    }

    शून्य visit(AstRefDType* nodep) override {

        जर (स्वतः* स्थिरांक typeOfp = nodep->typeofp()) {
            iterate(typeOfp);
            // After iteration, typeofp() may have been replaced with a resolved type
            // (e.g., DOT expression like if0.rq_t resolved to REFDTYPE)
            // If it's now a NodeDType, replace this RefDType with it
            जर (Ast_गाठ_DType* स्थिरांक resolvedDTypep = VN_CAST(nodep->typeofp(), गाठ_DType)) {
                UINFO(9,
                      indent() << "iface capture typeofp resolved to dtype, replacing RefDType\n");
                resolvedDTypep->unlinkFrBack();
                nodep->replaceWith(resolvedDTypep);
                VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
                // Originally (PR #6637) this block re-captured a resolved
                // RefDType into the iface-capture ledger when user2p held a
                // Cell, to ensure re-resolution during the paramed pass.
                // In practice, the typeofp-resolved RefDType never carries a
                // Cell in user2p: the capture happens earlier when the
                // typedef is first linked.
                जर (V3LinkDotIfaceCapture::enabled()) {
                    जर (AstRefDType* स्थिरांक resolvedRefp = VN_CAST(resolvedDTypep, RefDType)) {
                        जर (VL_UNCOVERABLE(
                                VN_IS(resolvedRefp->user2p(), Cell))) {  // LCOV_EXCL_LINE
                            resolvedRefp->v3fatalSrc(  // LCOV_EXCL_LINE
                                "typeofp resolved RefDType has Cell in user2p;"
                                " expected to be captured already");
                        }
                    }
                }
            }
            परत;
        }

        // Resolve its reference
        जर (m_statep->forParamed() && nodep->user3()) {
            जर (V3LinkDotIfaceCapture::enabled() && nodep->user2p()) {
                UINFO(9, indent() << "iface capture clear user3 for captured typedef name="
                                  << nodep->name() << " cell=" << nodep->user2p());
            }
            nodep->user3(असत्य);
        }
        जर (nodep->user3SetOnce()) {
            जर (V3LinkDotIfaceCapture::enabled() && nodep->user2p()) {
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
            जर (VL_UNCOVERABLE(m_statep->forParamed() && nodep->classOrPackageOpp())) {
                nodep->v3fatalSrc(  // LCOV_EXCL_LINE
                    "RefDType user3 revisit with classOrPackageOpp in paramed pass");
            }
            परत;
        }
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        जर (nodep->classOrPackageOpp()) {
            // Resolve a chained scope (`pkg::cls::t`, `pkg::cls#(P)::t`) from outside in.
            // On failure reduceScopeDot has reported, so stop before using the operand.
            जर (AstDot* स्थिरांक scopeDotp = VN_CAST(nodep->classOrPackageOpp(), Dot)) {
                जर (!reduceScopeDot(nodep, scopeDotp)) परत;
            }
            // Re-read: reduceScopeDot may have replaced the operand
            Ast_गाठ* स्थिरांक cpackagep = nodep->classOrPackageOpp();
            UASSERT_OBJ(!VN_IS(cpackagep, Dot), cpackagep,
                        "Package/class scope operand should be reduced by now");
            जर (AstClassOrPackageRef* स्थिरांक cpackagerefp = VN_CAST(cpackagep, ClassOrPackageRef)) {
                iterate(cpackagerefp);
                स्थिरांक AstClass* स्थिरांक clsp = VN_CAST(cpackagerefp->classOrPackageNodep(), Class);
                जर (clsp && clsp->hasGParam()) {
                    // Unable to link before the instantiation of parameter classes.
                    // The class reference node still has to be visited now to later link
                    // parameters.
                    iterate(cpackagep);
                    परत;
                }
                // Defer non-typedef references through typedef aliases of parameterized classes.
                जर (m_statep->forPrimary() && !VN_IS(nodep->backp(), Typedef)
                    && isParamedClassRef(cpackagerefp)) {
                    iterate(cpackagep);
                    परत;
                }

                स्थिरांक तार्किक doDefaultTypedef = !(m_resolvingTypedef && m_statep->forPrimary());
                जर (!cpackagerefp->classOrPackageSkipp(doDefaultTypedef)
                    && !cpackagerefp->classOrPackageNodep()) {
                    VSymEnt* स्थिरांक foundp = m_statep->resolveClassOrPackage(
                        m_ds.m_dotSymp, cpackagerefp, सत्य, असत्य, "class/package reference");
                    जर (!foundp) परत;
                }
                nodep->classOrPackagep(cpackagerefp->classOrPackageSkipp(doDefaultTypedef));
                जर (!VN_IS(nodep->classOrPackagep(), Class)
                    && !VN_IS(nodep->classOrPackagep(), Package)) {
                    जर (m_statep->forPrimary()) {
                        // It may be a type that comes from parameter class that is not
                        // instantioned yet
                        iterate(cpackagep);
                        परत;
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
            VL_DO_DANGLING(ढकला_Deletep(cpackagep->unlinkFrBack()), cpackagep);
        }

        जर (m_ds.m_dotp && (m_ds.m_dotPos == DP_PACKAGE || m_ds.m_dotPos == DP_SCOPE)) {
            UASSERT_OBJ(VN_IS(m_ds.m_dotp->lhsp(), ClassOrPackageRef), m_ds.m_dotp->lhsp(),
                        "Bad package link");
            स्वतः* स्थिरांक cpackagerefp = VN_CAST(m_ds.m_dotp->lhsp(), ClassOrPackageRef);
            UASSERT_OBJ(cpackagerefp->classOrPackageSkipp(), m_ds.m_dotp->lhsp(),
                        "Bad package link");
            nodep->classOrPackagep(cpackagerefp->classOrPackageSkipp());
            m_ds.m_dotPos = DP_SCOPE;
        } अन्यथा {
            // Allow REFDTYPE under DOT when referencing interface typedefs
            // This is needed for patterns like: typedef iface.a_t a_t;
            // The dependency graph handles resolution when IfaceCapture is disabled
            स्थिरांक तार्किक ifaceFinalSegmentAllowed
                = (m_ds.m_dotPos == DP_FINAL) && m_ds.m_dotSymp
                  && VN_IS(m_ds.m_dotSymp->nodep(), Cell)
                  && VN_CAST(m_ds.m_dotSymp->nodep(), Cell)->modp()
                  && VN_IS(VN_CAST(m_ds.m_dotSymp->nodep(), Cell)->modp(), Iface);
            जर (!ifaceFinalSegmentAllowed) {
                checkNoDot(nodep);
            } अन्यथा {
                UINFO(9, indent() << "allow REFDTYPE under DOT for iface typedef name="
                                  << nodep->prettyNameQ() << " dotSym=" << m_ds.m_dotSymp);
                // Clear the dot state so we don't propagate errors
                m_ds.m_dotPos = DP_SCOPE;
            }
        }
        जर (!nodep->typedefp() && !nodep->subDTypep()) {
            स्थिरांक VSymEnt* foundp;
            जर (nodep->classOrPackagep()) {
                foundp = m_statep->get_गाठ_Sym(nodep->classOrPackagep())->findIdFlat(nodep->name());
            } अन्यथा जर (m_ds.m_dotPos == DP_FIRST || m_ds.m_dotPos == DP_NONE) {
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
                जर (AstTypedef* स्थिरांक defp
                    = foundp ? VN_CAST(foundp->nodep(), Typedef) : nullptr) {
                    // Don't check if typedef is to a <type T>::<reference> as might not be
                    // resolved yet
                    जर (!nodep->classOrPackagep() && !defp->isUnderClass())
                        checkDeclOrder(nodep, defp);
                    nodep->typedefp(defp);
                    nodep->classOrPackagep(foundp->classOrPackagep());

                } अन्यथा जर (AstParamTypeDType* स्थिरांक defp
                           = foundp ? VN_CAST(foundp->nodep(), ParamTypeDType) : nullptr) {
                    // A PARAMTYPEDTYPE's child REFDTYPE referencing itself is the normal
                    // AST structure for type parameters (e.g., "type T = base" has a child
                    // REFDTYPE for the default type). Only error on true recursion where
                    // a TYPEDEF contains a reference back to itself.
                    स्थिरांक तार्किक isParamTypeChild = (defp == nodep->backp());
                    स्थिरांक तार्किक isTypedefRecursion
                        = isParamTypeChild && VN_IS(defp->backp(), Typedef);
                    जर (isTypedefRecursion) {
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
                } अन्यथा जर (AstClass* स्थिरांक defp
                           = foundp ? VN_CAST(foundp->nodep(), Class) : nullptr) {
                    // Don't check if typedef is to a <type T>::<reference> as might not be
                    // resolved yet
                    जर (!nodep->classOrPackagep()) checkDeclOrder(nodep, defp);
                    AstPin* स्थिरांक paramsp = nodep->paramsp();
                    जर (paramsp) paramsp->unlinkFrBackWithNext();
                    AstClassRefDType* स्थिरांक newp
                        = नवीन AstClassRefDType{nodep->fileline(), defp, paramsp};
                    newp->classOrPackagep(foundp->classOrPackagep());
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
                    परत;
                } अन्यथा जर (m_insideClassExtParam) {
                    परत;
                } अन्यथा {
                    जर (foundp) {
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
    शून्य visit(AstRequireDType* nodep) override {
        // Handle type validation for localparam type assignments
        // Iterate child to resolve any ParseRef nodes
        iterateChildren(nodep);
        // Only emit error if the child is not a type.
        // Do NOT unwrap valid types here - leave that to V3Width.
        // Unwrapping here breaks type parameter resolution during cloning.
        // Allow Dot through to defer the handling until the dot expression is resolved.
        जर (nodep->lhsp() && !VN_IS(nodep->lhsp(), गाठ_DType) && !VN_IS(nodep->lhsp(), Dot)) {
            // Not a type - emit error
            जर (VN_IS(nodep->lhsp(), Const)) {
                nodep->lhsp()->v3error("Expecting a data type, not a constant");
            } अन्यथा {
                nodep->lhsp()->v3error("Expecting a data type, not "
                                       << nodep->lhsp()->typeName() << ": '"
                                       << nodep->lhsp()->prettyName() << "'");
            }
            // Replace with void to keep AST valid and allow clean exit
            nodep->replaceWith(नवीन AstVoidDType{nodep->fileline()});
            VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
        }
    }
    शून्य visit(AstDpiExport* nodep) override {
        // AstDpiExport: Make sure the function referenced exists, then dump it
        LINKDOT_VISIT_START();
        iterateChildren(nodep);
        checkNoDot(nodep);
        VSymEnt* स्थिरांक foundp = m_curSymp->findIdFallback(nodep->name());
        Ast_गाठ_FTask* स्थिरांक taskp = foundp ? VN_AS(foundp->nodep(), गाठ_FTask) : nullptr;
        जर (!taskp) {
            nodep->v3error(
                "Can't find definition of exported task/function: " << nodep->prettyNameQ());
        } अन्यथा जर (taskp->dpiExport()) {
            nodep->v3error("Function was already DPI Exported, duplicate not allowed: "
                           << nodep->prettyNameQ());
        } अन्यथा {
            taskp->dpiExport(सत्य);
            जर (nodep->cname() != "") taskp->cname(nodep->cname());
        }
        VL_DO_DANGLING(ढकला_Deletep(nodep->unlinkFrBack()), nodep);
    }
    शून्य visit(AstDisable* nodep) override {
        LINKDOT_VISIT_START();
        checkNoDot(nodep);
        तार्किक hasPartSelect = असत्य;
        स्थिरांक string targetPath
            = nodep->targetRefp() ? extractDottedPath(nodep->targetRefp(), hasPartSelect) : "";
        VL_RESTORER_COPY(m_ds);
        m_ds.init(m_curSymp);
        m_ds.m_dotPos = DP_FIRST;
        m_ds.m_disablep = nodep;
        iterateChildren(nodep);
        जर (nodep->targetRefp()) {
            जर (AstTaskRef* स्थिरांक taskRefp = VN_CAST(nodep->targetRefp(), TaskRef)) {
                nodep->targetp(taskRefp->taskp());
            } अन्यथा जर (!VN_IS(nodep->targetRefp(), ParseRef)) {
                // If it is a ParseRef, either it couldn't be linked or it is linked to a block
                nodep->v3warn(E_UNSUPPORTED, "Node of type "
                                                 << nodep->targetRefp()->prettyTypeName()
                                                 << " referenced by disable");
                VL_DO_DANGLING(ढकला_Deletep(nodep->unlinkFrBack()), nodep);
                परत;
            }
            जर (nodep->targetp()) {
                nodep->targetRefp()->unlinkFrBack()->delete_वृक्ष();
                जर (!hasPartSelect) {
                    स्थिरांक string instancePath = targetInstancePath(nodep->targetp(), targetPath);
                    जर (!instancePath.empty()) {
                        // Keep only the instance prefix so V3LinkJump can reference the selected
                        // instance's process queue without reparsing the disable target.
                        nodep->targetRefp(
                            नवीन AstVarXRef{nodep->fileline(), "", instancePath, VAccess::READ});
                    }
                }
            }
        }
    }
    शून्य visitPackageImportOrExport(Ast_गाठ* nodep) {
        // No longer needed
        LINKDOT_VISIT_START();
        checkNoDot(nodep);
        जर (m_statep->forParamed()) VL_DO_DANGLING(ढकला_Deletep(nodep->unlinkFrBack()), nodep);
    }
    शून्य visit(AstPackageImport* nodep) override { visitPackageImportOrExport(nodep); }
    शून्य visit(AstPackageExport* nodep) override { visitPackageImportOrExport(nodep); }
    शून्य visit(AstPackageExportStarStar* nodep) override { visitPackageImportOrExport(nodep); }
    शून्य visit(AstCellRef* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        iterateChildren(nodep);
    }
    शून्य visit(AstCellArrayRef* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        // Expression already iterated
    }
    शून्य visit(AstUnlinkedRef* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        // No need to iterate, if we have a UnlinkedVarXRef, we're already done
    }
    शून्य visit(Ast_Stmt_अभिव्यक्ती* nodep) override {
        LINKDOT_VISIT_START();
        checkNoDot(nodep);
        // Check if nodep represents a super.new call;
        जर (AstNew* स्थिरांक newExprp = VN_CAST(nodep->exprp(), New)) {
            // in this case it was already linked, so it doesn't have a super reference
            m_explicitSuperNewp = newExprp;
        } अन्यथा जर (स्थिरांक AstDot* स्थिरांक dotp = VN_CAST(nodep->exprp(), Dot)) {
            जर (dotp->lhsp()->name() == "super") {
                जर (AstNew* स्थिरांक newExprp = VN_CAST(dotp->rhsp(), New)) {
                    m_explicitSuperNewp = newExprp;
                }
            }
        }
        iterateChildren(nodep);
    }

    शून्य visit(AstIfaceRefDType* nodep) override {
        जर (nodep->user3SetOnce()) परत;
        LINKDOT_VISIT_START();
        जर (nodep->paramsp()) {
            // If there is no parameters, there is no need to visit this node.
            AstIface* स्थिरांक ifacep = nodep->ifacep();
            UASSERT_OBJ(ifacep, nodep, "Port parameters of AstIfaceRefDType without ifacep()");
            जर (ifacep->dead()) परत;
            checkNoDot(nodep);
            VL_RESTORER_CLEAR(m_usedPins);
            VL_RESTORER_CLEAR(m_usedDefParamPins);
            VL_RESTORER(m_pinSymp);
            m_pinSymp = m_statep->get_गाठ_Sym(ifacep);
            iterateAndNextNull(nodep->paramsp());
        }
    }

    शून्य visit(AstAttrOf* nodep) override { iterateChildren(nodep); }
    शून्य visit(AstCastParse* nodep) override {
        जर (nodep->user3SetOnce()) परत;
        LINKDOT_VISIT_START();
        // A cast may start a dotted expression: HERE type'(expr) . member_or_method
        स्थिरांक तार्किक dotLhs = m_ds.m_dotPos == DP_FIRST;
        जर (!dotLhs) checkNoDot(nodep);
        {
            VL_RESTORER_COPY(m_ds);
            m_ds.init(m_curSymp);
            iterateChildren(nodep);
        }
        जर (dotLhs) m_ds.m_dotPos = DP_MEMBER;
    }

    शून्य visit(AstAssignW* nodep) override {
        LINKDOT_VISIT_START();
        checkNoDot(nodep);
        VL_RESTORER(m_replaceWithAlias);
        जर (nodep->user2()) m_replaceWithAlias = असत्य;
        iterateChildren(nodep);
    }

    शून्य visit(AstTypedef* nodep) override {
        VL_RESTORER(m_resolvingTypedef)
        m_resolvingTypedef = सत्य;
        iterateChildren(nodep);
    }

    शून्य visit(Ast_गाठ* nodep) override {
        VL_RESTORER(m_inPackedArray);
        जर (VN_IS(nodep, PackArrayDType)) {
            m_inPackedArray = सत्य;
        } अन्यथा जर (!m_inPackedArray) {
            LINKDOT_VISIT_START();
            checkNoDot(nodep);
        }
        iterateChildren(nodep);
    }

सार्वजनिक:
    // CONSTRUCTORS
    LinkDotResolveVisitor(AstNetlist* rootp, LinkDotState* statep)
        : m_statep{statep} {
        UINFO(4, __FUNCTION__ << ": ");

        // Note: no need to clear user3 on ledger entries here.
        // VNUser3InUse (m_inuser3) already logically clears user3 on all
        // nodes via the generation counter.  The ledger may also hold stale
        // refp pointers to deleted AST nodes, so iterating it is unsafe.

        iterate(rootp);
        स्थिरांक std::map<std::string, Ast_गाठ_Module*> modulesToRevisit
            = std::move(m_modulesToRevisit);
        m_lastDeferredp = nullptr;
        साठी (स्वतः& p : modulesToRevisit) {
            Ast_गाठ_Module* स्थिरांक modp = p.second;
            modp->foreach([](Ast_गाठ* स्थिरांक nodep) { nodep->user3(असत्य); });
            iterate(modp);
        }
    }
    ~LinkDotResolveVisitor() override = पूर्वनिर्धारित;
};

//######################################################################
// Link class functions

शून्य V3LinkDot::dumpSubstep(स्थिरांक string& name) {
    जर (dump_वृक्ष_Either_Level() >= 9) {
        V3Global::dump_Check_Global_वृक्ष(name);
    } अन्यथा जर (debug() >= 5) {  // on high dbg level, dump even if not explicitly told to
        v3Global.rootp()->dump_वृक्ष_File(v3Global.debugFilename(name + ".tree"));
    }
}

शून्य V3LinkDot::linkDotGuts(AstNetlist* rootp, VLinkDotStep step) {
    VIsCached::clear_Cache_वृक्ष();  // Avoid using any stale isPure
    dumpSubstep("prelinkdot");
    LinkDotState state{rootp, step};

    { LinkDotFindVisitor{rootp, &state}; }
    dumpSubstep("prelinkdot-find");

    { LinkDotFindIfaceVisitor{rootp, &state}; }
    dumpSubstep("prelinkdot-findiface");

    जर (step == LDS_SCOPED) {
        // Well after the initial link when we're ready to operate on the flat design,
        // process AstScope's.  This needs to be separate pass after whole hierarchy graph created.
        { LinkDotScopeVisitor{rootp, &state}; }
        v3Global.assertScoped(सत्य);
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
    स्थिरांक LinkDotResolveVisitor visitor{rootp, &state};
    state.dumpSelf("linkdot-done");
}

शून्य V3LinkDot::linkDotPrimary(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    linkDotGuts(nodep, LDS_PRIMARY);
    V3Global::dump_Check_Global_वृक्ष("linkdot", 0, dump_वृक्ष_Either_Level() >= 6);
}

शून्य V3LinkDot::linkDotParamed(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    linkDotGuts(nodep, LDS_PARAMED);
    V3Global::dump_Check_Global_वृक्ष("linkdotparam", 0, dump_वृक्ष_Either_Level() >= 3);
}

शून्य V3LinkDot::linkDotScope(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    linkDotGuts(nodep, LDS_SCOPED);
    V3Global::dump_Check_Global_वृक्ष("linkdot", 0, dump_वृक्ष_Either_Level() >= 3);
}
