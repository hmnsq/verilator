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

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3Param.h"

#ضمن "V3Case.h"
#ضمن "V3Const.h"
#ضمن "V3EmitV.h"
#ضمن "V3HierBlock.h"
#ضمن "V3LinkDotIfaceCapture.h"
#ضمن "V3MemberMap.h"
#ضمن "V3Os.h"
#ضمن "V3Parse.h"
#ضمن "V3Simulate.h"
#ضمن "V3Stats.h"
#ضمن "V3Unroll.h"
#ضمن "V3Width.h"

#ضمن <cctype>
#ضمن <cstdlib>
#ضمن <deque>
#ضمن <map>
#ضمن <memory>
#ضمن <set>
#ضمن <unordered_map>
#ضمن <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

// Walk a class-typed node through typedef/RefDType chains to its ClassRefDType, or null.
ساكن AstClassRefDType* class_Ref_DType_جو_ڳنڍ(Ast_ڳنڍ* nodep) {
    طالما (AstTypedef* ثابت tdp = VN_CAST(nodep, Typedef)) nodep = tdp->subDTypep();
    Ast_ڳنڍ_DType* ثابت dtp = VN_CAST(nodep, ڳنڍ_DType);
    أرجع dtp ? VN_CAST(dtp->skipRefOrNullp(), ClassRefDType) : خالي_اشارو;
}

//######################################################################
// Hierarchical block and parameter db (modules without parameters are also handled)

صنف Parameterized_Hier_بلاڪون final {
    استعمال Hier_بلاڪ_Opts_By_Orig_Name = std::multimap<std::string, ثابت V3_درجيوار_بلاڪ_Option*>;
    استعمال HierMapIt = Hier_بلاڪ_Opts_By_Orig_Name::const_iterator;
    استعمال Hier_بلاڪ_Mod_Map = std::map<ثابت std::string, Ast_ڳنڍ_Module*>;
    استعمال ParamConstMap = std::map<ثابت std::string, std::unique_ptr<AstConst>>;
    استعمال GParamsMap = std::map<ثابت std::string, AstVar*>;  // key:parameter name value:parameter

    // MEMBERS
    ثابت منطقي m_hier_Sub_لاڳيتو;  // Is in sub-run for hierarchical verilation
    // key:Original module name, value:HiearchyBlockOption*
    // If a module is parameterized, the module is uniquified to overridden parameters.
    // This is why HierBlockOptsByOrigName is multimap.
    Hier_بلاڪ_Opts_By_Orig_Name m_hier_بلاڪ_Opts_By_Orig_Name;
    // key:mangled module name, value:AstNodeModule*
    Hier_بلاڪ_Mod_Map m_hier_بلاڪ_Mod;
    // Overridden parameters of the hierarchical block
    std::map<ثابت V3_درجيوار_بلاڪ_Option*, ParamConstMap> m_hierParams;
    // Parameter variables of hierarchical blocks
    std::map<ثابت std::string, GParamsMap> m_modParams;
    // Formals declared without a type, whose type is that of the assigned value
    std::سيٽ<ثابت AstVar*> m_untypedParams;

    // METHODS

عام:
    Parameterized_Hier_بلاڪون(ثابت V3_Hier_بلاڪ_Opt_سيٽ& hierOpts, AstNetlist* nodep)
        // A type parameter file only describes a child's own types; the final consolidation
        // run has none, but must still substitute the libraries it consumes.
        : m_hier_Sub_لاڳيتو{!v3Global.opt.hier_بلاڪون().خالي_جايون() || v3Global.opt.hier_ٻار()} {
        لكل (ثابت خودڪار& hierOpt : hierOpts) {
            m_hier_بلاڪ_Opts_By_Orig_Name.emplace(hierOpt.ٻيو.origName(), &hierOpt.ٻيو);
            ثابت V3_درجيوار_بلاڪ_Option::ParamStrMap& params = hierOpt.ٻيو.params();
            ParamConstMap& consts = m_hierParams[&hierOpt.ٻيو];
            لكل (V3_درجيوار_بلاڪ_Option::ParamStrMap::const_iterator pIt = params.begin();
                 pIt != params.پڇاڙي(); ++pIt) {
                std::unique_ptr<AstConst> constp{AstConst::parseParamLiteral(
                    جديد File_سٽ{File_سٽ::built_اندريون_Filename()}, pIt->ٻيو)};
                UASSERT(constp, pIt->ٻيو << " is not a valid parameter literal");
                ثابت منطقي inserted = consts.emplace(pIt->پهريون, std::move(constp)).ٻيو;
                UASSERT(inserted, pIt->پهريون << " is already added");
            }
            // origName may be already registered, but it's fine.
            m_modParams.وجهو({hierOpt.ٻيو.origName(), {}});
        }
        لكل (Ast_ڳنڍ_Module* modp = nodep->modulesp(); modp;
             modp = VN_AS(modp->nextp(), ڳنڍ_Module)) {
            إذا (hierOpts.ڳوليو(modp->prettyName()) != hierOpts.پڇاڙي()) {
                m_hier_بلاڪ_Mod.emplace(modp->name(), modp);
            }
            // Recursive hierarchical module may change its name, so we have to match its origName.
            // Collect values from recursive cloned module as parameters in the top module could be
            // overridden.
            ثابت string actualModName = modp->recursiveClone() ? modp->origName() : modp->name();
            ثابت خودڪار defParamIt = m_modParams.ڳوليو(actualModName);
            إذا (defParamIt != m_modParams.پڇاڙي()) {
                // modp is the original of parameterized hierarchical block
                لكل (Ast_ڳنڍ* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    إذا (AstVar* ثابت varp = VN_CAST(stmtp, Var)) {
                        إذا (varp->isGParam()) defParamIt->ٻيو.emplace(varp->name(), varp);
                        // Recorded before V3Param resolves the type from the assigned value.
                        ثابت AstBasicDType* ثابت dtypep
                            = VN_CAST(varp->ٻار_DTypep(), BasicDType);
                        إذا (varp->isGParam() && dtypep && dtypep->implicit()) {
                            m_untypedParams.emplace(varp);
                        }
                    }
                }
            }
        }
    }
    منطقي hier_Sub_لاڳيتو() ثابت { أرجع m_hier_Sub_لاڳيتو; }
    منطقي is_Hier_بلاڪ(ثابت string& origName) ثابت {
        أرجع m_hier_بلاڪ_Opts_By_Orig_Name.ڳوليو(origName) != m_hier_بلاڪ_Opts_By_Orig_Name.پڇاڙي();
    }
    Ast_ڳنڍ_Module* ڳوليو_By_Params(ثابت string& origName, AstPin* پهريون_Pinp,
                                ثابت Ast_ڳنڍ_Module* modp) {
        UASSERT(is_Hier_بلاڪ(origName), origName << " is not hierarchical block");
        // This module is a hierarchical block. Need to replace it by the --lib-create wrapper.
        ثابت std::pair<HierMapIt, HierMapIt> candidates
            = m_hier_بلاڪ_Opts_By_Orig_Name.equal_range(origName);
        ثابت خودڪار paramsIt = m_modParams.ڳوليو(origName);
        UASSERT_OBJ(paramsIt != m_modParams.پڇاڙي(), modp, origName << " must be registered");
        // An untyped parameter takes the type of its value, but libraries are distinguished
        // only by parameter values. This is a pin whose value equals that of a library or the
        // default, but whose type differs.
        ثابت AstPin* typeMismatchPinp = خالي_اشارو;
        HierMapIt hierIt;
        لكل (hierIt = candidates.پهريون; hierIt != candidates.ٻيو; ++hierIt) {
            منطقي مليو = سچ;
            سائيز_t paramIdx = 0;
            ثابت ParamConstMap& params = m_hierParams[hierIt->ٻيو];
            UASSERT(params.سائيز() == hierIt->ٻيو->params().سائيز(), "not match");
            لكل (AstPin* pinp = پهريون_Pinp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                إذا (!pinp->exprp()) استمر;
                إذا (ثابت AstVar* ثابت modvarp = pinp->modVarp()) {
                    AstConst* ثابت constp = VN_AS(pinp->exprp(), Const);
                    UASSERT_OBJ(constp, pinp,
                                "parameter for a hierarchical block must have been constified");
                    ثابت خودڪار paramIt = paramsIt->ٻيو.ڳوليو(modvarp->name());
                    UASSERT_OBJ(paramIt != paramsIt->ٻيو.پڇاڙي(), modvarp, "must be registered");
                    ثابت منطقي untyped = m_untypedParams.ڳڻپ(paramIt->ٻيو);
                    AstConst* ثابت defValuep = VN_CAST(paramIt->ٻيو->valuep(), Const);
                    إذا (defValuep && areSame(constp, defValuep)) {
                        إذا (!untyped || constp->same_قدر_Type(defValuep)) {
                            UINFO(5, "Setting default value of " << constp << " to " << modvarp);
                            استمر;  // Skip this parameter because setting the same value
                        }
                        typeMismatchPinp = pinp;
                    }
                    ثابت خودڪار pIt = vlstd::as_const(params).ڳوليو(modvarp->name());
                    UINFO(5, "Comparing " << modvarp->name() << " " << constp);
                    إذا (pIt == params.پڇاڙي() || paramIdx >= params.سائيز()
                        || !areSame(constp, pIt->ٻيو.get())) {
                        مليو = خطأ;
                        اكسر;
                    }
                    إذا (untyped && !constp->same_قدر_Type(pIt->ٻيو.get())) {
                        typeMismatchPinp = pinp;
                        مليو = خطأ;
                        اكسر;
                    }
                    UINFO(5, "Matched " << modvarp->name() << " " << constp << " and "
                                        << pIt->ٻيو.get());
                    ++paramIdx;
                }
            }
            إذا (مليو && paramIdx == hierIt->ٻيو->params().سائيز()) اكسر;
        }
        إذا (hierIt == candidates.ٻيو && typeMismatchPinp) {
            typeMismatchPinp->v3warn(
                E_UNSUPPORTED, "Unsupported: Untyped parameter "
                                   << typeMismatchPinp->modVarp()->prettyNameQ()
                                   << " of hierarchical block " << Ast_ڳنڍ::prettyNameQ(origName)
                                   << " given equal values of different width or"
                                      " signedness");
            أرجع خالي_اشارو;
        }
        UASSERT_OBJ(hierIt != candidates.ٻيو, پهريون_Pinp, "No --lib-create wrapper found");
        // parameter settings will be removed in the bottom of caller visitCell().
        ثابت Hier_بلاڪ_Mod_Map::const_iterator modIt
            = m_hier_بلاڪ_Mod.ڳوليو(hierIt->ٻيو->mangledName());
        UASSERT_OBJ(modIt != m_hier_بلاڪ_Mod.پڇاڙي(), پهريون_Pinp,
                    hierIt->ٻيو->mangledName() << " is not found");

        ثابت خودڪار it = vlstd::as_const(m_hier_بلاڪ_Mod).ڳوليو(hierIt->ٻيو->mangledName());
        إذا (it == m_hier_بلاڪ_Mod.پڇاڙي()) أرجع خالي_اشارو;
        أرجع it->ٻيو;
    }
    ساكن منطقي areSame(AstConst* pinValuep, AstConst* hierOptParamp) {
        إذا (pinValuep->isString()) {
            أرجع pinValuep->انگ().منزل_String() == hierOptParamp->انگ().منزل_String();
        }

        إذا (hierOptParamp->isDouble()) {
            مزدوج var;
            إذا (pinValuep->isDouble()) {
                var = pinValuep->انگ().منزل_Double();
            } وإلا {  // Cast from integer to real
                V3_عدد var_انگ{pinValuep, V3_عدد::Double{}, 0.0};
                var_انگ.opIToRD(pinValuep->انگ());
                var = var_انگ.منزل_Double();
            }
            أرجع V3_عدد::epsilonEqual(var, hierOptParamp->انگ().منزل_Double());
        } وإلا {  // Now integer type is assumed
            // Bitwidth of hierOptParamp is accurate because V3Width already calculated in the
            // previous run. Bitwidth of pinValuep is before width analysis, so pinValuep is casted
            // to hierOptParamp width.
            V3_عدد var_انگ{pinValuep, hierOptParamp->انگ().ويڪر()};
            إذا (pinValuep->isDouble()) {  // Need to cast to int
                // Parameter is actually an integral type, but passed value is floating point.
                // Conversion from real to integer uses rounding in V3Width.cpp
                var_انگ.opRToIRoundS(pinValuep->انگ());
            } وإلا إذا (pinValuep->isSigned()) {
                var_انگ.opExtendS(pinValuep->انگ(), pinValuep->انگ().ويڪر());
            } وإلا {
                var_انگ.op_سونپيو(pinValuep->انگ());
            }
            V3_عدد isEq{pinValuep, 1};
            isEq.opEq(var_انگ, hierOptParamp->انگ());
            أرجع isEq.isNeqZero();
        }
    }
};

//######################################################################
// Remove parameters from cells and build new modules

صنف ParamProcessor final {
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
    ثابت VNUser2_اندريون_Use m_inuser2;
    ثابت VNUser3_اندريون_Use m_inuser3;
    ثابت VNUser4_اندريون_Use m_inuser4;
    // User1 used by constant function simulations

    // TYPES
    // Note may have duplicate entries
    استعمال IfaceRefRefs = std::deque<std::pair<AstIfaceRefDType*, AstIfaceRefDType*>>;

    // STATE
    استعمال CloneMap = std::unordered_map<ثابت Ast_ڳنڍ*, Ast_ڳنڍ*>;
    بنية ModInfo final {
        Ast_ڳنڍ_Module* ثابت m_modp;  // Module with specified name
        CloneMap m_cloneMap;  // Map of old-varp -> new cloned varp
        واضح ModInfo(Ast_ڳنڍ_Module* modp)
            : m_modp{modp} {}
    };
    std::map<ثابت std::string, ModInfo> m_modNameMap;  // Hash of created module flavors by name

    std::map<ثابت std::string, std::string>
        m_longMap;  // Hash of very long names to unique identity number
    std::سيٽ<std::string> m_longSuffixes;  // Digest suffixes used by m_longMap

    // All module names that are loaded from source code
    // Generated modules by this visitor is not included
    VString_سيٽ m_all_Module_نالا;

    CloneMap m_originalParams;  // Map between parameters of copied parameteized classes and their
                                // original nodes

    std::map<ثابت std::string, std::string> m_قدر_نالا;  // Parameter value text to its name
    std::سيٽ<std::string> m_قدر_Suffixes;  // Digest suffixes used by m_valueNames
    // Number of digest hex digits in a name. With 8 digits, a collision between names made in
    // different runs, such as those of different hierarchical blocks, is unlikely. Within one
    // run, digestSuffix resolves a collision by lengthening the later text's suffix, which can
    // then differ between runs.
    ساكن constexpr std::string::سائيز_type DIGEST_DIGITS = 8;

    ثابت Ast_ڳنڍ_Module* m_modp = خالي_اشارو;  // Current module being processed

    // Database to get lib-create wrapper that matches parameters in hierarchical Verilation
    Parameterized_Hier_بلاڪون m_hier_بلاڪون;
    // Default parameter values key:parameter name, value:default value (can be nullptr)
    استعمال Default_قدر_Map = std::map<std::string, Ast_ڳنڍ*>;
    // Default parameter values of hierarchical blocks
    std::map<Ast_ڳنڍ_Module*, Default_قدر_Map> m_default_Parameter_قدرون;
    // Module -> a defparam in it or below it, or nullptr (see nestedDefparamp)
    std::map<ثابت Ast_ڳنڍ_Module*, ثابت AstPin*> m_nestedDefparamps;
    VNDeleter m_deleter;  // Used to delay deletion of nodes
    // Class default type paramater dependencies
    std::ويڪٽر<std::pair<AstParamTypeDType*, صحيح>> m_classTypeParams;
    std::unordered_map<AstParamTypeDType*, صحيح> m_param_اشاريو;

    // Guard against infinite recursion in classTypeMatchesDefaultClone slow path
    std::unordered_سيٽ<ثابت AstClass*> m_default_Clone_اندريون_ترقي;

    std::ويڪٽر<AstClass*> m_specializedClassps;  // Classes specialized since last drained

    // member names cached for fast lookup
    VMemberMap m_memberMap;

    // METHODS

    ساكن فراغ make_Small_نالا(Ast_ڳنڍ_Module* modp) {
        std::ويڪٽر<صحيح> استعمال_ٿيل_Letter;
        استعمال_ٿيل_Letter.resize(256);
        // Pass 1, assign first letter to each gparam's name
        لكل (Ast_ڳنڍ* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            إذا (AstVar* ثابت varp = VN_CAST(stmtp, Var)) {
                إذا (varp->isGParam() || varp->isIfaceRef()) {
                    حرف ch = varp->name()[0];
                    ch = std::وڏو_اکر(ch);
                    إذا (ch < 'A' || ch > 'Z') ch = 'Z';
                    varp->user3(استعمال_ٿيل_Letter[static_cast<صحيح>(ch)] * 256 + ch);
                    استعمال_ٿيل_Letter[static_cast<صحيح>(ch)]++;
                }
            } وإلا إذا (AstParamTypeDType* ثابت typep = VN_CAST(stmtp, ParamTypeDType)) {
                ثابت حرف ch = 'T';
                typep->user3(استعمال_ٿيل_Letter[static_cast<صحيح>(ch)] * 256 + ch);
                استعمال_ٿيل_Letter[static_cast<صحيح>(ch)]++;
            }
        }
    }
    ساكن string paramSmallName(Ast_ڳنڍ_Module* modp, Ast_ڳنڍ* varp) {
        إذا (varp->user3() <= 1) make_Small_نالا(modp);
        صحيح اشاريو = varp->user3() / 256;
        ثابت حرف ch = varp->user3() & 255;
        string st = cvt_منزل_Str(ch);
        طالما (اشاريو) {
            st += cvt_منزل_Str(static_cast<حرف>((اشاريو % 25) + 'A'));
            اشاريو /= 26;
        }
        أرجع st;
    }

    ساكن منطقي is_Aggregate_Param_قدر(ثابت Ast_ڳنڍ* nodep) {
        أرجع VN_IS(nodep, ConsArray) || VN_IS(nodep, ConsStruct);
    }

    ساكن string param_قدر_String(ثابت Ast_ڳنڍ* nodep) {
        إذا (ثابت AstRefDType* ثابت refp = VN_CAST(nodep, RefDType)) {
            nodep = refp->skip_Ref_منزل_Non_Refp();
        }
        string ڪنجي = nodep->name();
        إذا (ثابت AstIfaceRefDType* ثابت ifrtp = VN_CAST(nodep, IfaceRefDType)) {
            إذا (ifrtp->cellp() && ifrtp->cellp()->modp()) {
                ڪنجي = ifrtp->cellp()->modp()->name();
            } وإلا إذا (ifrtp->ifacep()) {
                ڪنجي = ifrtp->ifacep()->name();
            } وإلا {
                nodep->v3fatalSrc("Can't parameterize interface without module name");
            }
        } وإلا إذا (ثابت Ast_ڳنڍ_UOr_Struct_DType* ثابت dtypep
                   = VN_CAST(nodep, ڳنڍ_UOr_Struct_DType)) {
            ڪنجي += " ";
            ڪنجي += dtypep->verilogKwd();
            ڪنجي += " {";
            لكل (ثابت Ast_ڳنڍ* memberp = dtypep->membersp(); memberp;
                 memberp = memberp->nextp()) {
                ڪنجي += param_قدر_String(memberp);
                ڪنجي += ";";
            }
            ڪنجي += "}";
        } وإلا إذا (ثابت AstMemberDType* ثابت dtypep = VN_CAST(nodep, MemberDType)) {
            ڪنجي += " ";
            ڪنجي += param_قدر_String(dtypep->subDTypep());
        } وإلا إذا (ثابت AstBasicDType* ثابت dtypep = VN_CAST(nodep, BasicDType)) {
            إذا (dtypep->isSigned()) ڪنجي += " signed";
            إذا (dtypep->isRanged()) {
                ڪنجي += "[" + cvt_منزل_Str(dtypep->کاٻي()) + ":" + cvt_منزل_Str(dtypep->ساڄي()) + "]";
            }
        } وإلا إذا (ثابت AstPackArrayDType* ثابت dtypep = VN_CAST(nodep, PackArrayDType)) {
            ڪنجي += "[";
            ڪنجي += cvt_منزل_Str(dtypep->کاٻي());
            ڪنجي += ":";
            ڪنجي += cvt_منزل_Str(dtypep->ساڄي());
            ڪنجي += "] ";
            ڪنجي += param_قدر_String(dtypep->subDTypep());
        } وإلا إذا (ثابت Ast_نمونو* ثابت patp = VN_CAST(nodep, نمونو)) {
            // A parameter override pattern, keyed before the specialized module types it
            ڪنجي += "'{";
            لكل (ثابت AstPatMember* memp = VN_AS(patp->itemsp(), PatMember); memp;
                 memp = VN_AS(memp->nextp(), PatMember)) {
                إذا (memp->isDefault()) ڪنجي += "default";
                إذا (ثابت Ast_متن* ثابت textp = VN_CAST(memp->keyp(), متن_2)) {
                    ڪنجي += textp->متن();
                } وإلا إذا (memp->keyp()) {
                    ڪنجي += param_قدر_String(memp->keyp());
                }
                ڪنجي += ":";
                إذا (memp->repp()) ڪنجي += param_قدر_String(memp->repp()) + "x";
                لكل (ثابت Ast_ڳنڍ_اظهار* valuep = memp->lhssp(); valuep;
                     valuep = VN_AS(valuep->nextp(), ڳنڍ_اظهار)) {
                    ڪنجي += param_قدر_String(valuep) + ";";
                }
                ڪنجي += ",";
            }
            ڪنجي += "}";
        } وإلا إذا (ثابت AstConsArray* ثابت consArrayp = VN_CAST(nodep, ConsArray)) {
            // Include the indices and the default, as with a default the items may be sparse
            ڪنجي += "{";
            لكل (ثابت Ast_Cons_Array_شي* itemp = consArrayp->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), Cons_Array_شي)) {
                ڪنجي += cvt_منزل_Str(itemp->اشاريو()) + ":" + param_قدر_String(itemp->valuep()) + ",";
            }
            إذا (consArrayp->defaultp()) {
                ڪنجي += "default:" + param_قدر_String(consArrayp->defaultp()) + ",";
            }
            ڪنجي += "}";
        } وإلا إذا (ثابت AstConsStruct* ثابت consp = VN_CAST(nodep, ConsStruct)) {
            ڪنجي += "{";
            لكل (ثابت Ast_Cons_Struct_شي* itemp = consp->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), Cons_Struct_شي)) {
                ڪنجي += param_قدر_String(itemp->valuep());
                ڪنجي += ",";
            }
            ڪنجي += "}";
        } وإلا إذا (ثابت AstClassRefDType* ثابت classRefp = VN_CAST(nodep, ClassRefDType)) {
            // For parameterized class types, use the original class name (without specialization
            // suffix) plus the actual type parameter values. This ensures equivalent class types
            // get the same string representation regardless of which AST node is used.
            إذا (classRefp->classp()) {
                ثابت string& className = classRefp->classp()->name();
                ثابت string& origName = classRefp->classp()->origName();
                ثابت منطقي isSpecialized = (className != origName);
                UINFO(9, "paramValueString ClassRefDType: name="
                             << className << " origName=" << origName
                             << " isSpecialized=" << isSpecialized
                             << " hasParams=" << (classRefp->paramsp() ? "Y" : "N")
                             << " classHasGParam=" << classRefp->classp()->hasGParam());

                إذا (classRefp->paramsp()) {
                    // ClassRefDType should have been deparameterized (paramsp
                    // consumed) before cellPinCleanup calls paramValueString.
                    classRefp->v3fatalSrc(  // LCOV_EXCL_LINE
                        "ClassRefDType still has paramsp in paramValueString");
                } وإلا إذا (isSpecialized) {
                    // Already specialized class (e.g., c1__Tz1_TBz1) - use full name
                    // This ensures different specializations are distinguished
                    ڪنجي = className;
                } وإلا {
                    // Unspecialized class with no params - use origName
                    ڪنجي = origName;
                }
            } وإلا {
                // classp() should always be set; unresolved class refs
                // would have errored in LinkDot.
                classRefp->v3fatalSrc(  // LCOV_EXCL_LINE
                    "ClassRefDType has null classp in paramValueString");
            }
        } وإلا إذا (ثابت AstUnpackArrayDType* ثابت dtypep = VN_CAST(nodep, UnpackArrayDType)) {
            // Name containers by their elements as above, not by prettyDTypeName(), which names
            // classes by parameter values that may be yet to be resolved
            ڪنجي = paramElemString(dtypep->subDTypep()) + "$" + cvt_منزل_Str(dtypep->declRange());
        } وإلا إذا (ثابت Ast_قطار_DType* ثابت dtypep = VN_CAST(nodep, قطار_DType)) {
            ڪنجي = paramElemString(dtypep->subDTypep()) + "$[$";
            إذا (dtypep->boundConst()) ڪنجي += ":" + cvt_منزل_Str(dtypep->boundConst());
            ڪنجي += "]";
        } وإلا إذا (ثابت AstDynArrayDType* ثابت dtypep = VN_CAST(nodep, DynArrayDType)) {
            ڪنجي = paramElemString(dtypep->subDTypep()) + "$[]";
        } وإلا إذا (ثابت AstAssocArrayDType* ثابت dtypep = VN_CAST(nodep, AssocArrayDType)) {
            ڪنجي = paramElemString(dtypep->subDTypep()) + "$["
                  + paramElemString(dtypep->ڪنجي_DTypep()) + "]";
        } وإلا إذا (ثابت AstWildcardArrayDType* ثابت dtypep
                   = VN_CAST(nodep, WildcardArrayDType)) {
            ڪنجي = paramElemString(dtypep->subDTypep()) + "$[*]";
        } وإلا إذا (ثابت AstEnumDType* ثابت dtypep = VN_CAST(nodep, EnumDType)) {
            // With the items, as prettyDTypeName() omits them, yet a specialization of a class
            // may declare an enumeration of the same name with other values
            ڪنجي += " enum {";
            لكل (ثابت Ast_Enum_شي* itemp = dtypep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), Enum_شي)) {
                ثابت AstConst* ثابت constp = VN_CAST(itemp->valuep(), Const);
                ڪنجي += itemp->name() + "=" + (constp ? constp->انگ().ascii() : "?") + ";";
            }
            ڪنجي += "}";
        } وإلا إذا (ثابت Ast_ڳنڍ_DType* ثابت dtypep = VN_CAST(nodep, ڳنڍ_DType)) {
            ڪنجي += dtypep->prettyDTypeName(سچ);
        }
        UASSERT_OBJ(!ڪنجي.خالي_جايون(), nodep, "Parameter yielded no value string");
        أرجع ڪنجي;
    }
    // As paramValueString(), for the element or key type of a container
    ساكن string paramElemString(ثابت Ast_ڳنڍ_DType* dtypep) {
        // Unlike at the top of a value, a class may be yet to be specialized, so just name it
        ثابت AstClassRefDType* ثابت classRefp
            = VN_CAST(dtypep->skip_Ref_منزل_Non_Refp(), ClassRefDType);
        إذا (classRefp && classRefp->paramsp()) أرجع classRefp->prettyDTypeName(سچ);
        أرجع param_قدر_String(dtypep);
    }

    // Return a name suffix for 'text' from its SHA-512 digest. Hierarchical blocks are
    // Verilated in separate runs, where a counter would restart, but the digest is the same in
    // every run. Use the shortest digest prefix of at least DIGEST_DIGITS digits that is not
    // already in 'usedr', and add it to 'usedr'.
    ساكن string digestSuffix(ثابت string& متن, std::سيٽ<string>& usedr) {
        ثابت string hex = VHashSha512{متن}.digestHex();
        // Force collisions of the prefixes -- for testing only
        string::سائيز_type digits = v3Global.opt.debugCollision() ? 1 : DIGEST_DIGITS;
        طالما (digits < hex.سائيز() && !usedr.وجهو(hex.substr(0, digits)).ٻيو) ++digits;
        أرجع hex.substr(0, digits);
    }
    string param_قدر_عدد(Ast_ڳنڍ* nodep) {
        // All call sites resolve through skipRefToNonRefp() or pass non-DType
        // nodes, so nodep should never be a bare RefDType here.
        إذا (VN_IS(nodep, RefDType)) {  // LCOV_EXCL_LINE
            nodep->v3fatalSrc("Unexpected RefDType in paramValueNumber");  // LCOV_EXCL_LINE
        }
        // Name the value by its text, which is the same for equal values or types in every run.
        // V3Hasher is unsuitable, as it hashes node pointers, which can differ for equal types.
        // V3Hash of a string is unsuitable, as std::hash varies between C++ libraries.
        ثابت string متن = param_قدر_String(nodep);
        ثابت خودڪار pair = m_قدر_نالا.emplace(متن, "");
        إذا (pair.ٻيو) pair.پهريون->ٻيو = "z" + digestSuffix(متن, m_قدر_Suffixes);
        أرجع pair.پهريون->ٻيو;
    }
    string moduleCalcName(ثابت Ast_ڳنڍ_Module* srcModp, ثابت string& longname) {
        string newname = longname;
        إذا (longname.ڊيگهائي() > 30) {
            ثابت خودڪار pair = m_longMap.emplace(longname, "");
            إذا (pair.ٻيو) {
                // We use all upper case above, so lower here can't conflict
                pair.پهريون->ٻيو
                    = srcModp->name() + "__pi" + digestSuffix(longname, m_longSuffixes);
            }
            newname = pair.پهريون->ٻيو;
        }
        UINFO(4, "Name: " << srcModp->name() << "->" << longname << "->" << newname);
        أرجع newname;
    }
    ساكن Ast_ڳنڍ_DType* arraySubDTypep(Ast_ڳنڍ_DType* nodep) {
        // If an unpacked array, return the subDTypep under it
        إذا (ثابت AstUnpackArrayDType* ثابت adtypep = VN_CAST(nodep, UnpackArrayDType)) {
            أرجع adtypep->subDTypep();
        }
        // We have not resolved parameter of the child yet, so still
        // have BracketArrayDType's. We'll presume it'll end up as assignment
        // compatible (or V3Width will complain).
        إذا (ثابت AstBracketArrayDType* ثابت adtypep = VN_CAST(nodep, BracketArrayDType)) {
            أرجع adtypep->subDTypep();
        }
        أرجع خالي_اشارو;
    }
    // Peel all unpacked-array layers to reach the innermost subDTypep.
    // Used for multi-dim iface arrays where port dtype is nested UnpackArrayDType.
    ساكن Ast_ڳنڍ_DType* arraySubDTypeDeepp(Ast_ڳنڍ_DType* nodep) {
        طالما (Ast_ڳنڍ_DType* ثابت subp = arraySubDTypep(nodep)) nodep = subp;
        أرجع nodep;
    }
    ساكن منطقي isString(Ast_ڳنڍ_DType* nodep) {
        إذا (AstBasicDType* ثابت basicp = VN_CAST(nodep->skip_Ref_منزل_Non_Refp(), BasicDType))
            أرجع basicp->isString();
        أرجع خطأ;
    }
    فراغ collectPins(CloneMap* clonemapp, Ast_ڳنڍ_Module* modp, منطقي originalIsCopy) {
        // Grab all I/O so we can remap our pins later
        لكل (Ast_ڳنڍ* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ثابت Ast_ڳنڍ* originalParamp = خالي_اشارو;
            إذا (AstVar* ثابت varp = VN_CAST(stmtp, Var)) {
                إذا (varp->isIO() || varp->isGParam() || varp->isIfaceRef()) {
                    // Cloning saved a pointer to the new node for us, so just follow that link.
                    originalParamp = varp->clonep();
                }
            } وإلا إذا (AstParamTypeDType* ثابت ptp = VN_CAST(stmtp, ParamTypeDType)) {
                إذا (ptp->isGParam()) originalParamp = ptp->clonep();
            }
            إذا (originalIsCopy) originalParamp = m_originalParams[originalParamp];
            إذا (originalParamp) clonemapp->emplace(originalParamp, stmtp);
        }
    }
    فراغ relinkPins(ثابت CloneMap* clonemapp, AstPin* startpinp) {
        لكل (AstPin* pinp = startpinp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            إذا (pinp->modVarp()) {
                // Find it in the clone structure
                // UINFO(8,"Clone find 0x"<<hex<<(uint32_t)pinp->modVarp());
                ثابت خودڪار cloneiter = clonemapp->ڳوليو(pinp->modVarp());
                UASSERT_OBJ(cloneiter != clonemapp->پڇاڙي(), pinp,
                            "Couldn't find pin in clone list");
                pinp->modVarp(VN_AS(cloneiter->ٻيو, Var));
            } وإلا إذا (pinp->modPTypep()) {
                ثابت خودڪار cloneiter = clonemapp->ڳوليو(pinp->modPTypep());
                UASSERT_OBJ(cloneiter != clonemapp->پڇاڙي(), pinp,
                            "Couldn't find pin in clone list");
                pinp->modPTypep(VN_AS(cloneiter->ٻيو, ParamTypeDType));
            } وإلا {
                pinp->v3fatalSrc("Not linked?");
            }
        }
    }
    فراغ relinkPinsByName(AstPin* startpinp, Ast_ڳنڍ_Module* modp) {
        std::map<ثابت string, AstVar*> name_منزل_Pin;
        لكل (Ast_ڳنڍ* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            إذا (AstVar* ثابت varp = VN_CAST(stmtp, Var)) {
                إذا (varp->isIO() || varp->isGParam() || varp->isIfaceRef()) {
                    name_منزل_Pin.emplace(varp->name(), varp);
                }
            }
        }
        لكل (AstPin* pinp = startpinp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            إذا (ثابت AstVar* ثابت varp = pinp->modVarp()) {
                ثابت خودڪار varIt = vlstd::as_const(name_منزل_Pin).ڳوليو(varp->name());
                UASSERT_OBJ(varIt != name_منزل_Pin.پڇاڙي(), varp,
                            "Not found in " << modp->prettyNameQ());
                pinp->modVarp(varIt->ٻيو);
            }
        }
    }
    ساكن منطقي hasDescendantDefparams(ثابت Ast_ڳنڍ_Module* modp,
                                       std::سيٽ<ثابت Ast_ڳنڍ_Module*>& گهميل) {
        إذا (!گهميل.وجهو(modp).ٻيو) أرجع خطأ;
        لكل (Ast_ڳنڍ* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstCell* ثابت cellp = VN_CAST(stmtp, Cell);
            إذا (!cellp) استمر;
            لكل (AstPin* pinp = cellp->paramsp(); pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                إذا (!pinp->param_رستو().خالي_جايون()) {
                    گهميل.erase(modp);
                    أرجع سچ;
                }
            }
            إذا (Ast_ڳنڍ_Module* ثابت ٻار_Modp = cellp->modp()) {
                إذا (hasDescendantDefparams(ٻار_Modp, گهميل)) {
                    گهميل.erase(modp);
                    أرجع سچ;
                }
            }
        }
        گهميل.erase(modp);
        أرجع خطأ;
    }
    ساكن منطقي hasDescendantDefparams(ثابت Ast_ڳنڍ_Module* modp) {
        std::سيٽ<ثابت Ast_ڳنڍ_Module*> گهميل;
        أرجع hasDescendantDefparams(modp, گهميل);
    }
    // Return a defparam in the module or below it. Unlike hasDescendantDefparams, also look
    // inside generate constructs. Memoized, so each module is searched once.
    ثابت AstPin* nestedDefparamp(ثابت Ast_ڳنڍ_Module* modp) {
        ثابت خودڪار pair = m_nestedDefparamps.emplace(modp, خالي_اشارو);
        إذا (!pair.ٻيو) أرجع pair.پهريون->ٻيو;  // Searched, or being searched
        ثابت AstPin* foundp = خالي_اشارو;
        modp->exists([&](ثابت AstCell* cellp) {
            لكل (ثابت AstPin* pinp = cellp->paramsp(); pinp && !foundp;
                 pinp = VN_AS(pinp->nextp(), Pin)) {
                إذا (!pinp->param_رستو().خالي_جايون()) foundp = pinp;
            }
            إذا (!foundp && cellp->modp()) foundp = nestedDefparamp(cellp->modp());
            أرجع foundp != خالي_اشارو;
        });
        pair.پهريون->ٻيو = foundp;
        أرجع foundp;
    }
    // Check if parameter setting during instantiation is simple enough for hierarchical Verilation
    فراغ checkSupportedParam(Ast_ڳنڍ_Module* modp, AstPin* pinp) ثابت {
        // ConsArray is not supported because that can not be set via -G
        // option.
        إذا (pinp->modVarp()) {
            منطقي supported = خطأ;
            إذا (ثابت AstConst* ثابت constp = VN_CAST(pinp->exprp(), Const)) {
                supported = سچ;
                إذا (constp->انگ().isString()
                    && !V3_Hier_بلاڪ::stringParamPassable(constp->انگ().منزل_String())) {
                    pinp->v3warn(E_UNSUPPORTED, "Unsupported: String value of hierarchical block"
                                                " parameter with newline, double quote, '/*',"
                                                " or whitespace before '//'");
                }
            }
            إذا (!supported) {
                pinp->v3error(
                    Ast_ڳنڍ::prettyNameQ(modp->origName())
                    << " has hier_block metacomment, hierarchical Verilation"
                    << " supports only integer/floating point/string and type param parameters");
            }
        } وإلا إذا (ثابت AstParamTypeDType* ثابت typep = pinp->modPTypep()) {
            // Libraries are found by comparing values only
            pinp->v3warn(E_UNSUPPORTED, "Unsupported: Setting type parameter "
                                            << typep->prettyNameQ() << " of hierarchical block "
                                            << Ast_ڳنڍ::prettyNameQ(modp->origName()));
        }
    }
    منطقي moduleExists(ثابت string& modName) ثابت {
        إذا (m_all_Module_نالا.ڳوليو(modName) != m_all_Module_نالا.پڇاڙي()) أرجع سچ;
        إذا (m_modNameMap.ڳوليو(modName) != m_modNameMap.پڇاڙي()) أرجع سچ;
        أرجع خطأ;
    }

    string parameterized_Hier_بلاڪ_Name(Ast_ڳنڍ_Module* modp, AstPin* paramPinsp) {
        // Create a unique name in the following steps
        //  - Make a long name that includes all parameters, that appear
        //    in the alphabetical order.
        //  - Hash the long name to get valid Verilog symbol
        UASSERT_OBJ(modp->hier_بلاڪ(), modp, "should be used for hierarchical block");

        std::map<string, Ast_ڳنڍ*> pins;

        AstPin* pinp = paramPinsp;
        طالما (pinp) {
            checkSupportedParam(modp, pinp);
            إذا (ثابت AstVar* ثابت varp = pinp->modVarp()) {
                إذا (!pinp->exprp()) استمر;
                إذا (varp->isGParam()) pins.emplace(varp->name(), pinp->exprp());
            } وإلا إذا (VN_IS(pinp->exprp(), BasicDType) || VN_IS(pinp->exprp(), ڳنڍ_DType)) {
                pins.emplace(pinp->name(), pinp->exprp());
            }
            pinp = VN_AS(pinp->nextp(), Pin);
        }

        ثابت خودڪار pair = m_default_Parameter_قدرون.emplace(
            std::piecewise_construct, std::forward_as_tuple(modp), std::forward_as_tuple());
        إذا (pair.ٻيو) {  // Not cached yet, so check parameters
            // A separately compiled library cannot receive defparams aimed into its body
            إذا (ثابت AstPin* ثابت defparamp = nestedDefparamp(modp)) {
                defparamp->v3warn(E_UNSUPPORTED, "Unsupported: defparam inside hierarchical block "
                                                     << Ast_ڳنڍ::prettyNameQ(modp->origName()));
            }
            // Using map with key=string so that we can scan it in deterministic order
            Default_قدر_Map params;
            لكل (Ast_ڳنڍ* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                إذا (AstVar* ثابت varp = VN_CAST(stmtp, Var)) {
                    إذا (varp->isGParam()) {
                        AstConst* ثابت constp = VN_CAST(varp->valuep(), Const);
                        // constp can be nullptr if the parameter is not used to instantiate sub
                        // module. varp->valuep() is not constified yet in the case.
                        // nullptr means that the parameter is using some default value.
                        params.emplace(varp->name(), constp);
                    }
                } وإلا إذا (AstParamTypeDType* ثابت p = VN_CAST(stmtp, ParamTypeDType)) {
                    Ast_ڳنڍ* ثابت dtypep = static_cast<Ast_ڳنڍ*>(p->skipRefp());
                    params.emplace(p->name(), dtypep);
                }
            }
            pair.پهريون->ٻيو = std::move(params);
        }
        ثابت خودڪار paramsIt = pair.پهريون;
        إذا (paramsIt->ٻيو.خالي_جايون()) أرجع modp->origName();  // modp has no parameter

        string longname = modp->origName();
        لكل (خودڪار&& default_قدر : paramsIt->ٻيو) {
            ثابت خودڪار pinIt = pins.ڳوليو(default_قدر.پهريون);
            // If the pin does not have a value assigned, use the default one.
            ثابت Ast_ڳنڍ* ثابت nodep = pinIt == pins.پڇاڙي() ? default_قدر.ٻيو : pinIt->ٻيو;
            // This longname is not valid as verilog symbol, but ok, because it will be hashed
            longname += "_" + default_قدر.پهريون + "=";
            // constp can be nullptr

            إذا (ثابت AstConst* ثابت p = VN_CAST(nodep, Const)) {
                // Treat modules parameterized with the same values but with different type as the
                // same.
                longname += p->انگ().ascii(خطأ);
            } وإلا إذا (nodep) {
                std::stringstream type;
                V3EmitV::verilog_For_وڻ(nodep, type);
                longname += type.str();
            }
        }
        UINFO(9, "       module params longname: " << longname);

        ثابت خودڪار iter = m_longMap.ڳوليو(longname);
        إذا (iter != m_longMap.پڇاڙي()) أرجع iter->ٻيو;  // Already calculated

        VHashSha512 hash;
        // Calculate hash using longname
        // The hash is used as the module suffix to find a module name that is unique in the design
        hash.وجهو(longname);
        طالما (سچ) {
            // Copy VHashSha512 just in case of hash collision
            VHashSha512 hashStrGen = hash;
            // Hex string must be a safe suffix for any symbol
            ثابت string hashStr = hashStrGen.digestHex();
            لكل (string::سائيز_type i = 1; i < hashStr.سائيز(); ++i) {
                string newName = modp->origName();
                // Don't use '__' not to be encoded when this module is loaded later by Verilator
                إذا (newName.at(newName.سائيز() - 1) != '_') newName += '_';
                newName += hashStr.substr(0, i);
                إذا (!moduleExists(newName)) {
                    m_longMap.emplace(longname, newName);
                    أرجع newName;
                }
            }
            // Hash collision. maybe just v3error is practically enough
            hash.وجهو(V3Os::trueRandom(64));
        }
    }
    فراغ replaceRefsRecurse(Ast_ڳنڍ* ثابت nodep, ثابت AstClass* ثابت oldClassp,
                            AstClass* ثابت newClassp) {
        // Self references linked in the first pass of V3LinkDot.cpp should point to the default
        // instance.
        إذا (AstClassRefDType* ثابت classRefp = VN_CAST(nodep, ClassRefDType)) {
            إذا (classRefp->classp() == oldClassp) classRefp->classp(newClassp);
        } وإلا إذا (AstClassOrPackageRef* ثابت classRefp = VN_CAST(nodep, ClassOrPackageRef)) {
            إذا (classRefp->classOrPackageSkipp() == oldClassp)
                classRefp->classOrPackagep(newClassp);
        } وإلا إذا (AstTypedef* ثابت typedefp = VN_CAST(nodep, Typedef)) {
            // Update typedefs that refer to the old class to point to the new class
            إذا (typedefp->subDTypep()) {
                إذا (AstClassRefDType* ثابت classRefp
                    = VN_CAST(typedefp->subDTypep(), ClassRefDType)) {
                    إذا (classRefp->classp() == oldClassp) { classRefp->classp(newClassp); }
                }
            }
        } وإلا إذا (Ast_ڳنڍ_FTask_Ref* ثابت ftaskRefp = VN_CAST(nodep, ڳنڍ_FTask_Ref)) {
            // Also update FuncRef/TaskRef packagep to point to new class
            // This fixes static method calls through typedefs in parameterized classes
            إذا (ftaskRefp->classOrPackagep() == oldClassp) ftaskRefp->classOrPackagep(newClassp);
            // Also update taskp if it points to a function in the old class.
            // AstNodeFTask::classOrPackagep() (op2) holds a parse-time
            // Dot/ClassOrPackageRef for extern declarations, which is deleted
            // in LinkDot::moveExternFuncDecl before V3Param runs. For inline
            // class methods op2 is nullptr. So this should never match.
            إذا (Ast_ڳنڍ_FTask* ثابت oldTaskp = ftaskRefp->taskp()) {
                إذا (oldTaskp->classOrPackagep() == oldClassp) {
                    oldTaskp->v3fatalSrc(  // LCOV_EXCL_LINE
                        "FTask classOrPackagep unexpectedly matches old class "
                        << oldClassp->prettyNameQ());
                }
            }
        }

        إذا (nodep->op1p()) replaceRefsRecurse(nodep->op1p(), oldClassp, newClassp);
        إذا (nodep->op2p()) replaceRefsRecurse(nodep->op2p(), oldClassp, newClassp);
        إذا (nodep->op3p()) replaceRefsRecurse(nodep->op3p(), oldClassp, newClassp);
        إذا (nodep->op4p()) replaceRefsRecurse(nodep->op4p(), oldClassp, newClassp);
        إذا (nodep->nextp()) replaceRefsRecurse(nodep->nextp(), oldClassp, newClassp);
    }

    // Helper visitor to update VarXRefs to use variables from specialized interfaces.
    // When a module with interface ports is cloned and the port's interface is remapped
    // to a specialized version, VarXRefs that access members of the old interface need
    // to be updated to reference the corresponding members in the new interface.
    صنف VarXRefRelinkVisitor final : عام VNVisitor {
        Ast_ڳنڍ_Module* m_modp;  // The cloned module we're updating
        std::unordered_map<AstVar*, Ast_ڳنڍ_Module*> m_varModuleMap;  // Cache var->module lookups

    عام:
        واضح VarXRefRelinkVisitor(Ast_ڳنڍ_Module* newModp)
            : m_modp{newModp} {
            iterate(newModp);
        }

    خاص:
        // Find which module a variable belongs to, using cache to avoid repeated backp() walks
        Ast_ڳنڍ_Module* ڳوليو_Var_Module(AstVar* varp) {
            ثابت خودڪار it = m_varModuleMap.ڳوليو(varp);
            إذا (it != m_varModuleMap.پڇاڙي()) أرجع it->ٻيو;
            Ast_ڳنڍ_Module* varModp = خالي_اشارو;
            لكل (Ast_ڳنڍ* np = varp; np; np = np->backp()) {
                إذا (Ast_ڳنڍ_Module* ثابت modp = VN_CAST(np, ڳنڍ_Module)) {
                    varModp = modp;
                    اكسر;
                }
            }
            m_varModuleMap[varp] = varModp;
            أرجع varModp;
        }

        فراغ گهمو(AstVarXRef* nodep) override {
            AstVar* ثابت varp = nodep->varp();
            إذا (!varp) {
                iterateChildren(nodep);
                أرجع;
            }

            // Get the dotted prefix (port name) from the VarXRef
            // dotted() format: "portname" or "portname.subpath" or empty
            ثابت string& dotted = nodep->dotted();
            إذا (dotted.خالي_جايون()) {
                iterateChildren(nodep);
                أرجع;
            }

            ثابت سائيز_t dotPos = dotted.ڳوليو('.');
            ثابت string پورٽ_Name = (dotPos == string::npos) ? dotted : dotted.substr(0, dotPos);
            إذا (پورٽ_Name.خالي_جايون()) {
                iterateChildren(nodep);
                أرجع;
            }

            // Find the interface port variable in the cloned module
            AstVar* پورٽ_Varp = خالي_اشارو;
            لكل (Ast_ڳنڍ* stmtp = m_modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                إذا (AstVar* ثابت varChkp = VN_CAST(stmtp, Var)) {
                    إذا (varChkp->name() == پورٽ_Name && varChkp->isIfaceRef()) {
                        پورٽ_Varp = varChkp;
                        اكسر;
                    }
                }
            }
            إذا (!پورٽ_Varp) {
                iterateChildren(nodep);
                أرجع;
            }

            // Get the interface module from the port's dtype
            AstIfaceRefDType* ثابت irefp = VN_CAST(پورٽ_Varp->subDTypep(), IfaceRefDType);
            إذا (!irefp) {
                iterateChildren(nodep);
                أرجع;
            }

            Ast_ڳنڍ_Module* ثابت newIfacep = irefp->iface_ذريعي_Cellp();
            إذا (!newIfacep) {
                iterateChildren(nodep);
                أرجع;
            }

            // Find which module the variable currently belongs to (cached)
            Ast_ڳنڍ_Module* ثابت varModp = ڳوليو_Var_Module(varp);

            // If variable is in a different module than the port's interface, remap it
            إذا (varModp && varModp != newIfacep) {
                لكل (Ast_ڳنڍ* stmtp = newIfacep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                    إذا (AstVar* ثابت newVarp = VN_CAST(stmtp, Var)) {
                        إذا (newVarp->name() == varp->name()) {
                            UINFO(9, "VarXRef relink " << varp->name() << " in " << varModp->name()
                                                       << " -> " << newIfacep->name());
                            nodep->varp(newVarp);
                            اكسر;
                        }
                    }
                }
            }
            iterateChildren(nodep);
        }
        فراغ گهمو(Ast_ڳنڍ* nodep) override { iterateChildren(nodep); }
    };

    // Retarget the captured references in newModp to its specialized interfaces.
    فراغ fixupCrossModuleRefDTypes(Ast_ڳنڍ_Module* newModp) {
        إذا (!V3LinkDotIfaceCapture::enabled()) أرجع;
        UINFO(9, "iface capture clone fixup: " << newModp->prettyNameQ());
        صحيح fixed_ڳڻپ = 0;
        // References in a class nested in newModp (e.g. a covergroup) are included.
        newModp->foreach([&](AstRefDType* refp) {
            ثابت VIface_Capture_ٽيگ* ثابت tagp = V3LinkDotIfaceCapture::capture_ٽيگ(refp);
            إذا (!tagp) أرجع;

            Ast_ڳنڍ_Module* ثابت correctModp
                = V3LinkDotIfaceCapture::follow_Cell_رستو(newModp, tagp->m_cell_رستو);
            UINFO(9, "  path fixup: " << refp << " cellPath='" << tagp->m_cell_رستو << "' -> "
                                      << (correctModp ? correctModp->name() : "<null>"));
            إذا (!correctModp || correctModp->dead()) أرجع;
            إذا (correctModp->parameterizedTemplate()) أرجع;

            منطقي fixed = خطأ;
            إذا (refp->typedefp()) {
                إذا (AstTypedef* ثابت newTdp = V3LinkDotIfaceCapture::ڳوليو_Typedef_اندريون_Module(
                        correctModp, refp->typedefp()->name())) {
                    refp->typedefp(newTdp);
                    إذا (newTdp->subDTypep()) refp->refDTypep(newTdp->subDTypep());
                    fixed = سچ;
                }
            }
            إذا (!fixed && refp->refDTypep()) {
                إذا (Ast_ڳنڍ_DType* ثابت newDtp = V3LinkDotIfaceCapture::ڳوليو_DType_اندريون_Module(
                        correctModp, refp->refDTypep()->name(), refp->refDTypep()->type())) {
                    refp->refDTypep(newDtp);
                    fixed = سچ;
                }
            }
            إذا (fixed) ++fixed_ڳڻپ;
        });
        V3Stats::جوڙيو_Stat_جوڙ("IfaceCapture, Ledger fixups in V3Param", fixed_ڳڻپ);
    }

    // Return true on success, false on error
    منطقي deepCloneModule(Ast_ڳنڍ_Module* srcModp, Ast_ڳنڍ* ifErrorp, AstPin* paramsp,
                         ثابت string& newname, ثابت IfaceRefRefs& ifaceRefRefs) {
        // Deep clone of new module
        // Note all module internal variables will be re-linked to the new modules by clone
        // However links outside the module (like on the upper cells) will not.
        Ast_ڳنڍ_Module* newModp;
        إذا (srcModp->user3p()) {
            newModp = VN_CAST(srcModp->user3p()->clone_وڻ(خطأ), ڳنڍ_Module);
        } وإلا {
            newModp = srcModp->clone_وڻ(خطأ);
        }
        // AstPin normally retains links to external module formals across cloning. For a cloned
        // interface, relink pins whose formals were cloned with the interface while clonep() is
        // still valid, so nested parameterized classes use the cloned interface parameters.
        إذا (AstIface* ثابت newIfacep = VN_CAST(newModp, Iface)) {
            newIfacep->foreach([](AstPin* pinp) { pinp->cloneRelinkGen(); });
        }

        // Mark the source module as a parameterized template now that a specialized
        // clone exists.  This suppresses width/type errors on the unresolved template
        // during widthParamsEdit (which runs before V3LinkDot sets dead()).
        srcModp->parameterizedTemplate(سچ);
        // The clone is a specialized instance, not a template.  Clear the flag in
        // case it was inherited from a prior cloneTree (when srcModp was already
        // marked by an earlier specialization).
        newModp->parameterizedTemplate(خطأ);

        newModp->name(newname);
        newModp->user2(خطأ);  // We need to re-recurse this module once changed
        newModp->recursive(خطأ);
        newModp->recursiveClone(خطأ);
        newModp->hasGParam(خطأ);
        // Recursion may need level cleanups
        إذا (newModp->level() <= m_modp->level()) newModp->level(m_modp->level() + 1);
        إذا ((newModp->level() - srcModp->level()) >= (v3Global.opt.module_Recursion_اونهائي() - 2)) {
            ifErrorp->v3error("Exceeded maximum --module-recursion-depth of "
                              << v3Global.opt.module_Recursion_اونهائي());
            VL_DO_DANGLING(newModp->delete_وڻ(), newModp);
            أرجع خطأ;
        }

        إذا (AstClass* ثابت newClassp = VN_CAST(newModp, Class)) {
            replaceRefsRecurse(newModp->stmtsp(), newClassp, VN_AS(srcModp, Class));
        }

        // Keep tree sorted by level. Note: Different parameterizations of the same recursive
        // module end up with the same level, which we will need to fix up at the end, as we do not
        // know up front how recursive modules are expanded, and a later expansion might re-use an
        // earlier expansion (see t_recursive_module_bug_2).
        Ast_ڳنڍ* insertp = srcModp;
        طالما (VN_IS(insertp->nextp(), ڳنڍ_Module)
               && VN_AS(insertp->nextp(), ڳنڍ_Module)->level() <= newModp->level()) {
            insertp = insertp->nextp();
        }
        insertp->جوڙيو_ايندڙ_هتي(newModp);

        m_modNameMap.emplace(newModp->name(), ModInfo{newModp});
        ثابت خودڪار iter = m_modNameMap.ڳوليو(newname);
        CloneMap* ثابت clonemapp = &(iter->ٻيو.m_cloneMap);
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
        منطقي needRelinkByName = خطأ;
        إذا (paramsp) {
            لكل (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                إذا (pinp->modVarp() && clonemapp->ڳوليو(pinp->modVarp()) == clonemapp->پڇاڙي()) {
                    needRelinkByName = سچ;
                    اكسر;
                }
            }
        }
        إذا (needRelinkByName) {
            relinkPinsByName(paramsp, newModp);
        } وإلا {
            relinkPins(clonemapp, paramsp);
        }

        // Fix any interface references
        لكل (خودڪار it = ifaceRefRefs.cbegin(); it != ifaceRefRefs.cend(); ++it) {
            ثابت AstIfaceRefDType* ثابت پورٽ_Irefp = it->پهريون;
            ثابت AstIfaceRefDType* ثابت pinIrefp = it->ٻيو;
            AstIfaceRefDType* ثابت cloneIrefp = پورٽ_Irefp->clonep();
            UINFO(8, "     IfaceOld " << پورٽ_Irefp);
            UINFO(8, "     IfaceTo  " << pinIrefp);
            UASSERT_OBJ(cloneIrefp, پورٽ_Irefp, "parameter clone didn't hit AstIfaceRefDType");
            UINFO(8, "     IfaceClo " << cloneIrefp);
            cloneIrefp->ifacep(pinIrefp->iface_ذريعي_Cellp());
            UINFO(8, "     IfaceNew " << cloneIrefp);
        }

        // Fix VarXRefs that reference variables in old interfaces.
        // Now that interface port dtypes have been updated above, we can use them
        // to find the correct interface for each VarXRef.
        إذا (!ifaceRefRefs.خالي_جايون()) { VarXRefRelinkVisitor{newModp}; }

        // Fix cross-module REFDTYPE pointers in newModp.
        UASSERT_OBJ(newModp, srcModp, "newModp null before hierarchy fixup");
        fixupCrossModuleRefDTypes(newModp);

        // Assign parameters to the constants specified
        // DOES clone() so must be finished with module clonep() before here
        لكل (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            إذا (pinp->exprp()) {
                إذا (AstVar* ثابت modvarp = pinp->modVarp()) {
                    Ast_ڳنڍ* ثابت newp = pinp->exprp();  // Const or ConsArray
                    AstConst* ثابت exprp = VN_CAST(newp, Const);
                    AstConst* ثابت origp = VN_CAST(modvarp->valuep(), Const);
                    ثابت منطقي overridden
                        = !(origp && exprp && Parameterized_Hier_بلاڪون::areSame(exprp, origp));
                    // Remove any existing parameter
                    إذا (modvarp->valuep()) modvarp->valuep()->unlink_Fr_پٺ()->delete_وڻ();
                    // Set this parameter to value requested by cell
                    UINFO(9, "       set param " << modvarp << " = " << newp);
                    modvarp->valuep(newp->clone_وڻ(خطأ));
                    modvarp->overriddenParam(overridden);
                } وإلا إذا (AstParamTypeDType* ثابت modptp = pinp->modPTypep()) {
                    Ast_ڳنڍ_DType* ثابت dtypep = VN_AS(pinp->exprp(), ڳنڍ_DType);
                    UASSERT_OBJ(dtypep, pinp, "unlinked param dtype");
                    إذا (modptp->ٻار_DTypep()) modptp->ٻار_DTypep()->unlink_Fr_پٺ()->delete_وڻ();
                    // Set this parameter to value requested by cell
                    modptp->ٻار_DTypep(dtypep->clone_وڻ(خطأ));
                    // Later V3LinkDot will convert the ParamDType to a Typedef
                    // Not done here as may be localparams, etc, that also need conversion
                }
            }
        }

        أرجع سچ;
    }
    ثابت ModInfo* module_ڳوليو_Or_Clone(Ast_ڳنڍ_Module* srcModp, Ast_ڳنڍ* ifErrorp, AstPin* paramsp,
                                     ثابت string& newname, ثابت IfaceRefRefs& ifaceRefRefs) {
        // Already made this flavor?
        خودڪار it = m_modNameMap.ڳوليو(newname);
        إذا (it != m_modNameMap.پڇاڙي()) {
            UINFO(4, "     De-parameterize to prev: " << it->ٻيو.m_modp);
        } وإلا {
            إذا (!deepCloneModule(srcModp, ifErrorp, paramsp, newname, ifaceRefRefs)) {
                أرجع خالي_اشارو;
            }
            it = m_modNameMap.ڳوليو(newname);
            UASSERT(it != m_modNameMap.پڇاڙي(), "should find just-made module");
        }
        ثابت ModInfo* ثابت modInfop = &(it->ٻيو);
        أرجع modInfop;
    }

    فراغ convert_منزل_Stringp(Ast_ڳنڍ* nodep) {
        // Should be called on values of parameters of type string to convert them
        // to properly typed string constants.
        // Has no effect if the value is not a string constant.
        AstConst* ثابت constp = VN_CAST(nodep, Const);
        // Check if it wasn't already converted
        إذا (constp && !constp->انگ().isString()) {
            constp->replaceWith(
                جديد AstConst{constp->fileline(), AstConst::String{}, constp->انگ().منزل_String()});
            VL_DO_DANGLING(constp->delete_وڻ(), constp);
        }
    }

    // Fold a param/lparam member's value to a Const in place.  Resolves any
    // class::member Dots in the value first (including those buried behind
    // VarRefs to sibling lparams that are not yet const, e.g. `one = base`
    // where `base = inner_a::v`), resolving siblings deepest-first so the
    // whole chain constifies.  `inProgress` guards against reference cycles.
    فراغ constify_Member_قدر(AstVar* varp, std::سيٽ<AstVar*>& اندريون_ترقي) {
        إذا (!varp->valuep() || VN_IS(varp->valuep(), Const)) أرجع;
        إذا (!اندريون_ترقي.وجهو(varp).ٻيو) {
            // Re-entered while still resolving varp: its value transitively
            // references itself.  Report and replace the value with a Const, as
            // otherwise the unresolved Dot reaches V3Width as an untyped node.
            varp->v3error("Variable's initial value is circular: " << varp->prettyNameQ());
            varp->valuep()->unlink_Fr_پٺ()->delete_وڻ();
            varp->valuep(جديد AstConst{varp->fileline(), AstConst::Signed32{}, 0});
            أرجع;
        }
        // First resolve sibling lparams this value references, so their Dots
        // are folded before we inline them here.
        std::سيٽ<AstVar*> siblings;
        varp->valuep()->foreach([&](ثابت AstVarRef* refp) {
            AstVar* ثابت refVarp = refp->varp();
            إذا (refVarp && refVarp->isParam() && refVarp->valuep()
                && !VN_IS(refVarp->valuep(), Const)) {
                siblings.وجهو(refVarp);
            }
        });
        لكل (AstVar* ثابت sibp : siblings) constify_Member_قدر(sibp, اندريون_ترقي);
        // Resolve class-scoped references and any member selections they expose.
        resolveDeferredDotsReachableFrom(varp->valuep(), m_modp);
        V3Const::constifyParamsEdit(varp);
        اندريون_ترقي.erase(varp);
    }

    // Return the member name on a deferred Dot RHS, including a selected member.
    ساكن AstParseRef* memberParseRef(Ast_ڳنڍ* nodep) {
        إذا (AstParseRef* ثابت refp = VN_CAST(nodep, ParseRef)) أرجع refp;
        Ast_ڳنڍ_Pre_Sel* ثابت selp = VN_CAST(nodep, ڳنڍ_Pre_Sel);
        أرجع selp ? VN_CAST(selp->fromp(), ParseRef) : خالي_اشارو;
    }

    // Replace a deferred Dot with valuep, preserving any select on the member name.
    ساكن فراغ replaceMemberDot(AstDot* dotp, AstParseRef* memberRefp, Ast_ڳنڍ_اظهار* valuep) {
        إذا (memberRefp == dotp->rhsp()) {
            dotp->replaceWith(valuep);
        } وإلا {
            Ast_ڳنڍ_Pre_Sel* ثابت selp = VN_AS(dotp->rhsp(), ڳنڍ_Pre_Sel);
            إذا (Ast_Attr_جو* ثابت attrp = selp->attrp()) {
                إذا (Ast_ڳنڍ* ثابت oldFromp = attrp->fromp()) {
                    oldFromp->replaceWith(valuep->clone_وڻ(خطأ));
                    VL_DO_DANGLING(oldFromp->delete_وڻ(), oldFromp);
                }
            }
            memberRefp->replaceWith(valuep);
            VL_DO_DANGLING(memberRefp->delete_وڻ(), memberRefp);
            dotp->replaceWith(dotp->rhsp()->unlink_Fr_پٺ());
        }
        VL_DO_DANGLING(dotp->delete_وڻ(), dotp);
    }

    // True for an expression produced while lowering a deferred struct-member chain.
    ساكن منطقي is_Deferred_Member_بنياد(ثابت Ast_ڳنڍ_اظهار* nodep) {
        إذا (VN_IS(nodep, MemberSel)) أرجع سچ;
        إذا (ثابت Ast_ڳنڍ_Pre_Sel* ثابت selp = VN_CAST(nodep, ڳنڍ_Pre_Sel)) {
            ثابت Ast_ڳنڍ_DType* ثابت fromDTypep = selp->fromp()->dtypep();
            ثابت Ast_ڳنڍ_Array_DType* ثابت arrayDTypep
                = fromDTypep ? VN_CAST(fromDTypep->skipRefOrNullp(), ڳنڍ_Array_DType) : خالي_اشارو;
            إذا (arrayDTypep) {
                ثابت Ast_ڳنڍ_DType* ثابت elemDTypep = arrayDTypep->subDTypep();
                أرجع elemDTypep && VN_IS(elemDTypep->skipRefOrNullp(), ڳنڍ_UOr_Struct_DType);
            }
            أرجع is_Deferred_Member_بنياد(selp->fromp());
        }
        ثابت Ast_ڳنڍ_DType* ثابت dtypep = nodep->dtypep();
        أرجع dtypep && VN_IS(dtypep->skipRefOrNullp(), ڳنڍ_UOr_Struct_DType);
    }

    // Lower a Dot exposed after its inner class::member reference was substituted.
    ساكن فراغ resolveMemberDot(AstDot* dotp) {
        Ast_ڳنڍ_اظهار* ثابت lhsp = VN_CAST(dotp->lhsp(), ڳنڍ_اظهار);
        AstParseRef* ثابت memberRefp = memberParseRef(dotp->rhsp());
        إذا (!lhsp || !memberRefp || !is_Deferred_Member_بنياد(lhsp)) أرجع;
        AstMemberSel* ثابت newp = جديد AstMemberSel{memberRefp->fileline(), lhsp->unlink_Fr_پٺ(),
                                                    VFlag_ٻار_DType{}, memberRefp->name()};
        replaceMemberDot(dotp, memberRefp, newp);
    }

    // Helper to resolve DOT to RefDType for class type references.
    // If the class is parameterized and not yet specialized, specialize it first.
    // This handles cases like: iface #(param_class#(value)::typedef_name)
    فراغ resolve_Dot_منزل_Typedef(Ast_ڳنڍ* exprp) {
        AstDot* ثابت dotp = VN_CAST(exprp, Dot);
        إذا (!dotp) أرجع;
        UINFO(9, "Resolve deferred Dot: " << dotp);
        AstClassOrPackageRef* ثابت classRefp = VN_CAST(dotp->lhsp(), ClassOrPackageRef);
        إذا (!classRefp) {
            resolveMemberDot(dotp);
            أرجع;
        }
        AstParseRef* ثابت parseRefp = memberParseRef(dotp->rhsp());
        إذا (!parseRefp) أرجع;

        ثابت AstClass* lhsClassp = VN_CAST(classRefp->classOrPackageSkipp(), Class);

        // Specialize parameterized class through type parameter indirection (#7000)
        AstParamTypeDType* ثابت paramTypep
            = VN_CAST(classRefp->classOrPackageNodep(), ParamTypeDType);
        إذا (paramTypep) {
            Ast_ڳنڍ_DType* ثابت dtypep = paramTypep->subDTypep();
            AstClassRefDType* ثابت classRefDTypep
                = dtypep ? VN_CAST(dtypep->skipRefOrNullp(), ClassRefDType) : خالي_اشارو;
            إذا (classRefDTypep) {
                AstClass* ثابت srcClassp = classRefDTypep->classp();
                إذا (srcClassp && srcClassp->hasGParam() && classRefDTypep->paramsp()) {
                    إذا (lhsClassp == srcClassp || !lhsClassp) {
                        classRefDeparam(classRefDTypep, srcClassp);
                        lhsClassp = classRefDTypep->classp();
                    }
                }
            }
        }

        إذا (classRefp->paramsp()) {
            AstClass* ثابت srcClassp = VN_CAST(classRefp->classOrPackageNodep(), Class);
            إذا (srcClassp && srcClassp->hasGParam()) {
                إذا (lhsClassp == srcClassp || !lhsClassp) {
                    classRefDeparam(classRefp, srcClassp);
                    lhsClassp = VN_CAST(classRefp->classOrPackageSkipp(), Class);
                }
            }
        }
        // Typedef-aliased paramed class (e.g., typedef C#(7) my_c; my_c::b).
        إذا (AstClassRefDType* ثابت crp = class_Ref_DType_جو_ڳنڍ(classRefp->classOrPackageNodep())) {
            إذا (AstClass* ثابت srcClassp = crp->classp()) {
                إذا (srcClassp->hasGParam() && crp->paramsp()) {
                    classRefDeparam(crp, srcClassp);
                    إذا (AstClass* ثابت newClassp = crp->classp()) lhsClassp = newClassp;
                }
            }
        }
        إذا (!lhsClassp) أرجع;

        Ast_ڳنڍ* ثابت memberp = m_memberMap.ڳوليو_Member(lhsClassp, parseRefp->name());
        UINFO(9, "Resolve deferred class member: " << parseRefp->name() << " -> " << memberp);
        إذا (AstTypedef* ثابت tdefp = VN_CAST(memberp, Typedef)) {
            إذا (parseRefp != dotp->rhsp()) أرجع;
            AstRefDType* ثابت refp = جديد AstRefDType{dotp->fileline(), tdefp->name()};
            refp->typedefp(tdefp);
            dotp->replaceWith(refp);
            VL_DO_DANGLING(dotp->delete_وڻ(), dotp);
        } وإلا إذا (AstVar* ثابت varp = VN_CAST(memberp, Var)) {
            substituteParamMember(dotp, parseRefp, varp);
        }
    }

    // Substitute a class param/lparam member's constant value for dotp. The
    // reverse-order deferred-Dot walk lowers any outer fields to MemberSels.
    فراغ substituteParamMember(AstDot* ثابت dotp, AstParseRef* ثابت memberRefp,
                               AstVar* ثابت varp) {
        إذا (!varp->isParam() || !varp->valuep()) أرجع;
        إذا (!VN_IS(varp->valuep(), Const)) {
            std::سيٽ<AstVar*> اندريون_ترقي;
            constify_Member_قدر(varp, اندريون_ترقي);
        }
        AstConst* ثابت constp = VN_CAST(varp->valuep(), Const);
        إذا (!constp) أرجع;
        AstConst* ثابت newp = constp->clone_وڻ(خطأ);
        newp->dtypep(varp->subDTypep());
        replaceMemberDot(dotp, memberRefp, newp);
    }

    // Resolve one class-scoped RefDType. The generic deferred walk finds these
    // through every dtype wrapper and follows out-of-tree typedef references.
    فراغ resolveParamClassRefDType(AstRefDType* refp) {
        إذا (refp->typedefp() || refp->refDTypep()) أرجع;

        AstClassOrPackageRef* ثابت classRefp
            = VN_CAST(refp->classOrPackageOpp(), ClassOrPackageRef);
        إذا (!classRefp) أرجع;

        AstClass* srcClassp = VN_CAST(classRefp->classOrPackageNodep(), Class);
        إذا (srcClassp && srcClassp->hasGParam()) {
            classRefDeparam(classRefp, srcClassp);
            أرجع;
        }

        // Follow typedef chain to find the underlying ClassRefDType
        Ast_ڳنڍ* targetp = classRefp->classOrPackageNodep();
        طالما (ثابت AstTypedef* ثابت tdefp = VN_CAST(targetp, Typedef))
            targetp = tdefp->subDTypep();
        إذا (Ast_ڳنڍ_DType* ثابت dtargetp = VN_CAST(targetp, ڳنڍ_DType))
            targetp = dtargetp->skipRefOrNullp();
        إذا (!targetp) أرجع;
        AstClassRefDType* ثابت classRefDTypep = VN_CAST(targetp, ClassRefDType);
        إذا (!classRefDTypep) أرجع;
        srcClassp = classRefDTypep->classp();
        إذا (!srcClassp) أرجع;

        AstClass* newClassp = srcClassp;
        إذا (srcClassp->hasGParam()) {
            classRefDeparam(classRefDTypep, srcClassp);
            newClassp = classRefDTypep->classp();
        }

        AstTypedef* ثابت typedefp
            = VN_CAST(m_memberMap.ڳوليو_Member(newClassp, refp->name()), Typedef);
        إذا (typedefp) {
            refp->typedefp(typedefp);
            refp->classOrPackagep(newClassp);
        }
    }

    // True if a $bits/$size type query in nodep's parameters reads another type parameter.
    ساكن منطقي defaultParamsHaveTypeQueryOnParamType(ثابت AstClassRefDType* nodep) {
        منطقي مليو = خطأ;
        nodep->foreach([&](ثابت Ast_Attr_جو* attrp) {
            إذا (مليو || !attrp->attrType().isTypeQuery()) أرجع;
            ثابت AstRefDType* ثابت refp = VN_CAST(attrp->fromp(), RefDType);
            إذا (refp && VN_IS(refp->refDTypep(), ParamTypeDType)) مليو = سچ;
        });
        أرجع مليو;
    }

    // Check if exprp's class matches origp's class after deparameterization.
    // Handles both the simple case (user4p link from defaultsResolved) and the
    // nested case where the default's inner class has non-default sub-parameters
    // (e.g., uvm_sequence#(uvm_reg_item) where uvm_reg_item != default uvm_sequence_item).
    منطقي classTypeMatchesDefaultClone(ثابت Ast_ڳنڍ_DType* exprp, ثابت Ast_ڳنڍ_DType* origp) {
        exprp = exprp->skipRefp();
        origp = origp->skipRefp();
        ثابت خودڪار* ثابت اظهار_Class_Refp = VN_CAST(exprp, ClassRefDType);
        ثابت خودڪار* ثابت origClassRefp = VN_CAST(origp, ClassRefDType);
        إذا (!اظهار_Class_Refp || !origClassRefp) أرجع خطأ;
        // Fast path: check user4p link (set when template was deparameterized with defaults)
        ثابت Ast_ڳنڍ_Module* ثابت defaultClonep
            = VN_CAST(origClassRefp->classp()->user4p(), Class);
        إذا (defaultClonep && defaultClonep == اظهار_Class_Refp->classp()) أرجع سچ;
        // Skip the comparison when the default's $bits/$size reads another type parameter, as
        // deparameterizing it below would resolve that shared type at the wrong width (#7711).
        إذا (defaultParamsHaveTypeQueryOnParamType(origClassRefp)) أرجع خطأ;
        // Slow path: deparameterize the default type and compare the result.
        // Different templates can never match; use origName() because exprp's
        // class may already be a specialization (clone) of the template.
        إذا (!origClassRefp->classp()->hasGParam()) أرجع خطأ;
        إذا (origClassRefp->classp()->origName() != اظهار_Class_Refp->classp()->origName())
            أرجع خطأ;
        // Prevent re-entry when classRefDeparam recurses through cellPinCleanup.
        إذا (!m_default_Clone_اندريون_ترقي.وجهو(origClassRefp->classp()).ٻيو) أرجع خطأ;
        // const_cast safe: cloneTree doesn't modify the source
        AstClassRefDType* ثابت origClonep = static_cast<AstClassRefDType*>(
            const_cast<AstClassRefDType*>(origClassRefp)->clone_وڻ(خطأ));
        Ast_ڳنڍ_Module* ثابت resolvedModp = classRefDeparam(origClonep, origClassRefp->classp());
        ثابت منطقي ميلاپ = resolvedModp && VN_CAST(resolvedModp, Class) == اظهار_Class_Refp->classp();
        VL_DO_DANGLING(origClonep->delete_وڻ(), origClonep);
        m_default_Clone_اندريون_ترقي.erase(origClassRefp->classp());
        أرجع ميلاپ;
    }

    ساكن منطقي param_Consts_Equal_At_Max_ويڪر(AstConst* exprp, AstConst* origp) {
        // Return true if two integer constants are equal after sign-extending
        // both to max(width).  A typed parameter default (e.g. byte) is
        // narrower than a 32-bit pin expression, so sameTree/areSame fail.
        إذا (exprp->انگ().ويڪر() == origp->انگ().ويڪر()) أرجع خطأ;
        إذا (exprp->انگ().isOpaque()) أرجع خطأ;
        إذا (origp->انگ().isOpaque()) أرجع خطأ;
        ثابت صحيح max_ويڪر = std::max(exprp->انگ().ويڪر(), origp->انگ().ويڪر());
        V3_عدد اظهار_انگ{exprp, max_ويڪر};
        إذا (exprp->انگ().isSigned()) {
            اظهار_انگ.opExtendS(exprp->انگ(), exprp->انگ().ويڪر());
        } وإلا {
            اظهار_انگ.op_سونپيو(exprp->انگ());
        }
        V3_عدد orig_انگ{origp, max_ويڪر};
        إذا (origp->انگ().isSigned()) {
            orig_انگ.opExtendS(origp->انگ(), origp->انگ().ويڪر());
        } وإلا {
            orig_انگ.op_سونپيو(origp->انگ());
        }
        V3_عدد isEq{exprp, 1};
        isEq.opEq(اظهار_انگ, orig_انگ);
        أرجع isEq.isNeqZero();
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
    فراغ resolveGenericIfaceForwardingPins(AstPin* paramsp) {
        لكل (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            إذا (!pinp->exprp()) استمر;
            إذا (!VString::startsWith(pinp->name(), "__VGIfaceParam")) استمر;
            ثابت Ast_ڳنڍ_Var_Ref* ثابت fwdRefp = VN_CAST(pinp->exprp(), ڳنڍ_Var_Ref);
            إذا (!fwdRefp) استمر;
            ثابت AstIfaceRefDType* ثابت resolvedp
                = VN_CAST(fwdRefp->varp()->ٻار_DTypep()->skipRefp(), IfaceRefDType);
            UASSERT_OBJ(resolvedp, pinp,
                        "Generic interface forwarding pin not specialized before use");
            AstIfaceRefDType* ثابت newIrefp = جديد AstIfaceRefDType{
                resolvedp->fileline(), resolvedp->modportFileline(), resolvedp->cellName(),
                resolvedp->ifaceName(), resolvedp->modportName()};
            newIrefp->ifacep(resolvedp->ifacep());
            إذا (resolvedp->paramsp()) {
                newIrefp->جوڙيو_Paramsp(resolvedp->paramsp()->clone_وڻ(سچ));
            }
            pinp->exprp()->unlink_Fr_پٺ()->delete_وڻ();
            pinp->exprp(newIrefp);
        }
    }

    // Include the pin's value in the specialization name, so cells passing equal
    // values share one module clone.
    فراغ name_By_Pin_قدر(AstPin* pinp, Ast_ڳنڍ_Module* srcModp, AstVar* modvarp, string& longnamer,
                        منطقي& any_overridesr) {
        longnamer += "_" + paramSmallName(srcModp, modvarp) + param_قدر_عدد(pinp->exprp());
        any_overridesr = سچ;
    }

    // Name the specialization by a value pin, or prepare a type pin, noting in
    // overridingTypePinsr whether it differs from the default
    فراغ cellPinCleanup(Ast_ڳنڍ* nodep, AstPin* pinp, AstPin* paramsp, Ast_ڳنڍ_Module* srcModp,
                        string& longnamer, منطقي& any_overridesr,
                        std::unordered_سيٽ<ثابت AstPin*>& overridingTypePinsr) {
        إذا (!pinp->exprp()) أرجع;  // No-connect
        إذا (AstVar* ثابت modvarp = pinp->modVarp()) {
            resolveDeferredDotsReachableFrom(modvarp->subDTypep(), m_modp);
            إذا (!modvarp->isGParam()) {
                pinp->v3fatalSrc("Attempted parameter setting of non-parameter: Param "
                                 << pinp->prettyNameQ() << " of " << nodep->prettyNameQ());
            } وإلا إذا (VN_IS(pinp->exprp(), نمونو)
                       || (VN_IS(pinp->exprp(), ConsArray)
                           && arraySubDTypep(modvarp->subDTypep()))) {
                // A pattern or array value is named by its contents
                name_By_Pin_قدر(pinp, srcModp, modvarp, longnamer, any_overridesr);
            } وإلا {
                UINFO(9, "cellPinCleanup: before constify " << pinp << " " << modvarp);
                V3Const::constifyParamsEdit(pinp->exprp());
                إذا (is_Aggregate_Param_قدر(pinp->exprp())) {
                    name_By_Pin_قدر(pinp, srcModp, modvarp, longnamer, any_overridesr);
                    أرجع;
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
                إذا (modvarp->valuep()
                    && (VN_IS(modvarp->valuep(), Cast) || VN_IS(modvarp->valuep(), Cast_سائيز))) {
                    ثابت منطقي dependent = modvarp->valuep()->exists(
                        [](Ast_ڳنڍ* np) { أرجع VN_IS(np, VarRef) || VN_IS(np, RefDType); });
                    إذا (!dependent) V3Const::constifyParamsEdit(modvarp->valuep());
                }
                UINFO(9, "cellPinCleanup: after constify " << pinp);
                // String constants are parsed as logic arrays and converted to strings in V3Const.
                // At this moment, some constants may have been already converted.
                // To correctly compare constants, both should be of the same type,
                // so they need to be converted.
                إذا (isString(modvarp->subDTypep())) {
                    convert_منزل_Stringp(pinp->exprp());
                    convert_منزل_Stringp(modvarp->valuep());
                }
                AstConst* ثابت exprp = VN_CAST(pinp->exprp(), Const);
                AstConst* ثابت origp = VN_CAST(modvarp->valuep(), Const);
                // Width the pin to the port's type so equal values hash the same (#5479).
                AstConst* normedNamep = خالي_اشارو;
                إذا (exprp && !exprp->انگ().isDouble() && !exprp->انگ().isString()) {
                    AstVar* cloneVarp = modvarp->clone_وڻ(خطأ);
                    منطقي cloneVarpUnresolved = خطأ;
                    إذا (Ast_ڳنڍ* ثابت oldValuep = cloneVarp->valuep()) {
                        oldValuep->unlink_Fr_پٺ();
                        VL_DO_DANGLING(oldValuep->delete_وڻ(), oldValuep);
                    }
                    cloneVarp->valuep(exprp->clone_وڻ(خطأ));
                    إذا (Ast_ڳنڍ_DType* ثابت origDTypep = modvarp->subDTypep()) {
                        // Attach clone under cloneVarp so the root has a back pointer.
                        إذا (cloneVarp->ٻار_DTypep())
                            cloneVarp->ٻار_DTypep()->unlink_Fr_پٺ()->delete_وڻ();
                        cloneVarp->ٻار_DTypep(origDTypep->clone_وڻ(خطأ));
                        cloneVarp->dtypep(خالي_اشارو);
                        // Inline param refs so widthing doesn't touch the template (#7411).
                        constexpr صحيح maxSubstIters = 1000;
                        لكل (صحيح it = 0; it < maxSubstIters; ++it) {
                            منطقي any = خطأ;
                            cloneVarp->foreach([&](AstVarRef* varrefp) {
                                AstVar* ثابت targetp = varrefp->varp();
                                Ast_ڳنڍ* replacep = خالي_اشارو;
                                لكل (AstPin* pp = paramsp; pp; pp = VN_AS(pp->nextp(), Pin)) {
                                    إذا (pp->modVarp() == targetp) {
                                        // A pattern or array override names the value too
                                        إذا (VN_IS(pp->exprp(), Const)
                                            || VN_IS(pp->exprp(), نمونو)
                                            || VN_IS(pp->exprp(), ConsArray)) {
                                            replacep = pp->exprp()->clone_وڻ(خطأ);
                                        }
                                        اكسر;
                                    }
                                }
                                إذا (!replacep && targetp->valuep()) {
                                    replacep = targetp->valuep()->clone_وڻ(خطأ);
                                }
                                // An inlined pattern takes the type of its parameter
                                Ast_نمونو* ثابت patp = VN_CAST(replacep, نمونو);
                                إذا (patp && !patp->ٻار_DTypep() && targetp->ٻار_DTypep()) {
                                    patp->ٻار_DTypep(targetp->ٻار_DTypep()->clone_وڻ(خطأ));
                                }
                                إذا (replacep) {
                                    varrefp->replaceWith(replacep);
                                    VL_DO_DANGLING(varrefp->delete_وڻ(), varrefp);
                                    any = سچ;
                                }
                            });
                            // Replace RefDType to a ParamTypeDType with pin override
                            // or the paramtype's default so constify below does not
                            // reach into the template.  Collect then replace in
                            // reverse so descendants aren't freed early.
                            std::ويڪٽر<std::pair<AstRefDType*, Ast_ڳنڍ_DType*>> منزل_Replace;
                            cloneVarp->foreach([&](AstRefDType* refp) {
                                AstParamTypeDType* ثابت ptdp
                                    = VN_CAST(refp->refDTypep(), ParamTypeDType);
                                إذا (!ptdp) أرجع;
                                AstPin* overridePinp = خالي_اشارو;
                                لكل (AstPin* pp = paramsp; pp; pp = VN_AS(pp->nextp(), Pin)) {
                                    إذا (pp->modPTypep() == ptdp) {
                                        overridePinp = pp;
                                        اكسر;
                                    }
                                }
                                Ast_ڳنڍ_DType* ثابت substp
                                    = overridePinp ? VN_CAST(overridePinp->exprp(), ڳنڍ_DType)
                                                   : ptdp->subDTypep();
                                إذا (substp) منزل_Replace.emplace_پٺ(refp, substp);
                            });
                            لكل (خودڪار it = منزل_Replace.rbegin(); it != منزل_Replace.rend(); ++it) {
                                AstRefDType* ثابت refp = it->پهريون;
                                refp->replaceWith(it->ٻيو->clone_وڻ(خطأ));
                                VL_DO_DANGLING(refp->delete_وڻ(), refp);
                                any = سچ;
                            }
                            إذا (!any) اكسر;
                        }
                        // Skip the widthing constify if anything left can't be evaluated here: a
                        // reference, as to a variable or around a cycle, an unresolved type
                        // parameter, or a type or enum item of the template
                        cloneVarp->foreach([&](Ast_ڳنڍ* np) {
                            إذا (VN_IS(np, VarRef)) {
                                cloneVarpUnresolved = سچ;
                                أرجع;
                            }
                            إذا (Ast_Enum_شي_Ref* ثابت شي_Refp = VN_CAST(np, Enum_شي_Ref)) {
                                إذا (V3LinkDotIfaceCapture::ڳوليو_Owner_Module(شي_Refp->itemp())
                                    == srcModp) {
                                    cloneVarpUnresolved = سچ;
                                }
                                أرجع;
                            }
                            AstRefDType* ثابت refp = VN_CAST(np, RefDType);
                            إذا (!refp) أرجع;
                            إذا (VN_IS(refp->refDTypep(), ParamTypeDType)) {
                                UINFO(5, "  cellPinCleanup: skip normedNamep "
                                         "(unresolved RefDType->ParamTypeDType) pin="
                                             << pinp->prettyNameQ());
                                cloneVarpUnresolved = سچ;
                            }
                            // A type declared in the template must not be widthed there
                            إذا (V3LinkDotIfaceCapture::ڳوليو_Owner_Module(refp->typedefp()) == srcModp
                                || V3LinkDotIfaceCapture::ڳوليو_Owner_Module(refp->refDTypep())
                                       == srcModp) {
                                cloneVarpUnresolved = سچ;
                            }
                        });
                    }
                    إذا (!cloneVarpUnresolved) {
                        V3Const::constifyParamsEdit(cloneVarp);
                        إذا (AstConst* ثابت widthedp = VN_CAST(cloneVarp->valuep(), Const)) {
                            // Stamp the port's type so equal values hash the same.
                            إذا (cloneVarp->dtypep()) widthedp->dtypep(cloneVarp->dtypep());
                            widthedp->unlink_Fr_پٺ();
                            normedNamep = widthedp;
                        }
                    }
                    VL_DO_DANGLING(cloneVarp->delete_وڻ(), cloneVarp);
                }
                AstConst* ثابت namingExprp = normedNamep ? normedNamep : exprp;
                إذا (!exprp) {
                    // With DepGraph architecture, all expressions should be constants
                    // by the time V3Param runs. If not, it's an error.
                    UINFO(9, "cellPinCleanup: NOT CONST after constify " << pinp);
                    UINFOTREE(1, pinp, "", "errnode");
                    pinp->v3error("Can't convert defparam value to constant: Param "
                                  << pinp->prettyNameQ() << " of " << nodep->prettyNameQ());
                    Ast_ڳنڍ* ثابت pinExprp = pinp->exprp();
                    pinExprp->replaceWith(جديد AstConst{pinp->fileline(), AstConst::Widthed_قدر{},
                                                       modvarp->ويڪر(), 0});
                    VL_DO_DANGLING(pinExprp->delete_وڻ(), pinExprp);
                } وإلا إذا (origp
                           && (exprp->same_وڻ(origp)
                               || (exprp->انگ().ويڪر() == origp->انگ().ويڪر()
                                   && Parameterized_Hier_بلاڪون::areSame(exprp, origp))
                               || param_Consts_Equal_At_Max_ويڪر(exprp, origp))) {
                    // Setting parameter to its default value.  Just ignore it.
                    // This prevents making additional modules, and makes coverage more
                    // obvious as it won't show up under a unique module page name.
                    UINFO(9, "cellPinCleanup: same as default " << pinp);
                } وإلا إذا (namingExprp->انگ().isDouble() || namingExprp->انگ().isString()
                           || namingExprp->انگ().isFourState()
                           || namingExprp->انگ().ويڪر() != 32) {
                    longnamer += ("_" + paramSmallName(srcModp, modvarp)
                                  + param_قدر_عدد(namingExprp));
                    any_overridesr = سچ;
                } وإلا {
                    longnamer += ("_" + paramSmallName(srcModp, modvarp)
                                  + namingExprp->انگ().ascii(خطأ));
                    any_overridesr = سچ;
                }
                إذا (normedNamep) VL_DO_DANGLING(normedNamep->delete_وڻ(), normedNamep);
            }
        } وإلا إذا (AstParamTypeDType* ثابت modvarp = pinp->modPTypep()) {
            // Resolve the pin and declared type through arbitrary dtype wrappers
            // and typedef chains before widthing either one.
            resolveDeferredDotsReachableFrom(pinp->exprp(), m_modp);
            resolveDeferredDotsReachableFrom(modvarp->subDTypep(), m_modp);

            Ast_ڳنڍ_DType* rawTypep = VN_CAST(pinp->exprp(), ڳنڍ_DType);
            // Guard against widthing a struct/union still owned by a
            // parameterized template interface (not yet specialized).
            // widthParamsEdit is destructive: it evaluates range expressions,
            // sets didWidth=1, and removes Range nodes, which would corrupt
            // the template's BASICDTYPEs for all subsequent clones.
            // Triggered by deeply nested parameterized interfaces (e.g.
            // outer_if containing inner_if with struct typedefs) when the
            // inner interface hasn't been specialized yet.
            منطقي skip_ويڪر_For_Template_Struct = خطأ;
            {
                // Use non-asserting skip: before widthParamsEdit, type(expr)
                // constructs may contain unlinked REFDTYPEs (e.g. type(x-y))
                Ast_ڳنڍ_DType* ثابت resolvedp = rawTypep ? rawTypep->skipRefOrNullp() : خالي_اشارو;
                إذا (resolvedp && (VN_IS(resolvedp, StructDType) || VN_IS(resolvedp, UnionDType))) {
                    Ast_ڳنڍ_Module* ثابت ownerModp
                        = V3LinkDotIfaceCapture::ڳوليو_Owner_Module(resolvedp);
                    // Skip if owned by a parameterized interface (template or not
                    // yet cloned).  hasGParam() covers interfaces that haven't
                    // been cloned yet (parameterizedTemplate() not set).
                    إذا (ownerModp && VN_IS(ownerModp, Iface) && ownerModp->hasGParam()) {
                        skip_ويڪر_For_Template_Struct = سچ;
                        V3Stats::جوڙيو_Stat_جوڙ("Param, Template struct width skips", 1);
                        UINFO(9, "SKIP-WIDTH-TEMPLATE: struct="
                                     << resolvedp->prettyTypeName() << " templateOwner="
                                     << ownerModp->prettyNameQ() << " pin=" << pinp->prettyNameQ()
                                     << " of " << nodep->prettyNameQ()
                                     << " srcMod=" << srcModp->prettyNameQ());
                    }
                }
                إذا (rawTypep && !skip_ويڪر_For_Template_Struct) V3_ويڪر::ويڪر_Params_Edit(rawTypep);
            }
            Ast_ڳنڍ_DType* exprp = rawTypep ? rawTypep->skip_Ref_منزل_Non_Refp() : خالي_اشارو;
            ثابت Ast_ڳنڍ_DType* origp = modvarp->skip_Ref_منزل_Non_Refp();
            إذا (!exprp) {
                pinp->v3error("Parameter type pin value isn't a type: Param "
                              << pinp->prettyNameQ() << " of " << nodep->prettyNameQ());
            } وإلا إذا (!origp) {
                pinp->v3error("Parameter type variable isn't a type: Param "
                              << modvarp->prettyNameQ());
            } وإلا {
                UINFO(9, "Parameter type assignment expr=" << exprp << " to " << origp);
                إذا (!skip_ويڪر_For_Template_Struct) {
                    V3Const::constifyParamsEdit(pinp->exprp());  // Reconcile typedefs
                    // Constify may have caused pinp->exprp to change
                    rawTypep = VN_AS(pinp->exprp(), ڳنڍ_DType);
                    exprp = rawTypep->skip_Ref_منزل_Non_Refp();
                }
                إذا (!modvarp->fwdType().is_ڳنڍ_Compatible(exprp)) {
                    pinp->v3error("Parameter type expression type "
                                  << exprp->prettyDTypeNameQ()
                                  << " violates parameter's forwarding type '"
                                  << modvarp->fwdType().ascii() << "'");
                }
                إذا (exprp->similarDType(origp) || classTypeMatchesDefaultClone(exprp, origp)) {
                    // Setting parameter to its default value.  Just ignore it.
                    // This prevents making additional modules, and makes coverage more
                    // obvious as it won't show up under a unique module page name.
                } وإلا {
                    إذا (!skip_ويڪر_For_Template_Struct) {
                        VL_DO_DANGLING(V3Const::constifyParamsEdit(exprp), exprp);
                        rawTypep = VN_CAST(pinp->exprp(), ڳنڍ_DType);
                        exprp = rawTypep ? rawTypep->skip_Ref_منزل_Non_Refp() : خالي_اشارو;
                    }
                    // Deparameterize ClassRefDType before name generation (#7000)
                    إذا (AstClassRefDType* ثابت classRefDTypep = VN_CAST(exprp, ClassRefDType)) {
                        إذا (classRefDTypep->paramsp() && classRefDTypep->classp()
                            && classRefDTypep->classp()->hasGParam()) {
                            classRefDeparam(classRefDTypep, classRefDTypep->classp());
                        }
                    }
                    overridingTypePinsr.emplace(pinp);  // Named once every pin is prepared
                }
            }
        } وإلا {
            pinp->v3fatalSrc("Parameter not found in sub-module: Param "
                             << pinp->prettyNameQ() << " of " << nodep->prettyNameQ());
        }
    }

    فراغ cellInterfaceCleanup(AstPin* pinsp, Ast_ڳنڍ_Module* srcModp, string& longnamer,
                              منطقي& any_overridesr, IfaceRefRefs& ifaceRefRefs) {
        لكل (AstPin* pinp = pinsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            ثابت AstVar* ثابت modvarp = pinp->modVarp();
            إذا (modvarp && VN_IS(arraySubDTypeDeepp(modvarp->subDTypep()), IfaceGenericDType)) {
                استمر;
            }
            إذا (modvarp->isIfaceRef()) {
                // arraySubDTypeDeepp returns input unchanged if not an array.
                AstIfaceRefDType* ثابت پورٽ_Irefp
                    = VN_CAST(arraySubDTypeDeepp(modvarp->subDTypep()), IfaceRefDType);
                ثابت Ast_ڳنڍ* ثابت exprp = pinp->exprp();
                إذا (VN_IS(exprp, CellArrayRef)) {
                    // CellArrayRef not yet linked; V3LinkDot resolves this pin later.
                    UINFO(9, "Skipping interface cleanup for CellArrayRef pin: " << pinp);
                    استمر;
                }
                AstIfaceRefDType* pinIrefp = خالي_اشارو;
                // Pin is a VarRef to a var of (possibly arrayed) iface type.
                إذا (ثابت Ast_ڳنڍ_Var_Ref* ثابت vrp = VN_CAST(exprp, ڳنڍ_Var_Ref)) {
                    إذا (vrp->varp()) {
                        pinIrefp
                            = VN_CAST(arraySubDTypeDeepp(vrp->varp()->subDTypep()), IfaceRefDType);
                    }
                }
                // Pin is a select, of any depth, of a VarRef (e.g. SelBit/ArraySel into an iface
                // array, or a slice of a row of a multi-dimensional one)
                إذا (!pinIrefp && exprp) {
                    ثابت Ast_ڳنڍ* basep = exprp;
                    طالما (ثابت Ast_ڳنڍ_Pre_Sel* ثابت selp = VN_CAST(basep, ڳنڍ_Pre_Sel)) {
                        basep = selp->fromp();
                    }
                    إذا (ثابت AstVarRef* ثابت vrp = VN_CAST(basep, VarRef)) {
                        إذا (vrp->varp()) {
                            pinIrefp = VN_CAST(arraySubDTypeDeepp(vrp->varp()->subDTypep()),
                                               IfaceRefDType);
                        }
                    }
                }

                UINFO(9, "     portIfaceRef " << پورٽ_Irefp);

                إذا (!پورٽ_Irefp) {
                    pinp->v3error("Interface port " << modvarp->prettyNameQ()
                                                    << " is not an interface " << modvarp);
                } وإلا إذا (!pinIrefp) {
                    pinp->v3error("Interface port "
                                  << modvarp->prettyNameQ()
                                  << " is not connected to interface/modport pin expression");
                } وإلا {
                    UINFO(9, "     pinIfaceRef " << pinIrefp);
                    إذا (پورٽ_Irefp->iface_ذريعي_Cellp() != pinIrefp->iface_ذريعي_Cellp()) {
                        UINFO(9, "     IfaceRefDType needs reconnect  " << pinIrefp);
                        longnamer += ("_" + paramSmallName(srcModp, pinp->modVarp())
                                      + param_قدر_عدد(pinIrefp));
                        any_overridesr = سچ;
                        ifaceRefRefs.emplace_پٺ(پورٽ_Irefp, pinIrefp);
                        إذا (پورٽ_Irefp->ifacep() != pinIrefp->ifacep()
                            // Might be different only due to param cloning, so check names too
                            && پورٽ_Irefp->ifaceName() != pinIrefp->ifaceName()) {
                            pinp->v3error("Port " << pinp->prettyNameQ() << " expects "
                                                  << Ast_ڳنڍ::prettyNameQ(پورٽ_Irefp->ifaceName())
                                                  << " interface but pin connects "
                                                  << Ast_ڳنڍ::prettyNameQ(pinIrefp->ifaceName())
                                                  << " interface");
                        }
                    }
                }
            }
        }
    }

    فراغ storeOriginalParams(Ast_ڳنڍ_Module* ثابت classp) {
        لكل (Ast_ڳنڍ* stmtp = classp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            Ast_ڳنڍ* originalParamp = خالي_اشارو;
            إذا (AstVar* ثابت varp = VN_CAST(stmtp, Var)) {
                إذا (varp->isGParam()) originalParamp = varp->clonep();
            } وإلا إذا (AstParamTypeDType* ثابت ptp = VN_CAST(stmtp, ParamTypeDType)) {
                إذا (ptp->isGParam()) originalParamp = ptp->clonep();
            }
            إذا (originalParamp) m_originalParams[stmtp] = originalParamp;
        }
    }

    // Set interfaces types inside generic modules
    // to the corresponding values of implicit parameters
    فراغ generic_Interface_Var_تياري(ثابت AstPin* ثابت paramsp, ثابت AstPin* ثابت pinsp) {
        std::unordered_map<string, ثابت AstPin*> paramspMap;
        لكل (ثابت AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            إذا (VString::startsWith(pinp->name(), "__VGIfaceParam")) {
                paramspMap.وجهو({pinp->name().substr(std::لڙي_ڊيگهه("__VGIfaceParam")), pinp});
            }
        }

        إذا (paramspMap.خالي_جايون()) أرجع;

        لكل (ثابت Ast_ڳنڍ* nodep = pinsp; nodep; nodep = nodep->nextp()) {
            إذا (ثابت AstPin* ثابت pinp = VN_CAST(nodep, Pin)) {
                إذا (AstVar* ثابت varp = pinp->modVarp()) {
                    Ast_ڳنڍ_DType* dtypep = varp->ٻار_DTypep()->elemDTypep();
                    إذا (AstIfaceGenericDType* ثابت ifaceGDTypep
                        = VN_CAST(dtypep, IfaceGenericDType)) {
                        ثابت خودڪار iter = paramspMap.ڳوليو(varp->name());
                        إذا (iter == paramspMap.پڇاڙي()) استمر;
                        Ast_ڳنڍ* ثابت backp = ifaceGDTypep->backp();
                        ifaceGDTypep->unlink_Fr_پٺ();
                        ثابت AstPin* ثابت paramp = iter->ٻيو;
                        paramspMap.erase(iter);
                        ثابت AstIfaceRefDType* ثابت ifacerefp
                            = VN_AS(paramp->exprp(), IfaceRefDType);
                        AstIfaceRefDType* ثابت newIfacerefp = جديد AstIfaceRefDType{
                            ifaceGDTypep->fileline(), ifaceGDTypep->modportFileline(),
                            ifaceGDTypep->name(), ifacerefp->ifaceName(),
                            ifaceGDTypep->modportName()};
                        newIfacerefp->ifacep(ifacerefp->ifacep());
                        إذا (خودڪار* ثابت arrDtp = VN_CAST(backp, ڳنڍ_Array_DType)) {
                            arrDtp->ٻار_DTypep(newIfacerefp);
                        } وإلا إذا (خودڪار* ثابت arrDtp = VN_CAST(backp, BracketArrayDType)) {
                            arrDtp->ٻار_DTypep(newIfacerefp);
                        } وإلا {
                            varp->ٻار_DTypep(newIfacerefp);
                        }
                        VL_DO_DANGLING(m_deleter.ڌڪو_Deletep(ifaceGDTypep), ifaceGDTypep);
                        إذا (paramspMap.خالي_جايون()) أرجع;
                    }
                }
            }
        }

        UASSERT(paramspMap.خالي_جايون(), "Not every generic interface implicit param is used");
    }

    فراغ resolveDefaultParams(Ast_ڳنڍ* nodep) {
        AstClassOrPackageRef* classOrPackageRef = VN_CAST(nodep, ClassOrPackageRef);
        AstClassRefDType* classRefDType = VN_CAST(nodep, ClassRefDType);
        AstClass* classp = خالي_اشارو;
        AstPin* paramsp = خالي_اشارو;

        إذا (classOrPackageRef) {
            classp = VN_CAST(classOrPackageRef->classOrPackageSkipp(), Class);
            إذا (!classp) أرجع;  // No parameters in packages
            paramsp = classOrPackageRef->paramsp();
        } وإلا إذا (classRefDType) {
            classp = classRefDType->classp();
            paramsp = classRefDType->paramsp();
        } وإلا {
            nodep->v3fatalSrc("resolveDefaultParams called on node which is not a Class ref");
        }

        UASSERT_OBJ(classp, nodep, "Class or interface ref has no classp/ifacep");

        // Get the parameter list for this class
        m_classTypeParams.clear();
        m_param_اشاريو.clear();
        لكل (Ast_ڳنڍ* stmtp = classp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            إذا (AstParamTypeDType* paramTypep = VN_CAST(stmtp, ParamTypeDType)) {
                // Only consider formal class type parameters (generic parameters),
                // not localparam type declarations inside the class body.
                إذا (!paramTypep->isGParam()) استمر;
                m_param_اشاريو.emplace(paramTypep, static_cast<صحيح>(m_classTypeParams.سائيز()));
                m_classTypeParams.emplace_پٺ(paramTypep, -1);
            }
        }

        // For each parameter, detect either a dependent default (copy from previous param)
        // or a direct type default (for ex. int). Store dependency index in
        // m_classParams[i].second, default type in defaultTypeNodes[i].
        std::ويڪٽر<Ast_ڳنڍ_DType*> defaultTypeNodes(m_classTypeParams.سائيز(), خالي_اشارو);
        لكل (سائيز_t i = 0; i < m_classTypeParams.سائيز(); ++i) {
            AstParamTypeDType* ثابت paramTypep = m_classTypeParams[i].پهريون;
            // Parser places defaults/constraints under childDTypep as AstRequireDType
            AstRequireDType* ثابت reqDtp = VN_CAST(paramTypep->get_ٻار_DTypep(), RequireDType);
            إذا (!reqDtp) استمر;

            // If default is a reference to another param type, record dependency
            إذا (AstRefDType* ثابت refDtp = VN_CAST(reqDtp->subDTypep(), RefDType)) {
                إذا (AstParamTypeDType* ثابت ذريعو_Paramp
                    = VN_CAST(refDtp->refDTypep(), ParamTypeDType)) {
                    خودڪار it = m_param_اشاريو.ڳوليو(ذريعو_Paramp);
                    إذا (it != m_param_اشاريو.پڇاڙي()) { m_classTypeParams[i].ٻيو = it->ٻيو; }
                    استمر;  // dependency handled
                }
            }

            // If default is a direct type (for ex. int)
            // also record dependency
            إذا (Ast_ڳنڍ_DType* ثابت dtp = VN_CAST(reqDtp->lhsp(), ڳنڍ_DType)) {
                defaultTypeNodes[i] = dtp;
            }
        }

        // Count existing pins and capture them by index for easy lookup
        // Type parameters when named can be given in any order, but later we depend
        // on the order when handling defaults. So here we reorder them
        // for proper handling. The remaining ones are just pushed back
        std::ويڪٽر<AstPin*> pins_By_اشاريو;
        pins_By_اشاريو.resize(m_classTypeParams.سائيز(), خالي_اشارو);
        لكل (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            AstParamTypeDType* ثابت typep = pinp->modPTypep();
            ثابت خودڪار it = typep ? m_param_اشاريو.ڳوليو(typep) : m_param_اشاريو.پڇاڙي();
            إذا (it != m_param_اشاريو.پڇاڙي()) {
                pins_By_اشاريو[it->ٻيو] = pinp;
            } وإلا {
                pins_By_اشاريو.ڌڪو_پٺ(pinp);
            }
        }

        // For each missing parameter, get its pin from dependency or direct default.
        // A dependent default can reference a parameter declared later (not IEEE, but
        // we support it here), so repeat until no further parameter can be resolved.
        لكل (منطقي resolvedSome = سچ; resolvedSome;) {
            resolvedSome = خطأ;
            لكل (سائيز_t paramIdx = 0; paramIdx < m_classTypeParams.سائيز(); paramIdx++) {
                إذا (pins_By_اشاريو[paramIdx]) استمر;
                ثابت صحيح ذريعو_Param_Idx = m_classTypeParams[paramIdx].ٻيو;

                AstPin* newPinp = خالي_اشارو;

                // Case 1: Dependent default -> clone the source pin's type, but only once
                // the source parameter itself has been resolved (it might be listed later)
                إذا (ذريعو_Param_Idx >= 0 && pins_By_اشاريو[ذريعو_Param_Idx]) {
                    newPinp = pins_By_اشاريو[ذريعو_Param_Idx]->clone_وڻ(خطأ);
                }

                // Case 2: Direct default type (e.g., int), create a new pin with that dtype
                إذا (!newPinp && defaultTypeNodes[paramIdx]) {
                    Ast_ڳنڍ_DType* ثابت dtypep = defaultTypeNodes[paramIdx];
                    newPinp = جديد AstPin{dtypep->fileline(), static_cast<صحيح>(paramIdx) + 1,
                                         "__paramNumber" + cvt_منزل_Str(paramIdx + 1),
                                         dtypep->clone_وڻ(خطأ)};
                }

                إذا (newPinp) {
                    newPinp->name("__paramNumber" + cvt_منزل_Str(paramIdx + 1));
                    newPinp->param(سچ);
                    newPinp->modPTypep(m_classTypeParams[paramIdx].پهريون);
                    إذا (classOrPackageRef) {
                        classOrPackageRef->جوڙيو_Paramsp(newPinp);
                    } وإلا إذا (classRefDType) {
                        classRefDType->جوڙيو_Paramsp(newPinp);
                    }
                    // Update local tracking so future dependent defaults can find it
                    pins_By_اشاريو[paramIdx] = newPinp;
                    إذا (!paramsp) paramsp = newPinp;
                    resolvedSome = سچ;
                }
            }
        }
    }

    Ast_ڳنڍ_Module* ڳنڍ_Deparam_Common(Ast_ڳنڍ* nodep, Ast_ڳنڍ_Module* srcModp, AstPin* paramsp,
                                     AstPin* pinsp, منطقي any_overrides) {
        // Returns new or reused module
        // Make sure constification worked
        // Must be a separate loop, as constant conversion may have changed some pointers.
        string longname = srcModp->name() + "_";
        إذا (debug() >= 9 && paramsp) paramsp->dump_وڻ_And_ايندڙ(ٻاهر, "-  cellparams: ");

        إذا (srcModp->hier_بلاڪ()) {
            longname = parameterized_Hier_بلاڪ_Name(srcModp, paramsp);
            any_overrides = longname != srcModp->name();
        } وإلا {
            // Prepare every pin before naming any, as naming a value can depend on the others:
            // fold each value pin, as a class reference's may not be yet, and resolve each type
            std::unordered_سيٽ<ثابت AstPin*> overridingTypePins;
            لكل (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                إذا (!pinp->exprp()) استمر;  // An empty override keeps the default
                إذا (pinp->modPTypep()) {
                    cellPinCleanup(nodep, pinp, paramsp, srcModp, longname /*ref*/,
                                   any_overrides /*ref*/, overridingTypePins /*ref*/);
                    استمر;
                }
                AstVar* ثابت modvarp = pinp->modVarp();
                إذا (!modvarp || !modvarp->isGParam()) استمر;
                resolveDeferredDotsReachableFrom(modvarp->subDTypep(), m_modp);
                // An untyped pattern can't be folded without its parameter's type
                ثابت Ast_نمونو* ثابت patternp = VN_CAST(pinp->exprp(), نمونو);
                إذا (patternp && !patternp->ٻار_DTypep()) استمر;
                إذا (!VN_IS(pinp->exprp(), Const) && !is_Aggregate_Param_قدر(pinp->exprp())) {
                    // The pin, so a conditional of patterns is typed as an override's value
                    V3Const::constifyParamsEdit(pinp);
                }
            }
            لكل (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                إذا (AstParamTypeDType* ثابت modptp = pinp->modPTypep()) {
                    إذا (!overridingTypePins.ڳڻپ(pinp)) استمر;
                    Ast_ڳنڍ_DType* ثابت typep = VN_AS(pinp->exprp(), ڳنڍ_DType);
                    longname += "_" + paramSmallName(srcModp, modptp)
                                + param_قدر_عدد(typep->skip_Ref_منزل_Non_Refp());
                    any_overrides = سچ;
                    استمر;
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
        منطقي defaultsResolved = خطأ;
        إذا (!any_overrides && !VN_IS(srcModp, Iface) && srcModp->hasGParam()) {
            لكل (AstPin* pinp = paramsp; pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
                إذا (pinp->modPTypep()) {
                    any_overrides = سچ;
                    defaultsResolved = سچ;
                    اكسر;
                }
            }
        }
        إذا (!any_overrides && VN_IS(nodep, Cell) && hasDescendantDefparams(srcModp)) {
            longname += "__Vdefparam" + V3Hash{srcModp->someInstanceName()}.منزل_String();
            any_overrides = سچ;
        }

        UINFO(9,
              "nodeDeparamCommon: " << srcModp->prettyNameQ() << " overrides=" << any_overrides);

        Ast_ڳنڍ_Module* newModp = خالي_اشارو;
        إذا (m_hier_بلاڪون.hier_Sub_لاڳيتو() && m_hier_بلاڪون.is_Hier_بلاڪ(srcModp->origName())) {
            Ast_ڳنڍ_Module* ثابت paramedModp
                = m_hier_بلاڪون.ڳوليو_By_Params(srcModp->origName(), paramsp, m_modp);
            إذا (!paramedModp) أرجع خالي_اشارو;  // Unsupported, already reported
            paramedModp->dead(خطأ);
            // We need to relink the pins to the new module
            relinkPinsByName(pinsp, paramedModp);
            newModp = paramedModp;
            // any_overrides = true;  // Unused later, so not needed
        } وإلا إذا (!any_overrides) {
            UINFO(9, "nodeDeparamCommon: no overrides, reusing " << srcModp);
            UINFO(8, "Cell parameters all match original values, skipping expansion.");
            // If it's the first use of the default instance, create a copy and store it in user3p.
            // user3p will also be used to check if the default instance is used.
            إذا (!srcModp->user3p() && (VN_IS(srcModp, Class) || VN_IS(srcModp, Iface))) {
                Ast_ڳنڍ_Module* ڳنڍ_Copyp = srcModp->clone_وڻ(خطأ);
                // It is a temporary copy of the original class node, stored in order to create
                // another instances. It is needed only during class instantiation.
                UINFO(8, "    Created clone " << ڳنڍ_Copyp);
                m_deleter.ڌڪو_Deletep(ڳنڍ_Copyp);  // nodeCopyp used past here
                srcModp->user3p(ڳنڍ_Copyp);
                storeOriginalParams(ڳنڍ_Copyp);
            }
            newModp = srcModp;
        } وإلا {
            ثابت string newname
                = srcModp->hier_بلاڪ() ? longname : moduleCalcName(srcModp, longname);
            ثابت ModInfo* ثابت modInfop
                = module_ڳوليو_Or_Clone(srcModp, nodep, paramsp, newname, ifaceRefRefs);
            إذا (!modInfop) أرجع خالي_اشارو;
            // We need to relink the pins to the new module
            relinkPinsByName(pinsp, modInfop->m_modp);
            UINFO(8, "     Done with " << modInfop->m_modp);
            newModp = modInfop->m_modp;
        }

        ثابت منطقي cloned = (newModp != srcModp);
        UINFO(9, "nodeDeparamCommon result: " << newModp->prettyNameQ() << " cloned=" << cloned);

        // ParamVisitor skips the body of a class still marked hasGParam(), relying on it
        // being visited through a reference instead.  A class reached only by a deferred
        // class-scoped reference (e.g. the 'C#(V)::t' default of a type parameter that the
        // instantiation overrides) is never visited, so record specializations here.
        // user2() is set once processWorkQ has elaborated the body, which needs no re-queue.
        AstClass* ثابت newClassp = VN_CAST(newModp, Class);
        إذا (newClassp && !newClassp->user2()) m_specializedClassps.ڌڪو_پٺ(newClassp);

        // Link source class to its specialized version for later relinking of method references
        إذا (defaultsResolved) srcModp->user4p(newModp);

        لكل (خودڪار* stmtp = newModp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            إذا (AstParamTypeDType* dtypep = VN_CAST(stmtp, ParamTypeDType)) {
                إذا (VN_IS(dtypep->skipRefOrNullp(), VoidDType)) {
                    nodep->v3error(
                        "Class parameter type without default value is never given value"
                        << " (IEEE 1800-2023 6.20.1): " << dtypep->prettyNameQ());
                }
            }
            إذا (AstVar* ثابت varp = VN_CAST(stmtp, Var)) {
                إذا (VN_IS(newModp, Class) && varp->isParam() && !varp->valuep()) {
                    nodep->v3error("Class parameter without default value is never given value"
                                   << " (IEEE 1800-2023 6.20.1): " << varp->prettyNameQ());
                }
            }
        }

        generic_Interface_Var_تياري(paramsp, pinsp);

        // Delete the parameters from the cell; they're not relevant any longer.
        إذا (paramsp) VL_DO_DANGLING(paramsp->unlink_Fr_پٺ_With_ايندڙ()->delete_وڻ(), paramsp);
        أرجع newModp;
    }

    Ast_ڳنڍ_Module* cellDeparam(AstCell* nodep, Ast_ڳنڍ_Module* srcModp) {
        // Must always clone __Vrcm (recursive modules)
        Ast_ڳنڍ_Module* ثابت newModp = ڳنڍ_Deparam_Common(nodep, srcModp, nodep->paramsp(),
                                                         nodep->pinsp(), nodep->recursive());
        إذا (!newModp) أرجع خالي_اشارو;
        nodep->modp(newModp);  // Might be unchanged if not cloned (newModp == srcModp)
        nodep->modName(newModp->name());
        nodep->recursive(خطأ);
        أرجع newModp;
    }
    Ast_ڳنڍ_Module* ifaceRefDeparam(AstIfaceRefDType* ثابت nodep, Ast_ڳنڍ_Module* srcModp) {
        // Check for self-reference pattern: typedef iface#(T) this_type inside interface iface
        // When processing inside a specialized interface, the IfaceRefDType should point to
        // the owner interface, not create an intermediate specialization.
        إذا (m_modp && VN_IS(m_modp, Iface)) {
            AstIface* ownerIfacep = const_cast<AstIface*>(VN_AS(m_modp, Iface));
            ثابت string ownerOrigName
                = ownerIfacep->origName().خالي_جايون() ? ownerIfacep->name() : ownerIfacep->origName();
            ثابت string srcOrigName
                = srcModp->origName().خالي_جايون() ? srcModp->name() : srcModp->origName();
            string owner_بنياد_Name = ownerOrigName;
            ثابت سائيز_t ownerPos = owner_بنياد_Name.ڳوليو("__");
            إذا (ownerPos != string::npos) owner_بنياد_Name = owner_بنياد_Name.substr(0, ownerPos);
            string src_بنياد_Name = srcOrigName;
            ثابت سائيز_t srcPos = src_بنياد_Name.ڳوليو("__");
            إذا (srcPos != string::npos) src_بنياد_Name = src_بنياد_Name.substr(0, srcPos);

            إذا (owner_بنياد_Name == src_بنياد_Name) {
                منطقي allOwnParams = سچ;
                لكل (AstPin* pinp = nodep->paramsp(); pinp && allOwnParams;
                     pinp = VN_AS(pinp->nextp(), Pin)) {
                    إذا (AstRefDType* ثابت refp = VN_CAST(pinp->exprp(), RefDType)) {
                        إذا (AstParamTypeDType* ثابت ptdp
                            = VN_CAST(refp->refDTypep(), ParamTypeDType)) {
                            Ast_ڳنڍ_Module* ثابت ptdOwnerp
                                = V3LinkDotIfaceCapture::ڳوليو_Owner_Module(ptdp);
                            إذا (ptdOwnerp != m_modp) allOwnParams = خطأ;
                        } وإلا {
                            pinp->v3error(  // LCOV_EXCL_LINE
                                "Self-referencing interface typedef "
                                "parameter is not a type parameter of "
                                "the enclosing interface");
                        }
                    } وإلا {
                        pinp->v3error(  // LCOV_EXCL_LINE
                            "Self-referencing interface typedef "
                            "parameter is not a type reference");
                    }
                }
                إذا (allOwnParams) {
                    UINFO(5, "ifaceRefDeparam: self-reference pattern detected in "
                                 << ownerIfacep->prettyNameQ() << ", using owner interface");
                    V3Stats::جوڙيو_Stat_جوڙ("Param, Self-reference iface typedefs", 1);
                    nodep->ifacep(ownerIfacep);
                    إذا (nodep->paramsp()) nodep->paramsp()->unlink_Fr_پٺ_With_ايندڙ()->delete_وڻ();
                    أرجع ownerIfacep;
                }
            }
        }

        Ast_ڳنڍ_Module* ثابت newModp
            = ڳنڍ_Deparam_Common(nodep, srcModp, nodep->paramsp(), خالي_اشارو, خطأ);
        إذا (!newModp) أرجع خالي_اشارو;
        nodep->ifacep(VN_AS(newModp, Iface));
        أرجع newModp;
    }
    Ast_ڳنڍ_Module* classRefDeparam(AstClassOrPackageRef* nodep, Ast_ڳنڍ_Module* srcModp) {
        resolveDefaultParams(nodep);
        Ast_ڳنڍ_Module* ثابت newModp
            = ڳنڍ_Deparam_Common(nodep, srcModp, nodep->paramsp(), خالي_اشارو, خطأ);
        إذا (!newModp) أرجع خالي_اشارو;
        nodep->classOrPackagep(newModp);  // Might be unchanged if not cloned (newModp == srcModp)

        // If this ClassOrPackageRef is a child of a RefDType (e.g., typedef class#(T)::member_t),
        // resolve the RefDType's typedef to point to the typedef inside the specialized class
        AstRefDType* ثابت refDTypep = VN_CAST(nodep->backp(), RefDType);
        AstClass* ثابت newClassp = refDTypep ? VN_CAST(newModp, Class) : خالي_اشارو;
        إذا (newClassp && !refDTypep->typedefp() && !refDTypep->subDTypep()) {
            إذا (AstTypedef* ثابت typedefp
                = VN_CAST(m_memberMap.ڳوليو_Member(newClassp, refDTypep->name()), Typedef)) {
                refDTypep->typedefp(typedefp);
                refDTypep->classOrPackagep(newClassp);
                UINFO(9, "Resolved parameterized class typedef: " << refDTypep->name() << " -> "
                                                                  << typedefp << " in "
                                                                  << newClassp->name());
            }
        }
        أرجع newModp;
    }
    Ast_ڳنڍ_Module* classRefDeparam(AstClassRefDType* nodep, Ast_ڳنڍ_Module* srcModp) {
        resolveDefaultParams(nodep);
        Ast_ڳنڍ_Module* ثابت newModp
            = ڳنڍ_Deparam_Common(nodep, srcModp, nodep->paramsp(), خالي_اشارو, خطأ);
        إذا (!newModp) أرجع خالي_اشارو;
        AstClass* ثابت newClassp = VN_AS(newModp, Class);
        nodep->classp(newClassp);  // Might be unchanged if not cloned (newModp == srcModp)
        nodep->classOrPackagep(newClassp);
        أرجع newClassp;
    }

    // Resolve deferred class-scoped references in post-order. Resolution may
    // deparameterize a class and delete its parameter pins, so no pointer to a
    // child may remain pending when its parent is resolved.
    صنف DeferredResolverVisitor final : عام VNVisitor {
        ParamProcessor& m_processor;  // Processor used to resolve deferred references
        std::سيٽ<ثابت Ast_ڳنڍ*> m_reachedDecls;

        منطقي پهريون_پهچ(ثابت Ast_ڳنڍ* ثابت declp) { أرجع m_reachedDecls.وجهو(declp).ٻيو; }

        فراغ iterateParamType(AstParamTypeDType* ثابت paramTypep) {
            إذا (پهريون_پهچ(paramTypep)) iterate(paramTypep);
        }
        فراغ iterateTypedef(AstTypedef* ثابت tdefp) {
            إذا (پهريون_پهچ(tdefp)) iterate(tdefp->subDTypep());
        }

        فراغ گهمو(Ast_ڳنڍ* nodep) override { iterateChildren(nodep); }
        فراغ گهمو(AstClassOrPackageRef* nodep) override {
            iterateChildren(nodep);
            Ast_ڳنڍ* ثابت targetp = nodep->classOrPackageNodep();
            إذا (AstTypedef* ثابت tdefp = VN_CAST(targetp, Typedef)) {
                iterateTypedef(tdefp);
            } وإلا إذا (AstParamTypeDType* ثابت paramTypep = VN_CAST(targetp, ParamTypeDType)) {
                iterateParamType(paramTypep);
            }
        }
        فراغ گهمو(AstDot* nodep) override {
            iterateChildren(nodep);
            m_processor.resolve_Dot_منزل_Typedef(nodep);
        }
        فراغ گهمو(AstRefDType* nodep) override {
            iterateChildren(nodep);
            AstTypedef* ثابت tdefp = nodep->typedefp();
            إذا (tdefp) {
                iterateTypedef(tdefp);
            } وإلا إذا (AstParamTypeDType* ثابت paramTypep
                       = VN_CAST(nodep->refDTypep(), ParamTypeDType)) {
                iterateParamType(paramTypep);
            }
            m_processor.resolveParamClassRefDType(nodep);
        }
        فراغ گهمو(AstVarRef* nodep) override {
            iterateChildren(nodep);
            AstVar* ثابت varp = nodep->varp();
            إذا (پهريون_پهچ(varp)) {
                iterate(varp->subDTypep());
                ثابت خودڪار& deferredVarps = v3Global.rootp()->deferredParamVarps();
                إذا (varp->varType() == VVarType::LPARAM && deferredVarps.ڳڻپ(varp)) {
                    UASSERT_OBJ(varp->valuep(), varp, "VarRef should have non-null valuep");
                    iterate(varp->valuep());
                }
            }
        }

    عام:
        DeferredResolverVisitor(ParamProcessor& processor, Ast_ڳنڍ* rootp)
            : m_processor{processor} {
            iterate(rootp);
        }
    };

عام:
    // After an interface cell inside parentModp has been deparameterized
    // (rewired from template to clone), retarget REFDTYPEs that still
    // reference the old template's types so that $bits(iface_typedef)
    // evaluates with the clone's widths.
    فراغ retargetIfaceRefs(Ast_ڳنڍ_Module* والد_Modp, ثابت string& cellName) {
        Ast_ڳنڍ_Module* ثابت correctModp
            = V3LinkDotIfaceCapture::follow_Cell_رستو(والد_Modp, cellName);
        إذا (!correctModp || correctModp->dead() || correctModp->parameterizedTemplate()) أرجع;
        والد_Modp->foreach([&](AstRefDType* refp) {
            ثابت VIface_Capture_ٽيگ* ثابت tagp = V3LinkDotIfaceCapture::capture_ٽيگ(refp);
            إذا (!tagp || tagp->m_cell_رستو != cellName) أرجع;
            // Only retarget REFDTYPEs owned by parentModp.
            إذا (V3LinkDotIfaceCapture::ڳوليو_Owner_Module(refp) != والد_Modp) أرجع;
            إذا (V3LinkDotIfaceCapture::retarget_Ref_منزل_Module(refp, correctModp)) {
                UINFO(9, "retargetIfaceRefs: " << refp << " -> " << correctModp->prettyNameQ());
            }
        });
    }

    // Resolve deferred class-scoped values and types reachable from rootp. In
    // addition to AST children, follow typedefs, parameter types, deferred
    // localparam values, and referenced-variable dtypes. This makes the walk
    // independent of the particular dtype wrappers around a RefDType.
    فراغ resolveDeferredDotsReachableFrom(Ast_ڳنڍ* rootp, ثابت Ast_ڳنڍ_Module* modp) {
        VL_RESTORER(m_modp);
        m_modp = modp;
        DeferredResolverVisitor{*هذا, rootp};
    }

    Ast_ڳنڍ_Module* ڳنڍ_Deparam(Ast_ڳنڍ* nodep, Ast_ڳنڍ_Module* srcModp, Ast_ڳنڍ_Module* modp,
                               ثابت string& someInstanceName) {
        // Return new or reused de-parameterized module
        m_modp = modp;
        // Cell: Check for parameters in the instantiation.
        // We always run this, even if no parameters, as need to look for interfaces,
        // and remove any recursive references
        UINFO(4, "De-parameterize: " << nodep);
        UINFO(9, "nodeDeparam ENTER node=<"
                     << Ast_ڳنڍ::ڳنڍ_Addr(nodep) << ">" << " type=" << nodep->typeName()
                     << " srcMod=" << (srcModp ? srcModp->prettyNameQ() : "'<null>'")
                     << " srcSomeInstanceName='"
                     << (srcModp ? srcModp->someInstanceName() : string("<null>")) << "'"
                     << " parentMod=" << (modp ? modp->prettyNameQ() : "'<null>'")
                     << " parentSomeInstanceName='"
                     << (modp ? modp->someInstanceName() : string("<null>")) << "'"
                     << " inputSomeInstanceName='" << someInstanceName << "'");
        string ڳنڍ_Name = nodep->name();
        إذا (AstIfaceRefDType* ثابت ifaceRefp = VN_CAST(nodep, IfaceRefDType)) {
            إذا (ڳنڍ_Name.خالي_جايون()) ڳنڍ_Name = ifaceRefp->cellName();
        }
        ثابت string instanceName
            = ڳنڍ_Name.خالي_جايون() ? someInstanceName : (someInstanceName + "." + ڳنڍ_Name);

        إذا (AstCell* ثابت cellp = VN_CAST(nodep, Cell)) {
            لكل (AstPin* pinp = cellp->paramsp(); pinp;) {
                AstPin* ثابت nextp = VN_AS(pinp->nextp(), Pin);
                إذا (!pinp->param_رستو().خالي_جايون() && instanceName != pinp->param_رستو()
                    && !VString::پڇاڙيون_With(instanceName, "." + pinp->param_رستو())) {
                    VL_DO_DANGLING(pinp->unlink_Fr_پٺ()->delete_وڻ(), pinp);
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
                     << srcModp->someInstanceName() << "'" << " node=<" << Ast_ڳنڍ::ڳنڍ_Addr(nodep)
                     << ">" << " nodeType=" << nodep->typeName());

        Ast_ڳنڍ_Module* newModp = خالي_اشارو;
        إذا (AstCell* ثابت cellp = VN_CAST(nodep, Cell)) {
            newModp = cellDeparam(cellp, srcModp);
        } وإلا إذا (AstIfaceRefDType* ثابت ifaceRefDTypep = VN_CAST(nodep, IfaceRefDType)) {
            newModp = ifaceRefDeparam(ifaceRefDTypep, srcModp);
        } وإلا إذا (AstClassRefDType* ثابت classRefp = VN_CAST(nodep, ClassRefDType)) {
            newModp = classRefDeparam(classRefp, srcModp);
        } وإلا إذا (AstClassOrPackageRef* ثابت classRefp = VN_CAST(nodep, ClassOrPackageRef)) {
            newModp = classRefDeparam(classRefp, srcModp);
        } وإلا {
            nodep->v3fatalSrc("Expected module parameterization");
        }
        // 'newModp' might be nullptr on error
        إذا (!newModp) أرجع خالي_اشارو;

        // Set name for later warnings
        newModp->someInstanceName(instanceName);

        UINFO(9, "nodeDeparam EXIT  node=<"
                     << Ast_ڳنڍ::ڳنڍ_Addr(nodep) << ">" << " type=" << nodep->typeName()
                     << " srcMod=" << (srcModp ? srcModp->prettyNameQ() : "'<null>'")
                     << " srcSomeInstanceName='"
                     << (srcModp ? srcModp->someInstanceName() : string("<null>")) << "'"
                     << " newMod=" << (newModp ? newModp->prettyNameQ() : "'<null>'")
                     << " newSomeInstanceName='"
                     << (newModp ? newModp->someInstanceName() : string("<null>")) << "'");

        UINFO(8, "     Done with orig " << nodep);
        // if (debug() >= 10)
        // v3Global.rootp()->dumpTreeFile(v3Global.debugFilename("param-out.tree"));
        أرجع newModp;
    }

    // Return, and forget, the classes specialized since the previous call
    std::ويڪٽر<AstClass*> takeSpecializedClassps() {
        std::ويڪٽر<AstClass*> ورتل;
        ورتل.مٽا_سٽا(m_specializedClassps);
        أرجع ورتل;
    }

    // CONSTRUCTORS
    واضح ParamProcessor(AstNetlist* nodep)
        : m_hier_بلاڪون{v3Global.opt.hier_بلاڪون(), nodep} {
        لكل (Ast_ڳنڍ_Module* modp = nodep->modulesp(); modp;
             modp = VN_AS(modp->nextp(), ڳنڍ_Module)) {
            m_all_Module_نالا.وجهو(modp->name());
        }
    }
    ~ParamProcessor() = افتراضي;
    VL_UNCOPYABLE(ParamProcessor);
};

//######################################################################
// Record classes that are referenced with parameter pins

صنف ClassRefUnlinkerVisitor final : عام VNVisitor {

عام:
    واضح ClassRefUnlinkerVisitor(AstNetlist* netlistp) { iterate(netlistp); }
    فراغ گهمو(AstClassOrPackageRef* nodep) override {
        إذا (nodep->paramsp()) {
            إذا (AstClass* ثابت classp = VN_CAST(nodep->classOrPackageSkipp(), Class)) {
                إذا (!classp->user3p()) {
                    // Check if this ClassOrPackageRef is the lhsp of a DOT node
                    AstDot* ثابت dotp = VN_CAST(nodep->backp(), Dot);
                    إذا (dotp && dotp->lhsp() == nodep) {
                        // Replace DOT with just its rhsp to avoid leaving DOT with null lhsp
                        dotp->replaceWith(dotp->rhsp()->unlink_Fr_پٺ());
                        VL_DO_DANGLING2(ڌڪو_Deletep(dotp), dotp, nodep);
                    } وإلا {
                        VL_DO_DANGLING(ڌڪو_Deletep(nodep->unlink_Fr_پٺ()), nodep);
                    }
                }
            }
        }
    }
    فراغ گهمو(AstClass* nodep) override {}  // don't iterate inside classes
    فراغ گهمو(Ast_ڳنڍ* nodep) override { iterateChildren(nodep); }
};

//######################################################################
//  Relink RefDType nodes to point to parameterized classes' type parameters

صنف ParamClassRefDTypeRelinkVisitor final : عام VNVisitor {

    AstClass* m_classp = خالي_اشارو;
    std::unordered_map<ثابت AstParamTypeDType*, AstClass*> m_paramTypeClassMap;

    // Owner-tracking state for the post-clone REFDTYPE typedef retargeting
    // (see retargetRefDType()). m_ownerModp is the most recently entered
    // AstNodeModule (or AstClass). m_origNameToClone / m_ambiguous are
    // rebuilt lazily on the first AstRefDType visit per owner; the
    // m_ownerMapForp cache key avoids rebuilding when the owner is unchanged
    // (e.g. across siblings under the same parent).
    Ast_ڳنڍ_Module* m_ownerModp = خالي_اشارو;
    Ast_ڳنڍ_Module* m_ownerMapForp = خالي_اشارو;
    std::unordered_map<std::string, AstClass*> m_orig_Name_منزل_Clone;
    std::unordered_سيٽ<std::string> m_ambiguous;

    فراغ ensureOwnerMap() {
        إذا (m_ownerMapForp == m_ownerModp) أرجع;
        m_ownerMapForp = m_ownerModp;
        m_orig_Name_منزل_Clone.clear();
        m_ambiguous.clear();
        إذا (!m_ownerModp) أرجع;
        // Skip bare/generic class templates: their bodies are not
        // authoritative for any specialization, and their self-typedefs
        // (e.g. `typedef cls#(P,Q) this_type;` inside a parameterized class)
        // would otherwise be treated as ground truth and cause REFDTYPEs in
        // the body to be retargeted at the bare template.
        إذا (AstClass* ثابت ownerClassp = VN_CAST(m_ownerModp, Class)) {
            إذا (ownerClassp->hasGParam()) أرجع;
        }
        لكل (Ast_ڳنڍ* sp = m_ownerModp->stmtsp(); sp; sp = sp->nextp()) {
            AstTypedef* ثابت tdp = VN_CAST(sp, Typedef);
            إذا (!tdp) استمر;
            AstClassRefDType* ثابت crdtp = VN_CAST(tdp->subDTypep(), ClassRefDType);
            إذا (!crdtp || !crdtp->classp()) استمر;
            AstClass* ثابت targetp = crdtp->classp();
            // A typedef pointing at a still-generic template is a forward
            // declaration / unresolved reference, not ground truth.
            إذا (targetp->hasGParam()) استمر;
            ثابت std::string origName
                = targetp->origName().خالي_جايون() ? targetp->name() : targetp->origName();
            ثابت خودڪار pair = m_orig_Name_منزل_Clone.emplace(origName, targetp);
            // Two locally-resolved typedefs pointing at different
            // specializations of the same template -> ambiguous; we cannot
            // safely guess which one a stripped REFDTYPE meant.
            إذا (!pair.ٻيو && pair.پهريون->ٻيو != targetp) m_ambiguous.وجهو(origName);
        }
    }

    // A class's name before specialization (e.g. "holder" for "holder__Tz1").
    ساكن string classOrigName(ثابت AstClass* classp) {
        أرجع classp->origName().خالي_جايون() ? classp->name() : classp->origName();
    }

    // Find 'name' in classp or any base class (findTypedefInModule() searches
    // only the class itself).
    ساكن AstTypedef* ڳوليو_Typedef_With_Bases(AstClass* classp, ثابت string& name) {
        لكل (AstClass* cp = classp; cp; cp = cp->extendsp() ? cp->extendsp()->classp() : خالي_اشارو) {
            إذا (AstTypedef* ثابت tdp = V3LinkDotIfaceCapture::ڳوليو_Typedef_اندريون_Module(cp, name)) {
                إذا (tdp->subDTypep()) أرجع tdp;
            }
        }
        أرجع خالي_اشارو;
    }

    // Re-resolve REFDTYPE.typedefp/refDTypep using the containing module's
    // own resolved typedef chain. The eager retargeting in deepCloneModule
    // blindly retargets every captured entry to whichever sibling clone is
    // being created, so when a referenced parameterized class is cloned more
    // than once (e.g. driver -> driver__I5 + driver__I6), the second sibling
    // stomps the bindings made by the first. By this point each owner's
    // typedefs have been correctly bound (via classRefDeparam), so we use
    // those local typedefs as ground truth.
    فراغ retargetRefDType(AstRefDType* refp) {
        إذا (!m_ownerModp) أرجع;

        // IEEE 1800-2023 8.25.1: a bare class-qualified reference to the class
        // being compiled means the current specialization, but V3LinkDot binds
        // it to the default instance, so a typedef reached through it widens
        // with the template's defaults (#8348). Rebind it to the
        // specialization's own typedef.
        إذا (AstClass* ثابت ownerClassp = VN_CAST(m_ownerModp, Class)) {
            إذا (!ownerClassp->hasGParam() && !refp->paramsp()) {
                // What the reference resolves to: its typedef's owner if
                // linked, else the class it is qualified by.
                ثابت AstClass* refClassp = خالي_اشارو;
                إذا (AstTypedef* ثابت tdp = refp->typedefp()) {
                    refClassp = VN_CAST(V3LinkDotIfaceCapture::ڳوليو_Owner_Module(tdp), Class);
                } وإلا إذا (ثابت AstClassOrPackageRef* ثابت classRefp
                           = VN_CAST(refp->classOrPackageOpp(), ClassOrPackageRef)) {
                    refClassp = VN_CAST(classRefp->classOrPackageSkipp(), Class);
                }
                // Same originating class => bare self reference (another
                // specialization would carry #(), rejected above).
                إذا (refClassp && refClassp != ownerClassp
                    && classOrigName(refClassp) == classOrigName(ownerClassp)) {
                    AstTypedef* ثابت selfTdp = ڳوليو_Typedef_With_Bases(ownerClassp, refp->name());
                    إذا (selfTdp) {
                        UINFO(9, "post-param REFDTYPE self-reference retarget: "
                                     << refp << " from " << refClassp->name() << " to "
                                     << ownerClassp->name());
                        refp->typedefp(selfTdp);
                        refp->classOrPackagep(V3LinkDotIfaceCapture::ڳوليو_Owner_Module(selfTdp));
                        refp->refDTypep(selfTdp->subDTypep());
                        أرجع;
                    }
                }
            }
        }

        AstTypedef* ثابت oldTdp = refp->typedefp();
        إذا (!oldTdp) أرجع;
        AstClass* ثابت oldOwnerp = VN_CAST(V3LinkDotIfaceCapture::ڳوليو_Owner_Module(oldTdp), Class);
        إذا (!oldOwnerp) أرجع;

        ensureOwnerMap();
        إذا (m_orig_Name_منزل_Clone.خالي_جايون()) أرجع;
        ثابت std::string origName
            = oldOwnerp->origName().خالي_جايون() ? oldOwnerp->name() : oldOwnerp->origName();
        إذا (m_ambiguous.ڳڻپ(origName)) {
            // The owner has two locally-resolved typedefs pointing at
            // different specializations of the same template, e.g.:
            //   typedef driver #(.IW(5)) drv5_t;
            //   typedef driver #(.IW(6)) drv6_t;
            // A stripped REFDTYPE only carries the origName, so we cannot
            // tell which sibling it meant; leave it alone.
            UINFO(9, "post-param REFDTYPE retarget skipped (ambiguous owner '" << origName
                                                                               << "'): " << refp);
            أرجع;
        }
        ثابت خودڪار it = m_orig_Name_منزل_Clone.ڳوليو(origName);
        إذا (it == m_orig_Name_منزل_Clone.پڇاڙي()) أرجع;
        AstClass* ثابت correctClassp = it->ٻيو;
        إذا (correctClassp == oldOwnerp) أرجع;
        AstTypedef* ثابت newTdp
            = V3LinkDotIfaceCapture::ڳوليو_Typedef_اندريون_Module(correctClassp, refp->name());
        إذا (!newTdp) {
            // The owner map says correctClassp is the right specialization,
            // but it does not declare a typedef named refp->name(). Caveats
            // of findTypedefInModule (e.g. typedef declared in a base class,
            // not the specialization we resolved to) can land us here.
            UINFO(9, "post-param REFDTYPE retarget skipped (no typedef '"
                         << refp->name() << "' in " << correctClassp->name() << "): " << refp);
            أرجع;
        }
        // typedefp / classOrPackagep / refDTypep must agree. If newTdp lacks
        // a subDTypep we cannot keep all three in sync, so skip the entire
        // retarget rather than leaving the REFDTYPE half-updated.
        إذا (!newTdp->subDTypep()) {
            UINFO(9, "post-param REFDTYPE retarget skipped (newTdp has null subDTypep): " << refp);
            أرجع;
        }
        refp->typedefp(newTdp);
        refp->classOrPackagep(correctClassp);
        refp->refDTypep(newTdp->subDTypep());
        UINFO(9, "post-param REFDTYPE re-resolve: " << refp << " from " << oldOwnerp->name()
                                                    << " to " << correctClassp->name());
    }

عام:
    واضح ParamClassRefDTypeRelinkVisitor(AstNetlist* netlistp) { iterate(netlistp); }
    فراغ گهمو(Ast_ڳنڍ_Module* nodep) override {
        VL_RESTORER(m_ownerModp);
        m_ownerModp = nodep;
        iterateChildren(nodep);
    }
    فراغ گهمو(AstClass* nodep) override {
        VL_RESTORER(m_classp);
        VL_RESTORER(m_ownerModp);
        m_classp = nodep;
        m_ownerModp = nodep;
        لكل (Ast_ڳنڍ* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            إذا (AstParamTypeDType* ثابت ptp = VN_CAST(stmtp, ParamTypeDType)) {
                m_paramTypeClassMap[ptp] = nodep;
            }
        }
        iterateChildren(nodep);
    }
    فراغ گهمو(AstRefDType* nodep) override {
        iterateChildren(nodep);
        // Redirect refs whose refDTypep points at a stale ParamTypeDType in
        // a generic class template that has been replaced by a parametrized
        // clone.
        إذا (AstParamTypeDType* ثابت paramtypep = VN_CAST(nodep->refDTypep(), ParamTypeDType)) {
            ثابت خودڪار it = m_paramTypeClassMap.ڳوليو(paramtypep);
            إذا (it != m_paramTypeClassMap.پڇاڙي()) {
                AstClass* ثابت origClassp = it->ٻيو;
                إذا (origClassp->hasGParam() && !origClassp->user3p()) {
                    إذا (AstClass* ثابت parametrizedClassp
                        = VN_CAST(origClassp->user4p(), Class)) {
                        ثابت string paramName = paramtypep->name();
                        AstParamTypeDType* newParamTypep = خالي_اشارو;
                        لكل (Ast_ڳنڍ* stmtp = parametrizedClassp->stmtsp(); stmtp;
                             stmtp = stmtp->nextp()) {
                            إذا (AstParamTypeDType* ثابت ptp = VN_CAST(stmtp, ParamTypeDType)) {
                                إذا (ptp->name() == paramName) {
                                    newParamTypep = ptp;
                                    اكسر;
                                }
                            }
                        }
                        إذا (newParamTypep) {
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
    فراغ گهمو(Ast_ڳنڍ* nodep) override { iterateChildren(nodep); }
};

//######################################################################
// Process parameter top state

صنف ParamState final {
عام:
    // STATE - across all visitors
    std::ويڪٽر<AstClass*> m_paramClasses;  // Parameterized classes
    std::ويڪٽر<AstDot*> m_dots;  // Dot references to process
    // Work queue keyed by (!isIface, level) so interfaces are always processed
    // before non-interfaces.  This ensures interface clones have their types
    // properly widthed before any module that references those types.
    استعمال WQKey = std::pair<منطقي, صحيح>;
    std::multimap<WQKey, Ast_ڳنڍ_Module*> m_work_قطار_ايندڙ;  // Modules left to process
    // Map from AstNodeModule to set of all AstNodeModules that instantiates it.
    std::unordered_map<Ast_ڳنڍ_Module*, std::unordered_سيٽ<Ast_ڳنڍ_Module*>> m_parentps;
};

//######################################################################
// Process parameter visitor

صنف ParamVisitor final : عام VNVisitor {
    // NODE STATE
    // AstNodeModule::user1 -> bool: already fixed level (temporary)
    // AstNodeDType::user2  -> bool: already visited from typedef

    // STATE - across all visitors
    ParamState& m_state;  // Common state
    ParamProcessor m_processor;  // De-parameterize a cell, build modules
    GenForUnroller m_unroller;  // Unroller for AstGenFor

    منطقي m_iterateModule = خطأ;  // Iterating module body
    string m_unlinkedTxt;  // Text for AstUnlinkedRef
    std::multimap<منطقي, Ast_ڳنڍ*> m_cellps;  // Cells left to process (in current module)
    std::unordered_map<ثابت Ast_ڳنڍ*, std::string>
        m_gen_Hier_نالا;  // Maps ast nodes to generated hierarchy names
    std::map<ثابت AstRefDType*, منطقي>
        m_isCircular;  // Stores information whether `AstRefDType` is circular
    استعمال VarsByName = std::unordered_map<std::string, AstVar*>;
    // Persists across modules; one specialized interface clone serves every module bound to it
    std::unordered_map<ثابت Ast_ڳنڍ_Module*, VarsByName> m_ifaceParams;

    // STATE - for current visit position (use VL_RESTORER)
    Ast_ڳنڍ_Module* m_modp = خالي_اشارو;  // Module iterating
    std::unordered_سيٽ<std::string> m_iface_پورٽ_نالا;  // Interface port names in current module
    std::unordered_map<std::string, AstCell*>
        m_ifaceInstCells;  // Local interface instance cells in current module, keyed by name
    VarsByName m_modIfaceRefs;  // Interface-ref Vars in current module, keyed by name
    منطقي m_mod_Iface_Refs_پورو = خطأ;  // m_modIfaceRefs has been gathered for m_modp
    string m_generateHierName;  // Generate portion of hierarchy name
    منطقي m_اندريون_Generate_Cond = خطأ;  // Traversing a generate condition; see iterateGenerateCond

    // METHODS

    فراغ processWorkQ() {
        UASSERT(!m_iterateModule, "Should not nest");
        std::multimap<ParamState::WQKey, Ast_ڳنڍ_Module*> work_قطار;
        m_generateHierName = "";
        m_iterateModule = سچ;

        // Visit all cells under module, recursively
        طالما (سچ) {
            // Classes specialized since the last pass still need their bodies elaborated
            لكل (AstClass* ثابت classp : m_processor.takeSpecializedClassps()) {
                m_state.m_work_قطار_ايندڙ.emplace(ParamState::WQKey{سچ, classp->level()}, classp);
            }
            إذا (work_قطار.خالي_جايون()) std::مٽا_سٽا(work_قطار, m_state.m_work_قطار_ايندڙ);
            إذا (work_قطار.خالي_جايون()) اكسر;

            ثابت خودڪار itm = work_قطار.cbegin();
            Ast_ڳنڍ_Module* ثابت modp = itm->ٻيو;
            work_قطار.erase(itm);
            // Starting a new module, so what was learned about the last one no longer holds.
            v3Global.rootp()->clearContainingModules();

            // Process once; note user2 will be cleared on specialization, so we will do the
            // specialized module if needed
            إذا (!modp->user2_سيٽ_Once()) {
                UINFO(9, "processWorkQ module begin mod='"
                             << modp->name() << "' orig='" << modp->origName() << "'"
                             << " someInstanceName='" << modp->someInstanceName() << "'"
                             << " hasGParam=" << (modp->hasGParam() ? "yes" : "no")
                             << " user3p=" << (modp->user3p() ? "set" : "null")
                             << " dead=" << (modp->dead() ? "yes" : "no"));

                // TODO: this really should be an assert, but classes and hier_blocks are
                // special...
                إذا (modp->someInstanceName().خالي_جايون()) modp->someInstanceName(modp->origName());

                // Iterate the body
                {
                    VL_RESTORER(m_modp);
                    VL_RESTORER_CLEAR(m_iface_پورٽ_نالا);
                    VL_RESTORER_CLEAR(m_ifaceInstCells);
                    VL_RESTORER_CLEAR(m_modIfaceRefs);
                    VL_RESTORER(m_mod_Iface_Refs_پورو);
                    m_modp = modp;
                    m_mod_Iface_Refs_پورو = خطأ;
                    iterateChildren(modp);
                }
            }

            // Process interface cells, then non-interface cells, which may reference an interface
            // cell.
            طالما (!m_cellps.خالي_جايون()) {
                ثابت خودڪار itim = m_cellps.cbegin();
                Ast_ڳنڍ* ثابت cellp = itim->ٻيو;
                m_cellps.erase(itim);

                // Consume the generated hierarchy name for the node
                string genHierName;
                ثابت خودڪار genHierIt = m_gen_Hier_نالا.ڳوليو(cellp);
                إذا (genHierIt != m_gen_Hier_نالا.پڇاڙي()) {
                    genHierName = std::move(genHierIt->ٻيو);
                    m_gen_Hier_نالا.erase(genHierIt);
                }

                Ast_ڳنڍ_Module* srcModp = خالي_اشارو;
                إذا (ثابت AstCell* modCellp = VN_CAST(cellp, Cell)) {
                    srcModp = modCellp->modp();
                } وإلا إذا (ثابت AstClassOrPackageRef* classRefp
                           = VN_CAST(cellp, ClassOrPackageRef)) {
                    srcModp = classRefp->classOrPackageSkipp();
                    إذا (VN_IS(classRefp->classOrPackageNodep(), ParamTypeDType)) استمر;
                } وإلا إذا (ثابت AstClassRefDType* classRefp = VN_CAST(cellp, ClassRefDType)) {
                    srcModp = classRefp->classp();
                } وإلا إذا (ثابت AstIfaceRefDType* ifaceRefp = VN_CAST(cellp, IfaceRefDType)) {
                    srcModp = ifaceRefp->ifacep();
                } وإلا {
                    cellp->v3fatalSrc("Expected module parameterization");
                }
                إذا (!srcModp) استمر;

                // Update path
                ثابت string someInstanceName = modp->someInstanceName() + genHierName;

                // Apply parameter specialization
                إذا (Ast_ڳنڍ_Module* ثابت newModp
                    = m_processor.ڳنڍ_Deparam(cellp, srcModp, modp, someInstanceName)) {

                    إذا (VN_IS(srcModp, Iface)) {
                        logTemplateLeakRefs(modp, srcModp, "after queued nodeDeparam", cellp);
                        // After the interface cell is rewired to its clone,
                        // retarget REFDTYPEs in the parent module that still
                        // reference the template interface's types.
                        إذا (V3LinkDotIfaceCapture::enabled()) {
                            إذا (ثابت AstCell* ثابت modCellp = VN_CAST(cellp, Cell)) {
                                إذا (newModp != srcModp) {
                                    m_processor.retargetIfaceRefs(modp, modCellp->name());
                                }
                            }
                        }
                    }

                    // Add the (now potentially specialized) child module to the work queue
                    work_قطار.emplace(ParamState::WQKey{!VN_IS(newModp, Iface), newModp->level()},
                                      newModp);

                    // Add to the hierarchy registry
                    m_state.m_parentps[newModp].وجهو(modp);

                    // Eagerly specialize nested iface cells so their types
                    // have correct widths before sibling module cells run.
                    إذا (VN_IS(newModp, Iface) && newModp != srcModp) {
                        specializeNestedIfaceCells(newModp);
                    }
                }
            }
        }

        m_iterateModule = خطأ;
    }

    // Extract the base reference name from a dotted VarXRef (e.g., "iface.FOO" -> "iface")
    ساكن string get_Ref_بنياد_Name(ثابت AstVarXRef* refp) {
        ثابت string dotted = refp->dotted();
        إذا (dotted.خالي_جايون()) أرجع "";
        أرجع dotted.substr(0, dotted.ڳوليو('.'));
    }

    // Debug-only diagnostic (requires --debugi-V3Param 9).
    // Walks parentModp looking for RefDTypes or VarRefs whose typedef,
    // refDType, or variable target is still owned by templateModp (the
    // unspecialized interface template).  Any such "leak" indicates a
    // pointer that was not properly redirected to the clone during
    // deparameterization.  Logs each leak with ancestry for triage.
    فراغ logTemplateLeakRefs(Ast_ڳنڍ_Module* والد_Modp, Ast_ڳنڍ_Module* templateModp,
                             ثابت حرف* stage, Ast_ڳنڍ* contextp) {
        إذا (debug() < 9 || !والد_Modp || !templateModp || !VN_IS(templateModp, Iface)) أرجع;
        // LCOV_EXCL_START  // Debug-only diagnostic
        صحيح leak_ڳڻپ = 0;
        ثابت خودڪار ancestry_جو = [](ثابت Ast_ڳنڍ* nodep) {
            string ancestry;
            لكل (ثابت Ast_ڳنڍ* curp = nodep; curp; curp = curp->backp()) {
                إذا (!ancestry.خالي_جايون()) ancestry += "<-";
                ancestry += curp->typeName();
                إذا (VN_IS(curp, ڳنڍ_Module) || VN_IS(curp, Netlist) || VN_IS(curp, Type_جدول)) {
                    اكسر;
                }
            }
            أرجع ancestry;
        };

        والد_Modp->foreach([&](AstRefDType* refp) {
            إذا (refp->typedefp()) {
                Ast_ڳنڍ_Module* ثابت tdOwnerp
                    = V3LinkDotIfaceCapture::ڳوليو_Owner_Module(refp->typedefp());
                إذا (tdOwnerp == templateModp) {
                    ++leak_ڳڻپ;
                    UINFO(9, "TEMPLATE-LEAK "
                                 << stage << " parent=" << والد_Modp->prettyNameQ()
                                 << " template=" << templateModp->prettyNameQ() << " contextType="
                                 << (contextp ? contextp->typeName() : string("<null>"))
                                 << " contextName="
                                 << (contextp ? contextp->prettyNameQ() : "'<null>'")
                                 << " leak=REFDTYPE typedef owner" << " ref=<"
                                 << Ast_ڳنڍ::ڳنڍ_Addr(refp) << ">" << " refName="
                                 << refp->prettyNameQ() << " ancestry=" << ancestry_جو(refp));
                }
            }
            إذا (refp->refDTypep()) {
                Ast_ڳنڍ_Module* ثابت rdOwnerp
                    = V3LinkDotIfaceCapture::ڳوليو_Owner_Module(refp->refDTypep());
                إذا (rdOwnerp == templateModp) {
                    ++leak_ڳڻپ;
                    UINFO(9, "TEMPLATE-LEAK "
                                 << stage << " parent=" << والد_Modp->prettyNameQ()
                                 << " template=" << templateModp->prettyNameQ() << " contextType="
                                 << (contextp ? contextp->typeName() : string("<null>"))
                                 << " contextName="
                                 << (contextp ? contextp->prettyNameQ() : "'<null>'")
                                 << " leak=REFDTYPE refDType owner" << " ref=<"
                                 << Ast_ڳنڍ::ڳنڍ_Addr(refp) << ">" << " refName="
                                 << refp->prettyNameQ() << " ancestry=" << ancestry_جو(refp));
                }
            }
        });

        والد_Modp->foreach([&](AstVarRef* varrefp) {
            إذا (!varrefp->varp()) أرجع;
            Ast_ڳنڍ_Module* ثابت varOwnerp
                = V3LinkDotIfaceCapture::ڳوليو_Owner_Module(varrefp->varp());
            إذا (varOwnerp != templateModp) أرجع;
            ++leak_ڳڻپ;
            UINFO(9, "TEMPLATE-LEAK "
                         << stage << " parent=" << والد_Modp->prettyNameQ()
                         << " template=" << templateModp->prettyNameQ()
                         << " contextType=" << (contextp ? contextp->typeName() : string("<null>"))
                         << " contextName=" << (contextp ? contextp->prettyNameQ() : "'<null>'")
                         << " leak=VARREF target owner" << " ref=<" << Ast_ڳنڍ::ڳنڍ_Addr(varrefp)
                         << ">" << " var=" << varrefp->prettyNameQ()
                         << " ancestry=" << ancestry_جو(varrefp));
        });

        إذا (leak_ڳڻپ > 0) {
            UINFO(9, "TEMPLATE-LEAK summary stage='"
                         << stage << "' parent=" << والد_Modp->prettyNameQ()
                         << " template=" << templateModp->prettyNameQ() << " count=" << leak_ڳڻپ);
        }
        // LCOV_EXCL_STOP
    }

    // Flag hierarchical refs in a parameter value. Single top-down pass.
    فراغ checkParamNotHierRecurse(Ast_ڳنڍ* nodep, منطقي underTypeQuery = خطأ) {
        لكل (; nodep; nodep = nodep->nextp()) {
            // Refs read only for their type ($bits etc.) are allowed
            ثابت Ast_Attr_جو* ثابت attrp = VN_CAST(nodep, Attr_جو);
            ثابت منطقي ٻار_Under_Query
                = underTypeQuery || (attrp && attrp->attrType().isTypeQuery());
            إذا (ثابت AstVarXRef* ثابت refp = VN_CAST(nodep, VarXRef)) {
                // Allow hierarchical ref to interface params through interface/modport ports
                // or local interface instances
                منطقي isIfaceRef = خطأ;
                إذا (refp->varp() && refp->varp()->isIfaceParam()) {
                    ثابت string refname = get_Ref_بنياد_Name(refp);
                    isIfaceRef
                        = !refname.خالي_جايون()
                          && (m_iface_پورٽ_نالا.ڳڻپ(refname) || m_ifaceInstCells.ڳڻپ(refname));
                }
                إذا (!isIfaceRef && !underTypeQuery) {
                    refp->v3warn(HIERPARAM, "Parameter values cannot use hierarchical values"
                                            " (IEEE 1800-2023 6.20.2)");
                }
            } وإلا إذا (ثابت Ast_ڳنڍ_FTask_Ref* ثابت refp = VN_CAST(nodep, ڳنڍ_FTask_Ref)) {
                إذا (refp->dotted() != "") {
                    refp->v3error("Parameter values cannot call hierarchical functions"
                                  " (IEEE 1800-2023 6.20.2)");
                }
            }
            checkParamNotHierRecurse(nodep->op1p(), ٻار_Under_Query);
            checkParamNotHierRecurse(nodep->op2p(), ٻار_Under_Query);
            checkParamNotHierRecurse(nodep->op3p(), ٻار_Under_Query);
            checkParamNotHierRecurse(nodep->op4p(), ٻار_Under_Query);
        }
    }

    // Deparameterize and constify nested interface cells within ifaceModp.
    فراغ specializeNestedIfaceCells(Ast_ڳنڍ_Module* ifaceModp) {
        لكل (Ast_ڳنڍ* stmtp = ifaceModp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            AstCell* ثابت nestedCellp = VN_CAST(stmtp, Cell);
            إذا (!nestedCellp) استمر;
            إذا (!VN_IS(nestedCellp->modp(), Iface)) استمر;
            إذا (!nestedCellp->paramsp()) استمر;

            Ast_ڳنڍ_Module* ثابت nestedSrcModp = nestedCellp->modp();
            إذا (Ast_ڳنڍ_Module* ثابت nestedNewModp = m_processor.ڳنڍ_Deparam(
                    nestedCellp, nestedSrcModp, ifaceModp, ifaceModp->someInstanceName())) {
                إذا (nestedNewModp != nestedSrcModp) {
                    // Constify the nested clone's params so its types have correct widths.
                    لكل (Ast_ڳنڍ* sp = nestedNewModp->stmtsp(); sp; sp = sp->nextp()) {
                        إذا (AstVar* ثابت varp = VN_CAST(sp, Var)) {
                            إذا (varp->isParam() && varp->valuep()) {
                                V3Const::constifyParamsEdit(varp);
                            }
                        }
                    }
                    // Retarget REFDTYPEs in the outer clone to the nested clone's types.
                    إذا (V3LinkDotIfaceCapture::enabled()) {
                        m_processor.retargetIfaceRefs(ifaceModp, nestedCellp->name());
                    }
                    specializeNestedIfaceCells(nestedNewModp);
                }
            }
        }
    }

    // Check if cell parameters reference interface ports or local interface instances
    منطقي cellParamsReferenceIfacePorts(AstCell* cellp) {
        إذا (!cellp->paramsp()) أرجع خطأ;

        لكل (Ast_ڳنڍ* nodep = cellp->paramsp(); nodep; nodep = nodep->nextp()) {
            إذا (AstPin* ثابت pinp = VN_CAST(nodep, Pin)) {
                إذا (Ast_ڳنڍ* ثابت exprp = pinp->exprp()) {
                    إذا (ثابت AstVarXRef* ثابت refp = VN_CAST(exprp, VarXRef)) {
                        ثابت string refname = get_Ref_بنياد_Name(refp);
                        إذا (!refname.خالي_جايون()
                            && (m_iface_پورٽ_نالا.ڳڻپ(refname)
                                || m_ifaceInstCells.ڳڻپ(refname)))
                            أرجع سچ;
                    }
                }
            }
        }
        أرجع خطأ;
    }

    // Split interface array port 'portp' into a variable for each of its elements, in the
    // dimensions of 'dtypep' inwards. 'suffix' is the name suffix of the dimensions outside
    // 'dtypep'.
    فراغ expand_Iface_Array_پورٽ_Dimensions(AstVar* portp, Ast_ڳنڍ_DType* dtypep,
                                        ثابت std::string& suffix) {
        ثابت AstUnpackArrayDType* ثابت arrp = VN_CAST(dtypep->skipRefp(), UnpackArrayDType);
        // Base case: add the element variable when no dimensions left
        إذا (!arrp) {
            AstVar* ثابت varp = portp->clone_وڻ(خطأ);
            varp->name(portp->name() + suffix);
            varp->origName(portp->origName() + suffix);
            إذا (Ast_ڳنڍ_DType* ثابت oldp = varp->ٻار_DTypep()) {
                VL_DO_DANGLING(ڌڪو_Deletep(oldp->unlink_Fr_پٺ()), oldp);
            }
            varp->dtypep(خالي_اشارو);
            varp->ٻار_DTypep(portp->subDTypep()->elemDTypep()->clone_وڻ(خطأ));
            portp->جوڙيو_ايندڙ_هتي(varp);
            أرجع;
        }

        // Enumerate the current dimension given by 'arrp'
        // Each element is added right after 'portp', so go from right to left,
        // to end with an enumeration from the left index to the right index.
        لكل (ثابت صحيح n : arrp->declRange().seq_ساڄو_منزل_کاٻو()) {
            ثابت std::string s = suffix + "__BRA__" + Ast_ڳنڍ::encode_عدد(n) + "__KET__";
            expand_Iface_Array_پورٽ_Dimensions(portp, arrp->subDTypep(), s);
        }
    }

    // Add the elements of instance array 'arrayedCellp' in the dimensions from 'rangep' inwards.
    // 'suffix' and 'idx' are the name suffix and row-major position (each dimension counted from
    // the left) of the dimensions outside 'rangep'. For an array of interfaces, 'ifaceVarp' is
    // the variable referencing the whole array (see V3LinkCells), which is also split into
    // variables referencing the elements.
    فراغ expandCellArrayDimensions(AstCell* arrayedCellp, AstVar* ifaceVarp,
                                   ثابت AstRange* rangep, ثابت std::string& suffix, صحيح idx) {
        // Base case: insert the element when no dimensions left
        إذا (!rangep) {
            AstCell* ثابت elemp = arrayedCellp->clone_وڻ(خطأ);
            elemp->name(arrayedCellp->name() + suffix);
            elemp->origName(arrayedCellp->origName() + suffix);
            elemp->arrayIdx(idx);
            arrayedCellp->جوڙيو_ايندڙ_هتي(elemp);
            إذا (ifaceVarp) {
                AstIfaceRefDType* ثابت irefp
                    = VN_AS(ifaceVarp->subDTypep()->elemDTypep(), IfaceRefDType)->clone_وڻ(خطأ);
                irefp->cellp(elemp);
                irefp->cellName(elemp->name());
                // Named like the variable of any interface cell (see V3LinkCells)
                AstVar* ثابت varp = ifaceVarp->clone_وڻ(خطأ);
                varp->name(elemp->name() + "__Viftop");
                varp->origName(elemp->origName() + "__Viftop");
                إذا (Ast_ڳنڍ_DType* ثابت oldp = varp->ٻار_DTypep()) {
                    VL_DO_DANGLING(ڌڪو_Deletep(oldp->unlink_Fr_پٺ()), oldp);
                }
                varp->dtypep(خالي_اشارو);
                varp->ٻار_DTypep(irefp);
                ifaceVarp->جوڙيو_ايندڙ_هتي(varp);
            }
            أرجع;
        }

        // Enumerate the current dimension given by 'rangep'
        // Each element is added right after 'arrayedCellp', so go from right to left,
        // to end with an enumeration from the left index to the right index.
        idx = (idx + 1) * rangep->elementsConst();
        ثابت AstRange* ثابت subRangep = VN_AS(rangep->nextp(), Range);
        لكل (ثابت صحيح n : rangep->seq_ساڄو_منزل_کاٻو()) {
            ثابت std::string s = suffix + "__BRA__" + Ast_ڳنڍ::encode_عدد(n) + "__KET__";
            expandCellArrayDimensions(arrayedCellp, ifaceVarp, subRangep, s, --idx);
        }
    }

    // A generic visitor for cells and class refs
    فراغ گهمو_Cell_Or_Class_Ref(Ast_ڳنڍ* nodep, منطقي isIface) {
        // Must do ifaces first, so push to list and do in proper order
        m_gen_Hier_نالا.emplace(nodep, m_generateHierName);

        // Deparameterize iface cells early so types are available for lparams.
        إذا (isIface && VN_CAST(nodep, Cell) && VN_CAST(nodep, Cell)->paramsp()) {
            AstCell* ثابت cellp = VN_CAST(nodep, Cell);
            إذا (!cellParamsReferenceIfacePorts(cellp)) {
                Ast_ڳنڍ_Module* ثابت srcModp = cellp->modp();
                Ast_ڳنڍ_Module* ثابت newModp
                    = m_processor.ڳنڍ_Deparam(cellp, srcModp, m_modp, m_modp->someInstanceName());
                // Retarget template REFDTYPEs to the clone's types.
                إذا (V3LinkDotIfaceCapture::enabled() && cellp->modp() != srcModp) {
                    m_processor.retargetIfaceRefs(m_modp, cellp->name());
                }
                // Specialize nested iface cells so their types are correct.
                إذا (newModp && newModp != srcModp) { specializeNestedIfaceCells(newModp); }
            }
        }

        // Visit parameters in the instantiation.
        iterateChildren(nodep);
        // A generate condition is folded and deleted before the drain loop runs, so
        // queueing from one only leaves a dangling pointer.  See iterateGenerateCond.
        إذا (!m_اندريون_Generate_Cond) m_cellps.emplace(!isIface, nodep);
    }

    // VISITORS
    فراغ گهمو(Ast_ڳنڍ_Module* nodep) override {
        إذا (nodep->recursiveClone()) nodep->dead(سچ);  // Fake, made for recursive elimination
        إذا (nodep->dead()) أرجع;  // Marked by LinkDot (and above)
        UINFO(9, "V3Param: visit module name=" << nodep->prettyNameQ() << " orig='"
                                               << nodep->origName() << "' someInstanceName='"
                                               << nodep->someInstanceName() << "' hasGParam="
                                               << (nodep->hasGParam() ? "yes" : "no")
                                               << " user3p=" << (nodep->user3p() ? "set" : "null")
                                               << " dead=" << (nodep->dead() ? "yes" : "no"));
        إذا (AstClass* ثابت classp = VN_CAST(nodep, Class)) {
            إذا (classp->hasGParam()) {
                // Don't enter into a definition.
                // If a class is used, it will be visited through a reference and cloned
                m_state.m_paramClasses.ڌڪو_پٺ(classp);
                أرجع;
            }
        }

        إذا (m_iterateModule) {  // Iterating from processWorkQ
            UINFO(4, " MOD-under-MOD.  " << nodep);
            // Delay until current module is done.
            // processWorkQ() (which we are returning to) will process nodep later
            m_state.m_work_قطار_ايندڙ.emplace(
                ParamState::WQKey{!VN_IS(nodep, Iface), nodep->level()}, nodep);
            أرجع;
        }

        // Start traversal at root-like things
        إذا (nodep->is_چوٽي()  // Tops
            || VN_IS(nodep, Class)  //  Moved classes
            || VN_IS(nodep, Package)) {  // Likewise haven't done wrapTopPackages yet
            m_state.m_work_قطار_ايندڙ.emplace(
                ParamState::WQKey{!VN_IS(nodep, Iface), nodep->level()}, nodep);
            processWorkQ();
        }
    }

    منطقي isCircularType(ثابت AstRefDType* nodep) {
        ثابت خودڪار iter = m_isCircular.emplace(nodep, سچ);
        إذا (!iter.ٻيو) أرجع iter.پهريون->ٻيو;
        إذا (ثابت AstRefDType* ثابت subDTypep = VN_CAST(nodep->subDTypep(), RefDType)) {
            ثابت منطقي ret = isCircularType(subDTypep);
            iter.پهريون->ٻيو = ret;
            أرجع ret;
        }
        iter.پهريون->ٻيو = خطأ;
        أرجع خطأ;
    }

    فراغ گهمو(AstRefDType* nodep) override {
        ثابت منطقي isCircular = isCircularType(nodep);
        // A module-body `typedef C#(Cfg)::t alias` the deferred pin/param walk
        // never reached would survive to V3Width unlinked. If its alias chain is
        // broken, run the same deferred resolution here.
        إذا (!isCircular && nodep->typedefp() && !nodep->skipRefOrNullp()) {
            m_processor.resolveDeferredDotsReachableFrom(nodep, m_modp);
        }
        إذا (isCircular) {
            nodep->v3error("Typedef's type is circular: " << nodep->prettyName());
        } وإلا إذا (nodep->typedefp() && nodep->subDTypep()
                   && (VN_IS(nodep->subDTypep()->skipRefOrNullp(), IfaceRefDType)
                       || VN_IS(nodep->subDTypep()->skipRefOrNullp(), ClassRefDType))
                   && !nodep->skipRefp()->user2_سيٽ_Once()) {
            // The visit() function for every valid nodep->skipRefp() type below
            // must include a `user2SetOnce()` check to avoid adding duplicate
            // nodes to m_cellps
            iterate(nodep->skipRefp());
        }
        iterateChildren(nodep);
    }
    فراغ گهمو(AstCell* nodep) override {
        إذا (nodep->rangep() && nodep->arrayIdx() < 0) {
            // Type check the ranges
            لكل (AstRange* rangep = nodep->rangep(); rangep;
                 rangep = VN_AS(rangep->nextp(), Range)) {
                rangep = VN_AS(V3_ويڪر::ويڪر_Params_Edit(rangep), Range);
            }
            // If it's an interface, pick up the corresponding AstIfaceRefDType variable
            AstVar* ثابت ifaceVarp = [&]() -> AstVar* {
                إذا (!VN_IS(nodep->modp(), Iface)) أرجع خالي_اشارو;
                // The variable referencing the whole array, right after the cell (see V3LinkCells)
                AstVar* ثابت varp = VN_CAST(nodep->nextp(), Var);
                UASSERT_OBJ(varp && varp->name() == nodep->name() + "__Viftop", nodep,
                            "No __Viftop variable for interface array");
                أرجع varp;
            }();
            // Expand the instance array into its elements
            expandCellArrayDimensions(nodep, ifaceVarp, nodep->rangep(), "", 0);
            إذا (ifaceVarp) {
                ifaceVarp->is_Iface_Array_ورهايو(سچ);
                // Until V3Width removes it, the whole array variable refers to the first
                // element, which is also used for accessing the parameters of the array
                AstCell* ثابت firstp = VN_AS(nodep->nextp(), Cell);
                AstIfaceRefDType* ثابت irefp
                    = VN_AS(ifaceVarp->subDTypep()->elemDTypep(), IfaceRefDType);
                irefp->cellp(firstp);
                irefp->cellName(firstp->name());
                m_ifaceInstCells.emplace(nodep->name(), firstp);
            }
            VL_DO_DANGLING(ڌڪو_Deletep(nodep->unlink_Fr_پٺ()), nodep);
            // The elements replaced the arrayed cell in place, so will be iterated next.
            أرجع;
        }
        checkParamNotHierRecurse(nodep->paramsp());
        إذا (VN_IS(nodep->modp(), Iface)) m_ifaceInstCells.emplace(nodep->name(), nodep);
        گهمو_Cell_Or_Class_Ref(nodep, VN_IS(nodep->modp(), Iface));
    }
    فراغ گهمو(AstIfaceRefDType* nodep) override {
        nodep->skipRefp()->user2_سيٽ_Once();
        إذا (nodep->ifacep()) گهمو_Cell_Or_Class_Ref(nodep, سچ);
    }
    فراغ گهمو(AstClassRefDType* nodep) override {
        nodep->skipRefp()->user2_سيٽ_Once();
        checkParamNotHierRecurse(nodep->paramsp());
        گهمو_Cell_Or_Class_Ref(nodep, خطأ);
    }
    فراغ گهمو(AstClassOrPackageRef* nodep) override {
        // If it points to a typedef it is not really a class reference. That typedef will be
        // visited anyway (from its parent node), so even if it points to a parameterized class
        // type, the instance will be created.
        إذا (!VN_IS(nodep->classOrPackageNodep(), Typedef)) گهمو_Cell_Or_Class_Ref(nodep, خطأ);
    }

    // Recurse into AstGenBlock as generate blocks aren't flattened until V3Begin::debeginAll,
    // well after V3Param runs
    ساكن فراغ gatherVars(Ast_ڳنڍ* stmtsp, منطقي (*matchp)(ثابت AstVar*), VarsByName& varsr) {
        لكل (Ast_ڳنڍ* nodep = stmtsp; nodep; nodep = nodep->nextp()) {
            إذا (AstVar* ثابت varp = VN_CAST(nodep, Var)) {
                // emplace, not assign, so the first declaration of a name wins
                إذا (matchp(varp)) varsr.emplace(varp->name(), varp);
            } وإلا إذا (Ast_Gen_بلاڪ* ثابت genp = VN_CAST(nodep, Gen_بلاڪ)) {
                gatherVars(genp->itemsp(), matchp, varsr);
            }
        }
    }
    ثابت VarsByName& modIfaceRefs() {
        إذا (!m_mod_Iface_Refs_پورو) {
            gatherVars(
                m_modp->stmtsp(), [](ثابت AstVar* varp) { أرجع varp->isIfaceRef(); },
                m_modIfaceRefs);
            m_mod_Iface_Refs_پورو = سچ;
        }
        أرجع m_modIfaceRefs;
    }
    ثابت VarsByName& ifaceParams(Ast_ڳنڍ_Module* ifacep) {
        ثابت خودڪار pair = m_ifaceParams.emplace(ifacep, VarsByName{});
        إذا (pair.ٻيو) {
            gatherVars(
                ifacep->stmtsp(), [](ثابت AstVar* varp) { أرجع varp->isParam(); },
                pair.پهريون->ٻيو);
        }
        أرجع pair.پهريون->ٻيو;
    }
    // Resolve a generic-interface 'ifacePort.member' Dot left unlinked by V3LinkDot, now that
    // ifacePort may have been specialized. Returns nullptr if still unresolvable
    Ast_ڳنڍ* tryResolveGenericIfaceDot(AstDot* dotp) {
        AstParseRef* ثابت lhsp = VN_CAST(dotp->lhsp(), ParseRef);
        AstParseRef* ثابت rhsp = VN_CAST(dotp->rhsp(), ParseRef);
        إذا (!lhsp || !rhsp) أرجع خالي_اشارو;
        // Always called from visit(AstVar) inside processWorkQ()'s VL_RESTORER(m_modp) scope
        إذا (VL_UNCOVERABLE(!m_modp)) أرجع خالي_اشارو;

        ثابت خودڪار& refVars = modIfaceRefs();
        ثابت خودڪار ifaceVarIt = refVars.ڳوليو(lhsp->name());
        // V3LinkDot only defers Dots whose lhs is a non-array iface-ref var, so this can't fail
        إذا (VL_UNCOVERABLE(ifaceVarIt == refVars.پڇاڙي())) أرجع خالي_اشارو;
        AstVar* ثابت ifaceVarp = ifaceVarIt->ٻيو;
        AstIfaceRefDType* ثابت ifacerefp = VN_CAST(ifaceVarp->subDTypep(), IfaceRefDType);
        إذا (VL_UNCOVERABLE(!ifacerefp || !ifacerefp->ifacep())) أرجع خالي_اشارو;

        ثابت خودڪار& ifaceVars = ifaceParams(ifacerefp->ifacep());
        ثابت خودڪار هدف_Var_It = ifaceVars.ڳوليو(rhsp->name());
        إذا (هدف_Var_It == ifaceVars.پڇاڙي()) أرجع خالي_اشارو;
        AstVar* ثابت هدف_Varp = هدف_Var_It->ٻيو;
        // processWorkQ() visits interface cells first, so the interface is already constified
        إذا (VL_UNCOVERABLE(!VN_IS(هدف_Varp->valuep(), Const))) iterate(هدف_Varp);
        إذا (VL_UNCOVERABLE(!هدف_Varp->valuep() || !VN_IS(هدف_Varp->valuep(), Const))) {
            أرجع خالي_اشارو;  // LCOV_EXCL_LINE
        }
        أرجع هدف_Varp->valuep()->clone_وڻ(خطأ);
    }
    فراغ resolve_Generic_Iface_Dots_اندريون(Ast_ڳنڍ* nodep) {
        // Collect first: the deleteTree() below frees a node foreach() would still read from
        std::ويڪٽر<AstDot*> dotps;
        nodep->foreach([&](AstDot* dotp) { dotps.ڌڪو_پٺ(dotp); });
        لكل (AstDot* ثابت dotp : dotps) {
            إذا (Ast_ڳنڍ* ثابت newp = tryResolveGenericIfaceDot(dotp)) {
                dotp->replaceWith(newp);
                VL_DO_DANGLING(dotp->delete_وڻ(), dotp);
            }
        }
    }

    // Make sure all parameters are constantified
    فراغ گهمو(AstVar* nodep) override {
        إذا (nodep->user2_سيٽ_Once()) أرجع;  // Process once
        // Interface array port: split it into its elements
        إذا (nodep->isIfaceRef() && !nodep->is_Iface_والد()
            && (VN_IS(nodep->subDTypep()->skipRefp(), UnpackArrayDType)
                || VN_IS(nodep->subDTypep()->skipRefp(), BracketArrayDType))) {
            // Also converts any C-style [N] dimensions, the ranges must be constant
            V3_ويڪر::ويڪر_Params_Edit(nodep->subDTypep());
            expand_Iface_Array_پورٽ_Dimensions(nodep, nodep->subDTypep(), "");
            nodep->is_Iface_Array_ورهايو(سچ);
        }
        // Build cache of interface port names as we encounter them
        إذا (nodep->isIfaceRef()) { m_iface_پورٽ_نالا.وجهو(nodep->name()); }
        iterateChildren(nodep);
        إذا (nodep->isParam()) {
            checkParamNotHierRecurse(nodep->valuep());
            إذا (!nodep->valuep() && !VN_IS(m_modp, Class)) {
                nodep->v3error("Parameter without default value is never given value"
                               << " (IEEE 1800-2023 6.20.1): " << nodep->prettyNameQ());
            } وإلا إذا (nodep->valuep()) {
                // Resolve now: a sibling cell's pin fold may need this before linkDotParamed
                resolve_Generic_Iface_Dots_اندريون(nodep->valuep());
                // If the value expression contains a VarXRef to an interface
                // localparam whose value is not yet constant, defer constification
                // to avoid premature widthing with unresolved values (see
                // t_lparam_dep_iface tests).  When the referenced localparam is
                // already const, proceed normally so FUNCREFs with resolved iface
                // param args get folded.
                منطقي hasUnresolvedLparamXRef = خطأ;
                nodep->valuep()->foreach([&](ثابت AstVarXRef* xrefp) {
                    إذا (ثابت AstVar* ثابت varp = xrefp->varp()) {
                        إذا (varp->varType() == VVarType::LPARAM && !VN_IS(varp->valuep(), Const)) {
                            hasUnresolvedLparamXRef = سچ;
                        }
                    }
                });
                إذا (hasUnresolvedLparamXRef) أرجع;
                // Defer if value has any unresolved Dot, or references a deferred lparam
                ثابت منطقي hasDot = nodep->valuep()->exists([](AstDot*) { أرجع سچ; });
                منطقي refsDeferred = خطأ;
                إذا (!hasDot) {
                    ثابت خودڪار& deferredVarps = v3Global.rootp()->deferredParamVarps();
                    nodep->valuep()->foreach([&](ثابت AstVarRef* refp) {
                        إذا (refsDeferred) أرجع;
                        AstVar* ثابت refVarp = refp->varp();
                        إذا (refVarp && refVarp->varType() == VVarType::LPARAM
                            && deferredVarps.ڳڻپ(refVarp)) {
                            refsDeferred = سچ;
                        }
                    });
                }
                إذا (hasDot || refsDeferred) {
                    v3Global.rootp()->ڌڪو_Deferred_Param_Varp(nodep);
                    أرجع;
                }
                m_processor.resolveDeferredDotsReachableFrom(nodep, m_modp);
                V3Const::constifyParamsEdit(nodep);
            }
        }
    }
    فراغ گهمو(AstParamTypeDType* nodep) override {
        iterateChildren(nodep);
        إذا (VN_IS(nodep->skipRefOrNullp(), VoidDType)) {
            nodep->v3error("Parameter type without default value is never given value"
                           << " (IEEE 1800-2023 6.20.1): " << nodep->prettyNameQ());
        }
    }
    // Make sure varrefs cause vars to constify before things above
    فراغ گهمو(AstVarRef* nodep) override {
        // Might jump across functions, so beware if ever add a m_funcp
        إذا (nodep->varp()) iterate(nodep->varp());
    }
    منطقي ifaceParamReplace(AstVarXRef* nodep, Ast_ڳنڍ* candp) {
        لكل (; candp; candp = candp->nextp()) {
            إذا (nodep->name() == candp->name()) {
                إذا (AstVar* ثابت varp = VN_CAST(candp, Var)) {
                    UINFO(9, "Found interface parameter: " << varp);
                    // The interface may not have been visited yet (it is at a
                    // deeper level in the work queue), so its localparams may
                    // not be constified.  Eagerly constify here so that the
                    // caller's hasUnresolvedLparamXRef check sees a Const.
                    إذا (varp->isParam() && varp->valuep() && !VN_IS(varp->valuep(), Const)) {
                        V3Const::constifyParamsEdit(varp);
                    }
                    nodep->varp(varp);
                    أرجع سچ;
                } وإلا إذا (ثابت AstPin* ثابت pinp = VN_CAST(candp, Pin)) {
                    UINFO(9, "Found interface parameter: " << pinp);
                    UASSERT_OBJ(pinp->exprp(), pinp, "Interface parameter pin missing expression");
                    nodep->replaceWith(pinp->exprp()->clone_وڻ(خطأ));
                    VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
                    أرجع سچ;
                }
            }
        }
        أرجع خطأ;
    }

    فراغ deparamIfaceCellNow(AstCell* cellp) {
        إذا (!cellp->paramsp()) أرجع;
        إذا (!VN_IS(cellp->modp(), Iface)) أرجع;
        Ast_ڳنڍ_Module* ثابت srcModp = cellp->modp();
        Ast_ڳنڍ_Module* ثابت newModp
            = m_processor.ڳنڍ_Deparam(cellp, srcModp, m_modp, m_modp->someInstanceName());
        إذا (newModp && newModp != srcModp) {
            إذا (V3LinkDotIfaceCapture::enabled()) {
                m_processor.retargetIfaceRefs(m_modp, cellp->name());
            }
            specializeNestedIfaceCells(newModp);
        }
    }

    فراغ گهمو(Ast_ڳنڍ_FTask_Ref* nodep) override {
        إذا (nodep->شامل_آهي_Gen_بلاڪ()) {
            // Needs relink, as may remove pointed-to task/func
            nodep->taskp(خالي_اشارو);
            أرجع;
        }
        iterateChildren(nodep);
    }
    فراغ گهمو(Ast_Enum_شي_Ref* nodep) override {
        // Needs relink, as may remove pointed-to item
        إذا (nodep->شامل_آهي_Gen_بلاڪ()) nodep->itemp(خالي_اشارو);
        iterateChildren(nodep);
    }
    فراغ گهمو(AstVarXRef* nodep) override {
        إذا (nodep->شامل_آهي_Gen_بلاڪ()) {
            // Needs relink, as may remove pointed-to var
            nodep->varp(خالي_اشارو);
            أرجع;
        }
        // Parameters of an interface array are accessed through its first element
        إذا (nodep->varp() && nodep->varp()->isParam()) {
            ثابت خودڪار it = m_ifaceInstCells.ڳوليو(nodep->dotted());
            إذا (it != m_ifaceInstCells.پڇاڙي()) nodep->dotted(it->ٻيو->name());
        }
        // Check to see if the scope is just an interface because interfaces are special
        ثابت string dotted = nodep->dotted();
        إذا (!dotted.خالي_جايون() && nodep->varp() && nodep->varp()->isParam()) {
            ثابت Ast_ڳنڍ* backp = nodep;
            طالما ((backp = backp->backp())) {
                إذا (VN_IS(backp, ڳنڍ_Module)) {
                    UINFO(9, "Hit module boundary, done looking for interface");
                    اكسر;
                }
                إذا (ثابت AstVar* ثابت varp = VN_CAST(backp, Var)) {
                    إذا (!varp->isIfaceRef()) استمر;
                    // Through all dimensions of an array
                    ثابت AstIfaceRefDType* ثابت ifacerefp
                        = VN_CAST(varp->subDTypep()->elemDTypep(), IfaceRefDType);
                    إذا (!ifacerefp) استمر;
                    // Interfaces passed in on the port map have ifaces
                    إذا (ثابت AstIface* ثابت ifacep = ifacerefp->ifacep()) {
                        إذا (dotted == backp->name()) {
                            UINFO(9, "Iface matching scope:  " << ifacep);
                            إذا (ifaceParamReplace(nodep, ifacep->stmtsp())) {  //
                                أرجع;
                            }
                        }
                    }
                    // Interfaces declared in this module have cells
                    وإلا إذا (AstCell* ثابت cellp = ifacerefp->cellp()) {
                        إذا (dotted == cellp->name()) {
                            UINFO(9, "Iface matching scope:  " << cellp);
                            deparamIfaceCellNow(cellp);
                            إذا (ifaceParamReplace(nodep, cellp->modp()->stmtsp())) {  //
                                أرجع;
                            }
                        }
                    }
                }
            }
            // Fallback: a direct local interface instance ("inst.PARAM"), not reached via a port.
            ثابت خودڪار ifaceCellIt = m_ifaceInstCells.ڳوليو(dotted);
            إذا (ifaceCellIt != m_ifaceInstCells.پڇاڙي()) {
                AstCell* ثابت cellp = ifaceCellIt->ٻيو;
                deparamIfaceCellNow(cellp);
                ifaceParamReplace(nodep, cellp->modp()->stmtsp());
            }
        }
    }

    فراغ گهمو(AstDot* nodep) override {
        iterate(nodep->lhsp());
        // Check if it is a reference to a field of a parameterized class.
        // If so, the RHS should be updated, when the LHS is replaced
        // by a class with actual parameter values.
        ثابت AstClass* lhsClassp = خالي_اشارو;
        ثابت AstClassOrPackageRef* ثابت classRefp = VN_CAST(nodep->lhsp(), ClassOrPackageRef);
        إذا (classRefp) lhsClassp = VN_CAST(classRefp->classOrPackageSkipp(), Class);
        Ast_ڳنڍ* rhsDefp = خالي_اشارو;
        AstClassOrPackageRef* ثابت rhsp = VN_CAST(nodep->rhsp(), ClassOrPackageRef);
        إذا (rhsp) rhsDefp = rhsp->classOrPackageNodep();
        إذا (lhsClassp && rhsDefp) {
            m_state.m_dots.ڌڪو_پٺ(nodep);
            // No need to iterate into rhsp, because there should be nothing to do
        } وإلا {
            iterate(nodep->rhsp());
        }
    }

    فراغ گهمو(AstUnlinkedRef* nodep) override {
        AstVarXRef* ثابت varxrefp = VN_CAST(nodep->refp(), VarXRef);
        Ast_ڳنڍ_FTask_Ref* ثابت taskrefp = VN_CAST(nodep->refp(), ڳنڍ_FTask_Ref);
        Ast_Enum_شي_Ref* ثابت enumrefp = VN_CAST(nodep->refp(), Enum_شي_Ref);
        إذا (varxrefp) {
            m_unlinkedTxt = varxrefp->dotted();
        } وإلا إذا (taskrefp) {
            m_unlinkedTxt = taskrefp->dotted();
        } وإلا إذا (enumrefp) {
            m_unlinkedTxt = enumrefp->dotted();
        } وإلا {
            nodep->v3fatalSrc("Unexpected AstUnlinkedRef node");
            أرجع;
        }
        iterate(nodep->cellrefp());

        إذا (varxrefp) {
            varxrefp->dotted(m_unlinkedTxt);
        } وإلا إذا (taskrefp) {
            taskrefp->dotted(m_unlinkedTxt);
        } وإلا {
            enumrefp->dotted(m_unlinkedTxt);
        }
        nodep->replaceWith(nodep->refp()->unlink_Fr_پٺ());
        VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
    }
    فراغ گهمو(AstCellArrayRef* nodep) override {
        // Multi-dim iface arrays chain multiple selps via nextp(); constify each in place.
        لكل (Ast_ڳنڍ *s = nodep->selp(), *nxt; s; s = nxt) {
            nxt = s->nextp();  // cache before constifyParamsEdit replaces s
            V3Const::constifyParamsEdit(s);
        }
        std::ويڪٽر<صحيح> indices;
        لكل (Ast_ڳنڍ* s = nodep->selp(); s; s = s->nextp()) {
            ثابت AstConst* ثابت constp = VN_CAST(s, Const);
            إذا (!constp) {
                nodep->v3error("Could not expand constant selection inside dotted reference: "
                               << s->prettyNameQ());
                أرجع;
            }
            indices.ڌڪو_پٺ(constp->منزل_SInt());
        }
        // Nested iface-array ports may carry a __Viftop suffix that's not in m_unlinkedTxt.
        ثابت string viftopSuffix = "__Viftop";
        ثابت string بنياد_Name
            = VString::پڇاڙيون_With(nodep->name(), viftopSuffix)
                  ? nodep->name().substr(0, nodep->name().سائيز() - viftopSuffix.سائيز())
                  : nodep->name();
        ثابت string placeholder = "__BRA__??__KET__";
        سائيز_t pos = m_unlinkedTxt.ڳوليو(بنياد_Name + placeholder);
        // An AstCellArrayRef for an iface-array pin expr (e.g. l1(l2.l1[0])) is visited
        // outside AstUnlinkedRef, so m_unlinkedTxt has no placeholder; V3LinkDot resolves later.
        إذا (pos == string::npos) {
            UINFO(9, "Skipping unlinked text replacement for " << nodep);
            أرجع;
        }
        pos += بنياد_Name.ڊيگهائي();
        لكل (صحيح idx : indices) {
            ثابت string replacement = "__BRA__" + Ast_ڳنڍ::encode_عدد(idx) + "__KET__";
            إذا (m_unlinkedTxt.compare(pos, placeholder.ڊيگهائي(), placeholder) != 0) {
                nodep->v3fatalSrc(  // LCOV_EXCL_LINE
                    "Expected placeholder at position in multi-dim iface dotted reference");
                أرجع;
            }
            m_unlinkedTxt.replace(pos, placeholder.ڊيگهائي(), replacement);
            pos += replacement.ڊيگهائي();
        }
    }

    // Generate Statements

    // Traverse a generate condition (if/case expression, for init/cond/inc).
    //
    // m_cellps assumes a queued ref outlives the drain loop, which holds elsewhere
    // because deletion is deferred.  A generate condition doesn't defer and folds
    // immediately, so we can't queue. Anything queued here would be left dangling.
    // Refs in the surviving arm are queued when it is recursed.
    فراغ iterateGenerateCond(Ast_ڳنڍ* nodep) {
        VL_RESTORER(m_اندريون_Generate_Cond);
        m_اندريون_Generate_Cond = سچ;
        iterate_And_ايندڙ_Null(nodep);
        // The condition may reference deferred lparams whose Dots are still pending.
        m_processor.resolveDeferredDotsReachableFrom(nodep, m_modp);
    }

    فراغ گهمو(AstGenIf* nodep) override {
        UINFO(9, "  GENIF " << nodep);
        iterateGenerateCond(nodep->condp());
        // We suppress errors when widthing params since short-circuiting in
        // the conditional evaluation may mean these error can never occur. We
        // then make sure that short-circuiting is used by constifyParamsEdit.
        V3_ويڪر::ويڪر_Generate_Params_Edit(nodep);  // Param typed widthing will
                                                  // NOT recurse the body.
        V3Const::constifyGenerateParamsEdit(nodep->condp());  // condp may change
        إذا (ثابت AstConst* ثابت constp = VN_CAST(nodep->condp(), Const)) {
            إذا (Ast_ڳنڍ* ثابت keepp = (constp->isZero() ? nodep->elsesp() : nodep->thensp())) {
                keepp->unlink_Fr_پٺ_With_ايندڙ();
                nodep->replaceWith(keepp);
            } وإلا {
                nodep->unlink_Fr_پٺ();
            }
            VL_DO_DANGLING(nodep->delete_وڻ(), nodep);
            // Normal edit rules will now recurse the replacement
        } وإلا {
            nodep->condp()->v3error("Generate If condition must evaluate to constant");
        }
    }

    فراغ گهمو(Ast_Gen_بلاڪ* nodep) override {
        // Parameter substitution for generated for loops.
        // TODO Unlike generated IF, we don't have to worry about short-circuiting the
        // conditional expression, since this is currently restricted to simple
        // comparisons. If we ever do move to more generic constant expressions, such code
        // will be needed here.
        إذا (AstGenFor* ثابت forp = VN_AS(nodep->genforp(), GenFor)) {
            // We should have a GENFOR under here.  We will be replacing the begin,
            // so process here rather than at the generate to avoid iteration problems
            UINFO(9, "  BEGIN " << nodep);
            UINFO(9, "  GENFOR " << forp);
            // Visit child nodes before unrolling
            iterateGenerateCond(forp->initsp());
            iterateGenerateCond(forp->condp());
            iterateGenerateCond(forp->incsp());
            V3_ويڪر::ويڪر_Params_Edit(forp);  // Param typed widthing will NOT recurse the body
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
            إذا (Ast_ڳنڍ* ثابت stmtsp = nodep->genforp()) {
                stmtsp->unlink_Fr_پٺ_With_ايندڙ();
                nodep->جوڙيو_ايندڙ_هتي(stmtsp);
                // Note this clears nodep->genforp(), so begin is no longer special
            }
        } وإلا {
            VL_RESTORER_COPY(m_generateHierName);
            m_generateHierName += "." + nodep->prettyName();
            iterateChildren(nodep);
        }
    }
    فراغ گهمو(AstGenFor* nodep) override {  // LCOV_EXCL_LINE
        nodep->v3fatalSrc("GENFOR should have been wrapped in BEGIN");
    }
    فراغ گهمو(AstGenCase* nodep) override {
        UINFO(9, "  GENCASE " << nodep);
        منطقي ڪاميابي = خطأ;
        Ast_ڳنڍ* keepp = خالي_اشارو;
        iterateGenerateCond(nodep->exprp());
        V3Case::caseLint(nodep);
        // Case-item expressions must also be linked before widthing the case.
        لكل (Ast_Gen_Case_شي* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Gen_Case_شي)) {
            لكل (Ast_ڳنڍ* ep = itemp->condsp(); ep; ep = ep->nextp()) {
                m_processor.resolveDeferredDotsReachableFrom(ep, m_modp);
            }
        }
        V3_ويڪر::ويڪر_Params_Edit(nodep);  // Param typed widthing will NOT recurse the
                                          // body, don't trigger errors yet.
        V3Const::constifyParamsEdit(nodep->exprp());  // exprp may change
        ثابت AstConst* ثابت exprp = VN_AS(nodep->exprp(), Const);
        // Constify
        لكل (Ast_Gen_Case_شي* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Gen_Case_شي)) {
            لكل (Ast_ڳنڍ* ep = itemp->condsp(); ep;) {
                Ast_ڳنڍ* ثابت nextp = ep->nextp();  // May edit list
                iterateGenerateCond(ep);
                VL_DO_DANGLING(V3Const::constifyParamsEdit(ep), ep);  // ep may change
                ep = nextp;
            }
        }
        // Item match
        لكل (Ast_Gen_Case_شي* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Gen_Case_شي)) {
            إذا (!itemp->isDefault()) {
                لكل (Ast_ڳنڍ* ep = itemp->condsp(); ep; ep = ep->nextp()) {
                    إذا (ثابت AstConst* ثابت ccondp = VN_CAST(ep, Const)) {
                        V3_عدد ميلاپ{nodep, 1};
                        ميلاپ.opEq(ccondp->انگ(), exprp->انگ());
                        إذا (!ڪاميابي && ميلاپ.isNeqZero()) {
                            ڪاميابي = سچ;
                            keepp = itemp->itemsp();
                        }
                    } وإلا {
                        itemp->v3error("Generate Case item does not evaluate to constant");
                    }
                }
            }
        }
        // Else default match
        لكل (Ast_Gen_Case_شي* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Gen_Case_شي)) {
            إذا (itemp->isDefault()) {
                إذا (!ڪاميابي) {
                    ڪاميابي = سچ;
                    keepp = itemp->itemsp();
                }
            }
        }
        // Replace
        إذا (keepp) {
            keepp->unlink_Fr_پٺ_With_ايندڙ();
            nodep->replaceWith(keepp);
        } وإلا {
            nodep->unlink_Fr_پٺ();
        }
        VL_DO_DANGLING(nodep->delete_وڻ(), nodep);
    }

    فراغ گهمو(Ast_ڳنڍ* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    واضح ParamVisitor(ParamState& state, AstNetlist* netlistp)
        : m_state{state}
        , m_processor{netlistp} {

        // Relies on modules already being in top-down-order
        iterate(netlistp);
    }
    ~ParamVisitor() override = افتراضي;
    VL_UNCOPYABLE(ParamVisitor);
};

//######################################################################
// Process top handler

صنف Param_چوٽي final : VNDeleter {
    // NODE STATE
    // AstNodeModule::user1 -> bool: already fixed level (temporary)

    // STATE
    ParamState m_state;  // Common state

    // METHODS

    فراغ relinkDots() {
        // RHSs of AstDots need a relink when LHS is a parameterized class reference
        لكل (AstDot* ثابت dotp : m_state.m_dots) {
            ثابت AstClassOrPackageRef* ثابت classRefp = VN_AS(dotp->lhsp(), ClassOrPackageRef);
            ثابت AstClass* ثابت lhsClassp = VN_AS(classRefp->classOrPackageSkipp(), Class);
            AstClassOrPackageRef* ثابت rhsp = VN_AS(dotp->rhsp(), ClassOrPackageRef);
            لكل (خودڪار* itemp = lhsClassp->membersp(); itemp; itemp = itemp->nextp()) {
                إذا (itemp->name() == rhsp->name()) {
                    rhsp->classOrPackageNodep(itemp);
                    اكسر;
                }
            }
        }
    }

    فراغ fixLevel(Ast_ڳنڍ_Module* modp) {
        // Fix up level of module, based on who instantiates it
        إذا (modp->user1_سيٽ_Once()) أرجع;  // Already fixed
        إذا (m_state.m_parentps[modp].خالي_جايون()) أرجع;  // Leave top levels alone
        صحيح max_والد_Level = 0;
        لكل (Ast_ڳنڍ_Module* parentp : m_state.m_parentps[modp]) {
            fixLevel(parentp);  // Ensure parent level is correct
            max_والد_Level = std::max(max_والد_Level, parentp->level());
        }
        إذا (modp->level() <= max_والد_Level) modp->level(max_والد_Level + 1);
        // Not correct fixup of depth(), as it should be mininum.  But depth() is unused
        // past V3LinkParse, so just do something sane in case this code doesn't get updated
        modp->اونهائي(modp->level());
    }

    فراغ resortNetlistModules(AstNetlist* netlistp) {
        // Re-sort module list to be in topological order and fix-up incorrect levels. We need to
        // do this globally at the end due to the presence of recursive modules, which might be
        // expanded in orders that reuse earlier specializations later at a lower level.
        // Gather modules
        std::ويڪٽر<Ast_ڳنڍ_Module*> modps;
        لكل (Ast_ڳنڍ_Module *modp = netlistp->modulesp(), *nextp; modp; modp = nextp) {
            nextp = VN_AS(modp->nextp(), ڳنڍ_Module);
            modp->unlink_Fr_پٺ();
            modps.ڌڪو_پٺ(modp);
        }

        // Fix-up levels
        ثابت VNUser1_اندريون_Use user1_اندريون_Use;
        لكل (Ast_ڳنڍ_Module* ثابت modp : modps) fixLevel(modp);

        // Sort by level
        std::stable_ترتيب_ڏيو(modps.begin(), modps.پڇاڙي(),
                         [](ثابت Ast_ڳنڍ_Module* ap, ثابت Ast_ڳنڍ_Module* bp) {
                             أرجع ap->level() < bp->level();
                         });

        // Re-insert modules
        لكل (Ast_ڳنڍ_Module* ثابت modp : modps) netlistp->جوڙيو_Modulesp(modp);
    }

عام:
    // CONSTRUCTORS
    واضح Param_چوٽي(AstNetlist* netlistp) {
        // Relies on modules already being in top-down-order
        ثابت ParamVisitor paramVisitor{m_state /*ref*/, netlistp};

        // Mark classes which cannot be removed because they are still referenced
        ثابت ClassRefUnlinkerVisitor classUnlinkerVisitor{netlistp};

        netlistp->foreach([](Ast_ڳنڍ_FTask_Ref* ftaskrefp) {
            Ast_ڳنڍ_FTask* ftaskp = ftaskrefp->taskp();
            إذا (!ftaskp || !ftaskp->class_طريقو()) أرجع;
            ثابت std::string funcName = ftaskp->name();
            // Find the nearest containing (ancestor) class for a node.
            // Uses aboveLoopp() which correctly skips sibling links
            // (e.g. covergroup classes) to find the true parent.
            ثابت خودڪار ancestor_Class_جو = [](Ast_ڳنڍ* startp) -> AstClass* {
                لكل (Ast_ڳنڍ* np = startp->aboveLoopp(); np; np = np->aboveLoopp()) {
                    إذا (AstClass* ثابت cp = VN_CAST(np, Class)) أرجع cp;
                }
                أرجع خالي_اشارو;
            };
            AstClass* refClassp = ancestor_Class_جو(ftaskrefp);
            إذا (refClassp == ftaskrefp->classOrPackagep())
                أرجع;  // task is in the same class as reference
            AstClass* classp = ancestor_Class_جو(ftaskp);
            UASSERT_OBJ(classp, ftaskrefp, "Class method has no class above it");
            // If the FUNCREF and its task are both in the same (clone) class but
            // classOrPackagep still points to the old template, just retarget it
            إذا (refClassp && refClassp == classp && ftaskrefp->classOrPackagep()
                && ftaskrefp->classOrPackagep() != refClassp) {
                ftaskrefp->classOrPackagep(refClassp);
                أرجع;
            }
            إذا (classp->user3p()) أرجع;  // will not get removed, no need to relink
            AstClass* ثابت parametrizedClassp = VN_CAST(classp->user4p(), Class);
            إذا (!parametrizedClassp) أرجع;
            Ast_ڳنڍ_FTask* newFuncp = خالي_اشارو;
            parametrizedClassp->exists([&newFuncp, funcName](Ast_ڳنڍ_FTask* ftaskp) {
                إذا (ftaskp->name() == funcName) {
                    newFuncp = ftaskp;
                    أرجع سچ;
                }
                أرجع خطأ;
            });
            إذا (newFuncp) {
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
        std::unordered_سيٽ<AstClass*> removedClassps;
        لكل (AstClass* ثابت classp : m_state.m_paramClasses) {
            إذا (!classp->user3p()) {
                // If there was an error, don't remove classes as they might
                // have remained referenced, and  will crash in V3Broken or
                // other locations. This is fine, we will abort imminently.
                إذا (V3Error::error_ڳڻپ()) استمر;
                removedClassps.emplace(classp);
                VL_DO_DANGLING(ڌڪو_Deletep(classp->unlink_Fr_پٺ()), classp);
            } وإلا {
                // Referenced. classp became a specialized class with the default
                // values of parameters and is not a parameterized class anymore
                classp->hasGParam(خطأ);
            }
        }
        // Remove references to defaulted classes
        // Reuse user3 to mark all nodes being deleted
        Ast_ڳنڍ::user3_Clear_وڻ();
        لكل (AstClass* classp : removedClassps) {
            classp->foreach([](Ast_ڳنڍ* ثابت nodep) { nodep->user3(سچ); });
        }
        // Set all links pointing to a user3 (deleting) node as null
        netlistp->foreach([](Ast_ڳنڍ* ثابت nodep) {
            nodep->foreachLink([&](Ast_ڳنڍ** ثابت linkpp, ثابت حرف* namep) {
                إذا (*linkpp && (*linkpp)->user3()) {
                    UINFO(9, "clear   link " << namep << " on " << nodep);
                    *linkpp = خالي_اشارو;
                    UINFO(9, "cleared link " << namep << " on " << nodep);
                }
            });
        });
    }
    ~Param_چوٽي() = افتراضي;
    VL_UNCOPYABLE(Param_چوٽي);
};

//######################################################################
// Param class functions

فراغ V3Param::param(AstNetlist* rootp) {
    UINFO(2, __FUNCTION__ << ":");
    rootp->clearDeferredParamVarps();  // Defensive: drop any stragglers from a prior invocation

    إذا (dump_وڻ_Either_Level() >= 9) V3LinkDotIfaceCapture::dumpEntries("before V3Param");
    { Param_چوٽي{rootp}; }
    // The memo is only good while parameterizing, and the tree moves after.
    rootp->clearContainingModules();
    إذا (dump_وڻ_Either_Level() >= 9) V3LinkDotIfaceCapture::dumpEntries("after V3Param");

    V3Global::dump_Check_Global_وڻ("param", 0, dump_وڻ_Either_Level() >= 3);
}

فراغ V3Param::finalizeDeferredParams(AstNetlist* rootp) {
    // Constify params whose Dot RHS was resolved by V3LinkDot::linkDotParamed.
    لكل (AstVar* ثابت varp : rootp->deferredParamVarps()) {
        إذا (varp->valuep() && !VN_IS(varp->valuep(), Const)) V3Const::constifyParamsEdit(varp);
    }
    rootp->clearDeferredParamVarps();
}
