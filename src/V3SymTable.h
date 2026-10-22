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

#അന്തര്ഭാവ "config_build.h"
#അന്തര്ഭാവ "verilatedos.h"

#അന്തര്ഭാവ "V3Ast.h"
#അന്തര്ഭാവ "V3File.h"
#അന്തര്ഭാവ "V3Global.h"
#അന്തര്ഭാവ "V3String.h"

#അന്തര്ഭാവ <cstdarg>
#അന്തര്ഭാവ <iomanip>
#അന്തര്ഭാവ <map>
#അന്തര്ഭാവ <memory>
#അന്തര്ഭാവ <unordered_set>
#അന്തര്ഭാവ <vector>

class VSymGraph;
class VSymEnt;

//######################################################################
// Symbol table

using VSymConstMap = std::unordered_set<സ്ഥിരം VSymEnt*>;

class VSymEnt final {
    // Symbol table that can have a "superior" table for resolving upper references
    // MEMBERS
    using IdNameMap = std::multimap<std::string, VSymEnt*>;
    IdNameMap m_idNameMap;  // Hash of variables by name
    std::unordered_set<std::string> m_idNameSimilarMap;  // Variables by name with same case
    Ast_കണ്ണി* m_nodep;  // Node that entry belongs to
    VSymEnt* m_fallbackp = nullptr;  // Table "above" this in name scope, for fallback resolution
    VSymEnt* m_parentp = nullptr;  // Table that created this
    Ast_കണ്ണി_Module* m_classOrPackagep = nullptr;  // Package node is in (for V3LinkDot, unused here)
    string m_symPrefix;  // String to prefix symbols with (for V3LinkDot, unused here)
    bool m_exported = true;  // Allow importing
    bool m_imported = false;  // Was imported
#ifdef VL_DEBUG
    VL_DEFINE_DEBUG_FUNCTIONS;
#അല്ലെങ്കിൽ
    സ്ഥിതം constexpr പൂർണ്ണസംഖ്യ debug() { തിരികെ 0; }  // NOT runtime, too hot of a function
#endif
പൊതു:
    using const_iterator = IdNameMap::const_iterator;
    const_iterator begin() സ്ഥിരം { തിരികെ m_idNameMap.begin(); }
    const_iterator end() സ്ഥിരം { തിരികെ m_idNameMap.end(); }

    ശൂന്യം dumpIterate(std::ostream& os, VSymConstMap& doneSymsr, സ്ഥിരം string& indent,
                     പൂർണ്ണസംഖ്യ അക്കം_Levels, സ്ഥിരം string& തിരയൽ_Name) സ്ഥിരം {
        os << indent << "+ " << std::ഇടത് << std::setw(30) << ("'"s + തിരയൽ_Name + "'"s)
           << std::setw(0) << std::വലത്;
        os << "  se" << cvtToHex(this) << std::setw(0);
        os << "  fallb=se" << cvtToHex(m_fallbackp);
        എങ്കിൽ (m_imported || m_exported) {
            os << "  ";
            എങ്കിൽ (m_imported) os << "[I]";
            എങ്കിൽ (m_exported) os << "[E]";
        }
        എങ്കിൽ (m_symPrefix != "") os << "  symPrefix=" << m_symPrefix;
        എങ്കിൽ (nodep()) os << "  n=" << nodep();
        os << '\n';
        എങ്കിൽ (VL_UNCOVERABLE(!doneSymsr.തിരുകുക(this).second)) {
            os << indent << "| ^ duplicate, so no children printed\n";  // LCOV_EXCL_LINE
        } അല്ലെങ്കിൽ {
            വേണ്ടി (auto it = m_idNameMap.begin(); it != m_idNameMap.end(); ++it) {
                എങ്കിൽ (അക്കം_Levels >= 1) {
                    it->second->dumpIterate(os, doneSymsr, indent + "| ", അക്കം_Levels - 1,
                                            it->first);
                }
            }
        }
    }
    ശൂന്യം dumpSelf(std::ostream& os, സ്ഥിരം string& indent = "", പൂർണ്ണസംഖ്യ അക്കം_Levels = 1) സ്ഥിരം {
        VSymConstMap doneSyms;
        dumpIterate(os, doneSyms, indent, അക്കം_Levels, "TOP");
    }

    // METHODS
    VSymEnt(VSymGraph* graphp, സ്ഥിരം VSymEnt* symp);  // Below
    VSymEnt(VSymGraph* graphp, Ast_കണ്ണി* nodep);  // Below
    ~VSymEnt() {
        // Change links so we coredump if used
#ifdef VL_DEBUG
        m_nodep = reinterpret_cast<Ast_കണ്ണി*>(1);
        m_fallbackp = reinterpret_cast<VSymEnt*>(1);
        m_parentp = reinterpret_cast<VSymEnt*>(1);
        m_classOrPackagep = reinterpret_cast<Ast_കണ്ണി_Module*>(1);
#endif
    }
#എങ്കിൽ (defined(VL_DEBUG) || defined(VL_ALLOC_RANDOM_CHECKS)) && !defined(VL_LEAK_CHECKS)
    // For testing, leak so above destructor 1 assignments work
    ശൂന്യം* operator new(size_t size) { തിരികെ std::malloc(size); }
    ശൂന്യം operator delete(ശൂന്യം* objp, size_t size) {}
#endif
    ശൂന്യം fallbackp(VSymEnt* entp) { m_fallbackp = entp; }
    VSymEnt* fallbackp() സ്ഥിരം { തിരികെ m_fallbackp; }
    ശൂന്യം parentp(VSymEnt* entp) { m_parentp = entp; }
    VSymEnt* parentp() സ്ഥിരം { തിരികെ m_parentp; }
    ശൂന്യം classOrPackagep(Ast_കണ്ണി_Module* entp) { m_classOrPackagep = entp; }
    Ast_കണ്ണി_Module* classOrPackagep() സ്ഥിരം { തിരികെ m_classOrPackagep; }
    Ast_കണ്ണി* nodep() സ്ഥിരം { തിരികെ m_nodep; }
    string symPrefix() സ്ഥിരം { തിരികെ m_symPrefix; }
    ശൂന്യം symPrefix(സ്ഥിരം string& name) { m_symPrefix = name; }
    bool exported() സ്ഥിരം { തിരികെ m_exported; }
    ശൂന്യം exported(bool flag) { m_exported = flag; }
    bool imported() സ്ഥിരം { തിരികെ m_imported; }
    ശൂന്യം imported(bool flag) { m_imported = flag; }
    VSymEnt* തിരുകുക(സ്ഥിരം string& name, VSymEnt* entp) {
        UINFO(9, "     SymInsert se" << cvtToHex(this) << " '" << name << "' se" << cvtToHex(entp)
                                     << "  " << entp->nodep());
        എങ്കിൽ (name != "" && m_idNameMap.find(name) != m_idNameMap.end()) {
            // If didn't already report warning
            എങ്കിൽ (!V3Error::error_എണ്ണം()) {  // LCOV_EXCL_START
                എങ്കിൽ (debug() >= 9 || V3Error::debugDefault())
                    dumpSelf(std::cout, "- err-dump: ", 1);
                entp->nodep()->v3fatalSrc("Inserting two symbols with same name: " << name);
            }  // LCOV_EXCL_STOP
        } അല്ലെങ്കിൽ {
            m_idNameMap.emplace(name, entp);
        }
        എങ്കിൽ (!name.empty() && name.find("__DOT__") == std::string::npos  // ignore hierarchical
            && checkSimilarname(entp->nodep())) {
            string lc = name;
            വേണ്ടി (auto& c : lc) c = (അക്ഷരം)tolower(c);
            എങ്കിൽ (m_idNameSimilarMap.find(lc) == m_idNameSimilarMap.end())
                m_idNameSimilarMap.തിരുകുക(lc);
        }

        തിരികെ entp;
    }
    ശൂന്യം reinsert(സ്ഥിരം string& name, VSymEnt* entp) {
        സ്ഥിരം auto it = m_idNameMap.find(name);
        എങ്കിൽ (name != "" && it != m_idNameMap.end()) {
            UINFO(9, "     SymReinsert se" << cvtToHex(this) << " '" << name << "' se"
                                           << cvtToHex(entp) << "  " << entp->nodep());
            it->second = entp;  // Replace
        } അല്ലെങ്കിൽ {
            തിരുകുക(name, entp);
        }
    }
    VSymEnt* findIdFlat(സ്ഥിരം string& name) സ്ഥിരം {
        // Find identifier without looking upward through symbol hierarchy
        // First, scan this begin/end block or module for the name
        സ്ഥിരം auto it = m_idNameMap.find(name);
        UINFO(9, "     SymFind   se"
                     << cvtToHex(this) << " '" << name << "' -> "
                     << (it == m_idNameMap.end() ? "NONE"
                                                 : "se" + cvtToHex(it->second)
                                                       + " n=" + cvtToHex(it->second->nodep())));
        എങ്കിൽ (it != m_idNameMap.end()) തിരികെ it->second;
        തിരികെ nullptr;
    }
    സ്ഥിതം bool checkSimilarname(സ്ഥിരം Ast_കണ്ണി* nodep) {
        // Only declarations that can contribute a name to the netlist handed
        // to a backend tool.
        എങ്കിൽ (സ്ഥിരം AstVar* സ്ഥിരം varp = VN_CAST(nodep, Var)) {
            തിരികെ !varp->isParam() && !varp->isGenVar();
        }
        തിരികെ VN_IS(nodep, Cell)  //
               || VN_IS(nodep, കണ്ണി_Block)  //
               || VN_IS(nodep, GenBlock)  //
               || VN_IS(nodep, Func)  // Note: not AstNodeFTask (Property/Sequence)
               || VN_IS(nodep, Task);
    }
    VSymEnt* findSimilarIdFlat(സ്ഥിരം string& name) സ്ഥിരം {
        // Find identifier without looking upward through symbol hierarchy
        // Were looking for symbols that are the same when compared without
        // caring about case, but that are not the same name
        എങ്കിൽ (name.find("__DOT__") != std::string::npos)  // ignore hierarchical equivalents
            തിരികെ nullptr;
        string s = name;
        വേണ്ടി (auto& c : s) c = (അക്ഷരം)tolower(c);
        എങ്കിൽ (m_idNameSimilarMap.find(s) == m_idNameSimilarMap.end()) തിരികെ nullptr;
        വേണ്ടി (auto it = m_idNameMap.begin(); it != m_idNameMap.end(); ++it) {
            // Only report a declaration that is itself checked, otherwise we
            // might point at e.g. a localparam that merely sorts first
            എങ്കിൽ (!checkSimilarname(it->second->nodep())) തുടരുക;
            string t = it->first;
            വേണ്ടി (auto& c : t) c = (അക്ഷരം)tolower(c);
            എങ്കിൽ (t == s && name != it->first) { തിരികെ it->second; }
        }
        തിരികെ nullptr;
    }
    VSymEnt* findIdFallback(സ്ഥിരം string& name) സ്ഥിരം {
        // Find identifier looking upward through symbol hierarchy
        // First, scan this begin/end block or module for the name
        എങ്കിൽ (VSymEnt* സ്ഥിരം entp = findIdFlat(name)) തിരികെ entp;
        // Then scan the upper begin/end block or module for the name
        എങ്കിൽ (m_fallbackp) തിരികെ m_fallbackp->findIdFallback(name);
        തിരികെ nullptr;
    }
    ശൂന്യം candidateIdFlat(VSpellCheck* spellerp, സ്ഥിരം VNodeMatcher* matcherp,
                         bool includeType) സ്ഥിരം {
        // Suggest alternative symbol candidates without looking upward through symbol
        // hierarchy
        വേണ്ടി (IdNameMap::const_iterator it = m_idNameMap.begin(); it != m_idNameMap.end(); ++it) {
            സ്ഥിരം Ast_കണ്ണി* സ്ഥിരം itemp = it->second->nodep();
            എങ്കിൽ (itemp && (!matcherp || matcherp->കണ്ണി_Match(itemp))) {
                എങ്കിൽ (includeType) {
                    spellerp->തള്ളുക_Candidate(itemp->prettyName(), itemp->prettyTypeName());
                } അല്ലെങ്കിൽ {
                    spellerp->തള്ളുക_Candidate(itemp->prettyName());
                }
            }
        }
    }
    ശൂന്യം candidateIdFallback(VSpellCheck* spellerp, സ്ഥിരം VNodeMatcher* matcherp) സ്ഥിരം {
        // Suggest alternative symbol candidates with looking upward through symbol hierarchy
        // Note VSpellCheck wants the most important (closest) items pushed first
        candidateIdFlat(spellerp, matcherp, false);
        // Then suggest the upper begin/end block or module
        എങ്കിൽ (m_fallbackp) m_fallbackp->candidateIdFallback(spellerp, matcherp);
    }

സ്വകാര്യ:
    ശൂന്യം import_One_ചിഹ്നം(VSymGraph* graphp, സ്ഥിരം string& name, സ്ഥിരം VSymEnt* srcp,
                         bool honorExport) {
        എങ്കിൽ ((!honorExport || srcp->exported())
            && !findIdFlat(name)) {  // Don't insert over existing entry
            VSymEnt* സ്ഥിരം symp = new VSymEnt{graphp, srcp};
            symp->exported(false);  // Can't reimport an import without an export
            symp->imported(true);
            reinsert(name, symp);
        }
    }
    ശൂന്യം export_One_ചിഹ്നം(VSymGraph* /*graphp*/, സ്ഥിരം string& name, സ്ഥിരം VSymEnt* srcp) സ്ഥിരം {
        എങ്കിൽ (srcp->exported()) {
            എങ്കിൽ (VSymEnt* സ്ഥിരം symp = findIdFlat(name)) {  // Should already exist in current table
                എങ്കിൽ (!symp->exported()) symp->exported(true);
            }
        }
    }

പൊതു:
    ശൂന്യം importFromClass(VSymGraph* graphp, സ്ഥിരം VSymEnt* srcp) {
        // Import tokens from source symbol table into this symbol table
        // Used for classes in early parsing only to handle "extends"

        // If an "extern foo" exists, then we can't import "foo" from the base class.
        // But ok for "extern foo" and "foo" to both come from base (so must check before
        // insert)
        std::unordered_set<std::string> haveExterns;
        വേണ്ടി (IdNameMap::const_iterator it = srcp->m_idNameMap.begin();
             it != srcp->m_idNameMap.end(); ++it) {
            എങ്കിൽ (m_idNameMap.എണ്ണം("extern " + it->first)) haveExterns.emplace(it->first);
        }
        വേണ്ടി (IdNameMap::const_iterator it = srcp->m_idNameMap.begin();
             it != srcp->m_idNameMap.end(); ++it) {
            എങ്കിൽ (!haveExterns.എണ്ണം(it->first))
                import_One_ചിഹ്നം(graphp, it->first, it->second, false);
        }
    }
    ശൂന്യം importFromPackage(VSymGraph* graphp, സ്ഥിരം VSymEnt* srcp, സ്ഥിരം string& id_or_star) {
        // Import tokens from source symbol table into this symbol table
        എങ്കിൽ (id_or_star != "*") {
            സ്ഥിരം auto it = srcp->m_idNameMap.find(id_or_star);
            എങ്കിൽ (it != srcp->m_idNameMap.end()) {
                import_One_ചിഹ്നം(graphp, it->first, it->second, true);
            }
        } അല്ലെങ്കിൽ {
            വേണ്ടി (IdNameMap::const_iterator it = srcp->m_idNameMap.begin();
                 it != srcp->m_idNameMap.end(); ++it) {
                import_One_ചിഹ്നം(graphp, it->first, it->second, true);
            }
        }
    }
    ശൂന്യം exportFromPackage(VSymGraph* graphp, സ്ഥിരം VSymEnt* srcp, സ്ഥിരം string& id_or_star) {
        // Export tokens from source symbol table into this symbol table
        എങ്കിൽ (id_or_star != "*") {
            സ്ഥിരം auto it = vlstd::as_const(srcp->m_idNameMap).find(id_or_star);
            എങ്കിൽ (it != srcp->m_idNameMap.end()) export_One_ചിഹ്നം(graphp, it->first, it->second);
        } അല്ലെങ്കിൽ {
            വേണ്ടി (IdNameMap::const_iterator it = srcp->m_idNameMap.begin();
                 it != srcp->m_idNameMap.end(); ++it) {
                export_One_ചിഹ്നം(graphp, it->first, it->second);
            }
        }
    }
    ശൂന്യം exportStarStar(VSymGraph* /*graphp*/) {
        // Export *:*: Export all tokens from imported packages
        വേണ്ടി (IdNameMap::const_iterator it = m_idNameMap.begin(); it != m_idNameMap.end(); ++it) {
            VSymEnt* സ്ഥിരം symp = it->second;
            എങ്കിൽ (!symp->exported()) symp->exported(true);
        }
    }
    ശൂന്യം importFromIface(VSymGraph* graphp, സ്ഥിരം VSymEnt* srcp, bool onlyUnmodportable = false) {
        // Import interface tokens from source symbol table into this symbol table, recursively
        UINFO(9, "     importIf  se" << cvtToHex(this) << " from se" << cvtToHex(srcp));
        വേണ്ടി (IdNameMap::const_iterator it = srcp->m_idNameMap.begin();
             it != srcp->m_idNameMap.end(); ++it) {
            സ്ഥിരം string& name = it->first;
            VSymEnt* സ്ഥിരം subSrcp = it->second;
            സ്ഥിരം AstVar* സ്ഥിരം varp = VN_CAST(subSrcp->nodep(), Var);
            എങ്കിൽ (!onlyUnmodportable || (varp && varp->isParam())) {
                VSymEnt* സ്ഥിരം subSymp = new VSymEnt{graphp, subSrcp};
                reinsert(name, subSymp);
                // And recurse to create children
                subSymp->importFromIface(graphp, subSrcp);
            }
        }
    }
    string cellErrorScopes(സ്ഥിരം Ast_കണ്ണി* lookp, string prettyName = "") {
        എങ്കിൽ (prettyName == "") prettyName = lookp->prettyName();
        string scopes;
        വേണ്ടി (IdNameMap::iterator it = m_idNameMap.begin(); it != m_idNameMap.end(); ++it) {
            സ്ഥിരം Ast_കണ്ണി* സ്ഥിരം itemp = it->second->nodep();
            എങ്കിൽ (VN_IS(itemp, Cell) || (VN_IS(itemp, Module) && VN_AS(itemp, Module)->is_മുകൾ())) {
                എങ്കിൽ (scopes != "") scopes += ", ";
                scopes += Ast_കണ്ണി::prettyName(it->first);
            }
        }
        എങ്കിൽ (scopes == "") scopes = "<no instances found>";
        എങ്കിൽ (debug()) dumpSelf(std::cerr, "       KnownScope: ", 1);
        തിരികെ V3Error::warnMore() + "... Known scopes under '" + prettyName + "': " + scopes
               + '\n';
    }
};

//######################################################################
// Symbol tables

class VSymGraph final {
    // Collection of symbol tables
    // TYPES
    using Sym_അട്ടി = std::vector<VSymEnt*>;

    // MEMBERS
    VSymEnt* m_symRootp;  // Root symbol table
    Sym_അട്ടി m_symsp;  // All symbol tables, to cleanup

    // CONSTRUCTORS
    VL_UNCOPYABLE(VSymGraph);

    VL_DEFINE_DEBUG_FUNCTIONS;

പൊതു:
    explicit VSymGraph(AstNetlist* nodep) { m_symRootp = new VSymEnt{this, nodep}; }
    ~VSymGraph() {
        വേണ്ടി (സ്ഥിരം VSymEnt* entp : m_symsp) delete entp;
    }

    // METHODS
    VSymEnt* rootp() സ്ഥിരം { തിരികെ m_symRootp; }
    // Debug
    ശൂന്യം dumpSelf(std::ostream& os, സ്ഥിരം string& indent = "") {
        VSymConstMap doneSyms;
        os << "SymEnt Dump:\n";
        m_symRootp->dumpIterate(os, doneSyms, indent, 9999, "$root");
        bool first = true;
        വേണ്ടി (Sym_അട്ടി::iterator it = m_symsp.begin(); it != m_symsp.end(); ++it) {
            എങ്കിൽ (doneSyms.find(*it) == doneSyms.end()) {
                എങ്കിൽ (first) {
                    first = false;
                    os << "%%Warning: SymEnt Orphans:\n";
                }
                (*it)->dumpIterate(os, doneSyms, indent, 9999, "Orphan");
            }
        }
    }
    ശൂന്യം dumpFilePrefixed(സ്ഥിരം string& nameComment) {
        എങ്കിൽ (dump_വൃക്ഷം_Level()) {
            സ്ഥിരം string filename = v3Global.debugFilename(nameComment) + ".txt";
            UINFO(2, "Dumping " << filename);
            സ്ഥിരം std::unique_ptr<std::ofstream> logp{V3File::new_ofstream(filename)};
            എങ്കിൽ (logp->fail()) v3fatal("Can't write file: " << filename);
            dumpSelf(*logp, "");
        }
    }

സംരക്ഷിത:
    friend class VSymEnt;
    ശൂന്യം തള്ളുക_New_Ent(VSymEnt* entp) { m_symsp.തള്ളുക_back(entp); }
};

//######################################################################

വരിയിൽ VSymEnt::VSymEnt(VSymGraph* graphp, Ast_കണ്ണി* nodep)
    : m_nodep{nodep} {
    // No argument to set fallbackp, as generally it's wrong to set it in the new call,
    // Instead it needs to be set on a "findOrNew()" return, as it may have been new'ed
    // by an earlier search insertion.
    graphp->തള്ളുക_New_Ent(this);
}

വരിയിൽ VSymEnt::VSymEnt(VSymGraph* graphp, സ്ഥിരം VSymEnt* symp)
    : m_nodep{symp->m_nodep}
    , m_fallbackp{symp->m_fallbackp}
    , m_parentp{symp->m_parentp}
    , m_classOrPackagep{symp->m_classOrPackagep}
    , m_exported{symp->m_exported}
    , m_imported{symp->m_imported} {
    graphp->തള്ളുക_New_Ent(this);
}

#endif  // guard
