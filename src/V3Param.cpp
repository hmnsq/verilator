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

#समावेश "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#समावेश "V3Param.h"

#समावेश "V3Case.h"
#समावेश "V3Const.h"
#समावेश "V3EmitV.h"
#समावेश "V3HierBlock.h"
#समावेश "V3LinkDotIfaceCapture.h"
#समावेश "V3MemberMap.h"
#समावेश "V3Os.h"
#समावेश "V3Parse.h"
#समावेश "V3Simulate.h"
#समावेश "V3Stats.h"
#समावेश "V3Unroll.h"
#समावेश "V3Width.h"

#समावेश <cctype>
#समावेश <cstdlib>
#समावेश <deque>
#समावेश <map>
#समावेश <memory>
#समावेश <set>
#समावेश <unordered_map>
#समावेश <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

// Walk a class-typed node through typedef/RefDType chains to its ClassRefDType, or null.
अटल AstClassRefDType* class_Ref_DType_Of_गाँठ(Ast_गाँठ* nodep) {
    जबतक (AstTypedef* स्थिर tdp = VN_CAST(nodep, Typedef)) nodep = tdp->subDTypep();
    Ast_गाँठ_DType* स्थिर dtp = VN_CAST(nodep, गाँठ_DType);
    वापस dtp ? VN_CAST(dtp->skipRefOrNullp(), ClassRefDType) : nullptr;
}

//######################################################################
// Hierarchical block and parameter db (modules without parameters are also handled)

श्रेणी ParameterizedHierBlocks final {
    using HierBlockOptsByOrigName = std::multimap<std::string, स्थिर V3HierarchicalBlockOption*>;
    using HierMapIt = HierBlockOptsByOrigName::const_iterator;
    using HierBlockModMap = std::map<स्थिर std::string, Ast_गाँठ_Module*>;
    using ParamConstMap = std::map<स्थिर std::string, std::unique_ptr<AstConst>>;
    using GParamsMap = std::map<स्थिर std::string, AstVar*>;  // key:parameter name value:parameter

    // MEMBERS
    स्थिर bool m_hierSubRun;  // Is in sub-run for hierarchical verilation
    // key:Original module name, value:HiearchyBlockOption*
    // If a module is parameterized, the module is uniquified to overridden parameters.
    // This is why HierBlockOptsByOrigName is multimap.
    HierBlockOptsByOrigName m_hierBlockOptsByOrigName;
    // key:mangled module name, value:AstNodeModule*
    HierBlockModMap m_hierBlockMod;
    // Overridden parameters of the hierarchical block
    std::map<स्थिर V3HierarchicalBlockOption*, ParamConstMap> m_hierParams;
    // Parameter variables of hierarchical blocks
    std::map<स्थिर std::string, GParamsMap> m_modParams;
    // Formals declared without a type, whose type is that of the assigned value
    std::set<स्थिर AstVar*> m_untypedParams;

    // METHODS

खुला:
    ParameterizedHierBlocks(स्थिर V3HierBlockOptSet& hierOpts, AstNetlist* nodep)
        // A type parameter file only describes a child's own types; the final consolidation
        // run has none, but must still substitute the libraries it consumes.
        : m_hierSubRun{!v3Global.opt.hierBlocks().empty() || v3Global.opt.hierChild()} {
        क्रम (स्थिर स्वतः& hierOpt : hierOpts) {
            m_hierBlockOptsByOrigName.emplace(hierOpt.second.origName(), &hierOpt.second);
            स्थिर V3HierarchicalBlockOption::ParamStrMap& params = hierOpt.second.params();
            ParamConstMap& consts = m_hierParams[&hierOpt.second];
            क्रम (V3HierarchicalBlockOption::ParamStrMap::const_iterator pIt = params.begin();
                 pIt != params.end(); ++pIt) {
                std::unique_ptr<AstConst> constp{AstConst::parseParamLiteral(
                    नया File_पंक्ति{File_पंक्ति::builtInFilename()}, pIt->second)};
                UASSERT(constp, pIt->second << " is not a valid parameter literal");
                स्थिर bool inserted = consts.emplace(pIt->first, std::move(constp)).second;
                UASSERT(inserted, pIt->first << " is already added");
            }
            // origName may be already registered, but it's fine.
            m_modParams.जोड़ो({hierOpt.second.origName(), {}});
        }
        क्रम (Ast_गाँठ_Module* modp = nodep->modulesp(); modp;
             modp = VN_AS(modp->nextp(), गाँठ_Module)) {
            अगर (hierOpts.find(modp->prettyName()) != hierOpts.end()) {
                m_hierBlockMod.emplace(modp->name(), modp);
            }
            // Recursive hierarchical module may change its name, so we have to match its origName.
            // Collect values from recursive cloned module as parameters in the top module could be
            // overridden.
            स्थिर string actualModName = modp->recursiveClone() ? modp->origName() : modp->name();
            स्थिर स्वतः defParamIt = m_modParams.find(actualModName);
            अगर (defParamIt != m_modParams.end()) {
                // modp is the original of parameterized hierarchical block
                क्रम (Ast_गाँठ* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    अगर (AstVar* स्थिर varp = VN_CAST(stmtp, Var)) {
                        अगर (varp->isGParam()) defParamIt->second.emplace(varp->name(), varp);
                        // Recorded before V3Param resolves the type from the assigned value.
                        स्थिर AstBasicDType* स्थिर dtypep
                            = VN_CAST(varp->childDTypep(), BasicDType);
                        अगर (varp->isGParam() && dtypep && dtypep->implicit()) {
                            m_untypedParams.emplace(varp);
                        }
                    }
                }
            }
        }
    }
    bool hierSubRun() स्थिर { वापस m_hierSubRun; }
    bool isHierBlock(स्थिर string& origName) स्थिर {
        वापस m_hierBlockOptsByOrigName.find(origName) != m_hierBlockOptsByOrigName.end();
    }
    Ast_गाँठ_Module* findByParams(स्थिर string& origName, AstPin* firstPinp,
                                स्थिर Ast_गाँठ_Module* modp) {
        UASSERT(isHierBlock(origName), origName << " is not hierarchical block");
        // This module is a hierarchical block. Need to replace it by the --lib-create wrapper.
        स्थिर std::pair<HierMapIt, HierMapIt> candidates
            = m_hierBlockOptsByOrigName.equal_range(origName);
        स्थिर स्वतः paramsIt = m_modParams.find(origName);
        UASSERT_OBJ(paramsIt != m_modParams.end(), modp, origName << " must be registered");
        // An untyped parameter takes the type of its value, but libraries are distinguished
        // only by parameter values. This is a pin whose value equals that of a library or the
        // default, but whose type differs.
        स्थिर AstPin* typeMismatchPinp = nullptr;
        HierMapIt hierIt;
        क्रम (hierIt = candidates.first; hierIt != candidates.second; ++hierIt) {
            bool found = true;
            माप_प्रकार paramIdx = 0;
            स्थिर ParamConstMap& params = m_hierParams[hierIt->second];
            UASSERT(params.आकार() == hierIt->second->params().आकार(), "not match");
            क्रम (AstPin* pinp = firstPinp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                अगर (!pinp->exprp()) जारी;
                अगर (स्थिर AstVar* स्थिर modvarp = pinp->modVarp()) {
                    AstConst* स्थिर constp = VN_AS(pinp->exprp(), Const);
                    UASSERT_OBJ(constp, pinp,
                                "parameter for a hierarchical block must have been constified");
                    स्थिर स्वतः paramIt = paramsIt->second.find(modvarp->name());
                    UASSERT_OBJ(paramIt != paramsIt->second.end(), modvarp, "must be registered");
                    स्थिर bool untyped = m_untypedParams.गिनती(paramIt->second);
                    AstConst* स्थिर defValuep = VN_CAST(paramIt->second->valuep(), Const);
                    अगर (defValuep && areSame(constp, defValuep)) {
                        अगर (!untyped || constp->same_मूल्य_Type(defValuep)) {
                            UINFO(5, "Setting default value of " << constp << " to " << modvarp);
                            जारी;  // Skip this parameter because setting the same value
                        }
                        typeMismatchPinp = pinp;
                    }
                    स्थिर स्वतः pIt = vlstd::as_const(params).find(modvarp->name());
                    UINFO(5, "Comparing " << modvarp->name() << " " << constp);
                    अगर (pIt == params.end() || paramIdx >= params.आकार()
                        || !areSame(constp, pIt->second.लो())) {
                        found = false;
                        अवरोध;
                    }
                    अगर (untyped && !constp->same_मूल्य_Type(pIt->second.लो())) {
                        typeMismatchPinp = pinp;
                        found = false;
                        अवरोध;
                    }
                    UINFO(5, "Matched " << modvarp->name() << " " << constp << " and "
                                        << pIt->second.लो());
                    ++paramIdx;
                }
            }
            अगर (found && paramIdx == hierIt->second->params().आकार()) अवरोध;
        }
        अगर (hierIt == candidates.second && typeMismatchPinp) {
            typeMismatchPinp->v3warn(
                E_UNSUPPORTED, "Unsupported: Untyped parameter "
                                   << typeMismatchPinp->modVarp()->prettyNameQ()
                                   << " of hierarchical block " << Ast_गाँठ::prettyNameQ(origName)
                                   << " given equal values of different width or"
                                      " signedness");
            वापस nullptr;
        }
        UASSERT_OBJ(hierIt != candidates.second, firstPinp, "No --lib-create wrapper found");
        // parameter settings will be removed in the bottom of caller visitCell().
        स्थिर HierBlockModMap::const_iterator modIt
            = m_hierBlockMod.find(hierIt->second->mangledName());
        UASSERT_OBJ(modIt != m_hierBlockMod.end(), firstPinp,
                    hierIt->second->mangledName() << " is not found");

        स्थिर स्वतः it = vlstd::as_const(m_hierBlockMod).find(hierIt->second->mangledName());
        अगर (it == m_hierBlockMod.end()) वापस nullptr;
        वापस it->second;
    }
    अटल bool areSame(AstConst* pinValuep, AstConst* hierOptParamp) {
        अगर (pinValuep->isString()) {
            वापस pinValuep->अंक().toString() == hierOptParamp->अंक().toString();
        }

        अगर (hierOptParamp->isDouble()) {
            द्विगुन var;
            अगर (pinValuep->isDouble()) {
                var = pinValuep->अंक().toDouble();
            } अन्यथा {  // Cast from integer to real
                V3_संख्या var_अंक{pinValuep, V3_संख्या::Double{}, 0.0};
                var_अंक.opIToRD(pinValuep->अंक());
                var = var_अंक.toDouble();
            }
            वापस V3_संख्या::epsilonEqual(var, hierOptParamp->अंक().toDouble());
        } अन्यथा {  // Now integer type is assumed
            // Bitwidth of hierOptParamp is accurate because V3Width already calculated in the
            // previous run. Bitwidth of pinValuep is before width analysis, so pinValuep is casted
            // to hierOptParamp width.
            V3_संख्या var_अंक{pinValuep, hierOptParamp->अंक().width()};
            अगर (pinValuep->isDouble()) {  // Need to cast to int
                // Parameter is actually an integral type, but passed value is floating point.
                // Conversion from real to integer uses rounding in V3Width.cpp
                var_अंक.opRToIRoundS(pinValuep->अंक());
            } अन्यथा अगर (pinValuep->isSigned()) {
                var_अंक.opExtendS(pinValuep->अंक(), pinValuep->अंक().width());
            } अन्यथा {
                var_अंक.opAssign(pinValuep->अंक());
            }
            V3_संख्या isEq{pinValuep, 1};
            isEq.opEq(var_अंक, hierOptParamp->अंक());
            वापस isEq.isNeqZero();
        }
    }
};

//######################################################################
// Remove parameters from cells and build new modules

श्रेणी ParamProcessor final {
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
    स्थिर VNUser2InUse m_inuser2;
    स्थिर VNUser3InUse m_inuser3;
    स्थिर VNUser4InUse m_inuser4;
    // User1 used by constant function simulations

    // TYPES
    // Note may have duplicate entries
    using IfaceRefRefs = std::deque<std::pair<AstIfaceRefDType*, AstIfaceRefDType*>>;

    // STATE
    using CloneMap = std::unordered_map<स्थिर Ast_गाँठ*, Ast_गाँठ*>;
    काष्ठा ModInfo final {
        Ast_गाँठ_Module* स्थिर m_modp;  // Module with specified name
        CloneMap m_cloneMap;  // Map of old-varp -> new cloned varp
        explicit ModInfo(Ast_गाँठ_Module* modp)
            : m_modp{modp} {}
    };
    std::map<स्थिर std::string, ModInfo> m_modNameMap;  // Hash of created module flavors by name

    std::map<स्थिर std::string, std::string>
        m_longMap;  // Hash of very long names to unique identity number
    std::set<std::string> m_longSuffixes;  // Digest suffixes used by m_longMap

    // All module names that are loaded from source code
    // Generated modules by this visitor is not included
    VStringSet m_allModuleNames;

    CloneMap m_originalParams;  // Map between parameters of copied parameteized classes and their
                                // original nodes

    std::map<स्थिर std::string, std::string> m_मूल्य_Names;  // Parameter value text to its name
    std::set<std::string> m_मूल्य_Suffixes;  // Digest suffixes used by m_valueNames
    // Number of digest hex digits in a name. With 8 digits, a collision between names made in
    // different runs, such as those of different hierarchical blocks, is unlikely. Within one
    // run, digestSuffix resolves a collision by lengthening the later text's suffix, which can
    // then differ between runs.
    अटल constexpr std::string::आकार_type DIGEST_DIGITS = 8;

    स्थिर Ast_गाँठ_Module* m_modp = nullptr;  // Current module being processed

    // Database to get lib-create wrapper that matches parameters in hierarchical Verilation
    ParameterizedHierBlocks m_hierBlocks;
    // Default parameter values key:parameter name, value:default value (can be nullptr)
    using Default_मूल्य_Map = std::map<std::string, Ast_गाँठ*>;
    // Default parameter values of hierarchical blocks
    std::map<Ast_गाँठ_Module*, Default_मूल्य_Map> m_default_Parameter_मूल्यावली;
    // Module -> a defparam in it or below it, or nullptr (see nestedDefparamp)
    std::map<स्थिर Ast_गाँठ_Module*, स्थिर AstPin*> m_nestedDefparamps;
    VNDeleter m_deleter;  // Used to delay deletion of nodes
    // Class default type paramater dependencies
    std::vector<std::pair<AstParamTypeDType*, पूर्णांक>> m_classTypeParams;
    std::unordered_map<AstParamTypeDType*, पूर्णांक> m_param_सूचकांक;

    // Guard against infinite recursion in classTypeMatchesDefaultClone slow path
    std::unordered_set<स्थिर AstClass*> m_defaultCloneInProgress;

    std::vector<AstClass*> m_specializedClassps;  // Classes specialized since last drained

    // member names cached for fast lookup
    VMemberMap m_memberMap;

    // METHODS

    अटल व्योम makeSmallNames(Ast_गाँठ_Module* modp) {
        std::vector<पूर्णांक> usedLetter;
        usedLetter.resize(256);
        // Pass 1, assign first letter to each gparam's name
        क्रम (Ast_गाँठ* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            अगर (AstVar* स्थिर varp = VN_CAST(stmtp, Var)) {
                अगर (varp->isGParam() || varp->isIfaceRef()) {
                    अक्षर ch = varp->name()[0];
                    ch = std::बड़े(ch);
                    अगर (ch < 'A' || ch > 'Z') ch = 'Z';
                    varp->user3(usedLetter[static_cast<पूर्णांक>(ch)] * 256 + ch);
                    usedLetter[static_cast<पूर्णांक>(ch)]++;
                }
            } अन्यथा अगर (AstParamTypeDType* स्थिर typep = VN_CAST(stmtp, ParamTypeDType)) {
                स्थिर अक्षर ch = 'T';
                typep->user3(usedLetter[static_cast<पूर्णांक>(ch)] * 256 + ch);
                usedLetter[static_cast<पूर्णांक>(ch)]++;
            }
        }
    }
    अटल string paramSmallName(Ast_गाँठ_Module* modp, Ast_गाँठ* varp) {
        अगर (varp->user3() <= 1) makeSmallNames(modp);
        पूर्णांक सूचकांक_2 = varp->user3() / 256;
        स्थिर अक्षर ch = varp->user3() & 255;
        string st = cvtToStr(ch);
        जबतक (सूचकांक_2) {
            st += cvtToStr(static_cast<अक्षर>((सूचकांक_2 % 25) + 'A'));
            सूचकांक_2 /= 26;
        }
        वापस st;
    }

    अटल bool is_Aggregate_Param_मूल्य(स्थिर Ast_गाँठ* nodep) {
        वापस VN_IS(nodep, ConsArray) || VN_IS(nodep, ConsStruct);
    }

    अटल string param_मूल्य_String(स्थिर Ast_गाँठ* nodep) {
        अगर (स्थिर AstRefDType* स्थिर refp = VN_CAST(nodep, RefDType)) {
            nodep = refp->skipRefToNonRefp();
        }
        string कुंजी = nodep->name();
        अगर (स्थिर AstIfaceRefDType* स्थिर ifrtp = VN_CAST(nodep, IfaceRefDType)) {
            अगर (ifrtp->cellp() && ifrtp->cellp()->modp()) {
                कुंजी = ifrtp->cellp()->modp()->name();
            } अन्यथा अगर (ifrtp->ifacep()) {
                कुंजी = ifrtp->ifacep()->name();
            } अन्यथा {
                nodep->v3fatalSrc("Can't parameterize interface without module name");
            }
        } अन्यथा अगर (स्थिर Ast_गाँठ_UOr_Struct_DType* स्थिर dtypep
                   = VN_CAST(nodep, गाँठ_UOr_Struct_DType)) {
            कुंजी += " ";
            कुंजी += dtypep->verilogKwd();
            कुंजी += " {";
            क्रम (स्थिर Ast_गाँठ* memberp = dtypep->membersp(); memberp;
                 memberp = memberp->nextp()) {
                कुंजी += param_मूल्य_String(memberp);
                कुंजी += ";";
            }
            कुंजी += "}";
        } अन्यथा अगर (स्थिर AstMemberDType* स्थिर dtypep = VN_CAST(nodep, MemberDType)) {
            कुंजी += " ";
            कुंजी += param_मूल्य_String(dtypep->subDTypep());
        } अन्यथा अगर (स्थिर AstBasicDType* स्थिर dtypep = VN_CAST(nodep, BasicDType)) {
            अगर (dtypep->isSigned()) कुंजी += " signed";
            अगर (dtypep->isRanged()) {
                कुंजी += "[" + cvtToStr(dtypep->बाँए()) + ":" + cvtToStr(dtypep->दाँए()) + "]";
            }
        } अन्यथा अगर (स्थिर AstPackArrayDType* स्थिर dtypep = VN_CAST(nodep, PackArrayDType)) {
            कुंजी += "[";
            कुंजी += cvtToStr(dtypep->बाँए());
            कुंजी += ":";
            कुंजी += cvtToStr(dtypep->दाँए());
            कुंजी += "] ";
            कुंजी += param_मूल्य_String(dtypep->subDTypep());
        } अन्यथा अगर (स्थिर AstPattern* स्थिर patp = VN_CAST(nodep, Pattern)) {
            // A parameter override pattern, keyed before the specialized module types it
            कुंजी += "'{";
            क्रम (स्थिर AstPatMember* memp = VN_AS(patp->itemsp(), PatMember); memp;
                 memp = VN_AS(memp->nextp(), PatMember)) {
                अगर (memp->isDefault()) कुंजी += "default";
                अगर (स्थिर AstText* स्थिर textp = VN_CAST(memp->keyp(), Text)) {
                    कुंजी += textp->text();
                } अन्यथा अगर (memp->keyp()) {
                    कुंजी += param_मूल्य_String(memp->keyp());
                }
                कुंजी += ":";
                अगर (memp->repp()) कुंजी += param_मूल्य_String(memp->repp()) + "x";
                क्रम (स्थिर Ast_गाँठ_व्यंजक* valuep = memp->lhssp(); valuep;
                     valuep = VN_AS(valuep->nextp(), गाँठ_व्यंजक)) {
                    कुंजी += param_मूल्य_String(valuep) + ";";
                }
                कुंजी += ",";
            }
            कुंजी += "}";
        } अन्यथा अगर (स्थिर AstConsArray* स्थिर consArrayp = VN_CAST(nodep, ConsArray)) {
            // Include the indices and the default, as with a default the items may be sparse
            कुंजी += "{";
            क्रम (स्थिर AstConsArrayItem* itemp = consArrayp->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), ConsArrayItem)) {
                कुंजी += cvtToStr(itemp->सूचकांक_2()) + ":" + param_मूल्य_String(itemp->valuep()) + ",";
            }
            अगर (consArrayp->defaultp()) {
                कुंजी += "default:" + param_मूल्य_String(consArrayp->defaultp()) + ",";
            }
            कुंजी += "}";
        } अन्यथा अगर (स्थिर AstConsStruct* स्थिर consp = VN_CAST(nodep, ConsStruct)) {
            कुंजी += "{";
            क्रम (स्थिर AstConsStructItem* itemp = consp->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), ConsStructItem)) {
                कुंजी += param_मूल्य_String(itemp->valuep());
                कुंजी += ",";
            }
            कुंजी += "}";
        } अन्यथा अगर (स्थिर AstClassRefDType* स्थिर classRefp = VN_CAST(nodep, ClassRefDType)) {
            // For parameterized class types, use the original class name (without specialization
            // suffix) plus the actual type parameter values. This ensures equivalent class types
            // get the same string representation regardless of which AST node is used.
            अगर (classRefp->classp()) {
                स्थिर string& className = classRefp->classp()->name();
                स्थिर string& origName = classRefp->classp()->origName();
                स्थिर bool isSpecialized = (className != origName);
                UINFO(9, "paramValueString ClassRefDType: name="
                             << className << " origName=" << origName
                             << " isSpecialized=" << isSpecialized
                             << " hasParams=" << (classRefp->paramsp() ? "Y" : "N")
                             << " classHasGParam=" << classRefp->classp()->hasGParam());

                अगर (classRefp->paramsp()) {
                    // ClassRefDType should have been deparameterized (paramsp
                    // consumed) before cellPinCleanup calls paramValueString.
                    classRefp->v3fatalSrc(  // LCOV_EXCL_LINE
                        "ClassRefDType still has paramsp in paramValueString");
                } अन्यथा अगर (isSpecialized) {
                    // Already specialized class (e.g., c1__Tz1_TBz1) - use full name
                    // This ensures different specializations are distinguished
                    कुंजी = className;
                } अन्यथा {
                    // Unspecialized class with no params - use origName
                    कुंजी = origName;
                }
            } अन्यथा {
                // classp() should always be set; unresolved class refs
                // would have errored in LinkDot.
                classRefp->v3fatalSrc(  // LCOV_EXCL_LINE
                    "ClassRefDType has null classp in paramValueString");
            }
        } अन्यथा अगर (स्थिर AstUnpackArrayDType* स्थिर dtypep = VN_CAST(nodep, UnpackArrayDType)) {
            // Name containers by their elements as above, not by prettyDTypeName(), which names
            // classes by parameter values that may be yet to be resolved
            कुंजी = paramElemString(dtypep->subDTypep()) + "$" + cvtToStr(dtypep->declRange());
        } अन्यथा अगर (स्थिर AstQueueDType* स्थिर dtypep = VN_CAST(nodep, QueueDType)) {
            कुंजी = paramElemString(dtypep->subDTypep()) + "$[$";
            अगर (dtypep->boundConst()) कुंजी += ":" + cvtToStr(dtypep->boundConst());
            कुंजी += "]";
        } अन्यथा अगर (स्थिर AstDynArrayDType* स्थिर dtypep = VN_CAST(nodep, DynArrayDType)) {
            कुंजी = paramElemString(dtypep->subDTypep()) + "$[]";
        } अन्यथा अगर (स्थिर AstAssocArrayDType* स्थिर dtypep = VN_CAST(nodep, AssocArrayDType)) {
            कुंजी = paramElemString(dtypep->subDTypep()) + "$["
                  + paramElemString(dtypep->कुंजी_DTypep()) + "]";
        } अन्यथा अगर (स्थिर AstWildcardArrayDType* स्थिर dtypep
                   = VN_CAST(nodep, WildcardArrayDType)) {
            कुंजी = paramElemString(dtypep->subDTypep()) + "$[*]";
        } अन्यथा अगर (स्थिर AstEnumDType* स्थिर dtypep = VN_CAST(nodep, EnumDType)) {
            // With the items, as prettyDTypeName() omits them, yet a specialization of a class
            // may declare an enumeration of the same name with other values
            कुंजी += " enum {";
            क्रम (स्थिर AstEnumItem* itemp = dtypep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), EnumItem)) {
                स्थिर AstConst* स्थिर constp = VN_CAST(itemp->valuep(), Const);
                कुंजी += itemp->name() + "=" + (constp ? constp->अंक().ascii() : "?") + ";";
            }
            कुंजी += "}";
        } अन्यथा अगर (स्थिर Ast_गाँठ_DType* स्थिर dtypep = VN_CAST(nodep, गाँठ_DType)) {
            कुंजी += dtypep->prettyDTypeName(true);
        }
        UASSERT_OBJ(!कुंजी.empty(), nodep, "Parameter yielded no value string");
        वापस कुंजी;
    }
    // As paramValueString(), for the element or key type of a container
    अटल string paramElemString(स्थिर Ast_गाँठ_DType* dtypep) {
        // Unlike at the top of a value, a class may be yet to be specialized, so just name it
        स्थिर AstClassRefDType* स्थिर classRefp
            = VN_CAST(dtypep->skipRefToNonRefp(), ClassRefDType);
        अगर (classRefp && classRefp->paramsp()) वापस classRefp->prettyDTypeName(true);
        वापस param_मूल्य_String(dtypep);
    }

    // Return a name suffix for 'text' from its SHA-512 digest. Hierarchical blocks are
    // Verilated in separate runs, where a counter would restart, but the digest is the same in
    // every run. Use the shortest digest prefix of at least DIGEST_DIGITS digits that is not
    // already in 'usedr', and add it to 'usedr'.
    अटल string digestSuffix(स्थिर string& text, std::set<string>& usedr) {
        स्थिर string षष्ठ = VHashSha512{text}.digestHex();
        // Force collisions of the prefixes -- for testing only
        string::आकार_type digits = v3Global.opt.debugCollision() ? 1 : DIGEST_DIGITS;
        जबतक (digits < षष्ठ.आकार() && !usedr.जोड़ो(षष्ठ.substr(0, digits)).second) ++digits;
        वापस षष्ठ.substr(0, digits);
    }
    string param_मूल्य_संख्या(Ast_गाँठ* nodep) {
        // All call sites resolve through skipRefToNonRefp() or pass non-DType
        // nodes, so nodep should never be a bare RefDType here.
        अगर (VN_IS(nodep, RefDType)) {  // LCOV_EXCL_LINE
            nodep->v3fatalSrc("Unexpected RefDType in paramValueNumber");  // LCOV_EXCL_LINE
        }
        // Name the value by its text, which is the same for equal values or types in every run.
        // V3Hasher is unsuitable, as it hashes node pointers, which can differ for equal types.
        // V3Hash of a string is unsuitable, as std::hash varies between C++ libraries.
        स्थिर string text = param_मूल्य_String(nodep);
        स्थिर स्वतः pair = m_मूल्य_Names.emplace(text, "");
        अगर (pair.second) pair.first->second = "z" + digestSuffix(text, m_मूल्य_Suffixes);
        वापस pair.first->second;
    }
    string moduleCalcName(स्थिर Ast_गाँठ_Module* srcModp, स्थिर string& longname) {
        string newname = longname;
        अगर (longname.लंबाई() > 30) {
            स्थिर स्वतः pair = m_longMap.emplace(longname, "");
            अगर (pair.second) {
                // We use all upper case above, so lower here can't conflict
                pair.first->second
                    = srcModp->name() + "__pi" + digestSuffix(longname, m_longSuffixes);
            }
            newname = pair.first->second;
        }
        UINFO(4, "Name: " << srcModp->name() << "->" << longname << "->" << newname);
        वापस newname;
    }
    अटल Ast_गाँठ_DType* arraySubDTypep(Ast_गाँठ_DType* nodep) {
        // If an unpacked array, return the subDTypep under it
        अगर (स्थिर AstUnpackArrayDType* स्थिर adtypep = VN_CAST(nodep, UnpackArrayDType)) {
            वापस adtypep->subDTypep();
        }
        // We have not resolved parameter of the child yet, so still
        // have BracketArrayDType's. We'll presume it'll end up as assignment
        // compatible (or V3Width will complain).
        अगर (स्थिर AstBracketArrayDType* स्थिर adtypep = VN_CAST(nodep, BracketArrayDType)) {
            वापस adtypep->subDTypep();
        }
        वापस nullptr;
    }
    // Peel all unpacked-array layers to reach the innermost subDTypep.
    // Used for multi-dim iface arrays where port dtype is nested UnpackArrayDType.
    अटल Ast_गाँठ_DType* arraySubDTypeDeepp(Ast_गाँठ_DType* nodep) {
        जबतक (Ast_गाँठ_DType* स्थिर subp = arraySubDTypep(nodep)) nodep = subp;
        वापस nodep;
    }
    अटल bool isString(Ast_गाँठ_DType* nodep) {
        अगर (AstBasicDType* स्थिर basicp = VN_CAST(nodep->skipRefToNonRefp(), BasicDType))
            वापस basicp->isString();
        वापस false;
    }
    व्योम collectPins(CloneMap* clonemapp, Ast_गाँठ_Module* modp, bool originalIsCopy) {
        // Grab all I/O so we can remap our pins later
        क्रम (Ast_गाँठ* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            स्थिर Ast_गाँठ* originalParamp = nullptr;
            अगर (AstVar* स्थिर varp = VN_CAST(stmtp, Var)) {
                अगर (varp->isIO() || varp->isGParam() || varp->isIfaceRef()) {
                    // Cloning saved a pointer to the new node for us, so just follow that link.
                    originalParamp = varp->clonep();
                }
            } अन्यथा अगर (AstParamTypeDType* स्थिर ptp = VN_CAST(stmtp, ParamTypeDType)) {
                अगर (ptp->isGParam()) originalParamp = ptp->clonep();
            }
            अगर (originalIsCopy) originalParamp = m_originalParams[originalParamp];
            अगर (originalParamp) clonemapp->emplace(originalParamp, stmtp);
        }
    }
    व्योम relinkPins(स्थिर CloneMap* clonemapp, AstPin* startpinp) {
        क्रम (AstPin* pinp = startpinp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            अगर (pinp->modVarp()) {
                // Find it in the clone structure
                // UINFO(8,"Clone find 0x"<<hex<<(uint32_t)pinp->modVarp());
                स्थिर स्वतः cloneiter = clonemapp->find(pinp->modVarp());
                UASSERT_OBJ(cloneiter != clonemapp->end(), pinp,
                            "Couldn't find pin in clone list");
                pinp->modVarp(VN_AS(cloneiter->second, Var));
            } अन्यथा अगर (pinp->modPTypep()) {
                स्थिर स्वतः cloneiter = clonemapp->find(pinp->modPTypep());
                UASSERT_OBJ(cloneiter != clonemapp->end(), pinp,
                            "Couldn't find pin in clone list");
                pinp->modPTypep(VN_AS(cloneiter->second, ParamTypeDType));
            } अन्यथा {
                pinp->v3fatalSrc("Not linked?");
            }
        }
    }
    व्योम relinkPinsByName(AstPin* startpinp, Ast_गाँठ_Module* modp) {
        std::map<स्थिर string, AstVar*> nameToPin;
        क्रम (Ast_गाँठ* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            अगर (AstVar* स्थिर varp = VN_CAST(stmtp, Var)) {
                अगर (varp->isIO() || varp->isGParam() || varp->isIfaceRef()) {
                    nameToPin.emplace(varp->name(), varp);
                }
            }
        }
        क्रम (AstPin* pinp = startpinp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            अगर (स्थिर AstVar* स्थिर varp = pinp->modVarp()) {
                स्थिर स्वतः varIt = vlstd::as_const(nameToPin).find(varp->name());
                UASSERT_OBJ(varIt != nameToPin.end(), varp,
                            "Not found in " << modp->prettyNameQ());
                pinp->modVarp(varIt->second);
            }
        }
    }
    अटल bool hasDescendantDefparams(स्थिर Ast_गाँठ_Module* modp,
                                       std::set<स्थिर Ast_गाँठ_Module*>& visited) {
        अगर (!visited.जोड़ो(modp).second) वापस false;
        क्रम (Ast_गाँठ* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstCell* स्थिर cellp = VN_CAST(stmtp, Cell);
            अगर (!cellp) जारी;
            क्रम (AstPin* pinp = cellp->paramsp(); pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                अगर (!pinp->paramPath().empty()) {
                    visited.erase(modp);
                    वापस true;
                }
            }
            अगर (Ast_गाँठ_Module* स्थिर childModp = cellp->modp()) {
                अगर (hasDescendantDefparams(childModp, visited)) {
                    visited.erase(modp);
                    वापस true;
                }
            }
        }
        visited.erase(modp);
        वापस false;
    }
    अटल bool hasDescendantDefparams(स्थिर Ast_गाँठ_Module* modp) {
        std::set<स्थिर Ast_गाँठ_Module*> visited;
        वापस hasDescendantDefparams(modp, visited);
    }
    // Return a defparam in the module or below it. Unlike hasDescendantDefparams, also look
    // inside generate constructs. Memoized, so each module is searched once.
    स्थिर AstPin* nestedDefparamp(स्थिर Ast_गाँठ_Module* modp) {
        स्थिर स्वतः pair = m_nestedDefparamps.emplace(modp, nullptr);
        अगर (!pair.second) वापस pair.first->second;  // Searched, or being searched
        स्थिर AstPin* foundp = nullptr;
        modp->exists([&](स्थिर AstCell* cellp) {
            क्रम (स्थिर AstPin* pinp = cellp->paramsp(); pinp && !foundp;
                 pinp = VN_AS(pinp->nextp(), Pin)) {
                अगर (!pinp->paramPath().empty()) foundp = pinp;
            }
            अगर (!foundp && cellp->modp()) foundp = nestedDefparamp(cellp->modp());
            वापस foundp != nullptr;
        });
        pair.first->second = foundp;
        वापस foundp;
    }
    // Check if parameter setting during instantiation is simple enough for hierarchical Verilation
    व्योम checkSupportedParam(Ast_गाँठ_Module* modp, AstPin* pinp) स्थिर {
        // ConsArray is not supported because that can not be set via -G
        // option.
        अगर (pinp->modVarp()) {
            bool supported = false;
            अगर (स्थिर AstConst* स्थिर constp = VN_CAST(pinp->exprp(), Const)) {
                supported = true;
                अगर (constp->अंक().isString()
                    && !V3HierBlock::stringParamPassable(constp->अंक().toString())) {
                    pinp->v3warn(E_UNSUPPORTED, "Unsupported: String value of hierarchical block"
                                                " parameter with newline, double quote, '/*',"
                                                " or whitespace before '//'");
                }
            }
            अगर (!supported) {
                pinp->v3error(
                    Ast_गाँठ::prettyNameQ(modp->origName())
                    << " has hier_block metacomment, hierarchical Verilation"
                    << " supports only integer/floating point/string and type param parameters");
            }
        } अन्यथा अगर (स्थिर AstParamTypeDType* स्थिर typep = pinp->modPTypep()) {
            // Libraries are found by comparing values only
            pinp->v3warn(E_UNSUPPORTED, "Unsupported: Setting type parameter "
                                            << typep->prettyNameQ() << " of hierarchical block "
                                            << Ast_गाँठ::prettyNameQ(modp->origName()));
        }
    }
    bool moduleExists(स्थिर string& modName) स्थिर {
        अगर (m_allModuleNames.find(modName) != m_allModuleNames.end()) वापस true;
        अगर (m_modNameMap.find(modName) != m_modNameMap.end()) वापस true;
        वापस false;
    }

    string parameterizedHierBlockName(Ast_गाँठ_Module* modp, AstPin* paramPinsp) {
        // Create a unique name in the following steps
        //  - Make a long name that includes all parameters, that appear
        //    in the alphabetical order.
        //  - Hash the long name to get valid Verilog symbol
        UASSERT_OBJ(modp->hierBlock(), modp, "should be used for hierarchical block");

        std::map<string, Ast_गाँठ*> pins;

        AstPin* pinp = paramPinsp;
        जबतक (pinp) {
            checkSupportedParam(modp, pinp);
            अगर (स्थिर AstVar* स्थिर varp = pinp->modVarp()) {
                अगर (!pinp->exprp()) जारी;
                अगर (varp->isGParam()) pins.emplace(varp->name(), pinp->exprp());
            } अन्यथा अगर (VN_IS(pinp->exprp(), BasicDType) || VN_IS(pinp->exprp(), गाँठ_DType)) {
                pins.emplace(pinp->name(), pinp->exprp());
            }
            pinp = VN_AS(pinp->nextp(), Pin);
        }

        स्थिर स्वतः pair = m_default_Parameter_मूल्यावली.emplace(
            std::piecewise_construct, std::forward_as_tuple(modp), std::forward_as_tuple());
        अगर (pair.second) {  // Not cached yet, so check parameters
            // A separately compiled library cannot receive defparams aimed into its body
            अगर (स्थिर AstPin* स्थिर defparamp = nestedDefparamp(modp)) {
                defparamp->v3warn(E_UNSUPPORTED, "Unsupported: defparam inside hierarchical block "
                                                     << Ast_गाँठ::prettyNameQ(modp->origName()));
            }
            // Using map with key=string so that we can scan it in deterministic order
            Default_मूल्य_Map params;
            क्रम (Ast_गाँठ* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                अगर (AstVar* स्थिर varp = VN_CAST(stmtp, Var)) {
                    अगर (varp->isGParam()) {
                        AstConst* स्थिर constp = VN_CAST(varp->valuep(), Const);
                        // constp can be nullptr if the parameter is not used to instantiate sub
                        // module. varp->valuep() is not constified yet in the case.
                        // nullptr means that the parameter is using some default value.
                        params.emplace(varp->name(), constp);
                    }
                } अन्यथा अगर (AstParamTypeDType* स्थिर p = VN_CAST(stmtp, ParamTypeDType)) {
                    Ast_गाँठ* स्थिर dtypep = static_cast<Ast_गाँठ*>(p->skipRefp());
                    params.emplace(p->name(), dtypep);
                }
            }
            pair.first->second = std::move(params);
        }
        स्थिर स्वतः paramsIt = pair.first;
        अगर (paramsIt->second.empty()) वापस modp->origName();  // modp has no parameter

        string longname = modp->origName();
        क्रम (स्वतः&& default_मूल्य : paramsIt->second) {
            स्थिर स्वतः pinIt = pins.find(default_मूल्य.first);
            // If the pin does not have a value assigned, use the default one.
            स्थिर Ast_गाँठ* स्थिर nodep = pinIt == pins.end() ? default_मूल्य.second : pinIt->second;
            // This longname is not valid as verilog symbol, but ok, because it will be hashed
            longname += "_" + default_मूल्य.first + "=";
            // constp can be nullptr

            अगर (स्थिर AstConst* स्थिर p = VN_CAST(nodep, Const)) {
                // Treat modules parameterized with the same values but with different type as the
                // same.
                longname += p->अंक().ascii(false);
            } अन्यथा अगर (nodep) {
                std::stringstream type;
                V3EmitV::verilog_For_वृक्ष(nodep, type);
                longname += type.str();
            }
        }
        UINFO(9, "       module params longname: " << longname);

        स्थिर स्वतः iter = m_longMap.find(longname);
        अगर (iter != m_longMap.end()) वापस iter->second;  // Already calculated

        VHashSha512 hash;
        // Calculate hash using longname
        // The hash is used as the module suffix to find a module name that is unique in the design
        hash.जोड़ो(longname);
        जबतक (true) {
            // Copy VHashSha512 just in case of hash collision
            VHashSha512 hashStrGen = hash;
            // Hex string must be a safe suffix for any symbol
            स्थिर string hashStr = hashStrGen.digestHex();
            क्रम (string::आकार_type i = 1; i < hashStr.आकार(); ++i) {
                string newName = modp->origName();
                // Don't use '__' not to be encoded when this module is loaded later by Verilator
                अगर (newName.at(newName.आकार() - 1) != '_') newName += '_';
                newName += hashStr.substr(0, i);
                अगर (!moduleExists(newName)) {
                    m_longMap.emplace(longname, newName);
                    वापस newName;
                }
            }
            // Hash collision. maybe just v3error is practically enough
            hash.जोड़ो(V3Os::trueRandom(64));
        }
    }
    व्योम replaceRefsRecurse(Ast_गाँठ* स्थिर nodep, स्थिर AstClass* स्थिर oldClassp,
                            AstClass* स्थिर newClassp) {
        // Self references linked in the first pass of V3LinkDot.cpp should point to the default
        // instance.
        अगर (AstClassRefDType* स्थिर classRefp = VN_CAST(nodep, ClassRefDType)) {
            अगर (classRefp->classp() == oldClassp) classRefp->classp(newClassp);
        } अन्यथा अगर (AstClassOrPackageRef* स्थिर classRefp = VN_CAST(nodep, ClassOrPackageRef)) {
            अगर (classRefp->classOrPackageSkipp() == oldClassp)
                classRefp->classOrPackagep(newClassp);
        } अन्यथा अगर (AstTypedef* स्थिर typedefp = VN_CAST(nodep, Typedef)) {
            // Update typedefs that refer to the old class to point to the new class
            अगर (typedefp->subDTypep()) {
                अगर (AstClassRefDType* स्थिर classRefp
                    = VN_CAST(typedefp->subDTypep(), ClassRefDType)) {
                    अगर (classRefp->classp() == oldClassp) { classRefp->classp(newClassp); }
                }
            }
        } अन्यथा अगर (Ast_गाँठ_FTask_Ref* स्थिर ftaskRefp = VN_CAST(nodep, गाँठ_FTask_Ref)) {
            // Also update FuncRef/TaskRef packagep to point to new class
            // This fixes static method calls through typedefs in parameterized classes
            अगर (ftaskRefp->classOrPackagep() == oldClassp) ftaskRefp->classOrPackagep(newClassp);
            // Also update taskp if it points to a function in the old class.
            // AstNodeFTask::classOrPackagep() (op2) holds a parse-time
            // Dot/ClassOrPackageRef for extern declarations, which is deleted
            // in LinkDot::moveExternFuncDecl before V3Param runs. For inline
            // class methods op2 is nullptr. So this should never match.
            अगर (Ast_गाँठ_FTask* स्थिर oldTaskp = ftaskRefp->taskp()) {
                अगर (oldTaskp->classOrPackagep() == oldClassp) {
                    oldTaskp->v3fatalSrc(  // LCOV_EXCL_LINE
                        "FTask classOrPackagep unexpectedly matches old class "
                        << oldClassp->prettyNameQ());
                }
            }
        }

        अगर (nodep->op1p()) replaceRefsRecurse(nodep->op1p(), oldClassp, newClassp);
        अगर (nodep->op2p()) replaceRefsRecurse(nodep->op2p(), oldClassp, newClassp);
        अगर (nodep->op3p()) replaceRefsRecurse(nodep->op3p(), oldClassp, newClassp);
        अगर (nodep->op4p()) replaceRefsRecurse(nodep->op4p(), oldClassp, newClassp);
        अगर (nodep->nextp()) replaceRefsRecurse(nodep->nextp(), oldClassp, newClassp);
    }

    // Helper visitor to update VarXRefs to use variables from specialized interfaces.
    // When a module with interface ports is cloned and the port's interface is remapped
    // to a specialized version, VarXRefs that access members of the old interface need
    // to be updated to reference the corresponding members in the new interface.
    श्रेणी VarXRefRelinkVisitor final : खुला VNVisitor {
        Ast_गाँठ_Module* m_modp;  // The cloned module we're updating
        std::unordered_map<AstVar*, Ast_गाँठ_Module*> m_varModuleMap;  // Cache var->module lookups

    खुला:
        explicit VarXRefRelinkVisitor(Ast_गाँठ_Module* newModp)
            : m_modp{newModp} {
            iterate(newModp);
        }

    निजी:
        // Find which module a variable belongs to, using cache to avoid repeated backp() walks
        Ast_गाँठ_Module* findVarModule(AstVar* varp) {
            स्थिर स्वतः it = m_varModuleMap.find(varp);
            अगर (it != m_varModuleMap.end()) वापस it->second;
            Ast_गाँठ_Module* varModp = nullptr;
            क्रम (Ast_गाँठ* np = varp; np; np = np->backp()) {
                अगर (Ast_गाँठ_Module* स्थिर modp = VN_CAST(np, गाँठ_Module)) {
                    varModp = modp;
                    अवरोध;
                }
            }
            m_varModuleMap[varp] = varModp;
            वापस varModp;
        }

        व्योम visit(AstVarXRef* nodep) override {
            AstVar* स्थिर varp = nodep->varp();
            अगर (!varp) {
                iterateChildren(nodep);
                वापस;
            }

            // Get the dotted prefix (port name) from the VarXRef
            // dotted() format: "portname" or "portname.subpath" or empty
            स्थिर string& dotted = nodep->dotted();
            अगर (dotted.empty()) {
                iterateChildren(nodep);
                वापस;
            }

            स्थिर माप_प्रकार dotPos = dotted.find('.');
            स्थिर string portName = (dotPos == string::npos) ? dotted : dotted.substr(0, dotPos);
            अगर (portName.empty()) {
                iterateChildren(nodep);
                वापस;
            }

            // Find the interface port variable in the cloned module
            AstVar* portVarp = nullptr;
            क्रम (Ast_गाँठ* stmtp = m_modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                अगर (AstVar* स्थिर varChkp = VN_CAST(stmtp, Var)) {
                    अगर (varChkp->name() == portName && varChkp->isIfaceRef()) {
                        portVarp = varChkp;
                        अवरोध;
                    }
                }
            }
            अगर (!portVarp) {
                iterateChildren(nodep);
                वापस;
            }

            // Get the interface module from the port's dtype
            AstIfaceRefDType* स्थिर irefp = VN_CAST(portVarp->subDTypep(), IfaceRefDType);
            अगर (!irefp) {
                iterateChildren(nodep);
                वापस;
            }

            Ast_गाँठ_Module* स्थिर newIfacep = irefp->ifaceViaCellp();
            अगर (!newIfacep) {
                iterateChildren(nodep);
                वापस;
            }

            // Find which module the variable currently belongs to (cached)
            Ast_गाँठ_Module* स्थिर varModp = findVarModule(varp);

            // If variable is in a different module than the port's interface, remap it
            अगर (varModp && varModp != newIfacep) {
                क्रम (Ast_गाँठ* stmtp = newIfacep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    अगर (AstVar* स्थिर newVarp = VN_CAST(stmtp, Var)) {
                        अगर (newVarp->name() == varp->name()) {
                            UINFO(9, "VarXRef relink " << varp->name() << " in " << varModp->name()
                                                       << " -> " << newIfacep->name());
                            nodep->varp(newVarp);
                            अवरोध;
                        }
                    }
                }
            }
            iterateChildren(nodep);
        }
        व्योम visit(Ast_गाँठ* nodep) override { iterateChildren(nodep); }
    };

    // Retarget the captured references in newModp to its specialized interfaces.
    व्योम fixupCrossModuleRefDTypes(Ast_गाँठ_Module* newModp) {
        अगर (!V3LinkDotIfaceCapture::enabled()) वापस;
        UINFO(9, "iface capture clone fixup: " << newModp->prettyNameQ());
        पूर्णांक fixed_गिनती = 0;
        // References in a class nested in newModp (e.g. a covergroup) are included.
        newModp->foreach([&](AstRefDType* refp) {
            स्थिर VIfaceCaptureTag* स्थिर tagp = V3LinkDotIfaceCapture::captureTag(refp);
            अगर (!tagp) वापस;

            Ast_गाँठ_Module* स्थिर correctModp
                = V3LinkDotIfaceCapture::followCellPath(newModp, tagp->m_cellPath);
            UINFO(9, "  path fixup: " << refp << " cellPath='" << tagp->m_cellPath << "' -> "
                                      << (correctModp ? correctModp->name() : "<null>"));
            अगर (!correctModp || correctModp->dead()) वापस;
            अगर (correctModp->parameterizedTemplate()) वापस;

            bool जड़ = false;
            अगर (refp->typedefp()) {
                अगर (AstTypedef* स्थिर newTdp = V3LinkDotIfaceCapture::findTypedefInModule(
                        correctModp, refp->typedefp()->name())) {
                    refp->typedefp(newTdp);
                    अगर (newTdp->subDTypep()) refp->refDTypep(newTdp->subDTypep());
                    जड़ = true;
                }
            }
            अगर (!जड़ && refp->refDTypep()) {
                अगर (Ast_गाँठ_DType* स्थिर newDtp = V3LinkDotIfaceCapture::findDTypeInModule(
                        correctModp, refp->refDTypep()->name(), refp->refDTypep()->type())) {
                    refp->refDTypep(newDtp);
                    जड़ = true;
                }
            }
            अगर (जड़) ++fixed_गिनती;
        });
        V3Stats::add_Stat_योग("IfaceCapture, Ledger fixups in V3Param", fixed_गिनती);
    }

    // Return true on success, false on error
    bool deepCloneModule(Ast_गाँठ_Module* srcModp, Ast_गाँठ* ifErrorp, AstPin* paramsp,
                         स्थिर string& newname, स्थिर IfaceRefRefs& ifaceRefRefs) {
        // Deep clone of new module
        // Note all module internal variables will be re-linked to the new modules by clone
        // However links outside the module (like on the upper cells) will not.
        Ast_गाँठ_Module* newModp;
        अगर (srcModp->user3p()) {
            newModp = VN_CAST(srcModp->user3p()->clone_वृक्ष(false), गाँठ_Module);
        } अन्यथा {
            newModp = srcModp->clone_वृक्ष(false);
        }
        // AstPin normally retains links to external module formals across cloning. For a cloned
        // interface, relink pins whose formals were cloned with the interface while clonep() is
        // still valid, so nested parameterized classes use the cloned interface parameters.
        अगर (AstIface* स्थिर newIfacep = VN_CAST(newModp, Iface)) {
            newIfacep->foreach([](AstPin* pinp) { pinp->cloneRelinkGen(); });
        }

        // Mark the source module as a parameterized template now that a specialized
        // clone exists.  This suppresses width/type errors on the unresolved template
        // during widthParamsEdit (which runs before V3LinkDot sets dead()).
        srcModp->parameterizedTemplate(true);
        // The clone is a specialized instance, not a template.  Clear the flag in
        // case it was inherited from a prior cloneTree (when srcModp was already
        // marked by an earlier specialization).
        newModp->parameterizedTemplate(false);

        newModp->name(newname);
        newModp->user2(false);  // We need to re-recurse this module once changed
        newModp->recursive(false);
        newModp->recursiveClone(false);
        newModp->hasGParam(false);
        // Recursion may need level cleanups
        अगर (newModp->level() <= m_modp->level()) newModp->level(m_modp->level() + 1);
        अगर ((newModp->level() - srcModp->level()) >= (v3Global.opt.moduleRecursionDepth() - 2)) {
            ifErrorp->v3error("Exceeded maximum --module-recursion-depth of "
                              << v3Global.opt.moduleRecursionDepth());
            VL_DO_DANGLING(newModp->delete_वृक्ष(), newModp);
            वापस false;
        }

        अगर (AstClass* स्थिर newClassp = VN_CAST(newModp, Class)) {
            replaceRefsRecurse(newModp->stmtsp(), newClassp, VN_AS(srcModp, Class));
        }

        // Keep tree sorted by level. Note: Different parameterizations of the same recursive
        // module end up with the same level, which we will need to fix up at the end, as we do not
        // know up front how recursive modules are expanded, and a later expansion might re-use an
        // earlier expansion (see t_recursive_module_bug_2).
        Ast_गाँठ* insertp = srcModp;
        जबतक (VN_IS(insertp->nextp(), गाँठ_Module)
               && VN_AS(insertp->nextp(), गाँठ_Module)->level() <= newModp->level()) {
            insertp = insertp->nextp();
        }
        insertp->add_अग्र_Here(newModp);

        m_modNameMap.emplace(newModp->name(), ModInfo{newModp});
        स्थिर स्वतः iter = m_modNameMap.find(newname);
        CloneMap* स्थिर clonemapp = &(iter->second.m_cloneMap);
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
        bool needRelinkByName = false;
        अगर (paramsp) {
            क्रम (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                अगर (pinp->modVarp() && clonemapp->find(pinp->modVarp()) == clonemapp->end()) {
                    needRelinkByName = true;
                    अवरोध;
                }
            }
        }
        अगर (needRelinkByName) {
            relinkPinsByName(paramsp, newModp);
        } अन्यथा {
            relinkPins(clonemapp, paramsp);
        }

        // Fix any interface references
        क्रम (स्वतः it = ifaceRefRefs.cbegin(); it != ifaceRefRefs.cend(); ++it) {
            स्थिर AstIfaceRefDType* स्थिर portIrefp = it->first;
            स्थिर AstIfaceRefDType* स्थिर pinIrefp = it->second;
            AstIfaceRefDType* स्थिर cloneIrefp = portIrefp->clonep();
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
        अगर (!ifaceRefRefs.empty()) { VarXRefRelinkVisitor{newModp}; }

        // Fix cross-module REFDTYPE pointers in newModp.
        UASSERT_OBJ(newModp, srcModp, "newModp null before hierarchy fixup");
        fixupCrossModuleRefDTypes(newModp);

        // Assign parameters to the constants specified
        // DOES clone() so must be finished with module clonep() before here
        क्रम (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            अगर (pinp->exprp()) {
                अगर (AstVar* स्थिर modvarp = pinp->modVarp()) {
                    Ast_गाँठ* स्थिर newp = pinp->exprp();  // Const or ConsArray
                    AstConst* स्थिर exprp = VN_CAST(newp, Const);
                    AstConst* स्थिर origp = VN_CAST(modvarp->valuep(), Const);
                    स्थिर bool overridden
                        = !(origp && exprp && ParameterizedHierBlocks::areSame(exprp, origp));
                    // Remove any existing parameter
                    अगर (modvarp->valuep()) modvarp->valuep()->unlinkFrBack()->delete_वृक्ष();
                    // Set this parameter to value requested by cell
                    UINFO(9, "       set param " << modvarp << " = " << newp);
                    modvarp->valuep(newp->clone_वृक्ष(false));
                    modvarp->overriddenParam(overridden);
                } अन्यथा अगर (AstParamTypeDType* स्थिर modptp = pinp->modPTypep()) {
                    Ast_गाँठ_DType* स्थिर dtypep = VN_AS(pinp->exprp(), गाँठ_DType);
                    UASSERT_OBJ(dtypep, pinp, "unlinked param dtype");
                    अगर (modptp->childDTypep()) modptp->childDTypep()->unlinkFrBack()->delete_वृक्ष();
                    // Set this parameter to value requested by cell
                    modptp->childDTypep(dtypep->clone_वृक्ष(false));
                    // Later V3LinkDot will convert the ParamDType to a Typedef
                    // Not done here as may be localparams, etc, that also need conversion
                }
            }
        }

        वापस true;
    }
    स्थिर ModInfo* moduleFindOrClone(Ast_गाँठ_Module* srcModp, Ast_गाँठ* ifErrorp, AstPin* paramsp,
                                     स्थिर string& newname, स्थिर IfaceRefRefs& ifaceRefRefs) {
        // Already made this flavor?
        स्वतः it = m_modNameMap.find(newname);
        अगर (it != m_modNameMap.end()) {
            UINFO(4, "     De-parameterize to prev: " << it->second.m_modp);
        } अन्यथा {
            अगर (!deepCloneModule(srcModp, ifErrorp, paramsp, newname, ifaceRefRefs)) {
                वापस nullptr;
            }
            it = m_modNameMap.find(newname);
            UASSERT(it != m_modNameMap.end(), "should find just-made module");
        }
        स्थिर ModInfo* स्थिर modInfop = &(it->second);
        वापस modInfop;
    }

    व्योम convertToStringp(Ast_गाँठ* nodep) {
        // Should be called on values of parameters of type string to convert them
        // to properly typed string constants.
        // Has no effect if the value is not a string constant.
        AstConst* स्थिर constp = VN_CAST(nodep, Const);
        // Check if it wasn't already converted
        अगर (constp && !constp->अंक().isString()) {
            constp->replaceWith(
                नया AstConst{constp->fileline(), AstConst::String{}, constp->अंक().toString()});
            VL_DO_DANGLING(constp->delete_वृक्ष(), constp);
        }
    }

    // Fold a param/lparam member's value to a Const in place.  Resolves any
    // class::member Dots in the value first (including those buried behind
    // VarRefs to sibling lparams that are not yet const, e.g. `one = base`
    // where `base = inner_a::v`), resolving siblings deepest-first so the
    // whole chain constifies.  `inProgress` guards against reference cycles.
    व्योम constify_Member_मूल्य(AstVar* varp, std::set<AstVar*>& inProgress) {
        अगर (!varp->valuep() || VN_IS(varp->valuep(), Const)) वापस;
        अगर (!inProgress.जोड़ो(varp).second) {
            // Re-entered while still resolving varp: its value transitively
            // references itself.  Report and replace the value with a Const, as
            // otherwise the unresolved Dot reaches V3Width as an untyped node.
            varp->v3error("Variable's initial value is circular: " << varp->prettyNameQ());
            varp->valuep()->unlinkFrBack()->delete_वृक्ष();
            varp->valuep(नया AstConst{varp->fileline(), AstConst::Signed32{}, 0});
            वापस;
        }
        // First resolve sibling lparams this value references, so their Dots
        // are folded before we inline them here.
        std::set<AstVar*> siblings;
        varp->valuep()->foreach([&](स्थिर AstVarRef* refp) {
            AstVar* स्थिर refVarp = refp->varp();
            अगर (refVarp && refVarp->isParam() && refVarp->valuep()
                && !VN_IS(refVarp->valuep(), Const)) {
                siblings.जोड़ो(refVarp);
            }
        });
        क्रम (AstVar* स्थिर sibp : siblings) constify_Member_मूल्य(sibp, inProgress);
        // Resolve class-scoped references and any member selections they expose.
        resolveDeferredDotsReachableFrom(varp->valuep(), m_modp);
        V3Const::constifyParamsEdit(varp);
        inProgress.erase(varp);
    }

    // Return the member name on a deferred Dot RHS, including a selected member.
    अटल AstParseRef* memberParseRef(Ast_गाँठ* nodep) {
        अगर (AstParseRef* स्थिर refp = VN_CAST(nodep, ParseRef)) वापस refp;
        Ast_गाँठ_Pre_Sel* स्थिर selp = VN_CAST(nodep, गाँठ_Pre_Sel);
        वापस selp ? VN_CAST(selp->fromp(), ParseRef) : nullptr;
    }

    // Replace a deferred Dot with valuep, preserving any select on the member name.
    अटल व्योम replaceMemberDot(AstDot* dotp, AstParseRef* memberRefp, Ast_गाँठ_व्यंजक* valuep) {
        अगर (memberRefp == dotp->rhsp()) {
            dotp->replaceWith(valuep);
        } अन्यथा {
            Ast_गाँठ_Pre_Sel* स्थिर selp = VN_AS(dotp->rhsp(), गाँठ_Pre_Sel);
            अगर (AstAttrOf* स्थिर attrp = selp->attrp()) {
                अगर (Ast_गाँठ* स्थिर oldFromp = attrp->fromp()) {
                    oldFromp->replaceWith(valuep->clone_वृक्ष(false));
                    VL_DO_DANGLING(oldFromp->delete_वृक्ष(), oldFromp);
                }
            }
            memberRefp->replaceWith(valuep);
            VL_DO_DANGLING(memberRefp->delete_वृक्ष(), memberRefp);
            dotp->replaceWith(dotp->rhsp()->unlinkFrBack());
        }
        VL_DO_DANGLING(dotp->delete_वृक्ष(), dotp);
    }

    // True for an expression produced while lowering a deferred struct-member chain.
    अटल bool isDeferredMemberBase(स्थिर Ast_गाँठ_व्यंजक* nodep) {
        अगर (VN_IS(nodep, MemberSel)) वापस true;
        अगर (स्थिर Ast_गाँठ_Pre_Sel* स्थिर selp = VN_CAST(nodep, गाँठ_Pre_Sel)) {
            स्थिर Ast_गाँठ_DType* स्थिर fromDTypep = selp->fromp()->dtypep();
            स्थिर Ast_गाँठ_Array_DType* स्थिर arrayDTypep
                = fromDTypep ? VN_CAST(fromDTypep->skipRefOrNullp(), गाँठ_Array_DType) : nullptr;
            अगर (arrayDTypep) {
                स्थिर Ast_गाँठ_DType* स्थिर elemDTypep = arrayDTypep->subDTypep();
                वापस elemDTypep && VN_IS(elemDTypep->skipRefOrNullp(), गाँठ_UOr_Struct_DType);
            }
            वापस isDeferredMemberBase(selp->fromp());
        }
        स्थिर Ast_गाँठ_DType* स्थिर dtypep = nodep->dtypep();
        वापस dtypep && VN_IS(dtypep->skipRefOrNullp(), गाँठ_UOr_Struct_DType);
    }

    // Lower a Dot exposed after its inner class::member reference was substituted.
    अटल व्योम resolveMemberDot(AstDot* dotp) {
        Ast_गाँठ_व्यंजक* स्थिर lhsp = VN_CAST(dotp->lhsp(), गाँठ_व्यंजक);
        AstParseRef* स्थिर memberRefp = memberParseRef(dotp->rhsp());
        अगर (!lhsp || !memberRefp || !isDeferredMemberBase(lhsp)) वापस;
        AstMemberSel* स्थिर newp = नया AstMemberSel{memberRefp->fileline(), lhsp->unlinkFrBack(),
                                                    VFlagChildDType{}, memberRefp->name()};
        replaceMemberDot(dotp, memberRefp, newp);
    }

    // Helper to resolve DOT to RefDType for class type references.
    // If the class is parameterized and not yet specialized, specialize it first.
    // This handles cases like: iface #(param_class#(value)::typedef_name)
    व्योम resolveDotToTypedef(Ast_गाँठ* exprp) {
        AstDot* स्थिर dotp = VN_CAST(exprp, Dot);
        अगर (!dotp) वापस;
        UINFO(9, "Resolve deferred Dot: " << dotp);
        AstClassOrPackageRef* स्थिर classRefp = VN_CAST(dotp->lhsp(), ClassOrPackageRef);
        अगर (!classRefp) {
            resolveMemberDot(dotp);
            वापस;
        }
        AstParseRef* स्थिर parseRefp = memberParseRef(dotp->rhsp());
        अगर (!parseRefp) वापस;

        स्थिर AstClass* lhsClassp = VN_CAST(classRefp->classOrPackageSkipp(), Class);

        // Specialize parameterized class through type parameter indirection (#7000)
        AstParamTypeDType* स्थिर paramTypep
            = VN_CAST(classRefp->classOrPackageNodep(), ParamTypeDType);
        अगर (paramTypep) {
            Ast_गाँठ_DType* स्थिर dtypep = paramTypep->subDTypep();
            AstClassRefDType* स्थिर classRefDTypep
                = dtypep ? VN_CAST(dtypep->skipRefOrNullp(), ClassRefDType) : nullptr;
            अगर (classRefDTypep) {
                AstClass* स्थिर srcClassp = classRefDTypep->classp();
                अगर (srcClassp && srcClassp->hasGParam() && classRefDTypep->paramsp()) {
                    अगर (lhsClassp == srcClassp || !lhsClassp) {
                        classRefDeparam(classRefDTypep, srcClassp);
                        lhsClassp = classRefDTypep->classp();
                    }
                }
            }
        }

        अगर (classRefp->paramsp()) {
            AstClass* स्थिर srcClassp = VN_CAST(classRefp->classOrPackageNodep(), Class);
            अगर (srcClassp && srcClassp->hasGParam()) {
                अगर (lhsClassp == srcClassp || !lhsClassp) {
                    classRefDeparam(classRefp, srcClassp);
                    lhsClassp = VN_CAST(classRefp->classOrPackageSkipp(), Class);
                }
            }
        }
        // Typedef-aliased paramed class (e.g., typedef C#(7) my_c; my_c::b).
        अगर (AstClassRefDType* स्थिर crp = class_Ref_DType_Of_गाँठ(classRefp->classOrPackageNodep())) {
            अगर (AstClass* स्थिर srcClassp = crp->classp()) {
                अगर (srcClassp->hasGParam() && crp->paramsp()) {
                    classRefDeparam(crp, srcClassp);
                    अगर (AstClass* स्थिर newClassp = crp->classp()) lhsClassp = newClassp;
                }
            }
        }
        अगर (!lhsClassp) वापस;

        Ast_गाँठ* स्थिर memberp = m_memberMap.findMember(lhsClassp, parseRefp->name());
        UINFO(9, "Resolve deferred class member: " << parseRefp->name() << " -> " << memberp);
        अगर (AstTypedef* स्थिर tdefp = VN_CAST(memberp, Typedef)) {
            अगर (parseRefp != dotp->rhsp()) वापस;
            AstRefDType* स्थिर refp = नया AstRefDType{dotp->fileline(), tdefp->name()};
            refp->typedefp(tdefp);
            dotp->replaceWith(refp);
            VL_DO_DANGLING(dotp->delete_वृक्ष(), dotp);
        } अन्यथा अगर (AstVar* स्थिर varp = VN_CAST(memberp, Var)) {
            substituteParamMember(dotp, parseRefp, varp);
        }
    }

    // Substitute a class param/lparam member's constant value for dotp. The
    // reverse-order deferred-Dot walk lowers any outer fields to MemberSels.
    व्योम substituteParamMember(AstDot* स्थिर dotp, AstParseRef* स्थिर memberRefp,
                               AstVar* स्थिर varp) {
        अगर (!varp->isParam() || !varp->valuep()) वापस;
        अगर (!VN_IS(varp->valuep(), Const)) {
            std::set<AstVar*> inProgress;
            constify_Member_मूल्य(varp, inProgress);
        }
        AstConst* स्थिर constp = VN_CAST(varp->valuep(), Const);
        अगर (!constp) वापस;
        AstConst* स्थिर newp = constp->clone_वृक्ष(false);
        newp->dtypep(varp->subDTypep());
        replaceMemberDot(dotp, memberRefp, newp);
    }

    // Resolve one class-scoped RefDType. The generic deferred walk finds these
    // through every dtype wrapper and follows out-of-tree typedef references.
    व्योम resolveParamClassRefDType(AstRefDType* refp) {
        अगर (refp->typedefp() || refp->refDTypep()) वापस;

        AstClassOrPackageRef* स्थिर classRefp
            = VN_CAST(refp->classOrPackageOpp(), ClassOrPackageRef);
        अगर (!classRefp) वापस;

        AstClass* srcClassp = VN_CAST(classRefp->classOrPackageNodep(), Class);
        अगर (srcClassp && srcClassp->hasGParam()) {
            classRefDeparam(classRefp, srcClassp);
            वापस;
        }

        // Follow typedef chain to find the underlying ClassRefDType
        Ast_गाँठ* targetp = classRefp->classOrPackageNodep();
        जबतक (स्थिर AstTypedef* स्थिर tdefp = VN_CAST(targetp, Typedef))
            targetp = tdefp->subDTypep();
        अगर (Ast_गाँठ_DType* स्थिर dtargetp = VN_CAST(targetp, गाँठ_DType))
            targetp = dtargetp->skipRefOrNullp();
        अगर (!targetp) वापस;
        AstClassRefDType* स्थिर classRefDTypep = VN_CAST(targetp, ClassRefDType);
        अगर (!classRefDTypep) वापस;
        srcClassp = classRefDTypep->classp();
        अगर (!srcClassp) वापस;

        AstClass* newClassp = srcClassp;
        अगर (srcClassp->hasGParam()) {
            classRefDeparam(classRefDTypep, srcClassp);
            newClassp = classRefDTypep->classp();
        }

        AstTypedef* स्थिर typedefp
            = VN_CAST(m_memberMap.findMember(newClassp, refp->name()), Typedef);
        अगर (typedefp) {
            refp->typedefp(typedefp);
            refp->classOrPackagep(newClassp);
        }
    }

    // True if a $bits/$size type query in nodep's parameters reads another type parameter.
    अटल bool defaultParamsHaveTypeQueryOnParamType(स्थिर AstClassRefDType* nodep) {
        bool found = false;
        nodep->foreach([&](स्थिर AstAttrOf* attrp) {
            अगर (found || !attrp->attrType().isTypeQuery()) वापस;
            स्थिर AstRefDType* स्थिर refp = VN_CAST(attrp->fromp(), RefDType);
            अगर (refp && VN_IS(refp->refDTypep(), ParamTypeDType)) found = true;
        });
        वापस found;
    }

    // Check if exprp's class matches origp's class after deparameterization.
    // Handles both the simple case (user4p link from defaultsResolved) and the
    // nested case where the default's inner class has non-default sub-parameters
    // (e.g., uvm_sequence#(uvm_reg_item) where uvm_reg_item != default uvm_sequence_item).
    bool classTypeMatchesDefaultClone(स्थिर Ast_गाँठ_DType* exprp, स्थिर Ast_गाँठ_DType* origp) {
        exprp = exprp->skipRefp();
        origp = origp->skipRefp();
        स्थिर स्वतः* स्थिर व्यंजक_Class_Refp = VN_CAST(exprp, ClassRefDType);
        स्थिर स्वतः* स्थिर origClassRefp = VN_CAST(origp, ClassRefDType);
        अगर (!व्यंजक_Class_Refp || !origClassRefp) वापस false;
        // Fast path: check user4p link (set when template was deparameterized with defaults)
        स्थिर Ast_गाँठ_Module* स्थिर defaultClonep
            = VN_CAST(origClassRefp->classp()->user4p(), Class);
        अगर (defaultClonep && defaultClonep == व्यंजक_Class_Refp->classp()) वापस true;
        // Skip the comparison when the default's $bits/$size reads another type parameter, as
        // deparameterizing it below would resolve that shared type at the wrong width (#7711).
        अगर (defaultParamsHaveTypeQueryOnParamType(origClassRefp)) वापस false;
        // Slow path: deparameterize the default type and compare the result.
        // Different templates can never match; use origName() because exprp's
        // class may already be a specialization (clone) of the template.
        अगर (!origClassRefp->classp()->hasGParam()) वापस false;
        अगर (origClassRefp->classp()->origName() != व्यंजक_Class_Refp->classp()->origName())
            वापस false;
        // Prevent re-entry when classRefDeparam recurses through cellPinCleanup.
        अगर (!m_defaultCloneInProgress.जोड़ो(origClassRefp->classp()).second) वापस false;
        // const_cast safe: cloneTree doesn't modify the source
        AstClassRefDType* स्थिर origClonep = static_cast<AstClassRefDType*>(
            const_cast<AstClassRefDType*>(origClassRefp)->clone_वृक्ष(false));
        Ast_गाँठ_Module* स्थिर resolvedModp = classRefDeparam(origClonep, origClassRefp->classp());
        स्थिर bool match = resolvedModp && VN_CAST(resolvedModp, Class) == व्यंजक_Class_Refp->classp();
        VL_DO_DANGLING(origClonep->delete_वृक्ष(), origClonep);
        m_defaultCloneInProgress.erase(origClassRefp->classp());
        वापस match;
    }

    अटल bool paramConstsEqualAtMaxWidth(AstConst* exprp, AstConst* origp) {
        // Return true if two integer constants are equal after sign-extending
        // both to max(width).  A typed parameter default (e.g. byte) is
        // narrower than a 32-bit pin expression, so sameTree/areSame fail.
        अगर (exprp->अंक().width() == origp->अंक().width()) वापस false;
        अगर (exprp->अंक().isOpaque()) वापस false;
        अगर (origp->अंक().isOpaque()) वापस false;
        स्थिर पूर्णांक maxWidth = std::max(exprp->अंक().width(), origp->अंक().width());
        V3_संख्या व्यंजक_अंक{exprp, maxWidth};
        अगर (exprp->अंक().isSigned()) {
            व्यंजक_अंक.opExtendS(exprp->अंक(), exprp->अंक().width());
        } अन्यथा {
            व्यंजक_अंक.opAssign(exprp->अंक());
        }
        V3_संख्या orig_अंक{origp, maxWidth};
        अगर (origp->अंक().isSigned()) {
            orig_अंक.opExtendS(origp->अंक(), origp->अंक().width());
        } अन्यथा {
            orig_अंक.opAssign(origp->अंक());
        }
        V3_संख्या isEq{exprp, 1};
        isEq.opEq(व्यंजक_अंक, orig_अंक);
        वापस isEq.isNeqZero();
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
    व्योम resolveGenericIfaceForwardingPins(AstPin* paramsp) {
        क्रम (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            अगर (!pinp->exprp()) जारी;
            अगर (!VString::startsWith(pinp->name(), "__VGIfaceParam")) जारी;
            स्थिर Ast_गाँठ_Var_Ref* स्थिर fwdRefp = VN_CAST(pinp->exprp(), गाँठ_Var_Ref);
            अगर (!fwdRefp) जारी;
            स्थिर AstIfaceRefDType* स्थिर resolvedp
                = VN_CAST(fwdRefp->varp()->childDTypep()->skipRefp(), IfaceRefDType);
            UASSERT_OBJ(resolvedp, pinp,
                        "Generic interface forwarding pin not specialized before use");
            AstIfaceRefDType* स्थिर newIrefp = नया AstIfaceRefDType{
                resolvedp->fileline(), resolvedp->modportFileline(), resolvedp->cellName(),
                resolvedp->ifaceName(), resolvedp->modportName()};
            newIrefp->ifacep(resolvedp->ifacep());
            अगर (resolvedp->paramsp()) {
                newIrefp->addParamsp(resolvedp->paramsp()->clone_वृक्ष(true));
            }
            pinp->exprp()->unlinkFrBack()->delete_वृक्ष();
            pinp->exprp(newIrefp);
        }
    }

    // Include the pin's value in the specialization name, so cells passing equal
    // values share one module clone.
    व्योम name_By_Pin_मूल्य(AstPin* pinp, Ast_गाँठ_Module* srcModp, AstVar* modvarp, string& longnamer,
                        bool& any_overridesr) {
        longnamer += "_" + paramSmallName(srcModp, modvarp) + param_मूल्य_संख्या(pinp->exprp());
        any_overridesr = true;
    }

    // Name the specialization by a value pin, or prepare a type pin, noting in
    // overridingTypePinsr whether it differs from the default
    व्योम cellPinCleanup(Ast_गाँठ* nodep, AstPin* pinp, AstPin* paramsp, Ast_गाँठ_Module* srcModp,
                        string& longnamer, bool& any_overridesr,
                        std::unordered_set<स्थिर AstPin*>& overridingTypePinsr) {
        अगर (!pinp->exprp()) वापस;  // No-connect
        अगर (AstVar* स्थिर modvarp = pinp->modVarp()) {
            resolveDeferredDotsReachableFrom(modvarp->subDTypep(), m_modp);
            अगर (!modvarp->isGParam()) {
                pinp->v3fatalSrc("Attempted parameter setting of non-parameter: Param "
                                 << pinp->prettyNameQ() << " of " << nodep->prettyNameQ());
            } अन्यथा अगर (VN_IS(pinp->exprp(), Pattern)
                       || (VN_IS(pinp->exprp(), ConsArray)
                           && arraySubDTypep(modvarp->subDTypep()))) {
                // A pattern or array value is named by its contents
                name_By_Pin_मूल्य(pinp, srcModp, modvarp, longnamer, any_overridesr);
            } अन्यथा {
                UINFO(9, "cellPinCleanup: before constify " << pinp << " " << modvarp);
                V3Const::constifyParamsEdit(pinp->exprp());
                अगर (is_Aggregate_Param_मूल्य(pinp->exprp())) {
                    name_By_Pin_मूल्य(pinp, srcModp, modvarp, longnamer, any_overridesr);
                    वापस;
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
                अगर (modvarp->valuep()
                    && (VN_IS(modvarp->valuep(), Cast) || VN_IS(modvarp->valuep(), Cast_आकार))) {
                    स्थिर bool dependent = modvarp->valuep()->exists(
                        [](Ast_गाँठ* np) { वापस VN_IS(np, VarRef) || VN_IS(np, RefDType); });
                    अगर (!dependent) V3Const::constifyParamsEdit(modvarp->valuep());
                }
                UINFO(9, "cellPinCleanup: after constify " << pinp);
                // String constants are parsed as logic arrays and converted to strings in V3Const.
                // At this moment, some constants may have been already converted.
                // To correctly compare constants, both should be of the same type,
                // so they need to be converted.
                अगर (isString(modvarp->subDTypep())) {
                    convertToStringp(pinp->exprp());
                    convertToStringp(modvarp->valuep());
                }
                AstConst* स्थिर exprp = VN_CAST(pinp->exprp(), Const);
                AstConst* स्थिर origp = VN_CAST(modvarp->valuep(), Const);
                // Width the pin to the port's type so equal values hash the same (#5479).
                AstConst* normedNamep = nullptr;
                अगर (exprp && !exprp->अंक().isDouble() && !exprp->अंक().isString()) {
                    AstVar* cloneVarp = modvarp->clone_वृक्ष(false);
                    bool cloneVarpUnresolved = false;
                    अगर (Ast_गाँठ* स्थिर oldValuep = cloneVarp->valuep()) {
                        oldValuep->unlinkFrBack();
                        VL_DO_DANGLING(oldValuep->delete_वृक्ष(), oldValuep);
                    }
                    cloneVarp->valuep(exprp->clone_वृक्ष(false));
                    अगर (Ast_गाँठ_DType* स्थिर origDTypep = modvarp->subDTypep()) {
                        // Attach clone under cloneVarp so the root has a back pointer.
                        अगर (cloneVarp->childDTypep())
                            cloneVarp->childDTypep()->unlinkFrBack()->delete_वृक्ष();
                        cloneVarp->childDTypep(origDTypep->clone_वृक्ष(false));
                        cloneVarp->dtypep(nullptr);
                        // Inline param refs so widthing doesn't touch the template (#7411).
                        constexpr पूर्णांक maxSubstIters = 1000;
                        क्रम (पूर्णांक it = 0; it < maxSubstIters; ++it) {
                            bool any = false;
                            cloneVarp->foreach([&](AstVarRef* varrefp) {
                                AstVar* स्थिर targetp = varrefp->varp();
                                Ast_गाँठ* replacep = nullptr;
                                क्रम (AstPin* pp = paramsp; pp; pp = VN_AS(pp->nextp(), Pin)) {
                                    अगर (pp->modVarp() == targetp) {
                                        // A pattern or array override names the value too
                                        अगर (VN_IS(pp->exprp(), Const)
                                            || VN_IS(pp->exprp(), Pattern)
                                            || VN_IS(pp->exprp(), ConsArray)) {
                                            replacep = pp->exprp()->clone_वृक्ष(false);
                                        }
                                        अवरोध;
                                    }
                                }
                                अगर (!replacep && targetp->valuep()) {
                                    replacep = targetp->valuep()->clone_वृक्ष(false);
                                }
                                // An inlined pattern takes the type of its parameter
                                AstPattern* स्थिर patp = VN_CAST(replacep, Pattern);
                                अगर (patp && !patp->childDTypep() && targetp->childDTypep()) {
                                    patp->childDTypep(targetp->childDTypep()->clone_वृक्ष(false));
                                }
                                अगर (replacep) {
                                    varrefp->replaceWith(replacep);
                                    VL_DO_DANGLING(varrefp->delete_वृक्ष(), varrefp);
                                    any = true;
                                }
                            });
                            // Replace RefDType to a ParamTypeDType with pin override
                            // or the paramtype's default so constify below does not
                            // reach into the template.  Collect then replace in
                            // reverse so descendants aren't freed early.
                            std::vector<std::pair<AstRefDType*, Ast_गाँठ_DType*>> toReplace;
                            cloneVarp->foreach([&](AstRefDType* refp) {
                                AstParamTypeDType* स्थिर ptdp
                                    = VN_CAST(refp->refDTypep(), ParamTypeDType);
                                अगर (!ptdp) वापस;
                                AstPin* overridePinp = nullptr;
                                क्रम (AstPin* pp = paramsp; pp; pp = VN_AS(pp->nextp(), Pin)) {
                                    अगर (pp->modPTypep() == ptdp) {
                                        overridePinp = pp;
                                        अवरोध;
                                    }
                                }
                                Ast_गाँठ_DType* स्थिर substp
                                    = overridePinp ? VN_CAST(overridePinp->exprp(), गाँठ_DType)
                                                   : ptdp->subDTypep();
                                अगर (substp) toReplace.emplace_back(refp, substp);
                            });
                            क्रम (स्वतः it = toReplace.rbegin(); it != toReplace.rend(); ++it) {
                                AstRefDType* स्थिर refp = it->first;
                                refp->replaceWith(it->second->clone_वृक्ष(false));
                                VL_DO_DANGLING(refp->delete_वृक्ष(), refp);
                                any = true;
                            }
                            अगर (!any) अवरोध;
                        }
                        // Skip the widthing constify if anything left can't be evaluated here: a
                        // reference, as to a variable or around a cycle, an unresolved type
                        // parameter, or a type or enum item of the template
                        cloneVarp->foreach([&](Ast_गाँठ* np) {
                            अगर (VN_IS(np, VarRef)) {
                                cloneVarpUnresolved = true;
                                वापस;
                            }
                            अगर (AstEnumItemRef* स्थिर itemRefp = VN_CAST(np, EnumItemRef)) {
                                अगर (V3LinkDotIfaceCapture::findOwnerModule(itemRefp->itemp())
                                    == srcModp) {
                                    cloneVarpUnresolved = true;
                                }
                                वापस;
                            }
                            AstRefDType* स्थिर refp = VN_CAST(np, RefDType);
                            अगर (!refp) वापस;
                            अगर (VN_IS(refp->refDTypep(), ParamTypeDType)) {
                                UINFO(5, "  cellPinCleanup: skip normedNamep "
                                         "(unresolved RefDType->ParamTypeDType) pin="
                                             << pinp->prettyNameQ());
                                cloneVarpUnresolved = true;
                            }
                            // A type declared in the template must not be widthed there
                            अगर (V3LinkDotIfaceCapture::findOwnerModule(refp->typedefp()) == srcModp
                                || V3LinkDotIfaceCapture::findOwnerModule(refp->refDTypep())
                                       == srcModp) {
                                cloneVarpUnresolved = true;
                            }
                        });
                    }
                    अगर (!cloneVarpUnresolved) {
                        V3Const::constifyParamsEdit(cloneVarp);
                        अगर (AstConst* स्थिर widthedp = VN_CAST(cloneVarp->valuep(), Const)) {
                            // Stamp the port's type so equal values hash the same.
                            अगर (cloneVarp->dtypep()) widthedp->dtypep(cloneVarp->dtypep());
                            widthedp->unlinkFrBack();
                            normedNamep = widthedp;
                        }
                    }
                    VL_DO_DANGLING(cloneVarp->delete_वृक्ष(), cloneVarp);
                }
                AstConst* स्थिर namingExprp = normedNamep ? normedNamep : exprp;
                अगर (!exprp) {
                    // With DepGraph architecture, all expressions should be constants
                    // by the time V3Param runs. If not, it's an error.
                    UINFO(9, "cellPinCleanup: NOT CONST after constify " << pinp);
                    UINFOTREE(1, pinp, "", "errnode");
                    pinp->v3error("Can't convert defparam value to constant: Param "
                                  << pinp->prettyNameQ() << " of " << nodep->prettyNameQ());
                    Ast_गाँठ* स्थिर pinExprp = pinp->exprp();
                    pinExprp->replaceWith(नया AstConst{pinp->fileline(), AstConst::Widthed_मूल्य{},
                                                       modvarp->width(), 0});
                    VL_DO_DANGLING(pinExprp->delete_वृक्ष(), pinExprp);
                } अन्यथा अगर (origp
                           && (exprp->same_वृक्ष(origp)
                               || (exprp->अंक().width() == origp->अंक().width()
                                   && ParameterizedHierBlocks::areSame(exprp, origp))
                               || paramConstsEqualAtMaxWidth(exprp, origp))) {
                    // Setting parameter to its default value.  Just ignore it.
                    // This prevents making additional modules, and makes coverage more
                    // obvious as it won't show up under a unique module page name.
                    UINFO(9, "cellPinCleanup: same as default " << pinp);
                } अन्यथा अगर (namingExprp->अंक().isDouble() || namingExprp->अंक().isString()
                           || namingExprp->अंक().isFourState()
                           || namingExprp->अंक().width() != 32) {
                    longnamer += ("_" + paramSmallName(srcModp, modvarp)
                                  + param_मूल्य_संख्या(namingExprp));
                    any_overridesr = true;
                } अन्यथा {
                    longnamer += ("_" + paramSmallName(srcModp, modvarp)
                                  + namingExprp->अंक().ascii(false));
                    any_overridesr = true;
                }
                अगर (normedNamep) VL_DO_DANGLING(normedNamep->delete_वृक्ष(), normedNamep);
            }
        } अन्यथा अगर (AstParamTypeDType* स्थिर modvarp = pinp->modPTypep()) {
            // Resolve the pin and declared type through arbitrary dtype wrappers
            // and typedef chains before widthing either one.
            resolveDeferredDotsReachableFrom(pinp->exprp(), m_modp);
            resolveDeferredDotsReachableFrom(modvarp->subDTypep(), m_modp);

            Ast_गाँठ_DType* rawTypep = VN_CAST(pinp->exprp(), गाँठ_DType);
            // Guard against widthing a struct/union still owned by a
            // parameterized template interface (not yet specialized).
            // widthParamsEdit is destructive: it evaluates range expressions,
            // sets didWidth=1, and removes Range nodes, which would corrupt
            // the template's BASICDTYPEs for all subsequent clones.
            // Triggered by deeply nested parameterized interfaces (e.g.
            // outer_if containing inner_if with struct typedefs) when the
            // inner interface hasn't been specialized yet.
            bool skipWidthForTemplateStruct = false;
            {
                // Use non-asserting skip: before widthParamsEdit, type(expr)
                // constructs may contain unlinked REFDTYPEs (e.g. type(x-y))
                Ast_गाँठ_DType* स्थिर resolvedp = rawTypep ? rawTypep->skipRefOrNullp() : nullptr;
                अगर (resolvedp && (VN_IS(resolvedp, StructDType) || VN_IS(resolvedp, UnionDType))) {
                    Ast_गाँठ_Module* स्थिर ownerModp
                        = V3LinkDotIfaceCapture::findOwnerModule(resolvedp);
                    // Skip if owned by a parameterized interface (template or not
                    // yet cloned).  hasGParam() covers interfaces that haven't
                    // been cloned yet (parameterizedTemplate() not set).
                    अगर (ownerModp && VN_IS(ownerModp, Iface) && ownerModp->hasGParam()) {
                        skipWidthForTemplateStruct = true;
                        V3Stats::add_Stat_योग("Param, Template struct width skips", 1);
                        UINFO(9, "SKIP-WIDTH-TEMPLATE: struct="
                                     << resolvedp->prettyTypeName() << " templateOwner="
                                     << ownerModp->prettyNameQ() << " pin=" << pinp->prettyNameQ()
                                     << " of " << nodep->prettyNameQ()
                                     << " srcMod=" << srcModp->prettyNameQ());
                    }
                }
                अगर (rawTypep && !skipWidthForTemplateStruct) V3Width::widthParamsEdit(rawTypep);
            }
            Ast_गाँठ_DType* exprp = rawTypep ? rawTypep->skipRefToNonRefp() : nullptr;
            स्थिर Ast_गाँठ_DType* origp = modvarp->skipRefToNonRefp();
            अगर (!exprp) {
                pinp->v3error("Parameter type pin value isn't a type: Param "
                              << pinp->prettyNameQ() << " of " << nodep->prettyNameQ());
            } अन्यथा अगर (!origp) {
                pinp->v3error("Parameter type variable isn't a type: Param "
                              << modvarp->prettyNameQ());
            } अन्यथा {
                UINFO(9, "Parameter type assignment expr=" << exprp << " to " << origp);
                अगर (!skipWidthForTemplateStruct) {
                    V3Const::constifyParamsEdit(pinp->exprp());  // Reconcile typedefs
                    // Constify may have caused pinp->exprp to change
                    rawTypep = VN_AS(pinp->exprp(), गाँठ_DType);
                    exprp = rawTypep->skipRefToNonRefp();
                }
                अगर (!modvarp->fwdType().is_गाँठ_Compatible(exprp)) {
                    pinp->v3error("Parameter type expression type "
                                  << exprp->prettyDTypeNameQ()
                                  << " violates parameter's forwarding type '"
                                  << modvarp->fwdType().ascii() << "'");
                }
                अगर (exprp->similarDType(origp) || classTypeMatchesDefaultClone(exprp, origp)) {
                    // Setting parameter to its default value.  Just ignore it.
                    // This prevents making additional modules, and makes coverage more
                    // obvious as it won't show up under a unique module page name.
                } अन्यथा {
                    अगर (!skipWidthForTemplateStruct) {
                        VL_DO_DANGLING(V3Const::constifyParamsEdit(exprp), exprp);
                        rawTypep = VN_CAST(pinp->exprp(), गाँठ_DType);
                        exprp = rawTypep ? rawTypep->skipRefToNonRefp() : nullptr;
                    }
                    // Deparameterize ClassRefDType before name generation (#7000)
                    अगर (AstClassRefDType* स्थिर classRefDTypep = VN_CAST(exprp, ClassRefDType)) {
                        अगर (classRefDTypep->paramsp() && classRefDTypep->classp()
                            && classRefDTypep->classp()->hasGParam()) {
                            classRefDeparam(classRefDTypep, classRefDTypep->classp());
                        }
                    }
                    overridingTypePinsr.emplace(pinp);  // Named once every pin is prepared
                }
            }
        } अन्यथा {
            pinp->v3fatalSrc("Parameter not found in sub-module: Param "
                             << pinp->prettyNameQ() << " of " << nodep->prettyNameQ());
        }
    }

    व्योम cellInterfaceCleanup(AstPin* pinsp, Ast_गाँठ_Module* srcModp, string& longnamer,
                              bool& any_overridesr, IfaceRefRefs& ifaceRefRefs) {
        क्रम (AstPin* pinp = pinsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            स्थिर AstVar* स्थिर modvarp = pinp->modVarp();
            अगर (modvarp && VN_IS(arraySubDTypeDeepp(modvarp->subDTypep()), IfaceGenericDType)) {
                जारी;
            }
            अगर (modvarp->isIfaceRef()) {
                // arraySubDTypeDeepp returns input unchanged if not an array.
                AstIfaceRefDType* स्थिर portIrefp
                    = VN_CAST(arraySubDTypeDeepp(modvarp->subDTypep()), IfaceRefDType);
                स्थिर Ast_गाँठ* स्थिर exprp = pinp->exprp();
                अगर (VN_IS(exprp, CellArrayRef)) {
                    // CellArrayRef not yet linked; V3LinkDot resolves this pin later.
                    UINFO(9, "Skipping interface cleanup for CellArrayRef pin: " << pinp);
                    जारी;
                }
                AstIfaceRefDType* pinIrefp = nullptr;
                // Pin is a VarRef to a var of (possibly arrayed) iface type.
                अगर (स्थिर Ast_गाँठ_Var_Ref* स्थिर vrp = VN_CAST(exprp, गाँठ_Var_Ref)) {
                    अगर (vrp->varp()) {
                        pinIrefp
                            = VN_CAST(arraySubDTypeDeepp(vrp->varp()->subDTypep()), IfaceRefDType);
                    }
                }
                // Pin is a select, of any depth, of a VarRef (e.g. SelBit/ArraySel into an iface
                // array, or a slice of a row of a multi-dimensional one)
                अगर (!pinIrefp && exprp) {
                    स्थिर Ast_गाँठ* basep = exprp;
                    जबतक (स्थिर Ast_गाँठ_Pre_Sel* स्थिर selp = VN_CAST(basep, गाँठ_Pre_Sel)) {
                        basep = selp->fromp();
                    }
                    अगर (स्थिर AstVarRef* स्थिर vrp = VN_CAST(basep, VarRef)) {
                        अगर (vrp->varp()) {
                            pinIrefp = VN_CAST(arraySubDTypeDeepp(vrp->varp()->subDTypep()),
                                               IfaceRefDType);
                        }
                    }
                }

                UINFO(9, "     portIfaceRef " << portIrefp);

                अगर (!portIrefp) {
                    pinp->v3error("Interface port " << modvarp->prettyNameQ()
                                                    << " is not an interface " << modvarp);
                } अन्यथा अगर (!pinIrefp) {
                    pinp->v3error("Interface port "
                                  << modvarp->prettyNameQ()
                                  << " is not connected to interface/modport pin expression");
                } अन्यथा {
                    UINFO(9, "     pinIfaceRef " << pinIrefp);
                    अगर (portIrefp->ifaceViaCellp() != pinIrefp->ifaceViaCellp()) {
                        UINFO(9, "     IfaceRefDType needs reconnect  " << pinIrefp);
                        longnamer += ("_" + paramSmallName(srcModp, pinp->modVarp())
                                      + param_मूल्य_संख्या(pinIrefp));
                        any_overridesr = true;
                        ifaceRefRefs.emplace_back(portIrefp, pinIrefp);
                        अगर (portIrefp->ifacep() != pinIrefp->ifacep()
                            // Might be different only due to param cloning, so check names too
                            && portIrefp->ifaceName() != pinIrefp->ifaceName()) {
                            pinp->v3error("Port " << pinp->prettyNameQ() << " expects "
                                                  << Ast_गाँठ::prettyNameQ(portIrefp->ifaceName())
                                                  << " interface but pin connects "
                                                  << Ast_गाँठ::prettyNameQ(pinIrefp->ifaceName())
                                                  << " interface");
                        }
                    }
                }
            }
        }
    }

    व्योम storeOriginalParams(Ast_गाँठ_Module* स्थिर classp) {
        क्रम (Ast_गाँठ* stmtp = classp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            Ast_गाँठ* originalParamp = nullptr;
            अगर (AstVar* स्थिर varp = VN_CAST(stmtp, Var)) {
                अगर (varp->isGParam()) originalParamp = varp->clonep();
            } अन्यथा अगर (AstParamTypeDType* स्थिर ptp = VN_CAST(stmtp, ParamTypeDType)) {
                अगर (ptp->isGParam()) originalParamp = ptp->clonep();
            }
            अगर (originalParamp) m_originalParams[stmtp] = originalParamp;
        }
    }

    // Set interfaces types inside generic modules
    // to the corresponding values of implicit parameters
    व्योम genericInterfaceVarSetup(स्थिर AstPin* स्थिर paramsp, स्थिर AstPin* स्थिर pinsp) {
        std::unordered_map<string, स्थिर AstPin*> paramspMap;
        क्रम (स्थिर AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            अगर (VString::startsWith(pinp->name(), "__VGIfaceParam")) {
                paramspMap.जोड़ो({pinp->name().substr(std::म_माप("__VGIfaceParam")), pinp});
            }
        }

        अगर (paramspMap.empty()) वापस;

        क्रम (स्थिर Ast_गाँठ* nodep = pinsp; nodep; nodep = nodep->nextp()) {
            अगर (स्थिर AstPin* स्थिर pinp = VN_CAST(nodep, Pin)) {
                अगर (AstVar* स्थिर varp = pinp->modVarp()) {
                    Ast_गाँठ_DType* dtypep = varp->childDTypep()->elemDTypep();
                    अगर (AstIfaceGenericDType* स्थिर ifaceGDTypep
                        = VN_CAST(dtypep, IfaceGenericDType)) {
                        स्थिर स्वतः iter = paramspMap.find(varp->name());
                        अगर (iter == paramspMap.end()) जारी;
                        Ast_गाँठ* स्थिर backp = ifaceGDTypep->backp();
                        ifaceGDTypep->unlinkFrBack();
                        स्थिर AstPin* स्थिर paramp = iter->second;
                        paramspMap.erase(iter);
                        स्थिर AstIfaceRefDType* स्थिर ifacerefp
                            = VN_AS(paramp->exprp(), IfaceRefDType);
                        AstIfaceRefDType* स्थिर newIfacerefp = नया AstIfaceRefDType{
                            ifaceGDTypep->fileline(), ifaceGDTypep->modportFileline(),
                            ifaceGDTypep->name(), ifacerefp->ifaceName(),
                            ifaceGDTypep->modportName()};
                        newIfacerefp->ifacep(ifacerefp->ifacep());
                        अगर (स्वतः* स्थिर arrDtp = VN_CAST(backp, गाँठ_Array_DType)) {
                            arrDtp->childDTypep(newIfacerefp);
                        } अन्यथा अगर (स्वतः* स्थिर arrDtp = VN_CAST(backp, BracketArrayDType)) {
                            arrDtp->childDTypep(newIfacerefp);
                        } अन्यथा {
                            varp->childDTypep(newIfacerefp);
                        }
                        VL_DO_DANGLING(m_deleter.धकेलो_Deletep(ifaceGDTypep), ifaceGDTypep);
                        अगर (paramspMap.empty()) वापस;
                    }
                }
            }
        }

        UASSERT(paramspMap.empty(), "Not every generic interface implicit param is used");
    }

    व्योम resolveDefaultParams(Ast_गाँठ* nodep) {
        AstClassOrPackageRef* classOrPackageRef = VN_CAST(nodep, ClassOrPackageRef);
        AstClassRefDType* classRefDType = VN_CAST(nodep, ClassRefDType);
        AstClass* classp = nullptr;
        AstPin* paramsp = nullptr;

        अगर (classOrPackageRef) {
            classp = VN_CAST(classOrPackageRef->classOrPackageSkipp(), Class);
            अगर (!classp) वापस;  // No parameters in packages
            paramsp = classOrPackageRef->paramsp();
        } अन्यथा अगर (classRefDType) {
            classp = classRefDType->classp();
            paramsp = classRefDType->paramsp();
        } अन्यथा {
            nodep->v3fatalSrc("resolveDefaultParams called on node which is not a Class ref");
        }

        UASSERT_OBJ(classp, nodep, "Class or interface ref has no classp/ifacep");

        // Get the parameter list for this class
        m_classTypeParams.सफा();
        m_param_सूचकांक.सफा();
        क्रम (Ast_गाँठ* stmtp = classp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            अगर (AstParamTypeDType* paramTypep = VN_CAST(stmtp, ParamTypeDType)) {
                // Only consider formal class type parameters (generic parameters),
                // not localparam type declarations inside the class body.
                अगर (!paramTypep->isGParam()) जारी;
                m_param_सूचकांक.emplace(paramTypep, static_cast<पूर्णांक>(m_classTypeParams.आकार()));
                m_classTypeParams.emplace_back(paramTypep, -1);
            }
        }

        // For each parameter, detect either a dependent default (copy from previous param)
        // or a direct type default (for ex. int). Store dependency index in
        // m_classParams[i].second, default type in defaultTypeNodes[i].
        std::vector<Ast_गाँठ_DType*> defaultTypeNodes(m_classTypeParams.आकार(), nullptr);
        क्रम (माप_प्रकार i = 0; i < m_classTypeParams.आकार(); ++i) {
            AstParamTypeDType* स्थिर paramTypep = m_classTypeParams[i].first;
            // Parser places defaults/constraints under childDTypep as AstRequireDType
            AstRequireDType* स्थिर reqDtp = VN_CAST(paramTypep->getChildDTypep(), RequireDType);
            अगर (!reqDtp) जारी;

            // If default is a reference to another param type, record dependency
            अगर (AstRefDType* स्थिर refDtp = VN_CAST(reqDtp->subDTypep(), RefDType)) {
                अगर (AstParamTypeDType* स्थिर sourceParamp
                    = VN_CAST(refDtp->refDTypep(), ParamTypeDType)) {
                    स्वतः it = m_param_सूचकांक.find(sourceParamp);
                    अगर (it != m_param_सूचकांक.end()) { m_classTypeParams[i].second = it->second; }
                    जारी;  // dependency handled
                }
            }

            // If default is a direct type (for ex. int)
            // also record dependency
            अगर (Ast_गाँठ_DType* स्थिर dtp = VN_CAST(reqDtp->lhsp(), गाँठ_DType)) {
                defaultTypeNodes[i] = dtp;
            }
        }

        // Count existing pins and capture them by index for easy lookup
        // Type parameters when named can be given in any order, but later we depend
        // on the order when handling defaults. So here we reorder them
        // for proper handling. The remaining ones are just pushed back
        std::vector<AstPin*> pins_By_सूचकांक;
        pins_By_सूचकांक.resize(m_classTypeParams.आकार(), nullptr);
        क्रम (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            AstParamTypeDType* स्थिर typep = pinp->modPTypep();
            स्थिर स्वतः it = typep ? m_param_सूचकांक.find(typep) : m_param_सूचकांक.end();
            अगर (it != m_param_सूचकांक.end()) {
                pins_By_सूचकांक[it->second] = pinp;
            } अन्यथा {
                pins_By_सूचकांक.धकेलो_back(pinp);
            }
        }

        // For each missing parameter, get its pin from dependency or direct default.
        // A dependent default can reference a parameter declared later (not IEEE, but
        // we support it here), so repeat until no further parameter can be resolved.
        क्रम (bool resolvedSome = true; resolvedSome;) {
            resolvedSome = false;
            क्रम (माप_प्रकार paramIdx = 0; paramIdx < m_classTypeParams.आकार(); paramIdx++) {
                अगर (pins_By_सूचकांक[paramIdx]) जारी;
                स्थिर पूर्णांक sourceParamIdx = m_classTypeParams[paramIdx].second;

                AstPin* newPinp = nullptr;

                // Case 1: Dependent default -> clone the source pin's type, but only once
                // the source parameter itself has been resolved (it might be listed later)
                अगर (sourceParamIdx >= 0 && pins_By_सूचकांक[sourceParamIdx]) {
                    newPinp = pins_By_सूचकांक[sourceParamIdx]->clone_वृक्ष(false);
                }

                // Case 2: Direct default type (e.g., int), create a new pin with that dtype
                अगर (!newPinp && defaultTypeNodes[paramIdx]) {
                    Ast_गाँठ_DType* स्थिर dtypep = defaultTypeNodes[paramIdx];
                    newPinp = नया AstPin{dtypep->fileline(), static_cast<पूर्णांक>(paramIdx) + 1,
                                         "__paramNumber" + cvtToStr(paramIdx + 1),
                                         dtypep->clone_वृक्ष(false)};
                }

                अगर (newPinp) {
                    newPinp->name("__paramNumber" + cvtToStr(paramIdx + 1));
                    newPinp->param(true);
                    newPinp->modPTypep(m_classTypeParams[paramIdx].first);
                    अगर (classOrPackageRef) {
                        classOrPackageRef->addParamsp(newPinp);
                    } अन्यथा अगर (classRefDType) {
                        classRefDType->addParamsp(newPinp);
                    }
                    // Update local tracking so future dependent defaults can find it
                    pins_By_सूचकांक[paramIdx] = newPinp;
                    अगर (!paramsp) paramsp = newPinp;
                    resolvedSome = true;
                }
            }
        }
    }

    Ast_गाँठ_Module* गाँठ_Deparam_Common(Ast_गाँठ* nodep, Ast_गाँठ_Module* srcModp, AstPin* paramsp,
                                     AstPin* pinsp, bool any_overrides) {
        // Returns new or reused module
        // Make sure constification worked
        // Must be a separate loop, as constant conversion may have changed some pointers.
        string longname = srcModp->name() + "_";
        अगर (debug() >= 9 && paramsp) paramsp->dump_वृक्ष_And_अग्र(अ_बाहर, "-  cellparams: ");

        अगर (srcModp->hierBlock()) {
            longname = parameterizedHierBlockName(srcModp, paramsp);
            any_overrides = longname != srcModp->name();
        } अन्यथा {
            // Prepare every pin before naming any, as naming a value can depend on the others:
            // fold each value pin, as a class reference's may not be yet, and resolve each type
            std::unordered_set<स्थिर AstPin*> overridingTypePins;
            क्रम (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                अगर (!pinp->exprp()) जारी;  // An empty override keeps the default
                अगर (pinp->modPTypep()) {
                    cellPinCleanup(nodep, pinp, paramsp, srcModp, longname /*ref*/,
                                   any_overrides /*ref*/, overridingTypePins /*ref*/);
                    जारी;
                }
                AstVar* स्थिर modvarp = pinp->modVarp();
                अगर (!modvarp || !modvarp->isGParam()) जारी;
                resolveDeferredDotsReachableFrom(modvarp->subDTypep(), m_modp);
                // An untyped pattern can't be folded without its parameter's type
                स्थिर AstPattern* स्थिर patternp = VN_CAST(pinp->exprp(), Pattern);
                अगर (patternp && !patternp->childDTypep()) जारी;
                अगर (!VN_IS(pinp->exprp(), Const) && !is_Aggregate_Param_मूल्य(pinp->exprp())) {
                    // The pin, so a conditional of patterns is typed as an override's value
                    V3Const::constifyParamsEdit(pinp);
                }
            }
            क्रम (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                अगर (AstParamTypeDType* स्थिर modptp = pinp->modPTypep()) {
                    अगर (!overridingTypePins.गिनती(pinp)) जारी;
                    Ast_गाँठ_DType* स्थिर typep = VN_AS(pinp->exprp(), गाँठ_DType);
                    longname += "_" + paramSmallName(srcModp, modptp)
                                + param_मूल्य_संख्या(typep->skipRefToNonRefp());
                    any_overrides = true;
                    जारी;
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
        bool defaultsResolved = false;
        अगर (!any_overrides && !VN_IS(srcModp, Iface) && srcModp->hasGParam()) {
            क्रम (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                अगर (pinp->modPTypep()) {
                    any_overrides = true;
                    defaultsResolved = true;
                    अवरोध;
                }
            }
        }
        अगर (!any_overrides && VN_IS(nodep, Cell) && hasDescendantDefparams(srcModp)) {
            longname += "__Vdefparam" + V3Hash{srcModp->someInstanceName()}.toString();
            any_overrides = true;
        }

        UINFO(9,
              "nodeDeparamCommon: " << srcModp->prettyNameQ() << " overrides=" << any_overrides);

        Ast_गाँठ_Module* newModp = nullptr;
        अगर (m_hierBlocks.hierSubRun() && m_hierBlocks.isHierBlock(srcModp->origName())) {
            Ast_गाँठ_Module* स्थिर paramedModp
                = m_hierBlocks.findByParams(srcModp->origName(), paramsp, m_modp);
            अगर (!paramedModp) वापस nullptr;  // Unsupported, already reported
            paramedModp->dead(false);
            // We need to relink the pins to the new module
            relinkPinsByName(pinsp, paramedModp);
            newModp = paramedModp;
            // any_overrides = true;  // Unused later, so not needed
        } अन्यथा अगर (!any_overrides) {
            UINFO(9, "nodeDeparamCommon: no overrides, reusing " << srcModp);
            UINFO(8, "Cell parameters all match original values, skipping expansion.");
            // If it's the first use of the default instance, create a copy and store it in user3p.
            // user3p will also be used to check if the default instance is used.
            अगर (!srcModp->user3p() && (VN_IS(srcModp, Class) || VN_IS(srcModp, Iface))) {
                Ast_गाँठ_Module* गाँठ_Copyp = srcModp->clone_वृक्ष(false);
                // It is a temporary copy of the original class node, stored in order to create
                // another instances. It is needed only during class instantiation.
                UINFO(8, "    Created clone " << गाँठ_Copyp);
                m_deleter.धकेलो_Deletep(गाँठ_Copyp);  // nodeCopyp used past here
                srcModp->user3p(गाँठ_Copyp);
                storeOriginalParams(गाँठ_Copyp);
            }
            newModp = srcModp;
        } अन्यथा {
            स्थिर string newname
                = srcModp->hierBlock() ? longname : moduleCalcName(srcModp, longname);
            स्थिर ModInfo* स्थिर modInfop
                = moduleFindOrClone(srcModp, nodep, paramsp, newname, ifaceRefRefs);
            अगर (!modInfop) वापस nullptr;
            // We need to relink the pins to the new module
            relinkPinsByName(pinsp, modInfop->m_modp);
            UINFO(8, "     Done with " << modInfop->m_modp);
            newModp = modInfop->m_modp;
        }

        स्थिर bool cloned = (newModp != srcModp);
        UINFO(9, "nodeDeparamCommon result: " << newModp->prettyNameQ() << " cloned=" << cloned);

        // ParamVisitor skips the body of a class still marked hasGParam(), relying on it
        // being visited through a reference instead.  A class reached only by a deferred
        // class-scoped reference (e.g. the 'C#(V)::t' default of a type parameter that the
        // instantiation overrides) is never visited, so record specializations here.
        // user2() is set once processWorkQ has elaborated the body, which needs no re-queue.
        AstClass* स्थिर newClassp = VN_CAST(newModp, Class);
        अगर (newClassp && !newClassp->user2()) m_specializedClassps.धकेलो_back(newClassp);

        // Link source class to its specialized version for later relinking of method references
        अगर (defaultsResolved) srcModp->user4p(newModp);

        क्रम (स्वतः* stmtp = newModp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            अगर (AstParamTypeDType* dtypep = VN_CAST(stmtp, ParamTypeDType)) {
                अगर (VN_IS(dtypep->skipRefOrNullp(), VoidDType)) {
                    nodep->v3error(
                        "Class parameter type without default value is never given value"
                        << " (IEEE 1800-2023 6.20.1): " << dtypep->prettyNameQ());
                }
            }
            अगर (AstVar* स्थिर varp = VN_CAST(stmtp, Var)) {
                अगर (VN_IS(newModp, Class) && varp->isParam() && !varp->valuep()) {
                    nodep->v3error("Class parameter without default value is never given value"
                                   << " (IEEE 1800-2023 6.20.1): " << varp->prettyNameQ());
                }
            }
        }

        genericInterfaceVarSetup(paramsp, pinsp);

        // Delete the parameters from the cell; they're not relevant any longer.
        अगर (paramsp) VL_DO_DANGLING(paramsp->unlink_Fr_Back_With_अग्र()->delete_वृक्ष(), paramsp);
        वापस newModp;
    }

    Ast_गाँठ_Module* cellDeparam(AstCell* nodep, Ast_गाँठ_Module* srcModp) {
        // Must always clone __Vrcm (recursive modules)
        Ast_गाँठ_Module* स्थिर newModp = गाँठ_Deparam_Common(nodep, srcModp, nodep->paramsp(),
                                                         nodep->pinsp(), nodep->recursive());
        अगर (!newModp) वापस nullptr;
        nodep->modp(newModp);  // Might be unchanged if not cloned (newModp == srcModp)
        nodep->modName(newModp->name());
        nodep->recursive(false);
        वापस newModp;
    }
    Ast_गाँठ_Module* ifaceRefDeparam(AstIfaceRefDType* स्थिर nodep, Ast_गाँठ_Module* srcModp) {
        // Check for self-reference pattern: typedef iface#(T) this_type inside interface iface
        // When processing inside a specialized interface, the IfaceRefDType should point to
        // the owner interface, not create an intermediate specialization.
        अगर (m_modp && VN_IS(m_modp, Iface)) {
            AstIface* ownerIfacep = const_cast<AstIface*>(VN_AS(m_modp, Iface));
            स्थिर string ownerOrigName
                = ownerIfacep->origName().empty() ? ownerIfacep->name() : ownerIfacep->origName();
            स्थिर string srcOrigName
                = srcModp->origName().empty() ? srcModp->name() : srcModp->origName();
            string ownerBaseName = ownerOrigName;
            स्थिर माप_प्रकार ownerPos = ownerBaseName.find("__");
            अगर (ownerPos != string::npos) ownerBaseName = ownerBaseName.substr(0, ownerPos);
            string srcBaseName = srcOrigName;
            स्थिर माप_प्रकार srcPos = srcBaseName.find("__");
            अगर (srcPos != string::npos) srcBaseName = srcBaseName.substr(0, srcPos);

            अगर (ownerBaseName == srcBaseName) {
                bool allOwnParams = true;
                क्रम (AstPin* pinp = nodep->paramsp(); pinp && allOwnParams;
                     pinp = VN_AS(pinp->nextp(), Pin)) {
                    अगर (AstRefDType* स्थिर refp = VN_CAST(pinp->exprp(), RefDType)) {
                        अगर (AstParamTypeDType* स्थिर ptdp
                            = VN_CAST(refp->refDTypep(), ParamTypeDType)) {
                            Ast_गाँठ_Module* स्थिर ptdOwnerp
                                = V3LinkDotIfaceCapture::findOwnerModule(ptdp);
                            अगर (ptdOwnerp != m_modp) allOwnParams = false;
                        } अन्यथा {
                            pinp->v3error(  // LCOV_EXCL_LINE
                                "Self-referencing interface typedef "
                                "parameter is not a type parameter of "
                                "the enclosing interface");
                        }
                    } अन्यथा {
                        pinp->v3error(  // LCOV_EXCL_LINE
                            "Self-referencing interface typedef "
                            "parameter is not a type reference");
                    }
                }
                अगर (allOwnParams) {
                    UINFO(5, "ifaceRefDeparam: self-reference pattern detected in "
                                 << ownerIfacep->prettyNameQ() << ", using owner interface");
                    V3Stats::add_Stat_योग("Param, Self-reference iface typedefs", 1);
                    nodep->ifacep(ownerIfacep);
                    अगर (nodep->paramsp()) nodep->paramsp()->unlink_Fr_Back_With_अग्र()->delete_वृक्ष();
                    वापस ownerIfacep;
                }
            }
        }

        Ast_गाँठ_Module* स्थिर newModp
            = गाँठ_Deparam_Common(nodep, srcModp, nodep->paramsp(), nullptr, false);
        अगर (!newModp) वापस nullptr;
        nodep->ifacep(VN_AS(newModp, Iface));
        वापस newModp;
    }
    Ast_गाँठ_Module* classRefDeparam(AstClassOrPackageRef* nodep, Ast_गाँठ_Module* srcModp) {
        resolveDefaultParams(nodep);
        Ast_गाँठ_Module* स्थिर newModp
            = गाँठ_Deparam_Common(nodep, srcModp, nodep->paramsp(), nullptr, false);
        अगर (!newModp) वापस nullptr;
        nodep->classOrPackagep(newModp);  // Might be unchanged if not cloned (newModp == srcModp)

        // If this ClassOrPackageRef is a child of a RefDType (e.g., typedef class#(T)::member_t),
        // resolve the RefDType's typedef to point to the typedef inside the specialized class
        AstRefDType* स्थिर refDTypep = VN_CAST(nodep->backp(), RefDType);
        AstClass* स्थिर newClassp = refDTypep ? VN_CAST(newModp, Class) : nullptr;
        अगर (newClassp && !refDTypep->typedefp() && !refDTypep->subDTypep()) {
            अगर (AstTypedef* स्थिर typedefp
                = VN_CAST(m_memberMap.findMember(newClassp, refDTypep->name()), Typedef)) {
                refDTypep->typedefp(typedefp);
                refDTypep->classOrPackagep(newClassp);
                UINFO(9, "Resolved parameterized class typedef: " << refDTypep->name() << " -> "
                                                                  << typedefp << " in "
                                                                  << newClassp->name());
            }
        }
        वापस newModp;
    }
    Ast_गाँठ_Module* classRefDeparam(AstClassRefDType* nodep, Ast_गाँठ_Module* srcModp) {
        resolveDefaultParams(nodep);
        Ast_गाँठ_Module* स्थिर newModp
            = गाँठ_Deparam_Common(nodep, srcModp, nodep->paramsp(), nullptr, false);
        अगर (!newModp) वापस nullptr;
        AstClass* स्थिर newClassp = VN_AS(newModp, Class);
        nodep->classp(newClassp);  // Might be unchanged if not cloned (newModp == srcModp)
        nodep->classOrPackagep(newClassp);
        वापस newClassp;
    }

    // Resolve deferred class-scoped references in post-order. Resolution may
    // deparameterize a class and delete its parameter pins, so no pointer to a
    // child may remain pending when its parent is resolved.
    श्रेणी DeferredResolverVisitor final : खुला VNVisitor {
        ParamProcessor& m_processor;  // Processor used to resolve deferred references
        std::set<स्थिर Ast_गाँठ*> m_reachedDecls;

        bool firstReach(स्थिर Ast_गाँठ* स्थिर declp) { वापस m_reachedDecls.जोड़ो(declp).second; }

        व्योम iterateParamType(AstParamTypeDType* स्थिर paramTypep) {
            अगर (firstReach(paramTypep)) iterate(paramTypep);
        }
        व्योम iterateTypedef(AstTypedef* स्थिर tdefp) {
            अगर (firstReach(tdefp)) iterate(tdefp->subDTypep());
        }

        व्योम visit(Ast_गाँठ* nodep) override { iterateChildren(nodep); }
        व्योम visit(AstClassOrPackageRef* nodep) override {
            iterateChildren(nodep);
            Ast_गाँठ* स्थिर targetp = nodep->classOrPackageNodep();
            अगर (AstTypedef* स्थिर tdefp = VN_CAST(targetp, Typedef)) {
                iterateTypedef(tdefp);
            } अन्यथा अगर (AstParamTypeDType* स्थिर paramTypep = VN_CAST(targetp, ParamTypeDType)) {
                iterateParamType(paramTypep);
            }
        }
        व्योम visit(AstDot* nodep) override {
            iterateChildren(nodep);
            m_processor.resolveDotToTypedef(nodep);
        }
        व्योम visit(AstRefDType* nodep) override {
            iterateChildren(nodep);
            AstTypedef* स्थिर tdefp = nodep->typedefp();
            अगर (tdefp) {
                iterateTypedef(tdefp);
            } अन्यथा अगर (AstParamTypeDType* स्थिर paramTypep
                       = VN_CAST(nodep->refDTypep(), ParamTypeDType)) {
                iterateParamType(paramTypep);
            }
            m_processor.resolveParamClassRefDType(nodep);
        }
        व्योम visit(AstVarRef* nodep) override {
            iterateChildren(nodep);
            AstVar* स्थिर varp = nodep->varp();
            अगर (firstReach(varp)) {
                iterate(varp->subDTypep());
                स्थिर स्वतः& deferredVarps = v3Global.rootp()->deferredParamVarps();
                अगर (varp->varType() == VVarType::LPARAM && deferredVarps.गिनती(varp)) {
                    UASSERT_OBJ(varp->valuep(), varp, "VarRef should have non-null valuep");
                    iterate(varp->valuep());
                }
            }
        }

    खुला:
        DeferredResolverVisitor(ParamProcessor& processor, Ast_गाँठ* rootp)
            : m_processor{processor} {
            iterate(rootp);
        }
    };

खुला:
    // After an interface cell inside parentModp has been deparameterized
    // (rewired from template to clone), retarget REFDTYPEs that still
    // reference the old template's types so that $bits(iface_typedef)
    // evaluates with the clone's widths.
    व्योम retargetIfaceRefs(Ast_गाँठ_Module* parentModp, स्थिर string& cellName) {
        Ast_गाँठ_Module* स्थिर correctModp
            = V3LinkDotIfaceCapture::followCellPath(parentModp, cellName);
        अगर (!correctModp || correctModp->dead() || correctModp->parameterizedTemplate()) वापस;
        parentModp->foreach([&](AstRefDType* refp) {
            स्थिर VIfaceCaptureTag* स्थिर tagp = V3LinkDotIfaceCapture::captureTag(refp);
            अगर (!tagp || tagp->m_cellPath != cellName) वापस;
            // Only retarget REFDTYPEs owned by parentModp.
            अगर (V3LinkDotIfaceCapture::findOwnerModule(refp) != parentModp) वापस;
            अगर (V3LinkDotIfaceCapture::retargetRefToModule(refp, correctModp)) {
                UINFO(9, "retargetIfaceRefs: " << refp << " -> " << correctModp->prettyNameQ());
            }
        });
    }

    // Resolve deferred class-scoped values and types reachable from rootp. In
    // addition to AST children, follow typedefs, parameter types, deferred
    // localparam values, and referenced-variable dtypes. This makes the walk
    // independent of the particular dtype wrappers around a RefDType.
    व्योम resolveDeferredDotsReachableFrom(Ast_गाँठ* rootp, स्थिर Ast_गाँठ_Module* modp) {
        VL_RESTORER(m_modp);
        m_modp = modp;
        DeferredResolverVisitor{*यह, rootp};
    }

    Ast_गाँठ_Module* गाँठ_Deparam(Ast_गाँठ* nodep, Ast_गाँठ_Module* srcModp, Ast_गाँठ_Module* modp,
                               स्थिर string& someInstanceName) {
        // Return new or reused de-parameterized module
        m_modp = modp;
        // Cell: Check for parameters in the instantiation.
        // We always run this, even if no parameters, as need to look for interfaces,
        // and remove any recursive references
        UINFO(4, "De-parameterize: " << nodep);
        UINFO(9, "nodeDeparam ENTER node=<"
                     << Ast_गाँठ::गाँठ_Addr(nodep) << ">" << " type=" << nodep->typeName()
                     << " srcMod=" << (srcModp ? srcModp->prettyNameQ() : "'<null>'")
                     << " srcSomeInstanceName='"
                     << (srcModp ? srcModp->someInstanceName() : string("<null>")) << "'"
                     << " parentMod=" << (modp ? modp->prettyNameQ() : "'<null>'")
                     << " parentSomeInstanceName='"
                     << (modp ? modp->someInstanceName() : string("<null>")) << "'"
                     << " inputSomeInstanceName='" << someInstanceName << "'");
        string गाँठ_Name = nodep->name();
        अगर (AstIfaceRefDType* स्थिर ifaceRefp = VN_CAST(nodep, IfaceRefDType)) {
            अगर (गाँठ_Name.empty()) गाँठ_Name = ifaceRefp->cellName();
        }
        स्थिर string instanceName
            = गाँठ_Name.empty() ? someInstanceName : (someInstanceName + "." + गाँठ_Name);

        अगर (AstCell* स्थिर cellp = VN_CAST(nodep, Cell)) {
            क्रम (AstPin* pinp = cellp->paramsp(); pinp;) {
                AstPin* स्थिर nextp = VN_AS(pinp->nextp(), Pin);
                अगर (!pinp->paramPath().empty() && instanceName != pinp->paramPath()
                    && !VString::endsWith(instanceName, "." + pinp->paramPath())) {
                    VL_DO_DANGLING(pinp->unlinkFrBack()->delete_वृक्ष(), pinp);
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
                     << srcModp->someInstanceName() << "'" << " node=<" << Ast_गाँठ::गाँठ_Addr(nodep)
                     << ">" << " nodeType=" << nodep->typeName());

        Ast_गाँठ_Module* newModp = nullptr;
        अगर (AstCell* स्थिर cellp = VN_CAST(nodep, Cell)) {
            newModp = cellDeparam(cellp, srcModp);
        } अन्यथा अगर (AstIfaceRefDType* स्थिर ifaceRefDTypep = VN_CAST(nodep, IfaceRefDType)) {
            newModp = ifaceRefDeparam(ifaceRefDTypep, srcModp);
        } अन्यथा अगर (AstClassRefDType* स्थिर classRefp = VN_CAST(nodep, ClassRefDType)) {
            newModp = classRefDeparam(classRefp, srcModp);
        } अन्यथा अगर (AstClassOrPackageRef* स्थिर classRefp = VN_CAST(nodep, ClassOrPackageRef)) {
            newModp = classRefDeparam(classRefp, srcModp);
        } अन्यथा {
            nodep->v3fatalSrc("Expected module parameterization");
        }
        // 'newModp' might be nullptr on error
        अगर (!newModp) वापस nullptr;

        // Set name for later warnings
        newModp->someInstanceName(instanceName);

        UINFO(9, "nodeDeparam EXIT  node=<"
                     << Ast_गाँठ::गाँठ_Addr(nodep) << ">" << " type=" << nodep->typeName()
                     << " srcMod=" << (srcModp ? srcModp->prettyNameQ() : "'<null>'")
                     << " srcSomeInstanceName='"
                     << (srcModp ? srcModp->someInstanceName() : string("<null>")) << "'"
                     << " newMod=" << (newModp ? newModp->prettyNameQ() : "'<null>'")
                     << " newSomeInstanceName='"
                     << (newModp ? newModp->someInstanceName() : string("<null>")) << "'");

        UINFO(8, "     Done with orig " << nodep);
        // if (debug() >= 10)
        // v3Global.rootp()->dumpTreeFile(v3Global.debugFilename("param-out.tree"));
        वापस newModp;
    }

    // Return, and forget, the classes specialized since the previous call
    std::vector<AstClass*> takeSpecializedClassps() {
        std::vector<AstClass*> taken;
        taken.swap(m_specializedClassps);
        वापस taken;
    }

    // CONSTRUCTORS
    explicit ParamProcessor(AstNetlist* nodep)
        : m_hierBlocks{v3Global.opt.hierBlocks(), nodep} {
        क्रम (Ast_गाँठ_Module* modp = nodep->modulesp(); modp;
             modp = VN_AS(modp->nextp(), गाँठ_Module)) {
            m_allModuleNames.जोड़ो(modp->name());
        }
    }
    ~ParamProcessor() = शेष;
    VL_UNCOPYABLE(ParamProcessor);
};

//######################################################################
// Record classes that are referenced with parameter pins

श्रेणी ClassRefUnlinkerVisitor final : खुला VNVisitor {

खुला:
    explicit ClassRefUnlinkerVisitor(AstNetlist* netlistp) { iterate(netlistp); }
    व्योम visit(AstClassOrPackageRef* nodep) override {
        अगर (nodep->paramsp()) {
            अगर (AstClass* स्थिर classp = VN_CAST(nodep->classOrPackageSkipp(), Class)) {
                अगर (!classp->user3p()) {
                    // Check if this ClassOrPackageRef is the lhsp of a DOT node
                    AstDot* स्थिर dotp = VN_CAST(nodep->backp(), Dot);
                    अगर (dotp && dotp->lhsp() == nodep) {
                        // Replace DOT with just its rhsp to avoid leaving DOT with null lhsp
                        dotp->replaceWith(dotp->rhsp()->unlinkFrBack());
                        VL_DO_DANGLING2(धकेलो_Deletep(dotp), dotp, nodep);
                    } अन्यथा {
                        VL_DO_DANGLING(धकेलो_Deletep(nodep->unlinkFrBack()), nodep);
                    }
                }
            }
        }
    }
    व्योम visit(AstClass* nodep) override {}  // don't iterate inside classes
    व्योम visit(Ast_गाँठ* nodep) override { iterateChildren(nodep); }
};

//######################################################################
//  Relink RefDType nodes to point to parameterized classes' type parameters

श्रेणी ParamClassRefDTypeRelinkVisitor final : खुला VNVisitor {

    AstClass* m_classp = nullptr;
    std::unordered_map<स्थिर AstParamTypeDType*, AstClass*> m_paramTypeClassMap;

    // Owner-tracking state for the post-clone REFDTYPE typedef retargeting
    // (see retargetRefDType()). m_ownerModp is the most recently entered
    // AstNodeModule (or AstClass). m_origNameToClone / m_ambiguous are
    // rebuilt lazily on the first AstRefDType visit per owner; the
    // m_ownerMapForp cache key avoids rebuilding when the owner is unchanged
    // (e.g. across siblings under the same parent).
    Ast_गाँठ_Module* m_ownerModp = nullptr;
    Ast_गाँठ_Module* m_ownerMapForp = nullptr;
    std::unordered_map<std::string, AstClass*> m_origNameToClone;
    std::unordered_set<std::string> m_ambiguous;

    व्योम ensureOwnerMap() {
        अगर (m_ownerMapForp == m_ownerModp) वापस;
        m_ownerMapForp = m_ownerModp;
        m_origNameToClone.सफा();
        m_ambiguous.सफा();
        अगर (!m_ownerModp) वापस;
        // Skip bare/generic class templates: their bodies are not
        // authoritative for any specialization, and their self-typedefs
        // (e.g. `typedef cls#(P,Q) this_type;` inside a parameterized class)
        // would otherwise be treated as ground truth and cause REFDTYPEs in
        // the body to be retargeted at the bare template.
        अगर (AstClass* स्थिर ownerClassp = VN_CAST(m_ownerModp, Class)) {
            अगर (ownerClassp->hasGParam()) वापस;
        }
        क्रम (Ast_गाँठ* sp = m_ownerModp->stmtsp(); sp; sp = sp->nextp()) {
            AstTypedef* स्थिर tdp = VN_CAST(sp, Typedef);
            अगर (!tdp) जारी;
            AstClassRefDType* स्थिर crdtp = VN_CAST(tdp->subDTypep(), ClassRefDType);
            अगर (!crdtp || !crdtp->classp()) जारी;
            AstClass* स्थिर targetp = crdtp->classp();
            // A typedef pointing at a still-generic template is a forward
            // declaration / unresolved reference, not ground truth.
            अगर (targetp->hasGParam()) जारी;
            स्थिर std::string origName
                = targetp->origName().empty() ? targetp->name() : targetp->origName();
            स्थिर स्वतः pair = m_origNameToClone.emplace(origName, targetp);
            // Two locally-resolved typedefs pointing at different
            // specializations of the same template -> ambiguous; we cannot
            // safely guess which one a stripped REFDTYPE meant.
            अगर (!pair.second && pair.first->second != targetp) m_ambiguous.जोड़ो(origName);
        }
    }

    // A class's name before specialization (e.g. "holder" for "holder__Tz1").
    अटल string classOrigName(स्थिर AstClass* classp) {
        वापस classp->origName().empty() ? classp->name() : classp->origName();
    }

    // Find 'name' in classp or any base class (findTypedefInModule() searches
    // only the class itself).
    अटल AstTypedef* findTypedefWithBases(AstClass* classp, स्थिर string& name) {
        क्रम (AstClass* cp = classp; cp; cp = cp->extendsp() ? cp->extendsp()->classp() : nullptr) {
            अगर (AstTypedef* स्थिर tdp = V3LinkDotIfaceCapture::findTypedefInModule(cp, name)) {
                अगर (tdp->subDTypep()) वापस tdp;
            }
        }
        वापस nullptr;
    }

    // Re-resolve REFDTYPE.typedefp/refDTypep using the containing module's
    // own resolved typedef chain. The eager retargeting in deepCloneModule
    // blindly retargets every captured entry to whichever sibling clone is
    // being created, so when a referenced parameterized class is cloned more
    // than once (e.g. driver -> driver__I5 + driver__I6), the second sibling
    // stomps the bindings made by the first. By this point each owner's
    // typedefs have been correctly bound (via classRefDeparam), so we use
    // those local typedefs as ground truth.
    व्योम retargetRefDType(AstRefDType* refp) {
        अगर (!m_ownerModp) वापस;

        // IEEE 1800-2023 8.25.1: a bare class-qualified reference to the class
        // being compiled means the current specialization, but V3LinkDot binds
        // it to the default instance, so a typedef reached through it widens
        // with the template's defaults (#8348). Rebind it to the
        // specialization's own typedef.
        अगर (AstClass* स्थिर ownerClassp = VN_CAST(m_ownerModp, Class)) {
            अगर (!ownerClassp->hasGParam() && !refp->paramsp()) {
                // What the reference resolves to: its typedef's owner if
                // linked, else the class it is qualified by.
                स्थिर AstClass* refClassp = nullptr;
                अगर (AstTypedef* स्थिर tdp = refp->typedefp()) {
                    refClassp = VN_CAST(V3LinkDotIfaceCapture::findOwnerModule(tdp), Class);
                } अन्यथा अगर (स्थिर AstClassOrPackageRef* स्थिर classRefp
                           = VN_CAST(refp->classOrPackageOpp(), ClassOrPackageRef)) {
                    refClassp = VN_CAST(classRefp->classOrPackageSkipp(), Class);
                }
                // Same originating class => bare self reference (another
                // specialization would carry #(), rejected above).
                अगर (refClassp && refClassp != ownerClassp
                    && classOrigName(refClassp) == classOrigName(ownerClassp)) {
                    AstTypedef* स्थिर selfTdp = findTypedefWithBases(ownerClassp, refp->name());
                    अगर (selfTdp) {
                        UINFO(9, "post-param REFDTYPE self-reference retarget: "
                                     << refp << " from " << refClassp->name() << " to "
                                     << ownerClassp->name());
                        refp->typedefp(selfTdp);
                        refp->classOrPackagep(V3LinkDotIfaceCapture::findOwnerModule(selfTdp));
                        refp->refDTypep(selfTdp->subDTypep());
                        वापस;
                    }
                }
            }
        }

        AstTypedef* स्थिर oldTdp = refp->typedefp();
        अगर (!oldTdp) वापस;
        AstClass* स्थिर oldOwnerp = VN_CAST(V3LinkDotIfaceCapture::findOwnerModule(oldTdp), Class);
        अगर (!oldOwnerp) वापस;

        ensureOwnerMap();
        अगर (m_origNameToClone.empty()) वापस;
        स्थिर std::string origName
            = oldOwnerp->origName().empty() ? oldOwnerp->name() : oldOwnerp->origName();
        अगर (m_ambiguous.गिनती(origName)) {
            // The owner has two locally-resolved typedefs pointing at
            // different specializations of the same template, e.g.:
            //   typedef driver #(.IW(5)) drv5_t;
            //   typedef driver #(.IW(6)) drv6_t;
            // A stripped REFDTYPE only carries the origName, so we cannot
            // tell which sibling it meant; leave it alone.
            UINFO(9, "post-param REFDTYPE retarget skipped (ambiguous owner '" << origName
                                                                               << "'): " << refp);
            वापस;
        }
        स्थिर स्वतः it = m_origNameToClone.find(origName);
        अगर (it == m_origNameToClone.end()) वापस;
        AstClass* स्थिर correctClassp = it->second;
        अगर (correctClassp == oldOwnerp) वापस;
        AstTypedef* स्थिर newTdp
            = V3LinkDotIfaceCapture::findTypedefInModule(correctClassp, refp->name());
        अगर (!newTdp) {
            // The owner map says correctClassp is the right specialization,
            // but it does not declare a typedef named refp->name(). Caveats
            // of findTypedefInModule (e.g. typedef declared in a base class,
            // not the specialization we resolved to) can land us here.
            UINFO(9, "post-param REFDTYPE retarget skipped (no typedef '"
                         << refp->name() << "' in " << correctClassp->name() << "): " << refp);
            वापस;
        }
        // typedefp / classOrPackagep / refDTypep must agree. If newTdp lacks
        // a subDTypep we cannot keep all three in sync, so skip the entire
        // retarget rather than leaving the REFDTYPE half-updated.
        अगर (!newTdp->subDTypep()) {
            UINFO(9, "post-param REFDTYPE retarget skipped (newTdp has null subDTypep): " << refp);
            वापस;
        }
        refp->typedefp(newTdp);
        refp->classOrPackagep(correctClassp);
        refp->refDTypep(newTdp->subDTypep());
        UINFO(9, "post-param REFDTYPE re-resolve: " << refp << " from " << oldOwnerp->name()
                                                    << " to " << correctClassp->name());
    }

खुला:
    explicit ParamClassRefDTypeRelinkVisitor(AstNetlist* netlistp) { iterate(netlistp); }
    व्योम visit(Ast_गाँठ_Module* nodep) override {
        VL_RESTORER(m_ownerModp);
        m_ownerModp = nodep;
        iterateChildren(nodep);
    }
    व्योम visit(AstClass* nodep) override {
        VL_RESTORER(m_classp);
        VL_RESTORER(m_ownerModp);
        m_classp = nodep;
        m_ownerModp = nodep;
        क्रम (Ast_गाँठ* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            अगर (AstParamTypeDType* स्थिर ptp = VN_CAST(stmtp, ParamTypeDType)) {
                m_paramTypeClassMap[ptp] = nodep;
            }
        }
        iterateChildren(nodep);
    }
    व्योम visit(AstRefDType* nodep) override {
        iterateChildren(nodep);
        // Redirect refs whose refDTypep points at a stale ParamTypeDType in
        // a generic class template that has been replaced by a parametrized
        // clone.
        अगर (AstParamTypeDType* स्थिर paramtypep = VN_CAST(nodep->refDTypep(), ParamTypeDType)) {
            स्थिर स्वतः it = m_paramTypeClassMap.find(paramtypep);
            अगर (it != m_paramTypeClassMap.end()) {
                AstClass* स्थिर origClassp = it->second;
                अगर (origClassp->hasGParam() && !origClassp->user3p()) {
                    अगर (AstClass* स्थिर parametrizedClassp
                        = VN_CAST(origClassp->user4p(), Class)) {
                        स्थिर string paramName = paramtypep->name();
                        AstParamTypeDType* newParamTypep = nullptr;
                        क्रम (Ast_गाँठ* stmtp = parametrizedClassp->stmtsp(); stmtp;
                             stmtp = stmtp->nextp()) {
                            अगर (AstParamTypeDType* स्थिर ptp = VN_CAST(stmtp, ParamTypeDType)) {
                                अगर (ptp->name() == paramName) {
                                    newParamTypep = ptp;
                                    अवरोध;
                                }
                            }
                        }
                        अगर (newParamTypep) {
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
    व्योम visit(Ast_गाँठ* nodep) override { iterateChildren(nodep); }
};

//######################################################################
// Process parameter top state

श्रेणी ParamState final {
खुला:
    // STATE - across all visitors
    std::vector<AstClass*> m_paramClasses;  // Parameterized classes
    std::vector<AstDot*> m_dots;  // Dot references to process
    // Work queue keyed by (!isIface, level) so interfaces are always processed
    // before non-interfaces.  This ensures interface clones have their types
    // properly widthed before any module that references those types.
    using WQKey = std::pair<bool, पूर्णांक>;
    std::multimap<WQKey, Ast_गाँठ_Module*> m_work_Queue_अग्र;  // Modules left to process
    // Map from AstNodeModule to set of all AstNodeModules that instantiates it.
    std::unordered_map<Ast_गाँठ_Module*, std::unordered_set<Ast_गाँठ_Module*>> m_parentps;
};

//######################################################################
// Process parameter visitor

श्रेणी ParamVisitor final : खुला VNVisitor {
    // NODE STATE
    // AstNodeModule::user1 -> bool: already fixed level (temporary)
    // AstNodeDType::user2  -> bool: already visited from typedef

    // STATE - across all visitors
    ParamState& m_state;  // Common state
    ParamProcessor m_processor;  // De-parameterize a cell, build modules
    GenForUnroller m_unroller;  // Unroller for AstGenFor

    bool m_iterateModule = false;  // Iterating module body
    string m_unlinkedTxt;  // Text for AstUnlinkedRef
    std::multimap<bool, Ast_गाँठ*> m_cellps;  // Cells left to process (in current module)
    std::unordered_map<स्थिर Ast_गाँठ*, std::string>
        m_genHierNames;  // Maps ast nodes to generated hierarchy names
    std::map<स्थिर AstRefDType*, bool>
        m_isCircular;  // Stores information whether `AstRefDType` is circular
    using VarsByName = std::unordered_map<std::string, AstVar*>;
    // Persists across modules; one specialized interface clone serves every module bound to it
    std::unordered_map<स्थिर Ast_गाँठ_Module*, VarsByName> m_ifaceParams;

    // STATE - for current visit position (use VL_RESTORER)
    Ast_गाँठ_Module* m_modp = nullptr;  // Module iterating
    std::unordered_set<std::string> m_ifacePortNames;  // Interface port names in current module
    std::unordered_map<std::string, AstCell*>
        m_ifaceInstCells;  // Local interface instance cells in current module, keyed by name
    VarsByName m_modIfaceRefs;  // Interface-ref Vars in current module, keyed by name
    bool m_modIfaceRefsDone = false;  // m_modIfaceRefs has been gathered for m_modp
    string m_generateHierName;  // Generate portion of hierarchy name
    bool m_inGenerateCond = false;  // Traversing a generate condition; see iterateGenerateCond

    // METHODS

    व्योम processWorkQ() {
        UASSERT(!m_iterateModule, "Should not nest");
        std::multimap<ParamState::WQKey, Ast_गाँठ_Module*> workQueue;
        m_generateHierName = "";
        m_iterateModule = true;

        // Visit all cells under module, recursively
        जबतक (true) {
            // Classes specialized since the last pass still need their bodies elaborated
            क्रम (AstClass* स्थिर classp : m_processor.takeSpecializedClassps()) {
                m_state.m_work_Queue_अग्र.emplace(ParamState::WQKey{true, classp->level()}, classp);
            }
            अगर (workQueue.empty()) std::swap(workQueue, m_state.m_work_Queue_अग्र);
            अगर (workQueue.empty()) अवरोध;

            स्थिर स्वतः itm = workQueue.cbegin();
            Ast_गाँठ_Module* स्थिर modp = itm->second;
            workQueue.erase(itm);
            // Starting a new module, so what was learned about the last one no longer holds.
            v3Global.rootp()->clearContainingModules();

            // Process once; note user2 will be cleared on specialization, so we will do the
            // specialized module if needed
            अगर (!modp->user2SetOnce()) {
                UINFO(9, "processWorkQ module begin mod='"
                             << modp->name() << "' orig='" << modp->origName() << "'"
                             << " someInstanceName='" << modp->someInstanceName() << "'"
                             << " hasGParam=" << (modp->hasGParam() ? "yes" : "no")
                             << " user3p=" << (modp->user3p() ? "set" : "null")
                             << " dead=" << (modp->dead() ? "yes" : "no"));

                // TODO: this really should be an assert, but classes and hier_blocks are
                // special...
                अगर (modp->someInstanceName().empty()) modp->someInstanceName(modp->origName());

                // Iterate the body
                {
                    VL_RESTORER(m_modp);
                    VL_RESTORER_CLEAR(m_ifacePortNames);
                    VL_RESTORER_CLEAR(m_ifaceInstCells);
                    VL_RESTORER_CLEAR(m_modIfaceRefs);
                    VL_RESTORER(m_modIfaceRefsDone);
                    m_modp = modp;
                    m_modIfaceRefsDone = false;
                    iterateChildren(modp);
                }
            }

            // Process interface cells, then non-interface cells, which may reference an interface
            // cell.
            जबतक (!m_cellps.empty()) {
                स्थिर स्वतः itim = m_cellps.cbegin();
                Ast_गाँठ* स्थिर cellp = itim->second;
                m_cellps.erase(itim);

                // Consume the generated hierarchy name for the node
                string genHierName;
                स्थिर स्वतः genHierIt = m_genHierNames.find(cellp);
                अगर (genHierIt != m_genHierNames.end()) {
                    genHierName = std::move(genHierIt->second);
                    m_genHierNames.erase(genHierIt);
                }

                Ast_गाँठ_Module* srcModp = nullptr;
                अगर (स्थिर AstCell* modCellp = VN_CAST(cellp, Cell)) {
                    srcModp = modCellp->modp();
                } अन्यथा अगर (स्थिर AstClassOrPackageRef* classRefp
                           = VN_CAST(cellp, ClassOrPackageRef)) {
                    srcModp = classRefp->classOrPackageSkipp();
                    अगर (VN_IS(classRefp->classOrPackageNodep(), ParamTypeDType)) जारी;
                } अन्यथा अगर (स्थिर AstClassRefDType* classRefp = VN_CAST(cellp, ClassRefDType)) {
                    srcModp = classRefp->classp();
                } अन्यथा अगर (स्थिर AstIfaceRefDType* ifaceRefp = VN_CAST(cellp, IfaceRefDType)) {
                    srcModp = ifaceRefp->ifacep();
                } अन्यथा {
                    cellp->v3fatalSrc("Expected module parameterization");
                }
                अगर (!srcModp) जारी;

                // Update path
                स्थिर string someInstanceName = modp->someInstanceName() + genHierName;

                // Apply parameter specialization
                अगर (Ast_गाँठ_Module* स्थिर newModp
                    = m_processor.गाँठ_Deparam(cellp, srcModp, modp, someInstanceName)) {

                    अगर (VN_IS(srcModp, Iface)) {
                        logTemplateLeakRefs(modp, srcModp, "after queued nodeDeparam", cellp);
                        // After the interface cell is rewired to its clone,
                        // retarget REFDTYPEs in the parent module that still
                        // reference the template interface's types.
                        अगर (V3LinkDotIfaceCapture::enabled()) {
                            अगर (स्थिर AstCell* स्थिर modCellp = VN_CAST(cellp, Cell)) {
                                अगर (newModp != srcModp) {
                                    m_processor.retargetIfaceRefs(modp, modCellp->name());
                                }
                            }
                        }
                    }

                    // Add the (now potentially specialized) child module to the work queue
                    workQueue.emplace(ParamState::WQKey{!VN_IS(newModp, Iface), newModp->level()},
                                      newModp);

                    // Add to the hierarchy registry
                    m_state.m_parentps[newModp].जोड़ो(modp);

                    // Eagerly specialize nested iface cells so their types
                    // have correct widths before sibling module cells run.
                    अगर (VN_IS(newModp, Iface) && newModp != srcModp) {
                        specializeNestedIfaceCells(newModp);
                    }
                }
            }
        }

        m_iterateModule = false;
    }

    // Extract the base reference name from a dotted VarXRef (e.g., "iface.FOO" -> "iface")
    अटल string getRefBaseName(स्थिर AstVarXRef* refp) {
        स्थिर string dotted = refp->dotted();
        अगर (dotted.empty()) वापस "";
        वापस dotted.substr(0, dotted.find('.'));
    }

    // Debug-only diagnostic (requires --debugi-V3Param 9).
    // Walks parentModp looking for RefDTypes or VarRefs whose typedef,
    // refDType, or variable target is still owned by templateModp (the
    // unspecialized interface template).  Any such "leak" indicates a
    // pointer that was not properly redirected to the clone during
    // deparameterization.  Logs each leak with ancestry for triage.
    व्योम logTemplateLeakRefs(Ast_गाँठ_Module* parentModp, Ast_गाँठ_Module* templateModp,
                             स्थिर अक्षर* stage, Ast_गाँठ* contextp) {
        अगर (debug() < 9 || !parentModp || !templateModp || !VN_IS(templateModp, Iface)) वापस;
        // LCOV_EXCL_START  // Debug-only diagnostic
        पूर्णांक leak_गिनती = 0;
        स्थिर स्वतः ancestryOf = [](स्थिर Ast_गाँठ* nodep) {
            string ancestry;
            क्रम (स्थिर Ast_गाँठ* curp = nodep; curp; curp = curp->backp()) {
                अगर (!ancestry.empty()) ancestry += "<-";
                ancestry += curp->typeName();
                अगर (VN_IS(curp, गाँठ_Module) || VN_IS(curp, Netlist) || VN_IS(curp, TypeTable)) {
                    अवरोध;
                }
            }
            वापस ancestry;
        };

        parentModp->foreach([&](AstRefDType* refp) {
            अगर (refp->typedefp()) {
                Ast_गाँठ_Module* स्थिर tdOwnerp
                    = V3LinkDotIfaceCapture::findOwnerModule(refp->typedefp());
                अगर (tdOwnerp == templateModp) {
                    ++leak_गिनती;
                    UINFO(9, "TEMPLATE-LEAK "
                                 << stage << " parent=" << parentModp->prettyNameQ()
                                 << " template=" << templateModp->prettyNameQ() << " contextType="
                                 << (contextp ? contextp->typeName() : string("<null>"))
                                 << " contextName="
                                 << (contextp ? contextp->prettyNameQ() : "'<null>'")
                                 << " leak=REFDTYPE typedef owner" << " ref=<"
                                 << Ast_गाँठ::गाँठ_Addr(refp) << ">" << " refName="
                                 << refp->prettyNameQ() << " ancestry=" << ancestryOf(refp));
                }
            }
            अगर (refp->refDTypep()) {
                Ast_गाँठ_Module* स्थिर rdOwnerp
                    = V3LinkDotIfaceCapture::findOwnerModule(refp->refDTypep());
                अगर (rdOwnerp == templateModp) {
                    ++leak_गिनती;
                    UINFO(9, "TEMPLATE-LEAK "
                                 << stage << " parent=" << parentModp->prettyNameQ()
                                 << " template=" << templateModp->prettyNameQ() << " contextType="
                                 << (contextp ? contextp->typeName() : string("<null>"))
                                 << " contextName="
                                 << (contextp ? contextp->prettyNameQ() : "'<null>'")
                                 << " leak=REFDTYPE refDType owner" << " ref=<"
                                 << Ast_गाँठ::गाँठ_Addr(refp) << ">" << " refName="
                                 << refp->prettyNameQ() << " ancestry=" << ancestryOf(refp));
                }
            }
        });

        parentModp->foreach([&](AstVarRef* varrefp) {
            अगर (!varrefp->varp()) वापस;
            Ast_गाँठ_Module* स्थिर varOwnerp
                = V3LinkDotIfaceCapture::findOwnerModule(varrefp->varp());
            अगर (varOwnerp != templateModp) वापस;
            ++leak_गिनती;
            UINFO(9, "TEMPLATE-LEAK "
                         << stage << " parent=" << parentModp->prettyNameQ()
                         << " template=" << templateModp->prettyNameQ()
                         << " contextType=" << (contextp ? contextp->typeName() : string("<null>"))
                         << " contextName=" << (contextp ? contextp->prettyNameQ() : "'<null>'")
                         << " leak=VARREF target owner" << " ref=<" << Ast_गाँठ::गाँठ_Addr(varrefp)
                         << ">" << " var=" << varrefp->prettyNameQ()
                         << " ancestry=" << ancestryOf(varrefp));
        });

        अगर (leak_गिनती > 0) {
            UINFO(9, "TEMPLATE-LEAK summary stage='"
                         << stage << "' parent=" << parentModp->prettyNameQ()
                         << " template=" << templateModp->prettyNameQ() << " count=" << leak_गिनती);
        }
        // LCOV_EXCL_STOP
    }

    // Flag hierarchical refs in a parameter value. Single top-down pass.
    व्योम checkParamNotHierRecurse(Ast_गाँठ* nodep, bool underTypeQuery = false) {
        क्रम (; nodep; nodep = nodep->nextp()) {
            // Refs read only for their type ($bits etc.) are allowed
            स्थिर AstAttrOf* स्थिर attrp = VN_CAST(nodep, AttrOf);
            स्थिर bool childUnderQuery
                = underTypeQuery || (attrp && attrp->attrType().isTypeQuery());
            अगर (स्थिर AstVarXRef* स्थिर refp = VN_CAST(nodep, VarXRef)) {
                // Allow hierarchical ref to interface params through interface/modport ports
                // or local interface instances
                bool isIfaceRef = false;
                अगर (refp->varp() && refp->varp()->isIfaceParam()) {
                    स्थिर string refname = getRefBaseName(refp);
                    isIfaceRef
                        = !refname.empty()
                          && (m_ifacePortNames.गिनती(refname) || m_ifaceInstCells.गिनती(refname));
                }
                अगर (!isIfaceRef && !underTypeQuery) {
                    refp->v3warn(HIERPARAM, "Parameter values cannot use hierarchical values"
                                            " (IEEE 1800-2023 6.20.2)");
                }
            } अन्यथा अगर (स्थिर Ast_गाँठ_FTask_Ref* स्थिर refp = VN_CAST(nodep, गाँठ_FTask_Ref)) {
                अगर (refp->dotted() != "") {
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
    व्योम specializeNestedIfaceCells(Ast_गाँठ_Module* ifaceModp) {
        क्रम (Ast_गाँठ* stmtp = ifaceModp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstCell* स्थिर nestedCellp = VN_CAST(stmtp, Cell);
            अगर (!nestedCellp) जारी;
            अगर (!VN_IS(nestedCellp->modp(), Iface)) जारी;
            अगर (!nestedCellp->paramsp()) जारी;

            Ast_गाँठ_Module* स्थिर nestedSrcModp = nestedCellp->modp();
            अगर (Ast_गाँठ_Module* स्थिर nestedNewModp = m_processor.गाँठ_Deparam(
                    nestedCellp, nestedSrcModp, ifaceModp, ifaceModp->someInstanceName())) {
                अगर (nestedNewModp != nestedSrcModp) {
                    // Constify the nested clone's params so its types have correct widths.
                    क्रम (Ast_गाँठ* sp = nestedNewModp->stmtsp(); sp; sp = sp->nextp()) {
                        अगर (AstVar* स्थिर varp = VN_CAST(sp, Var)) {
                            अगर (varp->isParam() && varp->valuep()) {
                                V3Const::constifyParamsEdit(varp);
                            }
                        }
                    }
                    // Retarget REFDTYPEs in the outer clone to the nested clone's types.
                    अगर (V3LinkDotIfaceCapture::enabled()) {
                        m_processor.retargetIfaceRefs(ifaceModp, nestedCellp->name());
                    }
                    specializeNestedIfaceCells(nestedNewModp);
                }
            }
        }
    }

    // Check if cell parameters reference interface ports or local interface instances
    bool cellParamsReferenceIfacePorts(AstCell* cellp) {
        अगर (!cellp->paramsp()) वापस false;

        क्रम (Ast_गाँठ* nodep = cellp->paramsp(); nodep; nodep = nodep->nextp()) {
            अगर (AstPin* स्थिर pinp = VN_CAST(nodep, Pin)) {
                अगर (Ast_गाँठ* स्थिर exprp = pinp->exprp()) {
                    अगर (स्थिर AstVarXRef* स्थिर refp = VN_CAST(exprp, VarXRef)) {
                        स्थिर string refname = getRefBaseName(refp);
                        अगर (!refname.empty()
                            && (m_ifacePortNames.गिनती(refname)
                                || m_ifaceInstCells.गिनती(refname)))
                            वापस true;
                    }
                }
            }
        }
        वापस false;
    }

    // Split interface array port 'portp' into a variable for each of its elements, in the
    // dimensions of 'dtypep' inwards. 'suffix' is the name suffix of the dimensions outside
    // 'dtypep'.
    व्योम expandIfaceArrayPortDimensions(AstVar* portp, Ast_गाँठ_DType* dtypep,
                                        स्थिर std::string& suffix) {
        स्थिर AstUnpackArrayDType* स्थिर arrp = VN_CAST(dtypep->skipRefp(), UnpackArrayDType);
        // Base case: add the element variable when no dimensions left
        अगर (!arrp) {
            AstVar* स्थिर varp = portp->clone_वृक्ष(false);
            varp->name(portp->name() + suffix);
            varp->origName(portp->origName() + suffix);
            अगर (Ast_गाँठ_DType* स्थिर oldp = varp->childDTypep()) {
                VL_DO_DANGLING(धकेलो_Deletep(oldp->unlinkFrBack()), oldp);
            }
            varp->dtypep(nullptr);
            varp->childDTypep(portp->subDTypep()->elemDTypep()->clone_वृक्ष(false));
            portp->add_अग्र_Here(varp);
            वापस;
        }

        // Enumerate the current dimension given by 'arrp'
        // Each element is added right after 'portp', so go from right to left,
        // to end with an enumeration from the left index to the right index.
        क्रम (स्थिर पूर्णांक n : arrp->declRange().seq_दायाँ_To_बायाँ()) {
            स्थिर std::string s = suffix + "__BRA__" + Ast_गाँठ::encode_संख्या(n) + "__KET__";
            expandIfaceArrayPortDimensions(portp, arrp->subDTypep(), s);
        }
    }

    // Add the elements of instance array 'arrayedCellp' in the dimensions from 'rangep' inwards.
    // 'suffix' and 'idx' are the name suffix and row-major position (each dimension counted from
    // the left) of the dimensions outside 'rangep'. For an array of interfaces, 'ifaceVarp' is
    // the variable referencing the whole array (see V3LinkCells), which is also split into
    // variables referencing the elements.
    व्योम expandCellArrayDimensions(AstCell* arrayedCellp, AstVar* ifaceVarp,
                                   स्थिर AstRange* rangep, स्थिर std::string& suffix, पूर्णांक idx) {
        // Base case: insert the element when no dimensions left
        अगर (!rangep) {
            AstCell* स्थिर elemp = arrayedCellp->clone_वृक्ष(false);
            elemp->name(arrayedCellp->name() + suffix);
            elemp->origName(arrayedCellp->origName() + suffix);
            elemp->arrayIdx(idx);
            arrayedCellp->add_अग्र_Here(elemp);
            अगर (ifaceVarp) {
                AstIfaceRefDType* स्थिर irefp
                    = VN_AS(ifaceVarp->subDTypep()->elemDTypep(), IfaceRefDType)->clone_वृक्ष(false);
                irefp->cellp(elemp);
                irefp->cellName(elemp->name());
                // Named like the variable of any interface cell (see V3LinkCells)
                AstVar* स्थिर varp = ifaceVarp->clone_वृक्ष(false);
                varp->name(elemp->name() + "__Viftop");
                varp->origName(elemp->origName() + "__Viftop");
                अगर (Ast_गाँठ_DType* स्थिर oldp = varp->childDTypep()) {
                    VL_DO_DANGLING(धकेलो_Deletep(oldp->unlinkFrBack()), oldp);
                }
                varp->dtypep(nullptr);
                varp->childDTypep(irefp);
                ifaceVarp->add_अग्र_Here(varp);
            }
            वापस;
        }

        // Enumerate the current dimension given by 'rangep'
        // Each element is added right after 'arrayedCellp', so go from right to left,
        // to end with an enumeration from the left index to the right index.
        idx = (idx + 1) * rangep->elementsConst();
        स्थिर AstRange* स्थिर subRangep = VN_AS(rangep->nextp(), Range);
        क्रम (स्थिर पूर्णांक n : rangep->seq_दायाँ_To_बायाँ()) {
            स्थिर std::string s = suffix + "__BRA__" + Ast_गाँठ::encode_संख्या(n) + "__KET__";
            expandCellArrayDimensions(arrayedCellp, ifaceVarp, subRangep, s, --idx);
        }
    }

    // A generic visitor for cells and class refs
    व्योम visitCellOrClassRef(Ast_गाँठ* nodep, bool isIface) {
        // Must do ifaces first, so push to list and do in proper order
        m_genHierNames.emplace(nodep, m_generateHierName);

        // Deparameterize iface cells early so types are available for lparams.
        अगर (isIface && VN_CAST(nodep, Cell) && VN_CAST(nodep, Cell)->paramsp()) {
            AstCell* स्थिर cellp = VN_CAST(nodep, Cell);
            अगर (!cellParamsReferenceIfacePorts(cellp)) {
                Ast_गाँठ_Module* स्थिर srcModp = cellp->modp();
                Ast_गाँठ_Module* स्थिर newModp
                    = m_processor.गाँठ_Deparam(cellp, srcModp, m_modp, m_modp->someInstanceName());
                // Retarget template REFDTYPEs to the clone's types.
                अगर (V3LinkDotIfaceCapture::enabled() && cellp->modp() != srcModp) {
                    m_processor.retargetIfaceRefs(m_modp, cellp->name());
                }
                // Specialize nested iface cells so their types are correct.
                अगर (newModp && newModp != srcModp) { specializeNestedIfaceCells(newModp); }
            }
        }

        // Visit parameters in the instantiation.
        iterateChildren(nodep);
        // A generate condition is folded and deleted before the drain loop runs, so
        // queueing from one only leaves a dangling pointer.  See iterateGenerateCond.
        अगर (!m_inGenerateCond) m_cellps.emplace(!isIface, nodep);
    }

    // VISITORS
    व्योम visit(Ast_गाँठ_Module* nodep) override {
        अगर (nodep->recursiveClone()) nodep->dead(true);  // Fake, made for recursive elimination
        अगर (nodep->dead()) वापस;  // Marked by LinkDot (and above)
        UINFO(9, "V3Param: visit module name=" << nodep->prettyNameQ() << " orig='"
                                               << nodep->origName() << "' someInstanceName='"
                                               << nodep->someInstanceName() << "' hasGParam="
                                               << (nodep->hasGParam() ? "yes" : "no")
                                               << " user3p=" << (nodep->user3p() ? "set" : "null")
                                               << " dead=" << (nodep->dead() ? "yes" : "no"));
        अगर (AstClass* स्थिर classp = VN_CAST(nodep, Class)) {
            अगर (classp->hasGParam()) {
                // Don't enter into a definition.
                // If a class is used, it will be visited through a reference and cloned
                m_state.m_paramClasses.धकेलो_back(classp);
                वापस;
            }
        }

        अगर (m_iterateModule) {  // Iterating from processWorkQ
            UINFO(4, " MOD-under-MOD.  " << nodep);
            // Delay until current module is done.
            // processWorkQ() (which we are returning to) will process nodep later
            m_state.m_work_Queue_अग्र.emplace(
                ParamState::WQKey{!VN_IS(nodep, Iface), nodep->level()}, nodep);
            वापस;
        }

        // Start traversal at root-like things
        अगर (nodep->is_शीर्ष()  // Tops
            || VN_IS(nodep, Class)  //  Moved classes
            || VN_IS(nodep, Package)) {  // Likewise haven't done wrapTopPackages yet
            m_state.m_work_Queue_अग्र.emplace(
                ParamState::WQKey{!VN_IS(nodep, Iface), nodep->level()}, nodep);
            processWorkQ();
        }
    }

    bool isCircularType(स्थिर AstRefDType* nodep) {
        स्थिर स्वतः iter = m_isCircular.emplace(nodep, true);
        अगर (!iter.second) वापस iter.first->second;
        अगर (स्थिर AstRefDType* स्थिर subDTypep = VN_CAST(nodep->subDTypep(), RefDType)) {
            स्थिर bool ret = isCircularType(subDTypep);
            iter.first->second = ret;
            वापस ret;
        }
        iter.first->second = false;
        वापस false;
    }

    व्योम visit(AstRefDType* nodep) override {
        स्थिर bool isCircular = isCircularType(nodep);
        // A module-body `typedef C#(Cfg)::t alias` the deferred pin/param walk
        // never reached would survive to V3Width unlinked. If its alias chain is
        // broken, run the same deferred resolution here.
        अगर (!isCircular && nodep->typedefp() && !nodep->skipRefOrNullp()) {
            m_processor.resolveDeferredDotsReachableFrom(nodep, m_modp);
        }
        अगर (isCircular) {
            nodep->v3error("Typedef's type is circular: " << nodep->prettyName());
        } अन्यथा अगर (nodep->typedefp() && nodep->subDTypep()
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
    व्योम visit(AstCell* nodep) override {
        अगर (nodep->rangep() && nodep->arrayIdx() < 0) {
            // Type check the ranges
            क्रम (AstRange* rangep = nodep->rangep(); rangep;
                 rangep = VN_AS(rangep->nextp(), Range)) {
                rangep = VN_AS(V3Width::widthParamsEdit(rangep), Range);
            }
            // If it's an interface, pick up the corresponding AstIfaceRefDType variable
            AstVar* स्थिर ifaceVarp = [&]() -> AstVar* {
                अगर (!VN_IS(nodep->modp(), Iface)) वापस nullptr;
                // The variable referencing the whole array, right after the cell (see V3LinkCells)
                AstVar* स्थिर varp = VN_CAST(nodep->nextp(), Var);
                UASSERT_OBJ(varp && varp->name() == nodep->name() + "__Viftop", nodep,
                            "No __Viftop variable for interface array");
                वापस varp;
            }();
            // Expand the instance array into its elements
            expandCellArrayDimensions(nodep, ifaceVarp, nodep->rangep(), "", 0);
            अगर (ifaceVarp) {
                ifaceVarp->isIfaceArraySplit(true);
                // Until V3Width removes it, the whole array variable refers to the first
                // element, which is also used for accessing the parameters of the array
                AstCell* स्थिर firstp = VN_AS(nodep->nextp(), Cell);
                AstIfaceRefDType* स्थिर irefp
                    = VN_AS(ifaceVarp->subDTypep()->elemDTypep(), IfaceRefDType);
                irefp->cellp(firstp);
                irefp->cellName(firstp->name());
                m_ifaceInstCells.emplace(nodep->name(), firstp);
            }
            VL_DO_DANGLING(धकेलो_Deletep(nodep->unlinkFrBack()), nodep);
            // The elements replaced the arrayed cell in place, so will be iterated next.
            वापस;
        }
        checkParamNotHierRecurse(nodep->paramsp());
        अगर (VN_IS(nodep->modp(), Iface)) m_ifaceInstCells.emplace(nodep->name(), nodep);
        visitCellOrClassRef(nodep, VN_IS(nodep->modp(), Iface));
    }
    व्योम visit(AstIfaceRefDType* nodep) override {
        nodep->skipRefp()->user2SetOnce();
        अगर (nodep->ifacep()) visitCellOrClassRef(nodep, true);
    }
    व्योम visit(AstClassRefDType* nodep) override {
        nodep->skipRefp()->user2SetOnce();
        checkParamNotHierRecurse(nodep->paramsp());
        visitCellOrClassRef(nodep, false);
    }
    व्योम visit(AstClassOrPackageRef* nodep) override {
        // If it points to a typedef it is not really a class reference. That typedef will be
        // visited anyway (from its parent node), so even if it points to a parameterized class
        // type, the instance will be created.
        अगर (!VN_IS(nodep->classOrPackageNodep(), Typedef)) visitCellOrClassRef(nodep, false);
    }

    // Recurse into AstGenBlock as generate blocks aren't flattened until V3Begin::debeginAll,
    // well after V3Param runs
    अटल व्योम gatherVars(Ast_गाँठ* stmtsp, bool (*matchp)(स्थिर AstVar*), VarsByName& varsr) {
        क्रम (Ast_गाँठ* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
            अगर (AstVar* स्थिर varp = VN_CAST(nodep, Var)) {
                // emplace, not assign, so the first declaration of a name wins
                अगर (matchp(varp)) varsr.emplace(varp->name(), varp);
            } अन्यथा अगर (AstGenBlock* स्थिर genp = VN_CAST(nodep, GenBlock)) {
                gatherVars(genp->itemsp(), matchp, varsr);
            }
        }
    }
    स्थिर VarsByName& modIfaceRefs() {
        अगर (!m_modIfaceRefsDone) {
            gatherVars(
                m_modp->stmtsp(), [](स्थिर AstVar* varp) { वापस varp->isIfaceRef(); },
                m_modIfaceRefs);
            m_modIfaceRefsDone = true;
        }
        वापस m_modIfaceRefs;
    }
    स्थिर VarsByName& ifaceParams(Ast_गाँठ_Module* ifacep) {
        स्थिर स्वतः pair = m_ifaceParams.emplace(ifacep, VarsByName{});
        अगर (pair.second) {
            gatherVars(
                ifacep->stmtsp(), [](स्थिर AstVar* varp) { वापस varp->isParam(); },
                pair.first->second);
        }
        वापस pair.first->second;
    }
    // Resolve a generic-interface 'ifacePort.member' Dot left unlinked by V3LinkDot, now that
    // ifacePort may have been specialized. Returns nullptr if still unresolvable
    Ast_गाँठ* tryResolveGenericIfaceDot(AstDot* dotp) {
        AstParseRef* स्थिर lhsp = VN_CAST(dotp->lhsp(), ParseRef);
        AstParseRef* स्थिर rhsp = VN_CAST(dotp->rhsp(), ParseRef);
        अगर (!lhsp || !rhsp) वापस nullptr;
        // Always called from visit(AstVar) inside processWorkQ()'s VL_RESTORER(m_modp) scope
        अगर (VL_UNCOVERABLE(!m_modp)) वापस nullptr;

        स्थिर स्वतः& refVars = modIfaceRefs();
        स्थिर स्वतः ifaceVarIt = refVars.find(lhsp->name());
        // V3LinkDot only defers Dots whose lhs is a non-array iface-ref var, so this can't fail
        अगर (VL_UNCOVERABLE(ifaceVarIt == refVars.end())) वापस nullptr;
        AstVar* स्थिर ifaceVarp = ifaceVarIt->second;
        AstIfaceRefDType* स्थिर ifacerefp = VN_CAST(ifaceVarp->subDTypep(), IfaceRefDType);
        अगर (VL_UNCOVERABLE(!ifacerefp || !ifacerefp->ifacep())) वापस nullptr;

        स्थिर स्वतः& ifaceVars = ifaceParams(ifacerefp->ifacep());
        स्थिर स्वतः targetVarIt = ifaceVars.find(rhsp->name());
        अगर (targetVarIt == ifaceVars.end()) वापस nullptr;
        AstVar* स्थिर targetVarp = targetVarIt->second;
        // processWorkQ() visits interface cells first, so the interface is already constified
        अगर (VL_UNCOVERABLE(!VN_IS(targetVarp->valuep(), Const))) iterate(targetVarp);
        अगर (VL_UNCOVERABLE(!targetVarp->valuep() || !VN_IS(targetVarp->valuep(), Const))) {
            वापस nullptr;  // LCOV_EXCL_LINE
        }
        वापस targetVarp->valuep()->clone_वृक्ष(false);
    }
    व्योम resolveGenericIfaceDotsIn(Ast_गाँठ* nodep) {
        // Collect first: the deleteTree() below frees a node foreach() would still read from
        std::vector<AstDot*> dotps;
        nodep->foreach([&](AstDot* dotp) { dotps.धकेलो_back(dotp); });
        क्रम (AstDot* स्थिर dotp : dotps) {
            अगर (Ast_गाँठ* स्थिर newp = tryResolveGenericIfaceDot(dotp)) {
                dotp->replaceWith(newp);
                VL_DO_DANGLING(dotp->delete_वृक्ष(), dotp);
            }
        }
    }

    // Make sure all parameters are constantified
    व्योम visit(AstVar* nodep) override {
        अगर (nodep->user2SetOnce()) वापस;  // Process once
        // Interface array port: split it into its elements
        अगर (nodep->isIfaceRef() && !nodep->isIfaceParent()
            && (VN_IS(nodep->subDTypep()->skipRefp(), UnpackArrayDType)
                || VN_IS(nodep->subDTypep()->skipRefp(), BracketArrayDType))) {
            // Also converts any C-style [N] dimensions, the ranges must be constant
            V3Width::widthParamsEdit(nodep->subDTypep());
            expandIfaceArrayPortDimensions(nodep, nodep->subDTypep(), "");
            nodep->isIfaceArraySplit(true);
        }
        // Build cache of interface port names as we encounter them
        अगर (nodep->isIfaceRef()) { m_ifacePortNames.जोड़ो(nodep->name()); }
        iterateChildren(nodep);
        अगर (nodep->isParam()) {
            checkParamNotHierRecurse(nodep->valuep());
            अगर (!nodep->valuep() && !VN_IS(m_modp, Class)) {
                nodep->v3error("Parameter without default value is never given value"
                               << " (IEEE 1800-2023 6.20.1): " << nodep->prettyNameQ());
            } अन्यथा अगर (nodep->valuep()) {
                // Resolve now: a sibling cell's pin fold may need this before linkDotParamed
                resolveGenericIfaceDotsIn(nodep->valuep());
                // If the value expression contains a VarXRef to an interface
                // localparam whose value is not yet constant, defer constification
                // to avoid premature widthing with unresolved values (see
                // t_lparam_dep_iface tests).  When the referenced localparam is
                // already const, proceed normally so FUNCREFs with resolved iface
                // param args get folded.
                bool hasUnresolvedLparamXRef = false;
                nodep->valuep()->foreach([&](स्थिर AstVarXRef* xrefp) {
                    अगर (स्थिर AstVar* स्थिर varp = xrefp->varp()) {
                        अगर (varp->varType() == VVarType::LPARAM && !VN_IS(varp->valuep(), Const)) {
                            hasUnresolvedLparamXRef = true;
                        }
                    }
                });
                अगर (hasUnresolvedLparamXRef) वापस;
                // Defer if value has any unresolved Dot, or references a deferred lparam
                स्थिर bool hasDot = nodep->valuep()->exists([](AstDot*) { वापस true; });
                bool refsDeferred = false;
                अगर (!hasDot) {
                    स्थिर स्वतः& deferredVarps = v3Global.rootp()->deferredParamVarps();
                    nodep->valuep()->foreach([&](स्थिर AstVarRef* refp) {
                        अगर (refsDeferred) वापस;
                        AstVar* स्थिर refVarp = refp->varp();
                        अगर (refVarp && refVarp->varType() == VVarType::LPARAM
                            && deferredVarps.गिनती(refVarp)) {
                            refsDeferred = true;
                        }
                    });
                }
                अगर (hasDot || refsDeferred) {
                    v3Global.rootp()->धकेलो_Deferred_Param_Varp(nodep);
                    वापस;
                }
                m_processor.resolveDeferredDotsReachableFrom(nodep, m_modp);
                V3Const::constifyParamsEdit(nodep);
            }
        }
    }
    व्योम visit(AstParamTypeDType* nodep) override {
        iterateChildren(nodep);
        अगर (VN_IS(nodep->skipRefOrNullp(), VoidDType)) {
            nodep->v3error("Parameter type without default value is never given value"
                           << " (IEEE 1800-2023 6.20.1): " << nodep->prettyNameQ());
        }
    }
    // Make sure varrefs cause vars to constify before things above
    व्योम visit(AstVarRef* nodep) override {
        // Might jump across functions, so beware if ever add a m_funcp
        अगर (nodep->varp()) iterate(nodep->varp());
    }
    bool ifaceParamReplace(AstVarXRef* nodep, Ast_गाँठ* candp) {
        क्रम (; candp; candp = candp->nextp()) {
            अगर (nodep->name() == candp->name()) {
                अगर (AstVar* स्थिर varp = VN_CAST(candp, Var)) {
                    UINFO(9, "Found interface parameter: " << varp);
                    // The interface may not have been visited yet (it is at a
                    // deeper level in the work queue), so its localparams may
                    // not be constified.  Eagerly constify here so that the
                    // caller's hasUnresolvedLparamXRef check sees a Const.
                    अगर (varp->isParam() && varp->valuep() && !VN_IS(varp->valuep(), Const)) {
                        V3Const::constifyParamsEdit(varp);
                    }
                    nodep->varp(varp);
                    वापस true;
                } अन्यथा अगर (स्थिर AstPin* स्थिर pinp = VN_CAST(candp, Pin)) {
                    UINFO(9, "Found interface parameter: " << pinp);
                    UASSERT_OBJ(pinp->exprp(), pinp, "Interface parameter pin missing expression");
                    nodep->replaceWith(pinp->exprp()->clone_वृक्ष(false));
                    VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
                    वापस true;
                }
            }
        }
        वापस false;
    }

    व्योम deparamIfaceCellNow(AstCell* cellp) {
        अगर (!cellp->paramsp()) वापस;
        अगर (!VN_IS(cellp->modp(), Iface)) वापस;
        Ast_गाँठ_Module* स्थिर srcModp = cellp->modp();
        Ast_गाँठ_Module* स्थिर newModp
            = m_processor.गाँठ_Deparam(cellp, srcModp, m_modp, m_modp->someInstanceName());
        अगर (newModp && newModp != srcModp) {
            अगर (V3LinkDotIfaceCapture::enabled()) {
                m_processor.retargetIfaceRefs(m_modp, cellp->name());
            }
            specializeNestedIfaceCells(newModp);
        }
    }

    व्योम visit(Ast_गाँठ_FTask_Ref* nodep) override {
        अगर (nodep->containsGenBlock()) {
            // Needs relink, as may remove pointed-to task/func
            nodep->taskp(nullptr);
            वापस;
        }
        iterateChildren(nodep);
    }
    व्योम visit(AstEnumItemRef* nodep) override {
        // Needs relink, as may remove pointed-to item
        अगर (nodep->containsGenBlock()) nodep->itemp(nullptr);
        iterateChildren(nodep);
    }
    व्योम visit(AstVarXRef* nodep) override {
        अगर (nodep->containsGenBlock()) {
            // Needs relink, as may remove pointed-to var
            nodep->varp(nullptr);
            वापस;
        }
        // Parameters of an interface array are accessed through its first element
        अगर (nodep->varp() && nodep->varp()->isParam()) {
            स्थिर स्वतः it = m_ifaceInstCells.find(nodep->dotted());
            अगर (it != m_ifaceInstCells.end()) nodep->dotted(it->second->name());
        }
        // Check to see if the scope is just an interface because interfaces are special
        स्थिर string dotted = nodep->dotted();
        अगर (!dotted.empty() && nodep->varp() && nodep->varp()->isParam()) {
            स्थिर Ast_गाँठ* backp = nodep;
            जबतक ((backp = backp->backp())) {
                अगर (VN_IS(backp, गाँठ_Module)) {
                    UINFO(9, "Hit module boundary, done looking for interface");
                    अवरोध;
                }
                अगर (स्थिर AstVar* स्थिर varp = VN_CAST(backp, Var)) {
                    अगर (!varp->isIfaceRef()) जारी;
                    // Through all dimensions of an array
                    स्थिर AstIfaceRefDType* स्थिर ifacerefp
                        = VN_CAST(varp->subDTypep()->elemDTypep(), IfaceRefDType);
                    अगर (!ifacerefp) जारी;
                    // Interfaces passed in on the port map have ifaces
                    अगर (स्थिर AstIface* स्थिर ifacep = ifacerefp->ifacep()) {
                        अगर (dotted == backp->name()) {
                            UINFO(9, "Iface matching scope:  " << ifacep);
                            अगर (ifaceParamReplace(nodep, ifacep->stmtsp())) {  //
                                वापस;
                            }
                        }
                    }
                    // Interfaces declared in this module have cells
                    अन्यथा अगर (AstCell* स्थिर cellp = ifacerefp->cellp()) {
                        अगर (dotted == cellp->name()) {
                            UINFO(9, "Iface matching scope:  " << cellp);
                            deparamIfaceCellNow(cellp);
                            अगर (ifaceParamReplace(nodep, cellp->modp()->stmtsp())) {  //
                                वापस;
                            }
                        }
                    }
                }
            }
            // Fallback: a direct local interface instance ("inst.PARAM"), not reached via a port.
            स्थिर स्वतः ifaceCellIt = m_ifaceInstCells.find(dotted);
            अगर (ifaceCellIt != m_ifaceInstCells.end()) {
                AstCell* स्थिर cellp = ifaceCellIt->second;
                deparamIfaceCellNow(cellp);
                ifaceParamReplace(nodep, cellp->modp()->stmtsp());
            }
        }
    }

    व्योम visit(AstDot* nodep) override {
        iterate(nodep->lhsp());
        // Check if it is a reference to a field of a parameterized class.
        // If so, the RHS should be updated, when the LHS is replaced
        // by a class with actual parameter values.
        स्थिर AstClass* lhsClassp = nullptr;
        स्थिर AstClassOrPackageRef* स्थिर classRefp = VN_CAST(nodep->lhsp(), ClassOrPackageRef);
        अगर (classRefp) lhsClassp = VN_CAST(classRefp->classOrPackageSkipp(), Class);
        Ast_गाँठ* rhsDefp = nullptr;
        AstClassOrPackageRef* स्थिर rhsp = VN_CAST(nodep->rhsp(), ClassOrPackageRef);
        अगर (rhsp) rhsDefp = rhsp->classOrPackageNodep();
        अगर (lhsClassp && rhsDefp) {
            m_state.m_dots.धकेलो_back(nodep);
            // No need to iterate into rhsp, because there should be nothing to do
        } अन्यथा {
            iterate(nodep->rhsp());
        }
    }

    व्योम visit(AstUnlinkedRef* nodep) override {
        AstVarXRef* स्थिर varxrefp = VN_CAST(nodep->refp(), VarXRef);
        Ast_गाँठ_FTask_Ref* स्थिर taskrefp = VN_CAST(nodep->refp(), गाँठ_FTask_Ref);
        AstEnumItemRef* स्थिर enumrefp = VN_CAST(nodep->refp(), EnumItemRef);
        अगर (varxrefp) {
            m_unlinkedTxt = varxrefp->dotted();
        } अन्यथा अगर (taskrefp) {
            m_unlinkedTxt = taskrefp->dotted();
        } अन्यथा अगर (enumrefp) {
            m_unlinkedTxt = enumrefp->dotted();
        } अन्यथा {
            nodep->v3fatalSrc("Unexpected AstUnlinkedRef node");
            वापस;
        }
        iterate(nodep->cellrefp());

        अगर (varxrefp) {
            varxrefp->dotted(m_unlinkedTxt);
        } अन्यथा अगर (taskrefp) {
            taskrefp->dotted(m_unlinkedTxt);
        } अन्यथा {
            enumrefp->dotted(m_unlinkedTxt);
        }
        nodep->replaceWith(nodep->refp()->unlinkFrBack());
        VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
    }
    व्योम visit(AstCellArrayRef* nodep) override {
        // Multi-dim iface arrays chain multiple selps via nextp(); constify each in place.
        क्रम (Ast_गाँठ *s = nodep->selp(), *nxt; s; s = nxt) {
            nxt = s->nextp();  // cache before constifyParamsEdit replaces s
            V3Const::constifyParamsEdit(s);
        }
        std::vector<पूर्णांक> indices;
        क्रम (Ast_गाँठ* s = nodep->selp(); s; s = s->nextp()) {
            स्थिर AstConst* स्थिर constp = VN_CAST(s, Const);
            अगर (!constp) {
                nodep->v3error("Could not expand constant selection inside dotted reference: "
                               << s->prettyNameQ());
                वापस;
            }
            indices.धकेलो_back(constp->toSInt());
        }
        // Nested iface-array ports may carry a __Viftop suffix that's not in m_unlinkedTxt.
        स्थिर string viftopSuffix = "__Viftop";
        स्थिर string baseName
            = VString::endsWith(nodep->name(), viftopSuffix)
                  ? nodep->name().substr(0, nodep->name().आकार() - viftopSuffix.आकार())
                  : nodep->name();
        स्थिर string placeholder = "__BRA__??__KET__";
        माप_प्रकार pos = m_unlinkedTxt.find(baseName + placeholder);
        // An AstCellArrayRef for an iface-array pin expr (e.g. l1(l2.l1[0])) is visited
        // outside AstUnlinkedRef, so m_unlinkedTxt has no placeholder; V3LinkDot resolves later.
        अगर (pos == string::npos) {
            UINFO(9, "Skipping unlinked text replacement for " << nodep);
            वापस;
        }
        pos += baseName.लंबाई();
        क्रम (पूर्णांक idx : indices) {
            स्थिर string replacement = "__BRA__" + Ast_गाँठ::encode_संख्या(idx) + "__KET__";
            अगर (m_unlinkedTxt.compare(pos, placeholder.लंबाई(), placeholder) != 0) {
                nodep->v3fatalSrc(  // LCOV_EXCL_LINE
                    "Expected placeholder at position in multi-dim iface dotted reference");
                वापस;
            }
            m_unlinkedTxt.replace(pos, placeholder.लंबाई(), replacement);
            pos += replacement.लंबाई();
        }
    }

    // Generate Statements

    // Traverse a generate condition (if/case expression, for init/cond/inc).
    //
    // m_cellps assumes a queued ref outlives the drain loop, which holds elsewhere
    // because deletion is deferred.  A generate condition doesn't defer and folds
    // immediately, so we can't queue. Anything queued here would be left dangling.
    // Refs in the surviving arm are queued when it is recursed.
    व्योम iterateGenerateCond(Ast_गाँठ* nodep) {
        VL_RESTORER(m_inGenerateCond);
        m_inGenerateCond = true;
        iterate_And_अग्र_Null(nodep);
        // The condition may reference deferred lparams whose Dots are still pending.
        m_processor.resolveDeferredDotsReachableFrom(nodep, m_modp);
    }

    व्योम visit(AstGenIf* nodep) override {
        UINFO(9, "  GENIF " << nodep);
        iterateGenerateCond(nodep->condp());
        // We suppress errors when widthing params since short-circuiting in
        // the conditional evaluation may mean these error can never occur. We
        // then make sure that short-circuiting is used by constifyParamsEdit.
        V3Width::widthGenerateParamsEdit(nodep);  // Param typed widthing will
                                                  // NOT recurse the body.
        V3Const::constifyGenerateParamsEdit(nodep->condp());  // condp may change
        अगर (स्थिर AstConst* स्थिर constp = VN_CAST(nodep->condp(), Const)) {
            अगर (Ast_गाँठ* स्थिर keepp = (constp->isZero() ? nodep->elsesp() : nodep->thensp())) {
                keepp->unlink_Fr_Back_With_अग्र();
                nodep->replaceWith(keepp);
            } अन्यथा {
                nodep->unlinkFrBack();
            }
            VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
            // Normal edit rules will now recurse the replacement
        } अन्यथा {
            nodep->condp()->v3error("Generate If condition must evaluate to constant");
        }
    }

    व्योम visit(AstGenBlock* nodep) override {
        // Parameter substitution for generated for loops.
        // TODO Unlike generated IF, we don't have to worry about short-circuiting the
        // conditional expression, since this is currently restricted to simple
        // comparisons. If we ever do move to more generic constant expressions, such code
        // will be needed here.
        अगर (AstGenFor* स्थिर forp = VN_AS(nodep->genforp(), GenFor)) {
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
            स्थिर string beginName = nodep->name();
            // Leave the original Begin, as need a container for the (possible) GENVAR
            // Note m_unroller will replace some AstVarRef's to the loop variable with constants
            // Don't remove any deleted nodes in m_unroller until whole process finishes,
            // as some AstXRefs may still point to old nodes.
            VL_DO_DANGLING(m_unroller.unroll(forp, beginName), forp);
            // Blocks were constructed under the special begin, move them up
            // Note forp is null, so grab statements again
            अगर (Ast_गाँठ* स्थिर stmtsp = nodep->genforp()) {
                stmtsp->unlink_Fr_Back_With_अग्र();
                nodep->add_अग्र_Here(stmtsp);
                // Note this clears nodep->genforp(), so begin is no longer special
            }
        } अन्यथा {
            VL_RESTORER_COPY(m_generateHierName);
            m_generateHierName += "." + nodep->prettyName();
            iterateChildren(nodep);
        }
    }
    व्योम visit(AstGenFor* nodep) override {  // LCOV_EXCL_LINE
        nodep->v3fatalSrc("GENFOR should have been wrapped in BEGIN");
    }
    व्योम visit(AstGenCase* nodep) override {
        UINFO(9, "  GENCASE " << nodep);
        bool hit = false;
        Ast_गाँठ* keepp = nullptr;
        iterateGenerateCond(nodep->exprp());
        V3Case::caseLint(nodep);
        // Case-item expressions must also be linked before widthing the case.
        क्रम (AstGenCaseItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), GenCaseItem)) {
            क्रम (Ast_गाँठ* ep = itemp->condsp(); ep; ep = ep->nextp()) {
                m_processor.resolveDeferredDotsReachableFrom(ep, m_modp);
            }
        }
        V3Width::widthParamsEdit(nodep);  // Param typed widthing will NOT recurse the
                                          // body, don't trigger errors yet.
        V3Const::constifyParamsEdit(nodep->exprp());  // exprp may change
        स्थिर AstConst* स्थिर exprp = VN_AS(nodep->exprp(), Const);
        // Constify
        क्रम (AstGenCaseItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), GenCaseItem)) {
            क्रम (Ast_गाँठ* ep = itemp->condsp(); ep;) {
                Ast_गाँठ* स्थिर nextp = ep->nextp();  // May edit list
                iterateGenerateCond(ep);
                VL_DO_DANGLING(V3Const::constifyParamsEdit(ep), ep);  // ep may change
                ep = nextp;
            }
        }
        // Item match
        क्रम (AstGenCaseItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), GenCaseItem)) {
            अगर (!itemp->isDefault()) {
                क्रम (Ast_गाँठ* ep = itemp->condsp(); ep; ep = ep->nextp()) {
                    अगर (स्थिर AstConst* स्थिर ccondp = VN_CAST(ep, Const)) {
                        V3_संख्या match{nodep, 1};
                        match.opEq(ccondp->अंक(), exprp->अंक());
                        अगर (!hit && match.isNeqZero()) {
                            hit = true;
                            keepp = itemp->itemsp();
                        }
                    } अन्यथा {
                        itemp->v3error("Generate Case item does not evaluate to constant");
                    }
                }
            }
        }
        // Else default match
        क्रम (AstGenCaseItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), GenCaseItem)) {
            अगर (itemp->isDefault()) {
                अगर (!hit) {
                    hit = true;
                    keepp = itemp->itemsp();
                }
            }
        }
        // Replace
        अगर (keepp) {
            keepp->unlink_Fr_Back_With_अग्र();
            nodep->replaceWith(keepp);
        } अन्यथा {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
    }

    व्योम visit(Ast_गाँठ* nodep) override { iterateChildren(nodep); }

खुला:
    // CONSTRUCTORS
    explicit ParamVisitor(ParamState& state, AstNetlist* netlistp)
        : m_state{state}
        , m_processor{netlistp} {

        // Relies on modules already being in top-down-order
        iterate(netlistp);
    }
    ~ParamVisitor() override = शेष;
    VL_UNCOPYABLE(ParamVisitor);
};

//######################################################################
// Process top handler

श्रेणी Param_शीर्ष final : VNDeleter {
    // NODE STATE
    // AstNodeModule::user1 -> bool: already fixed level (temporary)

    // STATE
    ParamState m_state;  // Common state

    // METHODS

    व्योम relinkDots() {
        // RHSs of AstDots need a relink when LHS is a parameterized class reference
        क्रम (AstDot* स्थिर dotp : m_state.m_dots) {
            स्थिर AstClassOrPackageRef* स्थिर classRefp = VN_AS(dotp->lhsp(), ClassOrPackageRef);
            स्थिर AstClass* स्थिर lhsClassp = VN_AS(classRefp->classOrPackageSkipp(), Class);
            AstClassOrPackageRef* स्थिर rhsp = VN_AS(dotp->rhsp(), ClassOrPackageRef);
            क्रम (स्वतः* itemp = lhsClassp->membersp(); itemp; itemp = itemp->nextp()) {
                अगर (itemp->name() == rhsp->name()) {
                    rhsp->classOrPackageNodep(itemp);
                    अवरोध;
                }
            }
        }
    }

    व्योम fixLevel(Ast_गाँठ_Module* modp) {
        // Fix up level of module, based on who instantiates it
        अगर (modp->user1SetOnce()) वापस;  // Already fixed
        अगर (m_state.m_parentps[modp].empty()) वापस;  // Leave top levels alone
        पूर्णांक maxParentLevel = 0;
        क्रम (Ast_गाँठ_Module* parentp : m_state.m_parentps[modp]) {
            fixLevel(parentp);  // Ensure parent level is correct
            maxParentLevel = std::max(maxParentLevel, parentp->level());
        }
        अगर (modp->level() <= maxParentLevel) modp->level(maxParentLevel + 1);
        // Not correct fixup of depth(), as it should be mininum.  But depth() is unused
        // past V3LinkParse, so just do something sane in case this code doesn't get updated
        modp->depth(modp->level());
    }

    व्योम resortNetlistModules(AstNetlist* netlistp) {
        // Re-sort module list to be in topological order and fix-up incorrect levels. We need to
        // do this globally at the end due to the presence of recursive modules, which might be
        // expanded in orders that reuse earlier specializations later at a lower level.
        // Gather modules
        std::vector<Ast_गाँठ_Module*> modps;
        क्रम (Ast_गाँठ_Module *modp = netlistp->modulesp(), *nextp; modp; modp = nextp) {
            nextp = VN_AS(modp->nextp(), गाँठ_Module);
            modp->unlinkFrBack();
            modps.धकेलो_back(modp);
        }

        // Fix-up levels
        स्थिर VNUser1InUse user1InUse;
        क्रम (Ast_गाँठ_Module* स्थिर modp : modps) fixLevel(modp);

        // Sort by level
        std::stable_sort(modps.begin(), modps.end(),
                         [](स्थिर Ast_गाँठ_Module* ap, स्थिर Ast_गाँठ_Module* bp) {
                             वापस ap->level() < bp->level();
                         });

        // Re-insert modules
        क्रम (Ast_गाँठ_Module* स्थिर modp : modps) netlistp->addModulesp(modp);
    }

खुला:
    // CONSTRUCTORS
    explicit Param_शीर्ष(AstNetlist* netlistp) {
        // Relies on modules already being in top-down-order
        स्थिर ParamVisitor paramVisitor{m_state /*ref*/, netlistp};

        // Mark classes which cannot be removed because they are still referenced
        स्थिर ClassRefUnlinkerVisitor classUnlinkerVisitor{netlistp};

        netlistp->foreach([](Ast_गाँठ_FTask_Ref* ftaskrefp) {
            Ast_गाँठ_FTask* ftaskp = ftaskrefp->taskp();
            अगर (!ftaskp || !ftaskp->classMethod()) वापस;
            स्थिर std::string funcName = ftaskp->name();
            // Find the nearest containing (ancestor) class for a node.
            // Uses aboveLoopp() which correctly skips sibling links
            // (e.g. covergroup classes) to find the true parent.
            स्थिर स्वतः ancestorClassOf = [](Ast_गाँठ* startp) -> AstClass* {
                क्रम (Ast_गाँठ* np = startp->aboveLoopp(); np; np = np->aboveLoopp()) {
                    अगर (AstClass* स्थिर cp = VN_CAST(np, Class)) वापस cp;
                }
                वापस nullptr;
            };
            AstClass* refClassp = ancestorClassOf(ftaskrefp);
            अगर (refClassp == ftaskrefp->classOrPackagep())
                वापस;  // task is in the same class as reference
            AstClass* classp = ancestorClassOf(ftaskp);
            UASSERT_OBJ(classp, ftaskrefp, "Class method has no class above it");
            // If the FUNCREF and its task are both in the same (clone) class but
            // classOrPackagep still points to the old template, just retarget it
            अगर (refClassp && refClassp == classp && ftaskrefp->classOrPackagep()
                && ftaskrefp->classOrPackagep() != refClassp) {
                ftaskrefp->classOrPackagep(refClassp);
                वापस;
            }
            अगर (classp->user3p()) वापस;  // will not get removed, no need to relink
            AstClass* स्थिर parametrizedClassp = VN_CAST(classp->user4p(), Class);
            अगर (!parametrizedClassp) वापस;
            Ast_गाँठ_FTask* newFuncp = nullptr;
            parametrizedClassp->exists([&newFuncp, funcName](Ast_गाँठ_FTask* ftaskp) {
                अगर (ftaskp->name() == funcName) {
                    newFuncp = ftaskp;
                    वापस true;
                }
                वापस false;
            });
            अगर (newFuncp) {
                // v3error(ftaskp <<"->" << newFuncp);
                ftaskrefp->taskp(newFuncp);
                ftaskrefp->classOrPackagep(parametrizedClassp);
            }
        });

        स्थिर ParamClassRefDTypeRelinkVisitor paramClassDTypeRelinkVisitor{netlistp};

        relinkDots();

        resortNetlistModules(netlistp);

        // For occasional debug only; this will upset VL_LEAK_CHECKS
        // V3Global::dumpCheckGlobalTree("param-predel", 0, dumpTreeEitherLevel() >= 9);

        // Remove defaulted classes
        // Unlike modules, which we keep around and mark dead() for later V3Dead
        std::unordered_set<AstClass*> removedClassps;
        क्रम (AstClass* स्थिर classp : m_state.m_paramClasses) {
            अगर (!classp->user3p()) {
                // If there was an error, don't remove classes as they might
                // have remained referenced, and  will crash in V3Broken or
                // other locations. This is fine, we will abort imminently.
                अगर (V3Error::error_गिनती()) जारी;
                removedClassps.emplace(classp);
                VL_DO_DANGLING(धकेलो_Deletep(classp->unlinkFrBack()), classp);
            } अन्यथा {
                // Referenced. classp became a specialized class with the default
                // values of parameters and is not a parameterized class anymore
                classp->hasGParam(false);
            }
        }
        // Remove references to defaulted classes
        // Reuse user3 to mark all nodes being deleted
        Ast_गाँठ::user3_Clear_वृक्ष();
        क्रम (AstClass* classp : removedClassps) {
            classp->foreach([](Ast_गाँठ* स्थिर nodep) { nodep->user3(true); });
        }
        // Set all links pointing to a user3 (deleting) node as null
        netlistp->foreach([](Ast_गाँठ* स्थिर nodep) {
            nodep->foreachLink([&](Ast_गाँठ** स्थिर linkpp, स्थिर अक्षर* namep) {
                अगर (*linkpp && (*linkpp)->user3()) {
                    UINFO(9, "clear   link " << namep << " on " << nodep);
                    *linkpp = nullptr;
                    UINFO(9, "cleared link " << namep << " on " << nodep);
                }
            });
        });
    }
    ~Param_शीर्ष() = शेष;
    VL_UNCOPYABLE(Param_शीर्ष);
};

//######################################################################
// Param class functions

व्योम V3Param::param(AstNetlist* rootp) {
    UINFO(2, __FUNCTION__ << ":");
    rootp->clearDeferredParamVarps();  // Defensive: drop any stragglers from a prior invocation

    अगर (dump_वृक्ष_Either_Level() >= 9) V3LinkDotIfaceCapture::dumpEntries("before V3Param");
    { Param_शीर्ष{rootp}; }
    // The memo is only good while parameterizing, and the tree moves after.
    rootp->clearContainingModules();
    अगर (dump_वृक्ष_Either_Level() >= 9) V3LinkDotIfaceCapture::dumpEntries("after V3Param");

    V3Global::dump_Check_Global_वृक्ष("param", 0, dump_वृक्ष_Either_Level() >= 3);
}

व्योम V3Param::finalizeDeferredParams(AstNetlist* rootp) {
    // Constify params whose Dot RHS was resolved by V3LinkDot::linkDotParamed.
    क्रम (AstVar* स्थिर varp : rootp->deferredParamVarps()) {
        अगर (varp->valuep() && !VN_IS(varp->valuep(), Const)) V3Const::constifyParamsEdit(varp);
    }
    rootp->clearDeferredParamVarps();
}
