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

#جيڪڏهن_وصف_نه_ٿيل VERILATOR_V3SYMTABLE_H_
#وصف_ڪريو VERILATOR_V3SYMTABLE_H_

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

class VSym_گراف;
class VSymEnt;

//######################################################################
// Symbol table

using VSymConstMap = std::unordered_سيٽ<ثابت VSymEnt*>;

class VSymEnt final {
    // Symbol table that can have a "superior" table for resolving upper references
    // MEMBERS
    using IdNameMap = std::multimap<std::string, VSymEnt*>;
    IdNameMap m_idNameMap;  // Hash of variables by name
    std::unordered_سيٽ<std::string> m_idNameSimilarMap;  // Variables by name with same case
    Ast_ڳنڍ* m_nodep;  // Node that entry belongs to
    VSymEnt* m_fallbackp = nullptr;  // Table "above" this in name scope, for fallback resolution
    VSymEnt* m_parentp = nullptr;  // Table that created this
    Ast_ڳنڍ_Module* m_classOrPackagep = nullptr;  // Package node is in (for V3LinkDot, unused here)
    string m_sym_اڳياڙي;  // String to prefix symbols with (for V3LinkDot, unused here)
    bool m_exported = true;  // Allow importing
    bool m_imported = false;  // Was imported
#جيڪڏهن_وصف_ٿيل VL_DEBUG
    VL_DEFINE_DEBUG_FUNCTIONS;
#وإلا
    ساكن constexpr صحيح debug() { أرجع 0; }  // NOT runtime, too hot of a function
#جيڪڏهن_پورو
عام:
    using const_iterator = IdNameMap::const_iterator;
    const_iterator begin() ثابت { أرجع m_idNameMap.begin(); }
    const_iterator پڇاڙي() ثابت { أرجع m_idNameMap.پڇاڙي(); }

    فراغ dumpIterate(std::ostream& os, VSymConstMap& پورو_Symsr, ثابت string& indent,
                     صحيح انگ_سطحون, ثابت string& ڳولا_Name) ثابت {
        os << indent << "+ " << std::کاٻو << std::setw(30) << ("'"s + ڳولا_Name + "'"s)
           << std::setw(0) << std::ساڄو;
        os << "  se" << cvt_منزل_Hex(this) << std::setw(0);
        os << "  fallb=se" << cvt_منزل_Hex(m_fallbackp);
        إذا (m_imported || m_exported) {
            os << "  ";
            إذا (m_imported) os << "[I]";
            إذا (m_exported) os << "[E]";
        }
        إذا (m_sym_اڳياڙي != "") os << "  symPrefix=" << m_sym_اڳياڙي;
        إذا (nodep()) os << "  n=" << nodep();
        os << '\n';
        إذا (VL_UNCOVERABLE(!پورو_Symsr.وجهو(this).ٻيو)) {
            os << indent << "| ^ duplicate, so no children printed\n";  // LCOV_EXCL_LINE
        } وإلا {
            لكل (خودڪار it = m_idNameMap.begin(); it != m_idNameMap.پڇاڙي(); ++it) {
                إذا (انگ_سطحون >= 1) {
                    it->ٻيو->dumpIterate(os, پورو_Symsr, indent + "| ", انگ_سطحون - 1,
                                            it->پهريون);
                }
            }
        }
    }
    فراغ dumpSelf(std::ostream& os, ثابت string& indent = "", صحيح انگ_سطحون = 1) ثابت {
        VSymConstMap پورو_Syms;
        dumpIterate(os, پورو_Syms, indent, انگ_سطحون, "TOP");
    }

    // METHODS
    VSymEnt(VSym_گراف* graphp, ثابت VSymEnt* symp);  //هيٺ
    VSymEnt(VSym_گراف* graphp, Ast_ڳنڍ* nodep);  //هيٺ
    ~VSymEnt() {
        // Change links so we coredump if used
#جيڪڏهن_وصف_ٿيل VL_DEBUG
        m_nodep = reinterpret_cast<Ast_ڳنڍ*>(1);
        m_fallbackp = reinterpret_cast<VSymEnt*>(1);
        m_parentp = reinterpret_cast<VSymEnt*>(1);
        m_classOrPackagep = reinterpret_cast<Ast_ڳنڍ_Module*>(1);
#جيڪڏهن_پورو
    }
#إذا (defined(VL_DEBUG) || defined(VL_ALLOC_RANDOM_CHECKS)) && !defined(VL_LEAK_CHECKS)
    // For testing, leak so above destructor 1 assignments work
    فراغ* عامل new(سائيز_t سائيز) { أرجع std::يادگيري_ڏيو(سائيز); }
    فراغ عامل delete(فراغ* objp, سائيز_t سائيز) {}
#جيڪڏهن_پورو
    فراغ fallbackp(VSymEnt* entp) { m_fallbackp = entp; }
    VSymEnt* fallbackp() ثابت { أرجع m_fallbackp; }
    فراغ parentp(VSymEnt* entp) { m_parentp = entp; }
    VSymEnt* parentp() ثابت { أرجع m_parentp; }
    فراغ classOrPackagep(Ast_ڳنڍ_Module* entp) { m_classOrPackagep = entp; }
    Ast_ڳنڍ_Module* classOrPackagep() ثابت { أرجع m_classOrPackagep; }
    Ast_ڳنڍ* nodep() ثابت { أرجع m_nodep; }
    string sym_اڳياڙي() ثابت { أرجع m_sym_اڳياڙي; }
    فراغ sym_اڳياڙي(ثابت string& name) { m_sym_اڳياڙي = name; }
    bool exported() ثابت { أرجع m_exported; }
    فراغ exported(bool flag) { m_exported = flag; }
    bool imported() ثابت { أرجع m_imported; }
    فراغ imported(bool flag) { m_imported = flag; }
    VSymEnt* وجهو(ثابت string& name, VSymEnt* entp) {
        UINFO(9, "     SymInsert se" << cvt_منزل_Hex(this) << " '" << name << "' se" << cvt_منزل_Hex(entp)
                                     << "  " << entp->nodep());
        إذا (name != "" && m_idNameMap.ڳوليو(name) != m_idNameMap.پڇاڙي()) {
            // If didn't already report warning
            إذا (!V3Error::error_ڳڻپ()) {  // LCOV_EXCL_START
                إذا (debug() >= 9 || V3Error::debugDefault())
                    dumpSelf(std::cout, "- err-dump: ", 1);
                entp->nodep()->v3fatalSrc("Inserting two symbols with same name: " << name);
            }  // LCOV_EXCL_STOP
        } وإلا {
            m_idNameMap.emplace(name, entp);
        }
        إذا (!name.خالي_جايون() && name.ڳوليو("__DOT__") == std::string::npos  // ignore hierarchical
            && checkSimilarname(entp->nodep())) {
            string lc = name;
            لكل (خودڪار& c : lc) c = (حرف)tolower(c);
            إذا (m_idNameSimilarMap.ڳوليو(lc) == m_idNameSimilarMap.پڇاڙي())
                m_idNameSimilarMap.وجهو(lc);
        }

        أرجع entp;
    }
    فراغ reinsert(ثابت string& name, VSymEnt* entp) {
        ثابت خودڪار it = m_idNameMap.ڳوليو(name);
        إذا (name != "" && it != m_idNameMap.پڇاڙي()) {
            UINFO(9, "     SymReinsert se" << cvt_منزل_Hex(this) << " '" << name << "' se"
                                           << cvt_منزل_Hex(entp) << "  " << entp->nodep());
            it->ٻيو = entp;  // Replace
        } وإلا {
            وجهو(name, entp);
        }
    }
    VSymEnt* ڳوليو_Id_Flat(ثابت string& name) ثابت {
        // Find identifier without looking upward through symbol hierarchy
        // First, scan this begin/end block or module for the name
        ثابت خودڪار it = m_idNameMap.ڳوليو(name);
        UINFO(9, "     SymFind   se"
                     << cvt_منزل_Hex(this) << " '" << name << "' -> "
                     << (it == m_idNameMap.پڇاڙي() ? "NONE"
                                                 : "se" + cvt_منزل_Hex(it->ٻيو)
                                                       + " n=" + cvt_منزل_Hex(it->ٻيو->nodep())));
        إذا (it != m_idNameMap.پڇاڙي()) أرجع it->ٻيو;
        أرجع nullptr;
    }
    ساكن bool checkSimilarname(ثابت Ast_ڳنڍ* nodep) {
        // Only declarations that can contribute a name to the netlist handed
        // to a backend tool.
        إذا (ثابت AstVar* ثابت varp = VN_CAST(nodep, Var)) {
            أرجع !varp->isParam() && !varp->isGenVar();
        }
        أرجع VN_IS(nodep, Cell)  //
               || VN_IS(nodep, ڳنڍ_بلاڪ)  //
               || VN_IS(nodep, Gen_بلاڪ)  //
               || VN_IS(nodep, Func)  // Note: not AstNodeFTask (Property/Sequence)
               || VN_IS(nodep, Task);
    }
    VSymEnt* ڳوليو_Similar_Id_Flat(ثابت string& name) ثابت {
        // Find identifier without looking upward through symbol hierarchy
        // Were looking for symbols that are the same when compared without
        // caring about case, but that are not the same name
        إذا (name.ڳوليو("__DOT__") != std::string::npos)  // ignore hierarchical equivalents
            أرجع nullptr;
        string s = name;
        لكل (خودڪار& c : s) c = (حرف)tolower(c);
        إذا (m_idNameSimilarMap.ڳوليو(s) == m_idNameSimilarMap.پڇاڙي()) أرجع nullptr;
        لكل (خودڪار it = m_idNameMap.begin(); it != m_idNameMap.پڇاڙي(); ++it) {
            // Only report a declaration that is itself checked, otherwise we
            // might point at e.g. a localparam that merely sorts first
            إذا (!checkSimilarname(it->ٻيو->nodep())) استمر;
            string t = it->پهريون;
            لكل (خودڪار& c : t) c = (حرف)tolower(c);
            إذا (t == s && name != it->پهريون) { أرجع it->ٻيو; }
        }
        أرجع nullptr;
    }
    VSymEnt* ڳوليو_Id_Fallback(ثابت string& name) ثابت {
        // Find identifier looking upward through symbol hierarchy
        // First, scan this begin/end block or module for the name
        إذا (VSymEnt* ثابت entp = ڳوليو_Id_Flat(name)) أرجع entp;
        // Then scan the upper begin/end block or module for the name
        إذا (m_fallbackp) أرجع m_fallbackp->ڳوليو_Id_Fallback(name);
        أرجع nullptr;
    }
    فراغ candidateIdFlat(VSpellCheck* spellerp, ثابت VNodeMatcher* matcherp,
                         bool includeType) ثابت {
        // Suggest alternative symbol candidates without looking upward through symbol
        // hierarchy
        لكل (IdNameMap::const_iterator it = m_idNameMap.begin(); it != m_idNameMap.پڇاڙي(); ++it) {
            ثابت Ast_ڳنڍ* ثابت itemp = it->ٻيو->nodep();
            إذا (itemp && (!matcherp || matcherp->ڳنڍ_ميلاپ(itemp))) {
                إذا (includeType) {
                    spellerp->ڌڪو_Candidate(itemp->prettyName(), itemp->prettyTypeName());
                } وإلا {
                    spellerp->ڌڪو_Candidate(itemp->prettyName());
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
    فراغ import_One_نشان(VSym_گراف* graphp, ثابت string& name, ثابت VSymEnt* srcp,
                         bool honorExport) {
        إذا ((!honorExport || srcp->exported())
            && !ڳوليو_Id_Flat(name)) {  // Don't insert over existing entry
            VSymEnt* ثابت symp = new VSymEnt{graphp, srcp};
            symp->exported(false);  // Can't reimport an import without an export
            symp->imported(true);
            reinsert(name, symp);
        }
    }
    فراغ export_One_نشان(VSym_گراف* /*graphp*/, ثابت string& name, ثابت VSymEnt* srcp) ثابت {
        إذا (srcp->exported()) {
            إذا (VSymEnt* ثابت symp = ڳوليو_Id_Flat(name)) {  // Should already exist in current table
                إذا (!symp->exported()) symp->exported(true);
            }
        }
    }

عام:
    فراغ importFromClass(VSym_گراف* graphp, ثابت VSymEnt* srcp) {
        // Import tokens from source symbol table into this symbol table
        // Used for classes in early parsing only to handle "extends"

        // If an "extern foo" exists, then we can't import "foo" from the base class.
        // But ok for "extern foo" and "foo" to both come from base (so must check before
        // insert)
        std::unordered_سيٽ<std::string> haveExterns;
        لكل (IdNameMap::const_iterator it = srcp->m_idNameMap.begin();
             it != srcp->m_idNameMap.پڇاڙي(); ++it) {
            إذا (m_idNameMap.ڳڻپ("extern " + it->پهريون)) haveExterns.emplace(it->پهريون);
        }
        لكل (IdNameMap::const_iterator it = srcp->m_idNameMap.begin();
             it != srcp->m_idNameMap.پڇاڙي(); ++it) {
            إذا (!haveExterns.ڳڻپ(it->پهريون))
                import_One_نشان(graphp, it->پهريون, it->ٻيو, false);
        }
    }
    فراغ importFromPackage(VSym_گراف* graphp, ثابت VSymEnt* srcp, ثابت string& id_or_تارو) {
        // Import tokens from source symbol table into this symbol table
        إذا (id_or_تارو != "*") {
            ثابت خودڪار it = srcp->m_idNameMap.ڳوليو(id_or_تارو);
            إذا (it != srcp->m_idNameMap.پڇاڙي()) {
                import_One_نشان(graphp, it->پهريون, it->ٻيو, true);
            }
        } وإلا {
            لكل (IdNameMap::const_iterator it = srcp->m_idNameMap.begin();
                 it != srcp->m_idNameMap.پڇاڙي(); ++it) {
                import_One_نشان(graphp, it->پهريون, it->ٻيو, true);
            }
        }
    }
    فراغ exportFromPackage(VSym_گراف* graphp, ثابت VSymEnt* srcp, ثابت string& id_or_تارو) {
        // Export tokens from source symbol table into this symbol table
        إذا (id_or_تارو != "*") {
            ثابت خودڪار it = vlstd::as_const(srcp->m_idNameMap).ڳوليو(id_or_تارو);
            إذا (it != srcp->m_idNameMap.پڇاڙي()) export_One_نشان(graphp, it->پهريون, it->ٻيو);
        } وإلا {
            لكل (IdNameMap::const_iterator it = srcp->m_idNameMap.begin();
                 it != srcp->m_idNameMap.پڇاڙي(); ++it) {
                export_One_نشان(graphp, it->پهريون, it->ٻيو);
            }
        }
    }
    فراغ export_تارو_تارو(VSym_گراف* /*graphp*/) {
        // Export *:*: Export all tokens from imported packages
        لكل (IdNameMap::const_iterator it = m_idNameMap.begin(); it != m_idNameMap.پڇاڙي(); ++it) {
            VSymEnt* ثابت symp = it->ٻيو;
            إذا (!symp->exported()) symp->exported(true);
        }
    }
    فراغ importFromIface(VSym_گراف* graphp, ثابت VSymEnt* srcp, bool onlyUnmodportable = false) {
        // Import interface tokens from source symbol table into this symbol table, recursively
        UINFO(9, "     importIf  se" << cvt_منزل_Hex(this) << " from se" << cvt_منزل_Hex(srcp));
        لكل (IdNameMap::const_iterator it = srcp->m_idNameMap.begin();
             it != srcp->m_idNameMap.پڇاڙي(); ++it) {
            ثابت string& name = it->پهريون;
            VSymEnt* ثابت subSrcp = it->ٻيو;
            ثابت AstVar* ثابت varp = VN_CAST(subSrcp->nodep(), Var);
            إذا (!onlyUnmodportable || (varp && varp->isParam())) {
                VSymEnt* ثابت subSymp = new VSymEnt{graphp, subSrcp};
                reinsert(name, subSymp);
                // And recurse to create children
                subSymp->importFromIface(graphp, subSrcp);
            }
        }
    }
    string cellErrorScopes(ثابت Ast_ڳنڍ* lookp, string prettyName = "") {
        إذا (prettyName == "") prettyName = lookp->prettyName();
        string scopes;
        لكل (IdNameMap::iterator it = m_idNameMap.begin(); it != m_idNameMap.پڇاڙي(); ++it) {
            ثابت Ast_ڳنڍ* ثابت itemp = it->ٻيو->nodep();
            إذا (VN_IS(itemp, Cell) || (VN_IS(itemp, Module) && VN_AS(itemp, Module)->is_چوٽي())) {
                إذا (scopes != "") scopes += ", ";
                scopes += Ast_ڳنڍ::prettyName(it->پهريون);
            }
        }
        إذا (scopes == "") scopes = "<no instances found>";
        إذا (debug()) dumpSelf(std::cerr, "       KnownScope: ", 1);
        أرجع V3Error::warn_وڌيڪ() + "... Known scopes under '" + prettyName + "': " + scopes
               + '\n';
    }
};

//######################################################################
// Symbol tables

class VSym_گراف final {
    // Collection of symbol tables
    // TYPES
    using Sym_ڍير = std::ويڪٽر<VSymEnt*>;

    // MEMBERS
    VSymEnt* m_symRootp;  // Root symbol table
    Sym_ڍير m_symsp;  // All symbol tables, to cleanup

    // CONSTRUCTORS
    VL_UNCOPYABLE(VSym_گراف);

    VL_DEFINE_DEBUG_FUNCTIONS;

عام:
    explicit VSym_گراف(AstNetlist* nodep) { m_symRootp = new VSymEnt{this, nodep}; }
    ~VSym_گراف() {
        لكل (ثابت VSymEnt* entp : m_symsp) delete entp;
    }

    // METHODS
    VSymEnt* rootp() ثابت { أرجع m_symRootp; }
    // Debug
    فراغ dumpSelf(std::ostream& os, ثابت string& indent = "") {
        VSymConstMap پورو_Syms;
        os << "SymEnt Dump:\n";
        m_symRootp->dumpIterate(os, پورو_Syms, indent, 9999, "$root");
        bool پهريون = true;
        لكل (Sym_ڍير::iterator it = m_symsp.begin(); it != m_symsp.پڇاڙي(); ++it) {
            إذا (پورو_Syms.ڳوليو(*it) == پورو_Syms.پڇاڙي()) {
                إذا (پهريون) {
                    پهريون = false;
                    os << "%%Warning: SymEnt Orphans:\n";
                }
                (*it)->dumpIterate(os, پورو_Syms, indent, 9999, "Orphan");
            }
        }
    }
    فراغ dumpFilePrefixed(ثابت string& nameComment) {
        إذا (dump_وڻ_Level()) {
            ثابت string filename = v3Global.debugFilename(nameComment) + ".txt";
            UINFO(2, "Dumping " << filename);
            ثابت std::unique_ptr<std::ofstream> logp{V3File::new_ofstream(filename)};
            إذا (logp->fail()) v3fatal("Can't write file: " << filename);
            dumpSelf(*logp, "");
        }
    }

محمي:
    دوست class VSymEnt;
    فراغ ڌڪو_New_Ent(VSymEnt* entp) { m_symsp.ڌڪو_پٺ(entp); }
};

//######################################################################

مضمن VSymEnt::VSymEnt(VSym_گراف* graphp, Ast_ڳنڍ* nodep)
    : m_nodep{nodep} {
    // No argument to set fallbackp, as generally it's wrong to set it in the new call,
    // Instead it needs to be set on a "findOrNew()" return, as it may have been new'ed
    // by an earlier search insertion.
    graphp->ڌڪو_New_Ent(this);
}

مضمن VSymEnt::VSymEnt(VSym_گراف* graphp, ثابت VSymEnt* symp)
    : m_nodep{symp->m_nodep}
    , m_fallbackp{symp->m_fallbackp}
    , m_parentp{symp->m_parentp}
    , m_classOrPackagep{symp->m_classOrPackagep}
    , m_exported{symp->m_exported}
    , m_imported{symp->m_imported} {
    graphp->ڌڪو_New_Ent(this);
}

#جيڪڏهن_پورو  // guard
