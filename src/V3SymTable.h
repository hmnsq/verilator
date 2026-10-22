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

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ VERILATOR_V3SYMTABLE_H_
#ವ್ಯಾಖ್ಯಾನಿಸು VERILATOR_V3SYMTABLE_H_

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "V3Ast.h"
#ಅನ್ತರ್ಭಾವ "V3File.h"
#ಅನ್ತರ್ಭಾವ "V3Global.h"
#ಅನ್ತರ್ಭಾವ "V3String.h"

#ಅನ್ತರ್ಭಾವ <cstdarg>
#ಅನ್ತರ್ಭಾವ <iomanip>
#ಅನ್ತರ್ಭಾವ <map>
#ಅನ್ತರ್ಭಾವ <memory>
#ಅನ್ತರ್ಭಾವ <unordered_set>
#ಅನ್ತರ್ಭಾವ <vector>

class VSym_ನಕ್ಷೆ;
class VSymEnt;

//######################################################################
// Symbol table

using VSymConstMap = std::unordered_ಗಣ<ಸ್ಥಿರಾಂಕ VSymEnt*>;

class VSymEnt final {
    // Symbol table that can have a "superior" table for resolving upper references
    // MEMBERS
    using IdNameMap = std::multimap<std::string, VSymEnt*>;
    IdNameMap m_idNameMap;  // Hash of variables by name
    std::unordered_ಗಣ<std::string> m_idNameSimilarMap;  // Variables by name with same case
    Ast_ಗಂಟು* m_nodep;  // Node that entry belongs to
    VSymEnt* m_fallbackp = nullptr;  // Table "above" this in name scope, for fallback resolution
    VSymEnt* m_parentp = nullptr;  // Table that created this
    Ast_ಗಂಟು_Module* m_classOrPackagep = nullptr;  // Package node is in (for V3LinkDot, unused here)
    string m_sym_ಪೂರ್ವಪ್ರತ್ಯಯ;  // String to prefix symbols with (for V3LinkDot, unused here)
    bool m_exported = true;  // Allow importing
    bool m_imported = false;  // Was imported
#ವ್ಯಾಖ್ಯಾನಿತವಾದರೆ VL_DEBUG
    VL_DEFINE_DEBUG_FUNCTIONS;
#ಇಲ್ಲದಿದ್ದರೆ
    ಸ್ಥಿರ constexpr ಪೂರ್ಣಾಂಕ debug() { ಹಿಂತಿರುಗಿ 0; }  // NOT runtime, too hot of a function
#ಆದರೆ_ಅಂತ್ಯ
ಸಾರ್ವಜನಿಕ:
    using const_iterator = IdNameMap::const_iterator;
    const_iterator begin() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_idNameMap.begin(); }
    const_iterator ಕೊನೆ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_idNameMap.ಕೊನೆ(); }

    ಶೂನ್ಯ dumpIterate(std::ostream& os, VSymConstMap& ಮುಗಿದ_Symsr, ಸ್ಥಿರಾಂಕ string& indent,
                     ಪೂರ್ಣಾಂಕ ಅಂಕೆ_ಮಟ್ಟಗಳು, ಸ್ಥಿರಾಂಕ string& ಹುಡುಕು_Name) ಸ್ಥಿರಾಂಕ {
        os << indent << "+ " << std::ಎಡ_2 << std::setw(30) << ("'"s + ಹುಡುಕು_Name + "'"s)
           << std::setw(0) << std::ಬಲ_2;
        os << "  se" << cvt_ಗಮ್ಯ_Hex(this) << std::setw(0);
        os << "  fallb=se" << cvt_ಗಮ್ಯ_Hex(m_fallbackp);
        ಆದರೆ (m_imported || m_exported) {
            os << "  ";
            ಆದರೆ (m_imported) os << "[I]";
            ಆದರೆ (m_exported) os << "[E]";
        }
        ಆದರೆ (m_sym_ಪೂರ್ವಪ್ರತ್ಯಯ != "") os << "  symPrefix=" << m_sym_ಪೂರ್ವಪ್ರತ್ಯಯ;
        ಆದರೆ (nodep()) os << "  n=" << nodep();
        os << '\n';
        ಆದರೆ (VL_UNCOVERABLE(!ಮುಗಿದ_Symsr.ಸೇರಿಸು(this).ಎರಡನೇ)) {
            os << indent << "| ^ duplicate, so no children printed\n";  // LCOV_EXCL_LINE
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಗಾಗಿ (ಸ್ವಯಂಚಾಲಿತ it = m_idNameMap.begin(); it != m_idNameMap.ಕೊನೆ(); ++it) {
                ಆದರೆ (ಅಂಕೆ_ಮಟ್ಟಗಳು >= 1) {
                    it->ಎರಡನೇ->dumpIterate(os, ಮುಗಿದ_Symsr, indent + "| ", ಅಂಕೆ_ಮಟ್ಟಗಳು - 1,
                                            it->ಮೊದಲ);
                }
            }
        }
    }
    ಶೂನ್ಯ dumpSelf(std::ostream& os, ಸ್ಥಿರಾಂಕ string& indent = "", ಪೂರ್ಣಾಂಕ ಅಂಕೆ_ಮಟ್ಟಗಳು = 1) ಸ್ಥಿರಾಂಕ {
        VSymConstMap ಮುಗಿದ_Syms;
        dumpIterate(os, ಮುಗಿದ_Syms, indent, ಅಂಕೆ_ಮಟ್ಟಗಳು, "TOP");
    }

    // METHODS
    VSymEnt(VSym_ನಕ್ಷೆ* graphp, ಸ್ಥಿರಾಂಕ VSymEnt* symp);  //ಕೆಳಗೆ
    VSymEnt(VSym_ನಕ್ಷೆ* graphp, Ast_ಗಂಟು* nodep);  //ಕೆಳಗೆ
    ~VSymEnt() {
        // Change links so we coredump if used
#ವ್ಯಾಖ್ಯಾನಿತವಾದರೆ VL_DEBUG
        m_nodep = reinterpret_cast<Ast_ಗಂಟು*>(1);
        m_fallbackp = reinterpret_cast<VSymEnt*>(1);
        m_parentp = reinterpret_cast<VSymEnt*>(1);
        m_classOrPackagep = reinterpret_cast<Ast_ಗಂಟು_Module*>(1);
#ಆದರೆ_ಅಂತ್ಯ
    }
#ಆದರೆ (defined(VL_DEBUG) || defined(VL_ALLOC_RANDOM_CHECKS)) && !defined(VL_LEAK_CHECKS)
    // For testing, leak so above destructor 1 assignments work
    ಶೂನ್ಯ* ಕ್ರಿಯಾಕಾರಕ new(ಗಾತ್ರ_t ಗಾತ್ರ_2) { ಹಿಂತಿರುಗಿ std::ಸ್ಮರಣೆ_ಕೊಡು(ಗಾತ್ರ_2); }
    ಶೂನ್ಯ ಕ್ರಿಯಾಕಾರಕ delete(ಶೂನ್ಯ* objp, ಗಾತ್ರ_t ಗಾತ್ರ_2) {}
#ಆದರೆ_ಅಂತ್ಯ
    ಶೂನ್ಯ fallbackp(VSymEnt* entp) { m_fallbackp = entp; }
    VSymEnt* fallbackp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_fallbackp; }
    ಶೂನ್ಯ parentp(VSymEnt* entp) { m_parentp = entp; }
    VSymEnt* parentp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_parentp; }
    ಶೂನ್ಯ classOrPackagep(Ast_ಗಂಟು_Module* entp) { m_classOrPackagep = entp; }
    Ast_ಗಂಟು_Module* classOrPackagep() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_classOrPackagep; }
    Ast_ಗಂಟು* nodep() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_nodep; }
    string sym_ಪೂರ್ವಪ್ರತ್ಯಯ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_sym_ಪೂರ್ವಪ್ರತ್ಯಯ; }
    ಶೂನ್ಯ sym_ಪೂರ್ವಪ್ರತ್ಯಯ(ಸ್ಥಿರಾಂಕ string& name) { m_sym_ಪೂರ್ವಪ್ರತ್ಯಯ = name; }
    bool exported() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_exported; }
    ಶೂನ್ಯ exported(bool flag) { m_exported = flag; }
    bool imported() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_imported; }
    ಶೂನ್ಯ imported(bool flag) { m_imported = flag; }
    VSymEnt* ಸೇರಿಸು(ಸ್ಥಿರಾಂಕ string& name, VSymEnt* entp) {
        UINFO(9, "     SymInsert se" << cvt_ಗಮ್ಯ_Hex(this) << " '" << name << "' se" << cvt_ಗಮ್ಯ_Hex(entp)
                                     << "  " << entp->nodep());
        ಆದರೆ (name != "" && m_idNameMap.ಕಂಡುಹಿಡಿ(name) != m_idNameMap.ಕೊನೆ()) {
            // If didn't already report warning
            ಆದರೆ (!V3Error::error_ಎಣಿಕೆ()) {  // LCOV_EXCL_START
                ಆದರೆ (debug() >= 9 || V3Error::debugDefault())
                    dumpSelf(std::cout, "- err-dump: ", 1);
                entp->nodep()->v3fatalSrc("Inserting two symbols with same name: " << name);
            }  // LCOV_EXCL_STOP
        } ಇಲ್ಲದಿದ್ದರೆ {
            m_idNameMap.emplace(name, entp);
        }
        ಆದರೆ (!name.ಖಾಲಿಗಳು() && name.ಕಂಡುಹಿಡಿ("__DOT__") == std::string::npos  // ignore hierarchical
            && checkSimilarname(entp->nodep())) {
            string lc = name;
            ಗಾಗಿ (ಸ್ವಯಂಚಾಲಿತ& c : lc) c = (ಅಕ್ಷರ)tolower(c);
            ಆದರೆ (m_idNameSimilarMap.ಕಂಡುಹಿಡಿ(lc) == m_idNameSimilarMap.ಕೊನೆ())
                m_idNameSimilarMap.ಸೇರಿಸು(lc);
        }

        ಹಿಂತಿರುಗಿ entp;
    }
    ಶೂನ್ಯ reinsert(ಸ್ಥಿರಾಂಕ string& name, VSymEnt* entp) {
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ it = m_idNameMap.ಕಂಡುಹಿಡಿ(name);
        ಆದರೆ (name != "" && it != m_idNameMap.ಕೊನೆ()) {
            UINFO(9, "     SymReinsert se" << cvt_ಗಮ್ಯ_Hex(this) << " '" << name << "' se"
                                           << cvt_ಗಮ್ಯ_Hex(entp) << "  " << entp->nodep());
            it->ಎರಡನೇ = entp;  // Replace
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಸೇರಿಸು(name, entp);
        }
    }
    VSymEnt* ಕಂಡುಹಿಡಿ_Id_Flat(ಸ್ಥಿರಾಂಕ string& name) ಸ್ಥಿರಾಂಕ {
        // Find identifier without looking upward through symbol hierarchy
        // First, scan this begin/end block or module for the name
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ it = m_idNameMap.ಕಂಡುಹಿಡಿ(name);
        UINFO(9, "     SymFind   se"
                     << cvt_ಗಮ್ಯ_Hex(this) << " '" << name << "' -> "
                     << (it == m_idNameMap.ಕೊನೆ() ? "NONE"
                                                 : "se" + cvt_ಗಮ್ಯ_Hex(it->ಎರಡನೇ)
                                                       + " n=" + cvt_ಗಮ್ಯ_Hex(it->ಎರಡನೇ->nodep())));
        ಆದರೆ (it != m_idNameMap.ಕೊನೆ()) ಹಿಂತಿರುಗಿ it->ಎರಡನೇ;
        ಹಿಂತಿರುಗಿ nullptr;
    }
    ಸ್ಥಿರ bool checkSimilarname(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep) {
        // Only declarations that can contribute a name to the netlist handed
        // to a backend tool.
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ varp = VN_CAST(nodep, Var)) {
            ಹಿಂತಿರುಗಿ !varp->isParam() && !varp->isGenVar();
        }
        ಹಿಂತಿರುಗಿ VN_IS(nodep, Cell)  //
               || VN_IS(nodep, ಗಂಟು_ಖಂಡ)  //
               || VN_IS(nodep, Gen_ಖಂಡ)  //
               || VN_IS(nodep, Func)  // Note: not AstNodeFTask (Property/Sequence)
               || VN_IS(nodep, Task);
    }
    VSymEnt* ಕಂಡುಹಿಡಿ_Similar_Id_Flat(ಸ್ಥಿರಾಂಕ string& name) ಸ್ಥಿರಾಂಕ {
        // Find identifier without looking upward through symbol hierarchy
        // Were looking for symbols that are the same when compared without
        // caring about case, but that are not the same name
        ಆದರೆ (name.ಕಂಡುಹಿಡಿ("__DOT__") != std::string::npos)  // ignore hierarchical equivalents
            ಹಿಂತಿರುಗಿ nullptr;
        string s = name;
        ಗಾಗಿ (ಸ್ವಯಂಚಾಲಿತ& c : s) c = (ಅಕ್ಷರ)tolower(c);
        ಆದರೆ (m_idNameSimilarMap.ಕಂಡುಹಿಡಿ(s) == m_idNameSimilarMap.ಕೊನೆ()) ಹಿಂತಿರುಗಿ nullptr;
        ಗಾಗಿ (ಸ್ವಯಂಚಾಲಿತ it = m_idNameMap.begin(); it != m_idNameMap.ಕೊನೆ(); ++it) {
            // Only report a declaration that is itself checked, otherwise we
            // might point at e.g. a localparam that merely sorts first
            ಆದರೆ (!checkSimilarname(it->ಎರಡನೇ->nodep())) ಮುಂದುವರಿ;
            string t = it->ಮೊದಲ;
            ಗಾಗಿ (ಸ್ವಯಂಚಾಲಿತ& c : t) c = (ಅಕ್ಷರ)tolower(c);
            ಆದರೆ (t == s && name != it->ಮೊದಲ) { ಹಿಂತಿರುಗಿ it->ಎರಡನೇ; }
        }
        ಹಿಂತಿರುಗಿ nullptr;
    }
    VSymEnt* ಕಂಡುಹಿಡಿ_Id_Fallback(ಸ್ಥಿರಾಂಕ string& name) ಸ್ಥಿರಾಂಕ {
        // Find identifier looking upward through symbol hierarchy
        // First, scan this begin/end block or module for the name
        ಆದರೆ (VSymEnt* ಸ್ಥಿರಾಂಕ entp = ಕಂಡುಹಿಡಿ_Id_Flat(name)) ಹಿಂತಿರುಗಿ entp;
        // Then scan the upper begin/end block or module for the name
        ಆದರೆ (m_fallbackp) ಹಿಂತಿರುಗಿ m_fallbackp->ಕಂಡುಹಿಡಿ_Id_Fallback(name);
        ಹಿಂತಿರುಗಿ nullptr;
    }
    ಶೂನ್ಯ candidateIdFlat(VSpellCheck* spellerp, ಸ್ಥಿರಾಂಕ VNodeMatcher* matcherp,
                         bool includeType) ಸ್ಥಿರಾಂಕ {
        // Suggest alternative symbol candidates without looking upward through symbol
        // hierarchy
        ಗಾಗಿ (IdNameMap::const_iterator it = m_idNameMap.begin(); it != m_idNameMap.ಕೊನೆ(); ++it) {
            ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ itemp = it->ಎರಡನೇ->nodep();
            ಆದರೆ (itemp && (!matcherp || matcherp->ಗಂಟು_ಹೊಂದಿಕೆ(itemp))) {
                ಆದರೆ (includeType) {
                    spellerp->ತಳ್ಳು_Candidate(itemp->prettyName(), itemp->prettyTypeName());
                } ಇಲ್ಲದಿದ್ದರೆ {
                    spellerp->ತಳ್ಳು_Candidate(itemp->prettyName());
                }
            }
        }
    }
    ಶೂನ್ಯ candidateIdFallback(VSpellCheck* spellerp, ಸ್ಥಿರಾಂಕ VNodeMatcher* matcherp) ಸ್ಥಿರಾಂಕ {
        // Suggest alternative symbol candidates with looking upward through symbol hierarchy
        // Note VSpellCheck wants the most important (closest) items pushed first
        candidateIdFlat(spellerp, matcherp, false);
        // Then suggest the upper begin/end block or module
        ಆದರೆ (m_fallbackp) m_fallbackp->candidateIdFallback(spellerp, matcherp);
    }

ಖಾಸಗಿ:
    ಶೂನ್ಯ import_One_ಚಿಹ್ನೆ(VSym_ನಕ್ಷೆ* graphp, ಸ್ಥಿರಾಂಕ string& name, ಸ್ಥಿರಾಂಕ VSymEnt* srcp,
                         bool honorExport) {
        ಆದರೆ ((!honorExport || srcp->exported())
            && !ಕಂಡುಹಿಡಿ_Id_Flat(name)) {  // Don't insert over existing entry
            VSymEnt* ಸ್ಥಿರಾಂಕ symp = new VSymEnt{graphp, srcp};
            symp->exported(false);  // Can't reimport an import without an export
            symp->imported(true);
            reinsert(name, symp);
        }
    }
    ಶೂನ್ಯ export_One_ಚಿಹ್ನೆ(VSym_ನಕ್ಷೆ* /*graphp*/, ಸ್ಥಿರಾಂಕ string& name, ಸ್ಥಿರಾಂಕ VSymEnt* srcp) ಸ್ಥಿರಾಂಕ {
        ಆದರೆ (srcp->exported()) {
            ಆದರೆ (VSymEnt* ಸ್ಥಿರಾಂಕ symp = ಕಂಡುಹಿಡಿ_Id_Flat(name)) {  // Should already exist in current table
                ಆದರೆ (!symp->exported()) symp->exported(true);
            }
        }
    }

ಸಾರ್ವಜನಿಕ:
    ಶೂನ್ಯ importFromClass(VSym_ನಕ್ಷೆ* graphp, ಸ್ಥಿರಾಂಕ VSymEnt* srcp) {
        // Import tokens from source symbol table into this symbol table
        // Used for classes in early parsing only to handle "extends"

        // If an "extern foo" exists, then we can't import "foo" from the base class.
        // But ok for "extern foo" and "foo" to both come from base (so must check before
        // insert)
        std::unordered_ಗಣ<std::string> haveExterns;
        ಗಾಗಿ (IdNameMap::const_iterator it = srcp->m_idNameMap.begin();
             it != srcp->m_idNameMap.ಕೊನೆ(); ++it) {
            ಆದರೆ (m_idNameMap.ಎಣಿಕೆ_2("extern " + it->ಮೊದಲ)) haveExterns.emplace(it->ಮೊದಲ);
        }
        ಗಾಗಿ (IdNameMap::const_iterator it = srcp->m_idNameMap.begin();
             it != srcp->m_idNameMap.ಕೊನೆ(); ++it) {
            ಆದರೆ (!haveExterns.ಎಣಿಕೆ_2(it->ಮೊದಲ))
                import_One_ಚಿಹ್ನೆ(graphp, it->ಮೊದಲ, it->ಎರಡನೇ, false);
        }
    }
    ಶೂನ್ಯ importFromPackage(VSym_ನಕ್ಷೆ* graphp, ಸ್ಥಿರಾಂಕ VSymEnt* srcp, ಸ್ಥಿರಾಂಕ string& id_or_ನಕ್ಷತ್ರ) {
        // Import tokens from source symbol table into this symbol table
        ಆದರೆ (id_or_ನಕ್ಷತ್ರ != "*") {
            ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ it = srcp->m_idNameMap.ಕಂಡುಹಿಡಿ(id_or_ನಕ್ಷತ್ರ);
            ಆದರೆ (it != srcp->m_idNameMap.ಕೊನೆ()) {
                import_One_ಚಿಹ್ನೆ(graphp, it->ಮೊದಲ, it->ಎರಡನೇ, true);
            }
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಗಾಗಿ (IdNameMap::const_iterator it = srcp->m_idNameMap.begin();
                 it != srcp->m_idNameMap.ಕೊನೆ(); ++it) {
                import_One_ಚಿಹ್ನೆ(graphp, it->ಮೊದಲ, it->ಎರಡನೇ, true);
            }
        }
    }
    ಶೂನ್ಯ exportFromPackage(VSym_ನಕ್ಷೆ* graphp, ಸ್ಥಿರಾಂಕ VSymEnt* srcp, ಸ್ಥಿರಾಂಕ string& id_or_ನಕ್ಷತ್ರ) {
        // Export tokens from source symbol table into this symbol table
        ಆದರೆ (id_or_ನಕ್ಷತ್ರ != "*") {
            ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ it = vlstd::as_const(srcp->m_idNameMap).ಕಂಡುಹಿಡಿ(id_or_ನಕ್ಷತ್ರ);
            ಆದರೆ (it != srcp->m_idNameMap.ಕೊನೆ()) export_One_ಚಿಹ್ನೆ(graphp, it->ಮೊದಲ, it->ಎರಡನೇ);
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಗಾಗಿ (IdNameMap::const_iterator it = srcp->m_idNameMap.begin();
                 it != srcp->m_idNameMap.ಕೊನೆ(); ++it) {
                export_One_ಚಿಹ್ನೆ(graphp, it->ಮೊದಲ, it->ಎರಡನೇ);
            }
        }
    }
    ಶೂನ್ಯ export_ನಕ್ಷತ್ರ_ನಕ್ಷತ್ರ(VSym_ನಕ್ಷೆ* /*graphp*/) {
        // Export *:*: Export all tokens from imported packages
        ಗಾಗಿ (IdNameMap::const_iterator it = m_idNameMap.begin(); it != m_idNameMap.ಕೊನೆ(); ++it) {
            VSymEnt* ಸ್ಥಿರಾಂಕ symp = it->ಎರಡನೇ;
            ಆದರೆ (!symp->exported()) symp->exported(true);
        }
    }
    ಶೂನ್ಯ importFromIface(VSym_ನಕ್ಷೆ* graphp, ಸ್ಥಿರಾಂಕ VSymEnt* srcp, bool onlyUnmodportable = false) {
        // Import interface tokens from source symbol table into this symbol table, recursively
        UINFO(9, "     importIf  se" << cvt_ಗಮ್ಯ_Hex(this) << " from se" << cvt_ಗಮ್ಯ_Hex(srcp));
        ಗಾಗಿ (IdNameMap::const_iterator it = srcp->m_idNameMap.begin();
             it != srcp->m_idNameMap.ಕೊನೆ(); ++it) {
            ಸ್ಥಿರಾಂಕ string& name = it->ಮೊದಲ;
            VSymEnt* ಸ್ಥಿರಾಂಕ subSrcp = it->ಎರಡನೇ;
            ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ varp = VN_CAST(subSrcp->nodep(), Var);
            ಆದರೆ (!onlyUnmodportable || (varp && varp->isParam())) {
                VSymEnt* ಸ್ಥಿರಾಂಕ subSymp = new VSymEnt{graphp, subSrcp};
                reinsert(name, subSymp);
                // And recurse to create children
                subSymp->importFromIface(graphp, subSrcp);
            }
        }
    }
    string cellErrorScopes(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* lookp, string prettyName = "") {
        ಆದರೆ (prettyName == "") prettyName = lookp->prettyName();
        string scopes;
        ಗಾಗಿ (IdNameMap::iterator it = m_idNameMap.begin(); it != m_idNameMap.ಕೊನೆ(); ++it) {
            ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ itemp = it->ಎರಡನೇ->nodep();
            ಆದರೆ (VN_IS(itemp, Cell) || (VN_IS(itemp, Module) && VN_AS(itemp, Module)->is_ತುದಿ())) {
                ಆದರೆ (scopes != "") scopes += ", ";
                scopes += Ast_ಗಂಟು::prettyName(it->ಮೊದಲ);
            }
        }
        ಆದರೆ (scopes == "") scopes = "<no instances found>";
        ಆದರೆ (debug()) dumpSelf(std::cerr, "       KnownScope: ", 1);
        ಹಿಂತಿರುಗಿ V3Error::warn_ಇನ್ನಷ್ಟು() + "... Known scopes under '" + prettyName + "': " + scopes
               + '\n';
    }
};

//######################################################################
// Symbol tables

class VSym_ನಕ್ಷೆ final {
    // Collection of symbol tables
    // TYPES
    using Sym_ರಾಶಿ = std::ಸದಿಶ<VSymEnt*>;

    // MEMBERS
    VSymEnt* m_symRootp;  // Root symbol table
    Sym_ರಾಶಿ m_symsp;  // All symbol tables, to cleanup

    // CONSTRUCTORS
    VL_UNCOPYABLE(VSym_ನಕ್ಷೆ);

    VL_DEFINE_DEBUG_FUNCTIONS;

ಸಾರ್ವಜನಿಕ:
    explicit VSym_ನಕ್ಷೆ(AstNetlist* nodep) { m_symRootp = new VSymEnt{this, nodep}; }
    ~VSym_ನಕ್ಷೆ() {
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ VSymEnt* entp : m_symsp) delete entp;
    }

    // METHODS
    VSymEnt* rootp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_symRootp; }
    // Debug
    ಶೂನ್ಯ dumpSelf(std::ostream& os, ಸ್ಥಿರಾಂಕ string& indent = "") {
        VSymConstMap ಮುಗಿದ_Syms;
        os << "SymEnt Dump:\n";
        m_symRootp->dumpIterate(os, ಮುಗಿದ_Syms, indent, 9999, "$root");
        bool ಮೊದಲ = true;
        ಗಾಗಿ (Sym_ರಾಶಿ::iterator it = m_symsp.begin(); it != m_symsp.ಕೊನೆ(); ++it) {
            ಆದರೆ (ಮುಗಿದ_Syms.ಕಂಡುಹಿಡಿ(*it) == ಮುಗಿದ_Syms.ಕೊನೆ()) {
                ಆದರೆ (ಮೊದಲ) {
                    ಮೊದಲ = false;
                    os << "%%Warning: SymEnt Orphans:\n";
                }
                (*it)->dumpIterate(os, ಮುಗಿದ_Syms, indent, 9999, "Orphan");
            }
        }
    }
    ಶೂನ್ಯ dumpFilePrefixed(ಸ್ಥಿರಾಂಕ string& nameComment) {
        ಆದರೆ (dump_ವೃಕ್ಷ_Level()) {
            ಸ್ಥಿರಾಂಕ string filename = v3Global.debugFilename(nameComment) + ".txt";
            UINFO(2, "Dumping " << filename);
            ಸ್ಥಿರಾಂಕ std::unique_ptr<std::ofstream> logp{V3File::new_ofstream(filename)};
            ಆದರೆ (logp->fail()) v3fatal("Can't write file: " << filename);
            dumpSelf(*logp, "");
        }
    }

ಸಂರಕ್ಷಿತ:
    ಮಿತ್ರ class VSymEnt;
    ಶೂನ್ಯ ತಳ್ಳು_New_Ent(VSymEnt* entp) { m_symsp.ತಳ್ಳು_ಹಿಂಭಾಗ(entp); }
};

//######################################################################

ಸಾಲೊಳಗೆ VSymEnt::VSymEnt(VSym_ನಕ್ಷೆ* graphp, Ast_ಗಂಟು* nodep)
    : m_nodep{nodep} {
    // No argument to set fallbackp, as generally it's wrong to set it in the new call,
    // Instead it needs to be set on a "findOrNew()" return, as it may have been new'ed
    // by an earlier search insertion.
    graphp->ತಳ್ಳು_New_Ent(this);
}

ಸಾಲೊಳಗೆ VSymEnt::VSymEnt(VSym_ನಕ್ಷೆ* graphp, ಸ್ಥಿರಾಂಕ VSymEnt* symp)
    : m_nodep{symp->m_nodep}
    , m_fallbackp{symp->m_fallbackp}
    , m_parentp{symp->m_parentp}
    , m_classOrPackagep{symp->m_classOrPackagep}
    , m_exported{symp->m_exported}
    , m_imported{symp->m_imported} {
    graphp->ತಳ್ಳು_New_Ent(this);
}

#ಆದರೆ_ಅಂತ್ಯ  // guard
