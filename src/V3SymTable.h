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

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ VERILATOR_V3SYMTABLE_H_
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VERILATOR_V3SYMTABLE_H_

#ਅਨ੍ਤਰ੍ਭਾਵ "config_build.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Ast.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3File.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Global.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3String.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <cstdarg>
#ਅਨ੍ਤਰ੍ਭਾਵ <iomanip>
#ਅਨ੍ਤਰ੍ਭਾਵ <map>
#ਅਨ੍ਤਰ੍ਭਾਵ <memory>
#ਅਨ੍ਤਰ੍ਭਾਵ <unordered_set>
#ਅਨ੍ਤਰ੍ਭਾਵ <vector>

class VSym_ਗ੍ਰਾਫ;
class VSymEnt;

//######################################################################
// Symbol table

using VSymConstMap = std::unordered_ਸੈੱਟ<ਅਚਲ VSymEnt*>;

class VSymEnt final {
    // Symbol table that can have a "superior" table for resolving upper references
    // MEMBERS
    using IdNameMap = std::multimap<std::string, VSymEnt*>;
    IdNameMap m_idNameMap;  // Hash of variables by name
    std::unordered_ਸੈੱਟ<std::string> m_idNameSimilarMap;  // Variables by name with same case
    Ast_ਗੰਢ* m_nodep;  // Node that entry belongs to
    VSymEnt* m_fallbackp = nullptr;  // Table "above" this in name scope, for fallback resolution
    VSymEnt* m_parentp = nullptr;  // Table that created this
    Ast_ਗੰਢ_Module* m_classOrPackagep = nullptr;  // Package node is in (for V3LinkDot, unused here)
    string m_sym_ਅਗੇਤਰ;  // String to prefix symbols with (for V3LinkDot, unused here)
    bool m_exported = true;  // Allow importing
    bool m_imported = false;  // Was imported
#ਜੇ_ਪਰਿਭਾਸ਼ਿਤ VL_DEBUG
    VL_DEFINE_DEBUG_FUNCTIONS;
#ਨਹੀਂਤਾਂ
    ਸਥਿਰ constexpr ਪੂਰਨ debug() { ਵਾਪਸ 0; }  // NOT runtime, too hot of a function
#ਜੇ_ਅੰਤ
ਜਨਤਕ:
    using const_iterator = IdNameMap::const_iterator;
    const_iterator begin() ਅਚਲ { ਵਾਪਸ m_idNameMap.begin(); }
    const_iterator ਸਿਰਾ() ਅਚਲ { ਵਾਪਸ m_idNameMap.ਸਿਰਾ(); }

    ਖਾਲੀ dumpIterate(std::ostream& os, VSymConstMap& ਪੂਰਾ_Symsr, ਅਚਲ string& indent,
                     ਪੂਰਨ ਅੰਕ_ਪੱਧਰ, ਅਚਲ string& ਖੋਜ_Name) ਅਚਲ {
        os << indent << "+ " << std::ਖੱਬਾ << std::setw(30) << ("'"s + ਖੋਜ_Name + "'"s)
           << std::setw(0) << std::ਸੱਜਾ;
        os << "  se" << cvt_ਮੰਜ਼ਿਲ_Hex(this) << std::setw(0);
        os << "  fallb=se" << cvt_ਮੰਜ਼ਿਲ_Hex(m_fallbackp);
        ਜੇ (m_imported || m_exported) {
            os << "  ";
            ਜੇ (m_imported) os << "[I]";
            ਜੇ (m_exported) os << "[E]";
        }
        ਜੇ (m_sym_ਅਗੇਤਰ != "") os << "  symPrefix=" << m_sym_ਅਗੇਤਰ;
        ਜੇ (nodep()) os << "  n=" << nodep();
        os << '\n';
        ਜੇ (VL_UNCOVERABLE(!ਪੂਰਾ_Symsr.ਪਾਓ(this).ਦੂਜਾ)) {
            os << indent << "| ^ duplicate, so no children printed\n";  // LCOV_EXCL_LINE
        } ਨਹੀਂਤਾਂ {
            ਲਈ (ਸਵੈਚਾਲਿਤ it = m_idNameMap.begin(); it != m_idNameMap.ਸਿਰਾ(); ++it) {
                ਜੇ (ਅੰਕ_ਪੱਧਰ >= 1) {
                    it->ਦੂਜਾ->dumpIterate(os, ਪੂਰਾ_Symsr, indent + "| ", ਅੰਕ_ਪੱਧਰ - 1,
                                            it->ਪਹਿਲਾ);
                }
            }
        }
    }
    ਖਾਲੀ dumpSelf(std::ostream& os, ਅਚਲ string& indent = "", ਪੂਰਨ ਅੰਕ_ਪੱਧਰ = 1) ਅਚਲ {
        VSymConstMap ਪੂਰਾ_Syms;
        dumpIterate(os, ਪੂਰਾ_Syms, indent, ਅੰਕ_ਪੱਧਰ, "TOP");
    }

    // METHODS
    VSymEnt(VSym_ਗ੍ਰਾਫ* graphp, ਅਚਲ VSymEnt* symp);  //ਹੇਠਾਂ
    VSymEnt(VSym_ਗ੍ਰਾਫ* graphp, Ast_ਗੰਢ* nodep);  //ਹੇਠਾਂ
    ~VSymEnt() {
        // Change links so we coredump if used
#ਜੇ_ਪਰਿਭਾਸ਼ਿਤ VL_DEBUG
        m_nodep = reinterpret_cast<Ast_ਗੰਢ*>(1);
        m_fallbackp = reinterpret_cast<VSymEnt*>(1);
        m_parentp = reinterpret_cast<VSymEnt*>(1);
        m_classOrPackagep = reinterpret_cast<Ast_ਗੰਢ_Module*>(1);
#ਜੇ_ਅੰਤ
    }
#ਜੇ (defined(VL_DEBUG) || defined(VL_ALLOC_RANDOM_CHECKS)) && !defined(VL_LEAK_CHECKS)
    // For testing, leak so above destructor 1 assignments work
    ਖਾਲੀ* ਸੰਚਾਲਕ new(ਮਾਪ_t ਮਾਪ) { ਵਾਪਸ std::ਸਮ੍ਰਿਤੀ_ਦਿਓ(ਮਾਪ); }
    ਖਾਲੀ ਸੰਚਾਲਕ delete(ਖਾਲੀ* objp, ਮਾਪ_t ਮਾਪ) {}
#ਜੇ_ਅੰਤ
    ਖਾਲੀ fallbackp(VSymEnt* entp) { m_fallbackp = entp; }
    VSymEnt* fallbackp() ਅਚਲ { ਵਾਪਸ m_fallbackp; }
    ਖਾਲੀ parentp(VSymEnt* entp) { m_parentp = entp; }
    VSymEnt* parentp() ਅਚਲ { ਵਾਪਸ m_parentp; }
    ਖਾਲੀ classOrPackagep(Ast_ਗੰਢ_Module* entp) { m_classOrPackagep = entp; }
    Ast_ਗੰਢ_Module* classOrPackagep() ਅਚਲ { ਵਾਪਸ m_classOrPackagep; }
    Ast_ਗੰਢ* nodep() ਅਚਲ { ਵਾਪਸ m_nodep; }
    string sym_ਅਗੇਤਰ() ਅਚਲ { ਵਾਪਸ m_sym_ਅਗੇਤਰ; }
    ਖਾਲੀ sym_ਅਗੇਤਰ(ਅਚਲ string& name) { m_sym_ਅਗੇਤਰ = name; }
    bool exported() ਅਚਲ { ਵਾਪਸ m_exported; }
    ਖਾਲੀ exported(bool flag) { m_exported = flag; }
    bool imported() ਅਚਲ { ਵਾਪਸ m_imported; }
    ਖਾਲੀ imported(bool flag) { m_imported = flag; }
    VSymEnt* ਪਾਓ(ਅਚਲ string& name, VSymEnt* entp) {
        UINFO(9, "     SymInsert se" << cvt_ਮੰਜ਼ਿਲ_Hex(this) << " '" << name << "' se" << cvt_ਮੰਜ਼ਿਲ_Hex(entp)
                                     << "  " << entp->nodep());
        ਜੇ (name != "" && m_idNameMap.ਲੱਭੋ(name) != m_idNameMap.ਸਿਰਾ()) {
            // If didn't already report warning
            ਜੇ (!V3Error::error_ਗਿਣਤੀ()) {  // LCOV_EXCL_START
                ਜੇ (debug() >= 9 || V3Error::debugDefault())
                    dumpSelf(std::cout, "- err-dump: ", 1);
                entp->nodep()->v3fatalSrc("Inserting two symbols with same name: " << name);
            }  // LCOV_EXCL_STOP
        } ਨਹੀਂਤਾਂ {
            m_idNameMap.emplace(name, entp);
        }
        ਜੇ (!name.ਖਾਲੀ_ਥਾਵਾਂ() && name.ਲੱਭੋ("__DOT__") == std::string::npos  // ignore hierarchical
            && checkSimilarname(entp->nodep())) {
            string lc = name;
            ਲਈ (ਸਵੈਚਾਲਿਤ& c : lc) c = (ਅੱਖਰ)tolower(c);
            ਜੇ (m_idNameSimilarMap.ਲੱਭੋ(lc) == m_idNameSimilarMap.ਸਿਰਾ())
                m_idNameSimilarMap.ਪਾਓ(lc);
        }

        ਵਾਪਸ entp;
    }
    ਖਾਲੀ reinsert(ਅਚਲ string& name, VSymEnt* entp) {
        ਅਚਲ ਸਵੈਚਾਲਿਤ it = m_idNameMap.ਲੱਭੋ(name);
        ਜੇ (name != "" && it != m_idNameMap.ਸਿਰਾ()) {
            UINFO(9, "     SymReinsert se" << cvt_ਮੰਜ਼ਿਲ_Hex(this) << " '" << name << "' se"
                                           << cvt_ਮੰਜ਼ਿਲ_Hex(entp) << "  " << entp->nodep());
            it->ਦੂਜਾ = entp;  // Replace
        } ਨਹੀਂਤਾਂ {
            ਪਾਓ(name, entp);
        }
    }
    VSymEnt* ਲੱਭੋ_Id_Flat(ਅਚਲ string& name) ਅਚਲ {
        // Find identifier without looking upward through symbol hierarchy
        // First, scan this begin/end block or module for the name
        ਅਚਲ ਸਵੈਚਾਲਿਤ it = m_idNameMap.ਲੱਭੋ(name);
        UINFO(9, "     SymFind   se"
                     << cvt_ਮੰਜ਼ਿਲ_Hex(this) << " '" << name << "' -> "
                     << (it == m_idNameMap.ਸਿਰਾ() ? "NONE"
                                                 : "se" + cvt_ਮੰਜ਼ਿਲ_Hex(it->ਦੂਜਾ)
                                                       + " n=" + cvt_ਮੰਜ਼ਿਲ_Hex(it->ਦੂਜਾ->nodep())));
        ਜੇ (it != m_idNameMap.ਸਿਰਾ()) ਵਾਪਸ it->ਦੂਜਾ;
        ਵਾਪਸ nullptr;
    }
    ਸਥਿਰ bool checkSimilarname(ਅਚਲ Ast_ਗੰਢ* nodep) {
        // Only declarations that can contribute a name to the netlist handed
        // to a backend tool.
        ਜੇ (ਅਚਲ AstVar* ਅਚਲ varp = VN_CAST(nodep, Var)) {
            ਵਾਪਸ !varp->isParam() && !varp->isGenVar();
        }
        ਵਾਪਸ VN_IS(nodep, Cell)  //
               || VN_IS(nodep, ਗੰਢ_ਖੰਡ)  //
               || VN_IS(nodep, Gen_ਖੰਡ)  //
               || VN_IS(nodep, Func)  // Note: not AstNodeFTask (Property/Sequence)
               || VN_IS(nodep, Task);
    }
    VSymEnt* ਲੱਭੋ_Similar_Id_Flat(ਅਚਲ string& name) ਅਚਲ {
        // Find identifier without looking upward through symbol hierarchy
        // Were looking for symbols that are the same when compared without
        // caring about case, but that are not the same name
        ਜੇ (name.ਲੱਭੋ("__DOT__") != std::string::npos)  // ignore hierarchical equivalents
            ਵਾਪਸ nullptr;
        string s = name;
        ਲਈ (ਸਵੈਚਾਲਿਤ& c : s) c = (ਅੱਖਰ)tolower(c);
        ਜੇ (m_idNameSimilarMap.ਲੱਭੋ(s) == m_idNameSimilarMap.ਸਿਰਾ()) ਵਾਪਸ nullptr;
        ਲਈ (ਸਵੈਚਾਲਿਤ it = m_idNameMap.begin(); it != m_idNameMap.ਸਿਰਾ(); ++it) {
            // Only report a declaration that is itself checked, otherwise we
            // might point at e.g. a localparam that merely sorts first
            ਜੇ (!checkSimilarname(it->ਦੂਜਾ->nodep())) ਜਾਰੀ;
            string t = it->ਪਹਿਲਾ;
            ਲਈ (ਸਵੈਚਾਲਿਤ& c : t) c = (ਅੱਖਰ)tolower(c);
            ਜੇ (t == s && name != it->ਪਹਿਲਾ) { ਵਾਪਸ it->ਦੂਜਾ; }
        }
        ਵਾਪਸ nullptr;
    }
    VSymEnt* ਲੱਭੋ_Id_Fallback(ਅਚਲ string& name) ਅਚਲ {
        // Find identifier looking upward through symbol hierarchy
        // First, scan this begin/end block or module for the name
        ਜੇ (VSymEnt* ਅਚਲ entp = ਲੱਭੋ_Id_Flat(name)) ਵਾਪਸ entp;
        // Then scan the upper begin/end block or module for the name
        ਜੇ (m_fallbackp) ਵਾਪਸ m_fallbackp->ਲੱਭੋ_Id_Fallback(name);
        ਵਾਪਸ nullptr;
    }
    ਖਾਲੀ candidateIdFlat(VSpellCheck* spellerp, ਅਚਲ VNodeMatcher* matcherp,
                         bool includeType) ਅਚਲ {
        // Suggest alternative symbol candidates without looking upward through symbol
        // hierarchy
        ਲਈ (IdNameMap::const_iterator it = m_idNameMap.begin(); it != m_idNameMap.ਸਿਰਾ(); ++it) {
            ਅਚਲ Ast_ਗੰਢ* ਅਚਲ itemp = it->ਦੂਜਾ->nodep();
            ਜੇ (itemp && (!matcherp || matcherp->ਗੰਢ_ਮੇਲ(itemp))) {
                ਜੇ (includeType) {
                    spellerp->ਧੱਕੋ_Candidate(itemp->prettyName(), itemp->prettyTypeName());
                } ਨਹੀਂਤਾਂ {
                    spellerp->ਧੱਕੋ_Candidate(itemp->prettyName());
                }
            }
        }
    }
    ਖਾਲੀ candidateIdFallback(VSpellCheck* spellerp, ਅਚਲ VNodeMatcher* matcherp) ਅਚਲ {
        // Suggest alternative symbol candidates with looking upward through symbol hierarchy
        // Note VSpellCheck wants the most important (closest) items pushed first
        candidateIdFlat(spellerp, matcherp, false);
        // Then suggest the upper begin/end block or module
        ਜੇ (m_fallbackp) m_fallbackp->candidateIdFallback(spellerp, matcherp);
    }

ਨਿਜੀ:
    ਖਾਲੀ import_One_ਚਿੰਨ੍ਹ(VSym_ਗ੍ਰਾਫ* graphp, ਅਚਲ string& name, ਅਚਲ VSymEnt* srcp,
                         bool honorExport) {
        ਜੇ ((!honorExport || srcp->exported())
            && !ਲੱਭੋ_Id_Flat(name)) {  // Don't insert over existing entry
            VSymEnt* ਅਚਲ symp = new VSymEnt{graphp, srcp};
            symp->exported(false);  // Can't reimport an import without an export
            symp->imported(true);
            reinsert(name, symp);
        }
    }
    ਖਾਲੀ export_One_ਚਿੰਨ੍ਹ(VSym_ਗ੍ਰਾਫ* /*graphp*/, ਅਚਲ string& name, ਅਚਲ VSymEnt* srcp) ਅਚਲ {
        ਜੇ (srcp->exported()) {
            ਜੇ (VSymEnt* ਅਚਲ symp = ਲੱਭੋ_Id_Flat(name)) {  // Should already exist in current table
                ਜੇ (!symp->exported()) symp->exported(true);
            }
        }
    }

ਜਨਤਕ:
    ਖਾਲੀ importFromClass(VSym_ਗ੍ਰਾਫ* graphp, ਅਚਲ VSymEnt* srcp) {
        // Import tokens from source symbol table into this symbol table
        // Used for classes in early parsing only to handle "extends"

        // If an "extern foo" exists, then we can't import "foo" from the base class.
        // But ok for "extern foo" and "foo" to both come from base (so must check before
        // insert)
        std::unordered_ਸੈੱਟ<std::string> haveExterns;
        ਲਈ (IdNameMap::const_iterator it = srcp->m_idNameMap.begin();
             it != srcp->m_idNameMap.ਸਿਰਾ(); ++it) {
            ਜੇ (m_idNameMap.ਗਿਣਤੀ_2("extern " + it->ਪਹਿਲਾ)) haveExterns.emplace(it->ਪਹਿਲਾ);
        }
        ਲਈ (IdNameMap::const_iterator it = srcp->m_idNameMap.begin();
             it != srcp->m_idNameMap.ਸਿਰਾ(); ++it) {
            ਜੇ (!haveExterns.ਗਿਣਤੀ_2(it->ਪਹਿਲਾ))
                import_One_ਚਿੰਨ੍ਹ(graphp, it->ਪਹਿਲਾ, it->ਦੂਜਾ, false);
        }
    }
    ਖਾਲੀ importFromPackage(VSym_ਗ੍ਰਾਫ* graphp, ਅਚਲ VSymEnt* srcp, ਅਚਲ string& id_or_ਤਾਰਾ) {
        // Import tokens from source symbol table into this symbol table
        ਜੇ (id_or_ਤਾਰਾ != "*") {
            ਅਚਲ ਸਵੈਚਾਲਿਤ it = srcp->m_idNameMap.ਲੱਭੋ(id_or_ਤਾਰਾ);
            ਜੇ (it != srcp->m_idNameMap.ਸਿਰਾ()) {
                import_One_ਚਿੰਨ੍ਹ(graphp, it->ਪਹਿਲਾ, it->ਦੂਜਾ, true);
            }
        } ਨਹੀਂਤਾਂ {
            ਲਈ (IdNameMap::const_iterator it = srcp->m_idNameMap.begin();
                 it != srcp->m_idNameMap.ਸਿਰਾ(); ++it) {
                import_One_ਚਿੰਨ੍ਹ(graphp, it->ਪਹਿਲਾ, it->ਦੂਜਾ, true);
            }
        }
    }
    ਖਾਲੀ exportFromPackage(VSym_ਗ੍ਰਾਫ* graphp, ਅਚਲ VSymEnt* srcp, ਅਚਲ string& id_or_ਤਾਰਾ) {
        // Export tokens from source symbol table into this symbol table
        ਜੇ (id_or_ਤਾਰਾ != "*") {
            ਅਚਲ ਸਵੈਚਾਲਿਤ it = vlstd::as_const(srcp->m_idNameMap).ਲੱਭੋ(id_or_ਤਾਰਾ);
            ਜੇ (it != srcp->m_idNameMap.ਸਿਰਾ()) export_One_ਚਿੰਨ੍ਹ(graphp, it->ਪਹਿਲਾ, it->ਦੂਜਾ);
        } ਨਹੀਂਤਾਂ {
            ਲਈ (IdNameMap::const_iterator it = srcp->m_idNameMap.begin();
                 it != srcp->m_idNameMap.ਸਿਰਾ(); ++it) {
                export_One_ਚਿੰਨ੍ਹ(graphp, it->ਪਹਿਲਾ, it->ਦੂਜਾ);
            }
        }
    }
    ਖਾਲੀ export_ਤਾਰਾ_ਤਾਰਾ(VSym_ਗ੍ਰਾਫ* /*graphp*/) {
        // Export *:*: Export all tokens from imported packages
        ਲਈ (IdNameMap::const_iterator it = m_idNameMap.begin(); it != m_idNameMap.ਸਿਰਾ(); ++it) {
            VSymEnt* ਅਚਲ symp = it->ਦੂਜਾ;
            ਜੇ (!symp->exported()) symp->exported(true);
        }
    }
    ਖਾਲੀ importFromIface(VSym_ਗ੍ਰਾਫ* graphp, ਅਚਲ VSymEnt* srcp, bool onlyUnmodportable = false) {
        // Import interface tokens from source symbol table into this symbol table, recursively
        UINFO(9, "     importIf  se" << cvt_ਮੰਜ਼ਿਲ_Hex(this) << " from se" << cvt_ਮੰਜ਼ਿਲ_Hex(srcp));
        ਲਈ (IdNameMap::const_iterator it = srcp->m_idNameMap.begin();
             it != srcp->m_idNameMap.ਸਿਰਾ(); ++it) {
            ਅਚਲ string& name = it->ਪਹਿਲਾ;
            VSymEnt* ਅਚਲ subSrcp = it->ਦੂਜਾ;
            ਅਚਲ AstVar* ਅਚਲ varp = VN_CAST(subSrcp->nodep(), Var);
            ਜੇ (!onlyUnmodportable || (varp && varp->isParam())) {
                VSymEnt* ਅਚਲ subSymp = new VSymEnt{graphp, subSrcp};
                reinsert(name, subSymp);
                // And recurse to create children
                subSymp->importFromIface(graphp, subSrcp);
            }
        }
    }
    string cellErrorScopes(ਅਚਲ Ast_ਗੰਢ* lookp, string prettyName = "") {
        ਜੇ (prettyName == "") prettyName = lookp->prettyName();
        string scopes;
        ਲਈ (IdNameMap::iterator it = m_idNameMap.begin(); it != m_idNameMap.ਸਿਰਾ(); ++it) {
            ਅਚਲ Ast_ਗੰਢ* ਅਚਲ itemp = it->ਦੂਜਾ->nodep();
            ਜੇ (VN_IS(itemp, Cell) || (VN_IS(itemp, Module) && VN_AS(itemp, Module)->is_ਸਿਖਰ())) {
                ਜੇ (scopes != "") scopes += ", ";
                scopes += Ast_ਗੰਢ::prettyName(it->ਪਹਿਲਾ);
            }
        }
        ਜੇ (scopes == "") scopes = "<no instances found>";
        ਜੇ (debug()) dumpSelf(std::cerr, "       KnownScope: ", 1);
        ਵਾਪਸ V3Error::warn_ਹੋਰ() + "... Known scopes under '" + prettyName + "': " + scopes
               + '\n';
    }
};

//######################################################################
// Symbol tables

class VSym_ਗ੍ਰਾਫ final {
    // Collection of symbol tables
    // TYPES
    using Sym_ਢੇਰ = std::ਸਦਿਸ਼<VSymEnt*>;

    // MEMBERS
    VSymEnt* m_symRootp;  // Root symbol table
    Sym_ਢੇਰ m_symsp;  // All symbol tables, to cleanup

    // CONSTRUCTORS
    VL_UNCOPYABLE(VSym_ਗ੍ਰਾਫ);

    VL_DEFINE_DEBUG_FUNCTIONS;

ਜਨਤਕ:
    explicit VSym_ਗ੍ਰਾਫ(AstNetlist* nodep) { m_symRootp = new VSymEnt{this, nodep}; }
    ~VSym_ਗ੍ਰਾਫ() {
        ਲਈ (ਅਚਲ VSymEnt* entp : m_symsp) delete entp;
    }

    // METHODS
    VSymEnt* rootp() ਅਚਲ { ਵਾਪਸ m_symRootp; }
    // Debug
    ਖਾਲੀ dumpSelf(std::ostream& os, ਅਚਲ string& indent = "") {
        VSymConstMap ਪੂਰਾ_Syms;
        os << "SymEnt Dump:\n";
        m_symRootp->dumpIterate(os, ਪੂਰਾ_Syms, indent, 9999, "$root");
        bool ਪਹਿਲਾ = true;
        ਲਈ (Sym_ਢੇਰ::iterator it = m_symsp.begin(); it != m_symsp.ਸਿਰਾ(); ++it) {
            ਜੇ (ਪੂਰਾ_Syms.ਲੱਭੋ(*it) == ਪੂਰਾ_Syms.ਸਿਰਾ()) {
                ਜੇ (ਪਹਿਲਾ) {
                    ਪਹਿਲਾ = false;
                    os << "%%Warning: SymEnt Orphans:\n";
                }
                (*it)->dumpIterate(os, ਪੂਰਾ_Syms, indent, 9999, "Orphan");
            }
        }
    }
    ਖਾਲੀ dumpFilePrefixed(ਅਚਲ string& nameComment) {
        ਜੇ (dump_ਰੁੱਖ_Level()) {
            ਅਚਲ string filename = v3Global.debugFilename(nameComment) + ".txt";
            UINFO(2, "Dumping " << filename);
            ਅਚਲ std::unique_ptr<std::ofstream> logp{V3File::new_ofstream(filename)};
            ਜੇ (logp->fail()) v3fatal("Can't write file: " << filename);
            dumpSelf(*logp, "");
        }
    }

ਸੁਰੱਖਿਅਤ:
    ਮਿੱਤਰ class VSymEnt;
    ਖਾਲੀ ਧੱਕੋ_New_Ent(VSymEnt* entp) { m_symsp.ਧੱਕੋ_ਪਿੱਠ(entp); }
};

//######################################################################

ਸਤਰਅੰਦਰ VSymEnt::VSymEnt(VSym_ਗ੍ਰਾਫ* graphp, Ast_ਗੰਢ* nodep)
    : m_nodep{nodep} {
    // No argument to set fallbackp, as generally it's wrong to set it in the new call,
    // Instead it needs to be set on a "findOrNew()" return, as it may have been new'ed
    // by an earlier search insertion.
    graphp->ਧੱਕੋ_New_Ent(this);
}

ਸਤਰਅੰਦਰ VSymEnt::VSymEnt(VSym_ਗ੍ਰਾਫ* graphp, ਅਚਲ VSymEnt* symp)
    : m_nodep{symp->m_nodep}
    , m_fallbackp{symp->m_fallbackp}
    , m_parentp{symp->m_parentp}
    , m_classOrPackagep{symp->m_classOrPackagep}
    , m_exported{symp->m_exported}
    , m_imported{symp->m_imported} {
    graphp->ਧੱਕੋ_New_Ent(this);
}

#ਜੇ_ਅੰਤ  // guard
