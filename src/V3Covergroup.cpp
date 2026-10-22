// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Functional coverage implementation
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
// FUNCTIONAL COVERAGE TRANSFORMATIONS:
//      For each covergroup (AstClass with isCovergroup()):
//          For each coverpoint (AstCoverpoint):
//              Generate member variable for VerilatedCoverpoint
//              Generate initialization in constructor
//              Generate sample code in sample() method
//
//*************************************************************************

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3Covergroup.h"

#ضمن "V3Const.h"
#ضمن "V3Error.h"
#ضمن "V3File.h"
#ضمن "V3MemberMap.h"
#ضمن "V3UniqueNames.h"

#ضمن <bitset>
#ضمن <cmath>
#ضمن <deque>
#ضمن <set>
#ضمن <tuple>
#ضمن <unordered_map>
#ضمن <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Embedded covergroup assignment validation; and in the same pass over the netlist, the
// type_option variables through which SystemVerilog may set type_option.merge_instances

صنف Covergroup_تخصیص_بده_Valid_Visitor final : عام VNVisitorConst {
    VMemberMap m_memberMap;
    std::map<ثابت AstVar*, ثابت Ast_گره_FTask*>
        m_constructors;  // Implicit instance -> constructor
    std::مجموعه<ثابت AstVar*> m_mergeableTypeOptions;  // See mergeableTypeOptions()
    ثابت Ast_گره_FTask* m_ftaskp = اشاره_تهی;
    منطقي m_collecting = درست;
    منطقي m_valid = درست;

    فراغ بازدید(AstClass* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = اشاره_تهی;
        إذا (m_collecting) {
            ثابت Ast_گره_FTask* ثابت constructorp
                = VN_CAST(m_memberMap.پیدا_کن_Member(nodep, "new"), گره_FTask);
            لكل (ثابت Ast_گره* itemp = nodep->membersp(); itemp; itemp = itemp->nextp()) {
                ثابت AstVar* ثابت varp = VN_CAST(itemp, Var);
                // Only the implicit instance is restricted, not explicitly typed aliases.
                إذا (!varp || !varp->isClassMember() || varp->isDeclTyped()) استمر;
                ثابت AstClassRefDType* ثابت refp
                    = VN_CAST(varp->dtypep()->skipRefp(), ClassRefDType);
                إذا (refp && refp->classp()->covergroupEnclosingClassp() == nodep) {
                    m_constructors.emplace(varp, constructorp);
                }
            }
        }
        iterateChildrenConst(nodep);
    }
    فراغ بازدید(Ast_گره_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildrenConst(nodep);
    }
    فراغ بازدید(Ast_گره_تخصیص_بده* nodep) override {
        إذا (!m_collecting) {
            ثابت AstVar* varp = اشاره_تهی;
            إذا (ثابت Ast_گره_Var_Ref* ثابت refp = VN_CAST(nodep->lhsp(), گره_Var_Ref)) {
                varp = refp->varp();
            } وإلا إذا (ثابت AstMemberSel* ثابت selp = VN_CAST(nodep->lhsp(), MemberSel)) {
                varp = selp->varp();
            }
            ثابت خودکار it = m_constructors.پیدا_کن(varp);
            إذا (it != m_constructors.انتها() && (!m_ftaskp || m_ftaskp != it->دومی)) {
                m_valid = خطأ;
                nodep->v3error("Embedded covergroup variable "
                               << varp->prettyNameQ()
                               << " may only be assigned in the enclosing class's 'new' method "
                                  "(IEEE 1800-2023 19.4).");
            }
        }
        iterateChildrenConst(nodep);
    }
    فراغ بازدید(Ast_گره_Var_Ref* nodep) override {
        // Type options may be set at any time (IEEE 1800-2023 19.7.1), so SystemVerilog may set
        // type_option.merge_instances with any use of a type_option but a read of another
        // member, as not every write is marked yet (std::randomize)
        إذا (m_collecting && nodep->varp()->name() == "type_option") {
            ثابت AstStructSel* ثابت selp = VN_CAST(nodep->backp(), StructSel);
            إذا (!selp || selp->fromp() != nodep || selp->name() == "merge_instances"
                || !nodep->access().isReadOnly()) {
                m_mergeableTypeOptions.درج(nodep->varp());
            }
        }
        iterateChildrenConst(nodep);
    }
    فراغ بازدید(Ast_گره* nodep) override { iterateChildrenConst(nodep); }

عام:
    صریح Covergroup_تخصیص_بده_Valid_Visitor(AstNetlist* nodep) {
        // Uses can precede their enclosing class in the tree.
        iterateConst(nodep);
        m_collecting = خطأ;
        إذا (!m_constructors.خالی()) iterateConst(nodep);
    }
    منطقي valid() ثابت { أرجع m_valid; }
    // type_option variables through which SystemVerilog may set type_option.merge_instances, so
    // that covergroups are tested with optionVar(true)
    ثابت std::مجموعه<ثابت AstVar*>& mergeableTypeOptions() ثابت { أرجع m_mergeableTypeOptions; }
};

//######################################################################
// Covergroup expression validation visitor

صنف Covergroup_عبارت_Valid_Visitor final : عام VNVisitor {
    ثابت std::مجموعه<ثابت AstVar*>& m_sampleMembers;
    ثابت std::مجموعه<ثابت AstVar*>& m_constructorRefMembers;
    منطقي m_درون_Coverage_عبارت = خطأ;
    منطقي m_sampleFormalAllowed = خطأ;

    فراغ پیمایش_Sample_عبارت(Ast_گره* nodep) {
        إذا (!nodep) أرجع;
        VL_RESTORER(m_sampleFormalAllowed);
        m_sampleFormalAllowed = درست;
        iterate_And_بعدی_Null(nodep);
    }

    فراغ پیمایش_Coverage_عبارت(Ast_گره* nodep) {
        إذا (!nodep) أرجع;
        VL_RESTORER(m_درون_Coverage_عبارت);
        m_درون_Coverage_عبارت = درست;
        iterate_And_بعدی_Null(nodep);
    }

    فراغ بازدید(AstCoverpoint* nodep) override {
        پیمایش_Sample_عبارت(nodep->exprp());
        پیمایش_Sample_عبارت(nodep->iffp());
        iterate_And_بعدی_Null(nodep->binsp());
        iterate_And_بعدی_Null(nodep->optionsp());
    }
    فراغ بازدید(AstCoverCross* nodep) override {
        iterate_And_بعدی_Null(nodep->itemsp());
        پیمایش_Sample_عبارت(nodep->iffp());
        iterate_And_بعدی_Null(nodep->optionsp());
        iterate_And_بعدی_Null(nodep->binsp());
    }
    فراغ بازدید(AstCoverCrossBin* nodep) override {
        iterate_And_بعدی_Null(nodep->selectp());
        پیمایش_Sample_عبارت(nodep->iffp());
    }
    فراغ بازدید(AstCoverCrossWith* nodep) override {
        iterate_And_بعدی_Null(nodep->selectp());
        پیمایش_Coverage_عبارت(nodep->filterp());
        پیمایش_Coverage_عبارت(nodep->countp());
    }
    فراغ بازدید(AstCoverBinsof* nodep) override { پیمایش_Coverage_عبارت(nodep->rangesp()); }
    فراغ بازدید(AstCoverBin* nodep) override {
        پیمایش_Coverage_عبارت(nodep->rangesp());
        پیمایش_Sample_عبارت(nodep->iffp());
        پیمایش_Coverage_عبارت(nodep->arraySizep());
        پیمایش_Coverage_عبارت(nodep->transp());
    }
    فراغ بازدید(AstVarRef* nodep) override {
        إذا (!m_sampleFormalAllowed && m_sampleMembers.شمار(nodep->varp())) {
            nodep->v3error("Covergroup sample formal argument "
                           << nodep->varp()->prettyNameQ()
                           << " may only be used in a coverpoint or conditional guard "
                              "expression (IEEE 1800-2023 19.8.1).");
        }
        إذا (m_درون_Coverage_عبارت && m_constructorRefMembers.شمار(nodep->varp())) {
            nodep->v3error("Ref covergroup constructor formal argument "
                           << nodep->varp()->prettyNameQ()
                           << " may not be used in a covergroup expression "
                              "(IEEE 1800-2023 19.5).");
        }
    }
    فراغ بازدید(Ast_گره_FTask_Ref* nodep) override {
        إذا (m_درون_Coverage_عبارت && nodep->taskp()) {
            منطقي invalidDirection = خطأ;
            لكل (Ast_گره* stmtp = nodep->taskp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                ثابت AstVar* ثابت varp = VN_CAST(stmtp, Var);
                إذا (varp && varp->isIO() && varp->isWritable()) {
                    invalidDirection = درست;
                    اكسر;
                }
            }
            إذا (invalidDirection) {
                nodep->v3error("Function " << nodep->taskp()->prettyNameQ()
                                           << " called in a covergroup expression has an "
                                              "output, inout, or non-const ref argument "
                                              "(IEEE 1800-2023 19.5).");
            }
        }
        iterateChildren(nodep);
    }
    فراغ بازدید(Ast_گره* nodep) override { iterateChildren(nodep); }

عام:
    Covergroup_عبارت_Valid_Visitor(ثابت std::مجموعه<ثابت AstVar*>& sampleMembers,
                               ثابت std::مجموعه<ثابت AstVar*>& constructorRefMembers)
        : m_sampleMembers{sampleMembers}
        , m_constructorRefMembers{constructorRefMembers} {}
    فراغ پیمایش(Ast_گره* nodep) { iterate(nodep); }
};

//######################################################################
// Bins of one declaration whose values are computed rather than listed: bin k covers
// [m_lo + k * m_stride, m_lo + (k + 1) * m_stride - 1], and the last bin extends to m_hi.  An
// array bin element is a run of single-value bins; automatic bins partition the coverpoint
// domain.  Bounds are coverpoint values at FunctionalCoverageVisitor::runWidth(),
// sign-extended like a CrossValueRange's.

صنف Bin_پیاپی final {
عام:
    // MEMBERS
    uint32_t m_شمار;  // Number of bins
    V3_عدد m_lo;  // Lowest value of the first bin
    V3_عدد m_stride;  // Number of values of each bin but the last
    V3_عدد m_hi;  // Highest value of the last bin
    منطقي m_خالی = خطأ;  // A single bin without a value of the coverpoint type
    uint32_t m_declared = 0;  // Runtime index of the first bin, once generated

    // CONSTRUCTORS
    Bin_پیاپی(Ast_گره* nodep, صحيح عرض, uint32_t شمار)
        : m_شمار{شمار}
        , m_lo{nodep, عرض}
        , m_stride{nodep, عرض, 1}
        , m_hi{nodep, عرض} {}
};

//######################################################################
// Functional coverage visitor

صنف FunctionalCoverageVisitor final : عام VNVisitor {
    // NODE STATE
    // Entire netlist:
    //  AstCoverpoint::user1p()  -> AstVar*.  Previous-value variable for transition bins
    //  AstCoverpoint::user2()   -> bool.  Had a bins declaration ignored, so no automatic bins
    ثابت VNUser1_درون_Use m_inuser1;
    ثابت VNUser2_درون_Use m_inuser2;

    // STATE
    std::مجموعه<AstCoverpoint*>
        m_runtimePoints;  // Points needing value metadata and live-bin mapping
    std::مجموعه<AstCoverCross*> m_runtimeCrosses;  // Crosses over finalized live-bin dimensions
    std::map<AstVar*, AstVar*> m_excludedVars;  // Sample-time state-exclusion flags
    AstClass* m_covergroupp = اشاره_تهی;  // Current covergroup being processed
    Ast_گره_Module* m_unitp = اشاره_تهی;  // Design unit declaring the current class, outside classes
    AstClass* m_enclosingClassp = اشاره_تهی;  // Class lexically enclosing the covergroup, if any
    AstVar* m_embeddedVarp = اشاره_تهی;  // Embedded covergroup member of m_enclosingClassp, if any
    std::string m_covergroupName;  // Current covergroup's type name, see covergroupTypeName()
    AstFunc* m_sampleFuncp = اشاره_تهی;  // Current sample() function
    AstIf* m_cpGuardp = اشاره_تهی;  // Current coverpoint's iff guard, holding its sampling
    AstFunc* m_constructorp = اشاره_تهی;  // Current constructor
    std::وکتور<AstCoverpoint*> m_coverpoints;  // Coverpoints in current covergroup
    std::map<std::string, AstCoverpoint*> m_coverpointMap;  // Name -> coverpoint for fast lookup
    std::وکتور<AstCoverCross*> m_coverCrosses;  // Cross coverage items in current covergroup
    std::وکتور<Ast_Cg_Option_تخصیص_بده*> m_cgOptions;  // Covergroup-level options, before lowering
    uint32_t m_cg_Type_وزن = 1;  // The covergroup's type_option.weight, a constant
    منطقي m_cg_ادغام_Instances = خطأ;  // The covergroup's type_option.merge_instances, a constant
    // The covergroup's type_option.merge_instances is, or SystemVerilog may make it, true
    منطقي m_cg_May_ادغام = خطأ;
    // type_option members through which SystemVerilog may set type_option.merge_instances,
    // see CovergroupAssignValidVisitor::mergeableTypeOptions()
    ثابت std::مجموعه<ثابت AstVar*>& m_mergeableTypeOptions;
    صحيح m_cg_At_کمترین = 1;  // Covergroup option.at_least

    بنية EmbeddedEventTrigger final {
        File_سطر* eventFl;  // Clocking-event source location
        AstVar* پایه_Varp;  // Base enclosing-class member in the event expression
        AstVar* memberVarp;  // Selected member in a 'base.member' expression, or nullptr
        VEdgeType یال_Type;  // Clocking-event edge qualifier
        AstVar* prevVarp;  // Member containing the previous event value, or nullptr
        EmbeddedEventTrigger(File_سطر* eventFl, AstVar* پایه_Varp, AstVar* memberVarp,
                             VEdgeType یال_Type)
            : eventFl{eventFl}
            , پایه_Varp{پایه_Varp}
            , memberVarp{memberVarp}
            , یال_Type{یال_Type}
            , prevVarp{اشاره_تهی} {}
    };

    std::مجموعه<std::string> m_crossed_Cp_نامها;  // Coverpoints referenced by a cross
    std::map<std::string, AstVar*> m_cpVarMap;  // Coverpoint name -> its VlCoverpoint member
    بنية BinRuns final {
        std::وکتور<Bin_پیاپی> runs;  // Runs of an array or automatic bins declaration, in order
        uint32_t شمار = 0;  // Bins across all runs
        منطقي unsupported = خطأ;  // Too many bins, or invalid: the declaration is ignored
        // The value of each bin, which names it, of a wildcard array; else the bins are indexed
        std::وکتور<std::string> مقادیر;
        // Too many values of an ignore or illegal wildcard array, which is then one bin
        منطقي single = خطأ;
    };
    بنية Cross_Bin_مقادیر final {
        AstCoverBin* binp;  // Declaration owning this Normal bin
        Ast_گره_عبارت* valuep;  // Individual array-bin value, or nullptr for a scalar bin
        ثابت Bin_پیاپی* runp = اشاره_تهی;  // Run computing the bin's values, if any
        uint32_t element = 0;  // Index of the bin within runp
    };
    بنية Bin_بازه final {
        uint32_t اولی = 0;  // First Normal index of the bin declaration
        uint32_t شمار = 0;  // Number of Normal bins of the declaration
        uint32_t declared = 0;  // First runtime bin index, across all bin kinds
        int32_t sized = -1;  // Index of the sized array, placed at construction; or -1
    };
    بنية CoverpointBins final {
        uint32_t مجموع = 0;  // Number of Normal bins
        Ast_گره_عبارت* exprp = اشاره_تهی;  // Sampled expression, for the value domain
        منطقي crossed = خطأ;  // Feeds a cross, which needs 'values'
        std::وکتور<Cross_Bin_مقادیر> مقادیر;  // Values in runtime Normal-bin index order
        std::unordered_map<std::string, Bin_بازه> spans;  // Declared bin name -> index span
        Bin_بازه implicitAuto{0, 0, 0};  // Implicit automatic bins, each named 'auto_<i>'
        std::deque<Bin_پیاپی> runs;  // Runs 'values' refers to
    };
    std::map<AstVar*, CoverpointBins> m_cpBins;  // Runtime coverpoint -> binsof index ranges
    // Names of the bins declarations each coverpoint ignored, which binsof selects as no bins
    std::map<ثابت AstCoverpoint*, std::وکتور<std::string>> m_droppedBins;
    // Prefixes of the constructor temporaries of constructed bins, as coverpoint and bin names
    // alone may repeat: coverpoint 'a_' bins 'b', and coverpoint 'a' bins '_b'
    V3_Unique_نامها m_sized_نامها{"__Vsized"};
    std::وکتور<Ast_گره_عبارت*> m_detached_مقادیر;  // Array-bin values m_cpBins refers to
    std::مجموعه<AstCoverCross*>
        m_droppedCrosses;  // Crosses with a bare-variable item: drop (COVERIGN)
    std::map<uint32_t, AstCoverpointDType*> m_cpDTypes;  // Hit-list bound -> interned dtype
    استفاده CrossShape = std::tuple<uint32_t, uint32_t, uint32_t, uint32_t, uint64_t, منطقي>;
    std::map<CrossShape, AstCoverCrossDType*> m_cxDTypes;
    AstVar* m_cgInstVarp = اشاره_تهی;  // __Vcg_inst handle member of the current covergroup

    VMemberMap m_memberMap;  // Member names cached for fast lookup

    // METHODS
    // The covergroup's 'option' or static 'type_option' member (V3LinkParse creates both)
    AstVar* optionVar(منطقي typeOption) {
        AstVar* ثابت varp = VN_AS(
            m_memberMap.پیدا_کن_Member(m_covergroupp, typeOption ? "type_option" : "option"), Var);
        UASSERT_OBJ(varp, m_covergroupp, "Covergroup missing option member");
        أرجع varp;
    }

    // 'option.<member>' or 'type_option.<member>', per optionVarp
    AstStructSel* newOptionSel(File_سطر* fl, AstVar* optionVarp, ثابت std::string& member,
                               VAccess access) {
        ثابت AstMemberDType* ثابت memberp
            = VN_AS(m_memberMap.پیدا_کن_Member(optionVarp->dtypep()->skipRefp(), member), MemberDType);
        UASSERT_OBJ(memberp, optionVarp, "Coverage option structure missing '" << member << "'");
        Ast_گره_عبارت* ثابت fromp = optionVarp->lifetime().isStatic()
                                       ? جديد AstVarRef{fl, optionVarp, access}
                                       : memberRef(fl, optionVarp, access);
        AstStructSel* ثابت selp = جديد AstStructSel{fl, fromp, member};
        selp->dtypep(memberp->subDTypep()->skip_Ref_مقصد_Enump());
        selp->did_عرض(درست);
        أرجع selp;
    }

    // A constant option.at_least; a negative count of hits has no meaning, so takes the default
    ساكن صحيح const_At_کمترین(ثابت AstConst* constp) {
        ثابت صحيح مقدار = constp->مقصد_SInt();
        إذا (مقدار >= 0) أرجع مقدار;
        constp->v3warn(COVERIGN, "Ignoring negative 'option.at_least'; using 1");
        أرجع 1;
    }

    // An option's constant value, or nullptr after a warning, as non-constant is unsupported
    ساكن ثابت AstConst* constOption(ثابت Ast_گره* valuep, VCoverOptionType optType) {
        إذا (ثابت AstConst* ثابت constp = VN_CAST(valuep, Const)) أرجع constp;
        valuep->v3warn(COVERIGN, "Ignoring unsupported: non-constant 'option."
                                     << optType.ascii() << "'; using default value");
        أرجع اشاره_تهی;
    }

    // Store the covergroup-level options (IEEE 1800-2023 19.7) where SystemVerilog and the
    // runtime read them.  The instance options, option.weight and option.get_inst_coverage,
    // are evaluated by the constructor, as are the other instance options; the type options,
    // type_option.weight and type_option.merge_instances, are constant, and initialize the
    // static member.  Without its own type_option.merge_instances, a covergroup has the
    // default of --coverage-merge-instances.  A constant option.at_least is the default of
    // the coverpoints and crosses.
    فراغ پایینی_Covergroup_Options() {
        منطقي ادغام_مجموعه = خطأ;  // The covergroup sets type_option.merge_instances
        m_cg_At_کمترین = 1;
        لكل (Ast_Cg_Option_تخصیص_بده* ثابت optp : m_cgOptions) {
            إذا (optp->optType() == VCoverOptionType::AT_کمترین) {
                إذا (ثابت AstConst* ثابت constp = constOption(optp->valuep(), optp->optType())) {
                    m_cg_At_کمترین = const_At_کمترین(constp);
                }
                VL_DO_DANGLING(افزودن_Deletep(optp->unlink_Fr_پشت()), optp);
                استمر;
            }
            File_سطر* ثابت fl = optp->fileline();
            // V3Width left the type options constant, and type_option.weight non-negative
            ثابت AstConst* ثابت constp = VN_CAST(optp->valuep(), Const);
            std::string member = "weight";
            إذا (optp->optType() == VCoverOptionType::ادغام_INSTANCES) {
                member = "merge_instances";
                m_cg_ادغام_Instances = !constp->رقم().isEqZero();
                ادغام_مجموعه = درست;
            } وإلا إذا (optp->optType() == VCoverOptionType::GET_INST_COVERAGE) {
                member = "get_inst_coverage";
            } وإلا {
                UASSERT_OBJ(optp->optType() == VCoverOptionType::وزن, optp,
                            "Unexpected covergroup option reaching V3Covergroup");
                إذا (optp->typeOption()) m_cg_Type_وزن = constp->مقصد_UInt();
            }
            Ast_تخصیص_بده* ثابت assignp = جديد Ast_تخصیص_بده{
                fl, newOptionSel(fl, optionVar(optp->typeOption()), member, VAccess::WRITE),
                optp->valuep()->unlink_Fr_پشت()};
            إذا (optp->typeOption()) {
                m_covergroupp->جمع_کن_Membersp(جديد AstInitialStatic{fl, assignp});
                VL_DO_DANGLING(افزودن_Deletep(optp->unlink_Fr_پشت()), optp);
            } وإلا {
                optp->replaceWith(assignp);
                VL_DO_DANGLING(افزودن_Deletep(optp), optp);
            }
        }
        m_cgOptions.clear();
        إذا (!ادغام_مجموعه && v3Global.opt.coverage_ادغام_Instances()) {
            File_سطر* ثابت fl = m_covergroupp->fileline();
            m_covergroupp->جمع_کن_Membersp(جديد AstInitialStatic{
                fl, جديد Ast_تخصیص_بده{
                        fl, newOptionSel(fl, optionVar(درست), "merge_instances", VAccess::WRITE),
                        جديد AstConst{fl, AstConst::بیت_True{}}}});
            m_cg_ادغام_Instances = درست;
        }
        m_cg_May_ادغام = m_cg_ادغام_Instances || m_mergeableTypeOptions.شمار(optionVar(درست));
    }

    // The weight of an item in the coverage database, which merges the instances: its
    // type_option.weight if the covergroup merges them too (IEEE 1800-2023 19.11.3); else its
    // option.weight if a constant, and so of every instance; else its type_option.weight
    uint32_t قلم_Database_وزن(Ast_گره* optionsp) ثابت {
        ثابت Ast_گره_عبارت* weightp = اشاره_تهی;  // The option.weight in effect
        uint32_t type_وزن = 1;
        لكل (Ast_گره* nodep = optionsp; nodep; nodep = nodep->nextp()) {
            ثابت AstCoverOption* ثابت optp = VN_AS(nodep, CoverOption);
            إذا (!(optp->optType() == VCoverOptionType::وزن)) استمر;
            // V3Width left type_option.weight a non-negative constant
            إذا (optp->typeOption()) {
                type_وزن = VN_AS(optp->valuep(), Const)->مقصد_UInt();
            } وإلا {
                weightp = optp->valuep();
            }
        }
        إذا (m_cg_ادغام_Instances) أرجع type_وزن;
        إذا (!weightp) أرجع 1;
        إذا (ثابت AstConst* ثابت constp = VN_CAST(weightp, Const)) أرجع constp->مقصد_UInt();
        أرجع type_وزن;
    }

    // Configure an item's option.weight, its weight in instance coverage (IEEE 1800-2023
    // 19.11), and, if the covergroup may merge its instances, its type_option.weight, its
    // weight in type coverage then (19.11.3).
    فراغ generate_قلم_وزن(File_سطر* fl, AstVar* قلم_Varp, Ast_گره* optionsp) {
        لكل (Ast_گره* nodep = optionsp; nodep; nodep = nodep->nextp()) {
            ثابت AstCoverOption* ثابت optp = VN_AS(nodep, CoverOption);
            إذا (!(optp->optType() == VCoverOptionType::وزن)) استمر;
            إذا (!optp->typeOption()) {
                m_constructorp->جمع_کن_Stmtsp(
                    قلم_Call(fl, قلم_Varp, VCMethod::COVERGROUP_وزن,
                             {optp->valuep()->clone_درخت(خطأ), file_سطر_Debug(optp->fileline())})
                        ->makeStmt());
            } وإلا إذا (m_cg_May_ادغام) {
                // V3Width left type_option.weight a non-negative constant
                m_constructorp->جمع_کن_Stmtsp(قلم_Call(fl, قلم_Varp, VCMethod::COVERGROUP_TYPE_وزن,
                                                   {optp->valuep()->clone_درخت(خطأ)})
                                              ->makeStmt());
            }
        }
    }

    فراغ processCovergroup() {
        UINFO(4, "Processing covergroup: " << m_covergroupp->name() << " with "
                                           << m_coverpoints.اندازه() << " coverpoints and "
                                           << m_coverCrosses.اندازه() << " crosses");

        m_crossed_Cp_نامها.clear();
        m_cpVarMap.clear();
        m_cpBins.clear();
        m_droppedBins.clear();
        m_sized_نامها.reset();
        m_runtimePoints.clear();
        m_runtimeCrosses.clear();
        m_excludedVars.clear();
        m_droppedCrosses.clear();
        m_cgInstVarp = اشاره_تهی;
        m_cg_Type_وزن = 1;
        m_cg_ادغام_Instances = خطأ;
        m_cg_May_ادغام = خطأ;

        پایینی_Covergroup_Options();

        // Scan every cross item to record the coverpoints it references (the cross dimensions)
        // and to flag any cross naming a bare variable -- a would-be implicit coverpoint, which
        // Verilator does not synthesize.  An unresolvable item drops only that one cross (with a
        // COVERIGN in generateCrossCode), leaving the rest of the covergroup intact.
        لكل (AstCoverCross* crossp : m_coverCrosses) {
            لكل (Ast_گره* itemp = crossp->itemsp(); itemp; itemp = itemp->nextp()) {
                ثابت AstCoverpointRef* ثابت refp = VN_AS(itemp, CoverpointRef);
                إذا (refp->exprp()) استمر;  // hierarchical ref: dropped in generateCrossCode
                إذا (m_coverpointMap.پیدا_کن(refp->name()) == m_coverpointMap.انتها()) {
                    m_droppedCrosses.درج(crossp);  // bare variable: drop this cross only
                } وإلا {
                    m_crossed_Cp_نامها.درج(refp->name());
                }
            }
        }

        std::وکتور<Ast_گره*> pending;
        std::map<AstCoverpoint*, std::وکتور<AstCoverCross*>> consumers;
        std::map<AstCoverCross*, std::وکتور<AstCoverpoint*>> inputs;
        لكل (AstCoverpoint* ثابت cpp : m_coverpoints) {
            check_Bin_نامها(cpp);
            checkConstructedBins(cpp);
            إذا (!cpp->exprp()->dtypep()->skipRefp()->isIntegralOrPacked()) استمر;
            // Bins without values leave the report (IEEE 1800-2023 19.11.1), exclusions or not.
            // Constructed bins get their values when the covergroup is constructed.
            إذا (!coverpointHasStateExclusions(cpp) && !coverpoint_Has_خالی_Bins(cpp)
                && !coverpointHasConstructedBins(cpp)) {
                استمر;
            }
            m_runtimePoints.درج(cpp);
            pending.افزودن_پشت(cpp);
        }
        لكل (AstCoverCross* ثابت crossp : m_coverCrosses) {
            إذا (m_droppedCrosses.شمار(crossp)) استمر;
            لكل (Ast_گره* itemp = crossp->itemsp(); itemp; itemp = itemp->nextp()) {
                ثابت AstCoverpointRef* ثابت refp = VN_AS(itemp, CoverpointRef);
                إذا (refp->exprp()) استمر;
                ثابت خودکار نقطه = m_coverpointMap.پیدا_کن(refp->name());
                إذا (نقطه != m_coverpointMap.انتها()) {
                    consumers[نقطه->دومی].افزودن_پشت(crossp);
                    inputs[crossp].افزودن_پشت(نقطه->دومی);
                }
            }
            // The values of a filter's bin tuples are known when the covergroup is constructed
            إذا (crossp->exists([](ثابت AstCoverCrossWith*) { أرجع درست; })) {
                m_runtimeCrosses.emplace(crossp);
                pending.افزودن_پشت(crossp);
            }
        }
        لكل (اندازه_t بعدی = 0; بعدی < pending.اندازه(); ++بعدی) {
            إذا (AstCoverpoint* ثابت pointp = VN_CAST(pending[بعدی], Coverpoint)) {
                لكل (AstCoverCross* ثابت crossp : consumers[pointp]) {
                    إذا (m_runtimeCrosses.emplace(crossp).دومی) pending.افزودن_پشت(crossp);
                }
            } وإلا {
                لكل (AstCoverpoint* ثابت pointp : inputs[VN_AS(pending[بعدی], CoverCross)]) {
                    إذا (pointp->exprp()->dtypep()->skipRefp()->isIntegralOrPacked()
                        && m_runtimePoints.emplace(pointp).دومی) {
                        pending.افزودن_پشت(pointp);
                    }
                }
            }
        }

        // The instance node owns this instance's coverpoint/cross runtimes, so it must exist
        // before any of them is created.  Emitted first, ahead of both generate loops.
        generateInstanceAttach();

        // For each coverpoint, generate sampling code
        لكل (AstCoverpoint* cpp : m_coverpoints) generate_Coverpoint_کد(cpp);

        // For each cross, generate sampling code
        لكل (AstCoverCross* crossp : m_coverCrosses) generate_Cross_کد(crossp);

        // Every cross has been built, so runtime points only need their exclusions from here.
        لكل (AstCoverpoint* ثابت cpp : m_coverpoints) {
            إذا (!m_runtimePoints.شمار(cpp)) استمر;
            m_constructorp->جمع_کن_Stmtsp(قلم_Call(cpp->fileline(), m_cpVarMap.at(cpp->name()),
                                               VCMethod::COVERGROUP_مقدار_RELEASE)
                                          ->makeStmt());
        }
        لكل (Ast_گره_عبارت* valuep : m_detached_مقادیر) VL_DO_DANGLING(افزودن_Deletep(valuep), valuep);
        m_detached_مقادیر.clear();

        // Generate coverage computation code (even for empty covergroups).  Bin registration
        // with the coverage database is handled per coverpoint/cross by their runtime
        // registerBins() calls (emitted in generateCoverpoint/generateCross).
        generate_Coverage_Computation_کد();
    }

    ساكن constexpr اندازه_t مقدار_LIST_ENTRIES = 256;  // Metadata entries per constructor call

    // The value of a constant count, of bins or value tuples: -1 if it is negative, and
    // saturated above the largest limit
    ساكن int64_t const_شمار(ثابت AstConst* constp) {
        ثابت V3_عدد& رقم = constp->رقم();
        إذا (constp->isSigned() && رقم.isNegative()) أرجع -1;
        أرجع رقم.most_مجموعه_بیت_P1() > 32 ? INT64_MAX : static_cast<int64_t>(رقم.مقصد_UQuad());
    }

    // Whether a count, 'whatp', is integral and, if constant, positive, as the standard's clause
    // 'clausep' requires; else errors
    ساكن منطقي check_شمار(ثابت Ast_گره_عبارت* countp, ثابت حرف* whatp, ثابت حرف* clausep) {
        ثابت AstConst* ثابت constp = VN_CAST(countp, Const);
        إذا (!countp->dtypep()->skipRefp()->isIntegralOrPacked()) {
            countp->v3error(whatp << " must be integral (IEEE 1800-2023 " << clausep << ")");
            أرجع خطأ;
        } وإلا إذا (constp && (constp->رقم().isFourState() || const_شمار(constp) < 1)) {
            countp->v3error(whatp << " must be >= 1, got "
                                  << (constp->رقم().isFourState() ? constp->رقم().ascii(خطأ)
                                                                  : constp->رقم().مقصد_Decimal_S())
                                  << " (IEEE 1800-2023 " << clausep << ")");
            أرجع خطأ;
        }
        أرجع درست;
    }

    // The number of bins requested by a valid 'bins auto[N]', or 0
    ساكن uint32_t autoBinsRequested(ثابت AstCoverBin* binp) {
        ثابت AstConst* ثابت constp = VN_CAST(binp->arraySizep(), Const);
        إذا (!constp) أرجع 0;
        ثابت int64_t شمار = const_شمار(constp);
        أرجع شمار < 1 || شمار > v3Global.opt.coverageMaxBins() ? 0
                                                                   : static_cast<uint32_t>(شمار);
    }

    // True for a 'bins auto[N]' declaration, or the implicit automatic bins of a coverpoint
    ساكن منطقي isAutoBins(ثابت AstCoverBin* binp) {
        أرجع binp->binsType() == VCoverBinsType::BINS_AUTO
               || binp->binsType() == VCoverBinsType::BINS_AUTO_IMPLICIT;
    }

    // True for a sized array of bins, 'bins b[N] = {...}', whose values an integral coverpoint
    // distributes over N bins when the covergroup is constructed (IEEE 1800-2023 19.5.1)
    ساكن منطقي isSizedArray(ثابت AstCoverBin* binp) {
        أرجع binp->arraySizep() && !isAutoBins(binp);
    }

    // The 'with' filter of a bin, if any (IEEE 1800-2023 19.5.1.1)
    ساكن AstCoverWith* binWith(ثابت AstCoverBin* binp) {
        أرجع VN_CAST(binp->rangesp(), CoverWith);
    }

    // True for bins whose values the covergroup constructor computes: a sized array, or bins of
    // the values a 'with' filter keeps
    ساكن منطقي isConstructedBins(ثابت AstCoverBin* binp) {
        أرجع isSizedArray(binp) || binWith(binp);
    }

    // The range list of a bin's values, or of the candidates of its 'with' filter; null for all
    // of the coverpoint's values, which a filter of the coverpoint's name has
    ساكن Ast_گره* binRangesp(ثابت AstCoverBin* binp) {
        ثابت AstCoverWith* ثابت withp = binWith(binp);
        إذا (!withp) أرجع binp->rangesp();
        أرجع VN_IS(withp->subp(), CoverpointRef) ? اشاره_تهی : withp->subp();
    }

    // Report and delete a bins declaration of a name that another of its coverpoint has
    فراغ check_Bin_نامها(AstCoverpoint* coverpointp) {
        std::مجموعه<std::string> نامها;
        لكل (Ast_گره* nodep = coverpointp->binsp(); nodep;) {
            AstCoverBin* ثابت binp = VN_AS(nodep, CoverBin);
            nodep = nodep->nextp();
            إذا (نامها.emplace(binp->name()).دومی) استمر;
            binp->v3error("Duplicate bin " << binp->prettyNameQ() << " in coverpoint "
                                           << coverpointp->prettyNameQ()
                                           << " (IEEE 1800-2023 3.13)");
            VL_DO_DANGLING(افزودن_Deletep(binp->unlink_Fr_پشت()), binp);
        }
    }

    // Delete an ignored bins declaration, which binsof then selects as no bins
    فراغ dropBins(ثابت AstCoverpoint* coverpointp, AstCoverBin* binp) {
        m_droppedBins[coverpointp].افزودن_پشت(binp->name());
        VL_DO_DANGLING(افزودن_Deletep(binp->unlink_Fr_پشت()), binp);
    }

    // Check the size of a sized array of bins, which drops an invalid array.  A real coverpoint's
    // are unsupported, and treated as arrays of a bin per value.  False unless it stays sized.
    منطقي check_Bins_Array_اندازه(ثابت AstCoverpoint* coverpointp, AstCoverBin* binp, منطقي integral) {
        Ast_گره_عبارت* ثابت sizep = binp->arraySizep();
        إذا (VN_IS(sizep, Unbounded)) {  // A parameter of '$'; see bins_orBraE
            binp->v3error("Bins array size must be integral, not '$' (IEEE 1800-2023 19.5.1)");
        } وإلا إذا (check_شمار(sizep, "Bins array size", "19.5.1")) {
            إذا (integral) أرجع درست;
            binp->v3warn(COVERIGN, "Unsupported: 'bins' explicit array size of a real "
                                   "coverpoint (treated as '[]')");
            VL_DO_DANGLING(افزودن_Deletep(sizep->unlink_Fr_پشت()), sizep);
            أرجع خطأ;
        }
        dropBins(coverpointp, binp);
        أرجع خطأ;
    }

    // Check the bins of a coverpoint whose values the constructor computes, dropping invalid
    // ones.  A wildcard array of too many ranges of values is ignored, or if ignore or illegal,
    // treated as one bin; filtering too many ranges of values ignores the bins.
    فراغ checkConstructedBins(AstCoverpoint* coverpointp) {
        ثابت منطقي integral = coverpointp->exprp()->dtypep()->skipRefp()->isIntegralOrPacked();
        لكل (Ast_گره* nodep = coverpointp->binsp(); nodep;) {
            AstCoverBin* ثابت binp = VN_AS(nodep, CoverBin);
            nodep = nodep->nextp();
            إذا (!isConstructedBins(binp)) استمر;
            إذا (isSizedArray(binp) && !check_Bins_Array_اندازه(coverpointp, binp, integral)) استمر;
            إذا (!binp->isWildcard()
                || sizedWildcardRuns(binp, coverpointp->exprp())
                       <= v3Global.opt.coverageMaxBins()) {
                إذا (binWith(binp) && withCandidatesOver(binp, coverpointp->exprp())) {
                    binp->v3warn(COVERIGN, "Unsupported: 'with' filter of more than 2**32 "
                                           "candidate values; bin "
                                               << binp->prettyNameQ() << " ignored");
                    إذا (binp->binsType().bin_Is_نرمال()) coverpointp->user2(درست);
                    dropBins(coverpointp, binp);
                }
                استمر;
            }
            // An ignore or illegal array still excludes or checks its values, as one bin
            ثابت منطقي single = !binp->binsType().bin_Is_نرمال() && !binWith(binp);
            binp->v3warn(COVERIGN,
                         "Unsupported: " << (binWith(binp) ? "'with' filter of wildcard '"
                                                           : "sized wildcard array '")
                                         << binp->binsType().verilogKwd()
                                         << "' of more than --coverage-max-bins of "
                                         << v3Global.opt.coverageMaxBins()
                                         << " ranges of values; bin " << binp->prettyNameQ()
                                         << (single ? " treated as one bin" : " ignored") << "\n"
                                         << binp->warn_بیشتر()
                                         << "... Suggest a larger --coverage-max-bins");
            إذا (single) {
                Ast_گره_عبارت* ثابت sizep = binp->arraySizep();
                VL_DO_DANGLING(افزودن_Deletep(sizep->unlink_Fr_پشت()), sizep);
                binp->isArray(خطأ);
                استمر;
            }
            إذا (binp->binsType().bin_Is_نرمال()) coverpointp->user2(درست);
            dropBins(coverpointp, binp);
        }
    }

    // True if a 'with' filter would be evaluated for more than 2**32 candidate values, known
    // now for the coverpoint's name, or for a range list of constants: each is evaluated once
    // but for an array 'b[N]', which keeps their order and duplicates (see withBegin())
    ساكن منطقي withCandidatesOver(AstCoverBin* binp, Ast_گره_عبارت* exprp) {
        ثابت صحيح عرض = پیاپی_عرض(exprp);
        std::وکتور<std::pair<V3_عدد, V3_عدد>> runs;
        إذا (!binRangesp(binp)) runs = coverpoint_مقادیر(binp, exprp);
        ثابت خودکار constant
            = [](ثابت Ast_گره* nodep) { أرجع VN_IS(nodep, Const) || VN_IS(nodep, Unbounded); };
        لكل (Ast_گره* rangep = binRangesp(binp); rangep; rangep = rangep->nextp()) {
            ثابت AstInsideRange* ثابت irp = VN_CAST(rangep, InsideRange);
            // Else known when constructed
            إذا (irp ? !constant(irp->lhsp()) || !constant(irp->rhsp()) : !VN_IS(rangep, Const)) {
                أرجع خطأ;
            }
            Cross_مقدار_Range range{rangep, resolve_عرض(rangep, exprp)};
            إذا (!resolve_مقدار(rangep, exprp, درست, binp->isWildcard(), range)
                || cross_Range_خالی(range)) {
                استمر;
            }
            std::وکتور<std::pair<V3_عدد, V3_عدد>> یافت{{range.lo, range.hi}};
            إذا (range.wildcard) {  // checkConstructedBins bounded the runs
                یافت.clear();
                crossRangeRuns(range, v3Global.opt.coverageMaxBins(), یافت);
            }
            لكل (ثابت std::pair<V3_عدد, V3_عدد>& پیاپی : یافت) {
                runs.emplace_پشت(V3_عدد{rangep, عرض, پیاپی.اولی},
                                  V3_عدد{rangep, عرض, پیاپی.دومی});
            }
        }
        إذا (!isSizedArray(binp)) {  // The union of the values
            std::مرتب_سازی(runs.begin(), runs.انتها(), [](ثابت خودکار& lhs, ثابت خودکار& rhs) {
                أرجع cross_مقدار_Less(lhs.اولی, rhs.اولی);
            });
            std::وکتور<std::pair<V3_عدد, V3_عدد>> merged;
            لكل (ثابت std::pair<V3_عدد, V3_عدد>& پیاپی : runs) {
                إذا (merged.خالی() || cross_مقدار_Less(merged.پشت().دومی, پیاپی.اولی)) {
                    merged.افزودن_پشت(پیاپی);
                } وإلا إذا (cross_مقدار_Less(merged.پشت().دومی, پیاپی.دومی)) {
                    merged.پشت().دومی = پیاپی.دومی;
                }
            }
            runs = std::move(merged);
        }
        uint64_t شمار = 0;
        لكل (ثابت std::pair<V3_عدد, V3_عدد>& پیاپی : runs) {
            V3_عدد بازه{exprp, عرض};
            بازه.opSub(پیاپی.دومی, پیاپی.اولی);
            إذا (بازه.most_مجموعه_بیت_P1() > 32) أرجع درست;  // More than 2**32 values
            شمار += بازه.مقصد_UQuad() + 1;
            إذا (شمار > (uint64_t{1} << 32)) أرجع درست;
        }
        أرجع خطأ;
    }

    // The ranges of values the wildcard patterns of a sized wildcard array, or of a 'with'
    // filter's candidates, give, counted up to more than --coverage-max-bins
    ساكن اندازه_t sizedWildcardRuns(ثابت AstCoverBin* binp, Ast_گره_عبارت* exprp) {
        std::وکتور<std::pair<V3_عدد, V3_عدد>> runs;
        لكل (Ast_گره* rangep = binRangesp(binp); rangep; rangep = rangep->nextp()) {
            إذا (!VN_IS(rangep, Const)) استمر;  // A range, or a value known at construction
            Cross_مقدار_Range range{rangep, resolve_عرض(rangep, exprp)};
            إذا (resolve_مقدار(rangep, exprp, درست, درست, range) && !cross_Range_خالی(range)) {
                crossRangeRuns(range, v3Global.opt.coverageMaxBins(), runs);
            }
        }
        أرجع runs.اندازه();
    }

    // Check the automatic bins declarations of a coverpoint.  Each stays one declaration, which
    // generates as a partition of the coverpoint domain (see autoBinRuns).
    فراغ checkAutomaticBins(AstCoverpoint* coverpointp, ثابت Ast_گره_عبارت* exprp) {
        لكل (Ast_گره* binp = coverpointp->binsp(); binp; binp = binp->nextp()) {
            AstCoverBin* ثابت cbinp = VN_AS(binp, CoverBin);
            إذا (cbinp->binsType() != VCoverBinsType::BINS_AUTO) استمر;
            ثابت AstConst* ثابت constp = VN_CAST(cbinp->arraySizep(), Const);
            إذا (!constp) {
                cbinp->v3error("Automatic bins array size must be a constant");
            } وإلا إذا (const_شمار(constp) < 1) {
                cbinp->v3error("Automatic bins array size must be >= 1, got "
                               << constp->رقم().مقصد_Decimal_S());
            } وإلا إذا (const_شمار(constp) > v3Global.opt.coverageMaxBins()) {
                cbinp->v3error("Automatic bins array size of "
                               << constp->رقم().مقصد_Decimal_U() << " exceeds limit of "
                               << v3Global.opt.coverageMaxBins() << '\n'
                               << cbinp->warn_بیشتر() << "... Suggest a larger --coverage-max-bins");
            } وإلا إذا (!exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
                cbinp->v3error("Automatic bins are not allowed on a coverpoint of a non-integral "
                               "expression (IEEE 1800-2023 19.5.3).");
            }
        }
    }

    // Extract all option values of a coverpoint or cross in a single pass.
    // atLeastOut: option.at_least (default the covergroup's)
    // autoBinMaxOut: option.auto_bin_max (coverpoint overrides covergroup, default 64)
    فراغ استخراج_قلم_Options(Ast_گره* optionsp, صحيح& at_کمترین_بیرون, صحيح& auto_Bin_Max_بیرون) {
        at_کمترین_بیرون = m_cg_At_کمترین;
        auto_Bin_Max_بیرون = -1;  // -1 = not set at coverpoint level
        لكل (Ast_گره* optionp = optionsp; optionp; optionp = optionp->nextp()) {
            AstCoverOption* ثابت optp = VN_AS(optionp, CoverOption);
            // Weights may be non-constant; generateItemWeight() handles them.  A cross keeps
            // its unsupported options, which V3LinkParse warned of.
            إذا (!(optp->optType() == VCoverOptionType::AT_کمترین
                  || optp->optType() == VCoverOptionType::AUTO_BIN_MAX)) {
                استمر;
            }
            ثابت AstConst* ثابت constp = constOption(optp->valuep(), optp->optType());
            إذا (!constp) استمر;
            إذا (optp->optType() == VCoverOptionType::AT_کمترین) {
                at_کمترین_بیرون = const_At_کمترین(constp);
            } وإلا {
                auto_Bin_Max_بیرون = constp->مقصد_SInt();
            }
        }
        // Fall back to covergroup-level auto_bin_max if not set at coverpoint level
        إذا (auto_Bin_Max_بیرون < 0) {
            إذا (m_covergroupp->cgAutoBinMax() >= 0) {
                auto_Bin_Max_بیرون = m_covergroupp->cgAutoBinMax();
            } وإلا {
                auto_Bin_Max_بیرون = 64;  // Default per IEEE 1800-2023 Table 19-1
            }
        }
    }

    // IEEE 1800-2023 19.5.2: an enum coverpoint has one automatic bin per enumeration value
    فراغ createEnumAutoBins(AstCoverpoint* coverpointp, Ast_گره_عبارت* exprp,
                            ثابت AstEnumDType* enump) {
        File_سطر* ثابت fl = coverpointp->fileline();
        لكل (ثابت Ast_Enum_قلم* itemp = enump->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Enum_قلم)) {
            AstConst* ثابت lop = new_مقدار_Const(fl, VN_AS(itemp->valuep(), Const)->رقم(), exprp);
            AstInsideRange* ثابت rangep = جديد AstInsideRange{fl, lop, lop->clone_درخت(خطأ)};
            rangep->dtypeFrom(exprp);
            coverpointp->جمع_کن_Binsp(
                جديد AstCoverBin{fl, "auto[" + itemp->name() + "]", rangep, خطأ, خطأ});
        }
    }

    // IEEE 1800-2023 19.5.3/19.11.1: partition first, then apply exclusions.  The partition is one
    // automatic bins declaration, generated as a run like 'bins auto[N]' but numbering its bins.
    فراغ createImplicitAutoBins(AstCoverpoint* coverpointp, Ast_گره_عبارت* exprp, صحيح autoBinMax) {
        إذا (coverpointp->user2()) أرجع;  // Declared bins, ignored, leave no bins
        لكل (Ast_گره* nodep = coverpointp->binsp(); nodep; nodep = nodep->nextp()) {
            ثابت VCoverBinsType نوع = VN_AS(nodep, CoverBin)->binsType();
            إذا (نوع != VCoverBinsType::BINS_IGNORE && نوع != VCoverBinsType::BINS_ILLEGAL)
                أرجع;
        }
        إذا (ثابت AstEnumDType* ثابت enump
            = VN_CAST(exprp->dtypep()->skip_Ref_مقصد_Enump(), EnumDType)) {
            createEnumAutoBins(coverpointp, exprp, enump);
            أرجع;
        }
        ثابت صحيح عرض = exprp->عرض();
        uint32_t شمار = عرض < 31 ? std::min<uint32_t>(uint32_t{1} << عرض, autoBinMax)
                                    : static_cast<uint32_t>(autoBinMax);
        إذا (!شمار) أرجع;
        إذا (!exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
            coverpointp->v3error("Coverpoint of a non-integral expression requires explicit bins "
                                 "(IEEE 1800-2023 19.5.3).");
            أرجع;
        }
        إذا (شمار > v3Global.opt.coverageMaxBins()) {
            coverpointp->v3warn(COVERIGN, "Unsupported: more than "
                                              << v3Global.opt.coverageMaxBins()
                                              << " automatic bins from 'option.auto_bin_max'; "
                                                 "using "
                                              << v3Global.opt.coverageMaxBins() << ".\n"
                                              << coverpointp->warn_بیشتر()
                                              << "... Suggest a larger --coverage-max-bins");
            شمار = v3Global.opt.coverageMaxBins();
        }
        File_سطر* ثابت fl = coverpointp->fileline();
        coverpointp->جمع_کن_Binsp(جديد AstCoverBin{fl, "auto", جديد AstConst{fl, شمار},
                                              VCoverBinsType::BINS_AUTO_IMPLICIT});
    }

    // Sanitize generated names to be valid C++ identifiers
    ساكن string sanitizeGeneratedName(string name) {
        std::replace(name.begin(), name.انتها(), '[', '_');
        std::replace(name.begin(), name.انتها(), ']', '_');
        أرجع name;
    }

    // Capture an iff guard in a function-local temporary so it is evaluated once per sample()
    AstVarRef* capture_Iff_مقصد_موقت(Ast_گره_عبارت* iffp, ثابت string& موقت_Name) {
        File_سطر* ثابت fl = iffp->fileline();
        AstVar* ثابت iffVarp
            = جديد AstVar{fl, VVarType::BLOCKTEMP, موقت_Name, iffp->پیدا_کن_بیت_DType()};
        iffVarp->funcLocal(درست);
        m_sampleFuncp->جمع_کن_Stmtsp(iffVarp);
        iffp->unlink_Fr_پشت();
        m_sampleFuncp->جمع_کن_Stmtsp(
            جديد Ast_تخصیص_بده{fl, جديد AstVarRef{fl, iffVarp, VAccess::WRITE}, iffp});
        أرجع جديد AstVarRef{fl, iffVarp, VAccess::READ};
    }

    // Add a statement sampling the current coverpoint to sample(), under its iff guard if any
    فراغ جمع_کن_Sample_Stmt(Ast_گره* stmtp) {
        إذا (m_cpGuardp) {
            m_cpGuardp->جمع_کن_Thensp(stmtp);
        } وإلا {
            m_sampleFuncp->جمع_کن_Stmtsp(stmtp);
        }
    }

    // Create previous value variable for transition tracking
    AstVar* create_Prev_مقدار_Var(AstCoverpoint* coverpointp, Ast_گره_عبارت* exprp) {
        // Check if already created
        إذا (AstVar* ثابت prevVarp = VN_CAST(coverpointp->user1p(), Var)) أرجع prevVarp;

        // Create variable to store previous sampled value
        ثابت string varName = "__Vprev_" + coverpointp->name();
        AstVar* prevVarp
            = جديد AstVar{coverpointp->fileline(), VVarType::MEMBER, varName, exprp->dtypep()};
        m_covergroupp->جمع_کن_Membersp(prevVarp);

        UINFO(4, "    Created previous value variable: " << varName);

        // Initialize to zero in constructor
        Ast_گره_عبارت* ثابت initExprp
            = جديد AstConst{prevVarp->fileline(), AstConst::Widthed_مقدار{}, prevVarp->عرض(), 0};
        Ast_گره_Stmt* ثابت initStmtp = جديد Ast_تخصیص_بده{
            prevVarp->fileline(), جديد AstVarRef{prevVarp->fileline(), prevVarp, VAccess::WRITE},
            initExprp};
        m_constructorp->جمع_کن_Stmtsp(initStmtp);

        coverpointp->user1p(prevVarp);
        أرجع prevVarp;
    }

    // Create state position variable for multi-value transition bins
    // Tracks position in sequence: 0=not started, 1=seen first item, etc.
    AstVar* createSequenceStateVar(AstCoverpoint* coverpointp, AstCoverBin* binp) {
        // Create variable to track sequence position
        ثابت string varName = "__Vseqpos_" + coverpointp->name() + "_" + binp->name();
        // Use 8-bit integer for state position (sequences rarely > 255 items)
        AstVar* stateVarp
            = جديد AstVar{binp->fileline(), VVarType::MEMBER, varName, VFlagLogicPacked{}, 8};
        m_covergroupp->جمع_کن_Membersp(stateVarp);

        UINFO(4, "    Created sequence state variable: " << varName);

        // Initialize to 0 (not started) in constructor
        Ast_گره_Stmt* ثابت initStmtp = جديد Ast_تخصیص_بده{
            stateVarp->fileline(), جديد AstVarRef{stateVarp->fileline(), stateVarp, VAccess::WRITE},
            جديد AstConst{stateVarp->fileline(), AstConst::Widthed_مقدار{}, 8, 0}};
        m_constructorp->جمع_کن_Stmtsp(initStmtp);

        أرجع stateVarp;
    }

    فراغ generate_Coverpoint_کد(AstCoverpoint* coverpointp) {
        UINFO(4, "  Generating code for coverpoint: " << coverpointp->name());

        // Get the coverpoint expression
        Ast_گره_عبارت* exprp = coverpointp->exprp();

        // Check automatic bins before processing
        checkAutomaticBins(coverpointp, exprp);

        // Extract all coverpoint options in a single pass
        صحيح at_کمترین_مقدار;
        صحيح autoBinMax;
        استخراج_قلم_Options(coverpointp->optionsp(), at_کمترین_مقدار, autoBinMax);
        UINFO(6, "    Coverpoint at_least = " << at_کمترین_مقدار << " auto_bin_max = " << autoBinMax);

        // Create implicit automatic bins if no regular bins exist
        createImplicitAutoBins(coverpointp, exprp, autoBinMax);

        // A false iff guard disables sampling of the coverpoint (IEEE 1800-2023 19.5), so
        // neither is its expression evaluated, nor are its bins or transition state updated
        VL_RESTORER(m_cpGuardp);
        إذا (Ast_گره_عبارت* ثابت iffp = coverpointp->iffp()) {
            m_cpGuardp = جديد AstIf{iffp->fileline(), iffp->unlink_Fr_پشت()};
        }

        AstVar* ثابت مقدار_Varp = جديد AstVar{
            coverpointp->fileline(), VVarType::BLOCKTEMP,
            "__VcpValue_" + sanitizeGeneratedName(coverpointp->name()), exprp->dtypep()};
        مقدار_Varp->funcLocal(درست);
        m_sampleFuncp->جمع_کن_Stmtsp(مقدار_Varp);
        exprp->unlink_Fr_پشت();
        جمع_کن_Sample_Stmt(جديد Ast_تخصیص_بده{
            coverpointp->fileline(),
            جديد AstVarRef{coverpointp->fileline(), مقدار_Varp, VAccess::WRITE}, exprp});
        coverpointp->exprp(جديد AstVarRef{coverpointp->fileline(), مقدار_Varp, VAccess::READ});
        exprp = coverpointp->exprp();

        // Every coverpoint routes through the VlCoverpoint runtime.  Transition coverpoints are
        // included: their per-value matching is still generated as a state machine in sample()
        // (see generateCoverpoint), but the bin hit is recorded in the runtime bin
        // rather than a bare counter.
        generateCoverpoint(coverpointp, exprp, at_کمترین_مقدار);
        // Sampling follows what generateCoverpoint added unguarded, such as clearing the hit list
        إذا (m_cpGuardp) m_sampleFuncp->جمع_کن_Stmtsp(m_cpGuardp);
    }

    // Build the condition under which a default bin matches: NOT(OR of all normal bins).
    Ast_گره_عبارت* buildDefaultCondition(AstCoverpoint* coverpointp, Ast_گره_عبارت* exprp,
                                       File_سطر* fl) {
        Ast_گره_عبارت* anyBinMatchp = اشاره_تهی;
        لكل (Ast_گره* binp = coverpointp->binsp(); binp; binp = binp->nextp()) {
            AstCoverBin* ثابت cbinp = VN_AS(binp, CoverBin);
            إذا (cbinp->binsType() == VCoverBinsType::BINS_DEFAULT
                || cbinp->binsType() == VCoverBinsType::BINS_IGNORE
                || cbinp->binsType() == VCoverBinsType::BINS_ILLEGAL)
                استمر;
            // The values of constructed bins are known at construction; see emitSizedSample
            إذا (isConstructedBins(cbinp)) استمر;
            إذا (isAutoBins(cbinp)) {
                // Automatic bins partition the whole domain, leaving no default value
                إذا (anyBinMatchp) VL_DO_DANGLING(افزودن_Deletep(anyBinMatchp), anyBinMatchp);
                أرجع جديد AstConst{fl, AstConst::بیت_False{}};
            }
            Ast_گره_عبارت* ثابت binCondp = buildBinCondition(cbinp, exprp);
            UASSERT_OBJ(binCondp, cbinp,
                        "buildBinCondition returned nullptr for non-ignore/non-illegal bin");
            anyBinMatchp = anyBinMatchp ? جديد AstOr{fl, anyBinMatchp, binCondp} : binCondp;
        }
        أرجع anyBinMatchp ? static_cast<Ast_گره_عبارت*>(جديد AstNot{fl, anyBinMatchp})
                            : static_cast<Ast_گره_عبارت*>(جديد AstConst{fl, AstConst::بیت_True{}});
    }

    //====================================================================
    // VlCoverpoint conversion

    ساكن منطقي coverpointHasStateExclusions(ثابت AstCoverpoint* coverpointp) {
        لكل (ثابت Ast_گره* nodep = coverpointp->binsp(); nodep; nodep = nodep->nextp()) {
            ثابت AstCoverBin* ثابت binp = VN_AS(nodep, CoverBin);
            إذا (!binp->transp() && binp->rangesp()
                && (binp->binsType() == VCoverBinsType::BINS_IGNORE
                    || binp->binsType() == VCoverBinsType::BINS_ILLEGAL)) {
                أرجع درست;
            }
        }
        أرجع خطأ;
    }

    ساكن منطقي coverpointHasConstructedBins(ثابت AstCoverpoint* coverpointp) {
        لكل (ثابت Ast_گره* nodep = coverpointp->binsp(); nodep; nodep = nodep->nextp()) {
            إذا (isConstructedBins(VN_AS(nodep, CoverBin))) أرجع درست;
        }
        أرجع خطأ;
    }

    // True if a Normal state bin, or an array-bin element, has no value of the coverpoint's
    // type (IEEE 1800-2023 19.5.7).  Array ranges enumerate in-type values, so cannot vanish.
    ساكن منطقي coverpoint_Has_خالی_Bins(ثابت AstCoverpoint* coverpointp) {
        Ast_گره_عبارت* ثابت exprp = coverpointp->exprp();
        لكل (Ast_گره* nodep = coverpointp->binsp(); nodep; nodep = nodep->nextp()) {
            ثابت AstCoverBin* ثابت binp = VN_AS(nodep, CoverBin);
            إذا (!binp->binsType().bin_Is_نرمال() || binp->transp() || !binp->rangesp()) استمر;
            // A wildcard array has a bin for each value its elements match, so none empty, and
            // the constructor creates no bin without values of a 'with' filter
            إذا ((binp->isArray() && binp->isWildcard()) || binWith(binp)) استمر;
            منطقي خالی = درست;
            لكل (Ast_گره* valuep = binp->rangesp(); valuep; valuep = valuep->nextp()) {
                إذا (binp->isArray() && VN_IS(valuep, InsideRange)) استمر;
                Cross_مقدار_Range range{valuep, resolve_عرض(valuep, exprp)};
                ثابت منطقي none = resolve_مقدار(valuep, exprp, درست, binp->isWildcard(), range)
                                  && cross_Range_خالی(range);
                إذا (binp->isArray() && none) أرجع درست;
                خالی &= none;
            }
            إذا (خالی && !binp->isArray()) أرجع درست;
        }
        أرجع خطأ;
    }

    // True if a coverpoint has any transition bin.  Used to decide whether sample() emits the
    // end-of-sample previous-value update that transition matching needs.
    ساكن منطقي coverpointHasTransition(AstCoverpoint* coverpointp) {
        لكل (Ast_گره* binp = coverpointp->binsp(); binp; binp = binp->nextp()) {
            إذا (VN_AS(binp, CoverBin)->transp()) أرجع درست;
        }
        أرجع خطأ;
    }

    // The interned AstBasicDType for one of the covergroup runtime keywords.
    ساكن AstBasicDType* basicDType(File_سطر* fl, VBasicDTypeKwd kwd) {
        أرجع v3Global.rootp()->typeTablep()->پیدا_کن_Basic_DType(fl, kwd);
    }

    // Get (or create) the coverpoint dtype for a hit-list bound, interned so each distinct bound
    // yields one node.
    AstCoverpointDType* coverpointDType(File_سطر* fl, uint32_t اصابت_Bound) {
        AstCoverpointDType*& typep = m_cpDTypes[اصابت_Bound];
        إذا (!typep) {
            typep = جديد AstCoverpointDType{fl, اصابت_Bound};
            v3Global.rootp()->typeTablep()->جمع_کن_Typesp(typep);
        }
        أرجع typep;
    }

    // The key of the covergroup's type in the coverage registry, which get_coverage() queries
    std::string covergroupProtectedName() ثابت {
        أرجع VIdProtect::protect_واژگان_If(m_covergroupName, v3Global.opt.protectIds());
    }

    // The name of the covergroup's type, which keys the coverage registry and database: as
    // $typename names it (IEEE 1800-2023 20.6.1), so apart for covergroups of distinct scopes and
    // specializations, and alike in each Verilator run of a hierarchical design.  As design units
    // of distinct libraries may share a name, one of a library other than the default is prefixed
    // by its library, as '%l' prints it (IEEE 1800-2023 33.4).
    std::string covergroupTypeName() ثابت {
        UASSERT_OBJ(m_unitp, m_covergroupp, "Covergroup declared outside of a design unit");
        ثابت std::string& libname = m_unitp->libname();
        ثابت std::string name = m_covergroupp->dtypeName(درست);
        أرجع libname == "work" ? name : libname + "." + name;
    }

    // Emit the covergroup's instance handle member and the constructor statement that creates
    // its node in the per-context coverage registry.  Runs before any coverpoint or cross is
    // generated, so their runtimes can be added to the node as they are created.
    فراغ generateInstanceAttach() {
        File_سطر* ثابت fl = m_covergroupp->fileline();
        // V3LinkParse synthesizes a 'new' for every covergroup; the item generators below already
        // rely on that, and this attach runs even for a covergroup with no coverpoints at all.
        UASSERT_OBJ(m_constructorp, m_covergroupp, "Covergroup missing synthesized constructor");
        m_cgInstVarp = جديد AstVar{fl, VVarType::MEMBER, "__Vcg_inst",
                                  basicDType(fl, VBasicDTypeKwd::COVERGROUP_INSTHANDLE)};
        m_covergroupp->جمع_کن_Membersp(m_cgInstVarp);

        m_constructorp->جمع_کن_Stmtsp(
            قلم_Call(fl, m_cgInstVarp, VCMethod::COVERGROUP_ATTACH,
                     {ctext(fl, "vlSymsp->_vm_contextp__->covergroupRegistryp()"
                                "->newCovergroupInst("
                                    + quoted(covergroupProtectedName())
                                    + (m_cg_May_ادغام ? ", true" : ", false") + ")")},
                     /*usePtr=*/خطأ)
                ->makeStmt());
        // The node reads option.weight in place, so procedural assignments take effect
        AstCExpr* ثابت وزن_Addrp = جديد AstCExpr{fl, "&"};
        وزن_Addrp->جمع_کن(newOptionSel(fl, optionVar(خطأ), "weight", VAccess::READ));
        m_constructorp->جمع_کن_Stmtsp(قلم_Call(fl, m_cgInstVarp, VCMethod::COVERGROUP_LEND_وزن,
                                           {وزن_Addrp, file_سطر_Debug(fl)}, /*usePtr=*/خطأ)
                                      ->makeStmt());
    }

    // A '__Vcg_inst.p()-><method>()' call on the covergroup's instance node
    AstCMethodHard* instanceCall(File_سطر* fl, VCMethod روش) {
        // '__Vcg_inst.p()' -- a value handle, so '.' not '->'
        AstCMethodHard* ثابت instp
            = جديد AstCMethodHard{fl, memberRef(fl, m_cgInstVarp), VCMethod::COVERGROUP_INST_P};
        instp->usePtr(خطأ);
        instp->dtype_مجموعه_Void();  // Opaque receiver; only ever the 'fromp' of the call below
        AstCMethodHard* ثابت callp = جديد AstCMethodHard{fl, instp, روش};
        callp->usePtr(درست);
        أرجع callp;
    }

    // Emit 'this->__Vcp_x = this->__Vcg_inst.p()->addCoverpoint<K>();' (or addCross), which
    // creates the item runtime in the instance node and borrows a pointer to it.
    Ast_تخصیص_بده* make_قلم_Create(File_سطر* fl, AstVar* قلم_Varp, VCMethod روش) {
        AstCMethodHard* ثابت createp = instanceCall(fl, روش);
        createp->dtypep(قلم_Varp->dtypep());
        أرجع جديد Ast_تخصیص_بده{fl, memberRef(fl, قلم_Varp, VAccess::WRITE), createp};
    }

    // Constant bounds of one rangesp() element (an InsideRange or a single Const).  Each bound is
    // the raw AST node -- an AstConst or an AstUnbounded ('$') -- with the const/unbounded view
    // derived on demand, so there is one source of truth per bound.  After a successful
    // constRangeBounds() neither node is null; a single Const has both bounds aliasing one node.
    بنية RangeBounds final {
        Ast_گره* loNodep = اشاره_تهی;  // low bound: AstConst or AstUnbounded
        Ast_گره* hiNodep = اشاره_تهی;  // high bound: AstConst or AstUnbounded
        منطقي loUnbounded() ثابت { أرجع VN_IS(loNodep, Unbounded); }
        منطقي hiUnbounded() ثابت { أرجع VN_IS(hiNodep, Unbounded); }
        AstConst* loConstp() ثابت { أرجع VN_CAST(loNodep, Const); }
        AstConst* hiConstp() ثابت { أرجع VN_CAST(hiNodep, Const); }
    };

    // Decode one rangesp() element into its constant bounds.  Returns false if rp is neither an
    // InsideRange nor a single Const, or if a present bound is non-constant or 4-state.  '$'
    // bounds are left as AstUnbounded (not resolved) -- the caller applies its own policy.
    // Centralizes the InsideRange/Const/Unbounded decode shared by the hit-list-bound paths.
    ساكن منطقي constRangeBounds(Ast_گره* rp, RangeBounds& rb) {
        إذا (AstInsideRange* ثابت irp = VN_CAST(rp, InsideRange)) {
            rb.loNodep = irp->lhsp();
            rb.hiNodep = irp->rhsp();
        } وإلا إذا (AstConst* ثابت cp = VN_CAST(rp, Const)) {
            rb.loNodep = rb.hiNodep = cp;
        } وإلا {
            أرجع خطأ;
        }
        // Each bound must be a constant unless it is '$'; reject non-const and 4-state.
        AstConst* ثابت lc = rb.loConstp();
        AstConst* ثابت hc = rb.hiConstp();
        إذا ((!lc && !rb.loUnbounded()) || (!hc && !rb.hiUnbounded())) أرجع خطأ;
        إذا ((lc && lc->رقم().isFourState()) || (hc && hc->رقم().isFourState())) أرجع خطأ;
        أرجع درست;
    }

    // Collect the covered value intervals of a single (non-array) Normal bin.  Returns false
    // if any range isn't a constant/open InsideRange or single Const (e.g. wildcard, non-const).
    ساكن منطقي استخراج_Range_Intervals(AstCoverBin* cbinp, uint64_t maxVal,
                                      std::وکتور<std::pair<uint64_t, uint64_t>>& بیرون) {
        إذا (!cbinp->rangesp()) أرجع خطأ;
        لكل (Ast_گره* rp = cbinp->rangesp(); rp; rp = rp->nextp()) {
            RangeBounds rb;
            إذا (!constRangeBounds(rp, rb)) أرجع خطأ;
            إذا ((rb.loConstp() && rb.loConstp()->عرض() > 64)
                || (rb.hiConstp() && rb.hiConstp()->عرض() > 64)) {
                أرجع خطأ;  // Use the safe slot-count bound for wide values.
            }
            ثابت uint64_t lo = rb.loUnbounded() ? 0 : rb.loConstp()->مقصد_UQuad();
            ثابت uint64_t hi = rb.hiUnbounded() ? maxVal : rb.hiConstp()->مقصد_UQuad();
            إذا (lo > hi) أرجع خطأ;
            بیرون.emplace_پشت(lo, hi);
        }
        أرجع درست;
    }

    // Append one Normal bin's cross-slot interval-sets to `bins` and bump `slotCount` by an upper
    // bound of the bin's slots holding one value.  Returns false if any part isn't statically
    // enumerable (the caller then falls back to the always-safe slot count).  A non-array bin
    // is one slot covering the union of its intervals; the bins of an array element or of an
    // automatic bins declaration hold disjoint values, so the element or declaration counts once.
    منطقي appendBinCrossSlots(AstCoverBin* cbinp, uint64_t maxVal, Ast_گره_عبارت* exprp,
                             std::وکتور<std::وکتور<std::pair<uint64_t, uint64_t>>>& bins,
                             صحيح& جایگاه_شمار) {
        إذا (isAutoBins(cbinp)) {
            ++جایگاه_شمار;
            bins.افزودن_پشت({{0, maxVal}});
            أرجع exprp->عرض() <= 64;
        }
        إذا (binWith(cbinp)) {
            // A value is in one bin of a filter's, but of a sized array, in one for each range
            // list element holding it; the coverpoint's name is one element
            صحيح elements = 0;
            لكل (ثابت Ast_گره* rp = binRangesp(cbinp); rp && isSizedArray(cbinp);
                 rp = rp->nextp()) {
                ++elements;
            }
            جایگاه_شمار += std::max(1, elements);
            أرجع خطأ;
        }
        إذا (cbinp->isArray() && cbinp->isWildcard() && !cbinp->arraySizep()) {
            // A value is in at most one bin of a wildcard array: one slot covering its values.
            // Signed values are sign-extended, not unsigned intervals (see computeHitListBound)
            ++جایگاه_شمار;
            إذا (exprp->isSigned() || exprp->عرض() > 64) أرجع خطأ;
            ثابت BinRuns runs = wildcardBinRuns(cbinp, exprp, خطأ);
            إذا (runs.unsupported) أرجع خطأ;
            std::وکتور<std::pair<uint64_t, uint64_t>> ivs;
            لكل (ثابت Bin_پیاپی& پیاپی : runs.runs) {
                ivs.emplace_پشت(پیاپی.m_lo.مقصد_UQuad(), پیاپی.m_hi.مقصد_UQuad());
            }
            bins.افزودن_پشت(std::move(ivs));
            أرجع درست;
        }
        إذا (cbinp->isArray()) أرجع appendArrayBinCrossSlots(cbinp, exprp, bins, جایگاه_شمار);
        // Non-array bin: one slot covering the union of its intervals.
        ++جایگاه_شمار;
        std::وکتور<std::pair<uint64_t, uint64_t>> ivs;
        إذا (cbinp->isWildcard() || !استخراج_Range_Intervals(cbinp, maxVal, ivs)) أرجع خطأ;
        bins.افزودن_پشت(std::move(ivs));
        أرجع درست;
    }

    // Append the cross slots of an array Normal bin: each element is a slot covering its
    // values, as at most one of its single-value bins holds a value.  An element holds the
    // values arrayBinRuns() gives it: those of the coverpoint type (IEEE 1800-2023 19.5.7).
    // Elements of a signed coverpoint, non-constant elements, and elements with values beyond
    // 64 bits can't be enumerated, so they count one slot but lose exactness.  Returns false if
    // any element wasn't enumerable to exact values.
    منطقي appendArrayBinCrossSlots(AstCoverBin* cbinp, Ast_گره_عبارت* exprp,
                                  std::وکتور<std::وکتور<std::pair<uint64_t, uint64_t>>>& bins,
                                  صحيح& جایگاه_شمار) {
        // Signed values resolve sign-extended, not as unsigned intervals, and wildcard patterns
        // are not intervals
        منطقي exact = !exprp->isSigned() && !cbinp->isWildcard();
        لكل (Ast_گره* rp = cbinp->rangesp(); rp; rp = rp->nextp()) {
            ++جایگاه_شمار;
            RangeBounds rb;
            Cross_مقدار_Range range{rp, resolve_عرض(rp, exprp)};
            إذا (!exact || !constRangeBounds(rp, rb)
                || !resolve_مقدار(rp, exprp, درست, خطأ, range)) {
                exact = خطأ;
                استمر;
            }
            إذا (cross_Range_خالی(range)) استمر;  // Its bin, if any, holds no value
            إذا (range.hi.most_مجموعه_بیت_P1() > 64) {
                exact = خطأ;
                استمر;
            }
            bins.افزودن_پشت({{range.lo.مقصد_UQuad(), range.hi.مقصد_UQuad()}});
        }
        أرجع exact;
    }

    // Compute the hit-list bound for a coverpoint: the maximum number of Normal
    // bins one sample value can match.  Non-cross-fed coverpoints don't feed a cross, so
    // their hit list is unused -> 1.  Otherwise compute the exact max bin overlap; fall back
    // to the (always-safe) Normal-slot count when any bin isn't statically analyzable.
    صحيح compute_اصابت_List_Bound(AstCoverpoint* coverpointp, Ast_گره_عبارت* exprp, منطقي crossFed) {
        إذا (!crossFed) أرجع 1;
        ثابت صحيح عرض = exprp->عرض();
        ثابت uint64_t maxVal = (عرض >= 64) ? UINT64_MAX : ((1ULL << عرض) - 1);
        // One entry per Normal bin (cross slot): its covered intervals.
        std::وکتور<std::وکتور<std::pair<uint64_t, uint64_t>>> bins;
        صحيح جایگاه_شمار = 0;  // == runtime m_normal; the safe fallback bound
        // Unsigned intervals cannot establish overlap between differently sized signed values.
        منطقي exact = !exprp->isSigned();
        لكل (Ast_گره* binp = coverpointp->binsp(); binp; binp = binp->nextp()) {
            AstCoverBin* ثابت cbinp = VN_AS(binp, CoverBin);
            إذا (!cbinp->binsType().bin_Is_نرمال())
                استمر;  // ignore/illegal/default: not hit-listed
            إذا (!appendBinCrossSlots(cbinp, maxVal, exprp, bins, جایگاه_شمار)) exact = خطأ;
        }
        إذا (!exact) أرجع std::max(1, جایگاه_شمار);
        إذا (bins.خالی()) أرجع 1;
        // Max overlap occurs at some interval start; count covering bins at each lo.
        std::وکتور<uint64_t> pts;
        لكل (ثابت خودکار& b : bins)
            لكل (ثابت خودکار& iv : b) pts.افزودن_پشت(iv.اولی);
        std::مرتب_سازی(pts.begin(), pts.انتها());
        pts.erase(std::unique(pts.begin(), pts.انتها()), pts.انتها());
        صحيح maxOverlap = 1;
        لكل (ثابت uint64_t p : pts) {
            صحيح cnt = 0;
            لكل (ثابت خودکار& b : bins) {
                لكل (ثابت خودکار& iv : b) {
                    إذا (iv.اولی <= p && p <= iv.دومی) {
                        ++cnt;
                        اكسر;  // count each bin at most once
                    }
                }
            }
            إذا (cnt > maxOverlap) maxOverlap = cnt;
        }
        أرجع maxOverlap;
    }

    // A 'this->m_member' reference for embedding in an AstCStmt
    AstVarRef* memberRef(File_سطر* fl, AstVar* varp, VAccess access = VAccess::READ) {
        AstVarRef* ثابت refp = جديد AstVarRef{fl, varp, access};
        refp->self_اشاره_گر(VSelf_اشاره_گر_متن{VSelf_اشاره_گر_متن::This{}});
        أرجع refp;
    }

    // A 'this->m_member-><method>(args...)' call on one member.  usePtr is false only for
    // __Vcg_inst, which is a value handle; the item members are borrowed pointers into the
    // instance node.  Numeric arguments are AstConst; the rest are C++ text that has no AST
    // form (see ctext).
    AstCMethodHard* قلم_Call(File_سطر* fl, AstVar* varp, VCMethod روش,
                             ثابت std::وکتور<Ast_گره_عبارت*>& آرگومانها = {}, منطقي usePtr = درست) {
        AstCMethodHard* ثابت callp = جديد AstCMethodHard{fl, memberRef(fl, varp), روش};
        لكل (Ast_گره_عبارت* ثابت argp : آرگومانها) callp->جمع_کن_Pinsp(argp);
        callp->usePtr(usePtr);
        callp->dtype_مجموعه_Void();
        أرجع callp;
    }

    // An unsigned integer argument.
    ساكن AstConst* cnum(File_سطر* fl, uint32_t مقدار) { أرجع جديد AstConst{fl, مقدار}; }

    // A literal C++ argument with no AST equivalent: a 'const char*' string literal (an SV
    // string AstConst emits '"..."s', a std::string temporary the runtime cannot borrow), a
    // VlCovBinKind enum token, a constant selection-word initializer list, a VlFileLineDebug, or
    // a '__V' temporary declared by the enclosing AstCStmt.
    ساكن AstCExpr* ctext(File_سطر* fl, ثابت std::string& متن) {
        أرجع جديد AstCExpr{fl, متن};
    }

    // A C++ string literal.  Escapes control characters as the emitter does elsewhere -- bin
    // names and filenames reach the generated code verbatim when --protect-ids is off, and an
    // SV escaped identifier may hold a quote or backslash.
    ساكن std::string quoted(ثابت std::string& متن) {
        أرجع "\"" + V3_بیرون_Formatter::quoteNameControls(متن) + "\"";
    }

    // A 'VlFileLineDebug' argument: where the runtime reports an error about fl's construct
    ساكن AstCExpr* file_سطر_Debug(File_سطر* fl) {
        ثابت std::string filename
            = VIdProtect::protectIf(fl->filename(), v3Global.opt.protectIds());
        أرجع ctext(fl, "VlFileLineDebug{" + quoted(filename) + ", "
                             + std::مقصد_string(fl->lineno()) + "}");
    }

    // Check that an element of an array bin (bins b[] = {values/ranges}) is a two-state
    // constant value or range; false if not, after reporting it if 'report'.
    ساكن منطقي checkArrayBinElement(AstCoverBin* arrayBinp, Ast_گره* rangep, منطقي report = درست) {
        إذا (ثابت AstInsideRange* ثابت irp = VN_CAST(rangep, InsideRange)) {
            ثابت AstConst* ثابت minp = VN_CAST(irp->lhsp(), Const);
            ثابت AstConst* ثابت maxp = VN_CAST(irp->rhsp(), Const);
            إذا ((!minp && !VN_IS(irp->lhsp(), Unbounded))
                || (!maxp && !VN_IS(irp->rhsp(), Unbounded))) {
                إذا (report) {
                    arrayBinp->v3error("Non-constant expression in array bins range; "
                                       "range bounds must be constants (IEEE 1800-2023 19.5)");
                }
                أرجع خطأ;
            }
            إذا ((minp && minp->رقم().isFourState()) || (maxp && maxp->رقم().isFourState())) {
                إذا (report) {
                    arrayBinp->v3error("Four-state (x/z) value in array bins range bound; "
                                       "range bounds must be two-state constants");
                }
                أرجع خطأ;
            }
        } وإلا إذا (!VN_IS(rangep, Const)) {
            إذا (report) {
                arrayBinp->v3error("Non-constant expression in array bins value list; "
                                   "values must be constants (IEEE 1800-2023 19.5)");
            }
            أرجع خطأ;
        }
        أرجع درست;
    }

    // Individual equality targets of an array bin (bins b[] = {values/ranges}) of a real
    // coverpoint, in order; integral coverpoints generate array bins as runs (see arrayBinRuns).
    // An open-ended bound ('$', AstUnbounded) resolves to the coverpoint domain: '[lo:$]'
    // covers [lo:maxVal] and '[$:hi]' covers [0:hi].  One target is produced per value; ranges
    // whose resolved size would exceed --coverage-max-real-bins (e.g. an open '[lo:$]') are
    // unsupported -- emits COVERIGN, sets unsupportedOut, yields nothing.
    std::وکتور<Ast_گره_عبارت*> استخراج_Array_مقادیر(AstCoverBin* arrayBinp, Ast_گره_عبارت* exprp,
                                                 منطقي& unsupported_بیرون) {
        unsupported_بیرون = خطأ;
        ثابت صحيح عرض = exprp->عرض();
        ثابت uint64_t maxVal = (عرض >= 64) ? UINT64_MAX : ((1ULL << عرض) - 1);
        std::وکتور<Ast_گره_عبارت*> مقادیر;
        لكل (Ast_گره* rangep = arrayBinp->rangesp(); rangep; rangep = rangep->nextp()) {
            rangep = V3Const::constifyEdit(rangep);
            إذا (!checkArrayBinElement(arrayBinp, rangep)) أرجع مقادیر;
            إذا (AstInsideRange* ثابت irp = VN_CAST(rangep, InsideRange)) {
                ثابت منطقي loUnb = VN_IS(irp->lhsp(), Unbounded);
                ثابت منطقي hiUnb = VN_IS(irp->rhsp(), Unbounded);
                ثابت uint64_t lo = loUnb ? 0 : VN_AS(irp->lhsp(), Const)->مقصد_UQuad();
                ثابت uint64_t hi = hiUnb ? maxVal : VN_AS(irp->rhsp(), Const)->مقصد_UQuad();
                إذا (hi < lo) استمر;  // empty range contributes no bins
                // Guard against a '$'-bounded or otherwise huge range exploding the bin count.
                ثابت uint64_t بازه = hi - lo;  // == valueCount - 1 (no overflow: hi >= lo)
                إذا (بازه >= v3Global.opt.coverageMaxRealBins()
                    || مقادیر.اندازه() + بازه + 1 > v3Global.opt.coverageMaxRealBins()) {
                    arrayBinp->v3warn(COVERIGN,
                                      "Unsupported: array 'bins' of a real coverpoint "
                                      "covering more than "
                                          << v3Global.opt.coverageMaxRealBins() << " values; bin "
                                          << arrayBinp->prettyNameQ() << " ignored.\n"
                                          << arrayBinp->warn_بیشتر()
                                          << "... Suggest a larger --coverage-max-real-bins");
                    unsupported_بیرون = درست;
                    لكل (Ast_گره_عبارت* ثابت vp : مقادیر) VL_DO_DANGLING(افزودن_Deletep(vp), vp);
                    مقادیر.clear();
                    أرجع مقادیر;
                }
                لكل (uint64_t v = lo; v <= hi; ++v)
                    مقادیر.افزودن_پشت(جديد AstConst{irp->fileline(), AstConst::Widthed_مقدار{}, عرض,
                                                  static_cast<uint32_t>(v)});
            } وإلا {
                مقادیر.افزودن_پشت(VN_AS(rangep->clone_درخت(خطأ), گره_عبارت));
            }
        }
        أرجع مقادیر;
    }

    ساكن صحيح پیاپی_عرض(ثابت Ast_گره_عبارت* exprp) { أرجع exprp->عرض() + 1; }

    // Automatic bins partition the coverpoint domain in value order (IEEE 1800-2023 19.5.3): N
    // bins, capped at the number of values, each hold 2^width / N values, and the last bin also
    // holds the remainder.  False for an invalid declaration, already reported.
    منطقي autoBinRuns(AstCoverBin* binp, Ast_گره_عبارت* exprp, BinRuns& بیرون) {
        ثابت uint32_t requested = autoBinsRequested(binp);
        إذا (!requested || !exprp->dtypep()->skipRefp()->isIntegralOrPacked()) أرجع خطأ;
        ثابت صحيح عرض = exprp->عرض();
        ثابت صحيح arithmetic_عرض = پیاپی_عرض(exprp);
        ثابت uint32_t شمار
            = عرض < 32
                  ? static_cast<uint32_t>(std::min<uint64_t>(uint64_t{1} << عرض, requested))
                  : requested;
        Bin_پیاپی پیاپی{binp, arithmetic_عرض, شمار};
        V3_عدد مجموع{binp, arithmetic_عرض};
        مجموع.مجموعه_بیت(عرض, 1);
        پیاپی.m_stride.opDiv(مجموع, V3_عدد{binp, arithmetic_عرض, شمار});
        ثابت Cross_مقدار_Range domain
            = cross_مقدار_Domain(binp, عرض, exprp->isSigned(), arithmetic_عرض);
        پیاپی.m_lo = domain.lo;
        پیاپی.m_hi = domain.hi;
        بیرون.runs.افزودن_پشت(std::move(پیاپی));
        بیرون.شمار = شمار;
        أرجع درست;
    }

    // The elements of an array bin (bins b[] = {values/ranges}), in order, as runs of
    // single-value bins.  A range holds the values of the coverpoint type it contains (IEEE
    // 1800-2023 19.5.7), while a singleton names a bin even without such a value.  Errors on a
    // non-constant element.  More than --coverage-max-bins bins (e.g. an open '[lo:$]' range over
    // a wide coverpoint) are unsupported -- emits COVERIGN, and sets unsupported.
    BinRuns arrayBinRuns(AstCoverBin* arrayBinp, Ast_گره_عبارت* exprp) {
        BinRuns بیرون;
        ثابت صحيح عرض = پیاپی_عرض(exprp);
        لكل (Ast_گره* rangep = arrayBinp->rangesp(); rangep; rangep = rangep->nextp()) {
            rangep = V3Const::constifyEdit(rangep);
            إذا (!checkArrayBinElement(arrayBinp, rangep)) أرجع بیرون;
            ثابت AstInsideRange* ثابت irp = VN_CAST(rangep, InsideRange);
            Cross_مقدار_Range range{rangep, resolve_عرض(rangep, exprp)};
            منطقي خالی = درست;
            إذا (!resolve_مقدار(rangep, exprp, درست, خطأ, range)) {
                rangep->v3warn(E_UNSUPPORTED, "Unsupported: non-integral value in a coverage bin "
                                              "of an integral coverpoint.");
            } وإلا {
                خالی = cross_Range_خالی(range);
            }
            إذا (خالی && irp) استمر;  // A range without values contributes no bins
            Bin_پیاپی پیاپی{rangep, عرض, 1};
            پیاپی.m_خالی = خالی;
            uint64_t شمار = 1;
            إذا (!خالی) {
                پیاپی.m_lo.op_تخصیص_بده(range.lo);
                پیاپی.m_hi.op_تخصیص_بده(range.hi);
                V3_عدد بازه{rangep, عرض};
                بازه.opSub(پیاپی.m_hi, پیاپی.m_lo);
                // Wider spans exceed any limit
                شمار = بازه.most_مجموعه_بیت_P1() > 32 ? UINT64_MAX : بازه.مقصد_UQuad() + 1;
            }
            إذا (شمار > v3Global.opt.coverageMaxBins() - بیرون.شمار) {
                arrayBinp->v3warn(COVERIGN, "Unsupported: array 'bins' covering more than "
                                                << v3Global.opt.coverageMaxBins()
                                                << " values (e.g. an open '[lo:$]' range over "
                                                   "a wide coverpoint); bin "
                                                << arrayBinp->prettyNameQ() << " ignored\n"
                                                << arrayBinp->warn_بیشتر()
                                                << "... Suggest a larger --coverage-max-bins");
                بیرون.runs.clear();
                بیرون.شمار = 0;
                بیرون.unsupported = درست;
                أرجع بیرون;
            }
            پیاپی.m_شمار = static_cast<uint32_t>(شمار);
            بیرون.شمار += پیاپی.m_شمار;
            بیرون.runs.افزودن_پشت(std::move(پیاپی));
        }
        أرجع بیرون;
    }

    // The bins of a wildcard array (wildcard bins b[] = {...}): one for each coverpoint value
    // an element matches (IEEE 1800-2023 19.5.4, 19.5.7), in value order, and named by the value
    // (19.5.1), as runs of single-value bins.  Errors on a non-constant element.  More than
    // --coverage-max-bins values are unsupported -- emits COVERIGN, and sets unsupported, and for
    // an ignore or illegal array, single.  'report' false omits these diagnostics.
    ساكن BinRuns wildcardBinRuns(AstCoverBin* arrayBinp, Ast_گره_عبارت* exprp, منطقي report) {
        BinRuns بیرون;
        ثابت صحيح عرض = پیاپی_عرض(exprp);
        ثابت V3_عدد one{arrayBinp, عرض, 1};
        // Disjoint runs of the values, in value order, each value once
        std::وکتور<std::pair<V3_عدد, V3_عدد>> spans;
        uint64_t شمار = 0;  // Values of 'spans'
        لكل (Ast_گره* rangep = arrayBinp->rangesp(); rangep; rangep = rangep->nextp()) {
            rangep = V3Const::constifyEdit(rangep);
            إذا (!checkArrayBinElement(arrayBinp, rangep, report)) {
                بیرون.unsupported = درست;
                أرجع بیرون;
            }
            Cross_مقدار_Range range{rangep, resolve_عرض(rangep, exprp)};
            إذا (!resolve_مقدار(rangep, exprp, درست, درست, range)) {
                إذا (report) {
                    rangep->v3warn(E_UNSUPPORTED, "Unsupported: non-integral value in a "
                                                  "coverage bin of an integral coverpoint.");
                }
                استمر;
            }
            إذا (cross_Range_خالی(range)) استمر;
            std::وکتور<std::pair<V3_عدد, V3_عدد>> یافت;
            crossRangeRuns(range, v3Global.opt.coverageMaxBins(), یافت);
            لكل (ثابت std::pair<V3_عدد, V3_عدد>& پیاپی : یافت) {
                // Coverpoint values, sign-extended in both widths
                spans.emplace_پشت(V3_عدد{rangep, عرض, پیاپی.اولی},
                                   V3_عدد{rangep, عرض, پیاپی.دومی});
            }
            std::مرتب_سازی(spans.begin(), spans.انتها(), [](ثابت خودکار& lhs, ثابت خودکار& rhs) {
                أرجع cross_مقدار_Less(lhs.اولی, rhs.اولی);
            });
            std::وکتور<std::pair<V3_عدد, V3_عدد>> merged;
            لكل (ثابت std::pair<V3_عدد, V3_عدد>& بازه : spans) {
                // Adjacent or overlapping values join a run; 'first - 1' cannot overflow
                V3_عدد before{rangep, عرض};
                before.opSub(بازه.اولی, one);
                إذا (merged.خالی() || cross_مقدار_Less(merged.پشت().دومی, before)) {
                    merged.افزودن_پشت(بازه);
                } وإلا إذا (cross_مقدار_Less(merged.پشت().دومی, بازه.دومی)) {
                    merged.پشت().دومی = بازه.دومی;
                }
            }
            شمار = 0;
            لكل (ثابت std::pair<V3_عدد, V3_عدد>& بازه : merged) {
                V3_عدد اندازه{rangep, عرض};
                اندازه.opSub(بازه.دومی, بازه.اولی);
                // Beyond 2^32 values exceed any limit
                شمار += اندازه.most_مجموعه_بیت_P1() > 32 ? uint64_t{1} << 33 : اندازه.مقصد_UQuad() + 1;
                إذا (شمار > v3Global.opt.coverageMaxBins()) اكسر;
            }
            spans = std::move(merged);
            إذا (شمار > v3Global.opt.coverageMaxBins()) {
                // An ignore or illegal array still excludes or checks its values, as one bin
                بیرون.single = !arrayBinp->binsType().bin_Is_نرمال();
                إذا (report) {
                    arrayBinp->v3warn(
                        COVERIGN, "Unsupported: wildcard array '"
                                      << arrayBinp->binsType().verilogKwd()
                                      << "' of more than --coverage-max-bins of "
                                      << v3Global.opt.coverageMaxBins() << " values; bin "
                                      << arrayBinp->prettyNameQ()
                                      << (بیرون.single ? " treated as one bin" : " ignored") << "\n"
                                      << arrayBinp->warn_بیشتر()
                                      << "... Suggest a larger --coverage-max-bins");
                }
                بیرون.unsupported = درست;
                أرجع بیرون;
            }
        }
        لكل (ثابت std::pair<V3_عدد, V3_عدد>& بازه : spans) {
            بیرون.runs.emplace_پشت(arrayBinp, عرض, 0);
            Bin_پیاپی& پیاپی = بیرون.runs.پشت();
            پیاپی.m_lo = بازه.اولی;
            پیاپی.m_hi = بازه.دومی;
            // A bin for each value, named by it in the coverpoint's type
            V3_عدد مقدار = بازه.اولی;
            طالما (درست) {
                ثابت V3_عدد typed{arrayBinp, exprp->عرض(), مقدار};
                بیرون.مقادیر.افزودن_پشت(exprp->isSigned() ? typed.مقصد_Decimal_S() : typed.مقصد_Decimal_U());
                ++پیاپی.m_شمار;
                إذا (مقدار.isCaseEq(بازه.دومی)) اكسر;
                V3_عدد بعدی{arrayBinp, عرض};
                مقدار = بعدی.op_جمع_کن(مقدار, one);
            }
        }
        بیرون.شمار = static_cast<uint32_t>(شمار);
        أرجع بیرون;
    }

    // The runs of an automatic bins declaration, or of an array bin of an integral coverpoint.
    // False for other bins, which do not generate as runs, including a wildcard array then one
    // bin (see BinRuns::single).
    منطقي binRunsFor(AstCoverBin* binp, Ast_گره_عبارت* exprp, BinRuns& بیرون) {
        إذا (isAutoBins(binp)) {
            إذا (!autoBinRuns(binp, exprp, بیرون)) بیرون.unsupported = درست;
            أرجع درست;
        }
        إذا (binp->isArray() && binp->isWildcard()) {
            إذا (!exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
                Ast_گره_عبارت* ثابت falsep = wildcardTypeError(binp, exprp);
                VL_DO_DANGLING(افزودن_Deletep(falsep), falsep);
                بیرون.unsupported = درست;
            } وإلا {
                بیرون = wildcardBinRuns(binp, exprp, درست);
                إذا (بیرون.single) {
                    binp->isArray(خطأ);  // Generates as one bin
                    أرجع خطأ;
                }
            }
            أرجع درست;
        }
        إذا (!binp->isArray() || binp->transp() || binp->isWildcard()
            || !exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
            أرجع خطأ;
        }
        بیرون = arrayBinRuns(binp, exprp);
        أرجع درست;
    }

    // Emit a 'this->m_cp->addSingleNamer/addArrayNamer(...)' statement for one bin whose first
    // runtime bin index is 'declared'; or with 'valueNames', those naming each bin of an array
    Ast_گره_Stmt* makeNamer(AstVar* cpVarp, AstCoverBin* binp, int64_t شمار, uint32_t declared,
                           ثابت std::وکتور<Ast_گره_عبارت*>& مقادیر = {},
                           ثابت std::وکتور<std::string>& مقدار_نامها = {}) {
        File_سطر* ثابت fl = binp->fileline();
        CoverpointBins& bins = m_cpBins.at(cpVarp);
        ثابت uint32_t نرمال_شمار
            = binp->binsType().bin_Is_نرمال() ? static_cast<uint32_t>(شمار < 0 ? 1 : شمار) : 0;
        ثابت Bin_بازه بازه{bins.مجموع, نرمال_شمار, declared};
        إذا (binp->binsType() == VCoverBinsType::BINS_AUTO_IMPLICIT) {
            bins.implicitAuto = بازه;  // Selected by bin, see implicitAutoBinSpan
        } وإلا {
            bins.spans.emplace(binp->name(), بازه);
        }
        bins.مجموع += نرمال_شمار;
        لكل (uint32_t i = 0; bins.crossed && i < نرمال_شمار; ++i) {
            bins.مقادیر.افزودن_پشت({binp, مقادیر.خالی() ? اشاره_تهی : مقادیر[i]});
        }
        // Under --protect-ids the filename and bin name flow into the coverage database
        // verbatim, so obfuscate them exactly as line/toggle coverage points are (whole-
        // unit filename, per-word bin name).  A no-op when --protect-ids is off.
        ثابت منطقي prot = v3Global.opt.protectIds();
        ثابت منطقي single = شمار < 0;
        إذا (!مقدار_نامها.خالی()) {
            // A bin of a wildcard array is named by its value (IEEE 1800-2023 19.5.1)
            Ast_گره_Stmt* stmtsp = اشاره_تهی;
            لكل (ثابت std::string& مقدار : مقدار_نامها) {
                ثابت std::string name
                    = VIdProtect::protect_واژگان_If(binp->name(), prot) + "[" + مقدار + "]";
                stmtsp = Ast_گره::جمع_کن_بعدی(
                    stmtsp,
                    قلم_Call(fl, cpVarp, VCMethod::COVERGROUP_جمع_کن_SINGLE_NAMER,
                             {ctext(fl, binp->binsType().bin_مجموعه_Enum()), ctext(fl, quoted(name)),
                              ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), prot))),
                              cnum(fl, static_cast<uint32_t>(fl->lineno())),
                              cnum(fl, static_cast<uint32_t>(fl->اولی_ستونی()))})
                        ->makeStmt());
            }
            أرجع stmtsp;
        }
        std::وکتور<Ast_گره_عبارت*> آرگومانها{ctext(fl, binp->binsType().bin_مجموعه_Enum())};
        إذا (!single) آرگومانها.افزودن_پشت(cnum(fl, static_cast<uint32_t>(شمار)));  // value array bin
        آرگومانها.افزودن_پشت(ctext(fl, quoted(VIdProtect::protect_واژگان_If(binp->name(), prot))));
        آرگومانها.افزودن_پشت(ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), prot))));
        آرگومانها.افزودن_پشت(cnum(fl, static_cast<uint32_t>(fl->lineno())));
        آرگومانها.افزودن_پشت(cnum(fl, static_cast<uint32_t>(fl->اولی_ستونی())));
        أرجع قلم_Call(fl, cpVarp,
                        single ? VCMethod::COVERGROUP_جمع_کن_SINGLE_NAMER
                        : binp->binsType() == VCoverBinsType::BINS_AUTO_IMPLICIT
                            ? VCMethod::COVERGROUP_جمع_کن_NUMBERED_NAMER
                            : VCMethod::COVERGROUP_جمع_کن_ARRAY_NAMER,
                        آرگومانها)
            ->makeStmt();
    }

    // Emit 'if (iff && cond) m_cp.incrementBin(idx);' (or recordHit, + illegal action) in sample()
    // Where a bin's hit is recorded in the runtime VlCoverpoint member.
    بنية Conv_Bin_هدف final {
        AstVar* cpVarp;  // the __Vcp_<coverpoint> member
        uint32_t idx;  // bin index within that coverpoint
        منطقي is_نرمال;  // Normal -> incrementBin (count + cross hit list); else recordHit (count)
    };

    // Emit 'this->m_cp.incrementBin(idx);' (Normal) or '.recordHit(idx);'
    // (ignore/illegal/default).
    Ast_گره_Stmt* make_Runtime_Bin_اصابت(File_سطر* fl, AstVar* cpVarp, Ast_گره_عبارت* idxp,
                                   منطقي is_نرمال) {
        أرجع قلم_Call(fl, cpVarp,
                        is_نرمال ? VCMethod::COVERGROUP_INCREMENT_BIN
                                 : VCMethod::COVERGROUP_رکورد_اصابت,
                        {idxp})
            ->makeStmt();
    }
    Ast_گره_Stmt* make_Runtime_Bin_اصابت(File_سطر* fl, ثابت Conv_Bin_هدف& tgt) {
        أرجع make_Runtime_Bin_اصابت(fl, tgt.cpVarp, cnum(fl, static_cast<uint32_t>(tgt.idx)),
                                 tgt.is_نرمال);
    }

    // The condition under which a bin counts a sample: condp, and the bin's iff, and for a
    // Normal or default state bin, that the value is not excluded
    Ast_گره_عبارت* binCondition(AstCoverBin* binp, AstVar* cpVarp, Ast_گره_عبارت* condp) {
        File_سطر* ثابت fl = binp->fileline();
        إذا (binp->iffp()) condp = جديد AstLogAnd{fl, binp->iffp()->clone_درخت(خطأ), condp};
        ثابت خودکار excluded = m_excludedVars.پیدا_کن(cpVarp);
        إذا (excluded != m_excludedVars.انتها() && !binp->transp()
            && (binp->binsType().bin_Is_نرمال()
                || binp->binsType() == VCoverBinsType::BINS_DEFAULT)) {
            condp = جديد AstLogAnd{
                fl, جديد AstNot{fl, جديد AstVarRef{fl, excluded->دومی, VAccess::READ}}, condp};
        }
        أرجع condp;
    }

    فراغ emit_Conv_اصابت_If(AstCoverpoint* coverpointp, AstCoverBin* binp, AstVar* cpVarp,
                       Ast_گره_عبارت* idxp, Ast_گره_عبارت* condp) {
        File_سطر* ثابت fl = binp->fileline();
        Ast_گره* actionp = make_Runtime_Bin_اصابت(fl, cpVarp, idxp, binp->binsType().bin_Is_نرمال());
        إذا (binp->binsType() == VCoverBinsType::BINS_ILLEGAL) {
            actionp->جمع_کن_بعدی(makeIllegalBinAction(fl, "Illegal bin " + binp->prettyNameQ()
                                                          + " hit in coverpoint "
                                                          + coverpointp->prettyNameQ()));
        }
        UASSERT_OBJ(m_sampleFuncp, binp, "sample() CFunc not set for coverpoint");
        جمع_کن_Sample_Stmt(جديد AstIf{fl, binCondition(binp, cpVarp, condp), actionp, اشاره_تهی});
    }

    // Emit the sample() code of sized array 'sized': count the value in its bins holding it when
    // enabled, as other bins are, and for default bins note in matchedp whether any holds it
    فراغ emitSizedSample(AstCoverpoint* coverpointp, AstCoverBin* binp, AstVar* cpVarp,
                         Ast_گره_عبارت* exprp, uint32_t sized, AstVar* matchedp) {
        File_سطر* ثابت fl = binp->fileline();
        Ast_گره_عبارت* enabledp = binCondition(binp, cpVarp, جديد AstConst{fl, AstConst::بیت_True{}});
        UASSERT_OBJ(m_sampleFuncp, binp, "sample() CFunc not set for coverpoint");
        ثابت منطقي illegal = binp->binsType() == VCoverBinsType::BINS_ILLEGAL;
        إذا (illegal) {
            // The illegal action reads the condition too, which is evaluated once, as the guard
            // may have side effects
            AstVar* ثابت varp
                = جديد AstVar{fl, VVarType::BLOCKTEMP,
                             "__VcpEnabled_" + sanitizeGeneratedName(coverpointp->name()) + "_"
                                 + cvt_مقصد_Str(sized),
                             binp->پیدا_کن_بیت_DType()};
            varp->funcLocal(درست);
            m_sampleFuncp->جمع_کن_Stmtsp(varp);
            جمع_کن_Sample_Stmt(جديد Ast_تخصیص_بده{fl, جديد AstVarRef{fl, varp, VAccess::WRITE}, enabledp});
            enabledp = جديد AstVarRef{fl, varp, VAccess::READ};
        }
        AstCMethodHard* ثابت callp
            = قلم_Call(fl, cpVarp,
                       exprp->isWide() ? VCMethod::COVERGROUP_SIZED_SAMPLE_W
                                       : VCMethod::COVERGROUP_SIZED_SAMPLE,
                       {cnum(fl, sized), exprp->clone_درخت(خطأ), enabledp});
        callp->dtype_مجموعه_بیت();
        إذا (illegal) {
            جمع_کن_Sample_Stmt(جديد AstIf{fl, جديد AstLogAnd{fl, callp, enabledp->clone_درخت(خطأ)},
                                    makeIllegalBinAction(fl, "Illegal bin " + binp->prettyNameQ()
                                                                 + " hit in coverpoint "
                                                                 + coverpointp->prettyNameQ())});
        } وإلا إذا (matchedp && binp->binsType().bin_Is_نرمال()) {
            جمع_کن_Sample_Stmt(
                جديد Ast_تخصیص_بده{fl, جديد AstVarRef{fl, matchedp, VAccess::WRITE},
                              جديد AstOr{fl, جديد AstVarRef{fl, matchedp, VAccess::READ}, callp}});
        } وإلا {
            جمع_کن_Sample_Stmt(callp->makeStmt());
        }
    }

    // A variable local to the constructor
    AstVar* constructor_موقت(File_سطر* fl, ثابت string& name, Ast_گره_DType* dtypep) {
        AstVar* ثابت varp = جديد AstVar{fl, VVarType::BLOCKTEMP, name, dtypep};
        varp->funcLocal(درست);
        m_constructorp->جمع_کن_Stmtsp(varp);
        أرجع varp;
    }

    // Truncate or extend, as its signedness sets, a value to a type
    ساكن Ast_گره_عبارت* resize_مقدار(Ast_گره_عبارت* valuep, Ast_گره_DType* dtypep) {
        File_سطر* ثابت fl = valuep->fileline();
        إذا (valuep->عرض() > dtypep->عرض()) {
            valuep = جديد AstSel{fl, valuep, 0, dtypep->عرض()};
        } وإلا إذا (valuep->عرض() < dtypep->عرض()) {
            valuep = valuep->isSigned()
                         ? static_cast<Ast_گره_عبارت*>(جديد AstExtendS{fl, valuep, dtypep->عرض()})
                         : جديد AstExtend{fl, valuep, dtypep->عرض()};
        }
        valuep->dtypep(dtypep);
        أرجع valuep;
    }

    // The runtime arguments of a count, which the constructor evaluates once into a temporary:
    // its value, a count wider than 64 bits saturated over any limit, and whether it is positive
    std::pair<Ast_گره_عبارت*, Ast_گره_عبارت*> new_شمار_آرگومانها(File_سطر* fl, ثابت string& name,
                                                       Ast_گره_عبارت* exprp) {
        AstVar* ثابت countp = constructor_موقت(fl, name, exprp->dtypep());
        m_constructorp->جمع_کن_Stmtsp(
            جديد Ast_تخصیص_بده{fl, جديد AstVarRef{fl, countp, VAccess::WRITE}, exprp->clone_درخت(خطأ)});
        ثابت خودکار شمار_Ref = [&]() { أرجع جديد AstVarRef{fl, countp, VAccess::READ}; };
        AstConst* ثابت zerop = جديد AstConst{fl, AstConst::DTyped{}, countp->dtypep()};
        Ast_گره_عبارت* ثابت positivep
            = countp->isSigned() ? static_cast<Ast_گره_عبارت*>(جديد AstGtS{fl, شمار_Ref(), zerop})
                                 : جديد AstNeq{fl, شمار_Ref(), zerop};
        Ast_گره_عبارت* valuep = resize_مقدار(شمار_Ref(), countp->پیدا_کن_UInt64_DType());
        إذا (countp->عرض() > VL_QUADSIZE) {
            valuep = جديد AstCond{
                fl,
                جديد AstRedOr{
                    fl, جديد AstSel{fl, شمار_Ref(), VL_QUADSIZE, countp->عرض() - VL_QUADSIZE}},
                جديد AstConst{fl, AstConst::Unsized64{}, std::numeric_limits<uint64_t>::max()},
                valuep};
            valuep->dtype_مجموعه_UInt64();
        }
        أرجع {valuep, positivep};
    }
    // The runtime arguments of a constant count
    ساكن std::pair<Ast_گره_عبارت*, Ast_گره_عبارت*> new_شمار_آرگومانها(File_سطر* fl, uint64_t شمار) {
        أرجع {جديد AstConst{fl, AstConst::Unsized64{}, شمار},
                جديد AstConst{fl, AstConst::بیت_True{}}};
    }

    // The values of a coverpoint, which its name denotes (IEEE 1800-2023 19.5.1.1), as runs in
    // value order: an enumerated type's values (6.19), else all values of its type.  Bounds
    // are at runWidth(), sign-extended like a CrossValueRange's.
    ساكن std::وکتور<std::pair<V3_عدد, V3_عدد>> coverpoint_مقادیر(Ast_گره* nodep,
                                                                       Ast_گره_عبارت* exprp) {
        ثابت صحيح عرض = پیاپی_عرض(exprp);
        std::وکتور<std::pair<V3_عدد, V3_عدد>> runs;
        ثابت AstEnumDType* ثابت enump = VN_CAST(exprp->dtypep()->skip_Ref_مقصد_Enump(), EnumDType);
        إذا (!enump) {
            ثابت Cross_مقدار_Range domain
                = cross_مقدار_Domain(nodep, exprp->عرض(), exprp->isSigned(), عرض);
            runs.emplace_پشت(domain.lo, domain.hi);
            أرجع runs;
        }
        std::وکتور<V3_عدد> مقادیر;
        لكل (ثابت Ast_Enum_قلم* itemp = enump->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Enum_قلم)) {
            ثابت V3_عدد& رقم = VN_AS(itemp->valuep(), Const)->رقم();
            إذا (رقم.isFourState()) استمر;  // Not a coverpoint value (19.5.7)
            مقادیر.emplace_پشت(nodep, عرض);
            إذا (exprp->isSigned()) {
                مقادیر.پشت().opExtendS(رقم, رقم.عرض());
            } وإلا {
                مقادیر.پشت().op_تخصیص_بده(رقم);
            }
        }
        std::مرتب_سازی(مقادیر.begin(), مقادیر.انتها(), cross_مقدار_Less);
        ثابت V3_عدد one{nodep, عرض, 1};
        لكل (ثابت V3_عدد& مقدار : مقادیر) {
            V3_عدد بعدی{nodep, عرض};
            إذا (!runs.خالی() && بعدی.op_جمع_کن(runs.پشت().دومی, one).isCaseEq(مقدار)) {
                runs.پشت().دومی = مقدار;
            } وإلا {
                runs.emplace_پشت(مقدار, مقدار);
            }
        }
        أرجع runs;
    }

    // Emit the constructor code building the bins 'binp' whose values it computes: the values of
    // each element that are coverpoint values (IEEE 1800-2023 19.5.7), those a 'with' filter
    // keeps (19.5.1.1), then its bins
    فراغ generateConstructedBins(AstCoverpoint* coverpointp, AstCoverBin* binp, AstVar* cpVarp,
                                 Ast_گره_عبارت* exprp) {
        File_سطر* ثابت fl = binp->fileline();
        ثابت string پیشوند
            = m_sized_نامها.get(sanitizeGeneratedName(coverpointp->name() + "__" + binp->name()));
        // The count, and whether it is positive; one of a filter's scalar bin, or bin per value
        ثابت std::pair<Ast_گره_عبارت*, Ast_گره_عبارت*> شمار
            = binp->arraySizep() ? new_شمار_آرگومانها(fl, پیشوند + "_count", binp->arraySizep())
                                 : new_شمار_آرگومانها(fl, 1);
        AstCoverWith* ثابت withp = binWith(binp);
        إذا (withp && !binRangesp(binp)) {  // The coverpoint's name: all of its values
            لكل (ثابت std::pair<V3_عدد, V3_عدد>& پیاپی : coverpoint_مقادیر(binp, exprp)) {
                m_constructorp->جمع_کن_Stmtsp(قلم_Call(fl, cpVarp,
                                                   exprp->isWide()
                                                       ? VCMethod::COVERGROUP_SIZED_RANGE_W
                                                       : VCMethod::COVERGROUP_SIZED_RANGE,
                                                   {new_مقدار_Const(fl, پیاپی.اولی, exprp),
                                                    new_مقدار_Const(fl, پیاپی.دومی, exprp)})
                                              ->makeStmt());
            }
        }
        uint32_t element = 0;
        لكل (Ast_گره* rangep = binRangesp(binp); rangep; rangep = rangep->nextp()) {
            إذا (VN_IS(rangep, Unbounded)) {  // A parameter of '$'
                binp->v3error("Bins value may not be '$', which may only bound a range "
                              "(IEEE 1800-2023 6.20.7)");
                استمر;
            }
            generateSizedElement(cpVarp, binp, rangep, exprp, پیشوند + "_" + cvt_مقصد_Str(element++));
        }
        إذا (withp) generateWithFilter(binp, withp, cpVarp, exprp, پیشوند);
        std::وکتور<Ast_گره_عبارت*> آرگومانها{ctext(fl, binp->binsType().bin_مجموعه_Enum()), شمار.اولی,
                                       شمار.دومی};
        // A filter's bins have the limit it began with
        إذا (!withp) آرگومانها.افزودن_پشت(cnum(fl, v3Global.opt.coverageMaxBins()));
        ثابت منطقي prot = v3Global.opt.protectIds();
        آرگومانها.افزودن_پشت(ctext(fl, quoted(VIdProtect::protect_واژگان_If(binp->name(), prot))));
        آرگومانها.افزودن_پشت(ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), prot))));
        آرگومانها.افزودن_پشت(cnum(fl, static_cast<uint32_t>(fl->lineno())));
        آرگومانها.افزودن_پشت(cnum(fl, static_cast<uint32_t>(fl->اولی_ستونی())));
        m_constructorp->جمع_کن_Stmtsp(
            قلم_Call(fl, cpVarp,
                     withp ? VCMethod::COVERGROUP_WITH_پایان : VCMethod::COVERGROUP_SIZED_پایان,
                     آرگومانها)
                ->makeStmt());
    }

    // Emit the constructor code evaluating the 'with' filter of 'binp' for each of its candidate
    // values, which sizedRange() added, passing the runs of values it keeps (IEEE 1800-2023
    // 19.5.1.1).  The filter is evaluated in a loop, whose code does not grow with the elements:
    //   withBegin(grouping, limit);
    //   more = 1;
    //   while (withNext()) {
    //       value = withLo(); last = withHi(); run = 0;
    //       while (true) {
    //           item = value;
    //           if (filter) { if (!run) { first = value; run = 1; } }
    //           else if (run) { more = withRun(first, value - 1); run = 0; }
    //           if (!more || value == last) break;
    //           ++value;
    //       }
    //       if (run) more = withRun(first, last);
    //   }
    فراغ generateWithFilter(AstCoverBin* binp, AstCoverWith* withp, AstVar* cpVarp,
                            Ast_گره_عبارت* exprp, ثابت string& پیشوند) {
        File_سطر* ثابت fl = withp->fileline();
        ثابت منطقي wide = exprp->isWide();
        ثابت string grouping = !binp->isArray()     ? "Single"
                                : binp->arraySizep() ? "Fixed"
                                                     : "Values";
        m_constructorp->جمع_کن_Stmtsp(قلم_Call(fl, cpVarp, VCMethod::COVERGROUP_WITH_BEGIN,
                                           {ctext(fl, "VlCovBinGrouping::" + grouping),
                                            cnum(fl, v3Global.opt.coverageMaxBins())})
                                      ->makeStmt());
        // The candidates count in the coverpoint's width, and the filter reads each as 'item',
        // of the coverpoint's type, so that a filter changing 'item' cannot change the loop
        Ast_گره_DType* ثابت مقدار_DTypep = exprp->dtypep();
        AstVar* ثابت valuep = constructor_موقت(fl, پیشوند + "_value", مقدار_DTypep);
        AstVar* ثابت lastp = constructor_موقت(fl, پیشوند + "_last", مقدار_DTypep);
        AstVar* ثابت firstp = constructor_موقت(fl, پیشوند + "_first", مقدار_DTypep);
        AstVar* ثابت runp = constructor_موقت(fl, پیشوند + "_run", binp->پیدا_کن_بیت_DType());
        AstVar* ثابت morep = constructor_موقت(fl, پیشوند + "_more", binp->پیدا_کن_بیت_DType());
        AstVar* ثابت itemp = withp->itemp()->unlink_Fr_پشت();
        itemp->name(پیشوند + "_item");
        m_constructorp->جمع_کن_Stmtsp(itemp);
        ثابت خودکار ref = [&](AstVar* varp) { أرجع جديد AstVarRef{fl, varp, VAccess::READ}; };
        ثابت خودکار تخصیص_بده_2 = [&](AstVar* varp, Ast_گره_عبارت* rhsp) -> Ast_گره* {
            أرجع جديد Ast_تخصیص_بده{fl, جديد AstVarRef{fl, varp, VAccess::WRITE}, rhsp};
        };
        ثابت خودکار flag = [&](AstVar* varp, منطقي مقدار) {
            أرجع تخصیص_بده_2(varp, مقدار ? جديد AstConst{fl, AstConst::بیت_True{}}
                                      : جديد AstConst{fl, AstConst::بیت_False{}});
        };
        ثابت خودکار bound = [&](VCMethod narrow, VCMethod wide_روش, AstVar* varp) -> Ast_گره* {
            إذا (wide) {
                أرجع قلم_Call(fl, cpVarp, wide_روش, {جديد AstVarRef{fl, varp, VAccess::WRITE}})
                    ->makeStmt();
            }
            AstCMethodHard* ثابت callp = قلم_Call(fl, cpVarp, narrow);
            callp->dtype_مجموعه_UInt64();
            أرجع تخصیص_بده_2(varp, resize_مقدار(callp, مقدار_DTypep));
        };
        ثابت خودکار keep = [&](Ast_گره_عبارت* lop, Ast_گره_عبارت* hip) {
            AstCMethodHard* ثابت callp = قلم_Call(
                fl, cpVarp, wide ? VCMethod::COVERGROUP_WITH_پیاپی_W : VCMethod::COVERGROUP_WITH_پیاپی,
                {lop, hip});
            callp->dtype_مجموعه_بیت();
            أرجع تخصیص_بده_2(morep, callp);
        };
        ثابت خودکار گام = [&](منطقي به_بالا) {
            AstConst* ثابت onep = جديد AstConst{fl, AstConst::Widthed_مقدار{}, exprp->عرض(), 1};
            Ast_گره_عبارت* ثابت stepp
                = به_بالا ? static_cast<Ast_گره_عبارت*>(جديد Ast_جمع_کن{fl, ref(valuep), onep})
                     : جديد AstSub{fl, ref(valuep), onep};
            stepp->dtypep(مقدار_DTypep);
            أرجع stepp;
        };
        AstLoop* ثابت innerp = جديد AstLoop{fl};
        innerp->جمع_کن_Stmtsp(تخصیص_بده_2(itemp, ref(valuep)));
        innerp->جمع_کن_Stmtsp(جديد AstIf{
            fl, withp->filterp()->unlink_Fr_پشت(),
            جديد AstIf{fl, جديد AstNot{fl, ref(runp)},
                      تخصیص_بده_2(firstp, ref(valuep))->جمع_کن_بعدی(flag(runp, درست))},
            جديد AstIf{fl, ref(runp), keep(ref(firstp), گام(خطأ))->جمع_کن_بعدی(flag(runp, خطأ))}});
        innerp->جمع_کن_Stmtsp(جديد AstLoopTest{
            fl, innerp, جديد AstLogAnd{fl, ref(morep), جديد AstNeq{fl, ref(valuep), ref(lastp)}}});
        innerp->جمع_کن_Stmtsp(تخصیص_بده_2(valuep, گام(درست)));
        AstLoop* ثابت outerp = جديد AstLoop{fl};
        AstCMethodHard* ثابت nextp = قلم_Call(fl, cpVarp, VCMethod::COVERGROUP_WITH_بعدی);
        nextp->dtype_مجموعه_بیت();
        outerp->جمع_کن_Stmtsp(جديد AstLoopTest{fl, outerp, nextp});
        outerp->جمع_کن_Stmtsp(
            bound(VCMethod::COVERGROUP_WITH_LO, VCMethod::COVERGROUP_WITH_LO_W, valuep));
        outerp->جمع_کن_Stmtsp(
            bound(VCMethod::COVERGROUP_WITH_HI, VCMethod::COVERGROUP_WITH_HI_W, lastp));
        outerp->جمع_کن_Stmtsp(flag(runp, خطأ));
        outerp->جمع_کن_Stmtsp(innerp);
        outerp->جمع_کن_Stmtsp(جديد AstIf{fl, ref(runp), keep(ref(firstp), ref(lastp))});
        m_constructorp->جمع_کن_Stmtsp(flag(morep, درست));
        m_constructorp->جمع_کن_Stmtsp(outerp);
    }

    // Emit 'sizedRange(lo, hi)' for the coverpoint values of an element of bins 'binp', whose
    // values the constructor computes: resolved now if constant, else when constructed by
    // clipping to the coverpoint's values.  A wildcard pattern's values give a range for each
    // run of them.  'prefix' names its temporaries.
    فراغ generateSizedElement(AstVar* cpVarp, ثابت AstCoverBin* binp, Ast_گره* rangep,
                              Ast_گره_عبارت* exprp, ثابت string& پیشوند) {
        File_سطر* ثابت fl = rangep->fileline();
        ثابت VCMethod روش = exprp->isWide() ? VCMethod::COVERGROUP_SIZED_RANGE_W
                                                : VCMethod::COVERGROUP_SIZED_RANGE;
        ثابت AstInsideRange* ثابت irp = VN_CAST(rangep, InsideRange);
        Ast_گره_عبارت* ثابت lowp = irp ? irp->lhsp() : VN_AS(rangep, گره_عبارت);
        Ast_گره_عبارت* ثابت highp = irp ? irp->rhsp() : اشاره_تهی;
        ثابت خودکار unbounded
            = [](ثابت Ast_گره_عبارت* boundp) { أرجع !boundp || VN_IS(boundp, Unbounded); };
        ثابت خودکار constant
            = [&](ثابت Ast_گره_عبارت* boundp) { أرجع unbounded(boundp) || VN_IS(boundp, Const); };
        ثابت خودکار integral = [&](ثابت Ast_گره_عبارت* boundp) {
            أرجع unbounded(boundp) || boundp->dtypep()->skipRefp()->isIntegralOrPacked();
        };
        إذا (constant(lowp) && constant(highp)) {
            ثابت خودکار fourState = [](ثابت Ast_گره_عبارت* boundp) {
                ثابت AstConst* ثابت constp = VN_CAST(boundp, Const);
                أرجع constp && constp->رقم().isFourState();
            };
            إذا (irp && (fourState(lowp) || fourState(highp))) {
                rangep->v3error("Four-state (x/z) value in "
                                << (binp->isArray() ? "array bins" : "bin")
                                << " range bound; range bounds must be two-state constants");
                أرجع;
            }
            Cross_مقدار_Range range{rangep, resolve_عرض(rangep, exprp)};
            إذا (!resolve_مقدار(rangep, exprp, درست, binp->isWildcard(), range)) {
                rangep->v3warn(E_UNSUPPORTED, "Unsupported: non-integral value in a coverage bin "
                                              "of an integral coverpoint.");
            } وإلا إذا (!cross_Range_خالی(range)) {
                std::وکتور<std::pair<V3_عدد, V3_عدد>> runs{{range.lo, range.hi}};
                إذا (range.wildcard) {  // checkConstructedBins bounded the runs
                    runs.clear();
                    crossRangeRuns(range, v3Global.opt.coverageMaxBins(), runs);
                }
                لكل (ثابت std::pair<V3_عدد, V3_عدد>& پیاپی : runs) {
                    m_constructorp->جمع_کن_Stmtsp(قلم_Call(fl, cpVarp, روش,
                                                       {new_مقدار_Const(fl, پیاپی.اولی, exprp),
                                                        new_مقدار_Const(fl, پیاپی.دومی, exprp)})
                                                  ->makeStmt());
                }
            }
            أرجع;
        }
        إذا (!integral(lowp) || !integral(highp)) {
            rangep->v3warn(E_UNSUPPORTED, "Unsupported: non-integral value in a coverage bin "
                                          "of an integral coverpoint.");
            أرجع;
        }
        // Compare values and the coverpoint's domain signed, in a width holding all of them
        ثابت خودکار bound_عرض
            = [&](ثابت Ast_گره_عبارت* boundp) { أرجع unbounded(boundp) ? 0 : boundp->عرض(); };
        ثابت صحيح عرض = std::max({exprp->عرض(), bound_عرض(lowp), bound_عرض(highp)}) + 1;
        ثابت Cross_مقدار_Range domain
            = cross_مقدار_Domain(rangep, exprp->عرض(), exprp->isSigned(), عرض);
        AstVar* ثابت lop = constructor_موقت(fl, پیشوند + "_lo", exprp->dtypep());
        lop->dtype_مجموعه_بیت_Sized(عرض, VSigning::SIGNED);
        AstVar* ثابت hip = constructor_موقت(fl, پیشوند + "_hi", lop->dtypep());
        ثابت خودکار bound = [&](Ast_گره_عبارت* boundp, ثابت V3_عدد& حد) -> Ast_گره_عبارت* {
            إذا (unbounded(boundp)) {
                AstConst* ثابت limitp = جديد AstConst{fl, حد};
                limitp->dtypeFrom(lop);
                أرجع limitp;
            }
            Ast_گره_عبارت* valuep = boundp->clone_درخت(خطأ);
            // A value no wider than a signed coverpoint has its type (see crossRangeBound)
            إذا (exprp->isSigned() && boundp->عرض() <= exprp->عرض()) {
                valuep = resize_مقدار(valuep, exprp->dtypep());
            }
            أرجع resize_مقدار(valuep, lop->dtypep());
        };
        ثابت خودکار ref = [&](AstVar* varp, VAccess access = VAccess::READ) {
            أرجع جديد AstVarRef{fl, varp, access};
        };
        m_constructorp->جمع_کن_Stmtsp(
            جديد Ast_تخصیص_بده{fl, ref(lop, VAccess::WRITE), bound(lowp, domain.lo)});
        m_constructorp->جمع_کن_Stmtsp(
            جديد Ast_تخصیص_بده{fl, ref(hip, VAccess::WRITE), irp ? bound(highp, domain.hi) : ref(lop)});
        AstConst* ثابت minp = جديد AstConst{fl, domain.lo};
        AstConst* ثابت maxp = جديد AstConst{fl, domain.hi};
        minp->dtypeFrom(lop);
        maxp->dtypeFrom(lop);
        AstCond* ثابت lowerp
            = جديد AstCond{fl, جديد AstLtS{fl, ref(lop), minp}, minp->clone_درخت(خطأ), ref(lop)};
        AstCond* ثابت upperp
            = جديد AstCond{fl, جديد AstGtS{fl, ref(hip), maxp}, maxp->clone_درخت(خطأ), ref(hip)};
        lowerp->dtypeFrom(lop);
        upperp->dtypeFrom(lop);
        m_constructorp->جمع_کن_Stmtsp(جديد Ast_تخصیص_بده{fl, ref(lop, VAccess::WRITE), lowerp});
        m_constructorp->جمع_کن_Stmtsp(جديد Ast_تخصیص_بده{fl, ref(hip, VAccess::WRITE), upperp});
        m_constructorp->جمع_کن_Stmtsp(جديد AstIf{fl, جديد AstLteS{fl, ref(lop), ref(hip)},
                                            قلم_Call(fl, cpVarp, روش,
                                                     {resize_مقدار(ref(lop), exprp->dtypep()),
                                                      resize_مقدار(ref(hip), exprp->dtypep())})
                                                ->makeStmt()});
    }

    // The runtime index of the bin of a run holding the coverpoint value, which is in the run:
    // declared + (value - lo) / stride, capped at the last bin, which holds any remainder.
    ساكن Ast_گره_عبارت* پیاپی_Bin_شاخص(File_سطر* fl, Ast_گره_عبارت* exprp, ثابت Bin_پیاپی& پیاپی) {
        إذا (پیاپی.m_شمار == 1) أرجع cnum(fl, پیاپی.m_declared);
        ثابت صحيح عرض = exprp->عرض();
        // A run spans at most 2^width values, so offsets in it are unsigned width-bit numbers
        Ast_گره_عبارت* indexp
            = جديد AstSub{fl, exprp->clone_درخت(خطأ), new_مقدار_Const(fl, پیاپی.m_lo, exprp)};
        indexp->dtype_مجموعه_بیت_Sized(عرض, VSigning::UNSIGNED);
        V3_عدد stride{fl, عرض, 0};
        stride.op_تخصیص_بده(پیاپی.m_stride);
        إذا (stride.شمار_Ones() != 1) {
            indexp = جديد AstDiv{fl, indexp, جديد AstConst{fl, stride}};
        } وإلا إذا (!stride.isEqOne()) {
            indexp = جديد Ast_لغزش_R{fl, indexp, جديد AstConst{fl, stride.most_مجموعه_بیت_P1() - 1}};
        }
        // Compare the run's values with those of count bins of stride values, without overflow
        ثابت صحيح ext_عرض = پیاپی.m_lo.عرض() + 1;
        V3_عدد lo{fl, ext_عرض, 0};
        lo.opExtendS(پیاپی.m_lo, پیاپی.m_lo.عرض());
        V3_عدد بازه{fl, ext_عرض, 0};
        بازه.opExtendS(پیاپی.m_hi, پیاپی.m_hi.عرض());
        بازه.opSub(V3_عدد{بازه}, lo);
        V3_عدد covered{fl, ext_عرض, 0};
        covered.op_تخصیص_بده(پیاپی.m_stride);
        covered.opMul(V3_عدد{covered}, V3_عدد{fl, ext_عرض, پیاپی.m_شمار});
        V3_عدد hasRemainder{fl, 1, 0};
        إذا (!hasRemainder.opGte(بازه, covered).isEqZero()) {
            AstConst* ثابت lastp = جديد AstConst{fl, V3_عدد{fl, عرض, پیاپی.m_شمار - 1}};
            indexp = جديد AstCond{fl, جديد AstGt{fl, indexp->clone_درخت(خطأ), lastp},
                                 lastp->clone_درخت(خطأ), indexp};
        }
        إذا (عرض < VL_IDATASIZE) {
            indexp = جديد AstExtend{fl, indexp, VL_IDATASIZE};
        } وإلا إذا (عرض > VL_IDATASIZE) {
            indexp = جديد AstSel{fl, indexp, 0, VL_IDATASIZE};
        }
        أرجع جديد Ast_جمع_کن{fl, cnum(fl, پیاپی.m_declared), indexp};
    }

    // Emit the sample() hit of a run of bins, whose code does not grow with its number of bins:
    //   if (iff && lo <= value && value <= hi) m_cp.incrementBin(<runBinIndex>);
    فراغ emit_پیاپی_اصابت(AstCoverpoint* coverpointp, AstCoverBin* binp, AstVar* cpVarp,
                    Ast_گره_عبارت* exprp, ثابت Bin_پیاپی& پیاپی) {
        File_سطر* ثابت fl = binp->fileline();
        AstConst* ثابت lop = new_مقدار_Const(fl, پیاپی.m_lo, exprp);
        Ast_گره_عبارت* condp = اشاره_تهی;
        إذا (پیاپی.m_lo.isCaseEq(پیاپی.m_hi)) {
            condp = جديد AstEq{fl, exprp->clone_درخت(خطأ), lop};
        } وإلا {
            AstConst* ثابت hip = new_مقدار_Const(fl, پیاپی.m_hi, exprp);
            condp = makeRangeCondition(fl, exprp, lop, hip);
            VL_DO_DANGLING(افزودن_Deletep(lop), lop);
            VL_DO_DANGLING(افزودن_Deletep(hip), hip);
        }
        emit_Conv_اصابت_If(coverpointp, binp, cpVarp, پیاپی_Bin_شاخص(fl, exprp, پیاپی), condp);
    }

    // Emit a transition bin's hit action into sample():
    //   if (cond) { m_cp.incrementBin/recordHit(idx); [illegal: $error; $stop] }
    // Used by the transition generators so a completed sequence records into the runtime bin.
    فراغ جمع_کن_Conv_Trans_اصابت_If(AstCoverpoint* coverpointp, AstCoverBin* binp, ثابت Conv_Bin_هدف& tgt,
                           Ast_گره_عبارت* condp) {
        File_سطر* ثابت fl = binp->fileline();
        Ast_گره* actionp = make_Runtime_Bin_اصابت(fl, tgt);
        إذا (binp->binsType() == VCoverBinsType::BINS_ILLEGAL) {
            actionp->جمع_کن_بعدی(makeIllegalBinAction(
                fl, "Illegal transition bin " + binp->prettyNameQ() + " hit in coverpoint "
                        + coverpointp->prettyNameQ()));
        }
        UASSERT_OBJ(m_sampleFuncp, binp, "sample() CFunc not set for transition bin");
        جمع_کن_Sample_Stmt(جديد AstIf{fl, condp, actionp, اشاره_تهی});
    }

    // Route a coverpoint through a VlCoverpoint member: emit the member, its sample()
    // increments, the constructor configuration (init + namers), and registration.
    فراغ generateCoverpoint(AstCoverpoint* coverpointp, Ast_گره_عبارت* exprp, صحيح at_کمترین_مقدار) {
        File_سطر* ثابت fl = coverpointp->fileline();
        ثابت منطقي dynamic = m_runtimePoints.شمار(coverpointp);
        UINFO(4, "  Generating VlCoverpoint member: " << coverpointp->name());

        // Size the hit list to the gen-time max bin overlap (1 unless cross-fed with
        // overlapping ranges), so no cross hit is ever dropped and storage is minimal.
        ثابت منطقي crossFed = m_crossed_Cp_نامها.شمار(coverpointp->name()) != 0;
        ثابت صحيح اصابت_Bound = compute_اصابت_List_Bound(coverpointp, exprp, crossFed);
        UINFO(6, "    Hit-list bound (max bin overlap) = " << اصابت_Bound);
        AstVar* ثابت cpVarp = جديد AstVar{fl, VVarType::MEMBER, "__Vcp_" + coverpointp->name(),
                                          coverpointDType(fl, static_cast<uint32_t>(اصابت_Bound))};
        m_covergroupp->جمع_کن_Membersp(cpVarp);
        m_cpVarMap[coverpointp->name()] = cpVarp;
        m_cpBins.emplace(cpVarp, CoverpointBins{});
        m_cpBins.at(cpVarp).exprp = exprp;
        m_cpBins.at(cpVarp).crossed = crossFed;
        // Create the runtime in the instance node first; everything below configures it.
        m_constructorp->جمع_کن_Stmtsp(make_قلم_Create(fl, cpVarp, VCMethod::COVERGROUP_جمع_کن_COVERPOINT));
        generate_قلم_وزن(fl, cpVarp, coverpointp->optionsp());

        // A cross reads this coverpoint's hit list, so clear it at the start of the
        // coverpoint's sample() contribution (before any incrementBin appends to it), even
        // when the iff guard disables sampling.
        إذا (crossFed) {
            UASSERT_OBJ(m_sampleFuncp, coverpointp, "sample() CFunc not set for clearHitList");
            m_sampleFuncp->جمع_کن_Stmtsp(
                قلم_Call(fl, cpVarp, VCMethod::COVERGROUP_CLEAR_اصابت_LIST)->makeStmt());
        }
        إذا (dynamic && coverpointHasStateExclusions(coverpointp)) {
            AstVar* ثابت excludedp
                = جديد AstVar{fl, VVarType::BLOCKTEMP,
                             "__VcpExcluded_" + sanitizeGeneratedName(coverpointp->name()),
                             coverpointp->پیدا_کن_بیت_DType()};
            excludedp->funcLocal(درست);
            m_sampleFuncp->جمع_کن_Stmtsp(excludedp);
            AstCMethodHard* ثابت callp
                = قلم_Call(fl, cpVarp,
                           exprp->isWide() ? VCMethod::COVERGROUP_مقدار_EXCLUDED_W
                                           : VCMethod::COVERGROUP_مقدار_EXCLUDED,
                           {exprp->clone_درخت(خطأ)});
            callp->dtype_مجموعه_بیت();
            جمع_کن_Sample_Stmt(جديد Ast_تخصیص_بده{fl, جديد AstVarRef{fl, excludedp, VAccess::WRITE}, callp});
            m_excludedVars.emplace(cpVarp, excludedp);
        }

        // Walk bins (non-default, then default), assigning sequential indices that match the
        // namer append order; emit sample increments and collect namer statements.  Constructed
        // bins follow them all, placed when the coverpoint is constructed.
        std::وکتور<Ast_گره_Stmt*> namerStmts;
        std::وکتور<AstCoverBin*> defaultBins;
        std::وکتور<AstCoverBin*> sizedBins;
        std::وکتور<std::tuple<AstCoverBin*, uint32_t, Ast_گره_عبارت*>> metadata;
        std::وکتور<ثابت Bin_پیاپی*> پیاپی_Metadata;
        uint64_t idx = 0;  // Runtime index of the next bin; 32-bit once checked below
        لكل (Ast_گره* binp = coverpointp->binsp(); binp; binp = binp->nextp()) {
            AstCoverBin* ثابت cbinp = VN_AS(binp, CoverBin);
            ثابت صحيح errorsBefore = dynamic ? V3Error::error_شمار() : 0;
            إذا (cbinp->binsType() == VCoverBinsType::BINS_DEFAULT) {
                defaultBins.افزودن_پشت(cbinp);
                استمر;
            }
            إذا (isConstructedBins(cbinp)) {
                UASSERT_OBJ(dynamic, cbinp, "Constructed bins without value metadata");
                Bin_بازه بازه;
                بازه.sized = static_cast<int32_t>(sizedBins.اندازه());
                m_cpBins.at(cpVarp).spans.emplace(cbinp->name(), بازه);
                sizedBins.افزودن_پشت(cbinp);
                استمر;
            }
            إذا (cbinp->transp()) {
                // Transition bin (incl. array transition 'bins t[] = (a=>b),(c=>d)' and
                // illegal_bins/ignore_bins transitions).  All sequences of one transition bin
                // share a bin name and merge in the coverage DB to a single point, so model
                // them as one runtime bin incremented by any matching sequence.  The sequence
                // matching is generated as a state machine, with the hit routed to this bin's
                // runtime slot.
                namerStmts.افزودن_پشت(makeNamer(cpVarp, cbinp, -1, static_cast<uint32_t>(idx)));
                ثابت Conv_Bin_هدف tgt{cpVarp, static_cast<uint32_t>(idx),
                                        cbinp->binsType().bin_Is_نرمال()};
                لكل (Ast_گره* sp = cbinp->transp(); sp; sp = sp->nextp())
                    generate_Single_Transition_کد(coverpointp, cbinp, exprp, tgt,
                                                 VN_AS(sp, Cover_Trans_مجموعه));
                إذا (dynamic && V3Error::error_شمار() == errorsBefore) {
                    metadata.emplace_پشت(cbinp, static_cast<uint32_t>(idx), اشاره_تهی);
                }
                ++idx;
                استمر;
            }
            BinRuns plan;
            إذا (binRunsFor(cbinp, exprp, plan)) {
                // Array elements and automatic bins generate as runs, so neither sample() nor
                // the constructor grows with their number of bins.
                إذا (plan.unsupported) {  // bin ignored or invalid; reserve no slot
                    m_droppedBins[coverpointp].افزودن_پشت(cbinp->name());
                    استمر;
                }
                CoverpointBins& bins = m_cpBins.at(cpVarp);
                ثابت uint32_t اولی_مقدار = bins.مجموع;
                ثابت uint32_t اولی_Declared = static_cast<uint32_t>(idx);
                namerStmts.افزودن_پشت(
                    makeNamer(cpVarp, cbinp, plan.شمار, اولی_Declared, {}, plan.مقادیر));
                لكل (Bin_پیاپی& پیاپی : plan.runs) {
                    پیاپی.m_declared = static_cast<uint32_t>(idx);
                    bins.runs.افزودن_پشت(std::move(پیاپی));
                    ثابت Bin_پیاپی& stored = bins.runs.پشت();
                    إذا (bins.crossed && cbinp->binsType().bin_Is_نرمال()) {
                        ثابت uint32_t اولی = اولی_مقدار + stored.m_declared - اولی_Declared;
                        لكل (uint32_t element = 0; element < stored.m_شمار; ++element) {
                            bins.مقادیر[اولی + element].runp = &stored;
                            bins.مقادیر[اولی + element].element = element;
                        }
                    }
                    إذا (!stored.m_خالی) {
                        emit_پیاپی_اصابت(coverpointp, cbinp, cpVarp, exprp, stored);
                        إذا (dynamic && V3Error::error_شمار() == errorsBefore) {
                            پیاپی_Metadata.افزودن_پشت(&stored);
                        }
                    }
                    idx += stored.m_شمار;
                }
                استمر;
            }
            إذا (cbinp->isArray()) {  // value array of a real coverpoint: b[0]..b[N-1]
                // Only integral coverpoints have runtime value metadata (m_runtimePoints)
                UASSERT_OBJ(!dynamic, cbinp, "Runtime value metadata for a real coverpoint");
                منطقي unsupported = خطأ;
                std::وکتور<Ast_گره_عبارت*> مقادیر = استخراج_Array_مقادیر(cbinp, exprp, unsupported);
                إذا (unsupported) {  // bin ignored (COVERIGN emitted); reserve no slot
                    m_droppedBins[coverpointp].افزودن_پشت(cbinp->name());
                    استمر;
                }
                namerStmts.افزودن_پشت(makeNamer(cpVarp, cbinp, static_cast<int64_t>(مقادیر.اندازه()),
                                               static_cast<uint32_t>(idx), مقادیر));
                لكل (Ast_گره_عبارت* valuep : مقادیر) {
                    // The cross selections of this covergroup still read the value.
                    m_detached_مقادیر.افزودن_پشت(valuep);
                    emit_Conv_اصابت_If(coverpointp, cbinp, cpVarp,
                                  cnum(cbinp->fileline(), static_cast<uint32_t>(idx)),
                                  build_مقدار_Condition(cbinp, exprp, valuep));
                    ++idx;
                }
            } وإلا {
                namerStmts.افزودن_پشت(makeNamer(cpVarp, cbinp, -1, static_cast<uint32_t>(idx)));
                // buildBinCondition is null for 'ignore_bins = default' (no ranges); the bin
                // still gets a reserved slot (recorded, never incremented).
                إذا (Ast_گره_عبارت* ثابت condp = buildBinCondition(cbinp, exprp))
                    emit_Conv_اصابت_If(coverpointp, cbinp, cpVarp,
                                  cnum(cbinp->fileline(), static_cast<uint32_t>(idx)), condp);
                إذا (dynamic && V3Error::error_شمار() == errorsBefore) {
                    metadata.emplace_پشت(cbinp, static_cast<uint32_t>(idx), اشاره_تهی);
                }
                ++idx;
            }
        }
        // A cross selecting an ignored bins declaration selects no bins
        لكل (ثابت std::string& name : m_droppedBins[coverpointp]) {
            m_cpBins.at(cpVarp).spans.emplace(name, Bin_بازه{});
        }
        // A value a constructed bin holds is no default bin's; only sampling tells which do
        AstVar* sizedMatchedp = اشاره_تهی;
        إذا (!defaultBins.خالی()
            && std::any_از(sizedBins.begin(), sizedBins.انتها(), [](ثابت AstCoverBin* binp) {
                   أرجع binp->binsType().bin_Is_نرمال();
               })) {
            sizedMatchedp = جديد AstVar{fl, VVarType::BLOCKTEMP,
                                       "__VcpSized_" + sanitizeGeneratedName(coverpointp->name()),
                                       coverpointp->پیدا_کن_بیت_DType()};
            sizedMatchedp->funcLocal(درست);
            m_sampleFuncp->جمع_کن_Stmtsp(sizedMatchedp);
            جمع_کن_Sample_Stmt(جديد Ast_تخصیص_بده{fl, جديد AstVarRef{fl, sizedMatchedp, VAccess::WRITE},
                                        جديد AstConst{fl, AstConst::بیت_False{}}});
        }
        لكل (uint32_t sized = 0; sized < sizedBins.اندازه(); ++sized) {
            emitSizedSample(coverpointp, sizedBins[sized], cpVarp, exprp, sized, sizedMatchedp);
        }
        لكل (AstCoverBin* ثابت defBinp : defaultBins) {
            File_سطر* ثابت dfl = defBinp->fileline();
            namerStmts.افزودن_پشت(makeNamer(cpVarp, defBinp, -1, static_cast<uint32_t>(idx)));
            Ast_گره_عبارت* condp = buildDefaultCondition(coverpointp, exprp, dfl);
            إذا (sizedMatchedp) {
                condp = جديد AstLogAnd{
                    dfl, جديد AstNot{dfl, جديد AstVarRef{dfl, sizedMatchedp, VAccess::READ}}, condp};
            }
            emit_Conv_اصابت_If(coverpointp, defBinp, cpVarp, cnum(dfl, static_cast<uint32_t>(idx)),
                          condp);
            ++idx;
        }
        إذا (idx > std::numeric_limits<uint32_t>::max()) {
            // The runtime indexes bins with 32 bits; stop before generating a model
            coverpointp->v3warn(E_UNSUPPORTED, "Unsupported: coverpoint with more than "
                                                   << std::numeric_limits<uint32_t>::max()
                                                   << " bins");
        }

        // Transition coverpoints track the previous sampled value; update it once at the end of
        // this coverpoint's sample() contribution (the prev var was created on demand by the
        // transition matching above).
        إذا (coverpointHasTransition(coverpointp)) {
            AstVar* ثابت prevVarp = VN_AS(coverpointp->user1p(), Var);
            جمع_کن_Sample_Stmt(
                جديد Ast_تخصیص_بده{coverpointp->fileline(),
                              جديد AstVarRef{prevVarp->fileline(), prevVarp, VAccess::WRITE},
                              exprp->clone_درخت(خطأ)});
        }

        // Constructor: init (allocates), namers, then registration (under --coverage).
        // Under --protect-ids the hierarchy and page string reach the coverage database
        // verbatim, so obfuscate them like line/toggle points (per-word hierarchy, whole-
        // unit page).  No-ops when --protect-ids is off.
        ثابت منطقي prot = v3Global.opt.protectIds();
        ثابت std::string hier
            = VIdProtect::protect_واژگان_If(m_covergroupName + "." + coverpointp->name(), prot);
        m_constructorp->جمع_کن_Stmtsp(
            قلم_Call(fl, cpVarp, VCMethod::COVERGROUP_INIT,
                     {ctext(fl, quoted(hier)), cnum(fl, static_cast<uint32_t>(at_کمترین_مقدار)),
                      cnum(fl, static_cast<uint32_t>(idx))})
                ->makeStmt());
        لكل (Ast_گره_Stmt* ثابت ns : namerStmts) m_constructorp->جمع_کن_Stmtsp(ns);
        إذا (dynamic) {
            m_constructorp->جمع_کن_Stmtsp(
                قلم_Call(fl, cpVarp, VCMethod::COVERGROUP_مقدار_TYPE,
                         {cnum(fl, exprp->عرض()), cnum(fl, exprp->isSigned())})
                    ->makeStmt());
            مقدار_Lists lists;
            لكل (ثابت خودکار& مدخل : metadata) {
                collect_مقدار_Metadata(lists, exprp, std::get<0>(مدخل), std::get<1>(مدخل),
                                     std::get<2>(مدخل));
            }
            لكل (ثابت Bin_پیاپی* ثابت runp : پیاپی_Metadata) collect_پیاپی_Metadata(lists, exprp, *runp);
            emit_مقدار_List(fl, cpVarp, VCMethod::COVERGROUP_مقدار_RANGES, lists.m_ranges);
            emit_مقدار_List(fl, cpVarp, VCMethod::COVERGROUP_مقدار_RUNS, lists.m_runs);
            emit_مقدار_List(fl, cpVarp, VCMethod::COVERGROUP_مقدار_PATTERNS, lists.m_patterns);
            emit_مقدار_List(fl, cpVarp, VCMethod::COVERGROUP_مقدار_TRANSITIONS, lists.m_transitions);
            لكل (AstCoverBin* ثابت binp : sizedBins) {
                generateConstructedBins(coverpointp, binp, cpVarp, exprp);
            }
            m_constructorp->جمع_کن_Stmtsp(
                قلم_Call(fl, cpVarp, VCMethod::COVERGROUP_مقدار_FINALIZE)->makeStmt());
        }
        إذا (v3Global.opt.coverage()) {
            ثابت std::string صفحه
                = VIdProtect::protectIf("v_covergroup/" + m_covergroupName, prot);
            m_constructorp->جمع_کن_Stmtsp(
                قلم_Call(fl, cpVarp, VCMethod::COVERGROUP_ثبات_BINS,
                         {ctext(fl, "vlSymsp->_vm_contextp__->coveragep()"),
                          ctext(fl, quoted(صفحه)),
                          cnum(fl, قلم_Database_وزن(coverpointp->optionsp())),
                          cnum(fl, m_cg_Type_وزن)})
                    ->makeStmt());
        }
    }

    // Generate state machine code for multi-value transition sequences
    // Handles transitions like (1 => 2 => 3 => 4)
    فراغ generate_Multi_مقدار_Transition_کد(AstCoverpoint* coverpointp, AstCoverBin* binp,
                                          Ast_گره_عبارت* exprp, ثابت Conv_Bin_هدف& tgt,
                                          ثابت std::وکتور<Ast_Cover_Trans_قلم*>& اقلام) {
        UINFO(4, "    Generating multi-value transition state machine for: " << binp->name());
        UINFO(4, "      Sequence length: " << اقلام.اندازه() << " items");

        // Create state position variable
        AstVar* ثابت stateVarp = createSequenceStateVar(coverpointp, binp);

        // Build case statement with N cases (one for each state 0 to N-1)
        // State 0: Not started, looking for first item
        // State 1 to N-1: In progress, looking for next item

        AstCase* ثابت casep
            = جديد AstCase{binp->fileline(), VCaseType::CT_CASE,
                          جديد AstVarRef{stateVarp->fileline(), stateVarp, VAccess::READ}, اشاره_تهی};

        // Generate each case item in the switch statement
        لكل (اندازه_t state = 0; state < اقلام.اندازه(); ++state) {
            Ast_Case_قلم* caseItemp = generateTransitionStateCase(coverpointp, binp, exprp, tgt,
                                                                 stateVarp, اقلام, state);
            casep->جمع_کن_Itemsp(caseItemp);
        }

        // Add default case (reset to state 0) to prevent CASEINCOMPLETE warnings,
        // since the state variable is wider than the number of valid states.
        Ast_Case_قلم* ثابت defaultItemp = جديد Ast_Case_قلم{
            binp->fileline(), اشاره_تهی,
            جديد Ast_تخصیص_بده{binp->fileline(),
                          جديد AstVarRef{binp->fileline(), stateVarp, VAccess::WRITE},
                          جديد AstConst{binp->fileline(), AstConst::Widthed_مقدار{}, 8, 0}}};
        casep->جمع_کن_Itemsp(defaultItemp);

        جمع_کن_Sample_Stmt(casep);
        UINFO(4, "      Successfully added multi-value transition state machine");
    }

    // Generate code for a single state in the transition state machine
    // Returns the case item for this state
    Ast_Case_قلم* generateTransitionStateCase(AstCoverpoint* coverpointp, AstCoverBin* binp,
                                             Ast_گره_عبارت* exprp, ثابت Conv_Bin_هدف& tgt,
                                             AstVar* stateVarp,
                                             ثابت std::وکتور<Ast_Cover_Trans_قلم*>& اقلام,
                                             اندازه_t state) {
        File_سطر* ثابت fl = binp->fileline();

        // Build condition for current value matching expected item at this state
        Ast_گره_عبارت* ثابت تطابق_Condp = build_Transition_قلم_Condition(اقلام[state], exprp);

        Ast_گره_Stmt* تطابق_Actionp = اشاره_تهی;

        إذا (state == اقلام.اندازه() - 1) {
            // Last state: sequence complete!  Record the hit in the runtime VlCoverpoint.
            تطابق_Actionp = make_Runtime_Bin_اصابت(fl, tgt);

            // For illegal_bins, add error message
            إذا (binp->binsType() == VCoverBinsType::BINS_ILLEGAL) {
                ثابت string err_پیام = "Illegal transition bin " + binp->prettyNameQ()
                                      + " hit in coverpoint " + coverpointp->prettyNameQ();
                تطابق_Actionp = تطابق_Actionp->جمع_کن_بعدی(makeIllegalBinAction(fl, err_پیام));
            }

            // Reset state to 0
            تطابق_Actionp = تطابق_Actionp->جمع_کن_بعدی(
                جديد Ast_تخصیص_بده{fl, جديد AstVarRef{fl, stateVarp, VAccess::WRITE},
                              جديد AstConst{fl, AstConst::Widthed_مقدار{}, 8, 0}});
        } وإلا {
            // Intermediate state: advance to next state
            تطابق_Actionp = جديد Ast_تخصیص_بده{
                fl, جديد AstVarRef{fl, stateVarp, VAccess::WRITE},
                جديد AstConst{fl, AstConst::Widthed_مقدار{}, 8, static_cast<uint32_t>(state + 1)}};
        }

        // Build restart logic: check if current value matches first item
        // If so, restart sequence from state 1 (even if we're in middle of sequence)
        Ast_گره_Stmt* no_تطابق_Actionp = اشاره_تهی;
        إذا (state > 0) {
            // Check if current value matches first item (restart condition)
            Ast_گره_عبارت* ثابت restartCondp = build_Transition_قلم_Condition(اقلام[0], exprp);

            UASSERT_OBJ(restartCondp, اقلام[0],
                        "buildTransitionItemCondition returned nullptr for restart");

            // Restart to state 1
            Ast_گره_Stmt* restartActionp
                = جديد Ast_تخصیص_بده{fl, جديد AstVarRef{fl, stateVarp, VAccess::WRITE},
                                جديد AstConst{fl, AstConst::Widthed_مقدار{}, 8, 1}};

            // Reset to state 0 (else branch)
            Ast_گره_Stmt* resetActionp
                = جديد Ast_تخصیص_بده{fl, جديد AstVarRef{fl, stateVarp, VAccess::WRITE},
                                جديد AstConst{fl, AstConst::Widthed_مقدار{}, 8, 0}};

            no_تطابق_Actionp = جديد AstIf{fl, restartCondp, restartActionp, resetActionp};
        }
        // For state 0, no action needed if no match (stay in state 0)

        // Combine into if-else
        Ast_گره_Stmt* ثابت stmtp = جديد AstIf{fl, تطابق_Condp, تطابق_Actionp, no_تطابق_Actionp};

        // Create case item for this state value
        Ast_Case_قلم* ثابت caseItemp = جديد Ast_Case_قلم{
            fl, جديد AstConst{fl, AstConst::Widthed_مقدار{}, 8, static_cast<uint32_t>(state)},
            stmtp};

        أرجع caseItemp;
    }

    // Create: $error(msg); $stop;  Used when an illegal bin is hit.
    Ast_گره_Stmt* makeIllegalBinAction(File_سطر* fl, ثابت string& err_پیام) {
        AstDisplay* ثابت errorp
            = جديد AstDisplay{fl, VDisplayType::DT_ERROR, err_پیام, اشاره_تهی, اشاره_تهی};
        errorp->fmtp()->timeunit(m_covergroupp->timeunit());
        static_cast<Ast_گره*>(errorp)->جمع_کن_بعدی(جديد AstStop{fl, درست});
        أرجع errorp;
    }

    // Preserve the coverpoint's width and signedness after V3Width.
    ساكن AstConst* new_مقدار_Const(File_سطر* fl, ثابت V3_عدد& مقدار, ثابت Ast_گره_عبارت* exprp) {
        V3_عدد narrowed{fl, exprp->عرض(), 0};
        narrowed.op_تخصیص_بده(مقدار);
        AstConst* ثابت constp = جديد AstConst{fl, narrowed};
        constp->dtypeFrom(exprp);
        أرجع constp;
    }

    // A real copy of a real or integral range bound, for comparing with a real coverpoint.
    ساكن AstConst* newRealConst(AstConst* constp) {
        إذا (constp->رقم().isDouble()) أرجع constp->clone_درخت(خطأ);
        V3_عدد real{&constp->رقم(), 64};
        real.opIToRD(constp->رقم(), constp->isSigned());
        أرجع جديد AstConst{constp->fileline(), real};
    }

    // Clone a constant node, widening to targetWidth if needed (zero-extend).
    // Used to ensure comparisons use matching widths after V3Width has run.
    ساكن AstConst* widenConst(File_سطر* fl, AstConst* constp, صحيح هدف_عرض) {
        إذا (constp->عرض() == هدف_عرض) أرجع constp->clone_درخت(خطأ);
        V3_عدد رقم{fl, هدف_عرض, 0};
        رقم.op_تخصیص_بده(constp->رقم());
        أرجع جديد AstConst{fl, رقم};
    }

    // Build a range condition: minp <= exprp <= maxp.
    // Uses signed comparisons if exprp is signed; omits trivially-true domain bounds.
    // All arguments are non-owning; clones exprp/minp/maxp as needed.
    Ast_گره_عبارت* makeRangeCondition(File_سطر* fl, Ast_گره_عبارت* exprp, Ast_گره_عبارت* minp,
                                    Ast_گره_عبارت* maxp) {
        ثابت صحيح عبارت_عرض = exprp->عرض_Min();
        AstConst* ثابت minConstp = VN_AS(minp, Const);
        AstConst* ثابت maxConstp = VN_AS(maxp, Const);
        إذا (exprp->isDouble()) {
            // A real coverpoint has no finite domain bounds to omit.
            أرجع جديد AstAnd{fl,
                              جديد AstGteD{fl, exprp->clone_درخت(خطأ), newRealConst(minConstp)},
                              جديد AstLteD{fl, exprp->clone_درخت(خطأ), newRealConst(maxConstp)}};
        }
        // Widen constants to match expression width so post-V3Width nodes use correct macros
        AstConst* ثابت minWidep = widenConst(fl, minConstp, عبارت_عرض);
        AstConst* ثابت maxWidep = widenConst(fl, maxConstp, عبارت_عرض);
        V3_عدد minimum{fl, عبارت_عرض, 0};
        V3_عدد بیشینه{fl, عبارت_عرض, 0};
        بیشینه.مجموعه_All_Bits1();
        إذا (exprp->isSigned()) {
            minimum.مجموعه_بیت(عبارت_عرض - 1, 1);
            بیشینه.مجموعه_بیت(عبارت_عرض - 1, 0);
        }
        Ast_گره_عبارت* lowerp = اشاره_تهی;
        Ast_گره_عبارت* upperp = اشاره_تهی;
        إذا (minWidep->رقم().isCaseEq(minimum)) {
            VL_DO_DANGLING(افزودن_Deletep(minWidep), minWidep);
        } وإلا {
            lowerp = exprp->isSigned() ? static_cast<Ast_گره_عبارت*>(
                                             جديد AstGteS{fl, exprp->clone_درخت(خطأ), minWidep})
                                       : static_cast<Ast_گره_عبارت*>(
                                             جديد AstGte{fl, exprp->clone_درخت(خطأ), minWidep});
        }
        إذا (maxWidep->رقم().isCaseEq(بیشینه)) {
            VL_DO_DANGLING(افزودن_Deletep(maxWidep), maxWidep);
        } وإلا {
            upperp = exprp->isSigned() ? static_cast<Ast_گره_عبارت*>(
                                             جديد AstLteS{fl, exprp->clone_درخت(خطأ), maxWidep})
                                       : static_cast<Ast_گره_عبارت*>(
                                             جديد AstLte{fl, exprp->clone_درخت(خطأ), maxWidep});
        }
        إذا (lowerp && upperp) أرجع جديد AstAnd{fl, lowerp, upperp};
        إذا (lowerp) أرجع lowerp;
        إذا (upperp) أرجع upperp;
        أرجع جديد AstConst{fl, AstConst::بیت_True{}};
    }

    // Build a one-sided comparison for an open-ended bin range whose other bound is '$'.
    // '$' denotes the coverpoint domain extreme, so {[lo:$]} == (expr >= lo) and
    // {[$:hi]} == (expr <= hi).
    Ast_گره_عبارت* makeOpenRangeCondition(File_سطر* fl, Ast_گره_عبارت* exprp, AstConst* boundp,
                                        منطقي is_پایینی_Bound) {
        AstConst* ثابت widep = widenConst(fl, boundp, exprp->عرض_Min());
        إذا (is_پایینی_Bound) {
            إذا (exprp->isSigned()) أرجع جديد AstGteS{fl, exprp->clone_درخت(خطأ), widep};
            أرجع جديد AstGte{fl, exprp->clone_درخت(خطأ), widep};
        }
        إذا (exprp->isSigned()) أرجع جديد AstLteS{fl, exprp->clone_درخت(خطأ), widep};
        أرجع جديد AstLte{fl, exprp->clone_درخت(خطأ), widep};
    }

    // Build condition for a single transition item.
    // Returns expression that checks if exprp matches the item's value/range list.
    // Overload for when the expression is a variable read -- creates and manages the VarRef
    // internally, so callers don't need to construct a temporary node.
    Ast_گره_عبارت* build_Transition_قلم_Condition(Ast_Cover_Trans_قلم* itemp, AstVar* varp) {
        Ast_گره_عبارت* varRefp = جديد AstVarRef{varp->fileline(), varp, VAccess::READ};
        Ast_گره_عبارت* ثابت condp = build_Transition_قلم_Condition(itemp, varRefp);
        VL_DO_DANGLING(افزودن_Deletep(varRefp), varRefp);
        أرجع condp;
    }

    // Non-owning: exprp is cloned internally; caller retains ownership of exprp.
    Ast_گره_عبارت* build_Transition_قلم_Condition(Ast_Cover_Trans_قلم* itemp, Ast_گره_عبارت* exprp) {
        Ast_گره_عبارت* condp = اشاره_تهی;

        لكل (Ast_گره* valp = itemp->valuesp(); valp; valp = valp->nextp()) {
            Ast_گره_عبارت* singleCondp = اشاره_تهی;
            valp = V3Const::constifyEdit(valp);
            AstConst* ثابت constp = VN_CAST(valp, Const);
            إذا (!constp) {
                valp->v3error("Non-constant expression in transition bin; "
                              "values must be constants (IEEE 1800-2023 19.5)");
                أرجع جديد AstConst{valp->fileline(), AstConst::بیت_False_Erroring{}};
            }
            singleCondp
                = جديد AstEq{constp->fileline(), exprp->clone_درخت(خطأ), constp->clone_درخت(خطأ)};
            إذا (condp) {
                condp = جديد AstOr{itemp->fileline(), condp, singleCondp};
            } وإلا {
                condp = singleCondp;
            }
        }

        أرجع condp;
    }

    // Generate code for a single transition sequence (used by both regular and array bins)
    فراغ generate_Single_Transition_کد(AstCoverpoint* coverpointp, AstCoverBin* binp,
                                      Ast_گره_عبارت* exprp, ثابت Conv_Bin_هدف& tgt,
                                      Ast_Cover_Trans_مجموعه* transSetp) {
        UINFO(4, "      Generating code for transition sequence");

        // Get or create previous value variable
        AstVar* ثابت prevVarp = create_Prev_مقدار_Var(coverpointp, exprp);

        UASSERT_OBJ(
            transSetp, binp,
            "Transition bin has no transition set (transp() was checked before calling this)");

        // Get transition items (the sequence: item1 => item2 => item3)
        std::وکتور<Ast_Cover_Trans_قلم*> اقلام;
        لكل (Ast_گره* itemp = transSetp->itemsp(); itemp; itemp = itemp->nextp())
            اقلام.افزودن_پشت(VN_AS(itemp, Cover_Trans_قلم));

        إذا (اقلام.خالی()) {
            binp->v3error("Transition set without items");
            أرجع;
        }

        إذا (اقلام.اندازه() == 1) {
            // Single item transition not valid (need at least 2 values for =>)
            binp->v3error("Transition requires at least two values");
            أرجع;
        } وإلا إذا (اقلام.اندازه() == 2) {
            // Simple two-value transition: (val1 => val2)
            // Use optimized direct comparison (no state machine needed)
            Ast_گره_عبارت* ثابت cond1p = build_Transition_قلم_Condition(اقلام[0], prevVarp);
            Ast_گره_عبارت* ثابت cond2p = build_Transition_قلم_Condition(اقلام[1], exprp);

            // Combine: prev matches val1 AND current matches val2
            Ast_گره_عبارت* پر_Condp = جديد AstAnd{binp->fileline(), cond1p, cond2p};

            جمع_کن_Conv_Trans_اصابت_If(coverpointp, binp, tgt, پر_Condp);

            UINFO(4, "        Successfully added 2-value transition if statement");
        } وإلا {
            // Multi-value sequence (a => b => c => ...)
            // Use state machine to track position in sequence
            generate_Multi_مقدار_Transition_کد(coverpointp, binp, exprp, tgt, اقلام);
        }
    }

    // Append a "{ VlCoverpoint* __Vcx_cps[] = {cp0, cp1, ...}; <call> }" statement.  The brace
    // and the temporary array stay literal text -- a CMethodHard is one call, not a block --
    // but callp itself carries the member, method and '->'.  Construction only: init() copies
    // the array into the cross, so sample() reads it from there and needs no array at all.
    AstCStmt* makeCrossCpsCall(File_سطر* fl, ثابت std::وکتور<AstVar*>& cpVars,
                               AstCMethodHard* callp) {
        AstCStmt* ثابت cs = جديد AstCStmt{fl};
        cs->جمع_کن("{ VlCoverpoint* __Vcx_cps[] = {");
        لكل (اندازه_t d = 0; d < cpVars.اندازه(); ++d) {
            إذا (d != 0) cs->جمع_کن(", ");
            cs->جمع_کن(memberRef(fl, cpVars[d]));
        }
        cs->جمع_کن("}; ");
        cs->جمع_کن(callp);
        cs->جمع_کن("; }");
        أرجع cs;
    }

    // Assign the per-bin flags individually: one-bit SV results have integer C++ storage types,
    // which may narrow in a bool initializer list but convert implicitly in assignments.
    AstCStmt* makeCrossIffsCall(File_سطر* fl, ثابت std::وکتور<AstCoverCrossBin*>& bins,
                                AstCMethodHard* callp) {
        AstCStmt* ثابت cs = جديد AstCStmt{fl};
        cs->جمع_کن("{ bool __Vcx_iffs[" + cvt_مقصد_Str(bins.اندازه()) + "]; ");
        لكل (اندازه_t i = 0; i < bins.اندازه(); ++i) {
            ثابت AstCoverCrossBin* ثابت binp = bins[i];
            cs->جمع_کن("__Vcx_iffs[" + cvt_مقصد_Str(i) + "] = ");
            cs->جمع_کن(binp->iffp() ? binp->iffp()->clone_درخت(خطأ)
                                 : جديد AstConst{fl, AstConst::بیت_True{}});
            cs->جمع_کن("; ");
        }
        cs->جمع_کن(callp);
        cs->جمع_کن("; }");
        أرجع cs;
    }

    استفاده CrossSelection = std::وکتور<uint64_t>;
    بنية ResolvedCrossBin final {
        AstCoverCrossBin* binp;
        CrossSelection selection;
    };
    بنية CrossLayout final {
        uint32_t tuples = 0;
        uint32_t autoBins = 0;
        uint64_t bin_واژگان = 0;
        منطقي valid = درست;
        std::وکتور<ResolvedCrossBin> bins;
    };
    بنية CrossSelectionContext final {
        AstCoverCross* crossp;  // Cross whose tuple space is being selected
        ثابت std::وکتور<AstVar*>& cpVars;  // Feeding coverpoints in dimension order
        ثابت std::map<std::string, uint32_t>& dimensions;  // Coverpoint name -> dimension
        uint32_t tuples;  // Size of the Cartesian product
        std::وکتور<uint32_t> strides;  // Flat-index stride per dimension
        منطقي valid = درست;  // False if this explicit bin cannot be implemented
    };
    بنية Cross_Binsof_هدف final {
        ثابت CoverpointBins* m_binsp = اشاره_تهی;  // Declared bins; null after a reported error
        uint32_t m_dimension = 0;  // Coverpoint index within the cross
        uint32_t m_اولی = 0;  // First declared normal bin in the selected span
        uint32_t m_شمار = 0;  // Number of declared normal bins in the selected span
        uint32_t m_declared_اولی = 0;  // First runtime bin index of the selected span
        uint32_t m_declared_انتها = UINT32_MAX;  // One past the last runtime bin index
        int32_t m_sized = -1;  // Index of the sized array holding the selected bins; or -1
    };
    بنية Cross_مقدار_Range final {
        V3_عدد lo;  // Inclusive lower bound, sign-extended to the comparison width
        V3_عدد hi;  // Inclusive upper bound
        V3_عدد الگو_2;  // Allowed bit values; all X for an ordinary interval
        منطقي wildcard = خطأ;  // A wildcard singleton rather than an exact four-state value

        Cross_مقدار_Range(Ast_گره* nodep, صحيح عرض)
            : lo{nodep, عرض}
            , hi{nodep, عرض}
            , الگو_2{nodep, عرض} {
            الگو_2.مجموعه_All_بیتها_X();
        }
    };

    ساكن std::وکتور<Ast_گره*> cross_Bin_مقادیر(ثابت Cross_Bin_مقادیر& bin) {
        إذا (bin.valuep) أرجع {bin.valuep};
        std::وکتور<Ast_گره*> مقادیر;
        إذا (bin.binp->transp()) {
            // IEEE 1800-2023 19.6.1: binsof uses the last value of each transition.
            لكل (Ast_گره* setp = bin.binp->transp(); setp; setp = setp->nextp()) {
                Ast_Cover_Trans_قلم* lastp = VN_AS(setp, Cover_Trans_مجموعه)->itemsp();
                طالما (lastp->nextp()) lastp = VN_AS(lastp->nextp(), Cover_Trans_قلم);
                لكل (Ast_گره* valuep = lastp->valuesp(); valuep; valuep = valuep->nextp()) {
                    مقادیر.افزودن_پشت(valuep);
                }
            }
        } وإلا {
            لكل (Ast_گره* valuep = bin.binp->rangesp(); valuep; valuep = valuep->nextp()) {
                مقادیر.افزودن_پشت(valuep);
            }
        }
        أرجع مقادیر;
    }

    // The values of the element-th bin of a run, sign-extended to 'width'
    ساكن Cross_مقدار_Range پیاپی_Bin_Range(Ast_گره* nodep, ثابت Bin_پیاپی& پیاپی, uint32_t element,
                                       صحيح عرض) {
        ثابت صحيح runw = پیاپی.m_lo.عرض();
        V3_عدد جابه_جایی{nodep, runw};
        جابه_جایی.opMul(پیاپی.m_stride, V3_عدد{nodep, runw, element});
        V3_عدد lo{nodep, runw};
        lo.op_جمع_کن(پیاپی.m_lo, جابه_جایی);
        V3_عدد hi = پیاپی.m_hi;
        إذا (element + 1 < پیاپی.m_شمار) {
            جابه_جایی.opSub(پیاپی.m_stride, V3_عدد{nodep, runw, 1});
            hi.op_جمع_کن(lo, جابه_جایی);
        }
        Cross_مقدار_Range range{nodep, عرض};
        range.lo.opExtendS(lo, runw);
        range.hi.opExtendS(hi, runw);
        أرجع range;
    }

    ساكن صحيح cross_Range_عرض(Ast_گره* nodep) {
        إذا (ثابت AstInsideRange* ثابت rangep = VN_CAST(nodep, InsideRange)) {
            أرجع std::max(rangep->lhsp()->عرض(), rangep->rhsp()->عرض());
        }
        أرجع nodep->عرض();
    }

    ساكن منطقي cross_مقدار_Less(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) {
        V3_عدد نتیجه{&lhs};
        أرجع !نتیجه.opLtS(lhs, rhs).isEqZero();
    }

    // Round a real bound inward to the nearest coverpoint value (IEEE 1800-2023 19.5.7).  Sets
    // 'empty' if the domain has no value on the bound's side.
    ساكن فراغ crossRealBound(ثابت AstConst* constp, Ast_گره_عبارت* exprp, منطقي فوقانی,
                               ثابت Cross_مقدار_Range& domain, V3_عدد& نتیجه, منطقي& خالی) {
        ثابت مزدوج مقدار = constp->رقم().مقصد_Double();
        ثابت مزدوج bound = فوقانی ? std::floor(مقدار) : std::ceil(مقدار);
        // Powers of two are exact, so the integral bound compares exactly with the domain edges.
        ثابت مزدوج حد
            = std::ldexp(1.0, exprp->isSigned() ? exprp->عرض() - 1 : exprp->عرض());
        ثابت مزدوج minimum = exprp->isSigned() ? -حد : 0.0;
        إذا (std::isnan(bound) || (فوقانی ? bound < minimum : bound >= حد)) {
            خالی = درست;
            نتیجه = domain.lo;
        } وإلا إذا (bound < minimum) {
            نتیجه = domain.lo;
        } وإلا إذا (bound >= حد) {
            نتیجه = domain.hi;
        } وإلا {
            V3_عدد real{&نتیجه, 64};
            real.مجموعه_Double(bound);
            نتیجه.opRToIRoundS(real);
        }
    }

    ساكن منطقي crossRangeBound(Ast_گره* nodep, Ast_گره_عبارت* exprp, منطقي فوقانی, منطقي bin_مقدار,
                                ثابت Cross_مقدار_Range& domain, V3_عدد& نتیجه, منطقي& خالی) {
        إذا (VN_IS(nodep, Unbounded)) {
            V3_عدد حد{nodep, exprp->عرض()};
            إذا (فوقانی) حد.مجموعه_All_Bits1();
            إذا (exprp->isSigned()) {
                حد.مجموعه_بیت(exprp->عرض() - 1, !فوقانی);
                نتیجه.opExtendS(حد, حد.عرض());
            } وإلا {
                نتیجه.op_تخصیص_بده(حد);
            }
            أرجع درست;
        }
        ثابت AstConst* ثابت constp = VN_CAST(nodep, Const);
        إذا (!constp) أرجع خطأ;
        إذا (constp->رقم().isDouble()) {
            crossRealBound(constp, exprp, فوقانی, domain, نتیجه, خالی);
            أرجع درست;
        }
        إذا (constp->رقم().isString()) أرجع خطأ;
        إذا (bin_مقدار && exprp->isSigned() && constp->عرض() <= exprp->عرض()) {
            // Bin bit patterns use the coverpoint's effective type (IEEE 1800-2023 19.5.7).
            // Wider values and intersect filters retain their values for domain clipping.
            V3_عدد مقدار{nodep, exprp->عرض()};
            إذا (constp->isSigned()) {
                مقدار.opExtendS(constp->رقم(), constp->عرض());
            } وإلا {
                مقدار.op_تخصیص_بده(constp->رقم());
            }
            نتیجه.opExtendS(مقدار, مقدار.عرض());
        } وإلا إذا (constp->isSigned()) {
            نتیجه.opExtendS(constp->رقم(), constp->عرض());
        } وإلا {
            نتیجه.op_تخصیص_بده(constp->رقم());
        }
        أرجع درست;
    }

    ساكن Cross_مقدار_Range cross_مقدار_Domain(Ast_گره* nodep, صحيح مقدار_عرض, منطقي isSigned,
                                            صحيح عرض) {
        Cross_مقدار_Range domain{nodep, عرض};
        V3_عدد lo{nodep, مقدار_عرض};
        V3_عدد hi{nodep, مقدار_عرض};
        hi.مجموعه_All_Bits1();
        إذا (isSigned) {
            lo.مجموعه_بیت(مقدار_عرض - 1, 1);
            hi.مجموعه_بیت(مقدار_عرض - 1, 0);
            domain.lo.opExtendS(lo, مقدار_عرض);
            domain.hi.opExtendS(hi, مقدار_عرض);
        } وإلا {
            domain.lo.op_تخصیص_بده(lo);
            domain.hi.op_تخصیص_بده(hi);
        }
        أرجع domain;
    }

    ساكن فراغ intersectCrossRange(Cross_مقدار_Range& range, ثابت Cross_مقدار_Range& other) {
        إذا (cross_مقدار_Less(range.lo, other.lo)) range.lo = other.lo;
        إذا (cross_مقدار_Less(other.hi, range.hi)) range.hi = other.hi;
    }

    ساكن منطقي cross_مقدار_Range(Ast_گره* nodep, Ast_گره_عبارت* exprp, منطقي bin_مقدار, منطقي wildcard,
                                ثابت Cross_مقدار_Range& domain, Cross_مقدار_Range& range) {
        منطقي خالی = خطأ;
        ثابت AstConst* ثابت constp = VN_CAST(nodep, Const);
        إذا (ثابت AstInsideRange* ثابت rangep = VN_CAST(nodep, InsideRange)) {
            إذا (!crossRangeBound(rangep->lhsp(), exprp, خطأ, bin_مقدار, domain, range.lo, خالی)
                || !crossRangeBound(rangep->rhsp(), exprp, درست, bin_مقدار, domain, range.hi, خالی)
                || range.lo.isFourState() || range.hi.isFourState()) {
                أرجع خطأ;
            }
        } وإلا إذا (constp && constp->رقم().isDouble()) {
            // A real value participates only if integral; it has no wildcard bits.
            crossRealBound(constp, exprp, خطأ, domain, range.lo, خالی);
            crossRealBound(constp, exprp, درست, domain, range.hi, خالی);
        } وإلا {
            إذا (!crossRangeBound(nodep, exprp, خطأ, bin_مقدار, domain, range.lo, خالی)) {
                أرجع خطأ;
            }
            إذا (bin_مقدار && exprp->isSigned() && constp && !constp->isSigned()
                && constp->عرض() > exprp->عرض()) {
                منطقي representable = درست;
                لكل (صحيح بیت_2 = exprp->عرض(); بیت_2 < constp->عرض(); ++بیت_2) {
                    representable &= !constp->رقم().بیت_Is1(بیت_2);
                }
                إذا (representable) {
                    V3_عدد مقدار{nodep, exprp->عرض()};
                    مقدار.op_تخصیص_بده(constp->رقم());
                    range.lo.opExtendS(مقدار, مقدار.عرض());
                }
            }
            range.hi = range.lo;
            range.wildcard = wildcard;
            إذا (wildcard) {
                range.الگو_2 = range.lo;
                range.lo = domain.lo;
                range.hi = domain.hi;
                إذا (constp && constp->isSigned()) {
                    // Replicated X sign bits are correlated, not independent wildcards.
                    // The source domain preserves expansion-before-casting (19.5.7).
                    intersectCrossRange(
                        range, cross_مقدار_Domain(nodep, constp->عرض(), درست, domain.lo.عرض()));
                }
            }
        }
        إذا (خالی) {
            range.lo = domain.hi;
            range.hi = domain.lo;
            أرجع درست;
        }
        إذا (!range.lo.isFourState()) intersectCrossRange(range, domain);
        إذا (range.wildcard && exprp->isSigned()) {
            ثابت صحيح sign = exprp->عرض() - 1;
            إذا (constp && !constp->isSigned() && constp->عرض() > exprp->عرض()) {
                // Unsigned equality preserves every target bit pattern if discarded bits are zero.
                لكل (صحيح بیت_2 = exprp->عرض(); بیت_2 < constp->عرض(); ++بیت_2) {
                    إذا (constp->رقم().بیت_Is1(بیت_2)) {
                        range.lo = domain.hi;
                        range.hi = domain.lo;
                        أرجع درست;
                    }
                }
                لكل (صحيح بیت_2 = exprp->عرض(); بیت_2 < range.الگو_2.عرض(); ++بیت_2) {
                    إذا (range.الگو_2.بیت_Is_XZ(sign))
                        range.الگو_2.مجموعه_بیت(بیت_2, 'x');
                    وإلا
                        range.الگو_2.مجموعه_بیت(بیت_2, range.الگو_2.بیت_Is1(sign));
                }
                أرجع درست;
            }
            // Discarded high bits constrain the target sign, rather than becoming don't-cares.
            لكل (صحيح بیت_2 = exprp->عرض(); بیت_2 < range.الگو_2.عرض(); ++بیت_2) {
                إذا (range.الگو_2.بیت_Is_XZ(بیت_2)) استمر;
                ثابت منطقي مقدار = range.الگو_2.بیت_Is1(بیت_2);
                إذا (!range.الگو_2.بیت_Is_XZ(sign) && range.الگو_2.بیت_Is1(sign) != مقدار) {
                    range.lo = domain.hi;
                    range.hi = domain.lo;
                    اكسر;
                }
                range.الگو_2.مجموعه_بیت(sign, مقدار);
            }
        }
        أرجع درست;
    }

    تعداد CrossRangeState : uint8_t {
        CROSS_INSIDE_BOUNDS = 0,  // Prefix is strictly inside the interval
        CROSS_AT_پایینی = 1,
        CROSS_AT_فوقانی = 2,
        CROSS_AT_BOUNDS = CROSS_AT_پایینی | CROSS_AT_فوقانی,
        CROSS_NO_تطابق = 4  // Prefix cannot match the interval/pattern
    };

    ساكن CrossRangeState cross_Range_گام(ثابت Cross_مقدار_Range& range, CrossRangeState state,
                                          صحيح بیت_2, صحيح مقدار) {
        إذا (state == CROSS_NO_تطابق) أرجع CROSS_NO_تطابق;
        // Flipping the sign bit makes signed order lexicographic.
        ثابت منطقي sign = بیت_2 == range.الگو_2.عرض() - 1;
        إذا (!range.الگو_2.بیت_Is_XZ(بیت_2) && مقدار != (range.الگو_2.بیت_Is1(بیت_2) ^ sign)) {
            أرجع CROSS_NO_تطابق;
        }
        ثابت صحيح کم = range.lo.بیت_Is1(بیت_2) ^ sign;
        ثابت صحيح بالا = range.hi.بیت_Is1(بیت_2) ^ sign;
        إذا (((state & CROSS_AT_پایینی) && مقدار < کم)
            || ((state & CROSS_AT_فوقانی) && مقدار > بالا)) {
            أرجع CROSS_NO_تطابق;
        }
        أرجع static_cast<CrossRangeState>(
            ((state & CROSS_AT_پایینی) && مقدار == کم ? CROSS_AT_پایینی : CROSS_INSIDE_BOUNDS)
            | ((state & CROSS_AT_فوقانی) && مقدار == بالا ? CROSS_AT_فوقانی : CROSS_INSIDE_BOUNDS));
    }

    ساكن منطقي crossWildcardIntersects(ثابت Cross_مقدار_Range& range) {
        بیعلامت حالتها = 1U << CROSS_AT_BOUNDS;
        لكل (صحيح بیت_2 = range.الگو_2.عرض() - 1; بیت_2 >= 0 && حالتها; --بیت_2) {
            بیعلامت بعدی = 0;
            لكل (ثابت CrossRangeState state :
                 {CROSS_INSIDE_BOUNDS, CROSS_AT_پایینی, CROSS_AT_فوقانی, CROSS_AT_BOUNDS}) {
                إذا (!(حالتها & (1U << state))) استمر;
                لكل (صحيح مقدار = 0; مقدار < 2; ++مقدار) {
                    ثابت CrossRangeState equal = cross_Range_گام(range, state, بیت_2, مقدار);
                    إذا (equal != CROSS_NO_تطابق) بعدی |= 1U << equal;
                }
            }
            حالتها = بعدی;
        }
        أرجع حالتها != 0;
    }

    // The least value at least 'from' with the non-x bits of 'pattern', in unsigned order; false
    // if none.  A bit scan, which does not enumerate the pattern's x bits.
    ساكن منطقي بعدی_الگو_مقدار(ثابت V3_عدد& الگو_2, ثابت V3_عدد& from, V3_عدد& نتیجه) {
        نتیجه.op_تخصیص_بده(from);
        صحيح انتقال = -1;  // Lowest x bit above the bit scanned where 'from' has a 0
        لكل (صحيح بیت_2 = from.عرض() - 1; بیت_2 >= 0; --بیت_2) {
            إذا (الگو_2.بیت_Is_XZ(بیت_2)) {
                إذا (!from.بیت_Is1(بیت_2)) انتقال = بیت_2;
                استمر;
            }
            إذا (الگو_2.بیت_Is1(بیت_2) == from.بیت_Is1(بیت_2)) استمر;
            إذا (from.بیت_Is1(بیت_2)) {  // Only a greater prefix, an x bit above raised, can match
                إذا (انتقال < 0) أرجع خطأ;
                بیت_2 = انتقال;
            }
            // Then the least such value: the pattern's bits, with x bits of zero
            نتیجه.مجموعه_بیت(بیت_2, 1);
            طالما (--بیت_2 >= 0) نتیجه.مجموعه_بیت(بیت_2, الگو_2.بیت_Is1(بیت_2));
            أرجع درست;
        }
        أرجع درست;
    }

    // Append to 'runs' the maximal runs of consecutive values of 'range' that its pattern
    // matches, in value order, until there are more than 'limit'.  A match continues through
    // the pattern's trailing x bits only, which the next value's carry leaves.
    ساكن فراغ crossRangeRuns(ثابت Cross_مقدار_Range& range, اندازه_t حد,
                               std::وکتور<std::pair<V3_عدد, V3_عدد>>& runs) {
        // Values order signed, which is the unsigned order of values with the sign bit flipped
        ثابت صحيح sign = range.lo.عرض() - 1;
        ثابت خودکار flipped = [sign](V3_عدد مقدار) {
            إذا (!مقدار.بیت_Is_XZ(sign)) مقدار.مجموعه_بیت(sign, !مقدار.بیت_Is1(sign));
            أرجع مقدار;
        };
        ثابت V3_عدد الگو_2 = flipped(range.الگو_2);
        ثابت V3_عدد hi = flipped(range.hi);
        ثابت V3_عدد one{&hi, hi.عرض(), 1};
        V3_عدد trailing{&hi, hi.عرض()};
        لكل (صحيح بیت_2 = 0; بیت_2 <= sign && الگو_2.بیت_Is_XZ(بیت_2); ++بیت_2) trailing.مجموعه_بیت(بیت_2, 1);
        V3_عدد from = flipped(range.lo);
        V3_عدد اولی{&hi, hi.عرض()};
        V3_عدد آخرین{&hi, hi.عرض()};
        V3_عدد less{&hi};
        طالما (runs.اندازه() <= حد && بعدی_الگو_مقدار(الگو_2, from, اولی)
               && less.opLt(hi, اولی).isEqZero()) {
            آخرین.opOr(اولی, trailing);
            إذا (!less.opLt(hi, آخرین).isEqZero()) آخرین = hi;
            runs.emplace_پشت(flipped(اولی), flipped(آخرین));
            إذا (آخرین.isCaseEq(hi)) اكسر;
            from.op_جمع_کن(آخرین, one);
        }
    }

    // True if no coverpoint value participates in a resolved value or range.  A value with x
    // or z bits participates only as a wildcard pattern (IEEE 1800-2023 19.5.7).
    ساكن منطقي cross_Range_خالی(ثابت Cross_مقدار_Range& range) {
        إذا (range.lo.isFourState()) أرجع درست;
        أرجع cross_مقدار_Less(range.hi, range.lo)
               || (range.wildcard && !crossWildcardIntersects(range));
    }

    // Comparison width that holds both the coverpoint's and a bin or intersect value's range.
    ساكن صحيح resolve_عرض(Ast_گره* nodep, ثابت Ast_گره_عبارت* exprp) {
        أرجع std::max(exprp->عرض(), cross_Range_عرض(nodep)) + 1;
    }

    // Resolve a bin or intersect value to coverpoint values (IEEE 1800-2023 19.5.7), in the
    // width 'range' was created with.  False if the value is not a constant integral or real.
    ساكن منطقي resolve_مقدار(Ast_گره* nodep, Ast_گره_عبارت* exprp, منطقي bin_مقدار, منطقي wildcard,
                             Cross_مقدار_Range& range) {
        ثابت Cross_مقدار_Range domain
            = cross_مقدار_Domain(nodep, exprp->عرض(), exprp->isSigned(), range.lo.عرض());
        أرجع cross_مقدار_Range(nodep, exprp, bin_مقدار, wildcard, domain, range);
    }

    ساكن منطقي crossRangesIntersect(ثابت Cross_مقدار_Range& bin, ثابت Cross_مقدار_Range& filter) {
        // Values with x or z bits do not participate, even in an identical filter.
        إذا (bin.lo.isFourState() || filter.lo.isFourState()) أرجع خطأ;
        Cross_مقدار_Range تطابق = bin;
        intersectCrossRange(تطابق, filter);
        أرجع !cross_مقدار_Less(تطابق.hi, تطابق.lo)
               && (!bin.wildcard || crossWildcardIntersects(تطابق));
    }

    ساكن فراغ unsupportedCrossRange(AstCoverBinsof* selectp, منطقي& valid) {
        selectp->v3warn(COVERIGN, "Unsupported: non-constant or non-integral 'intersect' value, "
                                  "or four-state range bound.");
        valid = خطأ;
    }

    ساكن منطقي cross_مقدار_Matches_Filters(AstCoverBinsof* selectp, Ast_گره* valuep,
                                         Ast_گره_عبارت* exprp, ثابت AstCoverBin* binp,
                                         ثابت Cross_مقدار_Range& domain,
                                         ثابت std::وکتور<Cross_مقدار_Range>& filters,
                                         منطقي& valid) {
        Cross_مقدار_Range range{valuep, domain.lo.عرض()};
        إذا (!cross_مقدار_Range(valuep, exprp, درست, binp->isWildcard(), domain, range)) {
            unsupportedCrossRange(selectp, valid);
            أرجع خطأ;
        }
        لكل (ثابت Cross_مقدار_Range& filter : filters) {
            إذا (crossRangesIntersect(range, filter)) أرجع درست;
        }
        أرجع خطأ;
    }

    std::وکتور<منطقي> selectCoverpointBins(AstCoverBinsof* selectp, ثابت CoverpointBins& bins,
                                           uint32_t اولی, uint32_t شمار, منطقي& valid) {
        إذا (selectp->rangesp() && !bins.exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
            unsupportedCrossRange(selectp, valid);
            أرجع {};
        }
        std::وکتور<منطقي> selected(bins.مجموع, خطأ);
        std::وکتور<std::وکتور<Ast_گره*>> مقادیر;
        صحيح عرض = bins.exprp->عرض();
        لكل (Ast_گره* rangep = selectp->rangesp(); rangep; rangep = rangep->nextp()) {
            عرض = std::max(عرض, cross_Range_عرض(rangep));
        }
        إذا (selectp->rangesp()) {
            مقادیر.reserve(شمار);
            لكل (uint32_t i = اولی; i < اولی + شمار; ++i) {
                // A run's values are in the coverpoint type, whose width 'width' covers
                مقادیر.افزودن_پشت(bins.مقادیر[i].runp ? std::وکتور<Ast_گره*>{}
                                                     : cross_Bin_مقادیر(bins.مقادیر[i]));
                لكل (Ast_گره* ثابت valuep : مقادیر.پشت()) {
                    عرض = std::max(عرض, cross_Range_عرض(valuep));
                }
            }
        }
        // One extra bit preserves both unsigned maxima and negative signed bounds.
        ++عرض;
        ثابت Cross_مقدار_Range domain
            = cross_مقدار_Domain(selectp, bins.exprp->عرض(), bins.exprp->isSigned(), عرض);
        std::وکتور<Cross_مقدار_Range> filters;
        لكل (Ast_گره* rangep = selectp->rangesp(); rangep; rangep = rangep->nextp()) {
            Cross_مقدار_Range filter{rangep, عرض};
            إذا (!cross_مقدار_Range(rangep, bins.exprp, خطأ, خطأ, domain, filter)) {
                unsupportedCrossRange(selectp, valid);
                أرجع {};
            }
            filters.افزودن_پشت(std::move(filter));
        }
        لكل (uint32_t i = اولی; i < اولی + شمار; ++i) {
            إذا (!selectp->rangesp()) {
                selected[i] = درست;
                استمر;
            }
            إذا (ثابت Bin_پیاپی* ثابت runp = bins.مقادیر[i].runp) {
                ثابت Cross_مقدار_Range range
                    = پیاپی_Bin_Range(selectp, *runp, bins.مقادیر[i].element, عرض);
                selected[i] = std::any_از(filters.begin(), filters.انتها(),
                                          [&](ثابت Cross_مقدار_Range& filter) {
                                              أرجع crossRangesIntersect(range, filter);
                                          });
                استمر;
            }
            لكل (Ast_گره* ثابت valuep : مقادیر[i - اولی]) {
                selected[i] = cross_مقدار_Matches_Filters(
                    selectp, valuep, bins.exprp, bins.مقادیر[i].binp, domain, filters, valid);
                إذا (!valid) أرجع {};
                إذا (selected[i]) اكسر;
            }
        }
        إذا (selectp->isNegated()) {
            لكل (uint32_t i = 0; i < bins.مجموع; ++i) selected[i] = !selected[i];
        }
        أرجع selected;
    }

    // Constructor-time value metadata of one coverpoint, as C++ list entries
    بنية مقدار_Lists final {
        std::وکتور<std::string> m_ranges;  // Bin, then low and high words
        std::وکتور<std::string> m_runs;  // First bin, count, then low, span, and high words
        std::وکتور<std::string> m_patterns;  // Bin, then value, mask, low, and high words
        std::وکتور<std::string> m_transitions;  // Transition bin
    };

    // Append a value's words, in the coverpoint's width, to a C++ list entry.
    ساكن فراغ append_واژگان(std::string& متن, ثابت V3_عدد& مقدار, ثابت Ast_گره_عبارت* exprp) {
        V3_عدد narrowed{&مقدار, exprp->عرض()};
        narrowed.op_تخصیص_بده(مقدار);
        لكل (صحيح کلمه = 0; کلمه < exprp->عرض_واژگان(); ++کلمه) {
            متن += ", " + cvt_مقصد_Str(narrowed.edata_کلمه(کلمه)) + "U";
        }
    }

    فراغ collect_مقدار_Metadata(مقدار_Lists& lists, Ast_گره_عبارت* exprp, AstCoverBin* binp,
                              uint32_t شاخص, Ast_گره_عبارت* valuep) {
        ثابت std::string bin = cvt_مقصد_Str(شاخص) + "U";
        إذا (binp->transp()) lists.m_transitions.افزودن_پشت(bin);
        لكل (Ast_گره* ثابت sourcep : cross_Bin_مقادیر({binp, valuep})) {
            Cross_مقدار_Range range{sourcep, resolve_عرض(sourcep, exprp)};
            إذا (!resolve_مقدار(sourcep, exprp, درست, binp->isWildcard(), range)) {
                // Sampling already resolved every state bin value, so only transitions remain.
                sourcep->v3warn(E_UNSUPPORTED, "Unsupported: non-integral value in a transition "
                                               "bin of a coverpoint with exclusions.");
                استمر;
            }
            إذا (cross_Range_خالی(range)) استمر;
            std::string مدخل = bin;
            إذا (range.wildcard) {
                V3_عدد مقدار{sourcep, exprp->عرض()};
                V3_عدد ماسک{sourcep, exprp->عرض()};
                ماسک.op_بیتها_Non_XZ(range.الگو_2);
                مقدار.op_بیتها_One(range.الگو_2);
                append_واژگان(مدخل, مقدار, exprp);
                append_واژگان(مدخل, ماسک, exprp);
            }
            append_واژگان(مدخل, range.lo, exprp);
            append_واژگان(مدخل, range.hi, exprp);
            (range.wildcard ? lists.m_patterns : lists.m_ranges).افزودن_پشت(مدخل);
        }
    }

    // Describe a run with one entry, from which the runtime computes the values of its bins.
    ساكن فراغ collect_پیاپی_Metadata(مقدار_Lists& lists, Ast_گره_عبارت* exprp, ثابت Bin_پیاپی& پیاپی) {
        V3_عدد بازه{exprp, پیاپی.m_stride.عرض(), 0};
        بازه.opSub(پیاپی.m_stride, V3_عدد{exprp, پیاپی.m_stride.عرض(), 1});
        std::string مدخل = cvt_مقصد_Str(پیاپی.m_declared) + "U, " + cvt_مقصد_Str(پیاپی.m_شمار) + "U";
        append_واژگان(مدخل, پیاپی.m_lo, exprp);
        append_واژگان(مدخل, بازه, exprp);
        append_واژگان(مدخل, پیاپی.m_hi, exprp);
        lists.m_runs.افزودن_پشت(مدخل);
    }

    // Emit one batched metadata list, bounding the size of each call's temporary list.
    فراغ emit_مقدار_List(File_سطر* fl, AstVar* cpVarp, VCMethod روش,
                       ثابت std::وکتور<std::string>& entries) {
        لكل (اندازه_t اولی = 0; اولی < entries.اندازه(); اولی += مقدار_LIST_ENTRIES) {
            ثابت اندازه_t انتها = std::min(entries.اندازه(), اولی + مقدار_LIST_ENTRIES);
            std::string متن = "{" + entries[اولی];
            لكل (اندازه_t i = اولی + 1; i < انتها; ++i) متن += ", " + entries[i];
            m_constructorp->جمع_کن_Stmtsp(
                قلم_Call(fl, cpVarp, روش, {ctext(fl, متن + "}")})->makeStmt());
        }
    }

    ساكن منطقي checkCrossRef(ثابت AstCoverCrossRef* refp, ثابت AstCoverCross* crossp) {
        إذا (refp->name() == crossp->name()) أرجع درست;
        refp->v3error("Cross selection "
                      << refp->prettyNameQ() << " may only name its enclosing cross "
                      << crossp->prettyNameQ() << " (IEEE 1800-2023 19.6.1.2).");
        أرجع خطأ;
    }

    ساكن منطقي checkCrossBinName(ثابت AstCoverCrossBin* binp, std::مجموعه<std::string>& نامها) {
        إذا (نامها.emplace(binp->name()).دومی) أرجع درست;
        binp->v3error("Duplicate cross bin " << binp->prettyNameQ()
                                             << " (IEEE 1800-2023 19.6.1).");
        أرجع خطأ;
    }

    // The span of the implicit automatic bin reported as 'name' ('auto_<i>', see
    // createImplicitAutoBins), found without naming each of its bins
    ساكن منطقي implicit_Auto_Bin_بازه(ثابت CoverpointBins& bins, ثابت std::string& name,
                                    Bin_بازه& بازه) {
        ثابت std::string پیشوند = "auto_";
        إذا (!VString::startsWith(name, پیشوند)) أرجع خطأ;
        ثابت std::string digits = name.substr(پیشوند.اندازه());
        ثابت بیعلامت دراز شاخص = std::strtoul(digits.c_str(), اشاره_تهی, 10);
        // Only the reported spelling names the bin, not e.g. 'auto_01' or 'auto_x'
        إذا (شاخص >= bins.implicitAuto.شمار || digits != cvt_مقصد_Str(شاخص)) أرجع خطأ;
        ثابت uint32_t جابه_جایی = static_cast<uint32_t>(شاخص);
        بازه = Bin_بازه{bins.implicitAuto.اولی + جابه_جایی, 1, bins.implicitAuto.declared + جابه_جایی};
        أرجع درست;
    }

    Cross_Binsof_هدف
    resolve_Binsof_هدف(ثابت AstCoverBinsof* selectp, ثابت AstCoverCross* crossp,
                        ثابت std::وکتور<AstVar*>& cpVars,
                        ثابت std::map<std::string, uint32_t>& dimensions) ثابت {
        ثابت خودکار dim = dimensions.پیدا_کن(selectp->pointp()->name());
        إذا (dim == dimensions.انتها()) {
            selectp->v3error("binsof coverpoint "
                             << selectp->pointp()->prettyNameQ() << " is not an item of cross "
                             << crossp->prettyNameQ() << " (IEEE 1800-2023 19.6.1).");
            أرجع {};
        }
        ثابت CoverpointBins& bins = m_cpBins.at(cpVars[dim->دومی]);
        Cross_Binsof_هدف هدف{&bins, dim->دومی, 0, bins.مجموع};
        إذا (!selectp->name().خالی()) {
            ثابت خودکار bin = bins.spans.پیدا_کن(selectp->name());
            Bin_بازه بازه{0, 0, 0};
            إذا (bin != bins.spans.انتها()) {
                بازه = bin->دومی;
            } وإلا إذا (!implicit_Auto_Bin_بازه(bins, selectp->name(), بازه)) {
                selectp->v3error("Cannot find bin " << selectp->prettyNameQ() << " in coverpoint "
                                                    << selectp->pointp()->prettyNameQ()
                                                    << " (IEEE 1800-2023 19.6.1).");
                أرجع {};
            }
            هدف.m_اولی = بازه.اولی;
            هدف.m_شمار = بازه.شمار;
            هدف.m_declared_اولی = بازه.declared;
            هدف.m_declared_انتها = بازه.declared + بازه.شمار;
            هدف.m_sized = بازه.sized;
        }
        أرجع هدف;
    }

    // The variables a 'with' filter reads, each by its first reference
    استفاده CrossFilterReads = std::map<ثابت AstVar*, AstVarRef*>;

    // Whether a 'with' filter has a count of value tuples, of 'matches' other than '$'
    ساكن منطقي counts_مقدار_Tuples(ثابت AstCoverCrossWith* withp) {
        أرجع withp->countp() && !VN_IS(withp->countp(), Unbounded);
    }

    // Whether the runtime knows the values of the cross items a 'with' filter reads, or to count
    // value tuples, of all items, of integral coverpoints; else warns
    ساكن منطقي crossFilterSupported(ثابت AstCoverCrossWith* withp,
                                     ثابت CrossFilterReads& reads) {
        لكل (ثابت AstVar* varp = withp->varsp(); varp; varp = VN_AS(varp->nextp(), Var)) {
            إذا (varp->dtypep()->skipRefp()->isIntegralOrPacked()) استمر;
            ثابت خودکار it = reads.پیدا_کن(varp);
            إذا (it != reads.انتها()) {
                it->دومی->v3warn(COVERIGN, "Unsupported: cross bin 'with' filter reading the "
                                             "value of non-integral coverpoint "
                                                 << it->دومی->prettyNameQ());
                أرجع خطأ;
            }
            إذا (counts_مقدار_Tuples(withp)) {
                withp->countp()->v3warn(COVERIGN, "Unsupported: cross bin 'matches' count of "
                                                  "value tuples of non-integral coverpoint "
                                                      << varp->prettyNameQ());
                أرجع خطأ;
            }
        }
        أرجع درست;
    }

    // Emit the constructor code selecting, of the bin tuples of the selection, those with enough
    // value tuples for which the 'with' filter is true: one, all for 'matches $', or a count
    // (IEEE 1800-2023 19.6.1.2).  The runtime gives the values of the cross items the filter
    // reads, of each value tuple it may evaluate:
    //   withBegin({dimensions}, count, positive, bin, file, line);
    //   while (withNext()) {
    //       item = withValue(dimension);  // Of each cross item read
    //       withTest(filter);
    //   }
    //   withEnd();
    // The dimensions are those of the items read, or to count value tuples, of all items: the
    // filter's result for a value tuple of the items it reads repeats for each of the others'.
    فراغ generateCrossFilter(ثابت AstCoverCrossWith* withp, ثابت CrossFilterReads& reads,
                             ثابت AstCoverCrossBin* binp, ثابت AstCoverCross* crossp,
                             AstVar* cxp, ثابت std::map<string, uint32_t>& dimensions,
                             uint32_t اقلام) {
        File_سطر* ثابت fl = withp->fileline();
        ثابت string پیشوند
            = m_sized_نامها.get(sanitizeGeneratedName(crossp->name() + "__" + binp->name()));
        ثابت منطقي counting = counts_مقدار_Tuples(withp);
        // The count, 0 for all
        ثابت std::pair<Ast_گره_عبارت*, Ast_گره_عبارت*> شمار
            = counting ? new_شمار_آرگومانها(fl, پیشوند + "_count", withp->countp())
                       : new_شمار_آرگومانها(fl, withp->countp() ? 0 : 1);
        AstLoop* ثابت loopp = جديد AstLoop{fl};
        AstCMethodHard* ثابت nextp = قلم_Call(fl, cxp, VCMethod::COVERGROUP_WITH_بعدی);
        nextp->dtype_مجموعه_بیت();
        loopp->جمع_کن_Stmtsp(جديد AstLoopTest{fl, loopp, nextp});
        string dims;
        لكل (uint32_t dim = 0; counting && dim < اقلام; ++dim) {
            dims += (dims.خالی() ? "" : ", ") + cvt_مقصد_Str(dim) + "U";
        }
        طالما (AstVar* ثابت varp = withp->varsp()) {
            varp->unlink_Fr_پشت();
            إذا (!reads.شمار(varp)) {
                VL_DO_DANGLING(افزودن_Deletep(varp), varp);
                استمر;
            }
            ثابت خودکار dimIt = dimensions.پیدا_کن(varp->name());
            UASSERT_OBJ(dimIt != dimensions.انتها(), varp, "Cross 'with' candidate not of an item");
            ثابت uint32_t dim = dimIt->دومی;
            إذا (!counting) dims += (dims.خالی() ? "" : ", ") + cvt_مقصد_Str(dim) + "U";
            varp->name(پیشوند + "_" + sanitizeGeneratedName(varp->name()));
            m_constructorp->جمع_کن_Stmtsp(varp);
            إذا (varp->isWide()) {
                loopp->جمع_کن_Stmtsp(قلم_Call(fl, cxp, VCMethod::COVERGROUP_CROSS_WITH_مقدار_W,
                                          {cnum(fl, dim), جديد AstVarRef{fl, varp, VAccess::WRITE}})
                                     ->makeStmt());
            } وإلا {
                AstCMethodHard* ثابت valuep
                    = قلم_Call(fl, cxp, VCMethod::COVERGROUP_CROSS_WITH_مقدار, {cnum(fl, dim)});
                valuep->dtype_مجموعه_UInt64();
                Ast_گره_DType* ثابت dtypep = varp->پیدا_کن_Logic_DType(
                    varp->عرض(), varp->عرض(),
                    varp->isSigned() ? VSigning::SIGNED : VSigning::UNSIGNED);
                loopp->جمع_کن_Stmtsp(جديد Ast_تخصیص_بده{fl, جديد AstVarRef{fl, varp, VAccess::WRITE},
                                               resize_مقدار(valuep, dtypep)});
            }
        }
        loopp->جمع_کن_Stmtsp(قلم_Call(fl, cxp, VCMethod::COVERGROUP_CROSS_WITH_TEST,
                                  {withp->filterp()->unlink_Fr_پشت()})
                             ->makeStmt());
        ثابت منطقي prot = v3Global.opt.protectIds();
        m_constructorp->جمع_کن_Stmtsp(
            قلم_Call(fl, cxp, VCMethod::COVERGROUP_CROSS_WITH_BEGIN,
                     {ctext(fl, "{" + dims + "}"), شمار.اولی, شمار.دومی,
                      ctext(fl, quoted(VIdProtect::protect_واژگان_If(binp->name(), prot))),
                      ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), prot))),
                      cnum(fl, static_cast<uint32_t>(fl->lineno()))})
                ->makeStmt());
        m_constructorp->جمع_کن_Stmtsp(loopp);
        m_constructorp->جمع_کن_Stmtsp(
            قلم_Call(fl, cxp, VCMethod::COVERGROUP_CROSS_WITH_انتها)->makeStmt());
    }

    منطقي generateRuntimeSelection(Ast_گره* nodep, ثابت AstCoverCrossBin* binp,
                                  AstCoverCross* crossp, AstVar* cxp,
                                  ثابت std::وکتور<AstVar*>& cpVars,
                                  ثابت std::map<string, uint32_t>& dimensions) {
        File_سطر* ثابت fl = nodep->fileline();
        إذا (ثابت AstCoverCrossRef* ثابت refp = VN_CAST(nodep, CoverCrossRef)) {
            إذا (!checkCrossRef(refp, crossp)) أرجع خطأ;
            m_constructorp->جمع_کن_Stmtsp(
                قلم_Call(fl, cxp, VCMethod::COVERGROUP_SELECT_ALL)->makeStmt());
            أرجع درست;
        }
        إذا (ثابت AstCoverCrossSelect* ثابت opp = VN_CAST(nodep, CoverCrossSelect)) {
            إذا (!generateRuntimeSelection(opp->lhsp(), binp, crossp, cxp, cpVars, dimensions)
                || !generateRuntimeSelection(opp->rhsp(), binp, crossp, cxp, cpVars, dimensions)) {
                أرجع خطأ;
            }
            m_constructorp->جمع_کن_Stmtsp(قلم_Call(fl, cxp,
                                               opp->isOr() ? VCMethod::COVERGROUP_SELECT_OR
                                                           : VCMethod::COVERGROUP_SELECT_AND)
                                          ->makeStmt());
            أرجع درست;
        }
        إذا (ثابت AstCoverCrossWith* ثابت withp = VN_CAST(nodep, CoverCrossWith)) {
            CrossFilterReads reads;
            withp->filterp()->foreach([&](AstVarRef* refp) { reads.emplace(refp->varp(), refp); });
            // The count of 'matches' is a positive integer, or '$' (IEEE 1800-2023 19.6.1.2)
            إذا ((counts_مقدار_Tuples(withp)
                 && !check_شمار(withp->countp(), "Cross bin 'matches' count", "19.6.1.2"))
                || !crossFilterSupported(withp, reads)
                || !generateRuntimeSelection(withp->selectp(), binp, crossp, cxp, cpVars,
                                             dimensions)) {
                أرجع خطأ;
            }
            generateCrossFilter(withp, reads, binp, crossp, cxp, dimensions,
                                static_cast<uint32_t>(cpVars.اندازه()));
            أرجع درست;
        }
        AstCoverBinsof* ثابت selectp = VN_AS(nodep, CoverBinsof);
        ثابت Cross_Binsof_هدف هدف = resolve_Binsof_هدف(selectp, crossp, cpVars, dimensions);
        إذا (!هدف.m_binsp) أرجع خطأ;
        ثابت CoverpointBins& bins = *هدف.m_binsp;
        إذا (selectp->rangesp() && !bins.exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
            منطقي valid = درست;
            unsupportedCrossRange(selectp, valid);
            أرجع خطأ;
        }
        Ast_گره_عبارت* firstp;
        Ast_گره_عبارت* endp;
        إذا (هدف.m_sized < 0) {
            firstp = cnum(fl, هدف.m_declared_اولی);
            endp = cnum(fl, هدف.m_declared_انتها);
        } وإلا {  // A sized array, whose bins the coverpoint's construction placed
            AstVar* ثابت cpVarp = cpVars[هدف.m_dimension];
            ثابت uint32_t sized = static_cast<uint32_t>(هدف.m_sized);
            firstp = قلم_Call(fl, cpVarp, VCMethod::COVERGROUP_SIZED_اولی, {cnum(fl, sized)});
            endp = قلم_Call(fl, cpVarp, VCMethod::COVERGROUP_SIZED_انتها, {cnum(fl, sized)});
            firstp->dtype_مجموعه_UInt32();
            endp->dtype_مجموعه_UInt32();
        }
        m_constructorp->جمع_کن_Stmtsp(
            قلم_Call(fl, cxp, VCMethod::COVERGROUP_SELECT_DIM,
                     {cnum(fl, هدف.m_dimension), firstp, endp, cnum(fl, selectp->isNegated()),
                      cnum(fl, selectp->rangesp() != اشاره_تهی)})
                ->makeStmt());
        لكل (Ast_گره* rangep = selectp->rangesp(); rangep; rangep = rangep->nextp()) {
            Cross_مقدار_Range range{rangep, resolve_عرض(rangep, bins.exprp)};
            إذا (!resolve_مقدار(rangep, bins.exprp, خطأ, خطأ, range)) {
                منطقي valid = درست;
                unsupportedCrossRange(selectp, valid);
                أرجع خطأ;
            }
            إذا (cross_Range_خالی(range)) استمر;
            m_constructorp->جمع_کن_Stmtsp(قلم_Call(fl, cxp,
                                               bins.exprp->isWide()
                                                   ? VCMethod::COVERGROUP_SELECT_RANGE_W
                                                   : VCMethod::COVERGROUP_SELECT_RANGE,
                                               {new_مقدار_Const(fl, range.lo, bins.exprp),
                                                new_مقدار_Const(fl, range.hi, bins.exprp)})
                                          ->makeStmt());
        }
        m_constructorp->جمع_کن_Stmtsp(
            قلم_Call(fl, cxp, VCMethod::COVERGROUP_SELECT_DIM_انتها)->makeStmt());
        أرجع درست;
    }

    std::وکتور<AstCoverCrossBin*>
    generateRuntimeCrossBins(AstCoverCross* crossp, AstVar* cxp,
                             ثابت std::وکتور<AstVar*>& cpVars,
                             ثابت std::map<string, uint32_t>& dimensions) {
        std::وکتور<AstCoverCrossBin*> bins;
        std::مجموعه<string> نامها;
        لكل (Ast_گره* itemp = crossp->binsp(); itemp; itemp = itemp->nextp()) {
            AstCoverCrossBin* ثابت binp = VN_AS(itemp, CoverCrossBin);
            إذا (!checkCrossBinName(binp, نامها)) استمر;
            File_سطر* ثابت fl = binp->fileline();
            // Generate the selection apart, so that an unsupported one leaves no code, which
            // could leave its state for the next bin
            AstFunc* ثابت selectionp = جديد AstFunc{fl, "", اشاره_تهی, اشاره_تهی};
            منطقي supported;
            {
                VL_RESTORER(m_constructorp);
                m_constructorp = selectionp;
                supported = generateRuntimeSelection(binp->selectp(), binp, crossp, cxp, cpVars,
                                                     dimensions);
            }
            // A supported selection has code
            إذا (supported) m_constructorp->جمع_کن_Stmtsp(selectionp->stmtsp()->unlink_Fr_پشت_With_بعدی());
            VL_DO_DANGLING(افزودن_Deletep(selectionp), selectionp);
            إذا (!supported) استمر;
            ثابت منطقي protect = v3Global.opt.protectIds();
            m_constructorp->جمع_کن_Stmtsp(
                قلم_Call(fl, cxp, VCMethod::COVERGROUP_SELECT_BIN,
                         {ctext(fl, binp->binsType().bin_مجموعه_Enum()),
                          ctext(fl, quoted(VIdProtect::protect_واژگان_If(binp->name(), protect))),
                          ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), protect))),
                          cnum(fl, fl->lineno()), cnum(fl, fl->اولی_ستونی()),
                          cnum(fl, static_cast<uint32_t>(bins.اندازه()))})
                    ->makeStmt());
            bins.افزودن_پشت(binp);
        }
        m_constructorp->جمع_کن_Stmtsp(
            قلم_Call(crossp->fileline(), cxp, VCMethod::COVERGROUP_FINALIZE_BINS)->makeStmt());
        أرجع bins;
    }

    ساكن فراغ مجموعه_Cross_Selection_Range(CrossSelection& selection, uint64_t اولی, uint64_t انتها) {
        طالما (اولی < انتها) {
            ثابت بیعلامت بیت_2 = اولی % 64;
            ثابت بیعلامت بیتها = std::min<uint64_t>(64 - بیت_2, انتها - اولی);
            selection[VL_BITWORD_Q(اولی)]
                |= (بیتها == 64 ? ~uint64_t{0} : (uint64_t{1} << بیتها) - 1) << بیت_2;
            اولی += بیتها;
        }
    }

    CrossSelection crossSelection(Ast_گره* nodep, CrossSelectionContext& ctx) {
        إذا (ثابت AstCoverCrossRef* ثابت refp = VN_CAST(nodep, CoverCrossRef)) {
            إذا (!checkCrossRef(refp, ctx.crossp)) {
                ctx.valid = خطأ;
                أرجع {};
            }
            CrossSelection نتیجه(
                VL_BITWORD_Q(static_cast<uint64_t>(ctx.tuples) + VL_QUADSIZE - 1), 0);
            مجموعه_Cross_Selection_Range(نتیجه, 0, ctx.tuples);
            أرجع نتیجه;
        }
        إذا (AstCoverCrossSelect* ثابت opp = VN_CAST(nodep, CoverCrossSelect)) {
            CrossSelection lhs = crossSelection(opp->lhsp(), ctx);
            ثابت CrossSelection rhs = crossSelection(opp->rhsp(), ctx);
            إذا (!ctx.valid) أرجع {};
            لكل (اندازه_t i = 0; i < lhs.اندازه(); ++i) {
                lhs[i] = opp->isOr() ? lhs[i] | rhs[i] : lhs[i] & rhs[i];
            }
            أرجع lhs;
        }
        AstCoverBinsof* ثابت selectp = VN_AS(nodep, CoverBinsof);
        ثابت Cross_Binsof_هدف هدف
            = resolve_Binsof_هدف(selectp, ctx.crossp, ctx.cpVars, ctx.dimensions);
        إذا (!هدف.m_binsp) {
            ctx.valid = خطأ;
            أرجع {};
        }
        ثابت CoverpointBins& bins = *هدف.m_binsp;
        // Coverpoints with sized arrays are runtime points, so feed only runtime crosses
        UASSERT_OBJ(هدف.m_sized < 0, selectp, "Sized bin array selected by a static cross");
        ثابت std::وکتور<منطقي> selected
            = selectCoverpointBins(selectp, bins, هدف.m_اولی, هدف.m_شمار, ctx.valid);
        إذا (!ctx.valid) أرجع {};
        CrossSelection نتیجه(VL_BITWORD_Q(static_cast<uint64_t>(ctx.tuples) + VL_QUADSIZE - 1),
                              0);
        ثابت uint64_t stride = ctx.strides[هدف.m_dimension];
        ثابت uint64_t period = stride * bins.مجموع;
        لكل (uint64_t پایه = 0; پایه < ctx.tuples; پایه += period) {
            لكل (uint32_t i = 0; i < bins.مجموع;) {
                إذا (!selected[i]) {
                    ++i;
                    استمر;
                }
                ثابت uint32_t begin = i++;
                طالما (i < bins.مجموع && selected[i]) ++i;
                مجموعه_Cross_Selection_Range(نتیجه, پایه + begin * stride, پایه + i * stride);
            }
        }
        أرجع نتیجه;
    }

    // Size the Cartesian product of the declared Normal bins, with each dimension's flat-index
    // stride.  Live runtime bins only shrink it.  Warns and returns false if it is too large.
    منطقي crossShape(AstCoverCross* crossp, ثابت std::وکتور<AstVar*>& cpVars,
                    std::وکتور<uint32_t>& strides, uint32_t& tuples) ثابت {
        uint64_t حاصلضرب = std::any_از(cpVars.begin(), cpVars.انتها(),
                                       [هذا](AstVar* varp) { أرجع !m_cpBins.at(varp).مجموع; })
                               ? 0
                               : 1;
        strides.resize(cpVars.اندازه());
        لكل (اندازه_t d = cpVars.اندازه(); d > 0; --d) {
            strides[d - 1] = static_cast<uint32_t>(حاصلضرب);
            حاصلضرب *= m_cpBins.at(cpVars[d - 1]).مجموع;
            إذا (حاصلضرب > UINT32_MAX) {
                crossp->v3warn(COVERIGN,
                               "Unsupported: cross coverage with more than 2^32-1 tuples.");
                أرجع خطأ;
            }
        }
        tuples = static_cast<uint32_t>(حاصلضرب);
        أرجع درست;
    }

    CrossLayout resolveCrossLayout(AstCoverCross* crossp, ثابت std::وکتور<AstVar*>& cpVars,
                                   ثابت std::map<std::string, uint32_t>& dimensions) {
        CrossLayout layout;
        CrossSelectionContext ctx{crossp, cpVars, dimensions, 0, {}};
        إذا (!crossShape(crossp, cpVars, ctx.strides, ctx.tuples)) {
            layout.valid = خطأ;
            أرجع layout;
        }
        layout.tuples = ctx.tuples;
        CrossSelection occupied;
        CrossSelection excluded;
        std::مجموعه<std::string> نامها;
        لكل (Ast_گره* itemp = crossp->binsp(); itemp; itemp = itemp->nextp()) {
            AstCoverCrossBin* ثابت binp = VN_AS(itemp, CoverCrossBin);
            إذا (!checkCrossBinName(binp, نامها)) استمر;
            ctx.valid = درست;
            CrossSelection selection = crossSelection(binp->selectp(), ctx);
            إذا (!ctx.valid || std::all_از(selection.begin(), selection.انتها(), [](uint64_t کلمه) {
                    أرجع کلمه == 0;
                })) {
                استمر;
            }
            إذا (occupied.خالی()) {
                occupied.resize(selection.اندازه(), 0);
                excluded.resize(selection.اندازه(), 0);
            }
            لكل (اندازه_t i = 0; i < selection.اندازه(); ++i) {
                occupied[i] |= selection[i];
                إذا (!binp->binsType().bin_Is_نرمال()) excluded[i] |= selection[i];
            }
            layout.bins.افزودن_پشت({binp, std::move(selection)});
        }
        إذا (!layout.bins.خالی()) {
            // IEEE 1800-2023 19.6.2/19.6.3: exclusions also remove tuples from named
            // bins, independently of declaration order and sampling guards.
            لكل (ResolvedCrossBin& resolved : layout.bins) {
                لكل (اندازه_t i = 0; i < resolved.selection.اندازه(); ++i) {
                    إذا (resolved.binp->binsType().bin_Is_نرمال()) {
                        resolved.selection[i] &= ~excluded[i];
                    }
                    إذا (resolved.selection[i]) ++layout.bin_واژگان;
                }
            }
            layout.bins.erase(std::remove_if(layout.bins.begin(), layout.bins.انتها(),
                                             [](ثابت ResolvedCrossBin& resolved) {
                                                 أرجع std::all_از(
                                                     resolved.selection.begin(),
                                                     resolved.selection.انتها(),
                                                     [](uint64_t کلمه) { أرجع کلمه == 0; });
                                             }),
                              layout.bins.انتها());
            layout.autoBins = layout.tuples;
            لكل (ثابت uint64_t کلمه : occupied) {
                layout.autoBins -= static_cast<uint32_t>(std::bitset<VL_QUADSIZE>{کلمه}.شمار());
            }
        }
        أرجع layout;
    }

    AstCoverCrossDType* crossDType(File_سطر* fl, uint32_t dimensions, ثابت CrossLayout& layout,
                                   منطقي dynamic = خطأ) {
        ثابت uint32_t bins = static_cast<uint32_t>(layout.bins.اندازه());
        ثابت CrossShape shape{dimensions,      layout.tuples,   bins,
                               layout.autoBins, layout.bin_واژگان, dynamic};
        AstCoverCrossDType*& typep = m_cxDTypes[shape];
        إذا (!typep) {
            typep = جديد AstCoverCrossDType{
                fl, dimensions, layout.tuples, bins, layout.autoBins, layout.bin_واژگان, dynamic};
            v3Global.rootp()->typeTablep()->جمع_کن_Typesp(typep);
        }
        أرجع typep;
    }

    std::وکتور<AstCoverCrossBin*> generateCrossBins(AstCoverCross* crossp, AstVar* cxVarp,
                                                     ثابت CrossLayout& layout) {
        std::وکتور<AstCoverCrossBin*> bins;
        لكل (ثابت ResolvedCrossBin& resolved : layout.bins) {
            AstCoverCrossBin* ثابت binp = resolved.binp;
            ثابت CrossSelection& selection = resolved.selection;
            File_سطر* ثابت fl = binp->fileline();
            ثابت منطقي prot = v3Global.opt.protectIds();
            std::string ماسک = "{";
            لكل (اندازه_t i = 0; i < selection.اندازه(); ++i) {
                إذا (i) ماسک += ", ";
                ماسک += std::مقصد_string(selection[i]) + "ULL";
            }
            ماسک += "}";
            m_constructorp->جمع_کن_Stmtsp(
                قلم_Call(fl, cxVarp, VCMethod::COVERGROUP_جمع_کن_BIN,
                         {ctext(fl, binp->binsType().bin_مجموعه_Enum()), ctext(fl, ماسک),
                          ctext(fl, quoted(VIdProtect::protect_واژگان_If(binp->name(), prot))),
                          ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), prot))),
                          cnum(fl, static_cast<uint32_t>(fl->lineno())),
                          cnum(fl, static_cast<uint32_t>(fl->اولی_ستونی()))})
                    ->makeStmt());
            bins.افزودن_پشت(binp);
        }
        إذا (!bins.خالی()) {
            m_constructorp->جمع_کن_Stmtsp(
                قلم_Call(crossp->fileline(), cxVarp, VCMethod::COVERGROUP_FINALIZE_BINS)
                    ->makeStmt());
        }
        أرجع bins;
    }

    // Route a cross through a VlCoverCross member: emit the member, its constructor init +
    // registration, and the sample() call.  The feeding coverpoints are already generated
    // (their hit lists drive the cross). Each explicit bin adds one configuration call.
    فراغ generateCross(AstCoverCross* crossp) {
        File_سطر* ثابت fl = crossp->fileline();
        ثابت منطقي dynamic = m_runtimeCrosses.شمار(crossp);
        UINFO(4, "  Generating VlCoverCross member: " << crossp->name());

        إذا (Ast_گره_عبارت* ثابت iffp = crossp->iffp()) {
            crossp->iffp(
                capture_Iff_مقصد_موقت(iffp, "__VcrossIff_" + sanitizeGeneratedName(crossp->name())));
        }

        // Resolve and unlink the coverpoint refs, in dimension order.  Every ref resolves to a
        // known coverpoint (a cross with an unresolvable item was dropped earlier).
        std::وکتور<AstVar*> cpVars;
        std::map<std::string, uint32_t> dimensions;
        لكل (Ast_گره* itemp = crossp->itemsp(); itemp;) {
            Ast_گره* ثابت nextp = itemp->nextp();
            AstCoverpointRef* ثابت refp = VN_AS(itemp, CoverpointRef);
            ثابت خودکار it = m_cpVarMap.پیدا_کن(refp->name());
            UASSERT_OBJ(it != m_cpVarMap.انتها(), crossp, "Cross references an unknown coverpoint");
            dimensions.emplace(refp->name(), static_cast<uint32_t>(cpVars.اندازه()));
            cpVars.افزودن_پشت(it->دومی);
            VL_DO_DANGLING(افزودن_Deletep(refp->unlink_Fr_پشت()), refp);
            itemp = nextp;
        }
        ثابت صحيح dims = static_cast<صحيح>(cpVars.اندازه());
        CrossLayout layout;
        إذا (dynamic) {
            // The runtime sizes the layout from live bins; only check the declared bound here.
            std::وکتور<uint32_t> strides;
            uint32_t tuples = 0;
            layout.valid = crossShape(crossp, cpVars, strides, tuples);
        } وإلا {
            layout = resolveCrossLayout(crossp, cpVars, dimensions);
        }
        إذا (!layout.valid) أرجع;

        AstVar* ثابت cxVarp
            = جديد AstVar{fl, VVarType::MEMBER, "__Vcx_" + crossp->name(),
                         crossDType(fl, static_cast<uint32_t>(dims), layout, dynamic)};
        m_covergroupp->جمع_کن_Membersp(cxVarp);
        m_constructorp->جمع_کن_Stmtsp(make_قلم_Create(fl, cxVarp,
                                                 dynamic ? VCMethod::COVERGROUP_جمع_کن_CROSS_DYN
                                                         : VCMethod::COVERGROUP_جمع_کن_CROSS));
        generate_قلم_وزن(fl, cxVarp, crossp->optionsp());
        صحيح at_کمترین_مقدار;
        صحيح autoBinMax;  // Not a cross option
        استخراج_قلم_Options(crossp->optionsp(), at_کمترین_مقدار, autoBinMax);

        // Constructor: init (after the coverpoints, which generate earlier) then registration.
        // Obfuscate the hierarchy/filename/page under --protect-ids as for coverpoints above.
        ثابت منطقي prot = v3Global.opt.protectIds();
        ثابت std::string hier
            = VIdProtect::protect_واژگان_If(m_covergroupName + "." + crossp->name(), prot);
        m_constructorp->جمع_کن_Stmtsp(makeCrossCpsCall(
            fl, cpVars,
            قلم_Call(fl, cxVarp, VCMethod::COVERGROUP_INIT,
                     {ctext(fl, quoted(hier)), cnum(fl, static_cast<uint32_t>(at_کمترین_مقدار)),
                      cnum(fl, static_cast<uint32_t>(dims)), ctext(fl, "__Vcx_cps"),
                      ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), prot))),
                      cnum(fl, static_cast<uint32_t>(fl->lineno())),
                      cnum(fl, static_cast<uint32_t>(fl->اولی_ستونی()))})));
        ثابت std::وکتور<AstCoverCrossBin*> bins
            = dynamic ? generateRuntimeCrossBins(crossp, cxVarp, cpVars, dimensions)
                      : generateCrossBins(crossp, cxVarp, layout);
        إذا (v3Global.opt.coverage()) {
            ثابت std::string صفحه
                = VIdProtect::protectIf("v_covergroup/" + m_covergroupName, prot);
            m_constructorp->جمع_کن_Stmtsp(قلم_Call(fl, cxVarp, VCMethod::COVERGROUP_ثبات_BINS,
                                               {ctext(fl, "vlSymsp->_vm_contextp__->coveragep()"),
                                                ctext(fl, quoted(صفحه)),
                                                cnum(fl, قلم_Database_وزن(crossp->optionsp())),
                                                cnum(fl, m_cg_Type_وزن)})
                                          ->makeStmt());
        }

        // sample(): after all coverpoints have sampled (cross loop runs after coverpoint loop).
        UASSERT_OBJ(m_sampleFuncp, crossp, "sample() CFunc not set for cross");
        // The cross remembers its feeding coverpoints, so sample() needs no cps array;
        // per-bin iff guards still need a temporary array, hence the block form.
        ثابت منطقي hasIffs
            = std::any_از(bins.begin(), bins.انتها(),
                          [](ثابت AstCoverCrossBin* binp) { أرجع binp->iffp() != اشاره_تهی; });
        Ast_گره_Stmt* ثابت samplep
            = !hasIffs ? static_cast<Ast_گره_Stmt*>(
                             قلم_Call(fl, cxVarp, VCMethod::COVERGROUP_SAMPLE)->makeStmt())
                       : static_cast<Ast_گره_Stmt*>(makeCrossIffsCall(
                             fl, bins,
                             قلم_Call(fl, cxVarp, VCMethod::COVERGROUP_SAMPLE_IFFS,
                                      {ctext(fl, "__Vcx_iffs")})));
        إذا (Ast_گره_عبارت* ثابت iffp = crossp->iffp()) {
            m_sampleFuncp->جمع_کن_Stmtsp(جديد AstIf{fl, iffp->clone_درخت(خطأ), samplep});
        } وإلا {
            m_sampleFuncp->جمع_کن_Stmtsp(samplep);
        }
    }

    فراغ generate_Cross_کد(AstCoverCross* crossp) {
        UINFO(4, "  Generating code for cross: " << crossp->name());

        // Non-standard hierarchical/dotted cross item (e.g. 'cross a.b'): an implicit coverpoint
        // over the referenced expression (carried in refp->exprp()).  The grammar already warned
        // NONSTD; implicit coverpoints are not yet implemented, so generate no sampling code for
        // this cross.  When support is added the implicit coverpoint should be synthesized
        // upstream (V3LinkParse) as a real AstCoverpoint so it flows through the normal coverpoint
        // path - by here coverpoint lowering has already run.
        لكل (Ast_گره* itemp = crossp->itemsp(); itemp; itemp = itemp->nextp()) {
            ثابت AstCoverpointRef* ثابت refp = VN_AS(itemp, CoverpointRef);
            إذا (refp->exprp()) {
                refp->v3warn(COVERIGN,
                             "Unsupported: cross of hierarchical reference (implicit coverpoint)");
                أرجع;
            }
        }

        // A cross naming a bare variable (implicit coverpoint, which Verilator does not
        // synthesize) is dropped entirely with a COVERIGN warning -- it produces no coverage
        // either way -- but only this cross is dropped; its sibling crosses are still generated
        // and the real coverpoints it referenced remain as independent coverpoints.
        إذا (m_droppedCrosses.شمار(crossp)) {
            لكل (Ast_گره* itemp = crossp->itemsp(); itemp; itemp = itemp->nextp()) {
                ثابت AstCoverpointRef* ثابت refp = VN_AS(itemp, CoverpointRef);
                إذا (m_coverpointMap.پیدا_کن(refp->name()) == m_coverpointMap.انتها()) {
                    refp->v3warn(COVERIGN, "Unsupported: cross of "
                                               << refp->prettyNameQ()
                                               << " which is not a coverpoint (implicit "
                                                  "coverpoint)");
                    اكسر;
                }
            }
            أرجع;
        }

        // Every cross that isn't dropped routes through a VlCoverCross member.
        generateCross(crossp);
    }

    Ast_گره_عبارت* buildBinCondition(AstCoverBin* binp, Ast_گره_عبارت* exprp) {
        // Get the range list from the bin
        Ast_گره* ثابت rangep = binp->rangesp();
        إذا (!rangep) أرجع اشاره_تهی;

        // Integral values resolve to the coverpoint's type, as its runtime metadata does
        ثابت منطقي integral = exprp->dtypep()->skipRefp()->isIntegralOrPacked();
        // No value form of a wildcard bin is allowed on a real coverpoint (IEEE 1800-2023 19.5.4)
        إذا (binp->isWildcard() && !integral) أرجع wildcardTypeError(binp, exprp);

        // Build condition by OR-ing all ranges together
        Ast_گره_عبارت* پر_Condp = اشاره_تهی;

        لكل (Ast_گره* currRangep = rangep; currRangep; currRangep = currRangep->nextp()) {
            Ast_گره_عبارت* rangeCondp = اشاره_تهی;
            currRangep = V3Const::constifyEdit(currRangep);

            إذا (AstInsideRange* irp = VN_CAST(currRangep, InsideRange)) {
                Ast_گره_عبارت* ثابت minExprp = irp->lhsp();
                Ast_گره_عبارت* ثابت maxExprp = irp->rhsp();
                AstConst* ثابت minConstp = VN_CAST(minExprp, Const);
                AstConst* ثابت maxConstp = VN_CAST(maxExprp, Const);
                ثابت منطقي loUnbounded = VN_IS(minExprp, Unbounded);
                ثابت منطقي hiUnbounded = VN_IS(maxExprp, Unbounded);
                إذا (loUnbounded || hiUnbounded) {
                    // Open-ended range: '$' is the coverpoint domain min/max, so the
                    // range reduces to a single inequality (e.g. {[10:$]} -> expr >= 10).
                    AstConst* ثابت boundp = hiUnbounded ? minConstp : maxConstp;
                    إذا (loUnbounded && hiUnbounded) {
                        rangeCondp = جديد AstConst{irp->fileline(), AstConst::بیت_True{}};
                    } وإلا إذا (!boundp) {
                        irp->v3error("Non-constant expression in bin range; "
                                     "range bounds must be constants (IEEE 1800-2023 19.5)");
                        إذا (پر_Condp) VL_DO_DANGLING(افزودن_Deletep(پر_Condp), پر_Condp);
                        أرجع اشاره_تهی;
                    } وإلا إذا (boundp->رقم().isFourState()) {
                        irp->v3error("Four-state (x/z) value in bin range bound; "
                                     "range bounds must be two-state constants");
                        إذا (پر_Condp) VL_DO_DANGLING(افزودن_Deletep(پر_Condp), پر_Condp);
                        أرجع اشاره_تهی;
                    } وإلا إذا (integral) {
                        rangeCondp = build_مقدار_Condition(binp, exprp, irp);
                    } وإلا {
                        rangeCondp = makeOpenRangeCondition(irp->fileline(), exprp, boundp,
                                                            /*isLowerBound=*/hiUnbounded);
                    }
                } وإلا إذا (!minConstp || !maxConstp) {
                    irp->v3error("Non-constant expression in bin range; "
                                 "range bounds must be constants (IEEE 1800-2023 19.5)");
                    إذا (پر_Condp) VL_DO_DANGLING(افزودن_Deletep(پر_Condp), پر_Condp);
                    أرجع اشاره_تهی;
                } وإلا إذا (minConstp->رقم().isFourState() || maxConstp->رقم().isFourState()) {
                    irp->v3error("Four-state (x/z) value in bin range bound; "
                                 "range bounds must be two-state constants");
                    إذا (پر_Condp) VL_DO_DANGLING(افزودن_Deletep(پر_Condp), پر_Condp);
                    أرجع اشاره_تهی;
                } وإلا إذا (integral) {
                    rangeCondp = build_مقدار_Condition(binp, exprp, irp);
                } وإلا {
                    rangeCondp = makeRangeCondition(irp->fileline(), exprp, minExprp, maxExprp);
                }
            } وإلا إذا (AstConst* constp = VN_CAST(currRangep, Const)) {
                rangeCondp = build_مقدار_Condition(binp, exprp, constp);
            } وإلا {
                currRangep->v3error("Non-constant expression in bin range; values must be "
                                    "constants (IEEE 1800-2023 19.5)");
                إذا (پر_Condp) VL_DO_DANGLING(افزودن_Deletep(پر_Condp), پر_Condp);
                أرجع اشاره_تهی;
            }

            UASSERT_OBJ(rangeCondp, binp, "rangeCondp is null after building range condition");
            پر_Condp
                = پر_Condp ? جديد AstOr{binp->fileline(), پر_Condp, rangeCondp} : rangeCondp;
        }

        أرجع پر_Condp;
    }

    // Wildcard bits have no meaning for a non-integral coverpoint.
    ساكن Ast_گره_عبارت* wildcardTypeError(AstCoverBin* binp, Ast_گره_عبارت* exprp) {
        ثابت Ast_گره_DType* ثابت dtypep = exprp->dtypep()->skipRefp();
        exprp->v3error("Cannot use a wildcard bin on a coverpoint of type "
                       << dtypep->prettyDTypeNameQ() << " (IEEE 1800-2023 19.5.4).\n"
                       << exprp->warnContextPrimary() << '\n'
                       << binp->warnOther() << "... Location of wildcard bin\n"
                       << binp->warnContextSecondary());
        أرجع جديد AstConst{binp->fileline(), AstConst::بیت_False{}};
    }

    // Match one bin value, range, or wildcard pattern.  Integral coverpoints first resolve the
    // value to their type (IEEE 1800-2023 19.5.7).  Non-owning: clones what it uses.
    Ast_گره_عبارت* build_مقدار_Condition(AstCoverBin* binp, Ast_گره_عبارت* exprp, Ast_گره* valuep) {
        File_سطر* ثابت fl = valuep->fileline();
        إذا (!exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
            أرجع جديد AstEq{fl, exprp->clone_درخت(خطأ),
                             VN_AS(valuep, گره_عبارت)->clone_درخت(خطأ)};
        }
        Cross_مقدار_Range range{valuep, resolve_عرض(valuep, exprp)};
        إذا (!resolve_مقدار(valuep, exprp, درست, binp->isWildcard(), range)) {
            valuep->v3warn(E_UNSUPPORTED,
                           "Unsupported: non-integral value in a coverage bin of an "
                           "integral coverpoint.");
            أرجع جديد AstConst{fl, AstConst::بیت_False{}};
        }
        إذا (cross_Range_خالی(range)) أرجع جديد AstConst{fl, AstConst::بیت_False{}};
        AstConst* ثابت lop = new_مقدار_Const(fl, range.lo, exprp);
        AstConst* ثابت hip = new_مقدار_Const(fl, range.hi, exprp);
        Ast_گره_عبارت* condp = اشاره_تهی;
        إذا (lop->رقم().isCaseEq(hip->رقم())) {
            condp = جديد AstEq{fl, exprp->clone_درخت(خطأ), lop};
        } وإلا {
            condp = makeRangeCondition(fl, exprp, lop, hip);
            VL_DO_DANGLING(افزودن_Deletep(lop), lop);
        }
        VL_DO_DANGLING(افزودن_Deletep(hip), hip);
        إذا (!range.wildcard) أرجع condp;
        // Match the pattern's value bits within the source-value bounds.
        V3_عدد ماسک{valuep, exprp->عرض()};
        V3_عدد مقدار{valuep, exprp->عرض()};
        ماسک.op_بیتها_Non_XZ(range.الگو_2);
        مقدار.op_بیتها_One(range.الگو_2);
        AstConst* ثابت ماسک_Constp = new_مقدار_Const(fl, ماسک, exprp);
        AstConst* ثابت مقدار_Constp = new_مقدار_Const(fl, مقدار, exprp);
        Ast_گره_عبارت* ثابت عبارت_Masked = جديد AstAnd{fl, exprp->clone_درخت(خطأ), ماسک_Constp};
        Ast_گره_عبارت* ثابت مقدار_Masked = جديد AstAnd{fl, مقدار_Constp, ماسک_Constp->clone_درخت(خطأ)};
        أرجع جديد AstLogAnd{fl, condp, جديد AstEq{fl, عبارت_Masked, مقدار_Masked}};
    }

    فراغ generate_Coverage_Computation_کد() {
        UINFO(4, "  Generating coverage computation code");

        // Invalidate cache: addMembersp() calls in generateCoverpointCode/generateCrossCode
        // have added new members since the last scan, so clear before re-querying.
        m_memberMap.clear();

        // get_inst_coverage(): the average of the coverpoints and crosses, weighted by their
        // option.weight (IEEE 1800-2023 19.11).  The instance node holds their runtimes.  With
        // the instances merged, get_coverage() instead, unless option.get_inst_coverage
        // (Table 19-1).
        AstFunc* ثابت getInstCoveragep
            = VN_AS(m_memberMap.پیدا_کن_Member(m_covergroupp, "get_inst_coverage"), Func);
        File_سطر* ثابت instFl = getInstCoveragep->fileline();
        AstCMethodHard* ثابت instCallp = instanceCall(instFl, VCMethod::COVERGROUP_COVERAGE);
        instCallp->dtype_مجموعه_Double();
        Ast_گره_عبارت* instValuep = instCallp;
        إذا (m_cg_May_ادغام) {
            Ast_گره_عبارت* ثابت mergedp = جديد AstLogAnd{
                instFl, newOptionSel(instFl, optionVar(درست), "merge_instances", VAccess::READ),
                جديد AstLogNot{instFl, newOptionSel(instFl, optionVar(خطأ), "get_inst_coverage",
                                                   VAccess::READ)}};
            instValuep = جديد AstCond{instFl, mergedp, typeCoverageCall(instFl), instCallp};
        }
        getInstCoveragep->جمع_کن_Stmtsp(جديد Ast_تخصیص_بده{
            instFl, جديد AstVarRef{instFl, VN_AS(getInstCoveragep->fvarp(), Var), VAccess::WRITE},
            instValuep});

        // get_coverage(): the average of the covergroup's instances, weighted by their
        // option.weight, or with type_option.merge_instances, the coverage of the union of their
        // bins (IEEE 1800-2023 19.11.3).  Static, so the registry finds the instances.
        AstFunc* ثابت getCoveragep
            = VN_AS(m_memberMap.پیدا_کن_Member(m_covergroupp, "get_coverage"), Func);
        File_سطر* ثابت typeFl = getCoveragep->fileline();
        getCoveragep->جمع_کن_Stmtsp(جديد Ast_تخصیص_بده{
            typeFl, جديد AstVarRef{typeFl, VN_AS(getCoveragep->fvarp(), Var), VAccess::WRITE},
            typeCoverageCall(typeFl)});
    }

    // The registry call computing the covergroup's type coverage, per its type options
    AstCMethodHard* typeCoverageCall(File_سطر* fl) {
        AstCExpr* ثابت registryp = ctext(fl, "vlSymsp->_vm_contextp__->covergroupRegistryp()");
        registryp->dtype_مجموعه_Void();  // Opaque receiver; only ever the 'fromp' of the call below
        AstCMethodHard* ثابت callp
            = جديد AstCMethodHard{fl, registryp, VCMethod::COVERGROUP_TYPE_COVERAGE};
        callp->جمع_کن_Pinsp(ctext(fl, quoted(covergroupProtectedName())));
        callp->جمع_کن_Pinsp(newOptionSel(fl, optionVar(درست), "weight", VAccess::READ));
        callp->جمع_کن_Pinsp(newOptionSel(fl, optionVar(درست), "merge_instances", VAccess::READ));
        callp->جمع_کن_Pinsp(file_سطر_Debug(m_covergroupp->fileline()));
        callp->usePtr(درست);
        callp->dtype_مجموعه_Double();
        أرجع callp;
    }

    // VISITORS
    ساكن منطقي isEnclosingInstanceVar(ثابت AstVar* varp) {
        أرجع varp->isClassMember() && !varp->lifetime().isStatic() && !varp->isParam();
    }

    فراغ rewriteThisRef(AstThisRef* refp, AstVar* handleVarp) {
        ثابت AstClassRefDType* ثابت refDTypep
            = VN_CAST(refp->dtypep()->skipRefp(), ClassRefDType);
        UASSERT_OBJ(refDTypep && refDTypep->classp() == m_covergroupp, refp,
                    "Unexpected this reference in embedded covergroup");
        Ast_گره_عبارت* ثابت newp = جديد AstVarRef{refp->fileline(), handleVarp, VAccess::READ};
        refp->replaceWith(newp);
        VL_DO_DANGLING(افزودن_Deletep(refp), refp);
    }

    فراغ rewriteVarRef(AstVarRef* refp, AstVar* handleVarp) {
        File_سطر* ثابت fl = refp->fileline();
        // As V3LinkLValue does, the handle of a written member select is marked written
        AstMemberSel* ثابت selp
            = جديد AstMemberSel{fl, جديد AstVarRef{fl, handleVarp, refp->access()}, refp->varp()};
        selp->access(refp->access());
        refp->replaceWith(selp);
        VL_DO_DANGLING(افزودن_Deletep(refp), refp);
    }

    فراغ rewriteFuncRef(AstFuncRef* refp, AstVar* handleVarp) {
        File_سطر* ثابت fl = refp->fileline();
        AstArg* ثابت argsp = refp->argsp() ? refp->argsp()->unlink_Fr_پشت_With_بعدی() : اشاره_تهی;
        Ast_روش_Call* ثابت callp = جديد Ast_روش_Call{
            fl, جديد AstVarRef{fl, handleVarp, VAccess::READ}, refp->name(), argsp};
        callp->taskp(refp->taskp());
        callp->dtypeFrom(refp);
        refp->replaceWith(callp);
        VL_DO_DANGLING(افزودن_Deletep(refp), refp);
    }

    // True if funcp is an instance method of the enclosing class or one of its bases
    منطقي isEnclosingInstanceFunc(ثابت Ast_گره_FTask* funcp) ثابت {
        إذا (!funcp->class_روش() || funcp->isStatic()) أرجع خطأ;
        أرجع AstClass::isClassExtendedFrom(m_enclosingClassp, VN_AS(funcp->aboveLoopp(), Class));
    }

    منطقي isEmbeddedCovergroupVar(ثابت AstVar* varp) ثابت {
        إذا (!varp || !varp->isClassMember() || varp->isDeclTyped()) أرجع خطأ;
        ثابت AstClassRefDType* ثابت refp = VN_CAST(varp->dtypep()->skipRefp(), ClassRefDType);
        أرجع refp && refp->classp() == m_covergroupp;
    }

    AstVar* پیدا_کن_Embedded_Covergroup_Var() ثابت {
        إذا (!m_enclosingClassp) أرجع اشاره_تهی;
        لكل (Ast_گره* itemp = m_enclosingClassp->membersp(); itemp; itemp = itemp->nextp()) {
            إذا (AstVar* ثابت varp = VN_CAST(itemp, Var)) {
                إذا (isEmbeddedCovergroupVar(varp)) أرجع varp;
            }
        }
        // V3LinkParse always creates an implicit variable for an embedded covergroup.
        أرجع اشاره_تهی;  // LCOV_EXCL_LINE
    }

    std::وکتور<Ast_گره_تخصیص_بده*> پیدا_کن_Covergroup_Constructions() {
        std::وکتور<Ast_گره_تخصیص_بده*> foundps;
        إذا (!m_embeddedVarp) أرجع foundps;
        AstFunc* ثابت enclosingNewp
            = VN_CAST(m_memberMap.پیدا_کن_Member(m_enclosingClassp, "new"), Func);
        إذا (!enclosingNewp) أرجع foundps;
        enclosingNewp->foreach([&](Ast_گره_تخصیص_بده* asgnp) {
            ثابت AstNew* ثابت newp = VN_CAST(asgnp->rhsp(), New);
            ثابت AstVarRef* ثابت lhsRefp = VN_CAST(asgnp->lhsp(), VarRef);
            إذا (!newp || !lhsRefp || lhsRefp->varp() != m_embeddedVarp) أرجع;
            ثابت AstClassRefDType* ثابت refp = VN_CAST(newp->dtypep(), ClassRefDType);
            إذا (refp && refp->classp() == m_covergroupp) foundps.افزودن_پشت(asgnp);
        });
        أرجع foundps;
    }

    std::مجموعه<ثابت AstVar*> enclosingInstanceVars() ثابت {
        std::مجموعه<ثابت AstVar*> vars;
        إذا (m_enclosingClassp) {
            m_enclosingClassp->foreachMember([&](AstClass* ثابت, AstVar* ثابت varp) {
                إذا (isEnclosingInstanceVar(varp)) vars.درج(varp);
            });
        }
        أرجع vars;
    }

    منطقي hasEnclosingEventRef(AstCovergroup* cgp) ثابت {
        إذا (!m_embeddedVarp || !cgp->eventp()) أرجع خطأ;
        ثابت std::مجموعه<ثابت AstVar*> enclosingVars = enclosingInstanceVars();
        منطقي یافت = خطأ;
        cgp->eventp()->foreach([&](AstVarRef* refp) {
            إذا (enclosingVars.شمار(refp->varp())) یافت = درست;
        });
        أرجع یافت;
    }

    ساكن منطقي parse_Embedded_Event_عبارت(Ast_گره_عبارت* exprp, AstVar*& پایه_Varp,
                                       AstVar*& memberVarp) {
        إذا (AstVarRef* ثابت refp = VN_CAST(exprp, VarRef)) {
            پایه_Varp = refp->varp();
            memberVarp = اشاره_تهی;
            أرجع درست;
        }
        AstMemberSel* ثابت selp = VN_CAST(exprp, MemberSel);
        إذا (!selp) أرجع خطأ;
        AstVarRef* ثابت پایه_Refp = VN_CAST(selp->fromp(), VarRef);
        إذا (!پایه_Refp) أرجع خطأ;
        پایه_Varp = پایه_Refp->varp();
        memberVarp = selp->varp();
        أرجع درست;
    }

    منطقي isEventLvalue(Ast_گره_عبارت* exprp, ثابت EmbeddedEventTrigger& trigger) ثابت {
        إذا (AstSel* ثابت selp = VN_CAST(exprp, Sel)) exprp = selp->fromp();
        AstVar* پایه_Varp = اشاره_تهی;
        AstVar* memberVarp = اشاره_تهی;
        إذا (!parse_Embedded_Event_عبارت(exprp, پایه_Varp, memberVarp)) أرجع خطأ;
        أرجع پایه_Varp == trigger.پایه_Varp && memberVarp == trigger.memberVarp;
    }

    Ast_گره_عبارت* newEventRead(File_سطر* fl, ثابت EmbeddedEventTrigger& trigger) ثابت {
        Ast_گره_عبارت* ثابت basep = جديد AstVarRef{fl, trigger.پایه_Varp, VAccess::READ};
        إذا (!trigger.memberVarp) أرجع basep;
        AstMemberSel* ثابت selp = جديد AstMemberSel{fl, basep, trigger.memberVarp};
        selp->access(VAccess::READ);
        أرجع selp;
    }

    string eventPrevName(ثابت EmbeddedEventTrigger& trigger, اندازه_t trigger_شاخص) ثابت {
        string name = "__Vcg_prev_" + m_embeddedVarp->name() + "_" + std::مقصد_string(trigger_شاخص)
                      + "_" + trigger.پایه_Varp->name();
        إذا (trigger.memberVarp) name += "_" + trigger.memberVarp->name();
        أرجع name;
    }

    Ast_گره_عبارت* newEmbeddedVarNonNull(File_سطر* fl) ثابت {
        أرجع جديد AstNeq{fl, جديد AstVarRef{fl, m_embeddedVarp, VAccess::READ},
                          جديد AstConst{fl, AstConst::Null{}}};
    }

    Ast_گره_Stmt* newSampleStmt(File_سطر* fl) ثابت {
        Ast_روش_Call* ثابت callp = جديد Ast_روش_Call{
            fl, جديد AstVarRef{fl, m_embeddedVarp, VAccess::READ}, "sample", اشاره_تهی};
        callp->taskp(m_sampleFuncp);
        callp->dtype_مجموعه_Void();
        أرجع callp->makeStmt();
    }

    فراغ installEmbeddedEventFork(Ast_Sen_درخت* eventp,
                                  ثابت std::وکتور<Ast_گره_تخصیص_بده*>& constructps) {
        // IEEE 1800-2023 19.3 samples coverpoints whenever their clocking event occurs. A
        // per-instance event cannot use V3Active's static sensitivity path, so spawn
        // 'fork forever begin @(event); cg.sample(); end join_none' after each construction.
        لكل (Ast_گره_تخصیص_بده* ثابت constructp : constructps) {
            File_سطر* ثابت fl = constructp->fileline();
            AstLoop* ثابت loopp = جديد AstLoop{fl};
            loopp->جمع_کن_Stmtsp(جديد AstEventControl{fl, eventp->clone_درخت(خطأ), اشاره_تهی});
            loopp->جمع_کن_Stmtsp(جديد AstIf{fl, newEmbeddedVarNonNull(fl), newSampleStmt(fl)});
            AstFork* ثابت forkp = جديد AstFork{fl, VJoinType::JOIN_NONE};
            forkp->فوری_شروع(درست);
            forkp->جمع_کن_Forksp(جديد AstBegin{fl, "", loopp, درست});
            constructp->جمع_کن_بعدی_اینجا(forkp);
        }
        VL_DO_DANGLING(افزودن_Deletep(eventp), eventp);
    }

    Ast_گره_عبارت* newEventReadyCondition(File_سطر* fl, ثابت EmbeddedEventTrigger& trigger) ثابت {
        Ast_گره_عبارت* ثابت curp = newEventRead(fl, trigger);
        Ast_گره_عبارت* ثابت prevp = جديد AstVarRef{fl, trigger.prevVarp, VAccess::READ};
        Ast_گره_عبارت* edgep = اشاره_تهی;
        // IEEE 1800-2023 9.4.2 detects edge-qualified events only on the expression's LSB,
        // while an implicit change event observes the complete expression.
        إذا (trigger.یال_Type == VEdgeType::ET_POSEDGE) {
            edgep = جديد AstSel{fl, جديد AstAnd{fl, curp, جديد AstNot{fl, prevp}}, 0, 1};
        } وإلا إذا (trigger.یال_Type == VEdgeType::ET_NEGEDGE) {
            edgep = جديد AstSel{fl, جديد AstAnd{fl, جديد AstNot{fl, curp}, prevp}, 0, 1};
        } وإلا إذا (trigger.یال_Type == VEdgeType::ET_BOTHEDGE) {
            edgep = جديد AstSel{fl, جديد AstXor{fl, curp, prevp}, 0, 1};
        } وإلا {
            edgep = جديد AstNeq{fl, curp, prevp};
        }
        أرجع جديد AstLogAnd{fl, newEmbeddedVarNonNull(fl), edgep};
    }

    std::وکتور<EmbeddedEventTrigger> collectEmbeddedEventTriggers(AstCovergroup* cgp) {
        std::وکتور<EmbeddedEventTrigger> triggers;
        ثابت std::مجموعه<ثابت AstVar*> enclosingVars = enclosingInstanceVars();
        لكل (Ast_گره* senp = cgp->eventp()->sensesp(); senp; senp = senp->nextp()) {
            Ast_Sen_قلم* ثابت itemp = VN_AS(senp, Sen_قلم);
            AstVar* پایه_Varp = اشاره_تهی;
            AstVar* memberVarp = اشاره_تهی;
            إذا (!parse_Embedded_Event_عبارت(itemp->sensp(), پایه_Varp, memberVarp)
                || !enclosingVars.شمار(پایه_Varp)) {
                أرجع {};
            }
            triggers.emplace_پشت(itemp->fileline(), پایه_Varp, memberVarp, itemp->یال_Type());
        }
        أرجع triggers;
    }

    فراغ installEmbeddedEventTriggers(std::وکتور<EmbeddedEventTrigger>& triggers,
                                      ثابت std::وکتور<Ast_گره_تخصیص_بده*>& constructps) {
        // Without --timing, approximate a per-instance event by sampling after assignments
        // within the enclosing class. External writes and exact scheduling cannot be observed.
        إذا (constructps.خالی()) أرجع;
        لكل (اندازه_t trigger_شاخص = 0; trigger_شاخص < triggers.اندازه(); ++trigger_شاخص) {
            EmbeddedEventTrigger& trigger = triggers[trigger_شاخص];
            std::وکتور<Ast_گره_تخصیص_بده*> assignps;
            m_enclosingClassp->foreach([&](Ast_گره_تخصیص_بده* asgnp) {
                إذا (isEventLvalue(asgnp->lhsp(), trigger)) assignps.افزودن_پشت(asgnp);
            });
            إذا (assignps.خالی()) {
                trigger.eventFl->v3warn(
                    COVERIGN, "Unsupported: 'covergroup' clocking event signal has no assignment "
                              "within the enclosing class; no coverage sampled. Use --timing for "
                              "full support.");
                استمر;
            }
            Ast_گره_DType* ثابت dtypep
                = trigger.memberVarp ? trigger.memberVarp->dtypep() : trigger.پایه_Varp->dtypep();
            AstVar* ثابت prevVarp = جديد AstVar{trigger.eventFl, VVarType::MEMBER,
                                                eventPrevName(trigger, trigger_شاخص), dtypep};
            m_enclosingClassp->جمع_کن_Membersp(prevVarp);
            trigger.prevVarp = prevVarp;
            لكل (Ast_گره_تخصیص_بده* ثابت asgnp : assignps) {
                File_سطر* ثابت fl = asgnp->fileline();
                AstIf* ثابت ifp
                    = جديد AstIf{fl, newEventReadyCondition(fl, trigger), newSampleStmt(fl)};
                ifp->جمع_کن_بعدی_اینجا(جديد Ast_تخصیص_بده{fl,
                                               جديد AstVarRef{fl, trigger.prevVarp, VAccess::WRITE},
                                               newEventRead(fl, trigger)});
                asgnp->جمع_کن_بعدی_اینجا(ifp);
            }
        }
    }

    فراغ delete_Coverage_اقلام() {
        لكل (AstCoverpoint* ثابت cpp : m_coverpoints) {
            VL_DO_DANGLING(افزودن_Deletep(cpp->unlink_Fr_پشت()), cpp);
        }
        لكل (AstCoverCross* ثابت crossp : m_coverCrosses) {
            VL_DO_DANGLING(افزودن_Deletep(crossp->unlink_Fr_پشت()), crossp);
        }
        // Options not lowered: the covergroup was not processed
        لكل (Ast_Cg_Option_تخصیص_بده* ثابت optp : m_cgOptions) {
            VL_DO_DANGLING(افزودن_Deletep(optp->unlink_Fr_پشت()), optp);
        }
        m_cgOptions.clear();
    }

    صنف FormalRefVisitor final : عام VNVisitor {
        ثابت std::map<ثابت AstVar*, AstVar*>& m_replacements;

        فراغ بازدید(AstVarRef* nodep) override {
            ثابت خودکار it = m_replacements.پیدا_کن(nodep->varp());
            إذا (it == m_replacements.انتها()) أرجع;
            nodep->varp(it->دومی);
        }
        فراغ بازدید(Ast_گره* nodep) override { iterateChildren(nodep); }

    عام:
        صریح FormalRefVisitor(ثابت std::map<ثابت AstVar*, AstVar*>& replacements)
            : m_replacements{replacements} {}
        فراغ پیمایش(Ast_گره* nodep) { iterate(nodep); }
    };

    فراغ validateCovergroupExpressions() {
        std::مجموعه<ثابت AstVar*> sampleMembers;
        std::مجموعه<ثابت AstVar*> constructorRefMembers;
        لكل (Ast_گره* stmtp = m_constructorp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ثابت AstVar* ثابت varp = VN_CAST(stmtp, Var);
            إذا (!varp || !varp->isIO() || (!varp->isRef() && !varp->isConstRef())) استمر;
            ثابت AstVar* ثابت memberp
                = VN_CAST(m_memberMap.پیدا_کن_Member(m_covergroupp, varp->name()), Var);
            UASSERT_OBJ(memberp && memberp->isClassMember(), varp,
                        "Covergroup constructor argument missing persistent member");
            constructorRefMembers.درج(varp);
            constructorRefMembers.درج(memberp);
        }
        لكل (Ast_گره* stmtp = m_sampleFuncp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ثابت AstVar* ثابت varp = VN_CAST(stmtp, Var);
            إذا (!varp || !varp->isIO()) استمر;
            ثابت AstVar* ثابت memberp
                = VN_CAST(m_memberMap.پیدا_کن_Member(m_covergroupp, varp->name()), Var);
            UASSERT_OBJ(memberp && memberp->isClassMember(), varp,
                        "Covergroup sample argument missing persistent member");
            sampleMembers.درج(memberp);
        }
        Covergroup_عبارت_Valid_Visitor{sampleMembers, constructorRefMembers}.پیمایش(m_constructorp);
    }

    فراغ rebindFormalRefs() {
        std::map<ثابت AstVar*, AstVar*> replacements;
        لكل (Ast_گره* stmtp = m_constructorp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            إذا (ثابت AstVar* ثابت varp = VN_CAST(stmtp, Var)) {
                إذا (!varp->isIO()) استمر;
                AstVar* ثابت memberp
                    = VN_CAST(m_memberMap.پیدا_کن_Member(m_covergroupp, varp->name()), Var);
                UASSERT_OBJ(memberp && memberp->isClassMember(), varp,
                            "Covergroup constructor argument missing persistent member");
                replacements.emplace(varp, memberp);
            }
        }
        اندازه_t expectedBindings = 0;
        لكل (ثابت خودکار& pair : replacements) {
            إذا (pair.اولی->isRef() || pair.اولی->isConstRef()) ++expectedBindings;
        }
        اندازه_t rewrittenBindings = 0;
        لكل (Ast_گره* stmtp = m_constructorp->stmtsp(); stmtp;) {
            Ast_گره* ثابت nextp = stmtp->nextp();
            إذا (Ast_تخصیص_بده* ثابت assignp = VN_CAST(stmtp, تخصیص_بده)) {
                AstVarRef* ثابت lhsp = VN_CAST(assignp->lhsp(), VarRef);
                AstVarRef* ثابت rhsp = VN_CAST(assignp->rhsp(), VarRef);
                إذا (lhsp && rhsp
                    && (lhsp->varp()->declDirection() == VDirection::REF
                        || lhsp->varp()->declDirection() == VDirection::CONSTREF)) {
                    ثابت خودکار it = replacements.پیدا_کن(rhsp->varp());
                    UASSERT_OBJ(it != replacements.انتها() && it->دومی == lhsp->varp(), assignp,
                                "Unexpected covergroup reference binding assignment");
                    AstCExpr* ثابت bindp = جديد AstCExpr{assignp->fileline(), ""};
                    bindp->جمع_کن(lhsp->unlink_Fr_پشت());
                    bindp->جمع_کن(" = &");
                    bindp->جمع_کن(rhsp->unlink_Fr_پشت());
                    assignp->replaceWith(bindp->makeStmt());
                    VL_DO_DANGLING(افزودن_Deletep(assignp), assignp);
                    ++rewrittenBindings;
                }
            }
            stmtp = nextp;
        }
        UASSERT_OBJ(rewrittenBindings == expectedBindings, m_constructorp,
                    "Covergroup reference argument missing binding");
        FormalRefVisitor visitor{replacements};
        لكل (AstCoverpoint* ثابت cpp : m_coverpoints) visitor.پیمایش(cpp);
        لكل (AstCoverCross* ثابت crossp : m_coverCrosses) visitor.پیمایش(crossp);
    }

    AstVarRef* install_Enclosing_پشت_اشاره_گر(ثابت std::وکتور<Ast_گره_تخصیص_بده*>& constructps) {
        // Simple-case support for embedded covergroups (IEEE 1800-2023 19.4) whose
        // coverpoints reference members of the enclosing class ("Class members can be used
        // in coverpoint expressions").  The covergroup is lowered into a sibling class with
        // no implicit handle to the enclosing object, so such references would emit
        // uncompilable C++.  Add an explicit back-pointer member to the enclosing instance,
        // route member references through it, and pass it into the constructor so
        // coverage initialization can read enclosing members. Returns an invalid
        // reference if an outer class member cannot be reached; otherwise returns an empty result.
        إذا (!m_enclosingClassp) أرجع اشاره_تهی;  // Offending refs require an enclosing class

        AstVarRef* invalidp = اشاره_تهی;
        Ast_گره* offenderp = اشاره_تهی;
        std::مجموعه<ثابت AstVar*> ownVars;
        لكل (Ast_گره* itemp = m_covergroupp->membersp(); itemp; itemp = itemp->nextp()) {
            إذا (ثابت AstVar* ثابت varp = VN_CAST(itemp, Var)) ownVars.درج(varp);
        }
        ثابت std::مجموعه<ثابت AstVar*> enclosingVars = enclosingInstanceVars();
        std::وکتور<AstVarRef*> refs_مقصد_Rewrite;
        std::وکتور<AstThisRef*> this_Refs_مقصد_Rewrite;
        std::وکتور<AstFuncRef*> func_Refs_مقصد_Rewrite;
        ثابت خودکار پیمایش = [&](Ast_گره* rootp) {
            rootp->foreach([&](AstVarRef* refp) {
                إذا (invalidp) أرجع;
                ثابت AstVar* ثابت varp = refp->varp();
                إذا (!isEnclosingInstanceVar(varp) || ownVars.شمار(varp)) أرجع;
                إذا (!enclosingVars.شمار(varp)) {
                    invalidp = refp;
                    أرجع;
                }
                refs_مقصد_Rewrite.افزودن_پشت(refp);
                إذا (!offenderp) offenderp = refp;
            });
            إذا (invalidp) أرجع;
            rootp->foreach([&](AstThisRef* refp) {
                ثابت AstClassRefDType* ثابت refDTypep
                    = VN_CAST(refp->dtypep()->skipRefp(), ClassRefDType);
                إذا (refDTypep && refDTypep->classp() == m_covergroupp) {
                    this_Refs_مقصد_Rewrite.افزودن_پشت(refp);
                    إذا (!offenderp) offenderp = refp;
                }
            });
            rootp->foreach([&](AstFuncRef* refp) {
                إذا (!isEnclosingInstanceFunc(refp->taskp())) أرجع;
                func_Refs_مقصد_Rewrite.افزودن_پشت(refp);
                إذا (!offenderp) offenderp = refp;
            });
        };
        لكل (AstCoverpoint* ثابت cpp : m_coverpoints) پیمایش(cpp);
        لكل (AstCoverCross* ثابت crossp : m_coverCrosses) پیمایش(crossp);
        لكل (Ast_Cg_Option_تخصیص_بده* ثابت optp : m_cgOptions) پیمایش(optp);
        إذا (invalidp || !offenderp) أرجع invalidp;

        UASSERT_OBJ(m_embeddedVarp, m_covergroupp, "Embedded covergroup variable not found");
        // Commit: add the back-pointer member, rewrite the references, initialize the handle.
        File_سطر* ثابت fl = m_covergroupp->fileline();
        AstClassRefDType* ثابت enclDTypep = جديد AstClassRefDType{fl, m_enclosingClassp, اشاره_تهی};
        enclDTypep->raw_اشاره_گر(درست);
        v3Global.rootp()->typeTablep()->جمع_کن_Typesp(enclDTypep);
        AstVar* ثابت handleVarp
            = جديد AstVar{fl, VVarType::MEMBER, "__Vcg_enclosingp", enclDTypep};
        m_covergroupp->جمع_کن_Membersp(handleVarp);
        AstVar* ثابت argumentp = جديد AstVar{fl, VVarType::BLOCKTEMP, "__Vcg_parentp", enclDTypep};
        argumentp->direction(VDirection::درون_داد);
        argumentp->declDirection(VDirection::درون_داد);
        argumentp->funcLocal(درست);
        argumentp->noReset(درست);
        argumentp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        m_constructorp->جمع_کن_Stmtsp(argumentp);
        m_constructorp->prependStmtsp(جديد Ast_تخصیص_بده{fl, memberRef(fl, handleVarp, VAccess::WRITE),
                                                    جديد AstVarRef{fl, argumentp, VAccess::READ}});

        // Route each enclosing-member reference through the back-pointer: 'm' -> 'h.m'.
        لكل (AstVarRef* ثابت refp : refs_مقصد_Rewrite) { rewriteVarRef(refp, handleVarp); }
        لكل (AstThisRef* ثابت refp : this_Refs_مقصد_Rewrite) { rewriteThisRef(refp, handleVarp); }
        لكل (AstFuncRef* ثابت refp : func_Refs_مقصد_Rewrite) { rewriteFuncRef(refp, handleVarp); }

        // Append a named hidden argument to preserve positional and defaulted user arguments.
        لكل (Ast_گره_تخصیص_بده* ثابت constructp : constructps) {
            File_سطر* ثابت cfl = constructp->fileline();
            AstCExpr* ثابت thisp = جديد AstCExpr{cfl, "this"};
            thisp->dtypep(enclDTypep);
            VN_AS(constructp->rhsp(), New)->جمع_کن_Argsp(جديد AstArg{cfl, argumentp->name(), thisp});
        }
        أرجع اشاره_تهی;
    }

    فراغ بازدید(AstClass* nodep) override {
        UINFO(9, "Visiting class: " << nodep->name() << " isCovergroup=" << nodep->isCovergroup());
        إذا (nodep->isCovergroup()) {
            VL_RESTORER(m_covergroupp);
            VL_RESTORER(m_embeddedVarp);
            VL_RESTORER(m_sampleFuncp);
            VL_RESTORER(m_constructorp);
            VL_RESTORER_CLEAR(m_coverpoints);
            VL_RESTORER_CLEAR(m_coverpointMap);
            VL_RESTORER_CLEAR(m_coverCrosses);
            VL_RESTORER_CLEAR(m_cgOptions);
            VL_RESTORER_CLEAR(m_covergroupName);
            m_covergroupp = nodep;
            m_embeddedVarp = پیدا_کن_Embedded_Covergroup_Var();
            m_covergroupName = covergroupTypeName();
            m_sampleFuncp = اشاره_تهی;
            m_constructorp = اشاره_تهی;
            std::وکتور<EmbeddedEventTrigger> embeddedEventTriggers;
            Ast_Sen_درخت* embeddedEventForkp = اشاره_تهی;

            // Extract and store the clocking event from AstCovergroup node
            // The parser creates this node to preserve the event information
            منطقي hasUnsupportedEvent = خطأ;
            لكل (Ast_گره* itemp = nodep->membersp(); itemp;) {
                Ast_گره* ثابت nextp = itemp->nextp();
                إذا (AstCovergroup* ثابت cgp = VN_CAST(itemp, Covergroup)) {
                    // Store the event in the global map for V3Active to retrieve later
                    // V3LinkParse only creates this sentinel AstCovergroup node when a clocking
                    // event exists, so cgp->eventp() is always non-null here.
                    UASSERT_OBJ(cgp->eventp(), cgp,
                                "Sentinel AstCovergroup in class must have non-null eventp");
                    إذا (hasEnclosingEventRef(cgp)) {
                        UASSERT_OBJ(m_embeddedVarp, cgp,
                                    "Embedded covergroup event has no instance variable");
                        إذا (v3Global.opt.timing().is_مجموعه_True()) {
                            embeddedEventForkp = cgp->eventp()->unlink_Fr_پشت();
                        } وإلا {
                            embeddedEventTriggers = collectEmbeddedEventTriggers(cgp);
                            إذا (embeddedEventTriggers.خالی()) {
                                cgp->v3warn(COVERIGN,
                                            "Unsupported: 'covergroup' clocking event on complex "
                                            "member expression; use --timing for full support.");
                                hasUnsupportedEvent = درست;
                            }
                        }
                        VL_DO_DANGLING(افزودن_Deletep(cgp->unlink_Fr_پشت()), cgp);
                        itemp = nextp;
                        استمر;
                    }
                    // V3Active handles events that do not depend on an enclosing instance.
                    UINFO(4, "Keeping covergroup event node for V3Active: " << nodep->name());
                    itemp = nextp;
                    استمر;
                }
                itemp = nextp;
            }

            // Find the sample() method and constructor
            m_sampleFuncp = VN_CAST(m_memberMap.پیدا_کن_Member(nodep, "sample"), Func);
            // V3LinkParse always synthesizes a sample() method for every covergroup, and the
            // sampling-code generation below dereferences m_sampleFuncp unconditionally.
            UASSERT_OBJ(m_sampleFuncp, nodep, "Covergroup missing synthesized sample() method");
            m_sampleFuncp->isCovergroupSample(درست);
            m_constructorp = VN_CAST(m_memberMap.پیدا_کن_Member(nodep, "new"), Func);
            UINFO(9, "Found sample() method: " << (m_sampleFuncp ? "yes" : "no"));
            UINFO(9, "Found constructor: " << (m_constructorp ? "yes" : "no"));

            // If covergroup has unsupported clocking event, skip processing it
            // but still clean up coverpoints so they don't reach downstream passes
            إذا (hasUnsupportedEvent) {
                iterateChildren(nodep);
                validateCovergroupExpressions();
                rebindFormalRefs();
                delete_Coverage_اقلام();
                إذا (embeddedEventForkp) {
                    VL_DO_DANGLING(افزودن_Deletep(embeddedEventForkp), embeddedEventForkp);
                }
                أرجع;
            }

            iterateChildren(nodep);
            validateCovergroupExpressions();
            rebindFormalRefs();
            ثابت std::وکتور<Ast_گره_تخصیص_بده*> constructps = پیدا_کن_Covergroup_Constructions();

            // Embedded covergroups (IEEE 1800-2023 19.4): coverpoints, iff expressions, and
            // crosses may reference members of the enclosing class. The covergroup is lowered
            // into a sibling class with no implicit handle to the enclosing instance. Install
            // an explicit back-pointer and route the references through it.
            إذا (AstVarRef* ثابت invalidp = install_Enclosing_پشت_اشاره_گر(constructps)) {
                invalidp->v3error("Non-static member "
                                  << invalidp->varp()->prettyNameQ()
                                  << " of an outer class requires an explicit "
                                     "object handle (IEEE 1800-2023 8.23).");
                delete_Coverage_اقلام();
                إذا (embeddedEventForkp) {
                    VL_DO_DANGLING(افزودن_Deletep(embeddedEventForkp), embeddedEventForkp);
                }
                أرجع;
            }
            installEmbeddedEventTriggers(embeddedEventTriggers, constructps);
            إذا (embeddedEventForkp) installEmbeddedEventFork(embeddedEventForkp, constructps);
            processCovergroup();
            // Remove lowered coverpoints/crosses from the class - they have been
            // fully translated into C++ code and must not reach downstream passes
            delete_Coverage_اقلام();
        } وإلا {
            // Track the lexically enclosing class so a nested covergroup can resolve
            // references to the enclosing object's members (installEnclosingBackPointer).
            VL_RESTORER(m_enclosingClassp);
            m_enclosingClassp = nodep;
            iterateChildren(nodep);
        }
    }

    فراغ بازدید(AstCoverpoint* nodep) override {
        UINFO(9, "Found coverpoint: " << nodep->name());
        m_coverpoints.افزودن_پشت(nodep);
        m_coverpointMap.emplace(nodep->name(), nodep);
        iterateChildren(nodep);
    }

    فراغ بازدید(AstCoverCross* nodep) override {
        UINFO(9, "Found cross: " << nodep->name());
        m_coverCrosses.افزودن_پشت(nodep);
        iterateChildren(nodep);
    }

    // V3Width leaves only the covergroup-level options lowerCovergroupOptions() takes
    فراغ بازدید(Ast_Cg_Option_تخصیص_بده* nodep) override { m_cgOptions.افزودن_پشت(nodep); }

    // A package, interface, or module, so the design unit declaring the classes within
    فراغ بازدید(Ast_گره_Module* nodep) override {
        VL_RESTORER(m_unitp);
        m_unitp = nodep;
        iterateChildren(nodep);
    }

    فراغ بازدید(Ast_گره* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    FunctionalCoverageVisitor(AstNetlist* nodep,
                              ثابت std::مجموعه<ثابت AstVar*>& mergeableTypeOptions)
        : m_mergeableTypeOptions{mergeableTypeOptions} {
        iterate(nodep);
    }
    ~FunctionalCoverageVisitor() override = افتراضي;
};

//######################################################################
// Functional coverage class functions

فراغ V3Covergroup::covergroup(AstNetlist* nodep) {
    UINFO(4, __FUNCTION__ << ": ");
    ثابت Covergroup_تخصیص_بده_Valid_Visitor validVisitor{nodep};
    إذا (!validVisitor.valid()) V3Error::abortIfErrors();
    {  // Destruct before checking
        FunctionalCoverageVisitor{nodep, validVisitor.mergeableTypeOptions()};
    }
    V3Global::dump_Check_Global_درخت("coveragefunc", 0, dump_درخت_Either_Level() >= 3);
}
