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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3LinkDotIfaceCapture.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Error.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Global.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Stats.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3SymTable.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <unordered_map>
#ਅਨ੍ਤਰ੍ਭਾਵ <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

std::deque<VIface_Capture_ਟੈਗ> V3LinkDotIfaceCapture::s_ਟੈਗ_ਸੂਚੀ{};
ਤਾਰਕਿਕ V3LinkDotIfaceCapture::s_enabled = ਸੱਚ;

// LCOV_EXCL_START
ਖਾਲੀ V3LinkDotIfaceCapture::enable(ਤਾਰਕਿਕ flag) {
    s_enabled = flag;
    ਜੇ (!flag) clearModuleCache();
}
// LCOV_EXCL_STOP

ਖਾਲੀ V3LinkDotIfaceCapture::reset() {
    s_ਟੈਗ_ਸੂਚੀ.clear();
    clearModuleCache();
}

// Per-module cache of statement-level names to avoid O(N*M) linear scans.
// Lazily built on first access for a given module; cleared at phase boundaries.
// Uses vectors per name to handle rare cases where different node types share a name
// (e.g. a Typedef and a ParamTypeDType both named 'sc_tag_status_t').
ਨਾਮਾਕਾਸ਼ {
ਬਣਤਰ StmtNameMap final {
    std::unordered_map<string, std::ਸਦਿਸ਼<Ast_ਗੰਢ*>> m_byName;
};
std::unordered_map<Ast_ਗੰਢ_Module*, StmtNameMap> s_moduleCache;

ਅਚਲ StmtNameMap& getOrBuild(Ast_ਗੰਢ_Module* modp) {
    ਸਵੈਚਾਲਿਤ it = s_moduleCache.ਲੱਭੋ(modp);
    ਜੇ (it != s_moduleCache.ਸਿਰਾ()) ਵਾਪਸ it->ਦੂਜਾ;
    StmtNameMap& cache = s_moduleCache[modp];
    ਲਈ (Ast_ਗੰਢ* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
        ਅਚਲ string& nm = stmtp->name();
        ਜੇ (!nm.ਖਾਲੀ_ਥਾਵਾਂ()) cache.m_byName[nm].ਧੱਕੋ_ਪਿੱਠ(stmtp);
    }
    ਵਾਪਸ cache;
}
}  // namespace

ਖਾਲੀ V3LinkDotIfaceCapture::clearModuleCache() { s_moduleCache.clear(); }

AstIfaceRefDType* V3LinkDotIfaceCapture::ifaceRefFromVarDType(Ast_ਗੰਢ_DType* dtypep) {
    AstIfaceRefDType* resultp = ਖਾਲੀ_ਸੂਚਕ;
    ਲਈ (Ast_ਗੰਢ_DType* curp = dtypep; curp;) {
        ਜੇ (AstIfaceRefDType* ਅਚਲ irefp = VN_CAST(curp, IfaceRefDType)) {
            resultp = irefp;
            ਰੋਕੋ;
        } ਨਹੀਂਤਾਂ ਜੇ (AstBracketArrayDType* ਅਚਲ bracketp = VN_CAST(curp, BracketArrayDType)) {
            curp = bracketp->subDTypep();
        } ਨਹੀਂਤਾਂ ਜੇ (AstUnpackArrayDType* ਅਚਲ unpackp = VN_CAST(curp, UnpackArrayDType)) {
            curp = unpackp->subDTypep();
        } ਨਹੀਂਤਾਂ {
            v3fatalSrc("ifaceRefFromVarDType: unexpected dtype " << curp->prettyTypeName()
                                                                 << " in chain");
        }
    }
    ਵਾਪਸ resultp;
}

ਨਾਮਾਕਾਸ਼ {
// Resolve the owner module name for a typedef/paramType node.
// Returns hint if non-empty, otherwise walks backp() to find the owner module name.
string resolveOwnerName(ਅਚਲ string& hint, Ast_ਗੰਢ* nodep) {
    ਜੇ (!hint.ਖਾਲੀ_ਥਾਵਾਂ()) ਵਾਪਸ hint;
    ਜੇ (!nodep) ਵਾਪਸ "";
    Ast_ਗੰਢ_Module* ਅਚਲ ownerp = V3LinkDotIfaceCapture::ਲੱਭੋ_Owner_Module(nodep);
    ਵਾਪਸ ownerp ? ownerp->name() : string{};
}
}  // namespace

AstTypedef* V3LinkDotIfaceCapture::ਲੱਭੋ_Typedef_ਵਿੱਚ_Module(Ast_ਗੰਢ_Module* modp, ਅਚਲ string& name) {
    AstTypedef* resultp = ਖਾਲੀ_ਸੂਚਕ;
    ਅਚਲ StmtNameMap& cache = getOrBuild(modp);
    ਅਚਲ ਸਵੈਚਾਲਿਤ it = cache.m_byName.ਲੱਭੋ(name);
    ਜੇ (!(it == cache.m_byName.ਸਿਰਾ())) {
        ਲਈ (Ast_ਗੰਢ* nodep : it->ਦੂਜਾ) {
            ਜੇ (AstTypedef* ਅਚਲ tdp = VN_CAST(nodep, Typedef)) {
                resultp = tdp;
                ਰੋਕੋ;
            }
        }
    }
    ਵਾਪਸ resultp;
}
Ast_ਗੰਢ_DType* V3LinkDotIfaceCapture::ਲੱਭੋ_DType_ਵਿੱਚ_Module(Ast_ਗੰਢ_Module* modp, ਅਚਲ string& name,
                                                       VNType type) {

    Ast_ਗੰਢ_DType* resultp = ਖਾਲੀ_ਸੂਚਕ;
    ਅਚਲ StmtNameMap& cache = getOrBuild(modp);
    ਅਚਲ ਸਵੈਚਾਲਿਤ it = cache.m_byName.ਲੱਭੋ(name);
    ਜੇ (!(it == cache.m_byName.ਸਿਰਾ())) {
        ਲਈ (Ast_ਗੰਢ* nodep : it->ਦੂਜਾ) {
            ਜੇ (Ast_ਗੰਢ_DType* ਅਚਲ dtp = VN_CAST(nodep, ਗੰਢ_DType)) {
                ਜੇ (dtp->type() == type) {
                    resultp = dtp;
                    ਰੋਕੋ;
                }
            }
        }
    }
    ਵਾਪਸ resultp;
}
AstParamTypeDType* V3LinkDotIfaceCapture::ਲੱਭੋ_Param_Type_ਵਿੱਚ_Module(Ast_ਗੰਢ_Module* modp,
                                                                ਅਚਲ string& name) {

    AstParamTypeDType* resultp = ਖਾਲੀ_ਸੂਚਕ;
    ਅਚਲ StmtNameMap& cache = getOrBuild(modp);
    ਅਚਲ ਸਵੈਚਾਲਿਤ it = cache.m_byName.ਲੱਭੋ(name);
    ਜੇ (!(it == cache.m_byName.ਸਿਰਾ())) {
        ਲਈ (Ast_ਗੰਢ* nodep : it->ਦੂਜਾ) {
            ਜੇ (AstParamTypeDType* ਅਚਲ ptdp = VN_CAST(nodep, ParamTypeDType)) {
                resultp = ptdp;
                ਰੋਕੋ;
            }
        }
    }
    ਵਾਪਸ resultp;
}

ਤਾਰਕਿਕ V3LinkDotIfaceCapture::retarget_Ref_ਮੰਜ਼ਿਲ_Module(AstRefDType* refp, Ast_ਗੰਢ_Module* ਨਿਸ਼ਾਨਾ_Modp) {
    ਅਚਲ VIface_Capture_ਟੈਗ* ਅਚਲ tagp = refp->captureTagp();
    UASSERT_OBJ(tagp, refp, "Retarget of a reference that was not captured");
    UASSERT_OBJ(ਨਿਸ਼ਾਨਾ_Modp, refp, "Retarget to a null module");

    ਜੇ (tagp->m_ਕਿਸਮ == VIface_Capture_ਟੈਗ::ਕਿਸਮ_2::PARAM_TYPE) {
        AstParamTypeDType* ਅਚਲ paramTypep = ਲੱਭੋ_Param_Type_ਵਿੱਚ_Module(ਨਿਸ਼ਾਨਾ_Modp, refp->name());
        ਜੇ (!paramTypep) ਵਾਪਸ ਝੂਠ;
        retarget_Ref_ਮੰਜ਼ਿਲ_Param_Type(refp, paramTypep);
        ਵਾਪਸ ਸੱਚ;
    }

    AstTypedef* ਅਚਲ typedefp = ਲੱਭੋ_Typedef_ਵਿੱਚ_Module(ਨਿਸ਼ਾਨਾ_Modp, refp->name());
    ਜੇ (!typedefp) ਵਾਪਸ ਝੂਠ;
    retarget_Ref_ਮੰਜ਼ਿਲ_Typedef(refp, typedefp);
    ਵਾਪਸ ਸੱਚ;
}

ਖਾਲੀ V3LinkDotIfaceCapture::retarget_Ref_ਮੰਜ਼ਿਲ_Param_Type(AstRefDType* refp,
                                                   AstParamTypeDType* paramTypep) {
    UASSERT_OBJ(paramTypep, refp, "Retarget to a null parameter type");
    refp->refDTypep(paramTypep);
    refp->dtypep(paramTypep);
}

ਖਾਲੀ V3LinkDotIfaceCapture::retarget_Ref_ਮੰਜ਼ਿਲ_Typedef(AstRefDType* refp, AstTypedef* typedefp) {
    UASSERT_OBJ(typedefp, refp, "Retarget to a null typedef");
    refp->typedefp(typedefp);
    // An incomplete typedef is still a successful name resolution, but
    // must not erase type links that a later width pass can complete.
    ਜੇ (Ast_ਗੰਢ_DType* ਅਚਲ dtypep = typedefp->subDTypep()) {
        refp->refDTypep(dtypep);
        refp->dtypep(dtypep);
    }
}

ਨਾਮਾਕਾਸ਼ {
ਵਰਤੋਂ ਜੀਵਤ_Nodes = std::unordered_ਸੈੱਟ<ਅਚਲ Ast_ਗੰਢ*>;

// A scoped snapshot of every node currently in the tree.  V3Broken::isLinkable()
// cannot serve this role: its table is populated only while V3Broken::brokenAll()
// runs and is cleared before it returns.
ਜੀਵਤ_Nodes collect_ਜੀਵਤ_Nodes() {
    ਜੀਵਤ_Nodes ਜੀਵਤ_Nodes_2;
    v3Global.rootp()->foreach([&](Ast_ਗੰਢ* nodep) { ਜੀਵਤ_Nodes_2.ਪਾਓ(nodep); });
    ਵਾਪਸ ਜੀਵਤ_Nodes_2;
}

// Find the module that owns this node; a snapshot, if given, stops the walk at a stale link.
Ast_ਗੰਢ_Module* ਲੱਭੋ_Owner_Module_Impl(Ast_ਗੰਢ* nodep, ਅਚਲ ਜੀਵਤ_Nodes* ਜੀਵਤ_Nodesp) {
    ਲਈ (Ast_ਗੰਢ* curp = nodep; curp; curp = curp->backp()) {
        ਜੇ (ਜੀਵਤ_Nodesp && !ਜੀਵਤ_Nodesp->ਗਿਣਤੀ_2(curp)) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (Ast_ਗੰਢ_Module* ਅਚਲ modp = VN_CAST(curp, ਗੰਢ_Module)) ਵਾਪਸ modp;
    }
    ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
}

Ast_ਗੰਢ_Module* ਲੱਭੋ_Owner_Module_If_ਜੀਵਤ(Ast_ਗੰਢ* nodep, ਅਚਲ ਜੀਵਤ_Nodes& ਜੀਵਤ_Nodes_2) {
    ਵਾਪਸ ਲੱਭੋ_Owner_Module_Impl(nodep, &ਜੀਵਤ_Nodes_2);
}

ਤਾਰਕਿਕ moduleMatchesOwner(ਅਚਲ Ast_ਗੰਢ_Module* modp, ਅਚਲ string& ownerName) {
    ਜੇ (!modp || ownerName.ਖਾਲੀ_ਥਾਵਾਂ()) ਵਾਪਸ ਝੂਠ;
    ਵਾਪਸ modp->name() == ownerName || modp->origName() == ownerName;
}
}  // namespace

Ast_ਗੰਢ_Module* V3LinkDotIfaceCapture::ਲੱਭੋ_Owner_Module(Ast_ਗੰਢ* nodep) {
    ਵਾਪਸ ਲੱਭੋ_Owner_Module_Impl(nodep, ਖਾਲੀ_ਸੂਚਕ);
}

ਖਾਲੀ V3LinkDotIfaceCapture::dumpEntries(ਅਚਲ string& label) {
    UINFO(9, "========== iface capture dumpEntries: " << label << " ==========");
    ਪੂਰਨ idx = 0;
    v3Global.rootp()->foreach([&](AstRefDType* refp) {
        ਅਚਲ VIface_Capture_ਟੈਗ* ਅਚਲ tagp = capture_ਟੈਗ(refp);
        ਜੇ (!tagp) ਵਾਪਸ;
        ਅਚਲ Ast_ਗੰਢ_Module* ਅਚਲ ownerModp = ਲੱਭੋ_Owner_Module(refp);
        UINFO(9, "  [" << idx << "] ref=" << refp->name() << " refp=" << cvt_ਮੰਜ਼ਿਲ_Hex(refp)
                       << " ownerMod=" << (ownerModp ? ownerModp->name() : "<null>")
                       << " cellPath='" << tagp->m_cell_ਰਸਤਾ << "' ownerModName='"
                       << tagp->m_ownerModName << "' kind="
                       << (tagp->m_ਕਿਸਮ == VIface_Capture_ਟੈਗ::ਕਿਸਮ_2::PARAM_TYPE ? "param type"
                                                                              : "typedef"));
        ++idx;
    });
    UINFO(9, "========== end iface capture dumpEntries (" << idx << " captured) ==========");
}

ਖਾਲੀ V3LinkDotIfaceCapture::ਟੈਗ(AstRefDType* refp, ਅਚਲ Ast_ਗੰਢ_Module* capturedInp,
                                VIface_Capture_ਟੈਗ::ਕਿਸਮ_2 ਕਿਸਮ, ਅਚਲ string& cell_ਰਸਤਾ,
                                ਅਚਲ string& ownerModName) {
    UASSERT_OBJ(!refp->captureTagp(), refp, "Reference captured twice");
    UASSERT_OBJ(capturedInp, refp, "Captured reference is not in a module");
    UASSERT_OBJ(!cell_ਰਸਤਾ.ਖਾਲੀ_ਥਾਵਾਂ(), refp, "Captured reference has no cell path");
    s_ਟੈਗ_ਸੂਚੀ.emplace_ਪਿੱਠ(VIface_Capture_ਟੈਗ{ਕਿਸਮ, cell_ਰਸਤਾ, ownerModName, capturedInp->origName()});
    refp->captureTagp(&s_ਟੈਗ_ਸੂਚੀ.ਪਿੱਠ());
}

ਅਚਲ VIface_Capture_ਟੈਗ* V3LinkDotIfaceCapture::capture_ਟੈਗ(AstRefDType* refp) {
    ਅਚਲ VIface_Capture_ਟੈਗ* ਅਚਲ tagp = refp->captureTagp();
    ਜੇ (!tagp) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
    ਅਚਲ Ast_ਗੰਢ_Module* ਅਚਲ ownerp = ਲੱਭੋ_Owner_Module(refp);
    ਜੇ (!ownerp || ownerp->origName() != tagp->m_captured_ਵਿੱਚ_Name) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
    ਵਾਪਸ tagp;
}

// Walk a dot-separated cell path through the cell / IFACEREFDTYPE hierarchy
// starting from startModp.  Returns the module at the end of the path, or
// nullptr if any component cannot be resolved.
// Cell/port names are preserved across clones by cloneTree, so this works
// identically on template and cloned modules.
Ast_ਗੰਢ_Module* V3LinkDotIfaceCapture::follow_Cell_ਰਸਤਾ(Ast_ਗੰਢ_Module* ਸ਼ੁਰੂ_Modp,
                                                     ਅਚਲ string& cell_ਰਸਤਾ) {
    ਜੇ (cell_ਰਸਤਾ.ਖਾਲੀ_ਥਾਵਾਂ() || !ਸ਼ੁਰੂ_Modp) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
    Ast_ਗੰਢ_Module* curModp = ਸ਼ੁਰੂ_Modp;
    string ਬਚਿਆ = cell_ਰਸਤਾ;
    ਜਦੋਂ (!ਬਚਿਆ.ਖਾਲੀ_ਥਾਵਾਂ() && curModp) {
        string component;
        ਅਚਲ ਮਾਪ_t dotPos = ਬਚਿਆ.ਲੱਭੋ('.');
        ਜੇ (dotPos == string::npos) {
            component = ਬਚਿਆ;
            ਬਚਿਆ.clear();
        } ਨਹੀਂਤਾਂ {
            component = ਬਚਿਆ.substr(0, dotPos);
            ਬਚਿਆ = ਬਚਿਆ.substr(dotPos + 1);
        }
        // Matched without any array index; the elements of an instance array (see V3Param) all
        // instantiate the same module, so any of them will do
        ਅਚਲ string component_ਅਧਾਰ = Ast_ਗੰਢ::nameNoArray(component);
        Ast_ਗੰਢ_Module* ਅਗਲਾ_Modp = ਖਾਲੀ_ਸੂਚਕ;
        ਲਈ (Ast_ਗੰਢ* sp = curModp->stmtsp(); sp; sp = sp->nextp()) {
            ਜੇ (AstCell* ਅਚਲ cellp = VN_CAST(sp, Cell)) {
                ਜੇ ((cellp->name() == component
                     || Ast_ਗੰਢ::nameNoArray(cellp->name()) == component_ਅਧਾਰ)
                    && cellp->modp()) {
                    ਅਗਲਾ_Modp = cellp->modp();
                    ਰੋਕੋ;
                }
            }
            ਜੇ (AstVar* ਅਚਲ varp = VN_CAST(sp, Var)) {
                ਜੇ (varp->isIfaceRef() && varp->subDTypep()) {
                    string var_ਅਧਾਰ_Name = varp->name();
                    ਅਚਲ ਮਾਪ_t viftopPos = var_ਅਧਾਰ_Name.ਲੱਭੋ("__Viftop");
                    ਜੇ (viftopPos != string::npos) {
                        var_ਅਧਾਰ_Name = var_ਅਧਾਰ_Name.substr(0, viftopPos);
                    }
                    ਜੇ (var_ਅਧਾਰ_Name == component
                        || Ast_ਗੰਢ::nameNoArray(var_ਅਧਾਰ_Name) == component_ਅਧਾਰ) {
                        ਜੇ (AstIfaceRefDType* ਅਚਲ irefp
                            = ifaceRefFromVarDType(varp->subDTypep())) {
                            AstIface* ਅਚਲ ifacep = irefp->iface_ਰਾਹੀਂ_Cellp();
                            ਜੇ (ifacep) {
                                ਅਗਲਾ_Modp = ifacep;
                                ਰੋਕੋ;
                            }
                        }
                    }
                }
            }
        }
        curModp = ਅਗਲਾ_Modp;
    }
    ਵਾਪਸ curModp;
}

// replaces the lambda used in V3LinkDot.cpp for iface capture
ਖਾਲੀ V3LinkDotIfaceCapture::captureTypedefContext(AstRefDType* refp, ਅਚਲ ਅੱਖਰ* stageLabel,
                                                  ਪੂਰਨ dotPos, ਅਚਲ std::string& dot_ਪਾਠ,
                                                  VSymEnt* dotSymp, Ast_ਗੰਢ_Module* modp,
                                                  ਅਚਲ std::function<std::string()>& indentFn) {
    ਜੇ (!enabled() || !refp) ਵਾਪਸ;

    UINFO(9, indentFn() << "iface capture capture request stage=" << stageLabel
                        << " typedef=" << refp << " name=" << refp->name() << " dotPos=" << dotPos
                        << " dotText='" << dot_ਪਾਠ << "' dotSym=" << dotSymp);

    ਅਚਲ AstCell* ifaceCellp = ਖਾਲੀ_ਸੂਚਕ;
    ਜੇ (dotSymp && VN_IS(dotSymp->nodep(), Cell)) {
        ਅਚਲ AstCell* ਅਚਲ cellp = VN_AS(dotSymp->nodep(), Cell);
        ਜੇ (cellp->modp() && VN_IS(cellp->modp(), Iface)) ifaceCellp = cellp;
    }
    ਜੇ (!ifaceCellp) {
        UINFO(9, indentFn() << "iface capture capture skipped typedef=" << refp
                            << " (no iface context)");
        ਵਾਪਸ;
    }
    // Skip internal interface typedef references (typedef used within its own interface)
    ਜੇ (ifaceCellp->modp() == modp) {
        UINFO(9, indentFn() << "iface capture capture skipped typedef=" << refp
                            << " (internal to interface " << modp->name() << ")");
        ਵਾਪਸ;
    }

    // dotText is always non-empty for interface typedef captures.  If this
    // fires, the caller resolved to an interface Cell but did not accumulate
    // a dotText path - investigate the dot-state in visitParseRef.
    UASSERT_OBJ(!dot_ਪਾਠ.ਖਾਲੀ_ਥਾਵਾਂ(), refp, "captureTypedefContext: dotText empty");
    ਅਚਲ string cell_ਰਸਤਾ = dot_ਪਾਠ;

    // A reference to a ParamTypeDType is captured as a parameter type, else as a typedef
    ਜੇ (AstParamTypeDType* ਅਚਲ paramTypep = VN_CAST(refp->refDTypep(), ParamTypeDType)) {
        V3LinkDotIfaceCapture::ਜੋੜੋ_Param_Type(refp, cell_ਰਸਤਾ, modp, paramTypep, "");
    } ਨਹੀਂਤਾਂ {
        ਟੈਗ(refp, modp, VIface_Capture_ਟੈਗ::ਕਿਸਮ_2::TYPEDEF, cell_ਰਸਤਾ,
            resolveOwnerName("", refp->typedefp()));
    }

    UINFO(9, indentFn() << "iface capture capture success typedef=" << refp
                        << " cell=" << ifaceCellp << " cellPath='" << cell_ਰਸਤਾ << "'"
                        << " mod=" << (ifaceCellp->modp() ? ifaceCellp->modp()->name() : "<null>")
                        << " dotPos=" << dotPos);
}

ਖਾਲੀ V3LinkDotIfaceCapture::ਜੋੜੋ_Param_Type(AstRefDType* refp, ਅਚਲ string& cell_ਰਸਤਾ,
                                         Ast_ਗੰਢ_Module* ownerModp, AstParamTypeDType* paramTypep,
                                         ਅਚਲ string& paramTypeOwnerModName) {
    UASSERT(refp, "addParamType() called with null refp");
    UASSERT(ownerModp,
            "addParamType() called with null ownerModp for refp='" << refp->prettyNameQ() << "'");
    UASSERT_OBJ(paramTypep, refp,
                "addParamType() called with null paramTypep for refp='" << refp->prettyNameQ()
                                                                        << "'");
    ਅਚਲ string ptOwnerName = resolveOwnerName(paramTypeOwnerModName, paramTypep);
    UINFO(9, "addParamType: refp=" << refp << " cellPath='" << cell_ਰਸਤਾ << "'"
                                   << " ownerModp=" << (ownerModp ? ownerModp->name() : "<null>")
                                   << " paramTypep=" << paramTypep << " paramTypeOwnerModName='"
                                   << ptOwnerName << "'");
    UINFO(9, "addParamType: paramTypep subDTypep chain:");
    ਜੇ (debug())
        paramTypep->foreach([&](AstRefDType* innerRefp) {
            UINFO(9,
                  "  inner RefDType: "
                      << innerRefp << " refDTypep=" << innerRefp->refDTypep()
                      << (innerRefp->refDTypep() ? " refDTypep->name=" : "")
                      << (innerRefp->refDTypep() ? innerRefp->refDTypep()->prettyTypeName() : ""));
        });
    ਟੈਗ(refp, ownerModp, VIface_Capture_ਟੈਗ::ਕਿਸਮ_2::PARAM_TYPE, cell_ਰਸਤਾ, ptOwnerName);
}

ਪੂਰਨ V3LinkDotIfaceCapture::resolveCapturedRefs() {
    ਪੂਰਨ fixed = 0;

    // TARGET RESOLUTION.  By this point all cloning is complete and cell
    // pointers are wired to the correct interface clones.  For each tagged
    // reference we walk its cell path from its owner module to find the
    // correct target module, then locate the PARAMTYPEDTYPE / TYPEDEF by name.
    // See the ARCHITECTURE comment above for the full picture.

    v3Global.rootp()->foreach([&](AstRefDType* refp) {
        ਅਚਲ VIface_Capture_ਟੈਗ* ਅਚਲ tagp = capture_ਟੈਗ(refp);
        ਜੇ (!tagp) ਵਾਪਸ;
        Ast_ਗੰਢ_Module* ਅਚਲ ownerModp = ਲੱਭੋ_Owner_Module(refp);
        ਜੇ (!ownerModp || ownerModp->dead() || VN_IS(ownerModp, Package)) ਵਾਪਸ;

        UINFO(9, "finalizeIfaceCapture Phase3 entry: refp="
                     << refp->name() << " (" << cvt_ਮੰਜ਼ਿਲ_Hex(refp) << ")" << " ownerMod="
                     << ownerModp->name() << " cellPath='" << tagp->m_cell_ਰਸਤਾ << "' targetKind="
                     << (tagp->m_ਕਿਸਮ == VIface_Capture_ਟੈਗ::ਕਿਸਮ_2::PARAM_TYPE ? "param type"
                                                                            : "typedef"));

        // Prefer the owner itself when its stable template identity matches the
        // captured target owner.  Otherwise resolve and validate the cell path.
        Ast_ਗੰਢ_Module* correctModp = ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (moduleMatchesOwner(ownerModp, tagp->m_ownerModName)) {
            correctModp = ownerModp;
        } ਨਹੀਂਤਾਂ {
            correctModp = follow_Cell_ਰਸਤਾ(ownerModp, tagp->m_cell_ਰਸਤਾ);
            UINFO(9, "  followCellPath('"
                         << ownerModp->name() << "', '" << tagp->m_cell_ਰਸਤਾ
                         << "') = " << (correctModp ? correctModp->name() : "<null>")
                         << (correctModp ? (correctModp->dead() ? " (DEAD)" : " (live)") : ""));
            UASSERT_OBJ(correctModp && !correctModp->dead()
                            && moduleMatchesOwner(correctModp, tagp->m_ownerModName),
                        refp,
                        "captured ref '" << refp->prettyNameQ() << "' cell path '"
                                         << tagp->m_cell_ਰਸਤਾ << "' did not resolve to live owner '"
                                         << tagp->m_ownerModName << "'");
        }
        UASSERT_OBJ(retarget_Ref_ਮੰਜ਼ਿਲ_Module(refp, correctModp), refp,
                    "could not retarget captured "
                        << (tagp->m_ਕਿਸਮ == VIface_Capture_ਟੈਗ::ਕਿਸਮ_2::PARAM_TYPE ? "parameter type "
                                                                               : "typedef ")
                        << refp->prettyNameQ() << " in " << correctModp->prettyNameQ());
        refp->user3(ਸੱਚ);
        ++fixed;
    });

    ਵਾਪਸ fixed;
}

ਖਾਲੀ V3LinkDotIfaceCapture::verifyNoDeadRefs(ਅਚਲ ਜੀਵਤ_Nodes& ਜੀਵਤ_Nodes_2) {
    // Assert: no REFDTYPE in any live module should have typedefp or refDTypep
    // pointing to a dead module.
    ਲਈ (Ast_ਗੰਢ* nodep = v3Global.rootp()->modulesp(); nodep; nodep = nodep->nextp()) {
        ਜੇ (Ast_ਗੰਢ_Module* ਅਚਲ modp = VN_CAST(nodep, ਗੰਢ_Module)) {
            ਜੇ (modp->dead()) ਜਾਰੀ;
            modp->foreach([&](AstRefDType* refp) {
                ਜੇ (refp->typedefp()) {
                    UASSERT_OBJ(ਜੀਵਤ_Nodes_2.ਗਿਣਤੀ_2(refp->typedefp()), refp,
                                "REFDTYPE '" << refp->prettyNameQ() << "' in live module '"
                                             << modp->prettyNameQ()
                                             << "' has a dangling typedefp");
                    Ast_ਗੰਢ_Module* ਅਚਲ ownerModp
                        = ਲੱਭੋ_Owner_Module_If_ਜੀਵਤ(refp->typedefp(), ਜੀਵਤ_Nodes_2);
                    UASSERT_OBJ(!ownerModp || !ownerModp->dead(), refp,
                                "REFDTYPE '" << refp->prettyNameQ() << "' in live module '"
                                             << modp->prettyNameQ()
                                             << "' has typedefp pointing to dead module '"
                                             << ownerModp->prettyNameQ() << "'");
                }
                ਜੇ (refp->refDTypep()) {
                    UASSERT_OBJ(ਜੀਵਤ_Nodes_2.ਗਿਣਤੀ_2(refp->refDTypep()), refp,
                                "REFDTYPE '" << refp->prettyNameQ() << "' in live module '"
                                             << modp->prettyNameQ()
                                             << "' has a dangling refDTypep");
                    Ast_ਗੰਢ_Module* ਅਚਲ ownerModp
                        = ਲੱਭੋ_Owner_Module_If_ਜੀਵਤ(refp->refDTypep(), ਜੀਵਤ_Nodes_2);
                    UASSERT_OBJ(!ownerModp || !ownerModp->dead(), refp,
                                "REFDTYPE '" << refp->prettyNameQ() << "' in live module '"
                                             << modp->prettyNameQ()
                                             << "' has refDTypep pointing to dead module '"
                                             << ownerModp->prettyNameQ() << "'");
                }
            });
        }
    }
    ਜੇ (v3Global.rootp()->typeTablep()) {
        ਲਈ (Ast_ਗੰਢ* nodep = v3Global.rootp()->typeTablep()->typesp(); nodep;
             nodep = nodep->nextp()) {
            nodep->foreach([&](AstRefDType* refp) {
                ਜੇ (refp->typedefp()) {
                    UASSERT_OBJ(ਜੀਵਤ_Nodes_2.ਗਿਣਤੀ_2(refp->typedefp()), refp,
                                "REFDTYPE '" << refp->prettyNameQ()
                                             << "' in type table has a dangling typedefp");
                    Ast_ਗੰਢ_Module* ਅਚਲ ownerModp
                        = ਲੱਭੋ_Owner_Module_If_ਜੀਵਤ(refp->typedefp(), ਜੀਵਤ_Nodes_2);
                    UASSERT_OBJ(!ownerModp || !ownerModp->dead(), refp,
                                "REFDTYPE '"
                                    << refp->prettyNameQ()
                                    << "' in type table has typedefp pointing to dead module '"
                                    << ownerModp->prettyNameQ() << "'");
                }
                ਜੇ (refp->refDTypep()) {
                    UASSERT_OBJ(ਜੀਵਤ_Nodes_2.ਗਿਣਤੀ_2(refp->refDTypep()), refp,
                                "REFDTYPE '" << refp->prettyNameQ()
                                             << "' in type table has a dangling refDTypep");
                    Ast_ਗੰਢ_Module* ਅਚਲ ownerModp
                        = ਲੱਭੋ_Owner_Module_If_ਜੀਵਤ(refp->refDTypep(), ਜੀਵਤ_Nodes_2);
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

ਖਾਲੀ V3LinkDotIfaceCapture::finalizeIfaceCapture() {
    ਜੇ (!s_enabled) ਵਾਪਸ;
    UINFO(4, "finalizeIfaceCapture: fixing remaining cross-interface refs");
    ਜੇ (!v3Global.rootp()) ਵਾਪਸ;
    clearModuleCache();  // Ensure fresh view after all cloning/widthing

    // Resolve live captured refs from stable path metadata before inspecting
    // any inherited target pointers, which may refer to replaced template nodes.
    ਅਚਲ ਪੂਰਨ capturedFixed = resolveCapturedRefs();
    UINFO(4, "finalizeIfaceCapture: structurally resolved " << capturedFixed << " captured refs");

    ਜੇ (debug() >= 9) dumpEntries("after finalizeIfaceCapture");

    // Emit statistics for --stats
    V3Stats::ਜੋੜੋ_Stat("IfaceCapture, Captured refs", s_ਟੈਗ_ਸੂਚੀ.ਮਾਪ());
    V3Stats::ਜੋੜੋ_Stat("IfaceCapture, Captured refs resolved", capturedFixed);

    // No reference may be left pointing into a module about to be removed
    ਜੇ (v3Global.opt.debugCheck()) verifyNoDeadRefs(collect_ਜੀਵਤ_Nodes());
    // Drop every tag before its record is freed.
    v3Global.rootp()->foreach([](AstRefDType* refp) { refp->captureTagp(ਖਾਲੀ_ਸੂਚਕ); });
    reset();
}
