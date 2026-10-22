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

#که_نه_تعریف_شوی VERILATOR_V3SYMTABLE_H_
#تعریف_کړه VERILATOR_V3SYMTABLE_H_

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

class VSym_ګراف;
class VSymEnt;

//######################################################################
// Symbol table

using VSymConstMap = std::unordered_سېټ<ثابت VSymEnt*>;

class VSymEnt final {
    // Symbol table that can have a "superior" table for resolving upper references
    // MEMBERS
    using IdNameMap = std::multimap<std::string, VSymEnt*>;
    IdNameMap m_idNameMap;  // Hash of variables by name
    std::unordered_سېټ<std::string> m_idNameSimilarMap;  // Variables by name with same case
    Ast_غوټه* m_nodep;  // Node that entry belongs to
    VSymEnt* m_fallbackp = nullptr;  // Table "above" this in name scope, for fallback resolution
    VSymEnt* m_parentp = nullptr;  // Table that created this
    Ast_غوټه_Module* m_classOrPackagep = nullptr;  // Package node is in (for V3LinkDot, unused here)
    string m_sym_مختاړی;  // String to prefix symbols with (for V3LinkDot, unused here)
    bool m_exported = true;  // Allow importing
    bool m_imported = false;  // Was imported
#که_تعریف_شوی VL_DEBUG
    VL_DEFINE_DEBUG_FUNCTIONS;
#وإلا
    ساكن constexpr صحيح debug() { أرجع 0; }  // NOT runtime, too hot of a function
#د_که_پای
عام:
    using const_iterator = IdNameMap::const_iterator;
    const_iterator begin() ثابت { أرجع m_idNameMap.begin(); }
    const_iterator پای_ټکی() ثابت { أرجع m_idNameMap.پای_ټکی(); }

    فراغ dumpIterate(std::ostream& os, VSymConstMap& بشپړ_Symsr, ثابت string& indent,
                     صحيح عدد_پوړونه, ثابت string& پلټنه_Name) ثابت {
        os << indent << "+ " << std::کیڼ_2 << std::setw(30) << ("'"s + پلټنه_Name + "'"s)
           << std::setw(0) << std::ښی_2;
        os << "  se" << cvt_منزل_Hex(this) << std::setw(0);
        os << "  fallb=se" << cvt_منزل_Hex(m_fallbackp);
        إذا (m_imported || m_exported) {
            os << "  ";
            إذا (m_imported) os << "[I]";
            إذا (m_exported) os << "[E]";
        }
        إذا (m_sym_مختاړی != "") os << "  symPrefix=" << m_sym_مختاړی;
        إذا (nodep()) os << "  n=" << nodep();
        os << '\n';
        إذا (VL_UNCOVERABLE(!بشپړ_Symsr.ورننباسه(this).دوهم)) {
            os << indent << "| ^ duplicate, so no children printed\n";  // LCOV_EXCL_LINE
        } وإلا {
            لكل (خپلکاری it = m_idNameMap.begin(); it != m_idNameMap.پای_ټکی(); ++it) {
                إذا (عدد_پوړونه >= 1) {
                    it->دوهم->dumpIterate(os, بشپړ_Symsr, indent + "| ", عدد_پوړونه - 1,
                                            it->لومړی);
                }
            }
        }
    }
    فراغ dumpSelf(std::ostream& os, ثابت string& indent = "", صحيح عدد_پوړونه = 1) ثابت {
        VSymConstMap بشپړ_Syms;
        dumpIterate(os, بشپړ_Syms, indent, عدد_پوړونه, "TOP");
    }

    // METHODS
    VSymEnt(VSym_ګراف* graphp, ثابت VSymEnt* symp);  //لاندې
    VSymEnt(VSym_ګراف* graphp, Ast_غوټه* nodep);  //لاندې
    ~VSymEnt() {
        // Change links so we coredump if used
#که_تعریف_شوی VL_DEBUG
        m_nodep = reinterpret_cast<Ast_غوټه*>(1);
        m_fallbackp = reinterpret_cast<VSymEnt*>(1);
        m_parentp = reinterpret_cast<VSymEnt*>(1);
        m_classOrPackagep = reinterpret_cast<Ast_غوټه_Module*>(1);
#د_که_پای
    }
#إذا (defined(VL_DEBUG) || defined(VL_ALLOC_RANDOM_CHECKS)) && !defined(VL_LEAK_CHECKS)
    // For testing, leak so above destructor 1 assignments work
    فراغ* چلوونکی new(اندازه_t اندازه) { أرجع std::حافظه_ورکړه(اندازه); }
    فراغ چلوونکی delete(فراغ* objp, اندازه_t اندازه) {}
#د_که_پای
    فراغ fallbackp(VSymEnt* entp) { m_fallbackp = entp; }
    VSymEnt* fallbackp() ثابت { أرجع m_fallbackp; }
    فراغ parentp(VSymEnt* entp) { m_parentp = entp; }
    VSymEnt* parentp() ثابت { أرجع m_parentp; }
    فراغ classOrPackagep(Ast_غوټه_Module* entp) { m_classOrPackagep = entp; }
    Ast_غوټه_Module* classOrPackagep() ثابت { أرجع m_classOrPackagep; }
    Ast_غوټه* nodep() ثابت { أرجع m_nodep; }
    string sym_مختاړی() ثابت { أرجع m_sym_مختاړی; }
    فراغ sym_مختاړی(ثابت string& name) { m_sym_مختاړی = name; }
    bool exported() ثابت { أرجع m_exported; }
    فراغ exported(bool flag) { m_exported = flag; }
    bool imported() ثابت { أرجع m_imported; }
    فراغ imported(bool flag) { m_imported = flag; }
    VSymEnt* ورننباسه(ثابت string& name, VSymEnt* entp) {
        UINFO(9, "     SymInsert se" << cvt_منزل_Hex(this) << " '" << name << "' se" << cvt_منزل_Hex(entp)
                                     << "  " << entp->nodep());
        إذا (name != "" && m_idNameMap.پیدا_کړه(name) != m_idNameMap.پای_ټکی()) {
            // If didn't already report warning
            إذا (!V3Error::error_شمېر()) {  // LCOV_EXCL_START
                إذا (debug() >= 9 || V3Error::debugDefault())
                    dumpSelf(std::cout, "- err-dump: ", 1);
                entp->nodep()->v3fatalSrc("Inserting two symbols with same name: " << name);
            }  // LCOV_EXCL_STOP
        } وإلا {
            m_idNameMap.emplace(name, entp);
        }
        إذا (!name.تشه() && name.پیدا_کړه("__DOT__") == std::string::npos  // ignore hierarchical
            && checkSimilarname(entp->nodep())) {
            string lc = name;
            لكل (خپلکاری& c : lc) c = (حرف)tolower(c);
            إذا (m_idNameSimilarMap.پیدا_کړه(lc) == m_idNameSimilarMap.پای_ټکی())
                m_idNameSimilarMap.ورننباسه(lc);
        }

        أرجع entp;
    }
    فراغ reinsert(ثابت string& name, VSymEnt* entp) {
        ثابت خپلکاری it = m_idNameMap.پیدا_کړه(name);
        إذا (name != "" && it != m_idNameMap.پای_ټکی()) {
            UINFO(9, "     SymReinsert se" << cvt_منزل_Hex(this) << " '" << name << "' se"
                                           << cvt_منزل_Hex(entp) << "  " << entp->nodep());
            it->دوهم = entp;  // Replace
        } وإلا {
            ورننباسه(name, entp);
        }
    }
    VSymEnt* پیدا_کړه_Id_Flat(ثابت string& name) ثابت {
        // Find identifier without looking upward through symbol hierarchy
        // First, scan this begin/end block or module for the name
        ثابت خپلکاری it = m_idNameMap.پیدا_کړه(name);
        UINFO(9, "     SymFind   se"
                     << cvt_منزل_Hex(this) << " '" << name << "' -> "
                     << (it == m_idNameMap.پای_ټکی() ? "NONE"
                                                 : "se" + cvt_منزل_Hex(it->دوهم)
                                                       + " n=" + cvt_منزل_Hex(it->دوهم->nodep())));
        إذا (it != m_idNameMap.پای_ټکی()) أرجع it->دوهم;
        أرجع nullptr;
    }
    ساكن bool checkSimilarname(ثابت Ast_غوټه* nodep) {
        // Only declarations that can contribute a name to the netlist handed
        // to a backend tool.
        إذا (ثابت AstVar* ثابت varp = VN_CAST(nodep, Var)) {
            أرجع !varp->isParam() && !varp->isGenVar();
        }
        أرجع VN_IS(nodep, Cell)  //
               || VN_IS(nodep, غوټه_بلاک)  //
               || VN_IS(nodep, Gen_بلاک)  //
               || VN_IS(nodep, Func)  // Note: not AstNodeFTask (Property/Sequence)
               || VN_IS(nodep, Task);
    }
    VSymEnt* پیدا_کړه_Similar_Id_Flat(ثابت string& name) ثابت {
        // Find identifier without looking upward through symbol hierarchy
        // Were looking for symbols that are the same when compared without
        // caring about case, but that are not the same name
        إذا (name.پیدا_کړه("__DOT__") != std::string::npos)  // ignore hierarchical equivalents
            أرجع nullptr;
        string s = name;
        لكل (خپلکاری& c : s) c = (حرف)tolower(c);
        إذا (m_idNameSimilarMap.پیدا_کړه(s) == m_idNameSimilarMap.پای_ټکی()) أرجع nullptr;
        لكل (خپلکاری it = m_idNameMap.begin(); it != m_idNameMap.پای_ټکی(); ++it) {
            // Only report a declaration that is itself checked, otherwise we
            // might point at e.g. a localparam that merely sorts first
            إذا (!checkSimilarname(it->دوهم->nodep())) استمر;
            string t = it->لومړی;
            لكل (خپلکاری& c : t) c = (حرف)tolower(c);
            إذا (t == s && name != it->لومړی) { أرجع it->دوهم; }
        }
        أرجع nullptr;
    }
    VSymEnt* پیدا_کړه_Id_Fallback(ثابت string& name) ثابت {
        // Find identifier looking upward through symbol hierarchy
        // First, scan this begin/end block or module for the name
        إذا (VSymEnt* ثابت entp = پیدا_کړه_Id_Flat(name)) أرجع entp;
        // Then scan the upper begin/end block or module for the name
        إذا (m_fallbackp) أرجع m_fallbackp->پیدا_کړه_Id_Fallback(name);
        أرجع nullptr;
    }
    فراغ candidateIdFlat(VSpellCheck* spellerp, ثابت VNodeMatcher* matcherp,
                         bool includeType) ثابت {
        // Suggest alternative symbol candidates without looking upward through symbol
        // hierarchy
        لكل (IdNameMap::const_iterator it = m_idNameMap.begin(); it != m_idNameMap.پای_ټکی(); ++it) {
            ثابت Ast_غوټه* ثابت itemp = it->دوهم->nodep();
            إذا (itemp && (!matcherp || matcherp->غوټه_سمون(itemp))) {
                إذا (includeType) {
                    spellerp->ټېل_Candidate(itemp->prettyName(), itemp->prettyTypeName());
                } وإلا {
                    spellerp->ټېل_Candidate(itemp->prettyName());
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
    فراغ import_One_نښه(VSym_ګراف* graphp, ثابت string& name, ثابت VSymEnt* srcp,
                         bool honorExport) {
        إذا ((!honorExport || srcp->exported())
            && !پیدا_کړه_Id_Flat(name)) {  // Don't insert over existing entry
            VSymEnt* ثابت symp = new VSymEnt{graphp, srcp};
            symp->exported(false);  // Can't reimport an import without an export
            symp->imported(true);
            reinsert(name, symp);
        }
    }
    فراغ export_One_نښه(VSym_ګراف* /*graphp*/, ثابت string& name, ثابت VSymEnt* srcp) ثابت {
        إذا (srcp->exported()) {
            إذا (VSymEnt* ثابت symp = پیدا_کړه_Id_Flat(name)) {  // Should already exist in current table
                إذا (!symp->exported()) symp->exported(true);
            }
        }
    }

عام:
    فراغ importFromClass(VSym_ګراف* graphp, ثابت VSymEnt* srcp) {
        // Import tokens from source symbol table into this symbol table
        // Used for classes in early parsing only to handle "extends"

        // If an "extern foo" exists, then we can't import "foo" from the base class.
        // But ok for "extern foo" and "foo" to both come from base (so must check before
        // insert)
        std::unordered_سېټ<std::string> haveExterns;
        لكل (IdNameMap::const_iterator it = srcp->m_idNameMap.begin();
             it != srcp->m_idNameMap.پای_ټکی(); ++it) {
            إذا (m_idNameMap.شمېر("extern " + it->لومړی)) haveExterns.emplace(it->لومړی);
        }
        لكل (IdNameMap::const_iterator it = srcp->m_idNameMap.begin();
             it != srcp->m_idNameMap.پای_ټکی(); ++it) {
            إذا (!haveExterns.شمېر(it->لومړی))
                import_One_نښه(graphp, it->لومړی, it->دوهم, false);
        }
    }
    فراغ importFromPackage(VSym_ګراف* graphp, ثابت VSymEnt* srcp, ثابت string& id_or_ستوری) {
        // Import tokens from source symbol table into this symbol table
        إذا (id_or_ستوری != "*") {
            ثابت خپلکاری it = srcp->m_idNameMap.پیدا_کړه(id_or_ستوری);
            إذا (it != srcp->m_idNameMap.پای_ټکی()) {
                import_One_نښه(graphp, it->لومړی, it->دوهم, true);
            }
        } وإلا {
            لكل (IdNameMap::const_iterator it = srcp->m_idNameMap.begin();
                 it != srcp->m_idNameMap.پای_ټکی(); ++it) {
                import_One_نښه(graphp, it->لومړی, it->دوهم, true);
            }
        }
    }
    فراغ exportFromPackage(VSym_ګراف* graphp, ثابت VSymEnt* srcp, ثابت string& id_or_ستوری) {
        // Export tokens from source symbol table into this symbol table
        إذا (id_or_ستوری != "*") {
            ثابت خپلکاری it = vlstd::as_const(srcp->m_idNameMap).پیدا_کړه(id_or_ستوری);
            إذا (it != srcp->m_idNameMap.پای_ټکی()) export_One_نښه(graphp, it->لومړی, it->دوهم);
        } وإلا {
            لكل (IdNameMap::const_iterator it = srcp->m_idNameMap.begin();
                 it != srcp->m_idNameMap.پای_ټکی(); ++it) {
                export_One_نښه(graphp, it->لومړی, it->دوهم);
            }
        }
    }
    فراغ export_ستوری_ستوری(VSym_ګراف* /*graphp*/) {
        // Export *:*: Export all tokens from imported packages
        لكل (IdNameMap::const_iterator it = m_idNameMap.begin(); it != m_idNameMap.پای_ټکی(); ++it) {
            VSymEnt* ثابت symp = it->دوهم;
            إذا (!symp->exported()) symp->exported(true);
        }
    }
    فراغ importFromIface(VSym_ګراف* graphp, ثابت VSymEnt* srcp, bool onlyUnmodportable = false) {
        // Import interface tokens from source symbol table into this symbol table, recursively
        UINFO(9, "     importIf  se" << cvt_منزل_Hex(this) << " from se" << cvt_منزل_Hex(srcp));
        لكل (IdNameMap::const_iterator it = srcp->m_idNameMap.begin();
             it != srcp->m_idNameMap.پای_ټکی(); ++it) {
            ثابت string& name = it->لومړی;
            VSymEnt* ثابت subSrcp = it->دوهم;
            ثابت AstVar* ثابت varp = VN_CAST(subSrcp->nodep(), Var);
            إذا (!onlyUnmodportable || (varp && varp->isParam())) {
                VSymEnt* ثابت subSymp = new VSymEnt{graphp, subSrcp};
                reinsert(name, subSymp);
                // And recurse to create children
                subSymp->importFromIface(graphp, subSrcp);
            }
        }
    }
    string cellErrorScopes(ثابت Ast_غوټه* lookp, string prettyName = "") {
        إذا (prettyName == "") prettyName = lookp->prettyName();
        string scopes;
        لكل (IdNameMap::iterator it = m_idNameMap.begin(); it != m_idNameMap.پای_ټکی(); ++it) {
            ثابت Ast_غوټه* ثابت itemp = it->دوهم->nodep();
            إذا (VN_IS(itemp, Cell) || (VN_IS(itemp, Module) && VN_AS(itemp, Module)->is_سر())) {
                إذا (scopes != "") scopes += ", ";
                scopes += Ast_غوټه::prettyName(it->لومړی);
            }
        }
        إذا (scopes == "") scopes = "<no instances found>";
        إذا (debug()) dumpSelf(std::cerr, "       KnownScope: ", 1);
        أرجع V3Error::warn_نور() + "... Known scopes under '" + prettyName + "': " + scopes
               + '\n';
    }
};

//######################################################################
// Symbol tables

class VSym_ګراف final {
    // Collection of symbol tables
    // TYPES
    using Sym_ډېری = std::ویکټور<VSymEnt*>;

    // MEMBERS
    VSymEnt* m_symRootp;  // Root symbol table
    Sym_ډېری m_symsp;  // All symbol tables, to cleanup

    // CONSTRUCTORS
    VL_UNCOPYABLE(VSym_ګراف);

    VL_DEFINE_DEBUG_FUNCTIONS;

عام:
    explicit VSym_ګراف(AstNetlist* nodep) { m_symRootp = new VSymEnt{this, nodep}; }
    ~VSym_ګراف() {
        لكل (ثابت VSymEnt* entp : m_symsp) delete entp;
    }

    // METHODS
    VSymEnt* rootp() ثابت { أرجع m_symRootp; }
    // Debug
    فراغ dumpSelf(std::ostream& os, ثابت string& indent = "") {
        VSymConstMap بشپړ_Syms;
        os << "SymEnt Dump:\n";
        m_symRootp->dumpIterate(os, بشپړ_Syms, indent, 9999, "$root");
        bool لومړی = true;
        لكل (Sym_ډېری::iterator it = m_symsp.begin(); it != m_symsp.پای_ټکی(); ++it) {
            إذا (بشپړ_Syms.پیدا_کړه(*it) == بشپړ_Syms.پای_ټکی()) {
                إذا (لومړی) {
                    لومړی = false;
                    os << "%%Warning: SymEnt Orphans:\n";
                }
                (*it)->dumpIterate(os, بشپړ_Syms, indent, 9999, "Orphan");
            }
        }
    }
    فراغ dumpFilePrefixed(ثابت string& nameComment) {
        إذا (dump_ونه_Level()) {
            ثابت string filename = v3Global.debugFilename(nameComment) + ".txt";
            UINFO(2, "Dumping " << filename);
            ثابت std::unique_ptr<std::ofstream> logp{V3File::new_ofstream(filename)};
            إذا (logp->fail()) v3fatal("Can't write file: " << filename);
            dumpSelf(*logp, "");
        }
    }

محمي:
    ملګری class VSymEnt;
    فراغ ټېل_New_Ent(VSymEnt* entp) { m_symsp.ټېل_شا(entp); }
};

//######################################################################

مضمن VSymEnt::VSymEnt(VSym_ګراف* graphp, Ast_غوټه* nodep)
    : m_nodep{nodep} {
    // No argument to set fallbackp, as generally it's wrong to set it in the new call,
    // Instead it needs to be set on a "findOrNew()" return, as it may have been new'ed
    // by an earlier search insertion.
    graphp->ټېل_New_Ent(this);
}

مضمن VSymEnt::VSymEnt(VSym_ګراف* graphp, ثابت VSymEnt* symp)
    : m_nodep{symp->m_nodep}
    , m_fallbackp{symp->m_fallbackp}
    , m_parentp{symp->m_parentp}
    , m_classOrPackagep{symp->m_classOrPackagep}
    , m_exported{symp->m_exported}
    , m_imported{symp->m_imported} {
    graphp->ټېل_New_Ent(this);
}

#د_که_پای  // guard
