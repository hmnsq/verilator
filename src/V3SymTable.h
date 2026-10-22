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

using VSymConstMap = std::unordered_set<स्थिरांक VSymEnt*>;

class VSymEnt final {
    // Symbol table that can have a "superior" table for resolving upper references
    // MEMBERS
    using IdNameMap = std::multimap<std::string, VSymEnt*>;
    IdNameMap m_idNameMap;  // Hash of variables by name
    std::unordered_set<std::string> m_idNameSimilarMap;  // Variables by name with same case
    Ast_गाठ* m_nodep;  // Node that entry belongs to
    VSymEnt* m_fallbackp = nullptr;  // Table "above" this in name scope, for fallback resolution
    VSymEnt* m_parentp = nullptr;  // Table that created this
    Ast_गाठ_Module* m_classOrPackagep = nullptr;  // Package node is in (for V3LinkDot, unused here)
    string m_symPrefix;  // String to prefix symbols with (for V3LinkDot, unused here)
    bool m_exported = true;  // Allow importing
    bool m_imported = false;  // Was imported
#ifdef VL_DEBUG
    VL_DEFINE_DEBUG_FUNCTIONS;
#अन्यथा
    स्थिर constexpr पूर्णांक debug() { परत 0; }  // NOT runtime, too hot of a function
#endif
सार्वजनिक:
    using const_iterator = IdNameMap::const_iterator;
    const_iterator begin() स्थिरांक { परत m_idNameMap.begin(); }
    const_iterator end() स्थिरांक { परत m_idNameMap.end(); }

    शून्य dumpIterate(std::ostream& os, VSymConstMap& doneSymsr, स्थिरांक string& indent,
                     पूर्णांक अंक_Levels, स्थिरांक string& शोध_Name) स्थिरांक {
        os << indent << "+ " << std::डावा << std::setw(30) << ("'"s + शोध_Name + "'"s)
           << std::setw(0) << std::उजवा;
        os << "  se" << cvtToHex(this) << std::setw(0);
        os << "  fallb=se" << cvtToHex(m_fallbackp);
        जर (m_imported || m_exported) {
            os << "  ";
            जर (m_imported) os << "[I]";
            जर (m_exported) os << "[E]";
        }
        जर (m_symPrefix != "") os << "  symPrefix=" << m_symPrefix;
        जर (nodep()) os << "  n=" << nodep();
        os << '\n';
        जर (VL_UNCOVERABLE(!doneSymsr.घाला(this).second)) {
            os << indent << "| ^ duplicate, so no children printed\n";  // LCOV_EXCL_LINE
        } अन्यथा {
            साठी (स्वतः it = m_idNameMap.begin(); it != m_idNameMap.end(); ++it) {
                जर (अंक_Levels >= 1) {
                    it->second->dumpIterate(os, doneSymsr, indent + "| ", अंक_Levels - 1,
                                            it->first);
                }
            }
        }
    }
    शून्य dumpSelf(std::ostream& os, स्थिरांक string& indent = "", पूर्णांक अंक_Levels = 1) स्थिरांक {
        VSymConstMap doneSyms;
        dumpIterate(os, doneSyms, indent, अंक_Levels, "TOP");
    }

    // METHODS
    VSymEnt(VSymGraph* graphp, स्थिरांक VSymEnt* symp);  // Below
    VSymEnt(VSymGraph* graphp, Ast_गाठ* nodep);  // Below
    ~VSymEnt() {
        // Change links so we coredump if used
#ifdef VL_DEBUG
        m_nodep = reinterpret_cast<Ast_गाठ*>(1);
        m_fallbackp = reinterpret_cast<VSymEnt*>(1);
        m_parentp = reinterpret_cast<VSymEnt*>(1);
        m_classOrPackagep = reinterpret_cast<Ast_गाठ_Module*>(1);
#endif
    }
#जर (defined(VL_DEBUG) || defined(VL_ALLOC_RANDOM_CHECKS)) && !defined(VL_LEAK_CHECKS)
    // For testing, leak so above destructor 1 assignments work
    शून्य* चालक new(माप_प्रकार size) { परत std::दो_स्मृति(size); }
    शून्य चालक delete(शून्य* objp, माप_प्रकार size) {}
#endif
    शून्य fallbackp(VSymEnt* entp) { m_fallbackp = entp; }
    VSymEnt* fallbackp() स्थिरांक { परत m_fallbackp; }
    शून्य parentp(VSymEnt* entp) { m_parentp = entp; }
    VSymEnt* parentp() स्थिरांक { परत m_parentp; }
    शून्य classOrPackagep(Ast_गाठ_Module* entp) { m_classOrPackagep = entp; }
    Ast_गाठ_Module* classOrPackagep() स्थिरांक { परत m_classOrPackagep; }
    Ast_गाठ* nodep() स्थिरांक { परत m_nodep; }
    string symPrefix() स्थिरांक { परत m_symPrefix; }
    शून्य symPrefix(स्थिरांक string& name) { m_symPrefix = name; }
    bool exported() स्थिरांक { परत m_exported; }
    शून्य exported(bool flag) { m_exported = flag; }
    bool imported() स्थिरांक { परत m_imported; }
    शून्य imported(bool flag) { m_imported = flag; }
    VSymEnt* घाला(स्थिरांक string& name, VSymEnt* entp) {
        UINFO(9, "     SymInsert se" << cvtToHex(this) << " '" << name << "' se" << cvtToHex(entp)
                                     << "  " << entp->nodep());
        जर (name != "" && m_idNameMap.find(name) != m_idNameMap.end()) {
            // If didn't already report warning
            जर (!V3Error::error_मोजणी()) {  // LCOV_EXCL_START
                जर (debug() >= 9 || V3Error::debugDefault())
                    dumpSelf(std::cout, "- err-dump: ", 1);
                entp->nodep()->v3fatalSrc("Inserting two symbols with same name: " << name);
            }  // LCOV_EXCL_STOP
        } अन्यथा {
            m_idNameMap.emplace(name, entp);
        }
        जर (!name.empty() && name.find("__DOT__") == std::string::npos  // ignore hierarchical
            && checkSimilarname(entp->nodep())) {
            string lc = name;
            साठी (स्वतः& c : lc) c = (अक्षर)छोटे(c);
            जर (m_idNameSimilarMap.find(lc) == m_idNameSimilarMap.end())
                m_idNameSimilarMap.घाला(lc);
        }

        परत entp;
    }
    शून्य reinsert(स्थिरांक string& name, VSymEnt* entp) {
        स्थिरांक स्वतः it = m_idNameMap.find(name);
        जर (name != "" && it != m_idNameMap.end()) {
            UINFO(9, "     SymReinsert se" << cvtToHex(this) << " '" << name << "' se"
                                           << cvtToHex(entp) << "  " << entp->nodep());
            it->second = entp;  // Replace
        } अन्यथा {
            घाला(name, entp);
        }
    }
    VSymEnt* findIdFlat(स्थिरांक string& name) स्थिरांक {
        // Find identifier without looking upward through symbol hierarchy
        // First, scan this begin/end block or module for the name
        स्थिरांक स्वतः it = m_idNameMap.find(name);
        UINFO(9, "     SymFind   se"
                     << cvtToHex(this) << " '" << name << "' -> "
                     << (it == m_idNameMap.end() ? "NONE"
                                                 : "se" + cvtToHex(it->second)
                                                       + " n=" + cvtToHex(it->second->nodep())));
        जर (it != m_idNameMap.end()) परत it->second;
        परत nullptr;
    }
    स्थिर bool checkSimilarname(स्थिरांक Ast_गाठ* nodep) {
        // Only declarations that can contribute a name to the netlist handed
        // to a backend tool.
        जर (स्थिरांक AstVar* स्थिरांक varp = VN_CAST(nodep, Var)) {
            परत !varp->isParam() && !varp->isGenVar();
        }
        परत VN_IS(nodep, Cell)  //
               || VN_IS(nodep, गाठ_Block)  //
               || VN_IS(nodep, GenBlock)  //
               || VN_IS(nodep, Func)  // Note: not AstNodeFTask (Property/Sequence)
               || VN_IS(nodep, Task);
    }
    VSymEnt* findSimilarIdFlat(स्थिरांक string& name) स्थिरांक {
        // Find identifier without looking upward through symbol hierarchy
        // Were looking for symbols that are the same when compared without
        // caring about case, but that are not the same name
        जर (name.find("__DOT__") != std::string::npos)  // ignore hierarchical equivalents
            परत nullptr;
        string s = name;
        साठी (स्वतः& c : s) c = (अक्षर)छोटे(c);
        जर (m_idNameSimilarMap.find(s) == m_idNameSimilarMap.end()) परत nullptr;
        साठी (स्वतः it = m_idNameMap.begin(); it != m_idNameMap.end(); ++it) {
            // Only report a declaration that is itself checked, otherwise we
            // might point at e.g. a localparam that merely sorts first
            जर (!checkSimilarname(it->second->nodep())) सुरूठेवा;
            string t = it->first;
            साठी (स्वतः& c : t) c = (अक्षर)छोटे(c);
            जर (t == s && name != it->first) { परत it->second; }
        }
        परत nullptr;
    }
    VSymEnt* findIdFallback(स्थिरांक string& name) स्थिरांक {
        // Find identifier looking upward through symbol hierarchy
        // First, scan this begin/end block or module for the name
        जर (VSymEnt* स्थिरांक entp = findIdFlat(name)) परत entp;
        // Then scan the upper begin/end block or module for the name
        जर (m_fallbackp) परत m_fallbackp->findIdFallback(name);
        परत nullptr;
    }
    शून्य candidateIdFlat(VSpellCheck* spellerp, स्थिरांक VNodeMatcher* matcherp,
                         bool includeType) स्थिरांक {
        // Suggest alternative symbol candidates without looking upward through symbol
        // hierarchy
        साठी (IdNameMap::const_iterator it = m_idNameMap.begin(); it != m_idNameMap.end(); ++it) {
            स्थिरांक Ast_गाठ* स्थिरांक itemp = it->second->nodep();
            जर (itemp && (!matcherp || matcherp->गाठ_Match(itemp))) {
                जर (includeType) {
                    spellerp->ढकला_Candidate(itemp->prettyName(), itemp->prettyTypeName());
                } अन्यथा {
                    spellerp->ढकला_Candidate(itemp->prettyName());
                }
            }
        }
    }
    शून्य candidateIdFallback(VSpellCheck* spellerp, स्थिरांक VNodeMatcher* matcherp) स्थिरांक {
        // Suggest alternative symbol candidates with looking upward through symbol hierarchy
        // Note VSpellCheck wants the most important (closest) items pushed first
        candidateIdFlat(spellerp, matcherp, false);
        // Then suggest the upper begin/end block or module
        जर (m_fallbackp) m_fallbackp->candidateIdFallback(spellerp, matcherp);
    }

खाजगी:
    शून्य import_One_चिन्ह(VSymGraph* graphp, स्थिरांक string& name, स्थिरांक VSymEnt* srcp,
                         bool honorExport) {
        जर ((!honorExport || srcp->exported())
            && !findIdFlat(name)) {  // Don't insert over existing entry
            VSymEnt* स्थिरांक symp = new VSymEnt{graphp, srcp};
            symp->exported(false);  // Can't reimport an import without an export
            symp->imported(true);
            reinsert(name, symp);
        }
    }
    शून्य export_One_चिन्ह(VSymGraph* /*graphp*/, स्थिरांक string& name, स्थिरांक VSymEnt* srcp) स्थिरांक {
        जर (srcp->exported()) {
            जर (VSymEnt* स्थिरांक symp = findIdFlat(name)) {  // Should already exist in current table
                जर (!symp->exported()) symp->exported(true);
            }
        }
    }

सार्वजनिक:
    शून्य importFromClass(VSymGraph* graphp, स्थिरांक VSymEnt* srcp) {
        // Import tokens from source symbol table into this symbol table
        // Used for classes in early parsing only to handle "extends"

        // If an "extern foo" exists, then we can't import "foo" from the base class.
        // But ok for "extern foo" and "foo" to both come from base (so must check before
        // insert)
        std::unordered_set<std::string> haveExterns;
        साठी (IdNameMap::const_iterator it = srcp->m_idNameMap.begin();
             it != srcp->m_idNameMap.end(); ++it) {
            जर (m_idNameMap.मोजणी("extern " + it->first)) haveExterns.emplace(it->first);
        }
        साठी (IdNameMap::const_iterator it = srcp->m_idNameMap.begin();
             it != srcp->m_idNameMap.end(); ++it) {
            जर (!haveExterns.मोजणी(it->first))
                import_One_चिन्ह(graphp, it->first, it->second, false);
        }
    }
    शून्य importFromPackage(VSymGraph* graphp, स्थिरांक VSymEnt* srcp, स्थिरांक string& id_or_star) {
        // Import tokens from source symbol table into this symbol table
        जर (id_or_star != "*") {
            स्थिरांक स्वतः it = srcp->m_idNameMap.find(id_or_star);
            जर (it != srcp->m_idNameMap.end()) {
                import_One_चिन्ह(graphp, it->first, it->second, true);
            }
        } अन्यथा {
            साठी (IdNameMap::const_iterator it = srcp->m_idNameMap.begin();
                 it != srcp->m_idNameMap.end(); ++it) {
                import_One_चिन्ह(graphp, it->first, it->second, true);
            }
        }
    }
    शून्य exportFromPackage(VSymGraph* graphp, स्थिरांक VSymEnt* srcp, स्थिरांक string& id_or_star) {
        // Export tokens from source symbol table into this symbol table
        जर (id_or_star != "*") {
            स्थिरांक स्वतः it = vlstd::as_const(srcp->m_idNameMap).find(id_or_star);
            जर (it != srcp->m_idNameMap.end()) export_One_चिन्ह(graphp, it->first, it->second);
        } अन्यथा {
            साठी (IdNameMap::const_iterator it = srcp->m_idNameMap.begin();
                 it != srcp->m_idNameMap.end(); ++it) {
                export_One_चिन्ह(graphp, it->first, it->second);
            }
        }
    }
    शून्य exportStarStar(VSymGraph* /*graphp*/) {
        // Export *:*: Export all tokens from imported packages
        साठी (IdNameMap::const_iterator it = m_idNameMap.begin(); it != m_idNameMap.end(); ++it) {
            VSymEnt* स्थिरांक symp = it->second;
            जर (!symp->exported()) symp->exported(true);
        }
    }
    शून्य importFromIface(VSymGraph* graphp, स्थिरांक VSymEnt* srcp, bool onlyUnmodportable = false) {
        // Import interface tokens from source symbol table into this symbol table, recursively
        UINFO(9, "     importIf  se" << cvtToHex(this) << " from se" << cvtToHex(srcp));
        साठी (IdNameMap::const_iterator it = srcp->m_idNameMap.begin();
             it != srcp->m_idNameMap.end(); ++it) {
            स्थिरांक string& name = it->first;
            VSymEnt* स्थिरांक subSrcp = it->second;
            स्थिरांक AstVar* स्थिरांक varp = VN_CAST(subSrcp->nodep(), Var);
            जर (!onlyUnmodportable || (varp && varp->isParam())) {
                VSymEnt* स्थिरांक subSymp = new VSymEnt{graphp, subSrcp};
                reinsert(name, subSymp);
                // And recurse to create children
                subSymp->importFromIface(graphp, subSrcp);
            }
        }
    }
    string cellErrorScopes(स्थिरांक Ast_गाठ* lookp, string prettyName = "") {
        जर (prettyName == "") prettyName = lookp->prettyName();
        string scopes;
        साठी (IdNameMap::iterator it = m_idNameMap.begin(); it != m_idNameMap.end(); ++it) {
            स्थिरांक Ast_गाठ* स्थिरांक itemp = it->second->nodep();
            जर (VN_IS(itemp, Cell) || (VN_IS(itemp, Module) && VN_AS(itemp, Module)->is_शीर्ष())) {
                जर (scopes != "") scopes += ", ";
                scopes += Ast_गाठ::prettyName(it->first);
            }
        }
        जर (scopes == "") scopes = "<no instances found>";
        जर (debug()) dumpSelf(std::cerr, "       KnownScope: ", 1);
        परत V3Error::warnMore() + "... Known scopes under '" + prettyName + "': " + scopes
               + '\n';
    }
};

//######################################################################
// Symbol tables

class VSymGraph final {
    // Collection of symbol tables
    // TYPES
    using Sym_चळत = std::vector<VSymEnt*>;

    // MEMBERS
    VSymEnt* m_symRootp;  // Root symbol table
    Sym_चळत m_symsp;  // All symbol tables, to cleanup

    // CONSTRUCTORS
    VL_UNCOPYABLE(VSymGraph);

    VL_DEFINE_DEBUG_FUNCTIONS;

सार्वजनिक:
    explicit VSymGraph(AstNetlist* nodep) { m_symRootp = new VSymEnt{this, nodep}; }
    ~VSymGraph() {
        साठी (स्थिरांक VSymEnt* entp : m_symsp) delete entp;
    }

    // METHODS
    VSymEnt* rootp() स्थिरांक { परत m_symRootp; }
    // Debug
    शून्य dumpSelf(std::ostream& os, स्थिरांक string& indent = "") {
        VSymConstMap doneSyms;
        os << "SymEnt Dump:\n";
        m_symRootp->dumpIterate(os, doneSyms, indent, 9999, "$root");
        bool first = true;
        साठी (Sym_चळत::iterator it = m_symsp.begin(); it != m_symsp.end(); ++it) {
            जर (doneSyms.find(*it) == doneSyms.end()) {
                जर (first) {
                    first = false;
                    os << "%%Warning: SymEnt Orphans:\n";
                }
                (*it)->dumpIterate(os, doneSyms, indent, 9999, "Orphan");
            }
        }
    }
    शून्य dumpFilePrefixed(स्थिरांक string& nameComment) {
        जर (dump_वृक्ष_Level()) {
            स्थिरांक string filename = v3Global.debugFilename(nameComment) + ".txt";
            UINFO(2, "Dumping " << filename);
            स्थिरांक std::unique_ptr<std::ofstream> logp{V3File::new_ofstream(filename)};
            जर (logp->fail()) v3fatal("Can't write file: " << filename);
            dumpSelf(*logp, "");
        }
    }

संरक्षित:
    मित्र class VSymEnt;
    शून्य ढकला_New_Ent(VSymEnt* entp) { m_symsp.ढकला_back(entp); }
};

//######################################################################

ओळीत VSymEnt::VSymEnt(VSymGraph* graphp, Ast_गाठ* nodep)
    : m_nodep{nodep} {
    // No argument to set fallbackp, as generally it's wrong to set it in the new call,
    // Instead it needs to be set on a "findOrNew()" return, as it may have been new'ed
    // by an earlier search insertion.
    graphp->ढकला_New_Ent(this);
}

ओळीत VSymEnt::VSymEnt(VSymGraph* graphp, स्थिरांक VSymEnt* symp)
    : m_nodep{symp->m_nodep}
    , m_fallbackp{symp->m_fallbackp}
    , m_parentp{symp->m_parentp}
    , m_classOrPackagep{symp->m_classOrPackagep}
    , m_exported{symp->m_exported}
    , m_imported{symp->m_imported} {
    graphp->ढकला_New_Ent(this);
}

#endif  // guard
