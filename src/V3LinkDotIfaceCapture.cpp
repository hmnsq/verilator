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

std::deque<VIfaceCaptureTag> V3LinkDotIfaceCapture::s_tags{};
तार्किक V3LinkDotIfaceCapture::s_enabled = सत्य;

// LCOV_EXCL_START
शून्य V3LinkDotIfaceCapture::enable(तार्किक flag) {
    s_enabled = flag;
    यदि (!flag) clearModuleCache();
}
// LCOV_EXCL_STOP

शून्य V3LinkDotIfaceCapture::reset() {
    s_tags.clear();
    clearModuleCache();
}

// Per-module cache of statement-level names to avoid O(N*M) linear scans.
// Lazily built on first access for a given module; cleared at phase boundaries.
// Uses vectors per name to handle rare cases where different node types share a name
// (e.g. a Typedef and a ParamTypeDType both named 'sc_tag_status_t').
नामाकाश {
संरचना StmtNameMap final {
    std::unordered_map<string, std::vector<Ast_गाँठो*>> m_byName;
};
std::unordered_map<Ast_गाँठो_Module*, StmtNameMap> s_moduleCache;

अचर StmtNameMap& getOrBuild(Ast_गाँठो_Module* modp) {
    स्वतः it = s_moduleCache.find(modp);
    यदि (it != s_moduleCache.end()) फर्काऊ it->second;
    StmtNameMap& cache = s_moduleCache[modp];
    लागि (Ast_गाँठो* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
        अचर string& nm = stmtp->name();
        यदि (!nm.empty()) cache.m_byName[nm].थप_back(stmtp);
    }
    फर्काऊ cache;
}
}  // namespace

शून्य V3LinkDotIfaceCapture::clearModuleCache() { s_moduleCache.clear(); }

AstIfaceRefDType* V3LinkDotIfaceCapture::ifaceRefFromVarDType(Ast_गाँठो_DType* dtypep) {
    AstIfaceRefDType* resultp = nullptr;
    लागि (Ast_गाँठो_DType* curp = dtypep; curp;) {
        यदि (AstIfaceRefDType* अचर irefp = VN_CAST(curp, IfaceRefDType)) {
            resultp = irefp;
            रोक;
        } अन्यथा यदि (AstBracketArrayDType* अचर bracketp = VN_CAST(curp, BracketArrayDType)) {
            curp = bracketp->subDTypep();
        } अन्यथा यदि (AstUnpackArrayDType* अचर unpackp = VN_CAST(curp, UnpackArrayDType)) {
            curp = unpackp->subDTypep();
        } अन्यथा {
            v3fatalSrc("ifaceRefFromVarDType: unexpected dtype " << curp->prettyTypeName()
                                                                 << " in chain");
        }
    }
    फर्काऊ resultp;
}

नामाकाश {
// Resolve the owner module name for a typedef/paramType node.
// Returns hint if non-empty, otherwise walks backp() to find the owner module name.
string resolveOwnerName(अचर string& hint, Ast_गाँठो* nodep) {
    यदि (!hint.empty()) फर्काऊ hint;
    यदि (!nodep) फर्काऊ "";
    Ast_गाँठो_Module* अचर ownerp = V3LinkDotIfaceCapture::findOwnerModule(nodep);
    फर्काऊ ownerp ? ownerp->name() : string{};
}
}  // namespace

AstTypedef* V3LinkDotIfaceCapture::findTypedefInModule(Ast_गाँठो_Module* modp, अचर string& name) {
    AstTypedef* resultp = nullptr;
    अचर StmtNameMap& cache = getOrBuild(modp);
    अचर स्वतः it = cache.m_byName.find(name);
    यदि (!(it == cache.m_byName.end())) {
        लागि (Ast_गाँठो* nodep : it->second) {
            यदि (AstTypedef* अचर tdp = VN_CAST(nodep, Typedef)) {
                resultp = tdp;
                रोक;
            }
        }
    }
    फर्काऊ resultp;
}
Ast_गाँठो_DType* V3LinkDotIfaceCapture::findDTypeInModule(Ast_गाँठो_Module* modp, अचर string& name,
                                                       VNType type) {

    Ast_गाँठो_DType* resultp = nullptr;
    अचर StmtNameMap& cache = getOrBuild(modp);
    अचर स्वतः it = cache.m_byName.find(name);
    यदि (!(it == cache.m_byName.end())) {
        लागि (Ast_गाँठो* nodep : it->second) {
            यदि (Ast_गाँठो_DType* अचर dtp = VN_CAST(nodep, गाँठो_DType)) {
                यदि (dtp->type() == type) {
                    resultp = dtp;
                    रोक;
                }
            }
        }
    }
    फर्काऊ resultp;
}
AstParamTypeDType* V3LinkDotIfaceCapture::findParamTypeInModule(Ast_गाँठो_Module* modp,
                                                                अचर string& name) {

    AstParamTypeDType* resultp = nullptr;
    अचर StmtNameMap& cache = getOrBuild(modp);
    अचर स्वतः it = cache.m_byName.find(name);
    यदि (!(it == cache.m_byName.end())) {
        लागि (Ast_गाँठो* nodep : it->second) {
            यदि (AstParamTypeDType* अचर ptdp = VN_CAST(nodep, ParamTypeDType)) {
                resultp = ptdp;
                रोक;
            }
        }
    }
    फर्काऊ resultp;
}

तार्किक V3LinkDotIfaceCapture::retargetRefToModule(AstRefDType* refp, Ast_गाँठो_Module* targetModp) {
    अचर VIfaceCaptureTag* अचर tagp = refp->captureTagp();
    UASSERT_OBJ(tagp, refp, "Retarget of a reference that was not captured");
    UASSERT_OBJ(targetModp, refp, "Retarget to a null module");

    यदि (tagp->m_kind == VIfaceCaptureTag::Kind::PARAM_TYPE) {
        AstParamTypeDType* अचर paramTypep = findParamTypeInModule(targetModp, refp->name());
        यदि (!paramTypep) फर्काऊ असत्य;
        retargetRefToParamType(refp, paramTypep);
        फर्काऊ सत्य;
    }

    AstTypedef* अचर typedefp = findTypedefInModule(targetModp, refp->name());
    यदि (!typedefp) फर्काऊ असत्य;
    retargetRefToTypedef(refp, typedefp);
    फर्काऊ सत्य;
}

शून्य V3LinkDotIfaceCapture::retargetRefToParamType(AstRefDType* refp,
                                                   AstParamTypeDType* paramTypep) {
    UASSERT_OBJ(paramTypep, refp, "Retarget to a null parameter type");
    refp->refDTypep(paramTypep);
    refp->dtypep(paramTypep);
}

शून्य V3LinkDotIfaceCapture::retargetRefToTypedef(AstRefDType* refp, AstTypedef* typedefp) {
    UASSERT_OBJ(typedefp, refp, "Retarget to a null typedef");
    refp->typedefp(typedefp);
    // An incomplete typedef is still a successful name resolution, but
    // must not erase type links that a later width pass can complete.
    यदि (Ast_गाँठो_DType* अचर dtypep = typedefp->subDTypep()) {
        refp->refDTypep(dtypep);
        refp->dtypep(dtypep);
    }
}

नामाकाश {
using LiveNodes = std::unordered_set<अचर Ast_गाँठो*>;

// A scoped snapshot of every node currently in the tree.  V3Broken::isLinkable()
// cannot serve this role: its table is populated only while V3Broken::brokenAll()
// runs and is cleared before it returns.
LiveNodes collectLiveNodes() {
    LiveNodes liveNodes;
    v3Global.rootp()->foreach([&](Ast_गाँठो* nodep) { liveNodes.घुसाउ(nodep); });
    फर्काऊ liveNodes;
}

// Find the module that owns this node; a snapshot, if given, stops the walk at a stale link.
Ast_गाँठो_Module* findOwnerModuleImpl(Ast_गाँठो* nodep, अचर LiveNodes* liveNodesp) {
    लागि (Ast_गाँठो* curp = nodep; curp; curp = curp->backp()) {
        यदि (liveNodesp && !liveNodesp->गन्ती(curp)) फर्काऊ nullptr;
        यदि (Ast_गाँठो_Module* अचर modp = VN_CAST(curp, गाँठो_Module)) फर्काऊ modp;
    }
    फर्काऊ nullptr;
}

Ast_गाँठो_Module* findOwnerModuleIfLive(Ast_गाँठो* nodep, अचर LiveNodes& liveNodes) {
    फर्काऊ findOwnerModuleImpl(nodep, &liveNodes);
}

तार्किक moduleMatchesOwner(अचर Ast_गाँठो_Module* modp, अचर string& ownerName) {
    यदि (!modp || ownerName.empty()) फर्काऊ असत्य;
    फर्काऊ modp->name() == ownerName || modp->origName() == ownerName;
}
}  // namespace

Ast_गाँठो_Module* V3LinkDotIfaceCapture::findOwnerModule(Ast_गाँठो* nodep) {
    फर्काऊ findOwnerModuleImpl(nodep, nullptr);
}

शून्य V3LinkDotIfaceCapture::dumpEntries(अचर string& label) {
    UINFO(9, "========== iface capture dumpEntries: " << label << " ==========");
    पूर्णाङ्क idx = 0;
    v3Global.rootp()->foreach([&](AstRefDType* refp) {
        अचर VIfaceCaptureTag* अचर tagp = captureTag(refp);
        यदि (!tagp) फर्काऊ;
        अचर Ast_गाँठो_Module* अचर ownerModp = findOwnerModule(refp);
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

शून्य V3LinkDotIfaceCapture::tag(AstRefDType* refp, अचर Ast_गाँठो_Module* capturedInp,
                                VIfaceCaptureTag::Kind kind, अचर string& cellPath,
                                अचर string& ownerModName) {
    UASSERT_OBJ(!refp->captureTagp(), refp, "Reference captured twice");
    UASSERT_OBJ(capturedInp, refp, "Captured reference is not in a module");
    UASSERT_OBJ(!cellPath.empty(), refp, "Captured reference has no cell path");
    s_tags.emplace_back(VIfaceCaptureTag{kind, cellPath, ownerModName, capturedInp->origName()});
    refp->captureTagp(&s_tags.back());
}

अचर VIfaceCaptureTag* V3LinkDotIfaceCapture::captureTag(AstRefDType* refp) {
    अचर VIfaceCaptureTag* अचर tagp = refp->captureTagp();
    यदि (!tagp) फर्काऊ nullptr;
    अचर Ast_गाँठो_Module* अचर ownerp = findOwnerModule(refp);
    यदि (!ownerp || ownerp->origName() != tagp->m_capturedInName) फर्काऊ nullptr;
    फर्काऊ tagp;
}

// Walk a dot-separated cell path through the cell / IFACEREFDTYPE hierarchy
// starting from startModp.  Returns the module at the end of the path, or
// nullptr if any component cannot be resolved.
// Cell/port names are preserved across clones by cloneTree, so this works
// identically on template and cloned modules.
Ast_गाँठो_Module* V3LinkDotIfaceCapture::followCellPath(Ast_गाँठो_Module* startModp,
                                                     अचर string& cellPath) {
    यदि (cellPath.empty() || !startModp) फर्काऊ nullptr;
    Ast_गाँठो_Module* curModp = startModp;
    string remaining = cellPath;
    जबसम्म (!remaining.empty() && curModp) {
        string component;
        अचर माप_प्रकार dotPos = remaining.find('.');
        यदि (dotPos == string::npos) {
            component = remaining;
            remaining.clear();
        } अन्यथा {
            component = remaining.substr(0, dotPos);
            remaining = remaining.substr(dotPos + 1);
        }
        // Matched without any array index; the elements of an instance array (see V3Param) all
        // instantiate the same module, so any of them will do
        अचर string componentBase = Ast_गाँठो::nameNoArray(component);
        Ast_गाँठो_Module* nextModp = nullptr;
        लागि (Ast_गाँठो* sp = curModp->stmtsp(); sp; sp = sp->nextp()) {
            यदि (AstCell* अचर cellp = VN_CAST(sp, Cell)) {
                यदि ((cellp->name() == component
                     || Ast_गाँठो::nameNoArray(cellp->name()) == componentBase)
                    && cellp->modp()) {
                    nextModp = cellp->modp();
                    रोक;
                }
            }
            यदि (AstVar* अचर varp = VN_CAST(sp, Var)) {
                यदि (varp->isIfaceRef() && varp->subDTypep()) {
                    string varBaseName = varp->name();
                    अचर माप_प्रकार viftopPos = varBaseName.find("__Viftop");
                    यदि (viftopPos != string::npos) {
                        varBaseName = varBaseName.substr(0, viftopPos);
                    }
                    यदि (varBaseName == component
                        || Ast_गाँठो::nameNoArray(varBaseName) == componentBase) {
                        यदि (AstIfaceRefDType* अचर irefp
                            = ifaceRefFromVarDType(varp->subDTypep())) {
                            AstIface* अचर ifacep = irefp->ifaceViaCellp();
                            यदि (ifacep) {
                                nextModp = ifacep;
                                रोक;
                            }
                        }
                    }
                }
            }
        }
        curModp = nextModp;
    }
    फर्काऊ curModp;
}

// replaces the lambda used in V3LinkDot.cpp for iface capture
शून्य V3LinkDotIfaceCapture::captureTypedefContext(AstRefDType* refp, अचर वर्ण* stageLabel,
                                                  पूर्णाङ्क dotPos, अचर std::string& dotText,
                                                  VSymEnt* dotSymp, Ast_गाँठो_Module* modp,
                                                  अचर std::function<std::string()>& indentFn) {
    यदि (!enabled() || !refp) फर्काऊ;

    UINFO(9, indentFn() << "iface capture capture request stage=" << stageLabel
                        << " typedef=" << refp << " name=" << refp->name() << " dotPos=" << dotPos
                        << " dotText='" << dotText << "' dotSym=" << dotSymp);

    अचर AstCell* ifaceCellp = nullptr;
    यदि (dotSymp && VN_IS(dotSymp->nodep(), Cell)) {
        अचर AstCell* अचर cellp = VN_AS(dotSymp->nodep(), Cell);
        यदि (cellp->modp() && VN_IS(cellp->modp(), Iface)) ifaceCellp = cellp;
    }
    यदि (!ifaceCellp) {
        UINFO(9, indentFn() << "iface capture capture skipped typedef=" << refp
                            << " (no iface context)");
        फर्काऊ;
    }
    // Skip internal interface typedef references (typedef used within its own interface)
    यदि (ifaceCellp->modp() == modp) {
        UINFO(9, indentFn() << "iface capture capture skipped typedef=" << refp
                            << " (internal to interface " << modp->name() << ")");
        फर्काऊ;
    }

    // dotText is always non-empty for interface typedef captures.  If this
    // fires, the caller resolved to an interface Cell but did not accumulate
    // a dotText path - investigate the dot-state in visitParseRef.
    UASSERT_OBJ(!dotText.empty(), refp, "captureTypedefContext: dotText empty");
    अचर string cellPath = dotText;

    // A reference to a ParamTypeDType is captured as a parameter type, else as a typedef
    यदि (AstParamTypeDType* अचर paramTypep = VN_CAST(refp->refDTypep(), ParamTypeDType)) {
        V3LinkDotIfaceCapture::addParamType(refp, cellPath, modp, paramTypep, "");
    } अन्यथा {
        tag(refp, modp, VIfaceCaptureTag::Kind::TYPEDEF, cellPath,
            resolveOwnerName("", refp->typedefp()));
    }

    UINFO(9, indentFn() << "iface capture capture success typedef=" << refp
                        << " cell=" << ifaceCellp << " cellPath='" << cellPath << "'"
                        << " mod=" << (ifaceCellp->modp() ? ifaceCellp->modp()->name() : "<null>")
                        << " dotPos=" << dotPos);
}

शून्य V3LinkDotIfaceCapture::addParamType(AstRefDType* refp, अचर string& cellPath,
                                         Ast_गाँठो_Module* ownerModp, AstParamTypeDType* paramTypep,
                                         अचर string& paramTypeOwnerModName) {
    UASSERT(refp, "addParamType() called with null refp");
    UASSERT(ownerModp,
            "addParamType() called with null ownerModp for refp='" << refp->prettyNameQ() << "'");
    UASSERT_OBJ(paramTypep, refp,
                "addParamType() called with null paramTypep for refp='" << refp->prettyNameQ()
                                                                        << "'");
    अचर string ptOwnerName = resolveOwnerName(paramTypeOwnerModName, paramTypep);
    UINFO(9, "addParamType: refp=" << refp << " cellPath='" << cellPath << "'"
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
    tag(refp, ownerModp, VIfaceCaptureTag::Kind::PARAM_TYPE, cellPath, ptOwnerName);
}

पूर्णाङ्क V3LinkDotIfaceCapture::resolveCapturedRefs() {
    पूर्णाङ्क fixed = 0;

    // TARGET RESOLUTION.  By this point all cloning is complete and cell
    // pointers are wired to the correct interface clones.  For each tagged
    // reference we walk its cell path from its owner module to find the
    // correct target module, then locate the PARAMTYPEDTYPE / TYPEDEF by name.
    // See the ARCHITECTURE comment above for the full picture.

    v3Global.rootp()->foreach([&](AstRefDType* refp) {
        अचर VIfaceCaptureTag* अचर tagp = captureTag(refp);
        यदि (!tagp) फर्काऊ;
        Ast_गाँठो_Module* अचर ownerModp = findOwnerModule(refp);
        यदि (!ownerModp || ownerModp->dead() || VN_IS(ownerModp, Package)) फर्काऊ;

        UINFO(9, "finalizeIfaceCapture Phase3 entry: refp="
                     << refp->name() << " (" << cvtToHex(refp) << ")" << " ownerMod="
                     << ownerModp->name() << " cellPath='" << tagp->m_cellPath << "' targetKind="
                     << (tagp->m_kind == VIfaceCaptureTag::Kind::PARAM_TYPE ? "param type"
                                                                            : "typedef"));

        // Prefer the owner itself when its stable template identity matches the
        // captured target owner.  Otherwise resolve and validate the cell path.
        Ast_गाँठो_Module* correctModp = nullptr;
        यदि (moduleMatchesOwner(ownerModp, tagp->m_ownerModName)) {
            correctModp = ownerModp;
        } अन्यथा {
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
        refp->user3(सत्य);
        ++fixed;
    });

    फर्काऊ fixed;
}

शून्य V3LinkDotIfaceCapture::verifyNoDeadRefs(अचर LiveNodes& liveNodes) {
    // Assert: no REFDTYPE in any live module should have typedefp or refDTypep
    // pointing to a dead module.
    लागि (Ast_गाँठो* nodep = v3Global.rootp()->modulesp(); nodep; nodep = nodep->nextp()) {
        यदि (Ast_गाँठो_Module* अचर modp = VN_CAST(nodep, गाँठो_Module)) {
            यदि (modp->dead()) जारी;
            modp->foreach([&](AstRefDType* refp) {
                यदि (refp->typedefp()) {
                    UASSERT_OBJ(liveNodes.गन्ती(refp->typedefp()), refp,
                                "REFDTYPE '" << refp->prettyNameQ() << "' in live module '"
                                             << modp->prettyNameQ()
                                             << "' has a dangling typedefp");
                    Ast_गाँठो_Module* अचर ownerModp
                        = findOwnerModuleIfLive(refp->typedefp(), liveNodes);
                    UASSERT_OBJ(!ownerModp || !ownerModp->dead(), refp,
                                "REFDTYPE '" << refp->prettyNameQ() << "' in live module '"
                                             << modp->prettyNameQ()
                                             << "' has typedefp pointing to dead module '"
                                             << ownerModp->prettyNameQ() << "'");
                }
                यदि (refp->refDTypep()) {
                    UASSERT_OBJ(liveNodes.गन्ती(refp->refDTypep()), refp,
                                "REFDTYPE '" << refp->prettyNameQ() << "' in live module '"
                                             << modp->prettyNameQ()
                                             << "' has a dangling refDTypep");
                    Ast_गाँठो_Module* अचर ownerModp
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
    यदि (v3Global.rootp()->typeTablep()) {
        लागि (Ast_गाँठो* nodep = v3Global.rootp()->typeTablep()->typesp(); nodep;
             nodep = nodep->nextp()) {
            nodep->foreach([&](AstRefDType* refp) {
                यदि (refp->typedefp()) {
                    UASSERT_OBJ(liveNodes.गन्ती(refp->typedefp()), refp,
                                "REFDTYPE '" << refp->prettyNameQ()
                                             << "' in type table has a dangling typedefp");
                    Ast_गाँठो_Module* अचर ownerModp
                        = findOwnerModuleIfLive(refp->typedefp(), liveNodes);
                    UASSERT_OBJ(!ownerModp || !ownerModp->dead(), refp,
                                "REFDTYPE '"
                                    << refp->prettyNameQ()
                                    << "' in type table has typedefp pointing to dead module '"
                                    << ownerModp->prettyNameQ() << "'");
                }
                यदि (refp->refDTypep()) {
                    UASSERT_OBJ(liveNodes.गन्ती(refp->refDTypep()), refp,
                                "REFDTYPE '" << refp->prettyNameQ()
                                             << "' in type table has a dangling refDTypep");
                    Ast_गाँठो_Module* अचर ownerModp
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

शून्य V3LinkDotIfaceCapture::finalizeIfaceCapture() {
    यदि (!s_enabled) फर्काऊ;
    UINFO(4, "finalizeIfaceCapture: fixing remaining cross-interface refs");
    यदि (!v3Global.rootp()) फर्काऊ;
    clearModuleCache();  // Ensure fresh view after all cloning/widthing

    // Resolve live captured refs from stable path metadata before inspecting
    // any inherited target pointers, which may refer to replaced template nodes.
    अचर पूर्णाङ्क capturedFixed = resolveCapturedRefs();
    UINFO(4, "finalizeIfaceCapture: structurally resolved " << capturedFixed << " captured refs");

    यदि (debug() >= 9) dumpEntries("after finalizeIfaceCapture");

    // Emit statistics for --stats
    V3Stats::addStat("IfaceCapture, Captured refs", s_tags.size());
    V3Stats::addStat("IfaceCapture, Captured refs resolved", capturedFixed);

    // No reference may be left pointing into a module about to be removed
    यदि (v3Global.opt.debugCheck()) verifyNoDeadRefs(collectLiveNodes());
    // Drop every tag before its record is freed.
    v3Global.rootp()->foreach([](AstRefDType* refp) { refp->captureTagp(nullptr); });
    reset();
}
