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

#అన్తర్భావ "V3LinkDotIfaceCapture.h"

#అన్తర్భావ "V3Error.h"
#అన్తర్భావ "V3Global.h"
#అన్తర్భావ "V3Stats.h"
#అన్తర్భావ "V3SymTable.h"

#అన్తర్భావ <unordered_map>
#అన్తర్భావ <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

std::deque<VIface_Capture_ట్యాగ్> V3LinkDotIfaceCapture::s_ట్యాగులు{};
తార్కిక V3LinkDotIfaceCapture::s_enabled = నిజం;

// LCOV_EXCL_START
శూన్య V3LinkDotIfaceCapture::enable(తార్కిక flag) {
    s_enabled = flag;
    ఒకవేళ (!flag) clearModuleCache();
}
// LCOV_EXCL_STOP

శూన్య V3LinkDotIfaceCapture::reset() {
    s_ట్యాగులు.clear();
    clearModuleCache();
}

// Per-module cache of statement-level names to avoid O(N*M) linear scans.
// Lazily built on first access for a given module; cleared at phase boundaries.
// Uses vectors per name to handle rare cases where different node types share a name
// (e.g. a Typedef and a ParamTypeDType both named 'sc_tag_status_t').
నామాకాశ {
నిర్మాణం StmtNameMap final {
    std::unordered_map<string, std::సదిశ<Ast_కణుపు*>> m_byName;
};
std::unordered_map<Ast_కణుపు_Module*, StmtNameMap> s_moduleCache;

స్థిరరాశి StmtNameMap& getOrBuild(Ast_కణుపు_Module* modp) {
    స్వయంచాలక it = s_moduleCache.కనుగొను(modp);
    ఒకవేళ (it != s_moduleCache.చివర()) తిరిగి it->రెండవ;
    StmtNameMap& cache = s_moduleCache[modp];
    కొరకు (Ast_కణుపు* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
        స్థిరరాశి string& nm = stmtp->name();
        ఒకవేళ (!nm.ఖాళీలు()) cache.m_byName[nm].నెట్టు_వెనుక(stmtp);
    }
    తిరిగి cache;
}
}  // namespace

శూన్య V3LinkDotIfaceCapture::clearModuleCache() { s_moduleCache.clear(); }

AstIfaceRefDType* V3LinkDotIfaceCapture::ifaceRefFromVarDType(Ast_కణుపు_DType* dtypep) {
    AstIfaceRefDType* resultp = శూన్యసూచిక;
    కొరకు (Ast_కణుపు_DType* curp = dtypep; curp;) {
        ఒకవేళ (AstIfaceRefDType* స్థిరరాశి irefp = VN_CAST(curp, IfaceRefDType)) {
            resultp = irefp;
            ఆపు;
        } లేకపోతే ఒకవేళ (AstBracketArrayDType* స్థిరరాశి bracketp = VN_CAST(curp, BracketArrayDType)) {
            curp = bracketp->subDTypep();
        } లేకపోతే ఒకవేళ (AstUnpackArrayDType* స్థిరరాశి unpackp = VN_CAST(curp, UnpackArrayDType)) {
            curp = unpackp->subDTypep();
        } లేకపోతే {
            v3fatalSrc("ifaceRefFromVarDType: unexpected dtype " << curp->prettyTypeName()
                                                                 << " in chain");
        }
    }
    తిరిగి resultp;
}

నామాకాశ {
// Resolve the owner module name for a typedef/paramType node.
// Returns hint if non-empty, otherwise walks backp() to find the owner module name.
string resolveOwnerName(స్థిరరాశి string& hint, Ast_కణుపు* nodep) {
    ఒకవేళ (!hint.ఖాళీలు()) తిరిగి hint;
    ఒకవేళ (!nodep) తిరిగి "";
    Ast_కణుపు_Module* స్థిరరాశి ownerp = V3LinkDotIfaceCapture::కనుగొను_Owner_Module(nodep);
    తిరిగి ownerp ? ownerp->name() : string{};
}
}  // namespace

AstTypedef* V3LinkDotIfaceCapture::కనుగొను_Typedef_లోపల_Module(Ast_కణుపు_Module* modp, స్థిరరాశి string& name) {
    AstTypedef* resultp = శూన్యసూచిక;
    స్థిరరాశి StmtNameMap& cache = getOrBuild(modp);
    స్థిరరాశి స్వయంచాలక it = cache.m_byName.కనుగొను(name);
    ఒకవేళ (!(it == cache.m_byName.చివర())) {
        కొరకు (Ast_కణుపు* nodep : it->రెండవ) {
            ఒకవేళ (AstTypedef* స్థిరరాశి tdp = VN_CAST(nodep, Typedef)) {
                resultp = tdp;
                ఆపు;
            }
        }
    }
    తిరిగి resultp;
}
Ast_కణుపు_DType* V3LinkDotIfaceCapture::కనుగొను_DType_లోపల_Module(Ast_కణుపు_Module* modp, స్థిరరాశి string& name,
                                                       VNType type) {

    Ast_కణుపు_DType* resultp = శూన్యసూచిక;
    స్థిరరాశి StmtNameMap& cache = getOrBuild(modp);
    స్థిరరాశి స్వయంచాలక it = cache.m_byName.కనుగొను(name);
    ఒకవేళ (!(it == cache.m_byName.చివర())) {
        కొరకు (Ast_కణుపు* nodep : it->రెండవ) {
            ఒకవేళ (Ast_కణుపు_DType* స్థిరరాశి dtp = VN_CAST(nodep, కణుపు_DType)) {
                ఒకవేళ (dtp->type() == type) {
                    resultp = dtp;
                    ఆపు;
                }
            }
        }
    }
    తిరిగి resultp;
}
AstParamTypeDType* V3LinkDotIfaceCapture::కనుగొను_Param_Type_లోపల_Module(Ast_కణుపు_Module* modp,
                                                                స్థిరరాశి string& name) {

    AstParamTypeDType* resultp = శూన్యసూచిక;
    స్థిరరాశి StmtNameMap& cache = getOrBuild(modp);
    స్థిరరాశి స్వయంచాలక it = cache.m_byName.కనుగొను(name);
    ఒకవేళ (!(it == cache.m_byName.చివర())) {
        కొరకు (Ast_కణుపు* nodep : it->రెండవ) {
            ఒకవేళ (AstParamTypeDType* స్థిరరాశి ptdp = VN_CAST(nodep, ParamTypeDType)) {
                resultp = ptdp;
                ఆపు;
            }
        }
    }
    తిరిగి resultp;
}

తార్కిక V3LinkDotIfaceCapture::retarget_Ref_గమ్యం_Module(AstRefDType* refp, Ast_కణుపు_Module* లక్ష్యం_Modp) {
    స్థిరరాశి VIface_Capture_ట్యాగ్* స్థిరరాశి tagp = refp->captureTagp();
    UASSERT_OBJ(tagp, refp, "Retarget of a reference that was not captured");
    UASSERT_OBJ(లక్ష్యం_Modp, refp, "Retarget to a null module");

    ఒకవేళ (tagp->m_రకం == VIface_Capture_ట్యాగ్::రకం_2::PARAM_TYPE) {
        AstParamTypeDType* స్థిరరాశి paramTypep = కనుగొను_Param_Type_లోపల_Module(లక్ష్యం_Modp, refp->name());
        ఒకవేళ (!paramTypep) తిరిగి అబద్ధం;
        retarget_Ref_గమ్యం_Param_Type(refp, paramTypep);
        తిరిగి నిజం;
    }

    AstTypedef* స్థిరరాశి typedefp = కనుగొను_Typedef_లోపల_Module(లక్ష్యం_Modp, refp->name());
    ఒకవేళ (!typedefp) తిరిగి అబద్ధం;
    retarget_Ref_గమ్యం_Typedef(refp, typedefp);
    తిరిగి నిజం;
}

శూన్య V3LinkDotIfaceCapture::retarget_Ref_గమ్యం_Param_Type(AstRefDType* refp,
                                                   AstParamTypeDType* paramTypep) {
    UASSERT_OBJ(paramTypep, refp, "Retarget to a null parameter type");
    refp->refDTypep(paramTypep);
    refp->dtypep(paramTypep);
}

శూన్య V3LinkDotIfaceCapture::retarget_Ref_గమ్యం_Typedef(AstRefDType* refp, AstTypedef* typedefp) {
    UASSERT_OBJ(typedefp, refp, "Retarget to a null typedef");
    refp->typedefp(typedefp);
    // An incomplete typedef is still a successful name resolution, but
    // must not erase type links that a later width pass can complete.
    ఒకవేళ (Ast_కణుపు_DType* స్థిరరాశి dtypep = typedefp->subDTypep()) {
        refp->refDTypep(dtypep);
        refp->dtypep(dtypep);
    }
}

నామాకాశ {
ఉపయోగించి సజీవ_Nodes = std::unordered_సమితి<స్థిరరాశి Ast_కణుపు*>;

// A scoped snapshot of every node currently in the tree.  V3Broken::isLinkable()
// cannot serve this role: its table is populated only while V3Broken::brokenAll()
// runs and is cleared before it returns.
సజీవ_Nodes collect_సజీవ_Nodes() {
    సజీవ_Nodes సజీవ_Nodes_2;
    v3Global.rootp()->foreach([&](Ast_కణుపు* nodep) { సజీవ_Nodes_2.చొప్పించు(nodep); });
    తిరిగి సజీవ_Nodes_2;
}

// Find the module that owns this node; a snapshot, if given, stops the walk at a stale link.
Ast_కణుపు_Module* కనుగొను_Owner_Module_Impl(Ast_కణుపు* nodep, స్థిరరాశి సజీవ_Nodes* సజీవ_Nodesp) {
    కొరకు (Ast_కణుపు* curp = nodep; curp; curp = curp->backp()) {
        ఒకవేళ (సజీవ_Nodesp && !సజీవ_Nodesp->లెక్క(curp)) తిరిగి శూన్యసూచిక;
        ఒకవేళ (Ast_కణుపు_Module* స్థిరరాశి modp = VN_CAST(curp, కణుపు_Module)) తిరిగి modp;
    }
    తిరిగి శూన్యసూచిక;
}

Ast_కణుపు_Module* కనుగొను_Owner_Module_If_సజీవ(Ast_కణుపు* nodep, స్థిరరాశి సజీవ_Nodes& సజీవ_Nodes_2) {
    తిరిగి కనుగొను_Owner_Module_Impl(nodep, &సజీవ_Nodes_2);
}

తార్కిక moduleMatchesOwner(స్థిరరాశి Ast_కణుపు_Module* modp, స్థిరరాశి string& ownerName) {
    ఒకవేళ (!modp || ownerName.ఖాళీలు()) తిరిగి అబద్ధం;
    తిరిగి modp->name() == ownerName || modp->origName() == ownerName;
}
}  // namespace

Ast_కణుపు_Module* V3LinkDotIfaceCapture::కనుగొను_Owner_Module(Ast_కణుపు* nodep) {
    తిరిగి కనుగొను_Owner_Module_Impl(nodep, శూన్యసూచిక);
}

శూన్య V3LinkDotIfaceCapture::dumpEntries(స్థిరరాశి string& label) {
    UINFO(9, "========== iface capture dumpEntries: " << label << " ==========");
    పూర్ణాంకం idx = 0;
    v3Global.rootp()->foreach([&](AstRefDType* refp) {
        స్థిరరాశి VIface_Capture_ట్యాగ్* స్థిరరాశి tagp = capture_ట్యాగ్(refp);
        ఒకవేళ (!tagp) తిరిగి;
        స్థిరరాశి Ast_కణుపు_Module* స్థిరరాశి ownerModp = కనుగొను_Owner_Module(refp);
        UINFO(9, "  [" << idx << "] ref=" << refp->name() << " refp=" << cvt_గమ్యం_Hex(refp)
                       << " ownerMod=" << (ownerModp ? ownerModp->name() : "<null>")
                       << " cellPath='" << tagp->m_cell_మార్గం << "' ownerModName='"
                       << tagp->m_ownerModName << "' kind="
                       << (tagp->m_రకం == VIface_Capture_ట్యాగ్::రకం_2::PARAM_TYPE ? "param type"
                                                                              : "typedef"));
        ++idx;
    });
    UINFO(9, "========== end iface capture dumpEntries (" << idx << " captured) ==========");
}

శూన్య V3LinkDotIfaceCapture::ట్యాగ్(AstRefDType* refp, స్థిరరాశి Ast_కణుపు_Module* capturedInp,
                                VIface_Capture_ట్యాగ్::రకం_2 రకం, స్థిరరాశి string& cell_మార్గం,
                                స్థిరరాశి string& ownerModName) {
    UASSERT_OBJ(!refp->captureTagp(), refp, "Reference captured twice");
    UASSERT_OBJ(capturedInp, refp, "Captured reference is not in a module");
    UASSERT_OBJ(!cell_మార్గం.ఖాళీలు(), refp, "Captured reference has no cell path");
    s_ట్యాగులు.emplace_వెనుక(VIface_Capture_ట్యాగ్{రకం, cell_మార్గం, ownerModName, capturedInp->origName()});
    refp->captureTagp(&s_ట్యాగులు.వెనుక());
}

స్థిరరాశి VIface_Capture_ట్యాగ్* V3LinkDotIfaceCapture::capture_ట్యాగ్(AstRefDType* refp) {
    స్థిరరాశి VIface_Capture_ట్యాగ్* స్థిరరాశి tagp = refp->captureTagp();
    ఒకవేళ (!tagp) తిరిగి శూన్యసూచిక;
    స్థిరరాశి Ast_కణుపు_Module* స్థిరరాశి ownerp = కనుగొను_Owner_Module(refp);
    ఒకవేళ (!ownerp || ownerp->origName() != tagp->m_captured_లోపల_Name) తిరిగి శూన్యసూచిక;
    తిరిగి tagp;
}

// Walk a dot-separated cell path through the cell / IFACEREFDTYPE hierarchy
// starting from startModp.  Returns the module at the end of the path, or
// nullptr if any component cannot be resolved.
// Cell/port names are preserved across clones by cloneTree, so this works
// identically on template and cloned modules.
Ast_కణుపు_Module* V3LinkDotIfaceCapture::follow_Cell_మార్గం(Ast_కణుపు_Module* ప్రారంభం_Modp,
                                                     స్థిరరాశి string& cell_మార్గం) {
    ఒకవేళ (cell_మార్గం.ఖాళీలు() || !ప్రారంభం_Modp) తిరిగి శూన్యసూచిక;
    Ast_కణుపు_Module* curModp = ప్రారంభం_Modp;
    string మిగిలిన = cell_మార్గం;
    అయితే (!మిగిలిన.ఖాళీలు() && curModp) {
        string component;
        స్థిరరాశి కొలత_t dotPos = మిగిలిన.కనుగొను('.');
        ఒకవేళ (dotPos == string::npos) {
            component = మిగిలిన;
            మిగిలిన.clear();
        } లేకపోతే {
            component = మిగిలిన.substr(0, dotPos);
            మిగిలిన = మిగిలిన.substr(dotPos + 1);
        }
        // Matched without any array index; the elements of an instance array (see V3Param) all
        // instantiate the same module, so any of them will do
        స్థిరరాశి string component_పునాది = Ast_కణుపు::nameNoArray(component);
        Ast_కణుపు_Module* తదుపరి_Modp = శూన్యసూచిక;
        కొరకు (Ast_కణుపు* sp = curModp->stmtsp(); sp; sp = sp->nextp()) {
            ఒకవేళ (AstCell* స్థిరరాశి cellp = VN_CAST(sp, Cell)) {
                ఒకవేళ ((cellp->name() == component
                     || Ast_కణుపు::nameNoArray(cellp->name()) == component_పునాది)
                    && cellp->modp()) {
                    తదుపరి_Modp = cellp->modp();
                    ఆపు;
                }
            }
            ఒకవేళ (AstVar* స్థిరరాశి varp = VN_CAST(sp, Var)) {
                ఒకవేళ (varp->isIfaceRef() && varp->subDTypep()) {
                    string var_పునాది_Name = varp->name();
                    స్థిరరాశి కొలత_t viftopPos = var_పునాది_Name.కనుగొను("__Viftop");
                    ఒకవేళ (viftopPos != string::npos) {
                        var_పునాది_Name = var_పునాది_Name.substr(0, viftopPos);
                    }
                    ఒకవేళ (var_పునాది_Name == component
                        || Ast_కణుపు::nameNoArray(var_పునాది_Name) == component_పునాది) {
                        ఒకవేళ (AstIfaceRefDType* స్థిరరాశి irefp
                            = ifaceRefFromVarDType(varp->subDTypep())) {
                            AstIface* స్థిరరాశి ifacep = irefp->iface_ద్వారా_Cellp();
                            ఒకవేళ (ifacep) {
                                తదుపరి_Modp = ifacep;
                                ఆపు;
                            }
                        }
                    }
                }
            }
        }
        curModp = తదుపరి_Modp;
    }
    తిరిగి curModp;
}

// replaces the lambda used in V3LinkDot.cpp for iface capture
శూన్య V3LinkDotIfaceCapture::captureTypedefContext(AstRefDType* refp, స్థిరరాశి అక్షరం* stageLabel,
                                                  పూర్ణాంకం dotPos, స్థిరరాశి std::string& dot_పాఠ్యం,
                                                  VSymEnt* dotSymp, Ast_కణుపు_Module* modp,
                                                  స్థిరరాశి std::function<std::string()>& indentFn) {
    ఒకవేళ (!enabled() || !refp) తిరిగి;

    UINFO(9, indentFn() << "iface capture capture request stage=" << stageLabel
                        << " typedef=" << refp << " name=" << refp->name() << " dotPos=" << dotPos
                        << " dotText='" << dot_పాఠ్యం << "' dotSym=" << dotSymp);

    స్థిరరాశి AstCell* ifaceCellp = శూన్యసూచిక;
    ఒకవేళ (dotSymp && VN_IS(dotSymp->nodep(), Cell)) {
        స్థిరరాశి AstCell* స్థిరరాశి cellp = VN_AS(dotSymp->nodep(), Cell);
        ఒకవేళ (cellp->modp() && VN_IS(cellp->modp(), Iface)) ifaceCellp = cellp;
    }
    ఒకవేళ (!ifaceCellp) {
        UINFO(9, indentFn() << "iface capture capture skipped typedef=" << refp
                            << " (no iface context)");
        తిరిగి;
    }
    // Skip internal interface typedef references (typedef used within its own interface)
    ఒకవేళ (ifaceCellp->modp() == modp) {
        UINFO(9, indentFn() << "iface capture capture skipped typedef=" << refp
                            << " (internal to interface " << modp->name() << ")");
        తిరిగి;
    }

    // dotText is always non-empty for interface typedef captures.  If this
    // fires, the caller resolved to an interface Cell but did not accumulate
    // a dotText path - investigate the dot-state in visitParseRef.
    UASSERT_OBJ(!dot_పాఠ్యం.ఖాళీలు(), refp, "captureTypedefContext: dotText empty");
    స్థిరరాశి string cell_మార్గం = dot_పాఠ్యం;

    // A reference to a ParamTypeDType is captured as a parameter type, else as a typedef
    ఒకవేళ (AstParamTypeDType* స్థిరరాశి paramTypep = VN_CAST(refp->refDTypep(), ParamTypeDType)) {
        V3LinkDotIfaceCapture::కూడు_Param_Type(refp, cell_మార్గం, modp, paramTypep, "");
    } లేకపోతే {
        ట్యాగ్(refp, modp, VIface_Capture_ట్యాగ్::రకం_2::TYPEDEF, cell_మార్గం,
            resolveOwnerName("", refp->typedefp()));
    }

    UINFO(9, indentFn() << "iface capture capture success typedef=" << refp
                        << " cell=" << ifaceCellp << " cellPath='" << cell_మార్గం << "'"
                        << " mod=" << (ifaceCellp->modp() ? ifaceCellp->modp()->name() : "<null>")
                        << " dotPos=" << dotPos);
}

శూన్య V3LinkDotIfaceCapture::కూడు_Param_Type(AstRefDType* refp, స్థిరరాశి string& cell_మార్గం,
                                         Ast_కణుపు_Module* ownerModp, AstParamTypeDType* paramTypep,
                                         స్థిరరాశి string& paramTypeOwnerModName) {
    UASSERT(refp, "addParamType() called with null refp");
    UASSERT(ownerModp,
            "addParamType() called with null ownerModp for refp='" << refp->prettyNameQ() << "'");
    UASSERT_OBJ(paramTypep, refp,
                "addParamType() called with null paramTypep for refp='" << refp->prettyNameQ()
                                                                        << "'");
    స్థిరరాశి string ptOwnerName = resolveOwnerName(paramTypeOwnerModName, paramTypep);
    UINFO(9, "addParamType: refp=" << refp << " cellPath='" << cell_మార్గం << "'"
                                   << " ownerModp=" << (ownerModp ? ownerModp->name() : "<null>")
                                   << " paramTypep=" << paramTypep << " paramTypeOwnerModName='"
                                   << ptOwnerName << "'");
    UINFO(9, "addParamType: paramTypep subDTypep chain:");
    ఒకవేళ (debug())
        paramTypep->foreach([&](AstRefDType* innerRefp) {
            UINFO(9,
                  "  inner RefDType: "
                      << innerRefp << " refDTypep=" << innerRefp->refDTypep()
                      << (innerRefp->refDTypep() ? " refDTypep->name=" : "")
                      << (innerRefp->refDTypep() ? innerRefp->refDTypep()->prettyTypeName() : ""));
        });
    ట్యాగ్(refp, ownerModp, VIface_Capture_ట్యాగ్::రకం_2::PARAM_TYPE, cell_మార్గం, ptOwnerName);
}

పూర్ణాంకం V3LinkDotIfaceCapture::resolveCapturedRefs() {
    పూర్ణాంకం fixed = 0;

    // TARGET RESOLUTION.  By this point all cloning is complete and cell
    // pointers are wired to the correct interface clones.  For each tagged
    // reference we walk its cell path from its owner module to find the
    // correct target module, then locate the PARAMTYPEDTYPE / TYPEDEF by name.
    // See the ARCHITECTURE comment above for the full picture.

    v3Global.rootp()->foreach([&](AstRefDType* refp) {
        స్థిరరాశి VIface_Capture_ట్యాగ్* స్థిరరాశి tagp = capture_ట్యాగ్(refp);
        ఒకవేళ (!tagp) తిరిగి;
        Ast_కణుపు_Module* స్థిరరాశి ownerModp = కనుగొను_Owner_Module(refp);
        ఒకవేళ (!ownerModp || ownerModp->dead() || VN_IS(ownerModp, Package)) తిరిగి;

        UINFO(9, "finalizeIfaceCapture Phase3 entry: refp="
                     << refp->name() << " (" << cvt_గమ్యం_Hex(refp) << ")" << " ownerMod="
                     << ownerModp->name() << " cellPath='" << tagp->m_cell_మార్గం << "' targetKind="
                     << (tagp->m_రకం == VIface_Capture_ట్యాగ్::రకం_2::PARAM_TYPE ? "param type"
                                                                            : "typedef"));

        // Prefer the owner itself when its stable template identity matches the
        // captured target owner.  Otherwise resolve and validate the cell path.
        Ast_కణుపు_Module* correctModp = శూన్యసూచిక;
        ఒకవేళ (moduleMatchesOwner(ownerModp, tagp->m_ownerModName)) {
            correctModp = ownerModp;
        } లేకపోతే {
            correctModp = follow_Cell_మార్గం(ownerModp, tagp->m_cell_మార్గం);
            UINFO(9, "  followCellPath('"
                         << ownerModp->name() << "', '" << tagp->m_cell_మార్గం
                         << "') = " << (correctModp ? correctModp->name() : "<null>")
                         << (correctModp ? (correctModp->dead() ? " (DEAD)" : " (live)") : ""));
            UASSERT_OBJ(correctModp && !correctModp->dead()
                            && moduleMatchesOwner(correctModp, tagp->m_ownerModName),
                        refp,
                        "captured ref '" << refp->prettyNameQ() << "' cell path '"
                                         << tagp->m_cell_మార్గం << "' did not resolve to live owner '"
                                         << tagp->m_ownerModName << "'");
        }
        UASSERT_OBJ(retarget_Ref_గమ్యం_Module(refp, correctModp), refp,
                    "could not retarget captured "
                        << (tagp->m_రకం == VIface_Capture_ట్యాగ్::రకం_2::PARAM_TYPE ? "parameter type "
                                                                               : "typedef ")
                        << refp->prettyNameQ() << " in " << correctModp->prettyNameQ());
        refp->user3(నిజం);
        ++fixed;
    });

    తిరిగి fixed;
}

శూన్య V3LinkDotIfaceCapture::verifyNoDeadRefs(స్థిరరాశి సజీవ_Nodes& సజీవ_Nodes_2) {
    // Assert: no REFDTYPE in any live module should have typedefp or refDTypep
    // pointing to a dead module.
    కొరకు (Ast_కణుపు* nodep = v3Global.rootp()->modulesp(); nodep; nodep = nodep->nextp()) {
        ఒకవేళ (Ast_కణుపు_Module* స్థిరరాశి modp = VN_CAST(nodep, కణుపు_Module)) {
            ఒకవేళ (modp->dead()) కొనసాగు;
            modp->foreach([&](AstRefDType* refp) {
                ఒకవేళ (refp->typedefp()) {
                    UASSERT_OBJ(సజీవ_Nodes_2.లెక్క(refp->typedefp()), refp,
                                "REFDTYPE '" << refp->prettyNameQ() << "' in live module '"
                                             << modp->prettyNameQ()
                                             << "' has a dangling typedefp");
                    Ast_కణుపు_Module* స్థిరరాశి ownerModp
                        = కనుగొను_Owner_Module_If_సజీవ(refp->typedefp(), సజీవ_Nodes_2);
                    UASSERT_OBJ(!ownerModp || !ownerModp->dead(), refp,
                                "REFDTYPE '" << refp->prettyNameQ() << "' in live module '"
                                             << modp->prettyNameQ()
                                             << "' has typedefp pointing to dead module '"
                                             << ownerModp->prettyNameQ() << "'");
                }
                ఒకవేళ (refp->refDTypep()) {
                    UASSERT_OBJ(సజీవ_Nodes_2.లెక్క(refp->refDTypep()), refp,
                                "REFDTYPE '" << refp->prettyNameQ() << "' in live module '"
                                             << modp->prettyNameQ()
                                             << "' has a dangling refDTypep");
                    Ast_కణుపు_Module* స్థిరరాశి ownerModp
                        = కనుగొను_Owner_Module_If_సజీవ(refp->refDTypep(), సజీవ_Nodes_2);
                    UASSERT_OBJ(!ownerModp || !ownerModp->dead(), refp,
                                "REFDTYPE '" << refp->prettyNameQ() << "' in live module '"
                                             << modp->prettyNameQ()
                                             << "' has refDTypep pointing to dead module '"
                                             << ownerModp->prettyNameQ() << "'");
                }
            });
        }
    }
    ఒకవేళ (v3Global.rootp()->typeTablep()) {
        కొరకు (Ast_కణుపు* nodep = v3Global.rootp()->typeTablep()->typesp(); nodep;
             nodep = nodep->nextp()) {
            nodep->foreach([&](AstRefDType* refp) {
                ఒకవేళ (refp->typedefp()) {
                    UASSERT_OBJ(సజీవ_Nodes_2.లెక్క(refp->typedefp()), refp,
                                "REFDTYPE '" << refp->prettyNameQ()
                                             << "' in type table has a dangling typedefp");
                    Ast_కణుపు_Module* స్థిరరాశి ownerModp
                        = కనుగొను_Owner_Module_If_సజీవ(refp->typedefp(), సజీవ_Nodes_2);
                    UASSERT_OBJ(!ownerModp || !ownerModp->dead(), refp,
                                "REFDTYPE '"
                                    << refp->prettyNameQ()
                                    << "' in type table has typedefp pointing to dead module '"
                                    << ownerModp->prettyNameQ() << "'");
                }
                ఒకవేళ (refp->refDTypep()) {
                    UASSERT_OBJ(సజీవ_Nodes_2.లెక్క(refp->refDTypep()), refp,
                                "REFDTYPE '" << refp->prettyNameQ()
                                             << "' in type table has a dangling refDTypep");
                    Ast_కణుపు_Module* స్థిరరాశి ownerModp
                        = కనుగొను_Owner_Module_If_సజీవ(refp->refDTypep(), సజీవ_Nodes_2);
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

శూన్య V3LinkDotIfaceCapture::finalizeIfaceCapture() {
    ఒకవేళ (!s_enabled) తిరిగి;
    UINFO(4, "finalizeIfaceCapture: fixing remaining cross-interface refs");
    ఒకవేళ (!v3Global.rootp()) తిరిగి;
    clearModuleCache();  // Ensure fresh view after all cloning/widthing

    // Resolve live captured refs from stable path metadata before inspecting
    // any inherited target pointers, which may refer to replaced template nodes.
    స్థిరరాశి పూర్ణాంకం capturedFixed = resolveCapturedRefs();
    UINFO(4, "finalizeIfaceCapture: structurally resolved " << capturedFixed << " captured refs");

    ఒకవేళ (debug() >= 9) dumpEntries("after finalizeIfaceCapture");

    // Emit statistics for --stats
    V3Stats::కూడు_Stat("IfaceCapture, Captured refs", s_ట్యాగులు.కొలత());
    V3Stats::కూడు_Stat("IfaceCapture, Captured refs resolved", capturedFixed);

    // No reference may be left pointing into a module about to be removed
    ఒకవేళ (v3Global.opt.debugCheck()) verifyNoDeadRefs(collect_సజీవ_Nodes());
    // Drop every tag before its record is freed.
    v3Global.rootp()->foreach([](AstRefDType* refp) { refp->captureTagp(శూన్యసూచిక); });
    reset();
}
