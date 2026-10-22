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
//శిఖరం
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

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3LinkDot.h"

#అన్తర్భావ "V3Global.h"
#అన్తర్భావ "V3Graph.h"
#అన్తర్భావ "V3LinkDotIfaceCapture.h"
#అన్తర్భావ "V3MemberMap.h"
#అన్తర్భావ "V3Parse.h"
#అన్తర్భావ "V3Randomize.h"
#అన్తర్భావ "V3Stats.h"
#అన్తర్భావ "V3String.h"
#అన్తర్భావ "V3SymTable.h"

#అన్తర్భావ <algorithm>
#అన్తర్భావ <cstdlib>
#అన్తర్భావ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

స్థిర string వెలికితీయి_Dotted_మార్గం(Ast_కణుపు* nodep, తార్కిక& hasPartSelect) {
    ఒకవేళ (AstParseRef* స్థిరరాశి refp = VN_CAST(nodep, ParseRef)) {
        తిరిగి refp->name();
    } లేకపోతే ఒకవేళ (AstVarRef* స్థిరరాశి refp = VN_CAST(nodep, VarRef)) {
        తిరిగి refp->name();
    } లేకపోతే ఒకవేళ (AstDot* స్థిరరాశి dotp = VN_CAST(nodep, Dot)) {
        స్థిరరాశి string lhs = వెలికితీయి_Dotted_మార్గం(dotp->lhsp(), hasPartSelect);
        స్థిరరాశి string rhs = వెలికితీయి_Dotted_మార్గం(dotp->rhsp(), hasPartSelect);
        తిరిగి VString::dot(lhs, ".", rhs);
    } లేకపోతే ఒకవేళ (VN_IS(nodep, Sel_బిట్టు) || VN_IS(nodep, Sel_వెలికితీయి) || VN_IS(nodep, SelPlus)
               || VN_IS(nodep, SelMinus)) {
        hasPartSelect = నిజం;
        తిరిగి "";
    }
    తిరిగి "";
}
స్థిర string lexical_Disable_మార్గం(Ast_కణుపు* స్థిరరాశి targetp) {
    std::సదిశ<string> పేర్లు;
    కొరకు (Ast_కణుపు* curp = targetp; curp; curp = curp->aboveLoopp()) {
        ఒకవేళ (Ast_కణుపు_ఖండం* స్థిరరాశి blockp = VN_CAST(curp, కణుపు_ఖండం)) {
            ఒకవేళ (blockp->name() != "") పేర్లు.నెట్టు_వెనుక(blockp->name());
        } లేకపోతే ఒకవేళ (Ast_కణుపు_FTask* స్థిరరాశి ftaskp = VN_CAST(curp, కణుపు_FTask)) {
            పేర్లు.నెట్టు_వెనుక(ftaskp->name());
        } లేకపోతే ఒకవేళ (VN_IS(curp, కణుపు_Module)) {
            ఆపు;
        }
    }
    string మార్గం;
    కొరకు (స్వయంచాలక it = పేర్లు.crbegin(); it != పేర్లు.crend(); ++it) {
        మార్గం = VString::dot(మార్గం, ".", *it);
    }
    తిరిగి మార్గం;
}
స్థిర string లక్ష్యం_Instance_మార్గం(Ast_కణుపు* స్థిరరాశి targetp, స్థిరరాశి string& లక్ష్యం_మార్గం) {
    స్థిరరాశి string lexical_మార్గం = lexical_Disable_మార్గం(targetp);
    ఒకవేళ (lexical_మార్గం == "" || లక్ష్యం_మార్గం == lexical_మార్గం) తిరిగి "";
    స్థిరరాశి string suffix = "." + lexical_మార్గం;
    ఒకవేళ (!VString::చివర్లు_With(లక్ష్యం_మార్గం, suffix)) తిరిగి "";
    తిరిగి లక్ష్యం_మార్గం.substr(0, లక్ష్యం_మార్గం.కొలత() - suffix.కొలత());
}

// ######################################################################
//  Matcher classes (for suggestion matching)

తరగతి Link_కణుపు_Matcher_Class_Or_Package final : బహిరంగ VNodeMatcher {
బహిరంగ:
    తార్కిక కణుపు_సరిపోలిక(స్థిరరాశి Ast_కణుపు* nodep) స్థిరరాశి override {
        తిరిగి VN_IS(nodep, Class) || VN_IS(nodep, Package);
    }
};
తరగతి Link_కణుపు_Matcher_FTask final : బహిరంగ VNodeMatcher {
బహిరంగ:
    తార్కిక కణుపు_సరిపోలిక(స్థిరరాశి Ast_కణుపు* nodep) స్థిరరాశి override { తిరిగి VN_IS(nodep, కణుపు_FTask); }
};
తరగతి Link_కణుపు_Matcher_Modport final : బహిరంగ VNodeMatcher {
బహిరంగ:
    తార్కిక కణుపు_సరిపోలిక(స్థిరరాశి Ast_కణుపు* nodep) స్థిరరాశి override { తిరిగి VN_IS(nodep, Modport); }
};
తరగతి Link_కణుపు_Matcher_Prod final : బహిరంగ VNodeMatcher {
బహిరంగ:
    తార్కిక కణుపు_సరిపోలిక(స్థిరరాశి Ast_కణుపు* nodep) స్థిరరాశి override { తిరిగి VN_IS(nodep, RSProd); }
};
తరగతి Link_కణుపు_Matcher_Var final : బహిరంగ VNodeMatcher {
బహిరంగ:
    తార్కిక కణుపు_సరిపోలిక(స్థిరరాశి Ast_కణుపు* nodep) స్థిరరాశి override {
        తిరిగి VN_IS(nodep, Var) || VN_IS(nodep, LambdaArgRef);
    }
};
తరగతి Link_కణుపు_Matcher_Var_IO final : బహిరంగ VNodeMatcher {
బహిరంగ:
    తార్కిక కణుపు_సరిపోలిక(స్థిరరాశి Ast_కణుపు* nodep) స్థిరరాశి override {
        స్థిరరాశి AstVar* స్థిరరాశి varp = VN_CAST(nodep, Var);
        ఒకవేళ (!varp) తిరిగి అబద్ధం;
        తిరిగి varp->isIO();
    }
};
తరగతి Link_కణుపు_Matcher_Var_Param final : బహిరంగ VNodeMatcher {
బహిరంగ:
    తార్కిక కణుపు_సరిపోలిక(స్థిరరాశి Ast_కణుపు* nodep) స్థిరరాశి override {
        స్థిరరాశి AstVar* స్థిరరాశి varp = VN_CAST(nodep, Var);
        ఒకవేళ (!varp) తిరిగి అబద్ధం;
        తిరిగి varp->isParam();
    }
};

//######################################################################
// LinkDot state, as a visitor of each AstNode

తరగతి LinkDotState final {
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
    స్థిరరాశి VNUser1_లోపల_Use m_inuser1;
    స్థిరరాశి VNUser2_లోపల_Use m_inuser2;
    స్థిరరాశి VNUser4_లోపల_Use m_inuser4;

బహిరంగ:
    // ENUMS
    // In order of priority, compute first ... compute last
    గణన SAMNum : uint8_t { SAMN_MODPORT, SAMN_IFTOP, SAMN__MAX };  // Values for m_scopeAliasMap

అంతరంగ:
    // TYPES
    ఉపయోగించి ScopeAliasMap = std::unordered_map<VSymEnt*, VSymEnt*>;
    ఉపయోగించి IfaceModSyms = std::సదిశ<std::pair<AstIface*, VSymEnt*>>;

    స్థిర LinkDotState* s_errorThisp;  // Last self, for error reporting only

    // MEMBERS
    VSym_లేఖాచిత్రం m_syms;  // Symbol table by hierarchy
    VSym_లేఖాచిత్రం m_mods;  // Symbol table of all module names
    VSymEnt* m_dunitEntp = శూన్యసూచిక;  // $unit entry
    std::multimap<std::string, VSymEnt*>
        m_nameScopeSymMap;  // Map of scope referenced by non-pretty textual name
    std::సమితి<std::pair<Ast_కణుపు_Module*, std::string>>
        m_implicit_Name_సమితి;  // For [module][signalname] if we can implicitly create it
    std::array<ScopeAliasMap, SAMN__MAX> m_scopeAliasMap;  // Map of <lhs,rhs> aliases
    std::సదిశ<VSymEnt*> m_ifaceVarSyms;  // List of AstIfaceRefDType's to be imported
    IfaceModSyms m_ifaceModSyms;  // List of AstIface+Symbols to be processed
    స్థిరరాశి VLink_Dot_అడుగు m_అడుగు;  // Operational step

బహిరంగ:
    // METHODS

    శూన్య dumpSelf(స్థిరరాశి string& nameComment, తార్కిక force = అబద్ధం) {
        ఒకవేళ (debug() >= 6 || dumpLevel() >= 6 || force) {
            స్థిరరాశి string filename = v3Global.debugFilename(nameComment) + ".txt";
            UINFO(4, "Dumping " << filename);
            స్థిరరాశి std::unique_ptr<std::ofstream> logp{V3File::new_ofstream(filename)};
            ఒకవేళ (logp->fail()) v3fatal("Can't write file: " << filename);
            std::ostream& os = *logp;
            // TODO the symbol table has node pointers which may be
            // dangling, as we call deleteTree in these visitors without
            // pushing for later deletion (or deleting from the symbol table)
            // So, only completely safe to call this when under --debug or --debug-leak.
            m_syms.dumpSelf(os);
            తార్కిక మొదటి = నిజం;
            కొరకు (పూర్ణాంకం samn = 0; samn < SAMN__MAX; ++samn) {
                ఒకవేళ (!m_scopeAliasMap[samn].ఖాళీలు()) {
                    ఒకవేళ (మొదటి) os << "\nScopeAliasMap:\n";
                    మొదటి = అబద్ధం;
                    కొరకు (ScopeAliasMap::iterator it = m_scopeAliasMap[samn].begin();
                         it != m_scopeAliasMap[samn].చివర(); ++it) {
                        // left side is what we will import into
                        os << "\t" << samn << "\t" << it->మొదటి << " ("
                           << it->మొదటి->nodep()->typeName() << ") <- " << it->రెండవ << " "
                           << it->రెండవ->nodep() << '\n';
                    }
                }
            }
        }
    }
    స్థిర శూన్య preErrorDumpHandler() {
        ఒకవేళ (s_errorThisp) s_errorThisp->preErrorDump();
    }
    శూన్య preErrorDump() {
        స్థిర తార్కిక s_diddump = అబద్ధం;
        ఒకవేళ (!s_diddump && dump_వృక్షం_Level()) {
            s_diddump = నిజం;
            dumpSelf("linkdot-preerr", నిజం);
            v3Global.rootp()->dump_వృక్షం_File(v3Global.debugFilename("linkdot-preerr.tree"));
        }
    }

    // CONSTRUCTORS
    LinkDotState(AstNetlist* rootp, VLink_Dot_అడుగు అడుగు)
        : m_syms{rootp}
        , m_mods{rootp}
        , m_అడుగు{అడుగు} {
        UINFO(4, __FUNCTION__ << ": ");
        s_errorThisp = ఇది;
        V3Error::errorExitCb(preErrorDumpHandler);  // If get error, dump self
        read_Mod_పేర్లు();
    }
    ~LinkDotState() {
        V3Error::errorExitCb(శూన్యసూచిక);
        s_errorThisp = శూన్యసూచిక;
    }

    // ACCESSORS
    VSym_లేఖాచిత్రం* symsp() { తిరిగి &m_syms; }
    పూర్ణాంకం అడుగు_సంఖ్య() స్థిరరాశి { తిరిగి static_cast<పూర్ణాంకం>(m_అడుగు); }
    తార్కిక forPrimary() స్థిరరాశి { తిరిగి m_అడుగు == LDS_PRIMARY; }
    తార్కిక forParamed() స్థిరరాశి { తిరిగి m_అడుగు == LDS_PARAMED; }
    తార్కిక forScopeCreation() స్థిరరాశి { తిరిగి m_అడుగు == LDS_SCOPED; }

    // METHODS
    స్థిర string కణుపు_పాఠ్యం_Type(Ast_కణుపు* nodep) {
        ఒకవేళ (స్థిరరాశి AstVar* స్థిరరాశి varp = VN_CAST(nodep, Var)) {
            ఒకవేళ (varp->isIO() || varp->isIfaceRef()) {
                తిరిగి "port";
            } లేకపోతే ఒకవేళ (varp->isGParam()) {
                తిరిగి "parameter";
            } లేకపోతే ఒకవేళ (varp->varType() == VVarType::LPARAM) {
                తిరిగి "local parameter";
            } లేకపోతే {
                తిరిగి "variable";
            }
        } లేకపోతే ఒకవేళ (స్థిరరాశి AstParamTypeDType* స్థిరరాశి dtypep = VN_CAST(nodep, ParamTypeDType)) {
            ఒకవేళ (dtypep->isGParam()) {
                తిరిగి "type parameter";
            } లేకపోతే {
                తిరిగి "local type parameter";
            }
        } లేకపోతే ఒకవేళ (VN_IS(nodep, Begin)) {
            తిరిగి "block";
        } లేకపోతే ఒకవేళ (VN_IS(nodep, Cell)) {
            తిరిగి "instance";
        } లేకపోతే ఒకవేళ (VN_IS(nodep, Constraint)) {
            తిరిగి "constraint";
        } లేకపోతే ఒకవేళ (VN_IS(nodep, Func)) {
            తిరిగి "function";
        } లేకపోతే ఒకవేళ (VN_IS(nodep, Gen_ఖండం)) {
            తిరిగి "generate block";
        } లేకపోతే ఒకవేళ (VN_IS(nodep, Iface)) {
            తిరిగి "interface";
        } లేకపోతే ఒకవేళ (VN_IS(nodep, RSProd)) {
            తిరిగి "randsequence production";
        } లేకపోతే ఒకవేళ (VN_IS(nodep, Task)) {
            తిరిగి "task";
        } లేకపోతే {
            తిరిగి nodep->prettyTypeName();
        }
    }

    Ast_కణుపు_Module* కనుగొను_Module_Sym(స్థిరరాశి string& modName) {
        స్థిరరాశి VSymEnt* స్థిరరాశి foundp = m_mods.rootp()->కనుగొను_Id_Fallback(modName);
        తిరిగి foundp ? VN_AS(foundp->nodep(), కణుపు_Module) : శూన్యసూచిక;
    }
    శూన్య read_Mod_పేర్లు() {
        // Look at all modules, and store pointers to all module names
        కొరకు (Ast_కణుపు_Module *nextp, *nodep = v3Global.rootp()->modulesp(); nodep; nodep = nextp) {
            nextp = VN_AS(nodep->nextp(), కణుపు_Module);
            m_mods.rootp()->చొప్పించు(nodep->name(), కొత్త VSymEnt{&m_mods, nodep});
        }
    }

    VSymEnt* మూలం_Entp() స్థిరరాశి { తిరిగి m_syms.rootp(); }
    VSymEnt* dunitEntp() స్థిరరాశి { తిరిగి m_dunitEntp; }
    శూన్య checkDuplicate(VSymEnt* lookupSymp, Ast_కణుపు* nodep, స్థిరరాశి string& name) {
        // Lookup the given name under current symbol table
        // Insert if not found
        // Report error if there's a duplicate
        //
        // Note we only check for conflicts at the same level; it's ok if one block hides another
        // We also wouldn't want to not insert it even though it's lower down

        స్థిరరాశి VSymEnt* స్థిరరాశి foundp = lookupSymp->కనుగొను_Id_Flat(name);
        Ast_కణుపు* స్థిరరాశి fnodep = foundp ? foundp->nodep() : శూన్యసూచిక;
        ఒకవేళ (!fnodep) {
            // Not found, will add in a moment.
            ఒకవేళ (forPrimary() && VSymEnt::checkSimilarname(nodep)) {
                స్థిరరాశి VSymEnt* స్థిరరాశి alt = lookupSymp->కనుగొను_Similar_Id_Flat(name);
                ఒకవేళ (alt) {
                    nodep->v3warn(SIMILARNAME, "Declaration overlaps another with different case: "
                                                   << nodep->prettyNameQ() << '\n'
                                                   << nodep->warnContextPrimary() << '\n'
                                                   << alt->nodep()->warnOther()
                                                   << "... Location of original declaration\n"
                                                   << alt->nodep()->warnContextSecondary());
                }
            }
        } లేకపోతే ఒకవేళ (nodep == fnodep) {  // Already inserted.
            // Good.
        } లేకపోతే ఒకవేళ (foundp->imported()) {  // From package
            // We don't throw VARHIDDEN as if the import is later the symbol
            // table's import wouldn't warn
        } లేకపోతే ఒకవేళ (forPrimary() && VN_IS(nodep, Gen_ఖండం)
                   && (VN_IS(fnodep, Begin) || VN_IS(fnodep, Gen_ఖండం))) {
            // Begin: ... blocks often replicate under genif/genfor, so
            // suppress duplicate checks.  See t_gen_forif.v for an example.
        } లేకపోతే {
            UINFO(4, "name " << name);  // Not always same as nodep->name
            UINFO(4, "Var1 " << nodep);
            UINFO(4, "Var2 " << fnodep);
            // Quote the node's own name as for messages, if it is the symbol's
            స్థిరరాశి string nameQ
                = fnodep->name() == name ? fnodep->prettyNameQ() : Ast_కణుపు::prettyNameQ(name);
            ఒకవేళ (nodep->type() == fnodep->type()) {
                nodep->v3error("Duplicate declaration of "
                               << కణుపు_పాఠ్యం_Type(fnodep) << ": " << nameQ << '\n'
                               << nodep->warnContextPrimary() << '\n'
                               << fnodep->warnOther() << "... Location of original declaration\n"
                               << fnodep->warnContextSecondary());
                nodep->decl_Token_అంకె_సమితి_Min(0);  // Disable checking forward typedefs
                fnodep->decl_Token_అంకె_సమితి_Min(0);  // Disable checking forward typedefs
            } లేకపోతే {
                nodep->v3error("Unsupported in C: "
                               << ucfirst(కణుపు_పాఠ్యం_Type(nodep)) << " has the same name as "
                               << కణుపు_పాఠ్యం_Type(fnodep) << ": " << nameQ << '\n'
                               << nodep->warnContextPrimary() << '\n'
                               << fnodep->warnOther() << "... Location of original declaration\n"
                               << fnodep->warnContextSecondary());
            }
        }
    }
    శూన్య చొప్పించు_DUnit(AstNetlist* nodep) {
        // $unit on top scope
        VSymEnt* స్థిరరాశి symp = కొత్త VSymEnt{&m_syms, nodep};
        UINFO(9, "      INSERTdunit se" << cvt_గమ్యం_Hex(symp));
        symp->parentp(మూలం_Entp());  // Needed so backward search can find name of top module
        symp->fallbackp(శూన్యసూచిక);
        మూలం_Entp()->చొప్పించు("$unit ", symp);  // Space so can never name conflict with user code
        //
        UASSERT_OBJ(!m_dunitEntp, nodep, "Call insertDUnit only once");
        m_dunitEntp = symp;
    }
    VSymEnt* చొప్పించు_శిఖరం_Cell(Ast_కణుపు_Module* nodep, స్థిరరాశి string& scopename) {
        // Only called on the module at the very top of the hierarchy
        VSymEnt* స్థిరరాశి symp = కొత్త VSymEnt{&m_syms, nodep};
        UINFO(9, "      INSERTtop se" << cvt_గమ్యం_Hex(symp) << "  " << scopename << " " << nodep);
        symp->parentp(మూలం_Entp());  // Needed so backward search can find name of top module
        symp->fallbackp(dunitEntp());  // Needed so can find $unit stuff
        nodep->user1p(symp);
        checkDuplicate(మూలం_Entp(), nodep, nodep->origName());
        మూలం_Entp()->చొప్పించు(nodep->origName(), symp);
        ఒకవేళ (forScopeCreation()) m_nameScopeSymMap.emplace(scopename, symp);
        తిరిగి symp;
    }
    VSymEnt* చొప్పించు_శిఖరం_Iface(AstCell* nodep, స్థిరరాశి string& scopename) {
        VSymEnt* స్థిరరాశి symp = కొత్త VSymEnt{&m_syms, nodep};
        UINFO(9, "      INSERTtopiface se" << cvt_గమ్యం_Hex(symp) << "  " << scopename << " " << nodep);
        symp->parentp(మూలం_Entp());  // Needed so backward search can find name of top module
        symp->fallbackp(dunitEntp());  // Needed so can find $unit stuff
        nodep->user1p(symp);
        ఒకవేళ (nodep->modp()) nodep->modp()->user1p(symp);
        checkDuplicate(మూలం_Entp(), nodep, nodep->origName());
        మూలం_Entp()->చొప్పించు(nodep->origName(), symp);
        ఒకవేళ (forScopeCreation()) m_nameScopeSymMap.emplace(scopename, symp);
        తిరిగి symp;
    }
    VSymEnt* చొప్పించు_Cell(VSymEnt* abovep, VSymEnt* modSymp, AstCell* nodep,
                        స్థిరరాశి string& scopename) {
        UASSERT_OBJ(abovep, nodep, "Null symbol table inserting node");
        VSymEnt* స్థిరరాశి symp = కొత్త VSymEnt{&m_syms, nodep};
        UINFO(9, "      INSERTcel se" << cvt_గమ్యం_Hex(symp) << "  " << scopename << " above=se"
                                      << cvt_గమ్యం_Hex(abovep) << " mods=se" << cvt_గమ్యం_Hex(modSymp)
                                      << " node=" << nodep);
        symp->parentp(abovep);
        symp->fallbackp(dunitEntp());  // Needed so can find $unit stuff
        nodep->user1p(symp);
        ఒకవేళ (nodep->modp()) nodep->modp()->user1p(symp);
        checkDuplicate(abovep, nodep, nodep->origName());
        abovep->reinsert(nodep->origName(), symp);
        ఒకవేళ (forScopeCreation() && abovep != modSymp && !modSymp->కనుగొను_Id_Flat(nodep->name())) {
            // If it's foo_DOT_bar, we need to be able to find it under "foo_DOT_bar" too.
            // Duplicates are possible, as until resolve generates might
            // have 2 same cells under an if
            modSymp->reinsert(nodep->name(), symp);
        }
        ఒకవేళ (forScopeCreation()) m_nameScopeSymMap.emplace(scopename, symp);
        తిరిగి symp;
    }
    శూన్య చొప్పించు_Map(VSymEnt* symp, స్థిరరాశి string& scopename) {
        ఒకవేళ (forScopeCreation()) m_nameScopeSymMap.emplace(scopename, symp);
    }

    VSymEnt* చొప్పించు_Inline(VSymEnt* abovep, VSymEnt* modSymp, AstCellInline* nodep,
                          స్థిరరాశి string& basename) {
        // A fake point in the hierarchy, corresponding to a named block flattened by V3Begin.
        // Names are looked up in the module with the block's flattened prefix.
        UASSERT_OBJ(abovep, nodep, "Null symbol table inserting node");
        VSymEnt* స్థిరరాశి symp = కొత్త VSymEnt{&m_syms, nodep};
        UINFO(9, "      INSERTinl se" << cvt_గమ్యం_Hex(symp) << "  " << basename << " above=se"
                                      << cvt_గమ్యం_Hex(abovep) << " mods=se" << cvt_గమ్యం_Hex(modSymp)
                                      << " node=" << nodep);
        symp->parentp(abovep);
        symp->fallbackp(modSymp);
        symp->sym_ఉపసర్గ(nodep->name() + "__DOT__");
        nodep->user1p(symp);
        checkDuplicate(abovep, nodep, nodep->name());
        abovep->reinsert(basename, symp);
        ఒకవేళ (abovep != modSymp && !modSymp->కనుగొను_Id_Flat(nodep->name())) {
            // If it's foo_DOT_bar, we need to be able to find it under that too.
            modSymp->reinsert(nodep->name(), symp);
        }
        తిరిగి symp;
    }
    VSymEnt* చొప్పించు_ఖండం(VSymEnt* abovep, స్థిరరాశి string& name, Ast_కణుపు* nodep,
                         Ast_కణుపు_Module* classOrPackagep) {
        // A fake point in the hierarchy, corresponding to a begin or function/task block
        // After we remove begins these will go away
        // Note we fallback to the symbol table of the parent, as we want to find variables there
        // However, cells walk the graph, so cells will appear under the begin/ftask itself
        UASSERT_OBJ(abovep, nodep, "Null symbol table inserting node");
        VSymEnt* స్థిరరాశి symp = కొత్త VSymEnt{&m_syms, nodep};
        UINFO(9, "      INSERTblk se" << cvt_గమ్యం_Hex(symp) << "  above=se" << cvt_గమ్యం_Hex(abovep)
                                      << " pkg=" << cvt_గమ్యం_Hex(classOrPackagep)
                                      << "  node=" << nodep);
        symp->parentp(abovep);
        symp->classOrPackagep(classOrPackagep);
        symp->fallbackp(abovep);
        nodep->user1p(symp);
        ఒకవేళ (name != "") checkDuplicate(abovep, nodep, name);
        // Duplicates are possible, as until resolve generates might have 2 same cells under an if
        abovep->reinsert(name, symp);
        తిరిగి symp;
    }
    VSymEnt* చొప్పించు_Sym(VSymEnt* abovep, స్థిరరాశి string& name, Ast_కణుపు* nodep,
                       Ast_కణుపు_Module* classOrPackagep) {
        UASSERT_OBJ(abovep, nodep, "Null symbol table inserting node");
        VSymEnt* స్థిరరాశి symp = కొత్త VSymEnt{&m_syms, nodep};
        UINFO(9, "      INSERTsym se" << cvt_గమ్యం_Hex(symp) << "  name='" << name << "' above=se"
                                      << cvt_గమ్యం_Hex(abovep) << " pkg=" << cvt_గమ్యం_Hex(classOrPackagep)
                                      << "  node=" << nodep);
        // We don't remember the ent associated with each node, because we
        // need a unique scope entry for each instantiation
        symp->classOrPackagep(classOrPackagep);
        symp->parentp(abovep);
        symp->fallbackp(abovep);
        nodep->user1p(symp);
        checkDuplicate(abovep, nodep, name);
        abovep->reinsert(name, symp);
        తిరిగి symp;
    }
    స్థిర తార్కిక exists_కణుపు_Sym(Ast_కణుపు* nodep) { తిరిగి nodep->user1p() != శూన్యసూచిక; }
    స్థిర VSymEnt* get_కణుపు_Sym(Ast_కణుపు* nodep) {
        // Don't use this in ResolveVisitor, as we need to pick up the proper
        // reference under each SCOPE
        VSymEnt* స్థిరరాశి symp = nodep->user1u().గమ్యం_Sym_Ent();
        UASSERT_OBJ(symp, nodep, "Module/etc never assigned a symbol entry?");
        తిరిగి symp;
    }
    VSymEnt* getScopeSym(AstScope* nodep) {
        స్థిరరాశి స్వయంచాలక it = m_nameScopeSymMap.కనుగొను(nodep->name());
        UASSERT_OBJ(it != m_nameScopeSymMap.చివర(), nodep,
                    "Scope never assigned a symbol entry '" << nodep->name() << "'");
        తిరిగి it->రెండవ;
    }
    శూన్య implicit_Ok_కూడు(Ast_కణుపు_Module* nodep, స్థిరరాశి string& varname) {
        // Mark the given variable name as being allowed to be implicitly declared
        ఒకవేళ (nodep) {
            స్థిరరాశి స్వయంచాలక it = m_implicit_Name_సమితి.కనుగొను(std::make_pair(nodep, varname));
            ఒకవేళ (it == m_implicit_Name_సమితి.చివర()) m_implicit_Name_సమితి.emplace(nodep, varname);
        }
    }
    తార్కిక implicitOk(Ast_కణుపు_Module* nodep, స్థిరరాశి string& varname) {
        తిరిగి nodep
               && (m_implicit_Name_సమితి.కనుగొను(std::make_pair(nodep, varname))
                   != m_implicit_Name_సమితి.చివర());
    }

    // Track and later recurse interface modules
    శూన్య చొప్పించు_Iface_Mod_Sym(AstIface* nodep, VSymEnt* symp) {
        m_ifaceModSyms.emplace_వెనుక(nodep, symp);
    }
    శూన్య computeIfaceModSyms();

    // Track and later insert interface references
    శూన్య చొప్పించు_Iface_Var_Sym(VSymEnt* symp) {  // Where sym is for a VAR of dtype IFACEREFDTYPE
        m_ifaceVarSyms.నెట్టు_వెనుక(symp);
    }
    // Iface for a raw or arrayed iface; peels nested array layers for multi-dim arrays.
    స్థిర AstIfaceRefDType* ifaceRefFromArray(Ast_కణుపు_DType* nodep) {
        అయితే (nodep) {
            ఒకవేళ (AstIfaceRefDType* స్థిరరాశి ifp = VN_CAST(nodep, IfaceRefDType)) తిరిగి ifp;
            ఒకవేళ (స్థిరరాశి AstBracketArrayDType* స్థిరరాశి arrp = VN_CAST(nodep, BracketArrayDType)) {
                nodep = arrp->subDTypep();
            } లేకపోతే ఒకవేళ (స్థిరరాశి AstUnpackArrayDType* స్థిరరాశి arrp = VN_CAST(nodep, UnpackArrayDType)) {
                nodep = arrp->subDTypep();
            } లేకపోతే {
                తిరిగి శూన్యసూచిక;
            }
        }
        తిరిగి శూన్యసూచిక;
    }
    // Given a pin expression, resolve it to a live AstIface* (or nullptr).
    // Handles both simple VarRef and dotted VarXRef pin connections.
    AstIface* సజీవ_Iface_From_Pin_సమాసం(Ast_కణుపు* exprp, VSymEnt* మాతృ_Symp) {
        AstVar* resolvedVarp = శూన్యసూచిక;
        ఒకవేళ (AstVarRef* స్థిరరాశి refp = VN_CAST(exprp, VarRef)) {
            resolvedVarp = refp->varp();
        } లేకపోతే ఒకవేళ (AstVarXRef* స్థిరరాశి xrefp = VN_CAST(exprp, VarXRef)) {
            VSymEnt* స్థిరరాశి lookupSymp = మాతృ_Symp ? మాతృ_Symp->parentp() : శూన్యసూచిక;
            ఒకవేళ (lookupSymp) {
                string baddot;
                VSymEnt* okSymp = శూన్యసూచిక;
                స్థిరరాశి string dotpath = xrefp->dotted() + "." + xrefp->name();
                VSymEnt* స్థిరరాశి dotSymp
                    = కనుగొను_Dotted(xrefp->fileline(), lookupSymp, dotpath, baddot, okSymp, నిజం);
                ఒకవేళ (dotSymp) resolvedVarp = VN_CAST(dotSymp->nodep(), Var);
            }
        }
        AstIface* resultp = శూన్యసూచిక;
        ఒకవేళ (resolvedVarp) {
            AstIfaceRefDType* స్థిరరాశి irefp = ifaceRefFromArray(resolvedVarp->subDTypep());
            ఒకవేళ (irefp && irefp->iface_ద్వారా_Cellp() && !irefp->iface_ద్వారా_Cellp()->dead()) {
                resultp = irefp->iface_ద్వారా_Cellp();
            }
        }
        తిరిగి resultp;
    }

    // Attempt to repair a port's AstIfaceRefDType by tracing through the
    // parent cell's pin connection to find the correct live interface.
    // Returns true if repaired or already correct; false if unresolvable.
    తార్కిక repairIfaceRef(VSymEnt* varSymp, AstVar* varp, AstIfaceRefDType* ifacerefp, తార్కిక isDead) {
        // Walk up symbol-table parents to find the enclosing AstCell
        VSymEnt* మాతృ_Symp = varSymp->parentp();
        AstCell* మాతృ_Cellp = శూన్యసూచిక;
        కొరకు (పూర్ణాంకం లోతు = 0; మాతృ_Symp && లోతు < 20; ++లోతు) {
            ఒకవేళ (AstCell* స్థిరరాశి cp = VN_CAST(మాతృ_Symp->nodep(), Cell)) {
                మాతృ_Cellp = cp;
                ఆపు;
            }
            మాతృ_Symp = మాతృ_Symp->parentp();
        }
        ఒకవేళ (!మాతృ_Cellp) తిరిగి అబద్ధం;
        // Scan pins to find the one connected to this port variable
        కొరకు (AstPin* pinp = మాతృ_Cellp->pinsp(); pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            ఒకవేళ (pinp->modVarp() != varp && pinp->name() != varp->name()) కొనసాగు;
            Ast_కణుపు* స్థిరరాశి exprp = pinp->exprp();
            ఒకవేళ (!exprp) తిరిగి అబద్ధం;
            AstIface* స్థిరరాశి newIfacep = సజీవ_Iface_From_Pin_సమాసం(exprp, మాతృ_Symp);
            ఒకవేళ (newIfacep && newIfacep != ifacerefp->iface_ద్వారా_Cellp()) {
                UINFO(4, "  REPAIR-IFACE-REF var=" << varp->prettyNameQ()
                                                   << " old=" << ifacerefp->ifacep()->prettyNameQ()
                                                   << " new=" << newIfacep->prettyNameQ());
                ifacerefp->ifacep(newIfacep);
                తిరిగి నిజం;
            }
            ఒకవేళ (newIfacep) తిరిగి !isDead;  // Same interface - OK if live
            తిరిగి అబద్ధం;
        }
        తిరిగి అబద్ధం;
    }

    శూన్య computeIfaceVarSyms() {
        కొరకు (VSymEnt* varSymp : m_ifaceVarSyms) {
            AstVar* స్థిరరాశి varp = varSymp ? VN_AS(varSymp->nodep(), Var) : శూన్యసూచిక;
            UINFO(9, "  insAllIface se" << cvt_గమ్యం_Hex(varSymp) << " " << varp);
            AstIfaceRefDType* స్థిరరాశి ifacerefp = ifaceRefFromArray(varp->subDTypep());
            UASSERT_OBJ(ifacerefp, varp, "Non-ifacerefs on list!");
            ఒకవేళ (!ifacerefp->cellp() && !ifacerefp->isVirtual() && (varp->pin_అంకె() == 0)) {
                varp->v3error("Interface port declaration "
                              << varp->prettyNameQ() << " doesn't have corresponding port\n"
                              << varp->warn_మరిన్ని()
                                     + "... Perhaps intended an interface instantiation but "
                                       "are missing parenthesis (IEEE 1800-2023 25.3)?");
            }
            ఒకవేళ (!ifacerefp->iface_ద్వారా_Cellp()) {
                ఒకవేళ (!ifacerefp->cellp()) {  // Probably a NotFoundModule, or a normal module if
                                            // made mistake
                    UINFO(1, "Associated cell " << Ast_కణుపు::prettyNameQ(ifacerefp->cellName()));
                    ifacerefp->v3error("Cannot find file containing interface: "
                                       << Ast_కణుపు::prettyNameQ(ifacerefp->ifaceName()));
                    కొనసాగు;
                } లేకపోతే {
                    ifacerefp->v3fatalSrc("Unlinked interface");
                }
            } లేకపోతే ఒకవేళ (varp->isIfaceRef() && !ifacerefp->cellp() && ifacerefp->iface_ద్వారా_Cellp()) {
                // Port variable (cellp=null) pointing to an interface.
                // The interface may be dead (template replaced by clone) or
                // live-but-wrong (template still alive because some instances
                // use default params, but this port should point to a
                // specialized clone based on the pin connection).
                స్థిరరాశి తార్కిక isDead = ifacerefp->iface_ద్వారా_Cellp()->dead();
                ఒకవేళ (isDead && forPrimary() && !varp->is_Iface_మాతృ()
                    && !v3Global.opt.శిఖరం_Ifaces_Supported()) {
                    varp->v3warn(E_UNSUPPORTED,
                                 "Unsupported: Interfaced port on top level module");
                }
                // Attempt repair: trace through parent cell's pin to find
                // the correct live interface for this port variable
                స్థిరరాశి తార్కిక repaired = repairIfaceRef(varSymp, varp, ifacerefp, isDead);
                ఒకవేళ (!repaired && isDead) {
                    ifacerefp->v3error(
                        "Interface " << Ast_కణుపు::prettyNameQ(ifacerefp->ifaceName())
                                     << " not connected as parent's interface not connected\n"
                                     << ifacerefp->warn_మరిన్ని()
                                     << "... Perhaps caused by another error on the parent "
                                        "interface that needs resolving\n"
                                     << ifacerefp->warn_మరిన్ని()
                                     << "... Or, perhaps intended an interface instantiation but "
                                        "are missing parenthesis (IEEE 1800-2023 25.3)?");
                    కొనసాగు;
                }
            } లేకపోతే ఒకవేళ (ifacerefp->iface_ద్వారా_Cellp()->dead()
                       || !exists_కణుపు_Sym(ifacerefp->iface_ద్వారా_Cellp())) {
                ifacerefp->iface_ద్వారా_Cellp()->v3fatalSrc(
                    "Interface referenced by virtual iface is dead. "
                    "Dead interfaces have required linking steps skipped");
            }
            VSymEnt* స్థిరరాశి ifaceSymp = get_కణుపు_Sym(ifacerefp->iface_ద్వారా_Cellp());
            VSymEnt* if_Or_పోర్టు_Symp = ifaceSymp;
            // Link Modport names to the Modport Node under the Interface
            ఒకవేళ (ifacerefp->isModport()) {
                VSymEnt* స్థిరరాశి foundp = ifaceSymp->కనుగొను_Id_Fallback(ifacerefp->modportName());
                తార్కిక ok = అబద్ధం;
                ఒకవేళ (foundp) {
                    ఒకవేళ (AstModport* స్థిరరాశి modportp = VN_CAST(foundp->nodep(), Modport)) {
                        UINFO(4, "Link Modport: " << modportp);
                        ifacerefp->modportp(modportp);
                        if_Or_పోర్టు_Symp = foundp;
                        ok = నిజం;
                    }
                }
                ఒకవేళ (!ok) {
                    స్థిరరాశి string suggest = suggestSymFallback(ifaceSymp, ifacerefp->modportName(),
                                                              Link_కణుపు_Matcher_Modport{});
                    ifacerefp->modportFileline()->v3error(
                        "Modport not found under interface "
                        << ifacerefp->prettyNameQ(ifacerefp->ifaceName()) << ": "
                        << ifacerefp->prettyNameQ(ifacerefp->modportName()) << '\n'
                        << (suggest.ఖాళీలు() ? "" : ifacerefp->warn_మరిన్ని() + suggest));
                }
            }
            // Alias won't expand until interfaces and modport names are known; see notes at top
            చొప్పించు_Scope_Alias(SAMN_IFTOP, varSymp, if_Or_పోర్టు_Symp);
        }
        m_ifaceVarSyms.clear();
    }

    శూన్య చొప్పించు_Scope_Alias(SAMNum samn, VSymEnt* lhsp, VSymEnt* rhsp) {
        // Track and later insert scope aliases; an interface referenced by
        // a child cell connecting to that interface
        // Typically lhsp=VAR w/dtype IFACEREF, rhsp=IFACE cell
        UINFO(9, "   insertScopeAlias se" << cvt_గమ్యం_Hex(lhsp) << " se" << cvt_గమ్యం_Hex(rhsp));
        UASSERT_OBJ(
            !(VN_IS(rhsp->nodep(), Cell) && !VN_IS(VN_AS(rhsp->nodep(), Cell)->modp(), Iface)),
            rhsp->nodep(), "Got a non-IFACE alias RHS");
        m_scopeAliasMap[samn].emplace(lhsp, rhsp);
    }
    శూన్య computeScopeAliases() {
        UINFO(9, "computeIfaceAliases");
        కొరకు (పూర్ణాంకం samn = 0; samn < SAMN__MAX; ++samn) {
            కొరకు (ScopeAliasMap::iterator it = m_scopeAliasMap[samn].begin();
                 it != m_scopeAliasMap[samn].చివర(); ++it) {
                VSymEnt* స్థిరరాశి lhsp = it->మొదటి;
                VSymEnt* srcp = lhsp;
                అయితే (నిజం) {  // Follow chain of aliases up to highest level non-alias
                    స్థిరరాశి స్వయంచాలక it2 = m_scopeAliasMap[samn].కనుగొను(srcp);
                    ఒకవేళ (it2 != m_scopeAliasMap[samn].చివర()) {
                        srcp = it2->రెండవ;
                        కొనసాగు;
                    } లేకపోతే {
                        ఆపు;
                    }
                }
                UINFO(9, "  iiasa: Insert alias se" << lhsp << " (" << lhsp->nodep()->typeName()
                                                    << ") <- se" << srcp << " " << srcp->nodep());
                // srcp should be an interface reference pointing to the interface we want to
                // import
                lhsp->importFromIface(symsp(), srcp);
                // Allow access to objects not permissible to be listed in a modport
                ఒకవేళ (VN_IS(srcp->nodep(), Modport)) {
                    lhsp->importFromIface(symsp(), srcp->parentp(), నిజం);
                }
            }
            // m_scopeAliasMap[samn].clear();  // Done with it, but put into debug file
        }
    }

అంతరంగ:
    VSymEnt* కనుగొను_With_Alt_Fallback(VSymEnt* symp, స్థిరరాశి string& name, స్థిరరాశి string& altname) {
        VSymEnt* findp = symp->కనుగొను_Id_Fallback(name);
        ఒకవేళ (findp) తిరిగి findp;
        ఒకవేళ (altname != "") {
            UINFO(8, "     alt fallback");
            findp = symp->కనుగొను_Id_Fallback(altname);
        }
        తిరిగి findp;
    }

    VSymEnt* కనుగొను_With_Alt_Flat(VSymEnt* symp, స్థిరరాశి string& name, స్థిరరాశి string& altname) {
        VSymEnt* findp = symp->కనుగొను_Id_Flat(name);
        ఒకవేళ (findp) తిరిగి findp;
        ఒకవేళ (altname != "") {
            UINFO(8, "     alt flat");
            findp = symp->కనుగొను_Id_Flat(altname);
        }
        తిరిగి findp;
    }

బహిరంగ:
    VSymEnt* కనుగొను_Dotted(File_పంక్తి* refLocationp, VSymEnt* lookupSymp, స్థిరరాశి string& dotname,
                        string& baddot, VSymEnt*& okSymp, తార్కిక మొదటి_Id) {
        // Given a dotted hierarchy name, return where in scope it is
        // Note when dotname=="" we just fall through and return lookupSymp
        UINFO(8, "    dottedFind se" << cvt_గమ్యం_Hex(lookupSymp) << " '" << dotname << "'");
        string leftname = dotname;
        okSymp = lookupSymp;  // So can list bad scopes
        అయితే (leftname != "") {  // foreach dotted part of xref name
            string::కొలత_type pos = leftname.కనుగొను('.');
            string ident;
            ఒకవేళ (pos != string::npos) {
                ident = leftname.substr(0, pos);
                leftname = leftname.substr(pos + 1);
            } లేకపోతే {
                ident = leftname;
                leftname = "";
            }
            baddot = ident;  // So user can see where they botched it
            okSymp = lookupSymp;
            string altIdent;
            ఒకవేళ (!forScopeCreation()) {
                // GENFOR Begin is foo__BRA__##__KET__ after we've genloop unrolled,
                // but presently should be just "foo".
                // Likewise cell foo__[array] before we've expanded arrays is just foo.
                // Multi-dim iface arrays append multiple __BRA__..__KET__ suffixes; strip them
                // all.
                altIdent = ident;
                అయితే (VString::చివర్లు_With(altIdent, "__KET__")) {
                    స్థిరరాశి స్వయంచాలక braPos = altIdent.rfind("__BRA__");
                    ఒకవేళ (braPos == string::npos) ఆపు;
                    altIdent = altIdent.substr(0, braPos);
                }
                ఒకవేళ (altIdent == ident) altIdent.clear();
            }
            UINFO(8, "         id " << ident << " alt " << altIdent << " left " << leftname
                                    << " at se" << lookupSymp);
            // Spec says; Look at existing module (cellnames then modname),
            // then look up (inst name or modname)
            ఒకవేళ (మొదటి_Id) {
                // Check this module - subcellnames
                స్థిరరాశి AstCell* cellp = lookupSymp ? VN_CAST(lookupSymp->nodep(), Cell)
                                                  : శూన్యసూచిక;  // Replicated below
                ఒకవేళ (VSymEnt* స్థిరరాశి కనుగొను_Symp = కనుగొను_With_Alt_Fallback(lookupSymp, ident, altIdent)) {
                    lookupSymp = కనుగొను_Symp;
                }

                // Check this module - cur modname
                లేకపోతే ఒకవేళ (cellp && cellp->modp()->origName() == ident) {
                }
                // $root we walk up to Netlist
                లేకపోతే ఒకవేళ (ident == "$root") {
                    lookupSymp = మూలం_Entp();
                    // We've added the '$root' module, now everything else is one lower
                    ఒకవేళ (forScopeCreation()) {
                        lookupSymp = lookupSymp->కనుగొను_Id_Flat(ident);
                        UASSERT(lookupSymp, "Cannot find $root module under netlist");
                    }
                }
                // Move up and check cellname + modname
                లేకపోతే {
                    తార్కిక crossedCell = అబద్ధం;  // Crossed a cell boundary
                    అయితే (lookupSymp) {
                        lookupSymp = lookupSymp->parentp();
                        cellp = lookupSymp ? VN_CAST(lookupSymp->nodep(), Cell)
                                           : శూన్యసూచిక;  // Replicated above
                        ఒకవేళ (lookupSymp) {
                            UINFO(9, "    Up to " << lookupSymp);
                            ఒకవేళ (cellp || VN_IS(lookupSymp->nodep(), CellInline)) {
                                crossedCell = నిజం;
                            }
                            ఒకవేళ (cellp && cellp->modp()->origName() == ident) {
                                ఆపు;
                            } లేకపోతే ఒకవేళ (VSymEnt* స్థిరరాశి కనుగొను_Sym2p
                                       = కనుగొను_With_Alt_Fallback(lookupSymp, ident, altIdent)) {
                                lookupSymp = కనుగొను_Sym2p;
                                ఒకవేళ (crossedCell && VN_IS(lookupSymp->nodep(), Var)) {
                                    UINFO(9, "    Not found but matches var name in parent "
                                                 << lookupSymp);
                                    తిరిగి శూన్యసూచిక;
                                }
                                ఆపు;
                            }
                        } లేకపోతే {
                            ఆపు;
                        }
                    }
                    ఒకవేళ (!lookupSymp) తిరిగి శూన్యసూచిక;  // Not found
                }
            } లేకపోతే {  // Searching for middle path component, must be a cell or interface port
                VSymEnt* కనుగొను_Symp = కనుగొను_With_Alt_Flat(lookupSymp, ident, altIdent);
                ఒకవేళ (కనుగొను_Symp) {
                    lookupSymp = కనుగొను_Symp;
                } లేకపోతే {
                    // Try prefixed lookup for interface ports accessed through hierarchy
                    // (e.g., slave_inst.bus.data where bus is an interface port)
                    // After inlining, interface ports have prefixed names like
                    // top__DOT__slave_inst__DOT__bus
                    UINFO(8, "     middle-path: trying prefixed lookup for '" << ident << "'\n");
                    string baddot;
                    కనుగొను_Symp = కనుగొను_Sym_Prefixed(lookupSymp, ident, baddot, /*fallback=*/నిజం);
                    UINFO(8, "     middle-path: prefixed lookup result: "
                                 << (కనుగొను_Symp ? "found" : "not found") << "\n");
                    ఒకవేళ (కనుగొను_Symp) {
                        // Check if this is an interface reference variable
                        స్థిరరాశి AstVar* varp = VN_CAST(కనుగొను_Symp->nodep(), Var);
                        ఒకవేళ (!varp) {
                            ఒకవేళ (స్థిరరాశి AstVarScope* vscp = VN_CAST(కనుగొను_Symp->nodep(), VarScope)) {
                                varp = vscp->varp();
                            }
                        }
                        ఒకవేళ (varp && varp->isIfaceRef()) {
                            // Found an interface port - use findSymp directly
                            // computeScopeAliases has already imported interface members
                            // into this symbol entry, so we use it instead of redirecting
                            // to the shared modport definition (which would cause all
                            // instances to resolve to the same symbols - bug #2656)
                            lookupSymp = కనుగొను_Symp;
                        } లేకపోతే {
                            // Non-interface symbol in middle path must be a cell (module instance)
                            // to continue hierarchical resolution
                            ఒకవేళ (VN_IS(కనుగొను_Symp->nodep(), Cell)) {
                                lookupSymp = కనుగొను_Symp;
                            } లేకపోతే {
                                // Reject non-cell, non-interface symbols found via fallback
                                // to prevent accidental resolution to unrelated symbols
                                UINFO(8, "     middle-path: rejecting non-cell symbol\n");
                                తిరిగి శూన్యసూచిక;
                            }
                        }
                    } లేకపోతే {
                        తిరిగి శూన్యసూచిక;  // Not found
                    }
                }
            }
            ఒకవేళ (lookupSymp) {
                ఒకవేళ (స్థిరరాశి AstCell* స్థిరరాశి cellp = VN_CAST(lookupSymp->nodep(), Cell)) {
                    ఒకవేళ (స్థిరరాశి Ast_కణుపు_Module* స్థిరరాశి modp = cellp->modp()) {
                        ఒకవేళ (modp->hier_ఖండం() && !leftname.ఖాళీలు()) {
                            refLocationp->v3error("Cannot access scope inside hierarchical block");
                        } లేకపోతే ఒకవేళ (VN_IS(modp, Not_దొరికింది_Module)) {
                            refLocationp->v3error("Dotted reference to instance that refers to "
                                                  "missing module/interface: "
                                                  << modp->prettyNameQ());
                        }
                    }
                }
                // Follow scope alias for nested interface port access
                ఒకవేళ (!leftname.ఖాళీలు()) {
                    స్థిరరాశి స్వయంచాలక aliasIt = m_scopeAliasMap[SAMN_IFTOP].కనుగొను(lookupSymp);
                    ఒకవేళ (aliasIt != m_scopeAliasMap[SAMN_IFTOP].చివర()) {
                        lookupSymp = aliasIt->రెండవ;
                        // Alias may point to __Viftop VarScope; find corresponding Cell
                        ఒకవేళ (స్థిరరాశి AstVarScope* స్థిరరాశి vscp
                            = VN_CAST(lookupSymp->nodep(), VarScope)) {
                            స్థిరరాశి string varName = vscp->varp()->name();
                            స్థిర constexpr స్థిరరాశి అక్షరం* స్థిరరాశి VIFTOP_SUFFIX = "__Viftop";
                            ఒకవేళ (VString::చివర్లు_With(varName, VIFTOP_SUFFIX)) {
                                స్థిరరాశి string cellName
                                    = varName.substr(0, varName.కొలత() - అక్షరశ్రేణి_పొడవు(VIFTOP_SUFFIX));
                                VSymEnt* స్థిరరాశి మాతృ_Symp = lookupSymp->parentp();
                                ఒకవేళ (మాతృ_Symp) {
                                    VSymEnt* స్థిరరాశి cellSymp = మాతృ_Symp->కనుగొను_Id_Flat(cellName);
                                    ఒకవేళ (cellSymp && VN_IS(cellSymp->nodep(), Cell)) {
                                        lookupSymp = cellSymp;  // Use Cell for member lookup
                                    }
                                }
                            }
                        }
                    } లేకపోతే {
                        // No alias; try following IfaceRefDType to interface cell
                        స్థిరరాశి AstVar* varp = VN_CAST(lookupSymp->nodep(), Var);
                        ఒకవేళ (!varp) {
                            ఒకవేళ (స్థిరరాశి AstVarScope* స్థిరరాశి vscp
                                = VN_CAST(lookupSymp->nodep(), VarScope)) {
                                varp = vscp->varp();
                            }
                        }
                        ఒకవేళ (varp && varp->isIfaceRef()) {
                            ఒకవేళ (స్థిరరాశి AstIfaceRefDType* స్థిరరాశి ifaceRefp
                                = ifaceRefFromArray(varp->dtypep())) {
                                ఒకవేళ (ifaceRefp->cellp() && exists_కణుపు_Sym(ifaceRefp->cellp())) {
                                    lookupSymp = get_కణుపు_Sym(ifaceRefp->cellp());
                                } లేకపోతే ఒకవేళ (ifaceRefp->iface_ద్వారా_Cellp()
                                           && exists_కణుపు_Sym(ifaceRefp->iface_ద్వారా_Cellp())) {
                                    lookupSymp = get_కణుపు_Sym(ifaceRefp->iface_ద్వారా_Cellp());
                                } లేకపోతే ఒకవేళ (ifaceRefp->ifacep()
                                           && exists_కణుపు_Sym(ifaceRefp->ifacep())) {
                                    lookupSymp = get_కణుపు_Sym(ifaceRefp->ifacep());
                                }
                            }
                        }
                    }
                }
            }
            మొదటి_Id = అబద్ధం;
        }
        తిరిగి lookupSymp;
    }

    స్థిర string remove_చివరి_Inline_Scope(స్థిరరాశి string& name) {
        string బహిర్గతం = name;
        స్థిరరాశి string dot = "__DOT__";
        స్థిరరాశి కొలత_t dotPos = బహిర్గతం.rfind(dot, బహిర్గతం.కొలత() - dot.నిడివి() - 1);
        ఒకవేళ (dotPos == string::npos) {
            తిరిగి "";
        } లేకపోతే {
            తిరిగి బహిర్గతం.erase(dotPos + dot.నిడివి(), string::npos);
        }
    }

    VSymEnt* కనుగొను_Sym_Prefixed(VSymEnt* lookupSymp, స్థిరరాశి string& dotname, string& baddot,
                             తార్కిక fallback) {
        // Find symbol in given point in hierarchy, allowing prefix (post-Inline)
        // For simplicity lookupSymp may be passed nullptr result from findDotted
        ఒకవేళ (!lookupSymp) తిరిగి శూన్యసూచిక;
        UINFO(8, "    findSymPrefixed "
                     << dotname << " under se" << cvt_గమ్యం_Hex(lookupSymp)
                     << ((lookupSymp->sym_ఉపసర్గ() == "") ? "" : " as ")
                     << ((lookupSymp->sym_ఉపసర్గ() == "") ? "" : lookupSymp->sym_ఉపసర్గ() + dotname)
                     << "  at se" << lookupSymp << " fallback=" << fallback);
        string ఉపసర్గ = lookupSymp->sym_ఉపసర్గ();
        VSymEnt* foundp = శూన్యసూచిక;
        అయితే (!foundp) {
            ఒకవేళ (fallback) {
                foundp = lookupSymp->కనుగొను_Id_Fallback(ఉపసర్గ + dotname);  // Might be nullptr
            } లేకపోతే {
                foundp = lookupSymp->కనుగొను_Id_Flat(ఉపసర్గ + dotname);  // Might be nullptr
            }
            ఒకవేళ (ఉపసర్గ.ఖాళీలు()) ఆపు;
            స్థిరరాశి string తదుపరి_ఉపసర్గ = remove_చివరి_Inline_Scope(ఉపసర్గ);
            ఒకవేళ (ఉపసర్గ == తదుపరి_ఉపసర్గ) ఆపు;
            ఉపసర్గ = తదుపరి_ఉపసర్గ;
        }
        ఒకవేళ (!foundp) baddot = dotname;
        తిరిగి foundp;
    }
    స్థిర తార్కిక checkIfClassOrPackage(స్థిరరాశి VSymEnt* స్థిరరాశి symp) {
        ఒకవేళ (VN_IS(symp->nodep(), Class) || VN_IS(symp->nodep(), Package)) తిరిగి నిజం;

        // Helper: check if a RefDType might resolve to a class later.
        స్థిరరాశి స్వయంచాలక isPotentialClassRef = [](స్థిరరాశి AstRefDType* refp) -> తార్కిక {
            తిరిగి refp && !refp->typeofp()
                   && (!refp->classOrPackageOpp()
                       || VN_IS(refp->classOrPackageOpp(), ClassOrPackageRef));
        };

        // Helper: allow types that can represent a class/package handle or an unresolved ref.
        // BasicDType is allowed here only for null/object handles.
        స్థిరరాశి స్వయంచాలక is_Valid_Type_కణుపు = [](స్థిరరాశి Ast_కణుపు* nodep) -> తార్కిక {
            తిరిగి VN_IS(nodep, VoidDType) || VN_IS(nodep, BasicDType)
                   || VN_IS(nodep, ClassRefDType) || VN_IS(nodep, ParseRef);
        };

        ఒకవేళ (స్థిరరాశి AstTypedef* స్థిరరాశి typedefp = VN_CAST(symp->nodep(), Typedef)) {
            స్థిరరాశి Ast_కణుపు_DType* dtypep = typedefp->subDTypep();
            ఒకవేళ (!dtypep) dtypep = typedefp->బిడ్డ_DTypep();
            ఒకవేళ (VN_IS(dtypep, ClassRefDType)) తిరిగి నిజం;
            ఒకవేళ (isPotentialClassRef(VN_CAST(dtypep, RefDType))) తిరిగి నిజం;
        } లేకపోతే ఒకవేళ (స్థిరరాశి AstParamTypeDType* స్థిరరాశి paramTypep
                   = VN_CAST(symp->nodep(), ParamTypeDType)) {
            // Before V3Param the declared default is in childDTypep (possibly
            // wrapped in a RequireDType); after V3Param it is consumed and the
            // bound type is the resolved data type, e.g. a type parameter
            // inherited from a specialized base class (REQ #(Item) -> class Item).
            Ast_కణుపు* childp = paramTypep->బిడ్డ_DTypep();
            ఒకవేళ (స్థిరరాశి AstRequireDType* స్థిరరాశి reqp = VN_CAST(childp, RequireDType)) {
                childp = reqp->lhsp();
            }
            స్థిరరాశి Ast_కణుపు* స్థిరరాశి checkp = childp ? childp : paramTypep->skipRefp();
            ఒకవేళ (is_Valid_Type_కణుపు(checkp) || isPotentialClassRef(VN_CAST(checkp, RefDType))) {
                తిరిగి నిజం;
            }
        }
        తిరిగి అబద్ధం;
    }
    VSymEnt* resolveClassOrPackage(VSymEnt* lookSymp, AstClassOrPackageRef* nodep, తార్కిక fallback,
                                   తార్కిక classOnly, స్థిరరాశి string& forWhat,
                                   తార్కిక deferIfUnresolved = అబద్ధం) {
        ఒకవేళ (nodep->classOrPackageSkipp()) తిరిగి get_కణుపు_Sym(nodep->classOrPackageSkipp());
        VSymEnt* foundp;
        VSymEnt* శోధన_Symp = lookSymp;
        ఒకవేళ (VL_UNCOVERABLE(శోధన_Symp && VN_IS(శోధన_Symp->nodep(), ParamTypeDType))) {
            శోధన_Symp->nodep()->v3fatalSrc(  // LCOV_EXCL_LINE
                "resolveClassOrPackage: unexpected ParamTypeDType lookup");
        }
        ఒకవేళ (fallback) {
            VSymEnt* currentLookSymp = శోధన_Symp;
            చేయి {
                foundp = currentLookSymp->కనుగొను_Id_Flat(nodep->name());
                ఒకవేళ (foundp && !checkIfClassOrPackage(foundp)) foundp = శూన్యసూచిక;
                ఒకవేళ (!foundp) currentLookSymp = currentLookSymp->fallbackp();
            } అయితే (!foundp && currentLookSymp);
        } లేకపోతే {
            foundp = శోధన_Symp->కనుగొను_Id_Flat(nodep->name());
            ఒకవేళ (foundp && !checkIfClassOrPackage(foundp)) foundp = శూన్యసూచిక;
        }
        ఒకవేళ (!foundp && v3Global.rootp()->stdPackagep()) {  // Look under implied std::
            foundp = get_కణుపు_Sym(v3Global.rootp()->stdPackagep())->కనుగొను_Id_Flat(nodep->name());
        }
        ఒకవేళ (foundp) {
            nodep->classOrPackageNodep(foundp->nodep());
            తిరిగి foundp;
        }
        ఒకవేళ (deferIfUnresolved) తిరిగి శూన్యసూచిక;
        స్థిరరాశి string suggest
            = suggestSymFallback(lookSymp, nodep->name(), Link_కణుపు_Matcher_Class_Or_Package{});
        nodep->v3error((classOnly ? "Class" : "Package/class")
                       << " for '" << forWhat  // extends/implements
                       << "' not found: " << nodep->prettyNameQ() << '\n'
                       << (suggest.ఖాళీలు() ? "" : nodep->warn_మరిన్ని() + suggest));
        తిరిగి శూన్యసూచిక;
    }
    string suggestSymFallback(VSymEnt* lookupSymp, స్థిరరాశి string& name,
                              స్థిరరాశి VNodeMatcher& matcher) {
        // Suggest alternative symbol in given point in hierarchy
        // Does not support inline, as we find user-level errors before inlining
        // For simplicity lookupSymp may be passed nullptr result from findDotted
        ఒకవేళ (!lookupSymp) తిరిగి "";
        VSpellCheck speller;
        lookupSymp->candidateIdFallback(&speller, &matcher);
        తిరిగి speller.ఉత్తమ_Candidate_సందేశం(name);
    }
    string suggestSymFlat(VSymEnt* lookupSymp, స్థిరరాశి string& name, స్థిరరాశి VNodeMatcher& matcher,
                          తార్కిక includeType) {
        ఒకవేళ (!lookupSymp) తిరిగి "";
        VSpellCheck speller;
        lookupSymp->candidateIdFlat(&speller, &matcher, includeType);
        తిరిగి speller.ఉత్తమ_Candidate_సందేశం(name);
    }
};

LinkDotState* LinkDotState::s_errorThisp = శూన్యసూచిక;

//======================================================================

తరగతి Link_Dot_కనుగొను_Visitor final : బహిరంగ VNVisitor {
    // NODE STATE
    // Cleared on global
    //  *::user1p()             -> See LinkDotState
    //  *::user2p()             -> See LinkDotState
    //  *::user3()              // bool. Node stored in m_virtualIfaces
    //  *::user4()              -> See LinkDotState
    స్థిరరాశి VNUser3_లోపల_Use m_inuser3;

    // STATE
    LinkDotState* స్థిరరాశి m_statep;  // State to pass between visitors, including symbol table
    Ast_కణుపు_Module* m_classOrPackagep = శూన్యసూచిక;  // Current package
    AstClocking* m_clockingp = శూన్యసూచిక;  // Current clocking block
    std::సదిశ<AstScope*> m_scopes;  // Scopes detached during repeated module visits
    uint64_t m_statScopeVisits = 0;  // Non-top scopes encountered while finding instance symbols
    VSymEnt* m_modSymp = శూన్యసూచిక;  // Symbol Entry for current module
    VSymEnt* m_curSymp = శూన్యసూచిక;  // Symbol Entry for current table, where to lookup/insert
    string
        m_hierParamsName;  // Name of module with hierarchical type parameters, empty when not used
    string m_scope;  // Scope text
    స్థిరరాశి Ast_కణుపు_FTask* m_ftaskp = శూన్యసూచిక;  // Current function/task
    తార్కిక m_లోపల_Recursion = అబద్ధం;  // Inside a recursive module
    పూర్ణాంకం m_param_అంకె = 0;  // Parameter number, for position based connection
    తార్కిక m_explicitNew = అబద్ధం;  // Hit a "new" function
    పూర్ణాంకం m_mod_ఖండం_అంకె = 0;  // Begin block number in module, 0=none seen
    పూర్ణాంకం m_mod_With_అంకె = 0;  // With block number, 0=none seen
    పూర్ణాంకం m_mod_Arg_అంకె = 0;  // Arg block number for randomize(), 0=none seen
    std::సదిశ<AstIface*> m_virtIfaces;  // interfaces used as virtual,
                                          // needed for handleUnvisitedVirtIfaces()

    // METHODS
    శూన్య makeImplicitNew(AstClass* nodep) {
        AstFunc* స్థిరరాశి newp = కొత్త AstFunc{nodep->fileline(), "new", శూన్యసూచిక, శూన్యసూచిక};
        // If needed, super.new() call is added the 2nd pass of V3LinkDotResolve,
        // because base classes are already resolved there.
        newp->isConstructor(నిజం);
        nodep->కూడు_Membersp(newp);
        UINFO(8, "Made implicit new for " << nodep->name() << ": " << nodep);
        m_statep->చొప్పించు_ఖండం(m_curSymp, newp->name(), newp, m_classOrPackagep);
    }

    స్థిర తార్కిక is_Hier_ఖండం_Wrapper(స్థిరరాశి string& name) {
        స్థిరరాశి V3_Hier_ఖండం_Opt_సమితి& hier_ఖండాలు = v3Global.opt.hier_ఖండాలు();
        తిరిగి hier_ఖండాలు.కనుగొను(name) != hier_ఖండాలు.చివర();
    }

    శూన్య moveExternFuncDecl(Ast_కణుపు_FTask* nodep, AstClass* classp) {
        // Move an 'extern function|task' to inside a class or, from an outer to inner-class
        ఒకవేళ (nodep->isExternDef()) తిరిగి;
        // Move it to proper spot under the target class
        nodep->unlink_Fr_వెనుక();
        classp->కూడు_Stmtsp(nodep);
        nodep->isExternDef(నిజం);  // So we check there's a matching extern
        nodep->classOrPackagep()->unlink_Fr_వెనుక()->delete_వృక్షం();
        // Any "Type::" reference in the function's IO are really "MovedToClass::" references
        ఒకవేళ (nodep->fvarp()) moveExternFuncDeclRefs(nodep->fvarp(), classp);
        కొరకు (Ast_కణుపు* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ఒకవేళ (AstVar* స్థిరరాశి portp = VN_CAST(stmtp, Var)) {
                ఒకవేళ (portp->isIO()) moveExternFuncDeclRefs(portp, classp);
            }
        }
    }
    శూన్య moveExternFuncDeclRefs(Ast_కణుపు* nodep, AstClass* classp) {
        nodep->foreach([ఇది, classp](AstClassOrPackageRef* refp) {  //
            ఒకవేళ (refp->name() == classp->name() && !refp->paramsp()) {
                UINFO(9, "Cleaning up external function type for class " << refp);
                AstDot* స్థిరరాశి dotp = VN_CAST(refp->backp(), Dot);
                ఒకవేళ (dotp && dotp->lhsp() == refp) {
                    // If had DOT(CLASSREF this, ...) the DOT can go away
                    dotp->replaceWith(dotp->rhsp()->unlink_Fr_వెనుక());
                    VL_DO_DANGLING2(నెట్టు_Deletep(dotp), dotp, refp);
                } లేకపోతే {
                    VL_DO_DANGLING(నెట్టు_Deletep(refp->unlink_Fr_వెనుక()), refp);
                }
            }
        });
    }

    // VISITORS
    శూన్య సందర్శించు(AstNetlist* nodep) override {  // FindVisitor::
        // Process $unit or other packages
        // Not needed - dotted references not allowed from inside packages
        // for (AstNodeModule* nodep = v3Global.rootp()->modulesp();
        //     nodep; nodep=VN_AS(nodep->nextp(), NodeModule)) {
        //    if (VN_IS(nodep, Package)) {}}

        m_statep->చొప్పించు_DUnit(nodep);

        // First back iterate, to find all packages. Backward as must do base
        // packages before using packages
        iterateChildrenBackwardsConst(nodep);

        // The first modules in the list are always the top modules
        // (sorted before this is called).
        // This may not be the module with isTop() set, as early in the steps,
        // wrapTop may have not been created yet.
        // $unit and the constant pool always exist, so nothing else, and nothing in $unit,
        // means nothing was given
        Ast_కణుపు_Module* స్థిరరాశి modulesp = nodep->modulesp();
        UASSERT_OBJ(modulesp, nodep, "$unit should always be in the netlist");
        ఒకవేళ (m_statep->forPrimary()) {
            స్థిరరాశి తార్కిక onlyBuiltins = [&]() {
                కొరకు (Ast_కణుపు_Module* modp = modulesp; modp;
                     modp = VN_AS(modp->nextp(), కణుపు_Module)) {
                    ఒకవేళ (!modp->isDollarUnit() && !modp->isConstPool()) తిరిగి అబద్ధం;
                }
                తిరిగి నిజం;
            }();
            ఒకవేళ (onlyBuiltins && !v3Global.rootp()->dollarUnitPkgp()->stmtsp()) {
                nodep->v3error("No top level module found");
            }
        }
        కొరకు (Ast_కణుపు_Module* modp = nodep->modulesp(); modp && modp->is_శిఖరం();
             modp = VN_AS(modp->nextp(), కణుపు_Module)) {
            UINFO(8, "Top Module: " << modp);
            m_scope = "TOP";

            ఒకవేళ (!m_statep->forScopeCreation() && v3Global.opt.శిఖరం_Ifaces_Supported()) {
                కొరకు (Ast_కణుపు* subnodep = modp->stmtsp(); subnodep; subnodep = subnodep->nextp()) {
                    ఒకవేళ (AstVar* స్థిరరాశి varp = VN_CAST(subnodep, Var)) {
                        ఒకవేళ (varp->isIfaceRef()) {
                            స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి subtypep = varp->subDTypep();
                            స్థిరరాశి AstIfaceRefDType* ifacerefp = శూన్యసూచిక;
                            ఒకవేళ (VN_IS(subtypep, IfaceRefDType)) {
                                ifacerefp = VN_AS(varp->subDTypep(), IfaceRefDType);
                            } లేకపోతే ఒకవేళ (VN_IS(subtypep, BracketArrayDType)) {
                                స్థిరరాశి AstBracketArrayDType* స్థిరరాశి arrp
                                    = VN_AS(subtypep, BracketArrayDType);
                                స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి arrsubtypep = arrp->subDTypep();
                                ఒకవేళ (VN_IS(arrsubtypep, IfaceRefDType)) {
                                    ifacerefp = VN_AS(arrsubtypep, IfaceRefDType);
                                }
                            } లేకపోతే ఒకవేళ (VN_IS(subtypep, UnpackArrayDType)) {
                                స్థిరరాశి AstUnpackArrayDType* స్థిరరాశి arrp
                                    = VN_AS(subtypep, UnpackArrayDType);
                                స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి arrsubtypep = arrp->subDTypep();
                                ఒకవేళ (VN_IS(arrsubtypep, IfaceRefDType)) {
                                    ifacerefp = VN_AS(arrsubtypep, IfaceRefDType);
                                }
                            }

                            ఒకవేళ (ifacerefp && !ifacerefp->cellp()) {
                                // A dummy cell to keep the top level interface alive and correctly
                                // optimized for default parameter values
                                AstCell* ifacecellp
                                    = కొత్త AstCell{nodep->fileline(),
                                                  nodep->fileline(),
                                                  modp->name() + "__02E" + varp->name(),
                                                  ifacerefp->ifaceName(),
                                                  శూన్యసూచిక,
                                                  శూన్యసూచిక,
                                                  శూన్యసూచిక};
                                ifacecellp->modp(ifacerefp->ifacep());
                                m_curSymp = m_modSymp
                                    = m_statep->చొప్పించు_శిఖరం_Iface(ifacecellp, m_scope);
                                iterate(ifacecellp);
                                VL_DO_DANGLING(నెట్టు_Deletep(ifacecellp), ifacecellp);
                            }
                        }
                    }
                }
            }

            m_curSymp = m_modSymp = m_statep->చొప్పించు_శిఖరం_Cell(modp, m_scope);
            { iterate(modp); }

            m_scope = "";
            m_curSymp = m_modSymp = శూన్యసూచిక;
        }
    }
    శూన్య సందర్శించు(Ast_Type_పట్టిక*) override {}  // FindVisitor::
    శూన్య సందర్శించు(AstIfaceRefDType* nodep) override {  // FindVisitor::
        ఒకవేళ (!m_statep->forScopeCreation() && nodep->isVirtual() && nodep->ifacep()
            && !nodep->ifacep()->user3()) {
            m_virtIfaces.నెట్టు_వెనుక(nodep->ifacep());
            nodep->ifacep()->user3(నిజం);
        }
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(Ast_కణుపు_Module* nodep) override {  // FindVisitor::
        // Called on top module from Netlist, other modules from the cell creating them,
        // and packages
        UINFO(8, "   " << nodep);
        // m_curSymp/m_modSymp maybe nullptr for packages and non-top modules
        // Packages will be under top after the initial phases, but until then
        // need separate handling
        స్థిరరాశి తార్కిక standalonePkg
            = !m_modSymp && !m_statep->forScopeCreation() && VN_IS(nodep, Package);
        స్థిరరాశి తార్కిక doit = (m_modSymp || standalonePkg);
        VL_RESTORER_COPY(m_scope);
        VL_RESTORER(m_classOrPackagep);
        VL_RESTORER(m_modSymp);
        VL_RESTORER(m_curSymp);
        VL_RESTORER(m_param_అంకె);
        VL_RESTORER(m_mod_ఖండం_అంకె);
        VL_RESTORER(m_mod_With_అంకె);
        VL_RESTORER(m_mod_Arg_అంకె);
        ఒకవేళ (doit && nodep->user2()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Identically recursive module (module instantiates "
                          "itself, without changing parameters): "
                              << Ast_కణుపు::prettyNameQ(nodep->origName()));
        } లేకపోతే ఒకవేళ (doit) {
            UINFO(4, "     Link Module: " << nodep);
            UASSERT_OBJ(!nodep->dead(), nodep, "Module in instance tree mislabeled as dead?");
            AstPackage* స్థిరరాశి pkgp = VN_CAST(nodep, Package);
            m_classOrPackagep = pkgp;
            ఒకవేళ (standalonePkg) {
                ఒకవేళ (pkgp->isDollarUnit()) {
                    m_curSymp = m_modSymp = m_statep->dunitEntp();
                    nodep->user1p(m_curSymp);
                } లేకపోతే {
                    VSymEnt* స్థిరరాశి పై_Symp = m_statep->dunitEntp();
                    m_scope = nodep->name();
                    m_curSymp = m_modSymp = m_statep->చొప్పించు_ఖండం(పై_Symp, nodep->name(), nodep,
                                                                  m_classOrPackagep);
                    UINFO(9, "New module scope " << m_curSymp);
                }
            }
            //
            m_param_అంకె = 0;
            m_mod_ఖండం_అంకె = 0;
            m_mod_With_అంకె = 0;
            m_mod_Arg_అంకె = 0;
            // m_modSymp/m_curSymp for non-packages set by AstCell above this module
            // Iterate
            nodep->user2(నిజం);
            iterateChildren(nodep);
            nodep->user2(అబద్ధం);
            nodep->user4(నిజం);
            // Interfaces need another pass when signals are resolved. When creating
            // scopes this is done from the AstScopes in LinkDotScopeVisitor instead, as
            // only those entries hold the AstVarScopes the modport items resolve against.
            ఒకవేళ (!m_statep->forScopeCreation()) {
                ఒకవేళ (AstIface* స్థిరరాశి ifacep = VN_CAST(nodep, Iface)) {
                    m_statep->చొప్పించు_Iface_Mod_Sym(ifacep, m_curSymp);
                }
            }
        } లేకపోతే ఒకవేళ (is_Hier_ఖండం_Wrapper(nodep->name())) {
            UINFO(5, "Module is hierarchical block, must not be dead: " << nodep);
            m_scope = nodep->name();
            VSymEnt* స్థిరరాశి పై_Symp = m_curSymp ? m_curSymp : m_statep->మూలం_Entp();
            m_curSymp = m_modSymp
                = m_statep->చొప్పించు_ఖండం(పై_Symp, nodep->name() + "::", nodep, m_classOrPackagep);
            iterateChildren(nodep);
            nodep->user4(నిజం);
        } లేకపోతే {  // !doit
            ఒకవేళ (nodep->hierParams()) {
                UINFO(1, "Found module with hier type parameters");
                m_hierParamsName = nodep->name();
                కొరకు (స్థిరరాశి Ast_కణుపు* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    ఒకవేళ (స్థిరరాశి AstTypedef* స్థిరరాశి tdef = VN_CAST(stmtp, Typedef)) {
                        UINFO(1, "Inserting hier type parameter typedef: " << tdef);
                        VSymEnt* స్థిరరాశి పై_Symp = m_curSymp ? m_curSymp : m_statep->మూలం_Entp();
                        m_curSymp = m_modSymp = m_statep->చొప్పించు_ఖండం(పై_Symp, nodep->name(),
                                                                      nodep, m_classOrPackagep);
                    }
                }
            } లేకపోతే {
                // Will be optimized away later
                // Can't remove now, as our backwards iterator will throw up
                UINFO(5, "Module not under any CELL or top - dead module: " << nodep);
            }
        }
    }

    శూన్య సందర్శించు(AstClass* nodep) override {  // FindVisitor::
        UASSERT_OBJ(m_curSymp, nodep, "Class not under module/package/$unit");
        UINFO(8, "   " << nodep);
        VL_RESTORER_COPY(m_scope);
        VL_RESTORER(m_classOrPackagep);
        VL_RESTORER(m_modSymp);
        VL_RESTORER(m_curSymp);
        VL_RESTORER(m_param_అంకె);
        VL_RESTORER(m_mod_ఖండం_అంకె);
        VL_RESTORER(m_mod_With_అంకె);
        VL_RESTORER(m_mod_Arg_అంకె);
        VL_RESTORER(m_explicitNew);
        {
            UINFO(4, "     Link Class: " << nodep);
            VSymEnt* స్థిరరాశి పై_Symp = m_curSymp;
            m_scope = m_scope + "." + nodep->name();
            m_classOrPackagep = nodep;
            m_curSymp = m_modSymp
                = m_statep->చొప్పించు_ఖండం(పై_Symp, nodep->name(), nodep, m_classOrPackagep);
            m_statep->చొప్పించు_Map(m_curSymp, m_scope);
            UINFO(9, "New module scope " << m_curSymp);
            //
            m_param_అంకె = 0;
            m_mod_ఖండం_అంకె = 0;
            m_mod_With_అంకె = 0;
            m_mod_Arg_అంకె = 0;
            m_explicitNew = అబద్ధం;
            // m_modSymp/m_curSymp for non-packages set by AstCell above this module
            // Iterate
            iterateChildren(nodep);
            nodep->user4(నిజం);
            // Implicit new needed?
            ఒకవేళ (!m_explicitNew && m_statep->forPrimary()) makeImplicitNew(nodep);
        }
    }
    శూన్య సందర్శించు(AstClassOrPackageRef* nodep) override {  // FindVisitor::
        ఒకవేళ (!nodep->classOrPackageNodep() && nodep->name() == "$unit") {
            nodep->classOrPackageNodep(v3Global.rootp()->dollarUnitPkgp());
        }
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(AstScope* nodep) override {  // FindVisitor::
        UASSERT_OBJ(m_statep->forScopeCreation(), nodep,
                    "Scopes should only exist right after V3Scope");
        // Detach on first encounter, will re add at the end.
        // Otherwise would be visited O(cells^2) time after V3Scope
        ఒకవేళ (!nodep->is_శిఖరం()) {
            ++m_statScopeVisits;
            nodep->unlink_Fr_వెనుక();
            m_scopes.నెట్టు_వెనుక(nodep);
        }
    }
    శూన్య సందర్శించు(AstCell* nodep) override {  // FindVisitor::
        UINFO(5, "   CELL under " << m_scope << " is " << nodep);
        // Process XREFs/etc inside pins
        ఒకవేళ (nodep->recursive() && m_లోపల_Recursion) తిరిగి;
        iterateChildren(nodep);
        // Recurse in, preserving state
        VL_RESTORER_COPY(m_scope);
        VL_RESTORER(m_modSymp);
        VL_RESTORER(m_curSymp);
        VL_RESTORER(m_param_అంకె);
        VL_RESTORER(m_లోపల_Recursion);
        // Where do we add it?
        VSymEnt* aboveSymp = m_curSymp;
        స్థిరరాశి string origname = Ast_కణుపు::dedotName(nodep->name());
        string::కొలత_type pos = origname.rfind('.');
        ఒకవేళ (pos != string::npos) {
            // Flattened, find what CellInline it should live under
            స్థిరరాశి string scope = origname.substr(0, pos);
            string baddot;
            VSymEnt* okSymp;
            aboveSymp
                = m_statep->కనుగొను_Dotted(nodep->fileline(), aboveSymp, scope, baddot, okSymp, అబద్ధం);
            UASSERT_OBJ(aboveSymp, nodep,
                        "Can't find instance insertion point at "
                            << Ast_కణుపు::prettyNameQ(baddot) << " in: " << nodep->prettyNameQ());
        }
        {
            m_scope = m_scope + "." + nodep->name();
            m_curSymp = m_modSymp = m_statep->చొప్పించు_Cell(aboveSymp, m_modSymp, nodep, m_scope);
            m_లోపల_Recursion = nodep->recursive();
            // We don't report NotFoundModule, as may be a unused module in a generate
            ఒకవేళ (nodep->modp()) iterate(nodep->modp());
        }
    }
    శూన్య సందర్శించు(AstCellInline* nodep) override {  // FindVisitor::
        UINFO(5, "   CELLINLINE under " << m_scope << " is " << nodep);
        VSymEnt* aboveSymp = m_curSymp;
        std::string ident = nodep->name();
        // If baz__DOT__foo__DOT__bar, we need to find baz__DOT__foo and add bar to it.
        స్థిరరాశి std::string::కొలత_type pos = ident.rfind("__DOT__");
        ఒకవేళ (pos != std::string::npos) {
            స్థిరరాశి std::string dotted = ident.substr(0, pos);
            ident = ident.substr(pos + std::అక్షరశ్రేణి_పొడవు("__DOT__"));
            std::string baddot;
            VSymEnt* okSymp;
            aboveSymp = m_statep->కనుగొను_Dotted(nodep->fileline(), aboveSymp, dotted, baddot, okSymp,
                                             అబద్ధం);
            UASSERT_OBJ(aboveSymp, nodep,
                        "Can't find cellinline insertion point at "
                            << Ast_కణుపు::prettyNameQ(baddot) << " in: " << nodep->prettyNameQ());
        }
        m_statep->చొప్పించు_Inline(aboveSymp, m_modSymp, nodep, ident);
    }
    శూన్య సందర్శించు(AstDefParam* nodep) override {  // FindVisitor::
        nodep->user1p(m_curSymp);
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(AstRefDType* nodep) override {  // FindVisitor::
        nodep->user1p(m_curSymp);
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(Ast_Gen_ఖండం* nodep) override {  // FindVisitor::
        UINFO(5, "   " << nodep);
        ఒకవేళ (nodep->name() == "" && nodep->unnamed()) {
            // Unnamed blocks are only important when they contain var
            // decls, so search for them. (Otherwise adding all the
            // unnamed#'s would just confuse tracing variables in
            // places such as tasks, where "task ...; begin ... end"
            // are common.
            కొరకు (Ast_కణుపు* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
                ఒకవేళ (VN_IS(itemp, Var) || VN_IS(itemp, Foreach)) {
                    std::string name;
                    స్థిరరాశి std::string అడుగు_Str = m_statep->forPrimary()
                                                    ? ""
                                                    : std::గమ్యం_string(m_statep->అడుగు_సంఖ్య()) + "_";
                    చేయి {
                        ++m_mod_ఖండం_అంకె;
                        name = "unnamedblk" + అడుగు_Str + cvt_గమ్యం_Str(m_mod_ఖండం_అంకె);
                        // Increment again if earlier pass of V3LinkDot claimed this name
                    } అయితే (m_curSymp->కనుగొను_Id_Flat(name));
                    nodep->name(name);
                    ఆపు;
                }
            }
        }
        ఒకవేళ (nodep->name() == "") {
            iterateChildren(nodep);
        } లేకపోతే {
            VL_RESTORER(m_curSymp);
            {
                m_curSymp
                    = m_statep->చొప్పించు_ఖండం(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
                m_curSymp->fallbackp(VL_RESTORER_PREV(m_curSymp));
                // Iterate
                iterateChildren(nodep);
            }
        }
    }
    శూన్య సందర్శించు(Ast_కణుపు_ఖండం* nodep) override {  // FindVisitor::
        UINFO(5, "   " << nodep);
        ఒకవేళ (nodep->name() == "" && nodep->unnamed()) {
            // Unnamed blocks are only important when they contain var
            // decls, so search for them. (Otherwise adding all the
            // unnamed#'s would just confuse tracing variables in
            // places such as tasks, where "task ...; begin ... end"
            // are common.
            స్థిరరాశి స్వయంచాలక కలిగియుంది_Decl = [](స్థిరరాశి Ast_కణుపు* nodesp) -> తార్కిక {
                కొరకు (స్థిరరాశి Ast_కణుపు* np = nodesp; np; np = np->nextp()) {
                    ఒకవేళ (VN_IS(np, Var) || VN_IS(np, Foreach)) తిరిగి నిజం;
                }
                తిరిగి అబద్ధం;
            };
            తార్కిక needName = nodep->declsp() || కలిగియుంది_Decl(nodep->stmtsp());
            ఒకవేళ (స్థిరరాశి AstFork* స్థిరరాశి forkp = VN_CAST(nodep, Fork)) {
                needName = needName || కలిగియుంది_Decl(forkp->forksp());
            }
            ఒకవేళ (needName) {
                స్థిరరాశి std::string అడుగు_Str
                    = m_statep->forPrimary() ? "" : std::గమ్యం_string(m_statep->అడుగు_సంఖ్య()) + "_";
                std::string name;
                చేయి {
                    ++m_mod_ఖండం_అంకె;
                    name = "unnamedblk" + అడుగు_Str + cvt_గమ్యం_Str(m_mod_ఖండం_అంకె);
                    // Increment again if earlier pass of V3LinkDot claimed this name
                } అయితే (m_curSymp->కనుగొను_Id_Flat(name));
                nodep->name(name);
            }
        }
        ఒకవేళ (nodep->name() == "") {
            iterateChildren(nodep);
        } లేకపోతే {
            VL_RESTORER(m_curSymp);
            {
                m_curSymp
                    = m_statep->చొప్పించు_ఖండం(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
                m_curSymp->fallbackp(VL_RESTORER_PREV(m_curSymp));
                // Iterate
                iterateChildren(nodep);
            }
        }
    }
    // Handle out-of-block interface method definition (IEEE 1800-2023 25.7)
    // Move task body from module into the interface's prototype task,
    // rewriting port-prefixed references in the body.
    శూన్య moveIfaceExportBody(Ast_కణుపు_FTask* nodep) {
        స్థిరరాశి string& పోర్టు_Name = nodep->iface_పోర్టు_Name();
        UINFO(5, "   moveIfaceExportBody: port=" << పోర్టు_Name << " task=" << nodep->name());
        // Look up the interface port variable in the module's symbol table
        VSymEnt* స్థిరరాశి పోర్టు_Symp = m_modSymp->కనుగొను_Id_Fallback(పోర్టు_Name);
        ఒకవేళ (!పోర్టు_Symp) {
            nodep->v3error("Interface port not found for out-of-block definition: '"
                           << పోర్టు_Name << "' (IEEE 1800-2023 25.7)");
            nodep->unlink_Fr_వెనుక();
            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
            తిరిగి;
        }
        AstVar* స్థిరరాశి portp = VN_CAST(పోర్టు_Symp->nodep(), Var);
        ఒకవేళ (!portp) {
            nodep->v3fatalSrc("Out-of-block definition port is not a variable: " << పోర్టు_Name);
            తిరిగి;
        }
        // Get the interface name from the port's type
        // At this stage dtypep() may not be set yet; check childDTypep() too
        AstIfaceRefDType* ifaceRefDtp = శూన్యసూచిక;
        ఒకవేళ (portp->dtypep()) { ifaceRefDtp = VN_CAST(portp->dtypep(), IfaceRefDType); }
        ఒకవేళ (!ifaceRefDtp && portp->బిడ్డ_DTypep()) {
            ifaceRefDtp = VN_CAST(portp->బిడ్డ_DTypep(), IfaceRefDType);
        }
        ఒకవేళ (!ifaceRefDtp) {
            nodep->v3error("Out-of-block definition port is not an interface port: '"
                           << పోర్టు_Name << "' (IEEE 1800-2023 25.7)");
            nodep->unlink_Fr_వెనుక();
            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
            తిరిగి;
        }
        స్థిరరాశి string ifaceName = ifaceRefDtp->ifaceName();
        UINFO(5, "   moveIfaceExportBody: iface=" << ifaceName);
        // Find the interface module by name
        Ast_కణుపు_Module* స్థిరరాశి ifaceModp = m_statep->కనుగొను_Module_Sym(ifaceName);
        ఒకవేళ (!ifaceModp) {
            nodep->v3fatalSrc("Interface not found for out-of-block definition: " << ifaceName);
            తిరిగి;
        }
        // Find the prototype task in the interface
        Ast_కణుపు_FTask* protoTaskp = శూన్యసూచిక;
        కొరకు (Ast_కణుపు* itemp = ifaceModp->stmtsp(); itemp; itemp = itemp->nextp()) {
            ఒకవేళ (Ast_కణుపు_FTask* స్థిరరాశి ftaskp = VN_CAST(itemp, కణుపు_FTask)) {
                ఒకవేళ (ftaskp->name() == nodep->name() && ftaskp->prototype()) {
                    protoTaskp = ftaskp;
                    ఆపు;
                }
            }
        }
        ఒకవేళ (!protoTaskp) {
            nodep->v3error("No matching export prototype found in interface "
                           << ifaceName << " for task " << nodep->prettyNameQ()
                           << " (IEEE 1800-2023 25.7)");
            nodep->unlink_Fr_వెనుక();
            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
            తిరిగి;
        }
        // Move body statements (non-port declarations) from the out-of-block task
        // to the prototype task. Rewrite port.X references to just X in the body.
        కొరకు (Ast_కణుపు* stmtp = nodep->stmtsp(); stmtp;) {
            Ast_కణుపు* స్థిరరాశి nextp = stmtp->nextp();
            // Skip port declarations -- they already exist in the prototype
            ఒకవేళ (స్థిరరాశి AstVar* స్థిరరాశి varp = VN_CAST(stmtp, Var)) {
                ఒకవేళ (varp->isIO()) {
                    stmtp = nextp;
                    కొనసాగు;
                }
            }
            // Clone this statement and rewrite port references
            Ast_కణుపు* స్థిరరాశి newStmtp = stmtp->clone_వృక్షం(అబద్ధం);
            rewrite_Iface_పోర్టు_Refs_Single(newStmtp, పోర్టు_Name);
            protoTaskp->కూడు_Stmtsp(newStmtp);
            stmtp = nextp;
        }
        // Mark prototype as no longer prototype (it has a body now)
        protoTaskp->prototype(అబద్ధం);
        // Delete the out-of-block task from the module
        nodep->unlink_Fr_వెనుక();
        VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
    }

    // Rewrite AstDot nodes where lhsp is AstParseRef with the port name
    // to just the rhsp (stripping the port prefix).
    స్థిర శూన్య rewrite_Iface_పోర్టు_Refs_Single(Ast_కణుపు* nodep, స్థిరరాశి string& పోర్టు_Name) {
        ఒకవేళ (!nodep) తిరిగి;
        // Check if this is a non-colon Dot with lhs matching portName
        ఒకవేళ (AstDot* స్థిరరాశి dotp = VN_CAST(nodep, Dot)) {
            ఒకవేళ (!dotp->colon()) {
                స్థిరరాశి AstParseRef* స్థిరరాశి lhsRefp = VN_CAST(dotp->lhsp(), ParseRef);
                ఒకవేళ (lhsRefp && lhsRefp->name() == పోర్టు_Name) {
                    UINFO(5, "   rewriteIfacePortRefs: stripping port prefix from " << dotp);
                    Ast_కణుపు* స్థిరరాశి rhsp = dotp->rhsp()->unlink_Fr_వెనుక();
                    dotp->replaceWith(rhsp);
                    VL_DO_DANGLING(dotp->delete_వృక్షం(), dotp);
                    rewrite_Iface_పోర్టు_Refs_Single(rhsp, పోర్టు_Name);
                    తిరిగి;
                }
            }
        }
        // Recurse into all children
        కొరకు (Ast_కణుపు* childp : {nodep->op1p(), nodep->op2p(), nodep->op3p(), nodep->op4p()}) {
            అయితే (childp) {
                Ast_కణుపు* స్థిరరాశి nextp = childp->nextp();
                rewrite_Iface_పోర్టు_Refs_Single(childp, పోర్టు_Name);
                childp = nextp;
            }
        }
    }

    శూన్య సందర్శించు(Ast_కణుపు_FTask* nodep) override {  // FindVisitor::
        // NodeTask: Remember its name for later resolution
        UINFO(5, "   " << nodep);
        UASSERT_OBJ(m_curSymp && m_modSymp, nodep, "Function/Task not under module?");
        ఒకవేళ (nodep->name() == "new") m_explicitNew = నిజం;
        // Handle out-of-block interface method definition (IEEE 25.8)
        ఒకవేళ (!nodep->iface_పోర్టు_Name().ఖాళీలు() && m_statep->forPrimary()) {
            moveIfaceExportBody(nodep);
            తిరిగి;  // Task has been deleted
        }
        // Remember the existing symbol table scope
        VL_RESTORER(m_classOrPackagep);
        VL_RESTORER(m_curSymp);
        VSymEnt* పైకి_Symp = m_curSymp;

        ఒకవేళ (VN_IS(m_curSymp->nodep(), Class)) {
            ఒకవేళ (VN_AS(m_curSymp->nodep(), Class)->isInterfaceClass() && !nodep->శుద్ధ_Virtual()
                && !nodep->isConstructor()) {
                nodep->v3error("Interface class functions must be pure virtual"
                               << " (IEEE 1800-2023 8.26): " << nodep->prettyNameQ());
            }
            ఒకవేళ (m_statep->forPrimary()
                && (nodep->name() == "randomize" || nodep->name() == "srandom")) {
                nodep->v3error(nodep->prettyNameQ()
                               << " is a predefined class method; redefinition not allowed"
                                  " (IEEE 1800-2023 18.6.3)");
            }
        }
        // Change to appropriate package if extern declaration (vs definition)
        ఒకవేళ (nodep->classOrPackagep()) {
            // class-in-class
            AstDot* స్థిరరాశి dotp = VN_CAST(nodep->classOrPackagep(), Dot);
            AstClassOrPackageRef* స్థిరరాశి cpackagerefp
                = VN_CAST(nodep->classOrPackagep(), ClassOrPackageRef);
            ఒకవేళ (dotp) {
                AstClassOrPackageRef* స్థిరరాశి lhsp = VN_AS(dotp->lhsp(), ClassOrPackageRef);
                m_statep->resolveClassOrPackage(m_curSymp, lhsp, నిజం, అబద్ధం,
                                                "External definition :: reference");
                AstClass* స్థిరరాశి lhsclassp = VN_CAST(lhsp->classOrPackageSkipp(), Class);
                ఒకవేళ (!lhsclassp) {
                    nodep->v3error("Extern declaration's scope is not a defined class");
                } లేకపోతే {
                    m_curSymp = m_statep->get_కణుపు_Sym(lhsclassp);
                    పైకి_Symp = m_curSymp;
                    AstClassOrPackageRef* స్థిరరాశి rhsp = VN_AS(dotp->rhsp(), ClassOrPackageRef);
                    m_statep->resolveClassOrPackage(m_curSymp, rhsp, నిజం, అబద్ధం,
                                                    "External definition :: reference");
                    AstClass* స్థిరరాశి rhsclassp = VN_CAST(rhsp->classOrPackageSkipp(), Class);
                    ఒకవేళ (!rhsclassp) {
                        nodep->v3error("Extern declaration's scope is not a defined class");
                    } లేకపోతే {
                        m_curSymp = m_statep->get_కణుపు_Sym(rhsclassp);
                        పైకి_Symp = m_curSymp;
                        moveExternFuncDecl(nodep, rhsclassp);
                    }
                }
            } లేకపోతే ఒకవేళ (cpackagerefp) {
                ఒకవేళ (!cpackagerefp->classOrPackageSkipp()) {
                    m_statep->resolveClassOrPackage(m_curSymp, cpackagerefp, నిజం, అబద్ధం,
                                                    "External definition :: reference");
                }
                AstClass* స్థిరరాశి classp = VN_CAST(cpackagerefp->classOrPackageSkipp(), Class);
                ఒకవేళ (!classp) {
                    nodep->v3error("Extern declaration's scope is not a defined class");
                } లేకపోతే {
                    m_curSymp = m_statep->get_కణుపు_Sym(classp);
                    పైకి_Symp = m_curSymp;
                    moveExternFuncDecl(nodep, classp);
                }
            } లేకపోతే {
                v3fatalSrc("Unhandled extern function definition package");
            }
        }
        // Set the class as package for iteration
        ఒకవేళ (VN_IS(m_curSymp->nodep(), Class)) {
            m_classOrPackagep = VN_AS(m_curSymp->nodep(), Class);
        }
        // Create symbol table for the task's vars
        స్థిరరాశి string name = (nodep->isExternProto() ? "extern "s : ""s) + nodep->name();
        m_curSymp = m_statep->చొప్పించు_ఖండం(m_curSymp, name, nodep, m_classOrPackagep);
        m_curSymp->fallbackp(పైకి_Symp);
        // Convert the func's range to the output variable
        // This should probably be done in the Parser instead, as then we could
        // just attach normal signal attributes to it.
        ఒకవేళ (!nodep->isExternProto() && nodep->fvarp() && !VN_IS(nodep->fvarp(), Var)) {
            Ast_కణుపు_DType* dtypep = VN_CAST(nodep->fvarp(), కణుపు_DType);
            // If unspecified, function returns one bit; however when we
            // support NEW() it could also return the class reference.
            ఒకవేళ (dtypep) {
                dtypep->unlink_Fr_వెనుక();
            } లేకపోతే {
                dtypep = కొత్త AstBasicDType{nodep->fileline(), VBasicDTypeKwd::LOGIC};
            }
            AstVar* స్థిరరాశి newvarp
                = కొత్త AstVar{nodep->fileline(), VVarType::VAR, nodep->name(), VFlag_బిడ్డ_DType{},
                             dtypep};  // Not dtype resolved yet
            newvarp->direction(VDirection::OUTPUT);
            newvarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            newvarp->funcReturn(నిజం);
            newvarp->trace(అబద్ధం);  // Not user visible
            nodep->fvarp(newvarp);
            // Explicit insert required, as the var name shadows the upper level's task name
            m_statep->చొప్పించు_Sym(m_curSymp, newvarp->name(), newvarp, శూన్యసూచిక /*classOrPackagep*/);
        }
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(AstClocking* nodep) override {  // FindVisitor::
        VL_RESTORER(m_clockingp);
        m_clockingp = nodep;
        iterate(nodep->sentreep());
        iterate_And_తదుపరి_Null(nodep->itemsp());
        // If the block has no name, one cannot reference the clockvars
        VSymEnt* itSymp = శూన్యసూచిక;
        ఒకవేళ (nodep->isGlobal()  //
            && m_statep->forPrimary()) {  // else flattening may see two globals
            m_statep->checkDuplicate(m_curSymp, nodep, "__024global_clock");
            itSymp
                = m_statep->చొప్పించు_ఖండం(m_curSymp, "__024global_clock", nodep, m_classOrPackagep);
            itSymp->fallbackp(శూన్యసూచిక);
        }
        ఒకవేళ (!nodep->name().ఖాళీలు()) {
            itSymp = m_statep->చొప్పించు_ఖండం(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
            itSymp->fallbackp(శూన్యసూచిక);
        }
        ఒకవేళ (itSymp) {
            VL_RESTORER(m_curSymp);
            m_curSymp = itSymp;
            iterate_And_తదుపరి_Null(nodep->itemsp());
        }
    }
    శూన్య సందర్శించు(Ast_Clocking_అంశం* nodep) override {  // FindVisitor::
        ఒకవేళ (nodep->varp()) {
            ఒకవేళ (m_curSymp->nodep() == m_clockingp) iterate(nodep->varp());
            తిరిగి;
        }
        std::string varname;
        Ast_కణుపు_DType* dtypep;
        ఒకవేళ (Ast_కేటాయించు* స్థిరరాశి assignp = nodep->assignp()) {
            Ast_కణుపు_సమాసం* స్థిరరాశి rhsp = assignp->rhsp()->unlink_Fr_వెనుక();
            dtypep = కొత్త AstRefDType{nodep->fileline(), AstRefDType::Flag_Type_యొక్క_సమాసం{},
                                     rhsp->clone_వృక్షం(అబద్ధం)};
            nodep->exprp(rhsp);
            varname = assignp->lhsp()->name();
            VL_DO_DANGLING(assignp->unlink_Fr_వెనుక()->delete_వృక్షం(), assignp);
        } లేకపోతే {
            Ast_కణుపు_సమాసం* స్థిరరాశి refp = nodep->exprp();
            స్థిరరాశి VSymEnt* foundp = m_curSymp->కనుగొను_Id_Fallback(refp->name());
            ఒకవేళ (!foundp || !foundp->nodep()) {
                refp->v3error("Corresponding variable " << refp->prettyNameQ()
                                                        << " does not exist");
                VL_DO_DANGLING(nodep->unlink_Fr_వెనుక()->delete_వృక్షం(), nodep);
                తిరిగి;
            }
            varname = refp->name();
            dtypep = VN_AS(foundp->nodep(), Var)->బిడ్డ_DTypep()->clone_వృక్షం(అబద్ధం);
        }
        AstVar* స్థిరరాశి newvarp = కొత్త AstVar{nodep->fileline(), VVarType::MODULETEMP, varname,
                                           VFlag_బిడ్డ_DType{}, dtypep};
        newvarp->lifetime(VLifetime::STATIC_EXPLICIT);
        nodep->varp(newvarp);
        iterate(nodep->exprp());
    }
    శూన్య సందర్శించు(AstConstraint* nodep) override {  // FindVisitor::
        VL_RESTORER(m_curSymp);
        // Change to appropriate package if extern declaration (vs definition)
        VSymEnt* పైకి_Symp = m_curSymp;
        ఒకవేళ (nodep->classOrPackagep()) {
            AstClassOrPackageRef* స్థిరరాశి cpackagerefp
                = VN_CAST(nodep->classOrPackagep(), ClassOrPackageRef);
            ఒకవేళ (!cpackagerefp) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: extern constraint definition with class-in-class");
            } లేకపోతే {
                ఒకవేళ (!cpackagerefp->classOrPackageSkipp()) {
                    m_statep->resolveClassOrPackage(m_curSymp, cpackagerefp, నిజం, అబద్ధం,
                                                    "External definition :: reference");
                }
                AstClass* స్థిరరాశి classp = VN_CAST(cpackagerefp->classOrPackageSkipp(), Class);
                ఒకవేళ (!classp) {
                    nodep->v3error("Extern declaration's scope is not a defined class");
                } లేకపోతే {
                    m_curSymp = m_statep->get_కణుపు_Sym(classp);
                    పైకి_Symp = m_curSymp;
                    // Move it to proper spot under the target class
                    nodep->unlink_Fr_వెనుక();
                    classp->కూడు_Stmtsp(nodep);
                    nodep->classOrPackagep()->unlink_Fr_వెనుక()->delete_వృక్షం();
                }
            }
        }
        // Set the class as package for iteration
        స్థిరరాశి string name = (nodep->isExternProto() ? "extern "s : ""s) + nodep->name();
        m_curSymp = m_statep->చొప్పించు_ఖండం(పైకి_Symp, name, nodep, m_classOrPackagep);
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(AstVar* nodep) override {  // FindVisitor::
        // Var: Remember its name for later resolution
        UASSERT_OBJ(m_curSymp && m_modSymp, nodep, "Var not under module?");
        iterateChildren(nodep);
        ఒకవేళ (VN_IS(m_curSymp->nodep(), Class)
            && VN_AS(m_curSymp->nodep(), Class)->isInterfaceClass() && !nodep->isParam()) {
            nodep->v3error("Interface class cannot contain non-parameter members"
                           << " (IEEE 1800-2023 8.26): " << nodep->prettyNameQ());
        }
        ఒకవేళ (!m_statep->forScopeCreation()) {
            // Find under either a task or the module's vars
            స్థిరరాశి VSymEnt* foundp = m_curSymp->కనుగొను_Id_Fallback(nodep->name());
            ఒకవేళ (!foundp && m_modSymp && nodep->name() == m_modSymp->nodep()->name()) {
                foundp = m_modSymp;  // Conflicts with modname?
            }
            AstVar* స్థిరరాశి findvarp = foundp ? VN_CAST(foundp->nodep(), Var) : శూన్యసూచిక;
            // clocking items can have duplicate names (inout)
            ఒకవేళ (findvarp && VN_IS(findvarp->backp(), Clocking_అంశం)
                && VN_IS(nodep->backp(), Clocking_అంశం)) {
                Ast_Clocking_అంశం* స్థిరరాశి itemp = VN_AS(nodep->backp(), Clocking_అంశం);
                Ast_Clocking_అంశం* స్థిరరాశి finditemp = VN_AS(findvarp->backp(), Clocking_అంశం);
                UINFO(4, "ClockCompl: " << itemp << " ;; " << finditemp);
                UINFO(4, "ClockCompV: " << nodep << " ;; " << findvarp);
                ఒకవేళ (*itemp->exprp()->fileline() == *finditemp->exprp()->fileline()) {
                    UASSERT_OBJ(finditemp->direction() == VDirection::ఆదానం
                                    && itemp->direction() == VDirection::OUTPUT,
                                itemp, "Input after output?");
                    // pretend nothing found and rename
                    foundp = శూన్యసూచిక;
                    nodep->name("__Voutput_" + nodep->name());
                    finditemp->outputp(itemp);
                }
            }
            తార్కిక ins = అబద్ధం;
            ఒకవేళ (!foundp) {
                ins = నిజం;
            } లేకపోతే ఒకవేళ (!findvarp && m_curSymp->కనుగొను_Id_Flat(nodep->name()) && !foundp->imported()) {
                nodep->v3error("Unsupported in C: Variable has same name as "
                               << LinkDotState::కణుపు_పాఠ్యం_Type(foundp->nodep()) << ": "
                               << nodep->prettyNameQ());
            } లేకపోతే ఒకవేళ (findvarp != nodep) {
                UINFO(4, "DupVar: " << nodep << " ;; " << foundp->nodep());
                UINFO(4, "    found  cur=se" << cvt_గమ్యం_Hex(m_curSymp) << " ;; parent=se"
                                             << cvt_గమ్యం_Hex(foundp->parentp()));
                ఒకవేళ (foundp->parentp() == m_curSymp  // Only when on same level
                    && !foundp->imported()) {  // and not from package
                    ఒకవేళ (VN_IS(m_curSymp->nodep(), Clocking)) {
                        nodep->v3error("Multiple clockvars with the same name not allowed");
                        తిరిగి;
                    }
                    స్థిరరాశి తార్కిక nansiBad
                        = (((findvarp->isDeclTyped() || findvarp->isNet())
                            && (nodep->isDeclTyped() || nodep->isNet()))
                           || (findvarp->isIO() && nodep->isIO())  // e.g. !(output && output)
                           || findvarp->gotNansiType());  // e.g. int x && int x
                    స్థిరరాశి తార్కిక ansiBad
                        = findvarp->isAnsi() || nodep->isAnsi();  // dup illegal with ANSI
                    ఒకవేళ (ansiBad || nansiBad) {
                        తార్కిక ansiWarn = ansiBad && !nansiBad;
                        ఒకవేళ (ansiWarn) {
                            స్థిర పూర్ణాంకం s_didAnsiWarn = అబద్ధం;
                            ఒకవేళ (s_didAnsiWarn++) ansiWarn = అబద్ధం;
                        }
                        nodep->v3error("Duplicate declaration of signal: "
                                       << nodep->prettyNameQ() << '\n'
                                       << (ansiWarn ? nodep->warn_మరిన్ని()
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
                    } లేకపోతే {
                        findvarp->combineType(nodep);
                        findvarp->fileline()->modifyStateInherit(nodep->fileline());
                        findvarp->gotNansiType(నిజం);
                        UASSERT_OBJ(nodep->subDTypep(), nodep, "Var has no type");
                        ఒకవేళ (nodep->subDTypep()->numeric().isSigned()
                            && !findvarp->subDTypep()->numeric().isSigned()) {
                            findvarp->subDTypep()->numeric(VSigning{నిజం});
                        }
                        Ast_కణుపు_DType* స్థిరరాశి varDtp = findvarp->subDTypep();
                        Ast_కణుపు_DType* స్థిరరాశి otherDtp = nodep->subDTypep();
                        ఒకవేళ (varDtp
                            && (VN_IS(varDtp, BasicDType)
                                && VN_AS(varDtp, BasicDType)->implicit())) {
                            // Then have "input foo" and "real foo" so the
                            // dtype comes from the other side.
                            VL_DO_DANGLING(varDtp->unlink_Fr_వెనుక()->delete_వృక్షం(), varDtp);
                            findvarp->బిడ్డ_DTypep(otherDtp->unlink_Fr_వెనుక());
                        } లేకపోతే ఒకవేళ (!m_statep->forScopeCreation() && otherDtp && varDtp
                                   && !(VN_IS(otherDtp, BasicDType)
                                        && VN_AS(otherDtp, BasicDType)->implicit())) {
                            // otherDtp and varDtp both non-nullptr and neither are implicit
                            // Can't compare dtypes now as might contain parameters,
                            // defer to V3Width
                            Ast_Attr_యొక్క* స్థిరరాశి newp
                                = కొత్త Ast_Attr_యొక్క{otherDtp->fileline(), VAttrType::VAR_పోర్టు_DTYPE,
                                                otherDtp->unlink_Fr_వెనుక()};
                            findvarp->కూడు_Attrsp(newp);
                        }
                    }
                    VL_DO_DANGLING(nodep->unlink_Fr_వెనుక()->delete_వృక్షం(), nodep);
                } లేకపోతే {
                    // User can disable the message at either point
                    ఒకవేళ (!(m_ftaskp && m_ftaskp->dpiImport())
                        && (!m_ftaskp || m_ftaskp != foundp->nodep())  // Not the function's
                                                                       // variable hiding function
                        && !nodep->fileline()->warnIsOff(V3_Error_సంకేతం::VARHIDDEN)
                        && !foundp->nodep()->fileline()->warnIsOff(V3_Error_సంకేతం::VARHIDDEN)) {
                        nodep->v3warn(VARHIDDEN,
                                      "Declaration of signal hides declaration in upper scope: "
                                          << nodep->prettyNameQ() << '\n'
                                          << nodep->warnContextPrimary() << '\n'
                                          << foundp->nodep()->warnOther()
                                          << "... Location of original declaration\n"
                                          << foundp->nodep()->warnContextSecondary());
                    }
                    ins = నిజం;
                }
            }
            ఒకవేళ (ins) {
                ఒకవేళ (m_statep->forPrimary() && nodep->isGParam()
                    && VN_IS(m_modSymp->nodep(), Module)
                    && (m_statep->మూలం_Entp()->nodep() == m_modSymp->parentp()->nodep())) {
                    // This is the toplevel module. Check for command line overwrites of parameters
                    // We first search if the parameter is overwritten and then replace it with a
                    // new value.
                    ఒకవేళ (v3Global.opt.hasParameter(nodep->name())) {
                        స్థిరరాశి string svalue = v3Global.opt.parameter(nodep->name());
                        ఒకవేళ (AstConst* స్థిరరాశి valuep
                            = AstConst::parseParamLiteral(nodep->fileline(), svalue)) {
                            UINFO(9, "       replace parameter " << nodep);
                            UINFO(9, "       with " << valuep);
                            ఒకవేళ (nodep->valuep()) నెట్టు_Deletep(nodep->valuep()->unlink_Fr_వెనుక());
                            nodep->valuep(valuep);
                            // Do not warn ENUMVALUE when overriding an enum param from the cli.
                            nodep->fileline()->warnOff(V3_Error_సంకేతం::ENUMVALUE, నిజం);
                        }
                    }
                }
                m_statep->చొప్పించు_Sym(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
                ఒకవేళ (m_statep->forPrimary() && nodep->isGParam()) {
                    ++m_param_అంకె;
                    VSymEnt* స్థిరరాశి symp
                        = m_statep->చొప్పించు_Sym(m_curSymp, "__paramNumber" + cvt_గమ్యం_Str(m_param_అంకె),
                                              nodep, m_classOrPackagep);
                    symp->exported(అబద్ధం);
                }
            }
        }
    }
    శూన్య సందర్శించు(AstTypedef* nodep) override {  // FindVisitor::
        UASSERT_OBJ(m_curSymp, nodep, "Typedef not under module/package/$unit");
        ఒకవేళ (VN_IS(m_classOrPackagep, Class)) nodep->isUnderClass(నిజం);
        iterateChildren(nodep);
        m_statep->చొప్పించు_Sym(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
    }
    శూన్య సందర్శించు(AstTypedefFwd* nodep) override {  // FindVisitor::
        UASSERT_OBJ(m_curSymp, nodep, "Typedef not under module/package/$unit");
        iterateChildren(nodep);
        // No need to insert, only the real typedef matters, but need to track for errors
        nodep->user1p(m_curSymp);
    }
    శూన్య సందర్శించు(Ast_కణుపు_UOr_Struct_DType* nodep) override {  // FindVisitor::
        UASSERT_OBJ(m_curSymp, nodep, "Struct/union dtype not under module/package/$unit");
        VL_RESTORER(m_curSymp);
        m_curSymp = m_statep->చొప్పించు_ఖండం(m_curSymp, "__Vdtype" + cvt_గమ్యం_Str(nodep->unique_అంకె()),
                                          nodep, m_classOrPackagep);
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(AstMemberDType* nodep) override {  // FindVisitor::
        iterateChildren(nodep);
        m_statep->చొప్పించు_Sym(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
    }
    శూన్య సందర్శించు(AstParamTypeDType* nodep) override {  // FindVisitor::
        UASSERT_OBJ(m_curSymp, nodep, "Parameter type not under module/package/$unit");

        // Replace missing param types with provided hierarchical type params.
        ఒకవేళ (!m_hierParamsName.ఖాళీలు()) {
            ఒకవేళ (స్థిరరాశి VSymEnt* స్థిరరాశి typedefEntp = m_curSymp->కనుగొను_Id_Fallback(m_hierParamsName)) {
                స్థిరరాశి AstModule* modp = VN_CAST(typedefEntp->nodep(), Module);

                కొరకు (స్థిరరాశి Ast_కణుపు* stmtp = modp ? modp->stmtsp() : శూన్యసూచిక; stmtp;
                     stmtp = stmtp->nextp()) {
                    స్థిరరాశి AstTypedef* tdefp = VN_CAST(stmtp, Typedef);

                    ఒకవేళ (tdefp && tdefp->name() == nodep->name() && m_statep->forPrimary()) {
                        UINFO(8, "Replacing type of" << nodep << "  -->with " << tdefp);
                        Ast_కణుపు_DType* స్థిరరాశి newDtp = tdefp->బిడ్డ_DTypep();
                        Ast_కణుపు_DType* స్థిరరాశి oldDtp = nodep->బిడ్డ_DTypep();

                        oldDtp->replaceWith(newDtp->clone_వృక్షం(అబద్ధం));
                        VL_DO_DANGLING(oldDtp->delete_వృక్షం(), oldDtp);
                    }
                }
            }
        }

        iterateChildren(nodep);
        m_statep->చొప్పించు_Sym(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
        ఒకవేళ (m_statep->forPrimary() && nodep->isGParam()) {
            ++m_param_అంకె;
            VSymEnt* స్థిరరాశి symp = m_statep->చొప్పించు_Sym(
                m_curSymp, "__paramNumber" + cvt_గమ్యం_Str(m_param_అంకె), nodep, m_classOrPackagep);
            symp->exported(అబద్ధం);
        }
    }
    శూన్య సందర్శించు(AstCFunc* nodep) override {  // FindVisitor::
        // For dotted resolution, ignore all AstVars under functions, otherwise shouldn't exist
        UASSERT_OBJ(!m_statep->forScopeCreation(), nodep, "No CFuncs expected in tree yet");
    }
    శూన్య సందర్శించు(Ast_Enum_అంశం* nodep) override {  // FindVisitor::
        // EnumItem: Remember its name for later resolution
        iterateChildren(nodep);
        // Find under either a task or the module's vars
        స్థిరరాశి VSymEnt* foundp = m_curSymp->కనుగొను_Id_Fallback(nodep->name());
        ఒకవేళ (!foundp && m_modSymp && nodep->name() == m_modSymp->nodep()->name()) {
            foundp = m_modSymp;  // Conflicts with modname?
        }
        Ast_Enum_అంశం* స్థిరరాశి findvarp = foundp ? VN_CAST(foundp->nodep(), Enum_అంశం) : శూన్యసూచిక;
        తార్కిక ins = అబద్ధం;
        ఒకవేళ (!foundp) {
            ins = నిజం;
        } లేకపోతే ఒకవేళ (findvarp != nodep) {
            UINFO(4, "DupVar: " << nodep << " ;; " << foundp);
            ఒకవేళ (foundp->parentp() == m_curSymp  // Only when on same level
                && !foundp->imported()) {  // and not from package
                nodep->v3error("Duplicate declaration of enum value: "
                               << nodep->prettyName() << '\n'
                               << nodep->warnContextPrimary() << '\n'
                               << foundp->nodep()->warnOther()
                               << "... Location of original declaration\n"
                               << foundp->nodep()->warnContextSecondary());
            } లేకపోతే {
                // User can disable the message at either point
                ఒకవేళ (!nodep->fileline()->warnIsOff(V3_Error_సంకేతం::VARHIDDEN)
                    && !foundp->nodep()->fileline()->warnIsOff(V3_Error_సంకేతం::VARHIDDEN)) {
                    nodep->v3warn(VARHIDDEN,
                                  "Declaration of enum value hides declaration in upper scope: "
                                      << nodep->prettyName() << '\n'
                                      << nodep->warnContextPrimary() << '\n'
                                      << foundp->nodep()->warnOther()
                                      << "... Location of original declaration\n"
                                      << foundp->nodep()->warnContextSecondary());
                }
                ins = నిజం;
            }
        }
        ఒకవేళ (ins) m_statep->చొప్పించు_Sym(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
    }
    శూన్య సందర్శించు(AstPackageImport* nodep) override {  // FindVisitor::
        UINFO(4, "  Link: " << nodep);
        ఒకవేళ (!nodep->packagep()) తిరిగి;  // Errored in V3LinkCells
        VSymEnt* స్థిరరాశి srcp = m_statep->get_కణుపు_Sym(nodep->packagep());
        ఒకవేళ (nodep->name() == "*") {
            ఒకవేళ (nodep->packagep() != v3Global.rootp()->stdPackagep()) {
                ఒకవేళ (m_curSymp == m_statep->dunitEntp()) {
                    nodep->v3warn(IMPORTSTAR,
                                  "'import::*' in $unit scope may pollute global namespace");
                }
            }
        } లేకపోతే ఒకవేళ (!nodep->resolvedClassp()) {
            VSymEnt* స్థిరరాశి impp = srcp->కనుగొను_Id_Flat(nodep->name());
            ఒకవేళ (!impp) {
                nodep->v3error("Import object not found: " << nodep->prettyPkgNameQ());
            } లేకపోతే ఒకవేళ (nodep->resolvedClassp()) {
                UASSERT_OBJ(nodep->resolvedClassp()->classOrPackageSkipp() == impp->nodep(), nodep,
                            "Import should resolve to the same node");
            } లేకపోతే ఒకవేళ (AstClass* స్థిరరాశి classp = VN_CAST(impp->nodep(), Class)) {
                nodep->resolvedClassp(
                    కొత్త AstClassOrPackageRef{nodep->fileline(), classp->name(), classp, శూన్యసూచిక});
            }
        }
        m_curSymp->importFromPackage(m_statep->symsp(), srcp, nodep->name());
        UINFO(9, "    Link Done: " << nodep);
        // No longer needed, but can't delete until any multi-instantiated modules are expanded
    }
    శూన్య సందర్శించు(AstPackageExport* nodep) override {  // FindVisitor::
        UINFO(9, "  Link: " << nodep);
        ఒకవేళ (!nodep->packagep()) తిరిగి;  // Errored in V3LinkCells
        VSymEnt* స్థిరరాశి srcp = m_statep->get_కణుపు_Sym(nodep->packagep());
        ఒకవేళ (nodep->name() != "*") {
            VSymEnt* స్థిరరాశి impp = srcp->కనుగొను_Id_Flat(nodep->name());
            ఒకవేళ (!impp) {
                nodep->v3error("Export object not found: '" << nodep->packagep()->prettyName()
                                                            << "::" << nodep->prettyName() << "'");
            }
        }
        m_curSymp->exportFromPackage(m_statep->symsp(), srcp, nodep->name());
        UINFO(9, "    Link Done: " << nodep);
        // No longer needed, but can't delete until any multi-instantiated modules are expanded
    }
    శూన్య సందర్శించు(Ast_Package_Export_నక్షత్రం_నక్షత్రం* nodep) override {  // FindVisitor::
        UINFO(4, "  Link: " << nodep);
        m_curSymp->export_నక్షత్రం_నక్షత్రం(m_statep->symsp());
        // No longer needed, but can't delete until any multi-instantiated modules are expanded
    }

    శూన్య సందర్శించు(Ast_కణుపు_Foreach* nodep) override {  // FindVisitor::
        // Symbol table needs nodep->name() as the index variable's name
        VL_RESTORER(m_curSymp);
        {
            ++m_mod_With_అంకె;
            m_curSymp = m_statep->చొప్పించు_ఖండం(m_curSymp, "__Vforeach" + cvt_గమ్యం_Str(m_mod_With_అంకె),
                                              nodep, m_classOrPackagep);
            m_curSymp->fallbackp(VL_RESTORER_PREV(m_curSymp));
            కొరకు (Ast_కణుపు *nextp, *argp = nodep->headerp()->elementsp(); argp; argp = nextp) {
                nextp = argp->nextp();
                AstVar* argrefp = శూన్యసూచిక;
                ఒకవేళ (AstParseRef* స్థిరరాశి parserefp = VN_CAST(argp, ParseRef)) {
                    // IEEE 1800-2023 12.7.3: foreach loop variable type shall be int (2-state)
                    // This might get changed in V3Width when types resolve (e.g., for assoc
                    // arrays)
                    argrefp = కొత్త AstVar{parserefp->fileline(), VVarType::BLOCKTEMP,
                                         parserefp->name(), argp->కనుగొను_Int_DType()};
                    argrefp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                    parserefp->replaceWith(argrefp);
                    VL_DO_DANGLING2(parserefp->delete_వృక్షం(), parserefp, argp);
                    // Insert argref's name into symbol table
                    m_statep->చొప్పించు_Sym(m_curSymp, argrefp->name(), argrefp, శూన్యసూచిక);
                } లేకపోతే ఒకవేళ (స్థిరరాశి స్వయంచాలక largrefp = VN_CAST(argp, Var)) {
                    argrefp = largrefp;
                    // Insert argref's name into symbol table
                    m_statep->చొప్పించు_Sym(m_curSymp, argrefp->name(), argrefp, శూన్యసూచిక);
                } లేకపోతే ఒకవేళ (VN_IS(argp, ఖాళీలు_2)) {
                } లేకపోతే {
                    argp->v3error("'foreach' loop variable expects simple variable name");
                }
            }
            iterateChildren(nodep);
        }
    }

    శూన్య సందర్శించు(AstRandSequence* nodep) override {
        VL_RESTORER(m_curSymp);
        m_curSymp = m_statep->చొప్పించు_ఖండం(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(AstRSProd* nodep) override {
        VL_RESTORER(m_curSymp);
        m_curSymp = m_statep->చొప్పించు_ఖండం(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
        ఒకవేళ (nodep->fvarp())
            nodep->fvarp()->v3warn(E_UNSUPPORTED,
                                   "Unsupported: randsequence production function variable");
        // Mark formal ports as port-checked so the primary resolve pass does not
        // flag them with "does not appear in port list" -- V3RandSequence will
        // later move them onto the generated task as real input ports.
        కొరకు (Ast_కణుపు* itp = nodep->portsp(); itp; itp = itp->nextp()) {
            VN_AS(itp, Var)->user4(నిజం);
        }
        iterateChildren(nodep);
    }

    శూన్య సందర్శించు(AstWithParse* nodep) override {  // FindVisitor::
        // Change WITHPARSE(FUNCREF, equation) to FUNCREF(WITH(equation))
        Ast_కణుపు_FTask_Ref* funcrefp = VN_CAST(nodep->funcrefp(), కణుపు_FTask_Ref);
        ఒకవేళ (స్థిరరాశి AstDot* స్థిరరాశి dotp = VN_CAST(nodep->funcrefp(), Dot))
            funcrefp = VN_CAST(dotp->rhsp(), కణుపు_FTask_Ref);
        UASSERT_OBJ(funcrefp, nodep, "'with' only can operate on a function/task");
        string name = funcrefp->name() == "randomize" ? "__Vrandwith_obj" : "item";
        File_పంక్తి* argFl = nodep->fileline();
        AstArg* స్థిరరాశి argsp = funcrefp->argsp();
        ఒకవేళ (argsp) {
            argsp->unlink_Fr_వెనుక_With_తదుపరి();
            ఒకవేళ (funcrefp->name() != "randomize") {
                ఒకవేళ (స్థిరరాశి స్వయంచాలక parserefp = VN_CAST(argsp->exprp(), ParseRef)) {
                    name = parserefp->name();
                    argFl = parserefp->fileline();
                } లేకపోతే {
                    argsp->v3error("'with' function expects simple variable name");
                }
                ఒకవేళ (argsp->nextp()) {
                    argsp->nextp()->v3error("'with' function expects only up to one argument");
                }
                VL_DO_DANGLING(argsp->delete_వృక్షం(), argsp);
            }
        }
        // Type depends on the method used, let V3Width figure it out later
        // 'with (...) { ... }' constraint_block form is randomize-only
        // (IEEE 1800-2023 7.12 vs. 18.7); array methods take 'with (expr)'.
        ఒకవేళ (nodep->restricted() && funcrefp->name() != "randomize") {
            nodep->v3error("'with (...) { ... }' constraint block is only valid for"
                           " randomize() (IEEE 1800-2023 18.7)");
            funcrefp->కూడు_Argsp(argsp);
            nodep->replaceWith(nodep->funcrefp()->unlink_Fr_వెనుక());
            VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
            తిరిగి;
        }
        స్థిరరాశి తార్కిక restrictedRandomize = nodep->restricted();
        ఒకవేళ (nodep->exprsp() || nodep->constraintsp()
            || restrictedRandomize) {  // Else empty expression and pretend no "with"
            AstLambdaArgRef* స్థిరరాశి సూచీ_Arg_Refp
                = కొత్త AstLambdaArgRef{argFl, name + "__DOT__index", నిజం};
            AstLambdaArgRef* స్థిరరాశి విలువ_Arg_Refp = కొత్త AstLambdaArgRef{argFl, name, అబద్ధం};
            AstWith* స్థిరరాశి newp
                = కొత్త AstWith{nodep->fileline(), సూచీ_Arg_Refp, విలువ_Arg_Refp, శూన్యసూచిక};
            // Harvest the identifier_list into AstWith; grammar guarantees AstParseRef.
            ఒకవేళ (restrictedRandomize) {
                newp->restricted(నిజం);
                అయితే (Ast_కణుపు* స్థిరరాశి itemp = nodep->exprsp()) {
                    newp->కూడు_Restricted_Name(VN_AS(itemp, ParseRef)->name());
                    itemp->unlink_Fr_వెనుక();
                    VL_DO_DANGLING(నెట్టు_Deletep(itemp), itemp);
                }
            }
            Ast_కణుపు* సమాసం_Or_Constraintsp = శూన్యసూచిక;
            ఒకవేళ (nodep->exprsp()) సమాసం_Or_Constraintsp = nodep->exprsp()->unlink_Fr_వెనుక_With_తదుపరి();
            ఒకవేళ (nodep->constraintsp())
                సమాసం_Or_Constraintsp = Ast_కణుపు::కూడు_తదుపరి(
                    సమాసం_Or_Constraintsp, nodep->constraintsp()->unlink_Fr_వెనుక_With_తదుపరి());
            newp->కూడు_Exprp(సమాసం_Or_Constraintsp);  // addExprp() tolerates nullptr
            funcrefp->withp(newp);
        }
        funcrefp->కూడు_Argsp(argsp);
        nodep->replaceWith(nodep->funcrefp()->unlink_Fr_వెనుక());
        VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
    }
    శూన్య సందర్శించు(AstWith* nodep) override {  // FindVisitor::
        // Symbol table needs nodep->name() as the index variable's name
        // Iteration will pickup the AstVar we made under AstWith
        VL_RESTORER(m_curSymp);
        ++m_mod_With_అంకె;
        m_curSymp = m_statep->చొప్పించు_ఖండం(m_curSymp, "__Vwith" + cvt_గమ్యం_Str(m_mod_With_అంకె), nodep,
                                          m_classOrPackagep);
        m_curSymp->fallbackp(VL_RESTORER_PREV(m_curSymp));
        UASSERT_OBJ(nodep->సూచీ_Arg_Refp(), nodep, "Missing lambda argref");
        UASSERT_OBJ(nodep->విలువ_Arg_Refp(), nodep, "Missing lambda argref");
        // Insert argref's name into symbol table
        m_statep->చొప్పించు_Sym(m_curSymp, nodep->విలువ_Arg_Refp()->name(), nodep->విలువ_Arg_Refp(),
                            శూన్యసూచిక);
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(AstCoverWith* nodep) override {  // FindVisitor::
        // The range list is outside the scope of the filter, which declares 'item'
        iterate_And_తదుపరి_Null(nodep->subp());
        VL_RESTORER(m_curSymp);
        ++m_mod_With_అంకె;
        m_curSymp = m_statep->చొప్పించు_ఖండం(m_curSymp, "__VcoverWith" + cvt_గమ్యం_Str(m_mod_With_అంకె),
                                          nodep, m_classOrPackagep);
        m_curSymp->fallbackp(VL_RESTORER_PREV(m_curSymp));
        iterate_And_తదుపరి_Null(nodep->itemp());
        iterate_And_తదుపరి_Null(nodep->filterp());
    }
    శూన్య సందర్శించు(AstCoverCrossWith* nodep) override {  // FindVisitor::
        // The selection, the 'matches' count, and the cross items' types, of their coverpoints'
        // expressions with any nested scopes, are outside the filter's scope, which declares the
        // items.  As visit(AstVar) would resolve a type in the scope declaring its variable, the
        // items, generated and unique, are inserted directly.
        iterate_And_తదుపరి_Null(nodep->selectp());
        iterate_And_తదుపరి_Null(nodep->countp());
        కొరకు (AstVar* varp = nodep->varsp(); varp; varp = VN_AS(varp->nextp(), Var)) {
            iterate_And_తదుపరి_Null(varp->బిడ్డ_DTypep());
        }
        VL_RESTORER(m_curSymp);
        ++m_mod_With_అంకె;
        m_curSymp = m_statep->చొప్పించు_ఖండం(m_curSymp, "__VcoverWith" + cvt_గమ్యం_Str(m_mod_With_అంకె),
                                          nodep, m_classOrPackagep);
        m_curSymp->fallbackp(VL_RESTORER_PREV(m_curSymp));
        కొరకు (AstVar* varp = nodep->varsp(); varp; varp = VN_AS(varp->nextp(), Var)) {
            m_statep->చొప్పించు_Sym(m_curSymp, varp->name(), varp, m_classOrPackagep);
        }
        iterate_And_తదుపరి_Null(nodep->filterp());
    }

    శూన్య సందర్శించు(Ast_కణుపు* nodep) override { iterateChildren(nodep); }  // FindVisitor::

    శూన్య handleUnvisitedVirtIfaces() {
        Ast_కణుపు_Module* స్థిరరాశి శిఖరం = v3Global.rootp()->modulesp();
        m_curSymp = m_statep->get_కణుపు_Sym(శిఖరం);
        కొరకు (AstIface* ifacep : m_virtIfaces) {
            ఒకవేళ (!ifacep->user4()) {  // virtual interface never assigned any actual interface
                // Create dummy cell to keep the virtual interface alive
                // (later stages assume that non-dead interface has associated cell)
                AstCell* స్థిరరాశి ifacecellp = కొత్త AstCell{ifacep->fileline(),
                                                        ifacep->fileline(),
                                                        "__VdummyInstOf" + ifacep->name(),
                                                        ifacep->name(),
                                                        శూన్యసూచిక,
                                                        శూన్యసూచిక,
                                                        శూన్యసూచిక};
                ifacecellp->modp(ifacep);
                శిఖరం->కూడు_Stmtsp(ifacecellp);

                iterate(ifacecellp);
            }
        }
    }

బహిరంగ:
    // CONSTRUCTORS
    Link_Dot_కనుగొను_Visitor(AstNetlist* rootp, LinkDotState* statep)
        : m_statep{statep} {
        UINFO(4, __FUNCTION__ << ": ");
        iterate(rootp);

        ఒకవేళ (!m_virtIfaces.ఖాళీలు()) handleUnvisitedVirtIfaces();
        కొరకు (AstScope* స్థిరరాశి scopep : m_scopes) scopep->modp()->కూడు_Stmtsp(scopep);
        ఒకవేళ (m_statep->forScopeCreation()) {
            V3Stats::కూడు_Stat("LinkDot, Scope visits", m_statScopeVisits);
        }
    }
    ~Link_Dot_కనుగొను_Visitor() override = అప్రమేయ;
};

//======================================================================

తరగతి Link_Dot_కనుగొను_Iface_Visitor final : బహిరంగ VNVisitor {
    // NODE STATE
    //  *::user1p()             -> See LinkDotState

    // STATE - for current visit position (use VL_RESTORER)
    LinkDotState* స్థిరరాశి m_statep;  // State to pass between visitors, including symbol table
    Ast_కణుపు* m_declp = శూన్యసూచిక;  // Current declaring object that may soon contain IfaceRefDType

    // METHODS
    స్థిరరాశి VSymEnt* కనుగొను_Non_Decl_Sym(VSymEnt* symp, స్థిరరాశి string& name) {
        // Find if there is a symbol of given name, ignoring the node that is declaring
        // it (m_declp) itself.  Thus if searching for "ifc" (an interface):
        //
        // module x;  // Finishes here and finds the typedef
        //   typedef foo ifc;
        //   ifc ifc;   // symp starts pointing here, but matches m_declp
        అయితే (symp) {
            స్థిరరాశి VSymEnt* స్థిరరాశి foundp = symp->కనుగొను_Id_Flat(name);
            ఒకవేళ (foundp && foundp->nodep() != m_declp) తిరిగి foundp;
            symp = symp->fallbackp();
        }
        తిరిగి శూన్యసూచిక;
    }

    // VISITORS
    శూన్య సందర్శించు(AstRefDType* nodep) override {  // FindIfaceVisitor::
        ఒకవేళ (m_statep->forPrimary() && !nodep->classOrPackagep()) {
            UINFO(9, "  FindIfc: " << nodep);
            // If under a var, ignore the var itself as might be e.g. "intf intf;"
            // Critical tests:
            //   t_interface_param_genblk.v  // Checks this does make interface
            //   t_interface_hidden.v  // Checks this doesn't making interface when hidden
            ఒకవేళ (m_statep->exists_కణుపు_Sym(nodep)) {
                VSymEnt* symp = m_statep->get_కణుపు_Sym(nodep);
                స్థిరరాశి VSymEnt* foundp = కనుగొను_Non_Decl_Sym(symp, nodep->name());
                Ast_కణుపు* దొరికింది_Nodep = శూన్యసూచిక;
                // This: v4make test_regress/t/t_interface_param_genblk.py --debug
                // --debugi-V3LinkDot 9 Passes with this commented out:
                ఒకవేళ (foundp) దొరికింది_Nodep = foundp->nodep();
                ఒకవేళ (!దొరికింది_Nodep) దొరికింది_Nodep = m_statep->కనుగొను_Module_Sym(nodep->name());
                ఒకవేళ (దొరికింది_Nodep) UINFO(9, "    Ifc foundNodep " << దొరికింది_Nodep);
                ఒకవేళ (AstIface* స్థిరరాశి defp = VN_CAST(దొరికింది_Nodep, Iface)) {
                    // Must be found as module name, and not hidden/ by normal symbol (foundp)
                    AstIfaceRefDType* స్థిరరాశి newp
                        = కొత్త AstIfaceRefDType{nodep->fileline(), "", nodep->name()};
                    ఒకవేళ (nodep->paramsp())
                        newp->కూడు_Paramsp(nodep->paramsp()->unlink_Fr_వెనుక_With_తదుపరి());
                    newp->ifacep(defp);
                    newp->user1u(nodep->user1u());
                    UINFO(9, "    Resolved interface " << nodep << "  =>  " << defp);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
                    తిరిగి;
                }
            }
        }
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(AstVar* nodep) override {  // FindIfaceVisitor::
        VL_RESTORER(m_declp);
        m_declp = nodep;
        iterateChildren(nodep);
        AstIfaceRefDType* స్థిరరాశి ifacerefp = LinkDotState::ifaceRefFromArray(nodep->subDTypep());
        ఒకవేళ (ifacerefp && m_statep->exists_కణుపు_Sym(nodep)) {
            // Can't resolve until interfaces and modport names are
            // known; see notes at top
            UINFO(9, "  FindIfc Var IfaceRef " << ifacerefp);
            ఒకవేళ (!ifacerefp->isVirtual()) nodep->సమితి_Iface_Ref();
            VSymEnt* స్థిరరాశి symp = m_statep->get_కణుపు_Sym(nodep);
            m_statep->చొప్పించు_Iface_Var_Sym(symp);
        }
    }
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override { iterateChildren(nodep); }  // FindIfaceVisitor::

బహిరంగ:
    // CONSTRUCTORS
    Link_Dot_కనుగొను_Iface_Visitor(AstNetlist* rootp, LinkDotState* statep)
        : m_statep{statep} {
        UINFO(4, __FUNCTION__ << ": ");
        iterate(rootp);
    }
    ~Link_Dot_కనుగొను_Iface_Visitor() override = అప్రమేయ;
};

//======================================================================

తరగతి LinkDotParamVisitor final : బహిరంగ VNVisitor {
    // NODE STATE
    //  *::user1p()             -> See LinkDotState
    //  *::user2p()             -> See LinkDotState
    //  *::user4()              -> See LinkDotState

    // STATE
    LinkDotState* స్థిరరాశి m_statep;  // State to pass between visitors, including symbol table
    Ast_కణుపు_Module* m_modp = శూన్యసూచిక;  // Current module

    శూన్య pin_Implicit_సమాసం_Recurse(Ast_కణుపు* nodep) {
        // Under a pin, Check interconnect expression for a pin reference or a concat.
        // Create implicit variable as needed
        ఒకవేళ (VN_IS(nodep, Dot)) {  // Not creating a simple implied type,
            // and implying something else would just confuse later errors
        } లేకపోతే ఒకవేళ (VN_IS(nodep, VarRef) || VN_IS(nodep, ParseRef)) {
            // To prevent user errors, we should only do single bit
            // implicit vars, however some netlists (MIPS) expect single
            // bit implicit wires to get created with range 0:0 etc.
            m_statep->implicit_Ok_కూడు(m_modp, nodep->name());
        }
        // These are perhaps a little too generous, as a SELect of siga[sigb]
        // perhaps shouldn't create an implicit variable.  But, we've warned...
        లేకపోతే {
            ఒకవేళ (Ast_కణుపు* స్థిరరాశి refp = nodep->op1p()) pin_Implicit_సమాసం_Recurse(refp);
            ఒకవేళ (Ast_కణుపు* స్థిరరాశి refp = nodep->op2p()) pin_Implicit_సమాసం_Recurse(refp);
            ఒకవేళ (Ast_కణుపు* స్థిరరాశి refp = nodep->op3p()) pin_Implicit_సమాసం_Recurse(refp);
            ఒకవేళ (Ast_కణుపు* స్థిరరాశి refp = nodep->op4p()) pin_Implicit_సమాసం_Recurse(refp);
        }
        // Continue along a list (e.g. the terminals under AstImplicit), also after a reference
        ఒకవేళ (Ast_కణుపు* స్థిరరాశి refp = nodep->nextp()) pin_Implicit_సమాసం_Recurse(refp);
    }

    // VISITORS
    శూన్య సందర్శించు(Ast_Type_పట్టిక*) override {}  // ParamVisitor::
    శూన్య సందర్శించు(Ast_కణుపు_Module* nodep) override {  // ParamVisitor::
        UINFO(5, "   " << nodep);
        ఒకవేళ ((nodep->dead() || !nodep->user4()) && !nodep->hierParams()) {
            UINFO(4, "Mark dead module " << nodep);
            UASSERT_OBJ(!m_statep->forScopeCreation(), nodep,
                        "Dead module persisted past where should have removed");
            // Don't remove now, because we may have a tree of
            // parameterized modules with VARXREFs into the deleted module
            // region.  V3Dead should cleanup.
            // Downstream visitors up until V3Dead need to check for nodep->dead.
            nodep->dead(నిజం);
            తిరిగి;
        }
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(AstPin* nodep) override {  // ParamVisitor::
        // Pin: Link to submodule's port
        // Deal with implicit definitions - do before Resolve visitor as may
        // be referenced above declaration
        ఒకవేళ (nodep->exprp()  // Else specifically unconnected pin
            && !nodep->svImplicit()) {  // SV 19.11.3: .name pins don't allow implicit decls
            pin_Implicit_సమాసం_Recurse(nodep->exprp());
        }
    }
    శూన్య సందర్శించు(AstDefParam* nodep) override {  // ParamVisitor::
        iterateChildren(nodep);
        nodep->v3warn(DEFPARAM, "defparam is deprecated (IEEE 1800-2023 C.4.1)\n"
                                    << nodep->warn_మరిన్ని()
                                    << "... Suggest use instantiation with #(."
                                    << nodep->prettyName() << "(...etc...))");
        తార్కిక hasPartSelect = అబద్ధం;
        స్థిరరాశి string మార్గం = వెలికితీయి_Dotted_మార్గం(nodep->pathp(), hasPartSelect);
        UASSERT_OBJ(!hasPartSelect && !మార్గం.ఖాళీలు(), nodep, "Unexpected defparam path shape");
        string baddot;
        VSymEnt* okSymp = శూన్యసూచిక;
        VSymEnt* స్థిరరాశి foundp = m_statep->కనుగొను_Dotted(
            nodep->fileline(), m_statep->get_కణుపు_Sym(nodep), మార్గం, baddot, okSymp, నిజం);
        AstCell* స్థిరరాశి cellp = foundp ? VN_AS(foundp->nodep(), Cell) : శూన్యసూచిక;
        ఒకవేళ (!cellp) {
            nodep->v3error("In defparam, instance " << మార్గం << " never declared");
        } లేకపోతే {
            Ast_కణుపు_సమాసం* స్థిరరాశి exprp = nodep->rhsp()->unlink_Fr_వెనుక();
            UINFO(9, "Defparam cell " << మార్గం << "." << nodep->name() << " attach-to " << cellp
                                      << "  <= " << exprp);
            // Don't need to check the name of the defparam exists.  V3Param does.
            AstPin* స్థిరరాశి pinp = కొత్త AstPin{nodep->fileline(),
                                            -1,  // Pin# not relevant
                                            nodep->name(), exprp};
            pinp->param(నిజం);
            pinp->param_మార్గం(మార్గం);
            cellp->కూడు_Paramsp(pinp);
        }
        VL_DO_DANGLING(nodep->unlink_Fr_వెనుక()->delete_వృక్షం(), nodep);
    }
    శూన్య సందర్శించు(Ast_పోర్టు* nodep) override {  // ParamVisitor::
        // Port: Stash the pin number
        // Need to set pin numbers after varnames are created
        // But before we do the final resolution based on names
        VSymEnt* స్థిరరాశి foundp = m_statep->get_కణుపు_Sym(m_modp)->కనుగొను_Id_Flat(nodep->name());
        AstVar* స్థిరరాశి refp = foundp ? VN_CAST(foundp->nodep(), Var) : శూన్యసూచిక;
        ఒకవేళ (!foundp) {
            nodep->v3error(
                "Input/output/inout declaration not found for port: " << nodep->prettyNameQ());
        } లేకపోతే ఒకవేళ (!refp || (!refp->isIO() && !refp->isIfaceRef())) {
            nodep->v3error("Pin is not an in/out/inout/interface: " << nodep->prettyNameQ());
        } లేకపోతే {
            ఒకవేళ (refp->user4()) {
                nodep->v3error("Duplicate declaration of port: "
                               << nodep->prettyNameQ() << '\n'
                               << nodep->warnContextPrimary() << '\n'
                               << refp->warnOther() << "... Location of original declaration\n"
                               << refp->warnContextSecondary());
            }
            refp->user4(నిజం);
            VSymEnt* స్థిరరాశి symp = m_statep->చొప్పించు_Sym(m_statep->get_కణుపు_Sym(m_modp),
                                                      "__pinNumber" + cvt_గమ్యం_Str(nodep->pin_అంకె()),
                                                      refp, శూన్యసూచిక /*classOrPackagep*/);
            symp->exported(అబద్ధం);
            refp->pin_అంకె(nodep->pin_అంకె());
            // Put the variable where the port is, so that variables stay
            // in pin number sorted order. Otherwise hierarchical or JSON
            // may botch by-position instances.
            nodep->కూడు_ఇక్కడ_This_As_తదుపరి(refp->unlink_Fr_వెనుక());
        }
        // Ports not needed any more
        VL_DO_DANGLING(nodep->unlink_Fr_వెనుక()->delete_వృక్షం(), nodep);
    }
    శూన్య సందర్శించు(Ast_కేటాయించు_W* nodep) override {  // ParamVisitor::
        // Deal with implicit definitions
        // We used to nodep->allowImplicit() here, but it turns out
        // normal "assigns" can also make implicit wires.  Yuk.
        pin_Implicit_సమాసం_Recurse(nodep->lhsp());
        iterateChildren(nodep);
    }

    శూన్య సందర్శించు(AstImplicit* nodep) override {  // ParamVisitor::
        // Unsupported gates need implicit creation
        pin_Implicit_సమాసం_Recurse(nodep->exprsp());
        // We're done with implicit gates
        VL_DO_DANGLING(nodep->unlink_Fr_వెనుక()->delete_వృక్షం(), nodep);
    }
    శూన్య సందర్శించు(AstClassOrPackageRef* nodep) override {  // ParamVisitor::
        ఒకవేళ (స్వయంచాలక* స్థిరరాశి fwdp = VN_CAST(nodep->classOrPackageNodep(), TypedefFwd)) {
            // Relink forward definitions to the "real" definition
            VSymEnt* స్థిరరాశి foundp = m_statep->get_కణుపు_Sym(fwdp)->కనుగొను_Id_Fallback(fwdp->name());
            ఒకవేళ (foundp && (VN_IS(foundp->nodep(), Class) || VN_IS(foundp->nodep(), Package))) {
                nodep->classOrPackagep(VN_AS(foundp->nodep(), కణుపు_Module));
            } లేకపోతే ఒకవేళ (foundp && VN_IS(foundp->nodep(), ParamTypeDType)) {
                UASSERT(m_statep->forPrimary(), "Param types should have been resolved");
                nodep->classOrPackageNodep(VN_AS(foundp->nodep(), ParamTypeDType));
            } లేకపోతే {
                ఒకవేళ (foundp) UINFO(1, "found nodep = " << foundp->nodep());
                nodep->v3error(
                    "Forward typedef used as class/package does not resolve to class/package: "
                    << nodep->prettyNameQ() << '\n'
                    << nodep->warnContextPrimary() << '\n'
                    << (foundp ? nodep->warn_మరిన్ని() + "... Object with matching name\n"
                                     + foundp->nodep()->warnContextSecondary()
                               : ""));
            }
        }
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(AstPull* nodep) override {  // ParamVisitor::
        // Deal with implicit definitions
        // We used to nodep->allowImplicit() here, but it turns out
        // normal "assigns" can also make implicit wires.  Yuk.
        pin_Implicit_సమాసం_Recurse(nodep->lhsp());
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(AstTypedefFwd* nodep) override {  // ParamVisitor::
        VSymEnt* స్థిరరాశి foundp = m_statep->get_కణుపు_Sym(nodep)->కనుగొను_Id_Fallback(nodep->name());
        ఒకవేళ (foundp) {
            // If the typedef was earlier in source order (tokenNum), then remember that earlier
            // point to avoid error something wasn't declared
            // Might be forward declaring something odd (with declTokenNumSetMin not implemented,
            // but should be harmless to ignore as this is just for error detection
            foundp->nodep()->decl_Token_అంకె_సమితి_Min(nodep->fileline()->token_అంకె());
        } లేకపోతే ఒకవేళ (v3Global.opt.pedantic()) {
            // We only check it was ever really defined in pedantic mode, as it
            // might have been in a header file referring to a module we never
            // needed so there are false positives
            nodep->v3error(
                "Forward typedef unused or does not resolve to a data type (IEEE 1800-2023 6.18): "
                << nodep->prettyNameQ());
        }
        // We only needed the forward declaration in order to parse correctly.
        // Delete later as may be ClassOrPackageRef's still pointing to it
        VL_DO_DANGLING(నెట్టు_Deletep(nodep->unlink_Fr_వెనుక()), nodep);
    }

    శూన్య సందర్శించు(Ast_కణుపు* nodep) override { iterateChildren(nodep); }  // ParamVisitor::

బహిరంగ:
    // CONSTRUCTORS
    LinkDotParamVisitor(AstNetlist* rootp, LinkDotState* statep)
        : m_statep{statep} {
        UINFO(4, __FUNCTION__ << ": ");
        iterate(rootp);
    }
    ~LinkDotParamVisitor() override = అప్రమేయ;
};

//======================================================================

తరగతి LinkDotScopeVisitor final : బహిరంగ VNVisitor {

    // STATE
    LinkDotState* స్థిరరాశి m_statep;  // State to pass between visitors, including symbol table
    స్థిరరాశి AstScope* m_scopep = శూన్యసూచిక;  // The current scope
    VSymEnt* m_modSymp = శూన్యసూచిక;  // Symbol entry for current module
    // Deferred AliasScope processing - must be done outer-to-inner for correct alias resolution
    std::సదిశ<std::pair<AstAliasScope*, VSymEnt*>> m_deferredAliasScopes;

    // METHODS
బహిరంగ:
    // getAlias with setVarAlias and setIfaceAlias implement disjoint-set data structure.
    // This algorithm is needed for the case when multiple alias statements
    // reference partially the same variables.
    స్థిర AstVarScope* getAlias(AstVarScope* స్థిరరాశి vscp) {
        ఒకవేళ (vscp->user2p() && vscp != vscp->user2p()) {
            AstVarScope* స్థిరరాశి aliasp = getAlias(VN_AS(vscp->user2p(), VarScope));
            vscp->user2p(aliasp);
            తిరిగి aliasp;
        } లేకపోతే {
            తిరిగి vscp;
        }
    }

అంతరంగ:
    // Alias an interface reference to the interface it is connected to
    శూన్య సమితి_Iface_Alias(AstVarScope* స్థిరరాశి vscp, AstVarScope* స్థిరరాశి aliasp) {
        UASSERT_OBJ(vscp->varp()->isIfaceRef(), vscp, "Interface alias of non-interface");
        UASSERT_OBJ(aliasp->varp()->isIfaceRef(), aliasp, "Interface alias of non-interface");
        getAlias(vscp)->user2p(getAlias(aliasp));
    }

    // Alias two variables. An output port in the lower scope survives, unless the other
    // must keep its own storage.
    శూన్య సమితి_Var_Alias(AstVarScope* ap, AstVarScope* bp) {
        UASSERT_OBJ(!ap->varp()->isIfaceRef(), ap, "Variable alias of interface");
        UASSERT_OBJ(!bp->varp()->isIfaceRef(), bp, "Variable alias of interface");
        ap = getAlias(ap);
        bp = getAlias(bp);
        ఒకవేళ (ap == bp) తిరిగి;
        స్థిరరాశి తార్కిక aWins = [&]() {
            స్థిరరాశి AstVar* స్థిరరాశి aVarp = ap->varp();
            స్థిరరాశి AstVar* స్థిరరాశి bVarp = bp->varp();
            // A primary IO is accessed directly by the user of the model, so must survive
            UASSERT_OBJ(!aVarp->isPrimaryIO() || !bVarp->isPrimaryIO(), ap,
                        "Alias of two primary IOs");
            ఒకవేళ (bVarp->isPrimaryIO()) తిరిగి అబద్ధం;
            ఒకవేళ (aVarp->isPrimaryIO()) తిరిగి నిజం;
            // A forced or public variable must keep its own storage. If both are, either works.
            ఒకవేళ (aVarp->isForced() != bVarp->isForced()) తిరిగి aVarp->isForced();
            ఒకవేళ (aVarp->is_Sig_ప్రజా() != bVarp->is_Sig_ప్రజా()) తిరిగి aVarp->is_Sig_ప్రజా();
            // Otherwise prefer an output port in the lower scope. This enables better V3Combine
            // due to having fewer upward hierarchical references (which prevent combining).
            ఒకవేళ (aVarp->direction() != VDirection::OUTPUT) తిరిగి అబద్ధం;
            // Is 'ap' strictly below 'bp' in the hierarchy
            కొరకు (స్థిరరాశి AstScope* scopep = ap->scopep()->aboveScopep(); scopep;
                 scopep = scopep->aboveScopep()) {
                ఒకవేళ (scopep == bp->scopep()) తిరిగి నిజం;
            }
            తిరిగి అబద్ధం;
        }();
        ఒకవేళ (aWins) {
            bp->user2p(ap);
        } లేకపోతే {
            ap->user2p(bp);
        }
    }

    // VISITORS
    శూన్య సందర్శించు(AstNetlist* nodep) override {  // ScopeVisitor::
        // Recurse..., backward as must do packages before using packages
        iterateChildrenBackwardsConst(nodep);
    }
    శూన్య సందర్శించు(AstScope* nodep) override {  // ScopeVisitor::
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
        ఒకవేళ (AstIface* స్థిరరాశి ifacep = VN_CAST(nodep->modp(), Iface)) {
            m_statep->చొప్పించు_Iface_Mod_Sym(ifacep, m_modSymp);
        }
        m_scopep = nodep;
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(AstVarScope* nodep) override {  // ScopeVisitor::
        ఒకవేళ (!nodep->varp()->isFuncLocal() && !nodep->varp()->isClassMember()) {
            VSymEnt* స్థిరరాశి varSymp
                = m_statep->చొప్పించు_Sym(m_modSymp, nodep->varp()->name(), nodep, శూన్యసూచిక);
            ఒకవేళ (nodep->varp()->isIfaceRef()) {
                AstIfaceRefDType* స్థిరరాశి dtypep
                    = LinkDotState::ifaceRefFromArray(nodep->varp()->dtypep());
                UASSERT_OBJ(dtypep, nodep, "Non AstIfaceRefDType on isIfaceRef() var");
                ఒకవేళ (nodep->varp()->is_Iface_మాతృ()) {
                    UINFO(9, "Iface parent ref var " << nodep->varp()->name() << " " << nodep);
                    // Find the interface cell the var references
                    UINFO(9, "Iface parent dtype " << dtypep);
                    స్థిరరాశి string ifcellname = dtypep->cellName();
                    string baddot;
                    VSymEnt* okSymp;
                    VSymEnt* cellSymp = m_statep->కనుగొను_Dotted(nodep->fileline(), m_modSymp,
                                                             ifcellname, baddot, okSymp, అబద్ధం);
                    UASSERT_OBJ(
                        cellSymp, nodep,
                        "No symbol for interface instance: " << nodep->prettyNameQ(ifcellname));
                    UINFO(5, "       Found interface instance: se" << cvt_గమ్యం_Hex(cellSymp) << " "
                                                                   << cellSymp->nodep());
                    ఒకవేళ (dtypep->modportName() != "") {
                        // Look up the modport within the interface cell's symbol table
                        VSymEnt* స్థిరరాశి mpSymp = cellSymp->కనుగొను_Id_Fallback(dtypep->modportName());
                        UASSERT_OBJ(mpSymp, nodep,
                                    "No symbol for interface modport: "
                                        << nodep->prettyNameQ(dtypep->modportName()));
                        cellSymp = mpSymp;
                        UINFO(5, "       Found modport cell: se" << cvt_గమ్యం_Hex(cellSymp) << " "
                                                                 << mpSymp->nodep());
                    }
                    // Interface reference; need to put whole thing into
                    // symtable, but can't clone it now as we may have a later
                    // alias for it.
                    m_statep->చొప్పించు_Scope_Alias(LinkDotState::SAMN_IFTOP, varSymp, cellSymp);
                }
            }
        }
    }
    శూన్య సందర్శించు(Ast_కణుపు_FTask* nodep) override {  // ScopeVisitor::
        VSymEnt* స్థిరరాశి symp = m_statep->చొప్పించు_ఖండం(m_modSymp, nodep->name(), nodep, శూన్యసూచిక);
        symp->fallbackp(m_modSymp);
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(Ast_కణుపు_Foreach* nodep) override {  // ScopeVisitor::
        VSymEnt* స్థిరరాశి symp = m_statep->చొప్పించు_ఖండం(m_modSymp, nodep->name(), nodep, శూన్యసూచిక);
        symp->fallbackp(m_modSymp);
        // No recursion, we don't want to pick up variables
    }
    శూన్య సందర్శించు(AstConstraintForeach* nodep) override {  // ScopeVisitor::
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(AstWith* nodep) override {  // ScopeVisitor::
        VSymEnt* స్థిరరాశి symp = m_statep->చొప్పించు_ఖండం(m_modSymp, nodep->name(), nodep, శూన్యసూచిక);
        symp->fallbackp(m_modSymp);
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(AstAlias* nodep) override {  // ScopeVisitor::
        // Track aliases
        UINFOTREE(9, nodep, "", "alias");
        AstVarScope* aliasVscp = శూన్యసూచిక;
        కొరకు (Ast_కణుపు* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
            AstVarScope* vscp = శూన్యసూచిక;
            ఒకవేళ (స్థిరరాశి AstVarRef* స్థిరరాశి refp = VN_CAST(itemp, VarRef)) {
                vscp = refp->varScopep();
            } లేకపోతే {
                // Reaches into the scope of an instance, look it up by name
                స్థిరరాశి AstVarXRef* స్థిరరాశి xrefp = VN_AS(itemp, VarXRef);
                స్థిరరాశి string scopename = xrefp->dotted() + "." + xrefp->name();
                string baddot;
                VSymEnt* okSymp;
                VSymEnt* స్థిరరాశి symp = m_statep->కనుగొను_Dotted(xrefp->fileline(), m_modSymp, scopename,
                                                           baddot, okSymp, అబద్ధం);
                UASSERT_OBJ(symp, nodep, "No symbol for alias item: " << scopename);
                vscp = VN_CAST(symp->nodep(), VarScope);
            }
            UASSERT_OBJ(vscp, nodep, "VarScope unset");
            ఒకవేళ (aliasVscp) {
                సమితి_Var_Alias(aliasVscp, vscp);
            } లేకపోతే {
                aliasVscp = vscp;
            }
        }
        iterateChildren(nodep);
        VL_DO_DANGLING(నెట్టు_Deletep(nodep->unlink_Fr_వెనుక()), nodep);
    }
    శూన్య సందర్శించు(AstAliasScope* nodep) override {  // ScopeVisitor::
        // Defer AliasScope processing - must process outer scopes before inner ones
        // so that nested interface port alias resolution works correctly
        UINFO(5, "ALIASSCOPE (deferred) " << nodep);
        m_deferredAliasScopes.emplace_వెనుక(nodep, m_modSymp);
    }
    శూన్య processAliasScope(AstAliasScope* nodep, VSymEnt* modSymp) {
        UINFO(5, "ALIASSCOPE  " << nodep);
        UINFOTREE(9, nodep, "", "avs");
        VSymEnt* rhsSymp;
        {
            AstVarRef* స్థిరరాశి refp = VN_CAST(nodep->rhsp(), VarRef);
            AstVarXRef* స్థిరరాశి xrefp = VN_CAST(nodep->rhsp(), VarXRef);
            UASSERT_OBJ(refp || xrefp, nodep,
                        "Unsupported: Non Var(X)Ref attached to interface pin");
            string inl
                = ((xrefp && xrefp->inlinedDots().కొలత()) ? (xrefp->inlinedDots() + "__DOT__")
                                                          : "");
            స్థిరరాశి string dotted_మార్గం
                = (xrefp && !xrefp->dotted().ఖాళీలు()) ? (xrefp->dotted() + ".") : "";
            VSymEnt* symp = శూన్యసూచిక;
            string scopename;
            అయితే (!symp) {
                scopename = refp ? refp->name()
                                 : (inl.కొలత() ? (inl + dotted_మార్గం + xrefp->name())
                                               : (dotted_మార్గం + xrefp->name()));
                string baddot;
                VSymEnt* okSymp;
                symp = m_statep->కనుగొను_Dotted(nodep->rhsp()->fileline(), modSymp, scopename, baddot,
                                            okSymp, నిజం);
                ఒకవేళ (inl == "") ఆపు;
                inl = LinkDotState::remove_చివరి_Inline_Scope(inl);
            }
            UASSERT_OBJ(symp, nodep, "No symbol for interface alias rhs");
            UINFO(5, "       Found a linked scope RHS: " << scopename << "  se" << cvt_గమ్యం_Hex(symp)
                                                         << " " << symp->nodep());
            rhsSymp = symp;
        }
        VSymEnt* lhsSymp;
        {
            స్థిరరాశి AstVarXRef* స్థిరరాశి xrefp = VN_CAST(nodep->lhsp(), VarXRef);
            స్థిరరాశి AstVarRef* స్థిరరాశి refp = VN_CAST(nodep->lhsp(), VarRef);

            UASSERT_OBJ(refp || xrefp, nodep,
                        "Unsupported: Non Var(X)Ref attached to interface pin");
            స్థిరరాశి string scopename
                = refp ? refp->varp()->name() : xrefp->dotted() + "." + xrefp->name();
            string baddot;
            VSymEnt* okSymp;
            VSymEnt* స్థిరరాశి symp = m_statep->కనుగొను_Dotted(nodep->lhsp()->fileline(), modSymp,
                                                       scopename, baddot, okSymp, అబద్ధం);
            UASSERT_OBJ(symp, nodep, "No symbol for interface alias lhs");
            UINFO(5, "       Found a linked scope LHS: " << scopename << "  se" << cvt_గమ్యం_Hex(symp)
                                                         << " " << symp->nodep());
            lhsSymp = symp;
        }
        // Remember the alias - can't do it yet because we may have additional symbols to be added,
        // or maybe an alias of an alias
        m_statep->చొప్పించు_Scope_Alias(LinkDotState::SAMN_IFTOP, lhsSymp, rhsSymp);
        AstVarScope* స్థిరరాశి lhsVscp = VN_CAST(lhsSymp->nodep(), VarScope);
        AstVarScope* స్థిరరాశి rhsVscp = VN_CAST(rhsSymp->nodep(), VarScope);
        UASSERT_OBJ(lhsVscp && rhsVscp, nodep, "Interface alias missing variable scope");
        సమితి_Iface_Alias(lhsVscp, rhsVscp);
        // We have stored the link, we don't need these any more
        VL_DO_DANGLING(nodep->unlink_Fr_వెనుక()->delete_వృక్షం(), nodep);
    }
    శూన్య processDeferredAliasScopes() {
        // Sort by hierarchy depth (shallower first) so outer aliases are resolved before inner
        // Pre-compute depth map to avoid O(N log N * D) complexity in sort comparisons
        std::unordered_map<VSymEnt*, పూర్ణాంకం> లోతు_Map;
        కొరకు (స్థిరరాశి స్వయంచాలక& pair : m_deferredAliasScopes) {
            VSymEnt* స్థిరరాశి symp = pair.రెండవ;
            ఒకవేళ (లోతు_Map.కనుగొను(symp) == లోతు_Map.చివర()) {
                పూర్ణాంకం లోతు = 0;
                కొరకు (VSymEnt* p = symp; p; p = p->parentp()) ++లోతు;
                లోతు_Map[symp] = లోతు;
            }
        }
        std::stable_క్రమీకరించు(m_deferredAliasScopes.begin(), m_deferredAliasScopes.చివర(),
                         [&లోతు_Map](స్థిరరాశి std::pair<AstAliasScope*, VSymEnt*>& a,
                                     స్థిరరాశి std::pair<AstAliasScope*, VSymEnt*>& b) {
                             తిరిగి లోతు_Map.at(a.రెండవ) < లోతు_Map.at(b.రెండవ);
                         });
        // Process in sorted order
        కొరకు (స్వయంచాలక& pair : m_deferredAliasScopes) { processAliasScope(pair.మొదటి, pair.రెండవ); }
        m_deferredAliasScopes.clear();
    }
    శూన్య సందర్శించు(Ast_కణుపు_Gen* nodep) override {  // ScopeVisitor::  // LCOV_EXCL_LINE
        nodep->v3fatalSrc("Generate constructs should have been reduced out");
    }
    // For speed, don't recurse things that can't have scope
    // Note we allow AstNodeStmt's as generates may be under them
    శూన్య సందర్శించు(AstCell*) override {}  // ScopeVisitor::
    శూన్య సందర్శించు(AstVar*) override {}  // ScopeVisitor::
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override { iterateChildren(nodep); }  // ScopeVisitor::

బహిరంగ:
    // CONSTRUCTORS
    LinkDotScopeVisitor(AstNetlist* rootp, LinkDotState* statep)
        : m_statep{statep} {
        UINFO(4, __FUNCTION__ << ": ");
        iterate(rootp);
        // Process deferred AliasScopes in outer-to-inner order
        processDeferredAliasScopes();
    }
    ~LinkDotScopeVisitor() override = అప్రమేయ;
};

//======================================================================

// Iterate an interface to resolve modports
తరగతి LinkDotIfaceVisitor final : బహిరంగ VNVisitor {
    // STATE
    LinkDotState* స్థిరరాశి m_statep;  // State to pass between visitors, including symbol table
    VSymEnt* m_curSymp;  // Symbol Entry for current table, where to lookup/insert

    // VISITORS
    శూన్య సందర్శించు(AstModport* nodep) override {  // IfaceVisitor::
        // Modport: Remember its name for later resolution
        UINFO(5, "   fiv: " << nodep);
        VL_RESTORER(m_curSymp);
        {
            // Create symbol table for the vars
            m_curSymp = m_statep->చొప్పించు_ఖండం(m_curSymp, nodep->name(), nodep, శూన్యసూచిక);
            m_curSymp->fallbackp(VL_RESTORER_PREV(m_curSymp));
            iterateChildren(nodep);
        }
    }
    శూన్య సందర్శించు(AstModportFTaskRef* nodep) override {  // IfaceVisitor::
        UINFO(5, "   fif: " << nodep);
        iterateChildren(nodep);
        VSymEnt* స్థిరరాశి symp = m_curSymp->కనుగొను_Id_Fallback(nodep->name());
        ఒకవేళ (!symp) {
            nodep->v3error("Modport item not found: " << nodep->prettyNameQ());
        } లేకపోతే ఒకవేళ (Ast_కణుపు_FTask* స్థిరరాశి ftaskp = VN_CAST(symp->nodep(), కణుపు_FTask)) {
            // Make symbol under modport that points at the _interface_'s var, not the modport.
            nodep->ftaskp(ftaskp);
            VSymEnt* స్థిరరాశి subSymp
                = m_statep->చొప్పించు_Sym(m_curSymp, nodep->name(), ftaskp, శూన్యసూచిక /*package*/);
            m_statep->చొప్పించు_Scope_Alias(LinkDotState::SAMN_MODPORT, subSymp, symp);
        } లేకపోతే {
            nodep->v3error("Modport item is not a function/task: " << nodep->prettyNameQ());
        }
        ఒకవేళ (m_statep->forScopeCreation()) {
            // Done with AstModportFTaskRef.
            // Delete to prevent problems if we dead-delete pointed to ftask
            nodep->unlink_Fr_వెనుక();
            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
        }
    }

    // Helper to resolve remaining path through a nested interface
    // When findDotted() partially matches (okSymp set, baddot non-empty),
    // this follows the interface type to resolve the remaining path.
    // Returns the resolved symbol, or nullptr if not found.
    // On success, clears baddot; on partial match of multi-level path, updates baddot.
    VSymEnt* resolve_Nested_Interface_మార్గం(File_పంక్తి* fl, VSymEnt* okSymp, string& baddot) {
        ఒకవేళ (!okSymp || baddot.ఖాళీలు()) తిరిగి శూన్యసూచిక;

        స్థిర constexpr పూర్ణాంకం MAX_NESTING_లోతు = 64;
        VSymEnt* curOkSymp = okSymp;

        కొరకు (పూర్ణాంకం లోతు = 0; లోతు < MAX_NESTING_లోతు; ++లోతు) {
            // Try to get interface from the partially-matched symbol
            AstIface* ifacep = శూన్యసూచిక;
            ఒకవేళ (స్థిరరాశి AstCell* స్థిరరాశి cellp = VN_CAST(curOkSymp->nodep(), Cell)) {
                ifacep = VN_CAST(cellp->modp(), Iface);
            } లేకపోతే ఒకవేళ (స్థిరరాశి AstVar* స్థిరరాశి varp = VN_CAST(curOkSymp->nodep(), Var)) {
                ఒకవేళ (varp->isIfaceRef()) {
                    ఒకవేళ (స్థిరరాశి AstIfaceRefDType* స్థిరరాశి ifaceRefp
                        = LinkDotState::ifaceRefFromArray(varp->dtypep())) {
                        ifacep = ifaceRefp->iface_ద్వారా_Cellp();
                    }
                }
            }

            ఒకవేళ (!ifacep || !m_statep->exists_కణుపు_Sym(ifacep)) తిరిగి శూన్యసూచిక;

            VSymEnt* స్థిరరాశి ifaceSymp = m_statep->get_కణుపు_Sym(ifacep);

            ఒకవేళ (baddot.కనుగొను('.') == string::npos) {
                // Simple identifier - direct lookup
                VSymEnt* స్థిరరాశి symp = ifaceSymp->కనుగొను_Id_Fallback(baddot);
                ఒకవేళ (symp) baddot.clear();
                తిరిగి symp;
            }

            // Multi-level path - use findDotted for partial resolution
            string మిగిలిన_Baddot;
            VSymEnt* మిగిలిన_Ok_Symp = శూన్యసూచిక;
            VSymEnt* స్థిరరాశి symp = m_statep->కనుగొను_Dotted(fl, ifaceSymp, baddot, మిగిలిన_Baddot,
                                                       మిగిలిన_Ok_Symp, నిజం);
            ఒకవేళ (symp) {
                baddot = మిగిలిన_Baddot;
                తిరిగి symp;
            }

            // findDotted partially matched - check progress
            ఒకవేళ (మిగిలిన_Baddot == baddot || !మిగిలిన_Ok_Symp) తిరిగి శూన్యసూచిక;

            // Continue resolving with updated state
            baddot = మిగిలిన_Baddot;
            curOkSymp = మిగిలిన_Ok_Symp;
        }

        UINFO(1, "Nested interface resolution depth limit exceeded at " << fl);
        తిరిగి శూన్యసూచిక;
    }

    // Resolve a modport expression to find the referenced symbol
    VSymEnt* resolve_Modport_వ్యక్తీకరణ(File_పంక్తి* fl, Ast_కణుపు_సమాసం* exprp) {
        UINFO(5, "     resolveModportExpression: " << exprp);
        VSymEnt* symp = శూన్యసూచిక;
        ఒకవేళ (AstParseRef* స్థిరరాశి refp = VN_CAST(exprp, ParseRef)) {
            // Simple variable reference: modport mp(input .a(sig_a))
            symp = m_curSymp->కనుగొను_Id_Fallback(refp->name());
        } లేకపోతే ఒకవేళ (AstVarRef* స్థిరరాశి refp = VN_CAST(exprp, VarRef)) {
            // Already resolved VarRef (can happen if iterateChildren resolved it)
            ఒకవేళ (refp->varScopep()) {
                // During scope creation, VarRef may have VarScope already linked
                symp = m_statep->get_కణుపు_Sym(refp->varScopep());
            } లేకపోతే ఒకవేళ (refp->varp()) {
                symp = m_curSymp->కనుగొను_Id_Fallback(refp->varp()->name());
            }
        } లేకపోతే ఒకవేళ (AstVarXRef* స్థిరరాశి refp = VN_CAST(exprp, VarXRef)) {
            // Resolved VarXRef (dotted reference resolved by earlier pass)
            ఒకవేళ (refp->varp()) {
                // For nested interfaces, the var is in a different scope
                // First try to find the symbol via the var itself
                ఒకవేళ (m_statep->exists_కణుపు_Sym(refp->varp())) {
                    symp = m_statep->get_కణుపు_Sym(refp->varp());
                } లేకపోతే {
                    // Fallback: look up in current scope
                    symp = m_curSymp->కనుగొను_Id_Fallback(refp->varp()->name());
                }
            } లేకపోతే ఒకవేళ (!refp->dotted().ఖాళీలు()) {
                // varp not set yet - use dotted path to find the symbol
                // The dotted part (e.g., "base") is the interface cell, and name is the member
                స్థిరరాశి string నిండిన_మార్గం = refp->dotted() + "." + refp->name();
                string baddot;
                VSymEnt* okSymp = శూన్యసూచిక;
                symp = m_statep->కనుగొను_Dotted(fl, m_curSymp, నిండిన_మార్గం, baddot, okSymp, నిజం);
                // Handle nested interface path when findDotted had partial match
                ఒకవేళ (okSymp && !baddot.ఖాళీలు()) {
                    VSymEnt* స్థిరరాశి resolved = resolve_Nested_Interface_మార్గం(fl, okSymp, baddot);
                    ఒకవేళ (resolved) symp = resolved;
                }
            }
        } లేకపోతే ఒకవేళ (AstDot* స్థిరరాశి dotp = VN_CAST(exprp, Dot)) {
            // Dotted path: modport mp(input .a(inner.sig))
            తార్కిక hasPartSelect = అబద్ధం;
            స్థిరరాశి string dotted_మార్గం = వెలికితీయి_Dotted_మార్గం(dotp, hasPartSelect);
            ఒకవేళ (hasPartSelect) {
                fl->v3warn(
                    E_UNSUPPORTED,
                    "Unsupported: Modport expression with part select (IEEE 1800-2023 25.5.4)");
            } లేకపోతే {
                string baddot;
                VSymEnt* okSymp = శూన్యసూచిక;
                symp = m_statep->కనుగొను_Dotted(fl, m_curSymp, dotted_మార్గం, baddot, okSymp, నిజం);

                // Handle nested interface path when findDotted had partial match
                ఒకవేళ (okSymp && !baddot.ఖాళీలు()) {
                    VSymEnt* స్థిరరాశి resolved = resolve_Nested_Interface_మార్గం(fl, okSymp, baddot);
                    ఒకవేళ (resolved) symp = resolved;
                }

                ఒకవేళ (!symp || !baddot.ఖాళీలు()) {
                    fl->v3error("Can't find modport expression target: "
                                << Ast_కణుపు::prettyNameQ(dotted_మార్గం));
                    symp = శూన్యసూచిక;
                }
            }
        } లేకపోతే ఒకవేళ (VN_IS(exprp, Sel_బిట్టు) || VN_IS(exprp, Sel_వెలికితీయి)) {
            // Part select expressions not yet supported
            fl->v3warn(E_UNSUPPORTED,
                       "Unsupported: Modport expression with part select (IEEE 1800-2023 25.5.4)");
        } లేకపోతే {
            // Other expression types not supported
            fl->v3warn(E_UNSUPPORTED,
                       "Unsupported: Complex modport expression (IEEE 1800-2023 25.5.4)");
        }
        తిరిగి symp;
    }

    శూన్య సందర్శించు(AstModportVarRef* nodep) override {  // IfaceVisitor::
        UINFO(5, "   fiv: " << nodep);
        iterateChildren(nodep);
        VSymEnt* symp = శూన్యసూచిక;
        ఒకవేళ (nodep->exprp()) {
            // Modport expression syntax: modport mp(input .port_name(expression))
            symp = resolve_Modport_వ్యక్తీకరణ(nodep->fileline(), nodep->exprp());
        } లేకపోతే {
            symp = m_curSymp->కనుగొను_Id_Fallback(nodep->name());
        }
        ఒకవేళ (!symp) {
            nodep->v3error("Modport item not found: " << nodep->prettyNameQ());
        } లేకపోతే ఒకవేళ (AstVar* స్థిరరాశి varp = VN_CAST(symp->nodep(), Var)) {
            // Make symbol under modport that points at the _interface_'s var via the modport.
            // (Need modport still to test input/output markings)
            nodep->varp(varp);
            ఒకవేళ (nodep->exprp()) {
                // For modport expressions, insert symbol pointing to the underlying var (not the
                // ModportVarRef which will be deleted during scope creation). The virtual port
                // name maps to the real signal's var.
                VSymEnt* స్థిరరాశి subSymp
                    = m_statep->చొప్పించు_Sym(m_curSymp, nodep->name(), varp, శూన్యసూచిక /*package*/);
                m_statep->చొప్పించు_Scope_Alias(LinkDotState::SAMN_MODPORT, subSymp, symp);
            } లేకపోతే {
                // For regular modport items, insert symbol pointing to ModportVarRef for
                // input/output marking tests.
                m_statep->చొప్పించు_Sym(m_curSymp, nodep->name(), nodep, శూన్యసూచిక /*package*/);
            }
        } లేకపోతే ఒకవేళ (AstVarScope* స్థిరరాశి vscp = VN_CAST(symp->nodep(), VarScope)) {
            // Make symbol under modport that points at the _interface_'s var, not the modport.
            nodep->varp(vscp->varp());
            // Only insert symbol for modport expression virtual ports (exprp set).
            // For regular modport items, don't insert into the shared modport table because
            // each instance would overwrite the previous one, causing wrong VarScope lookups.
            // Regular items are found via the modport's fallback to the interface.
            ఒకవేళ (nodep->exprp()) {
                VSymEnt* స్థిరరాశి subSymp
                    = m_statep->చొప్పించు_Sym(m_curSymp, nodep->name(), vscp, శూన్యసూచిక /*package*/);
                m_statep->చొప్పించు_Scope_Alias(LinkDotState::SAMN_MODPORT, subSymp, symp);
            }
        } లేకపోతే {
            nodep->v3error("Modport item is not a variable: " << nodep->prettyNameQ());
        }
        ఒకవేళ (m_statep->forScopeCreation()) {
            // Done with AstModportVarRef.
            // Delete to prevent problems if we dead-delete pointed to variable
            nodep->unlink_Fr_వెనుక();
            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
        }
    }
    శూన్య సందర్శించు(AstModportClockingRef* nodep) override {  // IfaceVisitor::
        UINFO(5, "   fic: " << nodep);
        iterateChildren(nodep);
        VSymEnt* స్థిరరాశి symp = m_curSymp->కనుగొను_Id_Fallback(nodep->name());
        ఒకవేళ (!symp) {
            nodep->v3error("Modport item not found: " << nodep->prettyNameQ());
        } లేకపోతే ఒకవేళ (AstClocking* స్థిరరాశి clockingp = VN_CAST(symp->nodep(), Clocking)) {
            nodep->clockingp(clockingp);
            m_statep->చొప్పించు_Sym(m_curSymp, nodep->name(), nodep, శూన్యసూచిక /*package*/);
        } లేకపోతే {
            nodep->v3error("Modport item is not a clocking block: " << nodep->prettyNameQ());
        }
    }
    శూన్య సందర్శించు(AstScope* nodep) override {  // IfaceVisitor::
        ఒకవేళ (nodep->user3_సమితి_Once()) తిరిగి;
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override { iterateChildren(nodep); }  // IfaceVisitor::

బహిరంగ:
    // CONSTRUCTORS
    LinkDotIfaceVisitor(AstIface* nodep, VSymEnt* curSymp, LinkDotState* statep)
        : m_statep{statep}
        , m_curSymp{curSymp} {
        UINFO(4, __FUNCTION__ << ": ");
        iterate(nodep);
    }
    ~LinkDotIfaceVisitor() override = అప్రమేయ;
};

శూన్య LinkDotState::computeIfaceModSyms() {
    //  AstScope::user3()  // bool. Already iterated by a LinkDotIfaceVisitor
    స్థిరరాశి VNUser3_లోపల_Use user3_లోపల_Use;
    కొరకు (స్థిరరాశి స్వయంచాలక& itr : m_ifaceModSyms) {
        AstIface* స్థిరరాశి nodep = itr.మొదటి;
        VSymEnt* స్థిరరాశి symp = itr.రెండవ;
        LinkDotIfaceVisitor{nodep, symp, ఇది};
    }
    m_ifaceModSyms.clear();
}

//======================================================================

తరగతి LinkDotResolveVisitor final : బహిరంగ VNVisitor {
    // NODE STATE
    // Cleared on global
    //  *::user1p()             -> See LinkDotState
    //  *::user2p()             -> See LinkDotState
    //  *::user3()              // bool. Processed
    //  *::user4()              -> See LinkDotState
    స్థిరరాశి VNUser3_లోపల_Use m_inuser3;

    // TYPES
    గణన DotPosition : uint8_t {
        // Must match ascii() method below
        DP_NONE = 0,  // Not under a DOT
        DP_PACKAGE,  // {package-or-class}:: DOT
        DP_మొదటి,  // {scope-or-var} DOT
        DP_SCOPE,  // DOT... {scope-or-var} DOT
        DP_FINAL,  // [DOT...] {var-or-func-or-dtype} with no following dots
        DP_MEMBER  // DOT {member-name} [DOT...]
    };

    // STATE
    LinkDotState* స్థిరరాశి m_statep;  // State, including dotted symbol table
    VSymEnt* m_curSymp = శూన్యసూచిక;  // SymEnt for current lookup point
    VSymEnt* m_modSymp = శూన్యసూచిక;  // SymEnt for current module
    VSymEnt* m_pinSymp = శూన్యసూచిక;  // SymEnt for pin lookups
    VSymEnt* m_randSymp = శూన్యసూచిక;  // SymEnt for randomize target class's lookups
    స్థిరరాశి AstCell* m_cellp = శూన్యసూచిక;  // Current cell
    Ast_కణుపు_Module* m_modp = శూన్యసూచిక;  // Current module
    Ast_కణుపు_FTask* m_ftaskp = శూన్యసూచిక;  // Current function/task
    Ast_పద్ధతి_Call* m_rand_పద్ధతి_Callp = శూన్యసూచిక;  // Current randomize() method call
    పూర్ణాంకం m_modport_అంకె = 0;  // Uniqueify modport numbers
    పూర్ణాంకం m_indent = 0;  // Indentation (tree depth) for debug
    తార్కిక m_లోపల_Sens = అబద్ధం;  // True if in senitem
    స్థిరరాశి AstWith* m_currentWithp = శూన్యసూచిక;  // Enclosing 'with' (nullptr if none)
    std::సమితి<std::string>
        m_restricted_పేర్లు_వాడిన;  // Names from current 'with (id_list)' that resolved into target
    తార్కిక m_genericIfaceModule = అబద్ధం;  // True if in module containing generic interface
    std::map<std::string, Ast_కణుపు*> m_if_Class_Imp_పేర్లు;  // Names imported from interface class
    // Inherited interface implementations needing a forward in the current class, by name
    std::map<std::string, స్థిరరాశి VSymEnt*> m_ifForwards;
    std::సమితి<AstClass*> m_extendsParam;  // Classes that have a parameterized super class
                                         // (except the default instances)
                                         // They are added to the set only in linkDotPrimary.
    తార్కిక m_insideClassExtParam = అబద్ధం;  // Inside a class from m_extendsParam
    AstNew* m_explicitSuperNewp = శూన్యసూచిక;  // Hit a "super.new" call inside a "new" function
    std::map<Ast_కణుపు*, AstPin*> m_వాడిన_Pins;  // Pin used in this cell, map to duplicate
    std::map<Ast_కణుపు*, std::map<std::string, AstPin*>> m_వాడిన_Def_Param_Pins;
    // Defparam pins used for a given formal, by hierarchical path
    std::map<std::string, Ast_కణుపు_Module*> m_modules_గమ్యం_Revisit;  // Modules to revisit a second time
    Ast_కణుపు* m_చివరి_Deferredp = శూన్యసూచిక;  // Last node which requested a revisit of its module
    Ast_కణుపు_DType* m_packedArrayDtp = శూన్యసూచిక;  // Datatype reference for packed array
    తార్కిక m_లోపల_Packed_Array = అబద్ధం;  // Currently traversing a packed array tree
    తార్కిక m_replaceWithAlias
        = నిజం;  // Replace VarScope with an alias. Used in the handling of AstAlias
    తార్కిక m_isParam = అబద్ధం;  // Specifies whether currently visiting param variable
    తార్కిక m_resolvingTypedef = అబద్ధం;  // Currently traversing a Typedef tree

    నిర్మాణం Dot_స్థితులు final {
        DotPosition m_dotPos;  // Scope part of dotted resolution
        VSymEnt* m_dotSymp;  // SymEnt for dotted AstParse lookup
        స్థిరరాశి AstDot* m_dotp;  // Current dot
        తార్కిక m_super;  // Starts with super reference
        తార్కిక m_unresolvedCell;  // Unresolved cell, needs help from V3Param
        తార్కిక m_unresolvedClass;  // Unresolved class reference, needs help from V3Param
        తార్కిక m_unresolvedGenericIface;  // Unresolved generic interface, needs help from V3Param
        తార్కిక m_genBlk;  // Contains gen block reference
        Ast_కణుపు* m_unlinkedScopep;  // Unresolved scope, needs corresponding VarXRef
        AstDisable* m_disablep;  // Disable statement under which the reference is
        తార్కిక m_dotErr;  // Error found in dotted resolution, ignore upwards
        string m_dot_పాఠ్యం;  // String of dotted names found in below parseref
        Dot_స్థితులు() { init(శూన్యసూచిక); }
        ~Dot_స్థితులు() = అప్రమేయ;
        శూన్య init(VSymEnt* curSymp, తార్కిక unresolvedClass = అబద్ధం) {
            m_dotPos = DP_NONE;
            m_dotSymp = curSymp;
            m_dotp = శూన్యసూచిక;
            m_super = అబద్ధం;
            m_dotErr = అబద్ధం;
            m_dot_పాఠ్యం = "";
            m_unresolvedCell = అబద్ధం;
            m_unresolvedClass = unresolvedClass;
            m_unresolvedGenericIface = అబద్ధం;
            m_genBlk = అబద్ధం;
            m_unlinkedScopep = శూన్యసూచిక;
            m_disablep = శూన్యసూచిక;
        }
        string ascii() స్థిరరాశి {
            స్థిర స్థిరరాశి అక్షరం* స్థిరరాశి పేర్లు[]
                = {"DP_NONE", "DP_PACKAGE", "DP_FIRST", "DP_SCOPE", "DP_FINAL", "DP_MEMBER"};
            std::ostringstream sstr;
            sstr << "ds=" << పేర్లు[m_dotPos];
            sstr << "  dse" << cvt_గమ్యం_Hex(m_dotSymp);
            స్థిరరాశి string dsname = m_dotSymp->nodep()->name().substr(0, 8);
            sstr << "(" << m_dotSymp->nodep()->typeName() << (dsname.ఖాళీలు() ? "" : ":") << dsname
                 << ")";
            ఒకవేళ (m_dotErr) sstr << "  [dotErr]";
            ఒకవేళ (m_super) sstr << "  [super]";
            ఒకవేళ (m_unresolvedCell) sstr << "  [unrCell]";
            ఒకవేళ (m_unresolvedClass) sstr << "  [unrClass]";
            ఒకవేళ (m_unresolvedGenericIface) sstr << "  [unrGIface]";
            ఒకవేళ (m_genBlk) sstr << "  [genBlk]";
            sstr << "  txt=" << m_dot_పాఠ్యం;
            తిరిగి sstr.str();
        }
    } m_ds;  // State to preserve across recursions

    // METHODS - Variables
    AstVar* createImplicitVar(VSymEnt* /*lookupSymp*/, AstParseRef* nodep, Ast_కణుపు_Module* modp,
                              VSymEnt* moduleSymp, తార్కిక noWarn) {
        // Create implicit after warning
        ఒకవేళ (!noWarn) {
            ఒకవేళ (nodep->fileline()->warnIsOff(V3_Error_సంకేతం::I_DEF_NETTYPE_WIRE)) {
                స్థిరరాశి string suggest = m_statep->suggestSymFallback(moduleSymp, nodep->name(),
                                                                    Link_కణుపు_Matcher_Var{});
                nodep->v3error("Signal definition not found, and implicit disabled with "
                               "`default_nettype: "
                               << nodep->prettyNameQ() << '\n'
                               << (suggest.ఖాళీలు() ? "" : nodep->warn_మరిన్ని() + suggest));

            }
            // Bypass looking for suggestions if IMPLICIT is turned off
            // as there could be thousands of these suppressed in large netlists
            లేకపోతే ఒకవేళ (!nodep->fileline()->warnIsOff(V3_Error_సంకేతం::IMPLICIT)) {
                స్థిరరాశి string suggest = m_statep->suggestSymFallback(moduleSymp, nodep->name(),
                                                                    Link_కణుపు_Matcher_Var{});
                nodep->v3warn(IMPLICIT,
                              "Signal definition not found, creating implicitly: "
                                  << nodep->prettyNameQ() << '\n'
                                  << (suggest.ఖాళీలు() ? "" : nodep->warn_మరిన్ని() + suggest));
            }
        }
        AstVar* స్థిరరాశి newp
            = కొత్త AstVar{nodep->fileline(), VVarType::WIRE, nodep->name(), VFlagLogicPacked{}, 1};
        newp->lifetime(VLifetime::STATIC_IMPLICIT);
        newp->trace(modp->modTrace());
        modp->కూడు_Stmtsp(newp);
        // Link it to signal list, must add the variable under the module;
        // current scope might be lower now
        m_statep->చొప్పించు_Sym(moduleSymp, newp->name(), newp, శూన్యసూచిక /*classOrPackagep*/);
        తిరిగి newp;
    }
    AstVar* దొరికింది_గమ్యం_Varp(స్థిరరాశి VSymEnt* symp, Ast_కణుపు* nodep, VAccess access) {
        // Return a variable if possible, auto converting a modport to variable
        ఒకవేళ (!symp) {
            తిరిగి శూన్యసూచిక;
        } లేకపోతే ఒకవేళ (VN_IS(symp->nodep(), Var)) {
            తిరిగి VN_AS(symp->nodep(), Var);
        } లేకపోతే ఒకవేళ (VN_IS(symp->nodep(), ModportVarRef)) {
            AstModportVarRef* స్థిరరాశి snodep = VN_AS(symp->nodep(), ModportVarRef);
            AstVar* స్థిరరాశి varp = snodep->varp();
            ఒకవేళ (access.isWriteOrRW() && snodep->direction().isReadOnly()) {
                nodep->v3error("Attempt to drive input-only modport: " << nodep->prettyNameQ());
            }  // else other simulators don't warn about reading, and IEEE doesn't say illegal
            తిరిగి varp;
        } లేకపోతే {
            తిరిగి శూన్యసూచిక;
        }
    }

    // Look up a virtual port through modport expression mapping.
    // When a dotted reference uses a modport with expression syntax, the virtual port name
    // maps to the real signal. This helper resolves that mapping during scope creation.
    // Returns the VarScope for the real signal, or nullptr if not found.
    AstVarScope* కనుగొను_Virtual_పోర్టు_Var_Scope(VSymEnt* dotSymp, స్థిరరాశి string& పోర్టు_Name) {
        స్థిరరాశి AstVarScope* స్థిరరాశి dotVscp = VN_CAST(dotSymp->nodep(), VarScope);
        స్థిరరాశి AstVar* స్థిరరాశి dotVarp = dotVscp ? dotVscp->varp() : శూన్యసూచిక;
        ఒకవేళ (!dotVarp) తిరిగి శూన్యసూచిక;

        Ast_కణుపు_DType* dtypep = dotVarp->బిడ్డ_DTypep();
        ఒకవేళ (!dtypep) dtypep = dotVarp->subDTypep();
        ఒకవేళ (!dtypep) తిరిగి శూన్యసూచిక;
        స్థిరరాశి AstIfaceRefDType* స్థిరరాశి ifaceRefp = VN_CAST(dtypep, IfaceRefDType);
        ఒకవేళ (!ifaceRefp || !ifaceRefp->modportp() || !ifaceRefp->iface_ద్వారా_Cellp()) తిరిగి శూన్యసూచిక;

        // Get the interface cell's symbol table
        VSymEnt* స్థిరరాశి ifaceCellSymp = m_statep->get_కణుపు_Sym(ifaceRefp->iface_ద్వారా_Cellp());

        // Look up modport by name in interface cell
        VSymEnt* స్థిరరాశి modportSymp = ifaceCellSymp->కనుగొను_Id_Fallback(ifaceRefp->modportp()->name());
        ఒకవేళ (!modportSymp) తిరిగి శూన్యసూచిక;

        // Look up virtual port name in modport symbol table
        string baddot;
        VSymEnt* స్థిరరాశి virtPortp = m_statep->కనుగొను_Sym_Prefixed(modportSymp, పోర్టు_Name, baddot, నిజం);
        ఒకవేళ (!virtPortp) తిరిగి శూన్యసూచిక;

        // Symbol may point to VarScope or Var
        ఒకవేళ (AstVarScope* vscp = VN_CAST(virtPortp->nodep(), VarScope)) తిరిగి vscp;

        // Symbol points to Var - look up VarScope by name in interface cell
        ఒకవేళ (స్థిరరాశి AstVar* స్థిరరాశి realVarp = VN_CAST(virtPortp->nodep(), Var)) {
            VSymEnt* స్థిరరాశి realFoundp
                = m_statep->కనుగొను_Sym_Prefixed(ifaceCellSymp, realVarp->name(), baddot, నిజం);
            తిరిగి realFoundp ? VN_CAST(realFoundp->nodep(), VarScope) : శూన్యసూచిక;
        }
        తిరిగి శూన్యసూచిక;
    }

    // Capture a ParamTypeDType reference for interface typedef retargeting.
    // Called when a RefDType resolves to a ParamTypeDType owned by an interface.
    శూన్య captureIfaceParamType(AstRefDType* nodep, AstParamTypeDType* defp) {
        ఒకవేళ (!V3LinkDotIfaceCapture::enabled() || !m_statep->forPrimary()) తిరిగి;
        Ast_కణుపు_Module* స్థిరరాశి defOwnerModp = V3LinkDotIfaceCapture::కనుగొను_Owner_Module(defp);
        ఒకవేళ (!defOwnerModp || !VN_IS(defOwnerModp, Iface)) తిరిగి;
        AstCell* స్థిరరాశి cellForCapture
            = m_ds.m_dotSymp ? VN_CAST(m_ds.m_dotSymp->nodep(), Cell) : శూన్యసూచిక;
        ఒకవేళ (!cellForCapture) తిరిగి;
        UINFO(9, indent() << "iface capture add paramtype " << nodep
                          << " iface=" << defOwnerModp->prettyNameQ());
        V3LinkDotIfaceCapture::కూడు_Param_Type(nodep, cellForCapture->name(), m_modp, defp,
                                            defOwnerModp->name());
    }

    Ast_కణుపు_Stmt* కూడు_Implicit_Super_New_Call(AstFunc* స్థిరరాశి nodep,
                                         స్థిరరాశి AstClassExtends* స్థిరరాశి classExtendsp) {
        // Returns the added node
        File_పంక్తి* స్థిరరాశి fl = nodep->fileline();
        AstArg* argsp = శూన్యసూచిక;
        ఒకవేళ (classExtendsp->argsp()) argsp = classExtendsp->argsp()->clone_వృక్షం(నిజం);
        AstNew* స్థిరరాశి newExprp = కొత్త AstNew{fl, argsp};
        newExprp->isImplicit(నిజం);
        AstDot* స్థిరరాశి superNewp = కొత్త AstDot{fl, అబద్ధం, కొత్త AstParseRef{fl, "super"}, newExprp};
        Ast_కణుపు_Stmt* స్థిరరాశి superNewStmtp = superNewp->makeStmt();
        కొరకు (Ast_కణుపు* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            // super.new shall be the first statement (IEEE 1800-2023 8.15)
            // but some nodes (such as variable declarations and typedefs) should stay before
            ఒకవేళ (VN_IS(stmtp, కణుపు_Stmt)) {
                stmtp->కూడు_ఇక్కడ_This_As_తదుపరి(superNewStmtp);
                తిరిగి superNewStmtp;
            }
        }
        // There were no statements
        nodep->కూడు_Stmtsp(superNewStmtp);
        తిరిగి superNewStmtp;
    }
    శూన్య checkNoDot(Ast_కణుపు* nodep) {
        ఒకవేళ (VL_UNLIKELY(!nodep)) {
            UINFO(9, indent() << "iface capture null node passed to checkNoDot; dot state="
                              << m_ds.ascii());
            m_ds.m_dotErr = నిజం;
            తిరిగి;
        }
        ఒకవేళ (VL_UNLIKELY(m_ds.m_dotPos != DP_NONE)) {
            UINFO(9, indent() << "ds=" << m_ds.ascii());
            UINFO(9, indent() << "iface capture checkNoDot hit node=" << nodep
                              << " dotState=" << m_ds.ascii());
            ఒకవేళ (VL_UNLIKELY(!m_ds.m_dotp)) {
                nodep->v3error("Syntax error: Not expecting " << nodep->type()
                                                              << " here (missing dot context)");
            } లేకపోతే {
                nodep->v3error("Syntax error: Not expecting "
                               << nodep->type() << " under a " << nodep->backp()->type()
                               << " in dotted expression\n"
                               << nodep->warnContextPrimary() << m_ds.m_dotp->warnOther()
                               << "... Resolving this reference\n"
                               << m_ds.m_dotp->warnContextSecondary());
            }
            m_ds.m_dotErr = నిజం;
        }
    }
    AstVar* కనుగొను_Iface_శిఖరం_Varp(Ast_కణుపు* nodep, VSymEnt* మాతృ_Entp, స్థిరరాశి string& name) {
        స్థిరరాశి string కనుగొను_Name = name + "__Viftop";
        VSymEnt* స్థిరరాశి ifaceSymp = మాతృ_Entp->కనుగొను_Id_Fallback(కనుగొను_Name);
        AstVar* స్థిరరాశి iface_శిఖరం_Varp = ifaceSymp ? VN_AS(ifaceSymp->nodep(), Var) : శూన్యసూచిక;
        UASSERT_OBJ(iface_శిఖరం_Varp, nodep, "Can't find interface var ref: " << కనుగొను_Name);
        తిరిగి iface_శిఖరం_Varp;
    }
    AstRSProd* కనుగొను_Prod(Ast_కణుపు* nodep, VSymEnt* మాతృ_Entp, స్థిరరాశి string& name) {
        స్థిరరాశి VSymEnt* స్థిరరాశి foundp = మాతృ_Entp->కనుగొను_Id_Fallback(name);
        AstRSProd* దొరికింది_Nodep = foundp ? VN_CAST(foundp->nodep(), RSProd) : శూన్యసూచిక;
        ఒకవేళ (!దొరికింది_Nodep) {
            VSpellCheck speller;
            Link_కణుపు_Matcher_Prod matcher;
            మాతృ_Entp->candidateIdFlat(&speller, &matcher, అబద్ధం);
            స్థిరరాశి string suggest = speller.ఉత్తమ_Candidate_సందేశం(name);
            UINFO(1, "   ErrParseRef curSymp=se" << cvt_గమ్యం_Hex(మాతృ_Entp));
            nodep->v3error("Production " << Ast_కణుపు::prettyNameQ(name) << " not found\n"
                                         << (suggest.ఖాళీలు() ? "" : nodep->warn_మరిన్ని() + suggest));
            తిరిగి శూన్యసూచిక;
        }
        తిరిగి దొరికింది_Nodep;
    }
    శూన్య duplicatePinError(AstPin* nodep, Ast_కణుపు* origp, స్థిరరాశి అక్షరం* whatp) {
        nodep->v3error("Duplicate " << whatp << " connection: " << nodep->prettyNameQ() << '\n'
                                    << nodep->warnContextPrimary() << '\n'
                                    << origp->warnOther() << "... Location of original " << whatp
                                    << " connection\n"
                                    << origp->warnContextSecondary());
    }
    శూన్య markAndCheckPinDup(AstPin* nodep, Ast_కణుపు* refp, స్థిరరాశి అక్షరం* whatp) {
        స్థిరరాశి స్వయంచాలక pair = m_వాడిన_Pins.emplace(refp, nodep);
        ఒకవేళ (pair.రెండవ) {
            ఒకవేళ (!nodep->param_మార్గం().ఖాళీలు())
                m_వాడిన_Def_Param_Pins[refp].emplace(nodep->param_మార్గం(), nodep);
            తిరిగి;
        } లేకపోతే {
            Ast_కణుపు* స్థిరరాశి origp = pair.మొదటి->రెండవ;
            ఒకవేళ (nodep->param_మార్గం().ఖాళీలు() || VN_AS(origp, Pin)->param_మార్గం().ఖాళీలు()) {
                duplicatePinError(nodep, origp, whatp);
                తిరిగి;
            }
        }

        స్వయంచాలక& defParamPins = m_వాడిన_Def_Param_Pins[refp];
        స్థిరరాశి స్వయంచాలక defParamIt = defParamPins.కనుగొను(nodep->param_మార్గం());
        ఒకవేళ (defParamIt != defParamPins.చివర()) {
            duplicatePinError(nodep, defParamIt->రెండవ, whatp);
            తిరిగి;
        }
        defParamPins.emplace(nodep->param_మార్గం(), nodep);
    }
    స్థిర తార్కిక extendsClass(స్థిరరాశి AstClass* classp, స్థిరరాశి AstClass* targetp) {
        // Return true if classp inherits from targetp through already resolved extends
        // If performance of this becomes a problem, use just a counter to check
        // exceeds some number of iterations then perform the more expensive analysis
        std::సమితి<స్థిరరాశి AstClass*> సందర్శించిన;
        std::సదిశ<స్థిరరాశి AstClass*> todo{classp};
        అయితే (!todo.ఖాళీలు()) {
            స్థిరరాశి AstClass* స్థిరరాశి currp = todo.వెనుక();
            todo.తీయి_వెనుక();
            ఒకవేళ (currp == targetp) తిరిగి నిజం;
            ఒకవేళ (!సందర్శించిన.చొప్పించు(currp).రెండవ) కొనసాగు;
            కొరకు (స్థిరరాశి AstClassExtends* cextp = currp->extendsp(); cextp;
                 cextp = VN_AS(cextp->nextp(), ClassExtends)) {
                ఒకవేళ (స్థిరరాశి AstClass* స్థిరరాశి basep = cextp->classOrNullp()) todo.నెట్టు_వెనుక(basep);
            }
        }
        తిరిగి అబద్ధం;
    }
    స్థిర AstClocking* sensClockingp(Ast_కణుపు* nodep) {
        // Return the clocking block referenced by nodep, either directly or through a modport
        ఒకవేళ (AstClocking* స్థిరరాశి clockingp = VN_CAST(nodep, Clocking)) తిరిగి clockingp;
        ఒకవేళ (స్థిరరాశి AstModportClockingRef* స్థిరరాశి clockingRefp = VN_CAST(nodep, ModportClockingRef))
            తిరిగి clockingRefp->clockingp();
        తిరిగి శూన్యసూచిక;
    }
    VSymEnt* getCreateClockingEventSymEnt(AstClocking* clockingp) {
        AstVar* స్థిరరాశి eventp = clockingp->ensureEventp(నిజం);
        ఒకవేళ (!eventp->user1p()) eventp->user1p(కొత్త VSymEnt{m_statep->symsp(), eventp});
        తిరిగి reinterpret_cast<VSymEnt*>(eventp->user1p());
    }
    VSymEnt* కనుగొను_Inlined_Dots_Sym(Ast_కణుపు* nodep, స్థిరరాశి string& inlinedDots) {
        // Return scope that applies for searching, given renaming due to module inlining.
        // Dotted lookup is always relative to module, as maybe variable
        // name lower down with same scope name we want to ignore (t_math_divw)
        VSymEnt* dotSymp = m_modSymp;
        స్థిరరాశి string inl = Ast_కణుపు::dedotName(inlinedDots);
        string baddot;
        VSymEnt* okSymp = శూన్యసూచిక;
        dotSymp = m_statep->కనుగొను_Dotted(nodep->fileline(), dotSymp, inl, baddot, okSymp, అబద్ధం);
        UASSERT_OBJ(dotSymp, nodep,
                    "Couldn't resolve inlined scope " << Ast_కణుపు::prettyNameQ(baddot)
                                                      << " in: " << inlinedDots);
        UINFO(9, indent() << "findInlinedDotsSym " << dotSymp
                          << " search start due to inlinedDots=" << inlinedDots);
        తిరిగి dotSymp;
    }
    VSymEnt* కనుగొను_Dotted_Ref_Symp(Ast_కణుపు* nodep, VSymEnt* lookupSymp, స్థిరరాశి string& dotted,
                               string& baddot, VSymEnt*& okSymp, తార్కిక* modportp = శూన్యసూచిక) {
        // Return scope to search for a relinked hierarchical reference
        // (AstVarXRef or AstEnumItemRef), or nullptr after error if not found
        // If modportp is given, set true when the reference went through a modport
        ఒకవేళ (modportp) *modportp = అబద్ధం;
        VSymEnt* dotSymp = m_statep->కనుగొను_Dotted(nodep->fileline(), lookupSymp, dotted, baddot,
                                                okSymp, నిజం);  // Maybe nullptr
        ఒకవేళ (!dotSymp) {
            nodep->v3error(
                "Can't find definition of "
                << (!baddot.ఖాళీలు() ? Ast_కణుపు::prettyNameQ(baddot) : nodep->prettyNameQ()) << '\n'
                << nodep->warnContextPrimary());
            తిరిగి శూన్యసూచిక;
        }
        ఒకవేళ (స్థిరరాశి AstVar* స్థిరరాశి varp = VN_CAST(dotSymp->nodep(), Var)) {
            ఒకవేళ (స్థిరరాశి AstIfaceRefDType* స్థిరరాశి ifaceRefp
                = VN_CAST(varp->బిడ్డ_DTypep(), IfaceRefDType)) {
                ఒకవేళ (ifaceRefp->modportp()) {
                    dotSymp = m_statep->get_కణుపు_Sym(ifaceRefp->modportp());
                    ఒకవేళ (modportp) *modportp = నిజం;
                } లేకపోతే {
                    dotSymp = m_statep->get_కణుపు_Sym(ifaceRefp->ifacep());
                }
            }
        } లేకపోతే ఒకవేళ (స్థిరరాశి AstModportClockingRef* స్థిరరాశి clockingRefp
                   = VN_CAST(dotSymp->nodep(), ModportClockingRef)) {
            dotSymp = m_statep->get_కణుపు_Sym(clockingRefp->clockingp());
        }
        తిరిగి dotSymp;
    }

    స్థిర తార్కిక isParamedClassRefDType(స్థిరరాశి Ast_కణుపు* classp) {
        అయితే (స్థిరరాశి AstRefDType* స్థిరరాశి refp = VN_CAST(classp, RefDType))
            classp = refp->subDTypep();
        తిరిగి (VN_IS(classp, ClassRefDType) && VN_AS(classp, ClassRefDType)->paramsp())
               || VN_IS(classp, ParamTypeDType);
    }
    తార్కిక isParamedClassRef(స్థిరరాశి Ast_కణుపు* nodep) {
        // Is this a parameterized reference to a class, or a reference to class parameter
        ఒకవేళ (స్థిరరాశి స్వయంచాలక* classRefp = VN_CAST(nodep, ClassOrPackageRef)) {
            ఒకవేళ (classRefp->paramsp()) తిరిగి నిజం;
            స్థిరరాశి స్వయంచాలక* classp = classRefp->classOrPackageNodep();
            అయితే (స్థిరరాశి స్వయంచాలక* typedefp = VN_CAST(classp, Typedef)) classp = typedefp->subDTypep();
            తిరిగి isParamedClassRefDType(classp);
        }
        తిరిగి isParamedClassRefDType(nodep);
    }
    VSymEnt* getThisClassSymp() {
        VSymEnt* classSymp = m_ds.m_dotSymp;
        అయితే (classSymp && !VN_IS(classSymp->nodep(), Class)) {
            classSymp = classSymp->parentp();
        }
        తిరిగి classSymp;
    }
    // Is the current scope within the given class, including within its nested classes
    తార్కిక insideClass(స్థిరరాశి AstClass* classp) స్థిరరాశి {
        కొరకు (స్థిరరాశి VSymEnt* symp = m_curSymp; symp; symp = symp->parentp()) {
            ఒకవేళ (symp->nodep() == classp) తిరిగి నిజం;
        }
        తిరిగి అబద్ధం;
    }
    // 'randomize() with' target class not yet specialized; defer to post-V3Param pass
    తార్కిక rand_లక్ష్యం_Unresolved() స్థిరరాశి {
        తిరిగి m_statep->forPrimary() && m_currentWithp && m_rand_పద్ధతి_Callp && !m_randSymp;
    }
    // Is 'funcp' of 'classp' virtual, directly or by overriding a virtual method of a base
    // class (IEEE 1800-2023 8.20)
    తార్కిక is_Virtual_పద్ధతి(స్థిరరాశి Ast_కణుపు_FTask* funcp, స్థిరరాశి AstClass* classp) స్థిరరాశి {
        ఒకవేళ (funcp->isVirtual()) తిరిగి నిజం;
        కొరకు (స్థిరరాశి AstClassExtends* cextp = classp->extendsp(); cextp;
             cextp = VN_AS(cextp->nextp(), ClassExtends)) {
            AstClass* స్థిరరాశి basep = cextp->classOrNullp();
            ఒకవేళ (!basep) కొనసాగు;
            స్థిరరాశి VSymEnt* స్థిరరాశి foundp = m_statep->get_కణుపు_Sym(basep)->కనుగొను_Id_Flat(funcp->name());
            స్థిరరాశి Ast_కణుపు_FTask* స్థిరరాశి పునాది_Funcp
                = foundp ? VN_CAST(foundp->nodep(), కణుపు_FTask) : శూన్యసూచిక;
            స్థిరరాశి AstClass* స్థిరరాశి పునాది_Classp = foundp && foundp->parentp()
                                                   ? VN_CAST(foundp->parentp()->nodep(), Class)
                                                   : శూన్యసూచిక;
            ఒకవేళ (పునాది_Funcp && పునాది_Classp && is_Virtual_పద్ధతి(పునాది_Funcp, పునాది_Classp)) తిరిగి నిజం;
        }
        తిరిగి అబద్ధం;
    }
    // Is imported 'implp' an implementation of interface method 'ifSymp' (IEEE 1800-2023
    // 8.26.2)? If its class does not derive from the interface class, it needs a forwarding
    // override in the derived class to be reachable through the interface.
    తార్కిక inheritedImplements(స్థిరరాశి VSymEnt* implp, స్థిరరాశి VSymEnt* ifSymp, తార్కిక& needForwardr) {
        needForwardr = అబద్ధం;
        స్థిరరాశి Ast_కణుపు_FTask* స్థిరరాశి implFuncp = VN_CAST(implp->nodep(), కణుపు_FTask);
        ఒకవేళ (!implFuncp || implFuncp->శుద్ధ_Virtual() || implFuncp->isStatic()) తిరిగి అబద్ధం;
        స్థిరరాశి AstClass* స్థిరరాశి implClassp
            = implp->parentp() ? VN_CAST(implp->parentp()->nodep(), Class) : శూన్యసూచిక;
        స్థిరరాశి AstClass* స్థిరరాశి ifClassp
            = ifSymp->parentp() ? VN_CAST(ifSymp->parentp()->nodep(), Class) : శూన్యసూచిక;
        ఒకవేళ (!implClassp || !ifClassp) తిరిగి అబద్ధం;
        ఒకవేళ (extendsClass(implClassp, ifClassp)) తిరిగి నిజం;
        ఒకవేళ (!is_Virtual_పద్ధతి(implFuncp, implClassp)) తిరిగి అబద్ధం;
        needForwardr = నిజం;
        తిరిగి నిజం;
    }
    // Add 'implp' override to 'classp' that calls super.implp, so the C++ method of an
    // implemented interface class is overridden
    శూన్య కూడు_Interface_Forward(AstClass* classp, Ast_కణుపు_FTask* implp, AstClass* implClassp) {
        File_పంక్తి* స్థిరరాశి flp = implp->fileline();
        AstVar* fvarp = శూన్యసూచిక;
        Ast_కణుపు_FTask* newp;
        ఒకవేళ (AstFunc* స్థిరరాశి implFuncp = VN_CAST(implp, Func)) {
            fvarp = VN_AS(implFuncp->fvarp(), Var)->clone_వృక్షం(అబద్ధం);
            newp = కొత్త AstFunc{flp, implp->name(), శూన్యసూచిక, fvarp};
        } లేకపోతే {
            newp = కొత్త AstTask{flp, implp->name(), శూన్యసూచిక};
        }
        newp->class_పద్ధతి(నిజం);
        newp->isVirtual(నిజం);
        newp->lifetime(implp->lifetime());
        AstArg* argsp = శూన్యసూచిక;
        కొరకు (Ast_కణుపు* stmtp = implp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstVar* స్థిరరాశి portp = VN_CAST(stmtp, Var);
            ఒకవేళ (!portp || !portp->isIO()) కొనసాగు;
            AstVar* స్థిరరాశి newPortp = portp->clone_వృక్షం(అబద్ధం);
            newp->కూడు_Stmtsp(newPortp);
            argsp = Ast_కణుపు::కూడు_తదుపరి(
                argsp, కొత్త AstArg{flp, "", కొత్త AstVarRef{flp, newPortp, VAccess::READ}});
        }
        Ast_కణుపు_FTask_Ref* callp;
        ఒకవేళ (AstFunc* స్థిరరాశి implFuncp = VN_CAST(implp, Func)) {
            callp = కొత్త AstFuncRef{flp, implFuncp, argsp};
        } లేకపోతే {
            callp = కొత్త AstTaskRef{flp, VN_AS(implp, Task), argsp};
        }
        callp->classOrPackagep(implClassp);
        callp->superReference(నిజం);
        ఒకవేళ (fvarp) {
            newp->కూడు_Stmtsp(కొత్త Ast_కేటాయించు{flp, కొత్త AstVarRef{flp, fvarp, VAccess::WRITE}, callp});
        } లేకపోతే {
            newp->కూడు_Stmtsp(callp->makeStmt());
        }
        UINFO(9, indent() << "Interface forward " << newp);
        classp->కూడు_Membersp(newp);
    }
    శూన్య importDerivedClass(AstClass* derivedClassp, VSymEnt* పునాది_Symp, AstClass* పునాది_Classp) {
        // Also used for standard 'extends' from a base class
        UINFO(8, indent() << "importDerivedClass to " << derivedClassp << " from " << పునాది_Classp);
        కొరకు (VSymEnt::const_iterator it = పునాది_Symp->begin(); it != పునాది_Symp->చివర(); ++it) {
            ఒకవేళ (Ast_కణుపు* పునాది_Subp = it->రెండవ->nodep()) {
                UINFO(8, indent() << "  SymFunc " << పునాది_Subp);
                స్థిరరాశి string impOrExtends
                    = పునాది_Classp->isInterfaceClass() ? " implements " : " extends ";
                ఒకవేళ (VN_IS(పునాది_Subp, కణుపు_FTask)) {
                    స్థిరరాశి VSymEnt* స్థిరరాశి foundp = m_curSymp->కనుగొను_Id_Flat(పునాది_Subp->name());
                    స్థిరరాశి Ast_కణుపు_FTask* స్థిరరాశి పునాది_Funcp = VN_CAST(పునాది_Subp, కణుపు_FTask);
                    ఒకవేళ (!పునాది_Funcp || !పునాది_Funcp->శుద్ధ_Virtual()) కొనసాగు;
                    తార్కిక needForward = అబద్ధం;
                    స్థిరరాశి తార్కిక exists_లోపల_Derived
                        = foundp
                          && (!foundp->imported()
                              || inheritedImplements(foundp, it->రెండవ, needForward));
                    ఒకవేళ (needForward && m_statep->forPrimary()
                        && !derivedClassp->isInterfaceClass()) {
                        m_ifForwards.emplace(పునాది_Subp->name(), foundp);
                    }
                    ఒకవేళ (m_statep->forPrimary() && !exists_లోపల_Derived
                        && !derivedClassp->isInterfaceClass() && !derivedClassp->isVirtual()) {
                        derivedClassp->v3error(
                            "Class " << derivedClassp->prettyNameQ() << impOrExtends
                                     << పునాది_Classp->prettyNameQ()
                                     << " but is missing implementation for "
                                     << పునాది_Subp->prettyNameQ() << " (IEEE 1800-2023 8.26)\n"
                                     << derivedClassp->warnContextPrimary() << '\n'
                                     << పునాది_Subp->warnOther()
                                     << "... Location of interface class's function\n"
                                     << పునాది_Subp->warnContextSecondary());
                    }
                    స్థిరరాశి స్వయంచాలక itn = m_if_Class_Imp_పేర్లు.కనుగొను(పునాది_Subp->name());
                    ఒకవేళ (m_statep->forPrimary() && !exists_లోపల_Derived
                        && itn != m_if_Class_Imp_పేర్లు.చివర()
                        && itn->రెండవ != పునాది_Subp) {  // Not exact same function from diamond
                        derivedClassp->v3error(
                            "Class " << derivedClassp->prettyNameQ() << impOrExtends
                                     << పునాది_Classp->prettyNameQ()
                                     << " but missing inheritance conflict resolution for "
                                     << పునాది_Subp->prettyNameQ() << " (IEEE 1800-2023 8.26.6.2)\n"
                                     << derivedClassp->warnContextPrimary() << '\n'
                                     << పునాది_Subp->warnOther()
                                     << "... Location of interface class's function\n"
                                     << పునాది_Subp->warnContextSecondary());
                    }
                    m_if_Class_Imp_పేర్లు.emplace(పునాది_Subp->name(), పునాది_Subp);
                }
                ఒకవేళ (VN_IS(పునాది_Subp, Constraint)) {
                    స్థిరరాశి VSymEnt* స్థిరరాశి foundp = m_curSymp->కనుగొను_Id_Flat(పునాది_Subp->name());
                    స్థిరరాశి AstConstraint* స్థిరరాశి పునాది_Funcp = VN_CAST(పునాది_Subp, Constraint);
                    ఒకవేళ (!పునాది_Funcp || !పునాది_Funcp->is_Kwd_శుద్ధ()) కొనసాగు;
                    స్థిరరాశి తార్కిక exists_లోపల_Derived = foundp && !foundp->imported();
                    ఒకవేళ (m_statep->forPrimary() && !exists_లోపల_Derived
                        && !derivedClassp->isInterfaceClass() && !derivedClassp->isVirtual()) {
                        derivedClassp->v3error(
                            "Class " << derivedClassp->prettyNameQ() << impOrExtends
                                     << పునాది_Classp->prettyNameQ()
                                     << " but is missing constraint implementation for "
                                     << పునాది_Subp->prettyNameQ() << " (IEEE 1800-2023 18.5.2)\n"
                                     << derivedClassp->warnContextPrimary() << '\n'
                                     << పునాది_Subp->warnOther()
                                     << "... Location of interface class's pure constraint\n"
                                     << పునాది_Subp->warnContextSecondary());
                    }
                }
            }
        }
    }
    శూన్య importSymbolsFromExtended(AstClass* స్థిరరాశి nodep, AstClassExtends* స్థిరరాశి cextp) {
        AstClass* స్థిరరాశి పునాది_Classp = cextp->classp();
        VSymEnt* స్థిరరాశి srcp = m_statep->get_కణుపు_Sym(పునాది_Classp);
        importDerivedClass(nodep, srcp, పునాది_Classp);
        ఒకవేళ (!cextp->isImplements()) m_curSymp->importFromClass(m_statep->symsp(), srcp);
    }
    శూన్య classExtendImport(AstClass* nodep) {
        // A class reference might be to a class that is later in Ast due to
        // e.g. parameterization or referring to a "class (type T) extends T"
        // Resolve it so later Class:: references into its base classes work
        symIterateNull(nodep, m_statep->get_కణుపు_Sym(nodep));
    }

    శూన్య check_Decl_క్రమం(Ast_కణుపు* nodep, Ast_కణుపు* declp) {
        స్థిరరాశి uint32_t decl_Token_అంకె = declp->decl_Token_అంకె();
        ఒకవేళ (!decl_Token_అంకె) తిరిగి;  // Not implemented/valid on this object
        ఒకవేళ (nodep->fileline()->token_అంకె() < decl_Token_అంకె) {
            UINFO(1, "Related node " << nodep->fileline()->token_అంకె() << " " << nodep);
            UINFO(1, "Related decl " << decl_Token_అంకె << " " << declp);
            nodep->v3error("Reference to "
                           << nodep->prettyNameQ() << " before declaration (IEEE 1800-2023 6.18)\n"
                           << nodep->warn_మరిన్ని()
                           << "... Suggest move the declaration before the reference, or use a "
                              "forward typedef\n"
                           << nodep->warnContextPrimary() << '\n'
                           << declp->warnOther() << "... Location of original declaration\n"
                           << declp->warnContextSecondary());
        }
    }
    శూన్య check_Member_Decl_క్రమం(Ast_కణుపు* nodep, AstMemberDType* declp) {
        స్థిరరాశి uint32_t decl_Token_అంకె = declp->fileline()->token_అంకె();
        ఒకవేళ (nodep->fileline()->token_అంకె() < decl_Token_అంకె) {
            UINFO(1, "Related node " << nodep->fileline()->token_అంకె() << " " << nodep);
            UINFO(1, "Related decl " << decl_Token_అంకె << " " << declp);
            nodep->v3error("Reference to "
                           << nodep->prettyNameQ() << " before declaration (IEEE 1800-2023 6.18)\n"
                           << nodep->warn_మరిన్ని()
                           << "... Suggest move the declaration before the reference\n"
                           << nodep->warnContextPrimary() << '\n'
                           << declp->warnOther() << "... Location of original declaration\n"
                           << declp->warnContextSecondary());
        }
    }

    శూన్య replaceWithCheckBreak(Ast_కణుపు* oldp, Ast_కణుపు_DType* newp) {
        // Flag now to avoid V3Broken throwing an internal error
        ఒకవేళ (oldp->wouldBreak(newp)) {
            newp->v3error(
                "Data type used where a non-data type is expected: " << newp->prettyNameQ());
            oldp->replaceWith(కొత్త AstConst{newp->fileline(), AstConst::బిట్టు_False_Erroring{}});
        } లేకపోతే {
            oldp->replaceWith(newp);
        }
    }

    // Marks the current module to be revisited after the initial AST iteration
    శూన్య revisitLater(Ast_కణుపు* deferredNodep) {
        // Need to revisit entire module to build up all the necessary context
        m_చివరి_Deferredp = deferredNodep;
        m_modules_గమ్యం_Revisit.చొప్పించు(std::make_pair(m_modp->name(), m_modp));
    }

    శూన్య updateVarUse(AstVar* nodep) {
        // Avoid dotted.PARAM false positive when in a parameter block
        // that is if ()'ed off by same dotted name as another block
        ఒకవేళ (nodep && nodep->isParam()) nodep->వాడిన_Param(నిజం);
    }

    స్థిర VSymEnt* కనుగొను_Id_Fallback_Skip_Member_DType(VSymEnt* lookp, స్థిరరాశి string& name) {
        VSymEnt* shadowEntp = శూన్యసూచిక;  // Shadowing variable: not a type, kept for error report
        అయితే (lookp) {
            VSymEnt* స్థిరరాశి foundp = lookp->కనుగొను_Id_Flat(name);
            ఒకవేళ (foundp && !VN_IS(foundp->nodep(), MemberDType)) {
                // Non-type entries are not type candidates
                // (IEEE 1800-2023 6.18); skip them so an enclosing
                // type is found, but keep one to preserve the "found: ..." error.
                ఒకవేళ (VN_IS(foundp->nodep(), Typedef) || VN_IS(foundp->nodep(), ParamTypeDType)
                    || VN_IS(foundp->nodep(), Class)) {
                    తిరిగి foundp;
                }
                ఒకవేళ (!shadowEntp) shadowEntp = foundp;
            }
            lookp = lookp->fallbackp();
        }
        తిరిగి shadowEntp;
    }

    శూన్య symIterateChildren(Ast_కణుపు* nodep, VSymEnt* symp) {
        // Iterate children, changing to given context, with restore to old context
        VL_RESTORER_COPY(m_ds);
        VL_RESTORER(m_curSymp);
        m_curSymp = symp;
        m_ds.init(m_curSymp);
        iterateChildren(nodep);
    }
    శూన్య symIterateNull(Ast_కణుపు* nodep, VSymEnt* symp) {
        // Iterate node, changing to given context, with restore to old context
        VL_RESTORER_COPY(m_ds);
        VL_RESTORER(m_curSymp);
        m_curSymp = symp;
        m_ds.init(m_curSymp);
        iterateNull(nodep);
    }
    స్థిర స్థిరరాశి Ast_కణుపు_DType* get_సమాసం_DTypep(స్థిరరాశి Ast_కణుపు_సమాసం* selp) {
        అయితే (స్థిరరాశి Ast_కణుపు_Pre_Sel* స్థిరరాశి sp = VN_CAST(selp, కణుపు_Pre_Sel)) selp = sp->fromp();
        ఒకవేళ (స్థిరరాశి AstMemberSel* స్థిరరాశి sp = VN_CAST(selp, MemberSel)) {
            ఒకవేళ (స్థిరరాశి Ast_కణుపు_DType* dtypep = get_సమాసం_DTypep(sp->fromp())) {
                ఒకవేళ (స్థిరరాశి AstClassRefDType* స్థిరరాశి classRefp = VN_CAST(dtypep, ClassRefDType)) {
                    స్థిరరాశి AstClass* స్థిరరాశి classp = classRefp->classp();
                    స్థిరరాశి తార్కిక దొరికింది = classp->existsMember(
                        [&dtypep, name = selp->name()](స్థిరరాశి AstClass*, స్థిరరాశి AstVar* nodep) {
                            dtypep = nodep->బిడ్డ_DTypep();
                            తిరిగి nodep->name() == name;
                        });
                    ఒకవేళ (దొరికింది) తిరిగి dtypep->elemDTypep();
                    selp->v3error("Class " << classRefp->prettyNameQ()
                                           << " does not contain field " << selp->prettyNameQ());
                } లేకపోతే {
                    selp->v3fatalSrc("Member selection on expression of type "
                                     << dtypep->prettyDTypeNameQ()
                                     << ", which is not a class type");
                }
            }
        } లేకపోతే ఒకవేళ (స్థిరరాశి Ast_కణుపు_Var_Ref* స్థిరరాశి varRefp = VN_CAST(selp, కణుపు_Var_Ref)) {
            తిరిగి varRefp->varp()->బిడ్డ_DTypep()->elemDTypep();
        }
        తిరిగి శూన్యసూచిక;
    }
    స్థిర స్థిరరాశి AstVar* get_తదుపరి_Varp(స్థిరరాశి Ast_కణుపు* stmtsp) {
        // Only IO ports, as parameters are on paramsp(), not the pinsp() list paired here
        అయితే (stmtsp) {
            ఒకవేళ (స్థిరరాశి AstVar* స్థిరరాశి varp = VN_CAST(stmtsp, Var)) {
                ఒకవేళ (varp->isIO()) తిరిగి varp;
            }
            stmtsp = stmtsp->nextp();
        }
        తిరిగి శూన్యసూచిక;
    }
    // Introduce implicit parameters for modules with generic interafeces
    శూన్య కూడు_Implicit_Parameters_యొక్క_Generic_Iface(AstCell* స్థిరరాశి nodep, స్థిరరాశి AstModule* స్థిరరాశి modp) {
        // Get Param number
        పూర్ణాంకం param_అంకె = 1;
        కొరకు (AstPin* paramp = nodep->paramsp(); paramp; paramp = VN_CAST(paramp->nextp(), Pin)) {
            ++param_అంకె;
        }

        // Add each implicit parameter type
        స్థిరరాశి AstVar* modIfaceVarp = get_తదుపరి_Varp(modp->stmtsp());
        కొరకు (స్థిరరాశి AstPin* pinp = nodep->pinsp(); pinp && modIfaceVarp;
             pinp = VN_CAST(pinp->nextp(), Pin),
                           modIfaceVarp = get_తదుపరి_Varp(modIfaceVarp->nextp())) {
            ఒకవేళ (modIfaceVarp->varType() != VVarType::IFACEREF
                || !VN_IS(modIfaceVarp->బిడ్డ_DTypep()->elemDTypep(), IfaceGenericDType)) {
                కొనసాగు;
            }
            Ast_కణుపు* exprp = pinp->exprp();
            ఒకవేళ (!exprp) {
                modIfaceVarp->v3error("Interface port "
                                      << modIfaceVarp->prettyNameQ()
                                      << " is not connected to interface/modport pin expression");
                కొనసాగు;
            }
            అయితే (స్థిరరాశి Ast_కణుపు_Pre_Sel* స్థిరరాశి preSelp = VN_CAST(exprp, కణుపు_Pre_Sel)) {
                exprp = preSelp->fromp();
            }
            // Resolve pin expression to the enclosing module's port Var. At primary
            // LinkDot the expression may still be an AstParseRef, so also look up by name.
            AstVar* enclosingVarp = శూన్యసూచిక;
            స్థిరరాశి AstVarRef* స్థిరరాశి varRefp = VN_CAST(exprp, VarRef);
            ఒకవేళ (varRefp) {
                enclosingVarp = varRefp->varp();
            } లేకపోతే ఒకవేళ (స్థిరరాశి AstParseRef* స్థిరరాశి parseRefp = VN_CAST(exprp, ParseRef)) {
                ఒకవేళ (m_modp) {
                    ఒకవేళ (VSymEnt* స్థిరరాశి symp
                        = m_statep->get_కణుపు_Sym(m_modp)->కనుగొను_Id_Flat(parseRefp->name())) {
                        enclosingVarp = VN_CAST(symp->nodep(), Var);
                    }
                }
            }
            ఒకవేళ (enclosingVarp && enclosingVarp->varType() == VVarType::IFACEREF
                && VN_IS(enclosingVarp->బిడ్డ_DTypep()->skipRefp(), IfaceGenericDType)) {
                // Nested generic-iface forwarding (#7454): enclosing port is itself still
                // generic, so emit a placeholder __VGIfaceParam pin carrying a VarRef to
                // the outer port. V3Param rewrites it to the concrete IfaceRefDType once
                // the enclosing module is specialized (see
                // ParamProcessor::resolveGenericIfaceForwardingPins for the ordering
                // constraint that keeps the rewrite inside V3Param).
                AstVarRef* స్థిరరాశి fwdRefp
                    = కొత్త AstVarRef{exprp->fileline(), enclosingVarp, VAccess::READ};
                AstPin* స్థిరరాశి newPinp = కొత్త AstPin{
                    pinp->fileline(), param_అంకె, "__VGIfaceParam" + modIfaceVarp->name(), fwdRefp};
                newPinp->param(నిజం);
                సందర్శించు(newPinp);
                nodep->కూడు_Paramsp(newPinp);
            } లేకపోతే ఒకవేళ (varRefp) {
                స్థిరరాశి AstVar* స్థిరరాశి varp = varRefp->varp();
                ఒకవేళ (స్థిరరాశి AstIfaceRefDType* స్థిరరాశి refp
                    = VN_CAST(varp->బిడ్డ_DTypep()->elemDTypep(), IfaceRefDType)) {
                    AstIface* స్థిరరాశి ifacep = VN_AS(refp->cellp()->modp(), Iface);
                    AstIfaceRefDType* newIfaceRefp;
                    ఒకవేళ (refp->modportp()) {
                        newIfaceRefp = కొత్త AstIfaceRefDType{
                            refp->fileline(), refp->modportFileline(), m_modp->name(),
                            ifacep->name(), refp->modportName()};
                    } లేకపోతే {
                        newIfaceRefp = కొత్త AstIfaceRefDType{refp->fileline(), m_modp->name(),
                                                            ifacep->name()};
                    }
                    newIfaceRefp->ifacep(ifacep);
                    ఒకవేళ (refp->cellp()->paramsp()) {
                        newIfaceRefp->కూడు_Paramsp(refp->cellp()->paramsp()->clone_వృక్షం(నిజం));
                    }
                    UASSERT_OBJ(pinp->name().కనుగొను("__pinNumber") == 0
                                    || pinp->name() == modIfaceVarp->name(),
                                pinp, "Not found interface with such name");
                    AstPin* స్థిరరాశి newPinp
                        = కొత్త AstPin{pinp->fileline(), param_అంకె,
                                     "__VGIfaceParam" + modIfaceVarp->name(), newIfaceRefp};
                    newPinp->param(నిజం);
                    సందర్శించు(newPinp);
                    nodep->కూడు_Paramsp(newPinp);
                } లేకపోతే {
                    varRefp->v3error("Generic interfaces can only connect to an interface and "
                                     << varp->prettyNameQ() << " is "
                                     << (varp->బిడ్డ_DTypep()
                                             ? "of type " + varp->బిడ్డ_DTypep()->prettyDTypeNameQ()
                                             : "not an interface"));
                }
            } లేకపోతే {
                exprp->v3error("Expected an interface but " << exprp->prettyNameQ()
                                                            << " is not an interface");
            }
            ++param_అంకె;
        }
    }

    // Returns true and issues error iff 'varp' cannot be accesed with a VarXRef
    // in a hier_block given by 'scopeSymp'
    తార్కిక error_Hier_Non_పోర్టు(AstVarXRef* refp, AstVar* varp, VSymEnt* scopeSymp) {
        // OK to access ports on hier_block
        ఒకవేళ (varp->isIO()) తిరిగి అబద్ధం;
        // OK if not an instance
        స్థిరరాశి AstCell* స్థిరరాశి cellp = VN_CAST(scopeSymp->nodep(), Cell);
        ఒకవేళ (!cellp) తిరిగి అబద్ధం;
        // Ok if not a hier_block
        స్థిరరాశి Ast_కణుపు_Module* స్థిరరాశి modp = cellp->modp();
        ఒకవేళ (!modp->hier_ఖండం()) తిరిగి అబద్ధం;
        // Bad
        refp->v3error("Cannot access non-port symbols inside hierarchical block");
        తిరిగి నిజం;
    }

#నిర్వచించు LINKDOT_సందర్శించు_ప్రారంభం() \
    VL_RESTORER(m_indent); \
    ++m_indent;

    string indent() స్థిరరాశి {
        string ఫలితం = "";
        ఫలితం.చొప్పించు(0, m_indent, ':');
        తిరిగి ఫలితం + " ";
    }

    // VISITORS
    శూన్య సందర్శించు(AstNetlist* nodep) override {
        // Recurse..., backward as must do packages before using packages
        iterateChildrenBackwardsConst(nodep);
    }
    శూన్య సందర్శించు(Ast_Type_పట్టిక*) override {}
    శూన్య సందర్శించు(Ast_కణుపు_Module* nodep) override {
        ఒకవేళ (nodep->dead()) తిరిగి;
        LINKDOT_సందర్శించు_ప్రారంభం();
        UINFO(8, indent() << "visit " << nodep);
        VL_RESTORER(m_genericIfaceModule);
        ఒకవేళ (స్థిరరాశి AstModule* స్థిరరాశి modp = VN_CAST(nodep, Module)) {
            m_genericIfaceModule = modp->hasGenericIface();
        }
        checkNoDot(nodep);
        m_ds.init(m_curSymp);
        m_ds.m_dotSymp = m_curSymp = m_modSymp
            = m_statep->get_కణుపు_Sym(nodep);  // Until overridden by a SCOPE
        m_cellp = శూన్యసూచిక;
        m_modp = nodep;
        m_modport_అంకె = 0;
        iterateChildren(nodep);
        m_modp = శూన్యసూచిక;
        m_ds.m_dotSymp = m_curSymp = m_modSymp = శూన్యసూచిక;
        m_ds.m_dotPos = DP_NONE;
        m_ds.m_dotErr = అబద్ధం;
    }
    శూన్య సందర్శించు(AstScope* nodep) override {
        LINKDOT_సందర్శించు_ప్రారంభం();
        UINFO(8, indent() << "visit " << nodep);
        checkNoDot(nodep);
        VL_RESTORER(m_modSymp);
        VL_RESTORER(m_curSymp);
        m_ds.m_dotSymp = m_curSymp = m_modSymp = m_statep->getScopeSym(nodep);
        iterateChildren(nodep);
        m_ds.m_dotSymp = m_curSymp = m_modSymp = శూన్యసూచిక;
    }
    శూన్య సందర్శించు(AstCellInline* nodep) override {
        LINKDOT_సందర్శించు_ప్రారంభం();
        checkNoDot(nodep);
        UASSERT_OBJ(m_statep->forScopeCreation(), nodep, "CellInline should not exist yet");
        VL_DO_DANGLING(నెట్టు_Deletep(nodep->unlink_Fr_వెనుక()), nodep);
    }
    శూన్య సందర్శించు(AstCell* nodep) override {
        // Cell: Recurse inside or cleanup not founds
        LINKDOT_సందర్శించు_ప్రారంభం();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        VL_RESTORER_CLEAR(m_వాడిన_Pins);
        VL_RESTORER_CLEAR(m_వాడిన_Def_Param_Pins);
        UASSERT_OBJ(nodep->modp(), nodep,
                    "Instance has unlinked module");  // V3LinkCell should have errored out
        VL_RESTORER(m_cellp);
        VL_RESTORER(m_pinSymp);
        {
            m_cellp = nodep;
            ఒకవేళ (VN_IS(nodep->modp(), Not_దొరికింది_Module)) {
                // Prevent warnings about missing pin connects
                ఒకవేళ (nodep->pinsp()) nodep->pinsp()->unlink_Fr_వెనుక_With_తదుపరి()->delete_వృక్షం();
                ఒకవేళ (nodep->paramsp()) nodep->paramsp()->unlink_Fr_వెనుక_With_తదుపరి()->delete_వృక్షం();
            }
            // Need to pass the module info to this cell, so we can link up the pin names
            // However can't use m_curSymp as pin connections need to use the
            // instantiator's symbols
            లేకపోతే {
                m_pinSymp = m_statep->get_కణుపు_Sym(nodep->modp());
                UINFO(4, indent() << "(Backto) visit " << nodep);
                // UINFOTREE(1, nodep, "", "linkcell");
                // UINFOTREE(1, nodep->modp(), "", "linkcemd");
                iterateChildren(nodep);

                ఒకవేళ (m_statep->forPrimary())
                    ఒకవేళ (స్థిరరాశి AstModule* స్థిరరాశి modp = VN_CAST(nodep->modp(), Module)) {
                        ఒకవేళ (modp->hasGenericIface())
                            కూడు_Implicit_Parameters_యొక్క_Generic_Iface(nodep, modp);
                    }
            }
        }
        // Parent module inherits child's publicity
        // This is done bottom up in the LinkBotupVisitor stage
    }
    శూన్య సందర్శించు(AstClassRefDType* nodep) override {
        // Cell: Recurse inside or cleanup not founds
        LINKDOT_సందర్శించు_ప్రారంభం();
        UINFO(5, indent() << "visit " << nodep);
        // Can be under dot if called as package::class and that class resolves, so no checkNoDot
        VL_RESTORER_CLEAR(m_వాడిన_Pins);
        VL_RESTORER_CLEAR(m_వాడిన_Def_Param_Pins);
        UASSERT_OBJ(nodep->classp(), nodep, "ClassRef has unlinked class");
        UASSERT_OBJ(m_statep->forPrimary() || !nodep->paramsp() || V3Error::error_లెక్క(), nodep,
                    "class reference parameter not removed by V3Param");
        VL_RESTORER(m_pinSymp);
        {
            // ClassRef's have pins, so track
            m_pinSymp = m_statep->get_కణుపు_Sym(nodep->classp());
            UINFO(4, indent() << "(Backto) visit " << nodep);
            // UINFOTREE(1, nodep, "", "linkcell");
            // UINFOTREE(1, nodep->modp(), "", "linkcemd");
            iterateChildren(nodep);
        }
    }
    శూన్య సందర్శించు(AstPin* nodep) override {
        // Pin: Link to submodule's port
        LINKDOT_సందర్శించు_ప్రారంభం();
        checkNoDot(nodep);
        iterateChildren(nodep);
        ఒకవేళ (!nodep->modVarp()) {
            UASSERT_OBJ(m_pinSymp, nodep, "Pin not under instance?");
            VSymEnt* స్థిరరాశి foundp = m_pinSymp->కనుగొను_Id_Flat(nodep->name());
            స్థిరరాశి అక్షరం* స్థిరరాశి whatp = nodep->param() ? "parameter" : "pin";
            ఒకవేళ (!foundp) {
                ఒకవేళ (m_cellp && VN_IS(m_cellp->modp(), Primitive)
                    && (nodep->name() == "__paramNumber1" || nodep->name() == "__paramNumber2")) {
                    // Primitive parameter is really a delay2 we can just ignore
                    VL_DO_DANGLING(నెట్టు_Deletep(nodep->unlink_Fr_వెనుక()), nodep);
                    తిరిగి;
                } లేకపోతే ఒకవేళ (nodep->param() && !nodep->exprp() && !nodep->svDotName()) {
                    // Placeholder pin the parser makes for an empty '#()', to signal
                    // the user did type the '#()'. No further use, deleting.
                    VL_DO_DANGLING(నెట్టు_Deletep(nodep->unlink_Fr_వెనుక()), nodep);
                    తిరిగి;
                } లేకపోతే {
                    స్థిరరాశి std::string suggest
                        = (nodep->param()
                               ? m_statep->suggestSymFlat(m_pinSymp, nodep->name(),
                                                          Link_కణుపు_Matcher_Var_Param{}, అబద్ధం)
                               : m_statep->suggestSymFlat(m_pinSymp, nodep->name(),
                                                          Link_కణుపు_Matcher_Var_IO{}, అబద్ధం));
                    స్థిరరాశి std::string decl
                        = ((m_cellp && m_cellp->modp())
                               ? "\n"s + nodep->warn_మరిన్ని() + "... Location of instance's "
                                     + m_cellp->modp()->verilogKwd() + " declaration\n"
                                     + m_cellp->modp()->warnContextSecondary()
                               : "");
                    nodep->v3warn(PINNOTFOUND,
                                  ucfirst(whatp)
                                      << " not found: " << nodep->prettyNameQ() << '\n'
                                      << (suggest.ఖాళీలు() ? "" : nodep->warn_మరిన్ని() + suggest)
                                      << '\n'
                                      << nodep->warnContextPrimary() << decl);
                    VL_DO_DANGLING(నెట్టు_Deletep(nodep->unlink_Fr_వెనుక()), nodep);
                    తిరిగి;
                }
            }
            VVarType refVarType = VVarType::UNKNOWN;
            తార్కిక wrongPinType = అబద్ధం;
            ఒకవేళ (AstVar* స్థిరరాశి varp = VN_CAST(foundp->nodep(), Var)) {
                ఒకవేళ (varp->isIO() || varp->isParam() || varp->isIfaceRef()) {
                    refVarType = varp->varType();
                    nodep->modVarp(varp);
                } లేకపోతే {
                    wrongPinType = నిజం;
                }
            } లేకపోతే ఒకవేళ (AstParamTypeDType* స్థిరరాశి typep = VN_CAST(foundp->nodep(), ParamTypeDType)) {
                refVarType = typep->varType();
                nodep->modPTypep(typep);
            } లేకపోతే {
                wrongPinType = నిజం;
            }
            // Don't connect parameter pin to module ports or vice versa
            ఒకవేళ (nodep->param() != (refVarType == VVarType::GPARAM)) wrongPinType = నిజం;
            ఒకవేళ (wrongPinType) {
                string లక్ష్యం_Type = LinkDotState::కణుపు_పాఠ్యం_Type(foundp->nodep());
                లక్ష్యం_Type = VString::aOrAn(లక్ష్యం_Type) + ' ' + లక్ష్యం_Type;
                ఒకవేళ (nodep->param()) {
                    nodep->v3error("Instance attempts to override "
                                   << nodep->prettyNameQ() << " as a " << whatp << ", but it is "
                                   << లక్ష్యం_Type);
                } లేకపోతే {
                    nodep->v3error("Instance attempts to connect to "
                                   << nodep->prettyNameQ() << ", but it is " << లక్ష్యం_Type);
                }
                తిరిగి;
            }
            markAndCheckPinDup(nodep, foundp->nodep(), whatp);
        }
        // Early return() above when deleted
    }

    స్థిర తార్కిక isDotThisReference(స్థిరరాశి AstDot* nodep) {
        తిరిగి VN_IS(nodep->lhsp(), ParseRef) && nodep->lhsp()->name() == "this";
    }

    శూన్య సందర్శించు(AstDot* nodep) override {
        // Legal under a DOT: AstDot, AstParseRef, AstPackageRef, AstNodeSel, AstPackArrayDType
        //    also a DOT can be part of an expression, but only above plus
        //    AstFTaskRef are legal children
        // Dot(PackageRef, ParseRef(text))
        // Dot(Dot(ClassOrPackageRef,ClassOrPackageRef), ParseRef(text))
        // Dot(Dot(Dot(ParseRef(text), ...
        ఒకవేళ (nodep->user3_సమితి_Once()) తిరిగి;
        LINKDOT_సందర్శించు_ప్రారంభం();
        UINFO(8, indent() << "visit " << nodep);
        UINFO(8, indent() << m_ds.ascii());
        స్థిరరాశి Dot_స్థితులు చివరి_స్థితులు = m_ds;
        స్థిరరాశి తార్కిక ప్రారంభం = (m_ds.m_dotPos == DP_NONE);  // Save, as m_dotp will be changed
        స్థిరరాశి DotPosition initialDotPos = m_ds.m_dotPos;
        VL_RESTORER(m_randSymp);
        {
            ఒకవేళ (ప్రారంభం) {  // Starting dot sequence
                UINFOTREE(9, nodep, "", "dot-in");
                // Keep deferral from visit(AstMethodCall) for unspecialized rand target
                m_ds.init(m_curSymp, m_ds.m_unresolvedClass && rand_లక్ష్యం_Unresolved());
            }
            m_ds.m_dotp = nodep;  // Always, not just at start
            m_ds.m_dotPos = DP_మొదటి;

            ఒకవేళ (isDotThisReference(nodep)) {
                VSymEnt* classSymp = getThisClassSymp();
                // In 'randomize() with { this.member }', 'this' refers to randomized
                // object, not the calling class (IEEE 1800-2023 18.7)
                ఒకవేళ (m_randSymp && m_currentWithp) classSymp = m_randSymp;
                ఒకవేళ (!classSymp && rand_లక్ష్యం_Unresolved()) {
                    // Bind 'this' in post-V3Param pass
                } లేకపోతే ఒకవేళ (!classSymp) {
                    nodep->v3error("'this' used outside class (IEEE 1800-2023 8.11)");
                    m_ds.m_dotErr = నిజం;
                } లేకపోతే {
                    m_ds.m_dotSymp = classSymp;
                    UINFO(8, indent() << "this. " << m_ds.ascii());
                }
            } లేకపోతే ఒకవేళ (VN_IS(nodep->lhsp(), ParseRef) && nodep->lhsp()->name() == "super") {
                స్థిరరాశి VSymEnt* classSymp = getThisClassSymp();
                ఒకవేళ (!classSymp) {
                    nodep->v3error("'super' used outside class (IEEE 1800-2023 8.15)");
                    m_ds.m_dotErr = నిజం;
                } లేకపోతే {
                    స్థిరరాశి స్వయంచాలక classp = VN_AS(classSymp->nodep(), Class);
                    స్థిరరాశి స్వయంచాలక cextp = classp->extendsp();
                    ఒకవేళ (!cextp) {
                        nodep->v3error("'super' used on non-extended class (IEEE 1800-2023 8.15)");
                        m_ds.m_dotErr = నిజం;
                    } లేకపోతే {
                        ఒకవేళ (m_statep->forPrimary()
                            && m_extendsParam.కనుగొను(classp) != m_extendsParam.చివర()) {
                            UINFO(9, indent()
                                         << "deferring until post-V3Param: " << nodep->lhsp());
                            m_ds.m_unresolvedClass = నిజం;
                        } లేకపోతే {
                            స్థిరరాశి స్వయంచాలక పునాది_Classp = cextp->classp();
                            UASSERT_OBJ(పునాది_Classp, nodep, "Bad superclass");
                            m_ds.m_dotSymp = m_statep->get_కణుపు_Sym(పునాది_Classp);
                            m_ds.m_super = నిజం;
                            UINFO(8, indent() << "super. " << m_ds.ascii());
                        }
                    }
                }
            } లేకపోతే ఒకవేళ (AstClassOrPackageRef* స్థిరరాశి lhsp
                       = VN_CAST(nodep->lhsp(), ClassOrPackageRef)) {
                // m_ds.m_dotText communicates the cell prefix between stages
                UINFO(8, indent() << "iter.lhs   " << m_ds.ascii() << " " << nodep);
                iterate_And_తదుపరి_Null(lhsp);
                ఒకవేళ (!lhsp->classOrPackageSkipp() && lhsp->name() != "local::") {
                    revisitLater(nodep);
                    m_ds = చివరి_స్థితులు;
                    // Resolve function args before bailing
                    ఒకవేళ (Ast_కణుపు_FTask_Ref* స్థిరరాశి ftaskrefp = VN_CAST(nodep->rhsp(), కణుపు_FTask_Ref)) {
                        iterate_And_తదుపరి_Null(ftaskrefp->argsp());
                    }
                    తిరిగి;
                }
                m_ds.m_dotPos = DP_PACKAGE;
                // nodep->lhsp() may be a new node
                ఒకవేళ (AstClassOrPackageRef* స్థిరరాశి classOrPackageRefp
                    = VN_CAST(nodep->lhsp(), ClassOrPackageRef)) {
                    ఒకవేళ (Ast_కణుపు* స్థిరరాశి classOrPackageNodep
                        = classOrPackageRefp->classOrPackageSkipp()) {
                        m_ds.m_dotSymp = m_statep->get_కణుపు_Sym(classOrPackageNodep);
                    }
                }
                UINFO(8, indent() << "iter.ldone " << m_ds.ascii() << " " << nodep);
            } లేకపోతే ఒకవేళ (VN_IS(nodep->lhsp(), Dot) && VN_AS(nodep->lhsp(), Dot)->colon()) {
                // m_ds.m_dotText communicates the cell prefix between stages
                UINFO(8, indent() << "iter.lhs   " << m_ds.ascii() << " " << nodep);
                m_ds.m_dotPos = DP_PACKAGE;
                iterate_And_తదుపరి_Null(nodep->lhsp());
                // nodep->lhsp() may be a new node
                ఒకవేళ (AstClassOrPackageRef* స్థిరరాశి crefp
                    = VN_CAST(nodep->lhsp(), ClassOrPackageRef)) {
                    ఒకవేళ (!crefp->classOrPackageSkipp()) {
                        revisitLater(nodep);
                        m_ds = చివరి_స్థితులు;
                        // Resolve function args before bailing
                        ఒకవేళ (Ast_కణుపు_FTask_Ref* స్థిరరాశి ftaskrefp
                            = VN_CAST(nodep->rhsp(), కణుపు_FTask_Ref)) {
                            iterate_And_తదుపరి_Null(ftaskrefp->argsp());
                        }
                        తిరిగి;
                    }
                }
                ఒకవేళ (m_చివరి_Deferredp == nodep->lhsp()) {
                    // LHS got deferred, so this node won't be resolved. Defer it too
                    m_ds = చివరి_స్థితులు;
                    // Resolve function args before bailing
                    ఒకవేళ (Ast_కణుపు_FTask_Ref* స్థిరరాశి ftaskrefp = VN_CAST(nodep->rhsp(), కణుపు_FTask_Ref)) {
                        iterate_And_తదుపరి_Null(ftaskrefp->argsp());
                    }
                    తిరిగి;
                }
                UINFO(8, indent() << "iter.ldone " << m_ds.ascii() << " " << nodep);
            } లేకపోతే {
                m_ds.m_dotPos = DP_మొదటి;
                m_ds.m_unresolvedGenericIface = అబద్ధం;
                UINFO(8, indent() << "iter.lhs   " << m_ds.ascii() << " " << nodep);
                iterate_And_తదుపరి_Null(nodep->lhsp());
                UINFO(8, indent() << "iter.ldone " << m_ds.ascii() << " " << nodep);
                ఒకవేళ (m_ds.m_unresolvedGenericIface) {
                    m_ds.m_dotPos = initialDotPos;
                    తిరిగి;
                }
                // UINFOTREE(9, nodep, "", "dot-lho");
            }
            ఒకవేళ (m_statep->forPrimary() && isParamedClassRef(nodep->lhsp())) {
                // Dots of paramed classes will be linked after deparameterization
                UINFO(9, indent() << "deferring until post-V3Param: " << nodep->lhsp());
                m_ds.m_unresolvedClass = నిజం;
            }
            ఒకవేళ (m_ds.m_unresolvedCell
                && (VN_IS(nodep->lhsp(), CellRef) || VN_IS(nodep->lhsp(), CellArrayRef))) {
                m_ds.m_unlinkedScopep = nodep->lhsp();
            }
            ఒకవేళ (m_ds.m_dotPos == DP_మొదటి) m_ds.m_dotPos = DP_SCOPE;
            ఒకవేళ (!m_ds.m_dotErr) {  // Once something wrong, give up
                // Top 'final' dot RHS is final RHS, else it's a
                // DOT(DOT(x,*here*),real-rhs) which we consider a RHS
                ఒకవేళ (ప్రారంభం && m_ds.m_dotPos == DP_SCOPE) m_ds.m_dotPos = DP_FINAL;
                UINFO(8, indent() << "iter.rhs   " << m_ds.ascii() << " " << nodep);
                // m_ds.m_dotSymp points at lhsp()'s symbol table, so resolve RHS under that
                iterate_And_తదుపరి_Null(nodep->rhsp());
                UINFO(8, indent() << "iter.rdone " << m_ds.ascii() << " " << nodep);
                // UINFOTREE(9, nodep, "", "dot-rho");
            }
            ఒకవేళ (!m_ds.m_unresolvedClass) {
                ఒకవేళ (ప్రారంభం) {
                    Ast_కణుపు* newp;
                    ఒకవేళ (m_ds.m_dotErr) {
                        newp = కొత్త AstConst{nodep->fileline(), AstConst::బిట్టు_False{}};
                    } లేకపోతే {
                        newp = nodep->rhsp()->unlink_Fr_వెనుక();
                    }
                    UINFOTREE(9, newp, "", "dot-out");
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
                } లేకపోతే {  // Dot midpoint
                    Ast_కణుపు* newp = nodep->rhsp()->unlink_Fr_వెనుక();
                    ఒకవేళ (m_ds.m_unresolvedCell) {
                        AstCellRef* స్థిరరాశి crp = కొత్త AstCellRef{
                            nodep->fileline(), nodep->name(), nodep->lhsp()->unlink_Fr_వెనుక(), newp};
                        newp = crp;
                    }
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
                }
            }
        }
        ఒకవేళ (ప్రారంభం) {
            m_ds = చివరి_స్థితులు;
        } లేకపోతే {
            స్థిరరాశి తార్కిక unresolvedClass = m_ds.m_unresolvedClass;
            m_ds.m_dotp = చివరి_స్థితులు.m_dotp;
            m_ds.m_unresolvedClass |= unresolvedClass;
        }
        UINFO(8, indent() << "done " << m_ds.ascii() << " " << nodep);
    }
    శూన్య సందర్శించు(AstDefaultClocking* nodep) override {
        ఒకవేళ (VSymEnt* స్థిరరాశి foundp = m_curSymp->కనుగొను_Id_Fallback(nodep->name())) {
            ఒకవేళ (AstClocking* స్థిరరాశి clockingp = VN_CAST(foundp->nodep(), Clocking)) {
                clockingp->makeDefault();
                VL_DO_DANGLING(nodep->unlink_Fr_వెనుక()->delete_వృక్షం(), nodep);
            } లేకపోతే {
                nodep->v3error(nodep->prettyNameQ() << " is not a clocking identifier");
            }
        } లేకపోతే {
            nodep->v3error("Can't find definition of clocking: " << nodep->prettyNameQ());
        }
    }
    శూన్య సందర్శించు(Ast_Sen_అంశం* nodep) override {
        LINKDOT_సందర్శించు_ప్రారంభం();
        VL_RESTORER(m_లోపల_Sens);
        m_లోపల_Sens = నిజం;
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(AstPatMember* nodep) override {
        LINKDOT_సందర్శించు_ప్రారంభం();
        ఒకవేళ (nodep->varrefp()) తిరిగి;  // only do this mapping once
        // If we have a TEXT token as our key, lookup if it's a LPARAM
        ఒకవేళ (Ast_పాఠ్యం* స్థిరరాశి textp = VN_CAST(nodep->keyp(), పాఠ్యం_2)) {
            UINFO(9, indent() << "visit " << nodep);
            UINFO(9, indent() << "      " << textp);
            // Lookup
            ఒకవేళ (VSymEnt* స్థిరరాశి foundp = m_curSymp->కనుగొను_Id_Fallback(textp->పాఠ్యం())) {
                ఒకవేళ (AstVar* స్థిరరాశి varp = VN_CAST(foundp->nodep(), Var)) {
                    ఒకవేళ (varp->isParam() || varp->isGenVar()) {
                        // Attach found Text reference to PatMember
                        nodep->varrefp(
                            కొత్త AstVarRef{nodep->fileline(),
                                          foundp->imported() ? foundp->classOrPackagep() : శూన్యసూచిక,
                                          varp, VAccess::READ});
                        UINFO(9, indent() << " new " << nodep->varrefp());
                    }
                }
                ఒకవేళ (Ast_Enum_అంశం* స్థిరరాశి itemp = VN_CAST(foundp->nodep(), Enum_అంశం)) {
                    // Attach enum item value to PatMember
                    nodep->varrefp(
                        కొత్త Ast_Enum_అంశం_Ref{nodep->fileline(), itemp, foundp->classOrPackagep()});
                    UINFO(9, indent() << " new " << itemp);
                }
            }
        }
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(AstParseRef* nodep) override {
        ఒకవేళ (nodep->user3_సమితి_Once()) తిరిగి;
        LINKDOT_సందర్శించు_ప్రారంభం();
        UINFO(9, indent() << "visit " << nodep);
        UINFO(9, indent() << m_ds.ascii());
        ఒకవేళ (m_ds.m_unresolvedClass) తిరిగి;
        // m_curSymp is symbol table of outer expression
        // m_ds.m_dotSymp is symbol table relative to "."'s above now
        UASSERT_OBJ(m_ds.m_dotSymp, nodep, "nullptr lookup symbol table");
        // Generally resolved during Primary, but might be at param time under AstUnlinkedRef
        UASSERT_OBJ(!m_statep->forScopeCreation(), nodep, "ParseRefs should no longer exist");
        స్థిరరాశి Dot_స్థితులు చివరి_స్థితులు = m_ds;
        స్థిరరాశి తార్కిక ప్రారంభం = (m_ds.m_dotPos == DP_NONE);  // Save, as m_dotp will be changed
        తార్కిక మొదటి = ప్రారంభం || m_ds.m_dotPos == DP_మొదటి;

        ఒకవేళ (ప్రారంభం) {
            m_ds.init(m_curSymp);
            // Note m_ds.m_dotp remains nullptr; this is a reference not under a dot
        }
        ఒకవేళ (nodep->name() == "super") {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: super");
            m_ds.m_dotErr = నిజం;
        }
        ఒకవేళ (nodep->name() == "this") {
            iterateChildren(nodep);
            ఒకవేళ (m_statep->forPrimary()) తిరిగి;  // The class might be parameterized somewhere
            స్థిరరాశి VSymEnt* classSymp = getThisClassSymp();
            ఒకవేళ (!classSymp) {
                nodep->v3error("'this' used outside class (IEEE 1800-2023 8.11)");
                తిరిగి;
            }
            AstClass* స్థిరరాశి classp = VN_AS(classSymp->nodep(), Class);
            AstClassRefDType* స్థిరరాశి dtypep
                = కొత్త AstClassRefDType{nodep->fileline(), classp, శూన్యసూచిక};
            AstThisRef* స్థిరరాశి newp = కొత్త AstThisRef{nodep->fileline(), VFlag_బిడ్డ_DType{}, dtypep};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
            తిరిగి;
        }
        ఒకవేళ (m_ds.m_dotPos == DP_MEMBER && VN_IS(m_ds.m_dotp->lhsp(), LambdaArgRef)
            && nodep->name() == "index") {
            // 'with' statement's 'item.index'
            // m_ds.dotp->lhsp() was checked to know if `index` is directly after lambda arg ref.
            // If not, treat it as normal member select
            iterateChildren(nodep);
            AstLambdaArgRef* స్థిరరాశి newp = కొత్త AstLambdaArgRef{
                nodep->fileline(), m_ds.m_dotp->lhsp()->name() + "__DOT__index", నిజం};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
            తిరిగి;
        } లేకపోతే ఒకవేళ (m_ds.m_dotPos == DP_MEMBER) {
            // Found a Var, everything following is membership.  {scope}.{var}.HERE {member}
            ఒకవేళ (m_ds.m_dotErr) {
                nodep->unlink_Fr_వెనుక();  // Avoid circular node loop on errors
            } లేకపోతే {
                Ast_కణుపు_సమాసం* స్థిరరాశి varEtcp = VN_AS(m_ds.m_dotp->lhsp()->unlink_Fr_వెనుక(), కణుపు_సమాసం);
                Ast_కణుపు_సమాసం* స్థిరరాశి newp = కొత్త AstMemberSel{nodep->fileline(), varEtcp,  //
                                                           VFlag_బిడ్డ_DType{}, nodep->name()};
                nodep->replaceWith(newp);
            }
            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
        } లేకపోతే {
            //
            string expectWhat;
            తార్కిక allowScope = అబద్ధం;
            తార్కిక allowVar = అబద్ధం;
            తార్కిక allowFTask = అబద్ధం;
            తార్కిక staticAccess = అబద్ధం;
            ఒకవేళ (m_ds.m_disablep) {
                allowScope = నిజం;
                allowFTask = నిజం;
                expectWhat = "block/task";
            } లేకపోతే ఒకవేళ (m_ds.m_dotPos == DP_PACKAGE) {
                // {package-or-class}::{a}
                Ast_కణుపు_Module* classOrPackagep = శూన్యసూచిక;
                expectWhat = "scope/variable/func";
                allowScope = నిజం;
                allowVar = నిజం;
                allowFTask = నిజం;
                staticAccess = నిజం;
                UINFO(9, "chk pkg " << m_ds.ascii() << " lhsp=" << m_ds.m_dotp->lhsp());
                UASSERT_OBJ(VN_IS(m_ds.m_dotp->lhsp(), ClassOrPackageRef), m_ds.m_dotp->lhsp(),
                            "Bad package link");
                AstClassOrPackageRef* స్థిరరాశి cpackagerefp
                    = VN_AS(m_ds.m_dotp->lhsp(), ClassOrPackageRef);
                ఒకవేళ (cpackagerefp->name() == "local::") {
                    m_randSymp = శూన్యసూచిక;
                    మొదటి = నిజం;
                } లేకపోతే ఒకవేళ (!cpackagerefp->classOrPackageSkipp()) {
                    VSymEnt* స్థిరరాశి foundp = m_statep->resolveClassOrPackage(
                        m_ds.m_dotSymp, cpackagerefp, నిజం, అబద్ధం, ":: reference");
                    ఒకవేళ (!foundp) తిరిగి;
                    classOrPackagep = cpackagerefp->classOrPackageSkipp();
                    ఒకవేళ (classOrPackagep) {
                        m_ds.m_dotSymp = m_statep->get_కణుపు_Sym(classOrPackagep);
                    } లేకపోతే {
                        m_ds = చివరి_స్థితులు;
                        తిరిగి;
                    }
                } లేకపోతే {
                    classOrPackagep = cpackagerefp->classOrPackageSkipp();
                    UASSERT_OBJ(classOrPackagep, m_ds.m_dotp->lhsp(), "Bad package link");
                    m_ds.m_dotSymp = m_statep->get_కణుపు_Sym(classOrPackagep);
                }
                m_ds.m_dotPos = DP_SCOPE;
            } లేకపోతే ఒకవేళ (m_ds.m_dotPos == DP_SCOPE || m_ds.m_dotPos == DP_మొదటి) {
                // {a}.{b}, where {a} maybe a module name
                // or variable, where dotting into structure member
                expectWhat = "scope/variable";
                allowScope = నిజం;
                allowVar = నిజం;
            } లేకపోతే ఒకవేళ (m_ds.m_dotPos == DP_NONE) {
                expectWhat = "variable";
                allowVar = నిజం;
            } లేకపోతే ఒకవేళ (m_ds.m_dotPos == DP_FINAL) {
                expectWhat = "variable/method";
                allowFTask = నిజం;
                allowVar = నిజం;
            } లేకపోతే {
                UINFO(1, "ds=" << m_ds.ascii());
                nodep->v3fatalSrc("Unhandled DotPosition");
            }
            // Lookup
            VSymEnt* foundp;
            string baddot;
            VSymEnt* okSymp = శూన్యసూచిక;
            // Restricted 'with (id_list)': only id_list names bind into target class.
            ఒకవేళ (m_randSymp
                && (!m_currentWithp || m_currentWithp->name_Resolves_గమ్యం_లక్ష్యం(nodep->name()))) {
                foundp = m_randSymp->కనుగొను_Id_Flat(nodep->name());
                ఒకవేళ (foundp) {
                    ఒకవేళ (m_currentWithp && m_currentWithp->restricted()) {
                        m_restricted_పేర్లు_వాడిన.చొప్పించు(nodep->name());
                    }
                    ఒకవేళ (!ప్రారంభం) m_ds.m_dotPos = DP_MEMBER;
                    ఒకవేళ (!m_currentWithp) {
                        UASSERT_OBJ(m_rand_పద్ధతి_Callp, nodep, "Expected to be under randomize()");
                        // This will start failing once complex expressions are allowed on the LHS
                        // of randomize() with args
                        UASSERT_OBJ(VN_IS(m_rand_పద్ధతి_Callp->fromp(), VarRef), m_rand_పద్ధతి_Callp,
                                    "Expected simple randomize target");
                        // A ParseRef is used here so that the dot RHS gets resolved
                        AstMemberSel* స్థిరరాశి newp = కొత్త AstMemberSel{
                            nodep->fileline(), m_rand_పద్ధతి_Callp->fromp()->clone_వృక్షం(అబద్ధం),
                            VFlag_బిడ్డ_DType{}, nodep->name()};
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
                        తిరిగి;
                    }
                    UINFO(9, indent() << "randomize-with fromSym " << foundp->nodep());
                    AstLambdaArgRef* స్థిరరాశి lambdaRefp
                        = కొత్త AstLambdaArgRef{nodep->fileline(), "__Vrandwith_obj", అబద్ధం};
                    AstMemberSel* newp = కొత్త AstMemberSel{nodep->fileline(), lambdaRefp,
                                                          VFlag_బిడ్డ_DType{}, nodep->name()};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
                    తిరిగి;
                }
            }
            ఒకవేళ (allowScope) {
                foundp = m_statep->కనుగొను_Dotted(nodep->fileline(), m_ds.m_dotSymp, nodep->name(),
                                              baddot, okSymp, మొదటి);  // Maybe nullptr
            } లేకపోతే ఒకవేళ (మొదటి) {
                foundp = m_ds.m_dotSymp->కనుగొను_Id_Fallback(nodep->name());
            } లేకపోతే {
                foundp = m_ds.m_dotSymp->కనుగొను_Id_Flat(nodep->name());
            }
            ఒకవేళ (!foundp && m_ds.m_dotp && isDotThisReference(m_ds.m_dotp)) {
                స్థిరరాశి AstClass* స్థిరరాశి classp = VN_CAST(m_ds.m_dotSymp->nodep(), Class);
                ఒకవేళ (classp && classp->isCovergroup() && classp->covergroupEnclosingClassp()) {
                    VSymEnt* స్థిరరాశి enclosingClassSymp
                        = m_statep->get_కణుపు_Sym(classp->covergroupEnclosingClassp());
                    foundp = enclosingClassSymp->కనుగొను_Id_Fallback(nodep->name());
                    ఒకవేళ (foundp) m_ds.m_dotSymp = enclosingClassSymp;
                }
            }
            // If not found in modport, check interface fallback for parameters and typedefs.
            // Parameters and typedefs are always visible through a modport (IEEE 1800-2023 25.5).
            // This mirrors the VarXRef modport parameter fallback in visit(AstVarXRef).
            ఒకవేళ (!foundp && VN_IS(m_ds.m_dotSymp->nodep(), Modport)
                && m_ds.m_dotSymp->fallbackp()) {
                VSymEnt* స్థిరరాశి ifaceFallbackp = m_ds.m_dotSymp->fallbackp();
                VSymEnt* స్థిరరాశి ifaceFoundp = ifaceFallbackp->కనుగొను_Id_Flat(nodep->name());
                ఒకవేళ (ifaceFoundp) {
                    ఒకవేళ (స్థిరరాశి AstVar* స్థిరరాశి varp = VN_CAST(ifaceFoundp->nodep(), Var)) {
                        ఒకవేళ (varp->isParam()) foundp = ifaceFoundp;
                    } లేకపోతే ఒకవేళ (VN_IS(ifaceFoundp->nodep(), Typedef)
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
            ఒకవేళ (!foundp && !okSymp && VN_IS(m_ds.m_dotSymp->nodep(), Modport)) {
                okSymp = m_ds.m_dotSymp;
            }
            ఒకవేళ (foundp) {
                UINFO(9, indent() << "found=se" << cvt_గమ్యం_Hex(foundp) << "  exp=" << expectWhat
                                  << "  n=" << foundp->nodep());
            }
            // What fell out?
            తార్కిక ok = అబద్ధం;
            // Special case: waiting on clocking event
            ఒకవేళ (m_లోపల_Sens && foundp && m_ds.m_dotPos != DP_SCOPE && m_ds.m_dotPos != DP_మొదటి) {
                ఒకవేళ (AstClocking* స్థిరరాశి clockingp = sensClockingp(foundp->nodep())) {
                    foundp = getCreateClockingEventSymEnt(clockingp);
                }
            }
            ఒకవేళ (!foundp) {
            } లేకపోతే ఒకవేళ (VN_IS(foundp->nodep(), Cell) || VN_IS(foundp->nodep(), కణుపు_ఖండం)
                       || VN_IS(foundp->nodep(), Gen_ఖండం)
                       || VN_IS(foundp->nodep(), Netlist)  // for $root
                       || VN_IS(foundp->nodep(), Module)) {  // if top
                ఒకవేళ (allowScope) {
                    ok = నిజం;
                    m_ds.m_dot_పాఠ్యం = VString::dot(m_ds.m_dot_పాఠ్యం, ".", nodep->name());
                    m_ds.m_dotSymp = foundp;
                    m_ds.m_dotPos = DP_SCOPE;
                    ఒకవేళ (m_ds.m_disablep && VN_IS(foundp->nodep(), కణుపు_ఖండం)) {
                        // Possibly it is not the final link. If we are under dot and not in its
                        // last component, `targetp()` field will be overwritten by next components
                        m_ds.m_disablep->targetp(foundp->nodep());
                    }
                    ఒకవేళ (VN_IS(foundp->nodep(), Gen_ఖండం)) {
                        m_ds.m_genBlk = నిజం;
                        ఒకవేళ (m_ds.m_disablep) {
                            m_ds.m_disablep->v3warn(
                                E_UNSUPPORTED,
                                "Unsupported: Generate block referenced by disable");
                        }
                    }
                    // Upper AstDot visitor will handle it from here
                } లేకపోతే ఒకవేళ (VN_IS(foundp->nodep(), Cell) && allowVar) {
                    AstCell* స్థిరరాశి cellp = VN_AS(foundp->nodep(), Cell);
                    ఒకవేళ (VN_IS(cellp->modp(), Iface)) {
                        // Interfaces can be referenced like a variable for interconnect
                        VSymEnt* స్థిరరాశి cellEntp = m_statep->get_కణుపు_Sym(cellp);
                        UASSERT_OBJ(cellEntp, nodep, "No interface sym entry");
                        VSymEnt* స్థిరరాశి మాతృ_Entp
                            = cellEntp->parentp();  // Container of the var; probably a module or
                                                    // generate begin
                        AstVar* స్థిరరాశి ifaceRefVarp
                            = కనుగొను_Iface_శిఖరం_Varp(nodep, మాతృ_Entp, nodep->name());
                        //
                        ok = నిజం;
                        m_ds.m_dotSymp = foundp;
                        m_ds.m_dotPos = DP_SCOPE;
                        UINFO(9, indent() << " cell -> iface varref " << foundp->nodep());
                        Ast_కణుపు_సమాసం* newp;
                        ఒకవేళ (m_ds.m_dot_పాఠ్యం != "") {
                            newp = కొత్త AstVarXRef{nodep->fileline(), ifaceRefVarp, m_ds.m_dot_పాఠ్యం,
                                                  VAccess::READ};
                            ఒకవేళ (m_ds.m_unresolvedCell && m_ds.m_unlinkedScopep) {
                                newp = కొత్త AstUnlinkedRef{nodep->fileline(), newp, newp->name(),
                                                          m_ds.m_unlinkedScopep->unlink_Fr_వెనుక()};
                                m_ds.m_unlinkedScopep = శూన్యసూచిక;
                                m_ds.m_unresolvedCell = అబద్ధం;
                                m_ds.m_dot_పాఠ్యం = "";
                            }
                        } లేకపోతే {
                            AstVarRef* స్థిరరాశి refp
                                = కొత్త AstVarRef{nodep->fileline(), ifaceRefVarp, VAccess::READ};
                            newp = refp;
                        }
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
                    } లేకపోతే ఒకవేళ (VN_IS(cellp->modp(), Not_దొరికింది_Module)) {
                        cellp->modNameFileline()->v3error("Cannot find file containing interface: "
                                                          << cellp->modp()->prettyNameQ());
                    }
                }
            } లేకపోతే ఒకవేళ (allowScope && !allowFTask && VN_IS(foundp->nodep(), కణుపు_FTask)) {
                // Function/task used as scope for static variable access (IEEE std 1800-2017 6.21)
                // Don't set m_dotText so the static variable resolves as a VarRef rather than
                // VarXRef. This is so that V3Begin won't move it out of the function/task's scope.
                ok = నిజం;
                m_ds.m_dotSymp = foundp;
                m_ds.m_dotPos = DP_SCOPE;
            } లేకపోతే ఒకవేళ (allowFTask && VN_IS(foundp->nodep(), కణుపు_FTask)) {
                Ast_కణుపు_FTask_Ref* taskrefp;
                ఒకవేళ (VN_IS(foundp->nodep(), Task)) {
                    taskrefp = కొత్త AstTaskRef{nodep->fileline(), nodep->name()};
                } లేకపోతే {
                    taskrefp = కొత్త AstFuncRef{nodep->fileline(), nodep->name()};
                }
                nodep->replaceWith(taskrefp);
                VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
                m_ds = చివరి_స్థితులు;
                తిరిగి;
            } లేకపోతే ఒకవేళ (AstVar* స్థిరరాశి varp = దొరికింది_గమ్యం_Varp(foundp, nodep, VAccess::READ)) {
                // Preserve explicit 'this' references to members inherited from
                // generic classes until after specialization.
                ఒకవేళ (m_statep->forPrimary() && m_ds.m_dotp && isDotThisReference(m_ds.m_dotp)
                    && foundp->classOrPackagep() != m_modp
                    && VN_IS(foundp->classOrPackagep(), Class)
                    && foundp->classOrPackagep()->hasGParam()) {
                    m_ds.m_unresolvedClass = నిజం;
                    తిరిగి;
                }
                AstIfaceRefDType* స్థిరరాశి ifacerefp
                    = LinkDotState::ifaceRefFromArray(varp->subDTypep());
                ఒకవేళ (varp->isIfaceRef() && m_genericIfaceModule
                    && VN_IS(varp->బిడ్డ_DTypep(), IfaceGenericDType) && !ప్రారంభం) {
                    // Defer only dotted member access ('d.PARAM'), as V3Param must specialize
                    // first; a standalone ref ('.x(d)') resolves via allowVar below now
                    ok = నిజం;
                    m_ds.m_unresolvedGenericIface = నిజం;
                } లేకపోతే ఒకవేళ (ifacerefp && varp->isIfaceRef()) {
                    UASSERT_OBJ(ifacerefp->iface_ద్వారా_Cellp(), ifacerefp, "Unlinked interface");
                    // Really this is a scope reference into an interface
                    UINFO(9, indent() << "varref-ifaceref " << m_ds.m_dot_పాఠ్యం << "  " << nodep);
                    m_ds.m_dot_పాఠ్యం = VString::dot(m_ds.m_dot_పాఠ్యం, ".", nodep->name());
                    // If modport specified, use modport symbol table for lookup of virtual ports
                    ఒకవేళ (ifacerefp->modportp() && m_statep->exists_కణుపు_Sym(ifacerefp->modportp())) {
                        m_ds.m_dotSymp = m_statep->get_కణుపు_Sym(ifacerefp->modportp());
                    } లేకపోతే {
                        m_ds.m_dotSymp = m_statep->get_కణుపు_Sym(ifacerefp->iface_ద్వారా_Cellp());
                    }
                    m_ds.m_dotPos = DP_SCOPE;
                    ok = నిజం;

                    AstVarRef* స్థిరరాశి refp = కొత్త AstVarRef{nodep->fileline(), varp, VAccess::READ};
                    nodep->replaceWith(refp);

                    VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
                } లేకపోతే ఒకవేళ (allowVar) {
                    Ast_కణుపు* newp;
                    ఒకవేళ (m_ds.m_dot_పాఠ్యం != "") {
                        AstVarXRef* స్థిరరాశి refp
                            = కొత్త AstVarXRef{nodep->fileline(), nodep->name(), m_ds.m_dot_పాఠ్యం,
                                             VAccess::READ};  // lvalue'ness computed later
                        refp->varp(varp);
                        // Not linked again after V3LinkLValue, so that reports writing it
                        ఒకవేళ (స్థిరరాశి AstModportVarRef* స్థిరరాశి mvarp
                            = VN_CAST(foundp->nodep(), ModportVarRef)) {
                            ఒకవేళ (m_statep->forParamed() && mvarp->direction().isReadOnly()) {
                                refp->readOnlyModport(నిజం);
                            }
                        }
                        refp->కలిగియుంది_Gen_ఖండం(m_ds.m_genBlk);
                        m_ds.m_dot_పాఠ్యం = "";
                        ఒకవేళ (m_ds.m_unresolvedCell && m_ds.m_unlinkedScopep) {
                            స్థిరరాశి string dotted = refp->dotted();
                            స్థిరరాశి కొలత_t pos = dotted.కనుగొను("__BRA__??__KET__");
                            // Arrays of interfaces all have the same parameters
                            ఒకవేళ (pos != string::npos && varp->isParam()
                                && VN_IS(m_ds.m_unlinkedScopep, CellArrayRef)) {
                                refp->dotted(dotted.substr(0, pos));
                                newp = refp;
                            } లేకపోతే {
                                UINFO(9, indent() << "deferring until post-V3Param: " << refp);
                                newp = కొత్త AstUnlinkedRef{nodep->fileline(), refp, refp->name(),
                                                          m_ds.m_unlinkedScopep->unlink_Fr_వెనుక()};
                                m_ds.m_unlinkedScopep = శూన్యసూచిక;
                                m_ds.m_unresolvedCell = అబద్ధం;
                            }
                        } లేకపోతే {
                            newp = refp;
                        }
                    } లేకపోతే {
                        ఒకవేళ (staticAccess && !varp->lifetime().isStatic() && !varp->isParam()) {
                            // TODO bug4077
                            // nodep->v3error("Static access to non-static member variable "
                            //                << varp->prettyNameQ() << endl);
                        }
                        AstVarRef* స్థిరరాశి refp = కొత్త AstVarRef{
                            nodep->fileline(), varp, VAccess::READ};  // lvalue'ness computed later
                        refp->classOrPackagep(foundp->classOrPackagep());
                        newp = refp;
                    }
                    UINFO(9, indent() << "new " << newp);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
                    m_ds.m_dotPos = DP_MEMBER;
                    ok = నిజం;
                }
            } లేకపోతే ఒకవేళ (స్థిరరాశి AstModport* స్థిరరాశి modportp = VN_CAST(foundp->nodep(), Modport)) {
                // A scope reference into an interface's modport (not
                // necessarily at a pin connection)
                UINFO(9, indent() << "cell-ref-to-modport " << m_ds.m_dot_పాఠ్యం << "  " << nodep);
                UINFO(9, indent() << "unlinked " << m_ds.m_unlinkedScopep);
                UINFO(9,
                      indent() << "dotSymp " << m_ds.m_dotSymp << " " << m_ds.m_dotSymp->nodep());
                // Iface was the previously dotted component
                ఒకవేళ (!m_ds.m_dotSymp || !VN_IS(m_ds.m_dotSymp->nodep(), Cell)
                    || !VN_AS(m_ds.m_dotSymp->nodep(), Cell)->modp()
                    || !VN_IS(VN_AS(m_ds.m_dotSymp->nodep(), Cell)->modp(), Iface)) {
                    nodep->v3error("Modport not referenced as <interface>."
                                   << modportp->prettyNameQ());
                } లేకపోతే ఒకవేళ (!VN_AS(m_ds.m_dotSymp->nodep(), Cell)->modp()
                           || !VN_IS(VN_AS(m_ds.m_dotSymp->nodep(), Cell)->modp(), Iface)) {
                    nodep->v3error("Modport not referenced from underneath an interface: "
                                   << modportp->prettyNameQ());
                } లేకపోతే {
                    AstCell* స్థిరరాశి cellp = VN_AS(m_ds.m_dotSymp->nodep(), Cell);
                    UASSERT_OBJ(cellp, nodep, "Modport not referenced from an instance");
                    VSymEnt* స్థిరరాశి cellEntp = m_statep->get_కణుపు_Sym(cellp);
                    UASSERT_OBJ(cellEntp, nodep, "No interface sym entry");
                    VSymEnt* స్థిరరాశి మాతృ_Entp
                        = cellEntp->parentp();  // Container of the var; probably a
                                                // module or generate begin
                    // We drop __BRA__??__KET__ as cells don't have that naming yet
                    AstVar* స్థిరరాశి ifaceRefVarp
                        = కనుగొను_Iface_శిఖరం_Varp(nodep, మాతృ_Entp, cellp->name());
                    //
                    ok = నిజం;
                    m_ds.m_dot_పాఠ్యం = VString::dot(m_ds.m_dot_పాఠ్యం, ".", nodep->name());
                    m_ds.m_dotSymp = foundp;
                    m_ds.m_dotPos = DP_SCOPE;
                    UINFO(9, indent() << "modport -> iface varref " << foundp->nodep());
                    // We lose the modport name here, so we cannot detect mismatched modports.
                    AstVarRef* స్థిరరాశి refp
                        = కొత్త AstVarRef{nodep->fileline(), ifaceRefVarp, VAccess::READ};
                    Ast_కణుపు_సమాసం* newp = refp;

                    స్వయంచాలక* స్థిరరాశి cellarrayrefp = VN_CAST(m_ds.m_unlinkedScopep, CellArrayRef);
                    ఒకవేళ (cellarrayrefp) {
                        // iface[vec].modport became CellArrayRef(iface, lsb)
                        // Convert back to SelBit(iface, lsb)
                        UINFO(9, indent() << "Array modport to SelBit " << cellarrayrefp);
                        newp = కొత్త Ast_Sel_బిట్టు{cellarrayrefp->fileline(), newp,
                                             cellarrayrefp->selp()->unlink_Fr_వెనుక()};
                        newp->user3(నిజం);  // Don't process again
                        VL_DO_DANGLING(నెట్టు_Deletep(cellarrayrefp->unlink_Fr_వెనుక()), cellarrayrefp);
                        m_ds.m_unlinkedScopep = శూన్యసూచిక;
                    }
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
                }
            } లేకపోతే ఒకవేళ (AstConstraint* స్థిరరాశి defp = VN_CAST(foundp->nodep(), Constraint)) {
                Ast_కణుపు* స్థిరరాశి newp = కొత్త AstConstraintRef{nodep->fileline(), శూన్యసూచిక, defp};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
                ok = నిజం;
                m_ds.m_dotPos = DP_MEMBER;
                m_ds.m_dot_పాఠ్యం = "";
            } లేకపోతే ఒకవేళ (AstClass* స్థిరరాశి defp = VN_CAST(foundp->nodep(), Class)) {
                ఒకవేళ (allowVar) {
                    ఒకవేళ (m_ds.m_dotPos == DP_SCOPE && !staticAccess) {
                        // Continue hierarchical lookup in the class scope.
                        m_ds.m_dotSymp = foundp;
                    } లేకపోతే {
                        AstRefDType* స్థిరరాశి newp
                            = కొత్త AstRefDType{nodep->fileline(), nodep->name()};
                        replaceWithCheckBreak(nodep, newp);
                        VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
                    }
                    ok = నిజం;
                    m_ds.m_dot_పాఠ్యం = "";
                } లేకపోతే {
                    (శూన్య)defp;  // Prevent unused variable warning
                }
            } లేకపోతే ఒకవేళ (AstMemberDType* స్థిరరాశి defp = VN_CAST(foundp->nodep(), MemberDType)) {
                ఒకవేళ (allowVar) {
                    check_Member_Decl_క్రమం(nodep, defp);
                    AstRefDType* స్థిరరాశి refp = కొత్త AstRefDType{nodep->fileline(), nodep->name()};
                    refp->refDTypep(defp);
                    replaceWithCheckBreak(nodep, refp);
                    VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
                    ok = నిజం;
                    m_ds.m_dot_పాఠ్యం = "";
                }
            } లేకపోతే ఒకవేళ (Ast_Enum_అంశం* స్థిరరాశి valuep = VN_CAST(foundp->nodep(), Enum_అంశం)) {
                ఒకవేళ (allowVar) {
                    Ast_Enum_అంశం_Ref* స్థిరరాశి refp
                        = కొత్త Ast_Enum_అంశం_Ref{nodep->fileline(), valuep, foundp->classOrPackagep()};
                    Ast_కణుపు* newp = refp;
                    // Hierarchical reference, relinked after V3Param as the referenced
                    // module may be specialized, similar to AstVarXRef
                    refp->dotted(m_ds.m_dot_పాఠ్యం);
                    refp->కలిగియుంది_Gen_ఖండం(m_ds.m_genBlk);
                    ఒకవేళ (m_ds.m_unresolvedCell && m_ds.m_unlinkedScopep) {
                        UINFO(9, indent() << "deferring until post-V3Param: " << refp);
                        newp = కొత్త AstUnlinkedRef{nodep->fileline(), refp, refp->name(),
                                                  m_ds.m_unlinkedScopep->unlink_Fr_వెనుక()};
                        m_ds.m_unlinkedScopep = శూన్యసూచిక;
                        m_ds.m_unresolvedCell = అబద్ధం;
                    }
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
                    ok = నిజం;
                    m_ds.m_dot_పాఠ్యం = "";
                }
            } లేకపోతే ఒకవేళ (స్థిరరాశి AstLambdaArgRef* స్థిరరాశి argrefp
                       = VN_CAST(foundp->nodep(), LambdaArgRef)) {
                ఒకవేళ (allowVar) {
                    Ast_కణుపు* స్థిరరాశి newp
                        = కొత్త AstLambdaArgRef{nodep->fileline(), argrefp->name(), అబద్ధం};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
                    ok = నిజం;
                    m_ds.m_dotPos = DP_MEMBER;
                    m_ds.m_dot_పాఠ్యం = "";
                }
            } లేకపోతే ఒకవేళ (VN_IS(foundp->nodep(), Clocking)) {
                m_ds.m_dotSymp = foundp;
                ఒకవేళ (m_ds.m_dot_పాఠ్యం != "") m_ds.m_dot_పాఠ్యం += "." + nodep->name();
                ok = m_ds.m_dotPos == DP_SCOPE || m_ds.m_dotPos == DP_మొదటి;
            } లేకపోతే ఒకవేళ (స్థిరరాశి AstModportClockingRef* స్థిరరాశి clockingRefp
                       = VN_CAST(foundp->nodep(), ModportClockingRef)) {
                // Clocking block accessed through a modport - redirect to actual clocking
                m_ds.m_dotSymp = m_statep->get_కణుపు_Sym(clockingRefp->clockingp());
                ఒకవేళ (m_ds.m_dot_పాఠ్యం != "") m_ds.m_dot_పాఠ్యం += "." + nodep->name();
                ok = m_ds.m_dotPos == DP_SCOPE || m_ds.m_dotPos == DP_మొదటి;
            } లేకపోతే ఒకవేళ (స్థిరరాశి Ast_కణుపు_FTask* స్థిరరాశి ftaskp = VN_CAST(foundp->nodep(), కణుపు_FTask)) {
                ఒకవేళ (!ftaskp->isFunction() || ftaskp->class_పద్ధతి()) {
                    ok = m_ds.m_dotPos == DP_NONE;
                    ఒకవేళ (ok) {
                        // The condition is true for tasks,
                        // properties and void functions.
                        // In these cases, the parentheses may be skipped.
                        // Also SV class methods can be called without parens
                        AstFuncRef* స్థిరరాశి funcRefp
                            = కొత్త AstFuncRef{nodep->fileline(), nodep->name()};
                        nodep->replaceWith(funcRefp);
                        VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
                    }
                }
            } లేకపోతే ఒకవేళ (AstTypedef* స్థిరరాశి defp = VN_CAST(foundp->nodep(), Typedef)) {
                స్థిరరాశి తార్కిక ifaceFinalSegmentAllowed
                    = (m_ds.m_dotPos == DP_FINAL) && m_ds.m_dotSymp
                      && (VN_IS(m_ds.m_dotSymp->nodep(), Modport)
                          || (VN_IS(m_ds.m_dotSymp->nodep(), Cell)
                              && VN_CAST(m_ds.m_dotSymp->nodep(), Cell)->modp()
                              && VN_IS(VN_CAST(m_ds.m_dotSymp->nodep(), Cell)->modp(), Iface)));
                // Allow interface typedef references even without IfaceCapture
                // The dependency graph handles resolution when IfaceCapture is disabled
                ok = (m_ds.m_dotPos == DP_NONE || m_ds.m_dotPos == DP_SCOPE
                      || ifaceFinalSegmentAllowed);
                ఒకవేళ (ifaceFinalSegmentAllowed) {
                    UINFO(9, indent() << "allow final-segment typedef name="
                                      << nodep->prettyNameQ() << " dotText='" << m_ds.m_dot_పాఠ్యం
                                      << "' dotSym=" << m_ds.m_dotSymp);
                }
                ఒకవేళ (ok) {
                    AstRefDType* స్థిరరాశి refp = కొత్త AstRefDType{nodep->fileline(), nodep->name()};
                    // Don't check if typedef is to a <type T>::<reference> as might not be
                    // resolved yet
                    ఒకవేళ (m_ds.m_dotPos == DP_NONE) check_Decl_క్రమం(nodep, defp);
                    refp->typedefp(defp);

                    V3LinkDotIfaceCapture::captureTypedefContext(
                        refp, "typedef", static_cast<పూర్ణాంకం>(m_ds.m_dotPos), m_ds.m_dot_పాఠ్యం,
                        m_ds.m_dotSymp, m_modp, [ఇది]() { తిరిగి indent(); });

                    ఒకవేళ (VN_IS(nodep->backp(), Sel_వెలికితీయి)) {
                        m_packedArrayDtp = refp;
                    } లేకపోతే {
                        replaceWithCheckBreak(nodep, refp);
                        VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
                    }
                }
            } లేకపోతే ఒకవేళ (AstParamTypeDType* స్థిరరాశి defp = VN_CAST(foundp->nodep(), ParamTypeDType)) {
                ok = (m_ds.m_dotPos == DP_NONE || m_ds.m_dotPos == DP_SCOPE
                      || m_ds.m_dotPos == DP_FINAL);
                ఒకవేళ (ok) {
                    AstRefDType* స్థిరరాశి refp = కొత్త AstRefDType{nodep->fileline(), nodep->name()};
                    refp->refDTypep(defp);

                    V3LinkDotIfaceCapture::captureTypedefContext(
                        refp, "paramtype", static_cast<పూర్ణాంకం>(m_ds.m_dotPos), m_ds.m_dot_పాఠ్యం,
                        m_ds.m_dotSymp, m_modp, [ఇది]() { తిరిగి indent(); });

                    ఒకవేళ (VN_IS(nodep->backp(), Sel_వెలికితీయి)) {
                        m_packedArrayDtp = refp;
                    } లేకపోతే {
                        replaceWithCheckBreak(nodep, refp);
                        VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
                    }
                }
            }
            ఒకవేళ (!ok) {
                ఒకవేళ (m_insideClassExtParam) {
                    // Don't throw error if the reference is inside a class that extends a param,
                    // because some members can't be linked in such a case. m_insideClassExtParam
                    // may be true only in the first stage of linking.
                    // Mark that the Dot statement can't be resolved.
                    m_ds.m_unresolvedClass = నిజం;
                    // If the symbol was a scope name, it would be resolved.
                    m_ds.m_dotPos = DP_MEMBER;
                } లేకపోతే {
                    // Cells/interfaces can't be implicit
                    స్థిరరాశి తార్కిక checkImplicit
                        = (!m_ds.m_dotp && m_ds.m_dot_పాఠ్యం == "" && !m_ds.m_disablep && !foundp);
                    స్థిరరాశి తార్కిక err
                        = !(checkImplicit && m_statep->implicitOk(m_modp, nodep->name()));
                    ఒకవేళ (err) {
                        ఒకవేళ (foundp) {
                            nodep->v3error("Found definition of '"
                                           << m_ds.m_dot_పాఠ్యం << (m_ds.m_dot_పాఠ్యం == "" ? "" : ".")
                                           << nodep->prettyName() << "'" << " as a "
                                           << foundp->nodep()->typeName() << " but expected a "
                                           << expectWhat);
                        } లేకపోతే ఒకవేళ (m_ds.m_dot_పాఠ్యం == "") {
                            UINFO(1, "   ErrParseRef curSymp=se" << cvt_గమ్యం_Hex(m_curSymp)
                                                                 << " ds=" << m_ds.ascii());
                            స్థిరరాశి string suggest = m_statep->suggestSymFallback(
                                m_ds.m_dotSymp, nodep->name(), VNodeMatcher{});
                            nodep->v3error(
                                "Can't find definition of "
                                << expectWhat << ": " << nodep->prettyNameQ() << '\n'
                                << (suggest.ఖాళీలు() ? "" : nodep->warn_మరిన్ని() + suggest));
                        } లేకపోతే {
                            nodep->v3error(
                                "Can't find definition of "
                                << (!baddot.ఖాళీలు() ? Ast_కణుపు::prettyNameQ(baddot)
                                                    : nodep->prettyNameQ())
                                << " in dotted " << expectWhat << ": '"
                                << m_ds.m_dot_పాఠ్యం + "." + nodep->prettyName() << "'\n"
                                << nodep->warnContextPrimary()
                                << (okSymp ? okSymp->cellErrorScopes(
                                                 nodep, Ast_కణుపు::prettyName(m_ds.m_dot_పాఠ్యం))
                                           : ""));
                        }
                        m_ds.m_dotErr = నిజం;
                    }
                    ఒకవేళ (checkImplicit) {
                        // Create if implicit, and also if error (so only complain once)
                        // Else if a scope is allowed, making a signal won't help error cascade
                        AstVar* స్థిరరాశి varp
                            = createImplicitVar(m_curSymp, nodep, m_modp, m_modSymp, err);
                        AstVarRef* స్థిరరాశి newp
                            = కొత్త AstVarRef{nodep->fileline(), varp, VAccess::READ};
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
                    }
                }
            }
        }
        ఒకవేళ (ప్రారంభం) m_ds = చివరి_స్థితులు;
    }
    శూన్య సందర్శించు(AstClassOrPackageRef* nodep) override {
        // Class: Recurse inside or cleanup not founds
        // checkNoDot not appropriate, can be under a dot
        LINKDOT_సందర్శించు_ప్రారంభం();
        UINFO(8, indent() << "visit " << nodep);
        UINFO(9, indent() << m_ds.ascii());
        VL_RESTORER_CLEAR(m_వాడిన_Pins);
        VL_RESTORER_CLEAR(m_వాడిన_Def_Param_Pins);
        UASSERT_OBJ(m_statep->forPrimary() || !nodep->paramsp(), nodep,
                    "class reference parameter not removed by V3Param");
        {
            VL_RESTORER_COPY(m_ds);
            VL_RESTORER(m_pinSymp);

            ఒకవేళ (!nodep->classOrPackageSkipp() && !nodep->classOrPackageNodep()
                && nodep->name() != "local::") {
                స్థిరరాశి తార్కిక deferIfUnresolved = m_statep->forPrimary() && m_insideClassExtParam;
                m_statep->resolveClassOrPackage(m_ds.m_dotSymp, nodep, m_ds.m_dotPos != DP_PACKAGE,
                                                అబద్ధం, ":: reference", deferIfUnresolved);
            }

            // ClassRef's have pins, so track
            ఒకవేళ (nodep->classOrPackageSkipp()) {
                m_pinSymp = m_statep->get_కణుపు_Sym(nodep->classOrPackageSkipp());
            } లేకపోతే ఒకవేళ (nodep->name() != "local::") {
                తిరిగి;
            }

            AstClass* స్థిరరాశి refClassp = VN_CAST(nodep->classOrPackageSkipp(), Class);
            // Make sure any extends() are properly imported within referenced class
            ఒకవేళ (refClassp && !m_statep->forPrimary()) classExtendImport(refClassp);

            ఒకవేళ (m_statep->forPrimary() && refClassp && !nodep->paramsp()
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
                        << nodep->warn_మరిన్ని() << "... Suggest use '"
                        << nodep->classOrPackageNodep()->prettyName() << "#()'");
                // As other simulators do, use the default specialization. Add the same
                // placeholder pin V3LinkCells makes for '#()' so it is linked below.
                AstPin* స్థిరరాశి pinp = కొత్త AstPin{nodep->fileline(), 1, "__paramNumber1", శూన్యసూచిక};
                pinp->param(నిజం);
                nodep->కూడు_Paramsp(pinp);
            }

            m_ds.init(m_curSymp);
            UINFO(4, indent() << "(Backto) Link ClassOrPackageRef: " << nodep);
            iterateChildren(nodep);
        }

        ఒకవేళ (nodep->name() == "local::") {
            ఒకవేళ (!m_randSymp && !m_rand_పద్ధతి_Callp) {
                nodep->v3error("Illegal 'local::' outside 'randomize() with'"
                               " (IEEE 1800-2023 18.7.1)");
                m_ds.m_dotErr = నిజం;
                తిరిగి;
            }
        }
        ఒకవేళ (m_ds.m_dotPos == DP_PACKAGE && nodep->classOrPackageSkipp()) {
            m_ds.m_dotSymp = m_statep->get_కణుపు_Sym(nodep->classOrPackageSkipp());
            UINFO(9, indent() << "set sym " << m_ds.ascii());
        }
    }
    శూన్య సందర్శించు(AstConstraint* nodep) override {
        LINKDOT_సందర్శించు_ప్రారంభం();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        ఒకవేళ (nodep->isExternDef()) {
            ఒకవేళ (స్థిరరాశి VSymEnt* స్థిరరాశి foundp
                = m_curSymp->కనుగొను_Id_Fallback("extern " + nodep->name())) {
                స్థిరరాశి AstConstraint* స్థిరరాశి protop = VN_AS(foundp->nodep(), Constraint);
                // Copy specifiers.
                // External definition cannot have any specifiers, so no value will be overwritten.
                nodep->isStatic(protop->isStatic());
            } లేకపోతే {
                nodep->v3error("extern not found that declares " + nodep->prettyNameQ());
            }
        }
        ఒకవేళ (nodep->isExternProto()) {
            ఒకవేళ (!m_curSymp->కనుగొను_Id_Fallback(nodep->name()) && nodep->isExternExplicit()) {
                nodep->v3warn(PROTOTYPEMIS, "Definition not found for extern prototype "
                                                << nodep->prettyNameQ());
            }
        }
        VL_RESTORER(m_curSymp);
        VL_RESTORER_COPY(m_ds);
        m_ds.m_dotSymp = m_curSymp = m_statep->get_కణుపు_Sym(nodep);
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(AstConstraintRef* nodep) override {
        ఒకవేళ (nodep->user3_సమితి_Once()) తిరిగి;
        LINKDOT_సందర్శించు_ప్రారంభం();
        UINFO(8, indent() << "visit " << nodep);
        UINFO(8, indent() << m_ds.ascii());
        // No children defined
    }
    శూన్య సందర్శించు(AstVarRef* nodep) override {
        // VarRef: Resolve its reference
        // ParseRefs are used the first pass (forPrimary) so we shouldn't get can't find
        // errors here now that we have a VarRef.
        // No checkNoDot; created and iterated from a parseRef
        LINKDOT_సందర్శించు_ప్రారంభం();
        iterateChildren(nodep);
        ఒకవేళ (!nodep->varp()) {
            UINFO(9, indent() << "linkVarRef se" << cvt_గమ్యం_Hex(m_curSymp) << "  n=" << nodep);
            UASSERT_OBJ(m_curSymp, nodep, "nullptr lookup symbol table");
            VSymEnt* స్థిరరాశి foundp = m_curSymp->కనుగొను_Id_Fallback(nodep->name());
            ఒకవేళ (AstVar* స్థిరరాశి varp
                = foundp ? దొరికింది_గమ్యం_Varp(foundp, nodep, nodep->access()) : శూన్యసూచిక) {
                nodep->varp(varp);
                updateVarUse(nodep->varp());
                // Generally set by parse, but might be an import
                nodep->classOrPackagep(foundp->classOrPackagep());
            }
            ఒకవేళ (VL_UNCOVERABLE(!nodep->varp())) {
                nodep->v3error("Can't find definition of signal, again: "  // LCOV_EXCL_LINE
                               << nodep->prettyNameQ());
            }
        }
        AstVarScope* vscp = nodep->varScopep();
        ఒకవేళ (vscp && vscp->user2p() != vscp && m_replaceWithAlias) {
            vscp = LinkDotScopeVisitor::getAlias(vscp);
            nodep->varp(vscp->varp());
            nodep->varScopep(vscp);
            updateVarUse(nodep->varp());
            UINFO(7, indent() << "Resolved " << nodep);  // Also prints taskp
        }
    }
    శూన్య సందర్శించు(AstVarXRef* nodep) override {
        // VarRef: Resolve its reference
        // We always link even if varp() is set, because the module we choose may change
        // due to creating new modules, flattening, etc.
        ఒకవేళ (nodep->user3_సమితి_Once()) తిరిగి;
        LINKDOT_సందర్శించు_ప్రారంభం();
        UINFO(8, indent() << "visit " << nodep);
        // No checkNoDot; created and iterated from a parseRef
        ఒకవేళ (!m_modSymp) {
            // Module that is not in hierarchy.  We'll be dead code eliminating it later.
            UINFO(9, "Dead module for " << nodep);
            nodep->varp(శూన్యసూచిక);
        } లేకపోతే {
            VSymEnt* lookupSymp = m_curSymp;  // Start search at current scope
            ఒకవేళ (nodep->inlinedDots() != "") {  // Correct for current post-inlined scope
                lookupSymp = కనుగొను_Inlined_Dots_Sym(nodep, nodep->inlinedDots());
            }
            string baddot;
            VSymEnt* okSymp;
            తార్కిక modport;
            VSymEnt* స్థిరరాశి dotSymp
                = కనుగొను_Dotted_Ref_Symp(nodep, lookupSymp, nodep->dotted(), baddot, okSymp, &modport);
            ఒకవేళ (!dotSymp) తిరిగి;

            ఒకవేళ (!m_statep->forScopeCreation()) {
                VSymEnt* foundp = శూన్యసూచిక;
                ఒకవేళ (modport) {
                    // This is a copy of findSymPrefixed with few modifications.
                    // This searches without a fallback like findSymPrefixed but if lookup fails it
                    // will check one fallback (fallback of modport shall be interface) and take
                    // only L and G PARAMs since they should be visible from a modport always.
                    string ఉపసర్గ = dotSymp->sym_ఉపసర్గ();
                    అయితే (!foundp) {
                        foundp = dotSymp->కనుగొను_Id_Flat(ఉపసర్గ + nodep->name());
                        ఒకవేళ (foundp) ఆపు;
                        UASSERT_OBJ(dotSymp->fallbackp(), nodep, "Modports shall have fallback");
                        foundp = dotSymp->fallbackp()->కనుగొను_Id_Flat(ఉపసర్గ + nodep->name());
                        ఒకవేళ (స్థిరరాశి AstVar* స్థిరరాశి varp
                            = దొరికింది_గమ్యం_Varp(foundp, nodep, nodep->access())) {
                            ఒకవేళ (!varp->isParam()) foundp = శూన్యసూచిక;
                        }
                        ఒకవేళ (ఉపసర్గ.ఖాళీలు()) ఆపు;
                        string తదుపరి_ఉపసర్గ = LinkDotState::remove_చివరి_Inline_Scope(ఉపసర్గ);
                        ఒకవేళ (ఉపసర్గ == తదుపరి_ఉపసర్గ) ఆపు;
                        ఉపసర్గ = std::move(తదుపరి_ఉపసర్గ);
                    }
                    baddot = nodep->name();
                } లేకపోతే {
                    foundp = m_statep->కనుగొను_Sym_Prefixed(dotSymp, nodep->name(), baddot, నిజం);
                }
                ఒకవేళ (m_లోపల_Sens && foundp) {
                    ఒకవేళ (AstClocking* స్థిరరాశి clockingp = sensClockingp(foundp->nodep())) {
                        foundp = getCreateClockingEventSymEnt(clockingp);
                    }
                }
                AstVar* స్థిరరాశి varp
                    = foundp ? దొరికింది_గమ్యం_Varp(foundp, nodep, nodep->access()) : శూన్యసూచిక;
                nodep->varp(varp);
                updateVarUse(nodep->varp());
                UINFO(7, indent() << "Resolved " << nodep);  // Also prints varp
                // If found, check if it's ok to access in case it's in a hier_block
                ఒకవేళ (nodep->varp() && error_Hier_Non_పోర్టు(nodep, nodep->varp(), dotSymp)) తిరిగి;
                ఒకవేళ (!nodep->varp()) {
                    nodep->v3error("Can't find definition of "
                                   << Ast_కణుపు::prettyNameQ(baddot) << " in dotted signal: '"
                                   << nodep->dotted() + "." + nodep->prettyName() << "'\n"
                                   << nodep->warnContextPrimary()
                                   << okSymp->cellErrorScopes(nodep));
                    తిరిగి;
                }
            } లేకపోతే {
                VSymEnt* స్థిరరాశి foundp
                    = m_statep->కనుగొను_Sym_Prefixed(dotSymp, nodep->name(), baddot, నిజం);
                AstVarScope* vscp = foundp ? VN_CAST(foundp->nodep(), VarScope) : శూన్యసూచిక;
                // Handle modport expression virtual ports
                ఒకవేళ (!vscp) vscp = కనుగొను_Virtual_పోర్టు_Var_Scope(dotSymp, nodep->name());
                // If found, check if it's ok to access in case it's in a hier_block
                ఒకవేళ (vscp && error_Hier_Non_పోర్టు(nodep, vscp->varp(), dotSymp)) తిరిగి;
                ఒకవేళ (!vscp) {
                    nodep->v3error("Can't find varpin scope of "
                                   << Ast_కణుపు::prettyNameQ(baddot) << " in dotted signal: '"
                                   << nodep->dotted() + "." + nodep->prettyName() << "'\n"
                                   << nodep->warnContextPrimary()
                                   << okSymp->cellErrorScopes(nodep));
                } లేకపోతే {
                    ఒకవేళ (vscp->user2p() && m_replaceWithAlias) {
                        vscp = LinkDotScopeVisitor::getAlias(vscp);
                    }
                    // Convert the VarXRef to a VarRef, so we don't need
                    // later optimizations to deal with VarXRef.
                    nodep->varp(vscp->varp());
                    nodep->varScopep(vscp);
                    updateVarUse(nodep->varp());
                    UINFO(7, indent() << "Resolved " << nodep);  // Also prints taskp
                    AstVarRef* స్థిరరాశి newvscp
                        = కొత్త AstVarRef{nodep->fileline(), vscp, nodep->access()};
                    nodep->replaceWith(newvscp);
                    VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
                    UINFO(9, indent() << "new " << newvscp);  // Also prints taskp
                }
            }
        }
    }
    శూన్య సందర్శించు(AstEnumDType* nodep) override {
        LINKDOT_సందర్శించు_ప్రారంభం();
        iterateChildren(nodep);
        AstRefDType* స్థిరరాశి refdtypep = VN_CAST(nodep->subDTypep(), RefDType);
        ఒకవేళ (refdtypep && (nodep == refdtypep->subDTypep())) {
            refdtypep->v3error("Self-referential enumerated type definition");
        }
    }
    శూన్య సందర్శించు(Ast_కణుపు_UOr_Struct_DType* nodep) override {
        LINKDOT_సందర్శించు_ప్రారంభం();
        symIterateChildren(nodep, m_statep->get_కణుపు_Sym(nodep));
    }
    శూన్య సందర్శించు(Ast_Enum_అంశం_Ref* nodep) override {
        // Resolve its reference
        // EnumItemRefs are created by the first pass, but V3Param may regenerate due to
        // a parameterized class/module, so we shouldn't get can't find errors.
        // No checkNoDot; created and iterated from a parseRef
        ఒకవేళ (nodep->user3_సమితి_Once()) తిరిగి;
        LINKDOT_సందర్శించు_ప్రారంభం();
        ఒకవేళ (m_statep->forParamed() && !nodep->dotted().ఖాళీలు() && m_modSymp) {
            // Hierarchical reference, relink as V3Param may have specialized the module
            string baddot;
            VSymEnt* okSymp = శూన్యసూచిక;
            VSymEnt* స్థిరరాశి dotSymp
                = కనుగొను_Dotted_Ref_Symp(nodep, m_curSymp, nodep->dotted(), baddot, okSymp);
            ఒకవేళ (!dotSymp) తిరిగి;
            VSymEnt* స్థిరరాశి foundp
                = m_statep->కనుగొను_Sym_Prefixed(dotSymp, nodep->name(), baddot, నిజం);
            Ast_Enum_అంశం* స్థిరరాశి itemp = foundp ? VN_CAST(foundp->nodep(), Enum_అంశం) : శూన్యసూచిక;
            ఒకవేళ (!itemp) {
                nodep->v3error("Enum item " << nodep->prettyNameQ() << " not found in "
                                            << Ast_కణుపు::prettyNameQ(nodep->dotted()) << '\n'
                                            << nodep->warnContextPrimary());
                తిరిగి;
            }
            nodep->itemp(itemp);
            UINFO(9, indent() << " relinked " << nodep);
        }
        ఒకవేళ (!nodep->itemp()) {
            UINFO(9, indent() << "linkEnumRef se" << cvt_గమ్యం_Hex(m_curSymp) << "  n=" << nodep);
            UASSERT_OBJ(m_curSymp, nodep, "nullptr lookup symbol table");
            VSymEnt* స్థిరరాశి foundp = m_curSymp->కనుగొను_Id_Fallback(nodep->name());
            ఒకవేళ (Ast_Enum_అంశం* స్థిరరాశి itemp = foundp ? VN_CAST(foundp->nodep(), Enum_అంశం) : శూన్యసూచిక) {
                nodep->itemp(itemp);
                nodep->classOrPackagep(foundp->classOrPackagep());
                UINFO(9, indent() << " resolved " << nodep);
            }
            ఒకవేళ (VL_UNCOVERABLE(!nodep->itemp())) {
                nodep->v3error("Can't find definition of enum item, again: "  // LCOV_EXCL_LINE
                               << nodep->prettyNameQ());
            }
        }
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(Ast_పద్ధతి_Call* nodep) override {
        // Created here so should already be resolved.
        LINKDOT_సందర్శించు_ప్రారంభం();
        UINFO(5, indent() << "visit " << nodep);
        VL_RESTORER_COPY(m_ds);
        VL_RESTORER(m_randSymp);
        VL_RESTORER(m_rand_పద్ధతి_Callp);
        {
            m_ds.init(m_curSymp);
            ఒకవేళ (nodep->name() == "randomize" && (nodep->argsp() || nodep->withp())) {
                m_rand_పద్ధతి_Callp = nodep;
                స్థిరరాశి Ast_కణుపు_DType* fromDtp = nodep->fromp()->dtypep();
                ఒకవేళ (!fromDtp) {
                    ఒకవేళ (స్థిరరాశి Ast_కణుపు_Var_Ref* స్థిరరాశి varRefp = VN_CAST(nodep->fromp(), కణుపు_Var_Ref)) {
                        fromDtp = varRefp->varp()->subDTypep();
                    } లేకపోతే {
                        fromDtp = get_సమాసం_DTypep(nodep->fromp());
                    }
                    ఒకవేళ (!fromDtp) {
                        ఒకవేళ (nodep->withp()) {
                            nodep->v3warn(
                                E_UNSUPPORTED,
                                "Unsupported: 'randomize() with' on complex expressions");
                        } లేకపోతే {
                            nodep->v3warn(E_UNSUPPORTED,
                                          "Unsupported: Inline random variable control with "
                                          "'randomize()' called on complex expressions");
                        }
                    }
                }
                ఒకవేళ (m_statep->forPrimary() && isParamedClassRefDType(fromDtp)) {
                    m_ds.m_unresolvedClass = నిజం;
                } లేకపోతే ఒకవేళ (fromDtp) {
                    స్థిరరాశి AstClassRefDType* స్థిరరాశి classDtp
                        = VN_CAST(fromDtp->skipRefp(), ClassRefDType);
                    ఒకవేళ (!classDtp)
                        nodep->v3error("'randomize() with' on a non-class-instance "
                                       << fromDtp->prettyNameQ());
                    లేకపోతే ఒకవేళ (m_statep->forPrimary() && m_extendsParam.లెక్క(classDtp->classp())) {
                        // Inherited members are unavailable until the base class is specialized.
                        // Defer lookup rather than incorrectly binding names in the caller.
                        m_ds.m_unresolvedClass = నిజం;
                    } లేకపోతే {
                        m_randSymp = m_statep->get_కణుపు_Sym(classDtp->classp());
                    }
                }
            }
            iterateChildren(nodep);
        }
    }
    శూన్య సందర్శించు(AstVar* nodep) override {
        LINKDOT_సందర్శించు_ప్రారంభం();
        checkNoDot(nodep);
        VL_RESTORER(m_isParam);
        m_isParam = nodep->varType().isParam();
        iterateChildren(nodep);
        ఒకవేళ (m_statep->forPrimary() && nodep->isIO() && !m_ftaskp && !nodep->user4()) {
            nodep->v3error(
                "Input/output/inout does not appear in port list: " << nodep->prettyNameQ());
        }
    }
    శూన్య సందర్శించు(AstVarScope* nodep) override {
        LINKDOT_సందర్శించు_ప్రారంభం();
        checkNoDot(nodep);
        iterateChildren(nodep);
        AstVarScope* aliasp = LinkDotScopeVisitor::getAlias(nodep);
        ఒకవేళ (aliasp && aliasp != nodep && !nodep->varp()->isIfaceRef()) {
            // Aliased variable might still be references from outside,
            // eg through the VPI, and is traced, so we need the value to propagate.
            // TODO: this means external writes to the LHS (e.g.: through the VPI) don't work
            Ast_కేటాయించు_W* స్థిరరాశి assignp = కొత్త Ast_కేటాయించు_W{
                nodep->fileline(), కొత్త AstVarRef{nodep->fileline(), nodep, VAccess::WRITE},
                కొత్త AstVarRef{nodep->fileline(), aliasp, VAccess::READ}};
            assignp->user2(నిజం);
            nodep->scopep()->కూడు_Blocksp(కొత్త AstAlways{assignp});
            // Propagate attributes and lint state of the replaced variable,
            // because all references to it are replaced with references to the alias variable
            aliasp->varp()->propagateAttrFrom(nodep->varp());
            aliasp->varp()->fileline()->modifyStateInherit(nodep->varp()->fileline());
        }
    }
    శూన్య సందర్శించు(Ast_కణుపు_FTask_Ref* nodep) override {
        ఒకవేళ (nodep->user3_సమితి_Once()) తిరిగి;
        LINKDOT_సందర్శించు_ప్రారంభం();
        UINFO(8, indent() << "visit " << nodep);
        UINFO(8, indent() << m_ds.ascii());
        ఒకవేళ (m_ds.m_dotPos != DP_MEMBER || nodep->name() != "randomize") {
            // Visit arguments at the beginning.
            // They may be visitted even if the current node can't be linked now.
            symIterateChildren(nodep, m_curSymp);
        }

        ఒకవేళ (m_ds.m_super) {
            ఒకవేళ (AstFuncRef* స్థిరరాశి funcRefp = VN_CAST(nodep, FuncRef)) {
                funcRefp->superReference(నిజం);
            } లేకపోతే ఒకవేళ (AstTaskRef* స్థిరరాశి taskRefp = VN_CAST(nodep, TaskRef)) {
                taskRefp->superReference(నిజం);
            }
        }

        VL_RESTORER(m_randSymp);

        తార్కిక మొదటి = !m_ds.m_dotp || m_ds.m_dotPos == DP_మొదటి;
        తార్కిక staticAccess = అబద్ధం;
        ఒకవేళ (m_ds.m_unresolvedClass) {
            // Unable to link before V3Param
            తిరిగి;
        } లేకపోతే ఒకవేళ (m_ds.m_unresolvedCell && m_ds.m_dotPos == DP_FINAL && m_ds.m_unlinkedScopep) {
            Ast_కణుపు_FTask_Ref* స్థిరరాశి newftaskp = nodep->clone_వృక్షం(అబద్ధం);
            newftaskp->dotted(m_ds.m_dot_పాఠ్యం);
            Ast_కణుపు* స్థిరరాశి newp = కొత్త AstUnlinkedRef{nodep->fileline(), newftaskp, nodep->name(),
                                                     m_ds.m_unlinkedScopep->unlink_Fr_వెనుక()};
            m_ds.m_unlinkedScopep = శూన్యసూచిక;
            m_ds.m_unresolvedCell = అబద్ధం;
            nodep->replaceWith(newp);
            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
            తిరిగి;
        } లేకపోతే ఒకవేళ (m_ds.m_dotp && m_ds.m_dotPos == DP_PACKAGE) {
            UASSERT_OBJ(VN_IS(m_ds.m_dotp->lhsp(), ClassOrPackageRef), m_ds.m_dotp->lhsp(),
                        "Bad package link");
            staticAccess = నిజం;
            AstClassOrPackageRef* స్థిరరాశి cpackagerefp
                = VN_AS(m_ds.m_dotp->lhsp(), ClassOrPackageRef);
            ఒకవేళ (cpackagerefp->name() == "local::") {
                m_randSymp = శూన్యసూచిక;
                మొదటి = నిజం;
            } లేకపోతే ఒకవేళ (!cpackagerefp->classOrPackageSkipp()) {
                VSymEnt* స్థిరరాశి foundp = m_statep->resolveClassOrPackage(
                    m_ds.m_dotSymp, cpackagerefp, నిజం, అబద్ధం, ":: reference");
                ఒకవేళ (foundp) nodep->classOrPackagep(cpackagerefp->classOrPackageSkipp());
            } లేకపోతే {
                nodep->classOrPackagep(cpackagerefp->classOrPackageSkipp());
            }
            // Class/package :: HERE function() . method_called_on_function_return_value()
            m_ds.m_dotPos = DP_MEMBER;
        } లేకపోతే ఒకవేళ (m_ds.m_dotp && m_ds.m_dotPos == DP_FINAL) {
            nodep->dotted(m_ds.m_dot_పాఠ్యం);  // Maybe ""
            // Only flag FTaskRefs under generate-if/case blocks that may be
            // pruned.  GenFor and plain begin-blocks won't be pruned by V3Param.
            // A generate-case block's parent is the GenCaseItem, not the GenCase.
            // VarXRef uses the broader m_genBlk flag (set for all GenBlocks)
            // because genfor unrolling also removes variables.
            ఒకవేళ (m_ds.m_genBlk && m_ds.m_dotSymp) {
                స్థిరరాశి Ast_కణుపు* స్థిరరాశి blkp = m_ds.m_dotSymp->nodep();
                ఒకవేళ (VN_IS(blkp, Gen_ఖండం)
                    && (VN_IS(blkp->backp(), GenIf) || VN_IS(blkp->backp(), Gen_Case_అంశం))) {
                    nodep->కలిగియుంది_Gen_ఖండం(నిజం);
                }
            }
        } లేకపోతే ఒకవేళ (m_ds.m_dotp && m_ds.m_dotPos == DP_MEMBER) {
            // Found a Var, everything following is method call.
            // {scope}.{var}.HERE {method} ( ARGS )
            Ast_కణుపు_సమాసం* స్థిరరాశి varEtcp = VN_AS(m_ds.m_dotp->lhsp()->unlink_Fr_వెనుక(), కణుపు_సమాసం);
            AstArg* స్థిరరాశి argsp = nodep->argsp();
            ఒకవేళ (argsp) argsp->unlink_Fr_వెనుక_With_తదుపరి();
            Ast_పద్ధతి_Call* స్థిరరాశి newp = కొత్త Ast_పద్ధతి_Call{nodep->fileline(), varEtcp,
                                                          VFlag_బిడ్డ_DType{}, nodep->name(), argsp};
            ఒకవేళ (AstWith* స్థిరరాశి withp = nodep->withp()) newp->withp(withp->unlink_Fr_వెనుక());
            nodep->replaceWith(newp);
            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
            తిరిగి;
        } లేకపోతే ఒకవేళ (m_ds.m_dotp && (m_ds.m_dotPos == DP_SCOPE || m_ds.m_dotPos == DP_మొదటి)) {
            // HERE function() . method_called_on_function_return_value()
            m_ds.m_dotPos = DP_MEMBER;
            m_ds.m_dot_పాఠ్యం = "";
        } లేకపోతే ఒకవేళ (!m_ds.m_disablep) {
            // visit(AstDisable*) setup the dot handling
            checkNoDot(nodep);
        }
        ఒకవేళ (nodep->classOrPackagep() && nodep->taskp()) {
            // References into packages don't care about cell hierarchy.
        } లేకపోతే ఒకవేళ (!m_modSymp) {
            // Module that is not in hierarchy.  We'll be dead code eliminating it later.
            UINFO(9, indent() << "Dead module for " << nodep);
            nodep->taskp(శూన్యసూచిక);
        } లేకపోతే ఒకవేళ (nodep->dotted() == "" && nodep->taskp()) {
            // Earlier should have setup the links
            // Might be under a BEGIN we're not processing, so don't relink it
        } లేకపోతే {
            string baddot;
            VSymEnt* okSymp = శూన్యసూచిక;
            VSymEnt* dotSymp;
            ఒకవేళ (nodep->dotted().ఖాళీలు()) {
                // Non-'super.' dotted reference
                dotSymp = m_ds.m_dotSymp;
            } లేకపోతే {
                // Start search at module, as a variable of same name under a subtask isn't a
                // relevant hit however a function under a begin/end is.  So we want begins, but
                // not the function
                dotSymp = m_curSymp;
            }
            ఒకవేళ (nodep->classOrPackagep()) {  // Look only in specified package
                dotSymp = m_statep->get_కణుపు_Sym(nodep->classOrPackagep());
                UINFO(8, indent() << "Override classOrPackage " << dotSymp);
            } లేకపోతే {
                ఒకవేళ (nodep->inlinedDots() != "") {  // Correct for current post-inlined scope
                    dotSymp = కనుగొను_Inlined_Dots_Sym(nodep, nodep->inlinedDots());
                }
                dotSymp = m_statep->కనుగొను_Dotted(nodep->fileline(), dotSymp, nodep->dotted(), baddot,
                                               okSymp, నిజం);  // Maybe nullptr
            }
            // Restricted 'with (id_list)': only id_list names bind into target class.
            ఒకవేళ (m_randSymp
                && (!m_currentWithp || m_currentWithp->name_Resolves_గమ్యం_లక్ష్యం(nodep->name()))) {
                VSymEnt* స్థిరరాశి foundp = m_randSymp->కనుగొను_Id_Flat(nodep->name());
                ఒకవేళ (foundp && m_currentWithp) {
                    ఒకవేళ (m_currentWithp->restricted()) {
                        m_restricted_పేర్లు_వాడిన.చొప్పించు(nodep->name());
                    }
                    UINFO(9, indent() << "randomize-with fromSym " << foundp->nodep());
                    AstArg* argsp = శూన్యసూచిక;
                    ఒకవేళ (nodep->argsp()) {
                        iterate_And_తదుపరి_Null(nodep->argsp());
                        argsp = nodep->argsp()->unlink_Fr_వెనుక_With_తదుపరి();
                    }
                    ఒకవేళ (m_ds.m_dotPos != DP_NONE) m_ds.m_dotPos = DP_MEMBER;
                    Ast_కణుపు* స్థిరరాశి newp = కొత్త Ast_పద్ధతి_Call{
                        nodep->fileline(),
                        కొత్త AstLambdaArgRef{nodep->fileline(), "__Vrandwith_obj", అబద్ధం},
                        VFlag_బిడ్డ_DType{}, nodep->name(), argsp};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
                    తిరిగి;
                }
            }
            ఒకవేళ (మొదటి && nodep->name() == "randomize" && VN_IS(m_modp, Class)
                && !VN_IS(nodep->classOrPackagep(), Package)) {
                // need special handling to avoid falling back to std::randomize
                // Skip if classOrPackagep is a Package (i.e. std::randomize resolved earlier)
                VMemberMap memberMap;
                AstFunc* స్థిరరాశి randFuncp = V3Randomize::newRandomizeFunc(
                    memberMap, VN_AS(m_modp, Class), nodep->name(), నిజం, నిజం);
                nodep->taskp(randFuncp);
                nodep->classOrPackagep(VN_AS(m_modp, Class));
                m_curSymp = m_statep->చొప్పించు_ఖండం(m_curSymp, nodep->name(), randFuncp, m_modp);
                // Already linked to the class randomize function; skip findSymPrefixed
                // which may find std::randomize and overwrite classOrPackagep
                తిరిగి;
            }
            VSymEnt* స్థిరరాశి foundp
                = m_statep->కనుగొను_Sym_Prefixed(dotSymp, nodep->name(), baddot, మొదటి);
            Ast_కణుపు_FTask* స్థిరరాశి taskp = foundp ? VN_CAST(foundp->nodep(), కణుపు_FTask) : శూన్యసూచిక;
            Ast_కణుపు_Module* స్థిరరాశి taskModulep = foundp ? foundp->classOrPackagep() : శూన్యసూచిక;
            // Ignore deferring linking of functions inside params of classes extending
            // parametric classes if referenced function can be linked in the first pass.
            ఒకవేళ (!m_isParam || !taskp
                || (taskp->class_పద్ధతి() && taskModulep && taskModulep->hasParameterList())) {
                ఒకవేళ (m_insideClassExtParam) {
                    // The reference may point to a method declared in a super class, which is
                    // proved by a parameter. In such a case, it can't be linked at the first
                    // stage. Must not do any linking, because e.g. might find an extends of an
                    // upper class because the current class (under parent) isn't yet importing
                    // it's extended class symbols.
                    తిరిగి;
                }
                ఒకవేళ (AstClass* స్థిరరాశి లక్ష్యం_Classp = VN_CAST(dotSymp->nodep(), Class)) {
                    ఒకవేళ (m_extendsParam.లెక్క(లక్ష్యం_Classp)) {
                        // Target class has parameterized extends not yet resolved.
                        // Its inherited symbols (e.g. static functions from the base class)
                        // aren't imported yet - defer to linkDotParamed.
                        తిరిగి;
                    }
                }
            }

            ఒకవేళ (taskp) {
                ఒకవేళ (staticAccess && !taskp->isStatic()) {
                    // TODO bug4077
                    // nodep->v3error("Static access to non-static task/function "
                    //                << taskp->prettyNameQ() << endl);
                }
                nodep->taskp(taskp);
                nodep->classOrPackagep(foundp->classOrPackagep());
                UINFO(7, indent() << "Resolved " << nodep);  // Also prints taskp
            } లేకపోతే {
                // Note ParseRef has similar error handling/message output
                UINFO(7, indent() << "   ErrFtask curSymp=se" << cvt_గమ్యం_Hex(m_curSymp)
                                  << " dotSymp=se" << cvt_గమ్యం_Hex(dotSymp));
                ఒకవేళ (foundp) {
                    ఒకవేళ (VN_IS(foundp->nodep(), Var) && m_ds.m_dot_పాఠ్యం == "" && m_ftaskp
                        && m_ftaskp->name() == foundp->nodep()->name()) {
                        // This is a recursive reference to the function itself, not to the var
                        nodep->taskp(m_ftaskp);
                        nodep->classOrPackagep(foundp->classOrPackagep());
                        UINFO(7, indent() << "Resolved recursive " << nodep);  // Also prints taskp
                    } లేకపోతే {
                        nodep->v3error("Found definition of '"
                                       << m_ds.m_dot_పాఠ్యం << (m_ds.m_dot_పాఠ్యం == "" ? "" : ".")
                                       << nodep->prettyName() << "'" << " as a "
                                       << foundp->nodep()->typeName()
                                       << " but expected a task/function");
                    }
                } లేకపోతే ఒకవేళ (VN_IS(nodep, New) && !m_statep->forScopeCreation()) {
                    // Resolved in V3Width
                } లేకపోతే ఒకవేళ ((nodep->name() == "pre_randomize" || nodep->name() == "post_randomize")
                           && VN_IS(dotSymp->nodep(), Class)) {
                    AstClass* స్థిరరాశి classp = VN_AS(dotSymp->nodep(), Class);
                    // Functions that might be defined by user, but if not, they get empty
                    // definitions within referenced class.
                    // TODO perhaps cleaner to add to symbol table up-front with
                    // a lambda to construct them on a reference.
                    AstFunc* స్థిరరాశి funcp
                        = కొత్త AstFunc{nodep->fileline(), nodep->name(), శూన్యసూచిక, శూన్యసూచిక};
                    funcp->class_పద్ధతి(నిజం);
                    funcp->dtypep(funcp->కనుగొను_Void_DType());
                    classp->కూడు_Membersp(funcp);
                    m_statep->చొప్పించు_ఖండం(dotSymp, nodep->name(), funcp, classp);
                    nodep->taskp(funcp);
                    nodep->classOrPackagep(classp);
                    UINFO(9, indent() << " Auto-made " << funcp);
                    తిరిగి;
                } లేకపోతే ఒకవేళ (nodep->name() == "randomize" || nodep->name() == "srandom"
                           || nodep->name() == "get_randstate" || nodep->name() == "set_randstate"
                           || nodep->name() == "rand_mode" || nodep->name() == "constraint_mode") {
                    ఒకవేళ (AstClass* స్థిరరాశి classp = VN_CAST(m_modp, Class)) {
                        // Don't overwrite if already resolved to a package (e.g. std::randomize)
                        ఒకవేళ (!VN_IS(nodep->classOrPackagep(), Package))
                            nodep->classOrPackagep(classp);
                    } లేకపోతే ఒకవేళ (nodep->name() == "randomize") {
                        // A std::randomize resolved in V3Width
                        nodep->classOrPackagep(v3Global.rootp()->stdPackagep());
                    } లేకపోతే {
                        nodep->v3error("Calling implicit class method "
                                       << nodep->prettyNameQ() << " without being under class");
                        nodep->replaceWith(కొత్త AstConst{nodep->fileline(), 0});
                        VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
                        తిరిగి;
                    }
                } లేకపోతే ఒకవేళ (nodep->dotted() == "") {
                    ఒకవేళ (nodep->pli()) {
                        ఒకవేళ (v3Global.opt.bboxSys()) {
                            Ast_కణుపు* newp;
                            ఒకవేళ (VN_IS(nodep, FuncRef)) {
                                newp = కొత్త AstConst{nodep->fileline(), AstConst::All0{}};
                            } లేకపోతే {
                                Ast_కణుపు_సమాసం* outp = శూన్యసూచిక;
                                అయితే (AstArg* స్థిరరాశి argp = nodep->argsp()) {
                                    outp = Ast_కణుపు::కూడు_తదుపరి(outp, argp->exprp()->unlink_Fr_వెనుక());
                                    VL_DO_DANGLING(నెట్టు_Deletep(argp->unlink_Fr_వెనుక()), argp);
                                }
                                newp = కొత్త AstSysIgnore{nodep->fileline(), outp};
                                newp->dtypep(nodep->dtypep());
                            }
                            nodep->replaceWith(newp);
                            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
                            తిరిగి;
                        } లేకపోతే {
                            VSpellCheck speller;
                            V3Parse::candidatePli(&speller);
                            స్థిరరాశి string suggest = speller.ఉత్తమ_Candidate_సందేశం(nodep->prettyName());
                            nodep->v3error(
                                "Unsupported or unknown PLI call: "
                                << nodep->prettyNameQ() << '\n'
                                << (suggest.ఖాళీలు() ? "" : nodep->warn_మరిన్ని() + suggest));
                        }
                    } లేకపోతే {
                        స్థిరరాశి string suggest = m_statep->suggestSymFallback(
                            dotSymp, nodep->name(), Link_కణుపు_Matcher_FTask{});
                        nodep->v3error("Can't find definition of task/function: "
                                       << nodep->prettyNameQ() << '\n'
                                       << (suggest.ఖాళీలు() ? "" : nodep->warn_మరిన్ని() + suggest));
                    }
                } లేకపోతే {
                    స్థిరరాశి string suggest = m_statep->suggestSymFallback(dotSymp, nodep->name(),
                                                                        Link_కణుపు_Matcher_FTask{});
                    nodep->v3error("Can't find definition of "
                                   << Ast_కణుపు::prettyNameQ(baddot) << " in dotted task/function: '"
                                   << nodep->dotted() + "." + nodep->prettyName() << "'\n"
                                   << (suggest.ఖాళీలు() ? "" : nodep->warn_మరిన్ని() + suggest) << '\n'
                                   << nodep->warnContextPrimary()
                                   << okSymp->cellErrorScopes(nodep));
                }
            }
        }
    }
    శూన్య సందర్శించు(Ast_Sel_బిట్టు* nodep) override {
        ఒకవేళ (nodep->user3_సమితి_Once()) తిరిగి;
        LINKDOT_సందర్శించు_ప్రారంభం();
        iterate_And_తదుపరి_Null(nodep->fromp());
        ఒకవేళ (m_ds.m_unresolvedClass) {
            UASSERT_OBJ(m_ds.m_dotPos != DP_SCOPE && m_ds.m_dotPos != DP_మొదటి, nodep,
                        "Object of unresolved class on scope position in dotted reference");
            // The index does not depend on the class, and V3Param must see its class references
            symIterateNull(nodep->bitp(), m_curSymp);
            symIterateNull(nodep->attrp(), m_curSymp);
            తిరిగి;
        }
        ఒకవేళ (m_ds.m_dotPos == DP_SCOPE
            || m_ds.m_dotPos
                   == DP_మొదటి) {  // Already under dot, so this is {modulepart} DOT {modulepart}
            UINFO(9, indent() << "deferring until after a V3Param pass: " << nodep);
            m_ds.m_dot_పాఠ్యం += "__BRA__??__KET__";
            m_ds.m_unresolvedCell = నిజం;
            // And pass up m_ds.m_dotText
        }
        // Pass dot state down to only fromp()
        iterate_And_తదుపరి_Null(nodep->fromp());
        symIterateNull(nodep->bitp(), m_curSymp);
        symIterateNull(nodep->attrp(), m_curSymp);
        ఒకవేళ (m_ds.m_unresolvedCell && (m_ds.m_dotPos == DP_SCOPE || m_ds.m_dotPos == DP_మొదటి)) {
            Ast_కణుపు_సమాసం* స్థిరరాశి exprp = nodep->bitp()->unlink_Fr_వెనుక();
            ఒకవేళ (AstCellArrayRef* స్థిరరాశి fromArrRefp = VN_CAST(nodep->fromp(), CellArrayRef)) {
                // Multi-dim iface array access: append this select to the existing chain
                fromArrRefp->కూడు_Selp(exprp);
                AstCellArrayRef* స్థిరరాశి movedp = fromArrRefp->unlink_Fr_వెనుక();
                nodep->replaceWith(movedp);
                VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
            } లేకపోతే {
                AstCellArrayRef* స్థిరరాశి newp
                    = కొత్త AstCellArrayRef{nodep->fileline(), nodep->fromp()->name(), exprp};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
            }
        }
    }
    శూన్య సందర్శించు(Ast_కణుపు_Pre_Sel* nodep) override {
        // Excludes simple AstSelBit, see above
        ఒకవేళ (nodep->user3_సమితి_Once()) తిరిగి;
        LINKDOT_సందర్శించు_ప్రారంభం();
        ఒకవేళ (m_ds.m_dotPos == DP_SCOPE
            || m_ds.m_dotPos
                   == DP_మొదటి) {  // Already under dot, so this is {modulepart} DOT {modulepart}
            nodep->v3error("Syntax error: Range ':', '+:' etc are not allowed in the instance "
                           "part of a dotted reference");
            m_ds.m_dotErr = నిజం;
            తిరిగి;
        }
        Ast_కణుపు_DType* packedArrayDtp = శూన్యసూచిక;  // Datatype reference for packed array
        {
            VL_RESTORER(m_packedArrayDtp);
            iterate_And_తదుపరి_Null(nodep->fromp());
            symIterateNull(nodep->rhsp(), m_curSymp);
            symIterateNull(nodep->thsp(), m_curSymp);

            ఒకవేళ (m_packedArrayDtp) {
                AstRange* స్థిరరాశి newRangep
                    = కొత్త AstRange(nodep->fileline(), nodep->rhsp()->unlink_Fr_వెనుక(),
                                   nodep->thsp()->unlink_Fr_వెనుక());
                AstPackArrayDType* newArrayTypep
                    = కొత్త AstPackArrayDType(nodep->fileline(), m_packedArrayDtp, newRangep);
                newArrayTypep->బిడ్డ_DTypep(m_packedArrayDtp);
                newArrayTypep->refDTypep(శూన్యసూచిక);
                ఒకవేళ (VN_IS(nodep->backp(), Sel_వెలికితీయి)) {
                    packedArrayDtp = newArrayTypep;
                } లేకపోతే {
                    replaceWithCheckBreak(nodep, newArrayTypep);
                    VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
                }
            } లేకపోతే {
                ఒకవేళ (nodep->attrp()) {
                    Ast_కణుపు* స్థిరరాశి attrp = nodep->attrp()->unlink_Fr_వెనుక();
                    VL_DO_DANGLING(attrp->delete_వృక్షం(), attrp);
                }
                Ast_కణుపు* స్థిరరాశి basefromp = nodep->పునాది_Fromp(అబద్ధం);
                ఒకవేళ (VN_IS(basefromp, Replicate)) {
                    // From {...}[...] syntax in IEEE 2017
                    ఒకవేళ (basefromp) UINFO(9, indent() << " Related node: " << basefromp);
                } లేకపోతే {
                    nodep->attrp(కొత్త Ast_Attr_యొక్క{nodep->fileline(), VAttrType::VAR_పునాది,
                                               basefromp->clone_వృక్షం(అబద్ధం)});
                }
            }
        }

        ఒకవేళ (packedArrayDtp) m_packedArrayDtp = packedArrayDtp;
    }
    శూన్య సందర్శించు(AstMemberSel* nodep) override {
        // checkNoDot not appropriate, can be under a dot
        LINKDOT_సందర్శించు_ప్రారంభం();
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(Ast_Gen_ఖండం* nodep) override {
        LINKDOT_సందర్శించు_ప్రారంభం();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        {
            VL_RESTORER(m_curSymp);
            VL_RESTORER_COPY(m_ds);
            ఒకవేళ (nodep->name() != "") {
                m_ds.m_dotSymp = m_curSymp = m_statep->get_కణుపు_Sym(nodep);
                UINFO(5, indent() << "cur=se" << cvt_గమ్యం_Hex(m_curSymp));
            }
            iterateChildren(nodep);
        }
        UINFO(5, indent() << "cur=se" << cvt_గమ్యం_Hex(m_curSymp));
    }
    శూన్య సందర్శించు(Ast_కణుపు_ఖండం* nodep) override {
        LINKDOT_సందర్శించు_ప్రారంభం();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        {
            VL_RESTORER(m_curSymp);
            VL_RESTORER_COPY(m_ds);
            ఒకవేళ (nodep->name() != "") {
                m_ds.m_dotSymp = m_curSymp = m_statep->get_కణుపు_Sym(nodep);
                UINFO(5, indent() << "cur=se" << cvt_గమ్యం_Hex(m_curSymp));
            }
            iterateChildren(nodep);
        }
        UINFO(5, indent() << "cur=se" << cvt_గమ్యం_Hex(m_curSymp));
    }
    శూన్య సందర్శించు(Ast_కణుపు_FTask* nodep) override {
        LINKDOT_సందర్శించు_ప్రారంభం();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        ఒకవేళ (nodep->isExternDef() && !nodep->didProto()) {
            nodep->didProto(నిజం);
            ఒకవేళ (స్థిరరాశి VSymEnt* స్థిరరాశి foundp
                = m_curSymp->కనుగొను_Id_Fallback("extern " + nodep->name())) {
                స్థిరరాశి Ast_కణుపు_FTask* స్థిరరాశి protop = VN_AS(foundp->nodep(), కణుపు_FTask);
                // Copy specifiers.
                // External definition cannot have any specifiers, so no value will be overwritten.
                nodep->isHideLocal(protop->isHideLocal());
                nodep->isHideProtected(protop->isHideProtected());
                nodep->isStatic(protop->isStatic());
                nodep->isVirtual(protop->isVirtual());
                nodep->lifetime(protop->lifetime());
                // Always add an FUNC_RETURN_PROTO even if task, so we can locate the prototype
                nodep->కూడు_Stmtsp(కొత్త Ast_Attr_యొక్క{protop->fileline(), VAttrType::FUNC_RETURN_PROTO,
                                               protop->fvarp()
                                                   ? protop->fvarp()->clone_వృక్షం(అబద్ధం)
                                                   : కొత్త AstVoidDType{protop->fileline()}});
                కొరకు (Ast_కణుపు* stmtp = protop->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    ఒకవేళ (AstVar* స్థిరరాశి portp = VN_CAST(stmtp, Var)) {
                        Ast_Attr_యొక్క* స్థిరరాశి attrp
                            = కొత్త Ast_Attr_యొక్క{protop->fileline(), VAttrType::FUNC_ARG_PROTO,
                                            portp->subDTypep()->clone_వృక్షం(అబద్ధం)};
                        attrp->name(portp->name());
                        nodep->కూడు_Stmtsp(attrp);
                    }
                }
            } లేకపోతే {
                nodep->v3error("extern not found that declares " + nodep->prettyNameQ());
            }
        }
        ఒకవేళ (nodep->isExternProto()) {
            ఒకవేళ (!m_curSymp->కనుగొను_Id_Fallback(nodep->name())) {
                nodep->v3warn(PROTOTYPEMIS,
                              "Definition not found for extern prototype " + nodep->prettyNameQ());
            }
        }
        VL_RESTORER(m_curSymp);
        VL_RESTORER(m_ftaskp);
        VL_RESTORER(m_explicitSuperNewp);
        {
            m_ftaskp = nodep;
            m_ds.m_dotSymp = m_curSymp = m_statep->get_కణుపు_Sym(nodep);
            స్థిరరాశి తార్కిక isNew = nodep->name() == "new";
            ఒకవేళ (isNew) m_explicitSuperNewp = శూన్యసూచిక;
            iterateChildren(nodep);
            ఒకవేళ (isNew) {
                స్థిరరాశి AstClassExtends* స్థిరరాశి classExtendsp = VN_AS(m_modp, Class)->extendsp();
                ఒకవేళ (m_explicitSuperNewp && !m_explicitSuperNewp->isImplicit() && classExtendsp
                    && classExtendsp->argsp()) {
                    m_explicitSuperNewp->v3error(
                        "Explicit super.new not allowed with class "
                        "extends arguments (IEEE 1800-2023 8.17)\n"
                        << m_explicitSuperNewp->warn_మరిన్ని() << "... Suggest remove super.new\n"
                        << m_explicitSuperNewp->warnContextPrimary() << '\n'
                        << classExtendsp->argsp()->warnOther()
                        << "... Location of extends argument(s)\n"
                        << classExtendsp->argsp()->warnContextSecondary());
                }
                ఒకవేళ (classExtendsp && classExtendsp->classOrNullp()) {
                    ఒకవేళ (!m_explicitSuperNewp && m_statep->forParamed()
                        && !VN_AS(classExtendsp->బిడ్డ_DTypep()->skipRefp(), ClassRefDType)
                                ->classp()
                                ->isInterfaceClass()) {
                        Ast_కణుపు_Stmt* స్థిరరాశి superNewp
                            = కూడు_Implicit_Super_New_Call(VN_AS(nodep, Func), classExtendsp);
                        UINFO(9, "created super new " << superNewp);
                        iterate(superNewp);
                    }
                }
            }
        }
        m_ds.m_dotSymp = VL_RESTORER_PREV(m_curSymp);
    }
    శూన్య సందర్శించు(Ast_కణుపు_Foreach* nodep) override {
        LINKDOT_సందర్శించు_ప్రారంభం();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        symIterateChildren(nodep, m_statep->get_కణుపు_Sym(nodep));
    }
    శూన్య సందర్శించు(AstRandSequence* nodep) override {
        LINKDOT_సందర్శించు_ప్రారంభం();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        ఒకవేళ (!nodep->ప్రారంభం().ఖాళీలు())
            nodep->prodp(కనుగొను_Prod(nodep, m_statep->get_కణుపు_Sym(nodep), nodep->ప్రారంభం()));
        symIterateChildren(nodep, m_statep->get_కణుపు_Sym(nodep));
    }
    శూన్య సందర్శించు(AstRSProd* nodep) override {
        LINKDOT_సందర్శించు_ప్రారంభం();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        symIterateChildren(nodep, m_statep->get_కణుపు_Sym(nodep));
    }
    శూన్య సందర్శించు(Ast_RSProd_అంశం* nodep) override {
        LINKDOT_సందర్శించు_ప్రారంభం();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        nodep->prodp(కనుగొను_Prod(nodep, m_curSymp, nodep->name()));
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(AstCoverWith* nodep) override {
        LINKDOT_సందర్శించు_ప్రారంభం();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        iterate_And_తదుపరి_Null(nodep->subp());
        // The type of 'item' is that of the coverpoint expression, which may name 'item' too
        iterate_And_తదుపరి_Null(nodep->itemp()->బిడ్డ_DTypep());
        VL_RESTORER(m_curSymp);
        m_ds.m_dotSymp = m_curSymp = m_statep->get_కణుపు_Sym(nodep);
        iterate_And_తదుపరి_Null(nodep->itemp());
        iterate_And_తదుపరి_Null(nodep->filterp());
        m_ds.m_dotSymp = VL_RESTORER_PREV(m_curSymp);
    }
    శూన్య సందర్శించు(AstCoverCrossWith* nodep) override {
        LINKDOT_సందర్శించు_ప్రారంభం();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        iterate_And_తదుపరి_Null(nodep->selectp());
        iterate_And_తదుపరి_Null(nodep->countp());
        // The type of a cross item is that of its coverpoint's expression, which may name items
        iterate_And_తదుపరి_Null(nodep->varsp());
        VL_RESTORER(m_curSymp);
        m_ds.m_dotSymp = m_curSymp = m_statep->get_కణుపు_Sym(nodep);
        iterate_And_తదుపరి_Null(nodep->filterp());
        m_ds.m_dotSymp = VL_RESTORER_PREV(m_curSymp);
    }
    శూన్య సందర్శించు(AstWith* nodep) override {
        LINKDOT_సందర్శించు_ప్రారంభం();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        ఒకవేళ (m_ds.m_unresolvedClass) తిరిగి;
        VL_RESTORER(m_curSymp);
        VL_RESTORER(m_currentWithp);
        VL_RESTORER_COPY(m_restricted_పేర్లు_వాడిన);
        {
            m_ds.m_dotSymp = m_curSymp = m_statep->get_కణుపు_Sym(nodep);
            m_currentWithp = nodep;
            // Validate the identifier_list once per AstWith (m_validated travels
            // with cloneTree, so cloned witnesses skip the redundant check).
            స్థిరరాశి తార్కిక మొదటి_సందర్శించు = !nodep->validated();
            ఒకవేళ (మొదటి_సందర్శించు) {
                nodep->validated(నిజం);
                m_restricted_పేర్లు_వాడిన.clear();
                // IEEE 1800-2023 18.7: each name in the identifier_list must designate
                // a member of the randomize() target class.
                ఒకవేళ (nodep->restricted() && m_randSymp) {
                    కొరకు (స్థిరరాశి std::string& n : nodep->restricted_పేర్లు()) {
                        ఒకవేళ (!m_randSymp->కనుగొను_Id_Flat(n)) {
                            nodep->v3error("Identifier '"
                                           << n
                                           << "' in 'with (...)' identifier list is not a"
                                              " member of the randomize() target class"
                                              " (IEEE 1800-2023 18.7)");
                            m_restricted_పేర్లు_వాడిన.చొప్పించు(n);
                        }
                    }
                }
            }
            iterateChildren(nodep);
            // Flag names that the user listed but never referenced in the constraint.
            ఒకవేళ (మొదటి_సందర్శించు && nodep->restricted()) {
                కొరకు (స్థిరరాశి std::string& n : nodep->restricted_పేర్లు()) {
                    ఒకవేళ (!m_restricted_పేర్లు_వాడిన.లెక్క(n)) {
                        nodep->v3warn(వాడని_2, "Identifier '"
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
    శూన్య సందర్శించు(AstLambdaArgRef* nodep) override {
        LINKDOT_సందర్శించు_ప్రారంభం();
        UINFO(5, indent() << "visit " << nodep);
        // No checknodot(nodep), visit(AstScope) will check for LambdaArgRef
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(AstClassExtends* nodep) override {
        // Resolve the symbol and get the class.
        // If it is a parameterized case, the class will be resolved after V3Param.cpp
        ఒకవేళ (nodep->user3_సమితి_Once()) తిరిగి;
        // If the class is resolved, there is nothing more to do
        ఒకవేళ (nodep->classOrNullp()) తిరిగి;
        LINKDOT_సందర్శించు_ప్రారంభం();
        ఒకవేళ (m_statep->forPrimary()) {
            ఒకవేళ (nodep->బిడ్డ_DTypep()) తిరిగి;
            Ast_కణుపు* cprp = nodep->classOrPkgsp();
            VSymEnt* lookSymp = m_curSymp;
            ఒకవేళ (AstDot* స్థిరరాశి dotp = VN_CAST(cprp, Dot)) {
                dotp->user3(నిజం);
                ఒకవేళ (AstClassOrPackageRef* lookNodep = VN_CAST(dotp->lhsp(), ClassOrPackageRef)) {
                    iterate(lookNodep);
                    cprp = dotp->rhsp();
                    VSymEnt* స్థిరరాశి foundp = m_statep->resolveClassOrPackage(
                        lookSymp, lookNodep, నిజం, అబద్ధం, nodep->verilogKwd());
                    ఒకవేళ (!foundp) {
                        VL_DO_DANGLING(నెట్టు_Deletep(nodep->unlink_Fr_వెనుక()), nodep);
                        తిరిగి;
                    }
                    UASSERT_OBJ(lookNodep->classOrPackageSkipp(), nodep, "Bad package link");
                    lookSymp = m_statep->get_కణుపు_Sym(lookNodep->classOrPackageSkipp());
                } లేకపోతే {
                    dotp->lhsp()->v3error("Attempting to extend"  // LCOV_EXCL_LINE
                                          " using non-class under dot");
                }
            }
            AstClassOrPackageRef* స్థిరరాశి cpackagerefp = VN_CAST(cprp, ClassOrPackageRef);
            ఒకవేళ (VL_UNCOVERABLE(!cpackagerefp)) {
                // Linking the extend gives an error before this is hit
                nodep->v3error("Attempting to extend using non-class");  // LCOV_EXCL_LINE
                VL_DO_DANGLING(నెట్టు_Deletep(nodep->unlink_Fr_వెనుక()), nodep);
                తిరిగి;
            }
            VSymEnt* స్థిరరాశి foundp = m_statep->resolveClassOrPackage(lookSymp, cpackagerefp, నిజం,
                                                                    నిజం, nodep->verilogKwd());
            ఒకవేళ (!foundp) {
                VL_DO_DANGLING(నెట్టు_Deletep(nodep->unlink_Fr_వెనుక()), nodep);
                తిరిగి;
            }
            ఒకవేళ (AstClass* స్థిరరాశి classp = VN_CAST(foundp->nodep(), Class)) {
                AstPin* paramsp = cpackagerefp->paramsp();
                ఒకవేళ (paramsp) {
                    paramsp = paramsp->clone_వృక్షం(నిజం);
                    nodep->parameterized(నిజం);
                }
                nodep->బిడ్డ_DTypep(కొత్త AstClassRefDType{nodep->fileline(), classp, paramsp});
                // Link pins
                iterate(nodep->బిడ్డ_DTypep());
            } లేకపోతే ఒకవేళ (AstParamTypeDType* స్థిరరాశి paramp
                       = VN_CAST(foundp->nodep(), ParamTypeDType)) {
                AstRefDType* స్థిరరాశి refParamp = కొత్త AstRefDType{nodep->fileline(), paramp->name()};
                refParamp->refDTypep(paramp);
                nodep->బిడ్డ_DTypep(refParamp);
                nodep->parameterized(నిజం);
            } లేకపోతే ఒకవేళ (AstTypedef* స్థిరరాశి typedefp = VN_CAST(foundp->nodep(), Typedef)) {
                Ast_కణుపు_DType* స్థిరరాశి unwrappedp = typedefp->subDTypep()->skipRefp();
                ఒకవేళ (AstClassRefDType* స్థిరరాశి classRefp = VN_CAST(unwrappedp, ClassRefDType)) {
                    AstPin* paramsp = cpackagerefp->paramsp();
                    ఒకవేళ (!paramsp && classRefp->paramsp()) {
                        // No explicit #(...) on extends clause; carry over the
                        // typedef's type-parameter pins so V3Param sees them.
                        paramsp = classRefp->paramsp()->clone_వృక్షం(నిజం);
                        nodep->parameterized(నిజం);
                    } లేకపోతే ఒకవేళ (paramsp) {
                        paramsp = paramsp->clone_వృక్షం(నిజం);
                        nodep->parameterized(నిజం);
                    }
                    nodep->బిడ్డ_DTypep(
                        కొత్త AstClassRefDType{nodep->fileline(), classRefp->classp(), paramsp});
                    iterate(nodep->బిడ్డ_DTypep());
                } లేకపోతే {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: " << foundp->nodep()->prettyTypeName()
                                                  << " in 'class extends'");
                    తిరిగి;
                }
            } లేకపోతే {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: " << foundp->nodep()->prettyTypeName()
                                                             << " in 'class extends'");
                తిరిగి;
            }
            ఒకవేళ (!nodep->బిడ్డ_DTypep()) nodep->v3error("Attempting to extend using non-class");
            nodep->classOrPkgsp()->unlink_Fr_వెనుక()->delete_వృక్షం();
        } లేకపోతే {
            // Probably a parameter
            ఒకవేళ (AstRefDType* స్థిరరాశి refp = VN_CAST(nodep->బిడ్డ_DTypep(), RefDType)) {
                ఒకవేళ (AstClassRefDType* classRefp = VN_CAST(refp->skipRefp(), ClassRefDType)) {
                    // Resolved to a class reference.
                    refp->replaceWith(classRefp->clone_వృక్షం(అబద్ధం));
                    VL_DO_DANGLING(నెట్టు_Deletep(refp), refp);
                } లేకపోతే {
                    // Unable to resolve the ref type to a class reference.
                    // Get the value of type parameter passed to the class instance,
                    // to print the helpful error message.
                    స్థిరరాశి Ast_కణుపు_DType* typep = refp->refDTypep();
                    అయితే (నిజం) {
                        ఒకవేళ (స్థిరరాశి AstParamTypeDType* స్థిరరాశి atypep
                            = VN_CAST(typep, ParamTypeDType)) {
                            typep = atypep->subDTypep();
                            కొనసాగు;
                        }
                        ఒకవేళ (స్థిరరాశి AstRequireDType* స్థిరరాశి atypep = VN_CAST(typep, RequireDType)) {
                            typep = atypep->subDTypep();
                            కొనసాగు;
                        }
                        ఆపు;
                    }
                    typep->v3error("Attempting to extend using non-class");
                }
            }
        }
    }
    శూన్య సందర్శించు(AstClass* nodep) override {
        ఒకవేళ (nodep->user3_సమితి_Once()) తిరిగి;
        LINKDOT_సందర్శించు_ప్రారంభం();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        VL_RESTORER(m_curSymp);
        VL_RESTORER(m_modSymp);
        VL_RESTORER(m_modp);
        VL_RESTORER_COPY(m_if_Class_Imp_పేర్లు);
        VL_RESTORER_COPY(m_ifForwards);
        VL_RESTORER(m_insideClassExtParam);
        {
            m_ifForwards.clear();
            m_ds.init(m_curSymp);
            m_insideClassExtParam = nodep->isCovergroup() && nodep->covergroupEnclosingClassp()
                                    && m_extendsParam.కనుగొను(nodep->covergroupEnclosingClassp())
                                           != m_extendsParam.చివర();
            // Until overridden by a SCOPE
            m_ds.m_dotSymp = m_curSymp = m_modSymp = m_statep->get_కణుపు_Sym(nodep);
            m_modp = nodep;
            పూర్ణాంకం తదుపరి = 0;
            కొరకు (AstClassExtends* cextp = nodep->extendsp(); cextp;
                 cextp = VN_AS(cextp->nextp(), ClassExtends)) {
                // Replace abstract reference with hard pointer
                // Will need later resolution when deal with parameters
                ఒకవేళ (++తదుపరి == 2 && !nodep->isInterfaceClass() && !cextp->isImplements()) {
                    cextp->v3error("Multiple inheritance illegal on non-interface classes"
                                   " (IEEE 1800-2023 8.13)");
                }
                iterate(cextp);
                ఒకవేళ (m_statep->forPrimary()) {
                    ఒకవేళ (cextp->parameterized()) {
                        // Parameters in extends statement.
                        // The class can't be resolved in the current pass.
                        m_extendsParam.చొప్పించు(nodep);
                        m_insideClassExtParam = నిజం;
                    }
                    ఒకవేళ (AstClassRefDType* స్థిరరాశి classRefp
                        = VN_CAST(cextp->బిడ్డ_DTypep(), ClassRefDType)) {
                        AstClass* స్థిరరాశి classp = classRefp->classp();
                        ఒకవేళ (classp != nodep) iterate(classp);
                        ఒకవేళ (m_extendsParam.కనుగొను(classp) != m_extendsParam.చివర()) {
                            // One of its super classes has parameters in extends statement.
                            // Some links may not be resolved in the first pass.
                            m_extendsParam.చొప్పించు(nodep);
                            m_insideClassExtParam = నిజం;
                        }
                    }
                }

                ఒకవేళ (AstClass* స్థిరరాశి పునాది_Classp = cextp->classOrNullp()) {
                    // Already converted. Update symbol table to link unlinked members.
                    // Base class has to be visited in a case if its extends statement
                    // needs to be handled. Recursive inheritance was already checked.
                    // Must be here instead of in LinkDotParam to handle
                    // "class (type T) extends T".
                    ఒకవేళ (పునాది_Classp == nodep) {
                        cextp->v3error("Attempting to extend class " << nodep->prettyNameQ()
                                                                     << " from itself");
                    } లేకపోతే ఒకవేళ (extendsClass(పునాది_Classp, nodep)) {
                        cextp->v3error("Attempting to extend class "
                                       << nodep->prettyNameQ() << " from "
                                       << పునాది_Classp->prettyNameQ()
                                       << ", which circularly inherits from it");
                    } లేకపోతే ఒకవేళ (cextp->isImplements() && !పునాది_Classp->isInterfaceClass()) {
                        cextp->v3error("Attempting to implement from non-interface class "
                                       << పునాది_Classp->prettyNameQ() << '\n'
                                       << "... Suggest use 'extends'");
                    } లేకపోతే ఒకవేళ (!cextp->isImplements() && !nodep->isInterfaceClass()
                               && పునాది_Classp->isInterfaceClass()) {
                        cextp->v3error("Attempting to extend from interface class "
                                       << పునాది_Classp->prettyNameQ() << '\n'
                                       << "... Suggest use 'implements'");
                    }
                    పునాది_Classp->isExtended(నిజం);
                    nodep->isExtended(నిజం);
                    iterate(పునాది_Classp);
                    importSymbolsFromExtended(nodep, cextp);
                    కొనసాగు;
                }
            }
            m_ds.m_dotSymp = m_curSymp;

            iterateChildren(nodep);
            కొరకు (స్థిరరాశి స్వయంచాలక& itr : m_ifForwards) {
                స్థిరరాశి VSymEnt* స్థిరరాశి implSymp = itr.రెండవ;
                కూడు_Interface_Forward(nodep, VN_AS(implSymp->nodep(), కణుపు_FTask),
                                    VN_AS(implSymp->parentp()->nodep(), Class));
            }
        }
        // V3Width when determines types needs to find enum values and such
        // so add members pointing to appropriate enum values
        {
            VMemberMap memberMap;
            కొరకు (VSymEnt::const_iterator it = m_curSymp->begin(); it != m_curSymp->చివర(); ++it) {
                Ast_కణుపు* స్థిరరాశి itemp = it->రెండవ->nodep();
                ఒకవేళ (!memberMap.కనుగొను_Member(nodep, it->మొదటి)) {
                    ఒకవేళ (Ast_Enum_అంశం* స్థిరరాశి aitemp = VN_CAST(itemp, Enum_అంశం)) {
                        Ast_Enum_అంశం_Ref* స్థిరరాశి newp = కొత్త Ast_Enum_అంశం_Ref{
                            aitemp->fileline(), aitemp, it->రెండవ->classOrPackagep()};
                        UINFO(8, indent()
                                     << "Class import noderef '" << it->మొదటి << "' " << newp);
                        nodep->కూడు_Membersp(newp);
                        memberMap.చొప్పించు(nodep, newp);
                    }
                }
            }
        }
        m_ds.m_dotSymp = VL_RESTORER_PREV(m_curSymp);
    }

    // Collect the ClassOrPackageRefs in a chained scope operand in source order.
    // The grammar builds a scope operand only from packageClassScopeItem, which
    // always yields a ClassOrPackageRef, nested in Dots, so nothing else appears.
    స్థిర శూన్య collectScopeRefs(Ast_కణుపు* nodep, std::సదిశ<AstClassOrPackageRef*>& refps) {
        ఒకవేళ (AstClassOrPackageRef* స్థిరరాశి refp = VN_CAST(nodep, ClassOrPackageRef)) {
            refps.నెట్టు_వెనుక(refp);
            తిరిగి;
        }
        AstDot* స్థిరరాశి dotp = VN_CAST(nodep, Dot);
        UASSERT_OBJ(dotp, nodep, "Non-scope node in package/class scope operand");
        collectScopeRefs(dotp->lhsp(), refps);
        collectScopeRefs(dotp->rhsp(), refps);
    }

    // Resolve a chained class-scope operand (`pkg::outer::inner` in
    // `pkg::outer::inner::t`) and reduce it to its innermost ClassOrPackageRef.
    // Returns false, having reported, if a segment could not be resolved.
    తార్కిక reduceScopeDot(AstRefDType* nodep, AstDot* scopeDotp) {
        std::సదిశ<AstClassOrPackageRef*> refps;
        collectScopeRefs(scopeDotp, refps);

        VSymEnt* scopeSymp = m_ds.m_dotSymp;
        కొరకు (కొలత_t i = 0; i < refps.కొలత(); ++i) {
            AstClassOrPackageRef* స్థిరరాశి refp = refps[i];
            ఒకవేళ (!refp->classOrPackageSkipp() && !refp->classOrPackageNodep()
                && !m_statep->resolveClassOrPackage(scopeSymp, refp, i == 0, అబద్ధం,
                                                    "class/package reference")) {
                తిరిగి అబద్ధం;  // Error already reported
            }
            ఒకవేళ (i + 1 == refps.కొలత()) ఆపు;
            // A middle segment that resolved to a node with no reachable module
            // (e.g. a typedef alias of a not-yet-specialized parameterized class)
            // gives nothing to look the next segment up in.
            Ast_కణుపు_Module* స్థిరరాశి modp = refp->classOrPackageSkipp();
            ఒకవేళ (!modp) {
                refp->v3warn(E_UNSUPPORTED, "Unsupported: Multiple '::' package/class reference");
                తిరిగి అబద్ధం;
            }
            scopeSymp = m_statep->get_కణుపు_Sym(modp);
        }
        AstClassOrPackageRef* స్థిరరాశి innerp = refps.వెనుక();
        innerp->unlink_Fr_వెనుక();
        VL_DO_DANGLING(నెట్టు_Deletep(scopeDotp->unlink_Fr_వెనుక()), scopeDotp);
        nodep->classOrPackageOpp(innerp);
        తిరిగి నిజం;
    }

    శూన్య సందర్శించు(AstRefDType* nodep) override {

        ఒకవేళ (స్వయంచాలక* స్థిరరాశి typeOfp = nodep->typeofp()) {
            iterate(typeOfp);
            // After iteration, typeofp() may have been replaced with a resolved type
            // (e.g., DOT expression like if0.rq_t resolved to REFDTYPE)
            // If it's now a NodeDType, replace this RefDType with it
            ఒకవేళ (Ast_కణుపు_DType* స్థిరరాశి resolvedDTypep = VN_CAST(nodep->typeofp(), కణుపు_DType)) {
                UINFO(9,
                      indent() << "iface capture typeofp resolved to dtype, replacing RefDType\n");
                resolvedDTypep->unlink_Fr_వెనుక();
                nodep->replaceWith(resolvedDTypep);
                VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
                // Originally (PR #6637) this block re-captured a resolved
                // RefDType into the iface-capture ledger when user2p held a
                // Cell, to ensure re-resolution during the paramed pass.
                // In practice, the typeofp-resolved RefDType never carries a
                // Cell in user2p: the capture happens earlier when the
                // typedef is first linked.
                ఒకవేళ (V3LinkDotIfaceCapture::enabled()) {
                    ఒకవేళ (AstRefDType* స్థిరరాశి resolvedRefp = VN_CAST(resolvedDTypep, RefDType)) {
                        ఒకవేళ (VL_UNCOVERABLE(
                                VN_IS(resolvedRefp->user2p(), Cell))) {  // LCOV_EXCL_LINE
                            resolvedRefp->v3fatalSrc(  // LCOV_EXCL_LINE
                                "typeofp resolved RefDType has Cell in user2p;"
                                " expected to be captured already");
                        }
                    }
                }
            }
            తిరిగి;
        }

        // Resolve its reference
        ఒకవేళ (m_statep->forParamed() && nodep->user3()) {
            ఒకవేళ (V3LinkDotIfaceCapture::enabled() && nodep->user2p()) {
                UINFO(9, indent() << "iface capture clear user3 for captured typedef name="
                                  << nodep->name() << " cell=" << nodep->user2p());
            }
            nodep->user3(అబద్ధం);
        }
        ఒకవేళ (nodep->user3_సమితి_Once()) {
            ఒకవేళ (V3LinkDotIfaceCapture::enabled() && nodep->user2p()) {
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
            ఒకవేళ (VL_UNCOVERABLE(m_statep->forParamed() && nodep->classOrPackageOpp())) {
                nodep->v3fatalSrc(  // LCOV_EXCL_LINE
                    "RefDType user3 revisit with classOrPackageOpp in paramed pass");
            }
            తిరిగి;
        }
        LINKDOT_సందర్శించు_ప్రారంభం();
        UINFO(5, indent() << "visit " << nodep);
        ఒకవేళ (nodep->classOrPackageOpp()) {
            // Resolve a chained scope (`pkg::cls::t`, `pkg::cls#(P)::t`) from outside in.
            // On failure reduceScopeDot has reported, so stop before using the operand.
            ఒకవేళ (AstDot* స్థిరరాశి scopeDotp = VN_CAST(nodep->classOrPackageOpp(), Dot)) {
                ఒకవేళ (!reduceScopeDot(nodep, scopeDotp)) తిరిగి;
            }
            // Re-read: reduceScopeDot may have replaced the operand
            Ast_కణుపు* స్థిరరాశి cpackagep = nodep->classOrPackageOpp();
            UASSERT_OBJ(!VN_IS(cpackagep, Dot), cpackagep,
                        "Package/class scope operand should be reduced by now");
            ఒకవేళ (AstClassOrPackageRef* స్థిరరాశి cpackagerefp = VN_CAST(cpackagep, ClassOrPackageRef)) {
                iterate(cpackagerefp);
                స్థిరరాశి AstClass* స్థిరరాశి clsp = VN_CAST(cpackagerefp->classOrPackageNodep(), Class);
                ఒకవేళ (clsp && clsp->hasGParam()) {
                    // Unable to link before the instantiation of parameter classes.
                    // The class reference node still has to be visited now to later link
                    // parameters.
                    iterate(cpackagep);
                    తిరిగి;
                }
                // Defer non-typedef references through typedef aliases of parameterized classes.
                ఒకవేళ (m_statep->forPrimary() && !VN_IS(nodep->backp(), Typedef)
                    && isParamedClassRef(cpackagerefp)) {
                    iterate(cpackagep);
                    తిరిగి;
                }

                స్థిరరాశి తార్కిక doDefaultTypedef = !(m_resolvingTypedef && m_statep->forPrimary());
                ఒకవేళ (!cpackagerefp->classOrPackageSkipp(doDefaultTypedef)
                    && !cpackagerefp->classOrPackageNodep()) {
                    VSymEnt* స్థిరరాశి foundp = m_statep->resolveClassOrPackage(
                        m_ds.m_dotSymp, cpackagerefp, నిజం, అబద్ధం, "class/package reference");
                    ఒకవేళ (!foundp) తిరిగి;
                }
                nodep->classOrPackagep(cpackagerefp->classOrPackageSkipp(doDefaultTypedef));
                ఒకవేళ (!VN_IS(nodep->classOrPackagep(), Class)
                    && !VN_IS(nodep->classOrPackagep(), Package)) {
                    ఒకవేళ (m_statep->forPrimary()) {
                        // It may be a type that comes from parameter class that is not
                        // instantioned yet
                        iterate(cpackagep);
                        తిరిగి;
                    }
                    // Likely impossible, as error thrown earlier
                    cpackagerefp->v3error(  // LCOV_EXCL_LINE
                        "'::' expected to reference a class/package but referenced '"
                        << (nodep->classOrPackagep() ? nodep->classOrPackagep()->prettyTypeName()
                                                     : "<unresolved-object>")
                        << "'\n"
                        << cpackagerefp->warn_మరిన్ని() + "... Suggest '.' instead of '::'");
                }
            }
            VL_DO_DANGLING(నెట్టు_Deletep(cpackagep->unlink_Fr_వెనుక()), cpackagep);
        }

        ఒకవేళ (m_ds.m_dotp && (m_ds.m_dotPos == DP_PACKAGE || m_ds.m_dotPos == DP_SCOPE)) {
            UASSERT_OBJ(VN_IS(m_ds.m_dotp->lhsp(), ClassOrPackageRef), m_ds.m_dotp->lhsp(),
                        "Bad package link");
            స్వయంచాలక* స్థిరరాశి cpackagerefp = VN_CAST(m_ds.m_dotp->lhsp(), ClassOrPackageRef);
            UASSERT_OBJ(cpackagerefp->classOrPackageSkipp(), m_ds.m_dotp->lhsp(),
                        "Bad package link");
            nodep->classOrPackagep(cpackagerefp->classOrPackageSkipp());
            m_ds.m_dotPos = DP_SCOPE;
        } లేకపోతే {
            // Allow REFDTYPE under DOT when referencing interface typedefs
            // This is needed for patterns like: typedef iface.a_t a_t;
            // The dependency graph handles resolution when IfaceCapture is disabled
            స్థిరరాశి తార్కిక ifaceFinalSegmentAllowed
                = (m_ds.m_dotPos == DP_FINAL) && m_ds.m_dotSymp
                  && VN_IS(m_ds.m_dotSymp->nodep(), Cell)
                  && VN_CAST(m_ds.m_dotSymp->nodep(), Cell)->modp()
                  && VN_IS(VN_CAST(m_ds.m_dotSymp->nodep(), Cell)->modp(), Iface);
            ఒకవేళ (!ifaceFinalSegmentAllowed) {
                checkNoDot(nodep);
            } లేకపోతే {
                UINFO(9, indent() << "allow REFDTYPE under DOT for iface typedef name="
                                  << nodep->prettyNameQ() << " dotSym=" << m_ds.m_dotSymp);
                // Clear the dot state so we don't propagate errors
                m_ds.m_dotPos = DP_SCOPE;
            }
        }
        ఒకవేళ (!nodep->typedefp() && !nodep->subDTypep()) {
            స్థిరరాశి VSymEnt* foundp;
            ఒకవేళ (nodep->classOrPackagep()) {
                foundp = m_statep->get_కణుపు_Sym(nodep->classOrPackagep())->కనుగొను_Id_Flat(nodep->name());
            } లేకపోతే ఒకవేళ (m_ds.m_dotPos == DP_మొదటి || m_ds.m_dotPos == DP_NONE) {
                foundp = కనుగొను_Id_Fallback_Skip_Member_DType(m_curSymp, nodep->name());
            } లేకపోతే {
                // Defensive: dotPos should be DP_FIRST/DP_NONE or classOrPackagep set.
                v3fatalSrc("Unexpected dotPos="
                           << static_cast<పూర్ణాంకం>(m_ds.m_dotPos)  // LCOV_EXCL_LINE
                           << " in RefDType lookup for "
                           << nodep->prettyNameQ());  // LCOV_EXCL_LINE
                foundp = శూన్యసూచిక;  // LCOV_EXCL_LINE
            }
            {
                ఒకవేళ (AstTypedef* స్థిరరాశి defp
                    = foundp ? VN_CAST(foundp->nodep(), Typedef) : శూన్యసూచిక) {
                    // Don't check if typedef is to a <type T>::<reference> as might not be
                    // resolved yet
                    ఒకవేళ (!nodep->classOrPackagep() && !defp->isUnderClass())
                        check_Decl_క్రమం(nodep, defp);
                    nodep->typedefp(defp);
                    nodep->classOrPackagep(foundp->classOrPackagep());

                } లేకపోతే ఒకవేళ (AstParamTypeDType* స్థిరరాశి defp
                           = foundp ? VN_CAST(foundp->nodep(), ParamTypeDType) : శూన్యసూచిక) {
                    // A PARAMTYPEDTYPE's child REFDTYPE referencing itself is the normal
                    // AST structure for type parameters (e.g., "type T = base" has a child
                    // REFDTYPE for the default type). Only error on true recursion where
                    // a TYPEDEF contains a reference back to itself.
                    స్థిరరాశి తార్కిక is_Param_Type_బిడ్డ = (defp == nodep->backp());
                    స్థిరరాశి తార్కిక isTypedefRecursion
                        = is_Param_Type_బిడ్డ && VN_IS(defp->backp(), Typedef);
                    ఒకవేళ (isTypedefRecursion) {
                        nodep->v3error("Reference to '"
                                       << m_ds.m_dot_పాఠ్యం << (m_ds.m_dot_పాఠ్యం == "" ? "" : ".")
                                       << nodep->prettyName() << "'"
                                       << " type would form a recursive definition");
                        nodep->refDTypep(nodep->కనుగొను_Void_DType());  // Try to reduce later errors
                    } లేకపోతే {
                        nodep->refDTypep(defp);
                        nodep->classOrPackagep(foundp->classOrPackagep());
                        captureIfaceParamType(nodep, defp);
                    }
                } లేకపోతే ఒకవేళ (AstClass* స్థిరరాశి defp
                           = foundp ? VN_CAST(foundp->nodep(), Class) : శూన్యసూచిక) {
                    // Don't check if typedef is to a <type T>::<reference> as might not be
                    // resolved yet
                    ఒకవేళ (!nodep->classOrPackagep()) check_Decl_క్రమం(nodep, defp);
                    AstPin* స్థిరరాశి paramsp = nodep->paramsp();
                    ఒకవేళ (paramsp) paramsp->unlink_Fr_వెనుక_With_తదుపరి();
                    AstClassRefDType* స్థిరరాశి newp
                        = కొత్త AstClassRefDType{nodep->fileline(), defp, paramsp};
                    newp->classOrPackagep(foundp->classOrPackagep());
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
                    తిరిగి;
                } లేకపోతే ఒకవేళ (m_insideClassExtParam) {
                    తిరిగి;
                } లేకపోతే {
                    ఒకవేళ (foundp) {
                        UINFO(1, "Found sym node: " << foundp->nodep());
                        nodep->v3error("Expecting a data type: "
                                       << nodep->prettyNameQ()
                                       << ", found: " << foundp->nodep()->prettyTypeName());
                    } లేకపోతే {
                        nodep->v3error("Can't find typedef/interface: " << nodep->prettyNameQ());
                    }
                }
            }
        }
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(AstRequireDType* nodep) override {
        // Handle type validation for localparam type assignments
        // Iterate child to resolve any ParseRef nodes
        iterateChildren(nodep);
        // Only emit error if the child is not a type.
        // Do NOT unwrap valid types here - leave that to V3Width.
        // Unwrapping here breaks type parameter resolution during cloning.
        // Allow Dot through to defer the handling until the dot expression is resolved.
        ఒకవేళ (nodep->lhsp() && !VN_IS(nodep->lhsp(), కణుపు_DType) && !VN_IS(nodep->lhsp(), Dot)) {
            // Not a type - emit error
            ఒకవేళ (VN_IS(nodep->lhsp(), Const)) {
                nodep->lhsp()->v3error("Expecting a data type, not a constant");
            } లేకపోతే {
                nodep->lhsp()->v3error("Expecting a data type, not "
                                       << nodep->lhsp()->typeName() << ": '"
                                       << nodep->lhsp()->prettyName() << "'");
            }
            // Replace with void to keep AST valid and allow clean exit
            nodep->replaceWith(కొత్త AstVoidDType{nodep->fileline()});
            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
        }
    }
    శూన్య సందర్శించు(AstDpiExport* nodep) override {
        // AstDpiExport: Make sure the function referenced exists, then dump it
        LINKDOT_సందర్శించు_ప్రారంభం();
        iterateChildren(nodep);
        checkNoDot(nodep);
        VSymEnt* స్థిరరాశి foundp = m_curSymp->కనుగొను_Id_Fallback(nodep->name());
        Ast_కణుపు_FTask* స్థిరరాశి taskp = foundp ? VN_AS(foundp->nodep(), కణుపు_FTask) : శూన్యసూచిక;
        ఒకవేళ (!taskp) {
            nodep->v3error(
                "Can't find definition of exported task/function: " << nodep->prettyNameQ());
        } లేకపోతే ఒకవేళ (taskp->dpiExport()) {
            nodep->v3error("Function was already DPI Exported, duplicate not allowed: "
                           << nodep->prettyNameQ());
        } లేకపోతే {
            taskp->dpiExport(నిజం);
            ఒకవేళ (nodep->cname() != "") taskp->cname(nodep->cname());
        }
        VL_DO_DANGLING(నెట్టు_Deletep(nodep->unlink_Fr_వెనుక()), nodep);
    }
    శూన్య సందర్శించు(AstDisable* nodep) override {
        LINKDOT_సందర్శించు_ప్రారంభం();
        checkNoDot(nodep);
        తార్కిక hasPartSelect = అబద్ధం;
        స్థిరరాశి string లక్ష్యం_మార్గం
            = nodep->లక్ష్యం_Refp() ? వెలికితీయి_Dotted_మార్గం(nodep->లక్ష్యం_Refp(), hasPartSelect) : "";
        VL_RESTORER_COPY(m_ds);
        m_ds.init(m_curSymp);
        m_ds.m_dotPos = DP_మొదటి;
        m_ds.m_disablep = nodep;
        iterateChildren(nodep);
        ఒకవేళ (nodep->లక్ష్యం_Refp()) {
            ఒకవేళ (AstTaskRef* స్థిరరాశి taskRefp = VN_CAST(nodep->లక్ష్యం_Refp(), TaskRef)) {
                nodep->targetp(taskRefp->taskp());
            } లేకపోతే ఒకవేళ (!VN_IS(nodep->లక్ష్యం_Refp(), ParseRef)) {
                // If it is a ParseRef, either it couldn't be linked or it is linked to a block
                nodep->v3warn(E_UNSUPPORTED, "Node of type "
                                                 << nodep->లక్ష్యం_Refp()->prettyTypeName()
                                                 << " referenced by disable");
                VL_DO_DANGLING(నెట్టు_Deletep(nodep->unlink_Fr_వెనుక()), nodep);
                తిరిగి;
            }
            ఒకవేళ (nodep->targetp()) {
                nodep->లక్ష్యం_Refp()->unlink_Fr_వెనుక()->delete_వృక్షం();
                ఒకవేళ (!hasPartSelect) {
                    స్థిరరాశి string instance_మార్గం = లక్ష్యం_Instance_మార్గం(nodep->targetp(), లక్ష్యం_మార్గం);
                    ఒకవేళ (!instance_మార్గం.ఖాళీలు()) {
                        // Keep only the instance prefix so V3LinkJump can reference the selected
                        // instance's process queue without reparsing the disable target.
                        nodep->లక్ష్యం_Refp(
                            కొత్త AstVarXRef{nodep->fileline(), "", instance_మార్గం, VAccess::READ});
                    }
                }
            }
        }
    }
    శూన్య సందర్శించు_Package_Import_Or_Export(Ast_కణుపు* nodep) {
        // No longer needed
        LINKDOT_సందర్శించు_ప్రారంభం();
        checkNoDot(nodep);
        ఒకవేళ (m_statep->forParamed()) VL_DO_DANGLING(నెట్టు_Deletep(nodep->unlink_Fr_వెనుక()), nodep);
    }
    శూన్య సందర్శించు(AstPackageImport* nodep) override { సందర్శించు_Package_Import_Or_Export(nodep); }
    శూన్య సందర్శించు(AstPackageExport* nodep) override { సందర్శించు_Package_Import_Or_Export(nodep); }
    శూన్య సందర్శించు(Ast_Package_Export_నక్షత్రం_నక్షత్రం* nodep) override { సందర్శించు_Package_Import_Or_Export(nodep); }
    శూన్య సందర్శించు(AstCellRef* nodep) override {
        LINKDOT_సందర్శించు_ప్రారంభం();
        UINFO(5, indent() << "visit " << nodep);
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(AstCellArrayRef* nodep) override {
        LINKDOT_సందర్శించు_ప్రారంభం();
        UINFO(5, indent() << "visit " << nodep);
        // Expression already iterated
    }
    శూన్య సందర్శించు(AstUnlinkedRef* nodep) override {
        LINKDOT_సందర్శించు_ప్రారంభం();
        UINFO(5, indent() << "visit " << nodep);
        // No need to iterate, if we have a UnlinkedVarXRef, we're already done
    }
    శూన్య సందర్శించు(Ast_Stmt_సమాసం* nodep) override {
        LINKDOT_సందర్శించు_ప్రారంభం();
        checkNoDot(nodep);
        // Check if nodep represents a super.new call;
        ఒకవేళ (AstNew* స్థిరరాశి newExprp = VN_CAST(nodep->exprp(), New)) {
            // in this case it was already linked, so it doesn't have a super reference
            m_explicitSuperNewp = newExprp;
        } లేకపోతే ఒకవేళ (స్థిరరాశి AstDot* స్థిరరాశి dotp = VN_CAST(nodep->exprp(), Dot)) {
            ఒకవేళ (dotp->lhsp()->name() == "super") {
                ఒకవేళ (AstNew* స్థిరరాశి newExprp = VN_CAST(dotp->rhsp(), New)) {
                    m_explicitSuperNewp = newExprp;
                }
            }
        }
        iterateChildren(nodep);
    }

    శూన్య సందర్శించు(AstIfaceRefDType* nodep) override {
        ఒకవేళ (nodep->user3_సమితి_Once()) తిరిగి;
        LINKDOT_సందర్శించు_ప్రారంభం();
        ఒకవేళ (nodep->paramsp()) {
            // If there is no parameters, there is no need to visit this node.
            AstIface* స్థిరరాశి ifacep = nodep->ifacep();
            UASSERT_OBJ(ifacep, nodep, "Port parameters of AstIfaceRefDType without ifacep()");
            ఒకవేళ (ifacep->dead()) తిరిగి;
            checkNoDot(nodep);
            VL_RESTORER_CLEAR(m_వాడిన_Pins);
            VL_RESTORER_CLEAR(m_వాడిన_Def_Param_Pins);
            VL_RESTORER(m_pinSymp);
            m_pinSymp = m_statep->get_కణుపు_Sym(ifacep);
            iterate_And_తదుపరి_Null(nodep->paramsp());
        }
    }

    శూన్య సందర్శించు(Ast_Attr_యొక్క* nodep) override { iterateChildren(nodep); }
    శూన్య సందర్శించు(AstCastParse* nodep) override {
        ఒకవేళ (nodep->user3_సమితి_Once()) తిరిగి;
        LINKDOT_సందర్శించు_ప్రారంభం();
        // A cast may start a dotted expression: HERE type'(expr) . member_or_method
        స్థిరరాశి తార్కిక dotLhs = m_ds.m_dotPos == DP_మొదటి;
        ఒకవేళ (!dotLhs) checkNoDot(nodep);
        {
            VL_RESTORER_COPY(m_ds);
            m_ds.init(m_curSymp);
            iterateChildren(nodep);
        }
        ఒకవేళ (dotLhs) m_ds.m_dotPos = DP_MEMBER;
    }

    శూన్య సందర్శించు(Ast_కేటాయించు_W* nodep) override {
        LINKDOT_సందర్శించు_ప్రారంభం();
        checkNoDot(nodep);
        VL_RESTORER(m_replaceWithAlias);
        ఒకవేళ (nodep->user2()) m_replaceWithAlias = అబద్ధం;
        iterateChildren(nodep);
    }

    శూన్య సందర్శించు(AstTypedef* nodep) override {
        VL_RESTORER(m_resolvingTypedef)
        m_resolvingTypedef = నిజం;
        iterateChildren(nodep);
    }

    శూన్య సందర్శించు(Ast_కణుపు* nodep) override {
        VL_RESTORER(m_లోపల_Packed_Array);
        ఒకవేళ (VN_IS(nodep, PackArrayDType)) {
            m_లోపల_Packed_Array = నిజం;
        } లేకపోతే ఒకవేళ (!m_లోపల_Packed_Array) {
            LINKDOT_సందర్శించు_ప్రారంభం();
            checkNoDot(nodep);
        }
        iterateChildren(nodep);
    }

బహిరంగ:
    // CONSTRUCTORS
    LinkDotResolveVisitor(AstNetlist* rootp, LinkDotState* statep)
        : m_statep{statep} {
        UINFO(4, __FUNCTION__ << ": ");

        // Note: no need to clear user3 on ledger entries here.
        // VNUser3InUse (m_inuser3) already logically clears user3 on all
        // nodes via the generation counter.  The ledger may also hold stale
        // refp pointers to deleted AST nodes, so iterating it is unsafe.

        iterate(rootp);
        స్థిరరాశి std::map<std::string, Ast_కణుపు_Module*> modules_గమ్యం_Revisit
            = std::move(m_modules_గమ్యం_Revisit);
        m_చివరి_Deferredp = శూన్యసూచిక;
        కొరకు (స్వయంచాలక& p : modules_గమ్యం_Revisit) {
            Ast_కణుపు_Module* స్థిరరాశి modp = p.రెండవ;
            modp->foreach([](Ast_కణుపు* స్థిరరాశి nodep) { nodep->user3(అబద్ధం); });
            iterate(modp);
        }
    }
    ~LinkDotResolveVisitor() override = అప్రమేయ;
};

//######################################################################
// Link class functions

శూన్య V3LinkDot::dumpSubstep(స్థిరరాశి string& name) {
    ఒకవేళ (dump_వృక్షం_Either_Level() >= 9) {
        V3Global::dump_Check_Global_వృక్షం(name);
    } లేకపోతే ఒకవేళ (debug() >= 5) {  // on high dbg level, dump even if not explicitly told to
        v3Global.rootp()->dump_వృక్షం_File(v3Global.debugFilename(name + ".tree"));
    }
}

శూన్య V3LinkDot::linkDotGuts(AstNetlist* rootp, VLink_Dot_అడుగు అడుగు) {
    VIsCached::clear_Cache_వృక్షం();  // Avoid using any stale isPure
    dumpSubstep("prelinkdot");
    LinkDotState state{rootp, అడుగు};

    { Link_Dot_కనుగొను_Visitor{rootp, &state}; }
    dumpSubstep("prelinkdot-find");

    { Link_Dot_కనుగొను_Iface_Visitor{rootp, &state}; }
    dumpSubstep("prelinkdot-findiface");

    ఒకవేళ (అడుగు == LDS_SCOPED) {
        // Well after the initial link when we're ready to operate on the flat design,
        // process AstScope's.  This needs to be separate pass after whole hierarchy graph created.
        { LinkDotScopeVisitor{rootp, &state}; }
        v3Global.assertScoped(నిజం);
        dumpSubstep("prelinkdot-scoped");
    } లేకపోతే {
        // Initial link stage, resolve parameters and interfaces
        { LinkDotParamVisitor{rootp, &state}; }
        dumpSubstep("prelinkdot-param");
    }
    state.dumpSelf("prelinkdot");
    state.computeIfaceModSyms();
    state.computeIfaceVarSyms();
    state.computeScopeAliases();
    state.dumpSelf("linkdot-preresolve");
    స్థిరరాశి LinkDotResolveVisitor visitor{rootp, &state};
    state.dumpSelf("linkdot-done");
}

శూన్య V3LinkDot::linkDotPrimary(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    linkDotGuts(nodep, LDS_PRIMARY);
    V3Global::dump_Check_Global_వృక్షం("linkdot", 0, dump_వృక్షం_Either_Level() >= 6);
}

శూన్య V3LinkDot::linkDotParamed(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    linkDotGuts(nodep, LDS_PARAMED);
    V3Global::dump_Check_Global_వృక్షం("linkdotparam", 0, dump_వృక్షం_Either_Level() >= 3);
}

శూన్య V3LinkDot::linkDotScope(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    linkDotGuts(nodep, LDS_SCOPED);
    V3Global::dump_Check_Global_వృక్షం("linkdot", 0, dump_వృక్షం_Either_Level() >= 3);
}
