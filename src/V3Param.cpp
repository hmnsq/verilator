// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Replicate modules for parameterization
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
// PARAM TRANSFORMATIONS:
//   Top down traversal:
//      For each cell:
//          If parameterized,
//              Determine all parameter widths, constant values.
//              (Interfaces also matter, as if a module is parameterized
//              this effectively changes the width behavior of all that
//              reference the iface.)
//
//              Clone module cell calls, renaming with __{par1}_{par2}_...
//              Substitute constants for cell's module's parameters.
//              Relink pins and cell and ifacerefdtype to point to new module.
//
//              For interface Parent's we have the AstIfaceRefDType::cellp()
//              pointing to this module.  If that parent cell's interface
//              module gets parameterized, AstIfaceRefDType::cloneRelink
//              will update AstIfaceRefDType::cellp(), and V3LinkDot will
//              see the new interface.
//
//              However if a submodule's AstIfaceRefDType::ifacep() points
//              to the old (unparameterized) interface and needs correction.
//              To detect this we must walk all pins looking for interfaces
//              that the parent has changed and propagate down.
//
//          Then process all modules called by that cell.
//          (Cells never referenced after parameters expanded must be ignored.)
//
//      Arrayed instances of modules and interfaces are expanded into their
//      elements before the above, each element knowing its position in the
//      array (for V3Width to select its part of the port connections).
//      Interface array ports are split into a variable for each element.
//
//   After we complete parameters, the varp's will be wrong (point to old module)
//   and must be relinked.
//
//*************************************************************************

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3Param.h"

#अन्तर्भाव "V3Case.h"
#अन्तर्भाव "V3Const.h"
#अन्तर्भाव "V3EmitV.h"
#अन्तर्भाव "V3HierBlock.h"
#अन्तर्भाव "V3LinkDotIfaceCapture.h"
#अन्तर्भाव "V3MemberMap.h"
#अन्तर्भाव "V3Os.h"
#अन्तर्भाव "V3Parse.h"
#अन्तर्भाव "V3Simulate.h"
#अन्तर्भाव "V3Stats.h"
#अन्तर्भाव "V3Unroll.h"
#अन्तर्भाव "V3Width.h"

#अन्तर्भाव <cctype>
#अन्तर्भाव <cstdlib>
#अन्तर्भाव <deque>
#अन्तर्भाव <map>
#अन्तर्भाव <memory>
#अन्तर्भाव <set>
#अन्तर्भाव <unordered_map>
#अन्तर्भाव <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

// Walk a class-typed node through typedef/RefDType chains to its ClassRefDType, or null.
स्थिर AstClassRefDType* class_Ref_DType_Of_ग्रन्थिः(Ast_ग्रन्थिः* nodep) {
    याव (AstTypedef* ध्रुव tdp = VN_CAST(nodep, Typedef)) nodep = tdp->subDTypep();
    Ast_ग्रन्थिः_DType* ध्रुव dtp = VN_CAST(nodep, ग्रन्थिः_DType);
    पटिनिवत्त dtp ? VN_CAST(dtp->skipRefOrNullp(), ClassRefDType) : nullptr;
}

//######################################################################
// Hierarchical block and parameter db (modules without parameters are also handled)

वग्ग ParameterizedHierBlocks final {
    using HierBlockOptsByOrigName = std::multimap<std::string, ध्रुव V3HierarchicalBlockOption*>;
    using HierMapIt = HierBlockOptsByOrigName::const_iterator;
    using HierBlockModMap = std::map<ध्रुव std::string, Ast_ग्रन्थिः_Module*>;
    using ParamConstMap = std::map<ध्रुव std::string, std::unique_ptr<AstConst>>;
    using GParamsMap = std::map<ध्रुव std::string, AstVar*>;  // key:parameter name value:parameter

    // MEMBERS
    ध्रुव तार्किक m_hierSubRun;  // Is in sub-run for hierarchical verilation
    // key:Original module name, value:HiearchyBlockOption*
    // If a module is parameterized, the module is uniquified to overridden parameters.
    // This is why HierBlockOptsByOrigName is multimap.
    HierBlockOptsByOrigName m_hierBlockOptsByOrigName;
    // key:mangled module name, value:AstNodeModule*
    HierBlockModMap m_hierBlockMod;
    // Overridden parameters of the hierarchical block
    std::map<ध्रुव V3HierarchicalBlockOption*, ParamConstMap> m_hierParams;
    // Parameter variables of hierarchical blocks
    std::map<ध्रुव std::string, GParamsMap> m_modParams;
    // Formals declared without a type, whose type is that of the assigned value
    std::set<ध्रुव AstVar*> m_untypedParams;

    // METHODS

सार्वजनिक:
    ParameterizedHierBlocks(ध्रुव V3HierBlockOptSet& hierOpts, AstNetlist* nodep)
        // A type parameter file only describes a child's own types; the final consolidation
        // run has none, but must still substitute the libraries it consumes.
        : m_hierSubRun{!v3Global.opt.hierBlocks().empty() || v3Global.opt.hierChild()} {
        अत्थाय (ध्रुव auto& hierOpt : hierOpts) {
            m_hierBlockOptsByOrigName.emplace(hierOpt.second.origName(), &hierOpt.second);
            ध्रुव V3HierarchicalBlockOption::ParamStrMap& params = hierOpt.second.params();
            ParamConstMap& consts = m_hierParams[&hierOpt.second];
            अत्थाय (V3HierarchicalBlockOption::ParamStrMap::const_iterator pIt = params.begin();
                 pIt != params.end(); ++pIt) {
                std::unique_ptr<AstConst> constp{AstConst::parseParamLiteral(
                    नूतन File_पङ्क्तिः{File_पङ्क्तिः::builtInFilename()}, pIt->second)};
                UASSERT(constp, pIt->second << " is not a valid parameter literal");
                ध्रुव तार्किक inserted = consts.emplace(pIt->first, std::move(constp)).second;
                UASSERT(inserted, pIt->first << " is already added");
            }
            // origName may be already registered, but it's fine.
            m_modParams.निवेशय({hierOpt.second.origName(), {}});
        }
        अत्थाय (Ast_ग्रन्थिः_Module* modp = nodep->modulesp(); modp;
             modp = VN_AS(modp->nextp(), ग्रन्थिः_Module)) {
            सचे (hierOpts.find(modp->prettyName()) != hierOpts.end()) {
                m_hierBlockMod.emplace(modp->name(), modp);
            }
            // Recursive hierarchical module may change its name, so we have to match its origName.
            // Collect values from recursive cloned module as parameters in the top module could be
            // overridden.
            ध्रुव string actualModName = modp->recursiveClone() ? modp->origName() : modp->name();
            ध्रुव auto defParamIt = m_modParams.find(actualModName);
            सचे (defParamIt != m_modParams.end()) {
                // modp is the original of parameterized hierarchical block
                अत्थाय (Ast_ग्रन्थिः* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    सचे (AstVar* ध्रुव varp = VN_CAST(stmtp, Var)) {
                        सचे (varp->isGParam()) defParamIt->second.emplace(varp->name(), varp);
                        // Recorded before V3Param resolves the type from the assigned value.
                        ध्रुव AstBasicDType* ध्रुव dtypep
                            = VN_CAST(varp->childDTypep(), BasicDType);
                        सचे (varp->isGParam() && dtypep && dtypep->implicit()) {
                            m_untypedParams.emplace(varp);
                        }
                    }
                }
            }
        }
    }
    तार्किक hierSubRun() ध्रुव { पटिनिवत्त m_hierSubRun; }
    तार्किक isHierBlock(ध्रुव string& origName) ध्रुव {
        पटिनिवत्त m_hierBlockOptsByOrigName.find(origName) != m_hierBlockOptsByOrigName.end();
    }
    Ast_ग्रन्थिः_Module* findByParams(ध्रुव string& origName, AstPin* firstPinp,
                                ध्रुव Ast_ग्रन्थिः_Module* modp) {
        UASSERT(isHierBlock(origName), origName << " is not hierarchical block");
        // This module is a hierarchical block. Need to replace it by the --lib-create wrapper.
        ध्रुव std::pair<HierMapIt, HierMapIt> candidates
            = m_hierBlockOptsByOrigName.equal_range(origName);
        ध्रुव auto paramsIt = m_modParams.find(origName);
        UASSERT_OBJ(paramsIt != m_modParams.end(), modp, origName << " must be registered");
        // An untyped parameter takes the type of its value, but libraries are distinguished
        // only by parameter values. This is a pin whose value equals that of a library or the
        // default, but whose type differs.
        ध्रुव AstPin* typeMismatchPinp = nullptr;
        HierMapIt hierIt;
        अत्थाय (hierIt = candidates.first; hierIt != candidates.second; ++hierIt) {
            तार्किक found = सच्च;
            size_t paramIdx = 0;
            ध्रुव ParamConstMap& params = m_hierParams[hierIt->second];
            UASSERT(params.size() == hierIt->second->params().size(), "not match");
            अत्थाय (AstPin* pinp = firstPinp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                सचे (!pinp->exprp()) अनुवर्त;
                सचे (ध्रुव AstVar* ध्रुव modvarp = pinp->modVarp()) {
                    AstConst* ध्रुव constp = VN_AS(pinp->exprp(), Const);
                    UASSERT_OBJ(constp, pinp,
                                "parameter for a hierarchical block must have been constified");
                    ध्रुव auto paramIt = paramsIt->second.find(modvarp->name());
                    UASSERT_OBJ(paramIt != paramsIt->second.end(), modvarp, "must be registered");
                    ध्रुव तार्किक untyped = m_untypedParams.गणनम्(paramIt->second);
                    AstConst* ध्रुव defValuep = VN_CAST(paramIt->second->valuep(), Const);
                    सचे (defValuep && areSame(constp, defValuep)) {
                        सचे (!untyped || constp->same_मानम्_Type(defValuep)) {
                            UINFO(5, "Setting default value of " << constp << " to " << modvarp);
                            अनुवर्त;  // Skip this parameter because setting the same value
                        }
                        typeMismatchPinp = pinp;
                    }
                    ध्रुव auto pIt = vlstd::as_const(params).find(modvarp->name());
                    UINFO(5, "Comparing " << modvarp->name() << " " << constp);
                    सचे (pIt == params.end() || paramIdx >= params.size()
                        || !areSame(constp, pIt->second.get())) {
                        found = मिच्छा;
                        भङ्ग;
                    }
                    सचे (untyped && !constp->same_मानम्_Type(pIt->second.get())) {
                        typeMismatchPinp = pinp;
                        found = मिच्छा;
                        भङ्ग;
                    }
                    UINFO(5, "Matched " << modvarp->name() << " " << constp << " and "
                                        << pIt->second.get());
                    ++paramIdx;
                }
            }
            सचे (found && paramIdx == hierIt->second->params().size()) भङ्ग;
        }
        सचे (hierIt == candidates.second && typeMismatchPinp) {
            typeMismatchPinp->v3warn(
                E_UNSUPPORTED, "Unsupported: Untyped parameter "
                                   << typeMismatchPinp->modVarp()->prettyNameQ()
                                   << " of hierarchical block " << Ast_ग्रन्थिः::prettyNameQ(origName)
                                   << " given equal values of different width or"
                                      " signedness");
            पटिनिवत्त nullptr;
        }
        UASSERT_OBJ(hierIt != candidates.second, firstPinp, "No --lib-create wrapper found");
        // parameter settings will be removed in the bottom of caller visitCell().
        ध्रुव HierBlockModMap::const_iterator modIt
            = m_hierBlockMod.find(hierIt->second->mangledName());
        UASSERT_OBJ(modIt != m_hierBlockMod.end(), firstPinp,
                    hierIt->second->mangledName() << " is not found");

        ध्रुव auto it = vlstd::as_const(m_hierBlockMod).find(hierIt->second->mangledName());
        सचे (it == m_hierBlockMod.end()) पटिनिवत्त nullptr;
        पटिनिवत्त it->second;
    }
    स्थिर तार्किक areSame(AstConst* pinValuep, AstConst* hierOptParamp) {
        सचे (pinValuep->isString()) {
            पटिनिवत्त pinValuep->अङ्कः().toString() == hierOptParamp->अङ्कः().toString();
        }

        सचे (hierOptParamp->isDouble()) {
            द्विगुण var;
            सचे (pinValuep->isDouble()) {
                var = pinValuep->अङ्कः().toDouble();
            } अञ्ञथा {  // Cast from integer to real
                V3_सङ्ख्या var_अङ्कः{pinValuep, V3_सङ्ख्या::Double{}, 0.0};
                var_अङ्कः.opIToRD(pinValuep->अङ्कः());
                var = var_अङ्कः.toDouble();
            }
            पटिनिवत्त V3_सङ्ख्या::epsilonEqual(var, hierOptParamp->अङ्कः().toDouble());
        } अञ्ञथा {  // Now integer type is assumed
            // Bitwidth of hierOptParamp is accurate because V3Width already calculated in the
            // previous run. Bitwidth of pinValuep is before width analysis, so pinValuep is casted
            // to hierOptParamp width.
            V3_सङ्ख्या var_अङ्कः{pinValuep, hierOptParamp->अङ्कः().width()};
            सचे (pinValuep->isDouble()) {  // Need to cast to int
                // Parameter is actually an integral type, but passed value is floating point.
                // Conversion from real to integer uses rounding in V3Width.cpp
                var_अङ्कः.opRToIRoundS(pinValuep->अङ्कः());
            } अञ्ञथा सचे (pinValuep->isSigned()) {
                var_अङ्कः.opExtendS(pinValuep->अङ्कः(), pinValuep->अङ्कः().width());
            } अञ्ञथा {
                var_अङ्कः.opAssign(pinValuep->अङ्कः());
            }
            V3_सङ्ख्या isEq{pinValuep, 1};
            isEq.opEq(var_अङ्कः, hierOptParamp->अङ्कः());
            पटिनिवत्त isEq.isNeqZero();
        }
    }
};

//######################################################################
// Remove parameters from cells and build new modules

वग्ग ParamProcessor final {
    // NODE STATE - Local
    //   AstVar::user3()        // int    Global parameter number (for naming new module)
    //                          //        (0=not processed, 1=iterated, but no number,
    //                          //         65+ parameter numbered)
    // NODE STATE - Shared with ParamVisitor
    //   AstNodeModule::user3p()  // AstNodeModule* Unaltered copy of the parameterized module.
    //                               The module/class node may be modified according to parameter
    //                               values and an unchanged copy is needed to instantiate
    //                               modules/classes with different parameters.
    //   AstNodeModule::user2() // bool   True if processed
    //   AstGenFor::user2()     // bool   True if processed
    //   AstVar::user2()        // bool   True if constant propagated
    //   AstIfaceRefDType::user2()      // bool   True if visited
    //   AstClassRefDType::user2()      // bool   True if visited
    //   AstNodeModule:user4p() // AstNodeModule* Parametrized copy with default parameters
    ध्रुव VNUser2InUse m_inuser2;
    ध्रुव VNUser3InUse m_inuser3;
    ध्रुव VNUser4InUse m_inuser4;
    // User1 used by constant function simulations

    // TYPES
    // Note may have duplicate entries
    using IfaceRefRefs = std::deque<std::pair<AstIfaceRefDType*, AstIfaceRefDType*>>;

    // STATE
    using CloneMap = std::unordered_map<ध्रुव Ast_ग्रन्थिः*, Ast_ग्रन्थिः*>;
    संरचना ModInfo final {
        Ast_ग्रन्थिः_Module* ध्रुव m_modp;  // Module with specified name
        CloneMap m_cloneMap;  // Map of old-varp -> new cloned varp
        explicit ModInfo(Ast_ग्रन्थिः_Module* modp)
            : m_modp{modp} {}
    };
    std::map<ध्रुव std::string, ModInfo> m_modNameMap;  // Hash of created module flavors by name

    std::map<ध्रुव std::string, std::string>
        m_longMap;  // Hash of very long names to unique identity number
    std::set<std::string> m_longSuffixes;  // Digest suffixes used by m_longMap

    // All module names that are loaded from source code
    // Generated modules by this visitor is not included
    VStringSet m_allModuleNames;

    CloneMap m_originalParams;  // Map between parameters of copied parameteized classes and their
                                // original nodes

    std::map<ध्रुव std::string, std::string> m_मानम्_Names;  // Parameter value text to its name
    std::set<std::string> m_मानम्_Suffixes;  // Digest suffixes used by m_valueNames
    // Number of digest hex digits in a name. With 8 digits, a collision between names made in
    // different runs, such as those of different hierarchical blocks, is unlikely. Within one
    // run, digestSuffix resolves a collision by lengthening the later text's suffix, which can
    // then differ between runs.
    स्थिर constexpr std::string::size_type DIGEST_DIGITS = 8;

    ध्रुव Ast_ग्रन्थिः_Module* m_modp = nullptr;  // Current module being processed

    // Database to get lib-create wrapper that matches parameters in hierarchical Verilation
    ParameterizedHierBlocks m_hierBlocks;
    // Default parameter values key:parameter name, value:default value (can be nullptr)
    using Default_मानम्_Map = std::map<std::string, Ast_ग्रन्थिः*>;
    // Default parameter values of hierarchical blocks
    std::map<Ast_ग्रन्थिः_Module*, Default_मानम्_Map> m_default_Parameter_मानानि;
    // Module -> a defparam in it or below it, or nullptr (see nestedDefparamp)
    std::map<ध्रुव Ast_ग्रन्थिः_Module*, ध्रुव AstPin*> m_nestedDefparamps;
    VNDeleter m_deleter;  // Used to delay deletion of nodes
    // Class default type paramater dependencies
    std::vector<std::pair<AstParamTypeDType*, पूर्णाङ्क>> m_classTypeParams;
    std::unordered_map<AstParamTypeDType*, पूर्णाङ्क> m_paramIndex;

    // Guard against infinite recursion in classTypeMatchesDefaultClone slow path
    std::unordered_set<ध्रुव AstClass*> m_defaultCloneInProgress;

    std::vector<AstClass*> m_specializedClassps;  // Classes specialized since last drained

    // member names cached for fast lookup
    VMemberMap m_memberMap;

    // METHODS

    स्थिर शून्य makeSmallNames(Ast_ग्रन्थिः_Module* modp) {
        std::vector<पूर्णाङ्क> usedLetter;
        usedLetter.resize(256);
        // Pass 1, assign first letter to each gparam's name
        अत्थाय (Ast_ग्रन्थिः* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            सचे (AstVar* ध्रुव varp = VN_CAST(stmtp, Var)) {
                सचे (varp->isGParam() || varp->isIfaceRef()) {
                    वर्ण ch = varp->name()[0];
                    ch = std::toupper(ch);
                    सचे (ch < 'A' || ch > 'Z') ch = 'Z';
                    varp->user3(usedLetter[static_cast<पूर्णाङ्क>(ch)] * 256 + ch);
                    usedLetter[static_cast<पूर्णाङ्क>(ch)]++;
                }
            } अञ्ञथा सचे (AstParamTypeDType* ध्रुव typep = VN_CAST(stmtp, ParamTypeDType)) {
                ध्रुव वर्ण ch = 'T';
                typep->user3(usedLetter[static_cast<पूर्णाङ्क>(ch)] * 256 + ch);
                usedLetter[static_cast<पूर्णाङ्क>(ch)]++;
            }
        }
    }
    स्थिर string paramSmallName(Ast_ग्रन्थिः_Module* modp, Ast_ग्रन्थिः* varp) {
        सचे (varp->user3() <= 1) makeSmallNames(modp);
        पूर्णाङ्क index = varp->user3() / 256;
        ध्रुव वर्ण ch = varp->user3() & 255;
        string st = cvtToStr(ch);
        याव (index) {
            st += cvtToStr(static_cast<वर्ण>((index % 25) + 'A'));
            index /= 26;
        }
        पटिनिवत्त st;
    }

    स्थिर तार्किक is_Aggregate_Param_मानम्(ध्रुव Ast_ग्रन्थिः* nodep) {
        पटिनिवत्त VN_IS(nodep, ConsArray) || VN_IS(nodep, ConsStruct);
    }

    स्थिर string param_मानम्_String(ध्रुव Ast_ग्रन्थिः* nodep) {
        सचे (ध्रुव AstRefDType* ध्रुव refp = VN_CAST(nodep, RefDType)) {
            nodep = refp->skipRefToNonRefp();
        }
        string कुञ्चिका = nodep->name();
        सचे (ध्रुव AstIfaceRefDType* ध्रुव ifrtp = VN_CAST(nodep, IfaceRefDType)) {
            सचे (ifrtp->cellp() && ifrtp->cellp()->modp()) {
                कुञ्चिका = ifrtp->cellp()->modp()->name();
            } अञ्ञथा सचे (ifrtp->ifacep()) {
                कुञ्चिका = ifrtp->ifacep()->name();
            } अञ्ञथा {
                nodep->v3fatalSrc("Can't parameterize interface without module name");
            }
        } अञ्ञथा सचे (ध्रुव Ast_ग्रन्थिः_UOr_Struct_DType* ध्रुव dtypep
                   = VN_CAST(nodep, ग्रन्थिः_UOr_Struct_DType)) {
            कुञ्चिका += " ";
            कुञ्चिका += dtypep->verilogKwd();
            कुञ्चिका += " {";
            अत्थाय (ध्रुव Ast_ग्रन्थिः* memberp = dtypep->membersp(); memberp;
                 memberp = memberp->nextp()) {
                कुञ्चिका += param_मानम्_String(memberp);
                कुञ्चिका += ";";
            }
            कुञ्चिका += "}";
        } अञ्ञथा सचे (ध्रुव AstMemberDType* ध्रुव dtypep = VN_CAST(nodep, MemberDType)) {
            कुञ्चिका += " ";
            कुञ्चिका += param_मानम्_String(dtypep->subDTypep());
        } अञ्ञथा सचे (ध्रुव AstBasicDType* ध्रुव dtypep = VN_CAST(nodep, BasicDType)) {
            सचे (dtypep->isSigned()) कुञ्चिका += " signed";
            सचे (dtypep->isRanged()) {
                कुञ्चिका += "[" + cvtToStr(dtypep->वामः()) + ":" + cvtToStr(dtypep->दक्षिणः()) + "]";
            }
        } अञ्ञथा सचे (ध्रुव AstPackArrayDType* ध्रुव dtypep = VN_CAST(nodep, PackArrayDType)) {
            कुञ्चिका += "[";
            कुञ्चिका += cvtToStr(dtypep->वामः());
            कुञ्चिका += ":";
            कुञ्चिका += cvtToStr(dtypep->दक्षिणः());
            कुञ्चिका += "] ";
            कुञ्चिका += param_मानम्_String(dtypep->subDTypep());
        } अञ्ञथा सचे (ध्रुव AstPattern* ध्रुव patp = VN_CAST(nodep, Pattern)) {
            // A parameter override pattern, keyed before the specialized module types it
            कुञ्चिका += "'{";
            अत्थाय (ध्रुव AstPatMember* memp = VN_AS(patp->itemsp(), PatMember); memp;
                 memp = VN_AS(memp->nextp(), PatMember)) {
                सचे (memp->isDefault()) कुञ्चिका += "default";
                सचे (ध्रुव AstText* ध्रुव textp = VN_CAST(memp->keyp(), Text)) {
                    कुञ्चिका += textp->text();
                } अञ्ञथा सचे (memp->keyp()) {
                    कुञ्चिका += param_मानम्_String(memp->keyp());
                }
                कुञ्चिका += ":";
                सचे (memp->repp()) कुञ्चिका += param_मानम्_String(memp->repp()) + "x";
                अत्थाय (ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* valuep = memp->lhssp(); valuep;
                     valuep = VN_AS(valuep->nextp(), ग्रन्थिः_व्यञ्जकम्)) {
                    कुञ्चिका += param_मानम्_String(valuep) + ";";
                }
                कुञ्चिका += ",";
            }
            कुञ्चिका += "}";
        } अञ्ञथा सचे (ध्रुव AstConsArray* ध्रुव consArrayp = VN_CAST(nodep, ConsArray)) {
            // Include the indices and the default, as with a default the items may be sparse
            कुञ्चिका += "{";
            अत्थाय (ध्रुव AstConsArrayItem* itemp = consArrayp->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), ConsArrayItem)) {
                कुञ्चिका += cvtToStr(itemp->index()) + ":" + param_मानम्_String(itemp->valuep()) + ",";
            }
            सचे (consArrayp->defaultp()) {
                कुञ्चिका += "default:" + param_मानम्_String(consArrayp->defaultp()) + ",";
            }
            कुञ्चिका += "}";
        } अञ्ञथा सचे (ध्रुव AstConsStruct* ध्रुव consp = VN_CAST(nodep, ConsStruct)) {
            कुञ्चिका += "{";
            अत्थाय (ध्रुव AstConsStructItem* itemp = consp->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), ConsStructItem)) {
                कुञ्चिका += param_मानम्_String(itemp->valuep());
                कुञ्चिका += ",";
            }
            कुञ्चिका += "}";
        } अञ्ञथा सचे (ध्रुव AstClassRefDType* ध्रुव classRefp = VN_CAST(nodep, ClassRefDType)) {
            // For parameterized class types, use the original class name (without specialization
            // suffix) plus the actual type parameter values. This ensures equivalent class types
            // get the same string representation regardless of which AST node is used.
            सचे (classRefp->classp()) {
                ध्रुव string& className = classRefp->classp()->name();
                ध्रुव string& origName = classRefp->classp()->origName();
                ध्रुव तार्किक isSpecialized = (className != origName);
                UINFO(9, "paramValueString ClassRefDType: name="
                             << className << " origName=" << origName
                             << " isSpecialized=" << isSpecialized
                             << " hasParams=" << (classRefp->paramsp() ? "Y" : "N")
                             << " classHasGParam=" << classRefp->classp()->hasGParam());

                सचे (classRefp->paramsp()) {
                    // ClassRefDType should have been deparameterized (paramsp
                    // consumed) before cellPinCleanup calls paramValueString.
                    classRefp->v3fatalSrc(  // LCOV_EXCL_LINE
                        "ClassRefDType still has paramsp in paramValueString");
                } अञ्ञथा सचे (isSpecialized) {
                    // Already specialized class (e.g., c1__Tz1_TBz1) - use full name
                    // This ensures different specializations are distinguished
                    कुञ्चिका = className;
                } अञ्ञथा {
                    // Unspecialized class with no params - use origName
                    कुञ्चिका = origName;
                }
            } अञ्ञथा {
                // classp() should always be set; unresolved class refs
                // would have errored in LinkDot.
                classRefp->v3fatalSrc(  // LCOV_EXCL_LINE
                    "ClassRefDType has null classp in paramValueString");
            }
        } अञ्ञथा सचे (ध्रुव AstUnpackArrayDType* ध्रुव dtypep = VN_CAST(nodep, UnpackArrayDType)) {
            // Name containers by their elements as above, not by prettyDTypeName(), which names
            // classes by parameter values that may be yet to be resolved
            कुञ्चिका = paramElemString(dtypep->subDTypep()) + "$" + cvtToStr(dtypep->declRange());
        } अञ्ञथा सचे (ध्रुव AstQueueDType* ध्रुव dtypep = VN_CAST(nodep, QueueDType)) {
            कुञ्चिका = paramElemString(dtypep->subDTypep()) + "$[$";
            सचे (dtypep->boundConst()) कुञ्चिका += ":" + cvtToStr(dtypep->boundConst());
            कुञ्चिका += "]";
        } अञ्ञथा सचे (ध्रुव AstDynArrayDType* ध्रुव dtypep = VN_CAST(nodep, DynArrayDType)) {
            कुञ्चिका = paramElemString(dtypep->subDTypep()) + "$[]";
        } अञ्ञथा सचे (ध्रुव AstAssocArrayDType* ध्रुव dtypep = VN_CAST(nodep, AssocArrayDType)) {
            कुञ्चिका = paramElemString(dtypep->subDTypep()) + "$["
                  + paramElemString(dtypep->कुञ्चिका_DTypep()) + "]";
        } अञ्ञथा सचे (ध्रुव AstWildcardArrayDType* ध्रुव dtypep
                   = VN_CAST(nodep, WildcardArrayDType)) {
            कुञ्चिका = paramElemString(dtypep->subDTypep()) + "$[*]";
        } अञ्ञथा सचे (ध्रुव AstEnumDType* ध्रुव dtypep = VN_CAST(nodep, EnumDType)) {
            // With the items, as prettyDTypeName() omits them, yet a specialization of a class
            // may declare an enumeration of the same name with other values
            कुञ्चिका += " enum {";
            अत्थाय (ध्रुव AstEnumItem* itemp = dtypep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), EnumItem)) {
                ध्रुव AstConst* ध्रुव constp = VN_CAST(itemp->valuep(), Const);
                कुञ्चिका += itemp->name() + "=" + (constp ? constp->अङ्कः().ascii() : "?") + ";";
            }
            कुञ्चिका += "}";
        } अञ्ञथा सचे (ध्रुव Ast_ग्रन्थिः_DType* ध्रुव dtypep = VN_CAST(nodep, ग्रन्थिः_DType)) {
            कुञ्चिका += dtypep->prettyDTypeName(सच्च);
        }
        UASSERT_OBJ(!कुञ्चिका.empty(), nodep, "Parameter yielded no value string");
        पटिनिवत्त कुञ्चिका;
    }
    // As paramValueString(), for the element or key type of a container
    स्थिर string paramElemString(ध्रुव Ast_ग्रन्थिः_DType* dtypep) {
        // Unlike at the top of a value, a class may be yet to be specialized, so just name it
        ध्रुव AstClassRefDType* ध्रुव classRefp
            = VN_CAST(dtypep->skipRefToNonRefp(), ClassRefDType);
        सचे (classRefp && classRefp->paramsp()) पटिनिवत्त classRefp->prettyDTypeName(सच्च);
        पटिनिवत्त param_मानम्_String(dtypep);
    }

    // Return a name suffix for 'text' from its SHA-512 digest. Hierarchical blocks are
    // Verilated in separate runs, where a counter would restart, but the digest is the same in
    // every run. Use the shortest digest prefix of at least DIGEST_DIGITS digits that is not
    // already in 'usedr', and add it to 'usedr'.
    स्थिर string digestSuffix(ध्रुव string& text, std::set<string>& usedr) {
        ध्रुव string hex = VHashSha512{text}.digestHex();
        // Force collisions of the prefixes -- for testing only
        string::size_type digits = v3Global.opt.debugCollision() ? 1 : DIGEST_DIGITS;
        याव (digits < hex.size() && !usedr.निवेशय(hex.substr(0, digits)).second) ++digits;
        पटिनिवत्त hex.substr(0, digits);
    }
    string param_मानम्_सङ्ख्या(Ast_ग्रन्थिः* nodep) {
        // All call sites resolve through skipRefToNonRefp() or pass non-DType
        // nodes, so nodep should never be a bare RefDType here.
        सचे (VN_IS(nodep, RefDType)) {  // LCOV_EXCL_LINE
            nodep->v3fatalSrc("Unexpected RefDType in paramValueNumber");  // LCOV_EXCL_LINE
        }
        // Name the value by its text, which is the same for equal values or types in every run.
        // V3Hasher is unsuitable, as it hashes node pointers, which can differ for equal types.
        // V3Hash of a string is unsuitable, as std::hash varies between C++ libraries.
        ध्रुव string text = param_मानम्_String(nodep);
        ध्रुव auto pair = m_मानम्_Names.emplace(text, "");
        सचे (pair.second) pair.first->second = "z" + digestSuffix(text, m_मानम्_Suffixes);
        पटिनिवत्त pair.first->second;
    }
    string moduleCalcName(ध्रुव Ast_ग्रन्थिः_Module* srcModp, ध्रुव string& longname) {
        string newname = longname;
        सचे (longname.length() > 30) {
            ध्रुव auto pair = m_longMap.emplace(longname, "");
            सचे (pair.second) {
                // We use all upper case above, so lower here can't conflict
                pair.first->second
                    = srcModp->name() + "__pi" + digestSuffix(longname, m_longSuffixes);
            }
            newname = pair.first->second;
        }
        UINFO(4, "Name: " << srcModp->name() << "->" << longname << "->" << newname);
        पटिनिवत्त newname;
    }
    स्थिर Ast_ग्रन्थिः_DType* arraySubDTypep(Ast_ग्रन्थिः_DType* nodep) {
        // If an unpacked array, return the subDTypep under it
        सचे (ध्रुव AstUnpackArrayDType* ध्रुव adtypep = VN_CAST(nodep, UnpackArrayDType)) {
            पटिनिवत्त adtypep->subDTypep();
        }
        // We have not resolved parameter of the child yet, so still
        // have BracketArrayDType's. We'll presume it'll end up as assignment
        // compatible (or V3Width will complain).
        सचे (ध्रुव AstBracketArrayDType* ध्रुव adtypep = VN_CAST(nodep, BracketArrayDType)) {
            पटिनिवत्त adtypep->subDTypep();
        }
        पटिनिवत्त nullptr;
    }
    // Peel all unpacked-array layers to reach the innermost subDTypep.
    // Used for multi-dim iface arrays where port dtype is nested UnpackArrayDType.
    स्थिर Ast_ग्रन्थिः_DType* arraySubDTypeDeepp(Ast_ग्रन्थिः_DType* nodep) {
        याव (Ast_ग्रन्थिः_DType* ध्रुव subp = arraySubDTypep(nodep)) nodep = subp;
        पटिनिवत्त nodep;
    }
    स्थिर तार्किक isString(Ast_ग्रन्थिः_DType* nodep) {
        सचे (AstBasicDType* ध्रुव basicp = VN_CAST(nodep->skipRefToNonRefp(), BasicDType))
            पटिनिवत्त basicp->isString();
        पटिनिवत्त मिच्छा;
    }
    शून्य collectPins(CloneMap* clonemapp, Ast_ग्रन्थिः_Module* modp, तार्किक originalIsCopy) {
        // Grab all I/O so we can remap our pins later
        अत्थाय (Ast_ग्रन्थिः* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ध्रुव Ast_ग्रन्थिः* originalParamp = nullptr;
            सचे (AstVar* ध्रुव varp = VN_CAST(stmtp, Var)) {
                सचे (varp->isIO() || varp->isGParam() || varp->isIfaceRef()) {
                    // Cloning saved a pointer to the new node for us, so just follow that link.
                    originalParamp = varp->clonep();
                }
            } अञ्ञथा सचे (AstParamTypeDType* ध्रुव ptp = VN_CAST(stmtp, ParamTypeDType)) {
                सचे (ptp->isGParam()) originalParamp = ptp->clonep();
            }
            सचे (originalIsCopy) originalParamp = m_originalParams[originalParamp];
            सचे (originalParamp) clonemapp->emplace(originalParamp, stmtp);
        }
    }
    शून्य relinkPins(ध्रुव CloneMap* clonemapp, AstPin* startpinp) {
        अत्थाय (AstPin* pinp = startpinp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            सचे (pinp->modVarp()) {
                // Find it in the clone structure
                // UINFO(8,"Clone find 0x"<<hex<<(uint32_t)pinp->modVarp());
                ध्रुव auto cloneiter = clonemapp->find(pinp->modVarp());
                UASSERT_OBJ(cloneiter != clonemapp->end(), pinp,
                            "Couldn't find pin in clone list");
                pinp->modVarp(VN_AS(cloneiter->second, Var));
            } अञ्ञथा सचे (pinp->modPTypep()) {
                ध्रुव auto cloneiter = clonemapp->find(pinp->modPTypep());
                UASSERT_OBJ(cloneiter != clonemapp->end(), pinp,
                            "Couldn't find pin in clone list");
                pinp->modPTypep(VN_AS(cloneiter->second, ParamTypeDType));
            } अञ्ञथा {
                pinp->v3fatalSrc("Not linked?");
            }
        }
    }
    शून्य relinkPinsByName(AstPin* startpinp, Ast_ग्रन्थिः_Module* modp) {
        std::map<ध्रुव string, AstVar*> nameToPin;
        अत्थाय (Ast_ग्रन्थिः* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            सचे (AstVar* ध्रुव varp = VN_CAST(stmtp, Var)) {
                सचे (varp->isIO() || varp->isGParam() || varp->isIfaceRef()) {
                    nameToPin.emplace(varp->name(), varp);
                }
            }
        }
        अत्थाय (AstPin* pinp = startpinp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            सचे (ध्रुव AstVar* ध्रुव varp = pinp->modVarp()) {
                ध्रुव auto varIt = vlstd::as_const(nameToPin).find(varp->name());
                UASSERT_OBJ(varIt != nameToPin.end(), varp,
                            "Not found in " << modp->prettyNameQ());
                pinp->modVarp(varIt->second);
            }
        }
    }
    स्थिर तार्किक hasDescendantDefparams(ध्रुव Ast_ग्रन्थिः_Module* modp,
                                       std::set<ध्रुव Ast_ग्रन्थिः_Module*>& visited) {
        सचे (!visited.निवेशय(modp).second) पटिनिवत्त मिच्छा;
        अत्थाय (Ast_ग्रन्थिः* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstCell* ध्रुव cellp = VN_CAST(stmtp, Cell);
            सचे (!cellp) अनुवर्त;
            अत्थाय (AstPin* pinp = cellp->paramsp(); pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                सचे (!pinp->paramPath().empty()) {
                    visited.erase(modp);
                    पटिनिवत्त सच्च;
                }
            }
            सचे (Ast_ग्रन्थिः_Module* ध्रुव childModp = cellp->modp()) {
                सचे (hasDescendantDefparams(childModp, visited)) {
                    visited.erase(modp);
                    पटिनिवत्त सच्च;
                }
            }
        }
        visited.erase(modp);
        पटिनिवत्त मिच्छा;
    }
    स्थिर तार्किक hasDescendantDefparams(ध्रुव Ast_ग्रन्थिः_Module* modp) {
        std::set<ध्रुव Ast_ग्रन्थिः_Module*> visited;
        पटिनिवत्त hasDescendantDefparams(modp, visited);
    }
    // Return a defparam in the module or below it. Unlike hasDescendantDefparams, also look
    // inside generate constructs. Memoized, so each module is searched once.
    ध्रुव AstPin* nestedDefparamp(ध्रुव Ast_ग्रन्थिः_Module* modp) {
        ध्रुव auto pair = m_nestedDefparamps.emplace(modp, nullptr);
        सचे (!pair.second) पटिनिवत्त pair.first->second;  // Searched, or being searched
        ध्रुव AstPin* foundp = nullptr;
        modp->exists([&](ध्रुव AstCell* cellp) {
            अत्थाय (ध्रुव AstPin* pinp = cellp->paramsp(); pinp && !foundp;
                 pinp = VN_AS(pinp->nextp(), Pin)) {
                सचे (!pinp->paramPath().empty()) foundp = pinp;
            }
            सचे (!foundp && cellp->modp()) foundp = nestedDefparamp(cellp->modp());
            पटिनिवत्त foundp != nullptr;
        });
        pair.first->second = foundp;
        पटिनिवत्त foundp;
    }
    // Check if parameter setting during instantiation is simple enough for hierarchical Verilation
    शून्य checkSupportedParam(Ast_ग्रन्थिः_Module* modp, AstPin* pinp) ध्रुव {
        // ConsArray is not supported because that can not be set via -G
        // option.
        सचे (pinp->modVarp()) {
            तार्किक supported = मिच्छा;
            सचे (ध्रुव AstConst* ध्रुव constp = VN_CAST(pinp->exprp(), Const)) {
                supported = सच्च;
                सचे (constp->अङ्कः().isString()
                    && !V3HierBlock::stringParamPassable(constp->अङ्कः().toString())) {
                    pinp->v3warn(E_UNSUPPORTED, "Unsupported: String value of hierarchical block"
                                                " parameter with newline, double quote, '/*',"
                                                " or whitespace before '//'");
                }
            }
            सचे (!supported) {
                pinp->v3error(
                    Ast_ग्रन्थिः::prettyNameQ(modp->origName())
                    << " has hier_block metacomment, hierarchical Verilation"
                    << " supports only integer/floating point/string and type param parameters");
            }
        } अञ्ञथा सचे (ध्रुव AstParamTypeDType* ध्रुव typep = pinp->modPTypep()) {
            // Libraries are found by comparing values only
            pinp->v3warn(E_UNSUPPORTED, "Unsupported: Setting type parameter "
                                            << typep->prettyNameQ() << " of hierarchical block "
                                            << Ast_ग्रन्थिः::prettyNameQ(modp->origName()));
        }
    }
    तार्किक moduleExists(ध्रुव string& modName) ध्रुव {
        सचे (m_allModuleNames.find(modName) != m_allModuleNames.end()) पटिनिवत्त सच्च;
        सचे (m_modNameMap.find(modName) != m_modNameMap.end()) पटिनिवत्त सच्च;
        पटिनिवत्त मिच्छा;
    }

    string parameterizedHierBlockName(Ast_ग्रन्थिः_Module* modp, AstPin* paramPinsp) {
        // Create a unique name in the following steps
        //  - Make a long name that includes all parameters, that appear
        //    in the alphabetical order.
        //  - Hash the long name to get valid Verilog symbol
        UASSERT_OBJ(modp->hierBlock(), modp, "should be used for hierarchical block");

        std::map<string, Ast_ग्रन्थिः*> pins;

        AstPin* pinp = paramPinsp;
        याव (pinp) {
            checkSupportedParam(modp, pinp);
            सचे (ध्रुव AstVar* ध्रुव varp = pinp->modVarp()) {
                सचे (!pinp->exprp()) अनुवर्त;
                सचे (varp->isGParam()) pins.emplace(varp->name(), pinp->exprp());
            } अञ्ञथा सचे (VN_IS(pinp->exprp(), BasicDType) || VN_IS(pinp->exprp(), ग्रन्थिः_DType)) {
                pins.emplace(pinp->name(), pinp->exprp());
            }
            pinp = VN_AS(pinp->nextp(), Pin);
        }

        ध्रुव auto pair = m_default_Parameter_मानानि.emplace(
            std::piecewise_construct, std::forward_as_tuple(modp), std::forward_as_tuple());
        सचे (pair.second) {  // Not cached yet, so check parameters
            // A separately compiled library cannot receive defparams aimed into its body
            सचे (ध्रुव AstPin* ध्रुव defparamp = nestedDefparamp(modp)) {
                defparamp->v3warn(E_UNSUPPORTED, "Unsupported: defparam inside hierarchical block "
                                                     << Ast_ग्रन्थिः::prettyNameQ(modp->origName()));
            }
            // Using map with key=string so that we can scan it in deterministic order
            Default_मानम्_Map params;
            अत्थाय (Ast_ग्रन्थिः* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                सचे (AstVar* ध्रुव varp = VN_CAST(stmtp, Var)) {
                    सचे (varp->isGParam()) {
                        AstConst* ध्रुव constp = VN_CAST(varp->valuep(), Const);
                        // constp can be nullptr if the parameter is not used to instantiate sub
                        // module. varp->valuep() is not constified yet in the case.
                        // nullptr means that the parameter is using some default value.
                        params.emplace(varp->name(), constp);
                    }
                } अञ्ञथा सचे (AstParamTypeDType* ध्रुव p = VN_CAST(stmtp, ParamTypeDType)) {
                    Ast_ग्रन्थिः* ध्रुव dtypep = static_cast<Ast_ग्रन्थिः*>(p->skipRefp());
                    params.emplace(p->name(), dtypep);
                }
            }
            pair.first->second = std::move(params);
        }
        ध्रुव auto paramsIt = pair.first;
        सचे (paramsIt->second.empty()) पटिनिवत्त modp->origName();  // modp has no parameter

        string longname = modp->origName();
        अत्थाय (auto&& default_मानम् : paramsIt->second) {
            ध्रुव auto pinIt = pins.find(default_मानम्.first);
            // If the pin does not have a value assigned, use the default one.
            ध्रुव Ast_ग्रन्थिः* ध्रुव nodep = pinIt == pins.end() ? default_मानम्.second : pinIt->second;
            // This longname is not valid as verilog symbol, but ok, because it will be hashed
            longname += "_" + default_मानम्.first + "=";
            // constp can be nullptr

            सचे (ध्रुव AstConst* ध्रुव p = VN_CAST(nodep, Const)) {
                // Treat modules parameterized with the same values but with different type as the
                // same.
                longname += p->अङ्कः().ascii(मिच्छा);
            } अञ्ञथा सचे (nodep) {
                std::stringstream type;
                V3EmitV::verilog_For_वृक्षः(nodep, type);
                longname += type.str();
            }
        }
        UINFO(9, "       module params longname: " << longname);

        ध्रुव auto iter = m_longMap.find(longname);
        सचे (iter != m_longMap.end()) पटिनिवत्त iter->second;  // Already calculated

        VHashSha512 hash;
        // Calculate hash using longname
        // The hash is used as the module suffix to find a module name that is unique in the design
        hash.निवेशय(longname);
        याव (सच्च) {
            // Copy VHashSha512 just in case of hash collision
            VHashSha512 hashStrGen = hash;
            // Hex string must be a safe suffix for any symbol
            ध्रुव string hashStr = hashStrGen.digestHex();
            अत्थाय (string::size_type i = 1; i < hashStr.size(); ++i) {
                string newName = modp->origName();
                // Don't use '__' not to be encoded when this module is loaded later by Verilator
                सचे (newName.at(newName.size() - 1) != '_') newName += '_';
                newName += hashStr.substr(0, i);
                सचे (!moduleExists(newName)) {
                    m_longMap.emplace(longname, newName);
                    पटिनिवत्त newName;
                }
            }
            // Hash collision. maybe just v3error is practically enough
            hash.निवेशय(V3Os::trueRandom(64));
        }
    }
    शून्य replaceRefsRecurse(Ast_ग्रन्थिः* ध्रुव nodep, ध्रुव AstClass* ध्रुव oldClassp,
                            AstClass* ध्रुव newClassp) {
        // Self references linked in the first pass of V3LinkDot.cpp should point to the default
        // instance.
        सचे (AstClassRefDType* ध्रुव classRefp = VN_CAST(nodep, ClassRefDType)) {
            सचे (classRefp->classp() == oldClassp) classRefp->classp(newClassp);
        } अञ्ञथा सचे (AstClassOrPackageRef* ध्रुव classRefp = VN_CAST(nodep, ClassOrPackageRef)) {
            सचे (classRefp->classOrPackageSkipp() == oldClassp)
                classRefp->classOrPackagep(newClassp);
        } अञ्ञथा सचे (AstTypedef* ध्रुव typedefp = VN_CAST(nodep, Typedef)) {
            // Update typedefs that refer to the old class to point to the new class
            सचे (typedefp->subDTypep()) {
                सचे (AstClassRefDType* ध्रुव classRefp
                    = VN_CAST(typedefp->subDTypep(), ClassRefDType)) {
                    सचे (classRefp->classp() == oldClassp) { classRefp->classp(newClassp); }
                }
            }
        } अञ्ञथा सचे (Ast_ग्रन्थिः_FTask_Ref* ध्रुव ftaskRefp = VN_CAST(nodep, ग्रन्थिः_FTask_Ref)) {
            // Also update FuncRef/TaskRef packagep to point to new class
            // This fixes static method calls through typedefs in parameterized classes
            सचे (ftaskRefp->classOrPackagep() == oldClassp) ftaskRefp->classOrPackagep(newClassp);
            // Also update taskp if it points to a function in the old class.
            // AstNodeFTask::classOrPackagep() (op2) holds a parse-time
            // Dot/ClassOrPackageRef for extern declarations, which is deleted
            // in LinkDot::moveExternFuncDecl before V3Param runs. For inline
            // class methods op2 is nullptr. So this should never match.
            सचे (Ast_ग्रन्थिः_FTask* ध्रुव oldTaskp = ftaskRefp->taskp()) {
                सचे (oldTaskp->classOrPackagep() == oldClassp) {
                    oldTaskp->v3fatalSrc(  // LCOV_EXCL_LINE
                        "FTask classOrPackagep unexpectedly matches old class "
                        << oldClassp->prettyNameQ());
                }
            }
        }

        सचे (nodep->op1p()) replaceRefsRecurse(nodep->op1p(), oldClassp, newClassp);
        सचे (nodep->op2p()) replaceRefsRecurse(nodep->op2p(), oldClassp, newClassp);
        सचे (nodep->op3p()) replaceRefsRecurse(nodep->op3p(), oldClassp, newClassp);
        सचे (nodep->op4p()) replaceRefsRecurse(nodep->op4p(), oldClassp, newClassp);
        सचे (nodep->nextp()) replaceRefsRecurse(nodep->nextp(), oldClassp, newClassp);
    }

    // Helper visitor to update VarXRefs to use variables from specialized interfaces.
    // When a module with interface ports is cloned and the port's interface is remapped
    // to a specialized version, VarXRefs that access members of the old interface need
    // to be updated to reference the corresponding members in the new interface.
    वग्ग VarXRefRelinkVisitor final : सार्वजनिक VNVisitor {
        Ast_ग्रन्थिः_Module* m_modp;  // The cloned module we're updating
        std::unordered_map<AstVar*, Ast_ग्रन्थिः_Module*> m_varModuleMap;  // Cache var->module lookups

    सार्वजनिक:
        explicit VarXRefRelinkVisitor(Ast_ग्रन्थिः_Module* newModp)
            : m_modp{newModp} {
            iterate(newModp);
        }

    गुप्त:
        // Find which module a variable belongs to, using cache to avoid repeated backp() walks
        Ast_ग्रन्थिः_Module* findVarModule(AstVar* varp) {
            ध्रुव auto it = m_varModuleMap.find(varp);
            सचे (it != m_varModuleMap.end()) पटिनिवत्त it->second;
            Ast_ग्रन्थिः_Module* varModp = nullptr;
            अत्थाय (Ast_ग्रन्थिः* np = varp; np; np = np->backp()) {
                सचे (Ast_ग्रन्थिः_Module* ध्रुव modp = VN_CAST(np, ग्रन्थिः_Module)) {
                    varModp = modp;
                    भङ्ग;
                }
            }
            m_varModuleMap[varp] = varModp;
            पटिनिवत्त varModp;
        }

        शून्य visit(AstVarXRef* nodep) override {
            AstVar* ध्रुव varp = nodep->varp();
            सचे (!varp) {
                iterateChildren(nodep);
                पटिनिवत्त;
            }

            // Get the dotted prefix (port name) from the VarXRef
            // dotted() format: "portname" or "portname.subpath" or empty
            ध्रुव string& dotted = nodep->dotted();
            सचे (dotted.empty()) {
                iterateChildren(nodep);
                पटिनिवत्त;
            }

            ध्रुव size_t dotPos = dotted.find('.');
            ध्रुव string portName = (dotPos == string::npos) ? dotted : dotted.substr(0, dotPos);
            सचे (portName.empty()) {
                iterateChildren(nodep);
                पटिनिवत्त;
            }

            // Find the interface port variable in the cloned module
            AstVar* portVarp = nullptr;
            अत्थाय (Ast_ग्रन्थिः* stmtp = m_modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                सचे (AstVar* ध्रुव varChkp = VN_CAST(stmtp, Var)) {
                    सचे (varChkp->name() == portName && varChkp->isIfaceRef()) {
                        portVarp = varChkp;
                        भङ्ग;
                    }
                }
            }
            सचे (!portVarp) {
                iterateChildren(nodep);
                पटिनिवत्त;
            }

            // Get the interface module from the port's dtype
            AstIfaceRefDType* ध्रुव irefp = VN_CAST(portVarp->subDTypep(), IfaceRefDType);
            सचे (!irefp) {
                iterateChildren(nodep);
                पटिनिवत्त;
            }

            Ast_ग्रन्थिः_Module* ध्रुव newIfacep = irefp->ifaceViaCellp();
            सचे (!newIfacep) {
                iterateChildren(nodep);
                पटिनिवत्त;
            }

            // Find which module the variable currently belongs to (cached)
            Ast_ग्रन्थिः_Module* ध्रुव varModp = findVarModule(varp);

            // If variable is in a different module than the port's interface, remap it
            सचे (varModp && varModp != newIfacep) {
                अत्थाय (Ast_ग्रन्थिः* stmtp = newIfacep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    सचे (AstVar* ध्रुव newVarp = VN_CAST(stmtp, Var)) {
                        सचे (newVarp->name() == varp->name()) {
                            UINFO(9, "VarXRef relink " << varp->name() << " in " << varModp->name()
                                                       << " -> " << newIfacep->name());
                            nodep->varp(newVarp);
                            भङ्ग;
                        }
                    }
                }
            }
            iterateChildren(nodep);
        }
        शून्य visit(Ast_ग्रन्थिः* nodep) override { iterateChildren(nodep); }
    };

    // Retarget the captured references in newModp to its specialized interfaces.
    शून्य fixupCrossModuleRefDTypes(Ast_ग्रन्थिः_Module* newModp) {
        सचे (!V3LinkDotIfaceCapture::enabled()) पटिनिवत्त;
        UINFO(9, "iface capture clone fixup: " << newModp->prettyNameQ());
        पूर्णाङ्क fixed_गणनम् = 0;
        // References in a class nested in newModp (e.g. a covergroup) are included.
        newModp->foreach([&](AstRefDType* refp) {
            ध्रुव VIfaceCaptureTag* ध्रुव tagp = V3LinkDotIfaceCapture::captureTag(refp);
            सचे (!tagp) पटिनिवत्त;

            Ast_ग्रन्थिः_Module* ध्रुव correctModp
                = V3LinkDotIfaceCapture::followCellPath(newModp, tagp->m_cellPath);
            UINFO(9, "  path fixup: " << refp << " cellPath='" << tagp->m_cellPath << "' -> "
                                      << (correctModp ? correctModp->name() : "<null>"));
            सचे (!correctModp || correctModp->dead()) पटिनिवत्त;
            सचे (correctModp->parameterizedTemplate()) पटिनिवत्त;

            तार्किक fixed = मिच्छा;
            सचे (refp->typedefp()) {
                सचे (AstTypedef* ध्रुव newTdp = V3LinkDotIfaceCapture::findTypedefInModule(
                        correctModp, refp->typedefp()->name())) {
                    refp->typedefp(newTdp);
                    सचे (newTdp->subDTypep()) refp->refDTypep(newTdp->subDTypep());
                    fixed = सच्च;
                }
            }
            सचे (!fixed && refp->refDTypep()) {
                सचे (Ast_ग्रन्थिः_DType* ध्रुव newDtp = V3LinkDotIfaceCapture::findDTypeInModule(
                        correctModp, refp->refDTypep()->name(), refp->refDTypep()->type())) {
                    refp->refDTypep(newDtp);
                    fixed = सच्च;
                }
            }
            सचे (fixed) ++fixed_गणनम्;
        });
        V3Stats::addStatSum("IfaceCapture, Ledger fixups in V3Param", fixed_गणनम्);
    }

    // Return true on success, false on error
    तार्किक deepCloneModule(Ast_ग्रन्थिः_Module* srcModp, Ast_ग्रन्थिः* ifErrorp, AstPin* paramsp,
                         ध्रुव string& newname, ध्रुव IfaceRefRefs& ifaceRefRefs) {
        // Deep clone of new module
        // Note all module internal variables will be re-linked to the new modules by clone
        // However links outside the module (like on the upper cells) will not.
        Ast_ग्रन्थिः_Module* newModp;
        सचे (srcModp->user3p()) {
            newModp = VN_CAST(srcModp->user3p()->clone_वृक्षः(मिच्छा), ग्रन्थिः_Module);
        } अञ्ञथा {
            newModp = srcModp->clone_वृक्षः(मिच्छा);
        }
        // AstPin normally retains links to external module formals across cloning. For a cloned
        // interface, relink pins whose formals were cloned with the interface while clonep() is
        // still valid, so nested parameterized classes use the cloned interface parameters.
        सचे (AstIface* ध्रुव newIfacep = VN_CAST(newModp, Iface)) {
            newIfacep->foreach([](AstPin* pinp) { pinp->cloneRelinkGen(); });
        }

        // Mark the source module as a parameterized template now that a specialized
        // clone exists.  This suppresses width/type errors on the unresolved template
        // during widthParamsEdit (which runs before V3LinkDot sets dead()).
        srcModp->parameterizedTemplate(सच्च);
        // The clone is a specialized instance, not a template.  Clear the flag in
        // case it was inherited from a prior cloneTree (when srcModp was already
        // marked by an earlier specialization).
        newModp->parameterizedTemplate(मिच्छा);

        newModp->name(newname);
        newModp->user2(मिच्छा);  // We need to re-recurse this module once changed
        newModp->recursive(मिच्छा);
        newModp->recursiveClone(मिच्छा);
        newModp->hasGParam(मिच्छा);
        // Recursion may need level cleanups
        सचे (newModp->level() <= m_modp->level()) newModp->level(m_modp->level() + 1);
        सचे ((newModp->level() - srcModp->level()) >= (v3Global.opt.moduleRecursionDepth() - 2)) {
            ifErrorp->v3error("Exceeded maximum --module-recursion-depth of "
                              << v3Global.opt.moduleRecursionDepth());
            VL_DO_DANGLING(newModp->delete_वृक्षः(), newModp);
            पटिनिवत्त मिच्छा;
        }

        सचे (AstClass* ध्रुव newClassp = VN_CAST(newModp, Class)) {
            replaceRefsRecurse(newModp->stmtsp(), newClassp, VN_AS(srcModp, Class));
        }

        // Keep tree sorted by level. Note: Different parameterizations of the same recursive
        // module end up with the same level, which we will need to fix up at the end, as we do not
        // know up front how recursive modules are expanded, and a later expansion might re-use an
        // earlier expansion (see t_recursive_module_bug_2).
        Ast_ग्रन्थिः* insertp = srcModp;
        याव (VN_IS(insertp->nextp(), ग्रन्थिः_Module)
               && VN_AS(insertp->nextp(), ग्रन्थिः_Module)->level() <= newModp->level()) {
            insertp = insertp->nextp();
        }
        insertp->addNextHere(newModp);

        m_modNameMap.emplace(newModp->name(), ModInfo{newModp});
        ध्रुव auto iter = m_modNameMap.find(newname);
        CloneMap* ध्रुव clonemapp = &(iter->second.m_cloneMap);
        UINFO(4, "     De-parameterize to new: " << newModp);

        // Grab all I/O so we can remap our pins later
        // Note we allow multiple users of a parameterized model,
        // thus we need to stash this info.
        collectPins(clonemapp, newModp, srcModp->user3p());
        // Relink parameter vars to the new module
        // For interface ports (e.g., l3_if #(W, L0A_W) l3), the parameter pins may
        // reference variables from the enclosing module rather than from the interface
        // being cloned. In such cases, use relinkPinsByName to match by variable name.
        // Check if any parameter pins reference variables outside the cloned interface.
        // This is O(n) but acceptable since parameter pin lists are typically small (<10 pins).
        तार्किक needRelinkByName = मिच्छा;
        सचे (paramsp) {
            अत्थाय (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                सचे (pinp->modVarp() && clonemapp->find(pinp->modVarp()) == clonemapp->end()) {
                    needRelinkByName = सच्च;
                    भङ्ग;
                }
            }
        }
        सचे (needRelinkByName) {
            relinkPinsByName(paramsp, newModp);
        } अञ्ञथा {
            relinkPins(clonemapp, paramsp);
        }

        // Fix any interface references
        अत्थाय (auto it = ifaceRefRefs.cbegin(); it != ifaceRefRefs.cend(); ++it) {
            ध्रुव AstIfaceRefDType* ध्रुव portIrefp = it->first;
            ध्रुव AstIfaceRefDType* ध्रुव pinIrefp = it->second;
            AstIfaceRefDType* ध्रुव cloneIrefp = portIrefp->clonep();
            UINFO(8, "     IfaceOld " << portIrefp);
            UINFO(8, "     IfaceTo  " << pinIrefp);
            UASSERT_OBJ(cloneIrefp, portIrefp, "parameter clone didn't hit AstIfaceRefDType");
            UINFO(8, "     IfaceClo " << cloneIrefp);
            cloneIrefp->ifacep(pinIrefp->ifaceViaCellp());
            UINFO(8, "     IfaceNew " << cloneIrefp);
        }

        // Fix VarXRefs that reference variables in old interfaces.
        // Now that interface port dtypes have been updated above, we can use them
        // to find the correct interface for each VarXRef.
        सचे (!ifaceRefRefs.empty()) { VarXRefRelinkVisitor{newModp}; }

        // Fix cross-module REFDTYPE pointers in newModp.
        UASSERT_OBJ(newModp, srcModp, "newModp null before hierarchy fixup");
        fixupCrossModuleRefDTypes(newModp);

        // Assign parameters to the constants specified
        // DOES clone() so must be finished with module clonep() before here
        अत्थाय (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            सचे (pinp->exprp()) {
                सचे (AstVar* ध्रुव modvarp = pinp->modVarp()) {
                    Ast_ग्रन्थिः* ध्रुव newp = pinp->exprp();  // Const or ConsArray
                    AstConst* ध्रुव exprp = VN_CAST(newp, Const);
                    AstConst* ध्रुव origp = VN_CAST(modvarp->valuep(), Const);
                    ध्रुव तार्किक overridden
                        = !(origp && exprp && ParameterizedHierBlocks::areSame(exprp, origp));
                    // Remove any existing parameter
                    सचे (modvarp->valuep()) modvarp->valuep()->unlinkFrBack()->delete_वृक्षः();
                    // Set this parameter to value requested by cell
                    UINFO(9, "       set param " << modvarp << " = " << newp);
                    modvarp->valuep(newp->clone_वृक्षः(मिच्छा));
                    modvarp->overriddenParam(overridden);
                } अञ्ञथा सचे (AstParamTypeDType* ध्रुव modptp = pinp->modPTypep()) {
                    Ast_ग्रन्थिः_DType* ध्रुव dtypep = VN_AS(pinp->exprp(), ग्रन्थिः_DType);
                    UASSERT_OBJ(dtypep, pinp, "unlinked param dtype");
                    सचे (modptp->childDTypep()) modptp->childDTypep()->unlinkFrBack()->delete_वृक्षः();
                    // Set this parameter to value requested by cell
                    modptp->childDTypep(dtypep->clone_वृक्षः(मिच्छा));
                    // Later V3LinkDot will convert the ParamDType to a Typedef
                    // Not done here as may be localparams, etc, that also need conversion
                }
            }
        }

        पटिनिवत्त सच्च;
    }
    ध्रुव ModInfo* moduleFindOrClone(Ast_ग्रन्थिः_Module* srcModp, Ast_ग्रन्थिः* ifErrorp, AstPin* paramsp,
                                     ध्रुव string& newname, ध्रुव IfaceRefRefs& ifaceRefRefs) {
        // Already made this flavor?
        auto it = m_modNameMap.find(newname);
        सचे (it != m_modNameMap.end()) {
            UINFO(4, "     De-parameterize to prev: " << it->second.m_modp);
        } अञ्ञथा {
            सचे (!deepCloneModule(srcModp, ifErrorp, paramsp, newname, ifaceRefRefs)) {
                पटिनिवत्त nullptr;
            }
            it = m_modNameMap.find(newname);
            UASSERT(it != m_modNameMap.end(), "should find just-made module");
        }
        ध्रुव ModInfo* ध्रुव modInfop = &(it->second);
        पटिनिवत्त modInfop;
    }

    शून्य convertToStringp(Ast_ग्रन्थिः* nodep) {
        // Should be called on values of parameters of type string to convert them
        // to properly typed string constants.
        // Has no effect if the value is not a string constant.
        AstConst* ध्रुव constp = VN_CAST(nodep, Const);
        // Check if it wasn't already converted
        सचे (constp && !constp->अङ्कः().isString()) {
            constp->replaceWith(
                नूतन AstConst{constp->fileline(), AstConst::String{}, constp->अङ्कः().toString()});
            VL_DO_DANGLING(constp->delete_वृक्षः(), constp);
        }
    }

    // Fold a param/lparam member's value to a Const in place.  Resolves any
    // class::member Dots in the value first (including those buried behind
    // VarRefs to sibling lparams that are not yet const, e.g. `one = base`
    // where `base = inner_a::v`), resolving siblings deepest-first so the
    // whole chain constifies.  `inProgress` guards against reference cycles.
    शून्य constify_Member_मानम्(AstVar* varp, std::set<AstVar*>& inProgress) {
        सचे (!varp->valuep() || VN_IS(varp->valuep(), Const)) पटिनिवत्त;
        सचे (!inProgress.निवेशय(varp).second) {
            // Re-entered while still resolving varp: its value transitively
            // references itself.  Report and replace the value with a Const, as
            // otherwise the unresolved Dot reaches V3Width as an untyped node.
            varp->v3error("Variable's initial value is circular: " << varp->prettyNameQ());
            varp->valuep()->unlinkFrBack()->delete_वृक्षः();
            varp->valuep(नूतन AstConst{varp->fileline(), AstConst::Signed32{}, 0});
            पटिनिवत्त;
        }
        // First resolve sibling lparams this value references, so their Dots
        // are folded before we inline them here.
        std::set<AstVar*> siblings;
        varp->valuep()->foreach([&](ध्रुव AstVarRef* refp) {
            AstVar* ध्रुव refVarp = refp->varp();
            सचे (refVarp && refVarp->isParam() && refVarp->valuep()
                && !VN_IS(refVarp->valuep(), Const)) {
                siblings.निवेशय(refVarp);
            }
        });
        अत्थाय (AstVar* ध्रुव sibp : siblings) constify_Member_मानम्(sibp, inProgress);
        // Resolve class-scoped references and any member selections they expose.
        resolveDeferredDotsReachableFrom(varp->valuep(), m_modp);
        V3Const::constifyParamsEdit(varp);
        inProgress.erase(varp);
    }

    // Return the member name on a deferred Dot RHS, including a selected member.
    स्थिर AstParseRef* memberParseRef(Ast_ग्रन्थिः* nodep) {
        सचे (AstParseRef* ध्रुव refp = VN_CAST(nodep, ParseRef)) पटिनिवत्त refp;
        Ast_ग्रन्थिः_Pre_Sel* ध्रुव selp = VN_CAST(nodep, ग्रन्थिः_Pre_Sel);
        पटिनिवत्त selp ? VN_CAST(selp->fromp(), ParseRef) : nullptr;
    }

    // Replace a deferred Dot with valuep, preserving any select on the member name.
    स्थिर शून्य replaceMemberDot(AstDot* dotp, AstParseRef* memberRefp, Ast_ग्रन्थिः_व्यञ्जकम्* valuep) {
        सचे (memberRefp == dotp->rhsp()) {
            dotp->replaceWith(valuep);
        } अञ्ञथा {
            Ast_ग्रन्थिः_Pre_Sel* ध्रुव selp = VN_AS(dotp->rhsp(), ग्रन्थिः_Pre_Sel);
            सचे (AstAttrOf* ध्रुव attrp = selp->attrp()) {
                सचे (Ast_ग्रन्थिः* ध्रुव oldFromp = attrp->fromp()) {
                    oldFromp->replaceWith(valuep->clone_वृक्षः(मिच्छा));
                    VL_DO_DANGLING(oldFromp->delete_वृक्षः(), oldFromp);
                }
            }
            memberRefp->replaceWith(valuep);
            VL_DO_DANGLING(memberRefp->delete_वृक्षः(), memberRefp);
            dotp->replaceWith(dotp->rhsp()->unlinkFrBack());
        }
        VL_DO_DANGLING(dotp->delete_वृक्षः(), dotp);
    }

    // True for an expression produced while lowering a deferred struct-member chain.
    स्थिर तार्किक isDeferredMemberBase(ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* nodep) {
        सचे (VN_IS(nodep, MemberSel)) पटिनिवत्त सच्च;
        सचे (ध्रुव Ast_ग्रन्थिः_Pre_Sel* ध्रुव selp = VN_CAST(nodep, ग्रन्थिः_Pre_Sel)) {
            ध्रुव Ast_ग्रन्थिः_DType* ध्रुव fromDTypep = selp->fromp()->dtypep();
            ध्रुव Ast_ग्रन्थिः_Array_DType* ध्रुव arrayDTypep
                = fromDTypep ? VN_CAST(fromDTypep->skipRefOrNullp(), ग्रन्थिः_Array_DType) : nullptr;
            सचे (arrayDTypep) {
                ध्रुव Ast_ग्रन्थिः_DType* ध्रुव elemDTypep = arrayDTypep->subDTypep();
                पटिनिवत्त elemDTypep && VN_IS(elemDTypep->skipRefOrNullp(), ग्रन्थिः_UOr_Struct_DType);
            }
            पटिनिवत्त isDeferredMemberBase(selp->fromp());
        }
        ध्रुव Ast_ग्रन्थिः_DType* ध्रुव dtypep = nodep->dtypep();
        पटिनिवत्त dtypep && VN_IS(dtypep->skipRefOrNullp(), ग्रन्थिः_UOr_Struct_DType);
    }

    // Lower a Dot exposed after its inner class::member reference was substituted.
    स्थिर शून्य resolveMemberDot(AstDot* dotp) {
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhsp = VN_CAST(dotp->lhsp(), ग्रन्थिः_व्यञ्जकम्);
        AstParseRef* ध्रुव memberRefp = memberParseRef(dotp->rhsp());
        सचे (!lhsp || !memberRefp || !isDeferredMemberBase(lhsp)) पटिनिवत्त;
        AstMemberSel* ध्रुव newp = नूतन AstMemberSel{memberRefp->fileline(), lhsp->unlinkFrBack(),
                                                    VFlagChildDType{}, memberRefp->name()};
        replaceMemberDot(dotp, memberRefp, newp);
    }

    // Helper to resolve DOT to RefDType for class type references.
    // If the class is parameterized and not yet specialized, specialize it first.
    // This handles cases like: iface #(param_class#(value)::typedef_name)
    शून्य resolveDotToTypedef(Ast_ग्रन्थिः* exprp) {
        AstDot* ध्रुव dotp = VN_CAST(exprp, Dot);
        सचे (!dotp) पटिनिवत्त;
        UINFO(9, "Resolve deferred Dot: " << dotp);
        AstClassOrPackageRef* ध्रुव classRefp = VN_CAST(dotp->lhsp(), ClassOrPackageRef);
        सचे (!classRefp) {
            resolveMemberDot(dotp);
            पटिनिवत्त;
        }
        AstParseRef* ध्रुव parseRefp = memberParseRef(dotp->rhsp());
        सचे (!parseRefp) पटिनिवत्त;

        ध्रुव AstClass* lhsClassp = VN_CAST(classRefp->classOrPackageSkipp(), Class);

        // Specialize parameterized class through type parameter indirection (#7000)
        AstParamTypeDType* ध्रुव paramTypep
            = VN_CAST(classRefp->classOrPackageNodep(), ParamTypeDType);
        सचे (paramTypep) {
            Ast_ग्रन्थिः_DType* ध्रुव dtypep = paramTypep->subDTypep();
            AstClassRefDType* ध्रुव classRefDTypep
                = dtypep ? VN_CAST(dtypep->skipRefOrNullp(), ClassRefDType) : nullptr;
            सचे (classRefDTypep) {
                AstClass* ध्रुव srcClassp = classRefDTypep->classp();
                सचे (srcClassp && srcClassp->hasGParam() && classRefDTypep->paramsp()) {
                    सचे (lhsClassp == srcClassp || !lhsClassp) {
                        classRefDeparam(classRefDTypep, srcClassp);
                        lhsClassp = classRefDTypep->classp();
                    }
                }
            }
        }

        सचे (classRefp->paramsp()) {
            AstClass* ध्रुव srcClassp = VN_CAST(classRefp->classOrPackageNodep(), Class);
            सचे (srcClassp && srcClassp->hasGParam()) {
                सचे (lhsClassp == srcClassp || !lhsClassp) {
                    classRefDeparam(classRefp, srcClassp);
                    lhsClassp = VN_CAST(classRefp->classOrPackageSkipp(), Class);
                }
            }
        }
        // Typedef-aliased paramed class (e.g., typedef C#(7) my_c; my_c::b).
        सचे (AstClassRefDType* ध्रुव crp = class_Ref_DType_Of_ग्रन्थिः(classRefp->classOrPackageNodep())) {
            सचे (AstClass* ध्रुव srcClassp = crp->classp()) {
                सचे (srcClassp->hasGParam() && crp->paramsp()) {
                    classRefDeparam(crp, srcClassp);
                    सचे (AstClass* ध्रुव newClassp = crp->classp()) lhsClassp = newClassp;
                }
            }
        }
        सचे (!lhsClassp) पटिनिवत्त;

        Ast_ग्रन्थिः* ध्रुव memberp = m_memberMap.findMember(lhsClassp, parseRefp->name());
        UINFO(9, "Resolve deferred class member: " << parseRefp->name() << " -> " << memberp);
        सचे (AstTypedef* ध्रुव tdefp = VN_CAST(memberp, Typedef)) {
            सचे (parseRefp != dotp->rhsp()) पटिनिवत्त;
            AstRefDType* ध्रुव refp = नूतन AstRefDType{dotp->fileline(), tdefp->name()};
            refp->typedefp(tdefp);
            dotp->replaceWith(refp);
            VL_DO_DANGLING(dotp->delete_वृक्षः(), dotp);
        } अञ्ञथा सचे (AstVar* ध्रुव varp = VN_CAST(memberp, Var)) {
            substituteParamMember(dotp, parseRefp, varp);
        }
    }

    // Substitute a class param/lparam member's constant value for dotp. The
    // reverse-order deferred-Dot walk lowers any outer fields to MemberSels.
    शून्य substituteParamMember(AstDot* ध्रुव dotp, AstParseRef* ध्रुव memberRefp,
                               AstVar* ध्रुव varp) {
        सचे (!varp->isParam() || !varp->valuep()) पटिनिवत्त;
        सचे (!VN_IS(varp->valuep(), Const)) {
            std::set<AstVar*> inProgress;
            constify_Member_मानम्(varp, inProgress);
        }
        AstConst* ध्रुव constp = VN_CAST(varp->valuep(), Const);
        सचे (!constp) पटिनिवत्त;
        AstConst* ध्रुव newp = constp->clone_वृक्षः(मिच्छा);
        newp->dtypep(varp->subDTypep());
        replaceMemberDot(dotp, memberRefp, newp);
    }

    // Resolve one class-scoped RefDType. The generic deferred walk finds these
    // through every dtype wrapper and follows out-of-tree typedef references.
    शून्य resolveParamClassRefDType(AstRefDType* refp) {
        सचे (refp->typedefp() || refp->refDTypep()) पटिनिवत्त;

        AstClassOrPackageRef* ध्रुव classRefp
            = VN_CAST(refp->classOrPackageOpp(), ClassOrPackageRef);
        सचे (!classRefp) पटिनिवत्त;

        AstClass* srcClassp = VN_CAST(classRefp->classOrPackageNodep(), Class);
        सचे (srcClassp && srcClassp->hasGParam()) {
            classRefDeparam(classRefp, srcClassp);
            पटिनिवत्त;
        }

        // Follow typedef chain to find the underlying ClassRefDType
        Ast_ग्रन्थिः* targetp = classRefp->classOrPackageNodep();
        याव (ध्रुव AstTypedef* ध्रुव tdefp = VN_CAST(targetp, Typedef))
            targetp = tdefp->subDTypep();
        सचे (Ast_ग्रन्थिः_DType* ध्रुव dtargetp = VN_CAST(targetp, ग्रन्थिः_DType))
            targetp = dtargetp->skipRefOrNullp();
        सचे (!targetp) पटिनिवत्त;
        AstClassRefDType* ध्रुव classRefDTypep = VN_CAST(targetp, ClassRefDType);
        सचे (!classRefDTypep) पटिनिवत्त;
        srcClassp = classRefDTypep->classp();
        सचे (!srcClassp) पटिनिवत्त;

        AstClass* newClassp = srcClassp;
        सचे (srcClassp->hasGParam()) {
            classRefDeparam(classRefDTypep, srcClassp);
            newClassp = classRefDTypep->classp();
        }

        AstTypedef* ध्रुव typedefp
            = VN_CAST(m_memberMap.findMember(newClassp, refp->name()), Typedef);
        सचे (typedefp) {
            refp->typedefp(typedefp);
            refp->classOrPackagep(newClassp);
        }
    }

    // True if a $bits/$size type query in nodep's parameters reads another type parameter.
    स्थिर तार्किक defaultParamsHaveTypeQueryOnParamType(ध्रुव AstClassRefDType* nodep) {
        तार्किक found = मिच्छा;
        nodep->foreach([&](ध्रुव AstAttrOf* attrp) {
            सचे (found || !attrp->attrType().isTypeQuery()) पटिनिवत्त;
            ध्रुव AstRefDType* ध्रुव refp = VN_CAST(attrp->fromp(), RefDType);
            सचे (refp && VN_IS(refp->refDTypep(), ParamTypeDType)) found = सच्च;
        });
        पटिनिवत्त found;
    }

    // Check if exprp's class matches origp's class after deparameterization.
    // Handles both the simple case (user4p link from defaultsResolved) and the
    // nested case where the default's inner class has non-default sub-parameters
    // (e.g., uvm_sequence#(uvm_reg_item) where uvm_reg_item != default uvm_sequence_item).
    तार्किक classTypeMatchesDefaultClone(ध्रुव Ast_ग्रन्थिः_DType* exprp, ध्रुव Ast_ग्रन्थिः_DType* origp) {
        exprp = exprp->skipRefp();
        origp = origp->skipRefp();
        ध्रुव auto* ध्रुव व्यञ्जकम्_Class_Refp = VN_CAST(exprp, ClassRefDType);
        ध्रुव auto* ध्रुव origClassRefp = VN_CAST(origp, ClassRefDType);
        सचे (!व्यञ्जकम्_Class_Refp || !origClassRefp) पटिनिवत्त मिच्छा;
        // Fast path: check user4p link (set when template was deparameterized with defaults)
        ध्रुव Ast_ग्रन्थिः_Module* ध्रुव defaultClonep
            = VN_CAST(origClassRefp->classp()->user4p(), Class);
        सचे (defaultClonep && defaultClonep == व्यञ्जकम्_Class_Refp->classp()) पटिनिवत्त सच्च;
        // Skip the comparison when the default's $bits/$size reads another type parameter, as
        // deparameterizing it below would resolve that shared type at the wrong width (#7711).
        सचे (defaultParamsHaveTypeQueryOnParamType(origClassRefp)) पटिनिवत्त मिच्छा;
        // Slow path: deparameterize the default type and compare the result.
        // Different templates can never match; use origName() because exprp's
        // class may already be a specialization (clone) of the template.
        सचे (!origClassRefp->classp()->hasGParam()) पटिनिवत्त मिच्छा;
        सचे (origClassRefp->classp()->origName() != व्यञ्जकम्_Class_Refp->classp()->origName())
            पटिनिवत्त मिच्छा;
        // Prevent re-entry when classRefDeparam recurses through cellPinCleanup.
        सचे (!m_defaultCloneInProgress.निवेशय(origClassRefp->classp()).second) पटिनिवत्त मिच्छा;
        // const_cast safe: cloneTree doesn't modify the source
        AstClassRefDType* ध्रुव origClonep = static_cast<AstClassRefDType*>(
            const_cast<AstClassRefDType*>(origClassRefp)->clone_वृक्षः(मिच्छा));
        Ast_ग्रन्थिः_Module* ध्रुव resolvedModp = classRefDeparam(origClonep, origClassRefp->classp());
        ध्रुव तार्किक match = resolvedModp && VN_CAST(resolvedModp, Class) == व्यञ्जकम्_Class_Refp->classp();
        VL_DO_DANGLING(origClonep->delete_वृक्षः(), origClonep);
        m_defaultCloneInProgress.erase(origClassRefp->classp());
        पटिनिवत्त match;
    }

    स्थिर तार्किक paramConstsEqualAtMaxWidth(AstConst* exprp, AstConst* origp) {
        // Return true if two integer constants are equal after sign-extending
        // both to max(width).  A typed parameter default (e.g. byte) is
        // narrower than a 32-bit pin expression, so sameTree/areSame fail.
        सचे (exprp->अङ्कः().width() == origp->अङ्कः().width()) पटिनिवत्त मिच्छा;
        सचे (exprp->अङ्कः().isOpaque()) पटिनिवत्त मिच्छा;
        सचे (origp->अङ्कः().isOpaque()) पटिनिवत्त मिच्छा;
        ध्रुव पूर्णाङ्क maxWidth = std::max(exprp->अङ्कः().width(), origp->अङ्कः().width());
        V3_सङ्ख्या व्यञ्जकम्_अङ्कः{exprp, maxWidth};
        सचे (exprp->अङ्कः().isSigned()) {
            व्यञ्जकम्_अङ्कः.opExtendS(exprp->अङ्कः(), exprp->अङ्कः().width());
        } अञ्ञथा {
            व्यञ्जकम्_अङ्कः.opAssign(exprp->अङ्कः());
        }
        V3_सङ्ख्या orig_अङ्कः{origp, maxWidth};
        सचे (origp->अङ्कः().isSigned()) {
            orig_अङ्कः.opExtendS(origp->अङ्कः(), origp->अङ्कः().width());
        } अञ्ञथा {
            orig_अङ्कः.opAssign(origp->अङ्कः());
        }
        V3_सङ्ख्या isEq{exprp, 1};
        isEq.opEq(व्यञ्जकम्_अङ्कः, orig_अङ्कः);
        पटिनिवत्त isEq.isNeqZero();
    }

    // Rewrite placeholder __VGIfaceParam pins emitted by
    // LinkDotResolveVisitor::addImplicitParametersOfGenericIface for a nested
    // generic-interface forwarding chain (#7454). Must run here (not in
    // V3LinkDot's Param/Resolve visitor at LDS_PARAMED) because:
    //   * linkDotParamed runs AFTER V3Param, at which point cellDeparam has
    //     already consumed the pin via moduleFindOrClone (variant naming off
    //     pinp->exprp()'s IfaceRefDType) and genericInterfaceVarSetup (hard
    //     VN_AS cast to IfaceRefDType); and
    //   * the enclosing module's port only becomes a concrete IfaceRefDType
    //     after its own nodeDeparam has specialized it. That specialization
    //     happens in V3Param's top-down walk, so the window between "outer
    //     port resolved" and "inner cellDeparam starts" exists only here.
    शून्य resolveGenericIfaceForwardingPins(AstPin* paramsp) {
        अत्थाय (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            सचे (!pinp->exprp()) अनुवर्त;
            सचे (!VString::startsWith(pinp->name(), "__VGIfaceParam")) अनुवर्त;
            ध्रुव Ast_ग्रन्थिः_Var_Ref* ध्रुव fwdRefp = VN_CAST(pinp->exprp(), ग्रन्थिः_Var_Ref);
            सचे (!fwdRefp) अनुवर्त;
            ध्रुव AstIfaceRefDType* ध्रुव resolvedp
                = VN_CAST(fwdRefp->varp()->childDTypep()->skipRefp(), IfaceRefDType);
            UASSERT_OBJ(resolvedp, pinp,
                        "Generic interface forwarding pin not specialized before use");
            AstIfaceRefDType* ध्रुव newIrefp = नूतन AstIfaceRefDType{
                resolvedp->fileline(), resolvedp->modportFileline(), resolvedp->cellName(),
                resolvedp->ifaceName(), resolvedp->modportName()};
            newIrefp->ifacep(resolvedp->ifacep());
            सचे (resolvedp->paramsp()) {
                newIrefp->addParamsp(resolvedp->paramsp()->clone_वृक्षः(सच्च));
            }
            pinp->exprp()->unlinkFrBack()->delete_वृक्षः();
            pinp->exprp(newIrefp);
        }
    }

    // Include the pin's value in the specialization name, so cells passing equal
    // values share one module clone.
    शून्य name_By_Pin_मानम्(AstPin* pinp, Ast_ग्रन्थिः_Module* srcModp, AstVar* modvarp, string& longnamer,
                        तार्किक& any_overridesr) {
        longnamer += "_" + paramSmallName(srcModp, modvarp) + param_मानम्_सङ्ख्या(pinp->exprp());
        any_overridesr = सच्च;
    }

    // Name the specialization by a value pin, or prepare a type pin, noting in
    // overridingTypePinsr whether it differs from the default
    शून्य cellPinCleanup(Ast_ग्रन्थिः* nodep, AstPin* pinp, AstPin* paramsp, Ast_ग्रन्थिः_Module* srcModp,
                        string& longnamer, तार्किक& any_overridesr,
                        std::unordered_set<ध्रुव AstPin*>& overridingTypePinsr) {
        सचे (!pinp->exprp()) पटिनिवत्त;  // No-connect
        सचे (AstVar* ध्रुव modvarp = pinp->modVarp()) {
            resolveDeferredDotsReachableFrom(modvarp->subDTypep(), m_modp);
            सचे (!modvarp->isGParam()) {
                pinp->v3fatalSrc("Attempted parameter setting of non-parameter: Param "
                                 << pinp->prettyNameQ() << " of " << nodep->prettyNameQ());
            } अञ्ञथा सचे (VN_IS(pinp->exprp(), Pattern)
                       || (VN_IS(pinp->exprp(), ConsArray)
                           && arraySubDTypep(modvarp->subDTypep()))) {
                // A pattern or array value is named by its contents
                name_By_Pin_मानम्(pinp, srcModp, modvarp, longnamer, any_overridesr);
            } अञ्ञथा {
                UINFO(9, "cellPinCleanup: before constify " << pinp << " " << modvarp);
                V3Const::constifyParamsEdit(pinp->exprp());
                सचे (is_Aggregate_Param_मानम्(pinp->exprp())) {
                    name_By_Pin_मानम्(pinp, srcModp, modvarp, longnamer, any_overridesr);
                    पटिनिवत्त;
                }
                // Cast/CastSize default values are not yet folded by V3Width.
                // Constify here so the comparison below sees a Const node.
                // Other node kinds are handled in the branches above.
                //
                // Only fold when the cast is self-contained.  modvarp is the
                // parameter on the shared module template and constifyParamsEdit
                // is destructive: a default that reads another parameter
                // (VarRef) or a type parameter (RefDType) must be evaluated per
                // instance, and folding it here would evaluate it against the
                // template's own defaults and bake that in for every later
                // instance that relies on the default.
                सचे (modvarp->valuep()
                    && (VN_IS(modvarp->valuep(), Cast) || VN_IS(modvarp->valuep(), CastSize))) {
                    ध्रुव तार्किक dependent = modvarp->valuep()->exists(
                        [](Ast_ग्रन्थिः* np) { पटिनिवत्त VN_IS(np, VarRef) || VN_IS(np, RefDType); });
                    सचे (!dependent) V3Const::constifyParamsEdit(modvarp->valuep());
                }
                UINFO(9, "cellPinCleanup: after constify " << pinp);
                // String constants are parsed as logic arrays and converted to strings in V3Const.
                // At this moment, some constants may have been already converted.
                // To correctly compare constants, both should be of the same type,
                // so they need to be converted.
                सचे (isString(modvarp->subDTypep())) {
                    convertToStringp(pinp->exprp());
                    convertToStringp(modvarp->valuep());
                }
                AstConst* ध्रुव exprp = VN_CAST(pinp->exprp(), Const);
                AstConst* ध्रुव origp = VN_CAST(modvarp->valuep(), Const);
                // Width the pin to the port's type so equal values hash the same (#5479).
                AstConst* normedNamep = nullptr;
                सचे (exprp && !exprp->अङ्कः().isDouble() && !exprp->अङ्कः().isString()) {
                    AstVar* cloneVarp = modvarp->clone_वृक्षः(मिच्छा);
                    तार्किक cloneVarpUnresolved = मिच्छा;
                    सचे (Ast_ग्रन्थिः* ध्रुव oldValuep = cloneVarp->valuep()) {
                        oldValuep->unlinkFrBack();
                        VL_DO_DANGLING(oldValuep->delete_वृक्षः(), oldValuep);
                    }
                    cloneVarp->valuep(exprp->clone_वृक्षः(मिच्छा));
                    सचे (Ast_ग्रन्थिः_DType* ध्रुव origDTypep = modvarp->subDTypep()) {
                        // Attach clone under cloneVarp so the root has a back pointer.
                        सचे (cloneVarp->childDTypep())
                            cloneVarp->childDTypep()->unlinkFrBack()->delete_वृक्षः();
                        cloneVarp->childDTypep(origDTypep->clone_वृक्षः(मिच्छा));
                        cloneVarp->dtypep(nullptr);
                        // Inline param refs so widthing doesn't touch the template (#7411).
                        constexpr पूर्णाङ्क maxSubstIters = 1000;
                        अत्थाय (पूर्णाङ्क it = 0; it < maxSubstIters; ++it) {
                            तार्किक any = मिच्छा;
                            cloneVarp->foreach([&](AstVarRef* varrefp) {
                                AstVar* ध्रुव targetp = varrefp->varp();
                                Ast_ग्रन्थिः* replacep = nullptr;
                                अत्थाय (AstPin* pp = paramsp; pp; pp = VN_AS(pp->nextp(), Pin)) {
                                    सचे (pp->modVarp() == targetp) {
                                        // A pattern or array override names the value too
                                        सचे (VN_IS(pp->exprp(), Const)
                                            || VN_IS(pp->exprp(), Pattern)
                                            || VN_IS(pp->exprp(), ConsArray)) {
                                            replacep = pp->exprp()->clone_वृक्षः(मिच्छा);
                                        }
                                        भङ्ग;
                                    }
                                }
                                सचे (!replacep && targetp->valuep()) {
                                    replacep = targetp->valuep()->clone_वृक्षः(मिच्छा);
                                }
                                // An inlined pattern takes the type of its parameter
                                AstPattern* ध्रुव patp = VN_CAST(replacep, Pattern);
                                सचे (patp && !patp->childDTypep() && targetp->childDTypep()) {
                                    patp->childDTypep(targetp->childDTypep()->clone_वृक्षः(मिच्छा));
                                }
                                सचे (replacep) {
                                    varrefp->replaceWith(replacep);
                                    VL_DO_DANGLING(varrefp->delete_वृक्षः(), varrefp);
                                    any = सच्च;
                                }
                            });
                            // Replace RefDType to a ParamTypeDType with pin override
                            // or the paramtype's default so constify below does not
                            // reach into the template.  Collect then replace in
                            // reverse so descendants aren't freed early.
                            std::vector<std::pair<AstRefDType*, Ast_ग्रन्थिः_DType*>> toReplace;
                            cloneVarp->foreach([&](AstRefDType* refp) {
                                AstParamTypeDType* ध्रुव ptdp
                                    = VN_CAST(refp->refDTypep(), ParamTypeDType);
                                सचे (!ptdp) पटिनिवत्त;
                                AstPin* overridePinp = nullptr;
                                अत्थाय (AstPin* pp = paramsp; pp; pp = VN_AS(pp->nextp(), Pin)) {
                                    सचे (pp->modPTypep() == ptdp) {
                                        overridePinp = pp;
                                        भङ्ग;
                                    }
                                }
                                Ast_ग्रन्थिः_DType* ध्रुव substp
                                    = overridePinp ? VN_CAST(overridePinp->exprp(), ग्रन्थिः_DType)
                                                   : ptdp->subDTypep();
                                सचे (substp) toReplace.emplace_back(refp, substp);
                            });
                            अत्थाय (auto it = toReplace.rbegin(); it != toReplace.rend(); ++it) {
                                AstRefDType* ध्रुव refp = it->first;
                                refp->replaceWith(it->second->clone_वृक्षः(मिच्छा));
                                VL_DO_DANGLING(refp->delete_वृक्षः(), refp);
                                any = सच्च;
                            }
                            सचे (!any) भङ्ग;
                        }
                        // Skip the widthing constify if anything left can't be evaluated here: a
                        // reference, as to a variable or around a cycle, an unresolved type
                        // parameter, or a type or enum item of the template
                        cloneVarp->foreach([&](Ast_ग्रन्थिः* np) {
                            सचे (VN_IS(np, VarRef)) {
                                cloneVarpUnresolved = सच्च;
                                पटिनिवत्त;
                            }
                            सचे (AstEnumItemRef* ध्रुव itemRefp = VN_CAST(np, EnumItemRef)) {
                                सचे (V3LinkDotIfaceCapture::findOwnerModule(itemRefp->itemp())
                                    == srcModp) {
                                    cloneVarpUnresolved = सच्च;
                                }
                                पटिनिवत्त;
                            }
                            AstRefDType* ध्रुव refp = VN_CAST(np, RefDType);
                            सचे (!refp) पटिनिवत्त;
                            सचे (VN_IS(refp->refDTypep(), ParamTypeDType)) {
                                UINFO(5, "  cellPinCleanup: skip normedNamep "
                                         "(unresolved RefDType->ParamTypeDType) pin="
                                             << pinp->prettyNameQ());
                                cloneVarpUnresolved = सच्च;
                            }
                            // A type declared in the template must not be widthed there
                            सचे (V3LinkDotIfaceCapture::findOwnerModule(refp->typedefp()) == srcModp
                                || V3LinkDotIfaceCapture::findOwnerModule(refp->refDTypep())
                                       == srcModp) {
                                cloneVarpUnresolved = सच्च;
                            }
                        });
                    }
                    सचे (!cloneVarpUnresolved) {
                        V3Const::constifyParamsEdit(cloneVarp);
                        सचे (AstConst* ध्रुव widthedp = VN_CAST(cloneVarp->valuep(), Const)) {
                            // Stamp the port's type so equal values hash the same.
                            सचे (cloneVarp->dtypep()) widthedp->dtypep(cloneVarp->dtypep());
                            widthedp->unlinkFrBack();
                            normedNamep = widthedp;
                        }
                    }
                    VL_DO_DANGLING(cloneVarp->delete_वृक्षः(), cloneVarp);
                }
                AstConst* ध्रुव namingExprp = normedNamep ? normedNamep : exprp;
                सचे (!exprp) {
                    // With DepGraph architecture, all expressions should be constants
                    // by the time V3Param runs. If not, it's an error.
                    UINFO(9, "cellPinCleanup: NOT CONST after constify " << pinp);
                    UINFOTREE(1, pinp, "", "errnode");
                    pinp->v3error("Can't convert defparam value to constant: Param "
                                  << pinp->prettyNameQ() << " of " << nodep->prettyNameQ());
                    Ast_ग्रन्थिः* ध्रुव pinExprp = pinp->exprp();
                    pinExprp->replaceWith(नूतन AstConst{pinp->fileline(), AstConst::Widthed_मानम्{},
                                                       modvarp->width(), 0});
                    VL_DO_DANGLING(pinExprp->delete_वृक्षः(), pinExprp);
                } अञ्ञथा सचे (origp
                           && (exprp->same_वृक्षः(origp)
                               || (exprp->अङ्कः().width() == origp->अङ्कः().width()
                                   && ParameterizedHierBlocks::areSame(exprp, origp))
                               || paramConstsEqualAtMaxWidth(exprp, origp))) {
                    // Setting parameter to its default value.  Just ignore it.
                    // This prevents making additional modules, and makes coverage more
                    // obvious as it won't show up under a unique module page name.
                    UINFO(9, "cellPinCleanup: same as default " << pinp);
                } अञ्ञथा सचे (namingExprp->अङ्कः().isDouble() || namingExprp->अङ्कः().isString()
                           || namingExprp->अङ्कः().isFourState()
                           || namingExprp->अङ्कः().width() != 32) {
                    longnamer += ("_" + paramSmallName(srcModp, modvarp)
                                  + param_मानम्_सङ्ख्या(namingExprp));
                    any_overridesr = सच्च;
                } अञ्ञथा {
                    longnamer += ("_" + paramSmallName(srcModp, modvarp)
                                  + namingExprp->अङ्कः().ascii(मिच्छा));
                    any_overridesr = सच्च;
                }
                सचे (normedNamep) VL_DO_DANGLING(normedNamep->delete_वृक्षः(), normedNamep);
            }
        } अञ्ञथा सचे (AstParamTypeDType* ध्रुव modvarp = pinp->modPTypep()) {
            // Resolve the pin and declared type through arbitrary dtype wrappers
            // and typedef chains before widthing either one.
            resolveDeferredDotsReachableFrom(pinp->exprp(), m_modp);
            resolveDeferredDotsReachableFrom(modvarp->subDTypep(), m_modp);

            Ast_ग्रन्थिः_DType* rawTypep = VN_CAST(pinp->exprp(), ग्रन्थिः_DType);
            // Guard against widthing a struct/union still owned by a
            // parameterized template interface (not yet specialized).
            // widthParamsEdit is destructive: it evaluates range expressions,
            // sets didWidth=1, and removes Range nodes, which would corrupt
            // the template's BASICDTYPEs for all subsequent clones.
            // Triggered by deeply nested parameterized interfaces (e.g.
            // outer_if containing inner_if with struct typedefs) when the
            // inner interface hasn't been specialized yet.
            तार्किक skipWidthForTemplateStruct = मिच्छा;
            {
                // Use non-asserting skip: before widthParamsEdit, type(expr)
                // constructs may contain unlinked REFDTYPEs (e.g. type(x-y))
                Ast_ग्रन्थिः_DType* ध्रुव resolvedp = rawTypep ? rawTypep->skipRefOrNullp() : nullptr;
                सचे (resolvedp && (VN_IS(resolvedp, StructDType) || VN_IS(resolvedp, UnionDType))) {
                    Ast_ग्रन्थिः_Module* ध्रुव ownerModp
                        = V3LinkDotIfaceCapture::findOwnerModule(resolvedp);
                    // Skip if owned by a parameterized interface (template or not
                    // yet cloned).  hasGParam() covers interfaces that haven't
                    // been cloned yet (parameterizedTemplate() not set).
                    सचे (ownerModp && VN_IS(ownerModp, Iface) && ownerModp->hasGParam()) {
                        skipWidthForTemplateStruct = सच्च;
                        V3Stats::addStatSum("Param, Template struct width skips", 1);
                        UINFO(9, "SKIP-WIDTH-TEMPLATE: struct="
                                     << resolvedp->prettyTypeName() << " templateOwner="
                                     << ownerModp->prettyNameQ() << " pin=" << pinp->prettyNameQ()
                                     << " of " << nodep->prettyNameQ()
                                     << " srcMod=" << srcModp->prettyNameQ());
                    }
                }
                सचे (rawTypep && !skipWidthForTemplateStruct) V3Width::widthParamsEdit(rawTypep);
            }
            Ast_ग्रन्थिः_DType* exprp = rawTypep ? rawTypep->skipRefToNonRefp() : nullptr;
            ध्रुव Ast_ग्रन्थिः_DType* origp = modvarp->skipRefToNonRefp();
            सचे (!exprp) {
                pinp->v3error("Parameter type pin value isn't a type: Param "
                              << pinp->prettyNameQ() << " of " << nodep->prettyNameQ());
            } अञ्ञथा सचे (!origp) {
                pinp->v3error("Parameter type variable isn't a type: Param "
                              << modvarp->prettyNameQ());
            } अञ्ञथा {
                UINFO(9, "Parameter type assignment expr=" << exprp << " to " << origp);
                सचे (!skipWidthForTemplateStruct) {
                    V3Const::constifyParamsEdit(pinp->exprp());  // Reconcile typedefs
                    // Constify may have caused pinp->exprp to change
                    rawTypep = VN_AS(pinp->exprp(), ग्रन्थिः_DType);
                    exprp = rawTypep->skipRefToNonRefp();
                }
                सचे (!modvarp->fwdType().is_ग्रन्थिः_Compatible(exprp)) {
                    pinp->v3error("Parameter type expression type "
                                  << exprp->prettyDTypeNameQ()
                                  << " violates parameter's forwarding type '"
                                  << modvarp->fwdType().ascii() << "'");
                }
                सचे (exprp->similarDType(origp) || classTypeMatchesDefaultClone(exprp, origp)) {
                    // Setting parameter to its default value.  Just ignore it.
                    // This prevents making additional modules, and makes coverage more
                    // obvious as it won't show up under a unique module page name.
                } अञ्ञथा {
                    सचे (!skipWidthForTemplateStruct) {
                        VL_DO_DANGLING(V3Const::constifyParamsEdit(exprp), exprp);
                        rawTypep = VN_CAST(pinp->exprp(), ग्रन्थिः_DType);
                        exprp = rawTypep ? rawTypep->skipRefToNonRefp() : nullptr;
                    }
                    // Deparameterize ClassRefDType before name generation (#7000)
                    सचे (AstClassRefDType* ध्रुव classRefDTypep = VN_CAST(exprp, ClassRefDType)) {
                        सचे (classRefDTypep->paramsp() && classRefDTypep->classp()
                            && classRefDTypep->classp()->hasGParam()) {
                            classRefDeparam(classRefDTypep, classRefDTypep->classp());
                        }
                    }
                    overridingTypePinsr.emplace(pinp);  // Named once every pin is prepared
                }
            }
        } अञ्ञथा {
            pinp->v3fatalSrc("Parameter not found in sub-module: Param "
                             << pinp->prettyNameQ() << " of " << nodep->prettyNameQ());
        }
    }

    शून्य cellInterfaceCleanup(AstPin* pinsp, Ast_ग्रन्थिः_Module* srcModp, string& longnamer,
                              तार्किक& any_overridesr, IfaceRefRefs& ifaceRefRefs) {
        अत्थाय (AstPin* pinp = pinsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            ध्रुव AstVar* ध्रुव modvarp = pinp->modVarp();
            सचे (modvarp && VN_IS(arraySubDTypeDeepp(modvarp->subDTypep()), IfaceGenericDType)) {
                अनुवर्त;
            }
            सचे (modvarp->isIfaceRef()) {
                // arraySubDTypeDeepp returns input unchanged if not an array.
                AstIfaceRefDType* ध्रुव portIrefp
                    = VN_CAST(arraySubDTypeDeepp(modvarp->subDTypep()), IfaceRefDType);
                ध्रुव Ast_ग्रन्थिः* ध्रुव exprp = pinp->exprp();
                सचे (VN_IS(exprp, CellArrayRef)) {
                    // CellArrayRef not yet linked; V3LinkDot resolves this pin later.
                    UINFO(9, "Skipping interface cleanup for CellArrayRef pin: " << pinp);
                    अनुवर्त;
                }
                AstIfaceRefDType* pinIrefp = nullptr;
                // Pin is a VarRef to a var of (possibly arrayed) iface type.
                सचे (ध्रुव Ast_ग्रन्थिः_Var_Ref* ध्रुव vrp = VN_CAST(exprp, ग्रन्थिः_Var_Ref)) {
                    सचे (vrp->varp()) {
                        pinIrefp
                            = VN_CAST(arraySubDTypeDeepp(vrp->varp()->subDTypep()), IfaceRefDType);
                    }
                }
                // Pin is a select, of any depth, of a VarRef (e.g. SelBit/ArraySel into an iface
                // array, or a slice of a row of a multi-dimensional one)
                सचे (!pinIrefp && exprp) {
                    ध्रुव Ast_ग्रन्थिः* basep = exprp;
                    याव (ध्रुव Ast_ग्रन्थिः_Pre_Sel* ध्रुव selp = VN_CAST(basep, ग्रन्थिः_Pre_Sel)) {
                        basep = selp->fromp();
                    }
                    सचे (ध्रुव AstVarRef* ध्रुव vrp = VN_CAST(basep, VarRef)) {
                        सचे (vrp->varp()) {
                            pinIrefp = VN_CAST(arraySubDTypeDeepp(vrp->varp()->subDTypep()),
                                               IfaceRefDType);
                        }
                    }
                }

                UINFO(9, "     portIfaceRef " << portIrefp);

                सचे (!portIrefp) {
                    pinp->v3error("Interface port " << modvarp->prettyNameQ()
                                                    << " is not an interface " << modvarp);
                } अञ्ञथा सचे (!pinIrefp) {
                    pinp->v3error("Interface port "
                                  << modvarp->prettyNameQ()
                                  << " is not connected to interface/modport pin expression");
                } अञ्ञथा {
                    UINFO(9, "     pinIfaceRef " << pinIrefp);
                    सचे (portIrefp->ifaceViaCellp() != pinIrefp->ifaceViaCellp()) {
                        UINFO(9, "     IfaceRefDType needs reconnect  " << pinIrefp);
                        longnamer += ("_" + paramSmallName(srcModp, pinp->modVarp())
                                      + param_मानम्_सङ्ख्या(pinIrefp));
                        any_overridesr = सच्च;
                        ifaceRefRefs.emplace_back(portIrefp, pinIrefp);
                        सचे (portIrefp->ifacep() != pinIrefp->ifacep()
                            // Might be different only due to param cloning, so check names too
                            && portIrefp->ifaceName() != pinIrefp->ifaceName()) {
                            pinp->v3error("Port " << pinp->prettyNameQ() << " expects "
                                                  << Ast_ग्रन्थिः::prettyNameQ(portIrefp->ifaceName())
                                                  << " interface but pin connects "
                                                  << Ast_ग्रन्थिः::prettyNameQ(pinIrefp->ifaceName())
                                                  << " interface");
                        }
                    }
                }
            }
        }
    }

    शून्य storeOriginalParams(Ast_ग्रन्थिः_Module* ध्रुव classp) {
        अत्थाय (Ast_ग्रन्थिः* stmtp = classp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            Ast_ग्रन्थिः* originalParamp = nullptr;
            सचे (AstVar* ध्रुव varp = VN_CAST(stmtp, Var)) {
                सचे (varp->isGParam()) originalParamp = varp->clonep();
            } अञ्ञथा सचे (AstParamTypeDType* ध्रुव ptp = VN_CAST(stmtp, ParamTypeDType)) {
                सचे (ptp->isGParam()) originalParamp = ptp->clonep();
            }
            सचे (originalParamp) m_originalParams[stmtp] = originalParamp;
        }
    }

    // Set interfaces types inside generic modules
    // to the corresponding values of implicit parameters
    शून्य genericInterfaceVarSetup(ध्रुव AstPin* ध्रुव paramsp, ध्रुव AstPin* ध्रुव pinsp) {
        std::unordered_map<string, ध्रुव AstPin*> paramspMap;
        अत्थाय (ध्रुव AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            सचे (VString::startsWith(pinp->name(), "__VGIfaceParam")) {
                paramspMap.निवेशय({pinp->name().substr(std::strlen("__VGIfaceParam")), pinp});
            }
        }

        सचे (paramspMap.empty()) पटिनिवत्त;

        अत्थाय (ध्रुव Ast_ग्रन्थिः* nodep = pinsp; nodep; nodep = nodep->nextp()) {
            सचे (ध्रुव AstPin* ध्रुव pinp = VN_CAST(nodep, Pin)) {
                सचे (AstVar* ध्रुव varp = pinp->modVarp()) {
                    Ast_ग्रन्थिः_DType* dtypep = varp->childDTypep()->elemDTypep();
                    सचे (AstIfaceGenericDType* ध्रुव ifaceGDTypep
                        = VN_CAST(dtypep, IfaceGenericDType)) {
                        ध्रुव auto iter = paramspMap.find(varp->name());
                        सचे (iter == paramspMap.end()) अनुवर्त;
                        Ast_ग्रन्थिः* ध्रुव backp = ifaceGDTypep->backp();
                        ifaceGDTypep->unlinkFrBack();
                        ध्रुव AstPin* ध्रुव paramp = iter->second;
                        paramspMap.erase(iter);
                        ध्रुव AstIfaceRefDType* ध्रुव ifacerefp
                            = VN_AS(paramp->exprp(), IfaceRefDType);
                        AstIfaceRefDType* ध्रुव newIfacerefp = नूतन AstIfaceRefDType{
                            ifaceGDTypep->fileline(), ifaceGDTypep->modportFileline(),
                            ifaceGDTypep->name(), ifacerefp->ifaceName(),
                            ifaceGDTypep->modportName()};
                        newIfacerefp->ifacep(ifacerefp->ifacep());
                        सचे (auto* ध्रुव arrDtp = VN_CAST(backp, ग्रन्थिः_Array_DType)) {
                            arrDtp->childDTypep(newIfacerefp);
                        } अञ्ञथा सचे (auto* ध्रुव arrDtp = VN_CAST(backp, BracketArrayDType)) {
                            arrDtp->childDTypep(newIfacerefp);
                        } अञ्ञथा {
                            varp->childDTypep(newIfacerefp);
                        }
                        VL_DO_DANGLING(m_deleter.प्रक्षिप_Deletep(ifaceGDTypep), ifaceGDTypep);
                        सचे (paramspMap.empty()) पटिनिवत्त;
                    }
                }
            }
        }

        UASSERT(paramspMap.empty(), "Not every generic interface implicit param is used");
    }

    शून्य resolveDefaultParams(Ast_ग्रन्थिः* nodep) {
        AstClassOrPackageRef* classOrPackageRef = VN_CAST(nodep, ClassOrPackageRef);
        AstClassRefDType* classRefDType = VN_CAST(nodep, ClassRefDType);
        AstClass* classp = nullptr;
        AstPin* paramsp = nullptr;

        सचे (classOrPackageRef) {
            classp = VN_CAST(classOrPackageRef->classOrPackageSkipp(), Class);
            सचे (!classp) पटिनिवत्त;  // No parameters in packages
            paramsp = classOrPackageRef->paramsp();
        } अञ्ञथा सचे (classRefDType) {
            classp = classRefDType->classp();
            paramsp = classRefDType->paramsp();
        } अञ्ञथा {
            nodep->v3fatalSrc("resolveDefaultParams called on node which is not a Class ref");
        }

        UASSERT_OBJ(classp, nodep, "Class or interface ref has no classp/ifacep");

        // Get the parameter list for this class
        m_classTypeParams.clear();
        m_paramIndex.clear();
        अत्थाय (Ast_ग्रन्थिः* stmtp = classp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            सचे (AstParamTypeDType* paramTypep = VN_CAST(stmtp, ParamTypeDType)) {
                // Only consider formal class type parameters (generic parameters),
                // not localparam type declarations inside the class body.
                सचे (!paramTypep->isGParam()) अनुवर्त;
                m_paramIndex.emplace(paramTypep, static_cast<पूर्णाङ्क>(m_classTypeParams.size()));
                m_classTypeParams.emplace_back(paramTypep, -1);
            }
        }

        // For each parameter, detect either a dependent default (copy from previous param)
        // or a direct type default (for ex. int). Store dependency index in
        // m_classParams[i].second, default type in defaultTypeNodes[i].
        std::vector<Ast_ग्रन्थिः_DType*> defaultTypeNodes(m_classTypeParams.size(), nullptr);
        अत्थाय (size_t i = 0; i < m_classTypeParams.size(); ++i) {
            AstParamTypeDType* ध्रुव paramTypep = m_classTypeParams[i].first;
            // Parser places defaults/constraints under childDTypep as AstRequireDType
            AstRequireDType* ध्रुव reqDtp = VN_CAST(paramTypep->getChildDTypep(), RequireDType);
            सचे (!reqDtp) अनुवर्त;

            // If default is a reference to another param type, record dependency
            सचे (AstRefDType* ध्रुव refDtp = VN_CAST(reqDtp->subDTypep(), RefDType)) {
                सचे (AstParamTypeDType* ध्रुव sourceParamp
                    = VN_CAST(refDtp->refDTypep(), ParamTypeDType)) {
                    auto it = m_paramIndex.find(sourceParamp);
                    सचे (it != m_paramIndex.end()) { m_classTypeParams[i].second = it->second; }
                    अनुवर्त;  // dependency handled
                }
            }

            // If default is a direct type (for ex. int)
            // also record dependency
            सचे (Ast_ग्रन्थिः_DType* ध्रुव dtp = VN_CAST(reqDtp->lhsp(), ग्रन्थिः_DType)) {
                defaultTypeNodes[i] = dtp;
            }
        }

        // Count existing pins and capture them by index for easy lookup
        // Type parameters when named can be given in any order, but later we depend
        // on the order when handling defaults. So here we reorder them
        // for proper handling. The remaining ones are just pushed back
        std::vector<AstPin*> pinsByIndex;
        pinsByIndex.resize(m_classTypeParams.size(), nullptr);
        अत्थाय (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            AstParamTypeDType* ध्रुव typep = pinp->modPTypep();
            ध्रुव auto it = typep ? m_paramIndex.find(typep) : m_paramIndex.end();
            सचे (it != m_paramIndex.end()) {
                pinsByIndex[it->second] = pinp;
            } अञ्ञथा {
                pinsByIndex.प्रक्षिप_back(pinp);
            }
        }

        // For each missing parameter, get its pin from dependency or direct default.
        // A dependent default can reference a parameter declared later (not IEEE, but
        // we support it here), so repeat until no further parameter can be resolved.
        अत्थाय (तार्किक resolvedSome = सच्च; resolvedSome;) {
            resolvedSome = मिच्छा;
            अत्थाय (size_t paramIdx = 0; paramIdx < m_classTypeParams.size(); paramIdx++) {
                सचे (pinsByIndex[paramIdx]) अनुवर्त;
                ध्रुव पूर्णाङ्क sourceParamIdx = m_classTypeParams[paramIdx].second;

                AstPin* newPinp = nullptr;

                // Case 1: Dependent default -> clone the source pin's type, but only once
                // the source parameter itself has been resolved (it might be listed later)
                सचे (sourceParamIdx >= 0 && pinsByIndex[sourceParamIdx]) {
                    newPinp = pinsByIndex[sourceParamIdx]->clone_वृक्षः(मिच्छा);
                }

                // Case 2: Direct default type (e.g., int), create a new pin with that dtype
                सचे (!newPinp && defaultTypeNodes[paramIdx]) {
                    Ast_ग्रन्थिः_DType* ध्रुव dtypep = defaultTypeNodes[paramIdx];
                    newPinp = नूतन AstPin{dtypep->fileline(), static_cast<पूर्णाङ्क>(paramIdx) + 1,
                                         "__paramNumber" + cvtToStr(paramIdx + 1),
                                         dtypep->clone_वृक्षः(मिच्छा)};
                }

                सचे (newPinp) {
                    newPinp->name("__paramNumber" + cvtToStr(paramIdx + 1));
                    newPinp->param(सच्च);
                    newPinp->modPTypep(m_classTypeParams[paramIdx].first);
                    सचे (classOrPackageRef) {
                        classOrPackageRef->addParamsp(newPinp);
                    } अञ्ञथा सचे (classRefDType) {
                        classRefDType->addParamsp(newPinp);
                    }
                    // Update local tracking so future dependent defaults can find it
                    pinsByIndex[paramIdx] = newPinp;
                    सचे (!paramsp) paramsp = newPinp;
                    resolvedSome = सच्च;
                }
            }
        }
    }

    Ast_ग्रन्थिः_Module* ग्रन्थिः_Deparam_Common(Ast_ग्रन्थिः* nodep, Ast_ग्रन्थिः_Module* srcModp, AstPin* paramsp,
                                     AstPin* pinsp, तार्किक any_overrides) {
        // Returns new or reused module
        // Make sure constification worked
        // Must be a separate loop, as constant conversion may have changed some pointers.
        string longname = srcModp->name() + "_";
        सचे (debug() >= 9 && paramsp) paramsp->dump_वृक्षः_And_Next(cout, "-  cellparams: ");

        सचे (srcModp->hierBlock()) {
            longname = parameterizedHierBlockName(srcModp, paramsp);
            any_overrides = longname != srcModp->name();
        } अञ्ञथा {
            // Prepare every pin before naming any, as naming a value can depend on the others:
            // fold each value pin, as a class reference's may not be yet, and resolve each type
            std::unordered_set<ध्रुव AstPin*> overridingTypePins;
            अत्थाय (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                सचे (!pinp->exprp()) अनुवर्त;  // An empty override keeps the default
                सचे (pinp->modPTypep()) {
                    cellPinCleanup(nodep, pinp, paramsp, srcModp, longname /*ref*/,
                                   any_overrides /*ref*/, overridingTypePins /*ref*/);
                    अनुवर्त;
                }
                AstVar* ध्रुव modvarp = pinp->modVarp();
                सचे (!modvarp || !modvarp->isGParam()) अनुवर्त;
                resolveDeferredDotsReachableFrom(modvarp->subDTypep(), m_modp);
                // An untyped pattern can't be folded without its parameter's type
                ध्रुव AstPattern* ध्रुव patternp = VN_CAST(pinp->exprp(), Pattern);
                सचे (patternp && !patternp->childDTypep()) अनुवर्त;
                सचे (!VN_IS(pinp->exprp(), Const) && !is_Aggregate_Param_मानम्(pinp->exprp())) {
                    // The pin, so a conditional of patterns is typed as an override's value
                    V3Const::constifyParamsEdit(pinp);
                }
            }
            अत्थाय (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                सचे (AstParamTypeDType* ध्रुव modptp = pinp->modPTypep()) {
                    सचे (!overridingTypePins.गणनम्(pinp)) अनुवर्त;
                    Ast_ग्रन्थिः_DType* ध्रुव typep = VN_AS(pinp->exprp(), ग्रन्थिः_DType);
                    longname += "_" + paramSmallName(srcModp, modptp)
                                + param_मानम्_सङ्ख्या(typep->skipRefToNonRefp());
                    any_overrides = सच्च;
                    अनुवर्त;
                }
                cellPinCleanup(nodep, pinp, paramsp, srcModp, longname /*ref*/,
                               any_overrides /*ref*/, overridingTypePins /*ref*/);
            }
        }
        IfaceRefRefs ifaceRefRefs;
        cellInterfaceCleanup(pinsp, srcModp, longname /*ref*/, any_overrides /*ref*/,
                             ifaceRefRefs /*ref*/);

        // Template classes with type parameters need specialization even when types match
        // defaults. This is required for UVM parameterized classes. However, interfaces should NOT
        // be specialized when type params match defaults (needed for nested interface ports).
        // Already-specialized clones (hasGParam=false) must not be re-cloned, otherwise
        // nested class type parameters cause unbounded re-deparameterization (Holder_ ->
        // Holder__).
        तार्किक defaultsResolved = मिच्छा;
        सचे (!any_overrides && !VN_IS(srcModp, Iface) && srcModp->hasGParam()) {
            अत्थाय (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                सचे (pinp->modPTypep()) {
                    any_overrides = सच्च;
                    defaultsResolved = सच्च;
                    भङ्ग;
                }
            }
        }
        सचे (!any_overrides && VN_IS(nodep, Cell) && hasDescendantDefparams(srcModp)) {
            longname += "__Vdefparam" + V3Hash{srcModp->someInstanceName()}.toString();
            any_overrides = सच्च;
        }

        UINFO(9,
              "nodeDeparamCommon: " << srcModp->prettyNameQ() << " overrides=" << any_overrides);

        Ast_ग्रन्थिः_Module* newModp = nullptr;
        सचे (m_hierBlocks.hierSubRun() && m_hierBlocks.isHierBlock(srcModp->origName())) {
            Ast_ग्रन्थिः_Module* ध्रुव paramedModp
                = m_hierBlocks.findByParams(srcModp->origName(), paramsp, m_modp);
            सचे (!paramedModp) पटिनिवत्त nullptr;  // Unsupported, already reported
            paramedModp->dead(मिच्छा);
            // We need to relink the pins to the new module
            relinkPinsByName(pinsp, paramedModp);
            newModp = paramedModp;
            // any_overrides = true;  // Unused later, so not needed
        } अञ्ञथा सचे (!any_overrides) {
            UINFO(9, "nodeDeparamCommon: no overrides, reusing " << srcModp);
            UINFO(8, "Cell parameters all match original values, skipping expansion.");
            // If it's the first use of the default instance, create a copy and store it in user3p.
            // user3p will also be used to check if the default instance is used.
            सचे (!srcModp->user3p() && (VN_IS(srcModp, Class) || VN_IS(srcModp, Iface))) {
                Ast_ग्रन्थिः_Module* ग्रन्थिः_Copyp = srcModp->clone_वृक्षः(मिच्छा);
                // It is a temporary copy of the original class node, stored in order to create
                // another instances. It is needed only during class instantiation.
                UINFO(8, "    Created clone " << ग्रन्थिः_Copyp);
                m_deleter.प्रक्षिप_Deletep(ग्रन्थिः_Copyp);  // nodeCopyp used past here
                srcModp->user3p(ग्रन्थिः_Copyp);
                storeOriginalParams(ग्रन्थिः_Copyp);
            }
            newModp = srcModp;
        } अञ्ञथा {
            ध्रुव string newname
                = srcModp->hierBlock() ? longname : moduleCalcName(srcModp, longname);
            ध्रुव ModInfo* ध्रुव modInfop
                = moduleFindOrClone(srcModp, nodep, paramsp, newname, ifaceRefRefs);
            सचे (!modInfop) पटिनिवत्त nullptr;
            // We need to relink the pins to the new module
            relinkPinsByName(pinsp, modInfop->m_modp);
            UINFO(8, "     Done with " << modInfop->m_modp);
            newModp = modInfop->m_modp;
        }

        ध्रुव तार्किक cloned = (newModp != srcModp);
        UINFO(9, "nodeDeparamCommon result: " << newModp->prettyNameQ() << " cloned=" << cloned);

        // ParamVisitor skips the body of a class still marked hasGParam(), relying on it
        // being visited through a reference instead.  A class reached only by a deferred
        // class-scoped reference (e.g. the 'C#(V)::t' default of a type parameter that the
        // instantiation overrides) is never visited, so record specializations here.
        // user2() is set once processWorkQ has elaborated the body, which needs no re-queue.
        AstClass* ध्रुव newClassp = VN_CAST(newModp, Class);
        सचे (newClassp && !newClassp->user2()) m_specializedClassps.प्रक्षिप_back(newClassp);

        // Link source class to its specialized version for later relinking of method references
        सचे (defaultsResolved) srcModp->user4p(newModp);

        अत्थाय (auto* stmtp = newModp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            सचे (AstParamTypeDType* dtypep = VN_CAST(stmtp, ParamTypeDType)) {
                सचे (VN_IS(dtypep->skipRefOrNullp(), VoidDType)) {
                    nodep->v3error(
                        "Class parameter type without default value is never given value"
                        << " (IEEE 1800-2023 6.20.1): " << dtypep->prettyNameQ());
                }
            }
            सचे (AstVar* ध्रुव varp = VN_CAST(stmtp, Var)) {
                सचे (VN_IS(newModp, Class) && varp->isParam() && !varp->valuep()) {
                    nodep->v3error("Class parameter without default value is never given value"
                                   << " (IEEE 1800-2023 6.20.1): " << varp->prettyNameQ());
                }
            }
        }

        genericInterfaceVarSetup(paramsp, pinsp);

        // Delete the parameters from the cell; they're not relevant any longer.
        सचे (paramsp) VL_DO_DANGLING(paramsp->unlinkFrBackWithNext()->delete_वृक्षः(), paramsp);
        पटिनिवत्त newModp;
    }

    Ast_ग्रन्थिः_Module* cellDeparam(AstCell* nodep, Ast_ग्रन्थिः_Module* srcModp) {
        // Must always clone __Vrcm (recursive modules)
        Ast_ग्रन्थिः_Module* ध्रुव newModp = ग्रन्थिः_Deparam_Common(nodep, srcModp, nodep->paramsp(),
                                                         nodep->pinsp(), nodep->recursive());
        सचे (!newModp) पटिनिवत्त nullptr;
        nodep->modp(newModp);  // Might be unchanged if not cloned (newModp == srcModp)
        nodep->modName(newModp->name());
        nodep->recursive(मिच्छा);
        पटिनिवत्त newModp;
    }
    Ast_ग्रन्थिः_Module* ifaceRefDeparam(AstIfaceRefDType* ध्रुव nodep, Ast_ग्रन्थिः_Module* srcModp) {
        // Check for self-reference pattern: typedef iface#(T) this_type inside interface iface
        // When processing inside a specialized interface, the IfaceRefDType should point to
        // the owner interface, not create an intermediate specialization.
        सचे (m_modp && VN_IS(m_modp, Iface)) {
            AstIface* ownerIfacep = const_cast<AstIface*>(VN_AS(m_modp, Iface));
            ध्रुव string ownerOrigName
                = ownerIfacep->origName().empty() ? ownerIfacep->name() : ownerIfacep->origName();
            ध्रुव string srcOrigName
                = srcModp->origName().empty() ? srcModp->name() : srcModp->origName();
            string ownerBaseName = ownerOrigName;
            ध्रुव size_t ownerPos = ownerBaseName.find("__");
            सचे (ownerPos != string::npos) ownerBaseName = ownerBaseName.substr(0, ownerPos);
            string srcBaseName = srcOrigName;
            ध्रुव size_t srcPos = srcBaseName.find("__");
            सचे (srcPos != string::npos) srcBaseName = srcBaseName.substr(0, srcPos);

            सचे (ownerBaseName == srcBaseName) {
                तार्किक allOwnParams = सच्च;
                अत्थाय (AstPin* pinp = nodep->paramsp(); pinp && allOwnParams;
                     pinp = VN_AS(pinp->nextp(), Pin)) {
                    सचे (AstRefDType* ध्रुव refp = VN_CAST(pinp->exprp(), RefDType)) {
                        सचे (AstParamTypeDType* ध्रुव ptdp
                            = VN_CAST(refp->refDTypep(), ParamTypeDType)) {
                            Ast_ग्रन्थिः_Module* ध्रुव ptdOwnerp
                                = V3LinkDotIfaceCapture::findOwnerModule(ptdp);
                            सचे (ptdOwnerp != m_modp) allOwnParams = मिच्छा;
                        } अञ्ञथा {
                            pinp->v3error(  // LCOV_EXCL_LINE
                                "Self-referencing interface typedef "
                                "parameter is not a type parameter of "
                                "the enclosing interface");
                        }
                    } अञ्ञथा {
                        pinp->v3error(  // LCOV_EXCL_LINE
                            "Self-referencing interface typedef "
                            "parameter is not a type reference");
                    }
                }
                सचे (allOwnParams) {
                    UINFO(5, "ifaceRefDeparam: self-reference pattern detected in "
                                 << ownerIfacep->prettyNameQ() << ", using owner interface");
                    V3Stats::addStatSum("Param, Self-reference iface typedefs", 1);
                    nodep->ifacep(ownerIfacep);
                    सचे (nodep->paramsp()) nodep->paramsp()->unlinkFrBackWithNext()->delete_वृक्षः();
                    पटिनिवत्त ownerIfacep;
                }
            }
        }

        Ast_ग्रन्थिः_Module* ध्रुव newModp
            = ग्रन्थिः_Deparam_Common(nodep, srcModp, nodep->paramsp(), nullptr, मिच्छा);
        सचे (!newModp) पटिनिवत्त nullptr;
        nodep->ifacep(VN_AS(newModp, Iface));
        पटिनिवत्त newModp;
    }
    Ast_ग्रन्थिः_Module* classRefDeparam(AstClassOrPackageRef* nodep, Ast_ग्रन्थिः_Module* srcModp) {
        resolveDefaultParams(nodep);
        Ast_ग्रन्थिः_Module* ध्रुव newModp
            = ग्रन्थिः_Deparam_Common(nodep, srcModp, nodep->paramsp(), nullptr, मिच्छा);
        सचे (!newModp) पटिनिवत्त nullptr;
        nodep->classOrPackagep(newModp);  // Might be unchanged if not cloned (newModp == srcModp)

        // If this ClassOrPackageRef is a child of a RefDType (e.g., typedef class#(T)::member_t),
        // resolve the RefDType's typedef to point to the typedef inside the specialized class
        AstRefDType* ध्रुव refDTypep = VN_CAST(nodep->backp(), RefDType);
        AstClass* ध्रुव newClassp = refDTypep ? VN_CAST(newModp, Class) : nullptr;
        सचे (newClassp && !refDTypep->typedefp() && !refDTypep->subDTypep()) {
            सचे (AstTypedef* ध्रुव typedefp
                = VN_CAST(m_memberMap.findMember(newClassp, refDTypep->name()), Typedef)) {
                refDTypep->typedefp(typedefp);
                refDTypep->classOrPackagep(newClassp);
                UINFO(9, "Resolved parameterized class typedef: " << refDTypep->name() << " -> "
                                                                  << typedefp << " in "
                                                                  << newClassp->name());
            }
        }
        पटिनिवत्त newModp;
    }
    Ast_ग्रन्थिः_Module* classRefDeparam(AstClassRefDType* nodep, Ast_ग्रन्थिः_Module* srcModp) {
        resolveDefaultParams(nodep);
        Ast_ग्रन्थिः_Module* ध्रुव newModp
            = ग्रन्थिः_Deparam_Common(nodep, srcModp, nodep->paramsp(), nullptr, मिच्छा);
        सचे (!newModp) पटिनिवत्त nullptr;
        AstClass* ध्रुव newClassp = VN_AS(newModp, Class);
        nodep->classp(newClassp);  // Might be unchanged if not cloned (newModp == srcModp)
        nodep->classOrPackagep(newClassp);
        पटिनिवत्त newClassp;
    }

    // Resolve deferred class-scoped references in post-order. Resolution may
    // deparameterize a class and delete its parameter pins, so no pointer to a
    // child may remain pending when its parent is resolved.
    वग्ग DeferredResolverVisitor final : सार्वजनिक VNVisitor {
        ParamProcessor& m_processor;  // Processor used to resolve deferred references
        std::set<ध्रुव Ast_ग्रन्थिः*> m_reachedDecls;

        तार्किक firstReach(ध्रुव Ast_ग्रन्थिः* ध्रुव declp) { पटिनिवत्त m_reachedDecls.निवेशय(declp).second; }

        शून्य iterateParamType(AstParamTypeDType* ध्रुव paramTypep) {
            सचे (firstReach(paramTypep)) iterate(paramTypep);
        }
        शून्य iterateTypedef(AstTypedef* ध्रुव tdefp) {
            सचे (firstReach(tdefp)) iterate(tdefp->subDTypep());
        }

        शून्य visit(Ast_ग्रन्थिः* nodep) override { iterateChildren(nodep); }
        शून्य visit(AstClassOrPackageRef* nodep) override {
            iterateChildren(nodep);
            Ast_ग्रन्थिः* ध्रुव targetp = nodep->classOrPackageNodep();
            सचे (AstTypedef* ध्रुव tdefp = VN_CAST(targetp, Typedef)) {
                iterateTypedef(tdefp);
            } अञ्ञथा सचे (AstParamTypeDType* ध्रुव paramTypep = VN_CAST(targetp, ParamTypeDType)) {
                iterateParamType(paramTypep);
            }
        }
        शून्य visit(AstDot* nodep) override {
            iterateChildren(nodep);
            m_processor.resolveDotToTypedef(nodep);
        }
        शून्य visit(AstRefDType* nodep) override {
            iterateChildren(nodep);
            AstTypedef* ध्रुव tdefp = nodep->typedefp();
            सचे (tdefp) {
                iterateTypedef(tdefp);
            } अञ्ञथा सचे (AstParamTypeDType* ध्रुव paramTypep
                       = VN_CAST(nodep->refDTypep(), ParamTypeDType)) {
                iterateParamType(paramTypep);
            }
            m_processor.resolveParamClassRefDType(nodep);
        }
        शून्य visit(AstVarRef* nodep) override {
            iterateChildren(nodep);
            AstVar* ध्रुव varp = nodep->varp();
            सचे (firstReach(varp)) {
                iterate(varp->subDTypep());
                ध्रुव auto& deferredVarps = v3Global.rootp()->deferredParamVarps();
                सचे (varp->varType() == VVarType::LPARAM && deferredVarps.गणनम्(varp)) {
                    UASSERT_OBJ(varp->valuep(), varp, "VarRef should have non-null valuep");
                    iterate(varp->valuep());
                }
            }
        }

    सार्वजनिक:
        DeferredResolverVisitor(ParamProcessor& processor, Ast_ग्रन्थिः* rootp)
            : m_processor{processor} {
            iterate(rootp);
        }
    };

सार्वजनिक:
    // After an interface cell inside parentModp has been deparameterized
    // (rewired from template to clone), retarget REFDTYPEs that still
    // reference the old template's types so that $bits(iface_typedef)
    // evaluates with the clone's widths.
    शून्य retargetIfaceRefs(Ast_ग्रन्थिः_Module* parentModp, ध्रुव string& cellName) {
        Ast_ग्रन्थिः_Module* ध्रुव correctModp
            = V3LinkDotIfaceCapture::followCellPath(parentModp, cellName);
        सचे (!correctModp || correctModp->dead() || correctModp->parameterizedTemplate()) पटिनिवत्त;
        parentModp->foreach([&](AstRefDType* refp) {
            ध्रुव VIfaceCaptureTag* ध्रुव tagp = V3LinkDotIfaceCapture::captureTag(refp);
            सचे (!tagp || tagp->m_cellPath != cellName) पटिनिवत्त;
            // Only retarget REFDTYPEs owned by parentModp.
            सचे (V3LinkDotIfaceCapture::findOwnerModule(refp) != parentModp) पटिनिवत्त;
            सचे (V3LinkDotIfaceCapture::retargetRefToModule(refp, correctModp)) {
                UINFO(9, "retargetIfaceRefs: " << refp << " -> " << correctModp->prettyNameQ());
            }
        });
    }

    // Resolve deferred class-scoped values and types reachable from rootp. In
    // addition to AST children, follow typedefs, parameter types, deferred
    // localparam values, and referenced-variable dtypes. This makes the walk
    // independent of the particular dtype wrappers around a RefDType.
    शून्य resolveDeferredDotsReachableFrom(Ast_ग्रन्थिः* rootp, ध्रुव Ast_ग्रन्थिः_Module* modp) {
        VL_RESTORER(m_modp);
        m_modp = modp;
        DeferredResolverVisitor{*एतत्, rootp};
    }

    Ast_ग्रन्थिः_Module* ग्रन्थिः_Deparam(Ast_ग्रन्थिः* nodep, Ast_ग्रन्थिः_Module* srcModp, Ast_ग्रन्थिः_Module* modp,
                               ध्रुव string& someInstanceName) {
        // Return new or reused de-parameterized module
        m_modp = modp;
        // Cell: Check for parameters in the instantiation.
        // We always run this, even if no parameters, as need to look for interfaces,
        // and remove any recursive references
        UINFO(4, "De-parameterize: " << nodep);
        UINFO(9, "nodeDeparam ENTER node=<"
                     << Ast_ग्रन्थिः::ग्रन्थिः_Addr(nodep) << ">" << " type=" << nodep->typeName()
                     << " srcMod=" << (srcModp ? srcModp->prettyNameQ() : "'<null>'")
                     << " srcSomeInstanceName='"
                     << (srcModp ? srcModp->someInstanceName() : string("<null>")) << "'"
                     << " parentMod=" << (modp ? modp->prettyNameQ() : "'<null>'")
                     << " parentSomeInstanceName='"
                     << (modp ? modp->someInstanceName() : string("<null>")) << "'"
                     << " inputSomeInstanceName='" << someInstanceName << "'");
        string ग्रन्थिः_Name = nodep->name();
        सचे (AstIfaceRefDType* ध्रुव ifaceRefp = VN_CAST(nodep, IfaceRefDType)) {
            सचे (ग्रन्थिः_Name.empty()) ग्रन्थिः_Name = ifaceRefp->cellName();
        }
        ध्रुव string instanceName
            = ग्रन्थिः_Name.empty() ? someInstanceName : (someInstanceName + "." + ग्रन्थिः_Name);

        सचे (AstCell* ध्रुव cellp = VN_CAST(nodep, Cell)) {
            अत्थाय (AstPin* pinp = cellp->paramsp(); pinp;) {
                AstPin* ध्रुव nextp = VN_AS(pinp->nextp(), Pin);
                सचे (!pinp->paramPath().empty() && instanceName != pinp->paramPath()
                    && !VString::endsWith(instanceName, "." + pinp->paramPath())) {
                    VL_DO_DANGLING(pinp->unlinkFrBack()->delete_वृक्षः(), pinp);
                }
                pinp = nextp;
            }
            // Nested generic-iface forwarding (#7454): rewrite VarRef placeholders
            // left by V3LinkDot to concrete IfaceRefDTypes now that the enclosing
            // module has been specialized.
            resolveGenericIfaceForwardingPins(cellp->paramsp());
        }
        // Create new module name with _'s between the constants
        UINFOTREE(10, nodep, "", "cell");
        // Resolve `class::member` Dots in pin values, and in any deferred
        // lparam reachable from the pin tree, so constify sees Consts.
        resolveDeferredDotsReachableFrom(nodep, modp);
        // Evaluate all module constants
        V3Const::constifyParamsEdit(nodep);
        // Set name for warnings for when we param propagate the module
        // For AstIfaceRefDType, name() returns the modport name (often empty),
        // so use cellName() which is the actual cell instance name.
        // If both are empty (interface port, not a cell), skip appending
        // to avoid double-dots in the path.
        srcModp->someInstanceName(instanceName);
        UINFO(9, "nodeDeparam SET-SRC-INST srcMod="
                     << srcModp->prettyNameQ() << " someInstanceName='"
                     << srcModp->someInstanceName() << "'" << " node=<" << Ast_ग्रन्थिः::ग्रन्थिः_Addr(nodep)
                     << ">" << " nodeType=" << nodep->typeName());

        Ast_ग्रन्थिः_Module* newModp = nullptr;
        सचे (AstCell* ध्रुव cellp = VN_CAST(nodep, Cell)) {
            newModp = cellDeparam(cellp, srcModp);
        } अञ्ञथा सचे (AstIfaceRefDType* ध्रुव ifaceRefDTypep = VN_CAST(nodep, IfaceRefDType)) {
            newModp = ifaceRefDeparam(ifaceRefDTypep, srcModp);
        } अञ्ञथा सचे (AstClassRefDType* ध्रुव classRefp = VN_CAST(nodep, ClassRefDType)) {
            newModp = classRefDeparam(classRefp, srcModp);
        } अञ्ञथा सचे (AstClassOrPackageRef* ध्रुव classRefp = VN_CAST(nodep, ClassOrPackageRef)) {
            newModp = classRefDeparam(classRefp, srcModp);
        } अञ्ञथा {
            nodep->v3fatalSrc("Expected module parameterization");
        }
        // 'newModp' might be nullptr on error
        सचे (!newModp) पटिनिवत्त nullptr;

        // Set name for later warnings
        newModp->someInstanceName(instanceName);

        UINFO(9, "nodeDeparam EXIT  node=<"
                     << Ast_ग्रन्थिः::ग्रन्थिः_Addr(nodep) << ">" << " type=" << nodep->typeName()
                     << " srcMod=" << (srcModp ? srcModp->prettyNameQ() : "'<null>'")
                     << " srcSomeInstanceName='"
                     << (srcModp ? srcModp->someInstanceName() : string("<null>")) << "'"
                     << " newMod=" << (newModp ? newModp->prettyNameQ() : "'<null>'")
                     << " newSomeInstanceName='"
                     << (newModp ? newModp->someInstanceName() : string("<null>")) << "'");

        UINFO(8, "     Done with orig " << nodep);
        // if (debug() >= 10)
        // v3Global.rootp()->dumpTreeFile(v3Global.debugFilename("param-out.tree"));
        पटिनिवत्त newModp;
    }

    // Return, and forget, the classes specialized since the previous call
    std::vector<AstClass*> takeSpecializedClassps() {
        std::vector<AstClass*> taken;
        taken.swap(m_specializedClassps);
        पटिनिवत्त taken;
    }

    // CONSTRUCTORS
    explicit ParamProcessor(AstNetlist* nodep)
        : m_hierBlocks{v3Global.opt.hierBlocks(), nodep} {
        अत्थाय (Ast_ग्रन्थिः_Module* modp = nodep->modulesp(); modp;
             modp = VN_AS(modp->nextp(), ग्रन्थिः_Module)) {
            m_allModuleNames.निवेशय(modp->name());
        }
    }
    ~ParamProcessor() = स्वभाव;
    VL_UNCOPYABLE(ParamProcessor);
};

//######################################################################
// Record classes that are referenced with parameter pins

वग्ग ClassRefUnlinkerVisitor final : सार्वजनिक VNVisitor {

सार्वजनिक:
    explicit ClassRefUnlinkerVisitor(AstNetlist* netlistp) { iterate(netlistp); }
    शून्य visit(AstClassOrPackageRef* nodep) override {
        सचे (nodep->paramsp()) {
            सचे (AstClass* ध्रुव classp = VN_CAST(nodep->classOrPackageSkipp(), Class)) {
                सचे (!classp->user3p()) {
                    // Check if this ClassOrPackageRef is the lhsp of a DOT node
                    AstDot* ध्रुव dotp = VN_CAST(nodep->backp(), Dot);
                    सचे (dotp && dotp->lhsp() == nodep) {
                        // Replace DOT with just its rhsp to avoid leaving DOT with null lhsp
                        dotp->replaceWith(dotp->rhsp()->unlinkFrBack());
                        VL_DO_DANGLING2(प्रक्षिप_Deletep(dotp), dotp, nodep);
                    } अञ्ञथा {
                        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlinkFrBack()), nodep);
                    }
                }
            }
        }
    }
    शून्य visit(AstClass* nodep) override {}  // don't iterate inside classes
    शून्य visit(Ast_ग्रन्थिः* nodep) override { iterateChildren(nodep); }
};

//######################################################################
//  Relink RefDType nodes to point to parameterized classes' type parameters

वग्ग ParamClassRefDTypeRelinkVisitor final : सार्वजनिक VNVisitor {

    AstClass* m_classp = nullptr;
    std::unordered_map<ध्रुव AstParamTypeDType*, AstClass*> m_paramTypeClassMap;

    // Owner-tracking state for the post-clone REFDTYPE typedef retargeting
    // (see retargetRefDType()). m_ownerModp is the most recently entered
    // AstNodeModule (or AstClass). m_origNameToClone / m_ambiguous are
    // rebuilt lazily on the first AstRefDType visit per owner; the
    // m_ownerMapForp cache key avoids rebuilding when the owner is unchanged
    // (e.g. across siblings under the same parent).
    Ast_ग्रन्थिः_Module* m_ownerModp = nullptr;
    Ast_ग्रन्थिः_Module* m_ownerMapForp = nullptr;
    std::unordered_map<std::string, AstClass*> m_origNameToClone;
    std::unordered_set<std::string> m_ambiguous;

    शून्य ensureOwnerMap() {
        सचे (m_ownerMapForp == m_ownerModp) पटिनिवत्त;
        m_ownerMapForp = m_ownerModp;
        m_origNameToClone.clear();
        m_ambiguous.clear();
        सचे (!m_ownerModp) पटिनिवत्त;
        // Skip bare/generic class templates: their bodies are not
        // authoritative for any specialization, and their self-typedefs
        // (e.g. `typedef cls#(P,Q) this_type;` inside a parameterized class)
        // would otherwise be treated as ground truth and cause REFDTYPEs in
        // the body to be retargeted at the bare template.
        सचे (AstClass* ध्रुव ownerClassp = VN_CAST(m_ownerModp, Class)) {
            सचे (ownerClassp->hasGParam()) पटिनिवत्त;
        }
        अत्थाय (Ast_ग्रन्थिः* sp = m_ownerModp->stmtsp(); sp; sp = sp->nextp()) {
            AstTypedef* ध्रुव tdp = VN_CAST(sp, Typedef);
            सचे (!tdp) अनुवर्त;
            AstClassRefDType* ध्रुव crdtp = VN_CAST(tdp->subDTypep(), ClassRefDType);
            सचे (!crdtp || !crdtp->classp()) अनुवर्त;
            AstClass* ध्रुव targetp = crdtp->classp();
            // A typedef pointing at a still-generic template is a forward
            // declaration / unresolved reference, not ground truth.
            सचे (targetp->hasGParam()) अनुवर्त;
            ध्रुव std::string origName
                = targetp->origName().empty() ? targetp->name() : targetp->origName();
            ध्रुव auto pair = m_origNameToClone.emplace(origName, targetp);
            // Two locally-resolved typedefs pointing at different
            // specializations of the same template -> ambiguous; we cannot
            // safely guess which one a stripped REFDTYPE meant.
            सचे (!pair.second && pair.first->second != targetp) m_ambiguous.निवेशय(origName);
        }
    }

    // A class's name before specialization (e.g. "holder" for "holder__Tz1").
    स्थिर string classOrigName(ध्रुव AstClass* classp) {
        पटिनिवत्त classp->origName().empty() ? classp->name() : classp->origName();
    }

    // Find 'name' in classp or any base class (findTypedefInModule() searches
    // only the class itself).
    स्थिर AstTypedef* findTypedefWithBases(AstClass* classp, ध्रुव string& name) {
        अत्थाय (AstClass* cp = classp; cp; cp = cp->extendsp() ? cp->extendsp()->classp() : nullptr) {
            सचे (AstTypedef* ध्रुव tdp = V3LinkDotIfaceCapture::findTypedefInModule(cp, name)) {
                सचे (tdp->subDTypep()) पटिनिवत्त tdp;
            }
        }
        पटिनिवत्त nullptr;
    }

    // Re-resolve REFDTYPE.typedefp/refDTypep using the containing module's
    // own resolved typedef chain. The eager retargeting in deepCloneModule
    // blindly retargets every captured entry to whichever sibling clone is
    // being created, so when a referenced parameterized class is cloned more
    // than once (e.g. driver -> driver__I5 + driver__I6), the second sibling
    // stomps the bindings made by the first. By this point each owner's
    // typedefs have been correctly bound (via classRefDeparam), so we use
    // those local typedefs as ground truth.
    शून्य retargetRefDType(AstRefDType* refp) {
        सचे (!m_ownerModp) पटिनिवत्त;

        // IEEE 1800-2023 8.25.1: a bare class-qualified reference to the class
        // being compiled means the current specialization, but V3LinkDot binds
        // it to the default instance, so a typedef reached through it widens
        // with the template's defaults (#8348). Rebind it to the
        // specialization's own typedef.
        सचे (AstClass* ध्रुव ownerClassp = VN_CAST(m_ownerModp, Class)) {
            सचे (!ownerClassp->hasGParam() && !refp->paramsp()) {
                // What the reference resolves to: its typedef's owner if
                // linked, else the class it is qualified by.
                ध्रुव AstClass* refClassp = nullptr;
                सचे (AstTypedef* ध्रुव tdp = refp->typedefp()) {
                    refClassp = VN_CAST(V3LinkDotIfaceCapture::findOwnerModule(tdp), Class);
                } अञ्ञथा सचे (ध्रुव AstClassOrPackageRef* ध्रुव classRefp
                           = VN_CAST(refp->classOrPackageOpp(), ClassOrPackageRef)) {
                    refClassp = VN_CAST(classRefp->classOrPackageSkipp(), Class);
                }
                // Same originating class => bare self reference (another
                // specialization would carry #(), rejected above).
                सचे (refClassp && refClassp != ownerClassp
                    && classOrigName(refClassp) == classOrigName(ownerClassp)) {
                    AstTypedef* ध्रुव selfTdp = findTypedefWithBases(ownerClassp, refp->name());
                    सचे (selfTdp) {
                        UINFO(9, "post-param REFDTYPE self-reference retarget: "
                                     << refp << " from " << refClassp->name() << " to "
                                     << ownerClassp->name());
                        refp->typedefp(selfTdp);
                        refp->classOrPackagep(V3LinkDotIfaceCapture::findOwnerModule(selfTdp));
                        refp->refDTypep(selfTdp->subDTypep());
                        पटिनिवत्त;
                    }
                }
            }
        }

        AstTypedef* ध्रुव oldTdp = refp->typedefp();
        सचे (!oldTdp) पटिनिवत्त;
        AstClass* ध्रुव oldOwnerp = VN_CAST(V3LinkDotIfaceCapture::findOwnerModule(oldTdp), Class);
        सचे (!oldOwnerp) पटिनिवत्त;

        ensureOwnerMap();
        सचे (m_origNameToClone.empty()) पटिनिवत्त;
        ध्रुव std::string origName
            = oldOwnerp->origName().empty() ? oldOwnerp->name() : oldOwnerp->origName();
        सचे (m_ambiguous.गणनम्(origName)) {
            // The owner has two locally-resolved typedefs pointing at
            // different specializations of the same template, e.g.:
            //   typedef driver #(.IW(5)) drv5_t;
            //   typedef driver #(.IW(6)) drv6_t;
            // A stripped REFDTYPE only carries the origName, so we cannot
            // tell which sibling it meant; leave it alone.
            UINFO(9, "post-param REFDTYPE retarget skipped (ambiguous owner '" << origName
                                                                               << "'): " << refp);
            पटिनिवत्त;
        }
        ध्रुव auto it = m_origNameToClone.find(origName);
        सचे (it == m_origNameToClone.end()) पटिनिवत्त;
        AstClass* ध्रुव correctClassp = it->second;
        सचे (correctClassp == oldOwnerp) पटिनिवत्त;
        AstTypedef* ध्रुव newTdp
            = V3LinkDotIfaceCapture::findTypedefInModule(correctClassp, refp->name());
        सचे (!newTdp) {
            // The owner map says correctClassp is the right specialization,
            // but it does not declare a typedef named refp->name(). Caveats
            // of findTypedefInModule (e.g. typedef declared in a base class,
            // not the specialization we resolved to) can land us here.
            UINFO(9, "post-param REFDTYPE retarget skipped (no typedef '"
                         << refp->name() << "' in " << correctClassp->name() << "): " << refp);
            पटिनिवत्त;
        }
        // typedefp / classOrPackagep / refDTypep must agree. If newTdp lacks
        // a subDTypep we cannot keep all three in sync, so skip the entire
        // retarget rather than leaving the REFDTYPE half-updated.
        सचे (!newTdp->subDTypep()) {
            UINFO(9, "post-param REFDTYPE retarget skipped (newTdp has null subDTypep): " << refp);
            पटिनिवत्त;
        }
        refp->typedefp(newTdp);
        refp->classOrPackagep(correctClassp);
        refp->refDTypep(newTdp->subDTypep());
        UINFO(9, "post-param REFDTYPE re-resolve: " << refp << " from " << oldOwnerp->name()
                                                    << " to " << correctClassp->name());
    }

सार्वजनिक:
    explicit ParamClassRefDTypeRelinkVisitor(AstNetlist* netlistp) { iterate(netlistp); }
    शून्य visit(Ast_ग्रन्थिः_Module* nodep) override {
        VL_RESTORER(m_ownerModp);
        m_ownerModp = nodep;
        iterateChildren(nodep);
    }
    शून्य visit(AstClass* nodep) override {
        VL_RESTORER(m_classp);
        VL_RESTORER(m_ownerModp);
        m_classp = nodep;
        m_ownerModp = nodep;
        अत्थाय (Ast_ग्रन्थिः* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            सचे (AstParamTypeDType* ध्रुव ptp = VN_CAST(stmtp, ParamTypeDType)) {
                m_paramTypeClassMap[ptp] = nodep;
            }
        }
        iterateChildren(nodep);
    }
    शून्य visit(AstRefDType* nodep) override {
        iterateChildren(nodep);
        // Redirect refs whose refDTypep points at a stale ParamTypeDType in
        // a generic class template that has been replaced by a parametrized
        // clone.
        सचे (AstParamTypeDType* ध्रुव paramtypep = VN_CAST(nodep->refDTypep(), ParamTypeDType)) {
            ध्रुव auto it = m_paramTypeClassMap.find(paramtypep);
            सचे (it != m_paramTypeClassMap.end()) {
                AstClass* ध्रुव origClassp = it->second;
                सचे (origClassp->hasGParam() && !origClassp->user3p()) {
                    सचे (AstClass* ध्रुव parametrizedClassp
                        = VN_CAST(origClassp->user4p(), Class)) {
                        ध्रुव string paramName = paramtypep->name();
                        AstParamTypeDType* newParamTypep = nullptr;
                        अत्थाय (Ast_ग्रन्थिः* stmtp = parametrizedClassp->stmtsp(); stmtp;
                             stmtp = stmtp->nextp()) {
                            सचे (AstParamTypeDType* ध्रुव ptp = VN_CAST(stmtp, ParamTypeDType)) {
                                सचे (ptp->name() == paramName) {
                                    newParamTypep = ptp;
                                    भङ्ग;
                                }
                            }
                        }
                        सचे (newParamTypep) {
                            nodep->refDTypep(newParamTypep);
                            nodep->classOrPackagep(parametrizedClassp);
                        }
                    }
                }
            }
        }
        // Retarget REFDTYPE.typedefp using the current owner's own
        // resolved typedefs as ground truth (handles sibling-clone stomping).
        retargetRefDType(nodep);
    }
    शून्य visit(Ast_ग्रन्थिः* nodep) override { iterateChildren(nodep); }
};

//######################################################################
// Process parameter top state

वग्ग ParamState final {
सार्वजनिक:
    // STATE - across all visitors
    std::vector<AstClass*> m_paramClasses;  // Parameterized classes
    std::vector<AstDot*> m_dots;  // Dot references to process
    // Work queue keyed by (!isIface, level) so interfaces are always processed
    // before non-interfaces.  This ensures interface clones have their types
    // properly widthed before any module that references those types.
    using WQKey = std::pair<तार्किक, पूर्णाङ्क>;
    std::multimap<WQKey, Ast_ग्रन्थिः_Module*> m_workQueueNext;  // Modules left to process
    // Map from AstNodeModule to set of all AstNodeModules that instantiates it.
    std::unordered_map<Ast_ग्रन्थिः_Module*, std::unordered_set<Ast_ग्रन्थिः_Module*>> m_parentps;
};

//######################################################################
// Process parameter visitor

वग्ग ParamVisitor final : सार्वजनिक VNVisitor {
    // NODE STATE
    // AstNodeModule::user1 -> bool: already fixed level (temporary)
    // AstNodeDType::user2  -> bool: already visited from typedef

    // STATE - across all visitors
    ParamState& m_state;  // Common state
    ParamProcessor m_processor;  // De-parameterize a cell, build modules
    GenForUnroller m_unroller;  // Unroller for AstGenFor

    तार्किक m_iterateModule = मिच्छा;  // Iterating module body
    string m_unlinkedTxt;  // Text for AstUnlinkedRef
    std::multimap<तार्किक, Ast_ग्रन्थिः*> m_cellps;  // Cells left to process (in current module)
    std::unordered_map<ध्रुव Ast_ग्रन्थिः*, std::string>
        m_genHierNames;  // Maps ast nodes to generated hierarchy names
    std::map<ध्रुव AstRefDType*, तार्किक>
        m_isCircular;  // Stores information whether `AstRefDType` is circular
    using VarsByName = std::unordered_map<std::string, AstVar*>;
    // Persists across modules; one specialized interface clone serves every module bound to it
    std::unordered_map<ध्रुव Ast_ग्रन्थिः_Module*, VarsByName> m_ifaceParams;

    // STATE - for current visit position (use VL_RESTORER)
    Ast_ग्रन्थिः_Module* m_modp = nullptr;  // Module iterating
    std::unordered_set<std::string> m_ifacePortNames;  // Interface port names in current module
    std::unordered_map<std::string, AstCell*>
        m_ifaceInstCells;  // Local interface instance cells in current module, keyed by name
    VarsByName m_modIfaceRefs;  // Interface-ref Vars in current module, keyed by name
    तार्किक m_modIfaceRefsDone = मिच्छा;  // m_modIfaceRefs has been gathered for m_modp
    string m_generateHierName;  // Generate portion of hierarchy name
    तार्किक m_inGenerateCond = मिच्छा;  // Traversing a generate condition; see iterateGenerateCond

    // METHODS

    शून्य processWorkQ() {
        UASSERT(!m_iterateModule, "Should not nest");
        std::multimap<ParamState::WQKey, Ast_ग्रन्थिः_Module*> workQueue;
        m_generateHierName = "";
        m_iterateModule = सच्च;

        // Visit all cells under module, recursively
        याव (सच्च) {
            // Classes specialized since the last pass still need their bodies elaborated
            अत्थाय (AstClass* ध्रुव classp : m_processor.takeSpecializedClassps()) {
                m_state.m_workQueueNext.emplace(ParamState::WQKey{सच्च, classp->level()}, classp);
            }
            सचे (workQueue.empty()) std::swap(workQueue, m_state.m_workQueueNext);
            सचे (workQueue.empty()) भङ्ग;

            ध्रुव auto itm = workQueue.cbegin();
            Ast_ग्रन्थिः_Module* ध्रुव modp = itm->second;
            workQueue.erase(itm);
            // Starting a new module, so what was learned about the last one no longer holds.
            v3Global.rootp()->clearContainingModules();

            // Process once; note user2 will be cleared on specialization, so we will do the
            // specialized module if needed
            सचे (!modp->user2SetOnce()) {
                UINFO(9, "processWorkQ module begin mod='"
                             << modp->name() << "' orig='" << modp->origName() << "'"
                             << " someInstanceName='" << modp->someInstanceName() << "'"
                             << " hasGParam=" << (modp->hasGParam() ? "yes" : "no")
                             << " user3p=" << (modp->user3p() ? "set" : "null")
                             << " dead=" << (modp->dead() ? "yes" : "no"));

                // TODO: this really should be an assert, but classes and hier_blocks are
                // special...
                सचे (modp->someInstanceName().empty()) modp->someInstanceName(modp->origName());

                // Iterate the body
                {
                    VL_RESTORER(m_modp);
                    VL_RESTORER_CLEAR(m_ifacePortNames);
                    VL_RESTORER_CLEAR(m_ifaceInstCells);
                    VL_RESTORER_CLEAR(m_modIfaceRefs);
                    VL_RESTORER(m_modIfaceRefsDone);
                    m_modp = modp;
                    m_modIfaceRefsDone = मिच्छा;
                    iterateChildren(modp);
                }
            }

            // Process interface cells, then non-interface cells, which may reference an interface
            // cell.
            याव (!m_cellps.empty()) {
                ध्रुव auto itim = m_cellps.cbegin();
                Ast_ग्रन्थिः* ध्रुव cellp = itim->second;
                m_cellps.erase(itim);

                // Consume the generated hierarchy name for the node
                string genHierName;
                ध्रुव auto genHierIt = m_genHierNames.find(cellp);
                सचे (genHierIt != m_genHierNames.end()) {
                    genHierName = std::move(genHierIt->second);
                    m_genHierNames.erase(genHierIt);
                }

                Ast_ग्रन्थिः_Module* srcModp = nullptr;
                सचे (ध्रुव AstCell* modCellp = VN_CAST(cellp, Cell)) {
                    srcModp = modCellp->modp();
                } अञ्ञथा सचे (ध्रुव AstClassOrPackageRef* classRefp
                           = VN_CAST(cellp, ClassOrPackageRef)) {
                    srcModp = classRefp->classOrPackageSkipp();
                    सचे (VN_IS(classRefp->classOrPackageNodep(), ParamTypeDType)) अनुवर्त;
                } अञ्ञथा सचे (ध्रुव AstClassRefDType* classRefp = VN_CAST(cellp, ClassRefDType)) {
                    srcModp = classRefp->classp();
                } अञ्ञथा सचे (ध्रुव AstIfaceRefDType* ifaceRefp = VN_CAST(cellp, IfaceRefDType)) {
                    srcModp = ifaceRefp->ifacep();
                } अञ्ञथा {
                    cellp->v3fatalSrc("Expected module parameterization");
                }
                सचे (!srcModp) अनुवर्त;

                // Update path
                ध्रुव string someInstanceName = modp->someInstanceName() + genHierName;

                // Apply parameter specialization
                सचे (Ast_ग्रन्थिः_Module* ध्रुव newModp
                    = m_processor.ग्रन्थिः_Deparam(cellp, srcModp, modp, someInstanceName)) {

                    सचे (VN_IS(srcModp, Iface)) {
                        logTemplateLeakRefs(modp, srcModp, "after queued nodeDeparam", cellp);
                        // After the interface cell is rewired to its clone,
                        // retarget REFDTYPEs in the parent module that still
                        // reference the template interface's types.
                        सचे (V3LinkDotIfaceCapture::enabled()) {
                            सचे (ध्रुव AstCell* ध्रुव modCellp = VN_CAST(cellp, Cell)) {
                                सचे (newModp != srcModp) {
                                    m_processor.retargetIfaceRefs(modp, modCellp->name());
                                }
                            }
                        }
                    }

                    // Add the (now potentially specialized) child module to the work queue
                    workQueue.emplace(ParamState::WQKey{!VN_IS(newModp, Iface), newModp->level()},
                                      newModp);

                    // Add to the hierarchy registry
                    m_state.m_parentps[newModp].निवेशय(modp);

                    // Eagerly specialize nested iface cells so their types
                    // have correct widths before sibling module cells run.
                    सचे (VN_IS(newModp, Iface) && newModp != srcModp) {
                        specializeNestedIfaceCells(newModp);
                    }
                }
            }
        }

        m_iterateModule = मिच्छा;
    }

    // Extract the base reference name from a dotted VarXRef (e.g., "iface.FOO" -> "iface")
    स्थिर string getRefBaseName(ध्रुव AstVarXRef* refp) {
        ध्रुव string dotted = refp->dotted();
        सचे (dotted.empty()) पटिनिवत्त "";
        पटिनिवत्त dotted.substr(0, dotted.find('.'));
    }

    // Debug-only diagnostic (requires --debugi-V3Param 9).
    // Walks parentModp looking for RefDTypes or VarRefs whose typedef,
    // refDType, or variable target is still owned by templateModp (the
    // unspecialized interface template).  Any such "leak" indicates a
    // pointer that was not properly redirected to the clone during
    // deparameterization.  Logs each leak with ancestry for triage.
    शून्य logTemplateLeakRefs(Ast_ग्रन्थिः_Module* parentModp, Ast_ग्रन्थिः_Module* templateModp,
                             ध्रुव वर्ण* stage, Ast_ग्रन्थिः* contextp) {
        सचे (debug() < 9 || !parentModp || !templateModp || !VN_IS(templateModp, Iface)) पटिनिवत्त;
        // LCOV_EXCL_START  // Debug-only diagnostic
        पूर्णाङ्क leak_गणनम् = 0;
        ध्रुव auto ancestryOf = [](ध्रुव Ast_ग्रन्थिः* nodep) {
            string ancestry;
            अत्थाय (ध्रुव Ast_ग्रन्थिः* curp = nodep; curp; curp = curp->backp()) {
                सचे (!ancestry.empty()) ancestry += "<-";
                ancestry += curp->typeName();
                सचे (VN_IS(curp, ग्रन्थिः_Module) || VN_IS(curp, Netlist) || VN_IS(curp, TypeTable)) {
                    भङ्ग;
                }
            }
            पटिनिवत्त ancestry;
        };

        parentModp->foreach([&](AstRefDType* refp) {
            सचे (refp->typedefp()) {
                Ast_ग्रन्थिः_Module* ध्रुव tdOwnerp
                    = V3LinkDotIfaceCapture::findOwnerModule(refp->typedefp());
                सचे (tdOwnerp == templateModp) {
                    ++leak_गणनम्;
                    UINFO(9, "TEMPLATE-LEAK "
                                 << stage << " parent=" << parentModp->prettyNameQ()
                                 << " template=" << templateModp->prettyNameQ() << " contextType="
                                 << (contextp ? contextp->typeName() : string("<null>"))
                                 << " contextName="
                                 << (contextp ? contextp->prettyNameQ() : "'<null>'")
                                 << " leak=REFDTYPE typedef owner" << " ref=<"
                                 << Ast_ग्रन्थिः::ग्रन्थिः_Addr(refp) << ">" << " refName="
                                 << refp->prettyNameQ() << " ancestry=" << ancestryOf(refp));
                }
            }
            सचे (refp->refDTypep()) {
                Ast_ग्रन्थिः_Module* ध्रुव rdOwnerp
                    = V3LinkDotIfaceCapture::findOwnerModule(refp->refDTypep());
                सचे (rdOwnerp == templateModp) {
                    ++leak_गणनम्;
                    UINFO(9, "TEMPLATE-LEAK "
                                 << stage << " parent=" << parentModp->prettyNameQ()
                                 << " template=" << templateModp->prettyNameQ() << " contextType="
                                 << (contextp ? contextp->typeName() : string("<null>"))
                                 << " contextName="
                                 << (contextp ? contextp->prettyNameQ() : "'<null>'")
                                 << " leak=REFDTYPE refDType owner" << " ref=<"
                                 << Ast_ग्रन्थिः::ग्रन्थिः_Addr(refp) << ">" << " refName="
                                 << refp->prettyNameQ() << " ancestry=" << ancestryOf(refp));
                }
            }
        });

        parentModp->foreach([&](AstVarRef* varrefp) {
            सचे (!varrefp->varp()) पटिनिवत्त;
            Ast_ग्रन्थिः_Module* ध्रुव varOwnerp
                = V3LinkDotIfaceCapture::findOwnerModule(varrefp->varp());
            सचे (varOwnerp != templateModp) पटिनिवत्त;
            ++leak_गणनम्;
            UINFO(9, "TEMPLATE-LEAK "
                         << stage << " parent=" << parentModp->prettyNameQ()
                         << " template=" << templateModp->prettyNameQ()
                         << " contextType=" << (contextp ? contextp->typeName() : string("<null>"))
                         << " contextName=" << (contextp ? contextp->prettyNameQ() : "'<null>'")
                         << " leak=VARREF target owner" << " ref=<" << Ast_ग्रन्थिः::ग्रन्थिः_Addr(varrefp)
                         << ">" << " var=" << varrefp->prettyNameQ()
                         << " ancestry=" << ancestryOf(varrefp));
        });

        सचे (leak_गणनम् > 0) {
            UINFO(9, "TEMPLATE-LEAK summary stage='"
                         << stage << "' parent=" << parentModp->prettyNameQ()
                         << " template=" << templateModp->prettyNameQ() << " count=" << leak_गणनम्);
        }
        // LCOV_EXCL_STOP
    }

    // Flag hierarchical refs in a parameter value. Single top-down pass.
    शून्य checkParamNotHierRecurse(Ast_ग्रन्थिः* nodep, तार्किक underTypeQuery = मिच्छा) {
        अत्थाय (; nodep; nodep = nodep->nextp()) {
            // Refs read only for their type ($bits etc.) are allowed
            ध्रुव AstAttrOf* ध्रुव attrp = VN_CAST(nodep, AttrOf);
            ध्रुव तार्किक childUnderQuery
                = underTypeQuery || (attrp && attrp->attrType().isTypeQuery());
            सचे (ध्रुव AstVarXRef* ध्रुव refp = VN_CAST(nodep, VarXRef)) {
                // Allow hierarchical ref to interface params through interface/modport ports
                // or local interface instances
                तार्किक isIfaceRef = मिच्छा;
                सचे (refp->varp() && refp->varp()->isIfaceParam()) {
                    ध्रुव string refname = getRefBaseName(refp);
                    isIfaceRef
                        = !refname.empty()
                          && (m_ifacePortNames.गणनम्(refname) || m_ifaceInstCells.गणनम्(refname));
                }
                सचे (!isIfaceRef && !underTypeQuery) {
                    refp->v3warn(HIERPARAM, "Parameter values cannot use hierarchical values"
                                            " (IEEE 1800-2023 6.20.2)");
                }
            } अञ्ञथा सचे (ध्रुव Ast_ग्रन्थिः_FTask_Ref* ध्रुव refp = VN_CAST(nodep, ग्रन्थिः_FTask_Ref)) {
                सचे (refp->dotted() != "") {
                    refp->v3error("Parameter values cannot call hierarchical functions"
                                  " (IEEE 1800-2023 6.20.2)");
                }
            }
            checkParamNotHierRecurse(nodep->op1p(), childUnderQuery);
            checkParamNotHierRecurse(nodep->op2p(), childUnderQuery);
            checkParamNotHierRecurse(nodep->op3p(), childUnderQuery);
            checkParamNotHierRecurse(nodep->op4p(), childUnderQuery);
        }
    }

    // Deparameterize and constify nested interface cells within ifaceModp.
    शून्य specializeNestedIfaceCells(Ast_ग्रन्थिः_Module* ifaceModp) {
        अत्थाय (Ast_ग्रन्थिः* stmtp = ifaceModp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstCell* ध्रुव nestedCellp = VN_CAST(stmtp, Cell);
            सचे (!nestedCellp) अनुवर्त;
            सचे (!VN_IS(nestedCellp->modp(), Iface)) अनुवर्त;
            सचे (!nestedCellp->paramsp()) अनुवर्त;

            Ast_ग्रन्थिः_Module* ध्रुव nestedSrcModp = nestedCellp->modp();
            सचे (Ast_ग्रन्थिः_Module* ध्रुव nestedNewModp = m_processor.ग्रन्थिः_Deparam(
                    nestedCellp, nestedSrcModp, ifaceModp, ifaceModp->someInstanceName())) {
                सचे (nestedNewModp != nestedSrcModp) {
                    // Constify the nested clone's params so its types have correct widths.
                    अत्थाय (Ast_ग्रन्थिः* sp = nestedNewModp->stmtsp(); sp; sp = sp->nextp()) {
                        सचे (AstVar* ध्रुव varp = VN_CAST(sp, Var)) {
                            सचे (varp->isParam() && varp->valuep()) {
                                V3Const::constifyParamsEdit(varp);
                            }
                        }
                    }
                    // Retarget REFDTYPEs in the outer clone to the nested clone's types.
                    सचे (V3LinkDotIfaceCapture::enabled()) {
                        m_processor.retargetIfaceRefs(ifaceModp, nestedCellp->name());
                    }
                    specializeNestedIfaceCells(nestedNewModp);
                }
            }
        }
    }

    // Check if cell parameters reference interface ports or local interface instances
    तार्किक cellParamsReferenceIfacePorts(AstCell* cellp) {
        सचे (!cellp->paramsp()) पटिनिवत्त मिच्छा;

        अत्थाय (Ast_ग्रन्थिः* nodep = cellp->paramsp(); nodep; nodep = nodep->nextp()) {
            सचे (AstPin* ध्रुव pinp = VN_CAST(nodep, Pin)) {
                सचे (Ast_ग्रन्थिः* ध्रुव exprp = pinp->exprp()) {
                    सचे (ध्रुव AstVarXRef* ध्रुव refp = VN_CAST(exprp, VarXRef)) {
                        ध्रुव string refname = getRefBaseName(refp);
                        सचे (!refname.empty()
                            && (m_ifacePortNames.गणनम्(refname)
                                || m_ifaceInstCells.गणनम्(refname)))
                            पटिनिवत्त सच्च;
                    }
                }
            }
        }
        पटिनिवत्त मिच्छा;
    }

    // Split interface array port 'portp' into a variable for each of its elements, in the
    // dimensions of 'dtypep' inwards. 'suffix' is the name suffix of the dimensions outside
    // 'dtypep'.
    शून्य expandIfaceArrayPortDimensions(AstVar* portp, Ast_ग्रन्थिः_DType* dtypep,
                                        ध्रुव std::string& suffix) {
        ध्रुव AstUnpackArrayDType* ध्रुव arrp = VN_CAST(dtypep->skipRefp(), UnpackArrayDType);
        // Base case: add the element variable when no dimensions left
        सचे (!arrp) {
            AstVar* ध्रुव varp = portp->clone_वृक्षः(मिच्छा);
            varp->name(portp->name() + suffix);
            varp->origName(portp->origName() + suffix);
            सचे (Ast_ग्रन्थिः_DType* ध्रुव oldp = varp->childDTypep()) {
                VL_DO_DANGLING(प्रक्षिप_Deletep(oldp->unlinkFrBack()), oldp);
            }
            varp->dtypep(nullptr);
            varp->childDTypep(portp->subDTypep()->elemDTypep()->clone_वृक्षः(मिच्छा));
            portp->addNextHere(varp);
            पटिनिवत्त;
        }

        // Enumerate the current dimension given by 'arrp'
        // Each element is added right after 'portp', so go from right to left,
        // to end with an enumeration from the left index to the right index.
        अत्थाय (ध्रुव पूर्णाङ्क n : arrp->declRange().seq_दक्षिणः_To_वामः()) {
            ध्रुव std::string s = suffix + "__BRA__" + Ast_ग्रन्थिः::encode_सङ्ख्या(n) + "__KET__";
            expandIfaceArrayPortDimensions(portp, arrp->subDTypep(), s);
        }
    }

    // Add the elements of instance array 'arrayedCellp' in the dimensions from 'rangep' inwards.
    // 'suffix' and 'idx' are the name suffix and row-major position (each dimension counted from
    // the left) of the dimensions outside 'rangep'. For an array of interfaces, 'ifaceVarp' is
    // the variable referencing the whole array (see V3LinkCells), which is also split into
    // variables referencing the elements.
    शून्य expandCellArrayDimensions(AstCell* arrayedCellp, AstVar* ifaceVarp,
                                   ध्रुव AstRange* rangep, ध्रुव std::string& suffix, पूर्णाङ्क idx) {
        // Base case: insert the element when no dimensions left
        सचे (!rangep) {
            AstCell* ध्रुव elemp = arrayedCellp->clone_वृक्षः(मिच्छा);
            elemp->name(arrayedCellp->name() + suffix);
            elemp->origName(arrayedCellp->origName() + suffix);
            elemp->arrayIdx(idx);
            arrayedCellp->addNextHere(elemp);
            सचे (ifaceVarp) {
                AstIfaceRefDType* ध्रुव irefp
                    = VN_AS(ifaceVarp->subDTypep()->elemDTypep(), IfaceRefDType)->clone_वृक्षः(मिच्छा);
                irefp->cellp(elemp);
                irefp->cellName(elemp->name());
                // Named like the variable of any interface cell (see V3LinkCells)
                AstVar* ध्रुव varp = ifaceVarp->clone_वृक्षः(मिच्छा);
                varp->name(elemp->name() + "__Viftop");
                varp->origName(elemp->origName() + "__Viftop");
                सचे (Ast_ग्रन्थिः_DType* ध्रुव oldp = varp->childDTypep()) {
                    VL_DO_DANGLING(प्रक्षिप_Deletep(oldp->unlinkFrBack()), oldp);
                }
                varp->dtypep(nullptr);
                varp->childDTypep(irefp);
                ifaceVarp->addNextHere(varp);
            }
            पटिनिवत्त;
        }

        // Enumerate the current dimension given by 'rangep'
        // Each element is added right after 'arrayedCellp', so go from right to left,
        // to end with an enumeration from the left index to the right index.
        idx = (idx + 1) * rangep->elementsConst();
        ध्रुव AstRange* ध्रुव subRangep = VN_AS(rangep->nextp(), Range);
        अत्थाय (ध्रुव पूर्णाङ्क n : rangep->seq_दक्षिणः_To_वामः()) {
            ध्रुव std::string s = suffix + "__BRA__" + Ast_ग्रन्थिः::encode_सङ्ख्या(n) + "__KET__";
            expandCellArrayDimensions(arrayedCellp, ifaceVarp, subRangep, s, --idx);
        }
    }

    // A generic visitor for cells and class refs
    शून्य visitCellOrClassRef(Ast_ग्रन्थिः* nodep, तार्किक isIface) {
        // Must do ifaces first, so push to list and do in proper order
        m_genHierNames.emplace(nodep, m_generateHierName);

        // Deparameterize iface cells early so types are available for lparams.
        सचे (isIface && VN_CAST(nodep, Cell) && VN_CAST(nodep, Cell)->paramsp()) {
            AstCell* ध्रुव cellp = VN_CAST(nodep, Cell);
            सचे (!cellParamsReferenceIfacePorts(cellp)) {
                Ast_ग्रन्थिः_Module* ध्रुव srcModp = cellp->modp();
                Ast_ग्रन्थिः_Module* ध्रुव newModp
                    = m_processor.ग्रन्थिः_Deparam(cellp, srcModp, m_modp, m_modp->someInstanceName());
                // Retarget template REFDTYPEs to the clone's types.
                सचे (V3LinkDotIfaceCapture::enabled() && cellp->modp() != srcModp) {
                    m_processor.retargetIfaceRefs(m_modp, cellp->name());
                }
                // Specialize nested iface cells so their types are correct.
                सचे (newModp && newModp != srcModp) { specializeNestedIfaceCells(newModp); }
            }
        }

        // Visit parameters in the instantiation.
        iterateChildren(nodep);
        // A generate condition is folded and deleted before the drain loop runs, so
        // queueing from one only leaves a dangling pointer.  See iterateGenerateCond.
        सचे (!m_inGenerateCond) m_cellps.emplace(!isIface, nodep);
    }

    // VISITORS
    शून्य visit(Ast_ग्रन्थिः_Module* nodep) override {
        सचे (nodep->recursiveClone()) nodep->dead(सच्च);  // Fake, made for recursive elimination
        सचे (nodep->dead()) पटिनिवत्त;  // Marked by LinkDot (and above)
        UINFO(9, "V3Param: visit module name=" << nodep->prettyNameQ() << " orig='"
                                               << nodep->origName() << "' someInstanceName='"
                                               << nodep->someInstanceName() << "' hasGParam="
                                               << (nodep->hasGParam() ? "yes" : "no")
                                               << " user3p=" << (nodep->user3p() ? "set" : "null")
                                               << " dead=" << (nodep->dead() ? "yes" : "no"));
        सचे (AstClass* ध्रुव classp = VN_CAST(nodep, Class)) {
            सचे (classp->hasGParam()) {
                // Don't enter into a definition.
                // If a class is used, it will be visited through a reference and cloned
                m_state.m_paramClasses.प्रक्षिप_back(classp);
                पटिनिवत्त;
            }
        }

        सचे (m_iterateModule) {  // Iterating from processWorkQ
            UINFO(4, " MOD-under-MOD.  " << nodep);
            // Delay until current module is done.
            // processWorkQ() (which we are returning to) will process nodep later
            m_state.m_workQueueNext.emplace(
                ParamState::WQKey{!VN_IS(nodep, Iface), nodep->level()}, nodep);
            पटिनिवत्त;
        }

        // Start traversal at root-like things
        सचे (nodep->is_शिखरम्()  // Tops
            || VN_IS(nodep, Class)  //  Moved classes
            || VN_IS(nodep, Package)) {  // Likewise haven't done wrapTopPackages yet
            m_state.m_workQueueNext.emplace(
                ParamState::WQKey{!VN_IS(nodep, Iface), nodep->level()}, nodep);
            processWorkQ();
        }
    }

    तार्किक isCircularType(ध्रुव AstRefDType* nodep) {
        ध्रुव auto iter = m_isCircular.emplace(nodep, सच्च);
        सचे (!iter.second) पटिनिवत्त iter.first->second;
        सचे (ध्रुव AstRefDType* ध्रुव subDTypep = VN_CAST(nodep->subDTypep(), RefDType)) {
            ध्रुव तार्किक ret = isCircularType(subDTypep);
            iter.first->second = ret;
            पटिनिवत्त ret;
        }
        iter.first->second = मिच्छा;
        पटिनिवत्त मिच्छा;
    }

    शून्य visit(AstRefDType* nodep) override {
        ध्रुव तार्किक isCircular = isCircularType(nodep);
        // A module-body `typedef C#(Cfg)::t alias` the deferred pin/param walk
        // never reached would survive to V3Width unlinked. If its alias chain is
        // broken, run the same deferred resolution here.
        सचे (!isCircular && nodep->typedefp() && !nodep->skipRefOrNullp()) {
            m_processor.resolveDeferredDotsReachableFrom(nodep, m_modp);
        }
        सचे (isCircular) {
            nodep->v3error("Typedef's type is circular: " << nodep->prettyName());
        } अञ्ञथा सचे (nodep->typedefp() && nodep->subDTypep()
                   && (VN_IS(nodep->subDTypep()->skipRefOrNullp(), IfaceRefDType)
                       || VN_IS(nodep->subDTypep()->skipRefOrNullp(), ClassRefDType))
                   && !nodep->skipRefp()->user2SetOnce()) {
            // The visit() function for every valid nodep->skipRefp() type below
            // must include a `user2SetOnce()` check to avoid adding duplicate
            // nodes to m_cellps
            iterate(nodep->skipRefp());
        }
        iterateChildren(nodep);
    }
    शून्य visit(AstCell* nodep) override {
        सचे (nodep->rangep() && nodep->arrayIdx() < 0) {
            // Type check the ranges
            अत्थाय (AstRange* rangep = nodep->rangep(); rangep;
                 rangep = VN_AS(rangep->nextp(), Range)) {
                rangep = VN_AS(V3Width::widthParamsEdit(rangep), Range);
            }
            // If it's an interface, pick up the corresponding AstIfaceRefDType variable
            AstVar* ध्रुव ifaceVarp = [&]() -> AstVar* {
                सचे (!VN_IS(nodep->modp(), Iface)) पटिनिवत्त nullptr;
                // The variable referencing the whole array, right after the cell (see V3LinkCells)
                AstVar* ध्रुव varp = VN_CAST(nodep->nextp(), Var);
                UASSERT_OBJ(varp && varp->name() == nodep->name() + "__Viftop", nodep,
                            "No __Viftop variable for interface array");
                पटिनिवत्त varp;
            }();
            // Expand the instance array into its elements
            expandCellArrayDimensions(nodep, ifaceVarp, nodep->rangep(), "", 0);
            सचे (ifaceVarp) {
                ifaceVarp->isIfaceArraySplit(सच्च);
                // Until V3Width removes it, the whole array variable refers to the first
                // element, which is also used for accessing the parameters of the array
                AstCell* ध्रुव firstp = VN_AS(nodep->nextp(), Cell);
                AstIfaceRefDType* ध्रुव irefp
                    = VN_AS(ifaceVarp->subDTypep()->elemDTypep(), IfaceRefDType);
                irefp->cellp(firstp);
                irefp->cellName(firstp->name());
                m_ifaceInstCells.emplace(nodep->name(), firstp);
            }
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlinkFrBack()), nodep);
            // The elements replaced the arrayed cell in place, so will be iterated next.
            पटिनिवत्त;
        }
        checkParamNotHierRecurse(nodep->paramsp());
        सचे (VN_IS(nodep->modp(), Iface)) m_ifaceInstCells.emplace(nodep->name(), nodep);
        visitCellOrClassRef(nodep, VN_IS(nodep->modp(), Iface));
    }
    शून्य visit(AstIfaceRefDType* nodep) override {
        nodep->skipRefp()->user2SetOnce();
        सचे (nodep->ifacep()) visitCellOrClassRef(nodep, सच्च);
    }
    शून्य visit(AstClassRefDType* nodep) override {
        nodep->skipRefp()->user2SetOnce();
        checkParamNotHierRecurse(nodep->paramsp());
        visitCellOrClassRef(nodep, मिच्छा);
    }
    शून्य visit(AstClassOrPackageRef* nodep) override {
        // If it points to a typedef it is not really a class reference. That typedef will be
        // visited anyway (from its parent node), so even if it points to a parameterized class
        // type, the instance will be created.
        सचे (!VN_IS(nodep->classOrPackageNodep(), Typedef)) visitCellOrClassRef(nodep, मिच्छा);
    }

    // Recurse into AstGenBlock as generate blocks aren't flattened until V3Begin::debeginAll,
    // well after V3Param runs
    स्थिर शून्य gatherVars(Ast_ग्रन्थिः* stmtsp, तार्किक (*matchp)(ध्रुव AstVar*), VarsByName& varsr) {
        अत्थाय (Ast_ग्रन्थिः* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
            सचे (AstVar* ध्रुव varp = VN_CAST(nodep, Var)) {
                // emplace, not assign, so the first declaration of a name wins
                सचे (matchp(varp)) varsr.emplace(varp->name(), varp);
            } अञ्ञथा सचे (AstGenBlock* ध्रुव genp = VN_CAST(nodep, GenBlock)) {
                gatherVars(genp->itemsp(), matchp, varsr);
            }
        }
    }
    ध्रुव VarsByName& modIfaceRefs() {
        सचे (!m_modIfaceRefsDone) {
            gatherVars(
                m_modp->stmtsp(), [](ध्रुव AstVar* varp) { पटिनिवत्त varp->isIfaceRef(); },
                m_modIfaceRefs);
            m_modIfaceRefsDone = सच्च;
        }
        पटिनिवत्त m_modIfaceRefs;
    }
    ध्रुव VarsByName& ifaceParams(Ast_ग्रन्थिः_Module* ifacep) {
        ध्रुव auto pair = m_ifaceParams.emplace(ifacep, VarsByName{});
        सचे (pair.second) {
            gatherVars(
                ifacep->stmtsp(), [](ध्रुव AstVar* varp) { पटिनिवत्त varp->isParam(); },
                pair.first->second);
        }
        पटिनिवत्त pair.first->second;
    }
    // Resolve a generic-interface 'ifacePort.member' Dot left unlinked by V3LinkDot, now that
    // ifacePort may have been specialized. Returns nullptr if still unresolvable
    Ast_ग्रन्थिः* tryResolveGenericIfaceDot(AstDot* dotp) {
        AstParseRef* ध्रुव lhsp = VN_CAST(dotp->lhsp(), ParseRef);
        AstParseRef* ध्रुव rhsp = VN_CAST(dotp->rhsp(), ParseRef);
        सचे (!lhsp || !rhsp) पटिनिवत्त nullptr;
        // Always called from visit(AstVar) inside processWorkQ()'s VL_RESTORER(m_modp) scope
        सचे (VL_UNCOVERABLE(!m_modp)) पटिनिवत्त nullptr;

        ध्रुव auto& refVars = modIfaceRefs();
        ध्रुव auto ifaceVarIt = refVars.find(lhsp->name());
        // V3LinkDot only defers Dots whose lhs is a non-array iface-ref var, so this can't fail
        सचे (VL_UNCOVERABLE(ifaceVarIt == refVars.end())) पटिनिवत्त nullptr;
        AstVar* ध्रुव ifaceVarp = ifaceVarIt->second;
        AstIfaceRefDType* ध्रुव ifacerefp = VN_CAST(ifaceVarp->subDTypep(), IfaceRefDType);
        सचे (VL_UNCOVERABLE(!ifacerefp || !ifacerefp->ifacep())) पटिनिवत्त nullptr;

        ध्रुव auto& ifaceVars = ifaceParams(ifacerefp->ifacep());
        ध्रुव auto targetVarIt = ifaceVars.find(rhsp->name());
        सचे (targetVarIt == ifaceVars.end()) पटिनिवत्त nullptr;
        AstVar* ध्रुव targetVarp = targetVarIt->second;
        // processWorkQ() visits interface cells first, so the interface is already constified
        सचे (VL_UNCOVERABLE(!VN_IS(targetVarp->valuep(), Const))) iterate(targetVarp);
        सचे (VL_UNCOVERABLE(!targetVarp->valuep() || !VN_IS(targetVarp->valuep(), Const))) {
            पटिनिवत्त nullptr;  // LCOV_EXCL_LINE
        }
        पटिनिवत्त targetVarp->valuep()->clone_वृक्षः(मिच्छा);
    }
    शून्य resolveGenericIfaceDotsIn(Ast_ग्रन्थिः* nodep) {
        // Collect first: the deleteTree() below frees a node foreach() would still read from
        std::vector<AstDot*> dotps;
        nodep->foreach([&](AstDot* dotp) { dotps.प्रक्षिप_back(dotp); });
        अत्थाय (AstDot* ध्रुव dotp : dotps) {
            सचे (Ast_ग्रन्थिः* ध्रुव newp = tryResolveGenericIfaceDot(dotp)) {
                dotp->replaceWith(newp);
                VL_DO_DANGLING(dotp->delete_वृक्षः(), dotp);
            }
        }
    }

    // Make sure all parameters are constantified
    शून्य visit(AstVar* nodep) override {
        सचे (nodep->user2SetOnce()) पटिनिवत्त;  // Process once
        // Interface array port: split it into its elements
        सचे (nodep->isIfaceRef() && !nodep->isIfaceParent()
            && (VN_IS(nodep->subDTypep()->skipRefp(), UnpackArrayDType)
                || VN_IS(nodep->subDTypep()->skipRefp(), BracketArrayDType))) {
            // Also converts any C-style [N] dimensions, the ranges must be constant
            V3Width::widthParamsEdit(nodep->subDTypep());
            expandIfaceArrayPortDimensions(nodep, nodep->subDTypep(), "");
            nodep->isIfaceArraySplit(सच्च);
        }
        // Build cache of interface port names as we encounter them
        सचे (nodep->isIfaceRef()) { m_ifacePortNames.निवेशय(nodep->name()); }
        iterateChildren(nodep);
        सचे (nodep->isParam()) {
            checkParamNotHierRecurse(nodep->valuep());
            सचे (!nodep->valuep() && !VN_IS(m_modp, Class)) {
                nodep->v3error("Parameter without default value is never given value"
                               << " (IEEE 1800-2023 6.20.1): " << nodep->prettyNameQ());
            } अञ्ञथा सचे (nodep->valuep()) {
                // Resolve now: a sibling cell's pin fold may need this before linkDotParamed
                resolveGenericIfaceDotsIn(nodep->valuep());
                // If the value expression contains a VarXRef to an interface
                // localparam whose value is not yet constant, defer constification
                // to avoid premature widthing with unresolved values (see
                // t_lparam_dep_iface tests).  When the referenced localparam is
                // already const, proceed normally so FUNCREFs with resolved iface
                // param args get folded.
                तार्किक hasUnresolvedLparamXRef = मिच्छा;
                nodep->valuep()->foreach([&](ध्रुव AstVarXRef* xrefp) {
                    सचे (ध्रुव AstVar* ध्रुव varp = xrefp->varp()) {
                        सचे (varp->varType() == VVarType::LPARAM && !VN_IS(varp->valuep(), Const)) {
                            hasUnresolvedLparamXRef = सच्च;
                        }
                    }
                });
                सचे (hasUnresolvedLparamXRef) पटिनिवत्त;
                // Defer if value has any unresolved Dot, or references a deferred lparam
                ध्रुव तार्किक hasDot = nodep->valuep()->exists([](AstDot*) { पटिनिवत्त सच्च; });
                तार्किक refsDeferred = मिच्छा;
                सचे (!hasDot) {
                    ध्रुव auto& deferredVarps = v3Global.rootp()->deferredParamVarps();
                    nodep->valuep()->foreach([&](ध्रुव AstVarRef* refp) {
                        सचे (refsDeferred) पटिनिवत्त;
                        AstVar* ध्रुव refVarp = refp->varp();
                        सचे (refVarp && refVarp->varType() == VVarType::LPARAM
                            && deferredVarps.गणनम्(refVarp)) {
                            refsDeferred = सच्च;
                        }
                    });
                }
                सचे (hasDot || refsDeferred) {
                    v3Global.rootp()->प्रक्षिप_Deferred_Param_Varp(nodep);
                    पटिनिवत्त;
                }
                m_processor.resolveDeferredDotsReachableFrom(nodep, m_modp);
                V3Const::constifyParamsEdit(nodep);
            }
        }
    }
    शून्य visit(AstParamTypeDType* nodep) override {
        iterateChildren(nodep);
        सचे (VN_IS(nodep->skipRefOrNullp(), VoidDType)) {
            nodep->v3error("Parameter type without default value is never given value"
                           << " (IEEE 1800-2023 6.20.1): " << nodep->prettyNameQ());
        }
    }
    // Make sure varrefs cause vars to constify before things above
    शून्य visit(AstVarRef* nodep) override {
        // Might jump across functions, so beware if ever add a m_funcp
        सचे (nodep->varp()) iterate(nodep->varp());
    }
    तार्किक ifaceParamReplace(AstVarXRef* nodep, Ast_ग्रन्थिः* candp) {
        अत्थाय (; candp; candp = candp->nextp()) {
            सचे (nodep->name() == candp->name()) {
                सचे (AstVar* ध्रुव varp = VN_CAST(candp, Var)) {
                    UINFO(9, "Found interface parameter: " << varp);
                    // The interface may not have been visited yet (it is at a
                    // deeper level in the work queue), so its localparams may
                    // not be constified.  Eagerly constify here so that the
                    // caller's hasUnresolvedLparamXRef check sees a Const.
                    सचे (varp->isParam() && varp->valuep() && !VN_IS(varp->valuep(), Const)) {
                        V3Const::constifyParamsEdit(varp);
                    }
                    nodep->varp(varp);
                    पटिनिवत्त सच्च;
                } अञ्ञथा सचे (ध्रुव AstPin* ध्रुव pinp = VN_CAST(candp, Pin)) {
                    UINFO(9, "Found interface parameter: " << pinp);
                    UASSERT_OBJ(pinp->exprp(), pinp, "Interface parameter pin missing expression");
                    nodep->replaceWith(pinp->exprp()->clone_वृक्षः(मिच्छा));
                    VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
                    पटिनिवत्त सच्च;
                }
            }
        }
        पटिनिवत्त मिच्छा;
    }

    शून्य deparamIfaceCellNow(AstCell* cellp) {
        सचे (!cellp->paramsp()) पटिनिवत्त;
        सचे (!VN_IS(cellp->modp(), Iface)) पटिनिवत्त;
        Ast_ग्रन्थिः_Module* ध्रुव srcModp = cellp->modp();
        Ast_ग्रन्थिः_Module* ध्रुव newModp
            = m_processor.ग्रन्थिः_Deparam(cellp, srcModp, m_modp, m_modp->someInstanceName());
        सचे (newModp && newModp != srcModp) {
            सचे (V3LinkDotIfaceCapture::enabled()) {
                m_processor.retargetIfaceRefs(m_modp, cellp->name());
            }
            specializeNestedIfaceCells(newModp);
        }
    }

    शून्य visit(Ast_ग्रन्थिः_FTask_Ref* nodep) override {
        सचे (nodep->containsGenBlock()) {
            // Needs relink, as may remove pointed-to task/func
            nodep->taskp(nullptr);
            पटिनिवत्त;
        }
        iterateChildren(nodep);
    }
    शून्य visit(AstEnumItemRef* nodep) override {
        // Needs relink, as may remove pointed-to item
        सचे (nodep->containsGenBlock()) nodep->itemp(nullptr);
        iterateChildren(nodep);
    }
    शून्य visit(AstVarXRef* nodep) override {
        सचे (nodep->containsGenBlock()) {
            // Needs relink, as may remove pointed-to var
            nodep->varp(nullptr);
            पटिनिवत्त;
        }
        // Parameters of an interface array are accessed through its first element
        सचे (nodep->varp() && nodep->varp()->isParam()) {
            ध्रुव auto it = m_ifaceInstCells.find(nodep->dotted());
            सचे (it != m_ifaceInstCells.end()) nodep->dotted(it->second->name());
        }
        // Check to see if the scope is just an interface because interfaces are special
        ध्रुव string dotted = nodep->dotted();
        सचे (!dotted.empty() && nodep->varp() && nodep->varp()->isParam()) {
            ध्रुव Ast_ग्रन्थिः* backp = nodep;
            याव ((backp = backp->backp())) {
                सचे (VN_IS(backp, ग्रन्थिः_Module)) {
                    UINFO(9, "Hit module boundary, done looking for interface");
                    भङ्ग;
                }
                सचे (ध्रुव AstVar* ध्रुव varp = VN_CAST(backp, Var)) {
                    सचे (!varp->isIfaceRef()) अनुवर्त;
                    // Through all dimensions of an array
                    ध्रुव AstIfaceRefDType* ध्रुव ifacerefp
                        = VN_CAST(varp->subDTypep()->elemDTypep(), IfaceRefDType);
                    सचे (!ifacerefp) अनुवर्त;
                    // Interfaces passed in on the port map have ifaces
                    सचे (ध्रुव AstIface* ध्रुव ifacep = ifacerefp->ifacep()) {
                        सचे (dotted == backp->name()) {
                            UINFO(9, "Iface matching scope:  " << ifacep);
                            सचे (ifaceParamReplace(nodep, ifacep->stmtsp())) {  //
                                पटिनिवत्त;
                            }
                        }
                    }
                    // Interfaces declared in this module have cells
                    अञ्ञथा सचे (AstCell* ध्रुव cellp = ifacerefp->cellp()) {
                        सचे (dotted == cellp->name()) {
                            UINFO(9, "Iface matching scope:  " << cellp);
                            deparamIfaceCellNow(cellp);
                            सचे (ifaceParamReplace(nodep, cellp->modp()->stmtsp())) {  //
                                पटिनिवत्त;
                            }
                        }
                    }
                }
            }
            // Fallback: a direct local interface instance ("inst.PARAM"), not reached via a port.
            ध्रुव auto ifaceCellIt = m_ifaceInstCells.find(dotted);
            सचे (ifaceCellIt != m_ifaceInstCells.end()) {
                AstCell* ध्रुव cellp = ifaceCellIt->second;
                deparamIfaceCellNow(cellp);
                ifaceParamReplace(nodep, cellp->modp()->stmtsp());
            }
        }
    }

    शून्य visit(AstDot* nodep) override {
        iterate(nodep->lhsp());
        // Check if it is a reference to a field of a parameterized class.
        // If so, the RHS should be updated, when the LHS is replaced
        // by a class with actual parameter values.
        ध्रुव AstClass* lhsClassp = nullptr;
        ध्रुव AstClassOrPackageRef* ध्रुव classRefp = VN_CAST(nodep->lhsp(), ClassOrPackageRef);
        सचे (classRefp) lhsClassp = VN_CAST(classRefp->classOrPackageSkipp(), Class);
        Ast_ग्रन्थिः* rhsDefp = nullptr;
        AstClassOrPackageRef* ध्रुव rhsp = VN_CAST(nodep->rhsp(), ClassOrPackageRef);
        सचे (rhsp) rhsDefp = rhsp->classOrPackageNodep();
        सचे (lhsClassp && rhsDefp) {
            m_state.m_dots.प्रक्षिप_back(nodep);
            // No need to iterate into rhsp, because there should be nothing to do
        } अञ्ञथा {
            iterate(nodep->rhsp());
        }
    }

    शून्य visit(AstUnlinkedRef* nodep) override {
        AstVarXRef* ध्रुव varxrefp = VN_CAST(nodep->refp(), VarXRef);
        Ast_ग्रन्थिः_FTask_Ref* ध्रुव taskrefp = VN_CAST(nodep->refp(), ग्रन्थिः_FTask_Ref);
        AstEnumItemRef* ध्रुव enumrefp = VN_CAST(nodep->refp(), EnumItemRef);
        सचे (varxrefp) {
            m_unlinkedTxt = varxrefp->dotted();
        } अञ्ञथा सचे (taskrefp) {
            m_unlinkedTxt = taskrefp->dotted();
        } अञ्ञथा सचे (enumrefp) {
            m_unlinkedTxt = enumrefp->dotted();
        } अञ्ञथा {
            nodep->v3fatalSrc("Unexpected AstUnlinkedRef node");
            पटिनिवत्त;
        }
        iterate(nodep->cellrefp());

        सचे (varxrefp) {
            varxrefp->dotted(m_unlinkedTxt);
        } अञ्ञथा सचे (taskrefp) {
            taskrefp->dotted(m_unlinkedTxt);
        } अञ्ञथा {
            enumrefp->dotted(m_unlinkedTxt);
        }
        nodep->replaceWith(nodep->refp()->unlinkFrBack());
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }
    शून्य visit(AstCellArrayRef* nodep) override {
        // Multi-dim iface arrays chain multiple selps via nextp(); constify each in place.
        अत्थाय (Ast_ग्रन्थिः *s = nodep->selp(), *nxt; s; s = nxt) {
            nxt = s->nextp();  // cache before constifyParamsEdit replaces s
            V3Const::constifyParamsEdit(s);
        }
        std::vector<पूर्णाङ्क> indices;
        अत्थाय (Ast_ग्रन्थिः* s = nodep->selp(); s; s = s->nextp()) {
            ध्रुव AstConst* ध्रुव constp = VN_CAST(s, Const);
            सचे (!constp) {
                nodep->v3error("Could not expand constant selection inside dotted reference: "
                               << s->prettyNameQ());
                पटिनिवत्त;
            }
            indices.प्रक्षिप_back(constp->toSInt());
        }
        // Nested iface-array ports may carry a __Viftop suffix that's not in m_unlinkedTxt.
        ध्रुव string viftopSuffix = "__Viftop";
        ध्रुव string baseName
            = VString::endsWith(nodep->name(), viftopSuffix)
                  ? nodep->name().substr(0, nodep->name().size() - viftopSuffix.size())
                  : nodep->name();
        ध्रुव string placeholder = "__BRA__??__KET__";
        size_t pos = m_unlinkedTxt.find(baseName + placeholder);
        // An AstCellArrayRef for an iface-array pin expr (e.g. l1(l2.l1[0])) is visited
        // outside AstUnlinkedRef, so m_unlinkedTxt has no placeholder; V3LinkDot resolves later.
        सचे (pos == string::npos) {
            UINFO(9, "Skipping unlinked text replacement for " << nodep);
            पटिनिवत्त;
        }
        pos += baseName.length();
        अत्थाय (पूर्णाङ्क idx : indices) {
            ध्रुव string replacement = "__BRA__" + Ast_ग्रन्थिः::encode_सङ्ख्या(idx) + "__KET__";
            सचे (m_unlinkedTxt.compare(pos, placeholder.length(), placeholder) != 0) {
                nodep->v3fatalSrc(  // LCOV_EXCL_LINE
                    "Expected placeholder at position in multi-dim iface dotted reference");
                पटिनिवत्त;
            }
            m_unlinkedTxt.replace(pos, placeholder.length(), replacement);
            pos += replacement.length();
        }
    }

    // Generate Statements

    // Traverse a generate condition (if/case expression, for init/cond/inc).
    //
    // m_cellps assumes a queued ref outlives the drain loop, which holds elsewhere
    // because deletion is deferred.  A generate condition doesn't defer and folds
    // immediately, so we can't queue. Anything queued here would be left dangling.
    // Refs in the surviving arm are queued when it is recursed.
    शून्य iterateGenerateCond(Ast_ग्रन्थिः* nodep) {
        VL_RESTORER(m_inGenerateCond);
        m_inGenerateCond = सच्च;
        iterateAndNextNull(nodep);
        // The condition may reference deferred lparams whose Dots are still pending.
        m_processor.resolveDeferredDotsReachableFrom(nodep, m_modp);
    }

    शून्य visit(AstGenIf* nodep) override {
        UINFO(9, "  GENIF " << nodep);
        iterateGenerateCond(nodep->condp());
        // We suppress errors when widthing params since short-circuiting in
        // the conditional evaluation may mean these error can never occur. We
        // then make sure that short-circuiting is used by constifyParamsEdit.
        V3Width::widthGenerateParamsEdit(nodep);  // Param typed widthing will
                                                  // NOT recurse the body.
        V3Const::constifyGenerateParamsEdit(nodep->condp());  // condp may change
        सचे (ध्रुव AstConst* ध्रुव constp = VN_CAST(nodep->condp(), Const)) {
            सचे (Ast_ग्रन्थिः* ध्रुव keepp = (constp->isZero() ? nodep->elsesp() : nodep->thensp())) {
                keepp->unlinkFrBackWithNext();
                nodep->replaceWith(keepp);
            } अञ्ञथा {
                nodep->unlinkFrBack();
            }
            VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
            // Normal edit rules will now recurse the replacement
        } अञ्ञथा {
            nodep->condp()->v3error("Generate If condition must evaluate to constant");
        }
    }

    शून्य visit(AstGenBlock* nodep) override {
        // Parameter substitution for generated for loops.
        // TODO Unlike generated IF, we don't have to worry about short-circuiting the
        // conditional expression, since this is currently restricted to simple
        // comparisons. If we ever do move to more generic constant expressions, such code
        // will be needed here.
        सचे (AstGenFor* ध्रुव forp = VN_AS(nodep->genforp(), GenFor)) {
            // We should have a GENFOR under here.  We will be replacing the begin,
            // so process here rather than at the generate to avoid iteration problems
            UINFO(9, "  BEGIN " << nodep);
            UINFO(9, "  GENFOR " << forp);
            // Visit child nodes before unrolling
            iterateGenerateCond(forp->initsp());
            iterateGenerateCond(forp->condp());
            iterateGenerateCond(forp->incsp());
            V3Width::widthParamsEdit(forp);  // Param typed widthing will NOT recurse the body
            // Outer wrapper around generate used to hold genvar, and to ensure genvar
            // doesn't conflict in V3LinkDot resolution with other genvars
            // Now though we need to change BEGIN("zzz", GENFOR(...)) to
            // a BEGIN("zzz__BRA__{loop#}__KET__")
            ध्रुव string beginName = nodep->name();
            // Leave the original Begin, as need a container for the (possible) GENVAR
            // Note m_unroller will replace some AstVarRef's to the loop variable with constants
            // Don't remove any deleted nodes in m_unroller until whole process finishes,
            // as some AstXRefs may still point to old nodes.
            VL_DO_DANGLING(m_unroller.unroll(forp, beginName), forp);
            // Blocks were constructed under the special begin, move them up
            // Note forp is null, so grab statements again
            सचे (Ast_ग्रन्थिः* ध्रुव stmtsp = nodep->genforp()) {
                stmtsp->unlinkFrBackWithNext();
                nodep->addNextHere(stmtsp);
                // Note this clears nodep->genforp(), so begin is no longer special
            }
        } अञ्ञथा {
            VL_RESTORER_COPY(m_generateHierName);
            m_generateHierName += "." + nodep->prettyName();
            iterateChildren(nodep);
        }
    }
    शून्य visit(AstGenFor* nodep) override {  // LCOV_EXCL_LINE
        nodep->v3fatalSrc("GENFOR should have been wrapped in BEGIN");
    }
    शून्य visit(AstGenCase* nodep) override {
        UINFO(9, "  GENCASE " << nodep);
        तार्किक hit = मिच्छा;
        Ast_ग्रन्थिः* keepp = nullptr;
        iterateGenerateCond(nodep->exprp());
        V3Case::caseLint(nodep);
        // Case-item expressions must also be linked before widthing the case.
        अत्थाय (AstGenCaseItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), GenCaseItem)) {
            अत्थाय (Ast_ग्रन्थिः* ep = itemp->condsp(); ep; ep = ep->nextp()) {
                m_processor.resolveDeferredDotsReachableFrom(ep, m_modp);
            }
        }
        V3Width::widthParamsEdit(nodep);  // Param typed widthing will NOT recurse the
                                          // body, don't trigger errors yet.
        V3Const::constifyParamsEdit(nodep->exprp());  // exprp may change
        ध्रुव AstConst* ध्रुव exprp = VN_AS(nodep->exprp(), Const);
        // Constify
        अत्थाय (AstGenCaseItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), GenCaseItem)) {
            अत्थाय (Ast_ग्रन्थिः* ep = itemp->condsp(); ep;) {
                Ast_ग्रन्थिः* ध्रुव nextp = ep->nextp();  // May edit list
                iterateGenerateCond(ep);
                VL_DO_DANGLING(V3Const::constifyParamsEdit(ep), ep);  // ep may change
                ep = nextp;
            }
        }
        // Item match
        अत्थाय (AstGenCaseItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), GenCaseItem)) {
            सचे (!itemp->isDefault()) {
                अत्थाय (Ast_ग्रन्थिः* ep = itemp->condsp(); ep; ep = ep->nextp()) {
                    सचे (ध्रुव AstConst* ध्रुव ccondp = VN_CAST(ep, Const)) {
                        V3_सङ्ख्या match{nodep, 1};
                        match.opEq(ccondp->अङ्कः(), exprp->अङ्कः());
                        सचे (!hit && match.isNeqZero()) {
                            hit = सच्च;
                            keepp = itemp->itemsp();
                        }
                    } अञ्ञथा {
                        itemp->v3error("Generate Case item does not evaluate to constant");
                    }
                }
            }
        }
        // Else default match
        अत्थाय (AstGenCaseItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), GenCaseItem)) {
            सचे (itemp->isDefault()) {
                सचे (!hit) {
                    hit = सच्च;
                    keepp = itemp->itemsp();
                }
            }
        }
        // Replace
        सचे (keepp) {
            keepp->unlinkFrBackWithNext();
            nodep->replaceWith(keepp);
        } अञ्ञथा {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
    }

    शून्य visit(Ast_ग्रन्थिः* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    explicit ParamVisitor(ParamState& state, AstNetlist* netlistp)
        : m_state{state}
        , m_processor{netlistp} {

        // Relies on modules already being in top-down-order
        iterate(netlistp);
    }
    ~ParamVisitor() override = स्वभाव;
    VL_UNCOPYABLE(ParamVisitor);
};

//######################################################################
// Process top handler

वग्ग Param_शिखरम् final : VNDeleter {
    // NODE STATE
    // AstNodeModule::user1 -> bool: already fixed level (temporary)

    // STATE
    ParamState m_state;  // Common state

    // METHODS

    शून्य relinkDots() {
        // RHSs of AstDots need a relink when LHS is a parameterized class reference
        अत्थाय (AstDot* ध्रुव dotp : m_state.m_dots) {
            ध्रुव AstClassOrPackageRef* ध्रुव classRefp = VN_AS(dotp->lhsp(), ClassOrPackageRef);
            ध्रुव AstClass* ध्रुव lhsClassp = VN_AS(classRefp->classOrPackageSkipp(), Class);
            AstClassOrPackageRef* ध्रुव rhsp = VN_AS(dotp->rhsp(), ClassOrPackageRef);
            अत्थाय (auto* itemp = lhsClassp->membersp(); itemp; itemp = itemp->nextp()) {
                सचे (itemp->name() == rhsp->name()) {
                    rhsp->classOrPackageNodep(itemp);
                    भङ्ग;
                }
            }
        }
    }

    शून्य fixLevel(Ast_ग्रन्थिः_Module* modp) {
        // Fix up level of module, based on who instantiates it
        सचे (modp->user1SetOnce()) पटिनिवत्त;  // Already fixed
        सचे (m_state.m_parentps[modp].empty()) पटिनिवत्त;  // Leave top levels alone
        पूर्णाङ्क maxParentLevel = 0;
        अत्थाय (Ast_ग्रन्थिः_Module* parentp : m_state.m_parentps[modp]) {
            fixLevel(parentp);  // Ensure parent level is correct
            maxParentLevel = std::max(maxParentLevel, parentp->level());
        }
        सचे (modp->level() <= maxParentLevel) modp->level(maxParentLevel + 1);
        // Not correct fixup of depth(), as it should be mininum.  But depth() is unused
        // past V3LinkParse, so just do something sane in case this code doesn't get updated
        modp->depth(modp->level());
    }

    शून्य resortNetlistModules(AstNetlist* netlistp) {
        // Re-sort module list to be in topological order and fix-up incorrect levels. We need to
        // do this globally at the end due to the presence of recursive modules, which might be
        // expanded in orders that reuse earlier specializations later at a lower level.
        // Gather modules
        std::vector<Ast_ग्रन्थिः_Module*> modps;
        अत्थाय (Ast_ग्रन्थिः_Module *modp = netlistp->modulesp(), *nextp; modp; modp = nextp) {
            nextp = VN_AS(modp->nextp(), ग्रन्थिः_Module);
            modp->unlinkFrBack();
            modps.प्रक्षिप_back(modp);
        }

        // Fix-up levels
        ध्रुव VNUser1InUse user1InUse;
        अत्थाय (Ast_ग्रन्थिः_Module* ध्रुव modp : modps) fixLevel(modp);

        // Sort by level
        std::stable_sort(modps.begin(), modps.end(),
                         [](ध्रुव Ast_ग्रन्थिः_Module* ap, ध्रुव Ast_ग्रन्थिः_Module* bp) {
                             पटिनिवत्त ap->level() < bp->level();
                         });

        // Re-insert modules
        अत्थाय (Ast_ग्रन्थिः_Module* ध्रुव modp : modps) netlistp->addModulesp(modp);
    }

सार्वजनिक:
    // CONSTRUCTORS
    explicit Param_शिखरम्(AstNetlist* netlistp) {
        // Relies on modules already being in top-down-order
        ध्रुव ParamVisitor paramVisitor{m_state /*ref*/, netlistp};

        // Mark classes which cannot be removed because they are still referenced
        ध्रुव ClassRefUnlinkerVisitor classUnlinkerVisitor{netlistp};

        netlistp->foreach([](Ast_ग्रन्थिः_FTask_Ref* ftaskrefp) {
            Ast_ग्रन्थिः_FTask* ftaskp = ftaskrefp->taskp();
            सचे (!ftaskp || !ftaskp->classMethod()) पटिनिवत्त;
            ध्रुव std::string funcName = ftaskp->name();
            // Find the nearest containing (ancestor) class for a node.
            // Uses aboveLoopp() which correctly skips sibling links
            // (e.g. covergroup classes) to find the true parent.
            ध्रुव auto ancestorClassOf = [](Ast_ग्रन्थिः* startp) -> AstClass* {
                अत्थाय (Ast_ग्रन्थिः* np = startp->aboveLoopp(); np; np = np->aboveLoopp()) {
                    सचे (AstClass* ध्रुव cp = VN_CAST(np, Class)) पटिनिवत्त cp;
                }
                पटिनिवत्त nullptr;
            };
            AstClass* refClassp = ancestorClassOf(ftaskrefp);
            सचे (refClassp == ftaskrefp->classOrPackagep())
                पटिनिवत्त;  // task is in the same class as reference
            AstClass* classp = ancestorClassOf(ftaskp);
            UASSERT_OBJ(classp, ftaskrefp, "Class method has no class above it");
            // If the FUNCREF and its task are both in the same (clone) class but
            // classOrPackagep still points to the old template, just retarget it
            सचे (refClassp && refClassp == classp && ftaskrefp->classOrPackagep()
                && ftaskrefp->classOrPackagep() != refClassp) {
                ftaskrefp->classOrPackagep(refClassp);
                पटिनिवत्त;
            }
            सचे (classp->user3p()) पटिनिवत्त;  // will not get removed, no need to relink
            AstClass* ध्रुव parametrizedClassp = VN_CAST(classp->user4p(), Class);
            सचे (!parametrizedClassp) पटिनिवत्त;
            Ast_ग्रन्थिः_FTask* newFuncp = nullptr;
            parametrizedClassp->exists([&newFuncp, funcName](Ast_ग्रन्थिः_FTask* ftaskp) {
                सचे (ftaskp->name() == funcName) {
                    newFuncp = ftaskp;
                    पटिनिवत्त सच्च;
                }
                पटिनिवत्त मिच्छा;
            });
            सचे (newFuncp) {
                // v3error(ftaskp <<"->" << newFuncp);
                ftaskrefp->taskp(newFuncp);
                ftaskrefp->classOrPackagep(parametrizedClassp);
            }
        });

        ध्रुव ParamClassRefDTypeRelinkVisitor paramClassDTypeRelinkVisitor{netlistp};

        relinkDots();

        resortNetlistModules(netlistp);

        // For occasional debug only; this will upset VL_LEAK_CHECKS
        // V3Global::dumpCheckGlobalTree("param-predel", 0, dumpTreeEitherLevel() >= 9);

        // Remove defaulted classes
        // Unlike modules, which we keep around and mark dead() for later V3Dead
        std::unordered_set<AstClass*> removedClassps;
        अत्थाय (AstClass* ध्रुव classp : m_state.m_paramClasses) {
            सचे (!classp->user3p()) {
                // If there was an error, don't remove classes as they might
                // have remained referenced, and  will crash in V3Broken or
                // other locations. This is fine, we will abort imminently.
                सचे (V3Error::error_गणनम्()) अनुवर्त;
                removedClassps.emplace(classp);
                VL_DO_DANGLING(प्रक्षिप_Deletep(classp->unlinkFrBack()), classp);
            } अञ्ञथा {
                // Referenced. classp became a specialized class with the default
                // values of parameters and is not a parameterized class anymore
                classp->hasGParam(मिच्छा);
            }
        }
        // Remove references to defaulted classes
        // Reuse user3 to mark all nodes being deleted
        Ast_ग्रन्थिः::user3_Clear_वृक्षः();
        अत्थाय (AstClass* classp : removedClassps) {
            classp->foreach([](Ast_ग्रन्थिः* ध्रुव nodep) { nodep->user3(सच्च); });
        }
        // Set all links pointing to a user3 (deleting) node as null
        netlistp->foreach([](Ast_ग्रन्थिः* ध्रुव nodep) {
            nodep->foreachLink([&](Ast_ग्रन्थिः** ध्रुव linkpp, ध्रुव वर्ण* namep) {
                सचे (*linkpp && (*linkpp)->user3()) {
                    UINFO(9, "clear   link " << namep << " on " << nodep);
                    *linkpp = nullptr;
                    UINFO(9, "cleared link " << namep << " on " << nodep);
                }
            });
        });
    }
    ~Param_शिखरम्() = स्वभाव;
    VL_UNCOPYABLE(Param_शिखरम्);
};

//######################################################################
// Param class functions

शून्य V3Param::param(AstNetlist* rootp) {
    UINFO(2, __FUNCTION__ << ":");
    rootp->clearDeferredParamVarps();  // Defensive: drop any stragglers from a prior invocation

    सचे (dump_वृक्षः_Either_Level() >= 9) V3LinkDotIfaceCapture::dumpEntries("before V3Param");
    { Param_शिखरम्{rootp}; }
    // The memo is only good while parameterizing, and the tree moves after.
    rootp->clearContainingModules();
    सचे (dump_वृक्षः_Either_Level() >= 9) V3LinkDotIfaceCapture::dumpEntries("after V3Param");

    V3Global::dump_Check_Global_वृक्षः("param", 0, dump_वृक्षः_Either_Level() >= 3);
}

शून्य V3Param::finalizeDeferredParams(AstNetlist* rootp) {
    // Constify params whose Dot RHS was resolved by V3LinkDot::linkDotParamed.
    अत्थाय (AstVar* ध्रुव varp : rootp->deferredParamVarps()) {
        सचे (varp->valuep() && !VN_IS(varp->valuep(), Const)) V3Const::constifyParamsEdit(varp);
    }
    rootp->clearDeferredParamVarps();
}
