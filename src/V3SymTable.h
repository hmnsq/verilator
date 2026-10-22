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

#నిర్వచితం_కాకపోతే VERILATOR_V3SYMTABLE_H_
#నిర్వచించు VERILATOR_V3SYMTABLE_H_

#అన్తర్భావ "config_build.h"
#అన్తర్భావ "verilatedos.h"

#అన్తర్భావ "V3Ast.h"
#అన్తర్భావ "V3File.h"
#అన్తర్భావ "V3Global.h"
#అన్తర్భావ "V3String.h"

#అన్తర్భావ <cstdarg>
#అన్తర్భావ <iomanip>
#అన్తర్భావ <map>
#అన్తర్భావ <memory>
#అన్తర్భావ <unordered_set>
#అన్తర్భావ <vector>

class VSym_లేఖాచిత్రం;
class VSymEnt;

//######################################################################
// Symbol table

using VSymConstMap = std::unordered_సమితి<స్థిరరాశి VSymEnt*>;

class VSymEnt final {
    // Symbol table that can have a "superior" table for resolving upper references
    // MEMBERS
    using IdNameMap = std::multimap<std::string, VSymEnt*>;
    IdNameMap m_idNameMap;  // Hash of variables by name
    std::unordered_సమితి<std::string> m_idNameSimilarMap;  // Variables by name with same case
    Ast_కణుపు* m_nodep;  // Node that entry belongs to
    VSymEnt* m_fallbackp = nullptr;  // Table "above" this in name scope, for fallback resolution
    VSymEnt* m_parentp = nullptr;  // Table that created this
    Ast_కణుపు_Module* m_classOrPackagep = nullptr;  // Package node is in (for V3LinkDot, unused here)
    string m_sym_ఉపసర్గ;  // String to prefix symbols with (for V3LinkDot, unused here)
    bool m_exported = true;  // Allow importing
    bool m_imported = false;  // Was imported
#నిర్వచితమైతే VL_DEBUG
    VL_DEFINE_DEBUG_FUNCTIONS;
#లేకపోతే
    స్థిర constexpr పూర్ణాంకం debug() { తిరిగి 0; }  // NOT runtime, too hot of a function
#అయితే_ముగింపు
బహిరంగ:
    using const_iterator = IdNameMap::const_iterator;
    const_iterator begin() స్థిరరాశి { తిరిగి m_idNameMap.begin(); }
    const_iterator చివర() స్థిరరాశి { తిరిగి m_idNameMap.చివర(); }

    శూన్య dumpIterate(std::ostream& os, VSymConstMap& పూర్తి_Symsr, స్థిరరాశి string& indent,
                     పూర్ణాంకం అంకె_స్థాయిలు, స్థిరరాశి string& శోధన_Name) స్థిరరాశి {
        os << indent << "+ " << std::ఎడమ_2 << std::setw(30) << ("'"s + శోధన_Name + "'"s)
           << std::setw(0) << std::కుడి_2;
        os << "  se" << cvt_గమ్యం_Hex(this) << std::setw(0);
        os << "  fallb=se" << cvt_గమ్యం_Hex(m_fallbackp);
        ఒకవేళ (m_imported || m_exported) {
            os << "  ";
            ఒకవేళ (m_imported) os << "[I]";
            ఒకవేళ (m_exported) os << "[E]";
        }
        ఒకవేళ (m_sym_ఉపసర్గ != "") os << "  symPrefix=" << m_sym_ఉపసర్గ;
        ఒకవేళ (nodep()) os << "  n=" << nodep();
        os << '\n';
        ఒకవేళ (VL_UNCOVERABLE(!పూర్తి_Symsr.చొప్పించు(this).రెండవ)) {
            os << indent << "| ^ duplicate, so no children printed\n";  // LCOV_EXCL_LINE
        } లేకపోతే {
            కొరకు (స్వయంచాలక it = m_idNameMap.begin(); it != m_idNameMap.చివర(); ++it) {
                ఒకవేళ (అంకె_స్థాయిలు >= 1) {
                    it->రెండవ->dumpIterate(os, పూర్తి_Symsr, indent + "| ", అంకె_స్థాయిలు - 1,
                                            it->మొదటి);
                }
            }
        }
    }
    శూన్య dumpSelf(std::ostream& os, స్థిరరాశి string& indent = "", పూర్ణాంకం అంకె_స్థాయిలు = 1) స్థిరరాశి {
        VSymConstMap పూర్తి_Syms;
        dumpIterate(os, పూర్తి_Syms, indent, అంకె_స్థాయిలు, "TOP");
    }

    // METHODS
    VSymEnt(VSym_లేఖాచిత్రం* graphp, స్థిరరాశి VSymEnt* symp);  //క్రింద
    VSymEnt(VSym_లేఖాచిత్రం* graphp, Ast_కణుపు* nodep);  //క్రింద
    ~VSymEnt() {
        // Change links so we coredump if used
#నిర్వచితమైతే VL_DEBUG
        m_nodep = reinterpret_cast<Ast_కణుపు*>(1);
        m_fallbackp = reinterpret_cast<VSymEnt*>(1);
        m_parentp = reinterpret_cast<VSymEnt*>(1);
        m_classOrPackagep = reinterpret_cast<Ast_కణుపు_Module*>(1);
#అయితే_ముగింపు
    }
#ఒకవేళ (defined(VL_DEBUG) || defined(VL_ALLOC_RANDOM_CHECKS)) && !defined(VL_LEAK_CHECKS)
    // For testing, leak so above destructor 1 assignments work
    శూన్య* క్రియాకారకం new(కొలత_t కొలత) { తిరిగి std::జ్ఞాపకం_ఇవ్వు(కొలత); }
    శూన్య క్రియాకారకం delete(శూన్య* objp, కొలత_t కొలత) {}
#అయితే_ముగింపు
    శూన్య fallbackp(VSymEnt* entp) { m_fallbackp = entp; }
    VSymEnt* fallbackp() స్థిరరాశి { తిరిగి m_fallbackp; }
    శూన్య parentp(VSymEnt* entp) { m_parentp = entp; }
    VSymEnt* parentp() స్థిరరాశి { తిరిగి m_parentp; }
    శూన్య classOrPackagep(Ast_కణుపు_Module* entp) { m_classOrPackagep = entp; }
    Ast_కణుపు_Module* classOrPackagep() స్థిరరాశి { తిరిగి m_classOrPackagep; }
    Ast_కణుపు* nodep() స్థిరరాశి { తిరిగి m_nodep; }
    string sym_ఉపసర్గ() స్థిరరాశి { తిరిగి m_sym_ఉపసర్గ; }
    శూన్య sym_ఉపసర్గ(స్థిరరాశి string& name) { m_sym_ఉపసర్గ = name; }
    bool exported() స్థిరరాశి { తిరిగి m_exported; }
    శూన్య exported(bool flag) { m_exported = flag; }
    bool imported() స్థిరరాశి { తిరిగి m_imported; }
    శూన్య imported(bool flag) { m_imported = flag; }
    VSymEnt* చొప్పించు(స్థిరరాశి string& name, VSymEnt* entp) {
        UINFO(9, "     SymInsert se" << cvt_గమ్యం_Hex(this) << " '" << name << "' se" << cvt_గమ్యం_Hex(entp)
                                     << "  " << entp->nodep());
        ఒకవేళ (name != "" && m_idNameMap.కనుగొను(name) != m_idNameMap.చివర()) {
            // If didn't already report warning
            ఒకవేళ (!V3Error::error_లెక్క()) {  // LCOV_EXCL_START
                ఒకవేళ (debug() >= 9 || V3Error::debugDefault())
                    dumpSelf(std::cout, "- err-dump: ", 1);
                entp->nodep()->v3fatalSrc("Inserting two symbols with same name: " << name);
            }  // LCOV_EXCL_STOP
        } లేకపోతే {
            m_idNameMap.emplace(name, entp);
        }
        ఒకవేళ (!name.ఖాళీలు() && name.కనుగొను("__DOT__") == std::string::npos  // ignore hierarchical
            && checkSimilarname(entp->nodep())) {
            string lc = name;
            కొరకు (స్వయంచాలక& c : lc) c = (అక్షరం)tolower(c);
            ఒకవేళ (m_idNameSimilarMap.కనుగొను(lc) == m_idNameSimilarMap.చివర())
                m_idNameSimilarMap.చొప్పించు(lc);
        }

        తిరిగి entp;
    }
    శూన్య reinsert(స్థిరరాశి string& name, VSymEnt* entp) {
        స్థిరరాశి స్వయంచాలక it = m_idNameMap.కనుగొను(name);
        ఒకవేళ (name != "" && it != m_idNameMap.చివర()) {
            UINFO(9, "     SymReinsert se" << cvt_గమ్యం_Hex(this) << " '" << name << "' se"
                                           << cvt_గమ్యం_Hex(entp) << "  " << entp->nodep());
            it->రెండవ = entp;  // Replace
        } లేకపోతే {
            చొప్పించు(name, entp);
        }
    }
    VSymEnt* కనుగొను_Id_Flat(స్థిరరాశి string& name) స్థిరరాశి {
        // Find identifier without looking upward through symbol hierarchy
        // First, scan this begin/end block or module for the name
        స్థిరరాశి స్వయంచాలక it = m_idNameMap.కనుగొను(name);
        UINFO(9, "     SymFind   se"
                     << cvt_గమ్యం_Hex(this) << " '" << name << "' -> "
                     << (it == m_idNameMap.చివర() ? "NONE"
                                                 : "se" + cvt_గమ్యం_Hex(it->రెండవ)
                                                       + " n=" + cvt_గమ్యం_Hex(it->రెండవ->nodep())));
        ఒకవేళ (it != m_idNameMap.చివర()) తిరిగి it->రెండవ;
        తిరిగి nullptr;
    }
    స్థిర bool checkSimilarname(స్థిరరాశి Ast_కణుపు* nodep) {
        // Only declarations that can contribute a name to the netlist handed
        // to a backend tool.
        ఒకవేళ (స్థిరరాశి AstVar* స్థిరరాశి varp = VN_CAST(nodep, Var)) {
            తిరిగి !varp->isParam() && !varp->isGenVar();
        }
        తిరిగి VN_IS(nodep, Cell)  //
               || VN_IS(nodep, కణుపు_ఖండం)  //
               || VN_IS(nodep, Gen_ఖండం)  //
               || VN_IS(nodep, Func)  // Note: not AstNodeFTask (Property/Sequence)
               || VN_IS(nodep, Task);
    }
    VSymEnt* కనుగొను_Similar_Id_Flat(స్థిరరాశి string& name) స్థిరరాశి {
        // Find identifier without looking upward through symbol hierarchy
        // Were looking for symbols that are the same when compared without
        // caring about case, but that are not the same name
        ఒకవేళ (name.కనుగొను("__DOT__") != std::string::npos)  // ignore hierarchical equivalents
            తిరిగి nullptr;
        string s = name;
        కొరకు (స్వయంచాలక& c : s) c = (అక్షరం)tolower(c);
        ఒకవేళ (m_idNameSimilarMap.కనుగొను(s) == m_idNameSimilarMap.చివర()) తిరిగి nullptr;
        కొరకు (స్వయంచాలక it = m_idNameMap.begin(); it != m_idNameMap.చివర(); ++it) {
            // Only report a declaration that is itself checked, otherwise we
            // might point at e.g. a localparam that merely sorts first
            ఒకవేళ (!checkSimilarname(it->రెండవ->nodep())) కొనసాగు;
            string t = it->మొదటి;
            కొరకు (స్వయంచాలక& c : t) c = (అక్షరం)tolower(c);
            ఒకవేళ (t == s && name != it->మొదటి) { తిరిగి it->రెండవ; }
        }
        తిరిగి nullptr;
    }
    VSymEnt* కనుగొను_Id_Fallback(స్థిరరాశి string& name) స్థిరరాశి {
        // Find identifier looking upward through symbol hierarchy
        // First, scan this begin/end block or module for the name
        ఒకవేళ (VSymEnt* స్థిరరాశి entp = కనుగొను_Id_Flat(name)) తిరిగి entp;
        // Then scan the upper begin/end block or module for the name
        ఒకవేళ (m_fallbackp) తిరిగి m_fallbackp->కనుగొను_Id_Fallback(name);
        తిరిగి nullptr;
    }
    శూన్య candidateIdFlat(VSpellCheck* spellerp, స్థిరరాశి VNodeMatcher* matcherp,
                         bool includeType) స్థిరరాశి {
        // Suggest alternative symbol candidates without looking upward through symbol
        // hierarchy
        కొరకు (IdNameMap::const_iterator it = m_idNameMap.begin(); it != m_idNameMap.చివర(); ++it) {
            స్థిరరాశి Ast_కణుపు* స్థిరరాశి itemp = it->రెండవ->nodep();
            ఒకవేళ (itemp && (!matcherp || matcherp->కణుపు_సరిపోలిక(itemp))) {
                ఒకవేళ (includeType) {
                    spellerp->నెట్టు_Candidate(itemp->prettyName(), itemp->prettyTypeName());
                } లేకపోతే {
                    spellerp->నెట్టు_Candidate(itemp->prettyName());
                }
            }
        }
    }
    శూన్య candidateIdFallback(VSpellCheck* spellerp, స్థిరరాశి VNodeMatcher* matcherp) స్థిరరాశి {
        // Suggest alternative symbol candidates with looking upward through symbol hierarchy
        // Note VSpellCheck wants the most important (closest) items pushed first
        candidateIdFlat(spellerp, matcherp, false);
        // Then suggest the upper begin/end block or module
        ఒకవేళ (m_fallbackp) m_fallbackp->candidateIdFallback(spellerp, matcherp);
    }

అంతరంగ:
    శూన్య import_One_చిహ్నం(VSym_లేఖాచిత్రం* graphp, స్థిరరాశి string& name, స్థిరరాశి VSymEnt* srcp,
                         bool honorExport) {
        ఒకవేళ ((!honorExport || srcp->exported())
            && !కనుగొను_Id_Flat(name)) {  // Don't insert over existing entry
            VSymEnt* స్థిరరాశి symp = new VSymEnt{graphp, srcp};
            symp->exported(false);  // Can't reimport an import without an export
            symp->imported(true);
            reinsert(name, symp);
        }
    }
    శూన్య export_One_చిహ్నం(VSym_లేఖాచిత్రం* /*graphp*/, స్థిరరాశి string& name, స్థిరరాశి VSymEnt* srcp) స్థిరరాశి {
        ఒకవేళ (srcp->exported()) {
            ఒకవేళ (VSymEnt* స్థిరరాశి symp = కనుగొను_Id_Flat(name)) {  // Should already exist in current table
                ఒకవేళ (!symp->exported()) symp->exported(true);
            }
        }
    }

బహిరంగ:
    శూన్య importFromClass(VSym_లేఖాచిత్రం* graphp, స్థిరరాశి VSymEnt* srcp) {
        // Import tokens from source symbol table into this symbol table
        // Used for classes in early parsing only to handle "extends"

        // If an "extern foo" exists, then we can't import "foo" from the base class.
        // But ok for "extern foo" and "foo" to both come from base (so must check before
        // insert)
        std::unordered_సమితి<std::string> haveExterns;
        కొరకు (IdNameMap::const_iterator it = srcp->m_idNameMap.begin();
             it != srcp->m_idNameMap.చివర(); ++it) {
            ఒకవేళ (m_idNameMap.లెక్క("extern " + it->మొదటి)) haveExterns.emplace(it->మొదటి);
        }
        కొరకు (IdNameMap::const_iterator it = srcp->m_idNameMap.begin();
             it != srcp->m_idNameMap.చివర(); ++it) {
            ఒకవేళ (!haveExterns.లెక్క(it->మొదటి))
                import_One_చిహ్నం(graphp, it->మొదటి, it->రెండవ, false);
        }
    }
    శూన్య importFromPackage(VSym_లేఖాచిత్రం* graphp, స్థిరరాశి VSymEnt* srcp, స్థిరరాశి string& id_or_నక్షత్రం) {
        // Import tokens from source symbol table into this symbol table
        ఒకవేళ (id_or_నక్షత్రం != "*") {
            స్థిరరాశి స్వయంచాలక it = srcp->m_idNameMap.కనుగొను(id_or_నక్షత్రం);
            ఒకవేళ (it != srcp->m_idNameMap.చివర()) {
                import_One_చిహ్నం(graphp, it->మొదటి, it->రెండవ, true);
            }
        } లేకపోతే {
            కొరకు (IdNameMap::const_iterator it = srcp->m_idNameMap.begin();
                 it != srcp->m_idNameMap.చివర(); ++it) {
                import_One_చిహ్నం(graphp, it->మొదటి, it->రెండవ, true);
            }
        }
    }
    శూన్య exportFromPackage(VSym_లేఖాచిత్రం* graphp, స్థిరరాశి VSymEnt* srcp, స్థిరరాశి string& id_or_నక్షత్రం) {
        // Export tokens from source symbol table into this symbol table
        ఒకవేళ (id_or_నక్షత్రం != "*") {
            స్థిరరాశి స్వయంచాలక it = vlstd::as_const(srcp->m_idNameMap).కనుగొను(id_or_నక్షత్రం);
            ఒకవేళ (it != srcp->m_idNameMap.చివర()) export_One_చిహ్నం(graphp, it->మొదటి, it->రెండవ);
        } లేకపోతే {
            కొరకు (IdNameMap::const_iterator it = srcp->m_idNameMap.begin();
                 it != srcp->m_idNameMap.చివర(); ++it) {
                export_One_చిహ్నం(graphp, it->మొదటి, it->రెండవ);
            }
        }
    }
    శూన్య export_నక్షత్రం_నక్షత్రం(VSym_లేఖాచిత్రం* /*graphp*/) {
        // Export *:*: Export all tokens from imported packages
        కొరకు (IdNameMap::const_iterator it = m_idNameMap.begin(); it != m_idNameMap.చివర(); ++it) {
            VSymEnt* స్థిరరాశి symp = it->రెండవ;
            ఒకవేళ (!symp->exported()) symp->exported(true);
        }
    }
    శూన్య importFromIface(VSym_లేఖాచిత్రం* graphp, స్థిరరాశి VSymEnt* srcp, bool onlyUnmodportable = false) {
        // Import interface tokens from source symbol table into this symbol table, recursively
        UINFO(9, "     importIf  se" << cvt_గమ్యం_Hex(this) << " from se" << cvt_గమ్యం_Hex(srcp));
        కొరకు (IdNameMap::const_iterator it = srcp->m_idNameMap.begin();
             it != srcp->m_idNameMap.చివర(); ++it) {
            స్థిరరాశి string& name = it->మొదటి;
            VSymEnt* స్థిరరాశి subSrcp = it->రెండవ;
            స్థిరరాశి AstVar* స్థిరరాశి varp = VN_CAST(subSrcp->nodep(), Var);
            ఒకవేళ (!onlyUnmodportable || (varp && varp->isParam())) {
                VSymEnt* స్థిరరాశి subSymp = new VSymEnt{graphp, subSrcp};
                reinsert(name, subSymp);
                // And recurse to create children
                subSymp->importFromIface(graphp, subSrcp);
            }
        }
    }
    string cellErrorScopes(స్థిరరాశి Ast_కణుపు* lookp, string prettyName = "") {
        ఒకవేళ (prettyName == "") prettyName = lookp->prettyName();
        string scopes;
        కొరకు (IdNameMap::iterator it = m_idNameMap.begin(); it != m_idNameMap.చివర(); ++it) {
            స్థిరరాశి Ast_కణుపు* స్థిరరాశి itemp = it->రెండవ->nodep();
            ఒకవేళ (VN_IS(itemp, Cell) || (VN_IS(itemp, Module) && VN_AS(itemp, Module)->is_శిఖరం())) {
                ఒకవేళ (scopes != "") scopes += ", ";
                scopes += Ast_కణుపు::prettyName(it->మొదటి);
            }
        }
        ఒకవేళ (scopes == "") scopes = "<no instances found>";
        ఒకవేళ (debug()) dumpSelf(std::cerr, "       KnownScope: ", 1);
        తిరిగి V3Error::warn_మరిన్ని() + "... Known scopes under '" + prettyName + "': " + scopes
               + '\n';
    }
};

//######################################################################
// Symbol tables

class VSym_లేఖాచిత్రం final {
    // Collection of symbol tables
    // TYPES
    using Sym_దొంతర = std::సదిశ<VSymEnt*>;

    // MEMBERS
    VSymEnt* m_symRootp;  // Root symbol table
    Sym_దొంతర m_symsp;  // All symbol tables, to cleanup

    // CONSTRUCTORS
    VL_UNCOPYABLE(VSym_లేఖాచిత్రం);

    VL_DEFINE_DEBUG_FUNCTIONS;

బహిరంగ:
    explicit VSym_లేఖాచిత్రం(AstNetlist* nodep) { m_symRootp = new VSymEnt{this, nodep}; }
    ~VSym_లేఖాచిత్రం() {
        కొరకు (స్థిరరాశి VSymEnt* entp : m_symsp) delete entp;
    }

    // METHODS
    VSymEnt* rootp() స్థిరరాశి { తిరిగి m_symRootp; }
    // Debug
    శూన్య dumpSelf(std::ostream& os, స్థిరరాశి string& indent = "") {
        VSymConstMap పూర్తి_Syms;
        os << "SymEnt Dump:\n";
        m_symRootp->dumpIterate(os, పూర్తి_Syms, indent, 9999, "$root");
        bool మొదటి = true;
        కొరకు (Sym_దొంతర::iterator it = m_symsp.begin(); it != m_symsp.చివర(); ++it) {
            ఒకవేళ (పూర్తి_Syms.కనుగొను(*it) == పూర్తి_Syms.చివర()) {
                ఒకవేళ (మొదటి) {
                    మొదటి = false;
                    os << "%%Warning: SymEnt Orphans:\n";
                }
                (*it)->dumpIterate(os, పూర్తి_Syms, indent, 9999, "Orphan");
            }
        }
    }
    శూన్య dumpFilePrefixed(స్థిరరాశి string& nameComment) {
        ఒకవేళ (dump_వృక్షం_Level()) {
            స్థిరరాశి string filename = v3Global.debugFilename(nameComment) + ".txt";
            UINFO(2, "Dumping " << filename);
            స్థిరరాశి std::unique_ptr<std::ofstream> logp{V3File::new_ofstream(filename)};
            ఒకవేళ (logp->fail()) v3fatal("Can't write file: " << filename);
            dumpSelf(*logp, "");
        }
    }

రక్షిత:
    మిత్ర class VSymEnt;
    శూన్య నెట్టు_New_Ent(VSymEnt* entp) { m_symsp.నెట్టు_వెనుక(entp); }
};

//######################################################################

అంతఃవరుస VSymEnt::VSymEnt(VSym_లేఖాచిత్రం* graphp, Ast_కణుపు* nodep)
    : m_nodep{nodep} {
    // No argument to set fallbackp, as generally it's wrong to set it in the new call,
    // Instead it needs to be set on a "findOrNew()" return, as it may have been new'ed
    // by an earlier search insertion.
    graphp->నెట్టు_New_Ent(this);
}

అంతఃవరుస VSymEnt::VSymEnt(VSym_లేఖాచిత్రం* graphp, స్థిరరాశి VSymEnt* symp)
    : m_nodep{symp->m_nodep}
    , m_fallbackp{symp->m_fallbackp}
    , m_parentp{symp->m_parentp}
    , m_classOrPackagep{symp->m_classOrPackagep}
    , m_exported{symp->m_exported}
    , m_imported{symp->m_imported} {
    graphp->నెట్టు_New_Ent(this);
}

#అయితే_ముగింపు  // guard
