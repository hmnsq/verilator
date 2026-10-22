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

#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

#अन्तर्भाव "V3Ast.h"
#अन्तर्भाव "V3File.h"
#अन्तर्भाव "V3Global.h"
#अन्तर्भाव "V3String.h"

#अन्तर्भाव <cstdarg>
#अन्तर्भाव <iomanip>
#अन्तर्भाव <map>
#अन्तर्भाव <memory>
#अन्तर्भाव <unordered_set>
#अन्तर्भाव <vector>

class VSymGraph;
class VSymEnt;

//######################################################################
// Symbol table

using VSymConstMap = std::unordered_set<अचर VSymEnt*>;

class VSymEnt final {
    // Symbol table that can have a "superior" table for resolving upper references
    // MEMBERS
    using IdNameMap = std::multimap<std::string, VSymEnt*>;
    IdNameMap m_idNameMap;  // Hash of variables by name
    std::unordered_set<std::string> m_idNameSimilarMap;  // Variables by name with same case
    Ast_गाँठो* m_nodep;  // Node that entry belongs to
    VSymEnt* m_fallbackp = nullptr;  // Table "above" this in name scope, for fallback resolution
    VSymEnt* m_parentp = nullptr;  // Table that created this
    Ast_गाँठो_Module* m_classOrPackagep = nullptr;  // Package node is in (for V3LinkDot, unused here)
    string m_symPrefix;  // String to prefix symbols with (for V3LinkDot, unused here)
    bool m_exported = true;  // Allow importing
    bool m_imported = false;  // Was imported
#ifdef VL_DEBUG
    VL_DEFINE_DEBUG_FUNCTIONS;
#अन्यथा
    स्थिर constexpr पूर्णाङ्क debug() { फर्काऊ 0; }  // NOT runtime, too hot of a function
#endif
सार्वजनिक:
    using const_iterator = IdNameMap::const_iterator;
    const_iterator begin() अचर { फर्काऊ m_idNameMap.begin(); }
    const_iterator end() अचर { फर्काऊ m_idNameMap.end(); }

    शून्य dumpIterate(std::ostream& os, VSymConstMap& doneSymsr, अचर string& indent,
                     पूर्णाङ्क अङ्क_Levels, अचर string& खोज_Name) अचर {
        os << indent << "+ " << std::बायाँ << std::setw(30) << ("'"s + खोज_Name + "'"s)
           << std::setw(0) << std::दायाँ;
        os << "  se" << cvtToHex(this) << std::setw(0);
        os << "  fallb=se" << cvtToHex(m_fallbackp);
        यदि (m_imported || m_exported) {
            os << "  ";
            यदि (m_imported) os << "[I]";
            यदि (m_exported) os << "[E]";
        }
        यदि (m_symPrefix != "") os << "  symPrefix=" << m_symPrefix;
        यदि (nodep()) os << "  n=" << nodep();
        os << '\n';
        यदि (VL_UNCOVERABLE(!doneSymsr.घुसाउ(this).second)) {
            os << indent << "| ^ duplicate, so no children printed\n";  // LCOV_EXCL_LINE
        } अन्यथा {
            लागि (स्वतः it = m_idNameMap.begin(); it != m_idNameMap.end(); ++it) {
                यदि (अङ्क_Levels >= 1) {
                    it->second->dumpIterate(os, doneSymsr, indent + "| ", अङ्क_Levels - 1,
                                            it->first);
                }
            }
        }
    }
    शून्य dumpSelf(std::ostream& os, अचर string& indent = "", पूर्णाङ्क अङ्क_Levels = 1) अचर {
        VSymConstMap doneSyms;
        dumpIterate(os, doneSyms, indent, अङ्क_Levels, "TOP");
    }

    // METHODS
    VSymEnt(VSymGraph* graphp, अचर VSymEnt* symp);  // Below
    VSymEnt(VSymGraph* graphp, Ast_गाँठो* nodep);  // Below
    ~VSymEnt() {
        // Change links so we coredump if used
#ifdef VL_DEBUG
        m_nodep = reinterpret_cast<Ast_गाँठो*>(1);
        m_fallbackp = reinterpret_cast<VSymEnt*>(1);
        m_parentp = reinterpret_cast<VSymEnt*>(1);
        m_classOrPackagep = reinterpret_cast<Ast_गाँठो_Module*>(1);
#endif
    }
#यदि (defined(VL_DEBUG) || defined(VL_ALLOC_RANDOM_CHECKS)) && !defined(VL_LEAK_CHECKS)
    // For testing, leak so above destructor 1 assignments work
    शून्य* चालक new(माप_प्रकार size) { फर्काऊ std::दो_स्मृति(size); }
    शून्य चालक delete(शून्य* objp, माप_प्रकार size) {}
#endif
    शून्य fallbackp(VSymEnt* entp) { m_fallbackp = entp; }
    VSymEnt* fallbackp() अचर { फर्काऊ m_fallbackp; }
    शून्य parentp(VSymEnt* entp) { m_parentp = entp; }
    VSymEnt* parentp() अचर { फर्काऊ m_parentp; }
    शून्य classOrPackagep(Ast_गाँठो_Module* entp) { m_classOrPackagep = entp; }
    Ast_गाँठो_Module* classOrPackagep() अचर { फर्काऊ m_classOrPackagep; }
    Ast_गाँठो* nodep() अचर { फर्काऊ m_nodep; }
    string symPrefix() अचर { फर्काऊ m_symPrefix; }
    शून्य symPrefix(अचर string& name) { m_symPrefix = name; }
    bool exported() अचर { फर्काऊ m_exported; }
    शून्य exported(bool flag) { m_exported = flag; }
    bool imported() अचर { फर्काऊ m_imported; }
    शून्य imported(bool flag) { m_imported = flag; }
    VSymEnt* घुसाउ(अचर string& name, VSymEnt* entp) {
        UINFO(9, "     SymInsert se" << cvtToHex(this) << " '" << name << "' se" << cvtToHex(entp)
                                     << "  " << entp->nodep());
        यदि (name != "" && m_idNameMap.find(name) != m_idNameMap.end()) {
            // If didn't already report warning
            यदि (!V3Error::error_गन्ती()) {  // LCOV_EXCL_START
                यदि (debug() >= 9 || V3Error::debugDefault())
                    dumpSelf(std::cout, "- err-dump: ", 1);
                entp->nodep()->v3fatalSrc("Inserting two symbols with same name: " << name);
            }  // LCOV_EXCL_STOP
        } अन्यथा {
            m_idNameMap.emplace(name, entp);
        }
        यदि (!name.empty() && name.find("__DOT__") == std::string::npos  // ignore hierarchical
            && checkSimilarname(entp->nodep())) {
            string lc = name;
            लागि (स्वतः& c : lc) c = (वर्ण)छोटे(c);
            यदि (m_idNameSimilarMap.find(lc) == m_idNameSimilarMap.end())
                m_idNameSimilarMap.घुसाउ(lc);
        }

        फर्काऊ entp;
    }
    शून्य reinsert(अचर string& name, VSymEnt* entp) {
        अचर स्वतः it = m_idNameMap.find(name);
        यदि (name != "" && it != m_idNameMap.end()) {
            UINFO(9, "     SymReinsert se" << cvtToHex(this) << " '" << name << "' se"
                                           << cvtToHex(entp) << "  " << entp->nodep());
            it->second = entp;  // Replace
        } अन्यथा {
            घुसाउ(name, entp);
        }
    }
    VSymEnt* findIdFlat(अचर string& name) अचर {
        // Find identifier without looking upward through symbol hierarchy
        // First, scan this begin/end block or module for the name
        अचर स्वतः it = m_idNameMap.find(name);
        UINFO(9, "     SymFind   se"
                     << cvtToHex(this) << " '" << name << "' -> "
                     << (it == m_idNameMap.end() ? "NONE"
                                                 : "se" + cvtToHex(it->second)
                                                       + " n=" + cvtToHex(it->second->nodep())));
        यदि (it != m_idNameMap.end()) फर्काऊ it->second;
        फर्काऊ nullptr;
    }
    स्थिर bool checkSimilarname(अचर Ast_गाँठो* nodep) {
        // Only declarations that can contribute a name to the netlist handed
        // to a backend tool.
        यदि (अचर AstVar* अचर varp = VN_CAST(nodep, Var)) {
            फर्काऊ !varp->isParam() && !varp->isGenVar();
        }
        फर्काऊ VN_IS(nodep, Cell)  //
               || VN_IS(nodep, गाँठो_Block)  //
               || VN_IS(nodep, GenBlock)  //
               || VN_IS(nodep, Func)  // Note: not AstNodeFTask (Property/Sequence)
               || VN_IS(nodep, Task);
    }
    VSymEnt* findSimilarIdFlat(अचर string& name) अचर {
        // Find identifier without looking upward through symbol hierarchy
        // Were looking for symbols that are the same when compared without
        // caring about case, but that are not the same name
        यदि (name.find("__DOT__") != std::string::npos)  // ignore hierarchical equivalents
            फर्काऊ nullptr;
        string s = name;
        लागि (स्वतः& c : s) c = (वर्ण)छोटे(c);
        यदि (m_idNameSimilarMap.find(s) == m_idNameSimilarMap.end()) फर्काऊ nullptr;
        लागि (स्वतः it = m_idNameMap.begin(); it != m_idNameMap.end(); ++it) {
            // Only report a declaration that is itself checked, otherwise we
            // might point at e.g. a localparam that merely sorts first
            यदि (!checkSimilarname(it->second->nodep())) जारी;
            string t = it->first;
            लागि (स्वतः& c : t) c = (वर्ण)छोटे(c);
            यदि (t == s && name != it->first) { फर्काऊ it->second; }
        }
        फर्काऊ nullptr;
    }
    VSymEnt* findIdFallback(अचर string& name) अचर {
        // Find identifier looking upward through symbol hierarchy
        // First, scan this begin/end block or module for the name
        यदि (VSymEnt* अचर entp = findIdFlat(name)) फर्काऊ entp;
        // Then scan the upper begin/end block or module for the name
        यदि (m_fallbackp) फर्काऊ m_fallbackp->findIdFallback(name);
        फर्काऊ nullptr;
    }
    शून्य candidateIdFlat(VSpellCheck* spellerp, अचर VNodeMatcher* matcherp,
                         bool includeType) अचर {
        // Suggest alternative symbol candidates without looking upward through symbol
        // hierarchy
        लागि (IdNameMap::const_iterator it = m_idNameMap.begin(); it != m_idNameMap.end(); ++it) {
            अचर Ast_गाँठो* अचर itemp = it->second->nodep();
            यदि (itemp && (!matcherp || matcherp->गाँठो_Match(itemp))) {
                यदि (includeType) {
                    spellerp->थप_Candidate(itemp->prettyName(), itemp->prettyTypeName());
                } अन्यथा {
                    spellerp->थप_Candidate(itemp->prettyName());
                }
            }
        }
    }
    शून्य candidateIdFallback(VSpellCheck* spellerp, अचर VNodeMatcher* matcherp) अचर {
        // Suggest alternative symbol candidates with looking upward through symbol hierarchy
        // Note VSpellCheck wants the most important (closest) items pushed first
        candidateIdFlat(spellerp, matcherp, false);
        // Then suggest the upper begin/end block or module
        यदि (m_fallbackp) m_fallbackp->candidateIdFallback(spellerp, matcherp);
    }

गुप्त:
    शून्य import_One_चिह्न(VSymGraph* graphp, अचर string& name, अचर VSymEnt* srcp,
                         bool honorExport) {
        यदि ((!honorExport || srcp->exported())
            && !findIdFlat(name)) {  // Don't insert over existing entry
            VSymEnt* अचर symp = new VSymEnt{graphp, srcp};
            symp->exported(false);  // Can't reimport an import without an export
            symp->imported(true);
            reinsert(name, symp);
        }
    }
    शून्य export_One_चिह्न(VSymGraph* /*graphp*/, अचर string& name, अचर VSymEnt* srcp) अचर {
        यदि (srcp->exported()) {
            यदि (VSymEnt* अचर symp = findIdFlat(name)) {  // Should already exist in current table
                यदि (!symp->exported()) symp->exported(true);
            }
        }
    }

सार्वजनिक:
    शून्य importFromClass(VSymGraph* graphp, अचर VSymEnt* srcp) {
        // Import tokens from source symbol table into this symbol table
        // Used for classes in early parsing only to handle "extends"

        // If an "extern foo" exists, then we can't import "foo" from the base class.
        // But ok for "extern foo" and "foo" to both come from base (so must check before
        // insert)
        std::unordered_set<std::string> haveExterns;
        लागि (IdNameMap::const_iterator it = srcp->m_idNameMap.begin();
             it != srcp->m_idNameMap.end(); ++it) {
            यदि (m_idNameMap.गन्ती("extern " + it->first)) haveExterns.emplace(it->first);
        }
        लागि (IdNameMap::const_iterator it = srcp->m_idNameMap.begin();
             it != srcp->m_idNameMap.end(); ++it) {
            यदि (!haveExterns.गन्ती(it->first))
                import_One_चिह्न(graphp, it->first, it->second, false);
        }
    }
    शून्य importFromPackage(VSymGraph* graphp, अचर VSymEnt* srcp, अचर string& id_or_star) {
        // Import tokens from source symbol table into this symbol table
        यदि (id_or_star != "*") {
            अचर स्वतः it = srcp->m_idNameMap.find(id_or_star);
            यदि (it != srcp->m_idNameMap.end()) {
                import_One_चिह्न(graphp, it->first, it->second, true);
            }
        } अन्यथा {
            लागि (IdNameMap::const_iterator it = srcp->m_idNameMap.begin();
                 it != srcp->m_idNameMap.end(); ++it) {
                import_One_चिह्न(graphp, it->first, it->second, true);
            }
        }
    }
    शून्य exportFromPackage(VSymGraph* graphp, अचर VSymEnt* srcp, अचर string& id_or_star) {
        // Export tokens from source symbol table into this symbol table
        यदि (id_or_star != "*") {
            अचर स्वतः it = vlstd::as_const(srcp->m_idNameMap).find(id_or_star);
            यदि (it != srcp->m_idNameMap.end()) export_One_चिह्न(graphp, it->first, it->second);
        } अन्यथा {
            लागि (IdNameMap::const_iterator it = srcp->m_idNameMap.begin();
                 it != srcp->m_idNameMap.end(); ++it) {
                export_One_चिह्न(graphp, it->first, it->second);
            }
        }
    }
    शून्य exportStarStar(VSymGraph* /*graphp*/) {
        // Export *:*: Export all tokens from imported packages
        लागि (IdNameMap::const_iterator it = m_idNameMap.begin(); it != m_idNameMap.end(); ++it) {
            VSymEnt* अचर symp = it->second;
            यदि (!symp->exported()) symp->exported(true);
        }
    }
    शून्य importFromIface(VSymGraph* graphp, अचर VSymEnt* srcp, bool onlyUnmodportable = false) {
        // Import interface tokens from source symbol table into this symbol table, recursively
        UINFO(9, "     importIf  se" << cvtToHex(this) << " from se" << cvtToHex(srcp));
        लागि (IdNameMap::const_iterator it = srcp->m_idNameMap.begin();
             it != srcp->m_idNameMap.end(); ++it) {
            अचर string& name = it->first;
            VSymEnt* अचर subSrcp = it->second;
            अचर AstVar* अचर varp = VN_CAST(subSrcp->nodep(), Var);
            यदि (!onlyUnmodportable || (varp && varp->isParam())) {
                VSymEnt* अचर subSymp = new VSymEnt{graphp, subSrcp};
                reinsert(name, subSymp);
                // And recurse to create children
                subSymp->importFromIface(graphp, subSrcp);
            }
        }
    }
    string cellErrorScopes(अचर Ast_गाँठो* lookp, string prettyName = "") {
        यदि (prettyName == "") prettyName = lookp->prettyName();
        string scopes;
        लागि (IdNameMap::iterator it = m_idNameMap.begin(); it != m_idNameMap.end(); ++it) {
            अचर Ast_गाँठो* अचर itemp = it->second->nodep();
            यदि (VN_IS(itemp, Cell) || (VN_IS(itemp, Module) && VN_AS(itemp, Module)->is_शीर्ष())) {
                यदि (scopes != "") scopes += ", ";
                scopes += Ast_गाँठो::prettyName(it->first);
            }
        }
        यदि (scopes == "") scopes = "<no instances found>";
        यदि (debug()) dumpSelf(std::cerr, "       KnownScope: ", 1);
        फर्काऊ V3Error::warnMore() + "... Known scopes under '" + prettyName + "': " + scopes
               + '\n';
    }
};

//######################################################################
// Symbol tables

class VSymGraph final {
    // Collection of symbol tables
    // TYPES
    using Sym_थाक = std::vector<VSymEnt*>;

    // MEMBERS
    VSymEnt* m_symRootp;  // Root symbol table
    Sym_थाक m_symsp;  // All symbol tables, to cleanup

    // CONSTRUCTORS
    VL_UNCOPYABLE(VSymGraph);

    VL_DEFINE_DEBUG_FUNCTIONS;

सार्वजनिक:
    explicit VSymGraph(AstNetlist* nodep) { m_symRootp = new VSymEnt{this, nodep}; }
    ~VSymGraph() {
        लागि (अचर VSymEnt* entp : m_symsp) delete entp;
    }

    // METHODS
    VSymEnt* rootp() अचर { फर्काऊ m_symRootp; }
    // Debug
    शून्य dumpSelf(std::ostream& os, अचर string& indent = "") {
        VSymConstMap doneSyms;
        os << "SymEnt Dump:\n";
        m_symRootp->dumpIterate(os, doneSyms, indent, 9999, "$root");
        bool first = true;
        लागि (Sym_थाक::iterator it = m_symsp.begin(); it != m_symsp.end(); ++it) {
            यदि (doneSyms.find(*it) == doneSyms.end()) {
                यदि (first) {
                    first = false;
                    os << "%%Warning: SymEnt Orphans:\n";
                }
                (*it)->dumpIterate(os, doneSyms, indent, 9999, "Orphan");
            }
        }
    }
    शून्य dumpFilePrefixed(अचर string& nameComment) {
        यदि (dump_रूख_Level()) {
            अचर string filename = v3Global.debugFilename(nameComment) + ".txt";
            UINFO(2, "Dumping " << filename);
            अचर std::unique_ptr<std::ofstream> logp{V3File::new_ofstream(filename)};
            यदि (logp->fail()) v3fatal("Can't write file: " << filename);
            dumpSelf(*logp, "");
        }
    }

रक्षित:
    मित्र class VSymEnt;
    शून्य थप_New_Ent(VSymEnt* entp) { m_symsp.थप_back(entp); }
};

//######################################################################

अन्तःस्थ VSymEnt::VSymEnt(VSymGraph* graphp, Ast_गाँठो* nodep)
    : m_nodep{nodep} {
    // No argument to set fallbackp, as generally it's wrong to set it in the new call,
    // Instead it needs to be set on a "findOrNew()" return, as it may have been new'ed
    // by an earlier search insertion.
    graphp->थप_New_Ent(this);
}

अन्तःस्थ VSymEnt::VSymEnt(VSymGraph* graphp, अचर VSymEnt* symp)
    : m_nodep{symp->m_nodep}
    , m_fallbackp{symp->m_fallbackp}
    , m_parentp{symp->m_parentp}
    , m_classOrPackagep{symp->m_classOrPackagep}
    , m_exported{symp->m_exported}
    , m_imported{symp->m_imported} {
    graphp->थप_New_Ent(this);
}

#endif  // guard
