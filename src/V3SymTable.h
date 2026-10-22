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

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "V3Ast.h"
#ಅನ್ತರ್ಭಾವ "V3File.h"
#ಅನ್ತರ್ಭಾವ "V3Global.h"
#ಅನ್ತರ್ಭಾವ "V3String.h"

#ಅನ್ತರ್ಭಾವ <cstdarg>
#ಅನ್ತರ್ಭಾವ <iomanip>
#ಅನ್ತರ್ಭಾವ <map>
#ಅನ್ತರ್ಭಾವ <memory>
#ಅನ್ತರ್ಭಾವ <unordered_set>
#ಅನ್ತರ್ಭಾವ <vector>

class VSymGraph;
class VSymEnt;

//######################################################################
// Symbol table

using VSymConstMap = std::unordered_set<ಧ್ರುವ VSymEnt*>;

class VSymEnt final {
    // Symbol table that can have a "superior" table for resolving upper references
    // MEMBERS
    using IdNameMap = std::multimap<std::string, VSymEnt*>;
    IdNameMap m_idNameMap;  // Hash of variables by name
    std::unordered_set<std::string> m_idNameSimilarMap;  // Variables by name with same case
    Ast_ಗಂಟು* m_nodep;  // Node that entry belongs to
    VSymEnt* m_fallbackp = nullptr;  // Table "above" this in name scope, for fallback resolution
    VSymEnt* m_parentp = nullptr;  // Table that created this
    Ast_ಗಂಟು_Module* m_classOrPackagep = nullptr;  // Package node is in (for V3LinkDot, unused here)
    string m_symPrefix;  // String to prefix symbols with (for V3LinkDot, unused here)
    bool m_exported = true;  // Allow importing
    bool m_imported = false;  // Was imported
#ifdef VL_DEBUG
    VL_DEFINE_DEBUG_FUNCTIONS;
#ಅನ್ಯಥಾ
    ಸ್ಥಿರ constexpr ಪೂರ್ಣಾಙ್ಕ debug() { ಪ್ರತ್ಯಾಗಮ 0; }  // NOT runtime, too hot of a function
#endif
ಸಾರ್ವಜನಿಕ:
    using const_iterator = IdNameMap::const_iterator;
    const_iterator begin() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_idNameMap.begin(); }
    const_iterator end() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_idNameMap.end(); }

    ಶೂನ್ಯ dumpIterate(std::ostream& os, VSymConstMap& doneSymsr, ಧ್ರುವ string& indent,
                     ಪೂರ್ಣಾಙ್ಕ ಅಂಕೆ_Levels, ಧ್ರುವ string& ಹುಡುಕು_Name) ಧ್ರುವ {
        os << indent << "+ " << std::ಎಡ << std::setw(30) << ("'"s + ಹುಡುಕು_Name + "'"s)
           << std::setw(0) << std::ಬಲ;
        os << "  se" << cvtToHex(this) << std::setw(0);
        os << "  fallb=se" << cvtToHex(m_fallbackp);
        ಯದಿ (m_imported || m_exported) {
            os << "  ";
            ಯದಿ (m_imported) os << "[I]";
            ಯದಿ (m_exported) os << "[E]";
        }
        ಯದಿ (m_symPrefix != "") os << "  symPrefix=" << m_symPrefix;
        ಯದಿ (nodep()) os << "  n=" << nodep();
        os << '\n';
        ಯದಿ (VL_UNCOVERABLE(!doneSymsr.ಸೇರಿಸು(this).second)) {
            os << indent << "| ^ duplicate, so no children printed\n";  // LCOV_EXCL_LINE
        } ಅನ್ಯಥಾ {
            ಪ್ರತಿ (auto it = m_idNameMap.begin(); it != m_idNameMap.end(); ++it) {
                ಯದಿ (ಅಂಕೆ_Levels >= 1) {
                    it->second->dumpIterate(os, doneSymsr, indent + "| ", ಅಂಕೆ_Levels - 1,
                                            it->first);
                }
            }
        }
    }
    ಶೂನ್ಯ dumpSelf(std::ostream& os, ಧ್ರುವ string& indent = "", ಪೂರ್ಣಾಙ್ಕ ಅಂಕೆ_Levels = 1) ಧ್ರುವ {
        VSymConstMap doneSyms;
        dumpIterate(os, doneSyms, indent, ಅಂಕೆ_Levels, "TOP");
    }

    // METHODS
    VSymEnt(VSymGraph* graphp, ಧ್ರುವ VSymEnt* symp);  // Below
    VSymEnt(VSymGraph* graphp, Ast_ಗಂಟು* nodep);  // Below
    ~VSymEnt() {
        // Change links so we coredump if used
#ifdef VL_DEBUG
        m_nodep = reinterpret_cast<Ast_ಗಂಟು*>(1);
        m_fallbackp = reinterpret_cast<VSymEnt*>(1);
        m_parentp = reinterpret_cast<VSymEnt*>(1);
        m_classOrPackagep = reinterpret_cast<Ast_ಗಂಟು_Module*>(1);
#endif
    }
#ಯದಿ (defined(VL_DEBUG) || defined(VL_ALLOC_RANDOM_CHECKS)) && !defined(VL_LEAK_CHECKS)
    // For testing, leak so above destructor 1 assignments work
    ಶೂನ್ಯ* operator new(size_t size) { ಪ್ರತ್ಯಾಗಮ std::malloc(size); }
    ಶೂನ್ಯ operator delete(ಶೂನ್ಯ* objp, size_t size) {}
#endif
    ಶೂನ್ಯ fallbackp(VSymEnt* entp) { m_fallbackp = entp; }
    VSymEnt* fallbackp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_fallbackp; }
    ಶೂನ್ಯ parentp(VSymEnt* entp) { m_parentp = entp; }
    VSymEnt* parentp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_parentp; }
    ಶೂನ್ಯ classOrPackagep(Ast_ಗಂಟು_Module* entp) { m_classOrPackagep = entp; }
    Ast_ಗಂಟು_Module* classOrPackagep() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_classOrPackagep; }
    Ast_ಗಂಟು* nodep() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_nodep; }
    string symPrefix() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_symPrefix; }
    ಶೂನ್ಯ symPrefix(ಧ್ರುವ string& name) { m_symPrefix = name; }
    bool exported() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_exported; }
    ಶೂನ್ಯ exported(bool flag) { m_exported = flag; }
    bool imported() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_imported; }
    ಶೂನ್ಯ imported(bool flag) { m_imported = flag; }
    VSymEnt* ಸೇರಿಸು(ಧ್ರುವ string& name, VSymEnt* entp) {
        UINFO(9, "     SymInsert se" << cvtToHex(this) << " '" << name << "' se" << cvtToHex(entp)
                                     << "  " << entp->nodep());
        ಯದಿ (name != "" && m_idNameMap.find(name) != m_idNameMap.end()) {
            // If didn't already report warning
            ಯದಿ (!V3Error::error_ಎಣಿಕೆ()) {  // LCOV_EXCL_START
                ಯದಿ (debug() >= 9 || V3Error::debugDefault())
                    dumpSelf(std::cout, "- err-dump: ", 1);
                entp->nodep()->v3fatalSrc("Inserting two symbols with same name: " << name);
            }  // LCOV_EXCL_STOP
        } ಅನ್ಯಥಾ {
            m_idNameMap.emplace(name, entp);
        }
        ಯದಿ (!name.empty() && name.find("__DOT__") == std::string::npos  // ignore hierarchical
            && checkSimilarname(entp->nodep())) {
            string lc = name;
            ಪ್ರತಿ (auto& c : lc) c = (ವರ್ಣ)tolower(c);
            ಯದಿ (m_idNameSimilarMap.find(lc) == m_idNameSimilarMap.end())
                m_idNameSimilarMap.ಸೇರಿಸು(lc);
        }

        ಪ್ರತ್ಯಾಗಮ entp;
    }
    ಶೂನ್ಯ reinsert(ಧ್ರುವ string& name, VSymEnt* entp) {
        ಧ್ರುವ auto it = m_idNameMap.find(name);
        ಯದಿ (name != "" && it != m_idNameMap.end()) {
            UINFO(9, "     SymReinsert se" << cvtToHex(this) << " '" << name << "' se"
                                           << cvtToHex(entp) << "  " << entp->nodep());
            it->second = entp;  // Replace
        } ಅನ್ಯಥಾ {
            ಸೇರಿಸು(name, entp);
        }
    }
    VSymEnt* findIdFlat(ಧ್ರುವ string& name) ಧ್ರುವ {
        // Find identifier without looking upward through symbol hierarchy
        // First, scan this begin/end block or module for the name
        ಧ್ರುವ auto it = m_idNameMap.find(name);
        UINFO(9, "     SymFind   se"
                     << cvtToHex(this) << " '" << name << "' -> "
                     << (it == m_idNameMap.end() ? "NONE"
                                                 : "se" + cvtToHex(it->second)
                                                       + " n=" + cvtToHex(it->second->nodep())));
        ಯದಿ (it != m_idNameMap.end()) ಪ್ರತ್ಯಾಗಮ it->second;
        ಪ್ರತ್ಯಾಗಮ nullptr;
    }
    ಸ್ಥಿರ bool checkSimilarname(ಧ್ರುವ Ast_ಗಂಟು* nodep) {
        // Only declarations that can contribute a name to the netlist handed
        // to a backend tool.
        ಯದಿ (ಧ್ರುವ AstVar* ಧ್ರುವ varp = VN_CAST(nodep, Var)) {
            ಪ್ರತ್ಯಾಗಮ !varp->isParam() && !varp->isGenVar();
        }
        ಪ್ರತ್ಯಾಗಮ VN_IS(nodep, Cell)  //
               || VN_IS(nodep, ಗಂಟು_Block)  //
               || VN_IS(nodep, GenBlock)  //
               || VN_IS(nodep, Func)  // Note: not AstNodeFTask (Property/Sequence)
               || VN_IS(nodep, Task);
    }
    VSymEnt* findSimilarIdFlat(ಧ್ರುವ string& name) ಧ್ರುವ {
        // Find identifier without looking upward through symbol hierarchy
        // Were looking for symbols that are the same when compared without
        // caring about case, but that are not the same name
        ಯದಿ (name.find("__DOT__") != std::string::npos)  // ignore hierarchical equivalents
            ಪ್ರತ್ಯಾಗಮ nullptr;
        string s = name;
        ಪ್ರತಿ (auto& c : s) c = (ವರ್ಣ)tolower(c);
        ಯದಿ (m_idNameSimilarMap.find(s) == m_idNameSimilarMap.end()) ಪ್ರತ್ಯಾಗಮ nullptr;
        ಪ್ರತಿ (auto it = m_idNameMap.begin(); it != m_idNameMap.end(); ++it) {
            // Only report a declaration that is itself checked, otherwise we
            // might point at e.g. a localparam that merely sorts first
            ಯದಿ (!checkSimilarname(it->second->nodep())) ಅನುವರ್ತ;
            string t = it->first;
            ಪ್ರತಿ (auto& c : t) c = (ವರ್ಣ)tolower(c);
            ಯದಿ (t == s && name != it->first) { ಪ್ರತ್ಯಾಗಮ it->second; }
        }
        ಪ್ರತ್ಯಾಗಮ nullptr;
    }
    VSymEnt* findIdFallback(ಧ್ರುವ string& name) ಧ್ರುವ {
        // Find identifier looking upward through symbol hierarchy
        // First, scan this begin/end block or module for the name
        ಯದಿ (VSymEnt* ಧ್ರುವ entp = findIdFlat(name)) ಪ್ರತ್ಯಾಗಮ entp;
        // Then scan the upper begin/end block or module for the name
        ಯದಿ (m_fallbackp) ಪ್ರತ್ಯಾಗಮ m_fallbackp->findIdFallback(name);
        ಪ್ರತ್ಯಾಗಮ nullptr;
    }
    ಶೂನ್ಯ candidateIdFlat(VSpellCheck* spellerp, ಧ್ರುವ VNodeMatcher* matcherp,
                         bool includeType) ಧ್ರುವ {
        // Suggest alternative symbol candidates without looking upward through symbol
        // hierarchy
        ಪ್ರತಿ (IdNameMap::const_iterator it = m_idNameMap.begin(); it != m_idNameMap.end(); ++it) {
            ಧ್ರುವ Ast_ಗಂಟು* ಧ್ರುವ itemp = it->second->nodep();
            ಯದಿ (itemp && (!matcherp || matcherp->ಗಂಟು_Match(itemp))) {
                ಯದಿ (includeType) {
                    spellerp->ತಳ್ಳು_Candidate(itemp->prettyName(), itemp->prettyTypeName());
                } ಅನ್ಯಥಾ {
                    spellerp->ತಳ್ಳು_Candidate(itemp->prettyName());
                }
            }
        }
    }
    ಶೂನ್ಯ candidateIdFallback(VSpellCheck* spellerp, ಧ್ರುವ VNodeMatcher* matcherp) ಧ್ರುವ {
        // Suggest alternative symbol candidates with looking upward through symbol hierarchy
        // Note VSpellCheck wants the most important (closest) items pushed first
        candidateIdFlat(spellerp, matcherp, false);
        // Then suggest the upper begin/end block or module
        ಯದಿ (m_fallbackp) m_fallbackp->candidateIdFallback(spellerp, matcherp);
    }

ಗುಪ್ತ:
    ಶೂನ್ಯ import_One_ಚಿಹ್ನೆ(VSymGraph* graphp, ಧ್ರುವ string& name, ಧ್ರುವ VSymEnt* srcp,
                         bool honorExport) {
        ಯದಿ ((!honorExport || srcp->exported())
            && !findIdFlat(name)) {  // Don't insert over existing entry
            VSymEnt* ಧ್ರುವ symp = new VSymEnt{graphp, srcp};
            symp->exported(false);  // Can't reimport an import without an export
            symp->imported(true);
            reinsert(name, symp);
        }
    }
    ಶೂನ್ಯ export_One_ಚಿಹ್ನೆ(VSymGraph* /*graphp*/, ಧ್ರುವ string& name, ಧ್ರುವ VSymEnt* srcp) ಧ್ರುವ {
        ಯದಿ (srcp->exported()) {
            ಯದಿ (VSymEnt* ಧ್ರುವ symp = findIdFlat(name)) {  // Should already exist in current table
                ಯದಿ (!symp->exported()) symp->exported(true);
            }
        }
    }

ಸಾರ್ವಜನಿಕ:
    ಶೂನ್ಯ importFromClass(VSymGraph* graphp, ಧ್ರುವ VSymEnt* srcp) {
        // Import tokens from source symbol table into this symbol table
        // Used for classes in early parsing only to handle "extends"

        // If an "extern foo" exists, then we can't import "foo" from the base class.
        // But ok for "extern foo" and "foo" to both come from base (so must check before
        // insert)
        std::unordered_set<std::string> haveExterns;
        ಪ್ರತಿ (IdNameMap::const_iterator it = srcp->m_idNameMap.begin();
             it != srcp->m_idNameMap.end(); ++it) {
            ಯದಿ (m_idNameMap.ಎಣಿಕೆ("extern " + it->first)) haveExterns.emplace(it->first);
        }
        ಪ್ರತಿ (IdNameMap::const_iterator it = srcp->m_idNameMap.begin();
             it != srcp->m_idNameMap.end(); ++it) {
            ಯದಿ (!haveExterns.ಎಣಿಕೆ(it->first))
                import_One_ಚಿಹ್ನೆ(graphp, it->first, it->second, false);
        }
    }
    ಶೂನ್ಯ importFromPackage(VSymGraph* graphp, ಧ್ರುವ VSymEnt* srcp, ಧ್ರುವ string& id_or_star) {
        // Import tokens from source symbol table into this symbol table
        ಯದಿ (id_or_star != "*") {
            ಧ್ರುವ auto it = srcp->m_idNameMap.find(id_or_star);
            ಯದಿ (it != srcp->m_idNameMap.end()) {
                import_One_ಚಿಹ್ನೆ(graphp, it->first, it->second, true);
            }
        } ಅನ್ಯಥಾ {
            ಪ್ರತಿ (IdNameMap::const_iterator it = srcp->m_idNameMap.begin();
                 it != srcp->m_idNameMap.end(); ++it) {
                import_One_ಚಿಹ್ನೆ(graphp, it->first, it->second, true);
            }
        }
    }
    ಶೂನ್ಯ exportFromPackage(VSymGraph* graphp, ಧ್ರುವ VSymEnt* srcp, ಧ್ರುವ string& id_or_star) {
        // Export tokens from source symbol table into this symbol table
        ಯದಿ (id_or_star != "*") {
            ಧ್ರುವ auto it = vlstd::as_const(srcp->m_idNameMap).find(id_or_star);
            ಯದಿ (it != srcp->m_idNameMap.end()) export_One_ಚಿಹ್ನೆ(graphp, it->first, it->second);
        } ಅನ್ಯಥಾ {
            ಪ್ರತಿ (IdNameMap::const_iterator it = srcp->m_idNameMap.begin();
                 it != srcp->m_idNameMap.end(); ++it) {
                export_One_ಚಿಹ್ನೆ(graphp, it->first, it->second);
            }
        }
    }
    ಶೂನ್ಯ exportStarStar(VSymGraph* /*graphp*/) {
        // Export *:*: Export all tokens from imported packages
        ಪ್ರತಿ (IdNameMap::const_iterator it = m_idNameMap.begin(); it != m_idNameMap.end(); ++it) {
            VSymEnt* ಧ್ರುವ symp = it->second;
            ಯದಿ (!symp->exported()) symp->exported(true);
        }
    }
    ಶೂನ್ಯ importFromIface(VSymGraph* graphp, ಧ್ರುವ VSymEnt* srcp, bool onlyUnmodportable = false) {
        // Import interface tokens from source symbol table into this symbol table, recursively
        UINFO(9, "     importIf  se" << cvtToHex(this) << " from se" << cvtToHex(srcp));
        ಪ್ರತಿ (IdNameMap::const_iterator it = srcp->m_idNameMap.begin();
             it != srcp->m_idNameMap.end(); ++it) {
            ಧ್ರುವ string& name = it->first;
            VSymEnt* ಧ್ರುವ subSrcp = it->second;
            ಧ್ರುವ AstVar* ಧ್ರುವ varp = VN_CAST(subSrcp->nodep(), Var);
            ಯದಿ (!onlyUnmodportable || (varp && varp->isParam())) {
                VSymEnt* ಧ್ರುವ subSymp = new VSymEnt{graphp, subSrcp};
                reinsert(name, subSymp);
                // And recurse to create children
                subSymp->importFromIface(graphp, subSrcp);
            }
        }
    }
    string cellErrorScopes(ಧ್ರುವ Ast_ಗಂಟು* lookp, string prettyName = "") {
        ಯದಿ (prettyName == "") prettyName = lookp->prettyName();
        string scopes;
        ಪ್ರತಿ (IdNameMap::iterator it = m_idNameMap.begin(); it != m_idNameMap.end(); ++it) {
            ಧ್ರುವ Ast_ಗಂಟು* ಧ್ರುವ itemp = it->second->nodep();
            ಯದಿ (VN_IS(itemp, Cell) || (VN_IS(itemp, Module) && VN_AS(itemp, Module)->is_ತುದಿ())) {
                ಯದಿ (scopes != "") scopes += ", ";
                scopes += Ast_ಗಂಟು::prettyName(it->first);
            }
        }
        ಯದಿ (scopes == "") scopes = "<no instances found>";
        ಯದಿ (debug()) dumpSelf(std::cerr, "       KnownScope: ", 1);
        ಪ್ರತ್ಯಾಗಮ V3Error::warnMore() + "... Known scopes under '" + prettyName + "': " + scopes
               + '\n';
    }
};

//######################################################################
// Symbol tables

class VSymGraph final {
    // Collection of symbol tables
    // TYPES
    using Sym_ರಾಶಿ = std::vector<VSymEnt*>;

    // MEMBERS
    VSymEnt* m_symRootp;  // Root symbol table
    Sym_ರಾಶಿ m_symsp;  // All symbol tables, to cleanup

    // CONSTRUCTORS
    VL_UNCOPYABLE(VSymGraph);

    VL_DEFINE_DEBUG_FUNCTIONS;

ಸಾರ್ವಜನಿಕ:
    explicit VSymGraph(AstNetlist* nodep) { m_symRootp = new VSymEnt{this, nodep}; }
    ~VSymGraph() {
        ಪ್ರತಿ (ಧ್ರುವ VSymEnt* entp : m_symsp) delete entp;
    }

    // METHODS
    VSymEnt* rootp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_symRootp; }
    // Debug
    ಶೂನ್ಯ dumpSelf(std::ostream& os, ಧ್ರುವ string& indent = "") {
        VSymConstMap doneSyms;
        os << "SymEnt Dump:\n";
        m_symRootp->dumpIterate(os, doneSyms, indent, 9999, "$root");
        bool first = true;
        ಪ್ರತಿ (Sym_ರಾಶಿ::iterator it = m_symsp.begin(); it != m_symsp.end(); ++it) {
            ಯದಿ (doneSyms.find(*it) == doneSyms.end()) {
                ಯದಿ (first) {
                    first = false;
                    os << "%%Warning: SymEnt Orphans:\n";
                }
                (*it)->dumpIterate(os, doneSyms, indent, 9999, "Orphan");
            }
        }
    }
    ಶೂನ್ಯ dumpFilePrefixed(ಧ್ರುವ string& nameComment) {
        ಯದಿ (dump_ವೃಕ್ಷ_Level()) {
            ಧ್ರುವ string filename = v3Global.debugFilename(nameComment) + ".txt";
            UINFO(2, "Dumping " << filename);
            ಧ್ರುವ std::unique_ptr<std::ofstream> logp{V3File::new_ofstream(filename)};
            ಯದಿ (logp->fail()) v3fatal("Can't write file: " << filename);
            dumpSelf(*logp, "");
        }
    }

ರಕ್ಷಿತ:
    friend class VSymEnt;
    ಶೂನ್ಯ ತಳ್ಳು_New_Ent(VSymEnt* entp) { m_symsp.ತಳ್ಳು_back(entp); }
};

//######################################################################

ಅನ್ತಃಸ್ಥ VSymEnt::VSymEnt(VSymGraph* graphp, Ast_ಗಂಟು* nodep)
    : m_nodep{nodep} {
    // No argument to set fallbackp, as generally it's wrong to set it in the new call,
    // Instead it needs to be set on a "findOrNew()" return, as it may have been new'ed
    // by an earlier search insertion.
    graphp->ತಳ್ಳು_New_Ent(this);
}

ಅನ್ತಃಸ್ಥ VSymEnt::VSymEnt(VSymGraph* graphp, ಧ್ರುವ VSymEnt* symp)
    : m_nodep{symp->m_nodep}
    , m_fallbackp{symp->m_fallbackp}
    , m_parentp{symp->m_parentp}
    , m_classOrPackagep{symp->m_classOrPackagep}
    , m_exported{symp->m_exported}
    , m_imported{symp->m_imported} {
    graphp->ತಳ್ಳು_New_Ent(this);
}

#endif  // guard
