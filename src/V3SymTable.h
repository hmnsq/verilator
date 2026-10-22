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

#अगर_अघोषित VERILATOR_V3SYMTABLE_H_
#घोषणा VERILATOR_V3SYMTABLE_H_

#समावेश "config_build.h"
#समावेश "verilatedos.h"

#समावेश "V3Ast.h"
#समावेश "V3File.h"
#समावेश "V3Global.h"
#समावेश "V3String.h"

#समावेश <cstdarg>
#समावेश <iomanip>
#समावेश <map>
#समावेश <memory>
#समावेश <unordered_set>
#समावेश <vector>

class VSymGraph;
class VSymEnt;

//######################################################################
// Symbol table

using VSymConstMap = std::unordered_set<स्थिर VSymEnt*>;

class VSymEnt final {
    // Symbol table that can have a "superior" table for resolving upper references
    // MEMBERS
    using IdNameMap = std::multimap<std::string, VSymEnt*>;
    IdNameMap m_idNameMap;  // Hash of variables by name
    std::unordered_set<std::string> m_idNameSimilarMap;  // Variables by name with same case
    Ast_गाँठ* m_nodep;  // Node that entry belongs to
    VSymEnt* m_fallbackp = nullptr;  // Table "above" this in name scope, for fallback resolution
    VSymEnt* m_parentp = nullptr;  // Table that created this
    Ast_गाँठ_Module* m_classOrPackagep = nullptr;  // Package node is in (for V3LinkDot, unused here)
    string m_symPrefix;  // String to prefix symbols with (for V3LinkDot, unused here)
    bool m_exported = true;  // Allow importing
    bool m_imported = false;  // Was imported
#अगर_घोषित VL_DEBUG
    VL_DEFINE_DEBUG_FUNCTIONS;
#अन्यथा
    अटल constexpr पूर्णांक debug() { वापस 0; }  // NOT runtime, too hot of a function
#पूर्ण_अगर
खुला:
    using const_iterator = IdNameMap::const_iterator;
    const_iterator begin() स्थिर { वापस m_idNameMap.begin(); }
    const_iterator end() स्थिर { वापस m_idNameMap.end(); }

    व्योम dumpIterate(std::ostream& os, VSymConstMap& doneSymsr, स्थिर string& indent,
                     पूर्णांक अंक_Levels, स्थिर string& खोज_Name) स्थिर {
        os << indent << "+ " << std::बायाँ << std::setw(30) << ("'"s + खोज_Name + "'"s)
           << std::setw(0) << std::दायाँ;
        os << "  se" << cvtToHex(this) << std::setw(0);
        os << "  fallb=se" << cvtToHex(m_fallbackp);
        अगर (m_imported || m_exported) {
            os << "  ";
            अगर (m_imported) os << "[I]";
            अगर (m_exported) os << "[E]";
        }
        अगर (m_symPrefix != "") os << "  symPrefix=" << m_symPrefix;
        अगर (nodep()) os << "  n=" << nodep();
        os << '\n';
        अगर (VL_UNCOVERABLE(!doneSymsr.जोड़ो(this).second)) {
            os << indent << "| ^ duplicate, so no children printed\n";  // LCOV_EXCL_LINE
        } अन्यथा {
            क्रम (स्वतः it = m_idNameMap.begin(); it != m_idNameMap.end(); ++it) {
                अगर (अंक_Levels >= 1) {
                    it->second->dumpIterate(os, doneSymsr, indent + "| ", अंक_Levels - 1,
                                            it->first);
                }
            }
        }
    }
    व्योम dumpSelf(std::ostream& os, स्थिर string& indent = "", पूर्णांक अंक_Levels = 1) स्थिर {
        VSymConstMap doneSyms;
        dumpIterate(os, doneSyms, indent, अंक_Levels, "TOP");
    }

    // METHODS
    VSymEnt(VSymGraph* graphp, स्थिर VSymEnt* symp);  // Below
    VSymEnt(VSymGraph* graphp, Ast_गाँठ* nodep);  // Below
    ~VSymEnt() {
        // Change links so we coredump if used
#अगर_घोषित VL_DEBUG
        m_nodep = reinterpret_cast<Ast_गाँठ*>(1);
        m_fallbackp = reinterpret_cast<VSymEnt*>(1);
        m_parentp = reinterpret_cast<VSymEnt*>(1);
        m_classOrPackagep = reinterpret_cast<Ast_गाँठ_Module*>(1);
#पूर्ण_अगर
    }
#अगर (defined(VL_DEBUG) || defined(VL_ALLOC_RANDOM_CHECKS)) && !defined(VL_LEAK_CHECKS)
    // For testing, leak so above destructor 1 assignments work
    व्योम* चालक new(माप_प्रकार आकार) { वापस std::दो_स्मृति(आकार); }
    व्योम चालक delete(व्योम* objp, माप_प्रकार आकार) {}
#पूर्ण_अगर
    व्योम fallbackp(VSymEnt* entp) { m_fallbackp = entp; }
    VSymEnt* fallbackp() स्थिर { वापस m_fallbackp; }
    व्योम parentp(VSymEnt* entp) { m_parentp = entp; }
    VSymEnt* parentp() स्थिर { वापस m_parentp; }
    व्योम classOrPackagep(Ast_गाँठ_Module* entp) { m_classOrPackagep = entp; }
    Ast_गाँठ_Module* classOrPackagep() स्थिर { वापस m_classOrPackagep; }
    Ast_गाँठ* nodep() स्थिर { वापस m_nodep; }
    string symPrefix() स्थिर { वापस m_symPrefix; }
    व्योम symPrefix(स्थिर string& name) { m_symPrefix = name; }
    bool exported() स्थिर { वापस m_exported; }
    व्योम exported(bool flag) { m_exported = flag; }
    bool imported() स्थिर { वापस m_imported; }
    व्योम imported(bool flag) { m_imported = flag; }
    VSymEnt* जोड़ो(स्थिर string& name, VSymEnt* entp) {
        UINFO(9, "     SymInsert se" << cvtToHex(this) << " '" << name << "' se" << cvtToHex(entp)
                                     << "  " << entp->nodep());
        अगर (name != "" && m_idNameMap.find(name) != m_idNameMap.end()) {
            // If didn't already report warning
            अगर (!V3Error::error_गिनती()) {  // LCOV_EXCL_START
                अगर (debug() >= 9 || V3Error::debugDefault())
                    dumpSelf(std::cout, "- err-dump: ", 1);
                entp->nodep()->v3fatalSrc("Inserting two symbols with same name: " << name);
            }  // LCOV_EXCL_STOP
        } अन्यथा {
            m_idNameMap.emplace(name, entp);
        }
        अगर (!name.empty() && name.find("__DOT__") == std::string::npos  // ignore hierarchical
            && checkSimilarname(entp->nodep())) {
            string lc = name;
            क्रम (स्वतः& c : lc) c = (अक्षर)छोटे(c);
            अगर (m_idNameSimilarMap.find(lc) == m_idNameSimilarMap.end())
                m_idNameSimilarMap.जोड़ो(lc);
        }

        वापस entp;
    }
    व्योम reinsert(स्थिर string& name, VSymEnt* entp) {
        स्थिर स्वतः it = m_idNameMap.find(name);
        अगर (name != "" && it != m_idNameMap.end()) {
            UINFO(9, "     SymReinsert se" << cvtToHex(this) << " '" << name << "' se"
                                           << cvtToHex(entp) << "  " << entp->nodep());
            it->second = entp;  // Replace
        } अन्यथा {
            जोड़ो(name, entp);
        }
    }
    VSymEnt* findIdFlat(स्थिर string& name) स्थिर {
        // Find identifier without looking upward through symbol hierarchy
        // First, scan this begin/end block or module for the name
        स्थिर स्वतः it = m_idNameMap.find(name);
        UINFO(9, "     SymFind   se"
                     << cvtToHex(this) << " '" << name << "' -> "
                     << (it == m_idNameMap.end() ? "NONE"
                                                 : "se" + cvtToHex(it->second)
                                                       + " n=" + cvtToHex(it->second->nodep())));
        अगर (it != m_idNameMap.end()) वापस it->second;
        वापस nullptr;
    }
    अटल bool checkSimilarname(स्थिर Ast_गाँठ* nodep) {
        // Only declarations that can contribute a name to the netlist handed
        // to a backend tool.
        अगर (स्थिर AstVar* स्थिर varp = VN_CAST(nodep, Var)) {
            वापस !varp->isParam() && !varp->isGenVar();
        }
        वापस VN_IS(nodep, Cell)  //
               || VN_IS(nodep, गाँठ_Block)  //
               || VN_IS(nodep, GenBlock)  //
               || VN_IS(nodep, Func)  // Note: not AstNodeFTask (Property/Sequence)
               || VN_IS(nodep, Task);
    }
    VSymEnt* findSimilarIdFlat(स्थिर string& name) स्थिर {
        // Find identifier without looking upward through symbol hierarchy
        // Were looking for symbols that are the same when compared without
        // caring about case, but that are not the same name
        अगर (name.find("__DOT__") != std::string::npos)  // ignore hierarchical equivalents
            वापस nullptr;
        string s = name;
        क्रम (स्वतः& c : s) c = (अक्षर)छोटे(c);
        अगर (m_idNameSimilarMap.find(s) == m_idNameSimilarMap.end()) वापस nullptr;
        क्रम (स्वतः it = m_idNameMap.begin(); it != m_idNameMap.end(); ++it) {
            // Only report a declaration that is itself checked, otherwise we
            // might point at e.g. a localparam that merely sorts first
            अगर (!checkSimilarname(it->second->nodep())) जारी;
            string t = it->first;
            क्रम (स्वतः& c : t) c = (अक्षर)छोटे(c);
            अगर (t == s && name != it->first) { वापस it->second; }
        }
        वापस nullptr;
    }
    VSymEnt* findIdFallback(स्थिर string& name) स्थिर {
        // Find identifier looking upward through symbol hierarchy
        // First, scan this begin/end block or module for the name
        अगर (VSymEnt* स्थिर entp = findIdFlat(name)) वापस entp;
        // Then scan the upper begin/end block or module for the name
        अगर (m_fallbackp) वापस m_fallbackp->findIdFallback(name);
        वापस nullptr;
    }
    व्योम candidateIdFlat(VSpellCheck* spellerp, स्थिर VNodeMatcher* matcherp,
                         bool includeType) स्थिर {
        // Suggest alternative symbol candidates without looking upward through symbol
        // hierarchy
        क्रम (IdNameMap::const_iterator it = m_idNameMap.begin(); it != m_idNameMap.end(); ++it) {
            स्थिर Ast_गाँठ* स्थिर itemp = it->second->nodep();
            अगर (itemp && (!matcherp || matcherp->गाँठ_Match(itemp))) {
                अगर (includeType) {
                    spellerp->धकेलो_Candidate(itemp->prettyName(), itemp->prettyTypeName());
                } अन्यथा {
                    spellerp->धकेलो_Candidate(itemp->prettyName());
                }
            }
        }
    }
    व्योम candidateIdFallback(VSpellCheck* spellerp, स्थिर VNodeMatcher* matcherp) स्थिर {
        // Suggest alternative symbol candidates with looking upward through symbol hierarchy
        // Note VSpellCheck wants the most important (closest) items pushed first
        candidateIdFlat(spellerp, matcherp, false);
        // Then suggest the upper begin/end block or module
        अगर (m_fallbackp) m_fallbackp->candidateIdFallback(spellerp, matcherp);
    }

निजी:
    व्योम import_One_प्रतीक(VSymGraph* graphp, स्थिर string& name, स्थिर VSymEnt* srcp,
                         bool honorExport) {
        अगर ((!honorExport || srcp->exported())
            && !findIdFlat(name)) {  // Don't insert over existing entry
            VSymEnt* स्थिर symp = new VSymEnt{graphp, srcp};
            symp->exported(false);  // Can't reimport an import without an export
            symp->imported(true);
            reinsert(name, symp);
        }
    }
    व्योम export_One_प्रतीक(VSymGraph* /*graphp*/, स्थिर string& name, स्थिर VSymEnt* srcp) स्थिर {
        अगर (srcp->exported()) {
            अगर (VSymEnt* स्थिर symp = findIdFlat(name)) {  // Should already exist in current table
                अगर (!symp->exported()) symp->exported(true);
            }
        }
    }

खुला:
    व्योम importFromClass(VSymGraph* graphp, स्थिर VSymEnt* srcp) {
        // Import tokens from source symbol table into this symbol table
        // Used for classes in early parsing only to handle "extends"

        // If an "extern foo" exists, then we can't import "foo" from the base class.
        // But ok for "extern foo" and "foo" to both come from base (so must check before
        // insert)
        std::unordered_set<std::string> haveExterns;
        क्रम (IdNameMap::const_iterator it = srcp->m_idNameMap.begin();
             it != srcp->m_idNameMap.end(); ++it) {
            अगर (m_idNameMap.गिनती("extern " + it->first)) haveExterns.emplace(it->first);
        }
        क्रम (IdNameMap::const_iterator it = srcp->m_idNameMap.begin();
             it != srcp->m_idNameMap.end(); ++it) {
            अगर (!haveExterns.गिनती(it->first))
                import_One_प्रतीक(graphp, it->first, it->second, false);
        }
    }
    व्योम importFromPackage(VSymGraph* graphp, स्थिर VSymEnt* srcp, स्थिर string& id_or_star) {
        // Import tokens from source symbol table into this symbol table
        अगर (id_or_star != "*") {
            स्थिर स्वतः it = srcp->m_idNameMap.find(id_or_star);
            अगर (it != srcp->m_idNameMap.end()) {
                import_One_प्रतीक(graphp, it->first, it->second, true);
            }
        } अन्यथा {
            क्रम (IdNameMap::const_iterator it = srcp->m_idNameMap.begin();
                 it != srcp->m_idNameMap.end(); ++it) {
                import_One_प्रतीक(graphp, it->first, it->second, true);
            }
        }
    }
    व्योम exportFromPackage(VSymGraph* graphp, स्थिर VSymEnt* srcp, स्थिर string& id_or_star) {
        // Export tokens from source symbol table into this symbol table
        अगर (id_or_star != "*") {
            स्थिर स्वतः it = vlstd::as_const(srcp->m_idNameMap).find(id_or_star);
            अगर (it != srcp->m_idNameMap.end()) export_One_प्रतीक(graphp, it->first, it->second);
        } अन्यथा {
            क्रम (IdNameMap::const_iterator it = srcp->m_idNameMap.begin();
                 it != srcp->m_idNameMap.end(); ++it) {
                export_One_प्रतीक(graphp, it->first, it->second);
            }
        }
    }
    व्योम exportStarStar(VSymGraph* /*graphp*/) {
        // Export *:*: Export all tokens from imported packages
        क्रम (IdNameMap::const_iterator it = m_idNameMap.begin(); it != m_idNameMap.end(); ++it) {
            VSymEnt* स्थिर symp = it->second;
            अगर (!symp->exported()) symp->exported(true);
        }
    }
    व्योम importFromIface(VSymGraph* graphp, स्थिर VSymEnt* srcp, bool onlyUnmodportable = false) {
        // Import interface tokens from source symbol table into this symbol table, recursively
        UINFO(9, "     importIf  se" << cvtToHex(this) << " from se" << cvtToHex(srcp));
        क्रम (IdNameMap::const_iterator it = srcp->m_idNameMap.begin();
             it != srcp->m_idNameMap.end(); ++it) {
            स्थिर string& name = it->first;
            VSymEnt* स्थिर subSrcp = it->second;
            स्थिर AstVar* स्थिर varp = VN_CAST(subSrcp->nodep(), Var);
            अगर (!onlyUnmodportable || (varp && varp->isParam())) {
                VSymEnt* स्थिर subSymp = new VSymEnt{graphp, subSrcp};
                reinsert(name, subSymp);
                // And recurse to create children
                subSymp->importFromIface(graphp, subSrcp);
            }
        }
    }
    string cellErrorScopes(स्थिर Ast_गाँठ* lookp, string prettyName = "") {
        अगर (prettyName == "") prettyName = lookp->prettyName();
        string scopes;
        क्रम (IdNameMap::iterator it = m_idNameMap.begin(); it != m_idNameMap.end(); ++it) {
            स्थिर Ast_गाँठ* स्थिर itemp = it->second->nodep();
            अगर (VN_IS(itemp, Cell) || (VN_IS(itemp, Module) && VN_AS(itemp, Module)->is_शीर्ष())) {
                अगर (scopes != "") scopes += ", ";
                scopes += Ast_गाँठ::prettyName(it->first);
            }
        }
        अगर (scopes == "") scopes = "<no instances found>";
        अगर (debug()) dumpSelf(std::cerr, "       KnownScope: ", 1);
        वापस V3Error::warnMore() + "... Known scopes under '" + prettyName + "': " + scopes
               + '\n';
    }
};

//######################################################################
// Symbol tables

class VSymGraph final {
    // Collection of symbol tables
    // TYPES
    using Sym_ढेर = std::vector<VSymEnt*>;

    // MEMBERS
    VSymEnt* m_symRootp;  // Root symbol table
    Sym_ढेर m_symsp;  // All symbol tables, to cleanup

    // CONSTRUCTORS
    VL_UNCOPYABLE(VSymGraph);

    VL_DEFINE_DEBUG_FUNCTIONS;

खुला:
    explicit VSymGraph(AstNetlist* nodep) { m_symRootp = new VSymEnt{this, nodep}; }
    ~VSymGraph() {
        क्रम (स्थिर VSymEnt* entp : m_symsp) delete entp;
    }

    // METHODS
    VSymEnt* rootp() स्थिर { वापस m_symRootp; }
    // Debug
    व्योम dumpSelf(std::ostream& os, स्थिर string& indent = "") {
        VSymConstMap doneSyms;
        os << "SymEnt Dump:\n";
        m_symRootp->dumpIterate(os, doneSyms, indent, 9999, "$root");
        bool first = true;
        क्रम (Sym_ढेर::iterator it = m_symsp.begin(); it != m_symsp.end(); ++it) {
            अगर (doneSyms.find(*it) == doneSyms.end()) {
                अगर (first) {
                    first = false;
                    os << "%%Warning: SymEnt Orphans:\n";
                }
                (*it)->dumpIterate(os, doneSyms, indent, 9999, "Orphan");
            }
        }
    }
    व्योम dumpFilePrefixed(स्थिर string& nameComment) {
        अगर (dump_वृक्ष_Level()) {
            स्थिर string filename = v3Global.debugFilename(nameComment) + ".txt";
            UINFO(2, "Dumping " << filename);
            स्थिर std::unique_ptr<std::ofstream> logp{V3File::new_ofstream(filename)};
            अगर (logp->fail()) v3fatal("Can't write file: " << filename);
            dumpSelf(*logp, "");
        }
    }

रक्षित:
    मित्र class VSymEnt;
    व्योम धकेलो_New_Ent(VSymEnt* entp) { m_symsp.धकेलो_back(entp); }
};

//######################################################################

अंतरभूत VSymEnt::VSymEnt(VSymGraph* graphp, Ast_गाँठ* nodep)
    : m_nodep{nodep} {
    // No argument to set fallbackp, as generally it's wrong to set it in the new call,
    // Instead it needs to be set on a "findOrNew()" return, as it may have been new'ed
    // by an earlier search insertion.
    graphp->धकेलो_New_Ent(this);
}

अंतरभूत VSymEnt::VSymEnt(VSymGraph* graphp, स्थिर VSymEnt* symp)
    : m_nodep{symp->m_nodep}
    , m_fallbackp{symp->m_fallbackp}
    , m_parentp{symp->m_parentp}
    , m_classOrPackagep{symp->m_classOrPackagep}
    , m_exported{symp->m_exported}
    , m_imported{symp->m_imported} {
    graphp->धकेलो_New_Ent(this);
}

#पूर्ण_अगर  // guard
