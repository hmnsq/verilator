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

#شامل "V3LinkDotIfaceCapture.h"

#شامل "V3Error.h"
#شامل "V3Global.h"
#شامل "V3Stats.h"
#شامل "V3SymTable.h"

#شامل <unordered_map>
#شامل <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

std::deque<VIfaceCaptureTag> V3LinkDotIfaceCapture::s_tags{};
منطقی V3LinkDotIfaceCapture::s_enabled = درست;

// LCOV_EXCL_START
تهی V3LinkDotIfaceCapture::enable(منطقی flag) {
    s_enabled = flag;
    اگر (!flag) clearModuleCache();
}
// LCOV_EXCL_STOP

تهی V3LinkDotIfaceCapture::reset() {
    s_tags.clear();
    clearModuleCache();
}

// Per-module cache of statement-level names to avoid O(N*M) linear scans.
// Lazily built on first access for a given module; cleared at phase boundaries.
// Uses vectors per name to handle rare cases where different node types share a name
// (e.g. a Typedef and a ParamTypeDType both named 'sc_tag_status_t').
فضاینام {
ساختار StmtNameMap final {
    std::unordered_map<string, std::vector<Ast_گره*>> m_byName;
};
std::unordered_map<Ast_گره_Module*, StmtNameMap> s_moduleCache;

ثابت StmtNameMap& getOrBuild(Ast_گره_Module* modp) {
    auto it = s_moduleCache.find(modp);
    اگر (it != s_moduleCache.end()) بازگشت it->second;
    StmtNameMap& cache = s_moduleCache[modp];
    برای (Ast_گره* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
        ثابت string& nm = stmtp->name();
        اگر (!nm.empty()) cache.m_byName[nm].افزودن_back(stmtp);
    }
    بازگشت cache;
}
}  // namespace

تهی V3LinkDotIfaceCapture::clearModuleCache() { s_moduleCache.clear(); }

AstIfaceRefDType* V3LinkDotIfaceCapture::ifaceRefFromVarDType(Ast_گره_DType* dtypep) {
    AstIfaceRefDType* resultp = nullptr;
    برای (Ast_گره_DType* curp = dtypep; curp;) {
        اگر (AstIfaceRefDType* ثابت irefp = VN_CAST(curp, IfaceRefDType)) {
            resultp = irefp;
            بشکن;
        } وگرنه اگر (AstBracketArrayDType* ثابت bracketp = VN_CAST(curp, BracketArrayDType)) {
            curp = bracketp->subDTypep();
        } وگرنه اگر (AstUnpackArrayDType* ثابت unpackp = VN_CAST(curp, UnpackArrayDType)) {
            curp = unpackp->subDTypep();
        } وگرنه {
            v3fatalSrc("ifaceRefFromVarDType: unexpected dtype " << curp->prettyTypeName()
                                                                 << " in chain");
        }
    }
    بازگشت resultp;
}

فضاینام {
// Resolve the owner module name for a typedef/paramType node.
// Returns hint if non-empty, otherwise walks backp() to find the owner module name.
string resolveOwnerName(ثابت string& hint, Ast_گره* nodep) {
    اگر (!hint.empty()) بازگشت hint;
    اگر (!nodep) بازگشت "";
    Ast_گره_Module* ثابت ownerp = V3LinkDotIfaceCapture::findOwnerModule(nodep);
    بازگشت ownerp ? ownerp->name() : string{};
}
}  // namespace

AstTypedef* V3LinkDotIfaceCapture::findTypedefInModule(Ast_گره_Module* modp, ثابت string& name) {
    AstTypedef* resultp = nullptr;
    ثابت StmtNameMap& cache = getOrBuild(modp);
    ثابت auto it = cache.m_byName.find(name);
    اگر (!(it == cache.m_byName.end())) {
        برای (Ast_گره* nodep : it->second) {
            اگر (AstTypedef* ثابت tdp = VN_CAST(nodep, Typedef)) {
                resultp = tdp;
                بشکن;
            }
        }
    }
    بازگشت resultp;
}
Ast_گره_DType* V3LinkDotIfaceCapture::findDTypeInModule(Ast_گره_Module* modp, ثابت string& name,
                                                       VNType type) {

    Ast_گره_DType* resultp = nullptr;
    ثابت StmtNameMap& cache = getOrBuild(modp);
    ثابت auto it = cache.m_byName.find(name);
    اگر (!(it == cache.m_byName.end())) {
        برای (Ast_گره* nodep : it->second) {
            اگر (Ast_گره_DType* ثابت dtp = VN_CAST(nodep, گره_DType)) {
                اگر (dtp->type() == type) {
                    resultp = dtp;
                    بشکن;
                }
            }
        }
    }
    بازگشت resultp;
}
AstParamTypeDType* V3LinkDotIfaceCapture::findParamTypeInModule(Ast_گره_Module* modp,
                                                                ثابت string& name) {

    AstParamTypeDType* resultp = nullptr;
    ثابت StmtNameMap& cache = getOrBuild(modp);
    ثابت auto it = cache.m_byName.find(name);
    اگر (!(it == cache.m_byName.end())) {
        برای (Ast_گره* nodep : it->second) {
            اگر (AstParamTypeDType* ثابت ptdp = VN_CAST(nodep, ParamTypeDType)) {
                resultp = ptdp;
                بشکن;
            }
        }
    }
    بازگشت resultp;
}

منطقی V3LinkDotIfaceCapture::retargetRefToModule(AstRefDType* refp, Ast_گره_Module* targetModp) {
    ثابت VIfaceCaptureTag* ثابت tagp = refp->captureTagp();
    UASSERT_OBJ(tagp, refp, "Retarget of a reference that was not captured");
    UASSERT_OBJ(targetModp, refp, "Retarget to a null module");

    اگر (tagp->m_kind == VIfaceCaptureTag::Kind::PARAM_TYPE) {
        AstParamTypeDType* ثابت paramTypep = findParamTypeInModule(targetModp, refp->name());
        اگر (!paramTypep) بازگشت نادرست;
        retargetRefToParamType(refp, paramTypep);
        بازگشت درست;
    }

    AstTypedef* ثابت typedefp = findTypedefInModule(targetModp, refp->name());
    اگر (!typedefp) بازگشت نادرست;
    retargetRefToTypedef(refp, typedefp);
    بازگشت درست;
}

تهی V3LinkDotIfaceCapture::retargetRefToParamType(AstRefDType* refp,
                                                   AstParamTypeDType* paramTypep) {
    UASSERT_OBJ(paramTypep, refp, "Retarget to a null parameter type");
    refp->refDTypep(paramTypep);
    refp->dtypep(paramTypep);
}

تهی V3LinkDotIfaceCapture::retargetRefToTypedef(AstRefDType* refp, AstTypedef* typedefp) {
    UASSERT_OBJ(typedefp, refp, "Retarget to a null typedef");
    refp->typedefp(typedefp);
    // An incomplete typedef is still a successful name resolution, but
    // must not erase type links that a later width pass can complete.
    اگر (Ast_گره_DType* ثابت dtypep = typedefp->subDTypep()) {
        refp->refDTypep(dtypep);
        refp->dtypep(dtypep);
    }
}

فضاینام {
using LiveNodes = std::unordered_set<ثابت Ast_گره*>;

// A scoped snapshot of every node currently in the tree.  V3Broken::isLinkable()
// cannot serve this role: its table is populated only while V3Broken::brokenAll()
// runs and is cleared before it returns.
LiveNodes collectLiveNodes() {
    LiveNodes liveNodes;
    v3Global.rootp()->foreach([&](Ast_گره* nodep) { liveNodes.درج(nodep); });
    بازگشت liveNodes;
}

// Find the module that owns this node; a snapshot, if given, stops the walk at a stale link.
Ast_گره_Module* findOwnerModuleImpl(Ast_گره* nodep, ثابت LiveNodes* liveNodesp) {
    برای (Ast_گره* curp = nodep; curp; curp = curp->backp()) {
        اگر (liveNodesp && !liveNodesp->شمار(curp)) بازگشت nullptr;
        اگر (Ast_گره_Module* ثابت modp = VN_CAST(curp, گره_Module)) بازگشت modp;
    }
    بازگشت nullptr;
}

Ast_گره_Module* findOwnerModuleIfLive(Ast_گره* nodep, ثابت LiveNodes& liveNodes) {
    بازگشت findOwnerModuleImpl(nodep, &liveNodes);
}

منطقی moduleMatchesOwner(ثابت Ast_گره_Module* modp, ثابت string& ownerName) {
    اگر (!modp || ownerName.empty()) بازگشت نادرست;
    بازگشت modp->name() == ownerName || modp->origName() == ownerName;
}
}  // namespace

Ast_گره_Module* V3LinkDotIfaceCapture::findOwnerModule(Ast_گره* nodep) {
    بازگشت findOwnerModuleImpl(nodep, nullptr);
}

تهی V3LinkDotIfaceCapture::dumpEntries(ثابت string& label) {
    UINFO(9, "========== iface capture dumpEntries: " << label << " ==========");
    صحیح idx = 0;
    v3Global.rootp()->foreach([&](AstRefDType* refp) {
        ثابت VIfaceCaptureTag* ثابت tagp = captureTag(refp);
        اگر (!tagp) بازگشت;
        ثابت Ast_گره_Module* ثابت ownerModp = findOwnerModule(refp);
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

تهی V3LinkDotIfaceCapture::tag(AstRefDType* refp, ثابت Ast_گره_Module* capturedInp,
                                VIfaceCaptureTag::Kind kind, ثابت string& cellPath,
                                ثابت string& ownerModName) {
    UASSERT_OBJ(!refp->captureTagp(), refp, "Reference captured twice");
    UASSERT_OBJ(capturedInp, refp, "Captured reference is not in a module");
    UASSERT_OBJ(!cellPath.empty(), refp, "Captured reference has no cell path");
    s_tags.emplace_back(VIfaceCaptureTag{kind, cellPath, ownerModName, capturedInp->origName()});
    refp->captureTagp(&s_tags.back());
}

ثابت VIfaceCaptureTag* V3LinkDotIfaceCapture::captureTag(AstRefDType* refp) {
    ثابت VIfaceCaptureTag* ثابت tagp = refp->captureTagp();
    اگر (!tagp) بازگشت nullptr;
    ثابت Ast_گره_Module* ثابت ownerp = findOwnerModule(refp);
    اگر (!ownerp || ownerp->origName() != tagp->m_capturedInName) بازگشت nullptr;
    بازگشت tagp;
}

// Walk a dot-separated cell path through the cell / IFACEREFDTYPE hierarchy
// starting from startModp.  Returns the module at the end of the path, or
// nullptr if any component cannot be resolved.
// Cell/port names are preserved across clones by cloneTree, so this works
// identically on template and cloned modules.
Ast_گره_Module* V3LinkDotIfaceCapture::followCellPath(Ast_گره_Module* startModp,
                                                     ثابت string& cellPath) {
    اگر (cellPath.empty() || !startModp) بازگشت nullptr;
    Ast_گره_Module* curModp = startModp;
    string remaining = cellPath;
    وقتی (!remaining.empty() && curModp) {
        string component;
        ثابت size_t dotPos = remaining.find('.');
        اگر (dotPos == string::npos) {
            component = remaining;
            remaining.clear();
        } وگرنه {
            component = remaining.substr(0, dotPos);
            remaining = remaining.substr(dotPos + 1);
        }
        // Matched without any array index; the elements of an instance array (see V3Param) all
        // instantiate the same module, so any of them will do
        ثابت string componentBase = Ast_گره::nameNoArray(component);
        Ast_گره_Module* nextModp = nullptr;
        برای (Ast_گره* sp = curModp->stmtsp(); sp; sp = sp->nextp()) {
            اگر (AstCell* ثابت cellp = VN_CAST(sp, Cell)) {
                اگر ((cellp->name() == component
                     || Ast_گره::nameNoArray(cellp->name()) == componentBase)
                    && cellp->modp()) {
                    nextModp = cellp->modp();
                    بشکن;
                }
            }
            اگر (AstVar* ثابت varp = VN_CAST(sp, Var)) {
                اگر (varp->isIfaceRef() && varp->subDTypep()) {
                    string varBaseName = varp->name();
                    ثابت size_t viftopPos = varBaseName.find("__Viftop");
                    اگر (viftopPos != string::npos) {
                        varBaseName = varBaseName.substr(0, viftopPos);
                    }
                    اگر (varBaseName == component
                        || Ast_گره::nameNoArray(varBaseName) == componentBase) {
                        اگر (AstIfaceRefDType* ثابت irefp
                            = ifaceRefFromVarDType(varp->subDTypep())) {
                            AstIface* ثابت ifacep = irefp->ifaceViaCellp();
                            اگر (ifacep) {
                                nextModp = ifacep;
                                بشکن;
                            }
                        }
                    }
                }
            }
        }
        curModp = nextModp;
    }
    بازگشت curModp;
}

// replaces the lambda used in V3LinkDot.cpp for iface capture
تهی V3LinkDotIfaceCapture::captureTypedefContext(AstRefDType* refp, ثابت نویسه* stageLabel,
                                                  صحیح dotPos, ثابت std::string& dotText,
                                                  VSymEnt* dotSymp, Ast_گره_Module* modp,
                                                  ثابت std::function<std::string()>& indentFn) {
    اگر (!enabled() || !refp) بازگشت;

    UINFO(9, indentFn() << "iface capture capture request stage=" << stageLabel
                        << " typedef=" << refp << " name=" << refp->name() << " dotPos=" << dotPos
                        << " dotText='" << dotText << "' dotSym=" << dotSymp);

    ثابت AstCell* ifaceCellp = nullptr;
    اگر (dotSymp && VN_IS(dotSymp->nodep(), Cell)) {
        ثابت AstCell* ثابت cellp = VN_AS(dotSymp->nodep(), Cell);
        اگر (cellp->modp() && VN_IS(cellp->modp(), Iface)) ifaceCellp = cellp;
    }
    اگر (!ifaceCellp) {
        UINFO(9, indentFn() << "iface capture capture skipped typedef=" << refp
                            << " (no iface context)");
        بازگشت;
    }
    // Skip internal interface typedef references (typedef used within its own interface)
    اگر (ifaceCellp->modp() == modp) {
        UINFO(9, indentFn() << "iface capture capture skipped typedef=" << refp
                            << " (internal to interface " << modp->name() << ")");
        بازگشت;
    }

    // dotText is always non-empty for interface typedef captures.  If this
    // fires, the caller resolved to an interface Cell but did not accumulate
    // a dotText path - investigate the dot-state in visitParseRef.
    UASSERT_OBJ(!dotText.empty(), refp, "captureTypedefContext: dotText empty");
    ثابت string cellPath = dotText;

    // A reference to a ParamTypeDType is captured as a parameter type, else as a typedef
    اگر (AstParamTypeDType* ثابت paramTypep = VN_CAST(refp->refDTypep(), ParamTypeDType)) {
        V3LinkDotIfaceCapture::addParamType(refp, cellPath, modp, paramTypep, "");
    } وگرنه {
        tag(refp, modp, VIfaceCaptureTag::Kind::TYPEDEF, cellPath,
            resolveOwnerName("", refp->typedefp()));
    }

    UINFO(9, indentFn() << "iface capture capture success typedef=" << refp
                        << " cell=" << ifaceCellp << " cellPath='" << cellPath << "'"
                        << " mod=" << (ifaceCellp->modp() ? ifaceCellp->modp()->name() : "<null>")
                        << " dotPos=" << dotPos);
}

تهی V3LinkDotIfaceCapture::addParamType(AstRefDType* refp, ثابت string& cellPath,
                                         Ast_گره_Module* ownerModp, AstParamTypeDType* paramTypep,
                                         ثابت string& paramTypeOwnerModName) {
    UASSERT(refp, "addParamType() called with null refp");
    UASSERT(ownerModp,
            "addParamType() called with null ownerModp for refp='" << refp->prettyNameQ() << "'");
    UASSERT_OBJ(paramTypep, refp,
                "addParamType() called with null paramTypep for refp='" << refp->prettyNameQ()
                                                                        << "'");
    ثابت string ptOwnerName = resolveOwnerName(paramTypeOwnerModName, paramTypep);
    UINFO(9, "addParamType: refp=" << refp << " cellPath='" << cellPath << "'"
                                   << " ownerModp=" << (ownerModp ? ownerModp->name() : "<null>")
                                   << " paramTypep=" << paramTypep << " paramTypeOwnerModName='"
                                   << ptOwnerName << "'");
    UINFO(9, "addParamType: paramTypep subDTypep chain:");
    اگر (debug())
        paramTypep->foreach([&](AstRefDType* innerRefp) {
            UINFO(9,
                  "  inner RefDType: "
                      << innerRefp << " refDTypep=" << innerRefp->refDTypep()
                      << (innerRefp->refDTypep() ? " refDTypep->name=" : "")
                      << (innerRefp->refDTypep() ? innerRefp->refDTypep()->prettyTypeName() : ""));
        });
    tag(refp, ownerModp, VIfaceCaptureTag::Kind::PARAM_TYPE, cellPath, ptOwnerName);
}

صحیح V3LinkDotIfaceCapture::resolveCapturedRefs() {
    صحیح fixed = 0;

    // TARGET RESOLUTION.  By this point all cloning is complete and cell
    // pointers are wired to the correct interface clones.  For each tagged
    // reference we walk its cell path from its owner module to find the
    // correct target module, then locate the PARAMTYPEDTYPE / TYPEDEF by name.
    // See the ARCHITECTURE comment above for the full picture.

    v3Global.rootp()->foreach([&](AstRefDType* refp) {
        ثابت VIfaceCaptureTag* ثابت tagp = captureTag(refp);
        اگر (!tagp) بازگشت;
        Ast_گره_Module* ثابت ownerModp = findOwnerModule(refp);
        اگر (!ownerModp || ownerModp->dead() || VN_IS(ownerModp, Package)) بازگشت;

        UINFO(9, "finalizeIfaceCapture Phase3 entry: refp="
                     << refp->name() << " (" << cvtToHex(refp) << ")" << " ownerMod="
                     << ownerModp->name() << " cellPath='" << tagp->m_cellPath << "' targetKind="
                     << (tagp->m_kind == VIfaceCaptureTag::Kind::PARAM_TYPE ? "param type"
                                                                            : "typedef"));

        // Prefer the owner itself when its stable template identity matches the
        // captured target owner.  Otherwise resolve and validate the cell path.
        Ast_گره_Module* correctModp = nullptr;
        اگر (moduleMatchesOwner(ownerModp, tagp->m_ownerModName)) {
            correctModp = ownerModp;
        } وگرنه {
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
        refp->user3(درست);
        ++fixed;
    });

    بازگشت fixed;
}

تهی V3LinkDotIfaceCapture::verifyNoDeadRefs(ثابت LiveNodes& liveNodes) {
    // Assert: no REFDTYPE in any live module should have typedefp or refDTypep
    // pointing to a dead module.
    برای (Ast_گره* nodep = v3Global.rootp()->modulesp(); nodep; nodep = nodep->nextp()) {
        اگر (Ast_گره_Module* ثابت modp = VN_CAST(nodep, گره_Module)) {
            اگر (modp->dead()) ادامه;
            modp->foreach([&](AstRefDType* refp) {
                اگر (refp->typedefp()) {
                    UASSERT_OBJ(liveNodes.شمار(refp->typedefp()), refp,
                                "REFDTYPE '" << refp->prettyNameQ() << "' in live module '"
                                             << modp->prettyNameQ()
                                             << "' has a dangling typedefp");
                    Ast_گره_Module* ثابت ownerModp
                        = findOwnerModuleIfLive(refp->typedefp(), liveNodes);
                    UASSERT_OBJ(!ownerModp || !ownerModp->dead(), refp,
                                "REFDTYPE '" << refp->prettyNameQ() << "' in live module '"
                                             << modp->prettyNameQ()
                                             << "' has typedefp pointing to dead module '"
                                             << ownerModp->prettyNameQ() << "'");
                }
                اگر (refp->refDTypep()) {
                    UASSERT_OBJ(liveNodes.شمار(refp->refDTypep()), refp,
                                "REFDTYPE '" << refp->prettyNameQ() << "' in live module '"
                                             << modp->prettyNameQ()
                                             << "' has a dangling refDTypep");
                    Ast_گره_Module* ثابت ownerModp
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
    اگر (v3Global.rootp()->typeTablep()) {
        برای (Ast_گره* nodep = v3Global.rootp()->typeTablep()->typesp(); nodep;
             nodep = nodep->nextp()) {
            nodep->foreach([&](AstRefDType* refp) {
                اگر (refp->typedefp()) {
                    UASSERT_OBJ(liveNodes.شمار(refp->typedefp()), refp,
                                "REFDTYPE '" << refp->prettyNameQ()
                                             << "' in type table has a dangling typedefp");
                    Ast_گره_Module* ثابت ownerModp
                        = findOwnerModuleIfLive(refp->typedefp(), liveNodes);
                    UASSERT_OBJ(!ownerModp || !ownerModp->dead(), refp,
                                "REFDTYPE '"
                                    << refp->prettyNameQ()
                                    << "' in type table has typedefp pointing to dead module '"
                                    << ownerModp->prettyNameQ() << "'");
                }
                اگر (refp->refDTypep()) {
                    UASSERT_OBJ(liveNodes.شمار(refp->refDTypep()), refp,
                                "REFDTYPE '" << refp->prettyNameQ()
                                             << "' in type table has a dangling refDTypep");
                    Ast_گره_Module* ثابت ownerModp
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

تهی V3LinkDotIfaceCapture::finalizeIfaceCapture() {
    اگر (!s_enabled) بازگشت;
    UINFO(4, "finalizeIfaceCapture: fixing remaining cross-interface refs");
    اگر (!v3Global.rootp()) بازگشت;
    clearModuleCache();  // Ensure fresh view after all cloning/widthing

    // Resolve live captured refs from stable path metadata before inspecting
    // any inherited target pointers, which may refer to replaced template nodes.
    ثابت صحیح capturedFixed = resolveCapturedRefs();
    UINFO(4, "finalizeIfaceCapture: structurally resolved " << capturedFixed << " captured refs");

    اگر (debug() >= 9) dumpEntries("after finalizeIfaceCapture");

    // Emit statistics for --stats
    V3Stats::addStat("IfaceCapture, Captured refs", s_tags.size());
    V3Stats::addStat("IfaceCapture, Captured refs resolved", capturedFixed);

    // No reference may be left pointing into a module about to be removed
    اگر (v3Global.opt.debugCheck()) verifyNoDeadRefs(collectLiveNodes());
    // Drop every tag before its record is freed.
    v3Global.rootp()->foreach([](AstRefDType* refp) { refp->captureTagp(nullptr); });
    reset();
}
