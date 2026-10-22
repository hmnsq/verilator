// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Interface typedef capture helper
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

// ARCHITECTURE - Separation of Concerns (do not change without reading):
//
//   Each captured REFDTYPE carries its own capture record (a VIfaceCaptureTag,
//   see AstRefDType::captureTagp): the cell path from its owner module to the
//   interface, the interface's name, and whether the target is a typedef or a
//   parameter type.  Nothing here points into the AST, so a freed reference
//   takes its record with it, and a cloned reference shares it.
//
//   1. CAPTURE (V3LinkDot, primary pass):
//      captureTypedefContext() / addParamType() tag the reference.
//
//   2. PARAMETERIZATION (V3Param):
//      After a module is cloned, and after an interface cell is specialized,
//      V3Param walks that one module and retargets its tagged references by
//      cell path, so widths computed during parameterization use the
//      specialized interface.
//
//   3. TARGET RESOLUTION (finalizeIfaceCapture, after V3Param):
//      Walks every tagged reference in a live module, follows its cell path
//      from its owner module and retargets it by name, then clears all tags.
//

#ಅನ್ತರ್ಭಾವ "V3LinkDotIfaceCapture.h"

#ಅನ್ತರ್ಭಾವ "V3Error.h"
#ಅನ್ತರ್ಭಾವ "V3Global.h"
#ಅನ್ತರ್ಭಾವ "V3Stats.h"
#ಅನ್ತರ್ಭಾವ "V3SymTable.h"

#ಅನ್ತರ್ಭಾವ <unordered_map>
#ಅನ್ತರ್ಭಾವ <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

std::deque<VIface_Capture_ಟ್ಯಾಗ್> V3LinkDotIfaceCapture::s_ಟ್ಯಾಗುಗಳು{};
ತಾರ್ಕಿಕ V3LinkDotIfaceCapture::s_enabled = ನಿಜ;

// LCOV_EXCL_START
ಶೂನ್ಯ V3LinkDotIfaceCapture::enable(ತಾರ್ಕಿಕ flag) {
    s_enabled = flag;
    ಆದರೆ (!flag) clearModuleCache();
}
// LCOV_EXCL_STOP

ಶೂನ್ಯ V3LinkDotIfaceCapture::reset() {
    s_ಟ್ಯಾಗುಗಳು.clear();
    clearModuleCache();
}

// Per-module cache of statement-level names to avoid O(N*M) linear scans.
// Lazily built on first access for a given module; cleared at phase boundaries.
// Uses vectors per name to handle rare cases where different node types share a name
// (e.g. a Typedef and a ParamTypeDType both named 'sc_tag_status_t').
ನಾಮಾಕಾಶ {
ರಚನೆ StmtNameMap final {
    std::unordered_map<string, std::ಸದಿಶ<Ast_ಗಂಟು*>> m_byName;
};
std::unordered_map<Ast_ಗಂಟು_Module*, StmtNameMap> s_moduleCache;

ಸ್ಥಿರಾಂಕ StmtNameMap& getOrBuild(Ast_ಗಂಟು_Module* modp) {
    ಸ್ವಯಂಚಾಲಿತ it = s_moduleCache.ಕಂಡುಹಿಡಿ(modp);
    ಆದರೆ (it != s_moduleCache.ಕೊನೆ()) ಹಿಂತಿರುಗಿ it->ಎರಡನೇ;
    StmtNameMap& cache = s_moduleCache[modp];
    ಗಾಗಿ (Ast_ಗಂಟು* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
        ಸ್ಥಿರಾಂಕ string& nm = stmtp->name();
        ಆದರೆ (!nm.ಖಾಲಿಗಳು()) cache.m_byName[nm].ತಳ್ಳು_ಹಿಂಭಾಗ(stmtp);
    }
    ಹಿಂತಿರುಗಿ cache;
}
}  // namespace

ಶೂನ್ಯ V3LinkDotIfaceCapture::clearModuleCache() { s_moduleCache.clear(); }

AstIfaceRefDType* V3LinkDotIfaceCapture::ifaceRefFromVarDType(Ast_ಗಂಟು_DType* dtypep) {
    AstIfaceRefDType* resultp = ಶೂನ್ಯಸೂಚಕ;
    ಗಾಗಿ (Ast_ಗಂಟು_DType* curp = dtypep; curp;) {
        ಆದರೆ (AstIfaceRefDType* ಸ್ಥಿರಾಂಕ irefp = VN_CAST(curp, IfaceRefDType)) {
            resultp = irefp;
            ನಿಲ್ಲಿಸು;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstBracketArrayDType* ಸ್ಥಿರಾಂಕ bracketp = VN_CAST(curp, BracketArrayDType)) {
            curp = bracketp->subDTypep();
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (AstUnpackArrayDType* ಸ್ಥಿರಾಂಕ unpackp = VN_CAST(curp, UnpackArrayDType)) {
            curp = unpackp->subDTypep();
        } ಇಲ್ಲದಿದ್ದರೆ {
            v3fatalSrc("ifaceRefFromVarDType: unexpected dtype " << curp->prettyTypeName()
                                                                 << " in chain");
        }
    }
    ಹಿಂತಿರುಗಿ resultp;
}

ನಾಮಾಕಾಶ {
// Resolve the owner module name for a typedef/paramType node.
// Returns hint if non-empty, otherwise walks backp() to find the owner module name.
string resolveOwnerName(ಸ್ಥಿರಾಂಕ string& hint, Ast_ಗಂಟು* nodep) {
    ಆದರೆ (!hint.ಖಾಲಿಗಳು()) ಹಿಂತಿರುಗಿ hint;
    ಆದರೆ (!nodep) ಹಿಂತಿರುಗಿ "";
    Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ ownerp = V3LinkDotIfaceCapture::ಕಂಡುಹಿಡಿ_Owner_Module(nodep);
    ಹಿಂತಿರುಗಿ ownerp ? ownerp->name() : string{};
}
}  // namespace

AstTypedef* V3LinkDotIfaceCapture::ಕಂಡುಹಿಡಿ_Typedef_ಒಳ_Module(Ast_ಗಂಟು_Module* modp, ಸ್ಥಿರಾಂಕ string& name) {
    AstTypedef* resultp = ಶೂನ್ಯಸೂಚಕ;
    ಸ್ಥಿರಾಂಕ StmtNameMap& cache = getOrBuild(modp);
    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ it = cache.m_byName.ಕಂಡುಹಿಡಿ(name);
    ಆದರೆ (!(it == cache.m_byName.ಕೊನೆ())) {
        ಗಾಗಿ (Ast_ಗಂಟು* nodep : it->ಎರಡನೇ) {
            ಆದರೆ (AstTypedef* ಸ್ಥಿರಾಂಕ tdp = VN_CAST(nodep, Typedef)) {
                resultp = tdp;
                ನಿಲ್ಲಿಸು;
            }
        }
    }
    ಹಿಂತಿರುಗಿ resultp;
}
Ast_ಗಂಟು_DType* V3LinkDotIfaceCapture::ಕಂಡುಹಿಡಿ_DType_ಒಳ_Module(Ast_ಗಂಟು_Module* modp, ಸ್ಥಿರಾಂಕ string& name,
                                                       VNType type) {

    Ast_ಗಂಟು_DType* resultp = ಶೂನ್ಯಸೂಚಕ;
    ಸ್ಥಿರಾಂಕ StmtNameMap& cache = getOrBuild(modp);
    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ it = cache.m_byName.ಕಂಡುಹಿಡಿ(name);
    ಆದರೆ (!(it == cache.m_byName.ಕೊನೆ())) {
        ಗಾಗಿ (Ast_ಗಂಟು* nodep : it->ಎರಡನೇ) {
            ಆದರೆ (Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dtp = VN_CAST(nodep, ಗಂಟು_DType)) {
                ಆದರೆ (dtp->type() == type) {
                    resultp = dtp;
                    ನಿಲ್ಲಿಸು;
                }
            }
        }
    }
    ಹಿಂತಿರುಗಿ resultp;
}
AstParamTypeDType* V3LinkDotIfaceCapture::ಕಂಡುಹಿಡಿ_Param_Type_ಒಳ_Module(Ast_ಗಂಟು_Module* modp,
                                                                ಸ್ಥಿರಾಂಕ string& name) {

    AstParamTypeDType* resultp = ಶೂನ್ಯಸೂಚಕ;
    ಸ್ಥಿರಾಂಕ StmtNameMap& cache = getOrBuild(modp);
    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ it = cache.m_byName.ಕಂಡುಹಿಡಿ(name);
    ಆದರೆ (!(it == cache.m_byName.ಕೊನೆ())) {
        ಗಾಗಿ (Ast_ಗಂಟು* nodep : it->ಎರಡನೇ) {
            ಆದರೆ (AstParamTypeDType* ಸ್ಥಿರಾಂಕ ptdp = VN_CAST(nodep, ParamTypeDType)) {
                resultp = ptdp;
                ನಿಲ್ಲಿಸು;
            }
        }
    }
    ಹಿಂತಿರುಗಿ resultp;
}

ತಾರ್ಕಿಕ V3LinkDotIfaceCapture::retarget_Ref_ಗಮ್ಯ_Module(AstRefDType* refp, Ast_ಗಂಟು_Module* ಗುರಿ_Modp) {
    ಸ್ಥಿರಾಂಕ VIface_Capture_ಟ್ಯಾಗ್* ಸ್ಥಿರಾಂಕ tagp = refp->captureTagp();
    UASSERT_OBJ(tagp, refp, "Retarget of a reference that was not captured");
    UASSERT_OBJ(ಗುರಿ_Modp, refp, "Retarget to a null module");

    ಆದರೆ (tagp->m_ಬಗೆ == VIface_Capture_ಟ್ಯಾಗ್::ಬಗೆ_2::PARAM_TYPE) {
        AstParamTypeDType* ಸ್ಥಿರಾಂಕ paramTypep = ಕಂಡುಹಿಡಿ_Param_Type_ಒಳ_Module(ಗುರಿ_Modp, refp->name());
        ಆದರೆ (!paramTypep) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        retarget_Ref_ಗಮ್ಯ_Param_Type(refp, paramTypep);
        ಹಿಂತಿರುಗಿ ನಿಜ;
    }

    AstTypedef* ಸ್ಥಿರಾಂಕ typedefp = ಕಂಡುಹಿಡಿ_Typedef_ಒಳ_Module(ಗುರಿ_Modp, refp->name());
    ಆದರೆ (!typedefp) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    retarget_Ref_ಗಮ್ಯ_Typedef(refp, typedefp);
    ಹಿಂತಿರುಗಿ ನಿಜ;
}

ಶೂನ್ಯ V3LinkDotIfaceCapture::retarget_Ref_ಗಮ್ಯ_Param_Type(AstRefDType* refp,
                                                   AstParamTypeDType* paramTypep) {
    UASSERT_OBJ(paramTypep, refp, "Retarget to a null parameter type");
    refp->refDTypep(paramTypep);
    refp->dtypep(paramTypep);
}

ಶೂನ್ಯ V3LinkDotIfaceCapture::retarget_Ref_ಗಮ್ಯ_Typedef(AstRefDType* refp, AstTypedef* typedefp) {
    UASSERT_OBJ(typedefp, refp, "Retarget to a null typedef");
    refp->typedefp(typedefp);
    // An incomplete typedef is still a successful name resolution, but
    // must not erase type links that a later width pass can complete.
    ಆದರೆ (Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dtypep = typedefp->subDTypep()) {
        refp->refDTypep(dtypep);
        refp->dtypep(dtypep);
    }
}

ನಾಮಾಕಾಶ {
ಬಳಸಿ ಜೀವಂತ_Nodes = std::unordered_ಗಣ<ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು*>;

// A scoped snapshot of every node currently in the tree.  V3Broken::isLinkable()
// cannot serve this role: its table is populated only while V3Broken::brokenAll()
// runs and is cleared before it returns.
ಜೀವಂತ_Nodes collect_ಜೀವಂತ_Nodes() {
    ಜೀವಂತ_Nodes ಜೀವಂತ_Nodes_2;
    v3Global.rootp()->foreach([&](Ast_ಗಂಟು* nodep) { ಜೀವಂತ_Nodes_2.ಸೇರಿಸು(nodep); });
    ಹಿಂತಿರುಗಿ ಜೀವಂತ_Nodes_2;
}

// Find the module that owns this node; a snapshot, if given, stops the walk at a stale link.
Ast_ಗಂಟು_Module* ಕಂಡುಹಿಡಿ_Owner_Module_Impl(Ast_ಗಂಟು* nodep, ಸ್ಥಿರಾಂಕ ಜೀವಂತ_Nodes* ಜೀವಂತ_Nodesp) {
    ಗಾಗಿ (Ast_ಗಂಟು* curp = nodep; curp; curp = curp->backp()) {
        ಆದರೆ (ಜೀವಂತ_Nodesp && !ಜೀವಂತ_Nodesp->ಎಣಿಕೆ_2(curp)) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ modp = VN_CAST(curp, ಗಂಟು_Module)) ಹಿಂತಿರುಗಿ modp;
    }
    ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
}

Ast_ಗಂಟು_Module* ಕಂಡುಹಿಡಿ_Owner_Module_If_ಜೀವಂತ(Ast_ಗಂಟು* nodep, ಸ್ಥಿರಾಂಕ ಜೀವಂತ_Nodes& ಜೀವಂತ_Nodes_2) {
    ಹಿಂತಿರುಗಿ ಕಂಡುಹಿಡಿ_Owner_Module_Impl(nodep, &ಜೀವಂತ_Nodes_2);
}

ತಾರ್ಕಿಕ moduleMatchesOwner(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* modp, ಸ್ಥಿರಾಂಕ string& ownerName) {
    ಆದರೆ (!modp || ownerName.ಖಾಲಿಗಳು()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    ಹಿಂತಿರುಗಿ modp->name() == ownerName || modp->origName() == ownerName;
}
}  // namespace

Ast_ಗಂಟು_Module* V3LinkDotIfaceCapture::ಕಂಡುಹಿಡಿ_Owner_Module(Ast_ಗಂಟು* nodep) {
    ಹಿಂತಿರುಗಿ ಕಂಡುಹಿಡಿ_Owner_Module_Impl(nodep, ಶೂನ್ಯಸೂಚಕ);
}

ಶೂನ್ಯ V3LinkDotIfaceCapture::dumpEntries(ಸ್ಥಿರಾಂಕ string& label) {
    UINFO(9, "========== iface capture dumpEntries: " << label << " ==========");
    ಪೂರ್ಣಾಂಕ idx = 0;
    v3Global.rootp()->foreach([&](AstRefDType* refp) {
        ಸ್ಥಿರಾಂಕ VIface_Capture_ಟ್ಯಾಗ್* ಸ್ಥಿರಾಂಕ tagp = capture_ಟ್ಯಾಗ್(refp);
        ಆದರೆ (!tagp) ಹಿಂತಿರುಗಿ;
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ ownerModp = ಕಂಡುಹಿಡಿ_Owner_Module(refp);
        UINFO(9, "  [" << idx << "] ref=" << refp->name() << " refp=" << cvt_ಗಮ್ಯ_Hex(refp)
                       << " ownerMod=" << (ownerModp ? ownerModp->name() : "<null>")
                       << " cellPath='" << tagp->m_cell_ಮಾರ್ಗ << "' ownerModName='"
                       << tagp->m_ownerModName << "' kind="
                       << (tagp->m_ಬಗೆ == VIface_Capture_ಟ್ಯಾಗ್::ಬಗೆ_2::PARAM_TYPE ? "param type"
                                                                              : "typedef"));
        ++idx;
    });
    UINFO(9, "========== end iface capture dumpEntries (" << idx << " captured) ==========");
}

ಶೂನ್ಯ V3LinkDotIfaceCapture::ಟ್ಯಾಗ್(AstRefDType* refp, ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* capturedInp,
                                VIface_Capture_ಟ್ಯಾಗ್::ಬಗೆ_2 ಬಗೆ, ಸ್ಥಿರಾಂಕ string& cell_ಮಾರ್ಗ,
                                ಸ್ಥಿರಾಂಕ string& ownerModName) {
    UASSERT_OBJ(!refp->captureTagp(), refp, "Reference captured twice");
    UASSERT_OBJ(capturedInp, refp, "Captured reference is not in a module");
    UASSERT_OBJ(!cell_ಮಾರ್ಗ.ಖಾಲಿಗಳು(), refp, "Captured reference has no cell path");
    s_ಟ್ಯಾಗುಗಳು.emplace_ಹಿಂಭಾಗ(VIface_Capture_ಟ್ಯಾಗ್{ಬಗೆ, cell_ಮಾರ್ಗ, ownerModName, capturedInp->origName()});
    refp->captureTagp(&s_ಟ್ಯಾಗುಗಳು.ಹಿಂಭಾಗ());
}

ಸ್ಥಿರಾಂಕ VIface_Capture_ಟ್ಯಾಗ್* V3LinkDotIfaceCapture::capture_ಟ್ಯಾಗ್(AstRefDType* refp) {
    ಸ್ಥಿರಾಂಕ VIface_Capture_ಟ್ಯಾಗ್* ಸ್ಥಿರಾಂಕ tagp = refp->captureTagp();
    ಆದರೆ (!tagp) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
    ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ ownerp = ಕಂಡುಹಿಡಿ_Owner_Module(refp);
    ಆದರೆ (!ownerp || ownerp->origName() != tagp->m_captured_ಒಳ_Name) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
    ಹಿಂತಿರುಗಿ tagp;
}

// Walk a dot-separated cell path through the cell / IFACEREFDTYPE hierarchy
// starting from startModp.  Returns the module at the end of the path, or
// nullptr if any component cannot be resolved.
// Cell/port names are preserved across clones by cloneTree, so this works
// identically on template and cloned modules.
Ast_ಗಂಟು_Module* V3LinkDotIfaceCapture::follow_Cell_ಮಾರ್ಗ(Ast_ಗಂಟು_Module* ಆರಂಭ_Modp,
                                                     ಸ್ಥಿರಾಂಕ string& cell_ಮಾರ್ಗ) {
    ಆದರೆ (cell_ಮಾರ್ಗ.ಖಾಲಿಗಳು() || !ಆರಂಭ_Modp) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
    Ast_ಗಂಟು_Module* curModp = ಆರಂಭ_Modp;
    string ಉಳಿದ = cell_ಮಾರ್ಗ;
    ಆಗ (!ಉಳಿದ.ಖಾಲಿಗಳು() && curModp) {
        string component;
        ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t dotPos = ಉಳಿದ.ಕಂಡುಹಿಡಿ('.');
        ಆದರೆ (dotPos == string::npos) {
            component = ಉಳಿದ;
            ಉಳಿದ.clear();
        } ಇಲ್ಲದಿದ್ದರೆ {
            component = ಉಳಿದ.substr(0, dotPos);
            ಉಳಿದ = ಉಳಿದ.substr(dotPos + 1);
        }
        // Matched without any array index; the elements of an instance array (see V3Param) all
        // instantiate the same module, so any of them will do
        ಸ್ಥಿರಾಂಕ string component_ತಳ = Ast_ಗಂಟು::nameNoArray(component);
        Ast_ಗಂಟು_Module* ಮುಂದಿನ_Modp = ಶೂನ್ಯಸೂಚಕ;
        ಗಾಗಿ (Ast_ಗಂಟು* sp = curModp->stmtsp(); sp; sp = sp->nextp()) {
            ಆದರೆ (AstCell* ಸ್ಥಿರಾಂಕ cellp = VN_CAST(sp, Cell)) {
                ಆದರೆ ((cellp->name() == component
                     || Ast_ಗಂಟು::nameNoArray(cellp->name()) == component_ತಳ)
                    && cellp->modp()) {
                    ಮುಂದಿನ_Modp = cellp->modp();
                    ನಿಲ್ಲಿಸು;
                }
            }
            ಆದರೆ (AstVar* ಸ್ಥಿರಾಂಕ varp = VN_CAST(sp, Var)) {
                ಆದರೆ (varp->isIfaceRef() && varp->subDTypep()) {
                    string var_ತಳ_Name = varp->name();
                    ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t viftopPos = var_ತಳ_Name.ಕಂಡುಹಿಡಿ("__Viftop");
                    ಆದರೆ (viftopPos != string::npos) {
                        var_ತಳ_Name = var_ತಳ_Name.substr(0, viftopPos);
                    }
                    ಆದರೆ (var_ತಳ_Name == component
                        || Ast_ಗಂಟು::nameNoArray(var_ತಳ_Name) == component_ತಳ) {
                        ಆದರೆ (AstIfaceRefDType* ಸ್ಥಿರಾಂಕ irefp
                            = ifaceRefFromVarDType(varp->subDTypep())) {
                            AstIface* ಸ್ಥಿರಾಂಕ ifacep = irefp->iface_ಮೂಲಕ_Cellp();
                            ಆದರೆ (ifacep) {
                                ಮುಂದಿನ_Modp = ifacep;
                                ನಿಲ್ಲಿಸು;
                            }
                        }
                    }
                }
            }
        }
        curModp = ಮುಂದಿನ_Modp;
    }
    ಹಿಂತಿರುಗಿ curModp;
}

// replaces the lambda used in V3LinkDot.cpp for iface capture
ಶೂನ್ಯ V3LinkDotIfaceCapture::captureTypedefContext(AstRefDType* refp, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* stageLabel,
                                                  ಪೂರ್ಣಾಂಕ dotPos, ಸ್ಥಿರಾಂಕ std::string& dot_ಪಠ್ಯ,
                                                  VSymEnt* dotSymp, Ast_ಗಂಟು_Module* modp,
                                                  ಸ್ಥಿರಾಂಕ std::function<std::string()>& indentFn) {
    ಆದರೆ (!enabled() || !refp) ಹಿಂತಿರುಗಿ;

    UINFO(9, indentFn() << "iface capture capture request stage=" << stageLabel
                        << " typedef=" << refp << " name=" << refp->name() << " dotPos=" << dotPos
                        << " dotText='" << dot_ಪಠ್ಯ << "' dotSym=" << dotSymp);

    ಸ್ಥಿರಾಂಕ AstCell* ifaceCellp = ಶೂನ್ಯಸೂಚಕ;
    ಆದರೆ (dotSymp && VN_IS(dotSymp->nodep(), Cell)) {
        ಸ್ಥಿರಾಂಕ AstCell* ಸ್ಥಿರಾಂಕ cellp = VN_AS(dotSymp->nodep(), Cell);
        ಆದರೆ (cellp->modp() && VN_IS(cellp->modp(), Iface)) ifaceCellp = cellp;
    }
    ಆದರೆ (!ifaceCellp) {
        UINFO(9, indentFn() << "iface capture capture skipped typedef=" << refp
                            << " (no iface context)");
        ಹಿಂತಿರುಗಿ;
    }
    // Skip internal interface typedef references (typedef used within its own interface)
    ಆದರೆ (ifaceCellp->modp() == modp) {
        UINFO(9, indentFn() << "iface capture capture skipped typedef=" << refp
                            << " (internal to interface " << modp->name() << ")");
        ಹಿಂತಿರುಗಿ;
    }

    // dotText is always non-empty for interface typedef captures.  If this
    // fires, the caller resolved to an interface Cell but did not accumulate
    // a dotText path - investigate the dot-state in visitParseRef.
    UASSERT_OBJ(!dot_ಪಠ್ಯ.ಖಾಲಿಗಳು(), refp, "captureTypedefContext: dotText empty");
    ಸ್ಥಿರಾಂಕ string cell_ಮಾರ್ಗ = dot_ಪಠ್ಯ;

    // A reference to a ParamTypeDType is captured as a parameter type, else as a typedef
    ಆದರೆ (AstParamTypeDType* ಸ್ಥಿರಾಂಕ paramTypep = VN_CAST(refp->refDTypep(), ParamTypeDType)) {
        V3LinkDotIfaceCapture::ಕೂಡಿಸು_Param_Type(refp, cell_ಮಾರ್ಗ, modp, paramTypep, "");
    } ಇಲ್ಲದಿದ್ದರೆ {
        ಟ್ಯಾಗ್(refp, modp, VIface_Capture_ಟ್ಯಾಗ್::ಬಗೆ_2::TYPEDEF, cell_ಮಾರ್ಗ,
            resolveOwnerName("", refp->typedefp()));
    }

    UINFO(9, indentFn() << "iface capture capture success typedef=" << refp
                        << " cell=" << ifaceCellp << " cellPath='" << cell_ಮಾರ್ಗ << "'"
                        << " mod=" << (ifaceCellp->modp() ? ifaceCellp->modp()->name() : "<null>")
                        << " dotPos=" << dotPos);
}

ಶೂನ್ಯ V3LinkDotIfaceCapture::ಕೂಡಿಸು_Param_Type(AstRefDType* refp, ಸ್ಥಿರಾಂಕ string& cell_ಮಾರ್ಗ,
                                         Ast_ಗಂಟು_Module* ownerModp, AstParamTypeDType* paramTypep,
                                         ಸ್ಥಿರಾಂಕ string& paramTypeOwnerModName) {
    UASSERT(refp, "addParamType() called with null refp");
    UASSERT(ownerModp,
            "addParamType() called with null ownerModp for refp='" << refp->prettyNameQ() << "'");
    UASSERT_OBJ(paramTypep, refp,
                "addParamType() called with null paramTypep for refp='" << refp->prettyNameQ()
                                                                        << "'");
    ಸ್ಥಿರಾಂಕ string ptOwnerName = resolveOwnerName(paramTypeOwnerModName, paramTypep);
    UINFO(9, "addParamType: refp=" << refp << " cellPath='" << cell_ಮಾರ್ಗ << "'"
                                   << " ownerModp=" << (ownerModp ? ownerModp->name() : "<null>")
                                   << " paramTypep=" << paramTypep << " paramTypeOwnerModName='"
                                   << ptOwnerName << "'");
    UINFO(9, "addParamType: paramTypep subDTypep chain:");
    ಆದರೆ (debug())
        paramTypep->foreach([&](AstRefDType* innerRefp) {
            UINFO(9,
                  "  inner RefDType: "
                      << innerRefp << " refDTypep=" << innerRefp->refDTypep()
                      << (innerRefp->refDTypep() ? " refDTypep->name=" : "")
                      << (innerRefp->refDTypep() ? innerRefp->refDTypep()->prettyTypeName() : ""));
        });
    ಟ್ಯಾಗ್(refp, ownerModp, VIface_Capture_ಟ್ಯಾಗ್::ಬಗೆ_2::PARAM_TYPE, cell_ಮಾರ್ಗ, ptOwnerName);
}

ಪೂರ್ಣಾಂಕ V3LinkDotIfaceCapture::resolveCapturedRefs() {
    ಪೂರ್ಣಾಂಕ fixed = 0;

    // TARGET RESOLUTION.  By this point all cloning is complete and cell
    // pointers are wired to the correct interface clones.  For each tagged
    // reference we walk its cell path from its owner module to find the
    // correct target module, then locate the PARAMTYPEDTYPE / TYPEDEF by name.
    // See the ARCHITECTURE comment above for the full picture.

    v3Global.rootp()->foreach([&](AstRefDType* refp) {
        ಸ್ಥಿರಾಂಕ VIface_Capture_ಟ್ಯಾಗ್* ಸ್ಥಿರಾಂಕ tagp = capture_ಟ್ಯಾಗ್(refp);
        ಆದರೆ (!tagp) ಹಿಂತಿರುಗಿ;
        Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ ownerModp = ಕಂಡುಹಿಡಿ_Owner_Module(refp);
        ಆದರೆ (!ownerModp || ownerModp->dead() || VN_IS(ownerModp, Package)) ಹಿಂತಿರುಗಿ;

        UINFO(9, "finalizeIfaceCapture Phase3 entry: refp="
                     << refp->name() << " (" << cvt_ಗಮ್ಯ_Hex(refp) << ")" << " ownerMod="
                     << ownerModp->name() << " cellPath='" << tagp->m_cell_ಮಾರ್ಗ << "' targetKind="
                     << (tagp->m_ಬಗೆ == VIface_Capture_ಟ್ಯಾಗ್::ಬಗೆ_2::PARAM_TYPE ? "param type"
                                                                            : "typedef"));

        // Prefer the owner itself when its stable template identity matches the
        // captured target owner.  Otherwise resolve and validate the cell path.
        Ast_ಗಂಟು_Module* correctModp = ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (moduleMatchesOwner(ownerModp, tagp->m_ownerModName)) {
            correctModp = ownerModp;
        } ಇಲ್ಲದಿದ್ದರೆ {
            correctModp = follow_Cell_ಮಾರ್ಗ(ownerModp, tagp->m_cell_ಮಾರ್ಗ);
            UINFO(9, "  followCellPath('"
                         << ownerModp->name() << "', '" << tagp->m_cell_ಮಾರ್ಗ
                         << "') = " << (correctModp ? correctModp->name() : "<null>")
                         << (correctModp ? (correctModp->dead() ? " (DEAD)" : " (live)") : ""));
            UASSERT_OBJ(correctModp && !correctModp->dead()
                            && moduleMatchesOwner(correctModp, tagp->m_ownerModName),
                        refp,
                        "captured ref '" << refp->prettyNameQ() << "' cell path '"
                                         << tagp->m_cell_ಮಾರ್ಗ << "' did not resolve to live owner '"
                                         << tagp->m_ownerModName << "'");
        }
        UASSERT_OBJ(retarget_Ref_ಗಮ್ಯ_Module(refp, correctModp), refp,
                    "could not retarget captured "
                        << (tagp->m_ಬಗೆ == VIface_Capture_ಟ್ಯಾಗ್::ಬಗೆ_2::PARAM_TYPE ? "parameter type "
                                                                               : "typedef ")
                        << refp->prettyNameQ() << " in " << correctModp->prettyNameQ());
        refp->user3(ನಿಜ);
        ++fixed;
    });

    ಹಿಂತಿರುಗಿ fixed;
}

ಶೂನ್ಯ V3LinkDotIfaceCapture::verifyNoDeadRefs(ಸ್ಥಿರಾಂಕ ಜೀವಂತ_Nodes& ಜೀವಂತ_Nodes_2) {
    // Assert: no REFDTYPE in any live module should have typedefp or refDTypep
    // pointing to a dead module.
    ಗಾಗಿ (Ast_ಗಂಟು* nodep = v3Global.rootp()->modulesp(); nodep; nodep = nodep->nextp()) {
        ಆದರೆ (Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ modp = VN_CAST(nodep, ಗಂಟು_Module)) {
            ಆದರೆ (modp->dead()) ಮುಂದುವರಿ;
            modp->foreach([&](AstRefDType* refp) {
                ಆದರೆ (refp->typedefp()) {
                    UASSERT_OBJ(ಜೀವಂತ_Nodes_2.ಎಣಿಕೆ_2(refp->typedefp()), refp,
                                "REFDTYPE '" << refp->prettyNameQ() << "' in live module '"
                                             << modp->prettyNameQ()
                                             << "' has a dangling typedefp");
                    Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ ownerModp
                        = ಕಂಡುಹಿಡಿ_Owner_Module_If_ಜೀವಂತ(refp->typedefp(), ಜೀವಂತ_Nodes_2);
                    UASSERT_OBJ(!ownerModp || !ownerModp->dead(), refp,
                                "REFDTYPE '" << refp->prettyNameQ() << "' in live module '"
                                             << modp->prettyNameQ()
                                             << "' has typedefp pointing to dead module '"
                                             << ownerModp->prettyNameQ() << "'");
                }
                ಆದರೆ (refp->refDTypep()) {
                    UASSERT_OBJ(ಜೀವಂತ_Nodes_2.ಎಣಿಕೆ_2(refp->refDTypep()), refp,
                                "REFDTYPE '" << refp->prettyNameQ() << "' in live module '"
                                             << modp->prettyNameQ()
                                             << "' has a dangling refDTypep");
                    Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ ownerModp
                        = ಕಂಡುಹಿಡಿ_Owner_Module_If_ಜೀವಂತ(refp->refDTypep(), ಜೀವಂತ_Nodes_2);
                    UASSERT_OBJ(!ownerModp || !ownerModp->dead(), refp,
                                "REFDTYPE '" << refp->prettyNameQ() << "' in live module '"
                                             << modp->prettyNameQ()
                                             << "' has refDTypep pointing to dead module '"
                                             << ownerModp->prettyNameQ() << "'");
                }
            });
        }
    }
    ಆದರೆ (v3Global.rootp()->typeTablep()) {
        ಗಾಗಿ (Ast_ಗಂಟು* nodep = v3Global.rootp()->typeTablep()->typesp(); nodep;
             nodep = nodep->nextp()) {
            nodep->foreach([&](AstRefDType* refp) {
                ಆದರೆ (refp->typedefp()) {
                    UASSERT_OBJ(ಜೀವಂತ_Nodes_2.ಎಣಿಕೆ_2(refp->typedefp()), refp,
                                "REFDTYPE '" << refp->prettyNameQ()
                                             << "' in type table has a dangling typedefp");
                    Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ ownerModp
                        = ಕಂಡುಹಿಡಿ_Owner_Module_If_ಜೀವಂತ(refp->typedefp(), ಜೀವಂತ_Nodes_2);
                    UASSERT_OBJ(!ownerModp || !ownerModp->dead(), refp,
                                "REFDTYPE '"
                                    << refp->prettyNameQ()
                                    << "' in type table has typedefp pointing to dead module '"
                                    << ownerModp->prettyNameQ() << "'");
                }
                ಆದರೆ (refp->refDTypep()) {
                    UASSERT_OBJ(ಜೀವಂತ_Nodes_2.ಎಣಿಕೆ_2(refp->refDTypep()), refp,
                                "REFDTYPE '" << refp->prettyNameQ()
                                             << "' in type table has a dangling refDTypep");
                    Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ ownerModp
                        = ಕಂಡುಹಿಡಿ_Owner_Module_If_ಜೀವಂತ(refp->refDTypep(), ಜೀವಂತ_Nodes_2);
                    UASSERT_OBJ(!ownerModp || !ownerModp->dead(), refp,
                                "REFDTYPE '"
                                    << refp->prettyNameQ()
                                    << "' in type table has refDTypep pointing to dead module '"
                                    << ownerModp->prettyNameQ() << "'");
                }
            });
        }
    }
}

ಶೂನ್ಯ V3LinkDotIfaceCapture::finalizeIfaceCapture() {
    ಆದರೆ (!s_enabled) ಹಿಂತಿರುಗಿ;
    UINFO(4, "finalizeIfaceCapture: fixing remaining cross-interface refs");
    ಆದರೆ (!v3Global.rootp()) ಹಿಂತಿರುಗಿ;
    clearModuleCache();  // Ensure fresh view after all cloning/widthing

    // Resolve live captured refs from stable path metadata before inspecting
    // any inherited target pointers, which may refer to replaced template nodes.
    ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ capturedFixed = resolveCapturedRefs();
    UINFO(4, "finalizeIfaceCapture: structurally resolved " << capturedFixed << " captured refs");

    ಆದರೆ (debug() >= 9) dumpEntries("after finalizeIfaceCapture");

    // Emit statistics for --stats
    V3Stats::ಕೂಡಿಸು_Stat("IfaceCapture, Captured refs", s_ಟ್ಯಾಗುಗಳು.ಗಾತ್ರ_2());
    V3Stats::ಕೂಡಿಸು_Stat("IfaceCapture, Captured refs resolved", capturedFixed);

    // No reference may be left pointing into a module about to be removed
    ಆದರೆ (v3Global.opt.debugCheck()) verifyNoDeadRefs(collect_ಜೀವಂತ_Nodes());
    // Drop every tag before its record is freed.
    v3Global.rootp()->foreach([](AstRefDType* refp) { refp->captureTagp(ಶೂನ್ಯಸೂಚಕ); });
    reset();
}
