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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3Param.h"

#شامل "V3Case.h"
#شامل "V3Const.h"
#شامل "V3EmitV.h"
#شامل "V3HierBlock.h"
#شامل "V3LinkDotIfaceCapture.h"
#شامل "V3MemberMap.h"
#شامل "V3Os.h"
#شامل "V3Parse.h"
#شامل "V3Simulate.h"
#شامل "V3Stats.h"
#شامل "V3Unroll.h"
#شامل "V3Width.h"

#شامل <cctype>
#شامل <cstdlib>
#شامل <deque>
#شامل <map>
#شامل <memory>
#شامل <set>
#شامل <unordered_map>
#شامل <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

// Walk a class-typed node through typedef/RefDType chains to its ClassRefDType, or null.
ساکن AstClassRefDType* class_Ref_DType_کا_گرہ(Ast_گرہ* nodep) {
    جب (AstTypedef* ثابت tdp = VN_CAST(nodep, Typedef)) nodep = tdp->subDTypep();
    Ast_گرہ_DType* ثابت dtp = VN_CAST(nodep, گرہ_DType);
    واپس dtp ? VN_CAST(dtp->skipRefOrNullp(), ClassRefDType) : خالی_اشاریہ;
}

//######################################################################
// Hierarchical block and parameter db (modules without parameters are also handled)

جماعت Parameterized_Hier_بلاکیں final {
    استعمال Hier_بلاک_Opts_By_Orig_Name = std::multimap<std::string, ثابت V3_درجہ_وار_بلاک_Option*>;
    استعمال HierMapIt = Hier_بلاک_Opts_By_Orig_Name::const_iterator;
    استعمال Hier_بلاک_Mod_Map = std::map<ثابت std::string, Ast_گرہ_Module*>;
    استعمال ParamConstMap = std::map<ثابت std::string, std::unique_ptr<AstConst>>;
    استعمال GParamsMap = std::map<ثابت std::string, AstVar*>;  // key:parameter name value:parameter

    // MEMBERS
    ثابت منطقی m_hier_Sub_تسلسل;  // Is in sub-run for hierarchical verilation
    // key:Original module name, value:HiearchyBlockOption*
    // If a module is parameterized, the module is uniquified to overridden parameters.
    // This is why HierBlockOptsByOrigName is multimap.
    Hier_بلاک_Opts_By_Orig_Name m_hier_بلاک_Opts_By_Orig_Name;
    // key:mangled module name, value:AstNodeModule*
    Hier_بلاک_Mod_Map m_hier_بلاک_Mod;
    // Overridden parameters of the hierarchical block
    std::map<ثابت V3_درجہ_وار_بلاک_Option*, ParamConstMap> m_hierParams;
    // Parameter variables of hierarchical blocks
    std::map<ثابت std::string, GParamsMap> m_modParams;
    // Formals declared without a type, whose type is that of the assigned value
    std::سیٹ<ثابت AstVar*> m_untypedParams;

    // METHODS

عوامی:
    Parameterized_Hier_بلاکیں(ثابت V3_Hier_بلاک_Opt_سیٹ& hierOpts, AstNetlist* nodep)
        // A type parameter file only describes a child's own types; the final consolidation
        // run has none, but must still substitute the libraries it consumes.
        : m_hier_Sub_تسلسل{!v3Global.opt.hier_بلاکیں().خالی_خانے() || v3Global.opt.hier_بچہ()} {
        برائے (ثابت خودکار& hierOpt : hierOpts) {
            m_hier_بلاک_Opts_By_Orig_Name.emplace(hierOpt.دوسرا.origName(), &hierOpt.دوسرا);
            ثابت V3_درجہ_وار_بلاک_Option::ParamStrMap& params = hierOpt.دوسرا.params();
            ParamConstMap& consts = m_hierParams[&hierOpt.دوسرا];
            برائے (V3_درجہ_وار_بلاک_Option::ParamStrMap::const_iterator pIt = params.begin();
                 pIt != params.انتہا(); ++pIt) {
                std::unique_ptr<AstConst> constp{AstConst::parseParamLiteral(
                    نیا File_سطر{File_سطر::built_اندر_Filename()}, pIt->دوسرا)};
                UASSERT(constp, pIt->دوسرا << " is not a valid parameter literal");
                ثابت منطقی inserted = consts.emplace(pIt->پہلا, std::move(constp)).دوسرا;
                UASSERT(inserted, pIt->پہلا << " is already added");
            }
            // origName may be already registered, but it's fine.
            m_modParams.داخل_کرو({hierOpt.دوسرا.origName(), {}});
        }
        برائے (Ast_گرہ_Module* modp = nodep->modulesp(); modp;
             modp = VN_AS(modp->nextp(), گرہ_Module)) {
            اگر (hierOpts.ڈھونڈو(modp->prettyName()) != hierOpts.انتہا()) {
                m_hier_بلاک_Mod.emplace(modp->name(), modp);
            }
            // Recursive hierarchical module may change its name, so we have to match its origName.
            // Collect values from recursive cloned module as parameters in the top module could be
            // overridden.
            ثابت string actualModName = modp->recursiveClone() ? modp->origName() : modp->name();
            ثابت خودکار defParamIt = m_modParams.ڈھونڈو(actualModName);
            اگر (defParamIt != m_modParams.انتہا()) {
                // modp is the original of parameterized hierarchical block
                برائے (Ast_گرہ* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    اگر (AstVar* ثابت varp = VN_CAST(stmtp, Var)) {
                        اگر (varp->isGParam()) defParamIt->دوسرا.emplace(varp->name(), varp);
                        // Recorded before V3Param resolves the type from the assigned value.
                        ثابت AstBasicDType* ثابت dtypep
                            = VN_CAST(varp->بچہ_DTypep(), BasicDType);
                        اگر (varp->isGParam() && dtypep && dtypep->implicit()) {
                            m_untypedParams.emplace(varp);
                        }
                    }
                }
            }
        }
    }
    منطقی hier_Sub_تسلسل() ثابت { واپس m_hier_Sub_تسلسل; }
    منطقی is_Hier_بلاک(ثابت string& origName) ثابت {
        واپس m_hier_بلاک_Opts_By_Orig_Name.ڈھونڈو(origName) != m_hier_بلاک_Opts_By_Orig_Name.انتہا();
    }
    Ast_گرہ_Module* ڈھونڈو_By_Params(ثابت string& origName, AstPin* پہلا_Pinp,
                                ثابت Ast_گرہ_Module* modp) {
        UASSERT(is_Hier_بلاک(origName), origName << " is not hierarchical block");
        // This module is a hierarchical block. Need to replace it by the --lib-create wrapper.
        ثابت std::pair<HierMapIt, HierMapIt> candidates
            = m_hier_بلاک_Opts_By_Orig_Name.equal_range(origName);
        ثابت خودکار paramsIt = m_modParams.ڈھونڈو(origName);
        UASSERT_OBJ(paramsIt != m_modParams.انتہا(), modp, origName << " must be registered");
        // An untyped parameter takes the type of its value, but libraries are distinguished
        // only by parameter values. This is a pin whose value equals that of a library or the
        // default, but whose type differs.
        ثابت AstPin* typeMismatchPinp = خالی_اشاریہ;
        HierMapIt hierIt;
        برائے (hierIt = candidates.پہلا; hierIt != candidates.دوسرا; ++hierIt) {
            منطقی ملا = سچ;
            سائز_t paramIdx = 0;
            ثابت ParamConstMap& params = m_hierParams[hierIt->دوسرا];
            UASSERT(params.سائز() == hierIt->دوسرا->params().سائز(), "not match");
            برائے (AstPin* pinp = پہلا_Pinp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                اگر (!pinp->exprp()) جاری;
                اگر (ثابت AstVar* ثابت modvarp = pinp->modVarp()) {
                    AstConst* ثابت constp = VN_AS(pinp->exprp(), Const);
                    UASSERT_OBJ(constp, pinp,
                                "parameter for a hierarchical block must have been constified");
                    ثابت خودکار paramIt = paramsIt->دوسرا.ڈھونڈو(modvarp->name());
                    UASSERT_OBJ(paramIt != paramsIt->دوسرا.انتہا(), modvarp, "must be registered");
                    ثابت منطقی untyped = m_untypedParams.گنتی(paramIt->دوسرا);
                    AstConst* ثابت defValuep = VN_CAST(paramIt->دوسرا->valuep(), Const);
                    اگر (defValuep && areSame(constp, defValuep)) {
                        اگر (!untyped || constp->same_قدر_Type(defValuep)) {
                            UINFO(5, "Setting default value of " << constp << " to " << modvarp);
                            جاری;  // Skip this parameter because setting the same value
                        }
                        typeMismatchPinp = pinp;
                    }
                    ثابت خودکار pIt = vlstd::as_const(params).ڈھونڈو(modvarp->name());
                    UINFO(5, "Comparing " << modvarp->name() << " " << constp);
                    اگر (pIt == params.انتہا() || paramIdx >= params.سائز()
                        || !areSame(constp, pIt->دوسرا.get())) {
                        ملا = جھوٹ;
                        توڑو;
                    }
                    اگر (untyped && !constp->same_قدر_Type(pIt->دوسرا.get())) {
                        typeMismatchPinp = pinp;
                        ملا = جھوٹ;
                        توڑو;
                    }
                    UINFO(5, "Matched " << modvarp->name() << " " << constp << " and "
                                        << pIt->دوسرا.get());
                    ++paramIdx;
                }
            }
            اگر (ملا && paramIdx == hierIt->دوسرا->params().سائز()) توڑو;
        }
        اگر (hierIt == candidates.دوسرا && typeMismatchPinp) {
            typeMismatchPinp->v3warn(
                E_UNSUPPORTED, "Unsupported: Untyped parameter "
                                   << typeMismatchPinp->modVarp()->prettyNameQ()
                                   << " of hierarchical block " << Ast_گرہ::prettyNameQ(origName)
                                   << " given equal values of different width or"
                                      " signedness");
            واپس خالی_اشاریہ;
        }
        UASSERT_OBJ(hierIt != candidates.دوسرا, پہلا_Pinp, "No --lib-create wrapper found");
        // parameter settings will be removed in the bottom of caller visitCell().
        ثابت Hier_بلاک_Mod_Map::const_iterator modIt
            = m_hier_بلاک_Mod.ڈھونڈو(hierIt->دوسرا->mangledName());
        UASSERT_OBJ(modIt != m_hier_بلاک_Mod.انتہا(), پہلا_Pinp,
                    hierIt->دوسرا->mangledName() << " is not found");

        ثابت خودکار it = vlstd::as_const(m_hier_بلاک_Mod).ڈھونڈو(hierIt->دوسرا->mangledName());
        اگر (it == m_hier_بلاک_Mod.انتہا()) واپس خالی_اشاریہ;
        واپس it->دوسرا;
    }
    ساکن منطقی areSame(AstConst* pinValuep, AstConst* hierOptParamp) {
        اگر (pinValuep->isString()) {
            واپس pinValuep->عدد().مقصد_String() == hierOptParamp->عدد().مقصد_String();
        }

        اگر (hierOptParamp->isDouble()) {
            دوہرا var;
            اگر (pinValuep->isDouble()) {
                var = pinValuep->عدد().مقصد_Double();
            } ورنہ {  // Cast from integer to real
                V3_نمبر var_عدد{pinValuep, V3_نمبر::Double{}, 0.0};
                var_عدد.opIToRD(pinValuep->عدد());
                var = var_عدد.مقصد_Double();
            }
            واپس V3_نمبر::epsilonEqual(var, hierOptParamp->عدد().مقصد_Double());
        } ورنہ {  // Now integer type is assumed
            // Bitwidth of hierOptParamp is accurate because V3Width already calculated in the
            // previous run. Bitwidth of pinValuep is before width analysis, so pinValuep is casted
            // to hierOptParamp width.
            V3_نمبر var_عدد{pinValuep, hierOptParamp->عدد().چوڑائی()};
            اگر (pinValuep->isDouble()) {  // Need to cast to int
                // Parameter is actually an integral type, but passed value is floating point.
                // Conversion from real to integer uses rounding in V3Width.cpp
                var_عدد.opRToIRoundS(pinValuep->عدد());
            } ورنہ اگر (pinValuep->isSigned()) {
                var_عدد.opExtendS(pinValuep->عدد(), pinValuep->عدد().چوڑائی());
            } ورنہ {
                var_عدد.op_تفویض(pinValuep->عدد());
            }
            V3_نمبر isEq{pinValuep, 1};
            isEq.opEq(var_عدد, hierOptParamp->عدد());
            واپس isEq.isNeqZero();
        }
    }
};

//######################################################################
// Remove parameters from cells and build new modules

جماعت ParamProcessor final {
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
    ثابت VNUser2_اندر_Use m_inuser2;
    ثابت VNUser3_اندر_Use m_inuser3;
    ثابت VNUser4_اندر_Use m_inuser4;
    // User1 used by constant function simulations

    // TYPES
    // Note may have duplicate entries
    استعمال IfaceRefRefs = std::deque<std::pair<AstIfaceRefDType*, AstIfaceRefDType*>>;

    // STATE
    استعمال CloneMap = std::unordered_map<ثابت Ast_گرہ*, Ast_گرہ*>;
    ساخت ModInfo final {
        Ast_گرہ_Module* ثابت m_modp;  // Module with specified name
        CloneMap m_cloneMap;  // Map of old-varp -> new cloned varp
        صریح ModInfo(Ast_گرہ_Module* modp)
            : m_modp{modp} {}
    };
    std::map<ثابت std::string, ModInfo> m_modNameMap;  // Hash of created module flavors by name

    std::map<ثابت std::string, std::string>
        m_longMap;  // Hash of very long names to unique identity number
    std::سیٹ<std::string> m_longSuffixes;  // Digest suffixes used by m_longMap

    // All module names that are loaded from source code
    // Generated modules by this visitor is not included
    VString_سیٹ m_all_Module_نام;

    CloneMap m_originalParams;  // Map between parameters of copied parameteized classes and their
                                // original nodes

    std::map<ثابت std::string, std::string> m_قدر_نام;  // Parameter value text to its name
    std::سیٹ<std::string> m_قدر_Suffixes;  // Digest suffixes used by m_valueNames
    // Number of digest hex digits in a name. With 8 digits, a collision between names made in
    // different runs, such as those of different hierarchical blocks, is unlikely. Within one
    // run, digestSuffix resolves a collision by lengthening the later text's suffix, which can
    // then differ between runs.
    ساکن constexpr std::string::سائز_type DIGEST_DIGITS = 8;

    ثابت Ast_گرہ_Module* m_modp = خالی_اشاریہ;  // Current module being processed

    // Database to get lib-create wrapper that matches parameters in hierarchical Verilation
    Parameterized_Hier_بلاکیں m_hier_بلاکیں;
    // Default parameter values key:parameter name, value:default value (can be nullptr)
    استعمال Default_قدر_Map = std::map<std::string, Ast_گرہ*>;
    // Default parameter values of hierarchical blocks
    std::map<Ast_گرہ_Module*, Default_قدر_Map> m_default_Parameter_اقدار;
    // Module -> a defparam in it or below it, or nullptr (see nestedDefparamp)
    std::map<ثابت Ast_گرہ_Module*, ثابت AstPin*> m_nestedDefparamps;
    VNDeleter m_deleter;  // Used to delay deletion of nodes
    // Class default type paramater dependencies
    std::سمتیہ<std::pair<AstParamTypeDType*, صحیح>> m_classTypeParams;
    std::unordered_map<AstParamTypeDType*, صحیح> m_param_اشاریہ;

    // Guard against infinite recursion in classTypeMatchesDefaultClone slow path
    std::unordered_سیٹ<ثابت AstClass*> m_default_Clone_اندر_پیش_رفت;

    std::سمتیہ<AstClass*> m_specializedClassps;  // Classes specialized since last drained

    // member names cached for fast lookup
    VMemberMap m_memberMap;

    // METHODS

    ساکن خالی make_Small_نام(Ast_گرہ_Module* modp) {
        std::سمتیہ<صحیح> مستعمل_Letter;
        مستعمل_Letter.resize(256);
        // Pass 1, assign first letter to each gparam's name
        برائے (Ast_گرہ* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            اگر (AstVar* ثابت varp = VN_CAST(stmtp, Var)) {
                اگر (varp->isGParam() || varp->isIfaceRef()) {
                    حرف ch = varp->name()[0];
                    ch = std::بڑا_حرف(ch);
                    اگر (ch < 'A' || ch > 'Z') ch = 'Z';
                    varp->user3(مستعمل_Letter[static_cast<صحیح>(ch)] * 256 + ch);
                    مستعمل_Letter[static_cast<صحیح>(ch)]++;
                }
            } ورنہ اگر (AstParamTypeDType* ثابت typep = VN_CAST(stmtp, ParamTypeDType)) {
                ثابت حرف ch = 'T';
                typep->user3(مستعمل_Letter[static_cast<صحیح>(ch)] * 256 + ch);
                مستعمل_Letter[static_cast<صحیح>(ch)]++;
            }
        }
    }
    ساکن string paramSmallName(Ast_گرہ_Module* modp, Ast_گرہ* varp) {
        اگر (varp->user3() <= 1) make_Small_نام(modp);
        صحیح اشاریہ = varp->user3() / 256;
        ثابت حرف ch = varp->user3() & 255;
        string st = cvt_مقصد_Str(ch);
        جب (اشاریہ) {
            st += cvt_مقصد_Str(static_cast<حرف>((اشاریہ % 25) + 'A'));
            اشاریہ /= 26;
        }
        واپس st;
    }

    ساکن منطقی is_Aggregate_Param_قدر(ثابت Ast_گرہ* nodep) {
        واپس VN_IS(nodep, ConsArray) || VN_IS(nodep, ConsStruct);
    }

    ساکن string param_قدر_String(ثابت Ast_گرہ* nodep) {
        اگر (ثابت AstRefDType* ثابت refp = VN_CAST(nodep, RefDType)) {
            nodep = refp->skip_Ref_مقصد_Non_Refp();
        }
        string کلید = nodep->name();
        اگر (ثابت AstIfaceRefDType* ثابت ifrtp = VN_CAST(nodep, IfaceRefDType)) {
            اگر (ifrtp->cellp() && ifrtp->cellp()->modp()) {
                کلید = ifrtp->cellp()->modp()->name();
            } ورنہ اگر (ifrtp->ifacep()) {
                کلید = ifrtp->ifacep()->name();
            } ورنہ {
                nodep->v3fatalSrc("Can't parameterize interface without module name");
            }
        } ورنہ اگر (ثابت Ast_گرہ_UOr_Struct_DType* ثابت dtypep
                   = VN_CAST(nodep, گرہ_UOr_Struct_DType)) {
            کلید += " ";
            کلید += dtypep->verilogKwd();
            کلید += " {";
            برائے (ثابت Ast_گرہ* memberp = dtypep->membersp(); memberp;
                 memberp = memberp->nextp()) {
                کلید += param_قدر_String(memberp);
                کلید += ";";
            }
            کلید += "}";
        } ورنہ اگر (ثابت AstMemberDType* ثابت dtypep = VN_CAST(nodep, MemberDType)) {
            کلید += " ";
            کلید += param_قدر_String(dtypep->subDTypep());
        } ورنہ اگر (ثابت AstBasicDType* ثابت dtypep = VN_CAST(nodep, BasicDType)) {
            اگر (dtypep->isSigned()) کلید += " signed";
            اگر (dtypep->isRanged()) {
                کلید += "[" + cvt_مقصد_Str(dtypep->بائیں()) + ":" + cvt_مقصد_Str(dtypep->دائیں()) + "]";
            }
        } ورنہ اگر (ثابت AstPackArrayDType* ثابت dtypep = VN_CAST(nodep, PackArrayDType)) {
            کلید += "[";
            کلید += cvt_مقصد_Str(dtypep->بائیں());
            کلید += ":";
            کلید += cvt_مقصد_Str(dtypep->دائیں());
            کلید += "] ";
            کلید += param_قدر_String(dtypep->subDTypep());
        } ورنہ اگر (ثابت Ast_نمونہ* ثابت patp = VN_CAST(nodep, نمونہ)) {
            // A parameter override pattern, keyed before the specialized module types it
            کلید += "'{";
            برائے (ثابت AstPatMember* memp = VN_AS(patp->itemsp(), PatMember); memp;
                 memp = VN_AS(memp->nextp(), PatMember)) {
                اگر (memp->isDefault()) کلید += "default";
                اگر (ثابت Ast_متن* ثابت textp = VN_CAST(memp->keyp(), متن_2)) {
                    کلید += textp->متن();
                } ورنہ اگر (memp->keyp()) {
                    کلید += param_قدر_String(memp->keyp());
                }
                کلید += ":";
                اگر (memp->repp()) کلید += param_قدر_String(memp->repp()) + "x";
                برائے (ثابت Ast_گرہ_اظہاریہ* valuep = memp->lhssp(); valuep;
                     valuep = VN_AS(valuep->nextp(), گرہ_اظہاریہ)) {
                    کلید += param_قدر_String(valuep) + ";";
                }
                کلید += ",";
            }
            کلید += "}";
        } ورنہ اگر (ثابت AstConsArray* ثابت consArrayp = VN_CAST(nodep, ConsArray)) {
            // Include the indices and the default, as with a default the items may be sparse
            کلید += "{";
            برائے (ثابت Ast_Cons_Array_شے* itemp = consArrayp->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), Cons_Array_شے)) {
                کلید += cvt_مقصد_Str(itemp->اشاریہ()) + ":" + param_قدر_String(itemp->valuep()) + ",";
            }
            اگر (consArrayp->defaultp()) {
                کلید += "default:" + param_قدر_String(consArrayp->defaultp()) + ",";
            }
            کلید += "}";
        } ورنہ اگر (ثابت AstConsStruct* ثابت consp = VN_CAST(nodep, ConsStruct)) {
            کلید += "{";
            برائے (ثابت Ast_Cons_Struct_شے* itemp = consp->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), Cons_Struct_شے)) {
                کلید += param_قدر_String(itemp->valuep());
                کلید += ",";
            }
            کلید += "}";
        } ورنہ اگر (ثابت AstClassRefDType* ثابت classRefp = VN_CAST(nodep, ClassRefDType)) {
            // For parameterized class types, use the original class name (without specialization
            // suffix) plus the actual type parameter values. This ensures equivalent class types
            // get the same string representation regardless of which AST node is used.
            اگر (classRefp->classp()) {
                ثابت string& className = classRefp->classp()->name();
                ثابت string& origName = classRefp->classp()->origName();
                ثابت منطقی isSpecialized = (className != origName);
                UINFO(9, "paramValueString ClassRefDType: name="
                             << className << " origName=" << origName
                             << " isSpecialized=" << isSpecialized
                             << " hasParams=" << (classRefp->paramsp() ? "Y" : "N")
                             << " classHasGParam=" << classRefp->classp()->hasGParam());

                اگر (classRefp->paramsp()) {
                    // ClassRefDType should have been deparameterized (paramsp
                    // consumed) before cellPinCleanup calls paramValueString.
                    classRefp->v3fatalSrc(  // LCOV_EXCL_LINE
                        "ClassRefDType still has paramsp in paramValueString");
                } ورنہ اگر (isSpecialized) {
                    // Already specialized class (e.g., c1__Tz1_TBz1) - use full name
                    // This ensures different specializations are distinguished
                    کلید = className;
                } ورنہ {
                    // Unspecialized class with no params - use origName
                    کلید = origName;
                }
            } ورنہ {
                // classp() should always be set; unresolved class refs
                // would have errored in LinkDot.
                classRefp->v3fatalSrc(  // LCOV_EXCL_LINE
                    "ClassRefDType has null classp in paramValueString");
            }
        } ورنہ اگر (ثابت AstUnpackArrayDType* ثابت dtypep = VN_CAST(nodep, UnpackArrayDType)) {
            // Name containers by their elements as above, not by prettyDTypeName(), which names
            // classes by parameter values that may be yet to be resolved
            کلید = paramElemString(dtypep->subDTypep()) + "$" + cvt_مقصد_Str(dtypep->declRange());
        } ورنہ اگر (ثابت Ast_قطار_DType* ثابت dtypep = VN_CAST(nodep, قطار_DType)) {
            کلید = paramElemString(dtypep->subDTypep()) + "$[$";
            اگر (dtypep->boundConst()) کلید += ":" + cvt_مقصد_Str(dtypep->boundConst());
            کلید += "]";
        } ورنہ اگر (ثابت AstDynArrayDType* ثابت dtypep = VN_CAST(nodep, DynArrayDType)) {
            کلید = paramElemString(dtypep->subDTypep()) + "$[]";
        } ورنہ اگر (ثابت AstAssocArrayDType* ثابت dtypep = VN_CAST(nodep, AssocArrayDType)) {
            کلید = paramElemString(dtypep->subDTypep()) + "$["
                  + paramElemString(dtypep->کلید_DTypep()) + "]";
        } ورنہ اگر (ثابت AstWildcardArrayDType* ثابت dtypep
                   = VN_CAST(nodep, WildcardArrayDType)) {
            کلید = paramElemString(dtypep->subDTypep()) + "$[*]";
        } ورنہ اگر (ثابت AstEnumDType* ثابت dtypep = VN_CAST(nodep, EnumDType)) {
            // With the items, as prettyDTypeName() omits them, yet a specialization of a class
            // may declare an enumeration of the same name with other values
            کلید += " enum {";
            برائے (ثابت Ast_Enum_شے* itemp = dtypep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), Enum_شے)) {
                ثابت AstConst* ثابت constp = VN_CAST(itemp->valuep(), Const);
                کلید += itemp->name() + "=" + (constp ? constp->عدد().ascii() : "?") + ";";
            }
            کلید += "}";
        } ورنہ اگر (ثابت Ast_گرہ_DType* ثابت dtypep = VN_CAST(nodep, گرہ_DType)) {
            کلید += dtypep->prettyDTypeName(سچ);
        }
        UASSERT_OBJ(!کلید.خالی_خانے(), nodep, "Parameter yielded no value string");
        واپس کلید;
    }
    // As paramValueString(), for the element or key type of a container
    ساکن string paramElemString(ثابت Ast_گرہ_DType* dtypep) {
        // Unlike at the top of a value, a class may be yet to be specialized, so just name it
        ثابت AstClassRefDType* ثابت classRefp
            = VN_CAST(dtypep->skip_Ref_مقصد_Non_Refp(), ClassRefDType);
        اگر (classRefp && classRefp->paramsp()) واپس classRefp->prettyDTypeName(سچ);
        واپس param_قدر_String(dtypep);
    }

    // Return a name suffix for 'text' from its SHA-512 digest. Hierarchical blocks are
    // Verilated in separate runs, where a counter would restart, but the digest is the same in
    // every run. Use the shortest digest prefix of at least DIGEST_DIGITS digits that is not
    // already in 'usedr', and add it to 'usedr'.
    ساکن string digestSuffix(ثابت string& متن, std::سیٹ<string>& usedr) {
        ثابت string hex = VHashSha512{متن}.digestHex();
        // Force collisions of the prefixes -- for testing only
        string::سائز_type digits = v3Global.opt.debugCollision() ? 1 : DIGEST_DIGITS;
        جب (digits < hex.سائز() && !usedr.داخل_کرو(hex.substr(0, digits)).دوسرا) ++digits;
        واپس hex.substr(0, digits);
    }
    string param_قدر_نمبر(Ast_گرہ* nodep) {
        // All call sites resolve through skipRefToNonRefp() or pass non-DType
        // nodes, so nodep should never be a bare RefDType here.
        اگر (VN_IS(nodep, RefDType)) {  // LCOV_EXCL_LINE
            nodep->v3fatalSrc("Unexpected RefDType in paramValueNumber");  // LCOV_EXCL_LINE
        }
        // Name the value by its text, which is the same for equal values or types in every run.
        // V3Hasher is unsuitable, as it hashes node pointers, which can differ for equal types.
        // V3Hash of a string is unsuitable, as std::hash varies between C++ libraries.
        ثابت string متن = param_قدر_String(nodep);
        ثابت خودکار pair = m_قدر_نام.emplace(متن, "");
        اگر (pair.دوسرا) pair.پہلا->دوسرا = "z" + digestSuffix(متن, m_قدر_Suffixes);
        واپس pair.پہلا->دوسرا;
    }
    string moduleCalcName(ثابت Ast_گرہ_Module* srcModp, ثابت string& longname) {
        string newname = longname;
        اگر (longname.لمبائی_2() > 30) {
            ثابت خودکار pair = m_longMap.emplace(longname, "");
            اگر (pair.دوسرا) {
                // We use all upper case above, so lower here can't conflict
                pair.پہلا->دوسرا
                    = srcModp->name() + "__pi" + digestSuffix(longname, m_longSuffixes);
            }
            newname = pair.پہلا->دوسرا;
        }
        UINFO(4, "Name: " << srcModp->name() << "->" << longname << "->" << newname);
        واپس newname;
    }
    ساکن Ast_گرہ_DType* arraySubDTypep(Ast_گرہ_DType* nodep) {
        // If an unpacked array, return the subDTypep under it
        اگر (ثابت AstUnpackArrayDType* ثابت adtypep = VN_CAST(nodep, UnpackArrayDType)) {
            واپس adtypep->subDTypep();
        }
        // We have not resolved parameter of the child yet, so still
        // have BracketArrayDType's. We'll presume it'll end up as assignment
        // compatible (or V3Width will complain).
        اگر (ثابت AstBracketArrayDType* ثابت adtypep = VN_CAST(nodep, BracketArrayDType)) {
            واپس adtypep->subDTypep();
        }
        واپس خالی_اشاریہ;
    }
    // Peel all unpacked-array layers to reach the innermost subDTypep.
    // Used for multi-dim iface arrays where port dtype is nested UnpackArrayDType.
    ساکن Ast_گرہ_DType* arraySubDTypeDeepp(Ast_گرہ_DType* nodep) {
        جب (Ast_گرہ_DType* ثابت subp = arraySubDTypep(nodep)) nodep = subp;
        واپس nodep;
    }
    ساکن منطقی isString(Ast_گرہ_DType* nodep) {
        اگر (AstBasicDType* ثابت basicp = VN_CAST(nodep->skip_Ref_مقصد_Non_Refp(), BasicDType))
            واپس basicp->isString();
        واپس جھوٹ;
    }
    خالی collectPins(CloneMap* clonemapp, Ast_گرہ_Module* modp, منطقی originalIsCopy) {
        // Grab all I/O so we can remap our pins later
        برائے (Ast_گرہ* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ثابت Ast_گرہ* originalParamp = خالی_اشاریہ;
            اگر (AstVar* ثابت varp = VN_CAST(stmtp, Var)) {
                اگر (varp->isIO() || varp->isGParam() || varp->isIfaceRef()) {
                    // Cloning saved a pointer to the new node for us, so just follow that link.
                    originalParamp = varp->clonep();
                }
            } ورنہ اگر (AstParamTypeDType* ثابت ptp = VN_CAST(stmtp, ParamTypeDType)) {
                اگر (ptp->isGParam()) originalParamp = ptp->clonep();
            }
            اگر (originalIsCopy) originalParamp = m_originalParams[originalParamp];
            اگر (originalParamp) clonemapp->emplace(originalParamp, stmtp);
        }
    }
    خالی relinkPins(ثابت CloneMap* clonemapp, AstPin* startpinp) {
        برائے (AstPin* pinp = startpinp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            اگر (pinp->modVarp()) {
                // Find it in the clone structure
                // UINFO(8,"Clone find 0x"<<hex<<(uint32_t)pinp->modVarp());
                ثابت خودکار cloneiter = clonemapp->ڈھونڈو(pinp->modVarp());
                UASSERT_OBJ(cloneiter != clonemapp->انتہا(), pinp,
                            "Couldn't find pin in clone list");
                pinp->modVarp(VN_AS(cloneiter->دوسرا, Var));
            } ورنہ اگر (pinp->modPTypep()) {
                ثابت خودکار cloneiter = clonemapp->ڈھونڈو(pinp->modPTypep());
                UASSERT_OBJ(cloneiter != clonemapp->انتہا(), pinp,
                            "Couldn't find pin in clone list");
                pinp->modPTypep(VN_AS(cloneiter->دوسرا, ParamTypeDType));
            } ورنہ {
                pinp->v3fatalSrc("Not linked?");
            }
        }
    }
    خالی relinkPinsByName(AstPin* startpinp, Ast_گرہ_Module* modp) {
        std::map<ثابت string, AstVar*> name_مقصد_Pin;
        برائے (Ast_گرہ* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            اگر (AstVar* ثابت varp = VN_CAST(stmtp, Var)) {
                اگر (varp->isIO() || varp->isGParam() || varp->isIfaceRef()) {
                    name_مقصد_Pin.emplace(varp->name(), varp);
                }
            }
        }
        برائے (AstPin* pinp = startpinp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            اگر (ثابت AstVar* ثابت varp = pinp->modVarp()) {
                ثابت خودکار varIt = vlstd::as_const(name_مقصد_Pin).ڈھونڈو(varp->name());
                UASSERT_OBJ(varIt != name_مقصد_Pin.انتہا(), varp,
                            "Not found in " << modp->prettyNameQ());
                pinp->modVarp(varIt->دوسرا);
            }
        }
    }
    ساکن منطقی hasDescendantDefparams(ثابت Ast_گرہ_Module* modp,
                                       std::سیٹ<ثابت Ast_گرہ_Module*>& دیکھا) {
        اگر (!دیکھا.داخل_کرو(modp).دوسرا) واپس جھوٹ;
        برائے (Ast_گرہ* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstCell* ثابت cellp = VN_CAST(stmtp, Cell);
            اگر (!cellp) جاری;
            برائے (AstPin* pinp = cellp->paramsp(); pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                اگر (!pinp->param_راستہ().خالی_خانے()) {
                    دیکھا.erase(modp);
                    واپس سچ;
                }
            }
            اگر (Ast_گرہ_Module* ثابت بچہ_Modp = cellp->modp()) {
                اگر (hasDescendantDefparams(بچہ_Modp, دیکھا)) {
                    دیکھا.erase(modp);
                    واپس سچ;
                }
            }
        }
        دیکھا.erase(modp);
        واپس جھوٹ;
    }
    ساکن منطقی hasDescendantDefparams(ثابت Ast_گرہ_Module* modp) {
        std::سیٹ<ثابت Ast_گرہ_Module*> دیکھا;
        واپس hasDescendantDefparams(modp, دیکھا);
    }
    // Return a defparam in the module or below it. Unlike hasDescendantDefparams, also look
    // inside generate constructs. Memoized, so each module is searched once.
    ثابت AstPin* nestedDefparamp(ثابت Ast_گرہ_Module* modp) {
        ثابت خودکار pair = m_nestedDefparamps.emplace(modp, خالی_اشاریہ);
        اگر (!pair.دوسرا) واپس pair.پہلا->دوسرا;  // Searched, or being searched
        ثابت AstPin* foundp = خالی_اشاریہ;
        modp->exists([&](ثابت AstCell* cellp) {
            برائے (ثابت AstPin* pinp = cellp->paramsp(); pinp && !foundp;
                 pinp = VN_AS(pinp->nextp(), Pin)) {
                اگر (!pinp->param_راستہ().خالی_خانے()) foundp = pinp;
            }
            اگر (!foundp && cellp->modp()) foundp = nestedDefparamp(cellp->modp());
            واپس foundp != خالی_اشاریہ;
        });
        pair.پہلا->دوسرا = foundp;
        واپس foundp;
    }
    // Check if parameter setting during instantiation is simple enough for hierarchical Verilation
    خالی checkSupportedParam(Ast_گرہ_Module* modp, AstPin* pinp) ثابت {
        // ConsArray is not supported because that can not be set via -G
        // option.
        اگر (pinp->modVarp()) {
            منطقی supported = جھوٹ;
            اگر (ثابت AstConst* ثابت constp = VN_CAST(pinp->exprp(), Const)) {
                supported = سچ;
                اگر (constp->عدد().isString()
                    && !V3_Hier_بلاک::stringParamPassable(constp->عدد().مقصد_String())) {
                    pinp->v3warn(E_UNSUPPORTED, "Unsupported: String value of hierarchical block"
                                                " parameter with newline, double quote, '/*',"
                                                " or whitespace before '//'");
                }
            }
            اگر (!supported) {
                pinp->v3error(
                    Ast_گرہ::prettyNameQ(modp->origName())
                    << " has hier_block metacomment, hierarchical Verilation"
                    << " supports only integer/floating point/string and type param parameters");
            }
        } ورنہ اگر (ثابت AstParamTypeDType* ثابت typep = pinp->modPTypep()) {
            // Libraries are found by comparing values only
            pinp->v3warn(E_UNSUPPORTED, "Unsupported: Setting type parameter "
                                            << typep->prettyNameQ() << " of hierarchical block "
                                            << Ast_گرہ::prettyNameQ(modp->origName()));
        }
    }
    منطقی moduleExists(ثابت string& modName) ثابت {
        اگر (m_all_Module_نام.ڈھونڈو(modName) != m_all_Module_نام.انتہا()) واپس سچ;
        اگر (m_modNameMap.ڈھونڈو(modName) != m_modNameMap.انتہا()) واپس سچ;
        واپس جھوٹ;
    }

    string parameterized_Hier_بلاک_Name(Ast_گرہ_Module* modp, AstPin* paramPinsp) {
        // Create a unique name in the following steps
        //  - Make a long name that includes all parameters, that appear
        //    in the alphabetical order.
        //  - Hash the long name to get valid Verilog symbol
        UASSERT_OBJ(modp->hier_بلاک(), modp, "should be used for hierarchical block");

        std::map<string, Ast_گرہ*> pins;

        AstPin* pinp = paramPinsp;
        جب (pinp) {
            checkSupportedParam(modp, pinp);
            اگر (ثابت AstVar* ثابت varp = pinp->modVarp()) {
                اگر (!pinp->exprp()) جاری;
                اگر (varp->isGParam()) pins.emplace(varp->name(), pinp->exprp());
            } ورنہ اگر (VN_IS(pinp->exprp(), BasicDType) || VN_IS(pinp->exprp(), گرہ_DType)) {
                pins.emplace(pinp->name(), pinp->exprp());
            }
            pinp = VN_AS(pinp->nextp(), Pin);
        }

        ثابت خودکار pair = m_default_Parameter_اقدار.emplace(
            std::piecewise_construct, std::forward_as_tuple(modp), std::forward_as_tuple());
        اگر (pair.دوسرا) {  // Not cached yet, so check parameters
            // A separately compiled library cannot receive defparams aimed into its body
            اگر (ثابت AstPin* ثابت defparamp = nestedDefparamp(modp)) {
                defparamp->v3warn(E_UNSUPPORTED, "Unsupported: defparam inside hierarchical block "
                                                     << Ast_گرہ::prettyNameQ(modp->origName()));
            }
            // Using map with key=string so that we can scan it in deterministic order
            Default_قدر_Map params;
            برائے (Ast_گرہ* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                اگر (AstVar* ثابت varp = VN_CAST(stmtp, Var)) {
                    اگر (varp->isGParam()) {
                        AstConst* ثابت constp = VN_CAST(varp->valuep(), Const);
                        // constp can be nullptr if the parameter is not used to instantiate sub
                        // module. varp->valuep() is not constified yet in the case.
                        // nullptr means that the parameter is using some default value.
                        params.emplace(varp->name(), constp);
                    }
                } ورنہ اگر (AstParamTypeDType* ثابت p = VN_CAST(stmtp, ParamTypeDType)) {
                    Ast_گرہ* ثابت dtypep = static_cast<Ast_گرہ*>(p->skipRefp());
                    params.emplace(p->name(), dtypep);
                }
            }
            pair.پہلا->دوسرا = std::move(params);
        }
        ثابت خودکار paramsIt = pair.پہلا;
        اگر (paramsIt->دوسرا.خالی_خانے()) واپس modp->origName();  // modp has no parameter

        string longname = modp->origName();
        برائے (خودکار&& default_قدر : paramsIt->دوسرا) {
            ثابت خودکار pinIt = pins.ڈھونڈو(default_قدر.پہلا);
            // If the pin does not have a value assigned, use the default one.
            ثابت Ast_گرہ* ثابت nodep = pinIt == pins.انتہا() ? default_قدر.دوسرا : pinIt->دوسرا;
            // This longname is not valid as verilog symbol, but ok, because it will be hashed
            longname += "_" + default_قدر.پہلا + "=";
            // constp can be nullptr

            اگر (ثابت AstConst* ثابت p = VN_CAST(nodep, Const)) {
                // Treat modules parameterized with the same values but with different type as the
                // same.
                longname += p->عدد().ascii(جھوٹ);
            } ورنہ اگر (nodep) {
                std::stringstream type;
                V3EmitV::verilog_For_درخت(nodep, type);
                longname += type.str();
            }
        }
        UINFO(9, "       module params longname: " << longname);

        ثابت خودکار iter = m_longMap.ڈھونڈو(longname);
        اگر (iter != m_longMap.انتہا()) واپس iter->دوسرا;  // Already calculated

        VHashSha512 hash;
        // Calculate hash using longname
        // The hash is used as the module suffix to find a module name that is unique in the design
        hash.داخل_کرو(longname);
        جب (سچ) {
            // Copy VHashSha512 just in case of hash collision
            VHashSha512 hashStrGen = hash;
            // Hex string must be a safe suffix for any symbol
            ثابت string hashStr = hashStrGen.digestHex();
            برائے (string::سائز_type i = 1; i < hashStr.سائز(); ++i) {
                string newName = modp->origName();
                // Don't use '__' not to be encoded when this module is loaded later by Verilator
                اگر (newName.at(newName.سائز() - 1) != '_') newName += '_';
                newName += hashStr.substr(0, i);
                اگر (!moduleExists(newName)) {
                    m_longMap.emplace(longname, newName);
                    واپس newName;
                }
            }
            // Hash collision. maybe just v3error is practically enough
            hash.داخل_کرو(V3Os::trueRandom(64));
        }
    }
    خالی replaceRefsRecurse(Ast_گرہ* ثابت nodep, ثابت AstClass* ثابت oldClassp,
                            AstClass* ثابت newClassp) {
        // Self references linked in the first pass of V3LinkDot.cpp should point to the default
        // instance.
        اگر (AstClassRefDType* ثابت classRefp = VN_CAST(nodep, ClassRefDType)) {
            اگر (classRefp->classp() == oldClassp) classRefp->classp(newClassp);
        } ورنہ اگر (AstClassOrPackageRef* ثابت classRefp = VN_CAST(nodep, ClassOrPackageRef)) {
            اگر (classRefp->classOrPackageSkipp() == oldClassp)
                classRefp->classOrPackagep(newClassp);
        } ورنہ اگر (AstTypedef* ثابت typedefp = VN_CAST(nodep, Typedef)) {
            // Update typedefs that refer to the old class to point to the new class
            اگر (typedefp->subDTypep()) {
                اگر (AstClassRefDType* ثابت classRefp
                    = VN_CAST(typedefp->subDTypep(), ClassRefDType)) {
                    اگر (classRefp->classp() == oldClassp) { classRefp->classp(newClassp); }
                }
            }
        } ورنہ اگر (Ast_گرہ_FTask_Ref* ثابت ftaskRefp = VN_CAST(nodep, گرہ_FTask_Ref)) {
            // Also update FuncRef/TaskRef packagep to point to new class
            // This fixes static method calls through typedefs in parameterized classes
            اگر (ftaskRefp->classOrPackagep() == oldClassp) ftaskRefp->classOrPackagep(newClassp);
            // Also update taskp if it points to a function in the old class.
            // AstNodeFTask::classOrPackagep() (op2) holds a parse-time
            // Dot/ClassOrPackageRef for extern declarations, which is deleted
            // in LinkDot::moveExternFuncDecl before V3Param runs. For inline
            // class methods op2 is nullptr. So this should never match.
            اگر (Ast_گرہ_FTask* ثابت oldTaskp = ftaskRefp->taskp()) {
                اگر (oldTaskp->classOrPackagep() == oldClassp) {
                    oldTaskp->v3fatalSrc(  // LCOV_EXCL_LINE
                        "FTask classOrPackagep unexpectedly matches old class "
                        << oldClassp->prettyNameQ());
                }
            }
        }

        اگر (nodep->op1p()) replaceRefsRecurse(nodep->op1p(), oldClassp, newClassp);
        اگر (nodep->op2p()) replaceRefsRecurse(nodep->op2p(), oldClassp, newClassp);
        اگر (nodep->op3p()) replaceRefsRecurse(nodep->op3p(), oldClassp, newClassp);
        اگر (nodep->op4p()) replaceRefsRecurse(nodep->op4p(), oldClassp, newClassp);
        اگر (nodep->nextp()) replaceRefsRecurse(nodep->nextp(), oldClassp, newClassp);
    }

    // Helper visitor to update VarXRefs to use variables from specialized interfaces.
    // When a module with interface ports is cloned and the port's interface is remapped
    // to a specialized version, VarXRefs that access members of the old interface need
    // to be updated to reference the corresponding members in the new interface.
    جماعت VarXRefRelinkVisitor final : عوامی VNVisitor {
        Ast_گرہ_Module* m_modp;  // The cloned module we're updating
        std::unordered_map<AstVar*, Ast_گرہ_Module*> m_varModuleMap;  // Cache var->module lookups

    عوامی:
        صریح VarXRefRelinkVisitor(Ast_گرہ_Module* newModp)
            : m_modp{newModp} {
            iterate(newModp);
        }

    نجی:
        // Find which module a variable belongs to, using cache to avoid repeated backp() walks
        Ast_گرہ_Module* ڈھونڈو_Var_Module(AstVar* varp) {
            ثابت خودکار it = m_varModuleMap.ڈھونڈو(varp);
            اگر (it != m_varModuleMap.انتہا()) واپس it->دوسرا;
            Ast_گرہ_Module* varModp = خالی_اشاریہ;
            برائے (Ast_گرہ* np = varp; np; np = np->backp()) {
                اگر (Ast_گرہ_Module* ثابت modp = VN_CAST(np, گرہ_Module)) {
                    varModp = modp;
                    توڑو;
                }
            }
            m_varModuleMap[varp] = varModp;
            واپس varModp;
        }

        خالی دورہ(AstVarXRef* nodep) override {
            AstVar* ثابت varp = nodep->varp();
            اگر (!varp) {
                iterateChildren(nodep);
                واپس;
            }

            // Get the dotted prefix (port name) from the VarXRef
            // dotted() format: "portname" or "portname.subpath" or empty
            ثابت string& dotted = nodep->dotted();
            اگر (dotted.خالی_خانے()) {
                iterateChildren(nodep);
                واپس;
            }

            ثابت سائز_t dotPos = dotted.ڈھونڈو('.');
            ثابت string پورٹ_Name = (dotPos == string::npos) ? dotted : dotted.substr(0, dotPos);
            اگر (پورٹ_Name.خالی_خانے()) {
                iterateChildren(nodep);
                واپس;
            }

            // Find the interface port variable in the cloned module
            AstVar* پورٹ_Varp = خالی_اشاریہ;
            برائے (Ast_گرہ* stmtp = m_modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                اگر (AstVar* ثابت varChkp = VN_CAST(stmtp, Var)) {
                    اگر (varChkp->name() == پورٹ_Name && varChkp->isIfaceRef()) {
                        پورٹ_Varp = varChkp;
                        توڑو;
                    }
                }
            }
            اگر (!پورٹ_Varp) {
                iterateChildren(nodep);
                واپس;
            }

            // Get the interface module from the port's dtype
            AstIfaceRefDType* ثابت irefp = VN_CAST(پورٹ_Varp->subDTypep(), IfaceRefDType);
            اگر (!irefp) {
                iterateChildren(nodep);
                واپس;
            }

            Ast_گرہ_Module* ثابت newIfacep = irefp->iface_بذریعہ_Cellp();
            اگر (!newIfacep) {
                iterateChildren(nodep);
                واپس;
            }

            // Find which module the variable currently belongs to (cached)
            Ast_گرہ_Module* ثابت varModp = ڈھونڈو_Var_Module(varp);

            // If variable is in a different module than the port's interface, remap it
            اگر (varModp && varModp != newIfacep) {
                برائے (Ast_گرہ* stmtp = newIfacep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    اگر (AstVar* ثابت newVarp = VN_CAST(stmtp, Var)) {
                        اگر (newVarp->name() == varp->name()) {
                            UINFO(9, "VarXRef relink " << varp->name() << " in " << varModp->name()
                                                       << " -> " << newIfacep->name());
                            nodep->varp(newVarp);
                            توڑو;
                        }
                    }
                }
            }
            iterateChildren(nodep);
        }
        خالی دورہ(Ast_گرہ* nodep) override { iterateChildren(nodep); }
    };

    // Retarget the captured references in newModp to its specialized interfaces.
    خالی fixupCrossModuleRefDTypes(Ast_گرہ_Module* newModp) {
        اگر (!V3LinkDotIfaceCapture::enabled()) واپس;
        UINFO(9, "iface capture clone fixup: " << newModp->prettyNameQ());
        صحیح fixed_گنتی = 0;
        // References in a class nested in newModp (e.g. a covergroup) are included.
        newModp->foreach([&](AstRefDType* refp) {
            ثابت VIface_Capture_لیبل* ثابت tagp = V3LinkDotIfaceCapture::capture_لیبل(refp);
            اگر (!tagp) واپس;

            Ast_گرہ_Module* ثابت correctModp
                = V3LinkDotIfaceCapture::follow_Cell_راستہ(newModp, tagp->m_cell_راستہ);
            UINFO(9, "  path fixup: " << refp << " cellPath='" << tagp->m_cell_راستہ << "' -> "
                                      << (correctModp ? correctModp->name() : "<null>"));
            اگر (!correctModp || correctModp->dead()) واپس;
            اگر (correctModp->parameterizedTemplate()) واپس;

            منطقی fixed = جھوٹ;
            اگر (refp->typedefp()) {
                اگر (AstTypedef* ثابت newTdp = V3LinkDotIfaceCapture::ڈھونڈو_Typedef_اندر_Module(
                        correctModp, refp->typedefp()->name())) {
                    refp->typedefp(newTdp);
                    اگر (newTdp->subDTypep()) refp->refDTypep(newTdp->subDTypep());
                    fixed = سچ;
                }
            }
            اگر (!fixed && refp->refDTypep()) {
                اگر (Ast_گرہ_DType* ثابت newDtp = V3LinkDotIfaceCapture::ڈھونڈو_DType_اندر_Module(
                        correctModp, refp->refDTypep()->name(), refp->refDTypep()->type())) {
                    refp->refDTypep(newDtp);
                    fixed = سچ;
                }
            }
            اگر (fixed) ++fixed_گنتی;
        });
        V3Stats::جوڑو_Stat_مجموع("IfaceCapture, Ledger fixups in V3Param", fixed_گنتی);
    }

    // Return true on success, false on error
    منطقی deepCloneModule(Ast_گرہ_Module* srcModp, Ast_گرہ* ifErrorp, AstPin* paramsp,
                         ثابت string& newname, ثابت IfaceRefRefs& ifaceRefRefs) {
        // Deep clone of new module
        // Note all module internal variables will be re-linked to the new modules by clone
        // However links outside the module (like on the upper cells) will not.
        Ast_گرہ_Module* newModp;
        اگر (srcModp->user3p()) {
            newModp = VN_CAST(srcModp->user3p()->clone_درخت(جھوٹ), گرہ_Module);
        } ورنہ {
            newModp = srcModp->clone_درخت(جھوٹ);
        }
        // AstPin normally retains links to external module formals across cloning. For a cloned
        // interface, relink pins whose formals were cloned with the interface while clonep() is
        // still valid, so nested parameterized classes use the cloned interface parameters.
        اگر (AstIface* ثابت newIfacep = VN_CAST(newModp, Iface)) {
            newIfacep->foreach([](AstPin* pinp) { pinp->cloneRelinkGen(); });
        }

        // Mark the source module as a parameterized template now that a specialized
        // clone exists.  This suppresses width/type errors on the unresolved template
        // during widthParamsEdit (which runs before V3LinkDot sets dead()).
        srcModp->parameterizedTemplate(سچ);
        // The clone is a specialized instance, not a template.  Clear the flag in
        // case it was inherited from a prior cloneTree (when srcModp was already
        // marked by an earlier specialization).
        newModp->parameterizedTemplate(جھوٹ);

        newModp->name(newname);
        newModp->user2(جھوٹ);  // We need to re-recurse this module once changed
        newModp->recursive(جھوٹ);
        newModp->recursiveClone(جھوٹ);
        newModp->hasGParam(جھوٹ);
        // Recursion may need level cleanups
        اگر (newModp->level() <= m_modp->level()) newModp->level(m_modp->level() + 1);
        اگر ((newModp->level() - srcModp->level()) >= (v3Global.opt.module_Recursion_گہرائی() - 2)) {
            ifErrorp->v3error("Exceeded maximum --module-recursion-depth of "
                              << v3Global.opt.module_Recursion_گہرائی());
            VL_DO_DANGLING(newModp->delete_درخت(), newModp);
            واپس جھوٹ;
        }

        اگر (AstClass* ثابت newClassp = VN_CAST(newModp, Class)) {
            replaceRefsRecurse(newModp->stmtsp(), newClassp, VN_AS(srcModp, Class));
        }

        // Keep tree sorted by level. Note: Different parameterizations of the same recursive
        // module end up with the same level, which we will need to fix up at the end, as we do not
        // know up front how recursive modules are expanded, and a later expansion might re-use an
        // earlier expansion (see t_recursive_module_bug_2).
        Ast_گرہ* insertp = srcModp;
        جب (VN_IS(insertp->nextp(), گرہ_Module)
               && VN_AS(insertp->nextp(), گرہ_Module)->level() <= newModp->level()) {
            insertp = insertp->nextp();
        }
        insertp->جوڑو_اگلا_یہاں(newModp);

        m_modNameMap.emplace(newModp->name(), ModInfo{newModp});
        ثابت خودکار iter = m_modNameMap.ڈھونڈو(newname);
        CloneMap* ثابت clonemapp = &(iter->دوسرا.m_cloneMap);
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
        منطقی needRelinkByName = جھوٹ;
        اگر (paramsp) {
            برائے (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                اگر (pinp->modVarp() && clonemapp->ڈھونڈو(pinp->modVarp()) == clonemapp->انتہا()) {
                    needRelinkByName = سچ;
                    توڑو;
                }
            }
        }
        اگر (needRelinkByName) {
            relinkPinsByName(paramsp, newModp);
        } ورنہ {
            relinkPins(clonemapp, paramsp);
        }

        // Fix any interface references
        برائے (خودکار it = ifaceRefRefs.cbegin(); it != ifaceRefRefs.cend(); ++it) {
            ثابت AstIfaceRefDType* ثابت پورٹ_Irefp = it->پہلا;
            ثابت AstIfaceRefDType* ثابت pinIrefp = it->دوسرا;
            AstIfaceRefDType* ثابت cloneIrefp = پورٹ_Irefp->clonep();
            UINFO(8, "     IfaceOld " << پورٹ_Irefp);
            UINFO(8, "     IfaceTo  " << pinIrefp);
            UASSERT_OBJ(cloneIrefp, پورٹ_Irefp, "parameter clone didn't hit AstIfaceRefDType");
            UINFO(8, "     IfaceClo " << cloneIrefp);
            cloneIrefp->ifacep(pinIrefp->iface_بذریعہ_Cellp());
            UINFO(8, "     IfaceNew " << cloneIrefp);
        }

        // Fix VarXRefs that reference variables in old interfaces.
        // Now that interface port dtypes have been updated above, we can use them
        // to find the correct interface for each VarXRef.
        اگر (!ifaceRefRefs.خالی_خانے()) { VarXRefRelinkVisitor{newModp}; }

        // Fix cross-module REFDTYPE pointers in newModp.
        UASSERT_OBJ(newModp, srcModp, "newModp null before hierarchy fixup");
        fixupCrossModuleRefDTypes(newModp);

        // Assign parameters to the constants specified
        // DOES clone() so must be finished with module clonep() before here
        برائے (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            اگر (pinp->exprp()) {
                اگر (AstVar* ثابت modvarp = pinp->modVarp()) {
                    Ast_گرہ* ثابت newp = pinp->exprp();  // Const or ConsArray
                    AstConst* ثابت exprp = VN_CAST(newp, Const);
                    AstConst* ثابت origp = VN_CAST(modvarp->valuep(), Const);
                    ثابت منطقی overridden
                        = !(origp && exprp && Parameterized_Hier_بلاکیں::areSame(exprp, origp));
                    // Remove any existing parameter
                    اگر (modvarp->valuep()) modvarp->valuep()->unlink_Fr_عقب()->delete_درخت();
                    // Set this parameter to value requested by cell
                    UINFO(9, "       set param " << modvarp << " = " << newp);
                    modvarp->valuep(newp->clone_درخت(جھوٹ));
                    modvarp->overriddenParam(overridden);
                } ورنہ اگر (AstParamTypeDType* ثابت modptp = pinp->modPTypep()) {
                    Ast_گرہ_DType* ثابت dtypep = VN_AS(pinp->exprp(), گرہ_DType);
                    UASSERT_OBJ(dtypep, pinp, "unlinked param dtype");
                    اگر (modptp->بچہ_DTypep()) modptp->بچہ_DTypep()->unlink_Fr_عقب()->delete_درخت();
                    // Set this parameter to value requested by cell
                    modptp->بچہ_DTypep(dtypep->clone_درخت(جھوٹ));
                    // Later V3LinkDot will convert the ParamDType to a Typedef
                    // Not done here as may be localparams, etc, that also need conversion
                }
            }
        }

        واپس سچ;
    }
    ثابت ModInfo* module_ڈھونڈو_Or_Clone(Ast_گرہ_Module* srcModp, Ast_گرہ* ifErrorp, AstPin* paramsp,
                                     ثابت string& newname, ثابت IfaceRefRefs& ifaceRefRefs) {
        // Already made this flavor?
        خودکار it = m_modNameMap.ڈھونڈو(newname);
        اگر (it != m_modNameMap.انتہا()) {
            UINFO(4, "     De-parameterize to prev: " << it->دوسرا.m_modp);
        } ورنہ {
            اگر (!deepCloneModule(srcModp, ifErrorp, paramsp, newname, ifaceRefRefs)) {
                واپس خالی_اشاریہ;
            }
            it = m_modNameMap.ڈھونڈو(newname);
            UASSERT(it != m_modNameMap.انتہا(), "should find just-made module");
        }
        ثابت ModInfo* ثابت modInfop = &(it->دوسرا);
        واپس modInfop;
    }

    خالی convert_مقصد_Stringp(Ast_گرہ* nodep) {
        // Should be called on values of parameters of type string to convert them
        // to properly typed string constants.
        // Has no effect if the value is not a string constant.
        AstConst* ثابت constp = VN_CAST(nodep, Const);
        // Check if it wasn't already converted
        اگر (constp && !constp->عدد().isString()) {
            constp->replaceWith(
                نیا AstConst{constp->fileline(), AstConst::String{}, constp->عدد().مقصد_String()});
            VL_DO_DANGLING(constp->delete_درخت(), constp);
        }
    }

    // Fold a param/lparam member's value to a Const in place.  Resolves any
    // class::member Dots in the value first (including those buried behind
    // VarRefs to sibling lparams that are not yet const, e.g. `one = base`
    // where `base = inner_a::v`), resolving siblings deepest-first so the
    // whole chain constifies.  `inProgress` guards against reference cycles.
    خالی constify_Member_قدر(AstVar* varp, std::سیٹ<AstVar*>& اندر_پیش_رفت) {
        اگر (!varp->valuep() || VN_IS(varp->valuep(), Const)) واپس;
        اگر (!اندر_پیش_رفت.داخل_کرو(varp).دوسرا) {
            // Re-entered while still resolving varp: its value transitively
            // references itself.  Report and replace the value with a Const, as
            // otherwise the unresolved Dot reaches V3Width as an untyped node.
            varp->v3error("Variable's initial value is circular: " << varp->prettyNameQ());
            varp->valuep()->unlink_Fr_عقب()->delete_درخت();
            varp->valuep(نیا AstConst{varp->fileline(), AstConst::Signed32{}, 0});
            واپس;
        }
        // First resolve sibling lparams this value references, so their Dots
        // are folded before we inline them here.
        std::سیٹ<AstVar*> siblings;
        varp->valuep()->foreach([&](ثابت AstVarRef* refp) {
            AstVar* ثابت refVarp = refp->varp();
            اگر (refVarp && refVarp->isParam() && refVarp->valuep()
                && !VN_IS(refVarp->valuep(), Const)) {
                siblings.داخل_کرو(refVarp);
            }
        });
        برائے (AstVar* ثابت sibp : siblings) constify_Member_قدر(sibp, اندر_پیش_رفت);
        // Resolve class-scoped references and any member selections they expose.
        resolveDeferredDotsReachableFrom(varp->valuep(), m_modp);
        V3Const::constifyParamsEdit(varp);
        اندر_پیش_رفت.erase(varp);
    }

    // Return the member name on a deferred Dot RHS, including a selected member.
    ساکن AstParseRef* memberParseRef(Ast_گرہ* nodep) {
        اگر (AstParseRef* ثابت refp = VN_CAST(nodep, ParseRef)) واپس refp;
        Ast_گرہ_Pre_Sel* ثابت selp = VN_CAST(nodep, گرہ_Pre_Sel);
        واپس selp ? VN_CAST(selp->fromp(), ParseRef) : خالی_اشاریہ;
    }

    // Replace a deferred Dot with valuep, preserving any select on the member name.
    ساکن خالی replaceMemberDot(AstDot* dotp, AstParseRef* memberRefp, Ast_گرہ_اظہاریہ* valuep) {
        اگر (memberRefp == dotp->rhsp()) {
            dotp->replaceWith(valuep);
        } ورنہ {
            Ast_گرہ_Pre_Sel* ثابت selp = VN_AS(dotp->rhsp(), گرہ_Pre_Sel);
            اگر (Ast_Attr_کا* ثابت attrp = selp->attrp()) {
                اگر (Ast_گرہ* ثابت oldFromp = attrp->fromp()) {
                    oldFromp->replaceWith(valuep->clone_درخت(جھوٹ));
                    VL_DO_DANGLING(oldFromp->delete_درخت(), oldFromp);
                }
            }
            memberRefp->replaceWith(valuep);
            VL_DO_DANGLING(memberRefp->delete_درخت(), memberRefp);
            dotp->replaceWith(dotp->rhsp()->unlink_Fr_عقب());
        }
        VL_DO_DANGLING(dotp->delete_درخت(), dotp);
    }

    // True for an expression produced while lowering a deferred struct-member chain.
    ساکن منطقی is_Deferred_Member_بنیاد(ثابت Ast_گرہ_اظہاریہ* nodep) {
        اگر (VN_IS(nodep, MemberSel)) واپس سچ;
        اگر (ثابت Ast_گرہ_Pre_Sel* ثابت selp = VN_CAST(nodep, گرہ_Pre_Sel)) {
            ثابت Ast_گرہ_DType* ثابت fromDTypep = selp->fromp()->dtypep();
            ثابت Ast_گرہ_Array_DType* ثابت arrayDTypep
                = fromDTypep ? VN_CAST(fromDTypep->skipRefOrNullp(), گرہ_Array_DType) : خالی_اشاریہ;
            اگر (arrayDTypep) {
                ثابت Ast_گرہ_DType* ثابت elemDTypep = arrayDTypep->subDTypep();
                واپس elemDTypep && VN_IS(elemDTypep->skipRefOrNullp(), گرہ_UOr_Struct_DType);
            }
            واپس is_Deferred_Member_بنیاد(selp->fromp());
        }
        ثابت Ast_گرہ_DType* ثابت dtypep = nodep->dtypep();
        واپس dtypep && VN_IS(dtypep->skipRefOrNullp(), گرہ_UOr_Struct_DType);
    }

    // Lower a Dot exposed after its inner class::member reference was substituted.
    ساکن خالی resolveMemberDot(AstDot* dotp) {
        Ast_گرہ_اظہاریہ* ثابت lhsp = VN_CAST(dotp->lhsp(), گرہ_اظہاریہ);
        AstParseRef* ثابت memberRefp = memberParseRef(dotp->rhsp());
        اگر (!lhsp || !memberRefp || !is_Deferred_Member_بنیاد(lhsp)) واپس;
        AstMemberSel* ثابت newp = نیا AstMemberSel{memberRefp->fileline(), lhsp->unlink_Fr_عقب(),
                                                    VFlag_بچہ_DType{}, memberRefp->name()};
        replaceMemberDot(dotp, memberRefp, newp);
    }

    // Helper to resolve DOT to RefDType for class type references.
    // If the class is parameterized and not yet specialized, specialize it first.
    // This handles cases like: iface #(param_class#(value)::typedef_name)
    خالی resolve_Dot_مقصد_Typedef(Ast_گرہ* exprp) {
        AstDot* ثابت dotp = VN_CAST(exprp, Dot);
        اگر (!dotp) واپس;
        UINFO(9, "Resolve deferred Dot: " << dotp);
        AstClassOrPackageRef* ثابت classRefp = VN_CAST(dotp->lhsp(), ClassOrPackageRef);
        اگر (!classRefp) {
            resolveMemberDot(dotp);
            واپس;
        }
        AstParseRef* ثابت parseRefp = memberParseRef(dotp->rhsp());
        اگر (!parseRefp) واپس;

        ثابت AstClass* lhsClassp = VN_CAST(classRefp->classOrPackageSkipp(), Class);

        // Specialize parameterized class through type parameter indirection (#7000)
        AstParamTypeDType* ثابت paramTypep
            = VN_CAST(classRefp->classOrPackageNodep(), ParamTypeDType);
        اگر (paramTypep) {
            Ast_گرہ_DType* ثابت dtypep = paramTypep->subDTypep();
            AstClassRefDType* ثابت classRefDTypep
                = dtypep ? VN_CAST(dtypep->skipRefOrNullp(), ClassRefDType) : خالی_اشاریہ;
            اگر (classRefDTypep) {
                AstClass* ثابت srcClassp = classRefDTypep->classp();
                اگر (srcClassp && srcClassp->hasGParam() && classRefDTypep->paramsp()) {
                    اگر (lhsClassp == srcClassp || !lhsClassp) {
                        classRefDeparam(classRefDTypep, srcClassp);
                        lhsClassp = classRefDTypep->classp();
                    }
                }
            }
        }

        اگر (classRefp->paramsp()) {
            AstClass* ثابت srcClassp = VN_CAST(classRefp->classOrPackageNodep(), Class);
            اگر (srcClassp && srcClassp->hasGParam()) {
                اگر (lhsClassp == srcClassp || !lhsClassp) {
                    classRefDeparam(classRefp, srcClassp);
                    lhsClassp = VN_CAST(classRefp->classOrPackageSkipp(), Class);
                }
            }
        }
        // Typedef-aliased paramed class (e.g., typedef C#(7) my_c; my_c::b).
        اگر (AstClassRefDType* ثابت crp = class_Ref_DType_کا_گرہ(classRefp->classOrPackageNodep())) {
            اگر (AstClass* ثابت srcClassp = crp->classp()) {
                اگر (srcClassp->hasGParam() && crp->paramsp()) {
                    classRefDeparam(crp, srcClassp);
                    اگر (AstClass* ثابت newClassp = crp->classp()) lhsClassp = newClassp;
                }
            }
        }
        اگر (!lhsClassp) واپس;

        Ast_گرہ* ثابت memberp = m_memberMap.ڈھونڈو_Member(lhsClassp, parseRefp->name());
        UINFO(9, "Resolve deferred class member: " << parseRefp->name() << " -> " << memberp);
        اگر (AstTypedef* ثابت tdefp = VN_CAST(memberp, Typedef)) {
            اگر (parseRefp != dotp->rhsp()) واپس;
            AstRefDType* ثابت refp = نیا AstRefDType{dotp->fileline(), tdefp->name()};
            refp->typedefp(tdefp);
            dotp->replaceWith(refp);
            VL_DO_DANGLING(dotp->delete_درخت(), dotp);
        } ورنہ اگر (AstVar* ثابت varp = VN_CAST(memberp, Var)) {
            substituteParamMember(dotp, parseRefp, varp);
        }
    }

    // Substitute a class param/lparam member's constant value for dotp. The
    // reverse-order deferred-Dot walk lowers any outer fields to MemberSels.
    خالی substituteParamMember(AstDot* ثابت dotp, AstParseRef* ثابت memberRefp,
                               AstVar* ثابت varp) {
        اگر (!varp->isParam() || !varp->valuep()) واپس;
        اگر (!VN_IS(varp->valuep(), Const)) {
            std::سیٹ<AstVar*> اندر_پیش_رفت;
            constify_Member_قدر(varp, اندر_پیش_رفت);
        }
        AstConst* ثابت constp = VN_CAST(varp->valuep(), Const);
        اگر (!constp) واپس;
        AstConst* ثابت newp = constp->clone_درخت(جھوٹ);
        newp->dtypep(varp->subDTypep());
        replaceMemberDot(dotp, memberRefp, newp);
    }

    // Resolve one class-scoped RefDType. The generic deferred walk finds these
    // through every dtype wrapper and follows out-of-tree typedef references.
    خالی resolveParamClassRefDType(AstRefDType* refp) {
        اگر (refp->typedefp() || refp->refDTypep()) واپس;

        AstClassOrPackageRef* ثابت classRefp
            = VN_CAST(refp->classOrPackageOpp(), ClassOrPackageRef);
        اگر (!classRefp) واپس;

        AstClass* srcClassp = VN_CAST(classRefp->classOrPackageNodep(), Class);
        اگر (srcClassp && srcClassp->hasGParam()) {
            classRefDeparam(classRefp, srcClassp);
            واپس;
        }

        // Follow typedef chain to find the underlying ClassRefDType
        Ast_گرہ* targetp = classRefp->classOrPackageNodep();
        جب (ثابت AstTypedef* ثابت tdefp = VN_CAST(targetp, Typedef))
            targetp = tdefp->subDTypep();
        اگر (Ast_گرہ_DType* ثابت dtargetp = VN_CAST(targetp, گرہ_DType))
            targetp = dtargetp->skipRefOrNullp();
        اگر (!targetp) واپس;
        AstClassRefDType* ثابت classRefDTypep = VN_CAST(targetp, ClassRefDType);
        اگر (!classRefDTypep) واپس;
        srcClassp = classRefDTypep->classp();
        اگر (!srcClassp) واپس;

        AstClass* newClassp = srcClassp;
        اگر (srcClassp->hasGParam()) {
            classRefDeparam(classRefDTypep, srcClassp);
            newClassp = classRefDTypep->classp();
        }

        AstTypedef* ثابت typedefp
            = VN_CAST(m_memberMap.ڈھونڈو_Member(newClassp, refp->name()), Typedef);
        اگر (typedefp) {
            refp->typedefp(typedefp);
            refp->classOrPackagep(newClassp);
        }
    }

    // True if a $bits/$size type query in nodep's parameters reads another type parameter.
    ساکن منطقی defaultParamsHaveTypeQueryOnParamType(ثابت AstClassRefDType* nodep) {
        منطقی ملا = جھوٹ;
        nodep->foreach([&](ثابت Ast_Attr_کا* attrp) {
            اگر (ملا || !attrp->attrType().isTypeQuery()) واپس;
            ثابت AstRefDType* ثابت refp = VN_CAST(attrp->fromp(), RefDType);
            اگر (refp && VN_IS(refp->refDTypep(), ParamTypeDType)) ملا = سچ;
        });
        واپس ملا;
    }

    // Check if exprp's class matches origp's class after deparameterization.
    // Handles both the simple case (user4p link from defaultsResolved) and the
    // nested case where the default's inner class has non-default sub-parameters
    // (e.g., uvm_sequence#(uvm_reg_item) where uvm_reg_item != default uvm_sequence_item).
    منطقی classTypeMatchesDefaultClone(ثابت Ast_گرہ_DType* exprp, ثابت Ast_گرہ_DType* origp) {
        exprp = exprp->skipRefp();
        origp = origp->skipRefp();
        ثابت خودکار* ثابت اظہاریہ_Class_Refp = VN_CAST(exprp, ClassRefDType);
        ثابت خودکار* ثابت origClassRefp = VN_CAST(origp, ClassRefDType);
        اگر (!اظہاریہ_Class_Refp || !origClassRefp) واپس جھوٹ;
        // Fast path: check user4p link (set when template was deparameterized with defaults)
        ثابت Ast_گرہ_Module* ثابت defaultClonep
            = VN_CAST(origClassRefp->classp()->user4p(), Class);
        اگر (defaultClonep && defaultClonep == اظہاریہ_Class_Refp->classp()) واپس سچ;
        // Skip the comparison when the default's $bits/$size reads another type parameter, as
        // deparameterizing it below would resolve that shared type at the wrong width (#7711).
        اگر (defaultParamsHaveTypeQueryOnParamType(origClassRefp)) واپس جھوٹ;
        // Slow path: deparameterize the default type and compare the result.
        // Different templates can never match; use origName() because exprp's
        // class may already be a specialization (clone) of the template.
        اگر (!origClassRefp->classp()->hasGParam()) واپس جھوٹ;
        اگر (origClassRefp->classp()->origName() != اظہاریہ_Class_Refp->classp()->origName())
            واپس جھوٹ;
        // Prevent re-entry when classRefDeparam recurses through cellPinCleanup.
        اگر (!m_default_Clone_اندر_پیش_رفت.داخل_کرو(origClassRefp->classp()).دوسرا) واپس جھوٹ;
        // const_cast safe: cloneTree doesn't modify the source
        AstClassRefDType* ثابت origClonep = static_cast<AstClassRefDType*>(
            const_cast<AstClassRefDType*>(origClassRefp)->clone_درخت(جھوٹ));
        Ast_گرہ_Module* ثابت resolvedModp = classRefDeparam(origClonep, origClassRefp->classp());
        ثابت منطقی مطابقت = resolvedModp && VN_CAST(resolvedModp, Class) == اظہاریہ_Class_Refp->classp();
        VL_DO_DANGLING(origClonep->delete_درخت(), origClonep);
        m_default_Clone_اندر_پیش_رفت.erase(origClassRefp->classp());
        واپس مطابقت;
    }

    ساکن منطقی param_Consts_Equal_At_Max_چوڑائی(AstConst* exprp, AstConst* origp) {
        // Return true if two integer constants are equal after sign-extending
        // both to max(width).  A typed parameter default (e.g. byte) is
        // narrower than a 32-bit pin expression, so sameTree/areSame fail.
        اگر (exprp->عدد().چوڑائی() == origp->عدد().چوڑائی()) واپس جھوٹ;
        اگر (exprp->عدد().isOpaque()) واپس جھوٹ;
        اگر (origp->عدد().isOpaque()) واپس جھوٹ;
        ثابت صحیح max_چوڑائی = std::max(exprp->عدد().چوڑائی(), origp->عدد().چوڑائی());
        V3_نمبر اظہاریہ_عدد{exprp, max_چوڑائی};
        اگر (exprp->عدد().isSigned()) {
            اظہاریہ_عدد.opExtendS(exprp->عدد(), exprp->عدد().چوڑائی());
        } ورنہ {
            اظہاریہ_عدد.op_تفویض(exprp->عدد());
        }
        V3_نمبر orig_عدد{origp, max_چوڑائی};
        اگر (origp->عدد().isSigned()) {
            orig_عدد.opExtendS(origp->عدد(), origp->عدد().چوڑائی());
        } ورنہ {
            orig_عدد.op_تفویض(origp->عدد());
        }
        V3_نمبر isEq{exprp, 1};
        isEq.opEq(اظہاریہ_عدد, orig_عدد);
        واپس isEq.isNeqZero();
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
    خالی resolveGenericIfaceForwardingPins(AstPin* paramsp) {
        برائے (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            اگر (!pinp->exprp()) جاری;
            اگر (!VString::startsWith(pinp->name(), "__VGIfaceParam")) جاری;
            ثابت Ast_گرہ_Var_Ref* ثابت fwdRefp = VN_CAST(pinp->exprp(), گرہ_Var_Ref);
            اگر (!fwdRefp) جاری;
            ثابت AstIfaceRefDType* ثابت resolvedp
                = VN_CAST(fwdRefp->varp()->بچہ_DTypep()->skipRefp(), IfaceRefDType);
            UASSERT_OBJ(resolvedp, pinp,
                        "Generic interface forwarding pin not specialized before use");
            AstIfaceRefDType* ثابت newIrefp = نیا AstIfaceRefDType{
                resolvedp->fileline(), resolvedp->modportFileline(), resolvedp->cellName(),
                resolvedp->ifaceName(), resolvedp->modportName()};
            newIrefp->ifacep(resolvedp->ifacep());
            اگر (resolvedp->paramsp()) {
                newIrefp->جوڑو_Paramsp(resolvedp->paramsp()->clone_درخت(سچ));
            }
            pinp->exprp()->unlink_Fr_عقب()->delete_درخت();
            pinp->exprp(newIrefp);
        }
    }

    // Include the pin's value in the specialization name, so cells passing equal
    // values share one module clone.
    خالی name_By_Pin_قدر(AstPin* pinp, Ast_گرہ_Module* srcModp, AstVar* modvarp, string& longnamer,
                        منطقی& any_overridesr) {
        longnamer += "_" + paramSmallName(srcModp, modvarp) + param_قدر_نمبر(pinp->exprp());
        any_overridesr = سچ;
    }

    // Name the specialization by a value pin, or prepare a type pin, noting in
    // overridingTypePinsr whether it differs from the default
    خالی cellPinCleanup(Ast_گرہ* nodep, AstPin* pinp, AstPin* paramsp, Ast_گرہ_Module* srcModp,
                        string& longnamer, منطقی& any_overridesr,
                        std::unordered_سیٹ<ثابت AstPin*>& overridingTypePinsr) {
        اگر (!pinp->exprp()) واپس;  // No-connect
        اگر (AstVar* ثابت modvarp = pinp->modVarp()) {
            resolveDeferredDotsReachableFrom(modvarp->subDTypep(), m_modp);
            اگر (!modvarp->isGParam()) {
                pinp->v3fatalSrc("Attempted parameter setting of non-parameter: Param "
                                 << pinp->prettyNameQ() << " of " << nodep->prettyNameQ());
            } ورنہ اگر (VN_IS(pinp->exprp(), نمونہ)
                       || (VN_IS(pinp->exprp(), ConsArray)
                           && arraySubDTypep(modvarp->subDTypep()))) {
                // A pattern or array value is named by its contents
                name_By_Pin_قدر(pinp, srcModp, modvarp, longnamer, any_overridesr);
            } ورنہ {
                UINFO(9, "cellPinCleanup: before constify " << pinp << " " << modvarp);
                V3Const::constifyParamsEdit(pinp->exprp());
                اگر (is_Aggregate_Param_قدر(pinp->exprp())) {
                    name_By_Pin_قدر(pinp, srcModp, modvarp, longnamer, any_overridesr);
                    واپس;
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
                اگر (modvarp->valuep()
                    && (VN_IS(modvarp->valuep(), Cast) || VN_IS(modvarp->valuep(), Cast_سائز))) {
                    ثابت منطقی dependent = modvarp->valuep()->exists(
                        [](Ast_گرہ* np) { واپس VN_IS(np, VarRef) || VN_IS(np, RefDType); });
                    اگر (!dependent) V3Const::constifyParamsEdit(modvarp->valuep());
                }
                UINFO(9, "cellPinCleanup: after constify " << pinp);
                // String constants are parsed as logic arrays and converted to strings in V3Const.
                // At this moment, some constants may have been already converted.
                // To correctly compare constants, both should be of the same type,
                // so they need to be converted.
                اگر (isString(modvarp->subDTypep())) {
                    convert_مقصد_Stringp(pinp->exprp());
                    convert_مقصد_Stringp(modvarp->valuep());
                }
                AstConst* ثابت exprp = VN_CAST(pinp->exprp(), Const);
                AstConst* ثابت origp = VN_CAST(modvarp->valuep(), Const);
                // Width the pin to the port's type so equal values hash the same (#5479).
                AstConst* normedNamep = خالی_اشاریہ;
                اگر (exprp && !exprp->عدد().isDouble() && !exprp->عدد().isString()) {
                    AstVar* cloneVarp = modvarp->clone_درخت(جھوٹ);
                    منطقی cloneVarpUnresolved = جھوٹ;
                    اگر (Ast_گرہ* ثابت oldValuep = cloneVarp->valuep()) {
                        oldValuep->unlink_Fr_عقب();
                        VL_DO_DANGLING(oldValuep->delete_درخت(), oldValuep);
                    }
                    cloneVarp->valuep(exprp->clone_درخت(جھوٹ));
                    اگر (Ast_گرہ_DType* ثابت origDTypep = modvarp->subDTypep()) {
                        // Attach clone under cloneVarp so the root has a back pointer.
                        اگر (cloneVarp->بچہ_DTypep())
                            cloneVarp->بچہ_DTypep()->unlink_Fr_عقب()->delete_درخت();
                        cloneVarp->بچہ_DTypep(origDTypep->clone_درخت(جھوٹ));
                        cloneVarp->dtypep(خالی_اشاریہ);
                        // Inline param refs so widthing doesn't touch the template (#7411).
                        constexpr صحیح maxSubstIters = 1000;
                        برائے (صحیح it = 0; it < maxSubstIters; ++it) {
                            منطقی any = جھوٹ;
                            cloneVarp->foreach([&](AstVarRef* varrefp) {
                                AstVar* ثابت targetp = varrefp->varp();
                                Ast_گرہ* replacep = خالی_اشاریہ;
                                برائے (AstPin* pp = paramsp; pp; pp = VN_AS(pp->nextp(), Pin)) {
                                    اگر (pp->modVarp() == targetp) {
                                        // A pattern or array override names the value too
                                        اگر (VN_IS(pp->exprp(), Const)
                                            || VN_IS(pp->exprp(), نمونہ)
                                            || VN_IS(pp->exprp(), ConsArray)) {
                                            replacep = pp->exprp()->clone_درخت(جھوٹ);
                                        }
                                        توڑو;
                                    }
                                }
                                اگر (!replacep && targetp->valuep()) {
                                    replacep = targetp->valuep()->clone_درخت(جھوٹ);
                                }
                                // An inlined pattern takes the type of its parameter
                                Ast_نمونہ* ثابت patp = VN_CAST(replacep, نمونہ);
                                اگر (patp && !patp->بچہ_DTypep() && targetp->بچہ_DTypep()) {
                                    patp->بچہ_DTypep(targetp->بچہ_DTypep()->clone_درخت(جھوٹ));
                                }
                                اگر (replacep) {
                                    varrefp->replaceWith(replacep);
                                    VL_DO_DANGLING(varrefp->delete_درخت(), varrefp);
                                    any = سچ;
                                }
                            });
                            // Replace RefDType to a ParamTypeDType with pin override
                            // or the paramtype's default so constify below does not
                            // reach into the template.  Collect then replace in
                            // reverse so descendants aren't freed early.
                            std::سمتیہ<std::pair<AstRefDType*, Ast_گرہ_DType*>> مقصد_Replace;
                            cloneVarp->foreach([&](AstRefDType* refp) {
                                AstParamTypeDType* ثابت ptdp
                                    = VN_CAST(refp->refDTypep(), ParamTypeDType);
                                اگر (!ptdp) واپس;
                                AstPin* overridePinp = خالی_اشاریہ;
                                برائے (AstPin* pp = paramsp; pp; pp = VN_AS(pp->nextp(), Pin)) {
                                    اگر (pp->modPTypep() == ptdp) {
                                        overridePinp = pp;
                                        توڑو;
                                    }
                                }
                                Ast_گرہ_DType* ثابت substp
                                    = overridePinp ? VN_CAST(overridePinp->exprp(), گرہ_DType)
                                                   : ptdp->subDTypep();
                                اگر (substp) مقصد_Replace.emplace_عقب(refp, substp);
                            });
                            برائے (خودکار it = مقصد_Replace.rbegin(); it != مقصد_Replace.rend(); ++it) {
                                AstRefDType* ثابت refp = it->پہلا;
                                refp->replaceWith(it->دوسرا->clone_درخت(جھوٹ));
                                VL_DO_DANGLING(refp->delete_درخت(), refp);
                                any = سچ;
                            }
                            اگر (!any) توڑو;
                        }
                        // Skip the widthing constify if anything left can't be evaluated here: a
                        // reference, as to a variable or around a cycle, an unresolved type
                        // parameter, or a type or enum item of the template
                        cloneVarp->foreach([&](Ast_گرہ* np) {
                            اگر (VN_IS(np, VarRef)) {
                                cloneVarpUnresolved = سچ;
                                واپس;
                            }
                            اگر (Ast_Enum_شے_Ref* ثابت شے_Refp = VN_CAST(np, Enum_شے_Ref)) {
                                اگر (V3LinkDotIfaceCapture::ڈھونڈو_Owner_Module(شے_Refp->itemp())
                                    == srcModp) {
                                    cloneVarpUnresolved = سچ;
                                }
                                واپس;
                            }
                            AstRefDType* ثابت refp = VN_CAST(np, RefDType);
                            اگر (!refp) واپس;
                            اگر (VN_IS(refp->refDTypep(), ParamTypeDType)) {
                                UINFO(5, "  cellPinCleanup: skip normedNamep "
                                         "(unresolved RefDType->ParamTypeDType) pin="
                                             << pinp->prettyNameQ());
                                cloneVarpUnresolved = سچ;
                            }
                            // A type declared in the template must not be widthed there
                            اگر (V3LinkDotIfaceCapture::ڈھونڈو_Owner_Module(refp->typedefp()) == srcModp
                                || V3LinkDotIfaceCapture::ڈھونڈو_Owner_Module(refp->refDTypep())
                                       == srcModp) {
                                cloneVarpUnresolved = سچ;
                            }
                        });
                    }
                    اگر (!cloneVarpUnresolved) {
                        V3Const::constifyParamsEdit(cloneVarp);
                        اگر (AstConst* ثابت widthedp = VN_CAST(cloneVarp->valuep(), Const)) {
                            // Stamp the port's type so equal values hash the same.
                            اگر (cloneVarp->dtypep()) widthedp->dtypep(cloneVarp->dtypep());
                            widthedp->unlink_Fr_عقب();
                            normedNamep = widthedp;
                        }
                    }
                    VL_DO_DANGLING(cloneVarp->delete_درخت(), cloneVarp);
                }
                AstConst* ثابت namingExprp = normedNamep ? normedNamep : exprp;
                اگر (!exprp) {
                    // With DepGraph architecture, all expressions should be constants
                    // by the time V3Param runs. If not, it's an error.
                    UINFO(9, "cellPinCleanup: NOT CONST after constify " << pinp);
                    UINFOTREE(1, pinp, "", "errnode");
                    pinp->v3error("Can't convert defparam value to constant: Param "
                                  << pinp->prettyNameQ() << " of " << nodep->prettyNameQ());
                    Ast_گرہ* ثابت pinExprp = pinp->exprp();
                    pinExprp->replaceWith(نیا AstConst{pinp->fileline(), AstConst::Widthed_قدر{},
                                                       modvarp->چوڑائی(), 0});
                    VL_DO_DANGLING(pinExprp->delete_درخت(), pinExprp);
                } ورنہ اگر (origp
                           && (exprp->same_درخت(origp)
                               || (exprp->عدد().چوڑائی() == origp->عدد().چوڑائی()
                                   && Parameterized_Hier_بلاکیں::areSame(exprp, origp))
                               || param_Consts_Equal_At_Max_چوڑائی(exprp, origp))) {
                    // Setting parameter to its default value.  Just ignore it.
                    // This prevents making additional modules, and makes coverage more
                    // obvious as it won't show up under a unique module page name.
                    UINFO(9, "cellPinCleanup: same as default " << pinp);
                } ورنہ اگر (namingExprp->عدد().isDouble() || namingExprp->عدد().isString()
                           || namingExprp->عدد().isFourState()
                           || namingExprp->عدد().چوڑائی() != 32) {
                    longnamer += ("_" + paramSmallName(srcModp, modvarp)
                                  + param_قدر_نمبر(namingExprp));
                    any_overridesr = سچ;
                } ورنہ {
                    longnamer += ("_" + paramSmallName(srcModp, modvarp)
                                  + namingExprp->عدد().ascii(جھوٹ));
                    any_overridesr = سچ;
                }
                اگر (normedNamep) VL_DO_DANGLING(normedNamep->delete_درخت(), normedNamep);
            }
        } ورنہ اگر (AstParamTypeDType* ثابت modvarp = pinp->modPTypep()) {
            // Resolve the pin and declared type through arbitrary dtype wrappers
            // and typedef chains before widthing either one.
            resolveDeferredDotsReachableFrom(pinp->exprp(), m_modp);
            resolveDeferredDotsReachableFrom(modvarp->subDTypep(), m_modp);

            Ast_گرہ_DType* rawTypep = VN_CAST(pinp->exprp(), گرہ_DType);
            // Guard against widthing a struct/union still owned by a
            // parameterized template interface (not yet specialized).
            // widthParamsEdit is destructive: it evaluates range expressions,
            // sets didWidth=1, and removes Range nodes, which would corrupt
            // the template's BASICDTYPEs for all subsequent clones.
            // Triggered by deeply nested parameterized interfaces (e.g.
            // outer_if containing inner_if with struct typedefs) when the
            // inner interface hasn't been specialized yet.
            منطقی skip_چوڑائی_For_Template_Struct = جھوٹ;
            {
                // Use non-asserting skip: before widthParamsEdit, type(expr)
                // constructs may contain unlinked REFDTYPEs (e.g. type(x-y))
                Ast_گرہ_DType* ثابت resolvedp = rawTypep ? rawTypep->skipRefOrNullp() : خالی_اشاریہ;
                اگر (resolvedp && (VN_IS(resolvedp, StructDType) || VN_IS(resolvedp, UnionDType))) {
                    Ast_گرہ_Module* ثابت ownerModp
                        = V3LinkDotIfaceCapture::ڈھونڈو_Owner_Module(resolvedp);
                    // Skip if owned by a parameterized interface (template or not
                    // yet cloned).  hasGParam() covers interfaces that haven't
                    // been cloned yet (parameterizedTemplate() not set).
                    اگر (ownerModp && VN_IS(ownerModp, Iface) && ownerModp->hasGParam()) {
                        skip_چوڑائی_For_Template_Struct = سچ;
                        V3Stats::جوڑو_Stat_مجموع("Param, Template struct width skips", 1);
                        UINFO(9, "SKIP-WIDTH-TEMPLATE: struct="
                                     << resolvedp->prettyTypeName() << " templateOwner="
                                     << ownerModp->prettyNameQ() << " pin=" << pinp->prettyNameQ()
                                     << " of " << nodep->prettyNameQ()
                                     << " srcMod=" << srcModp->prettyNameQ());
                    }
                }
                اگر (rawTypep && !skip_چوڑائی_For_Template_Struct) V3_چوڑائی::چوڑائی_Params_Edit(rawTypep);
            }
            Ast_گرہ_DType* exprp = rawTypep ? rawTypep->skip_Ref_مقصد_Non_Refp() : خالی_اشاریہ;
            ثابت Ast_گرہ_DType* origp = modvarp->skip_Ref_مقصد_Non_Refp();
            اگر (!exprp) {
                pinp->v3error("Parameter type pin value isn't a type: Param "
                              << pinp->prettyNameQ() << " of " << nodep->prettyNameQ());
            } ورنہ اگر (!origp) {
                pinp->v3error("Parameter type variable isn't a type: Param "
                              << modvarp->prettyNameQ());
            } ورنہ {
                UINFO(9, "Parameter type assignment expr=" << exprp << " to " << origp);
                اگر (!skip_چوڑائی_For_Template_Struct) {
                    V3Const::constifyParamsEdit(pinp->exprp());  // Reconcile typedefs
                    // Constify may have caused pinp->exprp to change
                    rawTypep = VN_AS(pinp->exprp(), گرہ_DType);
                    exprp = rawTypep->skip_Ref_مقصد_Non_Refp();
                }
                اگر (!modvarp->fwdType().is_گرہ_Compatible(exprp)) {
                    pinp->v3error("Parameter type expression type "
                                  << exprp->prettyDTypeNameQ()
                                  << " violates parameter's forwarding type '"
                                  << modvarp->fwdType().ascii() << "'");
                }
                اگر (exprp->similarDType(origp) || classTypeMatchesDefaultClone(exprp, origp)) {
                    // Setting parameter to its default value.  Just ignore it.
                    // This prevents making additional modules, and makes coverage more
                    // obvious as it won't show up under a unique module page name.
                } ورنہ {
                    اگر (!skip_چوڑائی_For_Template_Struct) {
                        VL_DO_DANGLING(V3Const::constifyParamsEdit(exprp), exprp);
                        rawTypep = VN_CAST(pinp->exprp(), گرہ_DType);
                        exprp = rawTypep ? rawTypep->skip_Ref_مقصد_Non_Refp() : خالی_اشاریہ;
                    }
                    // Deparameterize ClassRefDType before name generation (#7000)
                    اگر (AstClassRefDType* ثابت classRefDTypep = VN_CAST(exprp, ClassRefDType)) {
                        اگر (classRefDTypep->paramsp() && classRefDTypep->classp()
                            && classRefDTypep->classp()->hasGParam()) {
                            classRefDeparam(classRefDTypep, classRefDTypep->classp());
                        }
                    }
                    overridingTypePinsr.emplace(pinp);  // Named once every pin is prepared
                }
            }
        } ورنہ {
            pinp->v3fatalSrc("Parameter not found in sub-module: Param "
                             << pinp->prettyNameQ() << " of " << nodep->prettyNameQ());
        }
    }

    خالی cellInterfaceCleanup(AstPin* pinsp, Ast_گرہ_Module* srcModp, string& longnamer,
                              منطقی& any_overridesr, IfaceRefRefs& ifaceRefRefs) {
        برائے (AstPin* pinp = pinsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            ثابت AstVar* ثابت modvarp = pinp->modVarp();
            اگر (modvarp && VN_IS(arraySubDTypeDeepp(modvarp->subDTypep()), IfaceGenericDType)) {
                جاری;
            }
            اگر (modvarp->isIfaceRef()) {
                // arraySubDTypeDeepp returns input unchanged if not an array.
                AstIfaceRefDType* ثابت پورٹ_Irefp
                    = VN_CAST(arraySubDTypeDeepp(modvarp->subDTypep()), IfaceRefDType);
                ثابت Ast_گرہ* ثابت exprp = pinp->exprp();
                اگر (VN_IS(exprp, CellArrayRef)) {
                    // CellArrayRef not yet linked; V3LinkDot resolves this pin later.
                    UINFO(9, "Skipping interface cleanup for CellArrayRef pin: " << pinp);
                    جاری;
                }
                AstIfaceRefDType* pinIrefp = خالی_اشاریہ;
                // Pin is a VarRef to a var of (possibly arrayed) iface type.
                اگر (ثابت Ast_گرہ_Var_Ref* ثابت vrp = VN_CAST(exprp, گرہ_Var_Ref)) {
                    اگر (vrp->varp()) {
                        pinIrefp
                            = VN_CAST(arraySubDTypeDeepp(vrp->varp()->subDTypep()), IfaceRefDType);
                    }
                }
                // Pin is a select, of any depth, of a VarRef (e.g. SelBit/ArraySel into an iface
                // array, or a slice of a row of a multi-dimensional one)
                اگر (!pinIrefp && exprp) {
                    ثابت Ast_گرہ* basep = exprp;
                    جب (ثابت Ast_گرہ_Pre_Sel* ثابت selp = VN_CAST(basep, گرہ_Pre_Sel)) {
                        basep = selp->fromp();
                    }
                    اگر (ثابت AstVarRef* ثابت vrp = VN_CAST(basep, VarRef)) {
                        اگر (vrp->varp()) {
                            pinIrefp = VN_CAST(arraySubDTypeDeepp(vrp->varp()->subDTypep()),
                                               IfaceRefDType);
                        }
                    }
                }

                UINFO(9, "     portIfaceRef " << پورٹ_Irefp);

                اگر (!پورٹ_Irefp) {
                    pinp->v3error("Interface port " << modvarp->prettyNameQ()
                                                    << " is not an interface " << modvarp);
                } ورنہ اگر (!pinIrefp) {
                    pinp->v3error("Interface port "
                                  << modvarp->prettyNameQ()
                                  << " is not connected to interface/modport pin expression");
                } ورنہ {
                    UINFO(9, "     pinIfaceRef " << pinIrefp);
                    اگر (پورٹ_Irefp->iface_بذریعہ_Cellp() != pinIrefp->iface_بذریعہ_Cellp()) {
                        UINFO(9, "     IfaceRefDType needs reconnect  " << pinIrefp);
                        longnamer += ("_" + paramSmallName(srcModp, pinp->modVarp())
                                      + param_قدر_نمبر(pinIrefp));
                        any_overridesr = سچ;
                        ifaceRefRefs.emplace_عقب(پورٹ_Irefp, pinIrefp);
                        اگر (پورٹ_Irefp->ifacep() != pinIrefp->ifacep()
                            // Might be different only due to param cloning, so check names too
                            && پورٹ_Irefp->ifaceName() != pinIrefp->ifaceName()) {
                            pinp->v3error("Port " << pinp->prettyNameQ() << " expects "
                                                  << Ast_گرہ::prettyNameQ(پورٹ_Irefp->ifaceName())
                                                  << " interface but pin connects "
                                                  << Ast_گرہ::prettyNameQ(pinIrefp->ifaceName())
                                                  << " interface");
                        }
                    }
                }
            }
        }
    }

    خالی storeOriginalParams(Ast_گرہ_Module* ثابت classp) {
        برائے (Ast_گرہ* stmtp = classp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            Ast_گرہ* originalParamp = خالی_اشاریہ;
            اگر (AstVar* ثابت varp = VN_CAST(stmtp, Var)) {
                اگر (varp->isGParam()) originalParamp = varp->clonep();
            } ورنہ اگر (AstParamTypeDType* ثابت ptp = VN_CAST(stmtp, ParamTypeDType)) {
                اگر (ptp->isGParam()) originalParamp = ptp->clonep();
            }
            اگر (originalParamp) m_originalParams[stmtp] = originalParamp;
        }
    }

    // Set interfaces types inside generic modules
    // to the corresponding values of implicit parameters
    خالی generic_Interface_Var_تیاری(ثابت AstPin* ثابت paramsp, ثابت AstPin* ثابت pinsp) {
        std::unordered_map<string, ثابت AstPin*> paramspMap;
        برائے (ثابت AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            اگر (VString::startsWith(pinp->name(), "__VGIfaceParam")) {
                paramspMap.داخل_کرو({pinp->name().substr(std::لڑی_لمبائی("__VGIfaceParam")), pinp});
            }
        }

        اگر (paramspMap.خالی_خانے()) واپس;

        برائے (ثابت Ast_گرہ* nodep = pinsp; nodep; nodep = nodep->nextp()) {
            اگر (ثابت AstPin* ثابت pinp = VN_CAST(nodep, Pin)) {
                اگر (AstVar* ثابت varp = pinp->modVarp()) {
                    Ast_گرہ_DType* dtypep = varp->بچہ_DTypep()->elemDTypep();
                    اگر (AstIfaceGenericDType* ثابت ifaceGDTypep
                        = VN_CAST(dtypep, IfaceGenericDType)) {
                        ثابت خودکار iter = paramspMap.ڈھونڈو(varp->name());
                        اگر (iter == paramspMap.انتہا()) جاری;
                        Ast_گرہ* ثابت backp = ifaceGDTypep->backp();
                        ifaceGDTypep->unlink_Fr_عقب();
                        ثابت AstPin* ثابت paramp = iter->دوسرا;
                        paramspMap.erase(iter);
                        ثابت AstIfaceRefDType* ثابت ifacerefp
                            = VN_AS(paramp->exprp(), IfaceRefDType);
                        AstIfaceRefDType* ثابت newIfacerefp = نیا AstIfaceRefDType{
                            ifaceGDTypep->fileline(), ifaceGDTypep->modportFileline(),
                            ifaceGDTypep->name(), ifacerefp->ifaceName(),
                            ifaceGDTypep->modportName()};
                        newIfacerefp->ifacep(ifacerefp->ifacep());
                        اگر (خودکار* ثابت arrDtp = VN_CAST(backp, گرہ_Array_DType)) {
                            arrDtp->بچہ_DTypep(newIfacerefp);
                        } ورنہ اگر (خودکار* ثابت arrDtp = VN_CAST(backp, BracketArrayDType)) {
                            arrDtp->بچہ_DTypep(newIfacerefp);
                        } ورنہ {
                            varp->بچہ_DTypep(newIfacerefp);
                        }
                        VL_DO_DANGLING(m_deleter.ڈالو_Deletep(ifaceGDTypep), ifaceGDTypep);
                        اگر (paramspMap.خالی_خانے()) واپس;
                    }
                }
            }
        }

        UASSERT(paramspMap.خالی_خانے(), "Not every generic interface implicit param is used");
    }

    خالی resolveDefaultParams(Ast_گرہ* nodep) {
        AstClassOrPackageRef* classOrPackageRef = VN_CAST(nodep, ClassOrPackageRef);
        AstClassRefDType* classRefDType = VN_CAST(nodep, ClassRefDType);
        AstClass* classp = خالی_اشاریہ;
        AstPin* paramsp = خالی_اشاریہ;

        اگر (classOrPackageRef) {
            classp = VN_CAST(classOrPackageRef->classOrPackageSkipp(), Class);
            اگر (!classp) واپس;  // No parameters in packages
            paramsp = classOrPackageRef->paramsp();
        } ورنہ اگر (classRefDType) {
            classp = classRefDType->classp();
            paramsp = classRefDType->paramsp();
        } ورنہ {
            nodep->v3fatalSrc("resolveDefaultParams called on node which is not a Class ref");
        }

        UASSERT_OBJ(classp, nodep, "Class or interface ref has no classp/ifacep");

        // Get the parameter list for this class
        m_classTypeParams.clear();
        m_param_اشاریہ.clear();
        برائے (Ast_گرہ* stmtp = classp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            اگر (AstParamTypeDType* paramTypep = VN_CAST(stmtp, ParamTypeDType)) {
                // Only consider formal class type parameters (generic parameters),
                // not localparam type declarations inside the class body.
                اگر (!paramTypep->isGParam()) جاری;
                m_param_اشاریہ.emplace(paramTypep, static_cast<صحیح>(m_classTypeParams.سائز()));
                m_classTypeParams.emplace_عقب(paramTypep, -1);
            }
        }

        // For each parameter, detect either a dependent default (copy from previous param)
        // or a direct type default (for ex. int). Store dependency index in
        // m_classParams[i].second, default type in defaultTypeNodes[i].
        std::سمتیہ<Ast_گرہ_DType*> defaultTypeNodes(m_classTypeParams.سائز(), خالی_اشاریہ);
        برائے (سائز_t i = 0; i < m_classTypeParams.سائز(); ++i) {
            AstParamTypeDType* ثابت paramTypep = m_classTypeParams[i].پہلا;
            // Parser places defaults/constraints under childDTypep as AstRequireDType
            AstRequireDType* ثابت reqDtp = VN_CAST(paramTypep->get_بچہ_DTypep(), RequireDType);
            اگر (!reqDtp) جاری;

            // If default is a reference to another param type, record dependency
            اگر (AstRefDType* ثابت refDtp = VN_CAST(reqDtp->subDTypep(), RefDType)) {
                اگر (AstParamTypeDType* ثابت ماخذ_Paramp
                    = VN_CAST(refDtp->refDTypep(), ParamTypeDType)) {
                    خودکار it = m_param_اشاریہ.ڈھونڈو(ماخذ_Paramp);
                    اگر (it != m_param_اشاریہ.انتہا()) { m_classTypeParams[i].دوسرا = it->دوسرا; }
                    جاری;  // dependency handled
                }
            }

            // If default is a direct type (for ex. int)
            // also record dependency
            اگر (Ast_گرہ_DType* ثابت dtp = VN_CAST(reqDtp->lhsp(), گرہ_DType)) {
                defaultTypeNodes[i] = dtp;
            }
        }

        // Count existing pins and capture them by index for easy lookup
        // Type parameters when named can be given in any order, but later we depend
        // on the order when handling defaults. So here we reorder them
        // for proper handling. The remaining ones are just pushed back
        std::سمتیہ<AstPin*> pins_By_اشاریہ;
        pins_By_اشاریہ.resize(m_classTypeParams.سائز(), خالی_اشاریہ);
        برائے (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            AstParamTypeDType* ثابت typep = pinp->modPTypep();
            ثابت خودکار it = typep ? m_param_اشاریہ.ڈھونڈو(typep) : m_param_اشاریہ.انتہا();
            اگر (it != m_param_اشاریہ.انتہا()) {
                pins_By_اشاریہ[it->دوسرا] = pinp;
            } ورنہ {
                pins_By_اشاریہ.ڈالو_عقب(pinp);
            }
        }

        // For each missing parameter, get its pin from dependency or direct default.
        // A dependent default can reference a parameter declared later (not IEEE, but
        // we support it here), so repeat until no further parameter can be resolved.
        برائے (منطقی resolvedSome = سچ; resolvedSome;) {
            resolvedSome = جھوٹ;
            برائے (سائز_t paramIdx = 0; paramIdx < m_classTypeParams.سائز(); paramIdx++) {
                اگر (pins_By_اشاریہ[paramIdx]) جاری;
                ثابت صحیح ماخذ_Param_Idx = m_classTypeParams[paramIdx].دوسرا;

                AstPin* newPinp = خالی_اشاریہ;

                // Case 1: Dependent default -> clone the source pin's type, but only once
                // the source parameter itself has been resolved (it might be listed later)
                اگر (ماخذ_Param_Idx >= 0 && pins_By_اشاریہ[ماخذ_Param_Idx]) {
                    newPinp = pins_By_اشاریہ[ماخذ_Param_Idx]->clone_درخت(جھوٹ);
                }

                // Case 2: Direct default type (e.g., int), create a new pin with that dtype
                اگر (!newPinp && defaultTypeNodes[paramIdx]) {
                    Ast_گرہ_DType* ثابت dtypep = defaultTypeNodes[paramIdx];
                    newPinp = نیا AstPin{dtypep->fileline(), static_cast<صحیح>(paramIdx) + 1,
                                         "__paramNumber" + cvt_مقصد_Str(paramIdx + 1),
                                         dtypep->clone_درخت(جھوٹ)};
                }

                اگر (newPinp) {
                    newPinp->name("__paramNumber" + cvt_مقصد_Str(paramIdx + 1));
                    newPinp->param(سچ);
                    newPinp->modPTypep(m_classTypeParams[paramIdx].پہلا);
                    اگر (classOrPackageRef) {
                        classOrPackageRef->جوڑو_Paramsp(newPinp);
                    } ورنہ اگر (classRefDType) {
                        classRefDType->جوڑو_Paramsp(newPinp);
                    }
                    // Update local tracking so future dependent defaults can find it
                    pins_By_اشاریہ[paramIdx] = newPinp;
                    اگر (!paramsp) paramsp = newPinp;
                    resolvedSome = سچ;
                }
            }
        }
    }

    Ast_گرہ_Module* گرہ_Deparam_Common(Ast_گرہ* nodep, Ast_گرہ_Module* srcModp, AstPin* paramsp,
                                     AstPin* pinsp, منطقی any_overrides) {
        // Returns new or reused module
        // Make sure constification worked
        // Must be a separate loop, as constant conversion may have changed some pointers.
        string longname = srcModp->name() + "_";
        اگر (debug() >= 9 && paramsp) paramsp->dump_درخت_And_اگلا(خارج, "-  cellparams: ");

        اگر (srcModp->hier_بلاک()) {
            longname = parameterized_Hier_بلاک_Name(srcModp, paramsp);
            any_overrides = longname != srcModp->name();
        } ورنہ {
            // Prepare every pin before naming any, as naming a value can depend on the others:
            // fold each value pin, as a class reference's may not be yet, and resolve each type
            std::unordered_سیٹ<ثابت AstPin*> overridingTypePins;
            برائے (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                اگر (!pinp->exprp()) جاری;  // An empty override keeps the default
                اگر (pinp->modPTypep()) {
                    cellPinCleanup(nodep, pinp, paramsp, srcModp, longname /*ref*/,
                                   any_overrides /*ref*/, overridingTypePins /*ref*/);
                    جاری;
                }
                AstVar* ثابت modvarp = pinp->modVarp();
                اگر (!modvarp || !modvarp->isGParam()) جاری;
                resolveDeferredDotsReachableFrom(modvarp->subDTypep(), m_modp);
                // An untyped pattern can't be folded without its parameter's type
                ثابت Ast_نمونہ* ثابت patternp = VN_CAST(pinp->exprp(), نمونہ);
                اگر (patternp && !patternp->بچہ_DTypep()) جاری;
                اگر (!VN_IS(pinp->exprp(), Const) && !is_Aggregate_Param_قدر(pinp->exprp())) {
                    // The pin, so a conditional of patterns is typed as an override's value
                    V3Const::constifyParamsEdit(pinp);
                }
            }
            برائے (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                اگر (AstParamTypeDType* ثابت modptp = pinp->modPTypep()) {
                    اگر (!overridingTypePins.گنتی(pinp)) جاری;
                    Ast_گرہ_DType* ثابت typep = VN_AS(pinp->exprp(), گرہ_DType);
                    longname += "_" + paramSmallName(srcModp, modptp)
                                + param_قدر_نمبر(typep->skip_Ref_مقصد_Non_Refp());
                    any_overrides = سچ;
                    جاری;
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
        منطقی defaultsResolved = جھوٹ;
        اگر (!any_overrides && !VN_IS(srcModp, Iface) && srcModp->hasGParam()) {
            برائے (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                اگر (pinp->modPTypep()) {
                    any_overrides = سچ;
                    defaultsResolved = سچ;
                    توڑو;
                }
            }
        }
        اگر (!any_overrides && VN_IS(nodep, Cell) && hasDescendantDefparams(srcModp)) {
            longname += "__Vdefparam" + V3Hash{srcModp->someInstanceName()}.مقصد_String();
            any_overrides = سچ;
        }

        UINFO(9,
              "nodeDeparamCommon: " << srcModp->prettyNameQ() << " overrides=" << any_overrides);

        Ast_گرہ_Module* newModp = خالی_اشاریہ;
        اگر (m_hier_بلاکیں.hier_Sub_تسلسل() && m_hier_بلاکیں.is_Hier_بلاک(srcModp->origName())) {
            Ast_گرہ_Module* ثابت paramedModp
                = m_hier_بلاکیں.ڈھونڈو_By_Params(srcModp->origName(), paramsp, m_modp);
            اگر (!paramedModp) واپس خالی_اشاریہ;  // Unsupported, already reported
            paramedModp->dead(جھوٹ);
            // We need to relink the pins to the new module
            relinkPinsByName(pinsp, paramedModp);
            newModp = paramedModp;
            // any_overrides = true;  // Unused later, so not needed
        } ورنہ اگر (!any_overrides) {
            UINFO(9, "nodeDeparamCommon: no overrides, reusing " << srcModp);
            UINFO(8, "Cell parameters all match original values, skipping expansion.");
            // If it's the first use of the default instance, create a copy and store it in user3p.
            // user3p will also be used to check if the default instance is used.
            اگر (!srcModp->user3p() && (VN_IS(srcModp, Class) || VN_IS(srcModp, Iface))) {
                Ast_گرہ_Module* گرہ_Copyp = srcModp->clone_درخت(جھوٹ);
                // It is a temporary copy of the original class node, stored in order to create
                // another instances. It is needed only during class instantiation.
                UINFO(8, "    Created clone " << گرہ_Copyp);
                m_deleter.ڈالو_Deletep(گرہ_Copyp);  // nodeCopyp used past here
                srcModp->user3p(گرہ_Copyp);
                storeOriginalParams(گرہ_Copyp);
            }
            newModp = srcModp;
        } ورنہ {
            ثابت string newname
                = srcModp->hier_بلاک() ? longname : moduleCalcName(srcModp, longname);
            ثابت ModInfo* ثابت modInfop
                = module_ڈھونڈو_Or_Clone(srcModp, nodep, paramsp, newname, ifaceRefRefs);
            اگر (!modInfop) واپس خالی_اشاریہ;
            // We need to relink the pins to the new module
            relinkPinsByName(pinsp, modInfop->m_modp);
            UINFO(8, "     Done with " << modInfop->m_modp);
            newModp = modInfop->m_modp;
        }

        ثابت منطقی cloned = (newModp != srcModp);
        UINFO(9, "nodeDeparamCommon result: " << newModp->prettyNameQ() << " cloned=" << cloned);

        // ParamVisitor skips the body of a class still marked hasGParam(), relying on it
        // being visited through a reference instead.  A class reached only by a deferred
        // class-scoped reference (e.g. the 'C#(V)::t' default of a type parameter that the
        // instantiation overrides) is never visited, so record specializations here.
        // user2() is set once processWorkQ has elaborated the body, which needs no re-queue.
        AstClass* ثابت newClassp = VN_CAST(newModp, Class);
        اگر (newClassp && !newClassp->user2()) m_specializedClassps.ڈالو_عقب(newClassp);

        // Link source class to its specialized version for later relinking of method references
        اگر (defaultsResolved) srcModp->user4p(newModp);

        برائے (خودکار* stmtp = newModp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            اگر (AstParamTypeDType* dtypep = VN_CAST(stmtp, ParamTypeDType)) {
                اگر (VN_IS(dtypep->skipRefOrNullp(), VoidDType)) {
                    nodep->v3error(
                        "Class parameter type without default value is never given value"
                        << " (IEEE 1800-2023 6.20.1): " << dtypep->prettyNameQ());
                }
            }
            اگر (AstVar* ثابت varp = VN_CAST(stmtp, Var)) {
                اگر (VN_IS(newModp, Class) && varp->isParam() && !varp->valuep()) {
                    nodep->v3error("Class parameter without default value is never given value"
                                   << " (IEEE 1800-2023 6.20.1): " << varp->prettyNameQ());
                }
            }
        }

        generic_Interface_Var_تیاری(paramsp, pinsp);

        // Delete the parameters from the cell; they're not relevant any longer.
        اگر (paramsp) VL_DO_DANGLING(paramsp->unlink_Fr_عقب_With_اگلا()->delete_درخت(), paramsp);
        واپس newModp;
    }

    Ast_گرہ_Module* cellDeparam(AstCell* nodep, Ast_گرہ_Module* srcModp) {
        // Must always clone __Vrcm (recursive modules)
        Ast_گرہ_Module* ثابت newModp = گرہ_Deparam_Common(nodep, srcModp, nodep->paramsp(),
                                                         nodep->pinsp(), nodep->recursive());
        اگر (!newModp) واپس خالی_اشاریہ;
        nodep->modp(newModp);  // Might be unchanged if not cloned (newModp == srcModp)
        nodep->modName(newModp->name());
        nodep->recursive(جھوٹ);
        واپس newModp;
    }
    Ast_گرہ_Module* ifaceRefDeparam(AstIfaceRefDType* ثابت nodep, Ast_گرہ_Module* srcModp) {
        // Check for self-reference pattern: typedef iface#(T) this_type inside interface iface
        // When processing inside a specialized interface, the IfaceRefDType should point to
        // the owner interface, not create an intermediate specialization.
        اگر (m_modp && VN_IS(m_modp, Iface)) {
            AstIface* ownerIfacep = const_cast<AstIface*>(VN_AS(m_modp, Iface));
            ثابت string ownerOrigName
                = ownerIfacep->origName().خالی_خانے() ? ownerIfacep->name() : ownerIfacep->origName();
            ثابت string srcOrigName
                = srcModp->origName().خالی_خانے() ? srcModp->name() : srcModp->origName();
            string owner_بنیاد_Name = ownerOrigName;
            ثابت سائز_t ownerPos = owner_بنیاد_Name.ڈھونڈو("__");
            اگر (ownerPos != string::npos) owner_بنیاد_Name = owner_بنیاد_Name.substr(0, ownerPos);
            string src_بنیاد_Name = srcOrigName;
            ثابت سائز_t srcPos = src_بنیاد_Name.ڈھونڈو("__");
            اگر (srcPos != string::npos) src_بنیاد_Name = src_بنیاد_Name.substr(0, srcPos);

            اگر (owner_بنیاد_Name == src_بنیاد_Name) {
                منطقی allOwnParams = سچ;
                برائے (AstPin* pinp = nodep->paramsp(); pinp && allOwnParams;
                     pinp = VN_AS(pinp->nextp(), Pin)) {
                    اگر (AstRefDType* ثابت refp = VN_CAST(pinp->exprp(), RefDType)) {
                        اگر (AstParamTypeDType* ثابت ptdp
                            = VN_CAST(refp->refDTypep(), ParamTypeDType)) {
                            Ast_گرہ_Module* ثابت ptdOwnerp
                                = V3LinkDotIfaceCapture::ڈھونڈو_Owner_Module(ptdp);
                            اگر (ptdOwnerp != m_modp) allOwnParams = جھوٹ;
                        } ورنہ {
                            pinp->v3error(  // LCOV_EXCL_LINE
                                "Self-referencing interface typedef "
                                "parameter is not a type parameter of "
                                "the enclosing interface");
                        }
                    } ورنہ {
                        pinp->v3error(  // LCOV_EXCL_LINE
                            "Self-referencing interface typedef "
                            "parameter is not a type reference");
                    }
                }
                اگر (allOwnParams) {
                    UINFO(5, "ifaceRefDeparam: self-reference pattern detected in "
                                 << ownerIfacep->prettyNameQ() << ", using owner interface");
                    V3Stats::جوڑو_Stat_مجموع("Param, Self-reference iface typedefs", 1);
                    nodep->ifacep(ownerIfacep);
                    اگر (nodep->paramsp()) nodep->paramsp()->unlink_Fr_عقب_With_اگلا()->delete_درخت();
                    واپس ownerIfacep;
                }
            }
        }

        Ast_گرہ_Module* ثابت newModp
            = گرہ_Deparam_Common(nodep, srcModp, nodep->paramsp(), خالی_اشاریہ, جھوٹ);
        اگر (!newModp) واپس خالی_اشاریہ;
        nodep->ifacep(VN_AS(newModp, Iface));
        واپس newModp;
    }
    Ast_گرہ_Module* classRefDeparam(AstClassOrPackageRef* nodep, Ast_گرہ_Module* srcModp) {
        resolveDefaultParams(nodep);
        Ast_گرہ_Module* ثابت newModp
            = گرہ_Deparam_Common(nodep, srcModp, nodep->paramsp(), خالی_اشاریہ, جھوٹ);
        اگر (!newModp) واپس خالی_اشاریہ;
        nodep->classOrPackagep(newModp);  // Might be unchanged if not cloned (newModp == srcModp)

        // If this ClassOrPackageRef is a child of a RefDType (e.g., typedef class#(T)::member_t),
        // resolve the RefDType's typedef to point to the typedef inside the specialized class
        AstRefDType* ثابت refDTypep = VN_CAST(nodep->backp(), RefDType);
        AstClass* ثابت newClassp = refDTypep ? VN_CAST(newModp, Class) : خالی_اشاریہ;
        اگر (newClassp && !refDTypep->typedefp() && !refDTypep->subDTypep()) {
            اگر (AstTypedef* ثابت typedefp
                = VN_CAST(m_memberMap.ڈھونڈو_Member(newClassp, refDTypep->name()), Typedef)) {
                refDTypep->typedefp(typedefp);
                refDTypep->classOrPackagep(newClassp);
                UINFO(9, "Resolved parameterized class typedef: " << refDTypep->name() << " -> "
                                                                  << typedefp << " in "
                                                                  << newClassp->name());
            }
        }
        واپس newModp;
    }
    Ast_گرہ_Module* classRefDeparam(AstClassRefDType* nodep, Ast_گرہ_Module* srcModp) {
        resolveDefaultParams(nodep);
        Ast_گرہ_Module* ثابت newModp
            = گرہ_Deparam_Common(nodep, srcModp, nodep->paramsp(), خالی_اشاریہ, جھوٹ);
        اگر (!newModp) واپس خالی_اشاریہ;
        AstClass* ثابت newClassp = VN_AS(newModp, Class);
        nodep->classp(newClassp);  // Might be unchanged if not cloned (newModp == srcModp)
        nodep->classOrPackagep(newClassp);
        واپس newClassp;
    }

    // Resolve deferred class-scoped references in post-order. Resolution may
    // deparameterize a class and delete its parameter pins, so no pointer to a
    // child may remain pending when its parent is resolved.
    جماعت DeferredResolverVisitor final : عوامی VNVisitor {
        ParamProcessor& m_processor;  // Processor used to resolve deferred references
        std::سیٹ<ثابت Ast_گرہ*> m_reachedDecls;

        منطقی پہلا_رسائی(ثابت Ast_گرہ* ثابت declp) { واپس m_reachedDecls.داخل_کرو(declp).دوسرا; }

        خالی iterateParamType(AstParamTypeDType* ثابت paramTypep) {
            اگر (پہلا_رسائی(paramTypep)) iterate(paramTypep);
        }
        خالی iterateTypedef(AstTypedef* ثابت tdefp) {
            اگر (پہلا_رسائی(tdefp)) iterate(tdefp->subDTypep());
        }

        خالی دورہ(Ast_گرہ* nodep) override { iterateChildren(nodep); }
        خالی دورہ(AstClassOrPackageRef* nodep) override {
            iterateChildren(nodep);
            Ast_گرہ* ثابت targetp = nodep->classOrPackageNodep();
            اگر (AstTypedef* ثابت tdefp = VN_CAST(targetp, Typedef)) {
                iterateTypedef(tdefp);
            } ورنہ اگر (AstParamTypeDType* ثابت paramTypep = VN_CAST(targetp, ParamTypeDType)) {
                iterateParamType(paramTypep);
            }
        }
        خالی دورہ(AstDot* nodep) override {
            iterateChildren(nodep);
            m_processor.resolve_Dot_مقصد_Typedef(nodep);
        }
        خالی دورہ(AstRefDType* nodep) override {
            iterateChildren(nodep);
            AstTypedef* ثابت tdefp = nodep->typedefp();
            اگر (tdefp) {
                iterateTypedef(tdefp);
            } ورنہ اگر (AstParamTypeDType* ثابت paramTypep
                       = VN_CAST(nodep->refDTypep(), ParamTypeDType)) {
                iterateParamType(paramTypep);
            }
            m_processor.resolveParamClassRefDType(nodep);
        }
        خالی دورہ(AstVarRef* nodep) override {
            iterateChildren(nodep);
            AstVar* ثابت varp = nodep->varp();
            اگر (پہلا_رسائی(varp)) {
                iterate(varp->subDTypep());
                ثابت خودکار& deferredVarps = v3Global.rootp()->deferredParamVarps();
                اگر (varp->varType() == VVarType::LPARAM && deferredVarps.گنتی(varp)) {
                    UASSERT_OBJ(varp->valuep(), varp, "VarRef should have non-null valuep");
                    iterate(varp->valuep());
                }
            }
        }

    عوامی:
        DeferredResolverVisitor(ParamProcessor& processor, Ast_گرہ* rootp)
            : m_processor{processor} {
            iterate(rootp);
        }
    };

عوامی:
    // After an interface cell inside parentModp has been deparameterized
    // (rewired from template to clone), retarget REFDTYPEs that still
    // reference the old template's types so that $bits(iface_typedef)
    // evaluates with the clone's widths.
    خالی retargetIfaceRefs(Ast_گرہ_Module* والد_Modp, ثابت string& cellName) {
        Ast_گرہ_Module* ثابت correctModp
            = V3LinkDotIfaceCapture::follow_Cell_راستہ(والد_Modp, cellName);
        اگر (!correctModp || correctModp->dead() || correctModp->parameterizedTemplate()) واپس;
        والد_Modp->foreach([&](AstRefDType* refp) {
            ثابت VIface_Capture_لیبل* ثابت tagp = V3LinkDotIfaceCapture::capture_لیبل(refp);
            اگر (!tagp || tagp->m_cell_راستہ != cellName) واپس;
            // Only retarget REFDTYPEs owned by parentModp.
            اگر (V3LinkDotIfaceCapture::ڈھونڈو_Owner_Module(refp) != والد_Modp) واپس;
            اگر (V3LinkDotIfaceCapture::retarget_Ref_مقصد_Module(refp, correctModp)) {
                UINFO(9, "retargetIfaceRefs: " << refp << " -> " << correctModp->prettyNameQ());
            }
        });
    }

    // Resolve deferred class-scoped values and types reachable from rootp. In
    // addition to AST children, follow typedefs, parameter types, deferred
    // localparam values, and referenced-variable dtypes. This makes the walk
    // independent of the particular dtype wrappers around a RefDType.
    خالی resolveDeferredDotsReachableFrom(Ast_گرہ* rootp, ثابت Ast_گرہ_Module* modp) {
        VL_RESTORER(m_modp);
        m_modp = modp;
        DeferredResolverVisitor{*یہ, rootp};
    }

    Ast_گرہ_Module* گرہ_Deparam(Ast_گرہ* nodep, Ast_گرہ_Module* srcModp, Ast_گرہ_Module* modp,
                               ثابت string& someInstanceName) {
        // Return new or reused de-parameterized module
        m_modp = modp;
        // Cell: Check for parameters in the instantiation.
        // We always run this, even if no parameters, as need to look for interfaces,
        // and remove any recursive references
        UINFO(4, "De-parameterize: " << nodep);
        UINFO(9, "nodeDeparam ENTER node=<"
                     << Ast_گرہ::گرہ_Addr(nodep) << ">" << " type=" << nodep->typeName()
                     << " srcMod=" << (srcModp ? srcModp->prettyNameQ() : "'<null>'")
                     << " srcSomeInstanceName='"
                     << (srcModp ? srcModp->someInstanceName() : string("<null>")) << "'"
                     << " parentMod=" << (modp ? modp->prettyNameQ() : "'<null>'")
                     << " parentSomeInstanceName='"
                     << (modp ? modp->someInstanceName() : string("<null>")) << "'"
                     << " inputSomeInstanceName='" << someInstanceName << "'");
        string گرہ_Name = nodep->name();
        اگر (AstIfaceRefDType* ثابت ifaceRefp = VN_CAST(nodep, IfaceRefDType)) {
            اگر (گرہ_Name.خالی_خانے()) گرہ_Name = ifaceRefp->cellName();
        }
        ثابت string instanceName
            = گرہ_Name.خالی_خانے() ? someInstanceName : (someInstanceName + "." + گرہ_Name);

        اگر (AstCell* ثابت cellp = VN_CAST(nodep, Cell)) {
            برائے (AstPin* pinp = cellp->paramsp(); pinp;) {
                AstPin* ثابت nextp = VN_AS(pinp->nextp(), Pin);
                اگر (!pinp->param_راستہ().خالی_خانے() && instanceName != pinp->param_راستہ()
                    && !VString::سرے_With(instanceName, "." + pinp->param_راستہ())) {
                    VL_DO_DANGLING(pinp->unlink_Fr_عقب()->delete_درخت(), pinp);
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
                     << srcModp->someInstanceName() << "'" << " node=<" << Ast_گرہ::گرہ_Addr(nodep)
                     << ">" << " nodeType=" << nodep->typeName());

        Ast_گرہ_Module* newModp = خالی_اشاریہ;
        اگر (AstCell* ثابت cellp = VN_CAST(nodep, Cell)) {
            newModp = cellDeparam(cellp, srcModp);
        } ورنہ اگر (AstIfaceRefDType* ثابت ifaceRefDTypep = VN_CAST(nodep, IfaceRefDType)) {
            newModp = ifaceRefDeparam(ifaceRefDTypep, srcModp);
        } ورنہ اگر (AstClassRefDType* ثابت classRefp = VN_CAST(nodep, ClassRefDType)) {
            newModp = classRefDeparam(classRefp, srcModp);
        } ورنہ اگر (AstClassOrPackageRef* ثابت classRefp = VN_CAST(nodep, ClassOrPackageRef)) {
            newModp = classRefDeparam(classRefp, srcModp);
        } ورنہ {
            nodep->v3fatalSrc("Expected module parameterization");
        }
        // 'newModp' might be nullptr on error
        اگر (!newModp) واپس خالی_اشاریہ;

        // Set name for later warnings
        newModp->someInstanceName(instanceName);

        UINFO(9, "nodeDeparam EXIT  node=<"
                     << Ast_گرہ::گرہ_Addr(nodep) << ">" << " type=" << nodep->typeName()
                     << " srcMod=" << (srcModp ? srcModp->prettyNameQ() : "'<null>'")
                     << " srcSomeInstanceName='"
                     << (srcModp ? srcModp->someInstanceName() : string("<null>")) << "'"
                     << " newMod=" << (newModp ? newModp->prettyNameQ() : "'<null>'")
                     << " newSomeInstanceName='"
                     << (newModp ? newModp->someInstanceName() : string("<null>")) << "'");

        UINFO(8, "     Done with orig " << nodep);
        // if (debug() >= 10)
        // v3Global.rootp()->dumpTreeFile(v3Global.debugFilename("param-out.tree"));
        واپس newModp;
    }

    // Return, and forget, the classes specialized since the previous call
    std::سمتیہ<AstClass*> takeSpecializedClassps() {
        std::سمتیہ<AstClass*> لیا_گیا;
        لیا_گیا.تبادلہ(m_specializedClassps);
        واپس لیا_گیا;
    }

    // CONSTRUCTORS
    صریح ParamProcessor(AstNetlist* nodep)
        : m_hier_بلاکیں{v3Global.opt.hier_بلاکیں(), nodep} {
        برائے (Ast_گرہ_Module* modp = nodep->modulesp(); modp;
             modp = VN_AS(modp->nextp(), گرہ_Module)) {
            m_all_Module_نام.داخل_کرو(modp->name());
        }
    }
    ~ParamProcessor() = طےشدہ;
    VL_UNCOPYABLE(ParamProcessor);
};

//######################################################################
// Record classes that are referenced with parameter pins

جماعت ClassRefUnlinkerVisitor final : عوامی VNVisitor {

عوامی:
    صریح ClassRefUnlinkerVisitor(AstNetlist* netlistp) { iterate(netlistp); }
    خالی دورہ(AstClassOrPackageRef* nodep) override {
        اگر (nodep->paramsp()) {
            اگر (AstClass* ثابت classp = VN_CAST(nodep->classOrPackageSkipp(), Class)) {
                اگر (!classp->user3p()) {
                    // Check if this ClassOrPackageRef is the lhsp of a DOT node
                    AstDot* ثابت dotp = VN_CAST(nodep->backp(), Dot);
                    اگر (dotp && dotp->lhsp() == nodep) {
                        // Replace DOT with just its rhsp to avoid leaving DOT with null lhsp
                        dotp->replaceWith(dotp->rhsp()->unlink_Fr_عقب());
                        VL_DO_DANGLING2(ڈالو_Deletep(dotp), dotp, nodep);
                    } ورنہ {
                        VL_DO_DANGLING(ڈالو_Deletep(nodep->unlink_Fr_عقب()), nodep);
                    }
                }
            }
        }
    }
    خالی دورہ(AstClass* nodep) override {}  // don't iterate inside classes
    خالی دورہ(Ast_گرہ* nodep) override { iterateChildren(nodep); }
};

//######################################################################
//  Relink RefDType nodes to point to parameterized classes' type parameters

جماعت ParamClassRefDTypeRelinkVisitor final : عوامی VNVisitor {

    AstClass* m_classp = خالی_اشاریہ;
    std::unordered_map<ثابت AstParamTypeDType*, AstClass*> m_paramTypeClassMap;

    // Owner-tracking state for the post-clone REFDTYPE typedef retargeting
    // (see retargetRefDType()). m_ownerModp is the most recently entered
    // AstNodeModule (or AstClass). m_origNameToClone / m_ambiguous are
    // rebuilt lazily on the first AstRefDType visit per owner; the
    // m_ownerMapForp cache key avoids rebuilding when the owner is unchanged
    // (e.g. across siblings under the same parent).
    Ast_گرہ_Module* m_ownerModp = خالی_اشاریہ;
    Ast_گرہ_Module* m_ownerMapForp = خالی_اشاریہ;
    std::unordered_map<std::string, AstClass*> m_orig_Name_مقصد_Clone;
    std::unordered_سیٹ<std::string> m_ambiguous;

    خالی ensureOwnerMap() {
        اگر (m_ownerMapForp == m_ownerModp) واپس;
        m_ownerMapForp = m_ownerModp;
        m_orig_Name_مقصد_Clone.clear();
        m_ambiguous.clear();
        اگر (!m_ownerModp) واپس;
        // Skip bare/generic class templates: their bodies are not
        // authoritative for any specialization, and their self-typedefs
        // (e.g. `typedef cls#(P,Q) this_type;` inside a parameterized class)
        // would otherwise be treated as ground truth and cause REFDTYPEs in
        // the body to be retargeted at the bare template.
        اگر (AstClass* ثابت ownerClassp = VN_CAST(m_ownerModp, Class)) {
            اگر (ownerClassp->hasGParam()) واپس;
        }
        برائے (Ast_گرہ* sp = m_ownerModp->stmtsp(); sp; sp = sp->nextp()) {
            AstTypedef* ثابت tdp = VN_CAST(sp, Typedef);
            اگر (!tdp) جاری;
            AstClassRefDType* ثابت crdtp = VN_CAST(tdp->subDTypep(), ClassRefDType);
            اگر (!crdtp || !crdtp->classp()) جاری;
            AstClass* ثابت targetp = crdtp->classp();
            // A typedef pointing at a still-generic template is a forward
            // declaration / unresolved reference, not ground truth.
            اگر (targetp->hasGParam()) جاری;
            ثابت std::string origName
                = targetp->origName().خالی_خانے() ? targetp->name() : targetp->origName();
            ثابت خودکار pair = m_orig_Name_مقصد_Clone.emplace(origName, targetp);
            // Two locally-resolved typedefs pointing at different
            // specializations of the same template -> ambiguous; we cannot
            // safely guess which one a stripped REFDTYPE meant.
            اگر (!pair.دوسرا && pair.پہلا->دوسرا != targetp) m_ambiguous.داخل_کرو(origName);
        }
    }

    // A class's name before specialization (e.g. "holder" for "holder__Tz1").
    ساکن string classOrigName(ثابت AstClass* classp) {
        واپس classp->origName().خالی_خانے() ? classp->name() : classp->origName();
    }

    // Find 'name' in classp or any base class (findTypedefInModule() searches
    // only the class itself).
    ساکن AstTypedef* ڈھونڈو_Typedef_With_Bases(AstClass* classp, ثابت string& name) {
        برائے (AstClass* cp = classp; cp; cp = cp->extendsp() ? cp->extendsp()->classp() : خالی_اشاریہ) {
            اگر (AstTypedef* ثابت tdp = V3LinkDotIfaceCapture::ڈھونڈو_Typedef_اندر_Module(cp, name)) {
                اگر (tdp->subDTypep()) واپس tdp;
            }
        }
        واپس خالی_اشاریہ;
    }

    // Re-resolve REFDTYPE.typedefp/refDTypep using the containing module's
    // own resolved typedef chain. The eager retargeting in deepCloneModule
    // blindly retargets every captured entry to whichever sibling clone is
    // being created, so when a referenced parameterized class is cloned more
    // than once (e.g. driver -> driver__I5 + driver__I6), the second sibling
    // stomps the bindings made by the first. By this point each owner's
    // typedefs have been correctly bound (via classRefDeparam), so we use
    // those local typedefs as ground truth.
    خالی retargetRefDType(AstRefDType* refp) {
        اگر (!m_ownerModp) واپس;

        // IEEE 1800-2023 8.25.1: a bare class-qualified reference to the class
        // being compiled means the current specialization, but V3LinkDot binds
        // it to the default instance, so a typedef reached through it widens
        // with the template's defaults (#8348). Rebind it to the
        // specialization's own typedef.
        اگر (AstClass* ثابت ownerClassp = VN_CAST(m_ownerModp, Class)) {
            اگر (!ownerClassp->hasGParam() && !refp->paramsp()) {
                // What the reference resolves to: its typedef's owner if
                // linked, else the class it is qualified by.
                ثابت AstClass* refClassp = خالی_اشاریہ;
                اگر (AstTypedef* ثابت tdp = refp->typedefp()) {
                    refClassp = VN_CAST(V3LinkDotIfaceCapture::ڈھونڈو_Owner_Module(tdp), Class);
                } ورنہ اگر (ثابت AstClassOrPackageRef* ثابت classRefp
                           = VN_CAST(refp->classOrPackageOpp(), ClassOrPackageRef)) {
                    refClassp = VN_CAST(classRefp->classOrPackageSkipp(), Class);
                }
                // Same originating class => bare self reference (another
                // specialization would carry #(), rejected above).
                اگر (refClassp && refClassp != ownerClassp
                    && classOrigName(refClassp) == classOrigName(ownerClassp)) {
                    AstTypedef* ثابت selfTdp = ڈھونڈو_Typedef_With_Bases(ownerClassp, refp->name());
                    اگر (selfTdp) {
                        UINFO(9, "post-param REFDTYPE self-reference retarget: "
                                     << refp << " from " << refClassp->name() << " to "
                                     << ownerClassp->name());
                        refp->typedefp(selfTdp);
                        refp->classOrPackagep(V3LinkDotIfaceCapture::ڈھونڈو_Owner_Module(selfTdp));
                        refp->refDTypep(selfTdp->subDTypep());
                        واپس;
                    }
                }
            }
        }

        AstTypedef* ثابت oldTdp = refp->typedefp();
        اگر (!oldTdp) واپس;
        AstClass* ثابت oldOwnerp = VN_CAST(V3LinkDotIfaceCapture::ڈھونڈو_Owner_Module(oldTdp), Class);
        اگر (!oldOwnerp) واپس;

        ensureOwnerMap();
        اگر (m_orig_Name_مقصد_Clone.خالی_خانے()) واپس;
        ثابت std::string origName
            = oldOwnerp->origName().خالی_خانے() ? oldOwnerp->name() : oldOwnerp->origName();
        اگر (m_ambiguous.گنتی(origName)) {
            // The owner has two locally-resolved typedefs pointing at
            // different specializations of the same template, e.g.:
            //   typedef driver #(.IW(5)) drv5_t;
            //   typedef driver #(.IW(6)) drv6_t;
            // A stripped REFDTYPE only carries the origName, so we cannot
            // tell which sibling it meant; leave it alone.
            UINFO(9, "post-param REFDTYPE retarget skipped (ambiguous owner '" << origName
                                                                               << "'): " << refp);
            واپس;
        }
        ثابت خودکار it = m_orig_Name_مقصد_Clone.ڈھونڈو(origName);
        اگر (it == m_orig_Name_مقصد_Clone.انتہا()) واپس;
        AstClass* ثابت correctClassp = it->دوسرا;
        اگر (correctClassp == oldOwnerp) واپس;
        AstTypedef* ثابت newTdp
            = V3LinkDotIfaceCapture::ڈھونڈو_Typedef_اندر_Module(correctClassp, refp->name());
        اگر (!newTdp) {
            // The owner map says correctClassp is the right specialization,
            // but it does not declare a typedef named refp->name(). Caveats
            // of findTypedefInModule (e.g. typedef declared in a base class,
            // not the specialization we resolved to) can land us here.
            UINFO(9, "post-param REFDTYPE retarget skipped (no typedef '"
                         << refp->name() << "' in " << correctClassp->name() << "): " << refp);
            واپس;
        }
        // typedefp / classOrPackagep / refDTypep must agree. If newTdp lacks
        // a subDTypep we cannot keep all three in sync, so skip the entire
        // retarget rather than leaving the REFDTYPE half-updated.
        اگر (!newTdp->subDTypep()) {
            UINFO(9, "post-param REFDTYPE retarget skipped (newTdp has null subDTypep): " << refp);
            واپس;
        }
        refp->typedefp(newTdp);
        refp->classOrPackagep(correctClassp);
        refp->refDTypep(newTdp->subDTypep());
        UINFO(9, "post-param REFDTYPE re-resolve: " << refp << " from " << oldOwnerp->name()
                                                    << " to " << correctClassp->name());
    }

عوامی:
    صریح ParamClassRefDTypeRelinkVisitor(AstNetlist* netlistp) { iterate(netlistp); }
    خالی دورہ(Ast_گرہ_Module* nodep) override {
        VL_RESTORER(m_ownerModp);
        m_ownerModp = nodep;
        iterateChildren(nodep);
    }
    خالی دورہ(AstClass* nodep) override {
        VL_RESTORER(m_classp);
        VL_RESTORER(m_ownerModp);
        m_classp = nodep;
        m_ownerModp = nodep;
        برائے (Ast_گرہ* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            اگر (AstParamTypeDType* ثابت ptp = VN_CAST(stmtp, ParamTypeDType)) {
                m_paramTypeClassMap[ptp] = nodep;
            }
        }
        iterateChildren(nodep);
    }
    خالی دورہ(AstRefDType* nodep) override {
        iterateChildren(nodep);
        // Redirect refs whose refDTypep points at a stale ParamTypeDType in
        // a generic class template that has been replaced by a parametrized
        // clone.
        اگر (AstParamTypeDType* ثابت paramtypep = VN_CAST(nodep->refDTypep(), ParamTypeDType)) {
            ثابت خودکار it = m_paramTypeClassMap.ڈھونڈو(paramtypep);
            اگر (it != m_paramTypeClassMap.انتہا()) {
                AstClass* ثابت origClassp = it->دوسرا;
                اگر (origClassp->hasGParam() && !origClassp->user3p()) {
                    اگر (AstClass* ثابت parametrizedClassp
                        = VN_CAST(origClassp->user4p(), Class)) {
                        ثابت string paramName = paramtypep->name();
                        AstParamTypeDType* newParamTypep = خالی_اشاریہ;
                        برائے (Ast_گرہ* stmtp = parametrizedClassp->stmtsp(); stmtp;
                             stmtp = stmtp->nextp()) {
                            اگر (AstParamTypeDType* ثابت ptp = VN_CAST(stmtp, ParamTypeDType)) {
                                اگر (ptp->name() == paramName) {
                                    newParamTypep = ptp;
                                    توڑو;
                                }
                            }
                        }
                        اگر (newParamTypep) {
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
    خالی دورہ(Ast_گرہ* nodep) override { iterateChildren(nodep); }
};

//######################################################################
// Process parameter top state

جماعت ParamState final {
عوامی:
    // STATE - across all visitors
    std::سمتیہ<AstClass*> m_paramClasses;  // Parameterized classes
    std::سمتیہ<AstDot*> m_dots;  // Dot references to process
    // Work queue keyed by (!isIface, level) so interfaces are always processed
    // before non-interfaces.  This ensures interface clones have their types
    // properly widthed before any module that references those types.
    استعمال WQKey = std::pair<منطقی, صحیح>;
    std::multimap<WQKey, Ast_گرہ_Module*> m_work_قطار_اگلا;  // Modules left to process
    // Map from AstNodeModule to set of all AstNodeModules that instantiates it.
    std::unordered_map<Ast_گرہ_Module*, std::unordered_سیٹ<Ast_گرہ_Module*>> m_parentps;
};

//######################################################################
// Process parameter visitor

جماعت ParamVisitor final : عوامی VNVisitor {
    // NODE STATE
    // AstNodeModule::user1 -> bool: already fixed level (temporary)
    // AstNodeDType::user2  -> bool: already visited from typedef

    // STATE - across all visitors
    ParamState& m_state;  // Common state
    ParamProcessor m_processor;  // De-parameterize a cell, build modules
    GenForUnroller m_unroller;  // Unroller for AstGenFor

    منطقی m_iterateModule = جھوٹ;  // Iterating module body
    string m_unlinkedTxt;  // Text for AstUnlinkedRef
    std::multimap<منطقی, Ast_گرہ*> m_cellps;  // Cells left to process (in current module)
    std::unordered_map<ثابت Ast_گرہ*, std::string>
        m_gen_Hier_نام;  // Maps ast nodes to generated hierarchy names
    std::map<ثابت AstRefDType*, منطقی>
        m_isCircular;  // Stores information whether `AstRefDType` is circular
    استعمال VarsByName = std::unordered_map<std::string, AstVar*>;
    // Persists across modules; one specialized interface clone serves every module bound to it
    std::unordered_map<ثابت Ast_گرہ_Module*, VarsByName> m_ifaceParams;

    // STATE - for current visit position (use VL_RESTORER)
    Ast_گرہ_Module* m_modp = خالی_اشاریہ;  // Module iterating
    std::unordered_سیٹ<std::string> m_iface_پورٹ_نام;  // Interface port names in current module
    std::unordered_map<std::string, AstCell*>
        m_ifaceInstCells;  // Local interface instance cells in current module, keyed by name
    VarsByName m_modIfaceRefs;  // Interface-ref Vars in current module, keyed by name
    منطقی m_mod_Iface_Refs_مکمل = جھوٹ;  // m_modIfaceRefs has been gathered for m_modp
    string m_generateHierName;  // Generate portion of hierarchy name
    منطقی m_اندر_Generate_Cond = جھوٹ;  // Traversing a generate condition; see iterateGenerateCond

    // METHODS

    خالی processWorkQ() {
        UASSERT(!m_iterateModule, "Should not nest");
        std::multimap<ParamState::WQKey, Ast_گرہ_Module*> work_قطار;
        m_generateHierName = "";
        m_iterateModule = سچ;

        // Visit all cells under module, recursively
        جب (سچ) {
            // Classes specialized since the last pass still need their bodies elaborated
            برائے (AstClass* ثابت classp : m_processor.takeSpecializedClassps()) {
                m_state.m_work_قطار_اگلا.emplace(ParamState::WQKey{سچ, classp->level()}, classp);
            }
            اگر (work_قطار.خالی_خانے()) std::تبادلہ(work_قطار, m_state.m_work_قطار_اگلا);
            اگر (work_قطار.خالی_خانے()) توڑو;

            ثابت خودکار itm = work_قطار.cbegin();
            Ast_گرہ_Module* ثابت modp = itm->دوسرا;
            work_قطار.erase(itm);
            // Starting a new module, so what was learned about the last one no longer holds.
            v3Global.rootp()->clearContainingModules();

            // Process once; note user2 will be cleared on specialization, so we will do the
            // specialized module if needed
            اگر (!modp->user2_سیٹ_Once()) {
                UINFO(9, "processWorkQ module begin mod='"
                             << modp->name() << "' orig='" << modp->origName() << "'"
                             << " someInstanceName='" << modp->someInstanceName() << "'"
                             << " hasGParam=" << (modp->hasGParam() ? "yes" : "no")
                             << " user3p=" << (modp->user3p() ? "set" : "null")
                             << " dead=" << (modp->dead() ? "yes" : "no"));

                // TODO: this really should be an assert, but classes and hier_blocks are
                // special...
                اگر (modp->someInstanceName().خالی_خانے()) modp->someInstanceName(modp->origName());

                // Iterate the body
                {
                    VL_RESTORER(m_modp);
                    VL_RESTORER_CLEAR(m_iface_پورٹ_نام);
                    VL_RESTORER_CLEAR(m_ifaceInstCells);
                    VL_RESTORER_CLEAR(m_modIfaceRefs);
                    VL_RESTORER(m_mod_Iface_Refs_مکمل);
                    m_modp = modp;
                    m_mod_Iface_Refs_مکمل = جھوٹ;
                    iterateChildren(modp);
                }
            }

            // Process interface cells, then non-interface cells, which may reference an interface
            // cell.
            جب (!m_cellps.خالی_خانے()) {
                ثابت خودکار itim = m_cellps.cbegin();
                Ast_گرہ* ثابت cellp = itim->دوسرا;
                m_cellps.erase(itim);

                // Consume the generated hierarchy name for the node
                string genHierName;
                ثابت خودکار genHierIt = m_gen_Hier_نام.ڈھونڈو(cellp);
                اگر (genHierIt != m_gen_Hier_نام.انتہا()) {
                    genHierName = std::move(genHierIt->دوسرا);
                    m_gen_Hier_نام.erase(genHierIt);
                }

                Ast_گرہ_Module* srcModp = خالی_اشاریہ;
                اگر (ثابت AstCell* modCellp = VN_CAST(cellp, Cell)) {
                    srcModp = modCellp->modp();
                } ورنہ اگر (ثابت AstClassOrPackageRef* classRefp
                           = VN_CAST(cellp, ClassOrPackageRef)) {
                    srcModp = classRefp->classOrPackageSkipp();
                    اگر (VN_IS(classRefp->classOrPackageNodep(), ParamTypeDType)) جاری;
                } ورنہ اگر (ثابت AstClassRefDType* classRefp = VN_CAST(cellp, ClassRefDType)) {
                    srcModp = classRefp->classp();
                } ورنہ اگر (ثابت AstIfaceRefDType* ifaceRefp = VN_CAST(cellp, IfaceRefDType)) {
                    srcModp = ifaceRefp->ifacep();
                } ورنہ {
                    cellp->v3fatalSrc("Expected module parameterization");
                }
                اگر (!srcModp) جاری;

                // Update path
                ثابت string someInstanceName = modp->someInstanceName() + genHierName;

                // Apply parameter specialization
                اگر (Ast_گرہ_Module* ثابت newModp
                    = m_processor.گرہ_Deparam(cellp, srcModp, modp, someInstanceName)) {

                    اگر (VN_IS(srcModp, Iface)) {
                        logTemplateLeakRefs(modp, srcModp, "after queued nodeDeparam", cellp);
                        // After the interface cell is rewired to its clone,
                        // retarget REFDTYPEs in the parent module that still
                        // reference the template interface's types.
                        اگر (V3LinkDotIfaceCapture::enabled()) {
                            اگر (ثابت AstCell* ثابت modCellp = VN_CAST(cellp, Cell)) {
                                اگر (newModp != srcModp) {
                                    m_processor.retargetIfaceRefs(modp, modCellp->name());
                                }
                            }
                        }
                    }

                    // Add the (now potentially specialized) child module to the work queue
                    work_قطار.emplace(ParamState::WQKey{!VN_IS(newModp, Iface), newModp->level()},
                                      newModp);

                    // Add to the hierarchy registry
                    m_state.m_parentps[newModp].داخل_کرو(modp);

                    // Eagerly specialize nested iface cells so their types
                    // have correct widths before sibling module cells run.
                    اگر (VN_IS(newModp, Iface) && newModp != srcModp) {
                        specializeNestedIfaceCells(newModp);
                    }
                }
            }
        }

        m_iterateModule = جھوٹ;
    }

    // Extract the base reference name from a dotted VarXRef (e.g., "iface.FOO" -> "iface")
    ساکن string get_Ref_بنیاد_Name(ثابت AstVarXRef* refp) {
        ثابت string dotted = refp->dotted();
        اگر (dotted.خالی_خانے()) واپس "";
        واپس dotted.substr(0, dotted.ڈھونڈو('.'));
    }

    // Debug-only diagnostic (requires --debugi-V3Param 9).
    // Walks parentModp looking for RefDTypes or VarRefs whose typedef,
    // refDType, or variable target is still owned by templateModp (the
    // unspecialized interface template).  Any such "leak" indicates a
    // pointer that was not properly redirected to the clone during
    // deparameterization.  Logs each leak with ancestry for triage.
    خالی logTemplateLeakRefs(Ast_گرہ_Module* والد_Modp, Ast_گرہ_Module* templateModp,
                             ثابت حرف* stage, Ast_گرہ* contextp) {
        اگر (debug() < 9 || !والد_Modp || !templateModp || !VN_IS(templateModp, Iface)) واپس;
        // LCOV_EXCL_START  // Debug-only diagnostic
        صحیح leak_گنتی = 0;
        ثابت خودکار ancestry_کا = [](ثابت Ast_گرہ* nodep) {
            string ancestry;
            برائے (ثابت Ast_گرہ* curp = nodep; curp; curp = curp->backp()) {
                اگر (!ancestry.خالی_خانے()) ancestry += "<-";
                ancestry += curp->typeName();
                اگر (VN_IS(curp, گرہ_Module) || VN_IS(curp, Netlist) || VN_IS(curp, Type_جدول)) {
                    توڑو;
                }
            }
            واپس ancestry;
        };

        والد_Modp->foreach([&](AstRefDType* refp) {
            اگر (refp->typedefp()) {
                Ast_گرہ_Module* ثابت tdOwnerp
                    = V3LinkDotIfaceCapture::ڈھونڈو_Owner_Module(refp->typedefp());
                اگر (tdOwnerp == templateModp) {
                    ++leak_گنتی;
                    UINFO(9, "TEMPLATE-LEAK "
                                 << stage << " parent=" << والد_Modp->prettyNameQ()
                                 << " template=" << templateModp->prettyNameQ() << " contextType="
                                 << (contextp ? contextp->typeName() : string("<null>"))
                                 << " contextName="
                                 << (contextp ? contextp->prettyNameQ() : "'<null>'")
                                 << " leak=REFDTYPE typedef owner" << " ref=<"
                                 << Ast_گرہ::گرہ_Addr(refp) << ">" << " refName="
                                 << refp->prettyNameQ() << " ancestry=" << ancestry_کا(refp));
                }
            }
            اگر (refp->refDTypep()) {
                Ast_گرہ_Module* ثابت rdOwnerp
                    = V3LinkDotIfaceCapture::ڈھونڈو_Owner_Module(refp->refDTypep());
                اگر (rdOwnerp == templateModp) {
                    ++leak_گنتی;
                    UINFO(9, "TEMPLATE-LEAK "
                                 << stage << " parent=" << والد_Modp->prettyNameQ()
                                 << " template=" << templateModp->prettyNameQ() << " contextType="
                                 << (contextp ? contextp->typeName() : string("<null>"))
                                 << " contextName="
                                 << (contextp ? contextp->prettyNameQ() : "'<null>'")
                                 << " leak=REFDTYPE refDType owner" << " ref=<"
                                 << Ast_گرہ::گرہ_Addr(refp) << ">" << " refName="
                                 << refp->prettyNameQ() << " ancestry=" << ancestry_کا(refp));
                }
            }
        });

        والد_Modp->foreach([&](AstVarRef* varrefp) {
            اگر (!varrefp->varp()) واپس;
            Ast_گرہ_Module* ثابت varOwnerp
                = V3LinkDotIfaceCapture::ڈھونڈو_Owner_Module(varrefp->varp());
            اگر (varOwnerp != templateModp) واپس;
            ++leak_گنتی;
            UINFO(9, "TEMPLATE-LEAK "
                         << stage << " parent=" << والد_Modp->prettyNameQ()
                         << " template=" << templateModp->prettyNameQ()
                         << " contextType=" << (contextp ? contextp->typeName() : string("<null>"))
                         << " contextName=" << (contextp ? contextp->prettyNameQ() : "'<null>'")
                         << " leak=VARREF target owner" << " ref=<" << Ast_گرہ::گرہ_Addr(varrefp)
                         << ">" << " var=" << varrefp->prettyNameQ()
                         << " ancestry=" << ancestry_کا(varrefp));
        });

        اگر (leak_گنتی > 0) {
            UINFO(9, "TEMPLATE-LEAK summary stage='"
                         << stage << "' parent=" << والد_Modp->prettyNameQ()
                         << " template=" << templateModp->prettyNameQ() << " count=" << leak_گنتی);
        }
        // LCOV_EXCL_STOP
    }

    // Flag hierarchical refs in a parameter value. Single top-down pass.
    خالی checkParamNotHierRecurse(Ast_گرہ* nodep, منطقی underTypeQuery = جھوٹ) {
        برائے (; nodep; nodep = nodep->nextp()) {
            // Refs read only for their type ($bits etc.) are allowed
            ثابت Ast_Attr_کا* ثابت attrp = VN_CAST(nodep, Attr_کا);
            ثابت منطقی بچہ_Under_Query
                = underTypeQuery || (attrp && attrp->attrType().isTypeQuery());
            اگر (ثابت AstVarXRef* ثابت refp = VN_CAST(nodep, VarXRef)) {
                // Allow hierarchical ref to interface params through interface/modport ports
                // or local interface instances
                منطقی isIfaceRef = جھوٹ;
                اگر (refp->varp() && refp->varp()->isIfaceParam()) {
                    ثابت string refname = get_Ref_بنیاد_Name(refp);
                    isIfaceRef
                        = !refname.خالی_خانے()
                          && (m_iface_پورٹ_نام.گنتی(refname) || m_ifaceInstCells.گنتی(refname));
                }
                اگر (!isIfaceRef && !underTypeQuery) {
                    refp->v3warn(HIERPARAM, "Parameter values cannot use hierarchical values"
                                            " (IEEE 1800-2023 6.20.2)");
                }
            } ورنہ اگر (ثابت Ast_گرہ_FTask_Ref* ثابت refp = VN_CAST(nodep, گرہ_FTask_Ref)) {
                اگر (refp->dotted() != "") {
                    refp->v3error("Parameter values cannot call hierarchical functions"
                                  " (IEEE 1800-2023 6.20.2)");
                }
            }
            checkParamNotHierRecurse(nodep->op1p(), بچہ_Under_Query);
            checkParamNotHierRecurse(nodep->op2p(), بچہ_Under_Query);
            checkParamNotHierRecurse(nodep->op3p(), بچہ_Under_Query);
            checkParamNotHierRecurse(nodep->op4p(), بچہ_Under_Query);
        }
    }

    // Deparameterize and constify nested interface cells within ifaceModp.
    خالی specializeNestedIfaceCells(Ast_گرہ_Module* ifaceModp) {
        برائے (Ast_گرہ* stmtp = ifaceModp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstCell* ثابت nestedCellp = VN_CAST(stmtp, Cell);
            اگر (!nestedCellp) جاری;
            اگر (!VN_IS(nestedCellp->modp(), Iface)) جاری;
            اگر (!nestedCellp->paramsp()) جاری;

            Ast_گرہ_Module* ثابت nestedSrcModp = nestedCellp->modp();
            اگر (Ast_گرہ_Module* ثابت nestedNewModp = m_processor.گرہ_Deparam(
                    nestedCellp, nestedSrcModp, ifaceModp, ifaceModp->someInstanceName())) {
                اگر (nestedNewModp != nestedSrcModp) {
                    // Constify the nested clone's params so its types have correct widths.
                    برائے (Ast_گرہ* sp = nestedNewModp->stmtsp(); sp; sp = sp->nextp()) {
                        اگر (AstVar* ثابت varp = VN_CAST(sp, Var)) {
                            اگر (varp->isParam() && varp->valuep()) {
                                V3Const::constifyParamsEdit(varp);
                            }
                        }
                    }
                    // Retarget REFDTYPEs in the outer clone to the nested clone's types.
                    اگر (V3LinkDotIfaceCapture::enabled()) {
                        m_processor.retargetIfaceRefs(ifaceModp, nestedCellp->name());
                    }
                    specializeNestedIfaceCells(nestedNewModp);
                }
            }
        }
    }

    // Check if cell parameters reference interface ports or local interface instances
    منطقی cellParamsReferenceIfacePorts(AstCell* cellp) {
        اگر (!cellp->paramsp()) واپس جھوٹ;

        برائے (Ast_گرہ* nodep = cellp->paramsp(); nodep; nodep = nodep->nextp()) {
            اگر (AstPin* ثابت pinp = VN_CAST(nodep, Pin)) {
                اگر (Ast_گرہ* ثابت exprp = pinp->exprp()) {
                    اگر (ثابت AstVarXRef* ثابت refp = VN_CAST(exprp, VarXRef)) {
                        ثابت string refname = get_Ref_بنیاد_Name(refp);
                        اگر (!refname.خالی_خانے()
                            && (m_iface_پورٹ_نام.گنتی(refname)
                                || m_ifaceInstCells.گنتی(refname)))
                            واپس سچ;
                    }
                }
            }
        }
        واپس جھوٹ;
    }

    // Split interface array port 'portp' into a variable for each of its elements, in the
    // dimensions of 'dtypep' inwards. 'suffix' is the name suffix of the dimensions outside
    // 'dtypep'.
    خالی expand_Iface_Array_پورٹ_Dimensions(AstVar* portp, Ast_گرہ_DType* dtypep,
                                        ثابت std::string& suffix) {
        ثابت AstUnpackArrayDType* ثابت arrp = VN_CAST(dtypep->skipRefp(), UnpackArrayDType);
        // Base case: add the element variable when no dimensions left
        اگر (!arrp) {
            AstVar* ثابت varp = portp->clone_درخت(جھوٹ);
            varp->name(portp->name() + suffix);
            varp->origName(portp->origName() + suffix);
            اگر (Ast_گرہ_DType* ثابت oldp = varp->بچہ_DTypep()) {
                VL_DO_DANGLING(ڈالو_Deletep(oldp->unlink_Fr_عقب()), oldp);
            }
            varp->dtypep(خالی_اشاریہ);
            varp->بچہ_DTypep(portp->subDTypep()->elemDTypep()->clone_درخت(جھوٹ));
            portp->جوڑو_اگلا_یہاں(varp);
            واپس;
        }

        // Enumerate the current dimension given by 'arrp'
        // Each element is added right after 'portp', so go from right to left,
        // to end with an enumeration from the left index to the right index.
        برائے (ثابت صحیح n : arrp->declRange().seq_دایاں_مقصد_بایاں()) {
            ثابت std::string s = suffix + "__BRA__" + Ast_گرہ::encode_نمبر(n) + "__KET__";
            expand_Iface_Array_پورٹ_Dimensions(portp, arrp->subDTypep(), s);
        }
    }

    // Add the elements of instance array 'arrayedCellp' in the dimensions from 'rangep' inwards.
    // 'suffix' and 'idx' are the name suffix and row-major position (each dimension counted from
    // the left) of the dimensions outside 'rangep'. For an array of interfaces, 'ifaceVarp' is
    // the variable referencing the whole array (see V3LinkCells), which is also split into
    // variables referencing the elements.
    خالی expandCellArrayDimensions(AstCell* arrayedCellp, AstVar* ifaceVarp,
                                   ثابت AstRange* rangep, ثابت std::string& suffix, صحیح idx) {
        // Base case: insert the element when no dimensions left
        اگر (!rangep) {
            AstCell* ثابت elemp = arrayedCellp->clone_درخت(جھوٹ);
            elemp->name(arrayedCellp->name() + suffix);
            elemp->origName(arrayedCellp->origName() + suffix);
            elemp->arrayIdx(idx);
            arrayedCellp->جوڑو_اگلا_یہاں(elemp);
            اگر (ifaceVarp) {
                AstIfaceRefDType* ثابت irefp
                    = VN_AS(ifaceVarp->subDTypep()->elemDTypep(), IfaceRefDType)->clone_درخت(جھوٹ);
                irefp->cellp(elemp);
                irefp->cellName(elemp->name());
                // Named like the variable of any interface cell (see V3LinkCells)
                AstVar* ثابت varp = ifaceVarp->clone_درخت(جھوٹ);
                varp->name(elemp->name() + "__Viftop");
                varp->origName(elemp->origName() + "__Viftop");
                اگر (Ast_گرہ_DType* ثابت oldp = varp->بچہ_DTypep()) {
                    VL_DO_DANGLING(ڈالو_Deletep(oldp->unlink_Fr_عقب()), oldp);
                }
                varp->dtypep(خالی_اشاریہ);
                varp->بچہ_DTypep(irefp);
                ifaceVarp->جوڑو_اگلا_یہاں(varp);
            }
            واپس;
        }

        // Enumerate the current dimension given by 'rangep'
        // Each element is added right after 'arrayedCellp', so go from right to left,
        // to end with an enumeration from the left index to the right index.
        idx = (idx + 1) * rangep->elementsConst();
        ثابت AstRange* ثابت subRangep = VN_AS(rangep->nextp(), Range);
        برائے (ثابت صحیح n : rangep->seq_دایاں_مقصد_بایاں()) {
            ثابت std::string s = suffix + "__BRA__" + Ast_گرہ::encode_نمبر(n) + "__KET__";
            expandCellArrayDimensions(arrayedCellp, ifaceVarp, subRangep, s, --idx);
        }
    }

    // A generic visitor for cells and class refs
    خالی دورہ_Cell_Or_Class_Ref(Ast_گرہ* nodep, منطقی isIface) {
        // Must do ifaces first, so push to list and do in proper order
        m_gen_Hier_نام.emplace(nodep, m_generateHierName);

        // Deparameterize iface cells early so types are available for lparams.
        اگر (isIface && VN_CAST(nodep, Cell) && VN_CAST(nodep, Cell)->paramsp()) {
            AstCell* ثابت cellp = VN_CAST(nodep, Cell);
            اگر (!cellParamsReferenceIfacePorts(cellp)) {
                Ast_گرہ_Module* ثابت srcModp = cellp->modp();
                Ast_گرہ_Module* ثابت newModp
                    = m_processor.گرہ_Deparam(cellp, srcModp, m_modp, m_modp->someInstanceName());
                // Retarget template REFDTYPEs to the clone's types.
                اگر (V3LinkDotIfaceCapture::enabled() && cellp->modp() != srcModp) {
                    m_processor.retargetIfaceRefs(m_modp, cellp->name());
                }
                // Specialize nested iface cells so their types are correct.
                اگر (newModp && newModp != srcModp) { specializeNestedIfaceCells(newModp); }
            }
        }

        // Visit parameters in the instantiation.
        iterateChildren(nodep);
        // A generate condition is folded and deleted before the drain loop runs, so
        // queueing from one only leaves a dangling pointer.  See iterateGenerateCond.
        اگر (!m_اندر_Generate_Cond) m_cellps.emplace(!isIface, nodep);
    }

    // VISITORS
    خالی دورہ(Ast_گرہ_Module* nodep) override {
        اگر (nodep->recursiveClone()) nodep->dead(سچ);  // Fake, made for recursive elimination
        اگر (nodep->dead()) واپس;  // Marked by LinkDot (and above)
        UINFO(9, "V3Param: visit module name=" << nodep->prettyNameQ() << " orig='"
                                               << nodep->origName() << "' someInstanceName='"
                                               << nodep->someInstanceName() << "' hasGParam="
                                               << (nodep->hasGParam() ? "yes" : "no")
                                               << " user3p=" << (nodep->user3p() ? "set" : "null")
                                               << " dead=" << (nodep->dead() ? "yes" : "no"));
        اگر (AstClass* ثابت classp = VN_CAST(nodep, Class)) {
            اگر (classp->hasGParam()) {
                // Don't enter into a definition.
                // If a class is used, it will be visited through a reference and cloned
                m_state.m_paramClasses.ڈالو_عقب(classp);
                واپس;
            }
        }

        اگر (m_iterateModule) {  // Iterating from processWorkQ
            UINFO(4, " MOD-under-MOD.  " << nodep);
            // Delay until current module is done.
            // processWorkQ() (which we are returning to) will process nodep later
            m_state.m_work_قطار_اگلا.emplace(
                ParamState::WQKey{!VN_IS(nodep, Iface), nodep->level()}, nodep);
            واپس;
        }

        // Start traversal at root-like things
        اگر (nodep->is_چوٹی()  // Tops
            || VN_IS(nodep, Class)  //  Moved classes
            || VN_IS(nodep, Package)) {  // Likewise haven't done wrapTopPackages yet
            m_state.m_work_قطار_اگلا.emplace(
                ParamState::WQKey{!VN_IS(nodep, Iface), nodep->level()}, nodep);
            processWorkQ();
        }
    }

    منطقی isCircularType(ثابت AstRefDType* nodep) {
        ثابت خودکار iter = m_isCircular.emplace(nodep, سچ);
        اگر (!iter.دوسرا) واپس iter.پہلا->دوسرا;
        اگر (ثابت AstRefDType* ثابت subDTypep = VN_CAST(nodep->subDTypep(), RefDType)) {
            ثابت منطقی ret = isCircularType(subDTypep);
            iter.پہلا->دوسرا = ret;
            واپس ret;
        }
        iter.پہلا->دوسرا = جھوٹ;
        واپس جھوٹ;
    }

    خالی دورہ(AstRefDType* nodep) override {
        ثابت منطقی isCircular = isCircularType(nodep);
        // A module-body `typedef C#(Cfg)::t alias` the deferred pin/param walk
        // never reached would survive to V3Width unlinked. If its alias chain is
        // broken, run the same deferred resolution here.
        اگر (!isCircular && nodep->typedefp() && !nodep->skipRefOrNullp()) {
            m_processor.resolveDeferredDotsReachableFrom(nodep, m_modp);
        }
        اگر (isCircular) {
            nodep->v3error("Typedef's type is circular: " << nodep->prettyName());
        } ورنہ اگر (nodep->typedefp() && nodep->subDTypep()
                   && (VN_IS(nodep->subDTypep()->skipRefOrNullp(), IfaceRefDType)
                       || VN_IS(nodep->subDTypep()->skipRefOrNullp(), ClassRefDType))
                   && !nodep->skipRefp()->user2_سیٹ_Once()) {
            // The visit() function for every valid nodep->skipRefp() type below
            // must include a `user2SetOnce()` check to avoid adding duplicate
            // nodes to m_cellps
            iterate(nodep->skipRefp());
        }
        iterateChildren(nodep);
    }
    خالی دورہ(AstCell* nodep) override {
        اگر (nodep->rangep() && nodep->arrayIdx() < 0) {
            // Type check the ranges
            برائے (AstRange* rangep = nodep->rangep(); rangep;
                 rangep = VN_AS(rangep->nextp(), Range)) {
                rangep = VN_AS(V3_چوڑائی::چوڑائی_Params_Edit(rangep), Range);
            }
            // If it's an interface, pick up the corresponding AstIfaceRefDType variable
            AstVar* ثابت ifaceVarp = [&]() -> AstVar* {
                اگر (!VN_IS(nodep->modp(), Iface)) واپس خالی_اشاریہ;
                // The variable referencing the whole array, right after the cell (see V3LinkCells)
                AstVar* ثابت varp = VN_CAST(nodep->nextp(), Var);
                UASSERT_OBJ(varp && varp->name() == nodep->name() + "__Viftop", nodep,
                            "No __Viftop variable for interface array");
                واپس varp;
            }();
            // Expand the instance array into its elements
            expandCellArrayDimensions(nodep, ifaceVarp, nodep->rangep(), "", 0);
            اگر (ifaceVarp) {
                ifaceVarp->is_Iface_Array_بانٹو(سچ);
                // Until V3Width removes it, the whole array variable refers to the first
                // element, which is also used for accessing the parameters of the array
                AstCell* ثابت firstp = VN_AS(nodep->nextp(), Cell);
                AstIfaceRefDType* ثابت irefp
                    = VN_AS(ifaceVarp->subDTypep()->elemDTypep(), IfaceRefDType);
                irefp->cellp(firstp);
                irefp->cellName(firstp->name());
                m_ifaceInstCells.emplace(nodep->name(), firstp);
            }
            VL_DO_DANGLING(ڈالو_Deletep(nodep->unlink_Fr_عقب()), nodep);
            // The elements replaced the arrayed cell in place, so will be iterated next.
            واپس;
        }
        checkParamNotHierRecurse(nodep->paramsp());
        اگر (VN_IS(nodep->modp(), Iface)) m_ifaceInstCells.emplace(nodep->name(), nodep);
        دورہ_Cell_Or_Class_Ref(nodep, VN_IS(nodep->modp(), Iface));
    }
    خالی دورہ(AstIfaceRefDType* nodep) override {
        nodep->skipRefp()->user2_سیٹ_Once();
        اگر (nodep->ifacep()) دورہ_Cell_Or_Class_Ref(nodep, سچ);
    }
    خالی دورہ(AstClassRefDType* nodep) override {
        nodep->skipRefp()->user2_سیٹ_Once();
        checkParamNotHierRecurse(nodep->paramsp());
        دورہ_Cell_Or_Class_Ref(nodep, جھوٹ);
    }
    خالی دورہ(AstClassOrPackageRef* nodep) override {
        // If it points to a typedef it is not really a class reference. That typedef will be
        // visited anyway (from its parent node), so even if it points to a parameterized class
        // type, the instance will be created.
        اگر (!VN_IS(nodep->classOrPackageNodep(), Typedef)) دورہ_Cell_Or_Class_Ref(nodep, جھوٹ);
    }

    // Recurse into AstGenBlock as generate blocks aren't flattened until V3Begin::debeginAll,
    // well after V3Param runs
    ساکن خالی gatherVars(Ast_گرہ* stmtsp, منطقی (*matchp)(ثابت AstVar*), VarsByName& varsr) {
        برائے (Ast_گرہ* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
            اگر (AstVar* ثابت varp = VN_CAST(nodep, Var)) {
                // emplace, not assign, so the first declaration of a name wins
                اگر (matchp(varp)) varsr.emplace(varp->name(), varp);
            } ورنہ اگر (Ast_Gen_بلاک* ثابت genp = VN_CAST(nodep, Gen_بلاک)) {
                gatherVars(genp->itemsp(), matchp, varsr);
            }
        }
    }
    ثابت VarsByName& modIfaceRefs() {
        اگر (!m_mod_Iface_Refs_مکمل) {
            gatherVars(
                m_modp->stmtsp(), [](ثابت AstVar* varp) { واپس varp->isIfaceRef(); },
                m_modIfaceRefs);
            m_mod_Iface_Refs_مکمل = سچ;
        }
        واپس m_modIfaceRefs;
    }
    ثابت VarsByName& ifaceParams(Ast_گرہ_Module* ifacep) {
        ثابت خودکار pair = m_ifaceParams.emplace(ifacep, VarsByName{});
        اگر (pair.دوسرا) {
            gatherVars(
                ifacep->stmtsp(), [](ثابت AstVar* varp) { واپس varp->isParam(); },
                pair.پہلا->دوسرا);
        }
        واپس pair.پہلا->دوسرا;
    }
    // Resolve a generic-interface 'ifacePort.member' Dot left unlinked by V3LinkDot, now that
    // ifacePort may have been specialized. Returns nullptr if still unresolvable
    Ast_گرہ* tryResolveGenericIfaceDot(AstDot* dotp) {
        AstParseRef* ثابت lhsp = VN_CAST(dotp->lhsp(), ParseRef);
        AstParseRef* ثابت rhsp = VN_CAST(dotp->rhsp(), ParseRef);
        اگر (!lhsp || !rhsp) واپس خالی_اشاریہ;
        // Always called from visit(AstVar) inside processWorkQ()'s VL_RESTORER(m_modp) scope
        اگر (VL_UNCOVERABLE(!m_modp)) واپس خالی_اشاریہ;

        ثابت خودکار& refVars = modIfaceRefs();
        ثابت خودکار ifaceVarIt = refVars.ڈھونڈو(lhsp->name());
        // V3LinkDot only defers Dots whose lhs is a non-array iface-ref var, so this can't fail
        اگر (VL_UNCOVERABLE(ifaceVarIt == refVars.انتہا())) واپس خالی_اشاریہ;
        AstVar* ثابت ifaceVarp = ifaceVarIt->دوسرا;
        AstIfaceRefDType* ثابت ifacerefp = VN_CAST(ifaceVarp->subDTypep(), IfaceRefDType);
        اگر (VL_UNCOVERABLE(!ifacerefp || !ifacerefp->ifacep())) واپس خالی_اشاریہ;

        ثابت خودکار& ifaceVars = ifaceParams(ifacerefp->ifacep());
        ثابت خودکار ہدف_Var_It = ifaceVars.ڈھونڈو(rhsp->name());
        اگر (ہدف_Var_It == ifaceVars.انتہا()) واپس خالی_اشاریہ;
        AstVar* ثابت ہدف_Varp = ہدف_Var_It->دوسرا;
        // processWorkQ() visits interface cells first, so the interface is already constified
        اگر (VL_UNCOVERABLE(!VN_IS(ہدف_Varp->valuep(), Const))) iterate(ہدف_Varp);
        اگر (VL_UNCOVERABLE(!ہدف_Varp->valuep() || !VN_IS(ہدف_Varp->valuep(), Const))) {
            واپس خالی_اشاریہ;  // LCOV_EXCL_LINE
        }
        واپس ہدف_Varp->valuep()->clone_درخت(جھوٹ);
    }
    خالی resolve_Generic_Iface_Dots_اندر(Ast_گرہ* nodep) {
        // Collect first: the deleteTree() below frees a node foreach() would still read from
        std::سمتیہ<AstDot*> dotps;
        nodep->foreach([&](AstDot* dotp) { dotps.ڈالو_عقب(dotp); });
        برائے (AstDot* ثابت dotp : dotps) {
            اگر (Ast_گرہ* ثابت newp = tryResolveGenericIfaceDot(dotp)) {
                dotp->replaceWith(newp);
                VL_DO_DANGLING(dotp->delete_درخت(), dotp);
            }
        }
    }

    // Make sure all parameters are constantified
    خالی دورہ(AstVar* nodep) override {
        اگر (nodep->user2_سیٹ_Once()) واپس;  // Process once
        // Interface array port: split it into its elements
        اگر (nodep->isIfaceRef() && !nodep->is_Iface_والد()
            && (VN_IS(nodep->subDTypep()->skipRefp(), UnpackArrayDType)
                || VN_IS(nodep->subDTypep()->skipRefp(), BracketArrayDType))) {
            // Also converts any C-style [N] dimensions, the ranges must be constant
            V3_چوڑائی::چوڑائی_Params_Edit(nodep->subDTypep());
            expand_Iface_Array_پورٹ_Dimensions(nodep, nodep->subDTypep(), "");
            nodep->is_Iface_Array_بانٹو(سچ);
        }
        // Build cache of interface port names as we encounter them
        اگر (nodep->isIfaceRef()) { m_iface_پورٹ_نام.داخل_کرو(nodep->name()); }
        iterateChildren(nodep);
        اگر (nodep->isParam()) {
            checkParamNotHierRecurse(nodep->valuep());
            اگر (!nodep->valuep() && !VN_IS(m_modp, Class)) {
                nodep->v3error("Parameter without default value is never given value"
                               << " (IEEE 1800-2023 6.20.1): " << nodep->prettyNameQ());
            } ورنہ اگر (nodep->valuep()) {
                // Resolve now: a sibling cell's pin fold may need this before linkDotParamed
                resolve_Generic_Iface_Dots_اندر(nodep->valuep());
                // If the value expression contains a VarXRef to an interface
                // localparam whose value is not yet constant, defer constification
                // to avoid premature widthing with unresolved values (see
                // t_lparam_dep_iface tests).  When the referenced localparam is
                // already const, proceed normally so FUNCREFs with resolved iface
                // param args get folded.
                منطقی hasUnresolvedLparamXRef = جھوٹ;
                nodep->valuep()->foreach([&](ثابت AstVarXRef* xrefp) {
                    اگر (ثابت AstVar* ثابت varp = xrefp->varp()) {
                        اگر (varp->varType() == VVarType::LPARAM && !VN_IS(varp->valuep(), Const)) {
                            hasUnresolvedLparamXRef = سچ;
                        }
                    }
                });
                اگر (hasUnresolvedLparamXRef) واپس;
                // Defer if value has any unresolved Dot, or references a deferred lparam
                ثابت منطقی hasDot = nodep->valuep()->exists([](AstDot*) { واپس سچ; });
                منطقی refsDeferred = جھوٹ;
                اگر (!hasDot) {
                    ثابت خودکار& deferredVarps = v3Global.rootp()->deferredParamVarps();
                    nodep->valuep()->foreach([&](ثابت AstVarRef* refp) {
                        اگر (refsDeferred) واپس;
                        AstVar* ثابت refVarp = refp->varp();
                        اگر (refVarp && refVarp->varType() == VVarType::LPARAM
                            && deferredVarps.گنتی(refVarp)) {
                            refsDeferred = سچ;
                        }
                    });
                }
                اگر (hasDot || refsDeferred) {
                    v3Global.rootp()->ڈالو_Deferred_Param_Varp(nodep);
                    واپس;
                }
                m_processor.resolveDeferredDotsReachableFrom(nodep, m_modp);
                V3Const::constifyParamsEdit(nodep);
            }
        }
    }
    خالی دورہ(AstParamTypeDType* nodep) override {
        iterateChildren(nodep);
        اگر (VN_IS(nodep->skipRefOrNullp(), VoidDType)) {
            nodep->v3error("Parameter type without default value is never given value"
                           << " (IEEE 1800-2023 6.20.1): " << nodep->prettyNameQ());
        }
    }
    // Make sure varrefs cause vars to constify before things above
    خالی دورہ(AstVarRef* nodep) override {
        // Might jump across functions, so beware if ever add a m_funcp
        اگر (nodep->varp()) iterate(nodep->varp());
    }
    منطقی ifaceParamReplace(AstVarXRef* nodep, Ast_گرہ* candp) {
        برائے (; candp; candp = candp->nextp()) {
            اگر (nodep->name() == candp->name()) {
                اگر (AstVar* ثابت varp = VN_CAST(candp, Var)) {
                    UINFO(9, "Found interface parameter: " << varp);
                    // The interface may not have been visited yet (it is at a
                    // deeper level in the work queue), so its localparams may
                    // not be constified.  Eagerly constify here so that the
                    // caller's hasUnresolvedLparamXRef check sees a Const.
                    اگر (varp->isParam() && varp->valuep() && !VN_IS(varp->valuep(), Const)) {
                        V3Const::constifyParamsEdit(varp);
                    }
                    nodep->varp(varp);
                    واپس سچ;
                } ورنہ اگر (ثابت AstPin* ثابت pinp = VN_CAST(candp, Pin)) {
                    UINFO(9, "Found interface parameter: " << pinp);
                    UASSERT_OBJ(pinp->exprp(), pinp, "Interface parameter pin missing expression");
                    nodep->replaceWith(pinp->exprp()->clone_درخت(جھوٹ));
                    VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
                    واپس سچ;
                }
            }
        }
        واپس جھوٹ;
    }

    خالی deparamIfaceCellNow(AstCell* cellp) {
        اگر (!cellp->paramsp()) واپس;
        اگر (!VN_IS(cellp->modp(), Iface)) واپس;
        Ast_گرہ_Module* ثابت srcModp = cellp->modp();
        Ast_گرہ_Module* ثابت newModp
            = m_processor.گرہ_Deparam(cellp, srcModp, m_modp, m_modp->someInstanceName());
        اگر (newModp && newModp != srcModp) {
            اگر (V3LinkDotIfaceCapture::enabled()) {
                m_processor.retargetIfaceRefs(m_modp, cellp->name());
            }
            specializeNestedIfaceCells(newModp);
        }
    }

    خالی دورہ(Ast_گرہ_FTask_Ref* nodep) override {
        اگر (nodep->شامل_ہے_Gen_بلاک()) {
            // Needs relink, as may remove pointed-to task/func
            nodep->taskp(خالی_اشاریہ);
            واپس;
        }
        iterateChildren(nodep);
    }
    خالی دورہ(Ast_Enum_شے_Ref* nodep) override {
        // Needs relink, as may remove pointed-to item
        اگر (nodep->شامل_ہے_Gen_بلاک()) nodep->itemp(خالی_اشاریہ);
        iterateChildren(nodep);
    }
    خالی دورہ(AstVarXRef* nodep) override {
        اگر (nodep->شامل_ہے_Gen_بلاک()) {
            // Needs relink, as may remove pointed-to var
            nodep->varp(خالی_اشاریہ);
            واپس;
        }
        // Parameters of an interface array are accessed through its first element
        اگر (nodep->varp() && nodep->varp()->isParam()) {
            ثابت خودکار it = m_ifaceInstCells.ڈھونڈو(nodep->dotted());
            اگر (it != m_ifaceInstCells.انتہا()) nodep->dotted(it->دوسرا->name());
        }
        // Check to see if the scope is just an interface because interfaces are special
        ثابت string dotted = nodep->dotted();
        اگر (!dotted.خالی_خانے() && nodep->varp() && nodep->varp()->isParam()) {
            ثابت Ast_گرہ* backp = nodep;
            جب ((backp = backp->backp())) {
                اگر (VN_IS(backp, گرہ_Module)) {
                    UINFO(9, "Hit module boundary, done looking for interface");
                    توڑو;
                }
                اگر (ثابت AstVar* ثابت varp = VN_CAST(backp, Var)) {
                    اگر (!varp->isIfaceRef()) جاری;
                    // Through all dimensions of an array
                    ثابت AstIfaceRefDType* ثابت ifacerefp
                        = VN_CAST(varp->subDTypep()->elemDTypep(), IfaceRefDType);
                    اگر (!ifacerefp) جاری;
                    // Interfaces passed in on the port map have ifaces
                    اگر (ثابت AstIface* ثابت ifacep = ifacerefp->ifacep()) {
                        اگر (dotted == backp->name()) {
                            UINFO(9, "Iface matching scope:  " << ifacep);
                            اگر (ifaceParamReplace(nodep, ifacep->stmtsp())) {  //
                                واپس;
                            }
                        }
                    }
                    // Interfaces declared in this module have cells
                    ورنہ اگر (AstCell* ثابت cellp = ifacerefp->cellp()) {
                        اگر (dotted == cellp->name()) {
                            UINFO(9, "Iface matching scope:  " << cellp);
                            deparamIfaceCellNow(cellp);
                            اگر (ifaceParamReplace(nodep, cellp->modp()->stmtsp())) {  //
                                واپس;
                            }
                        }
                    }
                }
            }
            // Fallback: a direct local interface instance ("inst.PARAM"), not reached via a port.
            ثابت خودکار ifaceCellIt = m_ifaceInstCells.ڈھونڈو(dotted);
            اگر (ifaceCellIt != m_ifaceInstCells.انتہا()) {
                AstCell* ثابت cellp = ifaceCellIt->دوسرا;
                deparamIfaceCellNow(cellp);
                ifaceParamReplace(nodep, cellp->modp()->stmtsp());
            }
        }
    }

    خالی دورہ(AstDot* nodep) override {
        iterate(nodep->lhsp());
        // Check if it is a reference to a field of a parameterized class.
        // If so, the RHS should be updated, when the LHS is replaced
        // by a class with actual parameter values.
        ثابت AstClass* lhsClassp = خالی_اشاریہ;
        ثابت AstClassOrPackageRef* ثابت classRefp = VN_CAST(nodep->lhsp(), ClassOrPackageRef);
        اگر (classRefp) lhsClassp = VN_CAST(classRefp->classOrPackageSkipp(), Class);
        Ast_گرہ* rhsDefp = خالی_اشاریہ;
        AstClassOrPackageRef* ثابت rhsp = VN_CAST(nodep->rhsp(), ClassOrPackageRef);
        اگر (rhsp) rhsDefp = rhsp->classOrPackageNodep();
        اگر (lhsClassp && rhsDefp) {
            m_state.m_dots.ڈالو_عقب(nodep);
            // No need to iterate into rhsp, because there should be nothing to do
        } ورنہ {
            iterate(nodep->rhsp());
        }
    }

    خالی دورہ(AstUnlinkedRef* nodep) override {
        AstVarXRef* ثابت varxrefp = VN_CAST(nodep->refp(), VarXRef);
        Ast_گرہ_FTask_Ref* ثابت taskrefp = VN_CAST(nodep->refp(), گرہ_FTask_Ref);
        Ast_Enum_شے_Ref* ثابت enumrefp = VN_CAST(nodep->refp(), Enum_شے_Ref);
        اگر (varxrefp) {
            m_unlinkedTxt = varxrefp->dotted();
        } ورنہ اگر (taskrefp) {
            m_unlinkedTxt = taskrefp->dotted();
        } ورنہ اگر (enumrefp) {
            m_unlinkedTxt = enumrefp->dotted();
        } ورنہ {
            nodep->v3fatalSrc("Unexpected AstUnlinkedRef node");
            واپس;
        }
        iterate(nodep->cellrefp());

        اگر (varxrefp) {
            varxrefp->dotted(m_unlinkedTxt);
        } ورنہ اگر (taskrefp) {
            taskrefp->dotted(m_unlinkedTxt);
        } ورنہ {
            enumrefp->dotted(m_unlinkedTxt);
        }
        nodep->replaceWith(nodep->refp()->unlink_Fr_عقب());
        VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
    }
    خالی دورہ(AstCellArrayRef* nodep) override {
        // Multi-dim iface arrays chain multiple selps via nextp(); constify each in place.
        برائے (Ast_گرہ *s = nodep->selp(), *nxt; s; s = nxt) {
            nxt = s->nextp();  // cache before constifyParamsEdit replaces s
            V3Const::constifyParamsEdit(s);
        }
        std::سمتیہ<صحیح> indices;
        برائے (Ast_گرہ* s = nodep->selp(); s; s = s->nextp()) {
            ثابت AstConst* ثابت constp = VN_CAST(s, Const);
            اگر (!constp) {
                nodep->v3error("Could not expand constant selection inside dotted reference: "
                               << s->prettyNameQ());
                واپس;
            }
            indices.ڈالو_عقب(constp->مقصد_SInt());
        }
        // Nested iface-array ports may carry a __Viftop suffix that's not in m_unlinkedTxt.
        ثابت string viftopSuffix = "__Viftop";
        ثابت string بنیاد_Name
            = VString::سرے_With(nodep->name(), viftopSuffix)
                  ? nodep->name().substr(0, nodep->name().سائز() - viftopSuffix.سائز())
                  : nodep->name();
        ثابت string placeholder = "__BRA__??__KET__";
        سائز_t pos = m_unlinkedTxt.ڈھونڈو(بنیاد_Name + placeholder);
        // An AstCellArrayRef for an iface-array pin expr (e.g. l1(l2.l1[0])) is visited
        // outside AstUnlinkedRef, so m_unlinkedTxt has no placeholder; V3LinkDot resolves later.
        اگر (pos == string::npos) {
            UINFO(9, "Skipping unlinked text replacement for " << nodep);
            واپس;
        }
        pos += بنیاد_Name.لمبائی_2();
        برائے (صحیح idx : indices) {
            ثابت string replacement = "__BRA__" + Ast_گرہ::encode_نمبر(idx) + "__KET__";
            اگر (m_unlinkedTxt.compare(pos, placeholder.لمبائی_2(), placeholder) != 0) {
                nodep->v3fatalSrc(  // LCOV_EXCL_LINE
                    "Expected placeholder at position in multi-dim iface dotted reference");
                واپس;
            }
            m_unlinkedTxt.replace(pos, placeholder.لمبائی_2(), replacement);
            pos += replacement.لمبائی_2();
        }
    }

    // Generate Statements

    // Traverse a generate condition (if/case expression, for init/cond/inc).
    //
    // m_cellps assumes a queued ref outlives the drain loop, which holds elsewhere
    // because deletion is deferred.  A generate condition doesn't defer and folds
    // immediately, so we can't queue. Anything queued here would be left dangling.
    // Refs in the surviving arm are queued when it is recursed.
    خالی iterateGenerateCond(Ast_گرہ* nodep) {
        VL_RESTORER(m_اندر_Generate_Cond);
        m_اندر_Generate_Cond = سچ;
        iterate_And_اگلا_Null(nodep);
        // The condition may reference deferred lparams whose Dots are still pending.
        m_processor.resolveDeferredDotsReachableFrom(nodep, m_modp);
    }

    خالی دورہ(AstGenIf* nodep) override {
        UINFO(9, "  GENIF " << nodep);
        iterateGenerateCond(nodep->condp());
        // We suppress errors when widthing params since short-circuiting in
        // the conditional evaluation may mean these error can never occur. We
        // then make sure that short-circuiting is used by constifyParamsEdit.
        V3_چوڑائی::چوڑائی_Generate_Params_Edit(nodep);  // Param typed widthing will
                                                  // NOT recurse the body.
        V3Const::constifyGenerateParamsEdit(nodep->condp());  // condp may change
        اگر (ثابت AstConst* ثابت constp = VN_CAST(nodep->condp(), Const)) {
            اگر (Ast_گرہ* ثابت keepp = (constp->isZero() ? nodep->elsesp() : nodep->thensp())) {
                keepp->unlink_Fr_عقب_With_اگلا();
                nodep->replaceWith(keepp);
            } ورنہ {
                nodep->unlink_Fr_عقب();
            }
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
            // Normal edit rules will now recurse the replacement
        } ورنہ {
            nodep->condp()->v3error("Generate If condition must evaluate to constant");
        }
    }

    خالی دورہ(Ast_Gen_بلاک* nodep) override {
        // Parameter substitution for generated for loops.
        // TODO Unlike generated IF, we don't have to worry about short-circuiting the
        // conditional expression, since this is currently restricted to simple
        // comparisons. If we ever do move to more generic constant expressions, such code
        // will be needed here.
        اگر (AstGenFor* ثابت forp = VN_AS(nodep->genforp(), GenFor)) {
            // We should have a GENFOR under here.  We will be replacing the begin,
            // so process here rather than at the generate to avoid iteration problems
            UINFO(9, "  BEGIN " << nodep);
            UINFO(9, "  GENFOR " << forp);
            // Visit child nodes before unrolling
            iterateGenerateCond(forp->initsp());
            iterateGenerateCond(forp->condp());
            iterateGenerateCond(forp->incsp());
            V3_چوڑائی::چوڑائی_Params_Edit(forp);  // Param typed widthing will NOT recurse the body
            // Outer wrapper around generate used to hold genvar, and to ensure genvar
            // doesn't conflict in V3LinkDot resolution with other genvars
            // Now though we need to change BEGIN("zzz", GENFOR(...)) to
            // a BEGIN("zzz__BRA__{loop#}__KET__")
            ثابت string beginName = nodep->name();
            // Leave the original Begin, as need a container for the (possible) GENVAR
            // Note m_unroller will replace some AstVarRef's to the loop variable with constants
            // Don't remove any deleted nodes in m_unroller until whole process finishes,
            // as some AstXRefs may still point to old nodes.
            VL_DO_DANGLING(m_unroller.unroll(forp, beginName), forp);
            // Blocks were constructed under the special begin, move them up
            // Note forp is null, so grab statements again
            اگر (Ast_گرہ* ثابت stmtsp = nodep->genforp()) {
                stmtsp->unlink_Fr_عقب_With_اگلا();
                nodep->جوڑو_اگلا_یہاں(stmtsp);
                // Note this clears nodep->genforp(), so begin is no longer special
            }
        } ورنہ {
            VL_RESTORER_COPY(m_generateHierName);
            m_generateHierName += "." + nodep->prettyName();
            iterateChildren(nodep);
        }
    }
    خالی دورہ(AstGenFor* nodep) override {  // LCOV_EXCL_LINE
        nodep->v3fatalSrc("GENFOR should have been wrapped in BEGIN");
    }
    خالی دورہ(AstGenCase* nodep) override {
        UINFO(9, "  GENCASE " << nodep);
        منطقی کامیابی = جھوٹ;
        Ast_گرہ* keepp = خالی_اشاریہ;
        iterateGenerateCond(nodep->exprp());
        V3Case::caseLint(nodep);
        // Case-item expressions must also be linked before widthing the case.
        برائے (Ast_Gen_Case_شے* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Gen_Case_شے)) {
            برائے (Ast_گرہ* ep = itemp->condsp(); ep; ep = ep->nextp()) {
                m_processor.resolveDeferredDotsReachableFrom(ep, m_modp);
            }
        }
        V3_چوڑائی::چوڑائی_Params_Edit(nodep);  // Param typed widthing will NOT recurse the
                                          // body, don't trigger errors yet.
        V3Const::constifyParamsEdit(nodep->exprp());  // exprp may change
        ثابت AstConst* ثابت exprp = VN_AS(nodep->exprp(), Const);
        // Constify
        برائے (Ast_Gen_Case_شے* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Gen_Case_شے)) {
            برائے (Ast_گرہ* ep = itemp->condsp(); ep;) {
                Ast_گرہ* ثابت nextp = ep->nextp();  // May edit list
                iterateGenerateCond(ep);
                VL_DO_DANGLING(V3Const::constifyParamsEdit(ep), ep);  // ep may change
                ep = nextp;
            }
        }
        // Item match
        برائے (Ast_Gen_Case_شے* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Gen_Case_شے)) {
            اگر (!itemp->isDefault()) {
                برائے (Ast_گرہ* ep = itemp->condsp(); ep; ep = ep->nextp()) {
                    اگر (ثابت AstConst* ثابت ccondp = VN_CAST(ep, Const)) {
                        V3_نمبر مطابقت{nodep, 1};
                        مطابقت.opEq(ccondp->عدد(), exprp->عدد());
                        اگر (!کامیابی && مطابقت.isNeqZero()) {
                            کامیابی = سچ;
                            keepp = itemp->itemsp();
                        }
                    } ورنہ {
                        itemp->v3error("Generate Case item does not evaluate to constant");
                    }
                }
            }
        }
        // Else default match
        برائے (Ast_Gen_Case_شے* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Gen_Case_شے)) {
            اگر (itemp->isDefault()) {
                اگر (!کامیابی) {
                    کامیابی = سچ;
                    keepp = itemp->itemsp();
                }
            }
        }
        // Replace
        اگر (keepp) {
            keepp->unlink_Fr_عقب_With_اگلا();
            nodep->replaceWith(keepp);
        } ورنہ {
            nodep->unlink_Fr_عقب();
        }
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
    }

    خالی دورہ(Ast_گرہ* nodep) override { iterateChildren(nodep); }

عوامی:
    // CONSTRUCTORS
    صریح ParamVisitor(ParamState& state, AstNetlist* netlistp)
        : m_state{state}
        , m_processor{netlistp} {

        // Relies on modules already being in top-down-order
        iterate(netlistp);
    }
    ~ParamVisitor() override = طےشدہ;
    VL_UNCOPYABLE(ParamVisitor);
};

//######################################################################
// Process top handler

جماعت Param_چوٹی final : VNDeleter {
    // NODE STATE
    // AstNodeModule::user1 -> bool: already fixed level (temporary)

    // STATE
    ParamState m_state;  // Common state

    // METHODS

    خالی relinkDots() {
        // RHSs of AstDots need a relink when LHS is a parameterized class reference
        برائے (AstDot* ثابت dotp : m_state.m_dots) {
            ثابت AstClassOrPackageRef* ثابت classRefp = VN_AS(dotp->lhsp(), ClassOrPackageRef);
            ثابت AstClass* ثابت lhsClassp = VN_AS(classRefp->classOrPackageSkipp(), Class);
            AstClassOrPackageRef* ثابت rhsp = VN_AS(dotp->rhsp(), ClassOrPackageRef);
            برائے (خودکار* itemp = lhsClassp->membersp(); itemp; itemp = itemp->nextp()) {
                اگر (itemp->name() == rhsp->name()) {
                    rhsp->classOrPackageNodep(itemp);
                    توڑو;
                }
            }
        }
    }

    خالی fixLevel(Ast_گرہ_Module* modp) {
        // Fix up level of module, based on who instantiates it
        اگر (modp->user1_سیٹ_Once()) واپس;  // Already fixed
        اگر (m_state.m_parentps[modp].خالی_خانے()) واپس;  // Leave top levels alone
        صحیح max_والد_Level = 0;
        برائے (Ast_گرہ_Module* parentp : m_state.m_parentps[modp]) {
            fixLevel(parentp);  // Ensure parent level is correct
            max_والد_Level = std::max(max_والد_Level, parentp->level());
        }
        اگر (modp->level() <= max_والد_Level) modp->level(max_والد_Level + 1);
        // Not correct fixup of depth(), as it should be mininum.  But depth() is unused
        // past V3LinkParse, so just do something sane in case this code doesn't get updated
        modp->گہرائی(modp->level());
    }

    خالی resortNetlistModules(AstNetlist* netlistp) {
        // Re-sort module list to be in topological order and fix-up incorrect levels. We need to
        // do this globally at the end due to the presence of recursive modules, which might be
        // expanded in orders that reuse earlier specializations later at a lower level.
        // Gather modules
        std::سمتیہ<Ast_گرہ_Module*> modps;
        برائے (Ast_گرہ_Module *modp = netlistp->modulesp(), *nextp; modp; modp = nextp) {
            nextp = VN_AS(modp->nextp(), گرہ_Module);
            modp->unlink_Fr_عقب();
            modps.ڈالو_عقب(modp);
        }

        // Fix-up levels
        ثابت VNUser1_اندر_Use user1_اندر_Use;
        برائے (Ast_گرہ_Module* ثابت modp : modps) fixLevel(modp);

        // Sort by level
        std::stable_ترتیب_دو(modps.begin(), modps.انتہا(),
                         [](ثابت Ast_گرہ_Module* ap, ثابت Ast_گرہ_Module* bp) {
                             واپس ap->level() < bp->level();
                         });

        // Re-insert modules
        برائے (Ast_گرہ_Module* ثابت modp : modps) netlistp->جوڑو_Modulesp(modp);
    }

عوامی:
    // CONSTRUCTORS
    صریح Param_چوٹی(AstNetlist* netlistp) {
        // Relies on modules already being in top-down-order
        ثابت ParamVisitor paramVisitor{m_state /*ref*/, netlistp};

        // Mark classes which cannot be removed because they are still referenced
        ثابت ClassRefUnlinkerVisitor classUnlinkerVisitor{netlistp};

        netlistp->foreach([](Ast_گرہ_FTask_Ref* ftaskrefp) {
            Ast_گرہ_FTask* ftaskp = ftaskrefp->taskp();
            اگر (!ftaskp || !ftaskp->class_طریقہ()) واپس;
            ثابت std::string funcName = ftaskp->name();
            // Find the nearest containing (ancestor) class for a node.
            // Uses aboveLoopp() which correctly skips sibling links
            // (e.g. covergroup classes) to find the true parent.
            ثابت خودکار ancestor_Class_کا = [](Ast_گرہ* startp) -> AstClass* {
                برائے (Ast_گرہ* np = startp->aboveLoopp(); np; np = np->aboveLoopp()) {
                    اگر (AstClass* ثابت cp = VN_CAST(np, Class)) واپس cp;
                }
                واپس خالی_اشاریہ;
            };
            AstClass* refClassp = ancestor_Class_کا(ftaskrefp);
            اگر (refClassp == ftaskrefp->classOrPackagep())
                واپس;  // task is in the same class as reference
            AstClass* classp = ancestor_Class_کا(ftaskp);
            UASSERT_OBJ(classp, ftaskrefp, "Class method has no class above it");
            // If the FUNCREF and its task are both in the same (clone) class but
            // classOrPackagep still points to the old template, just retarget it
            اگر (refClassp && refClassp == classp && ftaskrefp->classOrPackagep()
                && ftaskrefp->classOrPackagep() != refClassp) {
                ftaskrefp->classOrPackagep(refClassp);
                واپس;
            }
            اگر (classp->user3p()) واپس;  // will not get removed, no need to relink
            AstClass* ثابت parametrizedClassp = VN_CAST(classp->user4p(), Class);
            اگر (!parametrizedClassp) واپس;
            Ast_گرہ_FTask* newFuncp = خالی_اشاریہ;
            parametrizedClassp->exists([&newFuncp, funcName](Ast_گرہ_FTask* ftaskp) {
                اگر (ftaskp->name() == funcName) {
                    newFuncp = ftaskp;
                    واپس سچ;
                }
                واپس جھوٹ;
            });
            اگر (newFuncp) {
                // v3error(ftaskp <<"->" << newFuncp);
                ftaskrefp->taskp(newFuncp);
                ftaskrefp->classOrPackagep(parametrizedClassp);
            }
        });

        ثابت ParamClassRefDTypeRelinkVisitor paramClassDTypeRelinkVisitor{netlistp};

        relinkDots();

        resortNetlistModules(netlistp);

        // For occasional debug only; this will upset VL_LEAK_CHECKS
        // V3Global::dumpCheckGlobalTree("param-predel", 0, dumpTreeEitherLevel() >= 9);

        // Remove defaulted classes
        // Unlike modules, which we keep around and mark dead() for later V3Dead
        std::unordered_سیٹ<AstClass*> removedClassps;
        برائے (AstClass* ثابت classp : m_state.m_paramClasses) {
            اگر (!classp->user3p()) {
                // If there was an error, don't remove classes as they might
                // have remained referenced, and  will crash in V3Broken or
                // other locations. This is fine, we will abort imminently.
                اگر (V3Error::error_گنتی()) جاری;
                removedClassps.emplace(classp);
                VL_DO_DANGLING(ڈالو_Deletep(classp->unlink_Fr_عقب()), classp);
            } ورنہ {
                // Referenced. classp became a specialized class with the default
                // values of parameters and is not a parameterized class anymore
                classp->hasGParam(جھوٹ);
            }
        }
        // Remove references to defaulted classes
        // Reuse user3 to mark all nodes being deleted
        Ast_گرہ::user3_Clear_درخت();
        برائے (AstClass* classp : removedClassps) {
            classp->foreach([](Ast_گرہ* ثابت nodep) { nodep->user3(سچ); });
        }
        // Set all links pointing to a user3 (deleting) node as null
        netlistp->foreach([](Ast_گرہ* ثابت nodep) {
            nodep->foreachLink([&](Ast_گرہ** ثابت linkpp, ثابت حرف* namep) {
                اگر (*linkpp && (*linkpp)->user3()) {
                    UINFO(9, "clear   link " << namep << " on " << nodep);
                    *linkpp = خالی_اشاریہ;
                    UINFO(9, "cleared link " << namep << " on " << nodep);
                }
            });
        });
    }
    ~Param_چوٹی() = طےشدہ;
    VL_UNCOPYABLE(Param_چوٹی);
};

//######################################################################
// Param class functions

خالی V3Param::param(AstNetlist* rootp) {
    UINFO(2, __FUNCTION__ << ":");
    rootp->clearDeferredParamVarps();  // Defensive: drop any stragglers from a prior invocation

    اگر (dump_درخت_Either_Level() >= 9) V3LinkDotIfaceCapture::dumpEntries("before V3Param");
    { Param_چوٹی{rootp}; }
    // The memo is only good while parameterizing, and the tree moves after.
    rootp->clearContainingModules();
    اگر (dump_درخت_Either_Level() >= 9) V3LinkDotIfaceCapture::dumpEntries("after V3Param");

    V3Global::dump_Check_Global_درخت("param", 0, dump_درخت_Either_Level() >= 3);
}

خالی V3Param::finalizeDeferredParams(AstNetlist* rootp) {
    // Constify params whose Dot RHS was resolved by V3LinkDot::linkDotParamed.
    برائے (AstVar* ثابت varp : rootp->deferredParamVarps()) {
        اگر (varp->valuep() && !VN_IS(varp->valuep(), Const)) V3Const::constifyParamsEdit(varp);
    }
    rootp->clearDeferredParamVarps();
}
