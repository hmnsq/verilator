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

#অন্তর্ভাব "config_build.h"
#অন্তর্ভাব "verilatedos.h"

#অন্তর্ভাব "V3Ast.h"
#অন্তর্ভাব "V3File.h"
#অন্তর্ভাব "V3Global.h"
#অন্তর্ভাব "V3String.h"

#অন্তর্ভাব <cstdarg>
#অন্তর্ভাব <iomanip>
#অন্তর্ভাব <map>
#অন্তর্ভাব <memory>
#অন্তর্ভাব <unordered_set>
#অন্তর্ভাব <vector>

class VSymGraph;
class VSymEnt;

//######################################################################
// Symbol table

using VSymConstMap = std::unordered_set<ধ্রুব VSymEnt*>;

class VSymEnt final {
    // Symbol table that can have a "superior" table for resolving upper references
    // MEMBERS
    using IdNameMap = std::multimap<std::string, VSymEnt*>;
    IdNameMap m_idNameMap;  // Hash of variables by name
    std::unordered_set<std::string> m_idNameSimilarMap;  // Variables by name with same case
    Ast_গ্রন্থি* m_nodep;  // Node that entry belongs to
    VSymEnt* m_fallbackp = nullptr;  // Table "above" this in name scope, for fallback resolution
    VSymEnt* m_parentp = nullptr;  // Table that created this
    Ast_গ্রন্থি_Module* m_classOrPackagep = nullptr;  // Package node is in (for V3LinkDot, unused here)
    string m_symPrefix;  // String to prefix symbols with (for V3LinkDot, unused here)
    bool m_exported = true;  // Allow importing
    bool m_imported = false;  // Was imported
#ifdef VL_DEBUG
    VL_DEFINE_DEBUG_FUNCTIONS;
#অন্যথা
    স্থির constexpr পূর্ণাঙ্ক debug() { প্রত্যাগম 0; }  // NOT runtime, too hot of a function
#endif
সার্বজনিক:
    using const_iterator = IdNameMap::const_iterator;
    const_iterator begin() ধ্রুব { প্রত্যাগম m_idNameMap.begin(); }
    const_iterator end() ধ্রুব { প্রত্যাগম m_idNameMap.end(); }

    শূন্য dumpIterate(std::ostream& os, VSymConstMap& doneSymsr, ধ্রুব string& indent,
                     পূর্ণাঙ্ক অঙ্ক_Levels, ধ্রুব string& অনুসন্ধান_Name) ধ্রুব {
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
            প্রতি (auto it = m_idNameMap.begin(); it != m_idNameMap.end(); ++it) {
                যদি (অঙ্ক_Levels >= 1) {
                    it->second->dumpIterate(os, doneSymsr, indent + "| ", অঙ্ক_Levels - 1,
                                            it->first);
                }
            }
        }
    }
    শূন্য dumpSelf(std::ostream& os, ধ্রুব string& indent = "", পূর্ণাঙ্ক অঙ্ক_Levels = 1) ধ্রুব {
        VSymConstMap doneSyms;
        dumpIterate(os, doneSyms, indent, অঙ্ক_Levels, "TOP");
    }

    // METHODS
    VSymEnt(VSymGraph* graphp, ধ্রুব VSymEnt* symp);  // Below
    VSymEnt(VSymGraph* graphp, Ast_গ্রন্থি* nodep);  // Below
    ~VSymEnt() {
        // Change links so we coredump if used
#ifdef VL_DEBUG
        m_nodep = reinterpret_cast<Ast_গ্রন্থি*>(1);
        m_fallbackp = reinterpret_cast<VSymEnt*>(1);
        m_parentp = reinterpret_cast<VSymEnt*>(1);
        m_classOrPackagep = reinterpret_cast<Ast_গ্রন্থি_Module*>(1);
#endif
    }
#যদি (defined(VL_DEBUG) || defined(VL_ALLOC_RANDOM_CHECKS)) && !defined(VL_LEAK_CHECKS)
    // For testing, leak so above destructor 1 assignments work
    শূন্য* operator new(size_t size) { প্রত্যাগম std::malloc(size); }
    শূন্য operator delete(শূন্য* objp, size_t size) {}
#endif
    শূন্য fallbackp(VSymEnt* entp) { m_fallbackp = entp; }
    VSymEnt* fallbackp() ধ্রুব { প্রত্যাগম m_fallbackp; }
    শূন্য parentp(VSymEnt* entp) { m_parentp = entp; }
    VSymEnt* parentp() ধ্রুব { প্রত্যাগম m_parentp; }
    শূন্য classOrPackagep(Ast_গ্রন্থি_Module* entp) { m_classOrPackagep = entp; }
    Ast_গ্রন্থি_Module* classOrPackagep() ধ্রুব { প্রত্যাগম m_classOrPackagep; }
    Ast_গ্রন্থি* nodep() ধ্রুব { প্রত্যাগম m_nodep; }
    string symPrefix() ধ্রুব { প্রত্যাগম m_symPrefix; }
    শূন্য symPrefix(ধ্রুব string& name) { m_symPrefix = name; }
    bool exported() ধ্রুব { প্রত্যাগম m_exported; }
    শূন্য exported(bool flag) { m_exported = flag; }
    bool imported() ধ্রুব { প্রত্যাগম m_imported; }
    শূন্য imported(bool flag) { m_imported = flag; }
    VSymEnt* ঢোকাও(ধ্রুব string& name, VSymEnt* entp) {
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
            প্রতি (auto& c : lc) c = (বর্ণ)tolower(c);
            যদি (m_idNameSimilarMap.find(lc) == m_idNameSimilarMap.end())
                m_idNameSimilarMap.ঢোকাও(lc);
        }

        প্রত্যাগম entp;
    }
    শূন্য reinsert(ধ্রুব string& name, VSymEnt* entp) {
        ধ্রুব auto it = m_idNameMap.find(name);
        যদি (name != "" && it != m_idNameMap.end()) {
            UINFO(9, "     SymReinsert se" << cvtToHex(this) << " '" << name << "' se"
                                           << cvtToHex(entp) << "  " << entp->nodep());
            it->second = entp;  // Replace
        } অন্যথা {
            ঢোকাও(name, entp);
        }
    }
    VSymEnt* findIdFlat(ধ্রুব string& name) ধ্রুব {
        // Find identifier without looking upward through symbol hierarchy
        // First, scan this begin/end block or module for the name
        ধ্রুব auto it = m_idNameMap.find(name);
        UINFO(9, "     SymFind   se"
                     << cvtToHex(this) << " '" << name << "' -> "
                     << (it == m_idNameMap.end() ? "NONE"
                                                 : "se" + cvtToHex(it->second)
                                                       + " n=" + cvtToHex(it->second->nodep())));
        যদি (it != m_idNameMap.end()) প্রত্যাগম it->second;
        প্রত্যাগম nullptr;
    }
    স্থির bool checkSimilarname(ধ্রুব Ast_গ্রন্থি* nodep) {
        // Only declarations that can contribute a name to the netlist handed
        // to a backend tool.
        যদি (ধ্রুব AstVar* ধ্রুব varp = VN_CAST(nodep, Var)) {
            প্রত্যাগম !varp->isParam() && !varp->isGenVar();
        }
        প্রত্যাগম VN_IS(nodep, Cell)  //
               || VN_IS(nodep, গ্রন্থি_Block)  //
               || VN_IS(nodep, GenBlock)  //
               || VN_IS(nodep, Func)  // Note: not AstNodeFTask (Property/Sequence)
               || VN_IS(nodep, Task);
    }
    VSymEnt* findSimilarIdFlat(ধ্রুব string& name) ধ্রুব {
        // Find identifier without looking upward through symbol hierarchy
        // Were looking for symbols that are the same when compared without
        // caring about case, but that are not the same name
        যদি (name.find("__DOT__") != std::string::npos)  // ignore hierarchical equivalents
            প্রত্যাগম nullptr;
        string s = name;
        প্রতি (auto& c : s) c = (বর্ণ)tolower(c);
        যদি (m_idNameSimilarMap.find(s) == m_idNameSimilarMap.end()) প্রত্যাগম nullptr;
        প্রতি (auto it = m_idNameMap.begin(); it != m_idNameMap.end(); ++it) {
            // Only report a declaration that is itself checked, otherwise we
            // might point at e.g. a localparam that merely sorts first
            যদি (!checkSimilarname(it->second->nodep())) অনুবর্ত;
            string t = it->first;
            প্রতি (auto& c : t) c = (বর্ণ)tolower(c);
            যদি (t == s && name != it->first) { প্রত্যাগম it->second; }
        }
        প্রত্যাগম nullptr;
    }
    VSymEnt* findIdFallback(ধ্রুব string& name) ধ্রুব {
        // Find identifier looking upward through symbol hierarchy
        // First, scan this begin/end block or module for the name
        যদি (VSymEnt* ধ্রুব entp = findIdFlat(name)) প্রত্যাগম entp;
        // Then scan the upper begin/end block or module for the name
        যদি (m_fallbackp) প্রত্যাগম m_fallbackp->findIdFallback(name);
        প্রত্যাগম nullptr;
    }
    শূন্য candidateIdFlat(VSpellCheck* spellerp, ধ্রুব VNodeMatcher* matcherp,
                         bool includeType) ধ্রুব {
        // Suggest alternative symbol candidates without looking upward through symbol
        // hierarchy
        প্রতি (IdNameMap::const_iterator it = m_idNameMap.begin(); it != m_idNameMap.end(); ++it) {
            ধ্রুব Ast_গ্রন্থি* ধ্রুব itemp = it->second->nodep();
            যদি (itemp && (!matcherp || matcherp->গ্রন্থি_Match(itemp))) {
                যদি (includeType) {
                    spellerp->ঠেলো_Candidate(itemp->prettyName(), itemp->prettyTypeName());
                } অন্যথা {
                    spellerp->ঠেলো_Candidate(itemp->prettyName());
                }
            }
        }
    }
    শূন্য candidateIdFallback(VSpellCheck* spellerp, ধ্রুব VNodeMatcher* matcherp) ধ্রুব {
        // Suggest alternative symbol candidates with looking upward through symbol hierarchy
        // Note VSpellCheck wants the most important (closest) items pushed first
        candidateIdFlat(spellerp, matcherp, false);
        // Then suggest the upper begin/end block or module
        যদি (m_fallbackp) m_fallbackp->candidateIdFallback(spellerp, matcherp);
    }

গুপ্ত:
    শূন্য import_One_প্রতীক(VSymGraph* graphp, ধ্রুব string& name, ধ্রুব VSymEnt* srcp,
                         bool honorExport) {
        যদি ((!honorExport || srcp->exported())
            && !findIdFlat(name)) {  // Don't insert over existing entry
            VSymEnt* ধ্রুব symp = new VSymEnt{graphp, srcp};
            symp->exported(false);  // Can't reimport an import without an export
            symp->imported(true);
            reinsert(name, symp);
        }
    }
    শূন্য export_One_প্রতীক(VSymGraph* /*graphp*/, ধ্রুব string& name, ধ্রুব VSymEnt* srcp) ধ্রুব {
        যদি (srcp->exported()) {
            যদি (VSymEnt* ধ্রুব symp = findIdFlat(name)) {  // Should already exist in current table
                যদি (!symp->exported()) symp->exported(true);
            }
        }
    }

সার্বজনিক:
    শূন্য importFromClass(VSymGraph* graphp, ধ্রুব VSymEnt* srcp) {
        // Import tokens from source symbol table into this symbol table
        // Used for classes in early parsing only to handle "extends"

        // If an "extern foo" exists, then we can't import "foo" from the base class.
        // But ok for "extern foo" and "foo" to both come from base (so must check before
        // insert)
        std::unordered_set<std::string> haveExterns;
        প্রতি (IdNameMap::const_iterator it = srcp->m_idNameMap.begin();
             it != srcp->m_idNameMap.end(); ++it) {
            যদি (m_idNameMap.গুনতি("extern " + it->first)) haveExterns.emplace(it->first);
        }
        প্রতি (IdNameMap::const_iterator it = srcp->m_idNameMap.begin();
             it != srcp->m_idNameMap.end(); ++it) {
            যদি (!haveExterns.গুনতি(it->first))
                import_One_প্রতীক(graphp, it->first, it->second, false);
        }
    }
    শূন্য importFromPackage(VSymGraph* graphp, ধ্রুব VSymEnt* srcp, ধ্রুব string& id_or_star) {
        // Import tokens from source symbol table into this symbol table
        যদি (id_or_star != "*") {
            ধ্রুব auto it = srcp->m_idNameMap.find(id_or_star);
            যদি (it != srcp->m_idNameMap.end()) {
                import_One_প্রতীক(graphp, it->first, it->second, true);
            }
        } অন্যথা {
            প্রতি (IdNameMap::const_iterator it = srcp->m_idNameMap.begin();
                 it != srcp->m_idNameMap.end(); ++it) {
                import_One_প্রতীক(graphp, it->first, it->second, true);
            }
        }
    }
    শূন্য exportFromPackage(VSymGraph* graphp, ধ্রুব VSymEnt* srcp, ধ্রুব string& id_or_star) {
        // Export tokens from source symbol table into this symbol table
        যদি (id_or_star != "*") {
            ধ্রুব auto it = vlstd::as_const(srcp->m_idNameMap).find(id_or_star);
            যদি (it != srcp->m_idNameMap.end()) export_One_প্রতীক(graphp, it->first, it->second);
        } অন্যথা {
            প্রতি (IdNameMap::const_iterator it = srcp->m_idNameMap.begin();
                 it != srcp->m_idNameMap.end(); ++it) {
                export_One_প্রতীক(graphp, it->first, it->second);
            }
        }
    }
    শূন্য exportStarStar(VSymGraph* /*graphp*/) {
        // Export *:*: Export all tokens from imported packages
        প্রতি (IdNameMap::const_iterator it = m_idNameMap.begin(); it != m_idNameMap.end(); ++it) {
            VSymEnt* ধ্রুব symp = it->second;
            যদি (!symp->exported()) symp->exported(true);
        }
    }
    শূন্য importFromIface(VSymGraph* graphp, ধ্রুব VSymEnt* srcp, bool onlyUnmodportable = false) {
        // Import interface tokens from source symbol table into this symbol table, recursively
        UINFO(9, "     importIf  se" << cvtToHex(this) << " from se" << cvtToHex(srcp));
        প্রতি (IdNameMap::const_iterator it = srcp->m_idNameMap.begin();
             it != srcp->m_idNameMap.end(); ++it) {
            ধ্রুব string& name = it->first;
            VSymEnt* ধ্রুব subSrcp = it->second;
            ধ্রুব AstVar* ধ্রুব varp = VN_CAST(subSrcp->nodep(), Var);
            যদি (!onlyUnmodportable || (varp && varp->isParam())) {
                VSymEnt* ধ্রুব subSymp = new VSymEnt{graphp, subSrcp};
                reinsert(name, subSymp);
                // And recurse to create children
                subSymp->importFromIface(graphp, subSrcp);
            }
        }
    }
    string cellErrorScopes(ধ্রুব Ast_গ্রন্থি* lookp, string prettyName = "") {
        যদি (prettyName == "") prettyName = lookp->prettyName();
        string scopes;
        প্রতি (IdNameMap::iterator it = m_idNameMap.begin(); it != m_idNameMap.end(); ++it) {
            ধ্রুব Ast_গ্রন্থি* ধ্রুব itemp = it->second->nodep();
            যদি (VN_IS(itemp, Cell) || (VN_IS(itemp, Module) && VN_AS(itemp, Module)->is_শীর্ষ())) {
                যদি (scopes != "") scopes += ", ";
                scopes += Ast_গ্রন্থি::prettyName(it->first);
            }
        }
        যদি (scopes == "") scopes = "<no instances found>";
        যদি (debug()) dumpSelf(std::cerr, "       KnownScope: ", 1);
        প্রত্যাগম V3Error::warnMore() + "... Known scopes under '" + prettyName + "': " + scopes
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

সার্বজনিক:
    explicit VSymGraph(AstNetlist* nodep) { m_symRootp = new VSymEnt{this, nodep}; }
    ~VSymGraph() {
        প্রতি (ধ্রুব VSymEnt* entp : m_symsp) delete entp;
    }

    // METHODS
    VSymEnt* rootp() ধ্রুব { প্রত্যাগম m_symRootp; }
    // Debug
    শূন্য dumpSelf(std::ostream& os, ধ্রুব string& indent = "") {
        VSymConstMap doneSyms;
        os << "SymEnt Dump:\n";
        m_symRootp->dumpIterate(os, doneSyms, indent, 9999, "$root");
        bool first = true;
        প্রতি (Sym_স্তূপ::iterator it = m_symsp.begin(); it != m_symsp.end(); ++it) {
            যদি (doneSyms.find(*it) == doneSyms.end()) {
                যদি (first) {
                    first = false;
                    os << "%%Warning: SymEnt Orphans:\n";
                }
                (*it)->dumpIterate(os, doneSyms, indent, 9999, "Orphan");
            }
        }
    }
    শূন্য dumpFilePrefixed(ধ্রুব string& nameComment) {
        যদি (dump_বৃক্ষ_Level()) {
            ধ্রুব string filename = v3Global.debugFilename(nameComment) + ".txt";
            UINFO(2, "Dumping " << filename);
            ধ্রুব std::unique_ptr<std::ofstream> logp{V3File::new_ofstream(filename)};
            যদি (logp->fail()) v3fatal("Can't write file: " << filename);
            dumpSelf(*logp, "");
        }
    }

রক্ষিত:
    friend class VSymEnt;
    শূন্য ঠেলো_New_Ent(VSymEnt* entp) { m_symsp.ঠেলো_back(entp); }
};

//######################################################################

অন্তঃস্থ VSymEnt::VSymEnt(VSymGraph* graphp, Ast_গ্রন্থি* nodep)
    : m_nodep{nodep} {
    // No argument to set fallbackp, as generally it's wrong to set it in the new call,
    // Instead it needs to be set on a "findOrNew()" return, as it may have been new'ed
    // by an earlier search insertion.
    graphp->ঠেলো_New_Ent(this);
}

অন্তঃস্থ VSymEnt::VSymEnt(VSymGraph* graphp, ধ্রুব VSymEnt* symp)
    : m_nodep{symp->m_nodep}
    , m_fallbackp{symp->m_fallbackp}
    , m_parentp{symp->m_parentp}
    , m_classOrPackagep{symp->m_classOrPackagep}
    , m_exported{symp->m_exported}
    , m_imported{symp->m_imported} {
    graphp->ঠেলো_New_Ent(this);
}

#endif  // guard
