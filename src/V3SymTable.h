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

#অন্তর্ভাৱ "config_build.h"
#অন্তর্ভাৱ "verilatedos.h"

#অন্তর্ভাৱ "V3Ast.h"
#অন্তর্ভাৱ "V3File.h"
#অন্তর্ভাৱ "V3Global.h"
#অন্তর্ভাৱ "V3String.h"

#অন্তর্ভাৱ <cstdarg>
#অন্তর্ভাৱ <iomanip>
#অন্তর্ভাৱ <map>
#অন্তর্ভাৱ <memory>
#অন্তর্ভাৱ <unordered_set>
#অন্তর্ভাৱ <vector>

class VSymGraph;
class VSymEnt;

//######################################################################
// Symbol table

using VSymConstMap = std::unordered_set<ধ্রুৱ VSymEnt*>;

class VSymEnt final {
    // Symbol table that can have a "superior" table for resolving upper references
    // MEMBERS
    using IdNameMap = std::multimap<std::string, VSymEnt*>;
    IdNameMap m_idNameMap;  // Hash of variables by name
    std::unordered_set<std::string> m_idNameSimilarMap;  // Variables by name with same case
    Ast_গ্ৰন্থি* m_nodep;  // Node that entry belongs to
    VSymEnt* m_fallbackp = nullptr;  // Table "above" this in name scope, for fallback resolution
    VSymEnt* m_parentp = nullptr;  // Table that created this
    Ast_গ্ৰন্থি_Module* m_classOrPackagep = nullptr;  // Package node is in (for V3LinkDot, unused here)
    string m_symPrefix;  // String to prefix symbols with (for V3LinkDot, unused here)
    bool m_exported = true;  // Allow importing
    bool m_imported = false;  // Was imported
#ifdef VL_DEBUG
    VL_DEFINE_DEBUG_FUNCTIONS;
#অন্যথা
    স্থির constexpr পূর্ণাঙ্ক debug() { ঘূৰাই 0; }  // NOT runtime, too hot of a function
#endif
সার্ৱজনিক:
    using const_iterator = IdNameMap::const_iterator;
    const_iterator begin() ধ্রুৱ { ঘূৰাই m_idNameMap.begin(); }
    const_iterator end() ধ্রুৱ { ঘূৰাই m_idNameMap.end(); }

    শূন্য dumpIterate(std::ostream& os, VSymConstMap& doneSymsr, ধ্রুৱ string& indent,
                     পূর্ণাঙ্ক অঙ্ক_Levels, ধ্রুৱ string& অনুসন্ধান_Name) ধ্রুৱ {
        os << indent << "+ " << std::বাম << std::setw(30) << ("'"s + অনুসন্ধান_Name + "'"s)
           << std::setw(0) << std::ডান;
        os << "  se" << cvtToHex(this) << std::setw(0);
        os << "  fallb=se" << cvtToHex(m_fallbackp);
        যদি (m_imported || m_exported) {
            os << "  ";
            যদি (m_imported) os << "[I]";
            যদি (m_exported) os << "[E]";
        }
        যদি (m_symPrefix != "") os << "  symPrefix=" << m_symPrefix;
        যদি (nodep()) os << "  n=" << nodep();
        os << '\n';
        যদি (VL_UNCOVERABLE(!doneSymsr.ঢোকাও(this).second)) {
            os << indent << "| ^ duplicate, so no children printed\n";  // LCOV_EXCL_LINE
        } অন্যথা {
            বাবে (auto it = m_idNameMap.begin(); it != m_idNameMap.end(); ++it) {
                যদি (অঙ্ক_Levels >= 1) {
                    it->second->dumpIterate(os, doneSymsr, indent + "| ", অঙ্ক_Levels - 1,
                                            it->first);
                }
            }
        }
    }
    শূন্য dumpSelf(std::ostream& os, ধ্রুৱ string& indent = "", পূর্ণাঙ্ক অঙ্ক_Levels = 1) ধ্রুৱ {
        VSymConstMap doneSyms;
        dumpIterate(os, doneSyms, indent, অঙ্ক_Levels, "TOP");
    }

    // METHODS
    VSymEnt(VSymGraph* graphp, ধ্রুৱ VSymEnt* symp);  // Below
    VSymEnt(VSymGraph* graphp, Ast_গ্ৰন্থি* nodep);  // Below
    ~VSymEnt() {
        // Change links so we coredump if used
#ifdef VL_DEBUG
        m_nodep = reinterpret_cast<Ast_গ্ৰন্থি*>(1);
        m_fallbackp = reinterpret_cast<VSymEnt*>(1);
        m_parentp = reinterpret_cast<VSymEnt*>(1);
        m_classOrPackagep = reinterpret_cast<Ast_গ্ৰন্থি_Module*>(1);
#endif
    }
#যদি (defined(VL_DEBUG) || defined(VL_ALLOC_RANDOM_CHECKS)) && !defined(VL_LEAK_CHECKS)
    // For testing, leak so above destructor 1 assignments work
    শূন্য* operator new(size_t size) { ঘূৰাই std::malloc(size); }
    শূন্য operator delete(শূন্য* objp, size_t size) {}
#endif
    শূন্য fallbackp(VSymEnt* entp) { m_fallbackp = entp; }
    VSymEnt* fallbackp() ধ্রুৱ { ঘূৰাই m_fallbackp; }
    শূন্য parentp(VSymEnt* entp) { m_parentp = entp; }
    VSymEnt* parentp() ধ্রুৱ { ঘূৰাই m_parentp; }
    শূন্য classOrPackagep(Ast_গ্ৰন্থি_Module* entp) { m_classOrPackagep = entp; }
    Ast_গ্ৰন্থি_Module* classOrPackagep() ধ্রুৱ { ঘূৰাই m_classOrPackagep; }
    Ast_গ্ৰন্থি* nodep() ধ্রুৱ { ঘূৰাই m_nodep; }
    string symPrefix() ধ্রুৱ { ঘূৰাই m_symPrefix; }
    শূন্য symPrefix(ধ্রুৱ string& name) { m_symPrefix = name; }
    bool exported() ধ্রুৱ { ঘূৰাই m_exported; }
    শূন্য exported(bool flag) { m_exported = flag; }
    bool imported() ধ্রুৱ { ঘূৰাই m_imported; }
    শূন্য imported(bool flag) { m_imported = flag; }
    VSymEnt* ঢোকাও(ধ্রুৱ string& name, VSymEnt* entp) {
        UINFO(9, "     SymInsert se" << cvtToHex(this) << " '" << name << "' se" << cvtToHex(entp)
                                     << "  " << entp->nodep());
        যদি (name != "" && m_idNameMap.find(name) != m_idNameMap.end()) {
            // If didn't already report warning
            যদি (!V3Error::error_গুনতি()) {  // LCOV_EXCL_START
                যদি (debug() >= 9 || V3Error::debugDefault())
                    dumpSelf(std::cout, "- err-dump: ", 1);
                entp->nodep()->v3fatalSrc("Inserting two symbols with same name: " << name);
            }  // LCOV_EXCL_STOP
        } অন্যথা {
            m_idNameMap.emplace(name, entp);
        }
        যদি (!name.empty() && name.find("__DOT__") == std::string::npos  // ignore hierarchical
            && checkSimilarname(entp->nodep())) {
            string lc = name;
            বাবে (auto& c : lc) c = (ৱর্ণ)tolower(c);
            যদি (m_idNameSimilarMap.find(lc) == m_idNameSimilarMap.end())
                m_idNameSimilarMap.ঢোকাও(lc);
        }

        ঘূৰাই entp;
    }
    শূন্য reinsert(ধ্রুৱ string& name, VSymEnt* entp) {
        ধ্রুৱ auto it = m_idNameMap.find(name);
        যদি (name != "" && it != m_idNameMap.end()) {
            UINFO(9, "     SymReinsert se" << cvtToHex(this) << " '" << name << "' se"
                                           << cvtToHex(entp) << "  " << entp->nodep());
            it->second = entp;  // Replace
        } অন্যথা {
            ঢোকাও(name, entp);
        }
    }
    VSymEnt* findIdFlat(ধ্রুৱ string& name) ধ্রুৱ {
        // Find identifier without looking upward through symbol hierarchy
        // First, scan this begin/end block or module for the name
        ধ্রুৱ auto it = m_idNameMap.find(name);
        UINFO(9, "     SymFind   se"
                     << cvtToHex(this) << " '" << name << "' -> "
                     << (it == m_idNameMap.end() ? "NONE"
                                                 : "se" + cvtToHex(it->second)
                                                       + " n=" + cvtToHex(it->second->nodep())));
        যদি (it != m_idNameMap.end()) ঘূৰাই it->second;
        ঘূৰাই nullptr;
    }
    স্থির bool checkSimilarname(ধ্রুৱ Ast_গ্ৰন্থি* nodep) {
        // Only declarations that can contribute a name to the netlist handed
        // to a backend tool.
        যদি (ধ্রুৱ AstVar* ধ্রুৱ varp = VN_CAST(nodep, Var)) {
            ঘূৰাই !varp->isParam() && !varp->isGenVar();
        }
        ঘূৰাই VN_IS(nodep, Cell)  //
               || VN_IS(nodep, গ্ৰন্থি_Block)  //
               || VN_IS(nodep, GenBlock)  //
               || VN_IS(nodep, Func)  // Note: not AstNodeFTask (Property/Sequence)
               || VN_IS(nodep, Task);
    }
    VSymEnt* findSimilarIdFlat(ধ্রুৱ string& name) ধ্রুৱ {
        // Find identifier without looking upward through symbol hierarchy
        // Were looking for symbols that are the same when compared without
        // caring about case, but that are not the same name
        যদি (name.find("__DOT__") != std::string::npos)  // ignore hierarchical equivalents
            ঘূৰাই nullptr;
        string s = name;
        বাবে (auto& c : s) c = (ৱর্ণ)tolower(c);
        যদি (m_idNameSimilarMap.find(s) == m_idNameSimilarMap.end()) ঘূৰাই nullptr;
        বাবে (auto it = m_idNameMap.begin(); it != m_idNameMap.end(); ++it) {
            // Only report a declaration that is itself checked, otherwise we
            // might point at e.g. a localparam that merely sorts first
            যদি (!checkSimilarname(it->second->nodep())) অনুৱর্ত;
            string t = it->first;
            বাবে (auto& c : t) c = (ৱর্ণ)tolower(c);
            যদি (t == s && name != it->first) { ঘূৰাই it->second; }
        }
        ঘূৰাই nullptr;
    }
    VSymEnt* findIdFallback(ধ্রুৱ string& name) ধ্রুৱ {
        // Find identifier looking upward through symbol hierarchy
        // First, scan this begin/end block or module for the name
        যদি (VSymEnt* ধ্রুৱ entp = findIdFlat(name)) ঘূৰাই entp;
        // Then scan the upper begin/end block or module for the name
        যদি (m_fallbackp) ঘূৰাই m_fallbackp->findIdFallback(name);
        ঘূৰাই nullptr;
    }
    শূন্য candidateIdFlat(VSpellCheck* spellerp, ধ্রুৱ VNodeMatcher* matcherp,
                         bool includeType) ধ্রুৱ {
        // Suggest alternative symbol candidates without looking upward through symbol
        // hierarchy
        বাবে (IdNameMap::const_iterator it = m_idNameMap.begin(); it != m_idNameMap.end(); ++it) {
            ধ্রুৱ Ast_গ্ৰন্থি* ধ্রুৱ itemp = it->second->nodep();
            যদি (itemp && (!matcherp || matcherp->গ্ৰন্থি_Match(itemp))) {
                যদি (includeType) {
                    spellerp->ঠেলো_Candidate(itemp->prettyName(), itemp->prettyTypeName());
                } অন্যথা {
                    spellerp->ঠেলো_Candidate(itemp->prettyName());
                }
            }
        }
    }
    শূন্য candidateIdFallback(VSpellCheck* spellerp, ধ্রুৱ VNodeMatcher* matcherp) ধ্রুৱ {
        // Suggest alternative symbol candidates with looking upward through symbol hierarchy
        // Note VSpellCheck wants the most important (closest) items pushed first
        candidateIdFlat(spellerp, matcherp, false);
        // Then suggest the upper begin/end block or module
        যদি (m_fallbackp) m_fallbackp->candidateIdFallback(spellerp, matcherp);
    }

গুপ্ত:
    শূন্য import_One_প্ৰতীক(VSymGraph* graphp, ধ্রুৱ string& name, ধ্রুৱ VSymEnt* srcp,
                         bool honorExport) {
        যদি ((!honorExport || srcp->exported())
            && !findIdFlat(name)) {  // Don't insert over existing entry
            VSymEnt* ধ্রুৱ symp = new VSymEnt{graphp, srcp};
            symp->exported(false);  // Can't reimport an import without an export
            symp->imported(true);
            reinsert(name, symp);
        }
    }
    শূন্য export_One_প্ৰতীক(VSymGraph* /*graphp*/, ধ্রুৱ string& name, ধ্রুৱ VSymEnt* srcp) ধ্রুৱ {
        যদি (srcp->exported()) {
            যদি (VSymEnt* ধ্রুৱ symp = findIdFlat(name)) {  // Should already exist in current table
                যদি (!symp->exported()) symp->exported(true);
            }
        }
    }

সার্ৱজনিক:
    শূন্য importFromClass(VSymGraph* graphp, ধ্রুৱ VSymEnt* srcp) {
        // Import tokens from source symbol table into this symbol table
        // Used for classes in early parsing only to handle "extends"

        // If an "extern foo" exists, then we can't import "foo" from the base class.
        // But ok for "extern foo" and "foo" to both come from base (so must check before
        // insert)
        std::unordered_set<std::string> haveExterns;
        বাবে (IdNameMap::const_iterator it = srcp->m_idNameMap.begin();
             it != srcp->m_idNameMap.end(); ++it) {
            যদি (m_idNameMap.গুনতি("extern " + it->first)) haveExterns.emplace(it->first);
        }
        বাবে (IdNameMap::const_iterator it = srcp->m_idNameMap.begin();
             it != srcp->m_idNameMap.end(); ++it) {
            যদি (!haveExterns.গুনতি(it->first))
                import_One_প্ৰতীক(graphp, it->first, it->second, false);
        }
    }
    শূন্য importFromPackage(VSymGraph* graphp, ধ্রুৱ VSymEnt* srcp, ধ্রুৱ string& id_or_star) {
        // Import tokens from source symbol table into this symbol table
        যদি (id_or_star != "*") {
            ধ্রুৱ auto it = srcp->m_idNameMap.find(id_or_star);
            যদি (it != srcp->m_idNameMap.end()) {
                import_One_প্ৰতীক(graphp, it->first, it->second, true);
            }
        } অন্যথা {
            বাবে (IdNameMap::const_iterator it = srcp->m_idNameMap.begin();
                 it != srcp->m_idNameMap.end(); ++it) {
                import_One_প্ৰতীক(graphp, it->first, it->second, true);
            }
        }
    }
    শূন্য exportFromPackage(VSymGraph* graphp, ধ্রুৱ VSymEnt* srcp, ধ্রুৱ string& id_or_star) {
        // Export tokens from source symbol table into this symbol table
        যদি (id_or_star != "*") {
            ধ্রুৱ auto it = vlstd::as_const(srcp->m_idNameMap).find(id_or_star);
            যদি (it != srcp->m_idNameMap.end()) export_One_প্ৰতীক(graphp, it->first, it->second);
        } অন্যথা {
            বাবে (IdNameMap::const_iterator it = srcp->m_idNameMap.begin();
                 it != srcp->m_idNameMap.end(); ++it) {
                export_One_প্ৰতীক(graphp, it->first, it->second);
            }
        }
    }
    শূন্য exportStarStar(VSymGraph* /*graphp*/) {
        // Export *:*: Export all tokens from imported packages
        বাবে (IdNameMap::const_iterator it = m_idNameMap.begin(); it != m_idNameMap.end(); ++it) {
            VSymEnt* ধ্রুৱ symp = it->second;
            যদি (!symp->exported()) symp->exported(true);
        }
    }
    শূন্য importFromIface(VSymGraph* graphp, ধ্রুৱ VSymEnt* srcp, bool onlyUnmodportable = false) {
        // Import interface tokens from source symbol table into this symbol table, recursively
        UINFO(9, "     importIf  se" << cvtToHex(this) << " from se" << cvtToHex(srcp));
        বাবে (IdNameMap::const_iterator it = srcp->m_idNameMap.begin();
             it != srcp->m_idNameMap.end(); ++it) {
            ধ্রুৱ string& name = it->first;
            VSymEnt* ধ্রুৱ subSrcp = it->second;
            ধ্রুৱ AstVar* ধ্রুৱ varp = VN_CAST(subSrcp->nodep(), Var);
            যদি (!onlyUnmodportable || (varp && varp->isParam())) {
                VSymEnt* ধ্রুৱ subSymp = new VSymEnt{graphp, subSrcp};
                reinsert(name, subSymp);
                // And recurse to create children
                subSymp->importFromIface(graphp, subSrcp);
            }
        }
    }
    string cellErrorScopes(ধ্রুৱ Ast_গ্ৰন্থি* lookp, string prettyName = "") {
        যদি (prettyName == "") prettyName = lookp->prettyName();
        string scopes;
        বাবে (IdNameMap::iterator it = m_idNameMap.begin(); it != m_idNameMap.end(); ++it) {
            ধ্রুৱ Ast_গ্ৰন্থি* ধ্রুৱ itemp = it->second->nodep();
            যদি (VN_IS(itemp, Cell) || (VN_IS(itemp, Module) && VN_AS(itemp, Module)->is_শীৰ্ষ())) {
                যদি (scopes != "") scopes += ", ";
                scopes += Ast_গ্ৰন্থি::prettyName(it->first);
            }
        }
        যদি (scopes == "") scopes = "<no instances found>";
        যদি (debug()) dumpSelf(std::cerr, "       KnownScope: ", 1);
        ঘূৰাই V3Error::warnMore() + "... Known scopes under '" + prettyName + "': " + scopes
               + '\n';
    }
};

//######################################################################
// Symbol tables

class VSymGraph final {
    // Collection of symbol tables
    // TYPES
    using Sym_স্তূপ = std::vector<VSymEnt*>;

    // MEMBERS
    VSymEnt* m_symRootp;  // Root symbol table
    Sym_স্তূপ m_symsp;  // All symbol tables, to cleanup

    // CONSTRUCTORS
    VL_UNCOPYABLE(VSymGraph);

    VL_DEFINE_DEBUG_FUNCTIONS;

সার্ৱজনিক:
    explicit VSymGraph(AstNetlist* nodep) { m_symRootp = new VSymEnt{this, nodep}; }
    ~VSymGraph() {
        বাবে (ধ্রুৱ VSymEnt* entp : m_symsp) delete entp;
    }

    // METHODS
    VSymEnt* rootp() ধ্রুৱ { ঘূৰাই m_symRootp; }
    // Debug
    শূন্য dumpSelf(std::ostream& os, ধ্রুৱ string& indent = "") {
        VSymConstMap doneSyms;
        os << "SymEnt Dump:\n";
        m_symRootp->dumpIterate(os, doneSyms, indent, 9999, "$root");
        bool first = true;
        বাবে (Sym_স্তূপ::iterator it = m_symsp.begin(); it != m_symsp.end(); ++it) {
            যদি (doneSyms.find(*it) == doneSyms.end()) {
                যদি (first) {
                    first = false;
                    os << "%%Warning: SymEnt Orphans:\n";
                }
                (*it)->dumpIterate(os, doneSyms, indent, 9999, "Orphan");
            }
        }
    }
    শূন্য dumpFilePrefixed(ধ্রুৱ string& nameComment) {
        যদি (dump_বৃক্ষ_Level()) {
            ধ্রুৱ string filename = v3Global.debugFilename(nameComment) + ".txt";
            UINFO(2, "Dumping " << filename);
            ধ্রুৱ std::unique_ptr<std::ofstream> logp{V3File::new_ofstream(filename)};
            যদি (logp->fail()) v3fatal("Can't write file: " << filename);
            dumpSelf(*logp, "");
        }
    }

রক্ষিত:
    friend class VSymEnt;
    শূন্য ঠেলো_New_Ent(VSymEnt* entp) { m_symsp.ঠেলো_back(entp); }
};

//######################################################################

অন্তঃস্থ VSymEnt::VSymEnt(VSymGraph* graphp, Ast_গ্ৰন্থি* nodep)
    : m_nodep{nodep} {
    // No argument to set fallbackp, as generally it's wrong to set it in the new call,
    // Instead it needs to be set on a "findOrNew()" return, as it may have been new'ed
    // by an earlier search insertion.
    graphp->ঠেলো_New_Ent(this);
}

অন্তঃস্থ VSymEnt::VSymEnt(VSymGraph* graphp, ধ্রুৱ VSymEnt* symp)
    : m_nodep{symp->m_nodep}
    , m_fallbackp{symp->m_fallbackp}
    , m_parentp{symp->m_parentp}
    , m_classOrPackagep{symp->m_classOrPackagep}
    , m_exported{symp->m_exported}
    , m_imported{symp->m_imported} {
    graphp->ঠেলো_New_Ent(this);
}

#endif  // guard
