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

#अन्तर्भाव "V3LinkDotIfaceCapture.h"

#अन्तर्भाव "V3Error.h"
#अन्तर्भाव "V3Global.h"
#अन्तर्भाव "V3Stats.h"
#अन्तर्भाव "V3SymTable.h"

#अन्तर्भाव <unordered_map>
#अन्तर्भाव <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

std::deque<VIface_Capture_चिह्नकम्> V3LinkDotIfaceCapture::s_चिह्नकानि{};
तार्किक V3LinkDotIfaceCapture::s_enabled = सत्य;

// LCOV_EXCL_START
शून्य V3LinkDotIfaceCapture::enable(तार्किक flag) {
    s_enabled = flag;
    यदि (!flag) clearModuleCache();
}
// LCOV_EXCL_STOP

शून्य V3LinkDotIfaceCapture::reset() {
    s_चिह्नकानि.clear();
    clearModuleCache();
}

// Per-module cache of statement-level names to avoid O(N*M) linear scans.
// Lazily built on first access for a given module; cleared at phase boundaries.
// Uses vectors per name to handle rare cases where different node types share a name
// (e.g. a Typedef and a ParamTypeDType both named 'sc_tag_status_t').
नामाकाश {
संरचना StmtNameMap final {
    std::unordered_map<string, std::सदिशम्<Ast_ग्रन्थिः*>> m_byName;
};
std::unordered_map<Ast_ग्रन्थिः_Module*, StmtNameMap> s_moduleCache;

ध्रुव StmtNameMap& getOrBuild(Ast_ग्रन्थिः_Module* modp) {
    स्वचालित it = s_moduleCache.find(modp);
    यदि (it != s_moduleCache.अन्तः_2()) प्रत्यागम it->second;
    StmtNameMap& cache = s_moduleCache[modp];
    प्रति (Ast_ग्रन्थिः* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
        ध्रुव string& nm = stmtp->name();
        यदि (!nm.रिक्तानि()) cache.m_byName[nm].प्रक्षिप_पृष्ठभागः(stmtp);
    }
    प्रत्यागम cache;
}
}  // namespace

शून्य V3LinkDotIfaceCapture::clearModuleCache() { s_moduleCache.clear(); }

AstIfaceRefDType* V3LinkDotIfaceCapture::ifaceRefFromVarDType(Ast_ग्रन्थिः_DType* dtypep) {
    AstIfaceRefDType* resultp = शून्यसूचकः;
    प्रति (Ast_ग्रन्थिः_DType* curp = dtypep; curp;) {
        यदि (AstIfaceRefDType* ध्रुव irefp = VN_CAST(curp, IfaceRefDType)) {
            resultp = irefp;
            भङ्ग;
        } अन्यथा यदि (AstBracketArrayDType* ध्रुव bracketp = VN_CAST(curp, BracketArrayDType)) {
            curp = bracketp->subDTypep();
        } अन्यथा यदि (AstUnpackArrayDType* ध्रुव unpackp = VN_CAST(curp, UnpackArrayDType)) {
            curp = unpackp->subDTypep();
        } अन्यथा {
            v3fatalSrc("ifaceRefFromVarDType: unexpected dtype " << curp->prettyTypeName()
                                                                 << " in chain");
        }
    }
    प्रत्यागम resultp;
}

नामाकाश {
// Resolve the owner module name for a typedef/paramType node.
// Returns hint if non-empty, otherwise walks backp() to find the owner module name.
string resolveOwnerName(ध्रुव string& hint, Ast_ग्रन्थिः* nodep) {
    यदि (!hint.रिक्तानि()) प्रत्यागम hint;
    यदि (!nodep) प्रत्यागम "";
    Ast_ग्रन्थिः_Module* ध्रुव ownerp = V3LinkDotIfaceCapture::findOwnerModule(nodep);
    प्रत्यागम ownerp ? ownerp->name() : string{};
}
}  // namespace

AstTypedef* V3LinkDotIfaceCapture::find_Typedef_अन्तर्_Module(Ast_ग्रन्थिः_Module* modp, ध्रुव string& name) {
    AstTypedef* resultp = शून्यसूचकः;
    ध्रुव StmtNameMap& cache = getOrBuild(modp);
    ध्रुव स्वचालित it = cache.m_byName.find(name);
    यदि (!(it == cache.m_byName.अन्तः_2())) {
        प्रति (Ast_ग्रन्थिः* nodep : it->second) {
            यदि (AstTypedef* ध्रुव tdp = VN_CAST(nodep, Typedef)) {
                resultp = tdp;
                भङ्ग;
            }
        }
    }
    प्रत्यागम resultp;
}
Ast_ग्रन्थिः_DType* V3LinkDotIfaceCapture::find_DType_अन्तर्_Module(Ast_ग्रन्थिः_Module* modp, ध्रुव string& name,
                                                       VNType type) {

    Ast_ग्रन्थिः_DType* resultp = शून्यसूचकः;
    ध्रुव StmtNameMap& cache = getOrBuild(modp);
    ध्रुव स्वचालित it = cache.m_byName.find(name);
    यदि (!(it == cache.m_byName.अन्तः_2())) {
        प्रति (Ast_ग्रन्थिः* nodep : it->second) {
            यदि (Ast_ग्रन्थिः_DType* ध्रुव dtp = VN_CAST(nodep, ग्रन्थिः_DType)) {
                यदि (dtp->type() == type) {
                    resultp = dtp;
                    भङ्ग;
                }
            }
        }
    }
    प्रत्यागम resultp;
}
AstParamTypeDType* V3LinkDotIfaceCapture::find_Param_Type_अन्तर्_Module(Ast_ग्रन्थिः_Module* modp,
                                                                ध्रुव string& name) {

    AstParamTypeDType* resultp = शून्यसूचकः;
    ध्रुव StmtNameMap& cache = getOrBuild(modp);
    ध्रुव स्वचालित it = cache.m_byName.find(name);
    यदि (!(it == cache.m_byName.अन्तः_2())) {
        प्रति (Ast_ग्रन्थिः* nodep : it->second) {
            यदि (AstParamTypeDType* ध्रुव ptdp = VN_CAST(nodep, ParamTypeDType)) {
                resultp = ptdp;
                भङ्ग;
            }
        }
    }
    प्रत्यागम resultp;
}

तार्किक V3LinkDotIfaceCapture::retarget_Ref_गन्तव्यम्_Module(AstRefDType* refp, Ast_ग्रन्थिः_Module* targetModp) {
    ध्रुव VIface_Capture_चिह्नकम्* ध्रुव tagp = refp->captureTagp();
    UASSERT_OBJ(tagp, refp, "Retarget of a reference that was not captured");
    UASSERT_OBJ(targetModp, refp, "Retarget to a null module");

    यदि (tagp->m_विधा == VIface_Capture_चिह्नकम्::विधा_2::PARAM_TYPE) {
        AstParamTypeDType* ध्रुव paramTypep = find_Param_Type_अन्तर्_Module(targetModp, refp->name());
        यदि (!paramTypep) प्रत्यागम असत्य;
        retarget_Ref_गन्तव्यम्_Param_Type(refp, paramTypep);
        प्रत्यागम सत्य;
    }

    AstTypedef* ध्रुव typedefp = find_Typedef_अन्तर्_Module(targetModp, refp->name());
    यदि (!typedefp) प्रत्यागम असत्य;
    retarget_Ref_गन्तव्यम्_Typedef(refp, typedefp);
    प्रत्यागम सत्य;
}

शून्य V3LinkDotIfaceCapture::retarget_Ref_गन्तव्यम्_Param_Type(AstRefDType* refp,
                                                   AstParamTypeDType* paramTypep) {
    UASSERT_OBJ(paramTypep, refp, "Retarget to a null parameter type");
    refp->refDTypep(paramTypep);
    refp->dtypep(paramTypep);
}

शून्य V3LinkDotIfaceCapture::retarget_Ref_गन्तव्यम्_Typedef(AstRefDType* refp, AstTypedef* typedefp) {
    UASSERT_OBJ(typedefp, refp, "Retarget to a null typedef");
    refp->typedefp(typedefp);
    // An incomplete typedef is still a successful name resolution, but
    // must not erase type links that a later width pass can complete.
    यदि (Ast_ग्रन्थिः_DType* ध्रुव dtypep = typedefp->subDTypep()) {
        refp->refDTypep(dtypep);
        refp->dtypep(dtypep);
    }
}

नामाकाश {
उपयुज्य जीवितम्_Nodes = std::unordered_समुच्चयः<ध्रुव Ast_ग्रन्थिः*>;

// A scoped snapshot of every node currently in the tree.  V3Broken::isLinkable()
// cannot serve this role: its table is populated only while V3Broken::brokenAll()
// runs and is cleared before it returns.
जीवितम्_Nodes collect_जीवितम्_Nodes() {
    जीवितम्_Nodes जीवितम्_Nodes_2;
    v3Global.rootp()->foreach([&](Ast_ग्रन्थिः* nodep) { जीवितम्_Nodes_2.निवेशय(nodep); });
    प्रत्यागम जीवितम्_Nodes_2;
}

// Find the module that owns this node; a snapshot, if given, stops the walk at a stale link.
Ast_ग्रन्थिः_Module* findOwnerModuleImpl(Ast_ग्रन्थिः* nodep, ध्रुव जीवितम्_Nodes* जीवितम्_Nodesp) {
    प्रति (Ast_ग्रन्थिः* curp = nodep; curp; curp = curp->backp()) {
        यदि (जीवितम्_Nodesp && !जीवितम्_Nodesp->गणनम्(curp)) प्रत्यागम शून्यसूचकः;
        यदि (Ast_ग्रन्थिः_Module* ध्रुव modp = VN_CAST(curp, ग्रन्थिः_Module)) प्रत्यागम modp;
    }
    प्रत्यागम शून्यसूचकः;
}

Ast_ग्रन्थिः_Module* find_Owner_Module_If_जीवितम्(Ast_ग्रन्थिः* nodep, ध्रुव जीवितम्_Nodes& जीवितम्_Nodes_2) {
    प्रत्यागम findOwnerModuleImpl(nodep, &जीवितम्_Nodes_2);
}

तार्किक moduleMatchesOwner(ध्रुव Ast_ग्रन्थिः_Module* modp, ध्रुव string& ownerName) {
    यदि (!modp || ownerName.रिक्तानि()) प्रत्यागम असत्य;
    प्रत्यागम modp->name() == ownerName || modp->origName() == ownerName;
}
}  // namespace

Ast_ग्रन्थिः_Module* V3LinkDotIfaceCapture::findOwnerModule(Ast_ग्रन्थिः* nodep) {
    प्रत्यागम findOwnerModuleImpl(nodep, शून्यसूचकः);
}

शून्य V3LinkDotIfaceCapture::dumpEntries(ध्रुव string& label) {
    UINFO(9, "========== iface capture dumpEntries: " << label << " ==========");
    पूर्णाङ्क idx = 0;
    v3Global.rootp()->foreach([&](AstRefDType* refp) {
        ध्रुव VIface_Capture_चिह्नकम्* ध्रुव tagp = capture_चिह्नकम्(refp);
        यदि (!tagp) प्रत्यागम;
        ध्रुव Ast_ग्रन्थिः_Module* ध्रुव ownerModp = findOwnerModule(refp);
        UINFO(9, "  [" << idx << "] ref=" << refp->name() << " refp=" << cvt_गन्तव्यम्_Hex(refp)
                       << " ownerMod=" << (ownerModp ? ownerModp->name() : "<null>")
                       << " cellPath='" << tagp->m_cell_मार्गः << "' ownerModName='"
                       << tagp->m_ownerModName << "' kind="
                       << (tagp->m_विधा == VIface_Capture_चिह्नकम्::विधा_2::PARAM_TYPE ? "param type"
                                                                              : "typedef"));
        ++idx;
    });
    UINFO(9, "========== end iface capture dumpEntries (" << idx << " captured) ==========");
}

शून्य V3LinkDotIfaceCapture::चिह्नकम्(AstRefDType* refp, ध्रुव Ast_ग्रन्थिः_Module* capturedInp,
                                VIface_Capture_चिह्नकम्::विधा_2 विधा, ध्रुव string& cell_मार्गः,
                                ध्रुव string& ownerModName) {
    UASSERT_OBJ(!refp->captureTagp(), refp, "Reference captured twice");
    UASSERT_OBJ(capturedInp, refp, "Captured reference is not in a module");
    UASSERT_OBJ(!cell_मार्गः.रिक्तानि(), refp, "Captured reference has no cell path");
    s_चिह्नकानि.emplace_पृष्ठभागः(VIface_Capture_चिह्नकम्{विधा, cell_मार्गः, ownerModName, capturedInp->origName()});
    refp->captureTagp(&s_चिह्नकानि.पृष्ठभागः());
}

ध्रुव VIface_Capture_चिह्नकम्* V3LinkDotIfaceCapture::capture_चिह्नकम्(AstRefDType* refp) {
    ध्रुव VIface_Capture_चिह्नकम्* ध्रुव tagp = refp->captureTagp();
    यदि (!tagp) प्रत्यागम शून्यसूचकः;
    ध्रुव Ast_ग्रन्थिः_Module* ध्रुव ownerp = findOwnerModule(refp);
    यदि (!ownerp || ownerp->origName() != tagp->m_captured_अन्तर्_Name) प्रत्यागम शून्यसूचकः;
    प्रत्यागम tagp;
}

// Walk a dot-separated cell path through the cell / IFACEREFDTYPE hierarchy
// starting from startModp.  Returns the module at the end of the path, or
// nullptr if any component cannot be resolved.
// Cell/port names are preserved across clones by cloneTree, so this works
// identically on template and cloned modules.
Ast_ग्रन्थिः_Module* V3LinkDotIfaceCapture::follow_Cell_मार्गः(Ast_ग्रन्थिः_Module* startModp,
                                                     ध्रुव string& cell_मार्गः) {
    यदि (cell_मार्गः.रिक्तानि() || !startModp) प्रत्यागम शून्यसूचकः;
    Ast_ग्रन्थिः_Module* curModp = startModp;
    string remaining = cell_मार्गः;
    यावत् (!remaining.रिक्तानि() && curModp) {
        string component;
        ध्रुव आकार_t dotPos = remaining.find('.');
        यदि (dotPos == string::npos) {
            component = remaining;
            remaining.clear();
        } अन्यथा {
            component = remaining.substr(0, dotPos);
            remaining = remaining.substr(dotPos + 1);
        }
        // Matched without any array index; the elements of an instance array (see V3Param) all
        // instantiate the same module, so any of them will do
        ध्रुव string component_आधारः = Ast_ग्रन्थिः::nameNoArray(component);
        Ast_ग्रन्थिः_Module* अग्र_Modp = शून्यसूचकः;
        प्रति (Ast_ग्रन्थिः* sp = curModp->stmtsp(); sp; sp = sp->nextp()) {
            यदि (AstCell* ध्रुव cellp = VN_CAST(sp, Cell)) {
                यदि ((cellp->name() == component
                     || Ast_ग्रन्थिः::nameNoArray(cellp->name()) == component_आधारः)
                    && cellp->modp()) {
                    अग्र_Modp = cellp->modp();
                    भङ्ग;
                }
            }
            यदि (AstVar* ध्रुव varp = VN_CAST(sp, Var)) {
                यदि (varp->isIfaceRef() && varp->subDTypep()) {
                    string var_आधारः_Name = varp->name();
                    ध्रुव आकार_t viftopPos = var_आधारः_Name.find("__Viftop");
                    यदि (viftopPos != string::npos) {
                        var_आधारः_Name = var_आधारः_Name.substr(0, viftopPos);
                    }
                    यदि (var_आधारः_Name == component
                        || Ast_ग्रन्थिः::nameNoArray(var_आधारः_Name) == component_आधारः) {
                        यदि (AstIfaceRefDType* ध्रुव irefp
                            = ifaceRefFromVarDType(varp->subDTypep())) {
                            AstIface* ध्रुव ifacep = irefp->iface_द्वारा_Cellp();
                            यदि (ifacep) {
                                अग्र_Modp = ifacep;
                                भङ्ग;
                            }
                        }
                    }
                }
            }
        }
        curModp = अग्र_Modp;
    }
    प्रत्यागम curModp;
}

// replaces the lambda used in V3LinkDot.cpp for iface capture
शून्य V3LinkDotIfaceCapture::captureTypedefContext(AstRefDType* refp, ध्रुव वर्ण* stageLabel,
                                                  पूर्णाङ्क dotPos, ध्रुव std::string& dotText,
                                                  VSymEnt* dotSymp, Ast_ग्रन्थिः_Module* modp,
                                                  ध्रुव std::function<std::string()>& indentFn) {
    यदि (!enabled() || !refp) प्रत्यागम;

    UINFO(9, indentFn() << "iface capture capture request stage=" << stageLabel
                        << " typedef=" << refp << " name=" << refp->name() << " dotPos=" << dotPos
                        << " dotText='" << dotText << "' dotSym=" << dotSymp);

    ध्रुव AstCell* ifaceCellp = शून्यसूचकः;
    यदि (dotSymp && VN_IS(dotSymp->nodep(), Cell)) {
        ध्रुव AstCell* ध्रुव cellp = VN_AS(dotSymp->nodep(), Cell);
        यदि (cellp->modp() && VN_IS(cellp->modp(), Iface)) ifaceCellp = cellp;
    }
    यदि (!ifaceCellp) {
        UINFO(9, indentFn() << "iface capture capture skipped typedef=" << refp
                            << " (no iface context)");
        प्रत्यागम;
    }
    // Skip internal interface typedef references (typedef used within its own interface)
    यदि (ifaceCellp->modp() == modp) {
        UINFO(9, indentFn() << "iface capture capture skipped typedef=" << refp
                            << " (internal to interface " << modp->name() << ")");
        प्रत्यागम;
    }

    // dotText is always non-empty for interface typedef captures.  If this
    // fires, the caller resolved to an interface Cell but did not accumulate
    // a dotText path - investigate the dot-state in visitParseRef.
    UASSERT_OBJ(!dotText.रिक्तानि(), refp, "captureTypedefContext: dotText empty");
    ध्रुव string cell_मार्गः = dotText;

    // A reference to a ParamTypeDType is captured as a parameter type, else as a typedef
    यदि (AstParamTypeDType* ध्रुव paramTypep = VN_CAST(refp->refDTypep(), ParamTypeDType)) {
        V3LinkDotIfaceCapture::सङ्कलय_Param_Type(refp, cell_मार्गः, modp, paramTypep, "");
    } अन्यथा {
        चिह्नकम्(refp, modp, VIface_Capture_चिह्नकम्::विधा_2::TYPEDEF, cell_मार्गः,
            resolveOwnerName("", refp->typedefp()));
    }

    UINFO(9, indentFn() << "iface capture capture success typedef=" << refp
                        << " cell=" << ifaceCellp << " cellPath='" << cell_मार्गः << "'"
                        << " mod=" << (ifaceCellp->modp() ? ifaceCellp->modp()->name() : "<null>")
                        << " dotPos=" << dotPos);
}

शून्य V3LinkDotIfaceCapture::सङ्कलय_Param_Type(AstRefDType* refp, ध्रुव string& cell_मार्गः,
                                         Ast_ग्रन्थिः_Module* ownerModp, AstParamTypeDType* paramTypep,
                                         ध्रुव string& paramTypeOwnerModName) {
    UASSERT(refp, "addParamType() called with null refp");
    UASSERT(ownerModp,
            "addParamType() called with null ownerModp for refp='" << refp->prettyNameQ() << "'");
    UASSERT_OBJ(paramTypep, refp,
                "addParamType() called with null paramTypep for refp='" << refp->prettyNameQ()
                                                                        << "'");
    ध्रुव string ptOwnerName = resolveOwnerName(paramTypeOwnerModName, paramTypep);
    UINFO(9, "addParamType: refp=" << refp << " cellPath='" << cell_मार्गः << "'"
                                   << " ownerModp=" << (ownerModp ? ownerModp->name() : "<null>")
                                   << " paramTypep=" << paramTypep << " paramTypeOwnerModName='"
                                   << ptOwnerName << "'");
    UINFO(9, "addParamType: paramTypep subDTypep chain:");
    यदि (debug())
        paramTypep->foreach([&](AstRefDType* innerRefp) {
            UINFO(9,
                  "  inner RefDType: "
                      << innerRefp << " refDTypep=" << innerRefp->refDTypep()
                      << (innerRefp->refDTypep() ? " refDTypep->name=" : "")
                      << (innerRefp->refDTypep() ? innerRefp->refDTypep()->prettyTypeName() : ""));
        });
    चिह्नकम्(refp, ownerModp, VIface_Capture_चिह्नकम्::विधा_2::PARAM_TYPE, cell_मार्गः, ptOwnerName);
}

पूर्णाङ्क V3LinkDotIfaceCapture::resolveCapturedRefs() {
    पूर्णाङ्क fixed = 0;

    // TARGET RESOLUTION.  By this point all cloning is complete and cell
    // pointers are wired to the correct interface clones.  For each tagged
    // reference we walk its cell path from its owner module to find the
    // correct target module, then locate the PARAMTYPEDTYPE / TYPEDEF by name.
    // See the ARCHITECTURE comment above for the full picture.

    v3Global.rootp()->foreach([&](AstRefDType* refp) {
        ध्रुव VIface_Capture_चिह्नकम्* ध्रुव tagp = capture_चिह्नकम्(refp);
        यदि (!tagp) प्रत्यागम;
        Ast_ग्रन्थिः_Module* ध्रुव ownerModp = findOwnerModule(refp);
        यदि (!ownerModp || ownerModp->dead() || VN_IS(ownerModp, Package)) प्रत्यागम;

        UINFO(9, "finalizeIfaceCapture Phase3 entry: refp="
                     << refp->name() << " (" << cvt_गन्तव्यम्_Hex(refp) << ")" << " ownerMod="
                     << ownerModp->name() << " cellPath='" << tagp->m_cell_मार्गः << "' targetKind="
                     << (tagp->m_विधा == VIface_Capture_चिह्नकम्::विधा_2::PARAM_TYPE ? "param type"
                                                                            : "typedef"));

        // Prefer the owner itself when its stable template identity matches the
        // captured target owner.  Otherwise resolve and validate the cell path.
        Ast_ग्रन्थिः_Module* correctModp = शून्यसूचकः;
        यदि (moduleMatchesOwner(ownerModp, tagp->m_ownerModName)) {
            correctModp = ownerModp;
        } अन्यथा {
            correctModp = follow_Cell_मार्गः(ownerModp, tagp->m_cell_मार्गः);
            UINFO(9, "  followCellPath('"
                         << ownerModp->name() << "', '" << tagp->m_cell_मार्गः
                         << "') = " << (correctModp ? correctModp->name() : "<null>")
                         << (correctModp ? (correctModp->dead() ? " (DEAD)" : " (live)") : ""));
            UASSERT_OBJ(correctModp && !correctModp->dead()
                            && moduleMatchesOwner(correctModp, tagp->m_ownerModName),
                        refp,
                        "captured ref '" << refp->prettyNameQ() << "' cell path '"
                                         << tagp->m_cell_मार्गः << "' did not resolve to live owner '"
                                         << tagp->m_ownerModName << "'");
        }
        UASSERT_OBJ(retarget_Ref_गन्तव्यम्_Module(refp, correctModp), refp,
                    "could not retarget captured "
                        << (tagp->m_विधा == VIface_Capture_चिह्नकम्::विधा_2::PARAM_TYPE ? "parameter type "
                                                                               : "typedef ")
                        << refp->prettyNameQ() << " in " << correctModp->prettyNameQ());
        refp->user3(सत्य);
        ++fixed;
    });

    प्रत्यागम fixed;
}

शून्य V3LinkDotIfaceCapture::verifyNoDeadRefs(ध्रुव जीवितम्_Nodes& जीवितम्_Nodes_2) {
    // Assert: no REFDTYPE in any live module should have typedefp or refDTypep
    // pointing to a dead module.
    प्रति (Ast_ग्रन्थिः* nodep = v3Global.rootp()->modulesp(); nodep; nodep = nodep->nextp()) {
        यदि (Ast_ग्रन्थिः_Module* ध्रुव modp = VN_CAST(nodep, ग्रन्थिः_Module)) {
            यदि (modp->dead()) अनुवर्त;
            modp->foreach([&](AstRefDType* refp) {
                यदि (refp->typedefp()) {
                    UASSERT_OBJ(जीवितम्_Nodes_2.गणनम्(refp->typedefp()), refp,
                                "REFDTYPE '" << refp->prettyNameQ() << "' in live module '"
                                             << modp->prettyNameQ()
                                             << "' has a dangling typedefp");
                    Ast_ग्रन्थिः_Module* ध्रुव ownerModp
                        = find_Owner_Module_If_जीवितम्(refp->typedefp(), जीवितम्_Nodes_2);
                    UASSERT_OBJ(!ownerModp || !ownerModp->dead(), refp,
                                "REFDTYPE '" << refp->prettyNameQ() << "' in live module '"
                                             << modp->prettyNameQ()
                                             << "' has typedefp pointing to dead module '"
                                             << ownerModp->prettyNameQ() << "'");
                }
                यदि (refp->refDTypep()) {
                    UASSERT_OBJ(जीवितम्_Nodes_2.गणनम्(refp->refDTypep()), refp,
                                "REFDTYPE '" << refp->prettyNameQ() << "' in live module '"
                                             << modp->prettyNameQ()
                                             << "' has a dangling refDTypep");
                    Ast_ग्रन्थिः_Module* ध्रुव ownerModp
                        = find_Owner_Module_If_जीवितम्(refp->refDTypep(), जीवितम्_Nodes_2);
                    UASSERT_OBJ(!ownerModp || !ownerModp->dead(), refp,
                                "REFDTYPE '" << refp->prettyNameQ() << "' in live module '"
                                             << modp->prettyNameQ()
                                             << "' has refDTypep pointing to dead module '"
                                             << ownerModp->prettyNameQ() << "'");
                }
            });
        }
    }
    यदि (v3Global.rootp()->typeTablep()) {
        प्रति (Ast_ग्रन्थिः* nodep = v3Global.rootp()->typeTablep()->typesp(); nodep;
             nodep = nodep->nextp()) {
            nodep->foreach([&](AstRefDType* refp) {
                यदि (refp->typedefp()) {
                    UASSERT_OBJ(जीवितम्_Nodes_2.गणनम्(refp->typedefp()), refp,
                                "REFDTYPE '" << refp->prettyNameQ()
                                             << "' in type table has a dangling typedefp");
                    Ast_ग्रन्थिः_Module* ध्रुव ownerModp
                        = find_Owner_Module_If_जीवितम्(refp->typedefp(), जीवितम्_Nodes_2);
                    UASSERT_OBJ(!ownerModp || !ownerModp->dead(), refp,
                                "REFDTYPE '"
                                    << refp->prettyNameQ()
                                    << "' in type table has typedefp pointing to dead module '"
                                    << ownerModp->prettyNameQ() << "'");
                }
                यदि (refp->refDTypep()) {
                    UASSERT_OBJ(जीवितम्_Nodes_2.गणनम्(refp->refDTypep()), refp,
                                "REFDTYPE '" << refp->prettyNameQ()
                                             << "' in type table has a dangling refDTypep");
                    Ast_ग्रन्थिः_Module* ध्रुव ownerModp
                        = find_Owner_Module_If_जीवितम्(refp->refDTypep(), जीवितम्_Nodes_2);
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

शून्य V3LinkDotIfaceCapture::finalizeIfaceCapture() {
    यदि (!s_enabled) प्रत्यागम;
    UINFO(4, "finalizeIfaceCapture: fixing remaining cross-interface refs");
    यदि (!v3Global.rootp()) प्रत्यागम;
    clearModuleCache();  // Ensure fresh view after all cloning/widthing

    // Resolve live captured refs from stable path metadata before inspecting
    // any inherited target pointers, which may refer to replaced template nodes.
    ध्रुव पूर्णाङ्क capturedFixed = resolveCapturedRefs();
    UINFO(4, "finalizeIfaceCapture: structurally resolved " << capturedFixed << " captured refs");

    यदि (debug() >= 9) dumpEntries("after finalizeIfaceCapture");

    // Emit statistics for --stats
    V3Stats::सङ्कलय_Stat("IfaceCapture, Captured refs", s_चिह्नकानि.आकार());
    V3Stats::सङ्कलय_Stat("IfaceCapture, Captured refs resolved", capturedFixed);

    // No reference may be left pointing into a module about to be removed
    यदि (v3Global.opt.debugCheck()) verifyNoDeadRefs(collect_जीवितम्_Nodes());
    // Drop every tag before its record is freed.
    v3Global.rootp()->foreach([](AstRefDType* refp) { refp->captureTagp(शून्यसूचकः); });
    reset();
}
