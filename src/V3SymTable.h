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

#અન્તર્ભાવ "config_build.h"
#અન્તર્ભાવ "verilatedos.h"

#અન્તર્ભાવ "V3Ast.h"
#અન્તર્ભાવ "V3File.h"
#અન્તર્ભાવ "V3Global.h"
#અન્તર્ભાવ "V3String.h"

#અન્તર્ભાવ <cstdarg>
#અન્તર્ભાવ <iomanip>
#અન્તર્ભાવ <map>
#અન્તર્ભાવ <memory>
#અન્તર્ભાવ <unordered_set>
#અન્તર્ભાવ <vector>

class VSymGraph;
class VSymEnt;

//######################################################################
// Symbol table

using VSymConstMap = std::unordered_set<અચલ VSymEnt*>;

class VSymEnt final {
    // Symbol table that can have a "superior" table for resolving upper references
    // MEMBERS
    using IdNameMap = std::multimap<std::string, VSymEnt*>;
    IdNameMap m_idNameMap;  // Hash of variables by name
    std::unordered_set<std::string> m_idNameSimilarMap;  // Variables by name with same case
    Ast_ગાંઠ* m_nodep;  // Node that entry belongs to
    VSymEnt* m_fallbackp = nullptr;  // Table "above" this in name scope, for fallback resolution
    VSymEnt* m_parentp = nullptr;  // Table that created this
    Ast_ગાંઠ_Module* m_classOrPackagep = nullptr;  // Package node is in (for V3LinkDot, unused here)
    string m_symPrefix;  // String to prefix symbols with (for V3LinkDot, unused here)
    bool m_exported = true;  // Allow importing
    bool m_imported = false;  // Was imported
#ifdef VL_DEBUG
    VL_DEFINE_DEBUG_FUNCTIONS;
#નહીંતર
    સ્થિર constexpr પૂર્ણાંક debug() { પાછા 0; }  // NOT runtime, too hot of a function
#endif
જાહેર:
    using const_iterator = IdNameMap::const_iterator;
    const_iterator begin() અચલ { પાછા m_idNameMap.begin(); }
    const_iterator end() અચલ { પાછા m_idNameMap.end(); }

    શૂન્ય dumpIterate(std::ostream& os, VSymConstMap& doneSymsr, અચલ string& indent,
                     પૂર્ણાંક અંક_Levels, અચલ string& શોધ_Name) અચલ {
        os << indent << "+ " << std::ડાબું << std::setw(30) << ("'"s + શોધ_Name + "'"s)
           << std::setw(0) << std::જમણું;
        os << "  se" << cvtToHex(this) << std::setw(0);
        os << "  fallb=se" << cvtToHex(m_fallbackp);
        જો (m_imported || m_exported) {
            os << "  ";
            જો (m_imported) os << "[I]";
            જો (m_exported) os << "[E]";
        }
        જો (m_symPrefix != "") os << "  symPrefix=" << m_symPrefix;
        જો (nodep()) os << "  n=" << nodep();
        os << '\n';
        જો (VL_UNCOVERABLE(!doneSymsr.ઉમેરો(this).second)) {
            os << indent << "| ^ duplicate, so no children printed\n";  // LCOV_EXCL_LINE
        } નહીંતર {
            માટે (auto it = m_idNameMap.begin(); it != m_idNameMap.end(); ++it) {
                જો (અંક_Levels >= 1) {
                    it->second->dumpIterate(os, doneSymsr, indent + "| ", અંક_Levels - 1,
                                            it->first);
                }
            }
        }
    }
    શૂન્ય dumpSelf(std::ostream& os, અચલ string& indent = "", પૂર્ણાંક અંક_Levels = 1) અચલ {
        VSymConstMap doneSyms;
        dumpIterate(os, doneSyms, indent, અંક_Levels, "TOP");
    }

    // METHODS
    VSymEnt(VSymGraph* graphp, અચલ VSymEnt* symp);  // Below
    VSymEnt(VSymGraph* graphp, Ast_ગાંઠ* nodep);  // Below
    ~VSymEnt() {
        // Change links so we coredump if used
#ifdef VL_DEBUG
        m_nodep = reinterpret_cast<Ast_ગાંઠ*>(1);
        m_fallbackp = reinterpret_cast<VSymEnt*>(1);
        m_parentp = reinterpret_cast<VSymEnt*>(1);
        m_classOrPackagep = reinterpret_cast<Ast_ગાંઠ_Module*>(1);
#endif
    }
#જો (defined(VL_DEBUG) || defined(VL_ALLOC_RANDOM_CHECKS)) && !defined(VL_LEAK_CHECKS)
    // For testing, leak so above destructor 1 assignments work
    શૂન્ય* operator new(size_t size) { પાછા std::malloc(size); }
    શૂન્ય operator delete(શૂન્ય* objp, size_t size) {}
#endif
    શૂન્ય fallbackp(VSymEnt* entp) { m_fallbackp = entp; }
    VSymEnt* fallbackp() અચલ { પાછા m_fallbackp; }
    શૂન્ય parentp(VSymEnt* entp) { m_parentp = entp; }
    VSymEnt* parentp() અચલ { પાછા m_parentp; }
    શૂન્ય classOrPackagep(Ast_ગાંઠ_Module* entp) { m_classOrPackagep = entp; }
    Ast_ગાંઠ_Module* classOrPackagep() અચલ { પાછા m_classOrPackagep; }
    Ast_ગાંઠ* nodep() અચલ { પાછા m_nodep; }
    string symPrefix() અચલ { પાછા m_symPrefix; }
    શૂન્ય symPrefix(અચલ string& name) { m_symPrefix = name; }
    bool exported() અચલ { પાછા m_exported; }
    શૂન્ય exported(bool flag) { m_exported = flag; }
    bool imported() અચલ { પાછા m_imported; }
    શૂન્ય imported(bool flag) { m_imported = flag; }
    VSymEnt* ઉમેરો(અચલ string& name, VSymEnt* entp) {
        UINFO(9, "     SymInsert se" << cvtToHex(this) << " '" << name << "' se" << cvtToHex(entp)
                                     << "  " << entp->nodep());
        જો (name != "" && m_idNameMap.find(name) != m_idNameMap.end()) {
            // If didn't already report warning
            જો (!V3Error::error_ગણતરી()) {  // LCOV_EXCL_START
                જો (debug() >= 9 || V3Error::debugDefault())
                    dumpSelf(std::cout, "- err-dump: ", 1);
                entp->nodep()->v3fatalSrc("Inserting two symbols with same name: " << name);
            }  // LCOV_EXCL_STOP
        } નહીંતર {
            m_idNameMap.emplace(name, entp);
        }
        જો (!name.empty() && name.find("__DOT__") == std::string::npos  // ignore hierarchical
            && checkSimilarname(entp->nodep())) {
            string lc = name;
            માટે (auto& c : lc) c = (અક્ષર)tolower(c);
            જો (m_idNameSimilarMap.find(lc) == m_idNameSimilarMap.end())
                m_idNameSimilarMap.ઉમેરો(lc);
        }

        પાછા entp;
    }
    શૂન્ય reinsert(અચલ string& name, VSymEnt* entp) {
        અચલ auto it = m_idNameMap.find(name);
        જો (name != "" && it != m_idNameMap.end()) {
            UINFO(9, "     SymReinsert se" << cvtToHex(this) << " '" << name << "' se"
                                           << cvtToHex(entp) << "  " << entp->nodep());
            it->second = entp;  // Replace
        } નહીંતર {
            ઉમેરો(name, entp);
        }
    }
    VSymEnt* findIdFlat(અચલ string& name) અચલ {
        // Find identifier without looking upward through symbol hierarchy
        // First, scan this begin/end block or module for the name
        અચલ auto it = m_idNameMap.find(name);
        UINFO(9, "     SymFind   se"
                     << cvtToHex(this) << " '" << name << "' -> "
                     << (it == m_idNameMap.end() ? "NONE"
                                                 : "se" + cvtToHex(it->second)
                                                       + " n=" + cvtToHex(it->second->nodep())));
        જો (it != m_idNameMap.end()) પાછા it->second;
        પાછા nullptr;
    }
    સ્થિર bool checkSimilarname(અચલ Ast_ગાંઠ* nodep) {
        // Only declarations that can contribute a name to the netlist handed
        // to a backend tool.
        જો (અચલ AstVar* અચલ varp = VN_CAST(nodep, Var)) {
            પાછા !varp->isParam() && !varp->isGenVar();
        }
        પાછા VN_IS(nodep, Cell)  //
               || VN_IS(nodep, ગાંઠ_Block)  //
               || VN_IS(nodep, GenBlock)  //
               || VN_IS(nodep, Func)  // Note: not AstNodeFTask (Property/Sequence)
               || VN_IS(nodep, Task);
    }
    VSymEnt* findSimilarIdFlat(અચલ string& name) અચલ {
        // Find identifier without looking upward through symbol hierarchy
        // Were looking for symbols that are the same when compared without
        // caring about case, but that are not the same name
        જો (name.find("__DOT__") != std::string::npos)  // ignore hierarchical equivalents
            પાછા nullptr;
        string s = name;
        માટે (auto& c : s) c = (અક્ષર)tolower(c);
        જો (m_idNameSimilarMap.find(s) == m_idNameSimilarMap.end()) પાછા nullptr;
        માટે (auto it = m_idNameMap.begin(); it != m_idNameMap.end(); ++it) {
            // Only report a declaration that is itself checked, otherwise we
            // might point at e.g. a localparam that merely sorts first
            જો (!checkSimilarname(it->second->nodep())) ચાલુ;
            string t = it->first;
            માટે (auto& c : t) c = (અક્ષર)tolower(c);
            જો (t == s && name != it->first) { પાછા it->second; }
        }
        પાછા nullptr;
    }
    VSymEnt* findIdFallback(અચલ string& name) અચલ {
        // Find identifier looking upward through symbol hierarchy
        // First, scan this begin/end block or module for the name
        જો (VSymEnt* અચલ entp = findIdFlat(name)) પાછા entp;
        // Then scan the upper begin/end block or module for the name
        જો (m_fallbackp) પાછા m_fallbackp->findIdFallback(name);
        પાછા nullptr;
    }
    શૂન્ય candidateIdFlat(VSpellCheck* spellerp, અચલ VNodeMatcher* matcherp,
                         bool includeType) અચલ {
        // Suggest alternative symbol candidates without looking upward through symbol
        // hierarchy
        માટે (IdNameMap::const_iterator it = m_idNameMap.begin(); it != m_idNameMap.end(); ++it) {
            અચલ Ast_ગાંઠ* અચલ itemp = it->second->nodep();
            જો (itemp && (!matcherp || matcherp->ગાંઠ_Match(itemp))) {
                જો (includeType) {
                    spellerp->ધકેલો_Candidate(itemp->prettyName(), itemp->prettyTypeName());
                } નહીંતર {
                    spellerp->ધકેલો_Candidate(itemp->prettyName());
                }
            }
        }
    }
    શૂન્ય candidateIdFallback(VSpellCheck* spellerp, અચલ VNodeMatcher* matcherp) અચલ {
        // Suggest alternative symbol candidates with looking upward through symbol hierarchy
        // Note VSpellCheck wants the most important (closest) items pushed first
        candidateIdFlat(spellerp, matcherp, false);
        // Then suggest the upper begin/end block or module
        જો (m_fallbackp) m_fallbackp->candidateIdFallback(spellerp, matcherp);
    }

ખાનગી:
    શૂન્ય import_One_પ્રતીક(VSymGraph* graphp, અચલ string& name, અચલ VSymEnt* srcp,
                         bool honorExport) {
        જો ((!honorExport || srcp->exported())
            && !findIdFlat(name)) {  // Don't insert over existing entry
            VSymEnt* અચલ symp = new VSymEnt{graphp, srcp};
            symp->exported(false);  // Can't reimport an import without an export
            symp->imported(true);
            reinsert(name, symp);
        }
    }
    શૂન્ય export_One_પ્રતીક(VSymGraph* /*graphp*/, અચલ string& name, અચલ VSymEnt* srcp) અચલ {
        જો (srcp->exported()) {
            જો (VSymEnt* અચલ symp = findIdFlat(name)) {  // Should already exist in current table
                જો (!symp->exported()) symp->exported(true);
            }
        }
    }

જાહેર:
    શૂન્ય importFromClass(VSymGraph* graphp, અચલ VSymEnt* srcp) {
        // Import tokens from source symbol table into this symbol table
        // Used for classes in early parsing only to handle "extends"

        // If an "extern foo" exists, then we can't import "foo" from the base class.
        // But ok for "extern foo" and "foo" to both come from base (so must check before
        // insert)
        std::unordered_set<std::string> haveExterns;
        માટે (IdNameMap::const_iterator it = srcp->m_idNameMap.begin();
             it != srcp->m_idNameMap.end(); ++it) {
            જો (m_idNameMap.ગણતરી("extern " + it->first)) haveExterns.emplace(it->first);
        }
        માટે (IdNameMap::const_iterator it = srcp->m_idNameMap.begin();
             it != srcp->m_idNameMap.end(); ++it) {
            જો (!haveExterns.ગણતરી(it->first))
                import_One_પ્રતીક(graphp, it->first, it->second, false);
        }
    }
    શૂન્ય importFromPackage(VSymGraph* graphp, અચલ VSymEnt* srcp, અચલ string& id_or_star) {
        // Import tokens from source symbol table into this symbol table
        જો (id_or_star != "*") {
            અચલ auto it = srcp->m_idNameMap.find(id_or_star);
            જો (it != srcp->m_idNameMap.end()) {
                import_One_પ્રતીક(graphp, it->first, it->second, true);
            }
        } નહીંતર {
            માટે (IdNameMap::const_iterator it = srcp->m_idNameMap.begin();
                 it != srcp->m_idNameMap.end(); ++it) {
                import_One_પ્રતીક(graphp, it->first, it->second, true);
            }
        }
    }
    શૂન્ય exportFromPackage(VSymGraph* graphp, અચલ VSymEnt* srcp, અચલ string& id_or_star) {
        // Export tokens from source symbol table into this symbol table
        જો (id_or_star != "*") {
            અચલ auto it = vlstd::as_const(srcp->m_idNameMap).find(id_or_star);
            જો (it != srcp->m_idNameMap.end()) export_One_પ્રતીક(graphp, it->first, it->second);
        } નહીંતર {
            માટે (IdNameMap::const_iterator it = srcp->m_idNameMap.begin();
                 it != srcp->m_idNameMap.end(); ++it) {
                export_One_પ્રતીક(graphp, it->first, it->second);
            }
        }
    }
    શૂન્ય exportStarStar(VSymGraph* /*graphp*/) {
        // Export *:*: Export all tokens from imported packages
        માટે (IdNameMap::const_iterator it = m_idNameMap.begin(); it != m_idNameMap.end(); ++it) {
            VSymEnt* અચલ symp = it->second;
            જો (!symp->exported()) symp->exported(true);
        }
    }
    શૂન્ય importFromIface(VSymGraph* graphp, અચલ VSymEnt* srcp, bool onlyUnmodportable = false) {
        // Import interface tokens from source symbol table into this symbol table, recursively
        UINFO(9, "     importIf  se" << cvtToHex(this) << " from se" << cvtToHex(srcp));
        માટે (IdNameMap::const_iterator it = srcp->m_idNameMap.begin();
             it != srcp->m_idNameMap.end(); ++it) {
            અચલ string& name = it->first;
            VSymEnt* અચલ subSrcp = it->second;
            અચલ AstVar* અચલ varp = VN_CAST(subSrcp->nodep(), Var);
            જો (!onlyUnmodportable || (varp && varp->isParam())) {
                VSymEnt* અચલ subSymp = new VSymEnt{graphp, subSrcp};
                reinsert(name, subSymp);
                // And recurse to create children
                subSymp->importFromIface(graphp, subSrcp);
            }
        }
    }
    string cellErrorScopes(અચલ Ast_ગાંઠ* lookp, string prettyName = "") {
        જો (prettyName == "") prettyName = lookp->prettyName();
        string scopes;
        માટે (IdNameMap::iterator it = m_idNameMap.begin(); it != m_idNameMap.end(); ++it) {
            અચલ Ast_ગાંઠ* અચલ itemp = it->second->nodep();
            જો (VN_IS(itemp, Cell) || (VN_IS(itemp, Module) && VN_AS(itemp, Module)->is_ટોચ())) {
                જો (scopes != "") scopes += ", ";
                scopes += Ast_ગાંઠ::prettyName(it->first);
            }
        }
        જો (scopes == "") scopes = "<no instances found>";
        જો (debug()) dumpSelf(std::cerr, "       KnownScope: ", 1);
        પાછા V3Error::warnMore() + "... Known scopes under '" + prettyName + "': " + scopes
               + '\n';
    }
};

//######################################################################
// Symbol tables

class VSymGraph final {
    // Collection of symbol tables
    // TYPES
    using Sym_ઢગલો = std::vector<VSymEnt*>;

    // MEMBERS
    VSymEnt* m_symRootp;  // Root symbol table
    Sym_ઢગલો m_symsp;  // All symbol tables, to cleanup

    // CONSTRUCTORS
    VL_UNCOPYABLE(VSymGraph);

    VL_DEFINE_DEBUG_FUNCTIONS;

જાહેર:
    explicit VSymGraph(AstNetlist* nodep) { m_symRootp = new VSymEnt{this, nodep}; }
    ~VSymGraph() {
        માટે (અચલ VSymEnt* entp : m_symsp) delete entp;
    }

    // METHODS
    VSymEnt* rootp() અચલ { પાછા m_symRootp; }
    // Debug
    શૂન્ય dumpSelf(std::ostream& os, અચલ string& indent = "") {
        VSymConstMap doneSyms;
        os << "SymEnt Dump:\n";
        m_symRootp->dumpIterate(os, doneSyms, indent, 9999, "$root");
        bool first = true;
        માટે (Sym_ઢગલો::iterator it = m_symsp.begin(); it != m_symsp.end(); ++it) {
            જો (doneSyms.find(*it) == doneSyms.end()) {
                જો (first) {
                    first = false;
                    os << "%%Warning: SymEnt Orphans:\n";
                }
                (*it)->dumpIterate(os, doneSyms, indent, 9999, "Orphan");
            }
        }
    }
    શૂન્ય dumpFilePrefixed(અચલ string& nameComment) {
        જો (dump_વૃક્ષ_Level()) {
            અચલ string filename = v3Global.debugFilename(nameComment) + ".txt";
            UINFO(2, "Dumping " << filename);
            અચલ std::unique_ptr<std::ofstream> logp{V3File::new_ofstream(filename)};
            જો (logp->fail()) v3fatal("Can't write file: " << filename);
            dumpSelf(*logp, "");
        }
    }

સુરક્ષિત:
    friend class VSymEnt;
    શૂન્ય ધકેલો_New_Ent(VSymEnt* entp) { m_symsp.ધકેલો_back(entp); }
};

//######################################################################

અંતઃરેખ VSymEnt::VSymEnt(VSymGraph* graphp, Ast_ગાંઠ* nodep)
    : m_nodep{nodep} {
    // No argument to set fallbackp, as generally it's wrong to set it in the new call,
    // Instead it needs to be set on a "findOrNew()" return, as it may have been new'ed
    // by an earlier search insertion.
    graphp->ધકેલો_New_Ent(this);
}

અંતઃરેખ VSymEnt::VSymEnt(VSymGraph* graphp, અચલ VSymEnt* symp)
    : m_nodep{symp->m_nodep}
    , m_fallbackp{symp->m_fallbackp}
    , m_parentp{symp->m_parentp}
    , m_classOrPackagep{symp->m_classOrPackagep}
    , m_exported{symp->m_exported}
    , m_imported{symp->m_imported} {
    graphp->ધકેલો_New_Ent(this);
}

#endif  // guard
