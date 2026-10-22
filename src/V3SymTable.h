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

#اگر_غیر_تعریف_شدہ VERILATOR_V3SYMTABLE_H_
#تعریف_کرو VERILATOR_V3SYMTABLE_H_

#شامل "config_build.h"
#شامل "verilatedos.h"

#شامل "V3Ast.h"
#شامل "V3File.h"
#شامل "V3Global.h"
#شامل "V3String.h"

#شامل <cstdarg>
#شامل <iomanip>
#شامل <map>
#شامل <memory>
#شامل <unordered_set>
#شامل <vector>

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
    string m_sym_سابقہ;  // String to prefix symbols with (for V3LinkDot, unused here)
    bool m_exported = true;  // Allow importing
    bool m_imported = false;  // Was imported
#اگر_تعریف_شدہ VL_DEBUG
    VL_DEFINE_DEBUG_FUNCTIONS;
#ورنہ
    ساکن constexpr صحیح debug() { واپس 0; }  // NOT runtime, too hot of a function
#اگر_ختم
عوامی:
    using const_iterator = IdNameMap::const_iterator;
    const_iterator begin() ثابت { واپس m_idNameMap.begin(); }
    const_iterator انتہا() ثابت { واپس m_idNameMap.انتہا(); }

    خالی dumpIterate(std::ostream& os, VSymConstMap& مکمل_Symsr, ثابت string& indent,
                     صحیح عدد_سطحیں, ثابت string& تلاش_Name) ثابت {
        os << indent << "+ " << std::بایاں << std::setw(30) << ("'"s + تلاش_Name + "'"s)
           << std::setw(0) << std::دایاں;
        os << "  se" << cvt_مقصد_Hex(this) << std::setw(0);
        os << "  fallb=se" << cvt_مقصد_Hex(m_fallbackp);
        اگر (m_imported || m_exported) {
            os << "  ";
            اگر (m_imported) os << "[I]";
            اگر (m_exported) os << "[E]";
        }
        اگر (m_sym_سابقہ != "") os << "  symPrefix=" << m_sym_سابقہ;
        اگر (nodep()) os << "  n=" << nodep();
        os << '\n';
        اگر (VL_UNCOVERABLE(!مکمل_Symsr.داخل_کرو(this).دوسرا)) {
            os << indent << "| ^ duplicate, so no children printed\n";  // LCOV_EXCL_LINE
        } ورنہ {
            برائے (خودکار it = m_idNameMap.begin(); it != m_idNameMap.انتہا(); ++it) {
                اگر (عدد_سطحیں >= 1) {
                    it->دوسرا->dumpIterate(os, مکمل_Symsr, indent + "| ", عدد_سطحیں - 1,
                                            it->پہلا);
                }
            }
        }
    }
    خالی dumpSelf(std::ostream& os, ثابت string& indent = "", صحیح عدد_سطحیں = 1) ثابت {
        VSymConstMap مکمل_Syms;
        dumpIterate(os, مکمل_Syms, indent, عدد_سطحیں, "TOP");
    }

    // METHODS
    VSymEnt(VSym_گراف* graphp, ثابت VSymEnt* symp);  //نیچے
    VSymEnt(VSym_گراف* graphp, Ast_گرہ* nodep);  //نیچے
    ~VSymEnt() {
        // Change links so we coredump if used
#اگر_تعریف_شدہ VL_DEBUG
        m_nodep = reinterpret_cast<Ast_گرہ*>(1);
        m_fallbackp = reinterpret_cast<VSymEnt*>(1);
        m_parentp = reinterpret_cast<VSymEnt*>(1);
        m_classOrPackagep = reinterpret_cast<Ast_گرہ_Module*>(1);
#اگر_ختم
    }
#اگر (defined(VL_DEBUG) || defined(VL_ALLOC_RANDOM_CHECKS)) && !defined(VL_LEAK_CHECKS)
    // For testing, leak so above destructor 1 assignments work
    خالی* عامل new(سائز_t سائز) { واپس std::حافظہ_دو(سائز); }
    خالی عامل delete(خالی* objp, سائز_t سائز) {}
#اگر_ختم
    خالی fallbackp(VSymEnt* entp) { m_fallbackp = entp; }
    VSymEnt* fallbackp() ثابت { واپس m_fallbackp; }
    خالی parentp(VSymEnt* entp) { m_parentp = entp; }
    VSymEnt* parentp() ثابت { واپس m_parentp; }
    خالی classOrPackagep(Ast_گرہ_Module* entp) { m_classOrPackagep = entp; }
    Ast_گرہ_Module* classOrPackagep() ثابت { واپس m_classOrPackagep; }
    Ast_گرہ* nodep() ثابت { واپس m_nodep; }
    string sym_سابقہ() ثابت { واپس m_sym_سابقہ; }
    خالی sym_سابقہ(ثابت string& name) { m_sym_سابقہ = name; }
    bool exported() ثابت { واپس m_exported; }
    خالی exported(bool flag) { m_exported = flag; }
    bool imported() ثابت { واپس m_imported; }
    خالی imported(bool flag) { m_imported = flag; }
    VSymEnt* داخل_کرو(ثابت string& name, VSymEnt* entp) {
        UINFO(9, "     SymInsert se" << cvt_مقصد_Hex(this) << " '" << name << "' se" << cvt_مقصد_Hex(entp)
                                     << "  " << entp->nodep());
        اگر (name != "" && m_idNameMap.ڈھونڈو(name) != m_idNameMap.انتہا()) {
            // If didn't already report warning
            اگر (!V3Error::error_گنتی()) {  // LCOV_EXCL_START
                اگر (debug() >= 9 || V3Error::debugDefault())
                    dumpSelf(std::cout, "- err-dump: ", 1);
                entp->nodep()->v3fatalSrc("Inserting two symbols with same name: " << name);
            }  // LCOV_EXCL_STOP
        } ورنہ {
            m_idNameMap.emplace(name, entp);
        }
        اگر (!name.خالی_خانے() && name.ڈھونڈو("__DOT__") == std::string::npos  // ignore hierarchical
            && checkSimilarname(entp->nodep())) {
            string lc = name;
            برائے (خودکار& c : lc) c = (حرف)tolower(c);
            اگر (m_idNameSimilarMap.ڈھونڈو(lc) == m_idNameSimilarMap.انتہا())
                m_idNameSimilarMap.داخل_کرو(lc);
        }

        واپس entp;
    }
    خالی reinsert(ثابت string& name, VSymEnt* entp) {
        ثابت خودکار it = m_idNameMap.ڈھونڈو(name);
        اگر (name != "" && it != m_idNameMap.انتہا()) {
            UINFO(9, "     SymReinsert se" << cvt_مقصد_Hex(this) << " '" << name << "' se"
                                           << cvt_مقصد_Hex(entp) << "  " << entp->nodep());
            it->دوسرا = entp;  // Replace
        } ورنہ {
            داخل_کرو(name, entp);
        }
    }
    VSymEnt* ڈھونڈو_Id_Flat(ثابت string& name) ثابت {
        // Find identifier without looking upward through symbol hierarchy
        // First, scan this begin/end block or module for the name
        ثابت خودکار it = m_idNameMap.ڈھونڈو(name);
        UINFO(9, "     SymFind   se"
                     << cvt_مقصد_Hex(this) << " '" << name << "' -> "
                     << (it == m_idNameMap.انتہا() ? "NONE"
                                                 : "se" + cvt_مقصد_Hex(it->دوسرا)
                                                       + " n=" + cvt_مقصد_Hex(it->دوسرا->nodep())));
        اگر (it != m_idNameMap.انتہا()) واپس it->دوسرا;
        واپس nullptr;
    }
    ساکن bool checkSimilarname(ثابت Ast_گرہ* nodep) {
        // Only declarations that can contribute a name to the netlist handed
        // to a backend tool.
        اگر (ثابت AstVar* ثابت varp = VN_CAST(nodep, Var)) {
            واپس !varp->isParam() && !varp->isGenVar();
        }
        واپس VN_IS(nodep, Cell)  //
               || VN_IS(nodep, گرہ_بلاک)  //
               || VN_IS(nodep, Gen_بلاک)  //
               || VN_IS(nodep, Func)  // Note: not AstNodeFTask (Property/Sequence)
               || VN_IS(nodep, Task);
    }
    VSymEnt* ڈھونڈو_Similar_Id_Flat(ثابت string& name) ثابت {
        // Find identifier without looking upward through symbol hierarchy
        // Were looking for symbols that are the same when compared without
        // caring about case, but that are not the same name
        اگر (name.ڈھونڈو("__DOT__") != std::string::npos)  // ignore hierarchical equivalents
            واپس nullptr;
        string s = name;
        برائے (خودکار& c : s) c = (حرف)tolower(c);
        اگر (m_idNameSimilarMap.ڈھونڈو(s) == m_idNameSimilarMap.انتہا()) واپس nullptr;
        برائے (خودکار it = m_idNameMap.begin(); it != m_idNameMap.انتہا(); ++it) {
            // Only report a declaration that is itself checked, otherwise we
            // might point at e.g. a localparam that merely sorts first
            اگر (!checkSimilarname(it->دوسرا->nodep())) جاری;
            string t = it->پہلا;
            برائے (خودکار& c : t) c = (حرف)tolower(c);
            اگر (t == s && name != it->پہلا) { واپس it->دوسرا; }
        }
        واپس nullptr;
    }
    VSymEnt* ڈھونڈو_Id_Fallback(ثابت string& name) ثابت {
        // Find identifier looking upward through symbol hierarchy
        // First, scan this begin/end block or module for the name
        اگر (VSymEnt* ثابت entp = ڈھونڈو_Id_Flat(name)) واپس entp;
        // Then scan the upper begin/end block or module for the name
        اگر (m_fallbackp) واپس m_fallbackp->ڈھونڈو_Id_Fallback(name);
        واپس nullptr;
    }
    خالی candidateIdFlat(VSpellCheck* spellerp, ثابت VNodeMatcher* matcherp,
                         bool includeType) ثابت {
        // Suggest alternative symbol candidates without looking upward through symbol
        // hierarchy
        برائے (IdNameMap::const_iterator it = m_idNameMap.begin(); it != m_idNameMap.انتہا(); ++it) {
            ثابت Ast_گرہ* ثابت itemp = it->دوسرا->nodep();
            اگر (itemp && (!matcherp || matcherp->گرہ_مطابقت(itemp))) {
                اگر (includeType) {
                    spellerp->ڈالو_Candidate(itemp->prettyName(), itemp->prettyTypeName());
                } ورنہ {
                    spellerp->ڈالو_Candidate(itemp->prettyName());
                }
            }
        }
    }
    خالی candidateIdFallback(VSpellCheck* spellerp, ثابت VNodeMatcher* matcherp) ثابت {
        // Suggest alternative symbol candidates with looking upward through symbol hierarchy
        // Note VSpellCheck wants the most important (closest) items pushed first
        candidateIdFlat(spellerp, matcherp, false);
        // Then suggest the upper begin/end block or module
        اگر (m_fallbackp) m_fallbackp->candidateIdFallback(spellerp, matcherp);
    }

نجی:
    خالی import_One_علامت(VSym_گراف* graphp, ثابت string& name, ثابت VSymEnt* srcp,
                         bool honorExport) {
        اگر ((!honorExport || srcp->exported())
            && !ڈھونڈو_Id_Flat(name)) {  // Don't insert over existing entry
            VSymEnt* ثابت symp = new VSymEnt{graphp, srcp};
            symp->exported(false);  // Can't reimport an import without an export
            symp->imported(true);
            reinsert(name, symp);
        }
    }
    خالی export_One_علامت(VSym_گراف* /*graphp*/, ثابت string& name, ثابت VSymEnt* srcp) ثابت {
        اگر (srcp->exported()) {
            اگر (VSymEnt* ثابت symp = ڈھونڈو_Id_Flat(name)) {  // Should already exist in current table
                اگر (!symp->exported()) symp->exported(true);
            }
        }
    }

عوامی:
    خالی importFromClass(VSym_گراف* graphp, ثابت VSymEnt* srcp) {
        // Import tokens from source symbol table into this symbol table
        // Used for classes in early parsing only to handle "extends"

        // If an "extern foo" exists, then we can't import "foo" from the base class.
        // But ok for "extern foo" and "foo" to both come from base (so must check before
        // insert)
        std::unordered_سیٹ<std::string> haveExterns;
        برائے (IdNameMap::const_iterator it = srcp->m_idNameMap.begin();
             it != srcp->m_idNameMap.انتہا(); ++it) {
            اگر (m_idNameMap.گنتی("extern " + it->پہلا)) haveExterns.emplace(it->پہلا);
        }
        برائے (IdNameMap::const_iterator it = srcp->m_idNameMap.begin();
             it != srcp->m_idNameMap.انتہا(); ++it) {
            اگر (!haveExterns.گنتی(it->پہلا))
                import_One_علامت(graphp, it->پہلا, it->دوسرا, false);
        }
    }
    خالی importFromPackage(VSym_گراف* graphp, ثابت VSymEnt* srcp, ثابت string& id_or_ستارہ) {
        // Import tokens from source symbol table into this symbol table
        اگر (id_or_ستارہ != "*") {
            ثابت خودکار it = srcp->m_idNameMap.ڈھونڈو(id_or_ستارہ);
            اگر (it != srcp->m_idNameMap.انتہا()) {
                import_One_علامت(graphp, it->پہلا, it->دوسرا, true);
            }
        } ورنہ {
            برائے (IdNameMap::const_iterator it = srcp->m_idNameMap.begin();
                 it != srcp->m_idNameMap.انتہا(); ++it) {
                import_One_علامت(graphp, it->پہلا, it->دوسرا, true);
            }
        }
    }
    خالی exportFromPackage(VSym_گراف* graphp, ثابت VSymEnt* srcp, ثابت string& id_or_ستارہ) {
        // Export tokens from source symbol table into this symbol table
        اگر (id_or_ستارہ != "*") {
            ثابت خودکار it = vlstd::as_const(srcp->m_idNameMap).ڈھونڈو(id_or_ستارہ);
            اگر (it != srcp->m_idNameMap.انتہا()) export_One_علامت(graphp, it->پہلا, it->دوسرا);
        } ورنہ {
            برائے (IdNameMap::const_iterator it = srcp->m_idNameMap.begin();
                 it != srcp->m_idNameMap.انتہا(); ++it) {
                export_One_علامت(graphp, it->پہلا, it->دوسرا);
            }
        }
    }
    خالی export_ستارہ_ستارہ(VSym_گراف* /*graphp*/) {
        // Export *:*: Export all tokens from imported packages
        برائے (IdNameMap::const_iterator it = m_idNameMap.begin(); it != m_idNameMap.انتہا(); ++it) {
            VSymEnt* ثابت symp = it->دوسرا;
            اگر (!symp->exported()) symp->exported(true);
        }
    }
    خالی importFromIface(VSym_گراف* graphp, ثابت VSymEnt* srcp, bool onlyUnmodportable = false) {
        // Import interface tokens from source symbol table into this symbol table, recursively
        UINFO(9, "     importIf  se" << cvt_مقصد_Hex(this) << " from se" << cvt_مقصد_Hex(srcp));
        برائے (IdNameMap::const_iterator it = srcp->m_idNameMap.begin();
             it != srcp->m_idNameMap.انتہا(); ++it) {
            ثابت string& name = it->پہلا;
            VSymEnt* ثابت subSrcp = it->دوسرا;
            ثابت AstVar* ثابت varp = VN_CAST(subSrcp->nodep(), Var);
            اگر (!onlyUnmodportable || (varp && varp->isParam())) {
                VSymEnt* ثابت subSymp = new VSymEnt{graphp, subSrcp};
                reinsert(name, subSymp);
                // And recurse to create children
                subSymp->importFromIface(graphp, subSrcp);
            }
        }
    }
    string cellErrorScopes(ثابت Ast_گرہ* lookp, string prettyName = "") {
        اگر (prettyName == "") prettyName = lookp->prettyName();
        string scopes;
        برائے (IdNameMap::iterator it = m_idNameMap.begin(); it != m_idNameMap.انتہا(); ++it) {
            ثابت Ast_گرہ* ثابت itemp = it->دوسرا->nodep();
            اگر (VN_IS(itemp, Cell) || (VN_IS(itemp, Module) && VN_AS(itemp, Module)->is_چوٹی())) {
                اگر (scopes != "") scopes += ", ";
                scopes += Ast_گرہ::prettyName(it->پہلا);
            }
        }
        اگر (scopes == "") scopes = "<no instances found>";
        اگر (debug()) dumpSelf(std::cerr, "       KnownScope: ", 1);
        واپس V3Error::warn_مزید() + "... Known scopes under '" + prettyName + "': " + scopes
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

عوامی:
    explicit VSym_گراف(AstNetlist* nodep) { m_symRootp = new VSymEnt{this, nodep}; }
    ~VSym_گراف() {
        برائے (ثابت VSymEnt* entp : m_symsp) delete entp;
    }

    // METHODS
    VSymEnt* rootp() ثابت { واپس m_symRootp; }
    // Debug
    خالی dumpSelf(std::ostream& os, ثابت string& indent = "") {
        VSymConstMap مکمل_Syms;
        os << "SymEnt Dump:\n";
        m_symRootp->dumpIterate(os, مکمل_Syms, indent, 9999, "$root");
        bool پہلا = true;
        برائے (Sym_ڈھیر::iterator it = m_symsp.begin(); it != m_symsp.انتہا(); ++it) {
            اگر (مکمل_Syms.ڈھونڈو(*it) == مکمل_Syms.انتہا()) {
                اگر (پہلا) {
                    پہلا = false;
                    os << "%%Warning: SymEnt Orphans:\n";
                }
                (*it)->dumpIterate(os, مکمل_Syms, indent, 9999, "Orphan");
            }
        }
    }
    خالی dumpFilePrefixed(ثابت string& nameComment) {
        اگر (dump_درخت_Level()) {
            ثابت string filename = v3Global.debugFilename(nameComment) + ".txt";
            UINFO(2, "Dumping " << filename);
            ثابت std::unique_ptr<std::ofstream> logp{V3File::new_ofstream(filename)};
            اگر (logp->fail()) v3fatal("Can't write file: " << filename);
            dumpSelf(*logp, "");
        }
    }

محفوظ:
    دوست class VSymEnt;
    خالی ڈالو_New_Ent(VSymEnt* entp) { m_symsp.ڈالو_عقب(entp); }
};

//######################################################################

اندرونی VSymEnt::VSymEnt(VSym_گراف* graphp, Ast_گرہ* nodep)
    : m_nodep{nodep} {
    // No argument to set fallbackp, as generally it's wrong to set it in the new call,
    // Instead it needs to be set on a "findOrNew()" return, as it may have been new'ed
    // by an earlier search insertion.
    graphp->ڈالو_New_Ent(this);
}

اندرونی VSymEnt::VSymEnt(VSym_گراف* graphp, ثابت VSymEnt* symp)
    : m_nodep{symp->m_nodep}
    , m_fallbackp{symp->m_fallbackp}
    , m_parentp{symp->m_parentp}
    , m_classOrPackagep{symp->m_classOrPackagep}
    , m_exported{symp->m_exported}
    , m_imported{symp->m_imported} {
    graphp->ڈالو_New_Ent(this);
}

#اگر_ختم  // guard
