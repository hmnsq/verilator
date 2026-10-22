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

#include "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#include "V3LinkDot.h"

#include "V3Global.h"
#include "V3Graph.h"
#include "V3LinkDotIfaceCapture.h"
#include "V3MemberMap.h"
#include "V3Parse.h"
#include "V3Randomize.h"
#include "V3Stats.h"
#include "V3String.h"
#include "V3SymTable.h"

#include <algorithm>
#include <cstdlib>
#include <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

static string extractDottedPath(AstNode* nodep, bool& hasPartSelect) {
    如果 (AstParseRef* const refp = VN_CAST(nodep, ParseRef)) {
        返回 refp->name();
    } 否则 如果 (AstVarRef* const refp = VN_CAST(nodep, VarRef)) {
        返回 refp->name();
    } 否则 如果 (AstDot* const dotp = VN_CAST(nodep, Dot)) {
        const string lhs = extractDottedPath(dotp->lhsp(), hasPartSelect);
        const string rhs = extractDottedPath(dotp->rhsp(), hasPartSelect);
        返回 VString::dot(lhs, ".", rhs);
    } 否则 如果 (VN_IS(nodep, SelBit) || VN_IS(nodep, SelExtract) || VN_IS(nodep, SelPlus)
               || VN_IS(nodep, SelMinus)) {
        hasPartSelect = true;
        返回 "";
    }
    返回 "";
}
static string lexicalDisablePath(AstNode* const targetp) {
    std::vector<string> names;
    为 (AstNode* curp = targetp; curp; curp = curp->aboveLoopp()) {
        如果 (AstNodeBlock* const blockp = VN_CAST(curp, NodeBlock)) {
            如果 (blockp->name() != "") names.push_back(blockp->name());
        } 否则 如果 (AstNodeFTask* const ftaskp = VN_CAST(curp, NodeFTask)) {
            names.push_back(ftaskp->name());
        } 否则 如果 (VN_IS(curp, NodeModule)) {
            break;
        }
    }
    string path;
    为 (auto it = names.crbegin(); it != names.crend(); ++it) {
        path = VString::dot(path, ".", *it);
    }
    返回 path;
}
static string targetInstancePath(AstNode* const targetp, const string& targetPath) {
    const string lexicalPath = lexicalDisablePath(targetp);
    如果 (lexicalPath == "" || targetPath == lexicalPath) 返回 "";
    const string suffix = "." + lexicalPath;
    如果 (!VString::endsWith(targetPath, suffix)) 返回 "";
    返回 targetPath.substr(0, targetPath.size() - suffix.size());
}

// ######################################################################
//  Matcher classes (for suggestion matching)

class LinkNodeMatcherClassOrPackage final : public VNodeMatcher {
public:
    bool nodeMatch(const AstNode* nodep) const override {
        返回 VN_IS(nodep, Class) || VN_IS(nodep, Package);
    }
};
class LinkNodeMatcherFTask final : public VNodeMatcher {
public:
    bool nodeMatch(const AstNode* nodep) const override { 返回 VN_IS(nodep, NodeFTask); }
};
class LinkNodeMatcherModport final : public VNodeMatcher {
public:
    bool nodeMatch(const AstNode* nodep) const override { 返回 VN_IS(nodep, Modport); }
};
class LinkNodeMatcherProd final : public VNodeMatcher {
public:
    bool nodeMatch(const AstNode* nodep) const override { 返回 VN_IS(nodep, RSProd); }
};
class LinkNodeMatcherVar final : public VNodeMatcher {
public:
    bool nodeMatch(const AstNode* nodep) const override {
        返回 VN_IS(nodep, Var) || VN_IS(nodep, LambdaArgRef);
    }
};
class LinkNodeMatcherVarIO final : public VNodeMatcher {
public:
    bool nodeMatch(const AstNode* nodep) const override {
        const AstVar* const varp = VN_CAST(nodep, Var);
        如果 (!varp) 返回 false;
        返回 varp->isIO();
    }
};
class LinkNodeMatcherVarParam final : public VNodeMatcher {
public:
    bool nodeMatch(const AstNode* nodep) const override {
        const AstVar* const varp = VN_CAST(nodep, Var);
        如果 (!varp) 返回 false;
        返回 varp->isParam();
    }
};

//######################################################################
// LinkDot state, as a visitor of each AstNode

class LinkDotState final {
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
    const VNUser1InUse m_inuser1;
    const VNUser2InUse m_inuser2;
    const VNUser4InUse m_inuser4;

public:
    // ENUMS
    // In order of priority, compute first ... compute last
    enum SAMNum : uint8_t { SAMN_MODPORT, SAMN_IFTOP, SAMN__MAX };  // Values for m_scopeAliasMap

private:
    // TYPES
    using ScopeAliasMap = std::unordered_map<VSymEnt*, VSymEnt*>;
    using IfaceModSyms = std::vector<std::pair<AstIface*, VSymEnt*>>;

    static LinkDotState* s_errorThisp;  // Last self, for error reporting only

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
    const VLinkDotStep m_step;  // Operational step

public:
    // METHODS

    void dumpSelf(const string& nameComment, bool force = false) {
        如果 (debug() >= 6 || dumpLevel() >= 6 || force) {
            const string filename = v3Global.debugFilename(nameComment) + ".txt";
            UINFO(4, "Dumping " << filename);
            const std::unique_ptr<std::ofstream> logp{V3File::new_ofstream(filename)};
            如果 (logp->fail()) v3fatal("Can't write file: " << filename);
            std::ostream& os = *logp;
            // TODO the symbol table has node pointers which may be
            // dangling, as we call deleteTree in these visitors without
            // pushing for later deletion (or deleting from the symbol table)
            // So, only completely safe to call this when under --debug or --debug-leak.
            m_syms.dumpSelf(os);
            bool first = true;
            为 (整数 samn = 0; samn < SAMN__MAX; ++samn) {
                如果 (!m_scopeAliasMap[samn].empty()) {
                    如果 (first) os << "\nScopeAliasMap:\n";
                    first = false;
                    为 (ScopeAliasMap::iterator it = m_scopeAliasMap[samn].begin();
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
    static void preErrorDumpHandler() {
        如果 (s_errorThisp) s_errorThisp->preErrorDump();
    }
    void preErrorDump() {
        static bool s_diddump = false;
        如果 (!s_diddump && dumpTreeLevel()) {
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
        s_errorThisp = this;
        V3Error::errorExitCb(preErrorDumpHandler);  // If get error, dump self
        readModNames();
    }
    ~LinkDotState() {
        V3Error::errorExitCb(nullptr);
        s_errorThisp = nullptr;
    }

    // ACCESSORS
    VSymGraph* symsp() { 返回 &m_syms; }
    整数 stepNumber() const { 返回 static_cast<整数>(m_step); }
    bool forPrimary() const { 返回 m_step == LDS_PRIMARY; }
    bool forParamed() const { 返回 m_step == LDS_PARAMED; }
    bool forScopeCreation() const { 返回 m_step == LDS_SCOPED; }

    // METHODS
    static string nodeTextType(AstNode* nodep) {
        如果 (const AstVar* const varp = VN_CAST(nodep, Var)) {
            如果 (varp->isIO() || varp->isIfaceRef()) {
                返回 "port";
            } 否则 如果 (varp->isGParam()) {
                返回 "parameter";
            } 否则 如果 (varp->varType() == VVarType::LPARAM) {
                返回 "local parameter";
            } 否则 {
                返回 "variable";
            }
        } 否则 如果 (const AstParamTypeDType* const dtypep = VN_CAST(nodep, ParamTypeDType)) {
            如果 (dtypep->isGParam()) {
                返回 "type parameter";
            } 否则 {
                返回 "local type parameter";
            }
        } 否则 如果 (VN_IS(nodep, Begin)) {
            返回 "block";
        } 否则 如果 (VN_IS(nodep, Cell)) {
            返回 "instance";
        } 否则 如果 (VN_IS(nodep, Constraint)) {
            返回 "constraint";
        } 否则 如果 (VN_IS(nodep, Func)) {
            返回 "function";
        } 否则 如果 (VN_IS(nodep, GenBlock)) {
            返回 "generate block";
        } 否则 如果 (VN_IS(nodep, Iface)) {
            返回 "interface";
        } 否则 如果 (VN_IS(nodep, RSProd)) {
            返回 "randsequence production";
        } 否则 如果 (VN_IS(nodep, Task)) {
            返回 "task";
        } 否则 {
            返回 nodep->prettyTypeName();
        }
    }

    AstNodeModule* findModuleSym(const string& modName) {
        const VSymEnt* const foundp = m_mods.rootp()->findIdFallback(modName);
        返回 foundp ? VN_AS(foundp->nodep(), NodeModule) : nullptr;
    }
    void readModNames() {
        // Look at all modules, and store pointers to all module names
        为 (AstNodeModule *nextp, *nodep = v3Global.rootp()->modulesp(); nodep; nodep = nextp) {
            nextp = VN_AS(nodep->nextp(), NodeModule);
            m_mods.rootp()->insert(nodep->name(), new VSymEnt{&m_mods, nodep});
        }
    }

    VSymEnt* rootEntp() const { 返回 m_syms.rootp(); }
    VSymEnt* dunitEntp() const { 返回 m_dunitEntp; }
    void checkDuplicate(VSymEnt* lookupSymp, AstNode* nodep, const string& name) {
        // Lookup the given name under current symbol table
        // Insert if not found
        // Report error if there's a duplicate
        //
        // Note we only check for conflicts at the same level; it's ok if one block hides another
        // We also wouldn't want to not insert it even though it's lower down

        const VSymEnt* const foundp = lookupSymp->findIdFlat(name);
        AstNode* const fnodep = foundp ? foundp->nodep() : nullptr;
        如果 (!fnodep) {
            // Not found, will add in a moment.
            如果 (forPrimary() && VSymEnt::checkSimilarname(nodep)) {
                const VSymEnt* const alt = lookupSymp->findSimilarIdFlat(name);
                如果 (alt) {
                    nodep->v3warn(SIMILARNAME, "Declaration overlaps another with different case: "
                                                   << nodep->prettyNameQ() << '\n'
                                                   << nodep->warnContextPrimary() << '\n'
                                                   << alt->nodep()->warnOther()
                                                   << "... Location of original declaration\n"
                                                   << alt->nodep()->warnContextSecondary());
                }
            }
        } 否则 如果 (nodep == fnodep) {  // Already inserted.
            // Good.
        } 否则 如果 (foundp->imported()) {  // From package
            // We don't throw VARHIDDEN as if the import is later the symbol
            // table's import wouldn't warn
        } 否则 如果 (forPrimary() && VN_IS(nodep, GenBlock)
                   && (VN_IS(fnodep, Begin) || VN_IS(fnodep, GenBlock))) {
            // Begin: ... blocks often replicate under genif/genfor, so
            // suppress duplicate checks.  See t_gen_forif.v for an example.
        } 否则 {
            UINFO(4, "name " << name);  // Not always same as nodep->name
            UINFO(4, "Var1 " << nodep);
            UINFO(4, "Var2 " << fnodep);
            // Quote the node's own name as for messages, if it is the symbol's
            const string nameQ
                = fnodep->name() == name ? fnodep->prettyNameQ() : AstNode::prettyNameQ(name);
            如果 (nodep->type() == fnodep->type()) {
                nodep->v3error("Duplicate declaration of "
                               << nodeTextType(fnodep) << ": " << nameQ << '\n'
                               << nodep->warnContextPrimary() << '\n'
                               << fnodep->warnOther() << "... Location of original declaration\n"
                               << fnodep->warnContextSecondary());
                nodep->declTokenNumSetMin(0);  // Disable checking forward typedefs
                fnodep->declTokenNumSetMin(0);  // Disable checking forward typedefs
            } 否则 {
                nodep->v3error("Unsupported in C: "
                               << ucfirst(nodeTextType(nodep)) << " has the same name as "
                               << nodeTextType(fnodep) << ": " << nameQ << '\n'
                               << nodep->warnContextPrimary() << '\n'
                               << fnodep->warnOther() << "... Location of original declaration\n"
                               << fnodep->warnContextSecondary());
            }
        }
    }
    void insertDUnit(AstNetlist* nodep) {
        // $unit on top scope
        VSymEnt* const symp = new VSymEnt{&m_syms, nodep};
        UINFO(9, "      INSERTdunit se" << cvtToHex(symp));
        symp->parentp(rootEntp());  // Needed so backward search can find name of top module
        symp->fallbackp(nullptr);
        rootEntp()->insert("$unit ", symp);  // Space so can never name conflict with user code
        //
        UASSERT_OBJ(!m_dunitEntp, nodep, "Call insertDUnit only once");
        m_dunitEntp = symp;
    }
    VSymEnt* insertTopCell(AstNodeModule* nodep, const string& scopename) {
        // Only called on the module at the very top of the hierarchy
        VSymEnt* const symp = new VSymEnt{&m_syms, nodep};
        UINFO(9, "      INSERTtop se" << cvtToHex(symp) << "  " << scopename << " " << nodep);
        symp->parentp(rootEntp());  // Needed so backward search can find name of top module
        symp->fallbackp(dunitEntp());  // Needed so can find $unit stuff
        nodep->user1p(symp);
        checkDuplicate(rootEntp(), nodep, nodep->origName());
        rootEntp()->insert(nodep->origName(), symp);
        如果 (forScopeCreation()) m_nameScopeSymMap.emplace(scopename, symp);
        返回 symp;
    }
    VSymEnt* insertTopIface(AstCell* nodep, const string& scopename) {
        VSymEnt* const symp = new VSymEnt{&m_syms, nodep};
        UINFO(9, "      INSERTtopiface se" << cvtToHex(symp) << "  " << scopename << " " << nodep);
        symp->parentp(rootEntp());  // Needed so backward search can find name of top module
        symp->fallbackp(dunitEntp());  // Needed so can find $unit stuff
        nodep->user1p(symp);
        如果 (nodep->modp()) nodep->modp()->user1p(symp);
        checkDuplicate(rootEntp(), nodep, nodep->origName());
        rootEntp()->insert(nodep->origName(), symp);
        如果 (forScopeCreation()) m_nameScopeSymMap.emplace(scopename, symp);
        返回 symp;
    }
    VSymEnt* insertCell(VSymEnt* abovep, VSymEnt* modSymp, AstCell* nodep,
                        const string& scopename) {
        UASSERT_OBJ(abovep, nodep, "Null symbol table inserting node");
        VSymEnt* const symp = new VSymEnt{&m_syms, nodep};
        UINFO(9, "      INSERTcel se" << cvtToHex(symp) << "  " << scopename << " above=se"
                                      << cvtToHex(abovep) << " mods=se" << cvtToHex(modSymp)
                                      << " node=" << nodep);
        symp->parentp(abovep);
        symp->fallbackp(dunitEntp());  // Needed so can find $unit stuff
        nodep->user1p(symp);
        如果 (nodep->modp()) nodep->modp()->user1p(symp);
        checkDuplicate(abovep, nodep, nodep->origName());
        abovep->reinsert(nodep->origName(), symp);
        如果 (forScopeCreation() && abovep != modSymp && !modSymp->findIdFlat(nodep->name())) {
            // If it's foo_DOT_bar, we need to be able to find it under "foo_DOT_bar" too.
            // Duplicates are possible, as until resolve generates might
            // have 2 same cells under an if
            modSymp->reinsert(nodep->name(), symp);
        }
        如果 (forScopeCreation()) m_nameScopeSymMap.emplace(scopename, symp);
        返回 symp;
    }
    void insertMap(VSymEnt* symp, const string& scopename) {
        如果 (forScopeCreation()) m_nameScopeSymMap.emplace(scopename, symp);
    }

    VSymEnt* insertInline(VSymEnt* abovep, VSymEnt* modSymp, AstCellInline* nodep,
                          const string& basename) {
        // A fake point in the hierarchy, corresponding to a named block flattened by V3Begin.
        // Names are looked up in the module with the block's flattened prefix.
        UASSERT_OBJ(abovep, nodep, "Null symbol table inserting node");
        VSymEnt* const symp = new VSymEnt{&m_syms, nodep};
        UINFO(9, "      INSERTinl se" << cvtToHex(symp) << "  " << basename << " above=se"
                                      << cvtToHex(abovep) << " mods=se" << cvtToHex(modSymp)
                                      << " node=" << nodep);
        symp->parentp(abovep);
        symp->fallbackp(modSymp);
        symp->symPrefix(nodep->name() + "__DOT__");
        nodep->user1p(symp);
        checkDuplicate(abovep, nodep, nodep->name());
        abovep->reinsert(basename, symp);
        如果 (abovep != modSymp && !modSymp->findIdFlat(nodep->name())) {
            // If it's foo_DOT_bar, we need to be able to find it under that too.
            modSymp->reinsert(nodep->name(), symp);
        }
        返回 symp;
    }
    VSymEnt* insertBlock(VSymEnt* abovep, const string& name, AstNode* nodep,
                         AstNodeModule* classOrPackagep) {
        // A fake point in the hierarchy, corresponding to a begin or function/task block
        // After we remove begins these will go away
        // Note we fallback to the symbol table of the parent, as we want to find variables there
        // However, cells walk the graph, so cells will appear under the begin/ftask itself
        UASSERT_OBJ(abovep, nodep, "Null symbol table inserting node");
        VSymEnt* const symp = new VSymEnt{&m_syms, nodep};
        UINFO(9, "      INSERTblk se" << cvtToHex(symp) << "  above=se" << cvtToHex(abovep)
                                      << " pkg=" << cvtToHex(classOrPackagep)
                                      << "  node=" << nodep);
        symp->parentp(abovep);
        symp->classOrPackagep(classOrPackagep);
        symp->fallbackp(abovep);
        nodep->user1p(symp);
        如果 (name != "") checkDuplicate(abovep, nodep, name);
        // Duplicates are possible, as until resolve generates might have 2 same cells under an if
        abovep->reinsert(name, symp);
        返回 symp;
    }
    VSymEnt* insertSym(VSymEnt* abovep, const string& name, AstNode* nodep,
                       AstNodeModule* classOrPackagep) {
        UASSERT_OBJ(abovep, nodep, "Null symbol table inserting node");
        VSymEnt* const symp = new VSymEnt{&m_syms, nodep};
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
        返回 symp;
    }
    static bool existsNodeSym(AstNode* nodep) { 返回 nodep->user1p() != nullptr; }
    static VSymEnt* getNodeSym(AstNode* nodep) {
        // Don't use this in ResolveVisitor, as we need to pick up the proper
        // reference under each SCOPE
        VSymEnt* const symp = nodep->user1u().toSymEnt();
        UASSERT_OBJ(symp, nodep, "Module/etc never assigned a symbol entry?");
        返回 symp;
    }
    VSymEnt* getScopeSym(AstScope* nodep) {
        const auto it = m_nameScopeSymMap.find(nodep->name());
        UASSERT_OBJ(it != m_nameScopeSymMap.end(), nodep,
                    "Scope never assigned a symbol entry '" << nodep->name() << "'");
        返回 it->second;
    }
    void implicitOkAdd(AstNodeModule* nodep, const string& varname) {
        // Mark the given variable name as being allowed to be implicitly declared
        如果 (nodep) {
            const auto it = m_implicitNameSet.find(std::make_pair(nodep, varname));
            如果 (it == m_implicitNameSet.end()) m_implicitNameSet.emplace(nodep, varname);
        }
    }
    bool implicitOk(AstNodeModule* nodep, const string& varname) {
        返回 nodep
               && (m_implicitNameSet.find(std::make_pair(nodep, varname))
                   != m_implicitNameSet.end());
    }

    // Track and later recurse interface modules
    void insertIfaceModSym(AstIface* nodep, VSymEnt* symp) {
        m_ifaceModSyms.emplace_back(nodep, symp);
    }
    void computeIfaceModSyms();

    // Track and later insert interface references
    void insertIfaceVarSym(VSymEnt* symp) {  // Where sym is for a VAR of dtype IFACEREFDTYPE
        m_ifaceVarSyms.push_back(symp);
    }
    // Iface for a raw or arrayed iface; peels nested array layers for multi-dim arrays.
    static AstIfaceRefDType* ifaceRefFromArray(AstNodeDType* nodep) {
        当 (nodep) {
            如果 (AstIfaceRefDType* const ifp = VN_CAST(nodep, IfaceRefDType)) 返回 ifp;
            如果 (const AstBracketArrayDType* const arrp = VN_CAST(nodep, BracketArrayDType)) {
                nodep = arrp->subDTypep();
            } 否则 如果 (const AstUnpackArrayDType* const arrp = VN_CAST(nodep, UnpackArrayDType)) {
                nodep = arrp->subDTypep();
            } 否则 {
                返回 nullptr;
            }
        }
        返回 nullptr;
    }
    // Given a pin expression, resolve it to a live AstIface* (or nullptr).
    // Handles both simple VarRef and dotted VarXRef pin connections.
    AstIface* liveIfaceFromPinExpr(AstNode* exprp, VSymEnt* parentSymp) {
        AstVar* resolvedVarp = nullptr;
        如果 (AstVarRef* const refp = VN_CAST(exprp, VarRef)) {
            resolvedVarp = refp->varp();
        } 否则 如果 (AstVarXRef* const xrefp = VN_CAST(exprp, VarXRef)) {
            VSymEnt* const lookupSymp = parentSymp ? parentSymp->parentp() : nullptr;
            如果 (lookupSymp) {
                string baddot;
                VSymEnt* okSymp = nullptr;
                const string dotpath = xrefp->dotted() + "." + xrefp->name();
                VSymEnt* const dotSymp
                    = findDotted(xrefp->fileline(), lookupSymp, dotpath, baddot, okSymp, true);
                如果 (dotSymp) resolvedVarp = VN_CAST(dotSymp->nodep(), Var);
            }
        }
        AstIface* resultp = nullptr;
        如果 (resolvedVarp) {
            AstIfaceRefDType* const irefp = ifaceRefFromArray(resolvedVarp->subDTypep());
            如果 (irefp && irefp->ifaceViaCellp() && !irefp->ifaceViaCellp()->dead()) {
                resultp = irefp->ifaceViaCellp();
            }
        }
        返回 resultp;
    }

    // Attempt to repair a port's AstIfaceRefDType by tracing through the
    // parent cell's pin connection to find the correct live interface.
    // Returns true if repaired or already correct; false if unresolvable.
    bool repairIfaceRef(VSymEnt* varSymp, AstVar* varp, AstIfaceRefDType* ifacerefp, bool isDead) {
        // Walk up symbol-table parents to find the enclosing AstCell
        VSymEnt* parentSymp = varSymp->parentp();
        AstCell* parentCellp = nullptr;
        为 (整数 depth = 0; parentSymp && depth < 20; ++depth) {
            如果 (AstCell* const cp = VN_CAST(parentSymp->nodep(), Cell)) {
                parentCellp = cp;
                break;
            }
            parentSymp = parentSymp->parentp();
        }
        如果 (!parentCellp) 返回 false;
        // Scan pins to find the one connected to this port variable
        为 (AstPin* pinp = parentCellp->pinsp(); pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            如果 (pinp->modVarp() != varp && pinp->name() != varp->name()) continue;
            AstNode* const exprp = pinp->exprp();
            如果 (!exprp) 返回 false;
            AstIface* const newIfacep = liveIfaceFromPinExpr(exprp, parentSymp);
            如果 (newIfacep && newIfacep != ifacerefp->ifaceViaCellp()) {
                UINFO(4, "  REPAIR-IFACE-REF var=" << varp->prettyNameQ()
                                                   << " old=" << ifacerefp->ifacep()->prettyNameQ()
                                                   << " new=" << newIfacep->prettyNameQ());
                ifacerefp->ifacep(newIfacep);
                返回 true;
            }
            如果 (newIfacep) 返回 !isDead;  // Same interface - OK if live
            返回 false;
        }
        返回 false;
    }

    void computeIfaceVarSyms() {
        为 (VSymEnt* varSymp : m_ifaceVarSyms) {
            AstVar* const varp = varSymp ? VN_AS(varSymp->nodep(), Var) : nullptr;
            UINFO(9, "  insAllIface se" << cvtToHex(varSymp) << " " << varp);
            AstIfaceRefDType* const ifacerefp = ifaceRefFromArray(varp->subDTypep());
            UASSERT_OBJ(ifacerefp, varp, "Non-ifacerefs on list!");
            如果 (!ifacerefp->cellp() && !ifacerefp->isVirtual() && (varp->pinNum() == 0)) {
                varp->v3error("Interface port declaration "
                              << varp->prettyNameQ() << " doesn't have corresponding port\n"
                              << varp->warnMore()
                                     + "... Perhaps intended an interface instantiation but "
                                       "are missing parenthesis (IEEE 1800-2023 25.3)?");
            }
            如果 (!ifacerefp->ifaceViaCellp()) {
                如果 (!ifacerefp->cellp()) {  // Probably a NotFoundModule, or a normal module if
                                            // made mistake
                    UINFO(1, "Associated cell " << AstNode::prettyNameQ(ifacerefp->cellName()));
                    ifacerefp->v3error("Cannot find file containing interface: "
                                       << AstNode::prettyNameQ(ifacerefp->ifaceName()));
                    continue;
                } 否则 {
                    ifacerefp->v3fatalSrc("Unlinked interface");
                }
            } 否则 如果 (varp->isIfaceRef() && !ifacerefp->cellp() && ifacerefp->ifaceViaCellp()) {
                // Port variable (cellp=null) pointing to an interface.
                // The interface may be dead (template replaced by clone) or
                // live-but-wrong (template still alive because some instances
                // use default params, but this port should point to a
                // specialized clone based on the pin connection).
                const bool isDead = ifacerefp->ifaceViaCellp()->dead();
                如果 (isDead && forPrimary() && !varp->isIfaceParent()
                    && !v3Global.opt.topIfacesSupported()) {
                    varp->v3warn(E_UNSUPPORTED,
                                 "Unsupported: Interfaced port on top level module");
                }
                // Attempt repair: trace through parent cell's pin to find
                // the correct live interface for this port variable
                const bool repaired = repairIfaceRef(varSymp, varp, ifacerefp, isDead);
                如果 (!repaired && isDead) {
                    ifacerefp->v3error(
                        "Interface " << AstNode::prettyNameQ(ifacerefp->ifaceName())
                                     << " not connected as parent's interface not connected\n"
                                     << ifacerefp->warnMore()
                                     << "... Perhaps caused by another error on the parent "
                                        "interface that needs resolving\n"
                                     << ifacerefp->warnMore()
                                     << "... Or, perhaps intended an interface instantiation but "
                                        "are missing parenthesis (IEEE 1800-2023 25.3)?");
                    continue;
                }
            } 否则 如果 (ifacerefp->ifaceViaCellp()->dead()
                       || !existsNodeSym(ifacerefp->ifaceViaCellp())) {
                ifacerefp->ifaceViaCellp()->v3fatalSrc(
                    "Interface referenced by virtual iface is dead. "
                    "Dead interfaces have required linking steps skipped");
            }
            VSymEnt* const ifaceSymp = getNodeSym(ifacerefp->ifaceViaCellp());
            VSymEnt* ifOrPortSymp = ifaceSymp;
            // Link Modport names to the Modport Node under the Interface
            如果 (ifacerefp->isModport()) {
                VSymEnt* const foundp = ifaceSymp->findIdFallback(ifacerefp->modportName());
                bool ok = false;
                如果 (foundp) {
                    如果 (AstModport* const modportp = VN_CAST(foundp->nodep(), Modport)) {
                        UINFO(4, "Link Modport: " << modportp);
                        ifacerefp->modportp(modportp);
                        ifOrPortSymp = foundp;
                        ok = true;
                    }
                }
                如果 (!ok) {
                    const string suggest = suggestSymFallback(ifaceSymp, ifacerefp->modportName(),
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

    void insertScopeAlias(SAMNum samn, VSymEnt* lhsp, VSymEnt* rhsp) {
        // Track and later insert scope aliases; an interface referenced by
        // a child cell connecting to that interface
        // Typically lhsp=VAR w/dtype IFACEREF, rhsp=IFACE cell
        UINFO(9, "   insertScopeAlias se" << cvtToHex(lhsp) << " se" << cvtToHex(rhsp));
        UASSERT_OBJ(
            !(VN_IS(rhsp->nodep(), Cell) && !VN_IS(VN_AS(rhsp->nodep(), Cell)->modp(), Iface)),
            rhsp->nodep(), "Got a non-IFACE alias RHS");
        m_scopeAliasMap[samn].emplace(lhsp, rhsp);
    }
    void computeScopeAliases() {
        UINFO(9, "computeIfaceAliases");
        为 (整数 samn = 0; samn < SAMN__MAX; ++samn) {
            为 (ScopeAliasMap::iterator it = m_scopeAliasMap[samn].begin();
                 it != m_scopeAliasMap[samn].end(); ++it) {
                VSymEnt* const lhsp = it->first;
                VSymEnt* srcp = lhsp;
                当 (true) {  // Follow chain of aliases up to highest level non-alias
                    const auto it2 = m_scopeAliasMap[samn].find(srcp);
                    如果 (it2 != m_scopeAliasMap[samn].end()) {
                        srcp = it2->second;
                        continue;
                    } 否则 {
                        break;
                    }
                }
                UINFO(9, "  iiasa: Insert alias se" << lhsp << " (" << lhsp->nodep()->typeName()
                                                    << ") <- se" << srcp << " " << srcp->nodep());
                // srcp should be an interface reference pointing to the interface we want to
                // import
                lhsp->importFromIface(symsp(), srcp);
                // Allow access to objects not permissible to be listed in a modport
                如果 (VN_IS(srcp->nodep(), Modport)) {
                    lhsp->importFromIface(symsp(), srcp->parentp(), true);
                }
            }
            // m_scopeAliasMap[samn].clear();  // Done with it, but put into debug file
        }
    }

private:
    VSymEnt* findWithAltFallback(VSymEnt* symp, const string& name, const string& altname) {
        VSymEnt* findp = symp->findIdFallback(name);
        如果 (findp) 返回 findp;
        如果 (altname != "") {
            UINFO(8, "     alt fallback");
            findp = symp->findIdFallback(altname);
        }
        返回 findp;
    }

    VSymEnt* findWithAltFlat(VSymEnt* symp, const string& name, const string& altname) {
        VSymEnt* findp = symp->findIdFlat(name);
        如果 (findp) 返回 findp;
        如果 (altname != "") {
            UINFO(8, "     alt flat");
            findp = symp->findIdFlat(altname);
        }
        返回 findp;
    }

public:
    VSymEnt* findDotted(FileLine* refLocationp, VSymEnt* lookupSymp, const string& dotname,
                        string& baddot, VSymEnt*& okSymp, bool firstId) {
        // Given a dotted hierarchy name, return where in scope it is
        // Note when dotname=="" we just fall through and return lookupSymp
        UINFO(8, "    dottedFind se" << cvtToHex(lookupSymp) << " '" << dotname << "'");
        string leftname = dotname;
        okSymp = lookupSymp;  // So can list bad scopes
        当 (leftname != "") {  // foreach dotted part of xref name
            string::size_type pos = leftname.find('.');
            string ident;
            如果 (pos != string::npos) {
                ident = leftname.substr(0, pos);
                leftname = leftname.substr(pos + 1);
            } 否则 {
                ident = leftname;
                leftname = "";
            }
            baddot = ident;  // So user can see where they botched it
            okSymp = lookupSymp;
            string altIdent;
            如果 (!forScopeCreation()) {
                // GENFOR Begin is foo__BRA__##__KET__ after we've genloop unrolled,
                // but presently should be just "foo".
                // Likewise cell foo__[array] before we've expanded arrays is just foo.
                // Multi-dim iface arrays append multiple __BRA__..__KET__ suffixes; strip them
                // all.
                altIdent = ident;
                当 (VString::endsWith(altIdent, "__KET__")) {
                    const auto braPos = altIdent.rfind("__BRA__");
                    如果 (braPos == string::npos) break;
                    altIdent = altIdent.substr(0, braPos);
                }
                如果 (altIdent == ident) altIdent.clear();
            }
            UINFO(8, "         id " << ident << " alt " << altIdent << " left " << leftname
                                    << " at se" << lookupSymp);
            // Spec says; Look at existing module (cellnames then modname),
            // then look up (inst name or modname)
            如果 (firstId) {
                // Check this module - subcellnames
                const AstCell* cellp = lookupSymp ? VN_CAST(lookupSymp->nodep(), Cell)
                                                  : nullptr;  // Replicated below
                如果 (VSymEnt* const findSymp = findWithAltFallback(lookupSymp, ident, altIdent)) {
                    lookupSymp = findSymp;
                }

                // Check this module - cur modname
                否则 如果 (cellp && cellp->modp()->origName() == ident) {
                }
                // $root we walk up to Netlist
                否则 如果 (ident == "$root") {
                    lookupSymp = rootEntp();
                    // We've added the '$root' module, now everything else is one lower
                    如果 (forScopeCreation()) {
                        lookupSymp = lookupSymp->findIdFlat(ident);
                        UASSERT(lookupSymp, "Cannot find $root module under netlist");
                    }
                }
                // Move up and check cellname + modname
                否则 {
                    bool crossedCell = false;  // Crossed a cell boundary
                    当 (lookupSymp) {
                        lookupSymp = lookupSymp->parentp();
                        cellp = lookupSymp ? VN_CAST(lookupSymp->nodep(), Cell)
                                           : nullptr;  // Replicated above
                        如果 (lookupSymp) {
                            UINFO(9, "    Up to " << lookupSymp);
                            如果 (cellp || VN_IS(lookupSymp->nodep(), CellInline)) {
                                crossedCell = true;
                            }
                            如果 (cellp && cellp->modp()->origName() == ident) {
                                break;
                            } 否则 如果 (VSymEnt* const findSym2p
                                       = findWithAltFallback(lookupSymp, ident, altIdent)) {
                                lookupSymp = findSym2p;
                                如果 (crossedCell && VN_IS(lookupSymp->nodep(), Var)) {
                                    UINFO(9, "    Not found but matches var name in parent "
                                                 << lookupSymp);
                                    返回 nullptr;
                                }
                                break;
                            }
                        } 否则 {
                            break;
                        }
                    }
                    如果 (!lookupSymp) 返回 nullptr;  // Not found
                }
            } 否则 {  // Searching for middle path component, must be a cell or interface port
                VSymEnt* findSymp = findWithAltFlat(lookupSymp, ident, altIdent);
                如果 (findSymp) {
                    lookupSymp = findSymp;
                } 否则 {
                    // Try prefixed lookup for interface ports accessed through hierarchy
                    // (e.g., slave_inst.bus.data where bus is an interface port)
                    // After inlining, interface ports have prefixed names like
                    // top__DOT__slave_inst__DOT__bus
                    UINFO(8, "     middle-path: trying prefixed lookup for '" << ident << "'\n");
                    string baddot;
                    findSymp = findSymPrefixed(lookupSymp, ident, baddot, /*fallback=*/true);
                    UINFO(8, "     middle-path: prefixed lookup result: "
                                 << (findSymp ? "found" : "not found") << "\n");
                    如果 (findSymp) {
                        // Check if this is an interface reference variable
                        const AstVar* varp = VN_CAST(findSymp->nodep(), Var);
                        如果 (!varp) {
                            如果 (const AstVarScope* vscp = VN_CAST(findSymp->nodep(), VarScope)) {
                                varp = vscp->varp();
                            }
                        }
                        如果 (varp && varp->isIfaceRef()) {
                            // Found an interface port - use findSymp directly
                            // computeScopeAliases has already imported interface members
                            // into this symbol entry, so we use it instead of redirecting
                            // to the shared modport definition (which would cause all
                            // instances to resolve to the same symbols - bug #2656)
                            lookupSymp = findSymp;
                        } 否则 {
                            // Non-interface symbol in middle path must be a cell (module instance)
                            // to continue hierarchical resolution
                            如果 (VN_IS(findSymp->nodep(), Cell)) {
                                lookupSymp = findSymp;
                            } 否则 {
                                // Reject non-cell, non-interface symbols found via fallback
                                // to prevent accidental resolution to unrelated symbols
                                UINFO(8, "     middle-path: rejecting non-cell symbol\n");
                                返回 nullptr;
                            }
                        }
                    } 否则 {
                        返回 nullptr;  // Not found
                    }
                }
            }
            如果 (lookupSymp) {
                如果 (const AstCell* const cellp = VN_CAST(lookupSymp->nodep(), Cell)) {
                    如果 (const AstNodeModule* const modp = cellp->modp()) {
                        如果 (modp->hierBlock() && !leftname.empty()) {
                            refLocationp->v3error("Cannot access scope inside hierarchical block");
                        } 否则 如果 (VN_IS(modp, NotFoundModule)) {
                            refLocationp->v3error("Dotted reference to instance that refers to "
                                                  "missing module/interface: "
                                                  << modp->prettyNameQ());
                        }
                    }
                }
                // Follow scope alias for nested interface port access
                如果 (!leftname.empty()) {
                    const auto aliasIt = m_scopeAliasMap[SAMN_IFTOP].find(lookupSymp);
                    如果 (aliasIt != m_scopeAliasMap[SAMN_IFTOP].end()) {
                        lookupSymp = aliasIt->second;
                        // Alias may point to __Viftop VarScope; find corresponding Cell
                        如果 (const AstVarScope* const vscp
                            = VN_CAST(lookupSymp->nodep(), VarScope)) {
                            const string varName = vscp->varp()->name();
                            static constexpr const char* const VIFTOP_SUFFIX = "__Viftop";
                            如果 (VString::endsWith(varName, VIFTOP_SUFFIX)) {
                                const string cellName
                                    = varName.substr(0, varName.size() - strlen(VIFTOP_SUFFIX));
                                VSymEnt* const parentSymp = lookupSymp->parentp();
                                如果 (parentSymp) {
                                    VSymEnt* const cellSymp = parentSymp->findIdFlat(cellName);
                                    如果 (cellSymp && VN_IS(cellSymp->nodep(), Cell)) {
                                        lookupSymp = cellSymp;  // Use Cell for member lookup
                                    }
                                }
                            }
                        }
                    } 否则 {
                        // No alias; try following IfaceRefDType to interface cell
                        const AstVar* varp = VN_CAST(lookupSymp->nodep(), Var);
                        如果 (!varp) {
                            如果 (const AstVarScope* const vscp
                                = VN_CAST(lookupSymp->nodep(), VarScope)) {
                                varp = vscp->varp();
                            }
                        }
                        如果 (varp && varp->isIfaceRef()) {
                            如果 (const AstIfaceRefDType* const ifaceRefp
                                = ifaceRefFromArray(varp->dtypep())) {
                                如果 (ifaceRefp->cellp() && existsNodeSym(ifaceRefp->cellp())) {
                                    lookupSymp = getNodeSym(ifaceRefp->cellp());
                                } 否则 如果 (ifaceRefp->ifaceViaCellp()
                                           && existsNodeSym(ifaceRefp->ifaceViaCellp())) {
                                    lookupSymp = getNodeSym(ifaceRefp->ifaceViaCellp());
                                } 否则 如果 (ifaceRefp->ifacep()
                                           && existsNodeSym(ifaceRefp->ifacep())) {
                                    lookupSymp = getNodeSym(ifaceRefp->ifacep());
                                }
                            }
                        }
                    }
                }
            }
            firstId = false;
        }
        返回 lookupSymp;
    }

    static string removeLastInlineScope(const string& name) {
        string out = name;
        const string dot = "__DOT__";
        const size_t dotPos = out.rfind(dot, out.size() - dot.length() - 1);
        如果 (dotPos == string::npos) {
            返回 "";
        } 否则 {
            返回 out.erase(dotPos + dot.length(), string::npos);
        }
    }

    VSymEnt* findSymPrefixed(VSymEnt* lookupSymp, const string& dotname, string& baddot,
                             bool fallback) {
        // Find symbol in given point in hierarchy, allowing prefix (post-Inline)
        // For simplicity lookupSymp may be passed nullptr result from findDotted
        如果 (!lookupSymp) 返回 nullptr;
        UINFO(8, "    findSymPrefixed "
                     << dotname << " under se" << cvtToHex(lookupSymp)
                     << ((lookupSymp->symPrefix() == "") ? "" : " as ")
                     << ((lookupSymp->symPrefix() == "") ? "" : lookupSymp->symPrefix() + dotname)
                     << "  at se" << lookupSymp << " fallback=" << fallback);
        string prefix = lookupSymp->symPrefix();
        VSymEnt* foundp = nullptr;
        当 (!foundp) {
            如果 (fallback) {
                foundp = lookupSymp->findIdFallback(prefix + dotname);  // Might be nullptr
            } 否则 {
                foundp = lookupSymp->findIdFlat(prefix + dotname);  // Might be nullptr
            }
            如果 (prefix.empty()) break;
            const string nextPrefix = removeLastInlineScope(prefix);
            如果 (prefix == nextPrefix) break;
            prefix = nextPrefix;
        }
        如果 (!foundp) baddot = dotname;
        返回 foundp;
    }
    static bool checkIfClassOrPackage(const VSymEnt* const symp) {
        如果 (VN_IS(symp->nodep(), Class) || VN_IS(symp->nodep(), Package)) 返回 true;

        // Helper: check if a RefDType might resolve to a class later.
        const auto isPotentialClassRef = [](const AstRefDType* refp) -> bool {
            返回 refp && !refp->typeofp()
                   && (!refp->classOrPackageOpp()
                       || VN_IS(refp->classOrPackageOpp(), ClassOrPackageRef));
        };

        // Helper: allow types that can represent a class/package handle or an unresolved ref.
        // BasicDType is allowed here only for null/object handles.
        const auto isValidTypeNode = [](const AstNode* nodep) -> bool {
            返回 VN_IS(nodep, VoidDType) || VN_IS(nodep, BasicDType)
                   || VN_IS(nodep, ClassRefDType) || VN_IS(nodep, ParseRef);
        };

        如果 (const AstTypedef* const typedefp = VN_CAST(symp->nodep(), Typedef)) {
            const AstNodeDType* dtypep = typedefp->subDTypep();
            如果 (!dtypep) dtypep = typedefp->childDTypep();
            如果 (VN_IS(dtypep, ClassRefDType)) 返回 true;
            如果 (isPotentialClassRef(VN_CAST(dtypep, RefDType))) 返回 true;
        } 否则 如果 (const AstParamTypeDType* const paramTypep
                   = VN_CAST(symp->nodep(), ParamTypeDType)) {
            // Before V3Param the declared default is in childDTypep (possibly
            // wrapped in a RequireDType); after V3Param it is consumed and the
            // bound type is the resolved data type, e.g. a type parameter
            // inherited from a specialized base class (REQ #(Item) -> class Item).
            AstNode* childp = paramTypep->childDTypep();
            如果 (const AstRequireDType* const reqp = VN_CAST(childp, RequireDType)) {
                childp = reqp->lhsp();
            }
            const AstNode* const checkp = childp ? childp : paramTypep->skipRefp();
            如果 (isValidTypeNode(checkp) || isPotentialClassRef(VN_CAST(checkp, RefDType))) {
                返回 true;
            }
        }
        返回 false;
    }
    VSymEnt* resolveClassOrPackage(VSymEnt* lookSymp, AstClassOrPackageRef* nodep, bool fallback,
                                   bool classOnly, const string& forWhat,
                                   bool deferIfUnresolved = false) {
        如果 (nodep->classOrPackageSkipp()) 返回 getNodeSym(nodep->classOrPackageSkipp());
        VSymEnt* foundp;
        VSymEnt* searchSymp = lookSymp;
        如果 (VL_UNCOVERABLE(searchSymp && VN_IS(searchSymp->nodep(), ParamTypeDType))) {
            searchSymp->nodep()->v3fatalSrc(  // LCOV_EXCL_LINE
                "resolveClassOrPackage: unexpected ParamTypeDType lookup");
        }
        如果 (fallback) {
            VSymEnt* currentLookSymp = searchSymp;
            do {
                foundp = currentLookSymp->findIdFlat(nodep->name());
                如果 (foundp && !checkIfClassOrPackage(foundp)) foundp = nullptr;
                如果 (!foundp) currentLookSymp = currentLookSymp->fallbackp();
            } 当 (!foundp && currentLookSymp);
        } 否则 {
            foundp = searchSymp->findIdFlat(nodep->name());
            如果 (foundp && !checkIfClassOrPackage(foundp)) foundp = nullptr;
        }
        如果 (!foundp && v3Global.rootp()->stdPackagep()) {  // Look under implied std::
            foundp = getNodeSym(v3Global.rootp()->stdPackagep())->findIdFlat(nodep->name());
        }
        如果 (foundp) {
            nodep->classOrPackageNodep(foundp->nodep());
            返回 foundp;
        }
        如果 (deferIfUnresolved) 返回 nullptr;
        const string suggest
            = suggestSymFallback(lookSymp, nodep->name(), LinkNodeMatcherClassOrPackage{});
        nodep->v3error((classOnly ? "Class" : "Package/class")
                       << " for '" << forWhat  // extends/implements
                       << "' not found: " << nodep->prettyNameQ() << '\n'
                       << (suggest.empty() ? "" : nodep->warnMore() + suggest));
        返回 nullptr;
    }
    string suggestSymFallback(VSymEnt* lookupSymp, const string& name,
                              const VNodeMatcher& matcher) {
        // Suggest alternative symbol in given point in hierarchy
        // Does not support inline, as we find user-level errors before inlining
        // For simplicity lookupSymp may be passed nullptr result from findDotted
        如果 (!lookupSymp) 返回 "";
        VSpellCheck speller;
        lookupSymp->candidateIdFallback(&speller, &matcher);
        返回 speller.bestCandidateMsg(name);
    }
    string suggestSymFlat(VSymEnt* lookupSymp, const string& name, const VNodeMatcher& matcher,
                          bool includeType) {
        如果 (!lookupSymp) 返回 "";
        VSpellCheck speller;
        lookupSymp->candidateIdFlat(&speller, &matcher, includeType);
        返回 speller.bestCandidateMsg(name);
    }
};

LinkDotState* LinkDotState::s_errorThisp = nullptr;

//======================================================================

class LinkDotFindVisitor final : public VNVisitor {
    // NODE STATE
    // Cleared on global
    //  *::user1p()             -> See LinkDotState
    //  *::user2p()             -> See LinkDotState
    //  *::user3()              // bool. Node stored in m_virtualIfaces
    //  *::user4()              -> See LinkDotState
    const VNUser3InUse m_inuser3;

    // STATE
    LinkDotState* const m_statep;  // State to pass between visitors, including symbol table
    AstNodeModule* m_classOrPackagep = nullptr;  // Current package
    AstClocking* m_clockingp = nullptr;  // Current clocking block
    std::vector<AstScope*> m_scopes;  // Scopes detached during repeated module visits
    uint64_t m_statScopeVisits = 0;  // Non-top scopes encountered while finding instance symbols
    VSymEnt* m_modSymp = nullptr;  // Symbol Entry for current module
    VSymEnt* m_curSymp = nullptr;  // Symbol Entry for current table, where to lookup/insert
    string
        m_hierParamsName;  // Name of module with hierarchical type parameters, empty when not used
    string m_scope;  // Scope text
    const AstNodeFTask* m_ftaskp = nullptr;  // Current function/task
    bool m_inRecursion = false;  // Inside a recursive module
    整数 m_paramNum = 0;  // Parameter number, for position based connection
    bool m_explicitNew = false;  // Hit a "new" function
    整数 m_modBlockNum = 0;  // Begin block number in module, 0=none seen
    整数 m_modWithNum = 0;  // With block number, 0=none seen
    整数 m_modArgNum = 0;  // Arg block number for randomize(), 0=none seen
    std::vector<AstIface*> m_virtIfaces;  // interfaces used as virtual,
                                          // needed for handleUnvisitedVirtIfaces()

    // METHODS
    void makeImplicitNew(AstClass* nodep) {
        AstFunc* const newp = new AstFunc{nodep->fileline(), "new", nullptr, nullptr};
        // If needed, super.new() call is added the 2nd pass of V3LinkDotResolve,
        // because base classes are already resolved there.
        newp->isConstructor(true);
        nodep->addMembersp(newp);
        UINFO(8, "Made implicit new for " << nodep->name() << ": " << nodep);
        m_statep->insertBlock(m_curSymp, newp->name(), newp, m_classOrPackagep);
    }

    static bool isHierBlockWrapper(const string& name) {
        const V3HierBlockOptSet& hierBlocks = v3Global.opt.hierBlocks();
        返回 hierBlocks.find(name) != hierBlocks.end();
    }

    void moveExternFuncDecl(AstNodeFTask* nodep, AstClass* classp) {
        // Move an 'extern function|task' to inside a class or, from an outer to inner-class
        如果 (nodep->isExternDef()) 返回;
        // Move it to proper spot under the target class
        nodep->unlinkFrBack();
        classp->addStmtsp(nodep);
        nodep->isExternDef(true);  // So we check there's a matching extern
        nodep->classOrPackagep()->unlinkFrBack()->deleteTree();
        // Any "Type::" reference in the function's IO are really "MovedToClass::" references
        如果 (nodep->fvarp()) moveExternFuncDeclRefs(nodep->fvarp(), classp);
        为 (AstNode* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            如果 (AstVar* const portp = VN_CAST(stmtp, Var)) {
                如果 (portp->isIO()) moveExternFuncDeclRefs(portp, classp);
            }
        }
    }
    void moveExternFuncDeclRefs(AstNode* nodep, AstClass* classp) {
        nodep->foreach([this, classp](AstClassOrPackageRef* refp) {  //
            如果 (refp->name() == classp->name() && !refp->paramsp()) {
                UINFO(9, "Cleaning up external function type for class " << refp);
                AstDot* const dotp = VN_CAST(refp->backp(), Dot);
                如果 (dotp && dotp->lhsp() == refp) {
                    // If had DOT(CLASSREF this, ...) the DOT can go away
                    dotp->replaceWith(dotp->rhsp()->unlinkFrBack());
                    VL_DO_DANGLING2(pushDeletep(dotp), dotp, refp);
                } 否则 {
                    VL_DO_DANGLING(pushDeletep(refp->unlinkFrBack()), refp);
                }
            }
        });
    }

    // VISITORS
    void visit(AstNetlist* nodep) override {  // FindVisitor::
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
        AstNodeModule* const modulesp = nodep->modulesp();
        UASSERT_OBJ(modulesp, nodep, "$unit should always be in the netlist");
        如果 (m_statep->forPrimary()) {
            const bool onlyBuiltins = [&]() {
                为 (AstNodeModule* modp = modulesp; modp;
                     modp = VN_AS(modp->nextp(), NodeModule)) {
                    如果 (!modp->isDollarUnit() && !modp->isConstPool()) 返回 false;
                }
                返回 true;
            }();
            如果 (onlyBuiltins && !v3Global.rootp()->dollarUnitPkgp()->stmtsp()) {
                nodep->v3error("No top level module found");
            }
        }
        为 (AstNodeModule* modp = nodep->modulesp(); modp && modp->isTop();
             modp = VN_AS(modp->nextp(), NodeModule)) {
            UINFO(8, "Top Module: " << modp);
            m_scope = "TOP";

            如果 (!m_statep->forScopeCreation() && v3Global.opt.topIfacesSupported()) {
                为 (AstNode* subnodep = modp->stmtsp(); subnodep; subnodep = subnodep->nextp()) {
                    如果 (AstVar* const varp = VN_CAST(subnodep, Var)) {
                        如果 (varp->isIfaceRef()) {
                            const AstNodeDType* const subtypep = varp->subDTypep();
                            const AstIfaceRefDType* ifacerefp = nullptr;
                            如果 (VN_IS(subtypep, IfaceRefDType)) {
                                ifacerefp = VN_AS(varp->subDTypep(), IfaceRefDType);
                            } 否则 如果 (VN_IS(subtypep, BracketArrayDType)) {
                                const AstBracketArrayDType* const arrp
                                    = VN_AS(subtypep, BracketArrayDType);
                                const AstNodeDType* const arrsubtypep = arrp->subDTypep();
                                如果 (VN_IS(arrsubtypep, IfaceRefDType)) {
                                    ifacerefp = VN_AS(arrsubtypep, IfaceRefDType);
                                }
                            } 否则 如果 (VN_IS(subtypep, UnpackArrayDType)) {
                                const AstUnpackArrayDType* const arrp
                                    = VN_AS(subtypep, UnpackArrayDType);
                                const AstNodeDType* const arrsubtypep = arrp->subDTypep();
                                如果 (VN_IS(arrsubtypep, IfaceRefDType)) {
                                    ifacerefp = VN_AS(arrsubtypep, IfaceRefDType);
                                }
                            }

                            如果 (ifacerefp && !ifacerefp->cellp()) {
                                // A dummy cell to keep the top level interface alive and correctly
                                // optimized for default parameter values
                                AstCell* ifacecellp
                                    = new AstCell{nodep->fileline(),
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
    void visit(AstTypeTable*) override {}  // FindVisitor::
    void visit(AstIfaceRefDType* nodep) override {  // FindVisitor::
        如果 (!m_statep->forScopeCreation() && nodep->isVirtual() && nodep->ifacep()
            && !nodep->ifacep()->user3()) {
            m_virtIfaces.push_back(nodep->ifacep());
            nodep->ifacep()->user3(true);
        }
        iterateChildren(nodep);
    }
    void visit(AstNodeModule* nodep) override {  // FindVisitor::
        // Called on top module from Netlist, other modules from the cell creating them,
        // and packages
        UINFO(8, "   " << nodep);
        // m_curSymp/m_modSymp maybe nullptr for packages and non-top modules
        // Packages will be under top after the initial phases, but until then
        // need separate handling
        const bool standalonePkg
            = !m_modSymp && !m_statep->forScopeCreation() && VN_IS(nodep, Package);
        const bool doit = (m_modSymp || standalonePkg);
        VL_RESTORER_COPY(m_scope);
        VL_RESTORER(m_classOrPackagep);
        VL_RESTORER(m_modSymp);
        VL_RESTORER(m_curSymp);
        VL_RESTORER(m_paramNum);
        VL_RESTORER(m_modBlockNum);
        VL_RESTORER(m_modWithNum);
        VL_RESTORER(m_modArgNum);
        如果 (doit && nodep->user2()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Identically recursive module (module instantiates "
                          "itself, without changing parameters): "
                              << AstNode::prettyNameQ(nodep->origName()));
        } 否则 如果 (doit) {
            UINFO(4, "     Link Module: " << nodep);
            UASSERT_OBJ(!nodep->dead(), nodep, "Module in instance tree mislabeled as dead?");
            AstPackage* const pkgp = VN_CAST(nodep, Package);
            m_classOrPackagep = pkgp;
            如果 (standalonePkg) {
                如果 (pkgp->isDollarUnit()) {
                    m_curSymp = m_modSymp = m_statep->dunitEntp();
                    nodep->user1p(m_curSymp);
                } 否则 {
                    VSymEnt* const upperSymp = m_statep->dunitEntp();
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
            nodep->user2(false);
            nodep->user4(true);
            // Interfaces need another pass when signals are resolved. When creating
            // scopes this is done from the AstScopes in LinkDotScopeVisitor instead, as
            // only those entries hold the AstVarScopes the modport items resolve against.
            如果 (!m_statep->forScopeCreation()) {
                如果 (AstIface* const ifacep = VN_CAST(nodep, Iface)) {
                    m_statep->insertIfaceModSym(ifacep, m_curSymp);
                }
            }
        } 否则 如果 (isHierBlockWrapper(nodep->name())) {
            UINFO(5, "Module is hierarchical block, must not be dead: " << nodep);
            m_scope = nodep->name();
            VSymEnt* const upperSymp = m_curSymp ? m_curSymp : m_statep->rootEntp();
            m_curSymp = m_modSymp
                = m_statep->insertBlock(upperSymp, nodep->name() + "::", nodep, m_classOrPackagep);
            iterateChildren(nodep);
            nodep->user4(true);
        } 否则 {  // !doit
            如果 (nodep->hierParams()) {
                UINFO(1, "Found module with hier type parameters");
                m_hierParamsName = nodep->name();
                为 (const AstNode* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    如果 (const AstTypedef* const tdef = VN_CAST(stmtp, Typedef)) {
                        UINFO(1, "Inserting hier type parameter typedef: " << tdef);
                        VSymEnt* const upperSymp = m_curSymp ? m_curSymp : m_statep->rootEntp();
                        m_curSymp = m_modSymp = m_statep->insertBlock(upperSymp, nodep->name(),
                                                                      nodep, m_classOrPackagep);
                    }
                }
            } 否则 {
                // Will be optimized away later
                // Can't remove now, as our backwards iterator will throw up
                UINFO(5, "Module not under any CELL or top - dead module: " << nodep);
            }
        }
    }

    void visit(AstClass* nodep) override {  // FindVisitor::
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
            VSymEnt* const upperSymp = m_curSymp;
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
            m_explicitNew = false;
            // m_modSymp/m_curSymp for non-packages set by AstCell above this module
            // Iterate
            iterateChildren(nodep);
            nodep->user4(true);
            // Implicit new needed?
            如果 (!m_explicitNew && m_statep->forPrimary()) makeImplicitNew(nodep);
        }
    }
    void visit(AstClassOrPackageRef* nodep) override {  // FindVisitor::
        如果 (!nodep->classOrPackageNodep() && nodep->name() == "$unit") {
            nodep->classOrPackageNodep(v3Global.rootp()->dollarUnitPkgp());
        }
        iterateChildren(nodep);
    }
    void visit(AstScope* nodep) override {  // FindVisitor::
        UASSERT_OBJ(m_statep->forScopeCreation(), nodep,
                    "Scopes should only exist right after V3Scope");
        // Detach on first encounter, will re add at the end.
        // Otherwise would be visited O(cells^2) time after V3Scope
        如果 (!nodep->isTop()) {
            ++m_statScopeVisits;
            nodep->unlinkFrBack();
            m_scopes.push_back(nodep);
        }
    }
    void visit(AstCell* nodep) override {  // FindVisitor::
        UINFO(5, "   CELL under " << m_scope << " is " << nodep);
        // Process XREFs/etc inside pins
        如果 (nodep->recursive() && m_inRecursion) 返回;
        iterateChildren(nodep);
        // Recurse in, preserving state
        VL_RESTORER_COPY(m_scope);
        VL_RESTORER(m_modSymp);
        VL_RESTORER(m_curSymp);
        VL_RESTORER(m_paramNum);
        VL_RESTORER(m_inRecursion);
        // Where do we add it?
        VSymEnt* aboveSymp = m_curSymp;
        const string origname = AstNode::dedotName(nodep->name());
        string::size_type pos = origname.rfind('.');
        如果 (pos != string::npos) {
            // Flattened, find what CellInline it should live under
            const string scope = origname.substr(0, pos);
            string baddot;
            VSymEnt* okSymp;
            aboveSymp
                = m_statep->findDotted(nodep->fileline(), aboveSymp, scope, baddot, okSymp, false);
            UASSERT_OBJ(aboveSymp, nodep,
                        "Can't find instance insertion point at "
                            << AstNode::prettyNameQ(baddot) << " in: " << nodep->prettyNameQ());
        }
        {
            m_scope = m_scope + "." + nodep->name();
            m_curSymp = m_modSymp = m_statep->insertCell(aboveSymp, m_modSymp, nodep, m_scope);
            m_inRecursion = nodep->recursive();
            // We don't report NotFoundModule, as may be a unused module in a generate
            如果 (nodep->modp()) iterate(nodep->modp());
        }
    }
    void visit(AstCellInline* nodep) override {  // FindVisitor::
        UINFO(5, "   CELLINLINE under " << m_scope << " is " << nodep);
        VSymEnt* aboveSymp = m_curSymp;
        std::string ident = nodep->name();
        // If baz__DOT__foo__DOT__bar, we need to find baz__DOT__foo and add bar to it.
        const std::string::size_type pos = ident.rfind("__DOT__");
        如果 (pos != std::string::npos) {
            const std::string dotted = ident.substr(0, pos);
            ident = ident.substr(pos + std::strlen("__DOT__"));
            std::string baddot;
            VSymEnt* okSymp;
            aboveSymp = m_statep->findDotted(nodep->fileline(), aboveSymp, dotted, baddot, okSymp,
                                             false);
            UASSERT_OBJ(aboveSymp, nodep,
                        "Can't find cellinline insertion point at "
                            << AstNode::prettyNameQ(baddot) << " in: " << nodep->prettyNameQ());
        }
        m_statep->insertInline(aboveSymp, m_modSymp, nodep, ident);
    }
    void visit(AstDefParam* nodep) override {  // FindVisitor::
        nodep->user1p(m_curSymp);
        iterateChildren(nodep);
    }
    void visit(AstRefDType* nodep) override {  // FindVisitor::
        nodep->user1p(m_curSymp);
        iterateChildren(nodep);
    }
    void visit(AstGenBlock* nodep) override {  // FindVisitor::
        UINFO(5, "   " << nodep);
        如果 (nodep->name() == "" && nodep->unnamed()) {
            // Unnamed blocks are only important when they contain var
            // decls, so search for them. (Otherwise adding all the
            // unnamed#'s would just confuse tracing variables in
            // places such as tasks, where "task ...; begin ... end"
            // are common.
            为 (AstNode* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
                如果 (VN_IS(itemp, Var) || VN_IS(itemp, Foreach)) {
                    std::string name;
                    const std::string stepStr = m_statep->forPrimary()
                                                    ? ""
                                                    : std::to_string(m_statep->stepNumber()) + "_";
                    do {
                        ++m_modBlockNum;
                        name = "unnamedblk" + stepStr + cvtToStr(m_modBlockNum);
                        // Increment again if earlier pass of V3LinkDot claimed this name
                    } 当 (m_curSymp->findIdFlat(name));
                    nodep->name(name);
                    break;
                }
            }
        }
        如果 (nodep->name() == "") {
            iterateChildren(nodep);
        } 否则 {
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
    void visit(AstNodeBlock* nodep) override {  // FindVisitor::
        UINFO(5, "   " << nodep);
        如果 (nodep->name() == "" && nodep->unnamed()) {
            // Unnamed blocks are only important when they contain var
            // decls, so search for them. (Otherwise adding all the
            // unnamed#'s would just confuse tracing variables in
            // places such as tasks, where "task ...; begin ... end"
            // are common.
            const auto containsDecl = [](const AstNode* nodesp) -> bool {
                为 (const AstNode* np = nodesp; np; np = np->nextp()) {
                    如果 (VN_IS(np, Var) || VN_IS(np, Foreach)) 返回 true;
                }
                返回 false;
            };
            bool needName = nodep->declsp() || containsDecl(nodep->stmtsp());
            如果 (const AstFork* const forkp = VN_CAST(nodep, Fork)) {
                needName = needName || containsDecl(forkp->forksp());
            }
            如果 (needName) {
                const std::string stepStr
                    = m_statep->forPrimary() ? "" : std::to_string(m_statep->stepNumber()) + "_";
                std::string name;
                do {
                    ++m_modBlockNum;
                    name = "unnamedblk" + stepStr + cvtToStr(m_modBlockNum);
                    // Increment again if earlier pass of V3LinkDot claimed this name
                } 当 (m_curSymp->findIdFlat(name));
                nodep->name(name);
            }
        }
        如果 (nodep->name() == "") {
            iterateChildren(nodep);
        } 否则 {
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
    void moveIfaceExportBody(AstNodeFTask* nodep) {
        const string& portName = nodep->ifacePortName();
        UINFO(5, "   moveIfaceExportBody: port=" << portName << " task=" << nodep->name());
        // Look up the interface port variable in the module's symbol table
        VSymEnt* const portSymp = m_modSymp->findIdFallback(portName);
        如果 (!portSymp) {
            nodep->v3error("Interface port not found for out-of-block definition: '"
                           << portName << "' (IEEE 1800-2023 25.7)");
            nodep->unlinkFrBack();
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
            返回;
        }
        AstVar* const portp = VN_CAST(portSymp->nodep(), Var);
        如果 (!portp) {
            nodep->v3fatalSrc("Out-of-block definition port is not a variable: " << portName);
            返回;
        }
        // Get the interface name from the port's type
        // At this stage dtypep() may not be set yet; check childDTypep() too
        AstIfaceRefDType* ifaceRefDtp = nullptr;
        如果 (portp->dtypep()) { ifaceRefDtp = VN_CAST(portp->dtypep(), IfaceRefDType); }
        如果 (!ifaceRefDtp && portp->childDTypep()) {
            ifaceRefDtp = VN_CAST(portp->childDTypep(), IfaceRefDType);
        }
        如果 (!ifaceRefDtp) {
            nodep->v3error("Out-of-block definition port is not an interface port: '"
                           << portName << "' (IEEE 1800-2023 25.7)");
            nodep->unlinkFrBack();
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
            返回;
        }
        const string ifaceName = ifaceRefDtp->ifaceName();
        UINFO(5, "   moveIfaceExportBody: iface=" << ifaceName);
        // Find the interface module by name
        AstNodeModule* const ifaceModp = m_statep->findModuleSym(ifaceName);
        如果 (!ifaceModp) {
            nodep->v3fatalSrc("Interface not found for out-of-block definition: " << ifaceName);
            返回;
        }
        // Find the prototype task in the interface
        AstNodeFTask* protoTaskp = nullptr;
        为 (AstNode* itemp = ifaceModp->stmtsp(); itemp; itemp = itemp->nextp()) {
            如果 (AstNodeFTask* const ftaskp = VN_CAST(itemp, NodeFTask)) {
                如果 (ftaskp->name() == nodep->name() && ftaskp->prototype()) {
                    protoTaskp = ftaskp;
                    break;
                }
            }
        }
        如果 (!protoTaskp) {
            nodep->v3error("No matching export prototype found in interface "
                           << ifaceName << " for task " << nodep->prettyNameQ()
                           << " (IEEE 1800-2023 25.7)");
            nodep->unlinkFrBack();
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
            返回;
        }
        // Move body statements (non-port declarations) from the out-of-block task
        // to the prototype task. Rewrite port.X references to just X in the body.
        为 (AstNode* stmtp = nodep->stmtsp(); stmtp;) {
            AstNode* const nextp = stmtp->nextp();
            // Skip port declarations -- they already exist in the prototype
            如果 (const AstVar* const varp = VN_CAST(stmtp, Var)) {
                如果 (varp->isIO()) {
                    stmtp = nextp;
                    continue;
                }
            }
            // Clone this statement and rewrite port references
            AstNode* const newStmtp = stmtp->cloneTree(false);
            rewriteIfacePortRefsSingle(newStmtp, portName);
            protoTaskp->addStmtsp(newStmtp);
            stmtp = nextp;
        }
        // Mark prototype as no longer prototype (it has a body now)
        protoTaskp->prototype(false);
        // Delete the out-of-block task from the module
        nodep->unlinkFrBack();
        VL_DO_DANGLING(pushDeletep(nodep), nodep);
    }

    // Rewrite AstDot nodes where lhsp is AstParseRef with the port name
    // to just the rhsp (stripping the port prefix).
    static void rewriteIfacePortRefsSingle(AstNode* nodep, const string& portName) {
        如果 (!nodep) 返回;
        // Check if this is a non-colon Dot with lhs matching portName
        如果 (AstDot* const dotp = VN_CAST(nodep, Dot)) {
            如果 (!dotp->colon()) {
                const AstParseRef* const lhsRefp = VN_CAST(dotp->lhsp(), ParseRef);
                如果 (lhsRefp && lhsRefp->name() == portName) {
                    UINFO(5, "   rewriteIfacePortRefs: stripping port prefix from " << dotp);
                    AstNode* const rhsp = dotp->rhsp()->unlinkFrBack();
                    dotp->replaceWith(rhsp);
                    VL_DO_DANGLING(dotp->deleteTree(), dotp);
                    rewriteIfacePortRefsSingle(rhsp, portName);
                    返回;
                }
            }
        }
        // Recurse into all children
        为 (AstNode* childp : {nodep->op1p(), nodep->op2p(), nodep->op3p(), nodep->op4p()}) {
            当 (childp) {
                AstNode* const nextp = childp->nextp();
                rewriteIfacePortRefsSingle(childp, portName);
                childp = nextp;
            }
        }
    }

    void visit(AstNodeFTask* nodep) override {  // FindVisitor::
        // NodeTask: Remember its name for later resolution
        UINFO(5, "   " << nodep);
        UASSERT_OBJ(m_curSymp && m_modSymp, nodep, "Function/Task not under module?");
        如果 (nodep->name() == "new") m_explicitNew = true;
        // Handle out-of-block interface method definition (IEEE 25.8)
        如果 (!nodep->ifacePortName().empty() && m_statep->forPrimary()) {
            moveIfaceExportBody(nodep);
            返回;  // Task has been deleted
        }
        // Remember the existing symbol table scope
        VL_RESTORER(m_classOrPackagep);
        VL_RESTORER(m_curSymp);
        VSymEnt* upSymp = m_curSymp;

        如果 (VN_IS(m_curSymp->nodep(), Class)) {
            如果 (VN_AS(m_curSymp->nodep(), Class)->isInterfaceClass() && !nodep->pureVirtual()
                && !nodep->isConstructor()) {
                nodep->v3error("Interface class functions must be pure virtual"
                               << " (IEEE 1800-2023 8.26): " << nodep->prettyNameQ());
            }
            如果 (m_statep->forPrimary()
                && (nodep->name() == "randomize" || nodep->name() == "srandom")) {
                nodep->v3error(nodep->prettyNameQ()
                               << " is a predefined class method; redefinition not allowed"
                                  " (IEEE 1800-2023 18.6.3)");
            }
        }
        // Change to appropriate package if extern declaration (vs definition)
        如果 (nodep->classOrPackagep()) {
            // class-in-class
            AstDot* const dotp = VN_CAST(nodep->classOrPackagep(), Dot);
            AstClassOrPackageRef* const cpackagerefp
                = VN_CAST(nodep->classOrPackagep(), ClassOrPackageRef);
            如果 (dotp) {
                AstClassOrPackageRef* const lhsp = VN_AS(dotp->lhsp(), ClassOrPackageRef);
                m_statep->resolveClassOrPackage(m_curSymp, lhsp, true, false,
                                                "External definition :: reference");
                AstClass* const lhsclassp = VN_CAST(lhsp->classOrPackageSkipp(), Class);
                如果 (!lhsclassp) {
                    nodep->v3error("Extern declaration's scope is not a defined class");
                } 否则 {
                    m_curSymp = m_statep->getNodeSym(lhsclassp);
                    upSymp = m_curSymp;
                    AstClassOrPackageRef* const rhsp = VN_AS(dotp->rhsp(), ClassOrPackageRef);
                    m_statep->resolveClassOrPackage(m_curSymp, rhsp, true, false,
                                                    "External definition :: reference");
                    AstClass* const rhsclassp = VN_CAST(rhsp->classOrPackageSkipp(), Class);
                    如果 (!rhsclassp) {
                        nodep->v3error("Extern declaration's scope is not a defined class");
                    } 否则 {
                        m_curSymp = m_statep->getNodeSym(rhsclassp);
                        upSymp = m_curSymp;
                        moveExternFuncDecl(nodep, rhsclassp);
                    }
                }
            } 否则 如果 (cpackagerefp) {
                如果 (!cpackagerefp->classOrPackageSkipp()) {
                    m_statep->resolveClassOrPackage(m_curSymp, cpackagerefp, true, false,
                                                    "External definition :: reference");
                }
                AstClass* const classp = VN_CAST(cpackagerefp->classOrPackageSkipp(), Class);
                如果 (!classp) {
                    nodep->v3error("Extern declaration's scope is not a defined class");
                } 否则 {
                    m_curSymp = m_statep->getNodeSym(classp);
                    upSymp = m_curSymp;
                    moveExternFuncDecl(nodep, classp);
                }
            } 否则 {
                v3fatalSrc("Unhandled extern function definition package");
            }
        }
        // Set the class as package for iteration
        如果 (VN_IS(m_curSymp->nodep(), Class)) {
            m_classOrPackagep = VN_AS(m_curSymp->nodep(), Class);
        }
        // Create symbol table for the task's vars
        const string name = (nodep->isExternProto() ? "extern "s : ""s) + nodep->name();
        m_curSymp = m_statep->insertBlock(m_curSymp, name, nodep, m_classOrPackagep);
        m_curSymp->fallbackp(upSymp);
        // Convert the func's range to the output variable
        // This should probably be done in the Parser instead, as then we could
        // just attach normal signal attributes to it.
        如果 (!nodep->isExternProto() && nodep->fvarp() && !VN_IS(nodep->fvarp(), Var)) {
            AstNodeDType* dtypep = VN_CAST(nodep->fvarp(), NodeDType);
            // If unspecified, function returns one bit; however when we
            // support NEW() it could also return the class reference.
            如果 (dtypep) {
                dtypep->unlinkFrBack();
            } 否则 {
                dtypep = new AstBasicDType{nodep->fileline(), VBasicDTypeKwd::LOGIC};
            }
            AstVar* const newvarp
                = new AstVar{nodep->fileline(), VVarType::VAR, nodep->name(), VFlagChildDType{},
                             dtypep};  // Not dtype resolved yet
            newvarp->direction(VDirection::OUTPUT);
            newvarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            newvarp->funcReturn(true);
            newvarp->trace(false);  // Not user visible
            nodep->fvarp(newvarp);
            // Explicit insert required, as the var name shadows the upper level's task name
            m_statep->insertSym(m_curSymp, newvarp->name(), newvarp, nullptr /*classOrPackagep*/);
        }
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildren(nodep);
    }
    void visit(AstClocking* nodep) override {  // FindVisitor::
        VL_RESTORER(m_clockingp);
        m_clockingp = nodep;
        iterate(nodep->sentreep());
        iterateAndNextNull(nodep->itemsp());
        // If the block has no name, one cannot reference the clockvars
        VSymEnt* itSymp = nullptr;
        如果 (nodep->isGlobal()  //
            && m_statep->forPrimary()) {  // else flattening may see two globals
            m_statep->checkDuplicate(m_curSymp, nodep, "__024global_clock");
            itSymp
                = m_statep->insertBlock(m_curSymp, "__024global_clock", nodep, m_classOrPackagep);
            itSymp->fallbackp(nullptr);
        }
        如果 (!nodep->name().empty()) {
            itSymp = m_statep->insertBlock(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
            itSymp->fallbackp(nullptr);
        }
        如果 (itSymp) {
            VL_RESTORER(m_curSymp);
            m_curSymp = itSymp;
            iterateAndNextNull(nodep->itemsp());
        }
    }
    void visit(AstClockingItem* nodep) override {  // FindVisitor::
        如果 (nodep->varp()) {
            如果 (m_curSymp->nodep() == m_clockingp) iterate(nodep->varp());
            返回;
        }
        std::string varname;
        AstNodeDType* dtypep;
        如果 (AstAssign* const assignp = nodep->assignp()) {
            AstNodeExpr* const rhsp = assignp->rhsp()->unlinkFrBack();
            dtypep = new AstRefDType{nodep->fileline(), AstRefDType::FlagTypeOfExpr{},
                                     rhsp->cloneTree(false)};
            nodep->exprp(rhsp);
            varname = assignp->lhsp()->name();
            VL_DO_DANGLING(assignp->unlinkFrBack()->deleteTree(), assignp);
        } 否则 {
            AstNodeExpr* const refp = nodep->exprp();
            const VSymEnt* foundp = m_curSymp->findIdFallback(refp->name());
            如果 (!foundp || !foundp->nodep()) {
                refp->v3error("Corresponding variable " << refp->prettyNameQ()
                                                        << " does not exist");
                VL_DO_DANGLING(nodep->unlinkFrBack()->deleteTree(), nodep);
                返回;
            }
            varname = refp->name();
            dtypep = VN_AS(foundp->nodep(), Var)->childDTypep()->cloneTree(false);
        }
        AstVar* const newvarp = new AstVar{nodep->fileline(), VVarType::MODULETEMP, varname,
                                           VFlagChildDType{}, dtypep};
        newvarp->lifetime(VLifetime::STATIC_EXPLICIT);
        nodep->varp(newvarp);
        iterate(nodep->exprp());
    }
    void visit(AstConstraint* nodep) override {  // FindVisitor::
        VL_RESTORER(m_curSymp);
        // Change to appropriate package if extern declaration (vs definition)
        VSymEnt* upSymp = m_curSymp;
        如果 (nodep->classOrPackagep()) {
            AstClassOrPackageRef* const cpackagerefp
                = VN_CAST(nodep->classOrPackagep(), ClassOrPackageRef);
            如果 (!cpackagerefp) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: extern constraint definition with class-in-class");
            } 否则 {
                如果 (!cpackagerefp->classOrPackageSkipp()) {
                    m_statep->resolveClassOrPackage(m_curSymp, cpackagerefp, true, false,
                                                    "External definition :: reference");
                }
                AstClass* const classp = VN_CAST(cpackagerefp->classOrPackageSkipp(), Class);
                如果 (!classp) {
                    nodep->v3error("Extern declaration's scope is not a defined class");
                } 否则 {
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
        const string name = (nodep->isExternProto() ? "extern "s : ""s) + nodep->name();
        m_curSymp = m_statep->insertBlock(upSymp, name, nodep, m_classOrPackagep);
        iterateChildren(nodep);
    }
    void visit(AstVar* nodep) override {  // FindVisitor::
        // Var: Remember its name for later resolution
        UASSERT_OBJ(m_curSymp && m_modSymp, nodep, "Var not under module?");
        iterateChildren(nodep);
        如果 (VN_IS(m_curSymp->nodep(), Class)
            && VN_AS(m_curSymp->nodep(), Class)->isInterfaceClass() && !nodep->isParam()) {
            nodep->v3error("Interface class cannot contain non-parameter members"
                           << " (IEEE 1800-2023 8.26): " << nodep->prettyNameQ());
        }
        如果 (!m_statep->forScopeCreation()) {
            // Find under either a task or the module's vars
            const VSymEnt* foundp = m_curSymp->findIdFallback(nodep->name());
            如果 (!foundp && m_modSymp && nodep->name() == m_modSymp->nodep()->name()) {
                foundp = m_modSymp;  // Conflicts with modname?
            }
            AstVar* const findvarp = foundp ? VN_CAST(foundp->nodep(), Var) : nullptr;
            // clocking items can have duplicate names (inout)
            如果 (findvarp && VN_IS(findvarp->backp(), ClockingItem)
                && VN_IS(nodep->backp(), ClockingItem)) {
                AstClockingItem* const itemp = VN_AS(nodep->backp(), ClockingItem);
                AstClockingItem* const finditemp = VN_AS(findvarp->backp(), ClockingItem);
                UINFO(4, "ClockCompl: " << itemp << " ;; " << finditemp);
                UINFO(4, "ClockCompV: " << nodep << " ;; " << findvarp);
                如果 (*itemp->exprp()->fileline() == *finditemp->exprp()->fileline()) {
                    UASSERT_OBJ(finditemp->direction() == VDirection::INPUT
                                    && itemp->direction() == VDirection::OUTPUT,
                                itemp, "Input after output?");
                    // pretend nothing found and rename
                    foundp = nullptr;
                    nodep->name("__Voutput_" + nodep->name());
                    finditemp->outputp(itemp);
                }
            }
            bool ins = false;
            如果 (!foundp) {
                ins = true;
            } 否则 如果 (!findvarp && m_curSymp->findIdFlat(nodep->name()) && !foundp->imported()) {
                nodep->v3error("Unsupported in C: Variable has same name as "
                               << LinkDotState::nodeTextType(foundp->nodep()) << ": "
                               << nodep->prettyNameQ());
            } 否则 如果 (findvarp != nodep) {
                UINFO(4, "DupVar: " << nodep << " ;; " << foundp->nodep());
                UINFO(4, "    found  cur=se" << cvtToHex(m_curSymp) << " ;; parent=se"
                                             << cvtToHex(foundp->parentp()));
                如果 (foundp->parentp() == m_curSymp  // Only when on same level
                    && !foundp->imported()) {  // and not from package
                    如果 (VN_IS(m_curSymp->nodep(), Clocking)) {
                        nodep->v3error("Multiple clockvars with the same name not allowed");
                        返回;
                    }
                    const bool nansiBad
                        = (((findvarp->isDeclTyped() || findvarp->isNet())
                            && (nodep->isDeclTyped() || nodep->isNet()))
                           || (findvarp->isIO() && nodep->isIO())  // e.g. !(output && output)
                           || findvarp->gotNansiType());  // e.g. int x && int x
                    const bool ansiBad
                        = findvarp->isAnsi() || nodep->isAnsi();  // dup illegal with ANSI
                    如果 (ansiBad || nansiBad) {
                        bool ansiWarn = ansiBad && !nansiBad;
                        如果 (ansiWarn) {
                            static 整数 s_didAnsiWarn = false;
                            如果 (s_didAnsiWarn++) ansiWarn = false;
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
                    } 否则 {
                        findvarp->combineType(nodep);
                        findvarp->fileline()->modifyStateInherit(nodep->fileline());
                        findvarp->gotNansiType(true);
                        UASSERT_OBJ(nodep->subDTypep(), nodep, "Var has no type");
                        如果 (nodep->subDTypep()->numeric().isSigned()
                            && !findvarp->subDTypep()->numeric().isSigned()) {
                            findvarp->subDTypep()->numeric(VSigning{true});
                        }
                        AstNodeDType* const varDtp = findvarp->subDTypep();
                        AstNodeDType* const otherDtp = nodep->subDTypep();
                        如果 (varDtp
                            && (VN_IS(varDtp, BasicDType)
                                && VN_AS(varDtp, BasicDType)->implicit())) {
                            // Then have "input foo" and "real foo" so the
                            // dtype comes from the other side.
                            VL_DO_DANGLING(varDtp->unlinkFrBack()->deleteTree(), varDtp);
                            findvarp->childDTypep(otherDtp->unlinkFrBack());
                        } 否则 如果 (!m_statep->forScopeCreation() && otherDtp && varDtp
                                   && !(VN_IS(otherDtp, BasicDType)
                                        && VN_AS(otherDtp, BasicDType)->implicit())) {
                            // otherDtp and varDtp both non-nullptr and neither are implicit
                            // Can't compare dtypes now as might contain parameters,
                            // defer to V3Width
                            AstAttrOf* const newp
                                = new AstAttrOf{otherDtp->fileline(), VAttrType::VAR_PORT_DTYPE,
                                                otherDtp->unlinkFrBack()};
                            findvarp->addAttrsp(newp);
                        }
                    }
                    VL_DO_DANGLING(nodep->unlinkFrBack()->deleteTree(), nodep);
                } 否则 {
                    // User can disable the message at either point
                    如果 (!(m_ftaskp && m_ftaskp->dpiImport())
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
            如果 (ins) {
                如果 (m_statep->forPrimary() && nodep->isGParam()
                    && VN_IS(m_modSymp->nodep(), Module)
                    && (m_statep->rootEntp()->nodep() == m_modSymp->parentp()->nodep())) {
                    // This is the toplevel module. Check for command line overwrites of parameters
                    // We first search if the parameter is overwritten and then replace it with a
                    // new value.
                    如果 (v3Global.opt.hasParameter(nodep->name())) {
                        const string svalue = v3Global.opt.parameter(nodep->name());
                        如果 (AstConst* const valuep
                            = AstConst::parseParamLiteral(nodep->fileline(), svalue)) {
                            UINFO(9, "       replace parameter " << nodep);
                            UINFO(9, "       with " << valuep);
                            如果 (nodep->valuep()) pushDeletep(nodep->valuep()->unlinkFrBack());
                            nodep->valuep(valuep);
                            // Do not warn ENUMVALUE when overriding an enum param from the cli.
                            nodep->fileline()->warnOff(V3ErrorCode::ENUMVALUE, true);
                        }
                    }
                }
                m_statep->insertSym(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
                如果 (m_statep->forPrimary() && nodep->isGParam()) {
                    ++m_paramNum;
                    VSymEnt* const symp
                        = m_statep->insertSym(m_curSymp, "__paramNumber" + cvtToStr(m_paramNum),
                                              nodep, m_classOrPackagep);
                    symp->exported(false);
                }
            }
        }
    }
    void visit(AstTypedef* nodep) override {  // FindVisitor::
        UASSERT_OBJ(m_curSymp, nodep, "Typedef not under module/package/$unit");
        如果 (VN_IS(m_classOrPackagep, Class)) nodep->isUnderClass(true);
        iterateChildren(nodep);
        m_statep->insertSym(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
    }
    void visit(AstTypedefFwd* nodep) override {  // FindVisitor::
        UASSERT_OBJ(m_curSymp, nodep, "Typedef not under module/package/$unit");
        iterateChildren(nodep);
        // No need to insert, only the real typedef matters, but need to track for errors
        nodep->user1p(m_curSymp);
    }
    void visit(AstNodeUOrStructDType* nodep) override {  // FindVisitor::
        UASSERT_OBJ(m_curSymp, nodep, "Struct/union dtype not under module/package/$unit");
        VL_RESTORER(m_curSymp);
        m_curSymp = m_statep->insertBlock(m_curSymp, "__Vdtype" + cvtToStr(nodep->uniqueNum()),
                                          nodep, m_classOrPackagep);
        iterateChildren(nodep);
    }
    void visit(AstMemberDType* nodep) override {  // FindVisitor::
        iterateChildren(nodep);
        m_statep->insertSym(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
    }
    void visit(AstParamTypeDType* nodep) override {  // FindVisitor::
        UASSERT_OBJ(m_curSymp, nodep, "Parameter type not under module/package/$unit");

        // Replace missing param types with provided hierarchical type params.
        如果 (!m_hierParamsName.empty()) {
            如果 (const VSymEnt* const typedefEntp = m_curSymp->findIdFallback(m_hierParamsName)) {
                const AstModule* modp = VN_CAST(typedefEntp->nodep(), Module);

                为 (const AstNode* stmtp = modp ? modp->stmtsp() : nullptr; stmtp;
                     stmtp = stmtp->nextp()) {
                    const AstTypedef* tdefp = VN_CAST(stmtp, Typedef);

                    如果 (tdefp && tdefp->name() == nodep->name() && m_statep->forPrimary()) {
                        UINFO(8, "Replacing type of" << nodep << "  -->with " << tdefp);
                        AstNodeDType* const newDtp = tdefp->childDTypep();
                        AstNodeDType* const oldDtp = nodep->childDTypep();

                        oldDtp->replaceWith(newDtp->cloneTree(false));
                        VL_DO_DANGLING(oldDtp->deleteTree(), oldDtp);
                    }
                }
            }
        }

        iterateChildren(nodep);
        m_statep->insertSym(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
        如果 (m_statep->forPrimary() && nodep->isGParam()) {
            ++m_paramNum;
            VSymEnt* const symp = m_statep->insertSym(
                m_curSymp, "__paramNumber" + cvtToStr(m_paramNum), nodep, m_classOrPackagep);
            symp->exported(false);
        }
    }
    void visit(AstCFunc* nodep) override {  // FindVisitor::
        // For dotted resolution, ignore all AstVars under functions, otherwise shouldn't exist
        UASSERT_OBJ(!m_statep->forScopeCreation(), nodep, "No CFuncs expected in tree yet");
    }
    void visit(AstEnumItem* nodep) override {  // FindVisitor::
        // EnumItem: Remember its name for later resolution
        iterateChildren(nodep);
        // Find under either a task or the module's vars
        const VSymEnt* foundp = m_curSymp->findIdFallback(nodep->name());
        如果 (!foundp && m_modSymp && nodep->name() == m_modSymp->nodep()->name()) {
            foundp = m_modSymp;  // Conflicts with modname?
        }
        AstEnumItem* const findvarp = foundp ? VN_CAST(foundp->nodep(), EnumItem) : nullptr;
        bool ins = false;
        如果 (!foundp) {
            ins = true;
        } 否则 如果 (findvarp != nodep) {
            UINFO(4, "DupVar: " << nodep << " ;; " << foundp);
            如果 (foundp->parentp() == m_curSymp  // Only when on same level
                && !foundp->imported()) {  // and not from package
                nodep->v3error("Duplicate declaration of enum value: "
                               << nodep->prettyName() << '\n'
                               << nodep->warnContextPrimary() << '\n'
                               << foundp->nodep()->warnOther()
                               << "... Location of original declaration\n"
                               << foundp->nodep()->warnContextSecondary());
            } 否则 {
                // User can disable the message at either point
                如果 (!nodep->fileline()->warnIsOff(V3ErrorCode::VARHIDDEN)
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
        如果 (ins) m_statep->insertSym(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
    }
    void visit(AstPackageImport* nodep) override {  // FindVisitor::
        UINFO(4, "  Link: " << nodep);
        如果 (!nodep->packagep()) 返回;  // Errored in V3LinkCells
        VSymEnt* const srcp = m_statep->getNodeSym(nodep->packagep());
        如果 (nodep->name() == "*") {
            如果 (nodep->packagep() != v3Global.rootp()->stdPackagep()) {
                如果 (m_curSymp == m_statep->dunitEntp()) {
                    nodep->v3warn(IMPORTSTAR,
                                  "'import::*' in $unit scope may pollute global namespace");
                }
            }
        } 否则 如果 (!nodep->resolvedClassp()) {
            VSymEnt* const impp = srcp->findIdFlat(nodep->name());
            如果 (!impp) {
                nodep->v3error("Import object not found: " << nodep->prettyPkgNameQ());
            } 否则 如果 (nodep->resolvedClassp()) {
                UASSERT_OBJ(nodep->resolvedClassp()->classOrPackageSkipp() == impp->nodep(), nodep,
                            "Import should resolve to the same node");
            } 否则 如果 (AstClass* const classp = VN_CAST(impp->nodep(), Class)) {
                nodep->resolvedClassp(
                    new AstClassOrPackageRef{nodep->fileline(), classp->name(), classp, nullptr});
            }
        }
        m_curSymp->importFromPackage(m_statep->symsp(), srcp, nodep->name());
        UINFO(9, "    Link Done: " << nodep);
        // No longer needed, but can't delete until any multi-instantiated modules are expanded
    }
    void visit(AstPackageExport* nodep) override {  // FindVisitor::
        UINFO(9, "  Link: " << nodep);
        如果 (!nodep->packagep()) 返回;  // Errored in V3LinkCells
        VSymEnt* const srcp = m_statep->getNodeSym(nodep->packagep());
        如果 (nodep->name() != "*") {
            VSymEnt* const impp = srcp->findIdFlat(nodep->name());
            如果 (!impp) {
                nodep->v3error("Export object not found: '" << nodep->packagep()->prettyName()
                                                            << "::" << nodep->prettyName() << "'");
            }
        }
        m_curSymp->exportFromPackage(m_statep->symsp(), srcp, nodep->name());
        UINFO(9, "    Link Done: " << nodep);
        // No longer needed, but can't delete until any multi-instantiated modules are expanded
    }
    void visit(AstPackageExportStarStar* nodep) override {  // FindVisitor::
        UINFO(4, "  Link: " << nodep);
        m_curSymp->exportStarStar(m_statep->symsp());
        // No longer needed, but can't delete until any multi-instantiated modules are expanded
    }

    void visit(AstNodeForeach* nodep) override {  // FindVisitor::
        // Symbol table needs nodep->name() as the index variable's name
        VL_RESTORER(m_curSymp);
        {
            ++m_modWithNum;
            m_curSymp = m_statep->insertBlock(m_curSymp, "__Vforeach" + cvtToStr(m_modWithNum),
                                              nodep, m_classOrPackagep);
            m_curSymp->fallbackp(VL_RESTORER_PREV(m_curSymp));
            为 (AstNode *nextp, *argp = nodep->headerp()->elementsp(); argp; argp = nextp) {
                nextp = argp->nextp();
                AstVar* argrefp = nullptr;
                如果 (AstParseRef* const parserefp = VN_CAST(argp, ParseRef)) {
                    // IEEE 1800-2023 12.7.3: foreach loop variable type shall be int (2-state)
                    // This might get changed in V3Width when types resolve (e.g., for assoc
                    // arrays)
                    argrefp = new AstVar{parserefp->fileline(), VVarType::BLOCKTEMP,
                                         parserefp->name(), argp->findIntDType()};
                    argrefp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                    parserefp->replaceWith(argrefp);
                    VL_DO_DANGLING2(parserefp->deleteTree(), parserefp, argp);
                    // Insert argref's name into symbol table
                    m_statep->insertSym(m_curSymp, argrefp->name(), argrefp, nullptr);
                } 否则 如果 (const auto largrefp = VN_CAST(argp, Var)) {
                    argrefp = largrefp;
                    // Insert argref's name into symbol table
                    m_statep->insertSym(m_curSymp, argrefp->name(), argrefp, nullptr);
                } 否则 如果 (VN_IS(argp, Empty)) {
                } 否则 {
                    argp->v3error("'foreach' loop variable expects simple variable name");
                }
            }
            iterateChildren(nodep);
        }
    }

    void visit(AstRandSequence* nodep) override {
        VL_RESTORER(m_curSymp);
        m_curSymp = m_statep->insertBlock(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
        iterateChildren(nodep);
    }
    void visit(AstRSProd* nodep) override {
        VL_RESTORER(m_curSymp);
        m_curSymp = m_statep->insertBlock(m_curSymp, nodep->name(), nodep, m_classOrPackagep);
        如果 (nodep->fvarp())
            nodep->fvarp()->v3warn(E_UNSUPPORTED,
                                   "Unsupported: randsequence production function variable");
        // Mark formal ports as port-checked so the primary resolve pass does not
        // flag them with "does not appear in port list" -- V3RandSequence will
        // later move them onto the generated task as real input ports.
        为 (AstNode* itp = nodep->portsp(); itp; itp = itp->nextp()) {
            VN_AS(itp, Var)->user4(true);
        }
        iterateChildren(nodep);
    }

    void visit(AstWithParse* nodep) override {  // FindVisitor::
        // Change WITHPARSE(FUNCREF, equation) to FUNCREF(WITH(equation))
        AstNodeFTaskRef* funcrefp = VN_CAST(nodep->funcrefp(), NodeFTaskRef);
        如果 (const AstDot* const dotp = VN_CAST(nodep->funcrefp(), Dot))
            funcrefp = VN_CAST(dotp->rhsp(), NodeFTaskRef);
        UASSERT_OBJ(funcrefp, nodep, "'with' only can operate on a function/task");
        string name = funcrefp->name() == "randomize" ? "__Vrandwith_obj" : "item";
        FileLine* argFl = nodep->fileline();
        AstArg* const argsp = funcrefp->argsp();
        如果 (argsp) {
            argsp->unlinkFrBackWithNext();
            如果 (funcrefp->name() != "randomize") {
                如果 (const auto parserefp = VN_CAST(argsp->exprp(), ParseRef)) {
                    name = parserefp->name();
                    argFl = parserefp->fileline();
                } 否则 {
                    argsp->v3error("'with' function expects simple variable name");
                }
                如果 (argsp->nextp()) {
                    argsp->nextp()->v3error("'with' function expects only up to one argument");
                }
                VL_DO_DANGLING(argsp->deleteTree(), argsp);
            }
        }
        // Type depends on the method used, let V3Width figure it out later
        // 'with (...) { ... }' constraint_block form is randomize-only
        // (IEEE 1800-2023 7.12 vs. 18.7); array methods take 'with (expr)'.
        如果 (nodep->restricted() && funcrefp->name() != "randomize") {
            nodep->v3error("'with (...) { ... }' constraint block is only valid for"
                           " randomize() (IEEE 1800-2023 18.7)");
            funcrefp->addArgsp(argsp);
            nodep->replaceWith(nodep->funcrefp()->unlinkFrBack());
            VL_DO_DANGLING(nodep->deleteTree(), nodep);
            返回;
        }
        const bool restrictedRandomize = nodep->restricted();
        如果 (nodep->exprsp() || nodep->constraintsp()
            || restrictedRandomize) {  // Else empty expression and pretend no "with"
            AstLambdaArgRef* const indexArgRefp
                = new AstLambdaArgRef{argFl, name + "__DOT__index", true};
            AstLambdaArgRef* const valueArgRefp = new AstLambdaArgRef{argFl, name, false};
            AstWith* const newp
                = new AstWith{nodep->fileline(), indexArgRefp, valueArgRefp, nullptr};
            // Harvest the identifier_list into AstWith; grammar guarantees AstParseRef.
            如果 (restrictedRandomize) {
                newp->restricted(true);
                当 (AstNode* const itemp = nodep->exprsp()) {
                    newp->addRestrictedName(VN_AS(itemp, ParseRef)->name());
                    itemp->unlinkFrBack();
                    VL_DO_DANGLING(pushDeletep(itemp), itemp);
                }
            }
            AstNode* exprOrConstraintsp = nullptr;
            如果 (nodep->exprsp()) exprOrConstraintsp = nodep->exprsp()->unlinkFrBackWithNext();
            如果 (nodep->constraintsp())
                exprOrConstraintsp = AstNode::addNext(
                    exprOrConstraintsp, nodep->constraintsp()->unlinkFrBackWithNext());
            newp->addExprp(exprOrConstraintsp);  // addExprp() tolerates nullptr
            funcrefp->withp(newp);
        }
        funcrefp->addArgsp(argsp);
        nodep->replaceWith(nodep->funcrefp()->unlinkFrBack());
        VL_DO_DANGLING(nodep->deleteTree(), nodep);
    }
    void visit(AstWith* nodep) override {  // FindVisitor::
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
    void visit(AstCoverWith* nodep) override {  // FindVisitor::
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
    void visit(AstCoverCrossWith* nodep) override {  // FindVisitor::
        // The selection, the 'matches' count, and the cross items' types, of their coverpoints'
        // expressions with any nested scopes, are outside the filter's scope, which declares the
        // items.  As visit(AstVar) would resolve a type in the scope declaring its variable, the
        // items, generated and unique, are inserted directly.
        iterateAndNextNull(nodep->selectp());
        iterateAndNextNull(nodep->countp());
        为 (AstVar* varp = nodep->varsp(); varp; varp = VN_AS(varp->nextp(), Var)) {
            iterateAndNextNull(varp->childDTypep());
        }
        VL_RESTORER(m_curSymp);
        ++m_modWithNum;
        m_curSymp = m_statep->insertBlock(m_curSymp, "__VcoverWith" + cvtToStr(m_modWithNum),
                                          nodep, m_classOrPackagep);
        m_curSymp->fallbackp(VL_RESTORER_PREV(m_curSymp));
        为 (AstVar* varp = nodep->varsp(); varp; varp = VN_AS(varp->nextp(), Var)) {
            m_statep->insertSym(m_curSymp, varp->name(), varp, m_classOrPackagep);
        }
        iterateAndNextNull(nodep->filterp());
    }

    void visit(AstNode* nodep) override { iterateChildren(nodep); }  // FindVisitor::

    void handleUnvisitedVirtIfaces() {
        AstNodeModule* const top = v3Global.rootp()->modulesp();
        m_curSymp = m_statep->getNodeSym(top);
        为 (AstIface* ifacep : m_virtIfaces) {
            如果 (!ifacep->user4()) {  // virtual interface never assigned any actual interface
                // Create dummy cell to keep the virtual interface alive
                // (later stages assume that non-dead interface has associated cell)
                AstCell* const ifacecellp = new AstCell{ifacep->fileline(),
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

public:
    // CONSTRUCTORS
    LinkDotFindVisitor(AstNetlist* rootp, LinkDotState* statep)
        : m_statep{statep} {
        UINFO(4, __FUNCTION__ << ": ");
        iterate(rootp);

        如果 (!m_virtIfaces.empty()) handleUnvisitedVirtIfaces();
        为 (AstScope* const scopep : m_scopes) scopep->modp()->addStmtsp(scopep);
        如果 (m_statep->forScopeCreation()) {
            V3Stats::addStat("LinkDot, Scope visits", m_statScopeVisits);
        }
    }
    ~LinkDotFindVisitor() override = default;
};

//======================================================================

class LinkDotFindIfaceVisitor final : public VNVisitor {
    // NODE STATE
    //  *::user1p()             -> See LinkDotState

    // STATE - for current visit position (use VL_RESTORER)
    LinkDotState* const m_statep;  // State to pass between visitors, including symbol table
    AstNode* m_declp = nullptr;  // Current declaring object that may soon contain IfaceRefDType

    // METHODS
    const VSymEnt* findNonDeclSym(VSymEnt* symp, const string& name) {
        // Find if there is a symbol of given name, ignoring the node that is declaring
        // it (m_declp) itself.  Thus if searching for "ifc" (an interface):
        //
        // module x;  // Finishes here and finds the typedef
        //   typedef foo ifc;
        //   ifc ifc;   // symp starts pointing here, but matches m_declp
        当 (symp) {
            const VSymEnt* const foundp = symp->findIdFlat(name);
            如果 (foundp && foundp->nodep() != m_declp) 返回 foundp;
            symp = symp->fallbackp();
        }
        返回 nullptr;
    }

    // VISITORS
    void visit(AstRefDType* nodep) override {  // FindIfaceVisitor::
        如果 (m_statep->forPrimary() && !nodep->classOrPackagep()) {
            UINFO(9, "  FindIfc: " << nodep);
            // If under a var, ignore the var itself as might be e.g. "intf intf;"
            // Critical tests:
            //   t_interface_param_genblk.v  // Checks this does make interface
            //   t_interface_hidden.v  // Checks this doesn't making interface when hidden
            如果 (m_statep->existsNodeSym(nodep)) {
                VSymEnt* symp = m_statep->getNodeSym(nodep);
                const VSymEnt* foundp = findNonDeclSym(symp, nodep->name());
                AstNode* foundNodep = nullptr;
                // This: v4make test_regress/t/t_interface_param_genblk.py --debug
                // --debugi-V3LinkDot 9 Passes with this commented out:
                如果 (foundp) foundNodep = foundp->nodep();
                如果 (!foundNodep) foundNodep = m_statep->findModuleSym(nodep->name());
                如果 (foundNodep) UINFO(9, "    Ifc foundNodep " << foundNodep);
                如果 (AstIface* const defp = VN_CAST(foundNodep, Iface)) {
                    // Must be found as module name, and not hidden/ by normal symbol (foundp)
                    AstIfaceRefDType* const newp
                        = new AstIfaceRefDType{nodep->fileline(), "", nodep->name()};
                    如果 (nodep->paramsp())
                        newp->addParamsp(nodep->paramsp()->unlinkFrBackWithNext());
                    newp->ifacep(defp);
                    newp->user1u(nodep->user1u());
                    UINFO(9, "    Resolved interface " << nodep << "  =>  " << defp);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(pushDeletep(nodep), nodep);
                    返回;
                }
            }
        }
        iterateChildren(nodep);
    }
    void visit(AstVar* nodep) override {  // FindIfaceVisitor::
        VL_RESTORER(m_declp);
        m_declp = nodep;
        iterateChildren(nodep);
        AstIfaceRefDType* const ifacerefp = LinkDotState::ifaceRefFromArray(nodep->subDTypep());
        如果 (ifacerefp && m_statep->existsNodeSym(nodep)) {
            // Can't resolve until interfaces and modport names are
            // known; see notes at top
            UINFO(9, "  FindIfc Var IfaceRef " << ifacerefp);
            如果 (!ifacerefp->isVirtual()) nodep->setIfaceRef();
            VSymEnt* const symp = m_statep->getNodeSym(nodep);
            m_statep->insertIfaceVarSym(symp);
        }
    }
    void visit(AstNode* nodep) override { iterateChildren(nodep); }  // FindIfaceVisitor::

public:
    // CONSTRUCTORS
    LinkDotFindIfaceVisitor(AstNetlist* rootp, LinkDotState* statep)
        : m_statep{statep} {
        UINFO(4, __FUNCTION__ << ": ");
        iterate(rootp);
    }
    ~LinkDotFindIfaceVisitor() override = default;
};

//======================================================================

class LinkDotParamVisitor final : public VNVisitor {
    // NODE STATE
    //  *::user1p()             -> See LinkDotState
    //  *::user2p()             -> See LinkDotState
    //  *::user4()              -> See LinkDotState

    // STATE
    LinkDotState* const m_statep;  // State to pass between visitors, including symbol table
    AstNodeModule* m_modp = nullptr;  // Current module

    void pinImplicitExprRecurse(AstNode* nodep) {
        // Under a pin, Check interconnect expression for a pin reference or a concat.
        // Create implicit variable as needed
        如果 (VN_IS(nodep, Dot)) {  // Not creating a simple implied type,
            // and implying something else would just confuse later errors
        } 否则 如果 (VN_IS(nodep, VarRef) || VN_IS(nodep, ParseRef)) {
            // To prevent user errors, we should only do single bit
            // implicit vars, however some netlists (MIPS) expect single
            // bit implicit wires to get created with range 0:0 etc.
            m_statep->implicitOkAdd(m_modp, nodep->name());
        }
        // These are perhaps a little too generous, as a SELect of siga[sigb]
        // perhaps shouldn't create an implicit variable.  But, we've warned...
        否则 {
            如果 (AstNode* const refp = nodep->op1p()) pinImplicitExprRecurse(refp);
            如果 (AstNode* const refp = nodep->op2p()) pinImplicitExprRecurse(refp);
            如果 (AstNode* const refp = nodep->op3p()) pinImplicitExprRecurse(refp);
            如果 (AstNode* const refp = nodep->op4p()) pinImplicitExprRecurse(refp);
        }
        // Continue along a list (e.g. the terminals under AstImplicit), also after a reference
        如果 (AstNode* const refp = nodep->nextp()) pinImplicitExprRecurse(refp);
    }

    // VISITORS
    void visit(AstTypeTable*) override {}  // ParamVisitor::
    void visit(AstNodeModule* nodep) override {  // ParamVisitor::
        UINFO(5, "   " << nodep);
        如果 ((nodep->dead() || !nodep->user4()) && !nodep->hierParams()) {
            UINFO(4, "Mark dead module " << nodep);
            UASSERT_OBJ(!m_statep->forScopeCreation(), nodep,
                        "Dead module persisted past where should have removed");
            // Don't remove now, because we may have a tree of
            // parameterized modules with VARXREFs into the deleted module
            // region.  V3Dead should cleanup.
            // Downstream visitors up until V3Dead need to check for nodep->dead.
            nodep->dead(true);
            返回;
        }
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
    }
    void visit(AstPin* nodep) override {  // ParamVisitor::
        // Pin: Link to submodule's port
        // Deal with implicit definitions - do before Resolve visitor as may
        // be referenced above declaration
        如果 (nodep->exprp()  // Else specifically unconnected pin
            && !nodep->svImplicit()) {  // SV 19.11.3: .name pins don't allow implicit decls
            pinImplicitExprRecurse(nodep->exprp());
        }
    }
    void visit(AstDefParam* nodep) override {  // ParamVisitor::
        iterateChildren(nodep);
        nodep->v3warn(DEFPARAM, "defparam is deprecated (IEEE 1800-2023 C.4.1)\n"
                                    << nodep->warnMore()
                                    << "... Suggest use instantiation with #(."
                                    << nodep->prettyName() << "(...etc...))");
        bool hasPartSelect = false;
        const string path = extractDottedPath(nodep->pathp(), hasPartSelect);
        UASSERT_OBJ(!hasPartSelect && !path.empty(), nodep, "Unexpected defparam path shape");
        string baddot;
        VSymEnt* okSymp = nullptr;
        VSymEnt* const foundp = m_statep->findDotted(
            nodep->fileline(), m_statep->getNodeSym(nodep), path, baddot, okSymp, true);
        AstCell* const cellp = foundp ? VN_AS(foundp->nodep(), Cell) : nullptr;
        如果 (!cellp) {
            nodep->v3error("In defparam, instance " << path << " never declared");
        } 否则 {
            AstNodeExpr* const exprp = nodep->rhsp()->unlinkFrBack();
            UINFO(9, "Defparam cell " << path << "." << nodep->name() << " attach-to " << cellp
                                      << "  <= " << exprp);
            // Don't need to check the name of the defparam exists.  V3Param does.
            AstPin* const pinp = new AstPin{nodep->fileline(),
                                            -1,  // Pin# not relevant
                                            nodep->name(), exprp};
            pinp->param(true);
            pinp->paramPath(path);
            cellp->addParamsp(pinp);
        }
        VL_DO_DANGLING(nodep->unlinkFrBack()->deleteTree(), nodep);
    }
    void visit(AstPort* nodep) override {  // ParamVisitor::
        // Port: Stash the pin number
        // Need to set pin numbers after varnames are created
        // But before we do the final resolution based on names
        VSymEnt* const foundp = m_statep->getNodeSym(m_modp)->findIdFlat(nodep->name());
        AstVar* const refp = foundp ? VN_CAST(foundp->nodep(), Var) : nullptr;
        如果 (!foundp) {
            nodep->v3error(
                "Input/output/inout declaration not found for port: " << nodep->prettyNameQ());
        } 否则 如果 (!refp || (!refp->isIO() && !refp->isIfaceRef())) {
            nodep->v3error("Pin is not an in/out/inout/interface: " << nodep->prettyNameQ());
        } 否则 {
            如果 (refp->user4()) {
                nodep->v3error("Duplicate declaration of port: "
                               << nodep->prettyNameQ() << '\n'
                               << nodep->warnContextPrimary() << '\n'
                               << refp->warnOther() << "... Location of original declaration\n"
                               << refp->warnContextSecondary());
            }
            refp->user4(true);
            VSymEnt* const symp = m_statep->insertSym(m_statep->getNodeSym(m_modp),
                                                      "__pinNumber" + cvtToStr(nodep->pinNum()),
                                                      refp, nullptr /*classOrPackagep*/);
            symp->exported(false);
            refp->pinNum(nodep->pinNum());
            // Put the variable where the port is, so that variables stay
            // in pin number sorted order. Otherwise hierarchical or JSON
            // may botch by-position instances.
            nodep->addHereThisAsNext(refp->unlinkFrBack());
        }
        // Ports not needed any more
        VL_DO_DANGLING(nodep->unlinkFrBack()->deleteTree(), nodep);
    }
    void visit(AstAssignW* nodep) override {  // ParamVisitor::
        // Deal with implicit definitions
        // We used to nodep->allowImplicit() here, but it turns out
        // normal "assigns" can also make implicit wires.  Yuk.
        pinImplicitExprRecurse(nodep->lhsp());
        iterateChildren(nodep);
    }

    void visit(AstImplicit* nodep) override {  // ParamVisitor::
        // Unsupported gates need implicit creation
        pinImplicitExprRecurse(nodep->exprsp());
        // We're done with implicit gates
        VL_DO_DANGLING(nodep->unlinkFrBack()->deleteTree(), nodep);
    }
    void visit(AstClassOrPackageRef* nodep) override {  // ParamVisitor::
        如果 (auto* const fwdp = VN_CAST(nodep->classOrPackageNodep(), TypedefFwd)) {
            // Relink forward definitions to the "real" definition
            VSymEnt* const foundp = m_statep->getNodeSym(fwdp)->findIdFallback(fwdp->name());
            如果 (foundp && (VN_IS(foundp->nodep(), Class) || VN_IS(foundp->nodep(), Package))) {
                nodep->classOrPackagep(VN_AS(foundp->nodep(), NodeModule));
            } 否则 如果 (foundp && VN_IS(foundp->nodep(), ParamTypeDType)) {
                UASSERT(m_statep->forPrimary(), "Param types should have been resolved");
                nodep->classOrPackageNodep(VN_AS(foundp->nodep(), ParamTypeDType));
            } 否则 {
                如果 (foundp) UINFO(1, "found nodep = " << foundp->nodep());
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
    void visit(AstPull* nodep) override {  // ParamVisitor::
        // Deal with implicit definitions
        // We used to nodep->allowImplicit() here, but it turns out
        // normal "assigns" can also make implicit wires.  Yuk.
        pinImplicitExprRecurse(nodep->lhsp());
        iterateChildren(nodep);
    }
    void visit(AstTypedefFwd* nodep) override {  // ParamVisitor::
        VSymEnt* const foundp = m_statep->getNodeSym(nodep)->findIdFallback(nodep->name());
        如果 (foundp) {
            // If the typedef was earlier in source order (tokenNum), then remember that earlier
            // point to avoid error something wasn't declared
            // Might be forward declaring something odd (with declTokenNumSetMin not implemented,
            // but should be harmless to ignore as this is just for error detection
            foundp->nodep()->declTokenNumSetMin(nodep->fileline()->tokenNum());
        } 否则 如果 (v3Global.opt.pedantic()) {
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

    void visit(AstNode* nodep) override { iterateChildren(nodep); }  // ParamVisitor::

public:
    // CONSTRUCTORS
    LinkDotParamVisitor(AstNetlist* rootp, LinkDotState* statep)
        : m_statep{statep} {
        UINFO(4, __FUNCTION__ << ": ");
        iterate(rootp);
    }
    ~LinkDotParamVisitor() override = default;
};

//======================================================================

class LinkDotScopeVisitor final : public VNVisitor {

    // STATE
    LinkDotState* const m_statep;  // State to pass between visitors, including symbol table
    const AstScope* m_scopep = nullptr;  // The current scope
    VSymEnt* m_modSymp = nullptr;  // Symbol entry for current module
    // Deferred AliasScope processing - must be done outer-to-inner for correct alias resolution
    std::vector<std::pair<AstAliasScope*, VSymEnt*>> m_deferredAliasScopes;

    // METHODS
public:
    // getAlias with setVarAlias and setIfaceAlias implement disjoint-set data structure.
    // This algorithm is needed for the case when multiple alias statements
    // reference partially the same variables.
    static AstVarScope* getAlias(AstVarScope* const vscp) {
        如果 (vscp->user2p() && vscp != vscp->user2p()) {
            AstVarScope* const aliasp = getAlias(VN_AS(vscp->user2p(), VarScope));
            vscp->user2p(aliasp);
            返回 aliasp;
        } 否则 {
            返回 vscp;
        }
    }

private:
    // Alias an interface reference to the interface it is connected to
    void setIfaceAlias(AstVarScope* const vscp, AstVarScope* const aliasp) {
        UASSERT_OBJ(vscp->varp()->isIfaceRef(), vscp, "Interface alias of non-interface");
        UASSERT_OBJ(aliasp->varp()->isIfaceRef(), aliasp, "Interface alias of non-interface");
        getAlias(vscp)->user2p(getAlias(aliasp));
    }

    // Alias two variables. An output port in the lower scope survives, unless the other
    // must keep its own storage.
    void setVarAlias(AstVarScope* ap, AstVarScope* bp) {
        UASSERT_OBJ(!ap->varp()->isIfaceRef(), ap, "Variable alias of interface");
        UASSERT_OBJ(!bp->varp()->isIfaceRef(), bp, "Variable alias of interface");
        ap = getAlias(ap);
        bp = getAlias(bp);
        如果 (ap == bp) 返回;
        const bool aWins = [&]() {
            const AstVar* const aVarp = ap->varp();
            const AstVar* const bVarp = bp->varp();
            // A primary IO is accessed directly by the user of the model, so must survive
            UASSERT_OBJ(!aVarp->isPrimaryIO() || !bVarp->isPrimaryIO(), ap,
                        "Alias of two primary IOs");
            如果 (bVarp->isPrimaryIO()) 返回 false;
            如果 (aVarp->isPrimaryIO()) 返回 true;
            // A forced or public variable must keep its own storage. If both are, either works.
            如果 (aVarp->isForced() != bVarp->isForced()) 返回 aVarp->isForced();
            如果 (aVarp->isSigPublic() != bVarp->isSigPublic()) 返回 aVarp->isSigPublic();
            // Otherwise prefer an output port in the lower scope. This enables better V3Combine
            // due to having fewer upward hierarchical references (which prevent combining).
            如果 (aVarp->direction() != VDirection::OUTPUT) 返回 false;
            // Is 'ap' strictly below 'bp' in the hierarchy
            为 (const AstScope* scopep = ap->scopep()->aboveScopep(); scopep;
                 scopep = scopep->aboveScopep()) {
                如果 (scopep == bp->scopep()) 返回 true;
            }
            返回 false;
        }();
        如果 (aWins) {
            bp->user2p(ap);
        } 否则 {
            ap->user2p(bp);
        }
    }

    // VISITORS
    void visit(AstNetlist* nodep) override {  // ScopeVisitor::
        // Recurse..., backward as must do packages before using packages
        iterateChildrenBackwardsConst(nodep);
    }
    void visit(AstScope* nodep) override {  // ScopeVisitor::
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
        如果 (AstIface* const ifacep = VN_CAST(nodep->modp(), Iface)) {
            m_statep->insertIfaceModSym(ifacep, m_modSymp);
        }
        m_scopep = nodep;
        iterateChildren(nodep);
    }
    void visit(AstVarScope* nodep) override {  // ScopeVisitor::
        如果 (!nodep->varp()->isFuncLocal() && !nodep->varp()->isClassMember()) {
            VSymEnt* const varSymp
                = m_statep->insertSym(m_modSymp, nodep->varp()->name(), nodep, nullptr);
            如果 (nodep->varp()->isIfaceRef()) {
                AstIfaceRefDType* const dtypep
                    = LinkDotState::ifaceRefFromArray(nodep->varp()->dtypep());
                UASSERT_OBJ(dtypep, nodep, "Non AstIfaceRefDType on isIfaceRef() var");
                如果 (nodep->varp()->isIfaceParent()) {
                    UINFO(9, "Iface parent ref var " << nodep->varp()->name() << " " << nodep);
                    // Find the interface cell the var references
                    UINFO(9, "Iface parent dtype " << dtypep);
                    const string ifcellname = dtypep->cellName();
                    string baddot;
                    VSymEnt* okSymp;
                    VSymEnt* cellSymp = m_statep->findDotted(nodep->fileline(), m_modSymp,
                                                             ifcellname, baddot, okSymp, false);
                    UASSERT_OBJ(
                        cellSymp, nodep,
                        "No symbol for interface instance: " << nodep->prettyNameQ(ifcellname));
                    UINFO(5, "       Found interface instance: se" << cvtToHex(cellSymp) << " "
                                                                   << cellSymp->nodep());
                    如果 (dtypep->modportName() != "") {
                        // Look up the modport within the interface cell's symbol table
                        VSymEnt* const mpSymp = cellSymp->findIdFallback(dtypep->modportName());
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
    void visit(AstNodeFTask* nodep) override {  // ScopeVisitor::
        VSymEnt* const symp = m_statep->insertBlock(m_modSymp, nodep->name(), nodep, nullptr);
        symp->fallbackp(m_modSymp);
        iterateChildren(nodep);
    }
    void visit(AstNodeForeach* nodep) override {  // ScopeVisitor::
        VSymEnt* const symp = m_statep->insertBlock(m_modSymp, nodep->name(), nodep, nullptr);
        symp->fallbackp(m_modSymp);
        // No recursion, we don't want to pick up variables
    }
    void visit(AstConstraintForeach* nodep) override {  // ScopeVisitor::
        iterateChildren(nodep);
    }
    void visit(AstWith* nodep) override {  // ScopeVisitor::
        VSymEnt* const symp = m_statep->insertBlock(m_modSymp, nodep->name(), nodep, nullptr);
        symp->fallbackp(m_modSymp);
        iterateChildren(nodep);
    }
    void visit(AstAlias* nodep) override {  // ScopeVisitor::
        // Track aliases
        UINFOTREE(9, nodep, "", "alias");
        AstVarScope* aliasVscp = nullptr;
        为 (AstNode* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
            AstVarScope* vscp = nullptr;
            如果 (const AstVarRef* const refp = VN_CAST(itemp, VarRef)) {
                vscp = refp->varScopep();
            } 否则 {
                // Reaches into the scope of an instance, look it up by name
                const AstVarXRef* const xrefp = VN_AS(itemp, VarXRef);
                const string scopename = xrefp->dotted() + "." + xrefp->name();
                string baddot;
                VSymEnt* okSymp;
                VSymEnt* const symp = m_statep->findDotted(xrefp->fileline(), m_modSymp, scopename,
                                                           baddot, okSymp, false);
                UASSERT_OBJ(symp, nodep, "No symbol for alias item: " << scopename);
                vscp = VN_CAST(symp->nodep(), VarScope);
            }
            UASSERT_OBJ(vscp, nodep, "VarScope unset");
            如果 (aliasVscp) {
                setVarAlias(aliasVscp, vscp);
            } 否则 {
                aliasVscp = vscp;
            }
        }
        iterateChildren(nodep);
        VL_DO_DANGLING(pushDeletep(nodep->unlinkFrBack()), nodep);
    }
    void visit(AstAliasScope* nodep) override {  // ScopeVisitor::
        // Defer AliasScope processing - must process outer scopes before inner ones
        // so that nested interface port alias resolution works correctly
        UINFO(5, "ALIASSCOPE (deferred) " << nodep);
        m_deferredAliasScopes.emplace_back(nodep, m_modSymp);
    }
    void processAliasScope(AstAliasScope* nodep, VSymEnt* modSymp) {
        UINFO(5, "ALIASSCOPE  " << nodep);
        UINFOTREE(9, nodep, "", "avs");
        VSymEnt* rhsSymp;
        {
            AstVarRef* const refp = VN_CAST(nodep->rhsp(), VarRef);
            AstVarXRef* const xrefp = VN_CAST(nodep->rhsp(), VarXRef);
            UASSERT_OBJ(refp || xrefp, nodep,
                        "Unsupported: Non Var(X)Ref attached to interface pin");
            string inl
                = ((xrefp && xrefp->inlinedDots().size()) ? (xrefp->inlinedDots() + "__DOT__")
                                                          : "");
            const string dottedPath
                = (xrefp && !xrefp->dotted().empty()) ? (xrefp->dotted() + ".") : "";
            VSymEnt* symp = nullptr;
            string scopename;
            当 (!symp) {
                scopename = refp ? refp->name()
                                 : (inl.size() ? (inl + dottedPath + xrefp->name())
                                               : (dottedPath + xrefp->name()));
                string baddot;
                VSymEnt* okSymp;
                symp = m_statep->findDotted(nodep->rhsp()->fileline(), modSymp, scopename, baddot,
                                            okSymp, true);
                如果 (inl == "") break;
                inl = LinkDotState::removeLastInlineScope(inl);
            }
            UASSERT_OBJ(symp, nodep, "No symbol for interface alias rhs");
            UINFO(5, "       Found a linked scope RHS: " << scopename << "  se" << cvtToHex(symp)
                                                         << " " << symp->nodep());
            rhsSymp = symp;
        }
        VSymEnt* lhsSymp;
        {
            const AstVarXRef* const xrefp = VN_CAST(nodep->lhsp(), VarXRef);
            const AstVarRef* const refp = VN_CAST(nodep->lhsp(), VarRef);

            UASSERT_OBJ(refp || xrefp, nodep,
                        "Unsupported: Non Var(X)Ref attached to interface pin");
            const string scopename
                = refp ? refp->varp()->name() : xrefp->dotted() + "." + xrefp->name();
            string baddot;
            VSymEnt* okSymp;
            VSymEnt* const symp = m_statep->findDotted(nodep->lhsp()->fileline(), modSymp,
                                                       scopename, baddot, okSymp, false);
            UASSERT_OBJ(symp, nodep, "No symbol for interface alias lhs");
            UINFO(5, "       Found a linked scope LHS: " << scopename << "  se" << cvtToHex(symp)
                                                         << " " << symp->nodep());
            lhsSymp = symp;
        }
        // Remember the alias - can't do it yet because we may have additional symbols to be added,
        // or maybe an alias of an alias
        m_statep->insertScopeAlias(LinkDotState::SAMN_IFTOP, lhsSymp, rhsSymp);
        AstVarScope* const lhsVscp = VN_CAST(lhsSymp->nodep(), VarScope);
        AstVarScope* const rhsVscp = VN_CAST(rhsSymp->nodep(), VarScope);
        UASSERT_OBJ(lhsVscp && rhsVscp, nodep, "Interface alias missing variable scope");
        setIfaceAlias(lhsVscp, rhsVscp);
        // We have stored the link, we don't need these any more
        VL_DO_DANGLING(nodep->unlinkFrBack()->deleteTree(), nodep);
    }
    void processDeferredAliasScopes() {
        // Sort by hierarchy depth (shallower first) so outer aliases are resolved before inner
        // Pre-compute depth map to avoid O(N log N * D) complexity in sort comparisons
        std::unordered_map<VSymEnt*, 整数> depthMap;
        为 (const auto& pair : m_deferredAliasScopes) {
            VSymEnt* const symp = pair.second;
            如果 (depthMap.find(symp) == depthMap.end()) {
                整数 depth = 0;
                为 (VSymEnt* p = symp; p; p = p->parentp()) ++depth;
                depthMap[symp] = depth;
            }
        }
        std::stable_sort(m_deferredAliasScopes.begin(), m_deferredAliasScopes.end(),
                         [&depthMap](const std::pair<AstAliasScope*, VSymEnt*>& a,
                                     const std::pair<AstAliasScope*, VSymEnt*>& b) {
                             返回 depthMap.at(a.second) < depthMap.at(b.second);
                         });
        // Process in sorted order
        为 (auto& pair : m_deferredAliasScopes) { processAliasScope(pair.first, pair.second); }
        m_deferredAliasScopes.clear();
    }
    void visit(AstNodeGen* nodep) override {  // ScopeVisitor::  // LCOV_EXCL_LINE
        nodep->v3fatalSrc("Generate constructs should have been reduced out");
    }
    // For speed, don't recurse things that can't have scope
    // Note we allow AstNodeStmt's as generates may be under them
    void visit(AstCell*) override {}  // ScopeVisitor::
    void visit(AstVar*) override {}  // ScopeVisitor::
    void visit(AstNode* nodep) override { iterateChildren(nodep); }  // ScopeVisitor::

public:
    // CONSTRUCTORS
    LinkDotScopeVisitor(AstNetlist* rootp, LinkDotState* statep)
        : m_statep{statep} {
        UINFO(4, __FUNCTION__ << ": ");
        iterate(rootp);
        // Process deferred AliasScopes in outer-to-inner order
        processDeferredAliasScopes();
    }
    ~LinkDotScopeVisitor() override = default;
};

//======================================================================

// Iterate an interface to resolve modports
class LinkDotIfaceVisitor final : public VNVisitor {
    // STATE
    LinkDotState* const m_statep;  // State to pass between visitors, including symbol table
    VSymEnt* m_curSymp;  // Symbol Entry for current table, where to lookup/insert

    // VISITORS
    void visit(AstModport* nodep) override {  // IfaceVisitor::
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
    void visit(AstModportFTaskRef* nodep) override {  // IfaceVisitor::
        UINFO(5, "   fif: " << nodep);
        iterateChildren(nodep);
        VSymEnt* const symp = m_curSymp->findIdFallback(nodep->name());
        如果 (!symp) {
            nodep->v3error("Modport item not found: " << nodep->prettyNameQ());
        } 否则 如果 (AstNodeFTask* const ftaskp = VN_CAST(symp->nodep(), NodeFTask)) {
            // Make symbol under modport that points at the _interface_'s var, not the modport.
            nodep->ftaskp(ftaskp);
            VSymEnt* const subSymp
                = m_statep->insertSym(m_curSymp, nodep->name(), ftaskp, nullptr /*package*/);
            m_statep->insertScopeAlias(LinkDotState::SAMN_MODPORT, subSymp, symp);
        } 否则 {
            nodep->v3error("Modport item is not a function/task: " << nodep->prettyNameQ());
        }
        如果 (m_statep->forScopeCreation()) {
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
        如果 (!okSymp || baddot.empty()) 返回 nullptr;

        static constexpr 整数 MAX_NESTING_DEPTH = 64;
        VSymEnt* curOkSymp = okSymp;

        为 (整数 depth = 0; depth < MAX_NESTING_DEPTH; ++depth) {
            // Try to get interface from the partially-matched symbol
            AstIface* ifacep = nullptr;
            如果 (const AstCell* const cellp = VN_CAST(curOkSymp->nodep(), Cell)) {
                ifacep = VN_CAST(cellp->modp(), Iface);
            } 否则 如果 (const AstVar* const varp = VN_CAST(curOkSymp->nodep(), Var)) {
                如果 (varp->isIfaceRef()) {
                    如果 (const AstIfaceRefDType* const ifaceRefp
                        = LinkDotState::ifaceRefFromArray(varp->dtypep())) {
                        ifacep = ifaceRefp->ifaceViaCellp();
                    }
                }
            }

            如果 (!ifacep || !m_statep->existsNodeSym(ifacep)) 返回 nullptr;

            VSymEnt* const ifaceSymp = m_statep->getNodeSym(ifacep);

            如果 (baddot.find('.') == string::npos) {
                // Simple identifier - direct lookup
                VSymEnt* const symp = ifaceSymp->findIdFallback(baddot);
                如果 (symp) baddot.clear();
                返回 symp;
            }

            // Multi-level path - use findDotted for partial resolution
            string remainingBaddot;
            VSymEnt* remainingOkSymp = nullptr;
            VSymEnt* const symp = m_statep->findDotted(fl, ifaceSymp, baddot, remainingBaddot,
                                                       remainingOkSymp, true);
            如果 (symp) {
                baddot = remainingBaddot;
                返回 symp;
            }

            // findDotted partially matched - check progress
            如果 (remainingBaddot == baddot || !remainingOkSymp) 返回 nullptr;

            // Continue resolving with updated state
            baddot = remainingBaddot;
            curOkSymp = remainingOkSymp;
        }

        UINFO(1, "Nested interface resolution depth limit exceeded at " << fl);
        返回 nullptr;
    }

    // Resolve a modport expression to find the referenced symbol
    VSymEnt* resolveModportExpression(FileLine* fl, AstNodeExpr* exprp) {
        UINFO(5, "     resolveModportExpression: " << exprp);
        VSymEnt* symp = nullptr;
        如果 (AstParseRef* const refp = VN_CAST(exprp, ParseRef)) {
            // Simple variable reference: modport mp(input .a(sig_a))
            symp = m_curSymp->findIdFallback(refp->name());
        } 否则 如果 (AstVarRef* const refp = VN_CAST(exprp, VarRef)) {
            // Already resolved VarRef (can happen if iterateChildren resolved it)
            如果 (refp->varScopep()) {
                // During scope creation, VarRef may have VarScope already linked
                symp = m_statep->getNodeSym(refp->varScopep());
            } 否则 如果 (refp->varp()) {
                symp = m_curSymp->findIdFallback(refp->varp()->name());
            }
        } 否则 如果 (AstVarXRef* const refp = VN_CAST(exprp, VarXRef)) {
            // Resolved VarXRef (dotted reference resolved by earlier pass)
            如果 (refp->varp()) {
                // For nested interfaces, the var is in a different scope
                // First try to find the symbol via the var itself
                如果 (m_statep->existsNodeSym(refp->varp())) {
                    symp = m_statep->getNodeSym(refp->varp());
                } 否则 {
                    // Fallback: look up in current scope
                    symp = m_curSymp->findIdFallback(refp->varp()->name());
                }
            } 否则 如果 (!refp->dotted().empty()) {
                // varp not set yet - use dotted path to find the symbol
                // The dotted part (e.g., "base") is the interface cell, and name is the member
                const string fullPath = refp->dotted() + "." + refp->name();
                string baddot;
                VSymEnt* okSymp = nullptr;
                symp = m_statep->findDotted(fl, m_curSymp, fullPath, baddot, okSymp, true);
                // Handle nested interface path when findDotted had partial match
                如果 (okSymp && !baddot.empty()) {
                    VSymEnt* const resolved = resolveNestedInterfacePath(fl, okSymp, baddot);
                    如果 (resolved) symp = resolved;
                }
            }
        } 否则 如果 (AstDot* const dotp = VN_CAST(exprp, Dot)) {
            // Dotted path: modport mp(input .a(inner.sig))
            bool hasPartSelect = false;
            const string dottedPath = extractDottedPath(dotp, hasPartSelect);
            如果 (hasPartSelect) {
                fl->v3warn(
                    E_UNSUPPORTED,
                    "Unsupported: Modport expression with part select (IEEE 1800-2023 25.5.4)");
            } 否则 {
                string baddot;
                VSymEnt* okSymp = nullptr;
                symp = m_statep->findDotted(fl, m_curSymp, dottedPath, baddot, okSymp, true);

                // Handle nested interface path when findDotted had partial match
                如果 (okSymp && !baddot.empty()) {
                    VSymEnt* const resolved = resolveNestedInterfacePath(fl, okSymp, baddot);
                    如果 (resolved) symp = resolved;
                }

                如果 (!symp || !baddot.empty()) {
                    fl->v3error("Can't find modport expression target: "
                                << AstNode::prettyNameQ(dottedPath));
                    symp = nullptr;
                }
            }
        } 否则 如果 (VN_IS(exprp, SelBit) || VN_IS(exprp, SelExtract)) {
            // Part select expressions not yet supported
            fl->v3warn(E_UNSUPPORTED,
                       "Unsupported: Modport expression with part select (IEEE 1800-2023 25.5.4)");
        } 否则 {
            // Other expression types not supported
            fl->v3warn(E_UNSUPPORTED,
                       "Unsupported: Complex modport expression (IEEE 1800-2023 25.5.4)");
        }
        返回 symp;
    }

    void visit(AstModportVarRef* nodep) override {  // IfaceVisitor::
        UINFO(5, "   fiv: " << nodep);
        iterateChildren(nodep);
        VSymEnt* symp = nullptr;
        如果 (nodep->exprp()) {
            // Modport expression syntax: modport mp(input .port_name(expression))
            symp = resolveModportExpression(nodep->fileline(), nodep->exprp());
        } 否则 {
            symp = m_curSymp->findIdFallback(nodep->name());
        }
        如果 (!symp) {
            nodep->v3error("Modport item not found: " << nodep->prettyNameQ());
        } 否则 如果 (AstVar* const varp = VN_CAST(symp->nodep(), Var)) {
            // Make symbol under modport that points at the _interface_'s var via the modport.
            // (Need modport still to test input/output markings)
            nodep->varp(varp);
            如果 (nodep->exprp()) {
                // For modport expressions, insert symbol pointing to the underlying var (not the
                // ModportVarRef which will be deleted during scope creation). The virtual port
                // name maps to the real signal's var.
                VSymEnt* const subSymp
                    = m_statep->insertSym(m_curSymp, nodep->name(), varp, nullptr /*package*/);
                m_statep->insertScopeAlias(LinkDotState::SAMN_MODPORT, subSymp, symp);
            } 否则 {
                // For regular modport items, insert symbol pointing to ModportVarRef for
                // input/output marking tests.
                m_statep->insertSym(m_curSymp, nodep->name(), nodep, nullptr /*package*/);
            }
        } 否则 如果 (AstVarScope* const vscp = VN_CAST(symp->nodep(), VarScope)) {
            // Make symbol under modport that points at the _interface_'s var, not the modport.
            nodep->varp(vscp->varp());
            // Only insert symbol for modport expression virtual ports (exprp set).
            // For regular modport items, don't insert into the shared modport table because
            // each instance would overwrite the previous one, causing wrong VarScope lookups.
            // Regular items are found via the modport's fallback to the interface.
            如果 (nodep->exprp()) {
                VSymEnt* const subSymp
                    = m_statep->insertSym(m_curSymp, nodep->name(), vscp, nullptr /*package*/);
                m_statep->insertScopeAlias(LinkDotState::SAMN_MODPORT, subSymp, symp);
            }
        } 否则 {
            nodep->v3error("Modport item is not a variable: " << nodep->prettyNameQ());
        }
        如果 (m_statep->forScopeCreation()) {
            // Done with AstModportVarRef.
            // Delete to prevent problems if we dead-delete pointed to variable
            nodep->unlinkFrBack();
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
        }
    }
    void visit(AstModportClockingRef* nodep) override {  // IfaceVisitor::
        UINFO(5, "   fic: " << nodep);
        iterateChildren(nodep);
        VSymEnt* const symp = m_curSymp->findIdFallback(nodep->name());
        如果 (!symp) {
            nodep->v3error("Modport item not found: " << nodep->prettyNameQ());
        } 否则 如果 (AstClocking* const clockingp = VN_CAST(symp->nodep(), Clocking)) {
            nodep->clockingp(clockingp);
            m_statep->insertSym(m_curSymp, nodep->name(), nodep, nullptr /*package*/);
        } 否则 {
            nodep->v3error("Modport item is not a clocking block: " << nodep->prettyNameQ());
        }
    }
    void visit(AstScope* nodep) override {  // IfaceVisitor::
        如果 (nodep->user3SetOnce()) 返回;
        iterateChildren(nodep);
    }
    void visit(AstNode* nodep) override { iterateChildren(nodep); }  // IfaceVisitor::

public:
    // CONSTRUCTORS
    LinkDotIfaceVisitor(AstIface* nodep, VSymEnt* curSymp, LinkDotState* statep)
        : m_statep{statep}
        , m_curSymp{curSymp} {
        UINFO(4, __FUNCTION__ << ": ");
        iterate(nodep);
    }
    ~LinkDotIfaceVisitor() override = default;
};

void LinkDotState::computeIfaceModSyms() {
    //  AstScope::user3()  // bool. Already iterated by a LinkDotIfaceVisitor
    const VNUser3InUse user3InUse;
    为 (const auto& itr : m_ifaceModSyms) {
        AstIface* const nodep = itr.first;
        VSymEnt* const symp = itr.second;
        LinkDotIfaceVisitor{nodep, symp, this};
    }
    m_ifaceModSyms.clear();
}

//======================================================================

class LinkDotResolveVisitor final : public VNVisitor {
    // NODE STATE
    // Cleared on global
    //  *::user1p()             -> See LinkDotState
    //  *::user2p()             -> See LinkDotState
    //  *::user3()              // bool. Processed
    //  *::user4()              -> See LinkDotState
    const VNUser3InUse m_inuser3;

    // TYPES
    enum DotPosition : uint8_t {
        // Must match ascii() method below
        DP_NONE = 0,  // Not under a DOT
        DP_PACKAGE,  // {package-or-class}:: DOT
        DP_FIRST,  // {scope-or-var} DOT
        DP_SCOPE,  // DOT... {scope-or-var} DOT
        DP_FINAL,  // [DOT...] {var-or-func-or-dtype} with no following dots
        DP_MEMBER  // DOT {member-name} [DOT...]
    };

    // STATE
    LinkDotState* const m_statep;  // State, including dotted symbol table
    VSymEnt* m_curSymp = nullptr;  // SymEnt for current lookup point
    VSymEnt* m_modSymp = nullptr;  // SymEnt for current module
    VSymEnt* m_pinSymp = nullptr;  // SymEnt for pin lookups
    VSymEnt* m_randSymp = nullptr;  // SymEnt for randomize target class's lookups
    const AstCell* m_cellp = nullptr;  // Current cell
    AstNodeModule* m_modp = nullptr;  // Current module
    AstNodeFTask* m_ftaskp = nullptr;  // Current function/task
    AstMethodCall* m_randMethodCallp = nullptr;  // Current randomize() method call
    整数 m_modportNum = 0;  // Uniqueify modport numbers
    整数 m_indent = 0;  // Indentation (tree depth) for debug
    bool m_inSens = false;  // True if in senitem
    const AstWith* m_currentWithp = nullptr;  // Enclosing 'with' (nullptr if none)
    std::set<std::string>
        m_restrictedNamesUsed;  // Names from current 'with (id_list)' that resolved into target
    bool m_genericIfaceModule = false;  // True if in module containing generic interface
    std::map<std::string, AstNode*> m_ifClassImpNames;  // Names imported from interface class
    // Inherited interface implementations needing a forward in the current class, by name
    std::map<std::string, const VSymEnt*> m_ifForwards;
    std::set<AstClass*> m_extendsParam;  // Classes that have a parameterized super class
                                         // (except the default instances)
                                         // They are added to the set only in linkDotPrimary.
    bool m_insideClassExtParam = false;  // Inside a class from m_extendsParam
    AstNew* m_explicitSuperNewp = nullptr;  // Hit a "super.new" call inside a "new" function
    std::map<AstNode*, AstPin*> m_usedPins;  // Pin used in this cell, map to duplicate
    std::map<AstNode*, std::map<std::string, AstPin*>> m_usedDefParamPins;
    // Defparam pins used for a given formal, by hierarchical path
    std::map<std::string, AstNodeModule*> m_modulesToRevisit;  // Modules to revisit a second time
    AstNode* m_lastDeferredp = nullptr;  // Last node which requested a revisit of its module
    AstNodeDType* m_packedArrayDtp = nullptr;  // Datatype reference for packed array
    bool m_inPackedArray = false;  // Currently traversing a packed array tree
    bool m_replaceWithAlias
        = true;  // Replace VarScope with an alias. Used in the handling of AstAlias
    bool m_isParam = false;  // Specifies whether currently visiting param variable
    bool m_resolvingTypedef = false;  // Currently traversing a Typedef tree

    struct DotStates final {
        DotPosition m_dotPos;  // Scope part of dotted resolution
        VSymEnt* m_dotSymp;  // SymEnt for dotted AstParse lookup
        const AstDot* m_dotp;  // Current dot
        bool m_super;  // Starts with super reference
        bool m_unresolvedCell;  // Unresolved cell, needs help from V3Param
        bool m_unresolvedClass;  // Unresolved class reference, needs help from V3Param
        bool m_unresolvedGenericIface;  // Unresolved generic interface, needs help from V3Param
        bool m_genBlk;  // Contains gen block reference
        AstNode* m_unlinkedScopep;  // Unresolved scope, needs corresponding VarXRef
        AstDisable* m_disablep;  // Disable statement under which the reference is
        bool m_dotErr;  // Error found in dotted resolution, ignore upwards
        string m_dotText;  // String of dotted names found in below parseref
        DotStates() { init(nullptr); }
        ~DotStates() = default;
        void init(VSymEnt* curSymp, bool unresolvedClass = false) {
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
        string ascii() const {
            static const char* const names[]
                = {"DP_NONE", "DP_PACKAGE", "DP_FIRST", "DP_SCOPE", "DP_FINAL", "DP_MEMBER"};
            std::ostringstream sstr;
            sstr << "ds=" << names[m_dotPos];
            sstr << "  dse" << cvtToHex(m_dotSymp);
            const string dsname = m_dotSymp->nodep()->name().substr(0, 8);
            sstr << "(" << m_dotSymp->nodep()->typeName() << (dsname.empty() ? "" : ":") << dsname
                 << ")";
            如果 (m_dotErr) sstr << "  [dotErr]";
            如果 (m_super) sstr << "  [super]";
            如果 (m_unresolvedCell) sstr << "  [unrCell]";
            如果 (m_unresolvedClass) sstr << "  [unrClass]";
            如果 (m_unresolvedGenericIface) sstr << "  [unrGIface]";
            如果 (m_genBlk) sstr << "  [genBlk]";
            sstr << "  txt=" << m_dotText;
            返回 sstr.str();
        }
    } m_ds;  // State to preserve across recursions

    // METHODS - Variables
    AstVar* createImplicitVar(VSymEnt* /*lookupSymp*/, AstParseRef* nodep, AstNodeModule* modp,
                              VSymEnt* moduleSymp, bool noWarn) {
        // Create implicit after warning
        如果 (!noWarn) {
            如果 (nodep->fileline()->warnIsOff(V3ErrorCode::I_DEF_NETTYPE_WIRE)) {
                const string suggest = m_statep->suggestSymFallback(moduleSymp, nodep->name(),
                                                                    LinkNodeMatcherVar{});
                nodep->v3error("Signal definition not found, and implicit disabled with "
                               "`default_nettype: "
                               << nodep->prettyNameQ() << '\n'
                               << (suggest.empty() ? "" : nodep->warnMore() + suggest));

            }
            // Bypass looking for suggestions if IMPLICIT is turned off
            // as there could be thousands of these suppressed in large netlists
            否则 如果 (!nodep->fileline()->warnIsOff(V3ErrorCode::IMPLICIT)) {
                const string suggest = m_statep->suggestSymFallback(moduleSymp, nodep->name(),
                                                                    LinkNodeMatcherVar{});
                nodep->v3warn(IMPLICIT,
                              "Signal definition not found, creating implicitly: "
                                  << nodep->prettyNameQ() << '\n'
                                  << (suggest.empty() ? "" : nodep->warnMore() + suggest));
            }
        }
        AstVar* const newp
            = new AstVar{nodep->fileline(), VVarType::WIRE, nodep->name(), VFlagLogicPacked{}, 1};
        newp->lifetime(VLifetime::STATIC_IMPLICIT);
        newp->trace(modp->modTrace());
        modp->addStmtsp(newp);
        // Link it to signal list, must add the variable under the module;
        // current scope might be lower now
        m_statep->insertSym(moduleSymp, newp->name(), newp, nullptr /*classOrPackagep*/);
        返回 newp;
    }
    AstVar* foundToVarp(const VSymEnt* symp, AstNode* nodep, VAccess access) {
        // Return a variable if possible, auto converting a modport to variable
        如果 (!symp) {
            返回 nullptr;
        } 否则 如果 (VN_IS(symp->nodep(), Var)) {
            返回 VN_AS(symp->nodep(), Var);
        } 否则 如果 (VN_IS(symp->nodep(), ModportVarRef)) {
            AstModportVarRef* const snodep = VN_AS(symp->nodep(), ModportVarRef);
            AstVar* const varp = snodep->varp();
            如果 (access.isWriteOrRW() && snodep->direction().isReadOnly()) {
                nodep->v3error("Attempt to drive input-only modport: " << nodep->prettyNameQ());
            }  // else other simulators don't warn about reading, and IEEE doesn't say illegal
            返回 varp;
        } 否则 {
            返回 nullptr;
        }
    }

    // Look up a virtual port through modport expression mapping.
    // When a dotted reference uses a modport with expression syntax, the virtual port name
    // maps to the real signal. This helper resolves that mapping during scope creation.
    // Returns the VarScope for the real signal, or nullptr if not found.
    AstVarScope* findVirtualPortVarScope(VSymEnt* dotSymp, const string& portName) {
        const AstVarScope* const dotVscp = VN_CAST(dotSymp->nodep(), VarScope);
        const AstVar* const dotVarp = dotVscp ? dotVscp->varp() : nullptr;
        如果 (!dotVarp) 返回 nullptr;

        AstNodeDType* dtypep = dotVarp->childDTypep();
        如果 (!dtypep) dtypep = dotVarp->subDTypep();
        如果 (!dtypep) 返回 nullptr;
        const AstIfaceRefDType* const ifaceRefp = VN_CAST(dtypep, IfaceRefDType);
        如果 (!ifaceRefp || !ifaceRefp->modportp() || !ifaceRefp->ifaceViaCellp()) 返回 nullptr;

        // Get the interface cell's symbol table
        VSymEnt* const ifaceCellSymp = m_statep->getNodeSym(ifaceRefp->ifaceViaCellp());

        // Look up modport by name in interface cell
        VSymEnt* const modportSymp = ifaceCellSymp->findIdFallback(ifaceRefp->modportp()->name());
        如果 (!modportSymp) 返回 nullptr;

        // Look up virtual port name in modport symbol table
        string baddot;
        VSymEnt* const virtPortp = m_statep->findSymPrefixed(modportSymp, portName, baddot, true);
        如果 (!virtPortp) 返回 nullptr;

        // Symbol may point to VarScope or Var
        如果 (AstVarScope* vscp = VN_CAST(virtPortp->nodep(), VarScope)) 返回 vscp;

        // Symbol points to Var - look up VarScope by name in interface cell
        如果 (const AstVar* const realVarp = VN_CAST(virtPortp->nodep(), Var)) {
            VSymEnt* const realFoundp
                = m_statep->findSymPrefixed(ifaceCellSymp, realVarp->name(), baddot, true);
            返回 realFoundp ? VN_CAST(realFoundp->nodep(), VarScope) : nullptr;
        }
        返回 nullptr;
    }

    // Capture a ParamTypeDType reference for interface typedef retargeting.
    // Called when a RefDType resolves to a ParamTypeDType owned by an interface.
    void captureIfaceParamType(AstRefDType* nodep, AstParamTypeDType* defp) {
        如果 (!V3LinkDotIfaceCapture::enabled() || !m_statep->forPrimary()) 返回;
        AstNodeModule* const defOwnerModp = V3LinkDotIfaceCapture::findOwnerModule(defp);
        如果 (!defOwnerModp || !VN_IS(defOwnerModp, Iface)) 返回;
        AstCell* const cellForCapture
            = m_ds.m_dotSymp ? VN_CAST(m_ds.m_dotSymp->nodep(), Cell) : nullptr;
        如果 (!cellForCapture) 返回;
        UINFO(9, indent() << "iface capture add paramtype " << nodep
                          << " iface=" << defOwnerModp->prettyNameQ());
        V3LinkDotIfaceCapture::addParamType(nodep, cellForCapture->name(), m_modp, defp,
                                            defOwnerModp->name());
    }

    AstNodeStmt* addImplicitSuperNewCall(AstFunc* const nodep,
                                         const AstClassExtends* const classExtendsp) {
        // Returns the added node
        FileLine* const fl = nodep->fileline();
        AstArg* argsp = nullptr;
        如果 (classExtendsp->argsp()) argsp = classExtendsp->argsp()->cloneTree(true);
        AstNew* const newExprp = new AstNew{fl, argsp};
        newExprp->isImplicit(true);
        AstDot* const superNewp = new AstDot{fl, false, new AstParseRef{fl, "super"}, newExprp};
        AstNodeStmt* const superNewStmtp = superNewp->makeStmt();
        为 (AstNode* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            // super.new shall be the first statement (IEEE 1800-2023 8.15)
            // but some nodes (such as variable declarations and typedefs) should stay before
            如果 (VN_IS(stmtp, NodeStmt)) {
                stmtp->addHereThisAsNext(superNewStmtp);
                返回 superNewStmtp;
            }
        }
        // There were no statements
        nodep->addStmtsp(superNewStmtp);
        返回 superNewStmtp;
    }
    void checkNoDot(AstNode* nodep) {
        如果 (VL_UNLIKELY(!nodep)) {
            UINFO(9, indent() << "iface capture null node passed to checkNoDot; dot state="
                              << m_ds.ascii());
            m_ds.m_dotErr = true;
            返回;
        }
        如果 (VL_UNLIKELY(m_ds.m_dotPos != DP_NONE)) {
            UINFO(9, indent() << "ds=" << m_ds.ascii());
            UINFO(9, indent() << "iface capture checkNoDot hit node=" << nodep
                              << " dotState=" << m_ds.ascii());
            如果 (VL_UNLIKELY(!m_ds.m_dotp)) {
                nodep->v3error("Syntax error: Not expecting " << nodep->type()
                                                              << " here (missing dot context)");
            } 否则 {
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
    AstVar* findIfaceTopVarp(AstNode* nodep, VSymEnt* parentEntp, const string& name) {
        const string findName = name + "__Viftop";
        VSymEnt* const ifaceSymp = parentEntp->findIdFallback(findName);
        AstVar* const ifaceTopVarp = ifaceSymp ? VN_AS(ifaceSymp->nodep(), Var) : nullptr;
        UASSERT_OBJ(ifaceTopVarp, nodep, "Can't find interface var ref: " << findName);
        返回 ifaceTopVarp;
    }
    AstRSProd* findProd(AstNode* nodep, VSymEnt* parentEntp, const string& name) {
        const VSymEnt* const foundp = parentEntp->findIdFallback(name);
        AstRSProd* foundNodep = foundp ? VN_CAST(foundp->nodep(), RSProd) : nullptr;
        如果 (!foundNodep) {
            VSpellCheck speller;
            LinkNodeMatcherProd matcher;
            parentEntp->candidateIdFlat(&speller, &matcher, false);
            const string suggest = speller.bestCandidateMsg(name);
            UINFO(1, "   ErrParseRef curSymp=se" << cvtToHex(parentEntp));
            nodep->v3error("Production " << AstNode::prettyNameQ(name) << " not found\n"
                                         << (suggest.empty() ? "" : nodep->warnMore() + suggest));
            返回 nullptr;
        }
        返回 foundNodep;
    }
    void duplicatePinError(AstPin* nodep, AstNode* origp, const char* whatp) {
        nodep->v3error("Duplicate " << whatp << " connection: " << nodep->prettyNameQ() << '\n'
                                    << nodep->warnContextPrimary() << '\n'
                                    << origp->warnOther() << "... Location of original " << whatp
                                    << " connection\n"
                                    << origp->warnContextSecondary());
    }
    void markAndCheckPinDup(AstPin* nodep, AstNode* refp, const char* whatp) {
        const auto pair = m_usedPins.emplace(refp, nodep);
        如果 (pair.second) {
            如果 (!nodep->paramPath().empty())
                m_usedDefParamPins[refp].emplace(nodep->paramPath(), nodep);
            返回;
        } 否则 {
            AstNode* const origp = pair.first->second;
            如果 (nodep->paramPath().empty() || VN_AS(origp, Pin)->paramPath().empty()) {
                duplicatePinError(nodep, origp, whatp);
                返回;
            }
        }

        auto& defParamPins = m_usedDefParamPins[refp];
        const auto defParamIt = defParamPins.find(nodep->paramPath());
        如果 (defParamIt != defParamPins.end()) {
            duplicatePinError(nodep, defParamIt->second, whatp);
            返回;
        }
        defParamPins.emplace(nodep->paramPath(), nodep);
    }
    static bool extendsClass(const AstClass* classp, const AstClass* targetp) {
        // Return true if classp inherits from targetp through already resolved extends
        // If performance of this becomes a problem, use just a counter to check
        // exceeds some number of iterations then perform the more expensive analysis
        std::set<const AstClass*> visited;
        std::vector<const AstClass*> todo{classp};
        当 (!todo.empty()) {
            const AstClass* const currp = todo.back();
            todo.pop_back();
            如果 (currp == targetp) 返回 true;
            如果 (!visited.insert(currp).second) continue;
            为 (const AstClassExtends* cextp = currp->extendsp(); cextp;
                 cextp = VN_AS(cextp->nextp(), ClassExtends)) {
                如果 (const AstClass* const basep = cextp->classOrNullp()) todo.push_back(basep);
            }
        }
        返回 false;
    }
    static AstClocking* sensClockingp(AstNode* nodep) {
        // Return the clocking block referenced by nodep, either directly or through a modport
        如果 (AstClocking* const clockingp = VN_CAST(nodep, Clocking)) 返回 clockingp;
        如果 (const AstModportClockingRef* const clockingRefp = VN_CAST(nodep, ModportClockingRef))
            返回 clockingRefp->clockingp();
        返回 nullptr;
    }
    VSymEnt* getCreateClockingEventSymEnt(AstClocking* clockingp) {
        AstVar* const eventp = clockingp->ensureEventp(true);
        如果 (!eventp->user1p()) eventp->user1p(new VSymEnt{m_statep->symsp(), eventp});
        返回 reinterpret_cast<VSymEnt*>(eventp->user1p());
    }
    VSymEnt* findInlinedDotsSym(AstNode* nodep, const string& inlinedDots) {
        // Return scope that applies for searching, given renaming due to module inlining.
        // Dotted lookup is always relative to module, as maybe variable
        // name lower down with same scope name we want to ignore (t_math_divw)
        VSymEnt* dotSymp = m_modSymp;
        const string inl = AstNode::dedotName(inlinedDots);
        string baddot;
        VSymEnt* okSymp = nullptr;
        dotSymp = m_statep->findDotted(nodep->fileline(), dotSymp, inl, baddot, okSymp, false);
        UASSERT_OBJ(dotSymp, nodep,
                    "Couldn't resolve inlined scope " << AstNode::prettyNameQ(baddot)
                                                      << " in: " << inlinedDots);
        UINFO(9, indent() << "findInlinedDotsSym " << dotSymp
                          << " search start due to inlinedDots=" << inlinedDots);
        返回 dotSymp;
    }
    VSymEnt* findDottedRefSymp(AstNode* nodep, VSymEnt* lookupSymp, const string& dotted,
                               string& baddot, VSymEnt*& okSymp, bool* modportp = nullptr) {
        // Return scope to search for a relinked hierarchical reference
        // (AstVarXRef or AstEnumItemRef), or nullptr after error if not found
        // If modportp is given, set true when the reference went through a modport
        如果 (modportp) *modportp = false;
        VSymEnt* dotSymp = m_statep->findDotted(nodep->fileline(), lookupSymp, dotted, baddot,
                                                okSymp, true);  // Maybe nullptr
        如果 (!dotSymp) {
            nodep->v3error(
                "Can't find definition of "
                << (!baddot.empty() ? AstNode::prettyNameQ(baddot) : nodep->prettyNameQ()) << '\n'
                << nodep->warnContextPrimary());
            返回 nullptr;
        }
        如果 (const AstVar* const varp = VN_CAST(dotSymp->nodep(), Var)) {
            如果 (const AstIfaceRefDType* const ifaceRefp
                = VN_CAST(varp->childDTypep(), IfaceRefDType)) {
                如果 (ifaceRefp->modportp()) {
                    dotSymp = m_statep->getNodeSym(ifaceRefp->modportp());
                    如果 (modportp) *modportp = true;
                } 否则 {
                    dotSymp = m_statep->getNodeSym(ifaceRefp->ifacep());
                }
            }
        } 否则 如果 (const AstModportClockingRef* const clockingRefp
                   = VN_CAST(dotSymp->nodep(), ModportClockingRef)) {
            dotSymp = m_statep->getNodeSym(clockingRefp->clockingp());
        }
        返回 dotSymp;
    }

    static bool isParamedClassRefDType(const AstNode* classp) {
        当 (const AstRefDType* const refp = VN_CAST(classp, RefDType))
            classp = refp->subDTypep();
        返回 (VN_IS(classp, ClassRefDType) && VN_AS(classp, ClassRefDType)->paramsp())
               || VN_IS(classp, ParamTypeDType);
    }
    bool isParamedClassRef(const AstNode* nodep) {
        // Is this a parameterized reference to a class, or a reference to class parameter
        如果 (const auto* classRefp = VN_CAST(nodep, ClassOrPackageRef)) {
            如果 (classRefp->paramsp()) 返回 true;
            const auto* classp = classRefp->classOrPackageNodep();
            当 (const auto* typedefp = VN_CAST(classp, Typedef)) classp = typedefp->subDTypep();
            返回 isParamedClassRefDType(classp);
        }
        返回 isParamedClassRefDType(nodep);
    }
    VSymEnt* getThisClassSymp() {
        VSymEnt* classSymp = m_ds.m_dotSymp;
        当 (classSymp && !VN_IS(classSymp->nodep(), Class)) {
            classSymp = classSymp->parentp();
        }
        返回 classSymp;
    }
    // Is the current scope within the given class, including within its nested classes
    bool insideClass(const AstClass* classp) const {
        为 (const VSymEnt* symp = m_curSymp; symp; symp = symp->parentp()) {
            如果 (symp->nodep() == classp) 返回 true;
        }
        返回 false;
    }
    // 'randomize() with' target class not yet specialized; defer to post-V3Param pass
    bool randTargetUnresolved() const {
        返回 m_statep->forPrimary() && m_currentWithp && m_randMethodCallp && !m_randSymp;
    }
    // Is 'funcp' of 'classp' virtual, directly or by overriding a virtual method of a base
    // class (IEEE 1800-2023 8.20)
    bool isVirtualMethod(const AstNodeFTask* funcp, const AstClass* classp) const {
        如果 (funcp->isVirtual()) 返回 true;
        为 (const AstClassExtends* cextp = classp->extendsp(); cextp;
             cextp = VN_AS(cextp->nextp(), ClassExtends)) {
            AstClass* const basep = cextp->classOrNullp();
            如果 (!basep) continue;
            const VSymEnt* const foundp = m_statep->getNodeSym(basep)->findIdFlat(funcp->name());
            const AstNodeFTask* const baseFuncp
                = foundp ? VN_CAST(foundp->nodep(), NodeFTask) : nullptr;
            const AstClass* const baseClassp = foundp && foundp->parentp()
                                                   ? VN_CAST(foundp->parentp()->nodep(), Class)
                                                   : nullptr;
            如果 (baseFuncp && baseClassp && isVirtualMethod(baseFuncp, baseClassp)) 返回 true;
        }
        返回 false;
    }
    // Is imported 'implp' an implementation of interface method 'ifSymp' (IEEE 1800-2023
    // 8.26.2)? If its class does not derive from the interface class, it needs a forwarding
    // override in the derived class to be reachable through the interface.
    bool inheritedImplements(const VSymEnt* implp, const VSymEnt* ifSymp, bool& needForwardr) {
        needForwardr = false;
        const AstNodeFTask* const implFuncp = VN_CAST(implp->nodep(), NodeFTask);
        如果 (!implFuncp || implFuncp->pureVirtual() || implFuncp->isStatic()) 返回 false;
        const AstClass* const implClassp
            = implp->parentp() ? VN_CAST(implp->parentp()->nodep(), Class) : nullptr;
        const AstClass* const ifClassp
            = ifSymp->parentp() ? VN_CAST(ifSymp->parentp()->nodep(), Class) : nullptr;
        如果 (!implClassp || !ifClassp) 返回 false;
        如果 (extendsClass(implClassp, ifClassp)) 返回 true;
        如果 (!isVirtualMethod(implFuncp, implClassp)) 返回 false;
        needForwardr = true;
        返回 true;
    }
    // Add 'implp' override to 'classp' that calls super.implp, so the C++ method of an
    // implemented interface class is overridden
    void addInterfaceForward(AstClass* classp, AstNodeFTask* implp, AstClass* implClassp) {
        FileLine* const flp = implp->fileline();
        AstVar* fvarp = nullptr;
        AstNodeFTask* newp;
        如果 (AstFunc* const implFuncp = VN_CAST(implp, Func)) {
            fvarp = VN_AS(implFuncp->fvarp(), Var)->cloneTree(false);
            newp = new AstFunc{flp, implp->name(), nullptr, fvarp};
        } 否则 {
            newp = new AstTask{flp, implp->name(), nullptr};
        }
        newp->classMethod(true);
        newp->isVirtual(true);
        newp->lifetime(implp->lifetime());
        AstArg* argsp = nullptr;
        为 (AstNode* stmtp = implp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstVar* const portp = VN_CAST(stmtp, Var);
            如果 (!portp || !portp->isIO()) continue;
            AstVar* const newPortp = portp->cloneTree(false);
            newp->addStmtsp(newPortp);
            argsp = AstNode::addNext(
                argsp, new AstArg{flp, "", new AstVarRef{flp, newPortp, VAccess::READ}});
        }
        AstNodeFTaskRef* callp;
        如果 (AstFunc* const implFuncp = VN_CAST(implp, Func)) {
            callp = new AstFuncRef{flp, implFuncp, argsp};
        } 否则 {
            callp = new AstTaskRef{flp, VN_AS(implp, Task), argsp};
        }
        callp->classOrPackagep(implClassp);
        callp->superReference(true);
        如果 (fvarp) {
            newp->addStmtsp(new AstAssign{flp, new AstVarRef{flp, fvarp, VAccess::WRITE}, callp});
        } 否则 {
            newp->addStmtsp(callp->makeStmt());
        }
        UINFO(9, indent() << "Interface forward " << newp);
        classp->addMembersp(newp);
    }
    void importDerivedClass(AstClass* derivedClassp, VSymEnt* baseSymp, AstClass* baseClassp) {
        // Also used for standard 'extends' from a base class
        UINFO(8, indent() << "importDerivedClass to " << derivedClassp << " from " << baseClassp);
        为 (VSymEnt::const_iterator it = baseSymp->begin(); it != baseSymp->end(); ++it) {
            如果 (AstNode* baseSubp = it->second->nodep()) {
                UINFO(8, indent() << "  SymFunc " << baseSubp);
                const string impOrExtends
                    = baseClassp->isInterfaceClass() ? " implements " : " extends ";
                如果 (VN_IS(baseSubp, NodeFTask)) {
                    const VSymEnt* const foundp = m_curSymp->findIdFlat(baseSubp->name());
                    const AstNodeFTask* const baseFuncp = VN_CAST(baseSubp, NodeFTask);
                    如果 (!baseFuncp || !baseFuncp->pureVirtual()) continue;
                    bool needForward = false;
                    const bool existsInDerived
                        = foundp
                          && (!foundp->imported()
                              || inheritedImplements(foundp, it->second, needForward));
                    如果 (needForward && m_statep->forPrimary()
                        && !derivedClassp->isInterfaceClass()) {
                        m_ifForwards.emplace(baseSubp->name(), foundp);
                    }
                    如果 (m_statep->forPrimary() && !existsInDerived
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
                    const auto itn = m_ifClassImpNames.find(baseSubp->name());
                    如果 (m_statep->forPrimary() && !existsInDerived
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
                如果 (VN_IS(baseSubp, Constraint)) {
                    const VSymEnt* const foundp = m_curSymp->findIdFlat(baseSubp->name());
                    const AstConstraint* const baseFuncp = VN_CAST(baseSubp, Constraint);
                    如果 (!baseFuncp || !baseFuncp->isKwdPure()) continue;
                    const bool existsInDerived = foundp && !foundp->imported();
                    如果 (m_statep->forPrimary() && !existsInDerived
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
    void importSymbolsFromExtended(AstClass* const nodep, AstClassExtends* const cextp) {
        AstClass* const baseClassp = cextp->classp();
        VSymEnt* const srcp = m_statep->getNodeSym(baseClassp);
        importDerivedClass(nodep, srcp, baseClassp);
        如果 (!cextp->isImplements()) m_curSymp->importFromClass(m_statep->symsp(), srcp);
    }
    void classExtendImport(AstClass* nodep) {
        // A class reference might be to a class that is later in Ast due to
        // e.g. parameterization or referring to a "class (type T) extends T"
        // Resolve it so later Class:: references into its base classes work
        symIterateNull(nodep, m_statep->getNodeSym(nodep));
    }

    void checkDeclOrder(AstNode* nodep, AstNode* declp) {
        const uint32_t declTokenNum = declp->declTokenNum();
        如果 (!declTokenNum) 返回;  // Not implemented/valid on this object
        如果 (nodep->fileline()->tokenNum() < declTokenNum) {
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
    void checkMemberDeclOrder(AstNode* nodep, AstMemberDType* declp) {
        const uint32_t declTokenNum = declp->fileline()->tokenNum();
        如果 (nodep->fileline()->tokenNum() < declTokenNum) {
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

    void replaceWithCheckBreak(AstNode* oldp, AstNodeDType* newp) {
        // Flag now to avoid V3Broken throwing an internal error
        如果 (oldp->wouldBreak(newp)) {
            newp->v3error(
                "Data type used where a non-data type is expected: " << newp->prettyNameQ());
            oldp->replaceWith(new AstConst{newp->fileline(), AstConst::BitFalseErroring{}});
        } 否则 {
            oldp->replaceWith(newp);
        }
    }

    // Marks the current module to be revisited after the initial AST iteration
    void revisitLater(AstNode* deferredNodep) {
        // Need to revisit entire module to build up all the necessary context
        m_lastDeferredp = deferredNodep;
        m_modulesToRevisit.insert(std::make_pair(m_modp->name(), m_modp));
    }

    void updateVarUse(AstVar* nodep) {
        // Avoid dotted.PARAM false positive when in a parameter block
        // that is if ()'ed off by same dotted name as another block
        如果 (nodep && nodep->isParam()) nodep->usedParam(true);
    }

    static VSymEnt* findIdFallbackSkipMemberDType(VSymEnt* lookp, const string& name) {
        VSymEnt* shadowEntp = nullptr;  // Shadowing variable: not a type, kept for error report
        当 (lookp) {
            VSymEnt* const foundp = lookp->findIdFlat(name);
            如果 (foundp && !VN_IS(foundp->nodep(), MemberDType)) {
                // Non-type entries are not type candidates
                // (IEEE 1800-2023 6.18); skip them so an enclosing
                // type is found, but keep one to preserve the "found: ..." error.
                如果 (VN_IS(foundp->nodep(), Typedef) || VN_IS(foundp->nodep(), ParamTypeDType)
                    || VN_IS(foundp->nodep(), Class)) {
                    返回 foundp;
                }
                如果 (!shadowEntp) shadowEntp = foundp;
            }
            lookp = lookp->fallbackp();
        }
        返回 shadowEntp;
    }

    void symIterateChildren(AstNode* nodep, VSymEnt* symp) {
        // Iterate children, changing to given context, with restore to old context
        VL_RESTORER_COPY(m_ds);
        VL_RESTORER(m_curSymp);
        m_curSymp = symp;
        m_ds.init(m_curSymp);
        iterateChildren(nodep);
    }
    void symIterateNull(AstNode* nodep, VSymEnt* symp) {
        // Iterate node, changing to given context, with restore to old context
        VL_RESTORER_COPY(m_ds);
        VL_RESTORER(m_curSymp);
        m_curSymp = symp;
        m_ds.init(m_curSymp);
        iterateNull(nodep);
    }
    static const AstNodeDType* getExprDTypep(const AstNodeExpr* selp) {
        当 (const AstNodePreSel* const sp = VN_CAST(selp, NodePreSel)) selp = sp->fromp();
        如果 (const AstMemberSel* const sp = VN_CAST(selp, MemberSel)) {
            如果 (const AstNodeDType* dtypep = getExprDTypep(sp->fromp())) {
                如果 (const AstClassRefDType* const classRefp = VN_CAST(dtypep, ClassRefDType)) {
                    const AstClass* const classp = classRefp->classp();
                    const bool found = classp->existsMember(
                        [&dtypep, name = selp->name()](const AstClass*, const AstVar* nodep) {
                            dtypep = nodep->childDTypep();
                            返回 nodep->name() == name;
                        });
                    如果 (found) 返回 dtypep->elemDTypep();
                    selp->v3error("Class " << classRefp->prettyNameQ()
                                           << " does not contain field " << selp->prettyNameQ());
                } 否则 {
                    selp->v3fatalSrc("Member selection on expression of type "
                                     << dtypep->prettyDTypeNameQ()
                                     << ", which is not a class type");
                }
            }
        } 否则 如果 (const AstNodeVarRef* const varRefp = VN_CAST(selp, NodeVarRef)) {
            返回 varRefp->varp()->childDTypep()->elemDTypep();
        }
        返回 nullptr;
    }
    static const AstVar* getNextVarp(const AstNode* stmtsp) {
        // Only IO ports, as parameters are on paramsp(), not the pinsp() list paired here
        当 (stmtsp) {
            如果 (const AstVar* const varp = VN_CAST(stmtsp, Var)) {
                如果 (varp->isIO()) 返回 varp;
            }
            stmtsp = stmtsp->nextp();
        }
        返回 nullptr;
    }
    // Introduce implicit parameters for modules with generic interafeces
    void addImplicitParametersOfGenericIface(AstCell* const nodep, const AstModule* const modp) {
        // Get Param number
        整数 paramNum = 1;
        为 (AstPin* paramp = nodep->paramsp(); paramp; paramp = VN_CAST(paramp->nextp(), Pin)) {
            ++paramNum;
        }

        // Add each implicit parameter type
        const AstVar* modIfaceVarp = getNextVarp(modp->stmtsp());
        为 (const AstPin* pinp = nodep->pinsp(); pinp && modIfaceVarp;
             pinp = VN_CAST(pinp->nextp(), Pin),
                           modIfaceVarp = getNextVarp(modIfaceVarp->nextp())) {
            如果 (modIfaceVarp->varType() != VVarType::IFACEREF
                || !VN_IS(modIfaceVarp->childDTypep()->elemDTypep(), IfaceGenericDType)) {
                continue;
            }
            AstNode* exprp = pinp->exprp();
            如果 (!exprp) {
                modIfaceVarp->v3error("Interface port "
                                      << modIfaceVarp->prettyNameQ()
                                      << " is not connected to interface/modport pin expression");
                continue;
            }
            当 (const AstNodePreSel* const preSelp = VN_CAST(exprp, NodePreSel)) {
                exprp = preSelp->fromp();
            }
            // Resolve pin expression to the enclosing module's port Var. At primary
            // LinkDot the expression may still be an AstParseRef, so also look up by name.
            AstVar* enclosingVarp = nullptr;
            const AstVarRef* const varRefp = VN_CAST(exprp, VarRef);
            如果 (varRefp) {
                enclosingVarp = varRefp->varp();
            } 否则 如果 (const AstParseRef* const parseRefp = VN_CAST(exprp, ParseRef)) {
                如果 (m_modp) {
                    如果 (VSymEnt* const symp
                        = m_statep->getNodeSym(m_modp)->findIdFlat(parseRefp->name())) {
                        enclosingVarp = VN_CAST(symp->nodep(), Var);
                    }
                }
            }
            如果 (enclosingVarp && enclosingVarp->varType() == VVarType::IFACEREF
                && VN_IS(enclosingVarp->childDTypep()->skipRefp(), IfaceGenericDType)) {
                // Nested generic-iface forwarding (#7454): enclosing port is itself still
                // generic, so emit a placeholder __VGIfaceParam pin carrying a VarRef to
                // the outer port. V3Param rewrites it to the concrete IfaceRefDType once
                // the enclosing module is specialized (see
                // ParamProcessor::resolveGenericIfaceForwardingPins for the ordering
                // constraint that keeps the rewrite inside V3Param).
                AstVarRef* const fwdRefp
                    = new AstVarRef{exprp->fileline(), enclosingVarp, VAccess::READ};
                AstPin* const newPinp = new AstPin{
                    pinp->fileline(), paramNum, "__VGIfaceParam" + modIfaceVarp->name(), fwdRefp};
                newPinp->param(true);
                visit(newPinp);
                nodep->addParamsp(newPinp);
            } 否则 如果 (varRefp) {
                const AstVar* const varp = varRefp->varp();
                如果 (const AstIfaceRefDType* const refp
                    = VN_CAST(varp->childDTypep()->elemDTypep(), IfaceRefDType)) {
                    AstIface* const ifacep = VN_AS(refp->cellp()->modp(), Iface);
                    AstIfaceRefDType* newIfaceRefp;
                    如果 (refp->modportp()) {
                        newIfaceRefp = new AstIfaceRefDType{
                            refp->fileline(), refp->modportFileline(), m_modp->name(),
                            ifacep->name(), refp->modportName()};
                    } 否则 {
                        newIfaceRefp = new AstIfaceRefDType{refp->fileline(), m_modp->name(),
                                                            ifacep->name()};
                    }
                    newIfaceRefp->ifacep(ifacep);
                    如果 (refp->cellp()->paramsp()) {
                        newIfaceRefp->addParamsp(refp->cellp()->paramsp()->cloneTree(true));
                    }
                    UASSERT_OBJ(pinp->name().find("__pinNumber") == 0
                                    || pinp->name() == modIfaceVarp->name(),
                                pinp, "Not found interface with such name");
                    AstPin* const newPinp
                        = new AstPin{pinp->fileline(), paramNum,
                                     "__VGIfaceParam" + modIfaceVarp->name(), newIfaceRefp};
                    newPinp->param(true);
                    visit(newPinp);
                    nodep->addParamsp(newPinp);
                } 否则 {
                    varRefp->v3error("Generic interfaces can only connect to an interface and "
                                     << varp->prettyNameQ() << " is "
                                     << (varp->childDTypep()
                                             ? "of type " + varp->childDTypep()->prettyDTypeNameQ()
                                             : "not an interface"));
                }
            } 否则 {
                exprp->v3error("Expected an interface but " << exprp->prettyNameQ()
                                                            << " is not an interface");
            }
            ++paramNum;
        }
    }

    // Returns true and issues error iff 'varp' cannot be accesed with a VarXRef
    // in a hier_block given by 'scopeSymp'
    bool errorHierNonPort(AstVarXRef* refp, AstVar* varp, VSymEnt* scopeSymp) {
        // OK to access ports on hier_block
        如果 (varp->isIO()) 返回 false;
        // OK if not an instance
        const AstCell* const cellp = VN_CAST(scopeSymp->nodep(), Cell);
        如果 (!cellp) 返回 false;
        // Ok if not a hier_block
        const AstNodeModule* const modp = cellp->modp();
        如果 (!modp->hierBlock()) 返回 false;
        // Bad
        refp->v3error("Cannot access non-port symbols inside hierarchical block");
        返回 true;
    }

#define LINKDOT_VISIT_START() \
    VL_RESTORER(m_indent); \
    ++m_indent;

    string indent() const {
        string result = "";
        result.insert(0, m_indent, ':');
        返回 result + " ";
    }

    // VISITORS
    void visit(AstNetlist* nodep) override {
        // Recurse..., backward as must do packages before using packages
        iterateChildrenBackwardsConst(nodep);
    }
    void visit(AstTypeTable*) override {}
    void visit(AstNodeModule* nodep) override {
        如果 (nodep->dead()) 返回;
        LINKDOT_VISIT_START();
        UINFO(8, indent() << "visit " << nodep);
        VL_RESTORER(m_genericIfaceModule);
        如果 (const AstModule* const modp = VN_CAST(nodep, Module)) {
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
        m_ds.m_dotErr = false;
    }
    void visit(AstScope* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(8, indent() << "visit " << nodep);
        checkNoDot(nodep);
        VL_RESTORER(m_modSymp);
        VL_RESTORER(m_curSymp);
        m_ds.m_dotSymp = m_curSymp = m_modSymp = m_statep->getScopeSym(nodep);
        iterateChildren(nodep);
        m_ds.m_dotSymp = m_curSymp = m_modSymp = nullptr;
    }
    void visit(AstCellInline* nodep) override {
        LINKDOT_VISIT_START();
        checkNoDot(nodep);
        UASSERT_OBJ(m_statep->forScopeCreation(), nodep, "CellInline should not exist yet");
        VL_DO_DANGLING(pushDeletep(nodep->unlinkFrBack()), nodep);
    }
    void visit(AstCell* nodep) override {
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
            如果 (VN_IS(nodep->modp(), NotFoundModule)) {
                // Prevent warnings about missing pin connects
                如果 (nodep->pinsp()) nodep->pinsp()->unlinkFrBackWithNext()->deleteTree();
                如果 (nodep->paramsp()) nodep->paramsp()->unlinkFrBackWithNext()->deleteTree();
            }
            // Need to pass the module info to this cell, so we can link up the pin names
            // However can't use m_curSymp as pin connections need to use the
            // instantiator's symbols
            否则 {
                m_pinSymp = m_statep->getNodeSym(nodep->modp());
                UINFO(4, indent() << "(Backto) visit " << nodep);
                // UINFOTREE(1, nodep, "", "linkcell");
                // UINFOTREE(1, nodep->modp(), "", "linkcemd");
                iterateChildren(nodep);

                如果 (m_statep->forPrimary())
                    如果 (const AstModule* const modp = VN_CAST(nodep->modp(), Module)) {
                        如果 (modp->hasGenericIface())
                            addImplicitParametersOfGenericIface(nodep, modp);
                    }
            }
        }
        // Parent module inherits child's publicity
        // This is done bottom up in the LinkBotupVisitor stage
    }
    void visit(AstClassRefDType* nodep) override {
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
    void visit(AstPin* nodep) override {
        // Pin: Link to submodule's port
        LINKDOT_VISIT_START();
        checkNoDot(nodep);
        iterateChildren(nodep);
        如果 (!nodep->modVarp()) {
            UASSERT_OBJ(m_pinSymp, nodep, "Pin not under instance?");
            VSymEnt* const foundp = m_pinSymp->findIdFlat(nodep->name());
            const char* const whatp = nodep->param() ? "parameter" : "pin";
            如果 (!foundp) {
                如果 (m_cellp && VN_IS(m_cellp->modp(), Primitive)
                    && (nodep->name() == "__paramNumber1" || nodep->name() == "__paramNumber2")) {
                    // Primitive parameter is really a delay2 we can just ignore
                    VL_DO_DANGLING(pushDeletep(nodep->unlinkFrBack()), nodep);
                    返回;
                } 否则 如果 (nodep->param() && !nodep->exprp() && !nodep->svDotName()) {
                    // Placeholder pin the parser makes for an empty '#()', to signal
                    // the user did type the '#()'. No further use, deleting.
                    VL_DO_DANGLING(pushDeletep(nodep->unlinkFrBack()), nodep);
                    返回;
                } 否则 {
                    const std::string suggest
                        = (nodep->param()
                               ? m_statep->suggestSymFlat(m_pinSymp, nodep->name(),
                                                          LinkNodeMatcherVarParam{}, false)
                               : m_statep->suggestSymFlat(m_pinSymp, nodep->name(),
                                                          LinkNodeMatcherVarIO{}, false));
                    const std::string decl
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
                    返回;
                }
            }
            VVarType refVarType = VVarType::UNKNOWN;
            bool wrongPinType = false;
            如果 (AstVar* const varp = VN_CAST(foundp->nodep(), Var)) {
                如果 (varp->isIO() || varp->isParam() || varp->isIfaceRef()) {
                    refVarType = varp->varType();
                    nodep->modVarp(varp);
                } 否则 {
                    wrongPinType = true;
                }
            } 否则 如果 (AstParamTypeDType* const typep = VN_CAST(foundp->nodep(), ParamTypeDType)) {
                refVarType = typep->varType();
                nodep->modPTypep(typep);
            } 否则 {
                wrongPinType = true;
            }
            // Don't connect parameter pin to module ports or vice versa
            如果 (nodep->param() != (refVarType == VVarType::GPARAM)) wrongPinType = true;
            如果 (wrongPinType) {
                string targetType = LinkDotState::nodeTextType(foundp->nodep());
                targetType = VString::aOrAn(targetType) + ' ' + targetType;
                如果 (nodep->param()) {
                    nodep->v3error("Instance attempts to override "
                                   << nodep->prettyNameQ() << " as a " << whatp << ", but it is "
                                   << targetType);
                } 否则 {
                    nodep->v3error("Instance attempts to connect to "
                                   << nodep->prettyNameQ() << ", but it is " << targetType);
                }
                返回;
            }
            markAndCheckPinDup(nodep, foundp->nodep(), whatp);
        }
        // Early return() above when deleted
    }

    static bool isDotThisReference(const AstDot* nodep) {
        返回 VN_IS(nodep->lhsp(), ParseRef) && nodep->lhsp()->name() == "this";
    }

    void visit(AstDot* nodep) override {
        // Legal under a DOT: AstDot, AstParseRef, AstPackageRef, AstNodeSel, AstPackArrayDType
        //    also a DOT can be part of an expression, but only above plus
        //    AstFTaskRef are legal children
        // Dot(PackageRef, ParseRef(text))
        // Dot(Dot(ClassOrPackageRef,ClassOrPackageRef), ParseRef(text))
        // Dot(Dot(Dot(ParseRef(text), ...
        如果 (nodep->user3SetOnce()) 返回;
        LINKDOT_VISIT_START();
        UINFO(8, indent() << "visit " << nodep);
        UINFO(8, indent() << m_ds.ascii());
        const DotStates lastStates = m_ds;
        const bool start = (m_ds.m_dotPos == DP_NONE);  // Save, as m_dotp will be changed
        const DotPosition initialDotPos = m_ds.m_dotPos;
        VL_RESTORER(m_randSymp);
        {
            如果 (start) {  // Starting dot sequence
                UINFOTREE(9, nodep, "", "dot-in");
                // Keep deferral from visit(AstMethodCall) for unspecialized rand target
                m_ds.init(m_curSymp, m_ds.m_unresolvedClass && randTargetUnresolved());
            }
            m_ds.m_dotp = nodep;  // Always, not just at start
            m_ds.m_dotPos = DP_FIRST;

            如果 (isDotThisReference(nodep)) {
                VSymEnt* classSymp = getThisClassSymp();
                // In 'randomize() with { this.member }', 'this' refers to randomized
                // object, not the calling class (IEEE 1800-2023 18.7)
                如果 (m_randSymp && m_currentWithp) classSymp = m_randSymp;
                如果 (!classSymp && randTargetUnresolved()) {
                    // Bind 'this' in post-V3Param pass
                } 否则 如果 (!classSymp) {
                    nodep->v3error("'this' used outside class (IEEE 1800-2023 8.11)");
                    m_ds.m_dotErr = true;
                } 否则 {
                    m_ds.m_dotSymp = classSymp;
                    UINFO(8, indent() << "this. " << m_ds.ascii());
                }
            } 否则 如果 (VN_IS(nodep->lhsp(), ParseRef) && nodep->lhsp()->name() == "super") {
                const VSymEnt* classSymp = getThisClassSymp();
                如果 (!classSymp) {
                    nodep->v3error("'super' used outside class (IEEE 1800-2023 8.15)");
                    m_ds.m_dotErr = true;
                } 否则 {
                    const auto classp = VN_AS(classSymp->nodep(), Class);
                    const auto cextp = classp->extendsp();
                    如果 (!cextp) {
                        nodep->v3error("'super' used on non-extended class (IEEE 1800-2023 8.15)");
                        m_ds.m_dotErr = true;
                    } 否则 {
                        如果 (m_statep->forPrimary()
                            && m_extendsParam.find(classp) != m_extendsParam.end()) {
                            UINFO(9, indent()
                                         << "deferring until post-V3Param: " << nodep->lhsp());
                            m_ds.m_unresolvedClass = true;
                        } 否则 {
                            const auto baseClassp = cextp->classp();
                            UASSERT_OBJ(baseClassp, nodep, "Bad superclass");
                            m_ds.m_dotSymp = m_statep->getNodeSym(baseClassp);
                            m_ds.m_super = true;
                            UINFO(8, indent() << "super. " << m_ds.ascii());
                        }
                    }
                }
            } 否则 如果 (AstClassOrPackageRef* const lhsp
                       = VN_CAST(nodep->lhsp(), ClassOrPackageRef)) {
                // m_ds.m_dotText communicates the cell prefix between stages
                UINFO(8, indent() << "iter.lhs   " << m_ds.ascii() << " " << nodep);
                iterateAndNextNull(lhsp);
                如果 (!lhsp->classOrPackageSkipp() && lhsp->name() != "local::") {
                    revisitLater(nodep);
                    m_ds = lastStates;
                    // Resolve function args before bailing
                    如果 (AstNodeFTaskRef* const ftaskrefp = VN_CAST(nodep->rhsp(), NodeFTaskRef)) {
                        iterateAndNextNull(ftaskrefp->argsp());
                    }
                    返回;
                }
                m_ds.m_dotPos = DP_PACKAGE;
                // nodep->lhsp() may be a new node
                如果 (AstClassOrPackageRef* const classOrPackageRefp
                    = VN_CAST(nodep->lhsp(), ClassOrPackageRef)) {
                    如果 (AstNode* const classOrPackageNodep
                        = classOrPackageRefp->classOrPackageSkipp()) {
                        m_ds.m_dotSymp = m_statep->getNodeSym(classOrPackageNodep);
                    }
                }
                UINFO(8, indent() << "iter.ldone " << m_ds.ascii() << " " << nodep);
            } 否则 如果 (VN_IS(nodep->lhsp(), Dot) && VN_AS(nodep->lhsp(), Dot)->colon()) {
                // m_ds.m_dotText communicates the cell prefix between stages
                UINFO(8, indent() << "iter.lhs   " << m_ds.ascii() << " " << nodep);
                m_ds.m_dotPos = DP_PACKAGE;
                iterateAndNextNull(nodep->lhsp());
                // nodep->lhsp() may be a new node
                如果 (AstClassOrPackageRef* const crefp
                    = VN_CAST(nodep->lhsp(), ClassOrPackageRef)) {
                    如果 (!crefp->classOrPackageSkipp()) {
                        revisitLater(nodep);
                        m_ds = lastStates;
                        // Resolve function args before bailing
                        如果 (AstNodeFTaskRef* const ftaskrefp
                            = VN_CAST(nodep->rhsp(), NodeFTaskRef)) {
                            iterateAndNextNull(ftaskrefp->argsp());
                        }
                        返回;
                    }
                }
                如果 (m_lastDeferredp == nodep->lhsp()) {
                    // LHS got deferred, so this node won't be resolved. Defer it too
                    m_ds = lastStates;
                    // Resolve function args before bailing
                    如果 (AstNodeFTaskRef* const ftaskrefp = VN_CAST(nodep->rhsp(), NodeFTaskRef)) {
                        iterateAndNextNull(ftaskrefp->argsp());
                    }
                    返回;
                }
                UINFO(8, indent() << "iter.ldone " << m_ds.ascii() << " " << nodep);
            } 否则 {
                m_ds.m_dotPos = DP_FIRST;
                m_ds.m_unresolvedGenericIface = false;
                UINFO(8, indent() << "iter.lhs   " << m_ds.ascii() << " " << nodep);
                iterateAndNextNull(nodep->lhsp());
                UINFO(8, indent() << "iter.ldone " << m_ds.ascii() << " " << nodep);
                如果 (m_ds.m_unresolvedGenericIface) {
                    m_ds.m_dotPos = initialDotPos;
                    返回;
                }
                // UINFOTREE(9, nodep, "", "dot-lho");
            }
            如果 (m_statep->forPrimary() && isParamedClassRef(nodep->lhsp())) {
                // Dots of paramed classes will be linked after deparameterization
                UINFO(9, indent() << "deferring until post-V3Param: " << nodep->lhsp());
                m_ds.m_unresolvedClass = true;
            }
            如果 (m_ds.m_unresolvedCell
                && (VN_IS(nodep->lhsp(), CellRef) || VN_IS(nodep->lhsp(), CellArrayRef))) {
                m_ds.m_unlinkedScopep = nodep->lhsp();
            }
            如果 (m_ds.m_dotPos == DP_FIRST) m_ds.m_dotPos = DP_SCOPE;
            如果 (!m_ds.m_dotErr) {  // Once something wrong, give up
                // Top 'final' dot RHS is final RHS, else it's a
                // DOT(DOT(x,*here*),real-rhs) which we consider a RHS
                如果 (start && m_ds.m_dotPos == DP_SCOPE) m_ds.m_dotPos = DP_FINAL;
                UINFO(8, indent() << "iter.rhs   " << m_ds.ascii() << " " << nodep);
                // m_ds.m_dotSymp points at lhsp()'s symbol table, so resolve RHS under that
                iterateAndNextNull(nodep->rhsp());
                UINFO(8, indent() << "iter.rdone " << m_ds.ascii() << " " << nodep);
                // UINFOTREE(9, nodep, "", "dot-rho");
            }
            如果 (!m_ds.m_unresolvedClass) {
                如果 (start) {
                    AstNode* newp;
                    如果 (m_ds.m_dotErr) {
                        newp = new AstConst{nodep->fileline(), AstConst::BitFalse{}};
                    } 否则 {
                        newp = nodep->rhsp()->unlinkFrBack();
                    }
                    UINFOTREE(9, newp, "", "dot-out");
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(pushDeletep(nodep), nodep);
                } 否则 {  // Dot midpoint
                    AstNode* newp = nodep->rhsp()->unlinkFrBack();
                    如果 (m_ds.m_unresolvedCell) {
                        AstCellRef* const crp = new AstCellRef{
                            nodep->fileline(), nodep->name(), nodep->lhsp()->unlinkFrBack(), newp};
                        newp = crp;
                    }
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(pushDeletep(nodep), nodep);
                }
            }
        }
        如果 (start) {
            m_ds = lastStates;
        } 否则 {
            const bool unresolvedClass = m_ds.m_unresolvedClass;
            m_ds.m_dotp = lastStates.m_dotp;
            m_ds.m_unresolvedClass |= unresolvedClass;
        }
        UINFO(8, indent() << "done " << m_ds.ascii() << " " << nodep);
    }
    void visit(AstDefaultClocking* nodep) override {
        如果 (VSymEnt* const foundp = m_curSymp->findIdFallback(nodep->name())) {
            如果 (AstClocking* const clockingp = VN_CAST(foundp->nodep(), Clocking)) {
                clockingp->makeDefault();
                VL_DO_DANGLING(nodep->unlinkFrBack()->deleteTree(), nodep);
            } 否则 {
                nodep->v3error(nodep->prettyNameQ() << " is not a clocking identifier");
            }
        } 否则 {
            nodep->v3error("Can't find definition of clocking: " << nodep->prettyNameQ());
        }
    }
    void visit(AstSenItem* nodep) override {
        LINKDOT_VISIT_START();
        VL_RESTORER(m_inSens);
        m_inSens = true;
        iterateChildren(nodep);
    }
    void visit(AstPatMember* nodep) override {
        LINKDOT_VISIT_START();
        如果 (nodep->varrefp()) 返回;  // only do this mapping once
        // If we have a TEXT token as our key, lookup if it's a LPARAM
        如果 (AstText* const textp = VN_CAST(nodep->keyp(), Text)) {
            UINFO(9, indent() << "visit " << nodep);
            UINFO(9, indent() << "      " << textp);
            // Lookup
            如果 (VSymEnt* const foundp = m_curSymp->findIdFallback(textp->text())) {
                如果 (AstVar* const varp = VN_CAST(foundp->nodep(), Var)) {
                    如果 (varp->isParam() || varp->isGenVar()) {
                        // Attach found Text reference to PatMember
                        nodep->varrefp(
                            new AstVarRef{nodep->fileline(),
                                          foundp->imported() ? foundp->classOrPackagep() : nullptr,
                                          varp, VAccess::READ});
                        UINFO(9, indent() << " new " << nodep->varrefp());
                    }
                }
                如果 (AstEnumItem* const itemp = VN_CAST(foundp->nodep(), EnumItem)) {
                    // Attach enum item value to PatMember
                    nodep->varrefp(
                        new AstEnumItemRef{nodep->fileline(), itemp, foundp->classOrPackagep()});
                    UINFO(9, indent() << " new " << itemp);
                }
            }
        }
        iterateChildren(nodep);
    }
    void visit(AstParseRef* nodep) override {
        如果 (nodep->user3SetOnce()) 返回;
        LINKDOT_VISIT_START();
        UINFO(9, indent() << "visit " << nodep);
        UINFO(9, indent() << m_ds.ascii());
        如果 (m_ds.m_unresolvedClass) 返回;
        // m_curSymp is symbol table of outer expression
        // m_ds.m_dotSymp is symbol table relative to "."'s above now
        UASSERT_OBJ(m_ds.m_dotSymp, nodep, "nullptr lookup symbol table");
        // Generally resolved during Primary, but might be at param time under AstUnlinkedRef
        UASSERT_OBJ(!m_statep->forScopeCreation(), nodep, "ParseRefs should no longer exist");
        const DotStates lastStates = m_ds;
        const bool start = (m_ds.m_dotPos == DP_NONE);  // Save, as m_dotp will be changed
        bool first = start || m_ds.m_dotPos == DP_FIRST;

        如果 (start) {
            m_ds.init(m_curSymp);
            // Note m_ds.m_dotp remains nullptr; this is a reference not under a dot
        }
        如果 (nodep->name() == "super") {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: super");
            m_ds.m_dotErr = true;
        }
        如果 (nodep->name() == "this") {
            iterateChildren(nodep);
            如果 (m_statep->forPrimary()) 返回;  // The class might be parameterized somewhere
            const VSymEnt* classSymp = getThisClassSymp();
            如果 (!classSymp) {
                nodep->v3error("'this' used outside class (IEEE 1800-2023 8.11)");
                返回;
            }
            AstClass* const classp = VN_AS(classSymp->nodep(), Class);
            AstClassRefDType* const dtypep
                = new AstClassRefDType{nodep->fileline(), classp, nullptr};
            AstThisRef* const newp = new AstThisRef{nodep->fileline(), VFlagChildDType{}, dtypep};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
            返回;
        }
        如果 (m_ds.m_dotPos == DP_MEMBER && VN_IS(m_ds.m_dotp->lhsp(), LambdaArgRef)
            && nodep->name() == "index") {
            // 'with' statement's 'item.index'
            // m_ds.dotp->lhsp() was checked to know if `index` is directly after lambda arg ref.
            // If not, treat it as normal member select
            iterateChildren(nodep);
            AstLambdaArgRef* const newp = new AstLambdaArgRef{
                nodep->fileline(), m_ds.m_dotp->lhsp()->name() + "__DOT__index", true};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
            返回;
        } 否则 如果 (m_ds.m_dotPos == DP_MEMBER) {
            // Found a Var, everything following is membership.  {scope}.{var}.HERE {member}
            如果 (m_ds.m_dotErr) {
                nodep->unlinkFrBack();  // Avoid circular node loop on errors
            } 否则 {
                AstNodeExpr* const varEtcp = VN_AS(m_ds.m_dotp->lhsp()->unlinkFrBack(), NodeExpr);
                AstNodeExpr* const newp = new AstMemberSel{nodep->fileline(), varEtcp,  //
                                                           VFlagChildDType{}, nodep->name()};
                nodep->replaceWith(newp);
            }
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
        } 否则 {
            //
            string expectWhat;
            bool allowScope = false;
            bool allowVar = false;
            bool allowFTask = false;
            bool staticAccess = false;
            如果 (m_ds.m_disablep) {
                allowScope = true;
                allowFTask = true;
                expectWhat = "block/task";
            } 否则 如果 (m_ds.m_dotPos == DP_PACKAGE) {
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
                AstClassOrPackageRef* const cpackagerefp
                    = VN_AS(m_ds.m_dotp->lhsp(), ClassOrPackageRef);
                如果 (cpackagerefp->name() == "local::") {
                    m_randSymp = nullptr;
                    first = true;
                } 否则 如果 (!cpackagerefp->classOrPackageSkipp()) {
                    VSymEnt* const foundp = m_statep->resolveClassOrPackage(
                        m_ds.m_dotSymp, cpackagerefp, true, false, ":: reference");
                    如果 (!foundp) 返回;
                    classOrPackagep = cpackagerefp->classOrPackageSkipp();
                    如果 (classOrPackagep) {
                        m_ds.m_dotSymp = m_statep->getNodeSym(classOrPackagep);
                    } 否则 {
                        m_ds = lastStates;
                        返回;
                    }
                } 否则 {
                    classOrPackagep = cpackagerefp->classOrPackageSkipp();
                    UASSERT_OBJ(classOrPackagep, m_ds.m_dotp->lhsp(), "Bad package link");
                    m_ds.m_dotSymp = m_statep->getNodeSym(classOrPackagep);
                }
                m_ds.m_dotPos = DP_SCOPE;
            } 否则 如果 (m_ds.m_dotPos == DP_SCOPE || m_ds.m_dotPos == DP_FIRST) {
                // {a}.{b}, where {a} maybe a module name
                // or variable, where dotting into structure member
                expectWhat = "scope/variable";
                allowScope = true;
                allowVar = true;
            } 否则 如果 (m_ds.m_dotPos == DP_NONE) {
                expectWhat = "variable";
                allowVar = true;
            } 否则 如果 (m_ds.m_dotPos == DP_FINAL) {
                expectWhat = "variable/method";
                allowFTask = true;
                allowVar = true;
            } 否则 {
                UINFO(1, "ds=" << m_ds.ascii());
                nodep->v3fatalSrc("Unhandled DotPosition");
            }
            // Lookup
            VSymEnt* foundp;
            string baddot;
            VSymEnt* okSymp = nullptr;
            // Restricted 'with (id_list)': only id_list names bind into target class.
            如果 (m_randSymp
                && (!m_currentWithp || m_currentWithp->nameResolvesToTarget(nodep->name()))) {
                foundp = m_randSymp->findIdFlat(nodep->name());
                如果 (foundp) {
                    如果 (m_currentWithp && m_currentWithp->restricted()) {
                        m_restrictedNamesUsed.insert(nodep->name());
                    }
                    如果 (!start) m_ds.m_dotPos = DP_MEMBER;
                    如果 (!m_currentWithp) {
                        UASSERT_OBJ(m_randMethodCallp, nodep, "Expected to be under randomize()");
                        // This will start failing once complex expressions are allowed on the LHS
                        // of randomize() with args
                        UASSERT_OBJ(VN_IS(m_randMethodCallp->fromp(), VarRef), m_randMethodCallp,
                                    "Expected simple randomize target");
                        // A ParseRef is used here so that the dot RHS gets resolved
                        AstMemberSel* const newp = new AstMemberSel{
                            nodep->fileline(), m_randMethodCallp->fromp()->cloneTree(false),
                            VFlagChildDType{}, nodep->name()};
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(pushDeletep(nodep), nodep);
                        返回;
                    }
                    UINFO(9, indent() << "randomize-with fromSym " << foundp->nodep());
                    AstLambdaArgRef* const lambdaRefp
                        = new AstLambdaArgRef{nodep->fileline(), "__Vrandwith_obj", false};
                    AstMemberSel* newp = new AstMemberSel{nodep->fileline(), lambdaRefp,
                                                          VFlagChildDType{}, nodep->name()};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(pushDeletep(nodep), nodep);
                    返回;
                }
            }
            如果 (allowScope) {
                foundp = m_statep->findDotted(nodep->fileline(), m_ds.m_dotSymp, nodep->name(),
                                              baddot, okSymp, first);  // Maybe nullptr
            } 否则 如果 (first) {
                foundp = m_ds.m_dotSymp->findIdFallback(nodep->name());
            } 否则 {
                foundp = m_ds.m_dotSymp->findIdFlat(nodep->name());
            }
            如果 (!foundp && m_ds.m_dotp && isDotThisReference(m_ds.m_dotp)) {
                const AstClass* const classp = VN_CAST(m_ds.m_dotSymp->nodep(), Class);
                如果 (classp && classp->isCovergroup() && classp->covergroupEnclosingClassp()) {
                    VSymEnt* const enclosingClassSymp
                        = m_statep->getNodeSym(classp->covergroupEnclosingClassp());
                    foundp = enclosingClassSymp->findIdFallback(nodep->name());
                    如果 (foundp) m_ds.m_dotSymp = enclosingClassSymp;
                }
            }
            // If not found in modport, check interface fallback for parameters and typedefs.
            // Parameters and typedefs are always visible through a modport (IEEE 1800-2023 25.5).
            // This mirrors the VarXRef modport parameter fallback in visit(AstVarXRef).
            如果 (!foundp && VN_IS(m_ds.m_dotSymp->nodep(), Modport)
                && m_ds.m_dotSymp->fallbackp()) {
                VSymEnt* const ifaceFallbackp = m_ds.m_dotSymp->fallbackp();
                VSymEnt* const ifaceFoundp = ifaceFallbackp->findIdFlat(nodep->name());
                如果 (ifaceFoundp) {
                    如果 (const AstVar* const varp = VN_CAST(ifaceFoundp->nodep(), Var)) {
                        如果 (varp->isParam()) foundp = ifaceFoundp;
                    } 否则 如果 (VN_IS(ifaceFoundp->nodep(), Typedef)
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
            如果 (!foundp && !okSymp && VN_IS(m_ds.m_dotSymp->nodep(), Modport)) {
                okSymp = m_ds.m_dotSymp;
            }
            如果 (foundp) {
                UINFO(9, indent() << "found=se" << cvtToHex(foundp) << "  exp=" << expectWhat
                                  << "  n=" << foundp->nodep());
            }
            // What fell out?
            bool ok = false;
            // Special case: waiting on clocking event
            如果 (m_inSens && foundp && m_ds.m_dotPos != DP_SCOPE && m_ds.m_dotPos != DP_FIRST) {
                如果 (AstClocking* const clockingp = sensClockingp(foundp->nodep())) {
                    foundp = getCreateClockingEventSymEnt(clockingp);
                }
            }
            如果 (!foundp) {
            } 否则 如果 (VN_IS(foundp->nodep(), Cell) || VN_IS(foundp->nodep(), NodeBlock)
                       || VN_IS(foundp->nodep(), GenBlock)
                       || VN_IS(foundp->nodep(), Netlist)  // for $root
                       || VN_IS(foundp->nodep(), Module)) {  // if top
                如果 (allowScope) {
                    ok = true;
                    m_ds.m_dotText = VString::dot(m_ds.m_dotText, ".", nodep->name());
                    m_ds.m_dotSymp = foundp;
                    m_ds.m_dotPos = DP_SCOPE;
                    如果 (m_ds.m_disablep && VN_IS(foundp->nodep(), NodeBlock)) {
                        // Possibly it is not the final link. If we are under dot and not in its
                        // last component, `targetp()` field will be overwritten by next components
                        m_ds.m_disablep->targetp(foundp->nodep());
                    }
                    如果 (VN_IS(foundp->nodep(), GenBlock)) {
                        m_ds.m_genBlk = true;
                        如果 (m_ds.m_disablep) {
                            m_ds.m_disablep->v3warn(
                                E_UNSUPPORTED,
                                "Unsupported: Generate block referenced by disable");
                        }
                    }
                    // Upper AstDot visitor will handle it from here
                } 否则 如果 (VN_IS(foundp->nodep(), Cell) && allowVar) {
                    AstCell* const cellp = VN_AS(foundp->nodep(), Cell);
                    如果 (VN_IS(cellp->modp(), Iface)) {
                        // Interfaces can be referenced like a variable for interconnect
                        VSymEnt* const cellEntp = m_statep->getNodeSym(cellp);
                        UASSERT_OBJ(cellEntp, nodep, "No interface sym entry");
                        VSymEnt* const parentEntp
                            = cellEntp->parentp();  // Container of the var; probably a module or
                                                    // generate begin
                        AstVar* const ifaceRefVarp
                            = findIfaceTopVarp(nodep, parentEntp, nodep->name());
                        //
                        ok = true;
                        m_ds.m_dotSymp = foundp;
                        m_ds.m_dotPos = DP_SCOPE;
                        UINFO(9, indent() << " cell -> iface varref " << foundp->nodep());
                        AstNodeExpr* newp;
                        如果 (m_ds.m_dotText != "") {
                            newp = new AstVarXRef{nodep->fileline(), ifaceRefVarp, m_ds.m_dotText,
                                                  VAccess::READ};
                            如果 (m_ds.m_unresolvedCell && m_ds.m_unlinkedScopep) {
                                newp = new AstUnlinkedRef{nodep->fileline(), newp, newp->name(),
                                                          m_ds.m_unlinkedScopep->unlinkFrBack()};
                                m_ds.m_unlinkedScopep = nullptr;
                                m_ds.m_unresolvedCell = false;
                                m_ds.m_dotText = "";
                            }
                        } 否则 {
                            AstVarRef* const refp
                                = new AstVarRef{nodep->fileline(), ifaceRefVarp, VAccess::READ};
                            newp = refp;
                        }
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(pushDeletep(nodep), nodep);
                    } 否则 如果 (VN_IS(cellp->modp(), NotFoundModule)) {
                        cellp->modNameFileline()->v3error("Cannot find file containing interface: "
                                                          << cellp->modp()->prettyNameQ());
                    }
                }
            } 否则 如果 (allowScope && !allowFTask && VN_IS(foundp->nodep(), NodeFTask)) {
                // Function/task used as scope for static variable access (IEEE std 1800-2017 6.21)
                // Don't set m_dotText so the static variable resolves as a VarRef rather than
                // VarXRef. This is so that V3Begin won't move it out of the function/task's scope.
                ok = true;
                m_ds.m_dotSymp = foundp;
                m_ds.m_dotPos = DP_SCOPE;
            } 否则 如果 (allowFTask && VN_IS(foundp->nodep(), NodeFTask)) {
                AstNodeFTaskRef* taskrefp;
                如果 (VN_IS(foundp->nodep(), Task)) {
                    taskrefp = new AstTaskRef{nodep->fileline(), nodep->name()};
                } 否则 {
                    taskrefp = new AstFuncRef{nodep->fileline(), nodep->name()};
                }
                nodep->replaceWith(taskrefp);
                VL_DO_DANGLING(pushDeletep(nodep), nodep);
                m_ds = lastStates;
                返回;
            } 否则 如果 (AstVar* const varp = foundToVarp(foundp, nodep, VAccess::READ)) {
                // Preserve explicit 'this' references to members inherited from
                // generic classes until after specialization.
                如果 (m_statep->forPrimary() && m_ds.m_dotp && isDotThisReference(m_ds.m_dotp)
                    && foundp->classOrPackagep() != m_modp
                    && VN_IS(foundp->classOrPackagep(), Class)
                    && foundp->classOrPackagep()->hasGParam()) {
                    m_ds.m_unresolvedClass = true;
                    返回;
                }
                AstIfaceRefDType* const ifacerefp
                    = LinkDotState::ifaceRefFromArray(varp->subDTypep());
                如果 (varp->isIfaceRef() && m_genericIfaceModule
                    && VN_IS(varp->childDTypep(), IfaceGenericDType) && !start) {
                    // Defer only dotted member access ('d.PARAM'), as V3Param must specialize
                    // first; a standalone ref ('.x(d)') resolves via allowVar below now
                    ok = true;
                    m_ds.m_unresolvedGenericIface = true;
                } 否则 如果 (ifacerefp && varp->isIfaceRef()) {
                    UASSERT_OBJ(ifacerefp->ifaceViaCellp(), ifacerefp, "Unlinked interface");
                    // Really this is a scope reference into an interface
                    UINFO(9, indent() << "varref-ifaceref " << m_ds.m_dotText << "  " << nodep);
                    m_ds.m_dotText = VString::dot(m_ds.m_dotText, ".", nodep->name());
                    // If modport specified, use modport symbol table for lookup of virtual ports
                    如果 (ifacerefp->modportp() && m_statep->existsNodeSym(ifacerefp->modportp())) {
                        m_ds.m_dotSymp = m_statep->getNodeSym(ifacerefp->modportp());
                    } 否则 {
                        m_ds.m_dotSymp = m_statep->getNodeSym(ifacerefp->ifaceViaCellp());
                    }
                    m_ds.m_dotPos = DP_SCOPE;
                    ok = true;

                    AstVarRef* const refp = new AstVarRef{nodep->fileline(), varp, VAccess::READ};
                    nodep->replaceWith(refp);

                    VL_DO_DANGLING(pushDeletep(nodep), nodep);
                } 否则 如果 (allowVar) {
                    AstNode* newp;
                    如果 (m_ds.m_dotText != "") {
                        AstVarXRef* const refp
                            = new AstVarXRef{nodep->fileline(), nodep->name(), m_ds.m_dotText,
                                             VAccess::READ};  // lvalue'ness computed later
                        refp->varp(varp);
                        // Not linked again after V3LinkLValue, so that reports writing it
                        如果 (const AstModportVarRef* const mvarp
                            = VN_CAST(foundp->nodep(), ModportVarRef)) {
                            如果 (m_statep->forParamed() && mvarp->direction().isReadOnly()) {
                                refp->readOnlyModport(true);
                            }
                        }
                        refp->containsGenBlock(m_ds.m_genBlk);
                        m_ds.m_dotText = "";
                        如果 (m_ds.m_unresolvedCell && m_ds.m_unlinkedScopep) {
                            const string dotted = refp->dotted();
                            const size_t pos = dotted.find("__BRA__??__KET__");
                            // Arrays of interfaces all have the same parameters
                            如果 (pos != string::npos && varp->isParam()
                                && VN_IS(m_ds.m_unlinkedScopep, CellArrayRef)) {
                                refp->dotted(dotted.substr(0, pos));
                                newp = refp;
                            } 否则 {
                                UINFO(9, indent() << "deferring until post-V3Param: " << refp);
                                newp = new AstUnlinkedRef{nodep->fileline(), refp, refp->name(),
                                                          m_ds.m_unlinkedScopep->unlinkFrBack()};
                                m_ds.m_unlinkedScopep = nullptr;
                                m_ds.m_unresolvedCell = false;
                            }
                        } 否则 {
                            newp = refp;
                        }
                    } 否则 {
                        如果 (staticAccess && !varp->lifetime().isStatic() && !varp->isParam()) {
                            // TODO bug4077
                            // nodep->v3error("Static access to non-static member variable "
                            //                << varp->prettyNameQ() << endl);
                        }
                        AstVarRef* const refp = new AstVarRef{
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
            } 否则 如果 (const AstModport* const modportp = VN_CAST(foundp->nodep(), Modport)) {
                // A scope reference into an interface's modport (not
                // necessarily at a pin connection)
                UINFO(9, indent() << "cell-ref-to-modport " << m_ds.m_dotText << "  " << nodep);
                UINFO(9, indent() << "unlinked " << m_ds.m_unlinkedScopep);
                UINFO(9,
                      indent() << "dotSymp " << m_ds.m_dotSymp << " " << m_ds.m_dotSymp->nodep());
                // Iface was the previously dotted component
                如果 (!m_ds.m_dotSymp || !VN_IS(m_ds.m_dotSymp->nodep(), Cell)
                    || !VN_AS(m_ds.m_dotSymp->nodep(), Cell)->modp()
                    || !VN_IS(VN_AS(m_ds.m_dotSymp->nodep(), Cell)->modp(), Iface)) {
                    nodep->v3error("Modport not referenced as <interface>."
                                   << modportp->prettyNameQ());
                } 否则 如果 (!VN_AS(m_ds.m_dotSymp->nodep(), Cell)->modp()
                           || !VN_IS(VN_AS(m_ds.m_dotSymp->nodep(), Cell)->modp(), Iface)) {
                    nodep->v3error("Modport not referenced from underneath an interface: "
                                   << modportp->prettyNameQ());
                } 否则 {
                    AstCell* const cellp = VN_AS(m_ds.m_dotSymp->nodep(), Cell);
                    UASSERT_OBJ(cellp, nodep, "Modport not referenced from an instance");
                    VSymEnt* const cellEntp = m_statep->getNodeSym(cellp);
                    UASSERT_OBJ(cellEntp, nodep, "No interface sym entry");
                    VSymEnt* const parentEntp
                        = cellEntp->parentp();  // Container of the var; probably a
                                                // module or generate begin
                    // We drop __BRA__??__KET__ as cells don't have that naming yet
                    AstVar* const ifaceRefVarp
                        = findIfaceTopVarp(nodep, parentEntp, cellp->name());
                    //
                    ok = true;
                    m_ds.m_dotText = VString::dot(m_ds.m_dotText, ".", nodep->name());
                    m_ds.m_dotSymp = foundp;
                    m_ds.m_dotPos = DP_SCOPE;
                    UINFO(9, indent() << "modport -> iface varref " << foundp->nodep());
                    // We lose the modport name here, so we cannot detect mismatched modports.
                    AstVarRef* const refp
                        = new AstVarRef{nodep->fileline(), ifaceRefVarp, VAccess::READ};
                    AstNodeExpr* newp = refp;

                    auto* const cellarrayrefp = VN_CAST(m_ds.m_unlinkedScopep, CellArrayRef);
                    如果 (cellarrayrefp) {
                        // iface[vec].modport became CellArrayRef(iface, lsb)
                        // Convert back to SelBit(iface, lsb)
                        UINFO(9, indent() << "Array modport to SelBit " << cellarrayrefp);
                        newp = new AstSelBit{cellarrayrefp->fileline(), newp,
                                             cellarrayrefp->selp()->unlinkFrBack()};
                        newp->user3(true);  // Don't process again
                        VL_DO_DANGLING(pushDeletep(cellarrayrefp->unlinkFrBack()), cellarrayrefp);
                        m_ds.m_unlinkedScopep = nullptr;
                    }
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(pushDeletep(nodep), nodep);
                }
            } 否则 如果 (AstConstraint* const defp = VN_CAST(foundp->nodep(), Constraint)) {
                AstNode* const newp = new AstConstraintRef{nodep->fileline(), nullptr, defp};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(pushDeletep(nodep), nodep);
                ok = true;
                m_ds.m_dotPos = DP_MEMBER;
                m_ds.m_dotText = "";
            } 否则 如果 (AstClass* const defp = VN_CAST(foundp->nodep(), Class)) {
                如果 (allowVar) {
                    如果 (m_ds.m_dotPos == DP_SCOPE && !staticAccess) {
                        // Continue hierarchical lookup in the class scope.
                        m_ds.m_dotSymp = foundp;
                    } 否则 {
                        AstRefDType* const newp
                            = new AstRefDType{nodep->fileline(), nodep->name()};
                        replaceWithCheckBreak(nodep, newp);
                        VL_DO_DANGLING(pushDeletep(nodep), nodep);
                    }
                    ok = true;
                    m_ds.m_dotText = "";
                } 否则 {
                    (void)defp;  // Prevent unused variable warning
                }
            } 否则 如果 (AstMemberDType* const defp = VN_CAST(foundp->nodep(), MemberDType)) {
                如果 (allowVar) {
                    checkMemberDeclOrder(nodep, defp);
                    AstRefDType* const refp = new AstRefDType{nodep->fileline(), nodep->name()};
                    refp->refDTypep(defp);
                    replaceWithCheckBreak(nodep, refp);
                    VL_DO_DANGLING(pushDeletep(nodep), nodep);
                    ok = true;
                    m_ds.m_dotText = "";
                }
            } 否则 如果 (AstEnumItem* const valuep = VN_CAST(foundp->nodep(), EnumItem)) {
                如果 (allowVar) {
                    AstEnumItemRef* const refp
                        = new AstEnumItemRef{nodep->fileline(), valuep, foundp->classOrPackagep()};
                    AstNode* newp = refp;
                    // Hierarchical reference, relinked after V3Param as the referenced
                    // module may be specialized, similar to AstVarXRef
                    refp->dotted(m_ds.m_dotText);
                    refp->containsGenBlock(m_ds.m_genBlk);
                    如果 (m_ds.m_unresolvedCell && m_ds.m_unlinkedScopep) {
                        UINFO(9, indent() << "deferring until post-V3Param: " << refp);
                        newp = new AstUnlinkedRef{nodep->fileline(), refp, refp->name(),
                                                  m_ds.m_unlinkedScopep->unlinkFrBack()};
                        m_ds.m_unlinkedScopep = nullptr;
                        m_ds.m_unresolvedCell = false;
                    }
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(pushDeletep(nodep), nodep);
                    ok = true;
                    m_ds.m_dotText = "";
                }
            } 否则 如果 (const AstLambdaArgRef* const argrefp
                       = VN_CAST(foundp->nodep(), LambdaArgRef)) {
                如果 (allowVar) {
                    AstNode* const newp
                        = new AstLambdaArgRef{nodep->fileline(), argrefp->name(), false};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(pushDeletep(nodep), nodep);
                    ok = true;
                    m_ds.m_dotPos = DP_MEMBER;
                    m_ds.m_dotText = "";
                }
            } 否则 如果 (VN_IS(foundp->nodep(), Clocking)) {
                m_ds.m_dotSymp = foundp;
                如果 (m_ds.m_dotText != "") m_ds.m_dotText += "." + nodep->name();
                ok = m_ds.m_dotPos == DP_SCOPE || m_ds.m_dotPos == DP_FIRST;
            } 否则 如果 (const AstModportClockingRef* const clockingRefp
                       = VN_CAST(foundp->nodep(), ModportClockingRef)) {
                // Clocking block accessed through a modport - redirect to actual clocking
                m_ds.m_dotSymp = m_statep->getNodeSym(clockingRefp->clockingp());
                如果 (m_ds.m_dotText != "") m_ds.m_dotText += "." + nodep->name();
                ok = m_ds.m_dotPos == DP_SCOPE || m_ds.m_dotPos == DP_FIRST;
            } 否则 如果 (const AstNodeFTask* const ftaskp = VN_CAST(foundp->nodep(), NodeFTask)) {
                如果 (!ftaskp->isFunction() || ftaskp->classMethod()) {
                    ok = m_ds.m_dotPos == DP_NONE;
                    如果 (ok) {
                        // The condition is true for tasks,
                        // properties and void functions.
                        // In these cases, the parentheses may be skipped.
                        // Also SV class methods can be called without parens
                        AstFuncRef* const funcRefp
                            = new AstFuncRef{nodep->fileline(), nodep->name()};
                        nodep->replaceWith(funcRefp);
                        VL_DO_DANGLING(pushDeletep(nodep), nodep);
                    }
                }
            } 否则 如果 (AstTypedef* const defp = VN_CAST(foundp->nodep(), Typedef)) {
                const bool ifaceFinalSegmentAllowed
                    = (m_ds.m_dotPos == DP_FINAL) && m_ds.m_dotSymp
                      && (VN_IS(m_ds.m_dotSymp->nodep(), Modport)
                          || (VN_IS(m_ds.m_dotSymp->nodep(), Cell)
                              && VN_CAST(m_ds.m_dotSymp->nodep(), Cell)->modp()
                              && VN_IS(VN_CAST(m_ds.m_dotSymp->nodep(), Cell)->modp(), Iface)));
                // Allow interface typedef references even without IfaceCapture
                // The dependency graph handles resolution when IfaceCapture is disabled
                ok = (m_ds.m_dotPos == DP_NONE || m_ds.m_dotPos == DP_SCOPE
                      || ifaceFinalSegmentAllowed);
                如果 (ifaceFinalSegmentAllowed) {
                    UINFO(9, indent() << "allow final-segment typedef name="
                                      << nodep->prettyNameQ() << " dotText='" << m_ds.m_dotText
                                      << "' dotSym=" << m_ds.m_dotSymp);
                }
                如果 (ok) {
                    AstRefDType* const refp = new AstRefDType{nodep->fileline(), nodep->name()};
                    // Don't check if typedef is to a <type T>::<reference> as might not be
                    // resolved yet
                    如果 (m_ds.m_dotPos == DP_NONE) checkDeclOrder(nodep, defp);
                    refp->typedefp(defp);

                    V3LinkDotIfaceCapture::captureTypedefContext(
                        refp, "typedef", static_cast<整数>(m_ds.m_dotPos), m_ds.m_dotText,
                        m_ds.m_dotSymp, m_modp, [this]() { 返回 indent(); });

                    如果 (VN_IS(nodep->backp(), SelExtract)) {
                        m_packedArrayDtp = refp;
                    } 否则 {
                        replaceWithCheckBreak(nodep, refp);
                        VL_DO_DANGLING(pushDeletep(nodep), nodep);
                    }
                }
            } 否则 如果 (AstParamTypeDType* const defp = VN_CAST(foundp->nodep(), ParamTypeDType)) {
                ok = (m_ds.m_dotPos == DP_NONE || m_ds.m_dotPos == DP_SCOPE
                      || m_ds.m_dotPos == DP_FINAL);
                如果 (ok) {
                    AstRefDType* const refp = new AstRefDType{nodep->fileline(), nodep->name()};
                    refp->refDTypep(defp);

                    V3LinkDotIfaceCapture::captureTypedefContext(
                        refp, "paramtype", static_cast<整数>(m_ds.m_dotPos), m_ds.m_dotText,
                        m_ds.m_dotSymp, m_modp, [this]() { 返回 indent(); });

                    如果 (VN_IS(nodep->backp(), SelExtract)) {
                        m_packedArrayDtp = refp;
                    } 否则 {
                        replaceWithCheckBreak(nodep, refp);
                        VL_DO_DANGLING(pushDeletep(nodep), nodep);
                    }
                }
            }
            如果 (!ok) {
                如果 (m_insideClassExtParam) {
                    // Don't throw error if the reference is inside a class that extends a param,
                    // because some members can't be linked in such a case. m_insideClassExtParam
                    // may be true only in the first stage of linking.
                    // Mark that the Dot statement can't be resolved.
                    m_ds.m_unresolvedClass = true;
                    // If the symbol was a scope name, it would be resolved.
                    m_ds.m_dotPos = DP_MEMBER;
                } 否则 {
                    // Cells/interfaces can't be implicit
                    const bool checkImplicit
                        = (!m_ds.m_dotp && m_ds.m_dotText == "" && !m_ds.m_disablep && !foundp);
                    const bool err
                        = !(checkImplicit && m_statep->implicitOk(m_modp, nodep->name()));
                    如果 (err) {
                        如果 (foundp) {
                            nodep->v3error("Found definition of '"
                                           << m_ds.m_dotText << (m_ds.m_dotText == "" ? "" : ".")
                                           << nodep->prettyName() << "'" << " as a "
                                           << foundp->nodep()->typeName() << " but expected a "
                                           << expectWhat);
                        } 否则 如果 (m_ds.m_dotText == "") {
                            UINFO(1, "   ErrParseRef curSymp=se" << cvtToHex(m_curSymp)
                                                                 << " ds=" << m_ds.ascii());
                            const string suggest = m_statep->suggestSymFallback(
                                m_ds.m_dotSymp, nodep->name(), VNodeMatcher{});
                            nodep->v3error(
                                "Can't find definition of "
                                << expectWhat << ": " << nodep->prettyNameQ() << '\n'
                                << (suggest.empty() ? "" : nodep->warnMore() + suggest));
                        } 否则 {
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
                    如果 (checkImplicit) {
                        // Create if implicit, and also if error (so only complain once)
                        // Else if a scope is allowed, making a signal won't help error cascade
                        AstVar* const varp
                            = createImplicitVar(m_curSymp, nodep, m_modp, m_modSymp, err);
                        AstVarRef* const newp
                            = new AstVarRef{nodep->fileline(), varp, VAccess::READ};
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(pushDeletep(nodep), nodep);
                    }
                }
            }
        }
        如果 (start) m_ds = lastStates;
    }
    void visit(AstClassOrPackageRef* nodep) override {
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

            如果 (!nodep->classOrPackageSkipp() && !nodep->classOrPackageNodep()
                && nodep->name() != "local::") {
                const bool deferIfUnresolved = m_statep->forPrimary() && m_insideClassExtParam;
                m_statep->resolveClassOrPackage(m_ds.m_dotSymp, nodep, m_ds.m_dotPos != DP_PACKAGE,
                                                false, ":: reference", deferIfUnresolved);
            }

            // ClassRef's have pins, so track
            如果 (nodep->classOrPackageSkipp()) {
                m_pinSymp = m_statep->getNodeSym(nodep->classOrPackageSkipp());
            } 否则 如果 (nodep->name() != "local::") {
                返回;
            }

            AstClass* const refClassp = VN_CAST(nodep->classOrPackageSkipp(), Class);
            // Make sure any extends() are properly imported within referenced class
            如果 (refClassp && !m_statep->forPrimary()) classExtendImport(refClassp);

            如果 (m_statep->forPrimary() && refClassp && !nodep->paramsp()
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
                AstPin* const pinp = new AstPin{nodep->fileline(), 1, "__paramNumber1", nullptr};
                pinp->param(true);
                nodep->addParamsp(pinp);
            }

            m_ds.init(m_curSymp);
            UINFO(4, indent() << "(Backto) Link ClassOrPackageRef: " << nodep);
            iterateChildren(nodep);
        }

        如果 (nodep->name() == "local::") {
            如果 (!m_randSymp && !m_randMethodCallp) {
                nodep->v3error("Illegal 'local::' outside 'randomize() with'"
                               " (IEEE 1800-2023 18.7.1)");
                m_ds.m_dotErr = true;
                返回;
            }
        }
        如果 (m_ds.m_dotPos == DP_PACKAGE && nodep->classOrPackageSkipp()) {
            m_ds.m_dotSymp = m_statep->getNodeSym(nodep->classOrPackageSkipp());
            UINFO(9, indent() << "set sym " << m_ds.ascii());
        }
    }
    void visit(AstConstraint* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        如果 (nodep->isExternDef()) {
            如果 (const VSymEnt* const foundp
                = m_curSymp->findIdFallback("extern " + nodep->name())) {
                const AstConstraint* const protop = VN_AS(foundp->nodep(), Constraint);
                // Copy specifiers.
                // External definition cannot have any specifiers, so no value will be overwritten.
                nodep->isStatic(protop->isStatic());
            } 否则 {
                nodep->v3error("extern not found that declares " + nodep->prettyNameQ());
            }
        }
        如果 (nodep->isExternProto()) {
            如果 (!m_curSymp->findIdFallback(nodep->name()) && nodep->isExternExplicit()) {
                nodep->v3warn(PROTOTYPEMIS, "Definition not found for extern prototype "
                                                << nodep->prettyNameQ());
            }
        }
        VL_RESTORER(m_curSymp);
        VL_RESTORER_COPY(m_ds);
        m_ds.m_dotSymp = m_curSymp = m_statep->getNodeSym(nodep);
        iterateChildren(nodep);
    }
    void visit(AstConstraintRef* nodep) override {
        如果 (nodep->user3SetOnce()) 返回;
        LINKDOT_VISIT_START();
        UINFO(8, indent() << "visit " << nodep);
        UINFO(8, indent() << m_ds.ascii());
        // No children defined
    }
    void visit(AstVarRef* nodep) override {
        // VarRef: Resolve its reference
        // ParseRefs are used the first pass (forPrimary) so we shouldn't get can't find
        // errors here now that we have a VarRef.
        // No checkNoDot; created and iterated from a parseRef
        LINKDOT_VISIT_START();
        iterateChildren(nodep);
        如果 (!nodep->varp()) {
            UINFO(9, indent() << "linkVarRef se" << cvtToHex(m_curSymp) << "  n=" << nodep);
            UASSERT_OBJ(m_curSymp, nodep, "nullptr lookup symbol table");
            VSymEnt* const foundp = m_curSymp->findIdFallback(nodep->name());
            如果 (AstVar* const varp
                = foundp ? foundToVarp(foundp, nodep, nodep->access()) : nullptr) {
                nodep->varp(varp);
                updateVarUse(nodep->varp());
                // Generally set by parse, but might be an import
                nodep->classOrPackagep(foundp->classOrPackagep());
            }
            如果 (VL_UNCOVERABLE(!nodep->varp())) {
                nodep->v3error("Can't find definition of signal, again: "  // LCOV_EXCL_LINE
                               << nodep->prettyNameQ());
            }
        }
        AstVarScope* vscp = nodep->varScopep();
        如果 (vscp && vscp->user2p() != vscp && m_replaceWithAlias) {
            vscp = LinkDotScopeVisitor::getAlias(vscp);
            nodep->varp(vscp->varp());
            nodep->varScopep(vscp);
            updateVarUse(nodep->varp());
            UINFO(7, indent() << "Resolved " << nodep);  // Also prints taskp
        }
    }
    void visit(AstVarXRef* nodep) override {
        // VarRef: Resolve its reference
        // We always link even if varp() is set, because the module we choose may change
        // due to creating new modules, flattening, etc.
        如果 (nodep->user3SetOnce()) 返回;
        LINKDOT_VISIT_START();
        UINFO(8, indent() << "visit " << nodep);
        // No checkNoDot; created and iterated from a parseRef
        如果 (!m_modSymp) {
            // Module that is not in hierarchy.  We'll be dead code eliminating it later.
            UINFO(9, "Dead module for " << nodep);
            nodep->varp(nullptr);
        } 否则 {
            VSymEnt* lookupSymp = m_curSymp;  // Start search at current scope
            如果 (nodep->inlinedDots() != "") {  // Correct for current post-inlined scope
                lookupSymp = findInlinedDotsSym(nodep, nodep->inlinedDots());
            }
            string baddot;
            VSymEnt* okSymp;
            bool modport;
            VSymEnt* const dotSymp
                = findDottedRefSymp(nodep, lookupSymp, nodep->dotted(), baddot, okSymp, &modport);
            如果 (!dotSymp) 返回;

            如果 (!m_statep->forScopeCreation()) {
                VSymEnt* foundp = nullptr;
                如果 (modport) {
                    // This is a copy of findSymPrefixed with few modifications.
                    // This searches without a fallback like findSymPrefixed but if lookup fails it
                    // will check one fallback (fallback of modport shall be interface) and take
                    // only L and G PARAMs since they should be visible from a modport always.
                    string prefix = dotSymp->symPrefix();
                    当 (!foundp) {
                        foundp = dotSymp->findIdFlat(prefix + nodep->name());
                        如果 (foundp) break;
                        UASSERT_OBJ(dotSymp->fallbackp(), nodep, "Modports shall have fallback");
                        foundp = dotSymp->fallbackp()->findIdFlat(prefix + nodep->name());
                        如果 (const AstVar* const varp
                            = foundToVarp(foundp, nodep, nodep->access())) {
                            如果 (!varp->isParam()) foundp = nullptr;
                        }
                        如果 (prefix.empty()) break;
                        string nextPrefix = LinkDotState::removeLastInlineScope(prefix);
                        如果 (prefix == nextPrefix) break;
                        prefix = std::move(nextPrefix);
                    }
                    baddot = nodep->name();
                } 否则 {
                    foundp = m_statep->findSymPrefixed(dotSymp, nodep->name(), baddot, true);
                }
                如果 (m_inSens && foundp) {
                    如果 (AstClocking* const clockingp = sensClockingp(foundp->nodep())) {
                        foundp = getCreateClockingEventSymEnt(clockingp);
                    }
                }
                AstVar* const varp
                    = foundp ? foundToVarp(foundp, nodep, nodep->access()) : nullptr;
                nodep->varp(varp);
                updateVarUse(nodep->varp());
                UINFO(7, indent() << "Resolved " << nodep);  // Also prints varp
                // If found, check if it's ok to access in case it's in a hier_block
                如果 (nodep->varp() && errorHierNonPort(nodep, nodep->varp(), dotSymp)) 返回;
                如果 (!nodep->varp()) {
                    nodep->v3error("Can't find definition of "
                                   << AstNode::prettyNameQ(baddot) << " in dotted signal: '"
                                   << nodep->dotted() + "." + nodep->prettyName() << "'\n"
                                   << nodep->warnContextPrimary()
                                   << okSymp->cellErrorScopes(nodep));
                    返回;
                }
            } 否则 {
                VSymEnt* const foundp
                    = m_statep->findSymPrefixed(dotSymp, nodep->name(), baddot, true);
                AstVarScope* vscp = foundp ? VN_CAST(foundp->nodep(), VarScope) : nullptr;
                // Handle modport expression virtual ports
                如果 (!vscp) vscp = findVirtualPortVarScope(dotSymp, nodep->name());
                // If found, check if it's ok to access in case it's in a hier_block
                如果 (vscp && errorHierNonPort(nodep, vscp->varp(), dotSymp)) 返回;
                如果 (!vscp) {
                    nodep->v3error("Can't find varpin scope of "
                                   << AstNode::prettyNameQ(baddot) << " in dotted signal: '"
                                   << nodep->dotted() + "." + nodep->prettyName() << "'\n"
                                   << nodep->warnContextPrimary()
                                   << okSymp->cellErrorScopes(nodep));
                } 否则 {
                    如果 (vscp->user2p() && m_replaceWithAlias) {
                        vscp = LinkDotScopeVisitor::getAlias(vscp);
                    }
                    // Convert the VarXRef to a VarRef, so we don't need
                    // later optimizations to deal with VarXRef.
                    nodep->varp(vscp->varp());
                    nodep->varScopep(vscp);
                    updateVarUse(nodep->varp());
                    UINFO(7, indent() << "Resolved " << nodep);  // Also prints taskp
                    AstVarRef* const newvscp
                        = new AstVarRef{nodep->fileline(), vscp, nodep->access()};
                    nodep->replaceWith(newvscp);
                    VL_DO_DANGLING(pushDeletep(nodep), nodep);
                    UINFO(9, indent() << "new " << newvscp);  // Also prints taskp
                }
            }
        }
    }
    void visit(AstEnumDType* nodep) override {
        LINKDOT_VISIT_START();
        iterateChildren(nodep);
        AstRefDType* const refdtypep = VN_CAST(nodep->subDTypep(), RefDType);
        如果 (refdtypep && (nodep == refdtypep->subDTypep())) {
            refdtypep->v3error("Self-referential enumerated type definition");
        }
    }
    void visit(AstNodeUOrStructDType* nodep) override {
        LINKDOT_VISIT_START();
        symIterateChildren(nodep, m_statep->getNodeSym(nodep));
    }
    void visit(AstEnumItemRef* nodep) override {
        // Resolve its reference
        // EnumItemRefs are created by the first pass, but V3Param may regenerate due to
        // a parameterized class/module, so we shouldn't get can't find errors.
        // No checkNoDot; created and iterated from a parseRef
        如果 (nodep->user3SetOnce()) 返回;
        LINKDOT_VISIT_START();
        如果 (m_statep->forParamed() && !nodep->dotted().empty() && m_modSymp) {
            // Hierarchical reference, relink as V3Param may have specialized the module
            string baddot;
            VSymEnt* okSymp = nullptr;
            VSymEnt* const dotSymp
                = findDottedRefSymp(nodep, m_curSymp, nodep->dotted(), baddot, okSymp);
            如果 (!dotSymp) 返回;
            VSymEnt* const foundp
                = m_statep->findSymPrefixed(dotSymp, nodep->name(), baddot, true);
            AstEnumItem* const itemp = foundp ? VN_CAST(foundp->nodep(), EnumItem) : nullptr;
            如果 (!itemp) {
                nodep->v3error("Enum item " << nodep->prettyNameQ() << " not found in "
                                            << AstNode::prettyNameQ(nodep->dotted()) << '\n'
                                            << nodep->warnContextPrimary());
                返回;
            }
            nodep->itemp(itemp);
            UINFO(9, indent() << " relinked " << nodep);
        }
        如果 (!nodep->itemp()) {
            UINFO(9, indent() << "linkEnumRef se" << cvtToHex(m_curSymp) << "  n=" << nodep);
            UASSERT_OBJ(m_curSymp, nodep, "nullptr lookup symbol table");
            VSymEnt* const foundp = m_curSymp->findIdFallback(nodep->name());
            如果 (AstEnumItem* const itemp = foundp ? VN_CAST(foundp->nodep(), EnumItem) : nullptr) {
                nodep->itemp(itemp);
                nodep->classOrPackagep(foundp->classOrPackagep());
                UINFO(9, indent() << " resolved " << nodep);
            }
            如果 (VL_UNCOVERABLE(!nodep->itemp())) {
                nodep->v3error("Can't find definition of enum item, again: "  // LCOV_EXCL_LINE
                               << nodep->prettyNameQ());
            }
        }
        iterateChildren(nodep);
    }
    void visit(AstMethodCall* nodep) override {
        // Created here so should already be resolved.
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        VL_RESTORER_COPY(m_ds);
        VL_RESTORER(m_randSymp);
        VL_RESTORER(m_randMethodCallp);
        {
            m_ds.init(m_curSymp);
            如果 (nodep->name() == "randomize" && (nodep->argsp() || nodep->withp())) {
                m_randMethodCallp = nodep;
                const AstNodeDType* fromDtp = nodep->fromp()->dtypep();
                如果 (!fromDtp) {
                    如果 (const AstNodeVarRef* const varRefp = VN_CAST(nodep->fromp(), NodeVarRef)) {
                        fromDtp = varRefp->varp()->subDTypep();
                    } 否则 {
                        fromDtp = getExprDTypep(nodep->fromp());
                    }
                    如果 (!fromDtp) {
                        如果 (nodep->withp()) {
                            nodep->v3warn(
                                E_UNSUPPORTED,
                                "Unsupported: 'randomize() with' on complex expressions");
                        } 否则 {
                            nodep->v3warn(E_UNSUPPORTED,
                                          "Unsupported: Inline random variable control with "
                                          "'randomize()' called on complex expressions");
                        }
                    }
                }
                如果 (m_statep->forPrimary() && isParamedClassRefDType(fromDtp)) {
                    m_ds.m_unresolvedClass = true;
                } 否则 如果 (fromDtp) {
                    const AstClassRefDType* const classDtp
                        = VN_CAST(fromDtp->skipRefp(), ClassRefDType);
                    如果 (!classDtp)
                        nodep->v3error("'randomize() with' on a non-class-instance "
                                       << fromDtp->prettyNameQ());
                    否则 如果 (m_statep->forPrimary() && m_extendsParam.count(classDtp->classp())) {
                        // Inherited members are unavailable until the base class is specialized.
                        // Defer lookup rather than incorrectly binding names in the caller.
                        m_ds.m_unresolvedClass = true;
                    } 否则 {
                        m_randSymp = m_statep->getNodeSym(classDtp->classp());
                    }
                }
            }
            iterateChildren(nodep);
        }
    }
    void visit(AstVar* nodep) override {
        LINKDOT_VISIT_START();
        checkNoDot(nodep);
        VL_RESTORER(m_isParam);
        m_isParam = nodep->varType().isParam();
        iterateChildren(nodep);
        如果 (m_statep->forPrimary() && nodep->isIO() && !m_ftaskp && !nodep->user4()) {
            nodep->v3error(
                "Input/output/inout does not appear in port list: " << nodep->prettyNameQ());
        }
    }
    void visit(AstVarScope* nodep) override {
        LINKDOT_VISIT_START();
        checkNoDot(nodep);
        iterateChildren(nodep);
        AstVarScope* aliasp = LinkDotScopeVisitor::getAlias(nodep);
        如果 (aliasp && aliasp != nodep && !nodep->varp()->isIfaceRef()) {
            // Aliased variable might still be references from outside,
            // eg through the VPI, and is traced, so we need the value to propagate.
            // TODO: this means external writes to the LHS (e.g.: through the VPI) don't work
            AstAssignW* const assignp = new AstAssignW{
                nodep->fileline(), new AstVarRef{nodep->fileline(), nodep, VAccess::WRITE},
                new AstVarRef{nodep->fileline(), aliasp, VAccess::READ}};
            assignp->user2(true);
            nodep->scopep()->addBlocksp(new AstAlways{assignp});
            // Propagate attributes and lint state of the replaced variable,
            // because all references to it are replaced with references to the alias variable
            aliasp->varp()->propagateAttrFrom(nodep->varp());
            aliasp->varp()->fileline()->modifyStateInherit(nodep->varp()->fileline());
        }
    }
    void visit(AstNodeFTaskRef* nodep) override {
        如果 (nodep->user3SetOnce()) 返回;
        LINKDOT_VISIT_START();
        UINFO(8, indent() << "visit " << nodep);
        UINFO(8, indent() << m_ds.ascii());
        如果 (m_ds.m_dotPos != DP_MEMBER || nodep->name() != "randomize") {
            // Visit arguments at the beginning.
            // They may be visitted even if the current node can't be linked now.
            symIterateChildren(nodep, m_curSymp);
        }

        如果 (m_ds.m_super) {
            如果 (AstFuncRef* const funcRefp = VN_CAST(nodep, FuncRef)) {
                funcRefp->superReference(true);
            } 否则 如果 (AstTaskRef* const taskRefp = VN_CAST(nodep, TaskRef)) {
                taskRefp->superReference(true);
            }
        }

        VL_RESTORER(m_randSymp);

        bool first = !m_ds.m_dotp || m_ds.m_dotPos == DP_FIRST;
        bool staticAccess = false;
        如果 (m_ds.m_unresolvedClass) {
            // Unable to link before V3Param
            返回;
        } 否则 如果 (m_ds.m_unresolvedCell && m_ds.m_dotPos == DP_FINAL && m_ds.m_unlinkedScopep) {
            AstNodeFTaskRef* const newftaskp = nodep->cloneTree(false);
            newftaskp->dotted(m_ds.m_dotText);
            AstNode* const newp = new AstUnlinkedRef{nodep->fileline(), newftaskp, nodep->name(),
                                                     m_ds.m_unlinkedScopep->unlinkFrBack()};
            m_ds.m_unlinkedScopep = nullptr;
            m_ds.m_unresolvedCell = false;
            nodep->replaceWith(newp);
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
            返回;
        } 否则 如果 (m_ds.m_dotp && m_ds.m_dotPos == DP_PACKAGE) {
            UASSERT_OBJ(VN_IS(m_ds.m_dotp->lhsp(), ClassOrPackageRef), m_ds.m_dotp->lhsp(),
                        "Bad package link");
            staticAccess = true;
            AstClassOrPackageRef* const cpackagerefp
                = VN_AS(m_ds.m_dotp->lhsp(), ClassOrPackageRef);
            如果 (cpackagerefp->name() == "local::") {
                m_randSymp = nullptr;
                first = true;
            } 否则 如果 (!cpackagerefp->classOrPackageSkipp()) {
                VSymEnt* const foundp = m_statep->resolveClassOrPackage(
                    m_ds.m_dotSymp, cpackagerefp, true, false, ":: reference");
                如果 (foundp) nodep->classOrPackagep(cpackagerefp->classOrPackageSkipp());
            } 否则 {
                nodep->classOrPackagep(cpackagerefp->classOrPackageSkipp());
            }
            // Class/package :: HERE function() . method_called_on_function_return_value()
            m_ds.m_dotPos = DP_MEMBER;
        } 否则 如果 (m_ds.m_dotp && m_ds.m_dotPos == DP_FINAL) {
            nodep->dotted(m_ds.m_dotText);  // Maybe ""
            // Only flag FTaskRefs under generate-if/case blocks that may be
            // pruned.  GenFor and plain begin-blocks won't be pruned by V3Param.
            // A generate-case block's parent is the GenCaseItem, not the GenCase.
            // VarXRef uses the broader m_genBlk flag (set for all GenBlocks)
            // because genfor unrolling also removes variables.
            如果 (m_ds.m_genBlk && m_ds.m_dotSymp) {
                const AstNode* const blkp = m_ds.m_dotSymp->nodep();
                如果 (VN_IS(blkp, GenBlock)
                    && (VN_IS(blkp->backp(), GenIf) || VN_IS(blkp->backp(), GenCaseItem))) {
                    nodep->containsGenBlock(true);
                }
            }
        } 否则 如果 (m_ds.m_dotp && m_ds.m_dotPos == DP_MEMBER) {
            // Found a Var, everything following is method call.
            // {scope}.{var}.HERE {method} ( ARGS )
            AstNodeExpr* const varEtcp = VN_AS(m_ds.m_dotp->lhsp()->unlinkFrBack(), NodeExpr);
            AstArg* const argsp = nodep->argsp();
            如果 (argsp) argsp->unlinkFrBackWithNext();
            AstMethodCall* const newp = new AstMethodCall{nodep->fileline(), varEtcp,
                                                          VFlagChildDType{}, nodep->name(), argsp};
            如果 (AstWith* const withp = nodep->withp()) newp->withp(withp->unlinkFrBack());
            nodep->replaceWith(newp);
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
            返回;
        } 否则 如果 (m_ds.m_dotp && (m_ds.m_dotPos == DP_SCOPE || m_ds.m_dotPos == DP_FIRST)) {
            // HERE function() . method_called_on_function_return_value()
            m_ds.m_dotPos = DP_MEMBER;
            m_ds.m_dotText = "";
        } 否则 如果 (!m_ds.m_disablep) {
            // visit(AstDisable*) setup the dot handling
            checkNoDot(nodep);
        }
        如果 (nodep->classOrPackagep() && nodep->taskp()) {
            // References into packages don't care about cell hierarchy.
        } 否则 如果 (!m_modSymp) {
            // Module that is not in hierarchy.  We'll be dead code eliminating it later.
            UINFO(9, indent() << "Dead module for " << nodep);
            nodep->taskp(nullptr);
        } 否则 如果 (nodep->dotted() == "" && nodep->taskp()) {
            // Earlier should have setup the links
            // Might be under a BEGIN we're not processing, so don't relink it
        } 否则 {
            string baddot;
            VSymEnt* okSymp = nullptr;
            VSymEnt* dotSymp;
            如果 (nodep->dotted().empty()) {
                // Non-'super.' dotted reference
                dotSymp = m_ds.m_dotSymp;
            } 否则 {
                // Start search at module, as a variable of same name under a subtask isn't a
                // relevant hit however a function under a begin/end is.  So we want begins, but
                // not the function
                dotSymp = m_curSymp;
            }
            如果 (nodep->classOrPackagep()) {  // Look only in specified package
                dotSymp = m_statep->getNodeSym(nodep->classOrPackagep());
                UINFO(8, indent() << "Override classOrPackage " << dotSymp);
            } 否则 {
                如果 (nodep->inlinedDots() != "") {  // Correct for current post-inlined scope
                    dotSymp = findInlinedDotsSym(nodep, nodep->inlinedDots());
                }
                dotSymp = m_statep->findDotted(nodep->fileline(), dotSymp, nodep->dotted(), baddot,
                                               okSymp, true);  // Maybe nullptr
            }
            // Restricted 'with (id_list)': only id_list names bind into target class.
            如果 (m_randSymp
                && (!m_currentWithp || m_currentWithp->nameResolvesToTarget(nodep->name()))) {
                VSymEnt* const foundp = m_randSymp->findIdFlat(nodep->name());
                如果 (foundp && m_currentWithp) {
                    如果 (m_currentWithp->restricted()) {
                        m_restrictedNamesUsed.insert(nodep->name());
                    }
                    UINFO(9, indent() << "randomize-with fromSym " << foundp->nodep());
                    AstArg* argsp = nullptr;
                    如果 (nodep->argsp()) {
                        iterateAndNextNull(nodep->argsp());
                        argsp = nodep->argsp()->unlinkFrBackWithNext();
                    }
                    如果 (m_ds.m_dotPos != DP_NONE) m_ds.m_dotPos = DP_MEMBER;
                    AstNode* const newp = new AstMethodCall{
                        nodep->fileline(),
                        new AstLambdaArgRef{nodep->fileline(), "__Vrandwith_obj", false},
                        VFlagChildDType{}, nodep->name(), argsp};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(pushDeletep(nodep), nodep);
                    返回;
                }
            }
            如果 (first && nodep->name() == "randomize" && VN_IS(m_modp, Class)
                && !VN_IS(nodep->classOrPackagep(), Package)) {
                // need special handling to avoid falling back to std::randomize
                // Skip if classOrPackagep is a Package (i.e. std::randomize resolved earlier)
                VMemberMap memberMap;
                AstFunc* const randFuncp = V3Randomize::newRandomizeFunc(
                    memberMap, VN_AS(m_modp, Class), nodep->name(), true, true);
                nodep->taskp(randFuncp);
                nodep->classOrPackagep(VN_AS(m_modp, Class));
                m_curSymp = m_statep->insertBlock(m_curSymp, nodep->name(), randFuncp, m_modp);
                // Already linked to the class randomize function; skip findSymPrefixed
                // which may find std::randomize and overwrite classOrPackagep
                返回;
            }
            VSymEnt* const foundp
                = m_statep->findSymPrefixed(dotSymp, nodep->name(), baddot, first);
            AstNodeFTask* const taskp = foundp ? VN_CAST(foundp->nodep(), NodeFTask) : nullptr;
            AstNodeModule* const taskModulep = foundp ? foundp->classOrPackagep() : nullptr;
            // Ignore deferring linking of functions inside params of classes extending
            // parametric classes if referenced function can be linked in the first pass.
            如果 (!m_isParam || !taskp
                || (taskp->classMethod() && taskModulep && taskModulep->hasParameterList())) {
                如果 (m_insideClassExtParam) {
                    // The reference may point to a method declared in a super class, which is
                    // proved by a parameter. In such a case, it can't be linked at the first
                    // stage. Must not do any linking, because e.g. might find an extends of an
                    // upper class because the current class (under parent) isn't yet importing
                    // it's extended class symbols.
                    返回;
                }
                如果 (AstClass* const targetClassp = VN_CAST(dotSymp->nodep(), Class)) {
                    如果 (m_extendsParam.count(targetClassp)) {
                        // Target class has parameterized extends not yet resolved.
                        // Its inherited symbols (e.g. static functions from the base class)
                        // aren't imported yet - defer to linkDotParamed.
                        返回;
                    }
                }
            }

            如果 (taskp) {
                如果 (staticAccess && !taskp->isStatic()) {
                    // TODO bug4077
                    // nodep->v3error("Static access to non-static task/function "
                    //                << taskp->prettyNameQ() << endl);
                }
                nodep->taskp(taskp);
                nodep->classOrPackagep(foundp->classOrPackagep());
                UINFO(7, indent() << "Resolved " << nodep);  // Also prints taskp
            } 否则 {
                // Note ParseRef has similar error handling/message output
                UINFO(7, indent() << "   ErrFtask curSymp=se" << cvtToHex(m_curSymp)
                                  << " dotSymp=se" << cvtToHex(dotSymp));
                如果 (foundp) {
                    如果 (VN_IS(foundp->nodep(), Var) && m_ds.m_dotText == "" && m_ftaskp
                        && m_ftaskp->name() == foundp->nodep()->name()) {
                        // This is a recursive reference to the function itself, not to the var
                        nodep->taskp(m_ftaskp);
                        nodep->classOrPackagep(foundp->classOrPackagep());
                        UINFO(7, indent() << "Resolved recursive " << nodep);  // Also prints taskp
                    } 否则 {
                        nodep->v3error("Found definition of '"
                                       << m_ds.m_dotText << (m_ds.m_dotText == "" ? "" : ".")
                                       << nodep->prettyName() << "'" << " as a "
                                       << foundp->nodep()->typeName()
                                       << " but expected a task/function");
                    }
                } 否则 如果 (VN_IS(nodep, New) && !m_statep->forScopeCreation()) {
                    // Resolved in V3Width
                } 否则 如果 ((nodep->name() == "pre_randomize" || nodep->name() == "post_randomize")
                           && VN_IS(dotSymp->nodep(), Class)) {
                    AstClass* const classp = VN_AS(dotSymp->nodep(), Class);
                    // Functions that might be defined by user, but if not, they get empty
                    // definitions within referenced class.
                    // TODO perhaps cleaner to add to symbol table up-front with
                    // a lambda to construct them on a reference.
                    AstFunc* const funcp
                        = new AstFunc{nodep->fileline(), nodep->name(), nullptr, nullptr};
                    funcp->classMethod(true);
                    funcp->dtypep(funcp->findVoidDType());
                    classp->addMembersp(funcp);
                    m_statep->insertBlock(dotSymp, nodep->name(), funcp, classp);
                    nodep->taskp(funcp);
                    nodep->classOrPackagep(classp);
                    UINFO(9, indent() << " Auto-made " << funcp);
                    返回;
                } 否则 如果 (nodep->name() == "randomize" || nodep->name() == "srandom"
                           || nodep->name() == "get_randstate" || nodep->name() == "set_randstate"
                           || nodep->name() == "rand_mode" || nodep->name() == "constraint_mode") {
                    如果 (AstClass* const classp = VN_CAST(m_modp, Class)) {
                        // Don't overwrite if already resolved to a package (e.g. std::randomize)
                        如果 (!VN_IS(nodep->classOrPackagep(), Package))
                            nodep->classOrPackagep(classp);
                    } 否则 如果 (nodep->name() == "randomize") {
                        // A std::randomize resolved in V3Width
                        nodep->classOrPackagep(v3Global.rootp()->stdPackagep());
                    } 否则 {
                        nodep->v3error("Calling implicit class method "
                                       << nodep->prettyNameQ() << " without being under class");
                        nodep->replaceWith(new AstConst{nodep->fileline(), 0});
                        VL_DO_DANGLING(pushDeletep(nodep), nodep);
                        返回;
                    }
                } 否则 如果 (nodep->dotted() == "") {
                    如果 (nodep->pli()) {
                        如果 (v3Global.opt.bboxSys()) {
                            AstNode* newp;
                            如果 (VN_IS(nodep, FuncRef)) {
                                newp = new AstConst{nodep->fileline(), AstConst::All0{}};
                            } 否则 {
                                AstNodeExpr* outp = nullptr;
                                当 (AstArg* const argp = nodep->argsp()) {
                                    outp = AstNode::addNext(outp, argp->exprp()->unlinkFrBack());
                                    VL_DO_DANGLING(pushDeletep(argp->unlinkFrBack()), argp);
                                }
                                newp = new AstSysIgnore{nodep->fileline(), outp};
                                newp->dtypep(nodep->dtypep());
                            }
                            nodep->replaceWith(newp);
                            VL_DO_DANGLING(pushDeletep(nodep), nodep);
                            返回;
                        } 否则 {
                            VSpellCheck speller;
                            V3Parse::candidatePli(&speller);
                            const string suggest = speller.bestCandidateMsg(nodep->prettyName());
                            nodep->v3error(
                                "Unsupported or unknown PLI call: "
                                << nodep->prettyNameQ() << '\n'
                                << (suggest.empty() ? "" : nodep->warnMore() + suggest));
                        }
                    } 否则 {
                        const string suggest = m_statep->suggestSymFallback(
                            dotSymp, nodep->name(), LinkNodeMatcherFTask{});
                        nodep->v3error("Can't find definition of task/function: "
                                       << nodep->prettyNameQ() << '\n'
                                       << (suggest.empty() ? "" : nodep->warnMore() + suggest));
                    }
                } 否则 {
                    const string suggest = m_statep->suggestSymFallback(dotSymp, nodep->name(),
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
    void visit(AstSelBit* nodep) override {
        如果 (nodep->user3SetOnce()) 返回;
        LINKDOT_VISIT_START();
        iterateAndNextNull(nodep->fromp());
        如果 (m_ds.m_unresolvedClass) {
            UASSERT_OBJ(m_ds.m_dotPos != DP_SCOPE && m_ds.m_dotPos != DP_FIRST, nodep,
                        "Object of unresolved class on scope position in dotted reference");
            // The index does not depend on the class, and V3Param must see its class references
            symIterateNull(nodep->bitp(), m_curSymp);
            symIterateNull(nodep->attrp(), m_curSymp);
            返回;
        }
        如果 (m_ds.m_dotPos == DP_SCOPE
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
        如果 (m_ds.m_unresolvedCell && (m_ds.m_dotPos == DP_SCOPE || m_ds.m_dotPos == DP_FIRST)) {
            AstNodeExpr* const exprp = nodep->bitp()->unlinkFrBack();
            如果 (AstCellArrayRef* const fromArrRefp = VN_CAST(nodep->fromp(), CellArrayRef)) {
                // Multi-dim iface array access: append this select to the existing chain
                fromArrRefp->addSelp(exprp);
                AstCellArrayRef* const movedp = fromArrRefp->unlinkFrBack();
                nodep->replaceWith(movedp);
                VL_DO_DANGLING(pushDeletep(nodep), nodep);
            } 否则 {
                AstCellArrayRef* const newp
                    = new AstCellArrayRef{nodep->fileline(), nodep->fromp()->name(), exprp};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(pushDeletep(nodep), nodep);
            }
        }
    }
    void visit(AstNodePreSel* nodep) override {
        // Excludes simple AstSelBit, see above
        如果 (nodep->user3SetOnce()) 返回;
        LINKDOT_VISIT_START();
        如果 (m_ds.m_dotPos == DP_SCOPE
            || m_ds.m_dotPos
                   == DP_FIRST) {  // Already under dot, so this is {modulepart} DOT {modulepart}
            nodep->v3error("Syntax error: Range ':', '+:' etc are not allowed in the instance "
                           "part of a dotted reference");
            m_ds.m_dotErr = true;
            返回;
        }
        AstNodeDType* packedArrayDtp = nullptr;  // Datatype reference for packed array
        {
            VL_RESTORER(m_packedArrayDtp);
            iterateAndNextNull(nodep->fromp());
            symIterateNull(nodep->rhsp(), m_curSymp);
            symIterateNull(nodep->thsp(), m_curSymp);

            如果 (m_packedArrayDtp) {
                AstRange* const newRangep
                    = new AstRange(nodep->fileline(), nodep->rhsp()->unlinkFrBack(),
                                   nodep->thsp()->unlinkFrBack());
                AstPackArrayDType* newArrayTypep
                    = new AstPackArrayDType(nodep->fileline(), m_packedArrayDtp, newRangep);
                newArrayTypep->childDTypep(m_packedArrayDtp);
                newArrayTypep->refDTypep(nullptr);
                如果 (VN_IS(nodep->backp(), SelExtract)) {
                    packedArrayDtp = newArrayTypep;
                } 否则 {
                    replaceWithCheckBreak(nodep, newArrayTypep);
                    VL_DO_DANGLING(pushDeletep(nodep), nodep);
                }
            } 否则 {
                如果 (nodep->attrp()) {
                    AstNode* const attrp = nodep->attrp()->unlinkFrBack();
                    VL_DO_DANGLING(attrp->deleteTree(), attrp);
                }
                AstNode* const basefromp = nodep->baseFromp(false);
                如果 (VN_IS(basefromp, Replicate)) {
                    // From {...}[...] syntax in IEEE 2017
                    如果 (basefromp) UINFO(9, indent() << " Related node: " << basefromp);
                } 否则 {
                    nodep->attrp(new AstAttrOf{nodep->fileline(), VAttrType::VAR_BASE,
                                               basefromp->cloneTree(false)});
                }
            }
        }

        如果 (packedArrayDtp) m_packedArrayDtp = packedArrayDtp;
    }
    void visit(AstMemberSel* nodep) override {
        // checkNoDot not appropriate, can be under a dot
        LINKDOT_VISIT_START();
        iterateChildren(nodep);
    }
    void visit(AstGenBlock* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        {
            VL_RESTORER(m_curSymp);
            VL_RESTORER_COPY(m_ds);
            如果 (nodep->name() != "") {
                m_ds.m_dotSymp = m_curSymp = m_statep->getNodeSym(nodep);
                UINFO(5, indent() << "cur=se" << cvtToHex(m_curSymp));
            }
            iterateChildren(nodep);
        }
        UINFO(5, indent() << "cur=se" << cvtToHex(m_curSymp));
    }
    void visit(AstNodeBlock* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        {
            VL_RESTORER(m_curSymp);
            VL_RESTORER_COPY(m_ds);
            如果 (nodep->name() != "") {
                m_ds.m_dotSymp = m_curSymp = m_statep->getNodeSym(nodep);
                UINFO(5, indent() << "cur=se" << cvtToHex(m_curSymp));
            }
            iterateChildren(nodep);
        }
        UINFO(5, indent() << "cur=se" << cvtToHex(m_curSymp));
    }
    void visit(AstNodeFTask* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        如果 (nodep->isExternDef() && !nodep->didProto()) {
            nodep->didProto(true);
            如果 (const VSymEnt* const foundp
                = m_curSymp->findIdFallback("extern " + nodep->name())) {
                const AstNodeFTask* const protop = VN_AS(foundp->nodep(), NodeFTask);
                // Copy specifiers.
                // External definition cannot have any specifiers, so no value will be overwritten.
                nodep->isHideLocal(protop->isHideLocal());
                nodep->isHideProtected(protop->isHideProtected());
                nodep->isStatic(protop->isStatic());
                nodep->isVirtual(protop->isVirtual());
                nodep->lifetime(protop->lifetime());
                // Always add an FUNC_RETURN_PROTO even if task, so we can locate the prototype
                nodep->addStmtsp(new AstAttrOf{protop->fileline(), VAttrType::FUNC_RETURN_PROTO,
                                               protop->fvarp()
                                                   ? protop->fvarp()->cloneTree(false)
                                                   : new AstVoidDType{protop->fileline()}});
                为 (AstNode* stmtp = protop->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    如果 (AstVar* const portp = VN_CAST(stmtp, Var)) {
                        AstAttrOf* const attrp
                            = new AstAttrOf{protop->fileline(), VAttrType::FUNC_ARG_PROTO,
                                            portp->subDTypep()->cloneTree(false)};
                        attrp->name(portp->name());
                        nodep->addStmtsp(attrp);
                    }
                }
            } 否则 {
                nodep->v3error("extern not found that declares " + nodep->prettyNameQ());
            }
        }
        如果 (nodep->isExternProto()) {
            如果 (!m_curSymp->findIdFallback(nodep->name())) {
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
            const bool isNew = nodep->name() == "new";
            如果 (isNew) m_explicitSuperNewp = nullptr;
            iterateChildren(nodep);
            如果 (isNew) {
                const AstClassExtends* const classExtendsp = VN_AS(m_modp, Class)->extendsp();
                如果 (m_explicitSuperNewp && !m_explicitSuperNewp->isImplicit() && classExtendsp
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
                如果 (classExtendsp && classExtendsp->classOrNullp()) {
                    如果 (!m_explicitSuperNewp && m_statep->forParamed()
                        && !VN_AS(classExtendsp->childDTypep()->skipRefp(), ClassRefDType)
                                ->classp()
                                ->isInterfaceClass()) {
                        AstNodeStmt* const superNewp
                            = addImplicitSuperNewCall(VN_AS(nodep, Func), classExtendsp);
                        UINFO(9, "created super new " << superNewp);
                        iterate(superNewp);
                    }
                }
            }
        }
        m_ds.m_dotSymp = VL_RESTORER_PREV(m_curSymp);
    }
    void visit(AstNodeForeach* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        symIterateChildren(nodep, m_statep->getNodeSym(nodep));
    }
    void visit(AstRandSequence* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        如果 (!nodep->start().empty())
            nodep->prodp(findProd(nodep, m_statep->getNodeSym(nodep), nodep->start()));
        symIterateChildren(nodep, m_statep->getNodeSym(nodep));
    }
    void visit(AstRSProd* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        symIterateChildren(nodep, m_statep->getNodeSym(nodep));
    }
    void visit(AstRSProdItem* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        nodep->prodp(findProd(nodep, m_curSymp, nodep->name()));
        iterateChildren(nodep);
    }
    void visit(AstCoverWith* nodep) override {
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
    void visit(AstCoverCrossWith* nodep) override {
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
    void visit(AstWith* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        checkNoDot(nodep);
        如果 (m_ds.m_unresolvedClass) 返回;
        VL_RESTORER(m_curSymp);
        VL_RESTORER(m_currentWithp);
        VL_RESTORER_COPY(m_restrictedNamesUsed);
        {
            m_ds.m_dotSymp = m_curSymp = m_statep->getNodeSym(nodep);
            m_currentWithp = nodep;
            // Validate the identifier_list once per AstWith (m_validated travels
            // with cloneTree, so cloned witnesses skip the redundant check).
            const bool firstVisit = !nodep->validated();
            如果 (firstVisit) {
                nodep->validated(true);
                m_restrictedNamesUsed.clear();
                // IEEE 1800-2023 18.7: each name in the identifier_list must designate
                // a member of the randomize() target class.
                如果 (nodep->restricted() && m_randSymp) {
                    为 (const std::string& n : nodep->restrictedNames()) {
                        如果 (!m_randSymp->findIdFlat(n)) {
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
            如果 (firstVisit && nodep->restricted()) {
                为 (const std::string& n : nodep->restrictedNames()) {
                    如果 (!m_restrictedNamesUsed.count(n)) {
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
    void visit(AstLambdaArgRef* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        // No checknodot(nodep), visit(AstScope) will check for LambdaArgRef
        iterateChildren(nodep);
    }
    void visit(AstClassExtends* nodep) override {
        // Resolve the symbol and get the class.
        // If it is a parameterized case, the class will be resolved after V3Param.cpp
        如果 (nodep->user3SetOnce()) 返回;
        // If the class is resolved, there is nothing more to do
        如果 (nodep->classOrNullp()) 返回;
        LINKDOT_VISIT_START();
        如果 (m_statep->forPrimary()) {
            如果 (nodep->childDTypep()) 返回;
            AstNode* cprp = nodep->classOrPkgsp();
            VSymEnt* lookSymp = m_curSymp;
            如果 (AstDot* const dotp = VN_CAST(cprp, Dot)) {
                dotp->user3(true);
                如果 (AstClassOrPackageRef* lookNodep = VN_CAST(dotp->lhsp(), ClassOrPackageRef)) {
                    iterate(lookNodep);
                    cprp = dotp->rhsp();
                    VSymEnt* const foundp = m_statep->resolveClassOrPackage(
                        lookSymp, lookNodep, true, false, nodep->verilogKwd());
                    如果 (!foundp) {
                        VL_DO_DANGLING(pushDeletep(nodep->unlinkFrBack()), nodep);
                        返回;
                    }
                    UASSERT_OBJ(lookNodep->classOrPackageSkipp(), nodep, "Bad package link");
                    lookSymp = m_statep->getNodeSym(lookNodep->classOrPackageSkipp());
                } 否则 {
                    dotp->lhsp()->v3error("Attempting to extend"  // LCOV_EXCL_LINE
                                          " using non-class under dot");
                }
            }
            AstClassOrPackageRef* const cpackagerefp = VN_CAST(cprp, ClassOrPackageRef);
            如果 (VL_UNCOVERABLE(!cpackagerefp)) {
                // Linking the extend gives an error before this is hit
                nodep->v3error("Attempting to extend using non-class");  // LCOV_EXCL_LINE
                VL_DO_DANGLING(pushDeletep(nodep->unlinkFrBack()), nodep);
                返回;
            }
            VSymEnt* const foundp = m_statep->resolveClassOrPackage(lookSymp, cpackagerefp, true,
                                                                    true, nodep->verilogKwd());
            如果 (!foundp) {
                VL_DO_DANGLING(pushDeletep(nodep->unlinkFrBack()), nodep);
                返回;
            }
            如果 (AstClass* const classp = VN_CAST(foundp->nodep(), Class)) {
                AstPin* paramsp = cpackagerefp->paramsp();
                如果 (paramsp) {
                    paramsp = paramsp->cloneTree(true);
                    nodep->parameterized(true);
                }
                nodep->childDTypep(new AstClassRefDType{nodep->fileline(), classp, paramsp});
                // Link pins
                iterate(nodep->childDTypep());
            } 否则 如果 (AstParamTypeDType* const paramp
                       = VN_CAST(foundp->nodep(), ParamTypeDType)) {
                AstRefDType* const refParamp = new AstRefDType{nodep->fileline(), paramp->name()};
                refParamp->refDTypep(paramp);
                nodep->childDTypep(refParamp);
                nodep->parameterized(true);
            } 否则 如果 (AstTypedef* const typedefp = VN_CAST(foundp->nodep(), Typedef)) {
                AstNodeDType* const unwrappedp = typedefp->subDTypep()->skipRefp();
                如果 (AstClassRefDType* const classRefp = VN_CAST(unwrappedp, ClassRefDType)) {
                    AstPin* paramsp = cpackagerefp->paramsp();
                    如果 (!paramsp && classRefp->paramsp()) {
                        // No explicit #(...) on extends clause; carry over the
                        // typedef's type-parameter pins so V3Param sees them.
                        paramsp = classRefp->paramsp()->cloneTree(true);
                        nodep->parameterized(true);
                    } 否则 如果 (paramsp) {
                        paramsp = paramsp->cloneTree(true);
                        nodep->parameterized(true);
                    }
                    nodep->childDTypep(
                        new AstClassRefDType{nodep->fileline(), classRefp->classp(), paramsp});
                    iterate(nodep->childDTypep());
                } 否则 {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: " << foundp->nodep()->prettyTypeName()
                                                  << " in 'class extends'");
                    返回;
                }
            } 否则 {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: " << foundp->nodep()->prettyTypeName()
                                                             << " in 'class extends'");
                返回;
            }
            如果 (!nodep->childDTypep()) nodep->v3error("Attempting to extend using non-class");
            nodep->classOrPkgsp()->unlinkFrBack()->deleteTree();
        } 否则 {
            // Probably a parameter
            如果 (AstRefDType* const refp = VN_CAST(nodep->childDTypep(), RefDType)) {
                如果 (AstClassRefDType* classRefp = VN_CAST(refp->skipRefp(), ClassRefDType)) {
                    // Resolved to a class reference.
                    refp->replaceWith(classRefp->cloneTree(false));
                    VL_DO_DANGLING(pushDeletep(refp), refp);
                } 否则 {
                    // Unable to resolve the ref type to a class reference.
                    // Get the value of type parameter passed to the class instance,
                    // to print the helpful error message.
                    const AstNodeDType* typep = refp->refDTypep();
                    当 (true) {
                        如果 (const AstParamTypeDType* const atypep
                            = VN_CAST(typep, ParamTypeDType)) {
                            typep = atypep->subDTypep();
                            continue;
                        }
                        如果 (const AstRequireDType* const atypep = VN_CAST(typep, RequireDType)) {
                            typep = atypep->subDTypep();
                            continue;
                        }
                        break;
                    }
                    typep->v3error("Attempting to extend using non-class");
                }
            }
        }
    }
    void visit(AstClass* nodep) override {
        如果 (nodep->user3SetOnce()) 返回;
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
            整数 next = 0;
            为 (AstClassExtends* cextp = nodep->extendsp(); cextp;
                 cextp = VN_AS(cextp->nextp(), ClassExtends)) {
                // Replace abstract reference with hard pointer
                // Will need later resolution when deal with parameters
                如果 (++next == 2 && !nodep->isInterfaceClass() && !cextp->isImplements()) {
                    cextp->v3error("Multiple inheritance illegal on non-interface classes"
                                   " (IEEE 1800-2023 8.13)");
                }
                iterate(cextp);
                如果 (m_statep->forPrimary()) {
                    如果 (cextp->parameterized()) {
                        // Parameters in extends statement.
                        // The class can't be resolved in the current pass.
                        m_extendsParam.insert(nodep);
                        m_insideClassExtParam = true;
                    }
                    如果 (AstClassRefDType* const classRefp
                        = VN_CAST(cextp->childDTypep(), ClassRefDType)) {
                        AstClass* const classp = classRefp->classp();
                        如果 (classp != nodep) iterate(classp);
                        如果 (m_extendsParam.find(classp) != m_extendsParam.end()) {
                            // One of its super classes has parameters in extends statement.
                            // Some links may not be resolved in the first pass.
                            m_extendsParam.insert(nodep);
                            m_insideClassExtParam = true;
                        }
                    }
                }

                如果 (AstClass* const baseClassp = cextp->classOrNullp()) {
                    // Already converted. Update symbol table to link unlinked members.
                    // Base class has to be visited in a case if its extends statement
                    // needs to be handled. Recursive inheritance was already checked.
                    // Must be here instead of in LinkDotParam to handle
                    // "class (type T) extends T".
                    如果 (baseClassp == nodep) {
                        cextp->v3error("Attempting to extend class " << nodep->prettyNameQ()
                                                                     << " from itself");
                    } 否则 如果 (extendsClass(baseClassp, nodep)) {
                        cextp->v3error("Attempting to extend class "
                                       << nodep->prettyNameQ() << " from "
                                       << baseClassp->prettyNameQ()
                                       << ", which circularly inherits from it");
                    } 否则 如果 (cextp->isImplements() && !baseClassp->isInterfaceClass()) {
                        cextp->v3error("Attempting to implement from non-interface class "
                                       << baseClassp->prettyNameQ() << '\n'
                                       << "... Suggest use 'extends'");
                    } 否则 如果 (!cextp->isImplements() && !nodep->isInterfaceClass()
                               && baseClassp->isInterfaceClass()) {
                        cextp->v3error("Attempting to extend from interface class "
                                       << baseClassp->prettyNameQ() << '\n'
                                       << "... Suggest use 'implements'");
                    }
                    baseClassp->isExtended(true);
                    nodep->isExtended(true);
                    iterate(baseClassp);
                    importSymbolsFromExtended(nodep, cextp);
                    continue;
                }
            }
            m_ds.m_dotSymp = m_curSymp;

            iterateChildren(nodep);
            为 (const auto& itr : m_ifForwards) {
                const VSymEnt* const implSymp = itr.second;
                addInterfaceForward(nodep, VN_AS(implSymp->nodep(), NodeFTask),
                                    VN_AS(implSymp->parentp()->nodep(), Class));
            }
        }
        // V3Width when determines types needs to find enum values and such
        // so add members pointing to appropriate enum values
        {
            VMemberMap memberMap;
            为 (VSymEnt::const_iterator it = m_curSymp->begin(); it != m_curSymp->end(); ++it) {
                AstNode* const itemp = it->second->nodep();
                如果 (!memberMap.findMember(nodep, it->first)) {
                    如果 (AstEnumItem* const aitemp = VN_CAST(itemp, EnumItem)) {
                        AstEnumItemRef* const newp = new AstEnumItemRef{
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
    static void collectScopeRefs(AstNode* nodep, std::vector<AstClassOrPackageRef*>& refps) {
        如果 (AstClassOrPackageRef* const refp = VN_CAST(nodep, ClassOrPackageRef)) {
            refps.push_back(refp);
            返回;
        }
        AstDot* const dotp = VN_CAST(nodep, Dot);
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
        为 (size_t i = 0; i < refps.size(); ++i) {
            AstClassOrPackageRef* const refp = refps[i];
            如果 (!refp->classOrPackageSkipp() && !refp->classOrPackageNodep()
                && !m_statep->resolveClassOrPackage(scopeSymp, refp, i == 0, false,
                                                    "class/package reference")) {
                返回 false;  // Error already reported
            }
            如果 (i + 1 == refps.size()) break;
            // A middle segment that resolved to a node with no reachable module
            // (e.g. a typedef alias of a not-yet-specialized parameterized class)
            // gives nothing to look the next segment up in.
            AstNodeModule* const modp = refp->classOrPackageSkipp();
            如果 (!modp) {
                refp->v3warn(E_UNSUPPORTED, "Unsupported: Multiple '::' package/class reference");
                返回 false;
            }
            scopeSymp = m_statep->getNodeSym(modp);
        }
        AstClassOrPackageRef* const innerp = refps.back();
        innerp->unlinkFrBack();
        VL_DO_DANGLING(pushDeletep(scopeDotp->unlinkFrBack()), scopeDotp);
        nodep->classOrPackageOpp(innerp);
        返回 true;
    }

    void visit(AstRefDType* nodep) override {

        如果 (auto* const typeOfp = nodep->typeofp()) {
            iterate(typeOfp);
            // After iteration, typeofp() may have been replaced with a resolved type
            // (e.g., DOT expression like if0.rq_t resolved to REFDTYPE)
            // If it's now a NodeDType, replace this RefDType with it
            如果 (AstNodeDType* const resolvedDTypep = VN_CAST(nodep->typeofp(), NodeDType)) {
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
                如果 (V3LinkDotIfaceCapture::enabled()) {
                    如果 (AstRefDType* const resolvedRefp = VN_CAST(resolvedDTypep, RefDType)) {
                        如果 (VL_UNCOVERABLE(
                                VN_IS(resolvedRefp->user2p(), Cell))) {  // LCOV_EXCL_LINE
                            resolvedRefp->v3fatalSrc(  // LCOV_EXCL_LINE
                                "typeofp resolved RefDType has Cell in user2p;"
                                " expected to be captured already");
                        }
                    }
                }
            }
            返回;
        }

        // Resolve its reference
        如果 (m_statep->forParamed() && nodep->user3()) {
            如果 (V3LinkDotIfaceCapture::enabled() && nodep->user2p()) {
                UINFO(9, indent() << "iface capture clear user3 for captured typedef name="
                                  << nodep->name() << " cell=" << nodep->user2p());
            }
            nodep->user3(false);
        }
        如果 (nodep->user3SetOnce()) {
            如果 (V3LinkDotIfaceCapture::enabled() && nodep->user2p()) {
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
            如果 (VL_UNCOVERABLE(m_statep->forParamed() && nodep->classOrPackageOpp())) {
                nodep->v3fatalSrc(  // LCOV_EXCL_LINE
                    "RefDType user3 revisit with classOrPackageOpp in paramed pass");
            }
            返回;
        }
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        如果 (nodep->classOrPackageOpp()) {
            // Resolve a chained scope (`pkg::cls::t`, `pkg::cls#(P)::t`) from outside in.
            // On failure reduceScopeDot has reported, so stop before using the operand.
            如果 (AstDot* const scopeDotp = VN_CAST(nodep->classOrPackageOpp(), Dot)) {
                如果 (!reduceScopeDot(nodep, scopeDotp)) 返回;
            }
            // Re-read: reduceScopeDot may have replaced the operand
            AstNode* const cpackagep = nodep->classOrPackageOpp();
            UASSERT_OBJ(!VN_IS(cpackagep, Dot), cpackagep,
                        "Package/class scope operand should be reduced by now");
            如果 (AstClassOrPackageRef* const cpackagerefp = VN_CAST(cpackagep, ClassOrPackageRef)) {
                iterate(cpackagerefp);
                const AstClass* const clsp = VN_CAST(cpackagerefp->classOrPackageNodep(), Class);
                如果 (clsp && clsp->hasGParam()) {
                    // Unable to link before the instantiation of parameter classes.
                    // The class reference node still has to be visited now to later link
                    // parameters.
                    iterate(cpackagep);
                    返回;
                }
                // Defer non-typedef references through typedef aliases of parameterized classes.
                如果 (m_statep->forPrimary() && !VN_IS(nodep->backp(), Typedef)
                    && isParamedClassRef(cpackagerefp)) {
                    iterate(cpackagep);
                    返回;
                }

                const bool doDefaultTypedef = !(m_resolvingTypedef && m_statep->forPrimary());
                如果 (!cpackagerefp->classOrPackageSkipp(doDefaultTypedef)
                    && !cpackagerefp->classOrPackageNodep()) {
                    VSymEnt* const foundp = m_statep->resolveClassOrPackage(
                        m_ds.m_dotSymp, cpackagerefp, true, false, "class/package reference");
                    如果 (!foundp) 返回;
                }
                nodep->classOrPackagep(cpackagerefp->classOrPackageSkipp(doDefaultTypedef));
                如果 (!VN_IS(nodep->classOrPackagep(), Class)
                    && !VN_IS(nodep->classOrPackagep(), Package)) {
                    如果 (m_statep->forPrimary()) {
                        // It may be a type that comes from parameter class that is not
                        // instantioned yet
                        iterate(cpackagep);
                        返回;
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

        如果 (m_ds.m_dotp && (m_ds.m_dotPos == DP_PACKAGE || m_ds.m_dotPos == DP_SCOPE)) {
            UASSERT_OBJ(VN_IS(m_ds.m_dotp->lhsp(), ClassOrPackageRef), m_ds.m_dotp->lhsp(),
                        "Bad package link");
            auto* const cpackagerefp = VN_CAST(m_ds.m_dotp->lhsp(), ClassOrPackageRef);
            UASSERT_OBJ(cpackagerefp->classOrPackageSkipp(), m_ds.m_dotp->lhsp(),
                        "Bad package link");
            nodep->classOrPackagep(cpackagerefp->classOrPackageSkipp());
            m_ds.m_dotPos = DP_SCOPE;
        } 否则 {
            // Allow REFDTYPE under DOT when referencing interface typedefs
            // This is needed for patterns like: typedef iface.a_t a_t;
            // The dependency graph handles resolution when IfaceCapture is disabled
            const bool ifaceFinalSegmentAllowed
                = (m_ds.m_dotPos == DP_FINAL) && m_ds.m_dotSymp
                  && VN_IS(m_ds.m_dotSymp->nodep(), Cell)
                  && VN_CAST(m_ds.m_dotSymp->nodep(), Cell)->modp()
                  && VN_IS(VN_CAST(m_ds.m_dotSymp->nodep(), Cell)->modp(), Iface);
            如果 (!ifaceFinalSegmentAllowed) {
                checkNoDot(nodep);
            } 否则 {
                UINFO(9, indent() << "allow REFDTYPE under DOT for iface typedef name="
                                  << nodep->prettyNameQ() << " dotSym=" << m_ds.m_dotSymp);
                // Clear the dot state so we don't propagate errors
                m_ds.m_dotPos = DP_SCOPE;
            }
        }
        如果 (!nodep->typedefp() && !nodep->subDTypep()) {
            const VSymEnt* foundp;
            如果 (nodep->classOrPackagep()) {
                foundp = m_statep->getNodeSym(nodep->classOrPackagep())->findIdFlat(nodep->name());
            } 否则 如果 (m_ds.m_dotPos == DP_FIRST || m_ds.m_dotPos == DP_NONE) {
                foundp = findIdFallbackSkipMemberDType(m_curSymp, nodep->name());
            } 否则 {
                // Defensive: dotPos should be DP_FIRST/DP_NONE or classOrPackagep set.
                v3fatalSrc("Unexpected dotPos="
                           << static_cast<整数>(m_ds.m_dotPos)  // LCOV_EXCL_LINE
                           << " in RefDType lookup for "
                           << nodep->prettyNameQ());  // LCOV_EXCL_LINE
                foundp = nullptr;  // LCOV_EXCL_LINE
            }
            {
                如果 (AstTypedef* const defp
                    = foundp ? VN_CAST(foundp->nodep(), Typedef) : nullptr) {
                    // Don't check if typedef is to a <type T>::<reference> as might not be
                    // resolved yet
                    如果 (!nodep->classOrPackagep() && !defp->isUnderClass())
                        checkDeclOrder(nodep, defp);
                    nodep->typedefp(defp);
                    nodep->classOrPackagep(foundp->classOrPackagep());

                } 否则 如果 (AstParamTypeDType* const defp
                           = foundp ? VN_CAST(foundp->nodep(), ParamTypeDType) : nullptr) {
                    // A PARAMTYPEDTYPE's child REFDTYPE referencing itself is the normal
                    // AST structure for type parameters (e.g., "type T = base" has a child
                    // REFDTYPE for the default type). Only error on true recursion where
                    // a TYPEDEF contains a reference back to itself.
                    const bool isParamTypeChild = (defp == nodep->backp());
                    const bool isTypedefRecursion
                        = isParamTypeChild && VN_IS(defp->backp(), Typedef);
                    如果 (isTypedefRecursion) {
                        nodep->v3error("Reference to '"
                                       << m_ds.m_dotText << (m_ds.m_dotText == "" ? "" : ".")
                                       << nodep->prettyName() << "'"
                                       << " type would form a recursive definition");
                        nodep->refDTypep(nodep->findVoidDType());  // Try to reduce later errors
                    } 否则 {
                        nodep->refDTypep(defp);
                        nodep->classOrPackagep(foundp->classOrPackagep());
                        captureIfaceParamType(nodep, defp);
                    }
                } 否则 如果 (AstClass* const defp
                           = foundp ? VN_CAST(foundp->nodep(), Class) : nullptr) {
                    // Don't check if typedef is to a <type T>::<reference> as might not be
                    // resolved yet
                    如果 (!nodep->classOrPackagep()) checkDeclOrder(nodep, defp);
                    AstPin* const paramsp = nodep->paramsp();
                    如果 (paramsp) paramsp->unlinkFrBackWithNext();
                    AstClassRefDType* const newp
                        = new AstClassRefDType{nodep->fileline(), defp, paramsp};
                    newp->classOrPackagep(foundp->classOrPackagep());
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(pushDeletep(nodep), nodep);
                    返回;
                } 否则 如果 (m_insideClassExtParam) {
                    返回;
                } 否则 {
                    如果 (foundp) {
                        UINFO(1, "Found sym node: " << foundp->nodep());
                        nodep->v3error("Expecting a data type: "
                                       << nodep->prettyNameQ()
                                       << ", found: " << foundp->nodep()->prettyTypeName());
                    } 否则 {
                        nodep->v3error("Can't find typedef/interface: " << nodep->prettyNameQ());
                    }
                }
            }
        }
        iterateChildren(nodep);
    }
    void visit(AstRequireDType* nodep) override {
        // Handle type validation for localparam type assignments
        // Iterate child to resolve any ParseRef nodes
        iterateChildren(nodep);
        // Only emit error if the child is not a type.
        // Do NOT unwrap valid types here - leave that to V3Width.
        // Unwrapping here breaks type parameter resolution during cloning.
        // Allow Dot through to defer the handling until the dot expression is resolved.
        如果 (nodep->lhsp() && !VN_IS(nodep->lhsp(), NodeDType) && !VN_IS(nodep->lhsp(), Dot)) {
            // Not a type - emit error
            如果 (VN_IS(nodep->lhsp(), Const)) {
                nodep->lhsp()->v3error("Expecting a data type, not a constant");
            } 否则 {
                nodep->lhsp()->v3error("Expecting a data type, not "
                                       << nodep->lhsp()->typeName() << ": '"
                                       << nodep->lhsp()->prettyName() << "'");
            }
            // Replace with void to keep AST valid and allow clean exit
            nodep->replaceWith(new AstVoidDType{nodep->fileline()});
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
        }
    }
    void visit(AstDpiExport* nodep) override {
        // AstDpiExport: Make sure the function referenced exists, then dump it
        LINKDOT_VISIT_START();
        iterateChildren(nodep);
        checkNoDot(nodep);
        VSymEnt* const foundp = m_curSymp->findIdFallback(nodep->name());
        AstNodeFTask* const taskp = foundp ? VN_AS(foundp->nodep(), NodeFTask) : nullptr;
        如果 (!taskp) {
            nodep->v3error(
                "Can't find definition of exported task/function: " << nodep->prettyNameQ());
        } 否则 如果 (taskp->dpiExport()) {
            nodep->v3error("Function was already DPI Exported, duplicate not allowed: "
                           << nodep->prettyNameQ());
        } 否则 {
            taskp->dpiExport(true);
            如果 (nodep->cname() != "") taskp->cname(nodep->cname());
        }
        VL_DO_DANGLING(pushDeletep(nodep->unlinkFrBack()), nodep);
    }
    void visit(AstDisable* nodep) override {
        LINKDOT_VISIT_START();
        checkNoDot(nodep);
        bool hasPartSelect = false;
        const string targetPath
            = nodep->targetRefp() ? extractDottedPath(nodep->targetRefp(), hasPartSelect) : "";
        VL_RESTORER_COPY(m_ds);
        m_ds.init(m_curSymp);
        m_ds.m_dotPos = DP_FIRST;
        m_ds.m_disablep = nodep;
        iterateChildren(nodep);
        如果 (nodep->targetRefp()) {
            如果 (AstTaskRef* const taskRefp = VN_CAST(nodep->targetRefp(), TaskRef)) {
                nodep->targetp(taskRefp->taskp());
            } 否则 如果 (!VN_IS(nodep->targetRefp(), ParseRef)) {
                // If it is a ParseRef, either it couldn't be linked or it is linked to a block
                nodep->v3warn(E_UNSUPPORTED, "Node of type "
                                                 << nodep->targetRefp()->prettyTypeName()
                                                 << " referenced by disable");
                VL_DO_DANGLING(pushDeletep(nodep->unlinkFrBack()), nodep);
                返回;
            }
            如果 (nodep->targetp()) {
                nodep->targetRefp()->unlinkFrBack()->deleteTree();
                如果 (!hasPartSelect) {
                    const string instancePath = targetInstancePath(nodep->targetp(), targetPath);
                    如果 (!instancePath.empty()) {
                        // Keep only the instance prefix so V3LinkJump can reference the selected
                        // instance's process queue without reparsing the disable target.
                        nodep->targetRefp(
                            new AstVarXRef{nodep->fileline(), "", instancePath, VAccess::READ});
                    }
                }
            }
        }
    }
    void visitPackageImportOrExport(AstNode* nodep) {
        // No longer needed
        LINKDOT_VISIT_START();
        checkNoDot(nodep);
        如果 (m_statep->forParamed()) VL_DO_DANGLING(pushDeletep(nodep->unlinkFrBack()), nodep);
    }
    void visit(AstPackageImport* nodep) override { visitPackageImportOrExport(nodep); }
    void visit(AstPackageExport* nodep) override { visitPackageImportOrExport(nodep); }
    void visit(AstPackageExportStarStar* nodep) override { visitPackageImportOrExport(nodep); }
    void visit(AstCellRef* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        iterateChildren(nodep);
    }
    void visit(AstCellArrayRef* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        // Expression already iterated
    }
    void visit(AstUnlinkedRef* nodep) override {
        LINKDOT_VISIT_START();
        UINFO(5, indent() << "visit " << nodep);
        // No need to iterate, if we have a UnlinkedVarXRef, we're already done
    }
    void visit(AstStmtExpr* nodep) override {
        LINKDOT_VISIT_START();
        checkNoDot(nodep);
        // Check if nodep represents a super.new call;
        如果 (AstNew* const newExprp = VN_CAST(nodep->exprp(), New)) {
            // in this case it was already linked, so it doesn't have a super reference
            m_explicitSuperNewp = newExprp;
        } 否则 如果 (const AstDot* const dotp = VN_CAST(nodep->exprp(), Dot)) {
            如果 (dotp->lhsp()->name() == "super") {
                如果 (AstNew* const newExprp = VN_CAST(dotp->rhsp(), New)) {
                    m_explicitSuperNewp = newExprp;
                }
            }
        }
        iterateChildren(nodep);
    }

    void visit(AstIfaceRefDType* nodep) override {
        如果 (nodep->user3SetOnce()) 返回;
        LINKDOT_VISIT_START();
        如果 (nodep->paramsp()) {
            // If there is no parameters, there is no need to visit this node.
            AstIface* const ifacep = nodep->ifacep();
            UASSERT_OBJ(ifacep, nodep, "Port parameters of AstIfaceRefDType without ifacep()");
            如果 (ifacep->dead()) 返回;
            checkNoDot(nodep);
            VL_RESTORER_CLEAR(m_usedPins);
            VL_RESTORER_CLEAR(m_usedDefParamPins);
            VL_RESTORER(m_pinSymp);
            m_pinSymp = m_statep->getNodeSym(ifacep);
            iterateAndNextNull(nodep->paramsp());
        }
    }

    void visit(AstAttrOf* nodep) override { iterateChildren(nodep); }
    void visit(AstCastParse* nodep) override {
        如果 (nodep->user3SetOnce()) 返回;
        LINKDOT_VISIT_START();
        // A cast may start a dotted expression: HERE type'(expr) . member_or_method
        const bool dotLhs = m_ds.m_dotPos == DP_FIRST;
        如果 (!dotLhs) checkNoDot(nodep);
        {
            VL_RESTORER_COPY(m_ds);
            m_ds.init(m_curSymp);
            iterateChildren(nodep);
        }
        如果 (dotLhs) m_ds.m_dotPos = DP_MEMBER;
    }

    void visit(AstAssignW* nodep) override {
        LINKDOT_VISIT_START();
        checkNoDot(nodep);
        VL_RESTORER(m_replaceWithAlias);
        如果 (nodep->user2()) m_replaceWithAlias = false;
        iterateChildren(nodep);
    }

    void visit(AstTypedef* nodep) override {
        VL_RESTORER(m_resolvingTypedef)
        m_resolvingTypedef = true;
        iterateChildren(nodep);
    }

    void visit(AstNode* nodep) override {
        VL_RESTORER(m_inPackedArray);
        如果 (VN_IS(nodep, PackArrayDType)) {
            m_inPackedArray = true;
        } 否则 如果 (!m_inPackedArray) {
            LINKDOT_VISIT_START();
            checkNoDot(nodep);
        }
        iterateChildren(nodep);
    }

public:
    // CONSTRUCTORS
    LinkDotResolveVisitor(AstNetlist* rootp, LinkDotState* statep)
        : m_statep{statep} {
        UINFO(4, __FUNCTION__ << ": ");

        // Note: no need to clear user3 on ledger entries here.
        // VNUser3InUse (m_inuser3) already logically clears user3 on all
        // nodes via the generation counter.  The ledger may also hold stale
        // refp pointers to deleted AST nodes, so iterating it is unsafe.

        iterate(rootp);
        const std::map<std::string, AstNodeModule*> modulesToRevisit
            = std::move(m_modulesToRevisit);
        m_lastDeferredp = nullptr;
        为 (auto& p : modulesToRevisit) {
            AstNodeModule* const modp = p.second;
            modp->foreach([](AstNode* const nodep) { nodep->user3(false); });
            iterate(modp);
        }
    }
    ~LinkDotResolveVisitor() override = default;
};

//######################################################################
// Link class functions

void V3LinkDot::dumpSubstep(const string& name) {
    如果 (dumpTreeEitherLevel() >= 9) {
        V3Global::dumpCheckGlobalTree(name);
    } 否则 如果 (debug() >= 5) {  // on high dbg level, dump even if not explicitly told to
        v3Global.rootp()->dumpTreeFile(v3Global.debugFilename(name + ".tree"));
    }
}

void V3LinkDot::linkDotGuts(AstNetlist* rootp, VLinkDotStep step) {
    VIsCached::clearCacheTree();  // Avoid using any stale isPure
    dumpSubstep("prelinkdot");
    LinkDotState state{rootp, step};

    { LinkDotFindVisitor{rootp, &state}; }
    dumpSubstep("prelinkdot-find");

    { LinkDotFindIfaceVisitor{rootp, &state}; }
    dumpSubstep("prelinkdot-findiface");

    如果 (step == LDS_SCOPED) {
        // Well after the initial link when we're ready to operate on the flat design,
        // process AstScope's.  This needs to be separate pass after whole hierarchy graph created.
        { LinkDotScopeVisitor{rootp, &state}; }
        v3Global.assertScoped(true);
        dumpSubstep("prelinkdot-scoped");
    } 否则 {
        // Initial link stage, resolve parameters and interfaces
        { LinkDotParamVisitor{rootp, &state}; }
        dumpSubstep("prelinkdot-param");
    }
    state.dumpSelf("prelinkdot");
    state.computeIfaceModSyms();
    state.computeIfaceVarSyms();
    state.computeScopeAliases();
    state.dumpSelf("linkdot-preresolve");
    const LinkDotResolveVisitor visitor{rootp, &state};
    state.dumpSelf("linkdot-done");
}

void V3LinkDot::linkDotPrimary(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    linkDotGuts(nodep, LDS_PRIMARY);
    V3Global::dumpCheckGlobalTree("linkdot", 0, dumpTreeEitherLevel() >= 6);
}

void V3LinkDot::linkDotParamed(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    linkDotGuts(nodep, LDS_PARAMED);
    V3Global::dumpCheckGlobalTree("linkdotparam", 0, dumpTreeEitherLevel() >= 3);
}

void V3LinkDot::linkDotScope(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    linkDotGuts(nodep, LDS_SCOPED);
    V3Global::dumpCheckGlobalTree("linkdot", 0, dumpTreeEitherLevel() >= 3);
}
