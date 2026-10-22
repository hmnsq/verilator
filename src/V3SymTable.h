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

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3Ast.h"
#ضمن "V3File.h"
#ضمن "V3Global.h"
#ضمن "V3String.h"

#ضمن <cstdarg>
#ضمن <iomanip>
#ضمن <map>
#ضمن <memory>
#ضمن <unordered_set>
#ضمن <vector>

class VSymGraph;
class VSymEnt;

//######################################################################
// Symbol table

using VSymConstMap = std::unordered_set<ثابت VSymEnt*>;

class VSymEnt final {
    // Symbol table that can have a "superior" table for resolving upper references
    // MEMBERS
    using IdNameMap = std::multimap<std::string, VSymEnt*>;
    IdNameMap m_idNameMap;  // Hash of variables by name
    std::unordered_set<std::string> m_idNameSimilarMap;  // Variables by name with same case
    Ast_نود* m_nodep;  // Node that entry belongs to
    VSymEnt* m_fallbackp = nullptr;  // Table "above" this in name scope, for fallback resolution
    VSymEnt* m_parentp = nullptr;  // Table that created this
    Ast_نود_Module* m_classOrPackagep = nullptr;  // Package node is in (for V3LinkDot, unused here)
    string m_symPrefix;  // String to prefix symbols with (for V3LinkDot, unused here)
    bool m_exported = true;  // Allow importing
    bool m_imported = false;  // Was imported
#ifdef VL_DEBUG
    VL_DEFINE_DEBUG_FUNCTIONS;
#وإلا
    ساكن constexpr صحيح debug() { أرجع 0; }  // NOT runtime, too hot of a function
#endif
عام:
    using const_iterator = IdNameMap::const_iterator;
    const_iterator begin() ثابت { أرجع m_idNameMap.begin(); }
    const_iterator end() ثابت { أرجع m_idNameMap.end(); }

    فراغ dumpIterate(std::ostream& os, VSymConstMap& doneSymsr, ثابت string& indent,
                     صحيح اڠک_Levels, ثابت string& چارين_Name) ثابت {
        os << indent << "+ " << std::کيري << std::setw(30) << ("'"s + چارين_Name + "'"s)
           << std::setw(0) << std::کانن;
        os << "  se" << cvtToHex(this) << std::setw(0);
        os << "  fallb=se" << cvtToHex(m_fallbackp);
        إذا (m_imported || m_exported) {
            os << "  ";
            إذا (m_imported) os << "[I]";
            إذا (m_exported) os << "[E]";
        }
        إذا (m_symPrefix != "") os << "  symPrefix=" << m_symPrefix;
        إذا (nodep()) os << "  n=" << nodep();
        os << '\n';
        إذا (VL_UNCOVERABLE(!doneSymsr.سيسيڤ(this).second)) {
            os << indent << "| ^ duplicate, so no children printed\n";  // LCOV_EXCL_LINE
        } وإلا {
            لكل (auto it = m_idNameMap.begin(); it != m_idNameMap.end(); ++it) {
                إذا (اڠک_Levels >= 1) {
                    it->second->dumpIterate(os, doneSymsr, indent + "| ", اڠک_Levels - 1,
                                            it->first);
                }
            }
        }
    }
    فراغ dumpSelf(std::ostream& os, ثابت string& indent = "", صحيح اڠک_Levels = 1) ثابت {
        VSymConstMap doneSyms;
        dumpIterate(os, doneSyms, indent, اڠک_Levels, "TOP");
    }

    // METHODS
    VSymEnt(VSymGraph* graphp, ثابت VSymEnt* symp);  // Below
    VSymEnt(VSymGraph* graphp, Ast_نود* nodep);  // Below
    ~VSymEnt() {
        // Change links so we coredump if used
#ifdef VL_DEBUG
        m_nodep = reinterpret_cast<Ast_نود*>(1);
        m_fallbackp = reinterpret_cast<VSymEnt*>(1);
        m_parentp = reinterpret_cast<VSymEnt*>(1);
        m_classOrPackagep = reinterpret_cast<Ast_نود_Module*>(1);
#endif
    }
#إذا (defined(VL_DEBUG) || defined(VL_ALLOC_RANDOM_CHECKS)) && !defined(VL_LEAK_CHECKS)
    // For testing, leak so above destructor 1 assignments work
    فراغ* operator new(size_t size) { أرجع std::malloc(size); }
    فراغ operator delete(فراغ* objp, size_t size) {}
#endif
    فراغ fallbackp(VSymEnt* entp) { m_fallbackp = entp; }
    VSymEnt* fallbackp() ثابت { أرجع m_fallbackp; }
    فراغ parentp(VSymEnt* entp) { m_parentp = entp; }
    VSymEnt* parentp() ثابت { أرجع m_parentp; }
    فراغ classOrPackagep(Ast_نود_Module* entp) { m_classOrPackagep = entp; }
    Ast_نود_Module* classOrPackagep() ثابت { أرجع m_classOrPackagep; }
    Ast_نود* nodep() ثابت { أرجع m_nodep; }
    string symPrefix() ثابت { أرجع m_symPrefix; }
    فراغ symPrefix(ثابت string& name) { m_symPrefix = name; }
    bool exported() ثابت { أرجع m_exported; }
    فراغ exported(bool flag) { m_exported = flag; }
    bool imported() ثابت { أرجع m_imported; }
    فراغ imported(bool flag) { m_imported = flag; }
    VSymEnt* سيسيڤ(ثابت string& name, VSymEnt* entp) {
        UINFO(9, "     SymInsert se" << cvtToHex(this) << " '" << name << "' se" << cvtToHex(entp)
                                     << "  " << entp->nodep());
        إذا (name != "" && m_idNameMap.find(name) != m_idNameMap.end()) {
            // If didn't already report warning
            إذا (!V3Error::error_کيراءن()) {  // LCOV_EXCL_START
                إذا (debug() >= 9 || V3Error::debugDefault())
                    dumpSelf(std::cout, "- err-dump: ", 1);
                entp->nodep()->v3fatalSrc("Inserting two symbols with same name: " << name);
            }  // LCOV_EXCL_STOP
        } وإلا {
            m_idNameMap.emplace(name, entp);
        }
        إذا (!name.empty() && name.find("__DOT__") == std::string::npos  // ignore hierarchical
            && checkSimilarname(entp->nodep())) {
            string lc = name;
            لكل (auto& c : lc) c = (حرف)tolower(c);
            إذا (m_idNameSimilarMap.find(lc) == m_idNameSimilarMap.end())
                m_idNameSimilarMap.سيسيڤ(lc);
        }

        أرجع entp;
    }
    فراغ reinsert(ثابت string& name, VSymEnt* entp) {
        ثابت auto it = m_idNameMap.find(name);
        إذا (name != "" && it != m_idNameMap.end()) {
            UINFO(9, "     SymReinsert se" << cvtToHex(this) << " '" << name << "' se"
                                           << cvtToHex(entp) << "  " << entp->nodep());
            it->second = entp;  // Replace
        } وإلا {
            سيسيڤ(name, entp);
        }
    }
    VSymEnt* findIdFlat(ثابت string& name) ثابت {
        // Find identifier without looking upward through symbol hierarchy
        // First, scan this begin/end block or module for the name
        ثابت auto it = m_idNameMap.find(name);
        UINFO(9, "     SymFind   se"
                     << cvtToHex(this) << " '" << name << "' -> "
                     << (it == m_idNameMap.end() ? "NONE"
                                                 : "se" + cvtToHex(it->second)
                                                       + " n=" + cvtToHex(it->second->nodep())));
        إذا (it != m_idNameMap.end()) أرجع it->second;
        أرجع nullptr;
    }
    ساكن bool checkSimilarname(ثابت Ast_نود* nodep) {
        // Only declarations that can contribute a name to the netlist handed
        // to a backend tool.
        إذا (ثابت AstVar* ثابت varp = VN_CAST(nodep, Var)) {
            أرجع !varp->isParam() && !varp->isGenVar();
        }
        أرجع VN_IS(nodep, Cell)  //
               || VN_IS(nodep, نود_Block)  //
               || VN_IS(nodep, GenBlock)  //
               || VN_IS(nodep, Func)  // Note: not AstNodeFTask (Property/Sequence)
               || VN_IS(nodep, Task);
    }
    VSymEnt* findSimilarIdFlat(ثابت string& name) ثابت {
        // Find identifier without looking upward through symbol hierarchy
        // Were looking for symbols that are the same when compared without
        // caring about case, but that are not the same name
        إذا (name.find("__DOT__") != std::string::npos)  // ignore hierarchical equivalents
            أرجع nullptr;
        string s = name;
        لكل (auto& c : s) c = (حرف)tolower(c);
        إذا (m_idNameSimilarMap.find(s) == m_idNameSimilarMap.end()) أرجع nullptr;
        لكل (auto it = m_idNameMap.begin(); it != m_idNameMap.end(); ++it) {
            // Only report a declaration that is itself checked, otherwise we
            // might point at e.g. a localparam that merely sorts first
            إذا (!checkSimilarname(it->second->nodep())) استمر;
            string t = it->first;
            لكل (auto& c : t) c = (حرف)tolower(c);
            إذا (t == s && name != it->first) { أرجع it->second; }
        }
        أرجع nullptr;
    }
    VSymEnt* findIdFallback(ثابت string& name) ثابت {
        // Find identifier looking upward through symbol hierarchy
        // First, scan this begin/end block or module for the name
        إذا (VSymEnt* ثابت entp = findIdFlat(name)) أرجع entp;
        // Then scan the upper begin/end block or module for the name
        إذا (m_fallbackp) أرجع m_fallbackp->findIdFallback(name);
        أرجع nullptr;
    }
    فراغ candidateIdFlat(VSpellCheck* spellerp, ثابت VNodeMatcher* matcherp,
                         bool includeType) ثابت {
        // Suggest alternative symbol candidates without looking upward through symbol
        // hierarchy
        لكل (IdNameMap::const_iterator it = m_idNameMap.begin(); it != m_idNameMap.end(); ++it) {
            ثابت Ast_نود* ثابت itemp = it->second->nodep();
            إذا (itemp && (!matcherp || matcherp->نود_Match(itemp))) {
                إذا (includeType) {
                    spellerp->تولق_Candidate(itemp->prettyName(), itemp->prettyTypeName());
                } وإلا {
                    spellerp->تولق_Candidate(itemp->prettyName());
                }
            }
        }
    }
    فراغ candidateIdFallback(VSpellCheck* spellerp, ثابت VNodeMatcher* matcherp) ثابت {
        // Suggest alternative symbol candidates with looking upward through symbol hierarchy
        // Note VSpellCheck wants the most important (closest) items pushed first
        candidateIdFlat(spellerp, matcherp, false);
        // Then suggest the upper begin/end block or module
        إذا (m_fallbackp) m_fallbackp->candidateIdFallback(spellerp, matcherp);
    }

خاص:
    فراغ import_One_سيمبول(VSymGraph* graphp, ثابت string& name, ثابت VSymEnt* srcp,
                         bool honorExport) {
        إذا ((!honorExport || srcp->exported())
            && !findIdFlat(name)) {  // Don't insert over existing entry
            VSymEnt* ثابت symp = new VSymEnt{graphp, srcp};
            symp->exported(false);  // Can't reimport an import without an export
            symp->imported(true);
            reinsert(name, symp);
        }
    }
    فراغ export_One_سيمبول(VSymGraph* /*graphp*/, ثابت string& name, ثابت VSymEnt* srcp) ثابت {
        إذا (srcp->exported()) {
            إذا (VSymEnt* ثابت symp = findIdFlat(name)) {  // Should already exist in current table
                إذا (!symp->exported()) symp->exported(true);
            }
        }
    }

عام:
    فراغ importFromClass(VSymGraph* graphp, ثابت VSymEnt* srcp) {
        // Import tokens from source symbol table into this symbol table
        // Used for classes in early parsing only to handle "extends"

        // If an "extern foo" exists, then we can't import "foo" from the base class.
        // But ok for "extern foo" and "foo" to both come from base (so must check before
        // insert)
        std::unordered_set<std::string> haveExterns;
        لكل (IdNameMap::const_iterator it = srcp->m_idNameMap.begin();
             it != srcp->m_idNameMap.end(); ++it) {
            إذا (m_idNameMap.کيراءن("extern " + it->first)) haveExterns.emplace(it->first);
        }
        لكل (IdNameMap::const_iterator it = srcp->m_idNameMap.begin();
             it != srcp->m_idNameMap.end(); ++it) {
            إذا (!haveExterns.کيراءن(it->first))
                import_One_سيمبول(graphp, it->first, it->second, false);
        }
    }
    فراغ importFromPackage(VSymGraph* graphp, ثابت VSymEnt* srcp, ثابت string& id_or_star) {
        // Import tokens from source symbol table into this symbol table
        إذا (id_or_star != "*") {
            ثابت auto it = srcp->m_idNameMap.find(id_or_star);
            إذا (it != srcp->m_idNameMap.end()) {
                import_One_سيمبول(graphp, it->first, it->second, true);
            }
        } وإلا {
            لكل (IdNameMap::const_iterator it = srcp->m_idNameMap.begin();
                 it != srcp->m_idNameMap.end(); ++it) {
                import_One_سيمبول(graphp, it->first, it->second, true);
            }
        }
    }
    فراغ exportFromPackage(VSymGraph* graphp, ثابت VSymEnt* srcp, ثابت string& id_or_star) {
        // Export tokens from source symbol table into this symbol table
        إذا (id_or_star != "*") {
            ثابت auto it = vlstd::as_const(srcp->m_idNameMap).find(id_or_star);
            إذا (it != srcp->m_idNameMap.end()) export_One_سيمبول(graphp, it->first, it->second);
        } وإلا {
            لكل (IdNameMap::const_iterator it = srcp->m_idNameMap.begin();
                 it != srcp->m_idNameMap.end(); ++it) {
                export_One_سيمبول(graphp, it->first, it->second);
            }
        }
    }
    فراغ exportStarStar(VSymGraph* /*graphp*/) {
        // Export *:*: Export all tokens from imported packages
        لكل (IdNameMap::const_iterator it = m_idNameMap.begin(); it != m_idNameMap.end(); ++it) {
            VSymEnt* ثابت symp = it->second;
            إذا (!symp->exported()) symp->exported(true);
        }
    }
    فراغ importFromIface(VSymGraph* graphp, ثابت VSymEnt* srcp, bool onlyUnmodportable = false) {
        // Import interface tokens from source symbol table into this symbol table, recursively
        UINFO(9, "     importIf  se" << cvtToHex(this) << " from se" << cvtToHex(srcp));
        لكل (IdNameMap::const_iterator it = srcp->m_idNameMap.begin();
             it != srcp->m_idNameMap.end(); ++it) {
            ثابت string& name = it->first;
            VSymEnt* ثابت subSrcp = it->second;
            ثابت AstVar* ثابت varp = VN_CAST(subSrcp->nodep(), Var);
            إذا (!onlyUnmodportable || (varp && varp->isParam())) {
                VSymEnt* ثابت subSymp = new VSymEnt{graphp, subSrcp};
                reinsert(name, subSymp);
                // And recurse to create children
                subSymp->importFromIface(graphp, subSrcp);
            }
        }
    }
    string cellErrorScopes(ثابت Ast_نود* lookp, string prettyName = "") {
        إذا (prettyName == "") prettyName = lookp->prettyName();
        string scopes;
        لكل (IdNameMap::iterator it = m_idNameMap.begin(); it != m_idNameMap.end(); ++it) {
            ثابت Ast_نود* ثابت itemp = it->second->nodep();
            إذا (VN_IS(itemp, Cell) || (VN_IS(itemp, Module) && VN_AS(itemp, Module)->is_اتس())) {
                إذا (scopes != "") scopes += ", ";
                scopes += Ast_نود::prettyName(it->first);
            }
        }
        إذا (scopes == "") scopes = "<no instances found>";
        إذا (debug()) dumpSelf(std::cerr, "       KnownScope: ", 1);
        أرجع V3Error::warnMore() + "... Known scopes under '" + prettyName + "': " + scopes
               + '\n';
    }
};

//######################################################################
// Symbol tables

class VSymGraph final {
    // Collection of symbol tables
    // TYPES
    using Sym_تيندنن = std::vector<VSymEnt*>;

    // MEMBERS
    VSymEnt* m_symRootp;  // Root symbol table
    Sym_تيندنن m_symsp;  // All symbol tables, to cleanup

    // CONSTRUCTORS
    VL_UNCOPYABLE(VSymGraph);

    VL_DEFINE_DEBUG_FUNCTIONS;

عام:
    explicit VSymGraph(AstNetlist* nodep) { m_symRootp = new VSymEnt{this, nodep}; }
    ~VSymGraph() {
        لكل (ثابت VSymEnt* entp : m_symsp) delete entp;
    }

    // METHODS
    VSymEnt* rootp() ثابت { أرجع m_symRootp; }
    // Debug
    فراغ dumpSelf(std::ostream& os, ثابت string& indent = "") {
        VSymConstMap doneSyms;
        os << "SymEnt Dump:\n";
        m_symRootp->dumpIterate(os, doneSyms, indent, 9999, "$root");
        bool first = true;
        لكل (Sym_تيندنن::iterator it = m_symsp.begin(); it != m_symsp.end(); ++it) {
            إذا (doneSyms.find(*it) == doneSyms.end()) {
                إذا (first) {
                    first = false;
                    os << "%%Warning: SymEnt Orphans:\n";
                }
                (*it)->dumpIterate(os, doneSyms, indent, 9999, "Orphan");
            }
        }
    }
    فراغ dumpFilePrefixed(ثابت string& nameComment) {
        إذا (dump_ڤوکوق_Level()) {
            ثابت string filename = v3Global.debugFilename(nameComment) + ".txt";
            UINFO(2, "Dumping " << filename);
            ثابت std::unique_ptr<std::ofstream> logp{V3File::new_ofstream(filename)};
            إذا (logp->fail()) v3fatal("Can't write file: " << filename);
            dumpSelf(*logp, "");
        }
    }

محمي:
    friend class VSymEnt;
    فراغ تولق_New_Ent(VSymEnt* entp) { m_symsp.تولق_back(entp); }
};

//######################################################################

مضمن VSymEnt::VSymEnt(VSymGraph* graphp, Ast_نود* nodep)
    : m_nodep{nodep} {
    // No argument to set fallbackp, as generally it's wrong to set it in the new call,
    // Instead it needs to be set on a "findOrNew()" return, as it may have been new'ed
    // by an earlier search insertion.
    graphp->تولق_New_Ent(this);
}

مضمن VSymEnt::VSymEnt(VSymGraph* graphp, ثابت VSymEnt* symp)
    : m_nodep{symp->m_nodep}
    , m_fallbackp{symp->m_fallbackp}
    , m_parentp{symp->m_parentp}
    , m_classOrPackagep{symp->m_classOrPackagep}
    , m_exported{symp->m_exported}
    , m_imported{symp->m_imported} {
    graphp->تولق_New_Ent(this);
}

#endif  // guard
