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

#ଅନ୍ତର୍ଭାଵ "V3LinkDotIfaceCapture.h"

#ଅନ୍ତର୍ଭାଵ "V3Error.h"
#ଅନ୍ତର୍ଭାଵ "V3Global.h"
#ଅନ୍ତର୍ଭାଵ "V3Stats.h"
#ଅନ୍ତର୍ଭାଵ "V3SymTable.h"

#ଅନ୍ତର୍ଭାଵ <unordered_map>
#ଅନ୍ତର୍ଭାଵ <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

std::deque<VIfaceCaptureTag> V3LinkDotIfaceCapture::s_tags{};
ତାର୍କିକ V3LinkDotIfaceCapture::s_enabled = ସତ୍ୟ;

// LCOV_EXCL_START
ଶୂନ୍ୟ V3LinkDotIfaceCapture::enable(ତାର୍କିକ flag) {
    s_enabled = flag;
    ଯଦି (!flag) clearModuleCache();
}
// LCOV_EXCL_STOP

ଶୂନ୍ୟ V3LinkDotIfaceCapture::reset() {
    s_tags.clear();
    clearModuleCache();
}

// Per-module cache of statement-level names to avoid O(N*M) linear scans.
// Lazily built on first access for a given module; cleared at phase boundaries.
// Uses vectors per name to handle rare cases where different node types share a name
// (e.g. a Typedef and a ParamTypeDType both named 'sc_tag_status_t').
ନାମାକାଶ {
ସଂରଚନା StmtNameMap final {
    std::unordered_map<string, std::vector<Ast_ଗଣ୍ଠି*>> m_byName;
};
std::unordered_map<Ast_ଗଣ୍ଠି_Module*, StmtNameMap> s_moduleCache;

ଅଚଳ StmtNameMap& getOrBuild(Ast_ଗଣ୍ଠି_Module* modp) {
    auto it = s_moduleCache.find(modp);
    ଯଦି (it != s_moduleCache.end()) ଫେରନ୍ତୁ it->second;
    StmtNameMap& cache = s_moduleCache[modp];
    ପାଇଁ (Ast_ଗଣ୍ଠି* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
        ଅଚଳ string& nm = stmtp->name();
        ଯଦି (!nm.empty()) cache.m_byName[nm].ଠେଲ_back(stmtp);
    }
    ଫେରନ୍ତୁ cache;
}
}  // namespace

ଶୂନ୍ୟ V3LinkDotIfaceCapture::clearModuleCache() { s_moduleCache.clear(); }

AstIfaceRefDType* V3LinkDotIfaceCapture::ifaceRefFromVarDType(Ast_ଗଣ୍ଠି_DType* dtypep) {
    AstIfaceRefDType* resultp = nullptr;
    ପାଇଁ (Ast_ଗଣ୍ଠି_DType* curp = dtypep; curp;) {
        ଯଦି (AstIfaceRefDType* ଅଚଳ irefp = VN_CAST(curp, IfaceRefDType)) {
            resultp = irefp;
            ଭାଙ୍ଗ;
        } ଅନ୍ୟଥା ଯଦି (AstBracketArrayDType* ଅଚଳ bracketp = VN_CAST(curp, BracketArrayDType)) {
            curp = bracketp->subDTypep();
        } ଅନ୍ୟଥା ଯଦି (AstUnpackArrayDType* ଅଚଳ unpackp = VN_CAST(curp, UnpackArrayDType)) {
            curp = unpackp->subDTypep();
        } ଅନ୍ୟଥା {
            v3fatalSrc("ifaceRefFromVarDType: unexpected dtype " << curp->prettyTypeName()
                                                                 << " in chain");
        }
    }
    ଫେରନ୍ତୁ resultp;
}

ନାମାକାଶ {
// Resolve the owner module name for a typedef/paramType node.
// Returns hint if non-empty, otherwise walks backp() to find the owner module name.
string resolveOwnerName(ଅଚଳ string& hint, Ast_ଗଣ୍ଠି* nodep) {
    ଯଦି (!hint.empty()) ଫେରନ୍ତୁ hint;
    ଯଦି (!nodep) ଫେରନ୍ତୁ "";
    Ast_ଗଣ୍ଠି_Module* ଅଚଳ ownerp = V3LinkDotIfaceCapture::findOwnerModule(nodep);
    ଫେରନ୍ତୁ ownerp ? ownerp->name() : string{};
}
}  // namespace

AstTypedef* V3LinkDotIfaceCapture::findTypedefInModule(Ast_ଗଣ୍ଠି_Module* modp, ଅଚଳ string& name) {
    AstTypedef* resultp = nullptr;
    ଅଚଳ StmtNameMap& cache = getOrBuild(modp);
    ଅଚଳ auto it = cache.m_byName.find(name);
    ଯଦି (!(it == cache.m_byName.end())) {
        ପାଇଁ (Ast_ଗଣ୍ଠି* nodep : it->second) {
            ଯଦି (AstTypedef* ଅଚଳ tdp = VN_CAST(nodep, Typedef)) {
                resultp = tdp;
                ଭାଙ୍ଗ;
            }
        }
    }
    ଫେରନ୍ତୁ resultp;
}
Ast_ଗଣ୍ଠି_DType* V3LinkDotIfaceCapture::findDTypeInModule(Ast_ଗଣ୍ଠି_Module* modp, ଅଚଳ string& name,
                                                       VNType type) {

    Ast_ଗଣ୍ଠି_DType* resultp = nullptr;
    ଅଚଳ StmtNameMap& cache = getOrBuild(modp);
    ଅଚଳ auto it = cache.m_byName.find(name);
    ଯଦି (!(it == cache.m_byName.end())) {
        ପାଇଁ (Ast_ଗଣ୍ଠି* nodep : it->second) {
            ଯଦି (Ast_ଗଣ୍ଠି_DType* ଅଚଳ dtp = VN_CAST(nodep, ଗଣ୍ଠି_DType)) {
                ଯଦି (dtp->type() == type) {
                    resultp = dtp;
                    ଭାଙ୍ଗ;
                }
            }
        }
    }
    ଫେରନ୍ତୁ resultp;
}
AstParamTypeDType* V3LinkDotIfaceCapture::findParamTypeInModule(Ast_ଗଣ୍ଠି_Module* modp,
                                                                ଅଚଳ string& name) {

    AstParamTypeDType* resultp = nullptr;
    ଅଚଳ StmtNameMap& cache = getOrBuild(modp);
    ଅଚଳ auto it = cache.m_byName.find(name);
    ଯଦି (!(it == cache.m_byName.end())) {
        ପାଇଁ (Ast_ଗଣ୍ଠି* nodep : it->second) {
            ଯଦି (AstParamTypeDType* ଅଚଳ ptdp = VN_CAST(nodep, ParamTypeDType)) {
                resultp = ptdp;
                ଭାଙ୍ଗ;
            }
        }
    }
    ଫେରନ୍ତୁ resultp;
}

ତାର୍କିକ V3LinkDotIfaceCapture::retargetRefToModule(AstRefDType* refp, Ast_ଗଣ୍ଠି_Module* targetModp) {
    ଅଚଳ VIfaceCaptureTag* ଅଚଳ tagp = refp->captureTagp();
    UASSERT_OBJ(tagp, refp, "Retarget of a reference that was not captured");
    UASSERT_OBJ(targetModp, refp, "Retarget to a null module");

    ଯଦି (tagp->m_kind == VIfaceCaptureTag::Kind::PARAM_TYPE) {
        AstParamTypeDType* ଅଚଳ paramTypep = findParamTypeInModule(targetModp, refp->name());
        ଯଦି (!paramTypep) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        retargetRefToParamType(refp, paramTypep);
        ଫେରନ୍ତୁ ସତ୍ୟ;
    }

    AstTypedef* ଅଚଳ typedefp = findTypedefInModule(targetModp, refp->name());
    ଯଦି (!typedefp) ଫେରନ୍ତୁ ମିଥ୍ୟା;
    retargetRefToTypedef(refp, typedefp);
    ଫେରନ୍ତୁ ସତ୍ୟ;
}

ଶୂନ୍ୟ V3LinkDotIfaceCapture::retargetRefToParamType(AstRefDType* refp,
                                                   AstParamTypeDType* paramTypep) {
    UASSERT_OBJ(paramTypep, refp, "Retarget to a null parameter type");
    refp->refDTypep(paramTypep);
    refp->dtypep(paramTypep);
}

ଶୂନ୍ୟ V3LinkDotIfaceCapture::retargetRefToTypedef(AstRefDType* refp, AstTypedef* typedefp) {
    UASSERT_OBJ(typedefp, refp, "Retarget to a null typedef");
    refp->typedefp(typedefp);
    // An incomplete typedef is still a successful name resolution, but
    // must not erase type links that a later width pass can complete.
    ଯଦି (Ast_ଗଣ୍ଠି_DType* ଅଚଳ dtypep = typedefp->subDTypep()) {
        refp->refDTypep(dtypep);
        refp->dtypep(dtypep);
    }
}

ନାମାକାଶ {
using LiveNodes = std::unordered_set<ଅଚଳ Ast_ଗଣ୍ଠି*>;

// A scoped snapshot of every node currently in the tree.  V3Broken::isLinkable()
// cannot serve this role: its table is populated only while V3Broken::brokenAll()
// runs and is cleared before it returns.
LiveNodes collectLiveNodes() {
    LiveNodes liveNodes;
    v3Global.rootp()->foreach([&](Ast_ଗଣ୍ଠି* nodep) { liveNodes.ଭର୍ତ୍ତି(nodep); });
    ଫେରନ୍ତୁ liveNodes;
}

// Find the module that owns this node; a snapshot, if given, stops the walk at a stale link.
Ast_ଗଣ୍ଠି_Module* findOwnerModuleImpl(Ast_ଗଣ୍ଠି* nodep, ଅଚଳ LiveNodes* liveNodesp) {
    ପାଇଁ (Ast_ଗଣ୍ଠି* curp = nodep; curp; curp = curp->backp()) {
        ଯଦି (liveNodesp && !liveNodesp->ଗଣନା_2(curp)) ଫେରନ୍ତୁ nullptr;
        ଯଦି (Ast_ଗଣ୍ଠି_Module* ଅଚଳ modp = VN_CAST(curp, ଗଣ୍ଠି_Module)) ଫେରନ୍ତୁ modp;
    }
    ଫେରନ୍ତୁ nullptr;
}

Ast_ଗଣ୍ଠି_Module* findOwnerModuleIfLive(Ast_ଗଣ୍ଠି* nodep, ଅଚଳ LiveNodes& liveNodes) {
    ଫେରନ୍ତୁ findOwnerModuleImpl(nodep, &liveNodes);
}

ତାର୍କିକ moduleMatchesOwner(ଅଚଳ Ast_ଗଣ୍ଠି_Module* modp, ଅଚଳ string& ownerName) {
    ଯଦି (!modp || ownerName.empty()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
    ଫେରନ୍ତୁ modp->name() == ownerName || modp->origName() == ownerName;
}
}  // namespace

Ast_ଗଣ୍ଠି_Module* V3LinkDotIfaceCapture::findOwnerModule(Ast_ଗଣ୍ଠି* nodep) {
    ଫେରନ୍ତୁ findOwnerModuleImpl(nodep, nullptr);
}

ଶୂନ୍ୟ V3LinkDotIfaceCapture::dumpEntries(ଅଚଳ string& label) {
    UINFO(9, "========== iface capture dumpEntries: " << label << " ==========");
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା idx = 0;
    v3Global.rootp()->foreach([&](AstRefDType* refp) {
        ଅଚଳ VIfaceCaptureTag* ଅଚଳ tagp = captureTag(refp);
        ଯଦି (!tagp) ଫେରନ୍ତୁ;
        ଅଚଳ Ast_ଗଣ୍ଠି_Module* ଅଚଳ ownerModp = findOwnerModule(refp);
        UINFO(9, "  [" << idx << "] ref=" << refp->name() << " refp=" << cvtToHex(refp)
                       << " ownerMod=" << (ownerModp ? ownerModp->name() : "<null>")
                       << " cellPath='" << tagp->m_cellPath << "' ownerModName='"
                       << tagp->m_ownerModName << "' kind="
                       << (tagp->m_kind == VIfaceCaptureTag::Kind::PARAM_TYPE ? "param type"
                                                                              : "typedef"));
        ++idx;
    });
    UINFO(9, "========== end iface capture dumpEntries (" << idx << " captured) ==========");
}

ଶୂନ୍ୟ V3LinkDotIfaceCapture::tag(AstRefDType* refp, ଅଚଳ Ast_ଗଣ୍ଠି_Module* capturedInp,
                                VIfaceCaptureTag::Kind kind, ଅଚଳ string& cellPath,
                                ଅଚଳ string& ownerModName) {
    UASSERT_OBJ(!refp->captureTagp(), refp, "Reference captured twice");
    UASSERT_OBJ(capturedInp, refp, "Captured reference is not in a module");
    UASSERT_OBJ(!cellPath.empty(), refp, "Captured reference has no cell path");
    s_tags.emplace_back(VIfaceCaptureTag{kind, cellPath, ownerModName, capturedInp->origName()});
    refp->captureTagp(&s_tags.back());
}

ଅଚଳ VIfaceCaptureTag* V3LinkDotIfaceCapture::captureTag(AstRefDType* refp) {
    ଅଚଳ VIfaceCaptureTag* ଅଚଳ tagp = refp->captureTagp();
    ଯଦି (!tagp) ଫେରନ୍ତୁ nullptr;
    ଅଚଳ Ast_ଗଣ୍ଠି_Module* ଅଚଳ ownerp = findOwnerModule(refp);
    ଯଦି (!ownerp || ownerp->origName() != tagp->m_capturedInName) ଫେରନ୍ତୁ nullptr;
    ଫେରନ୍ତୁ tagp;
}

// Walk a dot-separated cell path through the cell / IFACEREFDTYPE hierarchy
// starting from startModp.  Returns the module at the end of the path, or
// nullptr if any component cannot be resolved.
// Cell/port names are preserved across clones by cloneTree, so this works
// identically on template and cloned modules.
Ast_ଗଣ୍ଠି_Module* V3LinkDotIfaceCapture::followCellPath(Ast_ଗଣ୍ଠି_Module* startModp,
                                                     ଅଚଳ string& cellPath) {
    ଯଦି (cellPath.empty() || !startModp) ଫେରନ୍ତୁ nullptr;
    Ast_ଗଣ୍ଠି_Module* curModp = startModp;
    string remaining = cellPath;
    ଯେତେବେଳେ (!remaining.empty() && curModp) {
        string component;
        ଅଚଳ size_t dotPos = remaining.find('.');
        ଯଦି (dotPos == string::npos) {
            component = remaining;
            remaining.clear();
        } ଅନ୍ୟଥା {
            component = remaining.substr(0, dotPos);
            remaining = remaining.substr(dotPos + 1);
        }
        // Matched without any array index; the elements of an instance array (see V3Param) all
        // instantiate the same module, so any of them will do
        ଅଚଳ string componentBase = Ast_ଗଣ୍ଠି::nameNoArray(component);
        Ast_ଗଣ୍ଠି_Module* nextModp = nullptr;
        ପାଇଁ (Ast_ଗଣ୍ଠି* sp = curModp->stmtsp(); sp; sp = sp->nextp()) {
            ଯଦି (AstCell* ଅଚଳ cellp = VN_CAST(sp, Cell)) {
                ଯଦି ((cellp->name() == component
                     || Ast_ଗଣ୍ଠି::nameNoArray(cellp->name()) == componentBase)
                    && cellp->modp()) {
                    nextModp = cellp->modp();
                    ଭାଙ୍ଗ;
                }
            }
            ଯଦି (AstVar* ଅଚଳ varp = VN_CAST(sp, Var)) {
                ଯଦି (varp->isIfaceRef() && varp->subDTypep()) {
                    string varBaseName = varp->name();
                    ଅଚଳ size_t viftopPos = varBaseName.find("__Viftop");
                    ଯଦି (viftopPos != string::npos) {
                        varBaseName = varBaseName.substr(0, viftopPos);
                    }
                    ଯଦି (varBaseName == component
                        || Ast_ଗଣ୍ଠି::nameNoArray(varBaseName) == componentBase) {
                        ଯଦି (AstIfaceRefDType* ଅଚଳ irefp
                            = ifaceRefFromVarDType(varp->subDTypep())) {
                            AstIface* ଅଚଳ ifacep = irefp->ifaceViaCellp();
                            ଯଦି (ifacep) {
                                nextModp = ifacep;
                                ଭାଙ୍ଗ;
                            }
                        }
                    }
                }
            }
        }
        curModp = nextModp;
    }
    ଫେରନ୍ତୁ curModp;
}

// replaces the lambda used in V3LinkDot.cpp for iface capture
ଶୂନ୍ୟ V3LinkDotIfaceCapture::captureTypedefContext(AstRefDType* refp, ଅଚଳ ଅକ୍ଷର* stageLabel,
                                                  ପୂର୍ଣ୍ଣସଂଖ୍ୟା dotPos, ଅଚଳ std::string& dotText,
                                                  VSymEnt* dotSymp, Ast_ଗଣ୍ଠି_Module* modp,
                                                  ଅଚଳ std::function<std::string()>& indentFn) {
    ଯଦି (!enabled() || !refp) ଫେରନ୍ତୁ;

    UINFO(9, indentFn() << "iface capture capture request stage=" << stageLabel
                        << " typedef=" << refp << " name=" << refp->name() << " dotPos=" << dotPos
                        << " dotText='" << dotText << "' dotSym=" << dotSymp);

    ଅଚଳ AstCell* ifaceCellp = nullptr;
    ଯଦି (dotSymp && VN_IS(dotSymp->nodep(), Cell)) {
        ଅଚଳ AstCell* ଅଚଳ cellp = VN_AS(dotSymp->nodep(), Cell);
        ଯଦି (cellp->modp() && VN_IS(cellp->modp(), Iface)) ifaceCellp = cellp;
    }
    ଯଦି (!ifaceCellp) {
        UINFO(9, indentFn() << "iface capture capture skipped typedef=" << refp
                            << " (no iface context)");
        ଫେରନ୍ତୁ;
    }
    // Skip internal interface typedef references (typedef used within its own interface)
    ଯଦି (ifaceCellp->modp() == modp) {
        UINFO(9, indentFn() << "iface capture capture skipped typedef=" << refp
                            << " (internal to interface " << modp->name() << ")");
        ଫେରନ୍ତୁ;
    }

    // dotText is always non-empty for interface typedef captures.  If this
    // fires, the caller resolved to an interface Cell but did not accumulate
    // a dotText path - investigate the dot-state in visitParseRef.
    UASSERT_OBJ(!dotText.empty(), refp, "captureTypedefContext: dotText empty");
    ଅଚଳ string cellPath = dotText;

    // A reference to a ParamTypeDType is captured as a parameter type, else as a typedef
    ଯଦି (AstParamTypeDType* ଅଚଳ paramTypep = VN_CAST(refp->refDTypep(), ParamTypeDType)) {
        V3LinkDotIfaceCapture::addParamType(refp, cellPath, modp, paramTypep, "");
    } ଅନ୍ୟଥା {
        tag(refp, modp, VIfaceCaptureTag::Kind::TYPEDEF, cellPath,
            resolveOwnerName("", refp->typedefp()));
    }

    UINFO(9, indentFn() << "iface capture capture success typedef=" << refp
                        << " cell=" << ifaceCellp << " cellPath='" << cellPath << "'"
                        << " mod=" << (ifaceCellp->modp() ? ifaceCellp->modp()->name() : "<null>")
                        << " dotPos=" << dotPos);
}

ଶୂନ୍ୟ V3LinkDotIfaceCapture::addParamType(AstRefDType* refp, ଅଚଳ string& cellPath,
                                         Ast_ଗଣ୍ଠି_Module* ownerModp, AstParamTypeDType* paramTypep,
                                         ଅଚଳ string& paramTypeOwnerModName) {
    UASSERT(refp, "addParamType() called with null refp");
    UASSERT(ownerModp,
            "addParamType() called with null ownerModp for refp='" << refp->prettyNameQ() << "'");
    UASSERT_OBJ(paramTypep, refp,
                "addParamType() called with null paramTypep for refp='" << refp->prettyNameQ()
                                                                        << "'");
    ଅଚଳ string ptOwnerName = resolveOwnerName(paramTypeOwnerModName, paramTypep);
    UINFO(9, "addParamType: refp=" << refp << " cellPath='" << cellPath << "'"
                                   << " ownerModp=" << (ownerModp ? ownerModp->name() : "<null>")
                                   << " paramTypep=" << paramTypep << " paramTypeOwnerModName='"
                                   << ptOwnerName << "'");
    UINFO(9, "addParamType: paramTypep subDTypep chain:");
    ଯଦି (debug())
        paramTypep->foreach([&](AstRefDType* innerRefp) {
            UINFO(9,
                  "  inner RefDType: "
                      << innerRefp << " refDTypep=" << innerRefp->refDTypep()
                      << (innerRefp->refDTypep() ? " refDTypep->name=" : "")
                      << (innerRefp->refDTypep() ? innerRefp->refDTypep()->prettyTypeName() : ""));
        });
    tag(refp, ownerModp, VIfaceCaptureTag::Kind::PARAM_TYPE, cellPath, ptOwnerName);
}

ପୂର୍ଣ୍ଣସଂଖ୍ୟା V3LinkDotIfaceCapture::resolveCapturedRefs() {
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା fixed = 0;

    // TARGET RESOLUTION.  By this point all cloning is complete and cell
    // pointers are wired to the correct interface clones.  For each tagged
    // reference we walk its cell path from its owner module to find the
    // correct target module, then locate the PARAMTYPEDTYPE / TYPEDEF by name.
    // See the ARCHITECTURE comment above for the full picture.

    v3Global.rootp()->foreach([&](AstRefDType* refp) {
        ଅଚଳ VIfaceCaptureTag* ଅଚଳ tagp = captureTag(refp);
        ଯଦି (!tagp) ଫେରନ୍ତୁ;
        Ast_ଗଣ୍ଠି_Module* ଅଚଳ ownerModp = findOwnerModule(refp);
        ଯଦି (!ownerModp || ownerModp->dead() || VN_IS(ownerModp, Package)) ଫେରନ୍ତୁ;

        UINFO(9, "finalizeIfaceCapture Phase3 entry: refp="
                     << refp->name() << " (" << cvtToHex(refp) << ")" << " ownerMod="
                     << ownerModp->name() << " cellPath='" << tagp->m_cellPath << "' targetKind="
                     << (tagp->m_kind == VIfaceCaptureTag::Kind::PARAM_TYPE ? "param type"
                                                                            : "typedef"));

        // Prefer the owner itself when its stable template identity matches the
        // captured target owner.  Otherwise resolve and validate the cell path.
        Ast_ଗଣ୍ଠି_Module* correctModp = nullptr;
        ଯଦି (moduleMatchesOwner(ownerModp, tagp->m_ownerModName)) {
            correctModp = ownerModp;
        } ଅନ୍ୟଥା {
            correctModp = followCellPath(ownerModp, tagp->m_cellPath);
            UINFO(9, "  followCellPath('"
                         << ownerModp->name() << "', '" << tagp->m_cellPath
                         << "') = " << (correctModp ? correctModp->name() : "<null>")
                         << (correctModp ? (correctModp->dead() ? " (DEAD)" : " (live)") : ""));
            UASSERT_OBJ(correctModp && !correctModp->dead()
                            && moduleMatchesOwner(correctModp, tagp->m_ownerModName),
                        refp,
                        "captured ref '" << refp->prettyNameQ() << "' cell path '"
                                         << tagp->m_cellPath << "' did not resolve to live owner '"
                                         << tagp->m_ownerModName << "'");
        }
        UASSERT_OBJ(retargetRefToModule(refp, correctModp), refp,
                    "could not retarget captured "
                        << (tagp->m_kind == VIfaceCaptureTag::Kind::PARAM_TYPE ? "parameter type "
                                                                               : "typedef ")
                        << refp->prettyNameQ() << " in " << correctModp->prettyNameQ());
        refp->user3(ସତ୍ୟ);
        ++fixed;
    });

    ଫେରନ୍ତୁ fixed;
}

ଶୂନ୍ୟ V3LinkDotIfaceCapture::verifyNoDeadRefs(ଅଚଳ LiveNodes& liveNodes) {
    // Assert: no REFDTYPE in any live module should have typedefp or refDTypep
    // pointing to a dead module.
    ପାଇଁ (Ast_ଗଣ୍ଠି* nodep = v3Global.rootp()->modulesp(); nodep; nodep = nodep->nextp()) {
        ଯଦି (Ast_ଗଣ୍ଠି_Module* ଅଚଳ modp = VN_CAST(nodep, ଗଣ୍ଠି_Module)) {
            ଯଦି (modp->dead()) ଜାରି;
            modp->foreach([&](AstRefDType* refp) {
                ଯଦି (refp->typedefp()) {
                    UASSERT_OBJ(liveNodes.ଗଣନା_2(refp->typedefp()), refp,
                                "REFDTYPE '" << refp->prettyNameQ() << "' in live module '"
                                             << modp->prettyNameQ()
                                             << "' has a dangling typedefp");
                    Ast_ଗଣ୍ଠି_Module* ଅଚଳ ownerModp
                        = findOwnerModuleIfLive(refp->typedefp(), liveNodes);
                    UASSERT_OBJ(!ownerModp || !ownerModp->dead(), refp,
                                "REFDTYPE '" << refp->prettyNameQ() << "' in live module '"
                                             << modp->prettyNameQ()
                                             << "' has typedefp pointing to dead module '"
                                             << ownerModp->prettyNameQ() << "'");
                }
                ଯଦି (refp->refDTypep()) {
                    UASSERT_OBJ(liveNodes.ଗଣନା_2(refp->refDTypep()), refp,
                                "REFDTYPE '" << refp->prettyNameQ() << "' in live module '"
                                             << modp->prettyNameQ()
                                             << "' has a dangling refDTypep");
                    Ast_ଗଣ୍ଠି_Module* ଅଚଳ ownerModp
                        = findOwnerModuleIfLive(refp->refDTypep(), liveNodes);
                    UASSERT_OBJ(!ownerModp || !ownerModp->dead(), refp,
                                "REFDTYPE '" << refp->prettyNameQ() << "' in live module '"
                                             << modp->prettyNameQ()
                                             << "' has refDTypep pointing to dead module '"
                                             << ownerModp->prettyNameQ() << "'");
                }
            });
        }
    }
    ଯଦି (v3Global.rootp()->typeTablep()) {
        ପାଇଁ (Ast_ଗଣ୍ଠି* nodep = v3Global.rootp()->typeTablep()->typesp(); nodep;
             nodep = nodep->nextp()) {
            nodep->foreach([&](AstRefDType* refp) {
                ଯଦି (refp->typedefp()) {
                    UASSERT_OBJ(liveNodes.ଗଣନା_2(refp->typedefp()), refp,
                                "REFDTYPE '" << refp->prettyNameQ()
                                             << "' in type table has a dangling typedefp");
                    Ast_ଗଣ୍ଠି_Module* ଅଚଳ ownerModp
                        = findOwnerModuleIfLive(refp->typedefp(), liveNodes);
                    UASSERT_OBJ(!ownerModp || !ownerModp->dead(), refp,
                                "REFDTYPE '"
                                    << refp->prettyNameQ()
                                    << "' in type table has typedefp pointing to dead module '"
                                    << ownerModp->prettyNameQ() << "'");
                }
                ଯଦି (refp->refDTypep()) {
                    UASSERT_OBJ(liveNodes.ଗଣନା_2(refp->refDTypep()), refp,
                                "REFDTYPE '" << refp->prettyNameQ()
                                             << "' in type table has a dangling refDTypep");
                    Ast_ଗଣ୍ଠି_Module* ଅଚଳ ownerModp
                        = findOwnerModuleIfLive(refp->refDTypep(), liveNodes);
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

ଶୂନ୍ୟ V3LinkDotIfaceCapture::finalizeIfaceCapture() {
    ଯଦି (!s_enabled) ଫେରନ୍ତୁ;
    UINFO(4, "finalizeIfaceCapture: fixing remaining cross-interface refs");
    ଯଦି (!v3Global.rootp()) ଫେରନ୍ତୁ;
    clearModuleCache();  // Ensure fresh view after all cloning/widthing

    // Resolve live captured refs from stable path metadata before inspecting
    // any inherited target pointers, which may refer to replaced template nodes.
    ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା capturedFixed = resolveCapturedRefs();
    UINFO(4, "finalizeIfaceCapture: structurally resolved " << capturedFixed << " captured refs");

    ଯଦି (debug() >= 9) dumpEntries("after finalizeIfaceCapture");

    // Emit statistics for --stats
    V3Stats::addStat("IfaceCapture, Captured refs", s_tags.size());
    V3Stats::addStat("IfaceCapture, Captured refs resolved", capturedFixed);

    // No reference may be left pointing into a module about to be removed
    ଯଦି (v3Global.opt.debugCheck()) verifyNoDeadRefs(collectLiveNodes());
    // Drop every tag before its record is freed.
    v3Global.rootp()->foreach([](AstRefDType* refp) { refp->captureTagp(nullptr); });
    reset();
}
