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

#جے_بنا_تعریف VERILATOR_V3SYMTABLE_H_
#تعریف_کرو VERILATOR_V3SYMTABLE_H_

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

using VSymConstMap = std::unordered_سیٹ<ثابت VSymEnt*>;

class VSymEnt final {
    // Symbol table that can have a "superior" table for resolving upper references
    // MEMBERS
    using IdNameMap = std::multimap<std::string, VSymEnt*>;
    IdNameMap m_idNameMap;  // Hash of variables by name
    std::unordered_سیٹ<std::string> m_idNameSimilarMap;  // Variables by name with same case
    Ast_گرہ* m_nodep;  // Node that entry belongs to
    VSymEnt* m_fallbackp = nullptr;  // Table "above" this in name scope, for fallback resolution
    VSymEnt* m_parentp = nullptr;  // Table that created this
    Ast_گرہ_Module* m_classOrPackagep = nullptr;  // Package node is in (for V3LinkDot, unused here)
    string m_sym_اگیتر;  // String to prefix symbols with (for V3LinkDot, unused here)
    bool m_exported = true;  // Allow importing
    bool m_imported = false;  // Was imported
#جے_تعریف_شدہ VL_DEBUG
    VL_DEFINE_DEBUG_FUNCTIONS;
#وإلا
    ساكن constexpr صحيح debug() { أرجع 0; }  // NOT runtime, too hot of a function
#جے_انت
عام:
    using const_iterator = IdNameMap::const_iterator;
    const_iterator begin() ثابت { أرجع m_idNameMap.begin(); }
    const_iterator سرا() ثابت { أرجع m_idNameMap.سرا(); }

    فراغ dumpIterate(std::ostream& os, VSymConstMap& پورا_Symsr, ثابت string& indent,
                     صحيح عدد_پدھراں, ثابت string& تلاش_Name) ثابت {
        os << indent << "+ " << std::بایاں << std::setw(30) << ("'"s + تلاش_Name + "'"s)
           << std::setw(0) << std::دایاں;
        os << "  se" << cvt_منزل_Hex(this) << std::setw(0);
        os << "  fallb=se" << cvt_منزل_Hex(m_fallbackp);
        إذا (m_imported || m_exported) {
            os << "  ";
            إذا (m_imported) os << "[I]";
            إذا (m_exported) os << "[E]";
        }
        إذا (m_sym_اگیتر != "") os << "  symPrefix=" << m_sym_اگیتر;
        إذا (nodep()) os << "  n=" << nodep();
        os << '\n';
        إذا (VL_UNCOVERABLE(!پورا_Symsr.داخل_کرو(this).دوجا)) {
            os << indent << "| ^ duplicate, so no children printed\n";  // LCOV_EXCL_LINE
        } وإلا {
            لكل (خودکار it = m_idNameMap.begin(); it != m_idNameMap.سرا(); ++it) {
                إذا (عدد_پدھراں >= 1) {
                    it->دوجا->dumpIterate(os, پورا_Symsr, indent + "| ", عدد_پدھراں - 1,
                                            it->پہلا);
                }
            }
        }
    }
    فراغ dumpSelf(std::ostream& os, ثابت string& indent = "", صحيح عدد_پدھراں = 1) ثابت {
        VSymConstMap پورا_Syms;
        dumpIterate(os, پورا_Syms, indent, عدد_پدھراں, "TOP");
    }

    // METHODS
    VSymEnt(VSym_گراف* graphp, ثابت VSymEnt* symp);  //ہیٹھاں
    VSymEnt(VSym_گراف* graphp, Ast_گرہ* nodep);  //ہیٹھاں
    ~VSymEnt() {
        // Change links so we coredump if used
#جے_تعریف_شدہ VL_DEBUG
        m_nodep = reinterpret_cast<Ast_گرہ*>(1);
        m_fallbackp = reinterpret_cast<VSymEnt*>(1);
        m_parentp = reinterpret_cast<VSymEnt*>(1);
        m_classOrPackagep = reinterpret_cast<Ast_گرہ_Module*>(1);
#جے_انت
    }
#إذا (defined(VL_DEBUG) || defined(VL_ALLOC_RANDOM_CHECKS)) && !defined(VL_LEAK_CHECKS)
    // For testing, leak so above destructor 1 assignments work
    فراغ* عامل new(ناپ_t ناپ) { أرجع std::حافظہ_دیو(ناپ); }
    فراغ عامل delete(فراغ* objp, ناپ_t ناپ) {}
#جے_انت
    فراغ fallbackp(VSymEnt* entp) { m_fallbackp = entp; }
    VSymEnt* fallbackp() ثابت { أرجع m_fallbackp; }
    فراغ parentp(VSymEnt* entp) { m_parentp = entp; }
    VSymEnt* parentp() ثابت { أرجع m_parentp; }
    فراغ classOrPackagep(Ast_گرہ_Module* entp) { m_classOrPackagep = entp; }
    Ast_گرہ_Module* classOrPackagep() ثابت { أرجع m_classOrPackagep; }
    Ast_گرہ* nodep() ثابت { أرجع m_nodep; }
    string sym_اگیتر() ثابت { أرجع m_sym_اگیتر; }
    فراغ sym_اگیتر(ثابت string& name) { m_sym_اگیتر = name; }
    bool exported() ثابت { أرجع m_exported; }
    فراغ exported(bool flag) { m_exported = flag; }
    bool imported() ثابت { أرجع m_imported; }
    فراغ imported(bool flag) { m_imported = flag; }
    VSymEnt* داخل_کرو(ثابت string& name, VSymEnt* entp) {
        UINFO(9, "     SymInsert se" << cvt_منزل_Hex(this) << " '" << name << "' se" << cvt_منزل_Hex(entp)
                                     << "  " << entp->nodep());
        إذا (name != "" && m_idNameMap.لبھو(name) != m_idNameMap.سرا()) {
            // If didn't already report warning
            إذا (!V3Error::error_گنتی()) {  // LCOV_EXCL_START
                إذا (debug() >= 9 || V3Error::debugDefault())
                    dumpSelf(std::cout, "- err-dump: ", 1);
                entp->nodep()->v3fatalSrc("Inserting two symbols with same name: " << name);
            }  // LCOV_EXCL_STOP
        } وإلا {
            m_idNameMap.emplace(name, entp);
        }
        إذا (!name.خالی_تھاواں() && name.لبھو("__DOT__") == std::string::npos  // ignore hierarchical
            && checkSimilarname(entp->nodep())) {
            string lc = name;
            لكل (خودکار& c : lc) c = (حرف)tolower(c);
            إذا (m_idNameSimilarMap.لبھو(lc) == m_idNameSimilarMap.سرا())
                m_idNameSimilarMap.داخل_کرو(lc);
        }

        أرجع entp;
    }
    فراغ reinsert(ثابت string& name, VSymEnt* entp) {
        ثابت خودکار it = m_idNameMap.لبھو(name);
        إذا (name != "" && it != m_idNameMap.سرا()) {
            UINFO(9, "     SymReinsert se" << cvt_منزل_Hex(this) << " '" << name << "' se"
                                           << cvt_منزل_Hex(entp) << "  " << entp->nodep());
            it->دوجا = entp;  // Replace
        } وإلا {
            داخل_کرو(name, entp);
        }
    }
    VSymEnt* لبھو_Id_Flat(ثابت string& name) ثابت {
        // Find identifier without looking upward through symbol hierarchy
        // First, scan this begin/end block or module for the name
        ثابت خودکار it = m_idNameMap.لبھو(name);
        UINFO(9, "     SymFind   se"
                     << cvt_منزل_Hex(this) << " '" << name << "' -> "
                     << (it == m_idNameMap.سرا() ? "NONE"
                                                 : "se" + cvt_منزل_Hex(it->دوجا)
                                                       + " n=" + cvt_منزل_Hex(it->دوجا->nodep())));
        إذا (it != m_idNameMap.سرا()) أرجع it->دوجا;
        أرجع nullptr;
    }
    ساكن bool checkSimilarname(ثابت Ast_گرہ* nodep) {
        // Only declarations that can contribute a name to the netlist handed
        // to a backend tool.
        إذا (ثابت AstVar* ثابت varp = VN_CAST(nodep, Var)) {
            أرجع !varp->isParam() && !varp->isGenVar();
        }
        أرجع VN_IS(nodep, Cell)  //
               || VN_IS(nodep, گرہ_بلاک)  //
               || VN_IS(nodep, Gen_بلاک)  //
               || VN_IS(nodep, Func)  // Note: not AstNodeFTask (Property/Sequence)
               || VN_IS(nodep, Task);
    }
    VSymEnt* لبھو_Similar_Id_Flat(ثابت string& name) ثابت {
        // Find identifier without looking upward through symbol hierarchy
        // Were looking for symbols that are the same when compared without
        // caring about case, but that are not the same name
        إذا (name.لبھو("__DOT__") != std::string::npos)  // ignore hierarchical equivalents
            أرجع nullptr;
        string s = name;
        لكل (خودکار& c : s) c = (حرف)tolower(c);
        إذا (m_idNameSimilarMap.لبھو(s) == m_idNameSimilarMap.سرا()) أرجع nullptr;
        لكل (خودکار it = m_idNameMap.begin(); it != m_idNameMap.سرا(); ++it) {
            // Only report a declaration that is itself checked, otherwise we
            // might point at e.g. a localparam that merely sorts first
            إذا (!checkSimilarname(it->دوجا->nodep())) استمر;
            string t = it->پہلا;
            لكل (خودکار& c : t) c = (حرف)tolower(c);
            إذا (t == s && name != it->پہلا) { أرجع it->دوجا; }
        }
        أرجع nullptr;
    }
    VSymEnt* لبھو_Id_Fallback(ثابت string& name) ثابت {
        // Find identifier looking upward through symbol hierarchy
        // First, scan this begin/end block or module for the name
        إذا (VSymEnt* ثابت entp = لبھو_Id_Flat(name)) أرجع entp;
        // Then scan the upper begin/end block or module for the name
        إذا (m_fallbackp) أرجع m_fallbackp->لبھو_Id_Fallback(name);
        أرجع nullptr;
    }
    فراغ candidateIdFlat(VSpellCheck* spellerp, ثابت VNodeMatcher* matcherp,
                         bool includeType) ثابت {
        // Suggest alternative symbol candidates without looking upward through symbol
        // hierarchy
        لكل (IdNameMap::const_iterator it = m_idNameMap.begin(); it != m_idNameMap.سرا(); ++it) {
            ثابت Ast_گرہ* ثابت itemp = it->دوجا->nodep();
            إذا (itemp && (!matcherp || matcherp->گرہ_میل(itemp))) {
                إذا (includeType) {
                    spellerp->ڈالو_Candidate(itemp->prettyName(), itemp->prettyTypeName());
                } وإلا {
                    spellerp->ڈالو_Candidate(itemp->prettyName());
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
    فراغ import_One_علامت(VSym_گراف* graphp, ثابت string& name, ثابت VSymEnt* srcp,
                         bool honorExport) {
        إذا ((!honorExport || srcp->exported())
            && !لبھو_Id_Flat(name)) {  // Don't insert over existing entry
            VSymEnt* ثابت symp = new VSymEnt{graphp, srcp};
            symp->exported(false);  // Can't reimport an import without an export
            symp->imported(true);
            reinsert(name, symp);
        }
    }
    فراغ export_One_علامت(VSym_گراف* /*graphp*/, ثابت string& name, ثابت VSymEnt* srcp) ثابت {
        إذا (srcp->exported()) {
            إذا (VSymEnt* ثابت symp = لبھو_Id_Flat(name)) {  // Should already exist in current table
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
        std::unordered_سیٹ<std::string> haveExterns;
        لكل (IdNameMap::const_iterator it = srcp->m_idNameMap.begin();
             it != srcp->m_idNameMap.سرا(); ++it) {
            إذا (m_idNameMap.گنتی("extern " + it->پہلا)) haveExterns.emplace(it->پہلا);
        }
        لكل (IdNameMap::const_iterator it = srcp->m_idNameMap.begin();
             it != srcp->m_idNameMap.سرا(); ++it) {
            إذا (!haveExterns.گنتی(it->پہلا))
                import_One_علامت(graphp, it->پہلا, it->دوجا, false);
        }
    }
    فراغ importFromPackage(VSym_گراف* graphp, ثابت VSymEnt* srcp, ثابت string& id_or_تارا) {
        // Import tokens from source symbol table into this symbol table
        إذا (id_or_تارا != "*") {
            ثابت خودکار it = srcp->m_idNameMap.لبھو(id_or_تارا);
            إذا (it != srcp->m_idNameMap.سرا()) {
                import_One_علامت(graphp, it->پہلا, it->دوجا, true);
            }
        } وإلا {
            لكل (IdNameMap::const_iterator it = srcp->m_idNameMap.begin();
                 it != srcp->m_idNameMap.سرا(); ++it) {
                import_One_علامت(graphp, it->پہلا, it->دوجا, true);
            }
        }
    }
    فراغ exportFromPackage(VSym_گراف* graphp, ثابت VSymEnt* srcp, ثابت string& id_or_تارا) {
        // Export tokens from source symbol table into this symbol table
        إذا (id_or_تارا != "*") {
            ثابت خودکار it = vlstd::as_const(srcp->m_idNameMap).لبھو(id_or_تارا);
            إذا (it != srcp->m_idNameMap.سرا()) export_One_علامت(graphp, it->پہلا, it->دوجا);
        } وإلا {
            لكل (IdNameMap::const_iterator it = srcp->m_idNameMap.begin();
                 it != srcp->m_idNameMap.سرا(); ++it) {
                export_One_علامت(graphp, it->پہلا, it->دوجا);
            }
        }
    }
    فراغ export_تارا_تارا(VSym_گراف* /*graphp*/) {
        // Export *:*: Export all tokens from imported packages
        لكل (IdNameMap::const_iterator it = m_idNameMap.begin(); it != m_idNameMap.سرا(); ++it) {
            VSymEnt* ثابت symp = it->دوجا;
            إذا (!symp->exported()) symp->exported(true);
        }
    }
    فراغ importFromIface(VSym_گراف* graphp, ثابت VSymEnt* srcp, bool onlyUnmodportable = false) {
        // Import interface tokens from source symbol table into this symbol table, recursively
        UINFO(9, "     importIf  se" << cvt_منزل_Hex(this) << " from se" << cvt_منزل_Hex(srcp));
        لكل (IdNameMap::const_iterator it = srcp->m_idNameMap.begin();
             it != srcp->m_idNameMap.سرا(); ++it) {
            ثابت string& name = it->پہلا;
            VSymEnt* ثابت subSrcp = it->دوجا;
            ثابت AstVar* ثابت varp = VN_CAST(subSrcp->nodep(), Var);
            إذا (!onlyUnmodportable || (varp && varp->isParam())) {
                VSymEnt* ثابت subSymp = new VSymEnt{graphp, subSrcp};
                reinsert(name, subSymp);
                // And recurse to create children
                subSymp->importFromIface(graphp, subSrcp);
            }
        }
    }
    string cellErrorScopes(ثابت Ast_گرہ* lookp, string prettyName = "") {
        إذا (prettyName == "") prettyName = lookp->prettyName();
        string scopes;
        لكل (IdNameMap::iterator it = m_idNameMap.begin(); it != m_idNameMap.سرا(); ++it) {
            ثابت Ast_گرہ* ثابت itemp = it->دوجا->nodep();
            إذا (VN_IS(itemp, Cell) || (VN_IS(itemp, Module) && VN_AS(itemp, Module)->is_چوٹی())) {
                إذا (scopes != "") scopes += ", ";
                scopes += Ast_گرہ::prettyName(it->پہلا);
            }
        }
        إذا (scopes == "") scopes = "<no instances found>";
        إذا (debug()) dumpSelf(std::cerr, "       KnownScope: ", 1);
        أرجع V3Error::warn_ہور() + "... Known scopes under '" + prettyName + "': " + scopes
               + '\n';
    }
};

//######################################################################
// Symbol tables

class VSym_گراف final {
    // Collection of symbol tables
    // TYPES
    using Sym_ڈھیر = std::سمتیہ<VSymEnt*>;

    // MEMBERS
    VSymEnt* m_symRootp;  // Root symbol table
    Sym_ڈھیر m_symsp;  // All symbol tables, to cleanup

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
        VSymConstMap پورا_Syms;
        os << "SymEnt Dump:\n";
        m_symRootp->dumpIterate(os, پورا_Syms, indent, 9999, "$root");
        bool پہلا = true;
        لكل (Sym_ڈھیر::iterator it = m_symsp.begin(); it != m_symsp.سرا(); ++it) {
            إذا (پورا_Syms.لبھو(*it) == پورا_Syms.سرا()) {
                إذا (پہلا) {
                    پہلا = false;
                    os << "%%Warning: SymEnt Orphans:\n";
                }
                (*it)->dumpIterate(os, پورا_Syms, indent, 9999, "Orphan");
            }
        }
    }
    فراغ dumpFilePrefixed(ثابت string& nameComment) {
        إذا (dump_درخت_Level()) {
            ثابت string filename = v3Global.debugFilename(nameComment) + ".txt";
            UINFO(2, "Dumping " << filename);
            ثابت std::unique_ptr<std::ofstream> logp{V3File::new_ofstream(filename)};
            إذا (logp->fail()) v3fatal("Can't write file: " << filename);
            dumpSelf(*logp, "");
        }
    }

محمي:
    یار class VSymEnt;
    فراغ ڈالو_New_Ent(VSymEnt* entp) { m_symsp.ڈالو_پٹھ(entp); }
};

//######################################################################

مضمن VSymEnt::VSymEnt(VSym_گراف* graphp, Ast_گرہ* nodep)
    : m_nodep{nodep} {
    // No argument to set fallbackp, as generally it's wrong to set it in the new call,
    // Instead it needs to be set on a "findOrNew()" return, as it may have been new'ed
    // by an earlier search insertion.
    graphp->ڈالو_New_Ent(this);
}

مضمن VSymEnt::VSymEnt(VSym_گراف* graphp, ثابت VSymEnt* symp)
    : m_nodep{symp->m_nodep}
    , m_fallbackp{symp->m_fallbackp}
    , m_parentp{symp->m_parentp}
    , m_classOrPackagep{symp->m_classOrPackagep}
    , m_exported{symp->m_exported}
    , m_imported{symp->m_imported} {
    graphp->ڈالو_New_Ent(this);
}

#جے_انت  // guard
