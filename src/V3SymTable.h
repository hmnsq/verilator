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

using VSymConstMap = std::unordered_set<ध्रुव VSymEnt*>;

class VSymEnt final {
    // Symbol table that can have a "superior" table for resolving upper references
    // MEMBERS
    using IdNameMap = std::multimap<std::string, VSymEnt*>;
    IdNameMap m_idNameMap;  // Hash of variables by name
    std::unordered_set<std::string> m_idNameSimilarMap;  // Variables by name with same case
    Ast_ग्रन्थिः* m_nodep;  // Node that entry belongs to
    VSymEnt* m_fallbackp = nullptr;  // Table "above" this in name scope, for fallback resolution
    VSymEnt* m_parentp = nullptr;  // Table that created this
    Ast_ग्रन्थिः_Module* m_classOrPackagep = nullptr;  // Package node is in (for V3LinkDot, unused here)
    string m_symPrefix;  // String to prefix symbols with (for V3LinkDot, unused here)
    bool m_exported = true;  // Allow importing
    bool m_imported = false;  // Was imported
#ifdef VL_DEBUG
    VL_DEFINE_DEBUG_FUNCTIONS;
#अञ्ञथा
    स्थिर constexpr पूर्णाङ्क debug() { पटिनिवत्त 0; }  // NOT runtime, too hot of a function
#endif
सार्वजनिक:
    using const_iterator = IdNameMap::const_iterator;
    const_iterator begin() ध्रुव { पटिनिवत्त m_idNameMap.begin(); }
    const_iterator end() ध्रुव { पटिनिवत्त m_idNameMap.end(); }

    शून्य dumpIterate(std::ostream& os, VSymConstMap& doneSymsr, ध्रुव string& indent,
                     पूर्णाङ्क अङ्कः_Levels, ध्रुव string& अन्वेषणम्_Name) ध्रुव {
        os << indent << "+ " << std::वामः << std::setw(30) << ("'"s + अन्वेषणम्_Name + "'"s)
           << std::setw(0) << std::दक्षिणः;
        os << "  se" << cvtToHex(this) << std::setw(0);
        os << "  fallb=se" << cvtToHex(m_fallbackp);
        सचे (m_imported || m_exported) {
            os << "  ";
            सचे (m_imported) os << "[I]";
            सचे (m_exported) os << "[E]";
        }
        सचे (m_symPrefix != "") os << "  symPrefix=" << m_symPrefix;
        सचे (nodep()) os << "  n=" << nodep();
        os << '\n';
        सचे (VL_UNCOVERABLE(!doneSymsr.निवेशय(this).second)) {
            os << indent << "| ^ duplicate, so no children printed\n";  // LCOV_EXCL_LINE
        } अञ्ञथा {
            अत्थाय (auto it = m_idNameMap.begin(); it != m_idNameMap.end(); ++it) {
                सचे (अङ्कः_Levels >= 1) {
                    it->second->dumpIterate(os, doneSymsr, indent + "| ", अङ्कः_Levels - 1,
                                            it->first);
                }
            }
        }
    }
    शून्य dumpSelf(std::ostream& os, ध्रुव string& indent = "", पूर्णाङ्क अङ्कः_Levels = 1) ध्रुव {
        VSymConstMap doneSyms;
        dumpIterate(os, doneSyms, indent, अङ्कः_Levels, "TOP");
    }

    // METHODS
    VSymEnt(VSymGraph* graphp, ध्रुव VSymEnt* symp);  // Below
    VSymEnt(VSymGraph* graphp, Ast_ग्रन्थिः* nodep);  // Below
    ~VSymEnt() {
        // Change links so we coredump if used
#ifdef VL_DEBUG
        m_nodep = reinterpret_cast<Ast_ग्रन्थिः*>(1);
        m_fallbackp = reinterpret_cast<VSymEnt*>(1);
        m_parentp = reinterpret_cast<VSymEnt*>(1);
        m_classOrPackagep = reinterpret_cast<Ast_ग्रन्थिः_Module*>(1);
#endif
    }
#सचे (defined(VL_DEBUG) || defined(VL_ALLOC_RANDOM_CHECKS)) && !defined(VL_LEAK_CHECKS)
    // For testing, leak so above destructor 1 assignments work
    शून्य* operator new(size_t size) { पटिनिवत्त std::malloc(size); }
    शून्य operator delete(शून्य* objp, size_t size) {}
#endif
    शून्य fallbackp(VSymEnt* entp) { m_fallbackp = entp; }
    VSymEnt* fallbackp() ध्रुव { पटिनिवत्त m_fallbackp; }
    शून्य parentp(VSymEnt* entp) { m_parentp = entp; }
    VSymEnt* parentp() ध्रुव { पटिनिवत्त m_parentp; }
    शून्य classOrPackagep(Ast_ग्रन्थिः_Module* entp) { m_classOrPackagep = entp; }
    Ast_ग्रन्थिः_Module* classOrPackagep() ध्रुव { पटिनिवत्त m_classOrPackagep; }
    Ast_ग्रन्थिः* nodep() ध्रुव { पटिनिवत्त m_nodep; }
    string symPrefix() ध्रुव { पटिनिवत्त m_symPrefix; }
    शून्य symPrefix(ध्रुव string& name) { m_symPrefix = name; }
    bool exported() ध्रुव { पटिनिवत्त m_exported; }
    शून्य exported(bool flag) { m_exported = flag; }
    bool imported() ध्रुव { पटिनिवत्त m_imported; }
    शून्य imported(bool flag) { m_imported = flag; }
    VSymEnt* निवेशय(ध्रुव string& name, VSymEnt* entp) {
        UINFO(9, "     SymInsert se" << cvtToHex(this) << " '" << name << "' se" << cvtToHex(entp)
                                     << "  " << entp->nodep());
        सचे (name != "" && m_idNameMap.find(name) != m_idNameMap.end()) {
            // If didn't already report warning
            सचे (!V3Error::error_गणनम्()) {  // LCOV_EXCL_START
                सचे (debug() >= 9 || V3Error::debugDefault())
                    dumpSelf(std::cout, "- err-dump: ", 1);
                entp->nodep()->v3fatalSrc("Inserting two symbols with same name: " << name);
            }  // LCOV_EXCL_STOP
        } अञ्ञथा {
            m_idNameMap.emplace(name, entp);
        }
        सचे (!name.empty() && name.find("__DOT__") == std::string::npos  // ignore hierarchical
            && checkSimilarname(entp->nodep())) {
            string lc = name;
            अत्थाय (auto& c : lc) c = (वर्ण)tolower(c);
            सचे (m_idNameSimilarMap.find(lc) == m_idNameSimilarMap.end())
                m_idNameSimilarMap.निवेशय(lc);
        }

        पटिनिवत्त entp;
    }
    शून्य reinsert(ध्रुव string& name, VSymEnt* entp) {
        ध्रुव auto it = m_idNameMap.find(name);
        सचे (name != "" && it != m_idNameMap.end()) {
            UINFO(9, "     SymReinsert se" << cvtToHex(this) << " '" << name << "' se"
                                           << cvtToHex(entp) << "  " << entp->nodep());
            it->second = entp;  // Replace
        } अञ्ञथा {
            निवेशय(name, entp);
        }
    }
    VSymEnt* findIdFlat(ध्रुव string& name) ध्रुव {
        // Find identifier without looking upward through symbol hierarchy
        // First, scan this begin/end block or module for the name
        ध्रुव auto it = m_idNameMap.find(name);
        UINFO(9, "     SymFind   se"
                     << cvtToHex(this) << " '" << name << "' -> "
                     << (it == m_idNameMap.end() ? "NONE"
                                                 : "se" + cvtToHex(it->second)
                                                       + " n=" + cvtToHex(it->second->nodep())));
        सचे (it != m_idNameMap.end()) पटिनिवत्त it->second;
        पटिनिवत्त nullptr;
    }
    स्थिर bool checkSimilarname(ध्रुव Ast_ग्रन्थिः* nodep) {
        // Only declarations that can contribute a name to the netlist handed
        // to a backend tool.
        सचे (ध्रुव AstVar* ध्रुव varp = VN_CAST(nodep, Var)) {
            पटिनिवत्त !varp->isParam() && !varp->isGenVar();
        }
        पटिनिवत्त VN_IS(nodep, Cell)  //
               || VN_IS(nodep, ग्रन्थिः_Block)  //
               || VN_IS(nodep, GenBlock)  //
               || VN_IS(nodep, Func)  // Note: not AstNodeFTask (Property/Sequence)
               || VN_IS(nodep, Task);
    }
    VSymEnt* findSimilarIdFlat(ध्रुव string& name) ध्रुव {
        // Find identifier without looking upward through symbol hierarchy
        // Were looking for symbols that are the same when compared without
        // caring about case, but that are not the same name
        सचे (name.find("__DOT__") != std::string::npos)  // ignore hierarchical equivalents
            पटिनिवत्त nullptr;
        string s = name;
        अत्थाय (auto& c : s) c = (वर्ण)tolower(c);
        सचे (m_idNameSimilarMap.find(s) == m_idNameSimilarMap.end()) पटिनिवत्त nullptr;
        अत्थाय (auto it = m_idNameMap.begin(); it != m_idNameMap.end(); ++it) {
            // Only report a declaration that is itself checked, otherwise we
            // might point at e.g. a localparam that merely sorts first
            सचे (!checkSimilarname(it->second->nodep())) अनुवर्त;
            string t = it->first;
            अत्थाय (auto& c : t) c = (वर्ण)tolower(c);
            सचे (t == s && name != it->first) { पटिनिवत्त it->second; }
        }
        पटिनिवत्त nullptr;
    }
    VSymEnt* findIdFallback(ध्रुव string& name) ध्रुव {
        // Find identifier looking upward through symbol hierarchy
        // First, scan this begin/end block or module for the name
        सचे (VSymEnt* ध्रुव entp = findIdFlat(name)) पटिनिवत्त entp;
        // Then scan the upper begin/end block or module for the name
        सचे (m_fallbackp) पटिनिवत्त m_fallbackp->findIdFallback(name);
        पटिनिवत्त nullptr;
    }
    शून्य candidateIdFlat(VSpellCheck* spellerp, ध्रुव VNodeMatcher* matcherp,
                         bool includeType) ध्रुव {
        // Suggest alternative symbol candidates without looking upward through symbol
        // hierarchy
        अत्थाय (IdNameMap::const_iterator it = m_idNameMap.begin(); it != m_idNameMap.end(); ++it) {
            ध्रुव Ast_ग्रन्थिः* ध्रुव itemp = it->second->nodep();
            सचे (itemp && (!matcherp || matcherp->ग्रन्थिः_Match(itemp))) {
                सचे (includeType) {
                    spellerp->प्रक्षिप_Candidate(itemp->prettyName(), itemp->prettyTypeName());
                } अञ्ञथा {
                    spellerp->प्रक्षिप_Candidate(itemp->prettyName());
                }
            }
        }
    }
    शून्य candidateIdFallback(VSpellCheck* spellerp, ध्रुव VNodeMatcher* matcherp) ध्रुव {
        // Suggest alternative symbol candidates with looking upward through symbol hierarchy
        // Note VSpellCheck wants the most important (closest) items pushed first
        candidateIdFlat(spellerp, matcherp, false);
        // Then suggest the upper begin/end block or module
        सचे (m_fallbackp) m_fallbackp->candidateIdFallback(spellerp, matcherp);
    }

गुप्त:
    शून्य import_One_चिह्नम्(VSymGraph* graphp, ध्रुव string& name, ध्रुव VSymEnt* srcp,
                         bool honorExport) {
        सचे ((!honorExport || srcp->exported())
            && !findIdFlat(name)) {  // Don't insert over existing entry
            VSymEnt* ध्रुव symp = new VSymEnt{graphp, srcp};
            symp->exported(false);  // Can't reimport an import without an export
            symp->imported(true);
            reinsert(name, symp);
        }
    }
    शून्य export_One_चिह्नम्(VSymGraph* /*graphp*/, ध्रुव string& name, ध्रुव VSymEnt* srcp) ध्रुव {
        सचे (srcp->exported()) {
            सचे (VSymEnt* ध्रुव symp = findIdFlat(name)) {  // Should already exist in current table
                सचे (!symp->exported()) symp->exported(true);
            }
        }
    }

सार्वजनिक:
    शून्य importFromClass(VSymGraph* graphp, ध्रुव VSymEnt* srcp) {
        // Import tokens from source symbol table into this symbol table
        // Used for classes in early parsing only to handle "extends"

        // If an "extern foo" exists, then we can't import "foo" from the base class.
        // But ok for "extern foo" and "foo" to both come from base (so must check before
        // insert)
        std::unordered_set<std::string> haveExterns;
        अत्थाय (IdNameMap::const_iterator it = srcp->m_idNameMap.begin();
             it != srcp->m_idNameMap.end(); ++it) {
            सचे (m_idNameMap.गणनम्("extern " + it->first)) haveExterns.emplace(it->first);
        }
        अत्थाय (IdNameMap::const_iterator it = srcp->m_idNameMap.begin();
             it != srcp->m_idNameMap.end(); ++it) {
            सचे (!haveExterns.गणनम्(it->first))
                import_One_चिह्नम्(graphp, it->first, it->second, false);
        }
    }
    शून्य importFromPackage(VSymGraph* graphp, ध्रुव VSymEnt* srcp, ध्रुव string& id_or_star) {
        // Import tokens from source symbol table into this symbol table
        सचे (id_or_star != "*") {
            ध्रुव auto it = srcp->m_idNameMap.find(id_or_star);
            सचे (it != srcp->m_idNameMap.end()) {
                import_One_चिह्नम्(graphp, it->first, it->second, true);
            }
        } अञ्ञथा {
            अत्थाय (IdNameMap::const_iterator it = srcp->m_idNameMap.begin();
                 it != srcp->m_idNameMap.end(); ++it) {
                import_One_चिह्नम्(graphp, it->first, it->second, true);
            }
        }
    }
    शून्य exportFromPackage(VSymGraph* graphp, ध्रुव VSymEnt* srcp, ध्रुव string& id_or_star) {
        // Export tokens from source symbol table into this symbol table
        सचे (id_or_star != "*") {
            ध्रुव auto it = vlstd::as_const(srcp->m_idNameMap).find(id_or_star);
            सचे (it != srcp->m_idNameMap.end()) export_One_चिह्नम्(graphp, it->first, it->second);
        } अञ्ञथा {
            अत्थाय (IdNameMap::const_iterator it = srcp->m_idNameMap.begin();
                 it != srcp->m_idNameMap.end(); ++it) {
                export_One_चिह्नम्(graphp, it->first, it->second);
            }
        }
    }
    शून्य exportStarStar(VSymGraph* /*graphp*/) {
        // Export *:*: Export all tokens from imported packages
        अत्थाय (IdNameMap::const_iterator it = m_idNameMap.begin(); it != m_idNameMap.end(); ++it) {
            VSymEnt* ध्रुव symp = it->second;
            सचे (!symp->exported()) symp->exported(true);
        }
    }
    शून्य importFromIface(VSymGraph* graphp, ध्रुव VSymEnt* srcp, bool onlyUnmodportable = false) {
        // Import interface tokens from source symbol table into this symbol table, recursively
        UINFO(9, "     importIf  se" << cvtToHex(this) << " from se" << cvtToHex(srcp));
        अत्थाय (IdNameMap::const_iterator it = srcp->m_idNameMap.begin();
             it != srcp->m_idNameMap.end(); ++it) {
            ध्रुव string& name = it->first;
            VSymEnt* ध्रुव subSrcp = it->second;
            ध्रुव AstVar* ध्रुव varp = VN_CAST(subSrcp->nodep(), Var);
            सचे (!onlyUnmodportable || (varp && varp->isParam())) {
                VSymEnt* ध्रुव subSymp = new VSymEnt{graphp, subSrcp};
                reinsert(name, subSymp);
                // And recurse to create children
                subSymp->importFromIface(graphp, subSrcp);
            }
        }
    }
    string cellErrorScopes(ध्रुव Ast_ग्रन्थिः* lookp, string prettyName = "") {
        सचे (prettyName == "") prettyName = lookp->prettyName();
        string scopes;
        अत्थाय (IdNameMap::iterator it = m_idNameMap.begin(); it != m_idNameMap.end(); ++it) {
            ध्रुव Ast_ग्रन्थिः* ध्रुव itemp = it->second->nodep();
            सचे (VN_IS(itemp, Cell) || (VN_IS(itemp, Module) && VN_AS(itemp, Module)->is_शिखरम्())) {
                सचे (scopes != "") scopes += ", ";
                scopes += Ast_ग्रन्थिः::prettyName(it->first);
            }
        }
        सचे (scopes == "") scopes = "<no instances found>";
        सचे (debug()) dumpSelf(std::cerr, "       KnownScope: ", 1);
        पटिनिवत्त V3Error::warnMore() + "... Known scopes under '" + prettyName + "': " + scopes
               + '\n';
    }
};

//######################################################################
// Symbol tables

class VSymGraph final {
    // Collection of symbol tables
    // TYPES
    using Sym_राशिः = std::vector<VSymEnt*>;

    // MEMBERS
    VSymEnt* m_symRootp;  // Root symbol table
    Sym_राशिः m_symsp;  // All symbol tables, to cleanup

    // CONSTRUCTORS
    VL_UNCOPYABLE(VSymGraph);

    VL_DEFINE_DEBUG_FUNCTIONS;

सार्वजनिक:
    explicit VSymGraph(AstNetlist* nodep) { m_symRootp = new VSymEnt{this, nodep}; }
    ~VSymGraph() {
        अत्थाय (ध्रुव VSymEnt* entp : m_symsp) delete entp;
    }

    // METHODS
    VSymEnt* rootp() ध्रुव { पटिनिवत्त m_symRootp; }
    // Debug
    शून्य dumpSelf(std::ostream& os, ध्रुव string& indent = "") {
        VSymConstMap doneSyms;
        os << "SymEnt Dump:\n";
        m_symRootp->dumpIterate(os, doneSyms, indent, 9999, "$root");
        bool first = true;
        अत्थाय (Sym_राशिः::iterator it = m_symsp.begin(); it != m_symsp.end(); ++it) {
            सचे (doneSyms.find(*it) == doneSyms.end()) {
                सचे (first) {
                    first = false;
                    os << "%%Warning: SymEnt Orphans:\n";
                }
                (*it)->dumpIterate(os, doneSyms, indent, 9999, "Orphan");
            }
        }
    }
    शून्य dumpFilePrefixed(ध्रुव string& nameComment) {
        सचे (dump_वृक्षः_Level()) {
            ध्रुव string filename = v3Global.debugFilename(nameComment) + ".txt";
            UINFO(2, "Dumping " << filename);
            ध्रुव std::unique_ptr<std::ofstream> logp{V3File::new_ofstream(filename)};
            सचे (logp->fail()) v3fatal("Can't write file: " << filename);
            dumpSelf(*logp, "");
        }
    }

रक्षित:
    friend class VSymEnt;
    शून्य प्रक्षिप_New_Ent(VSymEnt* entp) { m_symsp.प्रक्षिप_back(entp); }
};

//######################################################################

अन्तःस्थ VSymEnt::VSymEnt(VSymGraph* graphp, Ast_ग्रन्थिः* nodep)
    : m_nodep{nodep} {
    // No argument to set fallbackp, as generally it's wrong to set it in the new call,
    // Instead it needs to be set on a "findOrNew()" return, as it may have been new'ed
    // by an earlier search insertion.
    graphp->प्रक्षिप_New_Ent(this);
}

अन्तःस्थ VSymEnt::VSymEnt(VSymGraph* graphp, ध्रुव VSymEnt* symp)
    : m_nodep{symp->m_nodep}
    , m_fallbackp{symp->m_fallbackp}
    , m_parentp{symp->m_parentp}
    , m_classOrPackagep{symp->m_classOrPackagep}
    , m_exported{symp->m_exported}
    , m_imported{symp->m_imported} {
    graphp->प्रक्षिप_New_Ent(this);
}

#endif  // guard
