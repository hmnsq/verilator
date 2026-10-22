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
स्थिर AstClassRefDType* class_Ref_DType_Of_गाँठो(Ast_गाँठो* nodep) {
    जबसम्म (AstTypedef* अचर tdp = VN_CAST(nodep, Typedef)) nodep = tdp->subDTypep();
    Ast_गाँठो_DType* अचर dtp = VN_CAST(nodep, गाँठो_DType);
    फर्काऊ dtp ? VN_CAST(dtp->skipRefOrNullp(), ClassRefDType) : nullptr;
}

//######################################################################
// Hierarchical block and parameter db (modules without parameters are also handled)

वर्ग ParameterizedHierBlocks final {
    using HierBlockOptsByOrigName = std::multimap<std::string, अचर V3HierarchicalBlockOption*>;
    using HierMapIt = HierBlockOptsByOrigName::const_iterator;
    using HierBlockModMap = std::map<अचर std::string, Ast_गाँठो_Module*>;
    using ParamConstMap = std::map<अचर std::string, std::unique_ptr<AstConst>>;
    using GParamsMap = std::map<अचर std::string, AstVar*>;  // key:parameter name value:parameter

    // MEMBERS
    अचर तार्किक m_hierSubRun;  // Is in sub-run for hierarchical verilation
    // key:Original module name, value:HiearchyBlockOption*
    // If a module is parameterized, the module is uniquified to overridden parameters.
    // This is why HierBlockOptsByOrigName is multimap.
    HierBlockOptsByOrigName m_hierBlockOptsByOrigName;
    // key:mangled module name, value:AstNodeModule*
    HierBlockModMap m_hierBlockMod;
    // Overridden parameters of the hierarchical block
    std::map<अचर V3HierarchicalBlockOption*, ParamConstMap> m_hierParams;
    // Parameter variables of hierarchical blocks
    std::map<अचर std::string, GParamsMap> m_modParams;
    // Formals declared without a type, whose type is that of the assigned value
    std::set<अचर AstVar*> m_untypedParams;

    // METHODS

सार्वजनिक:
    ParameterizedHierBlocks(अचर V3HierBlockOptSet& hierOpts, AstNetlist* nodep)
        // A type parameter file only describes a child's own types; the final consolidation
        // run has none, but must still substitute the libraries it consumes.
        : m_hierSubRun{!v3Global.opt.hierBlocks().empty() || v3Global.opt.hierChild()} {
        लागि (अचर स्वतः& hierOpt : hierOpts) {
            m_hierBlockOptsByOrigName.emplace(hierOpt.second.origName(), &hierOpt.second);
            अचर V3HierarchicalBlockOption::ParamStrMap& params = hierOpt.second.params();
            ParamConstMap& consts = m_hierParams[&hierOpt.second];
            लागि (V3HierarchicalBlockOption::ParamStrMap::const_iterator pIt = params.begin();
                 pIt != params.end(); ++pIt) {
                std::unique_ptr<AstConst> constp{AstConst::parseParamLiteral(
                    नूतन File_पङ्क्ति{File_पङ्क्ति::builtInFilename()}, pIt->second)};
                UASSERT(constp, pIt->second << " is not a valid parameter literal");
                अचर तार्किक inserted = consts.emplace(pIt->first, std::move(constp)).second;
                UASSERT(inserted, pIt->first << " is already added");
            }
            // origName may be already registered, but it's fine.
            m_modParams.घुसाउ({hierOpt.second.origName(), {}});
        }
        लागि (Ast_गाँठो_Module* modp = nodep->modulesp(); modp;
             modp = VN_AS(modp->nextp(), गाँठो_Module)) {
            यदि (hierOpts.find(modp->prettyName()) != hierOpts.end()) {
                m_hierBlockMod.emplace(modp->name(), modp);
            }
            // Recursive hierarchical module may change its name, so we have to match its origName.
            // Collect values from recursive cloned module as parameters in the top module could be
            // overridden.
            अचर string actualModName = modp->recursiveClone() ? modp->origName() : modp->name();
            अचर स्वतः defParamIt = m_modParams.find(actualModName);
            यदि (defParamIt != m_modParams.end()) {
                // modp is the original of parameterized hierarchical block
                लागि (Ast_गाँठो* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    यदि (AstVar* अचर varp = VN_CAST(stmtp, Var)) {
                        यदि (varp->isGParam()) defParamIt->second.emplace(varp->name(), varp);
                        // Recorded before V3Param resolves the type from the assigned value.
                        अचर AstBasicDType* अचर dtypep
                            = VN_CAST(varp->childDTypep(), BasicDType);
                        यदि (varp->isGParam() && dtypep && dtypep->implicit()) {
                            m_untypedParams.emplace(varp);
                        }
                    }
                }
            }
        }
    }
    तार्किक hierSubRun() अचर { फर्काऊ m_hierSubRun; }
    तार्किक isHierBlock(अचर string& origName) अचर {
        फर्काऊ m_hierBlockOptsByOrigName.find(origName) != m_hierBlockOptsByOrigName.end();
    }
    Ast_गाँठो_Module* findByParams(अचर string& origName, AstPin* firstPinp,
                                अचर Ast_गाँठो_Module* modp) {
        UASSERT(isHierBlock(origName), origName << " is not hierarchical block");
        // This module is a hierarchical block. Need to replace it by the --lib-create wrapper.
        अचर std::pair<HierMapIt, HierMapIt> candidates
            = m_hierBlockOptsByOrigName.equal_range(origName);
        अचर स्वतः paramsIt = m_modParams.find(origName);
        UASSERT_OBJ(paramsIt != m_modParams.end(), modp, origName << " must be registered");
        // An untyped parameter takes the type of its value, but libraries are distinguished
        // only by parameter values. This is a pin whose value equals that of a library or the
        // default, but whose type differs.
        अचर AstPin* typeMismatchPinp = nullptr;
        HierMapIt hierIt;
        लागि (hierIt = candidates.first; hierIt != candidates.second; ++hierIt) {
            तार्किक found = सत्य;
            माप_प्रकार paramIdx = 0;
            अचर ParamConstMap& params = m_hierParams[hierIt->second];
            UASSERT(params.size() == hierIt->second->params().size(), "not match");
            लागि (AstPin* pinp = firstPinp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                यदि (!pinp->exprp()) जारी;
                यदि (अचर AstVar* अचर modvarp = pinp->modVarp()) {
                    AstConst* अचर constp = VN_AS(pinp->exprp(), Const);
                    UASSERT_OBJ(constp, pinp,
                                "parameter for a hierarchical block must have been constified");
                    अचर स्वतः paramIt = paramsIt->second.find(modvarp->name());
                    UASSERT_OBJ(paramIt != paramsIt->second.end(), modvarp, "must be registered");
                    अचर तार्किक untyped = m_untypedParams.गन्ती(paramIt->second);
                    AstConst* अचर defValuep = VN_CAST(paramIt->second->valuep(), Const);
                    यदि (defValuep && areSame(constp, defValuep)) {
                        यदि (!untyped || constp->same_मान_Type(defValuep)) {
                            UINFO(5, "Setting default value of " << constp << " to " << modvarp);
                            जारी;  // Skip this parameter because setting the same value
                        }
                        typeMismatchPinp = pinp;
                    }
                    अचर स्वतः pIt = vlstd::as_const(params).find(modvarp->name());
                    UINFO(5, "Comparing " << modvarp->name() << " " << constp);
                    यदि (pIt == params.end() || paramIdx >= params.size()
                        || !areSame(constp, pIt->second.get())) {
                        found = असत्य;
                        रोक;
                    }
                    यदि (untyped && !constp->same_मान_Type(pIt->second.get())) {
                        typeMismatchPinp = pinp;
                        found = असत्य;
                        रोक;
                    }
                    UINFO(5, "Matched " << modvarp->name() << " " << constp << " and "
                                        << pIt->second.get());
                    ++paramIdx;
                }
            }
            यदि (found && paramIdx == hierIt->second->params().size()) रोक;
        }
        यदि (hierIt == candidates.second && typeMismatchPinp) {
            typeMismatchPinp->v3warn(
                E_UNSUPPORTED, "Unsupported: Untyped parameter "
                                   << typeMismatchPinp->modVarp()->prettyNameQ()
                                   << " of hierarchical block " << Ast_गाँठो::prettyNameQ(origName)
                                   << " given equal values of different width or"
                                      " signedness");
            फर्काऊ nullptr;
        }
        UASSERT_OBJ(hierIt != candidates.second, firstPinp, "No --lib-create wrapper found");
        // parameter settings will be removed in the bottom of caller visitCell().
        अचर HierBlockModMap::const_iterator modIt
            = m_hierBlockMod.find(hierIt->second->mangledName());
        UASSERT_OBJ(modIt != m_hierBlockMod.end(), firstPinp,
                    hierIt->second->mangledName() << " is not found");

        अचर स्वतः it = vlstd::as_const(m_hierBlockMod).find(hierIt->second->mangledName());
        यदि (it == m_hierBlockMod.end()) फर्काऊ nullptr;
        फर्काऊ it->second;
    }
    स्थिर तार्किक areSame(AstConst* pinValuep, AstConst* hierOptParamp) {
        यदि (pinValuep->isString()) {
            फर्काऊ pinValuep->अङ्क().toString() == hierOptParamp->अङ्क().toString();
        }

        यदि (hierOptParamp->isDouble()) {
            द्विगुण var;
            यदि (pinValuep->isDouble()) {
                var = pinValuep->अङ्क().toDouble();
            } अन्यथा {  // Cast from integer to real
                V3_सङ्ख्या var_अङ्क{pinValuep, V3_सङ्ख्या::Double{}, 0.0};
                var_अङ्क.opIToRD(pinValuep->अङ्क());
                var = var_अङ्क.toDouble();
            }
            फर्काऊ V3_सङ्ख्या::epsilonEqual(var, hierOptParamp->अङ्क().toDouble());
        } अन्यथा {  // Now integer type is assumed
            // Bitwidth of hierOptParamp is accurate because V3Width already calculated in the
            // previous run. Bitwidth of pinValuep is before width analysis, so pinValuep is casted
            // to hierOptParamp width.
            V3_सङ्ख्या var_अङ्क{pinValuep, hierOptParamp->अङ्क().width()};
            यदि (pinValuep->isDouble()) {  // Need to cast to int
                // Parameter is actually an integral type, but passed value is floating point.
                // Conversion from real to integer uses rounding in V3Width.cpp
                var_अङ्क.opRToIRoundS(pinValuep->अङ्क());
            } अन्यथा यदि (pinValuep->isSigned()) {
                var_अङ्क.opExtendS(pinValuep->अङ्क(), pinValuep->अङ्क().width());
            } अन्यथा {
                var_अङ्क.opAssign(pinValuep->अङ्क());
            }
            V3_सङ्ख्या isEq{pinValuep, 1};
            isEq.opEq(var_अङ्क, hierOptParamp->अङ्क());
            फर्काऊ isEq.isNeqZero();
        }
    }
};

//######################################################################
// Remove parameters from cells and build new modules

वर्ग ParamProcessor final {
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
    अचर VNUser2InUse m_inuser2;
    अचर VNUser3InUse m_inuser3;
    अचर VNUser4InUse m_inuser4;
    // User1 used by constant function simulations

    // TYPES
    // Note may have duplicate entries
    using IfaceRefRefs = std::deque<std::pair<AstIfaceRefDType*, AstIfaceRefDType*>>;

    // STATE
    using CloneMap = std::unordered_map<अचर Ast_गाँठो*, Ast_गाँठो*>;
    संरचना ModInfo final {
        Ast_गाँठो_Module* अचर m_modp;  // Module with specified name
        CloneMap m_cloneMap;  // Map of old-varp -> new cloned varp
        explicit ModInfo(Ast_गाँठो_Module* modp)
            : m_modp{modp} {}
    };
    std::map<अचर std::string, ModInfo> m_modNameMap;  // Hash of created module flavors by name

    std::map<अचर std::string, std::string>
        m_longMap;  // Hash of very long names to unique identity number
    std::set<std::string> m_longSuffixes;  // Digest suffixes used by m_longMap

    // All module names that are loaded from source code
    // Generated modules by this visitor is not included
    VStringSet m_allModuleNames;

    CloneMap m_originalParams;  // Map between parameters of copied parameteized classes and their
                                // original nodes

    std::map<अचर std::string, std::string> m_मान_Names;  // Parameter value text to its name
    std::set<std::string> m_मान_Suffixes;  // Digest suffixes used by m_valueNames
    // Number of digest hex digits in a name. With 8 digits, a collision between names made in
    // different runs, such as those of different hierarchical blocks, is unlikely. Within one
    // run, digestSuffix resolves a collision by lengthening the later text's suffix, which can
    // then differ between runs.
    स्थिर constexpr std::string::size_type DIGEST_DIGITS = 8;

    अचर Ast_गाँठो_Module* m_modp = nullptr;  // Current module being processed

    // Database to get lib-create wrapper that matches parameters in hierarchical Verilation
    ParameterizedHierBlocks m_hierBlocks;
    // Default parameter values key:parameter name, value:default value (can be nullptr)
    using Default_मान_Map = std::map<std::string, Ast_गाँठो*>;
    // Default parameter values of hierarchical blocks
    std::map<Ast_गाँठो_Module*, Default_मान_Map> m_default_Parameter_मानहरू;
    // Module -> a defparam in it or below it, or nullptr (see nestedDefparamp)
    std::map<अचर Ast_गाँठो_Module*, अचर AstPin*> m_nestedDefparamps;
    VNDeleter m_deleter;  // Used to delay deletion of nodes
    // Class default type paramater dependencies
    std::vector<std::pair<AstParamTypeDType*, पूर्णाङ्क>> m_classTypeParams;
    std::unordered_map<AstParamTypeDType*, पूर्णाङ्क> m_paramIndex;

    // Guard against infinite recursion in classTypeMatchesDefaultClone slow path
    std::unordered_set<अचर AstClass*> m_defaultCloneInProgress;

    std::vector<AstClass*> m_specializedClassps;  // Classes specialized since last drained

    // member names cached for fast lookup
    VMemberMap m_memberMap;

    // METHODS

    स्थिर शून्य makeSmallNames(Ast_गाँठो_Module* modp) {
        std::vector<पूर्णाङ्क> usedLetter;
        usedLetter.resize(256);
        // Pass 1, assign first letter to each gparam's name
        लागि (Ast_गाँठो* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            यदि (AstVar* अचर varp = VN_CAST(stmtp, Var)) {
                यदि (varp->isGParam() || varp->isIfaceRef()) {
                    वर्ण ch = varp->name()[0];
                    ch = std::बड़े(ch);
                    यदि (ch < 'A' || ch > 'Z') ch = 'Z';
                    varp->user3(usedLetter[static_cast<पूर्णाङ्क>(ch)] * 256 + ch);
                    usedLetter[static_cast<पूर्णाङ्क>(ch)]++;
                }
            } अन्यथा यदि (AstParamTypeDType* अचर typep = VN_CAST(stmtp, ParamTypeDType)) {
                अचर वर्ण ch = 'T';
                typep->user3(usedLetter[static_cast<पूर्णाङ्क>(ch)] * 256 + ch);
                usedLetter[static_cast<पूर्णाङ्क>(ch)]++;
            }
        }
    }
    स्थिर string paramSmallName(Ast_गाँठो_Module* modp, Ast_गाँठो* varp) {
        यदि (varp->user3() <= 1) makeSmallNames(modp);
        पूर्णाङ्क index = varp->user3() / 256;
        अचर वर्ण ch = varp->user3() & 255;
        string st = cvtToStr(ch);
        जबसम्म (index) {
            st += cvtToStr(static_cast<वर्ण>((index % 25) + 'A'));
            index /= 26;
        }
        फर्काऊ st;
    }

    स्थिर तार्किक is_Aggregate_Param_मान(अचर Ast_गाँठो* nodep) {
        फर्काऊ VN_IS(nodep, ConsArray) || VN_IS(nodep, ConsStruct);
    }

    स्थिर string param_मान_String(अचर Ast_गाँठो* nodep) {
        यदि (अचर AstRefDType* अचर refp = VN_CAST(nodep, RefDType)) {
            nodep = refp->skipRefToNonRefp();
        }
        string साँचो = nodep->name();
        यदि (अचर AstIfaceRefDType* अचर ifrtp = VN_CAST(nodep, IfaceRefDType)) {
            यदि (ifrtp->cellp() && ifrtp->cellp()->modp()) {
                साँचो = ifrtp->cellp()->modp()->name();
            } अन्यथा यदि (ifrtp->ifacep()) {
                साँचो = ifrtp->ifacep()->name();
            } अन्यथा {
                nodep->v3fatalSrc("Can't parameterize interface without module name");
            }
        } अन्यथा यदि (अचर Ast_गाँठो_UOr_Struct_DType* अचर dtypep
                   = VN_CAST(nodep, गाँठो_UOr_Struct_DType)) {
            साँचो += " ";
            साँचो += dtypep->verilogKwd();
            साँचो += " {";
            लागि (अचर Ast_गाँठो* memberp = dtypep->membersp(); memberp;
                 memberp = memberp->nextp()) {
                साँचो += param_मान_String(memberp);
                साँचो += ";";
            }
            साँचो += "}";
        } अन्यथा यदि (अचर AstMemberDType* अचर dtypep = VN_CAST(nodep, MemberDType)) {
            साँचो += " ";
            साँचो += param_मान_String(dtypep->subDTypep());
        } अन्यथा यदि (अचर AstBasicDType* अचर dtypep = VN_CAST(nodep, BasicDType)) {
            यदि (dtypep->isSigned()) साँचो += " signed";
            यदि (dtypep->isRanged()) {
                साँचो += "[" + cvtToStr(dtypep->बायाँ()) + ":" + cvtToStr(dtypep->दायाँ()) + "]";
            }
        } अन्यथा यदि (अचर AstPackArrayDType* अचर dtypep = VN_CAST(nodep, PackArrayDType)) {
            साँचो += "[";
            साँचो += cvtToStr(dtypep->बायाँ());
            साँचो += ":";
            साँचो += cvtToStr(dtypep->दायाँ());
            साँचो += "] ";
            साँचो += param_मान_String(dtypep->subDTypep());
        } अन्यथा यदि (अचर AstPattern* अचर patp = VN_CAST(nodep, Pattern)) {
            // A parameter override pattern, keyed before the specialized module types it
            साँचो += "'{";
            लागि (अचर AstPatMember* memp = VN_AS(patp->itemsp(), PatMember); memp;
                 memp = VN_AS(memp->nextp(), PatMember)) {
                यदि (memp->isDefault()) साँचो += "default";
                यदि (अचर AstText* अचर textp = VN_CAST(memp->keyp(), Text)) {
                    साँचो += textp->text();
                } अन्यथा यदि (memp->keyp()) {
                    साँचो += param_मान_String(memp->keyp());
                }
                साँचो += ":";
                यदि (memp->repp()) साँचो += param_मान_String(memp->repp()) + "x";
                लागि (अचर Ast_गाँठो_अभिव्यक्ति* valuep = memp->lhssp(); valuep;
                     valuep = VN_AS(valuep->nextp(), गाँठो_अभिव्यक्ति)) {
                    साँचो += param_मान_String(valuep) + ";";
                }
                साँचो += ",";
            }
            साँचो += "}";
        } अन्यथा यदि (अचर AstConsArray* अचर consArrayp = VN_CAST(nodep, ConsArray)) {
            // Include the indices and the default, as with a default the items may be sparse
            साँचो += "{";
            लागि (अचर AstConsArrayItem* itemp = consArrayp->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), ConsArrayItem)) {
                साँचो += cvtToStr(itemp->index()) + ":" + param_मान_String(itemp->valuep()) + ",";
            }
            यदि (consArrayp->defaultp()) {
                साँचो += "default:" + param_मान_String(consArrayp->defaultp()) + ",";
            }
            साँचो += "}";
        } अन्यथा यदि (अचर AstConsStruct* अचर consp = VN_CAST(nodep, ConsStruct)) {
            साँचो += "{";
            लागि (अचर AstConsStructItem* itemp = consp->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), ConsStructItem)) {
                साँचो += param_मान_String(itemp->valuep());
                साँचो += ",";
            }
            साँचो += "}";
        } अन्यथा यदि (अचर AstClassRefDType* अचर classRefp = VN_CAST(nodep, ClassRefDType)) {
            // For parameterized class types, use the original class name (without specialization
            // suffix) plus the actual type parameter values. This ensures equivalent class types
            // get the same string representation regardless of which AST node is used.
            यदि (classRefp->classp()) {
                अचर string& className = classRefp->classp()->name();
                अचर string& origName = classRefp->classp()->origName();
                अचर तार्किक isSpecialized = (className != origName);
                UINFO(9, "paramValueString ClassRefDType: name="
                             << className << " origName=" << origName
                             << " isSpecialized=" << isSpecialized
                             << " hasParams=" << (classRefp->paramsp() ? "Y" : "N")
                             << " classHasGParam=" << classRefp->classp()->hasGParam());

                यदि (classRefp->paramsp()) {
                    // ClassRefDType should have been deparameterized (paramsp
                    // consumed) before cellPinCleanup calls paramValueString.
                    classRefp->v3fatalSrc(  // LCOV_EXCL_LINE
                        "ClassRefDType still has paramsp in paramValueString");
                } अन्यथा यदि (isSpecialized) {
                    // Already specialized class (e.g., c1__Tz1_TBz1) - use full name
                    // This ensures different specializations are distinguished
                    साँचो = className;
                } अन्यथा {
                    // Unspecialized class with no params - use origName
                    साँचो = origName;
                }
            } अन्यथा {
                // classp() should always be set; unresolved class refs
                // would have errored in LinkDot.
                classRefp->v3fatalSrc(  // LCOV_EXCL_LINE
                    "ClassRefDType has null classp in paramValueString");
            }
        } अन्यथा यदि (अचर AstUnpackArrayDType* अचर dtypep = VN_CAST(nodep, UnpackArrayDType)) {
            // Name containers by their elements as above, not by prettyDTypeName(), which names
            // classes by parameter values that may be yet to be resolved
            साँचो = paramElemString(dtypep->subDTypep()) + "$" + cvtToStr(dtypep->declRange());
        } अन्यथा यदि (अचर AstQueueDType* अचर dtypep = VN_CAST(nodep, QueueDType)) {
            साँचो = paramElemString(dtypep->subDTypep()) + "$[$";
            यदि (dtypep->boundConst()) साँचो += ":" + cvtToStr(dtypep->boundConst());
            साँचो += "]";
        } अन्यथा यदि (अचर AstDynArrayDType* अचर dtypep = VN_CAST(nodep, DynArrayDType)) {
            साँचो = paramElemString(dtypep->subDTypep()) + "$[]";
        } अन्यथा यदि (अचर AstAssocArrayDType* अचर dtypep = VN_CAST(nodep, AssocArrayDType)) {
            साँचो = paramElemString(dtypep->subDTypep()) + "$["
                  + paramElemString(dtypep->साँचो_DTypep()) + "]";
        } अन्यथा यदि (अचर AstWildcardArrayDType* अचर dtypep
                   = VN_CAST(nodep, WildcardArrayDType)) {
            साँचो = paramElemString(dtypep->subDTypep()) + "$[*]";
        } अन्यथा यदि (अचर AstEnumDType* अचर dtypep = VN_CAST(nodep, EnumDType)) {
            // With the items, as prettyDTypeName() omits them, yet a specialization of a class
            // may declare an enumeration of the same name with other values
            साँचो += " enum {";
            लागि (अचर AstEnumItem* itemp = dtypep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), EnumItem)) {
                अचर AstConst* अचर constp = VN_CAST(itemp->valuep(), Const);
                साँचो += itemp->name() + "=" + (constp ? constp->अङ्क().ascii() : "?") + ";";
            }
            साँचो += "}";
        } अन्यथा यदि (अचर Ast_गाँठो_DType* अचर dtypep = VN_CAST(nodep, गाँठो_DType)) {
            साँचो += dtypep->prettyDTypeName(सत्य);
        }
        UASSERT_OBJ(!साँचो.empty(), nodep, "Parameter yielded no value string");
        फर्काऊ साँचो;
    }
    // As paramValueString(), for the element or key type of a container
    स्थिर string paramElemString(अचर Ast_गाँठो_DType* dtypep) {
        // Unlike at the top of a value, a class may be yet to be specialized, so just name it
        अचर AstClassRefDType* अचर classRefp
            = VN_CAST(dtypep->skipRefToNonRefp(), ClassRefDType);
        यदि (classRefp && classRefp->paramsp()) फर्काऊ classRefp->prettyDTypeName(सत्य);
        फर्काऊ param_मान_String(dtypep);
    }

    // Return a name suffix for 'text' from its SHA-512 digest. Hierarchical blocks are
    // Verilated in separate runs, where a counter would restart, but the digest is the same in
    // every run. Use the shortest digest prefix of at least DIGEST_DIGITS digits that is not
    // already in 'usedr', and add it to 'usedr'.
    स्थिर string digestSuffix(अचर string& text, std::set<string>& usedr) {
        अचर string hex = VHashSha512{text}.digestHex();
        // Force collisions of the prefixes -- for testing only
        string::size_type digits = v3Global.opt.debugCollision() ? 1 : DIGEST_DIGITS;
        जबसम्म (digits < hex.size() && !usedr.घुसाउ(hex.substr(0, digits)).second) ++digits;
        फर्काऊ hex.substr(0, digits);
    }
    string param_मान_सङ्ख्या(Ast_गाँठो* nodep) {
        // All call sites resolve through skipRefToNonRefp() or pass non-DType
        // nodes, so nodep should never be a bare RefDType here.
        यदि (VN_IS(nodep, RefDType)) {  // LCOV_EXCL_LINE
            nodep->v3fatalSrc("Unexpected RefDType in paramValueNumber");  // LCOV_EXCL_LINE
        }
        // Name the value by its text, which is the same for equal values or types in every run.
        // V3Hasher is unsuitable, as it hashes node pointers, which can differ for equal types.
        // V3Hash of a string is unsuitable, as std::hash varies between C++ libraries.
        अचर string text = param_मान_String(nodep);
        अचर स्वतः pair = m_मान_Names.emplace(text, "");
        यदि (pair.second) pair.first->second = "z" + digestSuffix(text, m_मान_Suffixes);
        फर्काऊ pair.first->second;
    }
    string moduleCalcName(अचर Ast_गाँठो_Module* srcModp, अचर string& longname) {
        string newname = longname;
        यदि (longname.length() > 30) {
            अचर स्वतः pair = m_longMap.emplace(longname, "");
            यदि (pair.second) {
                // We use all upper case above, so lower here can't conflict
                pair.first->second
                    = srcModp->name() + "__pi" + digestSuffix(longname, m_longSuffixes);
            }
            newname = pair.first->second;
        }
        UINFO(4, "Name: " << srcModp->name() << "->" << longname << "->" << newname);
        फर्काऊ newname;
    }
    स्थिर Ast_गाँठो_DType* arraySubDTypep(Ast_गाँठो_DType* nodep) {
        // If an unpacked array, return the subDTypep under it
        यदि (अचर AstUnpackArrayDType* अचर adtypep = VN_CAST(nodep, UnpackArrayDType)) {
            फर्काऊ adtypep->subDTypep();
        }
        // We have not resolved parameter of the child yet, so still
        // have BracketArrayDType's. We'll presume it'll end up as assignment
        // compatible (or V3Width will complain).
        यदि (अचर AstBracketArrayDType* अचर adtypep = VN_CAST(nodep, BracketArrayDType)) {
            फर्काऊ adtypep->subDTypep();
        }
        फर्काऊ nullptr;
    }
    // Peel all unpacked-array layers to reach the innermost subDTypep.
    // Used for multi-dim iface arrays where port dtype is nested UnpackArrayDType.
    स्थिर Ast_गाँठो_DType* arraySubDTypeDeepp(Ast_गाँठो_DType* nodep) {
        जबसम्म (Ast_गाँठो_DType* अचर subp = arraySubDTypep(nodep)) nodep = subp;
        फर्काऊ nodep;
    }
    स्थिर तार्किक isString(Ast_गाँठो_DType* nodep) {
        यदि (AstBasicDType* अचर basicp = VN_CAST(nodep->skipRefToNonRefp(), BasicDType))
            फर्काऊ basicp->isString();
        फर्काऊ असत्य;
    }
    शून्य collectPins(CloneMap* clonemapp, Ast_गाँठो_Module* modp, तार्किक originalIsCopy) {
        // Grab all I/O so we can remap our pins later
        लागि (Ast_गाँठो* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            अचर Ast_गाँठो* originalParamp = nullptr;
            यदि (AstVar* अचर varp = VN_CAST(stmtp, Var)) {
                यदि (varp->isIO() || varp->isGParam() || varp->isIfaceRef()) {
                    // Cloning saved a pointer to the new node for us, so just follow that link.
                    originalParamp = varp->clonep();
                }
            } अन्यथा यदि (AstParamTypeDType* अचर ptp = VN_CAST(stmtp, ParamTypeDType)) {
                यदि (ptp->isGParam()) originalParamp = ptp->clonep();
            }
            यदि (originalIsCopy) originalParamp = m_originalParams[originalParamp];
            यदि (originalParamp) clonemapp->emplace(originalParamp, stmtp);
        }
    }
    शून्य relinkPins(अचर CloneMap* clonemapp, AstPin* startpinp) {
        लागि (AstPin* pinp = startpinp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            यदि (pinp->modVarp()) {
                // Find it in the clone structure
                // UINFO(8,"Clone find 0x"<<hex<<(uint32_t)pinp->modVarp());
                अचर स्वतः cloneiter = clonemapp->find(pinp->modVarp());
                UASSERT_OBJ(cloneiter != clonemapp->end(), pinp,
                            "Couldn't find pin in clone list");
                pinp->modVarp(VN_AS(cloneiter->second, Var));
            } अन्यथा यदि (pinp->modPTypep()) {
                अचर स्वतः cloneiter = clonemapp->find(pinp->modPTypep());
                UASSERT_OBJ(cloneiter != clonemapp->end(), pinp,
                            "Couldn't find pin in clone list");
                pinp->modPTypep(VN_AS(cloneiter->second, ParamTypeDType));
            } अन्यथा {
                pinp->v3fatalSrc("Not linked?");
            }
        }
    }
    शून्य relinkPinsByName(AstPin* startpinp, Ast_गाँठो_Module* modp) {
        std::map<अचर string, AstVar*> nameToPin;
        लागि (Ast_गाँठो* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            यदि (AstVar* अचर varp = VN_CAST(stmtp, Var)) {
                यदि (varp->isIO() || varp->isGParam() || varp->isIfaceRef()) {
                    nameToPin.emplace(varp->name(), varp);
                }
            }
        }
        लागि (AstPin* pinp = startpinp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            यदि (अचर AstVar* अचर varp = pinp->modVarp()) {
                अचर स्वतः varIt = vlstd::as_const(nameToPin).find(varp->name());
                UASSERT_OBJ(varIt != nameToPin.end(), varp,
                            "Not found in " << modp->prettyNameQ());
                pinp->modVarp(varIt->second);
            }
        }
    }
    स्थिर तार्किक hasDescendantDefparams(अचर Ast_गाँठो_Module* modp,
                                       std::set<अचर Ast_गाँठो_Module*>& visited) {
        यदि (!visited.घुसाउ(modp).second) फर्काऊ असत्य;
        लागि (Ast_गाँठो* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstCell* अचर cellp = VN_CAST(stmtp, Cell);
            यदि (!cellp) जारी;
            लागि (AstPin* pinp = cellp->paramsp(); pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                यदि (!pinp->paramPath().empty()) {
                    visited.erase(modp);
                    फर्काऊ सत्य;
                }
            }
            यदि (Ast_गाँठो_Module* अचर childModp = cellp->modp()) {
                यदि (hasDescendantDefparams(childModp, visited)) {
                    visited.erase(modp);
                    फर्काऊ सत्य;
                }
            }
        }
        visited.erase(modp);
        फर्काऊ असत्य;
    }
    स्थिर तार्किक hasDescendantDefparams(अचर Ast_गाँठो_Module* modp) {
        std::set<अचर Ast_गाँठो_Module*> visited;
        फर्काऊ hasDescendantDefparams(modp, visited);
    }
    // Return a defparam in the module or below it. Unlike hasDescendantDefparams, also look
    // inside generate constructs. Memoized, so each module is searched once.
    अचर AstPin* nestedDefparamp(अचर Ast_गाँठो_Module* modp) {
        अचर स्वतः pair = m_nestedDefparamps.emplace(modp, nullptr);
        यदि (!pair.second) फर्काऊ pair.first->second;  // Searched, or being searched
        अचर AstPin* foundp = nullptr;
        modp->exists([&](अचर AstCell* cellp) {
            लागि (अचर AstPin* pinp = cellp->paramsp(); pinp && !foundp;
                 pinp = VN_AS(pinp->nextp(), Pin)) {
                यदि (!pinp->paramPath().empty()) foundp = pinp;
            }
            यदि (!foundp && cellp->modp()) foundp = nestedDefparamp(cellp->modp());
            फर्काऊ foundp != nullptr;
        });
        pair.first->second = foundp;
        फर्काऊ foundp;
    }
    // Check if parameter setting during instantiation is simple enough for hierarchical Verilation
    शून्य checkSupportedParam(Ast_गाँठो_Module* modp, AstPin* pinp) अचर {
        // ConsArray is not supported because that can not be set via -G
        // option.
        यदि (pinp->modVarp()) {
            तार्किक supported = असत्य;
            यदि (अचर AstConst* अचर constp = VN_CAST(pinp->exprp(), Const)) {
                supported = सत्य;
                यदि (constp->अङ्क().isString()
                    && !V3HierBlock::stringParamPassable(constp->अङ्क().toString())) {
                    pinp->v3warn(E_UNSUPPORTED, "Unsupported: String value of hierarchical block"
                                                " parameter with newline, double quote, '/*',"
                                                " or whitespace before '//'");
                }
            }
            यदि (!supported) {
                pinp->v3error(
                    Ast_गाँठो::prettyNameQ(modp->origName())
                    << " has hier_block metacomment, hierarchical Verilation"
                    << " supports only integer/floating point/string and type param parameters");
            }
        } अन्यथा यदि (अचर AstParamTypeDType* अचर typep = pinp->modPTypep()) {
            // Libraries are found by comparing values only
            pinp->v3warn(E_UNSUPPORTED, "Unsupported: Setting type parameter "
                                            << typep->prettyNameQ() << " of hierarchical block "
                                            << Ast_गाँठो::prettyNameQ(modp->origName()));
        }
    }
    तार्किक moduleExists(अचर string& modName) अचर {
        यदि (m_allModuleNames.find(modName) != m_allModuleNames.end()) फर्काऊ सत्य;
        यदि (m_modNameMap.find(modName) != m_modNameMap.end()) फर्काऊ सत्य;
        फर्काऊ असत्य;
    }

    string parameterizedHierBlockName(Ast_गाँठो_Module* modp, AstPin* paramPinsp) {
        // Create a unique name in the following steps
        //  - Make a long name that includes all parameters, that appear
        //    in the alphabetical order.
        //  - Hash the long name to get valid Verilog symbol
        UASSERT_OBJ(modp->hierBlock(), modp, "should be used for hierarchical block");

        std::map<string, Ast_गाँठो*> pins;

        AstPin* pinp = paramPinsp;
        जबसम्म (pinp) {
            checkSupportedParam(modp, pinp);
            यदि (अचर AstVar* अचर varp = pinp->modVarp()) {
                यदि (!pinp->exprp()) जारी;
                यदि (varp->isGParam()) pins.emplace(varp->name(), pinp->exprp());
            } अन्यथा यदि (VN_IS(pinp->exprp(), BasicDType) || VN_IS(pinp->exprp(), गाँठो_DType)) {
                pins.emplace(pinp->name(), pinp->exprp());
            }
            pinp = VN_AS(pinp->nextp(), Pin);
        }

        अचर स्वतः pair = m_default_Parameter_मानहरू.emplace(
            std::piecewise_construct, std::forward_as_tuple(modp), std::forward_as_tuple());
        यदि (pair.second) {  // Not cached yet, so check parameters
            // A separately compiled library cannot receive defparams aimed into its body
            यदि (अचर AstPin* अचर defparamp = nestedDefparamp(modp)) {
                defparamp->v3warn(E_UNSUPPORTED, "Unsupported: defparam inside hierarchical block "
                                                     << Ast_गाँठो::prettyNameQ(modp->origName()));
            }
            // Using map with key=string so that we can scan it in deterministic order
            Default_मान_Map params;
            लागि (Ast_गाँठो* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                यदि (AstVar* अचर varp = VN_CAST(stmtp, Var)) {
                    यदि (varp->isGParam()) {
                        AstConst* अचर constp = VN_CAST(varp->valuep(), Const);
                        // constp can be nullptr if the parameter is not used to instantiate sub
                        // module. varp->valuep() is not constified yet in the case.
                        // nullptr means that the parameter is using some default value.
                        params.emplace(varp->name(), constp);
                    }
                } अन्यथा यदि (AstParamTypeDType* अचर p = VN_CAST(stmtp, ParamTypeDType)) {
                    Ast_गाँठो* अचर dtypep = static_cast<Ast_गाँठो*>(p->skipRefp());
                    params.emplace(p->name(), dtypep);
                }
            }
            pair.first->second = std::move(params);
        }
        अचर स्वतः paramsIt = pair.first;
        यदि (paramsIt->second.empty()) फर्काऊ modp->origName();  // modp has no parameter

        string longname = modp->origName();
        लागि (स्वतः&& default_मान : paramsIt->second) {
            अचर स्वतः pinIt = pins.find(default_मान.first);
            // If the pin does not have a value assigned, use the default one.
            अचर Ast_गाँठो* अचर nodep = pinIt == pins.end() ? default_मान.second : pinIt->second;
            // This longname is not valid as verilog symbol, but ok, because it will be hashed
            longname += "_" + default_मान.first + "=";
            // constp can be nullptr

            यदि (अचर AstConst* अचर p = VN_CAST(nodep, Const)) {
                // Treat modules parameterized with the same values but with different type as the
                // same.
                longname += p->अङ्क().ascii(असत्य);
            } अन्यथा यदि (nodep) {
                std::stringstream type;
                V3EmitV::verilog_For_रूख(nodep, type);
                longname += type.str();
            }
        }
        UINFO(9, "       module params longname: " << longname);

        अचर स्वतः iter = m_longMap.find(longname);
        यदि (iter != m_longMap.end()) फर्काऊ iter->second;  // Already calculated

        VHashSha512 hash;
        // Calculate hash using longname
        // The hash is used as the module suffix to find a module name that is unique in the design
        hash.घुसाउ(longname);
        जबसम्म (सत्य) {
            // Copy VHashSha512 just in case of hash collision
            VHashSha512 hashStrGen = hash;
            // Hex string must be a safe suffix for any symbol
            अचर string hashStr = hashStrGen.digestHex();
            लागि (string::size_type i = 1; i < hashStr.size(); ++i) {
                string newName = modp->origName();
                // Don't use '__' not to be encoded when this module is loaded later by Verilator
                यदि (newName.at(newName.size() - 1) != '_') newName += '_';
                newName += hashStr.substr(0, i);
                यदि (!moduleExists(newName)) {
                    m_longMap.emplace(longname, newName);
                    फर्काऊ newName;
                }
            }
            // Hash collision. maybe just v3error is practically enough
            hash.घुसाउ(V3Os::trueRandom(64));
        }
    }
    शून्य replaceRefsRecurse(Ast_गाँठो* अचर nodep, अचर AstClass* अचर oldClassp,
                            AstClass* अचर newClassp) {
        // Self references linked in the first pass of V3LinkDot.cpp should point to the default
        // instance.
        यदि (AstClassRefDType* अचर classRefp = VN_CAST(nodep, ClassRefDType)) {
            यदि (classRefp->classp() == oldClassp) classRefp->classp(newClassp);
        } अन्यथा यदि (AstClassOrPackageRef* अचर classRefp = VN_CAST(nodep, ClassOrPackageRef)) {
            यदि (classRefp->classOrPackageSkipp() == oldClassp)
                classRefp->classOrPackagep(newClassp);
        } अन्यथा यदि (AstTypedef* अचर typedefp = VN_CAST(nodep, Typedef)) {
            // Update typedefs that refer to the old class to point to the new class
            यदि (typedefp->subDTypep()) {
                यदि (AstClassRefDType* अचर classRefp
                    = VN_CAST(typedefp->subDTypep(), ClassRefDType)) {
                    यदि (classRefp->classp() == oldClassp) { classRefp->classp(newClassp); }
                }
            }
        } अन्यथा यदि (Ast_गाँठो_FTask_Ref* अचर ftaskRefp = VN_CAST(nodep, गाँठो_FTask_Ref)) {
            // Also update FuncRef/TaskRef packagep to point to new class
            // This fixes static method calls through typedefs in parameterized classes
            यदि (ftaskRefp->classOrPackagep() == oldClassp) ftaskRefp->classOrPackagep(newClassp);
            // Also update taskp if it points to a function in the old class.
            // AstNodeFTask::classOrPackagep() (op2) holds a parse-time
            // Dot/ClassOrPackageRef for extern declarations, which is deleted
            // in LinkDot::moveExternFuncDecl before V3Param runs. For inline
            // class methods op2 is nullptr. So this should never match.
            यदि (Ast_गाँठो_FTask* अचर oldTaskp = ftaskRefp->taskp()) {
                यदि (oldTaskp->classOrPackagep() == oldClassp) {
                    oldTaskp->v3fatalSrc(  // LCOV_EXCL_LINE
                        "FTask classOrPackagep unexpectedly matches old class "
                        << oldClassp->prettyNameQ());
                }
            }
        }

        यदि (nodep->op1p()) replaceRefsRecurse(nodep->op1p(), oldClassp, newClassp);
        यदि (nodep->op2p()) replaceRefsRecurse(nodep->op2p(), oldClassp, newClassp);
        यदि (nodep->op3p()) replaceRefsRecurse(nodep->op3p(), oldClassp, newClassp);
        यदि (nodep->op4p()) replaceRefsRecurse(nodep->op4p(), oldClassp, newClassp);
        यदि (nodep->nextp()) replaceRefsRecurse(nodep->nextp(), oldClassp, newClassp);
    }

    // Helper visitor to update VarXRefs to use variables from specialized interfaces.
    // When a module with interface ports is cloned and the port's interface is remapped
    // to a specialized version, VarXRefs that access members of the old interface need
    // to be updated to reference the corresponding members in the new interface.
    वर्ग VarXRefRelinkVisitor final : सार्वजनिक VNVisitor {
        Ast_गाँठो_Module* m_modp;  // The cloned module we're updating
        std::unordered_map<AstVar*, Ast_गाँठो_Module*> m_varModuleMap;  // Cache var->module lookups

    सार्वजनिक:
        explicit VarXRefRelinkVisitor(Ast_गाँठो_Module* newModp)
            : m_modp{newModp} {
            iterate(newModp);
        }

    गुप्त:
        // Find which module a variable belongs to, using cache to avoid repeated backp() walks
        Ast_गाँठो_Module* findVarModule(AstVar* varp) {
            अचर स्वतः it = m_varModuleMap.find(varp);
            यदि (it != m_varModuleMap.end()) फर्काऊ it->second;
            Ast_गाँठो_Module* varModp = nullptr;
            लागि (Ast_गाँठो* np = varp; np; np = np->backp()) {
                यदि (Ast_गाँठो_Module* अचर modp = VN_CAST(np, गाँठो_Module)) {
                    varModp = modp;
                    रोक;
                }
            }
            m_varModuleMap[varp] = varModp;
            फर्काऊ varModp;
        }

        शून्य visit(AstVarXRef* nodep) override {
            AstVar* अचर varp = nodep->varp();
            यदि (!varp) {
                iterateChildren(nodep);
                फर्काऊ;
            }

            // Get the dotted prefix (port name) from the VarXRef
            // dotted() format: "portname" or "portname.subpath" or empty
            अचर string& dotted = nodep->dotted();
            यदि (dotted.empty()) {
                iterateChildren(nodep);
                फर्काऊ;
            }

            अचर माप_प्रकार dotPos = dotted.find('.');
            अचर string portName = (dotPos == string::npos) ? dotted : dotted.substr(0, dotPos);
            यदि (portName.empty()) {
                iterateChildren(nodep);
                फर्काऊ;
            }

            // Find the interface port variable in the cloned module
            AstVar* portVarp = nullptr;
            लागि (Ast_गाँठो* stmtp = m_modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                यदि (AstVar* अचर varChkp = VN_CAST(stmtp, Var)) {
                    यदि (varChkp->name() == portName && varChkp->isIfaceRef()) {
                        portVarp = varChkp;
                        रोक;
                    }
                }
            }
            यदि (!portVarp) {
                iterateChildren(nodep);
                फर्काऊ;
            }

            // Get the interface module from the port's dtype
            AstIfaceRefDType* अचर irefp = VN_CAST(portVarp->subDTypep(), IfaceRefDType);
            यदि (!irefp) {
                iterateChildren(nodep);
                फर्काऊ;
            }

            Ast_गाँठो_Module* अचर newIfacep = irefp->ifaceViaCellp();
            यदि (!newIfacep) {
                iterateChildren(nodep);
                फर्काऊ;
            }

            // Find which module the variable currently belongs to (cached)
            Ast_गाँठो_Module* अचर varModp = findVarModule(varp);

            // If variable is in a different module than the port's interface, remap it
            यदि (varModp && varModp != newIfacep) {
                लागि (Ast_गाँठो* stmtp = newIfacep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    यदि (AstVar* अचर newVarp = VN_CAST(stmtp, Var)) {
                        यदि (newVarp->name() == varp->name()) {
                            UINFO(9, "VarXRef relink " << varp->name() << " in " << varModp->name()
                                                       << " -> " << newIfacep->name());
                            nodep->varp(newVarp);
                            रोक;
                        }
                    }
                }
            }
            iterateChildren(nodep);
        }
        शून्य visit(Ast_गाँठो* nodep) override { iterateChildren(nodep); }
    };

    // Retarget the captured references in newModp to its specialized interfaces.
    शून्य fixupCrossModuleRefDTypes(Ast_गाँठो_Module* newModp) {
        यदि (!V3LinkDotIfaceCapture::enabled()) फर्काऊ;
        UINFO(9, "iface capture clone fixup: " << newModp->prettyNameQ());
        पूर्णाङ्क fixed_गन्ती = 0;
        // References in a class nested in newModp (e.g. a covergroup) are included.
        newModp->foreach([&](AstRefDType* refp) {
            अचर VIfaceCaptureTag* अचर tagp = V3LinkDotIfaceCapture::captureTag(refp);
            यदि (!tagp) फर्काऊ;

            Ast_गाँठो_Module* अचर correctModp
                = V3LinkDotIfaceCapture::followCellPath(newModp, tagp->m_cellPath);
            UINFO(9, "  path fixup: " << refp << " cellPath='" << tagp->m_cellPath << "' -> "
                                      << (correctModp ? correctModp->name() : "<null>"));
            यदि (!correctModp || correctModp->dead()) फर्काऊ;
            यदि (correctModp->parameterizedTemplate()) फर्काऊ;

            तार्किक fixed = असत्य;
            यदि (refp->typedefp()) {
                यदि (AstTypedef* अचर newTdp = V3LinkDotIfaceCapture::findTypedefInModule(
                        correctModp, refp->typedefp()->name())) {
                    refp->typedefp(newTdp);
                    यदि (newTdp->subDTypep()) refp->refDTypep(newTdp->subDTypep());
                    fixed = सत्य;
                }
            }
            यदि (!fixed && refp->refDTypep()) {
                यदि (Ast_गाँठो_DType* अचर newDtp = V3LinkDotIfaceCapture::findDTypeInModule(
                        correctModp, refp->refDTypep()->name(), refp->refDTypep()->type())) {
                    refp->refDTypep(newDtp);
                    fixed = सत्य;
                }
            }
            यदि (fixed) ++fixed_गन्ती;
        });
        V3Stats::addStatSum("IfaceCapture, Ledger fixups in V3Param", fixed_गन्ती);
    }

    // Return true on success, false on error
    तार्किक deepCloneModule(Ast_गाँठो_Module* srcModp, Ast_गाँठो* ifErrorp, AstPin* paramsp,
                         अचर string& newname, अचर IfaceRefRefs& ifaceRefRefs) {
        // Deep clone of new module
        // Note all module internal variables will be re-linked to the new modules by clone
        // However links outside the module (like on the upper cells) will not.
        Ast_गाँठो_Module* newModp;
        यदि (srcModp->user3p()) {
            newModp = VN_CAST(srcModp->user3p()->clone_रूख(असत्य), गाँठो_Module);
        } अन्यथा {
            newModp = srcModp->clone_रूख(असत्य);
        }
        // AstPin normally retains links to external module formals across cloning. For a cloned
        // interface, relink pins whose formals were cloned with the interface while clonep() is
        // still valid, so nested parameterized classes use the cloned interface parameters.
        यदि (AstIface* अचर newIfacep = VN_CAST(newModp, Iface)) {
            newIfacep->foreach([](AstPin* pinp) { pinp->cloneRelinkGen(); });
        }

        // Mark the source module as a parameterized template now that a specialized
        // clone exists.  This suppresses width/type errors on the unresolved template
        // during widthParamsEdit (which runs before V3LinkDot sets dead()).
        srcModp->parameterizedTemplate(सत्य);
        // The clone is a specialized instance, not a template.  Clear the flag in
        // case it was inherited from a prior cloneTree (when srcModp was already
        // marked by an earlier specialization).
        newModp->parameterizedTemplate(असत्य);

        newModp->name(newname);
        newModp->user2(असत्य);  // We need to re-recurse this module once changed
        newModp->recursive(असत्य);
        newModp->recursiveClone(असत्य);
        newModp->hasGParam(असत्य);
        // Recursion may need level cleanups
        यदि (newModp->level() <= m_modp->level()) newModp->level(m_modp->level() + 1);
        यदि ((newModp->level() - srcModp->level()) >= (v3Global.opt.moduleRecursionDepth() - 2)) {
            ifErrorp->v3error("Exceeded maximum --module-recursion-depth of "
                              << v3Global.opt.moduleRecursionDepth());
            VL_DO_DANGLING(newModp->delete_रूख(), newModp);
            फर्काऊ असत्य;
        }

        यदि (AstClass* अचर newClassp = VN_CAST(newModp, Class)) {
            replaceRefsRecurse(newModp->stmtsp(), newClassp, VN_AS(srcModp, Class));
        }

        // Keep tree sorted by level. Note: Different parameterizations of the same recursive
        // module end up with the same level, which we will need to fix up at the end, as we do not
        // know up front how recursive modules are expanded, and a later expansion might re-use an
        // earlier expansion (see t_recursive_module_bug_2).
        Ast_गाँठो* insertp = srcModp;
        जबसम्म (VN_IS(insertp->nextp(), गाँठो_Module)
               && VN_AS(insertp->nextp(), गाँठो_Module)->level() <= newModp->level()) {
            insertp = insertp->nextp();
        }
        insertp->addNextHere(newModp);

        m_modNameMap.emplace(newModp->name(), ModInfo{newModp});
        अचर स्वतः iter = m_modNameMap.find(newname);
        CloneMap* अचर clonemapp = &(iter->second.m_cloneMap);
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
        तार्किक needRelinkByName = असत्य;
        यदि (paramsp) {
            लागि (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                यदि (pinp->modVarp() && clonemapp->find(pinp->modVarp()) == clonemapp->end()) {
                    needRelinkByName = सत्य;
                    रोक;
                }
            }
        }
        यदि (needRelinkByName) {
            relinkPinsByName(paramsp, newModp);
        } अन्यथा {
            relinkPins(clonemapp, paramsp);
        }

        // Fix any interface references
        लागि (स्वतः it = ifaceRefRefs.cbegin(); it != ifaceRefRefs.cend(); ++it) {
            अचर AstIfaceRefDType* अचर portIrefp = it->first;
            अचर AstIfaceRefDType* अचर pinIrefp = it->second;
            AstIfaceRefDType* अचर cloneIrefp = portIrefp->clonep();
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
        यदि (!ifaceRefRefs.empty()) { VarXRefRelinkVisitor{newModp}; }

        // Fix cross-module REFDTYPE pointers in newModp.
        UASSERT_OBJ(newModp, srcModp, "newModp null before hierarchy fixup");
        fixupCrossModuleRefDTypes(newModp);

        // Assign parameters to the constants specified
        // DOES clone() so must be finished with module clonep() before here
        लागि (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            यदि (pinp->exprp()) {
                यदि (AstVar* अचर modvarp = pinp->modVarp()) {
                    Ast_गाँठो* अचर newp = pinp->exprp();  // Const or ConsArray
                    AstConst* अचर exprp = VN_CAST(newp, Const);
                    AstConst* अचर origp = VN_CAST(modvarp->valuep(), Const);
                    अचर तार्किक overridden
                        = !(origp && exprp && ParameterizedHierBlocks::areSame(exprp, origp));
                    // Remove any existing parameter
                    यदि (modvarp->valuep()) modvarp->valuep()->unlinkFrBack()->delete_रूख();
                    // Set this parameter to value requested by cell
                    UINFO(9, "       set param " << modvarp << " = " << newp);
                    modvarp->valuep(newp->clone_रूख(असत्य));
                    modvarp->overriddenParam(overridden);
                } अन्यथा यदि (AstParamTypeDType* अचर modptp = pinp->modPTypep()) {
                    Ast_गाँठो_DType* अचर dtypep = VN_AS(pinp->exprp(), गाँठो_DType);
                    UASSERT_OBJ(dtypep, pinp, "unlinked param dtype");
                    यदि (modptp->childDTypep()) modptp->childDTypep()->unlinkFrBack()->delete_रूख();
                    // Set this parameter to value requested by cell
                    modptp->childDTypep(dtypep->clone_रूख(असत्य));
                    // Later V3LinkDot will convert the ParamDType to a Typedef
                    // Not done here as may be localparams, etc, that also need conversion
                }
            }
        }

        फर्काऊ सत्य;
    }
    अचर ModInfo* moduleFindOrClone(Ast_गाँठो_Module* srcModp, Ast_गाँठो* ifErrorp, AstPin* paramsp,
                                     अचर string& newname, अचर IfaceRefRefs& ifaceRefRefs) {
        // Already made this flavor?
        स्वतः it = m_modNameMap.find(newname);
        यदि (it != m_modNameMap.end()) {
            UINFO(4, "     De-parameterize to prev: " << it->second.m_modp);
        } अन्यथा {
            यदि (!deepCloneModule(srcModp, ifErrorp, paramsp, newname, ifaceRefRefs)) {
                फर्काऊ nullptr;
            }
            it = m_modNameMap.find(newname);
            UASSERT(it != m_modNameMap.end(), "should find just-made module");
        }
        अचर ModInfo* अचर modInfop = &(it->second);
        फर्काऊ modInfop;
    }

    शून्य convertToStringp(Ast_गाँठो* nodep) {
        // Should be called on values of parameters of type string to convert them
        // to properly typed string constants.
        // Has no effect if the value is not a string constant.
        AstConst* अचर constp = VN_CAST(nodep, Const);
        // Check if it wasn't already converted
        यदि (constp && !constp->अङ्क().isString()) {
            constp->replaceWith(
                नूतन AstConst{constp->fileline(), AstConst::String{}, constp->अङ्क().toString()});
            VL_DO_DANGLING(constp->delete_रूख(), constp);
        }
    }

    // Fold a param/lparam member's value to a Const in place.  Resolves any
    // class::member Dots in the value first (including those buried behind
    // VarRefs to sibling lparams that are not yet const, e.g. `one = base`
    // where `base = inner_a::v`), resolving siblings deepest-first so the
    // whole chain constifies.  `inProgress` guards against reference cycles.
    शून्य constify_Member_मान(AstVar* varp, std::set<AstVar*>& inProgress) {
        यदि (!varp->valuep() || VN_IS(varp->valuep(), Const)) फर्काऊ;
        यदि (!inProgress.घुसाउ(varp).second) {
            // Re-entered while still resolving varp: its value transitively
            // references itself.  Report and replace the value with a Const, as
            // otherwise the unresolved Dot reaches V3Width as an untyped node.
            varp->v3error("Variable's initial value is circular: " << varp->prettyNameQ());
            varp->valuep()->unlinkFrBack()->delete_रूख();
            varp->valuep(नूतन AstConst{varp->fileline(), AstConst::Signed32{}, 0});
            फर्काऊ;
        }
        // First resolve sibling lparams this value references, so their Dots
        // are folded before we inline them here.
        std::set<AstVar*> siblings;
        varp->valuep()->foreach([&](अचर AstVarRef* refp) {
            AstVar* अचर refVarp = refp->varp();
            यदि (refVarp && refVarp->isParam() && refVarp->valuep()
                && !VN_IS(refVarp->valuep(), Const)) {
                siblings.घुसाउ(refVarp);
            }
        });
        लागि (AstVar* अचर sibp : siblings) constify_Member_मान(sibp, inProgress);
        // Resolve class-scoped references and any member selections they expose.
        resolveDeferredDotsReachableFrom(varp->valuep(), m_modp);
        V3Const::constifyParamsEdit(varp);
        inProgress.erase(varp);
    }

    // Return the member name on a deferred Dot RHS, including a selected member.
    स्थिर AstParseRef* memberParseRef(Ast_गाँठो* nodep) {
        यदि (AstParseRef* अचर refp = VN_CAST(nodep, ParseRef)) फर्काऊ refp;
        Ast_गाँठो_Pre_Sel* अचर selp = VN_CAST(nodep, गाँठो_Pre_Sel);
        फर्काऊ selp ? VN_CAST(selp->fromp(), ParseRef) : nullptr;
    }

    // Replace a deferred Dot with valuep, preserving any select on the member name.
    स्थिर शून्य replaceMemberDot(AstDot* dotp, AstParseRef* memberRefp, Ast_गाँठो_अभिव्यक्ति* valuep) {
        यदि (memberRefp == dotp->rhsp()) {
            dotp->replaceWith(valuep);
        } अन्यथा {
            Ast_गाँठो_Pre_Sel* अचर selp = VN_AS(dotp->rhsp(), गाँठो_Pre_Sel);
            यदि (AstAttrOf* अचर attrp = selp->attrp()) {
                यदि (Ast_गाँठो* अचर oldFromp = attrp->fromp()) {
                    oldFromp->replaceWith(valuep->clone_रूख(असत्य));
                    VL_DO_DANGLING(oldFromp->delete_रूख(), oldFromp);
                }
            }
            memberRefp->replaceWith(valuep);
            VL_DO_DANGLING(memberRefp->delete_रूख(), memberRefp);
            dotp->replaceWith(dotp->rhsp()->unlinkFrBack());
        }
        VL_DO_DANGLING(dotp->delete_रूख(), dotp);
    }

    // True for an expression produced while lowering a deferred struct-member chain.
    स्थिर तार्किक isDeferredMemberBase(अचर Ast_गाँठो_अभिव्यक्ति* nodep) {
        यदि (VN_IS(nodep, MemberSel)) फर्काऊ सत्य;
        यदि (अचर Ast_गाँठो_Pre_Sel* अचर selp = VN_CAST(nodep, गाँठो_Pre_Sel)) {
            अचर Ast_गाँठो_DType* अचर fromDTypep = selp->fromp()->dtypep();
            अचर Ast_गाँठो_Array_DType* अचर arrayDTypep
                = fromDTypep ? VN_CAST(fromDTypep->skipRefOrNullp(), गाँठो_Array_DType) : nullptr;
            यदि (arrayDTypep) {
                अचर Ast_गाँठो_DType* अचर elemDTypep = arrayDTypep->subDTypep();
                फर्काऊ elemDTypep && VN_IS(elemDTypep->skipRefOrNullp(), गाँठो_UOr_Struct_DType);
            }
            फर्काऊ isDeferredMemberBase(selp->fromp());
        }
        अचर Ast_गाँठो_DType* अचर dtypep = nodep->dtypep();
        फर्काऊ dtypep && VN_IS(dtypep->skipRefOrNullp(), गाँठो_UOr_Struct_DType);
    }

    // Lower a Dot exposed after its inner class::member reference was substituted.
    स्थिर शून्य resolveMemberDot(AstDot* dotp) {
        Ast_गाँठो_अभिव्यक्ति* अचर lhsp = VN_CAST(dotp->lhsp(), गाँठो_अभिव्यक्ति);
        AstParseRef* अचर memberRefp = memberParseRef(dotp->rhsp());
        यदि (!lhsp || !memberRefp || !isDeferredMemberBase(lhsp)) फर्काऊ;
        AstMemberSel* अचर newp = नूतन AstMemberSel{memberRefp->fileline(), lhsp->unlinkFrBack(),
                                                    VFlagChildDType{}, memberRefp->name()};
        replaceMemberDot(dotp, memberRefp, newp);
    }

    // Helper to resolve DOT to RefDType for class type references.
    // If the class is parameterized and not yet specialized, specialize it first.
    // This handles cases like: iface #(param_class#(value)::typedef_name)
    शून्य resolveDotToTypedef(Ast_गाँठो* exprp) {
        AstDot* अचर dotp = VN_CAST(exprp, Dot);
        यदि (!dotp) फर्काऊ;
        UINFO(9, "Resolve deferred Dot: " << dotp);
        AstClassOrPackageRef* अचर classRefp = VN_CAST(dotp->lhsp(), ClassOrPackageRef);
        यदि (!classRefp) {
            resolveMemberDot(dotp);
            फर्काऊ;
        }
        AstParseRef* अचर parseRefp = memberParseRef(dotp->rhsp());
        यदि (!parseRefp) फर्काऊ;

        अचर AstClass* lhsClassp = VN_CAST(classRefp->classOrPackageSkipp(), Class);

        // Specialize parameterized class through type parameter indirection (#7000)
        AstParamTypeDType* अचर paramTypep
            = VN_CAST(classRefp->classOrPackageNodep(), ParamTypeDType);
        यदि (paramTypep) {
            Ast_गाँठो_DType* अचर dtypep = paramTypep->subDTypep();
            AstClassRefDType* अचर classRefDTypep
                = dtypep ? VN_CAST(dtypep->skipRefOrNullp(), ClassRefDType) : nullptr;
            यदि (classRefDTypep) {
                AstClass* अचर srcClassp = classRefDTypep->classp();
                यदि (srcClassp && srcClassp->hasGParam() && classRefDTypep->paramsp()) {
                    यदि (lhsClassp == srcClassp || !lhsClassp) {
                        classRefDeparam(classRefDTypep, srcClassp);
                        lhsClassp = classRefDTypep->classp();
                    }
                }
            }
        }

        यदि (classRefp->paramsp()) {
            AstClass* अचर srcClassp = VN_CAST(classRefp->classOrPackageNodep(), Class);
            यदि (srcClassp && srcClassp->hasGParam()) {
                यदि (lhsClassp == srcClassp || !lhsClassp) {
                    classRefDeparam(classRefp, srcClassp);
                    lhsClassp = VN_CAST(classRefp->classOrPackageSkipp(), Class);
                }
            }
        }
        // Typedef-aliased paramed class (e.g., typedef C#(7) my_c; my_c::b).
        यदि (AstClassRefDType* अचर crp = class_Ref_DType_Of_गाँठो(classRefp->classOrPackageNodep())) {
            यदि (AstClass* अचर srcClassp = crp->classp()) {
                यदि (srcClassp->hasGParam() && crp->paramsp()) {
                    classRefDeparam(crp, srcClassp);
                    यदि (AstClass* अचर newClassp = crp->classp()) lhsClassp = newClassp;
                }
            }
        }
        यदि (!lhsClassp) फर्काऊ;

        Ast_गाँठो* अचर memberp = m_memberMap.findMember(lhsClassp, parseRefp->name());
        UINFO(9, "Resolve deferred class member: " << parseRefp->name() << " -> " << memberp);
        यदि (AstTypedef* अचर tdefp = VN_CAST(memberp, Typedef)) {
            यदि (parseRefp != dotp->rhsp()) फर्काऊ;
            AstRefDType* अचर refp = नूतन AstRefDType{dotp->fileline(), tdefp->name()};
            refp->typedefp(tdefp);
            dotp->replaceWith(refp);
            VL_DO_DANGLING(dotp->delete_रूख(), dotp);
        } अन्यथा यदि (AstVar* अचर varp = VN_CAST(memberp, Var)) {
            substituteParamMember(dotp, parseRefp, varp);
        }
    }

    // Substitute a class param/lparam member's constant value for dotp. The
    // reverse-order deferred-Dot walk lowers any outer fields to MemberSels.
    शून्य substituteParamMember(AstDot* अचर dotp, AstParseRef* अचर memberRefp,
                               AstVar* अचर varp) {
        यदि (!varp->isParam() || !varp->valuep()) फर्काऊ;
        यदि (!VN_IS(varp->valuep(), Const)) {
            std::set<AstVar*> inProgress;
            constify_Member_मान(varp, inProgress);
        }
        AstConst* अचर constp = VN_CAST(varp->valuep(), Const);
        यदि (!constp) फर्काऊ;
        AstConst* अचर newp = constp->clone_रूख(असत्य);
        newp->dtypep(varp->subDTypep());
        replaceMemberDot(dotp, memberRefp, newp);
    }

    // Resolve one class-scoped RefDType. The generic deferred walk finds these
    // through every dtype wrapper and follows out-of-tree typedef references.
    शून्य resolveParamClassRefDType(AstRefDType* refp) {
        यदि (refp->typedefp() || refp->refDTypep()) फर्काऊ;

        AstClassOrPackageRef* अचर classRefp
            = VN_CAST(refp->classOrPackageOpp(), ClassOrPackageRef);
        यदि (!classRefp) फर्काऊ;

        AstClass* srcClassp = VN_CAST(classRefp->classOrPackageNodep(), Class);
        यदि (srcClassp && srcClassp->hasGParam()) {
            classRefDeparam(classRefp, srcClassp);
            फर्काऊ;
        }

        // Follow typedef chain to find the underlying ClassRefDType
        Ast_गाँठो* targetp = classRefp->classOrPackageNodep();
        जबसम्म (अचर AstTypedef* अचर tdefp = VN_CAST(targetp, Typedef))
            targetp = tdefp->subDTypep();
        यदि (Ast_गाँठो_DType* अचर dtargetp = VN_CAST(targetp, गाँठो_DType))
            targetp = dtargetp->skipRefOrNullp();
        यदि (!targetp) फर्काऊ;
        AstClassRefDType* अचर classRefDTypep = VN_CAST(targetp, ClassRefDType);
        यदि (!classRefDTypep) फर्काऊ;
        srcClassp = classRefDTypep->classp();
        यदि (!srcClassp) फर्काऊ;

        AstClass* newClassp = srcClassp;
        यदि (srcClassp->hasGParam()) {
            classRefDeparam(classRefDTypep, srcClassp);
            newClassp = classRefDTypep->classp();
        }

        AstTypedef* अचर typedefp
            = VN_CAST(m_memberMap.findMember(newClassp, refp->name()), Typedef);
        यदि (typedefp) {
            refp->typedefp(typedefp);
            refp->classOrPackagep(newClassp);
        }
    }

    // True if a $bits/$size type query in nodep's parameters reads another type parameter.
    स्थिर तार्किक defaultParamsHaveTypeQueryOnParamType(अचर AstClassRefDType* nodep) {
        तार्किक found = असत्य;
        nodep->foreach([&](अचर AstAttrOf* attrp) {
            यदि (found || !attrp->attrType().isTypeQuery()) फर्काऊ;
            अचर AstRefDType* अचर refp = VN_CAST(attrp->fromp(), RefDType);
            यदि (refp && VN_IS(refp->refDTypep(), ParamTypeDType)) found = सत्य;
        });
        फर्काऊ found;
    }

    // Check if exprp's class matches origp's class after deparameterization.
    // Handles both the simple case (user4p link from defaultsResolved) and the
    // nested case where the default's inner class has non-default sub-parameters
    // (e.g., uvm_sequence#(uvm_reg_item) where uvm_reg_item != default uvm_sequence_item).
    तार्किक classTypeMatchesDefaultClone(अचर Ast_गाँठो_DType* exprp, अचर Ast_गाँठो_DType* origp) {
        exprp = exprp->skipRefp();
        origp = origp->skipRefp();
        अचर स्वतः* अचर अभिव्यक्ति_Class_Refp = VN_CAST(exprp, ClassRefDType);
        अचर स्वतः* अचर origClassRefp = VN_CAST(origp, ClassRefDType);
        यदि (!अभिव्यक्ति_Class_Refp || !origClassRefp) फर्काऊ असत्य;
        // Fast path: check user4p link (set when template was deparameterized with defaults)
        अचर Ast_गाँठो_Module* अचर defaultClonep
            = VN_CAST(origClassRefp->classp()->user4p(), Class);
        यदि (defaultClonep && defaultClonep == अभिव्यक्ति_Class_Refp->classp()) फर्काऊ सत्य;
        // Skip the comparison when the default's $bits/$size reads another type parameter, as
        // deparameterizing it below would resolve that shared type at the wrong width (#7711).
        यदि (defaultParamsHaveTypeQueryOnParamType(origClassRefp)) फर्काऊ असत्य;
        // Slow path: deparameterize the default type and compare the result.
        // Different templates can never match; use origName() because exprp's
        // class may already be a specialization (clone) of the template.
        यदि (!origClassRefp->classp()->hasGParam()) फर्काऊ असत्य;
        यदि (origClassRefp->classp()->origName() != अभिव्यक्ति_Class_Refp->classp()->origName())
            फर्काऊ असत्य;
        // Prevent re-entry when classRefDeparam recurses through cellPinCleanup.
        यदि (!m_defaultCloneInProgress.घुसाउ(origClassRefp->classp()).second) फर्काऊ असत्य;
        // const_cast safe: cloneTree doesn't modify the source
        AstClassRefDType* अचर origClonep = static_cast<AstClassRefDType*>(
            const_cast<AstClassRefDType*>(origClassRefp)->clone_रूख(असत्य));
        Ast_गाँठो_Module* अचर resolvedModp = classRefDeparam(origClonep, origClassRefp->classp());
        अचर तार्किक match = resolvedModp && VN_CAST(resolvedModp, Class) == अभिव्यक्ति_Class_Refp->classp();
        VL_DO_DANGLING(origClonep->delete_रूख(), origClonep);
        m_defaultCloneInProgress.erase(origClassRefp->classp());
        फर्काऊ match;
    }

    स्थिर तार्किक paramConstsEqualAtMaxWidth(AstConst* exprp, AstConst* origp) {
        // Return true if two integer constants are equal after sign-extending
        // both to max(width).  A typed parameter default (e.g. byte) is
        // narrower than a 32-bit pin expression, so sameTree/areSame fail.
        यदि (exprp->अङ्क().width() == origp->अङ्क().width()) फर्काऊ असत्य;
        यदि (exprp->अङ्क().isOpaque()) फर्काऊ असत्य;
        यदि (origp->अङ्क().isOpaque()) फर्काऊ असत्य;
        अचर पूर्णाङ्क maxWidth = std::max(exprp->अङ्क().width(), origp->अङ्क().width());
        V3_सङ्ख्या अभिव्यक्ति_अङ्क{exprp, maxWidth};
        यदि (exprp->अङ्क().isSigned()) {
            अभिव्यक्ति_अङ्क.opExtendS(exprp->अङ्क(), exprp->अङ्क().width());
        } अन्यथा {
            अभिव्यक्ति_अङ्क.opAssign(exprp->अङ्क());
        }
        V3_सङ्ख्या orig_अङ्क{origp, maxWidth};
        यदि (origp->अङ्क().isSigned()) {
            orig_अङ्क.opExtendS(origp->अङ्क(), origp->अङ्क().width());
        } अन्यथा {
            orig_अङ्क.opAssign(origp->अङ्क());
        }
        V3_सङ्ख्या isEq{exprp, 1};
        isEq.opEq(अभिव्यक्ति_अङ्क, orig_अङ्क);
        फर्काऊ isEq.isNeqZero();
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
        लागि (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            यदि (!pinp->exprp()) जारी;
            यदि (!VString::startsWith(pinp->name(), "__VGIfaceParam")) जारी;
            अचर Ast_गाँठो_Var_Ref* अचर fwdRefp = VN_CAST(pinp->exprp(), गाँठो_Var_Ref);
            यदि (!fwdRefp) जारी;
            अचर AstIfaceRefDType* अचर resolvedp
                = VN_CAST(fwdRefp->varp()->childDTypep()->skipRefp(), IfaceRefDType);
            UASSERT_OBJ(resolvedp, pinp,
                        "Generic interface forwarding pin not specialized before use");
            AstIfaceRefDType* अचर newIrefp = नूतन AstIfaceRefDType{
                resolvedp->fileline(), resolvedp->modportFileline(), resolvedp->cellName(),
                resolvedp->ifaceName(), resolvedp->modportName()};
            newIrefp->ifacep(resolvedp->ifacep());
            यदि (resolvedp->paramsp()) {
                newIrefp->addParamsp(resolvedp->paramsp()->clone_रूख(सत्य));
            }
            pinp->exprp()->unlinkFrBack()->delete_रूख();
            pinp->exprp(newIrefp);
        }
    }

    // Include the pin's value in the specialization name, so cells passing equal
    // values share one module clone.
    शून्य name_By_Pin_मान(AstPin* pinp, Ast_गाँठो_Module* srcModp, AstVar* modvarp, string& longnamer,
                        तार्किक& any_overridesr) {
        longnamer += "_" + paramSmallName(srcModp, modvarp) + param_मान_सङ्ख्या(pinp->exprp());
        any_overridesr = सत्य;
    }

    // Name the specialization by a value pin, or prepare a type pin, noting in
    // overridingTypePinsr whether it differs from the default
    शून्य cellPinCleanup(Ast_गाँठो* nodep, AstPin* pinp, AstPin* paramsp, Ast_गाँठो_Module* srcModp,
                        string& longnamer, तार्किक& any_overridesr,
                        std::unordered_set<अचर AstPin*>& overridingTypePinsr) {
        यदि (!pinp->exprp()) फर्काऊ;  // No-connect
        यदि (AstVar* अचर modvarp = pinp->modVarp()) {
            resolveDeferredDotsReachableFrom(modvarp->subDTypep(), m_modp);
            यदि (!modvarp->isGParam()) {
                pinp->v3fatalSrc("Attempted parameter setting of non-parameter: Param "
                                 << pinp->prettyNameQ() << " of " << nodep->prettyNameQ());
            } अन्यथा यदि (VN_IS(pinp->exprp(), Pattern)
                       || (VN_IS(pinp->exprp(), ConsArray)
                           && arraySubDTypep(modvarp->subDTypep()))) {
                // A pattern or array value is named by its contents
                name_By_Pin_मान(pinp, srcModp, modvarp, longnamer, any_overridesr);
            } अन्यथा {
                UINFO(9, "cellPinCleanup: before constify " << pinp << " " << modvarp);
                V3Const::constifyParamsEdit(pinp->exprp());
                यदि (is_Aggregate_Param_मान(pinp->exprp())) {
                    name_By_Pin_मान(pinp, srcModp, modvarp, longnamer, any_overridesr);
                    फर्काऊ;
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
                यदि (modvarp->valuep()
                    && (VN_IS(modvarp->valuep(), Cast) || VN_IS(modvarp->valuep(), CastSize))) {
                    अचर तार्किक dependent = modvarp->valuep()->exists(
                        [](Ast_गाँठो* np) { फर्काऊ VN_IS(np, VarRef) || VN_IS(np, RefDType); });
                    यदि (!dependent) V3Const::constifyParamsEdit(modvarp->valuep());
                }
                UINFO(9, "cellPinCleanup: after constify " << pinp);
                // String constants are parsed as logic arrays and converted to strings in V3Const.
                // At this moment, some constants may have been already converted.
                // To correctly compare constants, both should be of the same type,
                // so they need to be converted.
                यदि (isString(modvarp->subDTypep())) {
                    convertToStringp(pinp->exprp());
                    convertToStringp(modvarp->valuep());
                }
                AstConst* अचर exprp = VN_CAST(pinp->exprp(), Const);
                AstConst* अचर origp = VN_CAST(modvarp->valuep(), Const);
                // Width the pin to the port's type so equal values hash the same (#5479).
                AstConst* normedNamep = nullptr;
                यदि (exprp && !exprp->अङ्क().isDouble() && !exprp->अङ्क().isString()) {
                    AstVar* cloneVarp = modvarp->clone_रूख(असत्य);
                    तार्किक cloneVarpUnresolved = असत्य;
                    यदि (Ast_गाँठो* अचर oldValuep = cloneVarp->valuep()) {
                        oldValuep->unlinkFrBack();
                        VL_DO_DANGLING(oldValuep->delete_रूख(), oldValuep);
                    }
                    cloneVarp->valuep(exprp->clone_रूख(असत्य));
                    यदि (Ast_गाँठो_DType* अचर origDTypep = modvarp->subDTypep()) {
                        // Attach clone under cloneVarp so the root has a back pointer.
                        यदि (cloneVarp->childDTypep())
                            cloneVarp->childDTypep()->unlinkFrBack()->delete_रूख();
                        cloneVarp->childDTypep(origDTypep->clone_रूख(असत्य));
                        cloneVarp->dtypep(nullptr);
                        // Inline param refs so widthing doesn't touch the template (#7411).
                        constexpr पूर्णाङ्क maxSubstIters = 1000;
                        लागि (पूर्णाङ्क it = 0; it < maxSubstIters; ++it) {
                            तार्किक any = असत्य;
                            cloneVarp->foreach([&](AstVarRef* varrefp) {
                                AstVar* अचर targetp = varrefp->varp();
                                Ast_गाँठो* replacep = nullptr;
                                लागि (AstPin* pp = paramsp; pp; pp = VN_AS(pp->nextp(), Pin)) {
                                    यदि (pp->modVarp() == targetp) {
                                        // A pattern or array override names the value too
                                        यदि (VN_IS(pp->exprp(), Const)
                                            || VN_IS(pp->exprp(), Pattern)
                                            || VN_IS(pp->exprp(), ConsArray)) {
                                            replacep = pp->exprp()->clone_रूख(असत्य);
                                        }
                                        रोक;
                                    }
                                }
                                यदि (!replacep && targetp->valuep()) {
                                    replacep = targetp->valuep()->clone_रूख(असत्य);
                                }
                                // An inlined pattern takes the type of its parameter
                                AstPattern* अचर patp = VN_CAST(replacep, Pattern);
                                यदि (patp && !patp->childDTypep() && targetp->childDTypep()) {
                                    patp->childDTypep(targetp->childDTypep()->clone_रूख(असत्य));
                                }
                                यदि (replacep) {
                                    varrefp->replaceWith(replacep);
                                    VL_DO_DANGLING(varrefp->delete_रूख(), varrefp);
                                    any = सत्य;
                                }
                            });
                            // Replace RefDType to a ParamTypeDType with pin override
                            // or the paramtype's default so constify below does not
                            // reach into the template.  Collect then replace in
                            // reverse so descendants aren't freed early.
                            std::vector<std::pair<AstRefDType*, Ast_गाँठो_DType*>> toReplace;
                            cloneVarp->foreach([&](AstRefDType* refp) {
                                AstParamTypeDType* अचर ptdp
                                    = VN_CAST(refp->refDTypep(), ParamTypeDType);
                                यदि (!ptdp) फर्काऊ;
                                AstPin* overridePinp = nullptr;
                                लागि (AstPin* pp = paramsp; pp; pp = VN_AS(pp->nextp(), Pin)) {
                                    यदि (pp->modPTypep() == ptdp) {
                                        overridePinp = pp;
                                        रोक;
                                    }
                                }
                                Ast_गाँठो_DType* अचर substp
                                    = overridePinp ? VN_CAST(overridePinp->exprp(), गाँठो_DType)
                                                   : ptdp->subDTypep();
                                यदि (substp) toReplace.emplace_back(refp, substp);
                            });
                            लागि (स्वतः it = toReplace.rbegin(); it != toReplace.rend(); ++it) {
                                AstRefDType* अचर refp = it->first;
                                refp->replaceWith(it->second->clone_रूख(असत्य));
                                VL_DO_DANGLING(refp->delete_रूख(), refp);
                                any = सत्य;
                            }
                            यदि (!any) रोक;
                        }
                        // Skip the widthing constify if anything left can't be evaluated here: a
                        // reference, as to a variable or around a cycle, an unresolved type
                        // parameter, or a type or enum item of the template
                        cloneVarp->foreach([&](Ast_गाँठो* np) {
                            यदि (VN_IS(np, VarRef)) {
                                cloneVarpUnresolved = सत्य;
                                फर्काऊ;
                            }
                            यदि (AstEnumItemRef* अचर itemRefp = VN_CAST(np, EnumItemRef)) {
                                यदि (V3LinkDotIfaceCapture::findOwnerModule(itemRefp->itemp())
                                    == srcModp) {
                                    cloneVarpUnresolved = सत्य;
                                }
                                फर्काऊ;
                            }
                            AstRefDType* अचर refp = VN_CAST(np, RefDType);
                            यदि (!refp) फर्काऊ;
                            यदि (VN_IS(refp->refDTypep(), ParamTypeDType)) {
                                UINFO(5, "  cellPinCleanup: skip normedNamep "
                                         "(unresolved RefDType->ParamTypeDType) pin="
                                             << pinp->prettyNameQ());
                                cloneVarpUnresolved = सत्य;
                            }
                            // A type declared in the template must not be widthed there
                            यदि (V3LinkDotIfaceCapture::findOwnerModule(refp->typedefp()) == srcModp
                                || V3LinkDotIfaceCapture::findOwnerModule(refp->refDTypep())
                                       == srcModp) {
                                cloneVarpUnresolved = सत्य;
                            }
                        });
                    }
                    यदि (!cloneVarpUnresolved) {
                        V3Const::constifyParamsEdit(cloneVarp);
                        यदि (AstConst* अचर widthedp = VN_CAST(cloneVarp->valuep(), Const)) {
                            // Stamp the port's type so equal values hash the same.
                            यदि (cloneVarp->dtypep()) widthedp->dtypep(cloneVarp->dtypep());
                            widthedp->unlinkFrBack();
                            normedNamep = widthedp;
                        }
                    }
                    VL_DO_DANGLING(cloneVarp->delete_रूख(), cloneVarp);
                }
                AstConst* अचर namingExprp = normedNamep ? normedNamep : exprp;
                यदि (!exprp) {
                    // With DepGraph architecture, all expressions should be constants
                    // by the time V3Param runs. If not, it's an error.
                    UINFO(9, "cellPinCleanup: NOT CONST after constify " << pinp);
                    UINFOTREE(1, pinp, "", "errnode");
                    pinp->v3error("Can't convert defparam value to constant: Param "
                                  << pinp->prettyNameQ() << " of " << nodep->prettyNameQ());
                    Ast_गाँठो* अचर pinExprp = pinp->exprp();
                    pinExprp->replaceWith(नूतन AstConst{pinp->fileline(), AstConst::Widthed_मान{},
                                                       modvarp->width(), 0});
                    VL_DO_DANGLING(pinExprp->delete_रूख(), pinExprp);
                } अन्यथा यदि (origp
                           && (exprp->same_रूख(origp)
                               || (exprp->अङ्क().width() == origp->अङ्क().width()
                                   && ParameterizedHierBlocks::areSame(exprp, origp))
                               || paramConstsEqualAtMaxWidth(exprp, origp))) {
                    // Setting parameter to its default value.  Just ignore it.
                    // This prevents making additional modules, and makes coverage more
                    // obvious as it won't show up under a unique module page name.
                    UINFO(9, "cellPinCleanup: same as default " << pinp);
                } अन्यथा यदि (namingExprp->अङ्क().isDouble() || namingExprp->अङ्क().isString()
                           || namingExprp->अङ्क().isFourState()
                           || namingExprp->अङ्क().width() != 32) {
                    longnamer += ("_" + paramSmallName(srcModp, modvarp)
                                  + param_मान_सङ्ख्या(namingExprp));
                    any_overridesr = सत्य;
                } अन्यथा {
                    longnamer += ("_" + paramSmallName(srcModp, modvarp)
                                  + namingExprp->अङ्क().ascii(असत्य));
                    any_overridesr = सत्य;
                }
                यदि (normedNamep) VL_DO_DANGLING(normedNamep->delete_रूख(), normedNamep);
            }
        } अन्यथा यदि (AstParamTypeDType* अचर modvarp = pinp->modPTypep()) {
            // Resolve the pin and declared type through arbitrary dtype wrappers
            // and typedef chains before widthing either one.
            resolveDeferredDotsReachableFrom(pinp->exprp(), m_modp);
            resolveDeferredDotsReachableFrom(modvarp->subDTypep(), m_modp);

            Ast_गाँठो_DType* rawTypep = VN_CAST(pinp->exprp(), गाँठो_DType);
            // Guard against widthing a struct/union still owned by a
            // parameterized template interface (not yet specialized).
            // widthParamsEdit is destructive: it evaluates range expressions,
            // sets didWidth=1, and removes Range nodes, which would corrupt
            // the template's BASICDTYPEs for all subsequent clones.
            // Triggered by deeply nested parameterized interfaces (e.g.
            // outer_if containing inner_if with struct typedefs) when the
            // inner interface hasn't been specialized yet.
            तार्किक skipWidthForTemplateStruct = असत्य;
            {
                // Use non-asserting skip: before widthParamsEdit, type(expr)
                // constructs may contain unlinked REFDTYPEs (e.g. type(x-y))
                Ast_गाँठो_DType* अचर resolvedp = rawTypep ? rawTypep->skipRefOrNullp() : nullptr;
                यदि (resolvedp && (VN_IS(resolvedp, StructDType) || VN_IS(resolvedp, UnionDType))) {
                    Ast_गाँठो_Module* अचर ownerModp
                        = V3LinkDotIfaceCapture::findOwnerModule(resolvedp);
                    // Skip if owned by a parameterized interface (template or not
                    // yet cloned).  hasGParam() covers interfaces that haven't
                    // been cloned yet (parameterizedTemplate() not set).
                    यदि (ownerModp && VN_IS(ownerModp, Iface) && ownerModp->hasGParam()) {
                        skipWidthForTemplateStruct = सत्य;
                        V3Stats::addStatSum("Param, Template struct width skips", 1);
                        UINFO(9, "SKIP-WIDTH-TEMPLATE: struct="
                                     << resolvedp->prettyTypeName() << " templateOwner="
                                     << ownerModp->prettyNameQ() << " pin=" << pinp->prettyNameQ()
                                     << " of " << nodep->prettyNameQ()
                                     << " srcMod=" << srcModp->prettyNameQ());
                    }
                }
                यदि (rawTypep && !skipWidthForTemplateStruct) V3Width::widthParamsEdit(rawTypep);
            }
            Ast_गाँठो_DType* exprp = rawTypep ? rawTypep->skipRefToNonRefp() : nullptr;
            अचर Ast_गाँठो_DType* origp = modvarp->skipRefToNonRefp();
            यदि (!exprp) {
                pinp->v3error("Parameter type pin value isn't a type: Param "
                              << pinp->prettyNameQ() << " of " << nodep->prettyNameQ());
            } अन्यथा यदि (!origp) {
                pinp->v3error("Parameter type variable isn't a type: Param "
                              << modvarp->prettyNameQ());
            } अन्यथा {
                UINFO(9, "Parameter type assignment expr=" << exprp << " to " << origp);
                यदि (!skipWidthForTemplateStruct) {
                    V3Const::constifyParamsEdit(pinp->exprp());  // Reconcile typedefs
                    // Constify may have caused pinp->exprp to change
                    rawTypep = VN_AS(pinp->exprp(), गाँठो_DType);
                    exprp = rawTypep->skipRefToNonRefp();
                }
                यदि (!modvarp->fwdType().is_गाँठो_Compatible(exprp)) {
                    pinp->v3error("Parameter type expression type "
                                  << exprp->prettyDTypeNameQ()
                                  << " violates parameter's forwarding type '"
                                  << modvarp->fwdType().ascii() << "'");
                }
                यदि (exprp->similarDType(origp) || classTypeMatchesDefaultClone(exprp, origp)) {
                    // Setting parameter to its default value.  Just ignore it.
                    // This prevents making additional modules, and makes coverage more
                    // obvious as it won't show up under a unique module page name.
                } अन्यथा {
                    यदि (!skipWidthForTemplateStruct) {
                        VL_DO_DANGLING(V3Const::constifyParamsEdit(exprp), exprp);
                        rawTypep = VN_CAST(pinp->exprp(), गाँठो_DType);
                        exprp = rawTypep ? rawTypep->skipRefToNonRefp() : nullptr;
                    }
                    // Deparameterize ClassRefDType before name generation (#7000)
                    यदि (AstClassRefDType* अचर classRefDTypep = VN_CAST(exprp, ClassRefDType)) {
                        यदि (classRefDTypep->paramsp() && classRefDTypep->classp()
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

    शून्य cellInterfaceCleanup(AstPin* pinsp, Ast_गाँठो_Module* srcModp, string& longnamer,
                              तार्किक& any_overridesr, IfaceRefRefs& ifaceRefRefs) {
        लागि (AstPin* pinp = pinsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            अचर AstVar* अचर modvarp = pinp->modVarp();
            यदि (modvarp && VN_IS(arraySubDTypeDeepp(modvarp->subDTypep()), IfaceGenericDType)) {
                जारी;
            }
            यदि (modvarp->isIfaceRef()) {
                // arraySubDTypeDeepp returns input unchanged if not an array.
                AstIfaceRefDType* अचर portIrefp
                    = VN_CAST(arraySubDTypeDeepp(modvarp->subDTypep()), IfaceRefDType);
                अचर Ast_गाँठो* अचर exprp = pinp->exprp();
                यदि (VN_IS(exprp, CellArrayRef)) {
                    // CellArrayRef not yet linked; V3LinkDot resolves this pin later.
                    UINFO(9, "Skipping interface cleanup for CellArrayRef pin: " << pinp);
                    जारी;
                }
                AstIfaceRefDType* pinIrefp = nullptr;
                // Pin is a VarRef to a var of (possibly arrayed) iface type.
                यदि (अचर Ast_गाँठो_Var_Ref* अचर vrp = VN_CAST(exprp, गाँठो_Var_Ref)) {
                    यदि (vrp->varp()) {
                        pinIrefp
                            = VN_CAST(arraySubDTypeDeepp(vrp->varp()->subDTypep()), IfaceRefDType);
                    }
                }
                // Pin is a select, of any depth, of a VarRef (e.g. SelBit/ArraySel into an iface
                // array, or a slice of a row of a multi-dimensional one)
                यदि (!pinIrefp && exprp) {
                    अचर Ast_गाँठो* basep = exprp;
                    जबसम्म (अचर Ast_गाँठो_Pre_Sel* अचर selp = VN_CAST(basep, गाँठो_Pre_Sel)) {
                        basep = selp->fromp();
                    }
                    यदि (अचर AstVarRef* अचर vrp = VN_CAST(basep, VarRef)) {
                        यदि (vrp->varp()) {
                            pinIrefp = VN_CAST(arraySubDTypeDeepp(vrp->varp()->subDTypep()),
                                               IfaceRefDType);
                        }
                    }
                }

                UINFO(9, "     portIfaceRef " << portIrefp);

                यदि (!portIrefp) {
                    pinp->v3error("Interface port " << modvarp->prettyNameQ()
                                                    << " is not an interface " << modvarp);
                } अन्यथा यदि (!pinIrefp) {
                    pinp->v3error("Interface port "
                                  << modvarp->prettyNameQ()
                                  << " is not connected to interface/modport pin expression");
                } अन्यथा {
                    UINFO(9, "     pinIfaceRef " << pinIrefp);
                    यदि (portIrefp->ifaceViaCellp() != pinIrefp->ifaceViaCellp()) {
                        UINFO(9, "     IfaceRefDType needs reconnect  " << pinIrefp);
                        longnamer += ("_" + paramSmallName(srcModp, pinp->modVarp())
                                      + param_मान_सङ्ख्या(pinIrefp));
                        any_overridesr = सत्य;
                        ifaceRefRefs.emplace_back(portIrefp, pinIrefp);
                        यदि (portIrefp->ifacep() != pinIrefp->ifacep()
                            // Might be different only due to param cloning, so check names too
                            && portIrefp->ifaceName() != pinIrefp->ifaceName()) {
                            pinp->v3error("Port " << pinp->prettyNameQ() << " expects "
                                                  << Ast_गाँठो::prettyNameQ(portIrefp->ifaceName())
                                                  << " interface but pin connects "
                                                  << Ast_गाँठो::prettyNameQ(pinIrefp->ifaceName())
                                                  << " interface");
                        }
                    }
                }
            }
        }
    }

    शून्य storeOriginalParams(Ast_गाँठो_Module* अचर classp) {
        लागि (Ast_गाँठो* stmtp = classp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            Ast_गाँठो* originalParamp = nullptr;
            यदि (AstVar* अचर varp = VN_CAST(stmtp, Var)) {
                यदि (varp->isGParam()) originalParamp = varp->clonep();
            } अन्यथा यदि (AstParamTypeDType* अचर ptp = VN_CAST(stmtp, ParamTypeDType)) {
                यदि (ptp->isGParam()) originalParamp = ptp->clonep();
            }
            यदि (originalParamp) m_originalParams[stmtp] = originalParamp;
        }
    }

    // Set interfaces types inside generic modules
    // to the corresponding values of implicit parameters
    शून्य genericInterfaceVarSetup(अचर AstPin* अचर paramsp, अचर AstPin* अचर pinsp) {
        std::unordered_map<string, अचर AstPin*> paramspMap;
        लागि (अचर AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            यदि (VString::startsWith(pinp->name(), "__VGIfaceParam")) {
                paramspMap.घुसाउ({pinp->name().substr(std::म_माप("__VGIfaceParam")), pinp});
            }
        }

        यदि (paramspMap.empty()) फर्काऊ;

        लागि (अचर Ast_गाँठो* nodep = pinsp; nodep; nodep = nodep->nextp()) {
            यदि (अचर AstPin* अचर pinp = VN_CAST(nodep, Pin)) {
                यदि (AstVar* अचर varp = pinp->modVarp()) {
                    Ast_गाँठो_DType* dtypep = varp->childDTypep()->elemDTypep();
                    यदि (AstIfaceGenericDType* अचर ifaceGDTypep
                        = VN_CAST(dtypep, IfaceGenericDType)) {
                        अचर स्वतः iter = paramspMap.find(varp->name());
                        यदि (iter == paramspMap.end()) जारी;
                        Ast_गाँठो* अचर backp = ifaceGDTypep->backp();
                        ifaceGDTypep->unlinkFrBack();
                        अचर AstPin* अचर paramp = iter->second;
                        paramspMap.erase(iter);
                        अचर AstIfaceRefDType* अचर ifacerefp
                            = VN_AS(paramp->exprp(), IfaceRefDType);
                        AstIfaceRefDType* अचर newIfacerefp = नूतन AstIfaceRefDType{
                            ifaceGDTypep->fileline(), ifaceGDTypep->modportFileline(),
                            ifaceGDTypep->name(), ifacerefp->ifaceName(),
                            ifaceGDTypep->modportName()};
                        newIfacerefp->ifacep(ifacerefp->ifacep());
                        यदि (स्वतः* अचर arrDtp = VN_CAST(backp, गाँठो_Array_DType)) {
                            arrDtp->childDTypep(newIfacerefp);
                        } अन्यथा यदि (स्वतः* अचर arrDtp = VN_CAST(backp, BracketArrayDType)) {
                            arrDtp->childDTypep(newIfacerefp);
                        } अन्यथा {
                            varp->childDTypep(newIfacerefp);
                        }
                        VL_DO_DANGLING(m_deleter.थप_Deletep(ifaceGDTypep), ifaceGDTypep);
                        यदि (paramspMap.empty()) फर्काऊ;
                    }
                }
            }
        }

        UASSERT(paramspMap.empty(), "Not every generic interface implicit param is used");
    }

    शून्य resolveDefaultParams(Ast_गाँठो* nodep) {
        AstClassOrPackageRef* classOrPackageRef = VN_CAST(nodep, ClassOrPackageRef);
        AstClassRefDType* classRefDType = VN_CAST(nodep, ClassRefDType);
        AstClass* classp = nullptr;
        AstPin* paramsp = nullptr;

        यदि (classOrPackageRef) {
            classp = VN_CAST(classOrPackageRef->classOrPackageSkipp(), Class);
            यदि (!classp) फर्काऊ;  // No parameters in packages
            paramsp = classOrPackageRef->paramsp();
        } अन्यथा यदि (classRefDType) {
            classp = classRefDType->classp();
            paramsp = classRefDType->paramsp();
        } अन्यथा {
            nodep->v3fatalSrc("resolveDefaultParams called on node which is not a Class ref");
        }

        UASSERT_OBJ(classp, nodep, "Class or interface ref has no classp/ifacep");

        // Get the parameter list for this class
        m_classTypeParams.clear();
        m_paramIndex.clear();
        लागि (Ast_गाँठो* stmtp = classp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            यदि (AstParamTypeDType* paramTypep = VN_CAST(stmtp, ParamTypeDType)) {
                // Only consider formal class type parameters (generic parameters),
                // not localparam type declarations inside the class body.
                यदि (!paramTypep->isGParam()) जारी;
                m_paramIndex.emplace(paramTypep, static_cast<पूर्णाङ्क>(m_classTypeParams.size()));
                m_classTypeParams.emplace_back(paramTypep, -1);
            }
        }

        // For each parameter, detect either a dependent default (copy from previous param)
        // or a direct type default (for ex. int). Store dependency index in
        // m_classParams[i].second, default type in defaultTypeNodes[i].
        std::vector<Ast_गाँठो_DType*> defaultTypeNodes(m_classTypeParams.size(), nullptr);
        लागि (माप_प्रकार i = 0; i < m_classTypeParams.size(); ++i) {
            AstParamTypeDType* अचर paramTypep = m_classTypeParams[i].first;
            // Parser places defaults/constraints under childDTypep as AstRequireDType
            AstRequireDType* अचर reqDtp = VN_CAST(paramTypep->getChildDTypep(), RequireDType);
            यदि (!reqDtp) जारी;

            // If default is a reference to another param type, record dependency
            यदि (AstRefDType* अचर refDtp = VN_CAST(reqDtp->subDTypep(), RefDType)) {
                यदि (AstParamTypeDType* अचर sourceParamp
                    = VN_CAST(refDtp->refDTypep(), ParamTypeDType)) {
                    स्वतः it = m_paramIndex.find(sourceParamp);
                    यदि (it != m_paramIndex.end()) { m_classTypeParams[i].second = it->second; }
                    जारी;  // dependency handled
                }
            }

            // If default is a direct type (for ex. int)
            // also record dependency
            यदि (Ast_गाँठो_DType* अचर dtp = VN_CAST(reqDtp->lhsp(), गाँठो_DType)) {
                defaultTypeNodes[i] = dtp;
            }
        }

        // Count existing pins and capture them by index for easy lookup
        // Type parameters when named can be given in any order, but later we depend
        // on the order when handling defaults. So here we reorder them
        // for proper handling. The remaining ones are just pushed back
        std::vector<AstPin*> pinsByIndex;
        pinsByIndex.resize(m_classTypeParams.size(), nullptr);
        लागि (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            AstParamTypeDType* अचर typep = pinp->modPTypep();
            अचर स्वतः it = typep ? m_paramIndex.find(typep) : m_paramIndex.end();
            यदि (it != m_paramIndex.end()) {
                pinsByIndex[it->second] = pinp;
            } अन्यथा {
                pinsByIndex.थप_back(pinp);
            }
        }

        // For each missing parameter, get its pin from dependency or direct default.
        // A dependent default can reference a parameter declared later (not IEEE, but
        // we support it here), so repeat until no further parameter can be resolved.
        लागि (तार्किक resolvedSome = सत्य; resolvedSome;) {
            resolvedSome = असत्य;
            लागि (माप_प्रकार paramIdx = 0; paramIdx < m_classTypeParams.size(); paramIdx++) {
                यदि (pinsByIndex[paramIdx]) जारी;
                अचर पूर्णाङ्क sourceParamIdx = m_classTypeParams[paramIdx].second;

                AstPin* newPinp = nullptr;

                // Case 1: Dependent default -> clone the source pin's type, but only once
                // the source parameter itself has been resolved (it might be listed later)
                यदि (sourceParamIdx >= 0 && pinsByIndex[sourceParamIdx]) {
                    newPinp = pinsByIndex[sourceParamIdx]->clone_रूख(असत्य);
                }

                // Case 2: Direct default type (e.g., int), create a new pin with that dtype
                यदि (!newPinp && defaultTypeNodes[paramIdx]) {
                    Ast_गाँठो_DType* अचर dtypep = defaultTypeNodes[paramIdx];
                    newPinp = नूतन AstPin{dtypep->fileline(), static_cast<पूर्णाङ्क>(paramIdx) + 1,
                                         "__paramNumber" + cvtToStr(paramIdx + 1),
                                         dtypep->clone_रूख(असत्य)};
                }

                यदि (newPinp) {
                    newPinp->name("__paramNumber" + cvtToStr(paramIdx + 1));
                    newPinp->param(सत्य);
                    newPinp->modPTypep(m_classTypeParams[paramIdx].first);
                    यदि (classOrPackageRef) {
                        classOrPackageRef->addParamsp(newPinp);
                    } अन्यथा यदि (classRefDType) {
                        classRefDType->addParamsp(newPinp);
                    }
                    // Update local tracking so future dependent defaults can find it
                    pinsByIndex[paramIdx] = newPinp;
                    यदि (!paramsp) paramsp = newPinp;
                    resolvedSome = सत्य;
                }
            }
        }
    }

    Ast_गाँठो_Module* गाँठो_Deparam_Common(Ast_गाँठो* nodep, Ast_गाँठो_Module* srcModp, AstPin* paramsp,
                                     AstPin* pinsp, तार्किक any_overrides) {
        // Returns new or reused module
        // Make sure constification worked
        // Must be a separate loop, as constant conversion may have changed some pointers.
        string longname = srcModp->name() + "_";
        यदि (debug() >= 9 && paramsp) paramsp->dump_रूख_And_Next(cout, "-  cellparams: ");

        यदि (srcModp->hierBlock()) {
            longname = parameterizedHierBlockName(srcModp, paramsp);
            any_overrides = longname != srcModp->name();
        } अन्यथा {
            // Prepare every pin before naming any, as naming a value can depend on the others:
            // fold each value pin, as a class reference's may not be yet, and resolve each type
            std::unordered_set<अचर AstPin*> overridingTypePins;
            लागि (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                यदि (!pinp->exprp()) जारी;  // An empty override keeps the default
                यदि (pinp->modPTypep()) {
                    cellPinCleanup(nodep, pinp, paramsp, srcModp, longname /*ref*/,
                                   any_overrides /*ref*/, overridingTypePins /*ref*/);
                    जारी;
                }
                AstVar* अचर modvarp = pinp->modVarp();
                यदि (!modvarp || !modvarp->isGParam()) जारी;
                resolveDeferredDotsReachableFrom(modvarp->subDTypep(), m_modp);
                // An untyped pattern can't be folded without its parameter's type
                अचर AstPattern* अचर patternp = VN_CAST(pinp->exprp(), Pattern);
                यदि (patternp && !patternp->childDTypep()) जारी;
                यदि (!VN_IS(pinp->exprp(), Const) && !is_Aggregate_Param_मान(pinp->exprp())) {
                    // The pin, so a conditional of patterns is typed as an override's value
                    V3Const::constifyParamsEdit(pinp);
                }
            }
            लागि (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                यदि (AstParamTypeDType* अचर modptp = pinp->modPTypep()) {
                    यदि (!overridingTypePins.गन्ती(pinp)) जारी;
                    Ast_गाँठो_DType* अचर typep = VN_AS(pinp->exprp(), गाँठो_DType);
                    longname += "_" + paramSmallName(srcModp, modptp)
                                + param_मान_सङ्ख्या(typep->skipRefToNonRefp());
                    any_overrides = सत्य;
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
        तार्किक defaultsResolved = असत्य;
        यदि (!any_overrides && !VN_IS(srcModp, Iface) && srcModp->hasGParam()) {
            लागि (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                यदि (pinp->modPTypep()) {
                    any_overrides = सत्य;
                    defaultsResolved = सत्य;
                    रोक;
                }
            }
        }
        यदि (!any_overrides && VN_IS(nodep, Cell) && hasDescendantDefparams(srcModp)) {
            longname += "__Vdefparam" + V3Hash{srcModp->someInstanceName()}.toString();
            any_overrides = सत्य;
        }

        UINFO(9,
              "nodeDeparamCommon: " << srcModp->prettyNameQ() << " overrides=" << any_overrides);

        Ast_गाँठो_Module* newModp = nullptr;
        यदि (m_hierBlocks.hierSubRun() && m_hierBlocks.isHierBlock(srcModp->origName())) {
            Ast_गाँठो_Module* अचर paramedModp
                = m_hierBlocks.findByParams(srcModp->origName(), paramsp, m_modp);
            यदि (!paramedModp) फर्काऊ nullptr;  // Unsupported, already reported
            paramedModp->dead(असत्य);
            // We need to relink the pins to the new module
            relinkPinsByName(pinsp, paramedModp);
            newModp = paramedModp;
            // any_overrides = true;  // Unused later, so not needed
        } अन्यथा यदि (!any_overrides) {
            UINFO(9, "nodeDeparamCommon: no overrides, reusing " << srcModp);
            UINFO(8, "Cell parameters all match original values, skipping expansion.");
            // If it's the first use of the default instance, create a copy and store it in user3p.
            // user3p will also be used to check if the default instance is used.
            यदि (!srcModp->user3p() && (VN_IS(srcModp, Class) || VN_IS(srcModp, Iface))) {
                Ast_गाँठो_Module* गाँठो_Copyp = srcModp->clone_रूख(असत्य);
                // It is a temporary copy of the original class node, stored in order to create
                // another instances. It is needed only during class instantiation.
                UINFO(8, "    Created clone " << गाँठो_Copyp);
                m_deleter.थप_Deletep(गाँठो_Copyp);  // nodeCopyp used past here
                srcModp->user3p(गाँठो_Copyp);
                storeOriginalParams(गाँठो_Copyp);
            }
            newModp = srcModp;
        } अन्यथा {
            अचर string newname
                = srcModp->hierBlock() ? longname : moduleCalcName(srcModp, longname);
            अचर ModInfo* अचर modInfop
                = moduleFindOrClone(srcModp, nodep, paramsp, newname, ifaceRefRefs);
            यदि (!modInfop) फर्काऊ nullptr;
            // We need to relink the pins to the new module
            relinkPinsByName(pinsp, modInfop->m_modp);
            UINFO(8, "     Done with " << modInfop->m_modp);
            newModp = modInfop->m_modp;
        }

        अचर तार्किक cloned = (newModp != srcModp);
        UINFO(9, "nodeDeparamCommon result: " << newModp->prettyNameQ() << " cloned=" << cloned);

        // ParamVisitor skips the body of a class still marked hasGParam(), relying on it
        // being visited through a reference instead.  A class reached only by a deferred
        // class-scoped reference (e.g. the 'C#(V)::t' default of a type parameter that the
        // instantiation overrides) is never visited, so record specializations here.
        // user2() is set once processWorkQ has elaborated the body, which needs no re-queue.
        AstClass* अचर newClassp = VN_CAST(newModp, Class);
        यदि (newClassp && !newClassp->user2()) m_specializedClassps.थप_back(newClassp);

        // Link source class to its specialized version for later relinking of method references
        यदि (defaultsResolved) srcModp->user4p(newModp);

        लागि (स्वतः* stmtp = newModp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            यदि (AstParamTypeDType* dtypep = VN_CAST(stmtp, ParamTypeDType)) {
                यदि (VN_IS(dtypep->skipRefOrNullp(), VoidDType)) {
                    nodep->v3error(
                        "Class parameter type without default value is never given value"
                        << " (IEEE 1800-2023 6.20.1): " << dtypep->prettyNameQ());
                }
            }
            यदि (AstVar* अचर varp = VN_CAST(stmtp, Var)) {
                यदि (VN_IS(newModp, Class) && varp->isParam() && !varp->valuep()) {
                    nodep->v3error("Class parameter without default value is never given value"
                                   << " (IEEE 1800-2023 6.20.1): " << varp->prettyNameQ());
                }
            }
        }

        genericInterfaceVarSetup(paramsp, pinsp);

        // Delete the parameters from the cell; they're not relevant any longer.
        यदि (paramsp) VL_DO_DANGLING(paramsp->unlinkFrBackWithNext()->delete_रूख(), paramsp);
        फर्काऊ newModp;
    }

    Ast_गाँठो_Module* cellDeparam(AstCell* nodep, Ast_गाँठो_Module* srcModp) {
        // Must always clone __Vrcm (recursive modules)
        Ast_गाँठो_Module* अचर newModp = गाँठो_Deparam_Common(nodep, srcModp, nodep->paramsp(),
                                                         nodep->pinsp(), nodep->recursive());
        यदि (!newModp) फर्काऊ nullptr;
        nodep->modp(newModp);  // Might be unchanged if not cloned (newModp == srcModp)
        nodep->modName(newModp->name());
        nodep->recursive(असत्य);
        फर्काऊ newModp;
    }
    Ast_गाँठो_Module* ifaceRefDeparam(AstIfaceRefDType* अचर nodep, Ast_गाँठो_Module* srcModp) {
        // Check for self-reference pattern: typedef iface#(T) this_type inside interface iface
        // When processing inside a specialized interface, the IfaceRefDType should point to
        // the owner interface, not create an intermediate specialization.
        यदि (m_modp && VN_IS(m_modp, Iface)) {
            AstIface* ownerIfacep = const_cast<AstIface*>(VN_AS(m_modp, Iface));
            अचर string ownerOrigName
                = ownerIfacep->origName().empty() ? ownerIfacep->name() : ownerIfacep->origName();
            अचर string srcOrigName
                = srcModp->origName().empty() ? srcModp->name() : srcModp->origName();
            string ownerBaseName = ownerOrigName;
            अचर माप_प्रकार ownerPos = ownerBaseName.find("__");
            यदि (ownerPos != string::npos) ownerBaseName = ownerBaseName.substr(0, ownerPos);
            string srcBaseName = srcOrigName;
            अचर माप_प्रकार srcPos = srcBaseName.find("__");
            यदि (srcPos != string::npos) srcBaseName = srcBaseName.substr(0, srcPos);

            यदि (ownerBaseName == srcBaseName) {
                तार्किक allOwnParams = सत्य;
                लागि (AstPin* pinp = nodep->paramsp(); pinp && allOwnParams;
                     pinp = VN_AS(pinp->nextp(), Pin)) {
                    यदि (AstRefDType* अचर refp = VN_CAST(pinp->exprp(), RefDType)) {
                        यदि (AstParamTypeDType* अचर ptdp
                            = VN_CAST(refp->refDTypep(), ParamTypeDType)) {
                            Ast_गाँठो_Module* अचर ptdOwnerp
                                = V3LinkDotIfaceCapture::findOwnerModule(ptdp);
                            यदि (ptdOwnerp != m_modp) allOwnParams = असत्य;
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
                यदि (allOwnParams) {
                    UINFO(5, "ifaceRefDeparam: self-reference pattern detected in "
                                 << ownerIfacep->prettyNameQ() << ", using owner interface");
                    V3Stats::addStatSum("Param, Self-reference iface typedefs", 1);
                    nodep->ifacep(ownerIfacep);
                    यदि (nodep->paramsp()) nodep->paramsp()->unlinkFrBackWithNext()->delete_रूख();
                    फर्काऊ ownerIfacep;
                }
            }
        }

        Ast_गाँठो_Module* अचर newModp
            = गाँठो_Deparam_Common(nodep, srcModp, nodep->paramsp(), nullptr, असत्य);
        यदि (!newModp) फर्काऊ nullptr;
        nodep->ifacep(VN_AS(newModp, Iface));
        फर्काऊ newModp;
    }
    Ast_गाँठो_Module* classRefDeparam(AstClassOrPackageRef* nodep, Ast_गाँठो_Module* srcModp) {
        resolveDefaultParams(nodep);
        Ast_गाँठो_Module* अचर newModp
            = गाँठो_Deparam_Common(nodep, srcModp, nodep->paramsp(), nullptr, असत्य);
        यदि (!newModp) फर्काऊ nullptr;
        nodep->classOrPackagep(newModp);  // Might be unchanged if not cloned (newModp == srcModp)

        // If this ClassOrPackageRef is a child of a RefDType (e.g., typedef class#(T)::member_t),
        // resolve the RefDType's typedef to point to the typedef inside the specialized class
        AstRefDType* अचर refDTypep = VN_CAST(nodep->backp(), RefDType);
        AstClass* अचर newClassp = refDTypep ? VN_CAST(newModp, Class) : nullptr;
        यदि (newClassp && !refDTypep->typedefp() && !refDTypep->subDTypep()) {
            यदि (AstTypedef* अचर typedefp
                = VN_CAST(m_memberMap.findMember(newClassp, refDTypep->name()), Typedef)) {
                refDTypep->typedefp(typedefp);
                refDTypep->classOrPackagep(newClassp);
                UINFO(9, "Resolved parameterized class typedef: " << refDTypep->name() << " -> "
                                                                  << typedefp << " in "
                                                                  << newClassp->name());
            }
        }
        फर्काऊ newModp;
    }
    Ast_गाँठो_Module* classRefDeparam(AstClassRefDType* nodep, Ast_गाँठो_Module* srcModp) {
        resolveDefaultParams(nodep);
        Ast_गाँठो_Module* अचर newModp
            = गाँठो_Deparam_Common(nodep, srcModp, nodep->paramsp(), nullptr, असत्य);
        यदि (!newModp) फर्काऊ nullptr;
        AstClass* अचर newClassp = VN_AS(newModp, Class);
        nodep->classp(newClassp);  // Might be unchanged if not cloned (newModp == srcModp)
        nodep->classOrPackagep(newClassp);
        फर्काऊ newClassp;
    }

    // Resolve deferred class-scoped references in post-order. Resolution may
    // deparameterize a class and delete its parameter pins, so no pointer to a
    // child may remain pending when its parent is resolved.
    वर्ग DeferredResolverVisitor final : सार्वजनिक VNVisitor {
        ParamProcessor& m_processor;  // Processor used to resolve deferred references
        std::set<अचर Ast_गाँठो*> m_reachedDecls;

        तार्किक firstReach(अचर Ast_गाँठो* अचर declp) { फर्काऊ m_reachedDecls.घुसाउ(declp).second; }

        शून्य iterateParamType(AstParamTypeDType* अचर paramTypep) {
            यदि (firstReach(paramTypep)) iterate(paramTypep);
        }
        शून्य iterateTypedef(AstTypedef* अचर tdefp) {
            यदि (firstReach(tdefp)) iterate(tdefp->subDTypep());
        }

        शून्य visit(Ast_गाँठो* nodep) override { iterateChildren(nodep); }
        शून्य visit(AstClassOrPackageRef* nodep) override {
            iterateChildren(nodep);
            Ast_गाँठो* अचर targetp = nodep->classOrPackageNodep();
            यदि (AstTypedef* अचर tdefp = VN_CAST(targetp, Typedef)) {
                iterateTypedef(tdefp);
            } अन्यथा यदि (AstParamTypeDType* अचर paramTypep = VN_CAST(targetp, ParamTypeDType)) {
                iterateParamType(paramTypep);
            }
        }
        शून्य visit(AstDot* nodep) override {
            iterateChildren(nodep);
            m_processor.resolveDotToTypedef(nodep);
        }
        शून्य visit(AstRefDType* nodep) override {
            iterateChildren(nodep);
            AstTypedef* अचर tdefp = nodep->typedefp();
            यदि (tdefp) {
                iterateTypedef(tdefp);
            } अन्यथा यदि (AstParamTypeDType* अचर paramTypep
                       = VN_CAST(nodep->refDTypep(), ParamTypeDType)) {
                iterateParamType(paramTypep);
            }
            m_processor.resolveParamClassRefDType(nodep);
        }
        शून्य visit(AstVarRef* nodep) override {
            iterateChildren(nodep);
            AstVar* अचर varp = nodep->varp();
            यदि (firstReach(varp)) {
                iterate(varp->subDTypep());
                अचर स्वतः& deferredVarps = v3Global.rootp()->deferredParamVarps();
                यदि (varp->varType() == VVarType::LPARAM && deferredVarps.गन्ती(varp)) {
                    UASSERT_OBJ(varp->valuep(), varp, "VarRef should have non-null valuep");
                    iterate(varp->valuep());
                }
            }
        }

    सार्वजनिक:
        DeferredResolverVisitor(ParamProcessor& processor, Ast_गाँठो* rootp)
            : m_processor{processor} {
            iterate(rootp);
        }
    };

सार्वजनिक:
    // After an interface cell inside parentModp has been deparameterized
    // (rewired from template to clone), retarget REFDTYPEs that still
    // reference the old template's types so that $bits(iface_typedef)
    // evaluates with the clone's widths.
    शून्य retargetIfaceRefs(Ast_गाँठो_Module* parentModp, अचर string& cellName) {
        Ast_गाँठो_Module* अचर correctModp
            = V3LinkDotIfaceCapture::followCellPath(parentModp, cellName);
        यदि (!correctModp || correctModp->dead() || correctModp->parameterizedTemplate()) फर्काऊ;
        parentModp->foreach([&](AstRefDType* refp) {
            अचर VIfaceCaptureTag* अचर tagp = V3LinkDotIfaceCapture::captureTag(refp);
            यदि (!tagp || tagp->m_cellPath != cellName) फर्काऊ;
            // Only retarget REFDTYPEs owned by parentModp.
            यदि (V3LinkDotIfaceCapture::findOwnerModule(refp) != parentModp) फर्काऊ;
            यदि (V3LinkDotIfaceCapture::retargetRefToModule(refp, correctModp)) {
                UINFO(9, "retargetIfaceRefs: " << refp << " -> " << correctModp->prettyNameQ());
            }
        });
    }

    // Resolve deferred class-scoped values and types reachable from rootp. In
    // addition to AST children, follow typedefs, parameter types, deferred
    // localparam values, and referenced-variable dtypes. This makes the walk
    // independent of the particular dtype wrappers around a RefDType.
    शून्य resolveDeferredDotsReachableFrom(Ast_गाँठो* rootp, अचर Ast_गाँठो_Module* modp) {
        VL_RESTORER(m_modp);
        m_modp = modp;
        DeferredResolverVisitor{*एतत्, rootp};
    }

    Ast_गाँठो_Module* गाँठो_Deparam(Ast_गाँठो* nodep, Ast_गाँठो_Module* srcModp, Ast_गाँठो_Module* modp,
                               अचर string& someInstanceName) {
        // Return new or reused de-parameterized module
        m_modp = modp;
        // Cell: Check for parameters in the instantiation.
        // We always run this, even if no parameters, as need to look for interfaces,
        // and remove any recursive references
        UINFO(4, "De-parameterize: " << nodep);
        UINFO(9, "nodeDeparam ENTER node=<"
                     << Ast_गाँठो::गाँठो_Addr(nodep) << ">" << " type=" << nodep->typeName()
                     << " srcMod=" << (srcModp ? srcModp->prettyNameQ() : "'<null>'")
                     << " srcSomeInstanceName='"
                     << (srcModp ? srcModp->someInstanceName() : string("<null>")) << "'"
                     << " parentMod=" << (modp ? modp->prettyNameQ() : "'<null>'")
                     << " parentSomeInstanceName='"
                     << (modp ? modp->someInstanceName() : string("<null>")) << "'"
                     << " inputSomeInstanceName='" << someInstanceName << "'");
        string गाँठो_Name = nodep->name();
        यदि (AstIfaceRefDType* अचर ifaceRefp = VN_CAST(nodep, IfaceRefDType)) {
            यदि (गाँठो_Name.empty()) गाँठो_Name = ifaceRefp->cellName();
        }
        अचर string instanceName
            = गाँठो_Name.empty() ? someInstanceName : (someInstanceName + "." + गाँठो_Name);

        यदि (AstCell* अचर cellp = VN_CAST(nodep, Cell)) {
            लागि (AstPin* pinp = cellp->paramsp(); pinp;) {
                AstPin* अचर nextp = VN_AS(pinp->nextp(), Pin);
                यदि (!pinp->paramPath().empty() && instanceName != pinp->paramPath()
                    && !VString::endsWith(instanceName, "." + pinp->paramPath())) {
                    VL_DO_DANGLING(pinp->unlinkFrBack()->delete_रूख(), pinp);
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
                     << srcModp->someInstanceName() << "'" << " node=<" << Ast_गाँठो::गाँठो_Addr(nodep)
                     << ">" << " nodeType=" << nodep->typeName());

        Ast_गाँठो_Module* newModp = nullptr;
        यदि (AstCell* अचर cellp = VN_CAST(nodep, Cell)) {
            newModp = cellDeparam(cellp, srcModp);
        } अन्यथा यदि (AstIfaceRefDType* अचर ifaceRefDTypep = VN_CAST(nodep, IfaceRefDType)) {
            newModp = ifaceRefDeparam(ifaceRefDTypep, srcModp);
        } अन्यथा यदि (AstClassRefDType* अचर classRefp = VN_CAST(nodep, ClassRefDType)) {
            newModp = classRefDeparam(classRefp, srcModp);
        } अन्यथा यदि (AstClassOrPackageRef* अचर classRefp = VN_CAST(nodep, ClassOrPackageRef)) {
            newModp = classRefDeparam(classRefp, srcModp);
        } अन्यथा {
            nodep->v3fatalSrc("Expected module parameterization");
        }
        // 'newModp' might be nullptr on error
        यदि (!newModp) फर्काऊ nullptr;

        // Set name for later warnings
        newModp->someInstanceName(instanceName);

        UINFO(9, "nodeDeparam EXIT  node=<"
                     << Ast_गाँठो::गाँठो_Addr(nodep) << ">" << " type=" << nodep->typeName()
                     << " srcMod=" << (srcModp ? srcModp->prettyNameQ() : "'<null>'")
                     << " srcSomeInstanceName='"
                     << (srcModp ? srcModp->someInstanceName() : string("<null>")) << "'"
                     << " newMod=" << (newModp ? newModp->prettyNameQ() : "'<null>'")
                     << " newSomeInstanceName='"
                     << (newModp ? newModp->someInstanceName() : string("<null>")) << "'");

        UINFO(8, "     Done with orig " << nodep);
        // if (debug() >= 10)
        // v3Global.rootp()->dumpTreeFile(v3Global.debugFilename("param-out.tree"));
        फर्काऊ newModp;
    }

    // Return, and forget, the classes specialized since the previous call
    std::vector<AstClass*> takeSpecializedClassps() {
        std::vector<AstClass*> taken;
        taken.swap(m_specializedClassps);
        फर्काऊ taken;
    }

    // CONSTRUCTORS
    explicit ParamProcessor(AstNetlist* nodep)
        : m_hierBlocks{v3Global.opt.hierBlocks(), nodep} {
        लागि (Ast_गाँठो_Module* modp = nodep->modulesp(); modp;
             modp = VN_AS(modp->nextp(), गाँठो_Module)) {
            m_allModuleNames.घुसाउ(modp->name());
        }
    }
    ~ParamProcessor() = स्वभाव;
    VL_UNCOPYABLE(ParamProcessor);
};

//######################################################################
// Record classes that are referenced with parameter pins

वर्ग ClassRefUnlinkerVisitor final : सार्वजनिक VNVisitor {

सार्वजनिक:
    explicit ClassRefUnlinkerVisitor(AstNetlist* netlistp) { iterate(netlistp); }
    शून्य visit(AstClassOrPackageRef* nodep) override {
        यदि (nodep->paramsp()) {
            यदि (AstClass* अचर classp = VN_CAST(nodep->classOrPackageSkipp(), Class)) {
                यदि (!classp->user3p()) {
                    // Check if this ClassOrPackageRef is the lhsp of a DOT node
                    AstDot* अचर dotp = VN_CAST(nodep->backp(), Dot);
                    यदि (dotp && dotp->lhsp() == nodep) {
                        // Replace DOT with just its rhsp to avoid leaving DOT with null lhsp
                        dotp->replaceWith(dotp->rhsp()->unlinkFrBack());
                        VL_DO_DANGLING2(थप_Deletep(dotp), dotp, nodep);
                    } अन्यथा {
                        VL_DO_DANGLING(थप_Deletep(nodep->unlinkFrBack()), nodep);
                    }
                }
            }
        }
    }
    शून्य visit(AstClass* nodep) override {}  // don't iterate inside classes
    शून्य visit(Ast_गाँठो* nodep) override { iterateChildren(nodep); }
};

//######################################################################
//  Relink RefDType nodes to point to parameterized classes' type parameters

वर्ग ParamClassRefDTypeRelinkVisitor final : सार्वजनिक VNVisitor {

    AstClass* m_classp = nullptr;
    std::unordered_map<अचर AstParamTypeDType*, AstClass*> m_paramTypeClassMap;

    // Owner-tracking state for the post-clone REFDTYPE typedef retargeting
    // (see retargetRefDType()). m_ownerModp is the most recently entered
    // AstNodeModule (or AstClass). m_origNameToClone / m_ambiguous are
    // rebuilt lazily on the first AstRefDType visit per owner; the
    // m_ownerMapForp cache key avoids rebuilding when the owner is unchanged
    // (e.g. across siblings under the same parent).
    Ast_गाँठो_Module* m_ownerModp = nullptr;
    Ast_गाँठो_Module* m_ownerMapForp = nullptr;
    std::unordered_map<std::string, AstClass*> m_origNameToClone;
    std::unordered_set<std::string> m_ambiguous;

    शून्य ensureOwnerMap() {
        यदि (m_ownerMapForp == m_ownerModp) फर्काऊ;
        m_ownerMapForp = m_ownerModp;
        m_origNameToClone.clear();
        m_ambiguous.clear();
        यदि (!m_ownerModp) फर्काऊ;
        // Skip bare/generic class templates: their bodies are not
        // authoritative for any specialization, and their self-typedefs
        // (e.g. `typedef cls#(P,Q) this_type;` inside a parameterized class)
        // would otherwise be treated as ground truth and cause REFDTYPEs in
        // the body to be retargeted at the bare template.
        यदि (AstClass* अचर ownerClassp = VN_CAST(m_ownerModp, Class)) {
            यदि (ownerClassp->hasGParam()) फर्काऊ;
        }
        लागि (Ast_गाँठो* sp = m_ownerModp->stmtsp(); sp; sp = sp->nextp()) {
            AstTypedef* अचर tdp = VN_CAST(sp, Typedef);
            यदि (!tdp) जारी;
            AstClassRefDType* अचर crdtp = VN_CAST(tdp->subDTypep(), ClassRefDType);
            यदि (!crdtp || !crdtp->classp()) जारी;
            AstClass* अचर targetp = crdtp->classp();
            // A typedef pointing at a still-generic template is a forward
            // declaration / unresolved reference, not ground truth.
            यदि (targetp->hasGParam()) जारी;
            अचर std::string origName
                = targetp->origName().empty() ? targetp->name() : targetp->origName();
            अचर स्वतः pair = m_origNameToClone.emplace(origName, targetp);
            // Two locally-resolved typedefs pointing at different
            // specializations of the same template -> ambiguous; we cannot
            // safely guess which one a stripped REFDTYPE meant.
            यदि (!pair.second && pair.first->second != targetp) m_ambiguous.घुसाउ(origName);
        }
    }

    // A class's name before specialization (e.g. "holder" for "holder__Tz1").
    स्थिर string classOrigName(अचर AstClass* classp) {
        फर्काऊ classp->origName().empty() ? classp->name() : classp->origName();
    }

    // Find 'name' in classp or any base class (findTypedefInModule() searches
    // only the class itself).
    स्थिर AstTypedef* findTypedefWithBases(AstClass* classp, अचर string& name) {
        लागि (AstClass* cp = classp; cp; cp = cp->extendsp() ? cp->extendsp()->classp() : nullptr) {
            यदि (AstTypedef* अचर tdp = V3LinkDotIfaceCapture::findTypedefInModule(cp, name)) {
                यदि (tdp->subDTypep()) फर्काऊ tdp;
            }
        }
        फर्काऊ nullptr;
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
        यदि (!m_ownerModp) फर्काऊ;

        // IEEE 1800-2023 8.25.1: a bare class-qualified reference to the class
        // being compiled means the current specialization, but V3LinkDot binds
        // it to the default instance, so a typedef reached through it widens
        // with the template's defaults (#8348). Rebind it to the
        // specialization's own typedef.
        यदि (AstClass* अचर ownerClassp = VN_CAST(m_ownerModp, Class)) {
            यदि (!ownerClassp->hasGParam() && !refp->paramsp()) {
                // What the reference resolves to: its typedef's owner if
                // linked, else the class it is qualified by.
                अचर AstClass* refClassp = nullptr;
                यदि (AstTypedef* अचर tdp = refp->typedefp()) {
                    refClassp = VN_CAST(V3LinkDotIfaceCapture::findOwnerModule(tdp), Class);
                } अन्यथा यदि (अचर AstClassOrPackageRef* अचर classRefp
                           = VN_CAST(refp->classOrPackageOpp(), ClassOrPackageRef)) {
                    refClassp = VN_CAST(classRefp->classOrPackageSkipp(), Class);
                }
                // Same originating class => bare self reference (another
                // specialization would carry #(), rejected above).
                यदि (refClassp && refClassp != ownerClassp
                    && classOrigName(refClassp) == classOrigName(ownerClassp)) {
                    AstTypedef* अचर selfTdp = findTypedefWithBases(ownerClassp, refp->name());
                    यदि (selfTdp) {
                        UINFO(9, "post-param REFDTYPE self-reference retarget: "
                                     << refp << " from " << refClassp->name() << " to "
                                     << ownerClassp->name());
                        refp->typedefp(selfTdp);
                        refp->classOrPackagep(V3LinkDotIfaceCapture::findOwnerModule(selfTdp));
                        refp->refDTypep(selfTdp->subDTypep());
                        फर्काऊ;
                    }
                }
            }
        }

        AstTypedef* अचर oldTdp = refp->typedefp();
        यदि (!oldTdp) फर्काऊ;
        AstClass* अचर oldOwnerp = VN_CAST(V3LinkDotIfaceCapture::findOwnerModule(oldTdp), Class);
        यदि (!oldOwnerp) फर्काऊ;

        ensureOwnerMap();
        यदि (m_origNameToClone.empty()) फर्काऊ;
        अचर std::string origName
            = oldOwnerp->origName().empty() ? oldOwnerp->name() : oldOwnerp->origName();
        यदि (m_ambiguous.गन्ती(origName)) {
            // The owner has two locally-resolved typedefs pointing at
            // different specializations of the same template, e.g.:
            //   typedef driver #(.IW(5)) drv5_t;
            //   typedef driver #(.IW(6)) drv6_t;
            // A stripped REFDTYPE only carries the origName, so we cannot
            // tell which sibling it meant; leave it alone.
            UINFO(9, "post-param REFDTYPE retarget skipped (ambiguous owner '" << origName
                                                                               << "'): " << refp);
            फर्काऊ;
        }
        अचर स्वतः it = m_origNameToClone.find(origName);
        यदि (it == m_origNameToClone.end()) फर्काऊ;
        AstClass* अचर correctClassp = it->second;
        यदि (correctClassp == oldOwnerp) फर्काऊ;
        AstTypedef* अचर newTdp
            = V3LinkDotIfaceCapture::findTypedefInModule(correctClassp, refp->name());
        यदि (!newTdp) {
            // The owner map says correctClassp is the right specialization,
            // but it does not declare a typedef named refp->name(). Caveats
            // of findTypedefInModule (e.g. typedef declared in a base class,
            // not the specialization we resolved to) can land us here.
            UINFO(9, "post-param REFDTYPE retarget skipped (no typedef '"
                         << refp->name() << "' in " << correctClassp->name() << "): " << refp);
            फर्काऊ;
        }
        // typedefp / classOrPackagep / refDTypep must agree. If newTdp lacks
        // a subDTypep we cannot keep all three in sync, so skip the entire
        // retarget rather than leaving the REFDTYPE half-updated.
        यदि (!newTdp->subDTypep()) {
            UINFO(9, "post-param REFDTYPE retarget skipped (newTdp has null subDTypep): " << refp);
            फर्काऊ;
        }
        refp->typedefp(newTdp);
        refp->classOrPackagep(correctClassp);
        refp->refDTypep(newTdp->subDTypep());
        UINFO(9, "post-param REFDTYPE re-resolve: " << refp << " from " << oldOwnerp->name()
                                                    << " to " << correctClassp->name());
    }

सार्वजनिक:
    explicit ParamClassRefDTypeRelinkVisitor(AstNetlist* netlistp) { iterate(netlistp); }
    शून्य visit(Ast_गाँठो_Module* nodep) override {
        VL_RESTORER(m_ownerModp);
        m_ownerModp = nodep;
        iterateChildren(nodep);
    }
    शून्य visit(AstClass* nodep) override {
        VL_RESTORER(m_classp);
        VL_RESTORER(m_ownerModp);
        m_classp = nodep;
        m_ownerModp = nodep;
        लागि (Ast_गाँठो* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            यदि (AstParamTypeDType* अचर ptp = VN_CAST(stmtp, ParamTypeDType)) {
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
        यदि (AstParamTypeDType* अचर paramtypep = VN_CAST(nodep->refDTypep(), ParamTypeDType)) {
            अचर स्वतः it = m_paramTypeClassMap.find(paramtypep);
            यदि (it != m_paramTypeClassMap.end()) {
                AstClass* अचर origClassp = it->second;
                यदि (origClassp->hasGParam() && !origClassp->user3p()) {
                    यदि (AstClass* अचर parametrizedClassp
                        = VN_CAST(origClassp->user4p(), Class)) {
                        अचर string paramName = paramtypep->name();
                        AstParamTypeDType* newParamTypep = nullptr;
                        लागि (Ast_गाँठो* stmtp = parametrizedClassp->stmtsp(); stmtp;
                             stmtp = stmtp->nextp()) {
                            यदि (AstParamTypeDType* अचर ptp = VN_CAST(stmtp, ParamTypeDType)) {
                                यदि (ptp->name() == paramName) {
                                    newParamTypep = ptp;
                                    रोक;
                                }
                            }
                        }
                        यदि (newParamTypep) {
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
    शून्य visit(Ast_गाँठो* nodep) override { iterateChildren(nodep); }
};

//######################################################################
// Process parameter top state

वर्ग ParamState final {
सार्वजनिक:
    // STATE - across all visitors
    std::vector<AstClass*> m_paramClasses;  // Parameterized classes
    std::vector<AstDot*> m_dots;  // Dot references to process
    // Work queue keyed by (!isIface, level) so interfaces are always processed
    // before non-interfaces.  This ensures interface clones have their types
    // properly widthed before any module that references those types.
    using WQKey = std::pair<तार्किक, पूर्णाङ्क>;
    std::multimap<WQKey, Ast_गाँठो_Module*> m_workQueueNext;  // Modules left to process
    // Map from AstNodeModule to set of all AstNodeModules that instantiates it.
    std::unordered_map<Ast_गाँठो_Module*, std::unordered_set<Ast_गाँठो_Module*>> m_parentps;
};

//######################################################################
// Process parameter visitor

वर्ग ParamVisitor final : सार्वजनिक VNVisitor {
    // NODE STATE
    // AstNodeModule::user1 -> bool: already fixed level (temporary)
    // AstNodeDType::user2  -> bool: already visited from typedef

    // STATE - across all visitors
    ParamState& m_state;  // Common state
    ParamProcessor m_processor;  // De-parameterize a cell, build modules
    GenForUnroller m_unroller;  // Unroller for AstGenFor

    तार्किक m_iterateModule = असत्य;  // Iterating module body
    string m_unlinkedTxt;  // Text for AstUnlinkedRef
    std::multimap<तार्किक, Ast_गाँठो*> m_cellps;  // Cells left to process (in current module)
    std::unordered_map<अचर Ast_गाँठो*, std::string>
        m_genHierNames;  // Maps ast nodes to generated hierarchy names
    std::map<अचर AstRefDType*, तार्किक>
        m_isCircular;  // Stores information whether `AstRefDType` is circular
    using VarsByName = std::unordered_map<std::string, AstVar*>;
    // Persists across modules; one specialized interface clone serves every module bound to it
    std::unordered_map<अचर Ast_गाँठो_Module*, VarsByName> m_ifaceParams;

    // STATE - for current visit position (use VL_RESTORER)
    Ast_गाँठो_Module* m_modp = nullptr;  // Module iterating
    std::unordered_set<std::string> m_ifacePortNames;  // Interface port names in current module
    std::unordered_map<std::string, AstCell*>
        m_ifaceInstCells;  // Local interface instance cells in current module, keyed by name
    VarsByName m_modIfaceRefs;  // Interface-ref Vars in current module, keyed by name
    तार्किक m_modIfaceRefsDone = असत्य;  // m_modIfaceRefs has been gathered for m_modp
    string m_generateHierName;  // Generate portion of hierarchy name
    तार्किक m_inGenerateCond = असत्य;  // Traversing a generate condition; see iterateGenerateCond

    // METHODS

    शून्य processWorkQ() {
        UASSERT(!m_iterateModule, "Should not nest");
        std::multimap<ParamState::WQKey, Ast_गाँठो_Module*> workQueue;
        m_generateHierName = "";
        m_iterateModule = सत्य;

        // Visit all cells under module, recursively
        जबसम्म (सत्य) {
            // Classes specialized since the last pass still need their bodies elaborated
            लागि (AstClass* अचर classp : m_processor.takeSpecializedClassps()) {
                m_state.m_workQueueNext.emplace(ParamState::WQKey{सत्य, classp->level()}, classp);
            }
            यदि (workQueue.empty()) std::swap(workQueue, m_state.m_workQueueNext);
            यदि (workQueue.empty()) रोक;

            अचर स्वतः itm = workQueue.cbegin();
            Ast_गाँठो_Module* अचर modp = itm->second;
            workQueue.erase(itm);
            // Starting a new module, so what was learned about the last one no longer holds.
            v3Global.rootp()->clearContainingModules();

            // Process once; note user2 will be cleared on specialization, so we will do the
            // specialized module if needed
            यदि (!modp->user2SetOnce()) {
                UINFO(9, "processWorkQ module begin mod='"
                             << modp->name() << "' orig='" << modp->origName() << "'"
                             << " someInstanceName='" << modp->someInstanceName() << "'"
                             << " hasGParam=" << (modp->hasGParam() ? "yes" : "no")
                             << " user3p=" << (modp->user3p() ? "set" : "null")
                             << " dead=" << (modp->dead() ? "yes" : "no"));

                // TODO: this really should be an assert, but classes and hier_blocks are
                // special...
                यदि (modp->someInstanceName().empty()) modp->someInstanceName(modp->origName());

                // Iterate the body
                {
                    VL_RESTORER(m_modp);
                    VL_RESTORER_CLEAR(m_ifacePortNames);
                    VL_RESTORER_CLEAR(m_ifaceInstCells);
                    VL_RESTORER_CLEAR(m_modIfaceRefs);
                    VL_RESTORER(m_modIfaceRefsDone);
                    m_modp = modp;
                    m_modIfaceRefsDone = असत्य;
                    iterateChildren(modp);
                }
            }

            // Process interface cells, then non-interface cells, which may reference an interface
            // cell.
            जबसम्म (!m_cellps.empty()) {
                अचर स्वतः itim = m_cellps.cbegin();
                Ast_गाँठो* अचर cellp = itim->second;
                m_cellps.erase(itim);

                // Consume the generated hierarchy name for the node
                string genHierName;
                अचर स्वतः genHierIt = m_genHierNames.find(cellp);
                यदि (genHierIt != m_genHierNames.end()) {
                    genHierName = std::move(genHierIt->second);
                    m_genHierNames.erase(genHierIt);
                }

                Ast_गाँठो_Module* srcModp = nullptr;
                यदि (अचर AstCell* modCellp = VN_CAST(cellp, Cell)) {
                    srcModp = modCellp->modp();
                } अन्यथा यदि (अचर AstClassOrPackageRef* classRefp
                           = VN_CAST(cellp, ClassOrPackageRef)) {
                    srcModp = classRefp->classOrPackageSkipp();
                    यदि (VN_IS(classRefp->classOrPackageNodep(), ParamTypeDType)) जारी;
                } अन्यथा यदि (अचर AstClassRefDType* classRefp = VN_CAST(cellp, ClassRefDType)) {
                    srcModp = classRefp->classp();
                } अन्यथा यदि (अचर AstIfaceRefDType* ifaceRefp = VN_CAST(cellp, IfaceRefDType)) {
                    srcModp = ifaceRefp->ifacep();
                } अन्यथा {
                    cellp->v3fatalSrc("Expected module parameterization");
                }
                यदि (!srcModp) जारी;

                // Update path
                अचर string someInstanceName = modp->someInstanceName() + genHierName;

                // Apply parameter specialization
                यदि (Ast_गाँठो_Module* अचर newModp
                    = m_processor.गाँठो_Deparam(cellp, srcModp, modp, someInstanceName)) {

                    यदि (VN_IS(srcModp, Iface)) {
                        logTemplateLeakRefs(modp, srcModp, "after queued nodeDeparam", cellp);
                        // After the interface cell is rewired to its clone,
                        // retarget REFDTYPEs in the parent module that still
                        // reference the template interface's types.
                        यदि (V3LinkDotIfaceCapture::enabled()) {
                            यदि (अचर AstCell* अचर modCellp = VN_CAST(cellp, Cell)) {
                                यदि (newModp != srcModp) {
                                    m_processor.retargetIfaceRefs(modp, modCellp->name());
                                }
                            }
                        }
                    }

                    // Add the (now potentially specialized) child module to the work queue
                    workQueue.emplace(ParamState::WQKey{!VN_IS(newModp, Iface), newModp->level()},
                                      newModp);

                    // Add to the hierarchy registry
                    m_state.m_parentps[newModp].घुसाउ(modp);

                    // Eagerly specialize nested iface cells so their types
                    // have correct widths before sibling module cells run.
                    यदि (VN_IS(newModp, Iface) && newModp != srcModp) {
                        specializeNestedIfaceCells(newModp);
                    }
                }
            }
        }

        m_iterateModule = असत्य;
    }

    // Extract the base reference name from a dotted VarXRef (e.g., "iface.FOO" -> "iface")
    स्थिर string getRefBaseName(अचर AstVarXRef* refp) {
        अचर string dotted = refp->dotted();
        यदि (dotted.empty()) फर्काऊ "";
        फर्काऊ dotted.substr(0, dotted.find('.'));
    }

    // Debug-only diagnostic (requires --debugi-V3Param 9).
    // Walks parentModp looking for RefDTypes or VarRefs whose typedef,
    // refDType, or variable target is still owned by templateModp (the
    // unspecialized interface template).  Any such "leak" indicates a
    // pointer that was not properly redirected to the clone during
    // deparameterization.  Logs each leak with ancestry for triage.
    शून्य logTemplateLeakRefs(Ast_गाँठो_Module* parentModp, Ast_गाँठो_Module* templateModp,
                             अचर वर्ण* stage, Ast_गाँठो* contextp) {
        यदि (debug() < 9 || !parentModp || !templateModp || !VN_IS(templateModp, Iface)) फर्काऊ;
        // LCOV_EXCL_START  // Debug-only diagnostic
        पूर्णाङ्क leak_गन्ती = 0;
        अचर स्वतः ancestryOf = [](अचर Ast_गाँठो* nodep) {
            string ancestry;
            लागि (अचर Ast_गाँठो* curp = nodep; curp; curp = curp->backp()) {
                यदि (!ancestry.empty()) ancestry += "<-";
                ancestry += curp->typeName();
                यदि (VN_IS(curp, गाँठो_Module) || VN_IS(curp, Netlist) || VN_IS(curp, TypeTable)) {
                    रोक;
                }
            }
            फर्काऊ ancestry;
        };

        parentModp->foreach([&](AstRefDType* refp) {
            यदि (refp->typedefp()) {
                Ast_गाँठो_Module* अचर tdOwnerp
                    = V3LinkDotIfaceCapture::findOwnerModule(refp->typedefp());
                यदि (tdOwnerp == templateModp) {
                    ++leak_गन्ती;
                    UINFO(9, "TEMPLATE-LEAK "
                                 << stage << " parent=" << parentModp->prettyNameQ()
                                 << " template=" << templateModp->prettyNameQ() << " contextType="
                                 << (contextp ? contextp->typeName() : string("<null>"))
                                 << " contextName="
                                 << (contextp ? contextp->prettyNameQ() : "'<null>'")
                                 << " leak=REFDTYPE typedef owner" << " ref=<"
                                 << Ast_गाँठो::गाँठो_Addr(refp) << ">" << " refName="
                                 << refp->prettyNameQ() << " ancestry=" << ancestryOf(refp));
                }
            }
            यदि (refp->refDTypep()) {
                Ast_गाँठो_Module* अचर rdOwnerp
                    = V3LinkDotIfaceCapture::findOwnerModule(refp->refDTypep());
                यदि (rdOwnerp == templateModp) {
                    ++leak_गन्ती;
                    UINFO(9, "TEMPLATE-LEAK "
                                 << stage << " parent=" << parentModp->prettyNameQ()
                                 << " template=" << templateModp->prettyNameQ() << " contextType="
                                 << (contextp ? contextp->typeName() : string("<null>"))
                                 << " contextName="
                                 << (contextp ? contextp->prettyNameQ() : "'<null>'")
                                 << " leak=REFDTYPE refDType owner" << " ref=<"
                                 << Ast_गाँठो::गाँठो_Addr(refp) << ">" << " refName="
                                 << refp->prettyNameQ() << " ancestry=" << ancestryOf(refp));
                }
            }
        });

        parentModp->foreach([&](AstVarRef* varrefp) {
            यदि (!varrefp->varp()) फर्काऊ;
            Ast_गाँठो_Module* अचर varOwnerp
                = V3LinkDotIfaceCapture::findOwnerModule(varrefp->varp());
            यदि (varOwnerp != templateModp) फर्काऊ;
            ++leak_गन्ती;
            UINFO(9, "TEMPLATE-LEAK "
                         << stage << " parent=" << parentModp->prettyNameQ()
                         << " template=" << templateModp->prettyNameQ()
                         << " contextType=" << (contextp ? contextp->typeName() : string("<null>"))
                         << " contextName=" << (contextp ? contextp->prettyNameQ() : "'<null>'")
                         << " leak=VARREF target owner" << " ref=<" << Ast_गाँठो::गाँठो_Addr(varrefp)
                         << ">" << " var=" << varrefp->prettyNameQ()
                         << " ancestry=" << ancestryOf(varrefp));
        });

        यदि (leak_गन्ती > 0) {
            UINFO(9, "TEMPLATE-LEAK summary stage='"
                         << stage << "' parent=" << parentModp->prettyNameQ()
                         << " template=" << templateModp->prettyNameQ() << " count=" << leak_गन्ती);
        }
        // LCOV_EXCL_STOP
    }

    // Flag hierarchical refs in a parameter value. Single top-down pass.
    शून्य checkParamNotHierRecurse(Ast_गाँठो* nodep, तार्किक underTypeQuery = असत्य) {
        लागि (; nodep; nodep = nodep->nextp()) {
            // Refs read only for their type ($bits etc.) are allowed
            अचर AstAttrOf* अचर attrp = VN_CAST(nodep, AttrOf);
            अचर तार्किक childUnderQuery
                = underTypeQuery || (attrp && attrp->attrType().isTypeQuery());
            यदि (अचर AstVarXRef* अचर refp = VN_CAST(nodep, VarXRef)) {
                // Allow hierarchical ref to interface params through interface/modport ports
                // or local interface instances
                तार्किक isIfaceRef = असत्य;
                यदि (refp->varp() && refp->varp()->isIfaceParam()) {
                    अचर string refname = getRefBaseName(refp);
                    isIfaceRef
                        = !refname.empty()
                          && (m_ifacePortNames.गन्ती(refname) || m_ifaceInstCells.गन्ती(refname));
                }
                यदि (!isIfaceRef && !underTypeQuery) {
                    refp->v3warn(HIERPARAM, "Parameter values cannot use hierarchical values"
                                            " (IEEE 1800-2023 6.20.2)");
                }
            } अन्यथा यदि (अचर Ast_गाँठो_FTask_Ref* अचर refp = VN_CAST(nodep, गाँठो_FTask_Ref)) {
                यदि (refp->dotted() != "") {
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
    शून्य specializeNestedIfaceCells(Ast_गाँठो_Module* ifaceModp) {
        लागि (Ast_गाँठो* stmtp = ifaceModp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstCell* अचर nestedCellp = VN_CAST(stmtp, Cell);
            यदि (!nestedCellp) जारी;
            यदि (!VN_IS(nestedCellp->modp(), Iface)) जारी;
            यदि (!nestedCellp->paramsp()) जारी;

            Ast_गाँठो_Module* अचर nestedSrcModp = nestedCellp->modp();
            यदि (Ast_गाँठो_Module* अचर nestedNewModp = m_processor.गाँठो_Deparam(
                    nestedCellp, nestedSrcModp, ifaceModp, ifaceModp->someInstanceName())) {
                यदि (nestedNewModp != nestedSrcModp) {
                    // Constify the nested clone's params so its types have correct widths.
                    लागि (Ast_गाँठो* sp = nestedNewModp->stmtsp(); sp; sp = sp->nextp()) {
                        यदि (AstVar* अचर varp = VN_CAST(sp, Var)) {
                            यदि (varp->isParam() && varp->valuep()) {
                                V3Const::constifyParamsEdit(varp);
                            }
                        }
                    }
                    // Retarget REFDTYPEs in the outer clone to the nested clone's types.
                    यदि (V3LinkDotIfaceCapture::enabled()) {
                        m_processor.retargetIfaceRefs(ifaceModp, nestedCellp->name());
                    }
                    specializeNestedIfaceCells(nestedNewModp);
                }
            }
        }
    }

    // Check if cell parameters reference interface ports or local interface instances
    तार्किक cellParamsReferenceIfacePorts(AstCell* cellp) {
        यदि (!cellp->paramsp()) फर्काऊ असत्य;

        लागि (Ast_गाँठो* nodep = cellp->paramsp(); nodep; nodep = nodep->nextp()) {
            यदि (AstPin* अचर pinp = VN_CAST(nodep, Pin)) {
                यदि (Ast_गाँठो* अचर exprp = pinp->exprp()) {
                    यदि (अचर AstVarXRef* अचर refp = VN_CAST(exprp, VarXRef)) {
                        अचर string refname = getRefBaseName(refp);
                        यदि (!refname.empty()
                            && (m_ifacePortNames.गन्ती(refname)
                                || m_ifaceInstCells.गन्ती(refname)))
                            फर्काऊ सत्य;
                    }
                }
            }
        }
        फर्काऊ असत्य;
    }

    // Split interface array port 'portp' into a variable for each of its elements, in the
    // dimensions of 'dtypep' inwards. 'suffix' is the name suffix of the dimensions outside
    // 'dtypep'.
    शून्य expandIfaceArrayPortDimensions(AstVar* portp, Ast_गाँठो_DType* dtypep,
                                        अचर std::string& suffix) {
        अचर AstUnpackArrayDType* अचर arrp = VN_CAST(dtypep->skipRefp(), UnpackArrayDType);
        // Base case: add the element variable when no dimensions left
        यदि (!arrp) {
            AstVar* अचर varp = portp->clone_रूख(असत्य);
            varp->name(portp->name() + suffix);
            varp->origName(portp->origName() + suffix);
            यदि (Ast_गाँठो_DType* अचर oldp = varp->childDTypep()) {
                VL_DO_DANGLING(थप_Deletep(oldp->unlinkFrBack()), oldp);
            }
            varp->dtypep(nullptr);
            varp->childDTypep(portp->subDTypep()->elemDTypep()->clone_रूख(असत्य));
            portp->addNextHere(varp);
            फर्काऊ;
        }

        // Enumerate the current dimension given by 'arrp'
        // Each element is added right after 'portp', so go from right to left,
        // to end with an enumeration from the left index to the right index.
        लागि (अचर पूर्णाङ्क n : arrp->declRange().seq_दायाँ_To_बायाँ()) {
            अचर std::string s = suffix + "__BRA__" + Ast_गाँठो::encode_सङ्ख्या(n) + "__KET__";
            expandIfaceArrayPortDimensions(portp, arrp->subDTypep(), s);
        }
    }

    // Add the elements of instance array 'arrayedCellp' in the dimensions from 'rangep' inwards.
    // 'suffix' and 'idx' are the name suffix and row-major position (each dimension counted from
    // the left) of the dimensions outside 'rangep'. For an array of interfaces, 'ifaceVarp' is
    // the variable referencing the whole array (see V3LinkCells), which is also split into
    // variables referencing the elements.
    शून्य expandCellArrayDimensions(AstCell* arrayedCellp, AstVar* ifaceVarp,
                                   अचर AstRange* rangep, अचर std::string& suffix, पूर्णाङ्क idx) {
        // Base case: insert the element when no dimensions left
        यदि (!rangep) {
            AstCell* अचर elemp = arrayedCellp->clone_रूख(असत्य);
            elemp->name(arrayedCellp->name() + suffix);
            elemp->origName(arrayedCellp->origName() + suffix);
            elemp->arrayIdx(idx);
            arrayedCellp->addNextHere(elemp);
            यदि (ifaceVarp) {
                AstIfaceRefDType* अचर irefp
                    = VN_AS(ifaceVarp->subDTypep()->elemDTypep(), IfaceRefDType)->clone_रूख(असत्य);
                irefp->cellp(elemp);
                irefp->cellName(elemp->name());
                // Named like the variable of any interface cell (see V3LinkCells)
                AstVar* अचर varp = ifaceVarp->clone_रूख(असत्य);
                varp->name(elemp->name() + "__Viftop");
                varp->origName(elemp->origName() + "__Viftop");
                यदि (Ast_गाँठो_DType* अचर oldp = varp->childDTypep()) {
                    VL_DO_DANGLING(थप_Deletep(oldp->unlinkFrBack()), oldp);
                }
                varp->dtypep(nullptr);
                varp->childDTypep(irefp);
                ifaceVarp->addNextHere(varp);
            }
            फर्काऊ;
        }

        // Enumerate the current dimension given by 'rangep'
        // Each element is added right after 'arrayedCellp', so go from right to left,
        // to end with an enumeration from the left index to the right index.
        idx = (idx + 1) * rangep->elementsConst();
        अचर AstRange* अचर subRangep = VN_AS(rangep->nextp(), Range);
        लागि (अचर पूर्णाङ्क n : rangep->seq_दायाँ_To_बायाँ()) {
            अचर std::string s = suffix + "__BRA__" + Ast_गाँठो::encode_सङ्ख्या(n) + "__KET__";
            expandCellArrayDimensions(arrayedCellp, ifaceVarp, subRangep, s, --idx);
        }
    }

    // A generic visitor for cells and class refs
    शून्य visitCellOrClassRef(Ast_गाँठो* nodep, तार्किक isIface) {
        // Must do ifaces first, so push to list and do in proper order
        m_genHierNames.emplace(nodep, m_generateHierName);

        // Deparameterize iface cells early so types are available for lparams.
        यदि (isIface && VN_CAST(nodep, Cell) && VN_CAST(nodep, Cell)->paramsp()) {
            AstCell* अचर cellp = VN_CAST(nodep, Cell);
            यदि (!cellParamsReferenceIfacePorts(cellp)) {
                Ast_गाँठो_Module* अचर srcModp = cellp->modp();
                Ast_गाँठो_Module* अचर newModp
                    = m_processor.गाँठो_Deparam(cellp, srcModp, m_modp, m_modp->someInstanceName());
                // Retarget template REFDTYPEs to the clone's types.
                यदि (V3LinkDotIfaceCapture::enabled() && cellp->modp() != srcModp) {
                    m_processor.retargetIfaceRefs(m_modp, cellp->name());
                }
                // Specialize nested iface cells so their types are correct.
                यदि (newModp && newModp != srcModp) { specializeNestedIfaceCells(newModp); }
            }
        }

        // Visit parameters in the instantiation.
        iterateChildren(nodep);
        // A generate condition is folded and deleted before the drain loop runs, so
        // queueing from one only leaves a dangling pointer.  See iterateGenerateCond.
        यदि (!m_inGenerateCond) m_cellps.emplace(!isIface, nodep);
    }

    // VISITORS
    शून्य visit(Ast_गाँठो_Module* nodep) override {
        यदि (nodep->recursiveClone()) nodep->dead(सत्य);  // Fake, made for recursive elimination
        यदि (nodep->dead()) फर्काऊ;  // Marked by LinkDot (and above)
        UINFO(9, "V3Param: visit module name=" << nodep->prettyNameQ() << " orig='"
                                               << nodep->origName() << "' someInstanceName='"
                                               << nodep->someInstanceName() << "' hasGParam="
                                               << (nodep->hasGParam() ? "yes" : "no")
                                               << " user3p=" << (nodep->user3p() ? "set" : "null")
                                               << " dead=" << (nodep->dead() ? "yes" : "no"));
        यदि (AstClass* अचर classp = VN_CAST(nodep, Class)) {
            यदि (classp->hasGParam()) {
                // Don't enter into a definition.
                // If a class is used, it will be visited through a reference and cloned
                m_state.m_paramClasses.थप_back(classp);
                फर्काऊ;
            }
        }

        यदि (m_iterateModule) {  // Iterating from processWorkQ
            UINFO(4, " MOD-under-MOD.  " << nodep);
            // Delay until current module is done.
            // processWorkQ() (which we are returning to) will process nodep later
            m_state.m_workQueueNext.emplace(
                ParamState::WQKey{!VN_IS(nodep, Iface), nodep->level()}, nodep);
            फर्काऊ;
        }

        // Start traversal at root-like things
        यदि (nodep->is_शीर्ष()  // Tops
            || VN_IS(nodep, Class)  //  Moved classes
            || VN_IS(nodep, Package)) {  // Likewise haven't done wrapTopPackages yet
            m_state.m_workQueueNext.emplace(
                ParamState::WQKey{!VN_IS(nodep, Iface), nodep->level()}, nodep);
            processWorkQ();
        }
    }

    तार्किक isCircularType(अचर AstRefDType* nodep) {
        अचर स्वतः iter = m_isCircular.emplace(nodep, सत्य);
        यदि (!iter.second) फर्काऊ iter.first->second;
        यदि (अचर AstRefDType* अचर subDTypep = VN_CAST(nodep->subDTypep(), RefDType)) {
            अचर तार्किक ret = isCircularType(subDTypep);
            iter.first->second = ret;
            फर्काऊ ret;
        }
        iter.first->second = असत्य;
        फर्काऊ असत्य;
    }

    शून्य visit(AstRefDType* nodep) override {
        अचर तार्किक isCircular = isCircularType(nodep);
        // A module-body `typedef C#(Cfg)::t alias` the deferred pin/param walk
        // never reached would survive to V3Width unlinked. If its alias chain is
        // broken, run the same deferred resolution here.
        यदि (!isCircular && nodep->typedefp() && !nodep->skipRefOrNullp()) {
            m_processor.resolveDeferredDotsReachableFrom(nodep, m_modp);
        }
        यदि (isCircular) {
            nodep->v3error("Typedef's type is circular: " << nodep->prettyName());
        } अन्यथा यदि (nodep->typedefp() && nodep->subDTypep()
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
        यदि (nodep->rangep() && nodep->arrayIdx() < 0) {
            // Type check the ranges
            लागि (AstRange* rangep = nodep->rangep(); rangep;
                 rangep = VN_AS(rangep->nextp(), Range)) {
                rangep = VN_AS(V3Width::widthParamsEdit(rangep), Range);
            }
            // If it's an interface, pick up the corresponding AstIfaceRefDType variable
            AstVar* अचर ifaceVarp = [&]() -> AstVar* {
                यदि (!VN_IS(nodep->modp(), Iface)) फर्काऊ nullptr;
                // The variable referencing the whole array, right after the cell (see V3LinkCells)
                AstVar* अचर varp = VN_CAST(nodep->nextp(), Var);
                UASSERT_OBJ(varp && varp->name() == nodep->name() + "__Viftop", nodep,
                            "No __Viftop variable for interface array");
                फर्काऊ varp;
            }();
            // Expand the instance array into its elements
            expandCellArrayDimensions(nodep, ifaceVarp, nodep->rangep(), "", 0);
            यदि (ifaceVarp) {
                ifaceVarp->isIfaceArraySplit(सत्य);
                // Until V3Width removes it, the whole array variable refers to the first
                // element, which is also used for accessing the parameters of the array
                AstCell* अचर firstp = VN_AS(nodep->nextp(), Cell);
                AstIfaceRefDType* अचर irefp
                    = VN_AS(ifaceVarp->subDTypep()->elemDTypep(), IfaceRefDType);
                irefp->cellp(firstp);
                irefp->cellName(firstp->name());
                m_ifaceInstCells.emplace(nodep->name(), firstp);
            }
            VL_DO_DANGLING(थप_Deletep(nodep->unlinkFrBack()), nodep);
            // The elements replaced the arrayed cell in place, so will be iterated next.
            फर्काऊ;
        }
        checkParamNotHierRecurse(nodep->paramsp());
        यदि (VN_IS(nodep->modp(), Iface)) m_ifaceInstCells.emplace(nodep->name(), nodep);
        visitCellOrClassRef(nodep, VN_IS(nodep->modp(), Iface));
    }
    शून्य visit(AstIfaceRefDType* nodep) override {
        nodep->skipRefp()->user2SetOnce();
        यदि (nodep->ifacep()) visitCellOrClassRef(nodep, सत्य);
    }
    शून्य visit(AstClassRefDType* nodep) override {
        nodep->skipRefp()->user2SetOnce();
        checkParamNotHierRecurse(nodep->paramsp());
        visitCellOrClassRef(nodep, असत्य);
    }
    शून्य visit(AstClassOrPackageRef* nodep) override {
        // If it points to a typedef it is not really a class reference. That typedef will be
        // visited anyway (from its parent node), so even if it points to a parameterized class
        // type, the instance will be created.
        यदि (!VN_IS(nodep->classOrPackageNodep(), Typedef)) visitCellOrClassRef(nodep, असत्य);
    }

    // Recurse into AstGenBlock as generate blocks aren't flattened until V3Begin::debeginAll,
    // well after V3Param runs
    स्थिर शून्य gatherVars(Ast_गाँठो* stmtsp, तार्किक (*matchp)(अचर AstVar*), VarsByName& varsr) {
        लागि (Ast_गाँठो* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
            यदि (AstVar* अचर varp = VN_CAST(nodep, Var)) {
                // emplace, not assign, so the first declaration of a name wins
                यदि (matchp(varp)) varsr.emplace(varp->name(), varp);
            } अन्यथा यदि (AstGenBlock* अचर genp = VN_CAST(nodep, GenBlock)) {
                gatherVars(genp->itemsp(), matchp, varsr);
            }
        }
    }
    अचर VarsByName& modIfaceRefs() {
        यदि (!m_modIfaceRefsDone) {
            gatherVars(
                m_modp->stmtsp(), [](अचर AstVar* varp) { फर्काऊ varp->isIfaceRef(); },
                m_modIfaceRefs);
            m_modIfaceRefsDone = सत्य;
        }
        फर्काऊ m_modIfaceRefs;
    }
    अचर VarsByName& ifaceParams(Ast_गाँठो_Module* ifacep) {
        अचर स्वतः pair = m_ifaceParams.emplace(ifacep, VarsByName{});
        यदि (pair.second) {
            gatherVars(
                ifacep->stmtsp(), [](अचर AstVar* varp) { फर्काऊ varp->isParam(); },
                pair.first->second);
        }
        फर्काऊ pair.first->second;
    }
    // Resolve a generic-interface 'ifacePort.member' Dot left unlinked by V3LinkDot, now that
    // ifacePort may have been specialized. Returns nullptr if still unresolvable
    Ast_गाँठो* tryResolveGenericIfaceDot(AstDot* dotp) {
        AstParseRef* अचर lhsp = VN_CAST(dotp->lhsp(), ParseRef);
        AstParseRef* अचर rhsp = VN_CAST(dotp->rhsp(), ParseRef);
        यदि (!lhsp || !rhsp) फर्काऊ nullptr;
        // Always called from visit(AstVar) inside processWorkQ()'s VL_RESTORER(m_modp) scope
        यदि (VL_UNCOVERABLE(!m_modp)) फर्काऊ nullptr;

        अचर स्वतः& refVars = modIfaceRefs();
        अचर स्वतः ifaceVarIt = refVars.find(lhsp->name());
        // V3LinkDot only defers Dots whose lhs is a non-array iface-ref var, so this can't fail
        यदि (VL_UNCOVERABLE(ifaceVarIt == refVars.end())) फर्काऊ nullptr;
        AstVar* अचर ifaceVarp = ifaceVarIt->second;
        AstIfaceRefDType* अचर ifacerefp = VN_CAST(ifaceVarp->subDTypep(), IfaceRefDType);
        यदि (VL_UNCOVERABLE(!ifacerefp || !ifacerefp->ifacep())) फर्काऊ nullptr;

        अचर स्वतः& ifaceVars = ifaceParams(ifacerefp->ifacep());
        अचर स्वतः targetVarIt = ifaceVars.find(rhsp->name());
        यदि (targetVarIt == ifaceVars.end()) फर्काऊ nullptr;
        AstVar* अचर targetVarp = targetVarIt->second;
        // processWorkQ() visits interface cells first, so the interface is already constified
        यदि (VL_UNCOVERABLE(!VN_IS(targetVarp->valuep(), Const))) iterate(targetVarp);
        यदि (VL_UNCOVERABLE(!targetVarp->valuep() || !VN_IS(targetVarp->valuep(), Const))) {
            फर्काऊ nullptr;  // LCOV_EXCL_LINE
        }
        फर्काऊ targetVarp->valuep()->clone_रूख(असत्य);
    }
    शून्य resolveGenericIfaceDotsIn(Ast_गाँठो* nodep) {
        // Collect first: the deleteTree() below frees a node foreach() would still read from
        std::vector<AstDot*> dotps;
        nodep->foreach([&](AstDot* dotp) { dotps.थप_back(dotp); });
        लागि (AstDot* अचर dotp : dotps) {
            यदि (Ast_गाँठो* अचर newp = tryResolveGenericIfaceDot(dotp)) {
                dotp->replaceWith(newp);
                VL_DO_DANGLING(dotp->delete_रूख(), dotp);
            }
        }
    }

    // Make sure all parameters are constantified
    शून्य visit(AstVar* nodep) override {
        यदि (nodep->user2SetOnce()) फर्काऊ;  // Process once
        // Interface array port: split it into its elements
        यदि (nodep->isIfaceRef() && !nodep->isIfaceParent()
            && (VN_IS(nodep->subDTypep()->skipRefp(), UnpackArrayDType)
                || VN_IS(nodep->subDTypep()->skipRefp(), BracketArrayDType))) {
            // Also converts any C-style [N] dimensions, the ranges must be constant
            V3Width::widthParamsEdit(nodep->subDTypep());
            expandIfaceArrayPortDimensions(nodep, nodep->subDTypep(), "");
            nodep->isIfaceArraySplit(सत्य);
        }
        // Build cache of interface port names as we encounter them
        यदि (nodep->isIfaceRef()) { m_ifacePortNames.घुसाउ(nodep->name()); }
        iterateChildren(nodep);
        यदि (nodep->isParam()) {
            checkParamNotHierRecurse(nodep->valuep());
            यदि (!nodep->valuep() && !VN_IS(m_modp, Class)) {
                nodep->v3error("Parameter without default value is never given value"
                               << " (IEEE 1800-2023 6.20.1): " << nodep->prettyNameQ());
            } अन्यथा यदि (nodep->valuep()) {
                // Resolve now: a sibling cell's pin fold may need this before linkDotParamed
                resolveGenericIfaceDotsIn(nodep->valuep());
                // If the value expression contains a VarXRef to an interface
                // localparam whose value is not yet constant, defer constification
                // to avoid premature widthing with unresolved values (see
                // t_lparam_dep_iface tests).  When the referenced localparam is
                // already const, proceed normally so FUNCREFs with resolved iface
                // param args get folded.
                तार्किक hasUnresolvedLparamXRef = असत्य;
                nodep->valuep()->foreach([&](अचर AstVarXRef* xrefp) {
                    यदि (अचर AstVar* अचर varp = xrefp->varp()) {
                        यदि (varp->varType() == VVarType::LPARAM && !VN_IS(varp->valuep(), Const)) {
                            hasUnresolvedLparamXRef = सत्य;
                        }
                    }
                });
                यदि (hasUnresolvedLparamXRef) फर्काऊ;
                // Defer if value has any unresolved Dot, or references a deferred lparam
                अचर तार्किक hasDot = nodep->valuep()->exists([](AstDot*) { फर्काऊ सत्य; });
                तार्किक refsDeferred = असत्य;
                यदि (!hasDot) {
                    अचर स्वतः& deferredVarps = v3Global.rootp()->deferredParamVarps();
                    nodep->valuep()->foreach([&](अचर AstVarRef* refp) {
                        यदि (refsDeferred) फर्काऊ;
                        AstVar* अचर refVarp = refp->varp();
                        यदि (refVarp && refVarp->varType() == VVarType::LPARAM
                            && deferredVarps.गन्ती(refVarp)) {
                            refsDeferred = सत्य;
                        }
                    });
                }
                यदि (hasDot || refsDeferred) {
                    v3Global.rootp()->थप_Deferred_Param_Varp(nodep);
                    फर्काऊ;
                }
                m_processor.resolveDeferredDotsReachableFrom(nodep, m_modp);
                V3Const::constifyParamsEdit(nodep);
            }
        }
    }
    शून्य visit(AstParamTypeDType* nodep) override {
        iterateChildren(nodep);
        यदि (VN_IS(nodep->skipRefOrNullp(), VoidDType)) {
            nodep->v3error("Parameter type without default value is never given value"
                           << " (IEEE 1800-2023 6.20.1): " << nodep->prettyNameQ());
        }
    }
    // Make sure varrefs cause vars to constify before things above
    शून्य visit(AstVarRef* nodep) override {
        // Might jump across functions, so beware if ever add a m_funcp
        यदि (nodep->varp()) iterate(nodep->varp());
    }
    तार्किक ifaceParamReplace(AstVarXRef* nodep, Ast_गाँठो* candp) {
        लागि (; candp; candp = candp->nextp()) {
            यदि (nodep->name() == candp->name()) {
                यदि (AstVar* अचर varp = VN_CAST(candp, Var)) {
                    UINFO(9, "Found interface parameter: " << varp);
                    // The interface may not have been visited yet (it is at a
                    // deeper level in the work queue), so its localparams may
                    // not be constified.  Eagerly constify here so that the
                    // caller's hasUnresolvedLparamXRef check sees a Const.
                    यदि (varp->isParam() && varp->valuep() && !VN_IS(varp->valuep(), Const)) {
                        V3Const::constifyParamsEdit(varp);
                    }
                    nodep->varp(varp);
                    फर्काऊ सत्य;
                } अन्यथा यदि (अचर AstPin* अचर pinp = VN_CAST(candp, Pin)) {
                    UINFO(9, "Found interface parameter: " << pinp);
                    UASSERT_OBJ(pinp->exprp(), pinp, "Interface parameter pin missing expression");
                    nodep->replaceWith(pinp->exprp()->clone_रूख(असत्य));
                    VL_DO_DANGLING(थप_Deletep(nodep), nodep);
                    फर्काऊ सत्य;
                }
            }
        }
        फर्काऊ असत्य;
    }

    शून्य deparamIfaceCellNow(AstCell* cellp) {
        यदि (!cellp->paramsp()) फर्काऊ;
        यदि (!VN_IS(cellp->modp(), Iface)) फर्काऊ;
        Ast_गाँठो_Module* अचर srcModp = cellp->modp();
        Ast_गाँठो_Module* अचर newModp
            = m_processor.गाँठो_Deparam(cellp, srcModp, m_modp, m_modp->someInstanceName());
        यदि (newModp && newModp != srcModp) {
            यदि (V3LinkDotIfaceCapture::enabled()) {
                m_processor.retargetIfaceRefs(m_modp, cellp->name());
            }
            specializeNestedIfaceCells(newModp);
        }
    }

    शून्य visit(Ast_गाँठो_FTask_Ref* nodep) override {
        यदि (nodep->containsGenBlock()) {
            // Needs relink, as may remove pointed-to task/func
            nodep->taskp(nullptr);
            फर्काऊ;
        }
        iterateChildren(nodep);
    }
    शून्य visit(AstEnumItemRef* nodep) override {
        // Needs relink, as may remove pointed-to item
        यदि (nodep->containsGenBlock()) nodep->itemp(nullptr);
        iterateChildren(nodep);
    }
    शून्य visit(AstVarXRef* nodep) override {
        यदि (nodep->containsGenBlock()) {
            // Needs relink, as may remove pointed-to var
            nodep->varp(nullptr);
            फर्काऊ;
        }
        // Parameters of an interface array are accessed through its first element
        यदि (nodep->varp() && nodep->varp()->isParam()) {
            अचर स्वतः it = m_ifaceInstCells.find(nodep->dotted());
            यदि (it != m_ifaceInstCells.end()) nodep->dotted(it->second->name());
        }
        // Check to see if the scope is just an interface because interfaces are special
        अचर string dotted = nodep->dotted();
        यदि (!dotted.empty() && nodep->varp() && nodep->varp()->isParam()) {
            अचर Ast_गाँठो* backp = nodep;
            जबसम्म ((backp = backp->backp())) {
                यदि (VN_IS(backp, गाँठो_Module)) {
                    UINFO(9, "Hit module boundary, done looking for interface");
                    रोक;
                }
                यदि (अचर AstVar* अचर varp = VN_CAST(backp, Var)) {
                    यदि (!varp->isIfaceRef()) जारी;
                    // Through all dimensions of an array
                    अचर AstIfaceRefDType* अचर ifacerefp
                        = VN_CAST(varp->subDTypep()->elemDTypep(), IfaceRefDType);
                    यदि (!ifacerefp) जारी;
                    // Interfaces passed in on the port map have ifaces
                    यदि (अचर AstIface* अचर ifacep = ifacerefp->ifacep()) {
                        यदि (dotted == backp->name()) {
                            UINFO(9, "Iface matching scope:  " << ifacep);
                            यदि (ifaceParamReplace(nodep, ifacep->stmtsp())) {  //
                                फर्काऊ;
                            }
                        }
                    }
                    // Interfaces declared in this module have cells
                    अन्यथा यदि (AstCell* अचर cellp = ifacerefp->cellp()) {
                        यदि (dotted == cellp->name()) {
                            UINFO(9, "Iface matching scope:  " << cellp);
                            deparamIfaceCellNow(cellp);
                            यदि (ifaceParamReplace(nodep, cellp->modp()->stmtsp())) {  //
                                फर्काऊ;
                            }
                        }
                    }
                }
            }
            // Fallback: a direct local interface instance ("inst.PARAM"), not reached via a port.
            अचर स्वतः ifaceCellIt = m_ifaceInstCells.find(dotted);
            यदि (ifaceCellIt != m_ifaceInstCells.end()) {
                AstCell* अचर cellp = ifaceCellIt->second;
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
        अचर AstClass* lhsClassp = nullptr;
        अचर AstClassOrPackageRef* अचर classRefp = VN_CAST(nodep->lhsp(), ClassOrPackageRef);
        यदि (classRefp) lhsClassp = VN_CAST(classRefp->classOrPackageSkipp(), Class);
        Ast_गाँठो* rhsDefp = nullptr;
        AstClassOrPackageRef* अचर rhsp = VN_CAST(nodep->rhsp(), ClassOrPackageRef);
        यदि (rhsp) rhsDefp = rhsp->classOrPackageNodep();
        यदि (lhsClassp && rhsDefp) {
            m_state.m_dots.थप_back(nodep);
            // No need to iterate into rhsp, because there should be nothing to do
        } अन्यथा {
            iterate(nodep->rhsp());
        }
    }

    शून्य visit(AstUnlinkedRef* nodep) override {
        AstVarXRef* अचर varxrefp = VN_CAST(nodep->refp(), VarXRef);
        Ast_गाँठो_FTask_Ref* अचर taskrefp = VN_CAST(nodep->refp(), गाँठो_FTask_Ref);
        AstEnumItemRef* अचर enumrefp = VN_CAST(nodep->refp(), EnumItemRef);
        यदि (varxrefp) {
            m_unlinkedTxt = varxrefp->dotted();
        } अन्यथा यदि (taskrefp) {
            m_unlinkedTxt = taskrefp->dotted();
        } अन्यथा यदि (enumrefp) {
            m_unlinkedTxt = enumrefp->dotted();
        } अन्यथा {
            nodep->v3fatalSrc("Unexpected AstUnlinkedRef node");
            फर्काऊ;
        }
        iterate(nodep->cellrefp());

        यदि (varxrefp) {
            varxrefp->dotted(m_unlinkedTxt);
        } अन्यथा यदि (taskrefp) {
            taskrefp->dotted(m_unlinkedTxt);
        } अन्यथा {
            enumrefp->dotted(m_unlinkedTxt);
        }
        nodep->replaceWith(nodep->refp()->unlinkFrBack());
        VL_DO_DANGLING(थप_Deletep(nodep), nodep);
    }
    शून्य visit(AstCellArrayRef* nodep) override {
        // Multi-dim iface arrays chain multiple selps via nextp(); constify each in place.
        लागि (Ast_गाँठो *s = nodep->selp(), *nxt; s; s = nxt) {
            nxt = s->nextp();  // cache before constifyParamsEdit replaces s
            V3Const::constifyParamsEdit(s);
        }
        std::vector<पूर्णाङ्क> indices;
        लागि (Ast_गाँठो* s = nodep->selp(); s; s = s->nextp()) {
            अचर AstConst* अचर constp = VN_CAST(s, Const);
            यदि (!constp) {
                nodep->v3error("Could not expand constant selection inside dotted reference: "
                               << s->prettyNameQ());
                फर्काऊ;
            }
            indices.थप_back(constp->toSInt());
        }
        // Nested iface-array ports may carry a __Viftop suffix that's not in m_unlinkedTxt.
        अचर string viftopSuffix = "__Viftop";
        अचर string baseName
            = VString::endsWith(nodep->name(), viftopSuffix)
                  ? nodep->name().substr(0, nodep->name().size() - viftopSuffix.size())
                  : nodep->name();
        अचर string placeholder = "__BRA__??__KET__";
        माप_प्रकार pos = m_unlinkedTxt.find(baseName + placeholder);
        // An AstCellArrayRef for an iface-array pin expr (e.g. l1(l2.l1[0])) is visited
        // outside AstUnlinkedRef, so m_unlinkedTxt has no placeholder; V3LinkDot resolves later.
        यदि (pos == string::npos) {
            UINFO(9, "Skipping unlinked text replacement for " << nodep);
            फर्काऊ;
        }
        pos += baseName.length();
        लागि (पूर्णाङ्क idx : indices) {
            अचर string replacement = "__BRA__" + Ast_गाँठो::encode_सङ्ख्या(idx) + "__KET__";
            यदि (m_unlinkedTxt.compare(pos, placeholder.length(), placeholder) != 0) {
                nodep->v3fatalSrc(  // LCOV_EXCL_LINE
                    "Expected placeholder at position in multi-dim iface dotted reference");
                फर्काऊ;
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
    शून्य iterateGenerateCond(Ast_गाँठो* nodep) {
        VL_RESTORER(m_inGenerateCond);
        m_inGenerateCond = सत्य;
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
        यदि (अचर AstConst* अचर constp = VN_CAST(nodep->condp(), Const)) {
            यदि (Ast_गाँठो* अचर keepp = (constp->isZero() ? nodep->elsesp() : nodep->thensp())) {
                keepp->unlinkFrBackWithNext();
                nodep->replaceWith(keepp);
            } अन्यथा {
                nodep->unlinkFrBack();
            }
            VL_DO_DANGLING(nodep->delete_रूख(), nodep);
            // Normal edit rules will now recurse the replacement
        } अन्यथा {
            nodep->condp()->v3error("Generate If condition must evaluate to constant");
        }
    }

    शून्य visit(AstGenBlock* nodep) override {
        // Parameter substitution for generated for loops.
        // TODO Unlike generated IF, we don't have to worry about short-circuiting the
        // conditional expression, since this is currently restricted to simple
        // comparisons. If we ever do move to more generic constant expressions, such code
        // will be needed here.
        यदि (AstGenFor* अचर forp = VN_AS(nodep->genforp(), GenFor)) {
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
            अचर string beginName = nodep->name();
            // Leave the original Begin, as need a container for the (possible) GENVAR
            // Note m_unroller will replace some AstVarRef's to the loop variable with constants
            // Don't remove any deleted nodes in m_unroller until whole process finishes,
            // as some AstXRefs may still point to old nodes.
            VL_DO_DANGLING(m_unroller.unroll(forp, beginName), forp);
            // Blocks were constructed under the special begin, move them up
            // Note forp is null, so grab statements again
            यदि (Ast_गाँठो* अचर stmtsp = nodep->genforp()) {
                stmtsp->unlinkFrBackWithNext();
                nodep->addNextHere(stmtsp);
                // Note this clears nodep->genforp(), so begin is no longer special
            }
        } अन्यथा {
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
        तार्किक hit = असत्य;
        Ast_गाँठो* keepp = nullptr;
        iterateGenerateCond(nodep->exprp());
        V3Case::caseLint(nodep);
        // Case-item expressions must also be linked before widthing the case.
        लागि (AstGenCaseItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), GenCaseItem)) {
            लागि (Ast_गाँठो* ep = itemp->condsp(); ep; ep = ep->nextp()) {
                m_processor.resolveDeferredDotsReachableFrom(ep, m_modp);
            }
        }
        V3Width::widthParamsEdit(nodep);  // Param typed widthing will NOT recurse the
                                          // body, don't trigger errors yet.
        V3Const::constifyParamsEdit(nodep->exprp());  // exprp may change
        अचर AstConst* अचर exprp = VN_AS(nodep->exprp(), Const);
        // Constify
        लागि (AstGenCaseItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), GenCaseItem)) {
            लागि (Ast_गाँठो* ep = itemp->condsp(); ep;) {
                Ast_गाँठो* अचर nextp = ep->nextp();  // May edit list
                iterateGenerateCond(ep);
                VL_DO_DANGLING(V3Const::constifyParamsEdit(ep), ep);  // ep may change
                ep = nextp;
            }
        }
        // Item match
        लागि (AstGenCaseItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), GenCaseItem)) {
            यदि (!itemp->isDefault()) {
                लागि (Ast_गाँठो* ep = itemp->condsp(); ep; ep = ep->nextp()) {
                    यदि (अचर AstConst* अचर ccondp = VN_CAST(ep, Const)) {
                        V3_सङ्ख्या match{nodep, 1};
                        match.opEq(ccondp->अङ्क(), exprp->अङ्क());
                        यदि (!hit && match.isNeqZero()) {
                            hit = सत्य;
                            keepp = itemp->itemsp();
                        }
                    } अन्यथा {
                        itemp->v3error("Generate Case item does not evaluate to constant");
                    }
                }
            }
        }
        // Else default match
        लागि (AstGenCaseItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), GenCaseItem)) {
            यदि (itemp->isDefault()) {
                यदि (!hit) {
                    hit = सत्य;
                    keepp = itemp->itemsp();
                }
            }
        }
        // Replace
        यदि (keepp) {
            keepp->unlinkFrBackWithNext();
            nodep->replaceWith(keepp);
        } अन्यथा {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(nodep->delete_रूख(), nodep);
    }

    शून्य visit(Ast_गाँठो* nodep) override { iterateChildren(nodep); }

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

वर्ग Param_शीर्ष final : VNDeleter {
    // NODE STATE
    // AstNodeModule::user1 -> bool: already fixed level (temporary)

    // STATE
    ParamState m_state;  // Common state

    // METHODS

    शून्य relinkDots() {
        // RHSs of AstDots need a relink when LHS is a parameterized class reference
        लागि (AstDot* अचर dotp : m_state.m_dots) {
            अचर AstClassOrPackageRef* अचर classRefp = VN_AS(dotp->lhsp(), ClassOrPackageRef);
            अचर AstClass* अचर lhsClassp = VN_AS(classRefp->classOrPackageSkipp(), Class);
            AstClassOrPackageRef* अचर rhsp = VN_AS(dotp->rhsp(), ClassOrPackageRef);
            लागि (स्वतः* itemp = lhsClassp->membersp(); itemp; itemp = itemp->nextp()) {
                यदि (itemp->name() == rhsp->name()) {
                    rhsp->classOrPackageNodep(itemp);
                    रोक;
                }
            }
        }
    }

    शून्य fixLevel(Ast_गाँठो_Module* modp) {
        // Fix up level of module, based on who instantiates it
        यदि (modp->user1SetOnce()) फर्काऊ;  // Already fixed
        यदि (m_state.m_parentps[modp].empty()) फर्काऊ;  // Leave top levels alone
        पूर्णाङ्क maxParentLevel = 0;
        लागि (Ast_गाँठो_Module* parentp : m_state.m_parentps[modp]) {
            fixLevel(parentp);  // Ensure parent level is correct
            maxParentLevel = std::max(maxParentLevel, parentp->level());
        }
        यदि (modp->level() <= maxParentLevel) modp->level(maxParentLevel + 1);
        // Not correct fixup of depth(), as it should be mininum.  But depth() is unused
        // past V3LinkParse, so just do something sane in case this code doesn't get updated
        modp->depth(modp->level());
    }

    शून्य resortNetlistModules(AstNetlist* netlistp) {
        // Re-sort module list to be in topological order and fix-up incorrect levels. We need to
        // do this globally at the end due to the presence of recursive modules, which might be
        // expanded in orders that reuse earlier specializations later at a lower level.
        // Gather modules
        std::vector<Ast_गाँठो_Module*> modps;
        लागि (Ast_गाँठो_Module *modp = netlistp->modulesp(), *nextp; modp; modp = nextp) {
            nextp = VN_AS(modp->nextp(), गाँठो_Module);
            modp->unlinkFrBack();
            modps.थप_back(modp);
        }

        // Fix-up levels
        अचर VNUser1InUse user1InUse;
        लागि (Ast_गाँठो_Module* अचर modp : modps) fixLevel(modp);

        // Sort by level
        std::stable_sort(modps.begin(), modps.end(),
                         [](अचर Ast_गाँठो_Module* ap, अचर Ast_गाँठो_Module* bp) {
                             फर्काऊ ap->level() < bp->level();
                         });

        // Re-insert modules
        लागि (Ast_गाँठो_Module* अचर modp : modps) netlistp->addModulesp(modp);
    }

सार्वजनिक:
    // CONSTRUCTORS
    explicit Param_शीर्ष(AstNetlist* netlistp) {
        // Relies on modules already being in top-down-order
        अचर ParamVisitor paramVisitor{m_state /*ref*/, netlistp};

        // Mark classes which cannot be removed because they are still referenced
        अचर ClassRefUnlinkerVisitor classUnlinkerVisitor{netlistp};

        netlistp->foreach([](Ast_गाँठो_FTask_Ref* ftaskrefp) {
            Ast_गाँठो_FTask* ftaskp = ftaskrefp->taskp();
            यदि (!ftaskp || !ftaskp->classMethod()) फर्काऊ;
            अचर std::string funcName = ftaskp->name();
            // Find the nearest containing (ancestor) class for a node.
            // Uses aboveLoopp() which correctly skips sibling links
            // (e.g. covergroup classes) to find the true parent.
            अचर स्वतः ancestorClassOf = [](Ast_गाँठो* startp) -> AstClass* {
                लागि (Ast_गाँठो* np = startp->aboveLoopp(); np; np = np->aboveLoopp()) {
                    यदि (AstClass* अचर cp = VN_CAST(np, Class)) फर्काऊ cp;
                }
                फर्काऊ nullptr;
            };
            AstClass* refClassp = ancestorClassOf(ftaskrefp);
            यदि (refClassp == ftaskrefp->classOrPackagep())
                फर्काऊ;  // task is in the same class as reference
            AstClass* classp = ancestorClassOf(ftaskp);
            UASSERT_OBJ(classp, ftaskrefp, "Class method has no class above it");
            // If the FUNCREF and its task are both in the same (clone) class but
            // classOrPackagep still points to the old template, just retarget it
            यदि (refClassp && refClassp == classp && ftaskrefp->classOrPackagep()
                && ftaskrefp->classOrPackagep() != refClassp) {
                ftaskrefp->classOrPackagep(refClassp);
                फर्काऊ;
            }
            यदि (classp->user3p()) फर्काऊ;  // will not get removed, no need to relink
            AstClass* अचर parametrizedClassp = VN_CAST(classp->user4p(), Class);
            यदि (!parametrizedClassp) फर्काऊ;
            Ast_गाँठो_FTask* newFuncp = nullptr;
            parametrizedClassp->exists([&newFuncp, funcName](Ast_गाँठो_FTask* ftaskp) {
                यदि (ftaskp->name() == funcName) {
                    newFuncp = ftaskp;
                    फर्काऊ सत्य;
                }
                फर्काऊ असत्य;
            });
            यदि (newFuncp) {
                // v3error(ftaskp <<"->" << newFuncp);
                ftaskrefp->taskp(newFuncp);
                ftaskrefp->classOrPackagep(parametrizedClassp);
            }
        });

        अचर ParamClassRefDTypeRelinkVisitor paramClassDTypeRelinkVisitor{netlistp};

        relinkDots();

        resortNetlistModules(netlistp);

        // For occasional debug only; this will upset VL_LEAK_CHECKS
        // V3Global::dumpCheckGlobalTree("param-predel", 0, dumpTreeEitherLevel() >= 9);

        // Remove defaulted classes
        // Unlike modules, which we keep around and mark dead() for later V3Dead
        std::unordered_set<AstClass*> removedClassps;
        लागि (AstClass* अचर classp : m_state.m_paramClasses) {
            यदि (!classp->user3p()) {
                // If there was an error, don't remove classes as they might
                // have remained referenced, and  will crash in V3Broken or
                // other locations. This is fine, we will abort imminently.
                यदि (V3Error::error_गन्ती()) जारी;
                removedClassps.emplace(classp);
                VL_DO_DANGLING(थप_Deletep(classp->unlinkFrBack()), classp);
            } अन्यथा {
                // Referenced. classp became a specialized class with the default
                // values of parameters and is not a parameterized class anymore
                classp->hasGParam(असत्य);
            }
        }
        // Remove references to defaulted classes
        // Reuse user3 to mark all nodes being deleted
        Ast_गाँठो::user3_Clear_रूख();
        लागि (AstClass* classp : removedClassps) {
            classp->foreach([](Ast_गाँठो* अचर nodep) { nodep->user3(सत्य); });
        }
        // Set all links pointing to a user3 (deleting) node as null
        netlistp->foreach([](Ast_गाँठो* अचर nodep) {
            nodep->foreachLink([&](Ast_गाँठो** अचर linkpp, अचर वर्ण* namep) {
                यदि (*linkpp && (*linkpp)->user3()) {
                    UINFO(9, "clear   link " << namep << " on " << nodep);
                    *linkpp = nullptr;
                    UINFO(9, "cleared link " << namep << " on " << nodep);
                }
            });
        });
    }
    ~Param_शीर्ष() = स्वभाव;
    VL_UNCOPYABLE(Param_शीर्ष);
};

//######################################################################
// Param class functions

शून्य V3Param::param(AstNetlist* rootp) {
    UINFO(2, __FUNCTION__ << ":");
    rootp->clearDeferredParamVarps();  // Defensive: drop any stragglers from a prior invocation

    यदि (dump_रूख_Either_Level() >= 9) V3LinkDotIfaceCapture::dumpEntries("before V3Param");
    { Param_शीर्ष{rootp}; }
    // The memo is only good while parameterizing, and the tree moves after.
    rootp->clearContainingModules();
    यदि (dump_रूख_Either_Level() >= 9) V3LinkDotIfaceCapture::dumpEntries("after V3Param");

    V3Global::dump_Check_Global_रूख("param", 0, dump_रूख_Either_Level() >= 3);
}

शून्य V3Param::finalizeDeferredParams(AstNetlist* rootp) {
    // Constify params whose Dot RHS was resolved by V3LinkDot::linkDotParamed.
    लागि (AstVar* अचर varp : rootp->deferredParamVarps()) {
        यदि (varp->valuep() && !VN_IS(varp->valuep(), Const)) V3Const::constifyParamsEdit(varp);
    }
    rootp->clearDeferredParamVarps();
}
