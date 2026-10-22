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

#ଅନ୍ତର୍ଭାଵ "config_build.h"
#ଅନ୍ତର୍ଭାଵ "verilatedos.h"

#ଅନ୍ତର୍ଭାଵ "V3Ast.h"
#ଅନ୍ତର୍ଭାଵ "V3File.h"
#ଅନ୍ତର୍ଭାଵ "V3Global.h"
#ଅନ୍ତର୍ଭାଵ "V3String.h"

#ଅନ୍ତର୍ଭାଵ <cstdarg>
#ଅନ୍ତର୍ଭାଵ <iomanip>
#ଅନ୍ତର୍ଭାଵ <map>
#ଅନ୍ତର୍ଭାଵ <memory>
#ଅନ୍ତର୍ଭାଵ <unordered_set>
#ଅନ୍ତର୍ଭାଵ <vector>

class VSymGraph;
class VSymEnt;

//######################################################################
// Symbol table

using VSymConstMap = std::unordered_set<ଅଚଳ VSymEnt*>;

class VSymEnt final {
    // Symbol table that can have a "superior" table for resolving upper references
    // MEMBERS
    using IdNameMap = std::multimap<std::string, VSymEnt*>;
    IdNameMap m_idNameMap;  // Hash of variables by name
    std::unordered_set<std::string> m_idNameSimilarMap;  // Variables by name with same case
    Ast_ଗଣ୍ଠି* m_nodep;  // Node that entry belongs to
    VSymEnt* m_fallbackp = nullptr;  // Table "above" this in name scope, for fallback resolution
    VSymEnt* m_parentp = nullptr;  // Table that created this
    Ast_ଗଣ୍ଠି_Module* m_classOrPackagep = nullptr;  // Package node is in (for V3LinkDot, unused here)
    string m_symPrefix;  // String to prefix symbols with (for V3LinkDot, unused here)
    bool m_exported = true;  // Allow importing
    bool m_imported = false;  // Was imported
#ifdef VL_DEBUG
    VL_DEFINE_DEBUG_FUNCTIONS;
#ଅନ୍ୟଥା
    ସ୍ଥିର constexpr ପୂର୍ଣ୍ଣସଂଖ୍ୟା debug() { ଫେରନ୍ତୁ 0; }  // NOT runtime, too hot of a function
#endif
ସର୍ବସାଧାରଣ:
    using const_iterator = IdNameMap::const_iterator;
    const_iterator begin() ଅଚଳ { ଫେରନ୍ତୁ m_idNameMap.begin(); }
    const_iterator end() ଅଚଳ { ଫେରନ୍ତୁ m_idNameMap.end(); }

    ଶୂନ୍ୟ dumpIterate(std::ostream& os, VSymConstMap& doneSymsr, ଅଚଳ string& indent,
                     ପୂର୍ଣ୍ଣସଂଖ୍ୟା ଅଙ୍କ_Levels, ଅଚଳ string& ସନ୍ଧାନ_Name) ଅଚଳ {
        os << indent << "+ " << std::ବାମ << std::setw(30) << ("'"s + ସନ୍ଧାନ_Name + "'"s)
           << std::setw(0) << std::ଡାହାଣ;
        os << "  se" << cvtToHex(this) << std::setw(0);
        os << "  fallb=se" << cvtToHex(m_fallbackp);
        ଯଦି (m_imported || m_exported) {
            os << "  ";
            ଯଦି (m_imported) os << "[I]";
            ଯଦି (m_exported) os << "[E]";
        }
        ଯଦି (m_symPrefix != "") os << "  symPrefix=" << m_symPrefix;
        ଯଦି (nodep()) os << "  n=" << nodep();
        os << '\n';
        ଯଦି (VL_UNCOVERABLE(!doneSymsr.ଭର୍ତ୍ତି(this).second)) {
            os << indent << "| ^ duplicate, so no children printed\n";  // LCOV_EXCL_LINE
        } ଅନ୍ୟଥା {
            ପାଇଁ (auto it = m_idNameMap.begin(); it != m_idNameMap.end(); ++it) {
                ଯଦି (ଅଙ୍କ_Levels >= 1) {
                    it->second->dumpIterate(os, doneSymsr, indent + "| ", ଅଙ୍କ_Levels - 1,
                                            it->first);
                }
            }
        }
    }
    ଶୂନ୍ୟ dumpSelf(std::ostream& os, ଅଚଳ string& indent = "", ପୂର୍ଣ୍ଣସଂଖ୍ୟା ଅଙ୍କ_Levels = 1) ଅଚଳ {
        VSymConstMap doneSyms;
        dumpIterate(os, doneSyms, indent, ଅଙ୍କ_Levels, "TOP");
    }

    // METHODS
    VSymEnt(VSymGraph* graphp, ଅଚଳ VSymEnt* symp);  // Below
    VSymEnt(VSymGraph* graphp, Ast_ଗଣ୍ଠି* nodep);  // Below
    ~VSymEnt() {
        // Change links so we coredump if used
#ifdef VL_DEBUG
        m_nodep = reinterpret_cast<Ast_ଗଣ୍ଠି*>(1);
        m_fallbackp = reinterpret_cast<VSymEnt*>(1);
        m_parentp = reinterpret_cast<VSymEnt*>(1);
        m_classOrPackagep = reinterpret_cast<Ast_ଗଣ୍ଠି_Module*>(1);
#endif
    }
#ଯଦି (defined(VL_DEBUG) || defined(VL_ALLOC_RANDOM_CHECKS)) && !defined(VL_LEAK_CHECKS)
    // For testing, leak so above destructor 1 assignments work
    ଶୂନ୍ୟ* operator new(size_t size) { ଫେରନ୍ତୁ std::malloc(size); }
    ଶୂନ୍ୟ operator delete(ଶୂନ୍ୟ* objp, size_t size) {}
#endif
    ଶୂନ୍ୟ fallbackp(VSymEnt* entp) { m_fallbackp = entp; }
    VSymEnt* fallbackp() ଅଚଳ { ଫେରନ୍ତୁ m_fallbackp; }
    ଶୂନ୍ୟ parentp(VSymEnt* entp) { m_parentp = entp; }
    VSymEnt* parentp() ଅଚଳ { ଫେରନ୍ତୁ m_parentp; }
    ଶୂନ୍ୟ classOrPackagep(Ast_ଗଣ୍ଠି_Module* entp) { m_classOrPackagep = entp; }
    Ast_ଗଣ୍ଠି_Module* classOrPackagep() ଅଚଳ { ଫେରନ୍ତୁ m_classOrPackagep; }
    Ast_ଗଣ୍ଠି* nodep() ଅଚଳ { ଫେରନ୍ତୁ m_nodep; }
    string symPrefix() ଅଚଳ { ଫେରନ୍ତୁ m_symPrefix; }
    ଶୂନ୍ୟ symPrefix(ଅଚଳ string& name) { m_symPrefix = name; }
    bool exported() ଅଚଳ { ଫେରନ୍ତୁ m_exported; }
    ଶୂନ୍ୟ exported(bool flag) { m_exported = flag; }
    bool imported() ଅଚଳ { ଫେରନ୍ତୁ m_imported; }
    ଶୂନ୍ୟ imported(bool flag) { m_imported = flag; }
    VSymEnt* ଭର୍ତ୍ତି(ଅଚଳ string& name, VSymEnt* entp) {
        UINFO(9, "     SymInsert se" << cvtToHex(this) << " '" << name << "' se" << cvtToHex(entp)
                                     << "  " << entp->nodep());
        ଯଦି (name != "" && m_idNameMap.find(name) != m_idNameMap.end()) {
            // If didn't already report warning
            ଯଦି (!V3Error::error_ଗଣନା()) {  // LCOV_EXCL_START
                ଯଦି (debug() >= 9 || V3Error::debugDefault())
                    dumpSelf(std::cout, "- err-dump: ", 1);
                entp->nodep()->v3fatalSrc("Inserting two symbols with same name: " << name);
            }  // LCOV_EXCL_STOP
        } ଅନ୍ୟଥା {
            m_idNameMap.emplace(name, entp);
        }
        ଯଦି (!name.empty() && name.find("__DOT__") == std::string::npos  // ignore hierarchical
            && checkSimilarname(entp->nodep())) {
            string lc = name;
            ପାଇଁ (auto& c : lc) c = (ଅକ୍ଷର)tolower(c);
            ଯଦି (m_idNameSimilarMap.find(lc) == m_idNameSimilarMap.end())
                m_idNameSimilarMap.ଭର୍ତ୍ତି(lc);
        }

        ଫେରନ୍ତୁ entp;
    }
    ଶୂନ୍ୟ reinsert(ଅଚଳ string& name, VSymEnt* entp) {
        ଅଚଳ auto it = m_idNameMap.find(name);
        ଯଦି (name != "" && it != m_idNameMap.end()) {
            UINFO(9, "     SymReinsert se" << cvtToHex(this) << " '" << name << "' se"
                                           << cvtToHex(entp) << "  " << entp->nodep());
            it->second = entp;  // Replace
        } ଅନ୍ୟଥା {
            ଭର୍ତ୍ତି(name, entp);
        }
    }
    VSymEnt* findIdFlat(ଅଚଳ string& name) ଅଚଳ {
        // Find identifier without looking upward through symbol hierarchy
        // First, scan this begin/end block or module for the name
        ଅଚଳ auto it = m_idNameMap.find(name);
        UINFO(9, "     SymFind   se"
                     << cvtToHex(this) << " '" << name << "' -> "
                     << (it == m_idNameMap.end() ? "NONE"
                                                 : "se" + cvtToHex(it->second)
                                                       + " n=" + cvtToHex(it->second->nodep())));
        ଯଦି (it != m_idNameMap.end()) ଫେରନ୍ତୁ it->second;
        ଫେରନ୍ତୁ nullptr;
    }
    ସ୍ଥିର bool checkSimilarname(ଅଚଳ Ast_ଗଣ୍ଠି* nodep) {
        // Only declarations that can contribute a name to the netlist handed
        // to a backend tool.
        ଯଦି (ଅଚଳ AstVar* ଅଚଳ varp = VN_CAST(nodep, Var)) {
            ଫେରନ୍ତୁ !varp->isParam() && !varp->isGenVar();
        }
        ଫେରନ୍ତୁ VN_IS(nodep, Cell)  //
               || VN_IS(nodep, ଗଣ୍ଠି_Block)  //
               || VN_IS(nodep, GenBlock)  //
               || VN_IS(nodep, Func)  // Note: not AstNodeFTask (Property/Sequence)
               || VN_IS(nodep, Task);
    }
    VSymEnt* findSimilarIdFlat(ଅଚଳ string& name) ଅଚଳ {
        // Find identifier without looking upward through symbol hierarchy
        // Were looking for symbols that are the same when compared without
        // caring about case, but that are not the same name
        ଯଦି (name.find("__DOT__") != std::string::npos)  // ignore hierarchical equivalents
            ଫେରନ୍ତୁ nullptr;
        string s = name;
        ପାଇଁ (auto& c : s) c = (ଅକ୍ଷର)tolower(c);
        ଯଦି (m_idNameSimilarMap.find(s) == m_idNameSimilarMap.end()) ଫେରନ୍ତୁ nullptr;
        ପାଇଁ (auto it = m_idNameMap.begin(); it != m_idNameMap.end(); ++it) {
            // Only report a declaration that is itself checked, otherwise we
            // might point at e.g. a localparam that merely sorts first
            ଯଦି (!checkSimilarname(it->second->nodep())) ଜାରି;
            string t = it->first;
            ପାଇଁ (auto& c : t) c = (ଅକ୍ଷର)tolower(c);
            ଯଦି (t == s && name != it->first) { ଫେରନ୍ତୁ it->second; }
        }
        ଫେରନ୍ତୁ nullptr;
    }
    VSymEnt* findIdFallback(ଅଚଳ string& name) ଅଚଳ {
        // Find identifier looking upward through symbol hierarchy
        // First, scan this begin/end block or module for the name
        ଯଦି (VSymEnt* ଅଚଳ entp = findIdFlat(name)) ଫେରନ୍ତୁ entp;
        // Then scan the upper begin/end block or module for the name
        ଯଦି (m_fallbackp) ଫେରନ୍ତୁ m_fallbackp->findIdFallback(name);
        ଫେରନ୍ତୁ nullptr;
    }
    ଶୂନ୍ୟ candidateIdFlat(VSpellCheck* spellerp, ଅଚଳ VNodeMatcher* matcherp,
                         bool includeType) ଅଚଳ {
        // Suggest alternative symbol candidates without looking upward through symbol
        // hierarchy
        ପାଇଁ (IdNameMap::const_iterator it = m_idNameMap.begin(); it != m_idNameMap.end(); ++it) {
            ଅଚଳ Ast_ଗଣ୍ଠି* ଅଚଳ itemp = it->second->nodep();
            ଯଦି (itemp && (!matcherp || matcherp->ଗଣ୍ଠି_Match(itemp))) {
                ଯଦି (includeType) {
                    spellerp->ଠେଲ_Candidate(itemp->prettyName(), itemp->prettyTypeName());
                } ଅନ୍ୟଥା {
                    spellerp->ଠେଲ_Candidate(itemp->prettyName());
                }
            }
        }
    }
    ଶୂନ୍ୟ candidateIdFallback(VSpellCheck* spellerp, ଅଚଳ VNodeMatcher* matcherp) ଅଚଳ {
        // Suggest alternative symbol candidates with looking upward through symbol hierarchy
        // Note VSpellCheck wants the most important (closest) items pushed first
        candidateIdFlat(spellerp, matcherp, false);
        // Then suggest the upper begin/end block or module
        ଯଦି (m_fallbackp) m_fallbackp->candidateIdFallback(spellerp, matcherp);
    }

ବ୍ୟକ୍ତିଗତ:
    ଶୂନ୍ୟ import_One_ପ୍ରତୀକ(VSymGraph* graphp, ଅଚଳ string& name, ଅଚଳ VSymEnt* srcp,
                         bool honorExport) {
        ଯଦି ((!honorExport || srcp->exported())
            && !findIdFlat(name)) {  // Don't insert over existing entry
            VSymEnt* ଅଚଳ symp = new VSymEnt{graphp, srcp};
            symp->exported(false);  // Can't reimport an import without an export
            symp->imported(true);
            reinsert(name, symp);
        }
    }
    ଶୂନ୍ୟ export_One_ପ୍ରତୀକ(VSymGraph* /*graphp*/, ଅଚଳ string& name, ଅଚଳ VSymEnt* srcp) ଅଚଳ {
        ଯଦି (srcp->exported()) {
            ଯଦି (VSymEnt* ଅଚଳ symp = findIdFlat(name)) {  // Should already exist in current table
                ଯଦି (!symp->exported()) symp->exported(true);
            }
        }
    }

ସର୍ବସାଧାରଣ:
    ଶୂନ୍ୟ importFromClass(VSymGraph* graphp, ଅଚଳ VSymEnt* srcp) {
        // Import tokens from source symbol table into this symbol table
        // Used for classes in early parsing only to handle "extends"

        // If an "extern foo" exists, then we can't import "foo" from the base class.
        // But ok for "extern foo" and "foo" to both come from base (so must check before
        // insert)
        std::unordered_set<std::string> haveExterns;
        ପାଇଁ (IdNameMap::const_iterator it = srcp->m_idNameMap.begin();
             it != srcp->m_idNameMap.end(); ++it) {
            ଯଦି (m_idNameMap.ଗଣନା_2("extern " + it->first)) haveExterns.emplace(it->first);
        }
        ପାଇଁ (IdNameMap::const_iterator it = srcp->m_idNameMap.begin();
             it != srcp->m_idNameMap.end(); ++it) {
            ଯଦି (!haveExterns.ଗଣନା_2(it->first))
                import_One_ପ୍ରତୀକ(graphp, it->first, it->second, false);
        }
    }
    ଶୂନ୍ୟ importFromPackage(VSymGraph* graphp, ଅଚଳ VSymEnt* srcp, ଅଚଳ string& id_or_star) {
        // Import tokens from source symbol table into this symbol table
        ଯଦି (id_or_star != "*") {
            ଅଚଳ auto it = srcp->m_idNameMap.find(id_or_star);
            ଯଦି (it != srcp->m_idNameMap.end()) {
                import_One_ପ୍ରତୀକ(graphp, it->first, it->second, true);
            }
        } ଅନ୍ୟଥା {
            ପାଇଁ (IdNameMap::const_iterator it = srcp->m_idNameMap.begin();
                 it != srcp->m_idNameMap.end(); ++it) {
                import_One_ପ୍ରତୀକ(graphp, it->first, it->second, true);
            }
        }
    }
    ଶୂନ୍ୟ exportFromPackage(VSymGraph* graphp, ଅଚଳ VSymEnt* srcp, ଅଚଳ string& id_or_star) {
        // Export tokens from source symbol table into this symbol table
        ଯଦି (id_or_star != "*") {
            ଅଚଳ auto it = vlstd::as_const(srcp->m_idNameMap).find(id_or_star);
            ଯଦି (it != srcp->m_idNameMap.end()) export_One_ପ୍ରତୀକ(graphp, it->first, it->second);
        } ଅନ୍ୟଥା {
            ପାଇଁ (IdNameMap::const_iterator it = srcp->m_idNameMap.begin();
                 it != srcp->m_idNameMap.end(); ++it) {
                export_One_ପ୍ରତୀକ(graphp, it->first, it->second);
            }
        }
    }
    ଶୂନ୍ୟ exportStarStar(VSymGraph* /*graphp*/) {
        // Export *:*: Export all tokens from imported packages
        ପାଇଁ (IdNameMap::const_iterator it = m_idNameMap.begin(); it != m_idNameMap.end(); ++it) {
            VSymEnt* ଅଚଳ symp = it->second;
            ଯଦି (!symp->exported()) symp->exported(true);
        }
    }
    ଶୂନ୍ୟ importFromIface(VSymGraph* graphp, ଅଚଳ VSymEnt* srcp, bool onlyUnmodportable = false) {
        // Import interface tokens from source symbol table into this symbol table, recursively
        UINFO(9, "     importIf  se" << cvtToHex(this) << " from se" << cvtToHex(srcp));
        ପାଇଁ (IdNameMap::const_iterator it = srcp->m_idNameMap.begin();
             it != srcp->m_idNameMap.end(); ++it) {
            ଅଚଳ string& name = it->first;
            VSymEnt* ଅଚଳ subSrcp = it->second;
            ଅଚଳ AstVar* ଅଚଳ varp = VN_CAST(subSrcp->nodep(), Var);
            ଯଦି (!onlyUnmodportable || (varp && varp->isParam())) {
                VSymEnt* ଅଚଳ subSymp = new VSymEnt{graphp, subSrcp};
                reinsert(name, subSymp);
                // And recurse to create children
                subSymp->importFromIface(graphp, subSrcp);
            }
        }
    }
    string cellErrorScopes(ଅଚଳ Ast_ଗଣ୍ଠି* lookp, string prettyName = "") {
        ଯଦି (prettyName == "") prettyName = lookp->prettyName();
        string scopes;
        ପାଇଁ (IdNameMap::iterator it = m_idNameMap.begin(); it != m_idNameMap.end(); ++it) {
            ଅଚଳ Ast_ଗଣ୍ଠି* ଅଚଳ itemp = it->second->nodep();
            ଯଦି (VN_IS(itemp, Cell) || (VN_IS(itemp, Module) && VN_AS(itemp, Module)->is_ଶୀର୍ଷ())) {
                ଯଦି (scopes != "") scopes += ", ";
                scopes += Ast_ଗଣ୍ଠି::prettyName(it->first);
            }
        }
        ଯଦି (scopes == "") scopes = "<no instances found>";
        ଯଦି (debug()) dumpSelf(std::cerr, "       KnownScope: ", 1);
        ଫେରନ୍ତୁ V3Error::warnMore() + "... Known scopes under '" + prettyName + "': " + scopes
               + '\n';
    }
};

//######################################################################
// Symbol tables

class VSymGraph final {
    // Collection of symbol tables
    // TYPES
    using Sym_ଗଦା = std::vector<VSymEnt*>;

    // MEMBERS
    VSymEnt* m_symRootp;  // Root symbol table
    Sym_ଗଦା m_symsp;  // All symbol tables, to cleanup

    // CONSTRUCTORS
    VL_UNCOPYABLE(VSymGraph);

    VL_DEFINE_DEBUG_FUNCTIONS;

ସର୍ବସାଧାରଣ:
    explicit VSymGraph(AstNetlist* nodep) { m_symRootp = new VSymEnt{this, nodep}; }
    ~VSymGraph() {
        ପାଇଁ (ଅଚଳ VSymEnt* entp : m_symsp) delete entp;
    }

    // METHODS
    VSymEnt* rootp() ଅଚଳ { ଫେରନ୍ତୁ m_symRootp; }
    // Debug
    ଶୂନ୍ୟ dumpSelf(std::ostream& os, ଅଚଳ string& indent = "") {
        VSymConstMap doneSyms;
        os << "SymEnt Dump:\n";
        m_symRootp->dumpIterate(os, doneSyms, indent, 9999, "$root");
        bool first = true;
        ପାଇଁ (Sym_ଗଦା::iterator it = m_symsp.begin(); it != m_symsp.end(); ++it) {
            ଯଦି (doneSyms.find(*it) == doneSyms.end()) {
                ଯଦି (first) {
                    first = false;
                    os << "%%Warning: SymEnt Orphans:\n";
                }
                (*it)->dumpIterate(os, doneSyms, indent, 9999, "Orphan");
            }
        }
    }
    ଶୂନ୍ୟ dumpFilePrefixed(ଅଚଳ string& nameComment) {
        ଯଦି (dump_ବୃକ୍ଷ_Level()) {
            ଅଚଳ string filename = v3Global.debugFilename(nameComment) + ".txt";
            UINFO(2, "Dumping " << filename);
            ଅଚଳ std::unique_ptr<std::ofstream> logp{V3File::new_ofstream(filename)};
            ଯଦି (logp->fail()) v3fatal("Can't write file: " << filename);
            dumpSelf(*logp, "");
        }
    }

ସୁରକ୍ଷିତ:
    friend class VSymEnt;
    ଶୂନ୍ୟ ଠେଲ_New_Ent(VSymEnt* entp) { m_symsp.ଠେଲ_back(entp); }
};

//######################################################################

ଧାଡ଼ିଭିତରେ VSymEnt::VSymEnt(VSymGraph* graphp, Ast_ଗଣ୍ଠି* nodep)
    : m_nodep{nodep} {
    // No argument to set fallbackp, as generally it's wrong to set it in the new call,
    // Instead it needs to be set on a "findOrNew()" return, as it may have been new'ed
    // by an earlier search insertion.
    graphp->ଠେଲ_New_Ent(this);
}

ଧାଡ଼ିଭିତରେ VSymEnt::VSymEnt(VSymGraph* graphp, ଅଚଳ VSymEnt* symp)
    : m_nodep{symp->m_nodep}
    , m_fallbackp{symp->m_fallbackp}
    , m_parentp{symp->m_parentp}
    , m_classOrPackagep{symp->m_classOrPackagep}
    , m_exported{symp->m_exported}
    , m_imported{symp->m_imported} {
    graphp->ଠେଲ_New_Ent(this);
}

#endif  // guard
