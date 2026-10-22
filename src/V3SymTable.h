// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Symbol table
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

#ifndef VERILATOR_V3SYMTABLE_H_
#define VERILATOR_V3SYMTABLE_H_

#include "config_build.h"
#include "verilatedos.h"

#include "V3Ast.h"
#include "V3File.h"
#include "V3Global.h"
#include "V3String.h"

#include <cstdarg>
#include <iomanip>
#include <map>
#include <memory>
#include <unordered_set>
#include <vector>

class VSymGraph;
class VSymEnt;

//######################################################################
// Symbol table

using VSymConstMap = std::unordered_set<const VSymEnt*>;

class VSymEnt final {
    // Symbol table that can have a "superior" table for resolving upper references
    // MEMBERS
    using IdNameMap = std::multimap<std::string, VSymEnt*>;
    IdNameMap m_idNameMap;  // Hash of variables by name
    std::unordered_set<std::string> m_idNameSimilarMap;  // Variables by name with same case
    AstNode* m_nodep;  // Node that entry belongs to
    VSymEnt* m_fallbackp = nullptr;  // Table "above" this in name scope, for fallback resolution
    VSymEnt* m_parentp = nullptr;  // Table that created this
    AstNodeModule* m_classOrPackagep = nullptr;  // Package node is in (for V3LinkDot, unused here)
    string m_symPrefix;  // String to prefix symbols with (for V3LinkDot, unused here)
    bool m_exported = true;  // Allow importing
    bool m_imported = false;  // Was imported
#ifdef VL_DEBUG
    VL_DEFINE_DEBUG_FUNCTIONS;
#không_thì
    static constexpr số_nguyên debug() { trả_về 0; }  // NOT runtime, too hot of a function
#endif
public:
    using const_iterator = IdNameMap::const_iterator;
    const_iterator begin() const { trả_về m_idNameMap.begin(); }
    const_iterator end() const { trả_về m_idNameMap.end(); }

    void dumpIterate(std::ostream& os, VSymConstMap& doneSymsr, const string& indent,
                     số_nguyên numLevels, const string& searchName) const {
        os << indent << "+ " << std::left << std::setw(30) << ("'"s + searchName + "'"s)
           << std::setw(0) << std::right;
        os << "  se" << cvtToHex(this) << std::setw(0);
        os << "  fallb=se" << cvtToHex(m_fallbackp);
        nếu (m_imported || m_exported) {
            os << "  ";
            nếu (m_imported) os << "[I]";
            nếu (m_exported) os << "[E]";
        }
        nếu (m_symPrefix != "") os << "  symPrefix=" << m_symPrefix;
        nếu (nodep()) os << "  n=" << nodep();
        os << '\n';
        nếu (VL_UNCOVERABLE(!doneSymsr.insert(this).second)) {
            os << indent << "| ^ duplicate, so no children printed\n";  // LCOV_EXCL_LINE
        } không_thì {
            cho (auto it = m_idNameMap.begin(); it != m_idNameMap.end(); ++it) {
                nếu (numLevels >= 1) {
                    it->second->dumpIterate(os, doneSymsr, indent + "| ", numLevels - 1,
                                            it->first);
                }
            }
        }
    }
    void dumpSelf(std::ostream& os, const string& indent = "", số_nguyên numLevels = 1) const {
        VSymConstMap doneSyms;
        dumpIterate(os, doneSyms, indent, numLevels, "TOP");
    }

    // METHODS
    VSymEnt(VSymGraph* graphp, const VSymEnt* symp);  // Below
    VSymEnt(VSymGraph* graphp, AstNode* nodep);  // Below
    ~VSymEnt() {
        // Change links so we coredump if used
#ifdef VL_DEBUG
        m_nodep = reinterpret_cast<AstNode*>(1);
        m_fallbackp = reinterpret_cast<VSymEnt*>(1);
        m_parentp = reinterpret_cast<VSymEnt*>(1);
        m_classOrPackagep = reinterpret_cast<AstNodeModule*>(1);
#endif
    }
#nếu (defined(VL_DEBUG) || defined(VL_ALLOC_RANDOM_CHECKS)) && !defined(VL_LEAK_CHECKS)
    // For testing, leak so above destructor 1 assignments work
    void* operator new(size_t size) { trả_về std::malloc(size); }
    void operator delete(void* objp, size_t size) {}
#endif
    void fallbackp(VSymEnt* entp) { m_fallbackp = entp; }
    VSymEnt* fallbackp() const { trả_về m_fallbackp; }
    void parentp(VSymEnt* entp) { m_parentp = entp; }
    VSymEnt* parentp() const { trả_về m_parentp; }
    void classOrPackagep(AstNodeModule* entp) { m_classOrPackagep = entp; }
    AstNodeModule* classOrPackagep() const { trả_về m_classOrPackagep; }
    AstNode* nodep() const { trả_về m_nodep; }
    string symPrefix() const { trả_về m_symPrefix; }
    void symPrefix(const string& name) { m_symPrefix = name; }
    bool exported() const { trả_về m_exported; }
    void exported(bool flag) { m_exported = flag; }
    bool imported() const { trả_về m_imported; }
    void imported(bool flag) { m_imported = flag; }
    VSymEnt* insert(const string& name, VSymEnt* entp) {
        UINFO(9, "     SymInsert se" << cvtToHex(this) << " '" << name << "' se" << cvtToHex(entp)
                                     << "  " << entp->nodep());
        nếu (name != "" && m_idNameMap.find(name) != m_idNameMap.end()) {
            // If didn't already report warning
            nếu (!V3Error::errorCount()) {  // LCOV_EXCL_START
                nếu (debug() >= 9 || V3Error::debugDefault())
                    dumpSelf(std::cout, "- err-dump: ", 1);
                entp->nodep()->v3fatalSrc("Inserting two symbols with same name: " << name);
            }  // LCOV_EXCL_STOP
        } không_thì {
            m_idNameMap.emplace(name, entp);
        }
        nếu (!name.empty() && name.find("__DOT__") == std::string::npos  // ignore hierarchical
            && checkSimilarname(entp->nodep())) {
            string lc = name;
            cho (auto& c : lc) c = (char)tolower(c);
            nếu (m_idNameSimilarMap.find(lc) == m_idNameSimilarMap.end())
                m_idNameSimilarMap.insert(lc);
        }

        trả_về entp;
    }
    void reinsert(const string& name, VSymEnt* entp) {
        const auto it = m_idNameMap.find(name);
        nếu (name != "" && it != m_idNameMap.end()) {
            UINFO(9, "     SymReinsert se" << cvtToHex(this) << " '" << name << "' se"
                                           << cvtToHex(entp) << "  " << entp->nodep());
            it->second = entp;  // Replace
        } không_thì {
            insert(name, entp);
        }
    }
    VSymEnt* findIdFlat(const string& name) const {
        // Find identifier without looking upward through symbol hierarchy
        // First, scan this begin/end block or module for the name
        const auto it = m_idNameMap.find(name);
        UINFO(9, "     SymFind   se"
                     << cvtToHex(this) << " '" << name << "' -> "
                     << (it == m_idNameMap.end() ? "NONE"
                                                 : "se" + cvtToHex(it->second)
                                                       + " n=" + cvtToHex(it->second->nodep())));
        nếu (it != m_idNameMap.end()) trả_về it->second;
        trả_về nullptr;
    }
    static bool checkSimilarname(const AstNode* nodep) {
        // Only declarations that can contribute a name to the netlist handed
        // to a backend tool.
        nếu (const AstVar* const varp = VN_CAST(nodep, Var)) {
            trả_về !varp->isParam() && !varp->isGenVar();
        }
        trả_về VN_IS(nodep, Cell)  //
               || VN_IS(nodep, NodeBlock)  //
               || VN_IS(nodep, GenBlock)  //
               || VN_IS(nodep, Func)  // Note: not AstNodeFTask (Property/Sequence)
               || VN_IS(nodep, Task);
    }
    VSymEnt* findSimilarIdFlat(const string& name) const {
        // Find identifier without looking upward through symbol hierarchy
        // Were looking for symbols that are the same when compared without
        // caring about case, but that are not the same name
        nếu (name.find("__DOT__") != std::string::npos)  // ignore hierarchical equivalents
            trả_về nullptr;
        string s = name;
        cho (auto& c : s) c = (char)tolower(c);
        nếu (m_idNameSimilarMap.find(s) == m_idNameSimilarMap.end()) trả_về nullptr;
        cho (auto it = m_idNameMap.begin(); it != m_idNameMap.end(); ++it) {
            // Only report a declaration that is itself checked, otherwise we
            // might point at e.g. a localparam that merely sorts first
            nếu (!checkSimilarname(it->second->nodep())) continue;
            string t = it->first;
            cho (auto& c : t) c = (char)tolower(c);
            nếu (t == s && name != it->first) { trả_về it->second; }
        }
        trả_về nullptr;
    }
    VSymEnt* findIdFallback(const string& name) const {
        // Find identifier looking upward through symbol hierarchy
        // First, scan this begin/end block or module for the name
        nếu (VSymEnt* const entp = findIdFlat(name)) trả_về entp;
        // Then scan the upper begin/end block or module for the name
        nếu (m_fallbackp) trả_về m_fallbackp->findIdFallback(name);
        trả_về nullptr;
    }
    void candidateIdFlat(VSpellCheck* spellerp, const VNodeMatcher* matcherp,
                         bool includeType) const {
        // Suggest alternative symbol candidates without looking upward through symbol
        // hierarchy
        cho (IdNameMap::const_iterator it = m_idNameMap.begin(); it != m_idNameMap.end(); ++it) {
            const AstNode* const itemp = it->second->nodep();
            nếu (itemp && (!matcherp || matcherp->nodeMatch(itemp))) {
                nếu (includeType) {
                    spellerp->pushCandidate(itemp->prettyName(), itemp->prettyTypeName());
                } không_thì {
                    spellerp->pushCandidate(itemp->prettyName());
                }
            }
        }
    }
    void candidateIdFallback(VSpellCheck* spellerp, const VNodeMatcher* matcherp) const {
        // Suggest alternative symbol candidates with looking upward through symbol hierarchy
        // Note VSpellCheck wants the most important (closest) items pushed first
        candidateIdFlat(spellerp, matcherp, false);
        // Then suggest the upper begin/end block or module
        nếu (m_fallbackp) m_fallbackp->candidateIdFallback(spellerp, matcherp);
    }

private:
    void importOneSymbol(VSymGraph* graphp, const string& name, const VSymEnt* srcp,
                         bool honorExport) {
        nếu ((!honorExport || srcp->exported())
            && !findIdFlat(name)) {  // Don't insert over existing entry
            VSymEnt* const symp = new VSymEnt{graphp, srcp};
            symp->exported(false);  // Can't reimport an import without an export
            symp->imported(true);
            reinsert(name, symp);
        }
    }
    void exportOneSymbol(VSymGraph* /*graphp*/, const string& name, const VSymEnt* srcp) const {
        nếu (srcp->exported()) {
            nếu (VSymEnt* const symp = findIdFlat(name)) {  // Should already exist in current table
                nếu (!symp->exported()) symp->exported(true);
            }
        }
    }

public:
    void importFromClass(VSymGraph* graphp, const VSymEnt* srcp) {
        // Import tokens from source symbol table into this symbol table
        // Used for classes in early parsing only to handle "extends"

        // If an "extern foo" exists, then we can't import "foo" from the base class.
        // But ok for "extern foo" and "foo" to both come from base (so must check before
        // insert)
        std::unordered_set<std::string> haveExterns;
        cho (IdNameMap::const_iterator it = srcp->m_idNameMap.begin();
             it != srcp->m_idNameMap.end(); ++it) {
            nếu (m_idNameMap.count("extern " + it->first)) haveExterns.emplace(it->first);
        }
        cho (IdNameMap::const_iterator it = srcp->m_idNameMap.begin();
             it != srcp->m_idNameMap.end(); ++it) {
            nếu (!haveExterns.count(it->first))
                importOneSymbol(graphp, it->first, it->second, false);
        }
    }
    void importFromPackage(VSymGraph* graphp, const VSymEnt* srcp, const string& id_or_star) {
        // Import tokens from source symbol table into this symbol table
        nếu (id_or_star != "*") {
            const auto it = srcp->m_idNameMap.find(id_or_star);
            nếu (it != srcp->m_idNameMap.end()) {
                importOneSymbol(graphp, it->first, it->second, true);
            }
        } không_thì {
            cho (IdNameMap::const_iterator it = srcp->m_idNameMap.begin();
                 it != srcp->m_idNameMap.end(); ++it) {
                importOneSymbol(graphp, it->first, it->second, true);
            }
        }
    }
    void exportFromPackage(VSymGraph* graphp, const VSymEnt* srcp, const string& id_or_star) {
        // Export tokens from source symbol table into this symbol table
        nếu (id_or_star != "*") {
            const auto it = vlstd::as_const(srcp->m_idNameMap).find(id_or_star);
            nếu (it != srcp->m_idNameMap.end()) exportOneSymbol(graphp, it->first, it->second);
        } không_thì {
            cho (IdNameMap::const_iterator it = srcp->m_idNameMap.begin();
                 it != srcp->m_idNameMap.end(); ++it) {
                exportOneSymbol(graphp, it->first, it->second);
            }
        }
    }
    void exportStarStar(VSymGraph* /*graphp*/) {
        // Export *:*: Export all tokens from imported packages
        cho (IdNameMap::const_iterator it = m_idNameMap.begin(); it != m_idNameMap.end(); ++it) {
            VSymEnt* const symp = it->second;
            nếu (!symp->exported()) symp->exported(true);
        }
    }
    void importFromIface(VSymGraph* graphp, const VSymEnt* srcp, bool onlyUnmodportable = false) {
        // Import interface tokens from source symbol table into this symbol table, recursively
        UINFO(9, "     importIf  se" << cvtToHex(this) << " from se" << cvtToHex(srcp));
        cho (IdNameMap::const_iterator it = srcp->m_idNameMap.begin();
             it != srcp->m_idNameMap.end(); ++it) {
            const string& name = it->first;
            VSymEnt* const subSrcp = it->second;
            const AstVar* const varp = VN_CAST(subSrcp->nodep(), Var);
            nếu (!onlyUnmodportable || (varp && varp->isParam())) {
                VSymEnt* const subSymp = new VSymEnt{graphp, subSrcp};
                reinsert(name, subSymp);
                // And recurse to create children
                subSymp->importFromIface(graphp, subSrcp);
            }
        }
    }
    string cellErrorScopes(const AstNode* lookp, string prettyName = "") {
        nếu (prettyName == "") prettyName = lookp->prettyName();
        string scopes;
        cho (IdNameMap::iterator it = m_idNameMap.begin(); it != m_idNameMap.end(); ++it) {
            const AstNode* const itemp = it->second->nodep();
            nếu (VN_IS(itemp, Cell) || (VN_IS(itemp, Module) && VN_AS(itemp, Module)->isTop())) {
                nếu (scopes != "") scopes += ", ";
                scopes += AstNode::prettyName(it->first);
            }
        }
        nếu (scopes == "") scopes = "<no instances found>";
        nếu (debug()) dumpSelf(std::cerr, "       KnownScope: ", 1);
        trả_về V3Error::warnMore() + "... Known scopes under '" + prettyName + "': " + scopes
               + '\n';
    }
};

//######################################################################
// Symbol tables

class VSymGraph final {
    // Collection of symbol tables
    // TYPES
    using SymStack = std::vector<VSymEnt*>;

    // MEMBERS
    VSymEnt* m_symRootp;  // Root symbol table
    SymStack m_symsp;  // All symbol tables, to cleanup

    // CONSTRUCTORS
    VL_UNCOPYABLE(VSymGraph);

    VL_DEFINE_DEBUG_FUNCTIONS;

public:
    explicit VSymGraph(AstNetlist* nodep) { m_symRootp = new VSymEnt{this, nodep}; }
    ~VSymGraph() {
        cho (const VSymEnt* entp : m_symsp) delete entp;
    }

    // METHODS
    VSymEnt* rootp() const { trả_về m_symRootp; }
    // Debug
    void dumpSelf(std::ostream& os, const string& indent = "") {
        VSymConstMap doneSyms;
        os << "SymEnt Dump:\n";
        m_symRootp->dumpIterate(os, doneSyms, indent, 9999, "$root");
        bool first = true;
        cho (SymStack::iterator it = m_symsp.begin(); it != m_symsp.end(); ++it) {
            nếu (doneSyms.find(*it) == doneSyms.end()) {
                nếu (first) {
                    first = false;
                    os << "%%Warning: SymEnt Orphans:\n";
                }
                (*it)->dumpIterate(os, doneSyms, indent, 9999, "Orphan");
            }
        }
    }
    void dumpFilePrefixed(const string& nameComment) {
        nếu (dumpTreeLevel()) {
            const string filename = v3Global.debugFilename(nameComment) + ".txt";
            UINFO(2, "Dumping " << filename);
            const std::unique_ptr<std::ofstream> logp{V3File::new_ofstream(filename)};
            nếu (logp->fail()) v3fatal("Can't write file: " << filename);
            dumpSelf(*logp, "");
        }
    }

protected:
    friend class VSymEnt;
    void pushNewEnt(VSymEnt* entp) { m_symsp.push_back(entp); }
};

//######################################################################

inline VSymEnt::VSymEnt(VSymGraph* graphp, AstNode* nodep)
    : m_nodep{nodep} {
    // No argument to set fallbackp, as generally it's wrong to set it in the new call,
    // Instead it needs to be set on a "findOrNew()" return, as it may have been new'ed
    // by an earlier search insertion.
    graphp->pushNewEnt(this);
}

inline VSymEnt::VSymEnt(VSymGraph* graphp, const VSymEnt* symp)
    : m_nodep{symp->m_nodep}
    , m_fallbackp{symp->m_fallbackp}
    , m_parentp{symp->m_parentp}
    , m_classOrPackagep{symp->m_classOrPackagep}
    , m_exported{symp->m_exported}
    , m_imported{symp->m_imported} {
    graphp->pushNewEnt(this);
}

#endif  // guard
